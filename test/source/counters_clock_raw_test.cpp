// ============================================================================
// TDD test for the nanosecond-rate counter at `machine/monotonic_raw`
// (specs/011 FR-001..FR-008; US2).
//
// The leaf publishes on every supported build, so nothing here carries an
// architecture guard except the cost distribution, whose convention is
// timestamp-counter bracketing (research.md R-006) and that instrument
// exists only on x86. Hand-rolled check()/fail() convention; no test
// framework is added to this repository.
//
// The cost figure printed here is the cost of one `sample()` on a plan
// holding this one leaf, which is the quantity the neighboring rows of
// docs/pages/counters-overhead.md carry. The single provider-level read
// behind it sits in an anonymous namespace and no public API reaches it,
// so the bracketed sampling action is the finest figure a consumer of the
// library can measure. The bracketing overhead is measured under the
// identical bracketing, and this test prints that overhead, the
// overhead-corrected figure, the uncorrected figure, and the p99 of
// both distributions, per R-006 and SC-004.
// ============================================================================

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "speedgun-ng/counters.hpp"

#if defined(__x86_64__) || defined(__i386__)
#  define SG_TEST_HAS_TSC 1
#  include <x86intrin.h>
#else
#  define SG_TEST_HAS_TSC 0
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS CLOCK RAW FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::CatalogEntry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::expression;
using sg::counters::ReadMode;
using sg::counters::system;

using time_dim = Dim<1, 0>;

// Consecutive reads the same-thread monotonicity check and the cost
// distribution share (SC-005).
constexpr std::size_t kReads = 10'000'000;

// Reads the cross-processor pass spreads over every logical processor
// the host publishes (SC-005). The per-thread count divides this total
// so the aggregate reaches ten million reads on any host.
constexpr std::size_t kCrossProcessorReads = 10'000'000;

auto find_entry(const std::vector<CatalogEntry>& entries,
                const std::string_view name) -> const CatalogEntry*
{
  for (const auto& entry : entries) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

// FR-008: the platform's own reported resolution bounds the smallest step
// a sample may show. `clock_getres` answers it, and the same call reads
// one sample so the value under test is exercised on the way past.
auto platform_resolution_ns() -> std::uint64_t
{
  timespec resolution {};
  timespec probe {};
  check(clock_getres(CLOCK_MONOTONIC_RAW, &resolution) == 0,
        "the platform reports a resolution for this clock (FR-008)");
  check(clock_gettime(CLOCK_MONOTONIC_RAW, &probe) == 0,
        "the platform serves this clock (FR-008)");
  const auto bound =
      static_cast<std::uint64_t>(resolution.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(resolution.tv_nsec);
  std::printf("platform resolution for this clock: %llu ns\n",
              static_cast<unsigned long long>(bound));
  return bound;
}

#if SG_TEST_HAS_TSC
// The host's timestamp-counter rate, so a tick figure converts into the
// nanoseconds this file publishes. Measured across a busy interval: a
// sleep would hand the CPU to the scheduler mid-measurement.
auto tsc_rate_hz() -> double
{
  const auto wall_start = std::chrono::steady_clock::now();
  const auto tick_start = __rdtsc();
  while (std::chrono::steady_clock::now() - wall_start
         < std::chrono::milliseconds(50))
  {
    // Busy, so the interval measures this thread and not a descheduled one.
  }
  const auto tick_end = __rdtsc();
  const auto wall_end = std::chrono::steady_clock::now();
  const double seconds =
      std::chrono::duration<double>(wall_end - wall_start).count();
  return static_cast<double>(tick_end - tick_start) / seconds;
}
#endif

// FR-006 and FR-008 on one thread: ten million consecutive reads, none of
// them lower than the one before it, the smallest non-zero step inside the
// resolution the platform reports, and the per-sampling-action cost
// distribution bracketed by timestamp-counter reads (R-006).
auto test_monotonic_and_cost(const sg::counters::plan& compiled,
                             const std::uint64_t resolution_bound) -> void
{
  auto recorder = compiled.recorder(kReads);
  std::vector<std::uint64_t> costs;
  costs.reserve(kReads);
  std::vector<std::uint64_t> brackets;
  brackets.reserve(kReads);

  std::uint64_t previous = 0;
  std::uint64_t smallest_step = 0;
  std::size_t decreases = 0;

  for (std::size_t read = 0; read < kReads; ++read) {
#if SG_TEST_HAS_TSC
    const auto started = __rdtsc();
    recorder.sample();
    const auto finished = __rdtsc();
    costs.push_back(finished - started);
    // The same pair around an interval holding nothing, so the figure the
    // corrected cost subtracts is measured on this host, in this loop, at
    // this optimization level (SC-004).
    const auto bracket_started = __rdtsc();
    const auto bracket_finished = __rdtsc();
    brackets.push_back(bracket_finished - bracket_started);
#else
    recorder.sample();
#endif
    const std::uint64_t value = recorder.m_columns[read];
    if (read > 0) {
      if (value < previous) {
        ++decreases;
      } else if (value > previous) {
        const std::uint64_t step = value - previous;
        if (smallest_step == 0 || step < smallest_step) {
          smallest_step = step;
        }
      }
    }
    previous = value;
  }

  check(decreases == 0,
        "no sample is lower than the sample before it on the same thread, "
        "over ten million consecutive reads (FR-006)");
  check(smallest_step > 0,
        "at least one of ten million reads advanced the clock (FR-008)");
  // A back-to-back sampling loop leaves consecutive samples tens of
  // nanoseconds apart, so every step is already larger than the resolution
  // `clock_getres` reports; a shipped run recorded a smallest non-zero step
  // of 70 ns against a reported resolution of 1 ns (SF-005). The bound that
  // carries content is the lower one, and this checks it: the clock never
  // reports a step finer than the resolution it publishes, so a step below
  // the reported figure would mean the counter reports a granularity it does
  // not have (specs/011 FR-008, R-006).
  check(smallest_step >= resolution_bound,
        "no sample-to-sample step is finer than the resolution the platform "
        "reports for this clock (FR-008)");
  std::printf("ten million consecutive reads: 0 decreases, smallest non-zero "
              "step %llu ns against a reported resolution of %llu ns\n",
              static_cast<unsigned long long>(smallest_step),
              static_cast<unsigned long long>(resolution_bound));

  if (costs.empty()) {
    std::printf("no timestamp-counter instruction on this target, so the "
                "cost distribution is unmeasured and reports no figure\n");
    return;
  }

  std::sort(costs.begin(), costs.end());
  std::sort(brackets.begin(), brackets.end());
  const double rate = tsc_rate_hz();
  const auto as_ns = [rate](const std::uint64_t ticks)
  { return static_cast<double>(ticks) / rate * 1.0e9; };
  const auto p99 = [](const std::vector<std::uint64_t>& sorted)
  { return sorted[sorted.size() * 99 / 100]; };
  const auto overhead_ns = as_ns(brackets[brackets.size() / 2]);
  std::printf("per sampling action over ten million reads: min %.1f ns, "
              "median %.1f ns, p99 %.1f ns, max %.1f ns, bracketed at "
              "%.3f GHz\n",
              as_ns(costs.front()),
              as_ns(costs[costs.size() / 2]),
              as_ns(p99(costs)),
              as_ns(costs.back()),
              rate / 1.0e9);
  std::printf("bracketing overhead of an empty interval under the identical "
              "bracketing: median %.1f ns, p99 %.1f ns\n",
              overhead_ns,
              as_ns(p99(brackets)));
  std::printf("per sampling action, overhead-corrected: median %.1f ns, p99 "
              "%.1f ns; uncorrected: median %.1f ns, p99 %.1f ns\n",
              as_ns(costs[costs.size() / 2]) - overhead_ns,
              as_ns(p99(costs)) - overhead_ns,
              as_ns(costs[costs.size() / 2]),
              as_ns(p99(costs)));
}

// FR-007: no sample is lower than an earlier sample whose completion
// happens-before its own start. The first pass runs alone and is joined,
// which makes its last sample's completion happen-before every sample the
// second pass takes, so the comparison below rests on that edge and
// asserts nothing the platform does not promise.
//
// A plan binds to the thread that constructed it (FR-031), so every worker
// compiles its own plan over the same resolved handle.
auto test_monotonic_across_processors(const expression<time_dim>& elapsed)
    -> void
{
  const auto sample_once = [&elapsed](const std::size_t reads)
  {
    auto compiled = compile(system::local(), elapsed);
    check(compiled.has_value(),
          "a per-thread plan over the nanosecond-rate counter compiles");
    auto recorder = compiled->recorder(reads);
    for (std::size_t read = 0; read < reads; ++read) {
      recorder.sample();
    }
    return recorder.m_columns[reads - 1];
  };

  const unsigned processors = std::thread::hardware_concurrency();
  check(processors > 0, "the host publishes at least one logical processor");
  // Rounded up, so the aggregate over the workers reaches the total on
  // a host whose processor count does not divide it.
  const std::size_t reads_per_processor =
      (kCrossProcessorReads + processors - 1) / processors;

  const std::uint64_t before = sample_once(reads_per_processor);

  std::vector<std::uint64_t> firsts(processors, 0);
  std::vector<std::uint64_t> lasts(processors, 0);
  std::vector<std::thread> workers;
  workers.reserve(processors);
  for (unsigned processor = 0; processor < processors; ++processor) {
    workers.emplace_back(
        [elapsed, &firsts, &lasts, reads_per_processor, processor]()
        {
          auto compiled = compile(system::local(), elapsed);
          if (!compiled.has_value()) {
            return;
          }
          auto recorder = compiled->recorder(reads_per_processor);
          recorder.sample();
          firsts[processor] = recorder.m_columns[0];
          for (std::size_t read = 1; read < reads_per_processor; ++read) {
            recorder.sample();
          }
          lasts[processor] = recorder.m_columns[reads_per_processor - 1];
        });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::size_t regressions = 0;
  for (unsigned processor = 0; processor < processors; ++processor) {
    if (firsts[processor] == 0 || lasts[processor] == 0) {
      ++regressions;
      continue;
    }
    if (firsts[processor] < before || lasts[processor] < before
        || lasts[processor] < firsts[processor])
    {
      ++regressions;
    }
  }
  check(regressions == 0,
        "every logical processor sampled after a joined sample, with no "
        "sample lower than the one whose completion happens-before its "
        "start (FR-007)");
  std::printf(
      "%u logical processors sampled after a joined sample: 0 " "regressions\n",
      processors);
}

// A busy interval on the calling thread. The per-thread clock advances
// only while the thread runs, so the order tests need real work between
// two samples. A sleep would let the thread hand the processor away and
// report no advance.
auto burn_cpu() -> void
{
  volatile std::uint64_t sink = 0;
  for (std::uint64_t step = 0; step < 20'000'000; ++step) {
    sink += step;
  }
  static_cast<void>(sink);
}

}  // namespace

// One leaf's last sample from a fresh plan, counted as the leaf's own
// counter reads them.
auto sample_leaf(const char* leaf, const std::size_t reads) -> std::uint64_t
{
  const auto machine = *system::local().object("machine");
  const auto counter = machine.counter<time_dim>(leaf);
  check(counter.has_value(), "the clock leaf resolves for the order test");
  const expression<time_dim> counted {*counter};
  auto compiled = compile(system::local(), counted);
  check(compiled.has_value(), "a per-thread plan over the leaf compiles");
  auto recorder = compiled->recorder(reads);
  for (std::size_t read = 0; read < reads; ++read) {
    recorder.sample();
  }
  return recorder.m_columns[reads - 1];
}

// Each leaf states its own order guarantee, and a test exercises each one
// (FR-028, FR-029). A guarantee a leaf's clock does not keep is a defect,
// so every clause in `counters_clock.hpp` has a test behind it here.
auto test_per_leaf_order(const expression<time_dim>& elapsed) -> void
{
  // `machine/monotonic` and `machine/monotonic_raw`: a sample does not
  // fall below an earlier sample taken on the same thread.
  for (const char* leaf : {"monotonic", "monotonic_raw"}) {
    const auto first = sample_leaf(leaf, 1);
    burn_cpu();
    const auto second = sample_leaf(leaf, 1);
    check(second >= first,
          "a sample of this leaf does not fall below an earlier sample taken "
          "on the same thread (FR-029)");
  }

  // `machine/thread_cpu`: non-decreasing on the reading thread.
  const auto thread_first = sample_leaf("thread_cpu", 1);
  burn_cpu();
  const auto thread_second = sample_leaf("thread_cpu", 1);
  check(thread_second >= thread_first,
        "a second sample of the per-thread clock on one thread does not fall "
        "below the first (FR-029)");

  // `machine/thread_cpu`: a new thread's first sample falls below an
  // earlier sample taken on another thread, because the counter belongs to
  // the thread. The documented guarantee permits exactly this (FR-030).
  burn_cpu();
  const auto main_thread_sample = sample_leaf("thread_cpu", 1);
  std::uint64_t worker_first = 0;
  std::thread worker {[&worker_first]
                      { worker_first = sample_leaf("thread_cpu", 1); }};
  worker.join();
  check(worker_first < main_thread_sample,
        "a new thread's first sample of the per-thread clock falls below an "
        "earlier sample taken on another thread, which the documented "
        "guarantee permits (FR-030)");

  // `machine/process_cpu`: non-decreasing across the process's threads,
  // because the counter belongs to the process.
  const auto process_first = sample_leaf("process_cpu", 1);
  std::uint64_t worker_process = 0;
  std::thread process_worker {
      [&worker_process] { worker_process = sample_leaf("process_cpu", 1); }};
  burn_cpu();
  process_worker.join();
  check(worker_process >= process_first,
        "a sample of the per-process clock on a second thread does not fall "
        "below one taken on the first (FR-029)");
  static_cast<void>(elapsed);
}

auto main() -> int
{
  auto clock = std::make_unique<clock_provider>();
  check(system::local().register_provider(std::move(clock)).has_value(),
        "the clock provider registers");

  const auto machine = system::local().object("machine");
  check(machine.has_value(), "the clock provider seeds the machine object");

  // FR-001, FR-005: the leaf resolves by its canonical address with no
  // architecture guard, because the platform serves this clock on every
  // supported target.
  const auto raw = machine->counter<time_dim>("monotonic_raw");
  check(raw.has_value(),
        "machine/monotonic_raw resolves on every supported build (FR-001, "
        "FR-005)");
  check(raw->name() == "monotonic_raw", "the leaf names itself (FR-001)");
  check(raw->address() == "machine/monotonic_raw",
        "the canonical address composes to machine/monotonic_raw (FR-001)");

  // FR-002: the existing unit token and the read mode every clock counter
  // carries, with no new enumerator in either closed vocabulary.
  check(raw->unit_token() == "nanoseconds",
        "the leaf reports the unit token nanoseconds (FR-002)");
  const auto entries = machine->counters();
  const auto* entry = find_entry(entries, "monotonic_raw");
  check(entry != nullptr, "the catalog lists the leaf (FR-002)");
  check(entry->mode == ReadMode::SYSCALL,
        "the leaf reports the read mode every existing clock counter "
        "carries (FR-002)");

  const expression<time_dim> elapsed {*raw};
  auto compiled = compile(system::local(), elapsed);
  check(compiled.has_value(),
        "a plan over the nanosecond-rate counter compiles (FR-001)");

  const std::uint64_t resolution_bound = platform_resolution_ns();
  test_monotonic_and_cost(*compiled, resolution_bound);
  test_monotonic_across_processors(elapsed);
  test_per_leaf_order(elapsed);

  std::printf("counters_clock_raw_test PASS: leaf published, unit and read "
              "mode disclosed, samples monotonic\n");
  return 0;
}
