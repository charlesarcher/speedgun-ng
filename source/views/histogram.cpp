#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "escape.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/views.hpp"

namespace sg
{

namespace
{

// One recorded value printed in the shortest form that reads the
// value back: the bin range labels print the recorded values and the
// bin edges in this form, the one the compare guard prints its numbers
// in at source/harness/compare.cpp.
auto figureNumber(const double value) -> std::string
{
  char text[32];
  std::snprintf(text, sizeof text, "%g", value);
  return text;
}

// The label of one bin: the range and the count in one text, the unit
// named beside the values when the column carries one (A-5), the text
// escaped where the format can read it as markup (A-6).
auto binLabel(const double low,
              const double high,
              const std::string& unit,
              const std::size_t count) -> std::string
{
  auto text = figureNumber(low) + '-' + figureNumber(high);
  if (!unit.empty()) {
    text += ' ' + unit;
  }
  text += ": " + std::to_string(count);
  return detail::escapeMarkup(text);
}

}  // namespace

auto renderHistogram(const ReportData& report,
                     const std::string_view benchmark,
                     const std::string_view column,
                     const std::size_t binCount)
    -> std::expected<std::string, std::string>
{
  SG_REQUIRE(binCount >= 2, "the histogram renders at least two bins");

  const auto series = viewSeries(report, benchmark, column);
  if (!series.has_value()) {
    return std::unexpected(series.error());
  }
  const auto& values = series->values;
  if (values.size() < 2) {
    return std::unexpected(
        "renderHistogram: the histogram of the column '" + std::string(column)
        + "' of '" + std::string(benchmark)
        + "' stands not available: the series carries the sample count "
        + std::to_string(values.size()));
  }

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

  // The bars stand proportional to the counts, the widest bar 200
  // units wide (B-2); every bin carries a bar, the empty bin a bar of
  // width zero.
  const auto widest = *std::max_element(counts.begin(), counts.end());

  auto svg = std::string {};
  svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"320\" height=\""
      + std::to_string(24 + 40 * binCount) + "\">";
  svg += "<text x=\"4\" y=\"16\">" + detail::escapeMarkup(column) + "</text>";
  const auto span = high - low;
  for (std::size_t bin = 0; bin < binCount; ++bin) {
    const auto width = 200 * counts[bin] / widest;
    const auto rowTop = 24 + 40 * bin;
    svg += "<rect x=\"4\" y=\"" + std::to_string(rowTop) + "\" width=\""
        + std::to_string(width) + "\" height=\"24\"/>";
    const auto binWidth = span / static_cast<double>(binCount);
    const auto lowEdge = low + binWidth * static_cast<double>(bin);
    const auto highEdge = bin + 1 == binCount
        ? high
        : low + binWidth * static_cast<double>(bin + 1);
    svg += "<text x=\"216\" y=\"" + std::to_string(rowTop + 18) + "\">"
        + binLabel(lowEdge, highEdge, series->unit, counts[bin]) + "</text>";
  }
  svg += "</svg>";
  return svg;
}

}  // namespace sg
