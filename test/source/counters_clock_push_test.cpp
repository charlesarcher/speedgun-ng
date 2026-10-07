// ============================================================================
// TDD test for the shipped clock and push providers (T034; US4
// scenarios 1..5; scenario 6 routes to counters_trap_fixture).
//
// Covers: monotonic/thread-CPU/process-CPU deltas positive and within
// calibration tolerance of each other on CPU-bound work (FR-033);
// add(1000) between samples folding to exactly 1000 with plain
// non-atomic increment and sample-read (FR-035, R-008); bytes /
// monotonic folding to the byte rate with standard disclosure
// (FR-019); the machine catalog listing clock leaves and push
// counters countable with descriptions and achieved read mode
// (FR-009); and, wherever the build executes the time-stamp
// instruction, the tsc leaf countable at fast tick mode with no rate
// attached, its frequency field 0 and its scaled flag false (FR-001,
// FR-002, FR-011). Hand-computed expectations, frameworkless
// check()/fail() convention. Registration precedes the open
// boundary.
// ============================================================================

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

// The raw time-stamp entry publishes exactly where the build executes the
// instruction (specs/008-timestamp-counter FR-001), so the presence checks
// are guarded on the same condition the provider uses.
#if (defined(__x86_64__) || defined(__i386__)) && !defined(_MSC_VER)
#  define SG_TEST_HAS_TSC 1
#else
#  define SG_TEST_HAS_TSC 0
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS CLOCK PUSH TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::Availability;
using sg::counters::CatalogEntry;
using sg::counters::ClockProvider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::LeafSet;
using sg::counters::Object;
using sg::counters::PointSink;
using sg::counters::PushCounter;
using sg::counters::PushProvider;
using sg::counters::ReadMode;
using sg::counters::Scope;
using sg::counters::System;
using sg::counters::Target;
using sg::counters::Unit;

using Events = Dim<0, 1>;
using TimeDim = Dim<1, 0>;

auto findEntry(const std::vector<CatalogEntry>& entries,
               const std::string_view name) -> const CatalogEntry*
{
  for (const auto& entry : entries) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

auto contains(std::string_view haystack, std::string_view needle) -> bool
{
  return haystack.find(needle) != std::string_view::npos;
}

// CPU-bound work: a few tens of milliseconds of scalar arithmetic.
auto burnCpu() -> void
{
  volatile double acc = 0.0;
  for (int i = 0; i < 30'000'000; ++i) {
    acc += i * 1.00000011;
  }
  static_cast<void>(acc);
}

// Scenario 1: the three clock windows are positive and mutually
// consistent on CPU-bound work (FR-033).
auto clockWindowsScenario() -> void
{
  struct WindowReading
  {
    double monoNs = 0.0;
    double threadNs = 0.0;
    double processNs = 0.0;
  };

  const auto machine = *System::local().object("machine");
  const Expression<TimeDim> mono {*machine.counter<TimeDim>("monotonic")};
  const Expression<TimeDim> thread {*machine.counter<TimeDim>("thread_cpu")};
  const Expression<TimeDim> process {*machine.counter<TimeDim>("process_cpu")};
  auto compiled = compile(System::local(), mono, thread, process);
  if (!compiled.has_value()) {
    fail("clock plan compiles");
  }
  const auto measure = [&]()
  {
    Scope window {*compiled};
    window.start();
    burnCpu();
    window.finish();
    return WindowReading {window.metric(mono).value,
                          window.metric(thread).value,
                          window.metric(process).value};
  };

  // Wall time also advances while the thread is descheduled, and the
  // instrumented coverage build stalls it for whole windows, so the ratio
  // below measures the host's scheduling as much as the counters. The
  // claim is that the counters agree on a window the thread ran for, so
  // the loop keeps the cleanest sample. Retrying cannot carry a wrong
  // delta: a counter that mismeasures disagrees in every sample.
  constexpr int kAttempts = 5;
  WindowReading cleanest;
  for (int attempt = 0; attempt < kAttempts; ++attempt) {
    const WindowReading sample = measure();
    check(sample.monoNs > 1.0e6, "monotonic window is positive on busy work");
    check(sample.threadNs > 0.0, "thread CPU is positive on busy work");
    const double skewNs = sample.monoNs - sample.threadNs;
    if (attempt == 0 || skewNs < cleanest.monoNs - cleanest.threadNs) {
      cleanest = sample;
    }
  }
  check(cleanest.threadNs <= 1.5 * cleanest.monoNs + 1.0e7,
        "thread CPU stays within tolerance of wall time");
  check(cleanest.monoNs <= 2.0 * cleanest.threadNs + 2.0e7,
        "thread CPU stays within tolerance below wall time on busy work");
  check(cleanest.processNs >= 0.9 * cleanest.threadNs,
        "process CPU covers at least the thread CPU");
}

// Scenario 2: add(1000) between two samples folds to exactly 1000
// (FR-035).
auto pushExactScenario(PushCounter& bytesHandle) -> void
{
  const auto machine = *System::local().object("machine");
  const Expression<Events> bytes {*machine.counter<Events>("bytes")};
  auto compiled = compile(System::local(), bytes);
  if (!compiled.has_value()) {
    fail("push plan compiles");
  }
  auto rec = compiled->recorder(2);
  rec.sample();
  bytesHandle.add(1000);
  rec.sample();
  const auto folded = bytes.fold(rec.view());
  check(sameDouble(folded.value, 1000.0),
        "add(1000) between samples folds to exactly 1000");
  check(sameDouble(folded.runningRatio, 1.0),
        "push fold carries the standard disclosure");
}

// Scenario 3: bytes / monotonic folds to the byte rate with standard
// disclosure; rate x window reconstitutes the pushed total (FR-019,
// FR-035).
auto byteRateScenario(PushCounter& bytesHandle) -> void
{
  const auto machine = *System::local().object("machine");
  const Expression<Events> bytes {*machine.counter<Events>("bytes")};
  const Expression<TimeDim> mono {*machine.counter<TimeDim>("monotonic")};
  const auto rate = bytes / mono;
  auto compiled = compile(System::local(), rate, mono);
  if (!compiled.has_value()) {
    fail("byte rate plan compiles");
  }
  Scope window {*compiled};
  window.start();
  bytesHandle.add(3000);
  window.finish();
  const auto rateResult = window.metric(rate);
  const auto monoNs = window.metric(mono).value;
  check(rateResult.value > 0.0, "the byte rate is positive");
  check(std::fabs(rateResult.value * monoNs - 3000.0) <= 3.0e-6,
        "rate x window reconstitutes the pushed total");
  check(sameDouble(rateResult.runningRatio, 1.0),
        "the composite carries the standard disclosure");
}

// Scenario 5 (US1): the raw time-stamp entry publishes wherever the build
// executes the instruction and carries a count with no rate
// (specs/008-timestamp-counter FR-001, FR-002). The checks read the
// entry's own fields, so they hold on a host that publishes a frequency
// and on this one, which publishes none.
auto tscScenario(const CatalogEntry* tsc) -> void
{
  if (tsc == nullptr) {
    std::printf("SKIP scenario 5: this build does not execute the "
                "time-stamp instruction, so no entry is published\n");
    return;
  }
  check(tsc->mode == ReadMode::FAST_TSC,
        "tsc reports the fast single-instruction read mode (FR-001)");
  check(tsc->avail == Availability::COUNTABLE,
        "the raw tsc entry is countable (FR-002)");
  check(tsc->unit == Unit::NONE,
        "the raw tsc entry carries a count, not a duration (FR-002)");
  check(tsc->frequencyHz == 0,
        "the raw tsc entry attaches no frequency (FR-002)");
  check(!tsc->scaled, "the raw tsc entry attaches no scaled flag (FR-002)");
  check(contains(tsc->description, "raw"),
        "the tsc description states that the count is raw (FR-002)");
  std::printf("tsc: mode fast_tsc, unit none, frequency 0, description '%s'\n",
              std::string(tsc->description).c_str());
}

// Scenario 4: the machine catalog lists clock leaves and the push
// counter countable with descriptions and achieved read modes
// (FR-009).
auto catalogScenario() -> void
{
  const auto machine = *System::local().object("machine");
  const auto entries = machine.counters();
  for (std::string_view name : {"monotonic", "thread_cpu", "process_cpu"}) {
    const auto* entry = findEntry(entries, name);
    check(entry != nullptr, "machine lists the clock leaf");
    check(entry->avail == Availability::COUNTABLE, "clock leaf is countable");
    check(entry->mode == ReadMode::SYSCALL,
          "clock leaf reports the syscall read mode");
    check(!entry->description.empty(), "clock leaf is described");
  }
  const auto* bytes = findEntry(entries, "bytes");
  check(bytes != nullptr, "machine lists the push counter");
  check(bytes->avail == Availability::COUNTABLE, "push counter is countable");
  check(bytes->mode == ReadMode::PUSH_LOAD,
        "push counter reports the push-load read mode");
  check(!bytes->description.empty(), "push counter is described");

  const auto* tsc = findEntry(entries, "tsc");
#if SG_TEST_HAS_TSC
  check(tsc != nullptr,
        "the tsc entry publishes wherever the build executes the "
        "instruction, whatever frequency the platform publishes (FR-001)");
#else
  check(tsc == nullptr,
        "a build without the instruction publishes no tsc entry (FR-001)");
#endif
  tscScenario(tsc);
}

// A push handle names the counter it was declared for, so a caller can
// report which cell a number came from (FR-035).
auto pushNameScenario(const PushCounter& first,
                      const PushCounter& second) -> void
{
  check(first.name() == "bytes",
        "a push handle names the counter it was declared for (FR-035)");
  check(second.name() == "records",
        "a second push handle names its own counter (FR-035)");
  check(first.name() != second.name(),
        "two push handles name two distinct counters (FR-035)");
}

// A window opened directly over addresses a provider does not serve.
// Every address the system resolves names a published leaf, so these
// refusals are reachable only through the open contract itself
// (FR-011, T066).
auto openRefusalScenario() -> void
{
  const Target where {};
  PushProvider pushes;
  static_cast<void>(
      pushes.addCounter("bytes", "bytes", "hot-path bytes written"));
  check(pushes.open(LeafSet {.addresses = {"machine/bytes"}}, where) != nullptr,
        "a declared push counter opens a window");
  check(pushes.open(LeafSet {.addresses = {"machine/nosuchcounter"}}, where)
            == nullptr,
        "an address naming an undeclared push counter opens no window");
  check(pushes.open(LeafSet {.addresses = {"other/bytes"}}, where) == nullptr,
        "an address on another object opens no window");

  ClockProvider clocks;
  check(clocks.open(LeafSet {.addresses = {"machine/monotonic"}}, where)
            != nullptr,
        "a published clock leaf opens a window");
  check(clocks.open(LeafSet {.addresses = {"machine/nosuchclock"}}, where)
            == nullptr,
        "an address naming no published clock leaf opens no window");
  check(
      clocks.open(LeafSet {.addresses = {"other/monotonic"}}, where) == nullptr,
      "an address on another object opens no window");

  // The time-stamp leaf publishes exactly where the build executes the
  // instruction (FR-001, FR-011). A build without it omits the leaf
  // and refuses the open. The fixture asks the catalog which, and holds
  // the open to the same answer (FR-003).
  const auto machine = *System::local().object("machine");
  bool catalogHasTsc = false;
  for (const auto& entry : machine.counters()) {
    if (entry.name == "tsc") {
      catalogHasTsc = true;
    }
  }
  const auto tscWindow =
      clocks.open(LeafSet {.addresses = {"machine/tsc"}}, where);
  check((tscWindow != nullptr) == catalogHasTsc,
        "the time-stamp entry opens exactly where the catalog publishes it "
        "(specs/008-timestamp-counter FR-003)");
  std::printf("clock: the catalog publishes the tsc entry: %s\n",
              catalogHasTsc ? "yes" : "no");
}

}  // namespace

// A window opened with the default disclosure column writes its leaves
// and writes no disclosure. compile() always names a column. This arm
// opens the provider directly (FR-007).
auto sampleWithoutDisclosure() -> void
{
  const Target where {};
  ClockProvider clocks;
  auto clockWindow =
      clocks.open(LeafSet {.addresses = {"machine/monotonic"}}, where);
  if (clockWindow == nullptr) {
    fail("the monotonic leaf opens with no disclosure column");
  }
  std::vector<std::uint64_t> clockColumns(1, 0);
  PointSink clockSink(clockColumns.data(), 1, 1, 1, 0);
  clockWindow->readPoints(clockSink);
  check(clockColumns[0] != 0,
        "a clock sample with no disclosure column still writes the leaf "
        "(FR-007)");

  PushProvider pushes;
  auto handle = pushes.addCounter("quiet", "ops", "opened with no disclosure");
  handle.add(3);
  auto pushWindow =
      pushes.open(LeafSet {.addresses = {"machine/quiet"}}, where);
  if (pushWindow == nullptr) {
    fail("the push leaf opens with no disclosure column");
  }
  std::vector<std::uint64_t> pushColumns(1, 0);
  PointSink pushSink(pushColumns.data(), 1, 1, 1, 0);
  pushWindow->readPoints(pushSink);
  check(pushColumns[0] == 3,
        "a push sample with no disclosure column writes the leaf alone "
        "(FR-007)");
}

auto main() -> int
{
  sampleWithoutDisclosure();
  auto clock = std::make_unique<ClockProvider>();
  auto push = std::make_unique<PushProvider>();
  auto bytesHandle =
      push->addCounter("bytes", "bytes", "hot-path bytes written");
  auto secondHandle = push->addCounter("records", "ops", "records appended");
  if (!System::local().registerProvider(std::move(clock)).has_value()) {
    fail("clock provider registers");
  }
  if (!System::local().registerProvider(std::move(push)).has_value()) {
    fail("push provider registers");
  }

  catalogScenario();
  pushNameScenario(bytesHandle, secondHandle);
  clockWindowsScenario();
  pushExactScenario(bytesHandle);
  byteRateScenario(bytesHandle);
  openRefusalScenario();

  std::printf("counters_clock_push_test PASS: clock, push, and composites\n");
  return 0;
}
