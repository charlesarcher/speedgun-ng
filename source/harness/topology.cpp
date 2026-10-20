// ============================================================================
// specs/022-topology-placement T010: the machine layout read (US1, FR-001,
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

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <sched.h>

#include <hwloc.h>

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
    const int rc = hwloc_topology_init(&m_topology);
    SG_REQUIRE(rc == 0 && m_topology != nullptr,
               "the walk reads a built topology (R-04)");
  }

  ~TopologyHolder()
  {
    hwloc_topology_destroy(m_topology);
  }

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

  ~BitmapHolder()
  {
    hwloc_bitmap_free(m_bitmap);
  }

  BitmapHolder(const BitmapHolder&) = delete;
  auto operator=(const BitmapHolder&) -> BitmapHolder& = delete;

  [[nodiscard]] auto get() const noexcept -> hwloc_bitmap_t
  {
    return m_bitmap;
  }

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
auto ordinalOf(std::vector<hwloc_obj_t>& objects, hwloc_obj_t object)
    -> std::uint32_t
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
auto readLayout(const std::optional<std::string>& description)
    -> detail::MachineLayout
{
  detail::MachineLayout layout;
  TopologyHolder holder;
  const hwloc_topology_t topology = holder.get();

  // R-04: the readable decision is one rc chain over the description and
  // load answers.
  int rc = 0;
  if (description.has_value()) {
    rc = hwloc_topology_set_synthetic(topology, description->c_str());
  }
  if (rc == 0) {
    rc = hwloc_topology_load(topology);
  }
  if (rc != 0) {
    return layout;
  }

  BitmapHolder allowedHolder;
  const hwloc_bitmap_t allowed = allowedHolder.get();
  processCpuSet(allowed);
  hwloc_bitmap_and(allowed, allowed,
                   hwloc_topology_get_topology_cpuset(topology));

  std::vector<hwloc_obj_t> coreObjects;
  std::vector<hwloc_obj_t> groupObjects;
  std::vector<hwloc_obj_t> packageObjects;

  int cpu = -1;
  while ((cpu = hwloc_bitmap_next(allowed, cpu)) != -1) {
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

    if (group != nullptr
        && thread.groupOrdinal == layout.groups.size()) {
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

  layout.packages = static_cast<std::uint32_t>(packageObjects.size());
  layout.readable = true;
  return layout;
}

// R-04: the two installs, each a function-local slot consulted before the
// one read and never able to invalidate the cache after it.
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

}  // namespace

namespace detail
{

auto installTestLayout(std::optional<detail::MachineLayout> layout) -> void
{
  installedLayoutSlot() = std::move(layout);
}

auto installTestTopologyDescription(std::optional<std::string> description)
    -> void
{
  installedDescriptionSlot() = std::move(description);
}

// FR-002, R-04: the one cached read. An installed layout short-circuits
// the host read; an installed description reaches
// `hwloc_topology_set_synthetic`.
auto layout() -> const MachineLayout&
{
  static const detail::MachineLayout kCached = [] {
    const auto& installed = installedLayoutSlot();
    if (installed.has_value()) {
      return *installed;
    }
    return readLayout(installedDescriptionSlot());
  }();
  return kCached;
}

}  // namespace detail
}  // namespace sg
