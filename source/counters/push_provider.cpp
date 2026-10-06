// The shipped push provider: user hot-path counters, confined to the
// creating thread, sampled by plain load
// (specs/007-counters-and-timers, FR-035, R-008).

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_push.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{

struct detail::push_window final : window_reader
{
  push_window() { set_thunk(&read_direct); }

  // The compiled plan hands the window over as the base reference
  // `read_thunk` declares, and `push_provider::open` constructs it as
  // this final type, so the reference names a push window on every
  // call. `final` fixes the target of the `read_points` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto read_direct(window_reader& base,
                          point_sink& sink) noexcept -> void
  {
    static_cast<push_window&>(base).read_points(sink);
  }

  std::vector<const std::uint64_t*> cells;
  std::thread::id owner {};
  std::size_t disclosure_column = leaf_set::no_disclosure_column;

  void read_points(point_sink& sink) noexcept override
  {
    SG_REQUIRE(
        std::this_thread::get_id() == owner,
        "push counters are sampled on the thread that created " "them "
                                                                "(FR-035)");
    for (const auto* cell : cells) {
      sink.put(*cell);  // plain load, never an atomic RMW (R-008)
    }
    // A push counter is a plain load of a cell its own thread owns, so
    // the action always measures and the disclosure names the entry's own
    // countability value (FR-007).
    if (disclosure_column != leaf_set::no_disclosure_column) {
      sink.put_disclosure(disclosure_column,
                          static_cast<std::uint64_t>(availability::countable));
    }
  }
};

push_provider::push_provider() = default;

push_provider::~push_provider() = default;

auto push_provider::add_counter(const std::string_view name,
                                const std::string_view unit,
                                const std::string_view description)
    -> push_counter
{
  SG_REQUIRE(!name.empty(), "add_counter names a counter (FR-035)");
  m_points.emplace_back(push_point {
      .value = 0,
      .owner = std::this_thread::get_id(),
      .name = std::string(name),
      .description = std::string(description),
      .unit = std::string(unit),
  });
  auto& point = m_points.back();
  SG_ENSURE(m_points.back().name == name,
            "the declared counter is enumerable (FR-035)");
  return push_counter(&point.value, point.owner, point.name);
}

void push_provider::enumerate(object_sink& sink) const
{
  std::vector<catalog_seed> entries;
  entries.reserve(m_points.size());
  for (const auto& point : m_points) {
    entries.push_back(catalog_seed {
        .name = point.name,
        .description = point.description,
        .unit = point.unit,
        .avail = availability::countable,
        .mode = read_mode::push_load,
    });
  }
  sink.add_object(object_seed {
      .kind = "machine",
      .path = "machine",
      .alias = {},
      .description = "local machine",
      .entries = std::move(entries),
  });
}

std::unique_ptr<window_reader> push_provider::open(const leaf_set& leaves,
                                                   const target& /*where*/)
{
  auto window = std::make_unique<detail::push_window>();
  window->disclosure_column = leaves.disclosure_column;
  window->cells.reserve(leaves.addresses.size());
  for (const auto& address : leaves.addresses) {
    const push_point* match = nullptr;
    for (const auto& point : m_points) {
      if (address == "machine/" + point.name) {
        match = &point;
        break;
      }
    }
    if (match == nullptr) {
      return nullptr;
    }
    // Every cell in one window shares the thread that created it, so
    // one owner per window decides the `read_points` guard for all of
    // them, and a per-cell owner array would add a load and a compare
    // per cell to the sampling path `plan.md` designates critical for
    // nothing. A leaf set carrying two owners has no safe sampling
    // thread at all: `sample_point` binds a plan to the thread that
    // compiled it (FR-031) and FR-035 confines a read to the owning
    // thread, so the check belongs here in the untimed open rather
    // than in the read (FR-035, FR-026, T267).
    SG_REQUIRE(window->cells.empty() || match->owner == window->owner,
               "one push window holds counters from one thread (FR-035)");
    window->cells.push_back(&match->value);
    window->owner = match->owner;
  }
  return window;
}

}  // namespace sg::counters
