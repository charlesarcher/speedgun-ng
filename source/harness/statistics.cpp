// specs/029 data-model.md E-11, FR-093 (PD-05): the one statistics summary
// the harness reports and a hunt rank both call. T009 moves the definition
// of `statisticsOf` here from the anonymous namespace of runner.cpp, beside
// the detail/statistics.hpp declaration it adds. specs/040 T011 adds the
// Spearman correlation and the least-squares slope of the hunt's
// sensitivity points (B-2, B-3), with the least-squares sums the complexity
// fit shares moved beside them (PD-02, D-8).

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <utility>
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

namespace
{

// The average rank of one coordinate, written back over the coordinate
// itself: the point set arrives by value, so the ranking is the call's own
// (the statisticsOf convention). A tie group stands at the average of the
// ranks it spans, the formulation research PD-01 pins, so a tie-heavy set
// reads no ordinal tie-break.
auto rankInPlace(std::vector<std::pair<double, double>>& points,
                 double std::pair<double, double>::*coordinate) -> void
{
  auto order = std::vector<std::size_t>(points.size());
  std::iota(order.begin(), order.end(), std::size_t {0});
  std::ranges::sort(order,
                    std::less<double> {},
                    [&points, coordinate](const std::size_t index)
                    { return points[index].*coordinate; });
  std::size_t start = 0;
  while (start < order.size()) {
    auto end = start + 1U;
    while (end < order.size()
           && std::is_eq(points[order[end]].*coordinate
                         <=> points[order[start]].*coordinate))
    {
      ++end;
    }
    const auto group = static_cast<double>(end - start);
    const double average = static_cast<double>(start) + (group + 1.0) / 2.0;
    for (auto index = start; index < end; ++index) {
      points[order[index]].*coordinate = average;
    }
    start = end;
  }
}

}  // namespace

auto spearmanCorrelation(std::vector<std::pair<double, double>> points)
    -> std::optional<double>
{
  if (points.size() < 3) {
    return std::nullopt;
  }
  rankInPlace(points, &std::pair<double, double>::first);
  rankInPlace(points, &std::pair<double, double>::second);

  const auto count = static_cast<double>(points.size());
  const double meanRank = (count + 1.0) / 2.0;
  double covariance = 0.0;
  double firstSpread = 0.0;
  double secondSpread = 0.0;
  for (const auto& [rankFirst, rankSecond] : points) {
    const double firstRank = rankFirst - meanRank;
    const double secondRank = rankSecond - meanRank;
    covariance += firstRank * secondRank;
    firstSpread += firstRank * firstRank;
    secondSpread += secondRank * secondRank;
  }
  SG_ASSERT(
      firstSpread >= 0.0 && secondSpread >= 0.0,
      "the rank deviations are squares, so their sums stand " "non-negative");
  if (std::fpclassify(firstSpread) == FP_ZERO
      || std::fpclassify(secondSpread) == FP_ZERO)
  {
    return std::nullopt;
  }
  return covariance / std::sqrt(firstSpread * secondSpread);
}

auto leastSquaresSums(const std::vector<std::pair<double, double>>& points)
    -> LeastSquaresSums
{
  auto sums = LeastSquaresSums {};
  for (const auto& [first, second] : points) {
    sums.cross += first * second;
    sums.square += first * first;
    sums.xSum += first;
    sums.ySum += second;
  }
  SG_ASSERT(sums.square >= 0.0,
            "the square sum adds squares of the first coordinates, so it "
            "stands non-negative");
  return sums;
}

auto leastSquaresSlope(std::vector<std::pair<double, double>> points)
    -> std::optional<double>
{
  if (points.size() < 2) {
    return std::nullopt;
  }
  const auto count = static_cast<double>(points.size());
  const auto sums = leastSquaresSums(points);
  const double denominator = (count * sums.square) - (sums.xSum * sums.xSum);
  if (std::fpclassify(denominator) == FP_ZERO) {
    return std::nullopt;
  }
  return ((count * sums.cross) - (sums.xSum * sums.ySum)) / denominator;
}

}  // namespace sg::detail
