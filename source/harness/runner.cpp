#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "detail/calibration.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace
{

// FR-007 and FR-011 carry the defaults; FR-010 and FR-016 bound the
// growth, and detail/calibration.hpp owns that rule.
constexpr std::int64_t kDefaultMinTimeNs = 500'000'000;
constexpr std::int64_t kDefaultWarmupNs = 0;
constexpr std::uint64_t kDefaultRepetitions = 1;

// A metric column of one run: the label, the FR-022 per-iteration
// decision, the erased spine, and the availability the catalog
// disclosed before the run (FR-023).
struct MetricColumn
{
  std::string label;
  bool perIteration = false;
  bool unavailable = false;
  sg::counters::Availability availability =
      sg::counters::Availability::COUNTABLE;
  sg::counters::detail::ExprCore core;
  // The sink column of this leaf: its index among the run's cores,
  // which is the order of the fold totals (FR-052).
  std::size_t foldColumn = 0;
};

auto splitAddress(std::string_view address)
    -> std::pair<std::string_view, std::string_view>
{
  const std::size_t slash = address.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {address, {}};
  }
  return {address.substr(0, slash), address.substr(slash + 1)};
}

auto statisticsOf(std::vector<double> values) -> sg::Statistics
{
  sg::Statistics stats;
  SG_ASSERT(!values.empty(),
            "the caller builds the summary only for a measured run with more "
            "than one repetition, so it hands over one value per row");

  std::sort(values.begin(), values.end());
  stats.samples = static_cast<std::uint64_t>(values.size());
  stats.min = values.front();
  stats.max = values.back();

  double sum = 0.0;
  for (const double value : values) {
    sum += value;
  }
  stats.mean = sum / static_cast<double>(values.size());

  const std::size_t n = values.size();
  stats.median =
      (n % 2 == 1) ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2.0;

  double squared = 0.0;
  for (const double value : values) {
    squared += (value - stats.mean) * (value - stats.mean);
  }
  SG_ASSERT(n > 1,
            "the caller builds the summary only for a measured run with more "
            "than one repetition");
  stats.sampleStdDev = std::sqrt(squared / static_cast<double>(n - 1));
  stats.coefficientOfVariation = std::fpclassify(stats.mean) != FP_ZERO
      ? stats.sampleStdDev / stats.mean
      : 0.0;
  return stats;
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
                       double overheadFloorNs,
                       const sg::counters::detail::ExprCore* primaryCore,
                       const Recorder& recorder,
                       std::size_t primaryFoldColumn) -> void
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
  row.timeAvailability = primaryCore != nullptr
      ? nettedFold(*primaryCore, recorder, record, primaryFoldColumn)
            .availability
      : sg::counters::Availability::COUNTABLE;
  row.overheadFloorNs = overheadFloorNs;
  for (const auto& column : columns) {
    sg::MetricValue value;
    if (column.unavailable) {
      value.availability = column.availability;
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
}

}  // namespace

namespace sg
{

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
      std::fprintf(
          stderr, "%s: %s\n", address.c_str(), object.error().message.c_str());
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
                   address.c_str());
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

  auto plan =
      sg::counters::detail::compileCore(sys, sg::counters::Target {}, cores);
  if (!plan.has_value()) {
    return fail("the run plan does not compile: " + plan.error().message);
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
  // through the run's two-point slot, minted once beside the plan, so
  // a run that pauses in every iteration completes.
  auto slotReset = plan->recorder(2);
  const double overheadFloorNs = plan->sampleOverheadNsMedian();

  // The attempt's recorder, set anew when every attempt reserves, and
  // read once more when the measured row folds its columns.
  std::optional<sg::counters::RecorderHandle<sg::counters::HardStop>>
      attemptRecorder;

  // R-06: one flag read after each run ends the benchmark with this
  // reason, and the process exits nonzero.

  auto sampleRun = [&](const std::uint64_t iterations)
      -> std::expected<RunRecord, std::string>
  {
    RunRecord record;
    record.iterations = iterations;

    // E-08, FR-051: two points per closed pause pair the previous
    // attempt observed per iteration, times this attempt's iteration
    // count, and no more: FR-025 and Q-10 cap the room, and an attempt
    // over the cap reserves no room and folds instead.
    const std::uint64_t history = entry.pausePairsPerIteration;
    std::size_t pauseRoom = 0;
    if (history > 0 && 2 * history <= kMaxPauseSamplesPerLeaf
        && iterations <= kMaxPauseSamplesPerLeaf / (2 * history))
    {
      pauseRoom = static_cast<std::size_t>(2 * history * iterations);
    }
    lastRecorderReservation() = RecorderReservation {2, pauseRoom};
    if (pauseRoom > maxRecorderPauseRoom()) {
      maxRecorderPauseRoom() = pauseRoom;
    }
    attemptRecorder = plan->recorder(static_cast<std::size_t>(2) + pauseRoom);

    // R-05, E-07: the callback state carries the arguments this
    // instance owns and holds no recorder, so it opens no sampling
    // window and the loop and skip operations of it are precondition
    // violations (FR-018).
    State callbackState(iterations, nullptr, instance.arguments, true);

    // R-04: the state reads the arguments this instance owns. The
    // instance outlives every run of itself, so the span needs no copy,
    // and the timed loop keeps its H1 shape (FR-009).
    State state(iterations, &*attemptRecorder, instance.arguments);
    // FR-052: the run's fold sink carries one running total per column
    // of the run, fed at the commit of every pair the room cannot hold.
    state.m_slotReset = slotReset;
    state.m_pauseFold = &record.pauseFold;
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
    // pairs that folded stand in the sink's totals.
    record.samples = state.m_samplesTaken;
    record.foldPoints = state.m_foldPoints;
    record.pauseSamplesFolded = state.m_foldedPairs > 0;
    record.pausePairs = state.m_pausePairs;
    record.realNs = static_cast<std::int64_t>(std::llround(
        nettedFold(realTime.core, *attemptRecorder, record, 0).value));
    record.cpuNs = static_cast<std::int64_t>(std::llround(
        nettedFold(cpuTime.core, *attemptRecorder, record, 1).value));

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
    if (state.outcome() == RunOutcome::SKIPPED) {
      record.skipped = true;
      record.reason = state.reason();
    }
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
      return result;
    }
    if (warm.outcome == GrowOutcome::INTERRUPTED) {
      markInterrupted(result);
      return result;
    }
  }

  // FR-014: a dry run takes one iteration, one repetition, and no
  // warm-up, and it qualifies at once.
  const std::uint64_t repetitionsOfRun = dryRun ? 1 : repetitions;

  for (std::uint64_t repetition = 0; repetition < repetitionsOfRun;
       ++repetition)
  {
    std::uint64_t iterations = dryRun ? 1 : settled;
    std::expected<RunRecord, std::string> chosen = std::unexpected("");

    if (repetition == 0 && !fixedIterations.has_value() && !dryRun) {
      // FR-014: the first repetition calibrates from one iteration, and
      // the run that qualifies is its measured run.
      const auto grown = growUntilQualified(1,
                                            minTimeNs,
                                            kRunBound,
                                            PhaseText {"calibration", "FR-016"},
                                            sampleRun,
                                            []() noexcept -> bool
                                            { return interruptFlag().load(); });
      if (grown.outcome == GrowOutcome::FAILED
          || grown.outcome == GrowOutcome::BOUND_EXHAUSTED)
      {
        return fail(grown.error);
      }
      if (grown.outcome == GrowOutcome::SKIPPED) {
        result.outcome = RunOutcome::SKIPPED;
        result.reason = grown.record.reason;
        return result;
      }
      if (grown.outcome == GrowOutcome::INTERRUPTED) {
        markInterrupted(result);
        return result;
      }
      chosen = std::move(grown.record);
      iterations = chosen->iterations;
      settled = iterations;
    } else {
      chosen = sampleRun(iterations);
      if (!chosen.has_value()) {
        return fail(chosen.error());
      }
      if (chosen->skipped) {
        result.outcome = RunOutcome::SKIPPED;
        result.reason = chosen->reason;
        return result;
      }
    }

    // FR-002, FR-005, FR-006: the fold behind the availability is the
    // CPU core by default and the real core under the real selection.
    // A manual run answers with a null core: its primary time is the
    // sum, and no fold stands between it and the row.
    const sg::counters::detail::ExprCore* primaryCore = entry.manualTime
        ? nullptr
        : (entry.realTime ? &realTime.core : &cpuTime.core);
    appendMeasuredRow(result.rows,
                      *chosen,
                      columns,
                      overheadFloorNs,
                      primaryCore,
                      *attemptRecorder,
                      entry.realTime ? 0 : 1);

    if (interruptFlag().load()) {
      result.outcome = RunOutcome::SKIPPED;
      result.reason = kInterruptReason;
      break;
    }
  }

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
        times.empty() ? sg::Statistics {} : statisticsOf(std::move(times));

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
      sg::Statistics columnStats =
          values.empty() ? sg::Statistics {} : statisticsOf(std::move(values));
      aggregate.metricStats.push_back(columnStats);
    }

    result.rows.push_back(aggregate);
  }

  return result;
}

}  // namespace sg::detail
