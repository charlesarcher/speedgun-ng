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
#include "detail/internal.hpp"
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

auto scriptProvider(const std::uint64_t cpuStep = kThreadCpuStep) -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addCounter("fake", "work", "ops", "scripted work counter");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, cpuStep);
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

// The body of the repetition scenario: the iteration count of the row is all
// that scenario reads, so the body opens no counter and joins no round.
auto bmRepetitionRow(sg::State& state) -> void
{
  for (auto index : state) {
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

// The repetition rows of one instance: a repetition row is the only row of
// the report that names the iteration count of its run.
auto rowsOf(const std::string& report,
            const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> rows;
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.starts_with(name) && line.contains("iterations=")) {
      rows.push_back(line);
    }
  }
  return rows;
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

// FR-012, FR-020: the count the first repetition settled is the count every
// later repetition hands each context, so the merged rows of one run all sum
// the same iteration count.
auto scenarioRepetitionCount() -> int
{
  constexpr std::size_t kRepetitions = 3;
  // The target stands under the decision time one iteration of four contexts
  // folds, so calibration settles at its first run (FR-016). Raising it makes
  // the walk take a second sample, and the arrival gate below counts entries
  // across samples, so a second sample waits for arrivals that already stood.
  constexpr std::int64_t kCalibrationNs = 1000;

  scriptProvider();
  auto handle = sg::registerBenchmark(&bmRepetitionRow, "bmThreadsRepeat");
  handle.threads(kThreadCount)
      .repetitions(kRepetitions)
      .minTime(kCalibrationNs);

  const std::vector<std::string> rows =
      rowsOf(captureRun({"--filter", "^bmThreadsRepeat"}), "bmThreadsRepeat");
  check(rows.size() == kRepetitions,
        "every repetition of the threaded instance prints its merged row "
        "(FR-012)");
  const std::int64_t summed = countFieldOf(rows.front(), "iterations=");
  check(summed > 0, "a calibrated repetition counts its iterations (FR-016)");
  for (const std::string& row : rows) {
    check(countFieldOf(row, "iterations=") == summed,
          "every repetition sums the iteration count the first repetition "
          "settled (FR-012, FR-020)");
  }

  std::puts("harness_threads_test repetition-count: ok");
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

// FR-020, FR-022, D-06: a metric whose dimension names neither time^1 nor
// events^1 keeps its window value: the rate each context measured sums over the
// contexts and never divides by the summed iteration count, where the
// per-iteration column above divides back to the same rate.
auto scenarioMergedRateMetric() -> int
{
  scriptProvider();
  auto attach = [](sg::BenchmarkHandle& handle)
  {
    const auto object = sg::counters::System::local().object("fake");
    check(object.has_value(), "the scripted work object resolves");
    const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
    check(work.has_value(), "the scripted work leaf resolves");
    const auto machine = sg::counters::System::local().object("machine");
    check(machine.has_value(), "the scripted machine object resolves");
    const auto seconds = machine->counter<sg::counters::Dim<1, 0>>("monotonic");
    check(seconds.has_value(), "the scripted monotonic leaf resolves");
    handle.addMetric(*work / *seconds, "workrate");
  };

  auto threaded = sg::registerBenchmark(&bmMergedRow, "bmThreadsRateMetric");
  attach(threaded);
  threaded.threads(kThreadCount);
  auto alone = sg::registerBenchmark(&bmMergedRow, "bmSingleRateMetric");
  attach(alone);

  const std::string report =
      captureRun({"--filter",
                  "^bmThreadsRateMetric|^bmSingleRateMetric",
                  "--iterations=4"});
  const std::string merged = lineOf(report, "bmThreadsRateMetric");
  const std::string single = lineOf(report, "bmSingleRateMetric");
  check(!merged.empty() && !single.empty(),
        "both rate instances print their row (FR-016)");
  const double singleRate = fieldOf(single, "workrate=");
  check(singleRate > 0.0 && fieldOf(merged, "workrate=") > 0.0,
        "the merged row carries the attached rate column (FR-020)");
  check(closeTo(fieldOf(merged, "workrate="),
                static_cast<double>(kThreadCount) * singleRate),
        "the merged rate sums the window rate of each context and stands "
        "undivided by the iteration count (FR-020, FR-022, D-06)");

  std::puts("harness_threads_test merged-rate-metric: ok");
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

// ---------------------------------------------------------------------------
// The availability, callback, failure and interrupt paths of the threaded
// runner: what a context folds when a column carries no fold, what the row
// discloses when the decision is not countable or not a divisor, what one
// skipped context does to the merged row, and what ends an interrupted run.

// The scripted provider whose thread-cpu leaf measures nothing on every
// sampling action: every window's fold over it, the measuring ones first,
// carries a gap.
auto gappedCpuProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addCounter("fake", "work", "ops", "scripted work counter");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  owned->setPoints("fake", "work", {0}, kWorkStep);
  owned->setGapActions(
      "machine",
      "thread_cpu",
      {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
       17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
       33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
       49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64});
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// The scripted provider whose work leaf carries a gap on the first three
// sampling actions of the run. A sampling action stands for every leaf it
// reads, so the folds of the contexts that sample first carry a gap while the
// rest stand countable.
auto gappedWorkProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addCounter("fake", "work", "ops", "scripted work counter");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  owned->setPoints("fake", "work", {0}, kWorkStep);
  owned->setGapActions("fake", "work", {1, 2, 3});
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the gapped work provider registers");
}

auto bmCounterLoop(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("own", {kCounterValue});
    }
    sg::doNotOptimize(index);
  }
}

// One context of the run claims the skip; the others run the whole loop.
std::atomic<bool> skipClaimed {false};

auto bmOneContextSkips(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      bool expected = false;
      if (skipClaimed.compare_exchange_strong(expected, true)) {
        state.skipWithError("the scripted threaded skip");
      }
    }
    sg::doNotOptimize(index);
  }
}

std::atomic<int> callbackSetups {0};
std::atomic<int> callbackTearDowns {0};

// The completed loop followed by an exception the catch-all of the body
// stands after: the int is not a std::exception.
auto bmThrowsIntAfterLoop(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
  throw 7;
}

// The second entry of the timed loop that FR-017 refuses.
auto bmEntersLoopTwice(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
  for (auto index : state) {
    sg::doNotOptimize(index);
    break;
  }
}

// Context 0 raises the signal the harness stands guard over, so a run that
// owes more repetitions ends at the interrupt check between them.
auto bmRaisesOnFirstIteration(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0 && state.threadIndex() == 0) {
      std::raise(SIGINT);
    }
    sg::doNotOptimize(index);
  }
}

// FR-020, FR-024: a metric column whose fold carries a gap and a column of
// a leaf no provider names each disclose what they carry, while the row
// still prints beside them.
auto scenarioUnavailableFold() -> int
{
  scriptProvider();
  const auto object = sg::counters::System::local().object("fake");
  check(object.has_value(), "the scripted object resolves");
  const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
  check(work.has_value(), "the scripted work leaf resolves");

  auto threaded = sg::registerBenchmark(&bmRepetitionRow, "bmThreadsStale");
  threaded.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*work),
                     "work");
  threaded.threads(2);

  const std::string report = captureRun({"--filter",
                                         "^bmThreadsStale",
                                         "--iterations=4",
                                         "--counter=fake/work",
                                         "--counter=machine/nosuch"});
  const std::string merged = lineOf(report, "bmThreadsStale");
  check(!merged.empty(),
        "the threaded row prints beside gapped and absent columns (FR-016)");
  check(merged.contains("work="),
        "the attached metric keeps its column when its fold carries a gap "
        "(FR-020, FR-022)");
  check(closeTo(fieldOf(merged, "machine/nosuch="), 0.0),
        "the column of the leaf no provider names carries no fold (FR-020, "
        "FR-024)");
  check(merged.contains("refusal=absent"),
        "the row names the refusal of the column that carries no fold "
        "(FR-024)");
  std::puts("harness_threads_test unavailable-fold: ok");
  return 0;
}

// FR-020: a decision whose folds carry gaps discloses the fold beside the
// row's counters. The gap never reaches the divisor.
auto scenarioFoldDisclosed() -> int
{
  gappedCpuProvider();
  auto handle = sg::registerBenchmark(&bmCounterLoop, "bmThreadsFoldNote");
  handle.threads(2);

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsFoldNote", "--iterations=4"}),
             "bmThreadsFoldNote");
  check(!row.empty(),
        "the threaded row prints beside a gapped " "decision (FR-016)");
  check(row.contains("fold-disclosed"),
        "the row discloses the fold when the decision folds a gap "
        "(FR-020, FR-024)");
  check(!row.contains("zero-divisor"),
        "the zero-divisor note stands aside while a decision second "
        "stands (FR-020)");
  std::puts("harness_threads_test fold-disclosed: ok");
  return 0;
}

// FR-020: a decision of zero measured seconds names the missing divisor.
// The division never happens. The scripted thread-cpu leaf never moves, so
// the CPU fold that stands as the primary of every context reads zero: the
// same seam the single-context row already discloses.
auto scenarioZeroDivisor() -> int
{
  scriptProvider(0);
  auto handle = sg::registerBenchmark(&bmCounterLoop, "bmThreadsZeroNote");
  handle.threads(2);

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsZeroNote", "--iterations=4"}),
             "bmThreadsZeroNote");
  check(!row.empty(),
        "the threaded row prints beside a zero decision " "(FR-016)");
  check(row.contains("zero-divisor"),
        "the row names the zero divisor the scripted leaf leaves (FR-020)");
  check(!row.contains("fold-disclosed"),
        "the fold note stands aside while the decision stays countable "
        "(FR-020)");
  std::puts("harness_threads_test zero-divisor: ok");
  return 0;
}

// FR-020, FR-022, FR-024: the merged metric column takes the availability of
// the worst fold among the contexts. The countable start of the merge stands
// only while every context folded a countable column: the gap one context
// folded stands on the merged column of the row.
auto scenarioWidenedMetric() -> int
{
  gappedWorkProvider();
  const auto object = sg::counters::System::local().object("fake");
  check(object.has_value(), "the gapped work object resolves");
  const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
  check(work.has_value(), "the gapped work leaf resolves");

  auto threaded = sg::registerBenchmark(&bmMergedRow, "bmThreadsWidened");
  threaded.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*work),
                     "gapped");
  threaded.threads(kThreadCount);

  const std::string merged =
      lineOf(captureRun({"--filter", "^bmThreadsWidened", "--iterations=4"}),
             "bmThreadsWidened");
  check(!merged.empty(),
        "the threaded row prints beside the gapped fold (FR-016)");
  check(merged.contains("gapped="),
        "the merged column of the gapped leaf keeps its column (FR-020, "
        "FR-022)");
  check(countFieldOf(merged, "gap=") == 1,
        "the merged column carries the gap its worst context folded (FR-020, "
        "FR-024)");
  std::puts("harness_threads_test widened-metric: ok");
  return 0;
}

auto scenarioSetupTeardown() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmRepetitionRow, "bmThreadsCallbacks");
  handle.threads(2)
      .setup(
          [](sg::State& state)
          {
            callbackSetups.fetch_add(1, std::memory_order_relaxed);
            check(state.threadIndex() >= 0,
                  "the setup callback receives the context that owns the half");
          })
      .teardown(
          [](sg::State& state)
          {
            callbackTearDowns.fetch_add(1, std::memory_order_relaxed);
            (void)state;
          });

  const std::string row =
      lineOf(captureRun({"--filter", "^bmThreadsCallbacks", "--iterations=4"}),
             "bmThreadsCallbacks");
  check(!row.empty(), "the threaded fixture family prints its row (FR-016)");
  check(callbackSetups.load() == 1,
        "the setup callback runs once for the attempt and not once per "
        "context (FR-023, D-07)");
  check(callbackTearDowns.load() == 1,
        "the teardown callback runs once for the attempt (FR-023, D-07)");
  std::puts("harness_threads_test setup-teardown: ok");
  return 0;
}

// The fixture whose setUp throws an int: the catch-all of the setup half
// names the failure the report shows.
class IntThrowFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override
  {
    (void)state;
    throw 42;
  }

  auto tearDown(sg::State& state) -> void override { (void)state; }
};

std::atomic<int> intThrowBodies {0};

// FR-023: a setup half that throws what is not an exception leaves every
// context out of the body and fails the benchmark.
auto scenarioSetupThrowsInt() -> int
{
  scriptProvider();
  auto handle = sg::registerFixtureBenchmark(
      [](sg::State& state)
      {
        intThrowBodies.fetch_add(1, std::memory_order_relaxed);
        for (auto index : state) {
          sg::doNotOptimize(index);
        }
      },
      "bmThreadsSetupInt",
      [] { return std::make_unique<IntThrowFixture>(); });
  handle.threads(2);

  int status = 0;
  const std::string report =
      captureRun({"--filter", "^bmThreadsSetupInt", "--iterations=2"}, status);
  check(status != 0, "the failed setup ends with the failure status (FR-023)");
  check(report.contains("the benchmark threw an unknown exception"),
        "the catch-all of the setup half names the int the scripted fixture "
        "threw (FR-023)");
  check(
      intThrowBodies.load() == 0,
      "no context enters the body when the setup half throws (FR-023, " "D-"
                                                                        "07)");
  std::puts("harness_threads_test setup-throws-int: ok");
  return 0;
}

// The fixture whose teardown half throws an int: the catch-all of the
// teardown half names the failure the report shows.
class TearThrowFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override { (void)state; }

  auto tearDown(sg::State& state) -> void override
  {
    (void)state;
    throw 7;
  }
};

std::atomic<int> tearThrowBodies {0};

// FR-023, D-07: a teardown half that throws after the stop barrier fails the
// run and leaves no measured row.
auto scenarioTearDownThrowsInt() -> int
{
  scriptProvider();
  tearThrowBodies.store(0, std::memory_order_relaxed);
  auto handle = sg::registerFixtureBenchmark(
      [](sg::State& state)
      {
        tearThrowBodies.fetch_add(1, std::memory_order_relaxed);
        for (auto index : state) {
          sg::doNotOptimize(index);
        }
      },
      "bmThreadsTearInt",
      [] { return std::make_unique<TearThrowFixture>(); });
  handle.threads(2);

  int status = 0;
  const std::string report =
      captureRun({"--filter", "^bmThreadsTearInt", "--iterations=2"}, status);
  check(status != 0,
        "the failed teardown ends with the failure status (FR-023)");
  check(report.contains("the benchmark threw an unknown exception"),
        "the catch-all of the teardown half names the int the scripted "
        "fixture threw (FR-023)");
  check(tearThrowBodies.load() == 2,
        "both contexts ran the body while the setup half stood silent (FR-023, "
        "D-07)");
  check(rowsOf(report, "bmThreadsTearInt").empty(),
        "a run whose teardown half threw prints no measured row (FR-023)");
  std::puts("harness_threads_test teardown-throws-int: ok");
  return 0;
}

// FR-005: one context that skips with a reason ends the run as skipped with
// that reason; no measured row stands.
auto scenarioSkipOne() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmOneContextSkips, "bmThreadsSkipOne");
  handle.threads(2);

  const std::string report =
      captureRun({"--filter", "^bmThreadsSkipOne", "--iterations=4"});
  check(report.contains("SKIPPED: the scripted threaded skip"),
        "the merged run is skipped with the reason the one context gave "
        "(FR-005, FR-016)");
  check(rowsOf(report, "bmThreadsSkipOne").empty(),
        "a skipped run prints no measured row (FR-016)");
  std::puts("harness_threads_test skip-one: ok");
  return 0;
}

std::atomic<int> twiceSkipClaims {0};

// Both contexts of the run claim the skip, each with the same reason, so the
// merge sees a skipped second context beside the first (FR-005, FR-016).
auto bmTwoContextsSkip(sg::State& state) -> void
{
  for (auto index : state) {
    if (twiceSkipClaims.fetch_add(1, std::memory_order_relaxed) < 2) {
      state.skipWithError("the scripted threaded skip twice");
    }
    sg::doNotOptimize(index);
  }
}

// FR-005, FR-016: a second context that skips with its own reason leaves the
// run skipped once, with the reason the merge took first; no measured row
// stands either way.
auto scenarioSkipTwo() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmTwoContextsSkip, "bmThreadsSkipTwo");
  handle.threads(2);

  const std::string report =
      captureRun({"--filter", "^bmThreadsSkipTwo", "--iterations=4"});
  check(report.contains("SKIPPED: the scripted threaded skip twice"),
        "the merged run is skipped once with the reason the contexts gave "
        "(FR-005, FR-016)");
  check(rowsOf(report, "bmThreadsSkipTwo").empty(),
        "a run every context skipped prints no measured row (FR-016)");
  check(!report.contains("SKIPPED: the scripted threaded skip twice  SKIPPED")
            && !report.contains("skip twice\" skipped"),
        "the skip of the second context adds no second note beside the run's "
        "(FR-005, FR-016)");
  std::puts("harness_threads_test skip-two: ok");
  return 0;
}

// FR-052, FR-054: the room learned from the first attempt lets the second
// run its pairs raw; only the folded row carries the note.
auto scenarioPauseOverflow() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPausedRow, "bmThreadsPauseTwo");
  handle.threads(2);

  const std::string report = captureRun(
      {"--filter", "^bmThreadsPauseTwo", "--iterations=4", "--repetitions=2"});
  const std::vector<std::string> rows = rowsOf(report, "bmThreadsPauseTwo");
  check(rows.size() == 2,
        "the run prints one row per repetition (FR-016, FR-054)");
  if (rows.size() == 2) {
    check(rows[0].contains("pause=folded"),
          "the first attempt folds its pairs through the slot because no "
          "room was learned yet (FR-052, FR-054)");
    check(!rows[1].contains("pause=folded"),
          "the second attempt runs its pairs raw inside the room it learned "
          "(FR-052, FR-054)");
  }
  std::puts("harness_threads_test pause-overflow: ok");
  return 0;
}

// FR-016: a body that throws what is not an exception fails the benchmark
// through the catch-all of the body, with no measured row.
auto scenarioBodyThrowsInt() -> int
{
  scriptProvider();
  auto handle =
      sg::registerBenchmark(&bmThrowsIntAfterLoop, "bmThreadsThrowInt");
  handle.threads(2);

  int status = 0;
  const std::string report =
      captureRun({"--filter", "^bmThreadsThrowInt", "--iterations=2"}, status);
  check(status != 0,
        "the body that throws ends with the failure status " "(FR-016)");
  check(report.contains("the benchmark threw an unknown exception"),
        "the catch-all of the body names the int it threw (FR-016)");
  check(rowsOf(report, "bmThreadsThrowInt").empty(),
        "a failed run prints no measured row (FR-016)");
  std::puts("harness_threads_test body-throws-int: ok");
  return 0;
}

// FR-017: the second entry of the timed loop fails the record, so the
// threaded run shows the rule as its failure.
auto scenarioDoubleLoop() -> int
{
  scriptProvider();
  auto handle =
      sg::registerBenchmark(&bmEntersLoopTwice, "bmThreadsDoubleLoop");
  handle.threads(2);

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmThreadsDoubleLoop", "--iterations=2"}, status);
  check(
      status != 0,
      "the second entry of the loop ends with the failure " "status (FR-017)");
  check(
      report.contains("the benchmark entered the timed loop twice " "(FR-017)"),
      "the failed record names the rule the body broke (FR-017)");
  std::puts("harness_threads_test double-loop: ok");
  return 0;
}

// FR-008, FR-016: the signal between two repetitions ends the run before
// the repetition it owes.
auto scenarioInterruptReps() -> int
{
  scriptProvider();
  auto handle =
      sg::registerBenchmark(&bmRaisesOnFirstIteration, "bmThreadsInterrupt");
  handle.threads(2);

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmThreadsInterrupt", "--iterations=2", "--repetitions=3"},
      status);
  check(status != 0,
        "the interrupted run ends with the failure status " "(FR-008)");
  check(report.contains("interrupted by the user (SIGINT)"),
        "the interrupted run discloses the signal as its reason (FR-008)");
  check(rowsOf(report, "bmThreadsInterrupt").empty(),
        "the interrupted run prints no measured row (FR-016)");
  std::puts("harness_threads_test interrupt-reps: ok");
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
// running with its plan opened (SC-09).
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
// with no race between them (FR-018, PC-1).
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
    // item to end; an overtaking one reads done: FR-010's dispatch returns
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

  // FR-024: bounds that stand out of order, and a lower bound below one,
  // name no count. The require of the call answers either mistake where
  // contracts stand, so the account both answers share is read here, at the
  // builder that holds it.
  check(sg::detail::buildThreadCounts(4, 2).empty(),
        "bounds that stand out of order name no thread count (FR-024)");
  check(sg::detail::buildThreadCounts(0, 4).empty(),
        "a lower bound below one names no thread count (FR-024)");
  check(sg::detail::buildThreadCounts(5, 8) == std::vector<int> {5},
        "only the counts that fit the bounds stand in the set (FR-016)");

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

  // FR-024: a step below one repeats one count forever, so it names the
  // empty set, as inverted bounds do (the reading of P-6).
  check(sg::detail::buildDenseThreadCounts(1, 10, 0).empty(),
        "a step below one names no thread count (FR-024)");
  check(sg::detail::buildDenseThreadCounts(6, 3, 1).empty(),
        "bounds that stand out of order name no thread count (FR-024)");
  check(sg::detail::buildDenseThreadCounts(2, 10, 3)
            == std::vector<int> {2, 5, 8},
        "one count per step that fits stands in the set (FR-016)");

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

// The calls the replacement placement function below took, and how many of
// them received a slot other than the one the threaded run carries.
std::atomic<int> placementCalls {0};
std::atomic<int> placementSlotMiss {0};

// A placement function that lands no slot and answers with a word of its own,
// so the record it returns says what the function did (FR-005, D-04).
auto decliningPlacement(const sg::PlacementSlot& slot,
                        std::thread::native_handle_type /*thread*/)
    -> sg::PlacementResult
{
  ++placementCalls;
  if (slot.cpuIndex != sg::PlacementSlot::noPin().cpuIndex) {
    ++placementSlotMiss;
  }
  return {.applied = false, .description = "DECLINED"};
}

// FR-005, FR-025, contracts/report-disclosure.md: each context's placement
// record reaches the report through that context's own disclosure field. The
// threaded run's contexts carry no pin, so a field built from the policy reads
// NO_PIN; DECLINED is a word only the installed function can produce, so the
// line below carries what the function returned at each context's start. The
// install stands before this process's first context, and every scenario of
// this binary runs in a re-exec of its own, so no other scenario sees it.
auto scenarioPlacementDisclosure() -> int
{
  constexpr int kPlacementContexts = 2;

  sg::setPlacementFunction(&decliningPlacement);
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmRepetitionRow, "bmThreadsPlacement");
  handle.threads(kPlacementContexts);

  const std::string report =
      captureRun({"--filter", "^bmThreadsPlacement", "--iterations=4"});

  check(lineOf(report, "  contexts=")
            == "  contexts=2  ctx0=SPIN/DECLINED/unapplied"
               "  ctx1=SPIN/DECLINED/unapplied",
        "the disclosure line names the description each context's placement "
        "function returned and the flag that says it did not land "
        "(FR-005, FR-025)");
  check(placementCalls.load() == kPlacementContexts,
        "the run placed each of its two contexts through the installed "
        "function (FR-005, D-04)");
  check(placementSlotMiss.load() == 0,
        "the installed function received the placement slot of the threaded "
        "policy on every context (FR-007)");

  std::puts("harness_threads_test placement-disclosure: ok");
  return 0;
}

// FR-023, D-07 at the thread count of four: a body that throws leaves its
// measured window, still arrives at the stop barrier, and the attempt answers
// with the body's message; the unmet arrival is never waited for.
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

std::atomic<int> failedPairBodies {0};

// D-07, FR-023: the setup half of this fixture stands before the start
// barrier, so that round owes its outcome to every context of the attempt.
class ThrowingPairFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override
  {
    (void)state;
    throw std::runtime_error("the scripted fixture setup failure");
  }

  auto tearDown(sg::State& state) -> void override { (void)state; }
};

// FR-023, D-07 at the thread count of four: a setup half that throws on
// context 0 leaves every context out of the body, and the report shows the
// benchmark as failed.
auto scenarioSetupFailure() -> int
{
  auto handle = sg::registerFixtureBenchmark(
      [](sg::State& state)
      {
        for (auto index : state) {
          failedPairBodies.fetch_add(1, std::memory_order_relaxed);
          sg::doNotOptimize(index);
        }
      },
      "bmThreadsSetupFail",
      [] { return std::make_unique<ThrowingPairFixture>(); });
  handle.threads(kThreadCount);

  int status = 0;
  const std::string report =
      captureRun({"--filter", "^bmThreadsSetupFail", "--iterations=1"}, status);

  check(status != 0,
        "a threaded attempt whose setup half throws ends with a failure status "
        "(FR-023)");
  check(lineOf(report, "bmThreadsSetupFail").contains("FAILED"),
        "the report shows the benchmark whose setup half threw as failed "
        "(FR-023)");
  check(report.contains("the scripted fixture setup failure"),
        "the reason of the failed benchmark is the message the setup half "
        "threw (FR-023)");
  check(failedPairBodies.load() == 0,
        "no context calls the body of an attempt whose setup half threw "
        "(D-07, FR-023)");

  std::puts("harness_threads_test setup-failure: ok");
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
  if (mode == "repetition-count") {
    return scenarioRepetitionCount();
  }
  if (mode == "merged-metric") {
    return scenarioMergedMetric();
  }
  if (mode == "merged-rate-metric") {
    return scenarioMergedRateMetric();
  }
  if (mode == "paused-contexts") {
    return scenarioPausedContexts();
  }
  if (mode == "default-pin") {
    return scenarioDefaultPin();
  }
  if (mode == "placement-disclosure") {
    return scenarioPlacementDisclosure();
  }
  if (mode == "body-throws") {
    return scenarioBodyThrows();
  }
  if (mode == "fixture-pair") {
    return scenarioFixturePair();
  }
  if (mode == "setup-failure") {
    return scenarioSetupFailure();
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
  if (mode == "unavailable-fold") {
    return scenarioUnavailableFold();
  }
  if (mode == "fold-disclosed") {
    return scenarioFoldDisclosed();
  }
  if (mode == "zero-divisor") {
    return scenarioZeroDivisor();
  }
  if (mode == "widened-metric") {
    return scenarioWidenedMetric();
  }
  if (mode == "setup-teardown") {
    return scenarioSetupTeardown();
  }
  if (mode == "setup-throws-int") {
    return scenarioSetupThrowsInt();
  }
  if (mode == "teardown-throws-int") {
    return scenarioTearDownThrowsInt();
  }
  if (mode == "skip-one") {
    return scenarioSkipOne();
  }
  if (mode == "skip-two") {
    return scenarioSkipTwo();
  }
  if (mode == "pause-overflow") {
    return scenarioPauseOverflow();
  }
  if (mode == "body-throws-int") {
    return scenarioBodyThrowsInt();
  }
  if (mode == "double-loop") {
    return scenarioDoubleLoop();
  }
  if (mode == "interrupt-reps") {
    return scenarioInterruptReps();
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
           "repetition-count",
           "merged-metric",
           "merged-rate-metric",
           "widened-metric",
           "paused-contexts",
           "default-pin",
           "placement-disclosure",
           "body-throws",
           "fixture-pair",
           "setup-failure",
           "cpu-decision",
           "real-decision",
           "manual-decision",
           "load-completes",
           "own-plan-values",
           "second-dispatch-thread",
           "range-expansion",
           "dense-range-expansion",
           "custom-runner",
           "unavailable-fold",
           "fold-disclosed",
           "zero-divisor",
           "setup-teardown",
           "setup-throws-int",
           "teardown-throws-int",
           "skip-one",
           "skip-two",
           "pause-overflow",
           "body-throws-int",
           "double-loop",
           "interrupt-reps",
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
