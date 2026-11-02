#ifndef SG_HARNESS_DETAIL_SPECTRUM_HPP
#define SG_HARNESS_DETAIL_SPECTRUM_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/**
 * @file spectrum.hpp
 * @brief The spectrum kernel behind the FWQ and FTQ period list: the
 * plain DFT over a sample span, and the resample that turns the FWQ unit
 * times into that span. The kernel is a pure function: it reads no probe,
 * no seam, no provider and no clock, and it runs after the window, outside
 * every measured loop (FR-017, FR-018, D-5, R-12, R-13). Nothing here is
 * public API.
 */

namespace sg::detail
{

// FR-018, D-5: the most points one transform call reads. R-6 measures one
// call at the cap at 3.3 s at -O2 and 14.6 s under the coverage build on
// the lane host, inside the standing 300 s an entry with no own timeout
// carries.
inline constexpr std::uint64_t kSpectrumCap = 65536;

// FR-018: the most periods one answer carries.
inline constexpr std::size_t kSpectrumTop = 5;

// FR-018, R-7: a bin stands kept only above three times the median of bins
// 1 through N/2, the DC bin out.
inline constexpr double kSpectrumMedianFactor = 3.0;

/// @brief One kept period: the period in nanoseconds, and the bin power
/// relative to the strongest bin, so the head of the list stands at 1.0
/// (FR-019, E-24).
struct SpectrumPeriod
{
  std::uint64_t periodNs = 0;
  double powerRatio = 0.0;
};

/// @brief The transform's answer: the points it read, the points it skipped,
/// the step it read, and the period list (FR-018, FR-019, FR-025).
struct SpectrumResult
{
  std::uint64_t points = 0;
  std::uint64_t skippedPoints = 0;
  std::uint64_t stepNs = 0;
  std::vector<SpectrumPeriod> periods;
};

/// @brief FR-017, R-12: the kept FWQ unit times as the transform's input.
///
/// The step stands at the median unit time of the kept series. Bin k covers
/// one step of cumulative unit time and holds the mean unit time inside it,
/// and an empty bin takes the previous sample. A unit time that ends at a
/// bin boundary closes that bin, and one that ends inside leaves it open:
/// the open bin takes the mean of the unit times that stand inside it.
///
/// \pre `unitTimes` holds the kept FWQ unit times in record order
/// \pre `stepNs` stands at or above 1
/// \pre `cap` stands at or below `kSpectrumCap`
/// \post one sample per whole step of cumulative unit time, at most `cap`
///       samples, an empty bin taking the previous sample (FR-017)
[[nodiscard]] auto resampleUnitTimes(std::span<const std::uint64_t> unitTimes,
                                     std::uint64_t stepNs,
                                     std::uint64_t cap) -> std::vector<double>;

/// @brief FR-018, R-13: the plain DFT over a sample span, strongest first.
///
/// The kernel reads the last `min(samples.size(), kSpectrumCap)` points and
/// names the skipped count. Bin k of N points stands at the angle
/// `2 * pi * k / N`, and its sine and cosine stand built by the angle-sum
/// recurrence, so no library call stands in the inner loop. The cost stands
/// at `N x N/2` complex multiply-adds, four real each, and the call
/// allocates one span of N bin powers before its loop and nothing inside
/// it. A bin stands kept when its power stands above `kSpectrumMedianFactor`
/// times the median of bins 1 through N/2, the DC bin out. The list sorts by
/// power, strongest first, ties in period order, and cuts at
/// `kSpectrumTop`. The period of bin k stands at `(N x stepNs) / k`
/// (FR-019).
///
/// \pre `samples` holds at least one sample
/// \pre `stepNs` stands at or above 1
/// \post the answer reads the last `min(samples.size(), kSpectrumCap)`
///       points, names the skipped count, and carries at most
///       `kSpectrumTop` periods ordered by power, strongest first, ties in
///       period order (FR-018)
[[nodiscard]] auto dftPeriods(std::span<const double> samples,
                              std::uint64_t stepNs) -> SpectrumResult;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_SPECTRUM_HPP
