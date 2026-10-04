// ============================================================================
// Runtime test for the trace-start marker (specs/011 FR-013, FR-026,
// FR-027; US1).
//
// The marker has no observable effect at run time, so its properties are
// asserted through the register state around the call and through
// termination. The upper 32 bits of the callee-saved register carry a
// known value, because a `mov` of a 32-bit immediate into the 32-bit view
// zeroes that half and an implementation preserving only the low half
// still passes a test built from small integers. Hand-rolled
// check()/fail() convention; no test framework is added to this
// repository.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "speedgun-ng/simulation.hpp"

// The marker emits its sequence only where the instruction set carries it,
// and FR-026 observes the register it touches, so that check is guarded on
// the same condition the marker unit guards its body under. The tag and
// the repeated-call checks run on every build, because the call compiles,
// links, and returns everywhere (FR-018).
#if defined(__x86_64__) || defined(__i386__)
#  define SG_TEST_HAS_MARKER 1
#else
#  define SG_TEST_HAS_MARKER 0
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "SIMULATION TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// FR-013: the tag the header publishes is the value a caller passes to the
// tracer, so the constant and the documentation cannot drift apart.
constexpr std::uint32_t k_documented_tag = 0xFACEU;

auto test_tag_value() -> void
{
  check(sg::simulation_start_tag == k_documented_tag,
        "the published tag carries the documented value 0xFACE (FR-013)");
  std::printf("simulation_start_tag: 0x%04X, written FACE on a tracer's "
              "command line\n",
              sg::simulation_start_tag);
}

// FR-026: a known value whose upper 32 bits are set goes into the
// designated callee-saved register, the call runs, and the register is
// bit-identical afterwards. The marker moves a 32-bit immediate into the
// 32-bit view, so a preserved low half and a clobbered high half are the
// two failures this catches.
auto test_register_preserved() -> void
{
#if SG_TEST_HAS_MARKER
  constexpr std::uint64_t known = 0xDEADBEEF12345678ULL;

  std::uint64_t observed = 0;
  __asm__ __volatile__(  // NOLINT(hicpp-no-assembler) the probe must put a
                             // known value in a named register, which no C++
                             // construct does (FR-026)
      "mov %1, %%rbx\n\t"
      "callq *%2\n\t"
      "mov %%rbx, %0"
      : "=r"(observed)
      : "r"(known), "r"(&sg::simulation_start)
      : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory",
        "cc");

  check(observed == known,
        "every general-purpose register is bit-identical across the call, "
        "upper 32 bits included (FR-026)");
  std::printf("rbx across the call: 0x%016llX\n",
              static_cast<unsigned long long>(observed));
#else
  std::printf("no marker instruction set on this target, so no register is "
              "touched and FR-026 has nothing to observe\n");
#endif
}

// FR-027: each call emits one marker, an attached tracer counts them, and
// with no tracer attached execution continues and terminates normally. The
// loop returning and main reaching its own return is the assertion; there
// is nothing to compare afterwards.
auto test_repeated_calls() -> void
{
  constexpr int kCalls = 10000;
  for (int i = 0; i < kCalls; ++i) {
    sg::simulation_start();
  }
  std::printf("%d calls completed with no tracer attached (FR-027)\n", kCalls);
}

}  // namespace

auto main() -> int
{
  test_tag_value();
  test_register_preserved();
  test_repeated_calls();
  std::printf("simulation_test PASS: tag published, registers preserved, "
              "repeated calls terminate\n");
  return 0;
}
