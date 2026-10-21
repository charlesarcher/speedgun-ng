#include <array>
#include <cstddef>
#include <memory>
#include <ranges>
#include <string_view>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

auto bmTick(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

// sg-example:begin metric-passes
#include <array>
#include <cstddef>
#include <ranges>
#include <string_view>

#include "speedgun-ng/counters.hpp"

constexpr std::array<std::string_view, 5> kOverLimitMetricLeafNames {
    "cpu-cycles", "instructions", "branch-misses", "ref-cycles", "bus-cycles"};

auto attachOverLimit(sg::BenchmarkHandle& handle) -> void
{
  const auto cpu = sg::counters::System::local().object("cpu");
  if (!cpu.has_value()) {
    return;
  }
  const std::size_t wanted = cpu->programmableCounters().value_or(0) + 1;
  for (const auto name : kOverLimitMetricLeafNames | std::views::take(wanted)) {
    const auto leaf = cpu->counter<sg::counters::Dim<0, 1>>(name);
    if (leaf.has_value()) {
      const sg::counters::Expression<sg::counters::Dim<0, 1>> metric {*leaf};
      handle.addMetric(metric, name);
    }
  }
}

// sg-example:end metric-passes

// The first catalog query opens the system and closes provider registration
// (FR-009), so main stages the providers before it reads the limit.
auto main(int argc, char** argv) -> int
{
  auto& sys = sg::counters::System::local();
  (void)sys.registerProvider(std::make_unique<sg::counters::ClockProvider>());
  (void)sys.registerProvider(std::make_unique<sg::counters::PmuProvider>());
  auto handle = sg::registerBenchmark(&bmTick, "Passes/overLimit");
  attachOverLimit(handle);
  return sg::speedgunMain(argc, argv);
}
