#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <sched.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace sg::detail
{

// The names sit in a table indexed by the enumerator, and a value
// outside the closed enumeration answers the one name the report and
// the listing print for it (FR-024).
constexpr std::array<const char*, 6> kAvailabilityNames {
    "countable",
    "permission-blocked",
    "not-encodable",
    "absent",
    "scope-refused",
    "gap",
};
static_assert(kAvailabilityNames.size()
              == std::to_underlying(sg::counters::Availability::GAP) + 1);

auto availabilityName(const sg::counters::Availability availability) -> const
    char*
{
  // The static_assert above ties the table to the last enumerator, so
  // the index of a value of the closed enumeration sits inside the
  // table, and the guard answers every other value (FR-021, FR-024).
  const auto index = std::to_underlying(availability);
  if (index >= kAvailabilityNames.size()) {
    return "unknown";
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kAvailabilityNames[index];
}

namespace
{

// The label and the divisor of each time unit, indexed by the
// enumerator, so one printed line carries one scale (E-02, FR-015).
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
static_assert(kUnitScales.size()
              == std::to_underlying(sg::TimeUnit::SECONDS) + 1);

auto unitScale(const sg::TimeUnit timeUnit) -> const UnitScale&
{
  // The static_assert above ties the table to the last enumerator, so
  // the index of a value of the closed enumeration sits inside the
  // table, and the guard answers every other value at the nanosecond
  // scale, the one divisor that cannot divide by zero (FR-015).
  const auto index = std::to_underlying(timeUnit);
  if (index >= kUnitScales.size()) {
    return kUnitScales[0];
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kUnitScales[index];
}

// FR-022, FR-023: the three rows one fitted quantity prints. The coefficient of
// the primary time follows the unit the family printed under, as every other
// time value of the report does; a counter or metric coefficient stands in that
// quantity's value, and the RMS is a percentage, so the unit scales neither.
auto printFitRow(const BenchmarkResult& result,
                 const ResultRow& row,
                 const UnitScale& scale) -> void
{
  const double coefficient = row.fit.scalesWithUnit
      ? row.fit.coefficient / scale.divisor
      : row.fit.coefficient;
  if (row.kind == RowKind::BIG_O) {
    std::printf("%-24s %10s  %s %s (%9.3f)\n",
                result.name.c_str(),
                "BIG_O",
                row.fit.quantity.c_str(),
                row.fit.curve.c_str(),
                coefficient);
    return;
  }
  if (row.kind == RowKind::RMS) {
    std::printf("%-24s %10s  %s %9.3f%%\n",
                result.name.c_str(),
                "RMS",
                row.fit.quantity.c_str(),
                row.fit.rmsPercent);
    return;
  }
  std::printf("%-24s %10s  %s: %s\n",
              result.name.c_str(),
              "SKIPPED",
              row.fit.quantity.c_str(),
              row.fit.reason.c_str());
}

// The wait mode names of the disclosure line, indexed by the enumerator,
// the table shape the availability names above take (FR-025).
constexpr std::array<const char*, 3> kWaitModeNames {
    "SPIN",
    "YIELD",
    "BLOCK",
};
static_assert(kWaitModeNames.size()
              == std::to_underlying(sg::WaitMode::BLOCK) + 1);

auto waitModeName(const sg::WaitMode waitMode) -> const char*
{
  // The static_assert above ties the table to the last enumerator of a
  // closed enumeration, so the index of a value of that enumeration sits
  // inside the table (FR-025).
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kWaitModeNames[std::to_underlying(waitMode)];
}

// FR-025: the one line a threaded result prints after its rows. It names
// the number of contexts and, for each context, the wait mode and the
// placement that context ran under. A context whose slot named no cpu
// prints those two fields. The line formats the records of
// `result.contexts` and adds nothing to them.
// The flag names of the placement account: whether the installed placement
// function reported its slot as landed. The line prints one for a slot
// that named a cpu, the only slot whose pin can land (FR-005, FR-025).
constexpr const char* kAppliedFlag = "applied";
constexpr const char* kUnappliedFlag = "unapplied";

// The mark that stands where the placement field cut a text short.
constexpr char kTruncationMark = '~';

// FR-025, specs/025 FR-021: the width of the placement field. The
// default placement function produces the `cpu@group` pair of the slot
// it ran on, and a pair of two wide indices stands longer than this
// field, so the field cuts that text and marks the cut, the way it
// cuts a long text from a replacement function. The width follows the
// type of the header's `PlacementSlot::cpuIndex` and keeps the shape
// the docs print.
constexpr std::size_t kPlacementFieldWidth =
    std::numeric_limits<decltype(sg::PlacementSlot::cpuIndex)>::digits10 + 1;
static_assert(kPlacementFieldWidth >= kNoPinPlacement.size());
static_assert(kPlacementFieldWidth >= kUnappliedPlacement.size());

auto printContextDisclosure(const BenchmarkResult& result) -> void
{
  std::printf("  contexts=%zu", result.contexts.size());
  for (std::size_t index = 0; index < result.contexts.size(); ++index) {
    const auto& context = result.contexts[index];
    const std::string field = placementField(context.placement);
    // FR-025: the flag is the account of a pin that was asked for. The
    // no-pin slot asked for nothing to land, so its field closes after
    // the placement name.
    if (context.placement == kNoPinPlacement) {
      std::printf("  ctx%zu=%s/%s",
                  index,
                  waitModeName(context.waitMode),
                  field.c_str());
      continue;
    }
    std::printf("  ctx%zu=%s/%s/%s",
                index,
                waitModeName(context.waitMode),
                field.c_str(),
                context.applied ? kAppliedFlag : kUnappliedFlag);
  }
  std::printf("\n");
}

// FR-026, FR-028: the noise warning of one result. The condition and the
// three fields of its account stand in `noiseWarningOf`, which the
// document writes beside the same result (specs/023 FR-017, D-11); this
// line prints the account and adds nothing to it.
auto printNoiseWarning(const BenchmarkResult& result) -> void
{
  const auto warning = noiseWarningOf(result);
  if (!warning.has_value()) {
    return;
  }
  std::printf(
      "warning: spinning contexts oversubscribed (%llu spin, " "%llu cpus)\n",
      static_cast<unsigned long long>(warning->spinning),
      static_cast<unsigned long long>(warning->runnableCpus));
}

}  // namespace

// FR-025: the placement field of one context. A text no longer than the
// field stands whole; a longer one stands cut to the field with the mark of
// the cut in its last position, so a reader sees that the text ran past the
// field.
auto placementField(const std::string_view placement) -> std::string
{
  if (placement.size() <= kPlacementFieldWidth) {
    return std::string(placement);
  }
  std::string field(placement.substr(0, kPlacementFieldWidth - 1));
  field.push_back(kTruncationMark);
  return field;
}

// FR-026: the choice the noise warning makes between the two cpu counts.
// It stands at external linkage so the report test can drive each answer;
// the affinity read above has no branch of its own.
auto selectRunnableCpuCount(const bool maskAnswered,
                            const std::uint64_t masked,
                            const std::uint64_t host) noexcept -> std::uint64_t
{
  if (maskAnswered) {
    return masked;
  }
  return host;
}

auto printContext(const bool placementInEffect) -> void
{
  // R-08: the catalog publishes no host or cpu model string at this
  // head, so those fields stay out of the line (FR-035).
  std::printf("speedgun-ng %s build=%s\n", SG_PROJECT_VERSION, SG_BUILD_TYPE);
  if (placementInEffect) {
    // FR-020, R-09: the layout read once at process start answers the
    // counts, and the line stands directly below the version line,
    // once per report (specs/022).
    const auto& machine = detail::layout();
    std::printf("topology: packages=%u l3-groups=%zu cores=%zu threads=%zu\n",
                machine.packages,
                machine.groups.size(),
                machine.cores.size(),
                machine.threads.size());
  }
}

// specs/021 FR-010, FR-013: one percentile line. The tag field names the
// line, the label names the column it answers, and a value the run cannot
// answer prints the token `not available` in its field. The divisor scales
// a time column the way the row scales its own time fields; a column that
// prints no time takes 1.0.
auto printPercentileLine(const std::string& name,
                         const char* tag,
                         const std::string& column,
                         const std::vector<double>& points,
                         const std::vector<sg::PercentileValue>& values,
                         const double divisor) -> void
{
  std::printf("%-24s %10s  %s", name.c_str(), tag, column.c_str());
  for (std::size_t at = 0; at < points.size(); ++at) {
    char point[32];
    std::snprintf(point, sizeof point, "p%g", points[at]);
    std::printf("  %s=", point);
    if (values[at].available) {
      std::printf("%9.3f", values[at].value / divisor);
    } else {
      std::printf("not available");
    }
  }
  std::printf("\n");
}

// specs/021 FR-010, SC-001: the label of one percentile column. The first
// two entries answer the two time columns of the row, the primary first and
// then the one the primary is not; the rest answer the metric columns in the
// order the row prints them.
auto percentileColumnLabel(const BenchmarkResult& result,
                           const std::size_t column,
                           std::string_view unitLabel) -> std::string
{
  const std::string unit(unitLabel);
  if (column == 0) {
    return "primary time/iter (" + unit + ")";
  }
  if (column == 1) {
    return (result.primaryIsCpu ? std::string("real time/iter (")
                                : std::string("cpu time/iter ("))
        + unit + ")";
  }
  return result.metricLabels[column - 2];
}

// specs/021 FR-002: the two time columns print their values divided by the
// unit scale; a metric column prints the value its own fold reached.
auto percentileColumnDivisor(const std::size_t column,
                             const double timeDivisor) -> double
{
  return column <= 1 ? timeDivisor : 1.0;
}

// specs/021 FR-007, FR-010: the chunk disclosure and the percentile lines
// of one repetition row, printed under that row. A run that captured no
// chunks prints neither (FR-023).
auto printRowPercentiles(const BenchmarkResult& result,
                         const sg::ResultRow& row,
                         const UnitScale& scale) -> void
{
  if (result.percentilePoints.empty() || row.chunkCount == 0) {
    return;
  }
  std::printf("%-24s %10s  chunks=%llu  iterations/chunk=%llu\n",
              result.name.c_str(),
              "chunks",
              static_cast<unsigned long long>(row.chunkCount),
              static_cast<unsigned long long>(row.chunkIterations));
  for (std::size_t column = 0; column < row.percentiles.size(); ++column) {
    printPercentileLine(result.name,
                        "percentile",
                        percentileColumnLabel(result, column, scale.label),
                        result.percentilePoints,
                        row.percentiles[column],
                        percentileColumnDivisor(column, scale.divisor));
  }
}

auto printResult(const BenchmarkResult& result) -> void
{
  if (result.outcome == RunOutcome::FAILED) {
    std::printf(
        "%-24s FAILED: %s\n", result.name.c_str(), result.reason.c_str());
    return;
  }
  if (result.outcome == RunOutcome::SKIPPED) {
    // FR-031: the reason prints, and no statistics print.
    std::printf(
        "%-24s SKIPPED: %s\n", result.name.c_str(), result.reason.c_str());
    return;
  }

  const auto& scale = unitScale(result.timeUnit);
  // FR-016, FR-017, FR-018: a family that asked to see its aggregates alone
  // shows no repetition row, and a family with no aggregate row at all shows
  // the rows it has: the mode hides no row that stands without an aggregate.
  const bool holdsAggregate =
      std::any_of(result.rows.begin(),
                  result.rows.end(),
                  [](const sg::ResultRow& row) noexcept -> bool
                  { return row.kind == RowKind::AGGREGATE; });
  const bool trimRepetitionRows = result.hideRepetitionRows && holdsAggregate;
  for (const auto& row : result.rows) {
    if (trimRepetitionRows && row.kind == RowKind::REPETITION) {
      continue;
    }
    // FR-022, FR-023: the rows of a complexity fit print as a family's own
    // report, after its aggregate row; the fit result carries nothing else.
    if (row.kind == RowKind::BIG_O || row.kind == RowKind::RMS
        || row.kind == RowKind::SKIPPED)
    {
      printFitRow(result, row, scale);
      continue;
    }
    if (row.kind == RowKind::AGGREGATE) {
      std::printf("%-24s %10s  primary time/iter (%s) mean=%9.3f "
                  "median=%9.3f sd=%9.3f cv=%8.4f min=%9.3f max=%9.3f "
                  "n=%llu",
                  result.name.c_str(),
                  "AGGREGATE",
                  scale.label,
                  row.timeStats.mean / scale.divisor,
                  row.timeStats.median / scale.divisor,
                  row.timeStats.sampleStdDev / scale.divisor,
                  row.timeStats.coefficientOfVariation,
                  row.timeStats.min / scale.divisor,
                  row.timeStats.max / scale.divisor,
                  static_cast<unsigned long long>(row.timeStats.samples));
      // FR-013, FR-014: the statistics the family attached print among the
      // built-in fields in the order of the calls, a time value in the unit
      // of the row and a percentage unscaled by it. They stand before the
      // trailing label, so the label stays the last field of the row.
      for (const auto& statistic : result.statisticValues) {
        const double printed = statistic.unit == sg::StatUnit::TIME
            ? statistic.value / scale.divisor
            : statistic.value;
        std::printf("  %s=%9.3f", statistic.name.c_str(), printed);
      }
      if (!row.label.empty()) {
        // FR-012: the shared label prints in the same trailing field.
        std::printf("  label=%s", row.label.c_str());
      }
      std::printf("\n");
      for (std::size_t column = 0; column < row.metricStats.size(); ++column) {
        const auto& stats = row.metricStats[column];
        const char* label = result.metricLabels[column].c_str();
        std::printf("%-24s %10s  %s mean=%9.3f median=%9.3f sd=%9.3f "
                    "cv=%8.4f min=%9.3f max=%9.3f n=%llu\n",
                    result.name.c_str(),
                    "",
                    label,
                    stats.mean,
                    stats.median,
                    stats.sampleStdDev,
                    stats.coefficientOfVariation,
                    stats.min,
                    stats.max,
                    static_cast<unsigned long long>(stats.samples));
      }
      // FR-006: one statistic line per named counter, in the order the names
      // opened, over the clean repetitions.
      for (std::size_t column = 0; column < result.counterStats.size();
           ++column)
      {
        const auto& stats = result.counterStats[column];
        const char* label = result.counterLabels[column].c_str();
        std::printf("%-24s %10s  %s mean=%9.3f median=%9.3f sd=%9.3f "
                    "cv=%8.4f min=%9.3f max=%9.3f n=%llu\n",
                    result.name.c_str(),
                    "",
                    label,
                    stats.mean,
                    stats.median,
                    stats.sampleStdDev,
                    stats.coefficientOfVariation,
                    stats.min,
                    stats.max,
                    static_cast<unsigned long long>(stats.samples));
      }
      continue;
    }

    std::printf("%-24s iterations=%10llu  real time/iter (%s)=%9.3f  "
                "cpu time/iter (%s)=%9.3f  overhead floor (%s)=%9.3f  "
                "endpoint samples=%llu",
                result.name.c_str(),
                static_cast<unsigned long long>(row.iterations),
                scale.label,
                row.timePerIterationNs / scale.divisor,
                scale.label,
                row.cpuTimePerIterationNs / scale.divisor,
                scale.label,
                row.overheadFloorNs / scale.divisor,
                static_cast<unsigned long long>(row.endpointSamples));
    for (std::size_t column = 0; column < row.metrics.size(); ++column) {
      const auto& metric = row.metrics[column];
      const char* label = result.metricLabels[column].c_str();
      std::printf(
          "  %s=%9.3f ratio=%.3f scaled=%d gap=%d",
          label,
          metric.value,
          metric.runningRatio,
          metric.scaled ? 1 : 0,
          metric.availability == sg::counters::Availability::GAP ? 1 : 0);
      if (metric.availability != sg::counters::Availability::COUNTABLE
          && metric.availability != sg::counters::Availability::GAP)
      {
        std::printf(" refusal=%s", availabilityName(metric.availability));
      }
    }
    // FR-006: one labeled column per user counter after the metric fields,
    // in the order the names opened, each carrying the flag-applied value
    // divided by its base. FR-005: a row whose counters could not take their
    // flags prints the disclosure beside every one of its counter columns.
    for (std::size_t column = 0; column < row.counters.size(); ++column) {
      std::printf("  %s=%9.3f",
                  result.counterLabels[column].c_str(),
                  row.counters[column]);
      if (!row.counterDisclosure.empty()) {
        std::printf(" %s", row.counterDisclosure.c_str());
      }
    }
    if (row.pauseSamplesFolded) {
      // FR-054: a folded run discloses the fold beside the row; a raw
      // row keeps the exact bytes it prints today.
      std::printf("  pause=folded");
    }
    if (!row.label.empty()) {
      // FR-011: the label is the trailing field of the row, separated by
      // two spaces, so an unlabelled row keeps its bytes whole.
      std::printf("  label=%s", row.label.c_str());
    }
    std::printf("\n");
    printRowPercentiles(result, row, scale);
  }

  // FR-025: a non-empty `contexts` vector names exactly a threaded
  // result, so a single-context result leaves here holding the bytes its
  // rows printed, with no line below them.
  if (result.contexts.empty()) {
    return;
  }
  printContextDisclosure(result);
  // specs/021 FR-014: under the disclosure that names the contexts, each
  // context answers from its own chunk values.
  for (std::size_t context = 0; context < result.contextPercentiles.size();
       ++context)
  {
    const auto& folds = result.contextPercentiles[context];
    for (std::size_t column = 0; column < folds.size(); ++column) {
      const std::string tag = "ctx" + std::to_string(context);
      const auto contextScale = unitScale(result.timeUnit);
      printPercentileLine(
          result.name,
          "percentile",
          tag + " " + percentileColumnLabel(result, column, contextScale.label),
          result.percentilePoints,
          folds[column],
          percentileColumnDivisor(column, contextScale.divisor));
    }
  }
  // FR-026: the warning of a result stands after that result's own
  // disclosure line, one line for each result whose condition holds.
  printNoiseWarning(result);
}

}  // namespace sg::detail
