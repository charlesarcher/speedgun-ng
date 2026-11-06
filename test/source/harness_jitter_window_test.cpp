// ============================================================================
// T007: the report surfaces of one run with the probe in play.
//
// The console block prints after the benchmark tables, one column per
// probe cpu, the sixteen stats fields in the E-03 order (FR-023, R-08);
// the disclosure lines print in the end-of-report region the noise
// warning owns (FR-036, D-8, D-12). The JSON scenarios pin the run-level
// `jitter` object in the E-07 key order after `speedgun` and before
// `benchmarks`, the per-entry nullable key after `noiseWarning` (E-08),
// the round trip (FR-029, SC-001), the raw gap list (FR-028, E-10), and
// the reader error codes on the new keys (FR-029, contracts/report-json.md).
// The JSON scenarios stand red against the writer and the reader that
// T013 and T014 land, so they run after the green ones and the first red
// assertion names the scope the wave still misses. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sched.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/jitter_probe.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER WINDOW TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The seam readers' ledger: one read count per thread a seam reader stands
// on. The probe's own reads land here, and the runner's window edges never
// reach it, so a wait can end on the reads every probe cpu made (FR-012).
auto seamLedger() -> std::map<std::thread::id, std::uint64_t>&
{
  static std::map<std::thread::id, std::uint64_t> counts;
  return counts;
}

auto seamLedgerLock() -> std::mutex&
{
  static std::mutex lock;
  return lock;
}

auto noteSeamRead() -> void
{
  const std::scoped_lock guard {seamLedgerLock()};
  ++seamLedger()[std::this_thread::get_id()];
}

auto seamLedgerSnapshot() -> std::map<std::thread::id, std::uint64_t>
{
  const std::scoped_lock guard {seamLedgerLock()};
  return seamLedger();
}

// The watched entry's arm: the read count each probe cpu owes inside the
// watched body's own timed window, and the cpu count the scenario armed. A
// scenario that reads an entry's own series arms both before its run and
// clears them after it. Every other scenario runs the body with no wait
// (FR-012, FR-020, R-11).
struct WatchedWindow
{
  std::uint64_t reads = 0;
  std::size_t cpus = 0;
};

auto watchedWindow() -> WatchedWindow&
{
  static WatchedWindow window;
  return window;
}

auto armWatchedWindow(const std::uint64_t reads, const std::size_t cpus) -> void
{
  watchedWindow() = WatchedWindow {.reads = reads, .cpus = cpus};
}

auto clearWatchedWindow() -> void
{
  watchedWindow() = WatchedWindow {};
}

// The reads each probe cpu owes inside the armed window. The scripted
// clock hands every reader a cursor of its own and keeps the furthest one
// in the leaf, so a window's open edge stands above the probe's cursor by
// the reads the runner made while the probe stood off the cpu. A sample
// carries a stamp recovered from the probe's own cursor, and it lands
// inside the window once that cursor passes the open edge. The runner
// reads the leaf a few dozen times per attempt, so an armed count of a
// thousand probe reads passes the open edge on every host (FR-012, FR-020,
// R-11).
inline constexpr std::uint64_t kWatchedWindowReads = 1024;

// The wait the watched body ends on: every armed probe cpu stands the
// armed count of its own reads past the count the ledger held when the
// body reached the wait.
auto waitForProbeReads(const std::size_t cpus,
                       const std::uint64_t reads) -> void
{
  const auto start = seamLedgerSnapshot();
  std::uint64_t spins = 0;
  while (true) {
    const auto now = seamLedgerSnapshot();
    std::size_t reached = 0;
    for (const auto& held : now) {
      const auto found = start.find(held.first);
      const auto base = found == start.end() ? 0 : found->second;
      if (held.second >= base + reads) {
        ++reached;
      }
    }
    if (reached >= cpus) {
      return;
    }
    // The wait yields, the way the gate body's waits do, so the pinned
    // probe thread progresses under concurrent load. The bound turns a
    // probe that never reads into a reported failure; the wait cannot
    // hang (FR-012).
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the probe's reads never landed inside the watched window "
           "(FR-020, R-11)");
    }
  }
}

// A trivial benchmark: the probe spans the batch, so the run needs one
// registered function and the shortest minimum time the option scanner
// admits. The armed body ends on the probe's own reads, which stand inside
// this entry's timed window and carry samples of their own into the
// entry's fold (FR-006, FR-020, R-11).
auto bmQuiet(sg::State& state) -> void
{
  std::uint64_t work = 0;
  for (auto _ : state) {
    work += 1;
    sg::doNotOptimize(work);
  }
  sg::doNotOptimize(work);
  const auto& window = watchedWindow();
  if (window.reads > 0) {
    waitForProbeReads(window.cpus, window.reads);
  }
}

// The step the scripted clock paces the shared tsc series at: the probe's
// unit target `kTargetUnitTimeNs`
// (`include/speedgun-ng/benchmark.hpp:2444`). The calibration settles at
// one read per unit at this step, so the probe reads many times inside a
// timed window and each entry's windows keep samples of their own
// (FR-006, R-11).
inline constexpr std::uint64_t kPacedStepNs = 1000;

// The iteration count of an entry whose own windows must hold probe
// samples. The wait described beside `bmQuiet` places the probe's reads
// inside the window, so the count stands at the length the batch's other
// figures read (FR-006, FR-020, R-11).
inline constexpr std::uint64_t kWatchedEntryIterations = 100000;
// The repetitions the scenarios that read an entry's own series run. Each
// repetition stamps a window of its own, and the armed wait ends inside
// every window the walk reaches, so each window carries samples of its own
// (FR-006, FR-020, R-11).
inline constexpr std::uint64_t kWatchedEntryRepetitions = 16;
// The work count the scenarios that read an entry's own series name to the
// probe. The unit body of 4,096 passes reads in 1.5 microseconds on the
// acceptance host, and the 1,048,576-entry series buffer spans 1.6 s of
// the batch, so the buffer holds the batch's samples with room to spare
// (FR-006, FR-014, R-11).
inline constexpr std::uint64_t kWatchedEntryWork = 4096;

// The jump the gate body scripts into the shared tsc series' tail: one
// read stands a kept gap above the risen threshold, and every read
// after it jumps again until the run ends, because the window close
// must read the same tail the kept records ride (R-02, FR-001).
inline constexpr std::uint64_t kSeriesJump = 1000000;

// The scripted provider: the leaves the runner resolves its plans
// through, plus the machine/tsc leaf the probe opens, declared in the
// ReadMode the probe's catalog check reads (R-02). The tsc series steps
// at `kPacedStepNs`, below the risen threshold, so no read stands a gap
// and every field of the fold lands at the empty kept set's zeros.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
    ref.setPoints("machine", "monotonic", {0}, 4000);
    ref.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    ref.setPoints("machine", "thread_cpu", {0}, 2000);
    ref.addCounter("machine",
                   "tsc",
                   "none",
                   "raw tick counter",
                   sg::counters::Availability::COUNTABLE,
                   sg::counters::ReadMode::FAST_TSC);
    ref.setPoints("machine", "tsc", {0}, kPacedStepNs);
    // The multi-pass family's event source: two scripted leaves on one
    // object whose scripted limit holds one counter, so the planner
    // opens one pass per column (E-01, FR-004). Every scripted fact
    // stands here, before the registration: a leaf declared after the
    // system opens the tree stays invisible to it.
    ref.addObject("cpu", "pmu", "the multi-pass family's event source");
    ref.setProgrammableCounters("cpu", 1);
    ref.addCounter("cpu", "multi0", "none", "the first column's leaf");
    ref.setPoints("cpu", "multi0", {0}, 1000);
    ref.addCounter("cpu", "multi1", "none", "the second column's leaf");
    ref.setPoints("cpu", "multi1", {0}, 1000);
    const auto registered =
        sg::counters::System::local().registerProvider(std::move(owned));
    check(registered.has_value(), "the scripted provider registers (R-02)");
    return &ref;
  }();
  return *provider;
}

// The jump the gate body scripts, mutable so the comparison scenarios of
// US4 script two different kept-gap magnitudes; every other scenario
// keeps the constexpr default (R-02). The function-local static keeps
// the mutable state out of namespace scope.
auto seriesJump() -> std::uint64_t&
{
  static std::uint64_t value = kSeriesJump;
  return value;
}

// The counting reader the gate waits end on: it stands where the probe's
// `machine/tsc` reader stands through the reader-install seam, counts one
// increment per read on one atomic, and delegates to windows the fake
// provider's `open()` returns over the same leaf, so the scripted series,
// the tail jumps, and the disclosure behave exactly as today (FR-012).
//
// One delegate window per probe thread: the probe's default path gives
// every item a window of its own, whose cursor advances by that item's
// reads alone, so a cpu's gap stays the scripted tail and never the count
// of another cpu's reads in between (FR-018). One shared delegate would
// make the two cpus' gaps measure their interleaving, and the quiet
// entry's windows would keep the interleaving gaps the run must not
// carry.
class CountingReader final : public sg::counters::WindowReader
{
public:
  explicit CountingReader(sg::counters::FakeProvider& provider) noexcept
      : m_provider {provider}
  {
  }

  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    const auto self = std::this_thread::get_id();
    sg::counters::WindowReader* delegate = nullptr;
    {
      const std::scoped_lock guard {m_mutex};
      auto& held = m_delegates[self];
      if (held == nullptr) {
        held = m_provider.open(
            sg::counters::LeafSet {.addresses = {"machine/tsc"}},
            sg::counters::Target {});
      }
      delegate = held.get();
    }
    m_count.fetch_add(1, std::memory_order_relaxed);
    delegate->readPoints(sink);
    // The ledger stands after the scripted read, so a wait that ends here
    // ends behind a stamp the leaf already carries.
    noteSeamRead();
  }

  [[nodiscard]] auto count() const noexcept -> std::uint64_t
  {
    return m_count.load(std::memory_order_relaxed);
  }

private:
  sg::counters::FakeProvider& m_provider;
  std::mutex m_mutex;
  std::map<std::thread::id, std::unique_ptr<sg::counters::WindowReader>>
      m_delegates;
  std::atomic<std::uint64_t> m_count {0};
};

// The installed counting reader and the cpu count the run armed: the
// scenarios install before a gate-body run and clear after it, and the
// gate bodies read both through the accessors beside the state (FR-012).
struct GateReaderState
{
  CountingReader* reader = nullptr;
  std::size_t armedCpus = 0;
};

auto gateReader() -> GateReaderState&
{
  static GateReaderState state;
  return state;
}

// The probe's own read count the waits end on. A body that runs with no
// counting reader installed reports the failure and never dereferences a
// null pointer (FR-012).
auto gateProbeReads() -> std::uint64_t
{
  auto& state = gateReader();
  check(state.reader != nullptr,
        "the gate body runs with no counting reader installed (FR-012)");
  return state.reader->count();
}

// The armed cpu count the wait targets multiply (FR-012).
auto gateArmedCpus() -> std::size_t
{
  auto& state = gateReader();
  check(state.reader != nullptr,
        "the gate body runs with no counting reader installed (FR-012)");
  return state.armedCpus;
}

// The scenario-side install: the reader arms over the `machine/tsc` leaf
// the fake provider's `open()` returns, after the scenario's `setPoints`
// calls, and the cpu count the run arms with stands beside it. The
// scenario clears both after its run, the way the reader-install seam's
// null pointer clears (FR-012).
auto installGateReader(std::size_t armedCpus) -> void
{
  auto& provider = scriptedProvider();
  auto& state = gateReader();
  check(state.reader == nullptr,
        "the counting reader installs over no earlier install (FR-012)");
  state.reader = new CountingReader(provider);
  state.armedCpus = armedCpus;
  sg::detail::installTestReader(
      std::unique_ptr<sg::counters::WindowReader> {state.reader});
}

auto clearGateReader() -> void
{
  auto& state = gateReader();
  // The run's session moved the reader out of the seam's slot at start
  // and owns it behind the join edge, so the clear empties the slot and
  // the state's pointer; the reader itself stands owned where the session
  // put it (R-02, Q-5).
  sg::detail::installTestReader(nullptr);
  state.reader = nullptr;
  state.armedCpus = 0;
}

// The gate benchmark: the body scripts the jump into the shared series
// inside its own timed window, then restores the paced tail after its
// last measured iteration. The window close reads the shared cursor the
// jumped reads pushed up, so the close stamp stands above every kept
// stamp of this window by construction, and the series stands paced for
// every entry the walk reaches next (specs/027 R-02, FR-039). The two
// waits end on the counting reader's count of the probe's own reads, so
// no read the runner or any other window made can end a wait the
// probe's rides must end (FR-012).
auto bmGate(sg::State& state) -> void
{
  auto& provider = scriptedProvider();
  // The pass wait: the measurement pass's reads stand behind the latch
  // before the first body call, so the wait stands as an ordering
  // signal whose target is met at entry and costs no spin (E-05,
  // specs/027 FR-007, R-05).
  const auto base = gateProbeReads();
  std::uint64_t spins = 0;
  while (gateProbeReads() < base + sg::detail::kPassReads * gateArmedCpus()) {
    // The wait yields, the way the probe test's waits do, so the pinned
    // probe thread progresses under concurrent load; the bound turns a
    // stalled series into a reported failure; the wait cannot hang
    // (FR-039).
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the probe's measurement pass never finished (FR-039)");
    }
  }
  // The jump wait: one pass's worth of the probe's own reads ride the
  // jumped tail inside this body's window, so each armed cpu keeps its
  // records at the jump's magnitude (FR-039, R-04).
  provider.setPoints("machine", "tsc", {}, seriesJump());
  const auto jumped = gateProbeReads();
  spins = 0;
  while (gateProbeReads() < jumped + sg::detail::kPassReads * gateArmedCpus()) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the scripted jump never reached the probe's reads (FR-039)");
    }
  }
  for (auto _ : state) {
  }
  // The restore stands after the last measured iteration and ahead of
  // the window close. The close edge reads the shared cursor the jumped
  // reads pushed up, so the close stamp stands above every kept stamp
  // of this window, and no later entry's windows open on a jumped tail
  // (FR-039, R-04).
  provider.setPoints("machine", "tsc", {}, kPacedStepNs);
}

// The body-call count of the multi-pass gate, mutable so the T047
// scenario resets it before its run: the k-th call scripts the k-th
// pass's jump magnitude. The function-local static keeps the mutable
// state out of namespace scope, the way `seriesJump` keeps its own.
auto passGateCalls() -> std::uint64_t&
{
  static std::uint64_t calls = 0;
  return calls;
}

// The multi-pass gate: the body of the family whose two metric columns
// force the passes method (specs/027 FR-022, E-01). Each call is one
// pass's one measured attempt. The call first restores the paced tail
// and waits for one of the probe's own reads per armed cpu, so the reads
// between the passes stand the paced step apart and keep no gap
// (FR-022, R-04), and the close edge forks from a cursor those
// hand-offs carry back above every kept stamp: a close forked from the
// reset cursor would read below the open stamp and wrap the window's
// runtime (R-04). The call then scripts the jump magnitude of its own
// pass and waits for one pass's worth of the probe's own reads to ride
// that tail inside its own timed window, so the kept set of the pass's
// window carries the pass's magnitude alone (FR-022, R-04).
//
// Both waits end on the counting reader's count of the probe's own
// reads (FR-012), and the counting reader gives every probe cpu a
// window of its own, so each cpu's gap stands at the scripted tail and
// no margin sizes against the read counts of the other cpus. The rides
// at the pass's tail keep their gaps, so the scenarios that run this
// body size the gap buffer beside the run (FR-010).
auto bmPassGate(sg::State& state) -> void
{
  auto& provider = scriptedProvider();
  const auto pass = ++passGateCalls();
  provider.setPoints("machine", "tsc", {}, kPacedStepNs);
  const auto rebased = gateProbeReads();
  std::uint64_t spins = 0;
  while (gateProbeReads() < rebased + gateArmedCpus()) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the probe's reads never resumed the paced tail (FR-039)");
    }
  }
  provider.setPoints("machine", "tsc", {}, pass * kSeriesJump);
  const auto jumped = gateProbeReads();
  spins = 0;
  while (gateProbeReads() < jumped + sg::detail::kPassReads * gateArmedCpus()) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the probe's reads never rode the pass's jump (FR-039)");
    }
  }
  for (auto _ : state) {
  }
  // The paced restore stands where the single-pass gate's does, so the
  // close edge reads the cursor the pass's jumped reads pushed up and
  // the tail stands paced for the entries the walk reaches next
  // (FR-022, R-04).
  provider.setPoints("machine", "tsc", {}, kPacedStepNs);
}

// The cpu indexes the scenarios pin probe threads to, taken from the
// process affinity mask, each below the online count (FR-040).
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  const int online = CPU_COUNT(&mask);
  check(online > 0, "the affinity mask holds cpus (FR-040)");
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask)) {
      check(index < static_cast<std::size_t>(online),
            "a picked cpu stands below the online count (FR-040)");
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (FR-040)");
  return cpus;
}

// One speedgunMain run with stdout captured, the way the gap test
// captures: the report lands in a scratch file beside the redirected
// descriptor, and the status must follow contracts/cli.md (FR-036).
auto captureRun(const std::vector<std::string>& arguments,
                const int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_window_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_window_out_" + std::to_string(++runIndex) + ".txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path.c_str());
  return buffer.str();
}

auto readDocument(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// One speedgunMain run with stderr captured: the comparison run's usage
// errors print their one line there (FR-032, FR-038), the way the option
// rejections of the cli test do.
auto captureError(const std::vector<std::string>& arguments,
                  const int expected) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_window_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_window_err_" + std::to_string(++runIndex) + ".txt";
  const int saved = dup(STDERR_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stderr) == nullptr) {
    fail("cannot redirect the failure note");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stderr);
  if (dup2(saved, STDERR_FILENO) < 0) {
    fail("cannot restore stderr");
  }
  close(saved);
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path.c_str());
  return buffer.str();
}

// The document with the digits of one integral key's value replaced, the
// first occurrence of the key at or after `from`: the comparison
// scenarios script one report's value the way the reader scenarios
// script a broken document, and the stats integers stand as bare digits
// (E-03).
auto withIntegerReplaced(const std::string& document,
                         std::size_t from,
                         const char* key,
                         std::uint64_t value) -> std::string
{
  const auto at = document.find(std::string("\"") + key + "\":", from);
  check(at != std::string::npos, "the document carries the key");
  const auto digitsEnd =
      document.find_first_not_of("0123456789", at + std::strlen(key) + 3);
  check(digitsEnd != std::string::npos, "the key carries a digit run");
  std::string replaced = document.substr(0, at + std::strlen(key) + 3);
  replaced += std::to_string(value);
  replaced += document.substr(digitsEnd);
  return replaced;
}

// The sixteen stats fields in the E-03 order, the canonical enumeration
// the console block's lines and the JSON writer's keys follow.
constexpr std::array<std::string_view, 16> kStatFields {"gapCount",
                                                        "gapsPerSecond",
                                                        "minNs",
                                                        "medianNs",
                                                        "meanNs",
                                                        "p90Ns",
                                                        "p99Ns",
                                                        "p999Ns",
                                                        "p9999Ns",
                                                        "maxNs",
                                                        "totalNs",
                                                        "totalPercent",
                                                        "runtimeNs",
                                                        "thresholdNs",
                                                        "loopPeriodNs",
                                                        "droppedGaps"};

// specs/043 T033, FR-019, R-14: the 30 series lines the console gains, in
// the standing order: the calibration group of four, the 16 figures of
// FR-021, then the five periods and their five powers.
constexpr std::array<std::string_view, 30> kSeriesLines {
    "method",       "workCount",    "targetNs",        "calibrationSpanNs",
    "sampleCount",  "minValue",     "medianValue",     "meanValue",
    "p90Value",     "p99Value",     "p999Value",       "p9999Value",
    "maxValue",     "noisePercent", "seriesRuntimeNs", "droppedSamples",
    "unitTimeNs",   "quantumNs",    "resampleStepNs",  "spectrumPoints",
    "period1Ns",    "period2Ns",    "period3Ns",       "period4Ns",
    "period5Ns",    "period1Power", "period2Power",    "period3Power",
    "period4Power", "period5Power"};

// specs/043 T033, FR-019: the line that opens with one field name, and the
// count of lines that do: a field the block prints twice stands caught by
// the count, and a field it leaves out by the position.
auto fieldLineIndex(const std::vector<std::string>& lines,
                    const std::string& field,
                    std::size_t& occurrences) -> std::size_t
{
  const std::string head = field + ":";
  std::size_t found = 0;
  occurrences = 0;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines.at(index).rfind(head, 0) == 0) {
      if (occurrences == 0) {
        found = index;
      }
      ++occurrences;
    }
  }
  return found;
}

// specs/043 T033, FR-019: the count of columns one field line carries: the
// tokens after the field's colon, the block printing one per probe cpu.
auto fieldColumns(const std::vector<std::string>& lines,
                  std::size_t index,
                  const std::string& field) -> std::size_t
{
  const std::string line = lines.at(index);
  check(line.rfind(field + ":", 0) == 0, "the field line names its field");
  std::size_t columns = 0;
  std::istringstream stream(line.substr(field.size() + 1));
  std::string token;
  while (stream >> token) {
    ++columns;
  }
  return columns;
}

auto splitLines(const std::string& text) -> std::vector<std::string>
{
  std::istringstream stream(text);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

// The unsigned value one integral JSON key carries, read as the digit
// run that follows the key's colon from `from` on: the writer writes
// the stats integers as bare digits (E-03).
auto integerValueAfter(const std::string& document,
                       std::size_t from,
                       const char* key) -> std::uint64_t
{
  const auto at = document.find(std::string("\"") + key + "\":", from);
  check(at != std::string::npos, "the document carries the key");
  auto cursor = at + std::strlen(key) + 3;
  std::uint64_t value = 0;
  while (std::isdigit(static_cast<unsigned char>(document.at(cursor))) != 0) {
    value = value * 10 + static_cast<std::uint64_t>(document.at(cursor) - '0');
    ++cursor;
  }
  return value;
}

// The real value one JSON key carries, read as the text that follows
// the key's colon from `from` on: the writer writes the three real
// stats fields through the shortest text that reads back the same
// double (E-03), so this parse returns the value the loaded record
// holds.
auto doubleValueAfter(const std::string& document,
                      std::size_t from,
                      const char* key) -> double
{
  const auto at = document.find(std::string("\"") + key + "\":", from);
  check(at != std::string::npos, "the document carries the key");
  const char* begin = document.c_str() + at + std::strlen(key) + 3;
  char* end = nullptr;
  const double value = std::strtod(begin, &end);
  check(end != begin, "the key carries a number");
  return value;
}

// The end of the object that opens at `at`: the position of its closing
// brace, so a key search inside the object stops at its border (FR-006).
auto objectEndAt(const std::string& document, std::size_t at) -> std::size_t
{
  std::size_t depth = 0;
  for (std::size_t i = at; i < document.size(); ++i) {
    if (document.at(i) == '{') {
      ++depth;
    } else if (document.at(i) == '}' && --depth == 0) {
      return i;
    }
  }
  return std::string::npos;
}

// The unsigned value one integral key carries inside the object that
// spans `from` to `to`, and 0 for a key the object leaves out: the
// writer writes the two method figures only for the method that uses
// them, and the reader answers 0 for a key that stands absent (FR-006).
auto integerValueIn(const std::string& document,
                    std::size_t from,
                    std::size_t to,
                    const char* key) -> std::uint64_t
{
  const auto at = document.find(std::string("\"") + key + "\":", from);
  if (at == std::string::npos || at > to) {
    return 0;
  }
  return integerValueAfter(document, at, key);
}

// The loaded result whose name the entry carries, the lookup the
// round-trip scenario compares the entries through (FR-029).
auto resultNamed(const std::vector<sg::BenchmarkResult>& results,
                 std::string_view name) -> const sg::BenchmarkResult*
{
  for (const auto& result : results) {
    if (result.name == name) {
      return &result;
    }
  }
  return nullptr;
}

// The sixteen fields of one loaded per-cpu stats object against the
// object the document carries at `objectAt`: the writer's value for
// every field, the integers as bare digits and the reals as the
// shortest text that reads back the same double (E-03, SC-001).
auto checkStatsRoundTrip(const sg::JitterStats& stats,
                         const std::string& document,
                         std::size_t objectAt,
                         const char* what) -> void
{
  check(integerValueAfter(document, objectAt, "gapCount") == stats.gapCount,
        what);
  check(std::fabs(doubleValueAfter(document, objectAt, "gapsPerSecond")
                  - stats.gapsPerSecond)
            <= 1e-9,
        what);
  check(integerValueAfter(document, objectAt, "minNs") == stats.minNs, what);
  check(integerValueAfter(document, objectAt, "medianNs") == stats.medianNs,
        what);
  check(std::fabs(doubleValueAfter(document, objectAt, "meanNs") - stats.meanNs)
            <= 1e-9,
        what);
  check(integerValueAfter(document, objectAt, "p90Ns") == stats.p90Ns, what);
  check(integerValueAfter(document, objectAt, "p99Ns") == stats.p99Ns, what);
  check(integerValueAfter(document, objectAt, "p999Ns") == stats.p999Ns, what);
  check(integerValueAfter(document, objectAt, "p9999Ns") == stats.p9999Ns,
        what);
  check(integerValueAfter(document, objectAt, "maxNs") == stats.maxNs, what);
  check(integerValueAfter(document, objectAt, "totalNs") == stats.totalNs,
        what);
  check(std::fabs(doubleValueAfter(document, objectAt, "totalPercent")
                  - stats.totalPercent)
            <= 1e-9,
        what);
  check(integerValueAfter(document, objectAt, "runtimeNs") == stats.runtimeNs,
        what);
  check(
      integerValueAfter(document, objectAt, "thresholdNs") == stats.thresholdNs,
      what);
  check(integerValueAfter(document, objectAt, "loopPeriodNs")
            == stats.loopPeriodNs,
        what);
  check(
      integerValueAfter(document, objectAt, "droppedGaps") == stats.droppedGaps,
      what);
}

// specs/043 T023, FR-006, E-23: the 21 keys of one closed series item, in
// the order the writer writes them. The item stands closed, so the key set
// and the key order of the written item read against this list.
constexpr std::array<std::string_view, 21> kSeriesKeys {"cpu",
                                                        "workCount",
                                                        "unitTimeNs",
                                                        "targetNs",
                                                        "quantumNs",
                                                        "calibrationSpanNs",
                                                        "sampleCount",
                                                        "minValue",
                                                        "medianValue",
                                                        "meanValue",
                                                        "p90Value",
                                                        "p99Value",
                                                        "p999Value",
                                                        "p9999Value",
                                                        "maxValue",
                                                        "noisePercent",
                                                        "seriesRuntimeNs",
                                                        "droppedSamples",
                                                        "resampleStepNs",
                                                        "spectrumPoints",
                                                        "periods"};

// The keys of the JSON object that opens at `objectAt`, in the order they
// stand: the walk counts the braces and the brackets, so a key of a nested
// object stands out of the answer and the walk stops at the object's own
// close (FR-006).
auto jsonKeysAt(const std::string& text,
                std::size_t objectAt) -> std::vector<std::string>
{
  std::vector<std::string> keys;
  std::size_t depth = 0;
  for (std::size_t at = objectAt; at < text.size(); ++at) {
    const char edge = text.at(at);
    if (edge == '{' || edge == '[') {
      ++depth;
      continue;
    }
    if (edge == '}' || edge == ']') {
      if (--depth == 0) {
        break;
      }
      continue;
    }
    if (depth != 1 || edge != '"') {
      continue;
    }
    const auto close = text.find('"', at + 1);
    check(close != std::string::npos, "a quoted key closes (FR-006)");
    if (text.find(':', close + 1) == close + 1) {
      keys.push_back(text.substr(at + 1, close - at - 1));
    }
    at = close;
  }
  return keys;
}

// The keys the closed series item carries, in the order the writer writes
// them, with the one key the method leaves out: `quantumNs` for a FWQ run
// and `resampleStepNs` for a FTQ run (FR-006, E-22, E-23).
auto wantedSeriesKeys(std::string_view skip) -> std::vector<std::string>
{
  std::vector<std::string> wanted;
  for (const auto key : kSeriesKeys) {
    if (key != skip) {
      wanted.emplace_back(key);
    }
  }
  return wanted;
}

// The first item of the series array the document carries at or after
// `from`: the position of its opening brace (FR-006, FR-020).
auto seriesItemAt(const std::string& document, std::size_t from) -> std::size_t
{
  const auto at = document.find("\"series\":[{", from);
  check(at != std::string::npos, "the object carries a series array (FR-006)");
  return at + std::strlen("\"series\":[");
}

// The 19 figures and the period list of one loaded series record against
// the item the document carries at `itemAt`: the writer's value for every
// key, the integers as bare digits and the reals as the shortest text that
// reads back the same double (FR-020, SC-001).
auto checkSeriesRoundTrip(const sg::JitterSeriesStats& series,
                          const std::string& document,
                          std::size_t itemAt,
                          const char* what) -> void
{
  const auto itemEnd = objectEndAt(document, itemAt);
  check(itemEnd != std::string::npos, what);
  check(integerValueIn(document, itemAt, itemEnd, "workCount")
            == series.workCount,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "unitTimeNs")
            == series.unitTimeNs,
        what);
  check(
      integerValueIn(document, itemAt, itemEnd, "targetNs") == series.targetNs,
      what);
  check(integerValueIn(document, itemAt, itemEnd, "quantumNs")
            == series.quantumNs,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "calibrationSpanNs")
            == series.calibrationSpanNs,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "sampleCount")
            == series.sampleCount,
        what);
  check(
      integerValueIn(document, itemAt, itemEnd, "minValue") == series.minValue,
      what);
  check(integerValueIn(document, itemAt, itemEnd, "medianValue")
            == series.medianValue,
        what);
  check(std::fabs(doubleValueAfter(document, itemAt, "meanValue")
                  - series.meanValue)
            <= 1e-9,
        what);
  check(
      integerValueIn(document, itemAt, itemEnd, "p90Value") == series.p90Value,
      what);
  check(
      integerValueIn(document, itemAt, itemEnd, "p99Value") == series.p99Value,
      what);
  check(integerValueIn(document, itemAt, itemEnd, "p999Value")
            == series.p999Value,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "p9999Value")
            == series.p9999Value,
        what);
  check(
      integerValueIn(document, itemAt, itemEnd, "maxValue") == series.maxValue,
      what);
  check(std::fabs(doubleValueAfter(document, itemAt, "noisePercent")
                  - series.noisePercent)
            <= 1e-9,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "seriesRuntimeNs")
            == series.seriesRuntimeNs,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "droppedSamples")
            == series.droppedSamples,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "resampleStepNs")
            == series.resampleStepNs,
        what);
  check(integerValueIn(document, itemAt, itemEnd, "spectrumPoints")
            == series.spectrumPoints,
        what);
  const auto periodsAt = document.find("\"periods\":[", itemAt);
  if (periodsAt == std::string::npos || periodsAt > itemEnd) {
    // The per-entry item carries no period list, so the reading answers
    // none (FR-020).
    check(series.periods.empty(), what);
    return;
  }
  const auto itemsAt = periodsAt + std::strlen("\"periods\":");
  const auto close = document.find(']', itemsAt);
  check(close != std::string::npos, what);
  std::size_t index = 0;
  for (auto at = document.find('{', itemsAt);
       at != std::string::npos && at < close;
       at = document.find('{', at + 1))
  {
    check(index < series.periods.size(), what);
    check(integerValueAfter(document, at, "periodNs")
              == series.periods.at(index).periodNs,
          what);
    check(std::fabs(doubleValueAfter(document, at, "powerRatio")
                    - series.periods.at(index).powerRatio)
              <= 1e-9,
          what);
    ++index;
  }
  check(index == series.periods.size(), what);
}

// The flat clock: a stamp that never moves, so every unit time the FWQ
// window keeps stands 0 and every FTQ quantum stands open. The two guards
// of the period list read these runs: the median unit time of 0, and the
// kept set of no sample (FR-017, FR-018, R-12).
class FlatSeriesReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    sink.put(0);
    noteSeamRead();
  }
};

// specs/043 T033, FR-019: the dipping clock the console block reads. Each
// thread's stamps step 1,000 ns per read, and every 16th read steps nine
// thousand ns more, so a FWQ window keeps unit times with a dip at a
// 16-unit period: the median unit time stands 1,000, the resample spreads
// each dipped unit over ten bins, and the transform names the period the
// cycle carries (FR-012, FR-017, FR-018). The per-thread state keeps the
// two probe cpus' series apart, the way the seam's other readers do (R-02).
class DipSeriesReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    thread_local std::uint64_t calls = 0;
    thread_local std::uint64_t stamp = 0;
    ++calls;
    stamp += kStepNs;
    if (calls % kDipEvery == 0) {
      stamp += kDipExtraNs;
    }
    sink.put(stamp);
    noteSeamRead();
  }

  static inline constexpr std::uint64_t kStepNs = 1000;
  static inline constexpr std::uint64_t kDipEvery = 16;
  static inline constexpr std::uint64_t kDipExtraNs = 9000;
};

// The square-wave clock the named-period scenario reads. Each thread's
// stamps step 1,000 ns for eight reads and 2,000 ns for the eight that
// follow, so a FWQ window keeps unit times that hold the median at 1,000
// and close 24 bins of that step per cycle: eight bins of the low reads
// and 16 of the high reads. The wave stands at two levels, so the
// strongest bin of the transform stands at the cycle itself (FR-017,
// FR-018). The per-thread state keeps the probe cpus' series apart, the
// way the seam's other readers do (R-02).
class WaveSeriesReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    thread_local std::uint64_t calls = 0;
    thread_local std::uint64_t stamp = 0;
    ++calls;
    stamp += calls % kCycle < kHalfCycle ? kLowNs : kHighNs;
    sink.put(stamp);
    noteSeamRead();
  }

  static inline constexpr std::uint64_t kCycle = 16;
  static inline constexpr std::uint64_t kHalfCycle = 8;
  static inline constexpr std::uint64_t kLowNs = 1000;
  static inline constexpr std::uint64_t kHighNs = 2000;
};

// The per-thread stamp state of the seam reader below: each probe
// thread carries its own series, so the two cpus' reads interleave
// without sharing a cursor (R-02, FR-039). The probe threads are new
// threads of the run, so the state starts fresh for every scenario.
struct GapThreadState
{
  std::uint64_t calls = 0;
  std::uint64_t stamp = 0;
};

thread_local GapThreadState gapThreadState;

// The per-thread stamp series the raw-gap scenario runs through the
// reader-install seam (R-02): the fake's one shared cursor cannot
// express a per-thread jump, because the run drives both probe threads'
// reads through it interleaved and the test cannot step between the
// phases. Each thread's series steps 1 through the measurement pass's
// 1,024 reads and jumps once at its 1,025th read, the first read of the
// pre-check phase, whose loop runs to its scripted span before any stop
// can matter, so each cpu keeps exactly one record under any load
// (FR-028, E-10, FR-014).
class GapSeriesReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    auto& thread = gapThreadState;
    ++thread.calls;
    thread.stamp += thread.calls == 1025 ? 1000000 : 1;
    sink.put(thread.stamp);
  }
};

// specs/043 T019, FR-020, R-11: the scripted series the two folds read.
// The series record carries one value and no stamp, so the fold
// recovers the stamp from the start: the FWQ stamp stands at the start
// plus the running sum of the unit times, and the FTQ stamp at the
// start plus the sample rank plus one, times the quantum. The four
// unit times below recover the stamps 1000, 3000, 4000 and 5000 from a
// start of 0, and the three counts recover 1100, 2100 and 3100 from a
// start of 100 at a 1 us quantum.
auto fillFwqSeries(sg::detail::ProbeCpuState& state) -> void
{
  state.cpu = 3;
  state.method = sg::JitterMethod::FWQ;
  state.series.assign({1000, 2000, 1000, 1000});
  state.seriesKept = 4;
  state.seriesStartNs = 0;
  state.workCount = 7;
  state.unitTimeNs = 1000;
  state.targetNs = 1000;
  state.quantumNs = 0;
  state.calibrationSpanNs = 500;
}

auto fillFtqSeries(sg::detail::ProbeCpuState& state) -> void
{
  state.cpu = 3;
  state.method = sg::JitterMethod::FTQ;
  state.series.assign({5, 0, 3});
  state.seriesKept = 3;
  state.seriesStartNs = 100;
  state.workCount = 40;
  state.unitTimeNs = 1;
  state.targetNs = 1000;
  state.quantumNs = 1000;
  state.calibrationSpanNs = 900;
}

// The one-window set the entry fold reads, in the shape the carrier of
// E-06 fills: the attempt attribution rides the window.
auto seriesWindow(std::uint64_t start,
                  std::uint64_t end) -> std::vector<sg::detail::TimedWindow>
{
  return {sg::detail::TimedWindow {
      .start = start,
      .end = end,
      .entry = 0,
      .repetition = 0,
      .chunk = 0,
      .pass = 0,
  }};
}

}  // namespace

SG_BENCHMARK(bmGate);

// specs/048-hwlat-probe T037: the detector lines of the probe block and of
// the pre-check block. The rows hand the two printers the state a detector
// run leaves behind, so they name no command-line word and start no probe
// thread (FR-016, FR-030, FR-031).

// One printer call with stdout captured into a string, the way `captureRun`
// captures a whole batch.
template<typename Call>
auto captureLines(const Call& call) -> std::string
{
  static int blockIndex = 0;
  const std::string path =
      "harness_jitter_window_block_" + std::to_string(++blockIndex) + ".txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
    fail("cannot redirect the printed block");
  }
  call();
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path.c_str());
  return buffer.str();
}

// The number of times one whole line stands in the block.
auto countLine(const std::string& block, std::string_view line) -> std::size_t
{
  std::size_t hits = 0;
  for (auto at = block.find(line); at != std::string::npos;
       at = block.find(line, at + line.size()))
  {
    ++hits;
  }
  return hits;
}

// One probe cpu's state as a detector run leaves it: two kept gaps with the
// change ring beside them, and the two counter reads that bound the run span.
// A run whose counter source answered nothing leaves the pair with no value
// at both edges and the change ring at zero, the shape FR-011 states (E-02,
// E-03, FR-012).
auto fillProbeState(sg::detail::ProbeCpuState& state,
                    const std::uint32_t cpu,
                    const sg::JitterMethod method,
                    const bool sourceStands) -> void
{
  state.cpu = cpu;
  state.method = method;
  state.buffer = {sg::detail::GapRecord {100, 210},
                  sg::detail::GapRecord {300, 280}};
  state.kept = 2;
  state.changes = sourceStands ? std::vector<std::uint64_t> {3, 0}
                               : std::vector<std::uint64_t> {0, 0};
  state.thresholdNs = 200;
  state.loopPeriodNs = 20;
  if (sourceStands) {
    state.edges.smiOpening = 10;
    state.edges.smiClosing = 11;
    state.edges.nmiOpening = 5;
    state.edges.nmiClosing = 9;
  }
}

// The probe block of a run over two probe cpus of one method, the pointers
// the printer takes standing beside the states of those two cpus (E-03).
auto probeBlockOf(const sg::JitterMethod method,
                  const bool sourceStands) -> std::string
{
  const auto cpus = pickCpus(2);
  sg::detail::ProbeCpuState first;
  sg::detail::ProbeCpuState second;
  fillProbeState(first, cpus.at(0), method, sourceStands);
  fillProbeState(second, cpus.at(1), method, sourceStands);
  const std::vector<const sg::detail::ProbeCpuState*> states {&first, &second};
  return captureLines([&] { sg::detail::printJitterReport(true, states); });
}

// The pre-check record of two measured cpus: the first carries the three
// attribution figures of a window whose sources answered, the second carries
// no delta and a zero count, the shape of a window whose source answered
// nothing (FR-016, FR-011).
auto attributionRecord() -> sg::PrecheckRecord
{
  const auto cpus = pickCpus(2);
  sg::PrecheckRecord record;
  record.windowMs = 10;
  sg::PrecheckCpuRecord first;
  first.cpu = cpus.at(0);
  first.planned = true;
  first.stats.gapCount = 2;
  first.stats.smiDelta = 1;
  first.stats.nmiDelta = 4;
  first.stats.smiGapCount = 2;
  first.attribution.interrupts = 5;
  first.attribution.switches = 1;
  first.attribution.schedules = 2;
  sg::PrecheckCpuRecord second;
  second.cpu = cpus.at(1);
  second.planned = true;
  second.stats.gapCount = 1;
  second.attribution.switches = 0;
  second.attribution.schedules = 0;
  record.cpus = {first, second};
  return record;
}

// FR-016, SC-006: the three figure lines stand after the standing
// `droppedGaps:` line, one column per probe cpu in the cpu order, each the
// change across the probe's own run span.
auto theThreeProbeFigureLinesStandAfterTheStandingCount() -> void
{
  const auto block = probeBlockOf(sg::JitterMethod::HWLAT, true);
  const auto dropped = block.find("droppedGaps:");
  const auto smi = block.find("smiDelta:");
  const auto nmi = block.find("nmiDelta:");
  const auto gaps = block.find("smiGapCount:");
  check(dropped != std::string::npos && smi > dropped && nmi > smi
            && gaps > nmi,
        "the three figure lines stand after the standing droppedGaps line in "
        "the order the stats field list states (FR-016)");
  check(block.find("smiDelta: 1 1\n") != std::string::npos
            && block.find("nmiDelta: 4 4\n") != std::string::npos
            && block.find("smiGapCount: 1 1\n") != std::string::npos,
        "each figure line carries one column per probe cpu: the SMI change, "
        "the NMI change and the count of marked gaps (FR-016, FR-013)");
  check(countLine(block, "jitter: SMI count not available") == 0
            && countLine(block, "jitter: NMI count not available") == 0,
        "a run whose two sources answered prints no disclosure line (FR-011)");
}

// FR-011: a source that answers nothing leaves its delta absent and its count
// zero, prints the standing token in the figure position, and discloses the
// source once for the run while the run keeps its status.
auto theTwoCounterDisclosuresPrintOncePerRun() -> void
{
  const auto block = probeBlockOf(sg::JitterMethod::HWLAT, false);
  check(countLine(block, "jitter: SMI count not available\n") == 1
            && countLine(block, "jitter: NMI count not available\n") == 1,
        "each source that answered nothing stands disclosed once for the run "
        "(FR-011)");
  check(block.find("smiDelta: not available not available\n")
                != std::string::npos,
        "the absent delta prints the standing not-available token in the "
        "figure position (FR-011, FR-015)");
  check(block.find("smiGapCount: 0 0\n") != std::string::npos,
        "the marked-gap count of a run whose source answered nothing folds to "
        "zero (FR-011, FR-013)");
}

// FR-030, FR-031: the limits line states the one limit of the user-space
// probe once per run of the detector method, the sentence the document
// carries under `jitter.limits`.
auto theLimitsLineStandsOncePerDetectorRun() -> void
{
  const auto block = probeBlockOf(sg::JitterMethod::HWLAT, true);
  check(countLine(block, "jitter: probe runs with interrupts enabled\n") == 1,
        "the limits line stands once per detector run (FR-030, FR-031)");
}

// FR-025, SC-006: a run of the standing method prints the bytes it printed
// before the detector existed, with none of the five new lines.
auto aStandingMethodPrintsNoDetectorLine() -> void
{
  const auto block = probeBlockOf(sg::JitterMethod::TSC, true);
  check(block.find("smiDelta") == std::string::npos
            && block.find("nmiDelta") == std::string::npos
            && block.find("smiGapCount") == std::string::npos
            && block.find("probe runs with interrupts enabled")
                    == std::string::npos
            && block.find("count not available") == std::string::npos,
        "a tsc run prints no figure line, no limits line and no counter "
        "disclosure (FR-025, FR-030, SC-006)");
  check(block.find("droppedGaps:") != std::string::npos
            && block.find("jitter: split and percentiles use the kept gaps "
                          "only\n") != std::string::npos,
        "the standing probe block stands whole over the detector method "
        "(FR-004, FR-029)");
}

// FR-016: the pre-check block prints the three attribution lines per measured
// cpu after the standing switch and schedule line, each naming its source
// file, and the count naming none.
auto theThreeAttributionLinesStandPerMeasuredCpu() -> void
{
  const auto cpus = pickCpus(2);
  const auto record = attributionRecord();
  const auto block = captureLines(
      [&]
      {
        sg::detail::printPrecheckReport(record,
                                        sg::TracerEngine::NONE,
                                        sg::kDefaultNoisePercent,
                                        sg::JitterMethod::HWLAT);
      });
  const std::string smiLine = "smi delta: 1 (source: /dev/cpu/"
      + std::to_string(cpus.at(0)) + "/msr)\n";
  const std::string absentLine = "smi delta: not available (source: /dev/cpu/"
      + std::to_string(cpus.at(1)) + "/msr)\n";
  check(block.find(smiLine) != std::string::npos,
        "the SMI delta of a measured cpu names the msr file behind it "
        "(FR-016)");
  check(block.find("nmi delta: 4 (source: /proc/interrupts)\n")
                != std::string::npos,
        "the NMI delta of a measured cpu names the interrupt list behind it "
        "(FR-016)");
  check(block.find("smi gaps: 2\n") != std::string::npos,
        "the marked-gap count of a measured cpu prints the count of its own "
        "kept gaps (FR-016, FR-013)");
  check(block.find("nmi delta: not available (source: /proc/interrupts)\n")
                != std::string::npos,
        "the cpu whose source answered nothing prints the standing token in "
        "the figure position (FR-011)");
  check(countLine(block, "smi gaps: 0\n") == 1
            && block.find(absentLine) != std::string::npos,
        "the cpu whose source answered nothing prints no delta and a zero "
        "count (FR-011)");
  const auto schedule = block.find("switch delta:");
  const auto smi = block.find("smi delta:");
  check(schedule != std::string::npos && smi > schedule,
        "the three lines stand after the standing switch and schedule line "
        "(FR-016, FR-030)");
}

// FR-025, SC-006: a pre-check block of the standing method keeps its former
// bytes, with none of the three attribution lines.
auto aStandingMethodPrintsNoAttributionLine() -> void
{
  const auto record = attributionRecord();
  const auto detector = captureLines(
      [&]
      {
        sg::detail::printPrecheckReport(record,
                                        sg::TracerEngine::NONE,
                                        sg::kDefaultNoisePercent,
                                        sg::JitterMethod::HWLAT);
      });
  const auto standing = captureLines(
      [&]
      {
        sg::detail::printPrecheckReport(record,
                                        sg::TracerEngine::NONE,
                                        sg::kDefaultNoisePercent,
                                        sg::JitterMethod::TSC);
      });
  check(standing.find("smi delta") == std::string::npos
            && standing.find("nmi delta") == std::string::npos
            && standing.find("smi gaps") == std::string::npos,
        "a pre-check block of the standing method prints none of the three "
        "attribution lines (FR-025, SC-006)");
  check(standing.find("switch delta:") != std::string::npos
            && standing.size() < detector.size(),
        "the standing block stands whole and shorter by the three lines the "
        "detector method adds (FR-004, FR-029)");
}

auto main() -> int
{
  scriptedProvider();
  // The multi-pass family the T047 scenario runs: two metric columns of
  // one scripted leaf each, attached here after the provider's
  // registration so the leaves resolve the way the pass tests resolve
  // theirs (E-01, FR-004).
  const auto cpuSource = sg::counters::System::local().object("cpu");
  check(cpuSource.has_value(), "the scripted cpu source resolves (FR-022)");
  const auto firstLeaf = cpuSource->counter<sg::counters::Dim<0, 1>>("multi0");
  check(firstLeaf.has_value(), "the first column's leaf resolves (FR-022)");
  const auto secondLeaf = cpuSource->counter<sg::counters::Dim<0, 1>>("multi1");
  check(secondLeaf.has_value(), "the second column's leaf resolves (FR-022)");
  auto passHandle = sg::registerBenchmark(&bmPassGate, "bmPassGate");
  passHandle.addMetric(
      sg::counters::Expression<sg::counters::Dim<0, 1>>(*firstLeaf), "first");
  passHandle.addMetric(
      sg::counters::Expression<sg::counters::Dim<0, 1>>(*secondLeaf), "second");
  // The walk order is the registration order, so the quiet entry stands
  // behind both gates and the walk stamps its windows after every gate
  // window closed. A kept gap's stamp never stands above the shared
  // cursor its own read left behind, so every gap the gates scripted
  // stands below the quiet entry's window start, and the paced tail the
  // gate bodies restore keeps the quiet entry's own windows gap-free on
  // every host (FR-027, E-08).
  [[maybe_unused]] auto quietHandle =
      sg::registerBenchmark(&bmQuiet, "bmQuiet");
  const auto cpus = pickCpus(2);
  const std::string cpuList =
      std::to_string(cpus.at(0)) + "," + std::to_string(cpus.at(1));
  const std::vector<std::string> quickRun {
      "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};

  // FR-013, SC-003: a run with no probe option prints the console
  // report with the fields and the shape master prints, and the JSON
  // writer of the same run carries no jitter key.
  {
    const auto report =
        captureRun({"--filter=bmQuiet", "--repetitions=1", "--iterations=2"});
    check(report.find("jitter probe:") == std::string::npos
              && report.find("jitter: ") == std::string::npos,
          "a probe-free run prints no probe block and no disclosure "
          "(FR-013, SC-003)");
    check(report.find("bmQuiet") != std::string::npos,
          "the probe-free run prints the benchmark (SC-003)");
  }

  // FR-026, SC-003, D-1: the probe-free document's key sequence equals
  // what master prints: the schema version, the run facts, the entries,
  // and no jitter key anywhere.
  {
    const std::string path = "harness_jitter_window_free.json";
    captureRun({"--filter=bmQuiet",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + path});
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto schema = document.find("\"schemaVersion\"");
    const auto speedgun = document.find("\"speedgun\"");
    const auto benchmarks = document.find("\"benchmarks\"");
    check(schema != std::string::npos && speedgun != std::string::npos
              && benchmarks != std::string::npos
              && schema < speedgun && speedgun < benchmarks,
          "the probe-free document keeps the master key sequence "
          "(FR-026, D-1)");
    check(document.find("\"jitter\"") == std::string::npos,
          "the probe-free document carries no jitter key (FR-013, E-07)");
  }

  // The two-cpu probe scenarios run the unpinned benchmark beside both
  // probe cpus, and the narrowed mask keeps a cpu only where the mask
  // holds a third one (FR-019). A mask that holds two prints the skip
  // and the single-cpu scenarios run on, the printed skip the fork
  // tests of dbc_test.cpp carry.
  cpu_set_t held {};
  CPU_ZERO(&held);
  check(sched_getaffinity(0, sizeof(held), &held) == 0,
        "the process affinity mask is readable (FR-040)");
  const bool twoCpuScenarios = CPU_COUNT(&held) >= 3;
  if (!twoCpuScenarios) {
    std::printf("the two-cpu probe scenarios: SKIPPED (the mask holds "
                "fewer than three cpus)\n");
  }
  if (twoCpuScenarios) {
    // FR-023, R-08: the console report of a run with the probe prints the
    // probe block after the benchmark tables, one column per probe cpu,
    // the sixteen stats fields in the E-03 order; the disclosure lines
    // follow in the end-of-report region (FR-036, D-8).
    {
      auto arguments = quickRun;
      arguments.push_back("--jitter-probe=" + cpuList);
      const auto probed = captureRun(arguments);
      const auto lines = splitLines(probed);

      std::size_t blockLine = 0;
      std::size_t lastBenchmarkLine = 0;
      for (std::size_t index = 0; index < lines.size(); ++index) {
        if (lines.at(index).rfind("bmQuiet", 0) == 0) {
          lastBenchmarkLine = index;
        }
        if (lines.at(index).rfind("jitter probe: cpus ", 0) == 0
            && blockLine == 0)
        {
          blockLine = index;
        }
      }
      check(blockLine > 0, "the run prints the probe block (FR-023, R-08)");
      check(blockLine > lastBenchmarkLine,
            "the probe block stands after the benchmark tables (FR-023, R-08)");
      check(lines.at(blockLine) == "jitter probe: cpus " + cpuList,
            "the probe block names one column per probe cpu (FR-023, R-08)");
      for (std::size_t field = 0; field < kStatFields.size(); ++field) {
        const auto& line = lines.at(blockLine + 1 + field);
        check(line.rfind(kStatFields.at(field), 0) == 0,
              "the block lists the stats fields in the E-03 order (R-08)");
        check(line.find_first_not_of(" \t", kStatFields.at(field).size())
                  != std::string::npos,
              "each stats line carries one value per probe cpu (FR-023)");
      }
      check(lines.at(blockLine + 14).find("10000") != std::string::npos,
            "the threshold field carries the risen figure of the paced pass "
            "(FR-007, R-08)");
      check(lines.at(blockLine + 15).find(" 1000 ") != std::string::npos,
            "the loop-period field carries the pass's minimum (E-05, R-05)");

      // FR-036, D-8, D-12: the disclosure lines stand in the
      // end-of-report region: the availability and the cpus, the measured
      // points, the per-cpu counts, and the kept-gaps-only rule.
      check(
          probed.find("jitter: probe on cpus " + cpuList) != std::string::npos,
          "the disclosure names the probe and the cpus (FR-036, E-09)");
      check(probed.find("jitter: loop period 1000 ns, threshold 10000 ns")
                != std::string::npos,
            "the disclosure carries the measured points (FR-007, E-09)");
      check(probed.find("jitter: split and percentiles use the kept gaps only")
                != std::string::npos,
            "the disclosure carries the kept-gaps-only rule (E-09)");
    }

    // FR-026, E-07, R-09: the JSON writer carries the run-level jitter
    // object in the E-07 key order after `speedgun` and before
    // `benchmarks`, and each benchmark entry the nullable per-entry key
    // after `noiseWarning` (E-08), standing null before the split ships.
    // Red against the writer T013 lands: the scope the wave still misses.
    {
      const std::string path = "harness_jitter_window_run.json";
      auto arguments = quickRun;
      arguments.push_back("--json=" + path);
      arguments.push_back("--jitter-probe=" + cpuList);
      captureRun(arguments);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto speedgun = document.find("\"speedgun\"");
      const auto jitter = document.find("\"jitter\"");
      const auto benchmarks = document.find("\"benchmarks\"");
      check(jitter != std::string::npos && jitter > speedgun
                && jitter < benchmarks,
            "the run-level jitter object stands after speedgun and before "
            "benchmarks (FR-026, E-07)");
      const std::string_view keys[] = {"mode",
                                       "thresholdNs",
                                       "loopPeriodNs",
                                       "cpus",
                                       "stats",
                                       "droppedGaps",
                                       "disclosure"};
      auto cursor = jitter;
      for (const auto key : keys) {
        cursor = document.find("\"" + std::string(key) + "\"", cursor);
        check(cursor != std::string::npos,
              "the jitter object carries its keys in the E-07 order (E-07)");
      }
      const auto noiseWarning = document.find("\"noiseWarning\"");
      check(noiseWarning != std::string::npos,
            "the entry carries its noise warning (FR-027)");
      const auto entryJitter = document.find("\"jitter\"", noiseWarning);
      check(entryJitter != std::string::npos,
            "the per-entry key stands after noiseWarning (FR-027, E-08)");
    }

    // FR-029, SC-001, convergence T050: the reader reads the written
    // file back with every jitter field equal to the value the writer
    // produced. The comparison parses the written document's own values
    // and holds them against the loaded record, so no host-measured
    // figure stands as a literal here. The run carries the probe over
    // the quiet entry and the two gate entries, so the loaded record
    // holds one fold per probe cpu at the run level and at the entries
    // whose windows kept gaps, and the quiet entry loads none (FR-027,
    // E-08). The walk reaches the quiet entry last, on the paced tail
    // the gate bodies restore, so its windows hold no kept gap on any
    // host. The mode and the disclosure stand in the document alone,
    // the shape the reader reads them with, so the scenario holds
    // their text against the run's own facts (E-07).
    {
      // The ride margins the multi-pass gate's body waits on keep
      // their gaps, so the run's gap buffer carries them beside the
      // gate's own (FR-010).
      auto& provider = scriptedProvider();
      provider.setPoints("machine", "tsc", {}, kPacedStepNs);
      const std::string path = "harness_jitter_window_roundtrip.json";
      installGateReader(2);
      captureRun({"--repetitions=1",
                  "--iterations=" + std::to_string(kWatchedEntryIterations),
                  "--json=" + path,
                  "--jitter-probe=" + cpuList + ",gaps=8388608"});
      clearGateReader();
      const auto document = readDocument(path);
      const auto loaded = sg::loadReport(path);
      std::remove(path.c_str());
      check(loaded.has_value(),
            "the document of a probed run loads (FR-029, SC-001)");
      const auto& folds = loaded->runJitterStats;
      check(
          folds.has_value(),
          "the loaded document carries the run-level record (FR-029, " "E-07)");
      check(folds->size() == 2,
            "the run-level record carries one fold per probe cpu (E-07)");
      check(folds->at(0).cpu == cpus.at(0) && folds->at(1).cpu == cpus.at(1),
            "the run-level folds carry the probe cpus ascending (E-07)");

      const auto rootJitter = document.find("\"jitter\"");
      check(rootJitter != std::string::npos,
            "the document carries the root jitter object (E-07)");
      check(document.find("\"mode\":\"ALONGSIDE\"", rootJitter)
                != std::string::npos,
            "the root record carries the mode the run ran (E-07)");
      check(document.find("\"cpus\":[" + cpuList + "]", rootJitter)
                != std::string::npos,
            "the root record names the probe cpus (E-07)");
      const auto rootThreshold =
          integerValueAfter(document, rootJitter, "thresholdNs");
      const auto rootPeriod =
          integerValueAfter(document, rootJitter, "loopPeriodNs");
      const auto rootStats = document.find("\"stats\":", rootJitter);
      check(rootStats != std::string::npos,
            "the root record carries the stats array (E-07)");
      for (std::size_t index = 0; index < folds->size(); ++index) {
        const auto& fold = folds->at(index);
        check(fold.stats.thresholdNs == rootThreshold
                  && fold.stats.loopPeriodNs == rootPeriod,
              "every fold carries the measured points the root record "
              "names (E-07)");
        const auto objectAt =
            document.find("\"cpu\":" + std::to_string(fold.cpu), rootStats);
        check(objectAt != std::string::npos,
              "the root stats array carries the fold's cpu (E-07)");
        checkStatsRoundTrip(fold.stats,
                            document,
                            objectAt,
                            "the run-level fold carries the writer's sixteen "
                            "fields " "(FR-029, SC-001)");
      }
      const auto disclosureAt = document.find("\"disclosure\"", rootJitter);
      check(disclosureAt != std::string::npos,
            "the root record carries the disclosure (E-07)");
      const auto rootDroppedAt =
          document.rfind("\"droppedGaps\":", disclosureAt);
      check(rootDroppedAt != std::string::npos,
            "the root record carries the dropped count (E-07)");
      check(integerValueAfter(document, rootDroppedAt, "droppedGaps")
                == (folds->at(0).stats.droppedGaps
                    + folds->at(1).stats.droppedGaps),
            "the root dropped count sums the folds' own counts (E-07)");
      check(document.find("probe on cpus " + cpuList, rootJitter)
                != std::string::npos,
            "the disclosure names the probe and the cpus (E-07, E-09)");
      check(document.find("split and percentiles use the kept gaps only",
                          rootJitter)
                != std::string::npos,
            "the disclosure carries the kept-gaps-only rule (E-09)");

      const auto* quiet = resultNamed(loaded->results, "bmQuiet");
      check(
          quiet != nullptr && quiet->jitterFolds.empty(),
          "the entry whose windows kept no gap loads no fold (FR-027, " "E-"
                                                                        "08)");
      const char* gateNames[] {"bmGate", "bmPassGate"};
      for (const char* gateName : gateNames) {
        const auto* gate = resultNamed(loaded->results, gateName);
        check(gate != nullptr && !gate->jitterFolds.empty(),
              "the gate entry loads the folds of the probe cpus whose "
              "windows kept gaps (FR-027, E-08)");
        for (std::size_t index = 0; index < gate->jitterFolds.size(); ++index) {
          const auto& fold = gate->jitterFolds.at(index);
          check(fold.cpu == cpus.at(0) || fold.cpu == cpus.at(1),
                "the entry's fold names a probe cpu (E-08)");
          check(index == 0 || gate->jitterFolds.at(index - 1).cpu < fold.cpu,
                "the entry's folds carry the probe cpus ascending (E-08)");
          const auto entryName =
              document.find("\"" + std::string(gateName) + "\"");
          check(entryName != std::string::npos,
                "the document carries the entry (FR-027)");
          const auto entryJitter = document.find("\"jitter\"", entryName);
          check(entryJitter != std::string::npos,
                "the entry carries its jitter object (FR-027, E-08)");
          const auto objectAt =
              document.find("\"cpu\":" + std::to_string(fold.cpu), entryJitter);
          check(objectAt != std::string::npos,
                "the entry's stats array carries the fold's cpu (E-08)");
          checkStatsRoundTrip(fold.stats,
                              document,
                              objectAt,
                              "the entry's fold carries the writer's sixteen "
                              "fields " "(FR-029, SC-001)");
        }
      }
    }

    // FR-028, E-10: the gaps value of the raw JSON option writes the
    // per-cpu gap list, each record carrying stamp and gapNs. The gap
    // series rides the reader-install seam, the route the fake's shared
    // cursor cannot express (R-02); red against the writer T014 lands.
    {
      const std::string path = "harness_jitter_window_gaps.json";
      auto arguments = quickRun;
      arguments.push_back("--json=" + path);
      arguments.push_back("--json-raw=gaps");
      // The pre-check mode makes the kept record load-independent: the
      // pre-check loop runs to its scripted span with no stop check, and
      // `precheck=1` with the series' one-tick step reaches that span at
      // the jump read, so the phase is one read that every cpu keeps
      // (FR-014, E-05).
      arguments.push_back("--jitter-probe=" + cpuList
                          + ",mode=precheck,precheck=1");
      sg::detail::installTestReader(std::make_unique<GapSeriesReader>());
      captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      check(document.find("\"gaps\"") != std::string::npos
                && document.find("\"stamp\"") != std::string::npos
                && document.find("\"gapNs\"") != std::string::npos,
            "the raw gap list carries its records (FR-028, E-10)");
    }
  }

  // FR-029, contracts/report-json.md: one test per reader error code on
  // the new keys. The documents carry a jitter scope the reader of
  // master ignores, so both stand red against the reader T014 lands.
  {
    const std::string path = "harness_jitter_window_free_reader.json";
    captureRun({"--filter=bmQuiet",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + path});
    auto document = readDocument(path);
    std::remove(path.c_str());
    const auto benchmarks = document.find(",\"benchmarks\"");
    check(benchmarks != std::string::npos,
          "the document carries its entries key (FR-026)");

    // NO_SUCH_FIELD: a jitter scope whose `mode` key stands missing.
    {
      auto broken = document;
      broken.insert(benchmarks, ",\"jitter\":{\"thresholdNs\":200}");
      const std::string brokenPath = "harness_jitter_window_nofield.json";
      std::ofstream out(brokenPath);
      out << broken;
      out.close();
      const auto loaded = sg::loadReport(brokenPath);
      std::remove(brokenPath.c_str());
      check(!loaded.has_value()
                && loaded.error().find("jitter") != std::string::npos
                && loaded.error().find("stands missing")
                       != std::string::npos,
            "a jitter scope with a missing key names the field path "
            "(FR-029, contracts/report-json.md)");
    }

    // INCORRECT_TYPE: a jitter scope whose `mode` key carries a number.
    {
      auto broken = document;
      broken.insert(benchmarks,
                    ",\"jitter\":{\"mode\":42,\"thresholdNs\":200}");
      const std::string brokenPath = "harness_jitter_window_wrongtype.json";
      std::ofstream out(brokenPath);
      out << broken;
      out.close();
      const auto loaded = sg::loadReport(brokenPath);
      std::remove(brokenPath.c_str());
      check(!loaded.has_value()
                && loaded.error().find("jitter") != std::string::npos
                && loaded.error().find("expected ") != std::string::npos,
            "a jitter scope with a wrong kind names the kind it expected "
            "(FR-029, contracts/report-json.md)");
    }
  }

  // US3 scenario 1, FR-021, FR-024, FR-027, E-08, R-04: the split
  // places the kept gaps of one probe cpu into the entry whose windows
  // contain their stamps. The gate benchmark scripts a jump into the
  // shared tsc series inside its own timed window, so the probe cpu's
  // next read stands a kept gap above the risen threshold, and the
  // window close reads the same jumped tail the kept records ride, so
  // the close stamp stands above every kept record's stamp (R-02,
  // R-04). Observe red before T023 to T025 exist: the
  // per-entry key stands null and no attribution line prints.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    const std::string path = "harness_jitter_window_split.json";
    installGateReader(1);
    const auto console =
        captureRun({"--repetitions=1",
                    "--iterations=2",
                    "--json=" + path,
                    "--jitter-probe=" + std::to_string(cpus.at(0))});
    clearGateReader();
    const auto document = readDocument(path);
    std::remove(path.c_str());

    const auto gateName = document.find("\"bmGate\"");
    const auto quietName = document.find("\"bmQuiet\"");
    check(gateName != std::string::npos && quietName != std::string::npos,
          "the document carries both entries (FR-021)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the gate entry's jitter object carries the stats (FR-021, E-08)");
    check(document.find("\"jitter\":null", quietName) != std::string::npos,
          "the entry whose windows held no kept gap stands null "
          "(FR-027, E-08)");
    check(document.find("\"cpu\":" + std::to_string(cpus.at(0)), gateName)
              != std::string::npos,
          "the stats name the probe cpu (E-08)");
    check(integerValueAfter(document, gateName, "maxNs") == kSeriesJump,
          "the stats carry the scripted gap (E-08, R-04)");

    const auto lines = splitLines(console);
    std::size_t gateLast = 0;
    std::size_t blockLine = 0;
    std::size_t lineAt = 0;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      if (lines.at(index).rfind("bmGate", 0) == 0) {
        gateLast = index;
      }
      if (lines.at(index).rfind("jitter: worst cpu ", 0) == 0 && lineAt == 0) {
        lineAt = index;
      }
      if (lines.at(index).rfind("jitter probe: cpus ", 0) == 0
          && blockLine == 0)
      {
        blockLine = index;
      }
    }
    check(lineAt != 0 && gateLast != 0 && blockLine != 0,
          "the run prints the attribution line (FR-024, R-08)");
    check(lineAt > gateLast && lineAt < blockLine,
          "the attribution line prints under that benchmark's rows "
          "(FR-024, R-08)");
    check(lines.at(lineAt).find("gaps=") != std::string::npos
              && lines.at(lineAt).find("max=") != std::string::npos
              && lines.at(lineAt).find("total=") != std::string::npos,
          "the line carries the grammar of the one code span (FR-024)");
    check(lines.at(lineAt).find("worst cpu " + std::to_string(cpus.at(0)))
              != std::string::npos,
          "the line names the worst probe cpu (FR-024, R-08)");
  }

  // US3 scenario 2, FR-022, PC-5: each pass the runner carries gives its
  // own timed windows and the split covers all of them. The gate's three
  // repetitions each keep their jump inside their own window, and the
  // entry's fold counts every repetition's kept gaps; a split over one
  // window alone would carry one repetition's records and stand red
  // against the count below.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    const std::string path = "harness_jitter_window_passes.json";
    installGateReader(1);
    captureRun({"--filter=bmGate",
                "--repetitions=3",
                "--iterations=2",
                "--json=" + path,
                "--jitter-probe=" + std::to_string(cpus.at(0))});
    clearGateReader();
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto gateName = document.find("\"bmGate\"");
    check(gateName != std::string::npos,
          "the document carries the gate entry (FR-022)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the fold covers the windows of every repetition (FR-022, PC-5)");
    check(integerValueAfter(document, gateName, "gapCount") >= 3,
          "each repetition's windows placed their kept gaps (FR-022, PC-5)");
    check(integerValueAfter(document, gateName, "maxNs") == kSeriesJump,
          "the kept gaps carry the scripted jump (E-04, R-04)");
  }

  // US3 scenario 3, FR-022, PC-5, convergence T047: the probe over a
  // multi-pass run. The family's two metric columns carry one scripted
  // leaf each and the cpu source they need publishes a limit of one
  // counter, so the auto request settles the passes method and the
  // planner opens one pass per column (E-01, FR-004). Each pass's one
  // attempt runs the gate body, which scripts its own pass's jump
  // magnitude into the shared tsc series' tail, a level that rises
  // across the passes and never steps back, and waits for the ride
  // margin the body documents before its window closes, so the kept
  // set of each pass's window carries the pass's magnitude alone. The
  // fold's minimum equals the first pass's magnitude, its maximum
  // equals the second pass's, and a fold over one pass's windows alone
  // misses the other pass's kept gaps and fails the bound that
  // magnitude names (FR-022, PC-5). The gap buffer carries the ride
  // margins' kept records, the way the kept-buffer scenario sizes its
  // own (FR-010).
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    passGateCalls() = 0;
    const std::string path = "harness_jitter_window_passsplit.json";
    installGateReader(1);
    captureRun(
        {"--filter=bmPassGate",
         "--repetitions=1",
         "--iterations=2",
         "--json=" + path,
         "--jitter-probe=" + std::to_string(cpus.at(0)) + ",gaps=8388608"});
    clearGateReader();
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto gateName = document.find("\"bmPassGate\"");
    check(gateName != std::string::npos,
          "the document carries the multi-pass entry (FR-022)");
    check(document.find("\"metricMethod\":\"PASSES\"", gateName)
              != std::string::npos,
          "the columns forced the passes method (FR-022, E-01)");
    check(integerValueAfter(document, gateName, "passCount") == 2,
          "the planner opened one pass per column (FR-022, E-01)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the fold covers the windows of every pass (FR-022, PC-5)");
    check(integerValueAfter(document, gateName, "gapCount") >= 128,
          "each pass's windows placed their kept gaps (FR-022, PC-5)");
    check(
        integerValueAfter(document, gateName, "minNs") == kSeriesJump,
        "the first pass's window kept the jump its body scripted " "(FR-022)");
    check(
        integerValueAfter(document, gateName, "maxNs") == 2 * kSeriesJump,
        "the second pass's window kept the jump its body scripted " "(FR-022)");
  }

  // FR-010, D-6: a buffer that filled attributes only the kept records.
  // The one-slot buffer keeps the jump's first record and drops the
  // reads after it, the entry's stats carry the kept record alone, the
  // run-level record counts the dropped gaps, and the report states the
  // rule.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    const std::string path = "harness_jitter_window_kept.json";
    installGateReader(1);
    const auto console = captureRun(
        {"--filter=bmGate",
         "--repetitions=1",
         "--iterations=2",
         "--json=" + path,
         "--jitter-probe=" + std::to_string(cpus.at(0)) + ",gaps=1"});
    clearGateReader();
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto gateName = document.find("\"bmGate\"");
    check(gateName != std::string::npos,
          "the document carries the gate entry (FR-010)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the entry carries the kept record's stats (FR-010, D-6)");
    check(integerValueAfter(document, gateName, "gapCount") == 1,
          "the stats attribute the kept record alone (FR-010, D-6)");
    const auto benchmarks = document.find(",\"benchmarks\"");
    check(benchmarks != std::string::npos,
          "the document carries its entries key (FR-026)");
    check(integerValueAfter(document, 0, "droppedGaps") >= 1,
          "the run-level record counts the dropped gaps (FR-010, E-07)");
    check(console.find("jitter: split and percentiles use the kept gaps only")
              != std::string::npos,
          "the report states the kept-gaps-only rule (FR-010, D-6)");
  }

  // E-06, R-04, FR-021: the fold of one probe cpu over one benchmark's
  // windows places each kept record by its stamp: a record inside a
  // window folds once into the entry's stats even when two of its
  // windows contain it, a record outside every window stands absent
  // from the stats, and the covered span the percent and rate fields
  // divide is the windows' total. The fold rides the seam header, so
  // the red this scenario predicts against T024 is the undefined
  // reference the split's definition answers (Principle III).
  {
    sg::detail::ProbeCpuState state;
    state.cpu = cpus.at(0);
    state.buffer.resize(4);
    state.buffer.at(0) = {.stamp = 10, .gapNs = 5};
    state.buffer.at(1) = {.stamp = 12, .gapNs = 9};
    state.buffer.at(2) = {.stamp = 30, .gapNs = 7};
    state.buffer.at(3) = {.stamp = 100, .gapNs = 11};
    state.kept = 4;
    state.thresholdNs = 200;
    state.loopPeriodNs = 1;
    const std::vector<sg::detail::TimedWindow> windows {
        {.start = 0,
         .end = 20,
         .entry = 0,
         .repetition = 0,
         .chunk = 0,
         .pass = 0},
        {.start = 10,
         .end = 25,
         .entry = 0,
         .repetition = 0,
         .chunk = 0,
         .pass = 0},
        {.start = 25,
         .end = 40,
         .entry = 0,
         .repetition = 1,
         .chunk = 0,
         .pass = 0},
    };
    const auto fold = sg::detail::gapStatsOf(state, windows);
    check(fold.gapCount == 3,
          "the fold counts each placed record once (E-06, FR-021)");
    check(fold.minNs == 5 && fold.maxNs == 9 && fold.totalNs == 21,
          "the fold carries the placed records' fields (E-03, E-06)");
    check(fold.thresholdNs == 200 && fold.loopPeriodNs == 1,
          "the fold carries the run's measured points (E-03)");
    check(fold.runtimeNs == 50,
          "the covered span is the windows' total (E-03, R-05)");
    check(std::fabs(fold.totalPercent - 42.0) <= 1e-9,
          "the percent divides by the windows' total (E-03, R-05)");

    const std::vector<sg::detail::TimedWindow> elsewhere {
        {.start = 1000,
         .end = 2000,
         .entry = 0,
         .repetition = 0,
         .chunk = 0,
         .pass = 0},
    };
    const auto absent = sg::detail::gapStatsOf(state, elsewhere);
    check(absent.gapCount == 0 && absent.totalNs == 0
              && absent.maxNs == 0,
          "a record outside every window stands absent from the stats "
          "(E-06, FR-021)");
  }

  // specs/043 T019, US1, US2, US4, FR-007 to FR-009, FR-020, R-09,
  // R-10, R-11: the two series folds over a scripted series. The run
  // fold answers the figures of the kept samples, and the entry fold
  // keeps the samples whose recovered stamp falls inside a half-open
  // window, standing null where the windows place none. The rows call
  // the folds on the seam header, so the red this scenario predicts is
  // the undefined reference the definitions answer (Principle III).
  {
    sg::detail::ProbeCpuState fwq;
    fillFwqSeries(fwq);
    const auto run = sg::detail::seriesStatsOf(fwq);
    check(run.sampleCount == 4,
          "the run fold counts the kept samples (FR-007)");
    check(run.minValue == 1000 && run.maxValue == 2000,
          "the run fold names the kept extremes (FR-007)");
    check(run.medianValue == 1000 && run.p90Value == 2000
              && run.p99Value == 2000 && run.p999Value == 2000
              && run.p9999Value == 2000,
          "the five points read the nearest rank over the kept samples "
          "(FR-008)");
    check(std::fabs(run.meanValue - 1250.0) <= 1e-9,
          "the mean divides the kept sum by the sample count (FR-007)");
    check(std::fabs(run.noisePercent - 20.0) <= 1e-9,
          "the FWQ fraction reads the kept unit times over their sum "
          "(FR-009, R-09)");
    check(run.seriesRuntimeNs == 5000,
          "the runtime is the recovered stamp span, the calibration span "
          "out (FR-007, FR-010)");
    check(run.droppedSamples == 0, "the drop count rides the state (FR-022)");
    check(run.workCount == 7 && run.unitTimeNs == 1000 && run.targetNs == 1000
              && run.quantumNs == 0 && run.calibrationSpanNs == 500,
          "the five calibration figures ride the state (FR-011, E-22)");

    const auto placed =
        sg::detail::seriesStatsOf(fwq, seriesWindow(1500, 4500));
    check(placed.has_value(),
          "thePerEntrySeriesFoldsTheWindows: the windows place samples, so "
          "the entry fold stands present (FR-020)");
    check(placed->sampleCount == 2,
          "thePerEntrySeriesFoldsTheWindows: the entry fold counts the "
          "samples the windows placed (FR-020)");
    check(placed->minValue == 1000 && placed->maxValue == 2000,
          "the entry fold names the placed extremes (FR-007, FR-020)");
    check(placed->medianValue == 1000 && placed->p90Value == 2000,
          "the entry's points read the nearest rank over the placed set "
          "(FR-008)");
    check(std::fabs(placed->meanValue - 1500.0) <= 1e-9,
          "the entry's mean divides the placed sum by its count (FR-007)");
    check(std::fabs(placed->noisePercent - 100.0 / 3.0) <= 1e-9,
          "the entry's fraction reads the placed unit times (FR-009, FR-020)");
    check(placed->seriesRuntimeNs == 3000,
          "the entry's runtime is the window's total (FR-007, E-03)");

    // The half-open rule at both ends: the window's start places the
    // stamp that stands on it, and its end stands past the stamp that
    // stands on it (E-06, R-04, R-11).
    const std::vector<sg::detail::TimedWindow> edges {
        seriesWindow(1000, 3000).front(), seriesWindow(5000, 6000).front()};
    const auto boundary = sg::detail::seriesStatsOf(fwq, edges);
    check(boundary.has_value() && boundary->sampleCount == 2,
          "the start places its stamp and the end stands past its own "
          "(FR-020, R-11)");
    check(boundary->minValue == 1000 && boundary->maxValue == 1000,
          "the two placed unit times stand equal (FR-007)");
    check(std::fabs(boundary->noisePercent) <= 1e-9,
          "equal unit times answer a zero fraction (FR-009, SC-001)");
    check(boundary->seriesRuntimeNs == 3000,
          "the covered span is the windows' total (E-03)");

    check(!sg::detail::seriesStatsOf(fwq, seriesWindow(10000, 20000))
               .has_value(),
          "thePerEntrySeriesStandsNullWithoutSamples: the windows that "
          "place no sample leave the entry fold null (FR-020)");
    check(!sg::detail::seriesStatsOf(
               fwq, std::span<const sg::detail::TimedWindow> {})
               .has_value(),
          "no window places no sample, so the entry fold stands null (FR-020)");
  }

  // specs/043 T019, US2, FR-007 to FR-009, FR-020, R-10, R-11: the FTQ
  // fold reads counts, the stamp of a count stands at the start plus
  // its rank plus one times the quantum, and a maximum count of 0
  // answers 100.
  {
    sg::detail::ProbeCpuState ftq;
    fillFtqSeries(ftq);
    const auto run = sg::detail::seriesStatsOf(ftq);
    check(run.sampleCount == 3, "the FTQ fold counts the kept counts (FR-007)");
    check(run.minValue == 0 && run.maxValue == 5,
          "the FTQ fold names the kept counts (FR-007)");
    check(run.medianValue == 3 && run.p90Value == 5 && run.p99Value == 5
              && run.p999Value == 5 && run.p9999Value == 5,
          "the five points read the nearest rank over the counts (FR-008)");
    check(std::fabs(run.meanValue - 8.0 / 3.0) <= 1e-9,
          "the mean divides the count sum by the sample count (FR-007)");
    check(std::fabs(run.noisePercent - 700.0 / 15.0) <= 1e-9,
          "the FTQ fraction reads the counts against the sample count "
          "times the maximum (FR-009, R-10)");
    check(run.seriesRuntimeNs == 3000,
          "the runtime is the recovered stamp span of the three quanta "
          "(FR-007, FR-015)");

    const auto placed =
        sg::detail::seriesStatsOf(ftq, seriesWindow(1100, 2100));
    check(placed.has_value() && placed->sampleCount == 1,
          "the edge the window opens on places its count, and the edge it "
          "closes on stands past (FR-020, R-11)");
    check(placed->minValue == 5 && placed->maxValue == 5,
          "one count stands its own maximum (FR-009)");
    check(std::fabs(placed->noisePercent) <= 1e-9,
          "the single placed count answers a zero fraction (FR-009)");
    check(placed->seriesRuntimeNs == 1000,
          "the entry's runtime is the window's total (FR-007, E-03)");

    sg::detail::ProbeCpuState silent;
    fillFtqSeries(silent);
    silent.series.assign({0, 0});
    silent.seriesKept = 2;
    const auto quiet = sg::detail::seriesStatsOf(silent);
    check(quiet.maxValue == 0 && std::fabs(quiet.noisePercent - 100.0) <= 1e-9,
          "a maximum count of 0 answers 100 (FR-009, R-10)");

    sg::detail::ProbeCpuState idle;
    fillFwqSeries(idle);
    idle.series.assign({0, 0});
    idle.seriesKept = 2;
    const auto still = sg::detail::seriesStatsOf(idle);
    check(still.maxValue == 0 && std::fabs(still.noisePercent) <= 1e-9,
          "a kept set of zero unit times answers a zero fraction (FR-009)");
    check(still.seriesRuntimeNs == 0,
          "the recovered stamps of a zero series span nothing (FR-007)");

    sg::detail::ProbeCpuState bare;
    fillFwqSeries(bare);
    bare.seriesKept = 0;
    const auto none = sg::detail::seriesStatsOf(bare);
    check(none.sampleCount == 0 && none.minValue == 0 && none.maxValue == 0
              && none.medianValue == 0 && none.p9999Value == 0
              && std::fabs(none.meanValue) <= 1e-9
              && std::fabs(none.noisePercent) <= 1e-9,
          "an empty kept set leaves every figure at 0 (FR-008)");
    check(none.workCount == 7 && none.calibrationSpanNs == 500,
          "the calibration figures ride the state through the empty set "
          "(FR-011, E-22)");
  }

  // specs/043 T019, US4, FR-020, R-11: the split fills the entry's
  // series record over that benchmark's timed windows. A FWQ run keeps
  // no gap, so the entry's fold stands on its series alone: the entry
  // carries a jitter object once the fold lands. A pre-check run's
  // phase closes before the batch opens, so its windows place no
  // sample and its entry stands null.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    const std::string path = "harness_jitter_window_series.json";
    installGateReader(1);
    armWatchedWindow(kWatchedWindowReads, 1);
    captureRun({"--filter=bmQuiet",
                "--repetitions=" + std::to_string(kWatchedEntryRepetitions),
                "--iterations=" + std::to_string(kWatchedEntryIterations),
                "--json=" + path,
                "--jitter-probe=" + std::to_string(cpus.at(0))
                    + ",method=fwq,work=" + std::to_string(kWatchedEntryWork)
                    + ",gaps=1048576"});
    clearGateReader();
    clearWatchedWindow();
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto quietName = document.find("\"bmQuiet\"");
    check(quietName != std::string::npos,
          "the document carries the entry (FR-020)");
    check(document.find("\"jitter\":{\"stats\"", quietName) != std::string::npos,
          "thePerEntrySeriesFoldsTheWindows: the FWQ entry carries the fold "
          "its series placed (FR-020, R-11)");
    // FR-006, R-11: the entry's jitter record carries both arrays, and the
    // series item carries the samples the probe read inside this entry's
    // windows.
    const auto entryJitter = document.find("\"jitter\":{", quietName);
    check(document.find("\"stats\":[{", entryJitter) != std::string::npos
              && document.find("\"series\":[{", entryJitter)
                     != std::string::npos,
          "thePerEntrySeriesFoldsTheWindows: the entry carries a real jitter "
          "record with its stats and its series (FR-020, E-08)");
    const auto entryItem = seriesItemAt(document, entryJitter);
    check(integerValueAfter(document, entryItem, "sampleCount") > 0,
          "thePerEntrySeriesFoldsTheWindows: the entry's window holds the "
          "watched entry's samples (FR-006, FR-020, R-11)");
  }
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    const std::string path = "harness_jitter_window_series_precheck.json";
    captureRun({"--filter=bmQuiet",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + path,
                "--jitter-probe=" + std::to_string(cpus.at(0))
                    + ",method=fwq,mode=precheck,precheck=1"});
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto quietName = document.find("\"bmQuiet\"");
    check(quietName != std::string::npos,
          "the document carries the entry (FR-020)");
    check(document.find("\"jitter\":null", quietName) != std::string::npos,
          "thePerEntrySeriesStandsNullWithoutSamples: the windows of a "
          "pre-check run place no sample, so the entry stands null (FR-020)");
  }

  // US4 scenario 1, FR-030, FR-031, D-11, contracts/rule-names.md: the
  // parser accepts the four jitter value names with every one of the
  // sixteen field names of FR-002 in camelCase and the standing
  // comparison suffix the N5 grammar carries. Observe red before T027
  // exists: the parser carries no `jitter:` names.
  {
    const std::string cpu = std::to_string(cpus.at(0));
    const std::string heads[] {"jitter:" + cpu + ":",
                               "jitter:any:",
                               "jitter:run:" + cpu + ":",
                               "jitter:run:any:"};
    for (const auto& head : heads) {
      for (const auto field : kStatFields) {
        const std::string text = head + std::string(field) + ":lower:abs=5000";
        const auto rule = sg::parseCompareRule(text);
        check(rule.has_value(),
              "the parser accepts the jitter value name (FR-030, D-11)");
        check(
            rule->text == text && rule->valueName == head + std::string(field),
            "the rule keeps the text and the value name it read (D-08)");
        check(rule->absoluteThreshold.has_value()
                  && std::fabs(*rule->absoluteThreshold - 5000.0) <= 1e-9,
              "the standing suffix's absolute bound lands in the rule "
              "(FR-033)");
      }
    }
    const auto percent = sg::parseCompareRule("jitter:any:p99Ns:higher:5%");
    check(percent.has_value() && percent->relativeThreshold.has_value()
              && percent->direction == sg::Direction::HIGHER_IS_BETTER,
          "the percent suffix and the direction word read on a jitter "
          "name (FR-033)");
    const auto filtered =
        sg::parseCompareRule("jitter:run:any:p99Ns:lower:abs=1:bm");
    check(filtered.has_value() && filtered->filter == "bm",
          "the filter field reads on a jitter name (FR-024)");
    // A field name outside the sixteen, a cpu that is neither an index
    // nor `any`, a cpu past the index range, and an aggregate suffix on
    // a jitter name are faults: the value carries one value (FR-038,
    // spec D-21).
    check(!sg::parseCompareRule("jitter:any").has_value(),
          "a jitter name with no field is a fault (FR-030)");
    check(!sg::parseCompareRule("jitter:any:madeUpNs:lower:abs=1").has_value(),
          "a field name outside the sixteen is a fault (FR-038)");
    check(!sg::parseCompareRule("jitter:x:p99Ns:lower:abs=1").has_value(),
          "a cpu that is neither an index nor any is a fault (FR-038)");
    check(!sg::parseCompareRule("jitter:4294967296:p99Ns:lower:abs=1")
               .has_value(),
          "a cpu past the index range is a fault (FR-038)");
    check(
        !sg::parseCompareRule("jitter:any:p99Ns@mean:lower:abs=1").has_value(),
        "an aggregate suffix on a jitter name is a fault (spec D-21)");
    // The empty cpu token, the bare name with no colon after it, and
    // the at-sign where the colon stands are faults: the cpu reads
    // neither an index nor `any`, and the name carries no field (FR-030,
    // FR-038).
    check(!sg::parseCompareRule("jitter::p99Ns:lower:abs=1").has_value(),
          "an empty cpu token is a fault (FR-038)");
    check(!sg::parseCompareRule("jitter").has_value(),
          "the bare name with no colon after it is a fault (FR-030)");
    check(!sg::parseCompareRule("jitter@median:lower:5%").has_value(),
          "an at sign where the colon stands is a fault (FR-030)");
  }

  // US4 scenario 2, FR-038: the field fault is the usage error of the
  // option scan: one line and exit status 2, before any report is read.
  {
    const auto note =
        captureError({"--compare=a.json,b.json",
                      "--compare-rule=jitter:any:madeUpNs:lower:abs=5000"},
                     2);
    check(
        note.rfind("--compare-rule: ", 0) == 0
            && std::count(note.begin(), note.end(), '\n') == 1,
        "the field fault prints one line and exits with status 2 " "(FR-038)");
  }

  // US4 scenarios 3 to 7, FR-030 to FR-032, SC-004: the comparison runs
  // over documents the probe writes. The gate benchmark scripts its
  // jump through `seriesJump`, so the two runs' kept gaps differ by more
  // than the example rule's bound of D-11.
  {
    const std::string cpu = std::to_string(cpus.at(0));
    const std::string otherCpu = std::to_string(cpus.at(1));
    const std::string probe = "--jitter-probe=" + cpu;

    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    seriesJump() = kSeriesJump;
    const std::string basePath = "harness_jitter_rule_base.json";
    installGateReader(1);
    captureRun({"--filter=bmGate",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + basePath,
                probe});
    clearGateReader();
    provider.setPoints("machine", "tsc", {}, kPacedStepNs);
    seriesJump() = 2 * kSeriesJump;
    const std::string bigPath = "harness_jitter_rule_big.json";
    installGateReader(1);
    captureRun({"--filter=bmGate",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + bigPath,
                probe});
    clearGateReader();
    seriesJump() = kSeriesJump;

    const auto baseDocument = readDocument(basePath);
    const auto gateName = baseDocument.find("\"bmGate\"");
    check(gateName != std::string::npos,
          "the document carries the gate entry (FR-030)");
    check(integerValueAfter(baseDocument, gateName, "p99Ns") == kSeriesJump,
          "the entry stats carry the scripted jump as their p99 (E-03)");
    check(integerValueAfter(baseDocument, 0, "p99Ns") == kSeriesJump,
          "the run-level stats carry the scripted jump as their p99 " "(E-07)");

    // US4 scenario 3, FR-030, SC-004: the bare rules read the
    // per-benchmark values, and a bound the contender's p99 moves past
    // fails the comparison with status 1.
    const auto regressed =
        captureRun({"--compare=" + basePath + "," + bigPath,
                    "--compare-rule=jitter:" + cpu + ":p99Ns:lower:abs=5000"},
                   1);
    check(regressed.find("REGRESSED") != std::string::npos,
          "the named-cpu bare rule reads the per-benchmark values and the "
          "moved bound fails with status 1 (SC-004, FR-030)");
    const auto regressedAny =
        captureRun({"--compare=" + basePath + "," + bigPath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   1);
    check(regressedAny.find("REGRESSED") != std::string::npos,
          "the worst-cpu bare rule reads the per-benchmark values and the "
          "moved bound fails with status 1 (SC-004, D-11)");
    // The same rule over two equal documents reads a delta of zero and
    // passes: the state follows the values the documents carry.
    const auto passed =
        captureRun({"--compare=" + basePath + "," + basePath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   0);
    check(passed.find("REGRESSED") == std::string::npos,
          "a bound the values clear passes the comparison (SC-004)");

    // US4 scenario 3a, FR-030, E-03: every one of the sixteen fields
    // reads through its own reader of the field table: one comparison
    // run carries one rule per field, so each reader answers both
    // reports' values. The moved jump regresses the fields the jump
    // moves, so the run fails with status 1.
    {
      std::vector<std::string> fieldArguments {"--compare=" + basePath + ","
                                               + bigPath};
      for (const auto field : kStatFields) {
        fieldArguments.push_back(
            "--compare-rule=jitter:any:" + std::string(field) + ":lower:abs=1");
      }
      const auto fields = captureRun(fieldArguments, 1);
      check(fields.find("error:") == std::string::npos,
            "every field of the table reads without a fault (FR-030, E-03)");
      check(fields.find("REGRESSED") != std::string::npos,
            "the fields the moved jump moves regress (SC-004)");
    }

    // US4 scenario 4, FR-031, SC-004: the `jitter:run:` prefix reads the
    // whole-run values, and the same moved bound fails with status 1.
    const auto regressedRun = captureRun(
        {"--compare=" + basePath + "," + bigPath,
         "--compare-rule=jitter:run:" + cpu + ":p99Ns:lower:abs=5000"},
        1);
    check(regressedRun.find("REGRESSED") != std::string::npos,
          "the run-scoped rule reads the whole-run values and the moved "
          "bound fails with status 1 (SC-004, FR-031)");
    const auto regressedRunAny =
        captureRun({"--compare=" + basePath + "," + bigPath,
                    "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                   1);
    check(regressedRunAny.find("REGRESSED") != std::string::npos,
          "the run-scoped worst-cpu rule reads the whole-run values and "
          "the moved bound fails with status 1 (SC-004, FR-031)");

    // US4 scenario 5, FR-031, D-11: the bare rule reads the entry's own
    // stats and the `jitter:run:` prefix reads the run-level object:
    // editing one scope of the contender moves only the rule that reads
    // it.
    const std::string entryEditedPath = "harness_jitter_rule_entry.json";
    {
      const std::string edited =
          withIntegerReplaced(baseDocument, gateName, "p99Ns", 2 * kSeriesJump);
      std::ofstream out(entryEditedPath);
      out << edited;
      out.close();
    }
    const auto entryMoved =
        captureRun({"--compare=" + basePath + "," + entryEditedPath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   1);
    check(entryMoved.find("REGRESSED") != std::string::npos,
          "the bare rule reads the entry's own stats (FR-031, D-11)");
    const auto entryUnmoved =
        captureRun({"--compare=" + basePath + "," + entryEditedPath,
                    "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                   0);
    check(entryUnmoved.find("REGRESSED") == std::string::npos,
          "the run-scoped rule reads the run-level object, which the "
          "entry edit leaves equal (FR-031, D-11)");
    const std::string rootEditedPath = "harness_jitter_rule_root.json";
    {
      const std::string edited =
          withIntegerReplaced(baseDocument, 0, "p99Ns", 2 * kSeriesJump);
      std::ofstream out(rootEditedPath);
      out << edited;
      out.close();
    }
    const auto rootMoved =
        captureRun({"--compare=" + basePath + "," + rootEditedPath,
                    "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                   1);
    check(rootMoved.find("REGRESSED") != std::string::npos,
          "the run-scoped rule reads the run-level object (FR-031, D-11)");
    const auto rootUnmoved =
        captureRun({"--compare=" + basePath + "," + rootEditedPath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   0);
    check(rootUnmoved.find("REGRESSED") == std::string::npos,
          "the bare rule reads the entry's own stats, which the "
          "run-level edit leaves equal (FR-031, D-11)");
    std::remove(entryEditedPath.c_str());
    std::remove(rootEditedPath.c_str());

    // US4 scenario 6, FR-032, FR-038: a named cpu the report's jitter
    // object does not carry is the same usage error: one line and exit
    // status 2, in both scopes. The probe ran on one cpu, so the mask's
    // other cpu stands uncarried.
    {
      const auto note = captureError(
          {"--compare=" + basePath + "," + basePath,
           "--compare-rule=jitter:" + otherCpu + ":p99Ns:lower:abs=5000"},
          2);
      check(note.rfind("error: ", 0) == 0
                && std::count(note.begin(), note.end(), '\n') == 1,
            "the uncarried cpu of a bare rule is the usage error, one "
            "line and status 2 (FR-032, FR-038)");
      const auto runNote = captureError(
          {"--compare=" + basePath + "," + basePath,
           "--compare-rule=jitter:run:" + otherCpu + ":p99Ns:lower:abs=5000"},
          2);
      check(runNote.rfind("error: ", 0) == 0
                && std::count(runNote.begin(), runNote.end(), '\n') == 1,
            "the uncarried cpu of a run-scoped rule is the usage error, "
            "one line and status 2 (FR-032, FR-038)");
    }

    // US4 scenario 7, FR-032, SC-004: a `jitter:` rule on a report with
    // no probe prints one line and exits with status 2, in both scopes.
    {
      const std::string freeA = "harness_jitter_rule_free_a.json";
      captureRun({"--filter=bmQuiet",
                  "--repetitions=1",
                  "--iterations=2",
                  "--json=" + freeA});
      const std::string freeB = "harness_jitter_rule_free_b.json";
      captureRun({"--filter=bmQuiet",
                  "--repetitions=1",
                  "--iterations=2",
                  "--json=" + freeB});
      const auto note =
          captureError({"--compare=" + freeA + "," + freeB,
                        "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                       2);
      check(note.rfind("error: ", 0) == 0
                && std::count(note.begin(), note.end(), '\n') == 1,
            "a bare jitter rule on a probe-free report is the usage "
            "error, one line and status 2 (FR-032, SC-004)");
      const auto runNote =
          captureError({"--compare=" + freeA + "," + freeB,
                        "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                       2);
      check(runNote.rfind("error: ", 0) == 0
                && std::count(runNote.begin(), runNote.end(), '\n') == 1,
            "a run-scoped jitter rule on a probe-free report is the "
            "usage error, one line and status 2 (FR-032, SC-004)");
      std::remove(freeA.c_str());
      std::remove(freeB.c_str());
    }

    std::remove(basePath.c_str());
    std::remove(bigPath.c_str());
  }

  // specs/043 T023, US4, FR-006, FR-019, FR-020, FR-031, R-16, R-17: the
  // record of a run that names a method. The document carries the method
  // word in the upper-case spelling the JSON takes, one closed series item
  // per probe cpu with its keys in the written order, and the periods array
  // riding the cpu's item alone; `loadReport` reads every key back; and a
  // run with no method key writes neither new key, the bytes of the
  // standing record standing whole (FR-031, SC-004). The scripted clock
  // steps `kPacedStepNs` per read, so a FWQ window keeps a series of equal
  // unit times, and the `gaps` value sizes the series buffer, so the
  // transform reads a bounded span (FR-006, FR-014, FR-018).
  {
    const std::string probeCpu = std::to_string(cpus.at(0));
    // The series buffer sizes the kept span. The scripted clock paces a
    // unit time of 1,000 ns, so the kept prefix has to reach past the
    // batch's setup and the watched entry's runs. A 1,048,576-entry
    // buffer spans a second of the batch on the paced step, so the batch
    // keeps its samples whole (FR-006, FR-014).
    const std::string fwqValue = "--jitter-probe=" + probeCpu
        + ",method=fwq,work=" + std::to_string(kWatchedEntryWork)
        + ",gaps=1048576";
    const std::string ftqValue =
        "--jitter-probe=" + probeCpu + ",method=ftq,gaps=1048576";
    const std::vector<std::string> quick {
        "--filter=bmQuiet",
        "--repetitions=" + std::to_string(kWatchedEntryRepetitions),
        "--iterations=" + std::to_string(kWatchedEntryIterations)};

    // FR-006, FR-019: the JSON spells the method with the upper-case word
    // the enumerator names, after `loopPeriodNs` and before `series`.
    {
      const std::string path = "harness_jitter_window_method_fwq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(fwqValue);
      captureRun(arguments);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      check(root != std::string::npos,
            "the run carries the root jitter object (E-07)");
      const auto period = document.find("\"loopPeriodNs\":", root);
      const auto method = document.find("\"method\":\"FWQ\"", root);
      check(method != std::string::npos && method > period,
            "theMethodWordStandsUpperCase: the root record carries the "
            "upper-case method word after loopPeriodNs (FR-006, FR-019)");
      check(document.find("\"series\":[", method) != std::string::npos,
            "theMethodWordStandsUpperCase: the method word stands before the "
            "series array (FR-006)");
    }
    {
      const std::string path = "harness_jitter_window_method_ftq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(ftqValue);
      captureRun(arguments);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      check(root != std::string::npos,
            "the run carries the root jitter object (E-07)");
      const auto period = document.find("\"loopPeriodNs\":", root);
      const auto method = document.find("\"method\":\"FTQ\"", root);
      check(method != std::string::npos && method > period,
            "theMethodWordStandsUpperCase: a FTQ run carries the FTQ word "
            "after loopPeriodNs (FR-006, FR-019)");
    }

    // FR-006, E-22, E-23: the closed item carries its keys in the written
    // order, the method's own key standing out: `quantumNs` for a FWQ run,
    // `resampleStepNs` for a FTQ run. The per-entry item carries the same
    // keys with no method and no periods (FR-020).
    {
      const std::string path = "harness_jitter_window_series_fwq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(fwqValue);
      installGateReader(1);
      armWatchedWindow(kWatchedWindowReads, 1);
      captureRun(arguments);
      clearGateReader();
      clearWatchedWindow();
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      check(root != std::string::npos,
            "the run carries the root jitter object (E-07)");
      const auto itemAt = seriesItemAt(document, root);
      check(jsonKeysAt(document, itemAt) == wantedSeriesKeys("quantumNs"),
            "theSeriesItemCarriesEveryKey: the FWQ item carries its method's "
            "keys in the written order (FR-006, E-23)");
      const auto entry = document.find("\"bmQuiet\"");
      check(entry != std::string::npos,
            "the document carries the entry (FR-020)");
      const auto entryJitter = document.find("\"jitter\":{", entry);
      check(entryJitter != std::string::npos,
            "the entry carries its jitter object (FR-027, E-08)");
      const auto entryItem = seriesItemAt(document, entryJitter);
      auto wanted = wantedSeriesKeys("quantumNs");
      // The per-entry copy carries no spectrum: the period list rides the
      // run-level item alone, so the resample step the transform read
      // stands out of it too, and a FWQ item stands at 19 keys (FR-020).
      wanted.erase(
          std::find(wanted.begin(), wanted.end(), std::string("periods")));
      wanted.erase(std::find(
          wanted.begin(), wanted.end(), std::string("resampleStepNs")));
      check(jsonKeysAt(document, entryItem) == wanted,
            "theSeriesItemCarriesEveryKey: the per-entry item carries the "
            "same keys with no method and no periods (FR-020)");
    }
    {
      const std::string path = "harness_jitter_window_series_ftq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(ftqValue);
      captureRun(arguments);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      check(root != std::string::npos,
            "the run carries the root jitter object (E-07)");
      const auto itemAt = seriesItemAt(document, root);
      check(jsonKeysAt(document, itemAt) == wantedSeriesKeys("resampleStepNs"),
            "theSeriesItemCarriesEveryKey: the FTQ item carries its method's "
            "keys in the written order (FR-006, E-23)");
    }

    // FR-006, FR-018, FR-020: the periods array rides the cpu's item and
    // stands out of the per-entry copy. The flat clock leaves the FWQ
    // median at 0 and the FTQ kept set empty, so the transform reads
    // neither run and the list stands empty with the point count (FR-017,
    // FR-018, R-12).
    {
      const std::string path = "harness_jitter_window_periods_fwq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(fwqValue);
      installGateReader(1);
      armWatchedWindow(kWatchedWindowReads, 1);
      captureRun(arguments);
      clearGateReader();
      clearWatchedWindow();
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      const auto itemAt = seriesItemAt(document, root);
      check(document.find("\"periods\":[", itemAt) != std::string::npos,
            "thePeriodsArrayRidesTheCpuItem: the cpu's item carries the "
            "periods array (FR-006)");
      const auto entry = document.find("\"bmQuiet\"");
      const auto entryJitter = document.find("\"jitter\":{", entry);
      const auto entryItem = seriesItemAt(document, entryJitter);
      const auto keys = jsonKeysAt(document, entryItem);
      check(std::find(keys.begin(), keys.end(), std::string("periods"))
                == keys.end(),
            "thePeriodsArrayRidesTheCpuItem: the per-entry item carries no "
            "periods array (FR-020)");
    }
    {
      const std::string path = "harness_jitter_window_flat_fwq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(fwqValue);
      sg::detail::installTestReader(std::make_unique<FlatSeriesReader>());
      armWatchedWindow(kWatchedWindowReads, 1);
      captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      clearWatchedWindow();
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      const auto itemAt = seriesItemAt(document, root);
      check(integerValueAfter(document, itemAt, "sampleCount") > 0,
            "thePeriodsArrayRidesTheCpuItem: the flat clock keeps unit times "
            "(FR-012)");
      check(integerValueAfter(document, itemAt, "medianValue") == 0,
            "thePeriodsArrayRidesTheCpuItem: the flat clock's median unit "
            "time stands 0 (FR-007)");
      check(integerValueAfter(document, itemAt, "spectrumPoints") == 0,
            "thePeriodsArrayRidesTheCpuItem: a median of 0 leaves the "
            "transform unread (FR-017, R-12)");
      check(document.find("\"periods\":[]", itemAt) != std::string::npos,
            "thePeriodsArrayRidesTheCpuItem: the unread transform answers an "
            "empty list (FR-018)");
    }
    {
      const std::string path = "harness_jitter_window_flat_ftq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(ftqValue);
      sg::detail::installTestReader(std::make_unique<FlatSeriesReader>());
      captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      const auto root = document.find("\"jitter\":{");
      const auto itemAt = seriesItemAt(document, root);
      check(integerValueAfter(document, itemAt, "sampleCount") == 0,
            "thePeriodsArrayRidesTheCpuItem: a clock that never moves leaves "
            "no quantum closed (FR-016)");
      check(integerValueAfter(document, itemAt, "quantumNs") > 0,
            "thePeriodsArrayRidesTheCpuItem: the FTQ item carries the "
            "quantum (FR-015, E-22)");
      check(document.find("\"periods\":[]", itemAt) != std::string::npos,
            "thePeriodsArrayRidesTheCpuItem: an empty kept set leaves the "
            "list empty (FR-018, R-12)");
    }

    // FR-018, FR-020, E-24: the period list of a run whose series carries
    // a period. The square-wave clock steps 1,000 ns for eight reads and
    // 2,000 ns for the eight that follow, so the resample closes 24 bins of
    // the 1,000 ns median step per cycle. The transform names that cycle,
    // the written item carries the list with its items in the written
    // order, and `loadReport` reads the figures back.
    {
      const std::string path = "harness_jitter_window_named_periods_fwq.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back("--jitter-probe=" + probeCpu
                          + ",method=fwq,gaps=1024");
      sg::detail::installTestReader(std::make_unique<WaveSeriesReader>());
      captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      const auto document = readDocument(path);
      const auto root = document.find("\"jitter\":{");
      const auto itemAt = seriesItemAt(document, root);
      const auto listAt = document.find("\"periods\":[{", itemAt);
      check(listAt != std::string::npos,
            "theNamedPeriodListCarriesItsFigures: the square-wave clock writes "
            "a period item into the cpu's item (FR-018, E-24)");
      const auto periodAt = listAt + std::strlen("\"periods\":[");
      const auto periodEnd = objectEndAt(document, periodAt);
      check(periodEnd != std::string::npos, "the period item closes (FR-018)");
      const std::vector<std::string> kPeriodKeys {"periodNs", "powerRatio"};
      check(jsonKeysAt(document, periodAt) == kPeriodKeys,
            "theNamedPeriodListCarriesItsFigures: the period item carries its "
            "two keys in the written order (FR-018, E-24)");
      // The planted cycle spans 24 steps of the 1,000 ns median unit time.
      // The kept span starts at no fixed phase of the wave, so the named
      // bin carries the cycle within one step of it.
      constexpr std::uint64_t kCycleNs = 24000;
      constexpr std::uint64_t kCycleSlackNs = 2400;
      const auto periodNs =
          integerValueIn(document, periodAt, periodEnd, "periodNs");
      check(periodNs + kCycleSlackNs >= kCycleNs
                && periodNs <= kCycleNs + kCycleSlackNs,
            "theNamedPeriodListCarriesItsFigures: the named period stands the "
            "24-step cycle the wave plants (FR-017, FR-018)");
      check(std::fabs(doubleValueAfter(document, periodAt, "powerRatio") - 1.0)
                <= 1e-9,
            "theNamedPeriodListCarriesItsFigures: the first item's power ratio "
            "stands 1 (FR-018)");
      const auto loaded = sg::loadReport(path);
      std::remove(path.c_str());
      check(loaded.has_value(),
            "theNamedPeriodListCarriesItsFigures: the document of the named "
            "period list loads (FR-020)");
      if (!loaded.has_value() || !loaded->runJitterStats.has_value()
          || loaded->runJitterStats->empty()
          || !loaded->runJitterStats->at(0).series.has_value())
      {
        fail("the run-level fold of the waved run leaves its series unread "
             "(FR-020)");
      }
      const auto& series = *loaded->runJitterStats->at(0).series;
      check(!series.periods.empty(),
            "theNamedPeriodListCarriesItsFigures: the read leaves the period "
            "list with its items (FR-020, E-24)");
      check(!series.periods.empty()
                && series.periods.front().periodNs == periodNs
                && std::fabs(series.periods.front().powerRatio - 1.0) <= 1e-9,
            "theNamedPeriodListCarriesItsFigures: the read answers the written "
            "figures of the first item (FR-020, R-17)");
      checkSeriesRoundTrip(series,
                           document,
                           itemAt,
                           "theNamedPeriodListCarriesItsFigures: the cpu's "
                           "item reads back every key (FR-020, R-17)");
    }

    // FR-020, R-17: `loadReport` reads every key back, the run record
    // carrying the method word and each fold carrying its series.
    {
      const std::string path = "harness_jitter_window_series_roundtrip.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back(fwqValue);
      // FR-006, R-11: the armed wait ends inside the entry's own window,
      // so the probe's reads stand kept there.
      auto& provider = scriptedProvider();
      provider.setPoints("machine", "tsc", {}, kPacedStepNs);
      installGateReader(1);
      armWatchedWindow(kWatchedWindowReads, 1);
      captureRun(arguments);
      clearGateReader();
      clearWatchedWindow();
      const auto document = readDocument(path);
      // FR-006, R-11: the entry's window holds the samples the probe read
      // during the batch, and the entry's jitter record carries the series
      // that holds them.
      const auto entryName = document.find("\"bmQuiet\"");
      check(document.find("\"series\":[{", entryName) != std::string::npos,
            "theSeriesRoundTripsThroughLoadReport: the entry's jitter record "
            "carries its series array (FR-006, FR-020, R-11)");
      const auto entrySeriesItem = seriesItemAt(document, entryName);
      check(integerValueAfter(document, entrySeriesItem, "sampleCount") > 0,
            "theSeriesRoundTripsThroughLoadReport: the entry's window holds "
            "the watched entry's samples (FR-006, FR-020, R-11)");
      const auto loaded = sg::loadReport(path);
      std::remove(path.c_str());
      check(loaded.has_value(),
            "theSeriesRoundTripsThroughLoadReport: the document of a FWQ run "
            "loads (FR-020)");
      if (!loaded.has_value()) {
        std::printf("  %s\n", loaded.error().c_str());
      } else {
        check(loaded->jitterRun.has_value()
                  && loaded->jitterRun->method == sg::JitterMethod::FWQ,
              "theSeriesRoundTripsThroughLoadReport: the run record reads "
              "back the method word (FR-020, E-25)");
        const auto& folds = loaded->runJitterStats;
        check(folds.has_value() && folds->size() == 1,
              "theSeriesRoundTripsThroughLoadReport: the run record carries "
              "one fold per probe cpu (E-07)");
        if (folds.has_value() && folds->size() == 1) {
          const auto& fold = folds->at(0);
          check(fold.series.has_value(),
                "theSeriesRoundTripsThroughLoadReport: the run-level fold "
                "reads back its series (FR-020)");
          if (fold.series.has_value()) {
            const auto itemAt = seriesItemAt(document, 0);
            checkSeriesRoundTrip(*fold.series,
                                 document,
                                 itemAt,
                                 "theSeriesRoundTripsThroughLoadReport: the "
                                 "run-level series reads back every key "
                                 "(FR-020)");
          }
        }
        const auto* quiet = resultNamed(loaded->results, "bmQuiet");
        check(quiet != nullptr && !quiet->jitterFolds.empty()
                  && quiet->jitterFolds.front().series.has_value(),
              "theSeriesRoundTripsThroughLoadReport: the entry's fold reads "
              "back its series (FR-020, E-08)");
        if (quiet != nullptr && !quiet->jitterFolds.empty()
            && quiet->jitterFolds.front().series.has_value())
        {
          const auto entry = document.find("\"bmQuiet\"");
          const auto entryJitter = document.find("\"jitter\":{", entry);
          const auto entryItem = seriesItemAt(document, entryJitter);
          checkSeriesRoundTrip(quiet->jitterFolds.front().series.value(),
                               document,
                               entryItem,
                               "theSeriesRoundTripsThroughLoadReport: the "
                               "entry's series reads back every key "
                               "(FR-020)");
        }
      }
    }

    // FR-026, FR-031, SC-004: a run with no method key writes neither new
    // key, so the standing record keeps the bytes master wrote.
    {
      const std::string path = "harness_jitter_window_series_tsc.json";
      auto arguments = quick;
      arguments.push_back("--json=" + path);
      arguments.push_back("--jitter-probe=" + probeCpu + ",gaps=256");
      captureRun(arguments);
      const auto document = readDocument(path);
      std::remove(path.c_str());
      check(document.find("\"jitter\":{") != std::string::npos,
            "aTscRunWritesNoNewKey: the tsc run carries the root jitter "
            "object (E-07, FR-031)");
      check(document.find("\"method\"") == std::string::npos
                && document.find("\"series\"") == std::string::npos,
            "aTscRunWritesNoNewKey: the tsc run writes neither the method "
            "key nor the series array (FR-006, FR-026, FR-031)");
    }
  }

  // specs/043 T033, US1, US2, FR-011, FR-013, FR-016, FR-019, FR-022, R-14:
  // the console block of a run that names a method. The 30 series lines
  // stand in the standing order, one field per line and one column per
  // probe cpu, each field printing once; the calibration group stands
  // ahead of the batch's first run; and a rank a cpu does not carry prints
  // 0. Observe red before T035 and T036 land: the block prints the 16 gap
  // lines and nothing of the series.
  {
    const std::string probeCpu = std::to_string(cpus.at(0));
    const std::vector<std::string> quick {
        "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};
    // The FTQ block counts the quanta the alongside loop closed. The
    // armed wait ends behind a thousand of the probe's own reads inside
    // the entry's window, and the loop closes its quanta there (FR-016).
    const std::vector<std::string> watched {
        "--filter=bmQuiet",
        "--repetitions=1",
        "--iterations=" + std::to_string(kWatchedEntryIterations)};

    // FR-013, FR-019, FR-022, R-14: the FWQ block. The dipping clock
    // keeps unit times of 1,000 ns with a dip every 16th unit, so the
    // block names the kept figures, the calibration figures, the unit
    // time, the resample step, and the period the cycle carries.
    if (twoCpuScenarios) {
      auto arguments = quick;
      arguments.push_back("--jitter-probe=" + cpuList
                          + ",method=fwq,gaps=1024");
      sg::detail::installTestReader(std::make_unique<DipSeriesReader>());
      armWatchedWindow(kWatchedWindowReads, 2);
      const auto report = captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      clearWatchedWindow();
      const auto lines = splitLines(report);
      std::size_t previous = 0;
      for (const auto field : kSeriesLines) {
        const std::string name(field);
        std::size_t occurrences = 0;
        const auto at = fieldLineIndex(lines, name, occurrences);
        check(occurrences == 1,
              "theFwqBlockPrintsTheThirtyNewLines: each series field prints "
              "once (FR-019)");
        check(at > 0 && at > previous,
              "theFwqBlockPrintsTheThirtyNewLines: the series lines stand in "
              "the standing order (FR-019, R-14)");
        check(fieldColumns(lines, at, name) == 2,
              "theFwqBlockPrintsTheThirtyNewLines: one column per probe cpu "
              "(FR-019)");
        previous = at;
      }
      std::size_t occurrences = 0;
      const auto methodAt = fieldLineIndex(lines, "method", occurrences);
      check(lines.at(methodAt) == "method: fwq fwq",
            "theFwqBlockPrintsTheThirtyNewLines: the console spells the "
            "method in lower case (FR-019)");
      const auto workAt = fieldLineIndex(lines, "workCount", occurrences);
      check(lines.at(workAt) == "workCount: 1 1",
            "theFwqBlockPrintsTheThirtyNewLines: the calibration count "
            "stands disclosed (FR-011, FR-013)");
      const auto unitAt = fieldLineIndex(lines, "unitTimeNs", occurrences);
      check(lines.at(unitAt) == "unitTimeNs: 1000 1000",
            "theFwqBlockPrintsTheThirtyNewLines: the measured unit time rides "
            "the 16 figures (FR-013, FR-019)");
      const auto maxAt = fieldLineIndex(lines, "maxValue", occurrences);
      check(lines.at(maxAt) == "maxValue: 10000 10000",
            "theFwqBlockPrintsTheThirtyNewLines: the maximum names the dipped "
            "unit (FR-007, FR-013)");
      const auto medianAt = fieldLineIndex(lines, "medianValue", occurrences);
      check(lines.at(medianAt) == "medianValue: 1000 1000",
            "theFwqBlockPrintsTheThirtyNewLines: the median stands the equal "
            "units (FR-007, FR-008)");
      const auto stepAt = fieldLineIndex(lines, "resampleStepNs", occurrences);
      check(lines.at(stepAt) == "resampleStepNs: 1000 1000",
            "theFwqBlockPrintsTheThirtyNewLines: the resample step stands the "
            "median unit time (FR-017, FR-019)");
      const auto periodAt = fieldLineIndex(lines, "period1Ns", occurrences);
      check(lines.at(periodAt) != "period1Ns: 0 0"
                && lines.at(periodAt).find(' ') != std::string::npos,
            "theFwqBlockPrintsTheThirtyNewLines: the strongest period stands "
            "disclosed (FR-018, FR-019)");
      const auto powerAt = fieldLineIndex(lines, "period1Power", occurrences);
      check(lines.at(powerAt) == "period1Power: 1.00 1.00",
            "theFwqBlockPrintsTheThirtyNewLines: the strongest period's power "
            "stands 1 (FR-018, FR-019)");
      const auto droppedAt =
          fieldLineIndex(lines, "droppedSamples", occurrences);
      check(fieldColumns(lines, droppedAt, "droppedSamples") == 2,
            "theFwqBlockPrintsTheThirtyNewLines: the drop count stands "
            "disclosed for every method (FR-022)");

      // FR-011, FR-019, SC-003: the calibration group stands ahead of the
      // batch's first run, so each cpu's window opens behind its own four
      // lines.
      const auto calibrationAt = methodAt;
      std::size_t firstRow = 0;
      for (std::size_t index = 0; index < lines.size(); ++index) {
        if (lines.at(index).rfind("bmQuiet", 0) == 0) {
          firstRow = index;
          break;
        }
      }
      check(calibrationAt > 0 && firstRow > 0 && calibrationAt < firstRow,
            "theCalibrationLinesStandBeforeTheWindow: the calibration lines "
            "stand before the window (FR-011, FR-019, SC-003)");
      const auto targetAt = fieldLineIndex(lines, "targetNs", occurrences);
      const auto spanAt =
          fieldLineIndex(lines, "calibrationSpanNs", occurrences);
      check(calibrationAt < targetAt && targetAt < spanAt
                && spanAt < firstRow,
            "theCalibrationLinesStandBeforeTheWindow: the four calibration "
            "lines stand together ahead of the window (FR-011)");
    }

    // FR-016, FR-019: the FTQ block names the quantum among the 16
    // figures, the counts of FR-007 among them, and the resample step the
    // count series needs none of at 0.
    {
      auto arguments = watched;
      arguments.push_back("--jitter-probe=" + probeCpu
                          + ",method=ftq,gaps=1024");
      sg::detail::installTestReader(std::make_unique<DipSeriesReader>());
      armWatchedWindow(kWatchedWindowReads, 1);
      const auto report = captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      clearWatchedWindow();
      const auto lines = splitLines(report);
      std::size_t occurrences = 0;
      const auto methodAt = fieldLineIndex(lines, "method", occurrences);
      check(occurrences == 1 && lines.at(methodAt) == "method: ftq",
            "theFtqBlockPrintsTheQuantumLines: the console spells the method "
            "ftq (FR-019)");
      const auto quantumAt = fieldLineIndex(lines, "quantumNs", occurrences);
      check(occurrences == 1
                && lines.at(quantumAt) == "quantumNs: 100000",
            "theFtqBlockPrintsTheQuantumLines: the quantum stands among the "
            "16 figures (FR-016, FR-019)");
      const auto samplesAt = fieldLineIndex(lines, "sampleCount", occurrences);
      check(occurrences == 1
                && lines.at(samplesAt) != "sampleCount: 0",
            "theFtqBlockPrintsTheQuantumLines: the closed quanta stand counted "
            "(FR-007, FR-016)");
      const auto stepAt = fieldLineIndex(lines, "resampleStepNs", occurrences);
      check(occurrences == 1 && lines.at(stepAt) == "resampleStepNs: 0",
            "theFtqBlockPrintsTheQuantumLines: a count series needs no "
            "resample step (FR-017, FR-019)");
      const auto pointsAt =
          fieldLineIndex(lines, "spectrumPoints", occurrences);
      check(occurrences == 1 && lines.at(pointsAt) != "spectrumPoints: 0",
            "theFtqBlockPrintsTheQuantumLines: the transform reads the count "
            "series (FR-017, FR-019)");
    }

    // FR-018, FR-019: a rank a cpu does not carry prints 0. The flat
    // clock answers one stamp, so the kept unit times stand at zero, the
    // median step stands below the one nanosecond the transform needs,
    // and the transform reads no span: all ten period lines print 0.
    {
      auto arguments = quick;
      arguments.push_back("--jitter-probe=" + probeCpu
                          + ",method=fwq,gaps=1024");
      sg::detail::installTestReader(std::make_unique<FlatSeriesReader>());
      const auto report = captureRun(arguments);
      sg::detail::installTestReader(nullptr);
      const auto lines = splitLines(report);
      std::size_t occurrences = 0;
      const auto pointsAt =
          fieldLineIndex(lines, "spectrumPoints", occurrences);
      check(occurrences == 1 && lines.at(pointsAt) == "spectrumPoints: 0",
            "theRankACpuOmitsPrintsZero: the transform read no span "
            "(FR-017, FR-019)");
      for (const auto field : kSeriesLines) {
        const std::string name(field);
        if (name.rfind("period", 0) != 0) {
          continue;
        }
        std::size_t periodOccurrences = 0;
        const auto at = fieldLineIndex(lines, name, periodOccurrences);
        check(periodOccurrences == 1,
              "theRankACpuOmitsPrintsZero: each period line prints once "
              "(FR-019)");
        const std::string wanted =
            name + ": " + (name.ends_with("Power") ? "0.00" : "0");
        check(lines.at(at) == wanted,
              "theRankACpuOmitsPrintsZero: a rank the cpu omits prints 0 "
              "(FR-019)");
      }
    }

    // specs/043 T066, US1, US2, FR-004, FR-020, E-27, R-04, and
    // specs/045-merge-fixes FR-008, D-5: the pre-check series of a
    // `mode=both` run. The alongside loop runs on while the pre-check
    // record builds, and the fold reads the latch-time snapshot the
    // probe thread publishes at the latch line: the prefix, its kept
    // count and its drop count stand copied there. The dipping clock
    // keeps unit times of 1,000 ns with a dip every 16th unit, so the
    // prefix names a kept count of its own and a noise fraction of 36
    // percent, and the alongside phase behind the latch keeps samples
    // the prefix leaves out.
    if (twoCpuScenarios) {
      sg::JitterProbeOptions options;
      options.cpus = {cpus.at(0)};
      options.mode = sg::JitterProbeMode::BOTH;
      options.method = sg::JitterMethod::FWQ;
      options.gapCount = 1024;
      options.precheckMs = 1;
      sg::detail::installTestReader(std::make_unique<DipSeriesReader>());
      sg::detail::JitterProbe probe(options);
      probe.start();
      const auto& states = probe.states();
      check(!states.empty(),
            "theBothModePrecheckFoldsTheLatchedPrefix: the probe measured a "
            "cpu (FR-004)");
      const auto& latch = sg::detail::latchSnapshotOf(states.front());
      const auto prefix = sg::detail::seriesStatsOf(latch);
      probe.stopAndJoin();
      const auto full = sg::detail::seriesStatsOf(states.front());
      sg::detail::installTestReader(nullptr);
      check(latch.seriesKept > 0 && prefix.sampleCount == latch.seriesKept,
            "theBothModePrecheckFoldsTheLatchedPrefix: the prefix counts the "
            "samples the latch snapshotted (FR-020, E-27)");
      check(prefix.sampleCount < full.sampleCount,
            "theBothModePrecheckFoldsTheLatchedPrefix: the alongside phase "
            "keeps samples the prefix leaves out (FR-004, R-04)");
      check(prefix.noisePercent > 30.0 && prefix.noisePercent < 40.0,
            "theBothModePrecheckFoldsTheLatchedPrefix: the prefix answers "
            "the pre-check window's noise fraction (FR-004, FR-009)");
    }
  }

  // specs/043 T027, US4, FR-021, FR-030, FR-031, R-18: the rule-name table
  // of the series. The 16 new field names resolve in the four heads, the
  // fault text names the grown table, the run-scoped name reads the run
  // record's series, and the `any` reading answers the largest count of the
  // probe cpus. Observe red before T031 exists: the table carries sixteen
  // names, and the writer of a method run writes no series yet.
  {
    const std::string cpu = std::to_string(cpus.at(0));
    const std::vector<std::string> heads {"jitter:" + cpu + ":",
                                          "jitter:any:",
                                          "jitter:run:" + cpu + ":",
                                          "jitter:run:any:"};
    const std::vector<std::string_view> kSeriesFields {"sampleCount",
                                                       "minValue",
                                                       "medianValue",
                                                       "meanValue",
                                                       "p90Value",
                                                       "p99Value",
                                                       "p999Value",
                                                       "p9999Value",
                                                       "maxValue",
                                                       "noisePercent",
                                                       "seriesRuntimeNs",
                                                       "droppedSamples",
                                                       "unitTimeNs",
                                                       "quantumNs",
                                                       "resampleStepNs",
                                                       "spectrumPoints"};
    for (const auto& head : heads) {
      for (const auto field : kSeriesFields) {
        const std::string text = head + std::string(field) + ":lower:abs=5000";
        const auto rule = sg::parseCompareRule(text);
        check(rule.has_value(),
              "theSixteenNewRuleNamesResolve: the parser accepts the new "
              "field name (FR-021, FR-030)");
        check(rule.has_value() && rule->text == text
                  && rule->valueName == head + std::string(field),
              "theSixteenNewRuleNamesResolve: the rule keeps the text and the "
              "value name it read (D-08)");
        check(rule.has_value() && rule->absoluteThreshold.has_value()
                  && std::fabs(*rule->absoluteThreshold - 5000.0) <= 1e-9,
              "theSixteenNewRuleNamesResolve: the standing suffix's absolute "
              "bound lands in the rule (FR-033)");
      }
    }

    // FR-021, FR-038: the fault of a field outside the table names the
    // grown table.
    const auto fault = sg::parseCompareRule("jitter:any:madeUpNs:lower:abs=1");
    check(!fault.has_value(),
          "a field name outside the table is a fault (FR-038)");
    check(fault.has_value()
              || fault.error().find(
                     "'madeUpNs' names none of the thirty-two jitter fields")
                     != std::string::npos,
          "theFaultTextNamesThirtyTwoFields: the fault names the thirty-two "
          "fields (FR-021, FR-038)");

    // FR-021, FR-031: the run-scoped name reads the run record's series,
    // and the bare name reads the entry's own. The armed wait places the
    // probe's reads inside the entry's own windows, so the bare name has a
    // series to read (FR-006, FR-020, R-11).
    const std::string fwqValue = "--jitter-probe=" + cpu + ",method=fwq,work="
        + std::to_string(kWatchedEntryWork) + ",gaps=1048576";
    const std::vector<std::string> quick {
        "--filter=bmQuiet",
        "--repetitions=" + std::to_string(kWatchedEntryRepetitions),
        "--iterations=" + std::to_string(kWatchedEntryIterations)};
    const std::string basePath = "harness_jitter_rule_series_base.json";
    {
      auto arguments = quick;
      arguments.push_back("--json=" + basePath);
      arguments.push_back(fwqValue);
      installGateReader(1);
      armWatchedWindow(kWatchedWindowReads, 1);
      captureRun(arguments);
      clearGateReader();
      clearWatchedWindow();
    }
    const auto baseDocument = readDocument(basePath);
    check(baseDocument.find("\"series\":[") != std::string::npos,
          "the FWQ run writes the series array (FR-006, FR-020)");
    const std::string rootEdited = "harness_jitter_rule_series_root.json";
    {
      const std::string edited =
          withIntegerReplaced(baseDocument, 0, "sampleCount", 4 * kSeriesJump);
      std::ofstream out(rootEdited);
      out << edited;
      out.close();
    }
    const std::string runRule =
        "--compare-rule=jitter:run:" + cpu + ":sampleCount:lower:abs=1";
    const std::string entryRule =
        "--compare-rule=jitter:" + cpu + ":sampleCount:lower:abs=1";
    const auto rootMoved =
        captureRun({"--compare=" + basePath + "," + rootEdited, runRule}, 1);
    check(rootMoved.find("REGRESSED") != std::string::npos,
          "theRunScopedNameReadsTheRunRecord: the run-scoped name reads the "
          "run record's series (FR-021, FR-031)");
    const auto entryUnmoved =
        captureRun({"--compare=" + basePath + "," + rootEdited, entryRule}, 0);
    check(entryUnmoved.find("REGRESSED") == std::string::npos,
          "theRunScopedNameReadsTheRunRecord: the bare name reads the entry's "
          "series, which the run-level edit leaves equal (FR-021, FR-031)");
    std::remove(rootEdited.c_str());
    std::remove(basePath.c_str());

    // FR-021, D-11: the `any` reading answers the largest count of the
    // probe cpus, the uniform maximum the standing reading carries.
    if (twoCpuScenarios) {
      const std::string pairPath = "harness_jitter_rule_series_pair.json";
      auto arguments = quick;
      arguments.push_back("--json=" + pairPath);
      arguments.push_back("--jitter-probe=" + cpuList
                          + ",method=fwq,gaps=16384");
      installGateReader(2);
      armWatchedWindow(kWatchedWindowReads, 2);
      captureRun(arguments);
      clearGateReader();
      clearWatchedWindow();
      const auto pairDocument = readDocument(pairPath);
      const auto root = pairDocument.find("\"jitter\":{");
      const auto firstCount = pairDocument.find("\"sampleCount\":", root);
      const auto secondCount =
          pairDocument.find("\"sampleCount\":", firstCount + 1);
      check(secondCount != std::string::npos,
            "the run-level series carries one item per probe cpu (FR-020)");
      const std::string pairEdited =
          "harness_jitter_rule_series_pair_edited.json";
      {
        const std::string edited = withIntegerReplaced(
            pairDocument, secondCount, "sampleCount", 4 * kSeriesJump);
        std::ofstream out(pairEdited);
        out << edited;
        out.close();
      }
      const std::string anyRule =
          "--compare-rule=jitter:run:any:sampleCount:lower:abs=1";
      const auto anyMoved =
          captureRun({"--compare=" + pairPath + "," + pairEdited, anyRule}, 1);
      check(anyMoved.find("REGRESSED") != std::string::npos,
            "theAnyReadingAnswersTheLargestCount: the any reading answers the "
            "largest count of the probe cpus (FR-021, D-11)");
      const std::string namedRule =
          "--compare-rule=jitter:run:" + cpu + ":sampleCount:lower:abs=1";
      const auto namedCpu = captureRun(
          {"--compare=" + pairPath + "," + pairEdited, namedRule}, 0);
      check(namedCpu.find("REGRESSED") == std::string::npos,
            "theAnyReadingAnswersTheLargestCount: the named cpu's own count "
            "stands where the edit left it (FR-021, D-11)");
      std::remove(pairEdited.c_str());
      std::remove(pairPath.c_str());
    }

    // FR-021, FR-030, E-22: every one of the sixteen series fields reads
    // through its own reader of the field table. One comparison run carries
    // one rule per field over two documents that carry the same series, so
    // each reader answers both reports' values and the run reads no fault.
    {
      const std::string allPath = "harness_jitter_rule_series_all.json";
      auto arguments = quick;
      arguments.push_back("--json=" + allPath);
      arguments.push_back(fwqValue);
      installGateReader(1);
      armWatchedWindow(kWatchedWindowReads, 1);
      captureRun(arguments);
      clearGateReader();
      clearWatchedWindow();
      std::vector<std::string> fieldArguments {"--compare=" + allPath + ","
                                               + allPath};
      for (const auto field : kSeriesFields) {
        fieldArguments.push_back(
            "--compare-rule=jitter:any:" + std::string(field) + ":lower:abs=1");
      }
      const auto fields = captureRun(fieldArguments, 0);
      check(fields.find("error:") == std::string::npos,
            "every series field of the table reads without a fault (FR-021, "
            "FR-030, E-22)");
      std::remove(allPath.c_str());
    }

    // FR-021, FR-032, E-22: a fold that carries no series answers 0 for
    // every series field, the reading of a run that kept no samples. A
    // `tsc` run keeps gaps and writes no series key, so each of the sixteen
    // readers reaches the empty series a fold leaves behind.
    {
      const std::string tscPath = "harness_jitter_rule_series_tsc.json";
      auto& provider = scriptedProvider();
      provider.setPoints("machine", "tsc", {}, kPacedStepNs);
      seriesJump() = kSeriesJump;
      installGateReader(1);
      captureRun({"--filter=bmGate",
                  "--repetitions=1",
                  "--iterations=2",
                  "--json=" + tscPath,
                  "--jitter-probe=" + cpu + ",method=tsc"});
      clearGateReader();
      provider.setPoints("machine", "tsc", {}, kPacedStepNs);
      const auto tscDocument = readDocument(tscPath);
      check(tscDocument.find("\"series\":[") == std::string::npos,
            "the tsc run writes no series array (FR-006, SC-004)");
      check(tscDocument.find("\"jitter\":{\"stats\"") != std::string::npos,
            "the tsc run's folds carry the probe cpu (E-08)");
      std::vector<std::string> fieldArguments {"--compare=" + tscPath + ","
                                               + tscPath};
      for (const auto field : kSeriesFields) {
        fieldArguments.push_back(
            "--compare-rule=jitter:any:" + std::string(field) + ":lower:abs=1");
      }
      const auto fields = captureRun(fieldArguments, 0);
      check(fields.find("error:") == std::string::npos,
            "theTscFoldsAnswerTheEmptySeries: each series field reads the "
            "empty series of a tsc fold (FR-021, FR-032, E-22)");
      std::remove(tscPath.c_str());
    }
  }

  // specs/048-hwlat-probe T037: the detector lines of the two blocks. The
  // rows stand red until T043 prints them, so they close the unit. The two
  // rows of the standing method guard the bytes the standing runs keep.
  aStandingMethodPrintsNoDetectorLine();
  theThreeProbeFigureLinesStandAfterTheStandingCount();
  theTwoCounterDisclosuresPrintOncePerRun();
  theLimitsLineStandsOncePerDetectorRun();
  theThreeAttributionLinesStandPerMeasuredCpu();
  aStandingMethodPrintsNoAttributionLine();

  std::puts("harness_jitter_window_test: ok");
  return 0;
}
