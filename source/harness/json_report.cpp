#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/json_report.hpp"

#include <sched.h>
#include <simdjson.h>

#include "detail/internal.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

using Builder = simdjson::builder::string_builder;

// The schema the writer emits and the checker reads (FR-007). The minor
// stands at 1.4 since specs/027 added the `jitter` object over the 1.3
// the specs/028 `run` object took, the move
// specs/023/contracts/schema-file.md states for a key added inside major 1
// (specs/027 FR-026, specs/028 FR-086, PC-6).
// The schema the writer emits and the checker reads (FR-007). The minor
// stands at 1.3 since specs/028 added the `run` object and at 1.4 since
// specs/029 added the root `hunt` object and the per-benchmark `hunt`
// object, the move
// specs/023/contracts/schema-file.md states for a key added inside major 1
// (specs/028 FR-086, PC-6).
// At rebase onto a master that already took 1.4, retake the next free minor.
constexpr std::string_view kSchemaVersion = "1.4";

// The `source` word of one disclosed use, spelled as the enum of
// `docs/schema/report-v1.json` states it (specs/028 E-04, FR-040). The
// gather answers a use for the two sources other than the command line, so
// the first word of the table stands in no written document.
inline constexpr std::array<std::string_view, 3> kSourceWords {
    "command line",
    "file",
    "environment",
};
static_assert(
    kSourceWords.size()
    == static_cast<std::size_t>(std::to_underlying(OptionSource::ENVIRONMENT))
        + 1);

// The hunt word tables of specs/029 (contracts/hunt-json.md). The document
// spells `norm` and `rank` with the lower-case words the option takes, and
// `skipped[].kind` with the words the console prints for a refusal, so this
// table stands apart from `kAvailabilityWords`, which spells the enumerator.
// Each table is indexed by its own enumeration and tied to its last
// enumerator, the shape the tables of the header take (E-02, E-06, FR-050).
constexpr std::array<std::string_view, 3> kHuntNormWords {
    "instr",
    "cycle",
    "iter",
};
static_assert(kHuntNormWords.size()
              == std::to_underlying(hunt::HuntNorm::ITER) + 1);

constexpr std::array<std::string_view, 2> kHuntRankWords {
    "rate",
    "cv",
};
static_assert(kHuntRankWords.size()
              == std::to_underlying(hunt::HuntRank::CV) + 1);

constexpr std::array<std::string_view, 5> kSkippedKindWords {
    "permission-blocked",
    "not-encodable",
    "absent",
    "scope-refused",
    "gap",
};
static_assert(kSkippedKindWords.size()
              == std::to_underlying(hunt::SkippedKind::GAP) + 1);

// The builder is a low-level writer that emits no separator of its own,
// so every object and array of the document opens a scope that puts one
// comma before every entry after the first and closes the bracket the
// scope opened.
class JsonObjectWriter
{
public:
  explicit JsonObjectWriter(Builder& builder) noexcept
      : m_builder(builder)
  {
    m_builder.start_object();
  }

  JsonObjectWriter(const JsonObjectWriter&) = delete;
  JsonObjectWriter& operator=(const JsonObjectWriter&) = delete;

  ~JsonObjectWriter() noexcept { m_builder.end_object(); }

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

class JsonArrayWriter
{
public:
  explicit JsonArrayWriter(Builder& builder) noexcept
      : m_builder(builder)
  {
    m_builder.start_array();
  }

  JsonArrayWriter(const JsonArrayWriter&) = delete;
  JsonArrayWriter& operator=(const JsonArrayWriter&) = delete;

  ~JsonArrayWriter() noexcept { m_builder.end_array(); }

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

// One key and one scalar value. The number path is the builder's, which
// is `std::to_chars` at the shortest text that reads back the same
// double, so the round trip of FR-011 rides on the vendored writer and
// this tree writes no formatter of its own (D-09).
auto put(Builder& builder,
         JsonObjectWriter& object,
         std::string_view key,
         std::string_view value) noexcept -> void
{
  object.key(key);
  builder.escape_and_append_with_quotes(value);
}

// A text the build hands over as a literal needs its own overload: the
// boolean overload below would otherwise win the overload set, and the
// document would hold `true` where it should hold the text.
auto put(Builder& builder,
         JsonObjectWriter& object,
         std::string_view key,
         const char* value) noexcept -> void
{
  put(builder, object, key, std::string_view {value});
}

auto put(Builder& builder,
         JsonObjectWriter& object,
         std::string_view key,
         double value) noexcept -> void
{
  object.key(key);
  builder.append(value);
}

auto put(Builder& builder,
         JsonObjectWriter& object,
         std::string_view key,
         std::uint64_t value) noexcept -> void
{
  object.key(key);
  builder.append(value);
}

auto put(Builder& builder,
         JsonObjectWriter& object,
         std::string_view key,
         bool value) noexcept -> void
{
  object.key(key);
  builder.append(value);
}

// A text the run may leave unset stands `null`, the one spelling the
// reader answers back as the empty text (E-05, FR-025).
auto putOrNull(Builder& builder,
               JsonObjectWriter& object,
               std::string_view key,
               std::string_view value) noexcept -> void
{
  object.key(key);
  if (value.empty()) {
    builder.append_null();
    return;
  }
  builder.escape_and_append_with_quotes(value);
}

// A count the run leaves unset stands `null`, the spelling the reader
// answers back as an empty optional (E-05, specs/024 E-08).
auto putOrNull(Builder& builder,
               JsonObjectWriter& object,
               std::string_view key,
               const std::optional<std::size_t>& value) noexcept -> void
{
  object.key(key);
  if (!value.has_value()) {
    builder.append_null();
    return;
  }
  builder.append(static_cast<std::uint64_t>(*value));
}

// A figure the run leaves unset stands `null` for the same reason, the file
// form of the console's `not available` (E-05, specs/024 FR-017).
auto putOrNull(Builder& builder,
               JsonObjectWriter& object,
               std::string_view key,
               const std::optional<double>& value) noexcept -> void
{
  object.key(key);
  if (!value.has_value()) {
    builder.append_null();
    return;
  }
  builder.append(*value);
}

void writeStatistics(Builder& builder, const sg::Statistics& stats)
{
  JsonObjectWriter object(builder);
  put(builder, object, "samples", stats.samples);
  put(builder, object, "mean", stats.mean);
  put(builder, object, "median", stats.median);
  put(builder, object, "sampleStdDev", stats.sampleStdDev);
  put(builder, object, "coefficientOfVariation", stats.coefficientOfVariation);
  put(builder, object, "min", stats.min);
  put(builder, object, "max", stats.max);
}

// A summary the run reached over no sample stands `null`, the one rule
// that decides the key at the row and at each metric alike (E-05).
void writeStatisticsOrNull(Builder& builder,
                           JsonObjectWriter& object,
                           std::string_view key,
                           const sg::Statistics& stats)
{
  object.key(key);
  if (stats.samples == 0) {
    builder.append_null();
    return;
  }
  writeStatistics(builder, stats);
}

void writeMetricValue(Builder& builder,
                      const sg::MetricValue& metric,
                      std::string_view label)
{
  JsonObjectWriter object(builder);
  put(builder, object, "label", label);
  put(builder, object, "value", metric.value);
  put(builder, object, "runningRatio", metric.runningRatio);
  put(builder, object, "scaled", metric.scaled);
  put(builder,
      object,
      "availability",
      enumWord(kAvailabilityWords, std::to_underlying(metric.availability)));
  // The pass that measured this column, `null` where one pass measured the
  // whole set (specs/024 E-08, FR-019).
  putOrNull(builder, object, "pass", metric.pass);
}

// One per-cpu object of the run-level `stats` array: the cpu index and
// the sixteen fields in the E-03 order, and under the `--json-raw=gaps`
// condition alone the kept gap records, each one object of the two
// fields E-04 carries (E-03, E-10, FR-028).
void writeJitterCpu(Builder& builder,
                    const JitterReportCpu& entry,
                    bool rawGaps)
{
  JsonObjectWriter object(builder);
  put(builder, object, "cpu", std::uint64_t {entry.cpu});
  put(builder, object, "gapCount", entry.stats.gapCount);
  put(builder, object, "gapsPerSecond", entry.stats.gapsPerSecond);
  put(builder, object, "minNs", entry.stats.minNs);
  put(builder, object, "medianNs", entry.stats.medianNs);
  put(builder, object, "meanNs", entry.stats.meanNs);
  put(builder, object, "p90Ns", entry.stats.p90Ns);
  put(builder, object, "p99Ns", entry.stats.p99Ns);
  put(builder, object, "p999Ns", entry.stats.p999Ns);
  put(builder, object, "p9999Ns", entry.stats.p9999Ns);
  put(builder, object, "maxNs", entry.stats.maxNs);
  put(builder, object, "totalNs", entry.stats.totalNs);
  put(builder, object, "totalPercent", entry.stats.totalPercent);
  put(builder, object, "runtimeNs", entry.stats.runtimeNs);
  put(builder, object, "thresholdNs", entry.stats.thresholdNs);
  put(builder, object, "loopPeriodNs", entry.stats.loopPeriodNs);
  put(builder, object, "droppedGaps", entry.stats.droppedGaps);
  if (rawGaps) {
    object.key("gaps");
    JsonArrayWriter gaps(builder);
    for (const auto& gap : entry.gaps) {
      gaps.item();
      JsonObjectWriter record(builder);
      put(builder, record, "stamp", gap.stamp);
      put(builder, record, "gapNs", gap.gapNs);
    }
  }
}

// One rank of one column: the availability stands beside the value in
// every case, and a rank the run could not answer carries `null` beside
// a false flag, which is the pair the reader answers back the same way
// (E-06, FR-015, FR-021, FR-025).
void writePercentileCell(Builder& builder, const sg::PercentileValue& cell)
{
  JsonObjectWriter object(builder);
  put(builder, object, "available", cell.available);
  object.key("value");
  if (cell.available) {
    builder.append(cell.value);
    return;
  }
  builder.append_null();
}

void writeFit(Builder& builder, const sg::FitRow& fit)
{
  JsonObjectWriter object(builder);
  put(builder, object, "quantity", fit.quantity);
  put(builder, object, "curve", fit.curve);
  put(builder, object, "coefficient", fit.coefficient);
  put(builder, object, "rmsPercent", fit.rmsPercent);
  put(builder, object, "scalesWithUnit", fit.scalesWithUnit);
  putOrNull(builder, object, "reason", fit.reason);
}

void writeRawColumns(Builder& builder, const RowRaw& raw)
{
  JsonObjectWriter object(builder);
  object.key("columns");
  JsonArrayWriter columns(builder);
  for (const auto& column : raw.columns) {
    columns.item();
    JsonArrayWriter values(builder);
    for (const double value : column) {
      values.item();
      builder.append(value);
    }
  }
}

void writeRow(Builder& builder,
              const BenchmarkResult& result,
              std::size_t position,
              const sg::ResultRow& row,
              const RowRaw& raw,
              const std::vector<RowRaw>& contextRaw)
{
  JsonObjectWriter object(builder);
  put(builder,
      object,
      "kind",
      enumWord(kRowKindWords, std::to_underlying(row.kind)));
  putOrNull(builder, object, "label", row.label);
  put(builder, object, "iterations", row.iterations);
  put(builder, object, "timePerIterationNs", row.timePerIterationNs);
  put(builder, object, "cpuTimePerIterationNs", row.cpuTimePerIterationNs);
  put(builder, object, "endpointSamples", row.endpointSamples);
  put(builder, object, "pauseSamplesFolded", row.pauseSamplesFolded);
  put(builder,
      object,
      "timeAvailability",
      enumWord(kAvailabilityWords, std::to_underlying(row.timeAvailability)));
  object.key("metrics");
  {
    JsonArrayWriter metrics(builder);
    for (std::size_t index = 0; index < row.metrics.size(); ++index) {
      metrics.item();
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      writeMetricValue(builder, row.metrics[index], result.metricLabels[index]);
    }
  }
  object.key("counters");
  {
    JsonArrayWriter counters(builder);
    for (const double value : row.counters) {
      counters.item();
      builder.append(value);
    }
  }
  putOrNull(builder, object, "counterDisclosure", row.counterDisclosure);
  object.key("fit");
  if (row.fit.quantity.empty()) {
    builder.append_null();
  } else {
    writeFit(builder, row.fit);
  }
  put(builder, object, "overheadFloorNs", row.overheadFloorNs);
  object.key("percentiles");
  {
    JsonArrayWriter columns(builder);
    for (const auto& column : row.percentiles) {
      columns.item();
      JsonArrayWriter points(builder);
      for (const auto& cell : column) {
        points.item();
        writePercentileCell(builder, cell);
      }
    }
  }
  // FR-015: the answer of each context of a threaded row stands beside
  // the merged answer, in context order. The flattened field of the
  // result holds one block of `contextCount` entries per row that
  // captured chunks, so a row past the last block captured none.
  const std::size_t contextCount = result.contexts.size();
  if (contextCount > 0
      && (position + 1) * contextCount <= result.contextPercentiles.size())
  {
    object.key("contextPercentiles");
    JsonArrayWriter contexts(builder);
    for (std::size_t index = 0; index < contextCount; ++index) {
      contexts.item();
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const auto& columnsOfContext =
          result.contextPercentiles[(position * contextCount) + index];
      JsonArrayWriter columns(builder);
      for (const auto& column : columnsOfContext) {
        columns.item();
        JsonArrayWriter points(builder);
        for (const auto& cell : column) {
          points.item();
          writePercentileCell(builder, cell);
        }
      }
    }
  }
  if (!contextRaw.empty()) {
    object.key("contextRaw");
    JsonArrayWriter contexts(builder);
    for (const auto& context : contextRaw) {
      contexts.item();
      writeRawColumns(builder, context);
    }
  }
  put(builder, object, "chunkCount", row.chunkCount);
  put(builder, object, "chunkIterations", row.chunkIterations);
  writeStatisticsOrNull(builder, object, "timeStats", row.timeStats);
  object.key("metricStats");
  {
    JsonArrayWriter stats(builder);
    for (const auto& stat : row.metricStats) {
      stats.item();
      if (stat.samples == 0) {
        builder.append_null();
        continue;
      }
      writeStatistics(builder, stat);
    }
  }
  if (!raw.columns.empty()) {
    object.key("raw");
    writeRawColumns(builder, raw);
  }
}

void writeEntry(Builder& builder,
                const ReportData& data,
                std::size_t index,
                const std::optional<JitterReportRecord>& jitter)
// specs/029 FR-050, contracts/hunt-json.md: the per-entry hunt object, the key
// `writeEntry` appends. Every key of the closed node stands in the document of
// every hunt entry; a rank or a stage the run left unset stands `null`, the
// shape `passSpread` already takes, so no key of the node is ever absent.
void writeEntryHunt(Builder& builder, const hunt::EntryHunt& record)
{
  JsonObjectWriter object(builder);
  // The settled method of the hunt plan, in the spelling `metricMethod` uses.
  // A record with no settled method writes the word of an unstated method,
  // the value the entry's own `metricMethod` key carries in that case.
  put(builder,
      object,
      "method",
      enumWord(kMetricMethodWords,
               std::to_underlying(record.method.value_or(MetricMethod::AUTO))));
  put(builder, object, "pass_count", record.passCount);
  put(builder, object, "top", record.top);
  put(builder, object, "retried_passes", record.retriedPasses);
  object.key("entries");
  {
    JsonArrayWriter entries(builder);
    for (const auto& ranked : record.entries) {
      entries.item();
      JsonObjectWriter entry(builder);
      put(builder, entry, "address", ranked.address);
      put(builder, entry, "rate", ranked.rate);
      put(builder, entry, "cv", ranked.cv);
      put(builder, entry, "noisy", ranked.noisy);
      put(builder, entry, "multiplexed", ranked.multiplexed);
      putOrNull(builder, entry, "rank", ranked.rank);
    }
  }
  object.key("skipped");
  {
    JsonArrayWriter skipped(builder);
    for (const auto& skip : record.skipped) {
      skipped.item();
      JsonObjectWriter item(builder);
      put(builder, item, "address", skip.address);
      put(builder,
          item,
          "kind",
          enumWord(kSkippedKindWords, std::to_underlying(skip.kind)));
      putOrNull(builder, item, "stage", skip.stage);
    }
  }
}

void writeEntry(Builder& builder, const ReportData& data, std::size_t index)
{
  const BenchmarkResult& result = data.results[index];
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  const std::vector<RowRaw>& rowRaw = data.rowRaw[index];
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  const std::vector<std::vector<RowRaw>>& contextRaw = data.contextRaw[index];
  JsonObjectWriter object(builder);
  put(builder, object, "name", result.name);
  put(builder, object, "suite", result.suite);
  put(builder, object, "case", result.caseName);
  put(builder,
      object,
      "outcome",
      enumWord(kOutcomeWords, std::to_underlying(result.outcome)));
  putOrNull(builder, object, "reason", result.reason);
  put(builder, object, "hideRepetitionRows", result.hideRepetitionRows);
  put(builder,
      object,
      "timeUnit",
      enumWord(kTimeUnitWords, std::to_underlying(result.timeUnit)));
  put(builder, object, "primaryIsCpu", result.primaryIsCpu);
  put(builder, object, "pauseSamplesFolded", result.pauseSamplesFolded);
  object.key("percentilePoints");
  {
    JsonArrayWriter points(builder);
    for (const double point : result.percentilePoints) {
      points.item();
      builder.append(point);
    }
  }
  object.key("contexts");
  {
    JsonArrayWriter contexts(builder);
    for (const auto& context : result.contexts) {
      contexts.item();
      JsonObjectWriter record(builder);
      put(builder,
          record,
          "waitMode",
          enumWord(kWaitModeWords, std::to_underlying(context.waitMode)));
      put(builder, record, "placement", context.placement);
      put(builder, record, "applied", context.applied);
    }
  }
  object.key("noiseWarning");
  if (const auto warning = noiseWarningOf(result); warning.has_value()) {
    JsonObjectWriter noise(builder);
    put(builder, noise, "spinning", warning->spinning);
    put(builder, noise, "runnableCpus", warning->runnableCpus);
    put(builder, noise, "sharesOneCpu", warning->sharesOneCpu);
  } else {
    builder.append_null();
  }
  if (jitter.has_value()) {
    // E-08, FR-027: the per-entry key stands after `noiseWarning`: null
    // when no kept gap landed inside this benchmark's windows, and the
    // stats object when they did, one object per probe cpu over those
    // windows in the order the result's carrier fills, ascending cpu
    // order. The sixteen fields of each object ride the writer the
    // run-level stats array shares; the raw gap list stays the run
    // level's condition alone (E-10).
    object.key("jitter");
    if (result.jitterFolds.empty()) {
      builder.append_null();
    } else {
      JsonObjectWriter entryJitter(builder);
      entryJitter.key("stats");
      JsonArrayWriter stats(builder);
      for (const auto& fold : result.jitterFolds) {
        stats.item();
        JitterReportCpu placed;
        placed.cpu = fold.cpu;
        placed.stats = fold.stats;
        writeJitterCpu(builder, placed, false);
      }
    }
  }
  object.key("rows");
  {
    JsonArrayWriter rows(builder);
    for (std::size_t position = 0; position < result.rows.size(); ++position) {
      rows.item();
      // The two empty column sets a row without retained chunk values
      // reads back as, so the absent key needs no branch of its own.
      static const RowRaw kNoRaw {};
      static const std::vector<RowRaw> kNoContextRaw {};
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const RowRaw& raw = position < rowRaw.size() ? rowRaw[position] : kNoRaw;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const std::vector<RowRaw>& contexts =
          position < contextRaw.size() ? contextRaw[position] : kNoContextRaw;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      writeRow(builder, result, position, result.rows[position], raw, contexts);
    }
  }
  object.key("metricLabels");
  {
    JsonArrayWriter labels(builder);
    for (const auto& label : result.metricLabels) {
      labels.item();
      builder.escape_and_append_with_quotes(label);
    }
  }
  object.key("counterLabels");
  {
    JsonArrayWriter labels(builder);
    for (const auto& label : result.counterLabels) {
      labels.item();
      builder.escape_and_append_with_quotes(label);
    }
  }
  object.key("counterStats");
  {
    JsonArrayWriter stats(builder);
    for (const auto& stat : result.counterStats) {
      stats.item();
      writeStatistics(builder, stat);
    }
  }
  object.key("statisticValues");
  {
    JsonArrayWriter values(builder);
    for (const auto& value : result.statisticValues) {
      values.item();
      JsonObjectWriter record(builder);
      put(builder, record, "name", value.name);
      put(builder,
          record,
          "unit",
          enumWord(kStatUnitWords, std::to_underlying(value.unit)));
      put(builder, record, "value", value.value);
    }
  }
  // specs/024 FR-038: the pass disclosure of the run in the form the
  // document carries. The keys stand for every run, the one-pass run
  // included, the case where the console prints no line (FR-021).
  put(builder,
      object,
      "metricMethod",
      enumWord(kMetricMethodWords, std::to_underlying(result.metricMethod)));
  put(builder, object, "passCount", result.passCount);
  putOrNull(builder, object, "passSpread", result.passSpread);
  put(builder, object, "spreadWarning", spreadWarned(result.passSpread));
  // specs/029 FR-050, FR-052: the hunt object is the last key of the entry.
  // An entry of a run that carried no hunt writes no key, the one rule that
  // leaves such a document the bytes it was before the hunt existed (I-8).
  if (result.hunt.has_value()) {
    object.key("hunt");
    writeEntryHunt(builder, *result.hunt);
  }
}

// specs/029 FR-050, FR-031 (PD-10): the run-level hunt object, the key
// `writeReport` appends after the `benchmarks` array. The record carries the
// settled option values, so the document states what the run asked for
// beside what it ran.
void writeHunt(Builder& builder, const hunt::RunHunt& record)
{
  const hunt::HuntOptions& options = record.options;
  // The budget keeps the unit the caller named, the rule the console plan
  // line takes too (FR-012); with no budget the text stays empty, which is
  // the one case `putOrNull` writes `null` for.
  auto budget = std::string {};
  if (options.budget.has_value()) {
    budget = std::to_string(options.budget->amount)
        + (options.budget->kind == hunt::HuntBudget::Kind::SECONDS ? "s" : "p");
  }
  JsonObjectWriter object(builder);
  put(builder, object, "selection", record.selection);
  // A run that named no norm ranks by instructions, the norm the console
  // label answers for the same case (D-13).
  put(builder,
      object,
      "norm",
      enumWord(
          kHuntNormWords,
          std::to_underlying(options.norm.value_or(hunt::HuntNorm::INSTR))));
  put(builder,
      object,
      "rank",
      enumWord(kHuntRankWords, std::to_underlying(options.rank)));
  put(builder, object, "top", options.top);
  put(builder, object, "noisePercent", options.noise);
  put(builder, object, "repeats", options.repeats);
  put(builder, object, "seed", options.seed);
  putOrNull(builder, object, "budget", std::string_view {budget});
  put(builder, object, "passes", record.passes);
  put(builder,
      object,
      "method",
      enumWord(kMetricMethodWords,
               std::to_underlying(record.method.value_or(MetricMethod::AUTO))));
  // Every planned address in plan order, the addresses the stages index
  // (FR-050, D-17).
  object.key("events");
  {
    JsonArrayWriter events(builder);
    for (const auto& address : record.plannedAddresses) {
      events.item();
      builder.escape_and_append_with_quotes(address);
    }
  }
  object.key("stages");
  {
    JsonArrayWriter stages(builder);
    for (const auto& stage : record.stages) {
      stages.item();
      JsonObjectWriter range(builder);
      put(builder, range, "first", stage.first);
      put(builder, range, "count", stage.count);
      put(builder, range, "repeats", stage.repeats);
    }
  }
  putOrNull(builder, object, "estimate_ns", record.estimateNs);
  put(builder, object, "planned_events", record.plannedAddresses.size());
  put(builder, object, "ran_events", record.ranEvents);
  put(builder, object, "stage_two_events", record.stageTwoEvents);
}

// The count of cpus the noise condition compares the spinning contexts
// against: the affinity mask of this process, which a cpuset, a taskset
// or a container narrows, and the host count of the standard library
// where that read answers nothing (FR-026). The read stands here and the
// choice stands in `selectRunnableCpuCount`, where a test drives each
// answer. The read names no clock and samples nothing (FR-027).
auto runnableCpuCount() -> std::uint64_t
{
  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  const bool answered = sched_getaffinity(0, sizeof(cpus), &cpus) == 0;
  return detail::selectRunnableCpuCount(
      answered,
      static_cast<std::uint64_t>(CPU_COUNT(&cpus)),
      static_cast<std::uint64_t>(std::thread::hardware_concurrency()));
}

}  // namespace

auto noiseWarningOf(const BenchmarkResult& result)
    -> std::optional<NoiseWarning>
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
  const NoiseWarning warning {
      .spinning = spinning,
      .runnableCpus = runnableCpuCount(),
      .sharesOneCpu = std::ranges::adjacent_find(pinned) != pinned.end(),
  };
  if (!warning.sharesOneCpu && warning.spinning <= warning.runnableCpus) {
    return std::nullopt;
  }
  return warning;
}

// specs/024 FR-018: the one answer the console line, the `spreadWarning` key
// and the reader's check of that key give.
auto spreadWarned(const std::optional<double> spread) noexcept -> bool
{
  return spread.has_value() && *spread > kSpreadWarningLimitPercent;
}

// The `run` object of specs/028: the seed, then one entry per disclosed use
// in the order the gather handed them, which is table order (FR-040,
// E-09). The option text is the long spelling with the leading dashes, the
// form the console line carries too (FR-039).
auto writeRunObject(Builder& builder, const RunDisclosure& disclosure) -> void
{
  JsonObjectWriter run(builder);
  if (disclosure.interleave.has_value()) {
    put(builder, run, "interleave", *disclosure.interleave);
  }
  run.key("options");
  {
    JsonArrayWriter entries(builder);
    for (const OptionUse& use : disclosure.uses) {
      entries.item();
      JsonObjectWriter entry(builder);
      put(builder, entry, "option", "--" + std::string {use.row->name});
      put(builder,
          entry,
          "source",
          enumWord(kSourceWords,
                   static_cast<std::size_t>(std::to_underlying(use.source))));
      put(builder, entry, "value", use.value);
    }
  }
}

auto writeReport(const std::string& path,
                 const ReportData& data,
                 const RunDisclosure& disclosure,
                 const std::optional<JitterReportRecord>& jitter) -> bool
{
  SG_REQUIRE(!path.empty(),
             "writeReport takes the path the option scanner settled "
             "(specs/023 contracts/report-writer.md)");
  SG_REQUIRE(data.rowRaw.size() == data.results.size()
                 && data.contextRaw.size() == data.results.size(),
             "the raw columns of a report stand beside every result of it "
             "(specs/023 E-01)");
  SG_REQUIRE(!jitter.has_value() || jitter->cpus.size() == jitter->stats.size(),
             "the jitter record carries one stats object per probe cpu (E-07)");
  Builder builder;
  {
    JsonObjectWriter root(builder);
    root.key("schemaVersion");
    builder.append(kSchemaVersion);
    root.key("speedgun");
    {
      JsonObjectWriter speedgun(builder);
      put(builder, speedgun, "version", SG_PROJECT_VERSION);
      put(builder, speedgun, "buildType", SG_BUILD_TYPE);
    }
    // FR-041: the object stands when a use or a seed stands, and the run of
    // a command line alone writes neither the key nor its members.
    if (!disclosure.uses.empty() || disclosure.interleave.has_value()) {
      root.key("run");
      writeRunObject(builder, disclosure);
    }
    if (jitter.has_value()) {
      // E-07: the run-level object stands after `speedgun` and before
      // `benchmarks`, in the key order the schema row states (FR-026,
      // R-09). The unavailable answer still appears, with the mode the
      // run requested and the disclosure text (FR-037, D-10).
      root.key("jitter");
      JsonObjectWriter probe(builder);
      put(builder,
          probe,
          "mode",
          enumWord(kJitterModeWords, std::to_underlying(jitter->mode)));
      put(builder, probe, "thresholdNs", jitter->thresholdNs);
      put(builder, probe, "loopPeriodNs", jitter->loopPeriodNs);
      probe.key("cpus");
      {
        JsonArrayWriter cpus(builder);
        for (const auto cpu : jitter->cpus) {
          cpus.item();
          builder.append(std::uint64_t {cpu});
        }
      }
      probe.key("stats");
      {
        JsonArrayWriter stats(builder);
        for (const auto& entry : jitter->stats) {
          stats.item();
          writeJitterCpu(builder, entry, jitter->rawGaps);
        }
      }
      put(builder, probe, "droppedGaps", jitter->droppedGaps);
      put(builder, probe, "disclosure", jitter->disclosure);
    }
    root.key("benchmarks");
    {
      JsonArrayWriter entries(builder);
      for (std::size_t index = 0; index < data.results.size(); ++index) {
        entries.item();
        writeEntry(builder, data, index, jitter);
      }
    }
    // specs/029 FR-050, FR-052: the hunt record of the run closes the
    // document; a run that carried no hunt writes no key and no byte of the
    // document it wrote before the hunt existed changes.
    if (data.hunt.has_value()) {
      root.key("hunt");
      writeHunt(builder, *data.hunt);
    }
  }

  const auto text = builder.view();
  SG_ENSURE(text.has_value(),
            "the builder's buffer holds the document it wrote");
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream.is_open()) {
    std::fprintf(stderr,
                 "error: the report file '%s' cannot be opened: %s\n",
                 sg::detail::quotedText(path).c_str(),
                 sg::detail::quotedText(std::strerror(errno)).c_str());
    return false;
  }
  stream.write(text->data(), static_cast<std::streamsize>(text->size()));
  stream.close();
  if (stream.fail()) {
    std::fprintf(stderr,
                 "error: the report file '%s' could not be written: %s\n",
                 sg::detail::quotedText(path).c_str(),
                 sg::detail::quotedText(std::strerror(errno)).c_str());
    return false;
  }
  return true;
}

}  // namespace sg::detail
