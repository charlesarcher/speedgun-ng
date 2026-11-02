// ============================================================================
// T060, T008, T009: the spectrum kernel of specs/043-fwq-ftq, driven as the
// pure function over a sample span the plan states (US3, module M-E). The
// transform names the strongest periods of a scripted span, keeps a bin
// whose power stands above three times the median of bins 1 through N/2,
// reads at most 65,536 points and names the skipped count (FR-018, SC-002,
// D-5, R-13). The resample turns a scripted FWQ unit-time series into the
// transform input at the median unit time (FR-017, R-12). The kernel reads
// no probe, no seam, no provider and no clock, so every row scripts a span
// and reads the period list back (FR-025). The tone cases stand at or below
// 4,096 points and the suite carries one cap case, the budget R-6 measures
// at the cap. Frameworkless check()/fail() convention, the way
// harness_json_schema_test.cpp carries it.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ranges>
#include <vector>

#include "detail/json_fixture.hpp"
#include "detail/spectrum.hpp"

using sg::test::check;
using sg::test::sameValue;

namespace
{

// The step the transform reads, the median unit time an FWQ run at a 1 us
// unit time answers (FR-017).
constexpr std::uint64_t kStepNs = 1000;

constexpr double kTwoPi = 6.28318530717958647692528676655900577;

// One planted bin of a span: the cosine of bin `bin` of `points` samples,
// the period `points * kStepNs / bin` ns. A period that stands at an exact
// bin of the span reads with no leakage, so the row names the period the
// span carries (FR-018, R-7).
struct Plant
{
  std::uint64_t bin = 0;
  double amplitude = 0.0;
};

auto planted(const std::size_t points,
             const std::vector<Plant>& plants) -> std::vector<double>
{
  std::vector<double> samples(points, 0.0);
  for (const Plant& plant : plants) {
    for (std::size_t n = 0; n < points; ++n) {
      samples[n] += plant.amplitude
          * std::cos(kTwoPi * static_cast<double>(plant.bin)
                     * static_cast<double>(n) / static_cast<double>(points));
    }
  }
  return samples;
}

// The period of one bin of a span, the reading the kernel states (FR-019).
constexpr auto binPeriodNs(const std::uint64_t points,
                           const std::uint64_t bin) -> std::uint64_t
{
  return (points * kStepNs) / bin;
}

// FR-018: a pure tone names its own period, strongest, at the head.
auto aPureToneNamesItsPeriod() -> void
{
  constexpr std::uint64_t kPoints = 1024;
  constexpr std::uint64_t kBin = 64;
  const auto samples = planted(kPoints, {Plant {kBin, 1.0}});
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == kPoints, "the transform reads every point (FR-018)");
  check(answer.skippedPoints == 0,
        "a span at the cap's floor skips no point (FR-018)");
  check(answer.stepNs == kStepNs,
        "the answer carries the step it read (FR-017)");
  check(answer.periods.size() <= sg::detail::kSpectrumTop,
        "the list stands at or below the five-entry cut (FR-018)");
  if (!answer.periods.empty()) {
    const auto& head = answer.periods.front();
    check(head.periodNs == binPeriodNs(kPoints, kBin),
          "the head names the planted 16 us period (FR-018, FR-019)");
    check(sameValue(head.powerRatio, 1.0),
          "the head carries the strongest power (FR-019)");
  }
}

// FR-018: two tones rank strongest first, each with its power relative to
// the strongest bin.
auto twoTonesRankStrongestFirst() -> void
{
  constexpr std::uint64_t kPoints = 1024;
  const auto samples = planted(kPoints, {Plant {32, 2.0}, Plant {64, 1.0}});
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.periods.size() >= 2, "the two tones stand kept (FR-018)");
  if (answer.periods.size() >= 2) {
    check(answer.periods[0].periodNs == binPeriodNs(kPoints, 32),
          "the louder 32 us tone stands first (FR-018)");
    check(answer.periods[1].periodNs == binPeriodNs(kPoints, 64),
          "the quieter 16 us tone stands second (FR-018)");
    check(sameValue(answer.periods[0].powerRatio, 1.0),
          "the head carries the strongest power (FR-019)");
    check(sameValue(answer.periods[1].powerRatio, 0.25),
          "the quieter tone carries a quarter of the power (FR-019)");
  }
}

// FR-018, SC-002: a constant series leaves every bin of 1 through N/2 at
// zero power, so the strict comparison against three times the median
// keeps nothing and the list stands empty.
auto aConstantSeriesAnswersAnEmptyList() -> void
{
  const std::vector<double> samples(1024, 0.0);
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == 1024, "the transform reads the whole span (FR-018)");
  check(answer.periods.empty(),
        "a constant series names no period (FR-018, SC-002)");
}

// FR-018, SC-002, R-7: a constant at 1000.0 stands level across the span,
// so the window mean stands removed ahead of the bin loop, bins 1 through
// N/2 read equal power, the floor the recurrence leaves reads zero, and the
// strict compare against three times the median keeps nothing.
auto aNonzeroConstantAnswersAnEmptyList() -> void
{
  const std::vector<double> samples(1024, 1000.0);
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == 1024, "the transform reads the whole span (FR-018)");
  check(answer.periods.empty(),
        "a constant series at 1000.0 names no period (FR-018, SC-002)");
}

// FR-018: a one-sample span leaves no bin, so the list stands empty.
auto oneSampleAnswersAnEmptyList() -> void
{
  const std::vector<double> samples {1000.0};
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == 1, "the transform reads the one point (FR-018)");
  check(answer.skippedPoints == 0, "a one-point span skips no point (FR-018)");
  check(answer.periods.empty(), "a one-sample span names no period (FR-018)");
}

// FR-018, FR-025: a span at or below the cap reads every point.
auto theCapReadsEveryPoint() -> void
{
  constexpr std::uint64_t kPoints = 4096;
  constexpr std::uint64_t kBin = 256;
  const auto samples = planted(kPoints, {Plant {kBin, 1.0}});
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == kPoints,
        "a span below the cap of 65,536 reads every point (FR-018, FR-025)");
  check(answer.skippedPoints == 0,
        "a span below the cap skips no point (FR-018, FR-025)");
  check(!answer.periods.empty(), "the tone stands kept (FR-018)");
  if (!answer.periods.empty()) {
    check(answer.periods.front().periodNs == binPeriodNs(kPoints, kBin),
          "the head names the planted 16 us period (FR-018)");
  }
}

// FR-018, FR-025: a 70,000-point span reads as its last 65,536 points and
// discloses the 4,464 points the read skipped.
auto aLongerSeriesDisclosesTheSkippedCount() -> void
{
  constexpr std::size_t kSpan = 70000;
  constexpr std::uint64_t kPoints = 65536;
  constexpr std::uint64_t kBin = 4096;
  std::vector<double> samples;
  samples.reserve(kSpan);
  for (std::size_t n = 0; n < kSpan; ++n) {
    samples.push_back(std::cos(kTwoPi * static_cast<double>(kBin * kStepNs)
                               * static_cast<double>(n)
                               / static_cast<double>(kPoints * kStepNs)));
  }
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == kPoints,
        "the transform reads the cap of 65,536 points (FR-018, FR-025)");
  check(answer.skippedPoints == kSpan - kPoints,
        "the answer names the 4,464 skipped points (FR-018, FR-025)");
  check(!answer.periods.empty(), "the tone of the last window stands kept");
  if (!answer.periods.empty()) {
    check(answer.periods.front().periodNs == binPeriodNs(kPoints, kBin),
          "the head names the 16 us period of the last window (FR-018)");
  }
}

// FR-018, FR-025: a six-tone span keeps five entries, strongest first, and
// the sixth period stands out of the list.
auto sixTonesKeepFiveEntries() -> void
{
  constexpr std::uint64_t kPoints = 4096;
  const auto samples = planted(kPoints,
                               {Plant {8, 6.0},
                                Plant {16, 5.0},
                                Plant {32, 4.0},
                                Plant {64, 3.0},
                                Plant {128, 2.0},
                                Plant {256, 1.0}});
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.periods.size() == sg::detail::kSpectrumTop,
        "the list cuts at five entries (FR-018, FR-025)");
  const std::uint64_t expected[5] = {binPeriodNs(kPoints, 8),
                                     binPeriodNs(kPoints, 16),
                                     binPeriodNs(kPoints, 32),
                                     binPeriodNs(kPoints, 64),
                                     binPeriodNs(kPoints, 128)};
  for (std::size_t rank = 0; rank < answer.periods.size() && rank < 5; ++rank) {
    check(answer.periods[rank].periodNs == expected[rank],
          "the ranks read the five strongest periods in order (FR-018)");
  }
  for (const auto& period : answer.periods) {
    check(period.periodNs != binPeriodNs(kPoints, 256),
          "the sixth tone stands out of the list (FR-018, FR-025)");
  }
  for (std::size_t rank = 1; rank < answer.periods.size(); ++rank) {
    const double standing = answer.periods[rank].powerRatio;
    const double ahead = answer.periods[rank - 1].powerRatio;
    check(standing <= ahead, "the list ranks strongest first (FR-018)");
  }
}

// The span that pins the median's domain: bins 1 through 64 carry one
// cosine, bins 65 through 127 carry two, the DC bin carries the offset,
// and bin 8 carries a quadrature tone at ten times the low level. The
// median over bins 1 through N/2 reads the low level, so the tone stands
// kept; the median over bins 0 through N/2 reads the high level, and the
// tone stands dropped (FR-018, R-7).
auto medianDomainSpan() -> std::vector<double>
{
  constexpr std::size_t kPoints = 256;
  std::vector<double> samples(kPoints, 1.0);
  for (std::size_t n = 0; n < kPoints; ++n) {
    const double at = static_cast<double>(n);
    for (std::uint64_t bin = 1; bin <= kPoints / 4; ++bin) {
      samples[n] += std::cos(kTwoPi * static_cast<double>(bin) * at
                             / static_cast<double>(kPoints));
    }
    for (std::uint64_t bin = kPoints / 4 + 1; bin < kPoints / 2; ++bin) {
      samples[n] += 2.0
          * std::cos(kTwoPi * static_cast<double>(bin) * at
                     / static_cast<double>(kPoints));
    }
    samples[n] +=
        3.0 * std::sin(kTwoPi * 8.0 * at / static_cast<double>(kPoints));
  }
  return samples;
}

// FR-018: the median stands over bins 1 through N/2, the DC bin out.
auto theMedianStandsOverBinsOneToHalf() -> void
{
  constexpr std::uint64_t kPoints = 256;
  const auto samples = medianDomainSpan();
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(!answer.periods.empty(),
        "the median of bins 1 through N/2 keeps the planted tone (FR-018)");
  if (!answer.periods.empty()) {
    check(answer.periods.front().periodNs == binPeriodNs(kPoints, 8),
          "the tone at ten times the median level stands first (FR-018)");
  }
  for (const auto& period : answer.periods) {
    check(period.periodNs != binPeriodNs(kPoints, 1),
          "the DC bin stands out of the list (FR-018, R-7)");
  }
}

// FR-018: a bin whose power stands below three times the median of bins 1
// through N/2 stands dropped. The flat span leaves every bin of 1 through
// N/2 - 1 at one level, so the threshold reads three times that level and
// the tone at twice the level stands dropped.
auto aBinBelowTheThresholdStandsDropped() -> void
{
  constexpr std::size_t kPoints = 256;
  std::vector<double> samples(kPoints, 0.0);
  for (std::size_t n = 0; n < kPoints; ++n) {
    const double at = static_cast<double>(n);
    for (std::uint64_t bin = 1; bin < kPoints / 2; ++bin) {
      samples[n] += std::cos(kTwoPi * static_cast<double>(bin) * at
                             / static_cast<double>(kPoints));
    }
    samples[n] += std::sin(kTwoPi * 8.0 * at / static_cast<double>(kPoints));
  }
  const auto answer = sg::detail::dftPeriods(samples, kStepNs);

  check(answer.points == kPoints, "the transform reads the flat span (FR-018)");
  check(answer.periods.empty(),
        "a bin at twice the median level stands dropped (FR-018)");
}

// ---------------------------------------------------------------------------
// The resample path of FR-017 and R-12: the FWQ unit times at the median
// unit time, one sample per whole step of cumulative unit time, each bin
// holding the mean unit time inside it and an empty bin taking the previous
// sample.
// ---------------------------------------------------------------------------

// The nearest-rank 50.0 point of a scripted series, the figure an FWQ run
// takes as the resample step (FR-017, R-5).
auto medianOf(const std::vector<std::uint64_t>& unitTimes) -> std::uint64_t
{
  std::vector<std::uint64_t> sorted = unitTimes;
  std::ranges::sort(sorted);
  return sorted[(sorted.size() - 1) / 2];
}

// FR-017: each bin of one step of cumulative unit time holds the mean unit
// time inside it. A unit time that ends at the bin boundary closes the bin,
// and one that ends inside leaves the bin open.
auto theResampleAveragesEachBin() -> void
{
  const std::vector<std::uint64_t> unitTimes {400, 600, 800, 200, 1000};
  const auto samples = sg::detail::resampleUnitTimes(
      unitTimes, kStepNs, sg::detail::kSpectrumCap);

  check(samples.size() == 3,
        "the resample answers one sample per whole step (FR-017)");
  if (samples.size() == 3) {
    check(sameValue(samples[0], 500.0),
          "the first bin holds the mean of its two unit times (FR-017)");
    check(sameValue(samples[1], 500.0),
          "the second bin holds the mean of its two unit times (FR-017)");
    check(sameValue(samples[2], 1000.0),
          "a unit time that ends at the boundary closes its own bin (FR-017)");
  }
}

// FR-017: a unit time that spans a bin leaves that bin empty, and the empty
// bin takes the previous sample.
auto anEmptyBinTakesThePreviousSample() -> void
{
  const std::vector<std::uint64_t> unitTimes {1000, 2500, 500};
  const auto samples = sg::detail::resampleUnitTimes(
      unitTimes, kStepNs, sg::detail::kSpectrumCap);

  check(samples.size() == 4,
        "the resample answers one sample per whole step (FR-017)");
  if (samples.size() == 4) {
    check(sameValue(samples[0], 1000.0),
          "a filled bin holds its own mean (FR-017)");
    check(sameValue(samples[1], 1000.0),
          "the bin the long unit time leaves empty takes the previous sample "
          "(FR-017)");
    check(sameValue(samples[2], 1000.0),
          "the next empty bin takes the same sample (FR-017)");
    check(
        sameValue(samples[3], 1500.0),
        "the bin the long unit time closes holds the mean of the two (FR-017)");
  }
}

// FR-017: the step stands at the median unit time of the kept series, and
// the cap bounds the count the resample answers.
auto theResampleStepStandsAtTheMedian() -> void
{
  const std::vector<std::uint64_t> unitTimes {1000, 1000, 1000, 1000, 5000};
  const std::uint64_t step = medianOf(unitTimes);
  check(step == 1000, "the median unit time of the series reads 1 us (FR-017)");

  const auto samples =
      sg::detail::resampleUnitTimes(unitTimes, step, sg::detail::kSpectrumCap);
  check(samples.size() == 9,
        "the median step counts nine whole steps of cumulative unit time "
        "(FR-017)");
  if (samples.size() == 9) {
    check(sameValue(samples.front(), 1000.0),
          "the head bin holds its own unit time (FR-017)");
    check(sameValue(samples.back(), 5000.0),
          "the bin the long unit time closes holds it (FR-017)");
  }

  const auto capped = sg::detail::resampleUnitTimes(unitTimes, step, 4);
  check(capped.size() == 4, "the cap bounds the count (FR-017)");
}

}  // namespace

auto main() -> int
{
  aPureToneNamesItsPeriod();
  twoTonesRankStrongestFirst();
  aConstantSeriesAnswersAnEmptyList();
  aNonzeroConstantAnswersAnEmptyList();
  oneSampleAnswersAnEmptyList();
  theCapReadsEveryPoint();
  aLongerSeriesDisclosesTheSkippedCount();
  sixTonesKeepFiveEntries();
  theMedianStandsOverBinsOneToHalf();
  aBinBelowTheThresholdStandsDropped();
  theResampleAveragesEachBin();
  anEmptyBinTakesThePreviousSample();
  theResampleStepStandsAtTheMedian();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
