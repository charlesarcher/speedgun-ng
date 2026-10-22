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

// The recorded run needs a host whose mask gives the idle resolution a
// free cpu behind the two planned contexts (FR-017), so a mask that
// holds fewer than three cpus cannot serve it. The run gates read the
// declaration below and skip with both counts on such a host, the way
// counters_overhead skips on a host without a probe-passing fast
// mechanism.
// example-run: needs-cpus 3
