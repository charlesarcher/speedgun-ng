// ============================================================================
// TDD test for the shared growth rule (IF-09; FR-008, FR-010, FR-016).
//
// The warm-up phase and the calibration phase of the runner share one
// stop rule, so the rule is tested on its own over scripted sample
// callables: the bound, the cap, the fivefold real-time condition, and
// the interrupt read after a run. The header is private to the harness
// target, so this test reaches it through the harness source directory.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <limits>

#include "detail/calibration.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS GROWTH TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// One sample callable per case: it records the count it was handed and
// returns a run record with the times the case names.
struct Sample
{
  std::int64_t realNs = 0;
  std::int64_t decisionNs = 0;
  std::uint64_t samples = 0;
  std::uint64_t lastCount = 0;

  auto operator()(const std::uint64_t count)
      -> std::expected<sg::detail::RunRecord, std::string>
  {
    ++samples;
    lastCount = count;
    return sg::detail::RunRecord {count, 0, realNs, decisionNs, false, {}};
  }
};

auto neverQualifiesWithinTheBound() -> void
{
  auto sample = Sample {10, 10, 0, 0};
  const auto result =
      sg::detail::growUntilQualified(1,
                                     1000,
                                     3,
                                     sg::detail::PhaseText {"probe", "FR-016"},
                                     sample,
                                     [] { return false; });
  check(result.outcome == sg::detail::GrowOutcome::BOUND_EXHAUSTED,
        "three runs without a qualifying run exhaust the bound (FR-016)");
  check(result.error == "probe did not qualify within 3 runs (FR-016)",
        "the exhausted bound carries the phase text (FR-011, FR-016)");
  check(sample.samples == 3, "the bound counts exactly three samples");
  check(result.record.iterations == 0,
        "an exhausted bound carries no qualifying record");
}

auto growthReachesTheCap() -> void
{
  auto sample = Sample {0, 0, 0, 0};
  const auto result =
      sg::detail::growUntilQualified(1,
                                     1000,
                                     sg::detail::kRunBound,
                                     sg::detail::PhaseText {"probe", "FR-016"},
                                     sample,
                                     [] { return false; });
  check(result.outcome == sg::detail::GrowOutcome::QUALIFIED,
        "the cap qualifies the run that reaches it (FR-016)");
  // The FR-010 rule multiplies by ten while the decision time sits at or
  // below one tenth of the target: 1, 10, 100, and on to the cap.
  check(sample.samples == 13, "the growth reaches the cap on run 13 (FR-010)");
  check(result.record.iterations == sg::detail::kIterationCap,
        "the qualifying count is the FR-016 cap");
}

auto fivefoldRealTimeQualifiesAtOnce() -> void
{
  auto sample = Sample {5000, 10, 0, 0};
  const auto result =
      sg::detail::growUntilQualified(1,
                                     1000,
                                     sg::detail::kRunBound,
                                     sg::detail::PhaseText {"probe", "FR-008"},
                                     sample,
                                     [] { return false; });
  check(result.outcome == sg::detail::GrowOutcome::QUALIFIED,
        "real time at five times the target qualifies (FR-008)");
  check(sample.samples == 1, "the first run qualified");
}

auto interruptAfterARunStopsTheGrowth() -> void
{
  auto sample = Sample {5000, 10, 0, 0};
  const auto result =
      sg::detail::growUntilQualified(1,
                                     1000,
                                     3,
                                     sg::detail::PhaseText {"probe", "FR-032"},
                                     sample,
                                     [] { return true; });
  check(result.outcome == sg::detail::GrowOutcome::INTERRUPTED,
        "the flag read after a run stops the growth (FR-032, R-06)");
  check(sample.samples == 1, "no further run starts after the interrupt");
}

}  // namespace

// FR-016: growth that would pass the cap stops at the cap.
auto growthClampsAtTheCap() -> void
{
  const auto grown =
      sg::detail::nextIterationCount(200'000'000'000ULL, 1, 1000);
  check(grown == sg::detail::kIterationCap,
        "growth that would pass the cap stops at the cap (FR-016)");
}

// FR-008: a target with no representable fivefold keeps the largest count
// the threshold can hold.
auto theFivefoldStaysInRange() -> void
{
  constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();
  check(sg::detail::fivefoldNs(kMax / 5 + 1) == kMax,
        "a target above one fifth of the largest count takes that count "
        "(FR-008)");
  check(sg::detail::fivefoldNs(1000) == 5000,
        "an ordinary target takes its fivefold (FR-008)");
}

// FR-016: a run slower than the target grows by the ordinary factor, so
// the count moves by one.
auto aSlowRunGrowsByOne() -> void
{
  const auto grown = sg::detail::nextIterationCount(100, 1'000'000, 1000);
  check(grown == 101, "a run slower than the target grows by one (FR-016)");
}

auto main() -> int
{
  neverQualifiesWithinTheBound();
  growthReachesTheCap();
  fivefoldRealTimeQualifiesAtOnce();
  interruptAfterARunStopsTheGrowth();
  growthClampsAtTheCap();
  theFivefoldStaysInRange();
  aSlowRunGrowsByOne();
  std::puts("harness_growth_test: ok");
  return 0;
}
