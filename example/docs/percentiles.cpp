#include <cstdint>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin percentiles
auto bmRoll(sg::State& state) -> void
{
  std::uint64_t roll = 0;
  for (auto _ : state) {
    for (std::uint64_t i = 0; i < 30'000; ++i) {
      roll = (roll + i) * 1'099'511'628'211ULL;
    }
    sg::doNotOptimize(roll);
  }
}

SG_BENCHMARK(bmRoll).iterations(300).showPercentiles(true);

// sg-example:end percentiles

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
