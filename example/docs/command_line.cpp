#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin command-line
auto bmRead(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

auto bmWrite(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmRead).argNames({"cells"}).arg(256);

SG_BENCHMARK(bmWrite).argNames({"cells"}).arg(256);
// sg-example:end command-line

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
