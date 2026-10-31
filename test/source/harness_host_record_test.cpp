// ============================================================================
// T006: the machine block of a planted layout (specs/031-host-validation;
// FR-012, FR-013, FR-014, FR-032, SC-001; wave 2, M-B).
//
// `installTestLayout` hands `hostMachineFromLayout` a planted layout
// through the standing topology seam, and the block answers the four
// counts, the mask text in the `appendRangeText` spelling, one row per L3
// group, and the node rows the standing listing prints
// (`source/harness/topology.cpp:551`). The layout read caches once per
// process, so each planted layout rides a process of its own, the fork and
// execv pattern of `harness_topology_test.cpp`. Nothing here reads `/proc`,
// `/sys`, a cpu number or a clock, so every runner answers one block from
// the planted figures (FR-033, FR-035).
//
// Scenario map:
//   machineBlock  the memory read answers: the four counts, the mask text,
//                 one row per L3 group in that vector's order, and one node
//                 row per node the mask leaves, each naming the cpus inside
//                 the mask alone (FR-012, FR-013, FR-014, FR-034)
//   noMemory      the memory read answers false: the node block stands
//                 absent, the value form of the standing
//                 `node: not available` line, and the counts and the group
//                 rows stand as the first scenario read them (FR-013)
//   keyOrder      the planted host writes through `hostRecordWriteMain` and
//                 the bytes hold every key where the schema's `x-order`
//                 puts it, opening with the version this writer speaks
//                 (FR-019, FR-020, FR-021, FR-023, SC-005)
//   repeatBytes   two writes of one planted host differ in no byte and
//                 leave one record for that CPU type (FR-009, FR-023,
//                 SC-007)
//   plainEnd      the file ends with the document and no newline, and its
//                 bytes carry no wall-clock instant (FR-018, FR-033)
//   sizeCap       a write whose document would pass 16 KiB answers one line
//                 and status 2, the standing record keeps its bytes and an
//                 output root that stands empty stays empty (FR-011,
//                 FR-018)
// ============================================================================

#include <algorithm>
#include <array>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/host_record.hpp"
#include "detail/internal.hpp"
#include "detail/json_fixture.hpp"

namespace
{

using sg::test::check;
using sg::test::fail;

// One thread of a planted layout: the cpu it sits on beside the ordinals of
// the core and the L3 group that hold it.
struct PlantedThread
{
  std::uint32_t cpu;
  std::uint32_t core;
  std::uint32_t group;
};

// The planted layout of both scenarios: two packages, two L3 groups, four
// cores and six threads over the mask `0-2,12-13,24`. Group 0 holds cores 0
// and 1 over cpus 0, 1 and 2; group 1 holds cores 2 and 3 over cpus 12, 13
// and 24. The three vectors carry the ordinals the walk that builds a read
// layout leaves (`source/harness/detail/internal.hpp:59`), so the planted
// layout reads as a read layout and the block reads the layout the listing
// prints (FR-014).
//
// `memoryReadable` answers the memory read. Where it answers true, three
// nodes stand: node 0 holds cpu 3, which the mask leaves, node 1 reaches
// past the mask with cpus 25, 30 and 31, and node 2 stands wholly outside
// the mask. The node cpu lists arrive unfiltered, the way the read leaves
// them, and the listing filters them (`source/harness/topology.cpp:583`).
auto plantedLayout(const bool memoryReadable) -> sg::detail::MachineLayout
{
  auto layout = sg::detail::MachineLayout {};
  layout.readable = true;
  layout.packages = 2;
  layout.cores.resize(4);
  layout.groups = {sg::detail::LayoutGroup {0, {0, 1}},
                   sg::detail::LayoutGroup {12, {2, 3}}};
  const std::array<PlantedThread, 6> threads {PlantedThread {0, 0, 0},
                                              PlantedThread {1, 1, 0},
                                              PlantedThread {2, 1, 0},
                                              PlantedThread {12, 2, 1},
                                              PlantedThread {13, 3, 1},
                                              PlantedThread {24, 3, 1}};
  for (const auto& thread : threads) {
    layout.mask.push_back(thread.cpu);
    auto& core = layout.cores[thread.core];
    core.groupOrdinal = thread.group;
    core.threads.push_back(static_cast<std::uint32_t>(layout.threads.size()));
    layout.threads.push_back(
        sg::detail::LayoutThread {.cpuIndex = thread.cpu,
                                  .coreOrdinal = thread.core,
                                  .groupOrdinal = thread.group});
  }
  layout.memoryReadable = memoryReadable;
  if (memoryReadable) {
    layout.nodes = {
        sg::detail::LayoutNode {0, {0, 1, 2, 3, 12, 13}},
        sg::detail::LayoutNode {1, {24, 25, 30, 31}},
        sg::detail::LayoutNode {2, {40, 41}},
    };
  }
  return layout;
}

// The counts and the mask text the planted layout answers: cpu 0, 1 and 2
// fold to `0-2`, cpus 12 and 13 fold to `12-13`, and cpu 24 stands alone
// (FR-012).
auto checkCountsAndMask(const sg::detail::HostMachine& block) -> void
{
  check(block.packages == 2U,
        "the block answers the package count of the layout it reads (FR-012)");
  check(block.l3Groups == 2U,
        "the block answers the L3 group count of the layout it reads (FR-012)");
  check(block.cores == 4U,
        "the block answers the core count of the layout it reads (FR-012)");
  check(block.threads == 6U,
        "the block answers the thread count of the layout it reads (FR-012)");
  check(block.mask == "0-2,12-13,24",
        "the mask carries the range spelling `appendRangeText` renders "
        "(FR-012)");
}

// One row per L3 group, in the order of that group's vector, each naming
// the cpus of its own threads in the same spelling (FR-012).
auto checkGroupRows(const sg::detail::HostMachine& block) -> void
{
  check(block.l3.size() == 2U,
        "the block carries one row per L3 group (FR-012)");
  if (block.l3.size() != 2U) {
    return;
  }
  check(block.l3[0].ordinal == 0U && block.l3[0].cpus == "0-2",
        "the first row names group 0 and the cpus inside the mask (FR-012)");
  check(block.l3[1].ordinal == 1U && block.l3[1].cpus == "12-13,24",
        "the second row names group 1 and the cpus inside the mask (FR-012)");
}

// One node row per node the mask leaves, in the order of the node vector,
// each naming only the cpus inside the mask: node 0 loses cpu 3, node 1
// loses cpus 25, 30 and 31, and node 2 stands no row (FR-013, FR-034).
auto checkNodeRows(const sg::detail::HostMachine& block) -> void
{
  check(block.nodes.has_value(),
        "a machine whose memory reads available carries a node block (FR-013)");
  if (!block.nodes.has_value()) {
    return;
  }
  check(block.nodes->size() == 2U,
        "a node the mask leaves out stands no row (FR-013)");
  if (block.nodes->size() != 2U) {
    return;
  }
  check((*block.nodes)[0].number == 0U && (*block.nodes)[0].cpus == "0-2,12-13",
        "the first node row names the cpus the mask leaves (FR-013, FR-034)");
  check((*block.nodes)[1].number == 1U && (*block.nodes)[1].cpus == "24",
        "the second node row names one cpu and no cpu outside the mask "
        "(FR-013, FR-034)");
}

// FR-012, FR-013, FR-014, FR-032: the block of a planted layout whose
// machine memory reads available, read from the layout the seam answers.
auto childMachineBlock() -> int
{
  const auto planted = plantedLayout(true);
  sg::detail::installTestLayout(planted);
  const auto& layout = sg::detail::layout();
  check(layout.readable && layout.threads.size() == planted.threads.size()
            && layout.nodes.size() == planted.nodes.size(),
        "the seam answers the layout the block reads (FR-032)");
  const auto block = sg::detail::hostMachineFromLayout(layout);
  checkCountsAndMask(block);
  checkGroupRows(block);
  checkNodeRows(block);
  return sg::test::failures == 0 ? 0 : 1;
}

// FR-013: the same layout with the memory read answered false. The node
// block stands absent, and the counts and the group rows stand as the
// first scenario read them.
auto childNoMemory() -> int
{
  sg::detail::installTestLayout(plantedLayout(false));
  const auto block = sg::detail::hostMachineFromLayout(sg::detail::layout());
  check(!block.nodes.has_value(),
        "a machine whose memory reads unavailable answers no node block "
        "(FR-013)");
  checkCountsAndMask(block);
  checkGroupRows(block);
  return sg::test::failures == 0 ? 0 : 1;
}

// One scenario per process: the layout read caches once, so an install
// after the first read answers nothing (`harness_topology_test.cpp:98`).
auto runChild(const char* self, const char* mode) -> int
{
  const pid_t pid = fork();
  if (pid < 0) {
    std::fprintf(stderr, "cannot fork the child\n");
    return 1;
  }
  if (pid == 0) {
    char* const execArgv[] = {
        const_cast<char*>(self), const_cast<char*>(mode), nullptr};
    (void)execv(self, execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    std::fprintf(stderr, "cannot wait for the child\n");
    return 1;
  }
  return status;
}

// The fixture identity of `contracts/host-record-json.md`: leaf 0 answers
// `GenuineIntel`, leaf 1 answers family 6, model 198 and stepping 2, and the
// mapfile of the run's root selects `arch/x86/arrowlake/`, so the record key
// carries one segment more than the model key (FR-007, FR-009).
auto plantedIdent() -> sg::detail::HostIdent
{
  auto ident = sg::detail::HostIdent {};
  ident.vendor = "GenuineIntel";
  ident.family = 6;
  ident.model = 198;
  ident.stepping = 2;
  ident.key = "GenuineIntel-6-C6-2";
  ident.directory = "arch/x86/arrowlake/";
  return ident;
}

// The host facts one planted host answers: the identity above beside the
// layout the machine-block scenarios plant, so the record's machine block
// carries the figures `checkCountsAndMask` pins (FR-032, FR-035).
auto plantedFacts() -> sg::detail::HostFacts
{
  auto facts = sg::detail::HostFacts {};
  facts.ident = plantedIdent();
  facts.layout = plantedLayout(true);
  return facts;
}

// The seam of the byte scenarios: it answers the planted facts for whichever
// root the parse named, so the arm reads no host (FR-032, FR-035).
auto plantedSeam() -> sg::detail::HostFactsSeam
{
  return [](const std::filesystem::path&) { return plantedFacts(); };
}

// The seam of a host that answers two fields by nothing: the fixture identity
// with leaf 1 left unset beside the layout whose memory read states nothing.
// The family, the model and the directory stand the fixture's, so the run
// answers a write (FR-007, FR-013).
auto absentFieldsSeam() -> sg::detail::HostFactsSeam
{
  return [](const std::filesystem::path&)
  {
    auto facts = plantedFacts();
    facts.ident.stepping = std::nullopt;
    facts.layout = plantedLayout(false);
    return facts;
  };
}

// The size cap FR-018 states, the ceiling one written record stays under.
constexpr std::size_t kCapBytes = 16U * 1024U;

// The seam of the cap scenario: an identity whose `vendor` text stands above
// the cap on its own, the planted host whose document the writer cannot
// lawfully write. The key stands the fixture's key, so the record name stays
// the standing file's name, and the directory stands the fixture's directory,
// so the catalog of the tree answers for the run and the cap stands the only
// refusal the document meets (FR-008, FR-018).
auto overCapSeam() -> sg::detail::HostFactsSeam
{
  return [](const std::filesystem::path&)
  {
    auto facts = plantedFacts();
    facts.ident.vendor = std::string(kCapBytes + 1024U, 'x');
    return facts;
  };
}

// One acceptance row of a standing record, spelled the way the writer's call
// order spells its four keys (`contracts/host-record-json.md`), so reading the
// bytes of a kept row reads that row's own fields. Each row cites a run the
// repository records, the shape FR-015 states.
constexpr std::string_view kRowPlacement {
    R"ROW({"path":"specs/035-numa-placement/results/nps1.md","command":"ctest --test-dir build -R harness_numa_placement_test","exit":0,"line":"$ ctest --test-dir build -R harness_numa_placement_test"})ROW"};
constexpr std::string_view kRowPrecheck {
    R"ROW({"path":"specs/034-host-noise-precheck/results/quickstart.md","command":"ctest --test-dir build -R harness_precheck","exit":0,"line":"| Feature suite | `ctest --test-dir build -R harness_precheck` | 0 | 1 test, ok |"})ROW"};
constexpr std::string_view kRowRoundTrip {
    R"ROW({"path":"specs/035-numa-placement/results/quickstart.md","command":"ctest --test-dir build -R harness_json","exit":0,"line":"| The round trip holds | `ctest --test-dir build -R harness_json` | 0 | the written and reloaded documents compare equal |"})ROW"};

// The evidence array of planted rows, in the order given (FR-015).
auto evidenceArray(std::initializer_list<std::string_view> rows) -> std::string
{
  auto text = std::string {"["};
  for (const auto row : rows) {
    if (text.size() > 1U) {
      text += ',';
    }
    text += row;
  }
  text += ']';
  return text;
}

// The standing record the refresh scenarios plant: the fixture's key, so the
// next run opens this file, beside a version, an identity and a machine
// block that stand for the next run to replace. Its rows stand in an order no
// sorting of their paths answers, so a writer that reordered them reads as a
// break (FR-006, FR-015).
auto standingRecord(const std::string& rows) -> std::string
{
  auto text = std::string {
      R"STANDING({"recordVersion":"0.9","cpu":{"vendor":"OldVendor","family":15,"model":5,"stepping":9,"key":"GenuineIntel-6-C6-2","directory":"arch/x86/oldlake/"},"machine":{"packages":9,"l3Groups":9,"cores":99,"threads":99,"mask":"0-1","l3":[{"ordinal":9,"cpus":"9"}],"nodes":null},)STANDING"};
  text += R"EVIDENCE("evidence":)EVIDENCE";
  text += rows;
  text += '}';
  return text;
}

// The evidence array of one written document: the key stands last in the
// document's order, so its array runs to the bracket that closes the document
// (FR-020).
auto evidenceArrayOf(std::string_view text) -> std::string
{
  constexpr std::string_view kEvidenceKey {R"EVIDENCE("evidence":)EVIDENCE"};
  const auto at = text.find(kEvidenceKey);
  const auto close = text.rfind(']');
  if (at == std::string_view::npos || close == std::string_view::npos
      || close < at)
  {
    return {};
  }
  const auto from = at + kEvidenceKey.size();
  return std::string(text.substr(from, close - from + 1U));
}

// One scenario root under the working directory the test binaries own, named
// with the pid so parallel runs stand apart
// (`harness_precheck_test.cpp:71`). The root stands empty when it answers,
// the state of a first write (FR-032).
auto testRoot(const std::string& name) -> std::filesystem::path
{
  const auto path = std::filesystem::path("harness_host_record_roots")
      / (name + "_pid" + std::to_string(::getpid()));
  std::filesystem::remove_all(path);
  const bool created = std::filesystem::create_directories(path);
  check(created || std::filesystem::is_directory(path),
        "the scenario root stands created (FR-032)");
  return path;
}

// A tree root that carries the pinned schema the way a checkout does, so the
// write path can read a standing record against it (FR-020, FR-022). The
// record stands under its own output root, because the run reads the schema
// from the tree root and writes the record under the output root (FR-002).
auto plantedTree(const std::filesystem::path& out) -> std::filesystem::path
{
  const auto tree = testRoot(out.filename().string() + "_tree");
  const auto target = tree / "docs" / "schema" / "host-record-v1.json";
  std::filesystem::create_directories(target.parent_path());
  sg::test::writeText(target.string(), sg::test::readText(SG_SCHEMA_PATH));
  // The catalog the planted identity names: `cpu.directory` stands relative to
  // the catalog the pinned rows read from, two levels under the mapfile
  // (FR-008).
  std::filesystem::create_directories(tree / "external" / "pmu-events"
                                      / plantedIdent().directory);
  return tree;
}

// The two arguments every write run takes: the tree root the planted facts
// answer for, which carries the schema a standing record reads against, beside
// the output root the record stands under (FR-002).
auto writeArguments(const std::filesystem::path& root)
    -> std::vector<std::string>
{
  const auto tree = plantedTree(root);
  return {"--root", tree.string(), "--out", root.string()};
}

// The record name of one CPU type: the key with the `.json` suffix and
// nothing else (FR-009).
auto recordPath(const std::filesystem::path& out) -> std::filesystem::path
{
  return out / "GenuineIntel-6-C6-2.json";
}

// The name a write lands its bytes under before the move: the record name with
// `.partial` behind the `.json` suffix, the name FR-009 keeps off the record
// root's reading.
auto landingPath(const std::filesystem::path& out) -> std::filesystem::path
{
  const auto record = recordPath(out);
  return record.parent_path() / (record.filename().string() + ".partial");
}

// The names of the entries one root holds, in name order, so a scenario
// counts what a run left behind (FR-003).
auto entriesUnder(const std::filesystem::path& root) -> std::vector<std::string>
{
  auto names = std::vector<std::string> {};
  for (const auto& entry : std::filesystem::directory_iterator(root)) {
    names.push_back(entry.path().filename().string());
  }
  std::ranges::sort(names);
  return names;
}

// The two files a captured run sends its streams to, apart from the roots so
// a stream never reads as a record (FR-003).
constexpr std::string_view kOutStreamPath = "harness_host_record_stdout.txt";
constexpr std::string_view kErrStreamPath = "harness_host_record_stderr.txt";

// The status and the two streams of one write-path call.
struct WriteRun
{
  int status = 0;
  std::string out;
  std::string err;
};

// One call of `hostRecordWriteMain` with both streams captured, the redirect
// shape of `harness_compare_cli_test.cpp:123`, so a scenario reads the lines
// of a refusal beside the status it answers (FR-004, FR-005).
auto runWrite(const std::vector<std::string>& arguments,
              const sg::detail::HostFactsSeam& facts) -> WriteRun
{
  auto argv =
      std::vector<char*> {const_cast<char*>("harness_host_record_test")};
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  const int savedOut = dup(STDOUT_FILENO);
  const int savedErr = dup(STDERR_FILENO);
  if (savedOut < 0
      || std::freopen(std::string(kOutStreamPath).c_str(), "w", stdout)
          == nullptr)
  {
    fail("the standard output of the write redirects");
  }
  if (savedErr < 0
      || std::freopen(std::string(kErrStreamPath).c_str(), "w", stderr)
          == nullptr)
  {
    fail("the standard error of the write redirects");
  }
  auto run = WriteRun {sg::detail::hostRecordWriteMain(
                           static_cast<int>(argv.size()), argv.data(), facts),
                       {},
                       {}};
  std::fflush(stdout);
  std::fflush(stderr);
  if (dup2(savedOut, STDOUT_FILENO) < 0 || dup2(savedErr, STDERR_FILENO) < 0) {
    fail("the two streams restore");
  }
  close(savedOut);
  close(savedErr);
  run.out = sg::test::readText(std::string(kOutStreamPath));
  run.err = sg::test::readText(std::string(kErrStreamPath));
  return run;
}

// The non-empty lines of one captured stream, in order
// (`harness_compare_cli_test.cpp:150`).
auto linesOf(const std::string& text) -> std::vector<std::string>
{
  auto lines = std::vector<std::string> {};
  auto stream = std::istringstream(text);
  auto line = std::string {};
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

// The key names of one written document, in the order the bytes hold them. A
// quoted text is a key when a colon follows its closing quote, so the value
// texts stay out of the answer and a key the writer adds stands named.
auto keysOf(std::string_view text) -> std::vector<std::string>
{
  auto keys = std::vector<std::string> {};
  for (std::size_t at = 0; at < text.size(); ++at) {
    if (text[at] != '"') {
      continue;
    }
    const auto last = text.find('"', at + 1);
    if (last == std::string_view::npos) {
      break;
    }
    if (last + 1 < text.size() && text[last + 1] == ':') {
      keys.emplace_back(text.substr(at + 1, last - at - 1));
    }
    at = last;
  }
  return keys;
}

// The order the pinned schema spells in its `x-order` lists, in the order a
// written document holds its keys: the four root keys, the six keys of
// `cpu`, the seven keys of `machine`, the two keys of each of the planted
// record's two group rows and two node rows, and the empty `evidence` list
// (FR-020, FR-021). `harness_host_record_schema_test` pins the same order
// against the schema file itself (FR-021).
constexpr std::array<std::string_view, 25> kRecordKeyOrder {
    "recordVersion", "cpu",   "vendor",    "family",  "model",
    "stepping",      "key",   "directory", "machine", "packages",
    "l3Groups",      "cores", "threads",   "mask",    "l3",
    "ordinal",       "cpus",  "ordinal",   "cpus",    "nodes",
    "number",        "cpus",  "number",    "cpus",    "evidence"};

// Every key of the written bytes stands where the schema puts it, so the
// document holds its keys in the schema order (FR-023, SC-005).
auto checkKeyOrder(std::string_view text) -> void
{
  const auto keys = keysOf(text);
  check(keys.size() == kRecordKeyOrder.size(),
        "the document holds the keys the schema names (FR-020, FR-021)");
  const std::size_t shared = std::min(keys.size(), kRecordKeyOrder.size());
  for (std::size_t at = 0; at < shared; ++at) {
    if (std::string_view(keys[at]) == kRecordKeyOrder[at]) {
      continue;
    }
    std::printf("  key %zu: the bytes hold '%s', the schema puts '%s' there\n",
                at,
                keys[at].c_str(),
                std::string(kRecordKeyOrder[at]).c_str());
    check(false,
          "the written key order follows the schema's x-order (FR-020, "
          "FR-021, FR-023)");
  }
}

// True when the bytes hold a wall-clock instant: a run of four digits, the
// shape of a year or an epoch reading, or a colon between two digits, the
// shape of a clock. JSON puts every colon after a closing quote, and the
// planted figures stand at three digits at the most, so a written record
// that trips either shape carries the instant the contract bans (FR-018,
// FR-033).
auto holdsInstant(std::string_view text) -> bool
{
  std::size_t run = 0;
  for (std::size_t at = 0; at < text.size(); ++at) {
    const char c = text[at];
    const bool digit = c >= '0' && c <= '9';
    if (!digit) {
      if (c == ':' && run > 0 && at + 1 < text.size() && text[at + 1] >= '0'
          && text[at + 1] <= '9')
      {
        return true;
      }
      run = 0;
      continue;
    }
    ++run;
    if (run >= 4) {
      return true;
    }
  }
  return false;
}

// FR-019, FR-020, FR-021, FR-023, SC-005: the planted host writes one
// record, and those bytes hold their keys in the order the pinned schema
// spells and open with the version this writer speaks.
// A ceiling of zero descriptors asks the kernel to refuse every fresh
// allocation, whatever descriptors the process already holds or inherited. A
// ceiling set to the count of descriptors held answers a runner a successful
// open: the next allocation takes the lowest free number, and a runner that
// inherits a high descriptor leaves a free number under that count.
constexpr rlim_t kNoDescriptors = 0;

// FR-011, SC-004: the standing record that stands there and answers no bytes.
// The arm runs in a process of its own, because the plant that refuses the read
// leaves no descriptor for the streams a run redirects to be read back. That
// plant is a ceiling on the descriptors this process may open: the read of the
// record answers `Too many open files`, the state question that follows needs
// none and answers that a regular file stands there, and the run ends at the
// line naming the record. A mode is no plant here, because uid 0 asks the
// permission bits no refusal and reads such a file whole, and a ceiling binds
// every user the kernel answers a descriptor to, the runner of a CI container
// included. Two shapes that bind uid 0 by kernel rule were weighed and move the
// sentence: a symbolic link that points at itself answers `ELOOP` to the read
// and nothing to the state question that follows, so the run sees no standing
// record and its move replaces the link, and a directory answers the move. This
// arm needs a file that stands there and answers no bytes, and only the ceiling
// refuses the read while the file still stands.
auto childStandingCeiling() -> int
{
  const auto standing = testRoot("standing_ceiling");
  const auto kept =
      standingRecord(evidenceArray({kRowPlacement, kRowPrecheck}));
  sg::test::writeText(recordPath(standing).string(), kept);
  const auto arguments = writeArguments(standing);
  auto argv =
      std::vector<char*> {const_cast<char*>("harness_host_record_test")};
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  // The refusal line lands in a file the child opens before the ceiling stands
  // lowered, so the arm still reads the sentence the run answered.
  const auto errPath = std::string {"harness_host_record_standing.txt"};
  const auto saved = dup(STDERR_FILENO);
  if (saved < 0 || std::freopen(errPath.c_str(), "w", stderr) == nullptr) {
    fail("the standard error of the ceiling run redirects");
  }
  auto ceiling = rlimit {};
  if (getrlimit(RLIMIT_NOFILE, &ceiling) != 0) {
    fail("the descriptor ceiling reads");
  }
  const auto stood = ceiling.rlim_cur;
  ceiling.rlim_cur = kNoDescriptors;
  if (setrlimit(RLIMIT_NOFILE, &ceiling) != 0) {
    fail("the descriptor ceiling lowers");
  }
  const auto status = sg::detail::hostRecordWriteMain(
      static_cast<int>(argv.size()), argv.data(), plantedSeam());
  ceiling.rlim_cur = stood;
  if (setrlimit(RLIMIT_NOFILE, &ceiling) != 0) {
    fail("the descriptor ceiling stands restored");
  }
  std::fflush(stderr);
  if (dup2(saved, STDERR_FILENO) < 0) {
    fail("the standard error of the ceiling run restores");
  }
  close(saved);
  const auto lines = linesOf(sg::test::readText(errPath));
  if (status != 2 || lines.size() != 1) {
    std::printf(
        "  standing ceiling: status=%d lines=%zu\n", status, lines.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(status == 2,
        "a standing record no read reaches answers status 2 (FR-011, SC-004)");
  check(lines.size() == 1, "the failed standing read prints one line (FR-011)");
  check(!lines.empty()
            && lines.front().find(recordPath(standing).string())
                != std::string::npos,
        "that line names the record the read failed on (FR-011)");
  check(!lines.empty()
            && lines.front().find("cannot be read") != std::string::npos,
        "that line speaks the read the ceiling refused (FR-011)");
  check(sg::test::readText(recordPath(standing).string()) == kept,
        "the standing bytes stand kept behind the failed read (FR-011)");
  check(entriesUnder(standing).size() == 1,
        "the failed standing read leaves one file, the record it read "
        "(FR-011, SC-004)");
  return sg::test::failures == 0 ? 0 : 1;
}

auto scenarioKeyOrder() -> void
{
  const auto out = testRoot("key_order");
  const auto run = runWrite(writeArguments(out), plantedSeam());
  check(run.status == 0, "the planted host writes its record (FR-005)");
  const auto text = sg::test::readText(recordPath(out).string());
  check(!text.empty(), "the record file holds the written bytes (FR-005)");
  if (text.empty()) {
    return;
  }
  check(text.starts_with(R"({"recordVersion":"1.0")"),
        "the document opens with the version this writer speaks (FR-019)");
  checkKeyOrder(text);
}

// FR-009, FR-023, SC-007: two writes of one planted host differ in no byte,
// the second opening the standing file afresh, and one record stands for
// that CPU type.
auto scenarioRepeatBytes() -> void
{
  const auto out = testRoot("repeat_bytes");
  const auto seam = plantedSeam();
  const auto first = runWrite(writeArguments(out), seam);
  const auto written = sg::test::readText(recordPath(out).string());
  const auto second = runWrite(writeArguments(out), seam);
  const auto again = sg::test::readText(recordPath(out).string());
  check(first.status == 0 && second.status == 0,
        "both writes answer status 0 (FR-005, FR-023)");
  check(!written.empty() && written == again,
        "two writes of one planted host differ in no byte (FR-023, SC-007)");
  check(entriesUnder(out).size() == 1,
        "one record stands for that CPU type (FR-009)");
}

// FR-018, FR-033: the file ends with the document and no newline, the way
// the comparison document ends (`source/harness/compare.cpp:1993`), and its
// bytes carry no wall-clock instant.
auto scenarioPlainEnd() -> void
{
  const auto out = testRoot("plain_end");
  const auto run = runWrite(writeArguments(out), plantedSeam());
  check(run.status == 0, "the planted host writes its record (FR-005)");
  const auto text = sg::test::readText(recordPath(out).string());
  check(!text.empty(), "the record file holds the written bytes (FR-005)");
  if (text.empty()) {
    return;
  }
  check(text.back() == '}',
        "the file ends with the document and no newline (FR-018, FR-023)");
  check(!holdsInstant(text),
        "the written bytes carry no wall-clock instant (FR-018, FR-033)");
}

// FR-011, FR-018: a write whose document would pass 16 KiB answers one line
// and status 2, a standing record keeps its bytes, and an output root that
// stands empty stays empty.
auto scenarioSizeCap() -> void
{
  const auto standing = testRoot("cap_standing");
  const auto fresh = testRoot("cap_fresh");
  const auto kept = runWrite(writeArguments(standing), plantedSeam());
  const auto bytes = sg::test::readText(recordPath(standing).string());
  check(kept.status == 0 && !bytes.empty(),
        "the standing record stands written before the cap run (FR-005)");

  const auto refused = runWrite(writeArguments(standing), overCapSeam());
  check(refused.status == 2,
        "a record that would pass the cap answers status 2 (FR-018)");
  check(linesOf(refused.err).size() == 1,
        "the cap refusal prints one line on standard error (FR-004, FR-018)");
  check(sg::test::readText(recordPath(standing).string()) == bytes,
        "a standing record keeps its bytes when the cap refuses (FR-018)");

  const auto empty = runWrite(writeArguments(fresh), overCapSeam());
  check(empty.status == 2,
        "the cap refusal answers status 2 against an empty root (FR-018)");
  check(linesOf(empty.err).size() == 1,
        "the cap refusal prints one line for an empty root (FR-004, FR-018)");
  check(entriesUnder(fresh).empty(),
        "the refused run leaves the output root empty (FR-011, FR-018)");
}

// FR-003, FR-005, FR-007, FR-008, FR-009, FR-032, SC-001: one run over an
// empty output root leaves one record named by the key the seam answered,
// prints one line naming the CPU type and the path it wrote, answers status 0,
// leaves nothing on standard error, and asks the seam once for the tree root
// the parse read.
//
// These checks characterize the write path of `4289d9c`: the plan puts this
// test task (T014) behind the task that wrote that code (T012), so each one
// answered green the moment it stood written, and they enter the suite
// without a red run of their own. They stand to hold the success path of
// FR-005 and FR-009 while the refresh and the refusal arms grow around it.
auto scenarioOneWrite() -> void
{
  const auto out = testRoot("one_write_out");
  const auto tree = plantedTree(out);
  auto asked = std::vector<std::string> {};
  const auto seam =
      [&asked](const std::filesystem::path& root) -> sg::detail::HostFacts
  {
    asked.push_back(root.string());
    return plantedFacts();
  };
  const auto run =
      runWrite({"--root", tree.string(), "--out", out.string()}, seam);
  check(run.status == 0,
        "one write of the planted host answers status 0 (FR-005)");
  check(asked.size() == 1 && asked.front() == tree.string(),
        "the seam answers once, for the tree root the parse read (FR-008, "
        "FR-037)");
  check(entriesUnder(out)
            == std::vector<std::string> {"GenuineIntel-6-C6-2.json"},
        "the root holds one record named by the key with the suffix (FR-003, "
        "FR-009)");
  const auto lines = linesOf(run.out);
  check(lines.size() == 1, "one write prints one line (FR-005)");
  const auto names = !lines.empty()
      && lines.front().find("GenuineIntel-6-C6-2") != std::string::npos
      && lines.front().find(recordPath(out).string()) != std::string::npos;
  check(names, "that line names the CPU type and the path it wrote (FR-005)");
  check(linesOf(run.err).empty(),
        "the successful write prints nothing on standard error (FR-005)");
  const auto text = sg::test::readText(recordPath(out).string());
  check(text.find(R"CPU("vendor":"GenuineIntel")CPU") != std::string::npos
            && text.find(R"DIR("directory":"arch/x86/arrowlake/")DIR")
                != std::string::npos,
        "the record carries the identity the seam answered (FR-007, FR-008)");
  check(text.find(R"MASK("mask":"0-2,12-13,24")MASK") != std::string::npos,
        "the record carries the machine facts the seam answered (FR-012, "
        "FR-032)");
}

// FR-006, SC-002: a write over a standing record speaks this writer's
// version, replaces the standing identity and machine block, keeps the
// standing rows row for row in the order the file holds them, and leaves one
// file for that CPU type.
auto scenarioRefresh() -> void
{
  const auto out = testRoot("refresh");
  const auto kept = evidenceArray({kRowPlacement, kRowPrecheck, kRowRoundTrip});
  sg::test::writeText(recordPath(out).string(), standingRecord(kept));
  const auto run = runWrite(writeArguments(out), plantedSeam());
  check(run.status == 0,
        "a write over a standing record answers status 0 (FR-006)");
  const auto text = sg::test::readText(recordPath(out).string());
  check(!text.empty(), "the refreshed record holds the written bytes (FR-006)");
  check(text.starts_with(R"({"recordVersion":"1.0")"),
        "the refresh speaks the version this writer speaks (FR-006, FR-019)");
  check(text.find("OldVendor") == std::string::npos
            && text.find("oldlake") == std::string::npos
            && text.find("\"family\":15") == std::string::npos,
        "the refresh replaces the standing identity (FR-006)");
  check(text.find("\"cores\":99") == std::string::npos
            && text.find(R"OLD("mask":"0-1")OLD") == std::string::npos,
        "the refresh replaces the standing machine block (FR-006)");
  if (evidenceArrayOf(text) != kept) {
    std::printf("  kept:  %.200s\n  stored:%.200s\n",
                evidenceArrayOf(text).c_str(),
                kept.c_str());
  }
  check(evidenceArrayOf(text) == kept,
        "the refresh keeps the standing rows in the order the file holds them "
        "(FR-006, SC-002)");
  check(entriesUnder(out).size() == 1,
        "one record stands for that CPU type after the refresh (FR-006, "
        "FR-009)");
}

// FR-015, FR-018, SC-007: three planted acceptance rows read back whole, each
// carrying its path, its command, its exit status and its cited line, the
// three standing in the order they were given, and the document they ride
// stays under the cap.
auto scenarioEvidenceRows() -> void
{
  const auto out = testRoot("evidence_rows");
  const auto given =
      evidenceArray({kRowPlacement, kRowPrecheck, kRowRoundTrip});
  sg::test::writeText(recordPath(out).string(), standingRecord(given));
  const auto run = runWrite(writeArguments(out), plantedSeam());
  check(
      run.status == 0,
      "the write over three standing rows answers status 0 (FR-005, " "FR-"
                                                                      "015)");
  const auto text = sg::test::readText(recordPath(out).string());
  check(!text.empty(),
        "the record the rows ride holds the written bytes (FR-015)");
  check(text.size() <= kCapBytes,
        "the document the three rows ride stays under the cap (FR-018)");
  auto before = std::size_t {0};
  for (const auto row : {kRowPlacement, kRowPrecheck, kRowRoundTrip}) {
    const auto at = text.find(std::string(row), before);
    check(at != std::string::npos,
          "one row reads back with its path, command, exit and line (FR-015)");
    check(at != std::string_view::npos && at > before,
          "the rows stand in the order they were given (FR-015, SC-007)");
    if (at != std::string::npos) {
      before = at + row.size();
    }
  }
}

// The seam of a refusal arm: it counts the asks, so an arm reads whether the
// parse reached the host before it refused, and it answers the planted host the
// other scenarios answer (FR-004, FR-032, FR-037).
auto countingSeam(std::size_t& asks) -> sg::detail::HostFactsSeam
{
  const auto facts = plantedFacts();
  return [&asks, facts](const std::filesystem::path&) -> sg::detail::HostFacts
  {
    ++asks;
    return facts;
  };
}

// The seam of an identity arm: the planted machine beside one identity with a
// single field left as the detection leaves it, so one arm names one cause
// (FR-010, FR-032).
auto brokenIdentSeam(const sg::detail::HostIdent& ident)
    -> sg::detail::HostFactsSeam
{
  auto facts = sg::detail::HostFacts {};
  facts.ident = ident;
  facts.layout = plantedLayout(true);
  return [facts](const std::filesystem::path&) -> sg::detail::HostFacts
  { return facts; };
}

// The identity of an arm: the planted host with one field returned to the
// value the header states for a field the host left unanswered (FR-010).
auto brokenIdent(const std::string& field) -> sg::detail::HostIdent
{
  auto ident = plantedIdent();
  if (field == "vendor") {
    ident.vendor.clear();
  } else if (field == "family") {
    ident.family = -1;
  } else if (field == "model") {
    ident.model = -1;
  } else if (field == "directory") {
    ident.directory.clear();
  }
  return ident;
}

// FR-004, FR-011, FR-037, SC-004: the three words the parse refuses, an option
// no arm accepts, an option that stands without its value, and a word that
// names no option. Each prints one line on standard error naming the word the
// parse read, answers status 2, asks the host seam nothing, and leaves the
// output root empty, because the parse stands ahead of the seam (plan R-09).
auto scenarioUsageArms() -> void
{
  const auto tree = plantedTree(testRoot("usage_tree"));
  const auto checkArm = [](const char* name,
                           const std::filesystem::path& out,
                           const std::vector<std::string>& arguments,
                           std::string_view token) -> void
  {
    auto asks = std::size_t {0};
    const auto run = runWrite(arguments, countingSeam(asks));
    const auto lines = linesOf(run.err);
    const auto entries = entriesUnder(out);
    if (run.status != 2 || lines.size() != 1 || asks != 0 || !entries.empty()) {
      std::printf("  %s: status=%d lines=%zu asks=%zu entries=%zu\n",
                  name,
                  run.status,
                  lines.size(),
                  asks,
                  entries.size());
      for (const auto& line : lines) {
        std::printf("    line: %s\n", line.c_str());
      }
    }
    check(run.status == 2,
          "an unusable argument answers status 2 (FR-004, SC-004)");
    check(lines.size() == 1, "an unusable argument prints one line (FR-004)");
    check(!lines.empty() && lines.front().find(token) != std::string::npos,
          "that line names the word the parse read (FR-004)");
    check(asks == 0,
          "the refusal reads no host, the parse stands ahead of the seam "
          "(FR-004, FR-037)");
    check(entries.empty(),
          "the refusal leaves the output root empty (FR-011, SC-004)");
  };
  const auto unknown = testRoot("usage_unknown");
  checkArm(
      "unknown option",
      unknown,
      {"--root", tree.string(), "--out", unknown.string(), "--bogus", "value"},
      "--bogus");
  const auto missing = testRoot("usage_missing");
  checkArm("option without its value",
           missing,
           {"--root", tree.string(), "--out", missing.string(), "--root"},
           "--root");
  const auto extra = testRoot("usage_extra");
  checkArm("word that names no option",
           extra,
           {"--root", tree.string(), "--out", extra.string(), "extra"},
           "extra");
}

// FR-010, SC-004: an identity the record cannot name, one cause per arm. An
// empty vendor, an unset family, an unset model and an identity whose directory
// stands empty each print one line naming the identity the run read, answer
// status 2 behind the one seam ask, and leave the output root empty.
auto scenarioIdentityRefusals() -> void
{
  for (const auto& field : {"vendor", "family", "model", "directory"}) {
    const auto out = testRoot(std::string("identity_") + field);
    const auto ident = brokenIdent(field);
    const auto run = runWrite(writeArguments(out), brokenIdentSeam(ident));
    const auto lines = linesOf(run.err);
    const auto entries = entriesUnder(out);
    if (run.status != 2 || lines.size() != 1 || !entries.empty()) {
      std::printf("  empty %s: status=%d lines=%zu entries=%zu\n",
                  field,
                  run.status,
                  lines.size(),
                  entries.size());
      for (const auto& line : lines) {
        std::printf("    line: %s\n", line.c_str());
      }
    }
    check(run.status == 2,
          "an identity the record cannot name answers status 2 (FR-010, "
          "SC-004)");
    check(lines.size() == 1, "the identity refusal prints one line (FR-010)");
    check(!lines.empty() && lines.front().find(ident.key) != std::string::npos,
          "that line names the identity the run read (FR-010)");
    check(
        entries.empty(),
        "the identity refusal leaves the output root empty (FR-011, " "SC-"
                                                                      "004)");
  }
}

// FR-002, FR-037: a run that names no `--out` resolves the default output root
// of FR-002 under the tree root it read, the directory the `validate` target
// owns and creates, and stands one record there.
//
// Characterization (T038): the write path grew this default in `eb3f77e`, the
// task that grew the standing read, so all three checks answered green the
// moment they stood written and they enter the suite with no red run of their
// own. They hold the resolution rule of FR-002, which no other unit reads.
auto scenarioDefaultOutRoot() -> void
{
  const auto tree = plantedTree(testRoot("default_out"));
  std::filesystem::create_directories(tree / "validation");
  auto asks = std::size_t {0};
  const auto run = runWrite({"--root", tree.string()}, countingSeam(asks));
  const auto written = tree / "validation";
  const auto entries = entriesUnder(written);
  if (run.status != 0
      || entries != std::vector<std::string> {"GenuineIntel-6-C6-2.json"})
  {
    std::printf("  no --out: status=%d entries=%zu asks=%zu\n",
                run.status,
                entries.size(),
                asks);
  }
  check(run.status == 0,
        "a run that names no output root answers status 0 (FR-002)");
  check(entries == std::vector<std::string> {"GenuineIntel-6-C6-2.json"},
        "the default root under the tree root holds one record (FR-002, "
        "FR-009)");
  check(asks == 1,
        "the seam answers once, for the tree root the parse read (FR-008)");
}

// FR-010: the catalog directory the mapfile selects stands under the catalog
// the pinned rows read from, so a record whose `directory` names a path the
// tree does not hold cites events no reader can open and no hunter can place.
// The reader never compares that field, so the write path stands alone in
// catching it: one line naming the identity and the directory that identity
// names, status 2, and no record in the output root (FR-010, SC-004).
auto scenarioMissingCatalog() -> void
{
  const auto tree = plantedTree(testRoot("missing_catalog"));
  std::filesystem::remove_all(tree / "external");
  const auto out = testRoot("missing_catalog_out");
  const auto run =
      runWrite({"--root", tree.string(), "--out", out.string()}, plantedSeam());
  const auto lines = linesOf(run.err);
  const auto entries = entriesUnder(out);
  if (run.status != 2 || lines.size() != 1 || !entries.empty()) {
    std::printf("  missing catalog: status=%d lines=%zu entries=%zu\n",
                run.status,
                lines.size(),
                entries.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(run.status == 2,
        "a directory the tree does not hold answers status 2 (FR-010, SC-004)");
  check(lines.size() == 1, "that refusal prints one line (FR-010)");
  check(!lines.empty()
            && lines.front().find("GenuineIntel-6-C6-2") != std::string::npos
            && lines.front().find("arch/x86/arrowlake/") != std::string::npos,
        "that line names the identity and the directory it names (FR-010)");
  check(entries.empty(),
        "the refused run leaves the output root empty (FR-010, SC-004)");

  // The same identity and the same tree, with the directory that tree was
  // missing stood back in place: the run answers status 0 and writes its
  // record, so the refusal above reads the one path this rule composes and
  // nothing else about the tree.
  //
  // Characterization (`48e4b46`): these two checks answered green the moment
  // they stood written beside the rule they hold, and they enter the suite
  // with no red run of their own.
  std::filesystem::create_directories(tree / "external" / "pmu-events"
                                      / plantedIdent().directory);
  const auto heldOut = testRoot("missing_catalog_held");
  const auto held = runWrite(
      {"--root", tree.string(), "--out", heldOut.string()}, plantedSeam());
  const auto heldEntries = entriesUnder(heldOut);
  if (held.status != 0
      || heldEntries != std::vector<std::string> {"GenuineIntel-6-C6-2.json"})
  {
    std::printf("  held catalog: status=%d entries=%zu\n",
                held.status,
                heldEntries.size());
    for (const auto& line : linesOf(held.err)) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(held.status == 0,
        "a directory the tree holds answers status 0 (FR-005, FR-008)");
  check(heldEntries == std::vector<std::string> {"GenuineIntel-6-C6-2.json"},
        "the run writes the record beside a catalog it can read (FR-003, "
        "FR-009)");
}

// The seam of a tree whose mapfile answered no rows the matcher could weigh:
// the fixture identity, whose directory stands whole, beside the answer the
// writer program gives when the file stands absent, stands unreadable, or holds
// a row whose pattern the matcher cannot compile. The identity is sound here,
// so the only failure the run meets is the file (FR-004, FR-008).
auto unreadableMapfileSeam() -> sg::detail::HostFactsSeam
{
  auto facts = plantedFacts();
  facts.mapfileReadable = false;
  return [facts](const std::filesystem::path&) -> sg::detail::HostFacts
  { return facts; };
}

// FR-007, FR-013, FR-020: a host that answers no leaf 1 and reads no memory
// writes two `null` spellings, the form the schema states beside those two keys
// and the JSON cousin of the standing `node: not available` line. Both answers
// stand in one document here, so the arm reads the bytes the run wrote and asks
// each key its own answer.
auto scenarioNullFields() -> void
{
  const auto out = testRoot("null_fields");
  const auto run = runWrite(writeArguments(out), absentFieldsSeam());
  const auto text = sg::test::readText(recordPath(out).string());
  if (run.status != 0 || text.empty()) {
    std::printf(
        "  null fields: status=%d bytes=%zu\n", run.status, text.size());
  }
  check(run.status == 0,
        "the host that answers two fields by nothing still writes its record "
        "(FR-007, FR-013)");
  check(
      text.find(R"("stepping":null)") != std::string::npos,
      "leaf 1 stands null beside its key, the one spelling the schema states "
      "(FR-007, FR-020)");
  check(text.find(R"("nodes":null)") != std::string::npos,
        "the memory read stands null beside its key (FR-013, FR-020)");
}

// The ceiling one process may write to a file, held under the length of the
// document this arm writes.
constexpr std::size_t kUnwritableCeiling = 1024U;

// The seam of a host whose document runs long: a vendor text of eight
// kilobytes, the key and the directory of the fixture, so the document stands
// under the cap of FR-018 and reaches the one write (FR-011, FR-018).
auto longDocumentSeam() -> sg::detail::HostFactsSeam
{
  return [](const std::filesystem::path&)
  {
    auto facts = plantedFacts();
    facts.ident.vendor = std::string(8U * 1024U, 'x');
    return facts;
  };
}

// FR-011, SC-004: a record file the run opened and could not write ends the run
// with the line naming that file. `RLIMIT_FSIZE` answers the kernel's own
// refusal, a write that fails while the file stands open, and the arm lowers
// the ceiling of its own process, then restores that ceiling and the signal the
// kernel sends beside it. The refusal line runs to about 200 bytes, so it stays
// under the ceiling the arm sets and reaches the stream.
auto scenarioUnwrittenRecord() -> void
{
  const auto out = testRoot("unwritten");
  // The standing record this run means to replace: its bytes answer a record of
  // its own, so the arm can ask whether a write the disk refuses reaches them
  // (FR-006, FR-011).
  const auto kept =
      standingRecord(evidenceArray({kRowPlacement, kRowPrecheck}));
  sg::test::writeText(recordPath(out).string(), kept);
  // The arguments compose before the ceiling stands lowered: the planted tree
  // writes a schema file of its own, and a ceiling held over that write would
  // shorten the schema along with the record.
  const auto arguments = writeArguments(out);
  auto ceiling = rlimit {};
  if (getrlimit(RLIMIT_FSIZE, &ceiling) != 0) {
    fail("the file-size ceiling reads");
  }
  const auto stood = ceiling.rlim_cur;
  const auto sender = std::signal(SIGXFSZ, SIG_IGN);
  ceiling.rlim_cur = static_cast<rlim_t>(kUnwritableCeiling);
  if (setrlimit(RLIMIT_FSIZE, &ceiling) != 0) {
    fail("the file-size ceiling lowers");
  }
  const auto run = runWrite(arguments, longDocumentSeam());
  ceiling.rlim_cur = stood;
  if (setrlimit(RLIMIT_FSIZE, &ceiling) != 0) {
    fail("the file-size ceiling stands restored");
  }
  (void)std::signal(SIGXFSZ, sender);
  const auto lines = linesOf(run.err);
  const auto bytes = sg::test::readText(recordPath(out).string());
  if (run.status != 2 || lines.size() != 1 || bytes != kept) {
    std::printf("  unwritten record: status=%d lines=%zu bytes=%zu of %zu\n",
                run.status,
                lines.size(),
                bytes.size(),
                kept.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(run.status == 2,
        "a record the write refuses after opening answers status 2 (FR-011, "
        "SC-004)");
  check(lines.size() == 1, "the failed write prints one line (FR-011)");
  check(
      !lines.empty()
          && lines.front().find(recordPath(out).string()) != std::string::npos,
      "that line names the file the write failed on (FR-011)");
  check(!lines.empty()
            && lines.front().find("could not be written") != std::string::npos,
        "that line speaks the write, the open having answered (FR-011)");
  check(sg::test::readText(recordPath(out).string()) == kept,
        "the standing record keeps the bytes it held behind the failed write "
        "(FR-006, FR-011)");
}

// FR-006, FR-011, FR-020: a standing record whose bytes answer no record keeps
// those bytes and ends the run. The file reads whole here, so the failure
// stands with the reader of record JSON, two levels past the open, and this arm
// plants bytes that parse as nothing.
auto scenarioInvalidStandingRecord() -> void
{
  const auto out = testRoot("invalid_standing");
  const auto kept = std::string {"{ \"version\": 1, \"cpu\": no record }\n"};
  sg::test::writeText(recordPath(out).string(), kept);
  const auto run = runWrite(writeArguments(out), plantedSeam());
  const auto lines = linesOf(run.err);
  const auto bytes = sg::test::readText(recordPath(out).string());
  if (run.status != 2 || lines.size() != 1 || bytes != kept) {
    std::printf("  record of no record: status=%d lines=%zu bytes=%zu\n",
                run.status,
                lines.size(),
                bytes.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(run.status == 2,
        "a standing record that answers no record answers status 2 (FR-011, "
        "SC-004)");
  check(lines.size() == 1,
        "the standing read that answers no record prints one line (FR-011)");
  check(
      !lines.empty()
          && lines.front().find(recordPath(out).string()) != std::string::npos,
      "that line names the record the run read past (FR-011)");
  check(bytes == kept,
        "the bytes that answered no record stand kept (FR-006, FR-011)");
  check(entriesUnder(out).size() == 1,
        "the run leaves one file, the record it read (FR-011, SC-004)");
}

// FR-006, FR-011, SC-004: a record name that stands a directory answers the
// read with nothing, the state question with no error and the answer that no
// record stands there, so the run writes its document and tries to move it onto
// that name. The move is what fails, and the arm asks the refusal to name the
// record, to speak the write, to leave the landing name behind nowhere, and to
// leave the bytes inside the directory as they stand. A user hits this shape by
// making a directory where the record of a CPU type belongs.
auto scenarioRecordNameStandsADirectory() -> void
{
  const auto out = testRoot("record_stands_dir");
  const auto held = std::string {"bytes the run must leave alone\n"};
  const auto inner = recordPath(out) / "held.json";
  std::filesystem::create_directories(inner.parent_path());
  sg::test::writeText(inner.string(), held);
  const auto run = runWrite(writeArguments(out), plantedSeam());
  const auto lines = linesOf(run.err);
  const auto entries = entriesUnder(out);
  if (run.status != 2 || lines.size() != 1 || entries.size() != 1
      || sg::test::readText(inner.string()) != held)
  {
    std::printf(
        "  record name of a directory: status=%d lines=%zu " "entries=%zu\n",
        run.status,
        lines.size(),
        entries.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(run.status == 2,
        "a record name that stands a directory answers status 2 (FR-011, "
        "SC-004)");
  check(lines.size() == 1, "that refusal prints one line (FR-011)");
  check(
      !lines.empty()
          && lines.front().find(recordPath(out).string()) != std::string::npos,
      "that line names the record the move failed on (FR-011)");
  check(!lines.empty()
            && lines.front().find("could not be written") != std::string::npos,
        "that line speaks the write, the landing having been written (FR-011)");
  check(entries.size() == 1 && entries.front() == "GenuineIntel-6-C6-2.json",
        "the landing name stands removed and one entry stays for the record "
        "(FR-009, SC-004)");
  check(sg::test::readText(inner.string()) == held,
        "the bytes inside the directory stand as the run left them (FR-011)");
}

// FR-006, FR-011: an output root that stands a regular file, the way a
// mistyped path lands, answers no state at all about the record name under it.
// `stat` refuses every name under that file with `Not a directory`, so the
// standing record answers nothing by the operating system's own refusal, the
// state a fresh record starts from, and the run meets its refusal at the write.
// The read that fails is asked of the file itself, and the state question that
// follows comes back empty: the operating system holds no state about a name
// under a file.
auto scenarioOutRootStandsAFile() -> void
{
  const auto dir = testRoot("out_stands_file");
  const auto tree = plantedTree(dir);
  const auto out = dir / "record_root";
  const auto stood = std::string {"a file where a directory belongs\n"};
  sg::test::writeText(out.string(), stood);
  const auto run =
      runWrite({"--root", tree.string(), "--out", out.string()}, plantedSeam());
  const auto path = out / "GenuineIntel-6-C6-2.json";
  const auto lines = linesOf(run.err);
  if (run.status != 2 || lines.size() != 1
      || sg::test::readText(out.string()) != stood)
  {
    std::printf("  output root of a file: status=%d lines=%zu\n",
                run.status,
                lines.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(run.status == 2,
        "an output root that stands a file answers status 2 (FR-011, SC-004)");
  check(lines.size() == 1, "that refusal prints one line (FR-011)");
  check(
      !lines.empty() && lines.front().find(path.string()) != std::string::npos,
      "that line names the record the run could not reach (FR-011)");
  check(sg::test::readText(out.string()) == stood,
        "the file the run took for a root holds the bytes it stood with "
        "(FR-011, SC-004)");
}

// FR-004, FR-008: a mapfile the run cannot read is a failure of the file, and
// the sentence the run answers names it, the way the reader names the mapfile
// it cannot open. "No row matched this identity" is a failure of the identity
// and keeps the identity line of FR-010; one sentence serving both sends a
// developer to the wrong place, so the two answers stand apart here. The
// refused run writes no record (SC-004).
auto scenarioUnreadableMapfile() -> void
{
  const auto tree = plantedTree(testRoot("unreadable_mapfile"));
  const auto out = testRoot("unreadable_mapfile_out");
  const auto mapfile =
      tree / "external" / "pmu-events" / "arch" / "x86" / "mapfile.csv";
  const auto run = runWrite({"--root", tree.string(), "--out", out.string()},
                            unreadableMapfileSeam());
  const auto lines = linesOf(run.err);
  const auto entries = entriesUnder(out);
  if (run.status != 2 || lines.size() != 1 || !entries.empty()) {
    std::printf("  unreadable mapfile: status=%d lines=%zu entries=%zu\n",
                run.status,
                lines.size(),
                entries.size());
    for (const auto& line : lines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(run.status == 2,
        "a mapfile the run cannot read answers status 2 (FR-004, SC-004)");
  check(lines.size() == 1, "that refusal prints one line (FR-004)");
  check(!lines.empty()
            && lines.front().find(mapfile.string()) != std::string::npos,
        "that line names the mapfile the run could not read (FR-004, FR-008)");
  check(entries.empty(),
        "the refused run leaves the output root empty (FR-004, SC-004)");
}

// The seam of the failed layout read: the planted identity beside the layout
// the standing read answers, which stands unreadable while the load failure
// stands installed, the shape the writer program hands its own seam (FR-011,
// FR-032).
auto hostLayoutSeam() -> sg::detail::HostFactsSeam
{
  return [](const std::filesystem::path&) -> sg::detail::HostFacts
  {
    auto facts = sg::detail::HostFacts {};
    facts.ident = plantedIdent();
    facts.layout = sg::detail::layout();
    return facts;
  };
}

// FR-011, SC-004: the refusals of a write that cannot finish. The layout read
// answers its refusal in a process of its own, a landing name that stands a
// directory refuses the open here, and the standing record that answers no
// bytes runs beside them under a ceiling of its own (`childStandingCeiling`).
// Every plant here binds uid 0 the way it binds the lane user, because the CI
// container runs this entry as root, where the permission bits ask no refusal.
auto scenarioWriteRefusals() -> void
{
  // FR-011: a machine layout that reads unreadable answers one line naming the
  // read, status 2 and no file, the shape the standing listing answers for its
  // own read (`source/harness/cli.cpp:671`). The run answers in this process,
  // ahead of the machine block that reads a layout it must be able to trust.
  sg::detail::installTestLoadFailure(1);
  const auto failed = testRoot("load_failure");
  const auto load = runWrite(writeArguments(failed), hostLayoutSeam());
  sg::detail::installTestLoadFailure(std::nullopt);
  const auto loadLines = linesOf(load.err);
  const auto loadEntries = entriesUnder(failed);
  if (load.status != 2 || loadLines.size() != 1 || !loadEntries.empty()) {
    std::printf("  failed load: status=%d lines=%zu entries=%zu\n",
                load.status,
                loadLines.size(),
                loadEntries.size());
    for (const auto& line : loadLines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(load.status == 2,
        "a layout that reads unreadable answers status 2 (FR-011, SC-004)");
  check(loadLines.size() == 1,
        "the failed layout read prints one line (FR-011)");
  check(!loadLines.empty()
            && loadLines.front().find("layout") != std::string::npos,
        "that line names the read that failed (FR-011)");
  check(loadEntries.empty(),
        "the failed layout read leaves the output root empty (FR-011, SC-004)");

  // FR-011: an output root that refuses the write prints one line naming the
  // file the write failed on, answers status 2, and leaves no record behind.
  //
  // The plant is a directory standing at the landing name FR-009 writes its
  // bytes under. `open` answers `Is a directory` for that name to every user,
  // the root of the CI container included. A mode reaches nobody there: uid 0
  // asks the permission bits no refusal at all, so a root reads a read-only
  // root as a directory it may write.
  //
  // Characterization (T039): `writeRecord` grew the refusal line in `4289d9c`,
  // so the checks of this arm answered green the moment they stood written and
  // stand with no red run of their own.
  const auto blocked = testRoot("landing_stands_dir");
  std::filesystem::create_directories(landingPath(blocked));
  const auto refused = runWrite(writeArguments(blocked), plantedSeam());
  const auto refusedLines = linesOf(refused.err);
  const auto entries = entriesUnder(blocked);
  if (refused.status != 2 || refusedLines.size() != 1 || entries.size() != 1) {
    std::printf("  landing of a directory: status=%d lines=%zu entries=%zu\n",
                refused.status,
                refusedLines.size(),
                entries.size());
    for (const auto& line : refusedLines) {
      std::printf("    line: %s\n", line.c_str());
    }
  }
  check(refused.status == 2,
        "a write the output root refuses answers status 2 (FR-011, SC-004)");
  check(refusedLines.size() == 1, "the failed write prints one line (FR-011)");
  check(!refusedLines.empty()
            && refusedLines.front().find(recordPath(blocked).string())
                != std::string::npos,
        "that line names the file the write failed on (FR-011)");
  check(
      !refusedLines.empty()
          && refusedLines.front().find("cannot be opened") != std::string::npos,
      "that line speaks the open the standing directory refused (FR-011)");
  const auto landingName = landingPath(blocked).filename().string();
  check(entries.size() == 1 && entries.front() == landingName
            && !std::filesystem::exists(recordPath(blocked)),
        "the run leaves the entry it found standing, adds nothing beside it, "
        "and writes no record (FR-011, SC-004)");
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc > 1) {
    const std::string_view mode {argv[1]};
    if (mode == "machineBlock") {
      return childMachineBlock();
    }
    if (mode == "noMemory") {
      return childNoMemory();
    }
    if (mode == "standingCeiling") {
      return childStandingCeiling();
    }
    std::fprintf(stderr, "unknown child mode\n");
    return 1;
  }

  const int blockStatus = runChild(argv[0], "machineBlock");
  check(WIFEXITED(blockStatus) && WEXITSTATUS(blockStatus) == 0,
        "the machine-block scenario ended clean (FR-012, FR-013, FR-014, "
        "FR-032)");

  const int memoryStatus = runChild(argv[0], "noMemory");
  check(WIFEXITED(memoryStatus) && WEXITSTATUS(memoryStatus) == 0,
        "the unreadable-memory scenario ended clean (FR-013)");

  // FR-011, SC-004: the standing record that answers no bytes, read under a
  // ceiling on this process's descriptors (`childStandingCeiling`).
  const int ceilingStatus = runChild(argv[0], "standingCeiling");
  check(WIFEXITED(ceilingStatus) && WEXITSTATUS(ceilingStatus) == 0,
        "the standing record no read reaches ended clean (FR-011, SC-004)");

  scenarioKeyOrder();
  scenarioRepeatBytes();
  scenarioPlainEnd();
  scenarioSizeCap();
  scenarioOneWrite();
  scenarioRefresh();
  scenarioEvidenceRows();
  scenarioUsageArms();
  scenarioIdentityRefusals();
  scenarioDefaultOutRoot();
  scenarioMissingCatalog();
  scenarioUnreadableMapfile();
  scenarioOutRootStandsAFile();
  scenarioRecordNameStandsADirectory();
  scenarioNullFields();
  scenarioUnwrittenRecord();
  scenarioInvalidStandingRecord();
  scenarioWriteRefusals();

  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
