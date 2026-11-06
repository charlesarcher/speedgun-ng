// ============================================================================
// T002 (specs/048-hwlat-probe): the detector engine. The setup writes the
// mask, the `per-cpu` mode, the two knob values, the threshold, the engine
// word and the trace switch in that order, saves the six values it
// overwrites, and restores them in the reverse order on every path the
// process controls (FR-018, FR-019, FR-020, FR-021, FR-022, FR-023). Wave
// W2 writes the rows; this unit stands open with the frameworkless
// check()/fail() convention, the way harness_noise_test carries it.
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
