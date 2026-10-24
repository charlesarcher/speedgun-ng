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
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "detail/jitter_probe.hpp"

#include <sched.h>

#include "detail/options.hpp"
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
    std::optional<counters::RecorderHandle<counters::Ring>>& recorder) -> void
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
  SG_ASSERT(compiled.has_value(), "the one-leaf tsc plan compiles (R-02)");
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

  // The wait stands outside the record: the alongside phase compares
  // each stamp with the one read here, behind the latch, so the time
  // the item spent waiting on the latch names no gap (FR-006, R-04).
  last = readStamp();

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

// The duration-division relation of E-03: the fields the fold filled
// answer the covered span. `-Wfloat-equal` bars the direct comparison,
// so the relation reads the difference against the scale of the
// values, the reading percentiles.cpp's ceiling takes. True on the
// zero span, the case a kept set with shared stamps answers no finite
// division through. The relation stands outside the file's unnamed
// namespace because the contract tests read it by name (E-03).
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

  // The availability gate stands here; the entry point resolved the
  // cpu list before this start - the precheck mode carries the planned
  // cpus, the alongside and both modes the free cpus the idle
  // resolution yields (R-07) - and stopped the batch on an empty one,
  // so an empty list arrives only from a probe armed outside the
  // batch's entry point.
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

namespace
{

// FR-011 with the value grammar of contracts/cli-options.md: a number
// with no sign and no trailing text; the cpu tokens admit zero, the
// keys do not.
auto parseProbeNumber(const std::string& text,
                      std::uint64_t& out,
                      const bool allowZero) -> bool
{
  if (text.empty() || text.front() < '0' || text.front() > '9') {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const long long number = std::strtoll(text.c_str(), &end, 10);
  if (errno != 0 || end != text.c_str() + text.size()) {
    return false;
  }
  if (!allowZero && number <= 0) {
    return false;
  }
  out = static_cast<std::uint64_t>(number);
  return true;
}

// The cpu token of the value grammar: an index or a closed a-b range,
// the form the topology listing prints; each cpu stands in the
// process affinity mask (FR-011, FR-038).
auto parseProbeCpuToken(const std::string& token,
                        std::vector<std::uint32_t>& cpus,
                        std::string& message) -> bool
{
  const auto dash = token.find('-');
  std::uint64_t low = 0;
  std::uint64_t high = 0;
  if (dash == std::string::npos) {
    if (!parseProbeNumber(token, low, true)) {
      message =
          "the cpu '" + quotedText(token) + "' is not a cpu index or range";
      return false;
    }
    high = low;
  } else {
    const auto lowText = token.substr(0, dash);
    const auto highText = token.substr(dash + 1);
    if (!parseProbeNumber(lowText, low, true)
        || !parseProbeNumber(highText, high, true) || low > high)
    {
      message =
          "the cpu '" + quotedText(token) + "' is not a cpu index or range";
      return false;
    }
  }
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    message = "the process affinity mask is unreadable";
    return false;
  }
  // FR-011: the mask bit read carries the set type's own bound and
  // the membership in one test - the read answers 0 for an index at
  // or above the set size and for an index the mask does not hold -
  // so one line stands for both refusals. The count the threads
  // report bounds the index the ExecutionContext construction reads
  // beside the mask: a count and not the highest index, so the bound
  // refuses an online cpu the mask holds whose index stands at or
  // above the count, on the line of its own below. The count stands
  // behind the detail seam the tests set and clear, the way the test
  // layout installs (FR-041).
  const auto online = onlineCpuCount();
  for (auto cpu = low; cpu <= high; ++cpu) {
    if (!CPU_ISSET(cpu, &mask)) {
      message = "the cpu '" + quotedText(token)
          + "' stands outside the process affinity mask";
      return false;
    }
    if (cpu >= static_cast<std::uint64_t>(online)) {
      message = "the cpu '" + quotedText(token)
          + "' stands past the online cpu count";
      return false;
    }
    cpus.push_back(static_cast<std::uint32_t>(cpu));
  }
  return true;
}

}  // namespace

// The value grammar of contracts/cli-options.md (FR-011, FR-014, R-06):
// the head names a cpu list or the word idle, and the keys follow, each
// standing once in the value, a cpu standing once in any form the
// tokens name. Every rejection names the token it read, quoted the way
// the option module quotes a value, so the caller's one line carries
// it (FR-038).
auto parseJitterProbeValue(const std::string& value,
                           JitterProbeOptions& parsed,
                           std::string& message) -> bool
{
  parsed = JitterProbeOptions {};
  std::set<std::string> seenKeys;
  bool idleHead = false;
  bool headSeen = false;
  std::size_t position = 0;
  while (position <= value.size()) {
    const auto comma = value.find(',', position);
    const std::string token = value.substr(
        position,
        comma == std::string::npos ? std::string::npos : comma - position);
    const bool last = comma == std::string::npos;
    position = last ? value.size() + 1 : comma + 1;

    if (!headSeen) {
      headSeen = true;
      if (token == "idle") {
        idleHead = true;
        continue;
      }
      std::vector<std::uint32_t> headCpus;
      if (parseProbeCpuToken(token, headCpus, message)) {
        parsed.cpus = headCpus;
        continue;
      }
      // parseProbeCpuToken names the token it read in every refusal it
      // answers (FR-038), so the head refusal carries that message.
      return false;
    }

    // A token after the head is a key when it carries '=' or names the
    // value-less `strict`; every other token extends the cpu list.
    const bool keyToken =
        idleHead || token.find('=') != std::string::npos || token == "strict";
    if (!keyToken) {
      if (!parseProbeCpuToken(token, parsed.cpus, message)) {
        return false;
      }
      continue;
    }

    const auto equals = token.find('=');
    const std::string key =
        equals == std::string::npos ? token : token.substr(0, equals);
    const std::string keyArgument =
        equals == std::string::npos ? "" : token.substr(equals + 1);
    if (!seenKeys.insert(key).second) {
      message = "the key '" + quotedText(key) + "' stands twice in the value";
      return false;
    }
    if (key == "threshold") {
      std::uint64_t number = 0;
      if (!parseProbeNumber(keyArgument, number, false)) {
        message = "the threshold '" + quotedText(keyArgument)
            + "' is not a positive number";
        return false;
      }
      parsed.thresholdNs = number;
    } else if (key == "gaps") {
      std::uint64_t number = 0;
      if (!parseProbeNumber(keyArgument, number, false)
          || number > kMaxGapCount)
      {
        message = "the gaps '" + quotedText(keyArgument)
            + "' is not a number between 1 and 16777216";
        return false;
      }
      parsed.gapCount = static_cast<std::uint32_t>(number);
    } else if (key == "mode") {
      if (keyArgument == "alongside") {
        parsed.mode = JitterProbeMode::ALONGSIDE;
      } else if (keyArgument == "precheck") {
        parsed.mode = JitterProbeMode::PRECHECK;
      } else if (keyArgument == "both") {
        parsed.mode = JitterProbeMode::BOTH;
      } else {
        message = "the mode '" + quotedText(keyArgument)
            + "' is not alongside, precheck or both";
        return false;
      }
    } else if (key == "precheck") {
      std::uint64_t number = 0;
      if (!parseProbeNumber(keyArgument, number, false)
          || number > std::numeric_limits<std::uint32_t>::max())
      {
        message = "the precheck '" + quotedText(keyArgument)
            + "' is not a positive number";
        return false;
      }
      parsed.precheckMs = static_cast<std::uint32_t>(number);
    } else if (key == "strict") {
      if (!keyArgument.empty()) {
        message = "the key 'strict' takes no value";
        return false;
      }
      parsed.strict = true;
    } else {
      message = "the key '" + quotedText(key) + "' is not one the value names";
      return false;
    }
  }

  // A cpu that stands twice in one value, in any form the tokens name,
  // is an invalid value (FR-011); the whole value stands named beside
  // the index that repeated (FR-038).
  std::set<std::uint32_t> seenCpus;
  for (const auto cpu : parsed.cpus) {
    if (!seenCpus.insert(cpu).second) {
      message = "the cpu '" + std::to_string(cpu)
          + "' stands more than once in '" + quotedText(value) + "'";
      return false;
    }
  }
  return true;
}

}  // namespace sg::detail
