#include <algorithm>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "escape.hpp"
#include "figure.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/views.hpp"

namespace sg
{

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

  const auto bins = detail::buildBins(values, binCount);

  // The bars stand proportional to the counts, the widest bar 200
  // units wide (B-2); every bin carries a bar, the empty bin a bar of
  // width zero.
  const auto widest =
      std::max_element(bins.begin(),
                       bins.end(),
                       [](const detail::Bin& left, const detail::Bin& right)
                       { return left.count < right.count; })
          ->count;

  auto svg = std::string {};
  svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"320\" height=\""
      + std::to_string(24 + 40 * binCount) + "\">";
  svg += "<text x=\"4\" y=\"16\">" + detail::escapeMarkup(column) + "</text>";
  for (std::size_t bin = 0; bin < binCount; ++bin) {
    const auto width = 200 * bins[bin].count / widest;
    const auto rowTop = 24 + 40 * bin;
    svg += "<rect x=\"4\" y=\"" + std::to_string(rowTop) + "\" width=\""
        + std::to_string(width) + "\" height=\"24\"/>";
    svg += "<text x=\"216\" y=\"" + std::to_string(rowTop + 18) + "\">"
        + detail::binLabel(
               bins[bin].low, bins[bin].high, series->unit, bins[bin].count)
        + "</text>";
  }
  svg += "</svg>";
  // The bars stand one per bin, the count the argument bounds (B-2).
  auto bars = std::size_t {};
  for (auto pos = svg.find("<rect"); pos != std::string::npos;
       pos = svg.find("<rect", pos + 1))
  {
    ++bars;
  }
  SG_ENSURE(bars == binCount, "renderHistogram: one bar stands per bin (B-2)");
  return svg;
}

}  // namespace sg
