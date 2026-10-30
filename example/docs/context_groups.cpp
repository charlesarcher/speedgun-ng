#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin groups
auto bmGrouped(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmGrouped).threads(2).placement("compact");

// sg-example:end groups

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
