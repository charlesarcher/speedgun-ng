// ============================================================================
// specs/037-execution-graphs T017: the start gate of one threaded attempt
// (US1, US4; FR-013 to FR-016, FR-019 to FR-021, FR-026; R-06, R-09, R-16;
// D-3, D-4, D-10 of research.md).
//
// One gate stands per threaded attempt, owned by `threadedSampleRun` beside
// the two barrier rounds (D-3). The gate reads no clock, opens no measured
// window, closes none, shifts none, and moves no barrier round: it gates the
// start of a context and touches nothing inside it (FR-016).
//
// One mutex carries the ready counts and the settled words, and one condition
// variable carries the wake (D-10). Every predicate re-reads the state, so a
// lost wake and a spurious wake both re-read: the notify is a hint and the
// state is the fact. Every state change another context's predicate reads
// stands beside a notify: the settle of a word, the drop of a ready count,
// and the release of the lowest ready slot. No sleep, no clock, no timeout
// (FR-026).
//
// The release fact of FR-013 is the completion state of specs/020 FR-049. The
// finishing context settles its word under the mutex before its item returns,
// and the context loop writes the completion state with release order as its
// last access to the node; the attempt's join reads that state with acquire
// order, so every write the dependency's work did stands ordered before the
// gated body (D-3, D-10).
//
// The order is the rule FR-013 fixes: among the nodes ready to start, the
// lowest slot goes first. A failed node settles the transitive closure of its
// dependents held, and the stop request settles every gated node stopped with
// its dependents (FR-019, FR-020).
//
// The state type stands private to `GraphGate` in the frozen header of T002,
// so the rules below read the state vector through one template parameter,
// and every call site deduces it inside the member.
// ============================================================================

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

#include "detail/graph.hpp"

#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{
namespace
{

// The ready count of a node the gate has released: the node stands started,
// so its count names no dependency any more. A gated node carries the count
// of the dependencies named on it, a number inside the node count of the
// graph, so this value names no count.
constexpr std::uint32_t kReleased = std::numeric_limits<std::uint32_t>::max();

// The slot of the node of `graph` that names `name`.
[[nodiscard]] auto slotNamed(const Graph& graph,
                             const std::string_view name) -> std::uint32_t
{
  auto slot = std::uint32_t {0};
  [[maybe_unused]] auto found = false;
  for (const auto& node : graph.nodes) {
    if (node.name == name) {
      slot = node.slot;
      found = true;
    }
  }
  SG_REQUIRE(found,
             "every dependency names a node of the graph (FR-010 rule 7)");
  return slot;
}

// A node of `graph` names `name`. The one caller stands inside a contract,
// so the contracts-ignored build carries the helper unread.
[[nodiscard]] [[maybe_unused]] auto namesNode(
    const Graph& graph, const std::string_view name) -> bool
{
  auto found = false;
  for (const auto& node : graph.nodes) {
    if (node.name == name) {
      found = true;
    }
  }
  return found;
}

// The node at `slot` stands ready: every dependency has finished, and no word
// has settled it.
template<class States>
[[nodiscard]] auto readyAt(const States& states,
                           const std::uint32_t slot) -> bool
{
  return !states[slot].settled.has_value() && states[slot].remaining == 0;
}

// Fan-out: the finished node at `slot` drops one dependency from the ready
// count of every node that depends on it alone (FR-014).
template<class States>
auto dropDependency(const Graph& graph,
                    States& states,
                    const std::uint32_t slot) -> void
{
  for (const auto& node : graph.nodes) {
    auto& state = states[node.slot];
    if (state.remaining == 0 || state.remaining == kReleased) {
      // A node with no dependency left, and a node the gate has released,
      // carry no count this finish drops.
      continue;
    }
    for (const auto& dependency : node.deps) {
      if (slotNamed(graph, dependency) == slot) {
        --state.remaining;
        break;
      }
    }
  }
}

// The transitive closure of the dependents of `slot`: every node that stands
// on it, directly or through another node, and stands gated, settles `word`
// (FR-019, FR-020).
template<class States>
auto settleClosure(const Graph& graph,
                   States& states,
                   const std::uint32_t slot,
                   const NodeOutcome word) -> void
{
  auto frontier = std::vector<std::uint32_t> {slot};
  while (!frontier.empty()) {
    auto next = std::vector<std::uint32_t> {};
    for (const auto& node : graph.nodes) {
      auto& state = states[node.slot];
      if (state.settled.has_value()) {
        // A node that stands settled stands outside the closure.
        continue;
      }
      for (const auto& dependency : node.deps) {
        const auto depSlot = slotNamed(graph, dependency);
        if (std::find(frontier.begin(), frontier.end(), depSlot)
            != frontier.end())
        {
          state.settled = word;
          next.push_back(node.slot);
          break;
        }
      }
    }
    frontier = next;
  }
}

// The stop request's arrival: every node that stands gated settles stopped,
// and its dependents settle stopped with it (FR-019, specs/020 FR-050).
template<class States>
auto settleStoppedAll(const Graph& graph, States& states) -> void
{
  for (std::size_t index = 0; index < states.size(); ++index) {
    const auto slot = static_cast<std::uint32_t>(index);
    auto& state = states[slot];
    if (state.settled.has_value() || state.remaining == kReleased) {
      // A settled node, and a node the gate has released and whose body
      // settles its own word, stand outside the stop request.
      continue;
    }
    state.settled = NodeOutcome::STOPPED;
    settleClosure(graph, states, slot, NodeOutcome::STOPPED);
  }
}

// The gate's decision for the node at `slot`, re-read on every wake: the word
// a gater settled for a node that never starts, the count that releases the
// lowest ready slot, and the count that keeps the node waiting (FR-013,
// FR-014, FR-019, FR-020).
//
// The word of a node that never starts stands settled by a closure, so this
// predicate reads no dependency word: `nodeFailed` settles the failed word and
// `settleClosure` the held word of every transitive dependent, and the stop
// request settles its own closure, each under the gate's mutex and through
// the same `slotNamed` map this predicate reads. The invariant every closure
// leaves is that no unsettled node names a dependency settled failed, held or
// stopped, so a read of a dependency's word here answers false on every wake;
// the arm that read it stood uncovered for that reason, and the coverage gate
// pinned it (T045).
template<class States>
auto decide(States& states,
            const std::uint32_t slot,
            std::condition_variable& wake) -> bool
{
  auto& state = states[slot];
  if (state.settled.has_value()) {
    // A gater, or the stop request, settled this node before it started: the
    // context arrives at its stop round with no body run (FR-015).
    return true;
  }
  if (state.remaining != 0) {
    return false;
  }
  // The lowest ready slot first: a ready node starts when no ready node
  // stands at a lower slot (FR-013).
  for (std::uint32_t lower = 0; lower < slot; ++lower) {
    if (readyAt(states, lower)) {
      return false;
    }
  }
  // The release is a state change the next ready slot reads: the node waiting
  // behind this one re-reads the order on this wake (FR-013, D-10).
  state.remaining = kReleased;
  wake.notify_all();
  return true;
}

}  // namespace

GraphGate::GraphGate(const Graph& graph)
    : m_graph(graph)
    , m_states(graph.nodes.size())
{
  // The count no node has named yet: the loop below fills one count per slot,
  // and a slot the loop visits twice names two nodes (FR-010 rule 10).
  for (auto& state : m_states) {
    state.remaining = kReleased;
  }
  for (const auto& node : graph.nodes) {
    SG_REQUIRE(node.slot < m_states.size(),
               "one node names a slot inside the graph's node count (FR-010 "
               "rule 6)");
    SG_REQUIRE(m_states[node.slot].remaining == kReleased,
               "one slot names one node of the graph (FR-010 rule 10)");
    m_states[node.slot].remaining =
        static_cast<std::uint32_t>(node.deps.size());
  }
  for (const auto& node : graph.nodes) {
    for ([[maybe_unused]] const auto& dependency : node.deps) {
      SG_REQUIRE(namesNode(graph, dependency),
                 "every dependency names a node of the graph (FR-010 rule 7)");
    }
  }
  // The cycle rule of FR-010 rule 9: `startOrder` answers a ready node for
  // every step of a graph that holds no cycle, and its own assertion names the
  // graph that holds one.
  SG_ASSERT(startOrder(graph).size() == m_states.size(),
            "the graph the gate stands over holds no cycle (FR-010 rule 9)");
  for ([[maybe_unused]] const auto& node : graph.nodes) {
    SG_ENSURE(m_states[node.slot].remaining
                  == static_cast<std::uint32_t>(node.deps.size()),
              "every node of the graph leaves the count of the dependencies "
              "named on it (FR-013)");
  }
}

auto GraphGate::awaitStart(const std::uint32_t slot) -> StartAnswer
{
  SG_REQUIRE(slot < m_states.size(),
             "the slot names a node of the graph (FR-013)");
  auto answer = StartAnswer::RUN;
  {
    auto lock = std::unique_lock(m_mutex);
    m_condition.wait(lock,
                     [this, slot]() -> bool
                     { return decide(m_states, slot, m_condition); });
    // The word the gate settled for this node answers the decision: a node
    // that starts carries no word, and a node that never starts carries held
    // or stopped (FR-019).
    answer = m_states[slot].settled.has_value() ? StartAnswer::SKIP
                                                : StartAnswer::RUN;
    SG_ENSURE((answer == StartAnswer::RUN) == !m_states[slot].settled.has_value(),
              "the gate releases a node it has not settled, and settles a "
              "node it has released to no start (FR-019)");
  }
  return answer;
}

auto GraphGate::nodeFinished(const std::uint32_t slot) -> void
{
  SG_REQUIRE(slot < m_states.size(),
             "the slot names a node of the graph (FR-019)");
  {
    const auto lock = std::lock_guard(m_mutex);
    SG_REQUIRE(m_states[slot].remaining == kReleased,
               "the node stands started (FR-019)");
    SG_REQUIRE(!m_states[slot].settled.has_value(),
               "the node stands unsettled (FR-019)");
    m_states[slot].settled = NodeOutcome::FINISHED;
    dropDependency(m_graph, m_states, slot);
  }
  // The wake stands outside the mutex: the finishing context settles its word,
  // notifies, and returns from its item, and the context loop's release store
  // of the completion state follows (D-10).
  m_condition.notify_all();
}

auto GraphGate::nodeFailed(const std::uint32_t slot) -> void
{
  SG_REQUIRE(slot < m_states.size(),
             "the slot names a node of the graph (FR-019)");
  {
    const auto lock = std::lock_guard(m_mutex);
    SG_REQUIRE(m_states[slot].remaining == kReleased,
               "the node stands started (FR-019)");
    SG_REQUIRE(!m_states[slot].settled.has_value(),
               "the node stands unsettled (FR-019)");
    m_states[slot].settled = NodeOutcome::FAILED;
    // The failed node's dependents never start: the transitive closure of its
    // dependents settles held with it (FR-020).
    settleClosure(m_graph, m_states, slot, NodeOutcome::HELD);
  }
  m_condition.notify_all();
}

auto GraphGate::settleStopped() -> void
{
  {
    const auto lock = std::lock_guard(m_mutex);
    settleStoppedAll(m_graph, m_states);
  }
  m_condition.notify_all();
}

[[nodiscard]] auto GraphGate::outcomes()
    -> std::vector<std::optional<NodeOutcome>>
{
  const auto lock = std::lock_guard(m_mutex);
  auto words = std::vector<std::optional<NodeOutcome>> {};
  words.reserve(m_states.size());
  for (const auto& state : m_states) {
    words.push_back(state.settled);
  }
  SG_ENSURE(words.size() == m_graph.nodes.size(),
            "the gate answers one word per node of the graph (FR-019)");
  return words;
}

}  // namespace sg::detail
