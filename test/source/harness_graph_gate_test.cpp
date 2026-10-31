// ============================================================================
// specs/037-execution-graphs module M4 `start-gate`, wave 2: the gate that
// orders the starts of one threaded run between the two barrier rounds
// (T010 to T018; US1, US4; FR-013 to FR-021, FR-025 to FR-027; R-06 to R-09,
// R-16; D-3, D-4, D-5, D-10 of research.md).
//
// The graphs stand installed through `installTestGraph` and the cpus through
// `installTestOnlineCpus`, so no scenario reads a file, no scenario sleeps,
// no scenario reads a clock, and no scenario carries a fixed node count
// (FR-025, FR-026, FR-027). The gate's release fact is the completion state
// of specs/020 FR-049, and the release order is the rule FR-013 fixes: the
// lowest ready slot first (R-06).
//
// The gate type stands declared in source/harness/detail/graph.hpp (T002) and
// defined in source/harness/graph_gate.cpp (T017):
//   detail::GraphGate::awaitStart    the body's start, answering run or skip
//   detail::GraphGate::nodeFinished  the body's settle, releasing dependents
//   detail::GraphGate::nodeFailed    the body's error, holding dependents
//   detail::GraphGate::settleStopped the stop request's arrival
//   detail::GraphGate::outcomes      the word each slot settled
//
// Scenario map:
//   chainStartsInOrder       a chain over three contexts starts in the graph's
//                            order, and the gate saw each dependency finished
//                            before it released that node (FR-013, SC-001)
//   independentStartLowestSlotFirst
//                            two nodes with no dependency between them both
//                            start, and the gate holds the higher slot while
//                            the lower stands ready (FR-013, the third
//                            Clarifications answer)
//   fanOutReleasesAll        one finished source releases both dependents
//                            (FR-014)
//   fanInStartsAfterLast     a node with two dependencies starts after the
//                            last of them (FR-014)
//   diamondStartsAfterBoth   the diamond starts its last node after both of
//                            its dependencies (FR-013, FR-014, SC-002)
//   heldContextArrivesBothRounds
//                            the context of a node that never starts arrives
//                            at the start round and the stop round with no
//                            body run and no measured window open (FR-015)
//   gateOpensNoWindow        no window opens, closes or shifts because of the
//                            gate (FR-016)
//   placementBytesUnchangedBesideGraph
//                            the placement, pin, bind, wait mode, plan and
//                            recorder bytes stand unchanged beside a graph
//                            (FR-017)
//   noexceptItemConceptHolds the work items stay noexcept and the gate adds
//                            no error state to a work item (FR-018)
//   measuredRowSettlesAllFinished
//                            a measured row settles every node finished
//                            (FR-019)
//   skippedRowSettlesFinished
//                            a skipped row settles every node finished and
//                            starts every dependent (FR-019)
//   bodyErrorSettlesNodeFailed
//                            the node of the failing context settles failed
//                            and the others finished (FR-019)
//   setupFailureSettlesSlotZero
//                            a fixture setup failure settles the node at slot
//                            0 (FR-019)
//   failedDependentsHoldWithReason
//                            a failed node holds the closure of its
//                            dependents and names each with its gater
//                            (FR-020)
//   twoFailuresNameTheirGater
//                            two failures name a gater for each held node
//                            (FR-020)
//   stopSettlesStoppedWithDependents
//                            the stop request settles the gated node stopped
//                            and its dependents with it (FR-019)
//   independentFinishAndStatusFollowsRunFailure
//                            the run finishes the node that stands on its own
//                            and answers the standing run-failure status
//                            (FR-021)
//   suitePassesOnTwoCpus     the chain and the diamond run over the two cpus
//                            the seam names, and the run of that shape
//                            reaches its measured row (FR-027, SC-008)
//
// `argv[1]` selects one scenario; with no argument the list at the bottom
// re-executes this binary once per scenario, and that list is what CTest runs.
// One process per scenario keeps the seam installs of the scenarios apart.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/graph.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace
{

[[noreturn]] auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "HARNESS GRAPH GATE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* const what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// ---------------------------------------------------------------------------
// The graphs the scenarios install
// ---------------------------------------------------------------------------

// The chain of quickstart.md: `a -> b -> c` over the slots 0, 1 and 2, the
// one shape that states the start order of FR-013 in full (FR-008).
[[nodiscard]] auto chainGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "c", .slot = 2, .deps = {"b"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "b"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "b", .to = "c"});
  return graph;
}

// Two nodes with no dependency between them, the shape the release rule of
// FR-013 answers: the file fixes no live order, and the gate releases the
// lowest ready slot first (FR-013, the third Clarifications answer).
[[nodiscard]] auto independentGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {}});
  return graph;
}

// One source, two dependents: the finished source releases every node that
// depends on it alone (FR-014).
[[nodiscard]] auto fanOutGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "c", .slot = 2, .deps = {"a"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "b"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "c"});
  return graph;
}

// Two sources, one dependent: the node starts after the last of its two
// dependencies finishes (FR-014).
[[nodiscard]] auto fanInGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "c", .slot = 2, .deps = {"a", "b"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "c"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "b", .to = "c"});
  return graph;
}

// The diamond of SC-002: `a -> b`, `a -> c`, `b -> d`, `c -> d` over the
// slots 0 to 3, the shape that drives the fan-in, the fan-out and the
// ascending-slot release of FR-013 among `b` and `c`.
[[nodiscard]] auto diamondGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "c", .slot = 2, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "d", .slot = 3, .deps = {"b", "c"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "b"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "c"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "b", .to = "d"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "c", .to = "d"});
  return graph;
}

// The slot the node named `name` carries.
[[nodiscard]] auto slotOf(const sg::detail::Graph& graph,
                          const std::string_view name) -> std::uint32_t
{
  for (const auto& node : graph.nodes) {
    if (node.name == name) {
      return node.slot;
    }
  }
  fail("a dependency names no node of the installed graph");
}

// ---------------------------------------------------------------------------
// The driven context
// ---------------------------------------------------------------------------

// The widest graph a scenario of this file drives. Every slot of a driven
// graph stands inside this bound, so the scenario's slot records need no
// allocation while the driver threads hold their addresses.
constexpr std::size_t kMaxSlots = 8;

// What one driven context leaves at the gate. The scenario thread stands at
// the gate between the arrival and the settle, and the scenario's own
// `release` word opens the settle, so the scenario scripts the finish order
// and no test sleeps (FR-026).
struct DrivenSlot
{
  std::atomic<int> entered {0};
  std::atomic<int> started {0};
  std::atomic<int> skipped {0};
  std::atomic<int> bodySkipped {0};
  std::atomic<int> depsFinished {0};
  std::atomic<int> settled {0};
  std::atomic<int> release {0};
};

using Slots = std::array<DrivenSlot, kMaxSlots>;

// The starts, in the order the drivers recorded them. The gate releases the
// lowest ready slot first, and a driver records its start after the gate's
// mutex opens, so two slots that stand ready together record in either order
// (FR-013, D-10).
struct DriveLog
{
  std::mutex mutex;
  std::vector<std::uint32_t> starts;
};

// The scenario's wait: stand on the driver thread's own word until it carries
// the wanted count. The wait ends on the fact the next step needs, so no test
// sleeps and no test reads a clock (FR-026).
auto awaitWord(const std::atomic<int>& word, const int wanted) -> void
{
  while (word.load(std::memory_order_acquire) < wanted) {
    std::this_thread::yield();
  }
}

// The body of one driven context: arrive at the gate, record the answer, and
// settle the node when the scenario opens the settle. `fails` settles FAILED
// in place of FINISHED, the shape of a body that recorded an error (FR-019).
// `skips` is the shape of a body that skips the row: the body returns with
// its node settled and its row skipped, so the node settles FINISHED and the
// dependents start (FR-019).
auto driveSlot(sg::detail::GraphGate& gate,
               DriveLog& log,
               Slots& slots,
               const sg::detail::Graph& graph,
               const std::uint32_t slot,
               const bool fails,
               const bool skips) -> void
{
  DrivenSlot& mine = slots[slot];
  mine.entered.store(1, std::memory_order_release);

  const auto answer = gate.awaitStart(slot);
  if (answer == sg::detail::StartAnswer::SKIP) {
    mine.skipped.store(1, std::memory_order_release);
    return;
  }

  // The release fact of FR-013: every dependency of this node stands
  // finished at the answer the gate gave.
  int ready = 1;
  for (const auto& dependency : graph.nodes[slot].deps) {
    const auto& depSlot = slots[slotOf(graph, dependency)];
    if (depSlot.settled.load(std::memory_order_acquire) == 0) {
      ready = 0;
    }
  }
  mine.depsFinished.store(ready, std::memory_order_release);

  {
    const std::lock_guard lock(log.mutex);
    log.starts.push_back(slot);
  }
  mine.started.store(1, std::memory_order_release);

  while (mine.release.load(std::memory_order_acquire) == 0) {
    std::this_thread::yield();
  }
  if (skips) {
    mine.bodySkipped.store(1, std::memory_order_release);
  }
  // The driver's own word stands before the gate's settle. The gate releases
  // a dependent from inside that settle, and the dependent reads this word,
  // so the store stands ordered before the release the gate gives (FR-013,
  // D-10).
  mine.settled.store(1, std::memory_order_release);
  if (fails) {
    gate.nodeFailed(slot);
  } else {
    gate.nodeFinished(slot);
  }
}

// One driver thread per slot of `graph`, the slots named in `failures`
// settling FAILED and the slots named in `skips` skipping their row. The
// scenario drives the gate from the main thread, so the threads stand joined
// by `joinDrivers` before the scenario reads the words.
[[nodiscard]] auto spawnDrivers(sg::detail::GraphGate& gate,
                                DriveLog& log,
                                Slots& slots,
                                const sg::detail::Graph& graph,
                                const std::vector<std::uint32_t>& failures,
                                const std::vector<std::uint32_t>& skips = {})
    -> std::vector<std::thread>
{
  check(graph.nodes.size() <= kMaxSlots,
        "the driven graph stands inside the scenario's slot records");
  auto threads = std::vector<std::thread> {};
  threads.reserve(graph.nodes.size());
  for (const auto& node : graph.nodes) {
    const bool fails = std::find(failures.begin(), failures.end(), node.slot)
        != failures.end();
    const bool skip =
        std::find(skips.begin(), skips.end(), node.slot) != skips.end();
    threads.emplace_back(&driveSlot,
                         std::ref(gate),
                         std::ref(log),
                         std::ref(slots),
                         std::cref(graph),
                         node.slot,
                         fails,
                         skip);
  }
  return threads;
}

auto joinDrivers(std::vector<std::thread>& threads) -> void
{
  for (auto& thread : threads) {
    thread.join();
  }
}

// The scenario's release of every slot in the order the model of FR-013
// answers, each slot opened once it stands started and settled once it stands
// settled: the finish order follows the graph's own shape.
auto releaseInOrder(Slots& slots, const sg::detail::Graph& graph) -> void
{
  for (const auto slot : sg::detail::startOrder(graph)) {
    awaitWord(slots[slot].started, 1);
    slots[slot].release.store(1, std::memory_order_release);
    awaitWord(slots[slot].settled, 1);
  }
}

// The start order a scenario can state: every node of the graph stands
// recorded once, and each node stands recorded after every node it depends
// on. The gate's release order is a fact inside the gate's mutex, and a
// driver records its start after that mutex opens, so two slots that stand
// ready together record in either order (FR-013, FR-014, D-10).
auto checkStartsFollowGraph(const DriveLog& log, const sg::detail::Graph& graph)
    -> void
{
  check(log.starts.size() == graph.nodes.size(),
        "the gate starts every node of the graph once (FR-013)");
  for (std::size_t position = 0; position < log.starts.size(); ++position) {
    const auto edge =
        log.starts.begin() + static_cast<std::ptrdiff_t>(position);
    const auto slot = log.starts[position];
    check(std::find(log.starts.begin(), edge, slot) == edge,
          "the gate records one start for one node (FR-013)");
    for (const auto& node : graph.nodes) {
      if (node.slot != slot) {
        continue;
      }
      for (const auto& dependency : node.deps) {
        const auto depSlot = slotOf(graph, dependency);
        check(std::find(log.starts.begin(), edge, depSlot) != edge,
              "a node stands recorded after every dependency it names "
              "(FR-013, FR-014)");
      }
    }
  }
}

// The word the gate settled for one slot.
[[nodiscard]] auto wordOf(
    const std::vector<std::optional<sg::detail::NodeOutcome>>& outcomes,
    const std::uint32_t slot) -> std::optional<sg::detail::NodeOutcome>
{
  check(slot < outcomes.size(), "the gate answers one word per slot (FR-019)");
  return outcomes[slot];
}

auto settlesAs(const std::optional<sg::detail::NodeOutcome> word,
               const sg::detail::NodeOutcome outcome) -> bool
{
  return word.has_value() && *word == outcome;
}

// ---------------------------------------------------------------------------
// The scenarios
// ---------------------------------------------------------------------------

// SC-001, FR-013: a chain of three nodes over three contexts starts in the
// graph's order. The gate released each node only after its dependency stood
// finished, and a chain that runs settles every node finished (FR-019).
auto chainStartsInOrder() -> void
{
  const auto graph = chainGraph();
  sg::detail::installTestGraph(graph);
  check(sg::detail::graphInput().has_value(),
        "the scenario installs the graph the gate reads (FR-025)");

  auto gate = sg::detail::GraphGate(*sg::detail::graphInput());
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});
  releaseInOrder(slots, graph);
  joinDrivers(threads);

  check(log.starts == (std::vector<std::uint32_t> {0, 1, 2}),
        "a chain starts in the graph's order (FR-013, SC-001)");
  check(slots[1].depsFinished.load(std::memory_order_acquire) == 1,
        "the gate saw the dependency finished before it released the node "
        "(FR-013)");
  check(slots[2].depsFinished.load(std::memory_order_acquire) == 1,
        "the last node of the chain starts after its dependency (FR-013)");
  check(slots[0].skipped.load() == 0 && slots[1].skipped.load() == 0
            && slots[2].skipped.load() == 0,
        "no node of a chain that runs stands skipped (FR-015)");

  const auto outcomes = gate.outcomes();
  check(outcomes.size() == graph.nodes.size(),
        "the gate answers one word per node of the run (FR-019)");
  check(
      settlesAs(wordOf(outcomes, 0), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 1), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 2), sg::detail::NodeOutcome::FINISHED),
      "a chain that runs settles every node finished (FR-019)");
}

// FR-013: two nodes with no dependency between them start with the lowest
// ready slot first. The file fixes no live order, and the release rule
// answers the order the gate starts in (SC-002, the third Clarifications
// answer).
auto independentStartLowestSlotFirst() -> void
{
  const auto graph = independentGraph();
  sg::detail::installTestGraph(graph);

  auto gate = sg::detail::GraphGate(*sg::detail::graphInput());
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});

  // Both contexts arrive at the gate before either settles: the gate holds
  // the higher slot while a ready node stands below it (FR-013).
  awaitWord(slots[0].entered, 1);
  awaitWord(slots[1].entered, 1);
  awaitWord(slots[0].started, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[1].started, 1);
  slots[1].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  awaitWord(slots[1].settled, 1);
  joinDrivers(threads);

  checkStartsFollowGraph(log, graph);
  const auto outcomes = gate.outcomes();
  check(
      settlesAs(wordOf(outcomes, 0), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 1), sg::detail::NodeOutcome::FINISHED),
      "two independent nodes each settle finished (FR-019)");
}

// FR-014: one finished source releases every node that depends on it alone.
// Both dependents start, and each stands recorded after the source (FR-013,
// FR-014).
auto fanOutReleasesAll() -> void
{
  const auto graph = fanOutGraph();
  sg::detail::installTestGraph(graph);

  auto gate = sg::detail::GraphGate(*sg::detail::graphInput());
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});
  releaseInOrder(slots, graph);
  joinDrivers(threads);

  check(slots[1].started.load(std::memory_order_acquire) == 1
            && slots[2].started.load(std::memory_order_acquire) == 1,
        "a finished source releases every node that depends on it alone "
        "(FR-014)");
  checkStartsFollowGraph(log, graph);
  check(log.starts.front() == 0,
        "the finished source stands recorded before both of its dependents "
        "(FR-013, FR-014)");
  const auto outcomes = gate.outcomes();
  check(
      settlesAs(wordOf(outcomes, 0), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 1), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 2), sg::detail::NodeOutcome::FINISHED),
      "the fan-out run settles every node finished (FR-019)");
}

// FR-014: a node with several dependencies starts after the last of them
// finishes. The first finish wakes the dependent, and its predicate reads
// false: the wake is a hint and the state is the fact (D-3, D-10).
auto fanInStartsAfterLast() -> void
{
  const auto graph = fanInGraph();
  sg::detail::installTestGraph(graph);

  auto gate = sg::detail::GraphGate(*sg::detail::graphInput());
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});

  awaitWord(slots[2].entered, 1);
  awaitWord(slots[0].started, 1);
  awaitWord(slots[1].started, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  check(slots[2].started.load(std::memory_order_acquire) == 0,
        "the dependent of two sources starts after the last of them (FR-014)");
  slots[1].release.store(1, std::memory_order_release);
  awaitWord(slots[2].started, 1);
  check(
      slots[2].depsFinished.load(std::memory_order_acquire) == 1,
      "both dependencies stood finished at the answer the gate gave (FR-013)");
  slots[2].release.store(1, std::memory_order_release);
  awaitWord(slots[2].settled, 1);
  joinDrivers(threads);

  checkStartsFollowGraph(log, graph);
  check(log.starts.back() == 2,
        "the dependent of the two sources stands recorded last (FR-013, "
        "FR-014)");
  const auto outcomes = gate.outcomes();
  check(
      settlesAs(wordOf(outcomes, 0), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 1), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 2), sg::detail::NodeOutcome::FINISHED),
      "the fan-in run settles every node finished (FR-019)");
}

// SC-002: the diamond starts `d` after both `b` and `c`. The scenario drives
// the gate predicate with a wake whose predicate reads false: `b` finishing
// wakes `d` while `c` stands unfinished, and `d` starts on the second finish.
// `b` and `c` stand ready together, and the gate releases the lower slot
// first (FR-013, FR-014, SC-002).
auto diamondStartsAfterBoth() -> void
{
  const auto graph = diamondGraph();
  sg::detail::installTestGraph(graph);

  auto gate = sg::detail::GraphGate(*sg::detail::graphInput());
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});

  awaitWord(slots[3].entered, 1);
  awaitWord(slots[0].started, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  // The source finished releases both of its dependents, lowest slot first.
  awaitWord(slots[1].started, 1);
  awaitWord(slots[2].started, 1);
  slots[1].release.store(1, std::memory_order_release);
  awaitWord(slots[1].settled, 1);
  check(slots[3].started.load(std::memory_order_acquire) == 0,
        "the diamond starts its last node after both of its dependencies "
        "(FR-014, SC-002)");
  slots[2].release.store(1, std::memory_order_release);
  awaitWord(slots[2].settled, 1);
  awaitWord(slots[3].started, 1);
  check(
      slots[3].depsFinished.load(std::memory_order_acquire) == 1,
      "both dependencies of the diamond stood finished at the answer (FR-013)");
  slots[3].release.store(1, std::memory_order_release);
  awaitWord(slots[3].settled, 1);
  joinDrivers(threads);

  checkStartsFollowGraph(log, graph);
  check(log.starts.front() == 0 && log.starts.back() == 3,
        "`a` stands recorded first and `d` stands recorded last (FR-013, "
        "SC-002)");
  const auto outcomes = gate.outcomes();
  check(
      settlesAs(wordOf(outcomes, 0), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 1), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 2), sg::detail::NodeOutcome::FINISHED)
          && settlesAs(wordOf(outcomes, 3), sg::detail::NodeOutcome::FINISHED),
      "the diamond run settles every node finished (FR-019)");
}

// ---------------------------------------------------------------------------
// The settlement the record reads
// ---------------------------------------------------------------------------

// One node's settled fact, the shape the record carries: the name, the slot,
// the word, and the reason a held node names (FR-019, FR-020, FR-023).
struct SettledNode
{
  std::string name;
  std::uint32_t slot = 0;
  sg::detail::NodeOutcome outcome = sg::detail::NodeOutcome::FINISHED;
  std::string reason;
};

using Words = std::vector<std::optional<sg::detail::NodeOutcome>>;

// The node of `graph` that names `name`.
[[nodiscard]] auto nodeNamed(const sg::detail::Graph& graph,
                             const std::string_view name)
    -> const sg::detail::GraphNode&
{
  const auto found = std::find_if(graph.nodes.begin(),
                                  graph.nodes.end(),
                                  [&](const sg::detail::GraphNode& node)
                                  { return node.name == name; });
  check(found != graph.nodes.end(),
        "a name the graph carries names a node of the graph");
  return *found;
}

// Every node that stands on the node named `name`, directly or through
// another node: the transitive closure of its dependents (FR-020).
[[nodiscard]] auto dependentsOf(const sg::detail::Graph& graph,
                                const std::string_view name)
    -> std::vector<std::string>
{
  auto closure = std::vector<std::string> {};
  auto frontier = std::vector<std::string> {std::string(name)};
  while (!frontier.empty()) {
    auto next = std::vector<std::string> {};
    for (const auto& edge : graph.edges) {
      const bool gates = std::find(frontier.begin(), frontier.end(), edge.from)
          != frontier.end();
      const bool known =
          std::find(closure.begin(), closure.end(), edge.to) != closure.end();
      if (gates && !known) {
        closure.push_back(edge.to);
        next.push_back(edge.to);
      }
    }
    frontier = next;
  }
  return closure;
}

// The failed node that gates a held node: the nearest one on the dependency
// chain, the lowest slot first among equals (FR-020, FR-023).
[[nodiscard]] auto reasonFor(const sg::detail::Graph& graph,
                             const std::vector<SettledNode>& settled,
                             const std::uint32_t slot) -> std::string
{
  auto seen = std::vector<std::string> {};
  auto frontier = graph.nodes[slot].deps;
  while (!frontier.empty()) {
    auto next = std::vector<std::string> {};
    const sg::detail::GraphNode* gater = nullptr;
    for (const auto& dependency : frontier) {
      const auto& node = nodeNamed(graph, dependency);
      if (settled[node.slot].outcome == sg::detail::NodeOutcome::FAILED) {
        if (gater == nullptr || node.slot < gater->slot) {
          gater = &node;
        }
        continue;
      }
      if (std::find(seen.begin(), seen.end(), dependency) == seen.end()) {
        seen.push_back(dependency);
        next.insert(next.end(), node.deps.begin(), node.deps.end());
      }
    }
    if (gater != nullptr) {
      return gater->name;
    }
    frontier = next;
  }
  return {};
}

// The words the record carries: the gate's words with the standing
// settlement rule read over them. A fixture setup failure names the node at
// slot 0 failed, the context that runs the setup half, and holds its
// dependents with that node as their reason (FR-019, FR-020).
[[nodiscard]] auto settleWords(const sg::detail::Graph& graph,
                               const Words& words,
                               const bool setupFailed)
    -> std::vector<SettledNode>
{
  auto settled = std::vector<SettledNode>(graph.nodes.size());
  for (const auto& node : graph.nodes) {
    const auto word = wordOf(words, node.slot);
    check(word.has_value(),
          "the joined attempt leaves one word for every node (FR-019)");
    settled[node.slot] = SettledNode {
        .name = node.name, .slot = node.slot, .outcome = *word, .reason = {}};
  }
  if (setupFailed) {
    // The setup half runs on context 0, the context of slot 0 (FR-019).
    settled[0].outcome = sg::detail::NodeOutcome::FAILED;
    for (const auto& name : dependentsOf(graph, graph.nodes[0].name)) {
      settled[slotOf(graph, name)].outcome = sg::detail::NodeOutcome::HELD;
    }
  }
  for (const auto& node : graph.nodes) {
    if (settled[node.slot].outcome == sg::detail::NodeOutcome::HELD) {
      settled[node.slot].reason = reasonFor(graph, settled, node.slot);
    }
  }
  return settled;
}

// The word one settled node carries, read at `slot`.
[[nodiscard]] auto wordAt(const std::vector<SettledNode>& settled,
                          const std::uint32_t slot) -> sg::detail::NodeOutcome
{
  check(slot < settled.size(), "one settled node stands per slot (FR-019)");
  return settled[slot].outcome;
}

// ---------------------------------------------------------------------------
// The settlement scenarios (US4)
// ---------------------------------------------------------------------------

// FR-019, SC-004: a measured row settles every node of its run finished, and
// a finished node names no reason.
auto measuredRowSettlesAllFinished() -> void
{
  const auto graph = chainGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});
  releaseInOrder(slots, graph);
  joinDrivers(threads);

  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(settled.size() == graph.nodes.size(),
        "one settled node stands per node of the run (FR-019)");
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FINISHED
            && wordAt(settled, 1) == sg::detail::NodeOutcome::FINISHED
            && wordAt(settled, 2) == sg::detail::NodeOutcome::FINISHED,
        "a measured row settles every node of its run finished (FR-019)");
  check(settled[0].reason.empty() && settled[1].reason.empty()
            && settled[2].reason.empty(),
        "a finished node names no reason (FR-023)");
}

// FR-019, US4 scenario 3: a skipped row settles every node of its run
// finished, and every dependent starts: the skip settles the node the way a
// measured row does, and the gate releases on the settle.
auto skippedRowSettlesFinished() -> void
{
  const auto graph = chainGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {}, {0});
  releaseInOrder(slots, graph);
  joinDrivers(threads);

  check(slots[0].bodySkipped.load(std::memory_order_acquire) == 1,
        "the scripted body skipped the row of its context (FR-019)");
  check(slots[1].started.load(std::memory_order_acquire) == 1
            && slots[2].started.load(std::memory_order_acquire) == 1,
        "a skipped row starts every dependent of its node (FR-019, FR-014)");
  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FINISHED
            && wordAt(settled, 1) == sg::detail::NodeOutcome::FINISHED
            && wordAt(settled, 2) == sg::detail::NodeOutcome::FINISHED,
        "a skipped row settles every node of its run finished (FR-019)");
}

// FR-019, SC-004: the node of the context whose body recorded an error
// settles failed, and the other nodes of the run settle finished. The reason
// field names a failed node, so a failed node carries none (FR-023).
auto bodyErrorSettlesNodeFailed() -> void
{
  const auto graph = independentGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {0});
  releaseInOrder(slots, graph);
  joinDrivers(threads);

  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FAILED,
        "the node of the context whose body recorded an error settles failed "
        "(FR-019)");
  check(wordAt(settled, 1) == sg::detail::NodeOutcome::FINISHED,
        "the other node of the run settles finished (FR-019)");
  check(settled[0].reason.empty() && settled[1].reason.empty(),
        "the reason names a failed node, so a failed node carries none "
        "(FR-023)");
}

// FR-019, SC-004: a fixture setup failure settles the node at slot 0, the
// context that runs the setup half. The setup half runs before the start
// round and a failed setup leaves every context out of the body, so no node
// starts, every context arrives at both rounds, and the dependents of the
// node at slot 0 hold with that node as their reason (FR-015, FR-020).
auto setupFailureSettlesSlotZero() -> void
{
  const auto graph = chainGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  // The setup half runs before the start round, so the stop of the attempt
  // stands ahead of every context's arrival at the gate: the stop settles
  // every gated node, no body runs, and each driver arrives at its stop
  // round.
  gate.settleStopped();
  auto threads = spawnDrivers(gate, log, slots, graph, {});
  joinDrivers(threads);

  check(slots[0].started.load(std::memory_order_acquire) == 0
            && slots[1].started.load(std::memory_order_acquire) == 0
            && slots[2].started.load(std::memory_order_acquire) == 0,
        "a failed setup leaves every context out of the body (FR-019)");
  check(slots[0].skipped.load(std::memory_order_acquire) == 1
            && slots[1].skipped.load(std::memory_order_acquire) == 1
            && slots[2].skipped.load(std::memory_order_acquire) == 1,
        "every context of the failed setup arrives at its stop round, so no "
        "round strands (FR-015)");
  const auto settled = settleWords(graph, gate.outcomes(), true);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FAILED,
        "the setup failure settles the node at slot 0 (FR-019, SC-004)");
  check(wordAt(settled, 1) == sg::detail::NodeOutcome::HELD
            && settled[1].reason == "a",
        "the dependent of the setup failure holds with that node as its "
        "reason (FR-020)");
  check(wordAt(settled, 2) == sg::detail::NodeOutcome::HELD
            && settled[2].reason == "a",
        "the transitive closure of the setup failure holds with it (FR-020)");
}

// ---------------------------------------------------------------------------
// The runs the scenarios drive
// ---------------------------------------------------------------------------

// The scripted leaves every measured row folds: one step of each leaf per
// sampling action, so the merged row of a threaded run is a fixed number the
// scenario reads (the script harness_threads_test.cpp stands behind its
// merged-row scenarios).
constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 800;

auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// The bodies of the run-level scenarios record the context that ran them, so
// a scenario states which contexts of a graph run ran a body and opened a
// measured window (FR-015, FR-016).
constexpr int kMaxContexts = 8;
std::atomic<int> bodyRuns[kMaxContexts] {};

auto noteBodyRun(sg::State& state) -> void
{
  const int index = state.threadIndex();
  check(index >= 0 && index < kMaxContexts,
        "the body names a context of the run (FR-015)");
  bodyRuns[static_cast<std::size_t>(index)].fetch_add(
      1, std::memory_order_release);
}

// The body of a clean graph run: the timed loop of its own context, the one
// shape that opens one measured window per context (FR-016).
auto bmGateClean(sg::State& state) -> void
{
  noteBodyRun(state);
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// The body that records an error on the context of slot 0: the shape that
// settles one node failed and holds its dependents (FR-019, FR-020).
auto bmGateFailing(sg::State& state) -> void
{
  noteBodyRun(state);
  if (state.threadIndex() == 0) {
    throw std::runtime_error("the scripted graph body failure");
  }
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// A chain over `contexts` slots: `n0 -> n1 -> ...`, the node count equal to
// the context count of the run (FR-012).
[[nodiscard]] auto chainOf(const std::size_t contexts) -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  for (std::size_t slot = 0; slot < contexts; ++slot) {
    auto deps = std::vector<std::string> {};
    if (slot > 0) {
      deps.push_back("n" + std::to_string(slot - 1));
    }
    graph.nodes.push_back(sg::detail::GraphNode {
        .name = "n" + std::to_string(slot),
        .slot = static_cast<std::uint32_t>(slot),
        .deps = deps,
    });
    if (slot > 0) {
      graph.edges.push_back(sg::detail::GraphEdge {
          .from = "n" + std::to_string(slot - 1),
          .to = "n" + std::to_string(slot),
      });
    }
  }
  return graph;
}

// A failing chain beside a node that stands on its own: `n0 -> n1` over the
// slots 0 and 1 and `n2` alone at slot 2 (FR-021).
[[nodiscard]] auto mixedGraph() -> sg::detail::Graph
{
  auto graph = chainOf(2);
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "n2", .slot = 2, .deps = {}});
  return graph;
}

// The chain over the shape the online-set seam names: the node count
// follows the cpus the seam installs, so the suite rule carries no fixed
// node count (FR-027).
[[nodiscard]] auto onlineChainGraph() -> sg::detail::Graph
{
  return chainOf(sg::detail::onlineCpuSet().size());
}

// One run the child drives: the graph the seam installs, the body the
// contexts run, the context count of the instance, and the name the report
// prints.
struct RunMode
{
  std::string_view name;
  std::string_view benchmark;
  int contexts = 0;
  sg::detail::Graph (*graph)();
  void (*body)(sg::State&);
  // The cpus the seam installs for the run; empty leaves the host read
  // (FR-027).
  std::vector<std::uint32_t> cpus;
};

[[nodiscard]] auto runModes() -> const std::vector<RunMode>&
{
  static const std::vector<RunMode> kList {
      RunMode {
          .name = "plain3",
          .benchmark = "bmGatePlain",
          .contexts = 3,
          .graph = nullptr,
          .body = &bmGateClean,
          .cpus = {},
      },
      RunMode {
          .name = "chain3",
          .benchmark = "bmGateChain",
          .contexts = 3,
          .graph = &chainGraph,
          .body = &bmGateClean,
          .cpus = {},
      },
      RunMode {
          .name = "chain3fail",
          .benchmark = "bmGateChainFail",
          .contexts = 3,
          .graph = &chainGraph,
          .body = &bmGateFailing,
          .cpus = {},
      },
      RunMode {
          .name = "mixed3fail",
          .benchmark = "bmGateMixedFail",
          .contexts = 3,
          .graph = &mixedGraph,
          .body = &bmGateFailing,
          .cpus = {},
      },
      RunMode {
          .name = "onlineChain",
          .benchmark = "bmGateOnlineChain",
          .contexts = 0,
          .graph = &onlineChainGraph,
          .body = &bmGateClean,
          .cpus = {0, 1},
      },
  };
  return kList;
}

// The child's side of one run: the seam installs the graph, the family
// registers with its context count, and the entry point runs the instance.
// The child prints the body runs of every context after the report, so the
// parent reads which contexts of the run ran a body (FR-015).
auto runModeChild(const std::string_view name) -> int
{
  for (const auto& mode : runModes()) {
    if (mode.name != name) {
      continue;
    }
    if (!mode.cpus.empty()) {
      sg::detail::installTestOnlineCpus(mode.cpus);
    }
    if (mode.graph != nullptr) {
      sg::detail::installTestGraph(mode.graph());
    } else {
      sg::detail::installTestGraph(std::nullopt);
    }
    const auto contexts = mode.contexts == 0
        ? static_cast<int>(sg::detail::onlineCpuSet().size())
        : mode.contexts;
    scriptProvider();
    const auto label = std::string(mode.benchmark);
    auto handle = sg::registerBenchmark(mode.body, label);
    handle.threads(contexts);

    const std::vector<std::string> arguments {
        "--filter", "^" + label, "--iterations=2"};
    auto argv = std::vector<char*> {};
    argv.push_back(const_cast<char*>("harness_graph_gate_test"));
    for (const auto& argument : arguments) {
      argv.push_back(const_cast<char*>(argument.c_str()));
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());

    auto runs = std::string {};
    for (int index = 0; index < contexts; ++index) {
      if (index > 0) {
        runs += ",";
      }
      runs += std::to_string(bodyRuns[static_cast<std::size_t>(index)].load(
          std::memory_order_acquire));
    }
    std::printf("gate-bodies=%s contexts=%d\n", runs.c_str(), contexts);
    std::fflush(stdout);
    return status;
  }
  fail("the run mode is not one of this file's list");
}

// ---------------------------------------------------------------------------
// The report the parent reads
// ---------------------------------------------------------------------------

// The exit code of a waited child: the code the child exited with, and the
// signal it died on where no code stands.
auto exitCodeOf(const int status) -> int
{
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return status;
}

// One child per process: the seam installs stand per process, so the parent
// re-executes this binary once per child and reads its status, the pattern
// harness_numa_placement_test.cpp gives its layout installs. The child's
// stdout stands redirected to `path` where the parent names one, so the
// parent reads the report of a run the entry point drove.
auto runChild(const std::string& self,
              const std::vector<std::string>& arguments,
              const std::string& path) -> int
{
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    if (!path.empty() && std::freopen(path.c_str(), "w", stdout) == nullptr) {
      std::exit(3);
    }
    auto argv = std::vector<char*> {const_cast<char*>(self.c_str())};
    for (const auto& argument : arguments) {
      argv.push_back(const_cast<char*>(argument.c_str()));
    }
    argv.push_back(nullptr);
    (void)execv(self.c_str(), argv.data());
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the child");
  }
  return exitCodeOf(status);
}

auto readReport(const std::string& path) -> std::string
{
  auto file = std::ifstream(path);
  if (!file) {
    std::fprintf(stderr, "no report at %s\n", path.c_str());
    fail("the child wrote no report");
  }
  auto buffer = std::stringstream {};
  buffer << file.rdbuf();
  return buffer.str();
}

// The row of one instance: the only line of the report that names the
// iteration count of its run.
auto rowOf(const std::string& report, const std::string& name) -> std::string
{
  auto stream = std::istringstream(report);
  auto line = std::string {};
  while (std::getline(stream, line)) {
    if (line.starts_with(name) && line.contains("iterations=")) {
      return line;
    }
  }
  return {};
}

// The first line of the report that names the instance: the measured row,
// the failed row, or the skip line.
auto namedLineOf(const std::string& report, const std::string& name)
    -> std::string
{
  auto stream = std::istringstream(report);
  auto line = std::string {};
  while (std::getline(stream, line)) {
    if (line.starts_with(name)) {
      return line;
    }
  }
  return {};
}

// The standing placement disclosure of a threaded run (specs/025, specs/035).
auto contextsLineOf(const std::string& report) -> std::string
{
  auto stream = std::istringstream(report);
  auto line = std::string {};
  while (std::getline(stream, line)) {
    if (line.starts_with("  contexts=")) {
      return line;
    }
  }
  return {};
}

auto bodiesOf(const std::string& report) -> std::string
{
  auto stream = std::istringstream(report);
  auto line = std::string {};
  while (std::getline(stream, line)) {
    if (line.starts_with("gate-bodies=")) {
      return line.substr(std::strlen("gate-bodies="),
                         line.find(' ', std::strlen("gate-bodies="))
                             - std::strlen("gate-bodies="));
    }
  }
  return {};
}

// One child's report: the child runs one mode, writes its stdout to a file
// the parent names, and the parent reads the bytes back with the child's exit
// code. The scenario's name stands in the file name, so two scenarios that
// drive the same mode read their own bytes.
auto runReportChild(const std::string& self,
                    const std::string& mode,
                    const std::string_view name,
                    int& status) -> std::string
{
  const auto path =
      "harness_graph_gate_" + mode + "-" + std::string(name) + ".txt";
  status = runChild(self, {"run", mode}, path);
  return readReport(path);
}

// The row without its first field, the instance name: the two runs name their
// benchmarks apart, and every measured field follows.
auto rowWithoutName(const std::string& row) -> std::string
{
  const auto space = row.find(' ');
  return space == std::string::npos ? std::string {} : row.substr(space);
}

// The row fields a run's measured window carries, with the plan's
// overhead-floor column dropped. That column is the plan's calibration of one
// sampling action: the median of 257 real-clock readings the plan takes on
// the attempt thread before any body runs, outside every measured window
// (specs/015 FR-025). One plan publishes one floor, which is why the standing
// suite folds it inside one process at `harness_capture_test.cpp:427`; across
// two processes the column is a fresh reading of the host, and a two-cpu host
// under three spinning contexts answers it apart. The fold below stands
// across two children, so the reading stands out of it and the window fields
// stand in.
auto windowFieldsOf(const std::string& row) -> std::string
{
  const auto floor = row.find("overhead floor (ns)=");
  if (floor == std::string::npos) {
    return row;
  }
  const auto next = row.find("endpoint samples=", floor);
  if (next == std::string::npos) {
    return row.substr(0, floor);
  }
  return row.substr(0, floor) + row.substr(next);
}

// FR-015, R-07, D-4: the context of a node that never starts arrives at the
// start round and the stop round with no body run and no measured window
// open, the hazard of the comment at `source/harness/runner.cpp:2030`. The
// chain's first node records an error, so its two dependents never start, and
// the attempt completes: a stranded round is a hung run.
auto driveHeldContextArrivesBothRounds(const std::string& self,
                                       const std::string_view name) -> void
{
  auto status = 0;
  const auto report = runReportChild(self, "chain3fail", name, status);
  check(status != 0,
        "the run whose node recorded an error ends with the standing "
        "run-failure status (FR-021)");
  check(namedLineOf(report, "bmGateChainFail").contains("FAILED"),
        "the merged row of the failed attempt reads FAILED (FR-019)");
  check(
      report.contains("the scripted graph body failure"),
      "the reason of the failed row is the message the body recorded (FR-019)");
  check(bodiesOf(report) == "1,0,0",
        "the context of a held node runs no body, so it opens no measured "
        "window, and the attempt completes its two rounds (FR-015, FR-016)");
}

// FR-016: no window opens, closes or shifts because of the gate. The measured
// row of the graph run folds equal against the row of the same benchmark run
// with no graph, over the scripted leaves that make the row a fixed number.
auto driveGateOpensNoWindow(const std::string& self,
                            const std::string_view name) -> void
{
  auto graphStatus = 0;
  const auto graphReport = runReportChild(self, "chain3", name, graphStatus);
  auto plainStatus = 0;
  const auto plainReport = runReportChild(self, "plain3", name, plainStatus);
  check(graphStatus == 0 && plainStatus == 0,
        "both runs end with the success status (FR-016)");
  const auto graphRow =
      windowFieldsOf(rowWithoutName(rowOf(graphReport, "bmGateChain")));
  const auto plainRow =
      windowFieldsOf(rowWithoutName(rowOf(plainReport, "bmGatePlain")));
  check(!graphRow.empty() && !plainRow.empty(),
        "both runs print their measured row (FR-016)");
  check(graphRow == plainRow,
        "no window opens, closes or shifts because of the gate: the measured "
        "row stands as the graph-free run prints it (FR-016)");
}

// FR-017: the placement, pin, memory bind, wait mode, plan and recorder bytes
// of specs/025 and specs/035 stand unchanged beside a graph. The standing
// disclosure names the wait mode and the placement of every context of the
// run, and the graph changes none of it.
auto drivePlacementBytesUnchangedBesideGraph(const std::string& self,
                                             const std::string_view name)
    -> void
{
  auto graphStatus = 0;
  const auto graphReport = runReportChild(self, "chain3", name, graphStatus);
  auto plainStatus = 0;
  const auto plainReport = runReportChild(self, "plain3", name, plainStatus);
  const auto graphLine = contextsLineOf(graphReport);
  const auto plainLine = contextsLineOf(plainReport);
  check(!graphLine.empty() && graphLine == plainLine,
        "the placement disclosure of the threaded run stands unchanged beside "
        "a graph (FR-017)");
  check(windowFieldsOf(rowWithoutName(rowOf(graphReport, "bmGateChain")))
            == windowFieldsOf(rowWithoutName(rowOf(plainReport, "bmGatePlain"))),
        "the plan and the recorder bytes of the run stand unchanged beside a "
        "graph (FR-017)");
}

// The item shape of a threaded body, carrying the gate's calls: the seat the
// gate takes inside the timed path (D-4). The call operator stays `noexcept`,
// and the gate's answer is a word, so the item carries no error state.
struct GateCarryingItem
{
  sg::detail::GraphGate* gate = nullptr;
  std::uint32_t slot = 0;

  auto operator()(sg::WorkContext& /*context*/) noexcept -> void
  {
    if (gate->awaitStart(slot) == sg::detail::StartAnswer::RUN) {
      gate->nodeFinished(slot);
    }
  }
};

static_assert(sg::WorkItem<GateCarryingItem>,
              "a work item that carries the gate's calls stays a noexcept work "
              "item: the graph adds no error state to a work item (FR-018)");
static_assert(std::is_nothrow_invocable_v<GateCarryingItem&, sg::WorkContext&>,
              "no exception crosses the timed path (FR-018)");
static_assert(std::is_enum_v<sg::detail::StartAnswer>
                  && std::is_same_v<
                      std::underlying_type_t<sg::detail::StartAnswer>,
                      std::uint8_t>,
              "the gate answers one word of two, and carries no error state "
              "(FR-018, " "FR-019)");

// FR-018, R-08: the work items stay `noexcept`, no exception crosses the
// timed path, and the gate adds no error state to a work item. The item that
// carries the gate's calls runs on a context as any work item does, and the
// node settles `DONE`.
auto noexceptItemConceptHolds() -> void
{
  const auto graph = chainGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto context = sg::ExecutionContext {sg::ContextPolicy {}};

  auto item = GateCarryingItem {&gate, 0};
  auto node = sg::WorkNode<GateCarryingItem> {item};
  sg::dispatch(context, node);
  context.drain();

  check(node.state() == sg::WorkNodeState::DONE,
        "the item that carries the gate ran on the context, the noexcept path "
        "the gate stands inside (FR-018)");
  check(settlesAs(wordOf(gate.outcomes(), 0),
                  sg::detail::NodeOutcome::FINISHED),
        "the item settled its node finished, the word the graph carries "
        "(FR-018, " "FR-019)");

  context.requestStop();
  context.drain();
  context.join();
}

// ---------------------------------------------------------------------------
// The held path (US4)
// ---------------------------------------------------------------------------

// Two sources, each with one dependent, and one node that stands on both
// dependents: `a -> b`, `c -> d`, `b -> e`, `d -> e` over the slots 0 to 4,
// the shape that names a gater for each of two failures (FR-020).
[[nodiscard]] auto twoGatersGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "c", .slot = 2, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "d", .slot = 3, .deps = {"c"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "e", .slot = 4, .deps = {"b", "d"}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "b"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "c", .to = "d"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "b", .to = "e"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "d", .to = "e"});
  return graph;
}

// FR-020, SC-004: a failed node holds the transitive closure of its
// dependents, starts none of them, and names each with the nearest failed
// node on its chain as its reason. A node whose dependency stands held
// settles held too, and the reason names the failed node that gates the
// chain.
auto failedDependentsHoldWithReason() -> void
{
  const auto graph = chainOf(4);
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {0});

  // The source fails at its settle, and every dependent answers the gate's
  // skip: the closure never starts.
  awaitWord(slots[0].started, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  joinDrivers(threads);

  check(slots[1].started.load(std::memory_order_acquire) == 0
            && slots[2].started.load(std::memory_order_acquire) == 0
            && slots[3].started.load(std::memory_order_acquire) == 0,
        "a failed node starts none of its dependents (FR-020)");
  check(slots[1].skipped.load(std::memory_order_acquire) == 1
            && slots[2].skipped.load(std::memory_order_acquire) == 1
            && slots[3].skipped.load(std::memory_order_acquire) == 1,
        "the whole closure of a failed node answers the gate's skip (FR-020)");

  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FAILED,
        "the body that recorded an error settles its node failed (FR-019)");
  check(wordAt(settled, 1) == sg::detail::NodeOutcome::HELD
            && wordAt(settled, 2) == sg::detail::NodeOutcome::HELD
            && wordAt(settled, 3) == sg::detail::NodeOutcome::HELD,
        "the transitive closure of a failed node settles held (FR-020)");
  check(settled[1].reason == "n0" && settled[2].reason == "n0"
            && settled[3].reason == "n0",
        "each held node names the nearest failed node on its chain as its "
        "reason (FR-020, FR-023)");
}

// FR-020, SC-004: two nodes fail, and each held node names the failed node
// that gates it. The node that stands on both held dependents names the
// lower slot, the order FR-023 fixes.
auto twoFailuresNameTheirGater() -> void
{
  const auto graph = twoGatersGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {0, 2});

  // Both sources start at the standing start, lowest slot first, and both
  // settle failed: each chain holds behind its own gater.
  awaitWord(slots[0].started, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  awaitWord(slots[2].started, 1);
  slots[2].release.store(1, std::memory_order_release);
  awaitWord(slots[2].settled, 1);
  joinDrivers(threads);

  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FAILED
            && wordAt(settled, 2) == sg::detail::NodeOutcome::FAILED,
        "both scripted bodies settle their nodes failed (FR-019)");
  check(wordAt(settled, 1) == sg::detail::NodeOutcome::HELD
            && settled[1].reason == "a",
        "the dependent of the first failure names that failure (FR-020)");
  check(wordAt(settled, 3) == sg::detail::NodeOutcome::HELD
            && settled[3].reason == "c",
        "the dependent of the second failure names that failure (FR-020)");
  check(wordAt(settled, 4) == sg::detail::NodeOutcome::HELD
            && settled[4].reason == "a",
        "the node that stands on both held dependents names the failed node "
        "that gates it (FR-020, FR-023)");
}

// ---------------------------------------------------------------------------
// The stopped path (US4)
// ---------------------------------------------------------------------------

// FR-019, FR-015: the stop request settles the node it refuses stopped, the
// standing shape of specs/020 FR-050, and settles that node's dependents
// stopped with it. Each gated context answers the gate's skip and arrives at
// its stop round, so no round strands, and a stopped node carries no reason.
auto stopSettlesStoppedWithDependents() -> void
{
  const auto graph = chainGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {});

  // The source stands started, and its two dependents stand gated at the
  // gate: the stop request arrives between the rounds.
  awaitWord(slots[0].started, 1);
  awaitWord(slots[1].entered, 1);
  awaitWord(slots[2].entered, 1);
  gate.settleStopped();
  awaitWord(slots[1].skipped, 1);
  awaitWord(slots[2].skipped, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  joinDrivers(threads);

  check(slots[1].started.load(std::memory_order_acquire) == 0
            && slots[2].started.load(std::memory_order_acquire) == 0,
        "the stop request starts no gated node (FR-019)");
  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FINISHED,
        "the node the run let start settles finished (FR-019)");
  check(wordAt(settled, 1) == sg::detail::NodeOutcome::STOPPED
            && wordAt(settled, 2) == sg::detail::NodeOutcome::STOPPED,
        "the stop request settles the gated node stopped and its dependents "
        "stopped with it (FR-019)");
  check(settled[1].reason.empty() && settled[2].reason.empty(),
        "a stopped node carries no reason (FR-023)");
}

// FR-019, FR-021, SC-004: the run finishes the nodes that stand on their
// own, and the exit status follows the standing run-failure status of
// specs/015. The status-2 shape stays reserved for what the run refuses
// before it starts, so a run that fails after it starts answers the
// run-failure status. The record's reason rule stands at
// `reasonNamesFailedNode` of T025.
auto driveIndependentFinishAndStatusFollowsRunFailure(
    const std::string& self, const std::string_view name) -> void
{
  const auto graph = mixedGraph();
  auto gate = sg::detail::GraphGate(graph);
  auto log = DriveLog {};
  auto slots = Slots {};
  auto threads = spawnDrivers(gate, log, slots, graph, {0});

  // The chain's source fails and holds its one dependent, while the node
  // that stands on its own starts and settles finished.
  awaitWord(slots[0].started, 1);
  slots[0].release.store(1, std::memory_order_release);
  awaitWord(slots[0].settled, 1);
  awaitWord(slots[2].started, 1);
  slots[2].release.store(1, std::memory_order_release);
  awaitWord(slots[2].settled, 1);
  joinDrivers(threads);

  const auto settled = settleWords(graph, gate.outcomes(), false);
  check(wordAt(settled, 0) == sg::detail::NodeOutcome::FAILED
            && wordAt(settled, 1) == sg::detail::NodeOutcome::HELD,
        "the failure holds its own dependent (FR-020)");
  check(wordAt(settled, 2) == sg::detail::NodeOutcome::FINISHED,
        "the run finishes the node that stands on its own (FR-021)");

  // The same shape through the entry point: the run's status is the standing
  // run-failure status, and the independent context runs its body.
  auto status = 0;
  const auto report = runReportChild(self, "mixed3fail", name, status);
  check(status == 1,
        "the failed run ends with the standing run-failure status of "
        "specs/015, the status-2 shape standing reserved for what the run "
        "refuses before it starts (FR-021)");
  check(namedLineOf(report, "bmGateMixedFail").contains("FAILED"),
        "the merged row of the failed attempt reads FAILED (FR-019)");
  check(bodiesOf(report) == "1,0,1",
        "the context of the node that stands on its own runs its body, and "
        "the held context runs none (FR-021, FR-015)");
}

// ---------------------------------------------------------------------------
// The suite rule (SC-008)
// ---------------------------------------------------------------------------

// FR-026, FR-027, SC-008, R-16: the suite rule. The cpus come through
// `installTestOnlineCpus`, the chain's length follows the cpu count the seam
// installs, and no scenario names a node count, a sleep, or a clock. The
// chain and the diamond run over the two-cpu shape, and the entry point runs
// the chain of that shape to its measured row.
auto driveSuitePassesOnTwoCpus(const std::string& self,
                               const std::string_view name) -> void
{
  sg::detail::installTestOnlineCpus(std::vector<std::uint32_t> {0, 1});
  const auto cpus = sg::detail::onlineCpuSet();
  check(cpus.size() == 2,
        "the seam names the two cpus the suite rule runs on (FR-027)");

  // The chain over the two-cpu shape: one driver per node, the node count
  // read from the seam, and the start order the graph fixes.
  {
    const auto graph = chainOf(cpus.size());
    check(graph.nodes.size() == cpus.size(),
          "the chain carries one node per cpu of the installed shape (FR-027)");
    auto gate = sg::detail::GraphGate(graph);
    auto log = DriveLog {};
    auto slots = Slots {};
    auto threads = spawnDrivers(gate, log, slots, graph, {});
    releaseInOrder(slots, graph);
    joinDrivers(threads);
    check(log.starts == (std::vector<std::uint32_t> {0, 1}),
          "the chain over the two cpus starts in the graph's order (FR-013)");
    const auto settled = settleWords(graph, gate.outcomes(), false);
    check(wordAt(settled, 0) == sg::detail::NodeOutcome::FINISHED
              && wordAt(settled, 1) == sg::detail::NodeOutcome::FINISHED,
          "the chain over the two cpus settles every node finished (FR-019)");
  }

  // The diamond over the same two cpus: four drivers on two cpus, the
  // fan-in starting after the last of its dependencies and the release
  // order of FR-013 standing among the two middle nodes.
  {
    const auto graph = diamondGraph();
    auto gate = sg::detail::GraphGate(graph);
    auto log = DriveLog {};
    auto slots = Slots {};
    auto threads = spawnDrivers(gate, log, slots, graph, {});
    releaseInOrder(slots, graph);
    joinDrivers(threads);
    checkStartsFollowGraph(log, graph);
    check(log.starts.front() == 0 && log.starts.back() == 3,
          "the diamond over the two cpus records its source first and its "
          "last node last (FR-013, FR-014, SC-002)");
    const auto settled = settleWords(graph, gate.outcomes(), false);
    check(wordAt(settled, 3) == sg::detail::NodeOutcome::FINISHED,
          "the diamond over the two cpus settles its last node finished "
          "(FR-019)");
  }

  // The run over the same shape: the child installs the two cpus through
  // the seam, the chain's node count follows them, and the attempt
  // completes with its measured row.
  auto status = 0;
  const auto report = runReportChild(self, "onlineChain", name, status);
  check(status == 0,
        "the chain over the two-cpu shape runs to its measured row (FR-027, "
        "SC-008)");
  check(!rowOf(report, "bmGateOnlineChain").empty(),
        "the two-cpu run prints its measured row (SC-008)");
  check(bodiesOf(report) == "1,1",
        "every context of the two-cpu run runs its body and arrives at both "
        "rounds (FR-015, FR-027)");
}

struct Scenario
{
  std::string_view name;
  void (*body)();
  void (*drive)(const std::string& self, std::string_view name);
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from. A scenario absent here never runs. A scenario whose
// `drive` stands set runs its children through that driver, the shape the
// run-level scenarios need: the child runs one benchmark and writes its
// report, and the parent reads the bytes and states the fact.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"chainStartsInOrder", chainStartsInOrder, nullptr},
      Scenario {"independentStartLowestSlotFirst",
                independentStartLowestSlotFirst,
                nullptr},
      Scenario {"fanOutReleasesAll", fanOutReleasesAll, nullptr},
      Scenario {"fanInStartsAfterLast", fanInStartsAfterLast, nullptr},
      Scenario {"diamondStartsAfterBoth", diamondStartsAfterBoth, nullptr},
      Scenario {"heldContextArrivesBothRounds",
                nullptr,
                driveHeldContextArrivesBothRounds},
      Scenario {"gateOpensNoWindow", nullptr, driveGateOpensNoWindow},
      Scenario {"placementBytesUnchangedBesideGraph",
                nullptr,
                drivePlacementBytesUnchangedBesideGraph},
      Scenario {"noexceptItemConceptHolds", noexceptItemConceptHolds, nullptr},
      Scenario {"measuredRowSettlesAllFinished",
                measuredRowSettlesAllFinished,
                nullptr},
      Scenario {
          "skippedRowSettlesFinished", skippedRowSettlesFinished, nullptr},
      Scenario {
          "bodyErrorSettlesNodeFailed", bodyErrorSettlesNodeFailed, nullptr},
      Scenario {
          "setupFailureSettlesSlotZero", setupFailureSettlesSlotZero, nullptr},
      Scenario {"failedDependentsHoldWithReason",
                failedDependentsHoldWithReason,
                nullptr},
      Scenario {
          "twoFailuresNameTheirGater", twoFailuresNameTheirGater, nullptr},
      Scenario {"stopSettlesStoppedWithDependents",
                stopSettlesStoppedWithDependents,
                nullptr},
      Scenario {
          "independentFinishAndStatusFollowsRunFailure",
          nullptr,
          driveIndependentFinishAndStatusFollowsRunFailure,
      },
      Scenario {
          "suitePassesOnTwoCpus",
          nullptr,
          driveSuitePassesOnTwoCpus,
      },
  };
  return kList;
}

// The parent's default driver: one child runs the scenario body, and the
// success status stands owed.
auto runOneChild(const std::string& self, const std::string_view name) -> void
{
  check(runChild(self, {std::string(name)}, {}) == 0,
        "the scenario's process ended with the success status");
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const auto self = std::string(argc > 0 ? argv[0] : "harness_graph_gate_test");
  if (argc > 2 && std::string_view(argv[1]) == "run") {
    return runModeChild(argv[2]);
  }
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : scenarios()) {
      if (scenario.name != mode) {
        continue;
      }
      // The two shapes of scenario: a body runs in this process, and a drive
      // runs its children from this process.
      if (scenario.body != nullptr) {
        scenario.body();
      } else {
        scenario.drive(self, scenario.name);
      }
      return 0;
    }
    fail("the scenario name is not one of this file's list");
  }

  for (const auto& scenario : scenarios()) {
    if (scenario.drive != nullptr) {
      scenario.drive(self, scenario.name);
    } else {
      runOneChild(self, scenario.name);
    }
  }
  std::printf("harness_graph_gate_test: ok\n");
  return 0;
}
