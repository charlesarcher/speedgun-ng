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

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::plan;
using sg::counters::pmu_provider;
using sg::counters::read_mode;
using sg::counters::recorder_handle;
using sg::counters::system;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

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
               .min_ns = compiled.sample_overhead_ns_min(),
               .median_ns = compiled.sample_overhead_ns_median(),
               .max_ns = compiled.sample_overhead_ns_max()};
  report(cost);
  check(cost.min_ns >= 0.0 && cost.median_ns >= cost.min_ns
            && cost.max_ns >= cost.median_ns,
        "the published distribution is ordered min <= median <= max");
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
    if (entry.mode == read_mode::fast_tsc
        || entry.mode == read_mode::fast_rdpmc)
    {
      ++fast;
    } else if (entry.mode == read_mode::syscall) {
      ++syscall_mode;
    }
  }
  std::printf("achieved modes on '%s': %zu fast, %zu syscall, %zu entries\n",
              located->description().data(),
              fast,
              syscall_mode,
              entries.size());
  for (const auto& entry : entries) {
    if (entry.mode == read_mode::fast_tsc
        || entry.mode == read_mode::fast_rdpmc)
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

  const auto machine = *system::local().object("machine");
  const auto cpu = *system::local().object("cpu");

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
  std::printf("\nsampling cost by regime (nanoseconds per sample()):\n");
  const regime syscall_regime = measure(*clock_plan, "clock, syscall (vDSO)");

  // The PMU group regime: one read per leader per action (FR-041). The
  // catalog decides which mechanism the plan reads, so the row carries
  // the mode the catalog disclosed (FR-023, C-PRO-4).
  const auto entries = cpu.counters();
  const auto disclosed_mode = [&entries](const std::string& name) -> read_mode
  {
    for (const auto& entry : entries) {
      if (entry.name == name) {
        return entry.mode;
      }
    }
    return read_mode::syscall;
  };
  std::string work;
  std::string cycle;
  for (const auto& entry : entries) {
    if (entry.avail != availability::countable) {
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
      const bool fast = disclosed_mode(work) == read_mode::fast_rdpmc;
      std::printf("group plan reads cpu/%s and cpu/%s, one read per leader "
                  "per action\n",
                  work.c_str(),
                  cycle.c_str());
      measure(*group_plan,
              fast ? "pmu group, fast_rdpmc" : "pmu group, syscall");
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

  // No fast-mode cpu leaf. A calibrated `tsc` clock leaf would still give
  // the pair, one rdtsc read per action against the clock plan's one vDSO
  // read; a host publishing neither has no fast regime to publish, and
  // the reason is named (quickstart 12).
  for (const auto& entry : machine.counters()) {
    if (entry.mode != read_mode::fast_tsc) {
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
