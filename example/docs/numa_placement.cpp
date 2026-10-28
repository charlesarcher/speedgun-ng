#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin numa-placement
auto bmSharded(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmSharded).threads(2).placement("node");
// sg-example:end numa-placement

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
