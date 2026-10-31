// ============================================================================
// The graph model: the node, the four outcome words, the start order and
// the slot-to-node map (specs/037-execution-graphs T001; US1; FR-008,
// FR-013, FR-019, FR-020; research R-04, R-06).
//
// The model is data and pure rules, so every scenario here stands without
// a run, without a provider, without a thread and without a clock: the
// node carries the three keys the file names (FR-008), the words are the
// four the record prints (FR-019), the start order follows the release
// rule of FR-013, and the slot map is the bijection R-04 states. The live
// order of a running thread stands in harness_graph_gate_test.cpp (T010);
// this file states the same rule over the model alone.
//
// The declarations stand in source/harness/detail/graph.hpp (T002) and
// the rules in source/harness/graph.cpp (T004):
//   detail::GraphNode        the name, the slot counted from 0, the deps
//   detail::Graph            the nodes, the edges in file order, the slot
//                            map
//   detail::GraphEdge        one dependency pair, `from` and `to`
//   detail::NodeOutcome      finished, failed, held, stopped
//   detail::nodeOutcomeName  the word the record prints for one outcome
//   detail::startOrder       the slots in the order the gate releases
//                            them, the lowest ready slot first
//   detail::slotMap          the bijection from the run's context indexes
//                            to the nodes, naming the node and the slot
//                            when two nodes name one slot
//
// The seam stands in source/harness/detail/internal.hpp and is defined in
// source/harness/graph.cpp (T004):
//   detail::installTestGraph a synthetic graph the run reads in place of
//                            the file, an empty install restoring the read
//   detail::graphInput()     the slot the seam and the entry point's read
//                            share, consulted at each threaded attempt
//
// Scenario map:
//   nodeNamesSlotAndDeps   one node states its name, its slot counted
//                          from 0 and its dependency list, and the edge
//                          list carries one edge per dependency in the
//                          file's order (FR-008, FR-014)
//   fourOutcomeWords       the four words finished, failed, held and
//                          stopped stand in the model, spell the words
//                          the record prints, and a node settles exactly
//                          one of them (FR-019, FR-020)
//   topologicalOrderRespectsSlots
//                          a chain starts in its dependency order, and
//                          two disconnected components start with the
//                          lower slot first (FR-013, R-06)
//   slotMapBijection       the slot map answers one node per slot, and
//                          two nodes naming one slot answer the fault
//                          naming the node and the slot (FR-008, R-04)
//   seamInstallsAndClears  an install stands in the slot the run reads,
//                          and an empty install restores the file read,
//                          the clear-on-null shape of `installTestLayout`
//                          at source/harness/topology.cpp:336 (FR-011,
//                          FR-025, R-12, D-7)
//   graphInputAnswersInstall
//                          the slot answers the installed graph when one
//                          stands, and the graph the entry point read once
//                          when none stands (FR-011, FR-025)
//
// `argv[1]` selects one scenario; with no argument the list at the bottom
// runs, and that list is what CTest runs, so a scenario absent from it
// never runs. Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "detail/graph.hpp"
#include "detail/internal.hpp"

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS GRAPH MODEL TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// ---------------------------------------------------------------------------
// The graphs the scenarios read
// ---------------------------------------------------------------------------

// The chain of quickstart.md: `alpha -> beta -> gamma` over the slots 0,
// 1 and 2, the file naming the nodes in the order of their slots
// (FR-008).
[[nodiscard]] auto chainGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "alpha", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "beta", .slot = 1, .deps = {"alpha"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "gamma", .slot = 2, .deps = {"beta"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "alpha", .to = "beta"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "beta", .to = "gamma"});
  return graph;
}

// Two disconnected components, the file naming the higher slots first:
// `alpha -> beta` over the slots 2 and 3 and `gamma -> delta` over the
// slots 0 and 1. The model stores no order, so the release rule of FR-013
// answers the start order (FR-013, R-06).
[[nodiscard]] auto pairGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "alpha", .slot = 2, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "beta", .slot = 3, .deps = {"alpha"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "gamma", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "delta", .slot = 1, .deps = {"gamma"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "alpha", .to = "beta"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "gamma", .to = "delta"});
  return graph;
}

// Two nodes name one slot: `alpha` at 0, `beta` at 1 and `gamma` at 1, so
// the three contexts of the run leave slot 2 unnamed (FR-010 rule 10,
// R-04).
[[nodiscard]] auto repeatedSlotGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "alpha", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "beta", .slot = 1, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "gamma", .slot = 1, .deps = {}});
  return graph;
}

// The position one slot holds in an answered order, and `kNoPosition`
// where the order names that slot never.
constexpr std::size_t kNoPosition = std::numeric_limits<std::size_t>::max();

auto orderPosition(const std::vector<std::uint32_t>& order,
                   const std::uint32_t slot) -> std::size_t
{
  const auto found = std::find(order.begin(), order.end(), slot);
  return found == order.end() ? kNoPosition
                              : static_cast<std::size_t>(found - order.begin());
}

// The order names every slot of the graph once, and every node stands
// after every node it depends on (FR-013).
auto orderRespectsDeps(const sg::detail::Graph& graph,
                       const std::vector<std::uint32_t>& order) -> bool
{
  if (order.size() != graph.nodes.size()) {
    return false;
  }
  for (const auto& node : graph.nodes) {
    const auto position = orderPosition(order, node.slot);
    if (position == kNoPosition) {
      return false;
    }
    for (const auto& dependency : node.deps) {
      const auto named = std::ranges::find(
          graph.nodes, dependency, &sg::detail::GraphNode::name);
      if (named == graph.nodes.end()
          || orderPosition(order, named->slot) >= position)
      {
        return false;
      }
    }
  }
  return true;
}

// The slot's answer names the installed graph node for node: the name, the
// slot and the dependency list, the three keys one node carries. The model
// carries no equality of its own, so the test reads the keys (FR-008, FR-025).
auto nodesMatch(const std::vector<sg::detail::GraphNode>& answer,
                const std::vector<sg::detail::GraphNode>& installed) -> bool
{
  if (answer.size() != installed.size()) {
    return false;
  }
  for (std::size_t index = 0; index < installed.size(); ++index) {
    const auto& node = installed[index];
    const auto& seen = answer[index];
    if (seen.name != node.name || seen.slot != node.slot
        || seen.deps != node.deps)
    {
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// The scenarios
// ---------------------------------------------------------------------------

// One node states the name the operator chooses, the context slot counted
// from 0, and the dependency list named by node name (FR-008). The edge
// list carries one edge per dependency in the file's order (FR-014).
auto nodeNamesSlotAndDeps() -> void
{
  const auto chain = chainGraph();
  check(chain.nodes.size() == 3, "the chain names three nodes (FR-008)");
  check(chain.nodes[0].name == "alpha",
        "one node names itself by the operator's name (FR-008)");
  check(chain.nodes[0].slot == 0,
        "the first context of the run names slot 0 (FR-008)");
  check(chain.nodes[0].deps.empty(),
        "a source node names an empty dependency list (FR-008)");
  const auto& middle = chain.nodes[1];
  check(middle.name == "beta",
        "the second node names itself by the operator's name (FR-008)");
  check(middle.slot == 1,
        "one node names the context slot counted from 0 (FR-008)");
  check(middle.deps == (std::vector<std::string> {"alpha"}),
        "one node names its dependency by node name (FR-008)");
  check(chain.nodes[2].deps == (std::vector<std::string> {"beta"}),
        "the last node names the node it depends on (FR-008)");

  check(chain.edges.size() == 2,
        "the edge list carries one edge per dependency (FR-014)");
  check(chain.edges[0].from == "alpha" && chain.edges[0].to == "beta",
        "the first edge is the first dependency the file names (FR-014)");
  check(chain.edges[1].from == "beta" && chain.edges[1].to == "gamma",
        "the second edge follows the file's order (FR-014)");
}

// The four words of FR-019 stand in the model, each spelling the word the
// record prints (FR-023). The words stand pairwise distinct, and the
// value range of the type holds the four and no more, so a node settles
// exactly one word (FR-019).
auto fourOutcomeWords() -> void
{
  constexpr std::array<sg::detail::NodeOutcome, 4> kWords {
      sg::detail::NodeOutcome::FINISHED,
      sg::detail::NodeOutcome::FAILED,
      sg::detail::NodeOutcome::HELD,
      sg::detail::NodeOutcome::STOPPED};
  constexpr std::array<std::string_view, 4> kSpellings {
      "finished", "failed", "held", "stopped"};

  for (std::size_t first = 0; first < kWords.size(); ++first) {
    check(sg::detail::nodeOutcomeName(kWords[first]) == kSpellings[first],
          "one word spells the word the record prints (FR-019, FR-023)");
    for (std::size_t second = first + 1; second < kWords.size(); ++second) {
      check(kWords[first] != kWords[second],
            "two words of the set stand distinct (FR-019)");
    }
  }

  for (int value = 0; value < static_cast<int>(kWords.size()); ++value) {
    const auto word = static_cast<sg::detail::NodeOutcome>(value);
    check(std::find(kWords.begin(), kWords.end(), word) != kWords.end(),
          "every value of the word type is one of the four words (FR-019)");
  }

  // The held word is the word of a node whose dependency settled failed
  // (FR-020), and the stopped word is the word of a node the stop request
  // refuses (FR-019): each stands as one word of the set, and the record
  // prints it for the one node it settles.
  check(sg::detail::nodeOutcomeName(sg::detail::NodeOutcome::HELD) == "held",
        "the held word is the word of a gated node (FR-020)");
  check(sg::detail::nodeOutcomeName(sg::detail::NodeOutcome::STOPPED)
            == "stopped",
        "the stopped word is the word of a refused node (FR-019)");
}

// The start order of a chain names each node after its dependency, and
// the start order of two disconnected components names the lower slot
// first: the file fixes no order, and the release rule of FR-013 answers
// the order the gate starts in (FR-013, R-06).
auto topologicalOrderRespectsSlots() -> void
{
  const auto chain = chainGraph();
  const auto chainOrder = sg::detail::startOrder(chain);
  check(chainOrder == (std::vector<std::uint32_t> {0, 1, 2}),
        "a chain starts in the order its dependencies run (FR-013)");
  check(orderRespectsDeps(chain, chainOrder),
        "each node of the chain starts after its dependency (FR-013)");

  const auto pair = pairGraph();
  const auto pairOrder = sg::detail::startOrder(pair);
  check(pairOrder == (std::vector<std::uint32_t> {0, 1, 2, 3}),
        "two disconnected components start lowest slot first (FR-013)");
  check(orderRespectsDeps(pair, pairOrder),
        "each node of the pair starts after its dependency (FR-013)");
  check(orderPosition(pairOrder, 2) > orderPosition(pairOrder, 0),
        "the component at the higher slots starts after the component at "
        "the lower slots (FR-013)");

  auto sortedOrder = pairOrder;
  std::ranges::sort(sortedOrder);
  check(sortedOrder == (std::vector<std::uint32_t> {0, 1, 2, 3}),
        "the start order names every slot of the run once (FR-013)");
}

// The slot map is the bijection from the run's context indexes to the
// nodes: it is indexed by slot and answers the node that names that slot,
// and it holds one entry per slot of the run (R-04). Two nodes naming one
// slot leave a slot no node names, and the map answers the FR-010 fault
// `a dependency the run cannot satisfy` with a line naming the node and
// the slot (FR-010 rule 10, R-04).
auto slotMapBijection() -> void
{
  const auto chain = chainGraph();
  const auto mapped = sg::detail::slotMap(chain);
  check(mapped.has_value(),
        "a graph of distinct slots answers a slot map (FR-008, R-04)");
  if (mapped.has_value()) {
    check(mapped->size() == chain.nodes.size(),
          "the map holds one entry for every context slot of the run "
          "(FR-012, R-04)");
    for (std::size_t slot = 0; slot < mapped->size(); ++slot) {
      check(chain.nodes[(*mapped)[slot]].slot == slot,
            "the map answers the node that names the slot (R-04)");
    }
    auto nodes = *mapped;
    std::ranges::sort(nodes);
    check(std::ranges::adjacent_find(nodes) == nodes.end(),
          "no node stands at two slots (R-04)");
  }

  const auto clash = repeatedSlotGraph();
  const auto refused = sg::detail::slotMap(clash);
  check(!refused.has_value(),
        "two nodes naming one slot answer no slot map (FR-010, R-04)");
  if (!refused.has_value()) {
    const auto& fault = refused.error();
    check(fault.find("beta") != std::string::npos ||
              fault.find("gamma") != std::string::npos,
          "the fault names a node of the pair that shares the slot "
          "(FR-010 rule 10)");
    check(fault.find("1") != std::string::npos,
          "the fault names the slot the two nodes share (FR-010 rule 10)");
  }
}

// The seam of FR-025 and D-7: `installTestGraph` installs a synthetic graph
// the run reads in place of the file, and an empty install restores the file
// read, the clear-on-null shape `installTestLayout` uses at
// `source/harness/topology.cpp:336`. The slot stands behind `graphInput()`,
// consulted at each threaded attempt, so an install decides the attempt that
// follows it (R-12).
auto seamInstallsAndClears() -> void
{
  sg::detail::installTestGraph(std::nullopt);
  check(!sg::detail::graphInput().has_value(),
        "no install stands, so the slot answers the file read (FR-025, D-7)");

  sg::detail::installTestGraph(chainGraph());
  check(sg::detail::graphInput().has_value(),
        "an install stands in the slot the run reads (FR-025, R-12)");
  if (sg::detail::graphInput().has_value()) {
    check(sg::detail::graphInput()->nodes.size() == 3,
          "the installed graph reaches the reader whole (FR-025, R-12)");
    check(sg::detail::graphInput()->nodes[0].name == "alpha",
          "the installed graph names its own nodes (FR-025, R-12)");
    check(sg::detail::graphInput()->edges.size() == 2,
          "the installed graph carries the edges in the file's order (FR-014)");
  }

  sg::detail::installTestGraph(std::nullopt);
  check(!sg::detail::graphInput().has_value(),
        "an empty install restores the file read, the clear-on-null shape "
        "(FR-025, D-7)");
}

// The slot answers the installed graph when one stands, and the graph the
// entry point read once when none stands: the entry point's read of FR-011
// installs into the same slot the tests inject into, so the runner reads one
// source and the parse-once rule stands at one call site (FR-011, FR-025,
// D-7, R-12).
auto graphInputAnswersInstall() -> void
{
  const auto chain = chainGraph();
  const auto pair = pairGraph();

  sg::detail::installTestGraph(chain);
  check(sg::detail::graphInput().has_value(),
        "one install stands in the slot (FR-011, FR-025)");
  check(nodesMatch(sg::detail::graphInput()->nodes, chain.nodes),
        "the slot answers the installed graph (D-7, R-12)");

  // The entry point's read rides the same slot: the graph it read once,
  // ahead of the first context, answers every attempt that follows (FR-011).
  sg::detail::installTestGraph(pair);
  check(nodesMatch(sg::detail::graphInput()->nodes, pair.nodes),
        "the slot answers the graph installed last (FR-011, R-12)");
  check(sg::detail::startOrder(*sg::detail::graphInput())
            == (std::vector<std::uint32_t> {0, 1, 2, 3}),
        "the answer carries the rules of the graph it names (FR-013)");

  sg::detail::installTestGraph(std::nullopt);
  check(!sg::detail::graphInput().has_value(),
        "with no install standing the slot answers the graph the entry point "
        "read once, and a process that read no file answers none (FR-011, "
        "FR-025)");
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
      Scenario {"nodeNamesSlotAndDeps", nodeNamesSlotAndDeps},
      Scenario {"fourOutcomeWords", fourOutcomeWords},
      Scenario {"topologicalOrderRespectsSlots", topologicalOrderRespectsSlots},
      Scenario {"slotMapBijection", slotMapBijection},
      Scenario {"seamInstallsAndClears", seamInstallsAndClears},
      Scenario {"graphInputAnswersInstall", graphInputAnswersInstall},
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
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : scenarios()) {
    scenario.body();
  }
  std::printf("harness_graph_model_test: ok\n");
  return 0;
}
