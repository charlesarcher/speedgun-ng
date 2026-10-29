#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

#include "escape.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/views.hpp"

namespace sg
{

namespace
{

// One identity field of the body: the field's name, the value the
// document or the request carries, the standing disclosure in the
// place of a value no document produced (FR-012), the text escaped
// where the format can read it as markup (A-6).
auto identityField(const std::string_view name,
                   const std::string& value) -> std::string
{
  auto field = std::string("<p>") + detail::escapeMarkup(name) + ": ";
  field += value.empty() ? "not available" : detail::escapeMarkup(value);
  return field + "</p>";
}

// One figure's place in the body: a rendering figure's bytes stand the
// standalone bytes (D-3), the figure escaping its own document-derived
// text; a figure that answers the disclosure leaves the disclosure
// text in the figure's place (FR-012, clarify 1/Q4), the disclosure
// naming document-derived text the escaper runs through (A-6).
auto figureBytes(const std::expected<std::string, std::string>& figure)
    -> std::string
{
  return figure.has_value() ? figure.value()
                            : detail::escapeMarkup(figure.error());
}

}  // namespace

auto renderPage(const ReportData& report,
                const std::string_view benchmark,
                const std::string_view column,
                const std::size_t binCount)
    -> std::expected<std::string, std::string>
{
  SG_REQUIRE(binCount >= 2, "the page renders at least two bins");

  // The two figures render by the rules they follow alone (B-5): a
  // figure that answers the disclosure leaves the disclosure text in
  // the figure's place, and the page answers the value form (FR-012,
  // clarify 1/Q4).
  const auto histogram = renderHistogram(report, benchmark, column, binCount);
  const auto plot = renderPlot(report, benchmark, column);

  auto page = std::string {"<!DOCTYPE html>"};
  page += "<html>";
  page += "<head><title>" + detail::escapeMarkup(benchmark) + ": "
      + detail::escapeMarkup(column) + "</title></head>";
  page += "<body>";
  page += identityField("speedgun", report.speedgunVersion);
  page += identityField("schemaVersion", report.schemaVersion);
  page += identityField("benchmark", std::string(benchmark));
  page += identityField("column", std::string(column));
  page += figureBytes(histogram);
  page += figureBytes(plot);
  page += "</body>";
  page += "</html>";
  return page;
}

}  // namespace sg
