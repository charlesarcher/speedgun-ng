// ============================================================================
// The CPUID read of the programmable-counter limit and its pure decode
// (specs/024-multipass-collection FR-005, FR-032; research D-01, D-04).
//
// The probe holds no decision and the decode holds no instruction, which is
// what keeps the hardware-dependent half branch-free and the decision half
// drivable from a table (research D-04, FR-032). See the header for the
// contract each piece keeps.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <optional>

#include "counter_limits.hpp"

#if defined(__x86_64__) || defined(__i386__)
#  include <cpuid.h>
#endif

namespace sg::counters::detail
{
namespace
{

/// CPUID leaf 0AH, the architectural performance-monitoring discovery leaf.
constexpr std::uint32_t kProgrammableLeaf = 0x0AU;

/// The width of one `EAX` field of leaf 0AH: the version sits in `EAX[7:0]`
/// and the counter count in `EAX[15:8]` (research D-01).
constexpr std::uint32_t kFieldMask = 0xFFU;

/// The shift from `EAX[7:0]` to `EAX[15:8]`: the count of general-purpose
/// counters that support programmable events (research D-01).
constexpr unsigned int kCountShift = 8U;

}  // namespace

auto readCounterLimitRegisters() noexcept -> CounterLimitRegisters
{
  CounterLimitRegisters registers {};
#if defined(__x86_64__) || defined(__i386__)
  unsigned int eax = 0;
  unsigned int ebx = 0;
  unsigned int ecx = 0;
  unsigned int edx = 0;
  // Straight-line by decision. `__get_cpuid` returns its own leaf test, and
  // testing it here would be a hardware-dependent branch whose other side
  // no CI host can reach; the leaf-0 answer gates the read instead, inside
  // the pure decode (research D-04, FR-032). Leaf 0AH takes no sub-leaf, so
  // `ecx` stays zero for both reads.
  (void)__get_cpuid(0U, &eax, &ebx, &ecx, &edx);
  registers.maxLeaf = eax;
  (void)__get_cpuid(kProgrammableLeaf, &eax, &ebx, &ecx, &edx);
  registers.eax = eax;
  registers.ebx = ebx;
#endif
  return registers;
}

/// `ebx` is the programmable-counter event vector that `EAX[31:24]` counts
/// the entries of, defined once the version exceeds 1 (research D-01). It is
/// ignored here by decision: measured on this host, an Intel i9-9920X
/// (Cascade Lake) on 2026-10-20, the leaf reads `EAX=0x07300404` with
/// `EBX=0x00000000`, four programmable counters and an empty vector, so the
/// count field is authoritative and the vector subtracts nothing. The
/// parameter stays in the signature so that omission stays a decision the
/// decision table pins with a nonzero vector (research D-04).
auto programmableCountersFromCpuid(const std::uint32_t maxLeaf,
                                   const std::uint32_t eax,
                                   [[maybe_unused]] const std::uint32_t ebx)
    -> std::optional<std::size_t>
{
  const auto version = eax & kFieldMask;
  const auto count = (eax >> kCountShift) & kFieldMask;
  if (maxLeaf < kProgrammableLeaf || version == 0U || count == 0U) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(count);
}

}  // namespace sg::counters::detail
