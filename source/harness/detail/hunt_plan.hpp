#ifndef SG_HARNESS_DETAIL_HUNT_PLAN_HPP
#define SG_HARNESS_DETAIL_HUNT_PLAN_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/hunt.hpp"

/**
 * @file hunt_plan.hpp
 * @brief The hunt internals of specs/029: the two records the run path keeps
 * to itself and the named internal cores behind the public calls.
 *
 * Nothing here is public API (D-15): no type declared here reaches a
 * signature of `speedgun-ng/hunt.hpp`, which names `hunt::` types alone, and
 * no harness header reaches this one from there. The plan and rank cores
 * themselves are public in `speedgun-ng/hunt.hpp` and stand defined in
 * `source/harness/hunt_plan.cpp`; what stands here is the stage split with
 * its cut verdict, the record of one retried pass, and the two discovery-side
 * mappings the run driver calls. No clock, no counter read, no `/proc`, no
 * sysfs and no CPUID enters any declaration here (I-7).
 */

namespace sg::detail
{

// specs/029 D-2, FR-012, FR-013, PD-05: the stage split of one plan. `stages`
// holds the one stage of an un-budgeted plan or the two of a budgeted one, and
// `cutStage2` states that the two-stage estimate did not fit the budget, so
// the run holds stage 1 alone and the report prints the one `budget cut` line
// naming `cutReason`. Stage 1 never shrinks and no stage carries a reduced
// repeat count (FR-013).
struct StageSplit
{
  std::vector<hunt::HuntStage> stages;
  bool cutStage2 = false;
  std::string cutReason;
};

// specs/029 D-18, PD-12: one pass whose group open refused and whose events
// were measured one at a time inside the same loop. `pass` is the index of the
// pass that refused and `loneColumns` the column indices the appended
// one-event passes carry, in the order the loop ran them. The runner records
// one per retried pass and `EntryHunt::retriedPasses` counts them (FR-024).
struct RetriedPass
{
  std::size_t pass = 0;
  std::vector<std::size_t> loneColumns;
};

/// @brief specs/029 D-2, FR-012, FR-013 (T033): the stage split of one plan,
/// with the budget cut verdict.
///
/// `stageOneRates` holds the stage-1 median rate of every column in column
/// order, the figure the run collected in stage 1 and the only ordering stage
/// 2 may read; the calibration figure converts the seconds budget into passes
/// and is the same figure `estimateNs` multiplies.
///
/// \pre `stageOneRates` holds one entry per entry of `plan.columns`;
///      `plan.columns` is not empty; `options` satisfies its own precondition
/// \post with no budget `stages` holds one stage over every column at
///       `options.repeats`; with a budget it holds stage 1 over every column at
///       one repeat and stage 2 over the top `3 x top` columns by
///       `stageOneRates`, capped at the column count, at `options.repeats`
///       (D-2, FR-013); `cutStage2` is true with a non-empty `cutReason`
///       exactly when the two-stage estimate exceeds the budget in the unit
///       the caller named, `stages` then holding stage 1 alone; no stage
///       carries a reduced repeat count and no column leaves stage 1 (FR-013)
[[nodiscard]] auto splitStages(
    const hunt::HuntPlan& plan,
    const std::vector<double>& stageOneRates,
    const hunt::HuntOptions& options,
    std::optional<std::uint64_t> calibrationNsPerPass) -> StageSplit;

/// @brief specs/029 D-10, FR-021, FR-050 (T022): the one mapping from a
/// discovery `Availability` to the refusal kind a skipped entry carries.
///
/// The single named mapping of D-10: the console prints the kind through
/// `report.cpp availabilityName`, so no second name or kind table exists.
///
/// \pre `availability` is not `counters::Availability::COUNTABLE`, the state
///      that turns a would-be column into a skipped entry
/// \post the answer is the same-named `SkippedKind` enumerator, so
///       `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED` and
///       `GAP` answer themselves and no other `Availability` answers a kind
[[nodiscard]] auto skippedKindOf(counters::Availability availability) noexcept
    -> hunt::SkippedKind;

/// @brief specs/029 D-18, spec PC-1 (T026): the refusal kind of a group open
/// that still fails when its event is opened alone.
///
/// `refusal` is the error the lone retry answered, mapped the way the counters
/// discovery probe maps an open refusal at
/// `source/counters/linux_pmu/provider.cpp:774-786`.
///
/// \pre none: every error text answers a kind
/// \post `EINVAL` or `EOPNOTSUPP` in the refusal answer `NOT_ENCODABLE` and
///       `EACCES` or `EPERM` answer `PERMISSION_BLOCKED`; any other refusal
///       answer `ABSENT`, the state the run reports for an event that
///       produced no value (D-18, FR-021)
[[nodiscard]] auto skippedKindOf(const counters::Error& refusal)
    -> hunt::SkippedKind;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_HUNT_PLAN_HPP
