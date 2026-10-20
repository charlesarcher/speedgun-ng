#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin topology-placement
auto bmCompact(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmCompact).threads(2).placement("compact");

auto bmPair(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmPair).threads(2);

// sg-example:end topology-placement

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
