#ifndef SG_HARNESS_JITTER_PROBE_HPP
#define SG_HARNESS_JITTER_PROBE_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_provider.hpp"

/**
 * @file jitter_probe.hpp
 * @brief The jitter probe internals behind the public surface: the
 * batch-scoped holder, the per-cpu states, the gap record, the timed
 * windows, the fold, and the reader-install seam. Nothing here is
 * public API (FR-015, R-01).
 */

namespace sg::detail
{

// E-04, R-04: the cache line the stop flag owns. A fixed 64 stands in
// for `std::hardware_destructive_interference_size`, whose value can
// vary with the tuning flags the build names.
inline constexpr std::size_t kCacheLineBytes = 64;

// The measurement pass's read count, one per armed probe cpu (E-05,
// R-05): the constant stands here, beside the probe the pass belongs
// to, so the probe and the test that drives its gates spell the value
// once.
inline constexpr std::uint64_t kPassReads = 1024;

/// @brief One kept gap, 16 bytes, two unsigned fields (E-04, D-6).
struct GapRecord
{
  std::uint64_t stamp = 0;
  std::uint64_t gapNs = 0;
};

static_assert(sizeof(GapRecord) == 16, "one kept gap record is 16 bytes");

/// @brief specs/043-fwq-ftq: one kept series sample, 8 bytes, one
/// unsigned field: the unit time in ns for FWQ, the count for FTQ
/// (E-26, FR-014, FR-016, D-6).
struct SeriesRecord
{
  std::uint64_t value = 0;
};

static_assert(sizeof(SeriesRecord) == 8,
              "one kept series sample is 8 bytes (E-26, D-6)");

/// @brief One timed window: the half-open TSC interval one benchmark
/// attempt covers, with the indexes that name the attempt (E-06, R-04).
struct TimedWindow
{
  std::uint64_t start = 0;
  std::uint64_t end = 0;
  std::size_t entry = 0;
  std::uint64_t repetition = 0;
  std::uint64_t chunk = 0;
  std::uint64_t pass = 0;
};

/// @brief specs/048-hwlat-probe D-7, P-3, E-02: the counter reads that
/// bound one window, one pair per source. The probe reads each source
/// once at the opening edge of a window and once at its closing edge, and
/// the change of the pair answers the change across the window (FR-012,
/// FR-016). A source that stands absent, unreadable or unparsable leaves
/// its pair with no value at both edges, and no figure stands in its
/// place (FR-011, FR-015).
struct WindowEdges
{
  std::optional<std::uint64_t> smiOpening;
  std::optional<std::uint64_t> smiClosing;
  std::optional<std::uint64_t> nmiOpening;
  std::optional<std::uint64_t> nmiClosing;
};

/// @brief specs/045-merge-fixes FR-008, D-5: the latch-time snapshot of
/// one probe cpu: the figures the mid-run fold reads, copied at the
/// latch line while the alongside loop runs on. The work item builds one
/// per probe cpu once `seriesKeptAtLatch` stands stored there, and
/// publishes it with one release store of the flag beside it. The
/// pre-check record of a `mode=both` run and the method noise gate read
/// this object, so no mid-run fold reads a live buffer (FR-008, R-04).
/// The copy holds the fields the folds read: the series prefix the
/// pre-check window kept with its kept count and the count of samples a
/// full buffer dropped, the gap prefix with its kept count and the count
/// of gaps a full buffer dropped, the change prefix beside that gap
/// prefix (specs/048 E-03), the two counter reads that bound the pre-check
/// window (specs/048 FR-016), the effective threshold and loop
/// period, the five calibration figures of E-22, and the method. Each
/// method fills one prefix: FWQ and FTQ fill the series, and a `tsc` run
/// fills the gaps.
struct JitterLatchSnapshot
{
  std::vector<std::uint64_t> series;
  std::size_t seriesKept = 0;
  std::uint64_t droppedSamples = 0;
  std::vector<GapRecord> gaps;
  std::size_t gapsKept = 0;
  std::uint64_t droppedGaps = 0;
  // specs/048-hwlat-probe FR-012, FR-013, P-3, E-03: the change prefix
  // beside the gap prefix, one unsigned change per kept gap of that
  // prefix (D-7). The copied kept count ends both prefixes, so index i of
  // one answers index i of the other and the fold needs no third list.
  std::vector<std::uint64_t> changes;
  // specs/048-hwlat-probe FR-016, P-5: the edge reads of the pre-check
  // window. The record folds this copy while the alongside loop runs on,
  // so the window's own two reads ride the copy and no live buffer stands
  // read mid-run.
  WindowEdges window;
  std::uint64_t thresholdNs = 0;
  std::uint64_t loopPeriodNs = 0;
  std::uint64_t workCount = 0;
  std::uint64_t unitTimeNs = 0;
  std::uint64_t targetNs = 0;
  std::uint64_t quantumNs = 0;
  std::uint64_t calibrationSpanNs = 0;
  JitterMethod method = JitterMethod::TSC;
};

/// @brief The per-cpu probe state: one instance per probe cpu, owned
/// by the holder (E-05, R-03).
///
/// The buffer pre-sizes to `gapCount` records, allocated and first
/// touched on the probe cpu inside the work item, before the window
/// opens (FR-009, D-6). The series buffer of the FWQ and FTQ methods
/// pre-sizes to the same `gapCount` entries, one 8-byte sample each,
/// and the kept count and the count of records a full buffer dropped
/// ride beside it (specs/043 FR-014, FR-016, FR-022, E-26, D-6). The
/// five calibration figures of E-22 stand beside `loopPeriodNs` and
/// `thresholdNs`, the values the search measured on this cpu before its
/// window (specs/043 FR-010, FR-011, E-22). The method names the run's
/// selection, the figure that decides which noise fraction the series
/// fold reads (specs/043 FR-009, R-10), and `seriesStartNs` carries the
/// stamp the first series store wrote, the base the window fold
/// recovers every sample's stamp from (specs/043 FR-020, R-11, E-26).
/// The change ring of specs/048 stands beside the gap ring with the same
/// capacity, one 8-byte count per kept gap (E-03, P-3), and the two
/// counter edge reads of this cpu's run span stand beside the series
/// figures (specs/048 FR-016, E-02). The stop flag stands last in the
/// field list, inside its own `alignas` block, so the one read per loop
/// pass shares no cache line with a field the runner writes (FR-005,
/// R-04).
struct ProbeCpuState
{
  std::uint32_t cpu = 0;
  // specs/043 FR-009, R-10: the method the run selected, so the series
  // fold reads the noise fraction of the method that filled the buffer.
  JitterMethod method = JitterMethod::TSC;
  std::vector<GapRecord> buffer;
  std::size_t kept = 0;
  std::uint64_t droppedGaps = 0;
  std::uint64_t droppedTotalNs = 0;
  std::uint64_t droppedMaxNs = 0;
  // specs/048-hwlat-probe FR-012, FR-013, D-7, P-3, E-03: the change ring
  // beside the gap ring, one unsigned change per kept gap. One element
  // holds this gap's counter read minus the read before it: the opening
  // edge read for the first gap of the window, and the previous gap's read
  // for every later gap (D-7). The ring pre-sizes to the same `gapCount`
  // entries, and the kept count and the count of records a full buffer
  // dropped above serve both rings, so index i of one answers index i of
  // the other (P-3). The gap path stores record and change in one step, so
  // a dropped gap stands in neither ring and raises no second counter
  // (FR-012).
  std::vector<std::uint64_t> changes;
  std::uint64_t loopPeriodNs = 0;
  std::uint64_t thresholdNs = 0;
  std::uint64_t workCount = 0;
  std::uint64_t unitTimeNs = 0;
  std::uint64_t targetNs = 0;
  std::uint64_t quantumNs = 0;
  std::uint64_t calibrationSpanNs = 0;
  // specs/043 FR-010, FR-011, D-2: a `work=` count the value names skips
  // the search, so the unit time the run discloses reads the first unit
  // of this cpu's window. The flag stands set until that unit fills it.
  bool unitTimeFromWindow = false;
  std::vector<std::uint64_t> series;
  std::size_t seriesKept = 0;
  // specs/043 FR-004, FR-020, E-27, R-04: the kept count the series
  // buffer stood at as the item passed the latch line, the end of the
  // pre-check phase. The buffer fills by index and drops what exceeds
  // it, so the samples below this count stand written and never written
  // again: the pre-check record of a `mode=both` run folds this prefix
  // while the alongside loop runs on, and no read of the live buffer
  // stands needed.
  std::size_t seriesKeptAtLatch = 0;
  std::uint64_t droppedSamples = 0;
  // specs/043 FR-020, R-11, E-26: the stamp the first series store
  // wrote, the base the window fold recovers each sample's stamp from.
  std::uint64_t seriesStartNs = 0;
  // specs/048-hwlat-probe FR-016, P-5: the edge reads of this cpu's own
  // run span, the read behind the latch and the read after the loop. The
  // run-level fold answers `smiDelta` and `nmiDelta` from this pair, and a
  // source that stood unavailable leaves its pair with no value at both
  // edges (FR-011, FR-016).
  WindowEdges edges;
  // specs/045-merge-fixes FR-008, D-5: the latch-time snapshot and its
  // publish flag. The item fills the snapshot at the latch line and
  // release-stores the flag there, and the mid-run fold sites
  // acquire-load the flag and read the snapshot while the alongside loop
  // runs on. The flag stands off the loop's read path, which the stop
  // flag below owns.
  JitterLatchSnapshot latch;
  std::atomic<bool> latchPublished {false};
  alignas(kCacheLineBytes) std::atomic<bool> stop {false};
};

/// @brief The batch-scoped probe holder: one instance per run, built
/// beside the batch state, started before the first run, and stopped,
/// drained and joined after the last (R-01, FR-006, D-7).
class JitterProbe
{
public:
  /// @brief Build the holder over the options; the probe cpus resolve
  /// against the plans and the layout at start (E-01, R-01, R-07).
  explicit JitterProbe(const JitterProbeOptions& options);

  /// @brief Start the probe contexts; nothing runs before this call,
  /// and the call returns only once every probe cpu stands past its
  /// buffer's first touch and its measurement pass and, in the
  /// precheck and both modes, past its pre-check phase, and past the
  /// latch-time snapshot it publishes at the latch line, with the
  /// precheck mode stopping and joining the probe before the return
  /// (FR-006, FR-008, FR-009, FR-014, R-01).
  auto start() -> void;

  /// @brief Set every probe cpu's stop flag; the loop's own read of the
  /// flag ends the work item, and the drain and the join stand in
  /// `stopAndJoin` (FR-006, D-7, R-04). The scripted cases set the flag
  /// behind the clock's park, so the window ends at its planned read
  /// (FR-012, FR-016).
  auto stop() noexcept -> void;

  /// @brief Stop, drain and join every probe context; the runner reads
  /// the buffers only after this call returns (FR-006, D-7, R-04).
  auto stopAndJoin() -> void;

  /// @brief One state per probe cpu (E-05).
  [[nodiscard]] auto states() const noexcept
      -> const std::vector<ProbeCpuState>&;

private:
  JitterProbeOptions m_options;
  std::vector<ProbeCpuState> m_states;
};

/// @brief The run-level fold: the whole kept set of one probe cpu
/// (E-03, R-15, contracts/public-api.md). The caller reads the buffer
/// only after the probe context is joined (R-04). That ordering is a
/// happens-before fact between threads. The state carries no field a
/// runtime check could read, so the contract stands as prose and no
/// `SG_REQUIRE` pairs with it.
///
/// \post the percentile fields follow the nearest-rank rule of
///       `source/harness/percentiles.cpp` over the sorted kept gaps
///       (PC-4, R-05)
/// \post minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <=
///       maxNs
/// \post `gapCount` counts the kept gaps, and `droppedGaps` counts the
///       gaps the buffer dropped (FR-010, D-6)
/// \post `smiGapCount` counts the kept gaps whose change element stands
///       above zero, and a change of zero marks its gap not SMI. A change ring
///       shorter than the kept set leaves the gaps it does not reach unmarked
///       (FR-014, FR-015, P-3)
/// \post `smiDelta` and `nmiDelta` answer the change of the pair of edge
///       reads of this cpu's run span, and stand with no value when
///       either read of that source stands absent (FR-011, FR-016)
[[nodiscard]] auto gapStatsOf(const ProbeCpuState& state) -> JitterStats;

/// @brief The per-benchmark fold: the kept gaps of one probe cpu
/// inside the timed windows of one benchmark entry (FR-021, E-06).
/// The caller reads the buffer only after the probe context is
/// joined (R-04). That ordering is a happens-before fact between
/// threads. The state carries no field a runtime check could read,
/// so the contract stands as prose and no `SG_REQUIRE` pairs with
/// it.
///
/// \post a kept gap folds into every window whose half-open
///       `[start, end)` contains its stamp, and the fields carry the
///       contracts the run-level fold states (E-06, R-04)
/// \post `smiGapCount` counts the placed gaps whose change element stands
///       above zero, and the gaps a change ring shorter than the kept set does
///       not reach stand unmarked. `smiDelta` and `nmiDelta` stand with no
///       value, because this fold holds no window edge (FR-015, FR-016, FR-025,
///       P-3)
[[nodiscard]] auto gapStatsOf(const ProbeCpuState& state,
                              std::span<const TimedWindow> windows)
    -> JitterStats;

/// @brief specs/043-fwq-ftq: the run-level series fold of one probe
/// cpu: the figures of FR-007 over the kept samples, the five
/// calibration figures of E-22, and the noise fraction of the method
/// that filled the buffer (FR-007, FR-008, FR-009, R-09, R-10). The
/// caller reads the series buffer only after the probe context is
/// joined (R-04). That ordering is a happens-before fact between
/// threads. The state carries no field a runtime check could read, so
/// the contract stands as prose and no `SG_REQUIRE` pairs with it.
///
/// \post the percentile fields follow the nearest-rank rule of
///       `source/harness/percentiles.cpp` over the kept samples read
///       in place, and the fold copies no buffer (FR-008)
/// \post minValue <= medianValue <= p90Value <= p99Value <= p999Value
///       <= p9999Value <= maxValue
/// \post `sampleCount` counts the kept samples and `droppedSamples`
///       counts the samples the full buffer dropped (FR-007, FR-022)
/// \post the FWQ fraction divides the kept unit times' sum, minus the
///       sample count times the minimum unit time, by that sum, times
///       100, and the FTQ fraction divides the sample count times the
///       maximum count, minus the counts' sum, by that product, times
///       100, a maximum count of 0 answering 100 (FR-009, R-10)
/// \post `seriesRuntimeNs` is the span the kept samples' recovered
///       stamps cover, the calibration span out (FR-007, FR-010)
/// \post an empty kept set leaves the series figures at 0 (FR-008)
[[nodiscard]] auto seriesStatsOf(const ProbeCpuState& state)
    -> JitterSeriesStats;

/// @brief specs/043-fwq-ftq FR-004, FR-020, E-27, R-04: the series fold
/// of one probe cpu over a prefix of its kept samples: the figures of
/// the first `prefixCount` samples of the series buffer. The buffer
/// fills by index and drops what exceeds it, so the prefix stands
/// written and never written again. The caller reads the state only
/// behind the join edge, the standing happens-before rule (R-04).
///
/// \pre `prefixCount` stands at or below `state.seriesKept`
/// \post the fold answers the figures of the first `prefixCount`
///       samples, the calibration figures and the drop count carrying
///       the state's own (FR-007, FR-022)
/// \post minValue <= medianValue <= p90Value <= p99Value <= p999Value
///       <= p9999Value <= maxValue
[[nodiscard]] auto seriesStatsOf(const ProbeCpuState& state,
                                 std::size_t prefixCount) -> JitterSeriesStats;

/// @brief specs/045-merge-fixes FR-008, D-5: the series fold over one
/// probe cpu's latch-time snapshot: the same `JitterSeriesStats` the
/// state folds answer, over the prefix the latch copied. The pre-check
/// record of a `mode=both` run and the method noise gate read this fold,
/// the alongside loop running on, and the fold reads no live buffer
/// (FR-004, FR-020, E-27).
///
/// \pre `seriesKept` equals the length of the copied series prefix
/// \post the fold answers the figures of the copied prefix, the
///       calibration figures and the drop count carrying the snapshot's
///       own (FR-007, FR-022)
/// \post minValue <= medianValue <= p90Value <= p99Value <= p999Value
///       <= p9999Value <= maxValue
/// \post `seriesRuntimeNs` is the span the copied prefix's recovered
///       stamps cover, the calibration span out (FR-007, FR-010)
/// \post an empty prefix leaves the series figures at 0 (FR-008)
[[nodiscard]] auto seriesStatsOf(const JitterLatchSnapshot& snapshot)
    -> JitterSeriesStats;

/// @brief specs/045-merge-fixes FR-008, D-5: the gap fold over one probe
/// cpu's latch-time snapshot: the same `JitterStats` the run-level fold
/// answers, over the gap prefix the latch copied. The pre-check record
/// of a `mode=both` run reads this fold for every method, the alongside
/// loop running on, and the fold reads no live buffer. The `hwlat`
/// method reads the same fold through the same record.
///
/// \pre `gapsKept` equals the length of the copied gap prefix
/// \post the fold answers the figures of the copied prefix, the
///       threshold, the loop period and the drop count carrying the
///       snapshot's own (FR-010, D-6)
/// \post `smiGapCount` counts the copied gaps whose change element stands
///       above zero, and a copied change ring shorter than the copied prefix
///       leaves the gaps it does not reach unmarked (FR-014, FR-015, P-3)
/// \post `smiDelta` and `nmiDelta` answer the change of the snapshot's
///       pair of edge reads, the two reads of the pre-check window, and
///       stand with no value when either read of that source stands
///       absent (FR-011, FR-016)
/// \post minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <=
///       maxNs
/// \post `runtimeNs` is the copied records' stamp span, and the rate
///       fields divide by it (E-03, R-05)
[[nodiscard]] auto gapStatsOf(const JitterLatchSnapshot& snapshot)
    -> JitterStats;

/// @brief specs/045-merge-fixes FR-008, D-5: the published latch-time
/// snapshot of one probe cpu. The call acquire-loads the publish flag
/// the item release-stores at the latch line, then answers the snapshot
/// that store published. The fold sites of a running batch read through
/// this call, and the read touches no field the alongside loop writes.
///
/// \pre the holder's `start` returned, so every item stands past its
///       latch arrival and past its snapshot publish (FR-006, FR-008)
/// \post the answer is the snapshot the item built at the latch line
[[nodiscard]] auto latchSnapshotOf(const ProbeCpuState& state)
    -> const JitterLatchSnapshot&;

/// @brief specs/043-fwq-ftq: the per-benchmark series fold: the kept
/// samples of one probe cpu whose recovered stamp falls inside the
/// half-open timed windows of one benchmark entry (FR-020, R-11). The
/// caller reads the series buffer only after the probe context is
/// joined (R-04), the standing happens-before rule. The fold reads the
/// one series buffer the probe cpu owns, so no second buffer joins
/// (FR-020, R-11).
///
/// \post the fold recovers each sample's stamp from
///       `seriesStartNs`, the stamp the first series store wrote: the
///       FWQ stamp is that start plus the running sum of the unit
///       times, and the FTQ stamp that start plus the sample rank plus
///       one, times the quantum (R-11, E-26)
/// \post a kept sample folds into every window whose half-open
///       `[start, end)` contains its recovered stamp, and the fields
///       carry the contracts the run-level fold states (E-06, R-04)
/// \post `seriesRuntimeNs` is the windows' total duration (E-03, R-05)
/// \post the fold stands absent when the windows place no sample
///       (FR-020)
[[nodiscard]] auto seriesStatsOf(const ProbeCpuState& state,
                                 std::span<const TimedWindow> windows)
    -> std::optional<JitterSeriesStats>;

/// @brief specs/043-fwq-ftq FR-021, D-9, R-15: the planned cpus whose
/// pre-check window kept a noise fraction at or above the `noise=`
/// threshold, in the order the planned list names. The gate reads the
/// method's own figure: a FWQ window and a FTQ window keep no gaps, so
/// the standing gap rule reads nothing there. A `tsc` window and a `hwlat`
/// window keep no series, and a cpu the states do not name stands
/// unjudged. The fold
/// reads each cpu's latch-time snapshot, the figures the pre-check
/// window kept, so the gate stands race-free beside a running
/// alongside loop (specs/045-merge-fixes FR-008, D-5).
///
/// \pre none
/// \post the answer names each planned cpu at or above the threshold,
///       in the order the planned list names
[[nodiscard]] auto precheckNoisyCpus(
    const std::vector<ProbeCpuState>& states,
    const std::vector<std::uint32_t>& plannedCpus,
    std::uint32_t noisePercent) -> std::vector<std::uint32_t>;

/// @brief specs/043-fwq-ftq FR-019, specs/048 FR-002, D-1: the console
/// spelling of one method, the lower-case words the value takes: `tsc`, `fwq`,
/// `ftq` and `hwlat`. The JSON writer keeps its own upper-case table beside
/// its own keys, and the console block and the calibration lines read this
/// one, so the two
/// console surfaces spell the method alike.
///
/// \pre none
/// \post the answer names the word of the enumerator
auto jitterMethodName(JitterMethod method) -> const char*;
///
/// @brief specs/043 FR-003, D-1, specs/048 D-1: the methods that fill the
/// series ring, one sample per entry. The two calibrated methods fill it, and
/// `tsc` and `hwlat` fill the gap ring alone, so neither calibrates a unit loop
/// nor pre-sizes the series buffer (E-05, A-4). The report, the console and the
/// noise fraction read this answer, so one rule stands behind all three.
///
/// \pre none
/// \post the answer names the methods that carry a series
auto fillsSeriesRing(JitterMethod method) -> bool;

///
/// @brief specs/048 P-3, E-03, FR-015: the change prefix of one kept set. The
/// keep step pairs the gap ring and the change ring element for element, so a
/// state whose change ring stands as long as its kept set answers that prefix
/// whole. A state built beside the item, the shape a fixture gives a fold,
/// carries a shorter ring, and the gaps that ring does not reach stand
/// unmarked, the answer an absent source leaves. The report reads the same
/// prefix the fold counts its marks from, so the marks of a document pair with
/// its gaps index for index.
///
/// \pre `kept` counts the kept gaps of the state `changes` rode beside
/// \post the elements of `changes` up to the shorter of `kept` and its own
///       length, in ring order
auto changePrefixOf(const std::vector<std::uint64_t>& changes,
                    std::size_t kept) -> std::span<const std::uint64_t>;

/// @brief specs/043-fwq-ftq FR-017, FR-018, FR-022: the spectrum keys of
/// one cpu's series record: the transform reads the FTQ count series at
/// the quantum, or the FWQ unit times resampled to the median unit time,
/// and the record gains the resample step, the point count and the
/// period list. The console block and the JSON record share this one
/// reading, so the two surfaces name the same periods (FR-019, R-12).
///
/// \pre `series` carries the figures of `state`'s kept set
/// \post a series with no sample, a FWQ median of 0 and a FTQ quantum of
///       0 leave the transform unread, and the record keeps 0 points and
///       an empty period list (FR-017, FR-018, R-12)
/// \post the point count names the span the transform read, and the
///       period list stands sorted by power, strongest first (FR-018)
auto addSpectrumTo(const ProbeCpuState& state,
                   JitterSeriesStats& series) -> void;

/// @brief The reader-install seam: a reader installed here stands in
/// place of the one the probe opens over the `machine/tsc` leaf, and
/// the seam installs a counters `WindowReader`; no second time source
/// arrives through it (R-02, FR-004). A null pointer clears the
/// installed one, the way `installTestLayout` clears with an empty
/// optional (R-02, Q-5).
auto installTestReader(std::unique_ptr<counters::WindowReader> reader) -> void;

/// @brief The value grammar of contracts/cli-options.md (FR-011,
/// FR-014): the head names a cpu list or the word `idle`, and the keys
/// follow, each standing once in the value, a cpu standing once in any
/// form the tokens name. Every rejection names the token it read,
/// quoted the way the option module quotes a value, so the caller's
/// one line carries it (FR-038). Declared beside the probe's surface
/// because the option file's settle arm parses the value where it
/// settles, with the origin the settle knows (FR-027, R-06).
[[nodiscard]] auto parseJitterProbeValue(const std::string& value,
                                         JitterProbeOptions& parsed,
                                         std::string& message) -> bool;

/// @brief The cpu token of the value grammars: an index or a closed
/// `a-b` range, the form the topology listing prints; each cpu stands
/// in the process affinity mask (FR-011, FR-038) and in the online cpu
/// set the topology seam owns, and the reader pushes every cpu of a
/// range in ascending order. The os-noise grammar reads its `cpus`
/// list through this one reader, so no second cpu-list reader stands
/// (specs/036 R-06). Every rejection names the token it read, quoted
/// the way the option module quotes a value, so the caller's one line
/// carries it (FR-038). Declared beside the probe's parser because
/// both value grammars read the standing cpu-list rules (specs/036
/// FR-002, R-06).
[[nodiscard]] auto parseProbeCpuToken(const std::string& token,
                                      std::vector<std::uint32_t>& cpus,
                                      std::string& message) -> bool;

/// @brief specs/034 R-01: the measured set of one pre-check run: the
/// planned cpus ascending, then the candidates ascending, the
/// candidates the standing `idle` resolution takes over the plans
/// (specs/027 D-4), one candidate per L3 group the plans touch and no
/// candidate a planned cpu or an SMT sibling of one names.
///
/// \pre `machine` stands readable (specs/027 FR-020)
/// \post `cpus` carries `planned` ascending and deduplicated, then
///       the resolution's cpus ascending, and no cpu stands twice in
///       the list (FR-010, R-01)
/// \post `skippedGroups` carries the touched groups that name no free
///       cpu, ascending (specs/027 FR-017)
/// \post a `planned` list that names no cpu answers an empty list,
///       the caller's status-2 condition (FR-010)
struct PrecheckMeasuredSet
{
  std::vector<std::uint32_t> cpus;
  std::vector<std::uint32_t> skippedGroups;
};

struct MachineLayout;
[[nodiscard]] auto precheckMeasuredSet(const MachineLayout& machine,
                                       std::vector<std::uint32_t> planned)
    -> PrecheckMeasuredSet;

/// @brief specs/034 E-12: the raw counts one attribution source
/// carries for one measured cpu, read before and after the window
/// (R-03); the deltas the record carries are the after-before
/// differences of these figures.
struct PrecheckCpuSnapshot
{
  std::optional<std::uint64_t> interrupts;
  std::vector<std::pair<std::string, std::optional<std::uint64_t>>> softirqs;
  std::optional<std::uint64_t> switches;
  std::optional<std::uint64_t> schedules;
};

/// @brief specs/034 R-03: the raw counts the attribution sources
/// carry for the measured cpus, read through the file-root seam
/// (FR-019): the interrupt count of each cpu (the sum of its column
/// over the rows of `/proc/interrupts`), a count per softirq kind the
/// file states (`/proc/softirqs`), and the context-switch and schedule
/// counts (`/proc/schedstat`, the run-queue delay figure and the
/// timeslices figure). A source that is missing, unreadable or
/// unparsable stands absent for that cpu, and the run never fails on
/// it (FR-018, AC-b2).
///
/// \pre `cpus` names no cpu twice
/// \post the answer carries one entry per named cpu
[[nodiscard]] auto precheckReadSnapshot(const std::vector<std::uint32_t>& cpus)
    -> std::map<std::uint32_t, PrecheckCpuSnapshot>;

/// @brief specs/034 R-04: the host settings the run discloses once,
/// read through the file-root seam (FR-019): the kernel command
/// line's `isolcpus`, `nohz_full` and `rcu_nocbs` values, the cpufreq
/// governor of each measured cpu, and the IRQ numbers whose affinity
/// names each measured cpu, the list form of
/// `/proc/irq/<n>/smp_affinity_list` with the mask form
/// `smp_affinity` as the fallback. A figure the host does not state
/// stands `not available` (FR-017).
///
/// \post the answer carries one governor entry and one affinity
///       entry per named cpu, ascending
[[nodiscard]] auto precheckReadHostSettings(
    const std::vector<std::uint32_t>& cpus) -> PrecheckHostSettings;

/// @brief specs/034 R-03: the window deltas of one cpu's attribution:
/// the after-before difference of each figure, and a figure stands
/// absent when either side stands absent (FR-016, FR-018).
///
/// \post a delta stands at or above zero when both sides stand
[[nodiscard]] auto precheckAttributionDelta(const PrecheckCpuSnapshot& before,
                                            const PrecheckCpuSnapshot& after)
    -> PrecheckAttribution;

/// @brief specs/034 R-05: the noisy planned cpus of one pre-check run:
/// a planned cpu stands noisy when its pre-check window kept at least
/// one above-threshold gap, the standing threshold rule's own record
/// (FR-012, FR-014); a cpu the planned list does not name stands a
/// candidate, and a candidate never stands in the answer (FR-015).
///
/// \pre every state of `states` stands joined (R-04)
/// \post the answer carries the planned cpus ascending, each one a
///       cpu of `states` whose kept gap count stands above zero, and
///       no cpu twice
[[nodiscard]] auto precheckNoisyCpus(
    const std::vector<ProbeCpuState>& states,
    const std::vector<std::uint32_t>& plannedCpus)
    -> std::vector<std::uint32_t>;

}  // namespace sg::detail

#endif
