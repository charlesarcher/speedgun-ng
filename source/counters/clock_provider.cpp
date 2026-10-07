// The shipped system clock provider: monotonic wall time, thread and
// process CPU time, and the raw time-stamp counter
// (specs/007-counters-and-timers, FR-033; specs/008-timestamp-counter,
// FR-001). Platform terms stay confined to this translation unit.

#include <cstdint>
#include <memory>
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
// tsc, monotonic_raw. Addresses are canonical machine-root spellings
// (C-PRO-2). The new leaf sits last so the three existing indices keep
// their values on every target, including one that publishes no tsc.
constexpr std::string_view kAddresses[] = {"machine/monotonic",
                                           "machine/thread_cpu",
                                           "machine/process_cpu",
                                           "machine/tsc",
                                           "machine/monotonic_raw"};

constexpr int kTscIndex = 3;

// The address table's length, so adding a leaf cannot leave the parser
// reading fewer entries than the catalog publishes.
constexpr int kLeafCount =
    static_cast<int>(sizeof(kAddresses) / sizeof(kAddresses[0]));

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
  for (int index = 0; index < kLeafCount; ++index) {
    if (address == kAddresses[index]) {
      return index;
    }
  }
  return -1;
}

auto monotonicNs() noexcept -> std::uint64_t
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
  // LCOV_EXCL_BR_START : coverage exclusion (T066): `monotonicNs` does not
  // fail on Linux. glibc routes it through the vDSO and the kernel clock is
  // unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_MONOTONIC, &stamp) != 0) {  // LCOV_EXCL_BR_LINE
    return 0;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  return static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(stamp.tv_nsec);
#endif
}

// The nanosecond-rate clock (specs/011 FR-001, FR-004): the kernel serves
// it from the hardware clocksource's own rate and never slews it, so a
// long campaign of stored results holds no accumulated drift against the
// hardware counter underneath. The conversion is the same integer
// expression the three readers above use, with no floating-point step.
auto monotonicRawNs() noexcept -> std::uint64_t
{
  timespec stamp {};
  // LCOV_EXCL_BR_START : coverage exclusion (011 T007): `monotonicRawNs`
  // does not fail on Linux. glibc routes it through the vDSO and the kernel
  // clock is unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &stamp) != 0) {  // LCOV_EXCL_BR_LINE
    return 0;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  return static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(stamp.tv_nsec);
}

auto threadCpuNs() noexcept -> std::uint64_t
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
  // LCOV_EXCL_BR_START : coverage exclusion (T066): `threadCpuNs` does not
  // fail on Linux. glibc routes it through the vDSO and the kernel clock is
  // unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stamp) != 0)
  {  // LCOV_EXCL_BR_LINE
    return 0;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  return static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL
      + static_cast<std::uint64_t>(stamp.tv_nsec);
#endif
}

auto processCpuNs() noexcept -> std::uint64_t
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
  // LCOV_EXCL_BR_START : coverage exclusion (T066): `processCpuNs` does not
  // fail on Linux. glibc routes it through the vDSO and the kernel clock is
  // unconditional, so no test can make this arm run.
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stamp) != 0)
  {  // LCOV_EXCL_BR_LINE
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
auto tscTicks() noexcept -> std::uint64_t
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

struct detail::ClockWindow final : WindowReader
{
  ClockWindow() { setThunk(&readDirect); }

  // The compiled plan hands the window over as the base reference
  // `ReadThunk` declares, and `ClockProvider::open` constructs it as
  // this final type, so the reference names a clock window on every
  // call. `final` fixes the target of the `readPoints` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto readDirect(WindowReader& base, PointSink& sink) noexcept -> void
  {
    static_cast<ClockWindow&>(base).readPoints(sink);
  }

  std::vector<std::uint8_t> kinds;
  std::size_t disclosure_column = LeafSet::kNoDisclosureColumn;

  void readPoints(PointSink& sink) noexcept override
  {
    for (const std::uint8_t kind : kinds) {
      switch (kind) {
        case 0:
          sink.put(monotonicNs());
          break;
        case 1:
          sink.put(threadCpuNs());
          break;
        case 2:
          sink.put(processCpuNs());
          break;
        case 4:
          sink.put(monotonicRawNs());
          break;
        default:
          sink.put(tscTicks());
          break;
      }
    }
    // A clock leaf is a read of a clock the platform always keeps running,
    // so the action always measures and the disclosure names the entry's
    // own countability value (FR-007).
    if (disclosure_column != LeafSet::kNoDisclosureColumn) {
      sink.putDisclosure(disclosure_column,
                         static_cast<std::uint64_t>(Availability::COUNTABLE));
    }
  }
};

ClockProvider::ClockProvider() = default;

ClockProvider::~ClockProvider() = default;

void ClockProvider::enumerate(ObjectSink& sink) const
{
  std::vector<CatalogSeed> entries {
      CatalogSeed {
          .name = "monotonic",
          .description = "wall-clock time, monotonic across the window",
          .unit = "nanoseconds",
          .avail = Availability::COUNTABLE,
          .mode = ReadMode::SYSCALL,
      },
      CatalogSeed {
          .name = "thread_cpu",
          .description = "CPU time consumed by the sampling thread",
          .unit = "nanoseconds",
          .avail = Availability::COUNTABLE,
          .mode = ReadMode::SYSCALL,
      },
      CatalogSeed {
          .name = "process_cpu",
          .description = "CPU time consumed by this process",
          .unit = "nanoseconds",
          .avail = Availability::COUNTABLE,
          .mode = ReadMode::SYSCALL,
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
  entries.push_back(CatalogSeed {
      .name = "tsc",
      .description = "raw time-stamp counter ticks; a count asserting no rate",
      .unit = "none",
      .avail = Availability::COUNTABLE,
      .mode = ReadMode::FAST_TSC,
      .frequencyHz = 0,
      .scaled = false,
  });
#endif
  // The nanosecond-rate leaf (specs/011 FR-001, FR-002, FR-005): it
  // publishes on every supported build, because the platform serves this
  // clock through its fast path on every supported target. The read mode
  // is the label every clock counter here carries, and the published
  // overhead table records the fast path beside it (FR-010).
  entries.push_back(CatalogSeed {
      .name = "monotonic_raw",
      .description =
          "hardware-rate time, never adjusted by the operating " "system",
      .unit = "nanoseconds",
      .avail = Availability::COUNTABLE,
      .mode = ReadMode::SYSCALL,
  });
  sink.addObject(ObjectSeed {
      .kind = "machine",
      .path = "machine",
      .alias = {},
      .description = "local machine",
      .entries = std::move(entries),
  });
}

std::unique_ptr<WindowReader> ClockProvider::open(const LeafSet& leaves,
                                                  const Target& /*where*/)
{
  auto window = std::make_unique<detail::ClockWindow>();
  window->disclosure_column = leaves.disclosureColumn;
  window->kinds.reserve(leaves.addresses.size());
  for (const auto& address : leaves.addresses) {
    const int index = parse(address);
    if (index < 0 || (index == kTscIndex && !kTscAvailable)) {
      return nullptr;
    }
    window->kinds.push_back(static_cast<std::uint8_t>(index));
  }
  return window;
}

}  // namespace sg::counters
