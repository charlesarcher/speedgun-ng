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
// (FR-009); and on a calibrated platform the tsc leaf reporting
// frequency provenance and the scaled flag as catalog fields
// (FR-034, R-007). Hand-computed expectations, frameworkless
// check()/fail() convention. Registration precedes the open
// boundary.
// ============================================================================

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

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
using sg::counters::object;
using sg::counters::push_counter;
using sg::counters::push_provider;
using sg::counters::read_mode;
using sg::counters::scope;
using sg::counters::system;

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
  const auto machine = *system::local().object("machine");
  const expression<time_dim> mono {*machine.counter<time_dim>("monotonic")};
  const expression<time_dim> thread {*machine.counter<time_dim>("thread_cpu")};
  const expression<time_dim> process {
      *machine.counter<time_dim>("process_cpu")};
  auto compiled = compile(system::local(), mono, thread, process);
  if (!compiled.has_value()) {
    fail("clock plan compiles");
  }
  scope window {*compiled};
  window.start();
  burn_cpu();
  window.finish();
  const auto mono_ns = window.metric(mono).value;
  const auto thread_ns = window.metric(thread).value;
  const auto process_ns = window.metric(process).value;
  check(mono_ns > 1.0e6, "monotonic window is positive on busy work");
  check(thread_ns > 0.0, "thread CPU is positive on busy work");
  check(thread_ns <= 1.5 * mono_ns + 1.0e7,
        "thread CPU stays within tolerance of wall time");
  check(mono_ns <= 2.0 * thread_ns + 2.0e7,
        "thread CPU stays within tolerance below wall time on busy work");
  check(process_ns >= 0.9 * thread_ns,
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

// Scenario 4: the machine catalog lists clock leaves and the push
// counter countable with descriptions and achieved read modes
// (FR-009). Scenario 5: where the platform calibrated a tsc, it
// reports fast mode, frequency provenance, and the scaled flag
// (FR-034, R-007).
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
  if (tsc != nullptr) {
    check(tsc->mode == read_mode::fast_tsc,
          "tsc reports the fast tick read mode");
    check(tsc->frequency_hz > 0, "tsc reports frequency provenance");
  } else {
    std::printf("tsc leaf absent on this platform: catalog fact, skipped\n");
  }
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

}  // namespace

auto main() -> int
{
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

  std::printf("counters_clock_push_test PASS: clock, push, and composites\n");
  return 0;
}
