// ============================================================================
// TDD test for manual time (T017; US3, FR-006, FR-007, FR-008, SC-001).
//
// The benchmark hands the state one nanosecond count per iteration and the
// harness sums them, so every figure here is the scripted sum over the
// iteration count the run carried:
//
//   report, 8 iterations at 1000 ns each: the sum 8000 over 8 is 1000.000
//     the scripted clocks step 7000 (monotonic) and 300 (thread CPU), so
//     the real column can only be the accepted sum, and the CPU column
//     only the scripted clock fold, 300 over 8 is 37.500
//   growth, minimum time 8000 ns, one accepted 1000 ns per iteration:
//     run 0: N=1,  sum 1000  -> factor 11.2 -> N=11
//     run 1: N=11, sum 11000 -> qualifies
//   fivefold exemption, minimum time 1000 ns, one accepted 1 ns:
//     the scripted monotonic step is 5000, five times the target, the step
//     that qualifies the walk of harness_calibration_test.cpp:361-390 at
//     once. A manual walk reads the accepted sum and grows by ten:
//     run 0: N=1,    sum 1    -> N=10
//     run 1: N=10,   sum 10   -> N=100
//     run 2: N=100,  sum 100  -> N=1000
//     run 3: N=1000, sum 1000 -> qualifies
//   empty: a manual benchmark that accepts nothing reports 0.000, and the
//     rule that reads the sum grows to the iteration cap and qualifies
//     there, the rule-level shape of harness_growth_test.cpp:78-95
//
// The script stands on the provider's sampling-action index, and one plan
// consumes its own sampling-cost calibration before the benchmark's first
// endpoint; a fixed-N probe names that length, so no constant here stands
// for it.
//
// FR-008: the second of useRealTime and useManualTime is a precondition
// violation, so each order runs in a re-exec of this binary and the parent
// reads the signal, the pattern of harness_callback_test.cpp:593-621.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/calibration.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The scripted clock steps of the manual scenarios. The monotonic step of
// the fivefold scenario is five times its target, the value the stop reads
// for a run that is not manual.
constexpr std::uint64_t kManualMonotonicStep = 5000;
constexpr std::uint64_t kManualThreadCpuStep = 100;

sg::counters::FakeProvider* fake = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS MANUAL TIME TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// One accepted 1000 ns per iteration.
auto bmManual(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1000);
  }
}

// One accepted 1 ns per iteration, the count the fivefold scenario grows on.
auto bmManualOne(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1);
  }
}

// Manual time selected and nothing accepted, so the sum stays zero.
auto bmManualEmpty(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  const int saved = ::dup(STDOUT_FILENO);
  if (saved < 0
      || std::freopen("harness_manual_time_out.txt", "w", stdout) == nullptr)
  {
    fail("the report redirect opened");
  }
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_manual_time_test"));
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

  std::ifstream file("harness_manual_time_out.txt");
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
// the action arithmetic of the scenarios counts against.
auto probePlanActions(const std::string& name) -> std::uint64_t
{
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^" + name + "$", "--iterations=1"});
  const std::uint64_t consumed = fake->readActions() - before;
  check(consumed > 2, "the probe run carries the plan's calibration");
  return consumed - 2;
}

// FR-007: the accepted sum, and that sum alone, reaches the real column.
auto scenarioReport() -> int
{
  scriptedProvider(7000, 300);
  auto handle = sg::registerBenchmark(&bmManual, "bmReport");
  handle.useManualTime();

  const std::string report =
      captureRun({"--filter", "^bmReport$", "--iterations=8"});
  const std::vector<std::string> rows = linesOf(report, "bmReport");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");
  check(countFieldOf(rows[0], "iterations=") == 8,
        "the fixed count reached the run (FR-013)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="), 1000.0),
        "the real column carries the accepted sum over N (FR-007)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="), 37.5),
        "the CPU column keeps the scripted clock fold (FR-001)");
  std::puts("harness_manual_time_test report: ok");
  return 0;
}

// FR-006: the growth rule reads the accepted sum. The scripted clocks stand
// far from that sum, so the walk length names the value the rule read.
auto scenarioGrowth() -> int
{
  scriptedProvider(kManualMonotonicStep, kManualThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmManual, "bmGrown");
  handle.useManualTime();
  const std::uint64_t planActions = probePlanActions("bmGrown");

  const std::uint64_t before = fake->readActions();
  const std::string report =
      captureRun({"--filter", "^bmGrown$", "--min-time=0.000008"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmGrown");
  check(rows.size() == 1, "the walk prints one measured row (FR-012)");
  check(countFieldOf(rows[0], "iterations=") == 11,
        "the sum of 1000 per iteration grew the count to 11 (FR-006)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="), 1000.0),
        "the qualifying run reports its own sum over N (FR-007)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="), 100.0 / 11.0),
        "the CPU column still folds the scripted leaf (FR-001)");
  check(consumed == planActions + 2 * 2,
        "two runs qualified on the accepted sum (FR-006)");
  std::puts("harness_manual_time_test growth: ok");
  return 0;
}

// FR-006: the fivefold real-time stop leaves a manual run alone. The
// scripted monotonic step is five times the target, the step that ends the
// walk of harness_calibration_test.cpp:361-390 at one iteration.
auto scenarioFivefold() -> int
{
  scriptedProvider(kManualMonotonicStep, kManualThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmManualOne, "bmFivefold");
  handle.useManualTime();
  const std::uint64_t planActions = probePlanActions("bmFivefold");

  const std::uint64_t before = fake->readActions();
  const std::string report =
      captureRun({"--filter", "^bmFivefold$", "--min-time=0.000001"});
  const std::uint64_t consumed = fake->readActions() - before;

  const std::vector<std::string> rows = linesOf(report, "bmFivefold");
  check(rows.size() == 1, "the walk prints one measured row (FR-012)");
  check(countFieldOf(rows[0], "iterations=") == 1000,
        "the manual walk grew by ten past the fivefold step (FR-006)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="), 1.0),
        "one accepted nanosecond per iteration reports 1.000 (FR-007)");
  check(consumed == planActions + 2 * 4,
        "four runs grew on the accepted sum (FR-006)");
  std::puts("harness_manual_time_test fivefold: ok");
  return 0;
}

// The spec edge case, report half: a manual benchmark that accepts nothing
// reports zero in the real column.
auto scenarioEmpty() -> int
{
  scriptedProvider(kManualMonotonicStep, kManualThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmManualEmpty, "bmEmpty");
  handle.useManualTime();

  const std::string report =
      captureRun({"--filter", "^bmEmpty$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmEmpty");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");
  check(countFieldOf(rows[0], "iterations=") == 4,
        "the fixed count reached the run (FR-013)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="), 0.0),
        "a run that accepted nothing reports zero (FR-007)");
  std::puts("harness_manual_time_test empty: ok");
  return 0;
}

// FR-002, FR-006: with more than one repetition the aggregate row
// summarizes the primary time. FR-006 makes the primary time of a
// manual run the accepted sum. The scripted thread CPU step of
// 300 over four iterations answers 75.000, so the aggregate separates
// the two candidates by the figures it prints.
auto scenarioAggregate() -> int
{
  scriptedProvider(7000, 300);
  auto handle = sg::registerBenchmark(&bmManual, "bmAgg");
  handle.useManualTime();

  const std::string report =
      captureRun({"--filter", "^bmAgg$", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> rows = linesOf(report, "bmAgg");
  check(rows.size() == 4,
        "three repetitions and the aggregate print four rows (FR-012)");
  for (std::size_t index = 0; index < 3; ++index) {
    check(closeTo(doubleFieldOf(rows[index], "real time/iter (ns)="), 1000.0),
          "each repetition reports the accepted sum over N (FR-007)");
    check(closeTo(doubleFieldOf(rows[index], "cpu time/iter (ns)="), 75.0),
          "each repetition keeps the scripted CPU fold (FR-001)");
  }
  const std::string& aggregate = rows[3];
  check(aggregate.find("AGGREGATE") != std::string::npos,
        "the fourth row is the aggregate (FR-018)");
  check(countFieldOf(aggregate, " n=") == 3,
        "the aggregate carries one sample per repetition (FR-018)");
  check(closeTo(doubleFieldOf(aggregate, "mean="), 1000.0),
        "the aggregate summarizes the accepted manual sum\n"
        "  (FR-002, FR-006; contracts/report-columns.md)");
  check(closeTo(doubleFieldOf(aggregate, "median="), 1000.0),
        "the median stands on the same sum (FR-006)");
  std::puts("harness_manual_time_test aggregate: ok");
  return 0;
}

// One sample callable of the growth rule: the record a manual run leaves
// behind. The accepted sum is zero, the manual flag stands, and the real
// field carries the fivefold value that qualifies any other run.
struct ManualSample
{
  std::uint64_t calls = 0;

  auto operator()(const std::uint64_t count)
      -> std::expected<sg::detail::RunRecord, std::string>
  {
    ++calls;
    auto record = sg::detail::RunRecord {};
    record.iterations = count;
    record.realNs = 5000;
    record.decisionNs = 0;
    record.manual = true;
    return record;
  }
};

// FR-006, rule level: the stop test reads the manual flag and skips the
// fivefold condition for a manual run.
auto fivefoldStaysExempt() -> void
{
  auto record = sg::detail::RunRecord {};
  record.iterations = 1;
  record.realNs = 5000;
  record.decisionNs = 10;
  record.manual = true;
  check(!sg::detail::qualifies(record, 1000, record.iterations),
        "a manual run at five times the target stays unqualified (FR-006)");
  record.manual = false;
  check(sg::detail::qualifies(record, 1000, record.iterations),
        "the same record qualifies when the run is not manual (FR-008)");
}

// FR-020, FR-046: the cap stands on the count one context ran. A threaded
// sample carries the sum of those counts, and that sum stays out of the test.
// The record of four contexts that each ran half the cap sums past the cap and
// sits below it per context, and each answer of the stop test is read here.
auto capStandsOnTheContextCount() -> void
{
  auto record = sg::detail::RunRecord {};
  record.iterations = 2 * sg::detail::kIterationCap;
  record.realNs = 0;
  record.decisionNs = 0;
  record.manual = true;
  check(!sg::detail::qualifies(record, 1000, sg::detail::kIterationCap / 2),
        "a threaded sample whose sum stands past the cap while each context "
        "sits below it stays unqualified (FR-020, FR-046)");
  check(sg::detail::qualifies(record, 1000, sg::detail::kIterationCap),
        "the context count that reaches the cap qualifies the run "
        "(FR-008, FR-016)");
}

// The spec edge case, rule half: with a sum of zero the growth rule keeps
// growing, and the run that reaches the cap is the run that qualifies.
auto emptySumGrowsToTheCap() -> void
{
  auto sample = ManualSample {};
  const auto grown =
      sg::detail::growUntilQualified(1,
                                     1000,
                                     sg::detail::kRunBound,
                                     sg::detail::PhaseText {"probe", "FR-016"},
                                     sample,
                                     []() noexcept -> bool { return false; });
  check(grown.outcome == sg::detail::GrowOutcome::QUALIFIED,
        "the walk that accepts nothing qualifies at the cap (FR-006)");
  check(sample.calls == 13, "the growth reaches the cap on run 13 (FR-010)");
  check(grown.record.iterations == sg::detail::kIterationCap,
        "the qualifying count is the FR-016 cap");
}

// FR-006 and FR-007 observed on the rule itself, with no provider behind it.
auto scenarioRule() -> int
{
  fivefoldStaysExempt();
  capStandsOnTheContextCount();
  emptySumGrowsToTheCap();
  std::puts("harness_manual_time_test rule: ok");
  return 0;
}

// FR-008: the second of the two selections is a precondition violation, so
// this process ends before it can report a status of its own.
auto modeRealThenManual() -> int
{
  auto handle = sg::registerBenchmark(&bmManualEmpty, "bmConflict");
  handle.useRealTime();
  handle.useManualTime();
  return 0;
}

auto modeManualThenReal() -> int
{
  auto handle = sg::registerBenchmark(&bmManualEmpty, "bmConflict");
  handle.useManualTime();
  handle.useRealTime();
  return 0;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "report") {
    return scenarioReport();
  }
  if (mode == "growth") {
    return scenarioGrowth();
  }
  if (mode == "fivefold") {
    return scenarioFivefold();
  }
  if (mode == "empty") {
    return scenarioEmpty();
  }
  if (mode == "aggregate") {
    return scenarioAggregate();
  }
  if (mode == "rule") {
    return scenarioRule();
  }
  if (mode == "real-then-manual") {
    return modeRealThenManual();
  }
  if (mode == "manual-then-real") {
    return modeManualThenReal();
  }

  // Each scripted scenario owns one provider script, so each runs in a
  // re-exec of this binary; a failed check exits there and shows here.
  const std::string self = argc > 0 ? argv[0] : "harness_manual_time_test";
  for (const auto& scenario :
       {"report", "growth", "fivefold", "empty", "aggregate", "rule"})
  {
    const int status =
        std::system(("\"" + self + "\" " + scenario + " 2>/dev/null").c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes");
  }

  // A shell reports a signaled child as its own exit 134, so the two
  // guard modes fork and exec directly, the pattern of
  // harness_callback_test.cpp:599, and the parent reads the signal from
  // waitpid (FR-008).
  for (const auto& guardMode : {"real-then-manual", "manual-then-real"}) {
    const pid_t pid = fork();
    if (pid < 0) {
      fail("cannot fork the guarded selection");
    }
    if (pid == 0) {
      if (std::freopen("/dev/null", "w", stderr) == nullptr) {
        _exit(2);
      }
      char* const execArgv[] = {const_cast<char*>(self.c_str()),
                                const_cast<char*>(guardMode),
                                nullptr};
      (void)execv(self.c_str(), execArgv);
      _exit(2);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
      fail("cannot wait for the guarded selection");
    }
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
          "the second of the two selections ends the re-exec (FR-008)");
  }
  std::puts("harness_manual_time_test: ok");
  return 0;
}
