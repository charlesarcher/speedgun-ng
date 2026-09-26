#pragma once

// Internal core of the counters library (specs/007-counters-and-timers).
// Never installed: this header is shared by the source/counters
// translation units and carries the tree, plan, and scope internals
// behind the opaque handles the public headers expose.

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace sg::counters
{

// One catalog leaf in the system tree.
struct leaf_record
{
  detail::leaf_core core;
  // True when the source discloses an enabled/running ratio pair as
  // ordinary leaves (FR-019); ratio pair slots land with US6.
  bool has_ratio_pair = false;
  // Index into the provider list; the merged machine root keeps the
  // per-leaf attribution its seeding provider had.
  int provider_index = -1;
};

// One countable object in the system tree.
struct tree_node
{
  std::string kind;
  std::string path;  // canonical; unique per parent
  std::string alias;
  std::string description;
  int provider_index = -1;  // index into the provider list, root is -1
  std::vector<leaf_record> leaves;
};

// Shared buffer state living beside every recorder arena (US2) and
// inside every scope.
struct buffer_state
{
  std::uint64_t head = 0;  // samples committed so far
  bool wrapped = false;
  std::uint64_t dropped = 0;
};

// One provider window in the compiled read plan: the resolved thunk,
// the live reader, and the contiguous slot range it fills.
struct read_group
{
  window_reader::read_thunk thunk = nullptr;
  std::unique_ptr<window_reader> reader;
  std::size_t offset = 0;
  std::size_t count = 0;
};

// The compiled plan behind the opaque handle (E-07).
struct plan_impl
{
  struct slot
  {
    detail::leaf_core core;
    bool has_ratio_pair = false;
  };

  std::vector<slot> slots;
  std::map<std::string, std::size_t> by_address;
  std::vector<read_group> groups;
  // Recorder arenas: one buffer per minted recorder, owned by the
  // plan; recorders are non-owning cursors (FR-029).
  std::vector<std::unique_ptr<std::uint64_t[]>> arenas;
  target bound_target;
  std::thread::id bound_thread = std::this_thread::get_id();

  [[nodiscard]] auto leaf_count() const noexcept -> std::size_t
  {
    return slots.size();
  }
};

// The compiled fan-out layout behind the fanout_plan handle
// (US3 scenario 5): the instantiated inner plan and the selected
// canonical paths in selection order.
struct fanout_impl
{
  std::unique_ptr<plan> inner;
  std::vector<std::string> paths;
};

// The two-point window behind the scope handle (FR-030).
struct scope_core
{
  const plan_impl* impl = nullptr;
  std::vector<std::uint64_t> buffer;  // leaf_count columns, stride 2
  buffer_state state;
  bool started = false;
  bool finished = false;

  [[nodiscard]] auto view() const noexcept -> recorder_api
  {
    return recorder_api {
        .impl = impl,
        .columns = buffer.data(),
        .stride = 2,
        .count = static_cast<std::size_t>(state.head),
        .wrapped = false,
        .dropped = 0,
    };
  }
};

struct system::impl
{
  std::vector<std::unique_ptr<provider_iface>> providers;
  std::map<std::string, std::unique_ptr<tree_node>> objects;  // canonical
  std::map<std::string, std::unique_ptr<sg::counters::object>> handles;
  std::map<std::string, std::string> aliases;  // alias to path
  bool open = false;

  auto ensure_open() noexcept -> void { open = true; }

  [[nodiscard]] auto canonicalize(std::string_view path) const -> std::string
  {
    const auto alias = aliases.find(std::string(path));
    if (alias != aliases.end()) {
      return alias->second;
    }
    return std::string(path);
  }

  [[nodiscard]] auto find(std::string_view path) const -> const tree_node*
  {
    const auto it = objects.find(canonicalize(path));
    if (it == objects.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  [[nodiscard]] auto find(std::string_view path) -> tree_node*
  {
    const auto it = objects.find(canonicalize(path));
    if (it == objects.end()) {
      return nullptr;
    }
    return it->second.get();
  }
};

// Splits a canonical leaf address into the object path and the leaf
// name at the final separator.
[[nodiscard]] inline auto split_leaf_address(std::string_view address)
    -> std::pair<std::string_view, std::string_view>
{
  const auto slash = address.rfind('/');
  if (slash == std::string_view::npos) {
    return {address, {}};
  }
  return {address.substr(0, slash), address.substr(slash + 1)};
}

}  // namespace sg::counters
