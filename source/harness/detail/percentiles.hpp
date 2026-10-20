#ifndef SG_HARNESS_DETAIL_PERCENTILES_HPP
#define SG_HARNESS_DETAIL_PERCENTILES_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_measurement.hpp"

namespace sg::detail
{

/// @brief specs/021 E-04, FR-001, FR-015: the value of every chunk of one
/// column of one run.
///
/// Chunk `i` is the window between the consecutive chunk-boundary points of
/// the run: the points of the span that stand in no pause pair, in the order
/// the run took them. A pause interval whose ordinals stand inside one chunk
/// window nets out of that window and out of no other, the rule the
/// whole-run netting of FR-019 already applies (FR-015). A pair that folded
/// at its commit under FR-052 leaves no points in the recorder, so no chunk
/// window covers it and its time stays inside the chunk that ran it.
///
/// \pre `api` holds `basePoint + chunkCount + 1 + 2 * pausePairs.size()`
///      committed points for `core`
/// \post one value per chunk, in chunk order, each the netted fold of that
///       chunk's window
[[nodiscard]] auto buildChunkValues(
    const counters::detail::ExprCore& core,
    const counters::RecorderApi& api,
    std::size_t basePoint,
    std::uint64_t chunkCount,
    const std::vector<std::pair<std::uint64_t, std::uint64_t>>& pausePairs)
    -> std::vector<double>;

/// @brief specs/021 FR-013: the least chunk count that can answer the
/// percentile point `p`.
///
/// \pre 0 < p < 100
/// \post the count is `ceil(100 / (100 - p))`: 2 for p50, 10 for p90, 100
///       for p99, 1000 for p99.9
[[nodiscard]] auto minSamplesFor(double p) noexcept -> std::uint64_t;

/// @brief specs/021 FR-011, FR-013: the nearest-rank read of one sorted
/// column of chunk values.
///
/// \pre `sortedValues` stands sorted ascending
/// \post the value answers only when the column holds at least
///       `minSamplesFor(p)` values, and the read then names the value at
///       rank `ceil(p * n / 100)`, 1-based. No value interpolates between
///       two chunk values and none extrapolates past the ends.
[[nodiscard]] auto percentileAt(const std::vector<double>& sortedValues,
                                double p) noexcept -> PercentileValue;

/// @brief specs/021 FR-024: the percentile points a run reports, ascending
/// with duplicates collapsed. An empty request answers with the default
/// list of p50, p90, p99 and p99.9.
///
/// \pre every element stands in the open interval (0, 100); the command
///      line refuses anything else before a benchmark runs (FR-022)
/// \post the list is sorted ascending and holds no duplicate
[[nodiscard]] auto normalizePercentilePoints(std::vector<double> points)
    -> std::vector<double>;

/// @brief specs/021 FR-010, FR-016: the percentile columns of one run.
///
/// Each input column of chunk values answers one output column, one value
/// per reported point. A column that answers no value for a point carries
/// the not-available answer for it, and a column whose fold disclosed a
/// gap answers not available for every point: no chunk value of such a
/// column carries a count the rank could name.
///
/// \pre every chunk column holds values of the same quantity, and
///      \c countable holds one availability flag for each of them
/// \post one entry per input column, each holding one value per point
[[nodiscard]] auto buildPercentileColumns(
    const std::vector<std::vector<double>>& chunkValues,
    const std::vector<double>& points,
    const std::vector<bool>& countable)
    -> std::vector<std::vector<PercentileValue>>;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_PERCENTILES_HPP
