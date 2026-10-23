// ============================================================================
// The hunt discovery, plan, budget and stage tables (specs/029-hunter-core
// T006, filled by T015 and T031; US1, US2; FR-001 to FR-003, FR-011 to FR-013,
// FR-020, FR-041, FR-081; AC-1, AC-7, SC-01, SC-07; research D-1, D-2, D-9,
// D-12, D-13, PD-01 to PD-04, PD-06; data-model E-01, E-04, I-1, I-3).
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
// need no provider at all; the discovery rows and the budget rows reach the
// counters system, and they register the fixture host once per process the way
// `registerPassFixture` registers the pass host, because the system takes an
// object path once.
//
// The budget and stage tables of T031 read two further units of the same plan
// lane. The option body behind `--hunt-budget` stands in `hunt_cli.cpp`, read
// here as it is read in `cli.cpp:326`, and a refused value is checked in a
// child of its own because the refusal path calls `std::exit(2)`. The stage
// split stands in `detail::splitStages`, the call `planHunt` makes for its own
// `stages` field: the plan record carries the stages but not the cut verdict,
// so the cut row reads the verdict there and every other row reads the same
// split through `planHunt`. One fixed seed and one fixed calibration figure are
// the only inputs, so each row states its whole numbers before it plans
// (FR-012, FR-013, SC-07, quickstart US2).
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
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/hunt_fixture.hpp"
#include "detail/hunt_plan.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/hunt.hpp"

// The `--hunt-budget` body of `hunt_cli.cpp`, declared here the way
// `cli.cpp:326` declares it beside its option row and
// `harness_compare_test.cpp:106` declares the compare bodies it drives: the
// parse core has no header of its own.
namespace sg::detail
{
auto parseHuntBudget(const char* text, RunOptions& options) -> void;
}  // namespace sg::detail

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

// The two fixed inputs SC-07 names: the seed of the column order and the
// calibration figure above. Every figure of the budget tables follows from
// these two, the fixture event count and the AC-7 limit, so no row reads a host
// value.
constexpr std::uint64_t kFixedSeed = 0;
constexpr std::size_t kDefaultRepeats = 3;
constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000;

// The AC-7 shape: 200 events at limit 6 with a top of 4. Stage 2 spans three
// times the top (D-2), `kAcSevenPasses` passes hold the 200 columns at the four
// events per pass of I-3, and the 12 stage-2 columns stand in 3 of those
// passes. The two stages therefore cost 50 + 3 x 3 = 59 pass runs, the count
// quickstart US2 states.
constexpr std::size_t kAcSevenTop = 4;
constexpr std::size_t kStageTwoWidth = 3 * kAcSevenTop;
constexpr std::size_t kAcSevenPasses = sg::test::kHuntEvents / kAcOneCapacity;
constexpr std::size_t kAcSevenRuns =
    kAcSevenPasses + (kStageTwoWidth / kAcOneCapacity) * kDefaultRepeats;

// The budgets of the tables: 60 passes host the 59 runs, the quickstart 8 does
// not, and 120 seconds hosts them through the calibration figure alone, because
// 120 x 1'000'000'000 / 1'920'000 answers 62'500 passes.
constexpr std::uint64_t kFitPasses = 60;
constexpr std::uint64_t kCutPasses = 8;
constexpr std::uint64_t kFitSeconds = 120;

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

// ---------------------------------------------------------------------------
// The budget and stage seams (T031)
// ---------------------------------------------------------------------------

// One pass budget, the record the `<n>p` spelling settles (FR-012).
[[nodiscard]] auto passBudget(const std::uint64_t passes)
    -> sg::hunt::HuntBudget
{
  return {sg::hunt::HuntBudget::Kind::PASSES, passes};
}

// One seconds budget, the record the `<n>s` spelling settles (FR-012).
[[nodiscard]] auto secondsBudget(const std::uint64_t seconds)
    -> sg::hunt::HuntBudget
{
  return {sg::hunt::HuntBudget::Kind::SECONDS, seconds};
}

// The settled values of one budget row: the top the row names, the budget it
// names and the defaults of E-03 beside them. The repeats of those defaults are
// checked here, because every figure of the stage tables counts them.
[[nodiscard]] auto budgetOptions(
    const std::optional<sg::hunt::HuntBudget>& budget,
    const std::size_t top) -> sg::hunt::HuntOptions
{
  auto options = sg::hunt::HuntOptions {};
  options.top = top;
  options.budget = budget;
  check(options.repeats == kDefaultRepeats,
        "the settled defaults run the repeats the rows state");
  return options;
}

// The budget the option body settles for one text: the parse creates the hunt
// record the way the option scan does, and the row reads that record back
// (FR-012, contracts/hunt-cli.md).
[[nodiscard]] auto settledBudget(const char* text) -> sg::hunt::HuntBudget
{
  auto run = sg::detail::RunOptions {};
  sg::detail::parseHuntBudget(text, run);
  check(run.hunt.has_value() && run.hunt->budget.has_value(),
        "the option settles a budget in the hunt record");
  return *run.hunt->budget;
}

// One refused budget, in a child of its own: the refusal path prints the one
// line its contract names and calls `std::exit(2)`, so the row needs the
// process that exit can take, the pattern of the refusal children of
// `harness_hunt_cli_test.cpp`. The child sends its stderr to a file the parent
// reads back, so the row sees the refusal line and none of this binary's own
// output (FR-012, FR-081).
auto refusesBudget(const char* text, const std::string_view line) -> void
{
  const std::string capture {"hunt_plan_budget_refusal.txt"};
  const pid_t child = ::fork();
  check(child >= 0, "the refusal row forked a child");
  if (child == 0) {
    if (std::freopen(capture.c_str(), "w", stderr) == nullptr) {
      std::exit(3);
    }
    auto run = sg::detail::RunOptions {};
    sg::detail::parseHuntBudget(text, run);
    // The parse returned, so it accepted a value the contract refuses.
    std::exit(0);
  }
  int status = 0;
  check(::waitpid(child, &status, 0) == child,
        "the refusal row waited for the child it forked");
  check(WIFEXITED(status) && WEXITSTATUS(status) == 2,
        "a malformed budget leaves with the usage status (FR-012, FR-081)");
  std::ifstream file(capture);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string printed = buffer.str();
  check(printed == std::string(line) + "\n",
        "one line names the value the option read (FR-081)");
  std::remove(capture.c_str());
}

// The columns of the AC-7 shape: the scripted catalog expanded and split at the
// fixed seed, the seam FR-014 fixes the stage order to. A row takes the leading
// `count` of that order, so every budget table stands on one fixed column order
// (SC-07).
[[nodiscard]] auto acSevenColumns(
    const std::size_t count = sg::test::kHuntEvents)
    -> std::vector<sg::hunt::CatalogAddress>
{
  check(registerHuntHost(), "the scripted hunt host registered");
  auto catalog = sg::hunt::EventCatalog {};
  check(catalog.expand("all").has_value(), "the fixed selection expands");
  const auto split = catalog.split(kFixedSeed);
  check(split.second.empty(), "every scripted entry is countable");
  check(split.first.size() == sg::test::kHuntEvents,
        "the fixed seed orders the 200 planned columns");
  check(count <= split.first.size(),
        "the row asks for columns the fixed order holds");
  return {split.first.begin(),
          split.first.begin() + static_cast<std::ptrdiff_t>(count)};
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

// The selection shapes `expand` reads beside a plain list, and the refusals it
// answers for the shapes it will not take. Spaces and tabs stand outside every
// address of a list (FR-006); a `*` with the rest of the pattern spent matches
// the address it follows (FR-007); a raw event code is refused before the
// catalog is consulted (FR-005); a pattern that matches nothing, an empty
// address line, and an address the catalog holds none of each answer the error
// naming the value it was given (FR-004, FR-006, E-01).
auto selectionShapesAndRefusals() -> void
{
  check(registerHuntHost(), "the scripted hunt host registered");
  const auto catalog = sg::test::huntCatalog();
  const auto first = std::string(catalog[0]);
  const auto second = std::string(catalog[1]);

  auto padded = sg::hunt::EventCatalog {};
  check(padded.expand("  " + first + " ,\t" + second + " \t").has_value(),
        "a list padded around every address expands (FR-006)");
  check(
      padded.addresses().size() == 2 && padded.addresses()[0] == first
          && padded.addresses()[1] == second,
      "the spaces stand outside the addresses the expansion answers (FR-006)");

  auto trailing = sg::hunt::EventCatalog {};
  check(trailing.expand(first + "*").has_value(),
        "a pattern with its star at the end matches (FR-007)");
  check(trailing.addresses().size() == 1 && trailing.addresses()[0] == first,
        "the star at the end admits the one address it covers (FR-007)");

  const auto unmatchedPattern = std::string(sg::test::kHuntCpu) + "/zzz*";
  auto unmatched = sg::hunt::EventCatalog {};
  const auto pattern = unmatched.expand(unmatchedPattern);
  check(!pattern.has_value(),
        "a pattern that matches no catalog address answers an error (FR-004)");
  check(pattern.has_value()
            || pattern.error().message.find(unmatchedPattern)
                != std::string::npos,
        "the refusal of a pattern names the pattern it was given (FR-004)");

  auto blanked = sg::hunt::EventCatalog {};
  const auto blankLine = blanked.expand(first + ",,");
  check(!blankLine.has_value(),
        "an empty address line of a list answers an error (FR-006)");
  check(blankLine.has_value()
            || blankLine.error().message.find("empty address line")
                != std::string::npos,
        "the refusal of an empty line states the shape it refuses (FR-006)");

  const auto missing = std::string(sg::test::kHuntCpu) + "/rose";
  auto unlisted = sg::hunt::EventCatalog {};
  const auto unknown = unlisted.expand(missing);
  check(!unknown.has_value(),
        "an address the catalog holds none of answers an error (FR-004)");
  check(unknown.has_value()
            || unknown.error().message.find(missing) != std::string::npos,
        "the refusal of an address names the address (FR-004)");

  auto untouched = sg::hunt::EventCatalog {};
  const auto rawCode = untouched.expand("r4a03");
  check(
      !rawCode.has_value(),
      "a raw event code answers an error before the catalog is read (FR-005)");
  check(rawCode.has_value()
            || rawCode.error().message.find("raw event code")
                != std::string::npos,
        "the refusal of a raw code states what it was given (FR-005)");
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

// ---------------------------------------------------------------------------
// The budget tables (US2, FR-012, FR-013)
// ---------------------------------------------------------------------------

// The `120s` parse row: the suffix settles the seconds unit and the count, and
// the planner converts the seconds by the calibration figure alone, because 120
// x 1'000'000'000 / 1'920'000 answers 62'500 passes, above the 59 pass runs the
// two-stage AC-7 plan costs, so both stages stand (FR-012, D-2).
auto parseSecondsForm() -> void
{
  const auto budget = settledBudget("120s");
  check(budget.kind == sg::hunt::HuntBudget::Kind::SECONDS,
        "the s suffix settles the seconds unit");
  check(budget.amount == kFitSeconds,
        "the parse keeps the count the option named");
  check(kFitSeconds * kNanosecondsPerSecond / kCalibrationNsPerPass
            >= kAcSevenRuns,
        "the seconds budget hosts the 59 pass runs of the AC-7 plan");
  const auto plan = planned(acSevenColumns(),
                            kAcOneLimit,
                            budgetOptions(budget, kAcSevenTop),
                            "the seconds row plans");
  check(plan.stages.size() == 2, "a seconds budget that fits keeps stage 2");
  check(plan.stages[1].columns.size() == kStageTwoWidth,
        "the kept stage 2 spans three times the top");
}

// The `8p` parse row, the budget of the walkthrough: the suffix settles the
// pass unit, the count needs no conversion, and the 59 runs of the AC-7 plan
// stand above 8, so the plan answers stage 1 alone (FR-012, FR-013).
auto parsePassesForm() -> void
{
  const auto budget = settledBudget("8p");
  check(budget.kind == sg::hunt::HuntBudget::Kind::PASSES,
        "the p suffix settles the pass unit");
  check(budget.amount == kCutPasses,
        "the parse keeps the count the option named");
  const auto plan = planned(acSevenColumns(),
                            kAcOneLimit,
                            budgetOptions(budget, kAcSevenTop),
                            "the passes row plans");
  check(plan.stages.size() == 1,
        "a pass budget below the 59 runs leaves stage 1 alone");
}

// The `1p` parse row: a count of one is the smallest budget above zero, so the
// run still splits into stages and still cannot host the 59 runs. Stage 1 keeps
// every column at its one repeat (FR-012, FR-013).
auto parseOnePassForm() -> void
{
  const auto budget = settledBudget("1p");
  check(budget.kind == sg::hunt::HuntBudget::Kind::PASSES && budget.amount == 1,
        "one pass is the smallest budget the contract admits");
  const auto plan = planned(acSevenColumns(),
                            kAcOneLimit,
                            budgetOptions(budget, kAcSevenTop),
                            "the one-pass row plans");
  check(plan.stages.size() == 1,
        "one pass cannot host the 59 runs of the two-stage plan");
  check(plan.stages.front().columns.size() == sg::test::kHuntEvents,
        "stage 1 of a cut plan still covers every column");
  check(plan.stages.front().repeats == 1, "stage 1 runs one repeat");
}

// `0s` reads as a number, so the count test is the one that refuses it, and the
// refusal quotes the whole value the option carried (FR-012, FR-081).
auto refusesZeroSeconds() -> void
{
  refusesBudget("0s", "--hunt-budget: expected a count above 0, got '0s'");
}

// `0p` is the same refusal in the pass unit (FR-012, FR-081).
auto refusesZeroPasses() -> void
{
  refusesBudget("0p", "--hunt-budget: expected a count above 0, got '0p'");
}

// A value with no suffix names no unit, so no budget record can stand behind
// it: the unit test refuses it before the count is read (FR-012).
auto refusesNoSuffix() -> void
{
  refusesBudget("120", "--hunt-budget: expected <n>s or <n>p, got '120'");
}

// A negative count has no spelling in a hunt count at all: the unit suffix
// stands, so the value reaches the count test and fails it (FR-012).
auto refusesNegativeCount() -> void
{
  refusesBudget("-5p", "--hunt-budget: expected a count above 0, got '-5p'");
}

// A non-numeric value names no count either; `garbage` of AC-3 and `abc` of the
// contract table are the same row (FR-012, FR-081).
auto refusesNonNumeric() -> void
{
  refusesBudget("abc", "--hunt-budget: expected <n>s or <n>p, got 'abc'");
}

// ---------------------------------------------------------------------------
// The stage tables (US2, AC-7, SC-07, D-2, FR-013)
// ---------------------------------------------------------------------------

// AC-7 and SC-07, the row of the walkthrough: 200 events at limit 6 with a top
// of 4 and a budget that fits give stage 1 over every column at one repeat and
// stage 2 over the leading 12 plan columns at the settled repeats. The figures
// are the walkthrough's: 50 passes hold the 200 columns, the 12 stage-2 columns
// stand in 3 of them, and 50 + 3 x 3 = 59 runs fit inside the 60 the budget
// names.
auto acSevenStages() -> void
{
  const auto events = acSevenColumns();
  const auto options = budgetOptions(passBudget(kFitPasses), kAcSevenTop);
  const auto plan = planned(events, kAcOneLimit, options, "the AC-7 row plans");

  check(plan.passes.size() == kAcSevenPasses,
        "200 columns at limit 6 give 50 passes");
  check(plan.stages.size() == 2, "a budget that hosts the 59 runs keeps both");
  check(plan.stages[0].columns.size() == sg::test::kHuntEvents,
        "stage 1 covers every column");
  check(plan.stages[0].repeats == 1, "stage 1 measures one repeat");
  check(plan.stages[1].columns.size() == kStageTwoWidth,
        "stage 2 covers three times the top");
  check(plan.stages[1].repeats == kDefaultRepeats,
        "stage 2 measures the settled repeats");
  check(std::ranges::is_sorted(plan.stages[1].columns),
        "stage 2 lists its columns in column order");

  // What the run drives: the runner is called once per stage of this plan
  // (PD-13), so stage 2 measures the leading `3 x top` columns of the plan
  // order. E-08 records a stage as `first`, `count` and `repeats`, a shape
  // that describes a contiguous leading set and no other (FR-013, FR-014).
  std::vector<std::size_t> plannedStageTwo;
  for (std::size_t index = 0; index < kStageTwoWidth; ++index) {
    plannedStageTwo.push_back(index);
  }
  check(plan.stages[1].columns == plannedStageTwo,
        "stage 2 is the leading 3 x top columns the run measures");

  // The seam beneath the plan, reached only from this row: the same split
  // called directly answers stage 2 with the leading `3 x top` columns, the
  // set the plan itself holds, because FR-014 derives a stage from the seed
  // and the catalog order only and no measured figure reaches the split
  // (D-2, FR-013).
  const auto split =
      sg::detail::splitStages(plan, options, kCalibrationNsPerPass);
  check(!split.cutStage2, "the row's budget hosts the two-stage run");
  check(split.stages.size() == 2, "the split of the measured plan keeps both");
  check(split.stages[1].columns == plannedStageTwo,
        "the seam takes stage 2 as the leading set of the plan order");
}

// The `3 x top` set capped at the event count: a top of 2 asks for 6 stage-2
// columns and the row holds 4, so stage 2 measures all 4 at the settled
// repeats. The 4 columns stand in the one pass of the plan, so the run costs 1
// pass of stage 1 plus 1 x 3 of stage 2: 4 runs. A 4-pass budget hosts them and
// a 3-pass one drops stage 2 whole, stage 1 keeping its full width (E-04,
// FR-013).
auto stageTwoSetCapped() -> void
{
  const auto events = acSevenColumns(kRowEvents);
  const auto wide = planned(events,
                            kAcOneLimit,
                            budgetOptions(passBudget(4), 2),
                            "the capped row plans");
  check(wide.passes.size() == 1, "four columns at limit 6 stand in one pass");
  check(wide.stages.size() == 2,
        "a budget that hosts the 4 runs keeps stage 2");
  check(wide.stages[1].columns.size() == kRowEvents,
        "the 3 x top set is capped at the column count");
  check(wide.stages[1].repeats == kDefaultRepeats,
        "the capped set measures the settled repeats");

  const auto tight = planned(events,
                             kAcOneLimit,
                             budgetOptions(passBudget(3), 2),
                             "the tight capped row plans");
  check(tight.stages.size() == 1,
        "a budget below the 4 runs drops the capped stage whole");
  check(tight.stages.front().columns.size() == kRowEvents,
        "the cut plan keeps stage 1 whole");
}

// An estimate that does not fit the budget answers one stage with the cut
// verdict, stage 2 absent and stage 1 whole: the walkthrough row of the 8-pass
// budget over the AC-7 plan, whose 59 runs stand above 8. The
// verdict names the reason E-07 prints as `reason=over-budget`, and the plan
// line still states the stage-1 figure of 1'920'000 x 50 = 96'000'000 ns (D-2,
// FR-013).
auto overBudgetCut() -> void
{
  const auto events = acSevenColumns();
  const auto options = budgetOptions(passBudget(kCutPasses), kAcSevenTop);
  const auto plan = planned(events, kAcOneLimit, options, "the cut row plans");

  check(plan.stages.size() == 1,
        "the unfit budget leaves one stage of the plan");
  check(plan.stages.front().columns.size() == sg::test::kHuntEvents,
        "stage 1 of a cut plan is not shortened");
  check(plan.stages.front().repeats == 1, "stage 1 keeps its one repeat");
  check(plan.estimateNs == kCalibrationNsPerPass * kAcSevenPasses,
        "the cut plan states the stage-1 estimate of the walkthrough");

  const auto split =
      sg::detail::splitStages(plan, options, kCalibrationNsPerPass);
  check(split.stages.size() == 1, "the split answers one stage");
  check(split.stages.front().columns.size() == sg::test::kHuntEvents,
        "the split drops the second stage whole, never the first");
  check(split.cutStage2, "the split carries the cut verdict");
  check(split.cutReason == "over-budget",
        "the verdict names the reason the cut line prints");
}

// FR-013: stage 1 never shrinks whatever the budget, and no stage carries a
// reduced repeat count. The 59 runs of the AC-7 plan are the boundary, so 58
// passes cut and 59 fit. Over every budget of the table stage 1 holds all 200
// columns, at one repeat of a budgeted run and at the full 3 of an un-budgeted
// one.
auto stageOneNeverShrinks() -> void
{
  struct Row
  {
    std::optional<sg::hunt::HuntBudget> budget;
    std::size_t stages;
  };

  const Row rows[] = {
      Row {std::nullopt, 1},
      Row {passBudget(1), 1},
      Row {passBudget(kCutPasses), 1},
      Row {passBudget(kAcSevenRuns - 1), 1},
      Row {passBudget(kAcSevenRuns), 2},
      Row {passBudget(kFitPasses), 2},
      Row {secondsBudget(kFitSeconds), 2},
  };
  const auto events = acSevenColumns();
  for (const auto& row : rows) {
    const auto plan = planned(events,
                              kAcOneLimit,
                              budgetOptions(row.budget, kAcSevenTop),
                              "every budget of the table plans");
    check(plan.stages.size() == row.stages,
          "the budget decides the stage count and nothing else");
    check(plan.stages.front().columns.size() == sg::test::kHuntEvents,
          "stage 1 never shrinks");
    check(plan.stages.front().repeats
              == (row.budget.has_value() ? std::size_t {1} : kDefaultRepeats),
          "stage 1 runs one repeat when budgeted and all repeats otherwise");
    if (plan.stages.size() == 2) {
      check(plan.stages[1].columns.size() == kStageTwoWidth,
            "a standing stage 2 holds three times the top");
      check(plan.stages[1].repeats == kDefaultRepeats,
            "no stage keeps a reduced repeat count");
    }
  }
}

// FR-012: `estimateNs` is the calibration figure times the passes times the
// repeats, with the stage-1 figures on a split plan. The fixed calibration
// answers 1'920'000 x 50 x 3 = 288'000'000 ns for the plan of one stage and the
// walkthrough figure 1'920'000 x 50 x 1 = 96'000'000 ns for the split one.
auto estimateStageOneFigures() -> void
{
  const auto events = acSevenColumns();
  const auto whole = planned(events,
                             kAcOneLimit,
                             budgetOptions(std::nullopt, kAcSevenTop),
                             "the un-budgeted plan stands");
  check(whole.stages.size() == 1, "no budget answers one stage");
  check(whole.stages.front().repeats == kDefaultRepeats,
        "the one stage measures the full repeats");
  check(whole.estimateNs
            == kCalibrationNsPerPass * kAcSevenPasses * kDefaultRepeats,
        "the estimate is calibration times passes times repeats");

  const auto split = planned(events,
                             kAcOneLimit,
                             budgetOptions(passBudget(kFitPasses), kAcSevenTop),
                             "the split plan stands");
  check(split.stages.size() == 2, "the fit budget splits the plan");
  check(split.estimateNs == kCalibrationNsPerPass * kAcSevenPasses,
        "a split plan states the stage-1 figures");
}

// FR-012, I-7: no calibration figure answers no estimate, because no clock
// stands here to supply one, so `estimateNs` stays empty and the report prints
// `estimate=not available`. A seconds budget with nothing to convert by bounds
// nothing the planner can weigh, so the plan does not fit it: stage 2 goes
// whole and the verdict carries the reason E-07 prints as
// `reason=over-budget` (D-2, D-19, FR-013). A calibration figure of zero
// behaves the same way in the conversion and still answers an estimate of zero.
auto estimateWithoutCalibration() -> void
{
  const auto events = acSevenColumns();
  const auto limits = sg::test::huntLimits(kAcOneLimit);
  const auto anchors = sg::test::huntAnchors();
  const auto options = budgetOptions(secondsBudget(kFitSeconds), kAcSevenTop);

  const auto open =
      sg::hunt::planHunt(events, limits, anchors, options, std::nullopt);
  check(open.has_value(), "the plan stands without a calibration figure");
  check(open.has_value() && !open->estimateNs.has_value(),
        "no calibration figure answers no estimate");
  check(open.has_value() && open->stages.size() == 1,
        "a seconds budget with nothing to convert by drops stage 2 whole");

  if (open.has_value()) {
    const auto split = sg::detail::splitStages(*open, options, std::nullopt);
    check(split.stages.size() == 1
              && split.stages.front().columns.size() == sg::test::kHuntEvents,
          "the unpriced budget leaves stage 1 whole at the full width");
    check(split.cutStage2 && split.cutReason == "over-budget",
          "the unpriced budget carries the reason the cut line prints");
  }

  const auto zero =
      sg::hunt::planHunt(events, limits, anchors, options, std::uint64_t {0});
  check(zero.has_value() && zero->estimateNs.has_value()
            && *zero->estimateNs == 0,
        "a calibration figure of zero answers an estimate of zero");
  check(zero.has_value() && zero->stages.size() == 1,
        "a zero calibration figure prices no seconds budget, so stage 2 goes");
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
      Scenario {"selection-shapes", selectionShapesAndRefusals},
      Scenario {"seed-order", seedOrder},
      Scenario {"core-device-rule", coreDeviceRule},
      Scenario {"norm-anchor-refused", normAnchorRefused},
      Scenario {"uncore-list-own-device", uncoreListOwnDevice},
      Scenario {"budget-seconds-form", parseSecondsForm},
      Scenario {"budget-passes-form", parsePassesForm},
      Scenario {"budget-one-pass-form", parseOnePassForm},
      Scenario {"budget-zero-seconds", refusesZeroSeconds},
      Scenario {"budget-zero-passes", refusesZeroPasses},
      Scenario {"budget-no-suffix", refusesNoSuffix},
      Scenario {"budget-negative-count", refusesNegativeCount},
      Scenario {"budget-non-numeric", refusesNonNumeric},
      Scenario {"budget-two-stage-plan", acSevenStages},
      Scenario {"stage-two-set-capped", stageTwoSetCapped},
      Scenario {"over-budget-cut-verdict", overBudgetCut},
      Scenario {"stage-one-never-shrinks", stageOneNeverShrinks},
      Scenario {"estimate-stage-one-figures", estimateStageOneFigures},
      Scenario {"estimate-no-calibration", estimateWithoutCalibration},
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
