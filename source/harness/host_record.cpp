// ============================================================================
// specs/031-host-validation T008, T012 and T018: the machine block of one
// layout, the document that block travels in, and the write path that stands
// the document in a root (US1, FR-002 to FR-009, FR-012 to FR-014, FR-018 to
// FR-023, E-22, E-24, R-02, R-05, R-08, R-09).
//
// The write path reads a standing record through `readHostRecord`, the one
// reader of record JSON, keeps the evidence rows it carries, and replaces the
// identity and machine blocks this run reads (FR-006, R-08). A file that
// stands there and answers no record keeps its bytes, and the run answers the
// status of a refusal (FR-011).
//
// `hostMachineFromLayout` reads the layout it is handed and answers the
// counts, the mask text and the two row lists `--topology` prints
// (`source/harness/topology.cpp:551`), so the listing and the record agree
// by construction and this file reads no host (FR-014, FR-032).
//
// The renderer writes through the vendored builder, the tool the report and
// the comparison document already use, and the key order of the bytes is the
// order of the calls, which is the order `docs/schema/host-record-v1.json`
// spells in `x-order` (FR-020, FR-023; `source/harness/compare.cpp:765`).
// The finished text passes the cap of FR-018 before the one write, so a
// refused run leaves no bytes behind (FR-011, R-09).
// ============================================================================

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "detail/host_record.hpp"

#include <simdjson.h>

#include "detail/host_record_check.hpp"
#include "detail/internal.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

// FR-012: the cpus of one L3 group, read the way the listing reads them:
// one per thread whose group ordinal names that group.
[[nodiscard]] auto groupCpus(const MachineLayout& machine,
                             const std::size_t group)
    -> std::vector<std::uint32_t>
{
  auto cpus = std::vector<std::uint32_t> {};
  for (const auto& thread : machine.threads) {
    if (thread.groupOrdinal == group) {
      cpus.push_back(thread.cpuIndex);
    }
  }
  return cpus;
}

// FR-013, FR-034: the cpus of one node that stand inside the process mask,
// in the order the read states them. The read leaves the node lists
// unfiltered and the listing filters them
// (`source/harness/topology.cpp:583`), and the mask stands ascending.
[[nodiscard]] auto maskedCpus(const MachineLayout& machine,
                              const std::vector<std::uint32_t>& nodeCpus)
    -> std::vector<std::uint32_t>
{
  auto cpus = std::vector<std::uint32_t> {};
  for (const auto cpu : nodeCpus) {
    const auto found = std::ranges::lower_bound(machine.mask, cpu);
    if (found != machine.mask.end() && *found == cpu) {
      cpus.push_back(cpu);
    }
  }
  return cpus;
}

// The document emits through the vendored builder, the writer the report and
// the comparison document emit through (`source/harness/json_report.cpp:28`,
// `source/harness/compare.cpp:765`).
using Builder = simdjson::builder::string_builder;

// The builder emits no separator of its own, so every object of the document
// opens a scope that puts one comma before every key after the first and
// closes the brace the scope opened (FR-020).
class RecordObjectWriter
{
public:
  explicit RecordObjectWriter(Builder& builder) noexcept
      : m_builder(builder)
  {
    m_builder.start_object();
  }

  RecordObjectWriter(const RecordObjectWriter&) = delete;
  RecordObjectWriter& operator=(const RecordObjectWriter&) = delete;

  ~RecordObjectWriter() noexcept { m_builder.end_object(); }

  auto key(std::string_view name) noexcept -> void
  {
    if (!m_first) {
      m_builder.append_comma();
    }
    m_first = false;
    m_builder.escape_and_append_with_quotes(name);
    m_builder.append_colon();
  }

private:
  Builder& m_builder;
  bool m_first = true;
};

// The array scope of the writer pair, the same comma rule over the items of
// `l3`, `nodes` and `evidence`.
class RecordArrayWriter
{
public:
  explicit RecordArrayWriter(Builder& builder) noexcept
      : m_builder(builder)
  {
    m_builder.start_array();
  }

  RecordArrayWriter(const RecordArrayWriter&) = delete;
  RecordArrayWriter& operator=(const RecordArrayWriter&) = delete;

  ~RecordArrayWriter() noexcept { m_builder.end_array(); }

  auto item() noexcept -> void
  {
    if (!m_first) {
      m_builder.append_comma();
    }
    m_first = false;
  }

private:
  Builder& m_builder;
  bool m_first = true;
};

// One key and one text: the builder escapes the text and quotes it.
auto put(Builder& builder,
         RecordObjectWriter& object,
         std::string_view key,
         std::string_view value) noexcept -> void
{
  object.key(key);
  builder.escape_and_append_with_quotes(value);
}

// One key and one integer: the builder writes the digits, so this file holds
// no number formatter of its own.
auto put(Builder& builder,
         RecordObjectWriter& object,
         std::string_view key,
         const std::int64_t value) noexcept -> void
{
  object.key(key);
  builder.append(value);
}

// One key whose value the host may leave unanswered: the stepping stands
// `null` where the host answers no leaf 1, the one spelling the schema
// states beside that key (FR-007, FR-020).
auto putOrNull(Builder& builder,
               RecordObjectWriter& object,
               std::string_view key,
               const std::optional<int>& value) noexcept -> void
{
  object.key(key);
  if (!value.has_value()) {
    builder.append_null();
    return;
  }
  builder.append(static_cast<std::int64_t>(*value));
}

// The document text of one record, in the order `contracts/host-record-json.md`
// lists and the schema spells: the version, the identity block, the machine
// block and the evidence list, each block in its own key order (FR-019 to
// FR-023). The bytes carry the values of the record and nothing else, so two
// runs over one host write one document (FR-023, FR-033).
[[nodiscard]] auto renderRecord(const HostRecord& record) -> std::string
{
  auto builder = Builder {};
  {
    RecordObjectWriter root(builder);
    put(builder, root, "recordVersion", record.recordVersion);
    root.key("cpu");
    {
      RecordObjectWriter cpu(builder);
      put(builder, cpu, "vendor", record.cpu.vendor);
      put(builder, cpu, "family", static_cast<std::int64_t>(record.cpu.family));
      put(builder, cpu, "model", static_cast<std::int64_t>(record.cpu.model));
      putOrNull(builder, cpu, "stepping", record.cpu.stepping);
      put(builder, cpu, "key", record.cpu.key);
      put(builder, cpu, "directory", record.cpu.directory);
    }
    root.key("machine");
    {
      RecordObjectWriter machine(builder);
      put(builder,
          machine,
          "packages",
          static_cast<std::int64_t>(record.machine.packages));
      put(builder,
          machine,
          "l3Groups",
          static_cast<std::int64_t>(record.machine.l3Groups));
      put(builder,
          machine,
          "cores",
          static_cast<std::int64_t>(record.machine.cores));
      put(builder,
          machine,
          "threads",
          static_cast<std::int64_t>(record.machine.threads));
      put(builder, machine, "mask", record.machine.mask);
      machine.key("l3");
      {
        RecordArrayWriter groups(builder);
        for (const auto& row : record.machine.l3) {
          groups.item();
          RecordObjectWriter group(builder);
          put(builder,
              group,
              "ordinal",
              static_cast<std::int64_t>(row.ordinal));
          put(builder, group, "cpus", row.cpus);
        }
      }
      // FR-013: the node block stands `null` exactly where the memory read
      // states nothing, the JSON form of the `node: not available` line.
      machine.key("nodes");
      if (!record.machine.nodes.has_value()) {
        builder.append_null();
      } else {
        RecordArrayWriter nodes(builder);
        for (const auto& row : *record.machine.nodes) {
          nodes.item();
          RecordObjectWriter node(builder);
          put(builder, node, "number", static_cast<std::int64_t>(row.number));
          put(builder, node, "cpus", row.cpus);
        }
      }
    }
    // FR-015, FR-041: the evidence rows ride the record, an empty list
    // standing as the empty array.
    root.key("evidence");
    {
      RecordArrayWriter rows(builder);
      for (const auto& row : record.evidence) {
        rows.item();
        RecordObjectWriter evidence(builder);
        put(builder, evidence, "path", row.path);
        put(builder, evidence, "command", row.command);
        put(builder, evidence, "exit", static_cast<std::int64_t>(row.exit));
        put(builder, evidence, "line", row.line);
      }
    }
  }
  auto text = builder.view();
  SG_ENSURE(text.has_value(),
            "the builder's buffer holds the document it wrote");
  return std::string(*text);
}

// The one write of the whole document: the bytes land under a name beside the
// record and move onto it once every byte stands written, so the write a disk
// refuses leaves no partial record behind and leaves the bytes of a standing
// record whole (FR-011, SC-004). The record holds the bytes the render answered
// and ends there, with no newline behind them, the way the comparison document
// ends (`source/harness/compare.cpp:1993`). A refusal reports itself on
// standard error naming the record the run meant to write, and answers false
// (FR-018).
[[nodiscard]] auto writeRecord(const std::filesystem::path& path,
                               std::string_view text) -> bool
{
  // FR-009: the name the bytes land under carries no `.json` suffix, so it
  // names no record while the run holds it open.
  const auto landing =
      path.parent_path() / (path.filename().string() + ".partial");
  const auto refusal = [&path, &landing](std::string_view cause) -> bool
  {
    std::fprintf(stderr,
                 "error: the record file '%s' could not be written: %s\n",
                 quotedText(path.string()).c_str(),
                 quotedText(std::string(cause)).c_str());
    auto left = std::error_code {};
    std::filesystem::remove(landing, left);
    return false;
  };
  auto stream = std::ofstream(landing, std::ios::binary | std::ios::trunc);
  if (!stream.is_open()) {
    std::fprintf(stderr,
                 "error: the record file '%s' cannot be opened: %s\n",
                 quotedText(path.string()).c_str(),
                 quotedText(std::strerror(errno)).c_str());
    return false;
  }
  stream.write(text.data(), static_cast<std::streamsize>(text.size()));
  stream.close();
  if (stream.fail()) {
    return refusal(std::strerror(errno));
  }
  auto moved = std::error_code {};
  std::filesystem::rename(landing, path, moved);
  if (moved) {
    return refusal(moved.message());
  }
  return true;
}

// FR-002: the output root a run takes where it names none, resolved under the
// tree root the parse read, so the directory that started the process names no
// path.
constexpr std::string_view kDefaultOutDir = "validation";

// The two roots a write run names: the tree root the seam reads for its
// mapfile and the output root the record stands under (FR-002, FR-008).
struct WriteArguments
{
  std::filesystem::path treeRoot;
  std::filesystem::path outRoot;
};

// FR-011: the standing record a run cannot read ends that run with one line
// naming the record, and the bytes of the file stand untouched behind it. Both
// shapes of the failure answer that line: bytes that answer no record to the
// reader of FR-022, and a file that answers no bytes to the read at all.
[[nodiscard]] auto refuseStandingRead(const std::filesystem::path& path) -> int
{
  std::fprintf(stderr,
               "error: the standing record '%s' cannot be read\n",
               quotedText(path.string()).c_str());
  return 2;
}

// FR-010: the field that leaves an identity unable to name a CPU, read in the
// order the identity reads its fields. The vendor that answers nothing, the
// family and the model the host leaves unset, and the mapfile answer that names
// no directory each stand here, the last one the case of a host whose identity
// selects no catalog row.
[[nodiscard]] auto identityCause(const HostIdent& ident)
    -> std::optional<std::string_view>
{
  if (ident.vendor.empty()) {
    return std::string_view {"the vendor stands empty"};
  }
  if (ident.family < 0) {
    return std::string_view {"the family stands unset"};
  }
  if (ident.model < 0) {
    return std::string_view {"the model stands unset"};
  }
  if (ident.directory.empty()) {
    return std::string_view {"the mapfile names no directory"};
  }
  return std::nullopt;
}

// The parse of the write path: two options, each with its value, and the
// default output root of FR-002 resolved under the tree root where the run
// names no `--out`. A word that names no option, an option that stands without
// its value, and a word the parse cannot use are refused here with one line
// naming that word, before the host stands asked (FR-004, FR-037, R-09). The
// refusal answers nothing, which is how the entry point reads it.
[[nodiscard]] auto parseWriteArguments(int argc, char** argv)
    -> std::optional<WriteArguments>
{
  auto answer = WriteArguments {};
  auto outNamed = false;
  for (int at = 1; at < argc; ++at) {
    const auto word = std::string_view(argv[at]);
    if (word != "--root" && word != "--out") {
      if (word.starts_with("--")) {
        std::fprintf(stderr,
                     "error: the option '%s' names no arm of the write path\n",
                     quotedText(std::string(word)).c_str());
      } else {
        std::fprintf(
            stderr,
            "error: the argument '%s' names no option of the write " "path\n",
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
      answer.outRoot = std::string(value);
      outNamed = true;
    }
  }
  if (!outNamed) {
    answer.outRoot = answer.treeRoot / std::filesystem::path(kDefaultOutDir);
  }
  return answer;
}

// Whether one path holds a file the run would read as a standing record. The
// answer stands false where the path holds nothing and where the operating
// system answers no state about it, the state a fresh record starts from
// (FR-006).
[[nodiscard]] auto fileStands(const std::filesystem::path& file) -> bool
{
  auto error = std::error_code {};
  const auto stands = std::filesystem::is_regular_file(file, error);
  return !error && stands;
}

}  // namespace

auto hostMachineFromLayout(const MachineLayout& machine) -> HostMachine
{
  SG_REQUIRE(machine.readable,
             "the machine block reads a layout that stands readable (FR-011)");
  auto block = HostMachine {};
  block.packages = machine.packages;
  block.l3Groups = machine.groups.size();
  block.cores = machine.cores.size();
  block.threads = machine.threads.size();
  appendRangeText(block.mask, machine.mask);
  for (std::size_t group = 0; group < machine.groups.size(); ++group) {
    auto row = HostMachine::L3Row {};
    row.ordinal = group;
    appendRangeText(row.cpus, groupCpus(machine, group));
    block.l3.push_back(std::move(row));
  }
  if (machine.memoryReadable) {
    auto rows = std::vector<HostMachine::NodeRow> {};
    for (const auto& node : machine.nodes) {
      auto cpus = maskedCpus(machine, node.cpus);
      if (cpus.empty()) {
        continue;
      }
      auto row = HostMachine::NodeRow {};
      row.number = node.number;
      appendRangeText(row.cpus, std::move(cpus));
      rows.push_back(std::move(row));
    }
    block.nodes = std::move(rows);
  }
  SG_ENSURE(block.nodes.has_value() == machine.memoryReadable,
            "the node block stands absent exactly where the memory read "
            "states nothing (FR-013)");
  return block;
}

auto hostRecordWriteMain(int argc,
                         char** argv,
                         const HostFactsSeam& facts) -> int
{
  SG_REQUIRE(argv != nullptr,
             "the write path reads the words the process handed main (FR-037)");
  const auto arguments = parseWriteArguments(argc, argv);
  if (!arguments.has_value()) {
    return 2;
  }
  const auto& treeRoot = arguments->treeRoot;
  // FR-002, FR-037: the seam answers once, after both roots stand named, and
  // it answers for the root the parse read, the root that holds the mapfile
  // whose directory the identity reports (FR-008, R-06).
  const auto host = facts(treeRoot);
  // FR-004, FR-008: the mapfile of the tree is the file the identity's
  // directory comes out of, and a run that weighed no rows of it reads a
  // refusal naming that file, the way the reader names the mapfile it cannot
  // open. The answer stands with the facts because the seam is the one party
  // that opens the mapfile (R-04). A row set that was weighed and names no
  // directory for this identity is the failure below, and the two read apart.
  if (!host.mapfileReadable) {
    const auto mapfile =
        treeRoot / std::filesystem::path(std::string(kMapfileRelPath));
    std::fprintf(stderr,
                 "error: the mapfile '%s' cannot be read\n",
                 quotedText(mapfile.string()).c_str());
    return 2;
  }
  // FR-010: an identity the record cannot name ends the run ahead of the
  // render with one line naming the identity the seam answered and the field
  // that left it unable to name a CPU. No record stands in the output root, so
  // the reader of FR-014 meets no file it would have to call valid (FR-011,
  // SC-004).
  if (const auto cause = identityCause(host.ident); cause.has_value()) {
    std::fprintf(stderr,
                 "error: the identity '%s' names no CPU: %s\n",
                 quotedText(host.ident.key).c_str(),
                 std::string(*cause).c_str());
    return 2;
  }
  // FR-010: the directory the identity carries names the catalog of the rows
  // that selected it. The matcher spells that directory from the `arch` level
  // of the catalog it read, three levels above the mapfile, so the tree root
  // answers the path beside it. A tree that does not hold that directory holds
  // no events for the CPU the identity names: such a record cites a table no
  // reader can open and no hunter can place, and the reader of FR-025 never
  // compares the field, so the refusal stands here beside the arms that weigh
  // an identity whose directory stands empty (SC-004).
  const auto mapfile = std::filesystem::path(std::string(kMapfileRelPath));
  const auto catalog = mapfile.parent_path().parent_path().parent_path()
      / std::filesystem::path(host.ident.directory);
  auto catalogRead = std::error_code {};
  if (!std::filesystem::is_directory(treeRoot / catalog, catalogRead)) {
    std::fprintf(stderr,
                 "error: the identity '%s' names the directory '%s' the tree "
                 "does not hold\n",
                 quotedText(host.ident.key).c_str(),
                 quotedText(host.ident.directory).c_str());
    return 2;
  }
  // FR-011, SC-004: a machine the harness cannot read is an operational
  // condition, the condition the contract prices at one line and status 2. The
  // check stands here, ahead of the machine block, whose contract reads a
  // layout it must be able to trust: an unreadable machine never reaches that
  // assertion, it reads a refusal (FR-011).
  if (!host.layout.readable) {
    std::fprintf(stderr, "error: the harness cannot read the machine layout\n");
    return 2;
  }
  auto record = HostRecord {};
  record.recordVersion = std::string(kHostRecordVersion);
  record.cpu = host.ident;
  record.machine = hostMachineFromLayout(host.layout);
  // FR-009: the file name is the key with the `.json` suffix and nothing
  // else, so one CPU type owns one record.
  const auto path = arguments->outRoot / (record.cpu.key + ".json");
  // FR-006, R-08: the standing record reads first, so the rows it carries
  // survive the run that replaces the identity and the machine block. The one
  // reader of record JSON reads it against the schema the tree root carries
  // (FR-020, FR-022), and the bytes answer for the file first because
  // `readHostRecord` reads a file that holds bytes. FR-011: a file that stands
  // there and answers no record keeps its bytes, and the run answers the
  // status of a refusal beside the line naming the record it failed on.
  const auto bytes = readTextFile(path.string());
  if (bytes.has_value()) {
    const auto standing = readHostRecord(
        path, treeRoot / std::filesystem::path(kHostRecordSchemaRelPath));
    if (!standing.has_value()) {
      return refuseStandingRead(path);
    }
    record.evidence = standing->evidence;
  } else if (fileStands(path)) {
    return refuseStandingRead(path);
  }
  const auto text = renderRecord(record);
  // FR-018: the finished text passes the cap before the one write, so a
  // record no reader can read stands unwritten and a standing record keeps
  // its bytes.
  if (text.size() > kHostRecordCapBytes) {
    std::fprintf(stderr,
                 "error: the record '%s' holds %zu bytes over the cap of %zu\n",
                 quotedText(path.string()).c_str(),
                 text.size(),
                 kHostRecordCapBytes);
    return 2;
  }
  if (!writeRecord(path, text)) {
    return 2;
  }
  // FR-005: one line names the CPU type and the path this run wrote.
  std::printf(
      "host-record: %s  %s\n", record.cpu.key.c_str(), path.string().c_str());
  return 0;
}

}  // namespace sg::detail
