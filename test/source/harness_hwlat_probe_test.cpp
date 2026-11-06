// ============================================================================
// T011 (specs/048-hwlat-probe): the probe path of the `hwlat` method. One
// read of the counter stands beside each kept gap, the change of the span
// that ends at a gap marks that gap, and the two window deltas and the
// marked count fold onto that cpu's figures (FR-012 to FR-017, D-7, P-3).
// Wave W3 writes the rows; this unit stands open with the frameworkless
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
