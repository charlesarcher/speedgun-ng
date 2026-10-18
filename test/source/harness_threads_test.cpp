// ============================================================================
// TDD test for one benchmark over N contexts (T014 to T017; US2, C-5, FR-016,
// FR-017, FR-020, research D-06; SC-06, SC-07).
//
// One scripted provider steps both machine leaves by a constant amount per
// sampling action, so the fold of one window is the count of the actions
// between its two endpoints times that step:
//
//   machine/monotonic   1000 ns per action
//   machine/thread_cpu   800 ns per action
//
// FR-018 and SC-09 keep every fold inside the plan of the context that owns
// it, so a context's playback of a scripted leaf advances only on that
// context's own reads: its window spans its own opening read and its own
// closing read and nothing else, one step of each leaf. The round still
// stands - every body joins it at the first iteration and no body leaves its
// loop before all four have arrived - but it no longer puts the endpoint
// reads of one context inside the window of another, so the fold of a
// context is the same one step whether it runs alone or beside three others.
// That is the property SC-09 tests with a noise load on a second context.
//
// Four contexts of four iterations therefore owe the merged row:
//
//   iterations         4 x 4                  =    16
//   real time/iter     4 x 1000 over 16       =   250.000
//   cpu time/iter      4 x  800 over 16       =   200.000
//
// which is the value the same body prints on one context: four iterations of
// one window fold one step, so a run on more contexts divides the same step
// by the same four iterations and cannot come out a multiple of it.
//
// and the decision seconds each timing mode divides a rate by, divided by the
// thread count of four, are the summed time of that mode:
//
//   default            4 x  800 / 4 =  800 ns
//   real time          4 x 1000 / 4 = 1000 ns
//   manual time        4 x 4 x 2000 / 4 = 8000 ns, with the summed manual time
//                      of 32000 ns standing in the real column over 16
//                      iterations, so 2000.000 per iteration
//
// The merged-row scenario carries six counters of 125.0 on every context, so
// the value the flag pass takes is the summed 500.0, and the cited `Finish`
// order at the thread count of four owes:
//
//   plain          500.000, the summed value with no divisor
//   rate           500 / 3.2e-6  = 156250000.000
//   avgthreads     500 / 4       =      125.000
//   iterinvariant  500 x 16      =     8000.000
//   avgiterations  500 / 16      =       31.250
//   invert         1 / 500       =        0.002
//
// The three decision scenarios read the divisor back off the row's own column
// of the mode, so each one pins the column the divisor stands on.
//
// The range, dense-range and runner scenarios cover the rest of the thread
// surface: the expansion of a count set into instances, the name segment of
// each count, and a dispatch function of the user's over the run's contexts,
// each with the breach at a count below one (FR-016, FR-021, FR-024).
//
// The noise-load scenarios of User Story 3 ride the same seams beside a load
// context of the test's own: the load runs on a caller-owned context, folds two
// leaves of its own scripted to steps of their own, and leaves the benchmark's
// row the only thing its own plan folded (SC-09, FR-018, PC-1).
//
// Each scenario registers one provider script and one family, so each runs in a
// re-exec of this binary and the parent reads its wait status. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/gcov_flush.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace
{

constexpr int kThreadCount = 4;
constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kMergedIterations =
    static_cast<std::uint64_t>(kThreadCount) * kIterations;

constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kWorkStep = 300;
constexpr std::uint64_t kThreadCpuStep = 800;
// One context's window holds its own playback, so it folds one step of each
// leaf: the opening read carries point zero and the closing read one step
// later, and no other context's sampling stands between them (FR-018, SC-07,
// SC-09). The merged fold is that step once per context.
constexpr std::uint64_t kWindowSteps = 1;
constexpr std::uint64_t kMergedSteps =
    static_cast<std::uint64_t>(kThreadCount) * kWindowSteps;

constexpr double kMergedRealPerIteration = static_cast<double>(kMergedSteps)
    * static_cast<double>(kMonotonicStep)
    / static_cast<double>(kMergedIterations);
constexpr double kMergedCpuPerIteration = static_cast<double>(kMergedSteps)
    * static_cast<double>(kThreadCpuStep)
    / static_cast<double>(kMergedIterations);

constexpr std::int64_t kManualIterationNs = 2000;
constexpr double kManualMergedNs = static_cast<double>(kManualIterationNs)
    * static_cast<double>(kMergedIterations);

constexpr double kCounterValue = 125.0;
constexpr double kMergedCounterValue =
    static_cast<double>(kThreadCount) * kCounterValue;

constexpr std::int64_t kItems = 500;
constexpr double kMergedItems =
    static_cast<double>(kThreadCount) * static_cast<double>(kItems);

// The arrivals of the bodies, so the round is joined once per attempt even
// where the run carries more than one.
std::atomic<int> bodyEntered {0};

// The index and the count each body of the index scenario read, tallied by the
// value read. Slots stand for the values 0 to kStateSlots - 1.
constexpr int kStateSlots = 8;
std::atomic<int> indexRuns[kStateSlots] {};
std::atomic<int> threadsRuns[kStateSlots] {};
std::atomic<int> indexOutOfRange {0};
std::atomic<int> threadsOutOfRange {0};

auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "HARNESS THREADS TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* const what) -> void
{
  if (!condition) {
    fail(what);
  }
}

auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addCounter("fake", "work", "ops", "scripted work counter");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  owned->setPoints("fake", "work", {0}, kWorkStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// The round of the body's first iteration: the arrival of one more context,
// then the wait for the rest of the run's contexts. FR-021 stands behind the
// count, one dispatch per body index, so every context of the run arrives.
auto enterRound(sg::State& state, const bool first) -> void
{
  if (!first || state.threads() != kThreadCount) {
    return;
  }
  const int arrived = bodyEntered.fetch_add(1, std::memory_order_release) + 1;
  const int roundEnd = (1 + ((arrived - 1) / kThreadCount)) * kThreadCount;
  while (bodyEntered.load(std::memory_order_acquire) < roundEnd) {
    std::this_thread::yield();
  }
}

auto bmIndexAndName(sg::State& state) -> void
{
  for (auto index : state) {
    const int threadIndex = state.threadIndex();
    if (threadIndex >= 0 && threadIndex < kStateSlots) {
      indexRuns[threadIndex].fetch_add(1, std::memory_order_relaxed);
    } else {
      indexOutOfRange.fetch_add(1, std::memory_order_relaxed);
    }
    const int threads = state.threads();
    if (threads >= 0 && threads < kStateSlots) {
      threadsRuns[threads].fetch_add(1, std::memory_order_relaxed);
    } else {
      threadsOutOfRange.fetch_add(1, std::memory_order_relaxed);
    }
    sg::doNotOptimize(index);
  }
}

// The six flags of the cited `Finish` order, one name each, every context
// setting every one of them.
auto bmMergedRow(sg::State& state) -> void
{
  for (auto index : state) {
    enterRound(state, index == 0);
    if (index == 0) {
      state.setCounter("plain", {kCounterValue});
      state.setCounter("rate", {kCounterValue, sg::CounterFlag::RATE});
      state.setCounter("avgthreads",
                       {kCounterValue, sg::CounterFlag::AVG_THREADS});
      state.setCounter("iterinvariant",
                       {kCounterValue, sg::CounterFlag::ITERATION_INVARIANT});
      state.setCounter("avgiterations",
                       {kCounterValue, sg::CounterFlag::AVG_ITERATIONS});
      state.setCounter("invert", {kCounterValue, sg::CounterFlag::INVERT});
    }
    sg::doNotOptimize(index);
  }
}

// The item rate, the one column whose divisor is the decision seconds of the
// run's timing mode divided by the thread count.
auto bmMergedItems(sg::State& state) -> void
{
  for (auto index : state) {
    enterRound(state, index == 0);
    if (index == 0) {
      state.setItemsProcessed(kItems);
    }
    sg::doNotOptimize(index);
  }
}

// The manual mode: the body hands the harness one time per iteration, so the
// summed manual time needs no leaf to stand for it.
auto bmMergedManual(sg::State& state) -> void
{
  for (auto index : state) {
    state.setIterationTime(kManualIterationNs);
    if (index == 0) {
      state.setItemsProcessed(kItems);
    }
    sg::doNotOptimize(index);
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int& exitStatus) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_threads_test"));
  for (const std::string& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_threads_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  exitStatus = status;

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The report of a run that owes the success status. A scenario that expects a
// failure reads the status from the overload above instead.
auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  int status = 0;
  std::string report = captureRun(arguments, status);
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("the run did not end with the success status");
  }
  return report;
}

auto lineOf(const std::string& report, const std::string& name) -> std::string
{
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.starts_with(name)) {
      return line;
    }
  }
  return {};
}

auto fieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

auto countFieldOf(const std::string& line,
                  const std::string& key) -> std::int64_t
{
  return static_cast<std::int64_t>(fieldOf(line, key));
}

auto closeTo(const double value, const double expected) -> bool
{
  return std::abs(value - expected) <= 1e-3 + (1e-9 * std::abs(expected));
}

// The row's first field, the instance name the report prints.
auto nameOf(const std::string& line) -> std::string
{
  const std::size_t space = line.find(' ');
  return space == std::string::npos ? line : line.substr(0, space);
}

// The summed decision seconds the printed column of one mode stands for: the
// per-iteration time of that column over the iterations of the row.
auto summedSecondsOf(const std::string& line, const std::string& key) -> double
{
  return fieldOf(line, key)
      * static_cast<double>(countFieldOf(line, "iterations=")) * 1e-9;
}

// SC-06, FR-016, FR-017: the four contexts report their indexes, the count,
// and the name segment of the count they run under.
auto scenarioIndexAndName() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmIndexAndName, "bmThreadsIndex");
  handle.threads(kThreadCount);

  const std::string report =
      captureRun({"--filter", "^bmThreadsIndex", "--iterations=4"});

  for (int index = 0; index != kThreadCount; ++index) {
    check(indexRuns[index].load() > 0,
          "every thread index from 0 to 3 reaches the body (FR-017, SC-06)");
  }
  for (int index = kThreadCount; index != kStateSlots; ++index) {
    check(indexRuns[index].load() == 0,
          "no thread index above 3 reaches the body (FR-017, SC-06)");
  }
  check(indexOutOfRange.load() == 0,
        "no thread index outside the slots reaches the body (FR-017)");
  for (int count = 0; count != kStateSlots; ++count) {
    check(count == kThreadCount ? threadsRuns[count].load() > 0
                                : threadsRuns[count].load() == 0,
          "every body reads the thread count of 4 (FR-017, SC-06)");
  }
  check(threadsOutOfRange.load() == 0,
        "no thread count outside the slots reaches the body (FR-017)");

  const std::string row = lineOf(report, "bmThreadsIndex");
  check(!row.empty(), "the threaded instance prints its row (FR-016)");
  check(nameOf(row).ends_with("/threads:4"),
        "the row name ends in the threads segment of its count (FR-016)");

  std::puts("harness_threads_test index-and-name: ok");
  return 0;
}

// SC-07, FR-020: the merged row sums the two times, the iterations, and every
// counter name ahead of the flag pass, and every flag carries the cited
// `Finish` result at the thread count of four.
auto scenarioMergedRow() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmMergedRow, "bmThreadsMerge");
  handle.threads(kThreadCount);

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsMerge", "--iterations=4"}),
             "bmThreadsMerge");
  check(!row.empty(), "the threaded instance prints its row (FR-016)");

  check(closeTo(fieldOf(row, "real time/iter (ns)="), kMergedRealPerIteration),
        "the real column carries the summed real time of the four contexts "
        "(FR-020, SC-07)");
  check(closeTo(fieldOf(row, "cpu time/iter (ns)="), kMergedCpuPerIteration),
        "the cpu column carries the summed cpu time of the four contexts "
        "(FR-020, SC-07)");
  check(countFieldOf(row, "iterations=")
            == static_cast<std::int64_t>(kMergedIterations),
        "the iterations are the summed per-context counts (FR-020, SC-07)");
  check(closeTo(fieldOf(row, "plain="), kMergedCounterValue),
        "the counter names are summed over the contexts before the flag pass "
        "(FR-020)");
  check(closeTo(fieldOf(row, "rate="),
                kMergedCounterValue
                    / (kMergedCpuPerIteration
                       * static_cast<double>(kMergedIterations) * 1e-9
                       / static_cast<double>(kThreadCount))),
        "RATE divides the summed value by the summed cpu time divided by the "
        "thread count (FR-020, D-06)");
  check(closeTo(fieldOf(row, "avgthreads="),
                kMergedCounterValue / static_cast<double>(kThreadCount)),
        "AVG_THREADS divides the summed value by the thread count (FR-020, "
        "D-06)");
  check(closeTo(fieldOf(row, "iterinvariant="),
                kMergedCounterValue
                    * static_cast<double>(kMergedIterations)),
        "ITERATION_INVARIANT multiplies by the merged iteration count "
        "(FR-020, D-06)");
  check(closeTo(fieldOf(row, "avgiterations="),
                kMergedCounterValue / static_cast<double>(kMergedIterations)),
        "AVG_ITERATIONS divides by the merged iteration count (FR-020, D-06)");
  check(closeTo(fieldOf(row, "invert="),
                1.0 / kMergedCounterValue),
        "INVERT takes the reciprocal of the flag-applied value last (FR-020, "
        "D-06)");

  std::puts("harness_threads_test merged-row: ok");
  return 0;
}

// FR-020, FR-022, D-06: a metric attached with addMetric on a threaded
// instance is folded per context and summed into the merged row, and a
// per-iteration metric of the merged row carries the value the same
// benchmark shows on its single-context row: the summed net over the
// summed iteration count divides back to the same per-iteration rate.
auto scenarioMergedMetric() -> int
{
  scriptProvider();
  auto attach = [](sg::BenchmarkHandle& handle)
  {
    const auto object = sg::counters::System::local().object("fake");
    check(object.has_value(), "the scripted work object resolves");
    const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
    check(work.has_value(), "the scripted work leaf resolves");
    handle.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*work),
                     "work");
  };

  auto threaded = sg::registerBenchmark(&bmMergedRow, "bmThreadsMetric");
  attach(threaded);
  threaded.threads(kThreadCount);
  auto alone = sg::registerBenchmark(&bmMergedRow, "bmSingleMetric");
  attach(alone);

  const std::string report = captureRun(
      {"--filter", "^bmThreadsMetric|^bmSingleMetric", "--iterations=4"});
  const std::string merged = lineOf(report, "bmThreadsMetric");
  const std::string single = lineOf(report, "bmSingleMetric");
  check(!merged.empty() && !single.empty(),
        "both instances print their row (FR-016)");
  check(fieldOf(merged, "work=") > 0.0,
        "the merged row carries the attached metric column (FR-020)");
  check(closeTo(fieldOf(merged, "work="), fieldOf(single, "work=")),
        "the merged per-iteration metric equals the single-context row's, the "
        "summed net over the summed iterations (FR-020, FR-022, D-06)");

  std::puts("harness_threads_test merged-metric: ok");
  return 0;
}

// A body that pauses inside every iteration, so the recorder of each context
// must hold the pairs its own body creates (FR-013, FR-052).
auto bmPausedRow(sg::State& state) -> void
{
  for (auto index : state) {
    enterRound(state, index == 0);
    state.pauseTiming();
    state.resumeTiming();
    sg::doNotOptimize(index);
  }
}

// FR-020, D-06, SC-07: the default mode divides by the summed cpu time over
// the thread count.
// FR-013, FR-052, FR-018: pause and resume above one context. Each context
// folds the pairs its own body closed, so the merged row's sample count is the
// sum over the contexts, and a body that pauses must count above the same body
// without the pause. The combination is what no scenario covered: the room each
// context's recorder reserves grows from what its own body did.
auto scenarioPausedContexts() -> int
{
  scriptProvider();
  auto paused = sg::registerBenchmark(&bmPausedRow, "bmThreadsPause");
  paused.threads(kThreadCount);
  auto steady = sg::registerBenchmark(&bmMergedRow, "bmThreadsNoPause");
  steady.threads(kThreadCount);
  auto alone = sg::registerBenchmark(&bmPausedRow, "bmSinglePause");
  (void)alone;

  const std::string report =
      captureRun({"--filter",
                  "^bmThreadsPause|^bmThreadsNoPause|^bmSinglePause",
                  "--iterations=4"});
  const std::string merged = lineOf(report, "bmThreadsPause");
  const std::string steadyRow = lineOf(report, "bmThreadsNoPause");
  const std::string single = lineOf(report, "bmSinglePause");
  check(!merged.empty() && !steadyRow.empty() && !single.empty(),
        "the three instances print their rows (FR-016)");

  const double pausedSamples = fieldOf(merged, "endpoint samples=");
  check(pausedSamples > fieldOf(steadyRow, "endpoint samples="),
        "the pairs the contexts' bodies closed reach the merged row's sample "
        "count instead of being lost (FR-013, FR-052)");
  check(closeTo(pausedSamples, 4.0 * fieldOf(single, "endpoint samples=")),
        "the merged sample count is the sum over the four contexts, each "
        "folding its own plan (FR-018, FR-020)");

  std::puts("harness_threads_test paused-contexts: ok");
  return 0;
}

auto scenarioCpuDecision() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmMergedItems, "bmThreadsCpuRate");
  handle.threads(kThreadCount);

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsCpuRate", "--iterations=4"}),
             "bmThreadsCpuRate");
  check(!row.empty(), "the threaded instance prints its row (FR-016)");
  check(closeTo(fieldOf(row, "real time/iter (ns)="), kMergedRealPerIteration),
        "the real column carries the summed real time (FR-020)");
  check(closeTo(fieldOf(row, "cpu time/iter (ns)="), kMergedCpuPerIteration),
        "the cpu column carries the summed cpu time (FR-020)");
  check(closeTo(fieldOf(row, "items_per_second="),
                kMergedItems
                    / (summedSecondsOf(row, "cpu time/iter (ns)=")
                       / static_cast<double>(kThreadCount))),
        "the rate divides by the summed cpu time divided by the thread count "
        "of the default mode (FR-020, D-06; SC-07)");

  std::puts("harness_threads_test cpu-decision: ok");
  return 0;
}

// FR-020, D-06, SC-07: the real-time mode divides by the summed real time over
// the thread count.
auto scenarioRealDecision() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmMergedItems, "bmThreadsRealRate");
  handle.threads(kThreadCount).useRealTime();

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsRealRate", "--iterations=4"}),
             "bmThreadsRealRate");
  check(!row.empty(), "the threaded instance prints its row (FR-016)");
  check(closeTo(fieldOf(row, "real time/iter (ns)="), kMergedRealPerIteration),
        "the real column carries the summed real time (FR-020)");
  check(closeTo(fieldOf(row, "items_per_second="),
                kMergedItems
                    / (summedSecondsOf(row, "real time/iter (ns)=")
                       / static_cast<double>(kThreadCount))),
        "the rate divides by the summed real time divided by the thread count "
        "of the real-time mode (FR-020, D-06; SC-07)");

  std::puts("harness_threads_test real-decision: ok");
  return 0;
}

// FR-020, D-06, SC-07: the manual mode stands the summed manual time in the
// real column and divides the rate by it over the thread count.
auto scenarioManualDecision() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmMergedManual, "bmThreadsManualRate");
  handle.threads(kThreadCount).useManualTime();

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsManualRate", "--iterations=4"}),
             "bmThreadsManualRate");
  check(!row.empty(), "the threaded instance prints its row (FR-016)");
  check(countFieldOf(row, "iterations=")
            == static_cast<std::int64_t>(kMergedIterations),
        "the iterations are the summed per-context counts (FR-020)");
  check(closeTo(fieldOf(row, "real time/iter (ns)="),
                kManualMergedNs / static_cast<double>(kMergedIterations)),
        "the manual-time sum stands in the real column (FR-020, SC-07)");
  check(closeTo(fieldOf(row, "items_per_second="),
                kMergedItems
                    / ((kManualMergedNs * 1e-9)
                       / static_cast<double>(kThreadCount))),
        "the rate divides by the summed manual time divided by the thread "
        "count of the manual mode (FR-020, D-06; SC-07)");

  std::puts("harness_threads_test manual-decision: ok");
  return 0;
}

}  // namespace

// ---------------------------------------------------------------------------
// User Story 3: the noise load beside a measurement (T021).
//
// A caller-owned context runs a load that hands itself its next round, so
// nothing but the scenario's stop request ends it; the benchmark runs on the
// contexts the harness creates beside it. The load folds two leaves scripted to
// steps of their own, so a value that came from the load's plan would carry a
// number the benchmark's row must never carry (FR-018, SC-09, PC-1). The last
// scenario hands work to the running load from inside a running round and from
// a second dispatch thread, the rule FR-013 states, observed beside a
// measurement (FR-010, FR-013).
//
// A named namespace stands in for the anonymous one for the reason the
// execution-context suite records: these scenarios hand item types to
// `dispatch` and `WorkNode`, and a node whose item type has internal linkage is
// the subobject-linkage case this project's `-Wall -Werror` build diagnoses.
// The scenarios still reach every helper above, because an anonymous namespace
// is visible to the rest of its translation unit.
namespace threads_noise
{

// The load's two leaves carry steps far above the benchmark's, so no value of a
// fold that crossed contexts can be mistaken for a value of the benchmark's own
// plan (FR-018, SC-09, PC-1).
constexpr std::uint64_t kNoiseMonotonicStep = 5'000'000;
constexpr std::uint64_t kNoiseCpuStep = 3'000'000;

// The merged values those two leaves would print over the same sixteen sampling
// actions: the numbers the benchmark's row must never carry (SC-09).
constexpr double kNoiseRealPerIteration = static_cast<double>(kWindowSteps)
    * static_cast<double>(kNoiseMonotonicStep)
    / static_cast<double>(kMergedIterations);
constexpr double kNoiseCpuPerIteration = static_cast<double>(kWindowSteps)
    * static_cast<double>(kNoiseCpuStep)
    / static_cast<double>(kMergedIterations);

// The busy work one load round does: the cpu the load competes for.
constexpr std::uint64_t kLoadBurn = 20'000;

// The rounds the scenario waits for before the run begins, so the load stands
// running rather than merely started (SC-09).
constexpr int kRoundsBeforeRun = 2;

// The load's time dimension, the tag the harness resolves its own two leaves
// under (FR-018).
using NoiseTime = sg::counters::Dim<1, 0>;

// The load context and the plan it folds. The plan is compiled on the load's
// own thread by its first round: plans, recorders and windows are per-thread
// objects, and the counters seam refuses a plan sampled on a thread other than
// the one that opened it (FR-018, PC-1).
std::unique_ptr<sg::ExecutionContext> loadOwner;
sg::ExecutionContext* load = nullptr;
std::optional<sg::counters::Plan> loadPlan;
sg::counters::Expression<NoiseTime> loadReal;
sg::counters::Expression<NoiseTime> loadCpu;

// The order the load context's items ran in. Every claim comes from that one
// thread, so the numbers order the items with no clock (FR-013).
std::atomic<std::uint64_t> sequence {0};
std::atomic<int> loadRounds {0};  // rounds the load has entered
std::atomic<int> loadInside {0};  // items of the load inside now
std::atomic<int> loadOverlap {0};  // rounds that found an item inside
std::atomic<int> loadForeign {0};  // items that named another context
std::atomic<int> loadPlanState {0};  // 1 once opened, 2 once refused
std::atomic<int> loadWindowBad {0};  // rounds that folded a foreign leaf
std::atomic<int> markerQueued {0};  // the second thread's call returned
std::atomic<int> markerWaited {0};  // its node waited, never ran
std::atomic<std::uint64_t> round0Exit {0};  // the order round 0 ended at
std::atomic<int> roundsAtBodyEntry {0};  // rounds live when a body entered
// Armed before the context stands built and never written afterwards, so the
// context's thread sees it through the thread's own creation.
int holdFirstRound = 0;

// Both names of the running context agree on the context the scenario started
// (FR-002, FR-011).
auto namesLoadContext(const sg::WorkContext& work) -> bool
{
  const auto current = sg::currentContext();
  return load != nullptr && current.has_value()
      && current->serial == work.id().serial
      && work.id().serial == load->id().serial;
}

// What one proof item records about the run that carried it (FR-010, FR-013).
struct ProofRecord
{
  std::atomic<std::uint64_t> entry {0};  // its place in the run order
  std::atomic<std::uint64_t> serial {0};  // the context it ran on
  std::atomic<int> insideItem {0};  // it ran while another item ran
  std::atomic<int> namedContext {0};  // both names named this context
};

// The proof item: it records where and when it ran and nothing else, so the
// scenario reads the order the item landed in (FR-010, FR-013).
struct ProofItem
{
  ProofRecord* record = nullptr;

  auto operator()(sg::WorkContext& work) const noexcept -> void
  {
    record->entry.store(sequence.fetch_add(1, std::memory_order_relaxed) + 1,
                        std::memory_order_release);
    if (loadInside.load(std::memory_order_acquire) != 0) {
      record->insideItem.store(1, std::memory_order_release);
    }
    record->serial.store(work.id().serial, std::memory_order_release);
    if (namesLoadContext(work)) {
      record->namedContext.store(1, std::memory_order_release);
    }
  }
};

ProofRecord nestedRecord;
ProofRecord markerRecord;
sg::WorkNode<ProofItem> nestedNode {ProofItem {.record = &nestedRecord}};
sg::WorkNode<ProofItem> markerNode {ProofItem {.record = &markerRecord}};

// The load's own plan, opened on the load's thread by its first round. The
// scenario waits on the state, so the open stands before the harness's run
// rather than racing it (FR-018, PC-1).
auto openOwnPlan() noexcept -> void
{
  auto& system = sg::counters::System::local();
  const auto object = system.object("noise");
  if (!object.has_value()) {
    loadPlanState.store(2, std::memory_order_release);
    return;
  }
  const auto real = object->counter<NoiseTime>("monotonic");
  if (!real.has_value()) {
    loadPlanState.store(2, std::memory_order_release);
    return;
  }
  const auto cpu = object->counter<NoiseTime>("thread_cpu");
  if (!cpu.has_value()) {
    loadPlanState.store(2, std::memory_order_release);
    return;
  }
  loadReal = sg::counters::Expression<NoiseTime> {*real};
  loadCpu = sg::counters::Expression<NoiseTime> {*cpu};
  auto compiled =
      sg::counters::compile(system, sg::counters::Target {}, loadReal, loadCpu);
  if (!compiled.has_value()) {
    loadPlanState.store(2, std::memory_order_release);
    return;
  }
  loadPlan = std::move(*compiled);
  loadPlanState.store(1, std::memory_order_release);
}

// The window the load folds over its own two leaves: the two-point shape a
// benchmark window has, over leaves no other thread reads, so the fold is the
// load's own and the round's busy work stands inside it (FR-018, PC-1).
auto foldOwnWindow(sg::counters::Plan& plan) noexcept -> void
{
  sg::counters::Scope window {plan};
  window.start();
  std::uint64_t sink = 0;
  for (std::uint64_t step = 0; step != kLoadBurn; ++step) {
    sink += step;
    sg::doNotOptimize(sink);
  }
  window.finish();
  if (!closeTo(window.metric(loadReal).value,
               static_cast<double>(kNoiseMonotonicStep))
      || !closeTo(window.metric(loadCpu).value,
                  static_cast<double>(kNoiseCpuStep)))
  {
    loadWindowBad.fetch_add(1, std::memory_order_relaxed);
  }
}

// One round of the noise load: its own window over its own leaves, the busy
// work that competes for the cpu, and the next round handed to the same
// context. A round hands itself the successor, so the load never stops on its
// own (SC-09, FR-006, FR-010).
auto loadRound(sg::WorkContext& work) noexcept -> void;

// A function pointer is the item, so both nodes share one type and either can
// name the other as the successor.
using LoadItem = decltype(&loadRound);
sg::WorkNode<LoadItem> loadNodeA {&loadRound};
sg::WorkNode<LoadItem> loadNodeB {&loadRound};

auto loadRound(sg::WorkContext& work) noexcept -> void
{
  if (loadInside.fetch_add(1, std::memory_order_acq_rel) != 0) {
    loadOverlap.fetch_add(1, std::memory_order_relaxed);
  }
  const int round = loadRounds.fetch_add(1, std::memory_order_relaxed);
  if (!namesLoadContext(work)) {
    loadForeign.fetch_add(1, std::memory_order_relaxed);
  }
  if (round == 0) {
    openOwnPlan();
  }
  if (loadPlan.has_value()) {
    foldOwnWindow(*loadPlan);
  }
  if (round == 0 && holdFirstRound != 0) {
    // FR-013: the node dispatched from inside this round is queued now and
    // cannot run before this item has returned.
    sg::dispatch(*load, nestedNode);
    // The second dispatch thread waits for this round to stand inside and hands
    // its node to the running context. The round stays inside until that call
    // has returned, so the state read below names a node that waits for this
    // item to end rather than one that overtook it: FR-010's dispatch returns
    // without waiting, observed. The round turns until the thread reaches its
    // dispatch, which it always does; no clock stands in that wait.
    while (markerQueued.load(std::memory_order_acquire) == 0) {
      std::this_thread::yield();
    }
    if (markerNode.state() == sg::WorkNodeState::PENDING) {
      markerWaited.store(1, std::memory_order_release);
    }
  }
  if (round == 0) {
    round0Exit.store(sequence.fetch_add(1, std::memory_order_relaxed) + 1,
                     std::memory_order_release);
  }
  loadInside.fetch_sub(1, std::memory_order_acq_rel);
  if (work.stopToken().stopRequested()) {
    return;
  }
  // The round hands the other node: the node a round runs on stays the loop's
  // until the loop has written its completion state (FR-012).
  sg::dispatch(*load, round % 2 == 0 ? loadNodeB : loadNodeA);
}

// The load context under the spinning policy: the competitor the story is
// about, waiting in SPIN so it contends for the cpu it shares with the
// benchmark (FR-004, D-3).
auto startLoad() -> void
{
  sg::ContextPolicy policy;
  policy.waitMode = sg::WaitMode::SPIN;
  loadOwner = std::make_unique<sg::ExecutionContext>(policy);
  load = loadOwner.get();
  sg::dispatch(*loadOwner, loadNodeA);
}

// The wait for the load to stand running: its own plan open on its own thread
// and its second round entered. The loop turns on the load's own signals, with
// no clock and no bound, and the load reaches both on its own (SC-09, FR-010).
auto awaitLoadRunning() -> void
{
  while (loadPlanState.load(std::memory_order_acquire) == 0
         || loadRounds.load(std::memory_order_acquire) < kRoundsBeforeRun)
  {
    std::this_thread::yield();
  }
}

// The second dispatch thread: it waits for the load's first round to stand
// inside, hands its node to that running context, and records that the call
// came back (FR-010, FR-013).
auto dispatchFromSecondThread() -> std::thread
{
  return std::thread {
      []
      {
        while (loadPlanState.load(std::memory_order_acquire) == 0) {
          std::this_thread::yield();
        }
        sg::dispatch(*load, markerNode);
        markerQueued.store(1, std::memory_order_release);
      }};
}

// The end of the load: the stop request, then the flush that proves every
// accepted node ran. That flush is the one wait the story needs, with no clock
// and no bound (FR-006, FR-050).
auto stopLoad() -> void
{
  loadOwner->requestStop();
  loadOwner->drain();
  loadOwner->join();
  loadOwner.reset();
  load = nullptr;
}

// One provider script per scenario process, in the standing pattern. The two
// leaves the harness folds keep the merged-row arithmetic, and the two leaves
// the load folds are scripted to steps of their own, so no value of one plan
// can be mistaken for a value of the other (FR-018, SC-09, PC-1).
auto scriptLoadProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, kMonotonicStep);
  provider.setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  provider.addCounter("noise", "monotonic", "nanoseconds", "load monotonic");
  provider.addCounter("noise", "thread_cpu", "nanoseconds", "load thread cpu");
  provider.setPoints("noise", "monotonic", {0}, kNoiseMonotonicStep);
  provider.setPoints("noise", "thread_cpu", {0}, kNoiseCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(),
        "the scripted load leaves register before the scenario dispatches");
}

// The body of the noise scenarios. It joins the round, so the merged row's
// arithmetic is the arithmetic the merged-row scenario pins, and it records the
// load's rounds at the moment a benchmark thread enters its loop. The load ends
// only when the scenario asks it to, after the row printed, so the rounds
// standing then stood while the benchmark worked (SC-09, FR-020).
auto bmLoadBeside(sg::State& state) -> void
{
  for (auto index : state) {
    enterRound(state, index == 0);
    if (index == 0) {
      roundsAtBodyEntry.store(loadRounds.load(std::memory_order_acquire),
                              std::memory_order_release);
      state.setCounter("own", {kCounterValue});
    }
    sg::doNotOptimize(index);
  }
}

// SC-09, acceptance 1: a load context running continuously beside the benchmark
// lets the benchmark complete and print its row.
auto scenarioLoadCompletes() -> int
{
  scriptLoadProvider();
  auto handle = sg::registerBenchmark(&bmLoadBeside, "bmNoiseCompletes");
  handle.threads(kThreadCount);
  startLoad();
  awaitLoadRunning();

  const std::string report =
      captureRun({"--filter", "^bmNoiseCompletes", "--iterations=4"});
  stopLoad();

  const std::string row = lineOf(report, "bmNoiseCompletes");
  check(!row.empty(),
        "the benchmark completes and prints its row beside a running load "
        "(SC-09)");
  check(nameOf(row).ends_with("/threads:4"),
        "the row beside the load keeps its threads segment (SC-09, FR-016)");
  check(
      countFieldOf(row, "iterations=")
          == static_cast<std::int64_t>(kMergedIterations),
      "the row beside the load carries the summed iterations (SC-09, FR-020)");
  check(loadPlanState.load(std::memory_order_acquire) == 1,
        "the load opened its own plan on its own thread (FR-018, PC-1)");
  check(roundsAtBodyEntry.load(std::memory_order_acquire) >= kRoundsBeforeRun,
        "the load stood under way while the benchmark's body ran (SC-09)");
  check(loadRounds.load(std::memory_order_acquire) >= kRoundsBeforeRun,
        "the load kept handing itself work instead of stopping on its own "
        "(SC-09)");
  check(loadOverlap.load(std::memory_order_relaxed) == 0,
        "one context ran one item at a time under the load (FR-013)");
  check(loadForeign.load(std::memory_order_relaxed) == 0,
        "every load item named the context that runs it (FR-002, FR-011)");

  std::puts("harness_threads_test load-completes: ok");
  return 0;
}

// SC-09, acceptance 2 and FR-018: every value of the benchmark's row comes from
// the plan its own context opened. The load folds two leaves scripted to
// distinct steps, so a fold that crossed contexts would print a number the
// scenario can name; the row carries its own plan's numbers and none of those.
auto scenarioOwnPlan() -> int
{
  scriptLoadProvider();
  auto handle = sg::registerBenchmark(&bmLoadBeside, "bmNoiseOwnPlan");
  handle.threads(kThreadCount);
  startLoad();
  awaitLoadRunning();

  const std::string report =
      captureRun({"--filter", "^bmNoiseOwnPlan", "--iterations=4"});
  stopLoad();

  const std::string row = lineOf(report, "bmNoiseOwnPlan");
  check(!row.empty(),
        "the benchmark prints its row beside the running load (SC-09)");
  check(closeTo(fieldOf(row, "real time/iter (ns)="), kMergedRealPerIteration),
        "the real column carries the summed windows of the plan the "
        "benchmark's " "own context opened (FR-018, SC-09)");
  check(closeTo(fieldOf(row, "cpu time/iter (ns)="), kMergedCpuPerIteration),
        "the cpu column carries the summed windows of the plan the benchmark's "
        "own context opened (FR-018, SC-09)");
  check(closeTo(fieldOf(row, "own="), kMergedCounterValue),
        "the counter column carries the four bodies and nothing the load set "
        "(FR-018, FR-019)");
  check(!closeTo(fieldOf(row, "real time/iter (ns)="), kNoiseRealPerIteration),
        "no real column value came from the load context's leaves (FR-018, "
        "SC-09, PC-1)");
  check(!closeTo(fieldOf(row, "cpu time/iter (ns)="), kNoiseCpuPerIteration),
        "no cpu column value came from the load context's leaves (FR-018, "
        "SC-09, PC-1)");
  check(loadWindowBad.load(std::memory_order_relaxed) == 0,
        "every load round folded exactly its own two leaves' steps, so neither "
        "plan read the other's leaves (FR-018, PC-1)");
  check(loadRounds.load(std::memory_order_acquire) >= kRoundsBeforeRun,
        "the load ran its own plan while the benchmark ran its (SC-09)");

  std::puts("harness_threads_test own-plan-values: ok");
  return 0;
}

// FR-010, FR-013: work dispatched from inside a running item runs after that
// item ends, on the same context, and a second dispatch thread reaching the
// same running context proves the rule from the other side. The benchmark runs
// on its own contexts throughout, so the proof stands beside a measurement.
auto scenarioSecondDispatcher() -> int
{
  scriptLoadProvider();
  auto handle = sg::registerBenchmark(&bmLoadBeside, "bmNoiseNested");
  handle.threads(kThreadCount);
  holdFirstRound = 1;
  startLoad();
  std::thread second = dispatchFromSecondThread();
  awaitLoadRunning();

  const std::string report =
      captureRun({"--filter", "^bmNoiseNested", "--iterations=4"});
  second.join();
  const std::uint64_t loadSerial = loadOwner->id().serial;
  stopLoad();

  const std::string row = lineOf(report, "bmNoiseNested");
  check(!row.empty(),
        "the benchmark prints its row while the dispatch proof runs (SC-09)");
  check(nestedNode.state() == sg::WorkNodeState::DONE,
        "the node dispatched from inside the running round ran (FR-010)");
  check(markerNode.state() == sg::WorkNodeState::DONE,
        "the node a second thread dispatched to the running context ran "
        "(FR-010, FR-013)");
  check(markerWaited.load(std::memory_order_acquire) == 1,
        "the dispatch from the second thread returned without waiting, and its "
        "node waited for the running item to end (FR-010, FR-013)");
  check(nestedRecord.entry.load(std::memory_order_acquire)
            > round0Exit.load(std::memory_order_acquire),
        "work dispatched from inside a running item runs after that item ends "
        "(FR-010, FR-013)");
  check(markerRecord.entry.load(std::memory_order_acquire)
            > round0Exit.load(std::memory_order_acquire),
        "work a second thread dispatched to a running context runs after that "
        "item ends (FR-013)");
  check(nestedRecord.serial.load(std::memory_order_acquire) == loadSerial
            && markerRecord.serial.load(std::memory_order_acquire)
                == loadSerial,
        "both nodes ran on the context they were dispatched to (FR-002, "
        "FR-010, " "FR-013)");
  check(nestedRecord.namedContext.load(std::memory_order_acquire) == 1
            && markerRecord.namedContext.load(std::memory_order_acquire) == 1,
        "both nodes named one context by both names (FR-002, FR-011)");
  check(nestedRecord.insideItem.load(std::memory_order_acquire) == 0
            && markerRecord.insideItem.load(std::memory_order_acquire) == 0,
        "no dispatched node ran while another item stood inside (FR-013)");
  check(loadOverlap.load(std::memory_order_relaxed) == 0,
        "one context runs one item at a time (FR-013)");

  std::puts("harness_threads_test second-dispatch-thread: ok");
  return 0;
}

}  // namespace threads_noise

// The three remaining thread setters of the handle: the two ranges, which
// expand the family into one instance per count and name each instance with the
// segment of its count, and the runner, which hands the run's contexts to a
// callable of the user's that dispatches the bodies itself (FR-016, FR-021).
// Each breach below states a count below one, which ends at the call rather
// than in the run (FR-024).
namespace
{

std::atomic<int> runnerCalls {0};
std::atomic<int> runnerContextCount {0};
std::atomic<int> runnerDispatches {0};
std::atomic<int> runnerIndexes[kStateSlots] {};
std::atomic<std::uint64_t> runnerSerials[kStateSlots] {};

auto bmExpansionBody(sg::State& state) -> void
{
  for (auto index : state) {
    enterRound(state, index == 0);
    sg::doNotOptimize(index);
  }
}

// FR-016: one row per count of the set, the instance name carrying the
// `/threads:N` segment only above one, and every row summing the leaves of the
// plan its own contexts opened, so the per-iteration columns stand at the step
// of one leaf whatever the count.
auto checkExpansion(const std::string& report,
                    const std::string& family,
                    const std::vector<int>& counts) -> void
{
  std::vector<std::string> rows;
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.starts_with(family)) {
      rows.push_back(line);
    }
  }
  check(rows.size() == counts.size(),
        "the range call expands one instance per count (FR-016)");

  for (const int count : counts) {
    const std::string name =
        count > 1 ? family + "/threads:" + std::to_string(count) : family;
    std::string row;
    for (const std::string& candidate : rows) {
      if (nameOf(candidate) == name) {
        row = candidate;
      }
    }
    check(!row.empty(),
          "the instance of each count of the range prints its row (FR-016)");
    check(nameOf(row) == name,
          "the instance name carries the segment of its count, and no segment "
          "at one context (FR-016)");
    check(countFieldOf(row, "iterations=")
              == static_cast<std::int64_t>(count)
                     * static_cast<std::int64_t>(kIterations),
          "the instance runs on that many contexts, so the iterations sum over "
          "them (FR-016, FR-020)");
    check(closeTo(fieldOf(row, "real time/iter (ns)="), kMergedRealPerIteration),
          "the real column of each instance stands at the step of its own "
          "leaves, so the same body on more contexts folds no multiple of it "
          "(FR-018, FR-020)");
    check(closeTo(fieldOf(row, "cpu time/iter (ns)="), kMergedCpuPerIteration),
          "the cpu column of each instance stands at the step of its own "
          "leaves, so the same body on more contexts folds no multiple of it "
          "(FR-018, FR-020)");
  }
}

// SC-06, FR-016: `threadRange` states one count per doubling between the
// bounds, inclusive of the last doubling that fits.
auto scenarioRangeExpansion() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmExpansionBody, "bmThreadsRange");
  handle.threadRange(1, 8);

  const std::string report =
      captureRun({"--filter", "^bmThreadsRange", "--iterations=4"});
  checkExpansion(report, "bmThreadsRange", {1, 2, 4, 8});

  std::puts("harness_threads_test range-expansion: ok");
  return 0;
}

// SC-06, FR-016: `denseThreadRange` states one count per step between the
// bounds, inclusive of the last step that fits.
auto scenarioDenseExpansion() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmExpansionBody, "bmThreadsDense");
  handle.denseThreadRange(1, 7, 3);

  const std::string report =
      captureRun({"--filter", "^bmThreadsDense", "--iterations=4"});
  checkExpansion(report, "bmThreadsDense", {1, 4, 7});

  std::puts("harness_threads_test dense-range-expansion: ok");
  return 0;
}

// The body of the runner scenario: the one counter name the merged row sums.
auto bmRunnerBody(sg::State& state) -> void
{
  for (auto index : state) {
    enterRound(state, index == 0);
    if (index == 0) {
      state.setCounter("own", {kCounterValue});
    }
    sg::doNotOptimize(index);
  }
}

// FR-021: the callable receives the run's view, states the count, names the
// context of each index and dispatches each body index once. The tally below is
// what the harness requires of the call before it returns.
auto dispatchEveryIndex(sg::ThreadRun& run) -> void
{
  runnerCalls.fetch_add(1, std::memory_order_release);
  runnerContextCount.store(run.contextCount(), std::memory_order_release);
  for (int index = 0; index != run.contextCount(); ++index) {
    if (index >= 0 && index < kStateSlots) {
      runnerSerials[index].store(run.context(index).id().serial,
                                 std::memory_order_relaxed);
    }
    run.dispatch(index);
    if (index >= 0 && index < kStateSlots) {
      runnerIndexes[index].fetch_add(1, std::memory_order_release);
    }
    runnerDispatches.fetch_add(1, std::memory_order_release);
  }
}

// SC-08, FR-021: the user callable performs the dispatches the harness built,
// and the merged row of the run stands as the harness merged it.
auto scenarioCustomRunner() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmRunnerBody, "bmThreadsRunner");
  handle.threads(kThreadCount).threadRunner(&dispatchEveryIndex);

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsRunner", "--iterations=4"}),
             "bmThreadsRunner");
  check(!row.empty(),
        "a run over the user's runner publishes its row (FR-021)");
  check(runnerCalls.load(std::memory_order_acquire) > 0,
        "the harness hands the run to the attached callable (FR-021)");
  check(runnerContextCount.load(std::memory_order_acquire) == kThreadCount,
        "the view states the context count of the instance (FR-021)");
  for (int index = 0; index != kThreadCount; ++index) {
    check(runnerIndexes[index].load(std::memory_order_acquire) > 0,
          "every body index of the run stands dispatched by the callable "
          "(FR-021)");
  }
  for (int index = kThreadCount; index != kStateSlots; ++index) {
    check(runnerIndexes[index].load(std::memory_order_acquire) == 0,
          "no index beyond the count stands dispatched (FR-021)");
  }
  check(runnerDispatches.load(std::memory_order_acquire)
            == kThreadCount
                   * runnerCalls.load(std::memory_order_acquire),
        "one dispatch per body index per run, so no index stands undispatched "
        "or dispatched twice (FR-021)");
  for (int first = 0; first != kThreadCount; ++first) {
    check(runnerSerials[first].load(std::memory_order_relaxed) != 0,
          "the view names a running context for each index (FR-021)");
    for (int second = first + 1; second != kThreadCount; ++second) {
      check(runnerSerials[first].load(std::memory_order_relaxed)
                != runnerSerials[second].load(std::memory_order_relaxed),
            "each index names its own context (FR-021)");
    }
  }

  check(countFieldOf(row, "iterations=")
            == static_cast<std::int64_t>(kMergedIterations),
        "the iterations of the run sum over the contexts the callable "
        "dispatched (FR-020, FR-021)");
  check(closeTo(fieldOf(row, "own="), kMergedCounterValue),
        "the counter names sum over the contexts before the flag pass of a run "
        "over the user's runner (FR-020, FR-021)");
  check(closeTo(fieldOf(row, "real time/iter (ns)="), kMergedRealPerIteration),
        "the merge of the run over the user's runner stays the harness's "
        "(FR-020, FR-021)");
  check(closeTo(fieldOf(row, "cpu time/iter (ns)="), kMergedCpuPerIteration),
        "the cpu column of the run over the user's runner stays the harness's "
        "(FR-020, FR-021)");

  std::puts("harness_threads_test custom-runner: ok");
  return 0;
}

// FR-024: the child calls one setter with a count below one and never reaches
// its run. The contract facility writes the violation to stderr and aborts, so
// the parent reads the signal and the tag of the message; the words of the
// message belong to the setter, so the tag of the kind is what stands pinned.
auto runBreach(const std::string& self,
               const char* const mode,
               const char* const expected) -> void
{
  const char* const path = "harness_threads_breach_out.txt";
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the breached child");
  }
  if (pid == 0) {
    if (std::freopen(path, "w", stderr) == nullptr) {
      _exit(2);
    }
    char* const execArgv[] = {
        const_cast<char*>(self.c_str()), const_cast<char*>(mode), nullptr};
    (void)execv(self.c_str(), execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the breached child");
  }
  check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
        "the breached count ends the child at the call (FR-024)");

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string message = buffer.str();
  if (!message.contains(expected)) {
    std::fprintf(stderr, "no '%s' in '%s'\n", expected, message.c_str());
    fail("the breach states no precondition violation (FR-024)");
  }
}

// The count of zero, which no call can honor (E-02, FR-024).
auto breachThreadsZero() -> int
{
  auto handle = sg::registerBenchmark(&bmExpansionBody, "bmThreadsBreach");
  (void)handle.threads(0);
  return 0;
}

auto breachRangeZero() -> int
{
  auto handle = sg::registerBenchmark(&bmExpansionBody, "bmThreadsBreach");
  (void)handle.threadRange(0, 4);
  return 0;
}

auto breachDenseZero() -> int
{
  auto handle = sg::registerBenchmark(&bmExpansionBody, "bmThreadsBreach");
  (void)handle.denseThreadRange(0, 4, 1);
  return 0;
}

}  // namespace

// FR-005, D-04: the shipped placement function, with no install standing in
// this process, pins the thread of the context to the cpu its slot names. The
// item reads the affinity mask of the thread it runs on, which is the platform
// state the pin writes; a replaced placement function would leave the mask as
// the process found it, so the count below is the pin and nothing else.
struct AffinityItem
{
  int* cpuCount = nullptr;
  int* firstCpu = nullptr;

  auto operator()(sg::WorkContext& /*work*/) noexcept -> void
  {
    cpu_set_t set {};
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) != 0) {
      *cpuCount = -1;
      return;
    }
    *cpuCount = CPU_COUNT(&set);
    for (std::size_t cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
      if (CPU_ISSET(cpu, &set)) {
        *firstCpu = static_cast<int>(cpu);
        return;
      }
    }
  }
};

auto scenarioDefaultPin() -> int
{
  constexpr std::uint32_t kPinTarget = 0;

  int cpuCount = 0;
  int firstCpu = -1;
  const sg::ContextPolicy policy {
      .waitMode = sg::WaitMode::YIELD,
      .placement = sg::PlacementSlot::cpu(kPinTarget)};
  sg::ExecutionContext context {policy};

  const AffinityItem item {&cpuCount, &firstCpu};
  sg::WorkNode<AffinityItem> node {item};
  sg::dispatch(context, node);
  context.drain();

  check(node.state() == sg::WorkNodeState::DONE,
        "the affinity probe ran on the context (FR-009)");
  check(context.policy().placement.cpuIndex == kPinTarget,
        "the policy of the context carries the slot it was made with (FR-007)");
  check(cpuCount == 1 && firstCpu == static_cast<int>(kPinTarget),
        "the shipped placement function pinned the context thread to the cpu "
        "its slot names (FR-005, D-04)");

  context.requestStop();
  context.drain();
  context.join();
  std::puts("harness_threads_test default-pin: ok");
  return 0;
}

// FR-023, D-07 at the thread count of four: a body that throws leaves its
// measured window, still arrives at the stop barrier, and the attempt answers
// with the body's message rather than waiting for an arrival that never comes.
auto bmThreadedThrow(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
    throw std::runtime_error("the scripted threaded throw");
  }
}

auto scenarioBodyThrows() -> int
{
  auto handle = sg::registerBenchmark(&bmThreadedThrow, "bmThreadsThrow");
  handle.threads(kThreadCount);

  int status = 0;
  const std::string report =
      captureRun({"--filter", "^bmThreadsThrow", "--iterations=1"}, status);

  check(status != 0,
        "a threaded attempt whose bodies throw ends with a failure status "
        "(FR-023)");
  check(report.contains("the scripted threaded throw"),
        "the message of a throwing body reaches the report of the threaded "
        "attempt (FR-023)");
  std::puts("harness_threads_test body-throws: ok");
  return 0;
}

std::atomic<int> pairSetUps {0};
std::atomic<int> pairTearDowns {0};

// D-07, FR-023: the fixture pair of a threaded attempt belongs to the attempt,
// not to its contexts, so one setUp and one tearDown stand for four contexts.
class PairFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override
  {
    (void)state;
    ++pairSetUps;
  }

  auto tearDown(sg::State& state) -> void override
  {
    (void)state;
    ++pairTearDowns;
  }
};

auto scenarioFixturePair() -> int
{
  auto handle = sg::registerFixtureBenchmark(
      [](sg::State& state)
      {
        for (auto index : state) {
          sg::doNotOptimize(index);
        }
      },
      "bmThreadsFixture",
      [] { return std::make_unique<PairFixture>(); });
  handle.threads(kThreadCount);

  const std::string report =
      captureRun({"--filter", "^bmThreadsFixture", "--iterations=2"});

  check(!lineOf(report, "bmThreadsFixture").empty(),
        "the threaded fixture instance prints its merged row (FR-016)");
  check(pairSetUps.load() == 1,
        "the setUp of the fixture runs once for the attempt and not once per "
        "context (D-07, FR-023)");
  check(pairTearDowns.load() == 1,
        "the tearDown of the fixture runs once for the attempt, on the context "
        "that closes it (D-07, FR-023)");
  std::puts("harness_threads_test fixture-pair: ok");
  return 0;
}

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (!mode.empty()) {
    sg::test::flushCountersOnAbort();
  }
  if (mode == "index-and-name") {
    return scenarioIndexAndName();
  }
  if (mode == "merged-row") {
    return scenarioMergedRow();
  }
  if (mode == "merged-metric") {
    return scenarioMergedMetric();
  }
  if (mode == "paused-contexts") {
    return scenarioPausedContexts();
  }
  if (mode == "default-pin") {
    return scenarioDefaultPin();
  }
  if (mode == "body-throws") {
    return scenarioBodyThrows();
  }
  if (mode == "fixture-pair") {
    return scenarioFixturePair();
  }
  if (mode == "cpu-decision") {
    return scenarioCpuDecision();
  }
  if (mode == "real-decision") {
    return scenarioRealDecision();
  }
  if (mode == "manual-decision") {
    return scenarioManualDecision();
  }
  if (mode == "load-completes") {
    return threads_noise::scenarioLoadCompletes();
  }
  if (mode == "own-plan-values") {
    return threads_noise::scenarioOwnPlan();
  }
  if (mode == "second-dispatch-thread") {
    return threads_noise::scenarioSecondDispatcher();
  }
  if (mode == "range-expansion") {
    return scenarioRangeExpansion();
  }
  if (mode == "dense-range-expansion") {
    return scenarioDenseExpansion();
  }
  if (mode == "custom-runner") {
    return scenarioCustomRunner();
  }
  if (mode == "breach-threads-zero") {
    return breachThreadsZero();
  }
  if (mode == "breach-thread-range-zero") {
    return breachRangeZero();
  }
  if (mode == "breach-dense-range-zero") {
    return breachDenseZero();
  }

  // Each scenario owns one provider script and one family, so each runs in a
  // re-exec of this binary; a failed check exits there and shows here.
  const std::string self = argc > 0 ? argv[0] : "harness_threads_test";
  for (const auto& scenario : {
           "index-and-name",
           "merged-row",
           "merged-metric",
           "paused-contexts",
           "default-pin",
           "body-throws",
           "fixture-pair",
           "cpu-decision",
           "real-decision",
           "manual-decision",
           "load-completes",
           "own-plan-values",
           "second-dispatch-thread",
           "range-expansion",
           "dense-range-expansion",
           "custom-runner",
       })
  {
    const int status =
        std::system(("\"" + self + "\" " + scenario + " 2>/dev/null").c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes");
  }

  for (const auto& breach : {"breach-threads-zero",
                             "breach-thread-range-zero",
                             "breach-dense-range-zero"})
  {
    runBreach(self, breach, "[precondition]");
  }

  std::puts("harness_threads_test: ok");
  return 0;
}
