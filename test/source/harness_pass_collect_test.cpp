// ============================================================================
// Skeleton of the pass execution tables (specs/024 T007; the scenarios land
// with T022 to T028 and T032 to T034; FR-009 to FR-016, FR-033, research
// D-06, D-10).
//
// A benchmark runs once per pass over a scripted provider, so every case
// here asserts a provider action count observed after the join: the
// calibration that runs once for the whole set, the compile, open and close
// per pass, the merged columns that answer from the pass that measured them,
// and the window order of two contexts over two passes with no sleep. The
// suite is registered and runs now so the build stays green before the first
// assertion lands. Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PASS COLLECT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The scenario of the skeleton: it states that the registered binary runs
// its scenario list. T022 replaces it with the pass loop cases.
auto registeredSuiteRunsItsScenarioList() -> void
{
  check(true, "the registered binary runs its scenario list");
}

}  // namespace

auto main() -> int
{
  // A scenario that needs a fresh process runs in a re-exec of this binary
  // behind an argv[1] dispatch, the pattern of
  // harness_percentile_test.cpp. Every scenario belongs to the list below,
  // which is what ctest runs.
  registeredSuiteRunsItsScenarioList();
  std::printf("harness_pass_collect_test: ok\n");
  return 0;
}
