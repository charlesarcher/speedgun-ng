#include <cstdint>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin registration-macro
auto bmCells(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmCells);

// sg-example:end registration-macro

// sg-example:begin registration-table
auto bmTable(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kTable = sg::registerBenchmark(&bmTable, "Table/cells");

// sg-example:end registration-table

// sg-example:begin capture
auto bmAdd(sg::State& state, int left, int right) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(left + right);
  }
}

SG_BENCHMARK_CAPTURE(bmAdd, pair, 2, 3);

// sg-example:end capture

// sg-example:begin template
template<typename Type>
auto bmWidth(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK_TEMPLATE(bmWidth, std::uint32_t);

// sg-example:end template

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
