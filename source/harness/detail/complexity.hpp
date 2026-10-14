#ifndef SG_HARNESS_DETAIL_COMPLEXITY_HPP
#define SG_HARNESS_DETAIL_COMPLEXITY_HPP

#include <functional>
#include <string>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

namespace sg::detail
{

// FR-022: one quantity of a family's fit. The caller fills the label, whether
// the report scales its coefficient by the unit of the family, and the value
// each instance of the family carries, in instance order; a value of NaN marks
// an instance that carries no value for that quantity, which leaves the
// quantity without its pair (FR-023, spec Edge Cases). The fit fills the curve
// token, the coefficient, the root-mean relative error and, when it holds no
// pair, the reason.
struct FitQuantity
{
  std::string label;
  bool scalesWithUnit = false;
  std::vector<double> values;
  std::string curve;
  double coefficient = 0.0;
  double rmsPercent = 0.0;
  std::string reason;
};

// FR-022, FR-023: the outcome of one family's fit. The reason is the family's
// own when the fit did not run at all, so no quantity carries a pair; otherwise
// the quantities stand in the order the report prints them, each with its own
// reason when that quantity does not stand in every instance.
struct FamilyFit
{
  std::string reason;
  std::vector<FitQuantity> quantities;
};

// FR-021, research D-08: the fit of one family over its instances. The sizes
// are the problem size of each instance and the quantities carry one value per
// instance, both in the same instance order.
//
// `selection` names one built-in curve and that curve stands whatever error it
// earns (FR-020); `BigO::AUTOMATIC` tries each built-in curve in the order of
// the table and keeps the least root-mean relative error, with the constant
// curve as the fallback (FR-021); a callable `function` is the curve of the fit
// and its rows name it `f(N)` (FR-020).
//
// The coefficient of a curve f minimizes the squared relative error
// (k * f(N) - y) / y, which is k = sum(f(N) / y) / sum((f(N) / y)^2), and the
// RMS is the root-mean of the same relative errors, as a percentage (FR-021,
// FR-022).
//
// \pre `sizes` and the values of every quantity hold the same number of
//      entries, and `selection` is not `BigO::NONE` (FR-019)
// \post a fit that ran holds the curve, coefficient and RMS of every quantity
//       that stands in every instance, and a reason on the rest (FR-022,
//       FR-023)
auto fitFamily(const std::vector<std::int64_t>& sizes,
               std::vector<FitQuantity> quantities,
               BigO selection,
               const std::function<double(double)>& function) -> FamilyFit;

}  // namespace sg::detail

#endif
