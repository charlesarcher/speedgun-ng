// ============================================================================
// The hunt discovery and plan tables (specs/029-hunter-core T006, filled by
// T015; US1; FR-002, FR-003, FR-041; AC-1, SC-01; research D-1, D-9, D-12,
// D-13, PD-01 to PD-04, PD-06; data-model E-01, E-04, I-1).
//
// The seams are the two plan-side units, `sg::hunt::EventCatalog` and
// `sg::detail::planHunt`, both of which read the published catalog and the
// published programmable-counter limits and nothing else, so every table here
// stands without a PMU, without a clock and without a host figure. The
// scripted host they read is `test/source/detail/hunt_fixture.hpp`, the
// fixture that registers the core devices, the `armv8_`-prefixed device, the
// uncore device, the 200 catalog events and the limit rows T015 drives.
//
// The file stands empty of cases until T015 fills the scenario list below;
// the list is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Frameworkless check()/fail() convention,
// the one `harness_pass_plan_test.cpp` and `harness_pass_report_test.cpp`
// carry.
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
  std::fprintf(stderr, "HARNESS HUNT PLAN TEST FAIL: %s\n", what);
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
// `argv[1]` selects from. T015 fills the list with the discovery and plan
// tables of US1.
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
  std::printf("harness_hunt_plan_test: ok\n");
  return 0;
}
