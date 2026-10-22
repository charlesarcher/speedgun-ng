// ============================================================================
// The hunt collection runs (specs/029-hunter-core T006, filled by T017; US1;
// FR-021, FR-022, FR-041, FR-092; AC-2 to AC-4, SC-02 to SC-04; research
// D-10, D-12, D-18, D-19; data-model E-04 to E-06, E-10).
//
// The seam is `sg::detail::runHunt` driven through `sg::detail::Runner` over
// the scripted host of `test/source/detail/hunt_fixture.hpp`, which publishes
// the catalog, the anchors and the programmable-counter limits, so a scenario
// needs no PMU, no sysfs seam, no sleep and no clock (FR-092). The refusal
// rows script their own machine fact on the provider they own, and the
// window-open counts come from `FakeProvider::readActions`, the scripted
// provider's own counter.
//
// A scenario that scripts a provider of its own runs in a re-exec of this
// binary, the way `harness_pass_collect_test.cpp` drives its scenarios,
// because the counters system takes an object path once per process.
//
// The file stands empty of cases until T017 fills the scenario list below;
// the list is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Frameworkless check()/fail() convention,
// the one `harness_pass_collect_test.cpp` carries.
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
  std::fprintf(stderr, "HARNESS HUNT COLLECT TEST FAIL: %s\n", what);
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
// `argv[1]` selects from. T017 fills the list with the scripted runs of US1.
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
  std::printf("harness_hunt_collect_test: ok\n");
  return 0;
}
