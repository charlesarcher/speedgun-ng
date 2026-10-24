// The context core: the event loop of `ExecutionContext`, its wait
// modes, the thread-current `ContextId`, the placement slot and its
// application at thread start, the dispatch path into the inbox, and the
// round a barrier joins (FR-001, FR-002, FR-004, FR-005, FR-009, FR-010,
// FR-014, FR-015, FR-049, T010, T011, T013).
//
// Feature 020-execution-contexts-core. The names stand in
// `include/speedgun-ng/execution_context.hpp` as the spec names them
// (FR-036, FR-045); what stands here is their definition inside the
// `speedgun-ng_harness` archive, which the top-level glob collects with
// no new target (FR-043).

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>

#include "speedgun-ng/execution_context.hpp"

#include <pthread.h>
#include <sched.h>

#include "detail/internal.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{
namespace
{

// Pause hint for the SPIN wait (FR-004, D-01). Written as the one
// `asm volatile` site that `include/speedgun-ng/barrier.hpp` already
// ships for the optimization barrier, so the inbox spin sits in the same
// documented shape as the shipped pause helper, carries no name of its
// own (FR-036), and needs no intrinsic header (D-01).
//
// P2 justification: the operand is absent, so nothing is aliased; the
// `memory` clobber is the compiler barrier, which is exactly the
// semantics of `barrier.hpp`'s empty-operand asm.
//
// The AArch64 spelling matches `barrier.hpp` and is inert on x86-64, the
// only CI architecture this file is verified on.
inline auto pauseHint() -> void
{
#if defined(__x86_64__) || defined(__i386__)
  asm volatile(
      "pause" ::
          : "memory");  // NOLINT(hicpp-no-assembler, portability-no-assembler)
#elifdef __aarch64__
  asm volatile(
      "yield" ::
          : "memory");  // NOLINT(hicpp-no-assembler, portability-no-assembler)
#else
  static_cast<void>(0);
#endif
}

// The thread-current context, held in a function-local `thread_local`
// slot (NMT-TH: no thread-local static data member exists, and a file-
// scope `thread_local` variable would be the mutable state the rule
// names). Only the owning thread reads and writes its own slot, and the
// context loop installs and clears it around one node's run (FR-002).
auto currentContextSlot() -> std::optional<ContextId>&
{
  thread_local std::optional<ContextId> slot {};
  return slot;
}

// The printable placement field `NO_PIN` of the disclosure record, which
// the default placement function returns for a slot that names no cpu
// (FR-005, FR-025, `contracts/report-disclosure.md`).
constexpr std::string_view kNoPinText = "NO_PIN";

// The printable placement field of a slot the placement function did not
// land, which a failed pin returns so the disclosure names the miss: the
// row carries no cpu the context never ran on (FR-005, FR-025, FR-026).
constexpr std::string_view kUnappliedText = "UNAPPLIED";

// The width of the widest `std::uint32_t` in decimal, so the rendered
// cpu index of the placement record never truncates (FR-005, FR-025).
constexpr std::size_t kCpuIndexWidth = 10;

// The width of the widest account the placement record can name: the
// `cpu@group` pair of a landed pin, the longest text the placement
// function composes from the layout (FR-021, FR-025, specs/025 E-09).
constexpr std::size_t kAccountTextWidth = kCpuIndexWidth * 2 + 1;

// The default placement function: one Linux pin of the started thread
// to the slot's cpu (FR-005, D-04). The call is the platform call D-04
// names for the seam; the installed function replaces the whole call, so
// a test that replaces it executes no platform pin at all (SC-05).
//
// specs/025 FR-021, E-09: the description is the disclosure text of the
// slot, the `cpu@group` pair the layout composes, and the account of a
// pin that did not land stays the unapplied word the seam chooses.
//
// P2 justification: the native handle reaches the platform as the type
// the platform call names it, through no cast.
auto pinToPlacementSlot(const PlacementSlot& slot,
                        const std::thread::native_handle_type thread)
    -> PlacementResult
{
  if (slot.cpuIndex == kNoPin) {
    return {.applied = false, .description = kNoPinText};
  }

  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  CPU_SET(slot.cpuIndex, &cpus);

  const bool landed = pthread_setaffinity_np(thread, sizeof(cpus), &cpus) == 0;
  return detail::selectPlacementAccount(landed, detail::placementText(slot));
}

// FR-005, research D-04: the one harness-level placement slot. A
// function-local static carries it, the shape `registry()` gives the
// registry in the same archive, and `setPlacementFunction` installs
// before any context starts, so no context reads the slot while it is
// written (D-04).
auto placementSlot() -> PlacementFn&
{
  static PlacementFn function = &pinToPlacementSlot;
  return function;
}

}  // namespace

namespace detail
{

// FR-005, FR-025: the disclosure account of one pin attempt. The platform
// answer names which text the field carries, and each answer stands in a
// branch a test drives directly: the replacement function of
// `setPlacementFunction` replaces the whole pin, so the default pin's own
// two accounts stand nowhere else (FR-025).
auto selectPlacementAccount(
    const bool landed, const std::string_view landedText) -> PlacementResult
{
  if (landed) {
    return {.applied = true, .description = landedText};
  }
  return {.applied = false, .description = kUnappliedText};
}

// Where the loop stands, as `drain()` reads it (FR-006, FR-050):
// `WORKING` covers taking, running and looking again; `IDLE` stands when
// the inbox provably stands empty and the loop waits by policy; `EXITED`
// stands after the stop-drained loop ended.
// NOLINTNEXTLINE(misc-use-internal-linkage) the phase word ContextControl holds
enum class LoopPhase : std::uint8_t
{
  WORKING,
  IDLE,
  EXITED,
};

// E-01, FR-001: the live state of one context. The header's object owns it
// through a pointer, and the thread-current control slot below names the one
// whose node this thread runs (FR-011).
// NOLINTNEXTLINE(misc-use-internal-linkage) the block behind the header's
// object
struct ContextControl
{
  explicit ContextControl(const ContextPolicy& creationPolicy,
                          const ContextId& identity)
      : policy(creationPolicy)
      , id(identity)
  {
    exchangeTail.store(&stub, std::memory_order_relaxed);
  }

  // The policy fixed at creation, read through `policy()` for the
  // context's life (FR-007).
  ContextPolicy policy;

  // The one identity this context names, from the live-context counter
  // (FR-002).
  ContextId id;

  // What the installed placement function did with the policy's slot,
  // written by the context's own thread at its start and read by the
  // threaded run that builds the disclosure record (FR-005, FR-025).
  PlacementResult placementResult {false, kNoPinText};

  // The bytes the field above points at, held in the block so the record
  // reads past the end of the thread that rendered them (FR-025).
  std::array<char, kAccountTextWidth> descriptionText {};

  // The sentinel of the inbox: the chain anchor both ends walk through,
  // owned by the loop, never run (FR-003, D-01).
  WorkNodeBase stub;

  // The atomic inbox tail the producers swap into, the `tail` of D-01
  // (FR-003).
  std::atomic<WorkNodeBase*> exchangeTail {nullptr};

  // The one inbox flag the loop reads when the inbox stands empty: the
  // waker of `BLOCK`, the word `SPIN` and `YIELD` watch (FR-004, D-01).
  std::atomic<bool> wakeFlag {false};

  // The stop request the work items read through `StopToken` (FR-006,
  // FR-011).
  std::atomic<bool> stopRequested {false};

  // Where the loop stands, for `drain()` (FR-006, FR-050).
  std::atomic<LoopPhase> phase {LoopPhase::WORKING};

  // The idle-state generation a blocked `drain()` waits in; the loop
  // bumps it when it parks and when it exits (FR-006).
  std::atomic<std::uint64_t> idleEpoch {0};

  // The two words of the FR-050 linearization: `dispatch()` publishes its
  // accept here before it reads the stop request, and a dispatch that
  // links its node counts the link here, so the loop and `drain()` run
  // every accepted node before they stand empty.
  std::atomic<std::uint64_t> acceptCount {0};
  std::atomic<std::uint64_t> dispatchCount {0};

  // The reference every running item receives (FR-011, E-03); stateless,
  // so one stands per context.
  WorkContext workContext;

  // The one thread the context owns (FR-001).
  std::thread worker;

  // Set by the joining thread after `join()`; the destructor reads it on
  // that same thread, the lifecycle's order (FR-006).
  bool joined = false;
};

}  // namespace detail

namespace
{

// The thread-current control block of the running context, beside its
// identity slot: the `StopToken` of the running item reads the stop
// request through it (FR-011). As with the identity slot, only the
// owning thread touches its own value.
auto currentControlSlot() -> detail::ContextControl*&
{
  thread_local detail::ContextControl* slot = nullptr;
  return slot;
}

// FR-002: the loop installs the running context's identity, and the
// control the token reads, for the span of one node's run and clears
// them after; the run never returns without the scope ending.
class CurrentContextScope
{
public:
  explicit CurrentContextScope(detail::ContextControl& control)
      : m_slot(currentContextSlot())
      , m_control(currentControlSlot())
  {
    m_slot = control.id;
    m_control = &control;
  }

  ~CurrentContextScope()
  {
    m_slot.reset();
    m_control = nullptr;
  }

  CurrentContextScope(const CurrentContextScope&) = delete;
  auto operator=(const CurrentContextScope&) -> CurrentContextScope& = delete;
  CurrentContextScope(CurrentContextScope&&) = delete;
  auto operator=(CurrentContextScope&&) -> CurrentContextScope& = delete;

private:
  std::optional<ContextId>& m_slot;
  detail::ContextControl*& m_control;
};

// FR-001, FR-024: the control block lives in the context object itself,
// so the paths that reach it read one member: no side table, no scan on
// the dispatch path, and no bound on the count of live contexts. The
// accessor stands in `namespace detail` below, where the block is defined.

// FR-049: the sentinel re-anchor. When the drain walk stands on the
// newest node of a batch, the loop links the sentinel behind it, so every
// later producer writes the loop-owned sentinel and never a node the
// loop has run or taken: the completion store then stands as the loop's
// last access to that node. The exchange chooses the predecessor in the
// inbox's total order, so no producer's link and this link ever race on
// one node's field.
auto reAnchorSentinel(detail::ContextControl& control) -> void
{
  // The sentinel stands walked by the time its own node stands at the
  // tail, so its stale link is cleared before the exchange claims the
  // tail: a producer writes the link of a node it swapped out as tail,
  // which is the sentinel only after this exchange, so no write to this
  // field races the clear.
  control.stub.next.store(nullptr, std::memory_order_relaxed);
  detail::WorkNodeBase* const previous =
      control.exchangeTail.exchange(&control.stub, std::memory_order_acq_rel);
  previous->next.store(&control.stub, std::memory_order_release);
}

// FR-004: the three clock-free waits for an empty inbox. `SPIN` watches
// the one flag through the pause helper, `YIELD` yields to the scheduler,
// `BLOCK` stands in the standard atomic wait on the same flag until
// `dispatch()` calls `notify_one()`; the stop request wakes every mode
// too, so the stop-drained loop ends while the inbox stands empty. No
// mode reads a clock or takes a timeout (FR-039).
auto waitInboxEmpty(const detail::ContextControl& control) -> void
{
  const WaitMode mode = control.policy.waitMode;
  while (!control.wakeFlag.load(std::memory_order_acquire)
         && !control.stopRequested.load(std::memory_order_acquire))
  {
    // The chain stands in the order the policy names the modes and its
    // last arm is `BLOCK`, so each of the three modes enters exactly one
    // arm and no arm stands unentered (FR-004).
    if (mode == WaitMode::SPIN) {
      pauseHint();
    } else if (mode == WaitMode::YIELD) {
      std::this_thread::yield();
    } else {
      control.wakeFlag.wait(false);
    }
  }
}

// The event loop of FR-001 on the one owned thread: take the oldest node
// when the inbox holds one, run it, look again; wait by policy when the
// inbox stands empty. The walk pointer is this thread's private drain
// head (FR-003); each pass decides the node's successor before running
// the node, so the loop reads the next queue link before the completion
// store and never touches a node again after it (FR-009, FR-049).
auto runContextLoop(detail::ContextControl& control) -> void
{
  detail::WorkNodeBase* walk = &control.stub;
  for (;;) {
    // The private drain head's step (FR-003): the tail the producers swap
    // is read before the link the walk follows, so the acquire on the tail
    // orders out a stale link the last re-anchor cleared (D-01). The table
    // `detail::walkStep` decides the pass from those two words and the
    // walk's position, so the pass that finds a producer between its
    // exchange and its link stands in the same outcome as the empty inbox:
    // this pass holds no node to run, and the loop turns again (FR-003,
    // FR-006). `EMPTY` answers only from the sentinel position with the tail
    // provably at the sentinel, the proof `drain()` and the loop's end read
    // (FR-001, FR-006).
    detail::WorkNodeBase* const tail =
        control.exchangeTail.load(std::memory_order_seq_cst);
    detail::WorkNodeBase* const next =
        walk->next.load(std::memory_order_acquire);
    const detail::WalkStep step =
        detail::walkStep(next != nullptr, tail == walk, walk == &control.stub);
    if (step == detail::WalkStep::REANCHOR) {
      // The re-anchor stands the tail on the sentinel, which is the proof a
      // waiter reads as an empty inbox. The node under the walk is stood here
      // and not yet run, so the mark goes first: a waiter that sees the
      // re-anchored tail sees this store with it, and the empty proof cannot
      // settle over a node the walk owns but has not run (FR-006, FR-050).
      control.phase.store(detail::LoopPhase::WORKING,
                          std::memory_order_release);
      reAnchorSentinel(control);
      continue;
    }

    if (step != detail::WalkStep::NODE) {
      // The inbox stands empty to every reached enqueue. With the stop
      // stood and every accepted dispatch linked or refused, the loop has
      // run every node it accepted and ends (FR-006, FR-050). The counts
      // are the account both proofs read: an accept counted before the
      // stop stands in it, and the loop turns again until that accept
      // stands linked, so no accepted node is ever left behind.
      const bool stopped =
          control.stopRequested.load(std::memory_order_seq_cst);
      if (!stopped) {
        control.phase.store(detail::LoopPhase::IDLE, std::memory_order_release);
        control.idleEpoch.fetch_add(1, std::memory_order_release);
        control.idleEpoch.notify_all();
      }
      const std::uint64_t accepted =
          control.acceptCount.load(std::memory_order_seq_cst);
      const std::uint64_t dispatched =
          control.dispatchCount.load(std::memory_order_seq_cst);
      if (detail::loopEnds(stopped, accepted, dispatched)) {
        control.phase.store(detail::LoopPhase::EXITED,
                            std::memory_order_release);
        control.idleEpoch.fetch_add(1, std::memory_order_release);
        control.idleEpoch.notify_all();
        return;
      }

      // Consume the flag after the proof, so an enqueue that set it
      // during this pass wakes the next one, which finds it set.
      if (!control.wakeFlag.exchange(false, std::memory_order_acquire)) {
        waitInboxEmpty(control);
      }
      continue;
    }

    detail::WorkNodeBase* const node = walk;
    walk = next;  // The take passes the node before its run (FR-009).
    if (node == &control.stub) {
      continue;  // The walk passed the sentinel; the node follows.
    }

    control.phase.store(detail::LoopPhase::WORKING, std::memory_order_release);
    node->completion.store(WorkNodeState::RUNNING, std::memory_order_relaxed);
    {
      const CurrentContextScope scope(control);
      node->run(*node, control.workContext);
    }
    // FR-049: the release store is the loop's last access to the node.
    // The successor link was decided before the run, the re-anchor link
    // precedes it, and the node carries no notify of its own.
    node->completion.store(WorkNodeState::DONE, std::memory_order_release);
  }
}

// FR-005, D-04: the placement of the policy's slot lands once, on the
// context's own thread, at its start, through the installed function,
// before the loop takes a node. The record the function returns stands in
// the control for the context's life: the disclosure of FR-025 names what
// the function did, which is the account of a slot that never landed.
auto applyPlacement(detail::ContextControl& control) -> void
{
  // The handle of the running thread is the handle of the context's
  // thread, and stands as the type the seam names through no cast.
  const std::thread::native_handle_type self = pthread_self();
  const PlacementResult placement =
      placementSlot()(control.policy.placement, self);

  // The function renders its text wherever it chose, and the default
  // renders into storage of the thread that pinned. The bytes move into
  // the block, so the record a run discloses later reads past the end of
  // that thread (FR-025).
  const std::size_t width =
      std::min(placement.description.size(), control.descriptionText.size());
  std::ranges::copy_n(placement.description.data(),
                      static_cast<std::ptrdiff_t>(width),
                      control.descriptionText.begin());
  control.placementResult = PlacementResult {
      placement.applied,
      std::string_view {control.descriptionText.data(), width},
  };
}

// FR-001, FR-005: the one thread the context owns: its placement first,
// then the loop.
auto runContextThread(detail::ContextControl& control) -> void
{
  applyPlacement(control);
  runContextLoop(control);
}

// FR-006, FR-050: the state `drain()` waits for, read through the table of
// `detail::inboxIdle`. The tail is read before the phase here: the loop marks
// `WORKING` before it re-anchors the sentinel, so a reader that sees the
// re-anchored tail sees the mark with it; reading the phase first would let a
// waiter take the new tail together with the idle phase that stood before it,
// and return from `drain()` while the walked node still owed its run
// (FR-006, FR-050).
auto inboxStandIdle(const detail::ContextControl& control) -> bool
{
  const bool tailAtSentinel =
      control.exchangeTail.load(std::memory_order_acquire) == &control.stub;
  const bool working = control.phase.load(std::memory_order_acquire)
      == detail::LoopPhase::WORKING;
  return detail::inboxIdle(
      tailAtSentinel,
      working,
      control.acceptCount.load(std::memory_order_seq_cst),
      control.dispatchCount.load(std::memory_order_seq_cst));
}

// FR-014, Q-3: the wait mode a barrier participant waits in is the wait
// mode of its own policy, which is the policy of the context the calling
// thread runs. A thread that runs no context names no policy, and waits
// in BLOCK.
auto participantWaitMode() -> WaitMode
{
  const detail::ContextControl* const control = currentControlSlot();
  return control != nullptr ? control->policy.waitMode : WaitMode::BLOCK;
}

}  // namespace

namespace detail
{

// FR-006, FR-050: the dispatch account, held apart from the two count reads
// so each outcome of the test of the pair stands in a test of its own. An
// accept counted before the stop stands in the account; an accept that reads
// the stop withdraws before the enqueue call returns (E-03).
auto countsSettled(const std::uint64_t accepted,
                   const std::uint64_t dispatched) noexcept -> bool
{
  return accepted == dispatched;
}

// FR-006, FR-050: the end test the context loop reads. A loop with the stop
// still open has no end to reach, since work still owed it may stand in the
// inbox on the next pass.
auto loopEnds(const bool stopRequested,
              const std::uint64_t accepted,
              const std::uint64_t dispatched) noexcept -> bool
{
  return stopRequested && countsSettled(accepted, dispatched);
}

// FR-003, FR-006: the walk's decision for one pass, held apart from the two
// reads and the walk's position so each outcome stands in a test of its own.
// The successor link decides first: a published link is a node to take, at
// either position. A pass with no link and the tail elsewhere stands in
// `PENDING`: a producer has swapped the tail and owes its link, a window of
// two instructions in one wait-free enqueue. With the tail at the walked node
// the walk owns the last node, so the sentinel position states the empty
// inbox and any other node asks for the re-anchor (D-01, FR-003, FR-006).
auto walkStep(const bool linkPublished,
              const bool tailAtWalk,
              const bool walkAtSentinel) noexcept -> WalkStep
{
  if (linkPublished) {
    return WalkStep::NODE;
  }
  if (!tailAtWalk) {
    return WalkStep::PENDING;
  }
  if (walkAtSentinel) {
    return WalkStep::EMPTY;
  }
  return WalkStep::REANCHOR;
}

// FR-006, FR-050: the drain proof, held apart from its three reads so each
// term stands in a test of its own. An accept that still owes its link holds
// the account apart, so drain stands until the loop has run that node.
auto inboxIdle(const bool tailAtSentinel,
               const bool working,
               const std::uint64_t accepted,
               const std::uint64_t dispatched) noexcept -> bool
{
  return tailAtSentinel && !working && countsSettled(accepted, dispatched);
}

// NOLINTNEXTLINE(misc-use-internal-linkage) the accessor the class befriends
// FR-001, FR-002: the block the object owns. The construction makes it and
// the joined destructor releases it, so it stands through every member call
// of the context's life (FR-006). The require answers the semantic the
// contract layer names; under the semantic-ignore build the read below
// would be undefined behaviour and yields no answer.
auto controlOf(const ExecutionContext& context) noexcept -> ContextControl&
{
  SG_REQUIRE(context.m_control != nullptr,
             "the context of this call holds no control block (FR-002)");
  return *context.m_control;
}

// FR-005, FR-025: the record the block holds. A threaded run reads the
// disclosed placement field through here, since the block itself stands
// out of the header (FR-045).
auto placementRecordOf(const ExecutionContext& context) noexcept
    -> PlacementResult
{
  return controlOf(context).placementResult;
}

// FR-010, FR-050: the erased body of `dispatch()`, which the header's
// template reaches after handing the typed node to its erased base. The
// link-swap is the two atomics D-01 names: the exchange of the tail and
// the publication of the predecessor link; nothing allocates (FR-003,
// FR-040).
// NOLINTNEXTLINE(misc-use-internal-linkage) the entry the header's dispatch
// template calls
auto dispatchNode(const ExecutionContext& context, WorkNodeBase& node) -> void
{
  ContextControl& control = controlOf(context);

  // FR-009, FR-012: a node stands in at most one inbox at a time, so the
  // dispatch of a node another dispatch still owns is a contract
  // violation. A caller may reuse a node once it reads DONE or STOPPED.
  [[maybe_unused]] const WorkNodeState prior =
      node.completion.load(std::memory_order_relaxed);
  SG_REQUIRE(prior != WorkNodeState::RUNNING
                 && (prior != WorkNodeState::PENDING || !node.accepted),
             "dispatch of a node that stands in a queue (FR-009, FR-012)");

  // FR-050: the accept stands in the total order before the stop read,
  // and `requestStop()` stands its store before the loop and `drain()`
  // read the counts, so a dispatch that reads the not-stopped state is
  // seen by their empty proofs and owns delivery, and a dispatch that
  // starts after the stop stood reads it and refuses. A refusal never
  // touches the chain, so the loop never sees the node (E-03).
  control.acceptCount.fetch_add(1, std::memory_order_seq_cst);
  if (control.stopRequested.load(std::memory_order_seq_cst)) {
    // The completing thread writes the state with release order as its
    // last access to the node (FR-009, FR-049).
    node.completion.store(WorkNodeState::STOPPED, std::memory_order_release);
    control.acceptCount.fetch_sub(1, std::memory_order_release);
    return;
  }

  // FR-009, FR-049: the reused node settles PENDING before the link below
  // makes it visible, so a reader of the state never finds a queued node
  // still stamped DONE (FR-012).
  node.completion.store(WorkNodeState::PENDING, std::memory_order_relaxed);
  node.accepted = true;
  node.next.store(nullptr, std::memory_order_relaxed);
  WorkNodeBase* const previous =
      control.exchangeTail.exchange(&node, std::memory_order_acq_rel);
  previous->next.store(&node, std::memory_order_release);

  control.dispatchCount.fetch_add(1, std::memory_order_release);

  // The wake stands last: a loop that wakes on the flag finds the
  // published link, and `notify_one()` is the waker `BLOCK` waits in
  // (FR-004). The call is legal from any thread, including work already
  // running on this context (FR-010, SC-04); the node runs after the
  // running item ends because the loop is single (FR-013).
  control.wakeFlag.store(true, std::memory_order_release);
  control.wakeFlag.notify_one();
}

}  // namespace detail

auto currentContext() -> std::optional<ContextId>
{
  return currentContextSlot();
}

auto setPlacementFunction(const PlacementFn function) -> void
{
  placementSlot() = function;
}

ExecutionContext::ExecutionContext()
    : ExecutionContext(ContextPolicy {})
{
}

ExecutionContext::ExecutionContext(const ContextPolicy policy)
{
  // FR-005, SC-05: the host bound of the slot's index stands at the
  // construction call, where the online cpu set stands; the placement
  // itself lands on the context's thread at its start (D-04). The set
  // is the layout's own, so a count the threads report short of the
  // set's highest index leaves the membership deciding (FR-005, FR-041).
  const auto& onlineCpus = detail::layout().mask;
  const bool online =
      std::find(onlineCpus.begin(), onlineCpus.end(), policy.placement.cpuIndex)
      != onlineCpus.end();
  SG_REQUIRE(policy.placement.cpuIndex == kNoPin || online,
             "the policy's placement names a cpu the host lacks (FR-005)");

  // FR-002: one identity per live context, from the harness-wide
  // counter; serial zero stands for no context, the value the default
  // `ContextId` carries.
  static std::atomic<std::uint64_t> nextSerial {0};
  m_control = std::make_unique<detail::ContextControl>(
      policy,
      ContextId {nextSerial.fetch_add(1, std::memory_order_relaxed) + 1});

  // FR-001: the one thread the context owns applies its placement and
  // starts the loop at once (FR-005, D-04).
  m_control->worker = std::thread(&runContextThread, std::ref(*m_control));
  SG_ENSURE(m_control->worker.joinable(),
            "a context constructed while its loop thread stands not "
            "started (FR-001, FR-002)");
}

ExecutionContext::~ExecutionContext()
{
  [[maybe_unused]] detail::ContextControl& control = detail::controlOf(*this);
  SG_REQUIRE(control.joined,
             "destructor of a context that stands unjoined (FR-006)");
}

auto ExecutionContext::id() const noexcept -> ContextId
{
  return detail::controlOf(*this).id;
}

auto ExecutionContext::policy() const noexcept -> ContextPolicy
{
  return detail::controlOf(*this).policy;
}

// NOLINTNEXTLINE(readability-make-member-function-const) the signature stands
// fixed in the header (FR-036); the state stands behind the control block
auto ExecutionContext::requestStop() -> void
{
  detail::ContextControl& control = detail::controlOf(*this);
  // FR-006, FR-050: the stop store stands in the total order, and the
  // wake carries it to a loop the empty inbox is waiting in, so the
  // stop-drained loop ends while it stands empty (FR-004). The call
  // returns without waiting.
  control.stopRequested.store(true, std::memory_order_seq_cst);
  control.wakeFlag.store(true, std::memory_order_release);
  control.wakeFlag.notify_all();
  SG_ENSURE(control.stopRequested.load(std::memory_order_seq_cst),
            "a stop request that does not stand in the total order "
            "(FR-006, FR-050)");
}

// NOLINTNEXTLINE(readability-make-member-function-const) the signature stands
// fixed in the header (FR-036)
auto ExecutionContext::drain() -> void
{
  const detail::ContextControl& control = detail::controlOf(*this);
  SG_REQUIRE(std::this_thread::get_id() != control.worker.get_id(),
             "drain() called on the thread of the context's own loop (FR-050)");
  // FR-006, FR-050: the caller waits in the idle generation the loop
  // bumps when it parks or exits; the wait takes no clock and no timeout
  // (FR-039). Without a stop request the loop parks again after the flush
  // and keeps running; with one, the loop has run every node it accepted
  // before it ended (FR-050).
  for (;;) {
    const std::uint64_t epoch =
        control.idleEpoch.load(std::memory_order_acquire);
    if (inboxStandIdle(control)) {
      return;
    }
    control.idleEpoch.wait(epoch);
  }
}

// NOLINTNEXTLINE(readability-make-member-function-const) the signature stands
// fixed in the header (FR-036); the state stands behind the control block
auto ExecutionContext::join() -> void
{
  detail::ContextControl& control = detail::controlOf(*this);
  SG_REQUIRE(std::this_thread::get_id() != control.worker.get_id(),
             "join() called on the thread of the context's own loop (FR-006)");
  // FR-006: the loop ends only with the stop stood and every accepted
  // node run, so no work item of this context runs after this call
  // returns.
  if (control.worker.joinable()) {
    control.worker.join();
  }
  control.joined = true;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) the signature
// stands fixed in the header (FR-036)
auto StopToken::stopRequested() const noexcept -> bool
{
  // FR-011: the token of the running item reads its own context's stop
  // request through the thread-current control; off any context it reads
  // false.
  const detail::ContextControl* const control = currentControlSlot();
  return control != nullptr
      && control->stopRequested.load(std::memory_order_acquire);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) the signature
// stands fixed in the header (FR-036)
auto WorkContext::id() const noexcept -> ContextId
{
  // FR-011, FR-002: the identity the loop installed for this run.
  return currentContextSlot().value_or(ContextId {});
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) the signature
// stands fixed in the header (FR-036)
auto WorkContext::stopToken() const noexcept -> const StopToken&
{
  // The token carries no state of its own: it reads the running
  // context's stop request through the thread-current control (FR-011).
  static const StopToken kToken {};
  return kToken;
}

// E-04, FR-014: the participant count of the rounds stands fixed at the
// construction call. A count of zero is refused there, because a round of
// no participant can never stand full, so every later arrival would wait
// on a release that no arrival can bring (E-04).
ContextBarrier::ContextBarrier(const int participants)
    : m_expectedCount(participants)
{
  SG_REQUIRE(participants > 0,
             "a context barrier constructed with no participant (FR-014)");
}

// FR-015: ending a barrier whose round still holds an arrival is that
// participant leaving the round early, which the contract refuses. The
// read carries no ordering of its own for the same reason the joined flag
// of a context carries none: the lifecycle of the caller's own threads
// stands before this call, and an end that outruns them is the violation
// this precondition names.
ContextBarrier::~ContextBarrier()
{
  SG_REQUIRE(m_arrivalCount.load(std::memory_order_relaxed) == 0,
             "a participant stands in a round of this barrier (FR-015)");
}

// FR-014, D-03: one round is one arrival count and one generation word,
// so the arrival path allocates nothing and calls nothing that blocks.
// The arrival that stands the count at the expected number resets it and
// advances the generation, which is the single release every other
// participant watches, and it happens for all of them at once. The wait
// reads no clock and takes no timeout, so no round times out (FR-039).
auto ContextBarrier::arriveAndWait() -> void
{
  // FR-014: the round this call arrives at stands recorded before the
  // arrival, so the exit can name the round it waited on. The generation
  // only ever advances, so the read at the exit races nothing.
  const std::uint64_t enteredRound =
      m_generation.load(std::memory_order_acquire);
  const int arrived =
      m_arrivalCount.fetch_add(1, std::memory_order_acq_rel) + 1;
  SG_INVARIANT(
      arrived <= m_expectedCount,
      "the arrivals of one round exceed its participant count " "(FR-014)");

  if (arrived == m_expectedCount) {
    // The reset stands before the generation advance, and the release
    // order of that advance carries it to every participant that
    // acquires the generation it advances to (FR-049). The arrival that
    // filled the round waits on nothing: it is the release.
    m_arrivalCount.store(0, std::memory_order_relaxed);
    m_generation.fetch_add(1, std::memory_order_release);
    m_generation.notify_all();
    SG_ENSURE(
        m_generation.load(std::memory_order_acquire) != enteredRound,
        "a round that released without advancing the generation " "(FR-014)");
    return;
  }

  // The mode of the participant's policy stands read once, so a spin costs one
  // load and one hint (FR-014, Principle VII). The chain stands in the order
  // the policy names the modes and its last arm is `BLOCK`, so each mode enters
  // exactly one arm (FR-004). No mode reads a clock or takes a timeout, so no
  // round times out (FR-039). A participant records its arrival before it
  // waits, so a non-last arrival that stands preempted between its own arrival
  // and this chain can reach the chain behind a release: the arrival that
  // filled the round advances the generation meanwhile. Each arm answers that
  // entry, the parked mode at once, because the standard wait returns at once
  // on a value that has left the round it was given, and the spinning modes
  // after one hint. A `std::barrier` backs itself with the same standard wait
  // as its fallback (Q-3).
  const WaitMode mode = participantWaitMode();
  if (mode == WaitMode::SPIN) {
    do {
      pauseHint();
    } while (m_generation.load(std::memory_order_acquire) == enteredRound);
  } else if (mode == WaitMode::YIELD) {
    do {
      std::this_thread::yield();
    } while (m_generation.load(std::memory_order_acquire) == enteredRound);
  } else {
    m_generation.wait(enteredRound);
  }
  SG_ENSURE(
      m_generation.load(std::memory_order_acquire) != enteredRound,
      "a round that released without advancing the generation " "(FR-014)");
}

}  // namespace sg
