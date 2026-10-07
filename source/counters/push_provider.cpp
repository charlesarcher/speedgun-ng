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

struct detail::PushWindow final : WindowReader
{
  PushWindow() { setThunk(&readDirect); }

  // The compiled plan hands the window over as the base reference
  // `ReadThunk` declares, and `PushProvider::open` constructs it as
  // this final type, so the reference names a push window on every
  // call. `final` fixes the target of the `readPoints` call, so the
  // sampling path takes one indirect call and no vtable lookup
  // (FR-022, T146).
  static auto readDirect(WindowReader& base, PointSink& sink) noexcept -> void
  {
    static_cast<PushWindow&>(base).readPoints(sink);
  }

  std::vector<const std::uint64_t*> cells;
  std::thread::id owner {};
  std::size_t disclosureColumn = LeafSet::kNoDisclosureColumn;

  void readPoints(PointSink& sink) noexcept override
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
    if (disclosureColumn != LeafSet::kNoDisclosureColumn) {
      sink.putDisclosure(disclosureColumn,
                         static_cast<std::uint64_t>(Availability::COUNTABLE));
    }
  }
};

PushProvider::PushProvider() = default;

PushProvider::~PushProvider() = default;

auto PushProvider::addCounter(const std::string_view name,
                              const std::string_view unit,
                              const std::string_view description) -> PushCounter
{
  SG_REQUIRE(!name.empty(), "add_counter names a counter (FR-035)");
  m_points.emplace_back(PushPoint {
      .value = 0,
      .owner = std::this_thread::get_id(),
      .name = std::string(name),
      .description = std::string(description),
      .unit = std::string(unit),
  });
  auto& point = m_points.back();
  SG_ENSURE(m_points.back().name == name,
            "the declared counter is enumerable (FR-035)");
  return PushCounter(&point.value, point.owner, point.name);
}

void PushProvider::enumerate(ObjectSink& sink) const
{
  std::vector<CatalogSeed> entries;
  entries.reserve(m_points.size());
  for (const auto& point : m_points) {
    entries.push_back(CatalogSeed {
        .name = point.name,
        .description = point.description,
        .unit = point.unit,
        .avail = Availability::COUNTABLE,
        .mode = ReadMode::PUSH_LOAD,
    });
  }
  sink.addObject(ObjectSeed {
      .kind = "machine",
      .path = "machine",
      .alias = {},
      .description = "local machine",
      .entries = std::move(entries),
  });
}

std::unique_ptr<WindowReader> PushProvider::open(const LeafSet& leaves,
                                                 const Target& /*where*/)
{
  auto window = std::make_unique<detail::PushWindow>();
  window->disclosureColumn = leaves.disclosureColumn;
  window->cells.reserve(leaves.addresses.size());
  for (const auto& address : leaves.addresses) {
    const PushPoint* match = nullptr;
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
    // one owner per window decides the `readPoints` guard for all of
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
