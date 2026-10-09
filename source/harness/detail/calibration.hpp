#ifndef SG_HARNESS_DETAIL_CALIBRATION_HPP
#define SG_HARNESS_DETAIL_CALIBRATION_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include "speedgun-ng/benchmark.hpp"

namespace sg::detail
{

// FR-016 bounds the iteration count; FR-010 bounds the runs of one phase.
inline constexpr std::uint64_t kIterationCap = 1'000'000'000'000ULL;
inline constexpr std::uint64_t kRunBound = 96;

// One sampled run: the two endpoint actions, the fold of each clock,
// and what the benchmark function asked for.
struct RunRecord
{
  std::uint64_t iterations = 0;
  std::uint64_t pair = 0;
  std::int64_t realNs = 0;
  std::int64_t decisionNs = 0;
  bool skipped = false;
  std::string reason;
};

// R-012: the growth factor is integer arithmetic on nanoseconds, and a
// run far below the target jumps by ten.
inline auto nextIterationCount(const std::uint64_t current,
                               const std::int64_t decisionNs,
                               const std::int64_t targetNs) -> std::uint64_t
{
  double factor = 1.4 * static_cast<double>(targetNs)
      / static_cast<double>(decisionNs > 0 ? decisionNs : 1);
  if (decisionNs <= targetNs / 10) {
    factor = 10.0;
  }

  const double grown = static_cast<double>(current) * factor;
  std::uint64_t next = current + 1;
  if (grown >= static_cast<double>(kIterationCap)) {
    next = kIterationCap;
  } else if (grown > static_cast<double>(next)) {
    next = static_cast<std::uint64_t>(std::llround(grown));
  }
  return std::min(next, kIterationCap);
}

// The fivefold threshold of FR-008 stays in range: a target above one
// fifth of the largest nanosecond count has no representable fivefold,
// so the threshold is then that largest count.
inline auto fivefoldNs(const std::int64_t targetNs) -> std::int64_t
{
  constexpr std::int64_t kMaxNs = std::numeric_limits<std::int64_t>::max();
  return targetNs > kMaxNs / 5 ? kMaxNs : 5 * targetNs;
}

// FR-008: the run that meets any one of these conditions is the run the
// phase stops at.
inline auto qualifies(const RunRecord& record,
                      const std::int64_t targetNs) -> bool
{
  return record.iterations >= kIterationCap || record.decisionNs >= targetNs
      || record.realNs >= fivefoldNs(targetNs);
}

enum class GrowOutcome : std::uint8_t
{
  QUALIFIED,
  SKIPPED,
  FAILED,
  INTERRUPTED,
  BOUND_EXHAUSTED
};

struct GrowResult
{
  GrowOutcome outcome = GrowOutcome::BOUND_EXHAUSTED;
  RunRecord record;
  std::string error;
};

// Which phase is growing, and the requirement its failure text cites.
struct PhaseText
{
  std::string_view name;
  std::string_view citation;
};

// \pre none
// \post The text names the phase, the bound it spent, and the citation,
//       so the caller forwards it without holding wording of its own.
auto boundText(const PhaseText& phase, std::uint64_t runBound) -> std::string;

// One phase of runs, the warm-up phase and the calibration phase alike:
// sample at the count, read what the function asked for, read the
// interrupt flag, then stop or grow (FR-008, FR-010, FR-016, FR-032).
// \pre runBound is at least one.
// \post An exhausted bound returns the phase text of boundText in error.
template<class Sample, class Interrupt>
auto growUntilQualified(const std::uint64_t start,
                        const std::int64_t targetNs,
                        const std::uint64_t runBound,
                        const PhaseText& phase,
                        Sample&& sample,
                        Interrupt&& interrupted) -> GrowResult
{
  std::uint64_t count = start;
  for (std::uint64_t run = 0; run < runBound; ++run) {
    auto record = sample(count);
    if (!record.has_value()) {
      return {GrowOutcome::FAILED, {}, record.error()};
    }
    if (record->skipped) {
      return {GrowOutcome::SKIPPED, std::move(*record), {}};
    }
    if (interrupted()) {
      return {GrowOutcome::INTERRUPTED, std::move(*record), {}};
    }
    if (qualifies(*record, targetNs)) {
      return {GrowOutcome::QUALIFIED, std::move(*record), {}};
    }
    count = nextIterationCount(count, record->decisionNs, targetNs);
  }
  return {GrowOutcome::BOUND_EXHAUSTED, {}, boundText(phase, runBound)};
}

inline auto boundText(const PhaseText& phase,
                      const std::uint64_t runBound) -> std::string
{
  return std::string(phase.name) + " did not qualify within "
      + std::to_string(runBound) + " runs (" + std::string(phase.citation)
      + ")";
}

}  // namespace sg::detail

#endif
