#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin reading-the-report
auto bmRows(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmRows).repetitions(3);

// sg-example:end reading-the-report

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
