// ============================================================================
// The jitter probe work item and its availability check (T009, US1).
//
// One translation unit owns the probe (R-01): the holder's lifecycle, the
// per-cpu work item the probe contexts run, the availability check over
// the provider catalog and `ClockProvider::open` (Q-1), the run-level
// fold, and the reader-install seam storage. The window-sided fold lands
// with T024 (US3); the declaration stands in the seam header.
// ============================================================================

#include "detail/jitter_probe.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "detail/percentiles.hpp"
#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace sg::detail {
namespace {

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

// The catalog half of the availability check (Q-1): the machine object
// publishes the tsc leaf, countable, at the read mode the probe opens
// through (R-02).
auto catalogCarriesTsc() -> bool
{
  const auto machine = counters::System::local().object("machine");
  if (!machine.has_value()) {
    return false;
  }
  for (const auto& entry : machine.value().counters()) {
    if (entry.name == "tsc"
        && entry.mode == counters::ReadMode::FAST_TSC
        && entry.avail == counters::Availability::COUNTABLE) {
      return true;
    }
  }
  return false;
}

// The open half of the availability check (Q-1): the null-reader result
// of `ClockProvider::open`. The false arm follows a build that executes
// no time-stamp instruction while a provider still publishes the leaf,
// the way the provider's own refusal follows one, so the coverage
// exclusion names that build and not this host.
auto clockOpenCarriesReader() -> bool
{
  counters::ClockProvider provider;
  const auto window = provider.open(
      counters::LeafSet {.addresses = {"machine/tsc"}}, counters::Target {});
  // LCOV_EXCL_BR_START : coverage exclusion (T034): the null-reader arm
  // needs a build without the time-stamp instruction; every build that
  // publishes the leaf through the shipped provider answers a reader
  // here (Q-1).
  return window != nullptr;  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
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

// The state one live holder's start owns: the seam reader, the barrier,
// the work nodes and the probe contexts. The contexts stand declared
// last, so their destruction runs first: a node dies behind the join
// edge of the context that settled it (FR-012, R-04). One holder holds
// the process's session at a time (R-01).
struct ProbeSession
{
  std::unique_ptr<counters::WindowReader> reader;
  std::optional<ContextBarrier> barrier;
  std::vector<std::unique_ptr<sg::WorkNode<ProbeItem>>> nodes;
  std::vector<std::unique_ptr<sg::ExecutionContext>> contexts;
};

auto ProbeItem::operator()(sg::WorkContext& /*work*/) noexcept -> void
{
    // The buffer allocates and first touches on this probe cpu, after
    // the pin the context applied at thread start and before the first
    // read, so the first-touch faults land on the probe cpu and outside
    // every measured window (FR-009, R-03).
    state->buffer.assign(options->gapCount, GapRecord {});

    // The read move: the installed reader's thunk over a fresh stack
    // point sink per pass, the way the plan's own sampling action
    // builds one, or the one-leaf plan's recorder through the system
    // otherwise (R-02, Q-1).
    std::uint64_t cell = 0;
    counters::WindowReader* reader = session->reader.get();
    counters::WindowReader::ReadThunk thunk = nullptr;
    std::optional<counters::Plan> plan;
    std::optional<counters::RecorderHandle<counters::Ring>> recorder;
    if (reader != nullptr) {
      thunk = reader->resolveThunk();
    } else {
      auto& sys = counters::System::local();
      const auto machine = sys.object("machine");
      if (!machine.has_value()) {
        return;
      }
      const auto leaf = machine.value().counter<counters::Dim<0, 1>>("tsc");
      if (!leaf.has_value()) {
        return;
      }
      counters::Expression<counters::Dim<0, 1>> expression(leaf.value());
      auto compiled = counters::detail::compileCore(
          sys, counters::Target {}, {&expression.core});
      if (!compiled.has_value()) {
        return;
      }
      plan.emplace(std::move(compiled.value()));
      auto minted = plan->recorder(2, counters::ring);
      if (!minted.has_value()) {
        return;
      }
      recorder.emplace(std::move(minted.value()));
    }
    const auto readStamp = [&]() noexcept -> std::uint64_t {
      if (thunk != nullptr) {
        counters::PointSink sink(&cell, 1, 1, 1, 0);
        thunk(*reader, sink);
        return cell;
      }
      recorder->sample();
      return recorder->mColumns[(recorder->mHead - 1)
                                & (recorder->mCapacity - 1)];
    };

    // The measurement pass: the 1,024 reads take no record and set the
    // effective threshold, the default rising to ten times the minimum
    // period the pass measured when that stands larger (E-05, FR-007,
    // D-2, R-05).
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
    state->thresholdNs =
        options->thresholdNs == kDefaultThresholdNs
            ? (risen > kDefaultThresholdNs ? risen : kDefaultThresholdNs)
            : options->thresholdNs;

    // The pre-check phase: every chosen cpu arrives at the barrier the
    // holder owns, so all of them spin together for the precheck
    // duration, measured through the same reader in the leaf's one time
    // base (FR-014, R-10, R-04).
    if (options->mode == JitterProbeMode::PRECHECK
        || options->mode == JitterProbeMode::BOTH) {
      session->barrier->arriveAndWait();
      const std::uint64_t span = static_cast<std::uint64_t>(options->precheckMs)
                                 * 1000000ULL;
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

    // The alongside phase: the loop spans the batch, from the first
    // read after the phases to the read of the stop flag, one reader
    // call, one comparison, one record store and one stop-flag read per
    // pass, and no allocation, no syscall, no I/O anywhere in the body
    // (FR-005, FR-006, D-7, R-04).
    if (options->mode == JitterProbeMode::ALONGSIDE
        || options->mode == JitterProbeMode::BOTH) {
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
// holds over the small kept sets too.
auto percentileField(const std::vector<double>& sorted,
                     const double point) -> std::uint64_t
{
  const auto ranked = percentileAt(sorted, point);
  if (ranked.available) {
    return static_cast<std::uint64_t>(ranked.value);
  }
  if (sorted.empty()) {
    return 0;
  }
  const auto rank = std::ceil(point * static_cast<double>(sorted.size())
                              / 100.0);
  const auto index = static_cast<std::size_t>(
      rank < 1.0 ? 1.0 : (rank > static_cast<double>(sorted.size())
                              ? static_cast<double>(sorted.size())
                              : rank));
  return static_cast<std::uint64_t>(sorted.at(index - 1));
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
  // machine/tsc leaf, and the null-reader result of ClockProvider::open.
  bool available = catalogCarriesTsc();
  if (available) {
    available = clockOpenCarriesReader();
  }

  // The availability gate stands here; the idle resolution and the
  // overlap check ran in the entry point before this start (R-07), so
  // an empty list arrives only when no probe stands to run.
  if (!available || m_options.cpus.empty()) {
    return;
  }

  auto session = std::make_unique<ProbeSession>();

  // The states build once, in place: the work items reference them for
  // the whole run, so the vector never grows after this line (E-05).
  m_states = std::vector<ProbeCpuState>(m_options.cpus.size());
  for (std::size_t index = 0; index < m_options.cpus.size(); ++index) {
    m_states.at(index).cpu = m_options.cpus.at(index);
  }

  if (m_options.mode == JitterProbeMode::PRECHECK
      || m_options.mode == JitterProbeMode::BOTH) {
    session->barrier.emplace(static_cast<int>(m_options.cpus.size()));
  }

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

  probeSession = std::move(session);
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

auto JitterProbe::states() const noexcept
    -> const std::vector<ProbeCpuState>&
{
  return m_states;
}

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
  stats.meanNs = static_cast<double>(total)
                 / static_cast<double>(sorted.size());
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
  return stats;
}

auto installTestReader(std::unique_ptr<counters::WindowReader> reader) -> void
{
  installedReader = std::move(reader);
}

}  // namespace sg::detail
