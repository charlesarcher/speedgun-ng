// ============================================================================
// The CPU identity the mapping-file keys name, decoded from the CPUID
// words (G-19, G-21).
//
// The decode is a pure function of the words, on the pattern of the
// counter-limit decodes in `counter_limits.hpp`: the probe runs the
// real `__get_cpuid_count` once, branchless, and every leaf that does
// not answer leaves its words at zero, so the decode reads an absent
// leaf as no answer and the probe keeps no guard. The tests drive the
// decode with word sets no host has to provide.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_PMU_IDENT_HPP
#define SG_COUNTERS_LINUX_PMU_PMU_IDENT_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include "../detail/pmu.hpp"  // PmuIdent

namespace sg::counters::detail
{

/// The CPUID answers the ident decode reads, raw. A leaf the CPU does
/// not answer leaves its flag false and its words at zero.
struct CpuidWords
{
  std::uint32_t leaf0Eax = 0;  ///< Leaf 0 `EAX`: the highest basic leaf.
  std::uint32_t leaf0Ebx = 0;  ///< Leaf 0 `EBX`: the first vendor word.
  std::uint32_t leaf0Edx = 0;  ///< Leaf 0 `EDX`: the second vendor word.
  std::uint32_t leaf0Ecx = 0;  ///< Leaf 0 `ECX`: the third vendor word.
  bool leaf1Answered = false;  ///< Leaf 1 answered.
  std::uint32_t leaf1Eax = 0;  ///< Leaf 1 `EAX`: family, model, stepping.
};

/// Reads the words the ident decode consumes, in one straight line:
/// leaf 0 and leaf 1. Branchless by decision, the way
/// `readCounterLimitRegisters` is: a leaf that does not answer leaves
/// its words at zero, so the decode reads an absent leaf as no answer
/// and the probe keeps no arm.
///
/// \pre none
/// \post On x86 each field holds exactly what its leaf answered, or
///       zero when the leaf did not answer. Off x86 every field is at
///       its default, which the decode reads as no answer.
[[nodiscard]] auto readCpuidWords() noexcept -> CpuidWords;

/// The identity the words decode to.
///
/// \pre none: every word is a legal input.
/// \post The vendor string holds leaf 0's `EBX:EDX:ECX` bytes when
///       leaf 0 answers with a highest basic leaf of one or more, and
///       stays empty when it does not. The family, the model, and the
///       stepping hold leaf 1's decode when leaf 1 answered, and their
///       zero defaults when it did not.
[[nodiscard]] auto pmuIdentFromWords(const CpuidWords& words) -> PmuIdent;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_LINUX_PMU_PMU_IDENT_HPP
