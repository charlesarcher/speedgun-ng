// ============================================================================
// T003: the foundational jitter probe surface. The option defaults of
// data-model.md E-01 (FR-003, R-15), the sixteen-field JitterStats
// aggregate in the E-03 order (FR-002, E-03), the three mode
// enumerators (E-02, R-10), the declared entry point and seam
// signatures (contracts/public-api.md, R-02), and the record and
// per-cpu state shapes the probe carries (E-04, E-05).
// Frameworkless check()/fail() convention, the way harness_gap_test
// carries it.
// ============================================================================

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>

#include <sched.h>
#include <unistd.h>

#include "detail/jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

// The counters stand atomic: the probe's context threads allocate
// through the hooks while the main thread arms and reads the count
// around the span under test (FR-041, the TSan gate).
std::atomic<std::uint64_t> allocationCount {0};
std::atomic<bool> countAllocations {false};
int const volatile* plantedEscape = nullptr;  // the plant's escape hatch

auto allocate(const std::size_t size) -> void*
{
  if (countAllocations.load(std::memory_order_relaxed)) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
  }
  return std::malloc(size == 0 ? 1 : size);
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

// GCC's warning pass attributes every allocation in a translation unit
// to the default `operator new` and never consults the replacement
// declared above, so it reports `-Wmismatched-new-delete` at each `free`
// below. The pairing is correct: these allocations hand out `malloc`
// memory and the deletes return it with `free`. The suppression covers
// exactly that false positive, the way test/source/counters_noalloc_test.cpp
// does (constitution I, X.2).
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

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER PROBE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// E-04, D-6: one kept gap carries two unsigned fields, 16 bytes.
static_assert(sizeof(sg::detail::GapRecord) == 16,
              "one kept gap record is 16 bytes (E-04, D-6)");

// E-03: the fields the table states as real values carry double, and
// the counts and ns durations carry the gap record's own width (D-6).
static_assert(std::is_same_v<decltype(sg::JitterStats::gapsPerSecond), double>,
              "gapsPerSecond carries a real value (E-03)");
static_assert(std::is_same_v<decltype(sg::JitterStats::meanNs), double>,
              "meanNs carries a real value (E-03)");
static_assert(std::is_same_v<decltype(sg::JitterStats::totalPercent), double>,
              "totalPercent carries a real value (E-03)");
static_assert(
    std::is_same_v<decltype(sg::JitterStats::gapCount), std::uint64_t>,
    "gapCount carries a count (E-03)");
static_assert(
    std::is_same_v<decltype(sg::JitterStats::droppedGaps), std::uint64_t>,
    "droppedGaps carries a count (E-03)");

// contracts/public-api.md: the surface of the two entry points, checked
// on the declared signatures alone, so no undefined definition stands
// in the link.
static_assert(
    std::is_same_v<decltype(&sg::runJitterProbe),
                   void (*)(const sg::JitterProbeOptions&)>,
    "the suite-level call takes one const options reference (FR-003)");

using HandleProbeFn = sg::BenchmarkHandle& (
    sg::BenchmarkHandle::*)(const sg::JitterProbeOptions&);
static_assert(
    std::is_same_v<decltype(&sg::BenchmarkHandle::jitterProbe), HandleProbeFn>,
    "the handle call returns this handle (FR-015)");

// R-02, FR-004: the reader-install seam takes one counters reader, and
// no second time source arrives through it.
static_assert(
    std::is_same_v<decltype(&sg::detail::installTestReader),
                   void (*)(std::unique_ptr<sg::counters::WindowReader>)>,
    "the seam installs one counters WindowReader (R-02, FR-004)");

// ============================================================================
// T006: the probe-loop scenarios. The gap series scripts through the fake
// provider's machine/tsc leaf at ReadMode::FAST_TSC (R-02) and, where the
// fake's action-count model cannot express a series, through the
// reader-install seam; start and stop synchronize on the scripted action
// counts, with no sleep and no clock anywhere (FR-039, FR-040).
// ============================================================================

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
    if (CPU_ISSET(index, &mask)) {
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

// Runs one probe over one scripted series and hands the per-cpu states to
// the assertions behind the join edge (R-04). The start and the stop
// synchronize on the provider's own read counter, the barrier the
// scenarios hang on (FR-039): the wait spins on the atomic count, with no
// sleep and no clock, and the bound turns a stalled series into a
// reported failure; the wait cannot hang.
template<class Assert>
auto runSeries(const sg::JitterProbeOptions& options,
               std::vector<std::uint64_t> points,
               std::uint64_t tailDelta,
               std::uint64_t waitActions,
               Assert&& assertions) -> void
{
  auto& provider = scriptedProvider();
  provider.setPoints("machine", "tsc", std::move(points), tailDelta);
  const auto baseActions = provider.readActions();
  sg::detail::JitterProbe probe(options);
  probe.start();
  std::uint64_t spins = 0;
  while (provider.readActions() < baseActions + waitActions) {
    // The wait yields, so the pinned probe thread progresses under
    // concurrent test load; the bound turns a stalled series into a
    // reported failure; the wait cannot hang (FR-039).
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the scripted series never reached the waited action (FR-039)");
    }
  }
  probe.stopAndJoin();
  assertions(probe.states());
}

// The per-thread stamp sequence of the scenarios the fake's action-count
// model cannot express: each probe thread reads its own series, so two
// cpus' reads interleave without sharing a cursor (R-02, FR-039).
struct PhaseThreadState
{
  std::uint64_t calls = 0;
  std::uint64_t stamp = 0;
  std::uint64_t ticket = 0;
};

thread_local PhaseThreadState phaseThreadState;
std::atomic<std::uint64_t> phaseReaderCalls {0};
std::atomic<std::uint64_t> phaseReaderSlots[2] {};
std::atomic<bool> phaseReaderBothPast {false};

// The in-pass step and the phase gap of the scripted thread series: the
// pass measures a minimum period of 100,000, the default threshold rises
// to ten times it, and the phase's jump stands a gap the risen threshold
// keeps (FR-007, R-05, FR-014).
inline constexpr std::uint64_t kPhaseStep = 100000;
inline constexpr std::uint64_t kPhaseGap = 2100000;

class PhasedReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    phaseReaderCalls.fetch_add(1, std::memory_order_relaxed);
    auto& thread = phaseThreadState;
    if (thread.ticket == 0) {
      thread.ticket = 1 + slotTicket.fetch_add(1, std::memory_order_relaxed);
    }
    ++thread.calls;
    std::uint64_t delta = kPhaseStep;
    // The measurement pass covers calls 1 to 1,024; the pre-check phase
    // ends at the call whose stamp stands precheckMs past its start, the
    // fifth phase read here; the alongside phase of `both` jumps at its
    // sixth read (FR-014, R-10).
    if (thread.calls == 1029 || thread.calls == 1040) {
      delta = kPhaseGap;
    }
    thread.stamp += delta;
    sink.put(thread.stamp);
    // The wait for `both` releases only when both threads' slots stand
    // past the alongside gap, so one thread's free-running passes cannot
    // stand in for the other's (FR-039).
    if (thread.ticket <= 2) {
      phaseReaderSlots[thread.ticket - 1].store(thread.calls,
                                                std::memory_order_relaxed);
      if (phaseReaderSlots[0].load(std::memory_order_relaxed) >= 1041
          && phaseReaderSlots[1].load(std::memory_order_relaxed) >= 1041)
      {
        phaseReaderBothPast.store(true, std::memory_order_relaxed);
      }
    }
  }

  std::atomic<std::uint64_t> slotTicket {0};
};

auto waitCalls(std::uint64_t target) -> void
{
  std::uint64_t spins = 0;
  while (phaseReaderCalls.load(std::memory_order_relaxed) < target) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the phased series never reached the waited call (FR-039)");
    }
  }
}

auto waitBothPast() -> void
{
  std::uint64_t spins = 0;
  while (!phaseReaderBothPast.load(std::memory_order_relaxed)) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the phased series never passed the alongside gap (FR-039)");
    }
  }
}

// The latch-order reader's step and call counts: the series steps by
// `kLatchStep` per read, so the measurement pass of 1,024 reads measures
// that step as its minimum period, the default threshold rises to ten
// times it, and the phase reads stand below the risen threshold and
// keep no gap; the scripted span of `precheckMs` 1 ends the pre-check
// phase at the tenth read past the pass (FR-007, FR-014, R-05).
inline constexpr std::uint64_t kLatchStep = 100000;
inline constexpr std::uint64_t kLatchPassCalls = 1024;
inline constexpr std::uint64_t kLatchPhaseCalls = 10;

// The latch-order reader's file-scope state: the per-thread series
// state, the ticket the thread's first read takes, and the call counts
// the reader's threads mirror. The counts stand at file scope, so the
// precheck mode's stop and join inside start() leaves them readable
// behind the reader it destroys (FR-006, R-04).
struct LatchThreadState
{
  std::uint64_t calls = 0;
  std::uint64_t stamp = 0;
  std::uint64_t ticket = 0;
};
thread_local LatchThreadState latchThreadState;
std::atomic<std::uint64_t> latchTicket {0};
std::atomic<std::uint64_t> latchCalls[2] {};

// The latch-order reader: one per-thread series the measurement pass
// reads and, in the precheck and both modes, the pre-check phase reads;
// each thread mirrors its call count to the file-scope slot its ticket
// names, so the benchmark-side snapshot reads the counts the reader's
// threads wrote (FR-006, FR-014, R-04).
class LatchOrderReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    auto& thread = latchThreadState;
    if (thread.ticket == 0) {
      thread.ticket =
          1 + latchTicket.fetch_add(1, std::memory_order_relaxed);
    }
    ++thread.calls;
    thread.stamp += kLatchStep;
    sink.put(thread.stamp);
    if (thread.ticket <= 2) {
      latchCalls[thread.ticket - 1].store(thread.calls,
                                          std::memory_order_relaxed);
    }
  }
};

auto latchOrderReset() -> void
{
  latchTicket.store(0, std::memory_order_relaxed);
  latchCalls[0].store(0, std::memory_order_relaxed);
  latchCalls[1].store(0, std::memory_order_relaxed);
}

// One latch-order case (FR-006, FR-014, R-04): the probe of one mode
// over one scripted cpu, and the snapshot the runner's first benchmark
// would stand behind. The reader answers the reads the measurement pass
// needs and, in the precheck and both modes, the reads the pre-check
// phase needs; the case reads the mirrored call count at the begin the
// runner's first benchmark rides and checks it against the guarded
// count the latch stands behind, and the joined state against the
// fields the guarded passes filled.
auto runLatchOrderCase(sg::JitterProbeMode mode, std::uint64_t guarded)
    -> void
{
  latchOrderReset();
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.mode = mode;
  options.precheckMs = 1;
  sg::detail::installTestReader(std::make_unique<LatchOrderReader>());
  sg::detail::JitterProbe probe(options);
  probe.start();
  // The benchmark side begins here: the runner's first benchmark stands
  // behind this return, so the snapshot reads the counts the latch
  // released (FR-006, R-04).
  const std::uint64_t began =
      latchCalls[0].load(std::memory_order_relaxed);
  probe.stopAndJoin();
  sg::detail::installTestReader(nullptr);
  check(began >= guarded,
        "the first benchmark begins only behind the latch (FR-006, "
        "FR-014, R-04)");
  const auto& probed = probe.states().at(0);
  check(probed.loopPeriodNs == kLatchStep,
        "the measurement pass the latch stands behind measured its "
        "period (FR-007, R-05)");
  check(probed.thresholdNs == 10 * kLatchStep,
        "the default rises to ten times the period (FR-007, D-2)");
  check(probed.kept == 0,
        "the scripted series below the risen threshold keeps no gap "
        "(FR-001, FR-005)");
}

}  // namespace

auto main() -> int
{
  // E-01, FR-003, R-15: the six default member initializers.
  const sg::JitterProbeOptions options;
  check(options.cpus.empty(),
        "an empty cpus names the idle preset (E-01, D-4)");
  check(options.thresholdNs == sg::kDefaultThresholdNs,
        "the default threshold stands at kDefaultThresholdNs (E-01)");
  check(options.mode == sg::JitterProbeMode::ALONGSIDE,
        "the default mode stands at ALONGSIDE (E-01, D-1)");
  check(options.gapCount == sg::kDefaultGapCount,
        "the default gap count stands at kDefaultGapCount (E-01)");
  check(options.precheckMs == sg::kDefaultPrecheckMs,
        "the default pre-check duration stands at kDefaultPrecheckMs (E-01)");
  check(!options.strict, "the strict flag stands false by default (E-01)");

  // E-01: the constants carry the values the table states.
  check(sg::kDefaultThresholdNs == 200,
        "kDefaultThresholdNs stands at 200 (E-01)");
  check(sg::kDefaultGapCount == 65536,
        "kDefaultGapCount stands at 65536 (E-01)");
  check(sg::kMinGapCount == 1, "kMinGapCount stands at 1 (FR-009)");
  check(sg::kMaxGapCount == 16777216,
        "kMaxGapCount stands at 16777216 (FR-009)");
  check(sg::kDefaultPrecheckMs == 200,
        "kDefaultPrecheckMs stands at 200 (E-01)");

  // E-02, R-10: the mode carries the three enumerators, each distinct.
  check(sg::JitterProbeMode::ALONGSIDE != sg::JitterProbeMode::PRECHECK
            && sg::JitterProbeMode::PRECHECK != sg::JitterProbeMode::BOTH
            && sg::JitterProbeMode::ALONGSIDE != sg::JitterProbeMode::BOTH,
        "the three mode enumerators stand distinct (E-02, R-10)");

  // E-03: the designated initializers name the sixteen fields in the
  // E-03 order; a misordered initializer is a compile error.
  const sg::JitterStats stats {
      .gapCount = 1,
      .gapsPerSecond = 2.0,
      .minNs = 3,
      .medianNs = 4,
      .meanNs = 5.0,
      .p90Ns = 6,
      .p99Ns = 7,
      .p999Ns = 8,
      .p9999Ns = 9,
      .maxNs = 10,
      .totalNs = 11,
      .totalPercent = 12.0,
      .runtimeNs = 13,
      .thresholdNs = 14,
      .loopPeriodNs = 15,
      .droppedGaps = 16,
  };
  check(stats.gapCount == 1 && stats.droppedGaps == 16,
        "the aggregate carries the E-03 fields in the E-03 order (E-03)");
  // The real-valued fields compare through the fabs form the house
  // convention carries, since -Werror=float-equal rules out `==` on a
  // double (the literals stand exact, so any small bound holds).
  check(std::fabs(stats.gapsPerSecond - 2.0) <= 1e-9
            && std::fabs(stats.meanNs - 5.0) <= 1e-9
            && std::fabs(stats.totalPercent - 12.0) <= 1e-9,
        "the real-valued fields carry doubles (E-03)");

  // E-05: one per-cpu state carries the pre-sized buffer of gapCount
  // 16-byte records, and the stop flag reads false (E-04, E-05). The
  // cpu index is synthetic data; no pin happens here (FR-040).
  sg::detail::ProbeCpuState state;
  state.cpu = 3;
  state.buffer.resize(sg::kDefaultGapCount);
  check(state.buffer.size() == sg::kDefaultGapCount,
        "the buffer pre-sizes to gapCount records (E-05, FR-009)");
  check(sizeof(sg::detail::GapRecord) == 16,
        "each record stands 16 bytes (E-04)");
  check(!state.stop.load(),
        "the stop flag stands readable and false (E-05, FR-005)");

  // E-06: the window record carries the TSC edges and the indexes that
  // name the attempt (R-04, FR-022).
  const sg::detail::TimedWindow window {
      .start = 10,
      .end = 20,
      .entry = 1,
      .repetition = 2,
      .chunk = 3,
      .pass = 4,
  };
  check(window.start < window.end && window.entry == 1 && window.pass == 4,
        "the window carries the E-06 fields in order (E-06)");

  // T006: the probe-loop scenarios. The red the ledger predicts stands
  // against the loop T009 implements: the holder, the folds and the seam
  // definitions arrive with that task.

  // FR-001, FR-005: the branches of the gap detection. The first kept gap
  // stands at the threshold, and the reads after it, every one below,
  // record none.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    auto points = passSeries(1);
    points.push_back(1223);  // the first window read: a gap of exactly 200
    runSeries(loopOptions,
              points,
              1,
              1100,
              [](const auto& states)
              {
                const auto& probed = states.at(0);
                check(probed.kept == 1,
                      "the read at the threshold keeps one gap (FR-001)");
                check(probed.buffer.at(0).gapNs == 200,
                      "the kept gap carries its size (E-04)");
                check(probed.buffer.at(0).stamp == 1223,
                      "the kept gap carries its stamp (E-04)");
                check(probed.droppedGaps == 0,
                      "the buffer never filled (FR-010)");
                const auto fold = sg::detail::gapStatsOf(probed);
                check(fold.gapCount == 1 && fold.minNs == 200
                          && fold.maxNs == 200 && fold.totalNs == 200,
                      "the whole-run fold reads the kept record (FR-002)");
                check(fold.droppedGaps == 0,
                      "the fold counts no dropped gap (FR-010)");
              });
  }

  // FR-001, FR-005: reads at or below the threshold record none, and the
  // fold of an empty kept set leaves every field at 0 (E-03).
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    runSeries(
        loopOptions,
        passSeries(1),
        1,
        1200,
        [](const auto& states)
        {
          const auto& probed = states.at(0);
          check(probed.kept == 0,
                "every read at or below the threshold records none (FR-001)");
          const auto fold = sg::detail::gapStatsOf(probed);
          check(fold.gapCount == 0 && fold.minNs == 0 && fold.medianNs == 0
                    && fold.maxNs == 0 && fold.totalNs == 0
                    && fold.runtimeNs == 0 && fold.droppedGaps == 0,
                "the empty fold leaves every field at 0 (E-03)");
        });
  }

  // FR-007, D-2, R-05: the default rises to ten times the measured
  // minimum loop period when that stands larger; a read below the risen
  // threshold records none and a read at it keeps.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    auto points = passSeries(30);  // the pass: deltas of 30
    points.push_back(30690 + 250);  // a gap of 250: below the rise
    points.push_back(30690 + 250 + 300);  // a gap of 300: at the rise
    runSeries(loopOptions,
              points,
              30,
              1100,
              [](const auto& states)
              {
                const auto& probed = states.at(0);
                check(probed.loopPeriodNs == 30,
                      "the pass measures the minimum period (E-05, R-05)");
                check(
                    probed.thresholdNs == 300,
                    "the default rises to ten times the period (FR-007, D-2)");
                check(probed.kept == 1 && probed.buffer.at(0).gapNs == 300,
                      "the risen threshold keeps its first gap (FR-007)");
              });
  }

  // FR-007, FR-008: an explicit threshold stands as given, and the rise
  // passes it by: the pass measures 30, the rise would hold 300, and the
  // gap of 100 the scenario keeps pins the explicit value.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    loopOptions.thresholdNs = 100;
    auto points = passSeries(30);
    points.push_back(30690 + 100);
    runSeries(
        loopOptions,
        points,
        30,
        1100,
        [](const auto& states)
        {
          const auto& probed = states.at(0);
          check(probed.thresholdNs == 100,
                "an explicit threshold stands as given (FR-007, FR-008)");
          check(probed.kept == 1 && probed.buffer.at(0).gapNs == 100,
                "the explicit threshold keeps the first gap at it (FR-007)");
        });
  }

  // FR-010, D-6, E-05: the buffer-full path. A buffer of two records
  // keeps the first two gaps, drops the stamps of the rest, and keeps
  // the count, the total and the max of the dropped gaps; the fold reads
  // the kept records only.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    loopOptions.gapCount = 2;
    auto points = passSeries(1);
    for (int index = 0; index < 4; ++index) {
      points.push_back(points.back() + 500);  // four gaps of 500
    }
    runSeries(loopOptions,
              points,
              1,
              1150,
              [](const auto& states)
              {
                const auto& probed = states.at(0);
                check(probed.kept == 2,
                      "the buffer keeps its first two gaps (E-05, FR-010)");
                check(probed.droppedGaps == 2,
                      "the buffer-full path counts the drops (FR-010)");
                check(probed.droppedTotalNs == 1000,
                      "the dropped total keeps (FR-010)");
                check(probed.droppedMaxNs == 500,
                      "the dropped max keeps (FR-010)");
                const auto fold = sg::detail::gapStatsOf(probed);
                check(fold.gapCount == 2 && fold.minNs == 500
                          && fold.maxNs == 500 && fold.medianNs == 500
                          && fold.totalNs == 1000 && fold.p90Ns == 500
                          && fold.p99Ns == 500 && fold.p999Ns == 500
                          && fold.p9999Ns == 500 && fold.droppedGaps == 2,
                      "the fold reads the kept records only (FR-010, D-6)");
              });
  }

  // FR-002, R-05: the whole-run fold fills the sixteen fields: the
  // nearest-rank percentiles over the sorted kept gaps, the chain, the
  // two coverage ratios over runtimeNs.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(1);
    auto points = passSeries(1);
    std::uint64_t stamp = 1023;
    for (std::uint64_t gap = 200; gap <= 1300; gap += 100) {
      stamp += gap;
      points.push_back(stamp);  // twelve gaps of 200 to 1300
    }
    runSeries(
        loopOptions,
        points,
        1,
        1200,
        [](const auto& states)
        {
          const auto& probed = states.at(0);
          check(probed.kept == 12, "every scripted gap keeps (FR-002)");
          const auto fold = sg::detail::gapStatsOf(probed);
          check(fold.gapCount == 12 && fold.minNs == 200 && fold.maxNs == 1300
                    && fold.totalNs == 9000,
                "the fold carries the kept set's own values (FR-002)");
          check(fold.medianNs == 700 && fold.p90Ns == 1200 && fold.p99Ns == 1300
                    && fold.p999Ns == 1300 && fold.p9999Ns == 1300,
                "the percentiles follow the nearest-rank rule (PC-4, R-05)");
          check(std::fabs(fold.meanNs - 750.0) <= 1e-9,
                "the mean carries the kept set's average (FR-002)");
          check(fold.minNs <= fold.medianNs && fold.medianNs <= fold.p90Ns
                    && fold.p90Ns <= fold.p99Ns && fold.p99Ns <= fold.p999Ns
                    && fold.p999Ns <= fold.p9999Ns
                    && fold.p9999Ns <= fold.maxNs,
                "the percentile chain holds (FR-002)");
          check(fold.runtimeNs == 8800,
                "runtimeNs covers the kept records' span (E-03, R-05)");
          const double seconds = static_cast<double>(fold.runtimeNs) / 1e9;
          check(std::fabs(fold.gapsPerSecond
                          - static_cast<double>(fold.gapCount) / seconds)
                    <= 1e-9 * (fold.gapsPerSecond + 1.0),
                "gapsPerSecond divides gapCount by the seconds of the span "
                "(E-03)");
          check(std::fabs(fold.totalPercent
                          - static_cast<double>(fold.totalNs) * 100.0
                              / static_cast<double>(fold.runtimeNs))
                    <= 1e-9 * (fold.totalPercent + 1.0),
                "totalPercent divides the kept total by the span (E-03)");
        });
  }

  // FR-014, R-10: the pre-check phase runs on all chosen cpus at the
  // same time for precheckMs, through the seam the fake's action-count
  // model cannot express; the phase's records land in the per-cpu
  // buffers the run-level fold reads.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(2);
    loopOptions.mode = sg::JitterProbeMode::PRECHECK;
    loopOptions.precheckMs = 1;
    phaseReaderCalls.store(0);
    sg::detail::installTestReader(std::make_unique<PhasedReader>());
    sg::detail::JitterProbe probe(loopOptions);
    probe.start();
    waitCalls(2 * 1029);  // both threads finished their pre-check phase
    probe.stopAndJoin();
    sg::detail::installTestReader(nullptr);
    const auto& states = probe.states();
    check(states.size() == 2, "both chosen cpus ran (FR-014, R-10)");
    for (const auto& probed : states) {
      check(probed.loopPeriodNs == kPhaseStep,
            "the pass measures the period the seam scripted (E-05, R-05)");
      check(probed.thresholdNs == 10 * kPhaseStep,
            "the default rises to ten times the period (FR-007, D-2)");
      check(probed.kept == 1 && probed.buffer.at(0).gapNs == kPhaseGap,
            "the pre-check phase keeps the gap its series scripted (FR-014)");
      check(probed.droppedGaps == 0, "the buffer never filled (FR-010)");
    }
  }

  // FR-014, R-10: `both` composes the pre-check phase and the alongside
  // phase over one probe cpu set and one buffer set per cpu.
  {
    sg::JitterProbeOptions loopOptions;
    loopOptions.cpus = pickCpus(2);
    loopOptions.mode = sg::JitterProbeMode::BOTH;
    loopOptions.precheckMs = 1;
    phaseReaderCalls.store(0);
    phaseReaderSlots[0].store(0);
    phaseReaderSlots[1].store(0);
    phaseReaderBothPast.store(false);
    sg::detail::installTestReader(std::make_unique<PhasedReader>());
    sg::detail::JitterProbe probe(loopOptions);
    probe.start();
    waitBothPast();  // both threads made their alongside-phase gap
    probe.stopAndJoin();
    sg::detail::installTestReader(nullptr);
    const auto& states = probe.states();
    for (const auto& probed : states) {
      check(probed.kept == 2,
            "both phases record into the one buffer (R-10, E-05)");
      check(probed.buffer.at(0).gapNs == kPhaseGap
                && probed.buffer.at(1).gapNs == kPhaseGap,
            "the pre-check record and the alongside record both keep (R-10)");
    }
  }

  // The latch order (FR-006, FR-014, R-04): in every mode, the first
  // benchmark the batch would begin stands behind the latch, so it
  // begins only once the probe cpu stands past its buffer's first
  // touch and its measurement pass and, in the precheck and both
  // modes, past its pre-check phase. The guarded count names the reads
  // the phases take: the measurement pass's 1,024 reads, plus the ten
  // pre-check reads the scripted span of precheckMs 1 takes.
  {
    runLatchOrderCase(sg::JitterProbeMode::ALONGSIDE, kLatchPassCalls);
    runLatchOrderCase(sg::JitterProbeMode::PRECHECK,
                      kLatchPassCalls + kLatchPhaseCalls);
    runLatchOrderCase(sg::JitterProbeMode::BOTH,
                      kLatchPassCalls + kLatchPhaseCalls);
  }

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
    const std::vector<char> planted(1 << 20);
    plantedEscape = reinterpret_cast<const volatile int*>(planted.data());
    check(allocationCount.load() == 1,
          "a planted allocation moves the count (FR-041, Q-2)");
    countAllocations.store(false, std::memory_order_relaxed);
  }

  std::puts("harness_jitter_probe_test: ok");
  return 0;
}
