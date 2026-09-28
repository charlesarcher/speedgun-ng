// ============================================================================
// A trap fixture that implements no mode (T291).
//
// It exists so the trap checker's discrimination is covered by the suite.
// The checker's abort branch reads a non-zero status with the survival
// marker absent as proof that a mode reached its guard, which the real
// fixture at counters_trap_fixture.cpp satisfies for every mode it
// implements. A mode registered in the checker's list with no body in the
// real fixture lands on the real fixture's refusal path, which exits 2
// with the same two properties, so the check cannot tell the two apart
// from the status and the marker alone.
//
// This fixture stands in for a bodyless mode. It exits non-zero with the
// marker absent and without the contract facility's report, so the
// checker has to report it. The CTest asserting that failure is
// counters_trap_checked_rejects_unknown_mode; it is not a test of its
// own, and it is never expected to succeed.
// ============================================================================

#include <cstdio>
#include <cstdlib>

auto main(int argc, char** argv) -> int
{
  std::fprintf(
      stderr, "fixture: unknown mode '%s'\n", argc >= 2 ? argv[1] : "");
  return 2;
}
