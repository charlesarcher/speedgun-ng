#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/views.hpp"

namespace sg
{

namespace
{

// The label and the divisor of each time unit, indexed by the enumerator:
// the same scale the console report divides a printed time value by, which
// the table has to follow (A-5, source/harness/report.cpp).
struct UnitScale
{
  const char* label;
  double divisor;
};

constexpr std::array<UnitScale, 4> kUnitScales {
    UnitScale {"ns", 1.0},
    UnitScale {"us", 1'000.0},
    UnitScale {"ms", 1'000'000.0},
    UnitScale {"s", 1'000'000'000.0},
};
static_assert(kUnitScales.size() == std::to_underlying(TimeUnit::SECONDS) + 1);

auto unitScaleOf(const TimeUnit timeUnit) -> const UnitScale&
{
  // The nanosecond scale answers a value outside the closed enumeration,
  // the one divisor that cannot divide by zero.
  const auto index = std::to_underlying(timeUnit);
  if (index >= kUnitScales.size()) {
    return kUnitScales[0];
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kUnitScales[index];
}

// The index of a metric label in the label list of one result, or the end
// of the list when it stands nowhere (A-3).
auto labelIndexOf(const std::vector<std::string>& labels,
                  std::string_view label) -> std::size_t
{
  return static_cast<std::size_t>(std::find(labels.begin(), labels.end(), label)
                                  - labels.begin());
}

// The recorded benchmark names joined with one separator, the set the
// refusal of an unmatched benchmark names (A-2).
auto recordedBenchmarkNames(const ReportData& report) -> std::string
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

// The recorded metric labels joined with one separator, the set the
// refusal of an unmatched column names (A-2, A-3).
auto recordedMetricLabels(const BenchmarkResult& result) -> std::string
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

}  // namespace

auto viewSeries(const ReportData& report,
                const std::string_view benchmark,
                const std::string_view column)
    -> std::expected<ViewSeries, std::string>
{
  SG_REQUIRE(!benchmark.empty(),
             "the resolution resolves no result of an empty benchmark name");
  SG_REQUIRE(!column.empty(),
             "the resolution resolves no column of an empty selector");

  const auto result =
      std::find_if(report.results.begin(),
                   report.results.end(),
                   [&benchmark](const BenchmarkResult& candidate)
                   { return candidate.name == benchmark; });
  if (result == report.results.end()) {
    return std::unexpected(
        "viewSeries: the benchmark '" + std::string(benchmark)
        + "' matches no result; the document records "
        + (report.results.empty() ? std::string("no benchmark")
                                  : recordedBenchmarkNames(report)));
  }

  auto values = std::vector<double> {};
  auto unit = std::string {};
  if (column == "time" || column == "cpu") {
    const auto& scale = unitScaleOf(result->timeUnit);
    for (const auto& row : result->rows) {
      if (row.kind != RowKind::REPETITION) {
        continue;
      }
      if (row.timeAvailability != sg::counters::Availability::COUNTABLE) {
        continue;
      }
      values.push_back(column == "time"
                           ? row.timePerIterationNs / scale.divisor
                           : row.cpuTimePerIterationNs / scale.divisor);
    }
    unit = scale.label;
    return ViewSeries {std::move(values), std::move(unit)};
  }

  const auto index = labelIndexOf(result->metricLabels, column);
  if (index >= result->metricLabels.size()) {
    return std::unexpected("viewSeries: the column '" + std::string(column)
                           + "' stands outside the selector; the benchmark '"
                           + result->name + "' records the metric labels "
                           + (result->metricLabels.empty()
                                  ? std::string("none")
                                  : recordedMetricLabels(*result)));
  }
  for (const auto& row : result->rows) {
    if (row.kind != RowKind::REPETITION) {
      continue;
    }
    if (index >= row.metrics.size()
        || row.metrics[index].availability
            != sg::counters::Availability::COUNTABLE)
    {
      continue;
    }
    values.push_back(row.metrics[index].value);
  }
  return ViewSeries {std::move(values), std::move(unit)};
}

}  // namespace sg
