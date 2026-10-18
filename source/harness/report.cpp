#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <sched.h>

#include "detail/internal.hpp"
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

// The placement field a context carries when it pins to no cpu. The
// disclosure line prints this word where a pinned context prints its cpu
// index (FR-005, FR-025).
constexpr std::string_view kNoPinPlacement = "NO_PIN";

// The placement field of a slot the placement function did not land. A
// spinning context carrying it stands on no cpu, so it joins neither the
// pinned set nor the shared-cpu pair of the noise condition (FR-026).
constexpr std::string_view kUnappliedPlacement = "UNAPPLIED";

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
// placement that context ran under. The line formats the records of
// `result.contexts` and adds nothing to them.
auto printContextDisclosure(const BenchmarkResult& result) -> void
{
  std::printf("  contexts=%zu", result.contexts.size());
  for (std::size_t index = 0; index < result.contexts.size(); ++index) {
    const auto& context = result.contexts[index];
    std::printf("  ctx%zu=%s/%s",
                index,
                waitModeName(context.waitMode),
                context.placement.c_str());
  }
  std::printf("\n");
}

// The count of cpus the report compares the spinning contexts against:
// the affinity mask of the calling thread, which a cpuset, a taskset or a
// container narrows, and the host count of the standard library where that
// read answers nothing (FR-026). The two products keep the choice in
// arithmetic: the mask read of a stack buffer names no failure a test run
// can produce, and a branch on it would stand uncovered at the gate.
auto runnableCpuCount() -> std::uint64_t
{
  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  const auto answered = static_cast<std::uint64_t>(
      sched_getaffinity(0, sizeof(cpus), &cpus) == 0);
  const auto masked = answered * static_cast<std::uint64_t>(CPU_COUNT(&cpus));
  return masked
      + (static_cast<std::uint64_t>(1) - answered)
      * static_cast<std::uint64_t>(std::thread::hardware_concurrency());
}

// FR-026, FR-028: the noise warning of one result. It prints when two
// spinning contexts of that result name one cpu index, or when the
// spinning contexts of that result outnumber the cpus this process may
// run on, and prints nothing at all when neither holds.
//
// The runnable cpu count is the one host fact the check reads, and the
// check reads it once, where its decision needs it. The mask
// `sched_getaffinity` reports is that fact: a process confined to part
// of the machine has exactly that much room to spin in, and the online
// count told it nothing. The hardware concurrency count of the standard
// library stands in when the call fails. The covering test reads the
// same source, so the printed number and the trigger agree (D-08). The
// read names no clock and samples nothing (FR-027, FR-039).
auto printNoiseWarning(const BenchmarkResult& result) -> void
{
  auto spinning = std::uint64_t {0};
  auto pinned = std::vector<std::string_view> {};
  for (const auto& context : result.contexts) {
    if (context.waitMode != sg::WaitMode::SPIN) {
      continue;
    }
    ++spinning;
    if (context.placement != kNoPinPlacement
        && context.placement != kUnappliedPlacement)
    {
      pinned.push_back(context.placement);
    }
  }
  // Sorting stands equal placement fields side by side, so one adjacent
  // comparison answers whether two spinning contexts name one cpu index.
  // shortcut: one index spelled two ways, as with a leading zero, reads
  // as two indices; the default placement function renders digits with
  // `std::to_chars`, which spells an index one way.
  std::ranges::sort(pinned);
  const bool sharesOneCpu = std::ranges::adjacent_find(pinned) != pinned.end();
  const auto runnableCpus = runnableCpuCount();
  if (sharesOneCpu || spinning > runnableCpus) {
    std::printf(
        "warning: spinning contexts oversubscribed (%llu spin, " "%llu cpus)\n",
        static_cast<unsigned long long>(spinning),
        static_cast<unsigned long long>(runnableCpus));
  }
}

}  // namespace

auto printContext() -> void
{
  // R-08: the catalog publishes no host or cpu model string at this
  // head, so those fields stay out of the line (FR-035).
  std::printf("speedgun-ng %s build=%s\n", SG_PROJECT_VERSION, SG_BUILD_TYPE);
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
  }

  // FR-025: a non-empty `contexts` vector names exactly a threaded
  // result, so a single-context result leaves here holding the bytes its
  // rows printed, with no line below them.
  if (result.contexts.empty()) {
    return;
  }
  printContextDisclosure(result);
  // FR-026: the warning of a result stands after that result's own
  // disclosure line, one line for each result whose condition holds.
  printNoiseWarning(result);
}

}  // namespace sg::detail
