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
#include <string>
#include <string_view>

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
// produces pairs the schema at the minor the writer stamps, `1.14`, the
// minor since specs/046-hunter-topdown added the `topdownArea` key.
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

  check(documentText.find("\"schemaVersion\":\"1.14\"") != std::string::npos,
        "the document the writer produces stands at the stamped minor 1.14 "
        "(FR-032, specs/039 FR-032, specs/037 FR-005, specs/043 FR-006, "
        "specs/042-topdown FR-036, specs/046-hunter-topdown FR-020)");
}

// specs/043 T025, US4, FR-006, R-16: the schema node of the series record.
// The item stands closed, its `x-order` names the 21 keys in the written
// order, the method word stands in a list of three, and the period item
// stands closed with its two keys. The document the writer produces pairs
// the schema at the minor the writer stamps, `1.14`, the minor since
// specs/046-hunter-topdown added the `topdownArea` key of the per-event
// node.
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
  check(documentText.find("\"schemaVersion\":\"1.14\"") != std::string::npos,
        "theSeriesKeysPairAtTheStampedMinor: the document the writer produces "
        "stands at the stamped minor 1.14 (FR-006, R-16)");
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

}  // namespace

auto main() -> int
{
  plantedDisagreementsAreNamedByPointer();
  entryHuntKeysPairAtStampedMinor(
      sg::test::readText("harness_json_schema.json"));
  brokenSchemasAreRefused(sg::test::readText("harness_json_schema.json"));
  theOpenShapesOfTheSchemaLanguagePass(
      sg::test::readText("harness_json_schema.json"));
  theSeriesKeysPairAtTheStampedMinor();
  aClosedSeriesItemRefusesAnAddedKey();
  aSeriesKeyOutOfOrderStandsRefused();
  aMethodWordOutsideTheThreeStandsRefused();
  anEmptyPeriodsArrayPasses();
  aPeriodItemOfTheWrongKindStandsRefused();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
