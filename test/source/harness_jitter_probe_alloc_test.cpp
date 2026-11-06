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
#include <type_traits>
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

// T016, specs/048 FR-012, Constraints: the kept gap record keeps its 16-byte
// size and its two standing fields, and the SMI change rides the ring beside
// it. The gap path costs no third field.
static_assert(sizeof(sg::detail::GapRecord) == 16,
              "one kept gap record is 16 bytes (E-04, specs/048 FR-012)");
static_assert(
    std::is_same_v<decltype(sg::detail::GapRecord::stamp), std::uint64_t>,
    "the record keeps its stamp field (E-04, specs/048 FR-012)");
static_assert(
    std::is_same_v<decltype(sg::detail::GapRecord::gapNs), std::uint64_t>,
    "the record keeps its gap width field (E-04, specs/048 FR-012)");

// The tail step of the gap scenarios: a delta past the threshold the
// measurement pass of a 1,000 ns series rises to, so every pass past the
// scripted points keeps a gap and the armed span covers the keep step
// (specs/048 FR-012, FR-007).
constexpr std::uint64_t kGapTailStep = 20000;

// The read the gap span arms at: past the 1,024 reads of the measurement
// pass and past the latch line, so the setup, the pass, and the two
// window-edge reads of the run span stand outside the count (FR-012,
// FR-041).
constexpr std::uint64_t kGapArmPast = 200;

// The wait for one scripted read count: the bound turns a stalled series
// into a reported failure, so the wait cannot hang (FR-039).
auto waitForActions(const std::uint64_t target, const char* what) -> void
{
  auto& provider = scriptedProvider();
  std::uint64_t spins = 0;
  while (provider.readActions() < target) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail(what);
    }
  }
}

// One armed span over the gap path of the `tsc` loop: the hooks arm over the
// passes the fake's read counter names, the hook runs while the span stands
// armed, and the answer is the count the hooks took (FR-012, FR-041). The
// kept count answers how many gaps the span kept.
template<class Hook>
auto runGapSpan(const sg::JitterProbeOptions& loopOptions,
                const std::uint64_t spanEnd,
                Hook&& hook,
                std::uint64_t& keptOut) -> std::uint64_t
{
  auto& provider = scriptedProvider();
  provider.setPoints("machine", "tsc", passSeries(1000), kGapTailStep);
  const auto baseActions = provider.readActions();
  sg::detail::JitterProbe probe(loopOptions);
  probe.start();
  waitForActions(baseActions + sg::detail::kPassReads + kGapArmPast,
                 "the gap loop never opened its window (FR-041)");
  allocationCount.store(0, std::memory_order_relaxed);
  countAllocations.store(true, std::memory_order_relaxed);
  hook();
  waitForActions(baseActions + spanEnd,
                 "the gap loop never ran the armed passes (FR-041)");
  countAllocations.store(false, std::memory_order_relaxed);
  probe.stopAndJoin();
  keptOut = probe.states().at(0).kept;
  return allocationCount.load(std::memory_order_relaxed);
}

// One armed span over one method's measured loop: the counting hooks
// arm over the loop passes the fake's read counter names, so the setup
// allocations, the buffer's first touch, the calibration and the
// measurement pass stand outside the count (FR-014, D-6, R-08). The
// span ends past the point the method's loop is recorded to have run, so
// a clean count cannot come from a loop that never opened.
auto runArmedSpan(const sg::JitterProbeOptions& loopOptions,
                  const std::uint64_t armAt,
                  const std::uint64_t spanEnd,
                  const char* what) -> void
{
  auto& provider = scriptedProvider();
  // The step of 1,000 ns stands at the FWQ target and above the FTQ
  // target of one thousandth of the quantum, so the calibration keeps
  // the count 1 and the armed span covers the loop's own passes (FR-012,
  // FR-015, R-08).
  provider.setPoints("machine", "tsc", passSeries(1000), 1);
  const auto baseActions = provider.readActions();
  sg::detail::JitterProbe probe(loopOptions);
  probe.start();
  std::uint64_t spins = 0;
  while (provider.readActions() < baseActions + armAt) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the loop never opened its window (FR-041)");
    }
  }
  allocationCount.store(0, std::memory_order_relaxed);
  countAllocations.store(true, std::memory_order_relaxed);
  spins = 0;
  while (provider.readActions() < baseActions + spanEnd) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the loop never ran the armed passes (FR-041)");
    }
  }
  countAllocations.store(false, std::memory_order_relaxed);
  probe.stopAndJoin();
  check(allocationCount.load() == 0, what);
  check(probe.states().at(0).seriesKept > 0,
        "the armed span covered the method's measured loop (FR-014, D-6)");
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

  // FR-014, D-6, R-08: the FWQ unit loop. The calibration prefix of 98
  // reads and the standing 1,024-read pass open the window at read 1,123,
  // so the span arms inside the unit loop and runs 18,800 armed passes.
  {
    sg::JitterProbeOptions fwqOptions;
    fwqOptions.cpus = pickCpus(1);
    fwqOptions.method = sg::JitterMethod::FWQ;
    runArmedSpan(fwqOptions,
                 1200,
                 20000,
                 "the FWQ unit loop takes no allocation (FR-014, D-6)");
  }

  // FR-014, FR-016, D-6, R-08: the FTQ quantum loop. A 10 us quantum
  // closes every 10,000 reads, so the span of 28,800 armed passes covers
  // two edge crossings, the store, the reset and the edge advance.
  {
    sg::JitterProbeOptions ftqOptions;
    ftqOptions.cpus = pickCpus(1);
    ftqOptions.method = sg::JitterMethod::FTQ;
    ftqOptions.quantumUs = 10;
    runArmedSpan(ftqOptions,
                 1200,
                 30000,
                 "the FTQ quantum loop takes no allocation (FR-014, D-6)");
  }

  // T016, specs/048 FR-012, FR-013, P-2, plan R-1: the gap path with the
  // register read beside it. Every pass past the scripted points keeps a gap,
  // so the armed span covers the step that stores record and change together.
  // The count stays clean, and the kept count proves the step ran under the
  // armed count.
  {
    sg::JitterProbeOptions gapOptions;
    gapOptions.cpus = pickCpus(1);
    std::uint64_t kept = 0;
    const auto clean = runGapSpan(gapOptions, 20000, [] {}, kept);
    check(clean == 0,
          "the gap path takes no allocation (specs/048 FR-012, P-2)");
    check(kept > 0,
          "the armed span covered the keep step of the gap path (specs/048 "
          "FR-012, FR-013)");
  }

  // T016, FR-041, Q-2: the plant on the gap path. One allocation the armed
  // gap span plants moves the count, so the clean run above cannot come from
  // a guard that inspects nothing.
  {
    sg::JitterProbeOptions plantOptions;
    plantOptions.cpus = pickCpus(1);
    std::uint64_t kept = 0;
    const auto planted = runGapSpan(
        plantOptions,
        20000,
        []
        {
          std::vector<char> allocation(1 << 20);
          plantedEscapeSink = allocation.data();
        },
        kept);
    check(planted == 1,
          "a planted allocation on the gap path moves the count (T016, "
          "FR-041, Q-2)");
    check(kept > 0, "the planted span kept gaps on the gap path (T016)");
  }

  std::puts("harness_jitter_probe_alloc_test: ok");
  return 0;
}
