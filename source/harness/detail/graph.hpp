#ifndef SG_HARNESS_DETAIL_GRAPH_HPP
#define SG_HARNESS_DETAIL_GRAPH_HPP

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file graph.hpp
 * @brief specs/037-execution-graphs: the execution graph model, the four
 * outcome words, the start order, the slot-to-node map and the start gate
 * type. Nothing here is public API (D-8).
 *
 * The model is data and pure rules. It stores the nodes, the edges in the
 * file's order and nothing else: the record's ascending slot order of
 * FR-023 and the start order of FR-013 are read from the nodes, so no
 * order field stands here (R-04). The gate type stands declared here and
 * defined by module M4 in `source/harness/graph_gate.cpp` (D-3, T002,
 * T017).
 */

namespace sg::detail
{

/**
 * @brief specs/037 data-model.md: one node of the graph (FR-008, R-04).
 *
 * \pre `name` stands unique across the nodes of the graph, `slot` stands
 *      inside the run's context count, and every entry of `deps` names a
 *      node of the graph (FR-010 rules 4, 6, 7)
 * \post `name` names the node the operator chose, `slot` names its context
 *       index counted from `0`, and `deps` names the nodes it depends on
 *       (FR-008)
 * \invariant the three fields carry the node for its whole life: the model
 *            derives no field from another and stores no order (R-04)
 */
struct GraphNode
{
  std::string name;
  std::uint32_t slot = 0;
  std::vector<std::string> deps;
};

/**
 * @brief specs/037 data-model.md: one dependency pair (FR-014, R-04).
 *
 * \pre `from` and `to` each name a node of the graph, and the pair names
 *      no self-edge (FR-010 rules 7, 8)
 * \post a finished `from` releases `to` alone (FR-014)
 * \invariant the edge list carries one edge per dependency in the file's
 *            order, the order the record prints (FR-023)
 */
struct GraphEdge
{
  std::string from;
  std::string to;
};

/**
 * @brief specs/037 data-model.md: the graph one run reads (FR-008, R-04).
 *
 * \pre the nodes carry unique names, the slots stand inside the run's
 *      context count, every dependency names a node, and the graph holds
 *      no cycle (FR-010 rules 4, 6, 7, 8, 9)
 * \post `nodes` carries one entry per node of the file and `edges` carries
 *       one entry per dependency in the file's order (FR-008, FR-014)
 * \invariant the model stores no start order and no settled word: the
 *            start order is read by `startOrder` and the words live in the
 *            gate (R-04, R-06)
 */
struct Graph
{
  std::vector<GraphNode> nodes;
  std::vector<GraphEdge> edges;
};

/**
 * @brief specs/037 FR-019, D-5: the four words one node settles (R-04).
 *
 * \invariant each node settles exactly one word of the set, the value
 *            range holds the four and no more, and no word returns
 *            (FR-019)
 */
enum class NodeOutcome : std::uint8_t
{
  FINISHED,
  FAILED,
  HELD,
  STOPPED
};

/**
 * @brief specs/037 FR-019, FR-023: the word one outcome prints.
 *
 * \pre `outcome` stands one of the four words of `NodeOutcome` (FR-019)
 * \post the answer spells `finished`, `failed`, `held` or `stopped`, and
 *       the four words answer four distinct spellings (FR-019, FR-023)
 */
[[nodiscard]] auto nodeOutcomeName(NodeOutcome outcome) noexcept
    -> std::string_view;

/**
 * @brief specs/037-execution-graphs FR-019, FR-020, FR-023, D-5 (T018): the
 * settlement one node of a threaded graph attempt leaves the record: the
 * node's name, its slot, the word it settled, and the failed node a held
 * node names as its reason.
 *
 * The graph stands outside the record: its nodes and edges are read from
 * the graph slot at print and write time, so one settlement carries the
 * facts the slot cannot answer (FR-023). The runner fills one entry per
 * node at the merged record, and the entry point reads the words of its
 * own run through `Runner::takeGraphSettlement` (T023).
 *
 * \pre `name` names a node of the run's graph, `slot` names its context
 *      index, and `outcome` stands one of the four words of `NodeOutcome`
 *      (FR-019)
 * \post `reason` names the failed node that gates a held node, and stands
 *       empty for every other word (FR-020, FR-023)
 * \invariant the four fields carry one node's settlement for the whole of
 *            the record's life, and one node settles one word (FR-019)
 */
struct GraphSettlement
{
  std::string name;
  std::uint32_t slot = 0;
  NodeOutcome outcome = NodeOutcome::FINISHED;
  std::string reason;
};

/**
 * @brief specs/037 FR-013, R-06: the slots in the order the gate releases
 * them, the lowest ready slot first.
 *
 * The rule reads the model alone: a node with no dependency stands ready
 * at the standing start, and among the ready nodes the lowest slot stands
 * first. The live order of a running thread stands in
 * `test/source/harness_graph_gate_test.cpp` (T010).
 *
 * \pre the nodes of `graph` carry unique names, every dependency names a
 *      node, and the graph holds no cycle (FR-010 rules 4, 7, 9)
 * \post the answer carries one slot per node of `graph`, each node standing
 *       after every node it depends on, and among the nodes ready at each
 *       step the lowest slot standing first (FR-013, FR-014, R-06)
 */
[[nodiscard]] auto startOrder(const Graph& graph) -> std::vector<std::uint32_t>;

/**
 * @brief specs/037 R-04: the bijection from the run's context indexes to
 * the nodes, indexed by slot and answering the node's index.
 *
 * \pre every slot of `graph` stands inside `graph.nodes.size()` (FR-010
 *      rule 6)
 * \post the answer carries one entry per slot of the run and answers at
 *       slot `s` the index of the node naming `s`, when the slots stand
 *       distinct (FR-008, FR-012, R-04)
 * \post a slot two nodes name answers the fault of FR-010 rule 10, naming
 *       the two nodes and the slot they share (R-04)
 */
[[nodiscard]] auto slotMap(const Graph& graph)
    -> std::expected<std::vector<std::size_t>, std::string>;

/**
 * @brief specs/037 R-06, D-4: the gate's answer at a context's start.
 *
 * \invariant a node answers RUN once, when every dependency has finished,
 *            and a node that never starts answers SKIP (FR-013, FR-015)
 */
enum class StartAnswer : std::uint8_t
{
  RUN,
  SKIP
};

/**
 * @brief specs/037 D-3, R-06: the start gate of one threaded attempt.
 *
 * One gate stands per threaded attempt, owned by `threadedSampleRun`
 * beside the two barriers (D-3). The gate reads no clock, opens no
 * measured window and moves no barrier round: it gates the start of a
 * context and touches nothing inside it (FR-016). Module M4 defines every
 * member in `source/harness/graph_gate.cpp` (T017).
 *
 * \invariant the ready counts and the settled words stand behind one
 *            mutex, and one condition variable carries the wake; every
 *            predicate re-reads the state, so a lost wake and a spurious
 *            wake both re-read (D-10, FR-026)
 * \invariant no node leaves its word, no word returns, and the gate stores
 *            no order of its own (FR-019, R-06)
 */
class GraphGate
{
public:
  /**
   * @brief Build the gate over `graph`: one ready count per node.
   *
   * \pre `graph` outlives this gate, its nodes carry unique names, its
   *      slots stand distinct and inside its node count, every dependency
   *      names a node, and it holds no cycle (FR-010 rules 4, 6, 7, 9, 10)
   * \post every node with no dependency stands ready and every other node
   *       carries the count of the dependencies named on it (FR-013)
   */
  explicit GraphGate(const Graph& graph);

  /**
   * @brief Wait at the body's start for the node at `slot`, and answer
   * the gate's decision.
   *
   * \pre `slot` names a node of the graph, and the calling context stands
   *      at the start round with every body index dispatched (FR-013)
   * \post RUN answers when every dependency of the node has finished, and
   *       the caller runs its callable (FR-013, FR-014)
   * \post SKIP answers when a dependency settled FAILED or HELD, and the
   *       node settles HELD; SKIP answers when the stop request refused
   *       the node, and the node settles STOPPED (FR-019, FR-020)
   */
  [[nodiscard]] auto awaitStart(std::uint32_t slot) -> StartAnswer;

  /**
   * @brief Settle the node at `slot` FINISHED and release its dependents.
   *
   * \pre `slot` names a node that stands started and unsettled (FR-019)
   * \post the node settles FINISHED, and every node that depends on it
   *       alone drops one dependency from its ready count (FR-014)
   */
  auto nodeFinished(std::uint32_t slot) -> void;

  /**
   * @brief Settle the node at `slot` FAILED and hold its dependents.
   *
   * \pre `slot` names a node that stands started and unsettled (FR-019)
   * \post the node settles FAILED, and the transitive closure of its
   *       dependents settles HELD (FR-020)
   */
  auto nodeFailed(std::uint32_t slot) -> void;

  /**
   * @brief Settle every gated node STOPPED, the stop request's arrival.
   *
   * \pre the stop request stands (FR-019, specs/020 FR-050)
   * \post every node that stands gated settles STOPPED, and its
   *       dependents settle STOPPED with it (FR-019)
   */
  auto settleStopped() -> void;

  /**
   * @brief The word each node of the graph settled, indexed by slot.
   *
   * \pre the attempt has joined, so every node has settled (FR-019)
   * \post the answer carries one entry per slot of the graph, naming the
   *       word that node settled (FR-019, FR-023)
   */
  [[nodiscard]] auto outcomes() -> std::vector<std::optional<NodeOutcome>>;

private:
  /// @brief specs/037 data-model.md: one node's gate state (D-10).
  struct NodeState
  {
    std::uint32_t remaining = 0;
    std::optional<NodeOutcome> settled;
  };

  const Graph& m_graph;
  std::vector<NodeState> m_states;
  std::mutex m_mutex;
  std::condition_variable m_condition;
};

}  // namespace sg::detail

#endif
