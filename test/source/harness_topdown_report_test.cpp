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
// The gate cases add the public comparison to that seam: the case writes two
// documents, loads both back and hands them to `compareReports` with the rule
// texts it names, so the tree reaches the rule through the writer and the
// reader. The fault of a rule with no node to read is the text
// `contracts/topdown-rule.md` states; the compare mode owns the `error: ` line
// and status 2, and `harness_compare_cli_test` drives that pair (FR-037,
// FR-038, FR-039, FR-050).
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
//   theGateReadsOneNodeOfEachTree
//                            the two rules SC-13 names each read their node's
//                            value in both reports and answer their own
//                            verdict (FR-037, SC-13)
//   aGateRuleWithNoNodeToReadAnswersOneLine
//                            a node the contender lacks, a contender with no
//                            object, a baseline written without the option,
//                            and a node the merge left as a gap each answer
//                            one quoted line naming the rule, the value, the
//                            benchmark and the report (FR-030, FR-038, FR-039,
//                            FR-050)
// ============================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

// The scratch documents of the writer and reader scenarios, one per shape a
// document answers: the supported tree, the record of an unsupported host, the
// entry of a run that names no option, the entry that carries a hunt object
// beside its record, and the document that carries an array where the object
// stands. The names stand under the `harness_json_` prefix the other JSON
// suites of this directory already write and read back.
constexpr const char* kTreeDocument = "harness_json_topdown_tree.json";
constexpr const char* kUnsupportedDocument =
    "harness_json_topdown_unsupported.json";
constexpr const char* kPlainDocument = "harness_json_topdown_plain.json";
constexpr const char* kHuntDocument = "harness_json_topdown_hunt.json";
constexpr const char* kOptionlessDocument =
    "harness_json_topdown_optionless.json";
constexpr const char* kWrongKindDocument =
    "harness_json_topdown_wrongkind.json";
constexpr const char* kRefusedRowsDocument =
    "harness_json_topdown_refused_rows.json";
constexpr const char* kNoSourceDocument = "harness_json_topdown_nosource.json";

// The five documents of the two gate scenarios: the pair whose trees carry the
// two nodes the two rules name, the contender whose tree carries the deeper
// node and lacks the level-1 node, the entry a run wrote with no option, which
// carries no object at all, and the contender whose level-1 node stands as a
// gap.
constexpr const char* kGateBaselineDocument =
    "harness_json_topdown_gate_base.json";
constexpr const char* kGateContenderDocument =
    "harness_json_topdown_gate_contender.json";
constexpr const char* kGatePartialDocument =
    "harness_json_topdown_gate_partial.json";
constexpr const char* kGateTreelessDocument =
    "harness_json_topdown_gate_treeless.json";
constexpr const char* kGateGapDocument = "harness_json_topdown_gate_gap.json";
constexpr const char* kNonFiniteDocument =
    "harness_json_topdown_nonfinite.json";

// The four keys of the two levels of the planted tree, in the order the writer
// emits them: parent before child (FR-032, contracts/topdown-report-key.md).
constexpr std::size_t kBackendBound = 0;
constexpr std::size_t kBadSpeculation = 2;
constexpr std::size_t kLevelTwoChild = 3;
constexpr std::size_t kLevelTwoGap = 4;

// The position of the node the table row leaves as a gap, and the level the
// planted table holds below the level the option asked for.
constexpr std::uint64_t kAskedLevel = 4;
constexpr std::uint64_t kShownLevel = 3;

// The benchmark the four gate documents hold, and the primary time of the one
// row of each: a rule over `time` reads that number, while a rule over a node
// reads the node's own value.
constexpr const char* kGateBenchmark = "bmTopdownGate";
constexpr double kGateTimeNs = 1000.0;

// The four node values the two gate trees answer. The level-1 node falls by
// 7.5 points, past the absolute bound of the rule that reads it, and the deeper
// node rises by 0.375 points, 6.1 percent of its own baseline and inside the
// bound of the rule that reads it (FR-037, SC-13).
constexpr double kGateLevelOneBaseline = 52.5;
constexpr double kGateLevelOneContender = 45.0;
constexpr double kGateLevelTwoBaseline = 6.125;
constexpr double kGateLevelTwoContender = 6.5;

// The two rule texts SC-13 names: the first reads the level-1 node, the second
// the deeper one, and neither needs a quoting of its own (FR-037, SC-13).
constexpr const char* kLevelOneRule = "topdown:backend_bound:higher:abs=5";
constexpr const char* kLevelTwoRule = "topdown:intel.tma_l1_bound:lower:10%";

// The value name the three fault cases read: the level-1 node the baseline
// carries, which is the node the contender leaves out in the first case
// (FR-038, FR-039).
constexpr const char* kLevelOneValue = "topdown:backend_bound";

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

// The whole document of one gate scenario, loaded through the reader the gate
// reads (FR-034, FR-037).
[[nodiscard]] auto loadedDocument(const char* path) -> sg::ReportData
{
  const auto loaded = sg::loadReport(path);
  if (!loaded.has_value()) {
    failed("the reader refused the gate document: " + loaded.error());
  }
  return *loaded;
}

// The tree one gate document carries: the level-1 node and the deeper node the
// two rule texts name, each under the value the case gives. Both names carry
// the `topdown.` head, because that head is what the collector answers and what
// a rule name drops (FR-032, FR-037).
[[nodiscard]] auto gateRecord(const double levelOne,
                              const double deeper) -> sg::TopdownRecord
{
  return sg::TopdownRecord {
      .supported = true,
      .reason = {},
      .reference = "v4.3",
      .directory = "arch/x86/skylakex/",
      .files = plantedFiles(),
      .askedLevel = 2,
      .level = 2,
      .nodes =
          {
              sg::TopdownNode {
                  .name = "topdown.backend_bound",
                  .vendorName = "TopL1.BackendBound",
                  .parent = {},
                  .level = 1,
                  .value = levelOne,
                  .availability = Availability::COUNTABLE,
                  .threshold = "> 20 %",
              },
              sg::TopdownNode {
                  .name = "topdown.intel.tma_l1_bound",
                  .vendorName = "tma_l1_bound",
                  .parent = "topdown.backend_bound",
                  .level = 2,
                  .value = deeper,
                  .availability = Availability::COUNTABLE,
                  .threshold = ">= 10 %",
              },
          },
      .refused = {},
  };
}

// The same tree with its level-1 node gone: the shape of a contender whose
// table answered the deeper row and left the row above it unread (FR-038).
[[nodiscard]] auto gateTreeWithoutLevelOne(const double deeper)
    -> sg::TopdownRecord
{
  auto record = gateRecord(kGateLevelOneBaseline, deeper);
  record.nodes.erase(record.nodes.begin());
  return record;
}

// The same tree with its level-1 node left as a gap by the merge: the row
// stands in the tree and carries no value, which a rule reads as a missing
// value (FR-030, FR-038).
[[nodiscard]] auto gateTreeWithLevelOneGapped(const double deeper)
    -> sg::TopdownRecord
{
  auto record = gateRecord(kGateLevelOneBaseline, deeper);
  record.nodes.front().value = {};
  record.nodes.front().availability = Availability::GAP;
  return record;
}

// The one entry of one gate document: the measured row the gate reads, and the
// record the case names, which stands absent for the entry a run wrote with no
// option (FR-035, FR-039).
[[nodiscard]] auto gateEntry(std::optional<sg::TopdownRecord> record)
    -> sg::BenchmarkResult
{
  auto result = makeResult(kGateBenchmark);
  auto& row = result.rows.emplace_back();
  row.timePerIterationNs = kGateTimeNs;
  row.cpuTimePerIterationNs = kGateTimeNs;
  result.topdown = std::move(record);
  return result;
}

// The two loaded documents of one gate pair, each read back with the one entry
// and the one row a rule reaches its value through.
[[nodiscard]] auto gatePair(const char* baselinePath, const char* contenderPath)
    -> std::pair<sg::ReportData, sg::ReportData>
{
  auto baseline = loadedDocument(baselinePath);
  auto contender = loadedDocument(contenderPath);
  for (const sg::ReportData* const report : {&baseline, &contender}) {
    checked(
        report->results.size() == 1 && report->results.front().rows.size() == 1,
        "the gate document holds one entry carrying one row");
  }
  return {std::move(baseline), std::move(contender)};
}

// The rules of one gate scenario, each text through the public parse, so the
// grammar and the comparison meet inside the case (FR-004, FR-037).
[[nodiscard]] auto gateRules(std::initializer_list<const char*> texts)
    -> std::vector<sg::CompareRule>
{
  std::vector<sg::CompareRule> rules;
  for (const char* const text : texts) {
    auto parsed = sg::parseCompareRule(text);
    if (!parsed.has_value()) {
      failed(std::string("the parser refused the rule ") + text + ": "
             + parsed.error());
    }
    rules.push_back(std::move(*parsed));
  }
  return rules;
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
  check(text.starts_with("{\"schemaVersion\":\"1.16\""),
        "the option-less document stamps the minor 1.16, the one difference"
        " this feature makes to its bytes (FR-035, FR-036,"
        " specs/046-hunter-topdown FR-023, specs/041-counter-probe FR-051)");
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

// The one line each of the four refusals answers, spelled as the answer table
// of `contracts/topdown-selection.md` states it. The record carries this text
// in `reason`, so a later reader of the document learns why the tree stands
// absent (FR-033, FR-040, SC-04).
constexpr std::string_view kUnknownVendorReason {
    "topdown: NoSuchVendorXXXXX family 999 model 999: no vendored table "
    "carries " "this vendor"};
constexpr std::string_view kArmReason {
    "topdown: Arm 0x410fd490: no Arm table is vendored"};
constexpr std::string_view kHybridReason {
    "topdown: GenuineIntel family 6 model 151 stepping 2: the selected table "
    "'arch/x86/alderlake/' publishes rows for two core scopes, and this feature "
    "makes no claim for a hybrid part"};
constexpr std::string_view kNoRowsReason {
    "topdown: AuthenticAMD family 25 model 15: the selected table "
    "'arch/x86/amdzen3/' holds no row of any top-down group"};

// The four identities SC-04 counts, beside the one line the refusal of that
// kind answers: the vendor, the identity in the spelling the contract states,
// and the reason, with the directory between quote marks where the rule read
// one (FR-004 to FR-007, FR-033, SC-04).
struct NoSourceCase
{
  const char* what;
  sg::counters::CpuIdentity identity;
  std::string_view reason;
};

// The four identities SC-04 counts: an unknown vendor, an Arm part by its
// `MIDR`, an Intel hybrid part whose directory publishes the two core scopes,
// and an AMD part whose directory holds no row of any top-down group. Each
// answers out of the table this build embeds, so no case below reads a cpu of
// the host it runs on (SC-04, spec D-13).
[[nodiscard]] auto noSourceCases() -> std::vector<NoSourceCase>
{
  sg::counters::CpuIdentity unknown;
  unknown.vendor = "NoSuchVendorXXXXX";
  unknown.family = 999;
  unknown.model = 999;

  sg::counters::CpuIdentity arm;
  arm.midr = std::optional<std::uint32_t> {0x410fd490};

  sg::counters::CpuIdentity hybrid;
  hybrid.vendor = "GenuineIntel";
  hybrid.family = 6;
  hybrid.model = 0x97;
  hybrid.stepping = 2;

  sg::counters::CpuIdentity zen3;
  zen3.vendor = "AuthenticAMD";
  zen3.family = 25;
  zen3.model = 15;

  return {
      NoSourceCase {
          "a vendor no vendored table carries", unknown, kUnknownVendorReason},
      NoSourceCase {"an Arm part by its `MIDR`", arm, kArmReason},
      NoSourceCase {"an Intel hybrid part", hybrid, kHybridReason},
      NoSourceCase {
          "an AMD part whose table holds no top-down row", zen3, kNoRowsReason},
  };
}

// T078: the `refused` array carries the name, the kind and the reason of every
// row of the record, and each of the four identities SC-04 counts writes the
// record of a host that has no source: `supported` false beside its reason and
// its empty arrays (FR-033, FR-034, SC-04).
auto theRecordCarriesRefusedRowsAndNamesAnUnsupportedHost() -> void
{
  // A pair of refused rows, so the array answers for each row of a record and
  // not for one item a single-row document could carry by accident (FR-033).
  auto want = plantedRecord();
  want.refused.push_back(sg::TopdownRefusal {
      .name = "topdown.intel.bad_arity",
      .refusal = "MALFORMED",
      .reason = "a helper call of two arguments stands where one stands",
  });
  auto result = makeResult("bmTopdownRefusedRows");
  result.topdown = want;
  const auto text = writeDocument(kRefusedRowsDocument, result);
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    failed("a tree with two refused rows disagrees with the schema: "
           + clean.error());
  }
  const auto body = objectBody(text, keyAt(text, "topdown"));
  const auto rowGap = orderGap(firstItem(body, "refused"), kRefusedKeys);
  checked(rowGap.empty(),
          "the first refused item carries name, kind and reason in the order "
          "the" " schema states"
              + rowGap);
  checked(body.find("\"name\":\"topdown.intel.frontend_l1_bound\"")
              != std::string_view::npos
              && body.find("\"kind\":\"UNRESOLVED\"") != std::string_view::npos
              && body.find("\"name\":\"topdown.intel.bad_arity\"")
                     != std::string_view::npos
              && body.find("\"kind\":\"MALFORMED\"") != std::string_view::npos
              && body.find("the row names a metric no other row")
                     != std::string_view::npos
              && body.find("a helper call of two arguments")
                     != std::string_view::npos,
          "the refused array carries the name, the UPPER SNAKE kind and the "
          "reason of each row of the record (FR-033)");
  const auto refusedRows = loadedRecord(kRefusedRowsDocument);
  const auto difference = recordDifference(refusedRows, want);
  checked(difference.empty(),
          "both refused rows read back field for field beside their tree: "
              + difference);

  for (const NoSourceCase& item : noSourceCases()) {
    // The record of a run that met the refusal, as the runner hangs it on the
    // result: the detail the refusal read, and nothing beside it (FR-040,
    // FR-041).
    auto options = sg::detail::RunOptions {};
    options.fixedIterations = sg::test::kWindowIterations;
    options.topdownLevel = 1;
    options.topdownIdentity = item.identity;
    const auto run = sg::test::scriptedRun(options);
    if (!run.result.topdown.has_value() || run.result.topdown->supported) {
      failed(std::string {"the run handed "} + item.what
             + " carries no record of an unsupported host");
    }
    const sg::TopdownRecord& record = *run.result.topdown;
    checked(record.reason == item.reason,
            std::string {"the record of "} + item.what
                + " carries the one line its refusal answers as the reason: "
                + record.reason);
    checked(record.nodes.empty() && record.refused.empty()
                && record.files.empty() && record.directory.empty()
                && record.level == 0,
            std::string {"the record of "} + item.what
                + " carries no node, no refused row, no file and no directory "
                  "(FR-033)");

    const auto hostText = writeDocument(kNoSourceDocument, run.result);
    const auto reasonKey = "\"reason\":\"" + std::string {item.reason} + "\"";
    checked(hostText.find("\"supported\":false") != std::string::npos
                && hostText.find(reasonKey) != std::string::npos
                && hostText.find("\"nodes\":[]") != std::string::npos
                && hostText.find("\"refused\":[]") != std::string::npos,
            std::string {"the document of "} + item.what
                + " writes supported false, its reason and two empty arrays "
                  "(FR-033, FR-034)");
    checked(hostText.find("\"files\":[]") != std::string::npos,
            std::string {"the document of "} + item.what
                + " names no contributing file either, the third array the "
                  "record's invariant closes (FR-033)");
    const auto hostClean = sg::detail::checkReport(SG_SCHEMA_PATH, hostText);
    if (!hostClean.has_value()) {
      failed(std::string {"the document of "} + item.what
             + " disagrees with the schema: " + hostClean.error());
    }
    const auto back = loadedRecord(kNoSourceDocument);
    checked(!back.supported && back.reason == item.reason
                && back.nodes.empty()
                && back.refused.empty() && back.files.empty(),
            std::string {"the document of "} + item.what
                + " reads back unsupported with its reason and its empty arrays "
                  "(FR-034)");
  }
}

// One row of the gate's answer, read through its six fields, with the row
// printed for the reader of a failure.
auto checkGateRow(const sg::CompareEntry& entry,
                  const char* ruleText,
                  const double wantBaseline,
                  const double wantContender,
                  const sg::CompareState wantState,
                  const char* what) -> void
{
  const bool values = entry.baseline.has_value() && entry.contender.has_value()
      && sameValue(*entry.baseline, wantBaseline)
      && sameValue(*entry.contender, wantContender);
  if (!values || entry.name != kGateBenchmark || entry.ruleText != ruleText
      || entry.state != wantState)
  {
    std::printf("  row: name=%s rule=%s baseline=%g contender=%g state=%d\n",
                entry.name.c_str(),
                entry.ruleText.c_str(),
                entry.baseline.value_or(0.0),
                entry.contender.value_or(0.0),
                static_cast<int>(entry.state));
  }
  check(values && entry.name == kGateBenchmark && entry.ruleText == ruleText
            && entry.state == wantState,
        what);
}

// The one line of the missing value, in the order the contract states: the
// rule in quotes, the value name, the benchmark, and the report the rule
// looked in (`contracts/topdown-rule.md`).
[[nodiscard]] auto missingText(std::string_view ruleText,
                               std::string_view value,
                               std::string_view report) -> std::string
{
  return "the rule '" + std::string(ruleText) + "' finds no "
      + std::string(value) + " for " + std::string(kGateBenchmark) + " in the "
      + std::string(report) + " report";
}

// One fault of the gate: the comparison answers one line, and that line is the
// text above, whole.
auto checkGateFault(const sg::ReportData& baseline,
                    const sg::ReportData& contender,
                    const char* ruleText,
                    const std::string& want,
                    const char* what) -> void
{
  const auto answer = sg::compareReports(
      baseline, contender, gateRules({ruleText}), sg::CompareOptions {});
  if (answer.has_value()) {
    std::printf("  the gate answered %zu rows and no fault\n",
                answer->entries.size());
    fail(what);
    return;
  }
  if (answer.error() != want) {
    std::printf("  answered: %s\n  expected: %s\n",
                answer.error().c_str(),
                want.c_str());
  }
  check(
      answer.error() == want && answer.error().find('\n') == std::string::npos,
      what);
}

// T056: the gate over two written trees. Each of the two rules SC-13 names
// reads its own node's value in both reports, so the pair answers two rows, two
// verdicts and four values, and no row reads the time column (FR-037, SC-13).
auto theGateReadsOneNodeOfEachTree() -> void
{
  const auto baselineBytes = writeDocument(
      kGateBaselineDocument,
      gateEntry(gateRecord(kGateLevelOneBaseline, kGateLevelTwoBaseline)));
  const auto contenderBytes = writeDocument(
      kGateContenderDocument,
      gateEntry(gateRecord(kGateLevelOneContender, kGateLevelTwoContender)));
  check(sg::detail::checkReport(SG_SCHEMA_PATH, baselineBytes).has_value(),
        "the baseline document of the gate pair passes the schema (FR-032)");
  check(sg::detail::checkReport(SG_SCHEMA_PATH, contenderBytes).has_value(),
        "the contender document of the gate pair passes the schema (FR-032)");

  const auto [baseline, contender] =
      gatePair(kGateBaselineDocument, kGateContenderDocument);
  const auto answer =
      sg::compareReports(baseline,
                         contender,
                         gateRules({kLevelOneRule, kLevelTwoRule}),
                         sg::CompareOptions {});
  if (!answer.has_value()) {
    failed("the gate refused the two rules: " + answer.error());
  }
  if (answer->entries.size() != 2) {
    failed("two rules over one benchmark answered "
           + std::to_string(answer->entries.size()) + " rows");
  }
  checkGateRow(answer->entries[0],
               kLevelOneRule,
               kGateLevelOneBaseline,
               kGateLevelOneContender,
               sg::CompareState::REGRESSED,
               "the level-1 rule reads backend_bound in both reports, and the"
               " fall past its bound answers REGRESSED (FR-037, SC-13)");
  checkGateRow(answer->entries[1],
               kLevelTwoRule,
               kGateLevelTwoBaseline,
               kGateLevelTwoContender,
               sg::CompareState::OK,
               "the dotted rule reads the deeper node in both reports, and the"
               " rise inside its bound answers OK (FR-037, SC-13)");
  check(!answer->passed,
        "the row that regressed moves the verdict of the pair (FR-037)");
}

// T056: the cases where a rule names no node to read. Each answers the one
// line `contracts/topdown-rule.md` states; the compare mode prints it as its
// one line at status 2, and `harness_compare_cli_test` drives that pair
// (FR-030, FR-038, FR-039, FR-050, D-14). A node the merge left as a gap
// stands in the tree and carries no value, so it reads here the way a node the
// contender carries nowhere reads.
auto aGateRuleWithNoNodeToReadAnswersOneLine() -> void
{
  const auto baselineBytes = writeDocument(
      kGateBaselineDocument,
      gateEntry(gateRecord(kGateLevelOneBaseline, kGateLevelTwoBaseline)));
  static_cast<void>(writeDocument(
      kGatePartialDocument,
      gateEntry(gateTreeWithoutLevelOne(kGateLevelTwoContender))));
  static_cast<void>(
      writeDocument(kGateTreelessDocument, gateEntry(std::nullopt)));
  static_cast<void>(writeDocument(
      kGateGapDocument,
      gateEntry(gateTreeWithLevelOneGapped(kGateLevelTwoBaseline))));
  check(sg::detail::checkReport(SG_SCHEMA_PATH, baselineBytes).has_value(),
        "the tree document of the three faults passes the schema (FR-032)");

  const auto [tree, partial] =
      gatePair(kGateBaselineDocument, kGatePartialDocument);
  checkGateFault(tree,
                 partial,
                 kLevelOneRule,
                 missingText(kLevelOneRule, kLevelOneValue, "contender"),
                 "a node the contender lacks stops the comparison (FR-038)");

  const auto [baseline, treeless] =
      gatePair(kGateBaselineDocument, kGateTreelessDocument);
  checkGateFault(baseline,
                 treeless,
                 kLevelOneRule,
                 missingText(kLevelOneRule, kLevelOneValue, "contender"),
                 "a contender with no topdown object stops the comparison"
                 " (FR-039, D-14)");

  const auto [plain, contender] =
      gatePair(kGateTreelessDocument, kGateBaselineDocument);
  checkGateFault(plain,
                 contender,
                 kLevelOneRule,
                 missingText(kLevelOneRule, kLevelOneValue, "baseline"),
                 "a baseline written without the option stops the comparison"
                 " naming the baseline report (FR-035, FR-039)");

  const auto [whole, gapped] =
      gatePair(kGateBaselineDocument, kGateGapDocument);
  checkGateFault(whole,
                 gapped,
                 kLevelOneRule,
                 missingText(kLevelOneRule, kLevelOneValue, "contender"),
                 "a node the merge left as a gap stops the comparison naming"
                 " the contender report (FR-030, FR-038)");
}

// One scenario of the suite with the name the verdict line prints.
// The belt at the writer, with the rule that refuses such a figure at the node
// standing above it (`runner.cpp`). A figure that is no number reaches the
// builder only past that rule, and the builder answers a finite magnitude for
// it: `2.696539702293474e+308` for a not-a-number and
// `1.797693134862316e+308` for an infinity, a figure a schema key typed
// `["number","null"]` accepts as a measurement. This case hands the writer both
// spellings directly and reads the document back
// (specs/042-topdown FR-032, PC-17).
auto aNonFiniteNodeFigureWritesNoFigure() -> void
{
  auto record = gateRecord(kGateLevelOneBaseline, kGateLevelTwoBaseline);
  record.nodes.front().value = std::numeric_limits<double>::quiet_NaN();
  record.nodes.front().availability = Availability::ABSENT;
  record.nodes.back().value = std::numeric_limits<double>::infinity();
  record.nodes.back().availability = Availability::ABSENT;
  const std::string bytes =
      writeDocument(kNonFiniteDocument, gateEntry(std::move(record)));
  check(
      bytes.find("nan") == std::string::npos
          && bytes.find("inf") == std::string::npos
          && bytes.find("2.696539702293474e+308") == std::string::npos
          && bytes.find("1.797693134862316e+308") == std::string::npos,
      "the document spells no figure for the two nodes no denominator served");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, bytes);
  check(clean.has_value(),
        "the document of the two refused nodes answers the schema");

  const sg::ReportData loaded = loadedDocument(kNonFiniteDocument);
  const auto& nodes = loaded.results.front().topdown->nodes;
  check(nodes.size() == 2, "the document carries both nodes");
  for (const sg::TopdownNode& node : nodes) {
    check(!node.value.has_value(),
          "the reader gives back no figure for the node that carried none");
    check(
        node.availability == Availability::ABSENT,
        "the reader gives back the state of the denominator that is not there");
  }
}

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
      {"theRecordCarriesRefusedRowsAndNamesAnUnsupportedHost",
       theRecordCarriesRefusedRowsAndNamesAnUnsupportedHost},
      {"theGateReadsOneNodeOfEachTree", theGateReadsOneNodeOfEachTree},
      {"aGateRuleWithNoNodeToReadAnswersOneLine",
       aGateRuleWithNoNodeToReadAnswersOneLine},
      {"aNonFiniteNodeFigureWritesNoFigure",
       aNonFiniteNodeFigureWritesNoFigure},
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
