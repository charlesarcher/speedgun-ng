// ============================================================================
// The group search, over the kernel-call seam (specs/041-counter-probe,
// T012; FR-011 to FR-021, D-4, D-7, F-1 to F-5).
//
// One size of one search opens k copies of one event as one group, enables
// the group, reads the leader once, and closes every descriptor. The pair
// that read answers is the only evidence the size takes: the group fitted
// when it ran for all of the window it was enabled for (F-3). A search that
// never reads that pair answers no count, because it holds no bound to state
// (FR-016, FR-017).
//
// The file issues no syscall of its own. Every kernel call names
// `kernelCalls()` once, on one line, which is what lets a scripted kernel
// answer the grant at 6, the short run at 7, and the refusal at any size the
// unit names (FR-011, FR-073, PC-1).
// ============================================================================

#ifdef __linux__

#  include <cerrno>
#  include <cstddef>
#  include <cstdint>
#  include <vector>

#  include "../detail/group_probe.hpp"

#  include <linux/perf_event.h>
#  include <sys/ioctl.h>
#  include <unistd.h>

#  include "kernel_calls.hpp"
#  include "speedgun-ng/dbc.hpp"

namespace sg::counters::detail
{

namespace
{

// The three words one read of a leader answers: the leader's own count, the
// nanoseconds the group stood enabled, and the nanoseconds it ran (F-3).
constexpr std::size_t kReadWords = 3;

// The binding F-1 to F-3 measured: the calling thread, on whatever cpu it
// runs on. FR-013a states what that shape cannot see, and the probe changes
// no affinity of its own (FR-013).
constexpr int kProbePid = 0;
constexpr int kProbeCpu = -1;

// The attribute of one copy of the searched event, in the shape F-1 to F-5
// measured. The group starts disabled so the enable instant is one instant
// for the whole group. `pinned` stays clear, so a device the host will not
// serve answers no value and the run multiplexes as it does today (FR-013).
auto fillProbeAttr(perf_event_attr& attr,
                   const GroupProbeOptions& options) -> void
{
  attr = perf_event_attr {};
  attr.type = options.pmuType;
  attr.size = sizeof(perf_event_attr);
  attr.config = options.eventConfig;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  attr.disabled = 1;
  attr.pinned = 0;
  attr.read_format =
      PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
}

// What one size of one search learned. Every field is a fact the size
// itself answers, and `fits` is the only one that decides a count (FR-014).
struct SizeVerdict
{
  bool openedAll = false;  ///< every copy of the size answered a descriptor
  bool permission = false;  ///< an open refused with `EACCES` or `EPERM`
  bool readShort = false;  ///< the read answered no full triple
  bool fits = false;  ///< the pair says the size ran its whole window
};

// Searches one size and releases everything it opened (FR-019). The enable
// and the disable answer no verdict of its own: an enable the kernel refused
// leaves the enabled word at zero, and the pair below answers that size as
// one that did not run (FR-014).
auto searchOneSize(const GroupProbeOptions& options,
                   const std::size_t size) -> SizeVerdict
{
  const auto& calls = kernelCalls();
  auto opened = std::vector<int> {};
  opened.reserve(size);
  auto verdict = SizeVerdict {};

  for (std::size_t copy = 0; copy < size; ++copy) {
    auto attr = perf_event_attr {};
    fillProbeAttr(attr, options);
    // The first copy leads the group and the rest name it, which is the one
    // shape FR-012 states for a size of k copies.
    const int leader = opened.empty() ? -1 : opened.front();
    const long fd = calls.openEvent(
        &attr, kProbePid, kProbeCpu, leader, PERF_FLAG_FD_CLOEXEC);
    if (fd < 0) {
      verdict.permission = errno == EACCES || errno == EPERM;
      break;
    }
    opened.push_back(static_cast<int>(fd));
  }
  verdict.openedAll = opened.size() == size;

  if (verdict.openedAll) {
    auto words = std::vector<std::uint64_t> {};
    words.assign(kReadWords, 0);
    calls.ioctlEvent(
        opened.front(), PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    // One read settles the size: the count, the enabled window, and the
    // running window, in the order the format names them (FR-014).
    const long returned = calls.readGroup(
        opened.front(), words.data(), kReadWords * sizeof(std::uint64_t));
    calls.ioctlEvent(
        opened.front(), PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    // Both sides of this compare are a byte count, and the cast names the
    // one signed type they settle in.
    // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
    verdict.readShort =
        returned < static_cast<long>(words.size() * sizeof(std::uint64_t));
    if (!verdict.readShort) {
      verdict.fits = words[1] > 0 && words[2] == words[1];
    }
  }

  for (const int fd : opened) {
    calls.closeFd(fd);
  }
  return verdict;
}

}  // namespace

auto probeGroupLimit(const GroupProbeOptions& options) noexcept
    -> GroupProbeAnswer
{
  for (std::size_t size = 1; size <= kGroupProbeMaxSize; ++size) {
    const auto verdict = searchOneSize(options, size);
    if (verdict.permission) {
      // The host gate refused the search, so it holds no bound at all and no
      // run of it may report a limit (FR-018).
      return GroupProbeAnswer {.count = std::nullopt,
                               .status = GroupProbeStatus::PERMISSION_REFUSED};
    }
    if (!verdict.fits) {
      // The search stopped here, and the size before it is the bound. The
      // reason states what the stopped size answered, which is what a reader
      // of the disclosure reads (FR-015, FR-050).
      auto status = GroupProbeStatus::RUNS_SHORT;
      if (verdict.readShort) {
        status = GroupProbeStatus::READ_SHORT;
      } else if (!verdict.openedAll) {
        status = GroupProbeStatus::OPEN_REFUSED;
      }
      // A size 1 that did not run leaves no bound below it (FR-017).
      return GroupProbeAnswer {.count = size > 1
                                   ? std::optional<std::size_t> {size - 1}
                                   : std::optional<std::size_t> {},
                               .status = status};
    }
  }
  // Every size to the cap ran whole, so the device gave the search no bound
  // to state, and no run reports a limit of 32 from here (FR-016).
  return GroupProbeAnswer {.count = std::nullopt,
                           .status = GroupProbeStatus::CAP_REACHED};
}

}  // namespace sg::counters::detail

#endif  // __linux__
