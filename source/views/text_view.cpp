#include <algorithm>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

#include "escape.hpp"
#include "figure.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/views.hpp"

namespace sg
{

namespace
{

// The width of the widest character bar one bin line carries, in '#'
// characters: the bars stand proportional to the counts under this
// bound (B-4).
constexpr std::size_t kWidestBar = 20;

// The line of one bin: the label the SVG histogram names the bin by,
// the character bar proportional to the count beside it (B-4), the
// empty bin carrying no bar.
auto binLine(const detail::Bin& bin,
             const std::string& unit,
             const std::size_t widest) -> std::string
{
  auto line = detail::binLabel(bin.low, bin.high, unit, bin.count);
  const auto bar = kWidestBar * bin.count / widest;
  if (bar != 0) {
    line += ' ' + std::string(bar, '#');
  }
  return line;
}

// The line of one recorded value: the value in the figureNumber form,
// the unit named beside it when the column carries one (A-5), the way
// the plot labels its points at source/views/plot.cpp.
auto valueLine(const double value, const std::string& unit) -> std::string
{
  auto line = detail::figureNumber(value);
  if (!unit.empty()) {
    line += ' ' + unit;
  }
  return line;
}

}  // namespace

auto renderText(const ReportData& report,
                const std::string_view benchmark,
                const std::string_view column,
                const std::size_t binCount)
    -> std::expected<std::string, std::string>
{
  SG_REQUIRE(binCount >= 2, "the text view renders at least two bins");

  const auto series = viewSeries(report, benchmark, column);
  if (!series.has_value()) {
    return std::unexpected(series.error());
  }

  // The view names its column, the document-derived label text the
  // escaper runs through (A-6); the plain numbers of the bin and the
  // value lines carry no escape requirement.
  auto text = detail::escapeMarkup(column);

  // The view composes per part (B-4, clarify 2/Q1): each part renders
  // by the rule the part follows alone, a figure answering the
  // disclosure leaving the disclosure line in the part's place, and
  // the whole answering the value form.
  const auto histogram = renderHistogram(report, benchmark, column, binCount);
  if (histogram.has_value()) {
    const auto bins = detail::buildBins(series->values, binCount);
    const auto widest =
        std::max_element(bins.begin(),
                         bins.end(),
                         [](const detail::Bin& left, const detail::Bin& right)
                         { return left.count < right.count; })
            ->count;
    for (const auto& bin : bins) {
      text += '\n' + binLine(bin, series->unit, widest);
    }
  } else {
    text += '\n' + histogram.error();
  }

  const auto plot = renderPlot(report, benchmark, column);
  if (plot.has_value()) {
    for (const double value : series->values) {
      text += '\n' + valueLine(value, series->unit);
    }
  } else {
    text += '\n' + plot.error();
  }

  return text;
}

}  // namespace sg
