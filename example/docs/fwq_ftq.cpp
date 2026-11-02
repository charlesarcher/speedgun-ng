#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// One run carries one method, so the run below names FWQ and the calibration
// measures the work count on each probe cpu ahead of the window. Fixed work
// quanta prices one unit of loop bodies, so a dip shows as a long unit. Fixed
// time quanta counts the units one quantum holds, so a dip shows as a short
// count.
// sg-example:begin fwq-unit
auto bmFixedWork(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmFixedWork).threads(2).placement("compact");

// sg-example:end fwq-unit

// sg-example:begin ftq-unit
auto bmFixedTime(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmFixedTime).threads(2).placement("compact");

// sg-example:end ftq-unit

// sg-example:begin fwq-ftq
auto main(int argc, char** argv) -> int
{
  sg::runJitterProbe({.thresholdNs = 1000000000,
                      .mode = sg::JitterProbeMode::PRECHECK,
                      .precheckMs = 500,
                      .precheckSerial = true,
                      .method = sg::JitterMethod::FWQ,
                      .workCount = 0,
                      .noisePercent = 1});
  return sg::speedgunMain(argc, argv);
}

// sg-example:end fwq-ftq
