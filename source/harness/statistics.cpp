// specs/029 data-model.md E-11, FR-093 (PD-05): the one statistics summary
// the harness reports and a hunt rank both call. T009 moves the definition
// of `statisticsOf` here from the anonymous namespace of runner.cpp, beside
// the detail/statistics.hpp declaration it adds.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "detail/statistics.hpp"

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

auto statisticsOf(std::vector<double> values) -> sg::Statistics
{
  sg::Statistics stats;
  SG_ASSERT(!values.empty(),
            "the caller builds the summary only for a measured run with more "
            "than one repetition, so it hands over one value per row");

  std::sort(values.begin(), values.end());
  stats.samples = static_cast<std::uint64_t>(values.size());
  stats.min = values.front();
  stats.max = values.back();

  double sum = 0.0;
  for (const double value : values) {
    sum += value;
  }
  stats.mean = sum / static_cast<double>(values.size());

  const std::size_t n = values.size();
  stats.median =
      (n % 2 == 1) ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2.0;

  double squared = 0.0;
  for (const double value : values) {
    squared += (value - stats.mean) * (value - stats.mean);
  }
  SG_ASSERT(n > 1,
            "the caller builds the summary only for a measured run with more "
            "than one repetition");
  stats.sampleStdDev = std::sqrt(squared / static_cast<double>(n - 1));
  stats.coefficientOfVariation = std::fpclassify(stats.mean) != FP_ZERO
      ? stats.sampleStdDev / stats.mean
      : 0.0;
  return stats;
}

}  // namespace sg::detail
