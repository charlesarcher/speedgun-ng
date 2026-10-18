#include <algorithm>
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin reading-the-report
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

// The main page quotes this pair: a loop that reports work done, then the
// registration that asks for the counter column and the label.
// sg-example:begin report-fields
auto bmFill(sg::State& state) -> void
{
  const auto cells = state.range(0);
  std::vector<std::uint64_t> row(static_cast<std::size_t>(cells), 0);
  for (auto _ : state) {
    row.front() = static_cast<std::uint64_t>(cells);
    sg::doNotOptimize(row);
  }
  state.setBytesProcessed(cells * 8);
  state.setLabel("filled");
}

SG_BENCHMARK(bmFill).argsProduct({{8}}).threadRange(1, 2);

// sg-example:end report-fields

// The thread range gives this family two instances: one context, which keeps
// the printed bytes a single-context run has always printed, and two, whose
// merged row ends the run with the contexts disclosure under it. The guide
// page records both.

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
