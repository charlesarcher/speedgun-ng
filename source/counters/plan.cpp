// Plan compilation, the plan handle, and the scope window
// (specs/007-counters-and-timers, FR-021, FR-022, FR-030).

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "detail/core.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{
namespace
{

// One sampling action across every read group: fill row `row` of the
// point buffer, `stride` rows reserved per column. Groups write
// contiguous slot ranges through one shared sink, so call order equals
// column order (C-PRO-2). The closing check is the semantic-gated
// kind, so a release build pays nothing for it.
auto sample_row(const plan_impl& layout,
                std::uint64_t* buffer,
                const std::size_t stride,
                const std::size_t row) -> void
{
  point_sink sink(buffer, layout.leaf_count(), stride, row);
  for (const auto& group : layout.groups) {
    group.thunk(*group.reader, sink);
  }
  sink.check_action();
}

// One sampling action: the thread a plan bound to, the read sequence,
// and the head advance. The scope, the hard-stop recorder, and the ring
// recorder all call this, which is what makes FR-030's one semantics an
// implementation fact. The two overflow policies keep one core each,
// because FR-027's always-enforced bounds check and FR-028's branchless
// mask are mutually exclusive, and `recorder_handle<P>::sample()` picks
// between them at compile time (FR-026, FR-027, FR-028, FR-031). The
// cached `bound_thread` names the one allowed thread; the current
// thread's identity is read per call, because caching it would cache
// the answer for the thread that cached it.
auto sample_point(const plan_impl& layout,
                  std::uint64_t* columns,
                  const std::size_t stride,
                  const std::size_t row,
                  std::size_t& head) noexcept -> void
{
  SG_REQUIRE(std::this_thread::get_id() == layout.bound_thread,
             "a sample_point runs on the thread its plan bound to (FR-031)");
  sample_row(layout, columns, stride, row);
  ++head;
}

// Fan-out instantiation: the exemplar spine re-homed under `path` by
// re-addressing every leaf (US3 scenario 5); the fold layer resolves
// the instances through the plan's address map.
auto instantiate_core(const detail::expr_core& core,
                      const std::string& path) -> detail::expr_core
{
  auto out = core;
  for (auto& leaf : out.leaves) {
    leaf.address = path + "/" + leaf.name;
  }
  return out;
}  // LCOV_EXCL_LINE

// The one object path shared by every spine leaf; empty for an empty
// spine or a spine spanning several objects.
auto exemplar_prefix(const detail::expr_core& core) -> std::string
{
  // LCOV_EXCL_BR_START : coverage exclusion (T066): both guards are
  // unreachable. A zero-leaf expression is refused by `compile_core`
  // (`plan.cpp:417`), and for a fan-out by `compile_fanout_core`
  // (`plan.cpp:536`). Every address reaching here was written by
  // `instantiate_core` as `path + "/" + name`: a separator is always present.
  if (core.leaves.empty()) {  // LCOV_EXCL_BR_LINE
    return {};  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  const auto& address = core.leaves.front().address;
  // LCOV_EXCL_BR_START : coverage exclusion (T066): as above, the address
  // carries the separator `instantiate_core` wrote.
  const auto slash = address.rfind('/');
  if (slash == std::string::npos) {  // LCOV_EXCL_BR_LINE
    return {};  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  const std::string prefix = address.substr(0, slash);
  const std::string home = prefix + "/";
  for (const auto& leaf : core.leaves) {
    if (leaf.address.compare(0, home.size(), home) != 0) {
      return {};
    }
  }
  return prefix;
}

auto availability_name(const availability state) -> std::string_view
{
  // LCOV_EXCL_START : coverage exclusion (T066): the `countable` arm.
  // `availability_name` runs only on the construction-failure path at
  // `plan.cpp:456`, and a leaf the catalog reports as `countable` never
  // takes it.
  switch (state) {
    case availability::countable:  // LCOV_EXCL_LINE
      return "countable";  // LCOV_EXCL_LINE
    case availability::permission_blocked:
      return "permission_blocked";
    case availability::not_encodable:
      return "not_encodable";
    case availability::absent:
      return "absent";
    case availability::scope_refused:
      return "scope_refused";
    case availability::gap:
      return "gap";
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): the defensive close of a
  // closed enumeration, reachable only by casting an out-of-range integer
  // to `availability`. `availability_name` is file-local, so no test can
  // hand it such a value.
  return "outside the closed enumeration";  // LCOV_EXCL_LINE
}  // LCOV_EXCL_STOP

// Resolves each slot's enabled/running partners once every slot exists,
// so a fold reads the multiplex pair by index and never by name lookup
// (FR-019, FR-020, FR-022).
void link_ratio_slots(plan_impl& layout)
{
  for (std::size_t index = 0; index < layout.slots.size(); ++index) {
    auto& slot = layout.slots[index];
    if (!slot.has_ratio_pair) {
      continue;
    }
    // LCOV_EXCL_BR_START : coverage exclusion (T066): a slot address is
    // written by `instantiate_core` or by resolution from the frozen tree,
    // and both spell it `object + "/" + name`, so the separator is present.
    const auto slash = slot.core.address.rfind('/');
    if (slash == std::string::npos) {  // LCOV_EXCL_BR_LINE
      continue;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_BR_STOP
    const std::string home = slot.core.address.substr(0, slash);
    for (const auto& [name, slot_index] : layout.by_address) {
      if (name == home + "/enabled") {
        slot.ratio_enabled = slot_index;
      } else if (name == home + "/running") {
        slot.ratio_running = slot_index;
      }
    }
  }
}

}  // namespace

plan::plan(plan&& other) noexcept
    : m_impl(other.m_impl)
{
  other.m_impl = nullptr;
  SG_ENSURE(other.m_impl == nullptr,
            "the moved-from plan holds no layout (FR-022)");
}

auto plan::operator=(plan&& other) noexcept -> plan&
{
  if (this != &other) {
    delete static_cast<plan_impl*>(m_impl);
    m_impl = other.m_impl;
    other.m_impl = nullptr;
  }
  return *this;
}

plan::~plan()
{
  delete static_cast<plan_impl*>(m_impl);
}

fanout_plan::fanout_plan(fanout_plan&& other) noexcept
    : m_impl(other.m_impl)
{
  other.m_impl = nullptr;
  SG_ENSURE(other.m_impl == nullptr,
            "the moved-from fan-out plan holds no layout (FR-022)");
}

auto fanout_plan::operator=(fanout_plan&& other) noexcept -> fanout_plan&
{
  if (this != &other) {
    delete static_cast<fanout_impl*>(m_impl);
    m_impl = other.m_impl;
    other.m_impl = nullptr;
  }
  return *this;
}

fanout_plan::~fanout_plan()
{
  delete static_cast<fanout_impl*>(m_impl);
}

auto fanout_plan::recorder(const std::size_t capacity) const
    -> recorder_handle<hard_stop_t>
{
  return static_cast<const fanout_impl*>(m_impl)->inner->recorder(capacity);
}

auto fanout_plan::object_paths() const -> std::vector<std::string>
{
  return static_cast<const fanout_impl*>(m_impl)->paths;
}

auto plan::recorder(const std::size_t capacity) const
    -> recorder_handle<hard_stop_t>
{
  auto& impl = *static_cast<plan_impl*>(m_impl);
  auto arena = std::make_unique<std::uint64_t[]>(capacity * impl.leaf_count());
  auto* columns = arena.get();
  impl.arenas.push_back(std::move(arena));
  return recorder_handle<hard_stop_t> {
      .m_impl = m_impl, .m_columns = columns, .m_capacity = capacity};
}

auto plan::recorder(const std::size_t capacity, const ring_t) const
    -> std::expected<recorder_handle<ring_t>, error>
{
  if (capacity == 0 || (capacity & (capacity - 1)) != 0) {
    return std::unexpected(error {
        .message = "ring capacity must be a non-zero power of two (FR-025)",
        .suggestions = {}});
  }
  auto& impl = *static_cast<plan_impl*>(m_impl);
  auto arena = std::make_unique<std::uint64_t[]>(capacity * impl.leaf_count());
  auto* columns = arena.get();
  impl.arenas.push_back(std::move(arena));
  return recorder_handle<ring_t> {
      .m_impl = m_impl, .m_columns = columns, .m_capacity = capacity};
}

namespace
{

// Calibration sample counts: the warm-up fills the caches and lets the
// clock settle, the measured run is long enough for a median.
constexpr int kCalibrationWarmup = 64;
constexpr int kCalibrationSamples = 257;

}  // namespace

// Runs the calibration on first request (FR-032): the plan's own read
// sequence over an empty workload, timed one action at a time. The
// scratch buffer is the plan's own, so no recorder observes it.
static auto calibrate(plan_impl& layout) -> const overhead_sample&
{
  if (layout.calibrated) {
    return layout.overhead;
  }
  constexpr std::size_t rows = 2;
  layout.calibration_buffer.assign(layout.leaf_count() * rows, 0);
  auto* columns = layout.calibration_buffer.data();
  std::size_t head = 0;
  for (int warm = 0; warm < kCalibrationWarmup; ++warm) {
    sample_row(layout, columns, rows, head & (rows - 1));
    ++head;
  }
  std::vector<double> costs;
  costs.reserve(static_cast<std::size_t>(kCalibrationSamples));
  for (int index = 0; index < kCalibrationSamples; ++index) {
    const auto before = std::chrono::steady_clock::now();
    sample_row(layout, columns, rows, head & (rows - 1));
    ++head;
    const auto after = std::chrono::steady_clock::now();
    costs.push_back(
        std::chrono::duration<double, std::nano>(after - before).count());
  }
  // The bracket's own cost, measured under the identical bracketing: two
  // clock reads with no sampling action between them. The published floor
  // excludes it, so a caller reads what one sampling action costs and not
  // what timing that action costs (FR-025).
  std::vector<double> brackets;
  brackets.reserve(static_cast<std::size_t>(kCalibrationSamples));
  for (int index = 0; index < kCalibrationSamples; ++index) {
    const auto before = std::chrono::steady_clock::now();
    const auto after = std::chrono::steady_clock::now();
    brackets.push_back(
        std::chrono::duration<double, std::nano>(after - before).count());
  }
  std::sort(brackets.begin(), brackets.end());
  const double bracket = brackets[brackets.size() / 2];
  // A host whose clock costs more than the action publishes a floor of
  // zero. The subtraction clamps there, and the published duration is
  // never negative (FR-025).
  for (auto& cost : costs) {
    cost = std::max(0.0, cost - bracket);
  }
  std::sort(costs.begin(), costs.end());
  layout.overhead.min_ns = costs.front();
  layout.overhead.median_ns = costs[costs.size() / 2];
  layout.overhead.max_ns = costs.back();
  layout.calibrated = true;
  return layout.overhead;
}

auto plan::sample_overhead_ns_min() const -> double
{
  const overhead_sample& cost = calibrate(*static_cast<plan_impl*>(m_impl));
  SG_ENSURE(cost.min_ns >= 0.0, "the calibrated minimum is a real duration");
  return cost.min_ns;
}

auto plan::sample_overhead_ns_median() const -> double
{
  const overhead_sample& cost = calibrate(*static_cast<plan_impl*>(m_impl));
  SG_ENSURE(cost.median_ns >= 0.0, "the calibrated median is a real duration");
  return cost.median_ns;
}

auto plan::sample_overhead_ns_max() const -> double
{
  const overhead_sample& cost = calibrate(*static_cast<plan_impl*>(m_impl));
  SG_ENSURE(cost.max_ns >= cost.min_ns,
            "the dearest sampled action is at least the cheapest");
  return cost.max_ns;
}

scope::scope(const plan& compiled)
{
  auto* core = new scope_core();
  core->impl = static_cast<const plan_impl*>(compiled.m_impl);
  core->buffer.assign(core->impl->leaf_count() * 2, 0);
  m_core = core;
  SG_ENSURE(!core->started && !core->finished,
            "a fresh scope awaits start() (FR-030)");
}

scope::~scope()
{
  delete static_cast<scope_core*>(m_core);
}

void scope::start()
{
  auto* core = static_cast<scope_core*>(m_core);
  SG_REQUIRE(!core->started, "scope start runs once per scope (FR-046)");
  // The window is a two-point hard-stop recorder, so its points are
  // sampled by the recorder's own core, capacity check included
  // (FR-027, FR-030).
  detail::hard_stop_sample_core(
      core->impl, core->buffer.data(), 2, core->state.head);
  core->started = true;
  SG_ENSURE(core->state.head == 1 && core->started,
            "the first point of the window is recorded (FR-011)");
}

void scope::finish()
{
  auto* core = static_cast<scope_core*>(m_core);
  SG_REQUIRE(core->started && !core->finished,
             "scope finish runs on a started, open window (FR-046)");
  detail::hard_stop_sample_core(
      core->impl, core->buffer.data(), 2, core->state.head);
  core->finished = true;
  SG_ENSURE(core->state.head == 2 && core->finished,
            "the window is closed with two recorded points (FR-011)");
}

auto scope::view() const noexcept -> recorder_api
{
  return static_cast<const scope_core*>(m_core)->view();
}

namespace detail
{

auto hard_stop_sample_core(const void* impl,
                           std::uint64_t* columns,
                           const std::size_t capacity,
                           std::size_t& head) noexcept -> void
{
  SG_REQUIRE_ALWAYS(head < capacity,
                    "hard_stop recorder samples within capacity (FR-027)");
  const auto& layout = *static_cast<const plan_impl*>(impl);
  sample_point(layout, columns, capacity, head, head);
}

auto ring_sample_core(const void* impl,
                      std::uint64_t* columns,
                      const std::size_t capacity,
                      std::size_t& head,
                      bool& wrapped,
                      std::uint64_t& dropped) noexcept -> void
{
  const auto& layout = *static_cast<const plan_impl*>(impl);
  sample_point(layout, columns, capacity, head & (capacity - 1), head);
  if (head > capacity) {
    wrapped = true;
    ++dropped;
  }
}

auto compile_core(const system& sys,
                  const target& tg,
                  const std::vector<const expr_core*>& exprs)
    -> std::expected<plan, error>
{
  auto& impl = *sys.m_impl;
  // The open flag is not written here: the open boundary sets it once,
  // and a compile writing it would race every other compile on a flag no
  // compile reads (FR-011).
  SG_REQUIRE(impl.is_open(),
             "a plan compiles only after the catalog is open (FR-011)");
  if (exprs.empty()) {
    return std::unexpected(
        error {.message = "compile requires at least one expression (FR-021)",
               .suggestions = {}});
  }

  struct pending_leaf
  {
    std::string address;
    // The record the loop above matched, carried into the provider
    // grouping so no second tree lookup runs there. The catalog is
    // frozen once opened and nothing between the loops reseats a
    // node's leaf vector, so the pointer holds across the boundary.
    const leaf_record* record = nullptr;
  };

  std::vector<pending_leaf> pending;
  std::map<std::string, std::size_t> seen;
  for (const auto* core : exprs) {
    // The leaf vector, because a scalar multiple adds its scale node
    // unconditionally: a scaled zero-leaf spine holds one node over
    // nothing, and the spec edge case makes that a recoverable
    // construction error (specs/007-counters-and-timers, FR-046).
    if (core->leaves.empty()) {
      return std::unexpected(
          error {.message = "expression carries no resolved leaves (FR-046)",
                 .suggestions = {}});
    }
    for (const auto& leaf : core->leaves) {
      if (seen.contains(leaf.address)) {
        continue;
      }
      const auto [object_path, name] = split_leaf_address(leaf.address);
      const leaf_record* record = nullptr;
      // LCOV_EXCL_BR_START : coverage exclusion (T066): the null side. Every
      // leaf address reaching `compile_core` came from a resolved handle or
      // from `instantiate_core`, and the object it names is in the frozen
      // tree, so the lookup never misses.
      if (const auto* node = impl.find(object_path); node != nullptr)
      {  // LCOV_EXCL_BR_LINE
        for (const auto& candidate : node->leaves) {
          if (candidate.core.name == name) {
            record = &candidate;
            break;
          }
        }
      }  // LCOV_EXCL_BR_LINE
      // LCOV_EXCL_BR_STOP
      if (record == nullptr) {
        return std::unexpected(
            error {.message = "leaf '" + leaf.address
                       + "' is not in the system tree (FR-017)",
                   .suggestions = {}});
      }
      // A leaf the catalog reports as not countable now is a
      // construction error naming the catalog state, refused in the
      // untimed region before any provider window opens and before any
      // hardware read (FR-021, FR-024, FR-046).
      if (record->core.avail != availability::countable) {
        std::string message = "counter '" + leaf.address
                              + "' is not countable on this host: the "
                                "catalog reports ";
        message += availability_name(record->core.avail);
        message += "; pick a countable counter or branch on the catalog "
                   "state before composing (FR-024)";
        return std::unexpected(
            error {.message = std::move(message), .suggestions = {}});
      }
      seen.emplace(leaf.address, pending.size());
      pending.push_back(
          pending_leaf {.address = leaf.address, .record = record});
    }
  }

  auto layout = std::make_unique<plan_impl>();
  layout->bound_target = tg;
  // One read group per provider, carrying every leaf that provider
  // owns, so a fan-out over many objects is one `open` and one sampling
  // action (FR-047). The instance is provider-internal, the PMU window
  // issuing one group read per leader from inside its own reader
  // (`source/counters/linux_pmu/group_io.cpp:216-239`); a per-instance
  // split would multiply the setup cost and add one indirect call per
  // instance to every sample (T098).
  // Two passes over the providers: the first lays out every slot, so the
  // second knows which group's leaves end at the plan's last managed
  // column and can name the disclosure column to the one window that
  // writes it (FR-007).
  std::vector<std::vector<std::string>> per_provider(impl.providers.size());
  std::vector<std::size_t> group_of(impl.providers.size(),
                                    static_cast<std::size_t>(-1));
  for (std::size_t p = 0; p < impl.providers.size(); ++p) {
    const auto provider = static_cast<int>(p);
    for (const auto& one : pending) {
      if (one.record->provider_index == provider) {
        per_provider[p].push_back(one.address);
      }
    }
    if (per_provider[p].empty()) {
      continue;
    }
    group_of[p] = layout->groups.size();
    read_group group;
    group.offset = layout->slots.size();
    group.count = per_provider[p].size();
    for (const auto& one : pending) {
      if (one.record->provider_index != provider) {
        continue;
      }
      const auto& candidate = *one.record;
      layout->slots.push_back(plan_impl::slot {
          .core = candidate.core, .has_ratio_pair = candidate.has_ratio_pair});
      layout->by_address.emplace(one.address, layout->slots.size() - 1);
    }
    layout->groups.push_back(std::move(group));
  }  // LCOV_EXCL_LINE
  // The disclosure column sits past the last managed leaf, so the
  // group that owns the plan's last leaf is the one that writes it.
  layout->disclosure_slot = layout->slots.size();
  for (std::size_t p = 0; p < impl.providers.size(); ++p) {
    if (per_provider[p].empty()) {
      continue;
    }
    auto& group = layout->groups[group_of[p]];
    const bool last = group.offset + group.count == layout->slots.size();
    auto reader = impl.providers[p]->open(
        leaf_set {.addresses = std::move(per_provider[p]),
                  .disclosure_column = last ? layout->disclosure_slot
                                            : leaf_set::no_disclosure_column},
        tg);
    // LCOV_EXCL_BR_START : coverage exclusion (T140): the open refusal. It
    // needs a provider that declines a window for leaves its own catalog
    // already reported countable, so the refusal follows a granted
    // `perf_event_open` the provider then does not turn into a window. A
    // runner whose `perf_event_open` is refused refuses every leaf earlier,
    // at the not-countable check above, and never opens here; the host that
    // grants the syscall reaches this arm when the open it granted cannot be
    // turned into a window.
    if (reader == nullptr) {  // LCOV_EXCL_BR_LINE
      // LCOV_EXCL_START : coverage exclusion (T140): the refusal itself.
      return std::unexpected(error {
          .message = "provider cannot open a window for its leaves (FR-011)",
          .suggestions = {}});
      // LCOV_EXCL_STOP
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    group.thunk = reader->resolve_thunk();
    group.reader = std::move(reader);
  }
  // LCOV_EXCL_LINE : coverage exclusion (T140): the block gcov attributes to
  // the loop's closing brace, marked on the brace above. A runner whose
  // `perf_event_open` is refused opens no provider window, so its provider
  // loop hands every group over through the push above and the brace block
  // stays at zero; the host that grants the syscall reaches it once per
  // provider it iterates.
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the two counts always
  // agree. Each pending entry carries the record its leaf bound to at
  // `plan.cpp:436`, and the refusal at `plan.cpp:442` rules a null one out.
  // The partition at `plan.cpp:481` and `plan.cpp:492` gives every entry
  // exactly one group, so the push at `plan.cpp:496` lands once per entry.
  if (layout->slots.size() != pending.size()) {  // LCOV_EXCL_BR_LINE
    return std::unexpected(  // LCOV_EXCL_LINE
        error {// LCOV_EXCL_LINE
               .message =
                   "leaf has no owning provider (FR-011)",  // LCOV_EXCL_LINE
               .suggestions = {}});  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_STOP
  link_ratio_slots(*layout);
  return plan(layout.release());
}

auto compile_fanout_core(const system& sys,
                         const target& tg,
                         const expr_core& exemplar,
                         const std::vector<const object*>& selection)
    -> std::expected<fanout_plan, error>
{
  // The leaf vector, for the reason `compile_core` gives above: a
  // scalar multiple adds its scale node over an empty spine, so a
  // scaled zero-leaf exemplar holds one node over nothing.
  if (exemplar.leaves.empty()) {
    return std::unexpected(
        error {.message = "fan-out exemplar carries no leaves (FR-024)",
               .suggestions = {}});
  }
  if (selection.empty()) {
    return std::unexpected(
        error {.message = "fan-out needs a non-empty selection (FR-024)",
               .suggestions = {}});
  }
  if (exemplar_prefix(exemplar).empty()) {
    return std::unexpected(
        error {.message = "fan-out exemplar spans several objects (FR-024)",
               .suggestions = {}});
  }
  std::vector<const expr_core*> instantiated;
  std::vector<expr_core> instances;
  std::vector<std::string> paths;
  for (const object* selected : selection) {
    if (selected == nullptr) {
      return std::unexpected(
          error {.message = "fan-out selection holds a null object (FR-024)",
                 .suggestions = {}});
    }
    const std::string path(selected->path());
    if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
      return std::unexpected(error {
          .message = "fan-out selection duplicates '" + path + "' (FR-024)",
          .suggestions = {}});
    }
    paths.push_back(path);
    instances.push_back(instantiate_core(exemplar, path));
  }
  for (const auto& instance : instances) {
    instantiated.push_back(&instance);
  }
  auto inner = compile_core(sys, tg, instantiated);
  if (!inner.has_value()) {
    return std::unexpected(inner.error());
  }
  auto impl = std::make_unique<fanout_impl>();
  impl->inner = std::make_unique<plan>(std::move(*inner));
  impl->paths = std::move(paths);
  return fanout_plan(impl.release());
}

auto fanout_fold_core(const void* fanout,
                      const expr_core& core,
                      const recorder_api& rec) -> std::vector<fanout_result>
{
  const auto& impl = *static_cast<const fanout_impl*>(fanout);
  std::vector<fanout_result> out;
  out.reserve(impl.paths.size());
  for (const auto& path : impl.paths) {
    out.push_back(fanout_result {
        .object_path = path,
        .metric =
            fold_core(instantiate_core(core, path), rec, 0, rec.count - 1),
    });
  }
  return out;
}  // LCOV_EXCL_LINE

}  // namespace detail

}  // namespace sg::counters
