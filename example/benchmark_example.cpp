#include <cstdint>
#include <memory>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto table() -> std::vector<std::uint64_t>&
{
  static std::vector<std::uint64_t> values(1 << 12, 1);
  return values;
}

auto bmSum(sg::State& state) -> void
{
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t value : table()) {
      total += value;
    }
    sg::doNotOptimize(total);
  }
}

auto bmTouch(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(table().front());
  }
}

}  // namespace

SG_BENCHMARK(bmTouch)

auto main(int argc, char** argv) -> int
{
  // The catalog freezes at its first use, so a suite that resolves a
  // counter itself registers the providers it needs first. The entry
  // point registers them for every other suite (R-03).
  auto& system = sg::counters::System::local();
  (void)system.registerProvider(
      std::make_unique<sg::counters::ClockProvider>());
  (void)system.registerProvider(std::make_unique<sg::counters::PmuProvider>());

  auto handle = sg::registerBenchmark(&bmSum, "bmSum");
  if (!handle.name().empty()) {
    const auto cpu = system.object("cpu");
    if (cpu.has_value()) {
      const auto instructions =
          cpu->counter<sg::counters::Dim<0, 1>>("instructions");
      const auto cycles = cpu->counter<sg::counters::Dim<0, 1>>("cpu-cycles");
      if (instructions.has_value() && cycles.has_value()) {
        handle.addMetric(*instructions / *cycles, "instructions/cycle");
      }
    }
  }

  return sg::speedgunMain(argc, argv);
}
