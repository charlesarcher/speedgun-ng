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
/// The stop flag stands last in the field list, inside its own `alignas`
/// block, so the one read per loop pass shares no cache line with a
/// field the runner writes (FR-005, R-04).
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
  std::uint64_t loopPeriodNs = 0;
  std::uint64_t thresholdNs = 0;
  std::uint64_t workCount = 0;
  std::uint64_t unitTimeNs = 0;
  std::uint64_t targetNs = 0;
  std::uint64_t quantumNs = 0;
  std::uint64_t calibrationSpanNs = 0;
  std::vector<std::uint64_t> series;
  std::size_t seriesKept = 0;
  std::uint64_t droppedSamples = 0;
  // specs/043 FR-020, R-11, E-26: the stamp the first series store
  // wrote, the base the window fold recovers each sample's stamp from.
  std::uint64_t seriesStartNs = 0;
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
  /// precheck and both modes, past its pre-check phase, with the
  /// precheck mode stopping and joining the probe before the return
  /// (FR-006, FR-009, FR-014, R-01).
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

/// @brief specs/043-fwq-ftq FR-019: the console spelling of one method,
/// the lower-case words the value takes: `tsc`, `fwq` and `ftq`. The
/// JSON writer keeps its own upper-case table beside its own keys, and
/// the console block and the calibration lines read this one, so the two
/// console surfaces spell the method alike.
///
/// \pre none
/// \post the answer names the word of the enumerator
auto jitterMethodName(JitterMethod method) -> const char*;

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
