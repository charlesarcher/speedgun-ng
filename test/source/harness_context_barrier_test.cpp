// ============================================================================
// TDD test for the context barrier (T012; C-4, FR-014, FR-015, D-03, Q-3).
//
// This file is the test-first half of the pair FR-042 fixes. It reaches the
// barrier of include/speedgun-ng/execution_context.hpp by name only, so it
// links against definitions T013 owns and stands red until they land.
//
// Every scenario runs in a re-exec of this binary, the pattern of
// harness_pause_test.cpp and harness_execution_context_test.cpp: one process
// per scenario keeps the barrier, the contexts, and any contract abort inside
// one address space, and the parent reads the wait status the kernel gave the
// child (fork, execv, waitpid). A breach of an `SG_REQUIRE` contract therefore
// arrives as the SIGABRT the contract facility gives it.
//
// No scenario sleeps, reads a clock, or bounds a wait by elapsed time. The
// waits are flag watches on completion states with no bound: where one thread
// must act after another has reached a state, it watches that state before it
// triggers the action, so no scenario turns on which thread the scheduler ran
// first.
//
// The wait-mode proof reads the per-thread counters the kernel keeps at
// /proc/self/task/<id>/status: `voluntary_ctxt_switches` moves exactly when a
// thread gives up the cpu by blocking, and never while it spins. The reads are
// observer-side only: a measured participant never opens its own status file,
// because an in-thread probe read was measured on this host (Linux
// 7.2.9-2-cachyos) to cost the reader one voluntary switch of its own, which
// would poison the count it takes. The observer also holds a released
// participant on a permit flag, so the participant stays alive until its
// counter has been read a last time.
//
// Scenario map:
//   round               one round releases every participant (FR-014): four
//                       participants enter and the last arrival - the main
//                       thread, watched for three entries before it arrives -
//                       releases them; each records that no participant
//                       returned before every one had arrived, and that the
//                       full arrival count stood at its own wake
//   spinWait            a participant running on a SPIN context waits in its
//                       own mode and never sleeps inside the barrier: its
//                       voluntary-switch counter stands flat across the round
//                       (D-03, Q-3, the mechanism SC-08 keeps out of a
//                       measured window; reached from inside running work,
//                       FR-015)
//   yieldWait           a participant running on a YIELD context waits in its
//                       own mode: the policy it reads names YIELD, and its
//                       voluntary-switch counter stands flat across the round,
//                       so the wait gives up the cpu without blocking (D-03,
//                       Q-3, FR-014; the YIELD arm T013 left for T039)
//   insideOutside       the barrier is legal from inside running work and
//                       outside it (FR-015): an item on a context and a plain
//                       thread complete one round, and `currentContext()`
//                       names the context inside and reports nothing outside
//                       (FR-002); a participant that runs on no context waits
//                       in BLOCK: its task state stands parked in the kernel
//                       before the release trigger and its voluntary-switch
//                       counter moves (FR-014)
//   noClockNoTimeout    the barrier reads no clock and no round times out
//                       (FR-014): the surface offers no timed wait, and a
//                       round held open across a fixed CPU workload releases
//                       no participant while the missing arrival stands out
// Breach, expected to end its child by SIGABRT:
//   earlyDeparture      a participant that leaves a round early is a contract
//                       violation (FR-015): the barrier is destroyed while a
//                       round still holds a participant's arrival
//   zeroParticipants    a participant count of zero is an `SG_REQUIRE`
//                       violation at the construction call, so the
//                       construction ends the child (FR-014, E-04; the
//                       precondition T013 left for T039)
// ============================================================================

#include <array>
#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>

#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "speedgun-ng/execution_context.hpp"

// A named namespace stands in for the anonymous one, for the reason the
// execution context suite records: the item types reach `dispatch` and
// `WorkNode`, whose templates the public header only declares, and internal
// linkage would turn every instantiation into a use no translation unit can
// define - a compile error, where the missing barrier definitions must show
// up at the link instead.
namespace context_barrier_test
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CONTEXT BARRIER TEST FAIL: %s\n", what);
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

// FR-014: the barrier reads no clock, so no timed wait exists to be called.
// The probe names the two spellings a timed surface would carry; the barrier
// must refuse both, the compile-time half of the noClockNoTimeout scenario.
template<class Barrier>
concept HasWaitFor = requires(Barrier& barrier) {
  {
    barrier.wait_for(1)
  };
};

template<class Barrier>
concept HasWaitUntil = requires(Barrier& barrier) {
  {
    barrier.wait_until(1)
  };
};

template<class Barrier>
concept HasTimedWait = HasWaitFor<Barrier> || HasWaitUntil<Barrier>;

static_assert(
    !HasTimedWait<sg::ContextBarrier>,
    "the barrier offers no timed wait, so no round can time out " "(FR-014)");

// ---------------------------------------------------------------------------
// Observer-side kernel reads. The buffers are stack storage and the reads are
// plain open/read/close, so a read allocates nothing and blocks on nothing
// beyond formatting its own file.
// ---------------------------------------------------------------------------

constexpr std::size_t kStatusCapacity = 8192;

auto readStatus(const char* path,
                std::array<char, kStatusCapacity>& buffer) -> std::size_t
{
  const int descriptor = ::open(path, O_RDONLY | O_CLOEXEC);
  if (descriptor < 0) {
    return 0;
  }
  std::size_t filled = 0;
  for (;;) {
    const ssize_t got =
        ::read(descriptor, buffer.data() + filled, buffer.size() - filled);
    if (got <= 0) {
      break;
    }
    filled += static_cast<std::size_t>(got);
    if (filled == buffer.size()) {
      break;
    }
  }
  ::close(descriptor);
  return filled;
}

auto taskStatusPath(int tid, char* buffer, std::size_t capacity) -> void
{
  const int written =
      std::snprintf(buffer, capacity, "/proc/self/task/%d/status", tid);
  (void)written;
}

// The value of a `key:\tN` line, matched at line start so the voluntary key
// cannot match the nonvoluntary one.
auto statusCount(std::string_view text, std::string_view key) -> long long
{
  std::size_t start = 0;
  while (start < text.size()) {
    std::size_t end = text.find('\n', start);
    if (end == std::string_view::npos) {
      end = text.size();
    }
    const std::string_view line = text.substr(start, end - start);
    if (line.size() > key.size() && line.compare(0, key.size(), key) == 0) {
      std::size_t index = key.size();
      while (index < line.size() && (line[index] == ' ' || line[index] == '\t'))
      {
        ++index;
      }
      long long value = 0;
      bool any = false;
      while (index < line.size() && line[index] >= '0' && line[index] <= '9') {
        value = value * 10 + (line[index] - '0');
        ++index;
        any = true;
      }
      return any ? value : -1;
    }
    start = end + 1;
  }
  return -1;
}

auto voluntarySwitchesOf(int tid) -> long long
{
  char path[64];
  taskStatusPath(tid, path, sizeof(path));
  std::array<char, kStatusCapacity> buffer {};
  const std::size_t filled = readStatus(path, buffer);
  return statusCount(std::string_view(buffer.data(), filled),
                     "voluntary_ctxt_switches:");
}

auto stateCharOf(int tid) -> char
{
  char path[64];
  taskStatusPath(tid, path, sizeof(path));
  std::array<char, kStatusCapacity> buffer {};
  const std::size_t filled = readStatus(path, buffer);
  const std::string_view text(buffer.data(), filled);
  const std::size_t at = text.find("State:");
  if (at == std::string_view::npos) {
    return '\0';
  }
  std::size_t index = at + 6;
  while (index < filled && (buffer[index] == ' ' || buffer[index] == '\t')) {
    ++index;
  }
  return index < filled ? buffer[index] : '\0';
}

auto selfThreadId() -> int
{
  return static_cast<int>(::syscall(SYS_gettid));
}

// ---------------------------------------------------------------------------
// The shape of every measured wait. The participant publishes its thread id,
// waits for the go signal so the observer can read the counter before the
// wait begins, records that it entered, arrives, and on return holds on a
// permit so the observer reads the counter once more while the thread is
// still alive. It touches no status file itself.
// ---------------------------------------------------------------------------

// Watches a completion flag with no bound: the store that sets it is the whole
// wait, the pattern the execution context suite sets with its state loops.
auto watch(const std::atomic<bool>& flag) -> void
{
  while (!flag.load(std::memory_order_acquire)) {
  }
}

struct Observer
{
  std::atomic<int> tid {0};
  std::atomic<bool> ready {false};
  std::atomic<bool> go {false};
  std::atomic<bool> entered {false};
  std::atomic<bool> departed {false};
  std::atomic<bool> permit {false};
  std::atomic<bool> sawContext {false};
  long long switchesBefore {0};
  long long switchesAfter {0};
};

struct Bracket
{
  long long before {0};
  long long after {0};
  bool parkedBeforeRelease {false};
};

auto participantDrive(Observer& observer,
                      sg::ContextBarrier& barrier) noexcept -> void
{
  observer.tid.store(selfThreadId(), std::memory_order_release);
  observer.sawContext.store(sg::currentContext().has_value(),
                            std::memory_order_relaxed);
  observer.ready.store(true, std::memory_order_release);
  watch(observer.go);
  observer.entered.store(true, std::memory_order_release);
  barrier.arriveAndWait();
  observer.departed.store(true, std::memory_order_release);
  watch(observer.permit);
}

// Releases the round from the observer side and brackets the participant's
// wait with the two counter reads. With `expectPark` set the release waits
// until the participant stands asleep in the kernel, which is only reachable
// from a blocking wait: the BLOCK mark. No release ever precedes the watch.
auto releaseAndBracket(Observer& observer,
                       sg::ContextBarrier& barrier,
                       const bool expectPark) -> Bracket
{
  watch(observer.ready);
  const int tid = observer.tid.load(std::memory_order_acquire);
  Bracket bracket;
  bracket.before = voluntarySwitchesOf(tid);
  observer.go.store(true, std::memory_order_release);
  watch(observer.entered);
  if (expectPark) {
    while (!observer.departed.load(std::memory_order_acquire)) {
      if (stateCharOf(tid) == 'S') {
        bracket.parkedBeforeRelease = true;
        break;
      }
    }
  }
  check(
      !observer.departed.load(std::memory_order_acquire),
      "no participant returns from a round before its last arrival " "(FR-"
                                                                     "014)");
  barrier.arriveAndWait();
  watch(observer.departed);
  bracket.after = voluntarySwitchesOf(tid);
  observer.permit.store(true, std::memory_order_release);
  return bracket;
}

// Closes a context through the one legal lifecycle (FR-006).
auto closeContext(sg::ExecutionContext& context) -> void
{
  context.requestStop();
  context.drain();
  context.join();
}

// FR-014: four participants, one round. Every participant records the arrival
// count standing at its own wake and whether the last arrival had announced
// itself when it returned; the last arrival is the main thread, which watches
// the other three enter before it arrives.
constexpr int kRoundParticipants = 4;

auto scenarioRound() -> int
{
  sg::ContextBarrier barrier {kRoundParticipants};
  std::atomic<int> enteredCount {0};
  std::atomic<int> departedCount {0};
  std::atomic<bool> lastArrivalAnnounced {false};
  std::array<std::thread, kRoundParticipants - 1> workers {};
  std::array<std::atomic<int>, kRoundParticipants - 1> wakeCount {};
  std::array<std::atomic<bool>, kRoundParticipants - 1> returnedTooEarly {};
  std::array<std::atomic<bool>, kRoundParticipants - 1> ran {};

  for (std::size_t index = 0; index < workers.size(); ++index) {
    workers[index] = std::thread {
        [&, index]
        {
          ran[index].store(true, std::memory_order_release);
          enteredCount.fetch_add(1, std::memory_order_acq_rel);
          barrier.arriveAndWait();
          wakeCount[index].store(enteredCount.load(std::memory_order_acquire),
                                 std::memory_order_release);
          returnedTooEarly[index].store(!lastArrivalAnnounced.load(),
                                        std::memory_order_release);
          departedCount.fetch_add(1, std::memory_order_acq_rel);
        }};
  }
  while (enteredCount.load(std::memory_order_acquire) < kRoundParticipants - 1)
  {
  }
  lastArrivalAnnounced.store(true);
  enteredCount.fetch_add(1, std::memory_order_acq_rel);
  barrier.arriveAndWait();
  const int mainWakeCount = enteredCount.load(std::memory_order_acquire);
  for (auto& worker : workers) {
    worker.join();
  }

  check(mainWakeCount == kRoundParticipants,
        "the last arrival found every participant entered (FR-014)");
  for (std::size_t index = 0; index < workers.size(); ++index) {
    check(ran[index].load(std::memory_order_acquire),
          "every participant took the round once (FR-014)");
    check(wakeCount[index].load(std::memory_order_acquire)
              == kRoundParticipants,
          "a participant woke only once every expected participant had "
          "arrived (FR-014)");
    check(!returnedTooEarly[index].load(std::memory_order_acquire),
          "no participant returned before the round's last arrival (FR-014)");
  }
  check(departedCount.load() == static_cast<int>(workers.size()),
        "one round releases every participant (FR-014)");
  check(!sg::currentContext().has_value(),
        "a thread the barrier waits on outside any context reports no context "
        "(FR-002, FR-014)");
  std::puts("harness_context_barrier_test round: ok");
  return 0;
}

// An item that drives the observer shape from inside running work (FR-015).
struct ArriveItem
{
  Observer* observer = nullptr;
  sg::ContextBarrier* barrier = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    participantDrive(*observer, *barrier);
  }
};

// D-03, Q-3, the SC-08 mechanism: the participant running on a SPIN context
// waits in its own mode, and the mode's own counter - the per-thread
// voluntary-switch count the kernel keeps - stands flat across the round.
auto scenarioSpinWait() -> int
{
  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::SPIN,
                                  .placement = sg::PlacementSlot::noPin()};
  sg::ExecutionContext context {policy};
  sg::ContextBarrier barrier {2};
  Observer observer;
  const ArriveItem item {&observer, &barrier};
  sg::WorkNode<ArriveItem> node {item};
  sg::dispatch(context, node);

  const Bracket bracket = releaseAndBracket(observer, barrier, false);
  check(bracket.before >= 0 && bracket.after >= 0,
        "the kernel counters were readable around the round");
  check(bracket.after == bracket.before,
        "the SPIN participant never sleeps inside the barrier: its "
        "voluntary-switch counter stands flat across the round (D-03, Q-3, "
        "FR-014)");
  check(observer.sawContext.load(std::memory_order_relaxed),
        "the barrier is legal from inside running work, where currentContext "
        "names a context (FR-002, FR-015)");
  closeContext(context);
  check(node.state() == sg::WorkNodeState::DONE,
        "the item that rode the round completed (FR-009, FR-015)");
  std::puts("harness_context_barrier_test spinWait: ok");
  return 0;
}

// FR-014, Q-3: the YIELD arm of the barrier wait. The participant runs on a
// context whose policy names YIELD, so `participantWaitMode()` answers YIELD
// and the wait takes the yield path. The mode's own kernel signature is what
// separates it from a block: a yield hands the cpu to another runnable task
// while the task stays runnable, so it moves the involuntary-switch counter
// and never the voluntary one, which is the counter this suite reads for a
// wait that blocks. The flat voluntary count is therefore the assertion here,
// exactly as for SPIN; the involuntary count is not asserted, because on an
// idle cpu a yield may keep the cpu and move nothing.
auto scenarioYieldWait() -> int
{
  const sg::ContextPolicy policy {.waitMode = sg::WaitMode::YIELD,
                                  .placement = sg::PlacementSlot::noPin()};
  sg::ExecutionContext context {policy};
  check(context.policy().waitMode == sg::WaitMode::YIELD,
        "the participant's own policy names the YIELD wait mode it must wait "
        "in (FR-007, FR-014)");
  sg::ContextBarrier barrier {2};
  Observer observer;
  const ArriveItem item {&observer, &barrier};
  sg::WorkNode<ArriveItem> node {item};
  sg::dispatch(context, node);

  const Bracket bracket = releaseAndBracket(observer, barrier, false);
  check(bracket.before >= 0 && bracket.after >= 0,
        "the kernel counters were readable around the round");
  check(bracket.after == bracket.before,
        "the YIELD participant never sleeps inside the barrier: its "
        "voluntary-switch counter stands flat across the round (D-03, Q-3, "
        "FR-014)");
  check(observer.sawContext.load(std::memory_order_relaxed),
        "the YIELD participant waited from inside running work, where the "
        "policy of its own context names the mode (FR-002, FR-014, FR-015)");
  closeContext(context);
  check(node.state() == sg::WorkNodeState::DONE,
        "the item that rode the round completed (FR-009, FR-015)");
  std::puts("harness_context_barrier_test yieldWait: ok");
  return 0;
}

// FR-015 and FR-014: one round from inside running work and outside it, then
// a round whose outside participant is watched standing in the BLOCK wait.
// The inside item does not gate on a go signal; its counter is not measured,
// the round's legality and identity view are.
struct UngatedArriveItem
{
  sg::ContextBarrier* barrier = nullptr;
  std::atomic<bool>* arrived = nullptr;
  std::atomic<bool>* sawContext = nullptr;

  auto operator()(sg::WorkContext&) noexcept -> void
  {
    sawContext->store(sg::currentContext().has_value(),
                      std::memory_order_relaxed);
    arrived->store(true, std::memory_order_release);
    barrier->arriveAndWait();
  }
};

auto scenarioInsideOutside() -> int
{
  // Part one: the item arrives from inside running work; the plain thread
  // watches the item's arrival and answers it as the round's last arrival.
  sg::ContextBarrier legalityBarrier {2};
  std::atomic<bool> insideArrived {false};
  std::atomic<bool> insideSawContext {false};
  bool outsideSawContext = true;
  sg::ExecutionContext context;
  const UngatedArriveItem item {
      &legalityBarrier, &insideArrived, &insideSawContext};
  sg::WorkNode<UngatedArriveItem> node {item};
  sg::dispatch(context, node);
  std::thread outside {[&legalityBarrier, &insideArrived, &outsideSawContext]
                       {
                         watch(insideArrived);
                         outsideSawContext = sg::currentContext().has_value();
                         legalityBarrier.arriveAndWait();
                       }};
  outside.join();
  closeContext(context);
  check(node.state() == sg::WorkNodeState::DONE,
        "the barrier is legal from inside running work (FR-015)");
  check(insideSawContext.load(std::memory_order_relaxed),
        "inside running work currentContext names the context (FR-002)");
  check(!outsideSawContext,
        "outside running work currentContext reports no context (FR-002, "
        "FR-015)");

  // Part two: a participant that runs on no context waits in BLOCK. The
  // observer sees its task state parked before the release, and its mode's
  // own counter moves across the round.
  sg::ContextBarrier blockBarrier {2};
  Observer outsideParticipant;
  std::thread blocker {[&outsideParticipant, &blockBarrier]
                       { participantDrive(outsideParticipant, blockBarrier); }};
  const Bracket bracket =
      releaseAndBracket(outsideParticipant, blockBarrier, true);
  blocker.join();

  check(bracket.parkedBeforeRelease,
        "a participant that runs on no context waits in BLOCK: its task state "
        "stands parked in the kernel before the release (FR-014)");
  check(bracket.after > bracket.before,
        "the BLOCK participant's voluntary-switch counter moves across the "
        "round, the counter of its own wait mode (FR-014)");
  check(!outsideParticipant.sawContext.load(std::memory_order_relaxed),
        "the BLOCK participant ran on no context (FR-014)");
  std::puts("harness_context_barrier_test insideOutside: ok");
  return 0;
}

// FR-014: the barrier reads no clock and no round times out. One participant
// arrives; the round stays open across a fixed CPU workload (a burn loop,
// no sleep: no clock stands anywhere in this scenario) and releases nothing
// until the missing arrival lands.
constexpr unsigned long long kHoldWork = 150'000'000ULL;
// The exact sum the holding workload must accumulate: 0 + 1 + ... + N - 1.
constexpr unsigned long long kHoldSum = kHoldWork * (kHoldWork - 1) / 2;

auto scenarioNoClockNoTimeout() -> int
{
  sg::ContextBarrier barrier {2};
  std::atomic<bool> entered {false};
  std::atomic<bool> returned {false};
  std::thread participant {[&barrier, &entered, &returned]
                           {
                             entered.store(true, std::memory_order_release);
                             barrier.arriveAndWait();
                             returned.store(true, std::memory_order_release);
                           }};
  watch(entered);
  volatile unsigned long long sink = 0;
  for (unsigned long long step = 0; step < kHoldWork; ++step) {
    sink += step;
  }
  const unsigned long long drained = sink;
  check(drained == kHoldSum,
        "the holding workload ran to completion while the round stood open");
  check(!returned.load(std::memory_order_acquire),
        "no round times out: a held round releases no participant while an "
        "arrival stands missing (FR-014)");
  barrier.arriveAndWait();
  participant.join();
  check(returned.load(std::memory_order_acquire),
        "the missing arrival, and nothing else, closed the round (FR-014)");
  std::puts("harness_context_barrier_test noClockNoTimeout: ok");
  return 0;
}

// FR-015: a participant that leaves a round early is a contract violation.
// The outside participant arrives and parks inside the round; the other
// participant ends the barrier's lifetime while that arrival still stands in
// the round, which is leaving the round early. The destructor must refuse the
// live round, and the enforce semantic ends this process inside the delete.
auto breachEarlyDeparture() -> int
{
  auto* barrier = new sg::ContextBarrier {2};
  Observer observer;
  std::thread participant {[&observer, &barrier]
                           { participantDrive(observer, *barrier); }};
  watch(observer.ready);
  observer.go.store(true, std::memory_order_release);
  watch(observer.entered);
  const int tid = observer.tid.load(std::memory_order_acquire);
  for (;;) {
    if (stateCharOf(tid) == 'S'
        || observer.departed.load(std::memory_order_acquire))
    {
      break;
    }
  }
  check(stateCharOf(tid) == 'S',
        "the arriving participant stands inside the round before it is left");
  delete barrier;
  participant.detach();
  check(false,
        "destroying the barrier while a round holds a participant's arrival is "
        "a contract violation (FR-015)");
  return 1;
}

// FR-014, E-04: the participant count of a round stands fixed at the
// construction call, and a count of zero is refused there - a round of no
// participant can never stand full, so every later arrival would wait on a
// release no arrival can bring. The construction is the violation, so the
// child ends inside the constructor and the object is never stood.
auto breachZeroParticipants() -> int
{
  const sg::ContextBarrier barrier {0};
  (void)barrier;
  check(false,
        "a participant count of zero is an SG_REQUIRE violation at the "
        "construction call (FR-014, E-04)");
  return 1;
}

struct Mode
{
  const char* name;
  auto (*run)() -> int;
};

constexpr std::array<Mode, 5> kScenarios {
    Mode {.name = "round", .run = &scenarioRound},
    Mode {.name = "spinWait", .run = &scenarioSpinWait},
    Mode {.name = "yieldWait", .run = &scenarioYieldWait},
    Mode {.name = "insideOutside", .run = &scenarioInsideOutside},
    Mode {.name = "noClockNoTimeout", .run = &scenarioNoClockNoTimeout},
};

constexpr std::array<Mode, 2> kBreaches {
    Mode {.name = "earlyDeparture", .run = &breachEarlyDeparture},
    Mode {.name = "zeroParticipants", .run = &breachZeroParticipants},
};

}  // namespace context_barrier_test

auto main(const int argc, char** argv) -> int
{
  using context_barrier_test::check;
  using context_barrier_test::kBreaches;
  using context_barrier_test::kScenarios;
  using context_barrier_test::runChild;

  const std::string_view argument = argc > 1 ? argv[1] : "";
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
    (void)std::fprintf(stderr, "unknown mode: %s\n", argc > 1 ? argv[1] : "");
    return 2;
  }

  const char* const self = argv[0];
  for (const auto& scenario : kScenarios) {
    const int status = runChild(self, scenario.name, false);
    const std::string message =
        std::string("the scenario passes: ") + scenario.name;
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, message.c_str());
  }

  // The breach ends its child through the contract facility, which aborts.
  // The parent forks and execs directly and reads the signal from waitpid,
  // the pattern harness_pause_test.cpp carries for the same reason.
  for (const auto& breach : kBreaches) {
    const int status = runChild(self, breach.name, true);
    const std::string message =
        std::string("the breach ends its child by SIGABRT: ") + breach.name;
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, message.c_str());
  }

  std::puts("harness_context_barrier_test: ok");
  return 0;
}
