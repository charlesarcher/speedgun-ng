// The trace-start marker: one extended-assembly statement emitting the
// eight-byte window an attached tracer pattern-matches
// (specs/011 FR-012). The register name and the immediate live here and
// in no public header (specs/011 FR-009).

#include <cstdint>

#include "speedgun-ng/simulation.hpp"

namespace sg
{

// The marker's immediate operand is 32 bits wide, so a tag outside the
// unsigned 32-bit range cannot be encoded. The assertion sits outside
// the architecture guard, so a target with no marker instruction still
// rejects an out-of-range tag (FR-016).
constexpr std::uint32_t k_tag_immediate_max = 0xFFFFFFFFU;

static_assert(simulation_start_tag <= k_tag_immediate_max,
              "the trace-start tag must fit the marker's 32-bit immediate");

auto simulation_start() noexcept -> void
{
#if defined(__x86_64__) || defined(__i386__)
  // EBX is named in the clobber list and nothing else, so the compiler
  // saves and restores the callee-saved register around the statement
  // and every general-purpose register stays bit-identical across the
  // call (FR-015). The memory clobber is absent on purpose: an attached
  // tracer observes executed-instruction order directly, so a barrier
  // would constrain the program's own ordering with no observable effect
  // on collection (FR-017, FR-022).
  __asm__ __volatile__(  // NOLINT(hicpp-no-assembler) the marker bytes are
                         // this feature; a tracer pattern-matches them
                         // and no C++ construct emits them (FR-012)
      "movl $0xFACE, %%ebx\n\t" ".byte 0x64, 0x67, 0x90"
      :
      :
      : "ebx");
#else
  // LCOV_EXCL_START : coverage exclusion (011 T003): no supported build
  // targets a processor family without the marker instruction, so no
  // test on this host can reach this arm (FR-018).
  // LCOV_EXCL_STOP
#endif
}

}  // namespace sg
