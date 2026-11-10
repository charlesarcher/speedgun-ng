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
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "detail/jitter_probe.hpp"

#include <sched.h>

#include "../detail/proc_root.hpp"
#include "detail/internal.hpp"
#include "detail/latency_counters.hpp"
#include "detail/options.hpp"
#include "detail/percentiles.hpp"
#include "detail/spectrum.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace sg::detail
{

// specs/034 R-03: the whole-file reader under the file-root seam. The
// definition stands below, beside the attribution readers it serves, so the
// window-edge reads of specs/048 name it here (FR-019, P-2).
auto readUnderRoot(const std::string& relative) -> std::optional<std::string>;

namespace
{

// The measurement pass's read count stands in detail/jitter_probe.hpp,
// beside the probe the pass belongs to, so the test that drives the
// gates spells the same value (E-05, R-05).

// The rise the default threshold takes at ten times the measured minimum
// loop period (FR-007, D-2).
inline constexpr std::uint64_t kRiseFactor = 10;

// The turn wait's pause hint: the same documented shape the shipped
// pause helper of execution_context.cpp carries (specs/034 R-06), with
// the same P2 justification at its site: the operand is absent, so
// nothing is aliased, and the `memory` clobber stands the compiler
// barrier. The AArch64 spelling matches the shipped helper and is
// inert on x86-64, the only CI architecture this file is verified on.
inline auto pauseHint() -> void
{
#if defined(__x86_64__) || defined(__i386__)
  asm volatile(
      "pause" ::
          : "memory");  // NOLINT(hicpp-no-assembler, portability-no-assembler)
#elifdef __aarch64__
  asm volatile(
      "yield" ::
          : "memory");  // NOLINT(hicpp-no-assembler, portability-no-assembler)
#endif
}

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

// specs/048 FR-012, FR-013, P-2, E-03: the SMI counter source of one probe
// cpu over one window. The item opens the node once, outside every window,
// through the unit that owns the kernel interface, and closes it at stop.
// `before` holds the read the next gap read stands against: the opening edge
// read for the window's first gap, and the previous kept gap's read for every
// later gap (FR-012, D-7). A source the vendor gate refuses, a node the open
// refuses and a read that answers nothing each leave the figures with no
// value, and no figure stands set in their place (FR-010, FR-011, FR-015).
struct SmiSource
{
  std::optional<int> descriptor {};
  std::optional<std::uint64_t> before {};
};

// The one read of the open node, or nothing when the descriptor stands absent
// or the kernel answers less than the counter's width (FR-007, FR-011).
auto counterFigure(const SmiSource& source) noexcept
    -> std::optional<std::uint64_t>
{
  if (!source.descriptor.has_value()) {
    return std::nullopt;
  }
  return smiCountOf(*source.descriptor);
}

// The counter read of one window edge: the figure, and the base the first gap
// read of that window stands against (FR-012, D-7).
auto edgeCounter(SmiSource& source) noexcept -> std::optional<std::uint64_t>
{
  const auto figure = counterFigure(source);
  source.before = figure;
  return figure;
}

// The non-maskable-interrupt figure of one cpu, read through the standing
// file-root reader and the unit's own text reader (FR-009). The read stands at
// a window edge, outside the measured loop (P-2).
auto nmiFigureOf(const std::uint32_t cpu) noexcept
    -> std::optional<std::uint64_t>
{
  const auto text = readUnderRoot("proc/interrupts");
  if (!text.has_value()) {
    return std::nullopt;
  }
  return nmiCountOfText(*text, cpu);
}

// The two reads that open one window, one per source (FR-012, FR-016). The
// counter read also sets the base of the window's first gap read (D-7).
auto openWindowEdges(ProbeCpuState& state, SmiSource& source) noexcept -> void
{
  state.edges = WindowEdges {};
  state.edges.smiOpening = edgeCounter(source);
  state.edges.nmiOpening = nmiFigureOf(state.cpu);
}

// The two reads that close one window, one per source (FR-012, FR-016). The
// closing read leaves the base where it stands, because no gap of that window
// reads after it (D-7).
auto closeWindowEdges(ProbeCpuState& state,
                      const SmiSource& source) noexcept -> void
{
  state.edges.smiClosing = counterFigure(source);
  state.edges.nmiClosing = nmiFigureOf(state.cpu);
}

// specs/048 FR-013, FR-014, FR-015, P-3: the change element of one kept gap:
// the element the keep step paired with its record, and zero for a gap the
// change ring does not reach.
auto changeAt(std::span<const std::uint64_t> changes,
              const std::size_t index) noexcept -> std::uint64_t
{
  if (index >= changes.size()) {
    return 0;
  }
  return *(changes.begin() + static_cast<std::ptrdiff_t>(index));
}

// The change of one pair of window-edge reads. The pair answers nothing when
// either read stands absent, and answers zero when the closing read stands at
// or below the opening read, the floor FR-016 validates (FR-016).
auto windowDeltaOf(const std::optional<std::uint64_t>& opening,
                   const std::optional<std::uint64_t>& closing) noexcept
    -> std::optional<std::uint64_t>
{
  if (!opening.has_value() || !closing.has_value()) {
    return std::nullopt;
  }
  return *closing > *opening ? *closing - *opening : std::uint64_t {0};
}

// One record store into the pre-sized buffer, or the drop the filled
// buffer answers with: the stamp is lost and the count, the total and
// the max of the dropped gaps stand in the per-cpu counters (E-04,
// FR-010, D-6). The counter read stands inside the keep step, so a gap the
// buffer drops costs no read and the next kept gap's span covers it (FR-012,
// E-03). The change answers this read over the read before it, and a read that
// answers nothing leaves the gap unmarked and the base where it stands, so the
// next gap's span covers the missing read too (FR-013, FR-014, FR-015).
auto keepOrDrop(ProbeCpuState& state,
                SmiSource& source,
                const std::uint64_t stamp,
                const std::uint64_t gap) noexcept -> void
{
  if (state.kept < state.buffer.size()) {
    const auto before = source.before;
    const auto figure = counterFigure(source);
    if (figure.has_value()) {
      source.before = figure;
    }
    const auto change =
        figure.has_value() && before.has_value() && *figure > *before
        ? *figure - *before
        : std::uint64_t {0};
    SG_ASSERT(state.changes.size() > state.kept,
              "the change ring stands as long as the gap ring (FR-012, E-03)");
    state.buffer.at(state.kept) = GapRecord {.stamp = stamp, .gapNs = gap};
    state.changes.at(state.kept) = change;
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

// The tick rate the run's projection scales by (specs/045-merge-fixes
// The tick rate the run's projection scales by (specs/045-merge-fixes
// phase 16, FR-039): the machine/tsc and machine/monotonic leaves read
// in pairs over the bounded spin, the ratio answered as
// deltaTicks * 1000000000 / deltaNs in Hz. The spin's bounds stand at
// the one million ns stop and the 100,000 pair cap, no sleep and no
// clock call anywhere: the leaves are the only time sources the spin
// touches. The measurement stands best-effort by decision: a catalog
// that resolves no monotonic leaf, or a pair the spin cannot bound,
// answers 0, the identity the projection stands at, and the record
// names it. The leaves stand resolved before any window opens, so a
// fixture whose tsc leaf scripts a point sequence stands unconsumed.
auto measureTickRate() -> std::uint64_t
{
  constexpr std::uint64_t kNsBound = 1000000;
  constexpr std::uint64_t kPairCap = 100000;
  auto& sys = counters::System::local();
  // The machine object and the tsc leaf stand read unchecked: the
  // availability gate the start ran owns both facts, the way
  // `catalogCarriesTsc` reads them (Q-1). The mono leaf stands checked:
  // the gate names the tsc leaf alone, and a catalog without the mono
  // leaf is the mono-less fixture case the rate answers 0 on (FR-039).
  const auto machine = sys.object("machine");
  const auto tscLeaf = machine.value().counter<counters::Dim<0, 1>>("tsc");
  const auto nsLeaf = machine.value().counter<counters::Dim<1, 0>>("monotonic");
  if (!nsLeaf.has_value()) {
    return 0;
  }
  counters::Expression<counters::Dim<0, 1>> tscExpr(tscLeaf.value());
  counters::Expression<counters::Dim<1, 0>> nsExpr(nsLeaf.value());
  auto compiled = counters::detail::compileCore(
      sys, counters::Target {}, {&tscExpr.core, &nsExpr.core});
  if (!compiled.has_value()) {
    return 0;
  }
  auto plan = std::move(compiled.value());
  // The recorder holds the spin's pairs without wrapping: the
  // hard-stop capacity covers the cap's pairs exactly, so the view's
  // points stand in sample order and the newest read sits at the index
  // the pair count names.
  auto recorder = plan.recorder(kPairCap + 1);
  recorder.sample();
  // The views resolve once: the recorder holds the spin's pairs without
  // wrapping, so the points stand in sample order and the newest read
  // sits at the index the pair count names, the shape the factor
  // measurement at the branch base took.
  // The plan bound the pair's leaves at the compile above, so the raw
  // views answer and stand read unchecked.
  const auto tscView = tscExpr.raw(recorder.view(), "machine", "tsc").value();
  const auto nsView = nsExpr.raw(recorder.view(), "machine", "monotonic").value();
  const std::uint64_t* tscPoints = tscView.points;
  const std::uint64_t* nsPoints = nsView.points;
  const std::uint64_t tscBase = tscPoints[0];
  const std::uint64_t nsBase = nsPoints[0];
  std::uint64_t pairs = 1;
  std::uint64_t deltaNs = 0;
  std::uint64_t deltaTicks = 0;
  while (pairs < kPairCap) {
    recorder.sample();
    ++pairs;
    if (nsPoints[pairs - 1] - nsBase >= kNsBound) {
      deltaNs = nsPoints[pairs - 1] - nsBase;
      deltaTicks = tscPoints[pairs - 1] - tscBase;
      break;
    }
  }
  if (deltaNs == 0) {
    return 0;
  }
  return deltaTicks * 1000000000ULL / deltaNs;
}

struct ProbeSession;

// specs/043 FR-010, FR-011, D-2, R-04: the calibration of one probe cpu.
// The search runs on the probe cpu, after the buffer's first touch and
// before the measurement pass, so its span stands out of the window's
// runtime and the latch line, hence `start()`, stands behind it (R-05).
// The doubling starts at one loop body and runs at most 25 trials over
// counts 1 through `kMaxWorkCount`; the bisection runs at most 24 steps;
// each trial reads the time source twice around a body closed on
// `sg::doNotOptimize`, so the search costs at most 98 reads. The search
// ends at the trial cap, at the count cap, or when the bisection
// interval holds one count, and the closest count it measured stands
// kept. The target stands at `kTargetUnitTimeNs` for FWQ and at one
// thousandth of the quantum for FTQ, the quantum in ns standing
// `quantumUs` x 1,000.

// The calibrated loop body: `count` passes closed on `sg::doNotOptimize`
// (FR-010, FR-012, FR-015, D-2). The search times this body and the
// window runs it, so the unit the calibration measured is the unit the
// window times and counts.
//
// \pre `count` stands at or above 1
auto runUnitBody(const std::uint64_t count) noexcept -> void
{
  std::uint64_t work = 0;
  for (std::uint64_t index = 0; index < count; ++index) {
    sg::doNotOptimize(work);
  }
  sg::doNotOptimize(work);
}

// The running state of one calibration search (FR-010, E-22).
struct CalibrationSearch
{
  std::uint64_t target = kTargetUnitTimeNs;
  std::uint64_t spanStart = 0;
  std::uint64_t spanEnd = 0;
  std::uint64_t bestCount = 1;
  std::uint64_t bestUnit = 0;
  bool haveBest = false;
};

// The distance of a measured unit time to the target, the pair the
// search compares to pick the closest count (FR-010).
auto unitDistance(const std::uint64_t unit,
                  const std::uint64_t target) noexcept -> std::uint64_t
{
  return unit > target ? unit - target : target - unit;
}

// One trial of the search: two reads around `count` loop bodies closed
// on `sg::doNotOptimize`, the unit time standing at the delta of the two
// reads, the whole body run the window times and the target names
// (FR-010, D-2). The closest unit time to the target wins, a tie keeping
// the earlier trial.
//
// \pre `readStamp` reads the probe's own time source and `count` stands
//      at or above 1
// \post the answer carries the measured unit time and the search keeps
//       the closest count it has measured
template<class ReadStamp>
auto calibrationTrial(CalibrationSearch& search,
                      const ReadStamp& readStamp,
                      const std::uint64_t count) noexcept -> std::uint64_t
{
  const std::uint64_t first = readStamp();
  if (!search.haveBest) {
    search.spanStart = first;
  }
  runUnitBody(count);
  search.spanEnd = readStamp();
  const std::uint64_t unit = search.spanEnd - first;
  const std::uint64_t target = search.target;
  const std::uint64_t distance = unitDistance(unit, target);
  const std::uint64_t bestDistance = unitDistance(search.bestUnit, target);
  if (!search.haveBest || distance < bestDistance) {
    search.haveBest = true;
    search.bestUnit = unit;
    search.bestCount = count;
  }
  return unit;
}

// The search itself: the doubling, then the bisection over the bracket
// the doubling left (FR-010, FR-011, R-04).
//
// \pre `readStamp` reads the probe's own time source
// \post the state carries the kept count, its measured unit time, the
//       target, the quantum for FTQ, and the span of the search's reads
//       (FR-011, E-22)
// \post a count the value names stands kept with a span of 0, its unit
//       time standing for the first unit of this cpu's window to fill
//       (FR-002, FR-010, FR-011, D-2)
template<class ReadStamp>
auto calibrateUnit(ProbeCpuState& state,
                   const JitterProbeOptions& options,
                   const ReadStamp& readStamp) noexcept -> void
{
  CalibrationSearch search;
  search.target = options.method == JitterMethod::FTQ
      ? static_cast<std::uint64_t>(options.quantumUs)
      : kTargetUnitTimeNs;
  if (options.workCount != 0) {
    // FR-002, FR-010, D-2: a `work=` count the value names stands as the
    // unit count, and the search does not run for this cpu. The span of
    // the reads it never made stands at 0, and the unit time the run
    // discloses reads the first unit of this cpu's window, so the window
    // fills it (FR-011).
    search.bestCount = options.workCount;
    state.unitTimeFromWindow = true;
  } else {
    std::uint64_t lo = 1;
    std::uint64_t hi = kMaxWorkCount;
    std::uint64_t count = 1;
    // The doubling reads counts 1, 2, 4 through kMaxWorkCount, the
    // `kCalibrationTrials` trials the search allows, and ends at the first
    // count whose unit time passes the target, bracketing it by the count
    // below (FR-010, R-02).
    while (true) {
      const std::uint64_t unit = calibrationTrial(search, readStamp, count);
      if (unit >= search.target) {
        hi = count;
        lo = count / 2;
        break;
      }
      // The count cap ends the doubling with no count above the target, so
      // the bisection reads the whole range and runs its full
      // `kCalibrationSteps` steps (FR-010, R-02).
      if (count == kMaxWorkCount) {
        lo = 1;
        break;
      }
      count *= 2;
    }
    // The bisection halves the bracket the doubling left: an interval of
    // `kMaxWorkCount` halves `kCalibrationSteps` times and ends holding one
    // count (FR-010).
    while (hi - lo > 1) {
      const std::uint64_t mid = lo + (hi - lo) / 2;
      const std::uint64_t unit = calibrationTrial(search, readStamp, mid);
      if (unit >= search.target) {
        hi = mid;
      } else {
        lo = mid;
      }
    }
  }
  state.workCount = search.bestCount;
  state.unitTimeNs = search.bestUnit;
  state.targetNs = search.target;
  state.quantumNs = options.method == JitterMethod::FTQ
      ? static_cast<std::uint64_t>(options.quantumUs) * 1000ULL
      : 0;
  state.calibrationSpanNs = search.spanEnd - search.spanStart;
}

// The per-cpu work item: the buffer's first touch, the calibration, the
// measurement pass, the phases, and the four-move loop (FR-005, FR-009,
// R-03, R-04).
struct ProbeItem
{
  ProbeCpuState* state = nullptr;
  const JitterProbeOptions* options = nullptr;
  ProbeSession* session = nullptr;
  // specs/034 R-06: the item's index in the states order, the order
  // the serial windows run in (the planned cpus ascending, then the
  // candidates ascending, the order the runner builds the options in).
  std::size_t index = 0;

  auto operator()(sg::WorkContext& /*work*/) noexcept -> void;
};

// One series sample into the pre-sized buffer, or the drop the filled
// buffer answers with: the sample is lost and the count of records the
// full buffer dropped stands in the per-cpu counters (FR-014, FR-016,
// FR-022, E-26, D-6).
auto keepSeriesOrDrop(ProbeCpuState& state,
                      const std::uint64_t sample,
                      const std::uint64_t phaseStart) noexcept -> void
{
  if (state.seriesKept < state.series.size()) {
    // specs/043 FR-020, R-11, E-26: the first store writes the phase's
    // first stamp, the base the window fold recovers every sample's
    // stamp from, and a later store leaves it standing.
    if (state.seriesKept == 0) {
      state.seriesStartNs = phaseStart;
    }
    state.series.at(state.seriesKept) = sample;
    ++state.seriesKept;
    return;
  }
  ++state.droppedSamples;
}

// One FWQ unit: the calibrated body, one read, and the unit time the
// delta between two consecutive reads answers (FR-012, FR-014, R-08).
// The body, the read, the one comparison and the one store of the
// record stand the whole unit, and the unit allocates nothing, reads no
// file, takes no lock and reads no clock outside the counters
// (FR-013, R-08).
//
// \pre `readStamp` reads the probe's own time source and the state
//      carries the count the calibration kept
template<class ReadStamp>
auto runFwqUnit(ProbeCpuState& state,
                const ReadStamp& readStamp,
                std::uint64_t& last,
                const std::uint64_t phaseStart) noexcept -> void
{
  runUnitBody(state.workCount);
  const std::uint64_t stamp = readStamp();
  const std::uint64_t unit = stamp - last;
  // FR-011, D-2: a run whose count the value names reads no search, so
  // the unit time it discloses reads this window's first unit.
  if (state.unitTimeFromWindow) {
    state.unitTimeNs = unit;
    state.unitTimeFromWindow = false;
  }
  keepSeriesOrDrop(state, unit, phaseStart);
  last = stamp;
}

// One FTQ unit: the calibrated body, one read, and the count of units
// since the last edge. The count stands incremented before the edge
// test, and at an edge the pass stores the count, resets it and
// advances the edge by one quantum, so a clock that skips quanta
// records the zero counts it skipped and a trailing open quantum
// records no count (FR-015, FR-016, D-4, R-07).
//
// \pre `readStamp` reads the probe's own time source, `edge` stands at
//      the next quantum boundary and `count` at the units since the
//      last one
template<class ReadStamp>
auto runFtqUnit(ProbeCpuState& state,
                const ReadStamp& readStamp,
                std::uint64_t& last,
                std::uint64_t& edge,
                std::uint64_t& count,
                const std::uint64_t phaseStart) noexcept -> void
{
  runUnitBody(state.workCount);
  const std::uint64_t stamp = readStamp();
  last = stamp;
  ++count;
  while (stamp >= edge) {
    keepSeriesOrDrop(state, count, phaseStart);
    count = 0;
    edge += state.quantumNs;
  }
}

// The latch-time snapshot: the prefix the pre-check phase kept, the
// counts as they stand at the latch line, and the calibration figures
// the search measured. The item calls this once per cpu at the latch
// line, off the hot path, and the release store publishes the copy to
// the mid-run fold sites, which read one object per cpu while the
// alongside loop runs on (specs/045-merge-fixes FR-008, D-5, R-04). Each
// method fills one prefix: a `tsc` run keeps no series, and the FWQ and
// FTQ loops keep no gaps, so the other prefix stands empty on its own.
auto publishLatchSnapshot(ProbeCpuState& state) noexcept -> void
{
  auto& latch = state.latch;
  latch.method = state.method;
  latch.workCount = state.workCount;
  latch.unitTimeNs = state.unitTimeNs;
  latch.targetNs = state.targetNs;
  latch.quantumNs = state.quantumNs;
  latch.calibrationSpanNs = state.calibrationSpanNs;
  latch.thresholdNs = state.thresholdNs;
  latch.loopPeriodNs = state.loopPeriodNs;
  const std::span<const std::uint64_t> samples(state.series.data(),
                                               state.seriesKept);
  latch.series.assign(samples.begin(), samples.end());
  latch.seriesKept = state.seriesKept;
  latch.droppedSamples = state.droppedSamples;
  const std::span<const GapRecord> gaps(state.buffer.data(), state.kept);
  latch.gaps.assign(gaps.begin(), gaps.end());
  latch.gapsKept = state.kept;
  latch.droppedGaps = state.droppedGaps;
  // specs/048 FR-012, FR-016, E-03: the change prefix rides beside the
  // gap prefix, and the edge pair of the pre-check window rides with it,
  // so the record folds the window it copied and reads no live buffer.
  const std::span<const std::uint64_t> changes(state.changes.data(),
                                               state.kept);
  latch.changes.assign(changes.begin(), changes.end());
  latch.window = state.edges;
  state.latchPublished.store(true, std::memory_order_release);
}

// The state one live holder's start owns: the seam reader, the start
// barrier, the latch, the work nodes and the probe contexts. The
// contexts stand declared last, so their destruction runs first: a node
// dies behind the join edge of the context that settled it (FR-012,
// R-04). One holder holds the process's session at a time (R-01).
struct ProbeSession
{
  std::unique_ptr<counters::WindowReader> reader;
  // specs/045-merge-fixes phase 16 (FR-039): the tick rate the run's
  // projection scales by, measured once at the start before the items
  // read, the identity 0 stands at when the catalog answers no rate
  // pair.
  std::uint64_t rateHz = 0;
  std::optional<ContextBarrier> barrier;
  // The second synchronizer between the holder and the items: the round
  // the items reach after the buffer's first touch, the measurement
  // pass and, in the precheck and both modes, the pre-check phase, and
  // the holder reaches before start() returns, so the batch's first
  // benchmark begins behind them (FR-006, FR-009, FR-014, R-04).
  std::optional<ContextBarrier> latch;
  // specs/034 R-06: the serial windows' turn word. The item whose
  // index the turn names runs its window, and the release stands the
  // next item's start; the wait spins on the pause hint, outside every
  // timed window and outside every record (FR-009, R-06).
  std::atomic<std::uint64_t> serialTurn {0};
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
  // specs/048 FR-012, P-3, E-03: the change ring pre-sizes with the gap
  // ring, one 8-byte count per gap the run can keep, so the gap path
  // stores record and change into standing space (D-6, R-08).
  state->changes.assign(options->gapCount, 0);
  // The series buffer of the two quanta methods pre-sizes from the same
  // gaps key, one 8-byte sample per entry, so the measured loop stores
  // into standing space and allocates nothing (FR-014, FR-016, E-26,
  // D-6, R-08).
  if (fillsSeriesRing(options->method)) {
    state->series.assign(options->gapCount, 0);
  }

  // specs/048 D-1, FR-012, FR-013, P-2: the `hwlat` method carries the two
  // kernel counters, and a run of another method reads neither. The node
  // opens once at start, ahead of the measurement pass and of every window,
  // and closes at the end of the item. The unit's vendor gate reads the
  // vendor word once here, so a host the gate refuses opens no node, reads no
  // counter and names no substitute (FR-010, FR-011, D-4).
  SmiSource source;
  const bool readsKernelCounters = options->method == JitterMethod::HWLAT;
  if (readsKernelCounters && smiSourceSupported()) {
    source.descriptor = openMsrNode(state->cpu);
  }

  // The read move: the installed reader's thunk over a fresh stack
  // point sink per pass, the way the plan's own sampling action
  // builds one, or the one-leaf plan's recorder through the system
  // otherwise (R-02, Q-1). The system open asserts the availability
  // gate's answer, so the item always reaches the latch line below
  // (FR-006, R-04).
  std::uint64_t cell = 0;
  // specs/045-merge-fixes phase 16 (FR-039): the read move projects
  // each raw read into the run's nanosecond base at the rate the start
  // measured, so every figure, threshold and span downstream keeps the
  // ns reading its contract states.
  TickProjector projector {.rateHz = session->rateHz};
  counters::WindowReader* reader = session->reader.get();
  counters::WindowReader::ReadThunk thunk = nullptr;
  std::optional<counters::Plan> plan;
  std::optional<counters::RecorderHandle<counters::Ring>> recorder;
  if (reader != nullptr) {
    thunk = reader->resolveThunk();
  } else {
    openSystemRecorder(plan, recorder);
  }
  bool seedCaptured = false;
  const auto readStamp = [&]() noexcept -> std::uint64_t
  {
    const auto raw = [&]() noexcept -> std::uint64_t
    {
      if (thunk != nullptr) {
        counters::PointSink sink(&cell, 1, 1, 1, 0);
        thunk(*reader, sink);
        return cell;
      }
      recorder->sample();
      return recorder
          ->mColumns[(recorder->mHead - 1) & (recorder->mCapacity - 1)];
    }();
    const auto projected = projector.project(raw);
    if (!seedCaptured) {
      seedCaptured = true;
      state->firstTickRaw = raw;
    }
    return projected;
  };

  // The calibration: FWQ and FTQ find their work count on this cpu,
  // after the buffer's first touch and before the measurement pass, so
  // the search's span stands out of the window's runtime and the latch
  // line stands behind it (FR-010, FR-011, R-04, R-05). A run of a method
  // that fills the gap ring keeps the standing layout and reads no
  // prefix (D-1, A-4).
  if (fillsSeriesRing(options->method)) {
    calibrateUnit(*state, *options, readStamp);
  }

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
  state->tickRateHz = session->rateHz;
  const std::uint64_t risen = kRiseFactor * minDelta;
  state->thresholdNs = options->thresholdNs.has_value()
      ? *options->thresholdNs
      : (risen > kDefaultThresholdNs ? risen : kDefaultThresholdNs);

  // The pre-check phase: the cpus stand lined at the start barrier,
  // so all of them spin together for the precheck duration, measured
  // through the same reader in the leaf's one time base (FR-014, R-10,
  // R-04). The serial shape runs one measured cpu per window: the item
  // spins on the turn word until its index, then re-reads its baseline
  // stamp, so the wait names no gap, and the release stands the next
  // item's start (specs/034 FR-009, R-06).
  if (options->mode == JitterProbeMode::PRECHECK
      || options->mode == JitterProbeMode::BOTH)
  {
    const std::uint64_t span =
        static_cast<std::uint64_t>(options->precheckMs) * 1000000ULL;
    if (options->precheckSerial) {
      while (session->serialTurn.load(std::memory_order_acquire)
             != static_cast<std::uint64_t>(index))
      {
        pauseHint();
      }
      last = readStamp();
    }
    const std::uint64_t phaseStart = last;
    if (readsKernelCounters) {
      // specs/048 FR-012, FR-016, D-7: the pre-check window opens here, so
      // the edge pair reads once, ahead of the phase loop, and sets the base
      // the phase's first gap read stands against.
      openWindowEdges(*state, source);
    }
    if (options->method == JitterMethod::FWQ) {
      // FR-004: the pre-check phase of a FWQ run records unit times,
      // and the phase ends at its span.
      while (true) {
        runFwqUnit(*state, readStamp, last, phaseStart);
        if (last - phaseStart >= span) {
          break;
        }
      }
    } else if (options->method == JitterMethod::FTQ) {
      // FR-004, FR-015, R-07: the pre-check phase of a FTQ run records the
      // count of whole units each quantum holds. Edge k stands at the
      // phase's first stamp plus k quanta, the read compares against the
      // next edge, and the phase ends at its span.
      std::uint64_t edge = phaseStart + state->quantumNs;
      std::uint64_t count = 0;
      while (true) {
        runFtqUnit(*state, readStamp, last, edge, count, phaseStart);
        if (last - phaseStart >= span) {
          break;
        }
      }
    } else {
      while (true) {
        const std::uint64_t stamp = readStamp();
        const std::uint64_t gap = stamp - last;
        if (gap >= state->thresholdNs) {
          keepOrDrop(*state, source, stamp, gap);
        }
        last = stamp;
        if (stamp - phaseStart >= span) {
          break;
        }
      }
    }
    if (readsKernelCounters) {
      // specs/048 FR-012, FR-016: the phase's last read stands behind the
      // loop, so the closing edge of the pre-check window reads here and no
      // later gap of this window reads after it.
      closeWindowEdges(*state, source);
    }
    if (options->precheckSerial) {
      // The release stands the next item's start; the store happens
      // once, before the latch line, so the windows stand one at a
      // time (FR-009, R-06).
      session->serialTurn.store(static_cast<std::uint64_t>(index) + 1,
                                std::memory_order_release);
    }
  }

  // The latch line: the item arrives here after its buffer's first
  // touch, its measurement pass and, in the precheck and both modes,
  // its pre-check phase; the holder waits on the same line, so
  // start() returns only after every item stands past them and the
  // batch's first benchmark begins behind them (FR-006, FR-009,
  // FR-014, R-04).
  session->latch->arriveAndWait();

  // specs/043 FR-004, FR-020, E-27, R-04: the latch line ends the
  // pre-check phase, so the kept count stands stable from here: the
  // snapshot names the prefix the pre-check record of a `mode=both` run
  // folds while the alongside loop below runs on.
  state->seriesKeptAtLatch = state->seriesKept;

  // specs/045-merge-fixes FR-008, D-5: the snapshot hand-off. The copy
  // and the release store stand here, behind the latch line and ahead of
  // the alongside loop, so the mid-run fold sites read one published
  // object per cpu and touch no field the loop writes.
  publishLatchSnapshot(*state);

  // The wait stands outside the record: the alongside phase compares
  // each stamp with the one read here, behind the latch, so the time
  // the item spent waiting on the latch names no gap (FR-006, R-04).
  last = readStamp();

  // The alongside phase: the loop spans the batch, from the first
  // read after the phases to the read of the stop flag, one reader
  // call, one comparison, one record store and one stop-flag read per
  // pass, and no allocation, no syscall, no I/O anywhere in the body
  // (FR-005, FR-006, D-7, R-04). The `hwlat` method adds the counter
  // read of the keep step alone, so a pass that keeps no gap costs no
  // second read and the pair of reads that bound a gap stays adjacent to
  // it (FR-012, P-2, E-03).
  if (options->mode == JitterProbeMode::ALONGSIDE
      || options->mode == JitterProbeMode::BOTH)
  {
    // specs/043 FR-020, R-11, E-26: the read behind the latch stands
    // the window's first stamp, the base the series fold recovers every
    // sample's stamp from.
    const std::uint64_t windowStart = last;
    if (readsKernelCounters) {
      // specs/048 FR-012, FR-016, D-7: the run window opens behind the latch
      // line, so the edge pair reads once, ahead of the loop, and the pair of
      // the pre-check window stands copied into the snapshot below.
      openWindowEdges(*state, source);
    }
    if (options->method == JitterMethod::FWQ) {
      // FR-012, FR-014, R-08: one unit per pass, the calibrated body
      // and one read, the unit time the delta answers, and the record
      // store into the pre-sized series.
      while (!state->stop.load(std::memory_order_relaxed)) {
        runFwqUnit(*state, readStamp, last, windowStart);
      }
    } else if (options->method == JitterMethod::FTQ) {
      // FR-015, FR-016, D-4, R-07: the quantum edges stand from the
      // window's first stamp, the read behind the latch, and one unit
      // per pass names the count of units the next edge holds.
      std::uint64_t edge = last + state->quantumNs;
      std::uint64_t count = 0;
      while (!state->stop.load(std::memory_order_relaxed)) {
        runFtqUnit(*state, readStamp, last, edge, count, windowStart);
      }
    } else {
      while (!state->stop.load(std::memory_order_relaxed)) {
        const std::uint64_t stamp = readStamp();
        const std::uint64_t gap = stamp - last;
        if (gap >= state->thresholdNs) {
          keepOrDrop(*state, source, stamp, gap);
        }
        last = stamp;
      }
    }
    if (readsKernelCounters) {
      // specs/048 FR-012, FR-016: the loop ends at the stop flag, so the
      // closing edge of the run window reads here, once, behind the loop.
      closeWindowEdges(*state, source);
    }
  }

  // specs/048 P-2: the descriptor closes once at stop, and the close's answer
  // stands unread (FR-011).
  if (source.descriptor.has_value()) {
    closeMsrNode(*source.descriptor);
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

// The noise fraction of one kept set, the method's rule of FR-009:
// FWQ divides the kept unit times' sum, minus the sample count times
// the minimum unit time, by that sum, times 100, and FTQ divides the
// sample count times the maximum count, minus the counts' sum, by that
// product, times 100. Each denominator that stands at 0 answers its
// own figure: a maximum count of 0 answers 100, and a sum of zero unit
// times answers 0, the equal units carrying no noise (FR-009, R-10).
auto seriesNoisePercent(const JitterMethod method,
                        const std::uint64_t sampleCount,
                        const std::uint64_t minValue,
                        const std::uint64_t maxValue,
                        const std::uint64_t total) noexcept -> double
{
  if (method == JitterMethod::FTQ) {
    const std::uint64_t span = sampleCount * maxValue;
    if (span == 0) {
      return 100.0;
    }
    return static_cast<double>(span - total) * 100.0
        / static_cast<double>(span);
  }
  if (total == 0) {
    return 0.0;
  }
  return static_cast<double>(total - sampleCount * minValue) * 100.0
      / static_cast<double>(total);
}

// The figures of one kept set of the series, the shape the gap folds
// carry: the values copy into the sorted double vector `percentileAt`
// reads, the five points read through `percentileField` with the
// standing minimum sample count, and the five calibration figures and
// the drop count ride the source (FR-007, FR-008, R-09). The source is
// the per-cpu state of a joined fold and the latch-time snapshot of a
// mid-run fold alike: both carry the fields by the same name. The series
// folds share the reading, the run fold over the kept set, the prefix
// fold over the prefix, the snapshot fold over the copied prefix, and
// the window fold over the set the windows placed, in any order.
template<class Source>
auto seriesFigures(const Source& source,
                   std::span<const std::uint64_t> values) -> JitterSeriesStats
{
  JitterSeriesStats stats {};
  stats.workCount = source.workCount;
  stats.unitTimeNs = source.unitTimeNs;
  stats.targetNs = source.targetNs;
  stats.quantumNs = source.quantumNs;
  stats.calibrationSpanNs = source.calibrationSpanNs;
  stats.droppedSamples = source.droppedSamples;
  stats.sampleCount = values.size();
  if (values.empty()) {
    return stats;
  }

  std::vector<double> sorted;
  sorted.reserve(values.size());
  std::uint64_t total = 0;
  for (const auto value : values) {
    sorted.push_back(static_cast<double>(value));
    total += value;
  }
  std::sort(sorted.begin(), sorted.end());

  stats.minValue = static_cast<std::uint64_t>(sorted.front());
  stats.maxValue = static_cast<std::uint64_t>(sorted.back());
  stats.meanValue =
      static_cast<double>(total) / static_cast<double>(sorted.size());
  stats.medianValue = percentileField(sorted, 50.0);
  stats.p90Value = percentileField(sorted, 90.0);
  stats.p99Value = percentileField(sorted, 99.0);
  stats.p999Value = percentileField(sorted, 99.9);
  stats.p9999Value = percentileField(sorted, 99.99);
  stats.noisePercent = seriesNoisePercent(
      source.method, stats.sampleCount, stats.minValue, stats.maxValue, total);
  return stats;
}

// The figures of one kept gap set, the shape the two run-level folds
// answer: the values copy into the sorted double vector `percentileAt`
// reads, the five points read through `percentileField` with the
// standing minimum sample count, the covered span the records' own stamp
// span, and the rate fields the duration divisions answer (E-03, R-05).
// The joined fold passes the buffer's kept set, and the snapshot fold
// passes the prefix the latch copied. The changes ride that prefix
// element for element, and the edge pair names the window the reads
// bound, so the three figures of specs/048 answer beside the standing
// sixteen (FR-013, FR-014, FR-016, P-3, P-5). An empty kept set leaves
// the standing sixteen at 0 and answers the window's own two changes.
auto gapFigures(std::span<const GapRecord> records,
                std::span<const std::uint64_t> changes,
                const WindowEdges& edges,
                const std::uint64_t droppedGaps,
                const std::uint64_t thresholdNs,
                const std::uint64_t loopPeriodNs) -> JitterStats
{
  JitterStats stats {};
  stats.gapCount = records.size();
  stats.droppedGaps = droppedGaps;
  stats.thresholdNs = thresholdNs;
  stats.loopPeriodNs = loopPeriodNs;

  // specs/048 FR-013, FR-014, FR-015, FR-016, P-3: a change above zero
  // marks its gap `SMI` and a change of zero marks it `NOT_SMI`, and the
  // marked count counts the gaps that carry that mark. One availability
  // stands over the marks and the count together: FR-015 leaves no gap of
  // the run marked when the count stands unavailable, so a window whose
  // SMI reads answer only one of its two edges carries no mark, and the
  // count stays at 0 beside the empty mark list the writer leaves (D-13,
  // P-3). A fold that holds no edge read is the per-benchmark and latch
  // shape, which counts the gaps of its own item (FR-025).
  stats.smiDelta = windowDeltaOf(edges.smiOpening, edges.smiClosing);
  const bool windowReadTheSource =
      edges.smiOpening.has_value() || edges.smiClosing.has_value();
  if (!windowReadTheSource || stats.smiDelta.has_value()) {
    for (const auto change : changes) {
      stats.smiGapCount += change > 0 ? std::uint64_t {1} : std::uint64_t {0};
    }
  }
  stats.nmiDelta = windowDeltaOf(edges.nmiOpening, edges.nmiClosing);

  std::vector<double> sorted;
  sorted.reserve(records.size());
  std::uint64_t total = 0;
  for (const auto& record : records) {
    sorted.push_back(static_cast<double>(record.gapNs));
    total += record.gapNs;
  }
  std::sort(sorted.begin(), sorted.end());
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
  // stamp span, the only duration the per-cpu figures carry. The loop's
  // start-to-stop span needs a field the state does not declare; the
  // deviation rides the wave ledger (E-03, R-05).
  std::uint64_t firstStamp = records.front().stamp;
  std::uint64_t lastStamp = firstStamp;
  for (const auto& record : records) {
    if (record.stamp < firstStamp) {
      firstStamp = record.stamp;
    }
    if (record.stamp > lastStamp) {
      lastStamp = record.stamp;
    }
  }
  stats.runtimeNs = lastStamp - firstStamp;

  // The rate fields divide by the span, and the span of a kept set that
  // holds one record stands at zero: the stamp span of one stamp is zero
  // by definition. The rate fields stand at zero there, the way the empty
  // kept set's zeros stand, so no division by zero reaches the writer
  // (E-03, R-05).
  stats.gapsPerSecond = stats.runtimeNs == 0
      ? 0.0
      : static_cast<double>(stats.gapCount) * 1e9
          / static_cast<double>(stats.runtimeNs);
  stats.totalPercent = stats.runtimeNs == 0 ? 0.0
                                            : static_cast<double>(stats.totalNs)
          * 100.0 / static_cast<double>(stats.runtimeNs);
  SG_ENSURE(stats.minNs <= stats.medianNs && stats.medianNs <= stats.p90Ns
                && stats.p90Ns <= stats.p99Ns && stats.p99Ns <= stats.p999Ns
                && stats.p999Ns <= stats.p9999Ns
                && stats.p9999Ns <= stats.maxNs,
            "the percentile chain stands ordered (E-03, PC-4)");
  return stats;
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

// The projection's implementation (specs/045-merge-fixes phase 16,
// FR-039): the exact integer form the header's contract states. The
// rate-absent run returns the raw tick, the identity the fixture
// catalogs without the rate pair's second leaf stand at.
auto TickProjector::project(std::uint64_t tick) noexcept -> std::uint64_t
{
  if (rateHz == 0) {
    return tick;
  }
  if (!seeded) {
    seeded = true;
    lastTick = tick;
    return lastNs;
  }
  const std::uint64_t delta = tick - lastTick;
  SG_REQUIRE(delta < (1ULL << 63),
             "the read delta stands under half the ring: a backward read "
             "stands refused");
  lastTick = tick;
  // The chunked exact form: a delta above one chunk's product range
  // walks the chunks, and the carried remainder keeps the chunk sum
  // the single-step projection answers, so a long pause between reads
  // projects exact and drift-free like any other.
  constexpr std::uint64_t kChunkTicks = 1000000000;
  std::uint64_t left = delta;
  while (left > kChunkTicks) {
    const std::uint64_t chunkProduct = kChunkTicks * 1000000000ULL + remainder;
    lastNs += chunkProduct / rateHz;
    remainder = chunkProduct % rateHz;
    left -= kChunkTicks;
  }
  const std::uint64_t product = left * 1000000000ULL + remainder;
  lastNs += product / rateHz;
  remainder = product % rateHz;
  return lastNs;
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

  // specs/045-merge-fixes phase 16 (FR-039): the tick rate stands
  // measured once per run, here before the items read and outside
  // every window, so the calibration, the thresholds and the stored
  // figures all project at the one rate. The best-effort reading
  // stands at the identity when the catalog answers no rate pair.
  session->rateHz = measureTickRate();

  // The states build once, in place: the work items reference them for
  // the whole run, so the vector never grows after this line (E-05).
  m_states = std::vector<ProbeCpuState>(m_options.cpus.size());
  for (std::size_t index = 0; index < m_options.cpus.size(); ++index) {
    m_states.at(index).cpu = m_options.cpus.at(index);
    // specs/043 FR-009, R-10: the method the run selected rides the
    // state, so the series fold reads the noise fraction of the method
    // that fills this cpu's buffer.
    m_states.at(index).method = m_options.method;
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
                   .session = session.get(),
                   .index = index}));
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

  // specs/045-merge-fixes FR-008, D-5: the snapshot publish. Each item
  // copies its prefix at the latch line and release-stores its flag
  // behind the barrier. The holder waits for every flag, so the return
  // stands behind the copies and the mid-run fold sites that follow read
  // a published snapshot. The wait spans one prefix copy per cpu and
  // stands off every measured window and off the hot path.
  for (auto& state : m_states) {
    while (!state.latchPublished.load(std::memory_order_acquire)) {
      pauseHint();
    }
  }

  probeSession = std::move(session);

  if (m_options.mode == JitterProbeMode::PRECHECK) {
    // FR-014, D-3: the precheck mode allows every overlap, because the
    // probe stops before the benchmarks start, so the stop and join
    // stand here, inside the start, and the states stay readable where
    // the runner reads them (R-04).
    stopAndJoin();
  }
}

auto JitterProbe::stop() noexcept -> void
{
  for (auto& state : m_states) {
    state.stop.store(true, std::memory_order_relaxed);
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
  stop();
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
 * \post `smiGapCount` counts the kept gaps whose change element stands above
 *       zero, and an absent source leaves it at 0. A change ring shorter than
 *       the kept set leaves the gaps it does not reach unmarked (FR-014,
 *       FR-015, P-3)
 * \post `smiDelta` and `nmiDelta` answer the change of the state's pair of
 *       edge reads and stand with no value when either read of that source
 *       stands absent (FR-011, FR-016)
 */
auto gapStatsOf(const ProbeCpuState& state) -> JitterStats
{
  const std::span<const GapRecord> kept(state.buffer.data(), state.kept);
  const auto changes = changePrefixOf(state.changes, state.kept);
  auto stats = gapFigures(kept,
                          changes,
                          state.edges,
                          state.droppedGaps,
                          state.thresholdNs,
                          state.loopPeriodNs);
  SG_ENSURE(
      stats.gapCount == state.kept && stats.droppedGaps == state.droppedGaps,
      "the counts answer the kept and dropped records (FR-010, D-6)");
  SG_ENSURE(stats.thresholdNs == state.thresholdNs
                && stats.loopPeriodNs == state.loopPeriodNs,
            "the effective fields carry the state's own (FR-007, R-05)");
  SG_ENSURE(durationDivisionsHold(stats),
            "the duration divisions answer the covered span (E-03, R-05)");
  return stats;
}

/**
 * @brief specs/045-merge-fixes FR-008, D-5: the gap fold over one probe
 * cpu's latch-time snapshot (FR-010, D-6): the sixteen fields over the
 * gap prefix the latch copied, the covered span that prefix's stamp
 * span, and the three attribution figures of specs/048. The pre-check
 * record of a `mode=both` run reads this fold for every method while the
 * alongside loop runs on, and the fold reads no live buffer. The
 * `hwlat` method reads the same fold through the same record.
 * The empty prefix answers the zero fields, the window's own two changes
 * and none of the rows below.
 *
 * \pre `gapsKept` equals the length of the copied gap prefix
 * \post the fold answers the figures of the copied prefix, the
 *       threshold, the loop period and the drop count carrying the
 *       snapshot's own (FR-010, D-6)
 * \post minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <=
 *       maxNs
 * \post `runtimeNs` is the copied records' stamp span, and the rate
 *       fields divide by it (E-03, R-05)
 * \post `smiGapCount` counts the copied gaps whose change element stands
 *       above zero, and a copied change ring shorter than the copied prefix
 *       leaves the gaps it does not reach unmarked (FR-014, FR-015, P-3)
 * \post `smiDelta` and `nmiDelta` answer the change of the snapshot's pair
 *       of edge reads, the two reads of the pre-check window, and stand with
 *       no value when either read of that source stands absent (FR-011,
 *       FR-016)
 */
auto gapStatsOf(const JitterLatchSnapshot& snapshot) -> JitterStats
{
  SG_REQUIRE(snapshot.gapsKept == snapshot.gaps.size(),
             "the count names the copied prefix (FR-008, D-5)");
  const std::span<const GapRecord> kept(snapshot.gaps.data(),
                                        snapshot.gapsKept);
  const auto changes = changePrefixOf(snapshot.changes, snapshot.gapsKept);
  auto stats = gapFigures(kept,
                          changes,
                          snapshot.window,
                          snapshot.droppedGaps,
                          snapshot.thresholdNs,
                          snapshot.loopPeriodNs);
  SG_ENSURE(stats.gapCount == snapshot.gapsKept
                && stats.droppedGaps == snapshot.droppedGaps,
            "the counts answer the copied and dropped records (FR-008, D-5)");
  SG_ENSURE(stats.thresholdNs == snapshot.thresholdNs
                && stats.loopPeriodNs == snapshot.loopPeriodNs,
            "the effective fields carry the snapshot's own (FR-007, R-05)");
  SG_ENSURE(durationDivisionsHold(stats),
            "the duration divisions answer the covered span (E-03, R-05)");
  return stats;
}

/**
 * @brief specs/043-fwq-ftq: the series fold of one probe cpu over a
 * prefix of its kept samples (FR-007, FR-008, FR-009, R-09, R-10): the
 * figures of the first `prefixCount` samples, the five calibration
 * figures of E-22, and the noise fraction of the method that filled the
 * buffer. The run-level fold of FR-007 reads the whole kept set through
 * this function with the kept count, and the pre-check record of a
 * `mode=both` run reads the prefix the latch snapshotted (FR-004,
 * FR-020, E-27). The covered span the runtime names is the span the kept
 * samples' recovered stamps cover,
 * the only duration the joined per-cpu state carries, the calibration
 * span standing out of it (FR-010). The empty kept set answers the
 * zero figures and none of the rows below.
 *
 * \post minValue <= medianValue <= p90Value <= p99Value <= p999Value
 *       <= p9999Value <= maxValue
 * \post `sampleCount` counts the kept samples, and `droppedSamples`
 *       counts the samples the full buffer dropped (FR-007, FR-022)
 * \post the noise fraction answers the method's rule of FR-009, a
 *       maximum count of 0 answering 100 (R-10)
 * \post `seriesRuntimeNs` is the recovered stamp span of the kept
 *       samples, the calibration span out (FR-007, FR-010)
 */
auto seriesStatsOf(const ProbeCpuState& state,
                   const std::size_t prefixCount) -> JitterSeriesStats
{
  SG_REQUIRE(prefixCount <= state.seriesKept,
             "the prefix stands inside the kept set (FR-020, E-27)");
  const std::span<const std::uint64_t> kept(state.series.data(), prefixCount);
  auto stats = seriesFigures(state, kept);

  // The span the recovered stamps cover: an FWQ stamp stands at the
  // start plus the running sum of the unit times, and an FTQ stamp at
  // the start plus the rank plus one, times the quantum, so the span
  // from the start to the last stamp is the unit times' sum for FWQ
  // and the sample count times the quantum for FTQ (R-11, FR-007,
  // FR-010).
  std::uint64_t span = 0;
  for (std::size_t index = 0; index < prefixCount; ++index) {
    if (state.method == JitterMethod::FTQ) {
      span += state.quantumNs;
    } else {
      span += state.series.at(index);
    }
  }
  stats.seriesRuntimeNs = span;
  // The chain the gap folds state beside the same five points: the
  // nearest-rank points stand ordered on the sorted values by
  // construction, and the check reads the construction (FR-008, FR-020).
  SG_ENSURE(stats.minValue <= stats.medianValue
                && stats.medianValue <= stats.p90Value
                && stats.p90Value <= stats.p99Value
                && stats.p99Value <= stats.p999Value
                && stats.p999Value <= stats.p9999Value
                && stats.p9999Value <= stats.maxValue,
            "the percentile chain stands ordered (FR-008, PC-4)");
  SG_ENSURE(stats.sampleCount == prefixCount
                && stats.noisePercent >= 0.0 && stats.noisePercent <= 100.0,
            "the count answers the kept samples and the fraction stands in "
            "the range 0 to 100 (FR-007, FR-009)");
  return stats;
}

auto seriesStatsOf(const ProbeCpuState& state) -> JitterSeriesStats
{
  return seriesStatsOf(state, state.seriesKept);
}

auto latchSnapshotOf(const ProbeCpuState& state) -> const JitterLatchSnapshot&
{
  SG_REQUIRE(state.latchPublished.load(std::memory_order_acquire),
             "the item published its snapshot at the latch line (FR-008, D-5)");
  return state.latch;
}

/**
 * @brief specs/045-merge-fixes FR-008, D-5: the series fold over one
 * probe cpu's latch-time snapshot (FR-007, FR-008, FR-009, R-09, R-10):
 * the figures of the copied prefix, the five calibration figures of
 * E-22, and the noise fraction of the method that filled the buffer. The
 * covered span the runtime names is the span the copied prefix's
 * recovered stamps covers, the calibration span standing out of it
 * (FR-010). The pre-check record of a `mode=both` run and the method
 * noise gate read this fold while the alongside loop runs on. The empty
 * prefix answers the zero figures and none of the rows below.
 *
 * \pre `seriesKept` equals the length of the copied series prefix
 * \post the fold answers the figures of the copied prefix, the
 *       calibration figures and the drop count carrying the snapshot's
 *       own (FR-007, FR-022)
 * \post minValue <= medianValue <= p90Value <= p99Value <= p999Value
 *       <= p9999Value <= maxValue
 * \post the noise fraction answers the method's rule of FR-009, a
 *       maximum count of 0 answering 100 (R-10)
 * \post `seriesRuntimeNs` is the recovered stamp span of the copied
 *       prefix (FR-007, FR-010)
 */
auto seriesStatsOf(const JitterLatchSnapshot& snapshot) -> JitterSeriesStats
{
  SG_REQUIRE(snapshot.seriesKept == snapshot.series.size(),
             "the count names the copied prefix (FR-008, D-5)");
  const std::span<const std::uint64_t> kept(snapshot.series.data(),
                                            snapshot.seriesKept);
  auto stats = seriesFigures(snapshot, kept);

  // The span the recovered stamps cover: an FWQ stamp stands at the
  // start plus the running sum of the unit times, and an FTQ stamp at
  // the start plus the rank plus one, times the quantum, so the span
  // from the start to the last stamp is the unit times' sum for FWQ and
  // the sample count times the quantum for FTQ (R-11, FR-007, FR-010).
  std::uint64_t span = 0;
  for (const auto value : kept) {
    span += snapshot.method == JitterMethod::FTQ ? snapshot.quantumNs : value;
  }
  stats.seriesRuntimeNs = span;
  SG_ENSURE(stats.minValue <= stats.medianValue
                && stats.medianValue <= stats.p90Value
                && stats.p90Value <= stats.p99Value
                && stats.p99Value <= stats.p999Value
                && stats.p999Value <= stats.p9999Value
                && stats.p9999Value <= stats.maxValue,
            "the percentile chain stands ordered (FR-008, PC-4)");
  SG_ENSURE(stats.sampleCount == snapshot.seriesKept
                && stats.noisePercent >= 0.0 && stats.noisePercent <= 100.0,
            "the count answers the copied prefix and the fraction stands in "
            "the range 0 to 100 (FR-007, FR-009)");
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
 * \post `smiGapCount` counts the placed gaps whose change element stands
 *       above zero, and the gaps a change ring shorter than the kept set does
 *       not reach stand unmarked. The two delta fields stand with no value,
 *       because this fold holds no window edge (FR-015, FR-016, FR-025, P-3)
 */
auto gapStatsOf(const ProbeCpuState& state,
                std::span<const TimedWindow> windows) -> JitterStats
{
  JitterStats stats {};

  // specs/045-merge-fixes phase 16 (FR-039): the windows' edges stand
  // in the runner's raw ticks, and the records' stamps stand in the
  // run's projected nanoseconds, so each edge converts at the state's
  // own origin and rate, the mapping the item's reads used, and the
  // placement compares one time base. A rate of 0 stands at the
  // identity and the edges pass through.
  const bool convertsEdges = state.tickRateHz != 0;
  const auto edgeNs = [&](std::uint64_t edge) noexcept -> std::uint64_t
  {
    if (!convertsEdges) {
      return edge;
    }
    TickProjector projector {.lastTick = state.firstTickRaw,
                             .remainder = 0,
                             .rateHz = state.tickRateHz,
                             .seeded = true};
    return projector.project(edge);
  };

  // The covered span the entry's fields divide (E-03, R-05): the
  // windows' total, the one duration the window set carries. The fold
  // of an empty placement answers the empty kept set's zeros below.
  std::uint64_t coveredNs = 0;
  for (const auto& window : windows) {
    coveredNs += window.end - window.start;
  }

  std::vector<double> sorted;
  sorted.reserve(state.kept);
  const auto changes = changePrefixOf(state.changes, state.kept);
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
        { return edgeNs(window.start) <= record.stamp
                     && record.stamp < edgeNs(window.end); });
    if (!placed) {
      continue;
    }
    sorted.push_back(static_cast<double>(record.gapNs));
    total += record.gapNs;
    // specs/048 FR-013, FR-014, FR-025, P-3: the change element the keep step
    // paired with this record marks it, so the item's count counts the placed
    // gaps whose element stands above zero, and a gap the change ring does not
    // reach stands unmarked. The two window changes answer nothing here,
    // because this fold holds no edge read (FR-015, FR-016).
    stats.smiGapCount +=
        changeAt(changes, index) > 0 ? std::uint64_t {1} : std::uint64_t {0};
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

/**
 * @brief specs/043-fwq-ftq: the per-benchmark series fold of one probe
 * cpu over one benchmark's timed windows (FR-020, R-11): the kept
 * samples whose recovered stamp falls inside a half-open window, over
 * the covered span the windows' total duration. The fold reads the one
 * series buffer the probe cpu owns, so no second buffer joins. The
 * empty placement answers the absent record and none of the rows
 * below.
 *
 * \post the fold recovers each sample's stamp from `seriesStartNs`,
 *       the stamp the first series store wrote: the FWQ stamp is that
 *       start plus the running sum of the unit times, and the FTQ
 *       stamp that start plus the sample rank plus one, times the
 *       quantum (R-11, E-26)
 * \post a kept sample folds into every window whose half-open
 *       `[start, end)` contains its recovered stamp, and the fields
 *       carry the contracts the run-level fold states (E-06, R-04)
 * \post `seriesRuntimeNs` is the windows' total duration (E-03, R-05)
 * \post the fold stands absent when the windows place no sample
 *       (FR-020)
 */
auto seriesStatsOf(const ProbeCpuState& state,
                   std::span<const TimedWindow> windows)
    -> std::optional<JitterSeriesStats>
{
  // specs/045-merge-fixes phase 16 (FR-039): the windows' edges stand
  // in the runner's raw ticks, and the recovered stamps stand in the
  // run's projected nanoseconds, so each edge converts at the state's
  // own origin and rate, the mapping the item's reads used, and the
  // placement compares one time base. A rate of 0 stands at the
  // identity and the edges pass through.
  const bool convertsEdges = state.tickRateHz != 0;
  const auto edgeNs = [&](std::uint64_t edge) noexcept -> std::uint64_t
  {
    if (!convertsEdges) {
      return edge;
    }
    TickProjector projector {.lastTick = state.firstTickRaw,
                             .remainder = 0,
                             .rateHz = state.tickRateHz,
                             .seeded = true};
    return projector.project(edge);
  };

  // The covered span the entry's fields divide (E-03, R-05): the
  // windows' total, the one duration the window set carries.
  std::uint64_t coveredNs = 0;
  for (const auto& window : windows) {
    coveredNs += window.end - window.start;
  }

  std::vector<std::uint64_t> placed;
  placed.reserve(state.seriesKept);
  std::uint64_t recoveredStamp = state.seriesStartNs;
  for (std::size_t index = 0; index < state.seriesKept; ++index) {
    const auto value = state.series.at(index);
    if (state.method == JitterMethod::FTQ) {
      recoveredStamp += state.quantumNs;
    } else {
      recoveredStamp += value;
    }
    // A sample folds into every window whose half-open `[start, end)`
    // contains its recovered stamp (E-06, R-04), and the entry's fold
    // reads each placed sample once, the one reading that leaves
    // `sampleCount` the count of the samples the benchmark's windows
    // placed (FR-020).
    const bool inside = std::any_of(
        windows.begin(),
        windows.end(),
        [&](const TimedWindow& window) noexcept -> bool {
          return edgeNs(window.start) <= recoveredStamp
                 && recoveredStamp < edgeNs(window.end);
        });
    if (inside) {
      placed.push_back(value);
    }
  }
  if (placed.empty()) {
    return std::nullopt;
  }
  auto stats = seriesFigures(state, placed);
  stats.seriesRuntimeNs = coveredNs;
  // The chain of the run fold, read beside the same five points of the
  // windows' set (FR-008, FR-020).
  SG_ENSURE(stats.minValue <= stats.medianValue
                && stats.medianValue <= stats.p90Value
                && stats.p90Value <= stats.p99Value
                && stats.p99Value <= stats.p999Value
                && stats.p999Value <= stats.p9999Value
                && stats.p9999Value <= stats.maxValue,
            "the percentile chain stands ordered (FR-020, PC-4)");
  SG_ENSURE(stats.sampleCount == placed.size()
                && stats.noisePercent >= 0.0 && stats.noisePercent <= 100.0,
            "the count answers the placed samples and the fraction stands in "
            "the range 0 to 100 (FR-020, FR-009)");
  return stats;
}

auto precheckNoisyCpus(const std::vector<ProbeCpuState>& states,
                       const std::vector<std::uint32_t>& plannedCpus,
                       const std::uint32_t noisePercent)
    -> std::vector<std::uint32_t>
{
  std::vector<std::uint32_t> noisy;
  for (const auto cpu : plannedCpus) {
    const auto found = std::ranges::find_if(
        states, [cpu](const ProbeCpuState& state) { return state.cpu == cpu; });
    if (found == states.end() || !fillsSeriesRing(found->method)) {
      continue;
    }
    if (seriesStatsOf(latchSnapshotOf(*found)).noisePercent
        >= static_cast<double>(noisePercent))
    {
      noisy.push_back(cpu);
    }
  }
  return noisy;
}

// specs/048 P-3, E-03, FR-015: the change prefix of one kept set. The item's
// keep step pairs the gap ring and the change ring element for element, so a
// state whose change ring stands as long as its kept set answers that prefix
// whole. A state built beside the item, the shape a fixture gives a fold,
// carries a shorter ring, and the gaps that ring does not reach stand unmarked,
// the answer an absent source leaves.
auto changePrefixOf(const std::vector<std::uint64_t>& changes,
                    const std::size_t kept) -> std::span<const std::uint64_t>
{
  return std::span<const std::uint64_t>(changes.data(),
                                        std::min(kept, changes.size()));
}

// specs/043 FR-003, D-1: the methods that fill the series ring, one sample
// per entry. `tsc` and `hwlat` fill the gap ring, so neither calibrates a
// unit loop nor pre-sizes the series buffer (E-05, A-4).
auto fillsSeriesRing(const JitterMethod method) -> bool
{
  return method == JitterMethod::FWQ || method == JitterMethod::FTQ;
}

auto jitterMethodName(const JitterMethod method) -> const char*
{
  // The names sit in a table indexed by the enumerator, the reading the
  // availability names take: a value outside the closed enumeration
  // answers the last word (FR-019). specs/048 D-1 adds the fourth word, and
  // the last word with it (FR-002).
  constexpr std::array<const char*, 4> kNames {"tsc", "fwq", "ftq", "hwlat"};
  static_assert(kNames.size() == std::to_underlying(JitterMethod::HWLAT) + 1);
  const auto index = std::to_underlying(method);
  return index < kNames.size() ? kNames.at(index) : kNames.back();
}

auto addSpectrumTo(const ProbeCpuState& state,
                   JitterSeriesStats& series) -> void
{
  const std::span<const std::uint64_t> kept {state.series.data(),
                                             state.seriesKept};
  if (series.sampleCount == 0 || kept.empty()) {
    return;
  }
  // FR-017, FR-018: each span's owner stands named beside the span, so
  // the vector the transform reads lives for the whole call.
  std::vector<double> counts;
  std::vector<double> resampled;
  std::span<const double> samples {};
  std::uint64_t stepNs = 0;
  if (state.method == JitterMethod::FWQ) {
    stepNs = series.medianValue;
    if (stepNs == 0) {
      return;
    }
  } else {
    stepNs = series.quantumNs;
    if (stepNs == 0) {
      return;
    }
  }
  // specs/045-merge-fixes BFW-005, D-20: the transform of the kept prefix
  // runs once per probe cpu. The report record and the console fold each fold
  // the same state, and the transform over a full ring is the standing cost
  // of the sample (specs/043 R-6), so the answer of the first fold stands
  // kept in the state that owns the series and the second fold reads it. The
  // step stands part of the answer, because the resample below reads it: a
  // fold at another step reads another series and stands recomputed.
  if (!state.spectrumAnswer.has_value()
      || state.spectrumAnswer->stepNs != stepNs)
  {
    if (state.method == JitterMethod::FWQ) {
      resampled = resampleUnitTimes(kept, stepNs, kSpectrumCap);
      samples = resampled;
    } else {
      counts.reserve(kept.size());
      for (const auto count : kept) {
        counts.push_back(static_cast<double>(count));
      }
      samples = counts;
    }
    if (samples.empty()) {
      return;
    }
    state.spectrumAnswer = dftPeriods(samples, stepNs);
  }
  const SpectrumResult& answer = *state.spectrumAnswer;
  series.spectrumPoints = answer.points;
  if (state.method == JitterMethod::FWQ) {
    series.resampleStepNs = answer.stepNs;
  }
  for (const auto& period : answer.periods) {
    series.periods.push_back(JitterPeriodRecord {
        .periodNs = period.periodNs, .powerRatio = period.powerRatio});
  }
}

auto installTestReader(std::unique_ptr<counters::WindowReader> reader) -> void
{
  installedReader = std::move(reader);
}

namespace
{

// specs/043 FR-002: the top of the `noise` percent range, the bound the
// value parser enforces at the command-line boundary.
constexpr std::uint64_t kMaxNoisePercent = 100;

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

}  // namespace

// The cpu token of the value grammar, lifted to the detail namespace so
// both value grammars read it (specs/036 R-06): see the declaration's
// contract in detail/jitter_probe.hpp.
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
  // so one line stands for both refusals. The online cpu set the
  // topology seam owns bounds the index beside the mask: the set is
  // the list the kernel owns, gaps and all, so a cpu the mask holds
  // and the set lacks stands refused on the line of its own below. An
  // index that reaches this test stands below the set type's size, the
  // mask read having refused the rest, so it reaches the set as the
  // width the set carries. The set stands behind the detail seam the
  // tests set and clear, the way the test layout installs (FR-005,
  // FR-041).
  const auto onlineCpus = onlineCpuSet();
  for (auto cpu = low; cpu <= high; ++cpu) {
    if (!CPU_ISSET(cpu, &mask)) {
      message = "the cpu '" + quotedText(token)
          + "' stands outside the process affinity mask";
      return false;
    }
    const auto index = static_cast<std::uint32_t>(cpu);
    if (std::find(onlineCpus.begin(), onlineCpus.end(), index)
        == onlineCpus.end())
    {
      message = "the cpu '" + quotedText(token)
          + "' stands outside the online cpu set";
      return false;
    }
    cpus.push_back(index);
  }
  return true;
}

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
    } else if (key == "method") {
      // specs/043 FR-001, D-1 and specs/048 FR-002, D-1: one method per run,
      // the four words the value names; a value with no method key keeps the
      // standing method. The fourth word keeps the gap loop, the standing
      // threshold rule and the two counter reads (D-11, A-4).
      if (keyArgument == "tsc") {
        parsed.method = JitterMethod::TSC;
      } else if (keyArgument == "fwq") {
        parsed.method = JitterMethod::FWQ;
      } else if (keyArgument == "ftq") {
        parsed.method = JitterMethod::FTQ;
      } else if (keyArgument == "hwlat") {
        parsed.method = JitterMethod::HWLAT;
      } else {
        message = "the method '" + quotedText(keyArgument)
            + "' is not tsc, fwq, ftq or hwlat";
        return false;
      }
    } else if (key == "quantum") {
      // specs/043 FR-002, FR-003: the quantum in microseconds, a whole
      // number inside 10 through 100,000; the standing number grammar
      // reads the digits and refuses a fractional token.
      std::uint64_t number = 0;
      if (!parseProbeNumber(keyArgument, number, false)
          || number < kMinQuantumUs || number > kMaxQuantumUs)
      {
        message = "the quantum '" + quotedText(keyArgument)
            + "' is not a number between 10 and 100000";
        return false;
      }
      parsed.quantumUs = static_cast<std::uint32_t>(number);
    } else if (key == "work") {
      // specs/043 FR-002, FR-003: the work count of one unit, a whole
      // number inside 1 through 16,777,216; the count 0 names the
      // calibration, so the key admits no zero.
      std::uint64_t number = 0;
      if (!parseProbeNumber(keyArgument, number, false)
          || number > kMaxWorkCount)
      {
        message = "the work '" + quotedText(keyArgument)
            + "' is not a number between 1 and 16777216";
        return false;
      }
      parsed.workCount = static_cast<std::uint32_t>(number);
    } else if (key == "noise") {
      // specs/043 FR-002, FR-003: the noise percent, a whole number
      // inside 0 through 100. The value accepts the key beside every
      // method, and the run discloses it as unused where no gate reads
      // it (FR-021).
      std::uint64_t number = 0;
      if (!parseProbeNumber(keyArgument, number, true)
          || number > kMaxNoisePercent)
      {
        message = "the noise '" + quotedText(keyArgument)
            + "' is not a number between 0 and 100";
        return false;
      }
      parsed.noisePercent = static_cast<std::uint32_t>(number);
    } else if (key == "precheck") {
      // specs/034 FR-002: the key carries two accepted forms, the
      // millisecond number and the word `serial`; a token that is
      // neither stands the standing one-line refusal (FR-003).
      if (keyArgument == "serial") {
        parsed.precheckSerial = true;
      } else {
        std::uint64_t number = 0;
        if (!parseProbeNumber(keyArgument, number, false)
            || number > std::numeric_limits<std::uint32_t>::max())
        {
          message = "the precheck '" + quotedText(keyArgument)
              + "' is not a positive number";
          return false;
        }
        parsed.precheckMs = static_cast<std::uint32_t>(number);
      }
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
  // specs/043 FR-003, R-03: the method and its key pair. The quantum
  // names the FTQ measure and the work count the FWQ measure, so a key
  // that belongs to the other method answers one quoted line.
  if (parsed.method == JitterMethod::FWQ && seenKeys.contains("quantum")) {
    message = "the key 'quantum' stands with the method 'fwq'";
    return false;
  }
  if (parsed.method == JitterMethod::FTQ && seenKeys.contains("work")) {
    message = "the key 'work' stands with the method 'ftq'";
    return false;
  }
  return true;
}

// specs/034 R-03, R-04: the attribution readers. Every read resolves
// under the file-root seam (FR-019), and a source that is missing,
// unreadable or unparsable stands absent for the figure it carries,
// the run never failing on it (FR-018, AC-b2).
auto readUnderRoot(const std::string& relative) -> std::optional<std::string>
{
  std::ifstream file(procRootPath(relative));
  if (!file) {
    return std::nullopt;
  }
  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line;
    text += '\n';
  }
  return text;
}

// One unsigned figure off one token; a token that parses as no number
// answers nothing (FR-018).
auto parseCountToken(const std::string& token) -> std::optional<std::uint64_t>
{
  std::uint64_t figure = 0;
  const auto* begin = token.data();
  const auto* end = begin + token.size();
  const auto result = std::from_chars(begin, end, figure);
  if (result.ec != std::errc {} || result.ptr != end) {
    return std::nullopt;
  }
  return figure;
}

// The interrupt count of one cpu: the sum of the cpu's column over
// every row of /proc/interrupts, the rows' per-cpu counts standing
// after the colon (R-4 of research.md). A needed column that parses as
// no number stands the source unparsable (FR-018).
auto interruptsOfText(const std::string& text,
                      std::uint32_t cpu) -> std::optional<std::uint64_t>
{
  std::uint64_t sum = 0;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    const auto tail = line.substr(colon + 1);
    auto tokens = std::istringstream(tail);
    std::vector<std::string> fields;
    std::string token;
    while (tokens >> token) {
      fields.push_back(token);
    }
    if (fields.size() <= cpu) {
      return std::nullopt;
    }
    const auto figure = parseCountToken(fields.at(cpu));
    if (!figure.has_value()) {
      return std::nullopt;
    }
    sum += *figure;
  }
  return sum;
}

// The per-kind softirq counts of one cpu: one figure per kind row the
// file states, the kind named as the file states it (R-5 of
// research.md). A kind whose needed column parses as no number stands
// absent for that kind alone (FR-018).
auto softirqsOfText(const std::string& text, std::uint32_t cpu)
    -> std::vector<std::pair<std::string, std::optional<std::uint64_t>>>
{
  std::vector<std::pair<std::string, std::optional<std::uint64_t>>> kinds;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    const auto colon = line.find(':');
    if (colon == std::string::npos || colon == 0) {
      continue;
    }
    auto kind = line.substr(0, colon);
    const auto trimEnd = kind.find_last_not_of(" \t");
    if (trimEnd == std::string::npos) {
      continue;
    }
    kind = kind.substr(0, trimEnd + 1);
    const auto trimStart = kind.find_first_not_of(" \t");
    kind = kind.substr(trimStart);
    const auto tail = line.substr(colon + 1);
    auto tokens = std::istringstream(tail);
    std::vector<std::string> fields;
    std::string token;
    while (tokens >> token) {
      fields.push_back(token);
    }
    if (fields.size() <= cpu) {
      kinds.emplace_back(kind, std::nullopt);
      continue;
    }
    kinds.emplace_back(kind, parseCountToken(fields.at(cpu)));
  }
  return kinds;
}

// The context-switch and schedule counts of one cpu: the run-queue
// delay figure and the timeslices figure of the cpu's own line of
// /proc/schedstat (R-3 of research.md). A line that names no figure
// for the cpu, or carries fewer than the three the file states, stands
// the source absent for that cpu (FR-018).
auto schedstatOfText(const std::string& text, std::uint32_t cpu)
    -> std::pair<std::optional<std::uint64_t>, std::optional<std::uint64_t>>
{
  std::pair<std::optional<std::uint64_t>, std::optional<std::uint64_t>> answer;
  const auto prefix = "cpu" + std::to_string(cpu) + " ";
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(prefix, 0) != 0) {
      continue;
    }
    auto tokens = std::istringstream(line.substr(prefix.size()));
    std::vector<std::uint64_t> figures;
    std::string token;
    while (tokens >> token) {
      const auto figure = parseCountToken(token);
      if (!figure.has_value()) {
        return answer;
      }
      figures.push_back(*figure);
    }
    if (figures.size() < 3) {
      return answer;
    }
    // The run-queue delay figure names the context-switch figure, the
    // timeslices figure the schedule count (R-3 of research.md).
    answer.first = figures.at(1);
    answer.second = figures.at(2);
    return answer;
  }
  return answer;
}

// The kernel command line's value of one setting: the token that names
// the key, the value after the equals sign; a line that names no such
// token stands the setting unstated (FR-017).
auto kernelSettingOfText(const std::string& text,
                         std::string_view key) -> std::optional<std::string>
{
  std::istringstream stream(text);
  std::string token;
  while (stream >> token) {
    if (token.rfind(key, 0) == 0 && token.size() > key.size()
        && token.at(key.size()) == '=')
    {
      return token.substr(key.size() + 1);
    }
  }
  return std::nullopt;
}

// The cpus one kernel cpu-list line spells: comma-separated lone
// indices and `low-high` spans, a token that parses as no number or a
// span that stands reversed standing unparsable (R-6 of research.md).
auto cpuListOfText(const std::string& text)
    -> std::optional<std::vector<std::uint32_t>>
{
  // A text that names no line spells no cpu list, the absent file the
  // mask fallback stands for (FR-018).
  if (text.empty()) {
    return std::nullopt;
  }
  std::vector<std::uint32_t> cpus;
  std::size_t scan = 0;
  while (scan < text.size()) {
    const auto stop = text.find(',', scan);
    const auto end = stop == std::string::npos ? text.size() : stop;
    const auto token = text.substr(scan, end - scan);
    if (token.empty()) {
      return std::nullopt;
    }
    const auto dash = token.find('-');
    if (dash == std::string::npos) {
      const auto cpu = parseCountToken(token);
      if (!cpu.has_value()) {
        return std::nullopt;
      }
      cpus.push_back(static_cast<std::uint32_t>(*cpu));
    } else {
      const auto low = parseCountToken(token.substr(0, dash));
      const auto high = parseCountToken(token.substr(dash + 1));
      if (!low.has_value() || !high.has_value() || *low > *high) {
        return std::nullopt;
      }
      for (auto cpu = static_cast<std::uint64_t>(*low);
           cpu <= static_cast<std::uint64_t>(*high);
           ++cpu)
      {
        cpus.push_back(static_cast<std::uint32_t>(cpu));
      }
    }
    if (stop == std::string::npos) {
      break;
    }
    scan = stop + 1;
  }
  return cpus;
}

// The cpus one kernel affinity mask line spells: comma-separated
// 8-hex-digit groups, the leftmost group naming the highest cpus, bit
// i of group g naming cpu g * 32 + i (R-6 of research.md).
auto cpuMaskOfText(const std::string& text)
    -> std::optional<std::vector<std::uint32_t>>
{
  std::vector<std::uint32_t> cpus;
  std::vector<std::string> groups;
  std::size_t scan = 0;
  while (scan < text.size()) {
    const auto stop = text.find(',', scan);
    const auto end = stop == std::string::npos ? text.size() : stop;
    const auto token = text.substr(scan, end - scan);
    if (token.empty() || token.size() > 8) {
      return std::nullopt;
    }
    groups.push_back(token);
    if (stop == std::string::npos) {
      break;
    }
    scan = stop + 1;
  }
  // The leftmost group names the highest cpus, the way the kernel's
  // mask text spells them (R-6 of research.md): the base counts the
  // groups from the right.
  for (std::size_t at = 0; at < groups.size(); ++at) {
    const auto base = static_cast<std::uint32_t>(groups.size() - 1 - at);
    // The strict parse the count reader carries: a token that names
    // no hex word, or one it does not end at, stands unparsable, and
    // no exception leaves the reader (FR-018).
    std::uint64_t word = 0;
    const auto* begin = groups.at(at).data();
    const auto* const end = begin + groups.at(at).size();
    const auto parsed = std::from_chars(begin, end, word, 16);
    if (parsed.ec != std::errc {} || parsed.ptr != end) {
      return std::nullopt;
    }
    for (std::uint32_t bit = 0; bit < 32; ++bit) {
      if ((word >> bit) & 1U) {
        cpus.push_back(base * 32U + bit);
      }
    }
  }
  return cpus;
}

// The trimmed first line of one text, or nothing when the text names
// no line.
auto firstLineOf(std::optional<std::string> text) -> std::optional<std::string>
{
  if (!text.has_value()) {
    return std::nullopt;
  }
  auto line = std::move(*text);
  const auto stop = line.find('\n');
  if (stop != std::string::npos) {
    line = line.substr(0, stop);
  }
  const auto trimEnd = line.find_last_not_of(" \t\r");
  if (trimEnd == std::string::npos) {
    return std::nullopt;
  }
  return line.substr(0, trimEnd + 1);
}

// specs/034 R-03: the attribution snapshot of the measured cpus, every
// read resolving under the file-root seam (FR-019).
auto precheckReadSnapshot(const std::vector<std::uint32_t>& cpus)
    -> std::map<std::uint32_t, PrecheckCpuSnapshot>
{
  std::map<std::uint32_t, PrecheckCpuSnapshot> snapshot;
  const auto interruptsText = readUnderRoot("proc/interrupts");
  const auto softirqsText = readUnderRoot("proc/softirqs");
  const auto schedstatText = readUnderRoot("proc/schedstat");
  for (const auto cpu : cpus) {
    PrecheckCpuSnapshot state;
    if (interruptsText.has_value()) {
      state.interrupts = interruptsOfText(*interruptsText, cpu);
    }
    if (softirqsText.has_value()) {
      state.softirqs = softirqsOfText(*softirqsText, cpu);
    }
    if (schedstatText.has_value()) {
      const auto figures = schedstatOfText(*schedstatText, cpu);
      state.switches = figures.first;
      state.schedules = figures.second;
    }
    snapshot.emplace(cpu, std::move(state));
  }
  return snapshot;
}

// specs/034 R-04: the host settings the run discloses once, every read
// resolving under the file-root seam (FR-019).
auto precheckReadHostSettings(const std::vector<std::uint32_t>& cpus)
    -> PrecheckHostSettings
{
  PrecheckHostSettings host;
  const auto commandLine = readUnderRoot("proc/cmdline");
  if (commandLine.has_value()) {
    host.isolcpus = kernelSettingOfText(*commandLine, "isolcpus");
    host.nohzFull = kernelSettingOfText(*commandLine, "nohz_full");
    host.rcuNocbs = kernelSettingOfText(*commandLine, "rcu_nocbs");
  }
  for (const auto cpu : cpus) {
    const auto governor = firstLineOf(
        readUnderRoot("sys/devices/system/cpu/cpu" + std::to_string(cpu)
                      + "/cpufreq/scaling_governor"));
    host.governors.emplace_back(cpu, std::move(governor));
  }
  std::map<std::uint32_t, std::vector<std::uint32_t>> affinity;
  std::error_code walkError;
  const auto irqRoot = procRootPath("proc/irq");
  auto entry = std::filesystem::directory_iterator(irqRoot, walkError);
  if (!walkError) {
    for (const auto& item : entry) {
      const auto name = item.path().filename().string();
      const auto irq = parseCountToken(name);
      if (!irq.has_value()) {
        continue;
      }
      const auto tail = "proc/irq/" + name;
      // The affinity files stand one line, the trailing newline the
      // file form carries included, so the line reader trims before
      // the parser reads (FR-018).
      auto named = cpuListOfText(
          firstLineOf(readUnderRoot(tail + "/smp_affinity_list")).value_or(""));
      if (!named.has_value()) {
        named = cpuMaskOfText(
            firstLineOf(readUnderRoot(tail + "/smp_affinity")).value_or(""));
      }
      if (!named.has_value()) {
        continue;
      }
      for (const auto cpu : cpus) {
        if (std::find(named->begin(), named->end(), cpu) != named->end()) {
          affinity[cpu].push_back(static_cast<std::uint32_t>(*irq));
        }
      }
    }
  }
  for (const auto cpu : cpus) {
    const auto found = affinity.find(cpu);
    host.irqAffinity.emplace_back(cpu,
                                  found != affinity.end()
                                      ? found->second
                                      : std::vector<std::uint32_t> {});
  }
  return host;
}

// specs/034 R-03: the window deltas of one cpu's attribution: the
// after-before difference of each figure, a figure standing absent
// when either side stands absent (FR-016, FR-018).
auto precheckAttributionDelta(const PrecheckCpuSnapshot& before,
                              const PrecheckCpuSnapshot& after)
    -> PrecheckAttribution
{
  const auto subtract = [](const std::optional<std::uint64_t>& low,
                           const std::optional<std::uint64_t>& high)
      -> std::optional<std::uint64_t>
  {
    if (!low.has_value() || !high.has_value() || *high < *low) {
      return std::nullopt;
    }
    return *high - *low;
  };
  PrecheckAttribution delta;
  delta.interrupts = subtract(before.interrupts, after.interrupts);
  for (const auto& [kind, afterCount] : after.softirqs) {
    std::optional<std::uint64_t> beforeCount;
    for (const auto& [beforeKind, beforeFigure] : before.softirqs) {
      if (beforeKind == kind) {
        beforeCount = beforeFigure;
        break;
      }
    }
    delta.softirqs.emplace_back(kind, subtract(beforeCount, afterCount));
  }
  delta.switches = subtract(before.switches, after.switches);
  delta.schedules = subtract(before.schedules, after.schedules);
  return delta;
}

// specs/034 R-05: the noisy planned cpus of one pre-check run: the
// planned cpus whose state kept at least one above-threshold gap, the
// candidates the planned list does not name standing out of the
// answer (FR-012, FR-014, FR-015).
auto precheckNoisyCpus(const std::vector<ProbeCpuState>& states,
                       const std::vector<std::uint32_t>& plannedCpus)
    -> std::vector<std::uint32_t>
{
  std::vector<std::uint32_t> noisy;
  for (const auto cpu : plannedCpus) {
    const auto found = std::ranges::find_if(
        states, [cpu](const ProbeCpuState& state) { return state.cpu == cpu; });
    if (found == states.end()) {
      continue;
    }
    if (found->kept > 0) {
      noisy.push_back(cpu);
    }
  }
  return noisy;
}

// specs/034 R-01: the measured set of one pre-check run, the planned
// cpus ascending and deduplicated, then the candidates ascending, the
// candidates the standing idle resolution takes over the plans
// (specs/027 D-4).
auto precheckMeasuredSet(const MachineLayout& machine,
                         std::vector<std::uint32_t> planned)
    -> PrecheckMeasuredSet
{
  PrecheckMeasuredSet set;
  std::ranges::sort(planned);
  planned.erase(std::unique(planned.begin(), planned.end()), planned.end());
  set.cpus = planned;
  if (planned.empty()) {
    // A batch with no pinned instance plans no cpu; the caller refuses
    // the run with the standing status-2 line (FR-010).
    return set;
  }
  const IdleResolution idle = resolveIdleCpus(machine, planned);
  auto candidates = idle.cpus;
  std::ranges::sort(candidates);
  set.cpus.insert(set.cpus.end(), candidates.begin(), candidates.end());
  set.skippedGroups = idle.skippedGroups;
  return set;
}

}  // namespace sg::detail
