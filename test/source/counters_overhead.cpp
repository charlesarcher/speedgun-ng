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
// syscall figures were printed first, so CTest reports the skip with
// its cause. Frameworkless check()/fail() convention.
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
                  const sg::counters::expression<events>& work_expr,
                  const sg::counters::expression<events>& cycle_expr,
                  const char* label) -> void
{
  auto recorder = compiled.recorder(64);
  for (int index = 0; index < 64; ++index) {
    recorder.sample();
  }
  const auto before = std::chrono::steady_clock::now();
  volatile double sink = 0.0;
  for (int index = 0; index < 1000; ++index) {
    sink += (work_expr.fold(recorder.view(), 0, 63).value
             + cycle_expr.fold(recorder.view(), 0, 63).value);
  }
  const auto after = std::chrono::steady_clock::now();
  static_cast<void>(sink);
  const double per_fold =
      std::chrono::duration<double, std::nano>(after - before).count() / 1000.0;
  std::printf("%-34s %9.1f ns per first-to-last fold\n", label, per_fold);
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
  std::printf("\nsampling cost by regime (nanoseconds per sample()):\n");
  const regime syscall_regime = measure(*clock_plan, "clock, syscall (vDSO)");

  // The PMU group regime: one read per leader per action (FR-041).
  const auto entries = cpu.counters();
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
      measure(*group_plan, "pmu group, syscall");
      measure_fold(*group_plan, work_expr, cycle_expr, "pmu group, fold only");
    }
  }

  // The fast regime: a leaf the probe passed. Without one the host
  // cannot publish the comparison, and the reason is named (quickstart
  // 12).
  const auto fast_leaf = [&]() -> std::string
  {
    for (const auto& entry : machine.counters()) {
      if (entry.mode == read_mode::fast_tsc) {
        return std::string(entry.name);
      }
    }
    for (const auto& entry : cpu.counters()) {
      if (entry.mode == read_mode::fast_rdpmc) {
        return "cpu/" + std::string(entry.name);
      }
    }
    return {};
  }();

  if (fast_leaf.empty()) {
    std::printf(
        "\nSKIP: this host probes no fast read mechanism, so the "
        "fast-versus-syscall comparison has no fast side. The kernel "
        "publishes no calibrated time-stamp frequency "
        "('/sys/devices/system/cpu/tsc_khz' absent) and the user counter "
        "page is unprobeable; both regimes above ran in syscall mode. "
        "Published budgets per platform live in "
        "docs/pages/counters-overhead.md.\n");
    return 2;
  }

  const auto slash = fast_leaf.rfind('/');
  const auto& fast_object = slash == std::string::npos ? machine : cpu;
  const std::string fast_name =
      slash == std::string::npos ? fast_leaf : fast_leaf.substr(slash + 1);
  const auto fast_counter = fast_object.counter<time_dim>(fast_name);
  if (!fast_counter.has_value()) {
    std::printf("\nSKIP: the fast leaf '%s' does not resolve in this "
                "build; no fast regime to publish\n",
                fast_leaf.c_str());
    return 2;
  }
  const sg::counters::expression<time_dim> fast_expression {*fast_counter};
  auto fast_plan = compile(system::local(), fast_expression);
  if (!fast_plan.has_value()) {
    fail("the fast plan compiles");
  }
  const regime fast_regime = measure(*fast_plan, "clock, fast");
  std::printf("\nfast median %.1f ns against syscall median %.1f ns: "
              "the fast regime is %.1fx cheaper per sampling action\n",
              fast_regime.median_ns,
              syscall_regime.median_ns,
              syscall_regime.median_ns / fast_regime.median_ns);
  std::printf(
      "counters_overhead PASS: distributions published, modes " "disclosed\n");
  return 0;
}
