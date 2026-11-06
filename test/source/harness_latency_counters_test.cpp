// ============================================================================
// T001 (specs/048-hwlat-probe): the two counter sources the `hwlat` method
// reads. The model-specific system-management-interrupt counter stands
// behind `dev/cpu/<cpu>/msr` and answers only for a part that documents it
// (FR-007, FR-008, FR-009, FR-010), and the non-maskable-interrupt count
// stands in the `NMI` line of `proc/interrupts` (FR-009). Wave W2 writes
// the rows; this unit stands open with the frameworkless check()/fail()
// convention, the way harness_noise_test carries it.
// ============================================================================

#include <cstdio>

namespace
{

int failures = 0;

[[maybe_unused]] auto fail(const char* what) -> void
{
  ++failures;
  std::printf("FAILED: %s\n", what);
}

[[maybe_unused]] auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

}  // namespace

auto main() -> int
{
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
