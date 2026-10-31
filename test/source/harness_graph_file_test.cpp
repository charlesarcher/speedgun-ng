// ============================================================================
// The graph file: the refusal arms of the reader, the ten faults FR-010
// lists in its own order, and the clean file that passes them
// (specs/037-execution-graphs T005, T006, T007; US3; FR-007, FR-008,
// FR-009, FR-010; SC-003; research R-03, D-1, D-2).
//
// One planted file per arm stands under a temporary directory, the standing
// fake-tree shape of specs/034 R-11, and the reader answers one fault line
// for each: a file the run cannot read names the path; a file the parser
// rejects names the path and the fault; a document that breaks the shape of
// `docs/schema/graph-v1.json` names the key, the order break or the kind; a
// node rule broken names the name, the node, the slot, the edge, the cycle
// or the slot the run starts no node for. The shape check finishes before
// the node rules (FR-007), and the first fault prints alone (SC-003).
//
// The reader stands declared for this translation unit below, the way
// `harness_compare_test.cpp:106` declares the compare bodies it drives and
// `harness_hunt_plan_test.cpp:81` declares the budget body:
//   detail::readGraphFile(path, contextCounts)
//       -> std::expected<detail::Graph, std::string>
// and `source/harness/graph_file.cpp` (T009) defines it.
//
// The refusal shape SC-003 pins: one line, exit status 2, and no context
// started. The one line and the no-context arms stand here: the fault text
// carries no line break, and the seam slot the runner reads at each
// threaded attempt (FR-011, FR-025) stands empty after every refusal, so a
// refused run reads no graph and starts no context. The exit status stands
// at the entry point, the standing split `harness_cli_sources_test.cpp:62`
// states for a source fault: the unit answers the fault text, and the entry
// point prints that text at status 2 (`harness_cli_graph_test.cpp`, T023).
//
// Scenario map:
//   unreadableFileRefuses  no file stands at the path the reader names: the
//                          fault names the path (FR-010 rule 1)
//   parserRejectRefuses    a truncated document: the fault names the path
//                          and the fault the parser answered (FR-010 rule 2)
//   schemaShapeBreakRefuses  a key the schema admits never, a key out of the
//                          `x-order`, and a `slot` spelled as a string: the
//                          faults name the key, the order break and the kind
//                          (FR-007, FR-010 rule 3)
//   readerChecksShapeAgainstSchemaFile
//                          a file that breaks the shape and a node rule
//                          prints the shape line alone, and a file that
//                          breaks one node rule prints the node line: the
//                          shape check of the schema file finishes before
//                          the node rules (FR-007)
//   repeatedNameRefuses    two nodes carry one name: the line names the
//                          repeated name (FR-010 rule 4)
//   missingSlotRefuses     one node carries no slot: the line names the node
//                          and the key (FR-010 rule 5)
//   slotOutOfRangeRefuses  one slot stands outside the run's context count:
//                          the line names the node and the slot against that
//                          count (FR-010 rule 6)
//   negativeSlotRefuses    one slot stands below zero, the bound the schema
//                          file sets none: the line names the node, the slot
//                          and the count (FR-007, FR-010 rule 6)
//   edgeNamesNoNodeRefuses one dependency names no node: the line names the
//                          edge (FR-010 rule 7)
//   selfEdgeRefuses        one node names itself: the line names the node
//                          (FR-010 rule 8)
//   cycleRefuses           two nodes name each other: the line names the
//                          cycle (FR-010 rule 9)
//   unsatisfiableDependencyRefuses
//                          two nodes name one slot: the line names the node
//                          and the slot the run starts no node for (FR-010
//                          rule 10, D-2)
//   firstFaultPrintsAlone  a repeated name beside a cycle prints the
//                          repeated-name line alone (SC-003)
//   fileCarriesNoExtraKeys the clean chain file carries no key beside
//                          `nodes`, `name`, `slot` and `deps`, passes every
//                          rule, and reaches the gate; one node key beside
//                          the four stands refused (FR-008)
//   nodeOrderOtherThanSlots
//                          a chain whose nodes stand in an order other than
//                          their slots passes every rule, and the release
//                          round of FR-013 leaves out the node an earlier
//                          round settled (FR-008, FR-013)
//
// `argv[1]` selects one scenario; with no argument the list at the bottom
// runs, and that list is what CTest runs, so a scenario absent from it
// never runs. Frameworkless check()/fail() convention.
// ============================================================================

#include <array>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "detail/graph.hpp"
#include "detail/internal.hpp"

// The reader of the graph file. `source/harness/graph_file.cpp` (T009)
// defines it; this block carries its own declaration for this translation
// unit, the way `harness_compare_test.cpp:106` declares the compare bodies
// it drives. The contract of the two parameters and of the answer stands at
// the definition: `path` names the file the operator gave, `contextCounts`
// the context count of every threaded run of the batch in batch order
// (FR-012), and the answer carries the graph or the one fault line FR-010
// names.
namespace sg::detail
{
[[nodiscard]] auto readGraphFile(std::string_view path,
                                 std::span<const std::uint32_t> contextCounts)
    -> std::expected<Graph, std::string>;
}  // namespace sg::detail

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS GRAPH FILE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// ---------------------------------------------------------------------------
// The planted files
// ---------------------------------------------------------------------------

// The directory the plants stand in: the system temporary directory under
// this process number, the shape `harness_cli_sources_test.cpp` carries for
// its captures, so two runs of the suite never share one file.
[[nodiscard]] auto plantDirectory() -> const std::filesystem::path&
{
  static const std::filesystem::path kDirectory =
      std::filesystem::temp_directory_path()
      / ("harness_graph_file_" + std::to_string(static_cast<long>(getpid())));
  return kDirectory;
}

// One planted file: the text `text` under the name `name`, and the answer
// is the path the reader is handed. An empty `text` plants no file at all,
// the arm FR-010 rule 1 reads.
//
// A name stands apart from the words its scenario requires of a fault line:
// every node-rule line quotes the path, so a name carrying a required word
// answers the check from its own spelling and the check proves nothing. One
// name per scenario stands here too, so no scenario plants over a file
// another scenario planted.
[[nodiscard]] auto plant(const std::string& name, std::string_view text)
    -> std::string
{
  std::filesystem::create_directories(plantDirectory());
  const std::filesystem::path path = plantDirectory() / name;
  if (text.empty()) {
    std::filesystem::remove(path);
    return path.string();
  }
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  stream << text;
  stream.close();
  if (!stream) {
    fail("the planted file stands written (the temporary directory works)");
  }
  return path.string();
}

// The chain of quickstart.md: three nodes over the slots 0, 1 and 2, the
// file naming the nodes in the order of their slots (FR-008).
constexpr std::string_view kChain = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "c", "slot": 2, "deps": ["b"] }
  ]
})";

// One node, the shape the three shape arms break: a key the schema admits
// never beside `nodes`, a node whose keys stand out of the `x-order`, a
// `slot` spelled as a string, and a node key beside the four (FR-007,
// FR-008).
constexpr std::string_view kExtraKey = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] }
  ],
  "preset": "ht"
})";
constexpr std::string_view kOrderBreak = R"({
  "nodes": [
    { "slot": 0, "name": "a", "deps": [] }
  ]
})";
constexpr std::string_view kSlotAsText = R"({
  "nodes": [
    { "name": "a", "slot": "0", "deps": [] }
  ]
})";

// Two nodes carry one name: FR-010 rule 4.
constexpr std::string_view kRepeatedName = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "a", "slot": 1, "deps": [] }
  ]
})";

// One node carries no slot: FR-010 rule 5.
constexpr std::string_view kNoSlot = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "deps": [] }
  ]
})";

// One slot stands outside the run's two contexts: FR-010 rule 6.
constexpr std::string_view kSlotOutside = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 5, "deps": [] }
  ]
})";

// One dependency names no node, and one node names itself: FR-010 rules 7
// and 8.
constexpr std::string_view kEdgeNoNode = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": ["zz"] }
  ]
})";
constexpr std::string_view kSelfEdge = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": ["a"] }
  ]
})";

// One slot stands below zero: FR-010 rule 6. The schema file names `slot` as
// an integer and sets it no bound (FR-007), so the node reaches the node
// rules and the run's own count settles the fault.
constexpr std::string_view kNegativeSlot = R"({
  "nodes": [
    { "name": "a", "slot": -1, "deps": [] },
    { "name": "b", "slot": 1, "deps": [] }
  ]
})";

// The chain of quickstart.md spelled in an order other than its slots: `c`
// first, `b` behind it, `a` behind `b`. The file's order is no release order,
// so the release round of FR-013 settles `a` in its first round and meets
// `a` standing settled in its second (FR-008, FR-013).
constexpr std::string_view kFileOrderChain = R"({
  "nodes": [
    { "name": "c", "slot": 2, "deps": ["b"] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "a", "slot": 0, "deps": [] }
  ]
})";

// Two nodes name each other: the cycle of FR-010 rule 9.
constexpr std::string_view kCycle = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": ["b"] },
    { "name": "b", "slot": 1, "deps": ["a"] }
  ]
})";

// Two nodes name one slot, so the run of two contexts starts no node for
// slot 1: the unsatisfiable dependency of FR-010 rule 10 (D-2).
constexpr std::string_view kOneSlot = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 0, "deps": [] }
  ]
})";

// A repeated name beside a cycle of two nodes: the file carries two faults,
// and the first in the FR-010 order prints alone (SC-003).
constexpr std::string_view kRepeatedAndCycle = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": ["b"] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "a", "slot": 2, "deps": [] }
  ]
})";

// One node key beside the four: the schema file closes the node object
// (FR-008).
constexpr std::string_view kNodeCpu = R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [], "cpu": 3 }
  ]
})";

// The context counts of the runs the arms read: one threaded run of three
// contexts, the node count of the chain, and of two and of one (FR-012).
constexpr std::array<std::uint32_t, 1> kRunOfThree {3};
constexpr std::array<std::uint32_t, 1> kRunOfTwo {2};
constexpr std::array<std::uint32_t, 1> kRunOfOne {1};

// ---------------------------------------------------------------------------
// The refusal shape of SC-003
// ---------------------------------------------------------------------------

// One refusal: the reader answered no graph, the fault text is one line,
// and the seam slot stands empty, so the refused run reads no graph and
// starts no context (FR-010, FR-011, SC-003). The answer is the fault text,
// for the arm that names what the line must carry.
[[nodiscard]] auto refusalOf(
    const std::expected<sg::detail::Graph, std::string>& answer,
    const char* what) -> std::string
{
  check(!answer.has_value(), what);
  const std::string& fault = answer.error();
  check(!fault.empty(), "the refusal names the fault (FR-010)");
  check(fault.find('\n') == std::string::npos,
        "the refusal prints one line (SC-003)");
  check(!sg::detail::graphInput().has_value(),
        "the refusal starts no context: the graph slot stands empty (FR-011, "
        "FR-025, SC-003)");
  return fault;
}

// The fault text names each of `names`, so the arm states what the one line
// carries (the table of contracts/graph-file.md).
auto faultNames(const std::string& fault,
                std::initializer_list<std::string_view> names,
                const char* what) -> void
{
  for (const auto name : names) {
    check(fault.find(std::string(name)) != std::string::npos, what);
  }
}

// ---------------------------------------------------------------------------
// The shape arms of FR-010 rules 1 to 3
// ---------------------------------------------------------------------------

// FR-010 rule 1: no file stands at the path, and the fault names the path.
auto unreadableFileRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("absent.json", {});
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfThree));
  const std::string fault = refusalOf(
      answer, "a file the run cannot read stands refused (FR-010 rule 1)");
  faultNames(fault, {path}, "the unreadable-file line names the path (FR-010)");
}

// FR-010 rule 2: a truncated document, and the fault names the path and the
// fault the parser answered.
auto parserRejectRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path =
      plant("truncated.json", R"({"nodes": [ { "name": "a", "slot": 0, )");
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfThree));
  const std::string fault = refusalOf(
      answer, "a file the parser rejects stands refused (FR-010 rule 2)");
  faultNames(fault,
             {path, "parser"},
             "the parser line names the path and the fault (FR-010 rule 2)");
}

// FR-010 rule 3: the three shape breaks the schema file names. A key the
// schema admits never, a key out of the `x-order`, and a `slot` spelled as a
// string (FR-007, FR-008).
auto schemaShapeBreakRefuses() -> void
{
  struct Arm
  {
    const char* name;
    std::string_view text;
    std::initializer_list<std::string_view> names;
    const char* what;
  };

  const std::array<Arm, 3> arms {
      Arm {"extra-key.json",
           kExtraKey,
           {"preset", "lists no key"},
           "a key the schema admits never names the key (FR-007, FR-008)"},
      Arm {"order.json",
           kOrderBreak,
           {"name", "out of the order"},
           "a key out of the x-order names the order break (FR-007)"},
      Arm {"kind.json",
           kSlotAsText,
           {"slot", "integer", "string"},
           "a slot spelled as a string names the kind (FR-007)"},
  };

  for (const auto& arm : arms) {
    sg::detail::installTestGraph(std::nullopt);
    const std::string path = plant(arm.name, arm.text);
    const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
    const std::string fault = refusalOf(answer, arm.what);
    faultNames(fault, arm.names, arm.what);
  }
}

// FR-007: the shape check of `docs/schema/graph-v1.json` finishes before
// the node rules. A file that breaks the shape and a node rule at once
// prints the shape line alone, and a file that breaks one node rule prints
// the node line, so the two checks stand in the order FR-007 states.
auto readerChecksShapeAgainstSchemaFile() -> void
{
  // The repeated name stands a node rule (FR-010 rule 4), and the key
  // `preset` stands a shape break (FR-010 rule 3): the shape line names the
  // key and the node rule prints nothing.
  sg::detail::installTestGraph(std::nullopt);
  const std::string both = plant("shape-and-node.json",
                                 R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "a", "slot": 1, "deps": [] }
  ],
  "preset": "ht"
})");
  const auto shapeFirst = sg::detail::readGraphFile(both, std::span(kRunOfTwo));
  const std::string shapeFault =
      refusalOf(shapeFirst, "a file that breaks both stands refused (FR-007)");
  faultNames(shapeFault,
             {"preset", "lists no key"},
             "the shape check finishes before the node rules (FR-007)");
  check(shapeFault.find("twice") == std::string::npos,
        "the first fault prints alone: the node rule prints nothing (SC-003)");

  // The same file with the key taken out breaks one node rule alone, and the
  // line names the repeated name: the shape check passed and the node rules
  // ran (FR-007, FR-010 rule 4).
  sg::detail::installTestGraph(std::nullopt);
  const std::string nodeOnly = plant("node-only.json", kRepeatedName);
  const auto nodeFirst =
      sg::detail::readGraphFile(nodeOnly, std::span(kRunOfTwo));
  const std::string nodeFault =
      refusalOf(nodeFirst, "a file that breaks one node rule stands refused");
  faultNames(
      nodeFault,
      {"a", "twice"},
      "the node rules run after the shape check (FR-007, FR-010 rule 4)");
}

// ---------------------------------------------------------------------------
// The node rules of FR-010 rules 4 to 8
// ---------------------------------------------------------------------------

// FR-010 rule 4: two nodes carry one name, and the line names the repeated
// name.
auto repeatedNameRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("repeated-name.json", kRepeatedName);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
  const std::string fault =
      refusalOf(answer, "a repeated node name stands refused (FR-010 rule 4)");
  faultNames(
      fault, {"a", "twice"}, "the line names the repeated name (FR-008)");
}

// FR-010 rule 5: one node carries no slot. The schema file names `slot` as
// a key every node carries, so the shape check settles this fault ahead of
// the node rules (FR-007), and the line names the node by its position in
// the file and the key that stands missing.
auto missingSlotRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("no-key.json", kNoSlot);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
  const std::string fault =
      refusalOf(answer, "a node with no slot stands refused (FR-010 rule 5)");
  faultNames(fault,
             {"/nodes/1", "slot", "missing"},
             "the line names the node and the key it carries no slot under "
             "(FR-010 rule 5, FR-008)");
}

// FR-010 rule 6: one slot stands outside the run's context count, and the
// line names the node and the slot against that count.
auto slotOutOfRangeRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("slot-range.json", kSlotOutside);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
  const std::string fault = refusalOf(answer,
                                      "a slot outside the run's context count "
                                      "stands refused " "(FR-010 rule 6)");
  faultNames(fault,
             {"b", "5", "2"},
             "the line names the node and the slot against the run's context "
             "count (FR-010 rule 6)");
}

// FR-010 rule 6: one slot stands below zero. The schema file sets `slot` no
// bound, so the shape check passes the node and the run's own count reads the
// slot: the line names the node, the slot it names and the run's context
// count, the same line the slot above the count answers (FR-007, FR-010 rule
// 6).
auto negativeSlotRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("slot-negative.json", kNegativeSlot);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
  const std::string fault =
      refusalOf(answer, "a slot below zero stands refused (FR-010 rule 6)");
  faultNames(fault,
             {"a", "-1", "2"},
             "the line names the node, the slot below zero and the run's "
             "context count (FR-010 rule 6)");
}

// FR-010 rule 7: one dependency names no node, and the line names the edge.
auto edgeNamesNoNodeRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("edge-no-node.json", kEdgeNoNode);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfOne));
  const std::string fault = refusalOf(
      answer, "an edge naming no node stands refused (FR-010 rule 7)");
  faultNames(fault,
             {"zz", "a"},
             "the line names the edge: the dependency and the node it stands "
             "on (FR-010 rule 7)");
}

// FR-010 rule 8: one node names itself as a dependency, and the line names
// the node.
auto selfEdgeRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("self-edge.json", kSelfEdge);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfOne));
  const std::string fault =
      refusalOf(answer, "a self-edge stands refused (FR-010 rule 8)");
  faultNames(fault,
             {"a"},
             "the line names the node that names itself (FR-010 " "rule 8)");
}

// ---------------------------------------------------------------------------
// The cycle, the order and the clean file
// ---------------------------------------------------------------------------

// FR-010 rule 9: two nodes name each other, and the line names the cycle.
auto cycleRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("two-nodes.json", kCycle);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
  const std::string fault =
      refusalOf(answer, "a cycle of two nodes stands refused (FR-010 rule 9)");
  faultNames(
      fault, {"a", "b", "cycle"}, "the line names the cycle (FR-010 rule 9)");
}

// FR-010 rule 10: two nodes name one slot, so the run starts no node for
// the slot none names, and the line names the node and the slot (D-2).
auto unsatisfiableDependencyRefuses() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("one-slot.json", kOneSlot);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfTwo));
  const std::string fault = refusalOf(
      answer,
      "a dependency the run cannot satisfy stands refused " "(FR-010 rule 10)");
  faultNames(fault,
             {"a", "b", "slot 0"},
             "the line names the node and the slot the run starts no node for "
             "(FR-010 rule 10, R-04)");
}

// SC-003: a file with several faults prints the first in the order FR-010
// lists, and that line stands alone. The plant carries a repeated name
// beside a cycle of two nodes: the repeated-name line prints, and the cycle
// prints nothing.
auto firstFaultPrintsAlone() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("two-faults.json", kRepeatedAndCycle);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfThree));
  const std::string fault =
      refusalOf(answer, "a file with several faults stands refused (SC-003)");
  faultNames(fault,
             {"a", "twice"},
             "the first fault in the FR-010 order prints (SC-003)");
  check(fault.find("cycle") == std::string::npos,
        "the first fault prints alone: the cycle behind it prints nothing "
        "(SC-003, FR-010)");
}

// FR-008: the clean chain file carries no key beside `nodes`, `name`,
// `slot` and `deps`, passes every rule, and reaches the gate. One node key
// beside the four stands refused by the schema file.
auto fileCarriesNoExtraKeys() -> void
{
  // The clean chain: the reader answers the graph, and the gate reads it.
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("chain.json", kChain);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfThree));
  check(answer.has_value(), "the clean chain file passes every rule (FR-008)");
  if (!answer.has_value()) {
    std::printf("  %s\n", answer.error().c_str());
    return;
  }

  const sg::detail::Graph& graph = *answer;
  check(graph.nodes.size() == 3,
        "the graph carries one node per entry of the file (FR-008)");
  check(graph.nodes[0].name == "a" && graph.nodes[1].name == "b"
            && graph.nodes[2].name == "c",
        "the three names stand in the file's order (FR-008)");
  check(graph.nodes[0].slot == 0 && graph.nodes[1].slot == 1
            && graph.nodes[2].slot == 2,
        "the three slots stand counted from 0 (FR-008)");
  check(graph.nodes[0].deps.empty()
            && graph.nodes[1].deps == (std::vector<std::string> {"a"})
            && graph.nodes[2].deps == (std::vector<std::string> {"b"}),
        "the dependency lists stand as the file spells them (FR-008)");
  check(
      graph.edges.size() == 2 && graph.edges[0].from == "a"
          && graph.edges[0].to == "b" && graph.edges[1].from == "b"
          && graph.edges[1].to == "c",
      "the edges carry one entry per dependency in the file's order (FR-014)");

  // The graph reaches the gate: the start order names every slot once in the
  // release order, and the slot map answers one node per slot (FR-013, R-04).
  check(sg::detail::startOrder(graph) == (std::vector<std::uint32_t> {0, 1, 2}),
        "the clean chain reaches the gate in its start order (FR-013)");
  check(sg::detail::slotMap(graph).has_value(),
        "the clean chain reaches the gate with one node per slot (R-04)");

  // One key beside the four, on a node: the schema file closes the node
  // object, and the line names the key (FR-008).
  sg::detail::installTestGraph(std::nullopt);
  const std::string cpuPath = plant("node-extra.json", kNodeCpu);
  const auto cpuAnswer =
      sg::detail::readGraphFile(cpuPath, std::span(kRunOfOne));
  const std::string cpuFault = refusalOf(
      cpuAnswer, "a node key beside the four stands refused (FR-008)");
  faultNames(
      cpuFault,
      {"cpu", "lists no key"},
      "the node object admits no key beside name, slot and deps (FR-008)");
}

// FR-008, FR-013: a file whose nodes stand in an order other than their
// slots passes every rule. The reader answers the graph in the file's order,
// and the release round of FR-013 that settles the cycle rule moves one node
// at a time over that order: the round behind the one that settled `a` meets
// `a` standing settled and leaves it out, so the rule reads the graph's
// dependencies and not the file's lines (FR-008, FR-013, R-06).
auto nodeOrderOtherThanSlots() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  const std::string path = plant("file-order.json", kFileOrderChain);
  const auto answer = sg::detail::readGraphFile(path, std::span(kRunOfThree));
  check(answer.has_value(),
        "a chain whose nodes stand in an order other than their slots passes "
        "every rule (FR-008)");
  if (!answer.has_value()) {
    std::printf("  %s\n", answer.error().c_str());
    return;
  }

  const sg::detail::Graph& graph = *answer;
  check(graph.nodes[0].name == "c" && graph.nodes[1].name == "b"
            && graph.nodes[2].name == "a",
        "the nodes stand in the file's order, not the release order (FR-008)");
  check(graph.nodes[0].slot == 2 && graph.nodes[1].slot == 1
            && graph.nodes[2].slot == 0,
        "each node carries the slot the file names (FR-008)");
  check(
      graph.edges.size() == 2 && graph.edges[0].from == "b"
          && graph.edges[0].to == "c" && graph.edges[1].from == "a"
          && graph.edges[1].to == "b",
      "the edges carry one entry per dependency in the file's order (FR-014)");

  // The graph reaches the gate in the order the dependencies fix: the file's
  // order names `c` first, and the gate releases it last (FR-013, R-04).
  check(sg::detail::startOrder(graph) == (std::vector<std::uint32_t> {0, 1, 2}),
        "the start order follows the dependencies, not the file's order "
        "(FR-013)");
  check(sg::detail::slotMap(graph).has_value(),
        "the file-order chain reaches the gate with one node per slot (R-04)");
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from. A scenario absent here never runs.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"unreadableFileRefuses", unreadableFileRefuses},
      Scenario {"parserRejectRefuses", parserRejectRefuses},
      Scenario {"schemaShapeBreakRefuses", schemaShapeBreakRefuses},
      Scenario {"readerChecksShapeAgainstSchemaFile",
                readerChecksShapeAgainstSchemaFile},
      Scenario {"repeatedNameRefuses", repeatedNameRefuses},
      Scenario {"missingSlotRefuses", missingSlotRefuses},
      Scenario {"slotOutOfRangeRefuses", slotOutOfRangeRefuses},
      Scenario {"negativeSlotRefuses", negativeSlotRefuses},
      Scenario {"edgeNamesNoNodeRefuses", edgeNamesNoNodeRefuses},
      Scenario {"selfEdgeRefuses", selfEdgeRefuses},
      Scenario {"cycleRefuses", cycleRefuses},
      Scenario {"unsatisfiableDependencyRefuses",
                unsatisfiableDependencyRefuses},
      Scenario {"firstFaultPrintsAlone", firstFaultPrintsAlone},
      Scenario {"fileCarriesNoExtraKeys", fileCarriesNoExtraKeys},
      Scenario {"nodeOrderOtherThanSlots", nodeOrderOtherThanSlots},
  };
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : scenarios()) {
      if (scenario.name == mode) {
        scenario.body();
        std::filesystem::remove_all(plantDirectory());
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : scenarios()) {
    scenario.body();
  }
  std::filesystem::remove_all(plantDirectory());
  std::printf("harness_graph_file_test: ok\n");
  return 0;
}
