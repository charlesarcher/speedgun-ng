// ============================================================================
// TDD test for the execution context (T004 to T009; US1, C-1 to C-3, FR-001 to
// FR-013, FR-040, FR-049, FR-050, SC-01 to SC-05).
//
// This file is the test-first half of the pair FR-042 fixes. It reaches the
// surface of include/speedgun-ng/execution_context.hpp by name only, so it
// links against definitions T010 and T011 own and stands red until they land.
//
// Every scenario runs in a re-exec of this binary, the pattern of
// harness_pause_test.cpp: one process per scenario keeps a context, the thread
// it owns, and any contract abort inside one address space, and the parent
// reads the wait status the kernel gave that child (fork, execv, waitpid). A
// breach of an `SG_REQUIRE` contract therefore arrives as the SIGABRT the
// contract facility gives it.
//
// Every wait ends on a state the scenario itself stands: `drain()` returns on
// the loop's idle proof, a read loop over `state()` ends on the completion
// state the completing thread writes, whose acquire order is what makes an
// item's writes visible to the caller (FR-049), and an atomic gate stands open
// from the moment the last thread it waits on arrives. Each scenario watches
// the state it depends on before it triggers the next step, so no scenario
// turns on which thread the scheduler ran first, and a wait carries no clock
// read and no count of polls.
//
// Scenario map:
//   identity            the context thread runs the work, `currentContext()`
//                       names the context inside the item and reports nothing
//                       on a foreign thread (SC-01, FR-001, FR-002)
//   waitModes           the one dispatch test under SPIN, YIELD and BLOCK, the
//                       BLOCK context woken by the dispatch it waits for
//                       (SC-02, FR-004)
//   lifecycle           the trio runs every queued node exactly once and
//                       nothing runs after `join` (SC-03, FR-006)
//   order               one dispatcher is FIFO exactly; two dispatchers are
//                       set membership plus per-dispatcher order (FR-013)
//   nested              work dispatched from inside running work runs after
//                       that item ends on the same context, and the running
//                       item reads the id and the stop token (FR-010, FR-011)
//   noalloc             zero allocations across the dispatch path under the
//                       counting hooks (SC-04, FR-040)
//   compileRefusal      an unapproved item type is refused where it names
//                       `WorkNode` (FR-008); the refusal is the compile of this
//                       file, and the mode prints the verdict
//   placement           a replaced placement function records one call per
//                       thread at thread start (SC-05, FR-005)
//   stopRace            no item runs twice when a BLOCK wake races the stop
//                       (FR-050, Edge Cases)
//   acceptedThenDrained nodes queued before the stop all run during `drain`
//                       (T006; FR-006, FR-050, SC-03)
//   refusedAfterStop    a dispatch that observes the stop completes its node
//                       STOPPED and the loop never touches it (T007; FR-050)
//   drainWithoutStop    a `drain` with no stop flushes, the loop keeps running,
//                       and the trio still closes the context (T009; FR-006)
//   completionOrder     the caller waits on `state`, reads the value the item
//                       wrote, and destroys the node at once (T005; FR-009,
//                       FR-012, FR-049)
//   yieldIdleWake       a YIELD context stands its empty-inbox wait after an
//                       empty flush, and each later dispatch is the wake that
//                       ends it (SC-02, FR-004)
//   blockIdleWake       a BLOCK context parks on its inbox flag; the dispatch
//                       notify and the stop request are the wakes that path
//                       names (FR-004, FR-006)
//   barrierLateArrival  one thread waits a round that its own arrival cannot
//                       complete while a second thread arrives later (FR-014)
//   acceptedNotLinked   three sets of nodes stand dispatched before, across
//                       and after one stop request; every node settles to one
//                       terminal state, the drain runs each node the accept
//                       counted, and the items the run settled match the
//                       nodes it took (FR-006, FR-012, FR-050)
//   enqueueWindow       a producer enqueues while the owner drains, so the
//                       walk meets nodes between their exchange and their
//                       link, and every node still runs once (FR-001, FR-006)
//   barrierRepeatedWaits one participant steps through its round wait while a
//                       second thread holds its arrival (FR-007, FR-014)
// Breaches, each expected to end its child by SIGABRT:
//   unjoined            the destructor of a context that was never joined
//                       (SC-03, FR-006)
//   liveNode            a node destroyed while the loop runs it (FR-012)
//   joinOwnThread       `join` from the thread of the context itself
//                       (Edge Cases)
//   badPlacement        a cpu index the host lacks, at the construction call
//                       (SC-05, FR-005)
//   ownThreadDrain      `drain` from the thread of the context itself (T008;
//                       FR-006, FR-050)
// ============================================================================

#include <array>
#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/execution_context.hpp"

// The zero-allocation proof replaces the global replaceable allocation
// operators, the pattern of counters_noalloc_test.cpp. The thread sanitizer
// runtime defines the same operators, so the two definitions cannot link
// together there; counters_noalloc_test keeps that collision out of the tsan
// preset at CMake level, and the same condition stands here at the level of the
// one scenario that needs the hooks, so the target itself still builds under
// the ci-tsan preset where the destroy-after-DONE proof of FR-049 runs.
// GCC answers `__SANITIZE_THREAD__`; Clang answers `__has_feature`.
#if defined(__SANITIZE_THREAD__)
#  define SG_TEST_HAS_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define SG_TEST_HAS_TSAN 1
#  else
#    define SG_TEST_HAS_TSAN 0
#  endif
#else
#  define SG_TEST_HAS_TSAN 0
#endif

// A named namespace stands in for the anonymous one: the item types below reach
// `dispatch` and `WorkNode`, and those templates are defined in the public
// header. Types with internal linkage would make every instantiation a use no
// translation unit can define, which is a compile error; external linkage lets
// the missing definitions show up where a test-first file must meet them, at
// the link.
namespace execution_context_test
{

// Nodes one scenario queues in a batch. Every tag stays inside these bounds,
// and the run log is indexed by position, so a scenario reads back the order
// the context ran its nodes in.
constexpr std::size_t kBatchNodes = 12;
constexpr std::size_t kDispatcherNodes = 8;
constexpr std::size_t kRunCapacity = 64;

// Tags at or above this base belong to the second dispatcher of the order
// scenario and to the nested work.
constexpr std::size_t kSecondDispatcherBase = 32;

// The value the completion-order item writes into caller storage (FR-049).
constexpr int kSentinelValue = 0x5E47;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS EXECUTION CONTEXT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto runChild(const char* self, const char* mode, const bool silence) -> int
{
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    if (silence && std::freopen("/dev/null", "w", stderr) == nullptr) {
      _exit(2);
    }
    char* const execArgv[] = {
        const_cast<char*>(self), const_cast<char*>(mode), nullptr};
    (void)execv(self, execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the child");
  }
  return status;
}

// The context of the running scenario, reached from inside running work: an
// item dispatches from within itself, and drains or joins the context that runs
// it, which is what the nested scenario and two breach modes drive (FR-010,
// FR-050).
sg::ExecutionContext* runningContext = nullptr;

// The contexts an item reaches are owned for the whole scenario, so the pointer
// a running item follows stands on every exit path of the scenario (FR-010).
std::unique_ptr<sg::ExecutionContext> contextOwner;

template<typename... Args>
auto ownedContext(Args&&... args) -> sg::ExecutionContext&
{
  contextOwner =
      std::make_unique<sg::ExecutionContext>(std::forward<Args>(args)...);
  return *contextOwner;
}

// The run log the context's own thread fills. Every write stands on that one
// thread and every read follows that thread's release write of a completion
// state, so the acquire read of `state()` is the whole synchronization
// (FR-049).
std::array<std::size_t, kRunCapacity> runOrder {};
std::array<std::size_t, kRunCapacity> runCount {};
std::size_t runPositions = 0;

struct Observation
{
  std::thread::id threadId {};
  std::size_t position = 0;
  std::uint64_t workContextSerial = 0;
  std::uint64_t currentSerial = 0;
  std::optional<sg::ContextId> workId {};
  std::optional<sg::ContextId> currentId {};
  int placementCalls = -1;
  bool sawCurrentContext = false;
  bool stopRequested = true;
};

// The placement recorder a scenario installs before its first context starts,
// so the pinning proof needs no privileges (FR-005, D-04).
std::atomic<int> placementCalls {0};
std::array<std::uint32_t, 4> placementSlot {};
std::array<std::thread::id, 4> placementThread {};

constexpr std::string_view kRecordedPlacement = "RECORDED";

auto recordPlacement(const sg::PlacementSlot& slot,
                     std::thread::native_handle_type) -> sg::PlacementResult
{
  const int index = placementCalls.fetch_add(1);
  if (index >= 0 && static_cast<std::size_t>(index) < placementSlot.size()) {
    placementSlot[static_cast<std::size_t>(index)] = slot.cpuIndex;
    placementThread[static_cast<std::size_t>(index)] =
        std::this_thread::get_id();
  }
  return {.applied = true, .description = kRecordedPlacement};
}

// The shared body of the items that report where they ran. The identity, the
// stop token and the placement count are read at the moment the item runs,
// which is what the contract promises a running item can see (FR-011).
auto observe(sg::WorkContext& work, std::size_t tag, Observation& out) -> void
{
  out.position = runPositions;
  out.threadId = std::this_thread::get_id();
  out.workContextSerial = work.id().serial;
  out.workId = work.id();
  if (const auto current = sg::currentContext(); current.has_value()) {
    out.sawCurrentContext = true;
    out.currentSerial = current->serial;
    out.currentId = *current;
  }
  out.stopRequested = work.stopToken().stopRequested();
  out.placementCalls = placementCalls.load();
  runOrder[runPositions] = tag;
  ++runPositions;
  ++runCount[tag];
}

// Reports the identity of the context that ran it (FR-002, FR-011).
struct ObserveItem
{
  std::size_t tag = 0;
  Observation* out = nullptr;

  auto operator()(sg::WorkContext& work) noexcept -> void
  {
    observe(work, tag, *out);
  }
};

// The node the nested item hands to its own context. Work dispatched from
// inside running work runs after that item ends (FR-010), so the node outlives
// the frame that offered it and stands at namespace scope for the scenario.
sg::WorkNode<ObserveItem>* nestedNode = nullptr;

// Reports its run and hands a second node to the same context from inside the
// run (FR-010).
struct NestedItem
{
  std::size_t tag = 0;
  Observation* out = nullptr;
  std::atomic<bool>* finished = nullptr;

  auto operator()(sg::WorkContext& work) noexcept -> void
  {
    observe(work, tag, *out);
    sg::dispatch(*runningContext, *nestedNode);
    finished->store(true, std::memory_order_release);
  }
};

// Writes one value into caller storage, the shape whose completion order FR-049
// holds the loop to (T005).
struct ValueItem
{
  int* slot = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void { *slot = kSentinelValue; }
};

// Finishes only once the stop request stands, so a scenario can hold the loop
// on one node while the rest of its batch stands queued (FR-006).
struct HoldItem
{
  std::size_t tag = 0;
  Observation* out = nullptr;

  auto operator()(sg::WorkContext& work) noexcept -> void
  {
    while (!work.stopToken().stopRequested()) {
    }
    observe(work, tag, *out);
  }
};

// Finishes only once the scenario releases it, so the caller can catch the node
// in RUNNING (FR-012). The flag crosses threads, so it stands atomic against
// the same elided wait: a spin on a plain bool whose value never changes is
// the undefined behaviour GCC at -O3 elides, and the elided item settled the
// node DONE before the caller reads RUNNING. An atomic load is a side effect no
// optimizer may drop, so the hold stands.
struct ReleaseItem
{
  std::atomic<bool>* released = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    while (!released->load(std::memory_order_acquire)) {
    }
  }
};

// Drains the context that runs it, the debug-precondition breach of FR-050.
// The flag records a call that came back, which happens only where the debug
// precondition is compiled out: under the enforce semantic the process ends
// inside the call, and the waiting thread never sees the flag.
struct SelfDrainItem
{
  std::atomic<bool>* returned = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    runningContext->drain();
    returned->store(true, std::memory_order_release);
  }
};

// Joins the context that runs it, the contract breach of FR-006, recorded the
// same way as the drain above.
struct SelfJoinItem
{
  std::atomic<bool>* returned = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    runningContext->join();
    returned->store(true, std::memory_order_release);
  }
};

// The body of the zero-allocation proof: nothing but the dispatch path runs
// while the counting hooks are armed (FR-040).
struct NoAllocItem
{
  std::size_t tag = 0;

  auto operator()(sg::WorkContext&) noexcept -> void { ++runCount[tag]; }
};

// A caller-owned batch of nodes. A node lives in caller storage and moves
// nowhere (FR-009), so a batch is one aligned block the scenario places each
// node into and destroys by hand.
template<sg::WorkItem Item, std::size_t Count>
class alignas(sg::WorkNode<Item>) NodeBatch
{
  std::array<std::byte, Count * sizeof(sg::WorkNode<Item>)> m_bytes {};

  auto addressOf(const std::size_t index) -> sg::WorkNode<Item>*
  {
    return reinterpret_cast<sg::WorkNode<Item>*>(
        m_bytes.data() + index * sizeof(sg::WorkNode<Item>));
  }

public:
  auto construct(const std::size_t index, Item item) -> sg::WorkNode<Item>&
  {
    return *new (addressOf(index)) sg::WorkNode<Item>(std::move(item));
  }

  auto at(const std::size_t index) -> sg::WorkNode<Item>&
  {
    return *addressOf(index);
  }

  auto destroyAll() -> void
  {
    for (std::size_t index = 0; index < Count; ++index) {
      std::destroy_at(addressOf(index));
    }
  }
};

// Waits for a node to settle with no clock and no bound: the completing thread
// writes the state with release order as its last access to the node, so this
// read loop is the whole wait (FR-049).
template<sg::WorkItem Item>
auto settled(sg::WorkNode<Item>& node) -> sg::WorkNodeState
{
  for (;;) {
    const sg::WorkNodeState state = node.state();
    if (state == sg::WorkNodeState::DONE || state == sg::WorkNodeState::STOPPED)
    {
      return state;
    }
  }
}

// Watches a node until the loop has taken it (FR-012). The hold that feeds
// this watch stands until the caller releases it after the observation, so
// the running state is read to stay: a settle seen here is the loop having
// run the node unseen, and the wait fails fast on that check rather than
// spinning on a state that cannot return.
template<sg::WorkItem Item>
auto watchRunning(sg::WorkNode<Item>& node) -> void
{
  for (;;) {
    const sg::WorkNodeState state = node.state();
    if (state == sg::WorkNodeState::RUNNING) {
      return;
    }
    check(state != sg::WorkNodeState::DONE
              && state != sg::WorkNodeState::STOPPED,
          "the loop settled the node before the caller observed its running "
          "(FR-012)");
  }
}

auto blockingPolicy() -> sg::ContextPolicy
{
  return {.waitMode = sg::WaitMode::BLOCK,
          .placement = sg::PlacementSlot::noPin()};
}

auto spinningPolicy() -> sg::ContextPolicy
{
  return {.waitMode = sg::WaitMode::SPIN,
          .placement = sg::PlacementSlot::noPin()};
}

// Closes a context through the one legal lifecycle (FR-006).
auto closeContext(sg::ExecutionContext& context) -> void
{
  context.requestStop();
  context.drain();
  context.join();
}

// SC-01, FR-001, FR-002: the work runs on the thread the context owns, names
// that context from inside itself, and a thread the library runs no context for
// names none.
auto scenarioIdentity() -> int
{
  sg::ExecutionContext context;
  check(context.policy().waitMode == sg::WaitMode::BLOCK,
        "a caller-created context with no policy waits in BLOCK (FR-004)");
  check(context.policy().placement.cpuIndex == sg::kNoPin,
        "a caller-created context with no policy names no pin (FR-005)");
  check(!sg::currentContext().has_value(),
        "the calling thread names no context (FR-002)");

  bool foreignSawContext = true;
  {
    std::thread foreign {[&foreignSawContext] {
      foreignSawContext = sg::currentContext().has_value();
    }};
    foreign.join();
  }
  check(!foreignSawContext,
        "a thread the library runs no context for names no context (FR-002)");

  Observation observed {};
  const ObserveItem item {0, &observed};
  sg::WorkNode<ObserveItem> node {item};
  sg::dispatch(context, node);
  context.drain();

  check(node.state() == sg::WorkNodeState::DONE,
        "a node the loop ran reads DONE (FR-009)");
  check(runCount[0] == 1, "the item ran once (FR-001)");
  check(observed.sawCurrentContext,
        "the running item names a context (FR-002)");
  check(observed.currentSerial == context.id().serial,
        "currentContext names the context running the item (FR-002)");
  check(observed.workContextSerial == context.id().serial,
        "the work context carries the same identity (FR-011)");
  check(observed.currentId.has_value() && observed.workId.has_value()
            && *observed.currentId == *observed.workId,
        "the two identities of one context compare equal (FR-002)");
  sg::ExecutionContext sibling;
  sibling.requestStop();
  sibling.drain();
  sibling.join();
  check(observed.workId.has_value() && *observed.workId != sibling.id(),
        "the identities of two live contexts compare unequal (FR-002)");
  check(observed.threadId != std::this_thread::get_id(),
        "the work ran on the thread of the context (FR-001)");
  check(!observed.stopRequested,
        "the stop token reads false before any stop request stands (FR-011)");

  closeContext(context);
  std::puts("harness_execution_context_test identity: ok");
  return 0;
}

// SC-02, FR-004: the same dispatch test under each wait mode. A BLOCK context
// parks on its inbox flag until a dispatch wakes it, and the only waits in this
// file are the loop's own: no poll interval and no clock stand here, so the
// item running at all is the wake.
auto scenarioWaitModes() -> int
{
  std::size_t tag = 0;
  for (const auto mode :
       {sg::WaitMode::SPIN, sg::WaitMode::YIELD, sg::WaitMode::BLOCK})
  {
    const sg::ContextPolicy policy {.waitMode = mode,
                                    .placement = sg::PlacementSlot::noPin()};
    sg::ExecutionContext context {policy};
    check(context.policy().waitMode == mode,
          "the policy stands fixed and readable for the life of the context "
          "(FR-007)");

    Observation observed {};
    const ObserveItem item {tag, &observed};
    sg::WorkNode<ObserveItem> node {item};
    sg::dispatch(context, node);
    context.drain();

    check(node.state() == sg::WorkNodeState::DONE,
          "the node settles DONE under each wait mode (SC-02, FR-004)");
    check(runCount[tag] == 1, "the item ran once under each wait mode");
    check(
        observed.threadId != std::this_thread::get_id(),
        "the item ran on the context's thread under each wait mode " "(FR-"
                                                                     "001)");
    check(observed.currentSerial == context.id().serial,
          "the item named its context under each wait mode (FR-002)");
    closeContext(context);
    ++tag;
  }
  std::puts("harness_execution_context_test waitModes: ok");
  return 0;
}

// SC-03, FR-006: the trio runs every queued node exactly once, and nothing runs
// after join returns.
auto scenarioLifecycle() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  NodeBatch<ObserveItem, kBatchNodes> batch;
  std::array<Observation, kBatchNodes> observed {};

  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    sg::dispatch(context,
                 batch.construct(index, ObserveItem {index, &observed[index]}));
  }
  closeContext(context);

  const std::size_t positionsAfterJoin = runPositions;
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    check(batch.at(index).state() == sg::WorkNodeState::DONE,
          "every node queued before the stop request reads DONE (SC-03, "
          "FR-006)");
    check(runCount[index] == 1,
          "every queued node ran exactly once (SC-03, FR-006)");
    check(observed[index].threadId != std::this_thread::get_id(),
          "every queued node ran on the context's thread (FR-001)");
  }
  check(runPositions == kBatchNodes,
        "the batch filled the run log once " "(SC-03, FR-006)");
  check(positionsAfterJoin == runPositions,
        "no node ran after join returned (FR-006)");

  batch.destroyAll();
  std::puts("harness_execution_context_test lifecycle: ok");
  return 0;
}

// FR-013: nodes one thread dispatches run in that order; two threads
// dispatching at once give set membership plus order within each dispatcher.
auto orderChecks() -> void
{
  {
    sg::ExecutionContext context {spinningPolicy()};
    NodeBatch<ObserveItem, kBatchNodes> batch;
    std::array<Observation, kBatchNodes> observed {};
    for (std::size_t index = 0; index < kBatchNodes; ++index) {
      sg::dispatch(
          context,
          batch.construct(index, ObserveItem {index, &observed[index]}));
    }
    closeContext(context);

    check(runPositions == kBatchNodes,
          "the single dispatcher's batch ran in full (FR-013)");
    for (std::size_t position = 0; position < runPositions; ++position) {
      check(runOrder[position] == position,
            "one dispatcher is served first in first out exactly (FR-013)");
    }
    batch.destroyAll();
  }

  runPositions = 0;
  for (std::size_t index = 0; index < kRunCapacity; ++index) {
    runCount[index] = 0;
  }

  auto& context = ownedContext(spinningPolicy());
  NodeBatch<ObserveItem, kDispatcherNodes> first;
  NodeBatch<ObserveItem, kDispatcherNodes> second;
  std::array<Observation, kDispatcherNodes> firstObserved {};
  std::array<Observation, kDispatcherNodes> secondObserved {};
  std::array<bool, kDispatcherNodes> seen {};

  // The two dispatchers start together on an atomic gate, so the inbox is what
  // orders them and no elapsed time stands in the test.
  std::atomic<int> ready {0};
  std::atomic<bool> start {false};
  const auto drive =
      [&ready, &start](NodeBatch<ObserveItem, kDispatcherNodes>& batch,
                       std::array<Observation, kDispatcherNodes>& observed,
                       const std::size_t base)
  {
    ++ready;
    while (!start.load()) {
    }
    for (std::size_t index = 0; index < kDispatcherNodes; ++index) {
      sg::dispatch(
          *runningContext,
          batch.construct(index, ObserveItem {base + index, &observed[index]}));
    }
  };
  runningContext = &context;
  std::thread firstThread {[&] { drive(first, firstObserved, 0); }};
  std::thread secondThread {
      [&] { drive(second, secondObserved, kSecondDispatcherBase); }};
  while (ready.load() < 2) {
  }
  start.store(true);
  firstThread.join();
  secondThread.join();
  closeContext(context);
  runningContext = nullptr;

  check(runPositions == 2 * kDispatcherNodes,
        "both dispatchers' nodes ran (FR-013)");
  std::size_t lastFirst = 0;
  std::size_t lastSecond = 0;
  std::size_t firstSeen = 0;
  std::size_t secondSeen = 0;
  for (std::size_t position = 0; position < runPositions; ++position) {
    const std::size_t tag = runOrder[position];
    check(tag < kRunCapacity && runCount[tag] == 1,
          "every dispatched node ran once and only once (FR-013)");
    if (tag < kSecondDispatcherBase) {
      check(tag >= lastFirst,
            "the first dispatcher's nodes ran in the order it dispatched them "
            "(FR-013)");
      lastFirst = tag;
      ++firstSeen;
    } else {
      check(tag >= lastSecond,
            "the second dispatcher's nodes ran in the order it dispatched them "
            "(FR-013)");
      lastSecond = tag;
      ++secondSeen;
    }
    seen[tag % kDispatcherNodes] = true;
  }
  check(firstSeen == kDispatcherNodes && secondSeen == kDispatcherNodes,
        "the run carries every node both dispatchers offered (FR-013)");
  for (const bool one : seen) {
    check(one, "every index of each dispatcher appears in the run (FR-013)");
  }
  for (std::size_t index = 0; index < kDispatcherNodes; ++index) {
    check(first.at(index).state() == sg::WorkNodeState::DONE
              && second.at(index).state() == sg::WorkNodeState::DONE,
          "every node of both dispatchers settles DONE (FR-009)");
    check(firstObserved[index].threadId == secondObserved[index].threadId,
          "both dispatchers' work ran on the one context's thread (FR-001)");
  }
  first.destroyAll();
  second.destroyAll();
}

auto scenarioOrder() -> int
{
  orderChecks();
  std::puts("harness_execution_context_test order: ok");
  return 0;
}

// FR-010, FR-011: work dispatched from inside running work runs after that item
// ends, on the same context, and the running item reads the identity of its
// context and a stop token that reads true once the stop request stands.
auto scenarioNested() -> int
{
  auto& context = ownedContext(spinningPolicy());
  runningContext = &context;

  std::atomic<bool> outerFinished {false};
  Observation outerObserved {};
  Observation innerObserved {};
  Observation holdObserved {};
  const ObserveItem innerItem {kSecondDispatcherBase, &innerObserved};
  sg::WorkNode<ObserveItem> innerNode {innerItem};
  nestedNode = &innerNode;

  const NestedItem outer {0, &outerObserved, &outerFinished};
  sg::WorkNode<NestedItem> outerNode {outer};
  const HoldItem hold {1, &holdObserved};
  sg::WorkNode<HoldItem> holdNode {hold};

  sg::dispatch(context, outerNode);
  sg::dispatch(context, holdNode);
  // FR-050: the nested dispatch belongs to the run it comes from, so the stop
  // request must not stand before it happens. The parent's two enqueues reach
  // the inbox before the handoff to the context's thread completes, so without
  // this wait the nested dispatch observes the stopped state and FR-050 refuses
  // the nested node STOPPED.
  while (!outerFinished.load(std::memory_order_acquire)) {
  }
  // The hold node was accepted before the stop request stood, so the drain runs
  // it and its token reads true (FR-006, FR-011).
  context.requestStop();
  context.drain();

  check(outerNode.state() == sg::WorkNodeState::DONE,
        "the item that dispatched from inside itself ran (FR-010)");
  check(innerNode.state() == sg::WorkNodeState::DONE,
        "work dispatched from inside running work runs (FR-010)");
  check(
      holdNode.state() == sg::WorkNodeState::DONE,
      "the node accepted before the stop request ran during drain " "(FR-006)");
  check(runCount[kSecondDispatcherBase] == 1,
        "the inner work ran once " "(FR-010)");
  check(innerObserved.position > outerObserved.position,
        "work dispatched from inside running work runs after that item ends "
        "(FR-010, FR-013)");
  check(innerObserved.threadId == outerObserved.threadId,
        "work dispatched from inside running work runs on the same context "
        "(FR-010)");
  check(innerObserved.threadId != std::this_thread::get_id(),
        "the nested work ran on the context's thread (FR-001)");
  check(innerObserved.currentSerial == context.id().serial
            && outerObserved.currentSerial == context.id().serial,
        "both items named the same context (FR-002)");
  check(outerFinished.load(std::memory_order_acquire),
        "the outer item reached its last statement (FR-010)");
  check(holdObserved.stopRequested,
        "a running item reads its stop token true once requestStop stood "
        "(FR-011)");
  check(holdObserved.workContextSerial == context.id().serial,
        "the work context of the held item names its context (FR-011)");

  context.join();
  runningContext = nullptr;
  nestedNode = nullptr;
  std::puts("harness_execution_context_test nested: ok");
  return 0;
}

// The counting hooks of the zero-allocation proof (SC-04, FR-040). The thread
// sanitizer runtime owns these operators, so the hooks, the replaced forms and
// the scenario that arms them stand aside there.
#if !SG_TEST_HAS_TSAN
std::atomic<bool> countAllocations {false};
std::atomic<long long> allocations {0};

auto noteAllocation() -> void
{
  if (countAllocations.load(std::memory_order_relaxed)) {
    ++allocations;
  }
}

auto allocate(const std::size_t size) -> void*
{
  noteAllocation();
  void* block = std::malloc(size == 0 ? 1 : size);
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  return block;
}

auto allocateNothrow(const std::size_t size) noexcept -> void*
{
  noteAllocation();
  return std::malloc(size == 0 ? 1 : size);
}

auto allocateAligned(const std::size_t size,
                     const std::size_t alignment) -> void*
{
  noteAllocation();
  const std::size_t request = size == 0 ? 1 : size;
  const std::size_t rounded = (request + alignment - 1) & ~(alignment - 1);
  void* block = std::aligned_alloc(alignment, rounded);
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  return block;
}

auto allocateAlignedNothrow(const std::size_t size,
                            const std::size_t alignment) noexcept -> void*
{
  noteAllocation();
  const std::size_t request = size == 0 ? 1 : size;
  const std::size_t rounded = (request + alignment - 1) & ~(alignment - 1);
  return std::aligned_alloc(alignment, rounded);
}
#endif  // !SG_TEST_HAS_TSAN

// SC-04, FR-040: building the caller's node and dispatching it performs no
// allocation, on either thread.
auto scenarioNoAlloc() -> int
{
#if SG_TEST_HAS_TSAN
  std::puts("harness_execution_context_test noalloc: the thread sanitizer "
            "runtime owns the replaceable allocation operators, so the "
            "counting hooks stand aside there (SC-04, FR-040)");
  return 0;
#else
  sg::ExecutionContext context {blockingPolicy()};
  NodeBatch<NoAllocItem, kBatchNodes> batch;

  allocations.store(0);
  countAllocations.store(true);
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    sg::dispatch(context, batch.construct(index, NoAllocItem {index}));
  }
  countAllocations.store(false);

  check(allocations.load() == 0,
        "the dispatch path allocated nothing (SC-04, FR-040)");
  closeContext(context);
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    check(batch.at(index).state() == sg::WorkNodeState::DONE,
          "every node of the counted batch ran (FR-010)");
    check(runCount[index] == 1, "every node of the counted batch ran once");
  }
  batch.destroyAll();
  std::puts("harness_execution_context_test noalloc: ok");
  return 0;
#endif
}

// FR-008: the refusal of an unapproved item type is a property of the compile
// of this file, the same proof shape as test/source/dbc_standalone.cpp. An item
// outside the concept cannot name `WorkNode` and cannot reach `dispatch()`,
// which is where the program stops building.
struct NotNoexceptItem
{
  auto operator()(sg::WorkContext&) -> void {}
};

struct WrongArgumentItem
{
  auto operator()(int) noexcept -> void {}
};

struct NotCallableItem
{
  int value = 0;
};

static_assert(sg::WorkItem<ObserveItem>,
              "a noexcept item taking the work context is approved (FR-008)");
static_assert(!sg::WorkItem<NotNoexceptItem>,
              "an item whose call is not noexcept is refused (FR-008)");
static_assert(!sg::WorkItem<WrongArgumentItem>,
              "an item that takes no work context is refused (FR-008)");
static_assert(!sg::WorkItem<NotCallableItem>,
              "a type that is not invocable is refused (FR-008)");
static_assert(
    !sg::WorkItem<std::function<void(sg::WorkContext&)>>,
    "a type-erased callable is refused: its call is not noexcept " "(FR-008)");

// Naming the node with a refused type is the refusal point FR-008 names.
template<class Item>
concept NodeFormable = requires { typename sg::WorkNode<Item>; };

static_assert(NodeFormable<ObserveItem>,
              "an approved item names a work node (FR-009)");
static_assert(!NodeFormable<NotNoexceptItem>,
              "an unapproved item cannot name a work node (FR-008)");
static_assert(!NodeFormable<WrongArgumentItem>,
              "an unapproved item cannot name a work node (FR-008)");
static_assert(!NodeFormable<NotCallableItem>,
              "an unapproved item cannot name a work node (FR-008)");

auto scenarioCompileRefusal() -> int
{
  check(!NodeFormable<NotNoexceptItem> && !NodeFormable<NotCallableItem>,
        "the unapproved item types were refused where they name WorkNode "
        "(FR-008)");
  std::puts("harness_execution_context_test compileRefusal: ok");
  return 0;
}

// SC-05, FR-005: the installed placement function is called once per context
// thread at thread start, with that context's slot, before any of that
// context's work runs. The install stands before the first context of the
// process, the rule D-04 states, so no privileges are needed. The contexts run
// one after the other, so the order of the records names them.
auto scenarioPlacement() -> int
{
  sg::setPlacementFunction(&recordPlacement);

  const std::array<sg::PlacementSlot, 2> slots {sg::PlacementSlot::noPin(),
                                                sg::PlacementSlot::cpu(0)};
  std::size_t tag = 0;
  for (std::size_t index = 0; index < slots.size(); ++index) {
    const sg::ContextPolicy policy {.waitMode = sg::WaitMode::BLOCK,
                                    .placement = slots[index]};
    sg::ExecutionContext context {policy};
    check(context.policy().placement.cpuIndex == slots[index].cpuIndex,
          "the placement of the policy reads back (FR-007)");

    Observation observed {};
    const ObserveItem item {tag, &observed};
    sg::WorkNode<ObserveItem> node {item};
    sg::dispatch(context, node);
    context.drain();

    check(node.state() == sg::WorkNodeState::DONE,
          "the placed context ran its work (FR-001)");
    check(observed.placementCalls >= 1,
          "the placement stands before the context runs any work (SC-05, "
          "FR-005)");
    check(placementCalls.load() == static_cast<int>(index) + 1,
          "one placement call per context, and no more (SC-05, FR-005)");
    check(placementSlot[index] == slots[index].cpuIndex,
          "the placement function received that context's slot (SC-05, "
          "FR-005)");
    check(placementThread[index] == observed.threadId,
          "the placement ran on the thread of that context, at its start "
          "(SC-05, FR-005)");
    closeContext(context);
    ++tag;
  }
  check(placementCalls.load() == static_cast<int>(slots.size()),
        "the replacement recorded exactly one call per context thread "
        "(SC-05, FR-005)");
  std::puts("harness_execution_context_test placement: ok");
  return 0;
}

// FR-050, Edge Cases: a BLOCK wake racing the stop request leaves no item run
// twice. Each node lands on one side of the linearization at its enqueue, and
// the checks hold whichever side that is.
auto scenarioStopRace() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  NodeBatch<NoAllocItem, kDispatcherNodes> batch;

  // Offered before the stop request can stand, so one node is certain to be
  // accepted and the drain has something to run.
  const NoAllocItem earlier {kSecondDispatcherBase};
  sg::WorkNode<NoAllocItem> earlierNode {earlier};
  sg::dispatch(context, earlierNode);

  std::atomic<bool> start {false};
  std::atomic<int> ready {0};
  std::thread dispatcher {
      [&]
      {
        ++ready;
        while (!start.load()) {
        }
        for (std::size_t index = 0; index < kDispatcherNodes; ++index) {
          sg::dispatch(context, batch.construct(index, NoAllocItem {index}));
        }
      }};
  while (ready.load() < 1) {
  }
  start.store(true);
  context.requestStop();
  dispatcher.join();
  context.drain();
  context.join();

  check(earlierNode.state() == sg::WorkNodeState::DONE,
        "the node accepted before the stop request ran (FR-050)");
  check(runCount[kSecondDispatcherBase] == 1,
        "the accepted node ran once while the stop request raced the "
        "dispatches " "(FR-050)");
  for (std::size_t index = 0; index < kDispatcherNodes; ++index) {
    const sg::WorkNodeState state = batch.at(index).state();
    check(
        state == sg::WorkNodeState::DONE || state == sg::WorkNodeState::STOPPED,
        "every node settles to one completion state (FR-050)");
    check(runCount[index] == (state == sg::WorkNodeState::DONE ? 1 : 0),
          "no item runs twice, and a refused item never runs (FR-050)");
  }
  batch.destroyAll();
  std::puts("harness_execution_context_test stopRace: ok");
  return 0;
}

// T006, FR-006, FR-050, SC-03: the nodes accepted before the stop request all
// run during drain, each exactly once.
auto scenarioAcceptedThenDrained() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  NodeBatch<ObserveItem, kBatchNodes> batch;
  std::array<Observation, kBatchNodes> observed {};
  Observation holdObserved {};
  const HoldItem hold {kSecondDispatcherBase, &holdObserved};
  sg::WorkNode<HoldItem> holdNode {hold};

  // The hold node keeps the loop busy while the batch behind it is queued, so
  // the whole batch stands in the inbox when requestStop runs.
  sg::dispatch(context, holdNode);
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    sg::dispatch(context,
                 batch.construct(index, ObserveItem {index, &observed[index]}));
  }
  context.requestStop();
  context.drain();

  check(holdNode.state() == sg::WorkNodeState::DONE,
        "the node that held the loop ran (T006, FR-006)");
  check(holdObserved.stopRequested,
        "the loop was still running work when the stop request stood "
        "(T006, FR-006)");
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    check(batch.at(index).state() == sg::WorkNodeState::DONE,
          "every node accepted before the stop request reads DONE (T006, "
          "FR-006)");
    check(runCount[index] == 1,
          "every node accepted before the stop request ran exactly once "
          "(T006, SC-03)");
  }
  check(
      runPositions == kBatchNodes + 1,
      "the drain ran the whole accepted set and nothing else (T006, " "FR-"
                                                                      "006)");
  context.join();
  batch.destroyAll();
  std::puts("harness_execution_context_test acceptedThenDrained: ok");
  return 0;
}

// T007, FR-050: a dispatch that observes the stop request refuses its node and
// completes it itself, and the loop never touches a node it never accepted.
auto scenarioRefusedAfterStop() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  context.requestStop();

  const NoAllocItem item {0};
  sg::WorkNode<NoAllocItem> node {item};
  sg::dispatch(context, node);

  check(node.state() == sg::WorkNodeState::STOPPED,
        "a dispatch that observes the stopped state completes its node "
        "STOPPED (T007, FR-050)");
  check(runCount[0] == 0, "the refused item never ran (T007, FR-050)");

  context.drain();
  check(node.state() == sg::WorkNodeState::STOPPED,
        "the loop never touches a node it never accepted (T007, FR-050)");
  check(runCount[0] == 0,
        "the refused item stayed unrun through the drain (T007, FR-050)");
  context.join();
  std::puts("harness_execution_context_test refusedAfterStop: ok");
  return 0;
}

// T009, FR-006, FR-050: a drain with no prior stop request flushes the inbox
// and leaves the loop running, so a later dispatch still runs and the lifecycle
// still closes the context.
auto scenarioDrainWithoutStop() -> int
{
  constexpr std::size_t kFlushNodes = 4;
  sg::ExecutionContext context {spinningPolicy()};
  NodeBatch<ObserveItem, kFlushNodes> batch;
  std::array<Observation, kFlushNodes> observed {};
  for (std::size_t index = 0; index < kFlushNodes; ++index) {
    sg::dispatch(context,
                 batch.construct(index, ObserveItem {index, &observed[index]}));
  }
  context.drain();

  for (std::size_t index = 0; index < kFlushNodes; ++index) {
    check(batch.at(index).state() == sg::WorkNodeState::DONE,
          "the drain left every queued node completed (T009, FR-006)");
    check(runCount[index] == 1,
          "the flush ran each queued node once (T009, FR-050)");
  }
  check(runPositions == kFlushNodes,
        "the drain returned with the inbox empty and the loop idle (T009, "
        "FR-050)");

  Observation later {};
  const ObserveItem laterItem {kSecondDispatcherBase, &later};
  sg::WorkNode<ObserveItem> laterNode {laterItem};
  sg::dispatch(context, laterNode);
  context.drain();
  check(laterNode.state() == sg::WorkNodeState::DONE,
        "the loop kept running after the flush, so a later dispatch still ran "
        "(T009, FR-006)");

  closeContext(context);
  check(runPositions == kFlushNodes + 1,
        "the lifecycle closed the context with nothing left to run (T009, "
        "FR-006)");
  batch.destroyAll();
  std::puts("harness_execution_context_test drainWithoutStop: ok");
  return 0;
}

// T005, FR-009, FR-012, FR-049: the caller waits on the completion state, reads
// what the item wrote, and destroys the node at the read that saw it. Under the
// ci-tsan run (T038) this sequence reports no race, because the release write
// of DONE is the loop's last access to the node.
auto scenarioCompletionOrder() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  int value = 0;
  alignas(sg::WorkNode<ValueItem>)
      std::array<std::byte, sizeof(sg::WorkNode<ValueItem>)>
          bytes {};
  auto* node = new (bytes.data()) sg::WorkNode<ValueItem> {ValueItem {&value}};

  sg::dispatch(context, *node);
  check(settled(*node) == sg::WorkNodeState::DONE,
        "the node settles DONE (T005, FR-009)");
  check(value == kSentinelValue,
        "the value the item wrote is visible to the reader that saw DONE "
        "(T005, FR-049)");
  node->~WorkNode();

  closeContext(context);
  std::puts("harness_execution_context_test completionOrder: ok");
  return 0;
}

// FR-006: the close of a context is idempotent, so the second join() of a
// joined context returns and runs nothing a second time.
auto scenarioJoinTwice() -> int
{
  constexpr std::size_t kJoinTwiceTag = 48;
  sg::ExecutionContext context {blockingPolicy()};
  const NoAllocItem item {kJoinTwiceTag};
  sg::WorkNode<NoAllocItem> node {item};
  sg::dispatch(context, node);
  context.requestStop();
  context.drain();
  context.join();
  context.join();

  check(node.state() == sg::WorkNodeState::DONE,
        "the node accepted before the close settled DONE (FR-006)");
  check(runCount[kJoinTwiceTag] == 1,
        "the item ran once across the second join() (FR-006)");
  std::puts("harness_execution_context_test joinTwice: ok");
  return 0;
}

// FR-011: a stop token used off any context reads no stop request, so a caller
// outside a running item never reads one, live or joined.
auto scenarioStopTokenOffContext() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  check(!sg::StopToken {}.stopRequested(),
        "a token used off any context reads no stop request (FR-011)");
  closeContext(context);
  check(!sg::StopToken {}.stopRequested(),
        "a token reads no stop request once the context is joined (FR-011)");
  std::puts("harness_execution_context_test stopTokenOffContext: ok");
  return 0;
}

// FR-001, FR-024, FR-002: a context holds its own control block, so no count of
// live contexts bounds a program. 72 stand alive at once, over the 64 the
// removed side table held: an aborted construction ends the child before any
// check runs, and the exit status the parent reads carries that half.
auto scenarioManyContexts() -> int
{
  // One slot and one node per context stand in caller storage: the shared run
  // log indexes kRunCapacity tags, which 72 nodes would exceed.
  constexpr std::size_t kManyContexts = 72;
  std::array<std::optional<sg::ExecutionContext>, kManyContexts> contexts {};
  std::array<sg::ContextId, kManyContexts> ids {};
  std::array<int, kManyContexts> slots {};
  NodeBatch<ValueItem, kManyContexts> batch;

  // The construction loop ends before any context is closed, so all 72 stand
  // alive together through the dispatch and drain loops below.
  for (std::size_t index = 0; index < kManyContexts; ++index) {
    sg::ExecutionContext& context = contexts[index].emplace(blockingPolicy());
    ids[index] = context.id();
  }
  for (std::size_t index = 0; index < kManyContexts; ++index) {
    sg::dispatch(*contexts[index],
                 batch.construct(index, ValueItem {&slots[index]}));
  }
  for (std::size_t index = 0; index < kManyContexts; ++index) {
    contexts[index]->drain();
  }
  // The lifecycle closes every context before the first check, so a failed
  // check leaves no context thread running behind it.
  for (std::size_t index = 0; index < kManyContexts; ++index) {
    contexts[index]->requestStop();
    contexts[index]->join();
    contexts[index].reset();
  }

  for (std::size_t index = 0; index < kManyContexts; ++index) {
    check(batch.at(index).state() == sg::WorkNodeState::DONE,
          "every node dispatched to a live context settles DONE (FR-001)");
    check(slots[index] == kSentinelValue,
          "every live context ran the node dispatched to it (FR-001)");
  }
  for (std::size_t first = 0; first < kManyContexts; ++first) {
    for (std::size_t second = first + 1; second < kManyContexts; ++second) {
      check(ids[first] != ids[second],
            "two live contexts name distinct ids (FR-002)");
    }
  }
  batch.destroyAll();
  std::puts("harness_execution_context_test manyContexts: ok");
  return 0;
}

// SC-02, FR-004: the YIELD wait of the loop. An empty drain stands the loop
// in its policy's wait with the inbox empty, and each dispatch that follows is
// the wake that ends it: the item running and the flush returning are the two
// observable halves of that wait (SC-02, FR-004, FR-006).
auto scenarioYieldIdleWake() -> int
{
  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::YIELD,
                                  .placement = sg::PlacementSlot::noPin()};
  sg::ExecutionContext context {policy};
  check(context.policy().waitMode == sg::WaitMode::YIELD,
        "the YIELD policy stands fixed and readable for the life of the "
        "context (FR-007)");

  // The loop reaches its empty-inbox wait through this flush: drain() returns
  // only once the loop stands IDLE with the inbox empty (FR-006).
  context.drain();

  Observation observed {};
  const ObserveItem item {0, &observed};
  sg::WorkNode<ObserveItem> node {item};
  sg::dispatch(context, node);
  context.drain();

  check(
      node.state() == sg::WorkNodeState::DONE,
      "the dispatch woke the yielding loop and its node ran (SC-02, " "FR-"
                                                                      "004)");
  check(runCount[0] == 1, "the item ran once (FR-001)");
  check(observed.threadId != std::this_thread::get_id(),
        "the item ran on the thread of the context (FR-001)");
  check(observed.currentSerial == context.id().serial,
        "the item named its context (FR-002)");

  // The woken loop waits again after the flush, and the second dispatch wakes
  // that wait too (SC-02, FR-004).
  Observation later {};
  const ObserveItem laterItem {1, &later};
  sg::WorkNode<ObserveItem> laterNode {laterItem};
  context.drain();
  sg::dispatch(context, laterNode);
  context.drain();
  check(laterNode.state() == sg::WorkNodeState::DONE,
        "the loop stood in its wait again and woke for the next dispatch "
        "(SC-02, FR-004)");

  closeContext(context);
  check(runPositions == 2,
        "the close ran nothing beyond the two dispatched items (FR-006)");
  std::puts("harness_execution_context_test yieldIdleWake: ok");
  return 0;
}

// FR-004, FR-006: the BLOCK wait of the loop. The empty flush stands the loop
// in the standard atomic wait on its inbox flag, and the two wakes that wait
// names (a dispatch's notify and the stop request) each end it: the accepted
// node running and the loop closing are the accounts read back (FR-004,
// FR-006, FR-050).
auto scenarioBlockIdleWake() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  check(
      context.policy().waitMode == sg::WaitMode::BLOCK,
      "a caller-created context with no stated mode waits in BLOCK " "(FR-"
                                                                     "004)");
  context.drain();

  Observation observed {};
  const ObserveItem item {0, &observed};
  sg::WorkNode<ObserveItem> node {item};
  sg::dispatch(context, node);
  context.drain();
  check(
      node.state() == sg::WorkNodeState::DONE,
      "the dispatch notify woke the blocked loop and its node ran " "(FR-004)");
  check(runCount[0] == 1, "the item ran once (FR-001)");

  // The stop request is the other wake of the same wait: the loop leaves it
  // with the inbox empty and closes (FR-006, FR-050).
  Observation later {};
  const ObserveItem laterItem {1, &later};
  sg::WorkNode<ObserveItem> laterNode {laterItem};
  context.drain();
  sg::dispatch(context, laterNode);
  context.requestStop();
  context.drain();
  check(laterNode.state() == sg::WorkNodeState::DONE,
        "the node accepted before the stop request ran during drain "
        "(FR-006, FR-050)");
  context.join();
  check(runPositions == 2,
        "the close ran nothing beyond the two dispatched items (FR-006)");
  std::puts("harness_execution_context_test blockIdleWake: ok");
  return 0;
}

// FR-014: one thread waits a round while the second arrival stands behind a
// flag. The waiter runs the arrival on a YIELD-policy context, so its own
// policy names the yield path of the round wait, and its arrival cannot
// complete the round it enters: the round stands full of one participant
// until the caller arrives. Nothing releasing before that arrival is the
// wait's observable half, and the item completing after the caller's arrival
// is the wake's (FR-014, Q-3).
namespace
{
struct LateArriveItem
{
  std::atomic<bool>* arrived = nullptr;
  std::atomic<bool>* released = nullptr;
  std::atomic<bool>* sawContext = nullptr;
  sg::ContextBarrier* barrier = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    sawContext->store(sg::currentContext().has_value(),
                      std::memory_order_release);
    arrived->store(true, std::memory_order_release);
    barrier->arriveAndWait();
    released->store(true, std::memory_order_release);
  }
};
}  // namespace

auto scenarioBarrierLateArrival() -> int
{
  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::YIELD,
                                  .placement = sg::PlacementSlot::noPin()};
  auto& context = ownedContext(policy);
  check(context.policy().waitMode == sg::WaitMode::YIELD,
        "the participant's own policy names the yield wait mode it must wait "
        "in (FR-007, FR-014)");
  sg::ContextBarrier barrier {2};
  std::atomic<bool> arrived {false};
  std::atomic<bool> released {false};
  std::atomic<bool> sawContext {false};
  const LateArriveItem item {&arrived, &released, &sawContext, &barrier};
  sg::WorkNode<LateArriveItem> node {item};
  sg::dispatch(context, node);

  // The same gate as the stop race's pending gate: the item runs on the
  // thread of the context, and its arrival stands inside running work.
  while (!arrived.load(std::memory_order_acquire)) {
  }
  // Only the arrival below can complete this round, so the item stands in
  // its round wait at this point: a participant proceeds only once every
  // participant has arrived (FR-014).
  check(!released.load(std::memory_order_acquire),
        "no participant proceeds while a participant of the round stands "
        "unarrived (FR-014)");
  barrier.arriveAndWait();
  context.requestStop();
  context.drain();
  context.join();
  check(released.load(std::memory_order_acquire),
        "the arrival that fills the round releases every participant at once "
        "(FR-014)");
  check(node.state() == sg::WorkNodeState::DONE,
        "the item that rode the round completed (FR-009, FR-015)");
  check(sawContext.load(std::memory_order_acquire),
        "the participant waited from inside running work, where the policy of "
        "its own context names the mode (FR-002, FR-014)");
  std::puts("harness_execution_context_test barrierLateArrival: ok");
  return 0;
}

// FR-007, FR-014: a participant whose round stays open waits by policy over
// and over until the arrival that fills the round lands.
auto scenarioBarrierRepeatedWaits() -> int
{
  constexpr unsigned kBusyPasses = 4'000'000U;

  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::YIELD,
                                  .placement = sg::PlacementSlot::noPin()};
  auto& context = ownedContext(policy);
  sg::ContextBarrier barrier {2};
  std::atomic<bool> arrived {false};
  std::atomic<bool> released {false};
  std::atomic<bool> sawContext {false};
  const LateArriveItem item {&arrived, &released, &sawContext, &barrier};
  sg::WorkNode<LateArriveItem> node {item};
  sg::dispatch(context, node);

  while (!arrived.load(std::memory_order_acquire)) {
  }
  // The window between the arrival and the release is where the waiting
  // thread takes its mode step, tests the round, and steps again.
  volatile unsigned sink = 0;
  for (unsigned passes = 0; passes < kBusyPasses; ++passes) {
    sink += passes;
  }
  check(sink != 0U,
        "the window between the arrival and the release ran to its end "
        "(FR-007, FR-014)");
  check(!released.load(std::memory_order_acquire),
        "the participant stands in its round wait across the whole window "
        "while one arrival is missing (FR-014)");
  barrier.arriveAndWait();
  context.requestStop();
  context.drain();
  context.join();
  check(released.load(std::memory_order_acquire),
        "the arrival that fills the round releases the thread that waited "
        "through repeated mode steps (FR-007, FR-014)");
  check(node.state() == sg::WorkNodeState::DONE,
        "the item that rode the round completed (FR-009, FR-015)");
  check(sawContext.load(std::memory_order_acquire),
        "the participant waited from inside running work, where the policy of "
        "its own context names the mode (FR-002, FR-014)");
  std::puts("harness_execution_context_test barrierRepeatedWaits: ok");
  return 0;
}

// FR-006, FR-050: the enqueue is the single linearization point, so the window
// between one dispatch's accept and its inbox link shows in the account a
// caller can read: every dispatched node reaches one terminal state, the
// drain's empty proof runs every node the accept counted, and a node the run
// never took reads `STOPPED` from the hand that refused it. The window spans
// two adjacent stores of one wait-free enqueue, so no scenario holds a thread
// inside it. The witness here is three sets of nodes under one stop request,
// and every check below holds on every schedule. The set dispatched before the
// request stands on the accepted side and the set dispatched after it returned
// on this thread stands on the refused side, so both sides of the
// linearization stand proven by states no schedule decides. The set that races
// the request lands on one side or the other, and the schedule decides only
// the second half of that set: the scenario holds the request back until the
// producer has posted half its batch, so each node of the first half stands
// accepted before the request enters the order and the drain owes it a run
// while the request stands and later enqueues are still landing. Each check
// reads the side back from the node and matches it against the item count the
// run settled. No read after the close can reach the instant between the two
// stores of one enqueue, so nothing here claims that witness. What the
// scenario does prove is the rule FR-050 states: the terminal state and the
// item count of every node agree with the one side its enqueue took, and the
// drain's stand let none of them through unrun.
auto scenarioAcceptedNotLinked() -> int
{
  // The three sets keep their tag ranges apart inside `kRunCapacity`, so one
  // sum over `runCount` is the run's own account of the items it ran.
  constexpr std::size_t kRacedBase = kBatchNodes;
  constexpr std::size_t kRefusedBase = 3 * kBatchNodes;
  // The count the producer posts before the request enters the order.
  constexpr std::size_t kRacedHalf = kBatchNodes / 2;

  sg::ExecutionContext context {blockingPolicy()};
  // The flush stands the loop in its empty-inbox wait (FR-006).
  context.drain();

  NodeBatch<NoAllocItem, kBatchNodes> accepted;
  NodeBatch<NoAllocItem, kBatchNodes> raced;
  NodeBatch<NoAllocItem, kBatchNodes> refused;

  // Each dispatch of this set returns before the stop request stands on this
  // thread, so each node is accepted, linked and counted before the request
  // enters the order (FR-050).
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    sg::dispatch(context, accepted.construct(index, NoAllocItem {index}));
  }

  // The producer enqueues its set across the request, so each of those
  // enqueues takes its side of the linearization on its own place in the
  // order, and two threads stand on one inbox the way FR-013 gives it.
  std::atomic<bool> start {false};
  std::atomic<int> ready {0};
  std::atomic<std::size_t> posted {0};
  std::thread producer {
      [&]
      {
        ++ready;
        while (!start.load()) {
        }
        for (std::size_t index = 0; index < kBatchNodes; ++index) {
          sg::dispatch(
              context,
              raced.construct(index, NoAllocItem {kRacedBase + index}));
          posted.fetch_add(1, std::memory_order_release);
        }
      }};
  while (ready.load() < 1) {
  }
  start.store(true);
  // Each post follows its dispatch's return, so the acquire read stands each
  // of those nodes linked and counted before the request enters the order, and
  // the request reaches the accept of none of them. The producer posts its
  // whole batch, so this watch ends on the producer's own progress.
  while (posted.load(std::memory_order_acquire) < kRacedHalf) {
  }
  context.requestStop();

  // The request returned on this thread before each dispatch below, so each
  // one reads the stopped state and completes its own node there (FR-050).
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    sg::dispatch(context,
                 refused.construct(index, NoAllocItem {kRefusedBase + index}));
  }

  // Every dispatch of the racing set has returned here, so each of its nodes
  // stands linked and counted or refused. The drain below owes a run to each
  // node the accept counted, and its empty proof reads the accept and the
  // dispatch as one account, so that stand cannot settle over a node still on
  // its way into the chain (FR-006, FR-050). The reads that follow stand after
  // `join`, so the joined loop thread is what carries its every write to them
  // (FR-049).
  producer.join();
  context.drain();
  context.join();

  std::size_t doneNodes = 0;
  std::size_t stoppedNodes = 0;

  // The set the request found accepted: FR-050 gives its delivery to the run
  // and FR-006 says the trio ran each node exactly once.
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    const sg::WorkNodeState state = accepted.at(index).state();
    check(state == sg::WorkNodeState::DONE,
          "a node accepted before the stop request settles DONE across the "
          "race (FR-006, FR-050)");
    check(runCount[index] == 1,
          "each node accepted before the stop request ran exactly once "
          "(FR-006, SC-03)");
    ++doneNodes;
  }

  // The set that raced the request: each node reads one side of the
  // linearization and the item count matches the side it reads. A node that
  // reads `PENDING` here is a drain that settled its empty proof over an
  // accept the count had not matched, and a node that reads `RUNNING` is a
  // loop that ran past its join (FR-009, FR-050).
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    const sg::WorkNodeState state = raced.at(index).state();
    const bool ran = state == sg::WorkNodeState::DONE;
    check(ran || state == sg::WorkNodeState::STOPPED,
          "a node that raced the stop request settles to one terminal state "
          "(FR-009, FR-050)");
    check(runCount[kRacedBase + index] == (ran ? 1 : 0),
          "a node that raced the stop request ran once on the accepted side "
          "and never on the refused side (FR-050)");
    if (index < kRacedHalf) {
      check(ran,
            "a node posted to the inbox before the request entered the order "
            "runs during the drain (FR-006, FR-050)");
    }
    if (ran) {
      ++doneNodes;
    } else {
      ++stoppedNodes;
    }
  }

  // The set the request had already refused: its dispatcher completed each
  // node itself and the loop never reached it (FR-050).
  for (std::size_t index = 0; index < kBatchNodes; ++index) {
    const sg::WorkNodeState state = refused.at(index).state();
    check(state == sg::WorkNodeState::STOPPED,
          "a dispatch that observes the stopped state completes its node "
          "STOPPED (FR-050)");
    check(runCount[kRefusedBase + index] == 0,
          "the loop never touches a node it never accepted (FR-050)");
    ++stoppedNodes;
  }

  // The two accounts close on each other: one item per node the run took, none
  // for a node it refused, and the two terminal states over every node the run
  // was offered (FR-006, FR-050).
  std::size_t itemsRan = 0;
  for (std::size_t tag = 0; tag < kRunCapacity; ++tag) {
    itemsRan += runCount[tag];
  }
  check(itemsRan == doneNodes,
        "the run settled one item for each node it took and none for a node it "
        "refused (FR-006, FR-050)");
  check(doneNodes + stoppedNodes == 3 * kBatchNodes,
        "the two terminal states account for every node the run was offered "
        "(FR-050)");

  // The checks above read every node at a terminal state, so each destructor
  // below stands at the point FR-012 allows. A node the run had left queued
  // trips that contract at this call, which ends the child by SIGABRT and the
  // parent reads that as the failure of the scenario (FR-012, FR-049).
  accepted.destroyAll();
  raced.destroyAll();
  refused.destroyAll();

  std::puts("harness_execution_context_test acceptedNotLinked: ok");
  return 0;
}

// FR-014, Q-3: a participant that runs on a blocked context waits its round
// in `BLOCK`, on the generation's futex, and every participant proceeds when
// the last arrival fills the round (FR-004, FR-014).
auto scenarioBarrierBlockMode() -> int
{
  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::BLOCK,
                                  .placement = sg::PlacementSlot::noPin()};
  auto& context = ownedContext(policy);
  check(context.policy().waitMode == sg::WaitMode::BLOCK,
        "the participant's own policy names the blocking wait mode it waits "
        "in (FR-007, FR-014)");
  sg::ContextBarrier barrier {2};
  std::atomic<bool> arrived {false};
  std::atomic<bool> released {false};
  std::atomic<bool> sawContext {false};
  const LateArriveItem item {&arrived, &released, &sawContext, &barrier};
  sg::WorkNode<LateArriveItem> node {item};
  sg::dispatch(context, node);

  while (!arrived.load(std::memory_order_acquire)) {
  }
  check(!released.load(std::memory_order_acquire),
        "the blocked participant stands in its round while the other "
        "participant stands unarrived (FR-014)");
  barrier.arriveAndWait();
  context.requestStop();
  context.drain();
  context.join();
  const bool done = node.state() == sg::WorkNodeState::DONE;
  check(released.load(std::memory_order_acquire) && done && sawContext.load(
                                                              std::memory_order_acquire),
        "the arrival that fills the round releases the blocked participant, "
        "the item completes, and the wait stood inside running work where the "
        "policy of its own context names the mode (FR-002, FR-009, FR-014)");
  std::puts("harness_execution_context_test barrierBlockMode: ok");
  return 0;
}

// FR-006, FR-050: the stand of a drain reads the tail of the walk before the
// phase, so a node that still waits in the inbox keeps the drain standing even
// while the context stands between two runs (FR-006).
namespace
{
struct GateItem
{
  std::atomic<bool>* entered = nullptr;
  std::atomic<bool>* gate = nullptr;
  std::atomic<std::size_t>* runs = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    entered->store(true, std::memory_order_release);
    while (!gate->load(std::memory_order_acquire)) {
    }
    runs->fetch_add(1, std::memory_order_release);
  }
};

struct FollowItem
{
  std::atomic<std::size_t>* runs = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    runs->fetch_add(1, std::memory_order_release);
  }
};

// The item of the enqueue-window run: it counts its own runs and stands no
// gate, so the walk keeps its pace beside the producer.
struct WindowItem
{
  std::atomic<std::size_t>* runs = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    runs->fetch_add(1, std::memory_order_release);
  }
};
}  // namespace

auto scenarioDrainPendingInbox() -> int
{
  auto& context = ownedContext(blockingPolicy());
  // The flush stands the loop in its empty-inbox wait (FR-006).
  context.drain();

  std::atomic<bool> entered {false};
  std::atomic<bool> gate {false};
  std::atomic<std::size_t> runs {0};
  const GateItem held {&entered, &gate, &runs};
  const FollowItem follow {&runs};
  sg::WorkNode<GateItem> heldNode {held};
  sg::WorkNode<FollowItem> followNode {follow};

  // The held item stands the context at work and the follow node waits in the
  // inbox behind it, so the tail of the walk names no sentinel (FR-006).
  sg::dispatch(context, heldNode);
  while (!entered.load(std::memory_order_acquire)) {
  }
  sg::dispatch(context, followNode);

  std::atomic<bool> standing {false};
  std::atomic<bool> returned {false};
  std::thread waiter {[&]
                      {
                        standing.store(true, std::memory_order_release);
                        context.drain();
                        returned.store(true, std::memory_order_release);
                      }};
  while (!standing.load(std::memory_order_acquire)) {
  }
  // No schedule returns from a drain that owes a run: the node the inbox
  // holds stands until the gate opens, so the stand below can only be the
  // predicate reading the tail. A thread still on its way here cannot
  // produce it (FR-006, FR-050).
  check(!returned.load(std::memory_order_acquire),
        "the drain stands while a node still waits in the inbox (FR-006, "
        "FR-050)");

  gate.store(true, std::memory_order_release);
  waiter.join();
  const bool both = heldNode.state() == sg::WorkNodeState::DONE
      && followNode.state() == sg::WorkNodeState::DONE;
  check(returned.load(std::memory_order_acquire) && runs.load(
                                                          std::memory_order_acquire)
                                                      == 2
            && both,
        "the drain returned once both nodes had run, in the order they "
        "arrived (FR-006, FR-009, FR-015)");
  context.requestStop();
  context.drain();
  context.join();
  std::puts("harness_execution_context_test drainPendingInbox: ok");
  return 0;
}

// FR-009, FR-012: a node whose run has ended may be dispatched again, and its
// state is settled PENDING before the inbox link becomes visible, so a node
// that waits in the inbox never reads DONE while the loop can still take it.
auto scenarioReuseAfterDone() -> int
{
  constexpr std::size_t kReusedTag = 40;
  sg::ExecutionContext context {blockingPolicy()};
  std::atomic<bool> released {false};
  const NoAllocItem item {kReusedTag};
  sg::WorkNode<NoAllocItem> node {item};
  const ReleaseItem hold {&released};
  sg::WorkNode<ReleaseItem> holdNode {hold};

  sg::dispatch(context, node);
  check(settled(node) == sg::WorkNodeState::DONE,
        "the node settles DONE on its first run (FR-009)");
  check(runCount[kReusedTag] == 1,
        "the item ran once before the node was reused (FR-009)");

  // The loop stands inside the held item, so the reused node stays queued
  // while the caller reads its state.
  sg::dispatch(context, holdNode);
  watchRunning(holdNode);

  sg::dispatch(context, node);
  check(node.state() == sg::WorkNodeState::PENDING,
        "a reused node reads PENDING while it stands queued (FR-009, FR-012)");

  released.store(true, std::memory_order_release);
  context.drain();
  check(node.state() == sg::WorkNodeState::DONE,
        "the reused node settles DONE on its second run (FR-009)");
  check(runCount[kReusedTag] == 2,
        "the reused item ran a second time (FR-010)");

  closeContext(context);
  std::puts("harness_execution_context_test reuseAfterDone: ok");
  return 0;
}

// FR-001, FR-006: an enqueue that has swapped the tail and has yet to write the
// link of the sentinel is waited out by the walk with the pause hint. The
// window spans two adjacent stores of one wait-free enqueue, so no scenario can
// hold a thread between them: the witness is a drain that walks beside a
// producer across many enqueues, and the assertion is that every node of every
// one of them runs once (FR-001, FR-006, FR-009).
auto scenarioEnqueueWindow() -> int
{
  auto& context = ownedContext(spinningPolicy());
  // The flush stands the loop in its empty-inbox wait (FR-006).
  context.drain();

  constexpr std::size_t kEnqueues = 4000;
  std::atomic<std::size_t> runs {0};
  std::vector<std::unique_ptr<WindowItem>> items;
  std::vector<std::unique_ptr<sg::WorkNode<WindowItem>>> nodes;
  items.reserve(kEnqueues);
  nodes.reserve(kEnqueues);
  for (std::size_t index = 0; index < kEnqueues; ++index) {
    items.push_back(std::make_unique<WindowItem>(WindowItem {&runs}));
    nodes.push_back(std::make_unique<sg::WorkNode<WindowItem>>(*items.back()));
  }

  std::atomic<bool> producerDone {false};
  std::thread producer {[&]
                        {
                          for (auto& node : nodes) {
                            sg::dispatch(context, *node);
                          }
                          producerDone.store(true, std::memory_order_release);
                        }};
  // The drain walks while the producer enqueues, so the walk meets enqueues
  // that stand between their exchange and their link. The assertion below
  // holds whatever the walk meets. The queue owes one run per node, and the
  // schedule decides only when each run happens (FR-001, FR-006).
  while (!producerDone.load(std::memory_order_acquire)) {
    context.drain();
  }
  producer.join();
  context.drain();

  std::size_t settled = 0;
  for (const auto& node : nodes) {
    settled += node->state() == sg::WorkNodeState::DONE ? 1U : 0U;
  }
  check(runs.load(std::memory_order_acquire) == kEnqueues,
        "each node of the run runs once beside the window (FR-001, FR-006)");
  check(settled == kEnqueues, "every node of the run reads DONE (FR-009)");
  closeContext(context);
  std::puts("harness_execution_context_test enqueueWindow: ok");
  return 0;
}

// SC-03, FR-006: the destructor requires a joined context.
auto breachUnjoined() -> int
{
  {
    // The context starts its thread here and stands unjoined when the scope
    // ends, so its destructor is the violation.
    sg::ExecutionContext context {blockingPolicy()};
    (void)context;
  }
  check(false,
        "the destructor of a context that was never joined is a contract "
        "violation (SC-03, FR-006)");
  return 1;
}

// FR-012: destroying a node while the loop runs it is a contract violation
// under the enforce semantic.
auto breachLiveNode() -> int
{
  sg::ExecutionContext context {blockingPolicy()};
  std::atomic<bool> released {false};
  alignas(sg::WorkNode<ReleaseItem>)
      std::array<std::byte, sizeof(sg::WorkNode<ReleaseItem>)>
          bytes {};
  auto* node =
      new (bytes.data()) sg::WorkNode<ReleaseItem> {ReleaseItem {&released}};

  // The item holds the node in RUNNING until this flag stands, and the flag
  // stands only after the destructor below, so on every schedule the loop
  // stands inside the item while the caller observes RUNNING and destroys
  // the node: the observation cannot be missed and the destructor reads a
  // live node.
  sg::dispatch(context, *node);
  watchRunning(*node);
  node->~WorkNode();
  // The enforce semantic ends the process at the line above. This point is
  // reached only where the semantic is compiled out: release the item, exit
  // the child non-zero so the parent reports the missed breach, and leave
  // the item's thread nothing to spin on.
  released.store(true, std::memory_order_release);
  check(false, "destroying a live node is a contract violation (FR-012)");
  return 1;
}

// Edge Cases, FR-006: join from the thread of the context itself.
auto breachJoinOwnThread() -> int
{
  auto& context = ownedContext(spinningPolicy());
  runningContext = &context;
  // The flag crosses threads for the same reason as the drain breach's, so it
  // stands atomic against the elided wait.
  std::atomic<bool> returned {false};
  const SelfJoinItem item {&returned};
  sg::WorkNode<SelfJoinItem> node {item};
  sg::dispatch(context, node);
  // The breach ends the process on the context's thread. The wait is on the
  // call coming back, so the abort always arrives first where the contract is
  // enforced.
  while (!returned.load(std::memory_order_acquire)) {
  }
  runningContext = nullptr;
  check(
      false,
      "join from the thread of the context is a contract violation " "(FR-"
                                                                     "006)");
  return 1;
}

// T008, FR-006, FR-050: drain from the thread of the context itself trips the
// debug precondition.
auto breachOwnThreadDrain() -> int
{
  auto& context = ownedContext(spinningPolicy());
  runningContext = &context;
  // The item sets this flag from the context's thread, so it crosses threads: a
  // plain bool would make the wait below a loop with no visible side effect,
  // which is elidable undefined behaviour that GCC at -O3 does elide.
  std::atomic<bool> returned {false};
  const SelfDrainItem item {&returned};
  sg::WorkNode<SelfDrainItem> node {item};
  sg::dispatch(context, node);
  while (!returned.load(std::memory_order_acquire)) {
  }
  runningContext = nullptr;
  check(false,
        "drain from the thread of the context is a contract violation "
        "(T008, FR-050)");
  return 1;
}

// SC-05, FR-005: a cpu index the host lacks is a violation at the call. The
// default placement function builds a cpu set, so the index stays inside the
// bound of that set on any host with room for it.
auto breachBadPlacement() -> int
{
  const long online = sysconf(_SC_NPROCESSORS_ONLN);
  if (online <= 0) {
    fail("the host reports no online cpus, so no index can name one it lacks");
  }
  const auto absent = static_cast<std::uint32_t>(online + 4);
  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::BLOCK,
                                  .placement = sg::PlacementSlot::cpu(absent)};
  const sg::ExecutionContext context {policy};
  (void)context;
  check(false, "a cpu index the host lacks ends the process at the "
               "construction call (SC-05, FR-005)");
  return 1;
}

// FR-006, FR-050: the end test of a context loop and the dispatch account
// under it. Each outcome of both tests stands here: a loop that reaches the
// empty inbox before the stop turned it stands open, a stopped loop with a
// settled account ends, and a stopped loop with an accept that still owes its
// link stands open.
auto scenarioSettleAccount() -> int
{
  check(!sg::detail::loopEnds(false, 3, 3),
        "a loop the stop has not reached has no end to make (FR-006)");
  check(sg::detail::loopEnds(true, 3, 3),
        "a stopped loop with a settled account ends it (FR-006, FR-050)");
  check(!sg::detail::loopEnds(true, 4, 3),
        "an accept that still owes its link holds the loop open (FR-050)");
  check(sg::detail::countsSettled(7, 7),
        "equal counts state a settled account (FR-050)");
  check(!sg::detail::countsSettled(7, 6),
        "an accept apart from its dispatch states an open account (FR-050)");
  std::puts("harness_execution_context_test settle-account: ok");
  return 0;
}

struct Mode
{
  const char* name;
  auto (*run)() -> int;
};

constexpr std::array<Mode, 26> kScenarios {
    Mode {.name = "identity", .run = &scenarioIdentity},
    Mode {.name = "waitModes", .run = &scenarioWaitModes},
    Mode {.name = "lifecycle", .run = &scenarioLifecycle},
    Mode {.name = "order", .run = &scenarioOrder},
    Mode {.name = "nested", .run = &scenarioNested},
    Mode {.name = "noalloc", .run = &scenarioNoAlloc},
    Mode {.name = "compileRefusal", .run = &scenarioCompileRefusal},
    Mode {.name = "placement", .run = &scenarioPlacement},
    Mode {.name = "stopRace", .run = &scenarioStopRace},
    Mode {.name = "acceptedThenDrained", .run = &scenarioAcceptedThenDrained},
    Mode {.name = "refusedAfterStop", .run = &scenarioRefusedAfterStop},
    Mode {.name = "settle-account", .run = &scenarioSettleAccount},
    Mode {.name = "drainWithoutStop", .run = &scenarioDrainWithoutStop},
    Mode {.name = "completionOrder", .run = &scenarioCompletionOrder},
    Mode {.name = "joinTwice", .run = &scenarioJoinTwice},
    Mode {.name = "stopTokenOffContext", .run = &scenarioStopTokenOffContext},
    Mode {.name = "manyContexts", .run = &scenarioManyContexts},
    Mode {.name = "reuseAfterDone", .run = &scenarioReuseAfterDone},
    Mode {.name = "yieldIdleWake", .run = &scenarioYieldIdleWake},
    Mode {.name = "blockIdleWake", .run = &scenarioBlockIdleWake},
    Mode {.name = "barrierLateArrival", .run = &scenarioBarrierLateArrival},
    Mode {.name = "barrierBlockMode", .run = &scenarioBarrierBlockMode},
    Mode {.name = "barrierRepeatedWaits", .run = &scenarioBarrierRepeatedWaits},
    Mode {.name = "drainPendingInbox", .run = &scenarioDrainPendingInbox},
    Mode {.name = "acceptedNotLinked", .run = &scenarioAcceptedNotLinked},
    Mode {.name = "enqueueWindow", .run = &scenarioEnqueueWindow},
};

constexpr std::array<Mode, 5> kBreaches {
    Mode {.name = "unjoined", .run = &breachUnjoined},
    Mode {.name = "liveNode", .run = &breachLiveNode},
    Mode {.name = "joinOwnThread", .run = &breachJoinOwnThread},
    Mode {.name = "ownThreadDrain", .run = &breachOwnThreadDrain},
    Mode {.name = "badPlacement", .run = &breachBadPlacement},
};

}  // namespace execution_context_test

#if !SG_TEST_HAS_TSAN
using execution_context_test::allocate;
using execution_context_test::allocateAligned;
using execution_context_test::allocateAlignedNothrow;
using execution_context_test::allocateNothrow;

// Every replaceable form is replaced, so no shape of allocation reaches the
// heap uncounted while the hooks are armed (SC-04, FR-040). Each new pairs with
// a delete that returns the same storage.
auto operator new(std::size_t size) -> void*
{
  return allocate(size);
}

auto operator new[](std::size_t size) -> void*
{
  return allocate(size);
}

auto operator new(std::size_t size, const std::nothrow_t&) noexcept -> void*
{
  return allocateNothrow(size);
}

auto operator new[](std::size_t size, const std::nothrow_t&) noexcept -> void*
{
  return allocateNothrow(size);
}

auto operator new(std::size_t size, std::align_val_t align) -> void*
{
  return allocateAligned(size, static_cast<std::size_t>(align));
}

auto operator new[](std::size_t size, std::align_val_t align) -> void*
{
  return allocateAligned(size, static_cast<std::size_t>(align));
}

auto operator new(std::size_t size,
                  std::align_val_t align,
                  const std::nothrow_t&) noexcept -> void*
{
  return allocateAlignedNothrow(size, static_cast<std::size_t>(align));
}

auto operator new[](std::size_t size,
                    std::align_val_t align,
                    const std::nothrow_t&) noexcept -> void*
{
  return allocateAlignedNothrow(size, static_cast<std::size_t>(align));
}

auto operator delete(void* block) noexcept -> void
{
  std::free(block);
}

auto operator delete[](void* block) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, std::size_t) noexcept -> void
{
  std::free(block);
}

auto operator delete[](void* block, std::size_t) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, const std::nothrow_t&) noexcept -> void
{
  std::free(block);
}

auto operator delete[](void* block, const std::nothrow_t&) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, std::align_val_t) noexcept -> void
{
  std::free(block);
}

auto operator delete[](void* block, std::align_val_t) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block,
                     std::size_t,
                     std::align_val_t) noexcept -> void
{
  std::free(block);
}

auto operator delete[](void* block,
                       std::size_t,
                       std::align_val_t) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block,
                     std::align_val_t,
                     const std::nothrow_t&) noexcept -> void
{
  std::free(block);
}

auto operator delete[](void* block,
                       std::align_val_t,
                       const std::nothrow_t&) noexcept -> void
{
  std::free(block);
}
#endif  // !SG_TEST_HAS_TSAN

auto main(const int argc, char** argv) -> int
{
  using execution_context_test::check;
  using execution_context_test::kBreaches;
  using execution_context_test::kScenarios;
  using execution_context_test::runChild;

  const std::string argument = argc > 1 ? argv[1] : "";
  for (const auto& scenario : kScenarios) {
    if (argument == scenario.name) {
      return scenario.run();
    }
  }
  for (const auto& breach : kBreaches) {
    if (argument == breach.name) {
      return breach.run();
    }
  }
  if (!argument.empty()) {
    (void)std::fprintf(stderr, "unknown mode: %s\n", argument.c_str());
    return 2;
  }

  const char* const self = argv[0];
  for (const auto& scenario : kScenarios) {
    const int status = runChild(self, scenario.name, false);
    const std::string message =
        std::string("the scenario passes: ") + scenario.name;
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, message.c_str());
  }

  // Each breach ends its child through the contract facility, which aborts. The
  // parent forks and execs directly and reads the signal from waitpid, the
  // pattern harness_pause_test.cpp carries for the same reason.
  for (const auto& breach : kBreaches) {
    const int status = runChild(self, breach.name, true);
    const std::string message =
        std::string("the breach ends its child by SIGABRT: ") + breach.name;
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, message.c_str());
  }

  std::puts("harness_execution_context_test: ok");
  return 0;
}
