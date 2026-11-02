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
using sg::counters::topdownTable;
using sg::counters::TopdownTable;
using sg::counters::topdownTree;
using sg::counters::TopdownTree;
using sg::counters::TopdownUnsupported;
using sg::counters::detail::EmbeddedDir;
using sg::counters::detail::embeddedFindDir;
using sg::counters::detail::ExprCore;
using sg::counters::detail::ExprNode;
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
using sg::test::kIntelDirectory;
using sg::test::kLevelOneThreshold;
using sg::test::kLevelTwoThreshold;
using sg::test::kNeutralBackendBound;
using sg::test::kNeutralBadSpeculation;
using sg::test::kNeutralFrontendBound;
using sg::test::kNeutralRetiring;
using sg::test::kNeutralSmtContention;
using sg::test::kZen4Directory;
using sg::test::kZen5Directory;
using sg::test::kZenLevelTwoNames;
using sg::test::MetricDefinition;
using sg::test::MetricRow;
using sg::test::registerMetricHost;
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

// The four level-1 rows of the Intel fixture table, each pair the table's own
// name and the shared name of FR-011, in the order the table spells them.
constexpr std::array<std::pair<std::string_view, std::string_view>, 4>
    kIntelLevelOneNames {{{"tma_frontend_bound", "topdown.frontend_bound"},
                          {"tma_bad_speculation", "topdown.bad_speculation"},
                          {"tma_backend_bound", "topdown.backend_bound"},
                          {"tma_retiring", "topdown.retiring"}}};

// FR-011, SC-01 and FR-052: the four shared names every table answers, the
// fifth an AMD table alone, and no parent at level 1.
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
  levelOneCarriesTheSharedNames();
  parentsComeFromTheGroupText();
  levelDeeperThanTheTableClamps();
  refusedRowsStayInTheDocument();
  rowsUnderTwoObjectsBothReach();
  thresholdRidesAsTextUnchanged();
  levelTwoNamesDifferBetweenTheTwoAmdTables();
  aNodeCarriesItsFormulaAndItsEventList();
  twoCallsOnOneIdentityAnswerOneTree();
  std::printf("counters_topdown_select_test PASS: the selection answers the "
              "table of the identity it is handed, and the tree of the rows it "
              "is given\n");
  return 0;
}
