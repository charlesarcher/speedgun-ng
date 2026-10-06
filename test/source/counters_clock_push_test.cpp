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

auto same_double(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::expression;
using sg::counters::leaf_set;
using sg::counters::object;
using sg::counters::point_sink;
using sg::counters::push_counter;
using sg::counters::push_provider;
using sg::counters::read_mode;
using sg::counters::scope;
using sg::counters::system;
using sg::counters::target;
using sg::counters::unit;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

auto find_entry(const std::vector<catalog_entry>& entries,
                const std::string_view name) -> const catalog_entry*
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
auto burn_cpu() -> void
{
  volatile double acc = 0.0;
  for (int i = 0; i < 30'000'000; ++i) {
    acc += i * 1.00000011;
  }
  static_cast<void>(acc);
}

// Scenario 1: the three clock windows are positive and mutually
// consistent on CPU-bound work (FR-033).
auto clock_windows_scenario() -> void
{
  struct window_reading
  {
    double mono_ns = 0.0;
    double thread_ns = 0.0;
    double process_ns = 0.0;
  };

  const auto machine = *system::local().object("machine");
  const expression<time_dim> mono {*machine.counter<time_dim>("monotonic")};
  const expression<time_dim> thread {*machine.counter<time_dim>("thread_cpu")};
  const expression<time_dim> process {
      *machine.counter<time_dim>("process_cpu")};
  auto compiled = compile(system::local(), mono, thread, process);
  if (!compiled.has_value()) {
    fail("clock plan compiles");
  }
  const auto measure = [&]()
  {
    scope window {*compiled};
    window.start();
    burn_cpu();
    window.finish();
    return window_reading {window.metric(mono).value,
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
  window_reading cleanest;
  for (int attempt = 0; attempt < kAttempts; ++attempt) {
    const window_reading sample = measure();
    check(sample.mono_ns > 1.0e6, "monotonic window is positive on busy work");
    check(sample.thread_ns > 0.0, "thread CPU is positive on busy work");
    const double skew_ns = sample.mono_ns - sample.thread_ns;
    if (attempt == 0 || skew_ns < cleanest.mono_ns - cleanest.thread_ns) {
      cleanest = sample;
    }
  }
  check(cleanest.thread_ns <= 1.5 * cleanest.mono_ns + 1.0e7,
        "thread CPU stays within tolerance of wall time");
  check(cleanest.mono_ns <= 2.0 * cleanest.thread_ns + 2.0e7,
        "thread CPU stays within tolerance below wall time on busy work");
  check(cleanest.process_ns >= 0.9 * cleanest.thread_ns,
        "process CPU covers at least the thread CPU");
}

// Scenario 2: add(1000) between two samples folds to exactly 1000
// (FR-035).
auto push_exact_scenario(push_counter& bytes_handle) -> void
{
  const auto machine = *system::local().object("machine");
  const expression<events> bytes {*machine.counter<events>("bytes")};
  auto compiled = compile(system::local(), bytes);
  if (!compiled.has_value()) {
    fail("push plan compiles");
  }
  auto rec = compiled->recorder(2);
  rec.sample();
  bytes_handle.add(1000);
  rec.sample();
  const auto folded = bytes.fold(rec.view());
  check(same_double(folded.value, 1000.0),
        "add(1000) between samples folds to exactly 1000");
  check(same_double(folded.running_ratio, 1.0),
        "push fold carries the standard disclosure");
}

// Scenario 3: bytes / monotonic folds to the byte rate with standard
// disclosure; rate x window reconstitutes the pushed total (FR-019,
// FR-035).
auto byte_rate_scenario(push_counter& bytes_handle) -> void
{
  const auto machine = *system::local().object("machine");
  const expression<events> bytes {*machine.counter<events>("bytes")};
  const expression<time_dim> mono {*machine.counter<time_dim>("monotonic")};
  const auto rate = bytes / mono;
  auto compiled = compile(system::local(), rate, mono);
  if (!compiled.has_value()) {
    fail("byte rate plan compiles");
  }
  scope window {*compiled};
  window.start();
  bytes_handle.add(3000);
  window.finish();
  const auto rate_result = window.metric(rate);
  const auto mono_ns = window.metric(mono).value;
  check(rate_result.value > 0.0, "the byte rate is positive");
  check(std::fabs(rate_result.value * mono_ns - 3000.0) <= 3.0e-6,
        "rate x window reconstitutes the pushed total");
  check(same_double(rate_result.running_ratio, 1.0),
        "the composite carries the standard disclosure");
}

// Scenario 5 (US1): the raw time-stamp entry publishes wherever the build
// executes the instruction and carries a count with no rate
// (specs/008-timestamp-counter FR-001, FR-002). The checks read the
// entry's own fields, so they hold on a host that publishes a frequency
// and on this one, which publishes none.
auto tsc_scenario(const catalog_entry* tsc) -> void
{
  if (tsc == nullptr) {
    std::printf("SKIP scenario 5: this build does not execute the "
                "time-stamp instruction, so no entry is published\n");
    return;
  }
  check(tsc->mode == read_mode::fast_tsc,
        "tsc reports the fast single-instruction read mode (FR-001)");
  check(tsc->avail == availability::countable,
        "the raw tsc entry is countable (FR-002)");
  check(tsc->unit == unit::none,
        "the raw tsc entry carries a count, not a duration (FR-002)");
  check(tsc->frequency_hz == 0,
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
auto catalog_scenario() -> void
{
  const auto machine = *system::local().object("machine");
  const auto entries = machine.counters();
  for (std::string_view name : {"monotonic", "thread_cpu", "process_cpu"}) {
    const auto* entry = find_entry(entries, name);
    check(entry != nullptr, "machine lists the clock leaf");
    check(entry->avail == availability::countable, "clock leaf is countable");
    check(entry->mode == read_mode::syscall,
          "clock leaf reports the syscall read mode");
    check(!entry->description.empty(), "clock leaf is described");
  }
  const auto* bytes = find_entry(entries, "bytes");
  check(bytes != nullptr, "machine lists the push counter");
  check(bytes->avail == availability::countable, "push counter is countable");
  check(bytes->mode == read_mode::push_load,
        "push counter reports the push-load read mode");
  check(!bytes->description.empty(), "push counter is described");

  const auto* tsc = find_entry(entries, "tsc");
#if SG_TEST_HAS_TSC
  check(tsc != nullptr,
        "the tsc entry publishes wherever the build executes the "
        "instruction, whatever frequency the platform publishes (FR-001)");
#else
  check(tsc == nullptr,
        "a build without the instruction publishes no tsc entry (FR-001)");
#endif
  tsc_scenario(tsc);
}

// A push handle names the counter it was declared for, so a caller can
// report which cell a number came from (FR-035).
auto push_name_scenario(const push_counter& first, const push_counter& second)
    -> void
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
auto open_refusal_scenario() -> void
{
  const target where {};
  push_provider pushes;
  static_cast<void>(
      pushes.add_counter("bytes", "bytes", "hot-path bytes written"));
  check(
      pushes.open(leaf_set {.addresses = {"machine/bytes"}}, where) != nullptr,
      "a declared push counter opens a window");
  check(pushes.open(leaf_set {.addresses = {"machine/nosuchcounter"}}, where)
            == nullptr,
        "an address naming an undeclared push counter opens no window");
  check(pushes.open(leaf_set {.addresses = {"other/bytes"}}, where) == nullptr,
        "an address on another object opens no window");

  clock_provider clocks;
  check(clocks.open(leaf_set {.addresses = {"machine/monotonic"}}, where)
            != nullptr,
        "a published clock leaf opens a window");
  check(clocks.open(leaf_set {.addresses = {"machine/nosuchclock"}}, where)
            == nullptr,
        "an address naming no published clock leaf opens no window");
  check(clocks.open(leaf_set {.addresses = {"other/monotonic"}}, where)
            == nullptr,
        "an address on another object opens no window");

  // The time-stamp leaf publishes exactly where the build executes the
  // instruction (FR-001, FR-011). A build without it omits the leaf
  // and refuses the open. The fixture asks the catalog which, and holds
  // the open to the same answer (FR-003).
  const auto machine = *system::local().object("machine");
  bool catalog_has_tsc = false;
  for (const auto& entry : machine.counters()) {
    if (entry.name == "tsc") {
      catalog_has_tsc = true;
    }
  }
  const auto tsc_window =
      clocks.open(leaf_set {.addresses = {"machine/tsc"}}, where);
  check((tsc_window != nullptr) == catalog_has_tsc,
        "the time-stamp entry opens exactly where the catalog publishes it "
        "(specs/008-timestamp-counter FR-003)");
  std::printf("clock: the catalog publishes the tsc entry: %s\n",
              catalog_has_tsc ? "yes" : "no");
}

}  // namespace

// A window opened with the default disclosure column writes its leaves
// and writes no disclosure. compile() always names a column. This arm
// opens the provider directly (FR-007).
auto sample_without_disclosure() -> void
{
  const target where {};
  clock_provider clocks;
  auto clock_window =
      clocks.open(leaf_set {.addresses = {"machine/monotonic"}}, where);
  if (clock_window == nullptr) {
    fail("the monotonic leaf opens with no disclosure column");
  }
  std::vector<std::uint64_t> clock_columns(1, 0);
  point_sink clock_sink(clock_columns.data(), 1, 1, 1, 0);
  clock_window->read_points(clock_sink);
  check(clock_columns[0] != 0,
        "a clock sample with no disclosure column still writes the leaf "
        "(FR-007)");

  push_provider pushes;
  auto handle = pushes.add_counter("quiet", "ops", "opened with no disclosure");
  handle.add(3);
  auto push_window =
      pushes.open(leaf_set {.addresses = {"machine/quiet"}}, where);
  if (push_window == nullptr) {
    fail("the push leaf opens with no disclosure column");
  }
  std::vector<std::uint64_t> push_columns(1, 0);
  point_sink push_sink(push_columns.data(), 1, 1, 1, 0);
  push_window->read_points(push_sink);
  check(push_columns[0] == 3,
        "a push sample with no disclosure column writes the leaf alone "
        "(FR-007)");
}

auto main() -> int
{
  sample_without_disclosure();
  auto clock = std::make_unique<clock_provider>();
  auto push = std::make_unique<push_provider>();
  auto bytes_handle =
      push->add_counter("bytes", "bytes", "hot-path bytes written");
  auto second_handle = push->add_counter("records", "ops", "records appended");
  if (!system::local().register_provider(std::move(clock)).has_value()) {
    fail("clock provider registers");
  }
  if (!system::local().register_provider(std::move(push)).has_value()) {
    fail("push provider registers");
  }

  catalog_scenario();
  push_name_scenario(bytes_handle, second_handle);
  clock_windows_scenario();
  push_exact_scenario(bytes_handle);
  byte_rate_scenario(bytes_handle);
  open_refusal_scenario();

  std::printf("counters_clock_push_test PASS: clock, push, and composites\n");
  return 0;
}
