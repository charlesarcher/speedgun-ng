// ============================================================================
// T010: the spectrum kernel of specs/043-fwq-ftq (US3, module M-E): the
// plain DFT over a sample span, and the resample that turns the FWQ unit
// times into that span (FR-017, FR-018, D-5, R-12, R-13). The kernel is a
// pure function over its argument: it reads no probe, no seam, no provider
// and no clock, and it runs after the window, outside every measured loop.
//
// The cost is the shape R-6 measures: `N x N/2` complex multiply-adds, four
// real each, and one span of N bin powers allocated before the loop with
// nothing allocated inside it, 524,288 bytes at the cap. Each bin builds its
// sine and cosine by the angle-sum recurrence, so no library call stands in
// the inner loop: the same call with a libm `cos` and `sin` per sample
// measures 47.8 s at the cap under the coverage build, against the 300 s the
// entry carries.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <span>
#include <vector>

#include "detail/spectrum.hpp"

#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

// The full turn of the plane: bin k of N points stands at the angle
// `2 * pi * k / N` (FR-018, R-13).
constexpr double kTwoPi = 6.28318530717958647692528676655900577;

// One kept bin on its way into the list: the period it names and the power
// it carries, before the ratio against the strongest bin lands.
struct KeptBin
{
  std::uint64_t periodNs = 0;
  double power = 0.0;
};

}  // namespace

auto resampleUnitTimes(std::span<const std::uint64_t> unitTimes,
                       std::uint64_t stepNs,
                       std::uint64_t cap) -> std::vector<double>
{
  SG_REQUIRE(stepNs >= 1, "the resample step stands at or above 1 ns (FR-017)");
  SG_REQUIRE(cap <= kSpectrumCap,
             "the resample count stands at or below the transform cap "
             "(FR-017, FR-018)");

  std::uint64_t total = 0;
  for (const std::uint64_t unit : unitTimes) {
    total += unit;
  }
  const auto whole = std::min<std::uint64_t>(total / stepNs, cap);
  std::vector<double> samples(static_cast<std::size_t>(whole), 0.0);

  std::uint64_t elapsed = 0;
  std::size_t bin = 0;
  double sum = 0.0;
  std::uint64_t count = 0;
  double previous = 0.0;
  for (const std::uint64_t unit : unitTimes) {
    sum += static_cast<double>(unit);
    ++count;
    elapsed += unit;
    // The bin closes when a unit time ends at its boundary. One that ends
    // inside leaves the bin open, and one that ends short of the step leaves
    // the bin standing open ahead of the series (FR-017).
    if (elapsed < stepNs || elapsed % stepNs != 0) {
      continue;
    }
    const double mean = sum / static_cast<double>(count);
    const auto closed = static_cast<std::size_t>(elapsed / stepNs - 1);
    // The bins between the last closed bin and this one hold no unit time,
    // so each takes the previous sample (FR-017).
    while (bin < closed && bin < samples.size()) {
      samples[bin] = previous;
      ++bin;
    }
    if (bin < samples.size()) {
      samples[bin] = mean;
      ++bin;
    }
    previous = mean;
    sum = 0.0;
    count = 0;
  }
  // The trailing open bin takes the mean of the unit times that stand
  // inside it, and the bins past it take that sample in turn (FR-017).
  const double open = count == 0 ? previous : sum / static_cast<double>(count);
  while (bin < samples.size()) {
    samples[bin] = open;
    ++bin;
  }
  SG_ENSURE(samples.size() == whole,
            "the answer carries one sample per whole step of cumulative unit "
            "time (FR-017)");
  return samples;
}

auto dftPeriods(std::span<const double> samples, std::uint64_t stepNs)
    -> SpectrumResult
{
  SG_REQUIRE(!samples.empty(),
             "the transform reads a span with at least one sample (FR-018)");
  SG_REQUIRE(stepNs >= 1,
             "the transform step stands at or above 1 ns (FR-018)");

  SpectrumResult result;
  result.stepNs = stepNs;
  const auto total = static_cast<std::uint64_t>(samples.size());
  // The read starts at the far end of the span, so a long series reads as
  // its last `kSpectrumCap` points and the skipped count rides the record
  // (FR-018, FR-025).
  const auto points = std::min<std::uint64_t>(total, kSpectrumCap);
  result.points = points;
  result.skippedPoints = total - points;

  const auto bins = points / 2;
  if (bins < 1) {
    return result;
  }
  const auto window = samples.subspan(samples.size() - points, points);

  // The window mean stands removed ahead of the bin loop. The mean feeds
  // only the DC bin, which the filter excludes, so bins 1 through N/2 keep
  // the value each carries, and a level series leaves them equal with the
  // recurrence's rounding floor at zero, the case the strict compare reads
  // (FR-018, SC-002, R-7). The pass allocates nothing.
  double windowSum = 0.0;
  for (const double sample : window) {
    windowSum += sample;
  }
  const double mean = windowSum / static_cast<double>(points);

  // One span of N bin powers, allocated ahead of the loop (FR-017).
  std::vector<double> power(static_cast<std::size_t>(points), 0.0);
  const double turn = kTwoPi / static_cast<double>(points);
  for (std::uint64_t bin = 1; bin <= bins; ++bin) {
    const double angle = turn * static_cast<double>(bin);
    const double stepCos = std::cos(angle);
    const double stepSin = std::sin(angle);
    double cosRun = 1.0;
    double sinRun = 0.0;
    double real = 0.0;
    double imag = 0.0;
    // The angle-sum recurrence carries the bin's cosine and sine: the pair
    // advances by the bin's own angle once per sample, and no library call
    // stands in the loop (FR-018, R-13).
    for (const double sample : window) {
      const double centred = sample - mean;
      real += centred * cosRun;
      imag -= centred * sinRun;
      const double nextCos = cosRun * stepCos - sinRun * stepSin;
      sinRun = sinRun * stepCos + cosRun * stepSin;
      cosRun = nextCos;
    }
    power[bin] = real * real + imag * imag;
  }

  // The median of bins 1 through N/2, the DC bin out, the nearest-rank 50.0
  // point the series fold already answers (FR-018, R-7).
  const auto last = static_cast<std::ptrdiff_t>(bins);
  std::vector<double> sorted(power.begin() + 1, power.begin() + last + 1);
  std::ranges::sort(sorted, std::less<> {});
  const double threshold =
      kSpectrumMedianFactor * sorted[(sorted.size() - 1) / 2];

  std::vector<KeptBin> kept;
  for (std::uint64_t bin = 1; bin <= bins; ++bin) {
    const double binPower = power[bin];
    if (binPower > threshold) {
      kept.push_back(
          KeptBin {.periodNs = (points * stepNs) / bin, .power = binPower});
    }
  }
  // Two stable passes order the list: the period pass first, then the power
  // pass, so a tie in power keeps the period order the first pass set
  // (FR-018).
  std::ranges::stable_sort(
      kept, std::less<> {}, [](const KeptBin& item) { return item.periodNs; });
  std::ranges::stable_sort(
      kept, std::greater<> {}, [](const KeptBin& item) { return item.power; });
  if (!kept.empty()) {
    const double strongest = kept.front().power;
    const auto take = std::min(kept.size(), kSpectrumTop);
    result.periods.reserve(take);
    for (std::size_t rank = 0; rank < take; ++rank) {
      result.periods.push_back(SpectrumPeriod {
          .periodNs = kept[rank].periodNs,
          .powerRatio = kept[rank].power / strongest,
      });
    }
  }
  SG_ENSURE(result.periods.size() <= kSpectrumTop,
            "the list carries at most the top-five entries (FR-018)");
  return result;
}

}  // namespace sg::detail
