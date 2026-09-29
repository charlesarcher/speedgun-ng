// The shipped system clock provider: monotonic wall time, thread and
// process CPU time, and the raw time-stamp counter
// (specs/007-counters-and-timers, FR-033; specs/008-timestamp-counter,
// FR-001). Platform terms stay confined to this translation unit.

#include <cstdint>
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

// The time-stamp entry publishes exactly where the instruction exists
// (FR-001) and the reader opens exactly where the catalog enumerates
// (FR-003). One condition, decided at build time: x86-64 mandates the
// instruction, so a build without it is a build without the counter.
#ifdef SG_COUNTERS_X86
constexpr bool kTscAvailable = true;
#else
constexpr bool kTscAvailable = false;
#endif

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

// The read's arm for a build without the instruction is marked below. The
// entry publishes wherever the instruction exists (FR-001), so the sampled
// arm is covered on every host this suite runs and carries no marker, and no
// fixture can remove an instruction from a running binary, so the absent arm
// does (T066, P2 recorded in
// specs/007-counters-and-timers/plan.md Complexity Tracking).
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
  return 0;  // LCOV_EXCL_LINE
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
        default:
          sink.put(tsc_ticks());
          break;
      }
    }
  }
};

clock_provider::clock_provider() = default;

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
#ifdef SG_COUNTERS_X86
  // The raw time-stamp counter (FR-001, FR-002): the entry publishes
  // wherever the build executes the instruction, and the count carries no
  // rate, so the frequency and the scaled flag keep their zero defaults
  // and the description asserts none. 007 gated this entry on a sysfs
  // frequency and attached that rate to it; a count needs neither, and the
  // gate withheld a working counter from every host publishing no
  // frequency (specs/008-timestamp-counter, FR-011).
  entries.push_back(catalog_seed {
      .name = "tsc",
      .description = "raw time-stamp counter ticks; a count asserting no rate",
      .unit = "none",
      .avail = availability::countable,
      .mode = read_mode::fast_tsc,
      .frequency_hz = 0,
      .scaled = false,
  });
#endif
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
        || (index == kTscIndex && !kTscAvailable)) {  // LCOV_EXCL_BR_LINE
      return nullptr;
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    window->kinds.push_back(static_cast<std::uint8_t>(index));
  }
  return window;
}

}  // namespace sg::counters
