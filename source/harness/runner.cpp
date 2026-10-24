#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <expected>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "detail/calibration.hpp"
#include "detail/complexity.hpp"
#include "detail/hunt_plan.hpp"
#include "detail/internal.hpp"
#include "detail/options.hpp"
#include "detail/pass_plan.hpp"
#include "detail/percentiles.hpp"
#include "detail/statistics.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_metrics.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace
{

// FR-007 and FR-011 carry the defaults; FR-010 and FR-016 bound the
// growth, and detail/calibration.hpp owns that rule.
constexpr std::int64_t kDefaultMinTimeNs = 500'000'000;
constexpr std::int64_t kDefaultWarmupNs = 0;
constexpr std::uint64_t kDefaultRepetitions = 1;

using sg::detail::MetricColumn;

// The machine/tsc stamp one timed-window edge carries (R-04, E-06): a
// fresh reader over the leaf through the same catalog the probe opens,
// read once on the calling thread. The fresh window seeds from the
// position the leaf carries at the edge, so an edge and the probe's gap
// stamps read as one series with no conversion between them (R-04).
// The machine object always stands in a run's catalog - the entry point
// registers the clock provider beside it before the batch starts - and
// the ring mint over the literal capacity 2, a power of two, succeeds
// (FR-025), so the assertions own those contracts; a leaf the catalog
// lacks, or a plan the build cannot compile over it, leaves the edge
// unstamped and the window unrecorded.
auto windowStampNow() -> std::optional<std::uint64_t>
{
  auto& sys = sg::counters::System::local();
  const auto machine = sys.object("machine");
  SG_ASSERT(machine.has_value(),
            "the machine object stands in the run's catalog (R-02)");
  const auto leaf = machine.value().counter<sg::counters::Dim<0, 1>>("tsc");
  if (!leaf.has_value()) {
    return std::nullopt;
  }
  sg::counters::Expression<sg::counters::Dim<0, 1>> expression(leaf.value());
  auto compiled = sg::counters::detail::compileCore(
      sys, sg::counters::Target {}, {&expression.core});
  if (!compiled.has_value()) {
    return std::nullopt;
  }
  sg::counters::Plan plan(std::move(compiled.value()));
  auto minted = plan.recorder(2, sg::counters::ring);
  SG_ASSERT(minted.has_value(),
            "the ring recorder mints over the literal capacity (FR-025)");
  auto& recorder = minted.value();
  recorder.sample();
  return recorder.mColumns[(recorder.mHead - 1) & (recorder.mCapacity - 1)];
}

auto splitAddress(std::string_view address)
    -> std::pair<std::string_view, std::string_view>
{
  const std::size_t slash = address.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {address, {}};
  }
  return {address.substr(0, slash), address.substr(slash + 1)};
}

constexpr const char* kInterruptReason = "interrupted by the user (SIGINT)";

// The run a signal stopped keeps the skip outcome and the one reason,
// whichever phase it stopped in.
auto markInterrupted(sg::BenchmarkResult& result) noexcept -> void
{
  result.outcome = sg::RunOutcome::SKIPPED;
  result.reason = kInterruptReason;
}

// The availability of the netted value is the worst availability of the
// folds it reads, so a gap on a pause interval reaches the row. With no
// pause pairs the helper is the H1 two-point fold. FR-052: a pair the
// reservation held no room for folded into the run's running totals at
// its commit, and the helper takes those totals out of the window too,
// column by column.
template<class Recorder>
auto nettedFold(const sg::counters::detail::ExprCore& core,
                const Recorder& recorder,
                const sg::detail::RunRecord& record,
                const std::size_t foldColumn) -> sg::counters::MetricResult
{
  const auto view = recorder.view();
  auto net = sg::counters::detail::foldCore(
      core,
      view,
      record.basePoint,
      static_cast<std::size_t>(record.basePoint + record.foldPoints - 1));
  for (const auto& pause : record.pausePairs) {
    const auto paused = sg::counters::detail::foldCore(
        core,
        view,
        static_cast<std::size_t>(record.basePoint + pause.first),
        static_cast<std::size_t>(record.basePoint + pause.second));
    net.value -= paused.value;
    if (paused.availability > net.availability) {
      net.availability = paused.availability;
    }
  }
  if (record.pauseFold.foldedPairs > 0) {
    SG_ASSERT(foldColumn < record.pauseFold.values.size(),
              "the sink carries one running total per column of the run, and "
              "every fold reads one column of that set (FR-052)");
    net.value -= record.pauseFold.values[foldColumn];
    if (record.pauseFold.availability[foldColumn] > net.availability) {
      net.availability = record.pauseFold.availability[foldColumn];
    }
  }
  return net;
}

// FR-002, FR-005, FR-006: the core behind the primary time column of a
// run: the CPU core by default and the real core under the real
// selection. A manual run answers with a null core: its primary time is
// the sum the benchmark supplied, and no fold stands between it and the
// row.
auto selectPrimaryCore(const bool manualTime,
                       const bool realTime,
                       const sg::counters::detail::ExprCore& realTimeCore,
                       const sg::counters::detail::ExprCore& cpuTimeCore)
    -> const sg::counters::detail::ExprCore*
{
  if (manualTime) {
    return nullptr;
  }
  return realTime ? &realTimeCore : &cpuTimeCore;
}

// specs/021 FR-002: the per-iteration columns divide their chunk values by
// the iterations one chunk holds; the rest stand as the fold left them.
auto divideChunkValues(std::vector<std::vector<double>>& values,
                       const std::vector<bool>& perIteration,
                       const std::uint64_t chunkIterations) -> void
{
  const double divisor = static_cast<double>(chunkIterations);
  for (std::size_t at = 0; at < values.size(); ++at) {
    if (!perIteration[at]) {
      continue;
    }
    for (double& value : values[at]) {
      value /= divisor;
    }
  }
}

// One measured row: the sample span the run left in the recorder,
// netted through every column, with the FR-022 division, the FR-019
// floor, and the two time columns of FR-001 beside the sample count of
// FR-013 and the primary availability of research D-13. A manual run
// has no primary fold (FR-006): its primary time is the sum the
// benchmark supplied, no fold can put a gap on it, and a null core
// says so.
template<class Recorder>
auto appendMeasuredRow(std::vector<sg::ResultRow>& rows,
                       const sg::detail::RunRecord& record,
                       const std::vector<MetricColumn>& columns,
                       const std::vector<bool>& measured,
                       double overheadFloorNs,
                       const sg::counters::detail::ExprCore* primaryCore,
                       const sg::counters::detail::ExprCore& secondaryCore,
                       const Recorder& recorder,
                       std::size_t primaryFoldColumn,
                       std::size_t secondaryFoldColumn,
                       const std::vector<double>& points,
                       std::vector<std::vector<double>>& retained) -> void
{
  sg::ResultRow& row = rows.emplace_back();
  row.kind = sg::RowKind::REPETITION;
  row.iterations = record.iterations;
  row.timePerIterationNs = static_cast<double>(record.realNs)
      / static_cast<double>(record.iterations);
  row.cpuTimePerIterationNs = static_cast<double>(record.cpuNs)
      / static_cast<double>(record.iterations);
  row.endpointSamples = record.samples;
  // FR-054: a folded run is disclosed beside its row; a raw row keeps
  // the bytes it prints today.
  row.pauseSamplesFolded = record.pauseSamplesFolded;
  // D-04, FR-011: the row carries the label its repetition ended with.
  row.label = record.label;
  row.timeAvailability = primaryCore != nullptr
      ? nettedFold(*primaryCore, recorder, record, primaryFoldColumn)
            .availability
      : sg::counters::Availability::COUNTABLE;
  row.overheadFloorNs = overheadFloorNs;
  for (std::size_t position = 0; position < columns.size(); ++position) {
    const auto& column = columns[position];
    sg::MetricValue value;
    if (column.unavailable) {
      value.availability = column.availability;
      row.metrics.push_back(value);
      continue;
    }
    // specs/024 FR-009, FR-014: a column this pass does not measure has no
    // leaf in this pass's plan, so no value of this pass stands in its slot;
    // the merge brings the value of the pass that measured the column.
    if (!measured[position]) {
      value.availability = sg::counters::Availability::GAP;
      row.metrics.push_back(value);
      continue;
    }
    const auto folded =
        nettedFold(column.core, recorder, record, column.foldColumn);
    value.value = folded.value;
    value.runningRatio = folded.runningRatio;
    value.scaled = folded.scaled;
    value.availability = folded.availability;
    SG_ASSERT(record.iterations > 0,
              "a measured run counts at least one iteration (FR-016)");
    if (column.perIteration
        && folded.availability == sg::counters::Availability::COUNTABLE)
    {
      value.value /= static_cast<double>(record.iterations);
    }
    row.metrics.push_back(value);
  }

  // FR-003, FR-004, FR-005: the flags apply now that the repetition is over,
  // and the primary time of the row is the divisor of a rate. A primary fold
  // that carries a disclosure keeps that disclosure, and the zero divisor is
  // named only when the fold itself is countable and still holds no time:
  // the two causes are distinct and each row names its own. Both leave the
  // flag-neutral value with a disclosure beside every counter column.
  const bool primaryCountable =
      row.timeAvailability == sg::counters::Availability::COUNTABLE;
  const double primarySeconds = record.decisionNs > 0 && primaryCountable
      ? static_cast<double>(record.decisionNs) * 1e-9
      : 0.0;
  if (!primaryCountable) {
    row.counterDisclosure = "fold-disclosed";
  } else if (record.decisionNs == 0) {
    row.counterDisclosure = "zero-divisor";
  }
  sg::detail::appendCounterColumns(row.counters,
                                   record.counterNames,
                                   record.counterStore,
                                   primarySeconds,
                                   record.iterations);

  // specs/021 FR-001, FR-010: the chunk values this run captured answer
  // the percentile columns of the row, in the order the row prints its
  // columns: the primary time, the time column the primary is not, then
  // each metric (SC-001). A run captures chunks only where the percentile
  // output is on, so a nonzero count already carries the points.
  if (record.chunkCount > 0) {
    const auto api = recorder.view();
    std::vector<std::vector<double>> chunkValues;
    std::vector<bool> countable;
    std::vector<bool> perIteration;
    if (primaryCore != nullptr) {
      chunkValues.push_back(sg::detail::buildChunkValues(*primaryCore,
                                                         api,
                                                         record.basePoint,
                                                         record.chunkCount,
                                                         record.pausePairs));
      countable.push_back(row.timeAvailability
                          == sg::counters::Availability::COUNTABLE);
      perIteration.push_back(true);
    } else {
      // FR-016: a manual run carries no fold behind its primary time, so
      // no chunk value stands behind its percentile either.
      chunkValues.emplace_back();
      countable.push_back(false);
      perIteration.push_back(false);
    }
    chunkValues.push_back(sg::detail::buildChunkValues(secondaryCore,
                                                       api,
                                                       record.basePoint,
                                                       record.chunkCount,
                                                       record.pausePairs));
    countable.push_back(
        nettedFold(secondaryCore, recorder, record, secondaryFoldColumn)
            .availability
        == sg::counters::Availability::COUNTABLE);
    perIteration.push_back(true);
    for (std::size_t position = 0; position < columns.size(); ++position) {
      const auto& column = columns[position];
      if (column.unavailable || !measured[position]) {
        chunkValues.emplace_back();
        countable.push_back(false);
        perIteration.push_back(false);
        continue;
      }
      chunkValues.push_back(sg::detail::buildChunkValues(column.core,
                                                         api,
                                                         record.basePoint,
                                                         record.chunkCount,
                                                         record.pausePairs));
      countable.push_back(row.metrics[position].availability
                          == sg::counters::Availability::COUNTABLE);
      perIteration.push_back(column.perIteration);
    }
    divideChunkValues(chunkValues, perIteration, record.chunkIterations);
    retained = chunkValues;
    row.percentiles =
        sg::detail::buildPercentileColumns(chunkValues, points, countable);
    row.chunkCount = record.chunkCount;
    row.chunkIterations = record.chunkIterations;
  }
}

// E-06, FR-020: the folded values of one context of a threaded attempt.
// Each value is the fold of that context's own plan, taken on that
// context's own thread (FR-018, PC-1). The merged-row build reads these
// values and no recorder, so no fold crosses contexts.
struct ContextFolds
{
  // The primary fold of the context: absent for a manual run, whose
  // primary time carries no fold (FR-006).
  std::optional<sg::counters::MetricResult> primary;
  // One fold per column of the run, in the order of MetricColumn; the
  // slot of an unavailable column carries no fold.
  std::vector<sg::counters::MetricResult> columns;
  // specs/021 FR-004, FR-014: the chunk values this context captured, the
  // primary time first and then one entry per MetricColumn slot. Empty when
  // the attempt captured no chunks.
  std::vector<std::vector<double>> chunkValues;
  // specs/021 FR-016: the count of those columns whose fold disclosed no
  // gap, in the same order.
  std::vector<bool> countable;
};

// The primary availability of a merged row (FR-018): the worst
// availability the contexts' own folds disclose. A manual run carries
// no primary fold and so no primary gap (FR-006).
auto worstPrimaryAvailability(const std::vector<ContextFolds>& folds,
                              const sg::counters::detail::ExprCore* primaryCore)
    -> sg::counters::Availability
{
  auto worst = sg::counters::Availability::COUNTABLE;
  if (primaryCore == nullptr) {
    return worst;
  }
  for (const ContextFolds& fold : folds) {
    // The manual switch is one family property, so every context of
    // one attempt carries the same primary shape.
    SG_ASSERT(fold.primary.has_value(),
              "a non-manual attempt leaves one primary fold per context, "
              "and the row reads the worst availability of that set "
              "(FR-006, FR-018)");
    if (fold.primary->availability > worst) {
      worst = fold.primary->availability;
    }
  }
  return worst;
}

// One metric column of a merged row (FR-020): the values of that column
// summed over the folds the contexts each did on its own plan, and the
// worst availability that set discloses; the first fold's ratio and
// scale stand as the column's. FR-022 over the summed values: the
// divisor is the summed iteration count, and the disclosure rule is the
// one the single-threaded row already applies. The slot of an
// unavailable column carries no fold.
auto mergedMetricValue(const MetricColumn& column,
                       const std::vector<ContextFolds>& folds,
                       const std::size_t position,
                       const std::uint64_t iterations,
                       const bool measured) -> sg::MetricValue
{
  sg::MetricValue value;
  if (column.unavailable) {
    value.availability = column.availability;
    return value;
  }
  if (!measured) {
    // specs/024 FR-009, FR-014: a column this pass does not measure folds in
    // no context of this pass, so no value of this pass stands in its slot;
    // the merge brings the value of the pass that measured the column.
    value.availability = sg::counters::Availability::GAP;
    return value;
  }
  double sum = 0.0;
  value.availability = sg::counters::Availability::COUNTABLE;
  for (const ContextFolds& fold : folds) {
    const sg::counters::MetricResult& folded = fold.columns[position];
    sum += folded.value;
    if (folded.availability > value.availability) {
      value.availability = folded.availability;
    }
  }
  value.value = sum;
  value.runningRatio = folds.front().columns[position].runningRatio;
  value.scaled = folds.front().columns[position].scaled;
  SG_ASSERT(iterations > 0,
            "a measured run counts at least one iteration (FR-016)");
  if (column.perIteration
      && value.availability == sg::counters::Availability::COUNTABLE)
  {
    value.value /= static_cast<double>(iterations);
  }
  return value;
}

// The merged row of one threaded attempt (E-06, FR-020): the real column
// carries the summed manual time under manual timing and the summed real
// time otherwise, the CPU column the summed CPU time, the iterations the
// summed per-context counts, and every counter the sum over the contexts
// before the flag pass (FR-020). The printed per-iteration time divides
// the summed time by the summed iterations.
//
// FR-020, D-06: the flag pass of a merged row runs at the real N. The
// shared pass divides a RATE by the primary seconds it is handed, so the
// merged row hands it the summed decision seconds of the run's timing
// mode divided by the context count: the thread-seconds the cited
// `Finish` divides a rate by. Every flag of that pass scales the value
// it is given and `INVERT` applies last, so the divisor of AVG_THREADS
// rides in as the same factor on the summed value of the names that
// carry the flag, which leaves the order of the cited pass. Handed the
// same two divisors at a count of one, the shared pass is the shipped
// single-context pass and its bytes (FR-022, PC-2).
auto appendMergedRow(
    std::vector<sg::ResultRow>& rows,
    const sg::detail::RunRecord& record,
    const std::vector<MetricColumn>& columns,
    const std::vector<bool>& measured,
    double overheadFloorNs,
    const sg::counters::detail::ExprCore* primaryCore,
    const std::vector<ContextFolds>& folds,
    const std::uint64_t threadCount,
    const std::vector<double>& points,
    std::vector<std::vector<std::vector<sg::PercentileValue>>>&
        contextPercentiles,
    std::vector<std::vector<std::vector<double>>>& contextRetained) -> void
{
  sg::ResultRow& row = rows.emplace_back();
  row.kind = sg::RowKind::REPETITION;
  row.iterations = record.iterations;
  row.timePerIterationNs = static_cast<double>(record.realNs)
      / static_cast<double>(record.iterations);
  row.cpuTimePerIterationNs = static_cast<double>(record.cpuNs)
      / static_cast<double>(record.iterations);
  row.endpointSamples = record.samples;
  row.pauseSamplesFolded = record.pauseSamplesFolded;
  // D-04, FR-011: the merged row carries the first label any context
  // of the attempt ended with; the merge wrote it into the record.
  row.label = record.label;
  row.overheadFloorNs = overheadFloorNs;
  row.timeAvailability = worstPrimaryAvailability(folds, primaryCore);
  for (std::size_t position = 0; position < columns.size(); ++position) {
    row.metrics.push_back(mergedMetricValue(columns[position],
                                            folds,
                                            position,
                                            record.iterations,
                                            measured[position]));
  }

  // FR-003, FR-004, FR-005: the same two disclosures the single-context
  // row names, read on the summed decision seconds of the merged row. The
  // availability test alone decides the divisor: a primary whose fold
  // stands above countable carries no seconds, so the product of an
  // uncountable primary hands out the same zero (Principle X).
  const bool primaryCountable =
      row.timeAvailability == sg::counters::Availability::COUNTABLE;
  const double primarySeconds = primaryCountable
      ? static_cast<double>(record.decisionNs) * 1e-9
          / static_cast<double>(threadCount)
      : 0.0;
  if (!primaryCountable) {
    row.counterDisclosure = "fold-disclosed";
  } else if (record.decisionNs == 0) {
    row.counterDisclosure = "zero-divisor";
  }
  // FR-020, D-06: AVG_THREADS divides the summed value by the real N.
  // The shared pass carries no thread count and divides that flag by
  // one, which is the whole of a single-context row; the merged row
  // scales the flagged names here, ahead of the pass, so the flag keeps
  // its place in the cited order.
  auto store = record.counterStore;
  for (auto& counter : store) {
    if ((counter.flags & sg::CounterFlag::AVG_THREADS) != sg::CounterFlag::NONE)
    {
      counter.value /= static_cast<double>(threadCount);
    }
  }
  sg::detail::appendCounterColumns(row.counters,
                                   record.counterNames,
                                   store,
                                   primarySeconds,
                                   record.iterations);

  // specs/021 FR-014: the merged row answers from the union of the
  // contexts' chunk values, and each context keeps its own answer.
  if (record.chunkCount > 0) {
    std::vector<std::vector<double>> mergedValues;
    std::vector<bool> countable;
    const auto addUnionColumn =
        [&](const std::size_t position, const bool answers)
    {
      std::vector<double> unionColumn;
      for (const ContextFolds& fold : folds) {
        unionColumn.insert(unionColumn.end(),
                           fold.chunkValues[position].begin(),
                           fold.chunkValues[position].end());
      }
      mergedValues.push_back(std::move(unionColumn));
      countable.push_back(answers);
    };
    addUnionColumn(
        0, row.timeAvailability == sg::counters::Availability::COUNTABLE);
    // The second column answers only where every context's own column of
    // that position answers (FR-016).
    bool secondaryAnswers = true;
    for (const ContextFolds& fold : folds) {
      if (!fold.countable[1]) {
        secondaryAnswers = false;
        break;
      }
    }
    addUnionColumn(1, secondaryAnswers);
    for (std::size_t position = 0; position < columns.size(); ++position) {
      addUnionColumn(position + 2,
                     !columns[position].unavailable
                         && row.metrics[position].availability
                             == sg::counters::Availability::COUNTABLE);
    }
    row.percentiles =
        sg::detail::buildPercentileColumns(mergedValues, points, countable);
    row.chunkCount = record.chunkCount;
    row.chunkIterations = record.chunkIterations;
    for (const ContextFolds& fold : folds) {
      contextRetained.push_back(fold.chunkValues);
      contextPercentiles.push_back(sg::detail::buildPercentileColumns(
          fold.chunkValues, points, fold.countable));
    }
  }
}

// What one context of a threaded attempt leaves in its slot. The slot
// stands in memory no other thread touches, and the attempt thread
// acquires it through the DONE state of the context's node, the write
// that happens-before that read (FR-049).
struct ThreadedOutcome
{
  std::string error;
  // FR-023, D-07: the failure of the setup half, which the start
  // barrier publishes to every context; the slot above gains a writer
  // after that round, so the two stay apart.
  std::string setupError;
  sg::detail::RunRecord record;
  ContextFolds folds;
  std::uint64_t pauseCount = 0;
  std::optional<std::int64_t> statedSize;
};

// E-06, FR-020: the attempt is one record: the times, the
// iterations, and the samples are summed, the counter store is
// summed ahead of the flag pass, and the label is the one context 0
// ended with (D-04, FR-011).
auto mergeThreadedRecords(const std::vector<ThreadedOutcome>& outcomes,
                          const std::size_t count) -> sg::detail::RunRecord
{
  sg::detail::RunRecord merged;
  for (std::size_t position = 0; position < count; ++position) {
    const sg::detail::RunRecord& record = outcomes[position].record;
    merged.iterations += record.iterations;
    merged.realNs += record.realNs;
    merged.cpuNs += record.cpuNs;
    merged.decisionNs += record.decisionNs;
    merged.samples += record.samples;
    merged.manual = merged.manual || record.manual;
    merged.pauseSamplesFolded =
        merged.pauseSamplesFolded || record.pauseSamplesFolded;
    if (sg::detail::takesSkip(record.skipped, merged.skipped)) {
      merged.skipped = true;
      merged.reason = record.reason;
    }
    // FR-020: every name is summed ahead of the flag pass, and the
    // flags and the base of a name are the first opener's (FR-001).
    for (std::size_t at = 0; at < record.counterNames.size(); ++at) {
      const std::string& counterName = record.counterNames[at];
      const auto found = std::ranges::find(merged.counterNames, counterName);
      if (found == merged.counterNames.end()) {
        merged.counterNames.push_back(counterName);
        merged.counterStore.push_back(record.counterStore[at]);
      } else {
        merged
            .counterStore[static_cast<std::size_t>(
                found - merged.counterNames.begin())]
            .value += record.counterStore[at].value;
      }
    }
  }
  merged.label = outcomes.front().record.label;
  // specs/021 FR-014: the merged run holds the chunks every context
  // captured, and one chunk holds the same iteration count in every
  // context of the attempt.
  for (const ThreadedOutcome& outcome : outcomes) {
    merged.chunkCount += outcome.record.chunkCount;
  }
  merged.chunkIterations = outcomes.front().record.chunkIterations;
  return merged;
}

}  // namespace

namespace sg::detail
{

// FR-027: the note one unavailable metric column prints: the lookup's own
// message, and the refusal kind in parentheses when the table carries the
// name as a refusal. The declaration stands in detail/internal.hpp, where the
// gap test drives both spellings beside the run.
[[nodiscard]] auto metricColumnNote(
    const std::string& name,
    const sg::counters::Error& error,
    const std::vector<sg::counters::MetricListing>& listed) -> std::string
{
  std::string note = error.message;
  if (const auto found =
          std::find_if(listed.begin(),
                       listed.end(),
                       [&](const sg::counters::MetricListing& row)
                       { return row.name == name; });
      found != listed.end() && !found->published)
  {
    note += " (";
    note += sg::counters::metricRefusalName(found->refusal);
    note += ")";
  }
  return note;
}

// FR-025, FR-026, FR-027: the column one metric-name lookup answers. The
// lookup hands back an erased spine, the same carrier `addMetric` of the
// same expression takes, so the column reports the window's summed fold;
// a refusal's kind rides on the line and the column stands unavailable
// while the benchmark runs. The declaration stands beside
// metricColumnNote, where the gap test drives both spellings beside the
// run.
[[nodiscard]] auto metricColumnFor(
    const std::string& name,
    const std::expected<sg::counters::MetricView, sg::counters::Error>& view,
    const std::vector<sg::counters::MetricListing>& listed) -> MetricColumn
{
  MetricColumn column {
      .label = name,
      .perIteration = false,
      .unavailable = false,
      .availability = sg::counters::Availability::COUNTABLE,
      .core = {},
  };
  if (!view.has_value()) {
    column.unavailable = true;
    column.availability = sg::counters::Availability::ABSENT;
    const std::string note =
        detail::metricColumnNote(name, view.error(), listed);
    std::fprintf(stderr, "%s: %s\n", name.c_str(), note.c_str());
    return column;
  }
  MetricColumn resolved {
      .label = name,
      .perIteration = false,
      .unavailable = false,
      .availability = sg::counters::Availability::COUNTABLE,
      .core = view->expression.core,
  };
  return resolved;
}

// FR-020: the skip rule of the threaded merge: a skipped context sets the
// merged flag and leaves its reason standing, so the merged row names the
// first context that skipped. The declaration stands in
// detail/internal.hpp, where the gap test drives the rule's whole input
// table beside the threaded run.
auto takesSkip(const bool recordSkipped,
               const bool mergedSkipped) noexcept -> bool
{
  return recordSkipped && !mergedSkipped;
}

// E-08, FR-021: the harness-owned record of one threaded attempt behind
// the public ThreadRun view. The harness builds the body nodes, keeps
// them alive across the runner call, and reads the ledger when the
// runner returns: a body left undispatched trips the ledger at that
// point, and a body dispatched twice trips at the dispatch call
// (FR-021).
class ThreadRunCore
{
public:
  int contextCount = 0;
  // The contexts of the instance, owned by the run and alive for the
  // whole of the runner call (D-05).
  std::vector<std::unique_ptr<sg::ExecutionContext>>* contexts = nullptr;
  // The harness's dispatch of one body index onto its context.
  std::function<void(int)> dispatchBody;
  // The dispatch ledger of the attempt, one slot per body index.
  std::vector<char> dispatched;
};

// FR-018, E-08: the one erased work item of one body of a threaded
// attempt. The type stands at namespace scope because a work node whose
// item type carries internal linkage trips the subobject-linkage rule
// this build holds (-Wall, -Werror), the same rule that keeps the
// counter seam's node types out of the test's anonymous namespaces.
// NOLINTNEXTLINE(misc-use-internal-linkage) the item type the work node
// instantiation names; internal linkage is what the build refuses
struct ThreadedBodyItem
{
  std::function<void(int)>* body = nullptr;
  int index = 0;

  auto operator()(sg::WorkContext& /*context*/) noexcept -> void
  {
    (*body)(index);
  }
};

// E-01, FR-003: the flag pass of the counters of one repetition. The flags
// apply in the order rate, thread average, iteration-invariant, average over
// iterations, invert, and the declared base scales the result (FR-003, FR-006).
// The thread average divides by one while threads stand out of scope (FR-004,
// Q-3), so that flag carries no divisor here. A divisor of zero leaves the
// flag-neutral value, the case FR-005 discloses, and a value of zero carries
// no reciprocal the row could print, so its flag-neutral value stands as well.
// The declaration stands in detail/internal.hpp, the seam the counter test
// calls through to reach a divisor no run hands the harness.
auto appendCounterColumns(std::vector<double>& printed,
                          const std::vector<std::string>& names,
                          const std::vector<Counter>& store,
                          const double primarySeconds,
                          const std::uint64_t iterations) -> void
{
  printed.reserve(names.size());
  const double count = static_cast<double>(iterations);
  for (std::size_t index = 0; index < names.size(); ++index) {
    const Counter& counter = store[index];
    double value = counter.value;
    // FR-005, N-3: a divisor that settled at zero is skipped and the
    // flag-neutral value stands, so each divisor carries its own test and
    // the flags the divisor cannot reach still apply.
    if ((counter.flags & CounterFlag::RATE) != CounterFlag::NONE
        && primarySeconds > 0.0)
    {
      value /= primarySeconds;
    }
    if ((counter.flags & CounterFlag::ITERATION_INVARIANT) != CounterFlag::NONE)
    {
      value *= count;
    }
    // A measured row arrives with an iteration count of one at the least,
    // because the run loop refuses a row below it (FR-025), so the zero
    // count stands only where a caller hands this pass one directly: a run
    // of the flag pass alone, which the counter test drives (FR-005).
    if ((counter.flags & CounterFlag::AVG_ITERATIONS) != CounterFlag::NONE
        && count > 0.0)
    {
      value /= count;
    }
    // The two-sided test reads the same value of zero as a comparison
    // against zero and leaves the floating-point equality warning aside
    // from a rule that has to name zero exactly (FR-005).
    if ((counter.flags & CounterFlag::INVERT) != CounterFlag::NONE
        && (value < 0.0 || value > 0.0))
    {
      value = 1.0 / value;
    }
    // FR-006, N-2: the base carries the suffix a rendered value shows and
    // divides nothing, so the column prints the flag-applied value.
    printed.push_back(value);
  }
}

auto counterAddressFailure(std::span<const CounterAddress> addresses) -> int
{
  // The host resolves every address through the object it names and that
  // object's own catalog, the two reads the runner makes when it builds a
  // column, so the line states what the host found there (FR-023, FR-087).
  auto& sys = sg::counters::System::local();
  for (const CounterAddress& item : addresses) {
    const auto [objectPath, leafName] = splitAddress(item.address);
    std::string message;
    const auto object = sys.object(objectPath);
    if (!object.has_value()) {
      message = quotedText(object.error().message);
    } else {
      const sg::counters::CatalogEntry* found = nullptr;
      // counters() returns the catalog by value, so the vector has to
      // outlive the search: `found` points into it below.
      const auto catalog = object->counters();
      for (const auto& entry : catalog) {
        if (entry.name == leafName) {
          found = &entry;
          break;
        }
      }
      if (found == nullptr) {
        message = "no such leaf in the catalog (FR-023)";
      } else if (found->avail != sg::counters::Availability::COUNTABLE) {
        message = std::string("listed as ") + availabilityName(found->avail)
            + " (FR-023)";
      }
    }
    if (message.empty()) {
      continue;
    }
    // The line of the words names the option the address came from, so only
    // a file and a variable carry an origin ahead of it (FR-033, FR-087).
    const std::string origin = item.origin.empty() ? "--counter" : item.origin;
    std::fprintf(stderr,
                 "%s: %s: %s\n",
                 origin.c_str(),
                 quotedText(item.address).c_str(),
                 message.c_str());
    return 2;
  }
  return 0;
}

}  // namespace sg::detail

namespace sg
{

// E-08, FR-021: the three members of the attempt view read the
// harness-owned record it stands on. The dispatch member is the ledger
// gate of FR-021: the bound of the index and the twice-dispatched index
// trip here, and the runner reads the ledger for an undispatched body
// when the callable returns.
auto ThreadRun::contextCount() const noexcept -> int
{
  // FR-021, FR-024: the class invariant of the view. The harness fills the
  // count when it builds the attempt, before the callable ever sees the
  // view, so the bound stands for the whole of the runner call.
  SG_INVARIANT(
      m_core != nullptr && m_core->contextCount >= 1,
      "a thread view whose context count stands below 1 " "(FR-021, FR-024)");
  return m_core->contextCount;
}

auto ThreadRun::context(const int index) const noexcept
    -> const ExecutionContext&
{
  SG_REQUIRE(index >= 0 && index < m_core->contextCount,
             "a context index outside the bodies of the attempt violates "
             "the bound 0 <= index < contextCount() (FR-021, FR-024)");
  return *(*m_core->contexts)[static_cast<std::size_t>(index)];
}

auto ThreadRun::dispatch(const int index) -> void
{
  SG_REQUIRE(index >= 0 && index < m_core->contextCount,
             "a dispatch outside the bodies of the attempt violates the "
             "bound 0 <= index < contextCount() (FR-021, FR-024)");
  SG_REQUIRE(m_core->dispatched[static_cast<std::size_t>(index)] == 0,
             "a second dispatch of one body index violates the bound that "
             "every body index of the attempt is dispatched once (FR-021)");
  m_core->dispatched[static_cast<std::size_t>(index)] = 1;
  m_core->dispatchBody(index);
}

// E-01, FR-001: the counter store of one run. The name bound and the
// sixteen-name cap trip as `SG_REQUIRE` preconditions at the call. The
// rule that names an offending name writes a runtime reason into the
// state, because an `SG_REQUIRE` message is a string literal (FR-008).
auto State::setCounter(const std::string_view name,
                       const Counter& counter) -> void
{
  SG_REQUIRE(name.size() < kMaxCounterNameBytes,
             "a counter name longer than kMaxCounterNameBytes minus one "
             "bytes, that is longer than 31 bytes, fails the run (FR-002, "
             "D-01)");
  for (auto& slot : m_counters) {
    if (!slot.used || slot.nameSize != static_cast<std::uint8_t>(name.size())) {
      continue;
    }
    if (std::string_view(slot.name.data(), slot.nameSize) != name) {
      continue;
    }
    if (slot.counter.flags != counter.flags
        || slot.counter.base != counter.base)
    {
      // FR-008 and the Edge Cases rule name the offending name, so the
      // reason is built here and the run ends with it (E-01).
      m_outcome = RunOutcome::FAILED;
      m_reason = "the counter name '" + std::string(name)
               + "' already stands with other flags or another base, and a "
                 "used name keeps the pair it opened with (FR-008)";
      return;
    }
    slot.counter.value = counter.value;
    SG_ENSURE(
        slot.counter.flags == counter.flags
            && slot.counter.base == counter.base,
        "the used slot keeps the flags and the base it opened with " "(FR-"
                                                                     "008)");
    return;
  }
  std::size_t freeSlot = kMaxUserCounters;
  for (std::size_t index = 0; index < m_counters.size(); ++index) {
    if (!m_counters[index].used) {
      freeSlot = index;
      break;
    }
  }
  SG_REQUIRE(freeSlot < kMaxUserCounters,
             "a benchmark carries at most kMaxUserCounters named counters, "
             "that is sixteen names, so a further name fails the run "
             "(FR-001)");
  auto& slot = m_counters[freeSlot];
  // The guard above is a contract, and a contract can stand disabled, so
  // the copy holds itself to the buffer it writes (FR-002).
  const std::size_t length = std::min(name.size(), slot.name.size() - 1);
  std::copy_n(name.begin(), length, slot.name.begin());
  slot.nameSize = static_cast<std::uint8_t>(length);
  slot.counter = counter;
  slot.used = true;
  SG_ENSURE(slot.used && slot.nameSize == static_cast<std::uint8_t>(length),
            "the free slot opens with the name, flags, base, and value of "
            "the call (FR-001)");
}

namespace
{

// D-1 (Constitution II): the two throughput clauses name conditions the code
// can read, so the setters read them. A clause left in documentation alone
// answers nothing. Two passes answer the clauses: where a name stands in the
// store, and whether the store can still carry the call. A template over the
// container keeps the slot type out of the exported header.
template<class Store>
auto counterIndexOf(const Store& counters,
                    const std::string_view name) -> std::size_t
{
  for (std::size_t index = 0; index < counters.size(); ++index) {
    const auto& slot = counters[index];
    if (slot.used && std::string_view(slot.name.data(), slot.nameSize) == name)
    {
      return index;
    }
  }
  return counters.size();
}

// The shared cap is what the precondition reads: the call needs no room when
// its own name already stands, and one free slot otherwise (FR-001).
template<class Store>
auto counterRoom(const Store& counters, const std::size_t own) -> bool
{
  if (own < counters.size()) {
    return true;
  }
  for (const auto& slot : counters) {
    if (!slot.used) {
      return true;
    }
  }
  return false;
}

}  // namespace

// FR-009, E-03: the throughput setters open their one fixed name on the same
// store as any user name, so a body reports throughput without naming a
// column and the two columns share the cap, the flag pass, and the last-write
// rule of the named counters.
auto State::setBytesProcessed(const std::int64_t bytes) -> void
{
  constexpr std::string_view kName = "bytes_per_second";
  const std::size_t before = counterIndexOf(m_counters, kName);
  SG_REQUIRE(counterRoom(m_counters, before),
             "the two throughput columns share the cap of sixteen counter "
             "names, so this call needs its name open or one slot free "
             "(FR-001, FR-009)");
  // A user name that opened this text with another pair keeps it, and the
  // run ends with that reason (FR-008): the write, and with it the
  // postcondition, belongs to the call only on the path it owns.
  [[maybe_unused]] const bool writes = before == m_counters.size()
      || (m_counters[before].counter.flags == CounterFlag::RATE
          && m_counters[before].counter.base == CounterBase::K1024);
  setCounter(
      kName,
      Counter {
          static_cast<double>(bytes), CounterFlag::RATE, CounterBase::K1024});
  [[maybe_unused]] const std::size_t after = counterIndexOf(m_counters, kName);
  SG_ENSURE(!writes
                || (after < m_counters.size()
                    // The stored value is the cast of the call, so the two-sided
                    // test reads the same relation without the equality
                    // the warning set forbids.
                    && !(m_counters[after].counter.value
                             < static_cast<double>(bytes)
                         || m_counters[after].counter.value
                                > static_cast<double>(bytes))
                    && m_counters[after].counter.flags == CounterFlag::RATE
                    && m_counters[after].counter.base == CounterBase::K1024),
            "the run carries bytes_per_second with the value of the call, the "
            "RATE flag and the K1024 base (FR-009)");
}

auto State::setItemsProcessed(const std::int64_t items) -> void
{
  constexpr std::string_view kName = "items_per_second";
  const std::size_t before = counterIndexOf(m_counters, kName);
  SG_REQUIRE(counterRoom(m_counters, before),
             "the two throughput columns share the cap of sixteen counter "
             "names, so this call needs its name open or one slot free "
             "(FR-001, FR-009)");
  [[maybe_unused]] const bool writes = before == m_counters.size()
      || (m_counters[before].counter.flags == CounterFlag::RATE
          && m_counters[before].counter.base == CounterBase::K1000);
  setCounter(
      kName,
      Counter {
          static_cast<double>(items), CounterFlag::RATE, CounterBase::K1000});
  [[maybe_unused]] const std::size_t after = counterIndexOf(m_counters, kName);
  SG_ENSURE(!writes
                || (after < m_counters.size()
                    // The stored value is the cast of the call, so the two-sided
                    // test reads the same relation without the equality
                    // the warning set forbids.
                    && !(m_counters[after].counter.value
                             < static_cast<double>(items)
                         || m_counters[after].counter.value
                                > static_cast<double>(items))
                    && m_counters[after].counter.flags == CounterFlag::RATE
                    && m_counters[after].counter.base == CounterBase::K1000),
            "the run carries items_per_second with the value of the call, the "
            "RATE flag and the K1000 base (FR-009)");
}

// D-04, FR-011: one string per repetition, and the newline bound is the
// one-line-per-row shape the report-shape checks assert. The rule that names
// the breach needs no runtime text, so the precondition carries the literal.
auto State::setLabel(const std::string_view label) -> void
{
  SG_REQUIRE(label.find('\n') == std::string_view::npos,
             "a label containing a newline character breaks the one line per "
             "row of the report, so the call violates its precondition "
             "(FR-011, spec Edge Cases)");
  m_label.assign(label);
  SG_ENSURE(m_label == label,
            "the repetition carries the label of the call " "(FR-011)");
}

}  // namespace sg

namespace sg::detail
{

// FR-051, FR-053: the two reservation seams the pause suite reads.
// `Runner::run` writes the first before every attempt samples and keeps
// the second at the largest room any of those reservations held.
auto lastRecorderReservation() -> RecorderReservation&
{
  static RecorderReservation last;
  return last;
}

auto maxRecorderPauseRoom() -> std::size_t&
{
  static std::size_t maximum = 0;
  return maximum;
}

// FR-052: the commit of one folded pair. The slot holds exactly the
// pair's two points, so the fold spans points zero and one, and the
// same two raw counter points the raw path would have retained answer
// it through the counters library alone. The totals a run accumulates
// this way reproduce the netted columns of the raw path (SC-13).
auto commitPauseFoldPair(PauseFoldSink& sink,
                         const sg::counters::RecorderApi& slot) noexcept -> void
{
  for (std::size_t column = 0; column < sink.columns.size(); ++column) {
    const auto folded =
        sg::counters::detail::foldCore(*sink.columns[column], slot, 0, 1);
    sink.values[column] += folded.value;
    if (folded.availability > sink.availability[column]) {
      sink.availability[column] = folded.availability;
    }
  }
  ++sink.foldedPairs;
}

auto Runner::run(Instance& instance) -> BenchmarkResult
{
  // The callable, the run control, and the metrics ride the family
  // record; the instance carries the name it runs under and the
  // arguments it owns (R-02).
  RegistryEntry& entry = *instance.family;
  entry.runStarted = true;

  BenchmarkResult result;
  result.name = instance.name;
  // FR-021, R-07: the split reaches the caller through these two
  // fields; the console row keeps the single H1 name column.
  result.suite = instance.suite;
  result.caseName = instance.caseName;
  // FR-015, FR-027: the report scales every printed time value of this
  // result by the resolved unit: the family's `unit` call, else the
  // batch snapshot `speedgunMain` took at its entry, else the process
  // default read at this run's start.
  result.timeUnit = entry.timeUnit.value_or(
      batchDefaultTimeUnit().value_or(getDefaultTimeUnit()));
  // specs/021 FR-001, FR-010: the report labels the second time column of
  // the percentile lines with the field the primary is not.
  result.primaryIsCpu = !entry.realTime;
  // FR-016, FR-017: either mode of the family trims its report; whether an
  // aggregate row exists to show instead is the report's decision, because
  // a family with no aggregate keeps its rows (FR-018).
  result.hideRepetitionRows =
      entry.reportAggregatesOnly || entry.displayAggregatesOnly;
  // specs/023 E-01: the chunk columns this run retains stand beside the
  // rows they answer, in the order those rows are built.
  m_rowRaw.emplace_back();
  m_contextRaw.emplace_back();
  // specs/027 E-06, R-04: this run's windows stand alone in the store
  // the caller takes after the run, and the warm-up and the measured
  // repetitions name their pass below.
  m_runWindows.clear();
  m_repetition = 0;

  auto& sys = sg::counters::System::local();
  const std::int64_t minTimeNs =
      entry.minTimeNs.value_or(m_options.minTimeNs.value_or(kDefaultMinTimeNs));
  const std::int64_t warmupNs = entry.warmupTimeNs.value_or(
      m_options.warmupTimeNs.value_or(kDefaultWarmupNs));
  const std::uint64_t repetitions = entry.repetitions.value_or(
      m_options.repetitions.value_or(kDefaultRepetitions));
  const std::optional<std::uint64_t> fixedIterations =
      entry.fixedIterations ? entry.fixedIterations : m_options.fixedIterations;
  const bool dryRun = m_options.dryRun;

  // specs/021 FR-020, FR-023: the command line wins over the family, and a
  // run that reports no percentile captures no chunk points, so the default
  // path keeps the bytes and the cost it has today (FR-001, Principle VII).
  if (m_options.showPercentiles.value_or(entry.showPercentiles)) {
    result.percentilePoints =
        normalizePercentilePoints(m_options.percentilePoints);
  }

  // D-05, FR-006: the threaded instance's contexts, created when a
  // threaded instance's sampling begins and joined when this run
  // returns. The holder ends every context with the legal triple: the
  // stop request, the drain of every accepted node, and the join, and
  // it runs on every exit of this run, the early returns included
  // (FR-006). Each context stands null after its join, so the vector's
  // own destruction stays a no-op over joined slots.
  std::vector<std::unique_ptr<sg::ExecutionContext>> contexts;

  struct ContextJoiner
  {
    std::vector<std::unique_ptr<sg::ExecutionContext>>* owned;

    explicit ContextJoiner(
        std::vector<std::unique_ptr<sg::ExecutionContext>>* owner)
        : owned {owner}
    {
    }

    ContextJoiner(const ContextJoiner&) = delete;
    auto operator=(const ContextJoiner&) -> ContextJoiner& = delete;

    ~ContextJoiner()
    {
      // Every element of the vector stands on a constructed context: the run
      // appends each one with `push_back`, which appends only once the
      // construction has returned, so no element ever stands null (FR-006).
      for (auto& context : *owned) {
        context->requestStop();
        context->drain();
        context->join();
        context.reset();
      }
    }
  } contextJoiner {&contexts};

  auto fail = [&](const std::string& reason) -> BenchmarkResult
  {
    result.outcome = RunOutcome::FAILED;
    result.reason = reason;
    return result;
  };

  const auto machine = sys.object("machine");
  SG_ASSERT(machine.has_value(),
            "the counters system opens the host provider, and that provider "
            "publishes the machine object (FR-002)");
  const auto monotonic = machine->counter<sg::counters::Dim<1, 0>>("monotonic");
  SG_ASSERT(monotonic.has_value(),
            "the host provider publishes machine/monotonic as its clock leaf "
            "(FR-002)");
  // D-02, FR-004: one CPU leaf per benchmark, chosen once beside the
  // monotonic resolution. The family's process-CPU switch moves the
  // leaf to machine/process_cpu, and a leaf the host does not publish
  // fails this benchmark with the resolution line.
  const std::string_view cpuLeafName =
      entry.processCpuTime ? "process_cpu" : "thread_cpu";
  const auto cpuLeaf = machine->counter<sg::counters::Dim<1, 0>>(cpuLeafName);
  if (!cpuLeaf.has_value()) {
    return fail("machine/" + std::string(cpuLeafName)
                + " does not resolve: " + cpuLeaf.error().message);
  }

  sg::counters::Expression<sg::counters::Dim<1, 0>> realTime(*monotonic);
  sg::counters::Expression<sg::counters::Dim<1, 0>> cpuTime(*cpuLeaf);

  std::vector<MetricColumn> columns;
  columns.reserve(entry.metrics.size() + m_options.counterAddresses.size());
  for (const auto& seed : entry.metrics) {
    columns.push_back(MetricColumn {
        .label = seed.label,
        .perIteration = seed.perIteration,
        .unavailable = false,
        .availability = sg::counters::Availability::COUNTABLE,
        .core = seed.core,
    });
  }

  // R-04: an address-attached leaf resolves through the catalog and is
  // instantiated as the pair its unit names, so it reaches the
  // per-iteration rule of FR-022.
  for (const auto& address : m_options.counterAddresses) {
    const auto [objectPath, leafName] = splitAddress(address);
    MetricColumn column {
        .label = address,
        .perIteration = true,
        .unavailable = false,
        .availability = sg::counters::Availability::COUNTABLE,
        .core = {},
    };

    const auto object = sys.object(objectPath);
    if (!object.has_value()) {
      column.unavailable = true;
      column.availability = sg::counters::Availability::ABSENT;
      std::fprintf(stderr,
                   "%s: %s\n",
                   quotedText(address).c_str(),
                   quotedText(object.error().message).c_str());
      columns.push_back(std::move(column));
      continue;
    }

    const sg::counters::CatalogEntry* found = nullptr;
    // counters() returns the catalog by value, so the vector has to
    // outlive the search: `found` points into it below.
    const auto catalog = object->counters();
    for (const auto& catalogEntry : catalog) {
      if (catalogEntry.name == leafName) {
        found = &catalogEntry;
        break;
      }
    }
    if (found == nullptr) {
      column.unavailable = true;
      column.availability = sg::counters::Availability::ABSENT;
      std::fprintf(stderr,
                   "%s: no such leaf in the catalog (FR-023)\n",
                   quotedText(address).c_str());
      columns.push_back(std::move(column));
      continue;
    }

    const auto dimension = sg::counters::dimensionOf(found->unit);
    SG_ASSERT(dimension.has_value(),
              "the counters system maps every unit name a provider registers, "
              "so a catalog entry always carries a dimension (FR-005)");

    if (dimension->time == 1) {
      SG_ASSERT(dimension->events == 0,
                "the unit table maps a time unit to no event exponent, so a "
                "catalog entry with a time exponent carries no event exponent "
                "(FR-005)");
      const auto leaf = object->counter<sg::counters::Dim<1, 0>>(leafName);
      SG_ASSERT(leaf.has_value(),
                "the entry's unit picks the dimension the lookup asks for, and "
                "the object lists that leaf (FR-005)");
      const sg::counters::Expression<sg::counters::Dim<1, 0>> expression(*leaf);
      column.core = expression.core;
    } else {
      const auto leaf = object->counter<sg::counters::Dim<0, 1>>(leafName);
      SG_ASSERT(leaf.has_value(),
                "the entry's unit picks the dimension the lookup asks for, and "
                "the object lists that leaf (FR-005)");
      const sg::counters::Expression<sg::counters::Dim<0, 1>> expression(*leaf);
      column.core = expression.core;
    }
    columns.push_back(std::move(column));
  }

  // FR-025, FR-026, FR-027: a named metric joins the columns in
  // command-line order, keyed by the name as typed, so a name repeated on
  // the command line or attached in code yields one column. The spine the
  // lookup returns is the metric's own, so the column folds through the
  // plan and the recorder like any other column and the timed loop reads
  // no counter of its own (FR-029).
  for (const auto& name : m_options.metricNames) {
    const bool taken = std::any_of(columns.begin(),
                                   columns.end(),
                                   [&](const MetricColumn& column)
                                   { return column.label == name; });
    if (taken) {
      continue;
    }
    columns.push_back(detail::metricColumnFor(name,
                                              sg::counters::metricByName(name),
                                              sg::counters::catalogMetrics()));
  }

  std::vector<const sg::counters::detail::ExprCore*> cores;
  cores.reserve(columns.size() + 2);
  cores.push_back(&realTime.core);
  cores.push_back(&cpuTime.core);
  for (auto& column : columns) {
    result.metricLabels.push_back(column.label);
    if (!column.unavailable) {
      // FR-052: the sink's totals stand in the order of these cores.
      column.foldColumn = cores.size();
      cores.push_back(&column.core);
    }
  }

  // E-03, FR-008 (specs/024-multipass-collection): the pass plan of the
  // columns this run measures, over the programmable-counter limit the
  // source of each leaf publishes. A leaf's source is the object its
  // address names, the object that publishes the fact (E-02): the
  // addresses come out of the resolved spines above and the limit out of
  // the catalog entry beside them, so the planner reads no counter and
  // opens no window (FR-008).
  std::vector<std::string> sources;
  std::vector<std::optional<std::size_t>> sourceLimits;
  std::vector<std::vector<sg::detail::ColumnLeaves>> columnLeaves;
  columnLeaves.reserve(columns.size());
  for (const auto& column : columns) {
    std::vector<sg::detail::ColumnLeaves> groups;
    for (const auto& leaf : column.core.leaves) {
      const std::string_view objectPath = splitAddress(leaf.address).first;
      auto located = std::find(sources.begin(), sources.end(), objectPath);
      if (located == sources.end()) {
        const auto object = sys.object(objectPath);
        SG_ASSERT(object.has_value(),
                  "every leaf of a metric column resolved against this open "
                  "tree, so the object its address names is one it "
                  "publishes");
        sources.emplace_back(objectPath);
        sourceLimits.push_back(object->programmableCounters());
        located = sources.end() - 1;
      }
      const auto source = static_cast<std::size_t>(located - sources.begin());
      auto group =
          std::find_if(groups.begin(),
                       groups.end(),
                       [source](const sg::detail::ColumnLeaves& leafGroup)
                       { return leafGroup.source == source; });
      if (group == groups.end()) {
        // One group per source the column spans: a leaf of a source already
        // grouped here joins that group, whose addresses the planner counts
        // once (E-03 rule 2).
        groups.push_back(
            sg::detail::ColumnLeaves {.source = source, .addresses = {}});
        group = groups.end() - 1;
      }
      group->addresses.push_back(leaf.address);
    }
    columnLeaves.push_back(std::move(groups));
  }
  // E-01, FR-024, FR-025, FR-007, research D-09: the run settles its
  // collection method once, here, before its first window opens. The command
  // line's word wins over the family's own request and over `auto`, on the
  // `--percentiles` precedent; the limit facts of the sources these columns
  // need and the count of the columns answer the rest of the table. One call
  // settles one run: nothing downstream re-decides.
  bool anyLimit = false;
  for (const auto& limit : sourceLimits) {
    anyLimit = anyLimit || limit.has_value();
  }
  const auto collectionMethod = sg::detail::settleMethod(
      m_options.metricMethod.value_or(entry.metricMethod),
      anyLimit,
      columns.size());

  // E-01, FR-024: only a settling of `passes` runs the planner. Any other
  // settling leaves the plan of the audit point standing, so the run keeps the
  // single `compileCore` call over every column and the kernel rotates the
  // whole spine (FR-025, research D-10).
  auto planning = collectionMethod == MetricMethod::PASSES
      ? sg::detail::planPasses(columnLeaves, sourceLimits)
      : sg::detail::PassPlan {};

  // Every column of the run in request order: the spine the audit point
  // compiled, and the spine a run with no plan of its own measures whole.
  std::vector<std::size_t> wholeRun;
  wholeRun.reserve(columns.size());
  for (std::size_t index = 0; index < columns.size(); ++index) {
    wholeRun.push_back(index);
  }

  // E-03 rule 7, E-01: when no source the run needs publishes a limit, no
  // plan stands and the run keeps the one kernel-multiplexed plan of the
  // audit point, so the pass list holds that single plan over every
  // column.
  std::vector<std::vector<std::size_t>> passColumns = planning.columns;
  if (passColumns.empty()) {
    passColumns.push_back(wholeRun);
  }
  // specs/029 PD-12: a hunt run appends passes to this list while the loop
  // reads it, so the count the loop bounds itself with moves with the list.
  std::size_t passCount = passColumns.size();

  // E-06, FR-022: the pass index the attempt-level windows carry. The
  // window-stamping lambdas below read it through their reference
  // capture. The warm-up and the calibration stand ahead of pass 0, so
  // their windows carry 0 the way their repetition index does.
  std::size_t pass = 0;

  // E-07, FR-017, FR-019: the disclosure of a collection whose method has
  // settled. Every return that follows this settling publishes it through this
  // one call, the stops inside the pass loop included, so a run stopped
  // mid-collection names the passes it folded and leaves the plan it was
  // given unpublished. The spread and the per-value pass tag belong to the
  // merge, the writer of both, so a stopped collection keeps the empty answer
  // for them and nothing resets them here. A settling of `AUTO`, the run that
  // carries no metric column, publishes no count.
  const auto publishSettled = [&](std::size_t standingPasses) noexcept -> void
  {
    result.metricMethod = collectionMethod;
    result.passCount =
        collectionMethod == MetricMethod::AUTO ? 0 : standingPasses;
  };

  // E-04, FR-009, FR-012: the seat of one pass. The two time leaves ride
  // every pass and consume no programmable counter (E-03 rule 6); the
  // columns the pass measures join them in request order, and that leaf
  // list is what this run's `compileCore` call compiles. Seating refreshes
  // the plan, the fold column of every column of the run, the mark of the
  // columns this pass measures, the two-point reset slot and the overhead
  // floor the plan measured. R-02, FR-014, FR-051: the reset handle is a
  // trivially copyable cursor over the plan's arena, so it is copied into
  // the attempt bodies at bind and reaches neither a shared pointer nor a
  // lock; two points stand in the slot between attempts, the reset
  // restores the pair the netted fold reads, and a value left over from an
  // earlier attempt can never reach a row.
  std::optional<sg::counters::Plan> plan;
  sg::counters::RecorderHandle<sg::counters::HardStop> slotReset {};
  double overheadFloorNs = 0.0;
  std::vector<bool> measuredColumn(columns.size(), true);
  const auto seatPass = [&](const std::vector<std::size_t>& columnsOfPass)
      -> std::expected<void, sg::counters::Error>
  {
    cores.clear();
    cores.push_back(&realTime.core);
    cores.push_back(&cpuTime.core);
    measuredColumn.assign(columns.size(), false);
    for (const std::size_t index : columnsOfPass) {
      measuredColumn[index] = true;
    }
    for (std::size_t index = 0; index < columns.size(); ++index) {
      if (measuredColumn[index] && !columns[index].unavailable) {
        columns[index].foldColumn = cores.size();
        cores.push_back(&columns[index].core);
      }
    }
    auto seated =
        sg::counters::detail::compileCore(sys, sg::counters::Target {}, cores);
    if (!seated.has_value()) {
      // specs/029 D-18, T073: the refusal travels as the error the counter
      // layer answered, so a seat that fails inside the pass loop still names
      // the cause the host gave and not a message this file wrote over it.
      return std::expected<void, sg::counters::Error> {
          std::unexpect,
          sg::counters::Error {
              .message =
                  "the run plan does not compile: " + seated.error().message,
              .suggestions = seated.error().suggestions,
          }};
    }
    plan = std::move(*seated);
    slotReset = plan->recorder(2);
    overheadFloorNs = plan->sampleOverheadNsMedian();
    return {};
  };
  // The seat of the first pass stands before the warm-up and the
  // calibration, so those measured runs sample the same leaf list the pass
  // itself samples. A run of one pass seats every column, the spine the
  // audit point compiled; a run the plan split seats its first pass's own
  // leaf list, because the whole spine is the leaf list that does not fit
  // the source's programmable-counter limit (FR-004).
  // specs/029 spec PC-9, contracts/hunt-api.md: this seat is the run's one
  // clean failure and a hunt run keeps it, because the same compile serves
  // warm-up and calibration. A host that refuses the first pass's group
  // therefore ends the run on the failure route with exit 1 naming the
  // refusal; the retry of D-18 belongs to a pass after the first, inside the
  // loop below, where the seat is a runtime condition and not the ground the
  // measured numbers stand on.
  if (const auto seated = seatPass(passColumns.front()); !seated.has_value()) {
    return fail(seated.error().message);
  }

  // FR-023: the availability of every leaf a metric needs is read
  // before the run, and an uncountable metric stays out of the run
  // while the benchmark itself still runs.
  for (auto& column : columns) {
    if (column.unavailable) {
      continue;
    }
    for ([[maybe_unused]] const auto& leaf : column.core.leaves) {
      SG_ASSERT(leaf.avail == sg::counters::Availability::COUNTABLE,
                "the plan compiler refuses a leaf the catalog does not mark "
                "countable, so a compiled plan holds only countable leaves "
                "(FR-023)");
    }
  }

  // R-02, FR-014, FR-051: one recorder per attempt, reserved at the
  // attempt's start: the endpoint shape, the two window points of the
  // one run the attempt starts, beside the pause room the family's
  // history earns. FR-053: a family without pause history reserves the
  // endpoint shape alone. FR-026: a pair that outruns the room folds
  // through the pass's two-point slot, minted when the pass takes its
  // seat, so a run that pauses in every iteration completes.

  // The attempt's recorder, set anew when every attempt reserves, and
  // read once more when the measured row folds its columns.
  std::optional<sg::counters::RecorderHandle<sg::counters::HardStop>>
      attemptRecorder;

  // FR-019: the problem size a body stated with `setComplexityN`, the N the
  // complexity fit of the family reads for this instance. The value leaves the
  // sampling run here and reaches the samples at the foot of the run.
  std::optional<std::int64_t> complexityN;

  // R-06: one flag read after each run ends the benchmark with this
  // reason, and the process exits nonzero.

  // FR-020: the folds of the last threaded attempt in context-index
  // order. The merged row build reads these folds and no recorder, so
  // the row carries each context's own fold of its own plan (FR-018,
  // PC-1).
  std::vector<ContextFolds> threadedFolds;

  // D-05, FR-004, Q-2: the threaded instance's contexts stand once for
  // the instance's whole run: they are created here, they serve the
  // warm-up, the calibration, and every repetition, and the holder
  // joined beside the run result ends each of them when this run
  // returns (FR-006). The threaded policy is the harness default: SPIN
  // waits and no pin. specs/025 FR-008, FR-010, FR-019, D-1: when the
  // instance carries a preset other than `none`, the plan of that preset
  // over the machine layout names each context's cpu by position; the
  // batch fit check has already stopped an unfittable count, so the plan
  // stands full here.
  if (instance.threadCount > 1) {
    const std::optional<detail::PlacementPlan> placePlan =
        instance.placement != detail::Preset::NONE
        ? std::optional<detail::PlacementPlan>(detail::resolvePreset(
              instance.placement, detail::layout(), instance.threadCount))
        : std::nullopt;
    for (int position = 0; position < instance.threadCount; ++position) {
      const sg::ContextPolicy policy {
          sg::WaitMode::SPIN,
          placePlan.has_value()
              ? sg::PlacementSlot::cpu(
                    placePlan->cpu[static_cast<std::size_t>(position)])
              : sg::PlacementSlot::noPin()};
      contexts.push_back(std::make_unique<sg::ExecutionContext>(policy));
    }
  }

  // FR-005, FR-021, FR-025: the disclosure records each context's own
  // wait mode and the record that context's placement function returned,
  // which names a slot that never landed. The fill runs after a sample,
  // where the run has met every context across a barrier and a node
  // completion, so the record it reads is one that thread published and
  // not a read that runs ahead of it (FR-049); it still stands before the
  // joiner ends any thread (FR-025). The vector stays empty for a
  // single-context run (FR-022) and for a threaded run that ended before
  // it sampled, whose contexts placed nothing.
  const auto fillDisclosures = [&result, &contexts]
  {
    result.contexts.clear();
    for (const auto& context : contexts) {
      const sg::PlacementResult placement =
          sg::detail::placementRecordOf(*context);
      result.contexts.push_back(
          sg::ContextDisclosure {context->policy().waitMode,
                                 std::string(placement.description),
                                 placement.applied});
    }
  };

  // E-08, FR-051: two points per closed pause pair the previous
  // attempt observed per iteration, times this attempt's iteration
  // count, and no more: FR-025 and Q-10 cap the room, and an attempt
  // over the cap reserves no room and folds instead.
  auto pauseRoomFor =
      [&](const std::uint64_t iterations) noexcept -> std::size_t
  {
    const std::uint64_t history = entry.pausePairsPerIteration;
    if (history > 0 && 2 * history <= kMaxPauseSamplesPerLeaf
        && iterations <= kMaxPauseSamplesPerLeaf / (2 * history))
    {
      return static_cast<std::size_t>(2 * history * iterations);
    }
    return 0;
  };

  // specs/021 FR-005, FR-007: the chunk plan of an attempt. The 1% rule
  // calibrates the chunk size from the sample cost this plan measured and
  // the per-iteration time the previous attempt of this instance measured;
  // an attempt that reports no percentile captures no chunks, and the
  // first attempt of a fixed-count run has no measured iteration time to
  // calibrate from, so the point ceiling of FR-006 alone sets its size.
  auto chunkPlanOf =
      [&](const std::uint64_t iterations,
          const std::size_t pauseRoom) noexcept -> sg::detail::ChunkPlan
  {
    if (result.percentilePoints.empty()
        && m_options.rawContent != sg::RawContent::CHUNKS)
    {
      return sg::detail::ChunkPlan {
          .chunkIterations = 0,
          .chunkCount = 0,
          .pointCapacity = static_cast<std::size_t>(2) + pauseRoom,
      };
    }
    return sg::detail::chunkPlanFor(
        iterations, entry.iterationNs, overheadFloorNs, pauseRoom);
  };

  // The record tail of one sampled run, shared by the single-context
  // sampler and by every context of a threaded attempt: the FR-008
  // failure, the loop-shape rules of FR-017, the span and the folds of
  // the run's own recorder, the decision rule of FR-002 and D-05, the
  // counter store of FR-001, and the label and skip of the run. Each
  // attempt hands in its own state, its own recorder, and its own
  // record, so no fold ever reads another context's plan (FR-018,
  // PC-1).
  auto buildRecord =
      [&](State& state,
          const sg::counters::RecorderHandle<sg::counters::HardStop>& recorder,
          RunRecord& record) -> std::expected<void, std::string>
  {
    // FR-008: a second setCounter for a used name with another flags
    // value or another base fails the run naming that name. The reason
    // is a runtime text the state holds, so the run ends here, ahead of
    // the loop-shape rules that read the state (E-01).
    if (state.outcome() == RunOutcome::FAILED) {
      return std::unexpected(state.reason());
    }

    // FR-017: a measured run owns one complete timed loop. A function
    // that never entered it, entered it twice, or left it early without
    // a skip has no single measured window to report.
    if (state.outcome() != RunOutcome::SKIPPED) {
      if (state.loopsStarted() == 0) {
        return std::unexpected(
            "the benchmark never entered the timed loop (FR-017)");
      }
      if (state.loopsStarted() > 1) {
        return std::unexpected(
            "the benchmark entered the timed loop twice (FR-017)");
      }
      if (!state.loopCompleted()) {
        return std::unexpected(
            "the benchmark left the timed loop without a skip (FR-017)");
      }
    }

    // E-07, FR-013: the span is the run's own sample count, read from
    // the state after the run, and the pause intervals are the state's
    // ordinals inside that span. FR-052: the fold span is the state's
    // recorder points, the slot's two standing outside it, and the
    // pairs that folded stand in the sink's totals. The fold's pause
    // pair is subtracted from the span, so the printed time excludes
    // the windows the body handed over, and the fold runs once after
    // the run's final top-up, outside the timed region (FR-014,
    // FR-015).
    record.samples = state.m_samplesTaken;
    record.foldPoints = state.m_foldPoints;
    record.pauseSamplesFolded = state.m_foldedPairs > 0;
    record.pausePairs = state.m_pausePairs;
    record.realNs = static_cast<std::int64_t>(
        std::llround(nettedFold(realTime.core, recorder, record, 0).value));
    record.cpuNs = static_cast<std::int64_t>(
        std::llround(nettedFold(cpuTime.core, recorder, record, 1).value));

    // FR-002, D-05: the primary time is the CPU fold by default and the
    // real fold when the family asked for it. A manual run reports the
    // sum the benchmark supplied: the real column and the value the
    // growth rule reads are that sum, read from the state after the run,
    // and the manual flag stands so the fivefold stop leaves the run
    // alone (FR-006, FR-007).
    if (entry.manualTime) {
      record.realNs = state.m_manualNs;
      record.decisionNs = state.m_manualNs;
      record.manual = true;
    } else {
      record.decisionNs = entry.realTime ? record.realNs : record.cpuNs;
    }
    // E-01, FR-001: the counter store leaves the run with the record.
    // The names copy out of the fixed slots, so the flag pass and the
    // row build read them after the loop (FR-003).
    for (const auto& slot : state.m_counters) {
      if (slot.used) {
        record.counterNames.emplace_back(slot.name.data(), slot.nameSize);
        record.counterStore.push_back(slot.counter);
      }
    }
    // D-04, FR-011: the label leaves the run with the record, one copy
    // for the repetition the record describes.
    record.label = state.m_label;
    if (state.outcome() == RunOutcome::SKIPPED) {
      record.skipped = true;
      record.reason = state.reason();
    }
    return {};
  };

  // FR-023, D-07: one half of the context 0 fixture pair of a
  // threaded attempt, named so each body keeps its own complexity.
  auto fixtureHalf = [&](bool setupHalf,
                         State& callbackState,
                         std::unique_ptr<Fixture>& fixture,
                         std::string& error)
  {
    try {
      if (setupHalf) {
        if (entry.setup) {
          entry.setup(callbackState);
        }
        if (entry.fixtureFactory) {
          fixture = entry.fixtureFactory();
          fixture->setUp(callbackState);
        }
      } else {
        if (fixture) {
          fixture->tearDown(callbackState);
        }
        if (entry.teardown) {
          entry.teardown(callbackState);
        }
      }
    } catch (const std::exception& thrown) {
      error = thrown.what();
    } catch (...) {
      error = "the benchmark threw an unknown exception";
    }
  };

  // FR-018: one context folds its own plan on its own thread into the
  // folds it owns; the slot of an unavailable column carries no fold.
  auto foldThreadedContext =
      [&](auto& recorderHere, RunRecord& record, ContextFolds& folds)
  {
    const sg::counters::detail::ExprCore* primaryCore = selectPrimaryCore(
        entry.manualTime, entry.realTime, realTime.core, cpuTime.core);
    if (primaryCore != nullptr) {
      folds.primary =
          nettedFold(*primaryCore,
                     recorderHere,
                     record,
                     static_cast<std::size_t>(entry.realTime ? 0 : 1));
    }
    for (std::size_t position = 0; position < columns.size(); ++position) {
      const auto& column = columns[position];
      // specs/024 FR-009, FR-014: a column this pass does not measure has no
      // leaf in this pass's plan, so no fold stands in its slot; the merge
      // brings the value of the pass that measured the column.
      if (column.unavailable || !measuredColumn[position]) {
        folds.columns.push_back(sg::counters::MetricResult {});
        continue;
      }
      folds.columns.push_back(
          nettedFold(column.core, recorderHere, record, column.foldColumn));
    }
    // specs/021 FR-004, FR-014: this context's own chunk values, folded on
    // this thread from this context's recorder and divided by the
    // iterations one chunk holds before any other thread reads them.
    if (record.chunkCount == 0) {
      return;
    }
    const auto api = recorderHere.view();
    std::vector<bool> perIteration;
    if (primaryCore != nullptr) {
      folds.chunkValues.push_back(
          sg::detail::buildChunkValues(*primaryCore,
                                       api,
                                       record.basePoint,
                                       record.chunkCount,
                                       record.pausePairs));
      folds.countable.push_back(folds.primary->availability
                                == sg::counters::Availability::COUNTABLE);
      perIteration.push_back(true);
    } else {
      folds.chunkValues.emplace_back();
      folds.countable.push_back(false);
      perIteration.push_back(false);
    }
    // specs/021 SC-001: the time column the primary is not answers too.
    const sg::counters::detail::ExprCore& secondaryCore =
        entry.realTime ? cpuTime.core : realTime.core;
    folds.chunkValues.push_back(
        sg::detail::buildChunkValues(secondaryCore,
                                     api,
                                     record.basePoint,
                                     record.chunkCount,
                                     record.pausePairs));
    folds.countable.push_back(
        nettedFold(secondaryCore,
                   recorderHere,
                   record,
                   static_cast<std::size_t>(entry.realTime ? 1 : 0))
            .availability
        == sg::counters::Availability::COUNTABLE);
    perIteration.push_back(true);
    for (std::size_t position = 0; position < columns.size(); ++position) {
      if (columns[position].unavailable || !measuredColumn[position]) {
        folds.chunkValues.emplace_back();
        folds.countable.push_back(false);
        perIteration.push_back(false);
        continue;
      }
      folds.chunkValues.push_back(
          sg::detail::buildChunkValues(columns[position].core,
                                       api,
                                       record.basePoint,
                                       record.chunkCount,
                                       record.pausePairs));
      folds.countable.push_back(folds.columns[position].availability
                                == sg::counters::Availability::COUNTABLE);
      perIteration.push_back(columns[position].perIteration);
    }
    divideChunkValues(folds.chunkValues, perIteration, record.chunkIterations);
  };

  // FR-012, FR-017, FR-049: one heap node per body index, the
  // default dispatch to the context of that index, and the wait that
  // ends when every body has settled.
  auto launchBodies =
      [&](std::function<void(int)>& body, const std::size_t count)
  {
    // One node per body index, the item erased through
    // ThreadedBodyItem. Every node stands on the heap before the first
    // dispatch, so no node moves while a context holds its address
    // (FR-012).
    std::vector<std::unique_ptr<sg::WorkNode<ThreadedBodyItem>>> nodes;
    nodes.reserve(count);
    for (std::size_t position = 0; position < count; ++position) {
      nodes.push_back(std::make_unique<sg::WorkNode<ThreadedBodyItem>>(
          ThreadedBodyItem {&body, static_cast<int>(position)}));
    }

    if (entry.threadRunner) {
      // FR-021, E-08: the family's own callable performs the
      // dispatches. The harness still owns the contexts, the body
      // nodes, the two barrier rounds and the merge: the view hands out
      // the context of an index and the dispatch of that index's body
      // node, and the ledger under it records what the call dispatched.
      // The call runs here, on the thread running the attempt, which
      // dispatches the bodies and runs no body of its own (FR-046,
      // D-05). The nodes and the contexts stand in this scope until
      // every body has settled below, so they outlive the call (FR-012,
      // FR-021, D-05).
      ThreadRunCore core;
      core.contextCount = static_cast<int>(count);
      core.contexts = &contexts;
      core.dispatched.assign(count, 0);
      core.dispatchBody = [&](const int index) noexcept -> void
      {
        sg::dispatch(*contexts[static_cast<std::size_t>(index)],
                     *nodes[static_cast<std::size_t>(index)]);
      };
      ThreadRun view(core);
      entry.threadRunner(view);
      // FR-021: a body index the call left undispatched trips the
      // ledger here, at the point the callable returned; a twice
      // dispatched index tripped at the dispatch call itself.
      for (std::size_t position = 0; position < count; ++position) {
        SG_REQUIRE(core.dispatched[position] != 0,
                   "the runner callable returned with a body index of the "
                   "attempt undispatched, which violates the bound that "
                   "every body index stands dispatched once before the "
                   "runner returns (FR-021)");
      }
    } else {
      // The default dispatch (FR-017): the harness hands every body
      // index to the context of that index, on the thread running the
      // attempt, which runs no body of its own (FR-046, D-05).
      for (std::size_t position = 0; position < count; ++position) {
        sg::dispatch(*contexts[position], *nodes[position]);
      }
    }
    // The attempt ends when every body has settled. Each context
    // writes its node's state with release order after its item, so
    // the reads below acquire every slot of outcomes (FR-049). A round
    // never times out, so this wait carries no bound and needs none
    // (FR-014).
    for (const auto& node : nodes) {
      while (node->state() != sg::WorkNodeState::DONE) {
        std::this_thread::yield();
      }
    }
  };

  // The attempt answers with the first body's error, if any context
  // closed its slot with one.
  auto firstThreadedError = [&](const std::vector<ThreadedOutcome>& outcomes)
      -> std::optional<std::string>
  {
    for (const ThreadedOutcome& outcome : outcomes) {
      if (!outcome.error.empty()) {
        return outcome.error;
      }
    }
    return std::nullopt;
  };

  // E-08, FR-051: the history rises to the context that closed the
  // most pause pairs, the same rule the single-context sampler
  // applies.
  auto risePauseHistory = [&](const std::vector<ThreadedOutcome>& outcomes,
                              const std::uint64_t iterations)
  {
    std::uint64_t mostPaused = 0;
    for (const ThreadedOutcome& outcome : outcomes) {
      mostPaused = std::max(mostPaused, outcome.pauseCount);
    }
    if (mostPaused > 0) {
      const std::uint64_t perIteration =
          (mostPaused + iterations - 1) / iterations;
      if (perIteration > entry.pausePairsPerIteration) {
        entry.pausePairsPerIteration = perIteration;
      }
    }
  };

  // The threaded attempt (E-05, D-05, FR-018, FR-020, FR-023, D-07):
  // one body per body index, each dispatched to the context that owns
  // that index, so every timed loop runs on its own context's thread
  // over its own plan; the two barrier rounds stand outside every
  // measured window; the fixture pair runs once, on context 0, outside
  // both rounds; and the attempt answers with one merged record and the
  // per-context folds the merged row reads.
  auto threadedSampleRun = [&](const std::uint64_t iterations)
      -> std::expected<RunRecord, std::string>
  {
    const std::size_t count = static_cast<std::size_t>(instance.threadCount);
    const std::size_t pauseRoom = pauseRoomFor(iterations);
    const sg::detail::ChunkPlan chunk = chunkPlanOf(iterations, pauseRoom);
    // FR-051: the seam records what this attempt reserved, written
    // once on the thread that runs the attempt. specs/021 FR-008 adds the
    // chunk points the plan reserved.
    lastRecorderReservation() = RecorderReservation {
        .endpointPoints = 2,
        .pauseRoomPoints = pauseRoom,
        .chunkPoints = chunk.chunkCount == 0
            ? 0
            : static_cast<std::size_t>(chunk.chunkCount) + 1,
    };
    if (pauseRoom > maxRecorderPauseRoom()) {
      maxRecorderPauseRoom() = pauseRoom;
    }

    // FR-019: the two rounds of the attempt, one before any context
    // opens its window and one after every context has closed its
    // window. Nothing dispatches, arrives, or waits inside a measured
    // window (FR-015, Q-3).
    sg::ContextBarrier startBarrier(instance.threadCount);
    sg::ContextBarrier stopBarrier(instance.threadCount);
    std::vector<ThreadedOutcome> outcomes(count);

    // FR-018, PC-1: the body of one body index, running on that
    // context's own thread. Plan, recorder, state, and window are the
    // thread's own objects, and this context's fold lands in its own
    // slot, so no fold reads another context's plan.
    std::function<void(int)> body = [&](const int index)
    {
      ThreadedOutcome& outcome = outcomes[static_cast<std::size_t>(index)];

      // Plans are per-thread objects (FR-018): this context compiles
      // the instance's spine on its own thread. The instance compiled
      // the same spine above, so this compile stands; the assert is
      // the contract's answer, because a refused plan with the
      // assertions disabled would leave this body's barrier round
      // stranded, and a stranded round is a hung run.
      auto contextPlan = sg::counters::detail::compileCore(
          sys, sg::counters::Target {}, cores);
      SG_ASSERT(contextPlan.has_value(),
                "the instance compiled this spine once, and a plan is a "
                "per-thread object over the same spine, so every context "
                "compiles the same cores on its own thread (FR-018)");

      auto slotResetHere = contextPlan->recorder(2);
      auto recorderHere = contextPlan->recorder(chunk.pointCapacity);

      RunRecord record;
      record.iterations = iterations;
      // specs/021 FR-001, FR-002: this context captures its own chunks, on
      // its own recorder, and no fold reads another context's points.
      record.chunkCount = chunk.chunkCount;
      record.chunkIterations = chunk.chunkIterations;
      State state(iterations, &recorderHere, instance.arguments);
      // FR-017: the body reads which context of the attempt runs it
      // and how many the attempt carries.
      state.m_threadIndex = index;
      state.m_threads = instance.threadCount;
      state.m_slotReset = slotResetHere;
      if (chunk.chunkCount != 0) {
        state.setChunking(chunk.chunkIterations);
      }
      state.m_pauseFold = &record.pauseFold;
      state.m_pausePairs.reserve(pauseRoom / 2);
      for (const auto* core : cores) {
        record.pauseFold.addColumn(*core);
      }

      // R-05: the callback state is context 0's own, and the pair it
      // serves carries the arguments this instance owns.
      State callbackState(iterations, nullptr, instance.arguments, true);
      std::unique_ptr<Fixture> fixture;
      if (index == 0) {
        // FR-023, D-07: the pair wraps the attempt once, on context 0,
        // and the setup half runs before the start barrier, so it
        // stands outside every window of the attempt.
        fixtureHalf(true, callbackState, fixture, outcome.setupError);
      }

      startBarrier.arriveAndWait();

      // FR-023, D-07: this round publishes the setup slot of context 0 to
      // every context, and this copy is the first write of this context's
      // error: the vector is built for the attempt, and nothing above
      // writes that field. A setup that failed leaves every context out of
      // the body; an empty slot leaves the error empty, so the test below
      // carries the whole decision.
      outcome.error = outcomes.front().setupError;

      if (outcome.error.empty()) {
        try {
          entry.callable(state);
        } catch (const std::exception& error) {
          state.topUpWindow();
          outcome.error = error.what();
        } catch (...) {
          state.topUpWindow();
          outcome.error = "the benchmark threw an unknown exception";
        }
        state.topUpWindow();
      }

      stopBarrier.arriveAndWait();

      if (index == 0 && outcome.error.empty()) {
        // FR-023, D-07: the teardown half runs on context 0 after the
        // stop barrier, outside every window of the attempt, and a
        // callable that threw leaves the pair, the way the
        // single-context sampler leaves it.
        fixtureHalf(false, callbackState, fixture, outcome.error);
      }
      if (!outcome.error.empty()) {
        return;
      }

      const auto built = buildRecord(state, recorderHere, record);
      if (!built.has_value()) {
        outcome.error = built.error();
        return;
      }
      outcome.pauseCount = state.m_pauseCount;
      outcome.statedSize = state.m_complexityN;

      // FR-018: this context folds its own plan on its own thread, and
      // the fold lands in this index's slot; the slot of an
      // unavailable column carries no fold.
      foldThreadedContext(recorderHere, record, outcome.folds);
      outcome.record = std::move(record);
    };

    // R-04, E-06: the attempt's timed window opens on this thread
    // before the bodies run and closes after they settle, so the
    // attempt's measured spans stand inside one `[start, end)` window
    // the split folds the kept gaps into. The stamps stand outside the
    // measured spans the window covers. The close stamp answers the
    // same frozen catalog the open stamp read - registration closed at
    // the boundary the first lookup opened (FR-009) - so the close's
    // refusal stands only where the open's does, and the guard re-tests
    // the open alone (K-8). The `entry` attribution rides the
    // per-entry window vector the split takes (E-06). The `chunk`
    // index stands 0 on the attempt-level window, whose timed loop
    // spans every chunk the plan reserves (specs/021 FR-008).
    const auto windowOpen = m_windowStamping ? windowStampNow() : std::nullopt;
    launchBodies(body, count);
    const auto windowClose = m_windowStamping ? windowStampNow() : std::nullopt;
    if (m_windowStamping && windowOpen.has_value()) {
      m_runWindows.push_back(sg::detail::TimedWindow {
          .start = *windowOpen,
          .end = *windowClose,
          .entry = 0,
          .repetition = m_repetition,
          .chunk = 0,
          .pass = pass,
      });
    }

    if (const auto failure = firstThreadedError(outcomes)) {
      return std::unexpected(*failure);
    }

    // E-06, FR-020: the merged record; the label is context 0's.
    RunRecord merged = mergeThreadedRecords(outcomes, count);
    // specs/021 FR-005: the summed decision seconds over the summed
    // iterations is the per-iteration time of one context, and it
    // calibrates the chunk size of the next attempt.
    if (merged.decisionNs > 0) {
      entry.iterationNs = static_cast<double>(merged.decisionNs)
          / static_cast<double>(merged.iterations);
    }
    complexityN = outcomes.front().statedSize;

    risePauseHistory(outcomes, iterations);

    threadedFolds.clear();
    threadedFolds.reserve(count);
    for (ThreadedOutcome& outcome : outcomes) {
      threadedFolds.push_back(std::move(outcome.folds));
    }
    return merged;
  };

  auto sampleRun = [&](const std::uint64_t iterations)
      -> std::expected<RunRecord, std::string>
  {
    if (instance.threadCount > 1) {
      // FR-016, FR-020: the threaded instance takes the threaded
      // attempt path. An instance of one context, which is every
      // family that calls no thread method, never reaches this branch
      // and keeps today's single-context bytes (FR-022).
      return threadedSampleRun(iterations);
    }

    RunRecord record;
    record.iterations = iterations;

    // E-08, FR-051: the room rule of the shared pause history stands
    // in pauseRoomFor; the seam keeps the FR-051 record of what this
    // attempt reserved. specs/021 FR-008 adds the chunk points the plan
    // reserved, and FR-001 opens a window per chunk at that capacity.
    const std::size_t pauseRoom = pauseRoomFor(iterations);
    const sg::detail::ChunkPlan chunk = chunkPlanOf(iterations, pauseRoom);
    lastRecorderReservation() = RecorderReservation {
        .endpointPoints = 2,
        .pauseRoomPoints = pauseRoom,
        .chunkPoints = chunk.chunkCount == 0
            ? 0
            : static_cast<std::size_t>(chunk.chunkCount) + 1,
    };
    if (pauseRoom > maxRecorderPauseRoom()) {
      maxRecorderPauseRoom() = pauseRoom;
    }
    attemptRecorder = plan->recorder(chunk.pointCapacity);
    record.chunkCount = chunk.chunkCount;
    record.chunkIterations = chunk.chunkIterations;

    // R-05, E-07: the callback state carries the arguments this
    // instance owns and holds no recorder, so it opens no sampling
    // window and the loop and skip operations of it are precondition
    // violations (FR-018).
    State callbackState(iterations, nullptr, instance.arguments, true);

    // R-04: the state reads the arguments this instance owns. The
    // instance outlives every run of itself, so the span needs no copy,
    // and the timed loop keeps its H1 shape (FR-009).
    State state(iterations, &*attemptRecorder, instance.arguments);
    if (chunk.chunkCount != 0) {
      state.setChunking(chunk.chunkIterations);
    }
    // FR-052: the run's fold sink carries one running total per column
    // of the run, fed at the commit of every pair the room cannot hold.
    state.m_slotReset = slotReset;
    state.m_pauseFold = &record.pauseFold;
    // FR-051: a closed pair takes one slot of the raw path, and the room
    // above counts two raw points per pair, so half that room is the pair
    // count an attempt can reach. Holding the vector to that size before
    // the loop opens keeps State::closePausePair free of any reallocation
    // inside the timed loop.
    state.m_pausePairs.reserve(pauseRoom / 2);
    for (const auto* core : cores) {
      record.pauseFold.addColumn(*core);
    }
    // R-09: one fixture object per run, built inside this scope before
    // `setUp` and destroyed after `tearDown` when the scope ends, the
    // catch paths included. The pair runs in the untimed region, before
    // `begin()` opens the window and after the loop closes it, so its
    // work adds nothing to the reported time (FR-017). The pair
    // receives the callback state, the one rule Q-5 settled for both
    // pairs: the argument and iteration reads are legal there and the
    // loop and skip operations are precondition violations (FR-017,
    // FR-018).
    std::unique_ptr<Fixture> fixture;
    // R-04, E-06: the attempt's timed window opens on this thread
    // before the measured loop and closes after it tops up, so the
    // attempt's measured span stands inside one `[start, end)` window
    // the split folds the kept gaps into; a callable that throws leaves
    // the close unstamped and the window unrecorded. The stamps stand
    // outside the measured span the window covers. The `entry`
    // attribution rides the per-entry window vector the split takes
    // (E-06). The `chunk` index stands 0 on the attempt-level window,
    // whose timed loop spans every chunk the plan reserves (specs/021
    // FR-008).
    const auto windowOpen = m_windowStamping ? windowStampNow() : std::nullopt;
    try {
      // R-10: the callback pair wraps the run, the setup callback
      // first and the teardown callback last, the warm-up,
      // calibration, and measured runs included (FR-018).
      if (entry.setup) {
        entry.setup(callbackState);
      }
      if (entry.fixtureFactory) {
        fixture = entry.fixtureFactory();
        fixture->setUp(callbackState);
      }
      entry.callable(state);
      if (fixture) {
        fixture->tearDown(callbackState);
      }
      if (entry.teardown) {
        entry.teardown(callbackState);
      }
    } catch (const std::exception& error) {
      state.topUpWindow();
      return std::unexpected(error.what());
    } catch (...) {
      state.topUpWindow();
      return std::unexpected("the benchmark threw an unknown exception");
    }
    state.topUpWindow();
    const auto windowClose = m_windowStamping ? windowStampNow() : std::nullopt;
    // The close stamp answers the same frozen catalog the open stamp
    // read - registration closed at the boundary the first lookup
    // opened (FR-009) - so the close's refusal stands only where the
    // open's does, and the guard re-tests the open alone (K-8).
    if (m_windowStamping && windowOpen.has_value()) {
      m_runWindows.push_back(sg::detail::TimedWindow {
          .start = *windowOpen,
          .end = *windowClose,
          .entry = 0,
          .repetition = m_repetition,
          .chunk = 0,
          .pass = pass,
      });
    }

    if (const auto built = buildRecord(state, *attemptRecorder, record);
        !built.has_value())
    {
      return std::unexpected(built.error());
    }

    // specs/021 FR-005: the per-iteration time this attempt measured
    // calibrates the chunk size of the next attempt of this instance.
    if (record.decisionNs > 0) {
      entry.iterationNs = static_cast<double>(record.decisionNs)
          / static_cast<double>(record.iterations);
    }

    // E-08, FR-051: the pairs this attempt closed stand as the
    // family's history, and the next attempt reserves from it. A body
    // that pauses more per iteration than the history saw folds from
    // that pair on, so no closed pair is lost (FR-025).
    if (state.m_pauseCount > 0) {
      const std::uint64_t perIteration =
          (state.m_pauseCount + iterations - 1) / iterations;
      if (perIteration > entry.pausePairsPerIteration) {
        entry.pausePairsPerIteration = perIteration;
      }
    }
    complexityN = state.m_complexityN;
    return record;
  };

  std::uint64_t settled = fixedIterations.value_or(1);

  // FR-011: warm-up runs grow by the same rule against the warm-up
  // target, and their results are discarded. The measured phase keeps
  // its own start count, so the warm-up count never carries into it.
  if (warmupNs > 0 && !dryRun) {
    const auto warm = growUntilQualified(settled,
                                         warmupNs,
                                         kRunBound,
                                         PhaseText {"warm-up", "FR-011"},
                                         sampleRun,
                                         []() noexcept -> bool
                                         { return interruptFlag().load(); });
    if (warm.outcome == GrowOutcome::FAILED
        || warm.outcome == GrowOutcome::BOUND_EXHAUSTED)
    {
      return fail(warm.error);
    }
    if (warm.outcome == GrowOutcome::SKIPPED) {
      result.outcome = RunOutcome::SKIPPED;
      result.reason = warm.record.reason;
      publishSettled(0);
      return result;
    }
    if (warm.outcome == GrowOutcome::INTERRUPTED) {
      markInterrupted(result);
      publishSettled(0);
      return result;
    }
  }

  // FR-014: a dry run takes one iteration, one repetition, and no
  // warm-up, and it qualifies at once.
  const std::uint64_t repetitionsOfRun = dryRun ? 1 : repetitions;

  // specs/024 FR-010, D-06: a run that splits its columns across passes
  // calibrates once, ahead of pass 0, and every pass then runs every
  // repetition at the iteration count that calibration produced, so the
  // passes measure the same amount of work. A run of one pass keeps the
  // shape the harness has at the audit point, where the run that qualifies
  // the first repetition is that repetition's measured row (SC-09).
  bool calibratedAhead = false;
  if (passCount > 1 && !fixedIterations.has_value() && !dryRun) {
    const auto ahead = growUntilQualified(1,
                                          minTimeNs,
                                          kRunBound,
                                          PhaseText {"calibration", "FR-016"},
                                          sampleRun,
                                          []() noexcept -> bool
                                          { return interruptFlag().load(); });
    if (ahead.outcome == GrowOutcome::FAILED
        || ahead.outcome == GrowOutcome::BOUND_EXHAUSTED)
    {
      return fail(ahead.error);
    }
    if (ahead.outcome == GrowOutcome::SKIPPED) {
      result.outcome = RunOutcome::SKIPPED;
      result.reason = ahead.record.reason;
      // The stop closed the calibration that stands ahead of pass 0, so no
      // pass has a fold standing (E-07).
      publishSettled(0);
      return result;
    }
    if (ahead.outcome == GrowOutcome::INTERRUPTED) {
      markInterrupted(result);
      publishSettled(0);
      return result;
    }
    // FR-012, FR-020: the qualified record sums the count of every context,
    // and the count a pass hands each context is one share of that sum.
    settled = ahead.record.iterations
        / static_cast<std::uint64_t>(instance.threadCount);
    calibratedAhead = true;
  }

  // E-04, FR-009, FR-011, FR-013: the pass loop, outside the threaded
  // dispatch. The run chose its contexts once above and every pass runs on
  // that same set, never re-placing one; the wait every attempt already
  // makes for its body nodes is the join between passes, so no window of a
  // later pass opens before the last window of this one closed in every
  // context. Each pass folds the rows of its own measurements into a result
  // of its own, and the merge after the last pass brings the one result the
  // run publishes out of them (FR-014).
  const BenchmarkResult passIdentity = result;
  std::vector<BenchmarkResult> passResults;
  passResults.reserve(passCount);
  bool interrupted = false;
  // E-07, FR-017: the count a pass loop publishes when it leaves early: the
  // passes that closed before the one in flight, beside that pass itself once
  // it has folded a row. A pass that folded no row has no fold standing, so it
  // stays out of the count.
  const auto standingPasses = [&passResults, &result] noexcept -> std::size_t
  { return passResults.size() + (result.rows.empty() ? 0U : 1U); };
  // specs/023 E-01 beside specs/024 E-04: each pass appends its retained
  // chunk rows behind the earlier passes', and the mark of each pass names
  // where its entries stand, so the join below - or a return inside a
  // pass - can leave the ledger standing beside the rows it publishes.
  std::vector<std::size_t> passLedgerMarks;
  const auto keepCurrentPassLedger = [this, &passLedgerMarks]
  {
    const auto drop = static_cast<std::ptrdiff_t>(passLedgerMarks.back());
    m_rowRaw.back().erase(m_rowRaw.back().begin(),
                          m_rowRaw.back().begin() + drop);
    m_contextRaw.back().erase(m_contextRaw.back().begin(),
                              m_contextRaw.back().begin() + drop);
  };
  for (pass = 0; pass < passCount; ++pass) {
  // specs/029 PD-11, PD-12, D-18: the retry route of a hunt run. The seat of a
  // pass after the first is a runtime condition for a run that carries hunt
  // columns - a schedulable-set refusal at open says nothing about the frozen
  // catalog - so such a pass hands each of its events a pass of its own beside
  // the two anchors of that event's device, and this same loop runs them
  // (D-18, hard rule 15). `loneEventOf` marks a pass a retry opened and the one
  // event it measures; `rowOf` names the fold result each pass of the grown
  // list produced, so the merge and the chunk ledger read the pass that
  // measured a column; a pass that was refused keeps no ledger.
  // `kNoHuntIndex` names the absent entry of both: no column and no row carry
  // that index. The retry route exists for a hunt run alone, so the three
  // tables stand only when one runs: a run that carries no hunt allocates what
  // it allocated before the hunt existed (plan VII, I-8, T130).
  constexpr std::size_t kNoHuntIndex = std::numeric_limits<std::size_t>::max();
  const bool huntRun = instance.huntPlan.has_value();
  std::vector<std::size_t> rowOf;
  std::vector<std::size_t> loneEventOf;
  // The pass that measures each column, remapped by the retry below. It feeds
  // the merge and the ledger exactly as `planning.measuringPass` does.
  std::vector<std::size_t> measuringOfColumn;
  if (huntRun) {
    rowOf.assign(passCount, kNoHuntIndex);
    loneEventOf.assign(passCount, kNoHuntIndex);
    measuringOfColumn = planning.measuringPass;
    measuringOfColumn.resize(columns.size(), 0);
  }
  const auto retryHuntPass = [&](const std::size_t pass) -> void
  {
    std::vector<std::size_t> loneColumns;
    // The anchors of the plan: an anchor column rides retry passes as a
    // member of an event, never as a retry event of its own (D-18).
    std::vector<std::string> anchorLabels;
    for (const auto& candidate : instance.huntPlan->columns) {
      if (!candidate.cycles.empty()) {
        anchorLabels.push_back(candidate.cycles);
      }
      if (!candidate.instructions.empty()) {
        anchorLabels.push_back(candidate.instructions);
      }
    }
    for (const std::size_t column : passColumns[pass]) {
      if (std::find(
              anchorLabels.begin(), anchorLabels.end(), columns[column].label)
          != anchorLabels.end())
      {
        continue;
      }
      // The plan column of this column, by address: an anchor column answers
      // nothing, because no plan column names an anchor as its event.
      const hunt::HuntColumn* planned = nullptr;
      for (const auto& candidate : instance.huntPlan->columns) {
        if (candidate.event == columns[column].label) {
          planned = &candidate;
          break;
        }
      }
      if (planned == nullptr) {
        continue;
      }
      std::vector<std::size_t> single {column};
      const auto addAnchor = [&](const std::string& anchor)
      {
        if (anchor.empty()) {
          return;
        }
        for (std::size_t index = 0; index < columns.size(); ++index) {
          if (columns[index].label != anchor) {
            continue;
          }
          // The loop above skipped every column whose label is an anchor of
          // any column, and the two anchors of one column are two distinct
          // addresses, so a matched index cannot already stand in `single`
          // twice (specs/029 D-18). The ignore build drops SG_* bodies, so a
          // local read only by the contract call would be unused there.
          SG_ASSERT(std::ranges::find(single, index) == single.end(),
                    "one retry pass opens each anchor column once");
          single.push_back(index);
        }
      };
      addAnchor(planned->cycles);
      addAnchor(planned->instructions);
      const std::size_t opened = passColumns.size();
      passColumns.push_back(std::move(single));
      rowOf.push_back(kNoHuntIndex);
      loneEventOf.push_back(column);
      // The retry pass measures the event it opened for and the anchors it
      // carries, so both name this pass from now on (I-2: an anchor one pass
      // claims is counted once in that pass).
      for (const std::size_t measured : passColumns[opened]) {
        if (measuringOfColumn[measured] == pass) {
          measuringOfColumn[measured] = opened;
        }
      }
      loneColumns.push_back(column);
    }
    if (!loneColumns.empty()) {
      instance.retriedHuntPasses.push_back(detail::RetriedPass {
          .pass = pass, .loneColumns = std::move(loneColumns)});
    }
    passCount = passColumns.size();
  };
  for (std::size_t pass = 0; pass < passCount; ++pass) {
    if (pass > 0) {
      // E-04, FR-012: the previous pass's seat closes first - the recorder
      // that stood over it, then the plan - and this pass compiles its own
      // leaf list before it opens its first timed window.
      attemptRecorder.reset();
      plan.reset();
      // FR-004, E-04: the catalog is frozen at open and every provider is
      // additive, so a leaf list the pre-loop seat compiled compiles again
      // at every later seat; the invariant below is the fuse for a
      // counter layer that broke that fact.
      [[maybe_unused]] const auto seated = seatPass(passColumns[pass]);
      if (huntRun && !seated.has_value()) {
        // specs/029 PD-11, D-18: the refusal of one group is a runtime
        // condition, so a hunt run keeps going where the invariant below would
        // stop every other run.
        if (const std::size_t lone = loneEventOf[pass]; lone != kNoHuntIndex) {
          // specs/029 D-18, FR-021, T128: the state the refusal names is the
          // state the column publishes, so the row of that address and the
          // skipped line of that event answer one state. A refusal that names
          // no state publishes `Availability::ABSENT`, the state the run
          // reports for an event that produced no value.
          const auto published =
              sg::detail::availabilityOfRefusal(seated.error())
                  .value_or(sg::counters::Availability::ABSENT);
          columns[lone].unavailable = true;
          columns[lone].availability = published;
          // specs/029 D-18, FR-021, T073, T118: the event that still refuses
          // is recorded with the refusal itself and the state published for
          // it, so `runHunt` gives it the kind that state names. No kind is
          // guessed here.
          instance.refusedHuntEvents.push_back(
              detail::RefusedRetry {.address = columns[lone].label,
                                    .refusal = seated.error(),
                                    .availability = published});
        } else {
          retryHuntPass(pass);
        }
        continue;
      }
      SG_INVARIANT(
          seated.has_value(),
          "the catalog frozen at open and the additive providers keep every "
          "pass's leaf list compilable after the pre-loop seat compiled it "
          "(FR-004, E-04)");
    }
    // E-05: one result per pass, each carrying the identity the run
    // established before the split.
    result = passIdentity;
    passLedgerMarks.push_back(m_rowRaw.back().size());

    for (std::uint64_t repetition = 0; repetition < repetitionsOfRun;
         ++repetition)
    {
      // E-06: the windows this repetition's attempts stamp name the pass
      // they belong to.
      m_repetition = repetition;
      std::uint64_t iterations = dryRun ? 1 : settled;
      std::expected<RunRecord, std::string> chosen = std::unexpected("");

      if (repetition == 0 && !calibratedAhead && !fixedIterations.has_value()
          && !dryRun)
      {
        // FR-014: the first repetition calibrates from one iteration, and
        // the run that qualifies is its measured run.
        const auto grown = growUntilQualified(
            1,
            minTimeNs,
            kRunBound,
            PhaseText {"calibration", "FR-016"},
            sampleRun,
            []() noexcept -> bool { return interruptFlag().load(); });
        if (grown.outcome == GrowOutcome::FAILED
            || grown.outcome == GrowOutcome::BOUND_EXHAUSTED)
        {
          return fail(grown.error);
        }
        if (grown.outcome == GrowOutcome::SKIPPED) {
          result.outcome = RunOutcome::SKIPPED;
          result.reason = grown.record.reason;
          keepCurrentPassLedger();
          publishSettled(standingPasses());
          return result;
        }
        if (grown.outcome == GrowOutcome::INTERRUPTED) {
          markInterrupted(result);
          keepCurrentPassLedger();
          publishSettled(standingPasses());
          return result;
        }
        chosen = std::move(grown.record);
        // FR-012, FR-020: the qualified record sums the count of every
        // context. The count a later repetition hands each context is one
        // share of that sum.
        iterations = chosen->iterations
            / static_cast<std::uint64_t>(instance.threadCount);
        settled = iterations;
      } else {
        chosen = sampleRun(iterations);
        if (!chosen.has_value()) {
          return fail(chosen.error());
        }
        if (chosen->skipped) {
          result.outcome = RunOutcome::SKIPPED;
          result.reason = chosen->reason;
          keepCurrentPassLedger();
          publishSettled(standingPasses());
          return result;
        }
      }

      if (instance.threadCount > 1) {
        fillDisclosures();
      }

      // FR-002, FR-005, FR-006: the fold behind the availability of the
      // primary column, a null core where the run carries a manual sum.
      const sg::counters::detail::ExprCore* primaryCore = selectPrimaryCore(
          entry.manualTime, entry.realTime, realTime.core, cpuTime.core);
      if (instance.threadCount > 1) {
        // FR-020: exactly one merged row per repetition, carrying the
        // summed values and the per-iteration division by the summed
        // iteration count, built from the folds each context did on its
        // own plan (FR-018).
        result.contextPercentiles.clear();
        std::vector<std::vector<std::vector<double>>> contextChunks;
        std::vector<RowRaw> rowContexts;
        appendMergedRow(result.rows,
                        *chosen,
                        columns,
                        measuredColumn,
                        overheadFloorNs,
                        primaryCore,
                        threadedFolds,
                        static_cast<std::uint64_t>(instance.threadCount),
                        result.percentilePoints,
                        result.contextPercentiles,
                        contextChunks);
        for (auto& chunks : contextChunks) {
          rowContexts.push_back(RowRaw {.columns = std::move(chunks)});
        }
        m_contextRaw.back().push_back(std::move(rowContexts));
        // The merged row answers from the union of the contexts' columns,
        // which the per-context entries carry whole, so the row itself
        // retains no copy of that union (specs/023 E-01).
        m_rowRaw.back().emplace_back();
      } else {
        std::vector<std::vector<double>> chunks;
        appendMeasuredRow(result.rows,
                          *chosen,
                          columns,
                          measuredColumn,
                          overheadFloorNs,
                          primaryCore,
                          entry.realTime ? cpuTime.core : realTime.core,
                          *attemptRecorder,
                          entry.realTime ? 0 : 1,
                          entry.realTime ? 1 : 0,
                          result.percentilePoints,
                          chunks);
        m_rowRaw.back().push_back(RowRaw {.columns = std::move(chunks)});
        m_contextRaw.back().emplace_back();
      }

      // FR-006: the labels of the run's counters are its names in the order
      // they opened, read from the rows as they are built.
      if (chosen->counterNames.size() > result.counterLabels.size()) {
        result.counterLabels = chosen->counterNames;
      }

      // FR-008: a counter name equal to the label of an attached metric fails
      // the run naming the collision, and the check runs when the first row of
      // the run stands.
      for (const auto& label : result.metricLabels) {
        for (const auto& name : chosen->counterNames) {
          if (label == name) {
            return fail("the counter name '" + name
                        + "' equals the label of an attached metric, and one "
                          "row carries that name once (FR-008)");
          }
        }
      }

      if (interruptFlag().load()) {
        result.outcome = RunOutcome::SKIPPED;
        result.reason = kInterruptReason;
        interrupted = true;
        break;
      }
    }

    // E-04, FR-012: this pass measured its last row, so it closes: the
    // attempt's recorder first, then the plan that recorder stood over.
    attemptRecorder.reset();
    plan.reset();
    if (huntRun) {
      rowOf[pass] = passResults.size();
    }
    passResults.push_back(std::move(result));
    if (interrupted) {
      break;
    }
  }

  if (!instance.retriedHuntPasses.empty()) {
    // specs/029 PD-12: the retry passes joined the list the loop read and a
    // refused pass folded no row, so the merge and the ledger read the passes
    // that folded, in the order they folded, with every column naming the pass
    // that measured it (FR-014).
    sg::detail::PassPlan folded;
    folded.columns.resize(passResults.size());
    folded.exclusive.assign(passResults.size(), false);
    folded.measuringPass = std::move(measuringOfColumn);
    for (std::size_t& measuring : folded.measuringPass) {
      // A column whose pass folded no row - the event that still refused after
      // its own pass, or an anchor no retry pass carried - reads the reference
      // pass, whose row holds that column's own disclosure (D-18).
      measuring = rowOf[measuring] == kNoHuntIndex ? 0 : rowOf[measuring];
    }
    planning = std::move(folded);
  }

  if (passResults.size() > 1 && !interrupted) {
    // FR-014, FR-016: the merge runs once, after the last pass closed and
    // outside every timed window, over the fold results of the passes. It
    // reads no counter and publishes no value a pass did not measure. The
    // merge indexes the plan by the column of a row, and a row can stand
    // wider than the columns the planner costed: a body that stores counters
    // adds one column each. Those columns ride every pass unmeasured by the
    // planner, so the reference pass answers them, and the padding keeps the
    // merge's index inside the plan it reads. The percentile grid needs no term
    // of its own: it holds one slot per printed column in the order the row
    // prints them, the primary time, the time column the primary is not, then
    // each metric column, so the grid is never wider than two slots plus the
    // widest row above, and a plan padded to that row names a measuring pass
    // for every slot the merge reads at `slot - 2`.
    std::size_t widestRow = 0;
    for (const BenchmarkResult& passResult : passResults) {
      for (const ResultRow& row : passResult.rows) {
        widestRow = std::max(
            widestRow, passIdentity.metricLabels.size() + row.counters.size());
      }
    }
    if (planning.measuringPass.size() < widestRow) {
      planning.measuringPass.resize(widestRow, 0);
    }
    result = sg::detail::mergePasses(planning, passResults);
  } else {
    // specs/029 D-18: the pass loop always folds pass 0, so no run gets here
    // with an empty list; the assertion says so and the move publishes the
    // folded pass. A hunt run whose passes refused is answered from the
    // instance's refusal list by `runHunt`, not from this list.
    SG_ASSERT(!passResults.empty(),
              "a run that folded no pass needs a pass list of zero: the "
              "settling that ran no planner keeps one whole-run pass "
              "(runner.cpp:1330, E-01), the seat before the loop ends the run "
              "when pass 0 refuses (runner.cpp:1416, PC-9), and the only "
              "in-loop skip needs a pass after the first (runner.cpp:2352, "
              "D-18), so the loop at runner.cpp:2351 folds pass 0 at "
              "runner.cpp:2546");
    result = std::move(passResults.back());
  }

  // specs/023 E-01, FR-019 beside specs/024 FR-014, FR-015: the retained
  // chunk columns stand beside the row they answer. The passes appended
  // their ledgers one behind the other, so the join reindexes the ledger
  // onto the rows the run publishes: a merged run reads every metric
  // column's chunks from the pass that measured it and keeps the two time
  // columns from the reference pass, the reading of the percentile columns
  // the merge publishes (FR-015), and a collection whose merge stayed
  // unrun keeps the last pass's own segment.
  {
    auto& rowRaws = m_rowRaw.back();
    if (passResults.size() > 1 && !interrupted) {
      for (std::size_t row = 0; row < result.rows.size(); ++row) {
        RowRaw& rawEntry = rowRaws[row];
        for (std::size_t metric = 0; metric + 2 < rawEntry.columns.size();
             ++metric)
        {
          SG_REQUIRE(metric < planning.measuringPass.size(),
                     "a merge column outside the padded plan violates the bound "
                     "the padding of measuringPass keeps (FR-014, FR-016)");
          const std::size_t measuring = planning.measuringPass[metric];
          if (measuring > 0) {
            rawEntry.columns[metric + 2] = std::move(
                rowRaws[passLedgerMarks[measuring] + row].columns[metric + 2]);
          }
        }
      }
      rowRaws.resize(result.rows.size());
      m_contextRaw.back().resize(result.rows.size());
    } else {
      // One mark stands per folded pass and the pass loop folds pass 0 on
      // every run, so a run outside the merge arm holds a mark to keep; the
      // helper below already reads that mark unguarded (specs/029 D-18).
      SG_ASSERT(!passLedgerMarks.empty(),
                "each folded pass marked its ledger and the pass loop folds "
                "pass 0 on every run, so a run outside the merge arm holds "
                "a mark to keep");
      keepCurrentPassLedger();
    }
  }

  // E-07, FR-017, FR-020, research D-10: the settled method and its pass
  // count ride the published result on both arms of this join. The merge
  // already carries them; the single-result arm is where the `multiplex`
  // settling and a one-pass `passes` settling publish theirs, because only
  // the merge path reaches `mergePasses`, the writer of these fields. A
  // settling of `AUTO` - the run that carries no metric column - publishes
  // no count, the reading E-07 gives a run whose pass mechanism did not run.
  // The count reads the folds that stand: a collection that ran to its last
  // pass owes the plan it was given, and one the loop left early owes the
  // passes before the stop. The single pass of such a run and of the one-plan
  // settleings measured no second time to spread, so the published spread
  // stays the empty answer.
  publishSettled(passResults.size());
  // FR-021, clarification Q2, research D-10: the forced-method record is
  // caller state - the command line's word or the family's own request -
  // that the settled method cannot recover, because a forced one-pass
  // `passes` run and an identical `auto` one settle alike. The runner holds
  // both records and carries the answer to the report's print rule.
  result.metricMethodForced = m_options.metricMethod.has_value()
      || entry.metricMethod != MetricMethod::AUTO;

  // FR-054: the result records the fold when a measured repetition's
  // run netted a pause pair through the fold path.
  result.pauseSamplesFolded =
      std::any_of(result.rows.begin(),
                  result.rows.end(),
                  [](const sg::ResultRow& row) noexcept -> bool
                  { return row.pauseSamplesFolded; });

  if (result.outcome == RunOutcome::MEASURED && repetitionsOfRun > 1) {
    ResultRow aggregate;
    aggregate.kind = RowKind::AGGREGATE;

    // FR-012: the aggregate carries the label only when every repetition
    // shares it, the shared-label rule of the cited ComputeStats.
    aggregate.label = result.rows.front().label;
    for (const auto& row : result.rows) {
      if (row.label != aggregate.label) {
        aggregate.label.clear();
        break;
      }
    }

    // D-13, FR-002: one aggregate summarizes the primary time, and the
    // gap rule of the Edge Cases keeps a repetition whose primary fold
    // carried a disclosure out of the statistics. The primary time of a
    // manual run is the sum the benchmark supplied, the one value the
    // time column holds for it (FR-006).
    std::vector<double> times;
    for (const auto& row : result.rows) {
      if (row.timeAvailability != sg::counters::Availability::COUNTABLE) {
        continue;
      }
      times.push_back(entry.manualTime || entry.realTime
                          ? row.timePerIterationNs
                          : row.cpuTimePerIterationNs);
    }
    // The Edge Cases edge: every repetition can carry a disclosure,
    // and the gap rule of FR-025 then hands the summary no value. The
    // aggregate keeps the default record, zero in every field beside
    // a zero sample count, and statisticsOf keeps its non-empty input.
    aggregate.timeStats =
        times.empty() ? sg::Statistics {} : sg::detail::statisticsOf(times);

    // FR-013, FR-014, FR-015: every attached statistic computes over the
    // same clean primary values the built-in set summarized, in the
    // order the calls came in. A function that refuses its input leaves no
    // value, so the row holds no field for it and the run still reports.
    for (const auto& statistic : entry.statistics) {
      sg::StatisticValue value;
      value.name = statistic.name;
      value.unit = statistic.unit;
      if (statistic.function(times, value.value)) {
        result.statisticValues.push_back(std::move(value));
      }
    }

    for (std::size_t column = 0; column < columns.size(); ++column) {
      std::vector<double> values;
      for (const auto& row : result.rows) {
        // FR-025: a repetition whose run carried a gap stays out of the
        // statistics of the affected quantity.
        if (row.metrics[column].availability
            == sg::counters::Availability::COUNTABLE)
        {
          values.push_back(row.metrics[column].value);
        }
      }
      // The same empty-input shape as the primary time above: a
      // quantity every repetition disclosed summarizes to the zero
      // record. The value lands in the slot before the push, so the
      // fold's summary carries the identical structure of the time
      // aggregate beside it.
      sg::Statistics columnStats = values.empty()
          ? sg::Statistics {}
          : sg::detail::statisticsOf(std::move(values));
      aggregate.metricStats.push_back(columnStats);
    }

    // FR-006, FR-005: one statistic set per named counter over the clean
    // repetitions. A row that disclosed one of its counters stands aside from
    // the statistics of that counter, and a counter every repetition disclosed
    // summarizes to the zero record beside a zero sample count.
    std::vector<std::vector<double>> counterValues(result.counterLabels.size());
    for (const auto& row : result.rows) {
      if (!row.counterDisclosure.empty()) {
        continue;
      }
      // FR-006, FR-022: the width of the label list is the caller's
      // guarantee, read at every site that indexes a label by a counter
      // column. The list is widened at the row site of this function, where
      // a row's names replace the list whenever they are wider, and
      // appendCounterColumns appends one column per name, so a row stands no
      // wider than the list. The aggregate row keeps an empty value list, so
      // it reads no label either. The report reads labels twice: once over
      // the columns of a row and once over the statistic sets, and the sets
      // are built one per label here. Each stands under this guarantee.
      SG_ASSERT(row.counters.size() <= counterValues.size(),
                "the counter labels of a result come from its widest row, so "
                "no row carries a column the value list lacks (FR-006, FR-022)");
      for (std::size_t column = 0; column < row.counters.size(); ++column) {
        counterValues[column].push_back(row.counters[column]);
      }
    }
    // The same two-step shape as the metric fold above, so both folds of one
    // aggregate row compute the summary into the slot before the push.
    for (auto& values : counterValues) {
      sg::Statistics columnStats = values.empty()
          ? sg::Statistics {}
          : sg::detail::statisticsOf(std::move(values));
      result.counterStats.push_back(columnStats);
    }

    result.rows.push_back(aggregate);
    m_rowRaw.back().emplace_back();
    m_contextRaw.back().emplace_back();
  }

  // FR-019, FR-022: a family that asked for a complexity fit collects one value
  // per instance, and the fit itself waits for the family to end. A run that
  // measured nothing hands the fit no value, so it stands out of the samples.
  if (result.outcome == RunOutcome::MEASURED
      && (entry.bigO != BigO::NONE || static_cast<bool>(entry.bigOFun)))
  {
    noteFitSample(instance, result, complexityN);
  }

  return result;
}

namespace
{

// FR-022: one instance's values of one family of quantities join the samples
// the fit reads. A name a later instance opens joins the label list, and the
// instances before it carry no value of it, which is the one shape that leaves
// that quantity without its pair (FR-023, spec Edge Cases).
auto appendQuantities(std::vector<std::string>& labels,
                      std::vector<std::vector<double>>& values,
                      const std::vector<std::string>& names,
                      const std::vector<double>& columns,
                      std::size_t instances) -> void
{
  for (std::size_t column = 0; column < columns.size(); ++column) {
    const std::size_t index = static_cast<std::size_t>(
        std::ranges::find(labels, names.at(column)) - labels.begin());
    if (index == labels.size()) {
      labels.push_back(names.at(column));
      values.emplace_back(instances - 1,
                          std::numeric_limits<double>::quiet_NaN());
    }
    values.at(index).push_back(columns.at(column));
  }
}

}  // namespace

auto Runner::noteFitSample(const Instance& instance,
                           const BenchmarkResult& result,
                           const std::optional<std::int64_t> statedSize) -> void
{
  // FR-019: the size of an instance is the value its body stated, else the
  // range value it runs under, and zero when it carries neither, which leaves
  // the family without a fit (FR-023).
  const std::int64_t size = statedSize.value_or(
      instance.arguments.empty() ? 0 : instance.arguments.front());

  // FR-022: the aggregate summarizes the instance when one stands; a family of
  // one repetition keeps its single row, and that row carries the value.
  const auto* chosen = &result.rows.back();
  for (const auto& row : result.rows) {
    if (row.kind == RowKind::AGGREGATE) {
      chosen = &row;
    }
  }
  const bool aggregate = chosen->kind == RowKind::AGGREGATE;
  const RegistryEntry& entry = *instance.family;

  FitSamples& samples = accumulatorFor(instance.family);
  m_pending = static_cast<std::size_t>(&samples - m_fits.data());
  samples.unit = result.timeUnit;
  samples.sizes.push_back(size);
  samples.times.push_back(aggregate ? chosen->timeStats.mean
                                    : (entry.manualTime || entry.realTime
                                           ? chosen->timePerIterationNs
                                           : chosen->cpuTimePerIterationNs));

  auto counters = std::vector<double> {};
  if (aggregate) {
    for (const auto& stats : result.counterStats) {
      counters.push_back(stats.mean);
    }
  } else {
    counters = chosen->counters;
  }
  auto metrics = std::vector<double> {};
  if (aggregate) {
    for (const auto& stats : chosen->metricStats) {
      metrics.push_back(stats.mean);
    }
  } else {
    for (const auto& metric : chosen->metrics) {
      metrics.push_back(metric.value);
    }
  }

  appendQuantities(samples.counterLabels,
                   samples.counterValues,
                   result.counterLabels,
                   counters,
                   samples.sizes.size());
  appendQuantities(samples.metricLabels,
                   samples.metricValues,
                   result.metricLabels,
                   metrics,
                   samples.sizes.size());
}

// The samples of `family`, opened the first time that family asked for a fit.
auto Runner::accumulatorFor(RegistryEntry* const family) -> FitSamples&
{
  for (std::size_t index = 0; index < m_fits.size(); ++index) {
    if (m_fits[index].family == family) {
      return m_fits[index];
    }
  }
  auto& samples = m_fits.emplace_back();
  samples.family = family;
  return samples;
}

auto Runner::fitForFamilyEnd(const Instance* next)
    -> std::optional<BenchmarkResult>
{
  if (!m_pending.has_value()
      || (next != nullptr && next->family == m_fits[*m_pending].family))
  {
    return std::nullopt;
  }
  return fitForFamily(m_fits[*m_pending].family);
}

// FR-022, FR-023, specs/028 FR-056: the fit of one family out of the samples
// of every instance of it this runner ran. A shuffled run reaches those
// instances in the order of its walk and the walk of another family stands
// between them, so the family is named here and the samples keep the order
// they arrived in: `fitFamily` regresses each quantity on the size of its
// instance, an answer no ordering of the pairs changes. One family answers
// once, which is the account the print pass reads at the last row of that
// family.
auto Runner::fitForFamily(RegistryEntry* const family)
    -> std::optional<BenchmarkResult>
{
  for (std::size_t index = 0; index < m_fits.size(); ++index) {
    FitSamples& samples = m_fits[index];
    // The expansion groups the instances of one family into one block, so
    // each print path names a family once and the record found here still
    // holds the samples of every instance of that family this runner ran
    // (FR-022, FR-056).
    if (samples.family != family) {
      continue;
    }
    auto outcome = outcomeOf(samples);
    samples.unit = std::nullopt;
    samples.sizes = {};
    samples.times = {};
    samples.counterLabels = {};
    samples.counterValues = {};
    samples.metricLabels = {};
    samples.metricValues = {};
    if (m_pending == index) {
      m_pending = std::nullopt;
    }
    return outcome;
  }
  return std::nullopt;
}

auto Runner::outcomeOf(FitSamples& samples) -> BenchmarkResult
{
  // FR-022: the quantities of the fit in the order the report prints them: the
  // primary time first, then the named counters and the attached metrics in the
  // order their names and expressions opened.
  auto quantities = std::vector<FitQuantity> {};
  quantities.push_back(FitQuantity {
      .label = "time/iter",
      .scalesWithUnit = true,
      .values = samples.times,
      .curve = {},
      .coefficient = 0.0,
      .rmsPercent = 0.0,
      .reason = {},
  });
  for (std::size_t column = 0; column < samples.counterLabels.size(); ++column)
  {
    quantities.push_back(
        FitQuantity {.label = samples.counterLabels.at(column),
                     .scalesWithUnit = false,
                     .values = samples.counterValues.at(column),
                     .curve = {},
                     .coefficient = 0.0,
                     .rmsPercent = 0.0,
                     .reason = {}});
  }
  for (std::size_t column = 0; column < samples.metricLabels.size(); ++column) {
    quantities.push_back(FitQuantity {.label = samples.metricLabels.at(column),
                                      .scalesWithUnit = false,
                                      .values = samples.metricValues.at(column),
                                      .curve = {},
                                      .coefficient = 0.0,
                                      .rmsPercent = 0.0,
                                      .reason = {}});
  }

  const auto fit = fitFamily(samples.sizes,
                             std::move(quantities),
                             samples.family->bigO,
                             samples.family->bigOFun);
  const std::string familyName = samples.family->name;
  const auto timeUnit = samples.unit.value_or(TimeUnit::NANOSECONDS);

  auto outcome = BenchmarkResult {};
  // FR-022: the fit reports for the family and for no single instance, so the
  // name fields carry the family name the rows print. The unit is the one the
  // rows of the family printed under, because the coefficient of the primary
  // time stands in it (FR-015).
  outcome.name = familyName;
  outcome.suite = familyName;
  outcome.caseName = familyName;
  outcome.timeUnit = timeUnit;
  if (!fit.reason.empty()) {
    // FR-023: the family states its reason in place of every pair.
    outcome.outcome = RunOutcome::SKIPPED;
    outcome.reason = fit.reason;
    return outcome;
  }
  for (const auto& quantity : fit.quantities) {
    if (!quantity.reason.empty()) {
      // FR-023, spec Edge Cases: the quantity that does not stand in every
      // instance holds no pair and states its own reason.
      auto& row = outcome.rows.emplace_back();
      row.kind = RowKind::SKIPPED;
      row.fit.quantity = quantity.label;
      row.fit.reason = quantity.reason;
      continue;
    }
    auto& bigO = outcome.rows.emplace_back();
    bigO.kind = RowKind::BIG_O;
    bigO.fit.quantity = quantity.label;
    bigO.fit.curve = quantity.curve;
    bigO.fit.coefficient = quantity.coefficient;
    bigO.fit.scalesWithUnit = quantity.scalesWithUnit;
    auto& rms = outcome.rows.emplace_back();
    rms.kind = RowKind::RMS;
    rms.fit.quantity = quantity.label;
    rms.fit.rmsPercent = quantity.rmsPercent;
  }
  return outcome;
}

}  // namespace sg::detail
