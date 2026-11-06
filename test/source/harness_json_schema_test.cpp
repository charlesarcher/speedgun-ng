// ============================================================================
// T017, T025: the written document validates, and the checker names the
// pointer of a disagreement (specs/023-json-reports; FR-008, FR-010, D-05).
//
// The oracle is `checkReport`, which reads `docs/schema/report-v1.json`
// with no validator library. A document the writer produced passes. Five
// planted documents, each with one disagreement of the list
// `contracts/schema-file.md` states, each fail with the JSON pointer of
// that position in the message. Frameworkless check()/fail() convention.
// ============================================================================

#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "detail/json_fixture.hpp"

using sg::test::check;

namespace

{
#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH"
#endif

// The two keys of the `speedgun` object, swapped: the document holds the
// same keys in an order the schema does not state.
auto swappedSpeedgun(const std::string& text) -> std::string
{
  const std::string head = "\"speedgun\":{";
  const auto start = text.find(head);
  const auto bodyStart = start + head.size();
  const auto bodyEnd = text.find('}', bodyStart);
  const auto comma = text.find(',', bodyStart);
  std::string mutated = text;
  const std::string first = text.substr(bodyStart, comma - bodyStart);
  const std::string second = text.substr(comma + 1, bodyEnd - comma - 1);
  mutated.replace(bodyStart, bodyEnd - bodyStart, second + "," + first);
  return mutated;
}

// The two carriers of the entry order of specs/042-topdown (FR-032, FR-036):
// a top-down object of the shape the schema states, and a hunt object of the
// shape the schema states, each hung on the entry after `spreadWarning`, the
// key that closed the entry before either of them existed.
constexpr const char* const kHuntObject =
    R"JSON({"method":"AUTO","pass_count":1,"top":20,"retried_passes":0,"entries":[],"skipped":[]})JSON";

constexpr const char* const kTopdownObject =
    R"JSON({"supported":true,"reason":"","reference":"v4.3","directory":"arch/x86/skylakex/","files":[{"name":"core_metrics.json","manifest_hash":"0a1b2c3d"}],"asked_level":4,"level":3,"nodes":[{"name":"backend_bound","vendor_name":"TopL1.BackendBound","parent":"","level":1,"value":52.5,"availability":"COUNTABLE","threshold":"> 20 %"}],"refused":[{"name":"topdown.intel.frontend_l1_bound","kind":"UNRESOLVED","reason":"the row names a metric no other row"}]})JSON";

// The same top-down object with one key the closed object does not list.
constexpr const char* const kTopdownWithExtra =
    R"JSON({"supported":true,"extra":true,"reason":"","reference":"v4.3","directory":"arch/x86/skylakex/","files":[{"name":"core_metrics.json","manifest_hash":"0a1b2c3d"}],"asked_level":4,"level":3,"nodes":[{"name":"backend_bound","vendor_name":"TopL1.BackendBound","parent":"","level":1,"value":52.5,"availability":"COUNTABLE","threshold":"> 20 %"}],"refused":[{"name":"topdown.intel.frontend_l1_bound","kind":"UNRESOLVED","reason":"the row names a metric no other row"}]})JSON";

// One entry of the document, with `keys` hung on it after the last key of the
// entry as the writer stands it.
auto entryWithKeys(const std::string& text,
                   const std::string& keys) -> std::string
{
  const std::string last = "\"spreadWarning\":";
  const auto at = text.find(last);
  check(at != std::string::npos, "the written entry carries a spread warning");
  const auto value = at + last.size();
  const auto stop = text[value] == 't' ? value + 4 : value + 5;
  std::string mutated = text;
  mutated.insert(stop, "," + keys);
  return mutated;
}

auto plantedDisagreementsAreNamedByPointer() -> void
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  options.showPercentiles = true;
  options.rawContent = sg::RawContent::CHUNKS;
  const auto run = sg::test::scriptedRun(options);
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  check(sg::detail::writeReport("harness_json_schema.json", data),
        "the writer wrote the document (FR-001)");
  const std::string text = sg::test::readText("harness_json_schema.json");
  check(!text.empty(), "the written file holds a document");

  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  check(clean.has_value(),
        "a document the writer produced passes the schema (FR-008)");
  if (!clean.has_value()) {
    std::printf("  %s\n", clean.error().c_str());
  }

  struct Plant
  {
    std::string text;
    const char* what;
    const char* pointer;
  };

  std::string missing = text;
  const auto missingAt = missing.find(",\"buildType\"");
  const auto valueStart = missing.find(':', missingAt) + 1;
  missing.erase(missingAt, missing.find('"', valueStart + 1) + 1 - missingAt);
  std::string extra = text;
  extra.replace(extra.find("\"speedgun\":{") + 12, 0, "\"extra\":true,");
  const std::string outOfOrder = swappedSpeedgun(text);
  std::string wrongKind = text;
  const auto stamp = wrongKind.find("\"schemaVersion\":") + 16;
  wrongKind.replace(stamp, wrongKind.find('"', stamp + 1) + 1 - stamp, "1");
  std::string badEnum = text;
  badEnum.replace(
      badEnum.find("\"outcome\":\"MEASURED\"") + 10, 10, "\"measured\"");
  // FR-017: a hand-edited document that carries one key twice stands
  // refused: the parser's lookup keeps the last, so the duplicate rides
  // the pairs the tape carries and the pointer names the key.
  std::string twice = text;
  twice.replace(twice.find('{') + 1, 0, "\"schemaVersion\":\"1.5\",");

  for (const Plant& plant : {
           Plant {std::move(missing),
                  "a key the schema requires stands " "missing",
                  "/speedgun"},
           Plant {std::move(extra),
                  "a key the closed object does not list",
                  "/speedgun/extra"},
           Plant {outOfOrder,
                  "a key stands out of the stated order",
                  "/speedgun/version"},
           Plant {std::move(wrongKind),
                  "a value of the wrong kind",
                  "/schemaVersion"},
           Plant {std::move(badEnum),
                  "an enum spelling outside its list",
                  "/benchmarks/0/outcome"},
           Plant {std::move(twice),
                  "a key that stands twice in one object",
                  "/schemaVersion"},
           // specs/042-topdown FR-032, FR-036: the entry order is normative,
           // so a top-down object that stands before the hunt object reads
           // out of that order, and a key the closed top-down object does not
           // list reads unlisted.
           Plant {entryWithKeys(text,
                                std::string("\"topdown\":") + kTopdownObject
                                    + ",\"hunt\":" + kHuntObject),
                  "a top-down key stands before the hunt key",
                  "/benchmarks/0/hunt"},
           Plant {entryWithKeys(text,
                                std::string("\"hunt\":") + kHuntObject
                                    + ",\"topdown\":" + kTopdownWithExtra),
                  "the closed top-down object holds an unlisted key",
                  "/benchmarks/0/topdown/extra"},
       })
  {
    const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, plant.text);
    check(!answer.has_value(), plant.what);
    if (!answer.has_value()) {
      check(answer.error().find(plant.pointer) != std::string::npos,
            "the message names the JSON pointer of the disagreement");
    }
  }

  // FR-017, D-14: a hand-edited document whose root is no object stands
  // refused beside the duplicate scan: the scan reads the pairs of an
  // object and answers nothing for an array root, and the kind refusal
  // names the disagreement the schema states.
  const auto nonObject =
      sg::detail::checkReport(SG_SCHEMA_PATH, std::string("[]"));
  check(!nonObject.has_value(),
        "a document whose root is an array stands refused (FR-017)");
  if (!nonObject.has_value()) {
    check(nonObject.error().find("kind") != std::string::npos
              || nonObject.error().find("object") != std::string::npos,
          "the refusal names the kind the root carries (FR-017)");
  }

  // FR-017: the schema document stands beside the report document: a
  // copy of the schema that carries one key twice stands refused, and
  // the schema as it stands names every top-level key once, so the
  // x-order list stands read once.
  const std::string schemaText = sg::test::readText(SG_SCHEMA_PATH);
  const auto keyAt = schemaText.find("\"x-order\"");
  check(keyAt != std::string::npos, "the schema states the key order");
  const auto keyEnd = schemaText.find(']', keyAt) + 1;
  std::string twiceOrder = schemaText;
  twiceOrder.insert(keyAt, schemaText.substr(keyAt, keyEnd - keyAt) + ",");
  sg::test::writeText("harness_json_schema_dup.json", twiceOrder);
  const auto doubled =
      sg::detail::checkReport("harness_json_schema_dup.json", text);
  check(!doubled.has_value(),
        "a schema that carries one key twice stands refused (FR-017)");
  if (!doubled.has_value()) {
    check(doubled.error().find("/x-order") != std::string::npos,
          "the schema refusal names the duplicated key (FR-017)");
  }
}

// The schema's per-entry hunt entry node (specs/040; FR-032, B-9): the
// node stands closed, so its `x-order` names the three rank keys after
// `rank` and the area key after `delta`, and the document the writer
// produces pairs the schema at the minor the writer stamps, `1.15`, the
// minor since specs/041-counter-probe added the run-level `counterLimits`
// array.
auto entryHuntKeysPairAtStampedMinor(const std::string& documentText) -> void
{
  const std::string schemaText = sg::test::readText(SG_SCHEMA_PATH);

  // The `entries` array stands once in the schema file, under the
  // per-entry `hunt` object of a benchmark; the entry node is its item
  // schema.
  const auto entriesAt = schemaText.find("\"entries\": {");
  check(entriesAt != std::string::npos,
        "the schema carries the per-entry hunt entries array (FR-032)");
  const auto itemsAt = schemaText.find("\"items\": {", entriesAt);
  const auto orderAt = schemaText.find("\"x-order\"", itemsAt);
  const auto open = schemaText.find('[', orderAt);
  const auto close = schemaText.find(']', open);
  const std::string order = schemaText.substr(open, close - open);

  const auto rankAt = order.find("\"rank\"");
  const auto correlationAt = order.find("\"correlation\"");
  const auto slopeAt = order.find("\"slope\"");
  const auto deltaAt = order.find("\"delta\"");
  const auto areaAt = order.find("\"topdownArea\"");
  check(rankAt != std::string::npos
            && correlationAt != std::string::npos
            && slopeAt != std::string::npos && deltaAt != std::string::npos
            && areaAt != std::string::npos
            && rankAt < correlationAt && correlationAt < slopeAt
            && slopeAt < deltaAt && deltaAt < areaAt,
        "the entry node names the three rank keys after `rank`, and the area "
        "key after `delta`, in its `x-order` (FR-032, B-9)");

  check(documentText.find("\"schemaVersion\":\"1.15\"") != std::string::npos,
        "the document the writer produces stands at the stamped minor 1.15 "
        "(FR-032, specs/039 FR-032, specs/037 FR-005, specs/043 FR-006, "
        "specs/042-topdown FR-036, specs/046-hunter-topdown FR-020, "
        "specs/041-counter-probe FR-051)");
}

// specs/043 T025, US4, FR-006, R-16: the schema node of the series record.
// The item stands closed, its `x-order` names the 21 keys in the written
// order, the method word stands in a list of three, and the period item
// stands closed with its two keys. The document the writer produces pairs
// the schema at the minor the writer stamps, `1.15`, the minor since
// specs/041-counter-probe added the run-level `counterLimits` array.
constexpr std::string_view kSeriesKeys[] = {
    "cpu",
    "workCount",
    "unitTimeNs",
    "targetNs",
    "quantumNs",
    "calibrationSpanNs",
    "sampleCount",
    "minValue",
    "medianValue",
    "meanValue",
    "p90Value",
    "p99Value",
    "p999Value",
    "p9999Value",
    "maxValue",
    "noisePercent",
    "seriesRuntimeNs",
    "droppedSamples",
    "resampleStepNs",
    "spectrumPoints",
    "periods",
};

// The closed series item, every key in the written order, the period list
// carrying one item (FR-006, FR-018).
constexpr const char* const kSeriesItem =
    R"JSON({"cpu":2,"workCount":7,"unitTimeNs":1000,"targetNs":1000,"quantumNs":100000,"calibrationSpanNs":500,"sampleCount":4096,"minValue":900,"medianValue":1000,"meanValue":1000.5,"p90Value":1100,"p99Value":1200,"p999Value":1300,"p9999Value":1400,"maxValue":1500,"noisePercent":0.25,"seriesRuntimeNs":4096000,"droppedSamples":3,"resampleStepNs":1000,"spectrumPoints":2048,"periods":[{"periodNs":1000000,"powerRatio":1.0}]})JSON";

// The written document of a run that carries the run-level jitter record:
// the six rows splice their series keys into the jitter object this writes,
// the shape the writer produces for a probed run (FR-006, FR-025).
auto documentWithJitter() -> std::string
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  const auto run = sg::test::scriptedRun(options);
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  sg::detail::JitterReportRecord record;
  record.thresholdNs = 200;
  record.loopPeriodNs = 20;
  record.cpus = {2};
  sg::detail::JitterReportCpu cpu;
  cpu.cpu = 2;
  cpu.stats.gapCount = 1;
  cpu.stats.gapsPerSecond = 1000.0;
  cpu.stats.minNs = 200;
  cpu.stats.medianNs = 200;
  cpu.stats.meanNs = 200.0;
  cpu.stats.p90Ns = 210;
  cpu.stats.p99Ns = 220;
  cpu.stats.p999Ns = 230;
  cpu.stats.p9999Ns = 240;
  cpu.stats.maxNs = 250;
  cpu.stats.totalNs = 260;
  cpu.stats.totalPercent = 0.5;
  cpu.stats.runtimeNs = 270;
  cpu.stats.thresholdNs = 280;
  cpu.stats.loopPeriodNs = 29;
  cpu.stats.droppedGaps = 30;
  record.stats.push_back(cpu);
  record.disclosure = "the run's disclosure";
  check(sg::detail::writeReport(
            "harness_json_schema_jitter.json", data, {}, record),
        "the writer wrote the document with the jitter record (FR-006)");
  return sg::test::readText("harness_json_schema_jitter.json");
}

// The written document with the method word and one series item spliced
// into the run-level `jitter` object, the place the writer keeps them:
// the plants reach the item's own keys, and the schema reaches the pairs
// the splice writes (FR-006).
auto withSeries(const std::string& text,
                const std::string& method,
                const std::string& item) -> std::string
{
  const auto jitterAt = text.find("\"jitter\":{");
  check(jitterAt != std::string::npos,
        "the written document carries the run-level jitter object (FR-006)");
  const auto cpusAt = text.find("\"cpus\":[", jitterAt);
  check(cpusAt != std::string::npos,
        "the run-level jitter object carries its cpu list (FR-006)");
  std::string spliced = text;
  spliced.insert(cpusAt,
                 "\"method\":" + method + ",\"series\":[" + item + "],");
  return spliced;
}

// The `x-order` list of the series item, read out of the schema file the
// way the hunt row reads the list of its own node (FR-006).
auto seriesOrderOfSchema() -> std::string
{
  const std::string schemaText = sg::test::readText(SG_SCHEMA_PATH);
  const auto seriesAt = schemaText.find("\"series\": {");
  check(seriesAt != std::string::npos,
        "the schema carries the run-level series array (FR-006)");
  if (seriesAt == std::string::npos) {
    return {};
  }
  const auto itemsAt = schemaText.find("\"items\": {", seriesAt);
  check(itemsAt != std::string::npos,
        "the series array states its item schema (FR-006)");
  const auto orderAt = schemaText.find("\"x-order\"", itemsAt);
  const auto open = schemaText.find('[', orderAt);
  if (itemsAt == std::string::npos || orderAt == std::string::npos
      || open == std::string::npos)
  {
    return {};
  }
  const auto close = schemaText.find(']', open);
  return schemaText.substr(open, close - open);
}

// FR-006, R-16: the item names the 21 keys in the written order, and the
// document the writer produces pairs the schema at the stamped minor.
auto theSeriesKeysPairAtTheStampedMinor() -> void
{
  const std::string documentText = documentWithJitter();
  const std::string order = seriesOrderOfSchema();
  check(!order.empty(), "the series item states its key order (FR-006)");
  std::size_t previous = 0;
  bool paired = true;
  for (const std::string_view key : kSeriesKeys) {
    const auto at = order.find(std::string("\"") + std::string(key) + "\"");
    paired = paired && at != std::string::npos && at > previous;
    previous = at;
  }
  check(paired,
        "theSeriesKeysPairAtTheStampedMinor: the item names the 21 keys in "
        "the written order (FR-006, FR-011)");
  check(documentText.find("\"schemaVersion\":\"1.15\"") != std::string::npos,
        "theSeriesKeysPairAtTheStampedMinor: the document the writer produces "
        "stands at the stamped minor 1.15 (FR-006, R-16, specs/041-counter-"
        "probe FR-051)");
}

// FR-006: the item stands closed, so a key the schema lists nowhere refuses
// with the pointer of the item that carries it.
auto aClosedSeriesItemRefusesAnAddedKey() -> void
{
  std::string added = withSeries(documentWithJitter(), "\"FWQ\"", kSeriesItem);
  const auto at = added.find("{\"cpu\":2,");
  check(at != std::string::npos, "the splice wrote the series item (FR-006)");
  added.insert(at + 1, "\"extra\":true,");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, added);
  check(!answer.has_value(),
        "aClosedSeriesItemRefusesAnAddedKey: the closed series item refuses a "
        "key the schema lists nowhere (FR-006)");
  if (!answer.has_value()) {
    check(answer.error().find("/jitter/series/0/extra") != std::string::npos,
          "aClosedSeriesItemRefusesAnAddedKey: the refusal names the item "
          "that carries the key (FR-006, FR-010)");
  }
}

// FR-006: the keys of the item ride a stated order, so the same keys in
// another order refuse.
auto aSeriesKeyOutOfOrderStandsRefused() -> void
{
  std::string swapped = std::string(kSeriesItem);
  static constexpr std::string_view kHead = "{\"cpu\":2,\"workCount\":7,";
  static constexpr std::string_view kSwap = "{\"workCount\":7,\"cpu\":2,";
  check(swapped.starts_with(kHead),
        "the item opens with its cpu and its work count (FR-006)");
  swapped.replace(0, kHead.size(), kSwap);
  const auto answer = sg::detail::checkReport(
      SG_SCHEMA_PATH, withSeries(documentWithJitter(), "\"FWQ\"", swapped));
  check(!answer.has_value(),
        "aSeriesKeyOutOfOrderStandsRefused: a key out of the stated order "
        "stands refused (FR-006, FR-010)");
  if (!answer.has_value()) {
    check(answer.error().find("out of the order") != std::string::npos
              && answer.error().find("/jitter/series/0") != std::string::npos,
          "aSeriesKeyOutOfOrderStandsRefused: the refusal names the order and "
          "the item (FR-006, FR-010)");
  }
}

// FR-006: the method word stands in a list of three, so a fourth spelling
// refuses beside the list.
auto aMethodWordOutsideTheThreeStandsRefused() -> void
{
  const auto answer = sg::detail::checkReport(
      SG_SCHEMA_PATH,
      withSeries(documentWithJitter(), "\"AUTO\"", kSeriesItem));
  check(!answer.has_value(),
        "aMethodWordOutsideTheThreeStandsRefused: a method word outside the "
        "three stands refused (FR-006)");
  if (!answer.has_value()) {
    check(answer.error().find("/jitter/method") != std::string::npos
              && answer.error().find("outside the list") != std::string::npos,
          "aMethodWordOutsideTheThreeStandsRefused: the refusal names the "
          "word and the list (FR-006, FR-010)");
  }
}

// FR-018, E-24: a run that keeps no period list writes an empty array, and
// the schema takes it.
auto anEmptyPeriodsArrayPasses() -> void
{
  std::string empty = std::string(kSeriesItem);
  static constexpr std::string_view kOpen = "\"periods\":[";
  const auto at = empty.find(kOpen);
  check(at != std::string::npos, "the item carries its period list (FR-018)");
  empty.erase(at + kOpen.size(), empty.find(']', at) - (at + kOpen.size()));
  const auto answer = sg::detail::checkReport(
      SG_SCHEMA_PATH, withSeries(documentWithJitter(), "\"FTQ\"", empty));
  if (!answer.has_value()) {
    std::printf("  the empty period list refused: %s\n",
                answer.error().c_str());
  }
  check(answer.has_value(),
        "anEmptyPeriodsArrayPasses: an empty period list passes the schema "
        "(FR-018, FR-022)");
}

// FR-018: the period item stands closed with two keys of its own kinds, so
// a figure of another kind refuses with its pointer.
auto aPeriodItemOfTheWrongKindStandsRefused() -> void
{
  std::string broken = std::string(kSeriesItem);
  const auto at = broken.find("\"periodNs\":1000000");
  check(at != std::string::npos, "the period item carries its period (FR-018)");
  static constexpr std::string_view kPeriod = "\"periodNs\":1000000";
  broken.replace(at, kPeriod.size(), "\"periodNs\":\"slow\"");
  const auto answer = sg::detail::checkReport(
      SG_SCHEMA_PATH, withSeries(documentWithJitter(), "\"FWQ\"", broken));
  check(!answer.has_value(),
        "aPeriodItemOfTheWrongKindStandsRefused: a period figure of the wrong "
        "kind stands refused (FR-018)");
  if (!answer.has_value()) {
    check(answer.error().find("/jitter/series/0/periods/0/periodNs")
              != std::string::npos,
          "aPeriodItemOfTheWrongKindStandsRefused: the refusal names the "
          "period figure (FR-018, FR-010)");
  }
}

// The checker reads the schema file as a document of its own: a schema it
// cannot read is refused with the place that broke, and so is a document it
// cannot read (FR-008, SC-008).
constexpr const char* const kBenchItemsSchema =
    "{\"properties\":{\"benchmarks\":{\"type\":\"array\","
    "\"items\":{\"type\":\"string\"}}}}";

auto brokenSchemasAreRefused(const std::string& text) -> void
{
  struct Schema
  {
    const char* body;
    const char* needle;
  };

  for (const Schema& schema : {
           Schema {"{\"type\":[\"object\",5]}", "/type"},
           Schema {"{\"type\":5}", "/type"},
           Schema {"{\"type\":\"string\"}", "the document root"},
           Schema {"{\"type\":[\"string\",\"number\"]}", "string or number"},
           Schema {"{\"enum\":[\"a\"]}", "outside the list the schema states"},
           Schema {"{\"type\":\"string\",\"enum\":[5]}", "/enum"},
           Schema {"{\"type\":\"object\",\"properties\":{\"x\":5}}",
                   "/properties/x"},
           Schema {"{\"type\":\"object\",\"required\":{}}", "/required"},
           Schema {"{\"type\":\"object\",\"x-order\":[5]}", "/x-order"},
           Schema {"{\"type\":\"string\",\"enum\":{}}", "/enum"},
           Schema {"{\"type\":\"object\",\"properties\":[]}", "/properties"},
           Schema {"{\"type\":\"array\",\"items\":[]}", "/items"},
           Schema {kBenchItemsSchema, "/benchmarks/0"},
           Schema {"not json", "holds no JSON"},
       })
  {
    sg::test::writeText("harness_json_schema_mutant.json", schema.body);
    const auto answer =
        sg::detail::checkReport("harness_json_schema_mutant.json", text);
    if (answer.has_value()) {
      std::printf("  the schema '%s' passed\n", schema.body);
      sg::test::fail("a schema the checker cannot read passes (FR-008)");
      continue;
    }
    if (answer.error().find(schema.needle) == std::string::npos) {
      std::printf("  '%s' refused without '%s': %s\n",
                  schema.body,
                  schema.needle,
                  answer.error().c_str());
      sg::test::fail("the refusal names no place in the schema (FR-008)");
    }
  }

  const auto absent =
      sg::detail::checkReport("harness_json_schema_absent.json", text);
  check(!absent.has_value()
            && absent.error().find("cannot be opened") != std::string::npos,
        "a schema file that cannot be opened is refused (FR-008)");

  const auto unreadable = sg::detail::checkReport(SG_SCHEMA_PATH, "{\"a\":[");
  check(!unreadable.has_value()
            && unreadable.error().find("the written document holds no JSON")
                != std::string::npos,
        "a document the parser cannot read is refused (FR-008)");
}

// The shapes of the schema language the shipped file never writes: a
// node with no type at all, an object left open for a key a newer minor
// adds, a listed key the stated order does not name, and an array with
// no item schema. Each passes the document it stands over (FR-008,
// FR-010).
constexpr const char* const kOpenObject =
    R"JSON({"type":"object","additionalProperties":true,)JSON" R"JSON("properties":{"schemaVersion":{"type":"string"}}})JSON";
constexpr const char* const kArrayWithoutItems =
    R"JSON({"type":"object","properties":)JSON" R"JSON({"benchmarks":{"type":"array"}}})JSON";
constexpr const char* const kNonBoolAdditional =
    R"JSON({"type":"object","additionalProperties":{}})JSON";
constexpr const char* const kItemsSchema =
    R"JSON({"type":"object","properties":)JSON" R"JSON({"benchmarks":{"type":"array","items":{"type":"object"}}}})JSON";
constexpr const char* const kItemsOnScalar =
    R"JSON({"type":"object","properties":)JSON" R"JSON({"schemaVersion":{"items":{"type":"string"}}}})JSON";

auto theOpenShapesOfTheSchemaLanguagePass(const std::string& text) -> void
{
  for (const char* body :
       {kOpenObject, kArrayWithoutItems, kNonBoolAdditional, kItemsOnScalar})
  {
    sg::test::writeText("harness_json_schema_open.json", body);
    const auto answer =
        sg::detail::checkReport("harness_json_schema_open.json", text);
    if (!answer.has_value()) {
      std::printf("  the open schema '%s': %s\n", body, answer.error().c_str());
    }
    check(answer.has_value(), "an open schema takes the document (FR-010)");
  }

  // An array with no element still satisfies the item schema it carries:
  // the loop that walks the elements closes without one. The scalar with
  // an item schema stands too: the array read answers not-that-kind and
  // the value passes, as it did when the kind question came first.
  sg::test::writeText("harness_json_schema_items.json", kItemsSchema);
  const auto empty = sg::detail::checkReport("harness_json_schema_items.json",
                                             R"JSON({"benchmarks":[]})JSON");
  if (!empty.has_value()) {
    std::printf("  the item schema over an empty array: %s\n",
                empty.error().c_str());
  }
  check(empty.has_value(),
        "an array with no element satisfies its item schema (FR-008)");
}

// specs/041-counter-probe FR-051: the run-level record of where each needed
// device's counter limit came from pairs at the stamped minor. One device
// carries the figure the search answered, one carries no figure at all, and the
// closed item of the schema takes its three keys in the stated order and
// nothing else. The record stands outside the top-level required list, so the
// document of a run that needed no device passes without it.
constexpr std::string_view kLimitEntry {
    R"JSON({"device":"cpu","limit":6,"source":"probe"})JSON"};

// One entry of the record, planted in place of the entry the writer wrote,
// checked against the shipped schema and required to name the pointer of the
// key at fault.
auto limitPlantStandsRefused(const std::string& text,
                             const std::string& entry,
                             const std::string& pointer,
                             const char* const what) -> void
{
  std::string mutated = text;
  const auto at = mutated.find(kLimitEntry);
  if (at == std::string::npos) {
    sg::test::fail("the written document holds no limit entry to plant");
    return;
  }
  mutated.replace(at, kLimitEntry.size(), entry);
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, mutated);
  const bool named =
      !answer.has_value() && answer.error().find(pointer) != std::string::npos;
  check(named, what);
  if (!named) {
    std::printf(
        "  %s: %s\n",
        pointer.c_str(),
        answer.has_value() ? "the checker took it" : answer.error().c_str());
  }
}

auto theLimitRecordPairsAtTheStampedMinor() -> void
{
  const std::vector<sg::detail::CounterLimitEntry> entries {
      sg::detail::CounterLimitEntry {
          .device = "cpu",
          .limit = std::optional<std::size_t> {6},
          .source = sg::counters::LimitSource::PROBE},
      sg::detail::CounterLimitEntry {
          .device = "uncore_imc_77",
          .limit = {},
          .source = sg::counters::LimitSource::NONE}};
  sg::ReportData data;
  check(sg::detail::writeReport("harness_json_schema_limits.json",
                                data,
                                {},
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                entries),
        "the writer wrote the document that carries the record (FR-051)");
  const std::string text =
      sg::test::readText("harness_json_schema_limits.json");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    std::printf("  the limit record: %s\n", clean.error().c_str());
  }
  check(clean.has_value(),
        "theLimitRecordPairsAtTheStampedMinor: the document that carries the "
        "record passes the schema (FR-051)");

  limitPlantStandsRefused(
      text,
      R"JSON({"extra":true,"device":"cpu","limit":6,"source":"probe"})JSON",
      "/counterLimits/0/extra",
      "theLimitRecordPairsAtTheStampedMinor: the closed limit item refuses a "
      "key the schema lists nowhere (FR-051, FR-010)");
  limitPlantStandsRefused(
      text,
      R"JSON({"source":"probe","limit":6,"device":"cpu"})JSON",
      "/counterLimits/0",
      "theLimitRecordPairsAtTheStampedMinor: a limit key out of the stated "
      "order stands refused (FR-051, FR-010)");
  limitPlantStandsRefused(
      text,
      R"JSON({"device":"cpu","limit":6,"source":"guess"})JSON",
      "/counterLimits/0/source",
      "theLimitRecordPairsAtTheStampedMinor: a source word outside the three "
      "stands refused (FR-051, FR-010)");
  limitPlantStandsRefused(text,
                          R"JSON({"device":"cpu","limit":"six","source":"probe"})JSON",
                          "/counterLimits/0/limit",
                          "theLimitRecordPairsAtTheStampedMinor: a limit of "
                          "another kind stands " "refused (FR-051, FR-010)");
// specs/048-hwlat-probe T034: the six moved `x-order` lists, the two moved
// enum lists, the closed objects and the still-standing top level. The rows
// read the schema file the way the standing rows read it, and plant documents
// the checker reads (FR-004, FR-029, D-6).

// The names of the schema list named `key` that stands at or after `from`,
// joined by commas with no space, so one list reads as one line whatever the
// file's wrapping (FR-029).
auto listNames(const std::string& schema,
               std::string_view key,
               std::size_t from) -> std::string
{
  const auto at = schema.find(std::string(key), from);
  check(at != std::string::npos, "the schema states the list the row reads");
  if (at == std::string::npos) {
    return {};
  }
  const auto open = schema.find('[', at);
  const auto close = schema.find(']', open);
  std::string names;
  for (auto pos = open + 1; pos < close; ++pos) {
    if (schema[static_cast<std::size_t>(pos)] != '"') {
      continue;
    }
    const auto end = schema.find('"', pos + 1);
    if (!names.empty()) {
      names.push_back(',');
    }
    names += schema.substr(static_cast<std::size_t>(pos) + 1,
                           end - static_cast<std::size_t>(pos) - 1);
    pos = end;
  }
  return names;
}

// The written order of the six objects, as `contracts/report-json.md` states
// it after the move (FR-029).
constexpr std::
    string_view
        kJitterOrder =
            "mode,thresholdNs,loopPeriodNs,method,series,cpus,stats,"
            "droppedGaps," "disclosure,limits";
constexpr std::string_view kStatsOrder =
    "cpu,gapCount,gapsPerSecond,minNs,medianNs,meanNs,p90Ns,p99Ns,p999Ns,"
    "p9999Ns,maxNs,totalNs,totalPercent,runtimeNs,thresholdNs,loopPeriodNs,"
    "droppedGaps,smiDelta,nmiDelta,smiGapCount,gaps";
constexpr std::string_view kGapOrder = "stamp,gapNs,smi";
constexpr std::string_view kPrecheckStatsOrder =
    "gapCount,gapsPerSecond,minNs,medianNs,meanNs,p90Ns,p99Ns,p999Ns,p9999Ns,"
    "maxNs,totalNs,totalPercent,runtimeNs,thresholdNs,loopPeriodNs,"
    "droppedGaps,smiDelta,nmiDelta,smiGapCount";
constexpr std::string_view kTracerOrder =
    "engine,windowUs,widthUs,thresholdUs,cpus";
constexpr std::string_view kTracerCpuOrder =
    "cpu,nmi,irq,softirq,thread,hardware,maxGapNs,gapCount,nmiCount";

// The standing `required` lists, which gain no name (FR-029, specs/036
// FR-014).
constexpr std::string_view kJitterRequired =
    "mode,thresholdNs,loopPeriodNs,cpus,stats,droppedGaps,disclosure";
constexpr std::string_view kStatsRequired =
    "cpu,gapCount,gapsPerSecond,minNs,medianNs,meanNs,p90Ns,p99Ns,p999Ns,"
    "p9999Ns,maxNs,totalNs,totalPercent,runtimeNs,thresholdNs,loopPeriodNs,"
    "droppedGaps";
constexpr std::string_view kGapRequired = "stamp,gapNs";
constexpr std::string_view kTracerRequired = "engine,cpus";
constexpr std::string_view kTracerCpuRequired =
    "cpu,nmi,irq,softirq,thread,hardware";

// The top-level order, which stands unchanged: every new key joins an object
// the document already carries (FR-029).
constexpr std::string_view kTopOrder =
    "schemaVersion,speedgun,run,jitter,precheck,graph,benchmarks,hunt";

// The positions of the six objects in the schema file, in the order the rows
// read them.
struct SchemaPlaces
{
  std::size_t jitter = 0;
  std::size_t stats = 0;
  std::size_t gap = 0;
  std::size_t precheckStats = 0;
  std::size_t tracer = 0;
  std::size_t tracerCpu = 0;
};

auto readPlaces(const std::string& schema) -> SchemaPlaces
{
  SchemaPlaces places;
  places.jitter = schema.find("\"jitter\": {");
  places.stats = schema.find("\"stats\": {", places.jitter);
  places.stats = schema.find("\"items\": {", places.stats);
  places.gap = schema.find("\"gaps\": {", places.stats);
  places.gap = schema.find("\"items\": {", places.gap);
  const auto precheck = schema.find("\"precheck\": {");
  places.precheckStats = schema.find("\"stats\": {", precheck);
  places.tracer = schema.find("\"tracer\": {");
  places.tracerCpu = schema.find("\"cpus\": {", places.tracer);
  places.tracerCpu = schema.find("\"items\": {", places.tracerCpu);
  check(places.jitter != std::string::npos && places.stats != std::string::npos
            && places.gap != std::string::npos
            && places.precheckStats != std::string::npos
            && places.tracer != std::string::npos
            && places.tracerCpu != std::string::npos,
        "the schema carries the six objects the detector keys join (FR-029)");
  return places;
}

// The written document with the tracer object of the detector engine: the
// engine word, the three settings the record carries and two cpu entries,
// the first with the three detector figures and the second without a
// figure (FR-027).
auto documentWithTracer() -> std::string
{
  sg::ReportData data;
  sg::PrecheckRecord record;
  sg::PrecheckCpuRecord cpu;
  cpu.cpu = 2;
  cpu.planned = true;
  cpu.stats.smiDelta = 1;
  cpu.stats.nmiDelta = 4;
  cpu.stats.smiGapCount = 2;
  record.cpus.push_back(cpu);
  sg::PrecheckTracerRecord tracer;
  tracer.engine = "osnoise";
  sg::PrecheckTracerCpuRecord first;
  first.cpu = 2;
  first.hardware = 2;
  first.maxGapNs = 7000;
  first.gapCount = 3;
  first.nmiCount = 2;
  sg::PrecheckTracerCpuRecord second;
  second.cpu = 5;
  second.hardware = 1;
  tracer.cpus = {first, second};
  tracer.windowUs = 10000;
  tracer.widthUs = 5000;
  tracer.thresholdUs = 1000;
  record.tracer = tracer;
  check(sg::detail::writeReport(
            "harness_json_schema_tracer.json", data, {}, std::nullopt, record),
        "the writer wrote the document with the tracer object (FR-014)");
  return sg::test::readText("harness_json_schema_tracer.json");
}

// The text with `keys` spliced in after the whole of `needle`.
auto after(const std::string& text,
           std::string_view needle,
           std::string_view keys) -> std::string
{
  const auto at = text.find(needle);
  check(at != std::string::npos,
        "the document carries the key the row splices");
  if (at == std::string::npos) {
    return text;
  }
  std::string spliced = text;
  spliced.insert(at + needle.size(), std::string(",") + std::string(keys));
  return spliced;
}

// FR-029, D-6: each of the six lists states the order the writer writes, the
// new names in the places the contract names.
auto theSixOrdersReadTheWrittenOrder() -> void
{
  const std::string schema = sg::test::readText(SG_SCHEMA_PATH);
  const auto places = readPlaces(schema);
  check(listNames(schema, "\"x-order\"", places.jitter) == kJitterOrder,
        "the jitter order states the limit key after the disclosure key "
        "(FR-029, FR-031)");
  check(listNames(schema, "\"x-order\"", places.stats) == kStatsOrder,
        "the stats order states the three attribution names after the "
        "standing count, ahead of the gap list (FR-025, FR-029)");
  check(listNames(schema, "\"x-order\"", places.gap) == kGapOrder,
        "the gap order states the mark after the gap figure (FR-026, FR-029)");
  check(listNames(schema, "\"x-order\"", places.precheckStats)
            == kPrecheckStatsOrder,
        "the pre-check stats order states the three attribution names after "
        "the standing sixteen (FR-025, FR-029)");
  check(listNames(schema, "\"x-order\"", places.tracer) == kTracerOrder,
        "the tracer order states the three settings beside the engine word "
        "(FR-027, FR-029)");
  check(listNames(schema, "\"x-order\"", places.tracerCpu) == kTracerCpuOrder,
        "the tracer cpu order states the three detector figures after the "
        "standing hardware figure (FR-027, FR-029)");
}

// FR-029, specs/036 FR-014: no new name joins a `required` list, so a
// document written before the feature keeps loading.
auto theRequiredListsHoldNoNewName() -> void
{
  const std::string schema = sg::test::readText(SG_SCHEMA_PATH);
  const auto places = readPlaces(schema);
  check(
      listNames(schema, "\"required\"", places.jitter) == kJitterRequired
          && listNames(schema, "\"required\"", places.stats) == kStatsRequired
          && listNames(schema, "\"required\"", places.gap) == kGapRequired
          && listNames(schema, "\"required\"", places.precheckStats)
              == kStatsRequired
          && listNames(schema, "\"required\"", places.tracer) == kTracerRequired
          && listNames(schema, "\"required\"", places.tracerCpu)
              == kTracerCpuRequired,
      "no new name joins a required list (FR-029, specs/036 FR-014, A-7)");
}

// FR-029: the top level stands unchanged, because no top-level key joins the
// document.
auto theTopLevelOrderStandsUnchanged() -> void
{
  const std::string schema = sg::test::readText(SG_SCHEMA_PATH);
  check(listNames(schema, "\"x-order\"", 0) == kTopOrder,
        "the top-level order holds the eight standing names (FR-029, D-13)");
}

// FR-029, FR-004: the two enum lists take the detector words, and a word
// outside the method list still refuses.
auto theTwoEnumListsTakeTheDetectorWords() -> void
{
  const auto standing = documentWithJitter();
  const auto method = sg::detail::checkReport(
      SG_SCHEMA_PATH, withSeries(standing, "\"HWLAT\"", kSeriesItem));
  check(method.has_value(),
        "the method list takes the detector word (FR-002, FR-029)");
  if (!method.has_value()) {
    std::printf("  the detector method word refused: %s\n",
                method.error().c_str());
  }
  const auto engine = sg::detail::checkReport(
      SG_SCHEMA_PATH,
      std::string(documentWithTracer())
          .replace(documentWithTracer().find("\"osnoise\""), 9, "\"hwlat\""));
  check(engine.has_value(),
        "the engine list takes the detector word (FR-018, FR-029)");
  if (!engine.has_value()) {
    std::printf("  the detector engine word refused: %s\n",
                engine.error().c_str());
  }
  const auto outside = sg::detail::checkReport(
      SG_SCHEMA_PATH, withSeries(standing, "\"HWLATO\"", kSeriesItem));
  check(!outside.has_value(),
        "a method word outside the four still stands refused (FR-002, FR-005)");
}

// FR-029, FR-025, FR-027, FR-031: each new key joins the `properties` of the
// object that carries it, so a document that holds the keys in the written
// order passes.
auto theWrittenKeysStandInTheSchema() -> void
{
  const auto stats = after(documentWithJitter(),
                           "\"droppedGaps\":30",
                           "\"smiDelta\":1,\"nmiDelta\":4,\"smiGapCount\":2");
  const auto limits =
      after(stats,
            "\"disclosure\":\"the run's disclosure\"",
            "\"limits\":\"probe runs with interrupts enabled\"");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, limits);
  check(answer.has_value(),
        "the schema lists the three attribution names and the limit text "
        "(FR-025, FR-029, FR-031)");
  if (!answer.has_value()) {
    std::printf("  the planted document refused: %s\n", answer.error().c_str());
  }
  const auto tracer = after(documentWithTracer(),
                            "\"hardware\":2",
                            "\"maxGapNs\":7000,\"gapCount\":3,\"nmiCount\":2");
  const auto figures = sg::detail::checkReport(SG_SCHEMA_PATH, tracer);
  check(figures.has_value(),
        "the schema lists the three detector figures of a tracer cpu "
        "(FR-027, FR-029)");
  if (!figures.has_value()) {
    std::printf("  the planted figures refused: %s\n", figures.error().c_str());
  }
}

// FR-029, specs/036 FR-006: every touched object stays closed, so a key the
// schema lists nowhere still refuses with the pointer of that object.
auto eachTouchedObjectStaysClosed() -> void
{
  struct Plant
  {
    std::string text;
    const char* pointer;
  };

  const std::vector<Plant> plants {
      Plant {after(documentWithJitter(),
                   "\"disclosure\":\"the run's " "disclosure\"",
                   "\"extra\":true"),
             "/jitter/extra"},
      Plant {
          after(documentWithJitter(), "\"droppedGaps\":30", "\"extra\":true"),
          "/jitter/stats/0/extra"},
      Plant {after(documentWithTracer(), "\"hardware\":2", "\"extra\":true"),
             "/precheck/tracer/cpus/0/extra"},
      Plant {
          after(documentWithTracer(), "\"skippedGroups\":[]", "\"extra\":true"),
          "/precheck/extra"},
  };
  for (const Plant& plant : plants) {
    const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, plant.text);
    check(!answer.has_value(),
          "each closed object refuses a key the schema lists nowhere "
          "(FR-029, specs/036 FR-006)");
    if (!answer.has_value()) {
      check(answer.error().find(plant.pointer) != std::string::npos,
            "the refusal names the object that holds the unlisted key "
            "(FR-029, FR-010)");
    }
  }
}

}  // namespace

auto main() -> int
{
  plantedDisagreementsAreNamedByPointer();
  entryHuntKeysPairAtStampedMinor(
      sg::test::readText("harness_json_schema.json"));
  brokenSchemasAreRefused(sg::test::readText("harness_json_schema.json"));
  theOpenShapesOfTheSchemaLanguagePass(
      sg::test::readText("harness_json_schema.json"));
  // specs/048-hwlat-probe T034: the six moved orders, the two moved enum
  // lists and the closed objects. The rows stand red until T041 moves the
  // schema file, so they close the unit.
  theSixOrdersReadTheWrittenOrder();
  theRequiredListsHoldNoNewName();
  theTopLevelOrderStandsUnchanged();
  theTwoEnumListsTakeTheDetectorWords();
  theWrittenKeysStandInTheSchema();
  eachTouchedObjectStaysClosed();
  theSeriesKeysPairAtTheStampedMinor();
  aClosedSeriesItemRefusesAnAddedKey();
  aSeriesKeyOutOfOrderStandsRefused();
  aMethodWordOutsideTheThreeStandsRefused();
  anEmptyPeriodsArrayPasses();
  aPeriodItemOfTheWrongKindStandsRefused();
  theLimitRecordPairsAtTheStampedMinor();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
