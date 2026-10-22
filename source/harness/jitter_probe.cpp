// ============================================================================
// The jitter probe work item and its availability check (T009, US1).
//
// One translation unit owns the probe (R-01): the holder's lifecycle, the
// per-cpu work item the probe contexts run, the availability check over
// the provider catalog (Q-1), the run-level
// fold, the window-sided fold the split of E-06 reads, and the
// reader-install seam storage.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "detail/jitter_probe.hpp"

#include "detail/percentiles.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace sg::detail
{
namespace
{

// The measurement pass takes a fixed 1,024 loop reads the loop discards
// (E-05, R-05).
inline constexpr std::uint64_t kPassReads = 1024;

// The rise the default threshold takes at ten times the measured minimum
// loop period (FR-007, D-2).
inline constexpr std::uint64_t kRiseFactor = 10;

// The reader-install seam's storage: the reader a scenario installs
// stands in place of the one the work item opens over the machine/tsc
// leaf (R-02, Q-5). A null pointer clears, the way `installTestLayout`
// clears with an empty optional.
std::unique_ptr<counters::WindowReader> installedReader;

// The duration-division relation of E-03: the fields the fold filled
// answer the covered span. `-Wfloat-equal` bars the direct comparison,
// so the relation reads the difference against the scale of the
// values, the reading percentiles.cpp's ceiling takes. True on the
// zero span, the case a kept set with shared stamps answers no finite
// division through.
[[maybe_unused]] auto durationDivisionsHold(const JitterStats& stats) -> bool
{
  if (stats.runtimeNs == 0) {
    return true;
  }
  const double perSecond = static_cast<double>(stats.gapCount) * 1e9
      / static_cast<double>(stats.runtimeNs);
  const double percent = static_cast<double>(stats.totalNs) * 100.0
      / static_cast<double>(stats.runtimeNs);
  return std::fabs(stats.gapsPerSecond - perSecond)
      <= 1e-9 * (std::fabs(perSecond) + 1.0)
      && std::fabs(stats.totalPercent - percent)
      <= 1e-9 * (std::fabs(percent) + 1.0);
}

// The availability check (Q-1): the machine object publishes the tsc
// leaf, countable, at the read mode the probe reads through (R-02).
// `System::local()` constructs the machine root beside its catalog
// (system.cpp), so the lookup answers on every build, and the loop
// answers whether a registered provider published the qualifying
// leaf. The publication answers to one build fact (`kTscAvailable`,
// clock_provider.cpp), the one fact the open refuses the request on,
// so the catalog's answer carries the open's.
auto catalogCarriesTsc() -> bool
{
  const auto machine = counters::System::local().object("machine");
  for (const auto& entry : machine.value().counters()) {
    if (entry.name == "tsc" && entry.mode == counters::ReadMode::FAST_TSC
        && entry.avail == counters::Availability::COUNTABLE)
    {
      return true;
    }
  }
  return false;
}

// One record store into the pre-sized buffer, or the drop the filled
// buffer answers with: the stamp is lost and the count, the total and
// the max of the dropped gaps stand in the per-cpu counters (E-04,
// FR-010, D-6).
auto keepOrDrop(ProbeCpuState& state,
                const std::uint64_t stamp,
                const std::uint64_t gap) noexcept -> void
{
  if (state.kept < state.buffer.size()) {
    state.buffer.at(state.kept) = GapRecord {.stamp = stamp, .gapNs = gap};
    ++state.kept;
    return;
  }
  ++state.droppedGaps;
  state.droppedTotalNs += gap;
  if (gap > state.droppedMaxNs) {
    state.droppedMaxNs = gap;
  }
}

// The recorder half of the read move: the one-leaf plan over the
// machine/tsc leaf the catalog published (R-02, Q-1). The availability
// gate (Q-1) answered before this start, and the catalog freezes at the
// boundary the first lookup opened (FR-009), so every lookup here
// answers what the gate's did: the machine object and the typed tsc
// leaf stand in the tree the gate read, the one-leaf expression a
// resolved countable leaf built compiles - the refusals `compileCore`
// answers stand on a zero-leaf expression, an address outside the
// tree, and a leaf the availability gate refuses, and the gate matched
// a countable entry - and the ring mint over the literal capacity 2, a
// power of two, succeeds (FR-025). A catalog that broke any of these
// breaks the contract the assertions own.
auto openSystemRecorder(
    std::optional<counters::Plan>& plan,
    std::optional<counters::RecorderHandle<counters::Ring>>& recorder)
    -> void
{
  auto& sys = counters::System::local();
  const auto machine = sys.object("machine");
  SG_ASSERT(machine.has_value(),
            "the machine object stands in the catalog (Q-1)");
  const auto leaf = machine.value().counter<counters::Dim<0, 1>>("tsc");
  SG_ASSERT(leaf.has_value(),
            "the machine/tsc leaf stands in the catalog (Q-1)");
  counters::Expression<counters::Dim<0, 1>> expression(leaf.value());
  auto compiled = counters::detail::compileCore(
      sys, counters::Target {}, {&expression.core});
  SG_ASSERT(compiled.has_value(),
            "the one-leaf tsc plan compiles (R-02)");
  plan.emplace(std::move(compiled.value()));
  auto minted = plan->recorder(2, counters::ring);
  SG_ASSERT(minted.has_value(),
            "the ring recorder mints over the literal capacity (FR-025)");
  recorder.emplace(std::move(minted.value()));
}

struct ProbeSession;

// The per-cpu work item: the buffer's first touch, the measurement pass,
// the phases, and the four-move loop (FR-005, FR-009, R-03, R-04).
struct ProbeItem
{
  ProbeCpuState* state = nullptr;
  const JitterProbeOptions* options = nullptr;
  ProbeSession* session = nullptr;

  auto operator()(sg::WorkContext& /*work*/) noexcept -> void;
};

// The state one live holder's start owns: the seam reader, the start
// barrier, the latch, the work nodes and the probe contexts. The
// contexts stand declared last, so their destruction runs first: a node
// dies behind the join edge of the context that settled it (FR-012,
// R-04). One holder holds the process's session at a time (R-01).
struct ProbeSession
{
  std::unique_ptr<counters::WindowReader> reader;
  std::optional<ContextBarrier> barrier;
  // The second synchronizer between the holder and the items: the round
  // the items reach after the buffer's first touch, the measurement
  // pass and, in the precheck and both modes, the pre-check phase, and
  // the holder reaches before start() returns, so the batch's first
  // benchmark begins behind them (FR-006, FR-009, FR-014, R-04).
  std::optional<ContextBarrier> latch;
  std::vector<std::unique_ptr<sg::WorkNode<ProbeItem>>> nodes;
  std::vector<std::unique_ptr<sg::ExecutionContext>> contexts;
};

auto ProbeItem::operator()(sg::WorkContext& /*work*/) noexcept -> void
{
  // The start line: every chosen cpu arrives at the barrier the holder
  // owns, and the holder arrives beside them, so the alongside loop
  // spans the batch from its first read: the batch begins only when
  // every item's thread stands live at this line (FR-006, R-01, R-04).
  session->barrier->arriveAndWait();

  // The buffer allocates and first touches on this probe cpu, after
  // the pin the context applied at thread start and before the first
  // read, so the first-touch faults land on the probe cpu and outside
  // every measured window (FR-009, R-03).
  state->buffer.assign(options->gapCount, GapRecord {});

  // The read move: the installed reader's thunk over a fresh stack
  // point sink per pass, the way the plan's own sampling action
  // builds one, or the one-leaf plan's recorder through the system
  // otherwise (R-02, Q-1). The system open asserts the availability
  // gate's answer, so the item always reaches the latch line below
  // (FR-006, R-04).
  std::uint64_t cell = 0;
  counters::WindowReader* reader = session->reader.get();
  counters::WindowReader::ReadThunk thunk = nullptr;
  std::optional<counters::Plan> plan;
  std::optional<counters::RecorderHandle<counters::Ring>> recorder;
  if (reader != nullptr) {
    thunk = reader->resolveThunk();
  } else {
    openSystemRecorder(plan, recorder);
  }
  const auto readStamp = [&]() noexcept -> std::uint64_t
  {
    if (thunk != nullptr) {
      counters::PointSink sink(&cell, 1, 1, 1, 0);
      thunk(*reader, sink);
      return cell;
    }
    recorder->sample();
    return recorder
        ->mColumns[(recorder->mHead - 1) & (recorder->mCapacity - 1)];
  };

  // The measurement pass: the 1,024 reads take no record and set the
  // effective threshold, the default rising to ten times the minimum
  // period the pass measured when that stands larger, and an explicit
  // value standing as given (E-05, FR-007, D-2, R-05).
  std::uint64_t last = readStamp();
  std::uint64_t minDelta = 0;
  for (std::uint64_t pass = 1; pass < kPassReads; ++pass) {
    const std::uint64_t stamp = readStamp();
    const std::uint64_t delta = stamp - last;
    if (pass == 1 || delta < minDelta) {
      minDelta = delta;
    }
    last = stamp;
  }
  state->loopPeriodNs = minDelta;
  const std::uint64_t risen = kRiseFactor * minDelta;
  state->thresholdNs = options->thresholdNs.has_value()
      ? *options->thresholdNs
      : (risen > kDefaultThresholdNs ? risen : kDefaultThresholdNs);

  // The pre-check phase: the cpus stand lined at the start barrier,
  // so all of them spin together for the precheck duration, measured
  // through the same reader in the leaf's one time base (FR-014, R-10,
  // R-04).
  if (options->mode == JitterProbeMode::PRECHECK
      || options->mode == JitterProbeMode::BOTH)
  {
    const std::uint64_t span =
        static_cast<std::uint64_t>(options->precheckMs) * 1000000ULL;
    const std::uint64_t phaseStart = last;
    while (true) {
      const std::uint64_t stamp = readStamp();
      const std::uint64_t gap = stamp - last;
      if (gap >= state->thresholdNs) {
        keepOrDrop(*state, stamp, gap);
      }
      last = stamp;
      if (stamp - phaseStart >= span) {
        break;
      }
    }
  }

  // The latch line: the item arrives here after its buffer's first
  // touch, its measurement pass and, in the precheck and both modes,
  // its pre-check phase; the holder waits on the same line, so
  // start() returns only after every item stands past them and the
  // batch's first benchmark begins behind them (FR-006, FR-009,
  // FR-014, R-04).
  session->latch->arriveAndWait();

  // The alongside phase: the loop spans the batch, from the first
  // read after the phases to the read of the stop flag, one reader
  // call, one comparison, one record store and one stop-flag read per
  // pass, and no allocation, no syscall, no I/O anywhere in the body
  // (FR-005, FR-006, D-7, R-04).
  if (options->mode == JitterProbeMode::ALONGSIDE
      || options->mode == JitterProbeMode::BOTH)
  {
    while (!state->stop.load(std::memory_order_relaxed)) {
      const std::uint64_t stamp = readStamp();
      const std::uint64_t gap = stamp - last;
      if (gap >= state->thresholdNs) {
        keepOrDrop(*state, stamp, gap);
      }
      last = stamp;
    }
  }
}

// The live session; a holder that never started leaves it null, and
// `stopAndJoin` tears it down behind the join edge (R-01, R-04).
std::unique_ptr<ProbeSession> probeSession;

// The percentile field of the run-level fold: the nearest-rank rule of
// `percentileAt` over the sorted kept gaps (PC-4, R-05). A column that
// holds fewer samples than the rule's reporting floor answers the rank
// the rule itself names, so the chain the fold's postcondition states
// holds over the small kept sets too. Both folds early-return on the
// empty kept set, so the set here carries at least one record, and the
// five reporting points the folds call with stand inside [1, 100), so
// the nearest-rank index lands inside [1, size] and the empty-set and
// clamp arms a general percentile table carries stand outside the
// grammar (K-8, E-03).
auto percentileField(const std::vector<double>& sorted,
                     const double point) -> std::uint64_t
{
  const auto ranked = percentileAt(sorted, point);
  if (ranked.available) {
    return static_cast<std::uint64_t>(ranked.value);
  }
  const auto rank =
      std::ceil(point * static_cast<double>(sorted.size()) / 100.0);
  return static_cast<std::uint64_t>(
      sorted.at(static_cast<std::size_t>(rank) - 1));
}

}  // namespace

JitterProbe::JitterProbe(const JitterProbeOptions& options)
    : m_options(options)
{
}

auto JitterProbe::start() -> void
{
  SG_REQUIRE(probeSession == nullptr,
             "one probe holds the process's session at a time (R-01)");

  // The availability check (Q-1): the provider catalog for the
  // machine/tsc leaf. The catalog publishes the qualifying entry only
  // where the build executes the time-stamp instruction, the one fact
  // the open refuses the request on, so the check asks the catalog
  // alone.
  if (!catalogCarriesTsc()) {
    return;
  }

  // The availability gate stands here; the idle resolution and the
  // overlap check ran in the entry point before this start (R-07), so
  // an empty list arrives only when no probe stands to run.
  if (m_options.cpus.empty()) {
    return;
  }

  auto session = std::make_unique<ProbeSession>();

  // The states build once, in place: the work items reference them for
  // the whole run, so the vector never grows after this line (E-05).
  m_states = std::vector<ProbeCpuState>(m_options.cpus.size());
  for (std::size_t index = 0; index < m_options.cpus.size(); ++index) {
    m_states.at(index).cpu = m_options.cpus.at(index);
  }

  // The party count: every chosen cpu at the item's start line, and the
  // holder itself, so the batch's first read waits for the probe's
  // (FR-006, R-01, R-04).
  session->barrier.emplace(static_cast<int>(m_options.cpus.size() + 1));

  // The latch party count: the same parties meet again at the latch
  // line, so the holder's return waits for every item's buffer first
  // touch, its measurement pass and, in the precheck and both modes,
  // its pre-check phase (FR-006, FR-009, FR-014, R-04).
  session->latch.emplace(static_cast<int>(m_options.cpus.size() + 1));

  // The seam reader moves out of the slot at start, so a scenario's
  // next install stands alone (R-02, Q-5).
  session->reader = std::move(installedReader);

  for (const auto cpu : m_options.cpus) {
    session->contexts.push_back(std::make_unique<sg::ExecutionContext>(
        sg::ContextPolicy {sg::WaitMode::SPIN, sg::PlacementSlot::cpu(cpu)}));
  }
  for (std::size_t index = 0; index < m_states.size(); ++index) {
    session->nodes.push_back(std::make_unique<sg::WorkNode<ProbeItem>>(
        ProbeItem {.state = &m_states.at(index),
                   .options = &m_options,
                   .session = session.get()}));
  }
  for (std::size_t index = 0; index < session->contexts.size(); ++index) {
    sg::dispatch(*session->contexts.at(index), *session->nodes.at(index));
  }
  // The holder's own arrival at the start line: the dispatch returns
  // once every item's thread stands live at the line, so the batch that
  // follows runs beside the probe's reads from their first one (FR-006,
  // R-01, R-04).
  session->barrier->arriveAndWait();

  // The holder's own arrival at the latch line: the items stand there
  // after their buffer's first touch, their measurement pass and, in
  // the precheck and both modes, their pre-check phase, so the return
  // stands behind them and the first benchmark the runner dispatches
  // never meets a probe cpu still in its setup or its pre-check spin
  // (FR-006, FR-009, FR-014, R-04).
  session->latch->arriveAndWait();

  probeSession = std::move(session);

  if (m_options.mode == JitterProbeMode::PRECHECK) {
    // FR-014, D-3: the precheck mode allows every overlap, because the
    // probe stops before the benchmarks start, so the stop and join
    // stand here, inside the start, and the states stay readable where
    // the runner reads them (R-04).
    stopAndJoin();
  }
}

auto JitterProbe::stopAndJoin() -> void
{
  if (probeSession == nullptr) {
    return;
  }

  // The stop flag stands first, so the loop's own read ends the work
  // item; the context's stop, drain and join then end the loop behind
  // the item (FR-006, R-04).
  for (auto& state : m_states) {
    state.stop.store(true, std::memory_order_relaxed);
  }
  for (auto& context : probeSession->contexts) {
    context->requestStop();
    context->drain();
    context->join();
    context.reset();
  }
  probeSession->nodes.clear();
  probeSession.reset();
}

auto JitterProbe::states() const noexcept -> const std::vector<ProbeCpuState>&
{
  return m_states;
}

/**
 * @brief The run-level fold of one probe cpu's kept gaps (E-03): the
 * sixteen fields over the whole kept set. The covered span the
 * duration fields divide is the kept records' own stamp span, the
 * only duration the joined per-cpu state carries; the loop's
 * start-to-stop span needs a field the state does not declare, and
 * the deviation rides the wave ledger (E-03, R-05). The empty kept
 * set answers the zero fields and none of the rows below.
 *
 * \post minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <=
 *       maxNs
 * \post `gapCount` counts the kept gaps, and `droppedGaps` counts the
 *       gaps the buffer dropped (FR-010, D-6)
 * \post `totalPercent` divides the kept gaps' total by `runtimeNs`
 *       (E-03, R-05)
 * \post `gapsPerSecond` divides `gapCount` by the seconds of
 *       `runtimeNs` (E-03, R-05)
 * \post `thresholdNs` and `loopPeriodNs` carry the state's own
 *       (FR-007, R-05)
 * \post `runtimeNs` is the kept records' stamp span (E-03, R-05)
 */
auto gapStatsOf(const ProbeCpuState& state) -> JitterStats
{
  JitterStats stats {};

  std::vector<double> sorted;
  sorted.reserve(state.kept);
  std::uint64_t total = 0;
  for (std::size_t index = 0; index < state.kept; ++index) {
    const auto gap = state.buffer.at(index).gapNs;
    sorted.push_back(static_cast<double>(gap));
    total += gap;
  }
  std::sort(sorted.begin(), sorted.end());

  stats.gapCount = state.kept;
  stats.droppedGaps = state.droppedGaps;
  stats.thresholdNs = state.thresholdNs;
  stats.loopPeriodNs = state.loopPeriodNs;
  if (sorted.empty()) {
    return stats;
  }

  stats.minNs = static_cast<std::uint64_t>(sorted.front());
  stats.maxNs = static_cast<std::uint64_t>(sorted.back());
  stats.totalNs = total;
  stats.meanNs =
      static_cast<double>(total) / static_cast<double>(sorted.size());
  stats.medianNs = percentileField(sorted, 50.0);
  stats.p90Ns = percentileField(sorted, 90.0);
  stats.p99Ns = percentileField(sorted, 99.0);
  stats.p999Ns = percentileField(sorted, 99.9);
  stats.p9999Ns = percentileField(sorted, 99.99);

  // The covered span the run-level fold reports: the kept records' own
  // stamp span, the only duration the joined per-cpu state carries. The
  // loop's start-to-stop span needs a field the state does not declare;
  // the deviation rides the wave ledger (E-03, R-05).
  std::uint64_t firstStamp = state.buffer.front().stamp;
  std::uint64_t lastStamp = firstStamp;
  for (std::size_t index = 1; index < state.kept; ++index) {
    const auto stamp = state.buffer.at(index).stamp;
    if (stamp < firstStamp) {
      firstStamp = stamp;
    }
    if (stamp > lastStamp) {
      lastStamp = stamp;
    }
  }
  stats.runtimeNs = lastStamp - firstStamp;

  stats.gapsPerSecond = static_cast<double>(stats.gapCount) * 1e9
      / static_cast<double>(stats.runtimeNs);
  stats.totalPercent = static_cast<double>(stats.totalNs) * 100.0
      / static_cast<double>(stats.runtimeNs);
  SG_ENSURE(stats.minNs <= stats.medianNs && stats.medianNs <= stats.p90Ns
                && stats.p90Ns <= stats.p99Ns && stats.p99Ns <= stats.p999Ns
                && stats.p999Ns <= stats.p9999Ns
                && stats.p9999Ns <= stats.maxNs,
            "the percentile chain stands ordered (E-03, PC-4)");
  SG_ENSURE(
      stats.gapCount == state.kept && stats.droppedGaps == state.droppedGaps,
      "the counts answer the kept and dropped records (FR-010, D-6)");
  SG_ENSURE(stats.thresholdNs == state.thresholdNs
                && stats.loopPeriodNs == state.loopPeriodNs,
            "the effective fields carry the state's own (FR-007, R-05)");
  SG_ENSURE(stats.runtimeNs == lastStamp - firstStamp,
            "the covered span is the kept records' stamp span (E-03)");
  SG_ENSURE(durationDivisionsHold(stats),
            "the duration divisions answer the covered span (E-03, R-05)");
  return stats;
}

/**
 * @brief The per-benchmark fold of one probe cpu's kept gaps over one
 * benchmark's timed windows (E-03, E-06): the sixteen fields over the
 * kept set the windows placed, the covered span the windows' total
 * duration. The empty placement answers the zero fields and none of
 * the rows below.
 *
 * \post minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <=
 *       maxNs
 * \post `gapCount` counts the placed gaps, and `droppedGaps` counts
 *       the gaps the buffer dropped (FR-010, D-6, E-06)
 * \post `totalPercent` divides the placed gaps' total by `runtimeNs`
 *       (E-03, R-05)
 * \post `gapsPerSecond` divides `gapCount` by the seconds of
 *       `runtimeNs` (E-03, R-05)
 * \post `thresholdNs` and `loopPeriodNs` carry the state's own
 *       (FR-007, R-05)
 * \post `runtimeNs` is the windows' total duration (E-03, R-05)
 */
auto gapStatsOf(const ProbeCpuState& state,
                std::span<const TimedWindow> windows) -> JitterStats
{
  JitterStats stats {};

  // The covered span the entry's fields divide (E-03, R-05): the
  // windows' total, the one duration the window set carries. The fold
  // of an empty placement answers the empty kept set's zeros below.
  std::uint64_t coveredNs = 0;
  for (const auto& window : windows) {
    coveredNs += window.end - window.start;
  }

  std::vector<double> sorted;
  sorted.reserve(state.kept);
  std::uint64_t total = 0;
  for (std::size_t index = 0; index < state.kept; ++index) {
    const auto& record = state.buffer.at(index);
    // A kept gap folds into every window whose half-open `[start, end)`
    // contains its stamp (E-06, R-04); the entry's stats fold each
    // placed record once, the one reading that leaves `gapCount` the
    // count of the gaps the benchmark's windows placed.
    const bool placed = std::any_of(
        windows.begin(),
        windows.end(),
        [&](const TimedWindow& window) noexcept -> bool
        { return window.start <= record.stamp && record.stamp < window.end; });
    if (!placed) {
      continue;
    }
    sorted.push_back(static_cast<double>(record.gapNs));
    total += record.gapNs;
  }
  std::sort(sorted.begin(), sorted.end());

  stats.gapCount = sorted.size();
  stats.droppedGaps = state.droppedGaps;
  stats.thresholdNs = state.thresholdNs;
  stats.loopPeriodNs = state.loopPeriodNs;
  if (sorted.empty()) {
    return stats;
  }

  stats.minNs = static_cast<std::uint64_t>(sorted.front());
  stats.maxNs = static_cast<std::uint64_t>(sorted.back());
  stats.totalNs = total;
  stats.meanNs =
      static_cast<double>(total) / static_cast<double>(sorted.size());
  stats.medianNs = percentileField(sorted, 50.0);
  stats.p90Ns = percentileField(sorted, 90.0);
  stats.p99Ns = percentileField(sorted, 99.0);
  stats.p999Ns = percentileField(sorted, 99.9);
  stats.p9999Ns = percentileField(sorted, 99.99);
  stats.runtimeNs = coveredNs;
  stats.gapsPerSecond = static_cast<double>(stats.gapCount) * 1e9
      / static_cast<double>(stats.runtimeNs);
  stats.totalPercent = static_cast<double>(stats.totalNs) * 100.0
      / static_cast<double>(stats.runtimeNs);
  SG_ENSURE(stats.minNs <= stats.medianNs && stats.medianNs <= stats.p90Ns
                && stats.p90Ns <= stats.p99Ns && stats.p99Ns <= stats.p999Ns
                && stats.p999Ns <= stats.p9999Ns
                && stats.p9999Ns <= stats.maxNs,
            "the percentile chain stands ordered (E-03, PC-4)");
  SG_ENSURE(
      stats.gapCount == sorted.size() && stats.droppedGaps == state.droppedGaps,
      "the counts answer the placed and dropped records (FR-010, D-6)");
  SG_ENSURE(stats.thresholdNs == state.thresholdNs
                && stats.loopPeriodNs == state.loopPeriodNs,
            "the effective fields carry the state's own (FR-007, R-05)");
  SG_ENSURE(stats.runtimeNs == coveredNs,
            "the covered span is the windows' total duration (E-03)");
  SG_ENSURE(durationDivisionsHold(stats),
            "the duration divisions answer the covered span (E-03, R-05)");
  return stats;
}

auto installTestReader(std::unique_ptr<counters::WindowReader> reader) -> void
{
  installedReader = std::move(reader);
}

}  // namespace sg::detail
