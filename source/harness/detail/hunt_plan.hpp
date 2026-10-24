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

/// @brief specs/029 D-2, FR-012, FR-013, FR-014 (T033, T125): the stage split
/// of one plan, with the budget cut verdict.
///
/// FR-014 derives a stage's columns from the seed and the catalog order only,
/// so the split reads no measured rate: stage 2 takes the leading columns of
/// the plan order. The calibration figure converts the seconds budget into
/// passes and is the same figure `estimateNs` multiplies. A seconds budget that
/// figure cannot price is a budget this split answers as unfit, because no
/// clock stands here to weigh it a second way.
///
/// \pre `plan.columns` is not empty; `options` satisfies its own precondition
/// \post with no budget `stages` holds one stage over every column at
///       `options.repeats`; with a budget it holds stage 1 over every column at
///       one repeat and stage 2 over the leading `3 x top` columns in column
///       order, capped at the column count, at `options.repeats`
///       (D-2, FR-013, FR-014); `cutStage2` is true with the word
///       `over-budget` in `cutReason` exactly when the two-stage estimate
///       exceeds the budget in the unit the caller named, or when a seconds
///       budget leaves that unit unpriced for want of a calibration figure,
///       `stages` then holding stage 1 alone (FR-012, FR-013, I-7); no
///       stage carries a reduced repeat count and no column leaves stage 1
///       (FR-013)
[[nodiscard]] auto splitStages(
    const hunt::HuntPlan& plan,
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

/// @brief specs/029 D-18, spec PC-1 (T118): the availability one refusal of a
/// seat names.
///
/// A refusal the counters layer can name carries the catalog state that
/// refused it. `compileCore` composes such a message from the word
/// `availabilityName` gives that state at
/// `source/counters/plan.cpp:485-490`, and the state itself is the verdict
/// the discovery probe settled for the entry at
/// `source/counters/linux_pmu/provider.cpp:773-786`. Reading the word reads
/// the probe's answer, so the errno rule is stated once, beside the switch
/// that applies it, and no second errno table stands in the harness.
///
/// \pre none: every error text answers a state or names none
/// \post the availability whose counters-layer word `refusal` names, and
///       `std::nullopt` for a refusal that names no state: the group open a
///       provider answers with a null window carries no state, so no kind is
///       guessed for it (D-18, FR-021)
[[nodiscard]] auto availabilityOfRefusal(
    const counters::Error& refusal) noexcept
    -> std::optional<counters::Availability>;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_HUNT_PLAN_HPP
