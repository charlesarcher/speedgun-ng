// ============================================================================
// T001: the public surface the fixed work quanta and fixed time quanta
// probes stand on (US4). The three method enumerators (FR-001, E-21), the
// four option fields with their defaults and their types (FR-002, E-27,
// R-20), the bounds the parser enforces and the calibration stops the
// search reads (FR-002, FR-010), the series record beside `JitterStats`
// and the run record of the loaded report (FR-020, E-23, E-24, E-25), and
// the version record the lane stands on (FR-005). The spellings follow
// plan.md R-20.
//
// The T011 block below runs the calibration of one probe cpu through the
// reader-install seam, the shape harness_jitter_probe_test carries with
// check(), fail(), pickCpus over sched_getaffinity, a pass series over
// 1,024 reads and a per-thread reader.
//
// Frameworkless check()/fail() convention, the way
// harness_jitter_probe_test carries it.
// ============================================================================

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

#include <sched.h>

#include "detail/jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_provider.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS FWQ FTQ TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// FR-001, E-21: the method enum closes the set at the type level the way
// `JitterProbeMode` closes the modes, and carries the three words. `TSC`
// stands first, the position the JSON word table indexes.
static_assert(std::is_enum_v<sg::JitterMethod>,
              "JitterMethod names the method set (FR-001)");
static_assert(
    std::is_same_v<std::underlying_type_t<sg::JitterMethod>, std::uint8_t>,
    "JitterMethod carries the mode enum's width (E-21)");
static_assert(sg::JitterMethod::TSC != sg::JitterMethod::FWQ
                  && sg::JitterMethod::FWQ != sg::JitterMethod::FTQ
                  && sg::JitterMethod::TSC != sg::JitterMethod::FTQ,
              "the three enumerators stand distinct (FR-001)");
static_assert(sg::JitterMethod::TSC == static_cast<sg::JitterMethod>(0),
              "TSC stands first, the word a value with no key takes (FR-001)");

// FR-002, E-27, R-20: the four fields, their types, and the microseconds
// the option carries.
static_assert(
    std::is_same_v<decltype(sg::JitterProbeOptions::method), sg::JitterMethod>,
    "method carries the method enum (FR-001)");
static_assert(
    std::is_same_v<decltype(sg::JitterProbeOptions::quantumUs), std::uint32_t>,
    "quantumUs carries microseconds as a count (FR-002, R-20)");
static_assert(
    std::is_same_v<decltype(sg::JitterProbeOptions::workCount), std::uint32_t>,
    "workCount carries loop bodies (FR-002, R-20)");
static_assert(std::is_same_v<decltype(sg::JitterProbeOptions::noisePercent),
                             std::uint32_t>,
              "noisePercent carries a whole percent (FR-002, R-20)");

// FR-020, E-23, E-25: the series record joins the fold beside the gap
// stats and the pre-check cpu, and the loaded report carries the run
// record. The period list rides the series record.
static_assert(std::is_same_v<decltype(sg::JitterEntryFold::series),
                             std::optional<sg::JitterSeriesStats>>,
              "the fold carries the series record (FR-020)");
static_assert(std::is_same_v<decltype(sg::PrecheckCpuRecord::series),
                             std::optional<sg::JitterSeriesStats>>,
              "the pre-check cpu carries the same record (FR-004, FR-020)");
static_assert(std::is_same_v<decltype(sg::ReportData::jitterRun),
                             std::optional<sg::JitterRunRecord>>,
              "the loaded report carries the run record (FR-020)");
static_assert(
    std::is_same_v<decltype(sg::JitterRunRecord::method), sg::JitterMethod>,
    "the run record carries the method word (FR-020)");
static_assert(std::is_same_v<decltype(sg::JitterSeriesStats::periods),
                             std::vector<sg::JitterPeriodRecord>>,
              "the series carries its period list (FR-018, E-24)");

// E-23, E-24: the units the record carries. The series figures count ns
// for FWQ and counts for FTQ, the mean and the noise fraction stand
// reals, and one period names its ns and its power ratio.
static_assert(
    std::is_same_v<decltype(sg::JitterSeriesStats::minValue), std::uint64_t>,
    "one series figure carries a count (E-23)");
static_assert(
    std::is_same_v<decltype(sg::JitterSeriesStats::meanValue), double>,
    "the series mean stands a real (E-23)");
static_assert(
    std::is_same_v<decltype(sg::JitterSeriesStats::noisePercent), double>,
    "the series noise figure stands a real (FR-009, E-23)");
static_assert(
    std::is_same_v<decltype(sg::JitterSeriesStats::unitTimeNs), std::uint64_t>,
    "the calibration discloses the measured unit time (FR-011, E-22)");
static_assert(
    std::is_same_v<decltype(sg::JitterPeriodRecord::periodNs), std::uint64_t>,
    "one period names its period in ns (E-24)");
static_assert(
    std::is_same_v<decltype(sg::JitterPeriodRecord::powerRatio), double>,
    "one period carries its power ratio (E-24)");


// ============================================================================
// T011: the calibration one probe cpu runs on its own cpu before its own
// window (US1). The search doubles a count from 1 in at most 25 trials,
// narrows by bisection in at most 24 steps, reads the clock twice around
// each trial's body, keeps the closest count it measured, and stands its
// span out of the window's runtime (FR-010, FR-011, D-2, R-04, R-05).
// Each row scripts the clock through the reader-install seam and asserts
// its own read count: the prefix the search costs, then the standing
// 1,024-read measurement pass, then the read behind the latch, then one
// read per unit (FR-012, R-01). The two known cases stand at 2 reads for
// a delta at or above the target and 98 reads for a delta below it.
// ============================================================================

// The call slots the scripted clock mirrors, one per probe thread.
inline constexpr std::size_t kScriptSlots = 4;

// The spin bound of the two waits: the reader's park inside its last
// planned read, and the case's watch of the call counts. The bound turns
// a clock that never reaches its plan into a reported failure, so no
// wait hangs and no test sleeps (FR-024).
inline constexpr std::uint64_t kScriptSpinCap = 1000000000ULL;

// The standing measurement pass, the read behind the latch, and one read
// per unit: the layout a scripted series carries behind its calibration
// prefix (FR-012, R-01).
inline constexpr std::uint64_t kScriptPassReads = 1024;
inline constexpr std::uint64_t kScriptLatchRead = 1;

// The layout one scripted case reads: the calibration prefix as the
// delta before each prefix read, the in-pass step, the delta of one
// window unit, and the unit count behind the latch read.
struct ScriptPlan
{
  std::vector<std::uint64_t> prefixDeltas {};
  std::uint64_t passStep = 1;
  std::uint64_t windowStep = 1000;
  std::uint64_t units = 4;
  std::uint64_t planned = 0;
};

ScriptPlan scriptPlan;

// The per-thread state of the scripted clock: the read count, the stamp
// cursor, and the slot this thread mirrors to (R-02, FR-039).
struct ScriptThread
{
  std::uint64_t calls = 0;
  std::uint64_t stamp = 0;
  std::uint64_t ticket = 0;
};

thread_local ScriptThread scriptThread;
std::atomic<std::uint64_t> scriptTicket {0};
std::atomic<std::uint64_t> scriptCalls[kScriptSlots] {};
std::atomic<bool> scriptGateOpen {false};

// The plan's read count: the prefix, the standing pass, the read behind
// the latch, and one read per unit (FR-012).
auto scriptTotal(const ScriptPlan& plan) -> std::uint64_t
{
  return plan.prefixDeltas.size() + kScriptPassReads + kScriptLatchRead
      + plan.units;
}

// The delta the scripted clock answers before read `call`: the prefix
// table first, then the pass step, then the window step.
auto scriptedDelta(const std::uint64_t call) -> std::uint64_t
{
  if (call <= scriptPlan.prefixDeltas.size()) {
    return scriptPlan.prefixDeltas.at(static_cast<std::size_t>(call) - 1);
  }
  return call <= scriptPlan.prefixDeltas.size() + kScriptPassReads
      ? scriptPlan.passStep
      : scriptPlan.windowStep;
}

// The scripted clock: one per-thread stamp series over the plan, parked
// inside its last planned read until the case opens the gate. The park
// makes the read count exact: the loop stands in its last read, records
// that unit, and the stop flag ends it (FR-024, R-08).
class ScriptedReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    auto& thread = scriptThread;
    if (thread.ticket == 0) {
      thread.ticket = 1 + scriptTicket.fetch_add(1, std::memory_order_relaxed);
    }
    ++thread.calls;
    if (thread.ticket <= kScriptSlots) {
      scriptCalls[thread.ticket - 1].store(thread.calls,
                                           std::memory_order_relaxed);
    }
    if (thread.calls >= scriptPlan.planned) {
      std::uint64_t spins = 0;
      while (!scriptGateOpen.load(std::memory_order_acquire)) {
        std::this_thread::yield();
        if (++spins > kScriptSpinCap) {
          break;
        }
      }
    }
    thread.stamp += scriptedDelta(thread.calls);
    sink.put(thread.stamp);
  }
};

auto scriptReset() -> void
{
  scriptTicket.store(0, std::memory_order_relaxed);
  scriptGateOpen.store(false, std::memory_order_relaxed);
  for (auto& slot : scriptCalls) {
    slot.store(0, std::memory_order_relaxed);
  }
}

// The cpu indexes the scenarios pin probe threads to, taken from the
// process affinity mask, each below the online count (FR-024). No cpu
// number stands fixed in this file.
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-024)");
  }
  const int online = CPU_COUNT(&mask);
  check(online > 0, "the affinity mask holds cpus (FR-024)");
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask) != 0) {
      check(index < static_cast<std::size_t>(online),
            "a picked cpu stands below the online count (FR-024)");
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (FR-024)");
  return cpus;
}

// One scripted case: install the clock, start the probe, and open the
// gate once every probe thread stands parked in its last planned read.
// The watch spins on the reader's own per-thread counts, yields, and
// bounds itself, so a clock that never reaches its plan answers a
// reported failure (FR-024, FR-039).
template<class Assert>
auto runScripted(const sg::JitterProbeOptions& options,
                 const std::size_t cpuCount,
                 Assert&& assertions) -> void
{
  scriptPlan.planned = scriptTotal(scriptPlan);
  scriptReset();
  sg::detail::installTestReader(std::make_unique<ScriptedReader>());
  sg::detail::JitterProbe probe(options);
  probe.start();
  for (std::size_t slot = 0; slot < cpuCount; ++slot) {
    std::uint64_t spins = 0;
    while (scriptCalls[slot].load(std::memory_order_relaxed)
           < scriptPlan.planned)
    {
      std::this_thread::yield();
      if (++spins > kScriptSpinCap) {
        fail("the scripted clock never reached its plan (FR-024)");
      }
    }
  }
  scriptGateOpen.store(true, std::memory_order_release);
  probe.stopAndJoin();
  sg::detail::installTestReader(nullptr);
  assertions(probe.states());
}

// The calibration prefix the case scripts: `prefixReads` reads, the pair
// around each trial's body, whose deltas stand in `trialDeltas`. Every
// builder takes the prefix length as a parameter (FR-010, D-2).
auto calibrationPlan(const std::size_t prefixReads,
                     const std::vector<std::uint64_t>& trialDeltas,
                     const std::uint64_t units) -> void
{
  scriptPlan = ScriptPlan {};
  scriptPlan.units = units;
  check(2 * trialDeltas.size() == prefixReads,
        "the prefix stands at the pair of reads of every trial (FR-010)");
  for (const auto delta : trialDeltas) {
    scriptPlan.prefixDeltas.push_back(delta);
    scriptPlan.prefixDeltas.push_back(delta);
  }
}

// `trials` trials measuring one delta: the shape of the clock that never
// passes the target, and of the clock that passes at its first trial.
auto flatTrialDeltas(const std::size_t trials, const std::uint64_t delta)
    -> std::vector<std::uint64_t>
{
  return std::vector<std::uint64_t>(trials, delta);
}

// The kept samples of one probe cpu's series, the set the statistics and
// the spectrum read (FR-022).
auto seriesSum(const sg::detail::ProbeCpuState& state) -> std::uint64_t
{
  std::uint64_t total = 0;
  for (std::size_t index = 0; index < state.seriesKept; ++index) {
    total += state.series.at(index);
  }
  return total;
}

// FR-010, FR-011, D-2, R-04: a delta at the target at the first trial
// ends the search there. The prefix stands at 2 reads, and the search
// keeps the count 1 with the unit time it measured.
auto calibrationStopsAtTheFirstTrial() -> void
{
  calibrationPlan(2, {1000}, 4);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptCalls[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead + 4,
              "the search costs 2 reads for a delta at the target (FR-010)");
        check(probed.workCount == 1,
              "the first trial's count stands kept (FR-010)");
        check(probed.unitTimeNs == 1000,
              "the measured unit time answers the scripted delta (FR-011)");
        check(probed.targetNs == sg::kTargetUnitTimeNs,
              "the FWQ target stands at 1 us (FR-010)");
        check(probed.calibrationSpanNs == 1000,
              "the span covers the trial's two reads (FR-011)");
        check(probed.loopPeriodNs == 1,
              "the pass measured its own step behind the search (FR-012)");
        check(probed.seriesKept == 4 && seriesSum(probed) == 4000,
              "the window ran its units behind the search (FR-012)");
      });
}

// FR-010, R-02: a clock whose delta never passes the target runs the
// doubling to its trial cap, 25 trials, and the search ends there. The
// prefix stands at 98 reads, the cap of the whole search.
auto calibrationStopsAtTheTrialCap() -> void
{
  calibrationPlan(98, flatTrialDeltas(49, 1), 4);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptCalls[0].load()
                  == 98 + kScriptPassReads + kScriptLatchRead + 4,
              "the search costs 98 reads at its trial cap (FR-010, R-02)");
        check(probed.workCount == 1,
              "the closest of the measured counts stands kept (FR-010)");
        check(probed.unitTimeNs == 1,
              "the closest unit time answers the scripted delta (FR-011)");
      });
}

// FR-010, R-02: a frozen clock answers a unit time of 0 at every count,
// so the doubling runs to the count cap 16,777,216 and the bisection
// runs its 24 steps over the whole range. The prefix stands at 98 reads.
auto calibrationStopsAtTheCountCap() -> void
{
  calibrationPlan(98, flatTrialDeltas(49, 0), 4);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptCalls[0].load()
                  == 98 + kScriptPassReads + kScriptLatchRead + 4,
              "the search costs 98 reads at its count cap (FR-010, R-02)");
        check(probed.workCount == 1,
              "the first measured count stands kept among equal unit times "
              "(FR-010)");
        check(probed.unitTimeNs == 0,
              "the frozen clock answers a unit time of 0 (FR-011)");
      });
}

// FR-010: a delta below the target at count 1 and above it at count 2
// closes the bisection interval at one count, so the search ends after
// 4 reads and keeps the count whose unit time stands closest.
auto calibrationStopsAtTheOneCountInterval() -> void
{
  calibrationPlan(4, {500, 2500}, 4);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptCalls[0].load()
                  == 4 + kScriptPassReads + kScriptLatchRead + 4,
              "the one-count interval ends the search at 4 reads (FR-010)");
        check(probed.workCount == 2,
              "the count whose unit time stands closest stands kept (FR-010)");
        check(probed.unitTimeNs == 1250,
              "the kept unit time answers the delta over the count (FR-011)");
      });
}

// FR-010, D-2: the search keeps the closest count it measured. The
// doubling passes the target at count 8, the bisection reads counts 6
// and 5, and the count 6 answers the target exactly.
auto calibrationKeepsTheClosestCount() -> void
{
  calibrationPlan(12, {100, 100, 100, 100000, 6000, 100000}, 4);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptCalls[0].load()
                  == 12 + kScriptPassReads + kScriptLatchRead + 4,
              "the search costs 12 reads over its six trials (FR-010)");
        check(probed.workCount == 6,
              "the closest of the six measured counts stands kept (FR-010)");
        check(probed.unitTimeNs == 1000,
              "the kept unit time answers the delta over the count (FR-011)");
      });
}

// FR-010, D-2: the search runs once per probe cpu, on that cpu. Two
// probe cpus each read their own prefix of 2 reads, and each state
// carries its own calibration.
auto calibrationRunsOncePerProbeCpu() -> void
{
  calibrationPlan(2, {1000}, 4);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(2);
  options.method = sg::JitterMethod::FWQ;
  runScripted(options,
              2,
              [](const auto& states)
              {
                check(states.size() == 2,
                      "the two probe cpus stand probed (FR-010)");
                for (std::size_t slot = 0; slot < 2; ++slot) {
                  check(scriptCalls[slot].load() == 2 + kScriptPassReads
                    + kScriptLatchRead + 4,
                "each probe cpu reads its own calibration prefix once "
                "(FR-010)");
                  const auto& probed = states.at(slot);
                  check(probed.workCount == 1 && probed.unitTimeNs == 1000
                            && probed.targetNs == sg::kTargetUnitTimeNs,
                        "each probe cpu carries its own calibration (FR-011)");
                }
              });
}

// FR-010, FR-011, R-05: the calibration span stands out of the window's
// runtime. The prefix reads a stamp 5 ms apart, the window reads a stamp
// 1 us apart, and the kept unit times answer the window's own span.
auto calibrationStaysOutOfTheWindowRuntime() -> void
{
  calibrationPlan(2, {5000000}, 10);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(probed.calibrationSpanNs == 5000000,
              "the span covers the calibration's own reads (FR-011)");
        check(probed.seriesKept == 10 && seriesSum(probed) == 10000,
              "the window's unit times answer the window's own span, the "
              "calibration span stands out of it (FR-010, R-05)");
        check(probed.loopPeriodNs == 1,
              "the pass measured its own step behind the search (FR-012)");
      });
}

}  // namespace

auto main() -> int
{
  // FR-001, FR-002, E-27: the defaults a value with no new key leaves
  // behind, the reading a run with no `method` key keeps today.
  const sg::JitterProbeOptions options;
  check(options.method == sg::JitterMethod::TSC,
        "the default method is TSC (FR-001)");
  check(options.quantumUs == 100U, "the default quantum reads 100 us (FR-002)");
  check(options.quantumUs == sg::kDefaultQuantumUs,
        "the quantum default reads its bound (FR-002)");
  check(options.workCount == 0U,
        "the default work count is 0, the value naming the calibration "
        "(FR-002, FR-010)");
  check(options.noisePercent == 1U,
        "the default noise percent reads 1 (FR-002, FR-021)");
  check(options.noisePercent == sg::kDefaultNoisePercent,
        "the noise default reads its bound (FR-002)");

  // FR-002: the three ranges the parser of T007 enforces at the
  // command-line boundary.
  check(sg::kMinQuantumUs == 10U && sg::kMaxQuantumUs == 100000U,
        "the quantum range reads 10 to 100,000 us (FR-002)");
  check(sg::kMaxWorkCount == 16777216U,
        "the work count cap reads 16,777,216 bodies (FR-002, FR-010)");
  check(options.workCount == 0U,
        "0 stands below the work range's low bound 1, naming the "
        "calibration (FR-002, FR-010)");
  check(1U <= sg::kDefaultNoisePercent && sg::kDefaultNoisePercent <= 100U,
        "the noise default stands inside the 0 to 100 percent range (FR-002)");

  // FR-010: the calibration stops the search reads, the 1 us FWQ target
  // and the two caps.
  check(sg::kTargetUnitTimeNs == 1000U,
        "the FWQ unit target reads 1 us (FR-010)");
  check(sg::kCalibrationTrials == 25U && sg::kCalibrationSteps == 24U,
        "the search stops at 25 trials and 24 bisection steps (FR-010)");

  // FR-020: the two records stand empty until a run fills them.
  const sg::JitterEntryFold fold;
  check(!fold.series.has_value(),
        "a default fold carries no series record (FR-020)");
  const sg::ReportData report;
  check(!report.jitterRun.has_value(),
        "a default loaded report carries no run record (FR-020)");

  // T011: the calibration rows (US1, FR-010, FR-011, D-2, R-04, R-05).
  calibrationStopsAtTheFirstTrial();
  calibrationStopsAtTheTrialCap();
  calibrationStopsAtTheCountCap();
  calibrationStopsAtTheOneCountInterval();
  calibrationKeepsTheClosestCount();
  calibrationRunsOncePerProbeCpu();
  calibrationStaysOutOfTheWindowRuntime();

  std::puts("harness_fwq_ftq_test: ok");
  return 0;
}
