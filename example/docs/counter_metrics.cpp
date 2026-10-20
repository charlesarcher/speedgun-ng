#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin metrics
#include "speedgun-ng/counters.hpp"

auto attachMetric(sg::BenchmarkHandle& handle) -> void
{
  const auto cpu = sg::counters::System::local().object("cpu");
  if (!cpu.has_value()) {
    return;
  }
  const auto cycles = cpu->counter<sg::counters::Dim<0, 1>>("cpu-cycles");
  const auto retired = cpu->counter<sg::counters::Dim<0, 1>>("instructions");
  if (cycles.has_value() && retired.has_value()) {
    handle.addMetric(*retired / *cycles, "instructions/cycle");
  }
}

// sg-example:end metrics

// sg-example:begin metric-lookup
#include <cstdio>
#include <string>
#include <string_view>

#include "speedgun-ng/counters_metrics.hpp"

auto describeMetric(std::string_view name) -> void
{
  const auto metric = sg::counters::metricByName(name);
  if (!metric.has_value()) {
    std::printf(
        "%s: %s\n", std::string(name).c_str(), metric.error().message.c_str());
    return;
  }
  std::printf("%s: group=%s unit=%s\n",
              std::string(name).c_str(),
              std::string(metric->group).c_str(),
              std::string(metric->unit).c_str());
}

// sg-example:end metric-lookup

auto bmTick(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

auto main(int argc, char** argv) -> int
{
  sg::registerBenchmark(&bmTick, "Table/tick").repetitions(2);
  return sg::speedgunMain(argc, argv);
}
