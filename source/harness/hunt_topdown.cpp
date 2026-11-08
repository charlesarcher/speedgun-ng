// specs/046-hunter-topdown T003 (FR-001 to FR-009; D-1, D-7, D-16, D-17;
// contracts/area-mapper.md): the top-down area mapper of the hunter.
//
// The four declarations of the pair stand in `detail/hunt_topdown.hpp`, and the
// one seam entry the run reads stands in `detail/internal.hpp`. Each body joins
// this file with the unit that reads it, so no arm of the mapper stands in the
// tree ahead of the case that names its answer.

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "detail/hunt_topdown.hpp"

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_topdown.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// The separator the path joins its names with, in the spelling `N10` publishes
// (FR-007, D-10).
constexpr std::string_view kPathSeparator {" > "};

// The word one address answers where nothing names it: no node of the tree, and
// no row of the grouping (FR-005, D-3).
constexpr std::string_view kUnmappedText {"unmapped"};

// Whether one node's event list holds `address` as an equal text. The node
// names an event with the spelling its own list carries, and no other spelling
// of that address reaches it (FR-002, D-1).
[[nodiscard]] auto namesAddress(const std::vector<std::string>& events,
                                const std::string_view address) -> bool
{
  return std::ranges::any_of(events,
                             [address](const std::string& event) -> bool
                             { return std::string_view {event} == address; });
}

// The index of the node whose name reads `name`, among the nodes that stand
// ahead of `stop`. A parent names a row the table published ahead of its child
// (specs/042-topdown FR-013), so the walk reads the prefix alone and a row that
// stands at or after its own child names no parent.
[[nodiscard]] auto parentNodeIndex(const sg::counters::TopdownTree& tree,
                                   std::string_view name,
                                   std::size_t stop) -> std::size_t
{
  for (std::size_t index = 0; index < stop; ++index) {
    if (std::string_view {tree.nodes.at(index).name} == name) {
      return index;
    }
  }
  return tree.nodes.size();
}

// The bare name of one hunt address: the text the device prefix stands ahead
// of. A vendored row carries a bare name and no device, so the grouping reads
// the name with the prefix dropped and the node arm reads the whole address
// (D-16, FR-037).
[[nodiscard]] auto barePart(const std::string_view address) -> std::string_view
{
  const auto cut = address.find('/');
  return cut == std::string_view::npos ? address : address.substr(cut + 1);
}

// The grouping index of one read: the bare name of every row of a listing,
// mapped to the stem of the file the vendor holds that row in. The listing of
// D-16 already answers one pair per event name, so the build holds no dedup
// rule and no second first-row branch of its own (D-16, FR-004, FR-037).
using GroupIndex = std::map<std::string_view, std::string_view>;

[[nodiscard]] auto groupIndexOf(
    const std::vector<sg::counters::TopdownEventGroup>& groups) -> GroupIndex
{
  auto index = GroupIndex {};
  for (const auto& row : groups) {
    index.insert_or_assign(std::string_view {row.event},
                           std::string_view {row.group});
  }
  SG_ENSURE(index.size() == groups.size(),
            "one key stands per event name of the listing, so a name the "
            "accessor held twice folded no row here (D-16, FR-037)");
  return index;
}

// The four arms of one address, over the index the caller built. The arms
// answer in the order the contract fixes them, and the last of them answers the
// word no node and no row names (FR-001 to FR-005).
[[nodiscard]] auto areaOfIndex(
    const std::string_view address,
    const std::optional<sg::counters::TopdownTree>& tree,
    const GroupIndex& groups) -> std::string
{
  auto answer = std::string {};
  // Arms 1 and 4 hold one body on purpose. They answer `unmapped` for two
  // different reasons: the first answers ahead of the listing because no tree
  // stands, and the last answers after it because no node and no row names the
  // address (FR-005, D-3).
  // NOLINTNEXTLINE(bugprone-branch-clone) Arms 1 and 4 answer one word by rule.
  if (!tree.has_value()) {
    // Arm 1: no tree stands, so no node names any address of the run and the
    // index plays no part (FR-005, D-5).
    answer = std::string {kUnmappedText};
  } else if (const auto node = topdownDeepestNode(*tree, address);
             node.has_value())
  {
    // Arm 2: the deepest node that names the address settles the path
    // (FR-001, FR-002, FR-003, FR-007).
    answer = topdownPathOf(*tree, *node);
  } else if (const auto row = groups.find(barePart(address));
             row != groups.end())
  {
    // Arm 3: the grouping answers the bare name of the address with the stem of
    // the vendored file that holds its row, verbatim (FR-004, D-3).
    answer = std::string {row->second};
  } else {
    // Arm 4: no node of the tree and no row of the listing names the address,
    // so the answer reads the closing word (FR-001, D-3).
    answer = std::string {kUnmappedText};
  }
  SG_ENSURE(!answer.empty(),
            "the answer stands never empty, so every address of a run carries "
            "an area, the unmapped one included (FR-001, FR-014)");
  return answer;
}

}  // namespace

// specs/046 FR-001, FR-003, D-4: the answer the assertion of
// `topdownDeepestNode` reads over the whole list. The declaration stands in
// detail/hunt_topdown.hpp, where the hunt report test drives every arm of the
// answer. A closure written inside an `SG_*` predicate gains a coverage record
// whose only line the standing extract filter of `cmake/coverage.cmake`
// removes, and lcov refuses a trace that holds a hit function carrying no hit
// line.
[[nodiscard]] auto nodesNamingAddressStandAtOrBelow(
    const sg::counters::TopdownTree& tree,
    const std::size_t answer,
    const std::string_view address) -> bool
{
  return std::ranges::all_of(tree.nodes,
                             [&](const sg::counters::TopdownNode& other) -> bool
                             {
                               return !namesAddress(other.events, address)
                                   || other.level
                                   <= tree.nodes.at(answer).level;
                             });
}

auto topdownDeepestNode(const sg::counters::TopdownTree& tree,
                        const std::string_view address)
    -> std::optional<std::size_t>
{
  std::optional<std::size_t> answer;
  for (std::size_t index = 0; index < tree.nodes.size(); ++index) {
    const auto& node = tree.nodes.at(index);
    if (!namesAddress(node.events, address)) {
      continue;
    }
    // A node of one depth that stands ahead keeps the answer, because the lower
    // index is the row that came first in the table (FR-003, D-4).
    if (!answer.has_value() || node.level > tree.nodes.at(*answer).level) {
      answer = index;
    }
  }
  SG_ASSERT(!answer.has_value()
                || (*answer < tree.nodes.size()
                    && nodesNamingAddressStandAtOrBelow(tree, *answer, address)),
            "an index the answer holds names one node of the list it was "
            "given, and its level stands at or above every node that names the "
            "address (FR-001, FR-003, D-4)");
  return answer;
}

auto topdownPathOf(const sg::counters::TopdownTree& tree,
                   const std::size_t index) -> std::string
{
  // The walk starts at the node it was given and climbs the parent links, so
  // the names arrive child first and the path joins them back root first.
  std::vector<std::string_view> names;
  auto cursor = index;
  while (true) {
    const auto& node = tree.nodes.at(cursor);
    names.push_back(node.name);
    if (node.parent.empty()) {
      break;
    }
    const auto parent = parentNodeIndex(tree, node.parent, cursor);
    SG_ASSERT(parent < tree.nodes.size(),
              "the parent text of a node names a row the table holds ahead of "
              "that node, so the walk reaches one name at every step (specs/"
              "042-topdown FR-013)");
    cursor = parent;
  }
  auto path = std::string {};
  for (const auto& name : std::views::reverse(names)) {
    if (!path.empty()) {
      path += kPathSeparator;
    }
    path += name;
  }
  SG_ENSURE(path.starts_with(names.back()) && path.ends_with(names.front())
                && !path.empty(),
            "the text starts with the name of the level-1 node the parent "
            "links reach and ends with the name of the node it was given, the "
            "names joined by the separator (FR-007, D-10)");
  return path;
}

auto topdownArea(const std::string_view address,
                 const std::optional<sg::counters::TopdownTree>& tree,
                 const std::vector<sg::counters::TopdownEventGroup>& groups)
    -> std::string
{
  // Arm 1 answers ahead of the grouping, so a host the selection refused builds
  // no index and reads no listing at all (FR-005, FR-006, D-16).
  if (!tree.has_value()) {
    return areaOfIndex(address, tree, GroupIndex {});
  }
  return areaOfIndex(address, tree, groupIndexOf(groups));
}

auto stampTopdownAreas(
    std::vector<sg::BenchmarkResult>& results,
    const std::optional<sg::counters::TopdownTree>& tree,
    const std::vector<sg::counters::TopdownEventGroup>& groups) -> void
{
  // The listing indexes once per run, because every record of the run reads one
  // grouping and a rebuild per record would read the same rows again (FR-009,
  // D-16). With no tree the grouping plays no part, so no index builds at all
  // (FR-005).
  const auto index = tree.has_value() ? groupIndexOf(groups) : GroupIndex {};
  for (auto& result : results) {
    if (!result.hunt.has_value()) {
      // An entry that carries no hunt record holds no ranked record to settle,
      // and the stamp of one run adds no record to it (FR-010).
      continue;
    }
    for (auto& record : result.hunt->entries) {
      // The address is the only field of the record this loop reads and the
      // area the only field it writes, so the rank, the marks, the figures and
      // the order of the list stand as they stood (FR-009, FR-010).
      record.topdownArea = areaOfIndex(record.address, tree, index);
    }
  }
}

auto topdownAreaSelection(const RunOptions& options)
    -> sg::counters::TopdownSelection
{
  auto ask = sg::counters::TopdownSelection {};
  ask.identity = options.topdownIdentity.has_value()
      ? *options.topdownIdentity
      : sg::counters::cpuIdentity();
  ask.level = kTopdownAreaLevel;
  ask.table = options.topdownTable;
  SG_ENSURE(ask.level == kTopdownAreaLevel
                && ask.table.has_value() == options.topdownTable.has_value()
                && (!options.topdownIdentity.has_value()
                    || (ask.identity.family == options.topdownIdentity->family
                        && ask.identity.model
                               == options.topdownIdentity->model)),
            "the ask carries the level of the pair, the planted table, and the "
            "identity the run planted or the cpu of this process answers "
            "(FR-001, FR-033, FR-034)");
  return ask;
}

auto topdownCompanionOptions(const RunOptions& options) -> RunOptions
{
  // The identity and the table of the ask resolve through the gatherer of the
  // pair, so the companion reads the one identity and the one table the area
  // ask of the same run read. The level is the only field this body sets apart
  // (FR-033, FR-034, D-17).
  const auto pair = topdownAreaSelection(options);
  auto ask = RunOptions {};
  ask.topdownIdentity = pair.identity;
  ask.topdownTable = pair.table;
  ask.topdownLevel = kTopdownCompanionLevel;
  ask.topdownCompanion = true;
  // One repetition of one fixed iteration, the shape of the stage run at
  // `source/harness/hunt.cpp:420-427`: the companion prices one short pass and
  // runs no calibration of its own (D-9).
  ask.repetitions = 1;
  ask.fixedIterations = 1;
  // A dry run leaves the companion a dry run, so the figures of a dry run stay
  // the one-iteration figures every pass of such a run answers (US3 AC-3).
  ask.dryRun = options.dryRun;
  SG_ENSURE(ask.topdownLevel == kTopdownCompanionLevel && ask.topdownCompanion
                && ask.repetitions == 1 && ask.fixedIterations == 1
                && ask.dryRun == options.dryRun && !ask.hunt.has_value()
                && ask.counterAddresses.empty() && ask.metricNames.empty()
                && !ask.metricMethod.has_value()
                && ask.topdownTable.has_value()
                       == options.topdownTable.has_value(),
            "the companion asks one level-1 pass of one identity and one table, "
            "and carries no hunt, no address and no metric method of the run "
            "(FR-033, FR-035, D-9)");
  return ask;
}

auto topdownCompanionRecord(const RunOptions& options, Instance& instance)
    -> std::optional<TopdownCompanion>
{
  if (!options.topdownCompanion) {
    // A run that asks no companion collects nothing: no ask, no pass and no
    // record, the bytes of the run before this feature (FR-035).
    return std::nullopt;
  }
  auto ask = topdownAreaSelection(options);
  ask.level = static_cast<int>(kTopdownCompanionLevel);
  if (!sg::counters::topdownTree(ask).has_value()) {
    // A host the ask refuses runs no pass. The one line of the refusal stands
    // on the command line, where the widened guard prints it, and no block
    // prints (FR-017, FR-005).
    return std::nullopt;
  }
  // The pass runs over a fresh option set with no hunt, so the instance reaches
  // the runner with `huntPlan` empty and the handoff of the retried and refused
  // lists records nothing of this pass (FR-036).
  const sg::BenchmarkResult result =
      Runner {topdownCompanionOptions(options)}.run(instance);
  auto answer = TopdownCompanion {};
  answer.passCount = result.passCount;
  // A pass that reached no strip leaves a record that states no support, and
  // the block printer of report.cpp prints no line for such a record (FR-017).
  answer.record = result.topdown.value_or(sg::TopdownRecord {});
  SG_ENSURE(!answer.record.supported
                || answer.record.level <= kTopdownCompanionLevel,
            "the carrier carries the record of the level-1 ask of the "
            "companion, or a record that states no support (FR-033, FR-017)");
  return answer;
}

}  // namespace sg::detail
