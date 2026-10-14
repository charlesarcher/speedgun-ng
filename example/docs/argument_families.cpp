#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin ranges
auto bmSquares(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) * state.range(0));
  }
}

SG_BENCHMARK(bmSquares)
    .argNames({"cells"})
    .arg(32)
    .args({96})
    .argsProduct({{4, 8}})
    .rangeMultiplier(2)
    .range(64, 512)
    .ranges({{4096, 8192}})
    .denseRange(1024, 2048, 512);
// sg-example:end ranges

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
