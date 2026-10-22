// ============================================================================
// T003, K-13: the no-allocation proof of the probe loop, in the
// executable its counting hooks demand. The hooks replace the global
// operator new and delete - the basic four forms and the four aligned
// forms std::align_val_t names - and the thread sanitizer runtime
// defines those operators too, so the two definitions cannot link: the
// executable stays out of that preset, the way counters_noalloc_test
// and harness_capture_test do. The threaded probe scenarios run in
// harness_jitter_probe_test, which replaces nothing. ProbeCpuState
// carries an alignas(kCacheLineBytes) member (detail/jitter_probe.hpp),
// so its allocation goes through the aligned new, and the count sees
// it: a planted aligned allocation moves the count like the basic
// plant does. Frameworkless check()/fail() convention, the way
// harness_jitter_probe_test carries it.
// ============================================================================

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <thread>
#include <vector>

#include <sched.h>

#include "detail/jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

// The counters stand atomic: the probe's context threads allocate
// through the hooks while the main thread arms and reads the count
// around the span under test (FR-041, the TSan gate).
std::atomic<std::uint64_t> allocationCount {0};
std::atomic<bool> countAllocations {false};

// The volatile sink the plants hand their pointers to: the reference
// the optimizer cannot fold away, the shape the escape hatch carried
// before it (FR-041, Q-2).
void* volatile plantedEscapeSink = nullptr;

auto allocate(const std::size_t size) -> void*
{
  if (countAllocations.load(std::memory_order_relaxed)) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
  }
  return std::malloc(size == 0 ? 1 : size);
}

auto allocateAligned(const std::size_t size,
                     const std::size_t alignment) -> void*
{
  if (countAllocations.load(std::memory_order_relaxed)) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
  }
  const std::size_t bound = size == 0 ? 1 : size;
  return std::aligned_alloc(alignment,
                            (bound + alignment - 1) / alignment * alignment);
}

auto operator new(std::size_t size) -> void*
{
  void* const block = allocate(size);
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  return block;
}

// The nothrow form hands out the same malloc memory the deletes free,
// the pairing the vendored parser allocations expect (the pattern
// harness_capture_test.cpp carries).
auto operator new(std::size_t size, const std::nothrow_t&) noexcept -> void*
{
  return allocate(size);
}

auto operator new(std::size_t size, std::align_val_t alignment) -> void*
{
  void* const block =
      allocateAligned(size, static_cast<std::size_t>(alignment));
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  return block;
}

auto operator new(std::size_t size,
                  std::align_val_t alignment,
                  const std::nothrow_t&) noexcept -> void*
{
  return allocateAligned(size, static_cast<std::size_t>(alignment));
}

// GCC's warning pass attributes every allocation in a translation unit
// to the default `operator new` and never consults the replacements
// declared above, so it reports `-Wmismatched-new-delete` at each
// `free` below. The pairing is correct: these allocations hand out
// malloc memory and the deletes return it. The suppression covers
// exactly that false positive, the way
// test/source/counters_noalloc_test.cpp does (constitution I, X.2).
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

auto operator delete(void* block) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, std::size_t) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, std::align_val_t) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block,
                     std::size_t,
                     std::align_val_t) noexcept -> void
{
  std::free(block);
}

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER PROBE ALLOC TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The cpu indexes the scenarios pin probe threads to, taken from the
// process affinity mask, each below the online count (FR-040). No cpu
// number stands fixed in this file.
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  const int online = CPU_COUNT(&mask);
  check(online > 0, "the affinity mask holds cpus (FR-040)");
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask) != 0) {
      check(index < static_cast<std::size_t>(online),
            "a picked cpu stands below the online count (FR-040)");
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (FR-040)");
  return cpus;
}

// The scripted provider: one FakeProvider declaring the machine/tsc leaf
// at ReadMode::FAST_TSC (R-02), registered once per process, the object
// reference staying valid behind the registration for the read counter
// the waits spin on.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine",
                   "tsc",
                   "none",
                   "raw tick counter",
                   sg::counters::Availability::COUNTABLE,
                   sg::counters::ReadMode::FAST_TSC);
    const auto registered =
        sg::counters::System::local().registerProvider(std::move(owned));
    check(registered.has_value(), "the scripted tsc provider registers (R-02)");
    return &ref;
  }();
  return *provider;
}

// The measurement pass takes a fixed 1,024 loop reads the loop discards
// (E-05, R-05): the pass series steps by `step`, so every in-pass delta
// stands at `step` and the minimum period the pass measures is `step`.
auto passSeries(std::uint64_t step) -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> points;
  points.reserve(1024);
  for (std::uint64_t index = 0; index < 1024; ++index) {
    points.push_back(index * step);
  }
  return points;
}

}  // namespace

auto main() -> int
{
  // FR-041, SC-005, Q-2: the no-allocation proof. The counting hooks
  // arm around a span of loop passes the fake's read counter names, so
  // the setup allocations stand outside the count; a planted allocation
  // moves the count, so a clean run cannot come from a counter that
  // inspects nothing.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", passSeries(1), 1);
    const auto baseActions = provider.readActions();
    sg::detail::JitterProbe probe(loopOptions);
    probe.start();
    std::uint64_t spins = 0;
    while (provider.readActions() < baseActions + 1100) {
      std::this_thread::yield();
      if (++spins > 1000000000ULL) {
        fail("the loop never opened its window (FR-041)");
      }
    }
    countAllocations.store(true, std::memory_order_relaxed);
    spins = 0;
    while (provider.readActions() < baseActions + 20000) {
      std::this_thread::yield();
      if (++spins > 1000000000ULL) {
        fail("the loop never ran the armed passes (FR-041)");
      }
    }
    countAllocations.store(false, std::memory_order_relaxed);
    probe.stopAndJoin();
    check(allocationCount.load() == 0,
          "the loop takes no allocation (FR-041, SC-005)");
    countAllocations.store(true, std::memory_order_relaxed);
    // The plant runs through a heap allocation large enough that no
    // new/delete pair elision folds it into automatic storage (FR-041,
    // Q-2).
    std::vector<char> planted(1 << 20);
    plantedEscapeSink = planted.data();
    check(allocationCount.load() == 1,
          "a planted allocation moves the count (FR-041, Q-2)");
    // The aligned plant runs through the aligned new the aligned forms
    // replace: a ProbeCpuState carries alignas(kCacheLineBytes)
    // (detail/jitter_probe.hpp), so its allocation goes there, and the
    // count sees it the way it sees the basic one (FR-041, K-13).
    void* const alignedPlant =
        ::operator new(1 << 20, std::align_val_t {sg::detail::kCacheLineBytes});
    plantedEscapeSink = alignedPlant;
    check(allocationCount.load() == 2,
          "a planted aligned allocation moves the count (FR-041, K-13)");
    ::operator delete(alignedPlant,
                      std::align_val_t {sg::detail::kCacheLineBytes});
    countAllocations.store(false, std::memory_order_relaxed);
  }

  std::puts("harness_jitter_probe_alloc_test: ok");
  return 0;
}
