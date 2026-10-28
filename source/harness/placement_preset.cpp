// ============================================================================
// specs/025-topology-placement T011: the preset table, the resolver and the
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
#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
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

// FR-007, E-05: the six names, matched in full and nothing else
// (specs/035 FR-001).
const std::array<PresetName, 6> kPresetNames {{{"none", Preset::NONE},
                                               {"core", Preset::CORE},
                                               {"l3", Preset::L3},
                                               {"smt", Preset::SMT},
                                               {"compact", Preset::COMPACT},
                                               {"node", Preset::NODE}}};

// FR-008: the resolver's postcondition predicate, the plan's cpus
// pairwise distinct. The sorted scan reads it off the finished plan.
// The predicate is consulted only inside SG_ENSURE, which expands to
// nothing in a Release build, so the attribute keeps it compiling in
// every build type.
[[maybe_unused]] auto planCpusDistinct(const std::vector<std::uint32_t>& cpus)
    -> bool
{
  std::vector<std::uint32_t> sorted = cpus;
  std::ranges::sort(sorted);
  return std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
}

// specs/035 FR-009, FR-012, R-04: the node plan's postcondition
// predicate, the planCpusDistinct precedent: every planned cpu stands
// inside the mask and stands on the node the context binds to, and the
// cpus stand pairwise distinct. The predicate is consulted only inside
// SG_ENSURE, which expands to nothing in a Release build, so the
// attribute keeps it compiling in every build type.
[[maybe_unused]] auto planNodesBind(const MachineLayout& layout,
                                    const PlacementPlan& plan) -> bool
{
  for (std::size_t index = 0; index < plan.cpu.size(); ++index) {
    const auto found =
        std::ranges::lower_bound(layout.mask, plan.cpu[index]);
    if (found == layout.mask.end() || *found != plan.cpu[index]) {
      return false;
    }
    const auto node = std::ranges::find_if(
        layout.nodes,
        [&plan, index](const LayoutNode& candidate)
        { return candidate.number == plan.node[index]; });
    if (node == layout.nodes.end()) {
      return false;
    }
    if (std::find(node->cpus.begin(), node->cpus.end(), plan.cpu[index])
        == node->cpus.end())
    {
      return false;
    }
  }
  return planCpusDistinct(plan.cpu);
}

// specs/035: the group ordinal one mask cpu names, out of the threads
// the layout read built (data-model.md).
auto groupOrdinalOf(const MachineLayout& layout, std::uint32_t cpu)
    -> std::uint32_t
{
  const auto found = std::ranges::lower_bound(layout.mask, cpu);
  if (found == layout.mask.end() || *found != cpu) {
    return kNoGroup;
  }
  return layout.threads[static_cast<std::size_t>(found - layout.mask.begin())]
      .groupOrdinal;
}

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

// specs/035 FR-011, R-08: the resolved node plan of the run in flight,
// the runner installs ahead of the contexts it spawns and clears for
// every other run (R-05). One function-local slot, the shape the
// registry and the placement slot carry (R-04).
auto resolvedNodePlan() -> std::optional<PlacementPlan>&
{
  static std::optional<PlacementPlan> slot;
  return slot;
}

// specs/035 FR-013: the one flag a denied start bind sets; the entry
// point answers it with the status-2 refusal the fit walk prints.
auto bindRefusalFlag() -> std::atomic<bool>&
{
  static std::atomic<bool> flag {false};
  return flag;
}

// FR-008 to FR-014, FR-018: the ordered walk. Every place list is built
// from the layout's own orders, which the read built ascending by cpu
// index, so one layout and one count give one plan (SC-03).
auto resolvePreset(const Preset preset,
                   const MachineLayout& layout,
                   const int contexts) -> PlacementPlan
{
  SG_REQUIRE(
      contexts >= 1,
      "a run reaches the resolver only with a count above one " "(FR-008)");
  PlacementPlan plan;
  const auto pushThread = [&plan, &layout](const std::uint32_t ordinal)
  {
    plan.cpu.push_back(layout.threads[ordinal].cpuIndex);
    plan.group.push_back(layout.threads[ordinal].groupOrdinal);
  };

  // The preset arms form a chain of equality tests. An exhaustive switch
  // makes gcov record a default arm no input can reach, and T032 admits
  // no coverage exclusion on this file. The `none` preset takes no arm
  // and leaves the plan empty.
  if (preset == Preset::CORE) {
    for (const auto& core : layout.cores) {
      pushThread(core.threads.front());
    }
  } else if (preset == Preset::L3) {
    for (std::uint32_t group = 0; group < layout.groups.size(); ++group) {
      plan.cpu.push_back(layout.groups[group].lowestCpu);
      plan.group.push_back(group);
    }
  } else if (preset == Preset::SMT) {
    for (const auto& core : layout.cores) {
      for (const auto ordinal : core.threads) {
        pushThread(ordinal);
      }
      break;
    }
  } else if (preset == Preset::COMPACT) {
    for (const auto& group : layout.groups) {
      for (const auto coreOrdinal : group.cores) {
        for (const auto ordinal : layout.cores[coreOrdinal].threads) {
          pushThread(ordinal);
        }
      }
    }
  } else if (preset == Preset::NODE) {
    // specs/035 FR-009, D-2: the mask-touching nodes in the read's
    // order, dealt one cpu at a time: the contexts cycle the nodes, so
    // a count above the node count walks the nodes again, and a count
    // below the node count fills the first nodes in node order. A node
    // the mask excludes takes no place, the way the `l3` plan skips a
    // group the mask does not touch (R-03).
    std::vector<std::uint32_t> numbers;
    std::vector<std::vector<std::uint32_t>> cpusOfNode;
    for (const auto& node : layout.nodes) {
      std::vector<std::uint32_t> masked;
      for (const auto cpu : node.cpus) {
        const auto found = std::ranges::lower_bound(layout.mask, cpu);
        if (found != layout.mask.end() && *found == cpu) {
          masked.push_back(cpu);
        }
      }
      if (!masked.empty()) {
        numbers.push_back(node.number);
        cpusOfNode.push_back(std::move(masked));
      }
    }
    std::vector<std::size_t> next(numbers.size(), 0);
    bool dealt = !numbers.empty();
    while (dealt) {
      dealt = false;
      for (std::size_t index = 0; index < numbers.size(); ++index) {
        if (next[index] < cpusOfNode[index].size()) {
          const std::uint32_t cpu = cpusOfNode[index][next[index]++];
          plan.cpu.push_back(cpu);
          plan.node.push_back(numbers[index]);
          plan.group.push_back(groupOrdinalOf(layout, cpu));
          dealt = true;
        }
      }
    }
  }

  plan.places = static_cast<std::uint32_t>(plan.cpu.size());
  if (static_cast<std::size_t>(contexts) > plan.places) {
    // FR-018: the empty plan carries the count the fit message prints.
    plan.cpu.clear();
    plan.group.clear();
    plan.node.clear();
    return plan;
  }
  plan.cpu.resize(static_cast<std::size_t>(contexts));
  plan.group.resize(static_cast<std::size_t>(contexts));
  if (preset == Preset::NODE) {
    plan.node.resize(static_cast<std::size_t>(contexts));
    SG_ENSURE(planNodesBind(layout, plan),
              "every planned cpu stands inside the mask, on the node "
              "the context binds to, and the cpus stand pairwise "
              "distinct (FR-009, FR-012, R-04)");
    return plan;
  }
  SG_ENSURE(planCpusDistinct(plan.cpu),
            "the plan's cpus stand pairwise distinct (FR-008)");
  return plan;
}

// specs/027 D-4: the planned cpus of one layout stand resolved to their
// thread records; the walk needs the core and the group each planned
// cpu names.
auto plannedCpusResolve(const MachineLayout& machine,
                        const std::vector<std::uint32_t>& plannedCpus) -> bool
{
  for (const auto cpu : plannedCpus) {
    const auto found = std::ranges::lower_bound(machine.mask, cpu);
    if (found == machine.mask.end() || *found != cpu) {
      return false;
    }
  }
  return true;
}

// FR-016, D-4: the resolution's postcondition predicate. The predicate
// is consulted only inside SG_ENSURE, which expands to nothing in a
// Release build, so the attribute keeps it compiling in every build
// type (the planCpusDistinct precedent).
[[maybe_unused]] auto idleResolutionValid(
    const MachineLayout& machine,
    const std::vector<std::uint32_t>& plannedCpus,
    const IdleResolution& resolution) -> bool
{
  const auto isPlanned = [&](const std::uint32_t cpu)
  {
    return std::find(plannedCpus.begin(), plannedCpus.end(), cpu)
        != plannedCpus.end();
  };
  std::vector<std::uint32_t> siblings;
  std::vector<std::uint32_t> touched;
  for (const auto& thread : machine.threads) {
    if (!isPlanned(thread.cpuIndex)) {
      continue;
    }
    if (thread.groupOrdinal != kNoGroup) {
      touched.push_back(thread.groupOrdinal);
    }
    if (thread.coreOrdinal != kNoGroup) {
      for (const auto ordinal : machine.cores.at(thread.coreOrdinal).threads) {
        siblings.push_back(machine.threads.at(ordinal).cpuIndex);
      }
    }
  }
  std::ranges::sort(touched);
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());

  const auto isSibling = [&](const std::uint32_t cpu) {
    return std::find(siblings.begin(), siblings.end(), cpu) != siblings.end();
  };
  // The skip record ascends, and the covered groups are the touched
  // ones, each named exactly once across the two lists (FR-017, D-4);
  // the touched record stands unique, so the covered set's equality
  // reads the whole account. One walk carries every refusal: the
  // planned and sibling names, the mask membership, the group record,
  // and the lowest-free-cpu rule (FR-016, D-4).
  if (!std::ranges::is_sorted(resolution.skippedGroups)) {
    return false;
  }
  std::vector<std::uint32_t> covered = resolution.skippedGroups;
  for (const auto cpu : resolution.cpus) {
    if (isPlanned(cpu) || isSibling(cpu)) {
      return false;
    }
    const auto found = std::ranges::lower_bound(machine.mask, cpu);
    if (found == machine.mask.end() || *found != cpu) {
      return false;
    }
    const auto& thread = machine.threads.at(
        static_cast<std::size_t>(found - machine.mask.begin()));
    if (thread.groupOrdinal == kNoGroup) {
      return false;
    }
    covered.push_back(thread.groupOrdinal);
    for (const auto& other : machine.threads) {
      if (other.groupOrdinal == thread.groupOrdinal && other.cpuIndex < cpu
          && !isPlanned(other.cpuIndex) && !isSibling(other.cpuIndex))
      {
        return false;
      }
    }
  }
  std::ranges::sort(covered);
  return covered == touched;
}

// specs/027 D-4, R-07: the idle resolution. A group stands touched when
// a planned cpu names it, and its free cpu is the lowest thread cpu the
// plans leave: no planned cpu names it and no SMT sibling of a planned
// cpu names it. A touched group with no free cpu stands in the skip
// record (FR-016, FR-017); the empty resolution is the status-2
// condition the caller owns (FR-017).
auto resolveIdleCpus(const MachineLayout& machine,
                     const std::vector<std::uint32_t>& plannedCpus)
    -> IdleResolution
{
  SG_REQUIRE(machine.readable,
             "the idle resolver reads an installed layout (FR-020)");
  SG_REQUIRE(plannedCpusResolve(machine, plannedCpus),
             "every planned cpu names a thread of the layout (R-07)");

  const auto isPlanned = [&](const std::uint32_t cpu)
  {
    return std::find(plannedCpus.begin(), plannedCpus.end(), cpu)
        != plannedCpus.end();
  };
  std::vector<std::uint32_t> excluded;
  std::vector<std::uint32_t> touched;
  for (const auto& thread : machine.threads) {
    if (!isPlanned(thread.cpuIndex)) {
      continue;
    }
    if (thread.groupOrdinal != kNoGroup) {
      touched.push_back(thread.groupOrdinal);
    }
    // The sibling relation reads from the core thread lists the layout
    // walk built (internal.hpp:60-72, R-07).
    if (thread.coreOrdinal != kNoGroup) {
      for (const auto ordinal : machine.cores.at(thread.coreOrdinal).threads) {
        excluded.push_back(machine.threads.at(ordinal).cpuIndex);
      }
    }
  }
  std::ranges::sort(touched);
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());

  IdleResolution result;
  for (const auto group : touched) {
    std::optional<std::uint32_t> freeCpu;
    for (const auto& thread : machine.threads) {
      if (thread.groupOrdinal != group || isPlanned(thread.cpuIndex)
          || std::find(excluded.begin(), excluded.end(), thread.cpuIndex)
              != excluded.end())
      {
        continue;
      }
      freeCpu = thread.cpuIndex;
      break;
    }
    if (freeCpu.has_value()) {
      result.cpus.push_back(*freeCpu);
    } else {
      result.skippedGroups.push_back(group);
    }
  }
  SG_ENSURE(idleResolutionValid(machine, plannedCpus, result),
            "the resolution names the lowest free cpu of each "
            "non-skipped touched group (FR-016, D-4)");
  return result;
}

// FR-018, D-9, R-10: the overlap answer's postcondition predicate, the
// planCpusDistinct precedent. The predicate is consulted only inside
// SG_ENSURE, which expands to nothing in a Release build, so the
// attribute keeps it compiling in every build type.
[[maybe_unused]] auto probeOverlapValid(
    const std::vector<std::uint32_t>& probeCpus,
    const sg::JitterProbeMode mode,
    const std::optional<ProbeOverlap>& overlap) -> bool
{
  return !overlap.has_value()
      || (mode != sg::JitterProbeMode::PRECHECK
          && std::find(probeCpus.begin(), probeCpus.end(), overlap->cpu)
              != probeCpus.end());
}

// FR-017, D-4, E-09: the skipped-groups line the placement disclosure
// carries, the one function the console line and the JSON text read, so
// the one and the many group record stand one shape (E-09).
auto skippedGroupsText(const std::vector<std::uint32_t>& groups) -> std::string
{
  if (groups.empty()) {
    return {};
  }
  std::string text = "skipped L3 group(s) ";
  bool firstGroup = true;
  for (const auto group : groups) {
    if (!firstGroup) {
      text.push_back(',');
    }
    text += std::to_string(group);
    firstGroup = false;
  }
  return text;
}

// specs/027 D-3, D-9, R-10: the overlap check of the alongside phase.
// Each selected instance that carries a preset and more than one
// context plans its contexts; a probe cpu that names a planned cpu, or
// an SMT sibling of one through the core thread lists the layout walk
// built (internal.hpp:60-72, R-07), stands refused with the benchmark's
// name. The `precheck` mode allows every overlap, because it stops
// before the benchmarks start.
auto probeOverlapOf(const MachineLayout& machine,
                    const std::vector<Instance*>& selected,
                    const std::vector<std::uint32_t>& probeCpus,
                    const JitterProbeMode mode) -> std::optional<ProbeOverlap>
{
  SG_REQUIRE(machine.readable,
             "the overlap check reads an installed layout (FR-020)");
  if (mode == JitterProbeMode::PRECHECK) {
    return std::nullopt;
  }
  std::optional<ProbeOverlap> overlap;
  for (const auto* instance : selected) {
    if (instance->threadCount <= 1 || instance->placement == Preset::NONE) {
      continue;
    }
    const auto plan =
        resolvePreset(instance->placement, machine, instance->threadCount);
    for (const auto plannedCpu : plan.cpu) {
      std::vector<std::uint32_t> coreCpus;
      const auto found = std::ranges::lower_bound(machine.mask, plannedCpu);
      if (found != machine.mask.end() && *found == plannedCpu) {
        const auto& thread = machine.threads.at(
            static_cast<std::size_t>(found - machine.mask.begin()));
        if (thread.coreOrdinal != kNoGroup) {
          for (const auto ordinal :
               machine.cores.at(thread.coreOrdinal).threads)
          {
            coreCpus.push_back(machine.threads.at(ordinal).cpuIndex);
          }
        }
      }
      for (const auto probe : probeCpus) {
        if (probe == plannedCpu
            || std::find(coreCpus.begin(), coreCpus.end(), probe)
                != coreCpus.end())
        {
          overlap = ProbeOverlap {probe, instance->name};
          break;
        }
      }
      if (overlap.has_value()) {
        break;
      }
    }
    if (overlap.has_value()) {
      break;
    }
  }
  SG_ENSURE(probeOverlapValid(probeCpus, mode, overlap),
            "the answer names a probe cpu, and the precheck mode "
            "names none (D-3, R-10)");
  return overlap;
}

// FR-019, D-5, FR-036: the mask decision's postcondition predicate, the
// planCpusDistinct precedent.
[[maybe_unused]] auto maskNarrowingValid(
    const std::vector<std::uint32_t>& probeCpus,
    const std::vector<std::uint32_t>& maskCpus,
    const MaskNarrowing& narrowing) -> bool
{
  for (const auto cpu : narrowing.remaining) {
    if (std::find(probeCpus.begin(), probeCpus.end(), cpu) != probeCpus.end()) {
      return false;
    }
    if (std::find(maskCpus.begin(), maskCpus.end(), cpu) == maskCpus.end()) {
      return false;
    }
  }
  bool limited = false;
  for (const auto probe : probeCpus) {
    if (std::find(maskCpus.begin(), maskCpus.end(), probe) == maskCpus.end()) {
      limited = true;
      break;
    }
  }
  return narrowing.limited == limited;
}

// specs/027 D-4, FR-016: the planned cpus the idle resolution skips.
// Every instance with more than one context and a preset contributes
// its plan's cpus; an instance with one context contributes nothing,
// and a multi-context instance the preset names none contributes
// nothing, its threads running unpinned beside the probe (FR-019,
// R-10). The named step stands beside the resolver the plans read
// (N-2.7).
auto plannedCpusOf(const std::vector<Instance*>& selected,
                   const MachineLayout& machine) -> std::vector<std::uint32_t>
{
  std::vector<std::uint32_t> planned;
  for (const auto* instance : selected) {
    if (instance->threadCount > 1 && instance->placement != Preset::NONE) {
      const auto plan =
          resolvePreset(instance->placement, machine, instance->threadCount);
      planned.insert(planned.end(), plan.cpu.begin(), plan.cpu.end());
    }
  }
  return planned;
}

// specs/027 FR-019, FR-006, R-10: the narrowing rule as one named
// step. The calling thread's mask narrows by the probe cpus for an
// instance the runner does not pin, beside a live probe that runs
// beside the benchmark; the precheck mode runs no thread beside one.
auto narrowingApplies(const Instance& entry,
                      const bool probeLive,
                      const JitterProbeMode mode) -> bool
{
  return probeLive && mode != JitterProbeMode::PRECHECK
      && !(entry.threadCount > 1 && entry.placement != Preset::NONE);
}

// specs/027 D-5, FR-036: the mask decision of one benchmark that
// carries no placement. The narrowed mask carries the mask cpus the
// probe cpus leave, in the mask's order; a probe cpu that names no mask
// cpu sets the cpuset-limit answer FR-036 discloses, and the empty
// narrowed mask is the status-2 condition the caller owns (D-5).
auto maskNarrowingOf(const std::vector<std::uint32_t>& probeCpus,
                     const std::vector<std::uint32_t>& maskCpus)
    -> MaskNarrowing
{
  MaskNarrowing result;
  for (const auto probe : probeCpus) {
    if (std::find(maskCpus.begin(), maskCpus.end(), probe) == maskCpus.end()) {
      result.limited = true;
      break;
    }
  }
  for (const auto cpu : maskCpus) {
    if (std::find(probeCpus.begin(), probeCpus.end(), cpu) == probeCpus.end()) {
      result.remaining.push_back(cpu);
    }
  }
  SG_ENSURE(maskNarrowingValid(probeCpus, maskCpus, result),
            "the narrowed mask drops the probe cpus only, and the "
            "limit answer follows the mask (FR-019, FR-036)");
  return result;
}

// specs/035 FR-011: the widest text the compose states, the
// `cpu@group@node` triple of a bound context: three widest indices and
// the two separators that join them, the width rule the standing
// `cpu@group` pair follows (specs/025 FR-021).
constexpr std::size_t kPlacementTextWidth =
    3 * (std::numeric_limits<std::uint32_t>::digits10 + 1) + 2;

// FR-021, data-model.md E-09: the disclosure text of one slot, composed
// from the layout with no pin and no platform call. The buffer stands in
// thread storage, the way the pin's digits do, and the record that
// outlives the thread holds a copy (FR-025).
auto placementText(const PlacementSlot& slot) -> std::string_view
{
  if (slot.cpuIndex == kNoPin) {
    return "NO_PIN";
  }
  thread_local std::array<char, kPlacementTextWidth> buffer {};
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
  // specs/035 FR-011, R-08: the node tail where the run's resolved plan
  // binds the context, and the standing `cpu@group` bytes where it does
  // not (FR-002).
  const auto& nodePlan = resolvedNodePlan();
  if (nodePlan.has_value()) {
    const auto planned =
        std::find(nodePlan->cpu.begin(), nodePlan->cpu.end(), slot.cpuIndex);
    if (planned != nodePlan->cpu.end()) {
      *end++ = '@';
      end = std::to_chars(
               end,
               buffer.end(),
               nodePlan->node[static_cast<std::size_t>(planned
                                  - nodePlan->cpu.begin())])
          .ptr;
    }
  }
  return std::string_view(buffer.data(),
                          static_cast<std::size_t>(end - buffer.data()));
}

// specs/025 FR-005, FR-018, Q-6, Q-7: the fit check walks the selected
// instances after selection and before the first run. A preset over a
// layout the harness cannot read stops the batch whatever the context
// count; a plan the places cannot hold stops it with one line naming
// the preset, the place count and the requested count. An instance
// with no preset stands outside the check, which keeps FR-022 byte
// identity for an ordinary threaded run.
auto placementFitStatus(const std::vector<Instance*>& selected)
    -> std::pair<int, bool>
{
  bool inEffect = false;
  for (auto* instance : selected) {
    if (instance->placement == Preset::NONE) {
      continue;
    }
    const auto& machine = layout();
    if (!machine.readable) {
      std::fprintf(
          stderr,
          "--placement: the harness cannot read the machine " "layout\n");
      return {2, false};
    }
    // specs/035 FR-014, R-10: a node preset over a read that states no
    // memory information refuses one line, before any context starts,
    // whatever the context count, the way the unreadable layout
    // refuses.
    if (instance->placement == Preset::NODE && !machine.memoryReadable) {
      std::fprintf(stderr,
                   "--placement: 'node' needs memory nodes the machine "
                   "read does not state\n");
      return {2, false};
    }
    if (instance->threadCount > 1) {
      inEffect = true;
      const auto plan =
          resolvePreset(instance->placement, machine, instance->threadCount);
      if (plan.cpu.empty()) {
        const auto name = presetText(instance->placement);
        std::fprintf(
            stderr,
            "--placement: '%.*s' places %u of the %d requested " "contexts\n",
            static_cast<int>(name.size()),
            name.data(),
            plan.places,
            instance->threadCount);
        return {2, false};
      }
      // specs/035 FR-013, D-4, R-07: the eager validation pass binds
      // and restores each planned context's node in plan order, before
      // the contexts spawn; a denial in that pass refuses one line
      // naming the context and the node, ahead of every context.
      if (instance->placement == Preset::NODE) {
        const auto& binder = memoryBinder();
        for (std::size_t position = 0; position < plan.node.size();
             ++position)
        {
          if (!binder.bind(plan.node[position])) {
            std::fprintf(stderr,
                         "--placement: the memory bind of ctx%zu to node "
                         "%u was denied\n",
                         position,
                         plan.node[position]);
            return {2, false};
          }
          binder.restore();
        }
      }
    }
  }
  return {0, inEffect};
}

}  // namespace detail
}  // namespace sg
