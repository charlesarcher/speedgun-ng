#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// The threshold stands above any gap this host inserts: the recorded
// run stands quiet, the block showing the disclosure shape. The tracer
// request stands the not-available disclosure on a host without the
// rights the setup writes need.
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
  sg::runJitterProbe({.thresholdNs = 1000000000,
                      .mode = sg::JitterProbeMode::PRECHECK,
                      .precheckSerial = true,
                      .tracerEngine = sg::TracerEngine::OSNOISE});
  return sg::speedgunMain(argc, argv);
}

// sg-example:end jitter-probe
