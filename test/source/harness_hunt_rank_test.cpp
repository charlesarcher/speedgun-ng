// ============================================================================
// The hunt rank decision table (specs/029-hunter-core T006, filled by T016;
// US1; FR-030; SC-03; research D-3, D-6, D-7, PD-07, PD-08, PD-14; data-model
// E-05, E-06, E-11, I-5, I-6, I-7).
//
// The seam is `sg::hunt::rankEvents` over literal `EventSamples` records: the
// ranker takes the sample list and the settled options and reads no clock, no
// counter and no `/proc`, so every row of the table stands without a provider
// (I-7). The rate and CV expectations are the figures that
// `sg::detail::statisticsOf` produces for the same sample list, never values
// mirrored here, which is what makes the factor-of-one-hundred claim of I-6
// able to fail.
//
// The file stands empty of cases until T016 fills the scenario list below;
// the list is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Frameworkless check()/fail() convention,
// the one `harness_pass_plan_test.cpp` carries.
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
  std::fprintf(stderr, "HARNESS HUNT RANK TEST FAIL: %s\n", what);
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
// `argv[1]` selects from. T016 fills the list with the rank decision table of
// US1.
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
  std::printf("harness_hunt_rank_test: ok\n");
  return 0;
}
