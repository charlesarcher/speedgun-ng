#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/complexity.hpp"
#include "detail/statistics.hpp"

#include "speedgun-ng/benchmark.hpp"

namespace sg::detail
{

namespace
{

// One built-in curve of the fit: the token its rows print and the curve itself.
struct Curve
{
  std::string_view token;
  double (*function)(double n) noexcept;
};

// The table stands in the order of the `BigO` enumerators after `NONE`, so a
// selection maps to its entry by subtracting one from its underlying value, and
// the constant curve leads so it stands as the fallback of an automatic fit
// (FR-020, FR-021).
constexpr std::array<Curve, 6> kCurves {{
    {"1", [](const double) noexcept { return 1.0; }},
    {"N", [](const double n) noexcept { return n; }},
    {"N^2", [](const double n) noexcept { return n * n; }},
    {"N^3", [](const double n) noexcept { return n * n * n; }},
    {"lgN", [](const double n) noexcept { return std::log2(n); }},
    {"NlgN", [](const double n) noexcept { return n * std::log2(n); }},
}};

static_assert(kCurves.size() == std::to_underlying(BigO::N_LOG_N));

// The token of a user-supplied curve (FR-020).
constexpr std::string_view kUserCurveToken = "f(N)";

// The reason of a family that holds too few instances to fit (FR-023).
constexpr std::string_view
    kTooFewInstances =
        "fewer than two instances hold a measured value, so the family holds "
        "no " "fit (FR-023)";

// The reason of a family whose instance states no positive problem size
// (FR-019, FR-023).
constexpr std::string_view
    kNoPositiveSize =
        "an instance carries no positive complexity size, so the family holds "
        "no " "fit (FR-023)";

// The reason of a quantity that does not stand in every instance of the family
// (FR-023, spec Edge Cases).
constexpr std::string_view
    kMissingQuantity =
        "the quantity does not stand in every instance of the family, so it "
        "holds " "no fit pair (FR-023)";

// The coefficient and the relative root-mean-square error of one curve: the
// coefficient is the least-squares solution sum(t * f(N)) / sum(f(N)^2), and
// the RMS is the root-mean of the residuals t - coefficient * f(N) divided by
// the mean of the values, printed as a percentage (FR-021, FR-022). A curve
// that is zero at every size has no solution and reports an infinite error, so
// it stands no fit; a family whose values average zero reports a zero error
// when the fit reproduces them and an infinite error when it does not.
auto fitCurve(const std::vector<std::int64_t>& sizes,
              const std::vector<double>& values,
              const std::function<double(double)>& curve)
    -> std::pair<double, double>
{
  // The sums stand in the shared helper beside the statistics surface
  // (PD-02, D-8): the fit reads the cross and the square sums over the
  // curve values of one curve, so its coefficient and its error stand as
  // they did when the sums lived here.
  auto points = std::vector<std::pair<double, double>> {};
  points.reserve(values.size());
  for (std::size_t index = 0; index < values.size(); ++index) {
    points.emplace_back(curve(static_cast<double>(sizes[index])),
                        values[index]);
  }
  const auto sums = leastSquaresSums(points);
  if (sums.square <= 0.0) {
    return {0.0, std::numeric_limits<double>::infinity()};
  }
  const double coefficient = sums.cross / sums.square;

  double squared = 0.0;
  double total = 0.0;
  for (std::size_t index = 0; index < values.size(); ++index) {
    const double residual =
        values[index] - coefficient * curve(static_cast<double>(sizes[index]));
    squared += residual * residual;
    total += values[index];
  }
  const double mean = total / static_cast<double>(values.size());
  if (std::abs(mean) <= std::numeric_limits<double>::min()) {
    const double error =
        squared <= 0.0 ? 0.0 : std::numeric_limits<double>::infinity();
    return {coefficient, error};
  }
  const double rootMean =
      std::sqrt(squared / static_cast<double>(values.size()));
  return {coefficient, rootMean / mean * 100.0};
}

// Whether every instance of the family carries a value of one quantity: a NaN
// value marks the instance that carries none (FR-023, spec Edge Cases).
auto holdsEveryValue(const std::vector<double>& values) -> bool
{
  for (const double value : values) {
    if (std::isnan(value)) {
      return false;
    }
  }
  return true;
}

// One candidate curve of a family's fit: the token its rows print and the curve
// to fit (FR-020, FR-021).
struct Candidate
{
  std::string_view token;
  std::function<double(double)> curve;
};

}  // namespace

auto fitFamily(const std::vector<std::int64_t>& sizes,
               std::vector<FitQuantity> quantities,
               const BigO selection,
               const std::function<double(double)>& function) -> FamilyFit
{
  auto outcome = FamilyFit {.reason = {}, .quantities = std::move(quantities)};
  if (sizes.size() < 2) {
    outcome.reason = kTooFewInstances;
    return outcome;
  }
  for (const std::int64_t size : sizes) {
    if (size <= 0) {
      outcome.reason = kNoPositiveSize;
      return outcome;
    }
  }

  // The candidate curves of the fit: a user function stands on its own, a fixed
  // selection names one entry of the table, and an automatic fit holds the
  // whole table from the constant curve, which therefore stands when no other
  // curve earns a smaller error (FR-020, FR-021).
  auto candidates = std::vector<Candidate> {};
  if (static_cast<bool>(function)) {
    candidates.push_back({kUserCurveToken, function});
  } else if (selection == BigO::AUTOMATIC) {
    for (const auto& curve : kCurves) {
      candidates.push_back({curve.token, curve.function});
    }
  } else {
    const auto& curve = kCurves.at(
        static_cast<std::size_t>(std::to_underlying(selection)) - 1U);
    candidates.push_back({curve.token, curve.function});
  }

  for (auto& quantity : outcome.quantities) {
    if (!holdsEveryValue(quantity.values)) {
      quantity.reason = kMissingQuantity;
      continue;
    }

    auto best = fitCurve(sizes, quantity.values, candidates.front().curve);
    std::string_view bestToken = candidates.front().token;
    for (std::size_t index = 1; index < candidates.size(); ++index) {
      const auto candidate =
          fitCurve(sizes, quantity.values, candidates.at(index).curve);
      if (candidate.second < best.second) {
        best = candidate;
        bestToken = candidates.at(index).token;
      }
    }
    quantity.curve = bestToken;
    quantity.coefficient = best.first;
    quantity.rmsPercent = best.second;
  }
  return outcome;
}

}  // namespace sg::detail
