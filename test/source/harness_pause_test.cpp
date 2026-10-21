// ============================================================================
// TDD test for pause and resume (T020; US4, C-5, FR-009 to FR-014, SC-002,
// SC-003, SC-004).
//
// Every scenario runs on the fake provider, and each scripted leaf steps by
// a constant per sampling action, so the fold of any span of a run is a count
// of scripted steps. One pause interval puts four sampling actions in the
// run's span: the entry sample, the pause sample, the resume sample, and the
// exit sample. The window fold is three steps, the paused interval one step,
// and the value the row reports two steps over the iteration count:
//
//   iterations=4
//   machine/monotonic  step 1000 ns -> (3 - 1) * 1000 / 4 = 500.000
//   machine/thread_cpu step  800 ns -> (3 - 1) *  800 / 4 = 400.000
//   pause/ops          step  200 ns -> (3 - 1) *  200 / 4 = 100.000
//
// The same benchmark with no pause owns two actions, one step of window and
// no interval to subtract:
//
//   machine/monotonic  -> 1000 / 4 = 250.000
//   machine/thread_cpu ->  800 / 4 = 200.000
//   pause/ops          ->  200 / 4 =  50.000
//
// Both benchmarks attach the `pause/ops` metric, so their plans hold the same
// three cores and their plan calibration costs the same number of sampling
// actions. The difference of the two runs' action counts is therefore the
// sampling cost of the one pause interval, and no constant for the plan
// calibration length stands in this file.
//
// Three scenarios cover a run that leaves the timed loop with a pause still
// open. A skip closes the sampling window at once and the function then
// leaves the loop, so the runner's window top-up owns the closing sample of
// the open interval, the guard destructor leaves the resume out on a skip
// path, and every recorded pair still holds two points (FR-012, FR-017,
// D-11). The last of the three scripts one gap action on the pause sample of
// one repetition: the fold of that paused interval carries the disclosure,
// the fold of the run keeps its two measured end points, and the row
// reports the worse availability of the two folds (research D-13).
//
// Each mode owns one provider script, so it runs in a re-exec of this binary,
// the way the calibration test's scripted scenarios do. A contract breach
// ends that child through the contract facility, which calls `std::abort`,
// and the parent reads the wait status. Frameworkless check()/fail()
// convention.
// ============================================================================

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/calibration.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

// FR-024 removes the fixed pause allowance, so no spelling of that constant
// may stand in the library, the tests, or the guide. A namespace-scope name
// that does not exist is a hard error inside a `requires` clause: the clause
// yields no substitution failure, so the absence check cannot be a
// compile-time one. It stands as the textual CTest entry
// `pause_bound_removed` in test/CMakeLists.txt.

namespace
{

constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 800;
constexpr std::uint64_t kOpsStep = 200;
constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kPausedWork = 2000;

// Two scripted steps of each leaf survive the subtraction of the one paused
// interval, and one step stands in a run with no pause.
constexpr double kPausedRealPerIteration = 2.0
    * static_cast<double>(kMonotonicStep) / static_cast<double>(kIterations);
constexpr double kPausedCpuPerIteration = 2.0
    * static_cast<double>(kThreadCpuStep) / static_cast<double>(kIterations);
constexpr double kPausedOpsPerIteration =
    2.0 * static_cast<double>(kOpsStep) / static_cast<double>(kIterations);
constexpr double kPlainRealPerIteration =
    static_cast<double>(kMonotonicStep) / static_cast<double>(kIterations);
constexpr double kPlainCpuPerIteration =
    static_cast<double>(kThreadCpuStep) / static_cast<double>(kIterations);
constexpr double kPlainOpsPerIteration =
    static_cast<double>(kOpsStep) / static_cast<double>(kIterations);

// The four sampling actions one paused run owns: entry, pause, resume and
// exit. A fixed iteration count leaves the plan no warm-up and no calibration
// phase of the run, so the window of a three-repetition batch holds twelve of
// them beside the actions the plan calibrates itself with.
constexpr std::uint64_t kPausedRunSamples = 4;

// The value of a row whose paused interval carries no measurement: the
// window fold of three steps stands unsubtracted over the iteration count.
constexpr double kGapRowRealPerIteration = 3.0
    * static_cast<double>(kMonotonicStep) / static_cast<double>(kIterations);

// `bmPauseEachIteration` and `bmPairEachIteration` close one interval in
// every iteration of a four-iteration run. A folded pair costs the two
// sampling actions of its own points, so the window of the run spans nine
// steps and the four intervals net four of them out of it.
constexpr std::uint64_t kEveryActions = 2 + 2 * kIterations;
constexpr double kEveryNetSteps =
    static_cast<double>(kEveryActions - 1) - static_cast<double>(kIterations);
constexpr double kEveryRealPerIteration = kEveryNetSteps
    * static_cast<double>(kMonotonicStep) / static_cast<double>(kIterations);
constexpr double kEveryCpuPerIteration = kEveryNetSteps
    * static_cast<double>(kThreadCpuStep) / static_cast<double>(kIterations);
constexpr double kEveryOpsPerIteration = kEveryNetSteps
    * static_cast<double>(kOpsStep) / static_cast<double>(kIterations);

// The window points a raw run of the same shape keeps: the two endpoints
// beside the two points of each of the four intervals.
constexpr std::uint64_t kEveryRawSamples = 2 + 2 * kIterations;

sg::counters::FakeProvider* fake = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PAUSE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The work the paused interval bounds. It stands inside the interval, so a
// run that reported the window fold alone would show it.
auto pausedWork() -> void
{
  std::uint64_t mark = 0;
  for (std::uint64_t index = 0; index < kPausedWork; ++index) {
    mark += index;
  }
  sg::doNotOptimize(mark);
}

// SC-004: the run with no pause call keeps the H1 sample pair.
auto bmPlain(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// FR-009, FR-010: the pair stands inside the timed loop, on the first pass,
// so one run holds exactly one paused interval.
auto bmPause(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.pauseTiming();
      pausedWork();
      state.resumeTiming();
    }
    sg::doNotOptimize(index);
  }
}

// FR-012: the guard pauses for its scope, so the interval it bounds is the
// region the explicit pair bounds.
auto bmScoped(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      sg::ScopedPauseTiming guard {state};
      pausedWork();
    }
    sg::doNotOptimize(index);
  }
}

// FR-011: the second pause before the resume.
auto bmDoublePause(sg::State& state) -> void
{
  for (auto _ : state) {
    state.pauseTiming();
    state.pauseTiming();
    state.resumeTiming();
  }
}

// FR-011: a resume with no open pause.
auto bmResumeFirst(sg::State& state) -> void
{
  for (auto _ : state) {
    state.resumeTiming();
  }
}

// FR-011: the loop leaves with the pause still open.
auto bmOpenAtExit(sg::State& state) -> void
{
  for (auto _ : state) {
    state.pauseTiming();
  }
}

// FR-011, D-15: the callback state opens no sampling window, so the pause
// there is a precondition violation. The callable itself never pauses.
auto bmCallbackPause(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// FR-012, FR-017, D-11: the skip closes the window, the function leaves the
// loop, and the interval the pause opened is the window top-up's to close.
auto bmSkipOpenPause(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.pauseTiming();
      state.skipWithMessage("the run leaves the loop with a pause open");
      return;
    }
  }
}

// FR-012: the guard destructor reads the skip flag of the run, so the resume
// it would otherwise take never runs and the open interval is left for the
// same top-up.
auto bmScopedSkip(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      sg::ScopedPauseTiming guard {state};
      state.skipWithError("the guard leaves its scope on a skip path");
      return;
    }
  }
}

// FR-026: a `ScopedPauseTiming` closed in every iteration of the run. The
// first attempt of a family holds no pause history, so every pair folds at
// its commit and the run still completes with every column netted.
auto bmPauseEachIteration(sg::State& state) -> void
{
  for (auto _ : state) {
    const sg::ScopedPauseTiming guard {state};
    sg::doNotOptimize(_);
  }
}

// FR-052: the same shape written with the explicit pair, so one batch can
// put the first repetition on the fold path and the next on the raw path.
auto bmPairEachIteration(sg::State& state) -> void
{
  for (auto _ : state) {
    state.pauseTiming();
    sg::doNotOptimize(_);
    state.resumeTiming();
  }
}

// FR-051: two closed pairs in every iteration, the shape whose reservation
// the cap of `kMaxPauseSamplesPerLeaf` bounds.
auto bmTwoPairsEachIteration(sg::State& state) -> void
{
  for (auto _ : state) {
    {
      const sg::ScopedPauseTiming first {state};
      sg::doNotOptimize(_);
    }
    {
      const sg::ScopedPauseTiming second {state};
      sg::doNotOptimize(_);
    }
  }
}

auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addObject("pause", "pause", "the pause test object");
  owned->addCounter("pause", "ops", "none", "an event leaf");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  owned->setPoints("pause", "ops", {0}, kOpsStep);
  fake = owned.get();
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// FR-010, Q-4: the metric rides the run plan, so its fold carries the same
// subtraction as the two time columns.
auto attachOps(sg::BenchmarkHandle& handle) -> void
{
  const auto object = sg::counters::System::local().object("pause");
  check(object.has_value(), "the pause object resolves");
  const auto leaf = object->counter<sg::counters::Dim<0, 1>>("ops");
  check(leaf.has_value(), "the ops leaf resolves");
  handle.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*leaf),
                   "ops");
}

// Registers a benchmark whose handle no scenario otherwise needs, and reads
// back the name so a silent registration failure cannot pass as a breach.
auto registerNamed(void (*callable)(sg::State&), const char* name) -> void
{
  const auto handle = sg::registerBenchmark(callable, name);
  check(handle.name() == name, "the breach benchmark registers");
}

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_pause_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_pause_out.txt";
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
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("the run did not exit zero");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto linesOf(const std::string& report,
             const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    // specs/024 FR-021: the pass disclosure line opens with the benchmark
    // name too. It is a disclosure, not a measured row, so the row counts
    // below must not see it.
    if (line.rfind(name, 0) == 0 && line.find("method=") == std::string::npos) {
      found.push_back(line);
    }
  }
  return found;
}

auto doubleFieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

auto closeTo(const double actual, const double expected) -> bool
{
  return std::abs(actual - expected) <= 1e-3 + 1e-9 * std::abs(expected);
}

// SC-002, SC-003, SC-004: the window fold minus the interval fold stands in
// every column, the row names the run's four endpoint samples, and the
// interval costs two sampling actions.
auto scenarioExact() -> int
{
  scriptProvider();
  auto plainHandle = sg::registerBenchmark(&bmPlain, "bmPlain");
  attachOps(plainHandle);
  auto pauseHandle = sg::registerBenchmark(&bmPause, "bmPause");
  attachOps(pauseHandle);

  const std::uint64_t plainBefore = fake->readActions();
  const std::string plain =
      captureRun({"--filter", "^bmPlain$", "--iterations=4"});
  const std::uint64_t plainActions = fake->readActions() - plainBefore;

  const std::uint64_t pauseBefore = fake->readActions();
  const std::string report =
      captureRun({"--filter", "^bmPause$", "--iterations=4"});
  const std::uint64_t pauseActions = fake->readActions() - pauseBefore;

  const std::vector<std::string> plainRows = linesOf(plain, "bmPlain");
  check(plainRows.size() == 1,
        "the run with no pause prints one measured row (FR-012)");
  check(plainRows[0].contains("endpoint samples=2"),
        "a run with no pause owns one sample pair (FR-013, SC-004)");
  check(closeTo(doubleFieldOf(plainRows[0], "real time/iter (ns)="),
                kPlainRealPerIteration),
        "the real column is the window fold over N (FR-001)");
  check(closeTo(doubleFieldOf(plainRows[0], "cpu time/iter (ns)="),
                kPlainCpuPerIteration),
        "the CPU column follows machine/thread_cpu (FR-001)");
  check(closeTo(doubleFieldOf(plainRows[0], "ops="), kPlainOpsPerIteration),
        "the attached metric keeps its H1 fold (FR-022)");

  const std::vector<std::string> rows = linesOf(report, "bmPause");
  check(rows.size() == 1, "the paused run prints one measured row (FR-012)");
  // The family holds no pause history when its first attempt starts, so the
  // pair folds at its commit: the window keeps its two endpoint points and
  // the run discloses the fold (FR-051, FR-054).
  check(rows[0].contains("endpoint samples=4"),
        "the folded row prints the endpoint count the raw path prints "
        "(FR-052)");
  check(rows[0].contains("pause=folded"),
        "the folded run carries the disclosure token (FR-054)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="),
                kPausedRealPerIteration),
        "the real column is the window fold minus the interval fold "
        "(FR-010, SC-003)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="),
                kPausedCpuPerIteration),
        "the CPU column carries the same subtraction (FR-010, SC-003)");
  check(closeTo(doubleFieldOf(rows[0], "ops="), kPausedOpsPerIteration),
        "the attached metric excludes the paused work (FR-010, Q-4)");
  check(pauseActions == plainActions + 2,
        "one pause interval costs two sampling actions (FR-009)");
  std::puts("harness_pause_test exact: ok");
  return 0;
}

// FR-012: the guard reports the same value as the explicit pair, because it
// bounds the same region of the same scripted run.
auto scenarioScoped() -> int
{
  scriptProvider();
  auto pauseHandle = sg::registerBenchmark(&bmPause, "bmPause");
  attachOps(pauseHandle);
  auto scopedHandle = sg::registerBenchmark(&bmScoped, "bmScoped");
  attachOps(scopedHandle);

  const std::string report = captureRun({"--filter", "^bm", "--iterations=4"});
  const std::vector<std::string> explicitRows = linesOf(report, "bmPause");
  const std::vector<std::string> guardRows = linesOf(report, "bmScoped");
  check(explicitRows.size() == 1 && guardRows.size() == 1,
        "both benchmarks print one measured row (FR-012)");
  check(guardRows[0].contains("endpoint samples=4"),
        "the guard owns the same four endpoint samples the explicit pair "
        "prints (FR-012, FR-052)");
  for (const char* field :
       {"real time/iter (ns)=", "cpu time/iter (ns)=", "ops="})
  {
    const std::string message = std::string(
                                    "the guard reports the value the explicit "
                                    "pair reports " "(FR-012): ")
        + field;
    check(closeTo(doubleFieldOf(guardRows[0], field),
                  doubleFieldOf(explicitRows[0], field)),
          message.c_str());
  }
  std::puts("harness_pause_test scoped: ok");
  return 0;
}

// FR-012, FR-017, D-11: a run that leaves the loop with a pause open still
// owns its closing sample, and the skip stands as the one outcome of the
// benchmark.
auto scenarioSkipOpenPause() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmSkipOpenPause, "bmSkipOpenPause");
  attachOps(handle);
  const std::string report =
      captureRun({"--filter", "^bmSkipOpenPause$", "--iterations=4"});
  check(report.contains("SKIPPED: the run leaves the loop with a pause open"),
        "the skipped run reports the reason it gave (FR-031)");
  check(!report.contains("iterations="),
        "a skipped run prints no measured row (FR-031)");
  std::puts("harness_pause_test skipOpenPause: ok");
  return 0;
}

// FR-012: the guard that leaves its scope on a skip path leaves one interval
// open, and the run still reports the single skip it requested.
auto scenarioScopedSkip() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmScopedSkip, "bmScopedSkip");
  attachOps(handle);
  const std::string report =
      captureRun({"--filter", "^bmScopedSkip$", "--iterations=4"});
  check(report.contains("SKIPPED: the guard leaves its scope on a skip path"),
        "the guard path reports the skip it requested (FR-012, FR-031)");
  check(!report.contains("iterations="),
        "a skipped run prints no measured row (FR-031)");
  std::puts("harness_pause_test scopedSkip: ok");
  return 0;
}

// D-13, FR-025: one gap action on the pause sample of one repetition makes
// the fold of that paused interval carry the disclosure, while the fold of
// the run keeps two measured end points. The row reports the worse of the
// two, so its time columns keep the window fold unsubtracted and the
// aggregate leaves the repetition out.
auto scenarioGapInPause() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPause, "bmGap");
  attachOps(handle);

  // The gap script names the sampling actions of the window that samples the
  // leaf, and that window opens with the actions the plan calibrates itself
  // with, so one single-repetition run reads the offset they leave: the four
  // actions of one paused run beside that fixed prefix.
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^bmGap$", "--iterations=4", "--repetitions=1"});
  const std::uint64_t oneRun = fake->readActions() - before;

  // The window of the run that follows holds three repetitions, so the pause
  // sample of its last repetition stands two paused runs and two actions
  // past the single run the read above measured.
  fake->setGapActions("pause", "ops", {oneRun + (2 * kPausedRunSamples) - 2});
  const std::string report =
      captureRun({"--filter", "^bmGap$", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> rows = linesOf(report, "bmGap");
  check(rows.size() == 5,
        "three repetitions print three rows, the time aggregate and the "
        "metric aggregate (FR-035)");

  std::size_t gapRows = 0;
  for (std::size_t row = 0; row < rows.size(); ++row) {
    if (rows[row].contains("gap=1")) {
      ++gapRows;
      check(row == 2,
            "the scripted action is the pause sample of the last repetition");
    }
  }
  check(gapRows == 1, "one repetition carries the gap disclosure (FR-025)");
  check(closeTo(doubleFieldOf(rows[2], "real time/iter (ns)="),
                kGapRowRealPerIteration),
        "the row whose paused interval carries the gap prints the window "
        "fold alone (D-13)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="),
                kPausedRealPerIteration)
            && closeTo(doubleFieldOf(rows[1], "real time/iter (ns)="),
                       kPausedRealPerIteration),
        "the two clean repetitions keep the netted value (FR-010)");
  check(rows[3].contains(" n=2"),
        "the promoted availability keeps that repetition out of the time "
        "aggregate (D-13, FR-025)");
  std::puts("harness_pause_test gapInPause: ok");
  return 0;
}

// FR-011: a second pause before the resume ends the process.
auto modeDoublePause() -> int
{
  scriptProvider();
  registerNamed(&bmDoublePause, "bmDoublePause");
  captureRun({"--filter", "^bmDoublePause$", "--iterations=1"});
  return 0;
}

// FR-011: a resume with no open pause ends the process.
auto modeResumeFirst() -> int
{
  scriptProvider();
  registerNamed(&bmResumeFirst, "bmResumeFirst");
  captureRun({"--filter", "^bmResumeFirst$", "--iterations=1"});
  return 0;
}

// FR-011: a pause still open at loop exit ends the process.
auto modeOpenAtExit() -> int
{
  scriptProvider();
  registerNamed(&bmOpenAtExit, "bmOpenAtExit");
  captureRun({"--filter", "^bmOpenAtExit$", "--iterations=1"});
  return 0;
}

// FR-011, D-15: a pause in a setup callback ends the process.
auto modeCallbackPause() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmCallbackPause, "bmCallbackPause");
  handle.setup([](sg::State& callbackState) { callbackState.pauseTiming(); });
  captureRun({"--filter", "^bmCallbackPause$", "--iterations=1"});
  return 0;
}

// FR-052, D-13: the raw pair whose own two points read a gap. The family
// earns its history from an unscripted run, so the scripted run reserves
// room and records its pairs beside the window, where the row still
// discloses the gap that pair read.
auto scenarioRawGapPair() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPauseEachIteration, "bmRawGap");
  attachOps(handle);

  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^bmRawGap$", "--iterations=4"});
  const std::uint64_t span = fake->readActions() - before;
  std::vector<std::uint64_t> interior;
  for (std::uint64_t action = span / 2 + 1; action + 2 < span; ++action) {
    interior.push_back(action);
  }
  fake->setGapActions("pause", "ops", interior);
  const std::string report =
      captureRun({"--filter", "^bmRawGap$", "--iterations=4"});

  const std::vector<std::string> rows = linesOf(report, "bmRawGap");
  check(rows.size() == 1, "the scripted run prints its one measured row");
  check(!rows[0].contains("pause=folded"),
        "the family with pause history records its pairs beside the window "
        "(FR-051)");
  check(rows[0].contains("gap=1"),
        "the pair whose points read a gap discloses the gap the row reports "
        "(FR-052, D-13)");
  check(!closeTo(doubleFieldOf(rows[0], "ops="), kEveryOpsPerIteration),
        "a pair that reports no value subtracts nothing from the window fold "
        "(D-13)");
  std::puts("harness_pause_test rawGapPair: ok");
  return 0;
}

// E-08, FR-051: the attempt whose earned history cannot fit the cap. The
// first run earns 17000 pairs per iteration, so the second asks for twice
// that over a cap of 32768, reserves no room at all, and folds every
// interval it closes.
auto bmManyEachIteration(sg::State& state) -> void
{
  for (auto index : state) {
    for (std::uint64_t pair = 0; pair < 17000; ++pair) {
      state.pauseTiming();
      sg::doNotOptimize(index + pair);
      state.resumeTiming();
    }
  }
}

auto scenarioHistoryOverCap() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmManyEachIteration, "bmMany");
  attachOps(handle);
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^bmMany$", "--iterations=1"});
  const std::uint64_t span = fake->readActions() - before;
  // The gaps sit a quarter of the window away from either end point, so the
  // window itself stays measured and only the folded pairs read a gap.
  std::vector<std::uint64_t> interior;
  for (std::uint64_t action = span / 4; action + span / 4 < span; ++action) {
    interior.push_back(action);
  }
  fake->setGapActions("pause", "ops", interior);
  const std::string report =
      captureRun({"--filter", "^bmMany$", "--iterations=1"});
  const std::vector<std::string> rows = linesOf(report, "bmMany");
  check(rows.size() == 1,
        "the attempt over the cap prints its one measured row (FR-051)");
  check(rows[0].contains("pause=folded"),
        "history that cannot fit the cap folds every interval (FR-026)");
  check(rows[0].contains("gap=1"),
        "a gap inside a folded interval reaches the row through the running "
        "totals the fold left behind (FR-052, D-13)");
  check(sg::detail::lastRecorderReservation().pauseRoomPoints == 0,
        "an attempt over the cap reserves no pause room (FR-051)");
  std::puts("harness_pause_test historyOverCap: ok");
  return 0;
}

// FR-052, D-13: the folded pair whose own two points read a gap. The window
// keeps its two measured end points, so the value of the column stays the
// window fold, while the availability the fold reported is the worse of the
// two folds and stands as the row's.
auto scenarioFoldedGap() -> int
{
  scriptProvider();
  // A probe family measures what one window spends, so the family the script
  // runs starts with no pause history and folds every pair it closes.
  auto probe = sg::registerBenchmark(&bmPauseEachIteration, "bmFoldProbe");
  attachOps(probe);
  auto handle = sg::registerBenchmark(&bmPauseEachIteration, "bmFoldGap");
  attachOps(handle);

  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^bmFoldProbe$", "--iterations=4"});
  const std::uint64_t span = fake->readActions() - before;
  // The script names the actions of the window that samples the leaf, and a
  // fresh window opens with the actions the plan calibrates itself with, so
  // the actions between the open and the close point of the span the probe
  // measured are the interior a pair folds on.
  std::vector<std::uint64_t> interior;
  for (std::uint64_t action = 2; action + 2 < span; ++action) {
    interior.push_back(action);
  }
  fake->setGapActions("pause", "ops", interior);
  const std::string report =
      captureRun({"--filter", "^bmFoldGap$", "--iterations=4"});

  const std::vector<std::string> rows = linesOf(report, "bmFoldGap");
  check(rows.size() == 1, "the folded run prints its one measured row");
  check(rows[0].contains("pause=folded"),
        "the no-history attempt folds the pair it closes (FR-054)");
  check(rows[0].contains("gap=1"),
        "the fold of the gapped pair discloses the gap the row reports "
        "(FR-052, D-13)");
  check(!closeTo(doubleFieldOf(rows[0], "ops="), kEveryOpsPerIteration),
        "a fold that reports no value subtracts nothing, so the column keeps "
        "the window fold alone (D-13)");
  check(!closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="),
                 kEveryRealPerIteration),
        "a row whose fold disclosed a gap does not print the value the clean "
        "fold would have netted out (D-13)");
  std::puts("harness_pause_test foldedGap: ok");
  return 0;
}

// FR-026, SC-11: the run that closes an interval in every iteration.
// Calibration sees no pause history when the family starts, so every pair of
// the attempt folds at its commit and the run reports the netted columns of
// the same arithmetic the raw path would report.
auto scenarioEveryIteration() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPauseEachIteration, "bmEvery");
  attachOps(handle);
  const std::string report =
      captureRun({"--filter", "^bmEvery$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmEvery");
  check(rows.size() == 1,
        "the run that pauses in every iteration prints one measured row "
        "(FR-026)");
  check(rows[0].contains("pause=folded"),
        "every pair of a no-history attempt folds at its commit (FR-026, "
        "FR-054)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="),
                kEveryRealPerIteration),
        "the real column nets every folded interval out of the window fold "
        "(FR-026, SC-11)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="),
                kEveryCpuPerIteration),
        "the CPU column carries the same subtraction (FR-026)");
  check(closeTo(doubleFieldOf(rows[0], "ops="), kEveryOpsPerIteration),
        "the attached metric nets the folded intervals too (FR-026, Q-4)");
  check(sg::detail::lastRecorderReservation().pauseRoomPoints == 0,
        "the no-history attempt reserves no pause room (FR-051)");
  std::puts("harness_pause_test everyIteration: ok");
  return 0;
}

// FR-026, FR-051, SC-11: two closed pairs in every iteration of a calibrated
// run. The reservation the earned history would ask for passes the cap of
// `kMaxPauseSamplesPerLeaf`, so that attempt folds and keeps every point,
// and no reservation the run makes passes the cap.
auto scenarioPauseHeavy() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmTwoPairsEachIteration, "bmHeavy");
  attachOps(handle);
  const std::uint64_t before = fake->readActions();
  const std::string report = captureRun({"--filter", "^bmHeavy$"});
  check(fake->readActions() > before,
        "the calibrated run sampled the scripted leaves");
  check(!report.contains("FAILED"),
        "the run whose pause room passes the cap no longer ends the process "
        "(FR-026, SC-11)");
  const std::vector<std::string> rows = linesOf(report, "bmHeavy");
  check(rows.size() == 1,
        "the calibrated heavy run prints one measured row (FR-026)");
  check(rows[0].contains("pause=folded"),
        "the attempt whose reservation would pass the cap folds (FR-051)");
  const std::size_t room = sg::detail::maxRecorderPauseRoom();
  check(room > 0, "an attempt of the family earned pause room (FR-051)");
  check(room <= sg::detail::kMaxPauseSamplesPerLeaf,
        "no reservation of the run passes the per-leaf cap (FR-051)");
  std::puts("harness_pause_test pauseHeavy: ok");
  return 0;
}

// FR-051, SC-12: the run that pauses nothing reserves the two points of the
// window alone, so the recorder shape of the fold path costs it nothing.
auto scenarioNonPauseReserve() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPlain, "bmPlain");
  attachOps(handle);
  const std::string report =
      captureRun({"--filter", "^bmPlain$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmPlain");
  check(rows.size() == 1, "the run with no pause prints one measured row");
  check(!rows[0].contains("pause=folded"),
        "a run that folds nothing carries no disclosure token (FR-054)");
  const sg::detail::RecorderReservation& reservation =
      sg::detail::lastRecorderReservation();
  check(reservation.pauseRoomPoints == 0,
        "the run with no pause reserves no pause room (FR-051, SC-12)");
  check(
      reservation.endpointPoints == 2,
      "the run with no pause reserves the one pair it owns (FR-051, " "SC-12)");
  check(sg::detail::maxRecorderPauseRoom() == 0,
        "no attempt of the family earned pause room (FR-051, SC-12)");
  std::puts("harness_pause_test nonPauseReserve: ok");
  return 0;
}

// FR-052, SC-13: two repetitions of one fixed-iterations family. The first
// folds on the no-history attempt; the second earns the room its own history
// asks for and keeps every point. The columns of the two rows are equal
// field for field, the token apart.
auto scenarioRawAfterHistory() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPairEachIteration, "bmPair");
  attachOps(handle);
  const std::string report =
      captureRun({"--filter", "^bmPair$", "--iterations=4", "--repetitions=2"});
  const std::vector<std::string> rows = linesOf(report, "bmPair");
  check(rows.size() == 4,
        "two repetitions print two rows, the time aggregate and the metric "
        "aggregate (FR-035)");
  check(rows[0].contains("pause=folded"),
        "the first repetition of the family folds (FR-051, FR-054)");
  check(!rows[1].contains("pause=folded"),
        "the repetition that earned its room takes the raw path (FR-052)");
  const std::string samples =
      "endpoint samples=" + std::to_string(kEveryRawSamples);
  check(rows[0].contains(samples) && rows[1].contains(samples),
        "both rows print the two points of every interval they own, the "
        "fold path and the raw path alike (FR-013, FR-052, SC-13)");
  for (const char* field :
       {"real time/iter (ns)=", "cpu time/iter (ns)=", "ops="})
  {
    const std::string message =
        std::string("the folded row and the raw row print the same value ")
        + "(FR-052, SC-13): " + field;
    check(closeTo(doubleFieldOf(rows[0], field), doubleFieldOf(rows[1], field)),
          message.c_str());
  }
  check(closeTo(doubleFieldOf(rows[1], "real time/iter (ns)="),
                kEveryRealPerIteration),
        "the raw row carries the netted real value (FR-052, SC-13)");
  check(closeTo(doubleFieldOf(rows[1], "cpu time/iter (ns)="),
                kEveryCpuPerIteration),
        "the raw row carries the netted CPU value (FR-052, SC-13)");
  check(closeTo(doubleFieldOf(rows[1], "ops="), kEveryOpsPerIteration),
        "the raw row carries the netted metric value (FR-052, SC-13)");
  check(sg::detail::lastRecorderReservation().pauseRoomPoints
            == 2 * kIterations,
        "the raw reservation is two points for each interval the history "
        "earned (FR-051, SC-13)");
  // D-13, FR-002: the aggregate summarizes the primary time, the CPU fold
  // under the default selection, and both repetitions are countedable.
  check(closeTo(doubleFieldOf(rows[2], "mean="), kEveryCpuPerIteration)
            && rows[2].contains(" n=2"),
        "the folded repetition enters the aggregate with the value the raw "
        "one has (FR-052, SC-13)");
  check(closeTo(doubleFieldOf(rows[3], "mean="), kEveryOpsPerIteration),
        "the metric aggregate carries the netted value of both paths "
        "(FR-052, SC-13)");
  std::puts("harness_pause_test rawAfterHistory: ok");
  return 0;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "exact") {
    return scenarioExact();
  }
  if (mode == "scoped") {
    return scenarioScoped();
  }
  if (mode == "skipOpenPause") {
    return scenarioSkipOpenPause();
  }
  if (mode == "scopedSkip") {
    return scenarioScopedSkip();
  }
  if (mode == "gapInPause") {
    return scenarioGapInPause();
  }
  if (mode == "everyIteration") {
    return scenarioEveryIteration();
  }
  if (mode == "foldedGap") {
    return scenarioFoldedGap();
  }
  if (mode == "historyOverCap") {
    return scenarioHistoryOverCap();
  }
  if (mode == "rawGapPair") {
    return scenarioRawGapPair();
  }
  if (mode == "pauseHeavy") {
    return scenarioPauseHeavy();
  }
  if (mode == "nonPauseReserve") {
    return scenarioNonPauseReserve();
  }
  if (mode == "rawAfterHistory") {
    return scenarioRawAfterHistory();
  }
  if (mode == "doublePause") {
    return modeDoublePause();
  }
  if (mode == "resumeFirst") {
    return modeResumeFirst();
  }
  if (mode == "openAtExit") {
    return modeOpenAtExit();
  }
  if (mode == "callbackPause") {
    return modeCallbackPause();
  }

  const std::string self = argv[0];
  for (const auto& scenario : {
           "exact",
           "scoped",
           "skipOpenPause",
           "scopedSkip",
           "gapInPause",
           "everyIteration",
           "foldedGap",
           "historyOverCap",
           "rawGapPair",
           "pauseHeavy",
           "nonPauseReserve",
           "rawAfterHistory",
       })
  {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (SC-003)");
  }

  // FR-011, FR-014: each breach ends its child through the contract
  // facility, which aborts. A shell that waits reports that death as its own
  // exit 134, so the loop forks and execs directly, the pattern of
  // harness_manual_time_test.cpp's guard modes, and the parent reads the
  // signal from waitpid.
  for (const auto& breach :
       {"doublePause", "resumeFirst", "openAtExit", "callbackPause"})
  {
    const pid_t pid = fork();
    if (pid < 0) {
      fail("cannot fork the breached child");
    }
    if (pid == 0) {
      if (std::freopen("/dev/null", "w", stderr) == nullptr) {
        _exit(2);
      }
      char* const execArgv[] = {
          const_cast<char*>(self.c_str()), const_cast<char*>(breach), nullptr};
      (void)execv(self.c_str(), execArgv);
      _exit(2);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
      fail("cannot wait for the breached child");
    }
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
          "the breach ends the signaled child (FR-011, FR-014)");
  }

  std::puts("harness_pause_test: ok");
  return 0;
}
