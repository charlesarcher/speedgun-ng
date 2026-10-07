// ============================================================================
// Sampling-cost benchmark and mode disclosure (T051; US7 scenarios 1, 6;
// SC-004, SC-010, quickstart 12).
//
// Publishes the per-action `sample()` distribution of this host's plans
// as min/median/max (Principle VII), the two regimes side by side where
// the host probes fast, and the achieved read mode of every catalog
// entry with the reason a fast mechanism was refused where it was.
// Fold cost is measured off the sampling path so the two costs stay
// separable (SC-010).
//
// Exit 0: the fast-versus-syscall comparison ran. Exit 2: this host
// publishes no probe-passing fast regime; the reason is named and the
// syscall figures were printed first. CTest's console line for a
// skipped test carries no reason, so `ctest -V` shows it. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

#include "speedgun-ng/counters_pmu.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS OVERHEAD FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::Availability;
using sg::counters::CatalogEntry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::plan;
using sg::counters::pmu_provider;
using sg::counters::ReadMode;
using sg::counters::RecorderHandle;
using sg::counters::system;

using events = Dim<0, 1>;
using time_dim = Dim<1, 0>;

// The bare baseline for the raw time-stamp read needs the instruction, so
// the measurement is guarded on the same condition the provider publishes
// the entry under (008 FR-001). A build without it skips the comparison and
// says so. It reports no figure it cannot measure.
#if (defined(__x86_64__) || defined(__i386__)) && !defined(_MSC_VER)
#  define SG_TEST_HAS_TSC 1
#else
#  define SG_TEST_HAS_TSC 0
#endif

#if SG_TEST_HAS_TSC
#  include <x86intrin.h>
#endif

// One sampling action's cost in ticks, as a distribution over repeats.
struct tick_cost
{
  std::uint64_t min = 0;
  std::uint64_t median = 0;
  std::uint64_t max = 0;
};

auto summarize_ticks(std::vector<std::uint64_t> samples) -> tick_cost
{
  std::sort(samples.begin(), samples.end());
  return tick_cost {
      samples.front(), samples[samples.size() / 2], samples.back()};
}

// Actions per timed loop and repeats of that loop. Enough actions that one
// scheduling interruption cannot dominate a repeat, enough repeats that the
// median is stable.
constexpr std::size_t kPerSampleActions = 1000;
constexpr std::size_t kPerSampleRepeats = 64;

// One sampling action's cost, bracketed by two reads and divided by the
// action count. The same bracketing measures both sides, so the difference
// between them is the library's own cost and not a difference of two timing
// methods. The library side is measured in a tight loop because that is how
// a plan samples; a lone pair measured cold costs about half again as much,
// which the distribution's minimum shows.
#if SG_TEST_HAS_TSC
auto measure_bare_tsc() -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> per_repeat;
  per_repeat.reserve(kPerSampleRepeats);
  for (std::size_t repeat = 0; repeat < kPerSampleRepeats; ++repeat) {
    std::uint64_t total = 0;
    for (std::size_t action = 0; action < kPerSampleActions; ++action) {
      const auto start = __rdtsc();
      total += __rdtsc() - start;
    }
    per_repeat.push_back(total / kPerSampleActions);
  }
  return per_repeat;
}

auto measure_library_tsc(const plan& compiled) -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> per_repeat;
  per_repeat.reserve(kPerSampleRepeats);
  for (std::size_t repeat = 0; repeat < kPerSampleRepeats; ++repeat) {
    auto actions = compiled.recorder(kPerSampleActions);
    const auto start = __rdtsc();
    for (std::size_t action = 0; action < kPerSampleActions; ++action) {
      actions.sample();
    }
    per_repeat.push_back((__rdtsc() - start) / kPerSampleActions);
  }
  return per_repeat;
}

// The host's TSC rate, so a tick figure converts into the nanoseconds the
// rest of this file publishes. Measured across a busy interval. A sleep
// would hand the CPU to the scheduler mid-measurement.
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

// The two plans FR-008 gates, measured in ticks. The nanosecond figures
// elsewhere in this file quantize to a 10 ns tick, and one tick is about a
// fifth of a 50 ns median, so the nanosecond domain cannot resolve a five
// percent bound in either direction. A tick is a finer count of the same
// work. FR-008 asks for this as a recorded measurement and asks for no
// test asserting it, so the figure prints and nothing branches on it.
auto report_gated_ticks(const plan& compiled, const std::string& label) -> void
{
  const tick_cost cost = summarize_ticks(measure_library_tsc(compiled));
  const double rate = tsc_rate_hz();
  std::printf(
      "%-24s min %4llu t   median %4llu t   max %4llu t   (median %6.1f ns)\n",
      label.c_str(),
      static_cast<unsigned long long>(cost.min),
      static_cast<unsigned long long>(cost.median),
      static_cast<unsigned long long>(cost.max),
      static_cast<double>(cost.median) / rate * 1e9);
}
#endif

// One published regime: what the plan reads, and the three numbers.
struct regime
{
  std::string label;
  double min_ns = 0.0;
  double median_ns = 0.0;
  double max_ns = 0.0;
};

auto report(const regime& cost) -> void
{
  std::printf("%-34s min %9.1f ns   median %9.1f ns   max %9.1f ns\n",
              cost.label.c_str(),
              cost.min_ns,
              cost.median_ns,
              cost.max_ns);
}

auto measure(plan& compiled, const std::string& label) -> regime
{
  // A two-row buffer is all the calibration needs; the plan's own
  // sample_overhead accessors run the distribution (FR-032).
  auto recorder = compiled.recorder(2);
  recorder.sample();
  recorder.sample();
  regime cost {.label = label,
               .min_ns = compiled.sampleOverheadNsMin(),
               .median_ns = compiled.sampleOverheadNsMedian(),
               .max_ns = compiled.sampleOverheadNsMax()};
  report(cost);
  check(cost.min_ns >= 0.0 && cost.median_ns >= cost.min_ns
            && cost.max_ns >= cost.median_ns,
        "the published distribution is ordered min <= median <= max");

  // The published floor excludes the clock reads that bracket it (FR-025).
  // The test measures the same sampling action under the same bracketing
  // and takes the median, so a floor that still carried the bracket would
  // equal this figure and fail here. No recorded timing constant decides
  // it, so the check reaches the same verdict on any host (Principle VI).
  auto timed = compiled.recorder(64);
  std::vector<double> bracketed;
  bracketed.reserve(64);
  for (int index = 0; index < 64; ++index) {
    const auto before = std::chrono::steady_clock::now();
    timed.sample();
    const auto after = std::chrono::steady_clock::now();
    bracketed.push_back(
        std::chrono::duration<double, std::nano>(after - before).count());
  }
  std::sort(bracketed.begin(), bracketed.end());
  const double bracketed_median = bracketed[bracketed.size() / 2];
  std::printf("%s: published floor median %.1f ns against %.1f ns measured "
              "under the identical bracketing\n",
              cost.label.c_str(),
              cost.median_ns,
              bracketed_median);
  check(cost.median_ns < bracketed_median,
        "the published floor excludes the two clock reads that bracket each "
        "sampling action (FR-025, FR-026)");
  return cost;
}

// Fold cost, measured over recorded points with the sampling path out
// of the loop (SC-010).
auto measure_fold(plan& compiled,
                  plan& clock_plan,
                  const sg::counters::expression<time_dim>& clock_expr,
                  const sg::counters::expression<events>& work_expr,
                  const sg::counters::expression<events>& cycle_expr,
                  const char* label) -> void
{
  auto recorder = compiled.recorder(64);
  for (int index = 0; index < 64; ++index) {
    recorder.sample();
  }
  // The bracket is the library's own monotonic clock counter, so this file
  // measures the library with the library (FR-018). Before the raw
  // time-stamp entry shipped, the catalog withheld a fast counter and this
  // call reached outside for a clock; the entry now exists, and the
  // nanosecond figures still come from the monotonic leaf, which is the one
  // library counter carrying a duration. The raw entry carries a count, so
  // it cannot stand in here, and the library attaches no rate to it that
  // would let a caller convert one into the other (FR-002).
  // The standard-library bracket runs alongside the library one so the page
  // keeps a comparable figure. Publishing both makes the bracket's own cost
  // visible, and stops one figure from silently replacing the other.
  const auto wall_before = std::chrono::steady_clock::now();
  auto bracket = clock_plan.recorder(2);
  bracket.sample();
  volatile double sink = 0.0;
  for (int index = 0; index < 1000; ++index) {
    sink += (work_expr.fold(recorder.view(), 0, 63).value
             + cycle_expr.fold(recorder.view(), 0, 63).value);
  }
  bracket.sample();
  const auto wall_after = std::chrono::steady_clock::now();
  static_cast<void>(sink);
  const auto span = clock_expr.fold(bracket.view(), 0, 1);
  const double per_fold = span.value / 1000.0;
  const double wall_per_fold =
      std::chrono::duration<double, std::nano>(wall_after - wall_before).count()
      / 1000.0;
  std::printf("%-34s %9.1f ns per first-to-last fold (bracketed by the "
              "library's monotonic counter)\n",
              label,
              per_fold);
  std::printf("%-34s %9.1f ns per first-to-last fold (bracketed by "
              "std::chrono, the comparable figure)\n",
              label,
              wall_per_fold);
}

auto describe_modes(const std::string& path) -> void
{
  const auto located = system::local().object(path);
  if (!located.has_value()) {
    return;
  }
  const auto entries = located->counters();
  std::size_t fast = 0;
  std::size_t syscall_mode = 0;
  for (const auto& entry : entries) {
    if (entry.mode == ReadMode::FAST_TSC || entry.mode == ReadMode::FAST_RDPMC)
    {
      ++fast;
    } else if (entry.mode == ReadMode::SYSCALL) {
      ++syscall_mode;
    }
  }
  std::printf("achieved modes on '%s': %zu fast, %zu syscall, %zu entries\n",
              located->description().data(),
              fast,
              syscall_mode,
              entries.size());
  for (const auto& entry : entries) {
    if (entry.mode == ReadMode::FAST_TSC || entry.mode == ReadMode::FAST_RDPMC)
    {
      std::printf("  fast leaf '%s': %s\n",
                  std::string(entry.name).c_str(),
                  std::string(entry.description).c_str());
    }
  }
}

}  // namespace

auto main() -> int
{
  auto clock = std::make_unique<clock_provider>();
  if (!system::local().register_provider(std::move(clock)).has_value()) {
    fail("clock provider registers");
  }
  auto pmu = std::make_unique<pmu_provider>();
  if (!system::local().register_provider(std::move(pmu)).has_value()) {
    fail("pmu provider registers");
  }

  const auto machine_object = system::local().object("machine");
  if (!machine_object.has_value()) {
    fail("the clock provider seeds the machine object");
  }
  const auto& machine = *machine_object;
  // A provider that seeds no cpu object leaves nothing for a counter regime
  // to compare a clock against. SKIP_RETURN_CODE 2 is registered for this
  // target, so the reason is named and the run skips, where dereferencing the
  // absent object aborted the process on the way past.
  const auto cpu_object = system::local().object("cpu");
  if (!cpu_object.has_value()) {
    std::printf("SKIP: no provider seeded a cpu object, so the counters "
                "overhead has no regime to compare against the clock\n");
    return 2;
  }
  const auto& cpu = *cpu_object;

  // Scenario 1: the achieved mode is disclosed per catalog entry, and
  // the device description names the reason a fast mechanism was
  // refused (FR-023).
  std::printf("machine: %s\n", std::string(machine.description()).c_str());
  std::printf("cpu: %s\n", std::string(cpu.description()).c_str());
  describe_modes("machine");
  describe_modes("cpu");

  // The syscall regime, measured on a clock-only plan: one vDSO read per
  // action (FR-026).
  const auto monotonic = machine.counter<time_dim>("monotonic");
  if (!monotonic.has_value()) {
    fail("the clock provider seeds the monotonic leaf");
  }
  const sg::counters::expression<time_dim> elapsed {*monotonic};
  auto clock_plan = compile(system::local(), elapsed);
  if (!clock_plan.has_value()) {
    fail("the clock-only plan compiles");
  }
  std::printf("clock plan reads machine/monotonic, one leaf per action\n");

  // The nanosecond-rate leaf reads the same fast path and differs in rate
  // only, so it is measured beside its sibling and under the identical
  // protocol. Its read mode is the label every clock counter carries
  // while the read itself avoids the system call, and the published page
  // records the fast path in that row's read-mode cell (specs/011 FR-008,
  // FR-010).
  const auto monotonic_raw = machine.counter<time_dim>("monotonic_raw");
  if (!monotonic_raw.has_value()) {
    fail("the clock provider seeds the monotonic_raw leaf");
  }
  const sg::counters::expression<time_dim> raw_elapsed {*monotonic_raw};
  auto raw_clock_plan = compile(system::local(), raw_elapsed);
  if (!raw_clock_plan.has_value()) {
    fail("a plan over the nanosecond-rate counter compiles");
  }
  std::printf(
      "raw clock plan reads machine/monotonic_raw, one leaf per " "action\n");

  std::printf("\nsampling cost by regime (nanoseconds per sample()):\n");
  const regime syscall_regime = measure(*clock_plan, "clock, syscall (vDSO)");
#if SG_TEST_HAS_TSC
  report_gated_ticks(*clock_plan, "gated clock, monotonic");
#endif
  const regime raw_syscall_regime =
      measure(*raw_clock_plan, "clock raw, syscall (vDSO)");
  std::printf("the raw-rate median is %.1f ns against %.1f ns for the "
              "adjusted clock, and both reads name the same fast path\n",
              raw_syscall_regime.median_ns,
              syscall_regime.median_ns);

#if SG_TEST_HAS_TSC
  // The library's own cost over a bare read of the same instruction. The
  // raw entry publishes wherever the build executes the instruction, so this
  // comparison sits with the clock plan. The fast-mechanism probe below may
  // return before it. A host whose PMU refuses a fast mode still executes
  // this instruction and still owes the figure.
  const auto tsc_entry = machine.counter<events>("tsc");
  if (!tsc_entry.has_value()) {
    fail("the clock provider publishes the tsc entry on this build");
  }
  const sg::counters::expression<events> tick_expression {*tsc_entry};
  const auto tick_plan = compile(system::local(), tick_expression);
  if (!tick_plan.has_value()) {
    fail("a plan over the raw time-stamp entry compiles");
  }

  // Both sides measured by one method, so the difference is the library's
  // own cost and not a difference of two timing methods. A tick is a count,
  // not a duration: the rate printed below is what converts it, because a
  // tick is not a core cycle unless the two frequencies happen to match.
  const tick_cost bare = summarize_ticks(measure_bare_tsc());
  const tick_cost library = summarize_ticks(measure_library_tsc(*tick_plan));
  const double rate = tsc_rate_hz();
  const auto as_ns = [rate](const std::uint64_t ticks)
  { return static_cast<double>(ticks) / rate * 1e9; };
  std::printf(
      "\nper sampling action, %llu actions x %zu repeats, " "TSC %.3f GHz\n",
      static_cast<unsigned long long>(kPerSampleActions),
      kPerSampleRepeats,
      rate / 1e9);
  std::printf(
      "%-24s min %4llu t   median %4llu t   max %4llu t   (median %6.1f ns)\n",
      "bare rdtsc pair",
      static_cast<unsigned long long>(bare.min),
      static_cast<unsigned long long>(bare.median),
      static_cast<unsigned long long>(bare.max),
      as_ns(bare.median));
  std::printf(
      "%-24s min %4llu t   median %4llu t   max %4llu t   (median %6.1f ns)\n",
      "library sampling path",
      static_cast<unsigned long long>(library.min),
      static_cast<unsigned long long>(library.median),
      static_cast<unsigned long long>(library.max),
      as_ns(library.median));
  std::printf("library cost over bare: %+lld ticks per sampling action\n",
              static_cast<long long>(library.median)
                  - static_cast<long long>(bare.median));
  std::printf(
      "the bare figure is a property of the instruction. The library "
      "figure is a property of this build: an optimized build inlines the "
      "read and the bookkeeping, an unoptimized one pays a call and a frame "
      "per action. The published budget is the release-preset figure.\n");

  // Liveness and ordering only. The ratio between the two is deliberately
  // not asserted, because it moves with the build's optimization level by
  // more than the difference it would be asserting: the same library costs
  // about one bare read optimized and about five unoptimized. A threshold
  // narrow enough to mean anything here would fail on half the presets, and
  // one wide enough to pass on both would not detect a regression.
  check(bare.median > 0,
        "the bare read measured a positive cost, so the comparison is live");
  check(library.median > 0,
        "the library path measured a positive cost, so the comparison is live");
  check(bare.min <= bare.median && bare.median <= bare.max,
        "the bare distribution is ordered min <= median <= max");
  check(library.min <= library.median && library.median <= library.max,
        "the library distribution is ordered min <= median <= max");
#endif

  // The PMU group regime: one read per leader per action (FR-041). The
  // catalog decides which mechanism the plan reads, so the row carries
  // the mode the catalog disclosed (FR-023, C-PRO-4).
  const auto entries = cpu.counters();
  const auto disclosed_mode = [&entries](const std::string& name) -> ReadMode
  {
    for (const auto& entry : entries) {
      if (entry.name == name) {
        return entry.mode;
      }
    }
    return ReadMode::SYSCALL;
  };
  std::string work;
  std::string cycle;
  for (const auto& entry : entries) {
    if (entry.avail != Availability::COUNTABLE) {
      continue;
    }
    const std::string name(entry.name);
    if (work.empty()
        && (name == "instructions" || name == "ex_ret_instr"
            || name == "inst_retired"))
    {
      work = name;
    }
    if (cycle.empty()
        && (name == "cpu-cycles" || name == "cycles" || name == "ex_ret_ops"))
    {
      cycle = name;
    }
  }
  if (!work.empty() && !cycle.empty()) {
    const auto numerator = cpu.counter<events>(work);
    const auto denominator = cpu.counter<events>(cycle);
    if (numerator.has_value() && denominator.has_value()) {
      const auto ipc = *numerator / *denominator;
      const sg::counters::expression<events> work_expr {*numerator};
      const sg::counters::expression<events> cycle_expr {*denominator};
      auto group_plan = compile(system::local(), ipc, work_expr, cycle_expr);
      if (!group_plan.has_value()) {
        fail("the pmu group plan compiles");
      }
      const bool fast = disclosed_mode(work) == ReadMode::FAST_RDPMC;
      std::printf("group plan reads cpu/%s and cpu/%s, one read per leader "
                  "per action\n",
                  work.c_str(),
                  cycle.c_str());
      measure(*group_plan,
              fast ? "pmu group, fast_rdpmc" : "pmu group, syscall");
#if SG_TEST_HAS_TSC
      report_gated_ticks(*group_plan, "gated core PMU group");
#endif
      measure_fold(*group_plan,
                   *clock_plan,
                   elapsed,
                   work_expr,
                   cycle_expr,
                   "pmu group, fold only");

      // The fast side of SC-004, measured against the clock plan above:
      // one leaf per sampling action in each, so the two rows differ in
      // the read mechanism and in nothing else the plan charges. A
      // single cpu-PMU leaf is the pair on a host that discloses
      // fast_rdpmc; the calibration below then charges one mapped-page
      // read against the clock plan's one vDSO read.
      if (fast) {
        auto single = compile(system::local(), work_expr);
        if (!single.has_value()) {
          std::printf("\nSKIP: the catalog discloses fast_rdpmc for cpu/%s "
                      "but the plan did not open: %s\n",
                      work.c_str(),
                      single.error().message.c_str());
          return 2;
        }
        const regime fast_regime = measure(*single, "pmu single, fast_rdpmc");
        std::printf("\nfast median %.1f ns against syscall median %.1f ns: "
                    "the fast regime is %.1fx cheaper per sampling action\n",
                    fast_regime.median_ns,
                    syscall_regime.median_ns,
                    syscall_regime.median_ns / fast_regime.median_ns);
        // The two rows read different leaves, which the spec states and no
        // fixed factor follows from: a vDSO clock read is the cheapest read
        // this host offers, so it is not a syscall-mode counterpart for a
        // hardware counter. The order comparison is therefore published
        // and not asserted, and the judgement belongs to the published
        // page. This file is not a CI gate on its numbers (Principle VII
        // baseline infrastructure is an open deferral).
        std::printf("order check: the fast-mode median is %s the "
                    "syscall-mode median\n",
                    fast_regime.median_ns < syscall_regime.median_ns
                        ? "below"
                        : "above");
        std::printf("counters_overhead PASS: distributions published, modes "
                    "disclosed\n");
        return 0;
      }
    }
  }

  // No fast-mode cpu leaf. The `tsc` clock leaf gives the pair on every
  // build that executes the time-stamp instruction (FR-001), one rdtsc
  // read per action against the clock plan's one vDSO read. A build
  // without the instruction has no fast regime to publish, and the
  // reason is named (quickstart 12).
  for (const auto& entry : machine.counters()) {
    if (entry.mode != ReadMode::FAST_TSC) {
      continue;
    }
    const auto fast_counter = machine.counter<time_dim>(entry.name);
    if (!fast_counter.has_value()) {
      continue;
    }
    const sg::counters::expression<time_dim> fast_expression {*fast_counter};
    auto fast_plan = compile(system::local(), fast_expression);
    if (!fast_plan.has_value()) {
      std::printf("\nSKIP: the catalog discloses fast_tsc for machine/%s "
                  "but the plan did not open: %s\n",
                  std::string(entry.name).c_str(),
                  fast_plan.error().message.c_str());
      return 2;
    }
    const regime fast_regime = measure(*fast_plan, "clock, fast_tsc");
    std::printf("\nfast median %.1f ns against syscall median %.1f ns: "
                "the fast regime is %.1fx cheaper per sampling action\n",
                fast_regime.median_ns,
                syscall_regime.median_ns,
                syscall_regime.median_ns / fast_regime.median_ns);
    // Published and reported. It is deliberately not asserted, for the
    // reason the fast_rdpmc branch above states: this file is not a CI
    // gate on its numbers.
    std::printf(
        "order check: the fast-mode median is %s the " "syscall-mode median\n",
        fast_regime.median_ns < syscall_regime.median_ns ? "below" : "above");
    std::printf(
        "counters_overhead PASS: distributions published, modes " "disclosed"
                                                                  "\n");
    return 0;
  }

  // The reason comes from the catalog, which the provider fills from its
  // own probe. A sentence written here goes stale the moment a host
  // clears one gate and stops at another.
  std::printf(
      "\nSKIP: this host probes no fast read mechanism, so the fast "
      "regime is unmeasured and the comparison has no fast side. The "
      "catalog reports the gating fact: %s\n"
      "Every catalog entry disclosed syscall mode, so both plans "
      "measured above ran in syscall mode. The page "
      "docs/pages/counters-overhead.md records the probe reason beside "
      "the syscall rows and leaves the fast row unmeasured.\n",
      std::string(cpu.description()).c_str());
  return 2;
}
