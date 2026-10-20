#include <cstdint>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin json-reports
auto bmDice(sg::State& state) -> void
{
  std::uint64_t dice = 0;
  for (auto _ : state) {
    for (std::uint64_t i = 0; i < 3'000; ++i) {
      dice = (dice + i) * 1'099'511'628'211ULL;
    }
    sg::doNotOptimize(dice);
  }
}

SG_BENCHMARK(bmDice).iterations(120).showPercentiles(true);

// sg-example:end json-reports

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
