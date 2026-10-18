# Contract: The execution context surface

`include/speedgun-ng/execution_context.hpp` (new, compiled into the
`speedgun-ng_harness` archive; no new target, FR-043) ships the
context core. All names live in namespace `sg` (FR-001).

## The surface (FR-001 to FR-015, FR-045, FR-049, FR-050)

```cpp
struct ContextId;   // opaque identity: copyable, equality-comparable (FR-002, FR-045)
[[nodiscard]] auto currentContext()
    -> std::optional<ContextId>;                    // FR-002

enum class WaitMode { SPIN, YIELD, BLOCK };         // FR-004
struct PlacementSlot {                              // FR-005
  static auto noPin() -> PlacementSlot;             //   NO_PIN
  static auto cpu(std::uint32_t index) -> PlacementSlot;
};
struct ContextPolicy {                              // FR-005
  WaitMode waitMode = WaitMode::BLOCK;              // FR-004 default
  PlacementSlot placement = PlacementSlot::noPin(); // FR-005 default
};

struct PlacementResult                              // FR-005, SC-05
{                                                   // small, printable; the
  bool applied;                                     //   description field is
  std::string_view description;                     //   the disclosure record's
};                                                  //   placement: "NO_PIN", one
                                                    //   cpu index, or "UNAPPLIED"
using PlacementFn = PlacementResult (*)(const PlacementSlot& slot,
                                        std::thread::native_handle_type thread);
auto setPlacementFunction(PlacementFn function) -> void;
// One harness-level function-pointer slot, set before any context
// starts; changing it while any context runs is not thread-safe (D-04).

class ExecutionContext                              // FR-001
{
public:
  ExecutionContext();                               // default policy
  explicit ExecutionContext(ContextPolicy policy);
  // destructor: SG_REQUIRE the context is joined (FR-006)
  [[nodiscard]] auto id() const noexcept -> ContextId;
  [[nodiscard]] auto policy() const noexcept -> ContextPolicy;  // FR-007
  auto requestStop() -> void;                       // FR-006
  auto drain() -> void;                             // FR-006, FR-050
  auto join() -> void;                              // FR-006
};

concept WorkItem = /* noexcept invocable with WorkContext& */;  // FR-008

class StopToken                                     // FR-011
{
public:
  [[nodiscard]] auto stopRequested() const noexcept -> bool;
};

class WorkContext                                   // FR-011
{
public:
  [[nodiscard]] auto id() const noexcept -> ContextId;
  [[nodiscard]] auto stopToken() const noexcept -> const StopToken&;
};

enum class WorkNodeState { PENDING, RUNNING, DONE, STOPPED };  // FR-009, FR-045

template <WorkItem Item> class WorkNode             // FR-009
{
public:
  explicit WorkNode(Item item);
  // Construction stamps the one erased run path from Item (D-02).
  [[nodiscard]] auto state() const noexcept -> WorkNodeState;
  // Reads the completion state with acquire order (FR-049).
  // No wait helper: a node names no context, so a wait on it would be
  // a fourth wait mode FR-004 does not carry; the caller uses drain()
  // or state().
};

template <WorkItem Item>
auto dispatch(ExecutionContext& context, WorkNode<Item>& node)
    -> void;                                        // FR-010, FR-050

class ContextBarrier                                // FR-014
{
public:
  explicit ContextBarrier(int participants);
  auto arriveAndWait() -> void;                     // FR-014, FR-015
};
```

The three counts and their ordinals, the barrier participant count, the
thread count, the context count, and the thread and context indexes,
take plain `int`. Precedent: master's harness carries no thread-count
parameter at all (its `AVG_THREADS` flag pass divides by nothing while
threads stand out of scope, `source/harness/runner.cpp`), its only
thread-or-cpu ordinal is `counters::Target::cpu`, an `int`
(`include/speedgun-ng/counters_provider.hpp`), and Google Benchmark's
thread count is `int` as well. `PlacementSlot::cpu` keeps
`std::uint32_t` because that index feeds the counters `System`
matching.

## Rules

| Rule | Requirement |
| --- | --- |
| Constructing an `ExecutionContext` shall start one thread running one event loop that takes work only through that context's inbox. | FR-001 |
| The inbox shall be an intrusive multi-producer single-consumer queue whose link lives in the work node; enqueueing shall allocate nothing. | FR-003 |
| `currentContext()` shall report the identity of the context the calling thread runs and report no context on any other thread. | FR-002 |
| `ContextId` shall be copyable and equality-comparable, so a caller can compare the id a work item read against the id of its context; no report field prints it. | FR-002, FR-025 |
| A context the harness creates to run a benchmark shall default to `SPIN`; a caller-created context with no policy shall default to `BLOCK`. | FR-004, Q-2 |
| A wait mode shall wait without reading a clock or taking a timeout; `BLOCK` waits in the standard atomic wait and `dispatch()` wakes it. | FR-004, FR-039 |
| A context shall apply its placement once at thread start through the installed `PlacementFn`: one harness-level function-pointer slot that `setPlacementFunction` sets before any context starts, not thread-safe to change while any context runs; the default applies the cpu index and the tests install a replacement; the `PlacementResult` it returns is the printable placement field of the disclosure (contracts/report-disclosure.md). | FR-005, SC-05 |
| A policy shall be fixed at creation and readable for the context's life; a cpu index the host lacks is an `SG_REQUIRE` violation at the call. | FR-005, FR-007 |
| The lifecycle shall be `requestStop()`, `drain()`, `join()` in that order; a node accepted before the stop request runs during `drain()`; no node runs after `join()` returns; the destructor requires a joined context and `join()` from the context's own thread is a contract violation. `drain()` shall block the caller until the inbox is empty and the loop is idle, so a `drain()` with no prior `requestStop()` is a flush that leaves the loop running, and `drain()` from the context's own thread is a debug-precondition contract violation. | FR-006, FR-050 |
| A work item shall be a `noexcept` invocable with the work context; an unapproved type shall be refused at compile time before the program builds. | FR-008 |
| A work node shall live in caller storage, hold its completion state (`PENDING`, `RUNNING` while the loop runs it, then `DONE` or `STOPPED`), read it through the `noexcept` `state()` accessor with acquire order, and carry its typed work to the loop through one erased run path; the loop shall run no run-time type test. | FR-009, FR-049, D-2 |
| `dispatch()` shall hand the node to the named context, allocate nothing, and return without waiting; it shall be legal from any thread and from work already running on that context. | FR-010 |
| A running work item shall receive the identity of the context running it and a `StopToken` reading true once `requestStop()` stood; `WorkContext::stopToken()` names the type. | FR-011, FR-045 |
| The caller shall keep a node it dispatches alive until its state settles (the body nodes a threaded run builds are harness-owned, FR-021); destroying a live node aborts with an `SG_REQUIRE` violation under the enforce semantic. The completing thread — the loop for `DONE`, the refusing `dispatch()` for `STOPPED` — shall write the state with release order as its last access to the node: the loop reads the next queue link and performs any notify before that write, and after it touches only its own data, never the node. Once a reader observes `DONE` or `STOPPED`, the caller may destroy or reuse the node. | FR-012, FR-049 |
| A `dispatch()` of a node that stands in no inbox shall settle that node `PENDING` before its queue link becomes visible, so no reader finds a queued node stamped `DONE`; the dispatch of a node another dispatch still owns, one whose state is `RUNNING`, or `PENDING` while accepted, shall be a contract violation under the enforce semantic. | FR-009, FR-012 |
| `dispatch()` and `requestStop()` shall linearize at the enqueue: a dispatch observing the not-stopped state appends the node and owns delivery; a dispatch observing the stopped state refuses the node and itself completes it with `STOPPED`; the loop shall never touch a node it never accepted. | FR-010, FR-050 |
| Nodes one thread dispatches shall run in that order; nodes two threads dispatch at once shall run in the order the inbox accepted them; work dispatched from inside a running item shall run after that item ends, on the same context. | FR-013 |
| `ContextBarrier` shall join its fixed participant count in rounds where all participants arrive before any proceeds; one round shall release all participants at once; a participant shall wait in the wait mode of its own policy, or in `BLOCK` when it runs on no context; `std::barrier` stands as the fallback for `BLOCK`; a participant count of zero is an `SG_REQUIRE` violation at the construction call. | FR-014, Q-3 |
| The barrier shall be legal from inside running work and outside it; the barrier shall read no clock and no round shall time out; a participant that leaves a round early is a contract violation. | FR-014, FR-015 |
| The node completion state, `dispatch()` from inside running work, `ContextId`, and the placement slot shall ship with the fixed names and meanings H16 reads. | FR-045 |

## Consumers

`source/harness/execution_context.cpp` implements the loop, the wait
modes, the placement function, the dispatch, and the barrier;
`test/source/harness_execution_context_test.cpp` asserts the lifecycle,
the wait modes, the order rule, the compile-time refusal, the
placement seam, and the no-allocation dispatch through the allocator
hook; `test/source/harness_context_barrier_test.cpp` asserts the rounds
and the early-departure violation; the harness threaded path
(`source/harness/runner.cpp`) is the internal consumer
(contracts/threaded-runs.md).
