// ============================================================================
// The top-down area of a ranked event (specs/046-hunter-topdown T004 to T030;
// FR-001 to FR-010, FR-029, FR-030, FR-033; D-1, D-3, D-4, D-7, D-10, D-16,
// D-17; specs/046-hunter-topdown/contracts/area-mapper.md).
//
// The mapper is pure over three values, so every case plants those three
// values in process: the tree a run answers, built here by hand; the vendor
// grouping listing of D-16; and the catalog address of one ranked record. The
// identity and the table of a run reach the ask through
// `RunOptions::topdownIdentity` and `RunOptions::topdownTable`, the two seams
// FR-033 names (FR-001, FR-029).
//
// No case opens an event, samples a counter, reads a clock, sleeps or names a
// cpu device the host lacks: a scripted seam answers from its own record, and
// no case needs a host other than the one it runs on (FR-030). A planted table
// resolves the metric rows of its fixture groups against the scripted host of
// `detail/metric_fixture.hpp`, which `main` registers once per process, so a
// case plants a table without collecting anything (FR-029).
//
// Six synthetic identities carry the vendor arms, each from a helper this file
// owns, because no shared header holds them: the model-85 stepping-4
// Skylake-X part, the family-25 model-17 Zen 4 part, the family-26 model-17
// Zen 5 part, the unsupported family-25 model-15 part, an Arm `MIDR` record,
// and an identity no mapping-file row matches (FR-029). A path case plants a
// `TopdownTable` beside its identity, because an identity alone answers zero
// nodes on a host of another vendor (FR-029).
//
// Scenario map:
//   area-takes-the-deepest-node  the deepest node that names the address
//                                answers, ahead of the level-1 row above it
//                                (FR-001, FR-002)
//   level-one-node-answers-one-name  a node at level 1 answers its own name
//                                    alone (FR-007, D-10)
//   tie-goes-to-the-first-row-in-table-order  two nodes of one depth answer
//                                             the row that stands first in the
//                                             table (FR-003, D-4)
//   second-spelling-names-no-node  an address in another spelling names no
//                                  node, so the grouping answers it (FR-002,
//                                  FR-004)
//   arm-midr-answers-unmapped  an Arm `MIDR` record answers `unmapped` ahead of
//                              the grouping (FR-005, D-5)
//   unknown-vendor-answers-unmapped  a vendor the mapping file does not name
//                                    answers `unmapped` (FR-005)
//   no-directory-answers-unmapped  an identity with no directory row answers
//                                  `unmapped` (FR-005)
//   no-topdown-rows-answers-unmapped  a directory with no top-down row answers
//                                     `unmapped` (FR-005)
//   two-scope-rows-answer-unmapped  two scope rows of one name answer
//                                   `unmapped`, because no tree stands
//                                   (FR-005, D-5)
//   area-falls-back-to-the-vendor-file-stem  no node names the address, so the
//                                            row of the listing answers its
//                                            group (FR-004, D-3)
//   index-drops-the-device-prefix  the listing reads the name behind the device
//                                  prefix of the address (FR-004)
//   no-node-and-no-row-answers-unmapped  nothing names the address, so the
//                                        answer reads the closing word
//                                        (FR-001, D-3)
//   two-calls-answer-one-area  the same three inputs twice answer one text
//                              (FR-006)
//   oneCallReadsNoSourceOfItsOwn  one call reads no counter, opens no event,
//                                 reads no clock and asks no kernel record of
//                                 its own (FR-001, D-7)
//   planted-run-asks-the-area-at-level-six  the seam of a planted run hands the
//                                           level-6 ask, the planted identity
//                                           and the planted table (FR-001,
//                                           FR-033)
//   unplanted-run-asks-the-area-at-level-six  with nothing planted the ask
//                                             carries the identity this cpu
//                                             answers (FR-033, FR-034)
//   level-six-ask-clamps-to-a-shallow-table  the deepest ask over a shallow
//                                            table answers the deepest node
//                                            that table holds (FR-008, D-17)
//   area-and-companion-asks-differ-in-level-alone  the level-1 companion of
//                                                  D-9 differs from the area
//                                                  ask by its level alone
//                                                  (FR-033, D-9)
//   stamp-sets-every-entry-including-unranked  one stamp settles every record
//                                              of `entries`, the unranked row
//                                              included, and the skipped list
//                                              gains nothing (FR-009, D-8)
//   stamp-leaves-an-unhunted-result-alone  a record that carries no hunt
//                                          changes by zero field (FR-010)
//   zen-five-area-answers-the-vendor-names  the planted Zen 5 table settles its
//                                           own level-2 spelling, carries no
//                                           Intel name, and reads its own stem
//                                           for the row Zen 4 spells cache
//                                           (T096; FR-024, SC-001, D-3)
//   an-amd-node-names-an-amd-path  the planted Zen 4 table settles the Zen 4
//                                  spelling of the same leaf, and the same
//                                  identity alone settles no path, so no host
//                                  stands under the AMD claim (T097; FR-029,
//                                  FR-030)
// ============================================================================

#include <algorithm>
#include <bit>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "../source/counters/linux_pmu/metric_formula.hpp"
#include "detail/gcov_flush.hpp"
#include "detail/hunt_topdown.hpp"
#include "detail/internal.hpp"
#include "detail/metric_fixture.hpp"
#include "detail/topdown_fixture.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_topdown.hpp"
#include "speedgun-ng/hunt.hpp"

namespace
{

using sg::counters::CpuIdentity;
using sg::counters::TopdownEventGroup;
using sg::counters::TopdownRow;
using sg::counters::TopdownTable;
using sg::test::fixtureHost;
using sg::test::kHybridDirectory;
using sg::test::kIntelDirectory;
using sg::test::kZen4Directory;
using sg::test::kZen5Directory;
using sg::test::MetricDefinition;
using sg::test::MetricRow;
using sg::test::registerMetricHost;
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

// The word a case prints and the status a failing case exits with, the shape
// `harness_hunt_report_test.cpp` carries: one scenario stops at its first
// failing claim, and the parent process reads that status as the verdict.
[[noreturn]] auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT TOPDOWN TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* const what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// Whether two texts stand equal, read as one line of a failing case: the
// answer a case weighs and the text it weighs it against both print.
auto checkText(const std::string& answer,
               const std::string& expected,
               const char* const what) -> void
{
  if (answer != expected) {
    std::fprintf(stderr,
                 "HARNESS HUNT TOPDOWN TEST FAIL: %s\n  answer:   '%s'\n  "
                 "expected: '%s'\n",
                 what,
                 answer.c_str(),
                 expected.c_str());
    std::exit(1);
  }
}

// The house double comparison: -Werror=float-equal rules out `==` on a double.
auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

// One Intel identity. The mapping file spells a model in uppercase hex, so the
// decimal 85 is the row that reads `55`, and the stepping is the digit that row
// carries after the model (specs/007 FR-034).
[[maybe_unused]] [[nodiscard]] auto intelIdentity(
    const int family, const int model, const int stepping) -> CpuIdentity
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
[[maybe_unused]] [[nodiscard]] auto amdIdentity(const int family,
                                                const int model) -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "AuthenticAMD", .family = family, .model = model};
}

// The Arm identity of FR-005 and D-5: the `MIDR` a test hands in beside four
// fields no mapping-file row matches, so the selection reads the `MIDR` and
// answers its refusal.
[[maybe_unused]] [[nodiscard]] auto armIdentity() -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "ARM",
      .family = 0,
      .model = 0,
      .stepping = -1,
      .coreType = std::string {kHandedCoreType},
      .midr = std::optional<std::uint32_t> {kHandedMidr},
  };
}

// The identity an identity mapfile row matches no text of, so the selection
// answers no directory at all.
[[maybe_unused]] [[nodiscard]] auto unmatchedIdentity() -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "NoSuchVendorXXXXX", .family = 999, .model = 999};
}

// One fixture table as the selection reads it: the event rows apart from the
// metric rows, and the metric rows parsed and resolved against the scripted
// host, so a planted row carries the verdict, the spine, the leaf addresses and
// the reason that host answers with (FR-010, FR-016 of specs/042-topdown).
[[maybe_unused]] [[nodiscard]] auto plantedTable(
    const std::string_view directory,
    const std::vector<MetricRow>& rows) -> TopdownTable
{
  std::vector<MetricRow> events;
  for (const MetricRow& row : rows) {
    if (row.metricName.empty()) {
      events.push_back(row);
    }
  }
  const auto parsed = sg::counters::detail::parseMetricRows(rows);
  const auto resolved =
      sg::counters::detail::metricRows(parsed, events, fixtureHost());
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

// The option set of a run that plants the Intel table and no identity: the ask
// of the area reads the table here, and the identity of the cpu this process
// runs on where no identity stands (FR-033).
[[maybe_unused]] [[nodiscard]] auto plantedTableOptions()
    -> sg::detail::RunOptions
{
  sg::detail::RunOptions options;
  options.topdownTable = plantedTable(kIntelDirectory, topdownIntelRows());
  return options;
}

// The same option set beside the identity of the part it plants, the second of
// the two planted forms FR-033 names (FR-029).
[[maybe_unused]] [[nodiscard]] auto plantedIdentityOptions()
    -> sg::detail::RunOptions
{
  sg::detail::RunOptions options = plantedTableOptions();
  options.topdownIdentity = intelIdentity(6, 85, 4);
  return options;
}

// The grouping listing of one identity in the shape the accessor of D-16
// answers it: two names of the Skylake-X directory, each beside the stem of the
// file that holds its row.
[[maybe_unused]] [[nodiscard]] auto plantedGroups()
    -> std::vector<TopdownEventGroup>
{
  return {
      TopdownEventGroup {.event = "LONGEST_LAT_CACHE.MISS", .group = "cache"},
      TopdownEventGroup {.event = "tma_backend_bound", .group = "skx-metrics"},
  };
}

// ---------------------------------------------------------------------------
// The tree of the node arm
// ---------------------------------------------------------------------------

// The address two nodes of `backendTree` name: the level-3 node and the level-2
// node above it (FR-001, D-4).
constexpr std::string_view kDeepAddress {"cpu/l1_bound_stalls"};

// The first spelling of the branch address, the one a node's formula carries.
constexpr std::string_view kBranchAddress {"cpu/branch-instructions"};

// The second spelling of that same address, in the AP-4 shape: the ranked table
// of the acceptance run holds both, and only the first stands in a node
// (FR-002).
constexpr std::string_view kBranchAlias {"cpu/BR_INST_RETIRED.ALL_BRANCHES"};

// The path the level-3 node settles for `kDeepAddress`: the neutral name of the
// level-1 node, then the names below it, joined by " > " (FR-007, D-10).
constexpr std::string_view kDeepPath {
    "topdown.backend_bound > topdown.intel.tma_core_bound > "
    "topdown.intel.tma_l1_bound"};

// One node of a planted tree, in the shape
// `include/speedgun-ng/counters_topdown.hpp` states: the neutral or vendor name
// the path reads, the parent the row's group token names, the row's own level,
// and the leaf addresses its formula reads. No case reads the spine, so no node
// carries one (FR-008, D-1).
[[nodiscard]] auto plantedNode(std::string name,
                               std::string parent,
                               const int level,
                               std::vector<std::string> events)
    -> sg::counters::TopdownNode
{
  auto node = sg::counters::TopdownNode {};
  node.name = std::move(name);
  node.parent = std::move(parent);
  node.level = level;
  node.events = std::move(events);
  return node;
}

// The tree of the node arm: one address two nodes name at two depths, one
// address a level-1 node names alone, the first spelling of the branch address,
// and a memory address no node of this tree names.
[[nodiscard]] auto backendTree() -> sg::counters::TopdownTree
{
  auto tree = sg::counters::TopdownTree {};
  tree.source.directory = std::string {kIntelDirectory};
  tree.depth = 3;
  tree.nodes = {
      plantedNode("topdown.backend_bound", {}, 1, {"cpu/backend_stalls"}),
      plantedNode("topdown.intel.tma_core_bound",
                  "topdown.backend_bound",
                  2,
                  {std::string {kBranchAddress}, "cpu/core_busy_cycles"}),
      plantedNode("topdown.intel.tma_l1_bound",
                  "topdown.intel.tma_core_bound",
                  3,
                  {std::string {kDeepAddress}}),
      plantedNode("topdown.frontend_bound", {}, 1, {"cpu/fetch_latency"}),
  };
  return tree;
}

// The tree of the tie: two level-2 nodes name one address under two level-1
// parents, and the row that stands first names the path (FR-003, D-4).
[[nodiscard]] auto tieTree() -> sg::counters::TopdownTree
{
  auto tree = sg::counters::TopdownTree {};
  tree.source.directory = std::string {kIntelDirectory};
  tree.depth = 2;
  tree.nodes = {
      plantedNode("topdown.frontend_bound", {}, 1, {}),
      plantedNode("topdown.bad_speculation", {}, 1, {}),
      plantedNode("topdown.intel.tma_fetch_latency",
                  "topdown.frontend_bound",
                  2,
                  {"cpu/two_parents"}),
      plantedNode("topdown.intel.tma_branch_resteers",
                  "topdown.bad_speculation",
                  2,
                  {"cpu/two_parents"}),
  };
  return tree;
}

// ---------------------------------------------------------------------------
// The node arm
// ---------------------------------------------------------------------------

// FR-001, FR-007, D-4, D-10: the deepest node that names the address settles
// the area, and the path carries every name above it.
auto scenarioAreaTakesTheDeepestNode() -> void
{
  const auto tree = backendTree();
  const auto index = sg::detail::topdownDeepestNode(tree, kDeepAddress);
  check(index.has_value(),
        "a node names the address, so the deepest-node ask answers an index "
        "(FR-001)");
  check(*index == 2,
        "the level-3 node wins over the level-2 node that names the same "
        "address two levels above it (FR-003, D-4)");
  checkText(sg::detail::topdownPathOf(tree, *index),
            std::string {kDeepPath},
            "the path reads the level-1 name, then the names below it, "
            "joined root first in the order the parent links reach (FR-007, D-10)");
  checkText(sg::detail::topdownArea(kDeepAddress, tree, {}),
            std::string {kDeepPath},
            "the area of the address is that path, so the mapper answers the "
            "deepest node and no other (FR-001, FR-007)");
}

// FR-007, D-10: a level-1 node settles a path of one name.
auto scenarioLevelOneNodeAnswersOneName() -> void
{
  const auto tree = backendTree();
  const auto index = sg::detail::topdownDeepestNode(tree, "cpu/fetch_latency");
  check(index.has_value() && *index == 3,
        "one level-1 node names the address, so the ask answers its index "
        "(FR-007)");
  checkText(sg::detail::topdownArea("cpu/fetch_latency", tree, {}),
            "topdown.frontend_bound",
            "a level-1 node answers a one-name path, so no separator stands "
            "in the area (FR-007, D-10)");
}

// FR-003, D-4: of two nodes of one depth that both name the address, the row
// that stands first in table order settles the area. The case swaps the two
// rows, so the answer follows the table and not the name text: the alphabet
// puts `tma_branch_resteers` first, and the table puts it last.
auto scenarioTieGoesToTheFirstRowInTableOrder() -> void
{
  auto tree = tieTree();
  const auto index = sg::detail::topdownDeepestNode(tree, "cpu/two_parents");
  check(index.has_value() && *index == 2,
        "two level-2 nodes name the address, and the ask answers the lower "
        "index of the two (FR-003, D-4)");
  checkText(sg::detail::topdownArea("cpu/two_parents", tree, {}),
            "topdown.frontend_bound > topdown.intel.tma_fetch_latency",
            "the row that stands first in table order names the area (FR-003)");
  std::swap(tree.nodes[2], tree.nodes[3]);
  const auto swapped = sg::detail::topdownDeepestNode(tree, "cpu/two_parents");
  check(swapped.has_value() && *swapped == 2,
        "the swapped table answers the row that now stands first, so the name "
        "text decides nothing (FR-003, D-4)");
  checkText(sg::detail::topdownArea("cpu/two_parents", tree, {}),
            "topdown.bad_speculation > topdown.intel.tma_branch_resteers",
            "the swapped table answers the other path, so table order decides "
            "the tie (FR-003)");
}

// FR-002, AP-4: the mapper compares an address with the spelling a node
// carries. The alias of a branch address names no node, so no path reaches it.
auto scenarioSecondSpellingNamesNoNode() -> void
{
  const auto tree = backendTree();
  check(sg::detail::topdownDeepestNode(tree, kBranchAddress).has_value(),
        "the spelling a node's formula carries names that node (FR-002)");
  check(!sg::detail::topdownDeepestNode(tree, kBranchAlias).has_value(),
        "the second spelling of one address stands in no node's list, so the "
        "ask answers no node for it (FR-002, AP-4)");
  const auto path = sg::detail::topdownArea(kBranchAlias, tree, {});
  check(path != sg::detail::topdownArea(kBranchAddress, tree, {}),
        "the alias answers no node path, so the two spellings of one address "
        "answer two areas (FR-002)");
}

// The whole log of one died child, or the empty text where the file stands
// unopenable.
[[nodiscard]] auto logText(const std::string& path) -> std::string
{
  auto stream = std::ifstream {path, std::ios::binary};
  auto text = std::string {};
  for (auto line = std::string {}; std::getline(stream, line);) {
    text += line;
    text += '\n';
  }
  return text;
}

// FR-007, D-10, specs/042-topdown FR-013: the walk climbs parent texts, and a
// planted table that puts one parent behind its own child breaks that promise.
// The walk dies at its assertion there, so no run prints a path it cannot
// build. The breach dies in a child of its own, so the rest of the list runs.
auto scenarioParentBehindItsChildDiesAtTheContract() -> void
{
  auto tree = tieTree();
  std::rotate(
      tree.nodes.begin(), tree.nodes.begin() + 2, tree.nodes.begin() + 3);
  const auto log = std::string {"hunt-topdown-parent-behind.txt"};
  const pid_t child = fork();
  check(child >= 0, "the scenario could not start a child of its own");
  if (child == 0) {
    if (std::freopen(log.c_str(), "w", stderr) == nullptr) {
      std::_Exit(3);
    }
    (void)sg::detail::topdownArea("cpu/two_parents", tree, {});
    std::_Exit(0);
  }
  int status = 0;
  check(waitpid(child, &status, 0) == child, "the child status did not arrive");
  check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
        "the walk dies at its assertion where a parent text names no row ahead "
        "of the child that carries it (specs/042-topdown FR-013)");
  check(logText(log).contains("names a row the table holds ahead"),
        "the death names the contract the walk enforces, so the case reports "
        "the breach it plants (specs/042-topdown FR-013)");
  std::remove(log.c_str());
}

// ---------------------------------------------------------------------------
// The refusal arms
// ---------------------------------------------------------------------------

// The word the mapper answers where no node and no row names an address
// (FR-005).
constexpr std::string_view kUnmappedText {"unmapped"};

// Two addresses the planted listing of `plantedGroups` names, so an arm that
// read the listing ahead of the tree answer would answer a group for them and
// not the word a refusal settles (FR-005, FR-006, D-16).
constexpr std::string_view kListedAddress {"cpu/LONGEST_LAT_CACHE.MISS"};
constexpr std::string_view kListedMetric {"cpu/tma_backend_bound"};

// The identity of the AMD part that predates the dispatch-slot table: family
// 25 model 15, which the mapping file gives `amdzen3`, the directory that holds
// no row of any top-down group (specs/042-topdown FR-006).
[[maybe_unused]] [[nodiscard]] auto zen3Identity() -> CpuIdentity
{
  return amdIdentity(25, 15);
}

// The hybrid identity of the vendored mapping file: the model 151 part the
// hybrid row of the mapping file names, whose directory publishes the two core
// scopes.
[[maybe_unused]] [[nodiscard]] auto hybridIdentity() -> CpuIdentity
{
  return intelIdentity(6, 0x97, 2);
}

// The ask the area makes: one identity, the level of the pair, and the table a
// case plants (FR-033, D-17).
[[nodiscard]] auto areaSelection(
    const CpuIdentity& identity,
    const std::optional<TopdownTable>& table = std::nullopt)
    -> sg::counters::TopdownSelection
{
  auto selection = sg::counters::TopdownSelection {};
  selection.identity = identity;
  selection.level = sg::detail::kTopdownAreaLevel;
  selection.table = table;
  return selection;
}

// The refusal one ask answers, and the failure of a case that asked a refusal
// and got a tree.
[[nodiscard]] auto askedRefusal(const sg::counters::TopdownSelection& selection)
    -> sg::counters::TopdownUnsupported
{
  const auto answered = sg::counters::topdownTree(selection);
  if (answered.has_value()) {
    std::fprintf(stderr,
                 "HARNESS HUNT TOPDOWN TEST FAIL: the selection answered a "
                 "tree where its case asked a refusal\n");
    std::exit(1);
  }
  return answered.error();
}

// The claim every refusal kind settles, stated once: the ask at the level of
// the pair answers no tree, the mapper reads the empty optional alone, and it
// answers the unmapped word for two addresses a standing listing names. The
// mapper reads no refusal value and no `MIDR`, so the kind is named here for
// the recipe and proven by the counters unit (FR-005, FR-030).
auto checkRefusalAnswers(const sg::counters::TopdownSelection& selection,
                         const sg::counters::TopdownUnsupport kind,
                         const char* const what) -> void
{
  check(askedRefusal(selection).kind == kind, what);
  const auto absent = std::optional<sg::counters::TopdownTree> {};
  checkText(sg::detail::topdownArea(kListedAddress, absent, plantedGroups()),
            std::string {kUnmappedText},
            "an absent tree answers the unmapped word for an address a "
            "standing listing names, so the refusal answers ahead of the "
            "listing (FR-005, D-16)");
  checkText(sg::detail::topdownArea(kListedMetric, absent, plantedGroups()),
            std::string {kUnmappedText},
            "an absent tree answers the unmapped word for every address of the "
            "run, and not the group of its own row (FR-005)");
}

// The `MIDR` of an Arm part answers no tree, so every address of such a run
// reads unmapped and no Arm table enters the answer (FR-005, D-5).
auto scenarioArmMidrAnswersUnmapped() -> void
{
  checkRefusalAnswers(
      areaSelection(armIdentity()),
      sg::counters::TopdownUnsupport::ARM,
      "a planted `MIDR` answers the Arm kind (specs/042-topdown " "FR-007)");
}

// A vendor the vendored tree holds no table of answers no tree, so the mapper
// answers the unmapped word for every address (FR-005).
auto scenarioUnknownVendorAnswersUnmapped() -> void
{
  checkRefusalAnswers(areaSelection(unmatchedIdentity()),
                      sg::counters::TopdownUnsupport::UNKNOWN_VENDOR,
                      "a vendor no mapping-file row names answers the unknown "
                      "vendor kind (specs/042-topdown FR-004)");
}

// An identity the mapping file matches no directory for answers no tree, so
// the answer reads unmapped with no listing read (FR-005, D-16).
auto scenarioNoDirectoryAnswersUnmapped() -> void
{
  checkRefusalAnswers(areaSelection(intelIdentity(15, 6, 5)),
                      sg::counters::TopdownUnsupport::NO_DIRECTORY,
                      "an identity no mapping-file row names a directory for "
                      "answers the no-directory kind (specs/042-topdown "
                      "FR-005)");
}

// A directory that holds no row of any top-down group answers no tree, so the
// run reads unmapped for every address (FR-005). The `amdzen3` directory is
// that directory, and no planted table stands in its way here.
auto scenarioNoTopdownRowsAnswersUnmapped() -> void
{
  checkRefusalAnswers(areaSelection(zen3Identity()),
                      sg::counters::TopdownUnsupport::NO_TOPDOWN_ROWS,
                      "a directory with no row of any top-down group answers "
                      "the no-rows kind (specs/042-topdown FR-006)");
}

// A directory that publishes the two core scopes answers no tree, so the
// addresses of that run read unmapped (FR-005). The case plants rows of its
// own, because the no-rows rule answers ahead of the scope rule over a
// directory that carries both scopes and no row of any top-down group. That
// precedence is the reading `counters_topdown_select_test.cpp` pins at
// `:1647-1668`, so a mixed-scope directory without level rows answers
// `NO_TOPDOWN_ROWS` and the planted rows are what settle the scope kind here.
auto scenarioTwoScopeRowsAnswerUnmapped() -> void
{
  const auto table = plantedTable(kHybridDirectory, topdownHybridRows());
  checkRefusalAnswers(areaSelection(hybridIdentity(), table),
                      sg::counters::TopdownUnsupport::HYBRID,
                      "a directory of the two core scopes whose rows carry a "
                      "level token answers the hybrid kind (specs/042-topdown "
                      "FR-006, FR-007)");
}

// ---------------------------------------------------------------------------
// The grouping arm
// ---------------------------------------------------------------------------

// The grouping of the Zen 5 directory for one row of its own: the vendor spells
// the same memory row inside `l2-cache`, where the Skylake-X directory spells
// its row inside `cache` (FR-004, D-3, AP-7).
[[nodiscard]] auto zen5Groups() -> std::vector<TopdownEventGroup>
{
  return {TopdownEventGroup {.event = "l2_lines_out.dram_only_slc",
                             .group = "l2-cache"}};
}

// FR-004, D-3: an address no node names answers the stem of the vendored file
// that holds its row, verbatim. The two listings name the vendor's own stems,
// and no listing here holds a name twice: the accessor owns the first row of a
// repeated name, so no case repeats one (D-16).
auto scenarioAreaFallsBackToTheVendorFileStem() -> void
{
  const auto tree = backendTree();
  checkText(sg::detail::topdownArea(kListedAddress, tree, plantedGroups()),
            "cache",
            "an address no node names answers the stem of the file that holds "
            "its row, the Skylake-X spelling (FR-004, D-3)");
  checkText(sg::detail::topdownArea(
                "cpu/l2_lines_out.dram_only_slc", tree, zen5Groups()),
            "l2-cache",
            "the same reading over the Zen 5 listing answers that vendor's own "
            "stem, " "so no name is renamed (FR-004, D-3, AP-7)");
}

// FR-037, D-16: the index keys on the bare name, so the device prefix stands
// ahead of the lookup and not inside the key. The listing holds one row named
// `cache-misses`, and two device spellings of that name answer that one row.
// `leafPartOf` of `hunt_selection.hpp` cuts after the last `/` instead, and the
// two readings agree while a device path holds one name, the shape
// `provider.cpp` builds.
auto scenarioIndexDropsTheDevicePrefix() -> void
{
  const auto groups = std::vector<TopdownEventGroup> {
      TopdownEventGroup {.event = "cache-misses", .group = "cache"}};
  const auto tree = backendTree();
  checkText(sg::detail::topdownArea("cpu/cache-misses", tree, groups), "cache",
            "the address carries a device and the row carries the bare name, so "
            "the lookup reads the name with its prefix dropped (FR-037, D-16)");
  checkText(sg::detail::topdownArea("cpu_atom/cache-misses", tree, groups),
            "cache",
            "a second device spelling of the same bare name answers the one "
            "group the listing holds, so the strip names a prefix and no "
            "suffix (FR-037)");
}

// ---------------------------------------------------------------------------
// The closing arm and the purity of one call
// ---------------------------------------------------------------------------

// The descriptors this process holds, counted through its own descriptor
// directory. An event the counters library opens hands back a descriptor, so a
// call that left one open moves this listing (FR-001, D-7).
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
// listing (FR-001, D-7).
// GCC answers `__SANITIZE_ADDRESS__` and `__SANITIZE_THREAD__`; Clang answers
// `__has_feature`. The allocator of a sanitizer maps its pages as it needs
// them, so the mapping count of such a build moves on calls that read nothing.
// The mapping claim of the case below stands where that count is the proxy for
// a page a call maps.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#  define SG_TEST_TRACKS_MAPPINGS 0
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#    define SG_TEST_TRACKS_MAPPINGS 0
#  else
#    define SG_TEST_TRACKS_MAPPINGS 1
#  endif
#else
#  define SG_TEST_TRACKS_MAPPINGS 1
#endif

[[maybe_unused]] [[nodiscard]] auto mappingCount() -> std::size_t
{
  auto maps = std::ifstream("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

// US1 AC-5, FR-001, D-3: an address no node names and no row of the grouping
// names answers the unmapped word. No other case of this file settles that
// shape, because every other case hands a listing that holds the row or a tree
// that holds the node.
auto scenarioNoNodeAndNoRowAnswersUnmapped() -> void
{
  checkText(sg::detail::topdownArea("cpu/no_such_event_at_all",
                                    backendTree(),
                                    plantedGroups()),
            std::string {kUnmappedText},
            "no node of the tree and no row of the listing names the address, "
            "so the answer reads unmapped (FR-001, FR-004, D-3)");
}

// FR-006: the same three inputs answer one text on every call, for each of the
// three kinds of answer the mapper holds.
auto scenarioTwoCallsAnswerOneArea() -> void
{
  const auto tree = backendTree();
  const auto groups = plantedGroups();
  for (const auto& address : {std::string_view {kDeepAddress},
                              std::string_view {kListedAddress},
                              std::string_view {"cpu/no_such_event_at_all"}})
  {
    const auto first = sg::detail::topdownArea(address, tree, groups);
    const auto second = sg::detail::topdownArea(address, tree, groups);
    checkText(second, first,
              "one address over one tree and one listing answers one text "
              "twice, so no figure that moves between calls stands in the "
              "answer (FR-006)");
  }
}

// FR-001, D-7: one call reads its three arguments and nothing else. No
// descriptor stands open behind it and no page stands mapped behind it. The
// answer follows the record the case handed to the call.
auto scenarioOneCallReadsNoSourceOfItsOwn() -> void
{
  const auto tree = backendTree();
  const auto groups = plantedGroups();
  const auto absent = std::optional<sg::counters::TopdownTree> {};
  const auto descriptors = openDescriptorCount();
#if SG_TEST_TRACKS_MAPPINGS
  const auto mappings = mappingCount();
#endif

  const auto path = sg::detail::topdownArea(kDeepAddress, tree, groups);
  const auto stem = sg::detail::topdownArea(kListedAddress, tree, groups);
  const auto refused = sg::detail::topdownArea(kListedAddress, absent, groups);

  check(openDescriptorCount() == descriptors,
        "three calls leave the process holding the descriptors it held before, "
        "so no event stands open behind an area (FR-001, D-7)");
#if SG_TEST_TRACKS_MAPPINGS
  check(mappingCount() == mappings,
        "three calls leave the process holding the mappings it held before, so "
        "no sampled page stands behind an area (FR-001, D-7)");
#endif
  check(!path.empty() && !stem.empty() && !refused.empty(),
        "the three calls measured above settled three answers, so the claim "
        "weighs calls that reached every arm (FR-001)");
  checkText(refused, std::string {kUnmappedText},
            "the absent tree answers unmapped for the address the listing "
            "names, so the answer follows the record and not the listing "
            "(FR-005)");
  check(sg::detail::topdownArea(kDeepAddress, tieTree(), groups) != path,
        "one address over a tree that names no node for it answers another "
        "text, so the answer follows the record it was handed and no source of "
        "its own (FR-001, D-7)");
}

// ---------------------------------------------------------------------------
// The ask of the run
// ---------------------------------------------------------------------------

// The leaf address a node of the planted Intel table reads: the scripted core
// device of the metric fixture, spelled as the selection spells it (FR-029).
constexpr std::string_view kFixtureLeaf {"cpu_core/fixture_cycles"};

// The path the full planted table settles for that leaf, measured at the case
// below: the deepest node that reaches the leaf stands at level 6, and its tie
// with `tma_divider_high_4k` falls to the row that stands first in the table
// (FR-001, FR-003, D-4, D-17).
constexpr std::string_view kFixturePath {
    "topdown.backend_bound > topdown.intel.tma_memory_bound > "
    "topdown.intel.tma_dram_bound > topdown.intel.tma_dram_bandwidth > "
    "topdown.intel.tma_dram_page_miss > "
    "topdown.intel.tma_dram_page_miss_4k"};

// The same leaf over the table with its two deepest levels dropped: the clamped
// tree answers the deepest node it holds, which stands at level 4 (FR-008,
// D-17).
constexpr std::string_view kShallowFixturePath {
    "topdown.frontend_bound > topdown.intel.tma_fetch_latency > "
    "topdown.intel.tma_branch_resteers > topdown.intel.tma_clears_resteers"};

// The identity of a selection as one text: the published record carries no
// comparison operator, so two answers of it compare field by field through this
// text, the way the counters unit compares a source record.
[[nodiscard]] auto identityText(const CpuIdentity& identity) -> std::string
{
  return identity.vendor + ' ' + std::to_string(identity.family) + ' '
      + std::to_string(identity.model) + ' ' + std::to_string(identity.stepping)
      + ' ' + identity.coreType + ' '
      + (identity.midr.has_value() ? std::to_string(*identity.midr)
                                   : std::string {"no-midr"});
}

// The tree one ask answers, and the failure of a case that asked a tree and got
// a refusal.
[[nodiscard]] auto askedTree(const sg::counters::TopdownSelection& selection)
    -> std::optional<sg::counters::TopdownTree>
{
  const auto answered = sg::counters::topdownTree(selection);
  if (!answered.has_value()) {
    std::fprintf(stderr,
                 "HARNESS HUNT TOPDOWN TEST FAIL: the ask answered a refusal "
                 "where its case asked a tree: %s\n",
                 answered.error().message().c_str());
    std::exit(1);
  }
  return answered.value();
}

// Whether one table is the table of the ask: the record carries no comparison,
// so the two fields the seam moves are read (FR-033).
auto checkPlantedTable(const std::optional<TopdownTable>& ask,
                       const std::optional<TopdownTable>& planted,
                       const char* const what) -> void
{
  check(ask.has_value() == planted.has_value(), what);
  if (!ask.has_value()) {
    return;
  }
  check(ask->source.directory == planted->source.directory
            && ask->rows.size() == planted->rows.size(),
        what);
}

// FR-001, FR-033, FR-034, D-17: the ask of a planted run carries the planted
// identity and the planted table at the level of the pair, and the level a
// `--topdown` row names moves none of the three.
auto scenarioPlantedRunAsksTheAreaAtLevelSix() -> void
{
  auto options = plantedIdentityOptions();
  options.topdownLevel = 2;
  const auto ask = sg::detail::topdownAreaSelection(options);
  check(ask.level == sg::detail::kTopdownAreaLevel,
        "the area ask reads the deepest level the type takes (FR-001, D-17)");
  check(ask.level != static_cast<int>(*options.topdownLevel),
        "the level the option row names stands apart from the level of the "
        "ask, " "so it cannot move the area (FR-034)");
  check(identityText(ask.identity) == identityText(*options.topdownIdentity),
        "the ask carries the planted identity, so a case hands in a synthetic "
        "cpu (FR-001, FR-029)");
  checkPlantedTable(
      ask.table,
      options.topdownTable,
      "the ask carries the planted table of the option set " "(FR-033)");
  const auto tree = askedTree(ask);
  check(tree->depth == sg::detail::kTopdownAreaLevel,
        "the planted table holds the level the ask names, so the answer states "
        "no clamp (FR-008)");
  checkText(sg::detail::topdownArea(kFixtureLeaf, tree, {}),
            std::string {kFixturePath},
            "the mapper settles a path over the tree the planted ask answered "
            "(FR-001, FR-007)");
}

// FR-001, FR-033: an unplanted run asks the same level, hands no table, and
// reads the identity of the cpu this process runs on. The case names no host
// fact: it compares two answers of one reader (FR-030).
auto scenarioUnplantedRunAsksTheAreaAtLevelSix() -> void
{
  const auto ask = sg::detail::topdownAreaSelection(sg::detail::RunOptions {});
  check(ask.level == sg::detail::kTopdownAreaLevel,
        "the level of the ask is the constant of the pair, planted or not "
        "(FR-001, D-17)");
  check(!ask.table.has_value(),
        "no planted table stands in an unplanted run, so the selection reads "
        "the table of the identity (FR-033)");
  check(identityText(ask.identity) == identityText(sg::counters::cpuIdentity()),
        "the identity of the ask is the answer of the library's own reader, a "
        "fact of the reader and no claim about the host (FR-001, FR-030)");
}

// FR-001, D-17, FR-008: a level-6 ask over a table that holds no level deeper
// than 4 answers that table's own depth, and the mapper still settles a path.
auto scenarioLevelSixAskClampsToAShallowTable() -> void
{
  auto options = sg::detail::RunOptions {};
  options.topdownIdentity = intelIdentity(6, 85, 4);
  options.topdownTable =
      plantedTable(kIntelDirectory, topdownIntelShallowRows());
  const auto ask = sg::detail::topdownAreaSelection(options);
  check(ask.level == sg::detail::kTopdownAreaLevel,
        "the ask keeps the level of the pair over a shallow table (D-17)");
  const auto tree = askedTree(ask);
  check(tree->depth == 4,
        "the table clamps the ask to its own deepest level, which is the "
        "reading `topdownTree` states (FR-008, D-17)");
  checkText(sg::detail::topdownArea(kFixtureLeaf, tree, {}),
            std::string {kShallowFixturePath},
            "the clamped tree still reaches the leaf, so the mapper settles "
            "the " "path of the deepest node that table holds (FR-001, D-17)");
}

// FR-033, D-17: the two asks of one option set are one record but for the
// level, so one identity and one table read feed both uses. The level-1
// companion of D-9 lands with its own collector and reads this shape.
auto scenarioAreaAndCompanionAsksDifferInLevelAlone() -> void
{
  auto options = plantedIdentityOptions();
  options.topdownLevel = 3;
  const auto area = sg::detail::topdownAreaSelection(options);
  auto companion = area;
  companion.level = 1;
  check(area.level != companion.level,
        "the two asks of one run hold two levels, so the deep tree cannot "
        "answer the block and the level-1 tree cannot answer the area (D-17)");
  check(identityText(companion.identity) == identityText(area.identity),
        "one identity feeds both asks (FR-033)");
  checkPlantedTable(companion.table, area.table,
                    "one table read feeds both asks, so the two asks differ in "
                    "the level alone (FR-033)");
}

// ---------------------------------------------------------------------------
// The companion of a hunter run
// ---------------------------------------------------------------------------

// FR-033, FR-035, D-9: the option set of the companion asks one short level-1
// pass over the identity and the table of the pair. It carries nothing of the
// run it rides, and the run keeps its own option set whole.
auto scenarioCompanionOptionsAskOneShortLevelOnePass() -> void
{
  auto options = plantedIdentityOptions();
  options.topdownLevel = 4;
  options.dryRun = true;
  options.repetitions = 7;
  options.counterAddresses = {std::string {kListedAddress}};
  options.metricMethod = sg::MetricMethod::MULTIPLEX;
  const auto area = sg::detail::topdownAreaSelection(options);
  const auto ask = sg::detail::topdownCompanionOptions(options);
  check(ask.topdownLevel == sg::detail::kTopdownCompanionLevel,
        "the companion asks the level D-9 names, and no other level (FR-033, "
        "FR-034)");
  check(
      ask.topdownCompanion,
      "the option set of the companion carries the bit that names it (FR-035)");
  check(ask.repetitions == 1 && ask.fixedIterations == 1,
        "one repetition of one fixed iteration, so the companion prices one "
        "short pass (D-9)");
  check(ask.dryRun == options.dryRun,
        "a dry run leaves the companion a dry run, the way every pass of such "
        "a " "run reads (D-9)");
  check(!ask.hunt.has_value() && ask.counterAddresses.empty()
            && !ask.metricMethod.has_value(),
        "no hunt, no address and no metric method of the run ride the ask of "
        "the " "companion (FR-035)");
  check(identityText(*ask.topdownIdentity) == identityText(area.identity),
        "one identity feeds the area ask and the companion ask of one run "
        "(FR-033, D-17)");
  checkPlantedTable(ask.topdownTable,
                    area.table,
                    "one table read feeds both asks, so the two asks differ in "
                    "the level alone (FR-033, D-17)");
  check(options.topdownLevel == 4 && options.repetitions == 7
            && !options.topdownCompanion,
        "the answer is a fresh set: the option set of the run keeps the level, "
        "the repetitions and the bit it held (FR-035)");
}

// FR-035, FR-017: the answer of the companion stands empty where its bit stands
// clear. The planted options of this case ask a tree that stands at level 1, so
// the bit alone answers the empty.
auto scenarioCompanionRecordStandsClearWithoutTheBit() -> void
{
  const auto options = plantedIdentityOptions();
  const auto area = sg::detail::topdownAreaSelection(options);
  auto flat = area;
  flat.level = static_cast<int>(sg::detail::kTopdownCompanionLevel);
  const auto standing = askedTree(flat);
  auto instance = sg::Instance {};
  check(!sg::detail::topdownCompanionRecord(options, instance).has_value(),
        "a run that asks no companion collects no record, the bytes of the run "
        "before the companion stood (FR-035)");
  check(sg::detail::topdownAreaSelection(options).table.has_value()
            && standing->depth
                   >= static_cast<int>(sg::detail::kTopdownCompanionLevel),
        "the ask of the same options answers a tree that stands, so the clear "
        "bit alone settled the empty answer (FR-035)");
}

// FR-017: a host the level-1 ask refuses collects nothing. The identity names
// no vendor this build vendors a table for, so the ask answers a refusal, the
// pass never runs, and the instance reaches no read.
auto scenarioCompanionRecordStandsClearWhereTheAskRefuses() -> void
{
  auto options = sg::detail::RunOptions {};
  options.topdownIdentity = intelIdentity(15, 6, 5);
  options.topdownCompanion = true;
  auto instance = sg::Instance {};
  check(!sg::detail::topdownCompanionRecord(options, instance).has_value(),
        "the run on a host that answers no tree collects no record and prints "
        "no " "block (FR-017, FR-005)");
}

// ---------------------------------------------------------------------------
// The AMD vendor vocabulary of a planted pipeline table
// ---------------------------------------------------------------------------

// The leaf every level-2 row of a fixture AMD pipeline table reads: the
// scripted core device of the metric fixture and the reference cycles of its
// leaves.
constexpr std::string_view kAmdLeaf {"cpu_core/fixture_ref_cycles"};

// The path the Zen 4 table settles for that leaf: the level-1 name the row's
// parent token reaches, then the Zen 4 spelling of the level-2 row that stands
// first among the rows of its depth (FR-003, FR-024, D-3, D-10).
constexpr std::string_view kZen4LeafPath {
    "topdown.frontend_bound > topdown.amd.frontend_bound_latency"};

// The same leaf over the Zen 5 table, whose own file spells that row with `by`:
// one address, two vendor texts, two AMD generations (FR-024, SC-001, D-3).
constexpr std::string_view kZen5LeafPath {
    "topdown.frontend_bound > topdown.amd.frontend_bound_by_latency"};

// The row two AMD directories hold, each inside a file of its own: Zen 4 holds
// it inside `cache` and Zen 5 inside `l2-cache`, read at `amdzen4/cache.json`
// and `amdzen5/l2-cache.json` (D-3, AP-7).
[[nodiscard]] auto amdRequestRow(const bool zen5) -> TopdownEventGroup
{
  return TopdownEventGroup {.event = "l2_request_g1.group2",
                            .group = zen5 ? "l2-cache" : "cache"};
}

// The area ask of one AMD pipeline table beside one AMD identity: the two seams
// FR-033 names, so the table reaches the ask and no host answers it (FR-029).
[[nodiscard]] auto amdAreaAsk(const CpuIdentity& identity,
                              const std::string_view directory,
                              const std::vector<MetricRow>& rows)
    -> sg::counters::TopdownSelection
{
  sg::detail::RunOptions options;
  options.topdownIdentity = identity;
  options.topdownTable = plantedTable(directory, rows);
  return sg::detail::topdownAreaSelection(options);
}

// Whether one tree holds a name of the Intel vocabulary: the prefix its metric
// rows carry and the label its level-2 names carry (FR-028).
[[nodiscard]] auto holdsIntelName(const sg::counters::TopdownTree& tree) -> bool
{
  return std::ranges::any_of(tree.nodes,
                             [](const sg::counters::TopdownNode& node) -> bool {
                               return node.name.contains("tma_")
                                   || node.name.contains("topdown.intel.");
                             });
}

// Whether one tree holds a level-2 name outside the AMD label, the label the
// selection gives an AMD table alone (FR-024, FR-028).
[[nodiscard]] auto holdsUnlabeledLevelTwoName(
    const sg::counters::TopdownTree& tree) -> bool
{
  return std::ranges::any_of(
      tree.nodes,
      [](const sg::counters::TopdownNode& node) -> bool
      { return node.level > 1 && !node.name.starts_with("topdown.amd."); });
}

// SC-001, FR-024 and D-3: the Zen 5 table answers the Zen 5 names beside the
// Zen 4 identity of the standing helpers, so one address reaches one vendor
// text per generation and the claim covers two AMD generations.
auto scenarioZenFiveAreaAnswersTheVendorNames() -> void
{
  const auto tree = askedTree(
      amdAreaAsk(amdIdentity(25, 17), kZen5Directory, topdownZen5Rows()));
  checkText(sg::detail::topdownArea(kAmdLeaf, tree, {}),
            std::string {kZen5LeafPath},
            "the Zen 5 table settles its own level-2 spelling, which carries "
            "the word its Zen 4 sibling omits (FR-024, SC-001, D-3)");
  check(!holdsIntelName(*tree),
        "no node of the Zen 5 table carries an Intel name, because the names "
        "come from the table and no renaming stands between them (FR-024, "
        "FR-028, D-3)");
  check(!holdsUnlabeledLevelTwoName(*tree),
        "every level-2 name of that table stands below the AMD label the "
        "identity selects (FR-024, FR-028)");
  const std::string request {"cpu/" + std::string {amdRequestRow(true).event}};
  checkText(sg::detail::topdownArea(request, tree, {amdRequestRow(true)}),
            "l2-cache",
            "the fallback reads the Zen 5 stem of the row that reads cache one "
            "generation earlier, so the vocabulary belongs to the directory "
            "(FR-004, D-3, AP-7)");
  checkText(sg::detail::topdownArea(request, tree, {amdRequestRow(false)}),
            "cache",
            "the same address over the Zen 4 stem answers that stem verbatim, "
            "so no listing reaches the cell renamed (FR-004, D-3)");
}

// FR-029 and the first scenario of US4: a planted table settles an AMD path on
// this host, and the identity alone settles none, so no claim of this unit
// rests on the vendor of the cpu it runs on.
auto scenarioAnAmdNodeNamesAnAmdPath() -> void
{
  const CpuIdentity identity = amdIdentity(25, 17);
  const auto tree =
      askedTree(amdAreaAsk(identity, kZen4Directory, topdownZen4Rows()));
  checkText(sg::detail::topdownArea(kAmdLeaf, tree, {}),
            std::string {kZen4LeafPath},
            "the Zen 4 table settles the Zen 4 spelling of the same leaf, so "
            "the two generations answer two texts (FR-024, SC-001, D-3)");
  check(!holdsIntelName(*tree) && !holdsUnlabeledLevelTwoName(*tree),
        "every name of that path and of the table above it reads an AMD name, "
        "so an AMD part answers no Intel name (FR-024, FR-028, FR-029)");
  check(tree->depth == 2 && tree->source.directory == std::string {kZen4Directory},
        "the planted table answers the two levels and the label its own rows "
        "carry, so a table and no host settles the path (FR-029, FR-030)");

  sg::detail::RunOptions bare;
  bare.topdownIdentity = identity;
  const auto alone =
      sg::counters::topdownTree(sg::detail::topdownAreaSelection(bare));
  const std::optional<sg::counters::TopdownTree> aloneTree =
      alone.has_value() ? std::optional {alone.value()} : std::nullopt;
  checkText(sg::detail::topdownArea(kAmdLeaf, aloneTree, {}),
            std::string {kUnmappedText},
            "the identity alone names no node of that leaf, so the planted "
            "table settles the path and the identity settles none (FR-029, "
            "FR-030, D-16)");
  const std::string request {"cpu/" + std::string {amdRequestRow(false).event}};
  checkText(sg::detail::topdownArea(request, aloneTree, {amdRequestRow(false)}),
            "cache",
            "the same ask still reads the stem it is handed, because a tree "
            "answer moves no listing (D-16, FR-004)");
}

// ---------------------------------------------------------------------------
// The stamp
// ---------------------------------------------------------------------------

// One record of the run the stamp reads, built by hand the way the top-down
// report unit builds a record: the hunt record holds one ranked row, one row
// the rank table left out on its deviation, and one skipped event the stamp
// must leave alone (FR-009, FR-010).
[[nodiscard]] auto huntedResult(const std::string& name) -> sg::BenchmarkResult
{
  auto ranked = sg::hunt::RankedEvent {};
  ranked.address = std::string {kListedAddress};
  ranked.rate = 12.5;
  ranked.cv = 1.25;
  ranked.samples = 3;
  ranked.repeated = true;
  ranked.rank = std::size_t {1};
  auto unranked = sg::hunt::RankedEvent {};
  unranked.address = std::string {kBranchAddress};
  unranked.rate = 3.5;
  unranked.cv = 31.0;
  unranked.samples = 1;
  unranked.noisy = true;
  auto skipped = sg::hunt::SkippedEvent {};
  skipped.address = "cpu/refused_event";
  skipped.kind = sg::hunt::SkippedKind::PERMISSION_BLOCKED;
  auto hunt = sg::hunt::EntryHunt {};
  hunt.passCount = 1;
  hunt.entries = {std::move(ranked), std::move(unranked)};
  hunt.skipped = {std::move(skipped)};
  auto result = sg::BenchmarkResult {};
  result.name = name;
  result.suite = name;
  result.caseName = name;
  result.outcome = sg::RunOutcome::MEASURED;
  result.hunt = std::move(hunt);
  return result;
}

// The fields of one record the stamp may not move, as one text: no carrier
// here holds a comparison operator, so the two answers of one record compare
// field by field through this text.
[[nodiscard]] auto resultText(const sg::BenchmarkResult& result) -> std::string
{
  auto text = std::string {result.name};
  text += '|';
  text += result.suite;
  text += '|';
  text += result.caseName;
  text += '|';
  text += std::to_string(result.rows.size());
  text += '|';
  text += result.hunt.has_value()
      ? "hunt:" + std::to_string(result.hunt->entries.size()) + ':'
          + std::to_string(result.hunt->skipped.size())
      : std::string {"no-hunt"};
  return text;
}

// FR-009, D-8: one stamp settles the area of every record of `entries`, the
// ranked rows and the unranked row alike, and the skipped list gains nothing.
auto scenarioStampSetsEveryEntryIncludingUnranked() -> void
{
  auto results =
      std::vector<sg::BenchmarkResult> {huntedResult("bmTopdownStamp")};
  const auto tree = std::optional {backendTree()};
  sg::detail::stampTopdownAreas(results, tree, plantedGroups());
  const auto& hunt = *results.front().hunt;
  check(hunt.entries.size() == 2,
        "the stamp added no record and removed none, so the table keeps its "
        "shape (FR-010)");
  checkText(hunt.entries[0].address, std::string {kListedAddress},
            "the ranked row still stands first, so the stamp reordered nothing "
            "(FR-010)");
  checkText(hunt.entries[0].topdownArea, "cache",
            "the ranked row carries the stem of the row its address names "
            "(FR-009)");
  checkText(hunt.entries[1].topdownArea,
            "topdown.backend_bound > topdown.intel.tma_core_bound",
            "the row no rank reached carries its area too, because `entries` "
            "covers every planned column of the run (FR-009, D-8)");
  check(hunt.entries[0].rank.has_value() && *hunt.entries[0].rank == 1
            && !hunt.entries[1].rank.has_value(),
        "the rank of each row stands as it stood, the unranked row included "
        "(FR-010)");
  check(sameDouble(hunt.entries[0].rate, 12.5)
            && sameDouble(hunt.entries[0].cv, 1.25)
            && sameDouble(hunt.entries[1].rate, 3.5)
            && sameDouble(hunt.entries[1].cv, 31.0),
        "no figure of any record moves with an area set (FR-010)");
  check(hunt.skipped.size() == 1
            && hunt.skipped.front().address == "cpu/refused_event",
        "the skipped list gains no entry and loses none (FR-009)");
}

// FR-010: a result that carries no hunt record changes by zero field, so the
// stamp reaches the hunted entries of a run and no other.
auto scenarioStampLeavesAnUnhuntedResultAlone() -> void
{
  auto plain = sg::BenchmarkResult {};
  plain.name = "bmNoHunt";
  plain.suite = "bmNoHunt";
  plain.caseName = "bmNoHunt";
  plain.outcome = sg::RunOutcome::MEASURED;
  const auto before = resultText(plain);
  auto results = std::vector<sg::BenchmarkResult> {std::move(plain)};
  sg::detail::stampTopdownAreas(results, std::nullopt, {});
  checkText(resultText(results.front()), before,
            "a record no hunt reached changes by zero field, so the stamp adds "
            "no hunt record and moves no field of one (FR-010)");
  check(!results.front().hunt.has_value(),
        "the stamp of a refused host adds no hunt record to an entry that "
        "carries none (FR-010)");
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
    std::fprintf(stderr,
                 "harness_hunt_topdown_test: cannot start a scenario\n");
    return false;
  }
  if (child == 0) {
    scenario.body();
    std::fflush(stdout);
    // libgcov writes its counters from the exit hooks only a return from main
    // reaches, so a child that ends by `_Exit` leaves the lines it ran out of
    // the coverage trace. The dump ahead of the death keeps them (FR-031). The
    // symbol is weak: an uninstrumented build has none and the call stays out
    // of the way.
    if (&__gcov_dump != nullptr) {
      __gcov_dump();
    }
    std::_Exit(0);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    std::fprintf(stderr, "harness_hunt_topdown_test: cannot read a status\n");
    return false;
  }
  const bool passed = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  std::printf("harness_hunt_topdown_test %s: %s\n",
              scenario.name,
              passed ? "pass" : "FAIL");
  std::fflush(stdout);
  return passed;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  // The scripted host a planted table resolves its rows against: the metric
  // fixture device and its leaves, laid down once per process. No case opens an
  // event on it, and a seam answers from its own record (FR-029, FR-030).
  if (!registerMetricHost()) {
    std::fprintf(stderr,
                 "harness_hunt_topdown_test: the metric fixture cannot "
                 "register its host\n");
    return 2;
  }
  // A case that plants a contract breach dies at the contract call, where no
  // exit hook runs. The handler of the house fixture writes the counters first
  // and then dies by the same signal, so the parent still sees the death it
  // asserts (FR-031).
  sg::test::flushCountersOnAbort();
  const std::vector<Scenario> scenarios = {
      {"area-takes-the-deepest-node", scenarioAreaTakesTheDeepestNode},
      {"level-one-node-answers-one-name", scenarioLevelOneNodeAnswersOneName},
      {"tie-goes-to-the-first-row-in-table-order",
       scenarioTieGoesToTheFirstRowInTableOrder},
      {"second-spelling-names-no-node", scenarioSecondSpellingNamesNoNode},
      {"parent-behind-its-child-dies-at-the-contract",
       scenarioParentBehindItsChildDiesAtTheContract},
      {"arm-midr-answers-unmapped", scenarioArmMidrAnswersUnmapped},
      {"unknown-vendor-answers-unmapped", scenarioUnknownVendorAnswersUnmapped},
      {"no-directory-answers-unmapped", scenarioNoDirectoryAnswersUnmapped},
      {"no-topdown-rows-answers-unmapped",
       scenarioNoTopdownRowsAnswersUnmapped},
      {"two-scope-rows-answer-unmapped", scenarioTwoScopeRowsAnswerUnmapped},
      {"area-falls-back-to-the-vendor-file-stem",
       scenarioAreaFallsBackToTheVendorFileStem},
      {"index-drops-the-device-prefix", scenarioIndexDropsTheDevicePrefix},
      {"no-node-and-no-row-answers-unmapped",
       scenarioNoNodeAndNoRowAnswersUnmapped},
      {"two-calls-answer-one-area", scenarioTwoCallsAnswerOneArea},
      {"oneCallReadsNoSourceOfItsOwn", scenarioOneCallReadsNoSourceOfItsOwn},
      {"planted-run-asks-the-area-at-level-six",
       scenarioPlantedRunAsksTheAreaAtLevelSix},
      {"unplanted-run-asks-the-area-at-level-six",
       scenarioUnplantedRunAsksTheAreaAtLevelSix},
      {"level-six-ask-clamps-to-a-shallow-table",
       scenarioLevelSixAskClampsToAShallowTable},
      {"area-and-companion-asks-differ-in-level-alone",
       scenarioAreaAndCompanionAsksDifferInLevelAlone},
      {"companion-options-ask-one-short-level-one-pass",
       scenarioCompanionOptionsAskOneShortLevelOnePass},
      {"companion-record-stands-clear-without-the-bit",
       scenarioCompanionRecordStandsClearWithoutTheBit},
      {"companion-record-stands-clear-where-the-ask-refuses",
       scenarioCompanionRecordStandsClearWhereTheAskRefuses},
      {"zen-five-area-answers-the-vendor-names",
       scenarioZenFiveAreaAnswersTheVendorNames},
      {"an-amd-node-names-an-amd-path", scenarioAnAmdNodeNamesAnAmdPath},
      {"stamp-sets-every-entry-including-unranked",
       scenarioStampSetsEveryEntryIncludingUnranked},
      {"stamp-leaves-an-unhunted-result-alone",
       scenarioStampLeavesAnUnhuntedResultAlone},
  };
  // `argv[1]` selects one scenario, so one arm of the mapper runs alone under a
  // debugger. No argument runs the whole table.
  const std::string requested =
      argc > 1 && argv[1] != nullptr ? argv[1] : std::string();
  for (const Scenario& scenario : scenarios) {
    if (requested == scenario.name) {
      scenario.body();
      return 0;
    }
  }
  if (!requested.empty()) {
    std::fprintf(stderr,
                 "harness_hunt_topdown_test: no scenario named '%s'\n",
                 requested.c_str());
    return 2;
  }
  std::size_t failedScenarios = 0;
  for (const auto& scenario : scenarios) {
    if (!runIsolated(scenario)) {
      ++failedScenarios;
    }
  }
  if (failedScenarios != 0) {
    std::printf("%zu scenarios failed\n", failedScenarios);
  }
  return failedScenarios == 0 ? 0 : 1;
}
