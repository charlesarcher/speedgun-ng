// ============================================================================
// specs/025-topology-placement T010: the machine layout read (US1, FR-001,
// FR-002, FR-004, FR-005, FR-006, R-01, R-04).
//
// This is the one harness translation unit that includes <hwloc.h>
// (FR-004, R-01); the vendored sg_hwloc archive answers the references and
// tools/hwloc/hwloc_nm_proof.sh is the covering check. The read happens at
// most once per process, on the first need, in the function-local static of
// `layout()`; both install seams are consulted before that read and the
// cache is never invalidated (FR-002, FR-006, R-04).
//
// The walk is one loop over the processing units of the intersection of
// the process affinity mask and the topology cpu set, with a parent, core
// and L3 lookup per unit and no per-object-type switch tree, so the single
// host read reaches every branch the walk holds (Principle VI, FR-034).
// The `hwloc_topology_init` answer rides a contract macro: no scenario can
// make it fail, an address-space limit low enough to starve it kills the
// process in the dynamic loader before the call, and the macro line is
// omitted from coverage the way `BenchmarkHandle::threads` carries its
// undrivable answer (R-04, cmake/coverage.cmake:76).
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <hwloc.h>
#include <sched.h>

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{
namespace
{

// The topology lives for one read: built here, destroyed on the way out,
// and the walk never enters on a null topology (R-04).
class TopologyHolder
{
public:
  TopologyHolder()
  {
    // The answer is read only inside SG_REQUIRE, which expands to
    // nothing in a Release build, so the attribute keeps it compiling
    // in every build type.
    [[maybe_unused]] const int rc = hwloc_topology_init(&m_topology);
    SG_REQUIRE(rc == 0 && m_topology != nullptr,
               "the walk reads a built topology (R-04)");
  }

  ~TopologyHolder() { hwloc_topology_destroy(m_topology); }

  TopologyHolder(const TopologyHolder&) = delete;
  auto operator=(const TopologyHolder&) -> TopologyHolder& = delete;

  [[nodiscard]] auto get() const noexcept -> hwloc_topology_t
  {
    return m_topology;
  }

private:
  hwloc_topology_t m_topology = nullptr;
};

// A bitmap the read owns and frees on the way out.
class BitmapHolder
{
public:
  BitmapHolder()
  {
    m_bitmap = hwloc_bitmap_alloc();
    SG_REQUIRE(m_bitmap != nullptr,
               "the walk reads an allocated bitmap (R-04)");
  }

  ~BitmapHolder() { hwloc_bitmap_free(m_bitmap); }

  BitmapHolder(const BitmapHolder&) = delete;
  auto operator=(const BitmapHolder&) -> BitmapHolder& = delete;

  [[nodiscard]] auto get() const noexcept -> hwloc_bitmap_t { return m_bitmap; }

private:
  hwloc_bitmap_t m_bitmap = nullptr;
};

// FR-005: the process affinity is a separate fact the harness reads
// itself; hwloc's own sets follow the machine and the control group.
auto processCpuSet(hwloc_bitmap_t set) -> void
{
  cpu_set_t affinity {};
  CPU_ZERO(&affinity);
  (void)sched_getaffinity(0, sizeof(affinity), &affinity);
  for (std::size_t cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
    if (CPU_ISSET(cpu, &affinity)) {
      hwloc_bitmap_set(set, static_cast<unsigned>(cpu));
    }
  }
}

// The ordinal of `object` in the first-seen order of `objects`, the order
// ascending cpu index gives; a null object names no ordinal.
auto ordinalOf(std::vector<hwloc_obj_t>& objects,
               hwloc_obj_t object) -> std::uint32_t
{
  if (object == nullptr) {
    return detail::kNoGroup;
  }
  for (std::size_t index = 0; index < objects.size(); ++index) {
    if (objects[index] == object) {
      return static_cast<std::uint32_t>(index);
    }
  }
  objects.push_back(object);
  return static_cast<std::uint32_t>(objects.size() - 1);
}

// FR-001, FR-002, FR-005: the one read. A failed load leaves `readable`
// false and no partial layout; the walk builds the threads, cores, groups
// and packages of the restricted set in ascending cpu order.
auto installedLoadFailureSlot() -> std::optional<int>&;

auto readLayout(const hwloc_topology_t topology,
                const std::optional<std::string>& description)
    -> detail::MachineLayout
{
  detail::MachineLayout layout;

  // R-04: the readable decision is one rc chain over the description and
  // load answers.
  int rc = 0;
  if (description.has_value()) {
    rc = hwloc_topology_set_synthetic(topology, description->c_str());
  }
  if (rc == 0) {
    // Review decision D-1: the load answer is a seam. An installed
    // failure stands in for the hwloc answer, so the failed-load arm
    // stands under test on every platform.
    const auto failure = installedLoadFailureSlot();
    rc = failure.has_value() ? *failure : hwloc_topology_load(topology);
  }
  if (rc != 0) {
    return layout;
  }

  BitmapHolder allowedHolder;
  const hwloc_bitmap_t allowed = allowedHolder.get();
  processCpuSet(allowed);
  hwloc_bitmap_and(
      allowed, allowed, hwloc_topology_get_topology_cpuset(topology));

  std::vector<hwloc_obj_t> coreObjects;
  std::vector<hwloc_obj_t> groupObjects;
  std::vector<hwloc_obj_t> packageObjects;

  int cpu = -1;
  for (;;) {
    cpu = hwloc_bitmap_next(allowed, cpu);
    if (cpu == -1) {
      break;
    }
    const hwloc_obj_t pu =
        hwloc_get_pu_obj_by_os_index(topology, static_cast<unsigned>(cpu));

    hwloc_obj_t core = pu;
    while (core != nullptr && core->type != HWLOC_OBJ_CORE) {
      core = core->parent;
    }
    hwloc_obj_t group = pu;
    while (group != nullptr && group->type != HWLOC_OBJ_L3CACHE) {
      group = group->parent;
    }
    hwloc_obj_t package = pu;
    while (package != nullptr && package->type != HWLOC_OBJ_PACKAGE) {
      package = package->parent;
    }

    detail::LayoutThread thread;
    thread.cpuIndex = static_cast<std::uint32_t>(cpu);
    thread.coreOrdinal = ordinalOf(coreObjects, core);
    thread.groupOrdinal = ordinalOf(groupObjects, group);
    const std::uint32_t threadOrdinal =
        static_cast<std::uint32_t>(layout.threads.size());
    layout.threads.push_back(thread);
    layout.mask.push_back(thread.cpuIndex);
    (void)ordinalOf(packageObjects, package);

    if (group != nullptr && thread.groupOrdinal == layout.groups.size()) {
      layout.groups.push_back({});
      layout.groups.back().lowestCpu = thread.cpuIndex;
    }
    if (core != nullptr) {
      if (thread.coreOrdinal == layout.cores.size()) {
        layout.cores.push_back({});
        layout.cores.back().groupOrdinal = thread.groupOrdinal;
        if (group != nullptr) {
          layout.groups[thread.groupOrdinal].cores.push_back(
              thread.coreOrdinal);
        }
      }
      layout.cores[thread.coreOrdinal].threads.push_back(threadOrdinal);
    }
  }

  // specs/035 FR-005, R-01: the memory nodes the read states, in the
  // read's order, each with the node number and the cpus the read
  // states; the mask filters at plan and listing time (data-model.md).
  // A read that states no memory information leaves the vector empty
  // and `memoryReadable` false beside it (FR-007, R-09).
  for (hwloc_obj_t node =
           hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE, nullptr);
       node != nullptr;
       node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE, node))
  {
    detail::LayoutNode layoutNode;
    layoutNode.number = static_cast<std::uint32_t>(node->os_index);
    int nodeCpu = -1;
    for (;;) {
      nodeCpu = hwloc_bitmap_next(node->cpuset, nodeCpu);
      if (nodeCpu == -1) {
        break;
      }
      layoutNode.cpus.push_back(static_cast<std::uint32_t>(nodeCpu));
    }
    layout.nodes.push_back(std::move(layoutNode));
  }
  layout.memoryReadable = !layout.nodes.empty();

  layout.packages = static_cast<std::uint32_t>(packageObjects.size());
  layout.readable = true;
  return layout;
}

// R-04: the three installs, each a function-local slot consulted before
// the one read and never able to invalidate the cache after it.
auto installedLayoutSlot() -> std::optional<detail::MachineLayout>&
{
  static std::optional<detail::MachineLayout> slot;
  return slot;
}

auto installedDescriptionSlot() -> std::optional<std::string>&
{
  static std::optional<std::string> slot;
  return slot;
}

auto installedLoadFailureSlot() -> std::optional<int>&
{
  static std::optional<int> slot;
  return slot;
}

auto installedOnlineCpusSlot() -> std::optional<std::vector<std::uint32_t>>&
{
  static std::optional<std::vector<std::uint32_t>> slot;
  return slot;
}

auto installedOnlineLineSlot() -> std::optional<std::string>&
{
  static std::optional<std::string> slot;
  return slot;
}

auto installedMemoryBinderSlot() -> std::optional<detail::MemoryBindOps>&
{
  static std::optional<detail::MemoryBindOps> slot;
  return slot;
}

}  // namespace

namespace detail
{

// The list `/sys/devices/system/cpu/online` carries one line in the
// kernel's cpu-list spelling: comma-separated lone indices and
// `low-high` spans in ascending order, the gaps standing for the cpus
// the kernel holds offline. A token that parses as no number names no
// cpu and the read moves to the token after it, so a short read answers
// the cpus it did carry.
auto appendOnlineCpuList(std::vector<std::uint32_t>& cpus,
                         std::string_view text) -> void
{
  while (!text.empty()) {
    const std::size_t comma = text.find_first_of(',');
    const auto part = text.substr(0, comma);
    text = comma == std::string_view::npos ? std::string_view {}
                                           : text.substr(comma + 1);
    std::size_t at = 0;
    auto readNumber = [&part, &at]() -> std::optional<std::uint32_t>
    {
      std::uint32_t value = 0;
      const std::size_t first = at;
      while (at < part.size() && part[at] >= '0' && part[at] <= '9') {
        value = value * 10U + static_cast<std::uint32_t>(part[at] - '0');
        ++at;
      }
      if (at == first) {
        return std::nullopt;
      }
      return value;
    };
    const auto low = readNumber();
    if (!low.has_value()) {
      continue;
    }
    auto high = low;
    if (at < part.size() && part[at] == '-') {
      ++at;
      const auto end = readNumber();
      if (!end.has_value()) {
        continue;
      }
      high = end;
    }
    // The counter stands wider than an index, so a span whose high end
    // is the last index the spelling carries answers every index in the
    // span and cannot wrap around.
    for (auto cpu = std::uint64_t {*low}; cpu <= *high; ++cpu) {
      cpus.push_back(static_cast<std::uint32_t>(cpu));
    }
  }
}

auto installTestLayout(std::optional<detail::MachineLayout> layout) -> void
{
  installedLayoutSlot() = std::move(layout);
}

auto installTestTopologyDescription(std::optional<std::string> description)
    -> void
{
  installedDescriptionSlot() = std::move(description);
}

auto installTestLoadFailure(std::optional<int> answer) -> void
{
  installedLoadFailureSlot() = answer;
}

auto installTestOnlineCpus(std::optional<std::vector<std::uint32_t>> cpus)
    -> void
{
  installedOnlineCpusSlot() = std::move(cpus);
}

auto installTestOnlineLine(std::optional<std::string> line) -> void
{
  installedOnlineLineSlot() = std::move(line);
}

auto installTestMemoryBinder(std::optional<MemoryBindOps> binder) -> void
{
  installedMemoryBinderSlot() = std::move(binder);
}

// FR-005, FR-041: the online cpu set the placement bound and the option
// value grammar read. The set is consulted at each call, never cached,
// so an installed set decides the call that follows it (Q-5). The host
// read is the line the kernel owns; a host whose line stands unread, as
// an empty line stands unread, answers the process affinity mask, gaps
// and all, and a count never stands as the fallback source (D-02,
// FR-021). A mask read that fails leaves the zeroed mask, so the set
// stands empty and both bounds refuse every index.
auto onlineCpuSet() -> std::vector<std::uint32_t>
{
  if (const auto& installed = installedOnlineCpusSlot(); installed.has_value())
  {
    return *installed;
  }
  auto text = std::string {};
  if (const auto& installed = installedOnlineLineSlot(); installed.has_value())
  {
    text = *installed;
  } else {
    auto stream = std::ifstream("/sys/devices/system/cpu/online");
    // A stream that opened no file reads as an empty line: getline
    // answers nothing and leaves the text empty, so the fallback below
    // owns the unreadable host beside the empty line (D-02, FR-021).
    std::getline(stream, text);
  }
  auto cpus = std::vector<std::uint32_t> {};
  appendOnlineCpuList(cpus, text);
  if (cpus.empty()) {
    cpu_set_t mask {};
    CPU_ZERO(&mask);
    (void)sched_getaffinity(0, sizeof(mask), &mask);
    for (std::size_t index = 0; index < CPU_SETSIZE; ++index) {
      if (CPU_ISSET(index, &mask)) {
        cpus.push_back(static_cast<std::uint32_t>(index));
      }
    }
  }
  return cpus;
}

// specs/035 R-06: the one cached read together with the topology that
// backs the production binder. An installed layout short-circuits the
// host read and stands with no topology behind it; the host read keeps
// the topology it loaded, so the bind call binds by the nodes that read
// states and the read stays one time per process (FR-002, R-04).
namespace
{

struct MachineRead
{
  TopologyHolder holder;
  hwloc_topology_t topology = nullptr;
  MachineLayout layout;
};

auto machineRead() -> const MachineRead&
{
  static const std::unique_ptr<const MachineRead> kCached = []
  {
    auto read = std::make_unique<MachineRead>();
    const auto& installed = installedLayoutSlot();
    if (installed.has_value()) {
      read->layout = *installed;
      return read;
    }
    read->topology = read->holder.get();
    read->layout = readLayout(read->holder.get(), installedDescriptionSlot());
    return read;
  }();
  return *kCached;
}

}  // namespace

auto layout() -> const MachineLayout&
{
  return machineRead().layout;
}

namespace
{

// specs/035 R-06: the policy one bind saved for its restore, the bind
// and the restore standing in sequence on one thread (FR-013). The
// saved state stands in thread storage, for the binds of the context
// threads that run beside each other: each thread saves the policy it
// found, and the restore reads the one its own binds saved (FR-013).
auto savedBindSet() -> hwloc_bitmap_t
{
  thread_local BitmapHolder saved;
  return saved.get();
}

auto savedBindPolicy() -> hwloc_membind_policy_t&
{
  thread_local hwloc_membind_policy_t policy = HWLOC_MEMBIND_DEFAULT;
  return policy;
}

// specs/035 R-06: the production binder, one hwloc membind call by
// node, thread scope, on the calling thread, inside the one hwloc
// translation unit (spec 025 FR-004). The bind saves the policy it
// found for its restore, so the eager validation pass leaves the
// process the policy it carried (FR-013). A run whose layout stands
// installed binds through no topology, and the call answers the
// refusal the fit walk prints (FR-013).
auto productionMemoryBind(const std::uint32_t node) -> bool
{
  const hwloc_topology_t topology = machineRead().topology;
  if (topology == nullptr) {
    return false;
  }
  BitmapHolder nodeset;
  hwloc_bitmap_only(nodeset.get(), node);
  BitmapHolder previous;
  hwloc_membind_policy_t policy = HWLOC_MEMBIND_DEFAULT;
  // A read that fails leaves the default policy and the empty set it
  // started with, so the restore answers the default the run carried
  // when the host states no policy to save (FR-013).
  (void)hwloc_get_membind(topology,
                          previous.get(),
                          &policy,
                          HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_THREAD);
  const int set =
      hwloc_set_membind(topology,
                        nodeset.get(),
                        HWLOC_MEMBIND_BIND,
                        HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_THREAD);
  if (set != 0) {
    return false;
  }
  hwloc_bitmap_copy(savedBindSet(), previous.get());
  savedBindPolicy() = policy;
  return true;
}

auto productionMemoryRestore() -> void
{
  const hwloc_topology_t topology = machineRead().topology;
  if (topology == nullptr) {
    return;
  }
  (void)hwloc_set_membind(topology,
                          savedBindSet(),
                          savedBindPolicy(),
                          HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_THREAD);
}

}  // namespace

auto memoryBinder() -> const MemoryBindOps&
{
  static const MemoryBindOps kProduction {&productionMemoryBind,
                                          &productionMemoryRestore};
  const auto& installed = installedMemoryBinderSlot();
  return installed.has_value() ? *installed : kProduction;
}

// FR-036: the comma-joined range spelling of a cpu set, consecutive
// cpus folded into one `low-high` span. The mask and each group line
// print through it.
auto appendRangeText(std::string& text, std::vector<std::uint32_t> cpus) -> void
{
  std::ranges::sort(cpus);
  for (std::size_t at = 0; at < cpus.size();) {
    std::size_t last = at;
    while (last + 1 < cpus.size() && cpus[last + 1] == cpus[last] + 1) {
      ++last;
    }
    if (at != 0) {
      text += ',';
    }
    text += std::to_string(cpus[at]);
    if (last != at) {
      text += '-' + std::to_string(cpus[last]);
    }
    at = last + 1;
  }
}

// FR-036, data-model.md E-10: the `--topology` block. The counts line
// carries the mask the layout was built from, and one line per L3
// group names that group's cpus inside the mask.
auto printTopologyListing(const MachineLayout& machine) -> void
{
  std::string maskText;
  appendRangeText(maskText, machine.mask);
  std::printf(
      "topology: packages=%u l3-groups=%zu cores=%zu threads=%zu mask=%s\n",
      machine.packages,
      machine.groups.size(),
      machine.cores.size(),
      machine.threads.size(),
      maskText.c_str());
  for (std::size_t group = 0; group < machine.groups.size(); ++group) {
    std::vector<std::uint32_t> cpus;
    for (const auto& thread : machine.threads) {
      if (thread.groupOrdinal == group) {
        cpus.push_back(thread.cpuIndex);
      }
    }
    std::string groupText;
    appendRangeText(groupText, std::move(cpus));
    std::printf("l3 %zu: %s\n", group, groupText.c_str());
  }
  // specs/035 FR-006, D-3: the node family under the L3 lines, in the
  // L3 lines' shape, one line per memory node the process mask touches,
  // in the read's node order; a node the mask excludes prints no line,
  // the way the plan gives it no context (R-02). A read that states no
  // memory information prints the not-available line in the family's
  // place and keeps the rest of the bytes (FR-007, R-09).
  if (!machine.memoryReadable) {
    std::printf("node: not available\n");
    return;
  }
  for (const auto& node : machine.nodes) {
    std::vector<std::uint32_t> cpus;
    for (const auto cpu : node.cpus) {
      const auto found = std::ranges::lower_bound(machine.mask, cpu);
      if (found != machine.mask.end() && *found == cpu) {
        cpus.push_back(cpu);
      }
    }
    if (cpus.empty()) {
      continue;
    }
    std::string nodeText;
    appendRangeText(nodeText, std::move(cpus));
    std::printf("node %u: %s\n", node.number, nodeText.c_str());
  }
}

}  // namespace detail
}  // namespace sg
