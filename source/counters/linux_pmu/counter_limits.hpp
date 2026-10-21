// ============================================================================
// The programmable-counter limit this core answers, per vendor and per
// device (specs/024-multipass-collection FR-005, FR-032; research D-01,
// D-04; data-model E-02).
//
// Three pieces, split on purpose. The probe executes one `__get_cpuid_count`
// per leaf D-01 names and hands back the raw words with no decision of its
// own; every word is zero-initialized before its call, and a leaf that
// answers false leaves its registers untouched, so an absent leaf reads as
// zeros and the probe keeps no arm (research D-04). The decoders are pure
// functions of those words: the vendor from the leaf 0 words, the Intel and
// Zhaoxin core answer from leaf 0AH, the AMD core answer from the extended
// leaves, the fixed cycles answer from leaf 0AH `EDX`, and the leaf 0x23
// answer. The adjustment is pure too: the watchdog value, the hybrid answer
// and the device name turn a hardware count into the published limit, and
// every rule is a row the decision table of
// `test/source/counters_counter_limits_test.cpp` drives with words no host
// has to provide.
//
// The seam is private: never installed, and read by exactly two places, the
// perf PMU provider's enumeration and that decision table.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_COUNTER_LIMITS_HPP
#define SG_COUNTERS_LINUX_PMU_COUNTER_LIMITS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace sg::counters::detail
{

/// The vendors whose core counter answer this header decodes. The leaf 0
/// vendor words decide it (research D-01).
enum class CounterLimitVendor : std::uint8_t
{
  INTEL,  ///< `GenuineIntel`.
  ZHAOXIN,  ///< `CentaurHauls` and `Shanghai`, which answer through the
            ///< same leaf 0AH fields (research D-01).
  AMD,  ///< `AuthenticAMD` and `HygonGenuine`, which answer through
        ///< the extended leaves (research D-01).
  OTHER,  ///< Any other vendor words, or no vendor words at all.
};

/// The register words the counter-limit decodes read, raw and before any
/// decision. Each field holds what the named leaf's named register answered;
/// a leaf the host does not answer leaves its fields zero (research D-04).
struct CounterLimitRegisters
{
  std::uint32_t maxLeaf;  ///< Leaf 0 `EAX`: the highest standard leaf.
  std::uint32_t vendorEbx;  ///< Leaf 0 `EBX`: the first vendor word.
  std::uint32_t vendorEdx;  ///< Leaf 0 `EDX`: the second vendor word.
  std::uint32_t vendorEcx;  ///< Leaf 0 `ECX`: the third vendor word.
  std::uint32_t perfMonEax;  ///< Leaf 0AH `EAX`: version and counter count.
  std::uint32_t perfMonEdx;  ///< Leaf 0AH `EDX`: the fixed-counter count.
  std::uint32_t unified0Eax;  ///< Leaf 0x23 sub-leaf 0 `EAX`: the version
                              ///< word, nonzero on any host that answers.
  std::uint32_t unified1Eax;  ///< Leaf 0x23 sub-leaf 1 `EAX`: the second
                              ///< version word, nonzero on any host that
                              ///< answers it.
  std::uint32_t maxExtended;  ///< Leaf 0x80000000 `EAX`: the highest
                              ///< extended leaf, the gate on the AMD rule.
  std::uint32_t extendedEcx;  ///< Leaf 0x80000001 `ECX`: carries the
                              ///< `PerfCtrExtCore` bit.
  std::uint32_t amdPerfEax;  ///< Leaf 0x80000022 `EAX`: carries the
                             ///< `PerfMonV2` bit.
  std::uint32_t amdPerfEbx;  ///< Leaf 0x80000022 `EBX`: `EBX[3:0]` counts
                             ///< the core counters under `PerfMonV2`.
};

/// Reads every leaf the counter-limit decodes consume, in one straight
/// line: leaf 0, leaf 0AH, leaf 0x23 sub-leaves 0 and 1, leaf 0x80000000,
/// leaf 0x80000001 and leaf 0x80000022.
///
/// Branchless by decision. Every word is zero-initialized before its call,
/// and `__get_cpuid_count` leaves the registers untouched when the leaf
/// exceeds the answered maximum, so an absent leaf reads as zeros and the
/// probe keeps no guard (research D-04).
///
/// \pre none
/// \post On x86 each field holds exactly what its leaf answered, or zero
/// when the leaf did not answer. Off x86 every field is zero, which every
/// decode reads as no answer.
[[nodiscard]] auto readCounterLimitRegisters() noexcept
    -> CounterLimitRegisters;

/// Which vendor's rule the registers answer through.
///
/// \pre none: every register word is a legal input.
/// \post `GenuineIntel` answers INTEL, `CentaurHauls` and `Shanghai` answer
///       ZHAOXIN, `AuthenticAMD` and `HygonGenuine` answer AMD, and every
///       other word triple answers OTHER (research D-01).
[[nodiscard]] auto vendorOf(const CounterLimitRegisters& registers) noexcept
    -> CounterLimitVendor;

/// The Intel and Zhaoxin core answer: leaf 0AH `EAX[15:8]` when leaf 0AH
/// `EAX[7:0]` is not zero, and no value when it is (research D-01).
///
/// \pre none: the registers of a host without leaf 0AH read zero and answer
///      no value on their own.
/// \post A value is `EAX[15:8]` as read; a zero count stays zero here and
///       `usableCounters` publishes it as no value (FR-005).
[[nodiscard]] auto intelCoreLimit(
    const CounterLimitRegisters& registers) noexcept
    -> std::optional<std::size_t>;

/// The AMD core answer from the extended leaves (research D-01).
///
/// The leaf 0x80000022 words count only when `maxExtended` reaches that
/// leaf. With `PerfMonV2` (leaf 0x80000022 `EAX` bit 0) set the answer is
/// `EBX[3:0]`; otherwise `PerfCtrExtCore` (leaf 0x80000001 `ECX` bit 23)
/// answers 6, and the pre-extension core PMU answers 4.
///
/// \pre none
/// \post Always a value: `EBX[3:0]` under the gated `PerfMonV2` bit, else
///       6 with `PerfCtrExtCore`, else 4 (FR-005).
[[nodiscard]] auto amdCoreLimit(const CounterLimitRegisters& registers) noexcept
    -> std::optional<std::size_t>;

/// Whether the host has a fixed cycles counter: leaf 0AH `EDX[4:0]` counts
/// the fixed counters, and a version of at least 2 with a count of at least
/// 2 means the fixed cycles counter is there (research D-01).
///
/// \pre none
/// \post True exactly when `EAX[7:0]` is at least 2 and `EDX[4:0]` is at
///       least 2.
[[nodiscard]] auto fixedCycles(const CounterLimitRegisters& registers) noexcept
    -> bool;

/// The watchdog answer from one line of `/proc/sys/kernel/nmi_watchdog`:
/// the parsed number, or -1 when the text carries no number (research
/// D-01's watchdog table).
///
/// \pre none: any text, empty included, is a legal input.
/// \post A value is the parsed integer; -1 means the parse failed.
[[nodiscard]] auto watchdogValueFrom(std::string_view text) noexcept -> int;

/// Reads `/proc/sys/kernel/nmi_watchdog` and hands the first line to
/// `watchdogValueFrom`. Branchless: a failed open leaves the line empty and
/// the parse answers -1, so the read keeps no arm.
///
/// \pre none
/// \post The watchdog value: 1 enabled, 0 released, -1 when the read or the
///       parse failed (research D-01).
[[nodiscard]] auto readWatchdogValue() noexcept -> int;

/// Whether a device list is a hybrid host's: it carries one of the Intel
/// hybrid core names, `cpu_core` or `cpu_atom` (research D-01's hybrid
/// rule).
///
/// \pre none
/// \post True exactly when some name equals `cpu_core` or `cpu_atom`.
[[nodiscard]] auto isHybrid(std::span<const std::string_view> deviceNames)
    -> bool;

/// The hardware counter answer for one device, before the watchdog
/// adjustment. The core devices (`cpu`, `cpu_core`, `cpu_atom`) answer
/// through the vendor's rule; every other device answers nothing; a hybrid
/// host whose leaf 0x23 answers publishes nothing, because one host-wide
/// core figure would be wrong for both core types (research D-01's device
/// table and hybrid rule).
///
/// \pre none
/// \post The vendor's core answer for a core device, nothing otherwise; a
///       zero core answer survives as zero for `usableCounters` to publish
///       as no value.
[[nodiscard]] auto limitForDevice(std::string_view deviceName,
                                  CounterLimitVendor vendor,
                                  bool hybrid,
                                  const CounterLimitRegisters& registers)
    -> std::optional<std::size_t>;

/// The published limit: the hardware answer adjusted for the watchdog
/// (research D-01's adjustment table). The first matching rule wins: a
/// watchdog value that is not 1 (a failed read gives -1) leaves the
/// hardware answer; Intel or Zhaoxin with a fixed cycles counter leaves it;
/// Intel or Zhaoxin without one, AMD, and any other vendor take one away.
/// A result of zero answers no value (FR-005).
///
/// \pre none
/// \post A value is the adjusted count; no value means the hardware answer
///       was no value, the adjustment reached zero, or the vendor answer
///       carries no count.
[[nodiscard]] auto usableCounters(std::optional<std::size_t> hardware,
                                  CounterLimitVendor vendor,
                                  int watchdogValue,
                                  bool hasFixedCycles) noexcept
    -> std::optional<std::size_t>;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_LINUX_PMU_COUNTER_LIMITS_HPP
