// ============================================================================
// TDD test for real time as the primary time (T013; US2, FR-002, FR-005,
// SC-001).
//
// One script drives both time leaves, and it doubles each run's fold: run
// k's monotonic fold is 1000 * 2^k and its thread CPU fold 2000 * 2^k. The
// two clocks then qualify on different runs of one script, and that is the
// observation the suite makes:
//
//   minimum time 8000 ns
//   useRealTime, the walk that reads the monotonic fold
//     run 0: N=1,   real 1000 -> factor 11.2 -> N=11
//     run 1: N=11,  real 2000 -> factor  5.6 -> N=62
//     run 2: N=62,  real 4000 -> factor  2.8 -> N=174
//     run 3: N=174, real 8000 -> qualifies
//   the default selection, the walk that reads the thread CPU fold
//     run 0: N=1,   cpu 2000 -> factor  5.6 -> N=6
//     run 1: N=6,   cpu 4000 -> factor  2.8 -> N=17
//     run 2: N=17,  cpu 8000 -> qualifies
//
// The qualifying run of the real walk carries the CPU fold 16000, twice the
// target, and the run before it carries 8000: a walk that read the CPU
// column would have stopped at N=62. The qualifying run of the default walk
// carries the real fold 4000, below the target: a walk that read the real
// column would have grown past N=17. One script, two walk lengths, 174
// against 17. The CPU-driven walk of harness_calibration_test.cpp:276-304
// settles at 174 on its own script, where this script settles the same
// benchmark at 17 while the selection stays default.
//
// The script stands on the provider's sampling-action index, and one plan
// consumes its own sampling-cost calibration before the benchmark's first
// endpoint, the fact the calibration test measures with a fixed-N probe. No
// constant here stands for that calibration length.
//
// The aggregate scenario fixes the iteration count, so its three rows carry
// the constant scripted folds: the real column reads 250 ns per iteration
// and the CPU column 500 ns, and the aggregate line summarizes the 250
// (FR-002).
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The scripted fold of run k on each leaf: base << k. The two bases differ
// by a factor of two, which is what separates the two walks.
constexpr std::uint64_t kRealWalkBase = 1000;
constexpr std::uint64_t kCpuWalkBase = 2000;

sg::counters::FakeProvider* fake = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS REAL TIME TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The loop shape carries no work, so every scripted fold of a run belongs
// to that run's window alone.
auto bmWalk(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// speedgunMain writes the report to the standard output stream, so the
// scenario redirects it to a file for the duration of the run and reads the
// report back.
auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  const int saved = ::dup(STDOUT_FILENO);
  if (saved < 0
      || std::freopen("harness_real_time_out.txt", "w", stdout) == nullptr)
  {
    fail("the report redirect opened");
  }
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_real_time_test"));
  for (const std::string& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (::dup2(saved, STDOUT_FILENO) < 0) {
    fail("the standard output stream restored");
  }
  ::close(saved);
  check(status == 0, "the run ended with the success status (FR-025)");

  std::ifstream file("harness_real_time_out.txt");
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

auto linesOf(const std::string& report,
             const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(report);
  for (std::string line; std::getline(stream, line);) {
    if (line.rfind(name, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

auto countFieldOf(const std::string& line, const std::string& key) -> long long
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    fail("the report row carries the field");
  }
  return std::strtoll(line.c_str() + at + key.size(), nullptr, 10);
}

auto doubleFieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    fail("the report row carries the time field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

auto closeTo(const double value, const double expected) -> bool
{
  return std::abs(value - expected) <= 1e-3 + 1e-9 * std::abs(expected);
}

auto scriptedProvider(const std::uint64_t monotonicStep,
                      const std::uint64_t threadCpuStep) -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  fake = owned.get();
  fake->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  fake->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  fake->setPoints("machine", "monotonic", {0}, monotonicStep);
  fake->setPoints("machine", "thread_cpu", {0}, threadCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// One fixed-N run costs exactly the plan's calibration plus its two
// endpoint actions (FR-013), so the difference names the calibration length
// the script below is laid out against.
auto probePlanActions(const std::string& name) -> std::uint64_t
{
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^" + name + "$", "--iterations=1"});
  const std::uint64_t consumed = fake->readActions() - before;
  check(consumed > 2, "the probe run carries the plan's calibration");
  return consumed - 2;
}

// Script one leaf so the window of run k, the pair of actions at the
// scripted offset, folds to base << k. Every other action steps by base,
// the tail the later repetitions read. setPoints rewinds the leaf, so the
// probe's points stand outside the scenario's script.
auto scriptGrowingWalk(const char* leaf,
                       const std::uint64_t planActions,
                       const std::uint64_t base) -> void
{
  std::vector<std::uint64_t> points;
  std::uint64_t value = 0;
  for (std::uint64_t action = 1; action <= planActions + 20; ++action) {
    std::uint64_t delta = base;
    for (std::uint64_t step = 0; step < 8; ++step) {
      if (action == planActions + 2 * step + 2) {
        delta = base << step;
      }
    }
    value += delta;
    points.push_back(value);
  }
  fake->setPoints("machine", leaf, std::move(points), base);
}

// FR-005: with the real selection the growth rule reads the monotonic fold,
// so the walk takes four scripted steps and settles at 174 while the CPU
// fold of the qualifying run stands at twice the target.
auto scenarioRealWalk() -> int
{
  scriptedProvider(kRealWalkBase, kCpuWalkBase);
  auto handle = sg::registerBenchmark(&bmWalk, "bmReal");
  handle.useRealTime();
  const std::uint64_t planActions = probePlanActions("bmReal");
  scriptGrowingWalk("monotonic", planActions, kRealWalkBase);
  scriptGrowingWalk("thread_cpu", planActions, kCpuWalkBase);

  const std::uint64_t before = fake->readActions();
  const std::string report = captureRun(
      {"--filter", "^bmReal$", "--min-time=0.000008", "--repetitions=3"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmReal");
  check(rows.size() == 4,
        "three repetitions print three rows and the aggregate");
  for (std::size_t row = 0; row + 1 < rows.size(); ++row) {
    check(countFieldOf(rows[row], "iterations=") == 174,
          "the real-driven walk settles at 174 (FR-005)");
  }
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="), 8000.0 / 174.0),
        "the qualifying run folds the scripted real sequence (FR-005)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="), 16000.0 / 174.0),
        "that run's CPU fold stands at twice the target (FR-001)");
  check(countFieldOf(rows.back(), " n=") == 3,
        "three repetitions summarize (FR-002)");
  // Four calibration runs, the qualifying run measured as the first
  // repetition, and two later repetitions: six runs of two actions.
  check(consumed == planActions + 2 * 6,
        "the walk grew on the four scripted real folds (FR-005)");
  std::puts("harness_real_time_test real-walk: ok");
  return 0;
}

// The same script with the selection left default: the walk reads the
// thread CPU fold, qualifies one run earlier at 17, and the real fold of
// the qualifying run stands below the target.
auto scenarioCpuWalk() -> int
{
  scriptedProvider(kRealWalkBase, kCpuWalkBase);
  auto handle = sg::registerBenchmark(&bmWalk, "bmPlain");
  (void)handle;
  const std::uint64_t planActions = probePlanActions("bmPlain");
  scriptGrowingWalk("monotonic", planActions, kRealWalkBase);
  scriptGrowingWalk("thread_cpu", planActions, kCpuWalkBase);

  const std::uint64_t before = fake->readActions();
  const std::string report = captureRun(
      {"--filter", "^bmPlain$", "--min-time=0.000008", "--repetitions=3"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmPlain");
  check(rows.size() == 4,
        "three repetitions print three rows and the aggregate");
  for (std::size_t row = 0; row + 1 < rows.size(); ++row) {
    check(countFieldOf(rows[row], "iterations=") == 17,
          "the CPU-driven walk settles at 17 (FR-008)");
  }
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="), 4000.0 / 17.0),
        "that run's real fold stands below the target (FR-001)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="), 8000.0 / 17.0),
        "the qualifying run folds the scripted CPU sequence (FR-008)");
  // Three calibration runs, the qualifying run measured as the first
  // repetition, and two later repetitions: five runs of two actions.
  check(consumed == planActions + 2 * 5,
        "the walk grew on the three scripted CPU folds (FR-008)");
  std::puts("harness_real_time_test cpu-walk: ok");
  return 0;
}

// FR-002: the aggregate summarizes the primary time. With the count fixed
// and both leaves scripted to a constant step, every row carries the same
// two values, so the mean names the column it summarizes.
auto scenarioAggregate() -> int
{
  scriptedProvider(kRealWalkBase, kCpuWalkBase);
  auto handle = sg::registerBenchmark(&bmWalk, "bmAggregate");
  handle.useRealTime();
  const std::string report = captureRun(
      {"--filter", "^bmAggregate$", "--iterations=4", "--repetitions=3"});

  const std::vector<std::string> rows = linesOf(report, "bmAggregate");
  check(rows.size() == 4,
        "three repetitions print three rows and the aggregate");
  for (std::size_t row = 0; row + 1 < rows.size(); ++row) {
    check(closeTo(doubleFieldOf(rows[row], "real time/iter (ns)="), 250.0),
          "each row folds the real leaf over the fixed count (FR-001)");
    check(closeTo(doubleFieldOf(rows[row], "cpu time/iter (ns)="), 500.0),
          "each row folds the CPU leaf over the fixed count (FR-001)");
  }
  check(closeTo(doubleFieldOf(rows.back(), "primary time/iter (ns) mean="),
                250.0),
        "the aggregate summarizes the real column (FR-002)");
  check(closeTo(doubleFieldOf(rows.back(), " median="), 250.0),
        "the median reads the same column (FR-002)");
  check(closeTo(doubleFieldOf(rows.back(), " min="), 250.0),
        "the minimum reads the same column (FR-002)");
  check(closeTo(doubleFieldOf(rows.back(), " max="), 250.0),
        "the maximum reads the same column (FR-002)");
  check(countFieldOf(rows.back(), " n=") == 3,
        "three repetitions summarize (FR-002)");
  std::puts("harness_real_time_test aggregate: ok");
  return 0;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "real-walk") {
    return scenarioRealWalk();
  }
  if (mode == "cpu-walk") {
    return scenarioCpuWalk();
  }
  if (mode == "aggregate") {
    return scenarioAggregate();
  }

  // Each scripted scenario owns one provider script, so each runs in a
  // re-exec of this binary; a failed check exits there and shows here.
  const std::string self = argc > 0 ? argv[0] : "harness_real_time_test";
  for (const auto& scenario : {"real-walk", "cpu-walk", "aggregate"}) {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    // A signaled child carries no exit code, so the exit test stands only
    // for a child that ran to completion.
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes");
  }
  std::puts("harness_real_time_test: ok");
  return 0;
}
