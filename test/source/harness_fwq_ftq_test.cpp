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
// Every row reads the declared surface alone: no probe runs, no time
// source is read, and no cpu is named. Frameworkless check()/fail()
// convention, the way harness_jitter_probe_test carries it.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <type_traits>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

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

  std::puts("harness_fwq_ftq_test: ok");
  return 0;
}
