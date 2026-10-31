#ifndef SG_HARNESS_DETAIL_CALIBRATION_HPP
#define SG_HARNESS_DETAIL_CALIBRATION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "internal.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace sg::detail
{

// FR-016 bounds the iteration count; FR-010 bounds the runs of one phase.
inline constexpr std::uint64_t kIterationCap = 1'000'000'000'000ULL;
inline constexpr std::uint64_t kRunBound = 96;

// FR-025, Q-10, T004: the ceiling of one reservation's pause room, raw points
// per leaf column. The recorder reserves pause room only where the family's
// previous attempt observed closed pairs, and an attempt over the ceiling
// folds instead (FR-051, FR-052).
inline constexpr std::size_t kMaxPauseSamplesPerLeaf = 32'768;

// specs/021 FR-006: the ceiling on the chunk points one leaf column of one
// attempt holds. The endpoint point and the pause room stand beside them,
// so the ceiling bounds the chunk count and the chunk size rises to meet
// it.
inline constexpr std::size_t kMaxChunkPointsPerLeaf = 32'768;

// specs/021 E-02: the chunk plan of one attempt: the iterations one chunk
// holds, the count of chunks the run leaves, and the points the attempt's
// recorder reserves.
struct ChunkPlan
{
  std::uint64_t chunkIterations = 1;
  std::uint64_t chunkCount = 1;
  std::size_t pointCapacity = 2;
};

// specs/021 D-03, FR-005, FR-006: the chunk plan of one attempt.
//
// The 1% rule sets the floor: a chunk holds the least count of iterations
// whose time stands at 100 times the cost of one sample action, so one
// sample costs at most 1% of the window it bounds. The point ceiling then
// raises the chunk size when the run would otherwise leave more chunks
// than the ceiling holds. An attempt with no measured iteration time of
// its own, the first of a fixed-count run, takes the ceiling alone.
//
// \pre iterations >= 1, iterationNs >= 0, sampleOverheadNs >= 0, and
//      pauseRoom <= kMaxPauseSamplesPerLeaf
// \post chunkCount >= 1, chunkIterations >= 1,
//       chunkCount <= kMaxChunkPointsPerLeaf,
//       pointCapacity == chunkCount + 1 + pauseRoom, and
//       (chunkCount - 1) * chunkIterations < iterations <=
//       chunkCount * chunkIterations
[[nodiscard]] inline auto chunkPlanFor(const std::uint64_t iterations,
                                       const double iterationNs,
                                       const double sampleOverheadNs,
                                       const std::size_t pauseRoom) noexcept
    -> ChunkPlan
{
  std::uint64_t chunkIterations = 1;
  if (iterationNs > 0.0 && sampleOverheadNs > 0.0) {
    const double wanted = 100.0 * sampleOverheadNs / iterationNs;
    if (wanted > 1.0) {
      chunkIterations = static_cast<std::uint64_t>(std::ceil(wanted));
    }
  }
  // The ceiling counts the chunk points alone: the endpoint point and the
  // pause room stand beside them, so the pause room never changes how the
  // run divides into chunks.
  const std::uint64_t maxChunks = kMaxChunkPointsPerLeaf;
  std::uint64_t chunkCount =
      (iterations + chunkIterations - 1) / chunkIterations;
  if (chunkCount > maxChunks) {
    chunkCount = maxChunks;
    chunkIterations = (iterations + chunkCount - 1) / chunkCount;
    chunkCount = (iterations + chunkIterations - 1) / chunkIterations;
  }
  return ChunkPlan {
      .chunkIterations = chunkIterations,
      .chunkCount = chunkCount,
      .pointCapacity = static_cast<std::size_t>(chunkCount) + 1 + pauseRoom,
  };
}

// One sampled run: the sample span the run owns, the fold of each clock
// over that span, the paused intervals to net out, and the iteration
// count the benchmark function asked for. Feature 017 replaced the H1
// pair field with the point cursor's span: the record owns its base
// point, its sample count, and its pause ordinals.
struct RunRecord
{
  std::uint64_t iterations = 0;
  std::int64_t realNs = 0;
  std::int64_t decisionNs = 0;
  bool skipped = false;
  std::string reason;
  std::size_t basePoint = 0;
  std::uint64_t samples = 0;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> pausePairs;
  std::int64_t cpuNs = 0;
  bool manual = false;
  // The fold path of FR-052: the points the run's recorder holds, the
  // running totals of the pause pairs that folded at their commit, and
  // whether that run folded one. The raw path folds none, so these
  // stand at their defaults and the columns keep their bytes.
  std::uint64_t foldPoints = 0;
  // specs/021 FR-001, FR-002: the chunk plan this run captured: the count
  // of chunks it left and the iterations one chunk holds. A chunk count of
  // zero names a run that captured no chunks.
  std::uint64_t chunkCount = 0;
  std::uint64_t chunkIterations = 0;
  PauseFoldSink pauseFold;
  bool pauseSamplesFolded = false;
  // E-01, FR-001: the counter store the run opened, in the order the names
  // opened. The name copies out of the fixed slot, so the record owns its
  // text and the flag pass runs after the loop on this snapshot (FR-003).
  std::vector<std::string> counterNames;
  std::vector<Counter> counterStore;
  // D-04, FR-011: the label the repetition ended with, copied out of the
  // state after the loop, one copy per repetition.
  std::string label;
  // specs/037-execution-graphs FR-019, FR-023, D-5 (T018): the settlement of
  // one threaded graph attempt, one entry per node in the ascending slot
  // order FR-023 fixes. A run with no graph carries none.
  std::vector<GraphSettlement> graphSettlement;
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
// phase stops at. D-05: the fivefold condition compares the wall-clock
// fold against the target, and a manual run has no clock fold to
// compare, so the manual flag exempts that one condition. FR-020, FR-046:
// the cap stands on the count one context ran, the quantity the cited
// revision caps (it compares kMaxIterations against the count each thread
// runs); `record.iterations` sums every context of a threaded sample, so the
// caller names the per-context count here and the two units never mix.
inline auto qualifies(const RunRecord& record,
                      const std::int64_t targetNs,
                      const std::uint64_t perContextCount) -> bool
{
  if (perContextCount >= kIterationCap || record.decisionNs >= targetNs) {
    return true;
  }
  return !record.manual && record.realNs >= fivefoldNs(targetNs);
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
    if (qualifies(*record, targetNs, count)) {
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
