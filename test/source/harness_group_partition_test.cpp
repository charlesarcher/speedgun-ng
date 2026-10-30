// ============================================================================
// TDD test for the partition of one instance's contexts into groups
// (specs/039-context-groups T005; FR-001 to FR-010, D-3, D-4, B-1 and B-2
// of contracts/group-partition.md, A-1 to A-6).
//
// The unit under test reads a preset, the plan `resolvePreset` wrote for
// that preset, and a context count, and answers the groups in
// first-appearance order of their slots. It starts no thread, reads no
// clock, and opens no file (A-5). The plan reaches it as an argument, so
// every shape of this file stands built in place beside the call that
// needs it, and no scenario installs the global seam, reads the host
// topology, or starts a child process (FR-036).
//
// The shapes are the four the task names, in the miniature of
// `fourGroupShape` at test/source/harness_cli_placement_test.cpp:260-297:
//   fourGroups  four L3 groups of two cores and two threads each, with no
//               memory unit in the read
//   twoNodes    the same sixteen cpus over two memory nodes, node 0
//               holding groups 0 and 1, node 1 holding groups 2 and 3
//   oneGroup    one L3 group of four cores beside one memory node
//   narrowMask  the read of one L3 group whose mask holds the second
//               node's cpus alone, the inverted twin of
//               `installTwoNodeShapeNarrowMask` at
//               test/source/harness_cli_placement_test.cpp:318
// A fifth shape states cores and no L3 unit, the read FR-006 names, and
// it carries one memory node so the `node` preset still has a unit to
// group by.
//
// The `node` rows of `twoNodes` name their groups `node-0` and `node-1`
// over a read whose L3 ordinals for the same positions read `l3-0` and
// `l3-2`, so a group source other than the plan's node list fails the
// name check (D-3, FR-008).
//
// Scenario map:
//   sixPresets         the six presets over the five shapes, at the count
//                      each preset carries: the group names and the
//                      member counts of every row that fits (FR-006 to
//                      FR-010, A-6)
//   slotSets           the positions each group holds, read position by
//                      position over five rows (FR-001, FR-002, A-1)
//   exactCover         every row covers the positions 0 to n-1 exactly
//                      once, holds at least one position per group, and
//                      names each group once (A-1, A-2)
//   firstAppearance    the answer lists its groups by the first position
//                      each holds, and two calls answer equal vectors
//                      (A-4, FR-001)
//   oneContext         every preset over every shape answers no group for
//                      a one-context instance (FR-010)
//   nonePreset         `none` answers no partition for the count its
//                      shape reads at under the other presets (FR-007,
//                      A-3)
//   noL3Read           cores that name no L3 unit answer no partition
//                      under the L3-keyed presets (FR-006)
//   nodeWithoutL3      the same read answers one `node-0` group, because
//                      `node` groups by the node the plan carries
//                      (FR-006, D-3)
//   nodeWithoutMemory  a read that states no memory unit hands the
//                      resolver no node plan, so a partition stands
//                      absent before the fit step stops the batch
//                      (FR-006, specs/025 FR-018)
//   narrowMaskNames    the mask that excludes the read's first node names
//                      the group `node-1` beside `l3-0` (D-4, FR-008)
//   planUnchanged      the answer leaves the plan's cpu, group and node
//                      lists and its place count as the resolver wrote
//                      them (FR-009, B-1)
//   groupOfContextRead the group index of every position of a two-group
//                      and of a four-group partition (B-2)
//   oneGroupAnswer     one group holds every position of the instance and
//                      stands a partition of its own (FR-002)
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "detail/group_shape_fixture.hpp"
#include "detail/internal.hpp"

namespace
{

auto fail(char const* const what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(bool const cond, char const* const what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The narrow-mask shape: four threads at cpus 4 to 7 over two cores and
// one L3 group, beside two memory nodes of which the mask holds the
// second alone. Node 0 takes no place, and the surviving node carries
// the number 1, the number a group name spells (D-4).
auto narrowMaskShape() -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 2; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = 0;
    for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = core * 2 + sibling + 4;
      thread.coreOrdinal = core;
      thread.groupOrdinal = 0;
      layoutCore.threads.push_back(core * 2 + sibling);
      installed.threads.push_back(thread);
      installed.mask.push_back(thread.cpuIndex);
    }
    installed.cores.push_back(layoutCore);
  }
  sg::detail::LayoutGroup layoutGroup;
  layoutGroup.lowestCpu = 4;
  layoutGroup.cores = {0, 1};
  installed.groups.push_back(layoutGroup);
  for (std::uint32_t node = 0; node < 2; ++node) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = node;
    for (std::uint32_t cpu = node * 4; cpu < node * 4 + 4; ++cpu) {
      layoutNode.cpus.push_back(cpu);
    }
    installed.nodes.push_back(layoutNode);
  }
  return installed;
}

// The read that states cores and no L3 unit: eight threads over four
// cores, `groups` empty, and every thread left at `kNoGroup` the way the
// read leaves it (FR-006). One memory node stands beside them, so the
// `node` preset still names a unit for the same positions.
auto noL3Shape() -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 4; ++core) {
    sg::detail::LayoutCore layoutCore;
    for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = core * 2 + sibling;
      thread.coreOrdinal = core;
      layoutCore.threads.push_back(core * 2 + sibling);
      installed.threads.push_back(thread);
      installed.mask.push_back(thread.cpuIndex);
    }
    installed.cores.push_back(layoutCore);
  }
  sg::detail::LayoutNode layoutNode;
  layoutNode.number = 0;
  for (std::uint32_t cpu = 0; cpu < 8; ++cpu) {
    layoutNode.cpus.push_back(cpu);
  }
  installed.nodes.push_back(layoutNode);
  return installed;
}

auto shapeOf(std::string_view const name) -> sg::detail::MachineLayout
{
  if (name == "fourGroups") {
    return sg::test::machineShape(4, 2, 2, 0);
  }
  if (name == "twoNodes") {
    return sg::test::machineShape(4, 2, 2, 2);
  }
  if (name == "oneGroup") {
    return sg::test::machineShape(1, 4, 2, 1);
  }
  if (name == "narrowMask") {
    return narrowMaskShape();
  }
  return noL3Shape();
}

// The plan of one row, read the way the run reads it: the resolver for
// the row's preset over the row's shape at the row's count.
auto planOf(std::string_view const shape,
            sg::detail::Preset const preset,
            std::size_t const contexts) -> sg::detail::PlacementPlan
{
  return sg::detail::resolvePreset(
      preset, shapeOf(shape), static_cast<int>(contexts));
}

auto presetName(sg::detail::Preset const preset) -> std::string
{
  return std::string(sg::detail::presetText(preset));
}

// One row of the table: a shape, a preset, a context count, and the
// partition the pair answers for it. An empty name ends the expected
// list, and `members` holds the count of positions each named group
// holds, in the same order.
struct Row
{
  char const* shape;
  sg::detail::Preset preset;
  std::size_t contexts;
  std::array<char const*, 5> names;
  std::array<std::size_t, 4> members;
};

// The expected answer ends where its name list ends, so the row of a
// run that carries no partition names no group at all.
auto expectedGroups(Row const& row) -> std::size_t
{
  std::size_t count = 0;
  while (count < row.names.size() && row.names[count][0] != '\0') {
    ++count;
  }
  return count;
}

// The five shapes under the six presets, each at a count its preset
// carries for the shape (FR-006 to FR-010). The `l3` rows of `oneGroup`
// and `narrowMask` read one context, where FR-010 answers no group, and
// the `core` and `smt` rows of `noL3` read the unit that stands
// `kNoGroup` (FR-006).
auto table() -> std::vector<Row>
{
  using sg::detail::Preset;
  return {
      Row {"fourGroups", Preset::NONE, 8, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"fourGroups",
           Preset::CORE,
           8,
           {"l3-0", "l3-1", "l3-2", "l3-3", ""},
           {2, 2, 2, 2}},
      Row {"fourGroups",
           Preset::L3,
           4,
           {"l3-0", "l3-1", "l3-2", "l3-3", ""},
           {1, 1, 1, 1}},
      Row {
          "fourGroups", Preset::SMT, 2, {"l3-0", "", "", "", ""}, {2, 0, 0, 0}},
      Row {"fourGroups",
           Preset::COMPACT,
           16,
           {"l3-0", "l3-1", "l3-2", "l3-3", ""},
           {4, 4, 4, 4}},
      Row {"twoNodes", Preset::NONE, 8, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"twoNodes",
           Preset::CORE,
           8,
           {"l3-0", "l3-1", "l3-2", "l3-3", ""},
           {2, 2, 2, 2}},
      Row {"twoNodes",
           Preset::L3,
           4,
           {"l3-0", "l3-1", "l3-2", "l3-3", ""},
           {1, 1, 1, 1}},
      Row {"twoNodes", Preset::SMT, 2, {"l3-0", "", "", "", ""}, {2, 0, 0, 0}},
      Row {"twoNodes",
           Preset::COMPACT,
           16,
           {"l3-0", "l3-1", "l3-2", "l3-3", ""},
           {4, 4, 4, 4}},
      Row {"twoNodes",
           Preset::NODE,
           8,
           {"node-0", "node-1", "", "", ""},
           {4, 4, 0, 0}},
      Row {"twoNodes",
           Preset::NODE,
           16,
           {"node-0", "node-1", "", "", ""},
           {8, 8, 0, 0}},
      Row {"oneGroup", Preset::NONE, 4, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"oneGroup", Preset::CORE, 4, {"l3-0", "", "", "", ""}, {4, 0, 0, 0}},
      Row {"oneGroup", Preset::L3, 1, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"oneGroup", Preset::SMT, 2, {"l3-0", "", "", "", ""}, {2, 0, 0, 0}},
      Row {"oneGroup",
           Preset::COMPACT,
           8,
           {"l3-0", "", "", "", ""},
           {8, 0, 0, 0}},
      Row {"oneGroup",
           Preset::NODE,
           8,
           {"node-0", "", "", "", ""},
           {8, 0, 0, 0}},
      Row {"narrowMask", Preset::NONE, 4, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"narrowMask",
           Preset::CORE,
           2,
           {"l3-0", "", "", "", ""},
           {2, 0, 0, 0}},
      Row {"narrowMask", Preset::L3, 1, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {
          "narrowMask", Preset::SMT, 2, {"l3-0", "", "", "", ""}, {2, 0, 0, 0}},
      Row {"narrowMask",
           Preset::COMPACT,
           4,
           {"l3-0", "", "", "", ""},
           {4, 0, 0, 0}},
      Row {"narrowMask",
           Preset::NODE,
           4,
           {"node-1", "", "", "", ""},
           {4, 0, 0, 0}},
      Row {"noL3", Preset::CORE, 4, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"noL3", Preset::SMT, 2, {"", "", "", "", ""}, {0, 0, 0, 0}},
      Row {"noL3", Preset::NODE, 8, {"node-0", "", "", "", ""}, {8, 0, 0, 0}},
  };
}

// A-4: one partition, read twice, answers the same groups in the same
// order. The fields carry the answer, so the fields state the equality.
auto equalPartitions(const std::vector<sg::detail::ContextGroup>& first,
                     const std::vector<sg::detail::ContextGroup>& second)
    -> bool
{
  if (first.size() != second.size()) {
    return false;
  }
  for (std::size_t group = 0; group < first.size(); ++group) {
    if (first[group].name != second[group].name
        || first[group].slots != second[group].slots)
    {
      return false;
    }
  }
  return true;
}

// B-1 states the plan of a partition read as the fitted one, the empty
// plan of `none` excepted: the fit step stops a batch a preset cannot
// carry before any partition read follows it, so a row that plans fewer
// cpus than it asks names a scenario no run reaches.
auto fitted(sg::detail::Preset const preset,
            sg::detail::PlacementPlan const& plan,
            std::size_t const contexts) -> bool
{
  return preset == sg::detail::Preset::NONE || plan.cpu.size() >= contexts;
}

// FR-006 to FR-010, A-6: the group names and the member counts of every
// row of the table.
auto sixPresets() -> void
{
  for (const auto& row : table()) {
    const auto plan = planOf(row.shape, row.preset, row.contexts);
    check(fitted(row.preset, plan, row.contexts),
          "a table row planned fewer cpus than it asked for");
    const auto partition =
        sg::detail::groupPartition(row.preset, plan, row.contexts);
    const auto want = expectedGroups(row);
    if (partition.size() != want) {
      std::fprintf(stderr,
                   "row %s/%s answered %zu groups, expected %zu\n",
                   row.shape,
                   presetName(row.preset).c_str(),
                   partition.size(),
                   want);
      fail("a table row answered the wrong group count");
    }
    for (std::size_t group = 0; group < want; ++group) {
      if (partition[group].name != row.names[group]) {
        std::fprintf(stderr,
                     "row %s/%s named group %zu %s, expected %s\n",
                     row.shape,
                     presetName(row.preset).c_str(),
                     group,
                     partition[group].name.c_str(),
                     row.names[group]);
        fail("a table row spelled a group name wrong");
      }
      check(partition[group].slots.size() == row.members[group],
            "a table row sized one of its groups wrong");
    }
  }
}

// FR-001, FR-002, A-1: the positions each group holds over five rows,
// read position by position.
auto slotSets() -> void
{
  using sg::detail::Preset;

  struct Case
  {
    char const* shape;
    Preset preset;
    std::size_t contexts;
    std::vector<std::vector<std::size_t>> slots;
  };

  const std::array<Case, 5> cases {
      Case {"fourGroups", Preset::CORE, 8, {{0, 1}, {2, 3}, {4, 5}, {6, 7}}},
      Case {"fourGroups", Preset::L3, 4, {{0}, {1}, {2}, {3}}},
      Case {"fourGroups",
            Preset::COMPACT,
            16,
            {{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}}},
      Case {"twoNodes", Preset::NODE, 8, {{0, 2, 4, 6}, {1, 3, 5, 7}}},
      Case {"narrowMask", Preset::NODE, 4, {{0, 1, 2, 3}}},
  };
  for (const auto& entry : cases) {
    const auto plan = planOf(entry.shape, entry.preset, entry.contexts);
    const auto partition =
        sg::detail::groupPartition(entry.preset, plan, entry.contexts);
    if (partition.size() != entry.slots.size()) {
      std::fprintf(stderr,
                   "%s/%s answered %zu groups, expected %zu\n",
                   entry.shape,
                   presetName(entry.preset).c_str(),
                   partition.size(),
                   entry.slots.size());
      fail("a slot case answered the wrong group count");
    }
    for (std::size_t group = 0; group < partition.size(); ++group) {
      if (partition[group].slots != entry.slots[group]) {
        std::fprintf(stderr,
                     "%s/%s group %zu holds %zu positions\n",
                     entry.shape,
                     presetName(entry.preset).c_str(),
                     group,
                     partition[group].slots.size());
        fail("a slot case placed a position in the wrong group");
      }
    }
  }
}

// A-1, A-2: every row of the table covers the positions of its instance
// exactly once, holds at least one position per group, and names each
// group once.
auto exactCover() -> void
{
  for (const auto& row : table()) {
    const auto plan = planOf(row.shape, row.preset, row.contexts);
    const auto partition =
        sg::detail::groupPartition(row.preset, plan, row.contexts);
    std::vector<std::size_t> seen;
    for (const auto& group : partition) {
      check(!group.slots.empty(), "a group of a partition holds no position");
      seen.insert(seen.end(), group.slots.begin(), group.slots.end());
    }
    for (std::size_t first = 0; first < partition.size(); ++first) {
      for (std::size_t second = first + 1; second < partition.size(); ++second)
      {
        check(partition[first].name != partition[second].name,
              "two groups of one partition share a name");
      }
    }
    std::sort(seen.begin(), seen.end());
    std::vector<std::size_t> all(row.contexts);
    for (std::size_t position = 0; position < row.contexts; ++position) {
      all[position] = position;
    }
    const auto want = expectedGroups(row);
    if (want == 0) {
      check(seen.empty(), "a run that names no partition named a position");
      continue;
    }
    if (seen != all) {
      std::fprintf(stderr,
                   "%s/%s covered %zu positions of %zu\n",
                   row.shape,
                   presetName(row.preset).c_str(),
                   seen.size(),
                   row.contexts);
      fail("a partition covered its instance more or less than once");
    }
  }
}

// A-4, FR-001: the answer lists its groups by the first position each
// holds, and one plan and one count answer one partition twice.
auto firstAppearance() -> void
{
  for (const auto& row : table()) {
    const auto plan = planOf(row.shape, row.preset, row.contexts);
    const auto partition =
        sg::detail::groupPartition(row.preset, plan, row.contexts);
    for (std::size_t group = 1; group < partition.size(); ++group) {
      check(partition[group - 1].slots.front() < partition[group].slots.front(),
            "a partition listed its groups out of first-appearance order");
    }
    const auto again =
        sg::detail::groupPartition(row.preset, plan, row.contexts);
    check(equalPartitions(again, partition),
          "one plan and one count answered two partitions");
  }
}

// FR-010: above one context a partition stands absent, whatever the
// preset and the shape.
auto oneContext() -> void
{
  const std::array<char const*, 5> shapes {
      "fourGroups", "twoNodes", "oneGroup", "narrowMask", "noL3"};
  const std::array<sg::detail::Preset, 6> presets {sg::detail::Preset::NONE,
                                                   sg::detail::Preset::CORE,
                                                   sg::detail::Preset::L3,
                                                   sg::detail::Preset::SMT,
                                                   sg::detail::Preset::COMPACT,
                                                   sg::detail::Preset::NODE};
  for (const auto* const shape : shapes) {
    for (const auto preset : presets) {
      const auto plan = planOf(shape, preset, 1);
      if (!fitted(preset, plan, 1)) {
        continue;
      }
      const auto partition = sg::detail::groupPartition(preset, plan, 1);
      if (!partition.empty()) {
        std::fprintf(stderr,
                     "%s/%s named %zu groups for one context\n",
                     shape,
                     presetName(preset).c_str(),
                     partition.size());
        fail("a one-context instance carries no group (FR-010)");
      }
    }
  }
}

// FR-007, A-3: `none` names no partition, the shape of the run with no
// preset stated and no layout stated.
auto nonePreset() -> void
{
  for (const auto* const shape :
       {"fourGroups", "twoNodes", "oneGroup", "narrowMask", "noL3"})
  {
    const auto plan = planOf(shape, sg::detail::Preset::NONE, 2);
    const auto partition =
        sg::detail::groupPartition(sg::detail::Preset::NONE, plan, 2);
    if (!partition.empty()) {
      std::fprintf(stderr,
                   "%s: the none preset named %zu groups\n",
                   shape,
                   partition.size());
      fail("the none preset names no partition (FR-007)");
    }
  }
}

// FR-006: a read that states cores and no L3 unit leaves the L3-keyed
// presets naming no partition, the run keeping the ungrouped shape.
auto noL3Read() -> void
{
  struct Read
  {
    sg::detail::Preset preset;
    std::size_t contexts;
  };

  const std::array<Read, 2> reads {Read {sg::detail::Preset::CORE, 4},
                                   Read {sg::detail::Preset::SMT, 2}};
  const auto layout = shapeOf("noL3");
  for (const auto& read : reads) {
    const auto preset = read.preset;
    const auto contexts = read.contexts;
    const auto plan =
        sg::detail::resolvePreset(preset, layout, static_cast<int>(contexts));
    check(plan.cpu.size() == contexts,
          "the no-L3 read planned no cpu for its preset");
    for (const auto unit : plan.group) {
      check(unit == sg::detail::kNoGroup,
            "a read with no L3 unit left a group ordinal in the plan");
    }
    const auto partition = sg::detail::groupPartition(preset, plan, contexts);
    check(partition.empty(), "a preset grouped a read that names no L3 unit");
  }
}

// FR-006, D-3: the same read groups by the node the plan carries, so the
// absent L3 unit leaves the `node` preset's partition standing.
auto nodeWithoutL3() -> void
{
  const auto plan = planOf("noL3", sg::detail::Preset::NODE, 8);
  check(plan.node.size() == 8, "the node plan of the no-L3 read named no node");
  for (const auto unit : plan.group) {
    check(unit == sg::detail::kNoGroup,
          "the no-L3 read carried an L3 ordinal into its node plan");
  }
  const auto partition =
      sg::detail::groupPartition(sg::detail::Preset::NODE, plan, 8);
  check(partition.size() == 1, "the node preset grouped the no-L3 read twice");
  check(partition[0].name == "node-0",
        "the no-L3 node group spelled its name wrong");
  check(partition[0].slots.size() == 8, "the no-L3 node group lost a position");
}

// FR-006, specs/025 FR-018: a read that states no memory unit hands the
// resolver no node plan at all, so the fit step stops the batch ahead of
// any partition read.
auto nodeWithoutMemory() -> void
{
  const auto plan = planOf("fourGroups", sg::detail::Preset::NODE, 4);
  check(plan.cpu.empty(), "a read with no memory unit answered a node plan");
  check(plan.node.empty(), "a node plan named a node the read does not state");
  check(plan.places == 0, "the place count of the nodeless plan named places");
}

// D-4, FR-008: beside a mask that excludes the read's first node, the
// group name spells the number the plan carries.
auto narrowMaskNames() -> void
{
  const auto plan = planOf("narrowMask", sg::detail::Preset::NODE, 4);
  for (const auto cpu : plan.cpu) {
    check(cpu >= 4, "the node plan placed a context on an excluded cpu");
  }
  for (const auto number : plan.node) {
    check(number == 1, "the node plan carried a node other than the number 1");
  }
  const auto partition =
      sg::detail::groupPartition(sg::detail::Preset::NODE, plan, 4);
  check(partition.size() == 1,
        "the narrow-mask node partition named two groups");
  check(partition[0].name == "node-1",
        "the narrow-mask group spelled its name wrong");

  const auto core = planOf("narrowMask", sg::detail::Preset::CORE, 2);
  const auto grouped =
      sg::detail::groupPartition(sg::detail::Preset::CORE, core, 2);
  check(grouped.size() == 1, "the narrow-mask core partition named two groups");
  check(grouped[0].name == "l3-0",
        "the narrow-mask L3 group spelled its name wrong");
}

// FR-009, B-1: the answer changes nothing in the plan the resolver wrote.
auto planUnchanged() -> void
{
  for (const auto& row : table()) {
    const auto plan = planOf(row.shape, row.preset, row.contexts);
    const sg::detail::PlacementPlan before = plan;
    [[maybe_unused]] const auto partition =
        sg::detail::groupPartition(row.preset, plan, row.contexts);
    if (plan.cpu != before.cpu || plan.group != before.group
        || plan.node != before.node || plan.places != before.places)
    {
      std::fprintf(stderr,
                   "%s/%s: the partition read moved the plan\n",
                   row.shape,
                   presetName(row.preset).c_str());
      fail("the partition read changed the plan (FR-009)");
    }
  }
}

// B-2: the index of the group that holds each position, over a
// two-group partition and a four-group partition.
auto groupOfContextRead() -> void
{
  struct Case
  {
    char const* shape;
    sg::detail::Preset preset;
    std::size_t contexts;
    std::vector<std::size_t> groups;
  };

  const std::array<Case, 2> cases {
      Case {"twoNodes", sg::detail::Preset::NODE, 8, {0, 1, 0, 1, 0, 1, 0, 1}},
      Case {"fourGroups",
            sg::detail::Preset::COMPACT,
            16,
            {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3}},
  };
  for (const auto& entry : cases) {
    const auto plan = planOf(entry.shape, entry.preset, entry.contexts);
    const auto partition =
        sg::detail::groupPartition(entry.preset, plan, entry.contexts);
    for (std::size_t position = 0; position < entry.contexts; ++position) {
      const auto group = sg::detail::groupOfContext(partition, position);
      if (group != entry.groups[position]) {
        std::fprintf(stderr,
                     "%s/%s put position %zu in group %zu\n",
                     entry.shape,
                     presetName(entry.preset).c_str(),
                     position,
                     group);
        fail("groupOfContext named the wrong group for a position");
      }
      const auto& slots = partition[group].slots;
      check(std::find(slots.begin(), slots.end(), position) != slots.end(),
            "groupOfContext named a group that does not hold the position");
    }
  }
}

// FR-002: one group over the whole instance stands a partition of its
// own, named for the unit it groups.
auto oneGroupAnswer() -> void
{
  const auto core = planOf("oneGroup", sg::detail::Preset::CORE, 4);
  const auto byGroup =
      sg::detail::groupPartition(sg::detail::Preset::CORE, core, 4);
  check(byGroup.size() == 1, "the one-group L3 partition named two groups");
  check(byGroup[0].name == "l3-0",
        "the one-group L3 partition spelled its name wrong");
  check(byGroup[0].slots.size() == 4,
        "the one-group L3 partition lost a position");

  const auto node = planOf("oneGroup", sg::detail::Preset::NODE, 8);
  const auto byNode =
      sg::detail::groupPartition(sg::detail::Preset::NODE, node, 8);
  check(byNode.size() == 1, "the one-group node partition named two groups");
  check(byNode[0].name == "node-0",
        "the one-group node partition spelled its name wrong");
  check(byNode[0].slots.size() == 8,
        "the one-group node partition lost a position");
}

}  // namespace

auto main() -> int
{
  sixPresets();
  slotSets();
  exactCover();
  firstAppearance();
  oneContext();
  nonePreset();
  noL3Read();
  nodeWithoutL3();
  nodeWithoutMemory();
  narrowMaskNames();
  planUnchanged();
  groupOfContextRead();
  oneGroupAnswer();

  std::printf("harness_group_partition_test PASS\n");
  return 0;
}
