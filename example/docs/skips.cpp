#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin skips
auto bmGuard(sg::State& state) -> void
{
  if (state.range(0) == 0) {
    state.skipWithMessage("no cells configured");
    return;
  }
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0));
  }
}

SG_BENCHMARK(bmGuard).denseRange(0, 4, 4);

// sg-example:end skips

// example-run: expect-skip bmGuard/0: no cells configured

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
