#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin jitter-probe
auto bmMeasured(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmMeasured).threads(2).placement("compact");

auto main(int argc, char** argv) -> int
{
  sg::runJitterProbe({.mode = sg::JitterProbeMode::PRECHECK});
  return sg::speedgunMain(argc, argv);
}
// sg-example:end jitter-probe
