// ============================================================================
// Skeleton of the pass plan tables (specs/024 T007; the scenarios land with
// T016 to T021 and T029 to T031; FR-001 to FR-008, FR-033, research D-05,
// D-07, D-09).
//
// The first-fit split, the merge and the spread are pure functions over a
// scripted column list, a method and a published limit, so every table here
// stands without a run: the request-order assignment, an over-limit column
// alone in an exclusive pass, the empty set, the minimum pass count, the
// gaps the merge keeps, and the shapes of every method row. The suite is
// registered and runs now so the build stays green before the first
// assertion lands. Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PASS PLAN TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The scenario of the skeleton: it states that the registered binary runs
// its scenario list. T016 replaces it with the split table.
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
  std::printf("harness_pass_plan_test: ok\n");
  return 0;
}
