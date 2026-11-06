#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// The detector engine stands one kernel thread per tracer cpu with interrupts
// off for the width of each window, so it records the interruptions a probe
// thread with interrupts on cannot see. The threshold stands above any gap
// this host inserts: the recorded run stands quiet and shows the shape of a
// quiet window. A host that withholds the model-specific register node keeps
// status 0 and discloses the counter it cannot read.
// sg-example:begin hwlat-unit
auto bmMeasured(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmMeasured).threads(2).placement("compact");

// sg-example:end hwlat-unit

// The method and the engine stand on one run: the method reads the two counter
// sources around the window, and the engine names the tracer the run sets up
// and restores around it.
// sg-example:begin hwlat-main
auto main(int argc, char** argv) -> int
{
  sg::runJitterProbe({.thresholdNs = 1000000000,
                      .mode = sg::JitterProbeMode::PRECHECK,
                      .precheckMs = 500,
                      .precheckSerial = true,
                      .tracerEngine = sg::TracerEngine::HWLAT,
                      .method = sg::JitterMethod::HWLAT});
  return sg::speedgunMain(argc, argv);
}

// sg-example:end hwlat-main
