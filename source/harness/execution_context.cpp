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

#include <array>
#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string_view>
#include <thread>

#include "speedgun-ng/execution_context.hpp"

#include <pthread.h>
#include <sched.h>

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

// The width of the widest `std::uint32_t` in decimal, so the rendered
// cpu index of the placement record never truncates (FR-005, FR-025).
constexpr std::size_t kCpuIndexWidth = 10;

// The default placement function: one Linux pin of the started thread
// to the slot's cpu (FR-005, D-04). The call is the platform call D-04
// names for the seam; the installed function replaces the whole call, so
// a test that replaces it executes no platform pin at all (SC-05).
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

  // The digits sit in storage the pinned thread owns. A thread pins once,
  // so they stand for that thread's whole life and the disclosure record
  // that outlives the thread holds a copy (FR-025).
  thread_local std::array<char, kCpuIndexWidth> digits {};
  const auto rendered =
      std::to_chars(digits.begin(), digits.end(), slot.cpuIndex);

  return {
      .applied = pthread_setaffinity_np(thread, sizeof(cpus), &cpus) == 0,
      .description =
          {
              digits.data(),
              static_cast<std::size_t>(rendered.ptr - digits.data()),
          },
  };
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

// The live-context bound of the control side table below.
// shortcut: the table shape dies when `ExecutionContext` carries its
// control-block member; a context above this bound is a contract
// violation, and no scenario of this feature lives more than two.
constexpr std::size_t kContextSlotCount = 64;

}  // namespace

namespace detail
{

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

// E-01, FR-001: the live state of one context, stood behind the header's
// object, which is its key through the control side table below.
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

  // The header object this block stands behind, the identity the control
  // side table matches (FR-002).
  const ExecutionContext* owner = nullptr;

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

// The control side table: the header object is one byte, so the live
// state stands here behind it, claimed at construction and released at
// the joined destructor, and read lock-free by `dispatch()` from any
// thread (FR-001, FR-010).
// shortcut: bound at `kContextSlotCount` live contexts while the header
// carries no member for the control block; the header's control-block
// member deletes this table, and the bound is a contract violation long
// before any scenario reaches it (FR-001).
auto contextSlotTable()
    -> std::array<std::atomic<detail::ContextControl*>, kContextSlotCount>&
{
  static std::array<std::atomic<detail::ContextControl*>, kContextSlotCount>
      slots {};
  return slots;
}

auto claimControlSlot(detail::ContextControl& control) -> bool
{
  for (auto& slot : contextSlotTable()) {
    auto* expected = static_cast<detail::ContextControl*>(nullptr);
    if (slot.compare_exchange_strong(
            expected, &control, std::memory_order_release))
    {
      return true;
    }
  }
  return false;
}

auto releaseControlSlot(const detail::ContextControl& control) -> void
{
  for (auto& slot : contextSlotTable()) {
    if (slot.load(std::memory_order_acquire) == &control) {
      slot.store(nullptr, std::memory_order_release);
    }
  }
}

auto controlFor(const ExecutionContext& context) -> detail::ContextControl&
{
  for (const auto& slot : contextSlotTable()) {
    detail::ContextControl* const control =
        slot.load(std::memory_order_acquire);
    if (control != nullptr && control->owner == &context) {
      return *control;
    }
  }
  // The construction claims the slot and the joined destructor releases
  // it, so a live context stands in the table through every member call
  // of its own life (FR-001, FR-006). The stop below matches the default
  // response of the contract layer; under the semantic-ignore build the
  // read here would be undefined behaviour and yields no answer.
  SG_REQUIRE(false, "the context of this call holds no control block (FR-002)");
  std::abort();
}

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

// The private drain head's step (FR-003): decide the node behind `walk`,
// oldest first, as D-01's chain gives it. A link a producer has swapped
// but not yet published is waited out with the pause hint: the window
// spans the two instructions of one wait-free enqueue, and it is not one
// of the three wait modes (FR-004), which wait only on the empty inbox.
// `nullptr` returns only from the sentinel position with the tail provably
// at the sentinel: the inbox then stands empty to every enqueue that has
// reached its exchange, the proof `drain()` and the loop's end read
// (FR-001, FR-006).
auto decideNextNode(detail::ContextControl& control,
                    detail::WorkNodeBase& walk) -> detail::WorkNodeBase*
{
  for (;;) {
    if (&walk == &control.stub) {
      // The sentinel's link is read after the tail, so the acquire on
      // the tail orders out a stale link the last re-anchor cleared,
      // and the read below is fresh or a genuine pending write.
      const detail::WorkNodeBase* const tail =
          control.exchangeTail.load(std::memory_order_acquire);
      detail::WorkNodeBase* const after =
          control.stub.next.load(std::memory_order_acquire);
      if (after != nullptr) {
        return after;
      }
      if (tail == &control.stub) {
        return nullptr;
      }
      pauseHint();
      continue;
    }
    detail::WorkNodeBase* const next =
        walk.next.load(std::memory_order_acquire);
    if (next != nullptr) {
      return next;
    }
    if (control.exchangeTail.load(std::memory_order_seq_cst) == &walk) {
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
    pauseHint();
  }
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
    switch (mode) {
      case WaitMode::SPIN:
        pauseHint();
        break;
      case WaitMode::YIELD:
        std::this_thread::yield();
        break;
      case WaitMode::BLOCK:
        control.wakeFlag.wait(false);
        break;
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
    detail::WorkNodeBase* const next = decideNextNode(control, *walk);

    if (next == nullptr) {
      // The inbox stands empty to every reached enqueue. With the stop
      // stood and every accepted dispatch linked or refused, the loop has
      // run every node it accepted and ends (FR-006, FR-050). The two
      // count reads close the window between another thread's accept and
      // its exchange: an accept counted before the stop stands in the
      // counts, and the loop turns again until it stands linked, so no
      // accepted node is ever left behind.
      if (control.stopRequested.load(std::memory_order_seq_cst)) {
        if (control.acceptCount.load(std::memory_order_seq_cst)
            == control.dispatchCount.load(std::memory_order_seq_cst))
        {
          control.phase.store(detail::LoopPhase::EXITED,
                              std::memory_order_release);
          control.idleEpoch.fetch_add(1, std::memory_order_release);
          control.idleEpoch.notify_all();
          return;
        }
      } else {
        control.phase.store(detail::LoopPhase::IDLE, std::memory_order_release);
        control.idleEpoch.fetch_add(1, std::memory_order_release);
        control.idleEpoch.notify_all();
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
// before the loop takes a node. Nothing reads the record it returns: the
// disclosure of FR-025 names the policy's slot, which the control holds
// for the context's life.
auto applyPlacement(const detail::ContextControl& control) -> void
{
  // The handle of the running thread is the handle of the context's
  // thread, and stands as the type the seam names through no cast.
  const std::thread::native_handle_type self = pthread_self();
  static_cast<void>(placementSlot()(control.policy.placement, self));
}

// FR-001, FR-005: the one thread the context owns: its placement first,
// then the loop.
auto runContextThread(detail::ContextControl& control) -> void
{
  applyPlacement(control);
  runContextLoop(control);
}

// FR-006, FR-050: the state `drain()` waits for: the loop parked or
// exited, the inbox at its sentinel, and every accepted dispatch linked
// or refused. A node a dispatch reached but has not yet linked holds the
// counts apart, so `drain()` stands until the loop has run it.
auto inboxStandIdle(const detail::ContextControl& control) -> bool
{
  // The tail stands read before the phase. The loop marks `WORKING` before it
  // re-anchors the sentinel, so a reader that sees the re-anchored tail sees
  // the mark with it; reading the phase first would let a waiter take the new
  // tail together with the idle phase that stood before it, and return from
  // `drain()` while the walked node still owed its run (FR-006, FR-050).
  return control.exchangeTail.load(std::memory_order_acquire) == &control.stub
      && control.phase.load(std::memory_order_acquire)
      != detail::LoopPhase::WORKING
      && control.acceptCount.load(std::memory_order_seq_cst)
      == control.dispatchCount.load(std::memory_order_seq_cst);
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

// FR-010, FR-050: the erased body of `dispatch()`, which the header's
// template reaches after handing the typed node to its erased base. The
// link-swap is the two atomics D-01 names: the exchange of the tail and
// the publication of the predecessor link; nothing allocates (FR-003,
// FR-040).
// NOLINTNEXTLINE(misc-use-internal-linkage) the entry the header's dispatch
// template calls
auto dispatchNode(const ExecutionContext& context, WorkNodeBase& node) -> void
{
  ContextControl& control = controlFor(context);

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
  // construction call, where the online cpu count stands; the placement
  // itself lands on the context's thread at its start (D-04).
  const auto onlineCpus =
      static_cast<std::uint32_t>(std::thread::hardware_concurrency());
  SG_REQUIRE(policy.placement.cpuIndex == kNoPin
                 || policy.placement.cpuIndex < onlineCpus,
             "the policy's placement names a cpu the host lacks (FR-005)");

  // FR-002: one identity per live context, from the harness-wide
  // counter; serial zero stands for no context, the value the default
  // `ContextId` carries.
  static std::atomic<std::uint64_t> nextSerial {0};
  auto* const control = new detail::ContextControl(
      policy,
      ContextId {nextSerial.fetch_add(1, std::memory_order_relaxed) + 1});
  const bool claimed = claimControlSlot(*control);
  SG_REQUIRE(claimed, "live contexts beyond the control-table bound (FR-001)");
  static_cast<void>(claimed);
  control->owner = this;

  // FR-001: the one thread the context owns applies its placement and
  // starts the loop at once (FR-005, D-04).
  control->worker = std::thread(&runContextThread, std::ref(*control));
  SG_ENSURE(control->worker.joinable(),
            "a context constructed while its loop thread stands not "
            "started (FR-001, FR-002)");
}

ExecutionContext::~ExecutionContext()
{
  detail::ContextControl& control = controlFor(*this);
  SG_REQUIRE(control.joined,
             "destructor of a context that stands unjoined (FR-006)");
  releaseControlSlot(control);
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) the side table hands the
  // raw block back for the joined end
  delete &control;
}

auto ExecutionContext::id() const noexcept -> ContextId
{
  return controlFor(*this).id;
}

auto ExecutionContext::policy() const noexcept -> ContextPolicy
{
  return controlFor(*this).policy;
}

// NOLINTNEXTLINE(readability-make-member-function-const) the signature stands
// fixed in the header (FR-036); the state stands behind the control block
auto ExecutionContext::requestStop() -> void
{
  detail::ContextControl& control = controlFor(*this);
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
  const detail::ContextControl& control = controlFor(*this);
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
  detail::ContextControl& control = controlFor(*this);
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

  // The round is the value captured before this arrival; a read here names
  // a newer round: an arrival that completed the round returned, and a round
  // that completed between this arrival and this point stands advanced past
  // the captured value already, so the loop is not entered. A generation
  // read here would be taken as the round to wait on, and this barrier never
  // advances past it (FR-014, FR-049).
  const WaitMode mode = participantWaitMode();
  while (m_generation.load(std::memory_order_acquire) == enteredRound) {
    // FR-014: the same three clock-free paths as the context loop, in the
    // mode of the participant's own policy, so a spinning participant
    // never sleeps inside the barrier. BLOCK stands in the standard
    // atomic wait, the one path `std::barrier` backs as the fallback of
    // Q-3.
    switch (mode) {
      case WaitMode::SPIN:
        pauseHint();
        break;
      case WaitMode::YIELD:
        std::this_thread::yield();
        break;
      case WaitMode::BLOCK:
        m_generation.wait(enteredRound);
        break;
    }
  }
  SG_ENSURE(
      m_generation.load(std::memory_order_acquire) != enteredRound,
      "a round that released without advancing the generation " "(FR-014)");
}

}  // namespace sg
