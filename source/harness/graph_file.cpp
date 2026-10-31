// ============================================================================
// specs/037-execution-graphs T009: the graph file reader (US3; FR-007,
// FR-009, FR-010, FR-011, FR-012, FR-029; R-03, D-1, D-2 of research.md).
//
// One fixed pipeline reads the file the operator named: read the text,
// parse with the vendored simdjson, check the document against
// docs/schema/graph-v1.json through the standing checkReport, then the node
// rules in the order FR-010 lists, the first fault alone (D-1). simdjson
// stands the only JSON reader the change uses (FR-029), and the shape check
// finishes before the node rules (FR-007).
//
// The answer carries the graph, or the one line the entry point prints at
// status 2 before any context starts (FR-010, SC-003). The reader installs
// nothing into the graph seam: the entry point reads once and installs what
// it read (FR-011, FR-025), so a refusal leaves the slot the runner reads
// untouched and no context starts.
//
// The ten faults and the line each prints stand in the table of
// contracts/graph-file.md. Rule 5, a node with no slot, stands settled by
// the schema file: `slot` is a key every node carries, so the shape check
// prints that fault ahead of the node rules (FR-007).
//
// Two assumptions stand recorded under Principle X.1:
//   - The schema path. The build writes no path for docs/, and no module of
//     the plan owns the root CMakeLists.txt line that would write one, so
//     the reader anchors the schema file to this source file: the compiler
//     names it by an absolute path, and the repository root stands three
//     levels above source/harness/graph_file.cpp.
//   - The run-relative rules. FR-010 rules 6 and 10 read the run's context
//     count. Rule 10 is the slot map of R-04, whose precondition is a node
//     count equal to the context count (FR-012). A run whose count stands
//     other than the node count answers FR-012's own refusal, naming both
//     counts, at the entry point, so the reader states no rule 10 fault for
//     that run.
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/graph.hpp"

#include <simdjson.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

// FR-007: the schema file the shape check reads. The build writes no path
// for docs/, so the file anchors to this source file, whose absolute path
// the compiler names: three levels above it stands the repository root, and
// the schema stands under docs/schema.
[[nodiscard]] auto graphSchemaPath() -> const std::string&
{
  static const std::string kPath =
      (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
       / "docs/schema/graph-v1.json")
          .string();
  return kPath;
}

// The one line of a node-rule fault: the file, then the fact. The table of
// contracts/graph-file.md names what each line carries.
[[nodiscard]] auto nodeFault(std::string_view path,
                             const std::string& fact) -> std::string
{
  return "the graph file '" + quotedText(std::string(path)) + "': " + fact;
}

// One node as the file spells it: the model's node, and the slot as the
// file counts it, so a slot below zero names its own number in the line
// FR-010 rule 6 prints.
struct NodeRead
{
  GraphNode node;
  std::int64_t slot = 0;
};

// The nodes of the document, in the file's order. The shape check of FR-007
// settled the key set, the key order and the value kinds, so each read
// below carries a contract in place of a branch: the extraction runs in
// every configuration, and the fuse answers a document the shape check
// passed by accident (II, X.2).
[[nodiscard]] auto nodesOf(const simdjson::dom::element& root)
    -> std::vector<NodeRead>
{
  auto reads = std::vector<NodeRead> {};
  simdjson::dom::array entries;
  [[maybe_unused]] const auto arrayCode =
      root["nodes"].get_array().get(entries);
  SG_ASSERT(arrayCode == simdjson::SUCCESS,
            "the shape check settled 'nodes' as an array (FR-007)");

  for (const auto entry : entries) {
    auto read = NodeRead {};
    std::string_view name;
    simdjson::dom::array deps;
    [[maybe_unused]] const auto nameCode = entry["name"].get_string().get(name);
    [[maybe_unused]] const auto slotCode =
        entry["slot"].get_int64().get(read.slot);
    [[maybe_unused]] const auto depsCode = entry["deps"].get_array().get(deps);
    SG_ASSERT(nameCode == simdjson::SUCCESS && slotCode == simdjson::SUCCESS
                  && depsCode == simdjson::SUCCESS,
              "the shape check settled name, slot and deps (FR-007)");

    read.node.name = std::string(name);
    read.node.slot = static_cast<std::uint32_t>(read.slot);
    for (const auto dependency : deps) {
      std::string_view text;
      [[maybe_unused]] const auto textCode = dependency.get_string().get(text);
      SG_ASSERT(
          textCode == simdjson::SUCCESS,
          "the shape check settled every dependency as a string (FR-007)");
      read.node.deps.emplace_back(text);
    }
    reads.push_back(std::move(read));
  }
  return reads;
}

// The node `name` names, and nothing where the file names no such node.
[[nodiscard]] auto nodeNamed(const std::vector<GraphNode>& nodes,
                             const std::string& name) -> const GraphNode*
{
  const auto found = std::ranges::find(nodes, name, &GraphNode::name);
  return found == nodes.end() ? nullptr : &*found;
}

// FR-010 rule 4: the first name two nodes carry.
[[nodiscard]] auto repeatedName(const std::vector<GraphNode>& nodes)
    -> std::optional<std::string>
{
  for (std::size_t first = 0; first < nodes.size(); ++first) {
    for (std::size_t second = first + 1; second < nodes.size(); ++second) {
      if (nodes[first].name == nodes[second].name) {
        return nodes[first].name;
      }
    }
  }
  return std::nullopt;
}

// FR-010 rule 6: the first node whose slot stands outside the run's
// `count` contexts.
[[nodiscard]] auto slotOutside(const std::vector<NodeRead>& reads,
                               const std::uint32_t count)
    -> std::optional<NodeRead>
{
  for (const auto& read : reads) {
    if (read.slot < 0 || static_cast<std::uint64_t>(read.slot) >= count) {
      return read;
    }
  }
  return std::nullopt;
}

// FR-010 rule 7: the first dependency that names no node, as the node it
// stands on and the dependency itself, the two ends of the edge.
[[nodiscard]] auto edgeWithoutNode(const std::vector<GraphNode>& nodes)
    -> std::optional<std::pair<std::string, std::string>>
{
  for (const auto& node : nodes) {
    for (const auto& dependency : node.deps) {
      if (nodeNamed(nodes, dependency) == nullptr) {
        return std::pair {node.name, dependency};
      }
    }
  }
  return std::nullopt;
}

// FR-010 rule 8: the first node that names itself as a dependency.
[[nodiscard]] auto selfEdge(const std::vector<GraphNode>& nodes)
    -> std::optional<std::string>
{
  for (const auto& node : nodes) {
    if (std::ranges::find(node.deps, node.name) != node.deps.end()) {
      return node.name;
    }
  }
  return std::nullopt;
}

// `name` stands among the names settled so far.
[[nodiscard]] auto settledHolds(const std::vector<std::string>& settled,
                                const std::string& name) -> bool
{
  return std::ranges::find(settled, name) != settled.end();
}

// One release round of FR-013: every node whose every dependency stands
// settled joins `settled`, and the answer says whether the round moved one.
auto settleRound(const std::vector<GraphNode>& nodes,
                 std::vector<std::string>& settled) -> bool
{
  auto progressed = false;
  for (const auto& node : nodes) {
    if (settledHolds(settled, node.name)) {
      continue;
    }
    const auto ready =
        std::ranges::all_of(node.deps,
                            [&](const std::string& dependency)
                            { return settledHolds(settled, dependency); });
    if (ready) {
      settled.push_back(node.name);
      progressed = true;
    }
  }
  return progressed;
}

// FR-010 rule 9: the nodes no round settles, the cycle of two or more nodes
// the file carries. A graph of no cycle answers no name (R-06).
[[nodiscard]] auto cycleNodes(const std::vector<GraphNode>& nodes)
    -> std::vector<std::string>
{
  auto settled = std::vector<std::string> {};
  while (settled.size() < nodes.size()) {
    if (!settleRound(nodes, settled)) {
      break;
    }
  }
  auto cycle = std::vector<std::string> {};
  for (const auto& node : nodes) {
    if (!settledHolds(settled, node.name)) {
      cycle.push_back(node.name);
    }
  }
  return cycle;
}

// The names of a cycle as one line spells them: each quoted, the list
// comma-separated.
[[nodiscard]] auto namesText(const std::vector<std::string>& names)
    -> std::string
{
  auto text = std::string {};
  for (const auto& name : names) {
    text += text.empty() ? "'" : ", '";
    text += quotedText(name);
    text += '\'';
  }
  return text;
}

// The edges of the file's order: one edge per dependency, from the node it
// names to the node that carries it (FR-014, FR-023).
[[nodiscard]] auto edgesOf(const std::vector<GraphNode>& nodes)
    -> std::vector<GraphEdge>
{
  auto edges = std::vector<GraphEdge> {};
  for (const auto& node : nodes) {
    for (const auto& dependency : node.deps) {
      edges.push_back(GraphEdge {.from = dependency, .to = node.name});
    }
  }
  return edges;
}

}  // namespace

/**
 * @brief specs/037 FR-007, FR-010, D-1: read one graph file and answer the
 * graph it names, or the one fault line.
 *
 * The pipeline stands in one order: the read, the simdjson parse, the shape
 * check against `docs/schema/graph-v1.json` through the standing
 * `checkReport`, then the node rules in the order FR-010 lists. The first
 * fault stands the answer alone, and the reader stops there (FR-007,
 * FR-010).
 *
 * @pre `path` names the file the operator gave, and every count of
 *      `contextCounts` names the context count of one threaded run of the
 *      batch, in batch order (FR-009, FR-012, R-05)
 * @post the answer carries the graph: one node per entry of the file in the
 *       file's order, and one edge per dependency in the file's order
 *       (FR-008, FR-014)
 * @post the answer carries one fault line naming what the table of
 *       contracts/graph-file.md names for that fault, and the reader
 *       installs nothing into the graph seam, so the refused run reads no
 *       graph and starts no context (FR-010, FR-011, SC-003)
 */
auto readGraphFile(std::string_view path,
                   std::span<const std::uint32_t> contextCounts)
    -> std::expected<Graph, std::string>
{
  SG_REQUIRE(!path.empty(),
             "readGraphFile reaches a path that names a file (FR-010 rule 1)");

  // FR-010 rule 1: the read. The standing reader's line names the path and
  // the reason the system refused.
  const auto text = readTextFile(std::string(path));
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }

  // FR-010 rule 2: the parse, with the vendored reader (FR-007, FR-029).
  auto parser = simdjson::dom::parser();
  auto parsed = parser.parse(simdjson::padded_string(std::string_view(*text)));
  simdjson::dom::element root;
  if (const auto code = parsed.get(root); code != simdjson::SUCCESS) {
    return std::unexpected("the graph file '" + quotedText(std::string(path))
                           + "' holds no JSON the parser reads: "
                           + simdjson::error_message(code));
  }

  // FR-010 rule 3: the shape, against the schema file through the standing
  // checker: the key set, the key order and the value kinds (FR-007).
  if (const auto shape =
          checkReport(graphSchemaPath(), std::string_view(*text));
      !shape.has_value())
  {
    return std::unexpected(shape.error());
  }

  const auto reads = nodesOf(root);
  auto nodes = std::vector<GraphNode> {};
  nodes.reserve(reads.size());
  for (const auto& read : reads) {
    nodes.push_back(read.node);
  }

  // FR-010 rule 4: one name carried twice.
  if (const auto repeated = repeatedName(nodes); repeated.has_value()) {
    return std::unexpected(nodeFault(
        path, "the node name '" + quotedText(*repeated) + "' stands twice"));
  }

  // FR-010 rule 6: one slot outside the run's context count, per threaded
  // run in batch order.
  for (const auto count : contextCounts) {
    if (const auto outside = slotOutside(reads, count); outside.has_value()) {
      return std::unexpected(nodeFault(
          path,
          "the node '" + quotedText(outside->node.name) + "' names slot "
              + std::to_string(outside->slot) + " outside the run's "
              + std::to_string(count) + " contexts"));
    }
  }

  // FR-010 rule 7: an edge naming no node.
  if (const auto edge = edgeWithoutNode(nodes); edge.has_value()) {
    return std::unexpected(
        nodeFault(path,
                  "the dependency '" + quotedText(edge->second) + "' of node '"
                      + quotedText(edge->first) + "' names no node"));
  }

  // FR-010 rule 8: a node naming itself.
  if (const auto self = selfEdge(nodes); self.has_value()) {
    return std::unexpected(nodeFault(
        path,
        "the node '" + quotedText(*self) + "' names itself as a dependency"));
  }

  // FR-010 rule 9: a cycle of two or more nodes.
  if (const auto cycle = cycleNodes(nodes); !cycle.empty()) {
    return std::unexpected(nodeFault(path, namesText(cycle) + " form a cycle"));
  }

  auto edges = edgesOf(nodes);
  auto graph = Graph {.nodes = std::move(nodes), .edges = std::move(edges)};

  // FR-010 rule 10: the slot map, per threaded run in batch order: a slot
  // two nodes name leaves a slot the run starts no node for (R-04, D-2).
  for (const auto count : contextCounts) {
    if (count == graph.nodes.size()) {
      const auto map = slotMap(graph);
      if (!map.has_value()) {
        return std::unexpected(nodeFault(path, map.error()));
      }
    }
  }

  auto dependencies = std::size_t {0};
  for (const auto& node : graph.nodes) {
    dependencies += node.deps.size();
  }
  SG_ENSURE(graph.nodes.size() == reads.size()
                && graph.edges.size() == dependencies,
            "the graph carries one node per entry of the file and one edge "
            "per dependency (FR-008, FR-014)");
  return graph;
}

}  // namespace sg::detail
