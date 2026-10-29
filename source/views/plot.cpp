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

// One recorded value printed in the shortest form that reads the value
// back: the point labels print the recorded values in this form, the
// one the histogram prints its bin ranges in at
// source/views/histogram.cpp.
auto figureNumber(const double value) -> std::string
{
  char text[32];
  std::snprintf(text, sizeof text, "%g", value);
  return text;
}

// The label of one point: the value and the unit named beside it
// (A-5), the text escaped where the format can read it as markup
// (A-6).
auto pointLabel(const double value, const std::string& unit) -> std::string
{
  auto text = figureNumber(value);
  if (!unit.empty()) {
    text += ' ' + unit;
  }
  return detail::escapeMarkup(text);
}

// The label naming the column beside the values it scales: the unit
// stands in parentheses when the column carries one (A-5), the text
// escaped where the format can read it as markup (A-6).
auto columnLabel(const std::string_view column,
                 const std::string& unit) -> std::string
{
  auto text = std::string(column);
  if (!unit.empty()) {
    text += " (" + unit + ")";
  }
  return detail::escapeMarkup(text);
}

}  // namespace

auto renderPlot(const ReportData& report,
                const std::string_view benchmark,
                const std::string_view column)
    -> std::expected<std::string, std::string>
{
  const auto series = viewSeries(report, benchmark, column);
  if (!series.has_value()) {
    return std::unexpected(series.error());
  }
  const auto& values = series->values;
  if (values.empty()) {
    return std::unexpected(
        "renderPlot: the plot of the column '" + std::string(column) + "' of '"
        + std::string(benchmark)
        + "' stands not available: the series carries the sample count 0");
  }

  const auto low = *std::min_element(values.begin(), values.end());
  const auto high = *std::max_element(values.begin(), values.end());
  const auto span = high - low;

  // The points stand in the plot area the two axes frame: the row axis
  // spans x 40 to 280, the value axis spans y 24 to 176 inverted, since
  // SVG grows y downward. One recorded value stands the plot's center
  // point, the single-value case B-3 pins, and a degenerate range,
  // every recorded value equal, stands the center line, the mapping
  // dividing by no span there.
  auto points = std::string {};
  auto labels = std::string {};
  for (std::size_t index = 0; index < values.size(); ++index) {
    const auto x = values.size() == 1 ? 160.0
                                      : 40.0
            + 240.0 * static_cast<double>(index)
                / static_cast<double>(values.size() - 1);
    const auto y =
        span > 0.0 ? 24.0 + 152.0 * (high - values[index]) / span : 100.0;
    if (!points.empty()) {
      points += ' ';
    }
    points += std::to_string(x) + ',' + std::to_string(y);
    labels += "<text x=\"" + std::to_string(x + 4) + "\" y=\""
        + std::to_string(y - 4) + "\">"
        + pointLabel(values[index], series->unit) + "</text>";
  }

  auto svg = std::string {};
  svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"320\" "
         "height=\"200\">";
  svg += "<polyline points=\"" + points + "\" fill=\"none\" "
         "stroke=\"black\"/>";
  svg +=
      "<text x=\"4\" y=\"16\">" + columnLabel(column, series->unit) + "</text>";
  svg += "<text x=\"4\" y=\"192\">row</text>";
  svg += labels;
  svg += "</svg>";
  return svg;
}

}  // namespace sg
