// ============================================================================
// TDD test for custom aggregate statistics (T021; US5, FR-013 to FR-015,
// SC-03).
//
// `BenchmarkHandle::addStatistic` registers a (name, unit, function) triple.
// The function computes over the clean repetition values of the primary time,
// the same list the built-in statistics consume, and its value prints among
// the built-in fields of the aggregate row after them, in registration order
// (FR-013). A `TIME` statistic prints in the unit the row prints in, so it
// divides by the unit divisor like the built-in time fields; a `PERCENTAGE`
// statistic prints with no unit scaling (FR-014). A function that returns
// false leaves its field out of the row while the run still reports (FR-015).
// A name equal to a built-in statistic is a precondition violation at the
// call, so that scenario runs in a re-exec child that dies through the
// contract facility (spec Edge Cases).
//
// The scripted provider gives the three repetitions primary CPU folds of 800,
// 1600 and 2400 ns over four fixed iterations, so the primary list is
// 200, 400 and 600 ns per iteration and the statistics have exact expected
// values:
//
//   mean            (200 + 400 + 600) / 3          = 400 ns
//   range (TIME)     600 - 200                     = 400 ns
//   share (PERCENT)  the value the function writes = 42.5
//
// One sampling window holds the whole plan, calibration reads first and the
// endpoint pairs of the repetitions last, and two captures of one plan hold the
// same number of actions, so a probe capture of the same flags names the length
// the fold pattern is placed after. `setPoints` rewinds the leaf, so the
// rewritten script keeps the calibration stretch point for point the ramp of
// the probe and carries the three folds only where the measured endpoints read.
//
// The family prints in microseconds, so the `TIME` statistic reads 0.400 where
// an unscaled value would read 400.000, and the `PERCENTAGE` statistic reads
// 42.500 where a unit-scaled value would read 0.042 (FR-014, SC-03).
//
// Each mode owns one provider script, so it runs in a re-exec of this binary.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
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

#include "detail/gcov_flush.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The primary CPU fold of each of the three repetitions, and the per-iteration
// values the statistic functions receive.
constexpr std::uint64_t kFoldFirst = 800;
constexpr std::uint64_t kFoldSecond = 1600;
constexpr std::uint64_t kFoldThird = 2400;
constexpr std::uint64_t kRepetitions = 3;
constexpr double kRangeNs = 400.0;
constexpr double kSharePercent = 42.5;

// The CPU step of the probe ramp, the fold of every run it measures, and the
// two endpoint reads one repetition takes out of the window.
constexpr std::uint64_t kCpuStep = kFoldFirst;
constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kRunSamples = 2;

// The row prints microseconds, so a time value divides by 1000 (FR-014).
constexpr double kMicroseconds = 1000.0;

sg::counters::FakeProvider* fake = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CUSTOM STATS TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Registers the two machine leaves every scenario needs. The CPU leaf walks a
// constant step, so every run of the probe folds one step.
auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {}, kCpuStep);
  fake = owned.get();
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// Rewrites the CPU script so the three measured repetitions of the next capture
// fold `kFoldFirst`, `kFoldSecond` and `kFoldThird`: the `calibration` reads
// before them keep the ramp of the probe point for point, and the endpoint
// pairs that follow carry the three folds (FR-013).
auto scriptFolds(const std::uint64_t calibration) -> void
{
  std::vector<std::uint64_t> points;
  points.reserve(calibration + kRunSamples * kRepetitions);
  for (std::uint64_t read = 1; read <= calibration; ++read) {
    points.push_back(read * kCpuStep);
  }
  const std::uint64_t first = (calibration + 1) * kCpuStep;
  points.push_back(first);
  points.push_back(first + kFoldFirst);
  points.push_back(first + kFoldFirst);
  points.push_back(first + kFoldFirst + kFoldSecond);
  points.push_back(first + kFoldFirst + kFoldSecond);
  points.push_back(first + kFoldFirst + kFoldSecond + kFoldThird);
  fake->setPoints("machine", "thread_cpu", points, kCpuStep);
}

auto bmStatistics(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// The family the probe capture runs, so the measured family's report holds only
// the rows its scenario reads. The same flags give it the same sampling plan.
auto bmProbe(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// Spec Edge Cases: the name equals a built-in statistic.
auto builtinNameStatistic(sg::BenchmarkHandle& handle) -> void
{
  handle.addStatistic("mean",
                      sg::StatUnit::TIME,
                      [](const std::vector<double>&, double& result)
                      {
                        result = 1.0;
                        return true;
                      });
}

auto captureRun(const std::vector<std::string>& arguments,
                int& status) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_custom_stats_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_custom_stats_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto captureRunWithStatus(const std::vector<std::string>& arguments,
                          int& status) -> std::string
{
  return captureRun(arguments, status);
}

auto linesOf(const std::string& report,
             const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

auto measuredRowsOf(const std::vector<std::string>& lines)
    -> std::vector<std::string>
{
  std::vector<std::string> rows;
  for (const auto& line : lines) {
    if (line.contains("iterations=")) {
      rows.push_back(line);
    }
  }
  return rows;
}

auto aggregateLineOf(const std::vector<std::string>& lines) -> std::string
{
  for (const auto& line : lines) {
    if (line.contains("AGGREGATE")) {
      return line;
    }
  }
  fail("the batch prints no aggregate row");
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

auto indexOf(const std::string& haystack,
             const std::string& needle) -> std::size_t
{
  const std::size_t at = haystack.find(needle);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no '%s' in '%s'\n", needle.c_str(), haystack.c_str());
    fail("the report carries no such text");
  }
  return at;
}

auto closeTo(const double actual, const double expected) -> bool
{
  return std::abs(actual - expected) <= 1e-3 + 1e-9 * std::abs(expected);
}

// FR-013, FR-014, SC-03: the range of the primary values in the row unit and
// the percentage beside it, both after the built-in fields.
auto scenarioPrinted() -> int
{
  scriptProvider();
  auto probe = sg::registerBenchmark(&bmProbe, "bmProbe");
  check(probe.name() == "bmProbe", "the probe family registers");
  auto handle = sg::registerBenchmark(&bmStatistics, "bmStats");
  check(handle.name() == "bmStats", "the statistic benchmark registers");
  handle.unit(sg::TimeUnit::MICROSECONDS);
  auto& chained = handle.addStatistic(
      "range",
      sg::StatUnit::TIME,
      [](const std::vector<double>& values, double& result)
      {
        const auto limits = std::minmax_element(values.begin(), values.end());
        result = *limits.second - *limits.first;
        return true;
      });
  check(&chained == &handle,
        "the call returns the handle for chaining (FR-013)");
  handle.addStatistic("share",
                      sg::StatUnit::PERCENTAGE,
                      [](const std::vector<double>& values, double& result)
                      {
                        result = kSharePercent
                            * static_cast<double>(values.size())
                            / static_cast<double>(values.size());
                        return true;
                      });

  // The probe names the actions of one window; the endpoint pairs of the three
  // repetitions stand at its end, so the calibration stretch holds the rest.
  const std::uint64_t before = fake->readActions();
  int probeStatus = 0;
  (void)captureRun(
      {"--filter", "^bmProbe$", "--iterations=4", "--repetitions=3"},
      probeStatus);
  check(probeStatus == 0, "the probe run reports");
  const std::uint64_t window = fake->readActions() - before;
  const std::uint64_t calibration =
      window - kRunSamples * static_cast<std::uint64_t>(kRepetitions);
  scriptFolds(calibration);

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmStats$", "--iterations=4", "--repetitions=3"}, status);
  check(status == 0, "a run with custom statistics still reports (FR-015)");

  const std::vector<std::string> lines = linesOf(report, "bmStats");
  for (const auto& row : measuredRowsOf(lines)) {
    check(!row.contains("range=") && !row.contains("share="),
          "a repetition row carries no custom statistic field (FR-013)");
  }

  const std::string aggregate = aggregateLineOf(lines);
  check(indexOf(aggregate, "mean=") < indexOf(aggregate, "range=")
            && indexOf(aggregate, "sd=") < indexOf(aggregate, "range="),
        "the statistic prints among the built-in fields (FR-013)");
  check(indexOf(aggregate, "range=") < indexOf(aggregate, "share="),
        "the statistics print in registration order (FR-013)");
  check(closeTo(fieldOf(aggregate, "range="), kRangeNs / kMicroseconds),
        "a TIME statistic prints in the unit the row prints in (FR-014)");
  check(closeTo(fieldOf(aggregate, "share="), kSharePercent),
        "a PERCENTAGE statistic prints unscaled (FR-014, SC-03)");
  check(closeTo(fieldOf(aggregate, "mean="), 400.0 / kMicroseconds)
            && closeTo(fieldOf(aggregate, "min="), 200.0 / kMicroseconds)
            && closeTo(fieldOf(aggregate, "max="), 600.0 / kMicroseconds),
        "the statistic functions received the three primary values (FR-013)");
  return 0;
}

// FR-015: the function declines, so its field stands aside and the run reports.
auto scenarioRefused() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmStatistics, "bmRefused");
  check(handle.name() == "bmRefused", "the refusing benchmark registers");
  handle.addStatistic("nothing",
                      sg::StatUnit::TIME,
                      [](const std::vector<double>&, double&)
                      { return false; });

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmRefused$", "--iterations=4", "--repetitions=3"}, status);
  check(status == 0, "a declined statistic leaves the run reporting (FR-015)");

  const std::vector<std::string> lines = linesOf(report, "bmRefused");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  const std::string aggregate = aggregateLineOf(lines);
  check(!aggregate.contains("nothing="),
        "a function returning false prints no value (FR-015)");
  check(aggregate.contains("mean=") && aggregate.contains("n="),
        "the built-in fields print beside the declined statistic (FR-015)");
  return 0;
}

// Spec Edge Cases: the built-in name aborts the re-exec at the call.
auto scenarioBuiltinName() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmStatistics, "bmBuiltin");
  check(handle.name() == "bmBuiltin", "the breach benchmark registers");
  builtinNameStatistic(handle);
  int status = 0;
  (void)captureRunWithStatus(
      {"--filter", "^bmBuiltin$", "--iterations=4", "--repetitions=3"}, status);
  return 0;
}

auto runBreach(const std::string& self,
               const char* mode,
               const char* const expected) -> void
{
  const char* const path = "harness_custom_stats_breach.txt";
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
        "the built-in statistic name ends the signaled child (FR-013)");

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string message = buffer.str();
  if (!message.contains(expected)) {
    std::fprintf(stderr, "no '%s' in '%s'\n", expected, message.c_str());
    fail("the breach message states no such rule");
  }
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::vector<std::string> arguments(argv, argv + argc);
  if (arguments.size() == 2) {
    sg::test::flushCountersOnAbort();
    const std::string& mode = arguments[1];
    if (mode == "printed") {
      return scenarioPrinted();
    }
    if (mode == "refused") {
      return scenarioRefused();
    }
    if (mode == "builtinName") {
      return scenarioBuiltinName();
    }
    fail("unknown mode");
  }

  const std::string self = argv[0];
  for (const auto& scenario : {"printed", "refused"}) {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (FR-013 to FR-015)");
  }

  runBreach(self, "builtinName", "built-in");

  std::puts("harness_custom_stats_test: ok");
  return 0;
}
