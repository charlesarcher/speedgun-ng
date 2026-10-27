#ifndef SG_HARNESS_JITTER_PROBE_HPP
#define SG_HARNESS_JITTER_PROBE_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
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

/// @brief One kept gap, 16 bytes, two unsigned fields (E-04, D-6).
struct GapRecord
{
  std::uint64_t stamp = 0;
  std::uint64_t gapNs = 0;
};

static_assert(sizeof(GapRecord) == 16, "one kept gap record is 16 bytes");

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
/// opens (FR-009, D-6). The stop flag stands last in the field list,
/// inside its own `alignas` block, so the one read per loop pass
/// shares no cache line with a field the runner writes (FR-005, R-04).
struct ProbeCpuState
{
  std::uint32_t cpu = 0;
  std::vector<GapRecord> buffer;
  std::size_t kept = 0;
  std::uint64_t droppedGaps = 0;
  std::uint64_t droppedTotalNs = 0;
  std::uint64_t droppedMaxNs = 0;
  std::uint64_t loopPeriodNs = 0;
  std::uint64_t thresholdNs = 0;
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

}  // namespace sg::detail

#endif
