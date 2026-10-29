#ifndef SG_VIEWS_FIGURE_HPP
#define SG_VIEWS_FIGURE_HPP

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "escape.hpp"

/**
 * @file figure.hpp
 * @brief The shared figure helpers the SVG histogram and the text
 * view run their recorded values, their bins and their bin labels
 * through (B-2, B-4 of
 * specs/038-report-view-generators/contracts/views-api.md).
 *
 * Nothing here is public API: the header carries no exported symbol,
 * the functions answer in the translation units that include it, and
 * the file joins no build target.
 */

namespace sg::detail
{

/**
 * @brief One recorded value printed in the shortest form that reads
 * the value back.
 *
 * The bin range labels print the recorded values and the bin edges in
 * this form, the one the compare guard prints its numbers in at
 * source/harness/compare.cpp.
 *
 * \pre none
 * \post the answer spells `value` the way `printf` spells `%g`
 */
[[nodiscard]] inline auto figureNumber(const double value) -> std::string
{
  char text[32];
  std::snprintf(text, sizeof text, "%g", value);
  return text;
}

/**
 * @brief One bin of the histogram: the range it covers and the count
 * of the recorded values it takes (B-2).
 */
struct Bin
{
  double low;
  double high;
  std::size_t count;
};

/**
 * @brief The bins of one histogram over the recorded values (B-2).
 *
 * \pre `values` carries at least one recorded value and `binCount` is
 *      at least 2; the public views check both at their own boundary,
 *      the one place each contract stands
 * \post the bins cover the recorded value range with the last bin
 *      closed on the maximum, and the counts sum to `values.size()`
 */
[[nodiscard]] inline auto buildBins(const std::vector<double>& values,
                                    const std::size_t binCount)
    -> std::vector<Bin>
{
  const auto low = *std::min_element(values.begin(), values.end());
  const auto high = *std::max_element(values.begin(), values.end());

  auto counts = std::vector<std::size_t>(binCount, 0);
  for (const double value : values) {
    // The bin of the value: the bins stand half-open over the recorded
    // range and the last bin stands closed on the maximum, so the
    // maximum lands in the last bin (B-2). A degenerate range, every
    // recorded value equal, stands in the closed bin too, the
    // equal-values edge case the spec pins.
    auto index = binCount - 1;
    if (low < high) {
      const auto scaled =
          (value - low) / (high - low) * static_cast<double>(binCount);
      index = scaled < static_cast<double>(binCount)
          ? static_cast<std::size_t>(scaled)
          : binCount - 1;
    }
    ++counts[index];
  }

  auto bins = std::vector<Bin> {};
  bins.reserve(binCount);
  const auto span = high - low;
  for (std::size_t bin = 0; bin < binCount; ++bin) {
    const auto binWidth = span / static_cast<double>(binCount);
    const auto lowEdge = low + binWidth * static_cast<double>(bin);
    const auto highEdge = bin + 1 == binCount
        ? high
        : low + binWidth * static_cast<double>(bin + 1);
    bins.push_back(Bin {lowEdge, highEdge, counts[bin]});
  }
  return bins;
}

/**
 * @brief The label of one bin: the range and the count in one text,
 * the unit named beside the values when the column carries one (A-5),
 * the text escaped where the format can read it as markup (A-6).
 *
 * \pre none
 * \post the answer names the range in the figureNumber form, the unit
 *       beside when `unit` names one, and the count after the `: `
 *       separator; the answer stands escaped (A-6)
 */
[[nodiscard]] inline auto binLabel(const double low,
                                   const double high,
                                   const std::string& unit,
                                   const std::size_t count) -> std::string
{
  auto text = figureNumber(low) + '-' + figureNumber(high);
  if (!unit.empty()) {
    text += ' ' + unit;
  }
  text += ": " + std::to_string(count);
  return escapeMarkup(text);
}

}  // namespace sg::detail

#endif
