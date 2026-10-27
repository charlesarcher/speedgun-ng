#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <expected>
#include <fstream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "detail/compare.hpp"

#include <simdjson.h>

#include "detail/json_report.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{
namespace
{

// The three value-name prefixes that carry a label of their own, read
// before the rest of the text splits so a label holding its own colon
// reads whole (FR-015, FR-021).
constexpr std::array<std::string_view, 3> kLabelPrefixes {
    "counter:", "metric:", "stat:"};

// The marker that opens the absolute threshold (FR-023, spec D-22).
constexpr std::string_view kAbsoluteMarker = "abs=";

// The only characters a backslash escapes inside a label; a backslash
// before anything else stands in the label itself (FR-020, spec D-15).
constexpr std::string_view kEscapedCharacters = ":,@\\";

auto isDigit(char character) -> bool
{
  return character >= '0' && character <= '9';
}

// A character of the plain decimal spelling the grammar allows (FR-023).
// The digit test stands inline: a call to `isDigit` here clones under
// coverage and leaves the clone's own branch records unreachable.
auto isDecimalCharacter(char character) -> bool
{
  return character == '.' || (character >= '0' && character <= '9');
}

// A fault naming the field of the fault and the text the call read; the
// caller owns the exit status (FR-005).
auto ruleFault(std::string_view field,
               std::string_view text,
               const std::string& detail)
    -> std::expected<CompareRule, std::string>
{
  return std::unexpected("the " + std::string(field) + " of the rule '"
                         + detail::quotedText(text)
                         + "' is not readable: " + detail);
}

// The tolerance that matches a percentile point written in a rule to the
// point a report carries: the two are decimal spellings of one point, so
// the match is never an exact comparison of two doubles (FR-014, D-10).
constexpr double kPercentilePointTolerance = 1e-9;

// The label prefix a value name carries, or none for a time column, a
// percentile point or a name that carries no prefix (FR-015, FR-021).
auto labelPrefixOf(std::string_view name) -> std::string_view
{
  for (const std::string_view candidate : kLabelPrefixes) {
    if (name.starts_with(candidate)) {
      return candidate;
    }
  }
  return {};
}

// The index of `character` at or after `from`, or the end of the text
// when it stands nowhere in the rest.
auto findOrEnd(std::string_view text,
               char character,
               std::size_t from) -> std::size_t
{
  const std::size_t found = text.find(character, from);
  return found == std::string_view::npos ? text.size() : found;
}

// The number of the grammar: digits, optionally a dot and more digits,
// and nothing else. The sign, the exponent and the hexadecimal spelling
// std::from_chars also reads are not numbers here (FR-023).
auto readNumber(std::string_view text) -> std::optional<double>
{
  if (text.empty() || !isDigit(text.front())) {
    return std::nullopt;
  }
  const auto* const begin = text.data();
  const auto* const end = begin + text.size();
  double value = 0.0;
  const auto [stop, code] = std::from_chars(begin, end, value);
  if (code != std::errc {} || stop != end
      || !std::all_of(text.begin(), text.end(), isDecimalCharacter))
  {
    return std::nullopt;
  }
  return value;
}

// `5%` answers the fraction 0.05 (FR-022).
auto readPercent(std::string_view text) -> std::optional<double>
{
  if (text.empty() || text.back() != '%') {
    return std::nullopt;
  }
  const auto number = readNumber(text.substr(0, text.size() - 1));
  if (!number.has_value()) {
    return std::nullopt;
  }
  return *number / 100.0;
}

// `abs=12.5` answers 12.5 in the stored unit of the value (FR-023,
// spec D-22).
auto readAbsolute(std::string_view text) -> std::optional<double>
{
  if (!text.starts_with(kAbsoluteMarker)) {
    return std::nullopt;
  }
  return readNumber(text.substr(kAbsoluteMarker.size()));
}

// The aggregate word the `@` marker names, or nothing for a word beside
// the four (FR-016).
auto readAggregate(std::string_view word) -> std::optional<Aggregate>
{
  if (word == "median") {
    return Aggregate::MEDIAN;
  }
  if (word == "mean") {
    return Aggregate::MEAN;
  }
  if (word == "min") {
    return Aggregate::MIN;
  }
  if (word == "max") {
    return Aggregate::MAX;
  }
  return std::nullopt;
}

// Read the value name that starts at `from`, ending at the first
// unescaped colon or at sign. Inside a label a backslash stands for the
// character it escapes, and the name the rule keeps is the unescaped
// one (FR-020, FR-021, D-06, spec D-15).
auto scanName(std::string_view text,
              std::size_t from,
              bool label) -> std::pair<std::string, std::size_t>
{
  std::string name;
  std::size_t cursor = from;
  while (cursor < text.size()) {
    if (text[cursor] == ':' || text[cursor] == '@') {
      break;
    }
    if (label && text[cursor] == '\\' && cursor + 1 < text.size()
        && kEscapedCharacters.find(text[cursor + 1]) != std::string_view::npos)
    {
      ++cursor;
    }
    name.push_back(text[cursor]);
    ++cursor;
  }
  return {name, cursor};
}

// Fill the two thresholds of the rule from the threshold field: the
// percent form, the `abs=` form, or the two in that order with one
// comma between (FR-022, FR-023, D-07).
auto readThreshold(std::string_view field, CompareRule& rule) -> bool
{
  const std::size_t comma = field.find(',');
  const bool oneComma = comma == std::string_view::npos
      || field.find(',', comma + 1) == std::string_view::npos;
  if (!oneComma) {
    return false;
  }
  const std::string_view first =
      comma == std::string_view::npos ? field : field.substr(0, comma);
  const auto percent = readPercent(first);
  if (comma == std::string_view::npos) {
    if (percent.has_value()) {
      rule.relativeThreshold = percent;
      return true;
    }
    const auto absolute = readAbsolute(field);
    rule.absoluteThreshold = absolute;
    return absolute.has_value();
  }
  const auto absolute = readAbsolute(field.substr(comma + 1));
  if (!percent.has_value() || !absolute.has_value()) {
    return false;
  }
  rule.relativeThreshold = percent;
  rule.absoluteThreshold = absolute;
  return true;
}

// The prefix the jitter value names carry (specs/027 FR-030).
constexpr std::string_view kJitterPrefix = "jitter:";

// One field of the sixteen `JitterStats` carries, with the reader that
// answers its value.
struct JitterField
{
  std::string_view name;
  double (*value)(const JitterStats&);
};

// The sixteen field names of `JitterStats` in the E-03 order, each with
// the reader that answers its value: the one table the name lookup and
// the value read share, so a field name outside the sixteen reads
// nowhere (specs/027 FR-002, E-03, FR-030).
constexpr std::array<JitterField, 16> kJitterFields {{
    {"gapCount",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.gapCount); }},
    {"gapsPerSecond",
     [](const JitterStats& stats) { return stats.gapsPerSecond; }},
    {"minNs",
     [](const JitterStats& stats) { return static_cast<double>(stats.minNs); }},
    {"medianNs",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.medianNs); }},
    {"meanNs", [](const JitterStats& stats) { return stats.meanNs; }},
    {"p90Ns",
     [](const JitterStats& stats) { return static_cast<double>(stats.p90Ns); }},
    {"p99Ns",
     [](const JitterStats& stats) { return static_cast<double>(stats.p99Ns); }},
    {"p999Ns",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.p999Ns); }},
    {"p9999Ns",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.p9999Ns); }},
    {"maxNs",
     [](const JitterStats& stats) { return static_cast<double>(stats.maxNs); }},
    {"totalNs",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.totalNs); }},
    {"totalPercent",
     [](const JitterStats& stats) { return stats.totalPercent; }},
    {"runtimeNs",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.runtimeNs); }},
    {"thresholdNs",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.thresholdNs); }},
    {"loopPeriodNs",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.loopPeriodNs); }},
    {"droppedGaps",
     [](const JitterStats& stats)
     { return static_cast<double>(stats.droppedGaps); }},
}};

// The parts of one jitter value name, the grammar
// specs/027-jitter-probe/contracts/rule-names.md carries (specs/027
// FR-030, FR-031): the `jitter:run:` scope, the cpu index or the word
// `any` for the worst one, and the field as its index into the table
// above.
struct JitterNameParts
{
  bool runScoped = false;
  bool worstCpu = true;
  std::uint32_t cpu = 0;
  std::size_t field = 0;
};

// Read the parts of one jitter value name and the length the name ends
// at, where `name` is the text after the `jitter:` prefix: the optional
// `run:` scope, the cpu (the word `any` or a decimal index), and one of
// the sixteen field names (specs/027 FR-030, FR-031). The parse of the
// value name and the comparison's own read share this one reader, so
// the grammar stands in one place (FR-033, R-12).
auto readJitterName(std::string_view name)
    -> std::expected<std::pair<JitterNameParts, std::size_t>, std::string>
{
  std::string_view rest = name;
  JitterNameParts parts;
  // The `run:` scope names the whole-run values of the run-level
  // object; its absence names the per-benchmark stats of the entry
  // (FR-031).
  if (rest.starts_with("run:")) {
    parts.runScoped = true;
    rest.remove_prefix(4);
  }
  const std::size_t cpuEnd = findOrEnd(rest, ':', 0);
  const std::string_view cpuToken = rest.substr(0, cpuEnd);
  // The word `any` names the worst cpu for the field; a decimal index
  // names one cpu the report's jitter object carries (FR-030).
  if (cpuToken != "any") {
    parts.worstCpu = false;
    if (cpuToken.empty()
        || !std::all_of(cpuToken.begin(), cpuToken.end(), isDigit))
    {
      return std::unexpected("'" + std::string(cpuToken)
                             + "' is neither an index nor 'any'");
    }
    const auto* const begin = cpuToken.data();
    const auto* const end = begin + cpuToken.size();
    const auto conversion = std::from_chars(begin, end, parts.cpu);
    // The token stands all digits and ends at the colon or at the end
    // of the name, so the conversion consumes the token whole or
    // refuses the value; a partial consumption stands outside the
    // grammar (FR-030).
    if (conversion.ec != std::errc {}) {
      return std::unexpected("'" + std::string(cpuToken)
                             + "' is neither an index nor 'any'");
    }
  }
  if (cpuEnd == rest.size()) {
    return std::unexpected("the name carries no field");
  }
  rest.remove_prefix(cpuEnd + 1);
  // The field name ends at the colon that opens the standing suffix, at
  // an aggregate marker, or at the end of the name; a written aggregate
  // marker then reads as the fault of a single-value name (spec D-21).
  std::size_t fieldEnd = findOrEnd(rest, ':', 0);
  const std::size_t atMark = findOrEnd(rest, '@', 0);
  if (atMark < fieldEnd) {
    fieldEnd = atMark;
  }
  const std::string_view fieldToken = rest.substr(0, fieldEnd);
  const auto field = std::find_if(kJitterFields.begin(),
                                  kJitterFields.end(),
                                  [fieldToken](const JitterField& candidate)
                                  { return candidate.name == fieldToken; });
  if (field == kJitterFields.end()) {
    return std::unexpected("'" + std::string(fieldToken)
                           + "' names none of the sixteen jitter fields");
  }
  parts.field = static_cast<std::size_t>(field - kJitterFields.begin());
  return {std::pair {parts, name.size() - rest.size() + fieldEnd}};
}

// Read one whole rule text into one rule value, the fields in the order
// the grammar gives them (FR-004, FR-012, FR-021).
auto readRule(std::string_view text) -> std::expected<CompareRule, std::string>
{
  CompareRule rule;
  rule.text = std::string(text);

  const std::string_view prefix = labelPrefixOf(text);
  const auto [name, afterName] = scanName(text, prefix.size(), !prefix.empty());

  // Only a value the report holds as a statistics record takes an
  // aggregate: a percentile cell and a custom statistic carry one value
  // (FR-016, FR-018, spec D-21).
  bool statisticsValue = false;
  // The end of a value name that carries a grammar of its own: the
  // jitter names read their scope, cpu and field before the standing
  // suffix reads (specs/027 FR-030, FR-031).
  std::optional<std::size_t> valueEnd;
  if (!prefix.empty()) {
    if (name.empty()) {
      return ruleFault(
          "value name",
          text,
          "the '" + detail::quotedText(prefix) + "' prefix names nothing");
    }
    rule.valueName = std::string(prefix) + name;
    statisticsValue = prefix != "stat:";
  } else if (name == "jitter" && afterName < text.size()
             && text[afterName] == ':')
  {
    const auto jitterName = readJitterName(text.substr(afterName + 1));
    if (!jitterName.has_value()) {
      return ruleFault("value name", text, jitterName.error());
    }
    rule.valueName =
        std::string(text.substr(0, afterName + 1 + jitterName->second));
    valueEnd = afterName + 1 + jitterName->second;
  } else if (name == "time" || name == "real_time" || name == "cpu_time") {
    rule.valueName = name;
    statisticsValue = true;
  } else if (name.starts_with('p')) {
    if (!readNumber(name.substr(1)).has_value()) {
      return ruleFault(
          "value name",
          text,
          "'" + detail::quotedText(name) + "' carries no percentile point");
    }
    rule.valueName = name;
  } else {
    return ruleFault(
        "value name",
        text,
        "'" + detail::quotedText(name) + "' is not a value the report carries");
  }

  std::size_t cursor = valueEnd.value_or(afterName);
  if (cursor < text.size() && text[cursor] == '@') {
    if (!statisticsValue) {
      return ruleFault("aggregate",
                       text,
                       "this value carries one value and takes no aggregate");
    }
    const std::size_t wordEnd = findOrEnd(text, ':', cursor + 1);
    const auto aggregate =
        readAggregate(text.substr(cursor + 1, wordEnd - cursor - 1));
    if (!aggregate.has_value()) {
      return ruleFault("aggregate",
                       text,
                       "the suffix names none of median, mean, min or max");
    }
    rule.aggregate = aggregate;
    cursor = wordEnd;
  } else if (statisticsValue) {
    rule.aggregate = Aggregate::MEDIAN;
  }

  // The name scan and findOrEnd both leave the cursor on a ':' or at the
  // end of the text, so no third shape needs a check here.
  if (cursor >= text.size()) {
    return ruleFault("direction", text, "the rule names no direction word");
  }
  const std::size_t directionEnd = findOrEnd(text, ':', cursor + 1);
  const std::string_view direction =
      text.substr(cursor + 1, directionEnd - cursor - 1);
  if (direction == "lower") {
    rule.direction = Direction::LOWER_IS_BETTER;
  } else if (direction == "higher") {
    rule.direction = Direction::HIGHER_IS_BETTER;
  } else {
    return ruleFault(
        "direction",
        text,
        "'" + detail::quotedText(direction) + "' is neither lower nor higher");
  }
  cursor = directionEnd;

  if (cursor >= text.size()) {
    return ruleFault("threshold", text, "the rule carries no threshold field");
  }
  const std::size_t filterStart = text.find(':', cursor + 1);
  const std::string_view field =
      text.substr(cursor + 1, findOrEnd(text, ':', cursor + 1) - cursor - 1);
  if (!readThreshold(field, rule)) {
    return ruleFault("threshold", text,
                     "'" + detail::quotedText(field)
                         + "' is neither a percent, nor the abs= marker, nor "
                           "the two in that order with one comma between");
  }

  // The filter is the rest of the text, and it must read as the same
  // pattern the batch filter option takes (FR-024, PC-8).
  if (filterStart != std::string_view::npos) {
    rule.filter = std::string(text.substr(filterStart + 1));
    if (!rule.filter.empty()) {
      try {
        static_cast<void>(std::regex(rule.filter));
      } catch (const std::regex_error& error) {
        return ruleFault("filter",
                         text,
                         "'" + detail::quotedText(rule.filter)
                             + "' is no regular expression: "
                             + detail::quotedText(error.what()));
      }
    }
  }
  return rule;
}

// The index of a label in the label list of its kind, or the end of the
// list when it stands nowhere (FR-015, FR-036).
auto labelIndexOf(const std::vector<std::string>& labels,
                  std::string_view label) -> std::size_t
{
  return static_cast<std::size_t>(std::find(labels.begin(), labels.end(), label)
                                  - labels.begin());
}

// The index of a percentile point in the points one result reports, or the
// end of the list when it holds the point nowhere (FR-014, D-10, FR-038).
auto pointIndexOf(const std::vector<double>& points,
                  const double point) -> std::size_t
{
  for (std::size_t index = 0; index < points.size(); ++index) {
    if (std::abs(points[index] - point) <= kPercentilePointTolerance) {
      return index;
    }
  }
  return points.size();
}

// The number one statistics record answers for one aggregate word (FR-016).
auto aggregateValueOf(const Statistics& stats,
                      const Aggregate aggregate) -> double
{
  if (aggregate == Aggregate::MEAN) {
    return stats.mean;
  }
  if (aggregate == Aggregate::MIN) {
    return stats.min;
  }
  if (aggregate == Aggregate::MAX) {
    return stats.max;
  }
  return stats.median;
}

// Where one value name stands in one result: the value the chosen row
// carries for it, and the statistics record the report holds for it, which
// is nothing for a column the report summarizes nowhere (E-08, spec D-18).
struct ValueSite
{
  std::optional<double> value;
  const Statistics* stats = nullptr;
};

// The place one value name occupies in one row of one result, the table of
// specs/026-regression-gate/data-model.md E-08 (FR-013, FR-014, FR-015,
// FR-016, FR-032, D-09). The label list decides whether a labeled name
// stands in the result at all; the row decides whether it carries a value
// at that index, and the record lists decide whether the report holds a
// statistics record for it. The three lists move apart over a repeated
// run, whose aggregate row carries the records and no column of its own
// (source/harness/runner.cpp:2163 to :2264). A row whose primary fold
// reported a gap carries no primary time, so a name that reads that column
// stands nowhere in it (FR-036).
auto valueSiteOf(const BenchmarkResult& result,
                 const ResultRow& row,
                 std::string_view name) -> ValueSite
{
  // FR-036: the row's availability names its primary fold, and a fold that
  // reported a gap holds no primary time, which is why the aggregate
  // statistics skip such a row (source/harness/runner.cpp:202 to :206,
  // FR-002). A name that reads that column answers nothing over such a row,
  // the unanswered value the missing-value fault names.
  const bool primaryColumn = name == "time"
      || (name == "real_time" && !result.primaryIsCpu)
      || (name == "cpu_time" && result.primaryIsCpu);
  if (primaryColumn
      && row.timeAvailability != counters::Availability::COUNTABLE)
  {
    return {};
  }
  // FR-013: `time` is the column the result's primary flag selects, and the
  // report summarizes the primary time in `timeStats` (spec D-18).
  if (name == "time") {
    return {.value = result.primaryIsCpu ? row.cpuTimePerIterationNs
                                         : row.timePerIterationNs,
            .stats = &row.timeStats};
  }
  if (name == "real_time") {
    return {.value = row.timePerIterationNs,
            .stats = result.primaryIsCpu ? nullptr : &row.timeStats};
  }
  if (name == "cpu_time") {
    return {.value = row.cpuTimePerIterationNs,
            .stats = result.primaryIsCpu ? &row.timeStats : nullptr};
  }
  const std::string_view prefix = labelPrefixOf(name);
  if (!prefix.empty()) {
    const std::string_view label = name.substr(prefix.size());
    if (prefix == "counter:") {
      // FR-015, FR-016: the column of `counters` at the label's index in
      // `counterLabels`, and the record at the same index of
      // `counterStats`. The aggregate row of a repeated run holds the
      // record and no counter column, so the value answers nothing there
      // while the record stands (FR-032, D-09).
      const std::size_t index = labelIndexOf(result.counterLabels, label);
      if (index >= result.counterLabels.size()) {
        return {};
      }
      return {.value = index < row.counters.size()
                  ? std::optional<double> {row.counters[index]}
                  : std::nullopt,
              .stats = index < result.counterStats.size()
                  ? &result.counterStats[index]
                  : nullptr};
    }
    if (prefix == "metric:") {
      // FR-015, FR-016: `MetricValue::value` at the label's index in
      // `metricLabels`, and the record at the same index of the row's
      // `metricStats`. The metric column of a metric is read in the unit
      // that value declares, which is the value itself: the fold already
      // scaled it, and no time unit divides it again (spec D-22,
      // source/harness/report.cpp:299).
      const std::size_t index = labelIndexOf(result.metricLabels, label);
      if (index >= result.metricLabels.size()) {
        return {};
      }
      return {.value = index < row.metrics.size()
                  ? std::optional<double> {row.metrics[index].value}
                  : std::nullopt,
              .stats = index < row.metricStats.size() ? &row.metricStats[index]
                                                      : nullptr};
    }
    // FR-015, spec D-21: a custom statistic holds one value and no record.
    for (const auto& statistic : result.statisticValues) {
      if (statistic.name == label) {
        return {.value = statistic.value, .stats = nullptr};
      }
    }
    return {};
  }
  if (name.starts_with('p')) {
    // FR-014, D-10: the cell of column 0, the primary time column, at the
    // point's index in the points the result reports. A point the result
    // does not report and a cell that answers nothing are missing values
    // (FR-038).
    const auto point = readNumber(name.substr(1));
    if (!point.has_value()) {
      return {};
    }
    const std::size_t index = pointIndexOf(result.percentilePoints, *point);
    if (index >= result.percentilePoints.size() || row.percentiles.empty()
        || index >= row.percentiles.front().size()
        || !row.percentiles.front()[index].available)
    {
      return {};
    }
    return {.value = row.percentiles.front()[index].value, .stats = nullptr};
  }
  return {};
}

// The seven state words of FR-054, indexed by the enumerator, the table
// shape the console report's availability names take.
constexpr std::array<const char*, 7> kCompareStateWords {
    "OK", "REGRESSED", "IMPROVED", "NOISY", "ADDED", "REMOVED", "SKIPPED"};
static_assert(kCompareStateWords.size()
              == std::to_underlying(CompareState::SKIPPED) + 1);

// The word one entry's state prints in the table and writes in the
// document (FR-054, E-04).
auto compareStateName(const CompareState state) -> const char*
{
  // The static_assert above ties the table to the last enumerator, so the
  // index of a value of the closed enumeration sits inside the table and
  // the guard answers every other value.
  const auto index = std::to_underlying(state);
  if (index >= kCompareStateWords.size()) {
    return "unknown";
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kCompareStateWords[index];
}

// The label and the divisor of each time unit, indexed by the enumerator:
// the same scale the console report divides a time value by, which the
// table has to follow (FR-055, source/harness/report.cpp).
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

// The value names whose stored unit is nanoseconds per iteration (E-08,
// spec D-22): the three time columns and a percentile cell of the primary
// column. A counter and a metric column carry their own value, and a
// custom statistic answers through its own unit record (FR-055).
auto isTimeColumnName(std::string_view name) -> bool
{
  // A parsed name starting with `p` is a percentile cell: the grammar
  // already rejected a `p` name whose point does not read, so no second
  // number check stands here (FR-014, spec D-22).
  if (name == "time" || name == "real_time" || name == "cpu_time") {
    return true;
  }
  return name.starts_with('p');
}

// The scale one entry's value prints under, or nothing for a value the
// report carries unscaled (FR-055): the unit the baseline report declares
// for that benchmark, the way the console row scales its own time fields,
// and the statistic's own unit for a `stat:` name.
auto entryScaleOf(const BenchmarkResult* unitSource,
                  std::string_view valueName) -> const UnitScale*
{
  if (unitSource == nullptr) {
    return nullptr;
  }
  if (isTimeColumnName(valueName)) {
    return &unitScaleOf(unitSource->timeUnit);
  }
  if (const std::string_view prefix = labelPrefixOf(valueName);
      prefix == "stat:")
  {
    for (const auto& statistic : unitSource->statisticValues) {
      if (statistic.name == valueName.substr(prefix.size())) {
        return statistic.unit == StatUnit::TIME
            ? &unitScaleOf(unitSource->timeUnit)
            : nullptr;
      }
    }
  }
  return nullptr;
}

// One numeric cell of the table: the console's three-decimal field, a
// time value divided by the row's scale and followed by the unit label,
// and `n/a` where no value stands (FR-053, FR-055, D-18).
auto formatCell(const double value,
                const UnitScale* scale,
                const bool signedValue) -> std::string
{
  const double printed = scale == nullptr ? value : value / scale->divisor;
  char text[64];
  if (signedValue) {
    std::snprintf(text, sizeof text, "%+.3f", printed);
  } else {
    std::snprintf(text, sizeof text, "%.3f", printed);
  }
  std::string cell(text);
  if (scale != nullptr) {
    cell += ' ';
    cell += scale->label;
  }
  return cell;
}

// The comparison document emits through the vendored builder, the report
// writer's tool (FR-057, D-03).
using Builder = simdjson::builder::string_builder;

// The document version this writer stamps (E-10).
constexpr std::string_view kCompareSchemaVersion = "1.0";

// The builder emits no separator of its own, so every object and array of
// the document opens a scope that puts one comma before every entry after
// the first and closes the bracket the scope opened, the shape the report
// writer gives its own document (source/harness/json_report.cpp).
class CompareObjectWriter
{
public:
  explicit CompareObjectWriter(Builder& builder) noexcept
      : m_builder(builder)
  {
    m_builder.start_object();
  }

  CompareObjectWriter(const CompareObjectWriter&) = delete;
  CompareObjectWriter& operator=(const CompareObjectWriter&) = delete;

  ~CompareObjectWriter() noexcept { m_builder.end_object(); }

  auto key(std::string_view name) noexcept -> void
  {
    if (!m_first) {
      m_builder.append_comma();
    }
    m_first = false;
    m_builder.escape_and_append_with_quotes(name);
    m_builder.append_colon();
  }

private:
  Builder& m_builder;
  bool m_first = true;
};

class CompareArrayWriter
{
public:
  explicit CompareArrayWriter(Builder& builder) noexcept
      : m_builder(builder)
  {
    m_builder.start_array();
  }

  CompareArrayWriter(const CompareArrayWriter&) = delete;
  CompareArrayWriter& operator=(const CompareArrayWriter&) = delete;

  ~CompareArrayWriter() noexcept { m_builder.end_array(); }

  auto item() noexcept -> void
  {
    if (!m_first) {
      m_builder.append_comma();
    }
    m_first = false;
  }

private:
  Builder& m_builder;
  bool m_first = true;
};

auto put(Builder& builder,
         CompareObjectWriter& object,
         std::string_view key,
         std::string_view value) noexcept -> void
{
  object.key(key);
  builder.escape_and_append_with_quotes(value);
}

// A text the build hands over as a literal needs its own overload: the
// boolean overload below would otherwise win the overload set, the reason
// the report writer carries the same one.
auto put(Builder& builder,
         CompareObjectWriter& object,
         std::string_view key,
         const char* value) noexcept -> void
{
  put(builder, object, key, std::string_view {value});
}

auto put(Builder& builder,
         CompareObjectWriter& object,
         std::string_view key,
         bool value) noexcept -> void
{
  object.key(key);
  builder.append(value);
}

// An empty optional writes `null`, the unanswered form the report format
// already uses (E-10, 023 D-12).
auto putOrNull(Builder& builder,
               CompareObjectWriter& object,
               std::string_view key,
               const std::optional<double> value) noexcept -> void
{
  object.key(key);
  if (!value.has_value()) {
    builder.append_null();
    return;
  }
  builder.append(*value);
}

// One entry of the document: the eight keys of E-10, in the order the
// schema's `x-order` gives them.
void writeEntry(Builder& builder, const CompareEntry& entry)
{
  CompareObjectWriter object(builder);
  put(builder, object, "name", entry.name);
  put(builder, object, "rule", entry.ruleText);
  putOrNull(builder, object, "baseline", entry.baseline);
  putOrNull(builder, object, "contender", entry.contender);
  putOrNull(builder, object, "changePercent", entry.changePercent);
  putOrNull(builder, object, "changeAbsolute", entry.changeAbsolute);
  put(builder, object, "state", compareStateName(entry.state));
  put(builder, object, "reason", entry.reason);
}

// The rules one comparison applies: the list the caller gave, or the one
// default rule when it gave none (specs/026 FR-033, FR-034, spec D-5).
// The default is a text this unit's grammar reads, so the parse of FR-004
// stays the one source.
auto appliedRulesOf(const std::vector<CompareRule>& rules)
    -> std::vector<CompareRule>
{
  if (!rules.empty()) {
    return rules;
  }
  const auto fallback = parseCompareRule(detail::kDefaultCompareRule);
  SG_ASSERT(fallback.has_value(),
            "the default rule text reads as a rule (specs/026 FR-033)");
  return {*fallback};
}

// True when one entry of the list answers one state word (specs/026 E-03).
// The verdict of FR-044 and FR-045 reads through it in both the engine and
// the postcondition check of `compareReports`, so one rule states the rule.
auto hasState(const std::vector<CompareEntry>& entries,
              const CompareState state) -> bool
{
  return std::any_of(entries.begin(),
                     entries.end(),
                     [state](const CompareEntry& entry)
                     { return entry.state == state; });
}

// True when every entry of the list carries the text of one of the rules the
// call applied (specs/026 FR-002, FR-034, FR-049). A rule's filter decides
// which benchmarks the rule admits (FR-024), so the entry count follows the
// filters while the rule text of each entry stays one of the applied ones.
// The call site stands inside `SG_ENSURE`, so the contracts=ignore build
// leaves the helper unused.
[[maybe_unused]] auto namesAppliedRule(const std::vector<CompareEntry>& entries,
                                       const std::vector<CompareRule>& applied)
    -> bool
{
  return std::all_of(entries.begin(),
                     entries.end(),
                     [&applied](const CompareEntry& entry)
                     {
                       return std::any_of(applied.begin(),
                                          applied.end(),
                                          [&entry](const CompareRule& rule) {
                                            return rule.text == entry.ruleText;
                                          });
                     });
}

}  // namespace

auto parseCompareRule(std::string_view text)
    -> std::expected<CompareRule, std::string>
{
  SG_REQUIRE(text.find('\0') == std::string_view::npos,
             "parseCompareRule reaches a rule text that is not one whole text");
  auto parsed = readRule(text);
  if (parsed.has_value()) {
    SG_ENSURE(std::string_view(parsed->text) == text
                  && (parsed->relativeThreshold.has_value()
                      || parsed->absoluteThreshold.has_value()),
              "parseCompareRule answers a rule carrying the text it read and "
              "at least one threshold");
  }
  return parsed;
}

auto compareReports(const ReportData& baseline,
                    const ReportData& contender,
                    const std::vector<CompareRule>& rules,
                    const CompareOptions& options)
    -> std::expected<CompareResult, std::string>
{
  // E-07: the warning records index the results by position, which is how
  // the noise guard reaches them.
  SG_REQUIRE(
      baseline.noiseWarnings.empty()
          || baseline.noiseWarnings.size() == baseline.results.size(),
      "a report's noise warnings stand beside its results " "(specs/026 E-07)");
  SG_REQUIRE(
      contender.noiseWarnings.empty()
          || contender.noiseWarnings.size() == contender.results.size(),
      "a report's noise warnings stand beside its results " "(specs/026 E-07)");
  const std::vector<CompareRule> applied = appliedRulesOf(rules);
  const auto answer =
      detail::compareReportValues(baseline, contender, applied, options.strict);
  // FR-003, FR-044, FR-045: a fault answers one text naming it, and a value
  // answers the verdict its entries state beside entries that each carry the
  // text of one applied rule. A rule's filter decides which benchmarks the
  // rule admits (FR-024), so the entry count follows the filters and the doc
  // block carries that claim.
  SG_ENSURE(!answer.has_value()
                ? !answer.error().empty()
                : namesAppliedRule(answer->entries, applied)
                      && answer->passed
                             == (!hasState(answer->entries,
                                           CompareState::REGRESSED)
                                 && (!options.strict
                                     || !hasState(answer->entries,
                                                  CompareState::NOISY))),
            "compareReports answers entries that each carry an applied rule "
            "text with the verdict of FR-044 and FR-045, or one text naming "
            "the fault (specs/026 FR-002, FR-003, FR-044, FR-045)");
  return answer;
}

namespace detail
{

/**
 * @brief The row the comparison reads one result's values from
 * (specs/026 FR-032, D-09, spec D-20).
 *
 * \pre `result` is a result the reader built, or one built to the same shape
 * \post the aggregate row when the result carries one, the single measured
 *       row when it carries none, and nothing when the result holds no row
 *       a rule can read: a non-measured outcome, no row at all, or only the
 *       rows of a complexity fit
 */
auto selectCompareRow(const BenchmarkResult& result) -> const ResultRow*
{
  if (result.outcome != RunOutcome::MEASURED) {
    return nullptr;
  }
  for (const auto& row : result.rows) {
    if (row.kind == RowKind::AGGREGATE) {
      return &row;
    }
  }
  const ResultRow* single = nullptr;
  for (const auto& row : result.rows) {
    if (row.kind != RowKind::REPETITION) {
      continue;
    }
    if (single != nullptr) {
      return nullptr;
    }
    single = &row;
  }
  return single;
}

/**
 * @brief The value one rule names in one result (specs/026 E-08, FR-013,
 * FR-014, FR-015, FR-016, FR-017, D-09, D-10, spec D-18).
 *
 * \pre `rule` is a rule the parse answered, so its value name carries one of
 *      the seven forms and no aggregate suffix on a single-value name
 * \post the value in the stored unit of the name, and nothing when the
 *       result holds no readable row or the name matches no value of it; the
 *       call reads no file, runs no benchmark and repairs no value
 */
auto resolveCompareValue(const BenchmarkResult& result,
                         const CompareRule& rule) -> std::optional<double>
{
  const ResultRow* const row = selectCompareRow(result);
  if (row == nullptr) {
    return std::nullopt;
  }
  const ValueSite site = valueSiteOf(result, *row, rule.valueName);
  // FR-017, D-09, spec D-18: a run that carries no aggregate row answers
  // every aggregate with the value of its one row, and a name that takes no
  // aggregate suffix - a percentile cell, a custom statistic - reads the
  // value of the row it was chosen from.
  if (row->kind != RowKind::AGGREGATE || !rule.aggregate.has_value()) {
    return site.value;
  }
  // FR-016, FR-032: over a repeated run the aggregate reads the record the
  // report carries for its own value - `timeStats` for the primary column,
  // `metricStats` at the metric label's index, `counterStats` at the counter
  // label's index - and never a column of the aggregate row.
  // spec D-18: the report holds a record for those three columns only, so an
  // aggregate named on a column it summarizes nowhere is a missing value
  // (FR-036).
  if (site.stats == nullptr) {
    return std::nullopt;
  }
  return aggregateValueOf(*site.stats, *rule.aggregate);
}

/**
 * @brief The threshold arithmetic and the verdict words for one matched
 * pair of values (specs/026 E-09, FR-025 to FR-031, spec D-4, D-18,
 * spec D-22).
 *
 * The noise guard of D-11 marks NOISY before this call answers its word;
 * the guard is a separate step, so this call reads the two values alone.
 *
 * \pre `rule` is a rule the parse answered, so it carries at least one
 *      threshold, and the two values stand in the stored unit of the
 *      rule's value name (spec D-22)
 * \post the entry carries the name, the rule text, the two values, the
 *       signed change as a percent of the baseline magnitude and in the
 *       stored unit, and the state word: IMPROVED for a delta below zero,
 *       REGRESSED when every threshold the rule carries is exceeded, and
 *       OK for a delta that clears them (FR-029, FR-030); the percent
 *       stands empty exactly when the baseline magnitude is zero, the
 *       `n/a` of D-18
 * \post the answer of a zero baseline under a relative threshold is one
 *       fault line naming the benchmark and the value (FR-031)
 */
auto compareValues(const CompareRule& rule,
                   std::string_view name,
                   const double baseline,
                   const double contender)
    -> std::expected<CompareEntry, std::string>
{
  SG_REQUIRE(
      rule.relativeThreshold.has_value() || rule.absoluteThreshold.has_value(),
      "a rule the parse answers carries a threshold (specs/026 FR-022)");
  // FR-025, spec D-4: the delta runs forward for a lower-is-better value
  // and backward for a higher-is-better one, so a positive delta is worse
  // and the sign of the change agrees with the state word.
  const double delta = rule.direction == Direction::LOWER_IS_BETTER
      ? contender - baseline
      : baseline - contender;
  const double magnitude = std::abs(baseline);
  // The house rule against comparing a double with `==`: the magnitude is
  // zero exactly when it answers no positive number.
  const bool zeroBaseline = !(magnitude > 0.0);
  // FR-031: a percent of a zero baseline has no value, so a rule that
  // carries the relative test measures this pair not at all.
  if (zeroBaseline && rule.relativeThreshold.has_value()) {
    return std::unexpected("the rule '" + detail::quotedText(rule.text)
                           + "' cannot measure " + std::string(name)
                           + ": the baseline of " + rule.valueName
                           + " is zero");
  }
  // FR-026, FR-027, FR-028, spec D-4: every test the rule carries must be
  // exceeded, so with both thresholds the row regresses only when both are
  // and an absolute floor stops a large relative change on a tiny value.
  const bool relativeExceeded = !rule.relativeThreshold.has_value()
      || delta / magnitude > *rule.relativeThreshold;
  const bool absoluteExceeded =
      !rule.absoluteThreshold.has_value() || delta > *rule.absoluteThreshold;

  CompareEntry entry;
  entry.name = std::string(name);
  entry.ruleText = rule.text;
  entry.baseline = baseline;
  entry.contender = contender;
  // D-18: the percent of a zero baseline answers nothing, which the table
  // prints as `n/a` and the document writes as `null`; the absolute test
  // answers alone.
  if (!zeroBaseline) {
    entry.changePercent = delta / magnitude * 100.0;
  }
  entry.changeAbsolute = delta;
  entry.state = delta < 0.0 ? CompareState::IMPROVED
      : (delta > 0.0 && relativeExceeded && absoluteExceeded)
      ? CompareState::REGRESSED
      : CompareState::OK;
  return entry;
}

/**
 * @brief The noise account one report carries for the value a rule names
 * (specs/026 D-11, FR-042, FR-043).
 */
struct NoiseAccount
{
  const Statistics* stats = nullptr;
  std::optional<double> runningRatio;
};

/**
 * @brief The noise account of the value `rule` names in `result`.
 *
 * \pre `rule` is a rule the parse answered
 * \post the statistics record of the row the comparison reads and the
 *       running ratio, which stands only for a metric column; both stand
 *       empty when the result holds no readable row, the name matches no
 *       value of it, or the report summarizes that column nowhere (spec
 *       D-18, FR-046)
 */
auto noiseAccountOf(const BenchmarkResult& result,
                    const CompareRule& rule) -> NoiseAccount
{
  const ResultRow* const row = selectCompareRow(result);
  if (row == nullptr) {
    return {};
  }
  NoiseAccount account;
  account.stats = valueSiteOf(result, *row, rule.valueName).stats;
  // FR-043: only a metric column carries a running ratio, and only the
  // guard reads it, so it stands beside the value site, in the account.
  if (const std::string_view prefix = labelPrefixOf(rule.valueName);
      prefix == "metric:")
  {
    const std::size_t index =
        labelIndexOf(result.metricLabels, rule.valueName.substr(prefix.size()));
    if (index < result.metricLabels.size() && index < row->metrics.size()) {
      account.runningRatio = row->metrics[index].runningRatio;
    }
  }
  return account;
}

// One number of the guard's account, printed in the shortest form that
// reads back the value: the account form `printNoiseWarning` prints beside
// the same record at source/harness/report.cpp:193.
auto reasonNumber(const double value) -> std::string
{
  char text[32];
  std::snprintf(text, sizeof text, "%g", value);
  return text;
}

// The account of one carried warning: the two counts the warning is made
// of, in the order and the field form of that same line.
auto reasonAccount(const NoiseWarning& warning) -> std::string
{
  char text[64];
  std::snprintf(text,
                sizeof text,
                " (%llu spin, %llu cpus)",
                static_cast<unsigned long long>(warning.spinning),
                static_cast<unsigned long long>(warning.runnableCpus));
  return text;
}

/**
 * @brief The noise guard of D-11 for one matched pair of results
 * (specs/026 E-09, FR-041, FR-042, FR-043, FR-046, spec D-19).
 *
 * The guard runs before the verdict words and reads only what the two
 * reports already carry: the warning the document carried, the value's
 * own statistics record held against the rule's two thresholds, and the
 * running ratio of a metric column. It computes no statistic and reads
 * no file, clock or sensor.
 *
 * \pre `rule` is a rule the parse answered, so it carries at least one
 *      threshold; each warning pointer stands null when the report of
 *      that side carried none for this benchmark
 * \post nothing when no condition fires; otherwise one text naming every
 *       condition that fired, separated by `; `, each with the numbers it
 *       read: the carried warning of either report by itself with its
 *       `spinning` and `runnableCpus` counts, `coefficientOfVariation`
 *       over the relative threshold with both numbers, `sampleStdDev`
 *       over the absolute threshold with both numbers in the stored unit
 *       of the value, either report over its bound being enough, and a
 *       `runningRatio` below one in either report with the ratio (FR-044,
 *       FR-045, E-02, SC-006)
 * \post a value the report summarizes nowhere - a percentile cell or a
 *       custom statistic - is guarded through the carried warning and the
 *       running ratio alone (FR-046)
 */
auto noiseGuard(const CompareRule& rule,
                const BenchmarkResult& baseline,
                const BenchmarkResult& contender,
                const NoiseWarning* baselineWarning,
                const NoiseWarning* contenderWarning)
    -> std::optional<std::string>
{
  SG_REQUIRE(
      rule.relativeThreshold.has_value() || rule.absoluteThreshold.has_value(),
      "a rule the parse answers carries a threshold (specs/026 FR-022)");
  std::string reason;
  // Each condition names itself, so the table answers why (SC-006).
  const auto mark = [&reason](const std::string_view text)
  {
    if (!reason.empty()) {
      reason += "; ";
    }
    reason += text;
  };
  // FR-041: the carried warning marks the row by itself, and the reason
  // carries the two counts the warning is made of.
  if (baselineWarning != nullptr) {
    mark("the baseline report carries a noise warning"
         + reasonAccount(*baselineWarning));
  }
  if (contenderWarning != nullptr) {
    mark("the contender report carries a noise warning"
         + reasonAccount(*contenderWarning));
  }
  // The two reports hold equal standing: either one over its bound marks
  // the row (spec D-19).
  const auto guardReport =
      [&](const std::string_view label, const BenchmarkResult& result)
  {
    const NoiseAccount account = noiseAccountOf(result, rule);
    // FR-042, spec D-19: the two fields of the value's own record, each
    // against the threshold it matches, the standard deviation in the
    // stored unit of the value (spec D-22). A value the report summarizes
    // nowhere holds no record and answers these two not at all (FR-046).
    if (rule.relativeThreshold.has_value() && account.stats != nullptr
        && account.stats->coefficientOfVariation > *rule.relativeThreshold)
    {
      mark(std::string(label)
           + " coefficient of variation exceeds the relative threshold ("
           + reasonNumber(account.stats->coefficientOfVariation) + " against "
           + reasonNumber(*rule.relativeThreshold) + ")");
    }
    if (rule.absoluteThreshold.has_value() && account.stats != nullptr
        && account.stats->sampleStdDev > *rule.absoluteThreshold)
    {
      mark(std::string(label)
           + " sample standard deviation exceeds the absolute threshold ("
           + reasonNumber(account.stats->sampleStdDev) + " against "
           + reasonNumber(*rule.absoluteThreshold) + ")");
    }
    // FR-043: a metric whose ratio sits below one was multiplexed.
    if (account.runningRatio.has_value() && *account.runningRatio < 1.0) {
      mark(std::string(label) + " metric running ratio sits below one ("
           + reasonNumber(*account.runningRatio) + ")");
    }
  };
  guardReport("baseline", baseline);
  guardReport("contender", contender);
  if (reason.empty()) {
    return std::nullopt;
  }
  return reason;
}

/**
 * @brief One benchmark of the pair, with the result each report holds for
 * it and the warning that report carried for it (specs/026 FR-008, FR-009,
 * E-07).
 *
 * One side stands null exactly when the name is present in the other report
 * alone, which is the ADDED and REMOVED pair of FR-009.
 */
struct PairSite
{
  std::string_view name;
  const BenchmarkResult* baseline = nullptr;
  const BenchmarkResult* contender = nullptr;
  const NoiseWarning* baselineWarning = nullptr;
  const NoiseWarning* contenderWarning = nullptr;
};

/**
 * @brief One benchmark of one rule's scope, as the validation walk leaves
 * it for the comparison (specs/026 FR-035, E-09).
 *
 * \post `comparable` stands false exactly when the pair answers a state
 *       word with no values: a name one report holds alone, or a matched
 *       benchmark whose rows hold no readable row
 *       on either side (FR-009, FR-011, spec D-20)
 */
struct ScopedValue
{
  std::size_t siteIndex = 0;
  bool comparable = false;
  double baseline = 0.0;
  double contender = 0.0;
};

// The index of the result carrying the full instance name `name`, or the
// end of the list when none does. A report holds a handful of benchmarks,
// so the match scans the list directly (FR-008).
auto indexOfResult(const std::vector<BenchmarkResult>& results,
                   std::string_view name) -> std::size_t
{
  const auto found = std::find_if(results.begin(),
                                  results.end(),
                                  [name](const BenchmarkResult& result)
                                  { return result.name == name; });
  return static_cast<std::size_t>(found - results.begin());
}

// The warning one report carried for one result, or nothing when it
// carried none for it (E-07, FR-070).
auto noiseWarningAt(const ReportData& report,
                    const std::size_t index) -> const NoiseWarning*
{
  if (index >= report.noiseWarnings.size()
      || !report.noiseWarnings[index].has_value())
  {
    return nullptr;
  }
  return &*report.noiseWarnings[index];
}

/**
 * @brief The pair matched by the full instance name (specs/026 FR-002,
 * FR-008, FR-009, FR-010).
 *
 * \pre `baseline` and `contender` hold no two results under one name, which
 *      is what the registry guarantees for a run and the reader for a
 *      document
 * \post one site per name of either report, the baseline's run order first
 *       and the names the contender adds after them in the contender's own
 *       order, whichever order the two documents hold their benchmarks in
 */
auto matchPair(const ReportData& baseline,
               const ReportData& contender) -> std::vector<PairSite>
{
  std::vector<PairSite> matched;
  matched.reserve(baseline.results.size() + contender.results.size());
  for (std::size_t index = 0; index < baseline.results.size(); ++index) {
    const std::size_t other =
        indexOfResult(contender.results, baseline.results[index].name);
    const bool paired = other < contender.results.size();
    matched.push_back(
        {.name = baseline.results[index].name,
         .baseline = &baseline.results[index],
         .contender = paired ? &contender.results[other] : nullptr,
         .baselineWarning = noiseWarningAt(baseline, index),
         .contenderWarning =
             paired ? noiseWarningAt(contender, other) : nullptr});
  }
  for (std::size_t index = 0; index < contender.results.size(); ++index) {
    if (indexOfResult(baseline.results, contender.results[index].name)
        < baseline.results.size())
    {
      continue;
    }
    matched.push_back({.name = contender.results[index].name,
                       .contender = &contender.results[index],
                       .contenderWarning = noiseWarningAt(contender, index)});
  }
  return matched;
}

// The fault text of the missing-value and uncountable-metric rows of the
// table of specs/026-regression-gate/contracts/compare-call.md, naming the
// rule, the benchmark and the report (FR-036, FR-037, FR-038).
auto missingValueFault(const CompareRule& rule,
                       std::string_view name,
                       std::string_view report,
                       const bool uncountable) -> std::string
{
  return "the rule '" + detail::quotedText(rule.text) + "' finds no "
      + (uncountable ? "countable " : "") + rule.valueName + " for "
      + detail::quotedText(name) + " in the " + std::string(report) + " report";
}

// The availability of the metric column a value name reads in one row,
// COUNTABLE for a name that reads no metric column (FR-037). The aggregate
// row of a repeated run carries the metric's record and no metric column,
// so a name whose value came from that record has no column availability
// to disclose and answers COUNTABLE (FR-016, FR-032, FR-037).
auto metricAvailabilityOf(const BenchmarkResult& result,
                          const ResultRow& row,
                          std::string_view name) -> counters::Availability
{
  const std::string_view prefix = labelPrefixOf(name);
  if (prefix != "metric:") {
    return counters::Availability::COUNTABLE;
  }
  const std::size_t index =
      labelIndexOf(result.metricLabels, name.substr(prefix.size()));
  if (index >= row.metrics.size()) {
    return counters::Availability::COUNTABLE;
  }
  return row.metrics[index].availability;
}

// The value one jitter rule names in one report (specs/027 FR-030 to
// FR-032): the entry's per-benchmark stats for a bare name, the
// run-level record for a `jitter:run:` name, the worst cpu for `any`,
// the named cpu's own value otherwise. A report that carries no stats
// where the rule looks, a probe-free report included, is the usage
// error of FR-032: the fault the comparison run prints as its one line
// at status 2.
auto jitterValueOf(const std::optional<std::vector<JitterEntryFold>>& runStats,
                   const BenchmarkResult& entry,
                   const JitterNameParts& parts,
                   const CompareRule& rule,
                   std::string_view name,
                   std::string_view reportLabel)
    -> std::expected<double, std::string>
{
  const std::vector<JitterEntryFold> noStats;
  const std::vector<JitterEntryFold>& stats = parts.runScoped
      ? (runStats.has_value() ? *runStats : noStats)
      : entry.jitterFolds;
  if (!parts.worstCpu) {
    const auto found = std::find_if(stats.begin(),
                                    stats.end(),
                                    [parts](const JitterEntryFold& fold)
                                    { return fold.cpu == parts.cpu; });
    if (found == stats.end()) {
      return std::unexpected(
          "the rule '" + rule.text + "' names cpu " + std::to_string(parts.cpu)
          + " and the jitter stats of " + std::string(name) + " in the "
          + std::string(reportLabel) + " report do not carry it");
    }
    return kJitterFields.at(parts.field).value(found->stats);
  }
  if (stats.empty()) {
    return std::unexpected(
        "the rule '" + rule.text + "' names the worst cpu and the jitter "
        "stats of " + std::string(name) + " in the "
        + std::string(reportLabel) + " report carry no probe cpu");
  }
  // FR-030, D-11: the worst cpu is the one carrying the largest value,
  // every field of `JitterStats` alike, so the maximum is uniform.
  double worst = 0.0;
  for (const auto& fold : stats) {
    worst = std::max(worst, kJitterFields.at(parts.field).value(fold.stats));
  }
  return worst;
}

/**
 * @brief The walk of one rule over its scope, before any value is compared
 * (specs/026 FR-035, FR-036, FR-037, FR-038, FR-039, FR-040, spec D-20).
 *
 * \pre `rule` is a rule `parseCompareRule` answered, or a value built to the
 *      same shape (D-08); a filter the regex library refuses answers the
 *      fault below, so the walk throws nothing (FR-051)
 * \post the scoped benchmarks of `matched` in that same order, each carrying
 *       the two values the walk read for a pair both reports hold with a
 *       readable row, and nothing else (FR-011, spec D-20)
 * \post one fault text, in the order the table of
 *       `contracts/compare-call.md` lists them, for a filter the regex
 *       library refuses, a filter that selects nothing, a value one report
 *       lacks, a metric that is not countable, or a scope of paired
 *       benchmarks none of which holds a readable row
 */
auto scopedValuesOf(const ReportData& baseline,
                    const ReportData& contender,
                    const std::vector<PairSite>& matched,
                    const CompareRule& rule)
    -> std::expected<std::vector<ScopedValue>, std::string>
{
  std::optional<std::regex> filter;
  if (!rule.filter.empty()) {
    try {
      filter.emplace(rule.filter);
    } catch (const std::regex_error& error) {
      return std::unexpected("the rule '" + detail::quotedText(rule.text)
                             + "' carries a filter that is no regular "
                               "expression: "
                             + detail::quotedText(error.what()));
    }
  }
  std::vector<ScopedValue> scope;
  for (std::size_t index = 0; index < matched.size(); ++index) {
    if (filter.has_value()
        && !std::regex_search(
            matched[index].name.begin(), matched[index].name.end(), *filter))
    {
      continue;
    }
    scope.push_back({.siteIndex = index});
  }
  // FR-039: a written filter that matches no benchmark in either report.
  if (filter.has_value() && scope.empty()) {
    return std::unexpected("the rule '" + detail::quotedText(rule.text)
                           + "' selects no benchmark: its filter '"
                           + detail::quotedText(rule.filter)
                           + "' matches neither report");
  }
  // The parts of a jitter value name, read once for the whole walk. The
  // parse validated the name the rule carries, so a rule built in code
  // with a name the grammar does not carry answers the same fault here
  // (D-08, specs/027 FR-030).
  std::optional<JitterNameParts> jitterParts;
  if (rule.valueName.starts_with(kJitterPrefix)) {
    const auto parts =
        readJitterName(rule.valueName.substr(kJitterPrefix.size()));
    if (!parts.has_value()) {
      return std::unexpected("the rule '" + rule.text
                             + "' names a jitter value that is not "
                               "readable: "
                             + parts.error());
    }
    jitterParts = parts->first;
  }
  std::size_t comparable = 0;
  std::string_view firstPaired;
  for (ScopedValue& slot : scope) {
    const PairSite& site = matched[slot.siteIndex];
    if (site.baseline == nullptr || site.contender == nullptr) {
      continue;
    }
    firstPaired = site.name;
    const ResultRow* const baselineRow = selectCompareRow(*site.baseline);
    const ResultRow* const contenderRow = selectCompareRow(*site.contender);
    if (baselineRow == nullptr || contenderRow == nullptr) {
      continue;
    }
    const auto readValue = [&](const BenchmarkResult& result,
                               const ResultRow& row,
                               const ReportData& report,
                               const std::string_view reportLabel)
        -> std::expected<double, std::string>
    {
      // specs/027 FR-030 to FR-032: a jitter name reads the stats the
      // report carries for it, with the usage errors of FR-032.
      if (jitterParts.has_value()) {
        return jitterValueOf(report.runJitterStats,
                             result,
                             *jitterParts,
                             rule,
                             site.name,
                             reportLabel);
      }
      const auto read = resolveCompareValue(result, rule);
      if (!read.has_value()) {
        return std::unexpected(
            missingValueFault(rule, site.name, reportLabel, false));
      }
      // FR-037: a metric that is not countable counts as a missing value.
      if (metricAvailabilityOf(result, row, rule.valueName)
          != counters::Availability::COUNTABLE)
      {
        return std::unexpected(
            missingValueFault(rule, site.name, reportLabel, true));
      }
      return *read;
    };
    const auto baselineValue =
        readValue(*site.baseline, *baselineRow, baseline, "baseline");
    if (!baselineValue.has_value()) {
      return std::unexpected(baselineValue.error());
    }
    const auto contenderValue =
        readValue(*site.contender, *contenderRow, contender, "contender");
    if (!contenderValue.has_value()) {
      return std::unexpected(contenderValue.error());
    }
    ++comparable;
    slot.comparable = true;
    slot.baseline = *baselineValue;
    slot.contender = *contenderValue;
  }
  // FR-040, spec D-20: a scope holding paired benchmarks and none of them
  // a readable row. A scope holding no pair at all is the empty pair of
  // D-19, which answers no entry and no fault.
  if (comparable == 0 && !firstPaired.empty()) {
    return std::unexpected(
        "the rule '" + detail::quotedText(rule.text)
        + "' reaches no comparable row: " + detail::quotedText(firstPaired)
        + " holds no row the rule can read in either report");
  }
  return scope;
}

// The entry one benchmark of one rule's scope answers, or the zero-baseline
// fault of the contract's table that only the comparison reads (FR-009,
// FR-011, FR-031, D-11).
auto entryFor(const PairSite& site,
              const CompareRule& rule,
              const ScopedValue& slot)
    -> std::expected<CompareEntry, std::string>
{
  CompareEntry entry;
  entry.name = std::string(site.name);
  entry.ruleText = rule.text;
  if (!slot.comparable) {
    // FR-009, FR-011: the word alone, with no values.
    entry.state = site.baseline == nullptr
        ? CompareState::ADDED
        : (site.contender == nullptr ? CompareState::REMOVED
                                     : CompareState::SKIPPED);
    return entry;
  }
  auto measured = compareValues(rule, site.name, slot.baseline, slot.contender);
  if (!measured.has_value()) {
    return std::unexpected(measured.error());
  }
  // D-11: the guard marks the row before the verdict words answer, and the
  // reason names what fired (FR-041 to FR-044).
  if (const auto reason = noiseGuard(rule,
                                     *site.baseline,
                                     *site.contender,
                                     site.baselineWarning,
                                     site.contenderWarning);
      reason.has_value())
  {
    measured->state = CompareState::NOISY;
    measured->reason = *reason;
  }
  return measured;
}

/**
 * @brief The comparison core: the version pairing, the matching and the
 * validation walk of `contracts/compare-call.md` (specs/026 FR-002, FR-003,
 * FR-008 to FR-011, FR-035 to FR-040, FR-051, D-13, D-19, spec D-20).
 *
 * The public `compareReports` of T018 wraps this call; the default rule of
 * FR-033 stands in that call, so an empty rule list here answers no entry.
 *
 * \pre `baseline` and `contender` are values `loadReport` answered, or
 *      values built to the same shape: `noiseWarnings`, when non-empty,
 *      holds one entry per entry of `results`, and `schemaVersion` carries
 *      the major and the minor the document carried
 * \pre every rule of `rules` is a rule `parseCompareRule` answered
 * \post the entries hold one entry per benchmark of the pair that the rule's
 *       filter admits, per rule, the baseline's run order first and the names
 *       the contender adds after them, and `passed` states the verdict
 *       (FR-002, FR-024, FR-044, FR-045)
 * \post when the call answers an error, the text names what the first fault
 *       of the walk names and no partial entry reaches the caller (FR-003)
 * \post a pair sharing no benchmark name answers an empty entry list and
 *       passes (D-19)
 * \post the call reads no file, runs no benchmark, reads no clock, samples
 *       nothing, repairs no value and prints nothing (FR-006, FR-007)
 */
auto compareReportValues(const ReportData& baseline,
                         const ReportData& contender,
                         const std::vector<CompareRule>& rules,
                         const bool strict)
    -> std::expected<CompareResult, std::string>
{
  // D-13, FR-051, SC-013: the pairing of the two majors stands first, so a
  // library caller answers the fault the command line prints as its one
  // line. The minor stands outside the pairing, so a 1.0 report compares
  // against a 1.1 one; a text that states no major holds no major to share.
  const auto baselineMajor = schemaMajor(baseline.schemaVersion);
  const auto contenderMajor = schemaMajor(contender.schemaVersion);
  if (!baselineMajor || baselineMajor != contenderMajor) {
    return std::unexpected(
        "the two reports carry no common schema major version: '"
        + detail::quotedText(baseline.schemaVersion) + "' against '"
        + detail::quotedText(contender.schemaVersion) + "'");
  }
  const std::vector<PairSite> matched = matchPair(baseline, contender);
  // FR-035: every rule walks its whole scope before any value is compared.
  std::vector<std::vector<ScopedValue>> scopes;
  scopes.reserve(rules.size());
  for (const CompareRule& rule : rules) {
    auto scope = scopedValuesOf(baseline, contender, matched, rule);
    if (!scope.has_value()) {
      return std::unexpected(scope.error());
    }
    scopes.push_back(std::move(*scope));
  }
  CompareResult result;
  const bool paired = std::any_of(
      matched.begin(),
      matched.end(),
      [](const PairSite& site)
      { return site.baseline != nullptr && site.contender != nullptr; });
  // D-19: a pair sharing no benchmark name matched nothing, which answers
  // no entry at all.
  if (paired) {
    // FR-002: the entries of one benchmark stand together, so the added
    // names follow the baseline's run order.
    std::vector<std::vector<CompareEntry>> bySite(matched.size());
    for (std::size_t ruleIndex = 0; ruleIndex < rules.size(); ++ruleIndex) {
      for (const ScopedValue& slot : scopes[ruleIndex]) {
        auto entry = entryFor(matched[slot.siteIndex], rules[ruleIndex], slot);
        if (!entry.has_value()) {
          return std::unexpected(entry.error());
        }
        bySite[slot.siteIndex].push_back(std::move(*entry));
      }
    }
    for (std::vector<CompareEntry>& siteEntries : bySite) {
      for (CompareEntry& entry : siteEntries) {
        result.entries.push_back(std::move(entry));
      }
    }
  }
  // FR-044, FR-045: a regression fails the verdict, and strict moves it for
  // a NOISY row without changing that row's word.
  result.passed = !hasState(result.entries, CompareState::REGRESSED)
      && (!strict || !hasState(result.entries, CompareState::NOISY));
  return result;
}

// D-12, FR-050: the batch filter scopes the two loaded values before the
// comparison, keeping the four position-indexed lists of ReportData paired
// the way the load call left them (specs/023 E-01, specs/026 E-07).
auto applyBatchFilter(ReportData& report, const std::regex& pattern) -> void
{
  std::size_t kept = 0;
  for (std::size_t index = 0; index < report.results.size(); ++index) {
    if (!std::regex_search(report.results[index].name, pattern)) {
      continue;
    }
    if (kept != index) {
      report.results[kept] = std::move(report.results[index]);
      report.rowRaw[kept] = std::move(report.rowRaw[index]);
      report.contextRaw[kept] = std::move(report.contextRaw[index]);
      report.noiseWarnings[kept] = std::move(report.noiseWarnings[index]);
    }
    ++kept;
  }
  report.results.resize(kept);
  report.rowRaw.resize(kept);
  report.contextRaw.resize(kept);
  report.noiseWarnings.resize(kept);
}

auto runCompareMode(const RunOptions& options) -> CompareExit
{
  // FR-006: the two documents are read through the load call and nothing
  // else; a rejected document answers one line naming the file and the
  // reader's fault, at status 2 (FR-051).
  auto baseline = loadReport(options.compareBaseline);
  if (!baseline.has_value()) {
    std::fprintf(stderr,
                 "error: the compare document '%s' was rejected: %s\n",
                 detail::quotedText(options.compareBaseline).c_str(),
                 baseline.error().c_str());
    return {.status = 2, .failed = true};
  }
  auto contender = loadReport(options.compareContender);
  if (!contender.has_value()) {
    std::fprintf(stderr,
                 "error: the compare document '%s' was rejected: %s\n",
                 detail::quotedText(options.compareContender).c_str(),
                 contender.error().c_str());
    return {.status = 2, .failed = true};
  }
  // D-12, FR-050: `--filter` scopes the loaded values. The command line
  // compiled the same pattern before the mode was called and answered an
  // invalid one with the run path's line and status 2 (cli.cpp, the scan),
  // so the pattern stands here (contracts/compare-options.md).
  if (!options.filter.empty()) {
    const std::regex pattern(options.filter);
    applyBatchFilter(*baseline, pattern);
    applyBatchFilter(*contender, pattern);
  }
  // FR-033: an empty rule list is the default rule, which the public call
  // stands in; the mode hands the parsed list through unchanged.
  const CompareOptions compareOptions {.strict = options.compareStrict};
  const auto comparison = compareReports(
      *baseline, *contender, options.compareRules, compareOptions);
  if (!comparison.has_value()) {
    // FR-003: a fault answers one line and no table; the text already
    // names the rule, the benchmark and the report, or the two versions
    // (FR-036 to FR-040, FR-031, FR-051).
    std::fprintf(stderr, "error: %s\n", comparison.error().c_str());
    return {.status = 2, .failed = true};
  }
  // FR-057, FR-074: `--json=` names the comparison document, and it is
  // written before anything prints, so a file system refusal answers the one
  // line naming the path beside no table, the form the report write already
  // answers (specs/015-benchmark-harness-core/contracts/cli.md:86). The
  // refusal line itself stands in `writeCompareReport`.
  if (!options.jsonPath.empty()
      && !writeCompareReport(options.jsonPath,
                             *comparison,
                             options.compareRules,
                             options.compareBaseline,
                             options.compareContender))
  {
    return {.status = 2, .failed = true};
  }
  if (comparison->entries.empty()) {
    // D-19: a pair the filter leaves with no shared name matched nothing;
    // one line says so and no table stands.
    std::printf("compare: the two reports matched nothing\n");
  } else {
    printCompareTable(*comparison,
                      *baseline,
                      *contender,
                      options.compareBaseline,
                      options.compareContender);
  }
  // FR-052: the status comes from the table of contracts/cli.md and
  // `failed` from the verdict, each named here so the closing SG_ENSURE
  // of the compare branch carries content (D-15).
  if (!comparison->passed) {
    return {.status = 1, .failed = true};
  }
  return {.status = 0, .failed = false};
}

auto printCompareTable(const CompareResult& result,
                       const ReportData& baseline,
                       const ReportData& contender,
                       const std::string& baselinePath,
                       const std::string& contenderPath) -> void
{
  // FR-056: the header names the two files and, for each report, the two
  // texts the load call read.
  std::printf("compare: baseline %s (speedgun %s, schema %s)\n",
              detail::quotedText(baselinePath).c_str(),
              detail::quotedText(baseline.speedgunVersion).c_str(),
              detail::quotedText(baseline.schemaVersion).c_str());
  std::printf("compare: contender %s (speedgun %s, schema %s)\n",
              detail::quotedText(contenderPath).c_str(),
              detail::quotedText(contender.speedgunVersion).c_str(),
              detail::quotedText(contender.schemaVersion).c_str());
  // FR-053: the seven columns of one row. The name field takes the
  // console report's width, so a long instance name overflows its field
  // the way a report row does.
  std::printf("%-24s %-32s %15s %15s %11s %15s  %s\n",
              "benchmark",
              "rule",
              "baseline",
              "contender",
              "change%",
              "change",
              "state");
  for (const CompareEntry& entry : result.entries) {
    // FR-055: the unit is the one the baseline report declares for this
    // benchmark; a name only the contender holds has no baseline result,
    // so its own declares the unit of the `n/a` fields it prints.
    const BenchmarkResult* unitSource = nullptr;
    const std::size_t baselineIndex =
        indexOfResult(baseline.results, entry.name);
    if (baselineIndex < baseline.results.size()) {
      unitSource = &baseline.results[baselineIndex];
    } else {
      const std::size_t contenderIndex =
          indexOfResult(contender.results, entry.name);
      if (contenderIndex < contender.results.size()) {
        unitSource = &contender.results[contenderIndex];
      }
    }
    // The entry carries the rule text and not the value name, so the unit
    // rule reads the text back through the same parse (D-08).
    const auto rule = parseCompareRule(entry.ruleText);
    const UnitScale* scale =
        rule.has_value() ? entryScaleOf(unitSource, rule->valueName) : nullptr;
    const std::string baselineCell = entry.baseline.has_value()
        ? formatCell(*entry.baseline, scale, false)
        : std::string("n/a");
    const std::string contenderCell = entry.contender.has_value()
        ? formatCell(*entry.contender, scale, false)
        : std::string("n/a");
    const std::string absoluteCell = entry.changeAbsolute.has_value()
        ? formatCell(*entry.changeAbsolute, scale, true)
        : std::string("n/a");
    // D-18: the percent of a zero baseline answers nothing, and prints the
    // token `n/a`.
    std::string percentCell("n/a");
    if (entry.changePercent.has_value()) {
      char text[64];
      std::snprintf(text, sizeof text, "%+.3f%%", *entry.changePercent);
      percentCell = text;
    }
    // FR-030: the name and the rule text stand quoted once, at the
    // print site, so a cell that carries a separator or a quote reads
    // back as one cell; the reason cell stands unquoted.
    std::printf("%-24s %-32s %15s %15s %11s %15s  %s",
                detail::quotedText(entry.name).c_str(),
                detail::quotedText(entry.ruleText).c_str(),
                baselineCell.c_str(),
                contenderCell.c_str(),
                percentCell.c_str(),
                absoluteCell.c_str(),
                compareStateName(entry.state));
    // SC-006: a NOISY row answers why, in the trailing field the console
    // report gives a row's own account.
    if (!entry.reason.empty()) {
      std::printf("  reason=%s", entry.reason.c_str());
    }
    std::printf("\n");
  }
}

auto writeCompareReport(const std::string& path,
                        const CompareResult& result,
                        const std::vector<CompareRule>& rules,
                        const std::string& baselinePath,
                        const std::string& contenderPath) -> bool
{
  SG_REQUIRE(!path.empty(),
             "writeCompareReport takes the path the option scanner settled "
             "(specs/026 contracts/compare-document.md)");

  Builder builder;
  {
    CompareObjectWriter root(builder);
    put(builder, root, "schemaVersion", kCompareSchemaVersion);
    root.key("speedgun");
    {
      CompareObjectWriter speedgun(builder);
      put(builder, speedgun, "version", SG_PROJECT_VERSION);
      put(builder, speedgun, "buildType", SG_BUILD_TYPE);
    }
    root.key("inputs");
    {
      CompareObjectWriter inputs(builder);
      put(builder, inputs, "baseline", baselinePath);
      put(builder, inputs, "contender", contenderPath);
    }
    root.key("rules");
    {
      CompareArrayWriter list(builder);
      // E-10, FR-033: the document names the rule the comparison applied
      // even when the caller gave none, which is the default rule's text.
      if (rules.empty()) {
        list.item();
        builder.escape_and_append_with_quotes(kDefaultCompareRule);
      }
      for (const CompareRule& rule : rules) {
        list.item();
        builder.escape_and_append_with_quotes(rule.text);
      }
    }
    root.key("entries");
    {
      CompareArrayWriter list(builder);
      for (const CompareEntry& entry : result.entries) {
        list.item();
        writeEntry(builder, entry);
      }
    }
    root.key("verdict");
    {
      CompareObjectWriter verdict(builder);
      put(builder, verdict, "passed", result.passed);
    }
  }
  // FR-058, SC-009: the bytes come from the values above and from nothing
  // else: no date, no host and no pid, so two runs of one command write
  // the same document.
  const auto text = builder.view();
  SG_ENSURE(text.has_value(),
            "the builder's buffer holds the document it wrote");
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream.is_open()) {
    std::fprintf(stderr,
                 "error: the comparison file '%s' cannot be opened: %s\n",
                 detail::quotedText(path).c_str(),
                 detail::quotedText(std::strerror(errno)).c_str());
    return false;
  }
  stream.write(text->data(), static_cast<std::streamsize>(text->size()));
  stream.close();
  if (stream.fail()) {
    std::fprintf(stderr,
                 "error: the comparison file '%s' could not be written: %s\n",
                 detail::quotedText(path).c_str(),
                 detail::quotedText(std::strerror(errno)).c_str());
    return false;
  }
  return true;
}

}  // namespace detail

}  // namespace sg
