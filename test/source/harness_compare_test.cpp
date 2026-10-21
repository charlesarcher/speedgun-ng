// ============================================================================
// T008: the comparison rule grammar of specs/026, over the nine rows of
// the fault table of specs/026-regression-gate/contracts/rule-parse.md
// (FR-004, FR-005, FR-012, FR-015, FR-016, FR-018, FR-019, FR-020,
// FR-021, FR-022, FR-023, FR-024, D-06, D-07, D-08, spec D-15, spec D-21).
//
// One case per fault row: the parse call answers an error, and that error
// text names the field that faulted and the text the call read. Two cases
// cover the accepted forms and answer the fields the text spelled. The
// field words the fault lines must carry are the contract's own:
// "threshold", "aggregate", "direction", "filter".
//
// T009 implements parseCompareRule. The grammar cases read no file; the
// fixture directory the registered command passes serves the
// fixture-driven cases T014 adds to this file. Frameworkless
// check()/fail() convention.
//
// T012: the value resolution and the row selection of
// specs/026-regression-gate/data-model.md E-08 over `ReportData` values
// built in code (FR-011, FR-013, FR-014, FR-015, FR-016, FR-017, FR-032,
// FR-036, FR-038, D-09, D-10, spec D-18, spec D-20). The two internal
// helpers these cases call stand in `source/harness/compare.cpp`; they
// are declared here because the public call that will use them lands at
// T018 and `detail/compare.hpp` carries only the mode-level calls.
//
// T014: the threshold arithmetic and the verdict words of E-09 over a
// matched pair of values: the delta as `c - b` for `LOWER_IS_BETTER` and
// `b - c` for `HIGHER_IS_BETTER`, the relative test over `|b|`, the
// absolute test in the stored unit, "With both thresholds the row
// regresses when both are exceeded", `delta` below zero answering
// `IMPROVED` and zero answering `OK`, and a zero baseline magnitude under
// a relative threshold answering one fault text beside the empty percent
// of D-18 (FR-025, FR-026, FR-027, FR-028, FR-029, FR-030, FR-031,
// spec D-4, D-18, spec D-22). The rule values these cases read come from
// the parse, so the grammar and the arithmetic meet here.
//
// T017: the matching, the validation walk and the version pairing of
// `contracts/compare-call.md` over two `ReportData` values built in code:
// matching by the full instance name ignoring the order the documents hold
// their benchmarks in, the `ADDED` and `REMOVED` entries with no values,
// the `SKIPPED` entry of a result with no readable row, every row of the
// fault table in the order the walk reaches it with the first fault
// stopping the call, and the empty pair answering an empty entry list
// (FR-002, FR-008, FR-009, FR-010, FR-011, FR-035, FR-036, FR-037,
// FR-038, FR-039, FR-040, FR-051, SC-013, D-13, D-19, spec D-20).
//
// T026: the value-kind matrix of specs/026-regression-gate/data-model.md
// E-08 over `ReportData` values built in code. Each name of the value space
// answers its own comparison under each aggregate it accepts, in both
// directions and with each threshold form, over the two row shapes the
// reader answers: the single measured row, where every aggregate answers
// that row's value, and the repeated run, where each aggregate reads the
// record the report carries for its own value. The cells that read a
// counter record and a metric record stand red until T031 lands (SC-003,
// FR-013, FR-014, FR-015, FR-016, FR-017, FR-018, FR-019, FR-025, FR-026,
// FR-027, FR-028, FR-029, FR-030, FR-032, FR-036, D-09, D-10, D-18,
// spec D-18, spec D-21, spec D-22).
//
// T043: the library surface of specs/026. A caller that links the harness
// archive builds two `ReportData` values and its rules in code, calls the
// public `compareReports`, and reads the entry list and the one verdict. The
// cases open no document and build no command line: the entries stand in the
// baseline's run order with the added names after them in the contender's
// order, each carrying both values, the change and the state word, and the
// rule a library caller hands over is the value `parseCompareRule` answers
// with its six fields, or one error naming the fault (SC-007, FR-001,
// FR-002, FR-004, FR-005, FR-033, FR-034, spec D-5).
//
// T038: the noise guard of D-11 over `ReportData` values built in code: the
// warning either document carries, `coefficientOfVariation` held against the
// relative threshold, `sampleStdDev` held against the absolute threshold in
// the stored unit of the value, and a metric `runningRatio` below one, each
// read from either report, beside the value that carries no statistics record
// and stands guarded through the carried warning alone. Each case plants one
// condition in a pair whose two values stand equal, so the reason names the
// condition that fired (SC-006, FR-041, FR-042, FR-043, FR-046, spec D-19).
// ============================================================================

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <initializer_list>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <unistd.h>

#include "detail/json_fixture.hpp"
#include "speedgun-ng/benchmark.hpp"

using sg::test::check;
using sg::test::fail;
using sg::test::sameValue;

// The row selection and the value resolution of E-08, D-09 and D-10,
// defined in `source/harness/compare.cpp` by T013. They are internal to
// the harness archive, so they are declared here, outside
// `detail/compare.hpp`, which carries only the mode-level calls until the
// public comparison call lands at T018.
namespace sg::detail
{
[[nodiscard]] auto selectCompareRow(const sg::BenchmarkResult& result)
    -> const sg::ResultRow*;

[[nodiscard]] auto resolveCompareValue(const sg::BenchmarkResult& result,
                                       const sg::CompareRule& rule)
    -> std::optional<double>;

// The threshold arithmetic and the verdict words of E-09 for one matched
// pair of values, defined in `source/harness/compare.cpp` by T015.
[[nodiscard]] auto compareValues(const sg::CompareRule& rule,
                                 std::string_view name,
                                 double baseline,
                                 double contender)
    -> std::expected<sg::CompareEntry, std::string>;

// The matching, the validation walk and the version pairing of
// `contracts/compare-call.md`, defined in `source/harness/compare.cpp` by
// T017. The public `compareReports` that will wrap it lands at T018.
[[nodiscard]] auto compareReportValues(
    const sg::ReportData& baseline,
    const sg::ReportData& contender,
    const std::vector<sg::CompareRule>& rules,
    bool strict) -> std::expected<sg::CompareResult, std::string>;

// The noise guard of FR-042 to FR-046, the node the comparison calls for
// every comparable slot, defined in `source/harness/compare.cpp` by T016.
[[nodiscard]] auto noiseGuard(const sg::CompareRule& rule,
                              const sg::BenchmarkResult& baseline,
                              const sg::BenchmarkResult& contender,
                              const sg::NoiseWarning* baselineWarning,
                              const sg::NoiseWarning* contenderWarning)
    -> std::optional<std::string>;

// The batch filter of the compare mode (D-12, FR-050), defined in
// `source/harness/compare.cpp` by T022: the four position-indexed lists
// of a loaded value move together.
auto applyBatchFilter(sg::ReportData& report,
                      const std::regex& pattern) -> void;

// The table of FR-053 to FR-056, the node the compare mode calls; its
// declaration stands in `detail/compare.hpp`.
auto printCompareTable(const sg::CompareResult& result,
                       const sg::ReportData& baseline,
                       const sg::ReportData& contender,
                       const std::string& baselinePath,
                       const std::string& contenderPath) -> void;
}  // namespace sg::detail

namespace
{

// FR-005 and the contract postcondition: the fault line names the field
// of the fault and the text read.
auto faultNames(const std::string& error,
                std::string_view text,
                std::string_view field) -> bool
{
  return error.find(field) != std::string::npos
      && error.find(text) != std::string::npos;
}

// One row of the fault table: the parse must answer an error, a value
// stays absent, and that error must name `field` and the text read.
auto faulted(const char* text, const char* field, const char* what) -> void
{
  const auto parsed = sg::parseCompareRule(text);
  if (parsed) {
    fail(what);
    return;
  }
  check(faultNames(parsed.error(), text, field), what);
}

// FR-022, FR-023: a bare number is neither the percent form nor the
// `abs=` form, so the threshold field faults.
auto bareNumberFaultsTheThreshold() -> void
{
  faulted("time:lower:5",
          "threshold",
          "a bare number faults the threshold field (FR-022, FR-023)");
}

// FR-016: only the four aggregate words stand, so the aggregate faults.
auto unknownAggregateFaults() -> void
{
  faulted("time@mode:lower:5%",
          "aggregate",
          "an unknown aggregate word faults the aggregate (FR-016)");
}

// FR-018, spec D-21: a percentile cell holds one value and takes no
// aggregate, so a written suffix faults.
auto writtenSuffixOnAPointFaults() -> void
{
  faulted("p99@median:lower:10%",
          "aggregate",
          "a written aggregate suffix on a percentile cell faults (FR-018)");
}

// FR-018, spec D-21: the same for a custom statistic.
auto writtenSuffixOnAStatisticFaults() -> void
{
  faulted("stat:peak@mean:lower:5%",
          "aggregate",
          "a written aggregate suffix on a statistic faults (FR-018)");
}

// FR-012: the direction word is required and only two stand.
auto unknownDirectionFaults() -> void
{
  faulted("time:faster:5%",
          "direction",
          "an unknown direction word faults the direction (FR-012)");
}

// D-07: the threshold field carries the percent first, then the `abs=`
// marker, so the reversed order faults the threshold.
auto reversedThresholdOrderFaults() -> void
{
  faulted("time@median:lower:abs=12.5,5%",
          "threshold",
          "the abs= marker before the percent faults the threshold (D-07)");
}

// FR-024: a rule filter is a std::regex over the full instance name, so an
// invalid pattern is a rule fault naming the filter.
auto invalidFilterRegexFaults() -> void
{
  faulted("time@median:lower:5%:(",
          "filter",
          "an invalid filter regex faults the filter field (FR-024)");
}

// FR-022, FR-023, D-07: the accepted order of the two threshold forms,
// with every field of the value answering the text.
auto percentThenAbsoluteIsAccepted() -> void
{
  constexpr std::string_view kText = "time@median:lower:5%,abs=12.5";
  const auto parsed = sg::parseCompareRule(kText);
  if (!parsed) {
    fail("the percent then the abs= marker parses (FR-022, FR-023, D-07)");
    return;
  }
  check(parsed->text == kText, "the rule keeps the text the call read (D-08)");
  check(parsed->valueName == "time", "the value name is time (FR-013)");
  check(parsed->aggregate == sg::Aggregate::MEDIAN,
        "the written suffix answers MEDIAN (FR-016)");
  check(parsed->direction == sg::Direction::LOWER_IS_BETTER,
        "the direction word answers LOWER_IS_BETTER (FR-012)");
  check(
      parsed->relativeThreshold && sameValue(*parsed->relativeThreshold, 0.05),
      "the percent answers the fraction 0.05 (FR-022)");
  check(
      parsed->absoluteThreshold && sameValue(*parsed->absoluteThreshold, 12.5),
      "the abs= marker answers 12.5 in the stored unit (FR-023, spec D-22)");
  check(parsed->filter.empty(), "the rule wrote no filter (FR-024)");
}

// FR-020, D-06, spec D-15: the escaped colon stands inside the label, and
// the value name carries the prefix plus the unescaped label.
auto escapedColonInLabelIsAccepted() -> void
{
  constexpr std::string_view kText = "counter:cache\\:misses@mean:higher:2%";
  const auto parsed = sg::parseCompareRule(kText);
  if (!parsed) {
    fail("the escaped colon inside a counter label parses (FR-020, D-06)");
    return;
  }
  check(parsed->text == kText, "the rule keeps the text the call read (D-08)");
  check(parsed->valueName == "counter:cache:misses",
        "the value name is the prefix plus the unescaped label (FR-015, "
        "FR-020, E-01)");
  check(parsed->aggregate == sg::Aggregate::MEAN,
        "the written suffix answers MEAN (FR-016)");
  check(parsed->direction == sg::Direction::HIGHER_IS_BETTER,
        "the direction word answers HIGHER_IS_BETTER (FR-012)");
  check(
      parsed->relativeThreshold && sameValue(*parsed->relativeThreshold, 0.02),
      "the percent answers the fraction 0.02 (FR-022)");
  check(!parsed->absoluteThreshold,
        "the rule wrote no abs= threshold (FR-023)");
  check(parsed->filter.empty(), "the rule wrote no filter (FR-024)");
}

// The value names of the resolution cases carry the aggregate their `@`
// suffix would have written; the parse writes MEDIAN for every value the
// report holds as a statistics record (FR-016, spec D-21).
constexpr std::optional<sg::Aggregate> kMedian {sg::Aggregate::MEDIAN};
constexpr std::optional<sg::Aggregate> kMean {sg::Aggregate::MEAN};
constexpr std::optional<sg::Aggregate> kMin {sg::Aggregate::MIN};
constexpr std::optional<sg::Aggregate> kMax {sg::Aggregate::MAX};

// One result built in code: these cases resolve values, so no document
// is read and no fixture is needed (FR-071 covers the fixture documents).
auto makeResult() -> sg::BenchmarkResult
{
  sg::BenchmarkResult result;
  result.name = "bm";
  result.suite = "bm";
  result.caseName = "bm";
  return result;
}

// One row of the given kind, carrying the two time columns.
auto addRow(sg::BenchmarkResult& result,
            const sg::RowKind kind,
            const double realTime,
            const double cpuTime) -> sg::ResultRow&
{
  auto& row = result.rows.emplace_back();
  row.kind = kind;
  row.timePerIterationNs = realTime;
  row.cpuTimePerIterationNs = cpuTime;
  return row;
}

// One statistics record carrying the four aggregates a rule can read; the
// two disclosure fields stay at their defaults (FR-016).
auto statsOf(const double mean,
             const double median,
             const double min,
             const double max) -> sg::Statistics
{
  return sg::Statistics {
      .samples = 2, .mean = mean, .median = median, .min = min, .max = max};
}

// The value one rule names in one result (E-08).
auto valueOf(const sg::BenchmarkResult& result,
             const char* valueName,
             const std::optional<sg::Aggregate> aggregate)
    -> std::optional<double>
{
  sg::CompareRule rule;
  rule.text = valueName;
  rule.valueName = valueName;
  rule.aggregate = aggregate;
  rule.relativeThreshold = 0.05;
  return sg::detail::resolveCompareValue(result, rule);
}

// The resolution answers one number.
auto answers(const sg::BenchmarkResult& result,
             const char* valueName,
             const std::optional<sg::Aggregate> aggregate,
             const double expected,
             const char* what) -> void
{
  const auto value = valueOf(result, valueName, aggregate);
  if (!value.has_value()) {
    fail(what);
    return;
  }
  check(sameValue(*value, expected), what);
}

// The resolution answers no value: the missing value that FR-036 and
// FR-038 turn into the fault line (FR-036, FR-038, spec D-18).
auto answersNothing(const sg::BenchmarkResult& result,
                    const char* valueName,
                    const std::optional<sg::Aggregate> aggregate,
                    const char* what) -> void
{
  check(!valueOf(result, valueName, aggregate).has_value(), what);
}

// FR-013: `time` names the column the result's primary flag selects,
// while `real_time` and `cpu_time` name their own columns whichever the
// primary is. The two time columns of each row hold different values, so
// a name that reads the other column fails.
auto timeFollowsThePrimaryFlag() -> void
{
  auto cpuPrimary = makeResult();
  cpuPrimary.primaryIsCpu = true;
  static_cast<void>(addRow(cpuPrimary, sg::RowKind::REPETITION, 1000.0, 250.0));
  answers(cpuPrimary,
          "time",
          kMedian,
          250.0,
          "time names the cpu column when the primary is cpu (FR-013)");
  answers(cpuPrimary,
          "real_time",
          kMedian,
          1000.0,
          "real_time names the real column under a cpu primary (FR-013)");
  answers(cpuPrimary,
          "cpu_time",
          kMedian,
          250.0,
          "cpu_time names the cpu column under a cpu primary (FR-013)");

  auto realPrimary = makeResult();
  realPrimary.primaryIsCpu = false;
  static_cast<void>(
      addRow(realPrimary, sg::RowKind::REPETITION, 1000.0, 250.0));
  answers(realPrimary,
          "time",
          kMedian,
          1000.0,
          "time names the real column when the primary is real (FR-013)");
  answers(realPrimary,
          "real_time",
          kMedian,
          1000.0,
          "real_time names the real column under a real primary (FR-013)");
  answers(realPrimary,
          "cpu_time",
          kMedian,
          250.0,
          "cpu_time names the cpu column under a real primary (FR-013)");
}

// FR-014, D-10: `p<point>` reads the chosen row's cell of column 0 at the
// point's index in `percentilePoints`. The second column holds other
// values, so a read of the wrong column fails; a point the result does
// not report and a cell that answers nothing are missing values.
auto percentileCellReadsColumnZero() -> void
{
  auto result = makeResult();
  result.percentilePoints = {50.0, 99.0};
  auto& row = addRow(result, sg::RowKind::REPETITION, 1000.0, 250.0);
  row.percentiles = {
      {sg::PercentileValue {true, 500.0}, sg::PercentileValue {true, 990.0}},
      {sg::PercentileValue {true, 11.0}, sg::PercentileValue {true, 22.0}}};
  answers(result,
          "p99",
          std::nullopt,
          990.0,
          "p99 reads the column 0 cell of the 99 point (FR-014, D-10)");
  answers(result,
          "p50",
          std::nullopt,
          500.0,
          "p50 reads the column 0 cell of the 50 point (FR-014, D-10)");
  answersNothing(
      result,
      "p90",
      std::nullopt,
      "a point the result does not report answers no value " "(FR-038)");

  auto gap = makeResult();
  gap.percentilePoints = {99.0};
  auto& gapRow = addRow(gap, sg::RowKind::REPETITION, 1000.0, 250.0);
  gapRow.percentiles = {{sg::PercentileValue {false, 0.0}}};
  answersNothing(
      gap,
      "p99",
      std::nullopt,
      "a percentile cell that answers nothing is a missing value " "(FR-014)");
}

// FR-016, FR-032, D-09: when an aggregate row stands it is the row the
// comparison reads, and the aggregate comes from the statistics record
// that row carries. The row's own columns hold other values, so a read of
// the record, the row's statistics, answers where the row itself fails.
auto aggregateRowIsReadWhenOneStands() -> void
{
  auto result = makeResult();
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1000.0, 250.0));
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1200.0, 300.0));
  auto& aggregate = addRow(result, sg::RowKind::AGGREGATE, 777.0, 888.0);
  aggregate.timeStats = statsOf(300.0, 250.0, 100.0, 900.0);
  const auto* const row = sg::detail::selectCompareRow(result);
  check(row != nullptr && row->kind == sg::RowKind::AGGREGATE,
        "the aggregate row is the row the comparison reads when one stands "
        "(FR-032, D-09)");
  answers(result,
          "time",
          kMedian,
          250.0,
          "the default aggregate reads the median of the record (FR-016)");
  answers(result,
          "time",
          kMean,
          300.0,
          "the mean aggregate reads the mean of the record (FR-016)");
  answers(result,
          "time",
          kMin,
          100.0,
          "the min aggregate reads the min of the record (FR-016)");
  answers(result,
          "time",
          kMax,
          900.0,
          "the max aggregate reads the max of the record (FR-016)");
}

// FR-032, D-09: with no aggregate row, the single measured row is the row
// the comparison reads.
auto singleMeasuredRowStandsWhenNoneStands() -> void
{
  auto result = makeResult();
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1000.0, 250.0));
  const auto* const row = sg::detail::selectCompareRow(result);
  check(row != nullptr && row->kind == sg::RowKind::REPETITION,
        "the single measured row is the row the comparison reads (FR-032, "
        "D-09)");
  answers(result, "time", kMedian, 250.0,
          "the single row answers the median aggregate with its value "
          "(FR-017, FR-032)");
}

// FR-017, spec D-18: a run with one row and no statistics record answers
// every aggregate with that row's value. The row carries a record of
// other numbers, so a read of the record instead fails.
auto singleRowAnswersEveryAggregateWithItsValue() -> void
{
  auto result = makeResult();
  auto& row = addRow(result, sg::RowKind::REPETITION, 1000.0, 250.0);
  row.timeStats = statsOf(999.0, 888.0, 666.0, 777.0);
  answers(result, "time", kMedian, 250.0,
          "a single row answers the median aggregate with its own value "
          "(FR-017)");
  answers(
      result,
      "time",
      kMean,
      250.0,
      "a single row answers the mean aggregate with its own value " "(FR-017)");
  answers(
      result,
      "time",
      kMin,
      250.0,
      "a single row answers the min aggregate with its own value " "(FR-017)");
  answers(
      result,
      "time",
      kMax,
      250.0,
      "a single row answers the max aggregate with its own value " "(FR-017)");
}

// spec D-18, FR-036: the report holds a statistics record for the primary
// time column only, so an aggregate named on the other time column of a
// repeated run is a missing value while the primary column reads its
// record.
auto aggregateOnAColumnWithNoRecordIsMissing() -> void
{
  auto result = makeResult();
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1000.0, 250.0));
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1200.0, 300.0));
  auto& aggregate = addRow(result, sg::RowKind::AGGREGATE, 777.0, 888.0);
  aggregate.timeStats = statsOf(300.0, 250.0, 100.0, 900.0);
  answers(
      result,
      "cpu_time",
      kMean,
      300.0,
      "cpu_time names the record of the primary column (FR-016, spec D-18)");
  answersNothing(result, "real_time", kMean,
                 "real_time with an aggregate over a repeated run whose "
                 "primary is cpu is a missing value (spec D-18, FR-036)");
  answersNothing(result, "real_time", kMedian,
                 "the default aggregate on the column with no record is a "
                 "missing value (FR-016, spec D-18)");
}

// FR-015, E-08: a counter column reads `counters` at the label's index in
// `counterLabels` and its record in `counterStats`; a metric column reads
// `metrics` at the index in `metricLabels` and its record in the row's
// `metricStats`; a custom statistic reads `statisticValues` and carries no
// record. The counter label holds a colon, the form the rule text escapes.
auto labelledColumnsReadTheirOwnRecords() -> void
{
  auto result = makeResult();
  result.counterLabels = {"cache:misses"};
  result.counterStats = {statsOf(40.0, 41.0, 30.0, 50.0)};
  result.metricLabels = {"work"};
  result.statisticValues = {
      sg::StatisticValue {"peak", sg::StatUnit::TIME, 33.0}};
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1000.0, 250.0));
  auto& aggregate = addRow(result, sg::RowKind::AGGREGATE, 777.0, 888.0);
  aggregate.counters = {7.0};
  aggregate.metrics = {sg::MetricValue {12.0}};
  aggregate.metricStats = {statsOf(15.0, 16.0, 10.0, 20.0)};
  answers(result, "counter:cache:misses", kMean, 40.0,
          "a counter label with an aggregate reads the counter's record "
          "(FR-015, FR-016, E-08)");
  answers(result,
          "counter:cache:misses",
          std::nullopt,
          7.0,
          "a counter label reads the row's column (FR-015, E-08)");
  answers(result, "metric:work", kMean, 15.0,
          "a metric label with an aggregate reads the metric's record "
          "(FR-015, FR-016, E-08)");
  answers(result,
          "metric:work",
          std::nullopt,
          12.0,
          "a metric label reads the row's value (FR-015, E-08)");
  answers(result,
          "stat:peak",
          std::nullopt,
          33.0,
          "a statistic name reads the statistic's value (FR-015, E-08)");
  answersNothing(
      result,
      "stat:absent",
      std::nullopt,
      "a statistic name the result does not carry answers no value " "(FR-"
                                                                     "036)");
  answersNothing(
      result,
      "counter:absent",
      std::nullopt,
      "a counter label the result does not carry answers no value " "(FR-036)");
}

// FR-011, spec D-20: a result whose rows hold no row a rule can read - a
// complexity fit entry, a non-measured outcome, a result with no row -
// answers no value, which is the entry the caller marks SKIPPED.
auto noReadableRowAnswersNothing() -> void
{
  auto fit = makeResult();
  auto& bigO = addRow(fit, sg::RowKind::BIG_O, 0.0, 0.0);
  bigO.fit.quantity = "time";
  bigO.fit.coefficient = 12.0;
  static_cast<void>(addRow(fit, sg::RowKind::RMS, 0.0, 0.0));
  check(sg::detail::selectCompareRow(fit) == nullptr,
        "a complexity fit result holds no readable row (FR-011, spec D-20)");
  answersNothing(
      fit,
      "time",
      kMedian,
      "a complexity fit result answers no value (FR-011, spec D-20)");

  auto skipped = makeResult();
  skipped.outcome = sg::RunOutcome::SKIPPED;
  skipped.reason = "the run was interrupted";
  static_cast<void>(addRow(skipped, sg::RowKind::REPETITION, 1000.0, 250.0));
  check(sg::detail::selectCompareRow(skipped) == nullptr,
        "a non-measured outcome holds no readable row (FR-011)");

  auto empty = makeResult();
  check(sg::detail::selectCompareRow(empty) == nullptr,
        "a result with no row holds no readable row (FR-011, spec D-20)");

  auto pair = makeResult();
  static_cast<void>(addRow(pair, sg::RowKind::REPETITION, 1000.0, 250.0));
  static_cast<void>(addRow(pair, sg::RowKind::REPETITION, 1200.0, 300.0));
  check(sg::detail::selectCompareRow(pair) == nullptr,
        "two measured rows with no aggregate row hold no single readable row "
        "(FR-032, D-09)");
}

// One rule of the grammar, the form the gate reads and the arithmetic
// consumes (FR-022, FR-023, spec D-22).
auto ruleOf(const char* text) -> sg::CompareRule
{
  const auto parsed = sg::parseCompareRule(text);
  if (!parsed) {
    fail("the rule text of the threshold case parses");
    return {};
  }
  return *parsed;
}

// The entry one rule answers for one matched pair, or the fault text
// (E-09).
auto verdictOf(const sg::CompareRule& rule,
               const double baseline,
               const double contender)
    -> std::expected<sg::CompareEntry, std::string>
{
  return sg::detail::compareValues(rule, "bm", baseline, contender);
}

// One matched pair answers one state word and the two change numbers; an
// absent number is the `null` the document writes and the `n/a` the table
// prints (E-02, E-09, D-18).
auto pairAnswers(const char* ruleText,
                 const double baseline,
                 const double contender,
                 const sg::CompareState state,
                 const std::optional<double> percent,
                 const std::optional<double> absolute,
                 const char* what) -> void
{
  const auto entry = verdictOf(ruleOf(ruleText), baseline, contender);
  if (!entry) {
    fail(what);
    return;
  }
  check(entry->state == state, what);
  check(entry->changePercent.has_value() == percent.has_value()
            && (!percent.has_value()
                || sameValue(*entry->changePercent, *percent)),
        what);
  check(entry->changeAbsolute.has_value() == absolute.has_value()
            && (!absolute.has_value()
                || sameValue(*entry->changeAbsolute, *absolute)),
        what);
}

// FR-025, spec D-4: the delta runs forward for a lower-is-better value and
// backward for a higher-is-better one, so the sign of the change and the
// state word agree (E-02).
auto deltaRunsForwardForLowerAndBackwardForHigher() -> void
{
  pairAnswers("time@median:lower:5%", 100.0, 150.0, sg::CompareState::REGRESSED,
              50.0, 50.0,
              "a lower-is-better delta is the contender minus the baseline and "
              "regresses (FR-025, FR-026)");
  pairAnswers("time@median:higher:5%", 100.0, 150.0,
              sg::CompareState::IMPROVED, -50.0, -50.0,
              "a higher-is-better delta is the baseline minus the contender "
              "(FR-025, FR-029)");

  const auto rule = ruleOf("time@median:lower:5%");
  const auto entry = verdictOf(rule, 100.0, 150.0);
  if (!entry) {
    fail("the matched pair answers an entry (E-02)");
    return;
  }
  check(entry->name == "bm" && entry->ruleText == rule.text
            && entry->baseline && sameValue(*entry->baseline, 100.0)
            && entry->contender && sameValue(*entry->contender, 150.0),
        "the entry carries the name, the rule text and the two values in the "
        "stored unit (E-02, spec D-22)");
}

// FR-026: the relative test divides the delta by the magnitude of the
// baseline, so a negative baseline still measures its change against a
// positive magnitude.
auto relativeTestDividesByTheBaselineMagnitude() -> void
{
  pairAnswers("time@median:lower:40%", -200.0, -100.0,
              sg::CompareState::REGRESSED, 50.0, 100.0,
              "the relative test divides the delta by the magnitude of a "
              "negative baseline (FR-026)");
  pairAnswers("time@median:lower:60%",
              -200.0,
              -100.0,
              sg::CompareState::OK,
              50.0,
              100.0,
              "a relative change under the threshold answers OK (FR-030)");
}

// FR-027, spec D-22: the absolute test stands in the stored unit of the
// value, so the number the `abs=` marker spelled is a number of
// nanoseconds per iteration.
auto absoluteTestStandsInTheStoredUnit() -> void
{
  pairAnswers("time@median:lower:abs=10", 100.0, 115.0,
              sg::CompareState::REGRESSED, 15.0, 15.0,
              "a delta over the abs= threshold regresses in the stored unit "
              "(FR-027, spec D-22)");
  pairAnswers("time@median:lower:abs=10",
              100.0,
              108.0,
              sg::CompareState::OK,
              8.0,
              8.0,
              "a delta under the abs= threshold answers OK (FR-030)");
}

// spec D-4, FR-028: with both thresholds the row regresses only when both
// are exceeded, so an absolute floor stops a large relative change on a
// tiny value.
auto bothThresholdsNeedBothExceeded() -> void
{
  pairAnswers("time@median:lower:5%,abs=20", 100.0, 112.0,
              sg::CompareState::OK, 12.0, 12.0,
              "a relative change the absolute threshold never reaches answers "
              "OK (FR-028, spec D-4)");
  pairAnswers("time@median:lower:20%,abs=5", 100.0, 112.0,
              sg::CompareState::OK, 12.0, 12.0,
              "an absolute change the relative threshold never reaches answers "
              "OK (FR-028, spec D-4)");
  pairAnswers("time@median:lower:5%,abs=20", 100.0, 130.0,
              sg::CompareState::REGRESSED, 30.0, 30.0,
              "with both thresholds the row regresses when both are exceeded "
              "(FR-028, spec D-4)");
}

// FR-029, FR-030: a delta below zero never regresses and answers
// `IMPROVED`, and a delta of zero answers `OK`.
auto improvementAndZeroAnswerTheirWords() -> void
{
  pairAnswers("time@median:lower:5%,abs=20",
              100.0,
              90.0,
              sg::CompareState::IMPROVED,
              -10.0,
              -10.0,
              "a delta below zero answers IMPROVED (FR-029)");
  pairAnswers("time@median:higher:5%,abs=20",
              100.0,
              110.0,
              sg::CompareState::IMPROVED,
              -10.0,
              -10.0,
              "a higher-is-better delta below zero answers IMPROVED (FR-029)");
  pairAnswers("time@median:lower:5%",
              100.0,
              100.0,
              sg::CompareState::OK,
              0.0,
              0.0,
              "a delta of zero answers OK (FR-030)");
}

// FR-031: a zero baseline magnitude under a rule that carries a relative
// threshold is a fault naming the benchmark and the value, in one line.
auto zeroBaselineUnderARelativeThresholdFaults() -> void
{
  for (const char* const text :
       {"time@median:lower:5%", "time@median:lower:5%,abs=20"})
  {
    const auto entry = verdictOf(ruleOf(text), 0.0, 5.0);
    if (entry) {
      fail("a zero baseline under a relative threshold faults (FR-031)");
      continue;
    }
    check(entry.error().find("bm") != std::string::npos
              && entry.error().find("time") != std::string::npos
              && entry.error().find('\n') == std::string::npos,
          "the zero-baseline fault names the benchmark and the value in one "
          "line (FR-031)");
  }
}

// D-18: with the absolute threshold alone the absolute test answers alone,
// the row still regresses or clears, and the percent stands empty: the
// table prints `n/a` and the document writes `null`.
auto zeroBaselineWithAnAbsoluteThresholdAnswersAlone() -> void
{
  pairAnswers("time@median:lower:abs=10", 0.0, 15.0,
              sg::CompareState::REGRESSED, std::nullopt, 15.0,
              "a zero baseline with the absolute threshold alone regresses on "
              "that test and leaves the percent empty (D-18, FR-027)");
  pairAnswers("time@median:lower:abs=10", 0.0, 5.0, sg::CompareState::OK,
              std::nullopt, 5.0,
              "a zero baseline under the abs= threshold answers OK with an "
              "empty percent (D-18, FR-030)");
}

// A report built in code: the comparison reads the values a load call
// would answer, so these cases read no document (FR-006, FR-007).
auto makeReport(std::vector<sg::BenchmarkResult> results) -> sg::ReportData
{
  sg::ReportData report;
  report.results = std::move(results);
  return report;
}

// One measured benchmark whose single row is the row the comparison reads,
// with the given primary (cpu) time per iteration.
auto measured(const char* name, const double primaryTime) -> sg::BenchmarkResult
{
  auto result = makeResult();
  result.name = name;
  static_cast<void>(
      addRow(result, sg::RowKind::REPETITION, 1000.0, primaryTime));
  return result;
}

// One measured benchmark reporting the 99 point of the primary column at
// `cell`.
auto pointResult(const char* name, const double cell) -> sg::BenchmarkResult
{
  auto result = makeResult();
  result.name = name;
  result.percentilePoints = {99.0};
  auto& row = addRow(result, sg::RowKind::REPETITION, 1000.0, 100.0);
  row.percentiles = {{sg::PercentileValue {true, cell}}};
  return result;
}

// One complexity fit result: its rows hold the fit and no row a rule can
// read (PC-6, spec D-20).
auto fitResult(const char* name) -> sg::BenchmarkResult
{
  auto result = makeResult();
  result.name = name;
  auto& bigO = addRow(result, sg::RowKind::BIG_O, 0.0, 0.0);
  bigO.fit.quantity = "time";
  bigO.fit.coefficient = 12.0;
  static_cast<void>(addRow(result, sg::RowKind::RMS, 0.0, 0.0));
  return result;
}

// The rules of one comparison, each text through the grammar so the parse
// and the comparison meet here (FR-004).
auto rulesOf(std::initializer_list<const char*> texts)
    -> std::vector<sg::CompareRule>
{
  std::vector<sg::CompareRule> rules;
  for (const char* const text : texts) {
    rules.push_back(ruleOf(text));
  }
  return rules;
}

// One comparison of two loaded-shaped values (E-03).
auto compared(const sg::ReportData& baseline,
              const sg::ReportData& contender,
              const std::vector<sg::CompareRule>& rules,
              const bool strict)
    -> std::expected<sg::CompareResult, std::string>
{
  return sg::detail::compareReportValues(baseline, contender, rules, strict);
}

// One row of the fault table of `contracts/compare-call.md`: the call
// answers one line naming what that row says it names, and no partial
// entries reach the caller (FR-003, FR-051).
auto faultRow(const sg::ReportData& baseline,
              const sg::ReportData& contender,
              std::initializer_list<const char*> ruleTexts,
              std::initializer_list<std::string_view> mustName,
              const char* what) -> void
{
  const auto answer = compared(baseline, contender, rulesOf(ruleTexts), false);
  if (answer.has_value()) {
    fail(what);
    return;
  }
  check(answer.error().find('\n') == std::string::npos, what);
  for (const std::string_view needle : mustName) {
    check(answer.error().find(needle) != std::string::npos, what);
  }
}

// The entry at `index`, checked by name, rule text and state word.
auto entryAt(const std::expected<sg::CompareResult, std::string>& answer,
             const std::size_t index,
             const char* name,
             const char* ruleText,
             const sg::CompareState state,
             const char* what) -> void
{
  if (!answer.has_value() || index >= answer->entries.size()) {
    fail(what);
    return;
  }
  const sg::CompareEntry& entry = answer->entries[index];
  check(
      entry.name == name && entry.ruleText == ruleText && entry.state == state,
      what);
}

// An entry holding no values: the two value columns and the two change
// columns all stand empty (E-02).
auto carriesNoValues(const sg::CompareEntry& entry) -> bool
{
  return !entry.baseline.has_value() && !entry.contender.has_value()
      && !entry.changePercent.has_value() && !entry.changeAbsolute.has_value();
}

// FR-008, FR-010: the pair matches by the full instance name, so two
// documents holding the same names in another order still match and answer
// no ADDED or REMOVED row. FR-002, E-03: one entry per benchmark per rule,
// in the baseline's run order.
auto matchingIgnoresTheDocumentOrder() -> void
{
  const auto baseline =
      makeReport({measured("a", 100.0), measured("b", 200.0)});
  const auto contender =
      makeReport({measured("b", 200.0), measured("a", 100.0)});
  const auto answer =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (!answer.has_value()) {
    fail("a reordered pair compares (FR-010)");
    return;
  }
  check(answer->entries.size() == 2,
        "one entry per matched benchmark and rule (FR-001, E-03)");
  entryAt(answer,
          0,
          "a",
          "time@median:lower:5%",
          sg::CompareState::OK,
          "the baseline's run order leads the entries (FR-002)");
  entryAt(answer, 1, "b", "time@median:lower:5%", sg::CompareState::OK,
          "a name the contender holds in another position still matches "
          "(FR-008, FR-010)");
  check(
      answer->entries.front().baseline
          && sameValue(*answer->entries.front().baseline, 100.0)
          && answer->entries.front().contender
          && sameValue(*answer->entries.front().contender, 100.0),
      "the entry carries the two values in the stored unit (E-02, spec D-22)");
  check(answer->passed, "a pair with no change passes (FR-044)");

  const auto twoRules =
      compared(baseline,
               contender,
               rulesOf({"time@median:lower:5%", "real_time:lower:5%"}),
               false);
  if (!twoRules.has_value()) {
    fail("two written rules compare (FR-034)");
    return;
  }
  check(twoRules->entries.size() == 4,
        "two rules answer two entries for one benchmark (FR-001)");
  entryAt(
      twoRules,
      1,
      "a",
      "real_time:lower:5%",
      sg::CompareState::OK,
      "the rules of one benchmark stand together in the entry list " "(FR-"
                                                                     "002)");
  entryAt(twoRules,
          2,
          "b",
          "time@median:lower:5%",
          sg::CompareState::OK,
          "the second benchmark follows the first (FR-002)");
}

// FR-009, FR-002: a name the contender holds alone answers ADDED and a
// name the baseline holds alone answers REMOVED, each with the word and no
// values, and the added names follow the baseline names.
auto addedAndRemovedAnswerTheirWordsWithNoValues() -> void
{
  const auto baseline =
      makeReport({measured("a", 100.0), measured("gone", 100.0)});
  const auto contender =
      makeReport({measured("a", 100.0), measured("new", 300.0)});
  const auto answer =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (!answer.has_value()) {
    fail("a pair with one name on either side alone compares (FR-009)");
    return;
  }
  check(answer->entries.size() == 3,
        "the unmatched names answer one entry each (FR-009)");
  entryAt(answer,
          1,
          "gone",
          "time@median:lower:5%",
          sg::CompareState::REMOVED,
          "a name the contender lacks answers REMOVED (FR-009)");
  entryAt(answer,
          2,
          "new",
          "time@median:lower:5%",
          sg::CompareState::ADDED,
          "an added name follows the baseline names (FR-002, FR-009)");
  check(carriesNoValues(answer->entries[1])
            && carriesNoValues(answer->entries[2]),
        "the ADDED and REMOVED entries carry no values (E-02)");
}

// D-19: a pair sharing no benchmark name matched nothing, which answers an
// empty entry list and passes; a written filter reaching a name on one side
// alone does not turn that into the fault of FR-040.
auto anEmptyPairAnswersAnEmptyEntryList() -> void
{
  const auto baseline = makeReport({measured("a", 100.0)});
  const auto contender = makeReport({measured("b", 100.0)});
  const auto answer =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (!answer.has_value()) {
    fail("a pair sharing no name compares (D-19)");
    return;
  }
  check(answer->entries.empty(),
        "the empty pair answers an empty entry list (D-19)");
  check(answer->passed, "the empty pair passes (D-19)");

  const auto empty = compared(
      makeReport({}), makeReport({}), rulesOf({"time@median:lower:5%"}), false);
  if (!empty.has_value()) {
    fail("two reports holding no benchmark compare (D-19)");
    return;
  }
  check(empty->entries.empty(),
        "two reports holding no benchmark answer no entry (D-19)");

  const auto filtered =
      compared(baseline, contender, rulesOf({"time@median:lower:5%:a"}), false);
  if (!filtered.has_value()) {
    fail("a filter reaching one side alone compares (D-19)");
    return;
  }
  check(filtered->entries.empty(),
        "a scope holding no pair answers no entry and no fault (D-19, FR-040)");
}

// FR-051, D-13, SC-013: the two schemaVersion texts must agree, the
// pairing check stands first inside the call, and the fault names both
// versions.
auto aSchemaVersionMismatchFaultsFirst() -> void
{
  auto baseline = makeReport({measured("a", 100.0)});
  auto contender = makeReport({measured("a", 100.0)});
  baseline.schemaVersion = "1.0";
  contender.schemaVersion = "1.1";
  const auto answer =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (answer.has_value()) {
    fail("a schema version mismatch faults (FR-051, D-13)");
    return;
  }
  check(answer.error().find("1.0") != std::string::npos
            && answer.error().find("1.1") != std::string::npos
            && answer.error().find('\n') == std::string::npos,
        "the version fault names both versions in one line (SC-013, D-13)");

  // The pairing stands before the rule walk, so a rule that would fault on
  // its own filter never reaches the answer.
  const auto both = compared(
      baseline, contender, rulesOf({"time@median:lower:5%:nomatch"}), false);
  check(!both.has_value() && both.error().find("1.0") != std::string::npos
            && both.error().find("nomatch") == std::string::npos,
        "the version pairing stands first in the walk (D-13)");
}

// FR-039: a written filter that matches no benchmark in either report
// faults, naming the rule and the filter.
auto aFilterSelectingNothingFaults() -> void
{
  const auto pair = makeReport({measured("a", 100.0)});
  faultRow(pair,
           pair,
           {"time@median:lower:5%:nomatch"},
           {"time@median:lower:5%:nomatch", "nomatch"},
           "a rule filter matching nothing faults the rule (FR-039)");
}

// FR-036: a value name that matches no value of a matched benchmark stops
// the call with one line naming the rule, the benchmark and the report.
auto aMissingValueFaultsNamingRuleBenchmarkAndReport() -> void
{
  const auto baseline = makeReport({measured("a", 100.0)});
  const auto contender = makeReport({measured("a", 100.0)});
  faultRow(baseline,
           contender,
           {"counter:absent@mean:higher:5%"},
           {"counter:absent@mean:higher:5%", "a", "baseline"},
           "a counter label the report lacks is a missing value (FR-036)");
}

// FR-038: a percentile point one report lacks counts as a missing value,
// and the fault names the report that lacks it.
auto aPercentilePointOneReportLacksFaults() -> void
{
  const auto baseline = makeReport({pointResult("a", 990.0)});
  const auto contender = makeReport({measured("a", 100.0)});
  faultRow(
      baseline,
      contender,
      {"p99:lower:5%"},
      {"p99:lower:5%", "a", "contender"},
      "a percentile point the contender lacks is a missing value (FR-038)");
}

// FR-037: a metric whose availability is not countable counts as a missing
// value, and the fault names the report that carries it.
auto anUncountableMetricCountsAsAMissingValue() -> void
{
  auto clean = makeResult();
  clean.name = "a";
  clean.metricLabels = {"work"};
  auto& cleanRow = addRow(clean, sg::RowKind::REPETITION, 1000.0, 100.0);
  cleanRow.metrics = {sg::MetricValue {12.0}};

  auto gapped = makeResult();
  gapped.name = "a";
  gapped.metricLabels = {"work"};
  auto& gappedRow = addRow(gapped, sg::RowKind::REPETITION, 1000.0, 100.0);
  gappedRow.metrics = {sg::MetricValue {
      .value = 12.0, .availability = sg::counters::Availability::GAP}};

  faultRow(makeReport({clean}),
           makeReport({gapped}),
           {"metric:work:lower:5%"},
           {"metric:work:lower:5%", "a", "contender"},
           "a metric that is not countable counts as a missing value (FR-037)");
}

// FR-040, spec D-20: a rule whose whole filtered scope holds only results
// with no readable row faults, naming the rule and a benchmark of that
// scope.
auto aScopeOfOnlyUnreadableResultsFaults() -> void
{
  const auto baseline = makeReport({fitResult("family")});
  const auto contender = makeReport({fitResult("family")});
  faultRow(baseline, contender, {"time@median:lower:5%"},
           {"time@median:lower:5%", "family"},
           "a scope holding only results with no readable row faults (FR-040, "
           "spec D-20)");
}

// FR-011, spec D-20: inside a scope that also holds readable results, a
// result with no readable row answers SKIPPED with no values and does not
// fail the run.
auto anUnreadableResultAnswersSkippedBesideAReadableOne() -> void
{
  const auto baseline = makeReport({fitResult("family"), measured("a", 100.0)});
  const auto contender =
      makeReport({fitResult("family"), measured("a", 100.0)});
  const auto answer =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (!answer.has_value()) {
    fail("a scope mixing a fit result and a readable one compares (spec D-20)");
    return;
  }
  check(answer->entries.size() == 2,
        "the fit result still answers an entry (FR-011)");
  entryAt(answer,
          0,
          "family",
          "time@median:lower:5%",
          sg::CompareState::SKIPPED,
          "a result with no readable row answers SKIPPED (FR-011, spec D-20)");
  entryAt(
      answer,
      1,
      "a",
      "time@median:lower:5%",
      sg::CompareState::OK,
      "the readable benchmark of the same scope still compares (spec D-20)");
  check(carriesNoValues(answer->entries.front()),
        "the SKIPPED entry carries no values (E-02)");
  check(answer->passed, "a SKIPPED row does not fail the run (spec D-20)");
}

// FR-031: a zero baseline magnitude under a rule that carries a relative
// threshold is the fault row of the contract's table, naming the rule and
// the benchmark, and no entry reaches the caller.
auto aZeroBaselineUnderARelativeThresholdFaults() -> void
{
  const auto baseline = makeReport({measured("a", 0.0)});
  const auto contender = makeReport({measured("a", 5.0)});
  faultRow(baseline,
           contender,
           {"time@median:lower:5%"},
           {"time@median:lower:5%", "a"},
           "a zero baseline under a relative threshold faults (FR-031)");
}

// FR-003, FR-035: the walk covers every rule over its whole scope before
// any value is compared, so the fault the table lists first is the one the
// call answers, and only that one.
auto theFirstFaultOfTheWalkStopsTheCall() -> void
{
  // Inside one rule's walk the filter row stands before the missing-value
  // row, so a rule that selects nothing answers its own fault even though
  // the value it names is missing as well.
  const auto pair = makeReport({measured("a", 100.0)});
  const auto filterFirst = compared(
      pair, pair, rulesOf({"counter:absent@mean:higher:5%:nomatch"}), false);
  check(!filterFirst.has_value()
            && filterFirst.error().find("selects no benchmark")
                   != std::string::npos
            && filterFirst.error().find("finds no") == std::string::npos,
        "the filter fault outranks the missing-value fault and stops the call "
        "(FR-003, FR-035, FR-039)");

  // The missing-value row outranks the zero-baseline row, even when the
  // zero baseline belongs to the benchmark the walk reads first.
  const auto zeroBase =
      makeReport({pointResult("z", 0.0), pointResult("m", 500.0)});
  const auto zeroContender =
      makeReport({pointResult("z", 0.0), measured("m", 100.0)});
  const auto walk =
      compared(zeroBase, zeroContender, rulesOf({"p99:lower:5%"}), false);
  check(!walk.has_value() && walk.error().find("m") != std::string::npos
            && walk.error().find("cannot measure") == std::string::npos,
        "the walk finds the missing value before any comparison finds the "
        "zero baseline (FR-003, FR-035)");
}

// FR-041, FR-044, FR-045: a warning the report carries marks the row
// NOISY with its reason, that row does not fail the run while strict
// stands off, and the same pair fails when strict stands.
auto aCarriedWarningMarksTheRowNoisy() -> void
{
  auto baseline = makeReport({measured("a", 100.0)});
  const auto contender = makeReport({measured("a", 100.0)});
  baseline.noiseWarnings = {sg::NoiseWarning {.spinning = 1}};
  const auto lenient =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (!lenient.has_value()) {
    fail("a pair whose report carries a warning compares (FR-041)");
    return;
  }
  entryAt(lenient,
          0,
          "a",
          "time@median:lower:5%",
          sg::CompareState::NOISY,
          "a carried warning marks the row NOISY (FR-041)");
  check(!lenient->entries.front().reason.empty(),
        "the NOISY row names the reason that fired (FR-044)");
  check(lenient->entries.front().baseline.has_value()
            && lenient->entries.front().contender.has_value(),
        "a NOISY row keeps its two values (E-02)");
  check(lenient->passed,
        "a NOISY row does not fail the run while strict stands off (FR-044)");

  const auto strict =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), true);
  check(strict.has_value() && !strict->passed,
        "strict moves the verdict for a NOISY row and no other word (FR-045)");
}

// FR-044: a REGRESSED row fails the verdict.
auto aRegressionFailsTheVerdict() -> void
{
  const auto baseline = makeReport({measured("a", 100.0)});
  const auto contender = makeReport({measured("a", 200.0)});
  const auto answer =
      compared(baseline, contender, rulesOf({"time@median:lower:5%"}), false);
  if (!answer.has_value()) {
    fail("a regressed pair compares (FR-026)");
    return;
  }
  entryAt(answer,
          0,
          "a",
          "time@median:lower:5%",
          sg::CompareState::REGRESSED,
          "a change over the threshold answers REGRESSED (FR-026)");
  check(!answer->passed, "a REGRESSED row fails the verdict (FR-044)");
}

// T038: the noise guard of D-11. Each case below plants one condition in one
// report of a pair whose two values stand equal, so the state word can only
// move through the guard, and reads the reason for the condition that fired
// (SC-006, FR-041, FR-042, FR-043, FR-046, spec D-19).

// One repeated run whose aggregate record carries the two noise fields the
// guard reads, with the primary time value standing at `median` (FR-042).
auto recorded(const char* name,
              const double median,
              const double coefficientOfVariation,
              const double sampleStdDev) -> sg::BenchmarkResult
{
  auto result = makeResult();
  result.name = name;
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1000.0, median));
  static_cast<void>(addRow(result, sg::RowKind::REPETITION, 1000.0, median));
  auto& aggregate = addRow(result, sg::RowKind::AGGREGATE, 0.0, 0.0);
  aggregate.timeStats =
      sg::Statistics {.samples = 2,
                      .mean = median,
                      .median = median,
                      .sampleStdDev = sampleStdDev,
                      .coefficientOfVariation = coefficientOfVariation,
                      .min = median,
                      .max = median};
  return result;
}

// One measured run carrying one metric column whose value stands at `value`
// beside the running ratio the disclosure carries (FR-043).
auto metricRun(const char* name,
               const double value,
               const double runningRatio) -> sg::BenchmarkResult
{
  auto result = makeResult();
  result.name = name;
  result.metricLabels = {"work"};
  auto& row = addRow(result, sg::RowKind::REPETITION, 1000.0, 100.0);
  row.metrics = {
      sg::MetricValue {.value = value, .runningRatio = runningRatio}};
  return result;
}

// One repeated run carrying one attached statistic beside the time record of
// the same run: the value the report summarizes nowhere (FR-046, spec D-18).
auto statisticRun(const char* name,
                  const double statistic,
                  const double coefficientOfVariation) -> sg::BenchmarkResult
{
  auto result = recorded(name, 100.0, coefficientOfVariation, 0.0);
  result.statisticValues = {
      sg::StatisticValue {"peak", sg::StatUnit::TIME, statistic}};
  return result;
}

// The guard fired: the pair answers one NOISY row that keeps its two values
// and passes while strict stands off, and its reason names every condition
// the case planted (FR-041, FR-042, FR-043, FR-044, SC-006).
auto guardMarks(const sg::ReportData& baseline,
                const sg::ReportData& contender,
                const char* ruleText,
                const double baselineValue,
                const double contenderValue,
                std::initializer_list<std::string_view> needles,
                const char* what) -> void
{
  const auto answer = compared(baseline, contender, rulesOf({ruleText}), false);
  if (!answer.has_value() || answer->entries.size() != 1) {
    fail(what);
    return;
  }
  const sg::CompareEntry& entry = answer->entries.front();
  check(entry.state == sg::CompareState::NOISY, what);
  check(entry.baseline.has_value() && entry.contender.has_value()
            && sameValue(*entry.baseline, baselineValue)
            && sameValue(*entry.contender, contenderValue),
        what);
  for (const std::string_view needle : needles) {
    check(entry.reason.find(needle) != std::string::npos, what);
  }
  check(answer->passed, what);
}

// The guard stayed aside: the pair answers the verdict word of its values and
// carries no reason, which is what makes each marking case a live probe
// (FR-042, FR-044, FR-046).
auto guardStaysAside(const sg::ReportData& baseline,
                     const sg::ReportData& contender,
                     const char* ruleText,
                     const sg::CompareState state,
                     const char* what) -> void
{
  const auto answer = compared(baseline, contender, rulesOf({ruleText}), false);
  if (!answer.has_value() || answer->entries.size() != 1) {
    fail(what);
    return;
  }
  check(answer->entries.front().state == state, what);
  check(answer->entries.front().reason.empty(), what);
}

// FR-041, spec D-19: the warning either document carries marks the row by
// itself, and the reason names the side that carried it.
auto aCarriedWarningOnEitherReportNamesTheWarning() -> void
{
  auto baselineWarning = makeReport({measured("a", 100.0)});
  baselineWarning.noiseWarnings = {sg::NoiseWarning {.spinning = 1}};
  guardMarks(baselineWarning,
             makeReport({measured("a", 100.0)}),
             "time@median:lower:5%",
             100.0,
             100.0,
             {"noise warning", "baseline"},
             "a baseline warning marks the row by itself (FR-041)");

  auto contenderWarning = makeReport({measured("a", 100.0)});
  contenderWarning.noiseWarnings = {sg::NoiseWarning {.sharesOneCpu = true}};
  guardMarks(makeReport({measured("a", 100.0)}),
             contenderWarning,
             "time@median:lower:5%",
             100.0,
             100.0,
             {"noise warning", "contender"},
             "a contender warning marks the row by itself (FR-041, spec D-19)");
}

// FR-042, spec D-19: the coefficient of variation of the value's own record
// stands against the relative threshold, and either report over its bound is
// enough to mark the row.
auto aCoefficientOfVariationOverTheRelativeThresholdMarksTheRow() -> void
{
  guardMarks(
      makeReport({recorded("a", 100.0, 0.2, 1.0)}),
      makeReport({recorded("a", 100.0, 0.0, 1.0)}),
      "time@median:lower:5%",
      100.0,
      100.0,
      {"coefficient of variation", "relative", "baseline"},
      "a baseline record over the relative threshold marks the row (FR-042)");
  guardMarks(makeReport({recorded("a", 100.0, 0.0, 1.0)}),
             makeReport({recorded("a", 100.0, 0.2, 1.0)}),
             "time@median:lower:5%",
             100.0,
             100.0,
             {"coefficient of variation", "relative", "contender"},
             "a contender record over the relative threshold marks the row "
             "(FR-042, spec D-19)");
  guardStaysAside(
      makeReport({recorded("a", 100.0, 0.01, 1.0)}),
      makeReport({recorded("a", 100.0, 0.02, 1.0)}),
      "time@median:lower:5%",
      sg::CompareState::OK,
      "two records under the relative threshold leave the word alone (FR-042)");
  // spec D-19 pairs each field with the threshold it matches, so a record
  // over the relative bound answers an absolute-only rule with nothing.
  guardStaysAside(makeReport({recorded("a", 100.0, 0.9, 1.0)}),
                  makeReport({recorded("a", 100.0, 0.9, 1.0)}),
                  "time@median:lower:abs=50",
                  sg::CompareState::OK,
                  "a coefficient of variation stands aside of an absolute-only "
                  "rule (spec D-19)");
  // D-11: the guard runs before the verdict words, so a row that would
  // regress answers NOISY while its two values stand in the columns.
  guardMarks(makeReport({recorded("a", 100.0, 0.2, 1.0)}),
             makeReport({measured("a", 200.0)}),
             "time@median:lower:5%",
             100.0,
             200.0,
             {"coefficient of variation"},
             "the guard answers NOISY over a pair that would regress (FR-042, "
             "FR-044)");
  guardStaysAside(
      makeReport({recorded("a", 100.0, 0.0, 1.0)}),
      makeReport({measured("a", 200.0)}),
      "time@median:lower:5%",
      sg::CompareState::REGRESSED,
      "the same pair without an over-bound record answers REGRESSED (FR-026)");
}

// FR-042, spec D-22: the sample standard deviation of the value's own record
// stands against the absolute threshold in the stored unit of the value, and
// either report over its bound is enough.
auto aSampleStdDevOverTheAbsoluteThresholdMarksTheRow() -> void
{
  guardMarks(
      makeReport({recorded("a", 100.0, 0.0, 100.0)}),
      makeReport({recorded("a", 100.0, 0.0, 0.0)}),
      "time@median:lower:abs=50",
      100.0,
      100.0,
      {"sample standard deviation", "absolute", "baseline"},
      "a baseline record over the absolute threshold marks the row (FR-042)");
  guardMarks(makeReport({recorded("a", 100.0, 0.0, 0.0)}),
             makeReport({recorded("a", 100.0, 0.0, 75.0)}),
             "time@median:lower:abs=50",
             100.0,
             100.0,
             {"sample standard deviation", "absolute", "contender"},
             "a contender record over the absolute threshold marks the row "
             "(FR-042, spec D-19)");
  // The bound is a number in the stored unit: 50 marks the record of 75
  // above and leaves the records of 40 and 49 alone.
  guardStaysAside(
      makeReport({recorded("a", 100.0, 0.0, 40.0)}),
      makeReport({recorded("a", 100.0, 0.0, 49.0)}),
      "time@median:lower:abs=50",
      sg::CompareState::OK,
      "two records under the absolute threshold leave the word alone (FR-042)");
}

// FR-043, spec D-19: a metric column whose running ratio sits below one in
// either report marks the row.
auto aMetricRunningRatioBelowOneMarksTheRow() -> void
{
  guardMarks(makeReport({metricRun("a", 2.0, 0.5)}),
             makeReport({metricRun("a", 2.0, 1.0)}),
             "metric:work:lower:5%",
             2.0,
             2.0,
             {"running ratio", "baseline"},
             "a baseline metric ratio below one marks the row (FR-043)");
  guardMarks(
      makeReport({metricRun("a", 2.0, 1.0)}),
      makeReport({metricRun("a", 2.0, 0.25)}),
      "metric:work:lower:5%",
      2.0,
      2.0,
      {"running ratio", "contender"},
      "a contender metric ratio below one marks the row (FR-043, spec D-19)");
  guardStaysAside(
      makeReport({metricRun("a", 2.0, 1.0)}),
      makeReport({metricRun("a", 2.0, 1.0)}),
      "metric:work:lower:5%",
      sg::CompareState::OK,
      "two metric columns at a full ratio leave the word alone (FR-043)");
}

// FR-046: a value the report summarizes nowhere carries no statistics record,
// so the record of another column of the same run stands aside of its guard
// and the carried warning guards it on its own.
auto aValueWithNoRecordIsGuardedByTheCarriedWarningAlone() -> void
{
  guardStaysAside(
      makeReport({statisticRun("a", 10.0, 0.9)}),
      makeReport({statisticRun("a", 10.0, 0.9)}),
      "stat:peak:lower:5%",
      sg::CompareState::OK,
      "a statistic value reads no record of the run it stands in (FR-046)");
  auto warned = makeReport({statisticRun("a", 10.0, 0.9)});
  warned.noiseWarnings = {sg::NoiseWarning {.runnableCpus = 3}};
  guardMarks(
      warned,
      makeReport({statisticRun("a", 10.0, 0.9)}),
      "stat:peak:lower:5%",
      10.0,
      10.0,
      {"noise warning", "baseline"},
      "a carried warning guards a value that carries no record (FR-046)");
}

// The seven numbers one matrix result carries, one for each name of the
// value space that stands in a row or in the statistic list (E-08).
struct MatrixValues
{
  double realTime;
  double cpuTime;
  double point;
  double counter;
  double metric;
  double timeStatistic;
  double percentageStatistic;
};

// The three statistics records a repeated run's report carries: the primary
// time column, one counter column and one metric column (E-08, spec D-18).
// Each record keeps its coefficient of variation and its standard deviation
// inside every threshold the matrix writes, so the noise guard of D-11
// stands aside of these cases.
struct MatrixRecords
{
  sg::Statistics time;
  sg::Statistics counter;
  sg::Statistics metric;
};

// One result carrying a value of every name of the value space: the two
// time columns, the 99 point of the primary column, one counter column, one
// metric column and the two attached statistics with their units (E-08,
// FR-013, FR-014, FR-015).
auto matrixResult(const char* name,
                  const MatrixValues& values) -> sg::BenchmarkResult
{
  auto result = makeResult();
  result.name = name;
  result.counterLabels = {"bytes"};
  result.metricLabels = {"work"};
  result.statisticValues = {
      sg::StatisticValue {"peak", sg::StatUnit::TIME, values.timeStatistic},
      sg::StatisticValue {
          "busy", sg::StatUnit::PERCENTAGE, values.percentageStatistic}};
  auto& row =
      addRow(result, sg::RowKind::REPETITION, values.realTime, values.cpuTime);
  result.percentilePoints = {99.0};
  row.percentiles = {{sg::PercentileValue {true, values.point}}};
  row.counters = {values.counter};
  row.metrics = {sg::MetricValue {.value = values.metric}};
  return result;
}

// The single-row shape: one measured row and no aggregate row, so every
// aggregate answers that row's value (FR-017, D-09).
auto singleMatrix(const char* name,
                  const MatrixValues& values) -> sg::BenchmarkResult
{
  return matrixResult(name, values);
}

// The repeated shape: two measured rows and the aggregate row the runner
// leaves behind them, which carries the three records and holds no value of
// its own in the time, percentile, counter or metric columns (FR-032,
// D-09, source/harness/runner.cpp:2163 to :2264). The measured rows carry
// numbers no field of a record holds, so a read of a row in place of the
// record fails the cell that names the record.
auto repeatedMatrix(const char* name,
                    const MatrixValues& values,
                    const MatrixRecords& records) -> sg::BenchmarkResult
{
  auto result = matrixResult(name, values);
  static_cast<void>(
      addRow(result, sg::RowKind::REPETITION, values.realTime, values.cpuTime));
  auto& aggregate = addRow(result, sg::RowKind::AGGREGATE, 0.0, 0.0);
  aggregate.timeStats = records.time;
  aggregate.metricStats = {records.metric};
  result.counterStats = {records.counter};
  return result;
}

// One cell of the matrix: the rule text, the case name the run prints, the
// state word the entry answers and the two change numbers (E-02, E-09).
struct MatrixCell
{
  const char* ruleText;
  const char* what;
  sg::CompareState state;
  std::optional<double> percent;
  std::optional<double> absolute;
};

// One cell over one matched pair: the rule answers exactly one entry, and
// that entry carries the rule text, the state word and the two change
// numbers of the cell, each double read through `sameValue` (E-02, E-03,
// E-08, E-09, SC-003).
auto cellAnswers(const sg::ReportData& baseline,
                 const sg::ReportData& contender,
                 const MatrixCell& cell,
                 const char* shape) -> void
{
  const auto answer =
      compared(baseline, contender, rulesOf({cell.ruleText}), false);
  if (!answer.has_value() || answer->entries.size() != 1) {
    std::printf(
        "%s over the %s shape answers %s\n",
        cell.ruleText,
        shape,
        answer.has_value() ? "no single entry" : answer.error().c_str());
    fail(cell.what);
    return;
  }
  const sg::CompareEntry& entry = answer->entries.front();
  check(entry.ruleText == cell.ruleText && entry.state == cell.state,
        cell.what);
  check(entry.changePercent.has_value() == cell.percent.has_value()
            && (!cell.percent.has_value()
                || sameValue(*entry.changePercent, *cell.percent)),
        cell.what);
  check(entry.changeAbsolute.has_value() == cell.absolute.has_value()
            && (!cell.absolute.has_value()
                || sameValue(*entry.changeAbsolute, *cell.absolute)),
        cell.what);
}

// SC-003, E-08: the single-row shape. Each name answers its own comparison
// under each aggregate it accepts, in both directions and with each
// threshold form, and each aggregate answers the value of the one row the
// result carries (FR-013 to FR-018, FR-025 to FR-030, FR-032, D-09, D-10,
// D-18, spec D-21, spec D-22).
auto everyValueNameAnswersUnderASingleRow() -> void
{
  constexpr MatrixValues kBase {.realTime = 1000.0,
                                .cpuTime = 400.0,
                                .point = 900.0,
                                .counter = 40.0,
                                .metric = 12.0,
                                .timeStatistic = 30.0,
                                .percentageStatistic = 40.0};
  constexpr MatrixValues kContender {.realTime = 1100.0,
                                     .cpuTime = 440.0,
                                     .point = 990.0,
                                     .counter = 44.0,
                                     .metric = 13.2,
                                     .timeStatistic = 33.0,
                                     .percentageStatistic = 44.0};
  const auto baseline = makeReport({singleMatrix("bm", kBase)});
  const auto contender = makeReport({singleMatrix("bm", kContender)});
  constexpr MatrixCell kCells[] {
      {"time@median:lower:5%",
       "the median primary time answers the row value (FR-017)",
       sg::CompareState::REGRESSED,
       10.0,
       40.0},
      {"time@mean:lower:5%,abs=100",
       "the mean primary time keeps the floor (FR-028)",
       sg::CompareState::OK,
       10.0,
       40.0},
      {"time@min:higher:5%",
       "the min primary time read backward improves (FR-029)",
       sg::CompareState::IMPROVED,
       -10.0,
       -40.0},
      {"time@max:higher:abs=20",
       "the max primary time alone improves (FR-027)",
       sg::CompareState::IMPROVED,
       -10.0,
       -40.0},
      {"real_time@median:lower:5%",
       "real_time names its own column (FR-013)",
       sg::CompareState::REGRESSED,
       10.0,
       100.0},
      {"cpu_time@mean:lower:5%",
       "cpu_time names its own column (FR-013, FR-017)",
       sg::CompareState::REGRESSED,
       10.0,
       40.0},
      {"p99:lower:5%",
       "the 99 point answers its cell (FR-014, D-10)",
       sg::CompareState::REGRESSED,
       10.0,
       90.0},
      {"p99:higher:abs=50",
       "the 99 point read backward improves (FR-027)",
       sg::CompareState::IMPROVED,
       -10.0,
       -90.0},
      {"counter:bytes@median:lower:5%",
       "the counter label answers its column (FR-015)",
       sg::CompareState::REGRESSED,
       10.0,
       4.0},
      {"counter:bytes@mean:higher:abs=10",
       "the counter label read backward improves (FR-027)",
       sg::CompareState::IMPROVED,
       -10.0,
       -4.0},
      {"metric:work@median:lower:5%,abs=1",
       "the metric label regresses past both (FR-028)",
       sg::CompareState::REGRESSED,
       10.0,
       1.2},
      {"metric:work@max:higher:5%",
       "the metric label read backward improves (FR-029)",
       sg::CompareState::IMPROVED,
       -10.0,
       -1.2},
      {"stat:peak:lower:5%",
       "a TIME statistic answers one value (FR-018)",
       sg::CompareState::REGRESSED,
       10.0,
       3.0},
      {"stat:busy:higher:abs=2",
       "a PERCENTAGE statistic answers percent points (D-22)",
       sg::CompareState::IMPROVED,
       -10.0,
       -4.0}};
  for (const MatrixCell& cell : kCells) {
    cellAnswers(baseline, contender, cell, "single-row");
  }
  faultRow(baseline,
           contender,
           {"counter:Bytes:lower:5%"},
           {"counter:Bytes", "bm", "baseline"},
           "a counter label in another case matches nothing (FR-019)");
}

// SC-003, E-08, FR-016: the repeated shape. The aggregate row stands, so
// each aggregate reads the record the report carries for its own value:
// `timeStats` for the primary time column, `counterStats` at the counter
// label's index and `metricStats` at the metric label's index, and the time
// column with no record is a missing value (FR-013, FR-015, FR-016, FR-018,
// FR-025 to FR-029, FR-032, FR-036, D-09, spec D-18, spec D-22).
auto everyAggregateReadsTheRecordOfItsValue() -> void
{
  const MatrixRecords kBaseRecords {
      .time = sg::Statistics {.samples = 2,
                              .mean = 410.0,
                              .median = 400.0,
                              .sampleStdDev = 8.0,
                              .coefficientOfVariation = 0.02,
                              .min = 380.0,
                              .max = 460.0},
      .counter = sg::Statistics {.samples = 2,
                                 .mean = 40.0,
                                 .median = 40.0,
                                 .sampleStdDev = 0.5,
                                 .coefficientOfVariation = 0.0125,
                                 .min = 38.0,
                                 .max = 42.0},
      .metric = sg::Statistics {.samples = 2,
                                .mean = 12.1,
                                .median = 12.0,
                                .sampleStdDev = 0.2,
                                .coefficientOfVariation = 0.0165,
                                .min = 11.5,
                                .max = 13.0}};
  const MatrixRecords kContenderRecords {
      .time = sg::Statistics {.samples = 2,
                              .mean = 451.0,
                              .median = 440.0,
                              .sampleStdDev = 8.8,
                              .coefficientOfVariation = 0.02,
                              .min = 418.0,
                              .max = 506.0},
      .counter = sg::Statistics {.samples = 2,
                                 .mean = 44.0,
                                 .median = 44.0,
                                 .sampleStdDev = 0.55,
                                 .coefficientOfVariation = 0.0125,
                                 .min = 41.8,
                                 .max = 46.2},
      .metric = sg::Statistics {.samples = 2,
                                .mean = 13.31,
                                .median = 13.2,
                                .sampleStdDev = 0.22,
                                .coefficientOfVariation = 0.0165,
                                .min = 12.65,
                                .max = 14.3}};
  constexpr MatrixValues kBase {.realTime = 1007.0,
                                .cpuTime = 397.0,
                                .point = 907.0,
                                .counter = 41.0,
                                .metric = 12.5,
                                .timeStatistic = 30.0,
                                .percentageStatistic = 40.0};
  constexpr MatrixValues kContender {.realTime = 1107.0,
                                     .cpuTime = 437.0,
                                     .point = 997.0,
                                     .counter = 41.5,
                                     .metric = 12.9,
                                     .timeStatistic = 33.0,
                                     .percentageStatistic = 44.0};
  const auto baseline = makeReport({repeatedMatrix("bm", kBase, kBaseRecords)});
  const auto contender =
      makeReport({repeatedMatrix("bm", kContender, kContenderRecords)});
  constexpr MatrixCell kCells[] {
      {"time@median:lower:5%",
       "the median aggregate reads the time record (FR-016)",
       sg::CompareState::REGRESSED,
       10.0,
       40.0},
      {"time@mean:lower:5%,abs=100",
       "the mean aggregate reads the time record (FR-028)",
       sg::CompareState::OK,
       10.0,
       41.0},
      {"time@min:higher:5%",
       "the min aggregate reads the time record (FR-029)",
       sg::CompareState::IMPROVED,
       -10.0,
       -38.0},
      {"time@max:lower:5%",
       "the max aggregate reads the time record (FR-026)",
       sg::CompareState::REGRESSED,
       10.0,
       46.0},
      {"cpu_time@mean:lower:5%",
       "cpu_time reads the primary record (spec D-18)",
       sg::CompareState::REGRESSED,
       10.0,
       41.0},
      {"counter:bytes@median:lower:5%",
       "the counter label reads its record (FR-016, E-08)",
       sg::CompareState::REGRESSED,
       10.0,
       4.0},
      {"counter:bytes@mean:higher:abs=10",
       "the counter record read backward improves (FR-027)",
       sg::CompareState::IMPROVED,
       -10.0,
       -4.0},
      {"counter:bytes@min:lower:5%,abs=1",
       "the counter record min regresses past both (FR-028)",
       sg::CompareState::REGRESSED,
       10.0,
       3.8},
      {"metric:work@median:lower:5%,abs=1",
       "the metric label reads its record (FR-016, E-08)",
       sg::CompareState::REGRESSED,
       10.0,
       1.2},
      {"metric:work@max:higher:5%",
       "the metric record max read backward improves (FR-029)",
       sg::CompareState::IMPROVED,
       -10.0,
       -1.3},
      {"stat:peak:lower:5%",
       "a TIME statistic answers beside an aggregate (D-21)",
       sg::CompareState::REGRESSED,
       10.0,
       3.0},
      {"stat:busy:higher:abs=2",
       "a PERCENTAGE statistic answers beside an aggregate",
       sg::CompareState::IMPROVED,
       -10.0,
       -4.0}};
  for (const MatrixCell& cell : kCells) {
    cellAnswers(baseline, contender, cell, "repeated");
  }
  faultRow(baseline,
           contender,
           {"real_time@median:lower:5%"},
           {"real_time", "bm", "baseline"},
           "an aggregate on a column with no record is missing (D-18)");
}

// The public call of the harness archive, the one a library caller reaches
// (FR-001).
auto libraryCall(const sg::ReportData& baseline,
                 const sg::ReportData& contender,
                 const std::vector<sg::CompareRule>& rules,
                 const bool strict)
    -> std::expected<sg::CompareResult, std::string>
{
  return sg::compareReports(baseline, contender, rules, strict);
}

// The entry list as the name and the rule text of every entry, joined in the
// order the call answers them (E-03, FR-002).
auto entryOrder(const sg::CompareResult& result) -> std::string
{
  std::string order;
  for (const sg::CompareEntry& entry : result.entries) {
    if (!order.empty()) {
      order += "; ";
    }
    order += entry.name;
    order += ' ';
    order += entry.ruleText;
  }
  return order;
}

// SC-007, FR-001, FR-002: two rules over a pair sharing two names and adding
// two more answer one entry per benchmark per rule, the baseline's run order
// first and the added names after in the contender's order, each entry
// carrying both values and both change numbers beside its state word, and the
// result carries one verdict for the whole table.
auto theLibraryCallAnswersTheEntriesAndTheVerdict() -> void
{
  const auto baseline =
      makeReport({measured("first", 100.0), measured("second", 100.0)});
  const auto contender = makeReport({measured("second", 120.0),
                                     measured("first", 100.0),
                                     measured("late", 300.0),
                                     measured("early", 400.0)});
  const auto answer =
      libraryCall(baseline,
                  contender,
                  rulesOf({"time@median:lower:5%", "real_time:lower:5%"}),
                  false);
  if (!answer.has_value()) {
    fail("a library caller compares two report values (FR-001)");
    return;
  }
  check(entryOrder(*answer)
            == "first time@median:lower:5%; first real_time:lower:5%; "
               "second time@median:lower:5%; second real_time:lower:5%; "
               "late time@median:lower:5%; late real_time:lower:5%; "
               "early time@median:lower:5%; early real_time:lower:5%",
        "one entry per benchmark per rule, the baseline's run order first and "
        "the added names after in the contender's order (FR-001, FR-002)");
  if (answer->entries.size() != 8) {
    fail("the entry list holds one entry per benchmark per rule (FR-002)");
    return;
  }

  const sg::CompareEntry& regressed = answer->entries[2];
  check(regressed.state == sg::CompareState::REGRESSED,
        "a change over the threshold answers REGRESSED (FR-026)");
  check(regressed.baseline && sameValue(*regressed.baseline, 100.0)
            && regressed.contender && sameValue(*regressed.contender, 120.0),
        "the entry carries both values in the stored unit (E-02, spec D-22)");
  check(regressed.changePercent && sameValue(*regressed.changePercent, 20.0)
            && regressed.changeAbsolute
            && sameValue(*regressed.changeAbsolute, 20.0),
        "the entry carries the change as a percent and in the stored unit "
        "(E-02, FR-025)");
  check(answer->entries[1].state == sg::CompareState::OK
            && answer->entries[1].baseline
            && sameValue(*answer->entries[1].baseline, 1000.0)
            && answer->entries[1].contender
            && sameValue(*answer->entries[1].contender, 1000.0),
        "the second rule reads its own column for the same benchmark (FR-013, "
        "FR-034)");
  check(answer->entries[4].state == sg::CompareState::ADDED
            && carriesNoValues(answer->entries[4]),
        "a name the contender holds alone answers ADDED with no values "
        "(FR-009, " "E-02)");
  check(!answer->passed,
        "one REGRESSED row moves the single verdict to false (FR-001, FR-044)");

  const auto clean = libraryCall(
      baseline,
      makeReport({measured("first", 100.0), measured("second", 100.0)}),
      rulesOf({"time@median:lower:5%"}),
      false);
  check(
      clean.has_value() && clean->passed,
      "a pair with no regression answers one verdict of true (FR-001, FR-044)");
}

// FR-033, spec D-5, FR-034: a library caller that names no rule is compared
// under the default rule, and a caller that names rules replaces it.
auto theLibraryCallAppliesTheDefaultRuleToAnEmptyRuleList() -> void
{
  const auto baseline = makeReport({measured("a", 100.0)});
  const auto contender = makeReport({measured("a", 110.0)});
  const auto defaulted = libraryCall(baseline, contender, {}, false);
  if (!defaulted.has_value()) {
    fail("a comparison that names no rule answers a result (FR-033)");
    return;
  }
  check(defaulted->entries.size() == 1,
        "the empty rule list answers the entries of one rule (FR-033)");
  entryAt(defaulted,
          0,
          "a",
          "time@median:lower:5%",
          sg::CompareState::REGRESSED,
          "the default rule reads the 10% change (FR-033, spec D-5)");
  check(!defaulted->passed, "the default rule moves the verdict (FR-044)");

  const auto named = libraryCall(
      baseline, contender, rulesOf({"time@median:lower:20%"}), false);
  if (!named.has_value()) {
    fail("a named rule compares (FR-034)");
    return;
  }
  check(named->entries.size() == 1,
        "one named rule answers one entry (FR-034)");
  entryAt(named,
          0,
          "a",
          "time@median:lower:20%",
          sg::CompareState::OK,
          "a named rule replaces the default (FR-034)");
  check(named->passed, "the verdict follows the named rule (FR-044)");
}

// FR-004, FR-005: the one parse call answers the fields a library caller
// compares with, and a text it cannot read answers one line naming the field
// of the fault and the text read.
auto aLibraryRuleCarriesItsParsedFieldsIntoTheComparison() -> void
{
  constexpr auto kText = "time@max:lower:5%,abs=5:second";
  const auto parsed = sg::parseCompareRule(kText);
  if (!parsed.has_value()) {
    fail("a rule with both thresholds and a filter parses (FR-022, FR-023, "
         "FR-024)");
    return;
  }
  check(parsed->text == kText, "the rule keeps the text the call read (D-08)");
  check(parsed->valueName == "time", "the value name is time (FR-013)");
  check(parsed->aggregate == sg::Aggregate::MAX,
        "the written suffix answers MAX (FR-016)");
  check(parsed->direction == sg::Direction::LOWER_IS_BETTER,
        "the direction word answers LOWER_IS_BETTER (FR-012)");
  check(
      parsed->relativeThreshold && sameValue(*parsed->relativeThreshold, 0.05),
      "the percent answers the fraction 0.05 (FR-022)");
  check(parsed->absoluteThreshold && sameValue(*parsed->absoluteThreshold, 5.0),
        "the abs= marker answers 5 in the stored unit (FR-023, spec D-22)");
  check(parsed->filter == "second",
        "the filter holds the text after the threshold field (FR-024)");

  const auto baseline =
      makeReport({measured("first", 100.0), measured("second", 100.0)});
  const auto contender =
      makeReport({measured("first", 100.0), measured("second", 104.0)});
  const auto answer = libraryCall(baseline, contender, {*parsed}, false);
  if (!answer.has_value()) {
    fail("the parsed rule compares the pair (FR-004)");
    return;
  }
  check(answer->entries.size() == 1,
        "the rule filter scopes the pair to one benchmark (FR-024)");
  if (answer->entries.size() != 1) {
    return;
  }
  const sg::CompareEntry& entry = answer->entries.front();
  check(entry.name == "second" && entry.ruleText == kText
            && entry.state == sg::CompareState::OK,
        "the filtered entry names the matched benchmark and carries the whole "
        "rule text (FR-024, D-08)");
  check(entry.baseline && sameValue(*entry.baseline, 100.0)
            && entry.contender && sameValue(*entry.contender, 104.0)
            && entry.changePercent && sameValue(*entry.changePercent, 4.0)
            && entry.changeAbsolute && sameValue(*entry.changeAbsolute, 4.0),
        "the parsed thresholds read the pair under the rule's own aggregate "
        "(FR-017, FR-030)");
  check(answer->passed,
        "a change under both thresholds passes (FR-028, FR-044)");

  const auto bad = sg::parseCompareRule("time@median:lower:5%,abs");
  check(!bad.has_value()
            && faultNames(bad.error(), "time@median:lower:5%,abs", "threshold")
            && bad.error().find('\n') == std::string::npos,
        "a rule text the parse cannot read answers one line naming the "
        "threshold field and the text read (FR-005)");
}

// T055: the grammar corners the fault table had not reached. A percentile
// name whose point does not read answers the value-name fault through
// each of the four ways the number test refuses: no text at all, a first
// character that is no digit, a spelling `std::from_chars` reads whole
// but the grammar does not allow, and a magnitude it refuses outright. A
// prefix that names nothing, a name outside the seven, an unreadable
// threshold in each shape, a rule that stops before the direction word
// and before the threshold field, and a backslash at the end of a label
// or before a plain character answer their own faults (FR-004, FR-005,
// FR-013, FR-018, FR-019, FR-022, FR-023, FR-024).
auto theGrammarFaultsEveryUnreadableCorner() -> void
{
  faulted(
      "p:", "value name", "a percentile name with no point faults (FR-018)");
  faulted("px:lower:5%",
          "value name",
          "a percentile name whose point is no number faults (FR-018)");
  faulted("p1e5:lower:5%",
          "value name",
          "a percentile point the grammar does not allow faults (FR-023)");
  faulted("p1e400:lower:5%",
          "value name",
          "a percentile point no number reads faults (FR-023)");
  faulted("p9e999:lower:5%",
          "value name",
          "a percentile point past the range of a double faults (FR-023)");
  faulted("p5x:lower:5%",
          "value name",
          "a percentile point with text after the digits faults (FR-023)");
  faulted(
      "p99999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "99999999999999999999999999999999999999999:lower:5%",
      "value name",
      "a percentile point past every double faults (FR-023)");
  faulted("counter::lower:5%",
          "value name",
          "a prefix that names nothing faults (FR-015)");
  faulted(
      "foo:lower:5%", "value name", "a name outside the seven faults (FR-013)");
  faulted("time@median:lower:x%",
          "threshold",
          "a percent with no number faults (FR-022)");
  faulted("time@median:lower:5x%",
          "threshold",
          "a percent number followed by a stray character faults (FR-023)");
  faulted("time@median:lower:1e400%",
          "threshold",
          "a percent past the range of a double faults (FR-022)");
  faulted("time@median:lower::",
          "threshold",
          "an empty threshold field faults (FR-022)");
  faulted("time@median:lower:5%,abs=1,2",
          "threshold",
          "two commas in the threshold field fault (FR-023, D-07)");
  faulted("time@median:lower",
          "threshold",
          "a rule that stops before the threshold field faults (FR-022)");
  faulted("time",
          "direction",
          "a rule that stops before the direction word faults (FR-012)");
  faulted("counter:ab\\", "direction",
          "a backslash at the end of a label stands inside it and the "
          "missing direction faults (FR-019, FR-012)");
}

// T055: the three accepted shapes the parse table had not carried: the
// default aggregate with no `@`, an empty filter after the last colon,
// and a backslash before a character that stands in no escape pair, the
// backslash kept inside the label (FR-016, FR-019, FR-024).
auto theGrammarAcceptsTheRemainingShapes() -> void
{
  const auto bare = sg::parseCompareRule("time:lower:5%");
  check(bare.has_value() && bare->valueName == "time"
            && bare->aggregate == sg::Aggregate::MEDIAN && bare->filter.empty(),
        "a rule with no aggregate suffix defaults to the median (FR-016)");
  const auto emptyFilter = sg::parseCompareRule("time@median:lower:5%:");
  check(emptyFilter.has_value() && emptyFilter->filter.empty(),
        "a rule with an empty filter field parses with an empty filter "
        "(FR-024)");
  const auto plainEscape = sg::parseCompareRule("counter:a\\xb:lower:5%");
  check(plainEscape.has_value() && plainEscape->valueName == "counter:a\\xb",
        "a backslash before a plain character stands inside the label "
        "(FR-019)");
}

// T055: the value sites no earlier case reached: the primary fold of a
// row answering a gap state, a metric label the result does not carry, a
// hand-built name outside the seven forms, a percentile name with no
// readable point, a row that carries no percentile columns at all, and a
// percentile column shorter than the point list the result declares
// (FR-013, FR-014, FR-036, FR-038).
auto theUnreadableValueSitesAnswerNothing() -> void
{
  auto gap = makeResult();
  auto& gapRow = addRow(gap, sg::RowKind::REPETITION, 1000.0, 250.0);
  gapRow.timeAvailability = sg::counters::Availability::GAP;
  answersNothing(
      gap,
      "time",
      kMedian,
      "the primary fold answering a gap state answers no value " "(FR-036)");

  auto plain = makeResult();
  plain.metricLabels = {"work"};
  static_cast<void>(addRow(plain, sg::RowKind::REPETITION, 1000.0, 250.0));
  answersNothing(
      plain,
      "metric:nosuch",
      std::nullopt,
      "a metric label the result does not carry answers no " "value (FR-036)");
  answersNothing(
      plain,
      "px",
      std::nullopt,
      "a percentile name with no readable point answers no " "value (FR-014)");
  answersNothing(plain,
                 "median",
                 std::nullopt,
                 "a name outside the seven forms answers no value " "(FR-013)");

  auto noColumns = makeResult();
  noColumns.percentilePoints = {50.0};
  static_cast<void>(addRow(noColumns, sg::RowKind::REPETITION, 1000.0, 250.0));
  answersNothing(
      noColumns,
      "p50",
      std::nullopt,
      "a row that carries no percentile columns answers no " "cell (FR-038)");

  auto thin = makeResult();
  thin.percentilePoints = {50.0, 99.0};
  auto& thinRow = addRow(thin, sg::RowKind::REPETITION, 1000.0, 250.0);
  thinRow.percentiles = {{sg::PercentileValue {true, 240.0}}};
  answersNothing(thin, "p99", std::nullopt,
                 "a percentile column shorter than the point list answers "
                 "no cell (FR-038)");
}

// T055: the guard reads no account for a result that holds no readable
// row, reads no running ratio for a metric label the result does not
// carry, and joins every condition that fired into one reason with one
// separator (FR-044, FR-046, SC-006).
auto theGuardAnswersTheNoRowTheAbsentLabelAndTheJoin() -> void
{
  check(!sg::detail::noiseGuard(ruleOf("time@median:lower:5%"),
                                fitResult("a"),
                                fitResult("a"),
                                nullptr,
                                nullptr)
             .has_value(),
        "a result with no readable row guards nothing (FR-046)");

  auto noLabels = measured("a", 100.0);
  check(!sg::detail::noiseGuard(ruleOf("metric:work:lower:5%"), noLabels,
                                noLabels, nullptr, nullptr)
             .has_value(),
        "a metric label the result does not carry guards no running ratio "
        "(FR-043, FR-046)");

  const sg::NoiseWarning warning {3, 16, true};
  const auto joined = sg::detail::noiseGuard(ruleOf("time@median:lower:5%"),
                                             recorded("a", 100.0, 0.2, 1.0),
                                             recorded("a", 100.0, 0.0, 1.0),
                                             &warning,
                                             nullptr);
  check(joined.has_value() && joined->find("; ") != std::string::npos
            && joined->find("noise warning") != std::string::npos
            && joined->find("coefficient of variation") != std::string::npos,
        "a carried warning and a coefficient of variation over the "
        "relative threshold join into one reason (SC-006)");
}

// T055: strict stands over the verdict, and a row that carries one stays
// untouched: a clean pair under strict answers a passing verdict, the other
// side of the flip that turns a NOISY row into a failure (FR-044, FR-045).
auto strictStandsOffForARowThatCarriesAVerdict() -> void
{
  const auto answer =
      sg::detail::compareReportValues(makeReport({measured("a", 100.0)}),
                                      makeReport({measured("a", 100.0)}),
                                      rulesOf({"time@median:lower:5%"}),
                                      true);
  check(answer.has_value() && answer->passed && answer->entries.size() == 1
            && answer->entries.front().state == sg::CompareState::OK,
        "strict over a clean pair answers a passing verdict (FR-045)");
}

// T055: the batch filter drops the results the pattern misses and moves
// the four position-indexed lists together, the shape only a drop before
// a kept entry reaches (D-12, FR-050).
auto theBatchFilterDropsAndMovesTheLists() -> void
{
  sg::ReportData report;
  report.results = {measured("keep", 100.0),
                    measured("drop", 100.0),
                    measured("keep2", 100.0)};
  report.rowRaw.resize(3);
  report.contextRaw.resize(3);
  report.noiseWarnings.resize(3);
  report.noiseWarnings[2] = sg::NoiseWarning {1, 2, true};
  sg::detail::applyBatchFilter(report, std::regex("keep"));
  check(report.results.size() == 2 && report.results[0].name == "keep"
            && report.results[1].name == "keep2" && report.rowRaw.size() == 2
            && report.contextRaw.size() == 2 && report.noiseWarnings.size() == 2
            && report.noiseWarnings[1].has_value(),
        "the filter keeps a leading match in place, drops the missed result "
        "and moves the later kept entry and its warning into place (D-12, "
        "FR-050)");
}

// T055: the table shapes no fixture pair reached: a name only the
// contender holds, a name neither report holds, a statistic printed in
// its own unit and one printed unscaled, a statistic the report does not
// carry, a percentile cell, a rule text that does not parse, a state
// word outside the seven and a time unit outside the four (FR-053 to
// FR-056).
auto theTablePrintsTheShapesNoFixtureCarries() -> void
{
  auto base = makeResult();
  base.name = "a";
  base.statisticValues = {
      sg::StatisticValue {"peak", sg::StatUnit::TIME, 33.0},
      sg::StatisticValue {"share", sg::StatUnit::PERCENTAGE, 4.0}};
  static_cast<void>(addRow(base, sg::RowKind::REPETITION, 1000.0, 100.0));
  auto other = makeResult();
  other.name = "b";
  other.timeUnit = static_cast<sg::TimeUnit>(42);
  static_cast<void>(addRow(other, sg::RowKind::REPETITION, 1000.0, 100.0));

  sg::ReportData baseline;
  baseline.results = {base, other};
  sg::ReportData contender;
  contender.results = {measured("a", 103.0), measured("new", 300.0)};

  // One entry: with `value` the cells print scaled numbers, without it
  // the table prints `n/a`.
  const auto entryOf = [](const char* name,
                          const char* ruleText,
                          const sg::CompareState state = sg::CompareState::OK,
                          const double value = 0.0) -> sg::CompareEntry
  {
    auto entry = sg::CompareEntry {};
    entry.name = name;
    entry.ruleText = ruleText;
    entry.state = state;
    if (value > 0.0) {
      entry.baseline = value;
      entry.contender = value + 3.0;
      entry.changePercent = 3.0;
      entry.changeAbsolute = 3.0;
    }
    return entry;
  };
  const sg::CompareResult comparison {
      {entryOf("a", "time@median:lower:5%", sg::CompareState::OK, 100.0),
       entryOf("a", "counter:bytes:lower:5%", sg::CompareState::OK, 7.0),
       entryOf("a", "stat:peak:lower:5%", sg::CompareState::OK, 30.0),
       entryOf("a", "stat:share:higher:5%", sg::CompareState::OK, 30.0),
       entryOf("a", "stat:missing:lower:5%", sg::CompareState::OK, 30.0),
       entryOf("a", "p50:lower:5%", sg::CompareState::OK, 30.0),
       entryOf("a", "real_time:lower:5%", sg::CompareState::OK, 100.0),
       entryOf("a", "cpu_time:lower:5%", sg::CompareState::OK, 100.0),
       entryOf("new", "time@median:lower:5%", sg::CompareState::ADDED),
       entryOf("gone", "counter:x:lower:5%", sg::CompareState::REMOVED),
       entryOf("b", "garbage", sg::CompareState::SKIPPED),
       entryOf("b",
               "time@median:lower:5%",
               static_cast<sg::CompareState>(42),
               100.0)},
      true};

  constexpr const char* const kTableCapture =
      "harness_compare_table_capture.txt";
  const int savedOut = dup(STDOUT_FILENO);
  if (std::freopen(kTableCapture, "w", stdout) == nullptr) {
    fail("the table capture opens");
    return;
  }
  sg::detail::printCompareTable(
      comparison, baseline, contender, "base.json", "contender.json");
  std::fflush(stdout);
  if (dup2(savedOut, STDOUT_FILENO) < 0) {
    fail("the table capture closes");
    return;
  }
  close(savedOut);
  const std::string printed = sg::test::readText(kTableCapture);

  std::size_t rows = 0;
  for (const char* needle : {"time@median:lower:5%",
                             "counter:bytes:lower:5%",
                             "stat:peak:lower:5%",
                             "stat:share:higher:5%",
                             "stat:missing:lower:5%",
                             "p50:lower:5%",
                             "real_time:lower:5%",
                             "cpu_time:lower:5%",
                             "counter:x:lower:5%",
                             "garbage"})
  {
    if (printed.find(needle) == std::string::npos) {
      fail(needle);
      return;
    }
  }
  for (const char line : printed) {
    rows += line == '\n';
  }
  check(rows == 15,
        "the table prints one row per entry under the three header lines "
        "(FR-053)");
  check(printed.find("unknown") != std::string::npos,
        "a state word outside the seven prints the guard's word (FR-054)");
  const auto lineWith = [&printed](const std::string& needle) -> std::string
  {
    const std::size_t at = printed.find(needle);
    if (at == std::string::npos) {
      return {};
    }
    const std::size_t end = printed.find('\n', at);
    return printed.substr(at, end - at);
  };
  check(lineWith("stat:peak").find("ns") != std::string::npos
            && lineWith("stat:share").find("ns") == std::string::npos,
        "a statistic of the time unit prints scaled and one of another "
        "unit prints unscaled (FR-055)");
  check(lineWith("counter:bytes").find("ns") == std::string::npos
            && lineWith("real_time").find("ns") != std::string::npos
            && lineWith("cpu_time").find("ns") != std::string::npos,
        "a counter column prints unscaled and the two named time columns "
        "print scaled (FR-055)");
  check(lineWith("garbage").find("n/a") != std::string::npos,
        "a rule text the parse refuses prints its cells with no unit "
        "(D-08, FR-055)");
  check(printed.find("42") == std::string::npos
            && lineWith("time@median:lower:5%").find("ns")
                   != std::string::npos,
        "a time unit outside the four prints under the nanosecond guard "
        "(FR-055)");
}
}  // namespace

auto main() -> int
{
  bareNumberFaultsTheThreshold();
  unknownAggregateFaults();
  writtenSuffixOnAPointFaults();
  writtenSuffixOnAStatisticFaults();
  unknownDirectionFaults();
  reversedThresholdOrderFaults();
  invalidFilterRegexFaults();
  percentThenAbsoluteIsAccepted();
  escapedColonInLabelIsAccepted();
  timeFollowsThePrimaryFlag();
  percentileCellReadsColumnZero();
  aggregateRowIsReadWhenOneStands();
  singleMeasuredRowStandsWhenNoneStands();
  singleRowAnswersEveryAggregateWithItsValue();
  aggregateOnAColumnWithNoRecordIsMissing();
  labelledColumnsReadTheirOwnRecords();
  noReadableRowAnswersNothing();
  deltaRunsForwardForLowerAndBackwardForHigher();
  relativeTestDividesByTheBaselineMagnitude();
  absoluteTestStandsInTheStoredUnit();
  bothThresholdsNeedBothExceeded();
  improvementAndZeroAnswerTheirWords();
  zeroBaselineUnderARelativeThresholdFaults();
  zeroBaselineWithAnAbsoluteThresholdAnswersAlone();
  matchingIgnoresTheDocumentOrder();
  addedAndRemovedAnswerTheirWordsWithNoValues();
  anEmptyPairAnswersAnEmptyEntryList();
  aSchemaVersionMismatchFaultsFirst();
  aFilterSelectingNothingFaults();
  aMissingValueFaultsNamingRuleBenchmarkAndReport();
  aPercentilePointOneReportLacksFaults();
  anUncountableMetricCountsAsAMissingValue();
  aScopeOfOnlyUnreadableResultsFaults();
  anUnreadableResultAnswersSkippedBesideAReadableOne();
  aZeroBaselineUnderARelativeThresholdFaults();
  theFirstFaultOfTheWalkStopsTheCall();
  aCarriedWarningMarksTheRowNoisy();
  aRegressionFailsTheVerdict();
  aCarriedWarningOnEitherReportNamesTheWarning();
  aCoefficientOfVariationOverTheRelativeThresholdMarksTheRow();
  aSampleStdDevOverTheAbsoluteThresholdMarksTheRow();
  aMetricRunningRatioBelowOneMarksTheRow();
  aValueWithNoRecordIsGuardedByTheCarriedWarningAlone();
  everyValueNameAnswersUnderASingleRow();
  everyAggregateReadsTheRecordOfItsValue();
  theLibraryCallAnswersTheEntriesAndTheVerdict();
  theLibraryCallAppliesTheDefaultRuleToAnEmptyRuleList();
  aLibraryRuleCarriesItsParsedFieldsIntoTheComparison();
  theGrammarFaultsEveryUnreadableCorner();
  theGrammarAcceptsTheRemainingShapes();
  theUnreadableValueSitesAnswerNothing();
  theGuardAnswersTheNoRowTheAbsentLabelAndTheJoin();
  strictStandsOffForARowThatCarriesAVerdict();
  theBatchFilterDropsAndMovesTheLists();
  theTablePrintsTheShapesNoFixtureCarries();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
