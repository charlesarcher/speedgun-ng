// ============================================================================
// The kernel-call seam the syscall-mode PMU windows and the provider's
// enumeration run through (G-19, G-21).
//
// The linux_pmu code issues five kernel calls: `perf_event_open` through
// the syscall catalog, `read` on a group leader, `ioctl` on a leader,
// `mmap` and `munmap` of an event page, and `close` of a descriptor. It
// also walks two kernel-published file trees: the device directory under
// `/sys/bus/event_source/devices/` and the
// `/proc/sys/kernel/perf_event_paranoid` control file. A host that
// refuses the syscall and a kernel that publishes one fixed `/sys` tree
// leave the code's refusing and odd-content arms without an input, and
// the markers those arms carried are the exclusions rulings G-19 and
// G-21 remove.
//
// The seam is one options struct of function references, the real calls
// as the values a fresh process carries. A test installs a scripted set
// over it (the grant, the refusal with each errno, the short read, the
// group header whose count sits below the member count, the page with
// and without `cap_user_rdpmc`), drives the windows and the enumeration
// over it, and clears back. Each call site names the seam once, on one
// line, and keeps its arguments in the kernel's own order. The seam
// changes no behavior on a host the kernel serves: every default is the
// call the code issued before.
//
// The struct is the tests' handle alone, and it carries no state. The
// two path functions answer the paths the host publishes unless a test
// stands a fixture directory in their place.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_KERNEL_CALLS_HPP
#define SG_COUNTERS_LINUX_PMU_KERNEL_CALLS_HPP

#include <cstddef>
#include <filesystem>

#include "pmu_ident.hpp"

namespace sg::counters::detail
{

/// The kernel calls and the two kernel-published paths the linux_pmu
/// code runs through, one function reference each, the real calls as
/// the defaults.
///
/// The reference types carry the kernel's own argument order and its
/// errno convention: a negative `long` is a refusal with the errno
/// set, `MAP_FAILED` is a refused mapping, and a short `readGroup`
/// count is a short read. The attribute pointer is untyped so the
/// struct compiles where the perf UAPI header is not available; the
/// real set reads it as the `perf_event_attr` the UAPI declares.
///
/// \pre none
/// \post Every field holds a callable; a null function reference
///       cannot reach a call site, because the real set and every
///       scripted set the tests install fill all nine fields. The one
///       call site the CPUID words feed checks the reference it reads,
///       so an install that misses the field fails the contract there.
struct KernelCalls
{
  long (*openEvent)(void* attr,
                    int pid,
                    int cpu,
                    int groupFd,
                    unsigned long flags);  ///< `perf_event_open` through
                                           ///< the syscall catalog.
  void* (*mapPage)(void* hint,
                   std::size_t length,
                   int prot,
                   int flags,
                   int fd,
                   long long offset);  ///< `mmap` of an event page.
  int (*unmapPage)(void* addr, std::size_t length);  ///< `munmap` of an
                                                     ///< event page.
  long (*readGroup)(int fd, void* buf, std::size_t count);  ///< `read` on
                                                            ///< a group leader.
  int (*ioctlEvent)(int fd, unsigned long request, unsigned long arg);
  ///< `ioctl` on a leader.
  int (*closeFd)(int fd);  ///< `close` of a descriptor.
  std::filesystem::path (*devicesRoot)();  ///< The directory the
                                           ///< enumeration walks.
  std::filesystem::path (*paranoidPath)();  ///< The paranoid control file.
  CpuidWords (*cpuidWords)();  ///< The CPUID words the ident decode
                               ///< consumes, answered on each call.
};

/// The call set the code runs through: the set a test installed, or the
/// real set where none stands.
///
/// \pre none
/// \post The answer is the set a test installed while that install
///       stands, and the real set at every other time. The answer is a
///       reference into static storage, so a caller that copies the set
///       keeps a stable value.
[[nodiscard]] auto kernelCalls() -> const KernelCalls&;

/// Installs a scripted call set for the calling test, or clears the
/// install with a null pointer, the way `installTestReader` clears.
///
/// \param scripted The set the tests drive, or nullptr to clear.
/// \pre The caller holds no concurrent installs: the install is a test
///      fixture action, and the tests that install run their scenario
///      before they clear.
/// \post With a non-null pointer, every call the code issues through
///       `kernelCalls()` reaches the set the pointer names until the
///       next install. With nullptr, every call reaches the real set.
///       The function answers nothing and keeps no copy: the caller
///       keeps the set alive while the install stands.
auto installKernelCalls(const KernelCalls* scripted) -> void;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_LINUX_PMU_KERNEL_CALLS_HPP
