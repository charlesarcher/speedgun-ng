#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "detail/percentiles.hpp"

#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// specs/021 FR-024: the list a run reports when nothing asked for one.
constexpr double kDefaultPoints[] {50.0, 90.0, 99.0, 99.9};

}  // namespace

auto buildChunkValues(
    const counters::detail::ExprCore& core,
    const counters::RecorderApi& api,
    const std::size_t basePoint,
    const std::uint64_t chunkCount,
    const std::vector<std::pair<std::uint64_t, std::uint64_t>>& pausePairs)
    -> std::vector<double>
{
  // FR-002, FR-015: the recorder holds the chunk boundaries and the pause
  // pairs in the order the run took them, so the boundary ordinals are the
  // ordinals of the span that stand in no pause pair. A pair belongs to the
  // one chunk whose window opens at or before its first point, and its fold
  // leaves that chunk's value alone.
  SG_ASSERT(api.count
                >= basePoint + chunkCount + 1 + 2 * pausePairs.size(),
            "the recorder of the run holds one point per chunk boundary, the "
            "exit point and the points of every recorded pause, so the fold "
            "of every chunk window reads committed points (FR-002, FR-008)");
  std::vector<std::size_t> boundaries;
  boundaries.reserve(chunkCount + 1);
  const std::size_t spanEnd =
      basePoint + chunkCount + 1 + 2 * pausePairs.size();
  const auto holdsPausePoint = [&](const std::size_t ordinal) noexcept
  {
    for (const auto& pause : pausePairs) {
      if (ordinal == basePoint + static_cast<std::size_t>(pause.first)
          || ordinal == basePoint + static_cast<std::size_t>(pause.second))
      {
        return true;
      }
    }
    return false;
  };
  for (std::size_t ordinal = basePoint; ordinal < spanEnd; ++ordinal) {
    if (!holdsPausePoint(ordinal)) {
      boundaries.push_back(ordinal);
    }
  }
  SG_ASSERT(boundaries.size() == chunkCount + 1,
            "the points outside the pause pairs are exactly the chunk "
            "boundaries and the exit point (FR-002)");

  std::vector<double> values;
  values.reserve(chunkCount);
  for (std::uint64_t chunk = 0; chunk < chunkCount; ++chunk) {
    const auto window = counters::detail::foldCore(
        core, api, boundaries[chunk], boundaries[chunk + 1]);
    double value = window.value;
    for (const auto& pause : pausePairs) {
      const auto pairFirst = basePoint + static_cast<std::size_t>(pause.first);
      const auto pairSecond =
          basePoint + static_cast<std::size_t>(pause.second);
      if (pairFirst < boundaries[chunk] || pairFirst >= boundaries[chunk + 1]) {
        continue;
      }
      const auto paused =
          counters::detail::foldCore(core, api, pairFirst, pairSecond);
      value -= paused.value;
    }
    values.push_back(value);
  }
  return values;
}

// The rank and the least count both divide by a decimal point, and a point
// like 99.9 stands as 99.900000000000006 in a double, so the division
// lands a hair above the integer it should reach. The ceiling nudges back
// by a relative epsilon before it rounds.
auto ceilNudged(const double raw) noexcept -> std::uint64_t
{
  return static_cast<std::uint64_t>(std::ceil(raw - 1e-9 * raw));
}

auto minSamplesFor(const double p) noexcept -> std::uint64_t
{
  // Any point above zero needs two chunks to answer at all, which is what
  // the ceiling of the rule gives for a point near zero.
  const std::uint64_t needed = ceilNudged(100.0 / (100.0 - p));
  return needed < 2 ? 2 : needed;
}

auto percentileAt(const std::vector<double>& sortedValues,
                  const double p) noexcept -> PercentileValue
{
  // FR-020: the option parser admits a point only from the open interval,
  // and the rank below divides by the point, so a point outside it names
  // no rank at all.
  SG_REQUIRE(p > 0.0 && p < 100.0,
             "a percentile point stands strictly between 0 and 100 (FR-020)");
  if (sortedValues.size() < minSamplesFor(p)) {
    return PercentileValue {};
  }
  // FR-011: the nearest rank. The rank is 1-based, so the value it names
  // stands one position below it in the sorted column. A point above zero
  // and a column that reaches the least count put the rank at one or
  // above, so the step below it stays inside the column.
  const auto rank =
      ceilNudged(p * static_cast<double>(sortedValues.size()) / 100.0);
  const std::size_t index = static_cast<std::size_t>(rank - 1);
  return PercentileValue {
      .available = true,
      .value = sortedValues[index],
  };
}

auto normalizePercentilePoints(std::vector<double> points)
    -> std::vector<double>
{
  if (points.empty()) {
    return std::vector<double> {std::begin(kDefaultPoints),
                                std::end(kDefaultPoints)};
  }
  std::ranges::sort(points);
  points.erase(std::unique(points.begin(), points.end()), points.end());
  return points;
}

auto buildPercentileColumns(const std::vector<std::vector<double>>& chunkValues,
                            const std::vector<double>& points,
                            const std::vector<bool>& countable)
    -> std::vector<std::vector<PercentileValue>>
{
  std::vector<std::vector<PercentileValue>> columns(chunkValues.size());
  // One sort buffer serves every column of the run.
  std::vector<double> sorted;
  for (std::size_t column = 0; column < chunkValues.size(); ++column) {
    sorted = chunkValues[column];
    std::ranges::sort(sorted);
    // FR-016: a column whose fold disclosed a gap answers nothing.
    const bool answers = countable[column];
    columns[column].reserve(points.size());
    for (const double point : points) {
      columns[column].push_back(answers ? percentileAt(sorted, point)
                                        : PercentileValue {});
    }
  }
  return columns;
}

}  // namespace sg::detail
