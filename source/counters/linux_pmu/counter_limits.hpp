// ============================================================================
// The programmable-counter limit the core PMU answers, as a count
// (specs/024-multipass-collection FR-005, FR-032; research D-01, D-04;
// data-model E-02).
//
// Two pieces, split on purpose. The probe executes the two CPUID leaves the
// limit lives in and hands back the raw registers with no decision of its own;
// the decode is a pure function of three integers and holds every decision,
// the guard on the leaf itself included. A CPUID above the maximum leaf
// answers the maximum leaf's registers, whose low byte is vendor-string text
// rather than a version, so those registers mean nothing until `maxLeaf`
// vouches for them, and a guard inside the probe would be one hardware arm no
// CI host can cover without a new exclusion marker (research D-04, FR-032).
//
// The seam is private: never installed, and read by exactly two places, the
// perf PMU provider's construction and the decision table of
// `test/source/counters_counter_limits_test.cpp`, which drives every arm of
// the decode with registers no host has to provide.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_COUNTER_LIMITS_HPP
#define SG_COUNTERS_LINUX_PMU_COUNTER_LIMITS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sg::counters::detail
{

/// The three registers the programmable-counter limit is decoded from.
///
/// `maxLeaf` is `EAX` of leaf 0, the highest leaf the host implements; `eax`
/// and `ebx` are those two registers of leaf 0AH, the architectural
/// performance-monitoring discovery leaf (research D-01). Raw values, before
/// any decision: the decode is what turns them into an answer.
struct CounterLimitRegisters
{
  std::uint32_t maxLeaf;
  std::uint32_t eax;
  std::uint32_t ebx;
};

/// Reads leaf 0 for the highest leaf the host implements and leaf 0AH for the
/// performance-monitoring answer.
///
/// Branchless by decision. Executing a leaf above the maximum is defined
/// behavior, the processor answers the maximum leaf's registers, so the guard
/// belongs to the decode and the probe keeps no arm whose other side no CI
/// host can reach (research D-04).
///
/// \pre none
/// \post On x86 the three fields hold exactly what the two leaves answered,
/// whatever the host. Off x86 they hold zero, which the decode reads as no
/// answer.
[[nodiscard]] auto readCounterLimitRegisters() noexcept
    -> CounterLimitRegisters;

/// How many general-purpose counters support programmable events on this
/// core, or nothing when the host cannot answer (FR-005).
///
/// `EAX[7:0]` is the architectural performance-monitoring version and
/// `EAX[15:8]` the count of counters that support programmable events, the
/// field split research D-01 records. The answer exists when `maxLeaf`
/// vouches for leaf 0AH and both fields are nonzero, and equals the count.
/// Every other triple answers nothing: a published zero limit would empty
/// every pass of the plan that reads it.
///
/// \pre none: every register triple is a legal input, including the clamped
///      registers of a host without leaf 0AH.
/// \post A value is a positive count taken from `EAX[15:8]`; no value means
///       the leaf is absent, the version reads zero, or the count reads zero
///       (FR-005). The decision depends on no register but these three.
[[nodiscard]] auto programmableCountersFromCpuid(std::uint32_t maxLeaf,
                                                 std::uint32_t eax,
                                                 std::uint32_t ebx)
    -> std::optional<std::size_t>;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_LINUX_PMU_COUNTER_LIMITS_HPP
