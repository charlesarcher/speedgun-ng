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

#include <algorithm>
#include <atomic>
#include <cmath>
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
#include "detail/spectrum.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"

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

// The sine's full turn, the one constant the planted dip reads.
inline constexpr double kTwoPi = 6.283185307179586;

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
// window unit, and the unit count behind the latch read. The window
// reads answer one delta each, moved by the planted slow unit, the
// planted dip period, and the ramp step.
struct ScriptPlan
{
  std::vector<std::uint64_t> prefixDeltas {};
  std::uint64_t passStep = 1;
  std::uint64_t windowStep = 1000;
  std::uint64_t units = 4;
  std::uint64_t planned = 0;
  std::uint64_t slowUnit = 0;
  std::uint64_t slowDelta = 0;
  std::uint64_t dipPeriod = 0;
  std::uint64_t dipDelta = 0;
  std::uint64_t rampStep = 0;
  std::uint64_t sinePeriod = 0;
  std::int64_t sineAmplitude = 0;
  bool park = true;
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
// The read count of one probe thread at the moment the case opened the
// gate: the count the clock reached on its own, before the stop flag
// lands. The loop keeps reading while the stop travels, so this mirror,
// not the live count, carries the run's read count (FR-010, FR-024).
std::atomic<std::uint64_t> scriptParked[kScriptSlots] {};
std::atomic<bool> scriptGateOpen {false};

// The plan's read count: the prefix, the standing pass, the read behind
// the latch, and one read per unit (FR-012).
auto scriptTotal(const ScriptPlan& plan) -> std::uint64_t
{
  return plan.prefixDeltas.size() + kScriptPassReads + kScriptLatchRead
      + plan.units;
}

// The delta the scripted clock answers before read `call`: the prefix
// table first, then the pass step, then the delta of one window unit.
auto scriptedDelta(const std::uint64_t call) -> std::uint64_t
{
  if (call <= scriptPlan.prefixDeltas.size()) {
    return scriptPlan.prefixDeltas.at(static_cast<std::size_t>(call) - 1);
  }
  if (call <= scriptPlan.prefixDeltas.size() + kScriptPassReads) {
    return scriptPlan.passStep;
  }
  const auto unit = call - scriptPlan.prefixDeltas.size() - kScriptPassReads;
  if (scriptPlan.slowUnit != 0 && unit == scriptPlan.slowUnit + 1) {
    return scriptPlan.slowDelta;
  }
  if (scriptPlan.dipPeriod != 0 && unit % scriptPlan.dipPeriod == 1) {
    return scriptPlan.dipDelta;
  }
  if (scriptPlan.rampStep != 0) {
    return scriptPlan.rampStep * (unit - 1);
  }
  if (scriptPlan.sinePeriod != 0) {
    const auto phase = kTwoPi * static_cast<double>(unit)
        / static_cast<double>(scriptPlan.sinePeriod);
    return scriptPlan.windowStep
        + static_cast<std::uint64_t>(
               static_cast<double>(scriptPlan.sineAmplitude) * std::sin(phase));
  }
  return scriptPlan.windowStep;
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
    if (scriptPlan.park && thread.calls >= scriptPlan.planned) {
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
  for (auto& slot : scriptParked) {
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

// The scripted provider: one FakeProvider declaring the machine/tsc leaf
// at ReadMode::FAST_TSC, registered once per process, the shape
// harness_jitter_probe_test carries. The probe's open asks the catalog
// for that leaf and refuses the run without it, which leaves the
// scripted clock with no read at all (FR-024, R-02).
auto scriptedTscProvider() -> void
{
  static const bool registered = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    owned->addCounter("machine",
                      "tsc",
                      "none",
                      "raw tick counter",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::FAST_TSC);
    const auto ok =
        sg::counters::System::local().registerProvider(std::move(owned));
    check(ok.has_value(), "the scripted tsc provider registers (FR-024)");
    return true;
  }();
  check(registered, "the scripted tsc provider stands registered (FR-024)");
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
    scriptParked[slot].store(scriptCalls[slot].load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
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
        check(scriptParked[0].load()
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
        check(scriptParked[0].load()
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
        check(scriptParked[0].load()
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
        check(scriptParked[0].load()
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
        check(scriptParked[0].load()
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
                  check(scriptParked[slot].load() == 2 + kScriptPassReads
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

// ============================================================================
// T012: the fixed work quanta series (US1). One unit is the calibrated
// loop body, timed by one read, and the unit time stands at the delta
// between two consecutive reads; every method runs the standing 1,024-
// read measurement pass, so a scripted series carries the calibration
// prefix, then 1,024 pass stamps, then one stamp per unit, then the read
// behind the latch (FR-004, FR-007, FR-008, FR-009, FR-012, FR-014,
// FR-022, SC-001, R-08, R-09, R-10, R-14). Each row carries its read-
// count check over the scripted layout.
// ============================================================================

// The unit index of the planted slow unit, 1-based, and its delta: the
// shape of the series with one slow unit (SC-001).
inline constexpr std::uint64_t kSlowUnitIndex = 5000;
inline constexpr std::uint64_t kSlowUnitDelta = 10000;

// The dip the resample row plants: one slow unit every 1,000 units at a
// 1 us unit time (FR-025).
inline constexpr std::uint64_t kDipPeriod = 1000;
inline constexpr std::uint64_t kDipDelta = 10000;

// The FWQ layout the row scripts: the calibration prefix of 2 reads at
// the target, the in-pass step, the delta of one unit, and the unit
// count behind the latch read (FR-012, R-01).
auto fwqPlan(const std::uint64_t units, const std::uint64_t windowStep) -> void
{
  calibrationPlan(2, {1000}, units);
  scriptPlan.passStep = 1;
  scriptPlan.windowStep = windowStep;
}

// The kept samples of one probe cpu's series, sorted: the set the
// statistics read (FR-008, FR-022).
auto seriesSorted(const sg::detail::ProbeCpuState& state)
    -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> sorted;
  sorted.reserve(state.seriesKept);
  for (std::size_t index = 0; index < state.seriesKept; ++index) {
    sorted.push_back(state.series.at(index));
  }
  std::sort(sorted.begin(), sorted.end());
  return sorted;
}

// The nearest-rank point of the kept samples, the rule of spec 021 the
// fold reads: the rank stands at the point times the sample count,
// rounded up (FR-008).
auto seriesRank(const std::vector<std::uint64_t>& sorted, const double point)
    -> std::uint64_t
{
  const auto rank = static_cast<std::size_t>(
      std::ceil(point * static_cast<double>(sorted.size()) / 100.0));
  return sorted.at(rank - 1);
}

// The FWQ noise fraction of the kept samples: the kept unit times minus
// the sample count times the minimum unit time, over the kept unit
// times, times 100 (FR-009, R-3).
auto fwqNoisePercent(const sg::detail::ProbeCpuState& state) -> double
{
  const auto sorted = seriesSorted(state);
  const auto total = seriesSum(state);
  if (total == 0) {
    return 0.0;
  }
  const auto spread = static_cast<double>(total)
      - static_cast<double>(sorted.size())
          * static_cast<double>(sorted.front());
  return spread * 100.0 / static_cast<double>(total);
}

// FR-007, FR-012, FR-013, SC-001: a clean series of 1,000 equal units
// answers equal figures, keeps every sample, and reads 2,027 reads
// behind a prefix of 2.
auto fwqCleanSeriesAnswersEqualFigures() -> void
{
  fwqPlan(1000, 1000);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load() == 2 + kScriptPassReads + kScriptLatchRead
                  + 1000,
              "the clean 1,000-unit case stands 2,027 reads behind a prefix "
              "of 2 (FR-012, R-01)");
        check(probed.seriesKept == 1000,
              "the series keeps every unit of a clean run (FR-007)");
        check(probed.droppedSamples == 0,
              "the buffer never filled (FR-014, FR-022)");
        const auto sorted = seriesSorted(probed);
        check(sorted.front() == 1000 && sorted.back() == 1000,
              "the minimum and the maximum answer the equal unit time "
              "(FR-007)");
        check(seriesRank(sorted, 50.0) == 1000 && seriesRank(sorted, 99.99) == 1000,
              "the median and the last rank answer the equal unit time "
              "(FR-007, FR-008)");
        check(seriesSum(probed) == 1000000,
              "the kept unit times answer the window's own span (FR-007)");
      });
}

// FR-008, SC-001: 10,000 units with one unit at ten times the unit time
// move the maximum, leave the median at the equal value, and read
// 11,027 reads.
auto fwqOneSlowUnitMovesTheMaximum() -> void
{
  fwqPlan(10000, 1000);
  scriptPlan.slowUnit = kSlowUnitIndex;
  scriptPlan.slowDelta = kSlowUnitDelta;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead + 10000,
              "the 10,000-unit case stands 11,027 reads (FR-012, R-01)");
        check(probed.seriesKept == 10000 && probed.droppedSamples == 0,
              "the series keeps all 10,000 samples (FR-007, FR-014)");
        const auto sorted = seriesSorted(probed);
        check(sorted.back() == kSlowUnitDelta,
              "the maximum names the slow unit (SC-001)");
        check(sorted.front() == 1000,
              "the minimum stands at the equal unit time (FR-007)");
        check(seriesRank(sorted, 50.0) == 1000,
              "the median stands at the equal unit time (FR-008)");
        check(probed.series.at(kSlowUnitIndex - 1) == kSlowUnitDelta,
              "the slow unit stands where the layout planted it (FR-012)");
      });
}

// FR-009, SC-001: the noise fraction of that series reads 0.09 percent:
// the kept unit times minus the sample count times the minimum, over the
// kept unit times.
auto fwqNoiseFractionOfASlowUnit() -> void
{
  fwqPlan(10000, 1000);
  scriptPlan.slowUnit = kSlowUnitIndex;
  scriptPlan.slowDelta = kSlowUnitDelta;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(options,
              1,
              [](const auto& states)
              {
                const auto& probed = states.at(0);
                check(scriptParked[0].load()
                          == 2 + kScriptPassReads + kScriptLatchRead + 10000,
                      "the row reads the same 11,027 reads (FR-012, R-01)");
                check(probed.seriesKept == 10000,
                      "the fraction reads the kept samples (FR-022)");
                check(std::fabs(fwqNoisePercent(probed) - 0.09) <= 0.001,
              "the noise fraction of one slow unit reads 0.09 percent "
              "(FR-009, SC-001)");
              });
}

// FR-014, FR-022: a buffer of 2 entries keeps the first two samples,
// counts the two the full buffer lost, and the run keeps going.
auto fwqBufferFullCountsDrops() -> void
{
  fwqPlan(4, 500);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  options.gapCount = 2;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead + 4,
              "the gaps=2 case reads its 1,031 stamps (FR-012, R-01)");
        check(probed.seriesKept == 2,
              "the gaps=2 buffer keeps 2 samples (FR-014)");
        check(probed.droppedSamples == 2,
              "the full buffer counts each lost record (FR-014, FR-022)");
        check(probed.series.at(0) == 500 && probed.series.at(1) == 500,
              "the kept samples answer the scripted unit time (FR-012)");
      });
}

// FR-008, FR-022: the statistics read the kept samples, in the order the
// series wrote them, and the buffer holds no sorted copy.
auto seriesPercentilesReadTheKeptSamples() -> void
{
  fwqPlan(100, 10);
  scriptPlan.rampStep = 10;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead + 100,
              "the row reads its 1,127 stamps (FR-012, R-01)");
        check(probed.seriesKept == 100 && probed.droppedSamples == 0,
              "the series keeps all 100 samples (FR-007, FR-014)");
        bool readOrder = true;
        for (std::size_t index = 0; index < probed.seriesKept; ++index) {
          readOrder = readOrder && probed.series.at(index) == 10 * (index + 1);
        }
        check(readOrder,
              "the buffer holds the kept samples in read order (FR-008)");
        const auto sorted = seriesSorted(probed);
        check(seriesRank(sorted, 50.0) == 500 && seriesRank(sorted, 90.0) == 900
                  && seriesRank(sorted, 99.0) == 990
                  && seriesRank(sorted, 99.99) == 1000,
              "the ranks the fold reads answer the kept samples (FR-008)");
      });
}

// FR-007, FR-008: a series of one sample reads that sample.
auto oneSampleSeriesReadsThatSample() -> void
{
  fwqPlan(1, 4000);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead + 1,
              "the one-sample case reads 1,028 stamps (FR-012, R-01)");
        check(probed.seriesKept == 1 && probed.series.at(0) == 4000,
              "the series of one sample reads that sample (FR-007, FR-008)");
        check(probed.droppedSamples == 0, "the buffer never filled (FR-014)");
      });
}

// FR-017, FR-025: the resample path reads the FWQ series of a dip every
// 1,000 units at a 1 us unit time. The series keeps all 65,536 samples,
// the median unit time stands the resample step, and the row reads
// 66,563 reads over 65,536 units.
auto fwqResampleReachesThePeriodList() -> void
{
  fwqPlan(65536, 1000);
  scriptPlan.dipPeriod = kDipPeriod;
  scriptPlan.dipDelta = kDipDelta;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load() == 2 + kScriptPassReads + kScriptLatchRead
                  + 65536,
              "the dip case stands 66,563 reads over 65,536 units "
              "(FR-012, R-01)");
        check(probed.seriesKept == 65536,
              "the default buffer keeps 65,536 samples (FR-014)");
        check(probed.droppedSamples == 0,
              "the series of the cap fills the buffer with no drop (FR-022)");
        bool dips = true;
        for (std::size_t index = 0; index < probed.seriesKept; ++index) {
          const auto expected =
              (index + 1) % kDipPeriod == 0 ? kDipDelta : std::uint64_t {1000};
          dips = dips && probed.series.at(index) == expected;
        }
        check(dips,
              "the dip stands at every 1,000th unit of the series (FR-025)");
        const auto sorted = seriesSorted(probed);
        check(seriesRank(sorted, 50.0) == 1000,
              "the median unit time stands the resample step (FR-017)");
      });
}

// FR-004: mode=precheck runs the selected method. The pre-check phase of
// a FWQ run records unit times, and the phase ends at its scripted span.
auto precheckModeRunsTheSelectedMethod() -> void
{
  fwqPlan(0, 1000);
  scriptPlan.park = false;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FWQ;
  options.mode = sg::JitterProbeMode::PRECHECK;
  options.precheckMs = 1;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load() == 2 + kScriptPassReads + 1000
                  + kScriptLatchRead,
              "the pre-check phase reads its scripted span, then the read "
              "behind the latch (FR-004, FR-012)");
        check(probed.seriesKept == 1000,
              "the pre-check phase runs the selected method (FR-004)");
        check(probed.droppedSamples == 0 && seriesSum(probed) == 1000000,
              "the pre-check series answers the scripted unit time "
              "(FR-004, FR-012)");
      });
}

// ============================================================================
// T013: the fixed time quanta series (US2). Edge k stands at the window's
// first stamp plus k times the quantum, the read compares against the
// next edge, the pass stores the count at the edge, resets it and
// advances the edge by one quantum, and a trailing open quantum records
// no count (FR-009, FR-015, FR-016, FR-022, FR-025, SC-002, D-4, R-07,
// R-08). The FTQ target stands at one thousandth of the quantum, so a
// 100 us quantum calibrates a 100 ns unit and one quantum holds 1,000
// units.
// ============================================================================

// The unit delta of a scripted FTQ window: 100 ns, so a 100 us quantum
// holds 1,000 units and the FTQ target of 100 ns stands calibrated by
// one trial (FR-010, FR-015).
inline constexpr std::uint64_t kFtqUnitDelta = 100;
inline constexpr std::uint64_t kFtqQuantumUs = 100;
inline constexpr std::uint64_t kFtqUnitsPerQuantum = 1000;

// The FTQ layout the row scripts: the calibration prefix of 2 reads at
// the quantum's target, the in-pass step, the delta of one unit, and the
// unit count behind the latch read (FR-010, FR-015, R-07).
auto ftqPlan(const std::uint64_t quantumUs,
             const std::uint64_t units,
             const std::uint64_t windowStep) -> void
{
  calibrationPlan(2, {quantumUs}, units);
  scriptPlan.passStep = 1;
  scriptPlan.windowStep = windowStep;
}

// The FTQ noise fraction of the kept counts: the sample count times the
// maximum count, minus the sum of the counts, over the sample count
// times the maximum count, times 100; a maximum count of 0 answers 100
// (FR-009, R-3).
auto ftqNoisePercent(const sg::detail::ProbeCpuState& state) -> double
{
  const auto sorted = seriesSorted(state);
  const auto maximum = sorted.empty() ? std::uint64_t {0} : sorted.back();
  if (maximum == 0) {
    return 100.0;
  }
  const auto span =
      static_cast<double>(sorted.size()) * static_cast<double>(maximum);
  return (span - static_cast<double>(seriesSum(state))) * 100.0 / span;
}

// FR-009, FR-015, FR-016, SC-002: 100 quanta at a 100 us quantum keep
// 100 equal counts, stand 101,027 reads, and answer equal figures.
auto ftqCleanCountsAnswerEqualFigures() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 100, kFtqUnitDelta);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load() == 2 + kScriptPassReads + kScriptLatchRead
                  + kFtqUnitsPerQuantum * 100,
              "the 100-quanta case at a 100 us quantum stands 101,027 reads "
              "(FR-015, R-08)");
        check(probed.seriesKept == 100,
              "the 100-quanta case keeps 100 counts (FR-015, FR-016)");
        check(probed.droppedSamples == 0,
              "the buffer never filled (FR-014, FR-022)");
        const auto sorted = seriesSorted(probed);
        check(sorted.front() == kFtqUnitsPerQuantum
                  && sorted.back() == kFtqUnitsPerQuantum,
              "the minimum and the maximum of a clean run stand equal "
              "(SC-002)");
        check(seriesRank(sorted, 50.0) == kFtqUnitsPerQuantum,
              "the median stands with them (FR-008, SC-002)");
        check(probed.quantumNs == 100000,
              "the quantum of 100 us reads 100,000 ns (FR-015, E-22)");
      });
}

// FR-009, FR-015: 1,000 quanta with one quantum at half the best count
// keep 1,000 counts and move the minimum.
auto ftqOneLowCountMovesTheMinimum() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 1000, kFtqUnitDelta);
  scriptPlan.slowUnit = 1;
  scriptPlan.slowDelta = 50100;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead
                      + kFtqUnitsPerQuantum * 1000,
              "the 1,000-quanta case reads one stamp per unit (FR-015, R-08)");
        check(probed.seriesKept == 1000,
              "the 1,000-quanta case keeps 1,000 counts (FR-015, FR-016)");
        const auto sorted = seriesSorted(probed);
        check(sorted.front() == kFtqUnitsPerQuantum / 2,
              "the minimum names the low quantum at half the best count "
              "(FR-009, FR-015)");
        check(sorted.back() == kFtqUnitsPerQuantum
                  && seriesRank(sorted, 50.0) == kFtqUnitsPerQuantum,
              "the maximum and the median stand at the best count (FR-016)");
      });
}

// FR-009: the noise fraction of that series reads 0.05 percent: the
// sample count times the maximum count, minus the sum of the counts.
auto ftqNoiseFractionOfALowQuantum() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 1000, kFtqUnitDelta);
  scriptPlan.slowUnit = 1;
  scriptPlan.slowDelta = 50100;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(probed.seriesKept == 1000,
              "the fraction reads the kept counts (FR-022)");
        check(std::fabs(ftqNoisePercent(probed) - 0.05) <= 0.001,
              "the noise fraction of one low quantum reads 0.05 percent "
              "(FR-009, R-3)");
      });
}

// FR-009: a window shorter than one quantum keeps no count, the maximum
// count stands 0, and the fraction answers 100.
auto ftqZeroMaximumAnswersOneHundred() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum / 2, kFtqUnitDelta);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead
                      + kFtqUnitsPerQuantum / 2,
              "the half-quantum window reads its units and stops (FR-015)");
        check(probed.seriesKept == 0,
              "an open quantum records no count (FR-015, FR-016)");
        check(std::fabs(ftqNoisePercent(probed) - 100.0) <= 1e-9,
              "a maximum count of 0 answers 100 (FR-009)");
      });
}

// FR-015, R-04, R-07: edge k stands at the window's first stamp plus k
// times the quantum. The prefix reads a stamp 5 ms apart, so the window
// opens at a stamp no absolute quantum edge names, and the counts stand
// equal.
auto ftqEdgesStandFromTheFirstStamp() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 100, kFtqUnitDelta);
  scriptPlan.prefixDeltas = {5000000, 5000000};
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(probed.calibrationSpanNs == 5000000,
              "the window opens 10,001,124 ns into the scripted clock "
              "(FR-011, FR-015)");
        check(probed.seriesKept == 100,
              "the shifted window keeps its 100 counts (FR-015)");
        const auto sorted = seriesSorted(probed);
        check(sorted.front() == kFtqUnitsPerQuantum
                  && sorted.back() == kFtqUnitsPerQuantum,
              "the edges of the window's first stamp hold 1,000 units each "
              "(FR-015, R-07)");
      });
}

// FR-015: a trailing open quantum records no count.
auto ftqTrailingOpenQuantumRecordsNothing() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 100 + 500, kFtqUnitDelta);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(options,
              1,
              [](const auto& states)
              {
                const auto& probed = states.at(0);
                check(scriptParked[0].load()
                          == 2 + kScriptPassReads + kScriptLatchRead
                              + kFtqUnitsPerQuantum * 100 + 500,
                      "the row reads its 500 units behind the 100th edge "
                      "(FR-015)");
                check(probed.seriesKept == 100,
                      "the trailing open quantum records no count (FR-015)");
                check(probed.series.at(99) == kFtqUnitsPerQuantum,
              "the last kept count stands at the last closed quantum "
              "(FR-015)");
              });
}

// FR-014, FR-016, FR-022: a full count buffer keeps its first two
// counts and counts each lost record.
auto ftqBufferFullCountsDrops() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 100, kFtqUnitDelta);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  options.gapCount = 2;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(probed.seriesKept == 2,
              "the gaps=2 buffer keeps 2 counts (FR-014, FR-016)");
        check(probed.droppedSamples == 98,
              "the full count buffer counts each lost record (FR-014, FR-022)");
        check(probed.series.at(0) == kFtqUnitsPerQuantum
                  && probed.series.at(1) == kFtqUnitsPerQuantum,
              "the kept counts answer the scripted quantum (FR-015)");
      });
}

// FR-010, FR-011, E-22: a 10 us quantum sets a 10 ns target, one
// thousandth of the quantum, and the search calibrates it on its own cpu.
auto ftqQuantumOfTenNanosecondsTarget() -> void
{
  ftqPlan(10, 5 * kFtqUnitsPerQuantum, 10);
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = 10;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(scriptParked[0].load()
                  == 2 + kScriptPassReads + kScriptLatchRead
                      + 5 * kFtqUnitsPerQuantum,
              "the row reads 6,027 stamps over 5 quanta (FR-015, R-08)");
        check(probed.targetNs == 10,
              "a 10 us quantum sets a 10 ns target (FR-010, E-22)");
        check(probed.quantumNs == 10000,
              "the quantum of 10 us reads 10,000 ns (FR-015, E-22)");
        check(probed.workCount == 1 && probed.unitTimeNs == 10,
              "the search keeps the count that answers the target (FR-010)");
        check(probed.seriesKept == 5,
              "the 10 us quantum holds 1,000 units of 10 ns (FR-015)");
        check(probed.series.at(0) == kFtqUnitsPerQuantum,
              "the count of the 10 us quantum reads 1,000 (FR-016)");
      });
}

// FR-018, FR-025, SC-002: a dip every 1 ms over 1,000 samples at a 100
// us quantum lands the transform's strongest period inside 990,000 to
// 1,010,000 ns, the one-bin tolerance of R-7.
auto ftqDipEveryMillisecondNamesThePeriod() -> void
{
  ftqPlan(kFtqQuantumUs, kFtqUnitsPerQuantum * 1000, kFtqUnitDelta);
  scriptPlan.sinePeriod = kFtqUnitsPerQuantum * 10;
  scriptPlan.sineAmplitude = 20;
  sg::JitterProbeOptions options;
  options.cpus = pickCpus(1);
  options.method = sg::JitterMethod::FTQ;
  options.quantumUs = kFtqQuantumUs;
  runScripted(
      options,
      1,
      [](const auto& states)
      {
        const auto& probed = states.at(0);
        check(probed.seriesKept == 1000,
              "the dip row keeps its 1,000 counts (FR-015, FR-016)");
        check(probed.series.at(2) < probed.series.at(7)
                  && probed.series.at(2) == probed.series.at(12),
              "the dip lands once every 10 quanta of the series (FR-025)");
        std::vector<double> samples;
        samples.reserve(probed.seriesKept);
        for (std::size_t index = 0; index < probed.seriesKept; ++index) {
          samples.push_back(static_cast<double>(probed.series.at(index)));
        }
        const auto spectrum = sg::detail::dftPeriods(samples, 100000);
        check(!spectrum.periods.empty(),
              "the planted dip reaches the period list (FR-018)");
        check(spectrum.periods.front().periodNs >= 990000
                  && spectrum.periods.front().periodNs <= 1010000,
              "the planted 1 ms period lands inside 990,000 to 1,010,000 ns "
              "(FR-018, SC-002, R-7)");
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
  scriptedTscProvider();
  calibrationStopsAtTheFirstTrial();
  calibrationStopsAtTheTrialCap();
  calibrationStopsAtTheCountCap();
  calibrationStopsAtTheOneCountInterval();
  calibrationKeepsTheClosestCount();
  calibrationRunsOncePerProbeCpu();
  calibrationStaysOutOfTheWindowRuntime();

  // T012: the FWQ series rows (US1, FR-004, FR-007 to FR-009, FR-012,
  // FR-014, FR-022, SC-001, R-08, R-09, R-10, R-14).
  fwqCleanSeriesAnswersEqualFigures();
  fwqOneSlowUnitMovesTheMaximum();
  fwqNoiseFractionOfASlowUnit();
  fwqBufferFullCountsDrops();
  seriesPercentilesReadTheKeptSamples();
  oneSampleSeriesReadsThatSample();
  fwqResampleReachesThePeriodList();
  precheckModeRunsTheSelectedMethod();

  // T013: the FTQ rows (US2, FR-009, FR-015, FR-016, FR-022, FR-025,
  // SC-002, D-4, R-07, R-08).
  ftqCleanCountsAnswerEqualFigures();
  ftqOneLowCountMovesTheMinimum();
  ftqNoiseFractionOfALowQuantum();
  ftqZeroMaximumAnswersOneHundred();
  ftqEdgesStandFromTheFirstStamp();
  ftqTrailingOpenQuantumRecordsNothing();
  ftqBufferFullCountsDrops();
  ftqQuantumOfTenNanosecondsTarget();
  ftqDipEveryMillisecondNamesThePeriod();

  std::puts("harness_fwq_ftq_test: ok");
  return 0;
}
