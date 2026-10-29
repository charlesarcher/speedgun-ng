// tools/report_view.cpp
// The report view driver (specs/038-report-view-generators FR-014 to
// FR-017, D-9): one report document in, one view out. The driver
// parses the four arguments, loads the document with `loadReport`,
// pre-checks the request against the loaded document the way the
// clarify 1/Q2 ruling fixes, calls the view function and maps the
// exits of contracts/views-api.md B-6: the view bytes and every
// disclosure on standard output at status 0, one refusal line on
// standard error at status 2.
//
// Grammar: report_view <report>, the benchmark argument, the metric
// argument and the view argument, each written as one name=value
// token; the help exit answers one line per argument row.

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/views.hpp"

namespace
{

// The bin count the driver passes the histogram, the page and the text
// views (FR-007: the bin count is the function's argument with the
// driver passing 20).
constexpr std::size_t kHistogramBins = 20;

// The four view words of the grammar (D-9), the set the view-word
// pre-check reads the request against.
constexpr std::array<std::string_view, 4> kViewWords {
    "histogram", "plot", "page", "text"};

// The parsed request of one run: every argument row of the grammar,
// present or absent.
struct Request
{
  std::optional<std::string> reportPath;
  std::optional<std::string> benchmark;
  std::optional<std::string> metric;
  std::optional<std::string> view;
};

// One line per argument row, the help exit of FR-017 prints.
void printHelp(std::ostream& out)
{
  out << "report_view <report>                     the report document the "
         "reader loads\n";
  out << "--benchmark=<name>                       the benchmark the view "
         "renders\n";
  out << "--metric=<column>                        the column: time, cpu or a "
         "metric label\n";
  out << "--view=histogram|plot|page|text          the view to render\n";
}

// One refusal line on standard error at status 2 (FR-015, table C):
// the driver prints the line before any view output and refuses.
auto refuse(const std::string& line) -> int
{
  std::cerr << "report_view: " << line << '\n';
  return 2;
}

// The request parse of the grammar (D-9): the report path as the one
// positional, the three options as name=value tokens. An unknown
// option name, a repeated argument and a second report path are parse
// faults the caller refuses (FR-015); an option naming no value and a
// missing argument answer the required check below.
auto parseRequest(const std::vector<std::string_view>& arguments)
    -> std::expected<Request, std::string>
{
  Request request;
  for (const auto& argument : arguments) {
    if (!argument.starts_with("--")) {
      if (request.reportPath.has_value()) {
        return std::unexpected("the report path repeats");
      }
      request.reportPath = std::string(argument);
      continue;
    }
    const auto split = argument.find('=');
    const auto name = argument.substr(
        0, split == std::string_view::npos ? argument.size() : split);
    std::optional<std::string>* field = nullptr;
    if (name == "--benchmark") {
      field = &request.benchmark;
    } else if (name == "--metric") {
      field = &request.metric;
    } else if (name == "--view") {
      field = &request.view;
    } else {
      return std::unexpected("the argument '" + std::string(name)
                             + "' stands outside the grammar");
    }
    if (field->has_value()) {
      return std::unexpected("the argument '" + std::string(name)
                             + "' repeats");
    }
    *field = std::string(split == std::string_view::npos
                             ? std::string_view {}
                             : argument.substr(split + 1));
  }
  return request;
}

// The first required argument the request leaves absent or empty, or
// an empty string when every row of the grammar is present (FR-015).
auto missingArgument(const Request& request) -> std::string
{
  if (!request.reportPath.has_value() || request.reportPath->empty()) {
    return "the report path is required";
  }
  const auto rows = std::array {
      std::pair {"--benchmark", &request.benchmark},
      std::pair {"--metric", &request.metric},
      std::pair {"--view", &request.view},
  };
  for (const auto& [name, field] : rows) {
    if (!field->has_value() || field->value().empty()) {
      return "the argument '" + std::string(name) + "' is required";
    }
  }
  return {};
}

// The recorded benchmark names joined with one separator, the set the
// unmatched-benchmark refusal names (A-2).
auto recordedBenchmarkNames(const sg::ReportData& report) -> std::string
{
  std::string names;
  for (const auto& result : report.results) {
    if (!names.empty()) {
      names += ", ";
    }
    names += result.name;
  }
  return names;
}

// The metric labels of one result joined with one separator, the set
// the outside-the-selector refusal names (A-2, A-3).
auto recordedMetricLabels(const sg::BenchmarkResult& result) -> std::string
{
  std::string labels;
  for (const auto& label : result.metricLabels) {
    if (!labels.empty()) {
      labels += ", ";
    }
    labels += label;
  }
  return labels;
}

// The column-selector check of A-3 against one result: the column
// stands inside the enumeration when it names the primary time column,
// the CPU-time column or a metric label of the benchmark; a counter
// label stands outside (clarify 1/Q3).
auto columnKnown(const sg::BenchmarkResult& result,
                 std::string_view column) -> bool
{
  if (column == "time" || column == "cpu") {
    return true;
  }
  return std::find(
             result.metricLabels.begin(), result.metricLabels.end(), column)
      != result.metricLabels.end();
}

// The view-word check against the four words the grammar names (D-9).
auto viewWordKnown(std::string_view view) -> bool
{
  return std::find(kViewWords.begin(), kViewWords.end(), view)
      != kViewWords.end();
}

}  // namespace

auto main(int argc, char* argv[]) -> int
{
  const auto arguments = std::vector<std::string_view>(argv + 1, argv + argc);
  if (std::find(arguments.begin(), arguments.end(), std::string_view("--help"))
      != arguments.end())
  {
    printHelp(std::cout);
    return 0;
  }

  const auto request = parseRequest(arguments);
  if (!request.has_value()) {
    return refuse(request.error());
  }
  const auto& parsed = *request;

  const auto missing = missingArgument(parsed);
  if (!missing.empty()) {
    return refuse(missing);
  }

  const auto document = sg::loadReport(*parsed.reportPath);
  if (!document.has_value()) {
    // The reader's own text, one line, at status 2 (table C row 1).
    std::cerr << document.error() << '\n';
    return 2;
  }

  // The pre-check of clarify 1/Q2: the request reads against the
  // loaded document, so a failed check refuses at status 2 and every
  // error a view function answers rides the disclosure of FR-016.
  const auto found =
      std::find_if(document->results.begin(),
                   document->results.end(),
                   [&parsed](const sg::BenchmarkResult& candidate)
                   { return candidate.name == *parsed.benchmark; });
  if (found == document->results.end()) {
    return refuse("the benchmark '" + *parsed.benchmark
                  + "' matches no result; the document records "
                  + (document->results.empty()
                         ? std::string("no benchmark")
                         : recordedBenchmarkNames(*document)));
  }
  const auto& result = *found;

  if (!columnKnown(result, *parsed.metric)) {
    return refuse("the column '" + *parsed.metric
                  + "' stands outside the selector; the benchmark '"
                  + result.name + "' records the columns time, cpu"
                  + (result.metricLabels.empty()
                         ? std::string(" and no metric label")
                         : ", " + recordedMetricLabels(result)));
  }
  if (!viewWordKnown(*parsed.view)) {
    return refuse("the view '" + *parsed.view
                  + "' stands outside histogram, plot, page, text");
  }

  auto view = std::expected<std::string, std::string> {};
  if (*parsed.view == "histogram") {
    view = sg::renderHistogram(
        *document, *parsed.benchmark, *parsed.metric, kHistogramBins);
  } else if (*parsed.view == "plot") {
    view = sg::renderPlot(*document, *parsed.benchmark, *parsed.metric);
  } else if (*parsed.view == "page") {
    view = sg::renderPage(
        *document, *parsed.benchmark, *parsed.metric, kHistogramBins);
  } else {  // "text", the last word the pre-check bounds the request to
    view = sg::renderText(
        *document, *parsed.benchmark, *parsed.metric, kHistogramBins);
  }

  if (!view.has_value()) {
    // The disclosure of FR-016: standard output at status 0.
    std::cout << view.error() << '\n';
    return 0;
  }
  std::cout << view.value();
  return 0;
}
