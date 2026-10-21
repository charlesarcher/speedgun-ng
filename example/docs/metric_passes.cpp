#include <memory>

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
#include <ranges>

#include "speedgun-ng/counters.hpp"

auto attachOverLimit(sg::BenchmarkHandle& handle) -> void
{
  using Leaf = sg::counters::Dim<0, 1>;
  const auto cpu = sg::counters::System::local().object("cpu");
  if (!cpu.has_value()) {
    return;
  }
  const auto limit = cpu->programmableCounters().value_or(0);
  for (const auto& entry : cpu->counters() | std::views::take(limit + 1)) {
    if (const auto leaf = cpu->counter<Leaf>(entry.name); leaf.has_value()) {
      handle.addMetric(sg::counters::Expression<Leaf> {*leaf}, entry.name);
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
