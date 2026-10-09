// ============================================================================
// TDD test for the run-control calibration (T019, T045; US2, FR-008 to
// FR-016, SC-002, SC-014).
//
// The scripted scenarios (SC-002) run on a FakeProvider that owns the
// machine time leaves. Two actions per run (FR-017) make the scripted
// deltas exact, and the scripted `machine/thread_cpu` sequence grows by
// doubling, so every step of the D-3 walk is hand-computable:
//
//   minimum time 8000 ns; run k's decision time 1000 * 2^k ns
//   run 0: N=1,   decision 1000 -> factor 11.2 -> N=11
//   run 1: N=11,  decision 2000 -> factor  5.6 -> N=62
//   run 2: N=62,  decision 4000 -> factor  2.8 -> N=174
//   run 3: N=174, decision 8000 -> qualifies (FR-008)
//
// The scripted sequence is laid out on the provider's sampling-action
// index, and one plan consumes its own sampling-cost calibration before
// the benchmark's first endpoint (the capture test's comment records the
// same fact). The test does not hard-code that calibration length: a
// fixed-N probe run reveals it, because one fixed-N run costs exactly
// the plan's calibration plus its two endpoint actions. `setPoints`
// rewinds the leaf, so the scenario's script is laid out from the probe
// result and the scenario's plan replays it from action one.
//
// The real-clock scenarios stay: the overhead floor of SC-014 is a
// wall-clock figure the plan calibrates against its own reference clock
// (source/counters/plan.cpp), so only a real provider shows it.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

constexpr std::uint64_t kWalkMonotonicStep = 100;
constexpr std::uint64_t kWalkThreadCpuBase = 1000;
constexpr std::uint64_t kFivefoldMonotonicStep = 5000;
constexpr std::uint64_t kFivefoldThreadCpuStep = 100;

sg::counters::FakeProvider* fake = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CALIBRATION TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmWork(sg::State& state) -> void
{
  double accumulator = 1.0;
  for (auto _ : state) {
    for (int index = 0; index < 200; ++index) {
      accumulator += accumulator * 1.000000001;
    }
    if (accumulator < 0.0) {
      std::exit(1);
    }
  }
}

auto bmWalk(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// IF-02: a warm-up phase that ignores the FR-008 stop keeps growing and
// reaches this function's third call. The skip names the overrun, so a
// measured row for this benchmark can only come from a warm-up that
// stopped on the rule.
auto bmCapped(sg::State& state) -> void
{
  static std::uint64_t calls = 0;
  ++calls;
  if (calls == 3) {
    state.skipWithError("warm-up overran the FR-008 stop");
    return;
  }
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_calibration_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_calibration_out.txt";
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
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtoll(line.c_str() + at + key.size(), nullptr, 10);
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

// One fixed-N run costs the plan's sampling-cost calibration plus its
// two endpoint actions, so the difference reveals the calibration length
// the scenario's script has to reach past.
auto probePlanActions() -> std::uint64_t
{
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^bmWalk$", "--iterations=1"});
  const std::uint64_t consumed = fake->readActions() - before;
  check(consumed > 2, "the probe run carries the plan's calibration");
  return consumed - 2;
}

// Script `machine/thread_cpu` so run k's window, the pair of actions at
// the scripted offset, folds to `kWalkThreadCpuBase << k`: the doubling
// sequence the hand-computed walk above reads. Every other action steps
// by the base, which no fold of the scenario sees.
auto scriptGrowingThreadCpu(const std::uint64_t planActions) -> void
{
  std::vector<std::uint64_t> points;
  std::uint64_t value = 0;
  for (std::uint64_t action = 1; action <= planActions + 20; ++action) {
    std::uint64_t delta = kWalkThreadCpuBase;
    for (std::uint64_t step = 0; step < 8; ++step) {
      if (action == planActions + 2 * step + 2) {
        delta = kWalkThreadCpuBase << step;
      }
    }
    value += delta;
    points.push_back(value);
  }
  fake->setPoints(
      "machine", "thread_cpu", std::move(points), kWalkThreadCpuBase);
}

// SC-002: the scripted walk, its step that qualifies, and the rule that
// warm-up and calibration runs add no sample to the statistics.
auto scenarioWalk() -> int
{
  scriptedProvider(kWalkMonotonicStep, kWalkThreadCpuBase);
  const std::uint64_t planActions = probePlanActions();
  scriptGrowingThreadCpu(planActions);

  const std::uint64_t before = fake->readActions();
  const std::string report = captureRun(
      {"--filter", "^bmWalk$", "--min-time=0.000008", "--repetitions=3"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmWalk");
  check(rows.size() == 4,
        "three repetitions print three rows and the aggregate (FR-012)");
  for (std::size_t row = 0; row + 1 < rows.size(); ++row) {
    check(countFieldOf(rows[row], "iterations=") == 174,
          "every repetition reuses the count the walk settled (FR-012)");
  }
  check(countFieldOf(rows.back(), " n=") == 3,
        "the warm-up and calibration runs add no sample (SC-002)");
  check(closeTo(doubleFieldOf(rows[0], "time/iter (ns)="),
                static_cast<double>(kWalkMonotonicStep) / 174.0),
        "the measured run is the qualifying run's fold over N (FR-018)");
  // Three calibration runs, their qualifying run, and the two later
  // repetitions: six runs of two actions over one plan's calibration.
  check(consumed == planActions + 2 * 6,
        "the walk took exactly the four scripted steps (SC-002)");
  std::puts("harness_calibration_test walk: ok");
  return 0;
}

// SC-002, FR-011: a fixed N skips calibration, and its warm-up starts
// at N and grows by the FR-010 rule.
auto scenarioWarmup() -> int
{
  scriptedProvider(kWalkMonotonicStep, kWalkThreadCpuBase);
  const std::uint64_t planActions = probePlanActions();
  scriptGrowingThreadCpu(planActions);

  const std::uint64_t before = fake->readActions();
  const std::string report = captureRun(
      {"--filter", "^bmWalk$", "--iterations=5", "--warmup-time=0.000008"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmWalk");
  check(rows.size() == 1, "the warm-up results are discarded (FR-011)");
  check(countFieldOf(rows[0], "iterations=") == 5,
        "the measured phase runs the fixed N, not the warm-up count "
        "(FR-011, FR-013)");
  // Four warm-up runs grew from N=5 by the scripted decision times, and
  // the measured phase then ran once.
  check(consumed == planActions + 2 * 5,
        "the warm-up started at N and grew by the FR-010 rule (SC-002)");
  std::puts("harness_calibration_test warmup: ok");
  return 0;
}

// IF-02, FR-011: warm-up applies the same stop rule as the measured
// phase. The scripted fivefold step qualifies the first warm-up run on
// real time, so one warm-up run and the fixed-N measured run follow.
auto scenarioWarmupFivefold() -> int
{
  scriptedProvider(kFivefoldMonotonicStep, kFivefoldThreadCpuStep);
  const std::uint64_t planActions = probePlanActions();

  const std::uint64_t before = fake->readActions();
  const std::string report = captureRun(
      {"--filter", "^bmCapped$", "--iterations=5", "--warmup-time=0.000001"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmCapped");
  check(rows.size() == 1, "the run carries one measured row (FR-011)");
  check(countFieldOf(rows[0], "iterations=") == 5,
        "the measured run keeps the fixed count (FR-013)");
  // One warm-up run qualified on the fivefold real-time condition, then
  // the measured run followed: two runs of two actions.
  check(consumed == planActions + 2 * 2,
        "warm-up stopped on the FR-008 rule (FR-011)");
  std::puts("harness_calibration_test warmupFivefold: ok");
  return 0;
}

// SC-002, FR-008: a scripted step whose real time reaches five times the
// minimum time qualifies while its thread CPU time stays below it. A
// fixed N with no warm-up also shows calibration skipped outright.
auto scenarioFivefold() -> int
{
  scriptedProvider(kFivefoldMonotonicStep, kFivefoldThreadCpuStep);
  const std::uint64_t planActions = probePlanActions();

  const std::uint64_t before = fake->readActions();
  const std::string report =
      captureRun({"--filter", "^bmWalk$", "--min-time=0.000001"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmWalk");
  check(rows.size() == 1, "the five-times step ends the walk (FR-008)");
  check(countFieldOf(rows[0], "iterations=") == 1,
        "the first run qualified on real time alone (SC-002)");
  check(closeTo(doubleFieldOf(rows[0], "time/iter (ns)="),
                static_cast<double>(kFivefoldMonotonicStep)),
        "the qualifying run's real time is the scripted delta (FR-020)");
  check(consumed == planActions + 2,
        "one run qualified, so the walk stopped at once (FR-008)");

  const std::uint64_t fixedBefore = fake->readActions();
  const std::string fixed =
      captureRun({"--filter", "^bmWalk$", "--iterations=5"});
  check(fake->readActions() - fixedBefore == planActions + 2,
        "a fixed N skips calibration (FR-013)");
  check(countFieldOf(linesOf(fixed, "bmWalk")[0], "iterations=") == 5,
        "the fixed count reached the run (FR-013)");
  std::puts("harness_calibration_test fivefold: ok");
  return 0;
}

}  // namespace

SG_BENCHMARK(bmWork)
SG_BENCHMARK(bmWalk)
SG_BENCHMARK(bmCapped)

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "walk") {
    return scenarioWalk();
  }
  if (mode == "warmup") {
    return scenarioWarmup();
  }
  if (mode == "warmupFivefold") {
    return scenarioWarmupFivefold();
  }
  if (mode == "fivefold") {
    return scenarioFivefold();
  }

  // The scripted scenarios own one provider script each, so each runs
  // in a re-exec of this binary; its failure exits nonzero here.
  const std::string self = argv[0];
  for (const auto& scenario : {"walk", "warmup", "warmupFivefold", "fivefold"})
  {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(status == 0, "the scripted scenario passes (SC-002)");
  }

  // FR-014 and FR-016: the first repetition calibrates from one
  // iteration, grows by the R-012 rule, and the run that meets the
  // target is the measured run of that repetition.
  const std::string calibrated =
      captureRun({"--filter", "^bmWork$", "--min-time=0.02"});
  const std::vector<std::string> grown = linesOf(calibrated, "bmWork");
  check(grown.size() == 1, "one repetition prints one measured row (FR-012)");
  const long long grownCount = countFieldOf(grown[0], "iterations=");
  check(grownCount > 1, "the calibration grew past one iteration (FR-014)");
  check(grownCount <= 1'000'000'000'000LL,
        "the growth stays inside the FR-016 bound");
  // SC-014: the row carries the plan's calibrated overhead floor.
  check(doubleFieldOf(grown[0], "overhead floor (ns)=") > 0.0,
        "the row carries the overhead floor (SC-014)");

  // FR-013: a fixed iteration count skips calibration.
  const std::string fixed =
      captureRun({"--filter", "^bmWork$", "--iterations=100"});
  check(countFieldOf(linesOf(fixed, "bmWork")[0], "iterations=") == 100,
        "the fixed iteration count reached the run (FR-013)");

  // FR-011: a warm-up time runs its own growing phase, and the measured
  // phase discards it and starts from its own count. One repetition means
  // one measured row, so a warm-up row would show as an extra line.
  const std::string warmed = captureRun(
      {"--filter", "^bmWork$", "--warmup-time=0.02", "--iterations=100"});
  const std::vector<std::string> warmedRows = linesOf(warmed, "bmWork");
  check(warmedRows.size() == 1,
        "the warm-up results are discarded from the report (FR-011)");
  check(countFieldOf(warmedRows[0], "iterations=") == 100,
        "the measured phase starts from its own count (FR-011)");

  // FR-012: the repetition count is the number of measured runs.
  const std::string repeated = captureRun(
      {"--filter", "^bmWork$", "--iterations=20", "--repetitions=3"});
  const std::vector<std::string> repeatedRows = linesOf(repeated, "bmWork");
  check(repeatedRows.size() == 4,
        "three repetitions print three rows plus the aggregate (FR-012)");

  // FR-014: a dry run takes one iteration, one repetition, and no warm-up.
  const std::string dry =
      captureRun({"--filter", "^bmWork$", "--dry-run", "--warmup-time=0.02"});
  const std::vector<std::string> dryRows = linesOf(dry, "bmWork");
  check(dryRows.size() == 1, "the dry run reports one row (FR-014)");
  check(countFieldOf(dryRows[0], "iterations=") == 1,
        "the dry run takes one iteration (FR-014)");

  std::puts("harness_calibration_test: ok");
  return 0;
}
