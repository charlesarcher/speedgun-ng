#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <sched.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/execution_context.hpp"
#include "speedgun-ng/hunt.hpp"

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
// prints those two fields. specs/039-context-groups FR-028: the group the
// run's partition named for that context closes its field, behind the flag
// where that flag prints. The line formats the records of
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
    } else {
      std::printf("  ctx%zu=%s/%s/%s",
                  index,
                  waitModeName(context.waitMode),
                  field.c_str(),
                  context.applied ? kAppliedFlag : kUnappliedFlag);
    }
    // FR-028: the group stands last among the segments of one context,
    // behind the flag where that flag prints. A run that carries no
    // partition names no group, and its line keeps the bytes it printed
    // before the segment existed (FR-033).
    if (!context.group.empty()) {
      std::printf("/%s", context.group.c_str());
    }
  }
  std::printf("\n");
}

// specs/037-execution-graphs FR-022, FR-023, D-6, R-10: the graph block of
// one threaded run, the lines that stand directly after that run's
// `contexts=` line. The nodes line names the run's nodes in the ascending
// slot order FR-023 fixes and its edges as `from->to` pairs in the graph
// file's order. The held line names each held node with the failed node
// that gates it in parentheses, and the stopped line names each stopped
// node, each list in ascending slot order and on one line. The two lines
// stand only for a run that holds or stops a node. The entry carries its
// arrays in these orders, so the block formats them and adds nothing, the
// way the disclosure line formats `result.contexts` (FR-025).
auto printGraphReport(const sg::GraphRunRecord& entry) -> void
{
  auto nodes = std::string {};
  for (const auto& node : entry.nodes) {
    nodes += (nodes.empty() ? "" : ",") + node.name;
  }
  auto edges = std::string {};
  for (const auto& edge : entry.edges) {
    edges += (edges.empty() ? "" : ",") + edge.from + "->" + edge.to;
  }
  std::printf("  graph nodes=%s edges=%s\n", nodes.c_str(), edges.c_str());
  if (!entry.held.empty()) {
    auto held = std::string {};
    for (const auto& gate : entry.held) {
      held += (held.empty() ? "" : ",") + gate.node + "(" + gate.reason + ")";
    }
    std::printf("  graph held=%s\n", held.c_str());
  }
  if (!entry.stopped.empty()) {
    auto stopped = std::string {};
    for (const auto& name : entry.stopped) {
      stopped += (stopped.empty() ? "" : ",") + name;
    }
    std::printf("  graph stopped=%s\n", stopped.c_str());
  }
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

// The tokens the pass disclosure prints for the collection methods, indexed
// by the enumerator, the table shape the availability names and the wait
// mode names take (E-01, FR-017).
constexpr std::array<const char*, 3> kMetricMethodNames {
    "auto",
    "passes",
    "multiplex",
};
static_assert(kMetricMethodNames.size()
              == std::to_underlying(sg::MetricMethod::MULTIPLEX) + 1);

auto metricMethodName(const sg::MetricMethod method) -> const char*
{
  // The static_assert above ties the table to the last enumerator, so the
  // index of a value of that closed enumeration sits inside the table, and
  // the guard answers every other value (FR-017).
  const auto index = std::to_underlying(method);
  if (index >= kMetricMethodNames.size()) {
    return "unknown";
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kMetricMethodNames[index];
}

// The spread limit the pass warning reads stands in
// `detail/json_report.hpp`, where the document's `spreadWarning` key reads
// the same figure (FR-018, E-06). The warning line names the figure in its
// own words, printed from here.

// specs/024 FR-017, FR-018, FR-020, FR-021: the pass disclosure of one
// result. One line names the method the run settled to and the passes it
// measured, in the tagged shape of the chunks and percentile lines, and a
// run that measured more than one pass adds the spread of their reference
// times - the print being the one rounding of the published figure, the
// words `not available` standing where the figure is not computable, as the
// percentile field does. A spread above the limit adds the warning line in
// the voice of the noise warning, which refuses nothing and changes nothing
// (FR-018). A run with no metric column has no method to name, and a run
// that settled to `passes` on its own and measured one pass measured the
// whole set in that pass, so neither prints a line: their report is the
// audit point's bytes (FR-021, SC-03).
//
// The third case the rule names, a method the caller forced, is caller
// state: research D-10 settles it on the command line's word and the
// family's own request, the runner records that answer on the result, and
// the print below reads the record.
auto printPassDisclosure(const BenchmarkResult& result) -> void
{
  if (result.metricLabels.empty()
      || result.metricMethod == sg::MetricMethod::AUTO)
  {
    return;
  }
  const bool multiplex = result.metricMethod == sg::MetricMethod::MULTIPLEX;
  if (!multiplex && result.passCount <= 1 && !result.metricMethodForced) {
    return;
  }
  std::printf("%-24s %10s  method=%s  count=%zu",
              result.name.c_str(),
              "passes",
              metricMethodName(result.metricMethod),
              result.passCount);
  // FR-017: one pass has no spread across which to speak, so the field
  // stands only where there is more than one pass to compare.
  if (result.passCount > 1) {
    if (result.passSpread.has_value()) {
      std::printf("  spread=%.1f%%", *result.passSpread);
    } else {
      std::printf("  spread=not available");
    }
  }
  std::printf("\n");
  if (spreadWarned(result.passSpread)) {
    std::printf("warning: pass spread %.1f%% exceeds %.0f%%\n",
                *result.passSpread,
                kSpreadWarningLimitPercent);
  }
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

// specs/029 T027, FR-040, PD-15: the hunt printers stand below this file's
// `printResult`, and the hunt block of a result calls them from here. The plan
// printer is declared in `detail/internal.hpp`, where the test lane reads it.
auto printHuntRanking(const sg::hunt::EntryHunt& hunt,
                      const sg::hunt::HuntNorm norm) -> void;
auto printHuntSkips(const sg::hunt::EntryHunt& hunt,
                    const sg::hunt::RunHunt& run) -> void;

auto printResult(const BenchmarkResult& result,
                 const std::optional<sg::hunt::RunHunt>& run,
                 const std::optional<sg::GraphRunRecord>& graph) -> void
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

  // specs/029 T074, T126, FR-012, FR-013, US2 scenario 1: the plan a hunt run
  // measured under names itself above the first row it bought, with the
  // budget cut line directly under it, so the budget the reader is being
  // scored against stands before the figures it explains. The record is the
  // one the hunt that built `result.hunt` answered (PD-10). A result that
  // carried no hunt reaches here false and prints the bytes it printed
  // before the hunt existed (I-8).
  const bool printsHuntPlan = result.hunt.has_value() && run.has_value();
  if (printsHuntPlan) {
    printHuntPlan(*run);
  }

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
      // specs/024 FR-019, FR-021: the tag names the pass that measured this
      // value and stands inside the same field group. A run that measured
      // one pass has no split to name, so its row keeps the four tokens the
      // audit point prints; a value no split measured carries no index.
      if (result.passCount > 1 && metric.pass.has_value()) {
        std::printf(" pass=%zu", *metric.pass);
      }
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

  // specs/024 FR-017, FR-022: the pass disclosure stands under the rows it
  // describes, and the context disclosure below it prints what it printed
  // before the pass mechanism existed (spec D-8, N4).
  printPassDisclosure(result);
  // FR-024, R-08: the per-benchmark attribution line under the rows,
  // naming the worst probe cpu for this benchmark's windows: the fold
  // whose max stands largest, ties keeping the ascending cpu order the
  // carrier fills. A benchmark whose windows placed no kept gap prints
  // no line (E-06, FR-021).
  if (!result.jitterFolds.empty()) {
    const sg::JitterEntryFold* worst = &result.jitterFolds.front();
    for (const auto& fold : result.jitterFolds) {
      if (fold.stats.maxNs > worst->stats.maxNs) {
        worst = &fold;
      }
    }
    std::printf("jitter: worst cpu %u gaps=%llu max=%llu total=%.2f\n",
                static_cast<unsigned>(worst->cpu),
                static_cast<unsigned long long>(worst->stats.gapCount),
                static_cast<unsigned long long>(worst->stats.maxNs),
                worst->stats.totalPercent);
  }

  // specs/029 T027, T074, T126, FR-040, FR-041, PD-15, data-model E-07: the
  // hunt lines that rank print behind the rows and disclosures of the
  // benchmark they ranked, in the order ranking, skipped list, summary; the
  // plan line and the cut line under it already stood above the first row. A
  // result that carried no hunt prints the bytes it printed before the hunt
  // existed (I-8), and the norm label is the one the run measured under,
  // `instr` when the caller named none (D-13).
  if (printsHuntPlan) {
    printHuntRanking(*result.hunt,
                     run->options.norm.value_or(sg::hunt::HuntNorm::INSTR));
    printHuntSkips(*result.hunt, *run);
  }

  // FR-025: a non-empty `contexts` vector names exactly a threaded
  // result, so a single-context result leaves here holding the bytes its
  // rows printed, with no line below them.
  if (result.contexts.empty()) {
    return;
  }
  printContextDisclosure(result);
  // specs/037 FR-022, FR-023, D-6: the graph block of this run stands
  // directly after the `contexts=` line the disclosure just printed, one
  // block per threaded run. A run that names no record entry prints no
  // block, the bytes it printed before the record existed (FR-006, SC-001).
  if (graph.has_value()) {
    printGraphReport(*graph);
  }
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

// T012, FR-023, R-08: the console probe block. One line per stats field
// in the E-03 order, one value per probe cpu; the disclosure lines
// follow in the end-of-report region the noise warning owns (FR-036,
// D-8, D-12), the block extending and replacing nothing the probe-free
// report prints (PC-6, FR-013).
auto printJitterReport(const bool available,
                       const std::vector<const ProbeCpuState*>& states) -> void
{
  if (!available) {
    std::printf("jitter: not available\n");
    return;
  }
  if (states.empty()) {
    return;
  }

  std::string cpuList;
  for (const auto* state : states) {
    if (!cpuList.empty()) {
      cpuList.push_back(',');
    }
    cpuList += std::to_string(state->cpu);
  }
  std::printf("jitter probe: cpus %s\n", cpuList.c_str());

  // The per-cpu folds, taken once: the block reads them and so does the
  // disclosure (E-03, E-09).
  std::vector<JitterStats> folds;
  folds.reserve(states.size());
  for (const auto* state : states) {
    folds.push_back(gapStatsOf(*state));
  }

  // FR-002: the sixteen fields in the E-03 order.
  const auto printUintField = [&folds](const char* name, auto get)
  {
    std::printf("%s:", name);
    for (const auto& fold : folds) {
      std::printf(" %llu", static_cast<unsigned long long>(get(fold)));
    }
    std::printf("\n");
  };
  const auto printRealField = [&folds](const char* name, auto get)
  {
    std::printf("%s:", name);
    for (const auto& fold : folds) {
      std::printf(" %.2f", get(fold));
    }
    std::printf("\n");
  };
  printUintField("gapCount",
                 [](const JitterStats& fold) { return fold.gapCount; });
  printRealField("gapsPerSecond",
                 [](const JitterStats& fold) { return fold.gapsPerSecond; });
  printUintField("minNs", [](const JitterStats& fold) { return fold.minNs; });
  printUintField("medianNs",
                 [](const JitterStats& fold) { return fold.medianNs; });
  printRealField("meanNs", [](const JitterStats& fold) { return fold.meanNs; });
  printUintField("p90Ns", [](const JitterStats& fold) { return fold.p90Ns; });
  printUintField("p99Ns", [](const JitterStats& fold) { return fold.p99Ns; });
  printUintField("p999Ns", [](const JitterStats& fold) { return fold.p999Ns; });
  printUintField("p9999Ns",
                 [](const JitterStats& fold) { return fold.p9999Ns; });
  printUintField("maxNs", [](const JitterStats& fold) { return fold.maxNs; });
  printUintField("totalNs",
                 [](const JitterStats& fold) { return fold.totalNs; });
  printRealField("totalPercent",
                 [](const JitterStats& fold) { return fold.totalPercent; });
  printUintField("runtimeNs",
                 [](const JitterStats& fold) { return fold.runtimeNs; });
  printUintField("thresholdNs",
                 [](const JitterStats& fold) { return fold.thresholdNs; });
  printUintField("loopPeriodNs",
                 [](const JitterStats& fold) { return fold.loopPeriodNs; });
  printUintField("droppedGaps",
                 [](const JitterStats& fold) { return fold.droppedGaps; });

  // FR-036, E-09: the disclosure lines: the availability and the cpus,
  // the measured points, the per-cpu counts, and the kept-gaps-only
  // rule; the JSON disclosure text carries the same content (FR-025,
  // D-8, D-12). The disclosure reads no clock (R-08).
  std::printf("jitter: probe on cpus %s\n", cpuList.c_str());
  std::printf("jitter: loop period %llu ns, threshold %llu ns\n",
              static_cast<unsigned long long>(folds.front().loopPeriodNs),
              static_cast<unsigned long long>(folds.front().thresholdNs));
  for (const auto* state : states) {
    std::printf("jitter: cpu %u kept %llu dropped %llu\n",
                static_cast<unsigned>(state->cpu),
                static_cast<unsigned long long>(state->kept),
                static_cast<unsigned long long>(state->droppedGaps));
  }
  std::printf("jitter: split and percentiles use the kept gaps only\n");
}

namespace
{

// The token that stands in a hunt line where the run holds no figure, the one
// the pass spread and the percentile fields print for the same case.
constexpr const char* kUnavailableFigure = "not available";

// specs/029 E-02, FR-040: the label of the column a hunt rate prints under,
// indexed by the enumerator, the table shape the availability names and the
// pass method names take. The JSON writer keeps its own table beside its own
// keys (E-02); the console reads this one.
constexpr std::array<const char*, 3> kHuntNormNames {
    "instr",
    "cycle",
    "iter",
};
static_assert(kHuntNormNames.size()
              == std::to_underlying(sg::hunt::HuntNorm::ITER) + 1);

auto huntNormName(const sg::hunt::HuntNorm norm) -> const char*
{
  // The static_assert above ties the table to the last enumerator of a closed
  // enumeration, so the index of a value of that enumeration sits inside the
  // table.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kHuntNormNames[std::to_underlying(norm)];
}

// specs/029 E-06, D-10: the refusal kinds of a skipped entry are the five
// `Availability` values such an entry carries, so the skipped line prints the
// words `availabilityName` prints - the same words the `--catalog` listing
// prints - and no second table of these words enters the tree (FR-021,
// FR-040). The table pairs the two enumerators by name, never by an
// offset, and the assert ties the last pair.
constexpr std::array<sg::counters::Availability, 5> kSkippedKindAvailability {
    sg::counters::Availability::PERMISSION_BLOCKED,
    sg::counters::Availability::NOT_ENCODABLE,
    sg::counters::Availability::ABSENT,
    sg::counters::Availability::SCOPE_REFUSED,
    sg::counters::Availability::GAP,
};
static_assert(
    kSkippedKindAvailability[std::to_underlying(sg::hunt::SkippedKind::GAP)]
    == sg::counters::Availability::GAP);

auto skippedKindName(const sg::hunt::SkippedKind kind) -> const char*
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return availabilityName(kSkippedKindAvailability[std::to_underlying(kind)]);
}

// specs/029 FR-012, E-03: the budget field of the two plan lines. The figure
// stands in the unit the caller named, so `120s` and `8p` print as they were
// given, and a run with no budget prints the word `none`.
auto printHuntBudget(const std::optional<sg::hunt::HuntBudget>& budget) -> void
{
  if (!budget.has_value()) {
    std::printf("none");
    return;
  }
  std::printf("%llu%c",
              static_cast<unsigned long long>(budget->amount),
              budget->kind == sg::hunt::HuntBudget::Kind::SECONDS ? 's' : 'p');
}

// specs/033 FR-001, FR-002, FR-023, R-10, N-03, N-06: the one sample-budget
// disclosure line. The stage split recorded the state the budget settled, so
// the line reads that state and answers one of its three words: the derived
// count under the word `over-budget` where the budget fitted below one repeat
// of a stage, the word `unpriced` for a stage no calibration figure priced, and
// the derived count weighed against the stage rule that capped it. The overrun
// is the state of a stage of the plan and the count is the figure of the stage
// the run prices, so the line prints the count the record carries beside the
// word, the figure the document writes under `derived_repeats`. The split
// records an overrun inside the derivation that answers the count, so no
// record holds an overrun without that count. A derived count the rule already
// gave moved nothing, so the record leaves no line.
auto printHuntSampleLine(const sg::hunt::RunHunt& run) -> void
{
  if (run.sample.overBudget) {
    SG_REQUIRE(run.sample.derivedRepeats.has_value(),
               "the split records an overrun inside the derivation that "
               "answers the count, so the record carries it");
    std::printf("hunt: plan sample repeats=%zu over-budget\n",
                *run.sample.derivedRepeats);
    return;
  }
  if (run.sample.unpriced) {
    std::printf("hunt: plan sample unpriced\n");
    return;
  }
  if (run.sample.derivedRepeats.has_value()
      && *run.sample.derivedRepeats != run.options.repeats)
  {
    std::printf("hunt: plan sample repeats=%zu of %zu\n",
                *run.sample.derivedRepeats,
                run.options.repeats);
  }
}

// specs/033 FR-003, FR-023, R-10, N-04, N-06: the one multiplex-cap disclosure
// line, the groups the cap split out of the plan in plan order, joined by
// commas. A `PASSES` settlement holds no group and prints no line; a cap that
// covers the whole plan leaves the one group of it.
auto printHuntMuxLine(const sg::hunt::RunHunt& run) -> void
{
  if (run.muxGroups.empty()) {
    return;
  }
  std::printf("hunt: plan mux groups=%zu sizes=", run.muxGroups.size());
  for (std::size_t index = 0; index < run.muxGroups.size(); ++index) {
    std::printf("%s%zu", index == 0 ? "" : ",", run.muxGroups[index]);
  }
  std::printf("\n");
}

/// specs/040 FR-012: the three sensitivity cells of one ranked row, printed
/// after the CV cell and before the trailing mark and the multiplexed flag.
/// Each cell names its figure at a fixed precision, and a figure the run
/// did not measure stands `n/a` in its cell. The correlation and the slope
/// are unitless and share one precision; the delta is a percent, so it
/// reads the precision the CV percent cell reads.
///
/// \pre none
/// \post prints exactly one ` correlation=`, one ` slope=` and one
///       ` delta=` cell; each cell carries the field's figure at its
///       precision, and the word `n/a` where the field stands empty; the
///       call measures nothing and reads no clock
auto printSensitivityCells(const sg::hunt::RankedEvent& entry) -> void
{
  if (entry.sensitivityCorrelation.has_value()) {
    std::printf(" correlation=%.4f", *entry.sensitivityCorrelation);
  } else {
    std::printf(" correlation=n/a");
  }
  if (entry.sensitivitySlope.has_value()) {
    std::printf(" slope=%.4f", *entry.sensitivitySlope);
  } else {
    std::printf(" slope=n/a");
  }
  if (entry.delta.has_value()) {
    std::printf(" delta=%.2f", *entry.delta);
  } else {
    std::printf(" delta=n/a");
  }
}

// specs/029 D-7, D-8, PD-14: the word that names why an event stands out of
// the top N. One sample carries no deviation, so an event that repeated and
// stayed under `--hunt-noise` left the table on the size of the table alone.
auto excludedMarkOf(const sg::hunt::RankedEvent& entry) -> const char*
{
  if (entry.noisy) {
    return "noisy";
  }
  if (!entry.repeated) {
    return "unrepeated";
  }
  return "beyond-top";
}

}  // namespace

// The words the hunt plan line prints for the settled collection method:
// the data model spells the figure in capital letters, a shape of its own
// beside the lowercase tokens the pass disclosure prints (E-07).
constexpr std::array<const char*, 3> kHuntMethodWords {
    "AUTO",
    "PASSES",
    "MULTIPLEX",
};
static_assert(kHuntMethodWords.size()
              == std::to_underlying(sg::MetricMethod::MULTIPLEX) + 1);

// specs/029 T027, T035, T119, FR-012, FR-013, FR-040, FR-041, PD-15, data-model
// E-07: the plan line of one hunt run, spelled from the run record the report
// lane reads, and the one line a cut plan adds below it. `events=`
// counts the planned columns and `passes=` the passes of the plan, `method=`
// names the settled collection method in the capital word the data model
// spells, and the estimate is the plan's own figure: the pass count weighed
// against the nanoseconds of one pass the hunt entry timed, so a plan whose
// hunt timed no pass prints `estimate=not available` (FR-012). A plan of two
// stages names both stages with the events each measures, which is where a
// seconds budget shows the sweep and the re-run it bought (US2 scenario 2).
// The record carries the
// norm the run measured under, so a device that lacked an anchors the run
// normalized by iterations, which the line states openly (D-13). The line
// carries no measurement: every figure is a field of the record the caller
// handed over.
// specs/033-hunter-budgets FR-020, FR-023, R-10, R-11, N-06: the canonical
// `e` clause text. The plan line and the document both spell the sample budget
// from the stored nanoseconds, so the shortest decimal word of the seconds is
// answered here once: a whole second carries no fraction, and a fraction keeps
// no trailing zero (`0.50e` reads back as `0.5e`). The letters divide the count
// exactly once, so no floating-point rounding enters the text.
auto huntSampleClauseText(std::uint64_t budgetNs) -> std::string
{
  constexpr std::uint64_t kNsPerSecond = 1'000'000'000ULL;
  const std::uint64_t whole = budgetNs / kNsPerSecond;
  const std::uint64_t fraction = budgetNs % kNsPerSecond;
  if (fraction == 0) {
    return std::to_string(whole) + "e";
  }
  auto digits = std::to_string(fraction + kNsPerSecond).substr(1);
  while (digits.back() == '0') {
    digits.pop_back();
  }
  return std::to_string(whole) + "." + digits + "e";
}

auto huntCapClauseText(std::size_t cap) -> std::string
{
  return std::to_string(cap) + "g";
}

auto printHuntPlan(const sg::hunt::RunHunt& run) -> void
{
  std::printf("hunt: plan events=%zu passes=%zu method=%s",
              run.plannedAddresses.size(),
              run.passes,
              run.method.has_value()
                  ? kHuntMethodWords[std::to_underlying(*run.method)]
                  : kUnavailableFigure);
  if (run.estimateNs.has_value()) {
    std::printf(" estimate=%lluns",
                static_cast<unsigned long long>(*run.estimateNs));
  } else {
    std::printf(" estimate=%s", kUnavailableFigure);
  }
  std::printf(" budget=");
  printHuntBudget(run.options.budget);
  // specs/033 FR-004, FR-020, FR-023, R-10, N-06: the two budgets of 033 join
  // the line in the canonical clause texts the document writes, each only while
  // its budget stands.
  if (run.options.sampleBudgetNs.has_value()) {
    std::printf(" sample=%s",
                huntSampleClauseText(*run.options.sampleBudgetNs).c_str());
  }
  if (run.options.multiplexCap.has_value()) {
    std::printf(" mux=%s",
                huntCapClauseText(*run.options.multiplexCap).c_str());
  }
  // specs/029 T035, FR-013: the stages the run measured, named in order with
  // the event count each carries. A record that holds one stage - the
  // un-budgeted plan, and the budgeted plan whose stage 2 was cut - names
  // none: no split stands there to be read.
  if (run.stages.size() > 1) {
    for (std::size_t index = 0; index < run.stages.size(); ++index) {
      std::printf(" stage%zu=%zu", index + 1, run.stages[index].count);
    }
  }
  if (run.options.norm == sg::hunt::HuntNorm::ITER) {
    std::printf(" normalized=iter");
  }
  std::printf("\n");
  // specs/029 D-2, FR-013, E-07: the cut line. `splitStages` drops stage 2
  // whole when the two-stage estimate does not fit the budget, leaving a
  // budgeted record with one stage. The reason of the drop is the record's own
  // field, the value the planner recorded, so this file spells no reason and
  // reads no shape into it. A record that carries both stages prints no line.
  if (run.options.budget.has_value() && run.stages.size() == 1) {
    std::printf("hunt: plan budget cut stage=2 reason=%s\n",
                run.cutReason.c_str());
  }
  // specs/033 FR-001 to FR-003, FR-023, N-06: the two disclosure lines stand
  // under the lines that spell the budgets. A run that stands under no sample
  // budget prints no sample line, and a plan that holds no group prints no
  // groups line.
  printHuntSampleLine(run);
  printHuntMuxLine(run);
}

// specs/029 T027, FR-040, FR-042, PD-14, PD-15, data-model E-07: the ranked
// table of one hunt, the first lines the caller prints after the benchmark
// tables. `entries` holds the ranked events first and the excluded ones after
// them in address order (I-5), so one walk of the vector prints the top N and
// then the marked tail at `rank=-`. `cv=` prints the entry's coefficient of
// variation in the percent the noise threshold compares against (I-6), and a
// plan that settled the multiplexing method discloses it beside the row
// (D-19). Nothing here decides a rank, a mark or the size of the table: the
// record the caller hands over already holds all three.
auto printHuntRanking(const sg::hunt::EntryHunt& hunt,
                      const sg::hunt::HuntNorm norm) -> void
{
  for (const auto& entry : hunt.entries) {
    if (entry.rank.has_value()) {
      std::printf("hunt: rank %zu %s %s=%.3f cv=%.2f",
                  *entry.rank,
                  entry.address.c_str(),
                  huntNormName(norm),
                  entry.rate,
                  entry.cv);
      // specs/040 FR-012: the sensitivity cells stand after the CV cell
      // and before the flag, the column order the document pins (E-07).
      printSensitivityCells(entry);
      if (entry.multiplexed) {
        std::printf(" multiplexed");
      }
      std::printf("\n");
      continue;
    }
    std::printf("hunt: rank - %s %s=%.3f cv=%.2f",
                entry.address.c_str(),
                huntNormName(norm),
                entry.rate,
                entry.cv);
    // specs/040 FR-012: the beyond-top row carries the same cells the
    // ranked row carries, and the mark closes the line (FR-012).
    printSensitivityCells(entry);
    std::printf(" mark=%s\n", excludedMarkOf(entry));
  }
}

// specs/029 T027, FR-040, FR-041, FR-050, D-12, PD-15, data-model E-07: the
// skipped list of one hunt and the plan summary that closes the hunt lines.
// The caller prints both after the ranked table: one line per event the
// run will not measure in the shape `hunt: skipped <address> <kind>`,
// the kind in the words the catalog listing uses, then the one summary
// line. A host that refused every event prints no ranking row above
// these lines and the summary reads `events=0/N` (D-12, FR-041), the
// denominator being the universe of addresses the hunt stood before:
// the planned ones plus the refusals discovery kept out of the plan.
auto printHuntSkips(const sg::hunt::EntryHunt& hunt,
                    const sg::hunt::RunHunt& run) -> void
{
  for (const auto& skipped : hunt.skipped) {
    std::printf("hunt: skipped %s %s",
                skipped.address.c_str(),
                skippedKindName(skipped.kind));
    std::printf("\n");
  }
  std::size_t refused = 0;
  for (const auto& skipped : hunt.skipped) {
    if (std::find(run.plannedAddresses.begin(),
                  run.plannedAddresses.end(),
                  skipped.address)
        == run.plannedAddresses.end())
    {
      ++refused;
    }
  }
  std::printf("hunt: plan summary events=%zu/%zu passes=%zu",
              run.ranEvents,
              run.plannedAddresses.size() + refused,
              run.passes);
  std::printf(" budget=");
  printHuntBudget(run.options.budget);
  std::printf("\n");
}

// specs/034 FR-023, D-8: the pre-check block, printed before the
// benchmark tables: one block per measured cpu, the sixteen field
// names the console probe block uses, the attribution deltas beside
// the gap figures each naming its source, then the host settings
// once, then the warning lines and the pinning-candidate line
// (FR-012, FR-013, the clarify answers). The strict refusal never
// reaches this printer: the run stops at the error surface (FR-014).
auto printPrecheckReport(const PrecheckRecord& record,
                         sg::TracerEngine requestedEngine) -> void
{
  const auto figureText =
      [](const std::optional<std::uint64_t>& value) -> std::string
  {
    return value.has_value() ? std::to_string(*value)
                             : std::string(kUnavailableFigure);
  };
  for (const auto& entry : record.cpus) {
    std::printf("precheck cpu %u (%s)\n",
                static_cast<unsigned>(entry.cpu),
                entry.planned ? "planned" : "candidate");
    const auto& stats = entry.stats;
    std::printf("gapCount: %llu\n",
                static_cast<unsigned long long>(stats.gapCount));
    std::printf("gapsPerSecond: %.2f\n", stats.gapsPerSecond);
    std::printf("minNs: %llu\n", static_cast<unsigned long long>(stats.minNs));
    std::printf("medianNs: %llu\n",
                static_cast<unsigned long long>(stats.medianNs));
    std::printf("meanNs: %.2f\n", stats.meanNs);
    std::printf("p90Ns: %llu\n", static_cast<unsigned long long>(stats.p90Ns));
    std::printf("p99Ns: %llu\n", static_cast<unsigned long long>(stats.p99Ns));
    std::printf("p999Ns: %llu\n",
                static_cast<unsigned long long>(stats.p999Ns));
    std::printf("p9999Ns: %llu\n",
                static_cast<unsigned long long>(stats.p9999Ns));
    std::printf("maxNs: %llu\n", static_cast<unsigned long long>(stats.maxNs));
    std::printf("totalNs: %llu\n",
                static_cast<unsigned long long>(stats.totalNs));
    std::printf("totalPercent: %.2f\n", stats.totalPercent);
    std::printf("runtimeNs: %llu\n",
                static_cast<unsigned long long>(stats.runtimeNs));
    std::printf("thresholdNs: %llu\n",
                static_cast<unsigned long long>(stats.thresholdNs));
    std::printf("loopPeriodNs: %llu\n",
                static_cast<unsigned long long>(stats.loopPeriodNs));
    std::printf("droppedGaps: %llu\n",
                static_cast<unsigned long long>(stats.droppedGaps));
    const auto& attribution = entry.attribution;
    std::printf("interrupts delta: %s (source: /proc/interrupts)\n",
                figureText(attribution.interrupts).c_str());
    for (const auto& [kind, delta] : attribution.softirqs) {
      std::printf("softirq %s delta: %s (source: /proc/softirqs)\n",
                  kind.c_str(),
                  figureText(delta).c_str());
    }
    std::printf(
        "switch delta: %s, schedule delta: %s (source: /proc/schedstat)\n",
        figureText(attribution.switches).c_str(),
        figureText(attribution.schedules).c_str());
  }
  const auto& host = record.host;
  const auto settingText = [](const std::optional<std::string>& value)
  { return value.has_value() ? value->c_str() : kUnavailableFigure; };
  std::printf("host settings: isolcpus=%s nohz_full=%s rcu_nocbs=%s\n",
              settingText(host.isolcpus),
              settingText(host.nohzFull),
              settingText(host.rcuNocbs));
  for (const auto& [cpu, governor] : host.governors) {
    std::printf(
        "governor: cpu %u %s (source: /sys devices system cpu "
        "scaling_governor)\n",
        static_cast<unsigned>(cpu),
        settingText(governor));
  }
  for (const auto& [cpu, irqs] : host.irqAffinity) {
    std::string list;
    for (const auto irq : irqs) {
      if (!list.empty()) {
        list += ',';
      }
      list += std::to_string(irq);
    }
    std::printf("irq affinity: cpu %u %s (source: /proc/irq)\n",
                static_cast<unsigned>(cpu),
                list.empty() ? kUnavailableFigure : list.c_str());
  }
  // specs/036 FR-013, R-14: the tracer lines stand after the host
  // settings and before the warning lines: the header line naming the
  // engine word and the cpus, then one figure line per source per tracer
  // cpu in the NMI, IRQ, softirq, thread, hardware order, cpus ascending,
  // each absent figure standing the kUnavailableFigure wording (FR-010).
  // The record leaves the member empty for a run that requests no tracer
  // and for a run whose tracer stands not available alike (E-17), so the
  // requested engine carries the signal: a requested tracer prints the
  // one disclosure line (FR-016), a tracer-free run prints none (FR-012).
  if (record.tracer.has_value()) {
    const auto& tracer = *record.tracer;
    std::string cpuList;
    for (const auto& entry : tracer.cpus) {
      if (!cpuList.empty()) {
        cpuList += ',';
      }
      cpuList += std::to_string(entry.cpu);
    }
    std::printf(
        "tracer: engine %s cpus %s\n", tracer.engine.c_str(), cpuList.c_str());
    for (const auto& entry : tracer.cpus) {
      std::printf("tracer cpu %u nmi: %s\n",
                  static_cast<unsigned>(entry.cpu),
                  figureText(entry.nmi).c_str());
      std::printf("tracer cpu %u irq: %s\n",
                  static_cast<unsigned>(entry.cpu),
                  figureText(entry.irq).c_str());
      std::printf("tracer cpu %u softirq: %s\n",
                  static_cast<unsigned>(entry.cpu),
                  figureText(entry.softirq).c_str());
      std::printf("tracer cpu %u thread: %s\n",
                  static_cast<unsigned>(entry.cpu),
                  figureText(entry.thread).c_str());
      std::printf("tracer cpu %u hardware: %s\n",
                  static_cast<unsigned>(entry.cpu),
                  figureText(entry.hardware).c_str());
    }
  } else if (requestedEngine != sg::TracerEngine::NONE) {
    std::printf("tracer: not available\n");
  }
  bool candidatesStand = false;
  for (const auto& entry : record.cpus) {
    if (!entry.planned) {
      candidatesStand = true;
    }
  }
  if (!candidatesStand) {
    std::printf("precheck candidates: none\n");
  }
  std::vector<const PrecheckCpuRecord*> noisy;
  for (const auto& entry : record.cpus) {
    if (entry.planned && entry.stats.gapCount > 0) {
      noisy.push_back(&entry);
    }
  }
  for (const auto* entry : noisy) {
    std::printf("warning: cpu %u gapsPerSecond %.2f maxNs %llu\n",
                static_cast<unsigned>(entry->cpu),
                entry->stats.gapsPerSecond,
                static_cast<unsigned long long>(entry->stats.maxNs));
  }
  std::vector<std::pair<double, std::uint32_t>> ranked;
  for (const auto& entry : record.cpus) {
    if (entry.planned) {
      ranked.emplace_back(entry.stats.gapsPerSecond, entry.cpu);
    }
  }
  std::ranges::sort(ranked);
  std::printf("pinning candidates:");
  for (const auto& [rate, cpu] : ranked) {
    std::printf(" cpu %u %.2f", static_cast<unsigned>(cpu), rate);
  }
  std::printf("\n");
}

}  // namespace sg::detail
