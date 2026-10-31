// ============================================================================
// specs/037-execution-graphs T002, T004: the graph model's two pure rules
// and the graph seam (US1; FR-008, FR-011, FR-013, FR-014, FR-019, FR-020,
// FR-025, FR-029; R-04, R-06, R-12, D-7).
//
// The model is data and pure rules, so this unit reads no file, starts no
// thread, opens no measured window and reads no clock (FR-026). The start
// order is the release rule of FR-013 read off the model alone: a node
// stands ready when every node it depends on stands in the order already,
// and among the ready nodes the lowest slot goes first (R-06). The slot map
// is the bijection from the run's context indexes to the nodes, and a slot
// two nodes name answers the FR-010 rule 10 fault, naming the node and the
// slot (R-04).
//
// The seam is one slot (D-7): the tests' install and the entry point's read
// of FR-011 share it, so the runner reads one source and the parse-once rule
// stands at one call site (FR-011, FR-025, R-12). The four word spellings
// are the words the record prints, and the four stand pairwise distinct
// (FR-019, FR-023). The gate type stands defined by module M4 in
// `source/harness/graph_gate.cpp` (T017).
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/graph.hpp"

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

// The slot map entry that names no node yet.
constexpr std::size_t kUnmapped = std::numeric_limits<std::size_t>::max();

// FR-011, FR-025, D-7, R-12: the one slot the tests' install and the entry
// point's read share. A function-local static, read at each threaded
// attempt, so an install decides the attempt that follows it.
auto installedGraphSlot() -> std::optional<Graph>&
{
  static std::optional<Graph> slot;
  return slot;
}

// Every dependency of `node` stands in `order` already, the order read so
// far. A source node names no dependency and stands ready at once (FR-013).
auto dependenciesReady(const Graph& graph,
                       const std::vector<std::uint32_t>& order,
                       const GraphNode& node) -> bool
{
  for (const auto& dependency : node.deps) {
    const auto named =
        std::ranges::find(graph.nodes, dependency, &GraphNode::name);
    SG_ASSERT(named != graph.nodes.end(),
              "every dependency names a node of the graph (FR-010 rule 7)");
    if (std::ranges::find(order, named->slot) == order.end()) {
      return false;
    }
  }
  return true;
}

}  // namespace

auto nodeOutcomeName(const NodeOutcome outcome) noexcept -> std::string_view
{
  constexpr std::array<std::string_view, 4> kSpellings {
      "finished", "failed", "held", "stopped"};
  const auto word = static_cast<std::size_t>(outcome);
  SG_REQUIRE(word < kSpellings.size(),
             "one word stands inside the set of the four (FR-019)");
  return kSpellings[word];
}

auto startOrder(const Graph& graph) -> std::vector<std::uint32_t>
{
  auto order = std::vector<std::uint32_t> {};
  order.reserve(graph.nodes.size());

  while (order.size() < graph.nodes.size()) {
    // The lowest slot among the nodes whose every dependency stands in the
    // order already: the release rule FR-013 fixes, read off the model.
    auto next = std::optional<const GraphNode*> {};
    for (const auto& node : graph.nodes) {
      if (std::ranges::find(order, node.slot) != order.end()
          || !dependenciesReady(graph, order, node))
      {
        continue;
      }
      if (!next || node.slot < (*next)->slot) {
        next = &node;
      }
    }
    SG_ASSERT(
        next.has_value(),
        "a graph of no cycle always answers a ready node (FR-010 rule 9)");
    order.push_back((*next)->slot);
  }

  SG_ENSURE(order.size() == graph.nodes.size(),
            "the order names every slot of the graph once (FR-013)");
  return order;
}

auto slotMap(const Graph& graph)
    -> std::expected<std::vector<std::size_t>, std::string>
{
  auto map = std::vector<std::size_t>(graph.nodes.size(), kUnmapped);
  for (std::size_t index = 0; index < graph.nodes.size(); ++index) {
    const auto slot = graph.nodes[index].slot;
    SG_REQUIRE(slot < map.size(),
               "one node names a slot inside the run's context count "
               "(FR-010 rule 6)");
    if (map[slot] != kUnmapped) {
      return std::unexpected("node " + graph.nodes[index].name + " and node "
                             + graph.nodes[map[slot]].name + " both name slot "
                             + std::to_string(slot));
    }
    map[slot] = index;
  }
  SG_ENSURE(map.size() == graph.nodes.size(),
            "the map holds one entry for every context slot of the run "
            "(FR-012, R-04)");
  return map;
}

auto installTestGraph(std::optional<Graph> graph) -> void
{
  installedGraphSlot() = std::move(graph);
}

auto graphInput() -> const std::optional<Graph>&
{
  return installedGraphSlot();
}

}  // namespace sg::detail
