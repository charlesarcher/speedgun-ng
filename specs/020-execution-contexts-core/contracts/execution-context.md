# Contract: The execution context surface

`include/speedgun-ng/execution_context.hpp` (new, compiled into the
`speedgun-ng_harness` archive; no new target, FR-043) ships the
context core. All names live in namespace `sg` (FR-001).

## The surface (FR-001 to FR-015, FR-045)

```cpp
struct ContextId;                                   // FR-002
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

class ExecutionContext                              // FR-001
{
public:
  ExecutionContext();                               // default policy
  explicit ExecutionContext(ContextPolicy policy);
  ExecutionContext(ExecutionContext&&) noexcept;    // movable
  // destructor: SG_REQUIRE the context is joined (FR-006)
  [[nodiscard]] auto id() const noexcept -> ContextId;
  [[nodiscard]] auto policy() const noexcept -> ContextPolicy;  // FR-007
  auto requestStop() -> void;                       // FR-006
  auto drain() -> void;                             // FR-006
  auto join() -> void;                              // FR-006
};

class WorkContext;                                  // FR-011
template <class Item> class WorkNode;               // FR-009
concept WorkItem = /* noexcept invocable with WorkContext& */;  // FR-008

template <WorkItem Item>
auto dispatch(ExecutionContext& context, WorkNode<Item>& node)
    -> void;                                        // FR-010

class ContextBarrier                                // FR-014
{
public:
  explicit ContextBarrier(int participants);
  auto arriveAndWait() -> void;                     // FR-014, FR-015
};
```

## Rules

| Rule | Requirement |
| --- | --- |
| Constructing an `ExecutionContext` shall start one thread running one event loop that takes work only through that context's inbox. | FR-001 |
| The inbox shall be an intrusive multi-producer single-consumer queue whose link lives in the work node; enqueueing shall allocate nothing. | FR-003 |
| `currentContext()` shall report the identity of the context the calling thread runs and report no context on any other thread. | FR-002 |
| A context the harness creates to run a benchmark shall default to `SPIN`; a caller-created context with no policy shall default to `BLOCK`. | FR-004, Q-2 |
| A wait mode shall wait without reading a clock or taking a timeout; `BLOCK` waits in the standard atomic wait and `dispatch()` wakes it. | FR-004, FR-039 |
| A context shall apply its placement once at thread start through one replaceable placement function; the default applies the cpu index and the tests replace the function. | FR-005 |
| A policy shall be fixed at creation and readable for the context's life; a cpu index the host lacks is an `SG_REQUIRE` violation at the call. | FR-005, FR-007 |
| The lifecycle shall be `requestStop()`, `drain()`, `join()` in that order; a node enqueued before the stop request runs during `drain()`; no node runs after `join()` returns; the destructor requires a joined context and `join()` from the context's own thread is a contract violation. | FR-006 |
| A work item shall be a `noexcept` invocable with the work context; an unapproved type shall be refused at compile time before the program builds. | FR-008 |
| A work node shall live in caller storage, hold its completion state (`pending`, then `done` or `stopped`), and carry its typed work to the loop through one erased run path; the loop shall run no run-time type test. | FR-009, D-2 |
| `dispatch()` shall hand the node to the named context, allocate nothing, and return without waiting; it shall be legal from any thread and from work already running on that context. | FR-010 |
| A running work item shall receive the identity of the context running it and a stop token reading true once `requestStop()` stood. | FR-011 |
| The caller shall keep a node alive until its state settles; destroying a live node aborts with an `SG_REQUIRE` violation in a debug build; work dispatched to a stopped context takes the `stopped` state and never runs. | FR-012 |
| Nodes one thread dispatches shall run in that order; nodes two threads dispatch at once shall run in the order the inbox accepted them; work dispatched from inside a running item shall run after that item ends, on the same context. | FR-013 |
| `ContextBarrier` shall join its fixed participant count in rounds where all participants arrive before any proceeds; one round shall release all participants at once; a participant shall wait in the wait mode of its own policy; `std::barrier` stands as the fallback for `BLOCK`. | FR-014, Q-3 |
| The barrier shall be legal from inside running work and outside it; the barrier shall read no clock and no round shall time out; a participant that leaves a round early is a contract violation. | FR-014, FR-015 |
| The completion state, the barrier, the stop request, and the placement slot shall ship with the fixed names and meanings H16 reads. | FR-045 |

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
