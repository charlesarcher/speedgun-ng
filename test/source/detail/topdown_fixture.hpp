#ifndef SG_TEST_DETAIL_TOPDOWN_FIXTURE_HPP
#define SG_TEST_DETAIL_TOPDOWN_FIXTURE_HPP

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "metric_fixture.hpp"

// ============================================================================
// The synthetic top-down tables the selection reads (specs/042-topdown T009;
// FR-004 to FR-017, FR-052, spec D-13).
//
// Five directories stand here as rows of `PmuTableEntry`, the shape
// `metric_fixture.hpp` builds with `fixtureEvent` and `fixtureMetric`, so one
// shape serves both fixtures. The scope of a row is its `Unit` text: the parse
// reads it into `PmuTableEntry::unit` (`table_parse.cpp:419` for a metric row,
// `:515` for an event row) and the resolution carries it as
// `ParsedMetricRow::scope` (`metric_formula.hpp:107-114`).
//
// The row names, group texts, scale texts and threshold texts follow the
// pinned tree at `external/pmu-events` in shape, with the fixture's own event
// names inside the formulas, so a resolution reaches the scripted leaves of
// `registerMetricHost` and a host with a real catalog answers none of them.
// The real Intel table holds 102 level rows in one file and the two AMD
// pipeline tables hold 14 rows each; these tables hold the same shapes at
// fixture scale.
//
//   * `topdownIntelRows()`: the Intel table of `kIntelDirectory`. Level groups
//     spell `TopdownL1` to `TopdownL6` and a parent token spells
//     `tma_<name>_group`, the Intel spelling FR-013 names, where the stem is a
//     row of the same directory. A `tma_L<n>_group` token stands beside it as
//     the level token of the real table, whose stem names no row. Four level-1
//     rows, seven level-2 rows and eleven rows across levels 3 to 6 form the
//     chains, so a node's level stands one below its parent's level (FR-013,
//     FR-014).
//   * `topdownIntelShallowRows()`: the same table with its two deepest levels
//     dropped, so the deepest level it holds is 4 and a request for level 6
//     clamps (FR-008, FR-020).
//   * `topdownZen4Rows()` and `topdownZen5Rows()`: the two AMD pipeline tables
//     of FR-052. Level groups spell `PipelineL1` and `PipelineL2` and a parent
//     token spells `<name>_group`, the AMD spelling, with no `tma_` prefix.
//     Five level-1 rows carry the four shared names plus `smt_contention`, and
//     eight level-2 rows carry names that differ between the two tables: Zen 4
//     names `backend_bound_cpu` where Zen 5 names `backend_bound_by_cpu`, and
//     seven rows differ the same way (`kZenLevelTwoNames` pairs them). The
//     scale text differs too, the way the pinned tables spell it (FR-011,
//     FR-052).
//   * `topdownHybridRows()`: a hybrid directory of `kHybridDirectory` whose
//     event rows carry the two core scopes `cpu_core` and `cpu_atom` beside
//     rows that carry level groups, so the scope rule reads that data
//     (FR-006). `topdownHybridEventRows()` alone is the directory of
//     `kNoLevelDirectory` that carries both scopes and no level row, the shape
//     `pantherlake` answers in the pinned tree.
//   * `topdownIntelRows()` is also the directory of one scope: every event row
//     of it carries `cpu_core` alone, so a selection reads a tree there
//     (FR-006, SC-12).
//
// The group-free helper row is `total_dispatch_slots`, the row both AMD tables
// carry with no `MetricGroup` key at all. It holds a formula and a scale and
// names no level, so it reaches no node and states no refusal (FR-014).
//
// `tma_memory_bandwidth` sits at level 2 with a scope of its own, so level 2 of
// the Intel table prints under two objects: the core object its six siblings
// reach and the memory object that row reaches (FR-010, FR-016).
//
// `tma_orphan_row` carries a level group and a parent token whose stem names no
// row, so the tree holds no node for it and no part of the feature invents one
// (FR-013).
//
// The refused rows carry one refusal kind each, and the reason text is the
// parse's own. `topdownIntelRefusalRows()` names each kind beside the row that
// answers it, and every one of them carries a level group, so a refusal stands
// in the record of a level where a published row would have stood (FR-016).
//
// The two threshold texts stand as `kLevelOneThreshold` and
// `kLevelTwoThreshold`. The pinned Intel file carries `MetricThreshold` on
// every level row and the two AMD pipeline files carry none, so the fixture
// spells the text at the two Intel rows a case reads. specs/042-topdown T020
// lands the row field that carries the text from the table file; until that
// field stands, a test plants the text at the row it plants and reads it back
// unchanged (FR-017, spec D-9, D-11).
//
// The header holds no assertion, so it carries no case of its own. The first
// unit that reads it is `counters_topdown_select_test.cpp`, which T010 writes
// and T022 turns green.
// ============================================================================

namespace sg::test
{

// The directory labels the five fixture tables answer, spelled as the pinned
// mapping file spells a directory. These are labels of fixture data: no
// mapping-file row moves, and a selection reaches a fixture table through the
// rows it is handed (spec D-13).
inline constexpr std::string_view kIntelDirectory {"arch/x86/skylakex/"};
inline constexpr std::string_view kZen4Directory {"arch/x86/amdzen4/"};
inline constexpr std::string_view kZen5Directory {"arch/x86/amdzen5/"};
inline constexpr std::string_view kHybridDirectory {"arch/x86/alderlake/"};
inline constexpr std::string_view kNoLevelDirectory {"arch/x86/pantherlake/"};

// The two core scopes a hybrid part publishes, spelled as the pinned hybrid
// tables spell them, and the memory-class scope `metric_fixture.hpp` gives its
// uncore rows. The hybrid rule reads the two core scopes of a directory's
// event rows and no other scope (FR-006).
inline constexpr std::string_view kCoreScope {"cpu_core"};
inline constexpr std::string_view kAtomScope {"cpu_atom"};
inline constexpr std::string_view kMemoryScope {"iMC"};

// The five vendor-neutral level-1 names of FR-011: the four every table
// answers and the fifth an AMD table alone.
inline constexpr std::string_view kNeutralFrontendBound {
    "topdown.frontend_bound"};
inline constexpr std::string_view kNeutralBadSpeculation {
    "topdown.bad_speculation"};
inline constexpr std::string_view kNeutralBackendBound {
    "topdown.backend_bound"};
inline constexpr std::string_view kNeutralRetiring {"topdown.retiring"};
inline constexpr std::string_view kNeutralSmtContention {
    "topdown.smt_contention"};

// The five level-1 row names both AMD tables carry, in the table order the
// pinned pipeline files spell them.
inline constexpr std::array<std::string_view, 5> kAmdLevelOneNames {
    "frontend_bound",
    "bad_speculation",
    "backend_bound",
    "smt_contention",
    "retiring",
};

// The eight level-2 row names of the two AMD pipeline tables, Zen 4 first and
// Zen 5 beside it, in the table order both files spell. Each pair names the
// same parent, so a selection reaches a differently named node out of each
// table (FR-052).
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 8>
    kZenLevelTwoNames {
        {
            {"frontend_bound_latency", "frontend_bound_by_latency"},
            {"frontend_bound_bandwidth", "frontend_bound_by_bandwidth"},
            {"bad_speculation_mispredicts", "bad_speculation_from_mispredicts"},
            {"bad_speculation_pipeline_restarts",
             "bad_speculation_from_pipeline_restarts"},
            {"backend_bound_memory", "backend_bound_by_memory"},
            {"backend_bound_cpu", "backend_bound_by_cpu"},
            {"retiring_fastpath", "retiring_from_fastpath"},
            {"retiring_microcode", "retiring_from_microcode"},
        },
    };

// The `MetricThreshold` texts of two Intel rows, spelled as the table spells a
// threshold: the row's own name, a comparison and a figure. The feature prints
// this text and evaluates none of it (FR-017, spec D-9).
inline constexpr std::string_view kLevelOneThreshold {
    "tma_backend_bound > 0.2"};
inline constexpr std::string_view kLevelTwoThreshold {
    "tma_core_bound > 0.1 & tma_backend_bound > 0.2"};

// The Intel group text of one level: the vendor's own level tag, the level
// token `TopdownL<n>`, the level group token whose stem names no row, and the
// parent token `tma_<parent>_group` whose stem names a row of the directory.
//
// \pre `level` stands in 1 to 6 and `vendorTag` is the vendor's own tag of
//      that level
// \post The text holds the tag, `TopdownL<level>` and `tma_L<level>_group`,
//         followed by `tma_<parent>_group` when `parent` holds a name; an
//         empty `parent` answers the group text of a level-1 row, which names
//         no parent
[[nodiscard]] inline auto intelGroup(int level,
                                     std::string_view vendorTag,
                                     std::string_view parent = {})
    -> std::string
{
  std::string group {std::string(vendorTag) + ";TopdownL"
                     + std::to_string(level) + ";tma_L" + std::to_string(level)
                     + "_group"};
  if (!parent.empty()) {
    group += ";tma_";
    group += parent;
    group += "_group";
  }
  return group;
}

// The AMD group text of one level: the level token `PipelineL<n>` and, below
// level 1, the parent token `<parent>_group` with no prefix (FR-013, FR-014).
//
// \pre `level` stands in 1 to 2
// \post The text holds `PipelineL<level>`, followed by `<parent>_group` when
//        `parent` holds a name
[[nodiscard]] inline auto amdGroup(int level,
                                   std::string_view parent = {}) -> std::string
{
  std::string group {"PipelineL" + std::to_string(level)};
  if (!parent.empty()) {
    group += ';';
    group += parent;
    group += "_group";
  }
  return group;
}

// The event rows of the Intel directory. Every row carries the core scope, so
// the directory holds one core scope and a selection reads a tree out of it
// (FR-006). The memory read of `tma_memory_bandwidth` reaches its leaf through
// the scope of that row, as the row-level scope rule of specs/022 places it.
//
// \pre none
// \post Five event rows, each with `kCoreScope` as its scope and none of them
//        carrying a metric name
[[nodiscard]] inline auto topdownIntelEventRows() -> std::vector<MetricRow>
{
  return {
      fixtureEvent(kCycles, kCoreScope),
      fixtureEvent(kInstructions, kCoreScope),
      fixtureEvent(kRefCycles, kCoreScope),
      fixtureEvent(kL2Requests, kCoreScope),
      fixtureEvent(kDataReads, kCoreScope),
  };
}

// The event rows of a hybrid directory: the same names on each of the two core
// scopes, the way a pinned hybrid table spells both, beside one row of a scope
// that names no core class. The two core scopes are the fact the hybrid rule
// reads, and the third scope changes nothing about it (FR-006).
//
// \pre none
// \post Nine event rows: four with `kCoreScope`, four with `kAtomScope` and
//        one with `kMemoryScope`, which is the two-scope fact of FR-006
[[nodiscard]] inline auto topdownHybridEventRows() -> std::vector<MetricRow>
{
  return {
      fixtureEvent(kCycles, kCoreScope),
      fixtureEvent(kInstructions, kCoreScope),
      fixtureEvent(kRefCycles, kCoreScope),
      fixtureEvent(kL2Requests, kCoreScope),
      fixtureEvent(kDataReads, kMemoryScope),
      fixtureEvent("fixture_atom_cycles", kAtomScope),
      fixtureEvent("fixture_atom_instructions", kAtomScope),
      fixtureEvent("fixture_atom_ref_cycles", kAtomScope),
      fixtureEvent("fixture_atom_l2_requests", kAtomScope),
  };
}

// The event rows of an AMD directory, which spell no `Unit` key at all, the
// way the pinned AMD tables spell their rows.
//
// \pre none
// \post Four event rows, each with an empty scope and none of them carrying a
//        metric name
[[nodiscard]] inline auto topdownAmdEventRows() -> std::vector<MetricRow>
{
  return {
      fixtureEvent(kCycles),
      fixtureEvent(kInstructions),
      fixtureEvent(kRefCycles),
      fixtureEvent(kL2Requests),
  };
}

// The four level-1 rows of the Intel table, named as the pinned table names
// them and scaled as it scales them (FR-011, FR-014).
//
// \pre none
// \post Four metric rows, each grouped `TopdownL1` with the `tma_L1_group`
//        token and no parent token, each scaled `100%`
[[nodiscard]] inline auto topdownIntelLevelOneRows() -> std::vector<MetricRow>
{
  return {
      fixtureMetric("tma_frontend_bound",
                    "fixture_cycles / fixture_ref_cycles",
                    "100%",
                    {},
                    intelGroup(1, "TmaL1")),
      fixtureMetric("tma_bad_speculation",
                    "fixture_l2_requests / fixture_instructions",
                    "100%",
                    {},
                    intelGroup(1, "TmaL1")),
      fixtureMetric(
          "tma_backend_bound",
          "1 - tma_frontend_bound - tma_bad_speculation " "- tma_retiring",
          "100%",
          {},
          intelGroup(1, "TmaL1")),
      fixtureMetric("tma_retiring",
                    "fixture_instructions / fixture_ref_cycles",
                    "100%",
                    {},
                    intelGroup(1, "TmaL1")),
  };
}

// The level-2 rows of the Intel table: a row under each level-1 parent, a
// second row under `tma_frontend_bound` and under `tma_backend_bound`, the
// memory row whose own scope seats it under the memory object beside the core
// rows of the same level, and the row whose only `_group` token names no
// row.
//
// \pre none
// \post Seven metric rows, each grouped `TopdownL2`; six reach the core
//        object, `tma_memory_bandwidth` carries `kMemoryScope` and reaches the
//        memory object, and `tma_orphan_row` carries `tma_L2_group` as its only
//        `_group` token, a stem that names no row of the directory
[[nodiscard]] inline auto topdownIntelLevelTwoRows() -> std::vector<MetricRow>
{
  return {
      fixtureMetric("tma_fetch_latency",
                    "4 * tma_frontend_bound",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "frontend_bound")),
      fixtureMetric("tma_fetch_bandwidth",
                    "tma_frontend_bound - tma_fetch_latency",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "frontend_bound")),
      fixtureMetric("tma_branch_mispredicts",
                    "tma_bad_speculation * 0.5",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "bad_speculation")),
      fixtureMetric("tma_core_bound",
                    "tma_backend_bound - tma_memory_bound",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "backend_bound")),
      fixtureMetric("tma_memory_bound",
                    "d_ratio(fixture_l2_requests, fixture_cycles)",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "backend_bound")),
      fixtureMetric("tma_memory_bandwidth",
                    "fixture_data_reads / cpu_core@fixture_time@",
                    "1MB/s",
                    kMemoryScope,
                    intelGroup(2, "TmaL2", "backend_bound")),
      fixtureMetric("tma_orphan_row",
                    "tma_retiring",
                    "100%",
                    {},
                    "TmaL2;TopdownL2;tma_L2_group"),
  };
}

// The rows of levels 3, 4, 5 and 6: chains below `tma_fetch_latency`,
// `tma_fetch_bandwidth`, `tma_core_bound` and `tma_memory_bound`, so every
// level token of FR-014 stands in the table and each node's level stands one
// below its parent's level.
//
// \pre none
// \post Eleven metric rows: four grouped `TopdownL3`, three `TopdownL4`, two
//        `TopdownL5` and two `TopdownL6`, each with the parent token of the
//        row one level above it
[[nodiscard]] inline auto topdownIntelDeepRows() -> std::vector<MetricRow>
{
  return {
      fixtureMetric(
          "tma_branch_resteers",
          "tma_fetch_latency * d_ratio(fixture_cycles, fixture_ref_cycles)",
          "100%",
          {},
          intelGroup(3, "TmaL3", "fetch_latency")),
      fixtureMetric(
          "tma_divider",
          "d_ratio(fixture_ref_cycles, fixture_cycles) * tma_core_bound",
          "100%",
          {},
          intelGroup(3, "TmaL3", "core_bound")),
      fixtureMetric("tma_dram_bound",
                    "tma_memory_bound * 0.5",
                    "100%",
                    {},
                    intelGroup(3, "TmaL3", "memory_bound")),
      fixtureMetric("tma_dsb",
                    "tma_fetch_bandwidth * 0.5",
                    "100%",
                    {},
                    intelGroup(3, "TmaL3", "fetch_bandwidth")),
      fixtureMetric("tma_clears_resteers",
                    "tma_branch_resteers * 0.5",
                    "100%",
                    {},
                    intelGroup(4, "TmaL4", "branch_resteers")),
      fixtureMetric("tma_divider_high",
                    "tma_divider * 0.5",
                    "100%",
                    {},
                    intelGroup(4, "TmaL4", "divider")),
      fixtureMetric("tma_dram_bandwidth",
                    "tma_dram_bound * 0.5",
                    "100%",
                    {},
                    intelGroup(4, "TmaL4", "dram_bound")),
      fixtureMetric("tma_dram_page_miss",
                    "tma_dram_bandwidth * 0.5",
                    "100%",
                    {},
                    intelGroup(5, "TmaL5", "dram_bandwidth")),
      fixtureMetric("tma_divider_high_2m",
                    "tma_divider_high * 0.5",
                    "100%",
                    {},
                    intelGroup(5, "TmaL5", "divider_high")),
      fixtureMetric("tma_dram_page_miss_4k",
                    "tma_dram_page_miss * 0.5",
                    "100%",
                    {},
                    intelGroup(6, "TmaL6", "dram_page_miss")),
      fixtureMetric("tma_divider_high_4k",
                    "tma_divider_high_2m * 0.5",
                    "100%",
                    {},
                    intelGroup(6, "TmaL6", "divider_high_2m")),
  };
}

// The refused rows of the Intel table, one per refusal kind of
// `MetricRefusal`. Each carries a level group, so the refusal stands in the
// record of the level it names, and each reason is the text the resolution
// answers with its own kind word (FR-016).
//
//   `tma_bad_arity`: MALFORMED, a helper call of two arguments
//   `tma_no_such_fact`: UNKNOWN_CONSTANT, a fact no reader answers
//   `tma_unknown_scale`: UNKNOWN_SCALE, a scale no rule carries
//   `tma_blocked_modifier`: UNSUPPORTED, a modifier the entry lacks
//   `tma_no_leaf_row`: UNRESOLVED, a reference no leaf answers
//   `tma_mixed_terms`: DIMENSION, a duration and a count in one sum
//   `tma_nested_guard`: NESTED, a predicate inside a guard argument
//   `tma_core_bound` again: COLLISION, the name level 2 took
//   `tma_split_a` and `tma_split_b`: CYCLE, a chain that returns
//
// \pre none
// \post Ten metric rows that the resolution refuses, eight of level 2 and a
//        pair of level 3, each with a level token and a parent token. The
//        nine kinds of `MetricRefusal` answer between them, the cycle kind
//        standing on the pair, and each reason text is the resolution's own.
//        These kinds hold on the host `registerMetricHost` registers, because
//        an unresolvable reference is a host fact (FR-016)
[[nodiscard]] inline auto topdownIntelRefusalRows() -> std::vector<MetricRow>
{
  return {
      fixtureMetric("tma_bad_arity",
                    "has_event(fixture_cycles, fixture_ref_cycles)",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "fetch_latency")),
      fixtureMetric("tma_no_such_fact",
                    "fixture_cycles / #has_pmem",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "core_bound")),
      fixtureMetric("tma_unknown_scale",
                    "tma_retiring",
                    "1furlong",
                    {},
                    intelGroup(2, "TmaL2", "frontend_bound")),
      fixtureMetric("tma_blocked_modifier",
                    "cpu_core@fixture_cycles,cmask\\=1@",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "fetch_bandwidth")),
      fixtureMetric("tma_no_leaf_row",
                    "fixture_no_such_event / fixture_instructions",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "bad_speculation")),
      fixtureMetric("tma_mixed_terms",
                    "fixture_cycles + fixture_time",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "memory_bound")),
      fixtureMetric("tma_nested_guard",
                    "min(has_event(fixture_cycles), 1)",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "bad_speculation")),
      fixtureMetric("tma_core_bound",
                    "tma_backend_bound",
                    "100%",
                    {},
                    intelGroup(2, "TmaL2", "backend_bound")),
      fixtureMetric("tma_split_a",
                    "tma_split_b + 1",
                    "100%",
                    {},
                    intelGroup(3, "TmaL3", "divider")),
      fixtureMetric("tma_split_b",
                    "tma_split_a + 1",
                    "100%",
                    {},
                    intelGroup(3, "TmaL3", "divider")),
  };
}

// The Intel table of `kIntelDirectory`, the directory of one core scope: the
// event rows, then the level rows of levels 1 and 2, then the refused rows,
// then the rows of levels 3 to 6, the order a directory's files append them.
//
// \pre none
// \post 37 rows in that order: five event rows, 22 published level rows and
//        10 refused rows, the deepest level token `TopdownL6` (FR-016)
[[nodiscard]] inline auto topdownIntelRows() -> std::vector<MetricRow>
{
  std::vector<MetricRow> rows = topdownIntelEventRows();
  const auto append = [&rows](const std::vector<MetricRow>& part)
  { rows.insert(rows.end(), part.begin(), part.end()); };
  append(topdownIntelLevelOneRows());
  append(topdownIntelLevelTwoRows());
  append(topdownIntelRefusalRows());
  append(topdownIntelDeepRows());
  return rows;
}

// The Intel table with its two deepest levels dropped: the level-4 rows are the
// deepest it holds, so an asked level of 6 answers a tree of level 4 and the
// record states the clamp (FR-008, FR-020).
//
// \pre none
// \post 33 rows, the Intel table without its four rows of levels 5 and 6, so
//        `TopdownL4` is the deepest level token it holds
[[nodiscard]] inline auto topdownIntelShallowRows() -> std::vector<MetricRow>
{
  std::vector<MetricRow> rows = topdownIntelEventRows();
  const auto append = [&rows](const std::vector<MetricRow>& part)
  { rows.insert(rows.end(), part.begin(), part.end()); };
  append(topdownIntelLevelOneRows());
  append(topdownIntelLevelTwoRows());
  append(topdownIntelRefusalRows());
  const auto deep = topdownIntelDeepRows();
  for (const MetricRow& row : deep) {
    if (row.group.find("TopdownL5") == std::string_view::npos
        && row.group.find("TopdownL6") == std::string_view::npos)
    {
      rows.push_back(row);
    }
  }
  return rows;
}

// The group-free helper row of an AMD pipeline table: a formula and a scale, no
// `MetricGroup` key, and a name both pipeline tables spell alike. Every level
// row of the table names it in its formula, and it names no level, so it
// reaches no node and states no refusal (FR-014).
//
// \pre `slots` is the dispatch width of the table: 6 for Zen 4, 8 for Zen 5
// \post One metric row named `total_dispatch_slots` with an empty group text;
//        the row of width 8 carries the `1slots` scale and the row of width 6
//        carries none
[[nodiscard]] inline auto topdownAmdHelperRow(int slots) -> MetricRow
{
  return fixtureMetric("total_dispatch_slots",
                       std::to_string(slots) + " * " + std::string(kCycles),
                       slots == 8 ? "1slots" : "",
                       {},
                       "");
}

// The five level-1 rows of an AMD pipeline table. The four shared names come
// first and `smt_contention` stands among them, the fifth name an AMD table
// alone answers (FR-011). The scale text is the pinned file's own: Zen 4 spells
// every row `100%` and Zen 5 spells the slot rows `100%slots` and the
// operation rows `100%ops`.
//
// \pre none
// \post Five metric rows named `kAmdLevelOneNames` in that order, each grouped
//        `PipelineL1` with no parent token, each resolving through the helper
//        row of the same table
[[nodiscard]] inline auto topdownAmdLevelOneRows(bool zen5)
    -> std::vector<MetricRow>
{
  const std::string_view slots {zen5 ? "100%slots" : "100%"};
  const std::string_view ops {zen5 ? "100%ops" : "100%"};
  return {
      fixtureMetric(
          "frontend_bound",
          "d_ratio(" + std::string(kCycles) + ", total_dispatch_slots)",
          slots,
          {},
          amdGroup(1)),
      fixtureMetric("bad_speculation",
                    "d_ratio(" + std::string(kInstructions) + " - "
                        + std::string(kL2Requests) + ", total_dispatch_slots)",
                    ops,
                    {},
                    amdGroup(1)),
      fixtureMetric(
          "backend_bound",
          "d_ratio(" + std::string(kRefCycles) + ", total_dispatch_slots)",
          slots,
          {},
          amdGroup(1)),
      fixtureMetric(
          "smt_contention",
          "d_ratio(" + std::string(kL2Requests) + ", total_dispatch_slots)",
          slots,
          {},
          amdGroup(1)),
      fixtureMetric(
          "retiring",
          "d_ratio(" + std::string(kInstructions) + ", total_dispatch_slots)",
          slots,
          {},
          amdGroup(1)),
  };
}

// The eight level-2 rows of an AMD pipeline table, two under each level-1
// parent, each naming its parent with the unprefixed `<name>_group` token and
// each named by the row of `kZenLevelTwoNames` that belongs to `zen5`
// (FR-013, FR-052).
//
// \pre none
// \post Eight metric rows grouped `PipelineL2`: the names of
//        `kZenLevelTwoNames` on the `zen5` side in table order, two under each
//        of the four level-1 parents
[[nodiscard]] inline auto topdownAmdLevelTwoRows(bool zen5)
    -> std::vector<MetricRow>
{
  const std::string_view slots {zen5 ? "100%slots" : "100%"};
  const std::string_view ops {zen5 ? "100%ops" : "100%"};
  const auto name = [zen5](const std::size_t index)
  {
    const auto& pair = kZenLevelTwoNames[index];
    return std::string(zen5 ? pair.second : pair.first);
  };
  const std::string latency = name(0);
  const std::string bandwidth = name(1);
  const std::string mispredicts = name(2);
  const std::string restarts = name(3);
  const std::string memory = name(4);
  const std::string cpu = name(5);
  const std::string fastpath = name(6);
  const std::string microcode = name(7);
  return {
      fixtureMetric(
          latency,
          "d_ratio(" + std::string(kRefCycles) + ", total_dispatch_slots)",
          slots,
          {},
          amdGroup(2, "frontend_bound")),
      fixtureMetric(bandwidth,
                    "frontend_bound - " + latency,
                    slots,
                    {},
                    amdGroup(2, "frontend_bound")),
      fixtureMetric(mispredicts,
                    "bad_speculation * d_ratio(" + std::string(kInstructions)
                        + ", " + std::string(kCycles) + ")",
                    ops,
                    {},
                    amdGroup(2, "bad_speculation")),
      fixtureMetric(restarts,
                    "bad_speculation * d_ratio(" + std::string(kL2Requests)
                        + ", " + std::string(kCycles) + ")",
                    ops,
                    {},
                    amdGroup(2, "bad_speculation")),
      fixtureMetric(memory,
                    "backend_bound * d_ratio(" + std::string(kRefCycles) + ", "
                        + std::string(kCycles) + ")",
                    slots,
                    {},
                    amdGroup(2, "backend_bound")),
      fixtureMetric(cpu,
                    "backend_bound * (1 - d_ratio(" + std::string(kRefCycles)
                        + ", " + std::string(kCycles) + "))",
                    slots,
                    {},
                    amdGroup(2, "backend_bound")),
      fixtureMetric(fastpath,
                    "retiring * (1 - d_ratio(" + std::string(kInstructions)
                        + ", " + std::string(kCycles) + "))",
                    slots,
                    {},
                    amdGroup(2, "retiring")),
      fixtureMetric(microcode,
                    "retiring * d_ratio(" + std::string(kInstructions) + ", "
                        + std::string(kCycles) + ")",
                    slots,
                    {},
                    amdGroup(2, "retiring")),
  };
}

// One AMD pipeline table: the helper row, the five level-1 rows, the eight
// level-2 rows, and the event rows of no scope (FR-052).
//
// \pre none
// \post 18 rows: four event rows, then the helper row, the five level-1 rows
//        and the eight level-2 rows, every metric row of them published
[[nodiscard]] inline auto topdownAmdRows(bool zen5) -> std::vector<MetricRow>
{
  std::vector<MetricRow> rows = topdownAmdEventRows();
  const auto append = [&rows](const std::vector<MetricRow>& part)
  { rows.insert(rows.end(), part.begin(), part.end()); };
  append({topdownAmdHelperRow(zen5 ? 8 : 6)});
  append(topdownAmdLevelOneRows(zen5));
  append(topdownAmdLevelTwoRows(zen5));
  return rows;
}

// The Zen 4 pipeline table of `kZen4Directory`: the six-slot helper, the
// `100%` scale text and the `backend_bound_cpu` spelling (FR-052).
//
// \pre none
// \post `topdownAmdRows(false)`
[[nodiscard]] inline auto topdownZen4Rows() -> std::vector<MetricRow>
{
  return topdownAmdRows(false);
}

// The Zen 5 pipeline table of `kZen5Directory`: the eight differing level-2
// names, the eight-slot helper and the slot-and-operation scale texts.
//
// \pre none
// \post `topdownAmdRows(true)` (FR-052)
[[nodiscard]] inline auto topdownZen5Rows() -> std::vector<MetricRow>
{
  return topdownAmdRows(true);
}

// The hybrid directory of `kHybridDirectory`: the event rows of both core
// scopes and the level rows of levels 1 and 2, so the scope rule answers from
// the rows ahead of any level row (FR-006, FR-007).
//
// \pre none
// \post 20 rows: the nine event rows of the two core scopes, then the four
//        level-1 rows and the seven level-2 rows of the Intel table
[[nodiscard]] inline auto topdownHybridRows() -> std::vector<MetricRow>
{
  std::vector<MetricRow> rows = topdownHybridEventRows();
  const auto append = [&rows](const std::vector<MetricRow>& part)
  { rows.insert(rows.end(), part.begin(), part.end()); };
  append(topdownIntelLevelOneRows());
  append(topdownIntelLevelTwoRows());
  return rows;
}

// The parsed view of a fixture table: the same parse pass the loader runs over
// a directory's bytes, so a fixture row carries the refusals the parse answers
// with its own kind word (FR-016).
//
// \pre `rows` is one fixture table, event rows and metric rows together
// \post One parsed entry per row that carries a metric name, in table order,
//        published and refused alike
[[nodiscard]] inline auto topdownTableOf(const std::vector<MetricRow>& rows)
    -> MetricTable
{
  return parseMetricRows(rows);
}

}  // namespace sg::test

#endif  // SG_TEST_DETAIL_TOPDOWN_FIXTURE_HPP
