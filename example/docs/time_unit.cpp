#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin time-unit
auto bmMillis(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kMillis = sg::registerBenchmark(&bmMillis, "Table/millis")
                         .unit(sg::TimeUnit::MILLISECONDS);

auto configureUnits() -> void
{
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);
}

// sg-example:end time-unit

auto main(int argc, char** argv) -> int
{
  configureUnits();
  return sg::speedgunMain(argc, argv);
}
