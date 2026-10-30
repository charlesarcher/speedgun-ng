// ============================================================================
// specs/039-context-groups T006: the partition of one instance's contexts
// into the groups one preset names (FR-006 to FR-010, D-3, D-4,
// contracts/group-partition.md B-1, B-2, A-1 to A-6).
//
// The walk is pure: it reads the plan `resolvePreset` wrote and answers
// the groups, so it starts no thread, reads no clock and opens no file
// (A-5). The plan keeps every value the resolver wrote, the way the fit
// walk reads a plan without moving a pin (FR-009).
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{
namespace detail
{
namespace
{

// B-2: the answer of one group for one position, read off the positions
// the group holds.
auto holdsPosition(const ContextGroup& group,
                   const std::size_t position) -> bool
{
  return std::find(group.slots.begin(), group.slots.end(), position)
      != group.slots.end();
}

}  // namespace

// FR-006 to FR-010, D-3, D-4: the ordered walk. A group joins the answer
// at the first position of its unit, so the answer lists its groups by
// that first position and reads the same over one plan twice (A-4).
auto groupPartition(const Preset preset,
                    const PlacementPlan& plan,
                    const std::size_t contexts) -> std::vector<ContextGroup>
{
  SG_REQUIRE(contexts >= 1,
             "a run reaches the partition with a count above one (FR-001)");
  std::vector<ContextGroup> partition;

  // FR-007 and FR-010: the preset that names no unit, and the instance
  // of one context, keep the ungrouped shape master runs (FR-004).
  if (preset == Preset::NONE || contexts < 2) {
    return partition;
  }

  // D-3: the unit the preset groups by, the memory node under `node`
  // and the L3 group under the presets that walk the cache topology.
  const bool byNode = preset == Preset::NODE;
  const auto& units = byNode ? plan.node : plan.group;
  SG_REQUIRE(units.size() >= contexts,
             "the plan answers the unit of every position it reads (B-1)");

  // D-4 and A-6: the name spells the unit and the ordinal the plan
  // carries, so a mask that excludes a unit names the surviving one by
  // its own number.
  const std::string_view prefix = byNode ? "node-" : "l3-";
  std::vector<std::uint32_t> ordinals;
  for (std::size_t position = 0; position < contexts; ++position) {
    const std::uint32_t unit = units[position];
    if (unit == kNoGroup) {
      // FR-006: a read that states no unit for this preset names no
      // partition, and the run keeps the shape of FR-004.
      return {};
    }
    const auto known = std::find(ordinals.begin(), ordinals.end(), unit);
    if (known == ordinals.end()) {
      ordinals.push_back(unit);
      partition.push_back(
          {std::string(prefix) + std::to_string(unit), {position}});
    } else {
      partition[static_cast<std::size_t>(known - ordinals.begin())]
          .slots.push_back(position);
    }
  }
  return partition;
}

// FR-022, B-2: the group of one position. The partition covers the
// position once, so the walk stops at the group that names it.
auto groupOfContext(const std::vector<ContextGroup>& partition,
                    const std::size_t position) -> std::size_t
{
  std::size_t group = 0;
  while (!holdsPosition(partition[group], position)) {
    ++group;
  }
  return group;
}

}  // namespace detail
}  // namespace sg
