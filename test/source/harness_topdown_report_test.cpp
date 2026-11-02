// ============================================================================
// The `topdown` entry key of the written record (specs/042-topdown T052, T053,
// T054; US3; FR-032, FR-033, FR-034, FR-035, FR-036, SC-06, SC-07).
//
// The seams are the writer, `loadReport` and the schema checker over the
// `SG_SCHEMA_PATH` document. A case hangs one record on one result, writes the
// document, reads the bytes back and compares the two field by field, or hands
// the written bytes to the checker and names the finding it answers. Every key
// and every `x-order` position is `contracts/topdown-report-key.md`'s, and no
// key is mirrored here.
//
// The tree a case writes stands in the record of specs/042-topdown E-16, the
// carrier `stripTopdownColumns` fills for a run that names the option. The
// neutral names, the vendor names and the threshold texts are the texts a
// vendored Intel directory carries, so a written node reads as a table row
// reads (FR-010, FR-017). No case opens a counter, reads a clock, sleeps or
// names a cpu.
//
// The option-less document comes from the scripted run of
// `detail/json_fixture.hpp`, the run `harness_json_report_test` writes with:
// it names no option, so its result carries no record and its bytes answer the
// bytes the same run wrote before this feature, the stamp aside (FR-035).
//
// Scenario map:
//   theRecordRoundTrips      the written tree reads back field for field, and
//                            an unsupported record comes back unsupported
//                            (FR-032, FR-033, FR-034, SC-07)
//   aDocumentWithoutTheKeyLoads
//                            a document with no key loads with the member
//                            disengaged (FR-035)
//   aRunWithoutTheOptionWritesNoKey
//                            the scripted run of a run that names no option
//                            writes no key and keeps the bytes it wrote
//                            before the key, the stamp aside (FR-035, SC-06)
//   theWrittenTreePassesTheSchemaWithTheKeyAfterHunt
//                            the writer's own document passes the schema, with
//                            `topdown` standing after `hunt` in the entry order
//                            and its nine keys in their stated order
//                            (FR-032, FR-036)
// ============================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/json_fixture.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_topdown.hpp"
#include "speedgun-ng/hunt.hpp"

#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH, the schema the checker reads"
#endif

namespace
{

using sg::test::check;
using sg::test::fail;
using sg::test::sameValue;

using Availability = sg::counters::Availability;

// The scratch documents of the four scenarios, one per shape a document
// answers: the supported tree, the record of an unsupported host, the entry of
// a run that names no option, the entry that carries a hunt object beside its
// record, and the document that carries an array where the object stands. The
// names stand under the `harness_json_` prefix the other JSON suites of this
// directory already write and read back.
constexpr const char* kTreeDocument = "harness_json_topdown_tree.json";
constexpr const char* kUnsupportedDocument =
    "harness_json_topdown_unsupported.json";
constexpr const char* kPlainDocument = "harness_json_topdown_plain.json";
constexpr const char* kHuntDocument = "harness_json_topdown_hunt.json";
constexpr const char* kOptionlessDocument =
    "harness_json_topdown_optionless.json";
constexpr const char* kWrongKindDocument =
    "harness_json_topdown_wrongkind.json";

// The four keys of the two levels of the planted tree, in the order the writer
// emits them: parent before child (FR-032, contracts/topdown-report-key.md).
constexpr std::size_t kBackendBound = 0;
constexpr std::size_t kFrontendBound = 1;
constexpr std::size_t kBadSpeculation = 2;
constexpr std::size_t kLevelTwoChild = 3;
constexpr std::size_t kLevelTwoGap = 4;

// The position of the node the table row leaves as a gap, and the level the
// planted table holds below the level the option asked for.
constexpr std::uint64_t kAskedLevel = 4;
constexpr std::uint64_t kShownLevel = 3;

// The nine keys of the object, in the `x-order` the schema states.
constexpr std::array<const char*, 9> kRecordKeys {
    "supported",
    "reason",
    "reference",
    "directory",
    "files",
    "asked_level",
    "level",
    "nodes",
    "refused",
};

// The seven keys of one node item and the three of one refused item, likewise
// in the order the schema states.
constexpr std::array<const char*, 7> kNodeKeys {
    "name",
    "vendor_name",
    "parent",
    "level",
    "value",
    "availability",
    "threshold",
};

constexpr std::array<const char*, 2> kFileKeys {
    "name",
    "manifest_hash",
};

constexpr std::array<const char*, 3> kRefusedKeys {
    "name",
    "kind",
    "reason",
};

[[noreturn]] auto failed(const std::string& what) -> void
{
  std::fprintf(stderr, "TOPDOWN REPORT TEST FAIL: %s\n", what.c_str());
  std::exit(1);
}

// The same failure for a claim that carries a message the scenario assembled.
auto checked(const bool condition, const std::string& what) -> void
{
  if (!condition) {
    failed(what);
  }
}

// The whole file as bytes, and the document of one result.
[[nodiscard]] auto readBytes(const std::string& path) -> std::string
{
  return sg::test::readText(path);
}

// The measured entry of one scenario: the three identity texts, one measured
// row and no metric column, so the entry object of the document holds the
// standing key set and the record alone decides the shape under test.
[[nodiscard]] auto makeResult(const std::string& name) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;
  result.suite = name;
  result.caseName = name;
  result.outcome = sg::RunOutcome::MEASURED;
  return result;
}

// The two files whose rows reached the planted tree, with the pair E-02
// carries: the embedded name and the digest the manifest line states (FR-015).
[[nodiscard]] auto plantedFiles() -> std::vector<sg::counters::TableFile>
{
  return {
      sg::counters::TableFile {
          .name = "frontend_by_category_metrics.json",
          .manifestHash = "0a1b2c3d4e5f60718293a4b5c6d7e8f901234567"},
      sg::counters::TableFile {
          .name = "fetch_and_execution_metrics.json",
          .manifestHash = "f9e8d7c6b5a4938271605f4e3d2c1b0a98765432"},
  };
}

// The tree of a supported host: three level-1 nodes, two level-2 nodes under
// the first of them, one of the pair left as a gap by the merge, and one
// refused row of the same directory (FR-032, FR-033).
[[nodiscard]] auto plantedRecord() -> sg::TopdownRecord
{
  return sg::TopdownRecord {
      .supported = true,
      .reason = {},
      .reference = "v4.3",
      .directory = "arch/x86/skylakex/",
      .files = plantedFiles(),
      .askedLevel = kAskedLevel,
      .level = kShownLevel,
      .nodes =
          {
              sg::TopdownNode {
                  .name = "backend_bound",
                  .vendorName = "TopL1.BackendBound",
                  .parent = {},
                  .level = 1,
                  .value = 52.5,
                  .availability = Availability::COUNTABLE,
                  .threshold = "> 20 %",
              },
              sg::TopdownNode {
                  .name = "frontend_bound",
                  .vendorName = "TopL1.FrontendBound",
                  .parent = {},
                  .level = 1,
                  .value = 25.75,
                  .availability = Availability::COUNTABLE,
                  .threshold = "> 15 %",
              },
              sg::TopdownNode {
                  .name = "bad_speculation",
                  .vendorName = "TopL1.BadSpeculation",
                  .parent = {},
                  .level = 1,
                  .value = 12.25,
                  .availability = Availability::COUNTABLE,
                  .threshold = {},
              },
              sg::TopdownNode {
                  .name = "topdown.intel.backend_l2_bound",
                  .vendorName = "TopL2.Backend.CoreBound",
                  .parent = "backend_bound",
                  .level = 2,
                  .value = 6.125,
                  .availability = Availability::COUNTABLE,
                  .threshold = ">= 10 %",
              },
              sg::TopdownNode {
                  .name = "topdown.intel.backend_l3_bound",
                  .vendorName = "TopL2.Backend.MemoryBound",
                  .parent = "backend_bound",
                  .level = 2,
                  .value = {},
                  .availability = Availability::GAP,
                  .threshold = ">= 10 %",
              },
          },
      .refused =
          {
              sg::TopdownRefusal {
                  .name = "topdown.intel.frontend_l1_bound",
                  .refusal = "UNRESOLVED",
                  .reason = "the row names a metric no other row",
              },
          },
  };
}

// The record of a host with no table: unsupported, with the one reason the
// selection answered and no file, node or refused row beside it (FR-033,
// D-14). The level shown stands at zero, because no table set a depth.
[[nodiscard]] auto unsupportedRecord() -> sg::TopdownRecord
{
  auto record = sg::TopdownRecord {};
  record.supported = false;
  record.reason = "no top-down table is vendored for Arm processors";
  record.askedLevel = kAskedLevel;
  return record;
}

// The hunt record of the entry of the fourth scenario: the key the tree follows
// in the entry order (FR-032, FR-036).
[[nodiscard]] auto plantedHunt() -> sg::hunt::EntryHunt
{
  return sg::hunt::EntryHunt {
      .method = sg::MetricMethod::PASSES,
      .passCount = 1,
      .top = 20,
      .entries = {},
      .skipped = {},
      .retriedPasses = 0,
  };
}

// Write one result as a whole document and hand back its bytes.
[[nodiscard]] auto writeDocument(
    const char* path, const sg::BenchmarkResult& result) -> std::string
{
  auto data = sg::ReportData {};
  data.results.push_back(result);
  data.rowRaw.push_back({});
  data.contextRaw.push_back({});
  check(sg::detail::writeReport(path, data),
        "the writer wrote the document of the scenario");
  return readBytes(path);
}

// The first disagreement between a loaded record and the record that wrote it,
// named by the field it stands in; the empty string answers two records that
// answer each other field for field (FR-034).
[[nodiscard]] auto recordDifference(
    const sg::TopdownRecord& got, const sg::TopdownRecord& want) -> std::string
{
  if (got.supported != want.supported) {
    return "supported";
  }
  if (got.reason != want.reason) {
    return "reason";
  }
  if (got.reference != want.reference) {
    return "reference";
  }
  if (got.directory != want.directory) {
    return "directory";
  }
  if (got.askedLevel != want.askedLevel) {
    return "asked_level";
  }
  if (got.level != want.level) {
    return "level";
  }
  if (got.files.size() != want.files.size()) {
    return "files";
  }
  for (std::size_t index = 0; index < want.files.size(); ++index) {
    const auto& gotFile = got.files[index];
    const auto& wantFile = want.files[index];
    if (gotFile.name != wantFile.name) {
      return "files[" + std::to_string(index) + "].name";
    }
    if (gotFile.manifestHash != wantFile.manifestHash) {
      return "files[" + std::to_string(index) + "].manifest_hash";
    }
  }
  if (got.nodes.size() != want.nodes.size()) {
    return "nodes";
  }
  for (std::size_t index = 0; index < want.nodes.size(); ++index) {
    const auto& gotNode = got.nodes[index];
    const auto& wantNode = want.nodes[index];
    const std::string where = "nodes[" + std::to_string(index) + "]";
    if (gotNode.name != wantNode.name) {
      return where + ".name";
    }
    if (gotNode.vendorName != wantNode.vendorName) {
      return where + ".vendor_name";
    }
    if (gotNode.parent != wantNode.parent) {
      return where + ".parent";
    }
    if (gotNode.level != wantNode.level) {
      return where + ".level";
    }
    if (gotNode.availability != wantNode.availability) {
      return where + ".availability";
    }
    if (gotNode.threshold != wantNode.threshold) {
      return where + ".threshold";
    }
    if (gotNode.value.has_value() != wantNode.value.has_value()) {
      return where + ".value";
    }
    if (wantNode.value.has_value()
        && !sameValue(*gotNode.value, *wantNode.value))
    {
      return where + ".value";
    }
  }
  if (got.refused.size() != want.refused.size()) {
    return "refused";
  }
  for (std::size_t index = 0; index < want.refused.size(); ++index) {
    const auto& gotRow = got.refused[index];
    const auto& wantRow = want.refused[index];
    const std::string where = "refused[" + std::to_string(index) + "]";
    if (gotRow.name != wantRow.name) {
      return where + ".name";
    }
    if (gotRow.refusal != wantRow.refusal) {
      return where + ".kind";
    }
    if (gotRow.reason != wantRow.reason) {
      return where + ".reason";
    }
  }
  return {};
}

// The loaded record of one written document, refused with the message the
// reader answers when the document stops loading.
[[nodiscard]] auto loadedRecord(const char* path) -> sg::TopdownRecord
{
  const auto loaded = sg::loadReport(path);
  if (!loaded.has_value()) {
    failed("the reader refused the written document: " + loaded.error());
  }
  if (loaded->results.size() != 1) {
    failed("the loaded document holds one entry");
  }
  const auto& record = loaded->results.front().topdown;
  if (!record.has_value()) {
    failed("the loaded entry carries no topdown record");
  }
  return *record;
}

// The byte at which one key of the document opens, refused when it stands
// absent.
[[nodiscard]] auto keyAt(std::string_view text,
                         std::string_view name) -> std::size_t
{
  const std::string key = "\"" + std::string(name) + "\":";
  const auto at = text.find(key);
  if (at == std::string::npos) {
    failed("the document holds no key named " + std::string(name));
  }
  return at;
}

// One object of the document, cut from the brace its key opens at to the brace
// that closes it, so a key of that object is read where it stands.
[[nodiscard]] auto objectBody(std::string_view text,
                              std::size_t key) -> std::string_view
{
  const auto open = text.find('{', key);
  if (open == std::string_view::npos) {
    failed("the key opens no object");
  }
  int depth = 0;
  for (auto at = open; at < text.size(); ++at) {
    if (text[at] == '{') {
      ++depth;
    } else if (text[at] == '}') {
      --depth;
      if (depth == 0) {
        return text.substr(open, at - open);
      }
    }
  }
  failed("no brace closes the object");
  return {};
}

// The keys of one object stand in the order named, each once, inside the
// object that opens at `body`.
[[nodiscard]] auto orderGap(std::string_view body,
                            std::span<const char* const> keys) -> std::string
{
  std::size_t previous = 0;
  for (const char* key : keys) {
    const auto at = body.find(std::string("\"") + key + "\":");
    if (at == std::string_view::npos) {
      return std::string("no key named ") + key;
    }
    if (at < previous) {
      return std::string(key) + " stands out of the order the schema states";
    }
    previous = at;
  }
  return {};
}

// The first item object of the array one key holds.
[[nodiscard]] auto firstItem(std::string_view body,
                             std::string_view key) -> std::string_view
{
  const auto at = body.find(std::string("\"") + std::string(key) + "\":[");
  if (at == std::string_view::npos) {
    failed(std::string("the object holds no array named ") + std::string(key));
  }
  const auto open = body.find('{', at);
  if (open == std::string_view::npos) {
    return {};
  }
  return objectBody(body, open);
}

// T052: the written tree reads back field for field, and the record of an
// unsupported host comes back unsupported with its reason and its three empty
// arrays (FR-032, FR-033, FR-034, SC-07).
auto theRecordRoundTrips() -> void
{
  const auto want = plantedRecord();
  auto result = makeResult("bmTopdownTree");
  result.topdown = want;
  const auto text = writeDocument(kTreeDocument, result);
  check(text.find("\"topdown\":{") != std::string::npos,
        "the entry of a run that carries a record writes the topdown object"
        " (FR-032)");

  const auto got = loadedRecord(kTreeDocument);
  const auto difference = recordDifference(got, want);
  if (!difference.empty()) {
    failed("the loaded record disagrees with the written record at "
           + difference);
  }
  check(got.files.size() == 2,
        "the loaded record names both contributing files (FR-015)");
  check(got.files[0].name == "frontend_by_category_metrics.json"
            && got.files[0].manifestHash
                == "0a1b2c3d4e5f60718293a4b5c6d7e8f901234567",
        "the first file entry keeps its name and its manifest_hash pair "
        "(FR-015," " FR-032)");
  check(got.files[1].name == "fetch_and_execution_metrics.json"
            && got.files[1].manifestHash
                   == "f9e8d7c6b5a4938271605f4e3d2c1b0a98765432",
        "the second file entry keeps its name and its manifest_hash pair"
        " (FR-015, FR-032)");
  check(got.askedLevel == kAskedLevel && got.level == kShownLevel,
        "the level the run asked for and the level the table holds both read"
        " back (FR-020, FR-032)");
  check(got.nodes[kLevelTwoChild].parent == "backend_bound"
            && got.nodes[kLevelTwoChild].level == 2,
        "a level-2 node reads back with its parent and its own level (FR-032)");
  check(!got.nodes[kLevelTwoGap].value.has_value()
            && got.nodes[kLevelTwoGap].availability == Availability::GAP,
        "a gap node reads back with no value and the gap availability (FR-030,"
        " FR-032)");
  check(got.nodes[kBackendBound].threshold == "> 20 %"
            && got.nodes[kBadSpeculation].threshold.empty(),
        "the threshold text of each row reads back unchanged (FR-017, FR-032)");
  check(
      got.refused.size() == 1 && got.refused[0].refusal == "UNRESOLVED"
          && !got.refused[0].reason.empty(),
      "the refused row reads back with its kind word and its reason (FR-033)");
  check(text.find("\"value\":52.5") != std::string::npos
            && text.find("\"value\":null") != std::string::npos,
        "the writer spells a counted value as a number and a gap as null"
        " (FR-030, FR-032)");
  check(text.find("\"availability\":\"GAP\"") != std::string::npos,
        "the writer spells the availability with the standing word of the"
        " counter columns (FR-030)");

  const auto plain = unsupportedRecord();
  auto refused = makeResult("bmTopdownUnsupported");
  refused.topdown = plain;
  const auto unsupportedText = writeDocument(kUnsupportedDocument, refused);
  check(unsupportedText.find("\"supported\":false") != std::string::npos
            && unsupportedText.find("\"files\":[]") != std::string::npos
            && unsupportedText.find("\"nodes\":[]") != std::string::npos
            && unsupportedText.find("\"refused\":[]") != std::string::npos,
        "an unsupported host writes the key with its reason and three empty"
        " arrays (FR-033, D-14)");
  const auto back = loadedRecord(kUnsupportedDocument);
  check(!back.supported && back.reason == plain.reason
            && back.files.empty() && back.nodes.empty() && back.refused.empty(),
        "the unsupported record reads back unsupported, with its reason and"
        " empty arrays (FR-033, FR-034)");
  check(back.askedLevel == kAskedLevel && back.level == 0,
        "the unsupported record keeps the level the run asked for (FR-020)");

  // The key of the wrong kind keeps the refusal of its own kind, the one text
  // every key of the object answers a value of another kind with (FR-050).
  auto wrongKind = text;
  const auto body = objectBody(text, keyAt(text, "topdown"));
  const auto open = static_cast<std::size_t>(body.data() - text.data());
  wrongKind.replace(open, body.size() + 1, "[]");
  sg::test::writeText(kWrongKindDocument, wrongKind);
  const auto refusedRead = sg::loadReport(kWrongKindDocument);
  checked(!refusedRead.has_value()
              && refusedRead.error()
                         == "/benchmarks/0/topdown: expected an object, found "
                            "array",
          "a topdown key of the wrong kind names the kind it carries: "
          + (refusedRead.has_value() ? std::string("it loaded")
                                     : refusedRead.error()));
}

// T053: a document that holds no topdown key loads, and leaves the member
// disengaged the way an absent hunt key does (FR-035).
auto aDocumentWithoutTheKeyLoads() -> void
{
  const auto text = writeDocument(kPlainDocument, makeResult("bmTopdownPlain"));
  check(text.find("\"topdown\"") == std::string::npos,
        "the entry of a run that carries no record writes no topdown key"
        " (FR-035)");
  const auto loaded = sg::loadReport(kPlainDocument);
  if (!loaded.has_value()) {
    failed("a document with no topdown key stopped loading: " + loaded.error());
  }
  if (loaded->results.size() != 1) {
    failed("the loaded document holds no single entry");
  }
  check(!loaded->results.front().topdown.has_value(),
        "a document with no topdown key loads with the member disengaged"
        " (FR-035)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    failed("a document with no topdown key disagrees with the schema: "
           + clean.error());
  }
}

// T053: the scripted run of a run that names no option writes no key, and its
// entry closes on the key it closed on before this feature: `spreadWarning`
// stands last, the benchmarks array closes on it and the document closes on
// the array. The stamp is the one difference this feature makes to those bytes
// (FR-035, SC-06).
auto aRunWithoutTheOptionWritesNoKey() -> void
{
  auto options = sg::detail::RunOptions {};
  options.fixedIterations = sg::test::kWindowIterations;
  const auto run = sg::test::scriptedRun(options);
  check(!run.result.topdown.has_value(),
        "a run that names no option carries no record");
  check(run.result.rows.size() == 1, "the scripted run measured one row");

  auto data = sg::ReportData {};
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  check(sg::detail::writeReport(kOptionlessDocument, data),
        "the writer wrote the document of the scripted run");
  const auto text = readBytes(kOptionlessDocument);

  check(text.find("\"topdown\"") == std::string::npos,
        "a run that names no option writes no topdown key (FR-035)");
  const std::string key = "\"spreadWarning\":";
  const auto at = text.find(key);
  const auto again =
      text.find(key, at == std::string::npos ? 0 : at + key.size());
  checked(at != std::string::npos && again == std::string::npos,
          "the entry of the scripted run carries one spreadWarning key");
  const std::string_view tail {text};
  const auto value = at + key.size();
  check(tail.substr(value).starts_with("false}]}")
            || tail.substr(value).starts_with("true}]}"),
        "the entry closes on spreadWarning, so no key stands after it (FR-035,"
        " SC-06)");
  check(text.starts_with("{\"schemaVersion\":\"1.13\""),
        "the option-less document stamps the minor 1.13, the one difference"
        " this feature makes to its bytes (FR-035, FR-036)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    failed("the document of a run that names no option disagrees with the"
           " schema: "
           + clean.error());
  }
}

// T054: the writer's own document passes the checker, with the key standing
// after `hunt` in the entry order and each object carrying its keys in the
// order the schema states (FR-032, FR-036).
auto theWrittenTreePassesTheSchemaWithTheKeyAfterHunt() -> void
{
  auto result = makeResult("bmTopdownHunt");
  result.hunt = plantedHunt();
  result.topdown = plantedRecord();
  const auto text = writeDocument(kHuntDocument, result);

  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    failed("the written tree disagrees with the schema: " + clean.error());
  }
  const auto spreadAt = keyAt(text, "spreadWarning");
  const auto huntAt = keyAt(text, "hunt");
  const auto topdownAt = keyAt(text, "topdown");
  checked(spreadAt < huntAt && huntAt < topdownAt,
          "the entry writes hunt, then topdown, after the last standing key of"
          " the entry before this feature");
  const auto record = objectBody(text, topdownAt);
  const auto recordGap = orderGap(record, kRecordKeys);
  if (!recordGap.empty()) {
    failed("the topdown order disagrees with the schema: " + recordGap);
  }
  const auto node = firstItem(record, "nodes");
  const auto nodeGap = orderGap(node, kNodeKeys);
  if (!nodeGap.empty()) {
    failed("the node item order disagrees with the schema: " + nodeGap);
  }
  const auto file = firstItem(record, "files");
  const auto fileGap = orderGap(file, kFileKeys);
  if (!fileGap.empty()) {
    failed("the file item order disagrees with the schema: " + fileGap);
  }
  const auto refused = firstItem(record, "refused");
  const auto refusedGap = orderGap(refused, kRefusedKeys);
  if (!refusedGap.empty()) {
    failed("the refused item order disagrees with the schema: " + refusedGap);
  }
  checked(text.rfind("}}]}") == text.size() - 4,
          "the topdown object closes the entry, the entry closes the"
          " benchmarks array, and the array closes the document");
}

// One scenario of the suite with the name the verdict line prints.
struct Scenario
{
  const char* name;
  void (*body)();
};

// Run one scenario in a child of this process and read its wait status, so one
// failing scenario reports beside the verdict of every other scenario.
auto runIsolated(const Scenario& scenario) -> bool
{
  std::fflush(stdout);
  const pid_t child = fork();
  if (child < 0) {
    fail("cannot start the scenario process");
    return false;
  }
  if (child == 0) {
    scenario.body();
    std::fflush(stdout);
    std::exit(sg::test::failures == 0 ? 0 : 1);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    fail("cannot read the status of the scenario process");
    return false;
  }
  const bool passed = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  std::printf("harness_topdown_report_test %s: %s\n",
              scenario.name,
              passed ? "pass" : "FAIL");
  std::fflush(stdout);
  return passed;
}

}  // namespace

auto main() -> int
{
  const std::vector<Scenario> scenarios = {
      {"theRecordRoundTrips", theRecordRoundTrips},
      {"aDocumentWithoutTheKeyLoads", aDocumentWithoutTheKeyLoads},
      {"aRunWithoutTheOptionWritesNoKey", aRunWithoutTheOptionWritesNoKey},
      {"theWrittenTreePassesTheSchemaWithTheKeyAfterHunt",
       theWrittenTreePassesTheSchemaWithTheKeyAfterHunt},
  };
  std::size_t failedScenarios = 0;
  for (const auto& scenario : scenarios) {
    if (!runIsolated(scenario)) {
      ++failedScenarios;
    }
  }
  if (failedScenarios != 0) {
    std::printf("%d scenarios failed\n", static_cast<int>(failedScenarios));
  }
  return failedScenarios == 0 ? 0 : 1;
}
