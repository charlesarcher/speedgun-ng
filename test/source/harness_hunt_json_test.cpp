// ============================================================================
// The hunt document: writer, reader and schema conformance
// (specs/029-hunter-core T006, filled by T038; US3; FR-051, FR-052; AC-6,
// SC-006; research PD-20; contracts/hunt-json.md,
// contracts/report-schema-key.md).
//
// The seams are the writer, `loadReport` and the schema checker over the
// `SG_SCHEMA_PATH` document T007 compiles into this target: a case writes a
// record, reads the bytes back and compares the two field by field, or plants
// one disagreement and names the pointer the refusal reports. Every key and
// every `x-order` position is the contract table's, never a key mirrored here.
//
// The file stands empty of cases until T038 fills the scenario list below; the
// list is what CTest runs, so an entry absent from it never runs, and `argv[1]`
// selects one entry of it. Frameworkless check()/fail() convention, the one
// `harness_json_report_test.cpp` carries.
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
  std::fprintf(stderr, "HARNESS HUNT JSON TEST FAIL: %s\n", what);
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
// `argv[1]` selects from. T038 fills the list with the round trip, the refusal
// codes and the schema conformance of US3.
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
  std::printf("harness_hunt_json_test: ok\n");
  return 0;
}
