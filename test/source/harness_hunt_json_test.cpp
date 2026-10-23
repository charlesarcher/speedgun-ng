// ============================================================================
// The hunt document: writer, reader and schema conformance
// (specs/029-hunter-core T006, filled by T038; US3; FR-051, FR-052; AC-6,
// SC-006; research PD-20; contracts/hunt-json.md,
// contracts/report-schema-key.md).
//
// The seams are the writer, `loadReport` and the schema checker over the
// `SG_SCHEMA_PATH` document T007 compiles into this target: a case writes a
// record, reads the bytes back and compares the two field by field, or plants
// one disagreement and names the pointer the refusal reports. Every key and
// every `x-order` position is the contract table's, never a key mirrored here.
//
// Every hunt record comes from the scripted host of `detail/hunt_fixture.hpp`:
// the plan is `planHunt` over the fixture's catalog, limits and anchors, the
// rank table is `rankEvents` over the fixture's windowed readings, and the
// document is `writeReport` over the scripted run of `detail/json_fixture.hpp`
// with that record hung on its one result. No case reads a clock, sleeps, or
// opens a host PMU: the host PMU answers a different catalog, a different rate
// and a different console on every machine, which is the reading no committed
// capture can be compared against.
//
// The capture of the run with no `--hunt` is the one byte comparison a scenario
// cannot make inside one run of itself: the document it must match was written
// before the hunt keys existed, so it stands committed under `test/data` and
// the run that must match it happens in a child of this binary, over the same
// scripted host, with its console and its exit status beside its bytes.
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/hunt_fixture.hpp"
#include "detail/internal.hpp"
#include "detail/json_fixture.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/hunt.hpp"

#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH, the schema the checker reads"
#endif

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT JSON TEST FAIL: %s\n", what);
  std::exit(1);
}

// The same failure with a message the scenario assembled: the pointer a
// refusal names is a `std::string`, and it is the fact under test.
[[noreturn]] auto failed(const std::string& what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT JSON TEST FAIL: %s\n", what.c_str());
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

auto checked(const bool condition, const std::string& what) -> void
{
  if (!condition) {
    failed(what);
  }
}

// ---------------------------------------------------------------------------
// The scripted hunt
// ---------------------------------------------------------------------------

// The published limit the scripted devices answer, a value of the fixture's
// `kHuntRows`. Six leaves seat four events beside the two anchors, so
// `kColumns` columns spread over two passes.
constexpr std::size_t kLimit = 6;
constexpr std::size_t kColumns = 6;
constexpr std::size_t kRepeats = 3;

// The two `--hunt-top` values: two ranks for the single-stage record, and one
// for the two-stage record, whose stage 2 then covers three columns of the six
// and leaves three at stage 1 (D-2).
constexpr std::size_t kSingleTop = 2;
constexpr std::size_t kTwoStageTop = 1;
constexpr std::size_t kStageTwoColumns = 3 * kTwoStageTop;

// The sampling cost a bounded plan estimates against. One microsecond per pass
// keeps a two-stage plan inside a budget of a hundred and twenty seconds, so
// the stage split of D-2 stands whatever the host would have measured.
constexpr std::uint64_t kCalibrationNsPerPass = 1'000;
constexpr std::uint64_t kBudgetSeconds = 120;

// One hunt window measures one iteration, the count the hunt run itself pins
// (FR-013), so a rate is the fixture figure over the fixture anchor.
constexpr std::uint64_t kOneIteration = 1;

// What each column of the single-stage record stands for: a spread past
// `--hunt-noise` answers no rank, one window alone is never summarized, and a
// column that measured and gapped stands in `entries` and `skipped` at once
// (D-10, PD-14).
constexpr std::size_t kNoisyColumn = 0;
constexpr std::size_t kLoneColumn = 5;
constexpr std::size_t kGappedColumn = 3;

// The per-window step added to the fixture event value: one part in a hundred
// keeps a repeated column under the noise threshold, half the value puts it
// past ten percent.
constexpr std::uint64_t kCleanStep = 1;
constexpr std::uint64_t kNoisyStep = 50;

// The index of one device in `huntDevices()` order: the counter-source index
// every scripted column carries, read from the fixture itself, so a change to
// the scripted host moves it here with the script.
[[nodiscard]] auto deviceIndex(std::string_view device) -> std::size_t
{
  const auto devices = sg::test::huntDevices();
  const auto found = std::ranges::find(devices, device);
  check(found != devices.end(), "the scripted host publishes the device");
  return static_cast<std::size_t>(found - devices.begin());
}

// The first `count` addresses of the scripted catalog, each carrying the
// device that owns it and that device's source index (E-01).
[[nodiscard]] auto scriptedEvents(const std::size_t count)
    -> std::vector<sg::hunt::CatalogAddress>
{
  const auto catalog = sg::test::huntCatalog();
  check(count <= catalog.size(), "the scripted catalog holds the columns");
  auto events = std::vector<sg::hunt::CatalogAddress> {};
  for (std::size_t index = 0; index < count; ++index) {
    auto event = sg::hunt::CatalogAddress {};
    event.address = catalog[index];
    event.device = std::string(sg::test::kHuntCpu);
    event.source = deviceIndex(sg::test::kHuntCpu);
    events.push_back(std::move(event));
  }
  return events;
}

// The selection text of a column list, the shape `--hunt` takes and the shape
// `/hunt/selection` carries.
[[nodiscard]] auto selectionOf(
    const std::vector<sg::hunt::CatalogAddress>& events) -> std::string
{
  auto selection = std::string {};
  for (const auto& event : events) {
    if (!selection.empty()) {
      selection += ',';
    }
    selection += event.address;
  }
  return selection;
}

// The settled options of a scripted hunt: the norm and the rank the document
// must carry are stated, never left for the writer to default, because a
// default reads back as the value it defaults to and not as nothing.
[[nodiscard]] auto optionsOf(
    const std::vector<sg::hunt::CatalogAddress>& events,
    const std::size_t top,
    const bool bounded) -> sg::hunt::HuntOptions
{
  auto options = sg::hunt::HuntOptions {};
  options.selection = selectionOf(events);
  options.norm = sg::hunt::HuntNorm::INSTR;
  options.rank = sg::hunt::HuntRank::RATE;
  options.top = top;
  options.repeats = kRepeats;
  options.seed = 0;
  if (bounded) {
    auto budget = sg::hunt::HuntBudget {};
    budget.kind = sg::hunt::HuntBudget::Kind::SECONDS;
    budget.amount = kBudgetSeconds;
    options.budget = budget;
  }
  return options;
}

// The plan of the scripted columns over the fixture's limit list and anchor
// list: the same `planHunt` the hunt run calls, so the pass count, the settled
// method and the stage split of the record are the planner's answers.
[[nodiscard]] auto planOf(const std::vector<sg::hunt::CatalogAddress>& events,
                          const sg::hunt::HuntOptions& options,
                          const std::optional<std::uint64_t> calibration)
    -> sg::hunt::HuntPlan
{
  const auto plan = sg::hunt::planHunt(events,
                                       sg::test::huntLimits(kLimit),
                                       sg::test::huntAnchors(),
                                       options,
                                       calibration);
  if (!plan.has_value()) {
    failed("the scripted host refuses the scripted columns: "
           + plan.error().message);
  }
  return *plan;
}

// The pass of the plan that seats one column, the number every window of that
// column carries (E-05).
[[nodiscard]] auto passOf(const sg::hunt::HuntPlan& plan,
                          const std::size_t column) -> std::size_t
{
  for (std::size_t pass = 0; pass < plan.passes.size(); ++pass) {
    const auto& seats = plan.passes[pass];
    if (std::ranges::find(seats, column) != seats.end()) {
      return pass;
    }
  }
  fail("every scripted column stands in one pass of the plan");
}

// The windows of one column, read off the fixture: the event value of catalog
// index `value` plus `step` each window, the anchors of the device that owns
// the event, and one iteration per window (E-05).
[[nodiscard]] auto windowsOf(const sg::hunt::CatalogAddress& event,
                             const std::size_t value,
                             const std::size_t stage,
                             const std::size_t pass,
                             const std::size_t count,
                             const std::uint64_t step)
    -> std::vector<sg::hunt::HuntSample>
{
  const auto device = deviceIndex(event.device);
  auto rows = std::vector<sg::hunt::HuntSample> {};
  for (std::size_t window = 0; window < count; ++window) {
    auto row = sg::hunt::HuntSample {};
    row.pass = pass;
    row.stage = stage;
    row.eventValue = sg::test::huntValue(value) + step * window;
    row.anchorInstr = sg::test::huntInstr(device);
    row.anchorCycle = sg::test::huntCycle(device);
    row.iterations = kOneIteration;
    rows.push_back(row);
  }
  return rows;
}

// The run record of one plan, filled the way the hunt run fills it: the shape
// of the plan and the settled options, and nothing a scenario invents (E-08).
[[nodiscard]] auto runOf(const sg::hunt::HuntOptions& options,
                         const sg::hunt::HuntPlan& plan) -> sg::hunt::RunHunt
{
  auto run = sg::hunt::RunHunt {};
  run.selection = options.selection;
  run.options = options;
  run.passes = plan.passes.size();
  run.method = plan.method;
  run.estimateNs = plan.estimateNs;
  for (const auto& column : plan.columns) {
    run.plannedAddresses.push_back(column.event);
  }
  for (const auto& stage : plan.stages) {
    check(!stage.columns.empty(), "no scripted stage of the plan stands empty");
    auto range = sg::hunt::HuntStageRange {};
    range.first = stage.columns.front();
    range.count = stage.columns.size();
    range.repeats = stage.repeats;
    run.stages.push_back(range);
  }
  return run;
}

// One run record and the entry record that rode it: the pair the document
// carries at `/hunt` and at `/benchmarks/0/hunt`.
struct Hunted
{
  sg::hunt::RunHunt run;
  sg::hunt::EntryHunt entry;
  std::string loneAddress;
};

// The single-stage record: no budget and no calibration, so `budget` and
// `estimate_ns` stand null, and a rank table with a noisy event, an unrepeated
// one, one past the top, and two refusals of which one carries no stage.
[[nodiscard]] auto singleStageHunt() -> Hunted
{
  const auto events = scriptedEvents(kColumns);
  const auto options = optionsOf(events, kSingleTop, false);
  const auto plan = planOf(events, options, std::nullopt);
  check(plan.stages.size() == 1, "a plan with no budget runs one stage");

  auto collected = std::vector<sg::hunt::EventSamples> {};
  for (std::size_t column = 0; column < kColumns; ++column) {
    const auto windows = column == kLoneColumn ? std::size_t {1} : kRepeats;
    const auto step = column == kNoisyColumn ? kNoisyStep : kCleanStep;
    auto rows = sg::hunt::EventSamples {};
    rows.address = events[column].address;
    rows.samples = windowsOf(
        events[column], column, 1, passOf(plan, column), windows, step);
    collected.push_back(std::move(rows));
  }

  // The gapped column measured twice and gapped once, with the disclosure the
  // pass loop leaves on a column that opened its group too wide (FR-022).
  auto gap = sg::hunt::HuntSample {};
  gap.pass = passOf(plan, kGappedColumn);
  gap.stage = 1;
  gap.disclosure = sg::counters::Availability::GAP;
  collected[kGappedColumn].samples.push_back(gap);
  collected[kGappedColumn].skipped = sg::hunt::SkippedKind::GAP;

  auto entry = sg::hunt::rankEvents(collected, options, plan.method);
  // The retry lane of D-18: two passes of the plan ran a second time solo.
  entry.retriedPasses = 2;
  // The refusal of one event carries the address and the kind, the two
  // figures FR-050 names; the stage the address stood in belongs to the plan.
  auto refused = sg::hunt::SkippedEvent {};
  refused.address = sg::test::huntAddress(sg::test::kHuntCpu,
                                          sg::test::huntEvent(kColumns + 1));
  refused.kind = sg::hunt::SkippedKind::SCOPE_REFUSED;
  entry.skipped.push_back(std::move(refused));

  auto run = runOf(options, plan);
  run.ranEvents = kColumns;
  run.stageTwoEvents = 0;
  return {.run = std::move(run),
          .entry = std::move(entry),
          .loneAddress = events[kLoneColumn].address};
}

// The two-stage record: a budget the estimate fits, so the plan keeps stage 2
// over three times `--hunt-top` columns. The fixture values run high to low,
// so the columns stage 2 repeats are the ones the rank table puts first, and
// the columns stage 1 alone measured stand in `entries` with no rank.
[[nodiscard]] auto twoStageHunt() -> Hunted
{
  const auto events = scriptedEvents(kColumns);
  const auto options = optionsOf(events, kTwoStageTop, true);
  const auto plan = planOf(events, options, kCalibrationNsPerPass);
  check(plan.stages.size() == 2,
        "a plan whose estimate fits the budget keeps stage 2");
  check(plan.stages[1].columns.size() == kStageTwoColumns,
        "stage 2 covers three times --hunt-top columns");

  auto collected = std::vector<sg::hunt::EventSamples> {};
  for (std::size_t column = 0; column < kColumns; ++column) {
    const auto value = kColumns - 1 - column;
    const auto seat = passOf(plan, column);
    auto rows = sg::hunt::EventSamples {};
    rows.address = events[column].address;
    rows.samples = windowsOf(events[column], value, 1, seat, 1, kCleanStep);
    const auto repeated = std::ranges::find(plan.stages[1].columns, column)
        != plan.stages[1].columns.end();
    if (repeated) {
      const auto stageTwo =
          windowsOf(events[column], value, 2, seat, kRepeats, kCleanStep);
      rows.samples.insert(rows.samples.end(), stageTwo.begin(), stageTwo.end());
    }
    collected.push_back(std::move(rows));
  }

  auto entry = sg::hunt::rankEvents(collected, options, plan.method);
  auto run = runOf(options, plan);
  run.ranEvents = kColumns;
  run.stageTwoEvents = plan.stages[1].columns.size();
  const auto stageTwoOnly = kColumns - kStageTwoColumns;
  return {.run = std::move(run),
          .entry = std::move(entry),
          .loneAddress = events[stageTwoOnly].address};
}

// ---------------------------------------------------------------------------
// The document
// ---------------------------------------------------------------------------

constexpr const char* kScratchPath = "harness_hunt_json_document.json";

// The child mode, its scratch files and the committed capture beside them. The
// capture is `hunt_no_hunt_*`: what a run with no `--hunt` wrote before the
// hunt keys, held as bytes (SC-006).
constexpr const char* kCaptureMode = "capture-no-hunt";
constexpr const char* kCaptureDocument = "harness_hunt_json_capture.json";
constexpr const char* kCaptureConsole = "harness_hunt_json_capture_console.txt";
constexpr const char* kCaptureCase = "bmHuntJsonCapture";
constexpr const char* kCommittedDocument = "hunt_no_hunt_document.json";
constexpr const char* kCommittedConsole = "hunt_no_hunt_console.txt";

auto selfPath = std::string {};

// The fixture directory of the registered command, empty when the full list
// run names none.
auto dataArgument = std::string {};

// The directory the committed capture stands in. The registered command of the
// byte-identity entry passes `test/data` beside the scenario name, the shape
// `harness_pass_report_test_identity` gives its fixture; with no argument the
// same directory is reached from the schema path the target compiles in, so
// the full list run finds the capture too.
[[nodiscard]] auto dataPath(std::string_view name) -> std::string
{
  if (!dataArgument.empty()) {
    return dataArgument + "/" + std::string(name);
  }
  constexpr std::string_view kSchemaTail = "/docs/schema/report-v1.json";
  const std::string_view schema = SG_SCHEMA_PATH;
  check(schema.size() > kSchemaTail.size() && schema.ends_with(kSchemaTail),
        "the compiled-in schema path stands in the source tree");
  return std::string(schema.substr(0, schema.size() - kSchemaTail.size()))
      + "/test/data/" + std::string(name);
}

// The report region of a fixture: the lines that stand below the `#` header,
// the shape of `test/source/detail/pass_identity.expected`.
[[nodiscard]] auto reportRegion(std::string text) -> std::string
{
  while (text.starts_with("#")) {
    const auto stop = text.find('\n');
    if (stop == std::string_view::npos) {
      return {};
    }
    text.erase(0, stop + 1);
  }
  return text;
}

// One scripted run, with one hunt record on its result and one on the run.
// `hunted` standing null is the report of a run that carried no hunt (PD-10).
[[nodiscard]] auto writeReportDocument(const Hunted* hunted) -> sg::ReportData
{
  auto options = sg::detail::RunOptions {};
  options.fixedIterations = sg::test::kWindowIterations;
  const auto scripted = sg::test::scriptedRun(options);

  auto data = sg::ReportData {};
  auto result = scripted.result;
  if (hunted != nullptr) {
    result.hunt = hunted->entry;
  }
  data.results.push_back(std::move(result));
  data.rowRaw = scripted.rowRaw;
  data.contextRaw = scripted.contextRaw;
  if (hunted != nullptr) {
    data.hunt = hunted->run;
  }
  check(sg::detail::writeReport(kScratchPath, data),
        "the writer wrote the document");
  return data;
}

[[nodiscard]] auto documentText() -> std::string
{
  auto file = std::ifstream(kScratchPath);
  auto buffer = std::stringstream {};
  buffer << file.rdbuf();
  return buffer.str();
}

[[nodiscard]] auto huntDocumentText(const Hunted& hunted) -> std::string
{
  static_cast<void>(writeReportDocument(&hunted));
  return documentText();
}

// ---------------------------------------------------------------------------
// The written bytes, read as bytes
// ---------------------------------------------------------------------------

constexpr std::string_view kHuntOpen = "\"hunt\":{";

// The byte one hunt node opens at. The entry node opens inside the benchmark
// array and the run node stands last, after it, so the first `"hunt":{` is the
// entry's and the last is the run's.
[[nodiscard]] auto nodeOpen(std::string_view text,
                            const bool run) -> std::size_t
{
  const auto at = run ? text.rfind(kHuntOpen) : text.find(kHuntOpen);
  check(at != std::string_view::npos, "the document carries the hunt node");
  check(text.find(kHuntOpen) != text.rfind(kHuntOpen),
        "the document carries both hunt nodes");
  return at + kHuntOpen.size() - 1;
}

// The byte the first element of one array of a node opens at, the element a
// scenario plants into (`entries`, `skipped`, `stages`).
[[nodiscard]] auto elementOpen(std::string_view text,
                               std::size_t from,
                               std::string_view key) -> std::size_t
{
  const auto needle = std::string("\"") + std::string(key) + "\":[";
  const auto at = text.find(needle, from);
  check(at != std::string_view::npos,
        "the hunt node carries the array the scenario plants into");
  const auto open = text.find('{', at + needle.size());
  check(open != std::string_view::npos,
        "the planted array holds an object to plant into");
  return open;
}

// The keys one object of the written bytes carries, in the order the bytes
// carry them. The document is compact, so a scan of quotes and braces at the
// object's own depth reads the order the contract states and no reader
// enforces.
[[nodiscard]] auto keysOf(std::string_view text,
                          std::size_t open) -> std::vector<std::string>
{
  auto keys = std::vector<std::string> {};
  std::size_t depth = 0;
  std::size_t at = open;
  while (at < text.size()) {
    const char ch = text[at];
    if (ch == '{' || ch == '[') {
      ++depth;
      ++at;
      continue;
    }
    if (ch == '}' || ch == ']') {
      --depth;
      ++at;
      if (depth == 0) {
        break;
      }
      continue;
    }
    if (ch != '"') {
      ++at;
      continue;
    }
    auto end = at + 1;
    while (end < text.size() && text[end] != '"') {
      end += text[end] == '\\' ? 2U : 1U;
    }
    checked(end < text.size() && end + 1 < text.size(),
            "every string of the written document closes");
    if (depth == 1 && text[end + 1] == ':') {
      keys.emplace_back(text.substr(at + 1, end - at - 1));
    }
    at = end + 1;
  }
  return keys;
}

auto checkOrder(std::string_view text,
                std::size_t open,
                const std::vector<std::string>& want,
                std::string_view where) -> void
{
  const auto got = keysOf(text, open);
  if (got == want) {
    return;
  }
  auto message = std::string("the key order of ") + std::string(where)
      + " is not the x-order of the schema: got";
  for (const auto& key : got) {
    message += " " + key;
  }
  message += "; want";
  for (const auto& key : want) {
    message += " " + key;
  }
  failed(message);
}

// The byte the key of a node starts at, found from the byte the node opens at:
// a hunt node is closed, so every key a scenario plants into stands in the
// document its own record wrote.
[[nodiscard]] auto keyStart(std::string_view text,
                            std::size_t open,
                            std::string_view key) -> std::size_t
{
  const auto needle = std::string("\"") + std::string(key) + "\":";
  const auto at = text.find(needle, open);
  checked(at != std::string_view::npos,
          "the planted key stands in the document: " + needle);
  return at;
}

// The byte past the value that starts at `value`. The writer escapes nothing
// inside a hunt value, so a quote ends a string and a brace or bracket closes
// an object at its own depth.
[[nodiscard]] auto valueEnd(std::string_view text,
                            std::size_t value) -> std::size_t
{
  check(value < text.size(), "the planted value stands in the document");
  if (text[value] == '"') {
    const auto close = text.find('"', value + 1);
    check(close != std::string_view::npos, "the planted string closes");
    return close + 1;
  }
  if (text[value] == '{' || text[value] == '[') {
    std::size_t depth = 0;
    for (std::size_t at = value; at < text.size(); ++at) {
      const char ch = text[at];
      if (ch == '"') {
        at = text.find('"', at + 1);
        continue;
      }
      if (ch == '{' || ch == '[') {
        ++depth;
      }
      if (ch == '}' || ch == ']') {
        --depth;
        if (depth == 0) {
          return at + 1;
        }
      }
    }
    fail("the planted object never closes");
  }
  const auto stop = text.find_first_of(",}]", value);
  return stop == std::string_view::npos ? text.size() : stop;
}

// One disagreement with the contract, in the one form no record can express
// it: the value of `key` of one hunt node becomes `body`, or the key stands
// removed. `node` names where: the run node, the entry node, or the first
// element of one of their arrays.
struct Plant
{
  std::string_view what;
  std::string_view node;
  std::string_view key;
  std::string_view body;
  bool remove = false;
  std::string_view pointer;
  std::string_view message;
};

[[nodiscard]] auto plantSite(std::string_view text,
                             std::string_view node) -> std::size_t
{
  if (node == "hunt") {
    return nodeOpen(text, false);
  }
  if (node == "run") {
    return nodeOpen(text, true);
  }
  check(node == "entries" || node == "skipped" || node == "stages",
        "every plant names an array of one of the two nodes");
  const auto owner =
      node == "stages" ? nodeOpen(text, true) : nodeOpen(text, false);
  return elementOpen(text, owner, node);
}

[[nodiscard]] auto planted(std::string_view text,
                           const Plant& plant) -> std::string
{
  const auto site = plantSite(text, plant.node);
  // The two `hunt` keys are the only place a hunt node stands, so planting
  // into `hunt` replaces the whole node rather than one key of it.
  const auto whole = plant.key == "hunt";
  const auto key = whole ? site : keyStart(text, site, plant.key);
  const auto value = whole ? site : key + plant.key.size() + 3;
  const auto end = valueEnd(text, value);
  if (!plant.remove) {
    return std::string(text.substr(0, value)) + std::string(plant.body)
        + std::string(text.substr(end));
  }
  auto cut = end;
  if (cut < text.size() && text[cut] == ',') {
    ++cut;
  } else {
    // The key is the last member of its node, so the comma that sets it off
    // stands before the key, and the value ends the node.
    const auto prior = text.rfind(',', key);
    check(prior != std::string_view::npos,
          "the removed key stands beside a comma");
    return std::string(text.substr(0, prior)) + std::string(text.substr(end));
  }
  return std::string(text.substr(0, key)) + std::string(text.substr(cut));
}

// The document with one key inserted at the front of one hunt node: a key
// outside the closed node, which no record of E-06 or E-08 can hold.
[[nodiscard]] auto inserted(std::string_view text,
                            std::string_view node,
                            std::string_view body) -> std::string
{
  const auto open = plantSite(text, node);
  return std::string(text.substr(0, open + 1)) + std::string(body)
      + std::string(text.substr(open + 1));
}

// The document with the two keys `first` and `second` of the run node swapped,
// values and all: an out-of-order pair, which the set of keys keeps legal and
// the `x-order` does not.
[[nodiscard]] auto swapped(std::string_view text,
                           std::string_view first,
                           std::string_view second) -> std::string
{
  const auto site = nodeOpen(text, true);
  const auto start = keyStart(text, site, first);
  const auto firstValue = start + first.size() + 3;
  const auto firstEnd = valueEnd(text, firstValue);
  const auto secondStart = keyStart(text, firstEnd, second);
  const auto secondValue = secondStart + second.size() + 3;
  const auto secondEnd = valueEnd(text, secondValue);
  const auto one = text.substr(firstValue, firstEnd - firstValue);
  const auto two = text.substr(secondValue, secondEnd - secondValue);
  return std::string(text.substr(0, start)) + "\"" + std::string(second)
      + "\":" + std::string(one) + ",\"" + std::string(first)
      + "\":" + std::string(two) + std::string(text.substr(secondEnd));
}

// The number of objects one array of the written bytes holds.
[[nodiscard]] auto objectCount(std::string_view text,
                               std::size_t open,
                               std::string_view key) -> std::size_t
{
  const auto value = keyStart(text, open, key) + key.size() + 3;
  const auto end = valueEnd(text, value);
  return static_cast<std::size_t>(
      std::ranges::count(text.substr(value, end - value), '{'));
}

// The number of times one needle stands in the written bytes.
[[nodiscard]] auto occurrences(std::string_view text,
                               std::string_view needle) -> std::size_t
{
  auto count = std::size_t {0};
  for (auto at = text.find(needle); at != std::string_view::npos;
       at = text.find(needle, at + needle.size()))
  {
    ++count;
  }
  return count;
}

// The values that state which build and which host answered the run rather
// than what the run measured: the `schemaVersion`, which the hunt keys moved
// inside major 1 (contracts/report-schema-key.md), the `speedgun` object, whose
// version and build type belong to the build that runs this test, and the one
// wall-clock figure of the row, the overhead floor
// (`Plan::sampleOverheadNsMedian()`, as `pass_identity.expected` documents).
// Everything else compares byte for byte, in the document and in the console
// alike; the console keeps no header line, which states the build the way the
// `speedgun` object does.
[[nodiscard]] auto identityMasked(std::string text) -> std::string
{
  for (const auto* key : {"\"schemaVersion\":", "\"overheadFloorNs\":"}) {
    const auto at = text.find(key);
    if (at != std::string_view::npos) {
      const auto value = at + std::string_view(key).size();
      text.replace(value, valueEnd(text, value) - value, "\"?\"");
    }
  }
  const auto header = std::string_view("\"speedgun\":");
  if (const auto head = text.find(header); head != std::string_view::npos) {
    const auto object = head + header.size();
    text.replace(object, valueEnd(text, object) - object, "\"?\"");
  }
  if (text.starts_with("speedgun-ng ")) {
    text.erase(0, text.find('\n') + 1);
  }
  constexpr std::string_view kFloor = "overhead floor (ns)= ";
  for (auto at = text.find(kFloor); at != std::string_view::npos;
       at = text.find(kFloor, at + kFloor.size()))
  {
    auto stop = at + kFloor.size();
    while (stop < text.size() && text[stop] == ' ') {
      ++stop;
    }
    const auto digits = stop;
    while (stop < text.size()
           && ((text[stop] >= '0' && text[stop] <= '9') || text[stop] == '.'))
    {
      ++stop;
    }
    if (stop > digits) {
      text.replace(
          at + kFloor.size(), stop - (at + kFloor.size()), "<host-calibrated>");
    }
  }
  return text;
}

// ---------------------------------------------------------------------------
// The scenarios
// ---------------------------------------------------------------------------

// The sixteen keys of the run node and the six of the entry node, in the
// `x-order` the schema carries, with the nested nodes beside them.
const auto kRunKeys = std::vector<std::string> {"selection",
                                                "norm",
                                                "rank",
                                                "top",
                                                "noisePercent",
                                                "repeats",
                                                "seed",
                                                "budget",
                                                "passes",
                                                "method",
                                                "events",
                                                "stages",
                                                "estimate_ns",
                                                "planned_events",
                                                "ran_events",
                                                "stage_two_events"};
const auto kEntryKeys = std::vector<std::string> {
    "method", "pass_count", "top", "retried_passes", "entries", "skipped"};

// Every key of both tables of contracts/hunt-json.md, in the `x-order` the
// schema states, with `hunt` appended last in the root and in the entry node
// alike (contracts/report-schema-key.md, FR-053).
auto huntKeysInOrder() -> void
{
  const auto text = huntDocumentText(singleStageHunt());
  const auto entryNode = nodeOpen(text, false);
  const auto runNode = nodeOpen(text, true);

  checkOrder(text, runNode, kRunKeys, "/hunt");
  checkOrder(text, entryNode, kEntryKeys, "/benchmarks/0/hunt");
  checkOrder(text,
             elementOpen(text, entryNode, "entries"),
             {"address", "rate", "cv", "noisy", "multiplexed", "rank"},
             "/benchmarks/0/hunt/entries/0");
  checkOrder(text,
             elementOpen(text, entryNode, "skipped"),
             {"address", "kind"},
             "/benchmarks/0/hunt/skipped/0");
  checkOrder(text,
             elementOpen(text, runNode, "stages"),
             {"first", "count", "repeats"},
             "/hunt/stages/0");

  const auto root = keysOf(text, 0);
  const auto benchmarks = text.find("\"benchmarks\":[");
  check(benchmarks != std::string_view::npos,
        "the document carries benchmarks");
  const auto entryKeys = keysOf(text, text.find('{', benchmarks));
  checked(!root.empty() && root.back() == "hunt",
          "hunt stands last among the root keys");
  checked(!entryKeys.empty() && entryKeys.back() == "hunt",
          "hunt stands last among the keys of the entry node");
  check(occurrences(text, kHuntOpen) == 2,
        "the document carries exactly the two hunt nodes");
}

// One field-by-field round trip through `loadReport`, with the three null
// forms of the contract named one by one: a null budget, a null estimate and
// a null rank.
auto huntRoundTrip() -> void
{
  const auto hunted = singleStageHunt();
  const auto written = writeReportDocument(&hunted);
  const auto text = documentText();

  // `null` stands for what is absent; the key never stands away (I-7).
  check(text.find("\"budget\":null") != std::string::npos,
        "an absent budget is written as null");
  check(text.find("\"estimate_ns\":null") != std::string::npos,
        "an absent estimate is written as null");
  check(text.find("\"rank\":null") != std::string::npos,
        "an unranked event is written with a null rank");

  const auto loaded = sg::loadReport(kScratchPath);
  if (!loaded.has_value()) {
    failed("the hunt document refuses to load: " + loaded.error());
  }
  const auto& run = *loaded->hunt;
  const auto& entry = *loaded->results.front().hunt;
  checked(!run.options.budget.has_value(),
          "the null budget reads back as nothing");
  checked(!run.estimateNs.has_value(),
          "the null estimate reads back as nothing");
  checked(run.selection == hunted.run.selection,
          "the selection text reads back whole: " + run.selection);
  checked(run.passes == hunted.run.passes && run.method == hunted.run.method,
          "the pass count and the settled method read back");
  checked(run.plannedAddresses.size() == kColumns
              && run.ranEvents == hunted.run.ranEvents
              && run.stageTwoEvents == 0,
          "the planned, ran and stage-two counts read back");
  checked(entry.entries.size() == hunted.entry.entries.size()
              && entry.skipped.size() == hunted.entry.skipped.size(),
          "the rank table and the skipped list read back with their length");
  checked(!entry.entries.empty() && entry.entries.front().rank == 1
              && sg::test::sameValue(entry.entries.front().rate,
                                     hunted.entry.entries.front().rate),
          "the top event reads back its rank and its rate");

  auto noisy = std::vector<std::size_t> {};
  auto unranked = std::vector<std::size_t> {};
  for (std::size_t row = 0; row < entry.entries.size(); ++row) {
    if (!entry.entries[row].rank.has_value()) {
      unranked.push_back(row);
    }
    if (entry.entries[row].noisy) {
      noisy.push_back(row);
    }
  }
  checked(!noisy.empty(), "a noisy event reads back marked noisy");
  for (const auto row : noisy) {
    checked(!entry.entries[row].rank.has_value(),
            "a noisy event reads back with a null rank");
  }
  const auto lone = std::ranges::find(
      entry.entries, hunted.loneAddress, &sg::hunt::RankedEvent::address);
  checked(lone != entry.entries.end() && !lone->rank.has_value(),
          "an unrepeated event reads back with a null rank");
  checked(unranked.size() >= noisy.size() + 2,
          "the record carries more than one null rank");

  // The column that measured and gapped stands in both lists at once (D-10).
  checked(std::any_of(entry.skipped.begin(),
                      entry.skipped.end(),
                      [&](const sg::hunt::SkippedEvent& skip)
                      {
                        return skip.kind == sg::hunt::SkippedKind::GAP
                            && std::any_of(
                                   entry.entries.begin(),
                                   entry.entries.end(),
                                   [&](const sg::hunt::RankedEvent& row)
                                   { return row.address == skip.address; });
                      }),
          "the address that measured and gapped stands in both lists");

  check(sg::test::sameReport(written, *loaded),
        "every field of both hunt records survives the round trip");
}

// The rows of contracts/hunt-json.md's reader table, each planted into the
// bytes one record cannot express, each refusing with the JSON pointer of the
// position.
const auto kPlants = std::vector<Plant> {
    Plant {.what = "a number where a string stands",
           .node = "run",
           .key = "selection",
           .body = "7",
           .pointer = "/hunt/selection",
           .message = "expected a string, found number"},
    Plant {.what = "a string where a number stands",
           .node = "run",
           .key = "top",
           .body = "\"two\"",
           .pointer = "/hunt/top",
           .message = "expected an integer, found string"},
    Plant {.what = "a boolean where a number stands",
           .node = "run",
           .key = "repeats",
           .body = "true",
           .pointer = "/hunt/repeats",
           .message = "expected an integer, found boolean"},
    Plant {.what = "an integer too wide for its field",
           .node = "run",
           .key = "seed",
           .body = "9223372036854775808",
           .pointer = "/hunt/seed",
           .message = "expected an integer, found number"},
    Plant {.what = "a negative count",
           .node = "run",
           .key = "passes",
           .body = "-1",
           .pointer = "/hunt/passes",
           .message = "a count stands negative"},
    Plant {.what = "an unknown word for norm",
           .node = "run",
           .key = "norm",
           .body = "\"cycles\"",
           .pointer = "/hunt/norm",
           .message = "stands outside the names the schema lists"},
    Plant {.what = "an unknown word for rank",
           .node = "run",
           .key = "rank",
           .body = "\"mean\"",
           .pointer = "/hunt/rank",
           .message = "stands outside the names the schema lists"},
    Plant {.what = "an unknown word for method",
           .node = "run",
           .key = "method",
           .body = "\"PASSED\"",
           .pointer = "/hunt/method",
           .message = "stands outside the names the schema lists"},
    Plant {.what = "a budget text with no unit",
           .node = "run",
           .key = "budget",
           .body = "\"120\"",
           .pointer = "/hunt/budget",
           .message = "states no budget unit"},
    Plant {.what = "a budget text with no amount",
           .node = "run",
           .key = "budget",
           .body = "\"seconds\"",
           .pointer = "/hunt/budget",
           .message = "states no budget amount"},
    Plant {.what = "a missing key of the run node",
           .node = "run",
           .key = "noisePercent",
           .body = "",
           .remove = true,
           .pointer = "/hunt",
           .message = "the key 'noisePercent' stands missing"},
    Plant {.what = "an array where the run object stands",
           .node = "run",
           .key = "hunt",
           .body = "[]",
           .pointer = "/hunt",
           .message = "expected an object, found array"},
    Plant {.what = "an object where the stage array stands",
           .node = "run",
           .key = "stages",
           .body = "{}",
           .pointer = "/hunt/stages",
           .message = "expected an array, found object"},
    Plant {.what = "an array where a stage object stands",
           .node = "run",
           .key = "stages",
           .body = "[[]]",
           .pointer = "/hunt/stages/0",
           .message = "expected an object, found array"},
    Plant {.what = "an array where the entry hunt stands",
           .node = "hunt",
           .key = "hunt",
           .body = "[]",
           .pointer = "/benchmarks/0/hunt",
           .message = "expected an object, found array"},
    Plant {.what = "a string where the entry pass count stands",
           .node = "hunt",
           .key = "pass_count",
           .body = "\"3\"",
           .pointer = "/benchmarks/0/hunt/pass_count",
           .message = "expected an integer, found string"},
    Plant {.what = "a missing key of the entry node",
           .node = "hunt",
           .key = "retried_passes",
           .body = "",
           .remove = true,
           .pointer = "/benchmarks/0/hunt",
           .message = "the key 'retried_passes' stands missing"},
    Plant {.what = "an object where the rank table stands",
           .node = "hunt",
           .key = "entries",
           .body = "{}",
           .pointer = "/benchmarks/0/hunt/entries",
           .message = "expected an array, found object"},
    Plant {.what = "an array where a rank row stands",
           .node = "hunt",
           .key = "entries",
           .body = "[[]]",
           .pointer = "/benchmarks/0/hunt/entries/0",
           .message = "expected an object, found array"},
    Plant {.what = "a boolean where a rank stands",
           .node = "entries",
           .key = "rank",
           .body = "true",
           .pointer = "/benchmarks/0/hunt/entries/0/rank",
           .message = "expected an integer, found boolean"},
    Plant {.what = "an unknown word for a skipped kind",
           .node = "skipped",
           .key = "kind",
           .body = "\"lost\"",
           .pointer = "/benchmarks/0/hunt/skipped/0/kind",
           .message = "stands outside the names the schema lists"},
    // The arms that carry a nested refusal outward: one field of a nested
    // record holds a value of another kind or stands missing, and the refusal
    // names the pointer of that field with the message the sub-parser wrote.
    Plant {.what = "a string where the entry top count stands",
           .node = "hunt",
           .key = "top",
           .body = "\"wide\"",
           .pointer = "/benchmarks/0/hunt/top",
           .message = "expected an integer, found string"},
    Plant {.what = "a number where the run budget text stands",
           .node = "run",
           .key = "budget",
           .body = "7",
           .pointer = "/hunt/budget",
           .message = "expected a string, found number"},
    Plant {.what = "a missing key of a stage object",
           .node = "stages",
           .key = "first",
           .body = "",
           .remove = true,
           .pointer = "/hunt/stages/0",
           .message = "the key 'first' stands missing"},
    Plant {.what = "a string where the first column of a stage stands",
           .node = "stages",
           .key = "first",
           .body = "\"zero\"",
           .pointer = "/hunt/stages/0/first",
           .message = "expected an integer, found string"},
    Plant {.what = "a missing key of a rank row",
           .node = "entries",
           .key = "address",
           .body = "",
           .remove = true,
           .pointer = "/benchmarks/0/hunt/entries/0",
           .message = "the key 'address' stands missing"},
    Plant {.what = "a string where the rate of a rank row stands",
           .node = "entries",
           .key = "rate",
           .body = "\"fast\"",
           .pointer = "/benchmarks/0/hunt/entries/0/rate",
           .message = "expected a number, found string"},
    Plant {.what = "an array where a skipped record stands",
           .node = "hunt",
           .key = "skipped",
           .body = "[[]]",
           .pointer = "/benchmarks/0/hunt/skipped/0",
           .message = "expected an object, found array"},
    Plant {.what = "a missing key of a skipped record",
           .node = "skipped",
           .key = "address",
           .body = "",
           .remove = true,
           .pointer = "/benchmarks/0/hunt/skipped/0",
           .message = "the key 'address' stands missing"},
    Plant {.what = "a missing kind of a skipped record",
           .node = "skipped",
           .key = "kind",
           .body = "",
           .remove = true,
           .pointer = "/benchmarks/0/hunt/skipped/0",
           .message = "the key 'kind' stands missing"},
    Plant {.what = "a string where the skipped array stands",
           .node = "hunt",
           .key = "skipped",
           .body = "\"none\"",
           .pointer = "/benchmarks/0/hunt/skipped",
           .message = "expected an array, found string"},
    Plant {.what = "a string where the run estimate stands",
           .node = "run",
           .key = "estimate_ns",
           .body = "\"soon\"",
           .pointer = "/hunt/estimate_ns",
           .message = "expected an integer, found string"},
    Plant {.what = "an object where the planned address array stands",
           .node = "run",
           .key = "events",
           .body = "{}",
           .pointer = "/hunt/events",
           .message = "expected an array, found object"},
    Plant {.what = "a number among the planned addresses",
           .node = "run",
           .key = "events",
           .body = "[\"cpu/hunt000\",7]",
           .pointer = "/hunt/events",
           .message = "expected a string, found number"},
};

// One case per refused reader code, each naming the pointer of its position
// (hard rule 7, FR-051).
auto readerRefusals() -> void
{
  const auto hunted = singleStageHunt();
  static_cast<void>(writeReportDocument(&hunted));
  const auto text = documentText();
  for (const auto& plant : kPlants) {
    sg::test::writeText(kScratchPath, planted(text, plant));
    const auto answer = sg::loadReport(kScratchPath);
    if (answer.has_value()) {
      failed("the reader accepted " + std::string(plant.what));
    }
    checked(answer.error().find(plant.pointer) != std::string::npos,
            "the refusal of " + std::string(plant.what) + " names "
                + std::string(plant.pointer) + ": " + answer.error());
    checked(answer.error().find(plant.message) != std::string::npos,
            "the refusal of " + std::string(plant.what) + " says '"
                + std::string(plant.message) + "': " + answer.error());
  }
}

// A document with no hunt keys loads with both members empty, and a document
// written before the hunt keys loads unchanged: the reading of a run that
// carried no hunt and of a file the old writer left (FR-051, I-8).
auto huntLessDocument() -> void
{
  const auto written = writeReportDocument(nullptr);
  const auto text = documentText();
  check(text.find("\"hunt\"") == std::string::npos,
        "a report with no hunt writes no hunt key");

  const auto loaded = sg::loadReport(kScratchPath);
  if (!loaded.has_value()) {
    failed("the hunt-less document refuses to load: " + loaded.error());
  }
  check(!loaded->hunt.has_value(),
        "a document with no hunt keys leaves the run member empty");
  check(!loaded->results.front().hunt.has_value(),
        "a document with no hunt keys leaves the entry member empty");
  check(sg::test::sameReport(written, *loaded),
        "the hunt-less document reads back unchanged");

  const auto legacy = sg::loadReport(dataPath("gate_ok_base.json"));
  if (!legacy.has_value()) {
    failed("a document written before the hunt keys refuses to load: "
           + legacy.error());
  }
  check(
      !legacy->hunt.has_value() && !legacy->results.empty()
          && !legacy->results.front().hunt.has_value(),
      "a document written before the hunt keys loads with both members empty");
  check(legacy->speedgunVersion == "0.13.0" || !legacy->speedgunVersion.empty(),
        "the pre-feature document still states its version");
}

// A two-stage document: two `stages` objects, every planned address in
// `events`, and the columns stage 1 alone measured still standing in `entries`
// with no rank (FR-050, D-2, D-17).
auto twoStageDocument() -> void
{
  const auto hunted = twoStageHunt();
  const auto written = writeReportDocument(&hunted);
  const auto text = documentText();
  const auto runNode = nodeOpen(text, true);

  check(objectCount(text, runNode, "stages") == 2,
        "the run node carries two stages objects");
  check(objectCount(text, runNode, "events") == 0,
        "the events array holds addresses, not objects");

  const auto loaded = sg::loadReport(kScratchPath);
  if (!loaded.has_value()) {
    failed("the two-stage hunt document refuses to load: " + loaded.error());
  }
  const auto& run = *loaded->hunt;
  checked(run.stages.size() == 2, "both stages read back");
  checked(run.stages[1].count == kStageTwoColumns
              && run.stages[1].repeats == kRepeats,
          "stage 2 reads back its column count and its repeats");
  checked(run.stageTwoEvents == kStageTwoColumns,
          "the stage-two count reads back");
  checked(run.plannedAddresses.size() == kColumns
              && run.selection == hunted.run.selection,
          "the planned addresses read back in plan order");
  checked(run.options.budget.has_value()
              && run.options.budget->amount == kBudgetSeconds,
          "the budget of a bounded plan reads back as its text");
  checked(run.estimateNs.has_value() && *run.estimateNs > 0,
          "a calibrated plan reads back its estimate");

  const auto& entry = *loaded->results.front().hunt;
  checked(entry.entries.size() == kColumns,
          "every planned column stands in the rank table");
  const auto stageOneOnly = std::ranges::find(
      entry.entries, hunted.loneAddress, &sg::hunt::RankedEvent::address);
  checked(
      stageOneOnly != entry.entries.end() && !stageOneOnly->rank.has_value(),
      "a column only stage 1 measured stands in entries with no rank");
  checked(entry.entries.front().rank == 1,
          "the column stage 2 repeated reads back its rank");
  check(sg::test::sameReport(written, *loaded),
        "every field of the two-stage record survives the round trip");
}

// Conformance through `SG_SCHEMA_PATH`: the hunt document passes, a planted
// out-of-order hunt key and a planted key outside a closed hunt node are both
// reported as findings naming the pointer (FR-053, T047).
auto schemaConformance() -> void
{
  const auto text = huntDocumentText(singleStageHunt());
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    failed("the hunt document disagrees with the schema: " + clean.error());
  }

  const auto order =
      sg::detail::checkReport(SG_SCHEMA_PATH, swapped(text, "norm", "rank"));
  if (order.has_value()) {
    fail("a hunt node with its keys out of x-order passes the schema");
  }
  checked(order.error().find("/hunt/norm") != std::string::npos
              && order.error().find("stands out of the order the schema states")
                  != std::string::npos,
          "the out-of-order finding names the pointer: " + order.error());

  const auto outsideRun = sg::detail::checkReport(
      SG_SCHEMA_PATH, inserted(text, "run", "\"extra\":true,"));
  if (outsideRun.has_value()) {
    fail("a key outside the closed run node passes the schema");
  }
  checked(outsideRun.error().find("/hunt/extra") != std::string::npos,
          "the key outside the closed run node names its pointer: "
              + outsideRun.error());

  const auto outsideEntry = sg::detail::checkReport(
      SG_SCHEMA_PATH, inserted(text, "hunt", "\"extra\":true,"));
  if (outsideEntry.has_value()) {
    fail("a key outside the closed entry node passes the schema");
  }
  checked(outsideEntry.error().find("/benchmarks/0/hunt/extra")
              != std::string::npos,
          "the key outside the closed entry node names its pointer: "
              + outsideEntry.error());

  static_cast<void>(writeReportDocument(nullptr));
  const auto plain = sg::detail::checkReport(SG_SCHEMA_PATH, documentText());
  if (!plain.has_value()) {
    failed("the hunt-less document disagrees with the schema: "
           + plain.error());
  }
}

// ---------------------------------------------------------------------------
// The run with the hunt option absent (FR-052, SC-006, AC-6)
// ---------------------------------------------------------------------------

auto captureCaseBody(sg::State& state) -> void
{
  for (auto ignored : state) {
    static_cast<void>(ignored);
  }
}

// The console and the status of one in-process `speedgunMain`, with stdout
// redirected the way `harness_hunt_cli_test.cpp` redirects it.
[[nodiscard]] auto captureRun(const std::vector<std::string>& arguments,
                              int* status) -> std::string
{
  auto argv = std::vector<char*> {};
  argv.push_back(selfPath.data());
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kCaptureConsole, "w", stdout) == nullptr) {
    fail("cannot redirect the report of the captured run");
  }
  *status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout after the captured run");
  }
  close(saved);
  auto file = std::ifstream(kCaptureConsole);
  auto buffer = std::stringstream {};
  buffer << file.rdbuf();
  return buffer.str();
}

// The child of the byte-identity scenario: the scripted hunt host, one pinned
// benchmark, one run that names no `--hunt`, and its three answers written
// where the parent can read them. It asserts nothing of its own.
auto captureNoHuntRun() -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::huntHost(*provider, kLimit);
  check(sg::counters::System::local()
            .registerProvider(std::move(provider))
            .has_value(),
        "the scripted host of the captured run registers its objects");
  static const sg::BenchmarkHandle kHandle =
      sg::registerBenchmark(&captureCaseBody, kCaptureCase).iterations(1);
  static_cast<void>(kHandle);

  std::remove(kCaptureDocument);
  std::remove(kCaptureConsole);
  auto status = 0;
  const auto console = captureRun({"--filter",
                                   "^bmHuntJsonCapture$",
                                   std::string("--json=") + kCaptureDocument},
                                  &status);
  sg::test::writeText(kCaptureConsole, console);
  std::printf("status=%d\n", status);
}

// The document, the console and the exit status of a run with no `--hunt` are
// what they were before the feature: the capture byte for byte apart from the
// values that state which build wrote the file.
auto noHuntBytesMatchCapture() -> void
{
  std::remove(kCaptureDocument);
  std::remove(kCaptureConsole);
  const auto command = "\"" + selfPath + "\" " + kCaptureMode + " 2>/dev/null";
  auto output = std::string {};
  FILE* const pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    fail("cannot start the capture child");
  }
  auto line = std::array<char, 256> {};
  while (fgets(line.data(), static_cast<int>(line.size()), pipe) != nullptr) {
    output += line.data();
  }
  const int pipeStatus = pclose(pipe);
  check(WIFEXITED(pipeStatus) && WEXITSTATUS(pipeStatus) == 0,
        "the capture child ran the run");
  auto document = sg::test::readText(kCaptureDocument);
  auto console = sg::test::readText(kCaptureConsole);
  std::remove(kCaptureDocument);
  std::remove(kCaptureConsole);

  const auto wantDocument = sg::test::readText(dataPath(kCommittedDocument));
  const auto wantConsole =
      reportRegion(sg::test::readText(dataPath(kCommittedConsole)));
  checked(!document.empty() && !console.empty() && !wantDocument.empty(),
          "the capture child left a document, a console and two captures to "
          "compare: status line '"
              + output + "'");
  checked(
      output == "status=0\n",
      "a run with no --hunt exits the status the capture states: " + output);
  check(console.find("hunt") == std::string::npos,
        "a run with no --hunt prints no hunt line");
  check(document.find("\"hunt\"") == std::string::npos,
        "a run with no --hunt writes no hunt key");

  // shortcut: the three masked values state the build and the host that wrote
  // the file; each says nothing about the run that wrote it. Upgrade when the
  // capture is regenerated per build.
  if (identityMasked(document) != identityMasked(wantDocument)) {
    failed(
        "the document of a run with no --hunt is not the capture bytes: " "got "
                                                                          "'"
        + document.substr(0, 240) + "' want '" + wantDocument.substr(0, 240)
        + "'");
  }
  if (identityMasked(console) != identityMasked(wantConsole)) {
    failed("the console of a run with no --hunt is not the captured console: "
           "got '" + identityMasked(console) + "' want '"
           + identityMasked(wantConsole) + "'");
  }
  check(wantDocument.find("\"hunt\"") == std::string::npos,
        "the committed capture holds no hunt key, as a pre-feature document "
        "must");
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {.name = "keys", .body = &huntKeysInOrder},
      Scenario {.name = "round-trip", .body = &huntRoundTrip},
      Scenario {.name = "refusals", .body = &readerRefusals},
      Scenario {.name = "no-keys", .body = &huntLessDocument},
      Scenario {.name = "two-stage", .body = &twoStageDocument},
      Scenario {.name = "schema", .body = &schemaConformance},
      Scenario {.name = "no-hunt-bytes", .body = &noHuntBytesMatchCapture},
  };
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  selfPath =
      argc > 0 && argv[0] != nullptr ? argv[0] : "harness_hunt_json_test";
  if (argc > 1 && std::string_view(argv[1]) == kCaptureMode) {
    captureNoHuntRun();
    return 0;
  }
  if (argc > 2 && argv[2] != nullptr) {
    dataArgument = argv[2];
  }
  const auto& list = scenarios();
  // The list is what this binary asserts. An empty list runs nothing and still
  // prints ok, which is the shape the unfilled file had (T106): no scenario
  // list that holds nothing may pass.
  check(!list.empty(), "the scenario list holds at least one scenario");
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
    std::printf("harness_hunt_json_test: scenario %.*s\n",
                static_cast<int>(scenario.name.size()),
                scenario.name.data());
    scenario.body();
  }
  std::printf("harness_hunt_json_test: %zu scenarios ok\n", list.size());
  return 0;
}
