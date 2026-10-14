#include <algorithm>
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

auto spreadOf(const std::vector<double>& times, double& spread) -> bool
{
  if (times.empty()) {
    return false;
  }
  double lo = times.front();
  double hi = lo;
  for (const double time : times) {
    lo = std::min(lo, time);
    hi = std::max(hi, time);
  }
  spread = lo > 0.0 ? (hi - lo) / lo * 100.0 : 0.0;
  return true;
}

// sg-example:begin reading-the-report
auto bmRows(sg::State& state) -> void
{
  const auto cells = state.range(0);
  for (auto _ : state) {
    for (auto cell = std::int64_t {0}; cell < cells; ++cell) {
      sg::doNotOptimize(cell);
    }
  }
  state.setBytesProcessed(cells * 8);
  state.setLabel("filled");
}

SG_BENCHMARK(bmRows)
    .argsProduct({{8, 16, 32}})
    .repetitions(3)
    .complexity(sg::BigO::AUTOMATIC)
    .addStatistic("spread", sg::StatUnit::PERCENTAGE, &spreadOf);

// sg-example:end reading-the-report

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
