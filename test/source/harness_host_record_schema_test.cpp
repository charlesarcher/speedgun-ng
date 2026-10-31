// ============================================================================
// T002: the pinned record schema guards the host record
// (specs/031-host-validation; FR-019, FR-020, FR-021, SC-005; wave 1, M-A).
//
// The oracle is `sg::detail::checkReport` over the file the
// `SG_SCHEMA_PATH` definition names, the way `harness_json_schema_test`
// reads its schema. The planted document holds the keys, the order and the
// kinds `contracts/host-record-json.md` pins, and it passes the guard: the
// guard walks a document in the order the document spells, so a schema that
// named those keys in another order would refuse it. Each plant breaks one
// rule of `contracts/record-schema-file.md` in one named object, and the
// check names the place and the rule the refusal carries, so a green run
// cannot come from a guard that inspects nothing. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "detail/host_record.hpp"
#include "detail/host_record_check.hpp"
#include "detail/internal.hpp"
#include "detail/json_fixture.hpp"

using sg::test::check;

namespace
{
#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH"
#endif

// The document kind the schema file carries and the format version the
// record names (FR-019).
constexpr std::string_view kKindLabel = "host-record";
constexpr std::string_view kFormatVersion = "1.0";

// The four root blocks, spelled as the fixture of
// `contracts/host-record-json.md` spells them: the identity, the figures and
// the one evidence row the writer tests plant, in the order that contract
// calls normative.
constexpr std::string_view kVersionPair = R"ROOT("recordVersion":"1.0")ROOT";
constexpr std::string_view kCpuBlock =
    R"CPU("cpu":{"vendor":"GenuineIntel","family":6,"model":198,"stepping":2,"key":"GenuineIntel-6-C6-2","directory":"arch/x86/arrowlake/"})CPU";
constexpr std::string_view kMachineBlock =
    R"MACHINE("machine":{"packages":1,"l3Groups":2,"cores":20,"threads":20,"mask":"0-19","l3":[{"ordinal":0,"cpus":"0-7"},{"ordinal":1,"cpus":"8-19"}],"nodes":[{"number":0,"cpus":"0-19"}]})MACHINE";
constexpr std::string_view kEvidenceBlock =
    R"EVIDENCE("evidence":[{"path":"specs/034-host-noise-precheck/results/quickstart.md","command":"ctest --test-dir build -R harness_precheck_test --output-on-failure","exit":0,"line":"1/1 Test #1: harness_precheck_test ... Passed"}])EVIDENCE";

// The one acceptance row the round trip plants: the four fields FR-015 states
// for a row, citing a run the repository records (FR-015).
constexpr std::string_view kRowPath =
    "specs/035-numa-placement/results/nps1.md";
constexpr std::string_view kRowCommand =
    "ctest --test-dir build -R harness_numa_placement_test";
constexpr std::string_view kRowLine =
    "$ ctest --test-dir build -R harness_numa_placement_test";

// The text that opens each object of the document, so a plant names the
// object it breaks: a closed object, a `required` list and a kind belong to
// one object each.
constexpr std::string_view kRootOpen = "{";
constexpr std::string_view kCpuOpen = R"CPU("cpu":{)CPU";
constexpr std::string_view kMachineOpen = R"MACHINE("machine":{)MACHINE";
constexpr std::string_view kL3Open = R"L3("l3":[{)L3";
constexpr std::string_view kNodesOpen = R"NODES("nodes":[{)NODES";
constexpr std::string_view kEvidenceOpen = R"EVIDENCE("evidence":[{)EVIDENCE";

// The document one host writes: the four root blocks in the order the
// contract's table lists them, compact, the document alone.
auto pinnedRecord() -> std::string
{
  auto record = std::string("{");
  record += kVersionPair;
  record += ',';
  record += kCpuBlock;
  record += ',';
  record += kMachineBlock;
  record += ',';
  record += kEvidenceBlock;
  record += '}';
  return record;
}

// The byte a value ends at, one past its last byte: a string closes at its
// own quote, and a bracketed value closes at the separator that leaves no
// bracket open behind it.
auto valueEndOf(std::string_view document, std::size_t from) -> std::size_t
{
  if (document[from] == '"') {
    return document.find('"', from + 1) + 1;
  }
  std::size_t depth = 0;
  for (auto at = from; at < document.size(); ++at) {
    const char letter = document[at];
    if (letter == '"') {
      const auto close = document.find('"', at + 1);
      at = close == std::string_view::npos ? document.size() : close;
    } else if (letter == '{' || letter == '[') {
      ++depth;
    } else if (depth > 0) {
      if (letter == '}' || letter == ']') {
        --depth;
      }
    } else if (letter == ',' || letter == '}' || letter == ']') {
      return at;
    }
  }
  return document.size();
}

// Where one key's pair stands: the byte the key opens at, the byte its value
// opens at and the byte its value ends at. `inside` opens the object that
// holds the key, so a key one row shares with another row still names its
// own place. An answer of `npos` names a document that holds no such pair.
struct Pair
{
  std::size_t key = std::string_view::npos;
  std::size_t value = std::string_view::npos;
  std::size_t end = std::string_view::npos;
};

auto pairOf(std::string_view document,
            std::string_view inside,
            std::string_view key) -> Pair
{
  const auto object = document.find(inside);
  if (object == std::string_view::npos) {
    return {};
  }
  const std::string needle = "\"" + std::string(key) + "\":";
  const auto at = document.find(needle, object);
  if (at == std::string_view::npos) {
    return {};
  }
  return Pair {
      .key = at,
      .value = at + needle.size(),
      .end = valueEndOf(document, at + needle.size()),
  };
}

// The document with one key's value spelled another way.
auto withValueOf(std::string_view document,
                 std::string_view inside,
                 std::string_view key,
                 std::string_view value) -> std::string
{
  const Pair pair = pairOf(document, inside, key);
  if (pair.key == std::string_view::npos) {
    return {};
  }
  std::string planted(document);
  planted.replace(pair.value, pair.end - pair.value, value);
  return planted;
}

// The document with one key cut out of its object: the comma that stands
// beside the pair goes with it, so what stands behind reads as JSON.
auto withoutKey(std::string_view document,
                std::string_view inside,
                std::string_view key) -> std::string
{
  const Pair pair = pairOf(document, inside, key);
  if (pair.key == std::string_view::npos) {
    return {};
  }
  auto from = pair.key;
  auto end = pair.end;
  if (document[pair.key - 1] == '{') {
    if (end < document.size() && document[end] == ',') {
      ++end;
    }
  } else {
    from = document.rfind(',', pair.key);
  }
  if (from == std::string_view::npos) {
    return {};
  }
  std::string planted(document);
  planted.erase(from, end - from);
  return planted;
}

// The document with one key no object lists, planted inside the object
// `opening` names.
auto withKeyAdded(std::string_view opening) -> std::string
{
  const std::string record = pinnedRecord();
  const auto at = record.find(opening);
  if (at == std::string_view::npos) {
    return {};
  }
  std::string planted = record;
  planted.insert(at + opening.size(), R"ROOT("extra":true,)ROOT");
  return planted;
}

// The rule one refusal has to quote, beside the place it names.
auto refusesAt(const std::string& error,
               std::string_view place,
               std::string_view rule) -> bool
{
  return error.find(place) != std::string::npos
      && error.find(rule) != std::string::npos;
}

// The plant and the refusal, for the record a failed check leaves behind.
auto report(const std::string& planted, const std::string& error) -> void
{
  std::printf("  %.60s... refused with: %s\n", planted.c_str(), error.c_str());
}

// One object the guard reaches by name: the key one plant cuts, and the
// pointer the refusal names.
struct MissingPlant
{
  std::string_view inside;
  const char* key;
  const char* pointer;
};

// One object the guard reaches by name: the text that opens it, and the
// pointer a key outside its set reaches.
struct ClosedPlant
{
  std::string_view opening;
  const char* pointer;
};

// One key of the document: the object that holds it, the key, a value of a
// kind the schema does not state for it, and the pointer the refusal names.
struct KindPlant
{
  std::string_view inside;
  const char* key;
  const char* value;
  const char* pointer;
};

// The pinned document passes the guard, and the file names the kind and the
// version (FR-019, FR-020, FR-021).
auto thePinnedDocumentPassesTheGuard() -> void
{
  const std::string record = pinnedRecord();
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, record);
  if (!answer.has_value()) {
    report(record, answer.error());
  }
  check(answer.has_value(),
        "the document the contract pins holds no disagreement with the "
        "pinned schema (FR-020, FR-021)");

  const std::string schema = sg::test::readText(SG_SCHEMA_PATH);
  check(!schema.empty(), "the pinned schema file stands readable (FR-020)");
  check(schema.find(kKindLabel) != std::string::npos,
        "the schema file names the kind the record speaks (FR-019)");
  check(schema.find(kFormatVersion) != std::string::npos,
        "the schema file names the version the record carries (FR-019)");
}

// Each of the six objects that carry keys states its own `required` array:
// a document that leaves one key of one object out stands refused there
// (FR-020, FR-021).
auto everyObjectRequiresItsOwnKeys() -> void
{
  for (const MissingPlant& plant : {
           MissingPlant {kRootOpen, "recordVersion", "the document root"},
           MissingPlant {kCpuOpen, "vendor", "/cpu"},
           MissingPlant {kMachineOpen, "packages", "/machine"},
           MissingPlant {kL3Open, "ordinal", "/machine/l3/0"},
           MissingPlant {kNodesOpen, "number", "/machine/nodes/0"},
           MissingPlant {kEvidenceOpen, "exit", "/evidence/0"},
       })
  {
    const std::string planted =
        withoutKey(pinnedRecord(), plant.inside, plant.key);
    check(!planted.empty(), "the plant cut a pair the document holds");
    if (planted.empty()) {
      continue;
    }
    const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, planted);
    check(!answer.has_value(),
          "a document missing one key of one object stands refused (FR-021)");
    if (!answer.has_value()
        && !(refusesAt(answer.error(), plant.pointer, "stands missing")
             && refusesAt(answer.error(), plant.key, "stands missing")))
    {
      report(planted, answer.error());
      sg::test::fail("the refusal names no object and no key (FR-021)");
    }
  }
}

// Every object stands closed: a key beyond the set the schema lists stands
// refused inside the object that holds it (FR-020).
auto everyObjectStandsClosed() -> void
{
  for (const ClosedPlant& plant : {
           ClosedPlant {kRootOpen, "the document root/extra"},
           ClosedPlant {kCpuOpen, "/cpu/extra"},
           ClosedPlant {kMachineOpen, "/machine/extra"},
           ClosedPlant {kL3Open, "/machine/l3/0/extra"},
           ClosedPlant {kNodesOpen, "/machine/nodes/0/extra"},
           ClosedPlant {kEvidenceOpen, "/evidence/0/extra"},
       })
  {
    const std::string planted = withKeyAdded(plant.opening);
    check(!planted.empty(), "the plant found the object it aims at");
    if (planted.empty()) {
      continue;
    }
    const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, planted);
    check(!answer.has_value(),
          "a key beyond the set the schema lists stands refused (FR-020)");
    if (!answer.has_value()
        && !refusesAt(answer.error(), plant.pointer, "lists no key"))
    {
      report(planted, answer.error());
      sg::test::fail("the refusal names no key the object lists (FR-020)");
    }
  }
}

// The order the schema states is normative: two root keys that swap places
// stand refused (FR-021).
auto twoRootKeysOutOfOrderStandRefused() -> void
{
  const std::string cut =
      withoutKey(pinnedRecord(), kRootOpen, "recordVersion");
  check(!cut.empty(), "the order plant cut the first root key");
  if (cut.empty()) {
    return;
  }
  const auto at = cut.find(kMachineOpen);
  check(at != std::string::npos, "the order plant found the key it moves to");
  if (at == std::string::npos) {
    return;
  }
  std::string planted = cut;
  planted.insert(at, std::string(kVersionPair) + ",");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, planted);
  check(!answer.has_value(),
        "a document whose two root keys swap places stands refused (FR-021)");
  if (!answer.has_value()
      && !refusesAt(answer.error(), "/recordVersion", "out of the order"))
  {
    report(planted, answer.error());
    sg::test::fail("the refusal names no key out of its order (FR-021)");
  }
}

// The schema states one kind per key, with `null` beside the kind of the two
// unanswered keys alone: a value of another kind stands refused at its own
// pointer (FR-020, FR-021).
auto everyKeyRefusesAValueOfAnotherKind() -> void
{
  for (const KindPlant& plant : {
           KindPlant {kRootOpen, "recordVersion", R"V(1)V", "/recordVersion"},
           KindPlant {
               kCpuOpen,
               "vendor",
               R"V(6)V",
               "/cpu/vendor",
           },
           KindPlant {kCpuOpen, "family", R"V("GenuineIntel")V", "/cpu/family"},
           KindPlant {kCpuOpen, "model", R"V("198")V", "/cpu/model"},
           KindPlant {kCpuOpen, "stepping", R"V("2")V", "/cpu/stepping"},
           KindPlant {kCpuOpen, "key", R"V(198)V", "/cpu/key"},
           KindPlant {kCpuOpen, "directory", R"V(2)V", "/cpu/directory"},
           KindPlant {
               kMachineOpen, "packages", R"V("1")V", "/machine/packages"},
           KindPlant {
               kMachineOpen, "l3Groups", R"V("2")V", "/machine/l3Groups"},
           KindPlant {kMachineOpen, "cores", R"V("20")V", "/machine/cores"},
           KindPlant {kMachineOpen, "threads", R"V("20")V", "/machine/threads"},
           KindPlant {kMachineOpen, "mask", R"V(null)V", "/machine/mask"},
           KindPlant {kMachineOpen, "l3", R"V({})V", "/machine/l3"},
           KindPlant {kMachineOpen, "nodes", R"V(0)V", "/machine/nodes"},
           KindPlant {kL3Open, "ordinal", R"V("0")V", "/machine/l3/0/ordinal"},
           KindPlant {kL3Open, "cpus", R"V(0)V", "/machine/l3/0/cpus"},
           KindPlant {
               kNodesOpen, "number", R"V("0")V", "/machine/nodes/0/number"},
           KindPlant {kNodesOpen, "cpus", R"V(0)V", "/machine/nodes/0/cpus"},
           KindPlant {kEvidenceOpen, "path", R"V(0)V", "/evidence/0/path"},
           KindPlant {
               kEvidenceOpen, "command", R"V(0)V", "/evidence/0/command"},
           KindPlant {kEvidenceOpen, "exit", R"V("0")V", "/evidence/0/exit"},
           KindPlant {kEvidenceOpen, "line", R"V(0)V", "/evidence/0/line"},
       })
  {
    const std::string planted =
        withValueOf(pinnedRecord(), plant.inside, plant.key, plant.value);
    check(!planted.empty(), "the plant found the key it retypes");
    if (planted.empty()) {
      continue;
    }
    const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, planted);
    check(!answer.has_value(),
          "a value of a kind the schema does not state stands refused "
          "(FR-020, FR-021)");
    if (!answer.has_value()
        && !refusesAt(answer.error(), plant.pointer, "the schema asks for"))
    {
      report(planted, answer.error());
      sg::test::fail("the refusal names no key holding another kind (FR-021)");
    }
  }
}

// A hand-edited document that carries one root key twice stands refused
// (FR-021).
auto oneRootKeyTwiceStandsRefused() -> void
{
  std::string planted = pinnedRecord();
  planted.insert(1, std::string(kVersionPair) + ",");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, planted);
  check(!answer.has_value(),
        "a document that carries one root key twice stands refused (FR-021)");
  if (!answer.has_value()
      && !refusesAt(answer.error(), "/recordVersion", "stands twice"))
  {
    report(planted, answer.error());
    sg::test::fail("the refusal names no key that stands twice (FR-021)");
  }
}

// An unanswered value stands `null` inside a closed object, so the stepping
// of a host that answers no leaf 1 and the nodes of a machine whose memory
// reads unavailable both pass (FR-013, FR-020).
auto theUnansweredValuesStandNull() -> void
{
  const std::string noStepping =
      withValueOf(pinnedRecord(), kCpuOpen, "stepping", "null");
  check(!noStepping.empty(), "the plant found the stepping pair");
  const auto stepping = sg::detail::checkReport(SG_SCHEMA_PATH, noStepping);
  if (!stepping.has_value()) {
    report(noStepping, stepping.error());
  }
  check(stepping.has_value(),
        "a record whose host answers no stepping passes the guard (FR-013)");

  const std::string noNodes =
      withValueOf(pinnedRecord(), kMachineOpen, "nodes", "null");
  check(!noNodes.empty(), "the plant found the nodes array");
  const auto nodes = sg::detail::checkReport(SG_SCHEMA_PATH, noNodes);
  if (!nodes.has_value()) {
    report(noNodes, nodes.error());
  }
  check(nodes.has_value(),
        "a record whose machine memory reads unavailable passes the guard "
        "(FR-013)");
}

// The bytes of the document travel a file and come back whole, and the guard
// reads the bytes that came back (FR-021, FR-023).
auto theBytesRoundTripThroughTheFile() -> void
{
  const std::string record = pinnedRecord();
  sg::test::writeText("harness_host_record_schema.json", record);
  const std::string readBack =
      sg::test::readText("harness_host_record_schema.json");
  check(readBack == record,
        "re-reading the written bytes yields the bytes the document spells "
        "(FR-023)");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, readBack);
  check(answer.has_value(),
        "the bytes read back hold no disagreement with the schema (FR-021)");
}

// The document a round trip starts from: the four root blocks above beside one
// acceptance row, the state a second write refreshes (FR-006, FR-015).
auto standingRecord() -> std::string
{
  auto row = std::string {R"ROW([{"path":")ROW"};
  row += kRowPath;
  row += R"ROW(","command":")ROW";
  row += kRowCommand;
  row += R"ROW(","exit":0,"line":")ROW";
  row += kRowLine;
  row += R"ROW("}])ROW";

  auto record = std::string {"{"};
  record += kVersionPair;
  record += ',';
  record += kCpuBlock;
  record += ',';
  record += kMachineBlock;
  record += ',';
  record += R"EVIDENCE("evidence":)EVIDENCE";
  record += row;
  record += '}';
  return record;
}

// The pinned schema, planted under one root the way a checkout carries it, so
// the write path can read a standing record against it and the round trip
// speaks the same schema as the record beside it (FR-020, FR-022).
auto plantSchema(const std::filesystem::path& root) -> void
{
  const auto target = root / "docs" / "schema" / "host-record-v1.json";
  std::filesystem::create_directories(target.parent_path());
  sg::test::writeText(target.string(), sg::test::readText(SG_SCHEMA_PATH));
}

// The catalog the planted identity names, stood under one root where the pinned
// rows read it: `cpu.directory` stands relative to the catalog level, three
// levels above the mapfile, and the writer refuses a run whose tree does not
// hold it, so a root this unit writes into stands as a checkout does (FR-008).
auto plantCatalog(const std::filesystem::path& root) -> void
{
  std::filesystem::create_directories(root / "external" / "pmu-events" / "arch"
                                      / "x86" / "arrowlake");
}

// The root one round trip writes into, under the name both host-record suites
// share, suffixed with the pid so two units and two runs stand apart
// (`harness_host_record_test.cpp:341`).
auto roundTripRoot(const std::string& name) -> std::filesystem::path
{
  const auto path = std::filesystem::path("harness_host_record_roots")
      / (name + "_pid" + std::to_string(::getpid()));
  std::filesystem::remove_all(path);
  const bool created = std::filesystem::create_directories(path);
  check(created || std::filesystem::is_directory(path),
        "the round-trip root stands created (FR-032)");
  return path;
}

// The identity the writer answers: the figures and the key the fixture above
// spells (FR-007).
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

// The layout that renders the machine block above: one package, twenty cores
// over twenty threads and the mask `0-19`, two L3 groups over cpus `0-7` and
// `8-19`, and one node the mask leaves whole (FR-012, FR-013).
auto plantedLayout() -> sg::detail::MachineLayout
{
  auto layout = sg::detail::MachineLayout {};
  layout.readable = true;
  layout.packages = 1;
  layout.cores.resize(20U);
  auto low = std::vector<std::uint32_t> {};
  auto high = std::vector<std::uint32_t> {};
  auto node = std::vector<std::uint32_t> {};
  for (std::uint32_t cpu = 0; cpu < 20U; ++cpu) {
    const auto group = cpu < 8U ? 0U : 1U;
    (group == 0U ? low : high).push_back(cpu);
    node.push_back(cpu);
    layout.mask.push_back(cpu);
    auto& core = layout.cores[cpu];
    core.groupOrdinal = group;
    core.threads.push_back(cpu);
    layout.threads.push_back(sg::detail::LayoutThread {
        .cpuIndex = cpu, .coreOrdinal = cpu, .groupOrdinal = group});
  }
  layout.groups = {sg::detail::LayoutGroup {0, low},
                   sg::detail::LayoutGroup {8, high}};
  layout.memoryReadable = true;
  layout.nodes = {sg::detail::LayoutNode {0, node}};
  return layout;
}

// The seam of the round trip: the planted host above, for whichever root the
// parse named, so the run reads no CPUID, no `/proc` and no `/sys` (FR-032,
// FR-035).
auto plantedSeam() -> sg::detail::HostFactsSeam
{
  return [](const std::filesystem::path&)
  {
    auto facts = sg::detail::HostFacts {};
    facts.ident = plantedIdent();
    facts.layout = plantedLayout();
    return facts;
  };
}

// The record name of the planted host: the key with the `.json` suffix and
// nothing else (FR-009).
auto recordFile(const std::filesystem::path& out) -> std::filesystem::path
{
  return out / "GenuineIntel-6-C6-2.json";
}

// One writer run over one root, and the status the entry point answers. The
// line the run prints belongs to the write-path tests, so this unit leaves it
// unread (FR-005).
auto runWriter(const std::filesystem::path& root) -> int
{
  const auto arguments = std::vector<std::string> {
      "--root", root.string(), "--out", root.string()};
  auto argv =
      std::vector<char*> {const_cast<char*>("harness_host_record_schema_test")};
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  return sg::detail::hostRecordWriteMain(
      static_cast<int>(argv.size()), argv.data(), plantedSeam());
}

// FR-021, FR-023, SC-005: the bytes the writer produced pass the guard, and
// reading the record back answers the values the planted host carried, the row
// the standing record holds included.
auto theWrittenRecordPassesTheGuardAndReadsBack() -> void
{
  const auto out = roundTripRoot("round_trip");
  plantSchema(out);
  plantCatalog(out);
  sg::test::writeText(recordFile(out).string(), standingRecord());
  const auto status = runWriter(out);
  check(status == 0, "the writer refreshes the standing record (FR-005)");
  const auto written = sg::test::readText(recordFile(out).string());
  check(!written.empty(), "the record file holds the written bytes (FR-023)");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, written);
  if (!answer.has_value()) {
    report(written, answer.error());
  }
  check(answer.has_value(),
        "the document the writer produced passes the guard (FR-021, SC-005)");

  const auto record =
      sg::detail::readHostRecord(recordFile(out), SG_SCHEMA_PATH);
  check(record.has_value(),
        "the written bytes read back as one record (FR-020, FR-023)");
  if (!record.has_value()) {
    return;
  }
  const auto ident = plantedIdent();
  check(record->cpu.vendor == ident.vendor
            && record->cpu.family == ident.family
            && record->cpu.model == ident.model
            && record->cpu.stepping.has_value()
            && *record->cpu.stepping == *ident.stepping
            && record->cpu.key == ident.key
            && record->cpu.directory == ident.directory,
        "the record read back carries the identity the writer answered "
        "(FR-023)");
  check(record->machine.packages == 1U && record->machine.l3Groups == 2U
            && record->machine.cores == 20U && record->machine.threads == 20U
            && record->machine.mask == "0-19" && record->machine.l3.size() == 2U
            && record->machine.nodes.has_value()
            && record->machine.nodes->size() == 1U
            && (*record->machine.nodes)[0].number == 0U
            && (*record->machine.nodes)[0].cpus == "0-19",
        "the record read back carries the machine block the writer answered "
        "(FR-023)");
  check(record->evidence.size() == 1U && record->evidence[0].path == kRowPath
            && record->evidence[0].command == kRowCommand
            && record->evidence[0].exit == 0
            && record->evidence[0].line == kRowLine,
        "the row the standing record carries reads back with its four fields "
        "(FR-015, FR-023)");
}

}  // namespace

auto main() -> int
{
  thePinnedDocumentPassesTheGuard();
  everyObjectRequiresItsOwnKeys();
  everyObjectStandsClosed();
  twoRootKeysOutOfOrderStandRefused();
  everyKeyRefusesAValueOfAnotherKind();
  oneRootKeyTwiceStandsRefused();
  theUnansweredValuesStandNull();
  theBytesRoundTripThroughTheFile();
  theWrittenRecordPassesTheGuardAndReadsBack();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
