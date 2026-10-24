#ifndef SG_EXECUTION_CONTEXT_HPP
#define SG_EXECUTION_CONTEXT_HPP

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>
#include <utility>

#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file execution_context.hpp
 * @brief The execution context surface: a context that owns one
 * event-loop thread, the policy of its wait and its placement, the
 * typed work node a caller dispatches to it, and the barrier its work
 * items meet on.
 *
 * Feature 020-execution-contexts-core. Namespace `sg`. Every name here
 * is the name `contracts/execution-context.md` states, and the four H16
 * hooks (the node completion state, `dispatch()` from inside running
 * work, `ContextId`, the placement slot) carry the meaning H16 reads on
 * top of them (FR-045). `source/harness/execution_context.cpp` defines
 * what this header declares, inside the `speedgun-ng_harness` archive
 * and behind no new target (FR-043).
 *
 * The counts and the ordinals of this surface (a thread count, a
 * context count, a thread index, a context index, a barrier
 * participant count) take plain `int`, the spelling the shipped
 * `counters::Target::cpu` already carries (FR-001). `PlacementSlot::cpu`
 * keeps `std::uint32_t`, because that index feeds the counters `System`
 * matching (FR-005).
 */

namespace sg
{

/**
 * @brief The identity of one live execution context (FR-002, FR-045).
 *
 * One value stands per live context. A work item reads this identity
 * from its `WorkContext` and compares it against
 * `ExecutionContext::id`, which is why the type is copyable and
 * equality-comparable (FR-002). No report field prints it (FR-025).
 *
 * \invariant none
 */
struct ContextId
{
  // The ordinal that identifies this context among live contexts.
  std::uint64_t serial = 0;

  // FR-002, FR-045: the identity is equality-comparable as a type, so a
  // caller compares the id a work item read against the id of its
  // context. The comparison stands on the type and not on a field of it,
  // which is the shape the H16 hook ships.
  auto operator==(const ContextId& other) const -> bool = default;
};

// FR-002, FR-045: the rule names a comparison of the type, so the
// requirement is checked here and by the tests that read
// `.serial`.
static_assert(requires(const ContextId left, const ContextId right) {
  left == right;
});

/**
 * @brief The identity of the context the calling thread runs (FR-002).
 *
 * The context loop installs this identity for the span of one work
 * item's run and clears it after (FR-002). A thread the library runs no
 * context for reports no context.
 *
 * \pre none
 * \post none
 */
[[nodiscard]] auto currentContext() -> std::optional<ContextId>;

/**
 * @brief How a context waits while its inbox stands empty (FR-004).
 *
 * `SPIN` watches the inbox flag through the pause hint, `YIELD` yields
 * to the scheduler, `BLOCK` waits in the standard atomic wait and
 * `dispatch()` wakes it. No mode reads a clock or takes a timeout
 * (FR-004, FR-039).
 */
enum class WaitMode : std::uint8_t
{
  SPIN,
  YIELD,
  BLOCK,
};

// The placement slot value that names no cpu, so no pin applies
// (FR-005).
inline constexpr std::uint32_t kNoPin =
    std::numeric_limits<std::uint32_t>::max();

/**
 * @brief Where a context's thread runs: no pin, or one cpu index
 * (FR-005, FR-045).
 *
 * A context applies its slot once, at thread start, through the
 * installed `PlacementFn` (FR-005, D-04). A cpu index the host lacks is
 * a contract violation at the `ExecutionContext` construction call
 * (FR-005, SC-05).
 *
 * \invariant none
 */
struct PlacementSlot
{
  // The cpu index the slot names, or `kNoPin` for no pin (FR-005).
  std::uint32_t cpuIndex = kNoPin;

  /**
   * @brief The slot that names no cpu (FR-005).
   *
   * \pre none
   * \post none
   */
  static auto noPin() -> PlacementSlot { return {kNoPin}; }

  /**
   * @brief The slot that names cpu `index` (FR-005).
   *
   * The host check for that index belongs to the context that applies
   * the slot: the index must stand in the online cpu set, the set the
   * kernel owns, gaps and all (FR-005, SC-05, specs/027 FR-044).
   *
   * \pre none
   * \post none
   */
  static auto cpu(std::uint32_t index) -> PlacementSlot { return {index}; }
};

/**
 * @brief The two choices one context makes at creation, fixed for its
 * life (E-02, FR-005, FR-007).
 *
 * A caller-created context that states no policy takes `BLOCK` with no
 * pin; a context the harness creates to run a benchmark states `SPIN`
 * with no pin (FR-004, Q-2, D-05). The report discloses both fields of
 * every context of a threaded run (FR-025).
 *
 * \invariant none
 */
struct ContextPolicy
{
  WaitMode waitMode = WaitMode::BLOCK;
  PlacementSlot placement = PlacementSlot::noPin();
};

/**
 * @brief What the installed placement function did with one slot
 * (FR-005, SC-05).
 *
 * `description` is the printable placement field of the disclosure
 * record: `NO_PIN`, or one cpu index
 * (`contracts/report-disclosure.md`). The value is small and printable,
 * and the printer adds nothing to it (FR-025).
 *
 * \invariant none
 */
struct PlacementResult
{
  bool applied = false;
  std::string_view description;
};

/**
 * @brief The one placement call a context makes at thread start
 * (FR-005, D-04).
 *
 * It receives the policy's slot and the native handle of the started
 * thread, applies the slot, and returns the record the report
 * discloses. `source/harness/execution_context.cpp` ships the default:
 * one Linux pin through `pthread_setaffinity_np` on that thread. This
 * signature is the seam H10 replaces with topology placement, and
 * nothing else reads it (SC-05).
 */
using PlacementFn = PlacementResult (*)(const PlacementSlot& slot,
                                        std::thread::native_handle_type thread);

/**
 * @brief Install the placement function every context calls at thread
 * start (FR-005, D-04).
 *
 * One harness-level slot holds the function. The install stands before
 * any context starts: the call is thread-safe there, and a call while
 * any context runs has no defined ordering against that context's read
 * of the slot (D-04). A test installs a replacement before its first
 * context and records the calls, so the placement tests need no
 * privileges (SC-05).
 *
 * \pre none
 * \post none
 */
auto setPlacementFunction(PlacementFn function) -> void;

class ExecutionContext;

namespace detail
{

/**
 * @brief The whole live state of one context, defined by the archive that
 *        owns it (E-01, FR-001).
 *
 * `ExecutionContext` names this type as the pointee of its one member and
 * `source/harness/execution_context.cpp` defines it, so the inbox, the
 * policy, the identity and the thread of a running context stay out of
 * this header (FR-045).
 */
struct ContextControl;

/**
 * @brief The control block this context owns (FR-001, FR-024).
 *
 * The class grants this function access to its member, which is the only
 * path from a context to its control block: no side table and no scan
 * stand between the two, and no bound caps the live contexts (FR-001,
 * FR-024). The harness archive defines it, and no caller reaches it
 * (FR-045).
 *
 * \pre none
 * \post none
 */
auto controlOf(const ExecutionContext& context) noexcept -> ContextControl&;

/**
 * @brief The placement record the thread of this context kept at its
 *        start (FR-005, FR-025).
 *
 * A threaded run reads the disclosed placement field through this
 * function, because the control block of FR-001 stands out of this header
 * (FR-045). The record carries what the installed placement function
 * returned at the start of that thread, which is the account the
 * disclosure of FR-025 prints for a slot that never landed.
 * `source/harness/execution_context.cpp` defines it, and no caller of the
 * public surface reaches it (FR-045).
 *
 * \pre none
 * \post none
 */
auto placementRecordOf(const ExecutionContext& context) noexcept
    -> PlacementResult;

}  // namespace detail

/**
 * @brief One context: one thread, its event loop, its inbox, and its
 * policy (E-01, FR-001).
 *
 * The thread takes work only through this context's inbox, an intrusive
 * multi-producer single-consumer queue whose link lives in the work node
 * and whose enqueue allocates nothing (FR-003). The legal lifecycle is
 * `requestStop()`, `drain()`, `join()` in that order: a node whose
 * dispatch observed the not-stopped state runs during `drain()`, a node
 * whose dispatch observed the stop request is completed `STOPPED` by its
 * dispatcher, and no node runs after `join()` returns (FR-006, FR-050).
 *
 * \invariant none
 */
class ExecutionContext
{
public:
  /**
   * @brief Start one context under the default policy (FR-001, FR-004).
   *
   * The default policy is `BLOCK` with no pin (FR-004, Q-2).
   *
   * \pre the slot of the policy names no cpu, or names a cpu the host
   *      has online; the default policy names no cpu, so the bound
   *      stands satisfied (FR-005, SC-05)
   * \post one thread runs this context's event loop and `id()` reports
   *       this context's identity (FR-001, FR-002)
   */
  ExecutionContext();

  /**
   * @brief Start one context under `policy` (FR-001, FR-005).
   *
   * \pre the slot of `policy` names no cpu, or names a cpu the host has
   *      online (FR-005, SC-05)
   * \post one thread runs this context's event loop and `id()` reports
   *       this context's identity (FR-001, FR-002)
   */
  explicit ExecutionContext(ContextPolicy policy);

  /**
   * @brief Require a joined context, then release it (FR-006).
   *
   * \pre this context is joined (FR-006)
   * \post none
   */
  ~ExecutionContext();

  // One context owns one thread and its inbox, so a second owner would
  // join the same thread twice (FR-006).
  ExecutionContext(const ExecutionContext&) = delete;
  auto operator=(const ExecutionContext&) -> ExecutionContext& = delete;
  ExecutionContext(ExecutionContext&&) = delete;
  auto operator=(ExecutionContext&&) -> ExecutionContext& = delete;

  /**
   * @brief The identity of this context (FR-002, FR-045).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto id() const noexcept -> ContextId;

  /**
   * @brief The policy this context was created under (FR-007).
   *
   * The policy stands fixed for the context's life (FR-007).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto policy() const noexcept -> ContextPolicy;

  /**
   * @brief Stand the stop request this context's work items read
   * (FR-006, FR-050).
   *
   * A node the inbox already holds still runs during `drain()`; a node
   * a later dispatch offers is refused and completed `STOPPED` by that
   * dispatch (FR-006, FR-050). The call returns without waiting.
   *
   * \pre none
   * \post the stop request stands, so every `StopToken` of this context
   *       reads true and a later `dispatch()` refuses its node (FR-006,
   *       FR-050)
   */
  auto requestStop() -> void;

  /**
   * @brief Wait until the inbox stands empty and the loop stands idle
   * (FR-006, FR-050).
   *
   * With no prior `requestStop()` this is a flush: every accepted node
   * ran, and the loop keeps running, so a later dispatch still runs
   * (FR-006). The emptiness stands at the observation the wait made: a
   * dispatch concurrent with this return can refill the inbox, so no
   * postcondition here can re-read it (FR-050).
   *
   * \pre the calling thread is not the thread of this context's loop
   *      (FR-050)
   * \post none
   */
  auto drain() -> void;

  /**
   * @brief End the loop and join the thread of this context (FR-006).
   *
   * No work item of this context runs after this call returns (FR-006).
   *
   * \pre the calling thread is not the thread of this context's loop
   *      (FR-006)
   * \post none
   */
  auto join() -> void;

private:
  friend auto detail::controlOf(const ExecutionContext& context) noexcept
      -> detail::ContextControl&;

  // The one member and the whole state of this context: its inbox, policy,
  // identity, loop phase and thread. The object owns the block for its
  // life, so a context stands in no table and no count of live contexts
  // bounds the program (FR-001, FR-024).
  std::unique_ptr<detail::ContextControl> m_control;
};

/**
 * @brief The running work item's view of the stop request (FR-011,
 * FR-045).
 *
 * The token reads true once `requestStop()` stood on the context
 * running the item (FR-011).
 *
 * \invariant none
 */
class StopToken
{
public:
  /**
   * @brief True once the stop request of the owning context stands
   * (FR-011).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto stopRequested() const noexcept -> bool;
};

/**
 * @brief What a running work item receives (E-03, FR-011).
 *
 * The item reads the identity of the context running it and the stop
 * token of that context (FR-011). The reference is valid for the span of
 * the item's run.
 *
 * \invariant none
 */
class WorkContext
{
public:
  /**
   * @brief The identity of the context running this item (FR-011,
   *        FR-002).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto id() const noexcept -> ContextId;

  /**
   * @brief The stop token of the context running this item (FR-011).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto stopToken() const noexcept -> const StopToken&;
};

/**
 * @brief The one shape work a context accepts: a `noexcept` callable
 * taking the work context of its run (FR-008).
 *
 * A type outside this concept is refused where it names
 * `WorkNode<Item>` or reaches `dispatch()`, which is before the program
 * builds (FR-008).
 */
template<class Item>
concept WorkItem = requires(Item item, WorkContext& context) {
  {
    item(context)
  } noexcept;
};

/**
 * @brief The completion state of one work node (FR-009, FR-045, FR-049).
 *
 * `PENDING` stands from construction until the loop takes the node,
 * `RUNNING` stands while the loop runs the item, then `DONE` stands for
 * an item that ran, or `STOPPED` for a node a `dispatch()` that observed
 * the stop request refused and completed itself (FR-009, FR-050). The
 * completing thread writes that state with release order as its last
 * access to the node (FR-049).
 */
enum class WorkNodeState : std::uint8_t
{
  PENDING,
  RUNNING,
  DONE,
  STOPPED,
};

namespace detail
{

/**
 * @brief The erased shape one typed work node stands upon (E-03,
 *        FR-003, FR-009, D-02).
 *
 * The inbox link, the completion state and the one erased run path are
 * the words the context loop and a dispatching thread share; the typed
 * node below adds the item and stamps the path. Internal to the
 * harness: no caller names this type (FR-045), and the header carries it
 * because the node template and the `dispatch` template run in the
 * caller's translation unit.
 *
 * The link lives in the node (FR-003): a producer writes it only through
 * the inbox link-swap and the sentinel re-anchor of the harness archive,
 * and the loop decides every node's link before it starts that node's
 * run, so the completion state's release store stands as the completing
 * thread's last access to the node (FR-049).
 *
 * \invariant none
 */
struct WorkNodeBase
{
  // The inbox link, the `WorkNode*` link research D-01 names (FR-003).
  std::atomic<WorkNodeBase*> next {nullptr};

  // The completion state, `PENDING` from construction until the loop or
  // the refusing dispatch settles it (FR-009).
  std::atomic<WorkNodeState> completion {WorkNodeState::PENDING};

  // The accept stamp a dispatching thread writes before it links the
  // node, so it reaches a reader of the settled state through that
  // state's release chain; the node destructor reads it (FR-012).
  bool accepted = false;

  // The one erased run path; null stands only in the inbox sentinel,
  // which is never run (FR-009, D-02).
  void (*run)(WorkNodeBase& node, WorkContext& context) noexcept = nullptr;
};

/**
 * @brief The erased body of `dispatch`: the inbox link-swap of FR-003
 *        and the stop linearization of FR-050 (FR-010, FR-050).
 *
 * `source/harness/execution_context.cpp` defines it; the `dispatch`
 * template is its only caller, and the call allocates nothing (FR-040).
 *
 * \pre `node` stands in no inbox: its state is `PENDING` with no prior
 *      dispatch, `DONE`, or `STOPPED` (FR-009, FR-012)
 * \post none
 */
SPEEDGUN_NG_EXPORT auto dispatchNode(const ExecutionContext& context,
                                     WorkNodeBase& node) -> void;

}  // namespace detail

/**
 * @brief The caller-owned carrier of one typed work item (E-03, FR-009).
 *
 * Construction stamps the one erased run path from `Item`, so the loop
 * runs no run-time type test on it (FR-009, D-02). The node lives in
 * caller storage and stays alive until its state settles; destroying a
 * live node is a contract violation under the enforce semantic, and a
 * reader that observes `DONE` or `STOPPED` may destroy or reuse the node
 * (FR-012, FR-049). The node names no context, so it carries no wait
 * helper: the caller uses `ExecutionContext::drain()` or `state()`
 * (FR-004).
 *
 * \invariant none
 */
template<WorkItem Item>
class WorkNode : public detail::WorkNodeBase
{
public:
  /**
   * @brief Carry `item` as one node, its one erased run path stamped
   *        from `Item` (FR-009, D-02).
   *
   * The state of the fresh node reads `PENDING`, and the stamp is what
   * the loop calls, so `Item` is checked once here and never again
   * (FR-008).
   *
   * \pre none
   * \post none
   */
  explicit WorkNode(Item item)
      : m_item(std::move(item))
  {
    // The stamp is the one place this node's type is known: the loop
    // calls the pointer and runs no run-time type test (D-02).
    this->run = &WorkNode::runItem;
  }

  /**
   * @brief Release a node no context holds (FR-012).
   *
   * The state read stands first: its acquire order is what carries the
   * accept stamp of the thread that dispatched the node to the read of
   * it that follows (FR-049).
   *
   * \pre the node stands undispatched, or `state()` reads `DONE` or
   *      `STOPPED` (FR-012)
   * \post none
   */
  ~WorkNode()
  {
    [[maybe_unused]] const WorkNodeState settled = state();
    SG_REQUIRE(settled == WorkNodeState::DONE
                   || settled == WorkNodeState::STOPPED || !this->accepted,
               "destructor of a work node a context still holds (FR-012)");
  }

  /**
   * @brief The completion state, read with acquire order (FR-009,
   *        FR-049).
   *
   * The acquire pairs with the release write of the completing thread,
   * so a reader that sees `DONE` or `STOPPED` may destroy or reuse the
   * node (FR-012, FR-049).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto state() const noexcept -> WorkNodeState
  {
    return this->completion.load(std::memory_order_acquire);
  }

private:
  // The erased run path of `Item`: the loop calls it through the stamp
  // the constructor wrote, on the node it took from the inbox.
  //
  // P2 justification (Principle I): the downcast is sound because the
  // stamp belongs to this specialization, so a node that reaches here is
  // exactly one `WorkNode<Item>`; no other shape names this function
  // (D-02).
  static auto runItem(detail::WorkNodeBase& node,
                      WorkContext& context) noexcept -> void
  {
    static_cast<WorkNode&>(node).m_item(context);
  }

  Item m_item;
};

/**
 * @brief Hand `node` to `context` and return without waiting (FR-010,
 *        FR-050).
 *
 * The call links the node into that context's inbox, allocates nothing,
 * and is legal from any thread and from work already running on that
 * context (FR-003, FR-010, SC-04). `dispatch()` and `requestStop()`
 * linearize at the enqueue: a dispatch that observes the not-stopped
 * state appends the node and owns delivery, and a dispatch that observes
 * the stopped state refuses the node and completes it `STOPPED` itself,
 * so the loop never touches a node it never accepted (FR-050). Nodes one
 * thread dispatches run in that order; work dispatched from inside a
 * running item runs after that item ends, on the same context (FR-013).
 *
 * \pre `node` stands in no inbox: its state is `PENDING` with no prior
 *      dispatch, `DONE`, or `STOPPED` (FR-009, FR-012)
 * \post none
 */
template<WorkItem Item>
auto dispatch(ExecutionContext& context, WorkNode<Item>& node) -> void
{
  [[maybe_unused]] const WorkNodeState handed = node.state();
  SG_REQUIRE(handed == WorkNodeState::PENDING || handed == WorkNodeState::DONE
                 || handed == WorkNodeState::STOPPED,
             "dispatch of a node a context still holds (FR-009, FR-012)");
  // The one erased entry: what the context runs is the stamp the node's
  // own construction wrote, so this path names no item type (D-02).
  detail::dispatchNode(context, node);
}

/**
 * @brief The round a fixed set of participants meets on (E-04, FR-014,
 *        FR-015).
 *
 * Every round stands until all participants arrive, then releases them
 * at once (FR-014). A waiting participant waits in the wait mode of its
 * own policy, or in `BLOCK` when it runs on no context; the barrier
 * reads no clock, so no round times out (FR-014, Q-3). The call is legal
 * from inside running work and outside it (FR-015). A threaded run meets
 * on two of these, one round outside the start of every window and one
 * outside the end of every window (FR-019, D-05).
 *
 * \invariant the arrival count of a round never exceeds the participant
 *            count fixed at construction (FR-014)
 */
class ContextBarrier
{
public:
  /**
   * @brief Await `participants` arrivals in every round (FR-014).
   *
   * \pre `participants` stands above zero (FR-014, E-04)
   * \post none
   */
  explicit ContextBarrier(int participants);

  /**
   * @brief Require that no participant stands in a round (FR-015).
   *
   * \pre no participant of this barrier stands in a round that has not
   *      released (FR-015)
   * \post none
   */
  ~ContextBarrier();

  // One barrier object holds one round's arrival count and generation,
  // so a second object would split a round (FR-014).
  ContextBarrier(const ContextBarrier&) = delete;
  auto operator=(const ContextBarrier&) -> ContextBarrier& = delete;
  ContextBarrier(ContextBarrier&&) = delete;
  auto operator=(ContextBarrier&&) -> ContextBarrier& = delete;

  /**
   * @brief Arrive at this round and wait until it releases (FR-014,
   *        FR-015).
   *
   * The wait reads no clock and takes no timeout (FR-014). A caller that
   * leaves before this round releases, or that stands in another round of
   * this barrier at the same time, splits the round; no code of this class
   * can see either, so the duty stands outside the contract (FR-015).
   *
   * \pre none
   * \post the generation of this barrier stands past the round this call
   *       arrived at, so every participant of that round has arrived
   *       (FR-014)
   */
  auto arriveAndWait() -> void;

private:
  // E-04: the participants that have reached the round now open. The
  // arrival that brings it to `m_expectedCount` releases the round and
  // resets it, so one word carries both the round and its release
  // (FR-014, D-03).
  std::atomic<int> m_arrivalCount {0};

  // E-04, D-03: the generation of the rounds. The releasing arrival
  // advances it, and a waiting participant watches it in the wait mode
  // of its own policy (FR-014).
  std::atomic<std::uint64_t> m_generation {0};

  // E-04: the participant count fixed at construction (FR-014).
  const int m_expectedCount;
};

}  // namespace sg

#endif  // SG_EXECUTION_CONTEXT_HPP
