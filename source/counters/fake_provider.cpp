// The shipped fake provider: scripted deterministic point sequences
// (specs/007-counters-and-timers, FR-036, R-009).

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
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
[[nodiscard]] auto nextDelta(const std::uint64_t state) noexcept
    -> std::uint64_t
{
  return (state * 37U + 11U) & 0xFFFFU;
}

// The playback state a window hands on when it samples: position, last value
// and seeded-tail state of each leaf, read by the next window that opens over
// that leaf so a family's later instances resume the script where its earlier
// ones stopped (FR-030). A window folds from a copy of this state and never
// from the shared one, so the fold of a threaded context reads that context's
// plan alone (FR-018). Only the hand-off and the seeding take this lock, and
// they move the three fields together because a half-updated seed would hand
// the next window a position and a value from different points of the script.
// File scope by necessity: FakeProvider is exported with no pimpl, so a
// member would change the layout of a published type.
[[nodiscard]] auto scriptLock() noexcept -> std::mutex&
{
  static std::mutex lock;
  return lock;
}

}  // namespace

struct detail::FakeWindow final : WindowReader
{
  FakeWindow() { setThunk(&readDirect); }

  // The compiled plan hands the window over as the base reference
  // `ReadThunk` declares, and `FakeProvider::open` constructs it as
  // this final type, so the reference names a fake window on every
  // call. `final` fixes the target of the `readPoints` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto readDirect(WindowReader& base, PointSink& sink) noexcept -> void
  {
    static_cast<FakeWindow&>(base).readPoints(sink);
  }

  FakeProvider* owner = nullptr;

  // One scripted playback cursor per sampled leaf of this window. FR-018 of
  // specs/020 requires every fold of a context's row to read that context's
  // plan alone, so playback cannot be shared between the windows of a
  // threaded run: a shared cursor let one context's fold depend on how many
  // times the other contexts had sampled. A window starts at the position its
  // leaf carries, which is the playback every single-window run already saw.
  struct Playback
  {
    std::size_t position = 0;
    std::uint64_t last = 0;
    std::uint64_t deltaState = 0;
  };

  struct Sampled
  {
    FakeCounterData* data = nullptr;
    Playback playback;
  };

  std::vector<Sampled> leaves;
  std::size_t disclosureColumn = LeafSet::kNoDisclosureColumn;
  // This window's own sampling-action count, so a leaf's scripted gaps
  // are counted from the first action of the plan that samples it and not
  // from the provider's lifetime total (FR-007).
  std::uint64_t actions = 0;

  void readPoints(PointSink& sink) noexcept override
  {
    owner->m_readActions.fetch_add(1, std::memory_order_relaxed);
    ++actions;
    bool gapped = false;
    for (auto& leaf : leaves) {
      auto* item = leaf.data;
      auto& played = leaf.playback;
      if (item->gapsAt(actions)) {
        // The scripted action measured nothing, so the column carries a
        // zero and the disclosure beside it names the gap. The script does
        // not advance, so the next measured action reads the next point
        // (FR-007).
        gapped = true;
        sink.put(0);
        continue;
      }
      std::uint64_t value = 0;
      if (played.position < item->script.points.size()) {
        value = item->script.points[played.position];
        ++played.position;
      } else if (item->script.deltaSeed.has_value()) {
        played.deltaState = nextDelta(played.deltaState);
        value = played.last + played.deltaState;
      } else {
        value = played.last + item->script.tailDelta;
      }
      played.last = value;
      sink.put(value);
    }
    if (disclosureColumn != LeafSet::kNoDisclosureColumn) {
      sink.putDisclosure(
          disclosureColumn,
          static_cast<std::uint64_t>(gapped ? Availability::GAP
                                            : Availability::COUNTABLE));
    }
    // Hand this window's playback on for whichever window opens next
    // (FR-030). Because the fold above read this window's own cursor, the
    // windows of a threaded run fold independently and the order in which
    // their hand-offs land changes nothing a row reports (FR-018).
    const std::scoped_lock guard {scriptLock()};
    for (auto& leaf : leaves) {
      leaf.data->position = leaf.playback.position;
      leaf.data->last = leaf.playback.last;
      leaf.data->deltaState = leaf.playback.deltaState;
    }
  }
};

FakeProvider::FakeProvider() = default;

FakeProvider::~FakeProvider() = default;

auto FakeProvider::addObject(const std::string_view path,
                             const std::string_view kind,
                             const std::string_view description)
    -> FakeProvider&
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

auto FakeProvider::addObject(const std::string_view path,
                             const std::string_view alias,
                             const std::string_view kind,
                             const std::string_view description)
    -> FakeProvider&
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

auto FakeProvider::setProgrammableCounters(
    const std::string_view objectPath, const std::size_t limit) -> FakeProvider&
{
  const std::string path(objectPath);
  SG_REQUIRE(limit > 0 && m_objects.count(path) > 0,
             "set_programmable_counters scripts a positive count for an "
             "object the fake owns (FR-006)");
  auto& object = m_objects.at(path);
  object.programmableCounters = limit;
  SG_ENSURE(object.programmableCounters.value_or(0) == limit,
            "the scripted limit is the one enumeration publishes (FR-006)");
  return *this;
}

auto FakeProvider::setGroupLeafLimit(const std::string_view objectPath,
                                     const std::size_t leaves) -> FakeProvider&
{
  const std::string path(objectPath);
  SG_REQUIRE(m_objects.count(path) > 0,
             "set_group_leaf_limit scripts a count for an object the fake "
             "owns (D-18, PD-09)");
  auto& object = m_objects.at(path);
  object.groupLeafLimit = leaves;
  SG_ENSURE(object.groupLeafLimit.value_or(0) == leaves,
            "the scripted count is the one open refuses a wider set by "
            "(D-18, PD-09)");
  return *this;
}

auto FakeProvider::addCounter(const std::string_view objectPath,
                              const std::string_view name,
                              const std::string_view unit,
                              const std::string_view description,
                              const Availability avail,
                              const ReadMode mode,
                              const bool ratioPair) -> FakeProvider&
{
  const std::string path(objectPath);
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
  item.ratioPair = ratioPair;
  SG_ENSURE(object.counters.count(std::string(name)) > 0,
            "the declared counter is enumerable (FR-002)");
  return *this;
}

auto FakeProvider::setPoints(const std::string_view objectPath,
                             const std::string_view name,
                             std::vector<std::uint64_t> points,
                             const std::uint64_t tailDelta,
                             const std::optional<std::uint64_t> deltaSeed)
    -> FakeProvider&
{
  const std::string path(objectPath);
  const std::string leaf(name);
  SG_REQUIRE(
      m_objects.count(path) > 0 && m_objects.at(path).counters.count(leaf) > 0,
      "set_points scripts a declared counter (FR-036)");
  const auto scripted = points.size();
  auto& item = counter(path, leaf);
  item.script = FakeScript {.points = std::move(points),
                            .tailDelta = tailDelta,
                            .deltaSeed = deltaSeed};
  item.position = 0;
  item.last = 0;
  // The seeded tail starts from the seed itself, so the first delta a
  // reader sees is `next_delta(seed)` (T014).
  item.deltaState = deltaSeed.value_or(0);
  SG_ENSURE(counter(path, leaf).script.points.size() == scripted,
            "the scripted sequence is held in order (FR-036)");
  // The postcondition is semantic-gated, so an ignoring build emits no
  // code that reads `scripted`. The discard keeps the committed warning
  // set intact (T174).
  static_cast<void>(scripted);
  return *this;
}

void FakeProvider::enumerate(ObjectSink& sink) const
{
  for (const auto& [path, data] : m_objects) {
    std::vector<CatalogSeed> entries;
    entries.reserve(data.counters.size());
    for (const auto& [name, item] : data.counters) {
      entries.push_back(CatalogSeed {
          .name = name,
          .description = item.description,
          .unit = item.unit,
          .avail = item.avail,
          .mode = item.mode,
          .hasRatioPair = item.ratioPair,
      });
    }
    sink.addObject(ObjectSeed {
        .kind = data.kind,
        .path = path,
        .alias = data.alias,
        .description = data.description,
        .entries = std::move(entries),
        .programmableCounters = data.programmableCounters,
    });
  }
}

std::unique_ptr<WindowReader> FakeProvider::open(const LeafSet& leaves,
                                                 const Target& /*where*/)
{
  // The scripted group refusal: a leaf set that carries more leaves of a
  // scripted object than its scripted count opens no window, and the
  // message names the object path and the leaf count the caller asked
  // for (D-18, PD-09). An
  // object nobody scripts is skipped whole, so it keeps refusing an
  // undeclared object or counter only.
  for (const auto& [path, data] : m_objects) {
    if (!data.groupLeafLimit) {
      continue;
    }
    std::size_t carried = 0;
    for (const auto& address : leaves.addresses) {
      if (splitLeafAddress(address).first == path) {
        ++carried;
      }
    }
    if (carried > *data.groupLeafLimit) {
      const std::scoped_lock seedGuard {scriptLock()};
      m_lastRefusal = "fake group open refuses " + path + " with "
          + std::to_string(carried) + " leaves over its scripted count of "
          + std::to_string(*data.groupLeafLimit);
      return nullptr;
    }
  }
  auto window = std::make_unique<detail::FakeWindow>();
  window->owner = this;
  window->disclosureColumn = leaves.disclosureColumn;
  for (const auto& address : leaves.addresses) {
    const auto [objectPath, name] = splitLeafAddress(address);
    const auto object = m_objects.find(std::string(objectPath));
    if (object == m_objects.end()) {
      return nullptr;
    }
    const auto item = object->second.counters.find(std::string(name));
    if (item == object->second.counters.end()) {
      return nullptr;
    }
    const std::scoped_lock seedGuard {scriptLock()};
    window->leaves.push_back(detail::FakeWindow::Sampled {
        .data = &item->second,
        .playback =
            detail::FakeWindow::Playback {
                .position = item->second.position,
                .last = item->second.last,
                .deltaState = item->second.deltaState,
            },
    });
  }
  return window;
}

auto FakeProvider::lastRefusalMessage() const -> std::string
{
  const std::scoped_lock seedGuard {scriptLock()};
  return m_lastRefusal;
}

auto FakeProvider::setGapActions(const std::string_view objectPath,
                                 const std::string_view name,
                                 std::vector<std::size_t> actions)
    -> FakeProvider&
{
  const std::string path(objectPath);
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

auto FakeProvider::counter(const std::string& objectPath,
                           const std::string& name) -> FakeCounterData&
{
  return m_objects.at(objectPath).counters.at(name);
}

}  // namespace sg::counters
