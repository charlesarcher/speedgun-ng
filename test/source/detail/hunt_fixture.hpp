#ifndef SG_TEST_DETAIL_HUNT_FIXTURE_HPP
#define SG_TEST_DETAIL_HUNT_FIXTURE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pass_fixture.hpp"
#include "speedgun-ng/counters_fake.hpp"

// ============================================================================
// The scripted host the hunt tests share (specs/029-hunter-core T008;
// FR-002, FR-003, FR-011; AC-1; research PD-02, PD-03; data-model E-01, E-04;
// spec PC-1, PC-2).
//
// One `FakeProvider` carries the device tree the hunter walks: the three core
// device objects `cpu`, `cpu_core` and `cpu_atom` that the core rule names
// exactly, one device whose path carries the `armv8_` prefix the same rule
// reads, and one uncore device the rule excludes. Every core device answers
// `cpu-cycles` and `instructions`, so a column always finds the two anchors of
// its own device (I-1). The uncore device answers no anchor and publishes no
// limit, which is the source that cannot answer: a hunt of its events alone
// reaches no plan and settles `MULTIPLEX` (D-19, spec PC-2).
//
// The catalog is the 200 events `huntCatalog()` names, all on `kHuntCpu`, so
// the row of AC-1 - 200 events at a limit of 6, 50 passes of four events, each
// anchor claimed once per pass - holds on one device. The three-digit event
// names are what make the scripted order the catalog order: the fake
// enumerates the leaves of an object in name order, so `hunt000` precedes
// `hunt001` and the padded numbering keeps the event of index k the k-th
// entry for every k below 1000.
//
// Every scripted fact is laid down before the provider registers, for the
// reason `registerPassFixture` states: the programmable-limit table and the
// catalog freeze at the open boundary, so a limit scripted afterwards is
// silently skipped. Nothing here reads sysfs, a PMU, a clock or a CPU count
// (FR-003, FR-092), and the clock leaves of the printed row are the pass
// fixture's own, scripted with the pass fixture's own step.
//
// The names this fixture states, and how a scenario registers it, are listed
// at the bottom of the file.
// ============================================================================

namespace sg::test
{

// The core device objects, spelled as the core rule spells them
// (`counter_limits.cpp:284-295` matches these three names and no other).
inline constexpr std::string_view kHuntCpu {"cpu"};
inline constexpr std::string_view kHuntCore {"cpu_core"};
inline constexpr std::string_view kHuntAtom {"cpu_atom"};

// The device the `armv8_` prefix rule of D-1 admits: the device part carries
// the prefix, so the exact-name rule alone answers false here and the hunt-side
// prefix rule alone answers true.
inline constexpr std::string_view kHuntArm {"armv8_pmuv3"};

// The device the core rule excludes. The instance number is one no host
// publishes, the way `metric_fixture.hpp` spells its uncore object, so the
// script cannot meet a real catalog path.
inline constexpr std::string_view kHuntUncore {"uncore_imc_78"};

// The two anchor leaves of a core device, spelled as the kernel spells them.
inline constexpr std::string_view kHuntCycle {"cpu-cycles"};
inline constexpr std::string_view kHuntInstr {"instructions"};

// The events on `kHuntCpu`, the 200 of AC-1 and SC-01, and the events on
// `kHuntUncore`. The uncore events continue the numbering, so
// `huntEvent(kHuntEvents)` names the first of them.
inline constexpr std::size_t kHuntEvents = 200;
inline constexpr std::size_t kHuntUncoreEvents = 2;

// The limit rows of the plan and collect tables: the narrow device that
// cannot hold an event beside its two anchors, and the rows of AC-1 and AC-2.
// The no-limit shape is `std::nullopt` at the call, the value `huntLimits`
// answers for the uncore device.
inline constexpr std::array<std::size_t, 5> kHuntRows {1, 3, 4, 6, 8};

// The scripted per-window figures. The event of index k folds
// `huntValue(k) == (k + 1) * kHuntValueStep`, and the anchors of the device at
// index d of `huntDevices()` fold `huntCycle(d)` and `huntInstr(d)`. The
// anchors step by a figure that differs per device on purpose: a column that
// divided by the anchor of another device could not then fold the number the
// expectation of `huntCycle` and `huntInstr` states.
inline constexpr std::uint64_t kHuntValueStep = 100;
inline constexpr std::uint64_t kHuntCycleStep = 1000;
inline constexpr std::uint64_t kHuntInstrStep = 2000;

// The device objects of the host in the order `Object::children()` reports
// them, which is path order: the arm device, the three core objects, the
// uncore device. That is the order a catalog walk sees, so it is the order
// `huntLimits` and `huntAnchors` answer in and the order a scenario's own
// limit list follows.
[[nodiscard]] inline auto huntDevices() -> std::vector<std::string_view>
{
  return {kHuntArm, kHuntCpu, kHuntAtom, kHuntCore, kHuntUncore};
}

// The name of the scripted event of one index: `hunt000` to `hunt199` on
// `kHuntCpu` and the two numbers past that on `kHuntUncore`.
//
// \pre `index` stands below `kHuntEvents + kHuntUncoreEvents`, the scripted
//      numbering; a larger index names a leaf no script lays down and an open
//      over it is the fake's unknown-counter refusal.
[[nodiscard]] inline auto huntEvent(const std::size_t index) -> std::string
{
  std::string name = "hunt";
  if (index < 10) {
    name += "00";
  } else if (index < 100) {
    name += '0';
  }
  name += std::to_string(index);
  return name;
}

// The address of one leaf, composed the one way the catalog composes it:
// `Object::path() + "/" + entry.name` (spec PC-1).
[[nodiscard]] inline auto huntAddress(std::string_view device,
                                      std::string_view leaf) -> std::string
{
  return std::string(device) + "/" + std::string(leaf);
}

// The 200 catalog events of `kHuntCpu` in catalog order.
//
// \post no address repeats, every address names an event the script lays down,
//       and the order is the order `Object::counters()` reports for the device.
[[nodiscard]] inline auto huntCatalog() -> std::vector<std::string>
{
  std::vector<std::string> addresses;
  addresses.reserve(kHuntEvents);
  for (std::size_t index = 0; index < kHuntEvents; ++index) {
    addresses.push_back(huntAddress(kHuntCpu, huntEvent(index)));
  }
  return addresses;
}

// The figure one sampling window folds for the scripted event of one index,
// and for the two anchors of the device at index `device` of `huntDevices()`.
[[nodiscard]] constexpr auto huntValue(const std::size_t index) noexcept
    -> std::uint64_t
{
  return static_cast<std::uint64_t>(index + 1) * kHuntValueStep;
}

[[nodiscard]] constexpr auto huntCycle(const std::size_t device) noexcept
    -> std::uint64_t
{
  return static_cast<std::uint64_t>(device + 1) * kHuntCycleStep;
}

[[nodiscard]] constexpr auto huntInstr(const std::size_t device) noexcept
    -> std::uint64_t
{
  return static_cast<std::uint64_t>(device + 1) * kHuntInstrStep;
}

// The published-limit list of the host in `huntDevices()` order: `limit` for
// every core device and nothing for the uncore device.
[[nodiscard]] inline auto huntLimits(std::optional<std::size_t> limit)
    -> std::vector<std::optional<std::size_t>>
{
  return {limit, limit, limit, limit, std::nullopt};
}

// The anchor list of the host in `huntDevices()` order, the shape `planHunt`
// takes: the two anchor addresses of every core device and the empty pair of
// the uncore device, which owns neither anchor (I-1).
[[nodiscard]] inline auto huntAnchors()
    -> std::vector<std::pair<std::string, std::string>>
{
  std::vector<std::pair<std::string, std::string>> anchors;
  for (const auto device : huntDevices()) {
    if (device == kHuntUncore) {
      anchors.emplace_back();
      continue;
    }
    anchors.emplace_back(huntAddress(device, kHuntCycle),
                         huntAddress(device, kHuntInstr));
  }
  return anchors;
}

// Lay the whole scripted host on a provider the caller owns and hand that
// provider back, so a scenario chains the fact of its own onto the same fluent
// line before it registers: a leaf re-scripted with `setPoints`, a refused
// `Availability` declared by a second `addCounter`, or an object limit
// overridden by a second `setProgrammableCounters`, whose last call wins.
//
// `limit` is the programmable-counter count every core device answers. The
// tables drive `kHuntRows` and the no-limit shape `std::nullopt`, the host a
// hunt of the uncore events alone settles on `MULTIPLEX` (D-19, spec PC-2).
// The default is the published limit of the pass fixture's host.
//
// \pre `limit` holds nothing or a value above zero, the precondition the
//      provider's own limit setter carries (FR-006).
// \post the five devices of `huntDevices()` are enumerable, every core device
//       publishes `limit` and answers `kHuntCycle` beside `kHuntInstr`, the
//       uncore device publishes nothing and answers `kHuntUncoreEvents` events
//       with no anchor, the events of `huntCatalog()` fold `huntValue` per
//       window, and the machine object answers the two clock leaves of the
//       pass fixture (FR-002, FR-003).
inline auto huntHost(sg::counters::FakeProvider& host,
                     std::optional<std::size_t> limit = kHostPassLimit)
    -> sg::counters::FakeProvider&
{
  const auto devices = huntDevices();
  const auto limits = huntLimits(limit);
  const auto anchors = huntAnchors();
  for (std::size_t index = 0; index < devices.size(); ++index) {
    host.addObject(devices[index], "pmu", "hunt fixture device");
    if (limits[index].has_value()) {
      host.setProgrammableCounters(devices[index], *limits[index]);
    }
    if (anchors[index].first.empty()) {
      continue;
    }
    host.addCounter(devices[index], kHuntCycle, "none", "hunt fixture cycles");
    host.addCounter(devices[index], kHuntInstr, "none", "hunt fixture instrs");
    host.setPoints(devices[index], kHuntCycle, {0}, huntCycle(index));
    host.setPoints(devices[index], kHuntInstr, {0}, huntInstr(index));
  }
  for (std::size_t index = 0; index < kHuntEvents; ++index) {
    const std::string name = huntEvent(index);
    host.addCounter(kHuntCpu, name, "none", "hunt fixture event");
    host.setPoints(kHuntCpu, name, {0}, huntValue(index));
  }
  for (std::size_t index = 0; index < kHuntUncoreEvents; ++index) {
    const std::string name = huntEvent(kHuntEvents + index);
    host.addCounter(kHuntUncore, name, "none", "hunt fixture uncore event");
    host.setPoints(kHuntUncore, name, {0}, huntValue(kHuntEvents + index));
  }
  host.addCounter(
      kPassClockObject, kPassRealTimeLeaf, "nanoseconds", "fixture clock");
  host.addCounter(
      kPassClockObject, kPassCpuTimeLeaf, "nanoseconds", "fixture cpu clock");
  host.setPoints(kPassClockObject, kPassRealTimeLeaf, {0}, kPassTimeStep);
  host.setPoints(kPassClockObject, kPassCpuTimeLeaf, {0}, kPassTimeStep);
  return host;
}

// ---------------------------------------------------------------------------
// The names this fixture states, for the tasks that call it.
//
// kHuntCpu, kHuntCore, kHuntAtom    the core device paths `cpu`, `cpu_core`
//                                   and `cpu_atom`
// kHuntArm                          `armv8_pmuv3`, the `armv8_` prefix device
// kHuntUncore                       `uncore_imc_78`, the excluded device
// kHuntCycle, kHuntInstr            the anchor leaves `cpu-cycles` and
//                                   `instructions`
// kHuntEvents, kHuntUncoreEvents    200 events on `kHuntCpu`, 2 on the uncore
// kHuntRows                         the limit rows 1, 3, 4, 6 and 8; the
//                                   no-limit row is `std::nullopt`
// kHuntValueStep, kHuntCycleStep,
// kHuntInstrStep                    the per-window steps below
// huntDevices()                     the five devices in `children()` order
// huntEvent(index)                  the event name of one index
// huntAddress(device, leaf)         `device + "/" + leaf`, the catalog spelling
// huntCatalog()                     the 200 event addresses of `kHuntCpu`
// huntValue(index)                  the figure an event window folds
// huntCycle(index), huntInstr(index)
//                                   the figure each anchor window folds on the
//                                   device at that index of `huntDevices()`
// huntLimits(limit)                 the `sourceLimits` list of the host
// huntAnchors()                     the `anchors` list of the host
// huntHost(host, limit)             the one builder: it scripts the host on a
//                                   caller-owned provider and returns it
//
// A scenario registers the host the way `registerPassFixture` registers the
// pass host, because the counters system takes an object path once per process
// and a scenario that needs a second host runs in a re-exec of its own binary:
//
//   auto provider = std::make_unique<sg::counters::FakeProvider>();
//   sg::test::huntHost(*provider, 6);
//   const bool accepted =
//       sg::counters::System::local().registerProvider(std::move(provider));
//
// which takes `speedgun-ng/counters_system.hpp` and `<memory>` beside this
// header.
}  // namespace sg::test

#endif
