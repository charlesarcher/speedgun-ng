// ============================================================================
// T017: the written document validates, and the checker names the pointer
// of a disagreement (specs/023-json-reports; FR-008, FR-010, D-05).
//
// The oracle is `checkReport`, which reads `docs/schema/report-v1.json`
// with no validator library. A document the writer produced passes. Five
// planted documents, each with one disagreement of the list
// `contracts/schema-file.md` states, each fail with the JSON pointer of
// that position in the message. Frameworkless check()/fail() convention.
// ============================================================================

#include <string>

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
// `rank`, and the document the writer produces pairs the schema at minor
// `1.9`.
auto entryHuntKeysPairAtMinorOneNine(const std::string& documentText) -> void
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
  check(rankAt != std::string::npos
            && correlationAt != std::string::npos
            && slopeAt != std::string::npos && deltaAt != std::string::npos
            && rankAt < correlationAt && correlationAt < slopeAt
            && slopeAt < deltaAt,
        "the entry node names the three rank keys after `rank` in its "
        "`x-order` (FR-032, B-9)");

  check(documentText.find("\"schemaVersion\":\"1.9\"") != std::string::npos,
        "the document the writer produces stands at minor 1.9 (FR-032)");
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
  entryHuntKeysPairAtMinorOneNine(
      sg::test::readText("harness_json_schema.json"));
  brokenSchemasAreRefused(sg::test::readText("harness_json_schema.json"));
  theOpenShapesOfTheSchemaLanguagePass(
      sg::test::readText("harness_json_schema.json"));
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
