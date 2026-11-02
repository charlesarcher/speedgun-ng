// ============================================================================
// The kernel-call seam's own tests: the install and the clear route the
// calls where a test points them, and the real set keeps the kernel's
// errno convention (G-19, G-21; Phase A).
//
// No unit test here needs a granted `perf_event_open`: the real set's
// open test drives a malformed attribute the kernel refuses on every
// host, and the read, map, and close tests run over a scratch file the
// test owns. The scripted set records the plumbing so the round trip is
// visible, and each clear hands the real set back.
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "speedgun-ng/dbc.hpp"  // SG_CONTRACTS_SEMANTIC

namespace
{

auto failures = int {0};

auto check(const bool ok, const char* sentence) -> void
{
  if (!ok) {
    std::printf("KERNEL CALLS TEST FAIL: %s\n", sentence);
    ++failures;
  }
}

/// A scratch file the read, map, and close tests run over, unlinked when
/// the scope ends.
class ScratchFile
{
public:
  ScratchFile()
  {
    m_path = std::filesystem::temp_directory_path()
        / ("kernel-calls-" + std::to_string(::getpid()) + ".scratch");
    std::FILE* handle = std::fopen(m_path.c_str(), "w+b");
    check(handle != nullptr, "the scratch file opens");
    std::fputs("perf-event-seam-scratch", handle);
    std::fflush(handle);
    m_fd = ::open(m_path.c_str(), O_RDONLY);
    std::fclose(handle);
    check(m_fd >= 0, "the scratch file answers a descriptor");
  }

  ~ScratchFile()
  {
    if (m_fd >= 0) {
      ::close(m_fd);
    }
    std::filesystem::remove(m_path);
  }

  ScratchFile(const ScratchFile&) = delete;
  auto operator=(const ScratchFile&) -> void = delete;

  [[nodiscard]] auto fd() const -> int { return m_fd; }

private:
  std::filesystem::path m_path {};
  int m_fd = -1;
};

/// A scripted set the round trip installs: every field answers a fixed
/// value the test states beside the call it drives.
auto scriptedCalls() -> sg::counters::detail::KernelCalls
{
  auto calls = sg::counters::detail::KernelCalls {};
  calls.openEvent =
      [](void*, int pid, int cpu, int groupFd, unsigned long flags) -> long
  { return pid + cpu + groupFd + static_cast<long>(flags) - 41; };
  calls.mapPage = [](void*, std::size_t, int, int, int, long long) -> void*
  {
    static char page;
    return &page;
  };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = [](int fd, void*, std::size_t count) -> long
  { return fd + static_cast<long>(count); };
  calls.ioctlEvent = [](int fd, unsigned long request, unsigned long arg) -> int
  {
    static_cast<void>(fd);
    static_cast<void>(request);
    static_cast<void>(arg);
    return 17;
  };
  calls.closeFd = [](int fd) -> int { return fd; };
  calls.devicesRoot = []() -> std::filesystem::path
  { return "/fixture/devices"; };
  calls.paranoidPath = []() -> std::filesystem::path
  { return "/fixture/paranoid"; };
  return calls;
}

}  // namespace

auto main() -> int
{
  using sg::counters::detail::installKernelCalls;
  using sg::counters::detail::KernelCalls;
  using sg::counters::detail::kernelCalls;

  static_assert(std::is_same_v<decltype(&installKernelCalls),
                               auto (*)(const KernelCalls*)->void>,
                "the install takes one pointer and answers nothing");

  // The real set stands before any install: the path functions answer
  // the paths the host publishes.
  check(kernelCalls().devicesRoot()
            == std::filesystem::path("/sys/bus/event_source/devices"),
        "the real set answers the published device directory");
  check(kernelCalls().paranoidPath()
            == std::filesystem::path("/proc/sys/kernel/perf_event_paranoid"),
        "the real set answers the paranoid control file");

  // A malformed attribute is refused on every host: the size field
  // carries zero, and the kernel answers EINVAL without granting
  // anything, so the errno convention holds where no grant is needed.
  perf_event_attr attr {};
  errno = 0;
  check(kernelCalls().openEvent(&attr, 0, 0, -1, 0UL) < 0,
        "the real open refuses a malformed attribute");
  check(errno != 0, "the real open sets an errno on its refusal");

  // The real read, map, and close run over a scratch file the test
  // owns, so the plumbing is measured without any kernel grant.
  {
    ScratchFile scratch;
    char buffer[8] {};
    const auto got =
        kernelCalls().readGroup(scratch.fd(), buffer, sizeof(buffer));
    check(got == 8, "the real read forwards the count the file answers");
    check(std::string_view(buffer, 4) == "perf",
          "the real read carries the file's own bytes");
    void* mapping = kernelCalls().mapPage(
        nullptr, 4096, PROT_READ, MAP_PRIVATE, scratch.fd(), 0);
    check(mapping != MAP_FAILED, "the real map maps the scratch file");
    if (mapping != MAP_FAILED) {
      check(kernelCalls().unmapPage(mapping, 4096) == 0,
            "the real unmap releases the mapping");
    }
    // The real ioctl on a regular file answers the refusal the kernel
    // gives any file that carries no event interface: -1 with ENOTTY,
    // the errno the seam's scripted refusals stand in for.
    errno = 0;
    check(kernelCalls().ioctlEvent(scratch.fd(), PERF_EVENT_IOC_RESET, 0) == -1,
          "the real ioctl refuses a descriptor that carries no event");
    check(errno == ENOTTY, "the real ioctl sets the kernel's errno");
    check(kernelCalls().closeFd(scratch.fd()) == 0,
          "the real close accepts the descriptor");
  }

  // The scripted set stands over the real one while the install holds,
  // and the clear hands the real set back.
  KernelCalls calls = scriptedCalls();
  installKernelCalls(&calls);
  check(
      kernelCalls().devicesRoot() == std::filesystem::path("/fixture/devices"),
      "the scripted root stands while the install holds");
  check(kernelCalls().readGroup(3, nullptr, 4) == 7,
        "the scripted read answers the recorded value");
  check(kernelCalls().ioctlEvent(2, 10, 5) == 17,
        "the scripted ioctl answers the recorded value");
  installKernelCalls(nullptr);
  check(kernelCalls().devicesRoot()
            == std::filesystem::path("/sys/bus/event_source/devices"),
        "the clear hands the real set back");

  if (failures != 0) {
    return 1;
  }
  std::printf("counters_kernel_calls_test: ok\n");
  return 0;
}
