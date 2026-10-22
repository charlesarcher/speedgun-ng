#ifndef SG_HARNESS_DETAIL_STATISTICS_HPP
#define SG_HARNESS_DETAIL_STATISTICS_HPP

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

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_STATISTICS_HPP
