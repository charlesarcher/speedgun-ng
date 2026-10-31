// ============================================================================
// specs/031-host-validation T009: the record one file holds (FR-020, FR-022,
// FR-024, FR-030, R-03).
//
// `readHostRecord` is the only reader of JSON this feature has, and it reads
// in two steps. The guard stands first: `sg::detail::checkReport` compares the
// bytes against the pinned schema at the path the caller resolved, and the cap
// of FR-018 counts them beside that comparison, so only a document that stands
// both reaches the extraction and the guard settles the key set, the key order,
// the kinds and the size before a key is read (FR-018, FR-020 to FR-022,
// FR-030).
//
// The extraction answers a key the document holds no value for, and names that
// key in the reason the entry reads broken with. It keeps no contract for that
// answer, because a contract would vanish where the reader still has to answer:
// `speedgun-ng_CONTRACTS=ignore` expands SG_ASSERT to nothing, and a record
// that holds no mask must then answer the broken line of FR-030 naming that
// mask: the answer belongs to the read, and no contract line carries it there
// (FR-021, FR-030). The guard
// still stands first, so no document the pinned schema settles reaches that
// answer; a caller that hands the reader another schema reaches it, which is
// how the unit plants it. The branches a planted record drives stay the two
// `null` keys and the three row lists (FR-007, FR-013).
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/host_record_check.hpp"

#include <simdjson.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

using Element = simdjson::dom::element;
using JsonArray = simdjson::dom::array;

// The words one key of a document answers when it holds no value the reader
// needs. They carry the parser's own reason, and the reader puts them beside
// the `host-record` label the guard already uses, so a record missing a key
// reads broken in the same shape however it came to be broken (FR-021, FR-030).
auto missingKey(std::string_view key, simdjson::error_code code) -> std::string
{
  return "the key '" + std::string(key)
      + "' answers nothing the record needs: " + simdjson::error_message(code);
}

// Every read of a guarded document. A read answers the value the document
// holds, and a key that holds no value of the kind the reader needs leaves its
// name in the list the read keeps; the first name of that list is the reason
// the record answers broken. The guard settles the key set, the order and the
// kinds of a document it accepts, so no planted record reaches that answer.
// It stands because the reads cannot rest on a contract: the ignore semantic
// of `speedgun-ng_CONTRACTS` expands SG_ASSERT to nothing, and a record with no
// mask must then answer broken naming that mask, the same answer it gives at
// every other semantic (FR-021, FR-030). A read that answers nothing
// answers an empty value or no value at all, so no element the document never
// held travels on to a field, and the model built beside a note is dropped
// with the note (FR-022).
class RecordReads
{
public:
  // The object under `key`, and nothing where the key holds no object.
  auto object(const Element& object,
              std::string_view key) -> std::optional<Element>
  {
    auto value = Element();
    if (const auto code = object[key].get(value); code != simdjson::SUCCESS) {
      note(key, code);
      return std::nullopt;
    }
    return value;
  }

  // The text under `key`, and empty text where the key holds no text.
  auto text(const Element& object, std::string_view key) -> std::string
  {
    auto value = std::string_view();
    if (const auto code = object[key].get(value); code != simdjson::SUCCESS) {
      note(key, code);
      return {};
    }
    return std::string(value);
  }

  // The number one element holds, named for the key it stands under, and zero
  // where the element holds no number.
  auto numberFrom(const Element& value, std::string_view name) -> std::int64_t
  {
    auto figure = std::int64_t();
    if (const auto code = value.get(figure); code != simdjson::SUCCESS) {
      note(name, code);
      return 0;
    }
    return figure;
  }

  // The number under `key`, and zero where the key holds no number.
  auto number(const Element& object, std::string_view key) -> std::int64_t
  {
    auto figure = std::int64_t();
    if (const auto code = object[key].get(figure); code != simdjson::SUCCESS) {
      note(key, code);
      return 0;
    }
    return figure;
  }

  // The rows one element lists, named for the key it stands under, and nothing
  // where the element lists no rows.
  auto arrayFrom(const Element& value,
                 std::string_view name) -> std::optional<JsonArray>
  {
    auto rows = JsonArray();
    if (const auto code = value.get(rows); code != simdjson::SUCCESS) {
      note(name, code);
      return std::nullopt;
    }
    return rows;
  }

  // The rows under `key`, and nothing where the key lists none.
  auto array(const Element& object,
             std::string_view key) -> std::optional<JsonArray>
  {
    auto rows = JsonArray();
    if (const auto code = object[key].get(rows); code != simdjson::SUCCESS) {
      note(key, code);
      return std::nullopt;
    }
    return rows;
  }

  // One of the two nullable keys, `cpu.stepping` and `machine.nodes`: the
  // document writes `null` for the unanswered value, and the record carries
  // that value as an absent member. An absent key answers the same no value,
  // and its note is what makes the two answers differ (FR-007, FR-013).
  auto nullable(const Element& object,
                std::string_view key) -> std::optional<Element>
  {
    const auto value = this->object(object, key);
    if (!value.has_value() || value->is_null()) {
      return std::nullopt;
    }
    return value;
  }

  // The reason the record answers broken, and nothing when every key the
  // reader asked answered its value (FR-021).
  auto refusal() const -> std::optional<std::string>
  {
    if (m_missing.empty()) {
      return std::nullopt;
    }
    return m_missing.front();
  }

private:
  auto note(std::string_view key, simdjson::error_code code) -> void
  {
    m_missing.push_back(missingKey(key, code));
  }

  std::vector<std::string> m_missing;
};

// The L3 rows, one per row of the document, in its order (FR-012).
auto l3RowsOf(RecordReads& reads,
              const Element& machine) -> std::vector<HostMachine::L3Row>
{
  auto rows = std::vector<HostMachine::L3Row>();
  const auto listed = reads.array(machine, "l3");
  if (!listed.has_value()) {
    return rows;
  }
  for (const auto row : *listed) {
    rows.push_back(HostMachine::L3Row {
        .ordinal = static_cast<std::size_t>(reads.number(row, "ordinal")),
        .cpus = reads.text(row, "cpus"),
    });
  }
  return rows;
}

// The node rows, and nothing where the block stands `null` (FR-013). A key
// that answers no list leaves its note behind and answers the same no rows;
// the note is what tells the two apart (FR-021).
auto nodeRowsOf(RecordReads& reads, const Element& machine)
    -> std::optional<std::vector<HostMachine::NodeRow>>
{
  const auto block = reads.nullable(machine, "nodes");
  if (!block.has_value()) {
    return std::nullopt;
  }
  const auto listed = reads.arrayFrom(*block, "machine.nodes");
  if (!listed.has_value()) {
    return std::nullopt;
  }
  auto rows = std::vector<HostMachine::NodeRow>();
  for (const auto row : *listed) {
    rows.push_back(HostMachine::NodeRow {
        .number = static_cast<std::uint32_t>(reads.number(row, "number")),
        .cpus = reads.text(row, "cpus"),
    });
  }
  return rows;
}

// The identity as its document spells it: the four figures, the record key
// and the selected directory (FR-007 to FR-009).
auto identFrom(RecordReads& reads, const Element& cpu) -> HostIdent
{
  auto ident = HostIdent {};
  const auto stepping = reads.nullable(cpu, "stepping");
  ident.vendor = reads.text(cpu, "vendor");
  ident.family = static_cast<int>(reads.number(cpu, "family"));
  ident.model = static_cast<int>(reads.number(cpu, "model"));
  if (stepping.has_value()) {
    ident.stepping =
        static_cast<int>(reads.numberFrom(*stepping, "cpu.stepping"));
  }
  ident.key = reads.text(cpu, "key");
  ident.directory = reads.text(cpu, "directory");
  return ident;
}

// The machine block as its document spells it: the four counts, the mask
// text and the two row lists (FR-012, FR-013).
auto machineFrom(RecordReads& reads, const Element& machine) -> HostMachine
{
  auto block = HostMachine {};
  block.packages =
      static_cast<std::uint32_t>(reads.number(machine, "packages"));
  block.l3Groups = static_cast<std::size_t>(reads.number(machine, "l3Groups"));
  block.cores = static_cast<std::size_t>(reads.number(machine, "cores"));
  block.threads = static_cast<std::size_t>(reads.number(machine, "threads"));
  block.mask = reads.text(machine, "mask");
  block.l3 = l3RowsOf(reads, machine);
  block.nodes = nodeRowsOf(reads, machine);
  return block;
}

// The recorded runs, row for row in the stored order, an empty list allowed
// (FR-015, FR-041).
auto evidenceFrom(RecordReads& reads,
                  const JsonArray& listed) -> std::vector<EvidenceRow>
{
  auto rows = std::vector<EvidenceRow>();
  for (const auto row : listed) {
    rows.push_back(EvidenceRow {
        .path = reads.text(row, "path"),
        .command = reads.text(row, "command"),
        .exit = static_cast<int>(reads.number(row, "exit")),
        .line = reads.text(row, "line"),
    });
  }
  return rows;
}

// The identity as rule 4 names it: the vendor and the two figures the mapfile
// row matches, the figures the record key already spells in hex (FR-007,
// FR-025).
auto identText(const HostIdent& cpu) -> std::string
{
  return "vendor " + cpu.vendor + ", family " + std::to_string(cpu.family)
      + ", model " + std::to_string(cpu.model);
}

// The document the guard accepted, read into the record its keys spell
// (FR-020, FR-021).
auto recordFrom(RecordReads& reads, const Element& record) -> HostRecord
{
  auto built = HostRecord {};
  built.recordVersion = reads.text(record, "recordVersion");
  const auto cpu = reads.object(record, "cpu");
  if (cpu.has_value()) {
    built.cpu = identFrom(reads, *cpu);
  }
  const auto machine = reads.object(record, "machine");
  if (machine.has_value()) {
    built.machine = machineFrom(reads, *machine);
  }
  const auto evidence = reads.array(record, "evidence");
  if (evidence.has_value()) {
    built.evidence = evidenceFrom(reads, *evidence);
  }
  return built;
}

// The guard of one document: the pinned schema over the bytes, then the size
// cap of `kHostRecordCapBytes`, which no schema keyword states because it is a
// property of the whole document and no value kind spells it. The two answer
// as one, so the step that turns a failed guard into the BROKEN reading of
// that entry stands in one place and a document over the cap answers no record
// either (FR-018, FR-020, FR-030).
auto guardRecord(const std::filesystem::path& schemaPath,
                 const std::string_view bytes)
    -> std::expected<void, std::string>
{
  if (bytes.size() > kHostRecordCapBytes) {
    return std::unexpected("the document holds " + std::to_string(bytes.size())
                           + " bytes over the cap of "
                           + std::to_string(kHostRecordCapBytes));
  }
  return checkReport(schemaPath.string(), bytes);
}

// The document kind, the label a broken line names beside what the guard
// reported. The schema file states the kind in its name and carries no kind
// keyword, so the reader spells the label of that file name here, the way it
// spells the version the format speaks (FR-030).
constexpr std::string_view kRecordKind = "host-record: ";

// The suffix that marks the entry as a record, the suffix the writer gives
// the file it names after the CPU type (FR-009, FR-024).
constexpr std::string_view kRecordSuffix = ".json";

// The entries of one record root that hold records, in the order their names
// sort. The listing rides the error-code form of the directory iterator, so a
// root that stands absent and a root that holds nothing answer the same empty
// list, and only a `.json` entry of a root that holds entries reaches the
// reader (D-10, FR-024, FR-029).
auto listedEntries(const std::filesystem::path& recordRoot)
    -> std::vector<std::filesystem::path>
{
  auto entries = std::vector<std::filesystem::path> {};
  auto listing = std::error_code {};
  for (auto it = std::filesystem::directory_iterator(recordRoot, listing);
       it != std::filesystem::directory_iterator();
       it.increment(listing))
  {
    const auto name = it->path().filename().string();
    if (std::string_view(name).ends_with(kRecordSuffix)) {
      entries.push_back(it->path());
    }
  }
  std::sort(
      entries.begin(),
      entries.end(),
      [](const std::filesystem::path& left, const std::filesystem::path& right)
      { return left.filename().compare(right.filename()) < 0; });
  return entries;
}

// The one word of one verdict, the three words FR-030 names for the verdict
// column of a line.
auto verdictWord(const RecordVerdict verdict) -> const char*
{
  if (verdict == RecordVerdict::BROKEN) {
    return "broken";
  }
  if (verdict == RecordVerdict::STALE) {
    return "stale";
  }
  return "valid";
}

// The line of one reading: the record that answered it, the CPU type, the word
// of its verdict and, where one fired, the rule the detail carries, the four
// columns `contracts/record-check.md` states (FR-024, FR-030). The record of a
// reading carries the name the writer gives that CPU type, the key with the
// suffix this reader lists on (FR-009, FR-024).
auto printReading(const std::filesystem::path& recordRoot,
                  const RecordReading& reading) -> void
{
  const auto file = recordRoot
      / std::filesystem::path(reading.cpuType + std::string(kRecordSuffix));
  if (reading.detail.empty()) {
    std::printf("%s  %s  %s\n",
                file.string().c_str(),
                reading.cpuType.c_str(),
                verdictWord(reading.verdict));
    return;
  }
  std::printf("%s  %s  %s  %s\n",
              file.string().c_str(),
              reading.cpuType.c_str(),
              verdictWord(reading.verdict),
              reading.detail.c_str());
}

// The causes of one record as the one detail of its reading: the reasons in
// rule order, each following the last after a semicolon, so a record that
// carries two causes prints them on the line its reading owns (FR-025,
// FR-030).
auto detailOf(const std::vector<std::string>& reasons) -> std::string
{
  auto detail = std::string {};
  for (const auto& reason : reasons) {
    if (!detail.empty()) {
      detail += "; ";
    }
    detail += reason;
  }
  return detail;
}

// The claim the parse of the guarded bytes rests on: the guard parsed them with
// the same parser and answered for the whole document (FR-022).
constexpr char kGuardRead[] = "the guard read these bytes as JSON";

// The record one file holds, and the reason it answers none. The guard stands
// first, so its own words answer a document it refused. Behind it the reads
// answer a key the document holds no value for, naming that key, and a file
// that cannot be read answers what the read reported. The reason travels so one
// broken line of FR-030 can carry either cause (FR-020 to FR-022, FR-030).
//
// The parse rides the guard's answer: the guard parsed these same bytes with
// this same parser, so the element stands read, and were it ever otherwise
// every read below would answer nothing and the record would answer broken on
// the name of the first key it asked. That is why this step holds no contract
// of its own, and why no element the document never held reaches a field
// (FR-022).
auto readRecord(const std::filesystem::path& file,
                const std::filesystem::path& schemaPath)
    -> std::expected<HostRecord, std::string>
{
  const auto bytes = readTextFile(file.string());
  if (!bytes.has_value()) {
    return std::unexpected(bytes.error());
  }
  const auto guarded = guardRecord(schemaPath, *bytes);
  if (!guarded.has_value()) {
    return std::unexpected(guarded.error());
  }
  simdjson::dom::parser parser;
  [[maybe_unused]] const auto parsed =
      parser.parse(simdjson::padded_string(std::string_view(*bytes)));
  auto root = Element();
  SG_ASSERT(parsed.get(root) == simdjson::SUCCESS, kGuardRead);
  auto reads = RecordReads();
  const auto record = recordFrom(reads, root);
  const auto reason = reads.refusal();
  if (reason.has_value()) {
    return std::unexpected(*reason);
  }
  return record;
}

}  // namespace

auto readHostRecord(const std::filesystem::path& file,
                    const std::filesystem::path& schemaPath)
    -> std::optional<HostRecord>
{
  const auto read = readRecord(file, schemaPath);
  if (!read.has_value()) {
    return std::nullopt;
  }
  return *read;
}

// The rules run in the order the contract states them. A row answers rule 2
// ahead of rule 3, and a row whose file stands absent answers no rule-3 cause,
// because its cited line has no file to stand in. The rows keep the order the
// record stores them, so one run names every cause the record carries
// (FR-015, FR-025).
auto staleReasons(const HostRecord& record,
                  const std::filesystem::path& treeRoot,
                  const DirectoryChooser& chooser) -> std::vector<std::string>
{
  SG_REQUIRE(!record.cpu.key.empty(),
             "staleReasons reads a record that passed the schema");
  auto reasons = std::vector<std::string> {};
  if (record.recordVersion != kHostRecordVersion) {
    reasons.push_back("the record speaks " + record.recordVersion
                      + " and the format speaks "
                      + std::string(kHostRecordVersion));
  }
  for (const auto& row : record.evidence) {
    const auto cited =
        readTextFile((treeRoot / std::filesystem::path(row.path)).string());
    if (!cited.has_value()) {
      reasons.push_back(row.path + " stands absent from the tree");
      continue;
    }
    if (std::string_view(*cited).find(row.line) == std::string::npos) {
      reasons.push_back(row.path + " holds no line the record cites");
    }
  }
  const auto choice = chooser(record.cpu);
  if (choice.answer == DirectoryAnswer::NO_ROW) {
    reasons.push_back(record.cpu.key + " selects no catalog directory: "
                      + identText(record.cpu));
  }
  return reasons;
}

// One entry is read twice on purpose. `readTextFile` answers the listing's own
// question, whether the entry opens, the answer that ends the run with
// `refusal` set to that entry, and `readRecord` answers the same bytes for
// the label and the report a broken line carries, the text it drops along
// with the record it declines (FR-021, FR-029, FR-030). The cap of
// FR-018 holds one record to 16 KiB and a root holds the records of the CPU
// types the lane knows, so the listing pays the second read for those two
// answers.
//
// The guard settles the document before `staleReasons` reads its fields, which
// is where the verdict order stands: a document that fails the guard, in key
// set, key order, value kind or the size of FR-018, answers the BROKEN reading
// and asks the staleness rules and the chooser nothing, and so does a document
// that holds no value for a key the record needs, so one record never carries a
// break and a cause on one line (plan R-07, FR-018, FR-021, FR-030).
// The chooser the check hands the staleness rules: every answer of rule 4
// passes through untouched, and the one answer that names the mapfile itself
// travels out through `unanswered`, because it ends the run in place of
// causing one record (FR-004, FR-025; `staleReasons` keeps that answer out of
// its reasons). It asks its own chooser only when a record asks, so one open
// stands behind a run that has records and none behind a run that has none
// (plan R-06).
auto mapfileSentinel(const DirectoryChooser& chooser,
                     bool& unanswered) -> DirectoryChooser
{
  return [&chooser, &unanswered](const HostIdent& ident) -> DirectoryChoice
  {
    const auto choice = chooser(ident);
    if (choice.answer == DirectoryAnswer::UNREADABLE_MAPFILE) {
      unanswered = true;
    }
    return choice;
  };
}

auto checkRecords(const std::filesystem::path& recordRoot,
                  const std::filesystem::path& treeRoot,
                  const std::filesystem::path& schemaPath,
                  const DirectoryChooser& chooser) -> RecordCheck
{
  auto readings = std::vector<RecordReading> {};
  auto unanswered = false;
  const auto sentinel = mapfileSentinel(chooser, unanswered);
  for (const auto& entry : listedEntries(recordRoot)) {
    const auto bytes = readTextFile(entry.string());
    if (!bytes.has_value()) {
      // A refusal answers alone: the readings a listing gathered before the
      // entry that refused belong to no line (FR-029).
      return RecordCheck {.readings = {}, .refusal = entry.string()};
    }
    const auto read = readRecord(entry, schemaPath);
    if (!read.has_value()) {
      readings.push_back(RecordReading {
          .cpuType = entry.stem().string(),
          .verdict = RecordVerdict::BROKEN,
          .detail = std::string(kRecordKind) + read.error(),
      });
      continue;
    }
    const auto& record = *read;
    const auto reasons = staleReasons(record, treeRoot, sentinel);
    if (unanswered) {
      // The mapfile answers the run: rule 4 stands unanswered for every record
      // of the listing, so the readings gathered before this question belong to
      // no line, and the refusal names the path that stands unreadable
      // (FR-004, FR-029).
      const auto mapfile =
          treeRoot / std::filesystem::path(std::string(kMapfileRelPath));
      return RecordCheck {.readings = {}, .refusal = mapfile.string()};
    }
    readings.push_back(RecordReading {
        .cpuType = record.cpu.key,
        .verdict =
            reasons.empty() ? RecordVerdict::VALID : RecordVerdict::STALE,
        .detail = detailOf(reasons),
    });
  }
  return RecordCheck {
      .readings = std::move(readings),
      .refusal = std::nullopt,
  };
}

// The two roots one reader run names. The parse reads them and nothing else: a
// word that names no option, an option that stands without its value and a word
// the parse cannot use each answer one line naming that word, ahead of the
// factory and ahead of any record, the shape the writer path gives its own
// parse (FR-004, FR-028, R-09).
struct CheckArguments
{
  std::filesystem::path treeRoot;
  std::filesystem::path recordRoot;
};

[[nodiscard]] auto parseCheckArguments(int argc, char** argv)
    -> std::optional<CheckArguments>
{
  auto answer = CheckArguments {};
  for (int at = 1; at < argc; ++at) {
    const auto word = std::string_view(argv[at]);
    if (word != "--root" && word != "--records") {
      if (word.starts_with("--")) {
        std::fprintf(stderr,
                     "error: the option '%s' names no arm of the reader\n",
                     quotedText(std::string(word)).c_str());
      } else {
        std::fprintf(stderr,
                     "error: the argument '%s' names no option of the reader\n",
                     quotedText(std::string(word)).c_str());
      }
      return std::nullopt;
    }
    if (at + 1 == argc) {
      std::fprintf(stderr,
                   "error: the option '%s' stands without its value\n",
                   quotedText(std::string(word)).c_str());
      return std::nullopt;
    }
    const auto value = std::string_view(argv[++at]);
    if (word == "--root") {
      answer.treeRoot = std::string(value);
    } else {
      answer.recordRoot = std::string(value);
    }
  }
  return answer;
}

// The run of one reader: the two roots the parse reads, the schema that guard
// stands or the run refuses, the chooser this run builds for that tree, and the
// answer mapped to lines and a status. The refusal of a root that holds no
// `.json` entry names that root, the answer the listing shares with a root that
// stands absent, and a run that reads nothing asks its chooser nothing, because
// the open waits for the first question (D-10, FR-004, FR-020, FR-029).
auto hostRecordCheckMain(int argc,
                         char** argv,
                         const DirectoryChooserFactory& factory) -> int
{
  SG_REQUIRE(argv != nullptr,
             "the reader reads the words the process handed main (FR-037)");
  SG_REQUIRE(factory != nullptr,
             "the reader builds its chooser from its factory (FR-025)");
  const auto arguments = parseCheckArguments(argc, argv);
  if (!arguments.has_value()) {
    return 2;
  }
  const auto& treeRoot = arguments->treeRoot;
  const auto& recordRoot = arguments->recordRoot;
  const auto schemaPath =
      treeRoot / std::filesystem::path(std::string(kHostRecordSchemaRelPath));
  if (!readTextFile(schemaPath.string()).has_value()) {
    // The guard stands unanswered, so no record of any root could read valid or
    // broken and this line carries the run alone (FR-004, FR-020).
    std::fprintf(stderr,
                 "error: the record schema '%s' cannot be read\n",
                 quotedText(schemaPath.string()).c_str());
    return 2;
  }
  const auto answer =
      checkRecords(recordRoot, treeRoot, schemaPath, factory(treeRoot));
  if (answer.refusal.has_value()) {
    std::fprintf(stderr,
                 "error: the record run cannot read '%s'\n",
                 quotedText(*answer.refusal).c_str());
    return 2;
  }
  if (answer.readings.empty()) {
    std::fprintf(stderr,
                 "error: the record root '%s' holds no record\n",
                 quotedText(recordRoot.string()).c_str());
    return 2;
  }
  auto status = 0;
  for (const auto& reading : answer.readings) {
    printReading(recordRoot, reading);
    if (reading.verdict != RecordVerdict::VALID) {
      status = 2;
    }
  }
  return status;
}

}  // namespace sg::detail
