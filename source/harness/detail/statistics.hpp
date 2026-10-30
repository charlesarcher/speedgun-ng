#ifndef SG_HARNESS_DETAIL_STATISTICS_HPP
#define SG_HARNESS_DETAIL_STATISTICS_HPP

#include <optional>
#include <utility>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

namespace sg::detail
{

/// @brief specs/029 data-model.md E-11, FR-093 (PD-05): the one statistics
/// summary the harness reports and a hunt rank both call.
///
/// The summary answers the seven fields of `sg::Statistics` for one column of
/// values: the number of values, the least and the greatest of them, their
/// mean, their median (the central value of an odd column, the average of the
/// two central values of an even one), the sample deviation over `n - 1` of
/// that mean, and the deviation over the mean, zero where the mean is zero.
/// The values arrive by value, so the sort that finds the median is the
/// summary's own.
///
/// \pre `values` holds more than one value: the guard of a column stands at
///      the call site that reads it, the report answering the zero record for
///      a column no clean repetition measured (FR-025) and a hunt rank
///      keeping its one sample out of the call (PD-14)
/// \post every field answers the value the same column of values carries, the
///       deviation reading the mean this summary computed
[[nodiscard]] auto statisticsOf(std::vector<double> values) -> sg::Statistics;

/// @brief specs/040 research PD-02 (D-8, R-4): the one least-squares sum set
/// the complexity fit and the hunt slope share.
///
/// `cross` holds sum(x * y), `square` holds sum(x * x), `xSum` holds sum(x)
/// and `ySum` holds sum(y) over one point set of `(x, y)` pairs, the four
/// figures the closed-form least-squares quotients read. The complexity fit
/// reads the cross and the square sums over the curve values of one family
/// (FR-021), the hunt slope reads all four over the time-rate points of one
/// family (B-3).
///
/// \pre none
/// \post every member answers the sum its name states over the point set
///       `leastSquaresSums` reads, accumulated in the order the pairs stand
///       in
/// \invariant the sums read the point set alone: no clock, no file and no
///            counter enters them (FR-006)
struct LeastSquaresSums
{
  double cross {};
  double square {};
  double xSum {};
  double ySum {};
};

/// @brief specs/040 research PD-02 (D-8, R-4): the least-squares sums of one
/// point set, the sums the complexity fit and the hunt slope share.
///
/// \pre none
/// \post the answer holds the four sums over `points`, accumulated in the
///       order the pairs stand in
/// \invariant the call reads the point set alone: no clock, no file and no
///            counter enters it (FR-006)
[[nodiscard]] auto leastSquaresSums(
    const std::vector<std::pair<double, double>>& points) -> LeastSquaresSums;

/// @brief specs/040 contracts/hunt-ranks-api.md B-2 (FR-001, FR-002, D-2,
/// research PD-01): the Spearman rank correlation of one sensitivity point
/// set.
///
/// Each coordinate stands ranked with average ranks at ties, and the answer
/// is the Pearson correlation of the rank pairs, a figure of [-1, 1] whose
/// bounds a strictly monotone set answers. The point set arrives by value,
/// so the ranking over it is the call's own.
///
/// \pre `points` holds no empty pair (the point assembly of B-4 guarantees
///      it)
/// \post under 3 points, or with a coordinate that carries no spread, the
///       answer stands empty (FR-002); any other point set answers the
///       Pearson correlation of the rank pairs, average ranks at ties, in
///       [-1, 1]
/// \invariant the call reads the point set alone: no clock, no file and no
///            counter enters it (FR-006)
[[nodiscard]] auto spearmanCorrelation(
    std::vector<std::pair<double, double>> points) -> std::optional<double>;

/// @brief specs/040 contracts/hunt-ranks-api.md B-3 (FR-006, D-8, research
/// PD-02): the least-squares slope of the rate against the time per
/// iteration over one sensitivity point set.
///
/// The slope stands through the closed form over the four sums
/// `leastSquaresSums` answers, the sums the complexity fit reads, so one
/// implementation of the least-squares arithmetic stands (R-4, PC-2).
///
/// \pre `points` holds no empty pair (the point assembly of B-4 guarantees
///      it)
/// \post under 2 points, or with a time coordinate that carries no spread,
///       the answer stands empty; any other point set answers the quotient
///       (count * sum(xy) - sum(x) * sum(y)) / (count * sum(x^2) - sum(x)^2)
/// \invariant the call reads the point set alone: no clock, no file and no
///            counter enters it (FR-006)
[[nodiscard]] auto leastSquaresSlope(
    std::vector<std::pair<double, double>> points) -> std::optional<double>;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_STATISTICS_HPP
