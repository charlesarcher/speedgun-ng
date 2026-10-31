// ============================================================================
// T007: the record one planted file holds (specs/031-host-validation;
// FR-020, FR-024, FR-030, SC-003; wave 3, M-C).
//
// `readHostRecord` reads the bytes of one planted file against the pinned
// schema, the guard the writer and the reader share (FR-022). Each scenario
// plants a tree under the scratch directory the test owns and writes the
// schema into that tree beside the record, from the bytes the
// `SG_SCHEMA_PATH` definition names, because a repo-relative path cannot
// reach inside a root the test builds (FR-035). Planted valid bytes answer a
// record that carries the planted values, and each planted break of the key
// set, the key order or a value kind answers none (FR-020, FR-030). Each
// empty answer carries the line of that entry, so a red run shows the file,
// the verdict and the CPU type the reader has to name (FR-024, FR-030).
// Nothing here reads `/proc`, `/sys`, a cpu number or a clock, so any runner
// answers the same verdicts (FR-026, FR-033, FR-035).
// ============================================================================

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <unistd.h>

#include "../../tools/hostcheck/mapfile.hpp"
#include "detail/host_record_check.hpp"
#include "detail/json_fixture.hpp"

#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH, the schema the reader reads"
#endif

namespace
{

using sg::test::check;

// The CPU type the fixture's document carries: the record key `mapfileKey`
// renders for the identity the contract plants, and the name its file
// carries with the `.json` suffix (FR-009).
constexpr std::string_view kRecordKey = "GenuineIntel-6-C6-2";

// The four root pairs of `contracts/host-record-json.md`, spelled as that
// contract spells them, so each plant names the object it breaks (FR-020).
constexpr std::string_view kVersionPair = R"ROOT("recordVersion":"1.0")ROOT";
constexpr std::string_view kCpuPair =
    R"CPU("cpu":{"vendor":"GenuineIntel","family":6,"model":198,"stepping":2,"key":"GenuineIntel-6-C6-2","directory":"arch/x86/arrowlake/"})CPU";
constexpr std::string_view kMachinePair =
    R"MACHINE("machine":{"packages":1,"l3Groups":2,"cores":20,"threads":20,"mask":"0-19","l3":[{"ordinal":0,"cpus":"0-7"},{"ordinal":1,"cpus":"8-19"}],"nodes":[{"number":0,"cpus":"0-19"}]})MACHINE";
constexpr std::string_view kEvidencePair =
    R"EVIDENCE("evidence":[{"path":"specs/034-host-noise-precheck/results/quickstart.md","command":"ctest --test-dir build -R harness_precheck_test --output-on-failure","exit":0,"line":"1/1 Test #1: harness_precheck_test ... Passed"}])EVIDENCE";

// The document one host writes: the four root pairs in the order the schema
// states, compact, the document alone (FR-020, FR-023).
auto pinnedRecord() -> std::string
{
  auto record = std::string("{");
  record += kVersionPair;
  record += ',';
  record += kCpuPair;
  record += ',';
  record += kMachinePair;
  record += ',';
  record += kEvidencePair;
  record += '}';
  return record;
}

// The document with one pair spelled another way. The pair is matched by its
// own bytes, so a plant that names no pair of the document reports itself
// before the guard reads anything.
auto withPairRewritten(const std::string& document,
                       std::string_view pair,
                       std::string_view replacement) -> std::string
{
  const auto at = document.find(pair);
  check(at != std::string::npos, "the plant names a pair the document holds");
  if (at == std::string::npos) {
    return {};
  }
  std::string planted(document);
  planted.replace(at, pair.size(), replacement);
  return planted;
}

// The document with one pair cut out. The comma that stands before the pair
// goes with it, so what stands behind reads as JSON.
auto withoutPair(const std::string& document,
                 std::string_view cut) -> std::string
{
  const auto at = document.find(cut);
  check(at != std::string::npos, "the plant names bytes the document holds");
  if (at == std::string::npos) {
    return {};
  }
  std::string planted(document);
  planted.erase(at, cut.size());
  return planted;
}

// The document whose two first root keys swap places: the version pair
// stands behind the identity, against the order the schema carries as
// `x-order` (FR-021).
auto recordWithRootKeysSwapped() -> std::string
{
  auto planted = std::string("{");
  planted += kCpuPair;
  planted += ',';
  planted += kVersionPair;
  planted += ',';
  planted += kMachinePair;
  planted += ',';
  planted += kEvidencePair;
  planted += '}';
  return planted;
}

// The CPU type the reader names for one entry: the entry's own name with the
// `.json` suffix removed, the reading of a document that yields no key
// (FR-024).
auto entryName(std::string_view file) -> std::string
{
  constexpr std::string_view kSuffix = ".json";
  if (file.size() >= kSuffix.size()
      && file.substr(file.size() - kSuffix.size()) == kSuffix)
  {
    return std::string(file.substr(0, file.size() - kSuffix.size()));
  }
  return std::string(file);
}

// One planted tree: a scratch directory this run owns, the pinned schema
// written under `kHostRecordSchemaRelPath`, and the record root beside it
// (FR-020, FR-035). The tree stands under the working directory the test
// binaries own, named with the pid and a sequence so parallel runs stand
// apart, the shape `harness_precheck_test.cpp:73` gives its fake root. It
// holds no `/proc` and no `/sys`, and a mapfile stands in it only where an
// arm plants one.
class PlantedTree
{
public:
  explicit PlantedTree(const std::string_view scenario)
  {
    static int sequence = 0;
    m_root = std::filesystem::path("harness_host_record_check_root")
        / ("pid" + std::to_string(::getpid()) + "-" + std::string(scenario)
           + "-" + std::to_string(sequence++));
    std::error_code failure;
    std::filesystem::create_directories(schemaPath().parent_path(), failure);
    check(!failure, "the planted tree stands created");
    const auto pinned = sg::test::readText(SG_SCHEMA_PATH);
    sg::test::writeText(schemaPath().string(), pinned);
    check(
        sg::test::readText(schemaPath().string()) == pinned,
        "the planted tree holds the bytes the pinned schema carries (FR-020)");
  }

  PlantedTree(const PlantedTree&) = delete;
  auto operator=(const PlantedTree&) -> PlantedTree& = delete;

  ~PlantedTree()
  {
    std::error_code failure;
    std::filesystem::remove_all(m_root, failure);
  }

  auto schemaPath() const -> std::filesystem::path
  {
    return m_root
        / std::filesystem::path(
               std::string(sg::detail::kHostRecordSchemaRelPath));
  }

  // One entry of the record root: the text written whole under the name the
  // listing reads, a record document or one of the entries it passes over
  // (FR-009, FR-024).
  auto writeRecord(const std::string_view name,
                   const std::string& document) const -> std::filesystem::path
  {
    const auto file = recordRoot() / std::string(name);
    std::error_code failure;
    std::filesystem::create_directories(file.parent_path(), failure);
    check(!failure, "the planted record root stands created");
    sg::test::writeText(file.string(), document);
    return file;
  }

  // The schema of an open object, beside the record root and out of its
  // listing: it names no key, requires nothing, closes nothing and states no
  // kind, so every document passes it and each read of the record answers for
  // itself (FR-021, FR-022).
  auto openSchemaPath() const -> std::filesystem::path
  {
    const auto file = m_root / "open-host-record-schema.json";
    sg::test::writeText(file.string(), "{}");
    return file;
  }

  // One file of the tree under the repository-relative path an evidence row
  // cites, the file rules 2 and 3 look for (FR-015, FR-025).
  auto writeTreeFile(const std::string& relative,
                     const std::string& text) const -> std::filesystem::path
  {
    const auto file = m_root / std::filesystem::path(relative);
    std::error_code failure;
    std::filesystem::create_directories(file.parent_path(), failure);
    check(!failure, "the planted evidence file stands created");
    sg::test::writeText(file.string(), text);
    return file;
  }

  // One file of the tree taken away, the state of a cited run whose file a
  // later change deleted (FR-025).
  auto dropTreeFile(const std::filesystem::path& file) const -> void
  {
    std::error_code failure;
    std::filesystem::remove(file, failure);
    check(!failure, "the planted evidence file stands removed");
  }

  // One directory of the record root, the entry a listing has to pass over
  // (FR-029).
  auto makeRecordDir(const std::string_view name) const -> std::filesystem::path
  {
    const auto dir = recordRoot() / std::string(name);
    std::error_code failure;
    std::filesystem::create_directories(dir, failure);
    check(!failure, "the planted entry directory stands created");
    return dir;
  }

  // One path of the planted tree the reader cannot read, the state of an entry
  // the reader cannot open and of a schema the reader cannot read (FR-020,
  // FR-029). A mode is no portable plant: CI runs this unit as root inside the
  // container, where no mode binds and a file closed with `chmod(path, 0)`
  // still opens, so the arm would read a record where it owes a refusal. A
  // directory standing where a file must stand answers EISDIR to every user,
  // root included, the same answer a name under a regular file answers with
  // ENOTDIR, and `readTextFile` answers both with its refusal (FR-029).
  auto blockFile(const std::filesystem::path& file) const -> void
  {
    auto failure = std::error_code();
    check(std::filesystem::remove(file, failure) && !failure,
          "the planted file stands taken away");
    std::filesystem::create_directory(file, failure);
    check(!failure, "a directory stands where the reader must read a file");
  }

  auto recordRoot() const -> std::filesystem::path
  {
    return m_root / "validation";
  }

  auto root() const -> const std::filesystem::path& { return m_root; }

private:
  std::filesystem::path m_root;
};

// The line of `contracts/record-check.md` for one entry, printed beside each
// empty answer so a red run shows the file, the verdict and the CPU type the
// reader has to name for it (FR-024, FR-030).
auto reportBroken(const std::filesystem::path& file,
                  const std::string& cpuType,
                  const std::string& cause) -> void
{
  std::printf("%s  %s  broken  host-record: %s\n",
              file.string().c_str(),
              cpuType.c_str(),
              cause.c_str());
}

// FR-020, FR-024, SC-003: planted valid bytes answer a record, and every
// field carries the value its document spells.
auto plantedValidBytesAnswerTheirValues(const PlantedTree& tree) -> void
{
  const auto file =
      tree.writeRecord(std::string(kRecordKey) + ".json", pinnedRecord());
  const auto record = sg::detail::readHostRecord(file, tree.schemaPath());
  if (!record.has_value()) {
    reportBroken(file,
                 entryName(file.filename().string()),
                 "the guard refused the planted document");
  }
  check(record.has_value(),
        "planted valid bytes answer a record (FR-020, SC-003)");
  if (!record.has_value()) {
    return;
  }
  check(record->recordVersion == "1.0",
        "the record carries the format version its document names (FR-019)");
  check(record->cpu.vendor == "GenuineIntel" && record->cpu.family == 6
            && record->cpu.model == 198
            && record->cpu.stepping == std::optional<int>(2),
        "the identity carries the four figures its document spells (FR-007)");
  check(record->cpu.key == kRecordKey
            && record->cpu.directory == "arch/x86/arrowlake/",
        "the identity carries the record key and the selected directory "
        "(FR-008, FR-009)");
  const bool counts = record->machine.packages == 1U
      && record->machine.l3Groups == 2U && record->machine.cores == 20U
      && record->machine.threads == 20U && record->machine.mask == "0-19";
  check(counts, "the machine block carries its counts and mask (FR-012)");
  check(record->machine.l3.size() == 2U && record->machine.l3[0].ordinal == 0U
            && record->machine.l3[0].cpus == "0-7"
            && record->machine.l3[1].ordinal == 1U
            && record->machine.l3[1].cpus == "8-19",
        "the machine block carries one L3 row per row of the document "
        "(FR-012)");
  const bool nodes = record->machine.nodes.has_value()
      && record->machine.nodes->size() == 1U
      && (*record->machine.nodes)[0].number == 0U
      && (*record->machine.nodes)[0].cpus == "0-19";
  check(nodes,
        "the machine block carries the node rows the document spells (FR-013)");
  check(record->evidence.size() == 1U
            && record->evidence[0].path
                   == "specs/034-host-noise-precheck/results/quickstart.md"
            && record->evidence[0].command
                   == "ctest --test-dir build -R harness_precheck_test "
                      "--output-on-failure"
            && record->evidence[0].exit == 0
            && record->evidence[0].line
                   == "1/1 Test #1: harness_precheck_test ... Passed",
        "the evidence list carries the row the document spells (FR-015)");
}

// FR-007, FR-013: the two nullable keys read their `null` into an absent
// field, and the rest of that document stands read.
auto theUnansweredValuesReadNull(const PlantedTree& tree) -> void
{
  auto document = withPairRewritten(pinnedRecord(),
                                    R"NULL(,"stepping":2)NULL",
                                    R"NULL(,"stepping":null)NULL");
  document =
      withPairRewritten(document,
                        R"NULL("nodes":[{"number":0,"cpus":"0-19"}])NULL",
                        R"NULL("nodes":null)NULL");
  check(!document.empty(), "the plant found both nullable keys");
  if (document.empty()) {
    return;
  }
  const auto file =
      tree.writeRecord(std::string(kRecordKey) + ".json", document);
  const auto record = sg::detail::readHostRecord(file, tree.schemaPath());
  check(record.has_value(),
        "a document whose two nullable keys read null passes the guard "
        "(FR-013)");
  if (!record.has_value()) {
    return;
  }
  check(!record->cpu.stepping.has_value(),
        "a host that answers no leaf 1 reads no stepping (FR-007)");
  check(!record->machine.nodes.has_value(),
        "a machine whose memory reads unavailable reads no node block "
        "(FR-013)");
  check(record->cpu.key == kRecordKey && record->evidence.size() == 1U,
        "the rest of that document stands read (FR-020)");
}

// One planted break of the key set, the key order or a value kind: the file
// the reader reads, the bytes it holds, the break the line names beside it
// and the check the empty answer stands against (FR-020, FR-030).
auto checkOneBreak(const std::string& name,
                   const std::string& document,
                   const char* cause,
                   const char* what) -> void
{
  check(!document.empty(), "one plant holds the document it plants");
  if (document.empty()) {
    return;
  }
  const PlantedTree tree("break");
  const auto file = tree.writeRecord(name, document);
  const auto record = sg::detail::readHostRecord(file, tree.schemaPath());
  reportBroken(file, entryName(name), cause);
  check(!record.has_value(), what);
}

// FR-020, FR-024, FR-030, SC-003: each planted break answers no record, and
// each empty answer carries the line of that entry. The document that drops
// `cpu.key` stands under a name its own keys never spell, so the line can
// read its CPU type from that name alone (FR-024).
auto eachPlantedBreakAnswersNoRecord() -> void
{
  const auto keyless =
      withoutPair(pinnedRecord(), R"BREAK(,"key":"GenuineIntel-6-C6-2")BREAK");
  checkOneBreak("AuthenticAMD-17-17-1.json", keyless,
                "the cpu object holds no key",
                "a document that drops one required key answers no record "
                "(FR-020, FR-030)");

  const auto maskless =
      withoutPair(pinnedRecord(), R"BREAK(,"mask":"0-19")BREAK");
  checkOneBreak(std::string(kRecordKey) + ".json", maskless,
                "the machine object holds no key",
                "a key missing from a second object answers no record "
                "(FR-020, FR-030)");

  const auto added =
      withPairRewritten(pinnedRecord(),
                        R"BREAK("evidence":[{)BREAK",
                        R"BREAK("evidence":[{"extra":true,)BREAK");
  checkOneBreak(std::string(kRecordKey) + ".json", added,
                "an evidence row holds a key beyond the set the schema lists",
                "a key beyond the set a closed object lists answers no record "
                "(FR-020, FR-030)");

  checkOneBreak(std::string(kRecordKey) + ".json", recordWithRootKeysSwapped(),
                "two root keys stand out of the order the schema states",
                "a document whose two root keys swap places answers no record "
                "(FR-020, FR-030)");

  const auto retyped = withPairRewritten(
      pinnedRecord(), R"BREAK(,"family":6)BREAK", R"BREAK(,"family":"6")BREAK");
  checkOneBreak(std::string(kRecordKey) + ".json", retyped,
                "the cpu object holds a value of another kind",
                "a value of a kind the schema does not state answers no "
                "record (FR-020, FR-030)");
}

// ============================================================================
// T021: the first two staleness rules (specs/031-host-validation; FR-015,
// FR-025, SC-003; wave 3, M-C).
//
// `staleReasons` answers every cause one record carries, in the order the
// reader names them, over a planted tree and a planted chooser (FR-025). Rule
// 1 reads the format version of the record against the version the format
// speaks, and rule 2 looks for each cited file under the tree the run names.
// Each arm plants its record in memory and its cited files on that tree, so
// the answers stand the same on a runner that reads no host and no clock
// (FR-026, FR-035). The tree holds its own copy of the pinned schema at the
// path the reader resolves under the root it is given (FR-020).
// ============================================================================

// The version the record format speaks, the one a current record carries
// (FR-019).
constexpr std::string_view kFormatVersion = "1.0";

// The identity of `contracts/host-record-json.md`, planted beside every
// evidence list so a cause stands beside a CPU type the reader names
// (FR-007 to FR-009).
auto plantedIdent() -> sg::detail::HostIdent
{
  auto ident = sg::detail::HostIdent {};
  ident.vendor = "GenuineIntel";
  ident.family = 6;
  ident.model = 198;
  ident.stepping = 2;
  ident.key = std::string(kRecordKey);
  ident.directory = "arch/x86/arrowlake/";
  return ident;
}

// One row of the US3 shape: the file that holds the run, the command that
// wrote it, the status it ended with and the line the row cites (FR-015).
auto plantedRow(std::string path,
                std::string command,
                std::string line) -> sg::detail::EvidenceRow
{
  return sg::detail::EvidenceRow {
      .path = std::move(path),
      .command = std::move(command),
      .exit = 0,
      .line = std::move(line),
  };
}

// The record a staleness arm reads: the fixture's identity, the version the
// format speaks and the rows the arm cites (FR-015, FR-019).
auto plantedRecord(std::vector<sg::detail::EvidenceRow> rows)
    -> sg::detail::HostRecord
{
  auto record = sg::detail::HostRecord {};
  record.recordVersion = std::string(kFormatVersion);
  record.cpu = plantedIdent();
  record.evidence = std::move(rows);
  return record;
}

// One row of the evidence list, the four keys the results table names in the
// order the schema spells them (FR-015, FR-020).
auto evidenceRowText(const sg::detail::EvidenceRow& row) -> std::string
{
  return "{\"path\":\"" + row.path + "\",\"command\":\"" + row.command
      + "\",\"exit\":" + std::to_string(row.exit) + ",\"line\":\"" + row.line
      + "\"}";
}

// The fixture document with its evidence list standing as the rows an arm
// planted, the one list the write and the read share (FR-015, FR-020).
auto recordWithRows(const std::vector<sg::detail::EvidenceRow>& rows)
    -> std::string
{
  auto list = std::string {R"LIST("evidence":[)LIST"};
  auto separator = std::string_view {};
  for (const auto& row : rows) {
    list += separator;
    list += evidenceRowText(row);
    separator = ",";
  }
  list += "]";
  return withPairRewritten(pinnedRecord(), kEvidencePair, list);
}

// The chooser that answers rule 4 for every identity: the mapfile of the tree
// selects a directory, so an arm reads no staleness from the routing. The
// path the answer carries reaches no comparison, because the record keeps the
// directory the writer wrote beside the same key (FR-008, FR-025, R-06).
auto matchingChooser() -> sg::detail::DirectoryChooser
{
  return [](const sg::detail::HostIdent&) -> sg::detail::DirectoryChoice
  {
    return sg::detail::DirectoryChoice {
        .answer = sg::detail::DirectoryAnswer::FOUND,
        .directory = "arch/x86/arrowlake/",
    };
  };
}

// The three acceptance rows of US3, each written into the tree so its cited
// line stands in the file its own row cites (FR-015, SC-003).
auto plantThreeRows(const PlantedTree& tree)
    -> std::vector<sg::detail::EvidenceRow>
{
  const std::vector<sg::detail::EvidenceRow> rows {
      plantedRow("specs/029-hunter-core/results/hunt-all.md",
                 "ctest --test-dir build -R hunter_all --output-on-failure",
                 "1/1 Test #148: hunter_all ... Passed"),
      plantedRow(
          "specs/035-numa-placement/results/placement.md",
          "ctest --test-dir build -R placement_preset --output-on-failure",
          "1/1 Test #161: placement_preset ... Passed"),
      plantedRow(
          "specs/034-host-noise-precheck/results/quickstart.md",
          "ctest --test-dir build -R harness_precheck_test --output-on-failure",
          "1/1 Test #212: harness_precheck_test ... Passed"),
  };
  for (const auto& row : rows) {
    tree.writeTreeFile(row.path,
                       "Run record\n\n" + row.line + "\n\nFigures follow.\n");
  }
  return rows;
}

// The line of `contracts/record-check.md` for one stale record, printed for
// each cause the answer carries so a red run shows the CPU type and the rule
// that fired (FR-024, FR-025, FR-030).
auto reportStale(const sg::detail::HostRecord& record,
                 const std::vector<std::string>& reasons) -> void
{
  for (const auto& reason : reasons) {
    std::printf("validation/%s.json  %s  stale  %s\n",
                record.cpu.key.c_str(),
                record.cpu.key.c_str(),
                reason.c_str());
  }
}

// FR-025, SC-003: a record whose format version lags the version the format
// speaks carries rule 1's cause, and that cause names the two versions.
auto versionBehindTheFormatNamesBothVersions(const PlantedTree& tree) -> void
{
  auto record = plantedRecord({});
  record.recordVersion = "0.9";
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), matchingChooser());
  reportStale(record, reasons);
  check(reasons.size() == 1U,
        "a record behind the version the format speaks carries one cause "
        "(FR-025)");
  const bool namesBoth = reasons.size() == 1U
      && reasons[0].find("0.9") != std::string::npos
      && reasons[0].find(kFormatVersion) != std::string::npos;
  check(namesBoth,
        "the cause names the version the record carries and the version the "
        "format speaks (FR-025)");
}

// FR-015, FR-025: a row whose file stands absent under the tree answers one
// cause, and that cause names the path the reader could not confirm.
auto absentCitedPathNamesItself(const PlantedTree& tree) -> void
{
  const auto row =
      plantedRow("specs/031-host-validation/results/no-such-run.md",
                 "ctest --test-dir build -R host_record --output-on-failure",
                 "1/1 Test #7: host_record ... Passed");
  const auto record = plantedRecord({row});
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), matchingChooser());
  reportStale(record, reasons);
  check(reasons.size() == 1U,
        "a cited path the tree does not hold carries one cause (FR-025)");
  const bool namesPath =
      reasons.size() == 1U && reasons[0].find(row.path) != std::string::npos;
  check(namesPath,
        "the cause names the evidence path the reader could not confirm "
        "(FR-025)");
}

// FR-015, FR-025, FR-027, SC-003: a record whose every cited path stands and
// whose every cited line stands in that file answers no cause.
auto everyStandingRowReadsCurrent(const PlantedTree& tree) -> void
{
  const auto rows = plantThreeRows(tree);
  const auto record = plantedRecord(rows);
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), matchingChooser());
  reportStale(record, reasons);
  check(reasons.empty(),
        "a record whose every cited path and cited line stands carries no "
        "cause (FR-025, FR-027)");
}

// FR-025, SC-003: the arm that deletes one cited file leaves that row the one
// cause, named by the path the reader could not confirm beside the CPU type
// of the record.
auto deletedCitedFileNamesItsRow(const PlantedTree& tree) -> void
{
  const auto rows = plantThreeRows(tree);
  tree.dropTreeFile(tree.root() / std::filesystem::path(rows[1].path));
  const auto record = plantedRecord(rows);
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), matchingChooser());
  reportStale(record, reasons);
  check(reasons.size() == 1U,
        "one deleted cited file leaves one cause beside two standing rows "
        "(FR-025)");
  const bool namesPath = reasons.size() == 1U
      && reasons[0].find(rows[1].path) != std::string::npos;
  check(namesPath,
        "the cause names the path of the row whose file stands deleted "
        "(FR-025)");
}

// ============================================================================
// T022: the last two staleness rules (specs/031-host-validation; FR-025,
// FR-028, FR-035, SC-003, AC-1; wave 3, M-C).
//
// Rule 3 looks for the cited line inside a file that stands, and rule 4 reads
// the answer of the chooser built over the mapfile of the tree the run names.
// Each arm plants its own `DirectoryAnswer`, the shape
// `contracts/record-check.md` gives the chooser, so the rule runs on a planted
// mapfile that no hardware read reaches (FR-025, FR-035, AC-1). The planted
// root holds the mapfile beside the schema copy the reader resolves under that
// root (FR-020, FR-028).
// ============================================================================

// The mapfile of one tree root, the path the chooser the reader's factory
// builds opens at its first question (FR-008, FR-025, R-06).
constexpr std::string_view kMapfileRelPath =
    "external/pmu-events/arch/x86/mapfile.csv";

// The schema of a planted tree, spelled here as the fence of
// `contracts/record-check.md` spells it, so an arm reads the path its own line
// names (FR-020).
constexpr std::string_view kSchemaRelPath = "docs/schema/host-record-v1.json";

// The rows the planted tree holds: one for the Intel part of the fixture,
// spelled as the pinned `external/pmu-events/arch/x86/mapfile.csv` spells
// them, and none for the identity rule 4 names (FR-008, FR-035).
auto plantedMapfile() -> std::string
{
  return "Family-model,Version,Filename,EventType\n"
         "GenuineIntel-6-C[56],v1.17,arrowlake,core\n";
}

// The identity no row of the planted mapfile names, spelled as the record
// spells an identity of that family (FR-007, FR-009).
auto unmappedIdent() -> sg::detail::HostIdent
{
  auto ident = plantedIdent();
  ident.vendor = "AuthenticAMD";
  ident.family = 23;
  ident.model = 1;
  ident.stepping = 0;
  ident.key = "AuthenticAMD-23-1-0";
  ident.directory = "arch/x86/amdzen1/";
  return ident;
}

// The chooser that answers rule 4 for an identity the planted mapfile holds
// no row for: no directory, and the staleness the reader prints beside the
// key and the identity (FR-025).
auto noRowChooser() -> sg::detail::DirectoryChooser
{
  return [](const sg::detail::HostIdent&) -> sg::detail::DirectoryChoice
  {
    return sg::detail::DirectoryChoice {
        .answer = sg::detail::DirectoryAnswer::NO_ROW,
        .directory = {},
    };
  };
}

// FR-025, SC-003: a file that stands while its cited line stands absent in it
// answers one cause, and that cause names the file.
auto citedLineAbsentFromAStandingFileNamesIt(const PlantedTree& tree) -> void
{
  const auto row =
      plantedRow("specs/030-ci-speedup/results/quickstart.md",
                 "ctest --test-dir build -R ci_speedup --output-on-failure",
                 "1/1 Test #9: ci_speedup ... Passed");
  tree.writeTreeFile(row.path, "Run record\n\nThe run moved to another row.\n");
  const auto record = plantedRecord({row});
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), matchingChooser());
  reportStale(record, reasons);
  check(reasons.size() == 1U,
        "a cited line the standing file does not hold carries one cause "
        "(FR-025)");
  const bool namesPath =
      reasons.size() == 1U && reasons[0].find(row.path) != std::string::npos;
  check(namesPath,
        "the cause names the file whose cited line stands absent (FR-025)");
}

// FR-025, FR-028, AC-1: the mapfile of the planted tree holds no row for the
// identity of the record, and the cause names the record key beside the
// identity that stands unmatched.
auto noRowForTheIdentityNamesTheKeyAndTheIdent(const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  auto record = plantedRecord({});
  record.cpu = unmappedIdent();
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), noRowChooser());
  reportStale(record, reasons);
  check(
      reasons.size() == 1U,
      "an identity the mapfile names no row for carries one cause " "(FR-025)");
  const bool namesBoth = reasons.size() == 1U
      && reasons[0].find(record.cpu.key) != std::string::npos
      && reasons[0].find(record.cpu.vendor) != std::string::npos;
  check(namesBoth,
        "the cause names the record key and the identity no row matches "
        "(FR-025)");
}

// FR-008, FR-025, FR-027: the row the planted mapfile holds answers the
// chooser for the identity of the record, and the routing answers no cause,
// while the directory that answer carries reaches no comparison.
auto rowForTheIdentityAnswersNoCause(const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  const auto record = plantedRecord({});
  const auto reasons =
      sg::detail::staleReasons(record, tree.root(), matchingChooser());
  reportStale(record, reasons);
  check(reasons.empty(),
        "an identity the mapfile selects a directory for carries no cause "
        "(FR-008, FR-025, FR-027)");
}

// ============================================================================
// T011: the listing `checkRecords` owns (specs/031-host-validation; FR-024,
// FR-026, FR-027, FR-029, SC-003; wave 3, M-C).
//
// `checkRecords` owns the listing and prints nothing: one reading per `.json`
// entry directly under the record root, in the file-name order of that set,
// and no other entry there (FR-024, FR-029). A record root that stands absent
// and one that holds no `.json` entry arrive as the same empty listing
// through the directory read answered by its error code, so both answer the
// empty pair, the answer that stands apart from the run an unreadable entry
// refuses (D-10, FR-029). The arms plant their records beside a stray file
// without the suffix and a subdirectory, and answer rule 4 through the chooser
// of T022, so no arm reads `/proc`, `/sys`, a cpu or a clock (FR-026, FR-035).
// ============================================================================

// A file of the record root that carries no `.json` suffix, the first entry
// the listing passes over (FR-029).
constexpr std::string_view kStrayName = "notes.md";

// A directory of the record root, the second entry the listing passes over
// (FR-029).
constexpr std::string_view kSubdirName = "drafts";

// The document one host writes for one identity, with the evidence list that
// stands empty, the list the lane commits its own record with (FR-015,
// FR-041).
auto recordBytesFor(const sg::detail::HostIdent& ident) -> std::string
{
  const auto stepping = ident.stepping.has_value()
      ? std::to_string(*ident.stepping)
      : std::string("null");
  const auto cpu = std::string("\"cpu\":{\"vendor\":\"") + ident.vendor
      + "\",\"family\":" + std::to_string(ident.family)
      + ",\"model\":" + std::to_string(ident.model)
      + ",\"stepping\":" + stepping + ",\"key\":\"" + ident.key
      + "\",\"directory\":\"" + ident.directory + "\"}";
  auto record = std::string("{");
  record += kVersionPair;
  record += ',';
  record += cpu;
  record += ',';
  record += kMachinePair;
  record += ',';
  record += R"EVIDENCE("evidence":[])EVIDENCE";
  record += '}';
  return record;
}

// The two entries a listing has to pass over, planted beside the records
// (FR-029).
auto plantStrayEntries(const PlantedTree& tree) -> void
{
  tree.writeRecord(kStrayName, "A note the reader passes over.\n");
  tree.makeRecordDir(kSubdirName);
}

// The one word of one verdict, the three words FR-030 names.
auto verdictWord(const sg::detail::RecordVerdict verdict) -> const char*
{
  if (verdict == sg::detail::RecordVerdict::BROKEN) {
    return "broken";
  }
  if (verdict == sg::detail::RecordVerdict::STALE) {
    return "stale";
  }
  return "valid";
}

// The lines of `contracts/record-check.md` for one answer, printed so a red
// run shows the CPU type, the verdict and the detail of every reading, and
// the path that refused the run (FR-024, FR-029, FR-030).
auto reportReadings(const sg::detail::RecordCheck& answer) -> void
{
  for (const auto& reading : answer.readings) {
    std::printf("validation/%s.json  %s  %s  %s\n",
                reading.cpuType.c_str(),
                reading.cpuType.c_str(),
                verdictWord(reading.verdict),
                reading.detail.c_str());
  }
  if (answer.refusal.has_value()) {
    std::printf("error: '%s'\n", answer.refusal->c_str());
  }
}

// FR-024, FR-026, FR-027, FR-029, SC-003: one reading stands per `.json`
// entry directly under the record root, in the file-name order of that set,
// each naming the CPU type its own document carries. The stray file and the
// subdirectory answer no reading and no refusal, and a valid reading carries
// no detail.
auto listingAnswersOneReadingPerRecord(const PlantedTree& tree) -> void
{
  const auto amd = unmappedIdent();
  const auto intel = plantedIdent();
  tree.writeRecord(amd.key + ".json", recordBytesFor(amd));
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  plantStrayEntries(tree);
  const auto answer = sg::detail::checkRecords(
      tree.recordRoot(), tree.root(), tree.schemaPath(), matchingChooser());
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "a standing set answers no refusal (FR-029)");
  check(answer.readings.size() == 2U,
        "one reading stands per .json entry and none for the stray file or "
        "the directory (FR-024, FR-029)");
  const bool ordered = answer.readings.size() == 2U
      && answer.readings[0].cpuType == amd.key
      && answer.readings[1].cpuType == intel.key;
  check(ordered,
        "the readings stand in the file-name order of that set (FR-024)");
  const bool valid = answer.readings.size() == 2U
      && answer.readings[0].verdict == sg::detail::RecordVerdict::VALID
      && answer.readings[1].verdict == sg::detail::RecordVerdict::VALID;
  check(valid,
        "a record the guard accepts and the rules leave alone reads valid "
        "(FR-024, FR-027)");
  const bool quiet = answer.readings.size() == 2U
      && answer.readings[0].detail.empty() && answer.readings[1].detail.empty();
  check(quiet, "a valid reading carries no detail (FR-024)");
}

// FR-029, D-10: a record root that stands absent answers neither readings nor
// refusal, and asks the chooser nothing.
auto absentRootAnswersTheEmptyPair(const PlantedTree& tree) -> void
{
  const auto answer = sg::detail::checkRecords(tree.root() / "no-records",
                                               tree.root(),
                                               tree.schemaPath(),
                                               matchingChooser());
  reportReadings(answer);
  check(answer.readings.empty(),
        "a record root that stands absent answers no reading (FR-029)");
  check(!answer.refusal.has_value(),
        "a record root that stands absent answers no refusal (FR-029)");
}

// FR-029, D-10: a record root that holds no `.json` entry answers the same
// empty pair the absent root answers, the one path the listing shares.
auto rootWithoutRecordsAnswersTheSamePair(const PlantedTree& tree) -> void
{
  plantStrayEntries(tree);
  const auto answer = sg::detail::checkRecords(
      tree.recordRoot(), tree.root(), tree.schemaPath(), matchingChooser());
  reportReadings(answer);
  check(answer.readings.empty(),
        "a root that holds no .json entry answers no reading (FR-029)");
  check(!answer.refusal.has_value(),
        "a root that holds no .json entry answers no refusal (FR-029)");
}

// FR-029: a `.json` entry the reader cannot open sets the refusal to itself,
// and the run answers no reading beside it. The entry the reader cannot read
// sorts last of the two, so one reading stands ready before the refusal and the
// empty answer belongs to the run.
auto unreadableEntrySetsTheRefusal(const PlantedTree& tree) -> void
{
  const auto amd = unmappedIdent();
  tree.writeRecord(amd.key + ".json", recordBytesFor(amd));
  const auto blocked = tree.writeRecord(plantedIdent().key + ".json",
                                        recordBytesFor(plantedIdent()));
  tree.blockFile(blocked);
  const auto answer = sg::detail::checkRecords(
      tree.recordRoot(), tree.root(), tree.schemaPath(), matchingChooser());
  reportReadings(answer);
  check(answer.readings.empty(),
        "an entry the reader cannot open ends the run before any reading "
        "(FR-029)");
  check(answer.refusal.has_value(),
        "an entry the reader cannot open sets the refusal (FR-029)");
  check(answer.refusal.value_or(std::string()) == blocked.string(),
        "the refusal names the entry the reader could not open (FR-029)");
}

// ============================================================================
// T024: the one verdict of one entry (specs/031-host-validation; FR-015,
// FR-018, FR-029, FR-030, FR-041, SC-003, SC-006; wave 3, M-C).
//
// The guard settles a document before any staleness rule reads it, so the entry
// that breaks answers `broken` alone and asks the chooser nothing while the
// entry beside it reads its causes (plan R-07, FR-030). Every guard that fails
// answers that one verdict through that one branch, the size cap of FR-018
// among the causes and a schema that cannot be read among the failures, so the
// reader holds no line apart for either (FR-018, FR-020, FR-030). A record
// whose evidence list stands empty is the shape the lane commits its own record
// with, and the four rules answer it nothing (FR-041, SC-006). The arm of a
// `.json` entry the reader cannot open stands from T011, and the line that
// names it stands in the run that prints the other refusals (FR-029). Each tree
// holds the schema copy the reader resolves under the root it is given, and one
// arm plants a directory in place of that copy (FR-020, FR-035).
// ============================================================================

// The size cap of one record of FR-018, stated here so an arm can plant a
// document that passes it: the test holds the size its document must exceed,
// so a constant that went wrong cannot carry its own check. Such a document is
// no record at all, the way a key set that disagrees is none (FR-018, FR-030).
constexpr std::size_t kRecordCapBytes = static_cast<std::size_t>(16U) * 1024U;

// The label a broken line carries before the rule and the key or size it fired
// on, the document kind `contracts/record-check.md` names (FR-019, FR-030).
constexpr std::string_view kBrokenLabel = "host-record: ";

// The version an arm plants for rule 1: one behind the version the format
// speaks, so a record the guard settled names it and a record the guard broke
// names nothing of the kind (FR-019, FR-025, FR-030).
constexpr std::string_view kLaggingVersion = "0.9";

// The chooser of an arm that counts questions, so the arm can state which
// records reach rule 4: the guard settles a break before any question, and one
// settled record asks exactly one (FR-025, plan R-06).
auto countingChooser(int& questions) -> sg::detail::DirectoryChooser
{
  return
      [&questions](const sg::detail::HostIdent&) -> sg::detail::DirectoryChoice
  {
    ++questions;
    return sg::detail::DirectoryChoice {
        .answer = sg::detail::DirectoryAnswer::FOUND,
        .directory = "arch/x86/arrowlake/",
    };
  };
}

// The record of an arm that reads rule 1 through the reader rather than
// through `staleReasons`: the fixture's document with its version pair spelled
// one behind the format (FR-019, FR-025).
auto recordWithLaggingVersion() -> std::string
{
  return withPairRewritten(pinnedRecord(),
                           kVersionPair,
                           R"VERB("recordVersion":")VERB"
                               + std::string(kLaggingVersion) + R"VERB(")VERB");
}

// The record of an arm that reads the size cap: the mask text of the fixture's
// machine block stands a run of digits long enough to pass the cap of FR-018,
// with every key of the document standing (FR-018, FR-030).
auto recordOverTheCap() -> std::string
{
  const auto wide = R"CAP(,"mask":")CAP"
      + std::string(kRecordCapBytes + 8U, '0') + R"CAP(")CAP";
  return withPairRewritten(pinnedRecord(), R"CAP(,"mask":"0-19")CAP", wide);
}

// FR-024, FR-025, FR-030, SC-003: a planted break beside a planted stale cause
// reports the break alone. Both entries lag the version the format speaks, and
// one of them drops a key the schema requires, so the reading of that entry
// names the break and no cause while the reading of the other names the version
// it lags, and the whole run asks rule 4 once.
auto breakBesideAStaleCauseReportsTheBreakAlone(const PlantedTree& tree) -> void
{
  const auto broken =
      withoutPair(recordWithLaggingVersion(), R"BREAK(,"mask":"0-19")BREAK");
  check(!broken.empty(), "the plant found the pair it cuts");
  if (broken.empty()) {
    return;
  }
  const auto amd = unmappedIdent();
  const auto intel = plantedIdent();
  tree.writeRecord(amd.key + ".json", broken);
  tree.writeRecord(intel.key + ".json", recordWithLaggingVersion());
  auto questions = 0;
  const auto answer = sg::detail::checkRecords(tree.recordRoot(),
                                               tree.root(),
                                               tree.schemaPath(),
                                               countingChooser(questions));
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "a standing set of records answers no refusal (FR-029)");
  check(answer.readings.size() == 2U,
        "one reading stands per record of the root (FR-024)");
  if (answer.readings.size() != 2U) {
    return;
  }
  const auto& first = answer.readings[0];
  const auto& second = answer.readings[1];
  check(first.cpuType == amd.key
            && first.verdict == sg::detail::RecordVerdict::BROKEN,
        "the entry whose document breaks reads broken (FR-024, FR-030)");
  check(first.detail.rfind(kBrokenLabel, 0) == 0,
        "the broken detail carries the document kind beside what the guard "
        "reported (FR-019, FR-030)");
  const auto named =
      std::string(kBrokenLabel) + "/machine: the key 'mask' stands missing";
  std::printf(
      "  detail  %s\n  expects %s\n", first.detail.c_str(), named.c_str());
  check(first.detail == named,
        "the detail names the pointer and the key it missed in one sentence, "
        "the shape the loader reads beside a real pointer (FR-019, FR-022, "
        "FR-030)");
  check(first.detail.find(kLaggingVersion) == std::string::npos,
        "a record that breaks reports the break and no staleness (FR-030)");
  check(second.verdict == sg::detail::RecordVerdict::STALE
            && second.detail.find(kLaggingVersion) != std::string::npos,
        "the record the guard settled reads stale with the version it lags "
        "(FR-025, FR-030)");
  check(questions == 1,
        "the staleness rules run over a record the guard settled and over no "
        "other (FR-025, FR-030)");
}

// FR-018, FR-030: a document over the cap of FR-018 answers no record and
// reads broken, naming the size it read, and the run it stands in answers no
// refusal, because the cap is a break and not a path the reader could not
// reach.
auto overCapDocumentReadsBroken(const PlantedTree& tree) -> void
{
  const auto document = recordOverTheCap();
  const bool planted = !document.empty() && document.size() > kRecordCapBytes;
  check(planted, "the planted document passes the cap of FR-018");
  if (!planted) {
    return;
  }
  const auto file =
      tree.writeRecord(std::string(kRecordKey) + ".json", document);
  check(!sg::detail::readHostRecord(file, tree.schemaPath()).has_value(),
        "a document over the cap answers no record (FR-018, FR-030)");
  auto questions = 0;
  const auto answer = sg::detail::checkRecords(tree.recordRoot(),
                                               tree.root(),
                                               tree.schemaPath(),
                                               countingChooser(questions));
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "an over-cap entry breaks and answers no refusal (FR-018, FR-029)");
  check(answer.readings.size() == 1U,
        "the over-cap entry answers one reading (FR-024)");
  if (answer.readings.size() != 1U) {
    return;
  }
  const auto& reading = answer.readings[0];
  check(reading.verdict == sg::detail::RecordVerdict::BROKEN,
        "a document over the cap reads broken (FR-018, FR-030)");
  check(reading.detail.rfind(kBrokenLabel, 0) == 0
            && reading.detail.find(std::to_string(document.size()))
                != std::string::npos,
        "the broken detail names the size the reader counted (FR-018, FR-030)");
  check(questions == 0,
        "a document the cap refuses reaches neither the rules nor rule 4 "
        "(FR-030)");
}

// FR-004, FR-020, FR-030: a schema the reader cannot read is a failed check
// like any other, so every entry of the root reads broken through that one
// branch, the run answers no refusal, and no record reaches the rules.
auto unreadableSchemaBreaksEveryEntry(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  tree.blockFile(tree.schemaPath());
  auto questions = 0;
  const auto answer = sg::detail::checkRecords(tree.recordRoot(),
                                               tree.root(),
                                               tree.schemaPath(),
                                               countingChooser(questions));
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "a schema the reader cannot read answers no refusal here, the entry "
        "itself standing readable (FR-020, FR-029)");
  check(answer.readings.size() == 1U,
        "a readable entry answers one reading whatever the guard read "
        "(FR-024)");
  if (answer.readings.size() != 1U) {
    return;
  }
  check(answer.readings[0].verdict == sg::detail::RecordVerdict::BROKEN
            && answer.readings[0].detail.rfind(kBrokenLabel, 0) == 0,
        "a guard that cannot answer reads broken beside what it reported "
        "(FR-004, FR-020, FR-030)");
  check(questions == 0,
        "a record no guard settled asks rule 4 nothing (FR-030)");
}

// FR-015, FR-025, FR-041, SC-006: a record whose evidence list stands empty,
// the shape the lane commits its own record with, passes the guard, answers no
// stale reason and answers no detail, and the run asks rule 4 once for it over
// a tree that holds the mapfile beside the schema.
auto emptyEvidenceListReadsCurrent(const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  auto questions = 0;
  const auto answer = sg::detail::checkRecords(tree.recordRoot(),
                                               tree.root(),
                                               tree.schemaPath(),
                                               countingChooser(questions));
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "a standing record with nothing to confirm answers no refusal "
        "(FR-029)");
  check(answer.readings.size() == 1U,
        "the record of an empty evidence list answers one reading (FR-024)");
  if (answer.readings.size() != 1U) {
    return;
  }
  check(answer.readings[0].cpuType == intel.key
            && answer.readings[0].verdict == sg::detail::RecordVerdict::VALID
            && answer.readings[0].detail.empty(),
        "a record whose evidence list stands empty reads valid and carries no "
        "detail (FR-024, FR-027, FR-041)");
  check(questions == 1,
        "rule 4 stands asked once for the one record of the root (FR-025)");
}

// ============================================================================
// T026: the reader's own run (specs/031-host-validation; FR-024 to FR-028,
// FR-037, SC-003, SC-006; wave 3, M-D).
//
// `hostRecordCheckMain` owns the parse, the schema under the root that parse
// names, the chooser its factory builds and the mapping from the answer to the
// lines and the status. The arms here drive one run whose every record stands
// valid, the case the quickstart reads over the committed set, one run whose
// mapfile stands unreadable, and the chooser the program hands its factory, so
// the status, the questions and the run refusal stand proved before the lines
// of a refused run stand planted (FR-027). The factory stands planted, so a run
// opens no mapfile, reads no counters table and reads no host, except the arm
// that asks the program's own builder (FR-004, FR-025, FR-026, FR-035).
// ============================================================================

// The words and the `argv` of one reader run over a planted tree: the two
// options the reader takes, each standing beside the root this arm planted, and
// the terminating null the shape the process hands `main` asks for (FR-028,
// FR-037).
class PlantedRun
{
public:
  explicit PlantedRun(const PlantedTree& tree)
      : m_words {"host_record_check",
                 "--root",
                 tree.root().string(),
                 "--records",
                 tree.recordRoot().string()}
  {
    for (auto& word : m_words) {
      m_argv.push_back(word.data());
    }
    m_argv.push_back(nullptr);
  }

  auto argc() const -> int { return static_cast<int>(m_words.size()); }

  auto argv() -> char** { return m_argv.data(); }

private:
  std::vector<std::string> m_words;
  std::vector<char*> m_argv;
};

// The factory of an arm that counts what the run asks: the entry point builds
// its chooser once for the root it read, and every record asks that chooser one
// question (FR-025, plan R-06).
auto countingFactory(int& builds,
                     int& questions) -> sg::detail::DirectoryChooserFactory
{
  return [&builds, &questions](
             const std::filesystem::path&) -> sg::detail::DirectoryChooser
  {
    ++builds;
    return countingChooser(questions);
  };
}

// FR-024, FR-027, FR-037: the reader's run over a record root whose every
// record stands valid answers status 0, prints the one line of that record as
// it goes, and asks its factory for one chooser that answers one question.
auto allValidRecordsAnswerStatusZero(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  auto builds = 0;
  auto questions = 0;
  auto run = PlantedRun(tree);
  const auto status = sg::detail::hostRecordCheckMain(
      run.argc(), run.argv(), countingFactory(builds, questions));
  std::printf(
      "status %d, %d chooser built, %d questions\n", status, builds, questions);
  check(status == 0,
        "a run over records that all stand valid answers status 0 (FR-027)");
  check(builds == 1,
        "the entry point builds the chooser once for the root its parse read "
        "(plan R-06)");
  check(questions == 1, "the one record of the root asks rule 4 once (FR-025)");
}

// FR-024, FR-027, FR-030: the run over a set that carries a break and a cause
// answers status 2 and prints one line for each, the verdict column carrying
// the word of that verdict and the detail the rule left behind. Two records
// stand there and the run asks rule 4 once, because the guard settles the break
// before any question.
auto breakAndCauseAnswerStatusTwo(const PlantedTree& tree) -> void
{
  const auto broken = withoutPair(pinnedRecord(), R"RUN(,"mask":"0-19")RUN");
  check(!broken.empty(), "the plant found the pair it cuts");
  if (broken.empty()) {
    return;
  }
  const auto amd = unmappedIdent();
  tree.writeRecord(amd.key + ".json", broken);
  tree.writeRecord(plantedIdent().key + ".json", recordWithLaggingVersion());
  auto builds = 0;
  auto questions = 0;
  auto run = PlantedRun(tree);
  const auto status = sg::detail::hostRecordCheckMain(
      run.argc(), run.argv(), countingFactory(builds, questions));
  std::printf(
      "status %d, %d chooser built, %d questions\n", status, builds, questions);
  check(status == 2,
        "a set that carries a break or a cause answers status 2 (FR-027)");
  check(builds == 1 && questions == 1,
        "the two records ask rule 4 once between them, the broken one asking "
        "nothing (FR-025, FR-030)");
}

// ---------------------------------------------------------------------------
// T040: every line a refused run answers, read back from the two streams the
// entry point writes. The fence of `contracts/record-check.md` spells three
// `error:` sentences naming the schema path, the refused path and the record
// root, each alone on standard error with status 2, and the per-record line
// three columns wide where no detail stands and four where one does. The
// argument arms stand planted for the shape the parse owes, one line naming the
// word it read, before any chooser stands built: the words themselves stand
// with T042 (FR-004, FR-020, FR-025, FR-029, SC-004).
// ---------------------------------------------------------------------------

// The file one captured stream writes, named with the pid and a sequence so
// parallel runs stand apart, the shape `harness_cli_interleave_test.cpp:431`
// gives its capture paths.
auto capturePath(const char* kind) -> std::filesystem::path
{
  static int sequence = 0;
  return std::filesystem::temp_directory_path()
      / ("harness_host_record_check-" + std::to_string(::getpid()) + "-"
         + std::to_string(sequence++) + kind);
}

// The lines one stream printed, the trailing newline of the last line left off
// so an empty stream reads as no line.
auto linesOf(const std::string& text) -> std::vector<std::string>
{
  auto lines = std::vector<std::string> {};
  auto line = std::string {};
  for (const char current : text) {
    if (current == '\n') {
      lines.push_back(line);
      line.clear();
      continue;
    }
    line += current;
  }
  if (!line.empty()) {
    lines.push_back(line);
  }
  return lines;
}

// What one run of the entry point answered and printed on the two streams.
struct CapturedRun
{
  int status {};
  std::vector<std::string> out;
  std::vector<std::string> err;
};

// One run of the entry point with both streams turned into files: the words the
// process hands `main`, the factory that builds its chooser, and the answer
// read back. The turn stands and falls around the one call, so a later arm
// still prints its report (FR-027, FR-029).
auto runCaptured(const std::vector<std::string>& words,
                 const sg::detail::DirectoryChooserFactory& factory)
    -> CapturedRun
{
  auto argv = std::vector<char*> {const_cast<char*>("host_record_check")};
  for (const auto& word : words) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  argv.push_back(nullptr);
  const auto outPath = capturePath(".out");
  const auto errPath = capturePath(".err");
  const auto savedOut = ::dup(STDOUT_FILENO);
  const auto savedErr = ::dup(STDERR_FILENO);
  if (savedOut < 0 || savedErr < 0
      || std::freopen(outPath.c_str(), "w", stdout) == nullptr
      || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
  {
    sg::test::fail("the two streams of the entry point can be turned aside");
    return {};
  }
  auto answer = CapturedRun {};
  answer.status = sg::detail::hostRecordCheckMain(
      static_cast<int>(argv.size()) - 1, argv.data(), factory);
  std::fflush(stdout);
  std::fflush(stderr);
  if (::dup2(savedOut, STDOUT_FILENO) < 0
      || ::dup2(savedErr, STDERR_FILENO) < 0)
  {
    sg::test::fail("the two streams of the entry point can be turned back");
  }
  ::close(savedOut);
  ::close(savedErr);
  answer.out = linesOf(sg::test::readText(outPath.string()));
  answer.err = linesOf(sg::test::readText(errPath.string()));
  std::filesystem::remove(outPath);
  std::filesystem::remove(errPath);
  return answer;
}

// The two roots of a planted tree, the words every arm hands the parse (FR-028,
// FR-037).
auto rootsOf(const PlantedTree& tree) -> std::vector<std::string>
{
  return {
      "--root", tree.root().string(), "--records", tree.recordRoot().string()};
}

// The quotes a refusal carries, spelled here so an arm reads the quotes the
// line carries beside the path, the same separation the planted cap keeps
// (FR-004).
auto quotedFor(const std::string& path) -> std::string
{
  return "'" + path + "'";
}

// The line of one reading: the record under the root the run named, the CPU
// type, the verdict word, and the detail where one stands (FR-024, FR-030).
auto plantedLine(const PlantedTree& tree,
                 const std::string& key,
                 const std::string& verdict) -> std::string
{
  return (tree.recordRoot() / std::string(key + ".json")).string() + "  " + key
      + "  " + verdict;
}

// One refusal read back: status 2, no line of the record stream, and one line
// of the error stream that is the sentence the fence spells (FR-004, FR-029).
auto checkRefusalShape(const CapturedRun& run,
                       const std::string& sentence,
                       const std::string& whose) -> void
{
  std::printf("%s expects  %s\n", whose.c_str(), sentence.c_str());
  for (const auto& line : run.out) {
    std::printf("  printed  %s\n", line.c_str());
  }
  for (const auto& line : run.err) {
    std::printf("  refused  %s\n", line.c_str());
  }
  const auto oneLine = run.err.size() == 1U;
  check(run.status == 2,
        (whose + " answers status 2 (FR-004, FR-029)").c_str());
  check(run.out.empty(),
        (whose + " prints no per-record line (FR-029)").c_str());
  check(oneLine,
        (whose + " prints one line on standard error (FR-004)").c_str());
  check(oneLine && run.err[0] == sentence,
        (whose + " prints the sentence the fence spells (FR-004)").c_str());
}

// One unusable argument read back: status 2, no line of the record stream, and
// one line of the error stream naming the word the parse read. The words of
// that line stand with T042 (FR-004, FR-028).
auto checkArgumentRefusal(const CapturedRun& run,
                          const std::string& word,
                          const std::string& whose) -> void
{
  std::printf("%s names  %s\n", whose.c_str(), word.c_str());
  for (const auto& line : run.err) {
    std::printf("  refused  %s\n", line.c_str());
  }
  const auto oneLine = run.err.size() == 1U;
  check(run.status == 2,
        (whose + " answers status 2 (FR-004, SC-004)").c_str());
  check(run.out.empty(),
        (whose + " reads no record and prints no line (FR-004)").c_str());
  check(oneLine,
        (whose + " prints one line on standard error (FR-004)").c_str());
  check(oneLine && run.err[0].find(quotedFor(word)) != std::string::npos,
        (whose + " names the word the parse read (FR-004)").c_str());
}

// The chooser of an arm whose tree holds no readable mapfile: every identity
// answers the answer that names the mapfile itself, the third shape
// `contracts/record-check.md` gives the chooser, which no record turns into a
// cause (FR-004, FR-025).
auto refusalChooser(int& questions) -> sg::detail::DirectoryChooser
{
  return
      [&questions](const sg::detail::HostIdent&) -> sg::detail::DirectoryChoice
  {
    ++questions;
    return sg::detail::DirectoryChoice {
        .answer = sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
        .directory = {},
    };
  };
}

// The factory of that arm, the seam T026 opens: the entry point builds its
// chooser from here, so the refusal of a mapfile the library never opens stands
// asserted as an answer (FR-025, plan R-04, R-06).
auto refusalFactory(int& builds,
                    int& questions) -> sg::detail::DirectoryChooserFactory
{
  return [&builds, &questions](
             const std::filesystem::path&) -> sg::detail::DirectoryChooser
  {
    ++builds;
    return refusalChooser(questions);
  };
}

// FR-004, FR-025, FR-029: the answer that names the mapfile ends the run. One
// record stands valid under the root and the mapfile of the tree stands
// unreadable, so `checkRecords` sets its refusal to that mapfile and leaves the
// readings empty, and the entry point answers status 2 with no per-record line
// before it. The line itself stands planted in T040.
auto unreadableMapfileRefusesTheRun(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  auto builds = 0;
  auto entryQuestions = 0;
  auto run = PlantedRun(tree);
  const auto status = sg::detail::hostRecordCheckMain(
      run.argc(), run.argv(), refusalFactory(builds, entryQuestions));
  std::printf("status %d, %d chooser built, %d questions\n",
              status,
              builds,
              entryQuestions);
  check(status == 2,
        "a run whose mapfile stands unreadable answers status 2 (FR-004)");
  check(builds == 1 && entryQuestions == 1,
        "the first question ends the run, so one chooser answers once (plan "
        "R-06)");
  auto checkQuestions = 0;
  const auto answer = sg::detail::checkRecords(tree.recordRoot(),
                                               tree.root(),
                                               tree.schemaPath(),
                                               refusalChooser(checkQuestions));
  reportReadings(answer);
  check(answer.readings.empty(),
        "the record that stands valid reads no line, rule 4 standing "
        "unanswered " "(FR-025, FR-029)");
  const auto mapfile =
      tree.root() / std::filesystem::path(std::string(kMapfileRelPath));
  const bool namesMapfile =
      answer.refusal.has_value() && *answer.refusal == mapfile.string();
  check(namesMapfile,
        "the refusal names the mapfile the refusal sentence quotes (FR-004)");
}

// FR-004, FR-008, FR-025: the chooser the program\'s factory builds answers the
// mapfile it cannot read. A path the reader cannot read and a tree that holds
// no such file answer the same way, twice on the one open that stands behind
// the run, and neither answer carries a directory (plan R-06).
auto programChooserAnswersTheMapfileRefusal(const PlantedTree& tree) -> void
{
  const auto blocked =
      tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  tree.blockFile(blocked);
  const auto chooser = sg::tools::makeDirectoryChooser(tree.root());
  const auto first = chooser(plantedIdent());
  const auto second = chooser(unmappedIdent());
  const auto absentChooser =
      sg::tools::makeDirectoryChooser(tree.root() / "holds-no-tree");
  const auto absent = absentChooser(plantedIdent());
  std::printf("program chooser answers %d and %d, directory '%s'\n",
              static_cast<int>(first.answer),
              static_cast<int>(second.answer),
              first.directory.c_str());
  check(first.answer == sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
        "the chooser of a mapfile it cannot open answers the run refusal "
        "(FR-004, FR-025)");
  check(first.directory.empty(),
        "the answer that names the mapfile carries no directory (FR-008)");
  check(second.answer == sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
        "the second question reads the same answer behind the one open "
        "(plan R-06)");
  check(absent.answer == sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
        "a tree that holds no mapfile answers the same refusal (FR-004)");
}

// FR-004, FR-025, FR-029: a tree whose record stands and whose mapfile stands
// unreadable answers the run sentence naming that mapfile, alone, with status 2
// and no line of the readings the listing had already gathered.
auto mapfileRefusalPrintsItsLine(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), refusalFactory(builds, questions));
  const auto mapfile =
      tree.root() / std::filesystem::path(std::string(kMapfileRelPath));
  checkRefusalShape(
      run,
      "error: the record run cannot read " + quotedFor(mapfile.string()),
      "the mapfile refusal");
  check(builds == 1 && questions == 1,
        "the mapfile refusal answers at the first question of the run "
        "(plan R-06)");
}

// FR-004, FR-020: a tree that holds no schema answers the schema sentence and
// nothing else, the factory standing unbuilt because the entry point refuses
// before it calls anything.
auto absentSchemaPrintsItsLine(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  tree.dropTreeFile(tree.root()
                    / std::filesystem::path(std::string(kSchemaRelPath)));
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), countingFactory(builds, questions));
  const auto schema =
      tree.root() / std::filesystem::path(std::string(kSchemaRelPath));
  checkRefusalShape(run,
                    "error: the record schema " + quotedFor(schema.string())
                        + " cannot be read",
                    "the schema refusal");
  check(builds == 0,
        "a tree with no schema refuses before its factory stands asked "
        "(FR-004, FR-020)");
}

// FR-004, FR-020, FR-025: a tree that holds neither the schema nor the mapfile
// answers the schema refusal, the case that settles which of the two paths the
// run names (FR-020, FR-004).
auto treeWithNeitherFilePrintsTheSchemaLine(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  tree.dropTreeFile(tree.root()
                    / std::filesystem::path(std::string(kSchemaRelPath)));
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), countingFactory(builds, questions));
  const auto schema =
      tree.root() / std::filesystem::path(std::string(kSchemaRelPath));
  checkRefusalShape(run,
                    "error: the record schema " + quotedFor(schema.string())
                        + " cannot be read",
                    "the tie-break of a tree with neither file");
  check(builds == 0,
        "the tie-break leaves the mapfile question unasked (FR-004, FR-025)");
}

// FR-029, D-10: a record root that stands absent answers the root sentence with
// status 2, and the chooser that stands built for the run opens nothing.
auto absentRecordRootPrintsItsLine(const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), countingFactory(builds, questions));
  checkRefusalShape(run,
                    "error: the record root "
                        + quotedFor(tree.recordRoot().string())
                        + " holds no record",
                    "an absent record root");
  check(builds == 1 && questions == 0,
        "a run with nothing to read asks nothing and opens no mapfile "
        "(plan R-06)");
}

// FR-029, D-10: a record root that stands with no `.json` entry in it shares
// that sentence and that status with the root that stands absent.
auto rootWithNoRecordPrintsItsLine(const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  tree.writeRecord(kStrayName, "A note the reader passes over.\n");
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), countingFactory(builds, questions));
  checkRefusalShape(run,
                    "error: the record root "
                        + quotedFor(tree.recordRoot().string())
                        + " holds no record",
                    "a record root that holds no record");
  check(builds == 1 && questions == 0,
        "the stray entry asks nothing and opens no mapfile (plan R-06)");
}

// FR-029, FR-004: one `.json` entry the reader cannot open ends the run with
// the sentence naming that entry, before the chooser stands asked, so the
// mapfile of the tree stands unopened beside it.
auto unopenableEntryPrintsItsLine(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  const auto entry =
      tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  tree.blockFile(entry);
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), countingFactory(builds, questions));
  checkRefusalShape(
      run,
      "error: the record run cannot read " + quotedFor(entry.string()),
      "an entry the reader cannot open");
  check(builds == 1 && questions == 0,
        "the entry that refuses ends the run before rule 4 stands asked "
        "(FR-025, plan R-06)");
}

// FR-004, FR-028, SC-004: three unusable words, each standing ahead of the
// factory and ahead of any record. The words of the line the parse owes stand
// with T042, so each arm reads one error line naming the word it handed over.
auto unusableArgumentsRefuseTheRun(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  const auto words = rootsOf(tree);
  auto builds = 0;
  auto questions = 0;
  const auto unknown =
      runCaptured({words[0], words[1], words[2], words[3], "--force"},
                  countingFactory(builds, questions));
  checkArgumentRefusal(unknown, "--force", "an unknown option");
  const auto missing = runCaptured({words[0], words[1], words[2]},
                                   countingFactory(builds, questions));
  checkArgumentRefusal(missing, words[2], "an option with no value");
  const auto extra =
      runCaptured({words[0], words[1], words[2], words[3], "extra"},
                  countingFactory(builds, questions));
  checkArgumentRefusal(extra, "extra", "an extra argument");
  check(builds == 0,
        "no unusable word reaches the factory of the run (FR-004, FR-028)");
}

// FR-024, FR-027, FR-030: the lines of a run that reads records carry the
// columns the fence spells, in the file-name order of the set: three columns
// where no detail stands, and the fourth carrying the label beside the break
// that fired.
auto printedLinesCarryTheContractColumns(const PlantedTree& tree) -> void
{
  const auto broken = withoutPair(pinnedRecord(), R"COL(,"mask":"0-19")COL");
  check(!broken.empty(), "the plant found the pair it cuts");
  if (broken.empty()) {
    return;
  }
  const auto amd = unmappedIdent();
  tree.writeRecord(amd.key + ".json", broken);
  tree.writeRecord(plantedIdent().key + ".json",
                   recordBytesFor(plantedIdent()));
  auto builds = 0;
  auto questions = 0;
  const auto run =
      runCaptured(rootsOf(tree), countingFactory(builds, questions));
  for (const auto& line : run.out) {
    std::printf("  printed  %s\n", line.c_str());
  }
  check(run.status == 2,
        "a set that carries a break answers status 2 (FR-027)");
  check(run.out.size() == 2U && run.err.empty(),
        "the two records print two lines and no refusal (FR-024, FR-029)");
  const auto brokenLine =
      plantedLine(tree, amd.key, "broken") + "  " + std::string(kBrokenLabel);
  const auto validLine = plantedLine(tree, plantedIdent().key, "valid");
  const bool shapes = run.out.size() == 2U
      && run.out[0].rfind(brokenLine, 0) == 0 && run.out[1] == validLine;
  check(shapes,
        "the broken line opens with the three columns, the label and the key "
        "it fired on, and the valid line stands three columns wide (FR-024, "
        "FR-030)");
  check(run.out.size() == 2U && !run.out[0].empty()
            && run.out[0].size() > brokenLine.size(),
        "the fourth column of the broken line names what the guard reported "
        "(FR-022, FR-030)");
  check(questions == 1,
        "the record the guard settled asks rule 4 once (FR-025, FR-030)");
}

// FR-015, FR-025, SC-003: three rows of the results-table shape travel through
// one write of the document and one read of it. Each row keeps its path, its
// command, its exit status and its cited line in the order the list stands, the
// set answers no stale reason while every cited file stands with its line, and
// the reader prints one valid line for the record that carries them.
auto threeRowsTravelThroughOneWriteAndOneRead(const PlantedTree& tree) -> void
{
  const auto rows = plantThreeRows(tree);
  const auto document = recordWithRows(rows);
  check(!document.empty(), "the plant carries the list of three rows");
  if (document.empty()) {
    return;
  }
  const auto intel = plantedIdent();
  const auto file = tree.writeRecord(intel.key + ".json", document);
  const auto record = sg::detail::readHostRecord(file, tree.schemaPath());
  check(record.has_value(),
        "the document whose list carries three rows passes the guard and reads "
        "a record (FR-020, FR-021)");
  if (!record.has_value()) {
    return;
  }
  auto same = record->evidence.size() == rows.size();
  for (std::size_t at = 0; same && at < rows.size(); ++at) {
    same = record->evidence[at].path == rows[at].path
        && record->evidence[at].command == rows[at].command
        && record->evidence[at].exit == rows[at].exit
        && record->evidence[at].line == rows[at].line;
  }
  check(same,
        "each row reads back with its path, its command, its exit status and "
        "its cited line, in the order the written list stands (FR-015)");
  const auto reasons =
      sg::detail::staleReasons(*record, tree.root(), matchingChooser());
  reportStale(*record, reasons);
  check(reasons.empty(),
        "three standing rows answer no stale reason (FR-025, SC-003)");
  const auto answer = sg::detail::checkRecords(
      tree.recordRoot(), tree.root(), tree.schemaPath(), matchingChooser());
  check(answer.readings.size() == 1U && !answer.refusal.has_value()
            && answer.readings[0].verdict == sg::detail::RecordVerdict::VALID,
        "the reader answers one valid reading for the record of three rows "
        "(FR-024, FR-027)");
}

// FR-015, FR-041, SC-006: the record the lane commits reads valid. The planted
// mirror of `validation/GenuineIntel-6-C6-2.json` stands with its evidence list
// empty over a root that holds the schema copy the guard reads and the mapfile
// copy the chooser of the program reads, so the four rules run over the bytes
// the lane writes and answer no stale reason and no refusal.
auto committedRecordMirrorReadsValid(const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  const auto intel = plantedIdent();
  const auto file =
      tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  const auto mirror = sg::detail::readHostRecord(file, tree.schemaPath());
  check(mirror.has_value(),
        "the mirror of the record the lane commits passes the pinned guard "
        "(FR-020, FR-021)");
  check(mirror.has_value() && mirror->cpu.key == intel.key
            && mirror->evidence.empty(),
        "the mirror carries the committed key with its evidence list empty "
        "(FR-016, FR-041)");
  if (!mirror.has_value()) {
    return;
  }
  const auto chooser = sg::tools::makeDirectoryChooser(tree.root());
  const auto reasons = sg::detail::staleReasons(*mirror, tree.root(), chooser);
  reportStale(*mirror, reasons);
  check(reasons.empty(),
        "an empty list answers no stale reason across the four rules, the "
        "mapfile of the tree holding a row for the committed identity "
        "(FR-025, FR-041, SC-006)");
  const auto answer = sg::detail::checkRecords(
      tree.recordRoot(), tree.root(), tree.schemaPath(), chooser);
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "a root holding the mirror, its schema and its mapfile answers no "
        "refusal (FR-029)");
  check(answer.readings.size() == 1U
            && answer.readings[0].verdict == sg::detail::RecordVerdict::VALID
            && answer.readings[0].detail.empty(),
        "the reader answers one valid line for the mirror with nothing beside "
        "it (FR-024, FR-027, SC-006)");
}

// FR-008, FR-025: the chooser the program builds over a planted tree answers
// rule 4 from the rows it reads. A row the identity matches names a directory,
// and a row set that holds no row for it answers `NO_ROW` with no directory
// beside it, the answer the staleness rules turn into one cause naming the
// record key. No arm plants a `DirectoryAnswer` here: the mapfile of the tree
// answers (FR-028, plan R-06).
auto chooserOfTheTreeAnswersNoRowForAnIdentityTheRowsMiss(
    const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath), plantedMapfile());
  const auto chooser = sg::tools::makeDirectoryChooser(tree.root());
  const auto matched = chooser(plantedIdent());
  const auto missed = chooser(unmappedIdent());
  std::printf("  tree chooser answers %d with '%s' and %d with '%s'\n",
              static_cast<int>(matched.answer),
              matched.directory.c_str(),
              static_cast<int>(missed.answer),
              missed.directory.c_str());
  check(matched.answer == sg::detail::DirectoryAnswer::FOUND
            && !matched.directory.empty(),
        "the row the identity matches names a directory (FR-008, FR-028)");
  check(missed.answer == sg::detail::DirectoryAnswer::NO_ROW
            && missed.directory.empty(),
        "the row set that holds no row for an identity answers it with no "
        "directory (FR-025, FR-028)");
  const auto amd = unmappedIdent();
  const auto file = tree.writeRecord(amd.key + ".json", recordBytesFor(amd));
  const auto record = sg::detail::readHostRecord(file, tree.schemaPath());
  check(record.has_value(),
        "the record of the identity the rows miss reads a record (FR-020)");
  if (!record.has_value()) {
    return;
  }
  const auto reasons = sg::detail::staleReasons(*record, tree.root(), chooser);
  reportStale(*record, reasons);
  const bool namesKey =
      reasons.size() == 1U && reasons[0].find(amd.key) != std::string::npos;
  check(namesKey,
        "the identity the rows miss answers one cause that names the record "
        "key (FR-025, FR-028)");
}

// FR-004, FR-025: a row whose pattern the matcher cannot compile makes the
// rows unreadable to the whole run. The chooser of the tree answers
// `UNREADABLE_MAPFILE` for every identity it is asked about, and the reader
// ends on the refusal that names that mapfile with no reading beside it.
auto chooserOfTheTreeRefusesARowWhosePatternWillNotCompile(
    const PlantedTree& tree) -> void
{
  tree.writeTreeFile(std::string(kMapfileRelPath),
                     "Family-model,Version,Filename,EventType\n"
                     "GenuineIntel-6-[C6,v1.17,arrowlake,core\n");
  const auto chooser = sg::tools::makeDirectoryChooser(tree.root());
  const auto first = chooser(plantedIdent());
  const auto second = chooser(unmappedIdent());
  std::printf("  broken row answers %d and %d\n",
              static_cast<int>(first.answer),
              static_cast<int>(second.answer));
  check(first.answer == sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE
            && second.answer == sg::detail::DirectoryAnswer::UNREADABLE_MAPFILE,
        "a row whose pattern will not compile leaves rule 4 unanswered for "
        "every identity of the run (FR-004, FR-025)");
  const auto intel = plantedIdent();
  tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  const auto answer = sg::detail::checkRecords(
      tree.recordRoot(), tree.root(), tree.schemaPath(), chooser);
  reportReadings(answer);
  const bool namesMapfile = answer.refusal.has_value()
      && answer.refusal->find("mapfile.csv") != std::string::npos;
  check(answer.readings.empty() && namesMapfile,
        "the run over that tree names the mapfile in its refusal and answers "
        "no reading (FR-004, FR-029)");
}

// ============================================================================
// The answer of one read: a key the document holds no value for
// (specs/031-host-validation; FR-021, FR-022, FR-030).
//
// The pinned schema settles the key set ahead of every read, so no document the
// lane commits reaches these answers. An open schema stands in its place: it
// names no key and states no kind, so any document passes it and each read
// answers for itself. That is the reach a caller has when it hands the reader a
// schema of its own, and it is the answer the ignore semantic of
// `speedgun-ng_CONTRACTS` owes, where no contract line stands to answer a key
// the document holds no value for (FR-021, FR-022).
// ============================================================================

// One reading per planted entry, in the order their names sort: each answers
// broken, each detail carries the `host-record` label and names the key that
// document holds no value for, the run answers no refusal, and rule 4 stands
// asked nothing (FR-021, FR-024, FR-030).
auto checkOpenSchemaReadings(const PlantedTree& tree,
                             const std::vector<std::string>& names,
                             const std::vector<std::string>& documents,
                             const std::vector<std::string>& keys) -> void
{
  for (std::size_t at = 0; at < names.size(); ++at) {
    tree.writeRecord(names[at], documents[at]);
  }
  auto questions = 0;
  const auto answer = sg::detail::checkRecords(tree.recordRoot(),
                                               tree.root(),
                                               tree.openSchemaPath(),
                                               countingChooser(questions));
  reportReadings(answer);
  check(!answer.refusal.has_value(),
        "a key the document holds no value for answers no refusal of the run "
        "(FR-029)");
  check(answer.readings.size() == names.size(),
        "each entry the open schema settled answers one reading (FR-024)");
  for (std::size_t at = 0; at < answer.readings.size() && at < keys.size();
       ++at)
  {
    const auto& reading = answer.readings[at];
    const bool namesTheKey =
        reading.verdict == sg::detail::RecordVerdict::BROKEN
        && reading.cpuType == entryName(names[at])
        && reading.detail.rfind(std::string(kBrokenLabel), 0) == 0
        && reading.detail.find("'" + keys[at] + "'") != std::string::npos;
    std::printf(
        "  %s names the key %s\n", reading.detail.c_str(), keys[at].c_str());
    check(namesTheKey,
          "the broken reading names the key the document holds no value for "
          "(FR-021, FR-030)");
  }
  check(questions == 0,
        "a record the reader cannot build asks rule 4 nothing (FR-030)");
}

// FR-021, FR-030: a document that holds none of the four keys the record needs
// most answers broken naming each of them, the ignore semantic included, where
// no contract line stands between the read and the answer.
auto aKeyTheDocumentOmitsReadsBrokenNamingIt(const PlantedTree& tree) -> void
{
  const auto record = pinnedRecord();
  const auto comma = std::string(",");
  const auto documents = std::vector<std::string> {
      withoutPair(record, comma + "\"cores\":20"),
      withoutPair(record, comma + std::string(kCpuPair)),
      withoutPair(record, comma + std::string(kEvidencePair)),
      withoutPair(record, comma + std::string(kMachinePair)),
      withoutPair(record, comma + "\"mask\":\"0-19\""),
      withoutPair(record, comma + "\"stepping\":2"),
  };
  checkOpenSchemaReadings(
      tree,
      {"no-cores.json",
       "no-cpu.json",
       "no-evidence.json",
       "no-machine.json",
       "no-mask.json",
       "no-stepping.json"},
      documents,
      {"cores", "cpu", "evidence", "machine", "mask", "stepping"});
}

// FR-007, FR-012, FR-013, FR-015, FR-021: a value of another kind answers the
// same broken line naming its own key, at the two keys the nullable read hands
// on and at the two row lists.
auto aValueOfAnotherKindReadsBrokenNamingIt(const PlantedTree& tree) -> void
{
  const auto record = pinnedRecord();
  const auto l3Pair = std::string {
      "\"l3\":[{\"ordinal\":0,\"cpus\":\"0-7\"},"
      "{\"ordinal\":1,\"cpus\":\"8-19\"}]"};
  const auto nodesPair =
      std::string {"\"nodes\":[{\"number\":0,\"cpus\":\"0-19\"}]"};
  const auto documents = std::vector<std::string> {
      withPairRewritten(record, l3Pair, "\"l3\":{}"),
      withPairRewritten(record, nodesPair, "\"nodes\":\"0-19\""),
      withPairRewritten(record, "\"exit\":0", "\"exit\":\"0\""),
      withPairRewritten(record, "\"stepping\":2", "\"stepping\":\"2\""),
  };
  checkOpenSchemaReadings(tree,
                          {"l3-object.json",
                           "nodes-text.json",
                           "row-exit-text.json",
                           "stepping-text.json"},
                          documents,
                          {"l3", "machine.nodes", "exit", "cpu.stepping"});
}

// FR-029: a file the reader cannot read answers no record and answers it
// itself, so no caller owes a read ahead of the reader and no mode of the file
// enters the answer.
auto anUnreadableRecordAnswersNoRecord(const PlantedTree& tree) -> void
{
  const auto intel = plantedIdent();
  const auto entry =
      tree.writeRecord(intel.key + ".json", recordBytesFor(intel));
  tree.blockFile(entry);
  const auto record = sg::detail::readHostRecord(entry, tree.schemaPath());
  std::printf("  an unreadable record answers a record: %d\n",
              int(record.has_value()));
  check(!record.has_value(),
        "a file the reader cannot read answers no record (FR-029)");
}

}  // namespace

auto main() -> int
{
  const PlantedTree validTree("valid");
  plantedValidBytesAnswerTheirValues(validTree);

  const PlantedTree nullTree("nulls");
  theUnansweredValuesReadNull(nullTree);

  eachPlantedBreakAnswersNoRecord();

  const PlantedTree versionTree("version");
  versionBehindTheFormatNamesBothVersions(versionTree);

  const PlantedTree absentTree("absent-path");
  absentCitedPathNamesItself(absentTree);

  const PlantedTree standingTree("standing-rows");
  everyStandingRowReadsCurrent(standingTree);

  const PlantedTree deletedTree("deleted-row");
  deletedCitedFileNamesItsRow(deletedTree);

  const PlantedTree lineTree("absent-line");
  citedLineAbsentFromAStandingFileNamesIt(lineTree);

  const PlantedTree noRowTree("no-row");
  noRowForTheIdentityNamesTheKeyAndTheIdent(noRowTree);

  const PlantedTree matchedTree("matched-row");
  rowForTheIdentityAnswersNoCause(matchedTree);

  const PlantedTree listingTree("listing");
  listingAnswersOneReadingPerRecord(listingTree);

  const PlantedTree absentRootTree("absent-root");
  absentRootAnswersTheEmptyPair(absentRootTree);

  const PlantedTree bareRootTree("bare-root");
  rootWithoutRecordsAnswersTheSamePair(bareRootTree);

  const PlantedTree blockedTree("blocked-entry");
  unreadableEntrySetsTheRefusal(blockedTree);

  const PlantedTree verdictTree("break-and-stale");
  breakBesideAStaleCauseReportsTheBreakAlone(verdictTree);

  const PlantedTree capTree("over-cap");
  overCapDocumentReadsBroken(capTree);

  const PlantedTree blockedSchemaTree("blocked-schema");
  unreadableSchemaBreaksEveryEntry(blockedSchemaTree);

  const PlantedTree emptyListTree("empty-evidence");
  emptyEvidenceListReadsCurrent(emptyListTree);

  const PlantedTree validRunTree("valid-run");
  allValidRecordsAnswerStatusZero(validRunTree);

  const PlantedTree mixedRunTree("mixed-run");
  breakAndCauseAnswerStatusTwo(mixedRunTree);

  const PlantedTree mapfileTree("unreadable-mapfile");
  unreadableMapfileRefusesTheRun(mapfileTree);

  const PlantedTree chooserTree("chooser-refusal");
  programChooserAnswersTheMapfileRefusal(chooserTree);

  const PlantedTree mapfileLineTree("mapfile-line");
  mapfileRefusalPrintsItsLine(mapfileLineTree);

  const PlantedTree noSchemaTree("schema-line");
  absentSchemaPrintsItsLine(noSchemaTree);

  const PlantedTree neitherTree("neither-file");
  treeWithNeitherFilePrintsTheSchemaLine(neitherTree);

  const PlantedTree absentRootLineTree("absent-root-line");
  absentRecordRootPrintsItsLine(absentRootLineTree);

  const PlantedTree bareRootLineTree("bare-root-line");
  rootWithNoRecordPrintsItsLine(bareRootLineTree);

  const PlantedTree blockedEntryTree("blocked-entry-line");
  unopenableEntryPrintsItsLine(blockedEntryTree);

  const PlantedTree argumentTree("arguments");
  unusableArgumentsRefuseTheRun(argumentTree);

  const PlantedTree columnTree("columns");
  printedLinesCarryTheContractColumns(columnTree);

  const PlantedTree rowsTree("evidence-rows");
  threeRowsTravelThroughOneWriteAndOneRead(rowsTree);

  const PlantedTree mirrorTree("committed-mirror");
  committedRecordMirrorReadsValid(mirrorTree);

  const PlantedTree chooserNoRow("tree-chooser-no-row");
  chooserOfTheTreeAnswersNoRowForAnIdentityTheRowsMiss(chooserNoRow);

  const PlantedTree chooserBrokenRow("tree-chooser-broken-row");
  chooserOfTheTreeRefusesARowWhosePatternWillNotCompile(chooserBrokenRow);

  const PlantedTree omittedTree("omitted-key");
  aKeyTheDocumentOmitsReadsBrokenNamingIt(omittedTree);

  const PlantedTree kindTree("another-kind");
  aValueOfAnotherKindReadsBrokenNamingIt(kindTree);

  const PlantedTree unreadableTree("unreadable-record");
  anUnreadableRecordAnswersNoRecord(unreadableTree);

  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
