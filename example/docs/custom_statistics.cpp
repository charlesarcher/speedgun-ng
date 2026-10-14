#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

auto bmHashRows(sg::State& state) -> void
{
  std::uint64_t hash = 1469598103934665603ULL;
  for (auto _ : state) {
    hash = (hash ^ state.iterations()) * 1099511628211ULL;
    sg::doNotOptimize(hash);
  }
}

// sg-example:begin custom-statistics
auto spreadOf(const std::vector<double>& times, double& spread) -> bool
{
  if (times.empty()) {
    return false;
  }
  double lo = times[0];
  double hi = times[0];
  for (const double value : times) {
    lo = value < lo ? value : lo;
    hi = value > hi ? value : hi;
  }
  if (lo <= 0.0) {
    return false;
  }
  spread = (hi - lo) / lo * 100.0;
  return true;
}
// sg-example:end custom-statistics

SG_BENCHMARK(bmHashRows)
    .repetitions(5)
    .addStatistic("spread", sg::StatUnit::PERCENTAGE, &spreadOf);

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
