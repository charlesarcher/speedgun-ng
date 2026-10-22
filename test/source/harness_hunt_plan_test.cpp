// ============================================================================
// The hunt discovery and plan tables (specs/029-hunter-core T006, filled by
// T015; US1; FR-001 to FR-003, FR-011, FR-020, FR-041, FR-081; AC-1, SC-01;
// research D-1, D-9, D-12, D-13, PD-01 to PD-04, PD-06; data-model E-01, E-04,
// I-1, I-3).
//
// The seams are the two plan-side units, `sg::hunt::EventCatalog` and
// `sg::hunt::planHunt`, both of which read the published catalog and the
// published programmable-counter limits and nothing else, so every table here
// stands without a PMU, without a clock and without a host figure. The
// scripted host they read is `test/source/detail/hunt_fixture.hpp`, the
// fixture that registers the core devices, the `armv8_`-prefixed device, the
// uncore device, the 200 catalog events and the limit rows T015 drives.
//
// `planHunt` takes its limit list and its anchor list as arguments, so the
// limit rows are driven at that seam with `huntLimits` and `huntAnchors` and
// need no provider at all; only the discovery rows reach the counters system,
// and they register the fixture host once per process the way
// `registerPassFixture` registers the pass host, because the system takes an
// object path once.
//
// The pass counts and the per-pass widths are derived from the fixture's own
// figures - the event count, the limit of the row and the `max(L - 2, 0)` rule
// of I-3 - so no case here states a host figure, a CPU count or a clock reading
// (FR-003, FR-020, I-7).
//
// The list below is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Frameworkless check()/fail() convention,
// the one `harness_pass_plan_test.cpp` and `harness_pass_report_test.cpp`
// carry.
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "detail/hunt_fixture.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/hunt.hpp"

namespace
{

// The row of AC-1 and SC-01: the limit the published figure of the development
// host is compared against, and the capacity the `max(L - 2, 0)` rule of I-3
// answers for it. Both figures come from the limit, never from a host read.
constexpr std::size_t kAcOneLimit = 6;
constexpr std::size_t kAcOneCapacity = kAcOneLimit - 2;

// The event count of the limit-row table: wide enough that the narrowest
// fitting row (limit 3, two leaves reserved for the anchors) answers one event
// per pass and the widest answers one pass.
constexpr std::size_t kRowEvents = 4;

// The per-pass calibration figure of the quickstart walkthrough, the only
// figure `planHunt` multiplies into `estimateNs`.
constexpr std::uint64_t kCalibrationNsPerPass = 1'920'000;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT PLAN TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

auto mentions(const sg::counters::Error& error,
              const std::string_view needle) -> bool
{
  return error.message.find(needle) != std::string::npos;
}

// ---------------------------------------------------------------------------
// The scripted host
// ---------------------------------------------------------------------------

// The fixture host, registered once per process: the counters system takes an
// object path once, so a scenario cannot lay down a second host, and every
// discovery row here wants the same scripted catalog. The limit of the
// registration is irrelevant to `expand` and `split`, which read the catalog
// and the availability of each entry alone; the plan rows hand `planHunt` the
// limit list of their own.
[[nodiscard]] auto registerHuntHost() -> bool
{
  static const bool kAccepted = []
  {
    auto provider = std::make_unique<sg::counters::FakeProvider>();
    sg::test::huntHost(*provider);
    const auto settled =
        sg::counters::System::local().registerProvider(std::move(provider));
    return settled.has_value();
  }();
  return kAccepted;
}

// The index of one fixture device in `huntDevices()` order, which is the order
// `huntLimits` and `huntAnchors` answer in, so it is the `source` index a
// `CatalogAddress` of that device carries.
[[nodiscard]] auto deviceIndex(const std::string_view device) -> std::size_t
{
  const auto devices = sg::test::huntDevices();
  const auto found = std::ranges::find(devices, device);
  check(found != devices.end(), "the fixture host carries that device");
  return static_cast<std::size_t>(found - devices.begin());
}

// One selected event, in the spelling `EventCatalog::split` answers: the
// catalog address, the device that owns it, and the index of that device in the
// limit list handed to the planner.
[[nodiscard]] auto eventOn(const std::string_view device,
                           const std::size_t leaf) -> sg::hunt::CatalogAddress
{
  auto entry = sg::hunt::CatalogAddress {};
  entry.address = sg::test::huntAddress(device, sg::test::huntEvent(leaf));
  entry.device = std::string(device);
  entry.source = deviceIndex(device);
  return entry;
}

// `count` events of one device in catalog order, starting at `first`.
[[nodiscard]] auto eventsOn(const std::string_view device,
                            const std::size_t count,
                            const std::size_t first = 0)
    -> std::vector<sg::hunt::CatalogAddress>
{
  std::vector<sg::hunt::CatalogAddress> events;
  events.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    events.push_back(eventOn(device, first + index));
  }
  return events;
}

// The plan of one table row: the events over the scripted limit list and the
// scripted anchor list, with the calibration figure that `estimateNs` reads.
[[nodiscard]] auto planOf(const std::vector<sg::hunt::CatalogAddress>& events,
                          const std::optional<std::size_t> limit,
                          const sg::hunt::HuntOptions& options)
    -> std::expected<sg::hunt::HuntPlan, sg::counters::Error>
{
  return sg::hunt::planHunt(events,
                            sg::test::huntLimits(limit),
                            sg::test::huntAnchors(),
                            options,
                            kCalibrationNsPerPass);
}

// The plan of one row, refused or not: the row that has to answer a plan gives
// the caller that plan, because every table below reads its fields.
[[nodiscard]] auto planned(const std::vector<sg::hunt::CatalogAddress>& events,
                           const std::optional<std::size_t> limit,
                           const sg::hunt::HuntOptions& options,
                           const char* what) -> sg::hunt::HuntPlan
{
  auto plan = planOf(events, limit, options);
  check(plan.has_value(), what);
  return *plan;
}

// The distinct leaf addresses one pass claims: every event of its columns plus
// the anchors, each address counted once. This is where I-2 and the AC-1 claim
// that `cpu-cycles` and `instructions` are claimed exactly once per pass are
// readable from a plan record: four events that share one device and its two
// anchors give six leaves and no more.
[[nodiscard]] auto leavesOf(const sg::hunt::HuntPlan& plan,
                            const std::size_t pass) -> std::vector<std::string>
{
  std::vector<std::string> leaves;
  for (const auto column : plan.passes[pass]) {
    const auto& entry = plan.columns[column];
    const std::string_view candidates[] = {
        entry.event,
        entry.cycles,
        entry.instructions,
    };
    for (const auto leaf : candidates) {
      if (leaf.empty()
          || std::ranges::find(leaves, std::string(leaf)) != leaves.end())
      {
        continue;
      }
      leaves.emplace_back(leaf);
    }
  }
  return leaves;
}

// The invariant of `planHunt`: every column index stands in exactly one pass.
auto checkPassesCoverColumnsOnce(const sg::hunt::HuntPlan& plan) -> void
{
  std::vector<std::size_t> seen(plan.columns.size(), 0);
  for (const auto& pass : plan.passes) {
    for (const auto column : pass) {
      check(column < seen.size(), "no pass names a column that does not exist");
      ++seen[column];
    }
  }
  for (const auto count : seen) {
    check(count == 1, "every column stands in exactly one pass");
  }
}

// The width rule of I-3 over one plan: a pass that is not exclusive holds at
// most `max(L - 2, 0)` events of a device that publishes `L`, and at most `L`
// leaves counting the anchors.
auto checkLimitRule(const sg::hunt::HuntPlan& plan,
                    const std::size_t limit) -> void
{
  for (std::size_t pass = 0; pass < plan.passes.size(); ++pass) {
    check(plan.passes[pass].size() <= limit - 2,
          "a pass holds at most max(L - 2, 0) events");
    check(leavesOf(plan, pass).size() <= limit,
          "a pass holds at most L leaves counting its anchors");
  }
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// ---------------------------------------------------------------------------
// The column shape (I-1)
// ---------------------------------------------------------------------------

// One column per event, holding the event plus the two anchor leaves of its own
// device, and no third column shape: the anchor pair of every column is the
// scripted pair of that column's device, empty on the device that owns neither
// anchor, and no column carries the anchor of another device.
auto columnShape() -> void
{
  std::vector<sg::hunt::CatalogAddress> events;
  const auto devices = sg::test::huntDevices();
  for (std::size_t index = 0; index < devices.size(); ++index) {
    events.push_back(eventOn(devices[index], 0));
  }
  const auto anchors = sg::test::huntAnchors();
  const auto options = sg::hunt::HuntOptions {};
  const auto plan =
      planned(events, kAcOneLimit, options, "a column list plans");

  check(plan.columns.size() == events.size(), "one column per event");
  for (std::size_t index = 0; index < events.size(); ++index) {
    const auto& column = plan.columns[index];
    check(column.event == events[index].address, "the column names its event");
    check(column.source == events[index].source,
          "the column names the index of its own device");
    check(column.cycles == anchors[index].first,
          "the column carries the cycles anchor of its own device");
    check(column.instructions == anchors[index].second,
          "the column carries the instructions anchor of its own device");
    const auto ownsAnchors = !anchors[index].first.empty();
    check(ownsAnchors == !anchors[index].second.empty(),
          "a device either owns both anchors or neither");
    if (ownsAnchors) {
      check(sg::hunt::EventCatalog::devicePart(column.cycles)
                == sg::hunt::EventCatalog::devicePart(column.event),
            "no column carries the anchor of another device");
      check(sg::hunt::EventCatalog::devicePart(column.instructions)
                == sg::hunt::EventCatalog::devicePart(column.event),
            "no column carries the anchor of another device");
    } else {
      check(column.cycles.empty() && column.instructions.empty(),
            "a device without an anchor leaves both anchor fields empty");
    }
  }
}

// ---------------------------------------------------------------------------
// The limit table (FR-011, I-3)
// ---------------------------------------------------------------------------

// The rows of the published-limit table: `kHuntRows` - 1, 3, 4, 6 and 8 - plus
// the device that cannot answer, `std::nullopt`. A published limit `L` admits
// `max(L - 2, 0)` events per pass, so the pass count of the row follows from
// the limit alone; no limit leaves the planner no plan and settles `MULTIPLEX`
// (D-19, spec PC-2).
auto limitRows() -> void
{
  const auto events = eventsOn(sg::test::kHuntCpu, kRowEvents);
  const auto options = sg::hunt::HuntOptions {};

  for (const auto limit : sg::test::kHuntRows) {
    if (limit < 3) {
      const auto narrow = planOf(events, limit, options);
      check(!narrow.has_value(), "a limit below 3 admits no event");
      continue;
    }
    const auto plan =
        planned(events, limit, options, "a published limit plans");
    check(plan.columns.size() == kRowEvents, "one column per event");
    check(plan.method == sg::MetricMethod::PASSES,
          "a needed device that publishes a limit settles PASSES");
    check(plan.anchorsPerPass == 2, "both anchors are claimed per pass");
    check(plan.normalized, "the plan normalizes by an anchor");
    const auto capacity = limit - 2;
    const auto expectedPasses = (kRowEvents + capacity - 1) / capacity;
    check(plan.passes.size() == expectedPasses,
          "the pass count follows from max(L - 2, 0)");
    checkLimitRule(plan, limit);
    checkPassesCoverColumnsOnce(plan);
    check(
        plan.estimateNs.has_value()
            && *plan.estimateNs
                == kCalibrationNsPerPass * plan.passes.size() * options.repeats,
        "the estimate is calibration times passes times repeats");
  }

  const auto unlimiting =
      planned(events, std::nullopt, options, "no published limit plans");
  check(unlimiting.method == sg::MetricMethod::MULTIPLEX,
        "no needed device publishes a limit, so the plan settles MULTIPLEX");
  check(unlimiting.passes.size() == 1,
        "the multiplex plan is the one plan holding every column");
  check(unlimiting.passes.front().size() == kRowEvents,
        "the multiplex plan lists every column");
  checkPassesCoverColumnsOnce(unlimiting);
}

// AC-1 and SC-01: 200 events at limit 6 give 50 passes of 4 events, with
// `cpu-cycles` and `instructions` claimed exactly once per pass. Every figure
// is derived from the fixture's event count and the row's limit.
auto twoHundredEvents() -> void
{
  const auto cpu = deviceIndex(sg::test::kHuntCpu);
  const auto events = eventsOn(sg::test::kHuntCpu, sg::test::kHuntEvents);
  const auto options = sg::hunt::HuntOptions {};
  const auto plan =
      planned(events, kAcOneLimit, options, "the 200-event list plans");

  check(plan.columns.size() == sg::test::kHuntEvents, "one column per event");
  check(plan.passes.size() == sg::test::kHuntEvents / kAcOneCapacity,
        "200 events at limit 6 give 50 passes");
  checkPassesCoverColumnsOnce(plan);
  checkLimitRule(plan, kAcOneLimit);

  const auto& anchors = sg::test::huntAnchors();
  const auto& pair = anchors[cpu];
  for (std::size_t pass = 0; pass < plan.passes.size(); ++pass) {
    check(plan.passes[pass].size() == kAcOneCapacity,
          "each pass holds four events");
    const auto leaves = leavesOf(plan, pass);
    check(leaves.size() == kAcOneLimit,
          "the pass claims its four events and both anchors, once each");
    check(std::find(leaves.begin(), leaves.end(), pair.first) != leaves.end(),
          "the pass claims the cpu-cycles anchor of its device");
    check(std::find(leaves.begin(), leaves.end(), pair.second) != leaves.end(),
          "the pass claims the instructions anchor of its device");
  }
  check(plan.anchorsPerPass == 2, "both anchors ride every pass");
  check(plan.normalized, "the 200-event plan normalizes by an anchor");
}

// Limit 3 admits one event per pass: the three-leaf shape of one event with its
// two anchors is the smallest plan (E-04 limit rule).
auto limitThree() -> void
{
  const auto events = eventsOn(sg::test::kHuntCpu, kRowEvents);
  const auto options = sg::hunt::HuntOptions {};
  const auto plan = planned(events, 3, options, "limit 3 plans");

  check(plan.passes.size() == kRowEvents, "limit 3 gives one event per pass");
  for (std::size_t pass = 0; pass < plan.passes.size(); ++pass) {
    check(plan.passes[pass].size() == 1, "one event per pass at limit 3");
    check(leavesOf(plan, pass).size() == 3,
          "the pass holds the event and its two anchors");
  }
  checkPassesCoverColumnsOnce(plan);
}

// A device that publishes a limit below 3 with both anchors present admits no
// event: the answer is the recoverable error naming the device and the limit
// (FR-081), and the same event plans as soon as the device publishes 3. The
// `armv8_` fixture device carries the check, because its name is unlike every
// other path of the scripted host.
auto narrowLimitRefusal() -> void
{
  const auto arm = sg::test::kHuntArm;
  const auto armIndex = deviceIndex(arm);
  const auto events = std::vector<sg::hunt::CatalogAddress> {eventOn(arm, 0)};
  const auto options = sg::hunt::HuntOptions {};

  const std::size_t narrowRows[] = {1, 2};
  for (const std::size_t narrow : narrowRows) {
    auto limits = sg::test::huntLimits(kAcOneLimit);
    limits[armIndex] = narrow;
    const auto plan = sg::hunt::planHunt(events,
                                         limits,
                                         sg::test::huntAnchors(),
                                         options,
                                         kCalibrationNsPerPass);
    check(!plan.has_value(), "a limit below 3 with both anchors is refused");
    if (!plan.has_value()) {
      check(mentions(plan.error(), arm),
            "the refusal names the device that publishes the narrow limit");
      check(mentions(plan.error(), std::to_string(narrow)),
            "the refusal names the limit itself");
    }
  }

  auto fits = sg::test::huntLimits(kAcOneLimit);
  fits[armIndex] = 3;
  const auto plan = sg::hunt::planHunt(
      events, fits, sg::test::huntAnchors(), options, kCalibrationNsPerPass);
  check(plan.has_value(), "the same event plans at limit 3");
  if (plan.has_value()) {
    check(plan->passes.size() == 1, "limit 3 seats the event alone");
    check(plan->columns.front().source == armIndex,
          "the column plans on the device that published the limit");
  }
}

// An empty event list is not a refusal: it is the shape of a host whose
// discovery refused every event, and the plan is zero columns, empty passes,
// `anchorsPerPass` 0 and `normalized` false, so the run prints the plan line,
// the skipped list and the `events=0/N` summary and exits 0 (D-12, FR-041).
auto emptyEventList() -> void
{
  const auto options = sg::hunt::HuntOptions {};
  const auto plan = planOf({}, kAcOneLimit, options);

  check(plan.has_value(), "an empty event list is not a refusal");
  if (!plan.has_value()) {
    return;
  }
  check(plan->columns.empty(), "zero columns");
  check(plan->passes.empty(), "empty passes");
  check(plan->anchorsPerPass == 0, "no anchor is claimed by an empty plan");
  check(!plan->normalized, "an empty plan normalizes by iterations");
}

// ---------------------------------------------------------------------------
// The discovery rows (FR-002, FR-010, D-1, D-9)
// ---------------------------------------------------------------------------

// The address order of one `split` answer, so a row compares orders and sets.
[[nodiscard]] auto orderOf(const std::vector<sg::hunt::CatalogAddress>& columns)
    -> std::vector<std::string>
{
  std::vector<std::string> order;
  order.reserve(columns.size());
  for (const auto& column : columns) {
    order.push_back(column.address);
  }
  return order;
}

// `all` gives the scripted catalog in catalog-scan order, and a list gives the
// order of its text with a repeated address kept at its first position and
// dropped afterwards - whether the repeat stands beside it or across a newline
// (FR-002, E-01).
auto catalogOrderAndDedup() -> void
{
  check(registerHuntHost(), "the scripted hunt host registered");
  const auto catalog = sg::test::huntCatalog();

  auto whole = sg::hunt::EventCatalog {};
  check(whole.expand("all").has_value(), "the whole-catalog selection expands");
  check(whole.addresses() == catalog,
        "all selects the scripted events in catalog order");

  const auto text =
      catalog[2] + "," + catalog[0] + "\n" + catalog[2] + "," + catalog[1];
  auto listed = sg::hunt::EventCatalog {};
  check(listed.expand(text).has_value(), "a comma and newline list expands");
  const std::vector<std::string> kExpected {
      catalog[2],
      catalog[0],
      catalog[1],
  };
  check(
      listed.addresses() == kExpected,
      "a list keeps the order of its text and the first position of a repeat");
}

// One seed gives one order and two seeds give two orders, over the same
// catalog, and the order is a permutation of the selection - no address enters
// twice and none disappears (D-9, PD-06, FR-014).
auto seedOrder() -> void
{
  check(registerHuntHost(), "the scripted hunt host registered");
  auto catalog = sg::hunt::EventCatalog {};
  check(catalog.expand("all").has_value(),
        "the whole-catalog selection expands");

  const auto first = catalog.split(0);
  const auto again = catalog.split(0);
  const auto other = catalog.split(1);

  check(first.second.empty() && other.second.empty(),
        "every scripted entry is countable, so none is skipped");
  check(first.first.size() == catalog.addresses().size(),
        "one column per countable address");
  check(orderOf(first.first) == orderOf(again.first),
        "two runs of one seed give the same order");
  check(orderOf(first.first) != orderOf(other.first),
        "two seeds give two orders");

  auto shuffled = orderOf(first.first);
  auto selected = catalog.addresses();
  std::ranges::sort(shuffled);
  std::ranges::sort(selected);
  check(shuffled == selected, "the shuffle is a permutation of the selection");
}

// The core-device rule: the three exact core device parts and the `armv8_`
// prefix device are core, the uncore device is not (D-1, PD-02). Every address
// is a leaf the scripted host carries.
auto coreDeviceRule() -> void
{
  check(registerHuntHost(), "the scripted hunt host registered");
  const auto catalog = sg::hunt::EventCatalog {};

  const std::string_view coreDevices[] = {
      sg::test::kHuntCpu,
      sg::test::kHuntCore,
      sg::test::kHuntAtom,
      sg::test::kHuntArm,
  };
  for (const auto device : coreDevices) {
    check(catalog.isCoreDevice(
              sg::test::huntAddress(device, sg::test::kHuntCycle)),
          "the core rule admits the device");
  }
  check(!catalog.isCoreDevice(sg::test::huntAddress(
            sg::test::kHuntUncore, sg::test::huntEvent(sg::test::kHuntEvents))),
        "the core rule excludes the uncore device");
}

// A norm whose anchor stands refused is the recoverable error naming the cause,
// while the unset norm plans `iter` with `normalized` false (D-13). The
// anchorless scripted device is the uncore device; the same norm on a cpu event
// plans, so the refusal is the refused anchor and not the norm value.
auto normAnchorRefused() -> void
{
  const auto anchorless =
      eventsOn(sg::test::kHuntUncore, 1, sg::test::kHuntEvents);
  const auto counted = eventsOn(sg::test::kHuntCpu, 1);
  auto options = sg::hunt::HuntOptions {};

  const sg::hunt::HuntNorm norms[] = {
      sg::hunt::HuntNorm::CYCLE,
      sg::hunt::HuntNorm::INSTR,
  };
  for (const auto norm : norms) {
    options.norm = norm;
    const auto refused = planOf(anchorless, kAcOneLimit, options);
    check(!refused.has_value(), "a norm whose anchor is refused is refused");
    if (!refused.has_value()) {
      const auto namesAnchor = mentions(refused.error(), sg::test::kHuntUncore)
          || mentions(refused.error(),
                      norm == sg::hunt::HuntNorm::CYCLE ? sg::test::kHuntCycle
                                                        : sg::test::kHuntInstr);
      check(namesAnchor, "the refusal names the cause");
    }
    const auto seated = planOf(counted, kAcOneLimit, options);
    check(seated.has_value(), "the same norm plans where the anchor stands");
    check(seated.has_value() && seated->normalized,
          "a plan that holds its norm's anchor normalizes");
  }

  options.norm = std::nullopt;
  const auto fallback =
      planned(anchorless, kAcOneLimit, options, "the unset norm plans");
  check(!fallback.normalized, "the unset norm falls back to iterations");
  check(fallback.anchorsPerPass == 0, "the fallback plan claims no anchor");
  check(fallback.columns.front().cycles.empty()
            && fallback.columns.front().instructions.empty(),
        "the fallback column carries no anchor of the anchorless device");
}

// One uncore address through a list plans on its own device: the list reaches
// any device, the column names the index and the anchors of that device, and
// the plan normalizes by iterations because its device owns neither anchor
// (D-1, I-1).
auto uncoreListOwnDevice() -> void
{
  check(registerHuntHost(), "the scripted hunt host registered");
  const auto uncore = deviceIndex(sg::test::kHuntUncore);
  const auto address = sg::test::huntAddress(
      sg::test::kHuntUncore, sg::test::huntEvent(sg::test::kHuntEvents));

  auto catalog = sg::hunt::EventCatalog {};
  check(catalog.expand(address).has_value(),
        "a list reaches the uncore device");
  check(
      catalog.addresses().size() == 1 && catalog.addresses().front() == address,
      "the list selects the one uncore address");

  const auto split = catalog.split(0);
  check(split.first.size() == 1, "the uncore address becomes one column");
  check(split.second.empty(), "the scripted uncore entry is countable");
  check(split.first.front().source == uncore,
        "the column names the uncore index of the limit list");

  const auto plan = planned(split.first,
                            kAcOneLimit,
                            sg::hunt::HuntOptions {},
                            "the uncore event plans");
  check(plan.columns.size() == 1, "one column for one event");
  check(plan.columns.front().event == address,
        "the column names the uncore event");
  check(plan.columns.front().source == uncore,
        "the column plans on the uncore device");
  check(plan.columns.front().cycles.empty()
            && plan.columns.front().instructions.empty(),
        "the uncore device owns neither anchor");
  check(plan.anchorsPerPass == 0, "no anchor rides the uncore plan");
  check(!plan.normalized, "the uncore plan normalizes by iterations");
  check(plan.method == sg::MetricMethod::MULTIPLEX,
        "the uncore device publishes no limit, so the plan settles MULTIPLEX");
  check(plan.passes.size() == 1 && plan.passes.front().size() == 1,
        "the multiplex plan lists the one column");
}

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"column-shape", columnShape},
      Scenario {"limit-rows", limitRows},
      Scenario {"two-hundred-events", twoHundredEvents},
      Scenario {"limit-three", limitThree},
      Scenario {"narrow-limit-refusal", narrowLimitRefusal},
      Scenario {"empty-event-list", emptyEventList},
      Scenario {"catalog-order-dedup", catalogOrderAndDedup},
      Scenario {"seed-order", seedOrder},
      Scenario {"core-device-rule", coreDeviceRule},
      Scenario {"norm-anchor-refused", normAnchorRefused},
      Scenario {"uncore-list-own-device", uncoreListOwnDevice},
  };
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const auto& list = scenarios();
  // One name selects one scenario through `argv[1]`, so a name that stands
  // twice would hide the scenario it repeats.
  for (std::size_t first = 0; first < list.size(); ++first) {
    check(list[first].body != nullptr, "every scenario carries a body");
    for (std::size_t second = first + 1; second < list.size(); ++second) {
      check(list[first].name != list[second].name,
            "no two scenarios of this file carry the same name");
    }
  }
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : list) {
      if (scenario.name == mode) {
        scenario.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : list) {
    scenario.body();
  }
  std::printf("harness_hunt_plan_test: ok\n");
  return 0;
}
