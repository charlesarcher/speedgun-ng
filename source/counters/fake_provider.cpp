// The shipped fake provider: scripted deterministic point sequences
// (specs/007-counters-and-timers, FR-036, R-009).

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/core.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{
namespace
{

// One step of the seeded per-sample tail: a full-period 16-bit linear
// congruential step, `state = (state * 37 + 11) mod 2^16`. A stronger
// mixer would cost the test suite hand-reproducible tails, and this
// one already gives a spread sequence from any seed (T014).
[[nodiscard]] auto next_delta(const std::uint64_t state) noexcept
    -> std::uint64_t
{
  return (state * 37U + 11U) & 0xFFFFU;
}

}  // namespace

struct detail::fake_window final : window_reader
{
  fake_window() { set_thunk(&read_direct); }

  // The compiled plan hands the window over as the base reference
  // `read_thunk` declares, and `fake_provider::open` constructs it as
  // this final type, so the reference names a fake window on every
  // call. `final` fixes the target of the `read_points` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto read_direct(window_reader& base,
                          point_sink& sink) noexcept -> void
  {
    static_cast<fake_window&>(base).read_points(sink);
  }

  fake_provider* owner = nullptr;
  std::vector<fake_counter_data*> counters;
  std::size_t disclosure_column = leaf_set::no_disclosure_column;
  // This window's own sampling-action count, so a leaf's scripted gaps
  // are counted from the first action of the plan that samples it and not
  // from the provider's lifetime total (FR-007).
  std::uint64_t actions = 0;

  void read_points(point_sink& sink) noexcept override
  {
    owner->m_read_actions.fetch_add(1, std::memory_order_relaxed);
    ++actions;
    bool gapped = false;
    for (auto* item : counters) {
      if (item->gaps_at(actions)) {
        // The scripted action measured nothing, so the column carries a
        // zero and the disclosure beside it names the gap. The script does
        // not advance, so the next measured action reads the next point
        // (FR-007).
        gapped = true;
        sink.put(0);
        continue;
      }
      std::uint64_t value = 0;
      if (item->position < item->script.points.size()) {
        value = item->script.points[item->position];
        ++item->position;
      } else if (item->script.delta_seed.has_value()) {
        item->delta_state = next_delta(item->delta_state);
        value = item->last + item->delta_state;
      } else {
        value = item->last + item->script.tail_delta;
      }
      item->last = value;
      sink.put(value);
    }
    if (disclosure_column != leaf_set::no_disclosure_column) {
      sink.put(static_cast<std::uint64_t>(gapped ? availability::gap
                                                 : availability::countable));
    }
  }
};

fake_provider::fake_provider() = default;

fake_provider::~fake_provider() = default;

auto fake_provider::add_object(const std::string_view path,
                               const std::string_view kind,
                               const std::string_view description)
    -> fake_provider&
{
  SG_REQUIRE(!path.empty() && path != "machine",
             "add_object names a structured path that is not the machine "
             "root (FR-003)");
  auto& object = m_objects[std::string(path)];
  object.kind = std::string(kind);
  object.description = std::string(description);
  SG_ENSURE(m_objects.count(std::string(path)) > 0,
            "the declared object is enumerable (FR-003)");
  return *this;
}

auto fake_provider::add_object(const std::string_view path,
                               const std::string_view alias,
                               const std::string_view kind,
                               const std::string_view description)
    -> fake_provider&
{
  SG_REQUIRE(
      !path.empty() && !alias.empty() && path != "machine",
      "add_object names a structured path and a platform alias " "(FR-004)");
  auto& object = m_objects[std::string(path)];
  object.alias = std::string(alias);
  object.kind = std::string(kind);
  object.description = std::string(description);
  SG_ENSURE(
      m_objects.at(std::string(path)).alias == alias,
      "both spellings of the alias resolve to the declared object " "(FR-004)");
  return *this;
}

auto fake_provider::add_counter(const std::string_view object_path,
                                const std::string_view name,
                                const std::string_view unit,
                                const std::string_view description,
                                const availability avail,
                                const read_mode mode,
                                const bool ratio_pair) -> fake_provider&
{
  const std::string path(object_path);
  SG_REQUIRE(!path.empty(), "add_counter names an object path (FR-002)");
  auto& object = m_objects[path];
  if (object.kind.empty()) {
    object.kind = path == "machine" ? "machine" : "object";
    object.description = path == "machine" ? "local machine" : "";
  }
  auto& item = object.counters[std::string(name)];
  item.description = std::string(description);
  item.unit = std::string(unit);
  item.avail = avail;
  item.mode = mode;
  item.ratio_pair = ratio_pair;
  SG_ENSURE(object.counters.count(std::string(name)) > 0,
            "the declared counter is enumerable (FR-002)");
  return *this;
}

auto fake_provider::set_points(const std::string_view object_path,
                               const std::string_view name,
                               std::vector<std::uint64_t> points,
                               const std::uint64_t tail_delta,
                               const std::optional<std::uint64_t> delta_seed)
    -> fake_provider&
{
  const std::string path(object_path);
  const std::string leaf(name);
  SG_REQUIRE(
      m_objects.count(path) > 0 && m_objects.at(path).counters.count(leaf) > 0,
      "set_points scripts a declared counter (FR-036)");
  const auto scripted = points.size();
  auto& item = counter(path, leaf);
  item.script = fake_script {.points = std::move(points),
                             .tail_delta = tail_delta,
                             .delta_seed = delta_seed};
  item.position = 0;
  item.last = 0;
  // The seeded tail starts from the seed itself, so the first delta a
  // reader sees is `next_delta(seed)` (T014).
  item.delta_state = delta_seed.value_or(0);
  SG_ENSURE(counter(path, leaf).script.points.size() == scripted,
            "the scripted sequence is held in order (FR-036)");
  // The postcondition is semantic-gated, so an ignoring build emits no
  // code that reads `scripted`. The discard keeps the committed warning
  // set intact (T174).
  static_cast<void>(scripted);
  return *this;
}

void fake_provider::enumerate(object_sink& sink) const
{
  for (const auto& [path, data] : m_objects) {
    std::vector<catalog_seed> entries;
    entries.reserve(data.counters.size());
    for (const auto& [name, item] : data.counters) {
      entries.push_back(catalog_seed {
          .name = name,
          .description = item.description,
          .unit = item.unit,
          .avail = item.avail,
          .mode = item.mode,
          .has_ratio_pair = item.ratio_pair,
      });
    }
    sink.add_object(object_seed {
        .kind = data.kind,
        .path = path,
        .alias = data.alias,
        .description = data.description,
        .entries = std::move(entries),
    });
  }
}

std::unique_ptr<window_reader> fake_provider::open(const leaf_set& leaves,
                                                   const target& /*where*/)
{
  auto window = std::make_unique<detail::fake_window>();
  window->owner = this;
  window->disclosure_column = leaves.disclosure_column;
  for (const auto& address : leaves.addresses) {
    const auto [object_path, name] = split_leaf_address(address);
    const auto object = m_objects.find(std::string(object_path));
    if (object == m_objects.end()) {
      return nullptr;
    }
    const auto item = object->second.counters.find(std::string(name));
    if (item == object->second.counters.end()) {
      return nullptr;
    }
    window->counters.push_back(&item->second);
  }
  return window;
}

auto fake_provider::set_gap_actions(const std::string_view object_path,
                                    const std::string_view name,
                                    std::vector<std::size_t> actions)
    -> fake_provider&
{
  const std::string path(object_path);
  const std::string leaf(name);
  SG_REQUIRE(
      m_objects.count(path) > 0 && m_objects.at(path).counters.count(leaf) > 0,
      "set_gap_actions scripts a declared counter (FR-007)");
  auto& gaps = counter(path, leaf).gaps;
  gaps = actions;
  SG_ENSURE(gaps == actions,
            "the leaf's scripted gap actions are the ones the caller named "
            "(FR-007)");
  return *this;
}

auto fake_provider::counter(const std::string& object_path,
                            const std::string& name) -> fake_counter_data&
{
  return m_objects.at(object_path).counters.at(name);
}

}  // namespace sg::counters
