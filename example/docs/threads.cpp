#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin threads
auto bmSharded(sg::State& state) -> void
{
  const int index = state.threadIndex();
  for (auto _ : state) {
    sg::doNotOptimize(index + state.threads());
  }
}

SG_BENCHMARK(bmSharded).threads(4);

// sg-example:end threads

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
