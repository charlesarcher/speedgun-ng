// Plan compilation, the plan handle, and the scope window
// (specs/007-counters-and-timers, FR-021, FR-022, FR-030).

#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <string>
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
// column order (C-PRO-2).
auto sample_row(const plan_impl& layout,
                std::uint64_t* buffer,
                const std::size_t stride,
                const std::size_t row) -> void
{
  point_sink sink(buffer, layout.leaf_count(), stride, row);
  for (const auto& group : layout.groups) {
    group.thunk(*group.reader, sink);
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

scope::scope(const plan& compiled)
{
  auto* core = new scope_core();
  core->impl = static_cast<const plan_impl*>(compiled.m_impl);
  core->buffer.assign(core->impl->leaf_count() * 2, 0);
  m_core = core;
}

scope::~scope()
{
  delete static_cast<scope_core*>(m_core);
}

void scope::start()
{
  auto* core = static_cast<scope_core*>(m_core);
  SG_REQUIRE(!core->started, "scope start runs once per scope (FR-046)");
  sample_row(*core->impl, core->buffer.data(), 2, 0);
  core->state.head = 1;
  core->started = true;
  SG_ENSURE(core->state.head == 1 && core->started,
            "the first point of the window is recorded (FR-011)");
}

void scope::finish()
{
  auto* core = static_cast<scope_core*>(m_core);
  SG_REQUIRE(core->started && !core->finished,
             "scope finish runs on a started, open window (FR-046)");
  sample_row(*core->impl, core->buffer.data(), 2, 1);
  core->state.head = 2;
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
  sample_row(*static_cast<const plan_impl*>(impl), columns, capacity, head);
  ++head;
}

auto ring_sample_core(const void* impl,
                      std::uint64_t* columns,
                      const std::size_t capacity,
                      std::size_t& head,
                      bool& wrapped,
                      std::uint64_t& dropped) noexcept -> void
{
  sample_row(*static_cast<const plan_impl*>(impl),
             columns,
             capacity,
             head & (capacity - 1));
  ++head;
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
  impl.ensure_open();
  if (exprs.empty()) {
    return std::unexpected(
        error {.message = "compile requires at least one expression (FR-021)",
               .suggestions = {}});
  }

  struct pending_leaf
  {
    std::string address;
    int provider = -1;
  };

  std::vector<pending_leaf> pending;
  std::map<std::string, std::size_t> seen;
  for (const auto* core : exprs) {
    if (core->empty()) {
      return std::unexpected(
          error {.message = "expression carries no resolved leaves (FR-021)",
                 .suggestions = {}});
    }
    for (const auto& leaf : core->leaves) {
      if (seen.contains(leaf.address)) {
        continue;
      }
      const auto [object_path, name] = split_leaf_address(leaf.address);
      const leaf_record* record = nullptr;
      if (const auto* node = impl.find(object_path); node != nullptr) {
        for (const auto& candidate : node->leaves) {
          if (candidate.core.name == name) {
            record = &candidate;
            break;
          }
        }
      }
      if (record == nullptr) {
        return std::unexpected(
            error {.message = "leaf '" + leaf.address
                       + "' is not in the system tree (FR-017)",
                   .suggestions = {}});
      }
      seen.emplace(leaf.address, pending.size());
      pending.push_back(pending_leaf {.address = leaf.address,
                                      .provider = record->provider_index});
    }
  }

  auto layout = std::make_unique<plan_impl>();
  layout->bound_target = tg;
  for (std::size_t p = 0; p < impl.providers.size(); ++p) {
    std::vector<std::string> addresses;
    for (const auto& one : pending) {
      if (one.provider == static_cast<int>(p)) {
        addresses.push_back(one.address);
      }
    }
    if (addresses.empty()) {
      continue;
    }
    read_group group;
    group.offset = layout->slots.size();
    group.count = addresses.size();
    for (const auto& address : addresses) {
      const auto [object_path, name] = split_leaf_address(address);
      const auto* node = impl.find(object_path);
      for (const auto& candidate : node->leaves) {
        if (candidate.core.name == name) {
          layout->slots.push_back(
              plan_impl::slot {.core = candidate.core,
                               .has_ratio_pair = candidate.has_ratio_pair});
        }
      }
      layout->by_address.emplace(address, layout->slots.size() - 1);
    }
    auto reader =
        impl.providers[p]->open(leaf_set {.addresses = addresses}, tg);
    if (reader == nullptr) {
      return std::unexpected(error {
          .message = "provider cannot open a window for its leaves (FR-011)",
          .suggestions = {}});
    }
    group.thunk = reader->resolve_thunk();
    group.reader = std::move(reader);
    layout->groups.push_back(std::move(group));
  }
  if (layout->slots.size() != pending.size()) {
    return std::unexpected(error {
        .message = "leaf has no owning provider (FR-011)", .suggestions = {}});
  }
  return plan(layout.release());
}

}  // namespace detail

}  // namespace sg::counters
