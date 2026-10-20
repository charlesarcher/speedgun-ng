// ============================================================================
// Skeleton of the pass disclosure tables (specs/024 T007; the scenarios land
// with T039 to T046; FR-017 to FR-024, FR-033, research D-08, D-11).
//
// The report lines are a fold over a result, so each case compares printed
// text against a golden capture: the `passes` line, the warning line of an
// over-limit column, the pass tag beside a metric column, the published
// fields of the result, every spread fixture of the data model printed with
// its pinned decimal, and the byte-identity of a one-pass report with the
// capture taken at the audit point. The suite is registered and runs now so
// the build stays green before the first assertion lands. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PASS REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The scenario of the skeleton: it states that the registered binary runs
// its scenario list. T039 replaces it with the disclosure cases.
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
  std::printf("harness_pass_report_test: ok\n");
  return 0;
}
