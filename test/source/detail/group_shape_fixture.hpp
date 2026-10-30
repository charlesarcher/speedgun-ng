#ifndef SG_TEST_DETAIL_GROUP_SHAPE_FIXTURE_HPP
#define SG_TEST_DETAIL_GROUP_SHAPE_FIXTURE_HPP

#include <cstdint>

#include "detail/internal.hpp"

// ============================================================================
// The machine shape the context-group tests share (specs/039-context-groups
// T006, T034; FR-036).
//
// `harness_group_partition_test.cpp` reads the partition a preset names over
// this shape and `harness_group_run_test.cpp` runs an instance over it, so the
// two halves need one machine: the same groups of the same cores answer the
// same `l3-<unit>` names in the partition read and in the run's disclosure, and
// a builder held twice drifts. The builder is the miniature of
// `fourGroupShape` at test/source/harness_cli_placement_test.cpp:260-297, with
// its group count, cores per group, threads per core and memory nodes left as
// the arguments the scenarios read (FR-036).
//
// The cpus stand ascending within the group order, and the memory nodes split
// the cpu range in equal slices. The nodes stand unfiltered, the way the read
// states them, so the mask filters them at plan time (specs/035
// data-model.md). Nothing here reads `/sys`, the host topology or a clock.
// ============================================================================

namespace sg::test
{

// The shape of `groups` L3 groups with `coresPerGroup` cores of `siblings`
// threads each, beside `memoryNodes` memory nodes. A count of zero answers the
// shape of a read that states no memory node, which the L3 presets group over
// and the `node` preset leaves ungrouped (FR-006, FR-007).
inline auto machineShape(const std::uint32_t groups,
                         const std::uint32_t coresPerGroup,
                         const std::uint32_t siblings,
                         const std::uint32_t memoryNodes)
    -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = memoryNodes > 0;
  installed.packages = 1;
  const std::uint32_t perGroup = coresPerGroup * siblings;
  for (std::uint32_t group = 0; group < groups; ++group) {
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = group;
      for (std::uint32_t sibling = 0; sibling < siblings; ++sibling) {
        sg::detail::LayoutThread thread;
        thread.cpuIndex = group * perGroup + core * siblings + sibling;
        thread.coreOrdinal = static_cast<std::uint32_t>(installed.cores.size());
        thread.groupOrdinal = group;
        layoutCore.threads.push_back(
            static_cast<std::uint32_t>(installed.threads.size()));
        installed.threads.push_back(thread);
        installed.mask.push_back(thread.cpuIndex);
      }
      installed.cores.push_back(layoutCore);
    }
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * perGroup;
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      layoutGroup.cores.push_back(group * coresPerGroup + core);
    }
    installed.groups.push_back(layoutGroup);
  }
  const std::uint32_t total = groups * perGroup;
  for (std::uint32_t node = 0; node < memoryNodes; ++node) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = node;
    for (std::uint32_t cpu = (node * total) / memoryNodes;
         cpu < ((node + 1) * total) / memoryNodes;
         ++cpu)
    {
      layoutNode.cpus.push_back(cpu);
    }
    installed.nodes.push_back(layoutNode);
  }
  return installed;
}

}  // namespace sg::test

#endif  // SG_TEST_DETAIL_GROUP_SHAPE_FIXTURE_HPP
