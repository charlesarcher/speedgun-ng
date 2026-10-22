// ============================================================================
// The hunt console tables (specs/029-hunter-core T006, filled by T032; US2;
// FR-040; SC-06; research PD-13, PD-15; data-model E-07, I-8;
// quickstart.md US2 walkthrough).
//
// The seam is the printer: a scenario builds or runs the record and reads the
// bytes `sg::detail::printResult` sends to the process stdout, captured to a
// scratch file the way `harness_pass_report_test.cpp` captures them, so every
// claim is a printed byte of the report. The line shapes are
// the seven rows of data-model E-07 in the PD-15 order, compared character for
// character, and the plan-line figures are the quickstart's.
//
// The file stands empty of cases until T032 fills the scenario list below; the
// list is what CTest runs, so an entry absent from it never runs, and `argv[1]`
// selects one entry of it. Frameworkless check()/fail() convention, the one
// `harness_pass_report_test.cpp` carries.
// ============================================================================

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from. T032 fills the list with the console tables of US2.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {};
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const auto& list = scenarios();
  // One name selects one scenario through `argv[1]`, so a name that stands
  // twice would hide the scenario it repeats.
  for (std::size_t first = 0; first < list.size(); ++first) {
    check(list[first].body != nullptr, "every scenario carries a body");
    for (std::size_t second = first + 1; second < list.size(); ++second) {
      check(list[first].name != list[second].name,
            "no two scenarios of this file carry the same name");
    }
  }
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : list) {
      if (scenario.name == mode) {
        scenario.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : list) {
    scenario.body();
  }
  std::printf("harness_hunt_report_test: ok\n");
  return 0;
}
