#include <algorithm>
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin custom-statistics
auto spreadOf(const std::vector<double>& times, double& spread) -> bool
{
  if (times.empty()) {
    return false;
  }
  const auto span = std::ranges::minmax(times);
  if (span.min <= 0.0) {
    return false;
  }
  spread = (span.max - span.min) / span.min * 100.0;
  return true;
}

auto bmHashRows(sg::State& state) -> void
{
  std::uint64_t hash = 1469598103934665603ULL;
  for (auto _ : state) {
    hash = (hash ^ state.iterations()) * 1099511628211ULL;
    sg::doNotOptimize(hash);
  }
}

SG_BENCHMARK(bmHashRows)
    .repetitions(5)
    .addStatistic("spread", sg::StatUnit::PERCENTAGE, &spreadOf);
// sg-example:end custom-statistics

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
