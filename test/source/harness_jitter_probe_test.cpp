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
#include <type_traits>
#include <vector>

#include "detail/jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"

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

using HandleProbeFn = sg::BenchmarkHandle& (sg::BenchmarkHandle::*)(
    const sg::JitterProbeOptions&);
static_assert(
    std::is_same_v<decltype(&sg::BenchmarkHandle::jitterProbe), HandleProbeFn>,
    "the handle call returns this handle (FR-015)");

// R-02, FR-004: the reader-install seam takes one counters reader, and
// no second time source arrives through it.
static_assert(
    std::is_same_v<decltype(&sg::detail::installTestReader),
                   void (*)(std::unique_ptr<sg::counters::WindowReader>)>,
    "the seam installs one counters WindowReader (R-02, FR-004)");

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

  std::puts("harness_jitter_probe_test: ok");
  return 0;
}
