// ============================================================================
// The hunt command-line refusals and accepted forms
// (specs/029-hunter-core T006, filled by T018; US1; FR-001, FR-006, FR-081;
// AC-5, SC-05; research D-1, D-16; contracts/hunt-cli.md).
//
// The seam is `speedgunMain`, the entry point that
// `test/source/harness_cli_test.cpp` drives: one case per refusal row of the
// contract asserts one stderr line naming the value and status 2, in process
// where the refusal path returns and through a forked child where it calls
// `std::exit` (the pattern of `harness_cli_test.cpp:800`, `:1166` and
// `:1417`).
//
// The file stands empty of cases until T018 fills the scenario list below;
// the list is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Frameworkless check()/fail() convention,
// the one `harness_cli_test.cpp` carries.
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
  std::fprintf(stderr, "HARNESS HUNT CLI TEST FAIL: %s\n", what);
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
// `argv[1]` selects from. T018 fills the list with the refusal rows and the
// accepted forms of US1.
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
  std::printf("harness_hunt_cli_test: ok\n");
  return 0;
}
