#pragma once

// Internal core of the counters library (specs/007-counters-and-timers).
// Never installed: this header is shared by the source/counters
// translation units and carries the tree, plan, and scope internals
// behind the opaque handles the public headers expose.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
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
  std::vector<leaf_record> leaves;
};

// Shared buffer state living beside every recorder arena (US2) and
// inside every scope.
struct buffer_state
{
  std::uint64_t head = 0;  // samples committed so far
};

// One provider window in the compiled read plan: the resolved thunk,
// the live reader, and the contiguous slot range it fills.
struct read_group
{
  window_reader::read_thunk thunk = nullptr;
  std::unique_ptr<window_reader> reader;
  std::size_t offset = 0;
  std::size_t count = 0;
  // This group's own disclosure column, the managed slot its window writes
  // the per-action state to. Each group has one, laid out past its own slot
  // range, because a plan drawing leaves from two providers discloses each
  // provider's gaps in its own column (FR-001, FR-002, FR-004).
  std::size_t disclosure_slot = 0;
};

// The measured per-action cost of one `sample()` on one plan (FR-032).
struct overhead_sample
{
  double min_ns = 0.0;
  double median_ns = 0.0;
  double max_ns = 0.0;
};

// The compiled plan behind the opaque handle (E-07).
struct plan_impl
{
  struct slot
  {
    detail::leaf_core core;
    bool has_ratio_pair = false;
    // Slot indices of this leaf's object-level enabled and running
    // leaves, or npos when the object discloses no time pair. The fold
    // reads them for the multiplex ratio (FR-019, FR-041).
    std::size_t ratio_enabled = no_ratio_slot;
    std::size_t ratio_running = no_ratio_slot;
  };

  static constexpr std::size_t no_ratio_slot = static_cast<std::size_t>(-1);

  std::vector<slot> slots;
  // The managed column carrying the per-action disclosure: a measured
  // action writes the countability value the catalog publishes for the
  // entry, and an action that measured nothing writes
  // `Availability::GAP`. One column per read group sits past that group's
  // own last managed leaf, so a plan drawing leaves from two providers
  // discloses each provider's gaps in its own column, and a caller reads
  // the column to tell a measured zero from a gap (FR-001, FR-002, FR-007).
  // A fold resolves the column from the group that owns the leaf rather
  // than from the plan, so this field names the first group's column for
  // the callers that read one column without naming a group.
  std::size_t disclosure_slot = 0;
  // Address to column slot, read by the fold layer. A composite's ops
  // and algebraic exponents travel with its spine, and no per-composite
  // program is stored here (T091): a fold accepts any expression over
  // these slots, a temporary or an instance a fan-out builds after
  // compile, so a program bound to the compiled objects covers few
  // folds, and one keyed on a compiled expression's address covers
  // whichever composite lands there (FR-022).
  //
  // The cost this decision accepts, recorded here because the decision
  // site is where a reader looks for it (Constitution X.1). A fold
  // resolves each of its leaves through this map with one lookup at
  // `source/counters/fold.cpp:43`, and `fold_pairs_core` at `:237-238`
  // calls the fold once per committed point pair, so a pair fold over
  // `N` committed points repeats every lookup `N - 1` times. The ceiling
  // that makes it acceptable is that the fold runs off the measurement
  // path: `recorder::sample()` never reads this map, and
  // `specs/007-counters-and-timers/plan.md:31` designates
  // `recorder::sample()` and push `add()` as the critical paths. A plan
  // over `L` leaves and `N` committed points therefore costs `(N - 1) *
  // L` lookups in the untimed reporting region, linear in the size of
  // the recorded window. Slot references stored per composite would
  // remove the lookup and would bind the plan to the composites that
  // existed at compile time, which is the reading T091 rejected above.
  std::map<std::string, std::size_t> by_address;
  std::vector<read_group> groups;
  // Recorder arenas: one buffer per minted recorder, owned by the
  // plan; recorders are non-owning cursors (FR-029).
  std::vector<std::unique_ptr<std::uint64_t[]>> arenas;
  std::thread::id bound_thread = std::this_thread::get_id();
  // Calibration state (FR-032): the first accessor call measures the
  // plan's own read sequence and the distribution is kept here.
  mutable bool calibrated = false;
  mutable overhead_sample overhead;
  mutable std::vector<std::uint64_t> calibration_buffer;

  // The managed leaves, and nothing else. One sampling action writes one
  // point per leaf through the shared sink, so this is the cursor's bound
  // and the obligation `check_action` enforces (FR-047).
  [[nodiscard]] auto leaf_count() const noexcept -> std::size_t
  {
    return slots.size();
  }

  // Every managed column: the leaves and one disclosure per read group. The
  // disclosures follow the leaves because a fold resolves a leaf to a slot
  // with `by_address` and then reads that slot as a column, so a slot index
  // and a column index are the same number and the two orders cannot
  // interleave. This is the count every buffer and arena is sized against
  // (FR-002, FR-007).
  [[nodiscard]] auto column_count() const noexcept -> std::size_t
  {
    return slots.size() + groups.size();
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
  // The handle map's own lock. Resolution, listing, and the parent and
  // children walk all reach the map through `handle_for`, and the map is
  // written on a miss, so the lock covers the lookup and the insert as one
  // step: two threads naming one address both receive the same entry
  // (FR-010). No contract check guards it, because a lock does.
  std::mutex handles_lock;
  std::map<std::string, std::string> aliases;  // alias to path
  // The catalog opens lazily, on the first resolution that needs it, so no
  // separate open call exists to own the transition. The flag is atomic
  // and the store runs through a compare-and-exchange, which makes the
  // false-to-true transition happen exactly once no matter how many
  // threads resolve at the same moment (FR-011).
  std::atomic<bool> open {false};

  // The open boundary. The first caller transitions the flag; every other
  // caller finds it already set and writes nothing, so concurrent
  // resolution performs one store in total (FR-011).
  auto ensure_open() noexcept -> void
  {
    bool expected = false;
    (void)open.compare_exchange_strong(
        expected, true, std::memory_order_acq_rel);
  }

  [[nodiscard]] auto is_open() const noexcept -> bool
  {
    return open.load(std::memory_order_acquire);
  }

  [[nodiscard]] auto canonicalize(std::string_view path) const -> std::string
  {
    const auto alias = aliases.find(std::string(path));
    if (alias != aliases.end()) {
      return alias->second;
    }
    return std::string(path);
  }

  // Deliberately no const overload: every call site reaches the tree through a
  // non-const `system::impl`, so one would be dead code (X.3).
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
