// specs/046-hunter-topdown T041 (FR-025, FR-027): the `area=` cell of a hunt
// row, printed over records this file writes. One ranked row carries the path
// of the nodes that reach its event, the second carries the file of the
// vendored table that holds its row, and the third carries the word nothing
// names, so one recorded run shows the three kinds of answer the mapper
// reaches. No top-down tree, no vendor listing and no host read stands in this
// program: the areas ride the records the way the stamp of the run leaves them.

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// The table printer of the hunt lines stands in `source/harness/report.cpp`
// below the report's own declaration and carries no header of its own, so this
// file names it the way `test/source/harness_hunt_report_test.cpp` names it.
namespace sg::detail
{
auto printHuntRanking(const hunt::EntryHunt& hunt,
                      const hunt::HuntNorm norm) -> void;
}  // namespace sg::detail

namespace
{

// The scripted device of the hunt: one core device and three events, so one
// recorded run is the golden on any host (PD-19). The two anchor counters ride
// every window and stand outside the ranked table.
constexpr std::string_view kDevice {"cpu"};
constexpr std::uint64_t kAnchor = 1'000'000;

// The rows of the scripted columns are laid down here: a collected rate
// carries the sampling count of the host (PD-19). The window fields are pass,
// stage, event value, both anchors, iterations, disclosure.
auto collectedRows() -> std::vector<sg::hunt::EventSamples>
{
  const auto steps =
      std::vector<std::pair<std::string_view, std::vector<std::uint64_t>>> {
          {"hunt_branches", {1, 1, 1}},
          {"hunt_loads", {2, 2, 2}},
          {"hunt_stores", {0, 0, 0}},
      };
  auto collected = std::vector<sg::hunt::EventSamples> {};
  for (const auto& [leaf, windows] : steps) {
    auto samples = std::vector<sg::hunt::HuntSample> {};
    for (const auto window : windows) {
      samples.push_back(
          {samples.size() + 1, 1, window * kAnchor, kAnchor, kAnchor, 1, {}});
    }
    collected.push_back({std::string {kDevice} + "/" + std::string {leaf},
                         std::move(samples),
                         {}});
  }
  return collected;
}

// sg-example:begin hunt-topdown
// The ranked table of a hunt whose records carry an area. `rankEvents` decides
// the order, the marks and the size of the table, and the loop below hands each
// record the area its address settled (FR-009, FR-013, FR-014).
auto rankHuntWithAreas(const std::vector<sg::hunt::EventSamples>& collected,
                       const std::map<std::string, std::string>& areas,
                       const sg::hunt::HuntOptions& options)
    -> sg::hunt::EntryHunt
{
  // No plan collected these scripted rows, so the record states no method.
  auto hunt = sg::hunt::rankEvents(collected, options, std::nullopt);
  for (auto& entry : hunt.entries) {
    if (const auto named = areas.find(entry.address); named != areas.end()) {
      entry.topdownArea = named->second;
    }
  }
  return hunt;
}

// sg-example:end hunt-topdown

// The three answers, one per kind. The path joins the node names from the
// level-1 root down. The file stem names the vendored file that holds the row,
// and the last word names a record nothing reaches (FR-025).
constexpr std::string_view kPathArea {
    "topdown.backend_bound > topdown.intel.tma_l1_bound"};
constexpr std::string_view kFileArea {"pipeline"};
constexpr std::string_view kUnmappedArea {"unmapped"};

// The area of each scripted address, in the order the table prints them.
auto scriptedAreas() -> std::map<std::string, std::string>
{
  const auto stem = std::string {kDevice} + "/";
  return {
      {stem + "hunt_loads", std::string {kPathArea}},
      {stem + "hunt_branches", std::string {kFileArea}},
      {stem + "hunt_stores", std::string {kUnmappedArea}},
  };
}

}  // namespace

auto bmHuntTopdown(sg::State& state) -> void
{
  const auto cells = static_cast<std::uint64_t>(state.range(0));
  std::uint64_t sum = 0;
  for (auto _ : state) {
    for (std::uint64_t cell = 0; cell < cells; ++cell) {
      sum += cell;
    }
    sg::doNotOptimize(sum);
  }
}

SG_BENCHMARK(bmHuntTopdown).arg(16);

auto holdsWord(int argc, char** argv, std::string_view prefix) -> bool
{
  for (int index = 0; index < argc; ++index) {
    if (std::string_view(argv[index]).starts_with(prefix)) {
      return true;
    }
  }
  return false;
}

// The table prints after the benchmark rows the way a run leaves it, and its
// rows come from the scripted records above, one fixed pass (PD-19). A listing
// answers the registry alone, because the run gate reads that listing for the
// registered set.
auto main(int argc, char** argv) -> int
{
  const auto status = sg::speedgunMain(argc, argv);
  if (!holdsWord(argc, argv, "--list")) {
    auto options = sg::hunt::HuntOptions {};
    options.top = 2;
    const auto hunt =
        rankHuntWithAreas(collectedRows(), scriptedAreas(), options);
    sg::detail::printHuntRanking(hunt, sg::hunt::HuntNorm::INSTR);
  }
  return status;
}
