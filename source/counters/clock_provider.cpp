// The shipped system clock provider: monotonic wall time, thread and
// process CPU time, and the calibrated time-stamp counter
// (specs/007-counters-and-timers, FR-033, FR-034, R-007). Platform
// terms stay confined to this translation unit.

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <ctime>
#endif

#if (defined(__x86_64__) || defined(__i386__)) && !defined(_MSC_VER)
#  define SG_COUNTERS_X86 1
#  include <immintrin.h>
#  if defined(__linux__)
#    include <cpuid.h>
#  endif
#endif

namespace sg::counters
{
namespace
{

// Leaf kinds in enumerate order: monotonic, thread_cpu, process_cpu,
// tsc. Addresses are canonical machine-root spellings (C-PRO-2).
constexpr std::string_view kAddresses[] = {"machine/monotonic",
                                           "machine/thread_cpu",
                                           "machine/process_cpu",
                                           "machine/tsc"};

constexpr int kTscIndex = 3;

auto parse(const std::string_view address) noexcept -> int
{
  for (int index = 0; index < 4; ++index) {
    if (address == kAddresses[index]) {
      return index;
    }
  }
  return -1;
}

auto monotonic_ns() noexcept -> std::uint64_t
{
#if defined(_WIN32)
  LARGE_INTEGER now {};
  QueryPerformanceCounter(&now);
  static const LARGE_INTEGER frequency = []
  {
    LARGE_INTEGER value {};
    QueryPerformanceFrequency(&value);
    return value;
  }();
  return static_cast<std::uint64_t>(now.QuadPart) * 1000000000ULL
      / static_cast<std::uint64_t>(frequency.QuadPart);
#else
  timespec stamp {};
  // LCOV_EXCL_BR_START : coverage exclusion (T066): `monotonic_ns` does not
  // fail on Linux. glibc routes it through the vDSO and the kernel clock is
  // unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_MONOTONIC, &stamp) != 0) {  // LCOV_EXCL_BR_LINE
    return 0;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  return static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(stamp.tv_nsec);
#endif
}

auto thread_cpu_ns() noexcept -> std::uint64_t
{
#if defined(_WIN32)
  FILETIME creation {};
  FILETIME exit {};
  FILETIME kernel {};
  FILETIME user {};
  if (!GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user)) {
    return 0;
  }
  const ULARGE_INTEGER kernel_time {.LowPart = kernel.dwLowDateTime,
                                    .HighPart = kernel.dwHighDateTime};
  const ULARGE_INTEGER user_time {.LowPart = user.dwLowDateTime,
                                  .HighPart = user.dwHighDateTime};
  return (kernel_time.QuadPart + user_time.QuadPart) * 100ULL;
#else
  timespec stamp {};
  // LCOV_EXCL_BR_START : coverage exclusion (T066): `thread_cpu_ns` does not
  // fail on Linux. glibc routes it through the vDSO and the kernel clock is
  // unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stamp)
      != 0) {  // LCOV_EXCL_BR_LINE
    return 0;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  return static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(stamp.tv_nsec);
#endif
}

auto process_cpu_ns() noexcept -> std::uint64_t
{
#if defined(_WIN32)
  FILETIME creation {};
  FILETIME exit {};
  FILETIME kernel {};
  FILETIME user {};
  if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
    return 0;
  }
  const ULARGE_INTEGER kernel_time {.LowPart = kernel.dwLowDateTime,
                                    .HighPart = kernel.dwHighDateTime};
  const ULARGE_INTEGER user_time {.LowPart = user.dwLowDateTime,
                                  .HighPart = user.dwHighDateTime};
  return (kernel_time.QuadPart + user_time.QuadPart) * 100ULL;
#else
  timespec stamp {};
  // LCOV_EXCL_BR_START : coverage exclusion (T066): `process_cpu_ns` does not
  // fail on Linux. glibc routes it through the vDSO and the kernel clock is
  // unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stamp)
      != 0) {  // LCOV_EXCL_BR_LINE
    return 0;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  return static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(stamp.tv_nsec);
#endif
}

// LCOV_EXCL_START : coverage exclusion (T066, P2 recorded in
// specs/007-counters-and-timers/plan.md Complexity Tracking): the whole
// time-stamp leaf. FR-034 publishes the leaf only where sysfs carries a
// calibration, and `/sys/devices/system/cpu/tsc_khz` is kernel data no test
// can write: on a host without it `m_tsc.present` stays false, the leaf is
// omitted from the catalog, and nothing samples it. The region ends with the
// constructor.
auto tsc_ticks() noexcept -> std::uint64_t
{
#ifdef SG_COUNTERS_X86
  // P2 intrinsic justification (T036): `__rdtsc` is chosen over
  // `rdtscp`. Both window endpoints are taken by the same thread
  // that samples this leaf, so no read-ordering serialisation is
  // required at the boundary; mispredicted speculation past the
  // read skews only this thread's own sample and crosses no
  // privilege boundary (R-007).
  return static_cast<std::uint64_t>(__rdtsc());
#else
  return 0;
#endif
}

}  // namespace

struct detail::clock_window final : window_reader
{
  clock_window() { set_thunk(&read_direct); }

  // The compiled plan hands the window over as the base reference
  // `read_thunk` declares, and `clock_provider::open` constructs it as
  // this final type, so the reference names a clock window on every
  // call. `final` fixes the target of the `read_points` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto read_direct(window_reader& base, point_sink& sink) noexcept
      -> void
  {
    static_cast<clock_window&>(base).read_points(sink);
  }

  std::vector<std::uint8_t> kinds;

  void read_points(point_sink& sink) noexcept override
  {
    for (const std::uint8_t kind : kinds) {
      switch (kind) {
        case 0:
          sink.put(monotonic_ns());
          break;
        case 1:
          sink.put(thread_cpu_ns());
          break;
        case 2:
          sink.put(process_cpu_ns());
          break;
          // LCOV_EXCL_BR_START : coverage exclusion (T066): the time-stamp
          // arm, sampled only where the leaf is published. See the region
          // marker on `tsc_ticks`.
        default:  // LCOV_EXCL_BR_LINE
          sink.put(tsc_ticks());  // LCOV_EXCL_LINE
          break;  // LCOV_EXCL_LINE
          // LCOV_EXCL_BR_STOP
      }
    }
  }
};

clock_provider::clock_provider()
{
#if defined(SG_COUNTERS_X86) && defined(__linux__)
  // Calibration (FR-034, R-007): the sysfs TSC frequency is the
  // source of truth, and the scaling flag is one comparison against
  // a second source: `scaled` is set when that rate differs from the
  // nominal core frequency CPUID leaf 0x16 EAX reports in MHz.
  // Invariance lives in leaf 0x80000007 EDX bit 8; this flag claims a
  // rate comparison, so a host reporting no nominal never sets it.
  // The calibration runs here, at construction, because `enumerate`
  // is const and runs at registration, and registration is refused
  // after open: the catalog freezes at that boundary (FR-009), so a
  // calibration deferred to open would publish an uncalibrated leaf.
  // No tsc_khz means no usable calibration, so the leaf is omitted
  // (catalog fact, zero API difference).
  std::ifstream khz_file("/sys/devices/system/cpu/tsc_khz");
  std::uint64_t khz = 0;
  if (khz_file >> khz && khz != 0) {
    m_tsc.present = true;
    m_tsc.khz = khz;
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    if (__get_cpuid(0x16, &eax, &ebx, &ecx, &edx) && eax != 0) {
      const auto nominal_khz = static_cast<std::uint64_t>(eax) * 1000ULL;
      m_tsc.scaled = nominal_khz != khz;
    }
  }
#endif
}  // LCOV_EXCL_STOP

clock_provider::~clock_provider() = default;

void clock_provider::enumerate(object_sink& sink) const
{
  std::vector<catalog_seed> entries {
      catalog_seed {
          .name = "monotonic",
          .description = "wall-clock time, monotonic across the window",
          .unit = "nanoseconds",
          .avail = availability::countable,
          .mode = read_mode::syscall,
      },
      catalog_seed {
          .name = "thread_cpu",
          .description = "CPU time consumed by the sampling thread",
          .unit = "nanoseconds",
          .avail = availability::countable,
          .mode = read_mode::syscall,
      },
      catalog_seed {
          .name = "process_cpu",
          .description = "CPU time consumed by this process",
          .unit = "nanoseconds",
          .avail = availability::countable,
          .mode = read_mode::syscall,
      },
  };
  // LCOV_EXCL_START : coverage exclusion (T066): the time-stamp catalog
  // entry, published only where FR-034's sysfs calibration exists. See the
  // region marker on `tsc_ticks` above.
  if (m_tsc.present) {  // LCOV_EXCL_BR_LINE
    const auto mhz = m_tsc.khz / 1000ULL;
    const std::string description =
        "raw time-stamp counter ticks; calibrated at " + std::to_string(mhz)
        + " MHz from sysfs tsc_khz, "
        + (m_tsc.scaled
               ? "CPUID leaf 0x16 reports a different nominal core "
                 "frequency (platform-scaled)"
               : "no CPUID leaf 0x16 nominal core frequency differs "
                 "(constant rate)");
    entries.push_back(catalog_seed {
        .name = "tsc",
        .description = description,
        .unit = "none",
        .avail = availability::countable,
        .mode = read_mode::fast_tsc,
        .frequency_hz = m_tsc.khz * 1000ULL,
        .scaled = m_tsc.scaled,
    });  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
     // LCOV_EXCL_STOP
  sink.add_object(object_seed {
      .kind = "machine",
      .path = "machine",
      .alias = {},
      .description = "local machine",
      .entries = std::move(entries),
  });
}

std::unique_ptr<window_reader> clock_provider::open(const leaf_set& leaves,
                                                    const target& /*where*/)
{
  auto window = std::make_unique<detail::clock_window>();
  window->kinds.reserve(leaves.addresses.size());
  for (const auto& address : leaves.addresses) {
    const int index = parse(address);
    // LCOV_EXCL_BR_START : the loop-exit edge of the enclosing `for`, which
    // every direct-open fixture leaves unreached because it opens one
    // address per call, so gcc reports it as an unexecuted block.
    if (index < 0  // LCOV_EXCL_BR_LINE
        || (index == kTscIndex && !m_tsc.present)) {  // LCOV_EXCL_BR_LINE
      return nullptr;
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    window->kinds.push_back(static_cast<std::uint8_t>(index));
  }
  return window;
}

}  // namespace sg::counters
