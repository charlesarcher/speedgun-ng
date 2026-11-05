// ============================================================================
// The group search that answers one question: how many copies of one event
// does one device schedule at a time (specs/041-counter-probe, T011; FR-010
// to FR-021, D-4, D-7, D-9, and the measured readings F-1 to F-5).
//
// A device whose limit no decode answers publishes no limit, and the pass
// planner seats one counter per pass for it. The search asks the kernel
// instead. It opens one group of k copies, enables it, reads the leader
// once, and stops at the first size that did not run for the whole window it
// was enabled for. F-3 states that test, and one enabled read settles it
// (FR-014). Sizes run from 1 upward, and 32 is the cap (D-4, FR-016).
//
// Every kernel call goes through the seam of `kernel_calls.hpp` and nowhere
// else, so a scripted kernel drives every arm on a host that refuses
// `perf_event_open` (FR-011, FR-073, PC-1). The search holds no timer and
// reads no clock: the window the kernel reports is the whole measurement
// (FR-021). FR-010 names the options struct and the device together, and the
// device travels inside that struct as `deviceName` and `pmuType`.
// ============================================================================

#ifndef SG_COUNTERS_DETAIL_GROUP_PROBE_HPP
#define SG_COUNTERS_DETAIL_GROUP_PROBE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace sg::counters::detail
{

/// The largest size the search tries, which D-4 states and FR-016 caps.
inline constexpr std::size_t kGroupProbeMaxSize = 32;

/// Why one search stopped, the closed set FR-015 to FR-018 settle on.
///
/// Every arm that stops past size 1 answers the same count. The reason states
/// what the search met at the stopped size, which is what the caller hands a
/// reader. `contracts/group-probe.md` names the scenario that reaches each
/// value.
enum class GroupProbeStatus
{
  OPEN_REFUSED,  ///< an open refused at this size, and the size before it fits
  RUNS_SHORT,  ///< this size ran below its enabled window, which F-1 measured
  READ_SHORT,  ///< the read answered no full pair, so nothing is known
  PERMISSION_REFUSED,  ///< `EACCES` or `EPERM` refused the first open (FR-018)
  CAP_REACHED,  ///< every size to 32 fit, so no bound is known (FR-016)
};

/// One search's input (FR-010). No field is a bare `bool`, and no field
/// states a size: the cap is `kGroupProbeMaxSize` alone, so a caller cannot
/// widen the search FR-016 caps.
struct GroupProbeOptions
{
  std::string deviceName;  ///< the device the answer is about
  std::uint32_t pmuType {};  ///< the device's own type number
  std::uint64_t eventConfig {};  ///< the config word the event rule named
};

/// One search's answer: the count of copies the device schedules together, or
/// nothing, beside the reason the search stopped.
struct GroupProbeAnswer
{
  std::optional<std::size_t> count {};  ///< the largest size that fit
  GroupProbeStatus status {GroupProbeStatus::OPEN_REFUSED};
};

/// Searches one device for the count of copies of one event it schedules at
/// one time (FR-010, FR-015).
///
/// The search runs one size at a time, smallest first. One size opens `k`
/// copies through the seam, the first as the group leader and the rest
/// naming that leader as their `group_fd` (FR-012); the attribute carries
/// `exclude_kernel`, starts the group disabled and clears `pinned`, and the
/// open binds the calling thread on any cpu (FR-013); the leader enables the
/// group, answers one read, and releases every descriptor the size opened
/// (FR-014, FR-019). A size fits when that one read carries
/// `time_running == time_enabled` with `time_enabled` past zero (F-3). The
/// open result alone decides nothing (FR-014).
///
/// \param options The device, its type number, and the event to search with.
/// \pre none. An empty `deviceName` or a type no device publishes answers a
///      refusal, and the search states that refusal as its answer.
/// \post The answer carries a count exactly when a size fitted and a later
///       size did not: the count is the largest size that fitted (FR-015).
/// \post A search that reaches the cap still fitting answers no count
///       (FR-016), and a size 1 that does not fit answers no count (FR-017).
/// \post A refusal of `EACCES` or `EPERM` answers no count with
///       `PERMISSION_REFUSED` (FR-018).
/// \post Every descriptor the search opened is closed on every path, the
///       refusals included (FR-019).
/// \post No path sleeps, spins, or reads a clock (FR-021).
[[nodiscard]] auto probeGroupLimit(const GroupProbeOptions& options) noexcept
    -> GroupProbeAnswer;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_DETAIL_GROUP_PROBE_HPP
