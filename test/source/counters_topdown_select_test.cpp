// ============================================================================
// The identity a selection reads and the table it selects (specs/042-topdown
// T010; FR-001, FR-002, FR-003, FR-009, FR-048; data-model E-01 to E-05;
// specs/042-topdown/contracts/topdown-selection.md).
//
// `cpuIdentity()` answers the four fields `pmuIdentCurrent()` reads, and it
// leaves the core type and the `MIDR` absent, because no reader for either
// stands in this feature (FR-003, spec D-13). A test hands both in, and the
// record carries what it was handed.
//
// `topdownTable()` answers the directory the vendored mapping file gives the
// identity it is handed. Three directories stand in one table each: the
// model-85 stepping-4 part the acceptance host is, the family-25 model-17
// part and the family-26 model-17 part. The two AMD identities are read on
// this Intel host, so an answer that names an AMD table can only come from the
// identity handed in: the selection reads no cpu of its own (FR-002).
//
// The table of the host's own identity carries the source record the manifest
// states: the reference of the vendored tree and one digest for each file that
// contributed a row. That case reads the expected record out of the registry
// and the parsed table, which `counters_topdown_source_test` pins against the
// vendored `RECORD`, so it opens no path and states no host's file count
// (FR-015, FR-033, spec D-12).
//
// The directory labels are the fixture's own, from
// `test/source/detail/topdown_fixture.hpp`, the header T009 wrote, so the
// labels one unit reads and the labels the next unit plants rows under stand in
// one place. No mapping-file row moves, and no host stands in for another.
//
// The two readers a case compares against are the seams the contract names:
// `pmuIdentCurrent()` and `pmuSelectDirectory()`
// (`source/counters/detail/pmu.hpp`).
//
// The tree cases read the selection through `TopdownSelection::table`, the seam
// the contract gives a test. A planted table holds the fixture rows of
// `test/source/detail/topdown_fixture.hpp` as the metric layer's own verdicts:
// the rows go through `parseMetricRows` and `metricRows` over the scripted host
// of `test/source/detail/metric_fixture.hpp`, so a planted row carries the
// spine, the leaf addresses, the refusal kind and the reason the host answers
// with, and the selection reaches its tree out of those rows alone (FR-010,
// FR-016, FR-048).
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "speedgun-ng/counters_topdown.hpp"

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/embedded_tables.hpp"
#include "../source/counters/linux_pmu/metric_formula.hpp"
#include "../source/counters/linux_pmu/topdown_select.hpp"
#include "detail/topdown_fixture.hpp"
#include "speedgun-ng/counters_metrics.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS TOPDOWN SELECT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::cpuIdentity;
using sg::counters::CpuIdentity;
using sg::counters::metricRefusalName;
using sg::counters::TableFile;
using sg::counters::TopdownNode;
using sg::counters::TopdownRefusal;
using sg::counters::TopdownRow;
using sg::counters::TopdownSelection;
using sg::counters::TopdownSource;
using sg::counters::topdownTable;
using sg::counters::TopdownTable;
using sg::counters::topdownTree;
using sg::counters::TopdownTree;
using sg::counters::TopdownUnsupport;
using sg::counters::TopdownUnsupported;
using sg::counters::detail::EmbeddedDir;
using sg::counters::detail::embeddedFindDir;
using sg::counters::detail::ExprCore;
using sg::counters::detail::ExprNode;
using sg::counters::detail::fillSourceRecord;
using sg::counters::detail::LeafCore;
using sg::counters::detail::metricRows;
using sg::counters::detail::parseMetricRows;
using sg::counters::detail::PmuIdent;
using sg::counters::detail::pmuIdentCurrent;
using sg::counters::detail::pmuLoadTable;
using sg::counters::detail::pmuSelectDirectory;
using sg::counters::detail::PmuTableEntry;
using sg::test::fixtureHost;
using sg::test::kAmdLevelOneNames;
using sg::test::kHybridDirectory;
using sg::test::kIntelDirectory;
using sg::test::kLevelOneThreshold;
using sg::test::kLevelTwoThreshold;
using sg::test::kNeutralBackendBound;
using sg::test::kNeutralBadSpeculation;
using sg::test::kNeutralFrontendBound;
using sg::test::kNeutralRetiring;
using sg::test::kNeutralSmtContention;
using sg::test::kNoLevelDirectory;
using sg::test::kZen4Directory;
using sg::test::kZen5Directory;
using sg::test::kZenLevelTwoNames;
using sg::test::MetricDefinition;
using sg::test::MetricRow;
using sg::test::registerMetricHost;
using sg::test::topdownHybridEventRows;
using sg::test::topdownHybridRows;
using sg::test::topdownIntelRows;
using sg::test::topdownIntelShallowRows;
using sg::test::topdownZen4Rows;
using sg::test::topdownZen5Rows;

// The `MIDR` a case hands in, spelled the way an Arm part publishes it: the
// implementer, the variant, the architecture, the part number and the revision
// in one 32-bit word. No production host answers it (spec D-13).
constexpr std::uint32_t kHandedMidr {0x410fd490U};

// The core type a case hands in, spelled the way a hybrid table spells the
// class its atom rows carry.
constexpr std::string_view kHandedCoreType {"cpu_atom"};

// The identity an identity mapfile row matches no text of, so the selection
// answers no directory at all.
[[nodiscard]] auto unmatchedIdentity() -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "NoSuchVendorXXXXX", .family = 999, .model = 999};
}

// One Intel identity. The mapping file spells a model in uppercase hex, so the
// decimal 85 is the row that reads `55`, and the stepping is the digit that row
// carries after the model (specs/007 FR-034).
[[nodiscard]] auto intelIdentity(const int family,
                                 const int model,
                                 const int stepping) -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "GenuineIntel",
      .family = family,
      .model = model,
      .stepping = stepping,
  };
}

// One AMD identity of no stepping class: a mapping-file row that carries no
// stepping is matched against the model key alone, so the field stays at its
// absent value (specs/007 FR-034).
[[nodiscard]] auto amdIdentity(const int family, const int model) -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "AuthenticAMD", .family = family, .model = model};
}

// The whole source record as one text. The published record carries no
// comparison operator, so two answers of one identity compare field by field
// through this text.
[[nodiscard]] auto sourceText(const TopdownTable& table) -> std::string
{
  std::string text {table.source.reference};
  text += '\n';
  text += table.source.directory;
  for (const TableFile& file : table.source.files) {
    text += '\n';
    text += file.name;
    text += ' ';
    text += file.manifestHash;
  }
  return text;
}

// The descriptors this process holds, counted through its own descriptor
// directory. An event the counters library opens hands back a descriptor, so a
// call that left one open moves this listing. `counters_pmu_test.cpp` reads
// the same listing for the same claim.
[[nodiscard]] auto openDescriptorCount() -> std::size_t
{
  std::error_code failure;
  std::size_t count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator("/proc/self/fd", failure))
  {
    static_cast<void>(entry);
    ++count;
  }
  if (failure) {
    fail("the process's own descriptor listing is readable");
  }
  return count;
}

// The mappings this process holds, counted through its own mapping listing. A
// sampled event maps a page, so a call that sampled a counter moves this
// listing.
[[nodiscard]] auto mappingCount() -> std::size_t
{
  std::ifstream maps("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

// The refusal a selection answers ends the unit with the kind and the three
// fields it carries, because every case below reads a tree.
auto failRefused(const TopdownUnsupported& why) -> void
{
  std::fprintf(stderr,
               "COUNTERS TOPDOWN SELECT TEST FAIL: the selection refused with "
               "kind %d, vendor '%s', identity '%s', detail '%s'\n",
               static_cast<int>(why.kind),
               why.vendor.c_str(),
               why.identity.c_str(),
               why.detail.c_str());
  std::exit(1);
}

// One fixture table as the selection reads it: the event rows apart from the
// metric rows, and the metric rows parsed and resolved against the scripted
// host, so a planted row carries the verdict, the spine, the leaf addresses and
// the reason that host answers with (FR-010, FR-016).
[[nodiscard]] auto plantedTable(const std::string_view directory,
                                const std::vector<MetricRow>& rows)
    -> TopdownTable
{
  std::vector<MetricRow> events;
  for (const MetricRow& row : rows) {
    if (row.metricName.empty()) {
      events.push_back(row);
    }
  }
  const auto parsed = parseMetricRows(rows);
  const auto resolved = metricRows(parsed, events, fixtureHost());
  TopdownTable table;
  table.source.directory = std::string {directory};
  for (const MetricDefinition& def : resolved) {
    TopdownRow row;
    row.name = def.name;
    row.group = def.group;
    row.unit = def.unit;
    row.threshold = def.threshold;
    row.formula.core = def.core;
    row.published = def.published;
    row.refusal = def.refusal;
    row.reason = def.reason;
    table.rows.push_back(std::move(row));
  }
  return table;
}

// The threshold text of one planted row, spelled the way a table spells it. The
// row field that carries the text out of a table file stands since T020, and no
// fixture row carries a text, so a case plants the text it reads back
// (FR-017, spec D-9, D-11).
auto plantThreshold(TopdownTable& table,
                    const std::string_view name,
                    const std::string_view text) -> void
{
  for (TopdownRow& row : table.rows) {
    if (row.name == name) {
      row.threshold = std::string {text};
    }
  }
}

// The group text of one planted row, replaced in place. The level rule of
// FR-014 reads that text, so a case that wants a row to carry tokens other than
// the ones the fixture spells for it carries them here (FR-014).
auto plantGroup(TopdownTable& table,
                const std::string_view name,
                const std::string_view group) -> void
{
  for (TopdownRow& row : table.rows) {
    if (row.name == name) {
      row.group = std::string {group};
    }
  }
}

// What a case asks for: one identity, one level, and, where a case plants rows,
// the table the answer must come from (FR-002, PC-9).
[[nodiscard]] auto selection(const CpuIdentity& identity,
                             const int level) -> TopdownSelection
{
  TopdownSelection options;
  options.identity = identity;
  options.level = level;
  return options;
}

[[nodiscard]] auto selection(const CpuIdentity& identity,
                             const int level,
                             const TopdownTable& table) -> TopdownSelection
{
  TopdownSelection options = selection(identity, level);
  options.table = table;
  return options;
}

// The tree one selection answers over one planted table.
[[nodiscard]] auto asked(const TopdownTable& table,
                         const int level,
                         const CpuIdentity& identity) -> TopdownTree
{
  const auto answered = topdownTree(selection(identity, level, table));
  if (!answered.has_value()) {
    failRefused(answered.error());
  }
  return *answered;
}

[[nodiscard]] auto nodeNamed(const TopdownTree& tree,
                             const std::string_view name) -> const TopdownNode*
{
  for (const TopdownNode& node : tree.nodes) {
    if (node.name == name) {
      return &node;
    }
  }
  return nullptr;
}

// The node that carries one row's own name, so a case can ask whether a row
// reached a node without naming the neutral name the selection gave it.
[[nodiscard]] auto nodeOfRow(const TopdownTree& tree,
                             const std::string_view vendorName)
    -> const TopdownNode*
{
  for (const TopdownNode& node : tree.nodes) {
    if (node.vendorName == vendorName) {
      return &node;
    }
  }
  return nullptr;
}

[[nodiscard]] auto refusalNamed(const TopdownTree& tree,
                                const std::string_view name)
    -> const TopdownRefusal*
{
  for (const TopdownRefusal& row : tree.refused) {
    if (row.name == name) {
      return &row;
    }
  }
  return nullptr;
}

[[nodiscard]] auto rowNamed(const TopdownTable& table,
                            const std::string_view name) -> const TopdownRow*
{
  for (const TopdownRow& row : table.rows) {
    if (row.name == name) {
      return &row;
    }
  }
  return nullptr;
}

// The addresses one row's spine reads, in the order the spine carries them.
[[nodiscard]] auto leafAddresses(const TopdownRow& row)
    -> std::vector<std::string>
{
  std::vector<std::string> addresses;
  for (const LeafCore& leaf : row.formula.core.leaves) {
    addresses.push_back(leaf.address);
  }
  return addresses;
}

// Two spines are one spine: the leaf addresses match, and so does every field
// of every node, in the order each spine carries them.
[[nodiscard]] auto differs(const double left, const double right) -> bool
{
  return !(left <= right) || !(right <= left);
}

[[nodiscard]] auto sameSpine(const ExprCore& left,
                             const ExprCore& right) -> bool
{
  if (left.leaves.size() != right.leaves.size()
      || left.nodes.size() != right.nodes.size())
  {
    return false;
  }
  for (std::size_t i = 0; i != left.leaves.size(); ++i) {
    if (left.leaves[i].address != right.leaves[i].address) {
      return false;
    }
  }
  for (std::size_t i = 0; i != left.nodes.size(); ++i) {
    const ExprNode& one = left.nodes[i];
    const ExprNode& two = right.nodes[i];
    if (one.kind != two.kind || one.left != two.left || one.right != two.right
        || one.leaf != two.leaf || differs(one.scale, two.scale))
    {
      return false;
    }
  }
  return true;
}

// The count of nodes of one level, and the deepest level a tree holds.
[[nodiscard]] auto nodesAtLevel(const TopdownTree& tree,
                                const int level) -> std::size_t
{
  std::size_t count = 0;
  for (const TopdownNode& node : tree.nodes) {
    if (node.level == level) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] auto deepestLevel(const TopdownTree& tree) -> int
{
  int deepest = 0;
  for (const TopdownNode& node : tree.nodes) {
    deepest = node.level > deepest ? node.level : deepest;
  }
  return deepest;
}

// Whether every node below level 1 names a parent that stands in the tree as a
// node one level above. A tree that answers this holds no parent no row stated,
// because the only parent a node can carry is the one its own group text names
// (FR-013).
[[nodiscard]] auto deeperParentsStand(const TopdownTree& tree) -> bool
{
  for (const TopdownNode& node : tree.nodes) {
    if (node.level == 1) {
      continue;
    }
    const TopdownNode* above = nodeNamed(tree, node.parent);
    if (above == nullptr || above->level + 1 != node.level) {
      return false;
    }
  }
  return true;
}

// A node name below level 1 of an Intel table, and the same shape for an AMD
// table: the vendor's own name behind the vendor prefix of FR-012.
[[nodiscard]] auto qualified(const std::string_view name) -> std::string
{
  return "topdown.intel." + std::string {name};
}

[[nodiscard]] auto qualifiedAmd(const std::string_view name) -> std::string
{
  return "topdown.amd." + std::string {name};
}

// The whole answer as one text, so two answers of one selection compare field
// by field: no carrier here holds a comparison operator.
[[nodiscard]] auto treeText(const TopdownTree& tree) -> std::string
{
  std::string text {tree.source.directory};
  text += '\n';
  text += std::to_string(tree.depth);
  text += tree.amd ? " amd\n" : " intel\n";
  for (const TopdownNode& node : tree.nodes) {
    text += node.name;
    text += ' ';
    text += node.vendorName;
    text += ' ';
    text += node.parent;
    text += ' ';
    text += std::to_string(node.level);
    text += ' ';
    text += node.threshold;
    text += ' ';
    for (const std::string& event : node.events) {
      text += event;
      text += ',';
    }
    text += '\n';
  }
  for (const TopdownRefusal& row : tree.refused) {
    text += row.name;
    text += ' ';
    text += row.parent;
    text += ' ';
    text += std::to_string(row.level);
    text += ' ';
    text += metricRefusalName(row.refusal);
    text += ' ';
    text += row.reason;
    text += '\n';
  }
  return text;
}

// The nodes of one answer as one text: the name, the parent and the level of
// each, in the order the answer holds them. Two asks that collect one set
// answer one text here, whatever each of them asked (FR-008, FR-009).
[[nodiscard]] auto nodeSetText(const TopdownTree& tree) -> std::string
{
  std::string text;
  for (const TopdownNode& node : tree.nodes) {
    text += node.name;
    text += ' ';
    text += node.parent;
    text += ' ';
    text += std::to_string(node.level);
    text += '\n';
  }
  return text;
}

// The four level-1 rows of the Intel fixture table, each pair the table's own
// name and the shared name of FR-011, in the order the table spells them.
constexpr std::array<std::pair<std::string_view, std::string_view>, 4>
    kIntelLevelOneNames {{{"tma_frontend_bound", "topdown.frontend_bound"},
                          {"tma_bad_speculation", "topdown.bad_speculation"},
                          {"tma_backend_bound", "topdown.backend_bound"},
                          {"tma_retiring", "topdown.retiring"}}};

// FR-011, SC-01 and FR-052: the four shared names every table answers, the
// fifth an AMD table alone, and no parent at level 1.
auto theFillOfOneNameFillsTheRecordOrLeavesIt() -> void
{
  // The fill is the seam the record of a production table rides, and one name
  // drives each of its arms, so no host has to stand behind either check
  // (FR-015, FR-051).
  TopdownSource named;
  named.directory = "arch/x86/amdzen3/";
  fillSourceRecord(named);
  check(!named.reference.empty(),
        "a directory the registry holds states the reference its manifest "
        "names (FR-015, spec D-12)");
  check(!named.files.empty(),
        "a directory whose rows the metric layer reads names the files that "
        "contributed them (FR-033)");
  for (const TableFile& file : named.files) {
    check(!file.name.empty(), "a contributed file names itself (FR-033)");
    check(file.manifestHash.size() == 64,
          "a contributed file carries the digest its manifest line states "
          "(FR-033)");
  }

  // The registry names each directory with no trailing separator, and a host
  // hands in one with it. The cut ahead of the lookup leaves one record for
  // both spellings, so the name without the separator answers the same
  // reference and the same files (FR-036).
  TopdownSource bare;
  bare.directory = "arch/x86/amdzen3";
  fillSourceRecord(bare);
  check(bare.directory == "arch/x86/amdzen3",
        "the fill leaves the spelling it was given (FR-036)");
  check(bare.reference == named.reference,
        "a directory named with no trailing separator answers the reference "
        "of the same registry record (FR-015, spec D-12)");
  check(bare.files.size() == named.files.size(),
        "both spellings of one directory answer one file count (FR-033)");
  for (std::size_t here = 0; here != bare.files.size(); ++here) {
    check(bare.files[here].name == named.files[here].name
              && bare.files[here].manifestHash
                     == named.files[here].manifestHash,
          "the cut separator leaves the record of the directory unchanged "
          "(FR-033)");
  }

  // A name the registry holds no bytes for answers the source as it stands: no
  // reference, no file, and the name it arrived with. The mapping file names
  // only directories of the vendored tree, so this name reaches no selection,
  // and the arm stands where a test can hold it (FR-036, FR-051).
  TopdownSource unknown;
  unknown.directory = "arch/x86/no-such-table/";
  fillSourceRecord(unknown);
  check(unknown.directory == "arch/x86/no-such-table/",
        "the fill leaves the name it was given (FR-036)");
  check(unknown.reference.empty(),
        "a name the registry holds no bytes for states no reference (FR-036)");
  check(unknown.files.empty(),
        "a name the registry holds no bytes for contributes no file (FR-036)");
}

auto levelOneCarriesTheSharedNames() -> void
{
  const TopdownTree intel =
      asked(plantedTable(kIntelDirectory, topdownIntelRows()),
            1,
            intelIdentity(6, 85, 4));
  check(!intel.amd,
        "an Intel table leaves the vendor flag unset (FR-011, spec D-3)");
  check(intel.depth == 1,
        "a selection asked for level 1 answers depth 1 (FR-008)");
  check(intel.nodes.size() == kIntelLevelOneNames.size(),
        "level 1 of an Intel table answers four nodes (FR-011)");
  check(intel.refused.empty(),
        "the level-1 rows of the fixture table all publish (FR-016)");
  for (std::size_t i = 0; i != kIntelLevelOneNames.size(); ++i) {
    const TopdownNode& node = intel.nodes[i];
    check(node.vendorName == kIntelLevelOneNames[i].first,
          "the node carries its row's own name in the order the table spells "
          "it " "(FR-010)");
    check(node.name == kIntelLevelOneNames[i].second,
          "the node carries the shared name of FR-011");
    check(node.parent.empty(), "a level-1 node names no parent (FR-013)");
    check(node.level == 1,
          "the level token of a level-1 row reads as 1 (FR-014)");
  }
  check(nodeNamed(intel, kNeutralSmtContention) == nullptr,
        "an Intel table answers no fifth name at level 1 (FR-011, spec D-3)");

  const TopdownTree zen = asked(
      plantedTable(kZen4Directory, topdownZen4Rows()), 1, amdIdentity(25, 17));
  check(zen.amd, "an AMD table sets the vendor flag (FR-011, spec D-3)");
  check(zen.nodes.size() == kAmdLevelOneNames.size(),
        "level 1 of a Zen table answers five nodes (FR-011, FR-052)");
  for (std::size_t i = 0; i != kAmdLevelOneNames.size(); ++i) {
    const TopdownNode& node = zen.nodes[i];
    check(node.vendorName == kAmdLevelOneNames[i],
          "the Zen node carries its row's own name in the order the pipeline "
          "file spells it (FR-010)");
    check(node.name == "topdown." + std::string {kAmdLevelOneNames[i]},
          "an AMD table reaches the shared names with no prefix to drop "
          "(FR-011, spec D-2)");
    check(node.parent.empty() && node.level == 1,
          "a level-1 row of either vendor roots a node of its own (FR-013, "
          "FR-014)");
  }
  check(
      nodeNamed(zen, kNeutralSmtContention) != nullptr,
      "an AMD table answers the fifth name `topdown.smt_contention` (FR-011)");

  // The fifth row planted in an Intel table: it stands at level 1 with the
  // group text an Intel level-1 row spells. The Intel answer still holds four
  // nodes, because the level-1 pass drops that row ahead of any node, and the
  // same rows answer an AMD selection with the fifth node (FR-011, spec D-3).
  std::vector<MetricRow> withContention = topdownIntelRows();
  withContention.push_back(
      sg::test::fixtureMetric("tma_smt_contention",
                              "fixture_cycles / fixture_ref_cycles",
                              "100%",
                              {},
                              sg::test::intelGroup(1, "TmaL1")));
  const TopdownTable fifthRowTable =
      plantedTable(kIntelDirectory, withContention);
  const TopdownTree intelWithFifth =
      asked(fifthRowTable, 1, intelIdentity(6, 85, 4));
  check(intelWithFifth.nodes.size() == kIntelLevelOneNames.size(),
        "an Intel table that carries the fifth row answers four nodes "
        "(FR-011, spec D-3)");
  check(nodeNamed(intelWithFifth, kNeutralSmtContention) == nullptr,
        "the level-1 pass of an Intel answer drops the fifth row before it "
        "builds a node (FR-011)");
  const TopdownTree amdWithFifth = asked(fifthRowTable, 1, amdIdentity(25, 17));
  check(nodeNamed(amdWithFifth, kNeutralSmtContention) != nullptr,
        "the same row reaches the fifth node of an AMD answer (FR-011)");
}

// FR-013 and FR-014: both parent spellings resolve, a row whose parent stems to
// no row stands out of the tree, and a row with no level token stands out of it
// as well.
auto parentsComeFromTheGroupText() -> void
{
  const TopdownTree intel =
      asked(plantedTable(kIntelDirectory, topdownIntelRows()),
            2,
            intelIdentity(6, 85, 4));
  const TopdownNode* latency = nodeNamed(intel, qualified("tma_fetch_latency"));
  check(latency != nullptr,
        "the Intel level-2 row reaches a node of its own (FR-013)");
  check(latency != nullptr && latency->parent == kNeutralFrontendBound,
        "the Intel `tma_<name>_group` token names the parent (FR-013)");
  check(latency != nullptr && latency->level == 2
            && latency->vendorName == "tma_fetch_latency",
        "the node carries the level of its token and the name of its row "
        "(FR-010, FR-014)");

  const TopdownTree zen = asked(
      plantedTable(kZen4Directory, topdownZen4Rows()), 2, amdIdentity(25, 17));
  const TopdownNode* memory =
      nodeNamed(zen, qualifiedAmd("backend_bound_memory"));
  check(memory != nullptr && memory->parent == kNeutralBackendBound,
        "the AMD `<name>_group` token, spelled with no prefix, names the same "
        "parent (FR-013, spec D-2)");
  check(memory != nullptr && memory->level == 2,
        "the `PipelineL2` token reads as level 2 (FR-014)");

  check(nodeNamed(intel, qualified("tma_orphan_row")) == nullptr,
        "a row whose only `_group` token stems to no row reaches no node, and "
        "no " "part of the selection invents a parent for it (FR-013)");
  check(nodeNamed(zen, qualifiedAmd("total_dispatch_slots")) == nullptr,
        "the group-free helper row of an AMD table reaches no node (FR-014)");
  check(nodesAtLevel(intel, 2) == 6 && nodesAtLevel(zen, 2) == 8,
        "every published row of a level the answer reaches reaches one node "
        "(FR-010, FR-014)");
}

// FR-014, research D-02 and SC-01: the level of a node is the digit of the one
// level token its own group text carries. Intel spells that token `TopdownL1`
// to `TopdownL6` and AMD spells it `PipelineL1` and `PipelineL2`, and a row
// with no level token among its `;` tokens reaches no node and states no
// refusal.
auto theLevelTokenOfEachGroupSpellingStatesTheLevel() -> void
{
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const CpuIdentity zen = amdIdentity(25, 17);
  const TopdownTree deep =
      asked(plantedTable(kIntelDirectory, topdownIntelRows()), 6, intel);
  check(deep.depth == 6,
        "the Intel fixture carries every level token, so an ask of 6 clamps "
        "nowhere (FR-008)");
  // One row of each chain level, named the way the tree names it, beside the
  // digit its own `TopdownL<n>` token carries. The chain runs from a level-1
  // root to the deepest row of the fixture (FR-014).
  constexpr std::array<std::pair<std::string_view, int>, 6> kIntelChain {
      {{kNeutralFrontendBound, 1},
       {"topdown.intel.tma_fetch_latency", 2},
       {"topdown.intel.tma_branch_resteers", 3},
       {"topdown.intel.tma_clears_resteers", 4},
       {"topdown.intel.tma_dram_page_miss", 5},
       {"topdown.intel.tma_dram_page_miss_4k", 6}}};
  for (const auto& [name, level] : kIntelChain) {
    const TopdownNode* node = nodeNamed(deep, name);
    check(node != nullptr,
          "the chain of the fixture holds a node of that name at each of the "
          "six levels (FR-014)");
    check(node != nullptr && node->level == level,
          "the `TopdownL<n>` token of that row states its level (FR-014)");
  }
  // The published rows of each level of the fixture, counted in the order the
  // level tokens run: four roots, six rows under them, and four, three, two and
  // two rows down the chains. The orphan row of level 2 stands in no level.
  constexpr std::array<std::size_t, 6> kIntelLevelCounts {{4, 6, 4, 3, 2, 2}};
  for (int level = 1; level <= 6; ++level) {
    const std::size_t stated =
        kIntelLevelCounts[static_cast<std::size_t>(level - 1)];
    check(nodesAtLevel(deep, level) == stated,
          "every row whose group carries `TopdownL` of that level reaches one "
          "node of that level (FR-014)");
  }

  const TopdownTree zen4 =
      asked(plantedTable(kZen4Directory, topdownZen4Rows()), 2, zen);
  check(zen4.depth == 2,
        "a pipeline table answers the level an ask names, the way an Intel "
        "table does (FR-008)");
  check(nodesAtLevel(zen4, 1) == kAmdLevelOneNames.size()
            && nodesAtLevel(zen4, 2) == kZenLevelTwoNames.size(),
        "the `PipelineL1` rows answer level 1 and the `PipelineL2` rows answer "
        "level 2 (FR-014)");

  // The helper row of a pipeline table: a formula, a scale, and no group text
  // at all, so no token of it states a level and it reaches neither carrier.
  const TopdownTable bare = plantedTable(kZen4Directory, topdownZen4Rows());
  const TopdownTree groupless = asked(bare, 2, zen);
  check(nodeOfRow(groupless, "total_dispatch_slots") == nullptr
            && nodeNamed(groupless, qualifiedAmd("total_dispatch_slots"))
                   == nullptr,
        "the group-free helper row of a pipeline table reaches no node "
        "(FR-014)");
  check(refusalNamed(groupless, "total_dispatch_slots") == nullptr
            && groupless.refused.empty(),
        "the group-free helper row states no refusal, so a row of no level "
        "stands in no carrier (FR-014, FR-016)");

  // The same row with a group text of two tokens, neither a level token and
  // neither naming a row of the directory: the level rule reads the tokens and
  // finds none, so the answer above stands unchanged (FR-014).
  TopdownTable grouped = bare;
  plantGroup(
      grouped, "total_dispatch_slots", "TmaDispatch;dispatch_slots_group");
  check(treeText(asked(grouped, 2, zen)) == treeText(groupless),
        "a row whose group carries tokens and no level token among them "
        "reaches no node and states no refusal (FR-014)");
}

// FR-014, FR-013 and FR-016: the three group texts that name no place in the
// tree. A level token whose digit stands outside the range that spelling states
// names no level, a `_group` token whose stem names the row itself names no
// parent, and a `_group` token whose stem names a row the metric layer refused
// names a parent that holds no node. Each row stands out of the tree, and no
// part of the pass invents a level or a parent for it. The refused row of the
// last case still stands in the document, and its parent line reads one level
// above it because the parent row states no level of its own.
auto aGroupTextOfNoLevelOrOfNoParentStandsOutOfTheTree() -> void
{
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const TopdownTable plain = plantedTable(kIntelDirectory, topdownIntelRows());

  // `TopdownL0` carries the Intel spelling with a digit below the range and
  // `TopdownL7` carries it above it. Neither states a level, so the row of that
  // group reaches no node and states no refusal (FR-014, spec D-2).
  TopdownTable outside = plain;
  plantGroup(outside,
             "tma_fetch_latency",
             "TmaL2;TopdownL0;TopdownL7;tma_frontend_bound_group");
  const TopdownTree beyond = asked(outside, 6, intel);
  check(nodeOfRow(beyond, "tma_fetch_latency") == nullptr,
        "a row whose level tokens carry digits outside the range the spelling "
        "states reaches no node (FR-014)");
  check(refusalNamed(beyond, "tma_fetch_latency") == nullptr,
        "the same row states no refusal, because it holds no level the answer "
        "reaches (FR-016)");

  // A `_group` token whose stem names the row itself: the search for a parent
  // holds no other row, so the row stands out of the tree (FR-013).
  TopdownTable itself = plain;
  plantGroup(itself, "tma_fetch_latency", "TopdownL2;tma_fetch_latency_group");
  const TopdownTree alone = asked(itself, 6, intel);
  check(nodeOfRow(alone, "tma_fetch_latency") == nullptr,
        "a `_group` token whose stem names its own row states no parent, and "
        "the row reaches no node (FR-013)");

  // A `_group` token whose stem names a row the metric layer refused: the
  // parent row stands in the table and holds no node, so the child stands out
  // of the tree beside it (FR-013, FR-016).
  TopdownTable refusedParent = plain;
  plantGroup(
      refusedParent, "tma_fetch_latency", "TopdownL2;tma_bad_arity_group");
  const TopdownTree parentless = asked(refusedParent, 6, intel);
  check(nodeOfRow(parentless, "tma_fetch_latency") == nullptr,
        "a row whose parent row the metric layer refused reaches no node "
        "(FR-013)");
  check(nodeOfRow(parentless, "tma_bad_arity") == nullptr
            && refusalNamed(parentless, "tma_bad_arity") != nullptr,
        "the refused row the parent token names keeps its place in the "
        "document (FR-016)");

  // The parent line of a refused row reads the level its parent row states, and
  // one level above the refused row where that parent row states no level at
  // all (FR-016, FR-031). The first plant leaves the parent row of no level;
  // the second moves the refused row under it.
  TopdownTable noLevel = plain;
  plantGroup(
      noLevel, "tma_memory_bandwidth", "TmaDispatch;dispatch_slots_group");
  plantGroup(noLevel, "tma_bad_arity", "TopdownL2;tma_memory_bandwidth_group");
  const TopdownTree folded = asked(noLevel, 6, intel);
  const TopdownRefusal* arity = refusalNamed(folded, "tma_bad_arity");
  check(arity != nullptr && arity->parent == "topdown.memory_bandwidth",
        "a refused row whose parent row states no level names that parent one "
        "level above itself (FR-016, FR-031)");
  check(arity != nullptr && arity->level == 2,
        "the refused row keeps the level its own token states (FR-016)");
}

// FR-013 and SC-02: every node below level 1 names the parent its own `_group`
// token states, the Intel spelling `tma_<name>_group` both at the level below a
// root and behind the vendor prefix deeper still, and a row whose stem names no
// row of the directory reaches no node under any name. The AMD spelling stands
// row for row in `theAmdLevelTwoRowsStandRowForRow`.
auto everyDeeperNodeNamesTheParentItsOwnGroupTokenStates() -> void
{
  const TopdownTable table = plantedTable(kIntelDirectory, topdownIntelRows());
  const TopdownTree deep = asked(table, 6, intelIdentity(6, 85, 4));
  // Every deeper row of the fixture beside the neutral name of the row its own
  // `tma_<name>_group` token states, in the order the table spells the rows. A
  // parent of level 2 or deeper stands behind the vendor prefix of FR-012,
  // which is the name the tree carries for it.
  constexpr std::array<std::pair<std::string_view, std::string_view>, 17>
      kStatedParents {{
          {"tma_fetch_latency", kNeutralFrontendBound},
          {"tma_fetch_bandwidth", kNeutralFrontendBound},
          {"tma_branch_mispredicts", kNeutralBadSpeculation},
          {"tma_core_bound", kNeutralBackendBound},
          {"tma_memory_bound", kNeutralBackendBound},
          {"tma_memory_bandwidth", kNeutralBackendBound},
          {"tma_branch_resteers", "topdown.intel.tma_fetch_latency"},
          {"tma_dsb", "topdown.intel.tma_fetch_bandwidth"},
          {"tma_divider", "topdown.intel.tma_core_bound"},
          {"tma_dram_bound", "topdown.intel.tma_memory_bound"},
          {"tma_clears_resteers", "topdown.intel.tma_branch_resteers"},
          {"tma_divider_high", "topdown.intel.tma_divider"},
          {"tma_dram_bandwidth", "topdown.intel.tma_dram_bound"},
          {"tma_dram_page_miss", "topdown.intel.tma_dram_bandwidth"},
          {"tma_divider_high_2m", "topdown.intel.tma_divider_high"},
          {"tma_dram_page_miss_4k", "topdown.intel.tma_dram_page_miss"},
          {"tma_divider_high_4k", "topdown.intel.tma_divider_high_2m"},
      }};
  for (const auto& [row, parent] : kStatedParents) {
    const TopdownNode* node = nodeNamed(deep, qualified(row));
    check(node != nullptr,
          "each deeper row of the fixture reaches a node of its own (FR-013)");
    check(node != nullptr && node->parent == parent,
          "that node names the parent its own `_group` token states (FR-013)");
  }
  check(deep.nodes.size() == kIntelLevelOneNames.size() + kStatedParents.size(),
        "no deeper row of the fixture stands apart from the table above, so no "
        "node carries a parent the rows state for no row (FR-013)");
  check(deeperParentsStand(deep),
        "every parent stands in the tree as a node one level above its child "
        "(FR-013)");

  // The row whose only `_group` token is `tma_L2_group`, a stem no row of the
  // directory carries: the tree holds no node for it, and the token states no
  // parent for any other row either (FR-013).
  check(nodeNamed(deep, qualified("tma_orphan_row")) == nullptr,
        "a level-2 row whose stem names no row reaches no node (FR-013)");
  check(deeperParentsStand(deep),
        "every parent names a node the table published, so the pass invents no "
        "parent for a row whose stem names no row (FR-013)");

  // The AMD spelling of the same token, with no `tma_` prefix ahead of the stem
  // (FR-013, spec D-2). `theAmdLevelTwoRowsStandRowForRow` names all eight rows
  // of a pipeline table and the parent of each.
  const TopdownTree zen4 = asked(
      plantedTable(kZen4Directory, topdownZen4Rows()), 2, amdIdentity(25, 17));
  check(deeperParentsStand(zen4) && nodesAtLevel(zen4, 2) > 0,
        "the `<name>_group` spelling resolves too, so one token rule covers "
        "both vendors (FR-013, research D-02)");
}

// FR-008 and FR-020: the asked level clamps to the deepest level the table
// holds, and a row below the answer's depth stands out of it.
auto levelDeeperThanTheTableClamps() -> void
{
  const TopdownTable shallow =
      plantedTable(kIntelDirectory, topdownIntelShallowRows());
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const TopdownTree sixth = asked(shallow, 6, intel);
  check(sixth.depth == 4,
        "asking level 6 of a table whose deepest level token is `TopdownL4` "
        "answers depth 4 (FR-008, FR-020)");
  check(deepestLevel(sixth) == 4,
        "the deepest node of the answer stands at the depth the answer states "
        "(FR-020)");
  check(nodesAtLevel(sixth, 4) == 3,
        "the level the clamp lands on keeps its rows (FR-008)");
  const TopdownTree fourth = asked(shallow, 4, intel);
  check(treeText(fourth) == treeText(sixth),
        "asking the depth the table holds answers the tree the deeper ask "
        "answers (FR-008, FR-009)");

  const TopdownTable deep = plantedTable(kIntelDirectory, topdownIntelRows());
  check(asked(deep, 6, intel).depth == 6,
        "a table that holds six levels answers the ask itself (FR-008)");
  const TopdownTree second = asked(deep, 2, intel);
  check(second.depth == 2 && deepestLevel(second) == 2,
        "a table deeper than the ask answers the ask, and no deeper node "
        "stands " "in it (FR-008)");
  check(
      nodeNamed(second, qualified("tma_branch_resteers")) == nullptr,
      "a row of a level the answer does not reach stands in no node (FR-008)");
}

// FR-008, FR-009 and FR-020: an ask deeper than the selected table holds
// answers the deepest level that table holds, holds no node below it, and
// answers the node set the ask that names that level answers. FR-008 states the
// shape over the shallowest table of all: level-1 rows and no deeper row answer
// a level-1 tree, whatever deeper levels the request asked for.
auto anAskDeeperThanTheTableHoldsAnswersTheLevelItHolds() -> void
{
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const TopdownTable shallow =
      plantedTable(kIntelDirectory, topdownIntelShallowRows());
  const TopdownTree sixth = asked(shallow, 6, intel);
  const TopdownTree fifth = asked(shallow, 5, intel);
  const TopdownTree fourth = asked(shallow, 4, intel);
  check(sixth.depth == 4 && fifth.depth == 4 && fourth.depth == 4,
        "an ask of 6, an ask of 5 and an ask of 4 settle on the deepest level "
        "the table holds (FR-008, FR-020)");
  check(nodesAtLevel(sixth, 5) == 0 && nodesAtLevel(sixth, 6) == 0
            && deepestLevel(sixth) == 4,
        "the clamped answer holds no node below the level it states (FR-008)");
  check(!nodeSetText(sixth).empty(),
        "the answer measured below holds nodes (FR-009)");
  check(nodeSetText(fifth) == nodeSetText(sixth)
            && nodeSetText(fourth) == nodeSetText(sixth),
        "an ask deeper than the table holds answers the node set the ask that "
        "names its deepest level answers (FR-008, FR-009)");

  // The table FR-008 states in its own words: the four level-1 rows of the
  // fixture and no deeper row, the event rows beside them so each spine
  // resolves (FR-008, FR-016).
  std::vector<MetricRow> rootsOnly;
  for (const MetricRow& row : topdownIntelRows()) {
    const bool levelOne = row.group.find("TopdownL1") != std::string_view::npos;
    if (row.metricName.empty() || levelOne) {
      rootsOnly.push_back(row);
    }
  }
  const TopdownTable roots = plantedTable(kIntelDirectory, rootsOnly);
  check(roots.rows.size() == kIntelLevelOneNames.size(),
        "the roots-only table holds the four level-1 rows of the fixture and "
        "no deeper row (FR-008)");
  const TopdownTree rootsAtSix = asked(roots, 6, intel);
  check(rootsAtSix.depth == 1 && deepestLevel(rootsAtSix) == 1
            && rootsAtSix.nodes.size() == kIntelLevelOneNames.size(),
        "a table of level-1 rows and no deeper row answers a level-1 tree to "
        "an ask of 6 (FR-008)");
  const TopdownTree rootsAtOne = asked(roots, 1, intel);
  check(nodeSetText(rootsAtOne) == nodeSetText(rootsAtSix)
            && rootsAtOne.depth == 1,
        "the level-1 tree stands the same for an ask of 1 and an ask of 6 "
        "(FR-008, FR-009)");
}

// FR-016: a refused row stands in the document with the metric layer's kind and
// reason, at the level its own token states, and out of the nodes.
auto refusedRowsStayInTheDocument() -> void
{
  const TopdownTable table = plantedTable(kIntelDirectory, topdownIntelRows());
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const TopdownTree second = asked(table, 2, intel);
  check(second.refused.size() == 8,
        "the eight refused level-2 rows stand in the answer (FR-016)");
  check(nodesAtLevel(second, 2) == 6,
        "the six published level-2 rows stand in the nodes (FR-016)");
  for (const char* const name :
       {"tma_bad_arity", "tma_no_leaf_row", "tma_mixed_terms"})
  {
    const TopdownRefusal* refusal = refusalNamed(second, name);
    const TopdownRow* row = rowNamed(table, name);
    check(refusal != nullptr && row != nullptr && !row->published,
          "the refused row of that name reaches the document (FR-016)");
    check(refusal != nullptr && row != nullptr
              && refusal->refusal == row->refusal,
          "the refusal carries the metric layer's kind unchanged (FR-016, "
          "FR-031)");
    check(refusal != nullptr && row != nullptr && refusal->reason == row->reason,
          "the refusal carries the metric layer's reason unchanged (FR-016, "
          "FR-031)");
    check(refusal != nullptr && refusal->level == 2,
          "the refusal stands at the level its own group text states (FR-016)");
    check(nodeNamed(second, qualified(name)) == nullptr,
          "a refused row stands in no node (FR-016)");
  }
  const TopdownRefusal* arity = refusalNamed(second, "tma_bad_arity");
  check(arity != nullptr && arity->parent == qualified("tma_fetch_latency"),
        "the refusal names the parent its row's group text states (FR-016)");
  check(refusalNamed(second, "tma_split_a") == nullptr,
        "a refused row below the answer's depth stands out of the answer "
        "(FR-016)");

  const TopdownTree third = asked(table, 3, intel);
  const TopdownRefusal* cycle = refusalNamed(third, "tma_split_a");
  check(cycle != nullptr && cycle->level == 3,
        "the same row reaches the answer once the ask reaches its level "
        "(FR-016)");
  check(third.refused.size() == 10 && third.nodes.size() == 14,
        "every refused row of every level the answer reaches stands once, and "
        "no " "refusal builds a node (FR-016)");
}

// FR-016, spec D-20 and PC-11: a level whose rows print under two objects loses
// no row.
auto rowsUnderTwoObjectsBothReach() -> void
{
  const TopdownTree second =
      asked(plantedTable(kIntelDirectory, topdownIntelRows()),
            2,
            intelIdentity(6, 85, 4));
  const TopdownNode* bandwidth =
      nodeNamed(second, qualified("tma_memory_bandwidth"));
  check(bandwidth != nullptr,
        "the row of the memory scope reaches the tree beside the rows of the "
        "core scope (FR-016, spec D-20, PC-11)");
  check(bandwidth != nullptr && bandwidth->parent == "topdown.backend_bound",
        "the memory row keeps the parent its group text states, one level "
        "above (FR-013)");
  bool coreLeaf = false;
  bool memoryLeaf = false;
  if (bandwidth != nullptr) {
    for (const std::string& event : bandwidth->events) {
      coreLeaf = coreLeaf || event.starts_with("cpu_core/");
      memoryLeaf = memoryLeaf || event.starts_with("uncore_imc_77/");
    }
  }
  check(coreLeaf && memoryLeaf,
        "one spine reads leaves of both objects, so the selection reads the "
        "whole listing wherever a row prints (FR-010, PC-11)");
  check(nodesAtLevel(second, 2) == 6,
        "no row of the level is lost between the two objects (FR-016)");
}

// FR-017, spec D-9 and D-11: the threshold text rides to the node byte for
// byte, and a row that carries none reaches an empty text.
auto thresholdRidesAsTextUnchanged() -> void
{
  TopdownTable table = plantedTable(kIntelDirectory, topdownIntelRows());
  plantThreshold(table, "tma_backend_bound", kLevelOneThreshold);
  plantThreshold(table, "tma_core_bound", kLevelTwoThreshold);
  const TopdownTree second = asked(table, 2, intelIdentity(6, 85, 4));
  const TopdownNode* backend = nodeNamed(second, kNeutralBackendBound);
  check(backend != nullptr && backend->threshold == kLevelOneThreshold,
        "the table's threshold text reaches the node byte for byte (FR-017)");
  const TopdownNode* core = nodeNamed(second, qualified("tma_core_bound"));
  check(core != nullptr && core->threshold == kLevelTwoThreshold,
        "a level-2 row carries its own text (FR-017)");
  const TopdownNode* frontend = nodeNamed(second, kNeutralFrontendBound);
  check(frontend != nullptr && frontend->threshold.empty(),
        "a row that carries no threshold reaches an empty text (FR-017)");
}

// FR-052 and FR-012: the two AMD pipeline tables name their level-2 rows
// differently, and a selection reaches the nodes of each.
auto levelTwoNamesDifferBetweenTheTwoAmdTables() -> void
{
  const TopdownTree zen4 = asked(
      plantedTable(kZen4Directory, topdownZen4Rows()), 2, amdIdentity(25, 17));
  const TopdownTree zen5 = asked(
      plantedTable(kZen5Directory, topdownZen5Rows()), 2, amdIdentity(26, 17));
  check(nodesAtLevel(zen4, 2) == 8 && nodesAtLevel(zen5, 2) == 8,
        "both pipeline tables answer eight level-2 nodes (FR-052)");
  for (const auto& pair : kZenLevelTwoNames) {
    const std::string zen4Name = qualifiedAmd(pair.first);
    const std::string zen5Name = qualifiedAmd(pair.second);
    const TopdownNode* four = nodeNamed(zen4, zen4Name);
    const TopdownNode* five = nodeNamed(zen5, zen5Name);
    check(four != nullptr && five != nullptr,
          "each table reaches its own spelling of the row behind the vendor "
          "prefix (FR-012, FR-052)");
    check(nodeNamed(zen4, zen5Name) == nullptr
              && nodeNamed(zen5, zen4Name) == nullptr,
          "one name of the pair stands in one tree, so a selection names the "
          "level-2 nodes of each table (FR-052)");
    check(four != nullptr && five != nullptr
              && four->vendorName == pair.first
              && five->vendorName == pair.second,
          "each node carries the vendor's own name beside the qualified name "
          "(FR-010)");
    check(four != nullptr && five != nullptr && four->parent == five->parent,
          "the two spellings name one parent, so the difference is a name and "
          "no shape (FR-013, FR-052)");
  }
}

// FR-013, FR-014, FR-052 and spec D-2: the eight level-2 rows of the two AMD
// pipeline tables stand row for row, `backend_bound_cpu` of Zen 4 against
// `backend_bound_by_cpu` of Zen 5 with the seven that differ the same way, each
// under the level-1 parent its own `<name>_group` token states.
auto theAmdLevelTwoRowsStandRowForRow() -> void
{
  // The level-1 parent of each row of `kZenLevelTwoNames`, in the order both
  // pipeline files spell their rows: two rows under each of the four shared
  // names, and none under the fifth name `smt_contention` of FR-011.
  constexpr std::array<std::string_view, 8> kZenLevelTwoParents {
      {kNeutralFrontendBound,
       kNeutralFrontendBound,
       kNeutralBadSpeculation,
       kNeutralBadSpeculation,
       kNeutralBackendBound,
       kNeutralBackendBound,
       kNeutralRetiring,
       kNeutralRetiring}};
  const TopdownTree zen4 = asked(
      plantedTable(kZen4Directory, topdownZen4Rows()), 2, amdIdentity(25, 17));
  const TopdownTree zen5 = asked(
      plantedTable(kZen5Directory, topdownZen5Rows()), 2, amdIdentity(26, 17));
  check(nodesAtLevel(zen4, 2) == kZenLevelTwoParents.size()
            && nodesAtLevel(zen5, 2) == kZenLevelTwoParents.size(),
        "each pipeline table answers the eight level-2 nodes the file spells "
        "(FR-052)");
  check(deepestLevel(zen4) == 2 && deepestLevel(zen5) == 2,
        "a pipeline table holds no level below the two its tokens name "
        "(FR-014)");

  for (std::size_t index = 0; index != kZenLevelTwoNames.size(); ++index) {
    const std::string_view four = kZenLevelTwoNames[index].first;
    const std::string_view five = kZenLevelTwoNames[index].second;
    const TopdownNode* left = nodeNamed(zen4, qualifiedAmd(four));
    const TopdownNode* right = nodeNamed(zen5, qualifiedAmd(five));
    check(left != nullptr && right != nullptr,
          "the row of that index reaches a node in each table (FR-052)");
    check(left != nullptr && left->level == 2 && right != nullptr
              && right->level == 2,
          "the `PipelineL2` token of each row of the pair states level 2 "
          "(FR-014)");
    check(left != nullptr && left->parent == kZenLevelTwoParents[index]
              && right != nullptr && right->parent == kZenLevelTwoParents[index],
          "each row of the pair stands under the level-1 parent its own "
          "`<name>_group` token states (FR-013)");
  }

  // The pair FR-052 names. The row under `topdown.backend_bound` is
  // `backend_bound_cpu` in the Zen 4 file and `backend_bound_by_cpu` in the Zen
  // 5 file, and both stand at the same index of the pair list above.
  const TopdownNode* cpu = nodeNamed(zen4, qualifiedAmd("backend_bound_cpu"));
  const TopdownNode* byCpu =
      nodeNamed(zen5, qualifiedAmd("backend_bound_by_cpu"));
  check(cpu != nullptr && byCpu != nullptr
            && cpu->parent == kNeutralBackendBound
            && byCpu->parent == kNeutralBackendBound,
        "`backend_bound_cpu` and `backend_bound_by_cpu` stand under "
        "`topdown.backend_bound` (FR-013, FR-052)");

  // The parents the eight rows state are the four shared names of FR-011, so
  // the fifth name an AMD table answers alone carries no row below it.
  bool underSmt = false;
  for (const TopdownNode& node : zen4.nodes) {
    underSmt = underSmt || node.parent == kNeutralSmtContention;
  }
  check(!underSmt && nodeNamed(zen4, kNeutralSmtContention) != nullptr,
        "the fifth name of an AMD table stands as a root of its own and as no "
        "row's parent (FR-011, FR-013)");
}

// FR-010: a node reaches the published view with the spine of its row and the
// events that spine reads.
auto aNodeCarriesItsFormulaAndItsEventList() -> void
{
  const TopdownTable table = plantedTable(kIntelDirectory, topdownIntelRows());
  const TopdownTree second = asked(table, 2, intelIdentity(6, 85, 4));
  for (const TopdownNode& node : second.nodes) {
    const TopdownRow* row = rowNamed(table, node.vendorName);
    check(row != nullptr && row->published,
          "every node names a row the table published (FR-010, FR-013)");
    check(row != nullptr && sameSpine(node.formula.core, row->formula.core),
          "the node carries the spine of its row, so a caller folds what the "
          "row states (FR-010)");
    check(row != nullptr && node.events == leafAddresses(*row),
          "the node's events are the addresses that spine reads, which is what "
          "the planner groups its passes on (FR-010, FR-021)");
  }
  const TopdownNode* backend = nodeNamed(second, kNeutralBackendBound);
  check(backend != nullptr && backend->events.size() == 4,
        "a level-1 row that names three other rows reaches the leaves all four "
        "spines read (FR-010)");
  check(backend != nullptr && backend->vendorName == "tma_backend_bound",
        "the shared name stands beside the vendor's own name (FR-010, FR-011)");

  const TopdownTree first = asked(
      plantedTable(kZen4Directory, topdownZen4Rows()), 1, amdIdentity(25, 17));
  const TopdownNode* frontend = nodeNamed(first, kNeutralFrontendBound);
  check(frontend != nullptr && frontend->events.size() == 1
            && frontend->events.front() == "cpu_core/fixture_cycles",
        "a row that divides by the dispatch helper reaches the leaf the helper "
        "expands to (FR-010)");
}

// FR-009: two calls over one identity and one table answer one tree.
auto twoCallsOnOneIdentityAnswerOneTree() -> void
{
  const TopdownTable zen4 = plantedTable(kZen4Directory, topdownZen4Rows());
  const CpuIdentity identity = amdIdentity(25, 17);
  const std::string first = treeText(asked(zen4, 2, identity));
  const std::string second = treeText(asked(zen4, 2, identity));
  check(first == second,
        "two calls over one identity and one table answer one tree (FR-009)");
  check(first.find("topdown.smt_contention") != std::string::npos
            && first.find("backend_bound_cpu") != std::string::npos,
        "the text names the fields a tree carries, so the equality above reads "
        "a tree that holds rows (FR-009)");
  const TopdownTree other = asked(
      plantedTable(kZen5Directory, topdownZen5Rows()), 2, amdIdentity(26, 17));
  check(treeText(other) != first,
        "another table answers another tree, so the equality above is no match "
        "of empty texts (FR-009)");
}

// FR-003 and spec D-13: the four fields production reads, the two fields no
// reader fills, and the two fields a test hands in.
auto cpuIdentityCarriesTheReadersFourFields() -> void
{
  const CpuIdentity published = cpuIdentity();
  const PmuIdent read = pmuIdentCurrent();
  check(published.vendor == read.vendor,
        "the vendor text is the one the library's own cpu reader answers "
        "(FR-003)");
  check(published.family == read.family,
        "the family is the reader's own answer, in the mapping file's decimal "
        "spelling (FR-003)");
  check(published.model == read.model,
        "the model is the reader's own answer, in the mapping file's spelling "
        "(FR-003)");
  check(published.stepping == read.stepping,
        "the stepping is the reader's own answer, and a negative value names "
        "no stepping class (FR-003)");
  check(published.coreType.empty(),
        "no reader fills the core type of a hybrid part, so a production "
        "identity leaves it empty (FR-003, spec D-13)");
  check(!published.midr.has_value(),
        "no reader fills an Arm `MIDR`, so a production identity leaves it "
        "unset (FR-003, spec D-13)");

  const CpuIdentity handed = CpuIdentity {
      .vendor = "ARM",
      .family = 0,
      .model = 0,
      .stepping = -1,
      .coreType = std::string {kHandedCoreType},
      .midr = std::optional<std::uint32_t> {kHandedMidr},
  };
  check(handed.coreType == kHandedCoreType,
        "the record carries the core type a test hands in (FR-003)");
  check(handed.midr.has_value() && *handed.midr == kHandedMidr,
        "the record carries the `MIDR` a test hands in (FR-003)");
}

// FR-001 and FR-003: the identity is the whole input to the selection, and the
// first mapping-file row that hits names the directory.
auto selectsTheTableOfTheIdentity() -> void
{
  check(topdownTable(intelIdentity(6, 85, 4)).source.directory
            == kIntelDirectory,
        "the model-85 stepping-4 part selects the Skylake-X table, the row the "
        "mapping file splits from the stepping-5 Cascade Lake row of the same "
        "model (FR-001)");
  check(topdownTable(amdIdentity(25, 17)).source.directory == kZen4Directory,
        "the family-25 model-17 part selects the Zen 4 pipeline table, the "
        "first of the two rows that model hits (FR-001)");
  check(topdownTable(amdIdentity(26, 17)).source.directory == kZen5Directory,
        "the family-26 model-17 part selects the Zen 5 pipeline table, ahead "
        "of the Zen 6 row that model also hits (FR-001)");

  const TopdownTable unmatched = topdownTable(unmatchedIdentity());
  check(unmatched.source.directory.empty(),
        "an identity no row of the mapping file matches selects no directory, "
        "and an empty directory names that answer (FR-001)");
  check(unmatched.rows.empty(),
        "an identity the mapping file matched no row for answers no row "
        "(FR-001)");

  // The identity the library's own reader answers selects the table its own
  // four fields match, so the record a test hands in and the record the reader
  // answers reach one directory (FR-003).
  check(topdownTable(cpuIdentity()).source.directory
            == pmuSelectDirectory(pmuIdentCurrent()),
        "the identity the library's own reader answers selects the directory "
        "that reader's four fields match (FR-003)");

  // The two fields no reader answers carry no weight in a selection: the
  // mapping file matches the vendor, the family, the model and the stepping,
  // and no row of it reads a core type or an `MIDR` (spec D-13).
  CpuIdentity hybrid = intelIdentity(6, 85, 4);
  hybrid.coreType = std::string {kHandedCoreType};
  hybrid.midr = std::optional<std::uint32_t> {kHandedMidr};
  check(topdownTable(hybrid).source.directory == kIntelDirectory,
        "a core type and a `MIDR` a test hands in move no selection, because "
        "no mapping-file row reads either (FR-003, spec D-13)");
}

// FR-002 and FR-009: one call answers from what it is handed, and nothing
// else moves the answer.
auto oneCallReadsNoSourceOfItsOwn() -> void
{
  const std::size_t descriptors = openDescriptorCount();
  const std::size_t mappings = mappingCount();
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const CpuIdentity amd = amdIdentity(25, 17);
  const TopdownTable table = topdownTable(amd);

  check(openDescriptorCount() == descriptors,
        "one selection leaves the process holding the descriptors it held "
        "before, so no event stands open behind the answer (FR-002)");
  check(mappingCount() == mappings,
        "one selection leaves the process holding the mappings it held before, "
        "so no sampled event page stands behind the answer (FR-002)");

  // Two identities that differ answer two directories. A call that read this
  // cpu of its own would answer one directory for both, whatever it was handed
  // (FR-002).
  check(table.source.directory == kZen4Directory,
        "a Zen 4 identity answers the Zen 4 table on this host (FR-002)");
  check(topdownTable(intel).source.directory == kIntelDirectory,
        "an Intel identity answers the Intel table in the same process, so the "
        "answer follows the identity and the cpu of this host entered nothing "
        "(FR-002)");

  // One answer for one input: a figure that moves between calls, a counter or
  // a clock, would part two calls over one identity (FR-009).
  check(sourceText(topdownTable(amd)) == sourceText(table),
        "two selections of one identity answer one source record, so nothing "
        "that moves between calls stands in it (FR-009)");

  // The tree pass over a planted table reads the rows it is handed. A row it
  // reached carries a spine, so the two counts below measure a pass that walked
  // rows and read their leaves (FR-002).
  const TopdownTable planted =
      plantedTable(kIntelDirectory, topdownIntelRows());
  const std::size_t plantedDescriptors = openDescriptorCount();
  const std::size_t plantedMappings = mappingCount();
  const TopdownTree plantedTree = asked(planted, 3, intel);
  check(openDescriptorCount() == plantedDescriptors,
        "one tree call leaves the process holding the descriptors it held "
        "before, so no event stands open behind the tree (FR-002)");
  check(mappingCount() == plantedMappings,
        "one tree call leaves the process holding the mappings it held before, "
        "so no sampled event page stands behind the tree (FR-002)");
  check(plantedTree.nodes.size() == 14 && plantedTree.refused.size() == 10,
        "the tree call measured above reached rows, leaves and refusals "
        "(FR-002, FR-016)");

  // A tree over the rows of the selected directory reads the verdicts the
  // metric layer resolved once for that directory, which is the resolution the
  // FR-002 claim rests on (FR-016).
  const std::size_t resolvedDescriptors = openDescriptorCount();
  const std::size_t resolvedMappings = mappingCount();
  const auto resolved = topdownTree(selection(amd, 2));
  check(resolved.has_value(),
        "a directory the mapping file names answers a tree over its own rows "
        "(FR-001, FR-016)");
  check(openDescriptorCount() == resolvedDescriptors,
        "the tree over the selected directory opens no event of its own, because "
        "its verdicts come from the table that resolved once (FR-002)");
  check(mappingCount() == resolvedMappings,
        "the tree over the selected directory samples no counter, so no page "
        "stands mapped behind its rows (FR-002)");
  if (resolved.has_value()) {
    check(
        resolved->source.directory == kZen4Directory,
        "the tree carries the source record of the directory it read (FR-015)");
    check(resolved->nodes.empty() && resolved->refused.size() == 13,
          "the rows of the selected directory reach the tree with the verdicts "
          "this scripted host answered, and a refused row builds no node "
          "(FR-016)");
  }
}

// The digest the registry states beside one file name of a directory, and an
// empty text where the directory holds no file of that name. The registry
// carries the digest the embedder read out of the manifest line for the file
// (T018), so this is the manifest digest as far as a run can read it without
// opening the vendored tree (spec D-12).
[[nodiscard]] auto registryDigest(const EmbeddedDir& registry,
                                  const std::string_view name) -> std::string
{
  for (std::size_t index = 0; index != registry.fileCount; ++index) {
    if (registry.files[index].name == name) {
      return {registry.files[index].sha256};
    }
  }
  return {};
}

// FR-015 and FR-033, spec D-12, data-model E-02 and E-03: the source record a
// production table reads. The case asks the table of the identity this host's
// own reader answers, the way the production AMD scenarios do, so only the
// fill inside `topdownTable()` can answer it and no planted record stands in
// any part of it.
//
// The expected record comes from the two reads T018 and T019 put in place: the
// registry, whose index holds a directory's files in the order the embedder
// sorted them with one digest each, and the parsed table, which tags every row
// with the file it came from. `counters_topdown_source_test` pins both of
// those reads against the vendored `RECORD`, so a record that equals them
// equals the manifest this build read (PC-12). How many files contribute is a
// fact of the selected directory, so no case here carries one host's count as
// a literal.
auto productionTableCarriesItsSourceRecord() -> void
{
  const TopdownTable table = topdownTable(cpuIdentity());
  if (table.source.directory.empty()) {
    // The cpu of this host matches no row of the mapping file, so it selects
    // no directory and no record stands here to measure. A build that carries
    // no mapping file at all reported its reason and skipped already (FR-001).
    return;
  }
  // A directory arrives with a trailing separator and the registry names it
  // without one, which is the spelling `pmuLoadTable` reads (FR-036).
  std::string_view key {table.source.directory};
  if (key.ends_with('/')) {
    key.remove_suffix(1);
  }
  const EmbeddedDir* registry = embeddedFindDir(key);
  check(registry != nullptr,
        "the registry holds the bytes of the directory the selection named "
        "(FR-015)");
  if (registry == nullptr) {
    return;
  }

  check(!table.source.reference.empty(),
        "a production table states a reference (FR-015, FR-033)");
  check(table.source.reference == registry->reference,
        "the reference is the `ref:` line of the manifest the embedder read "
        "for this directory (FR-015, spec D-12)");

  const std::vector<PmuTableEntry>& rows = pmuLoadTable(table.source.directory);
  check(!rows.empty(), "the selected directory parses its rows (FR-015)");

  // The files that contributed a row, in the order the index holds them, which
  // is the sorted order the embedder emitted. A file that holds no row of the
  // directory stands in no record (FR-015).
  std::vector<std::string> contributing;
  for (std::size_t index = 0; index != registry->fileCount; ++index) {
    const std::string_view name {registry->files[index].name};
    const bool holdsRow = std::ranges::any_of(rows,
                                              [name](const PmuTableEntry& row)
                                              { return row.fileName == name; });
    if (holdsRow) {
      contributing.push_back(std::string {name});
    }
  }
  check(!contributing.empty(),
        "a selected directory holds a file that contributed a row (FR-015)");
  check(table.source.files.size() == contributing.size(),
        "the record holds one entry per file that contributed a row (FR-015, "
        "FR-033)");
  for (std::size_t index = 0;
       index != table.source.files.size() && index != contributing.size();
       ++index)
  {
    const TableFile& file = table.source.files[index];
    check(!file.name.empty(),
          "every entry of the record names a file (FR-015)");
    check(!file.manifestHash.empty(),
          "every entry of the record carries a digest (FR-015, PC-12)");
    check(file.name == contributing[index],
          "the entry names the file the index gives at its own position, so "
          "the record stands in the embedder's sorted order (FR-015)");
    check(file.manifestHash == registryDigest(*registry, contributing[index]),
          "the entry carries the digest the manifest line beside that name "
          "states (FR-015, PC-12)");
  }
}

// The five lines `TopdownUnsupported::message()` owns, each quoted from the
// answer table of `specs/042-topdown/contracts/topdown-selection.md`: the tag,
// the vendor, the identity in the spelling that table states, and the reason
// the kind states (FR-004 to FR-007, FR-040, spec D-14).
constexpr std::string_view kUnknownVendorText {
    "topdown: NoSuchVendorXXXXX family 999 model 999: no vendored table "
    "carries " "this vendor"};
constexpr std::string_view kNoDirectoryText {
    "topdown: GenuineIntel family 15 model 6 stepping 5: the mapping file "
    "names " "no table for this identity"};
constexpr std::string_view kHybridText {
    "topdown: GenuineIntel family 6 model 151 stepping 2: the selected table "
    "'arch/x86/alderlake/' publishes rows for two core scopes, and this feature "
    "makes no claim for a hybrid part"};
constexpr std::string_view kArmText {
    "topdown: Arm 0x410fd490: no Arm table is vendored"};
constexpr std::string_view kNoRowsText {
    "topdown: AuthenticAMD family 25 model 15: the selected table "
    "'arch/x86/amdzen3/' holds no row of any top-down group"};

// The same `HYBRID` line with the label of the second directory of the vendored
// tree whose rows carry the two core scopes: the rule answers it out of that
// directory's own rows, the way it answers the first (FR-006).
constexpr std::string_view kPantherLakeHybridText {
    "topdown: GenuineIntel family 6 model 151 stepping 2: the selected table "
    "'arch/x86/pantherlake/' publishes rows for two core scopes, and this "
    "feature makes no claim for a hybrid part"};

// The identity of an Arm part, spelled the way such a part states itself with
// its `MIDR`, and no vendor text: no reader of this feature answers a vendor
// for an Arm part (FR-007, spec D-13).
[[nodiscard]] auto armIdentity() -> CpuIdentity
{
  CpuIdentity identity;
  identity.midr = std::optional<std::uint32_t> {kHandedMidr};
  return identity;
}

// The hybrid identity of the vendored mapping file: the model 97 part the first
// hybrid row of the mapping file names (mapfile.csv:2).
[[nodiscard]] auto hybridIdentity() -> CpuIdentity
{
  return intelIdentity(6, 0x97, 2);
}

// The identity whose model the mapping file gives `pantherlake`, the directory
// that carries the two core scopes beside no level row (mapfile.csv:29).
[[nodiscard]] auto pantherLakeIdentity() -> CpuIdentity
{
  return intelIdentity(6, 0xCC, 5);
}

// The identity of the AMD part that predates the dispatch-slot table: family 25
// model 15, which the mapping file gives `amdzen3`.
[[nodiscard]] auto zen3Identity() -> CpuIdentity
{
  return amdIdentity(25, 15);
}

// The refusal one selection answers, or the failure of a case that asked for a
// refusal and got a tree (FR-004 to FR-007).
[[nodiscard]] auto askedRefusal(const TopdownSelection& options)
    -> TopdownUnsupported
{
  const auto answered = topdownTree(options);
  if (answered.has_value()) {
    std::fprintf(stderr,
                 "COUNTERS TOPDOWN SELECT TEST FAIL: the selection answered a "
                 "tree where its case asked a refusal\n");
    std::exit(1);
  }
  return answered.error();
}

// Whether one line carries the vendor, the identity and the reason FR-040 asks
// of every refusal.
[[nodiscard]] auto quotesAll(const std::string& line,
                             const std::string_view vendor,
                             const std::string_view identityText,
                             const std::string_view reason) -> bool
{
  return line.find(vendor) != std::string::npos
      && line.find(identityText) != std::string::npos
      && line.find(reason) != std::string::npos;
}

// FR-004 to FR-007, FR-040 and spec D-14: each kind answers one line that names
// the vendor, the identity and the reason, and the line stands as the contract
// spells it. One synthetic identity per kind, and no identity of this host
// stands in any of them (spec D-13).
auto unsupportedAnswersQuoteTheIdentity() -> void
{
  const TopdownUnsupported unknown =
      askedRefusal(selection(unmatchedIdentity(), 1));
  check(unknown.kind == TopdownUnsupport::UNKNOWN_VENDOR,
        "a vendor the vendored tree holds no table of answers UNKNOWN_VENDOR "
        "(FR-004)");
  check(quotesAll(unknown.message(),
                  "NoSuchVendorXXXXX",
                  "family 999 model 999",
                  "no vendored table carries this vendor"),
        "the UNKNOWN_VENDOR line names the vendor, the family and model, and "
        "the reason (FR-004, FR-040)");
  check(unknown.message() == kUnknownVendorText,
        "the UNKNOWN_VENDOR line stands as the contract spells it (FR-040)");

  // The mapping file holds rows for two vendors alone, so a family outside both
  // of them names no directory at all.
  const TopdownUnsupported missed =
      askedRefusal(selection(intelIdentity(15, 6, 5), 1));
  check(missed.kind == TopdownUnsupport::NO_DIRECTORY,
        "an identity the mapping file matched no row for answers NO_DIRECTORY "
        "(FR-005)");
  check(quotesAll(missed.message(),
                  "GenuineIntel",
                  "family 15 model 6 stepping 5",
                  "the mapping file names no table for this identity"),
        "the NO_DIRECTORY line names the vendor, the family and model, and the "
        "reason (FR-005, FR-040)");
  check(missed.message() == kNoDirectoryText,
        "the NO_DIRECTORY line stands as the contract spells it (FR-040)");
  check(missed.detail.empty(),
        "a mapping-file miss read no directory, so it states none (FR-005)");

  const TopdownUnsupported arm = askedRefusal(selection(armIdentity(), 1));
  check(arm.kind == TopdownUnsupport::ARM,
        "an `MIDR` answers ARM before any table is read (FR-007)");
  check(arm.detail == "no Arm table is vendored",
        "the Arm refusal carries the sentence that table states (FR-007)");
  check(arm.message() == kArmText,
        "the ARM line names the `MIDR` and states that no Arm table is "
        "vendored, as the contract spells it (FR-007, FR-040)");
  check(
      quotesAll(arm.message(), "Arm", "0x410fd490", "no Arm table is vendored"),
      "the ARM line names the vendor, the `MIDR` and the reason (FR-040)");

  const TopdownUnsupported hybrid = askedRefusal(
      selection(hybridIdentity(),
                2,
                plantedTable(kHybridDirectory, topdownHybridRows())));
  check(hybrid.kind == TopdownUnsupport::HYBRID,
        "a directory whose rows carry the two core scopes answers HYBRID "
        "(FR-006)");
  check(hybrid.detail == kHybridDirectory,
        "the hybrid refusal names the directory it read (FR-006)");
  check(hybrid.message() == kHybridText,
        "the HYBRID line names the vendor, the family and model, the directory "
        "and the claim this feature makes no hybrid part (FR-006, FR-040)");

  // The same rows under the label of a second two-scope directory answer the
  // same kind with that label in its line, so the answer follows the scopes a
  // directory's rows publish, which is the rule FR-006 states, beside no list
  // of directory names.
  const TopdownUnsupported second = askedRefusal(
      selection(hybridIdentity(),
                2,
                plantedTable(kNoLevelDirectory, topdownHybridRows())));
  check(second.kind == TopdownUnsupport::HYBRID
            && second.message() == kPantherLakeHybridText,
        "a second two-scope directory answers the same kind with its own label "
        "in the line (FR-006)");

  // The Zen 3 directory holds metric rows and no row of any top-down group, so
  // the rule of FR-007 answers there over the vendored data of its own.
  const TopdownTable zen3 = topdownTable(zen3Identity());
  check(zen3.source.directory == "arch/x86/amdzen3/",
        "the mapping file gives the family 25 model 15 part the Zen 3 "
        "directory " "(FR-001)");
  const TopdownUnsupported empty = askedRefusal(selection(zen3Identity(), 1));
  check(empty.kind == TopdownUnsupport::NO_TOPDOWN_ROWS,
        "a directory that holds no row of any top-down group answers "
        "NO_TOPDOWN_ROWS (FR-007)");
  check(empty.detail == zen3.source.directory,
        "the no-rows refusal names the directory it read (FR-007)");
  check(empty.message() == kNoRowsText,
        "the NO_TOPDOWN_ROWS line names the vendor, the family and model, the "
        "directory and the reason, as the contract spells it (FR-007, FR-040)");
  check(quotesAll(empty.message(),
                  "AuthenticAMD",
                  "family 25 model 15",
                  "holds no row of any top-down group"),
        "the NO_TOPDOWN_ROWS line names the vendor, the family and model, and "
        "the reason (FR-007, FR-040)");
}

// FR-006: the scope rule reads the scopes a directory's own event rows publish,
// and one core scope answers a tree.
auto aHybridDirectoryRefusesByItsRows() -> void
{
  const CpuIdentity identity = hybridIdentity();
  check(topdownTable(identity).source.directory == kHybridDirectory,
        "the mapping file gives this hybrid identity the directory the fixture "
        "spells for two core scopes (FR-001, FR-006)");
  const TopdownUnsupported why = askedRefusal(selection(
      identity, 2, plantedTable(kHybridDirectory, topdownHybridRows())));
  check(why.kind == TopdownUnsupport::HYBRID,
        "the fixture directory whose event rows carry both core scopes answers "
        "HYBRID (FR-006)");
  check(why.detail == std::string {kHybridDirectory},
        "the answer names the directory whose rows it read (FR-006)");
  check(why.message().find("'arch/x86/alderlake/'") != std::string::npos,
        "the line carries the label of that directory between the quote marks "
        "the contract spells (FR-006, FR-040)");
}

// FR-006: a directory of one core scope answers a tree, and so does one whose
// rows carry no scope token at all, which is how the three hybrid-shaped
// directories of the vendored tree read.
auto aOneScopeDirectorySelectsNormally() -> void
{
  const TopdownTree one =
      asked(plantedTable(kIntelDirectory, topdownIntelRows()),
            2,
            intelIdentity(6, 85, 4));
  check(one.depth == 2 && !one.nodes.empty(),
        "the fixture directory whose every event row carries one core scope "
        "answers a tree (FR-006)");

  // The hybrid fixture rows again, this time under the label of a directory
  // whose rows carry neither core token: the same rows answer a tree there, so
  // the two-scope fact of the rows above is what refused them.
  const TopdownTree tokenless =
      asked(plantedTable("arch/x86/grandridge/", topdownHybridRows()),
            2,
            intelIdentity(6, 0xB6, 0));
  check(tokenless.depth == 2 && !tokenless.nodes.empty(),
        "a directory whose event rows carry no core scope token answers a tree "
        "over the same rows (FR-006)");
}

// FR-006 and FR-007: a directory that carries the two core scopes beside no
// level row answers NO_TOPDOWN_ROWS, because the no-rows rule stands ahead of
// the scope rule.
auto noLevelRowRefusesBeforeTheScopeRule() -> void
{
  const CpuIdentity identity = pantherLakeIdentity();
  check(topdownTable(identity).source.directory == kNoLevelDirectory,
        "the mapping file gives this identity the directory the fixture spells "
        "for two core scopes and no level row (FR-001, FR-007)");
  // The event rows of the two scopes alone: the shape that directory answers in
  // the vendored tree, where 606 event rows stand and no metric row stands.
  const TopdownTable table =
      plantedTable(kNoLevelDirectory, topdownHybridEventRows());
  check(table.rows.empty(),
        "the planted directory holds event rows alone, so no row of any "
        "top-down group stands in it (FR-007)");
  const TopdownUnsupported why = askedRefusal(selection(identity, 1, table));
  check(why.kind == TopdownUnsupport::NO_TOPDOWN_ROWS,
        "the no-rows answer outranks the scope answer over a directory that "
        "publishes both scopes and no level row (FR-006, FR-007)");
  check(why.detail == kNoLevelDirectory,
        "the answer names the directory it read (FR-007)");
  check(
      why.message().find("holds no row of any top-down group")
          != std::string::npos,
      "the line states the reason of the rule that answered (FR-007, FR-040)");
}

}  // namespace

auto main() -> int
{
  // The selection reads the mapping file out of the bytes this build
  // embedded. A build that carries no architecture directory answers no
  // selection at all, which leaves this unit nothing to measure, so it reports
  // the reason and exits 2, the skip code the `test/tier_labels.sh` row of T001
  // reserves.
  if (embeddedFindDir("arch/x86") == nullptr) {
    std::printf("counters_topdown_select_test SKIP: this build carries no "
                "vendored architecture mapping file, so a selection answers no "
                "directory (FR-001)\n");
    return 2;
  }
  // The planted tables resolve through the scripted host of the fixture, and
  // registration has to precede the first use of the system (FR-036).
  if (!registerMetricHost()) {
    std::printf("counters_topdown_select_test SKIP: the scripted host cannot "
                "register against a system that opened already, so no case can "
                "resolve a planted row (FR-036)\n");
    return 2;
  }
  cpuIdentityCarriesTheReadersFourFields();
  selectsTheTableOfTheIdentity();
  oneCallReadsNoSourceOfItsOwn();
  productionTableCarriesItsSourceRecord();
  theFillOfOneNameFillsTheRecordOrLeavesIt();
  levelOneCarriesTheSharedNames();
  parentsComeFromTheGroupText();
  theLevelTokenOfEachGroupSpellingStatesTheLevel();
  aGroupTextOfNoLevelOrOfNoParentStandsOutOfTheTree();
  everyDeeperNodeNamesTheParentItsOwnGroupTokenStates();
  levelDeeperThanTheTableClamps();
  anAskDeeperThanTheTableHoldsAnswersTheLevelItHolds();
  refusedRowsStayInTheDocument();
  rowsUnderTwoObjectsBothReach();
  thresholdRidesAsTextUnchanged();
  levelTwoNamesDifferBetweenTheTwoAmdTables();
  theAmdLevelTwoRowsStandRowForRow();
  aNodeCarriesItsFormulaAndItsEventList();
  twoCallsOnOneIdentityAnswerOneTree();
  unsupportedAnswersQuoteTheIdentity();
  aHybridDirectoryRefusesByItsRows();
  aOneScopeDirectorySelectsNormally();
  noLevelRowRefusesBeforeTheScopeRule();
  std::printf("counters_topdown_select_test PASS: the selection answers the "
              "table of the identity it is handed, and the tree of the rows it "
              "is given\n");
  return 0;
}
