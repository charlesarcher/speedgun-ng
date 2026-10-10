// ============================================================================
// TDD test for the SG_BENCHMARK_CAPTURE registration macro (T022; US3,
// capability C-4; FR-012, FR-013).
//
// The macro names an instance by the capture name alone: the family name
// plus one '/'-joined segment that is the capture name, with no segment per
// captured argument (FR-013). The names are read back through list mode,
// which prints one name per line and runs no benchmark function, and
// through the rows of one filtered run. The captured values are read back
// through a value the body accumulates and the test states: the body folds
// the two captured ints into a sum weighted by state.iterations(), so a
// wrong capture changes the reported number and the hand-computed sum of 2
// and 2 is the expectation. The FR-012 duplicate is read from the standard
// error of a re-exec of this binary with one argument, the way
// harness_instance_name_test.cpp reads it: the two streams go to two files.
// A scripted FakeProvider owns the machine time leaves, so no run grows
// into a long calibration. Hand-computed expectations, frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CAPTURE MACRO TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The body of the capture cases. Its count is the number of instances a
// selection reached, and list mode has to leave it at zero.
int instanceRuns = 0;

// The sum the addTwo bodies accumulated across the runs this test performs.
// Unsigned because state.iterations() is unsigned, so the accumulation in
// the body needs no signed conversion (-Wsign-conversion).
std::uint64_t captureSum = 0;
int capChainCaptured = 0;
int capChainRange0 = -1;

auto addTwo(sg::State& state, int a, int b) -> void
{
  ++instanceRuns;
  for (auto _ : state) {
  }
  // The captured values, weighted by the iterations the run
  // performed, accumulated into a value this file reads back. A capture
  // that wired the wrong values here would change the sum.
  captureSum += state.iterations() * static_cast<std::uint64_t>(a + b);
}

// The body of the earlier registration of a clashing pair. The two bodies
// have to differ: with one body for both registrations, the mark a run
// leaves says nothing about which instance survived, and "the earlier one
// stays" cannot be read from the report at all.
auto bmDupEarlier(sg::State& state) -> void
{
  std::fputs("bmDupEarlierRan\n", stdout);
  for (auto _ : state) {
  }
}

// The body of the later registration of a clashing pair. FR-012 owes that
// the instance never runs, and only a body of its own can say so: it marks
// the report with its name, and the mark staying absent is the reading.
auto bmDupLater(sg::State& state) -> void
{
  std::fputs("bmDupLaterRan\n", stdout);
  for (auto _ : state) {
  }
}

// The body of the two-captures-on-one-function case: one function, two
// capture names, so two instances.
auto fn(sg::State& state) -> void
{
  ++instanceRuns;
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_capture_macro_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_capture_macro_out.txt";
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
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The body of the chained-site case: one captured value and one family
// argument chained at the macro site (FR-013).
auto bmCapChain(sg::State& state, int captured) -> void
{
  capChainCaptured = captured;
  capChainRange0 = static_cast<int>(state.range(0));
  ++instanceRuns;
  for (auto _ : state) {
  }
}

auto readFile(const char* path) -> std::string
{
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

auto lineOf(const std::string& report, const std::string& name) -> std::string
{
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0) {
      return line;
    }
  }
  return {};
}

// The instance names of one function, in the order list mode printed them:
// the function name plus the '/'-joined capture-name segment (FR-013).
auto namesUnder(const std::string& report,
                const std::string& family) -> std::vector<std::string>
{
  std::vector<std::string> found;
  for (const std::string& line : linesOf(report, family)) {
    if (line.size() == family.size() || line[family.size()] == '/') {
      found.push_back(line);
    }
  }
  return found;
}

auto nonEmptyLineCount(const std::string& text) -> int
{
  int lines = 0;
  std::istringstream stream(text);
  for (std::string line; std::getline(stream, line);) {
    if (!line.empty()) {
      ++lines;
    }
  }
  return lines;
}

auto scriptedProvider() -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  provider->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider->setPoints("machine", "monotonic", {0}, 4000);
  provider->setPoints("machine", "thread_cpu", {0}, 2000);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the fake provider registers");
}

// FR-013: one capture registers exactly one instance, named by the capture
// name as the name segment. The captured arguments draw no segment of
// their own, so the name is the function name and the capture name and
// nothing else.
auto captureNameScenario() -> void
{
  SG_BENCHMARK_CAPTURE(addTwo, pair, 2, 2);

  const std::string listed = captureRun({"--list"});
  check(instanceRuns == 0, "list mode runs no benchmark function (FR-011)");
  check(
      namesUnder(listed, "addTwo") == std::vector<std::string> {"addTwo/pair"},
      "one capture registers exactly one instance named fn/capture (FR-013)");
  check(lineOf(listed, "addTwo/pair") == "addTwo/pair",
        "the capture name is the name segment (FR-013)");
  for (const char* const segment : {"addTwo/pair/2", "addTwo/2", "addTwo/2/2"})
  {
    check(linesOf(listed, segment).empty(),
          "the captured arguments draw no segment of their own (FR-013)");
  }
}

// The captured values reach the function: the body folds them into a sum
// weighted by state.iterations(), and the hand-computed sum of 2 and 2 is
// what the test reads back. A wrong capture changes the number.
auto capturedValuesScenario() -> void
{
  const int before = instanceRuns;
  const std::string run =
      captureRun({"--filter", "^addTwo/pair$", "--iterations=1"});
  check(linesOf(run, "addTwo/pair").size() == 1,
        "the capture reports one row (FR-011)");
  check(instanceRuns - before == 1, "the filtered run reaches one instance");
  check(captureSum == 4ULL,
        "the captured values reach the function: the reported sum is the "
        "hand-computed 2 + 2");
}

// FR-013: two captures under two names on one function give two
// instances, each named by its own capture name.
auto twoCapturesScenario() -> void
{
  SG_BENCHMARK_CAPTURE(fn, captureA);
  SG_BENCHMARK_CAPTURE(fn, captureB);

  const std::string listed = captureRun({"--list"});
  check(namesUnder(listed, "fn") ==
            std::vector<std::string> {"fn/captureA", "fn/captureB"},
        "two captures on one function give two instances named fn/capture "
        "(FR-013)");
}

// FR-012: the clash of a capture name with an existing instance name, read
// from the standard error of a re-exec of this binary. The family bmDup
// with the argument 8 and the family bmDup/8 with no family call expand to
// the one name bmDup/8, so the line that names both names names that
// string.
auto duplicateScenario(const std::string& self) -> void
{
  const int status = std::system(("\"" + self + "\" duplicate"
                                  + " >harness_capture_macro_run.txt"
                                  + " 2>harness_capture_macro_err.txt")
                                     .c_str());
  check(status == 0,
        "a duplicate instance name leaves the exit status as a normal run "
        "carries it (FR-012)");

  const std::string error = readFile("harness_capture_macro_err.txt");
  check(nonEmptyLineCount(error) == 1,
        "a duplicate instance name draws one line on the standard error "
        "stream (FR-012)");
  check(error.find("bmDup/8") != std::string::npos,
        "that line names the clashing instance name (FR-012)");

  const std::string run = readFile("harness_capture_macro_run.txt");
  check(linesOf(run, "bmDup/8").size() == 1,
        "the earlier instance stays and the later is dropped (FR-012)");
  check(run.find("bmDupEarlierRan") != std::string::npos,
        "the instance that stays is the earlier registration (FR-012)");
  check(run.find("bmDupLaterRan") == std::string::npos,
        "the later instance never reaches its function (FR-012)");
  check(linesOf(run, "bmOther/").size() == 1,
        "every other instance still runs (FR-012)");
}

// FR-013: the chained capture site runs one instance named by the
// capture, and the body reads both the captured value and the family
// argument.
auto chainedCaptureScenario() -> void
{
  SG_BENCHMARK_CAPTURE(bmCapChain, pair, 2).arg(4);

  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "bmCapChain/pair/4") == "bmCapChain/pair/4",
        "the chained capture site names bmCapChain/pair/4 (FR-013)");
  const int before = instanceRuns;
  const std::string run =
      captureRun({"--filter", "^bmCapChain/pair/4$", "--iterations=1"});
  check(linesOf(run, "bmCapChain/pair/4").size() == 1,
        "the chained capture site runs one instance (FR-013)");
  check(instanceRuns - before == 1, "the run reaches the body (FR-013)");
  check(capChainCaptured == 2 && capChainRange0 == 4,
        "the body reads the captured value and range(0) (FR-013)");
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self = argc > 0 ? argv[0] : "harness_capture_macro_test";
  const std::string mode = argc > 1 ? argv[1] : "";

  // The FR-012 re-exec: the two registrations whose names clash - the
  // family bmDup with the argument 8, and the family bmDup/8 with no
  // family call - plus one instance that has to keep running, in a process
  // whose two streams the parent can separate into two files.
  if (mode == "duplicate") {
    scriptedProvider();
    auto kept = sg::registerBenchmark(&bmDupEarlier, "bmDup");
    kept.arg(8);
    (void)sg::registerBenchmark(&bmDupLater, "bmDup/8");
    auto other = sg::registerBenchmark(&bmDupEarlier, "bmOther");
    other.args({1, 2});
    const std::vector<std::string> arguments = {"--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  scriptedProvider();

  captureNameScenario();
  capturedValuesScenario();
  twoCapturesScenario();
  chainedCaptureScenario();
  duplicateScenario(self);

  std::puts("harness_capture_macro_test: ok");
  return 0;
}
