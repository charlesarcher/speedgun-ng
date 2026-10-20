// ============================================================================
// specs/022-topology-placement T011: the preset table, the resolver and the
// disclosure text (US1, US2, FR-007 to FR-014, FR-018, FR-021, E-09, R-03).
//
// The resolver is the pure walk of `contracts/topology-placement.md`: it
// reads a layout and a count and returns the plan, with no hwloc type in
// this file (R-03) and no platform call. `placementText` composes the
// disclosure pair from the layout, and the text stands in thread storage
// the way the pin's digits do, so the record that outlives the thread
// copies it (FR-021, FR-025).
// ============================================================================

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace sg
{
namespace detail
{
namespace
{

struct PresetName
{
  std::string_view name;
  Preset preset;
};

// FR-007, E-05: the five names, matched in full and nothing else.
const std::array<PresetName, 5> kPresetNames {
    {{"none", Preset::NONE},
     {"core", Preset::CORE},
     {"l3", Preset::L3},
     {"smt", Preset::SMT},
     {"compact", Preset::COMPACT}}};

}  // namespace

auto parsePreset(const std::string_view text) -> std::optional<Preset>
{
  for (const auto& entry : kPresetNames) {
    if (text == entry.name) {
      return entry.preset;
    }
  }
  return std::nullopt;
}

// FR-016: the name a message prints for a preset; the table stands in
// enum order.
auto presetText(const Preset preset) -> std::string_view
{
  return kPresetNames[static_cast<std::size_t>(preset)].name;
}

// FR-008 to FR-014, FR-018: the ordered walk. Every place list is built
// from the layout's own orders, which the read built ascending by cpu
// index, so one layout and one count give one plan (SC-03).
auto resolvePreset(const Preset preset, const MachineLayout& layout,
                   const int contexts) -> PlacementPlan
{
  SG_REQUIRE(contexts >= 1,
             "a run reaches the resolver only with a count above one "
             "(FR-008)");
  PlacementPlan plan;
  const auto pushThread = [&plan, &layout](const std::uint32_t ordinal) {
    plan.cpu.push_back(layout.threads[ordinal].cpuIndex);
    plan.group.push_back(layout.threads[ordinal].groupOrdinal);
  };

  switch (preset) {
  case Preset::NONE:
    return plan;
  case Preset::CORE:
    for (const auto& core : layout.cores) {
      pushThread(core.threads.front());
    }
    break;
  case Preset::L3:
    for (std::uint32_t group = 0; group < layout.groups.size(); ++group) {
      plan.cpu.push_back(layout.groups[group].lowestCpu);
      plan.group.push_back(group);
    }
    break;
  case Preset::SMT:
    for (const auto& core : layout.cores) {
      for (const auto ordinal : core.threads) {
        pushThread(ordinal);
      }
      break;
    }
    break;
  case Preset::COMPACT:
    for (const auto& group : layout.groups) {
      for (const auto coreOrdinal : group.cores) {
        for (const auto ordinal : layout.cores[coreOrdinal].threads) {
          pushThread(ordinal);
        }
      }
    }
    break;
  }

  plan.places = static_cast<std::uint32_t>(plan.cpu.size());
  if (static_cast<std::size_t>(contexts) > plan.places) {
    // FR-018: the empty plan carries the count the fit message prints.
    plan.cpu.clear();
    plan.group.clear();
    return plan;
  }
  plan.cpu.resize(static_cast<std::size_t>(contexts));
  plan.group.resize(static_cast<std::size_t>(contexts));
  SG_ENSURE(
      [](const std::vector<std::uint32_t>& cpus) {
        std::vector<std::uint32_t> sorted = cpus;
        std::ranges::sort(sorted);
        return std::adjacent_find(sorted.begin(), sorted.end())
            == sorted.end();
      }(plan.cpu),
      "the plan's cpus stand pairwise distinct (FR-008)");
  return plan;
}

// FR-021, data-model.md E-09: the disclosure text of one slot, composed
// from the layout with no pin and no platform call. The buffer stands in
// thread storage, the way the pin's digits do, and the record that
// outlives the thread holds a copy (FR-025).
auto placementText(const PlacementSlot& slot) -> std::string_view
{
  if (slot.cpuIndex == kNoPin) {
    return "NO_PIN";
  }
  thread_local std::array<char, 24> buffer {};
  char* end = std::to_chars(buffer.begin(), buffer.end(), slot.cpuIndex).ptr;
  const auto& machine = layout();
  const auto found = std::ranges::lower_bound(machine.mask, slot.cpuIndex);
  if (found != machine.mask.end() && *found == slot.cpuIndex) {
    const auto& thread =
        machine.threads[static_cast<std::size_t>(found - machine.mask.begin())];
    if (thread.groupOrdinal != kNoGroup) {
      *end++ = '@';
      end = std::to_chars(end, buffer.end(), thread.groupOrdinal).ptr;
    }
  }
  return std::string_view(
      buffer.data(), static_cast<std::size_t>(end - buffer.data()));
}

}  // namespace detail
}  // namespace sg
