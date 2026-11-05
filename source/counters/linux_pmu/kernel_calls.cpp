// The kernel-call seam's storage and its real set: the defaults are the
// calls the linux_pmu code issued before the seam stood, so a host the
// kernel serves runs exactly the code it ran before (G-19, G-21). Off
// Linux the real set answers the refusal values the provider's own
// platform guards answer, so the reduced-catalog build compiles from
// the same list (FR-042) and carries no kernel call.

#include "kernel_calls.hpp"

#include <sys/types.h>

#include "pmu_ident.hpp"

#if defined(__linux__)
#  include <linux/perf_event.h>
#  include <sys/ioctl.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#else
#  include <cerrno>
#endif

namespace sg::counters::detail
{

namespace
{

auto realOpenEvent(
    void* attr, int pid, int cpu, int groupFd, unsigned long flags) -> long
{
#if defined(__linux__)
  return ::syscall(
      SYS_perf_event_open, attr, static_cast<pid_t>(pid), cpu, groupFd, flags);
#else
  static_cast<void>(attr);
  static_cast<void>(pid);
  static_cast<void>(cpu);
  static_cast<void>(groupFd);
  static_cast<void>(flags);
  errno = ENOSYS;
  return -1;
#endif
}

auto realMapPage(void* hint,
                 std::size_t length,
                 int prot,
                 int flags,
                 int fd,
                 long long offset) -> void*
{
#if defined(__linux__)
  return ::mmap(hint, length, prot, flags, fd, static_cast<off_t>(offset));
#else
  static_cast<void>(hint);
  static_cast<void>(length);
  static_cast<void>(prot);
  static_cast<void>(flags);
  static_cast<void>(fd);
  static_cast<void>(offset);
  errno = ENOSYS;
  // The value <sys/mman.h> names MAP_FAILED, spelled here because the
  // stub compiles where that header does not stand (FR-042).
  return static_cast<void*>(-1);
#endif
}

auto realUnmapPage(void* addr, std::size_t length) -> int
{
#if defined(__linux__)
  return ::munmap(addr, length);
#else
  static_cast<void>(addr);
  static_cast<void>(length);
  errno = ENOSYS;
  return -1;
#endif
}

auto realReadGroup(int fd, void* buf, std::size_t count) -> long
{
#if defined(__linux__)
  return static_cast<long>(::read(fd, buf, count));
#else
  static_cast<void>(fd);
  static_cast<void>(buf);
  static_cast<void>(count);
  errno = ENOSYS;
  return -1;
#endif
}

auto realIoctlEvent(int fd, unsigned long request, unsigned long arg) -> int
{
#if defined(__linux__)
  return ::ioctl(fd, request, arg);
#else
  static_cast<void>(fd);
  static_cast<void>(request);
  static_cast<void>(arg);
  errno = ENOSYS;
  return -1;
#endif
}

auto realCloseFd(int fd) -> int
{
#if defined(__linux__)
  return ::close(fd);
#else
  static_cast<void>(fd);
  errno = ENOSYS;
  return -1;
#endif
}

auto realDevicesRoot() -> std::filesystem::path
{
  return std::filesystem::path("/sys/bus/event_source/devices");
}

auto realParanoidPath() -> std::filesystem::path
{
  return std::filesystem::path("/proc/sys/kernel/perf_event_paranoid");
}

auto realCpuidWords() -> CpuidWords
{
#if defined(__linux__)
  return readCpuidWords();
#else
  // Off Linux the probe stands in table_parse, which compiles to
  // nothing (FR-042), so the stub answers the no-answer words the
  // decode reads as an absent identity.
  return CpuidWords {};
#endif
}

const KernelCalls kRealCalls {
    .openEvent = realOpenEvent,
    .mapPage = realMapPage,
    .unmapPage = realUnmapPage,
    .readGroup = realReadGroup,
    .ioctlEvent = realIoctlEvent,
    .closeFd = realCloseFd,
    .devicesRoot = realDevicesRoot,
    .paranoidPath = realParanoidPath,
    .cpuidWords = realCpuidWords,
};

const KernelCalls* installed = nullptr;

}  // namespace

auto kernelCalls() -> const KernelCalls&
{
  return installed != nullptr ? *installed : kRealCalls;
}

auto installKernelCalls(const KernelCalls* scripted) -> void
{
  installed = scripted;
}

}  // namespace sg::counters::detail
