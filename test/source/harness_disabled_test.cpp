// ============================================================================
// TDD test for the DISABLED_ prefix (T041; US6, capability C-9; FR-020,
// FR-021).
//
// One DISABLED_ family stands for the prefix rule of FR-020: it registers,
// never runs, stays out of the filter match, and stays out of list mode,
// while a non-disabled sibling registered in the same process keeps running.
// The readings are the counters the bodies keep, the lines of list mode, and
// the rows of one run, and - the way harness_instance_name_test.cpp reads
// the FR-012 duplicate - the exit status and the two streams of a re-exec of
// this binary, the two streams going to two files: a filter naming only the
// DISABLED_ family, and a process whose sole registration is disabled, both
// saying so on the standard error stream and exiting zero as H1 FR-036
// fixes it. A fixture method named DISABLED_x runs, because that instance
// name starts with its fixture class and not with the prefix (FR-020,
// R-11). A scripted FakeProvider owns the machine time leaves, so no run
// grows into a long calibration. Hand-computed expectations, frameworkless
// check()/fail() convention.
// ============================================================================

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
  std::fprintf(stderr, "HARNESS DISABLED TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_disabled_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_disabled_out.txt";
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

// The count each body keeps is the reading of the scenario: the DISABLED_
// body has to stay at zero, the sibling, the fixture method, and the family
// whose name merely carries the prefix in the middle all have to move.
int disabledRuns = 0;
int siblingRuns = 0;
int fixtureDisabledRuns = 0;
int midNameRuns = 0;

auto bmDisabled(sg::State& state) -> void
{
  ++disabledRuns;
  for (auto _ : state) {
  }
}

auto bmQuick(sg::State& state) -> void
{
  ++siblingRuns;
  for (auto _ : state) {
  }
}

// A plain family whose name carries DISABLED_ in the middle of the name,
// past the prefix: the prefix test of FR-020 is a prefix test, so this
// family runs.
auto bmDISABLEDx(sg::State& state) -> void
{
  ++midNameRuns;
  for (auto _ : state) {
  }
}

// The fixture whose method carries the DISABLED_ name, which the prefix
// never reaches because the instance name starts with the class (FR-020,
// R-11). The method is defined here and registered only in the parent
// process, so a re-exec child can hold nothing but disabled registrations.
class DisabledFixture : public sg::Fixture
{
};

SG_BENCHMARK_DEFINE_F(DisabledFixture, DISABLED_x)

(sg::State& state)
{
  ++fixtureDisabledRuns;
  for (auto _ : state) {
  }
}

auto registerDisabledFixture() -> void
{
  SG_BENCHMARK_REGISTER_F(DisabledFixture, DISABLED_x);
}

// FR-020: a family registered as DISABLED_slow registers without error and,
// with no filter, its function never runs, while the sibling registered in
// the same process does run. The zero exit status captureRun asserts is the
// registration drawing no error.
auto disabledNeverRunsScenario() -> void
{
  (void)sg::registerBenchmark(&bmDisabled, "DISABLED_slow");
  (void)sg::registerBenchmark(&bmQuick, "bmQuick");

  captureRun({"--iterations=1"});
  check(disabledRuns == 0,
        "a DISABLED_ family with no filter runs no function (FR-020)");
  check(siblingRuns > 0,
        "a non-disabled sibling in the same process runs (FR-020)");
}

// FR-020: list mode keeps the DISABLED_ family out of the printed set and
// still prints the sibling.
auto listExcludesScenario() -> void
{
  const std::string listed = captureRun({"--list"});
  check(linesOf(listed, "DISABLED_").empty(),
        "list mode prints no name starting with DISABLED_ (FR-020)");
  check(lineOf(listed, "bmQuick") == "bmQuick",
        "list mode still prints the non-disabled sibling (FR-020)");
}

// FR-020, R-11: a fixture method named DISABLED_x runs, because the
// instance name starts with the fixture class and not with the prefix, and
// list mode prints that name.
auto fixtureScenario() -> void
{
  registerDisabledFixture();

  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "DisabledFixture/DISABLED_x")
            == "DisabledFixture/DISABLED_x",
        "list mode prints the fixture instance FixtureClass/DISABLED_x "
        "(FR-020, R-11)");

  const int before = fixtureDisabledRuns;
  captureRun({"--filter", "^DisabledFixture/DISABLED_x$", "--iterations=1"});
  check(fixtureDisabledRuns - before == 1,
        "a fixture method named DISABLED_x runs (FR-020, R-11)");
}

// FR-020: the prefix test is a prefix test, so a plain family whose name
// merely carries DISABLED_ in the middle runs normally.
auto midNameScenario() -> void
{
  (void)sg::registerBenchmark(&bmDISABLEDx, "bmDISABLEDx");

  const int before = midNameRuns;
  const std::string run = captureRun({"--iterations=1"});
  check(midNameRuns - before == 1,
        "a family whose name only carries DISABLED_ in the middle runs "
        "(FR-020)");
  check(!linesOf(run, "bmDISABLEDx").empty(),
        "that family reports its row like any other family (FR-020)");
}

// FR-020, FR-036: a filter naming only the DISABLED_ family matches nothing
// and runs nothing; the executable says so on the standard error stream and
// exits zero, read from a re-exec whose two streams go to two files.
auto filterNoMatchScenario(const std::string& self) -> void
{
  const int status =
      std::system(("\"" + self + "\" filter" + " >harness_disabled_run.txt"
                   + " 2>harness_disabled_err.txt")
                      .c_str());
  check(status == 0,
        "a filter naming only a DISABLED_ family exits zero (FR-036)");

  const std::string error = readFile("harness_disabled_err.txt");
  check(error.find("no benchmark matches") != std::string::npos,
        "the executable says the filter matched nothing on the standard "
        "error stream (FR-036)");

  const std::string run = readFile("harness_disabled_run.txt");
  check(
      linesOf(run, "DISABLED_slow").empty(),
      "the filter naming only a DISABLED_ family runs no function " "(FR-020)");
  check(linesOf(run, "bmQuick").empty(),
        "and the run reports no other row either (FR-020)");
}

// FR-020, FR-036: a DISABLED_ family as the only registration is an empty
// selection: exit status zero, one no-match line on the standard error
// stream, nothing on the standard output stream.
auto onlyDisabledScenario(const std::string& self) -> void
{
  const int status = std::system(("\"" + self + "\" only-disabled"
                                  + " >harness_disabled_only_run.txt"
                                  + " 2>harness_disabled_only_err.txt")
                                     .c_str());
  check(status == 0,
        "a process whose sole registration is disabled exits zero "
        "(FR-020, FR-036)");

  const std::string error = readFile("harness_disabled_only_err.txt");
  check(nonEmptyLineCount(error) == 1,
        "it draws one no-match line on the standard error stream (FR-036)");
  check(error.find("no benchmark matches") != std::string::npos,
        "that line says no benchmark matches (FR-036)");

  const std::string run = readFile("harness_disabled_only_run.txt");
  check(run.empty(),
        "it prints nothing on the standard output stream (FR-020)");
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self = argc > 0 ? argv[0] : "harness_disabled_test";
  const std::string mode = argc > 1 ? argv[1] : "";

  // The two re-execs: the registrations a child owns, in a process whose
  // two streams the parent can separate into two files. The fixture never
  // enters either, so the only-disabled child holds nothing but the
  // DISABLED_ family.
  if (mode == "filter") {
    scriptedProvider();
    (void)sg::registerBenchmark(&bmDisabled, "DISABLED_slow");
    (void)sg::registerBenchmark(&bmQuick, "bmQuick");
    const std::vector<std::string> arguments = {
        "--filter", "^DISABLED_slow$", "--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  if (mode == "only-disabled") {
    scriptedProvider();
    (void)sg::registerBenchmark(&bmDisabled, "DISABLED_slow");
    const std::vector<std::string> arguments = {"--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  scriptedProvider();

  disabledNeverRunsScenario();
  listExcludesScenario();
  fixtureScenario();
  midNameScenario();
  filterNoMatchScenario(self);
  onlyDisabledScenario(self);

  std::puts("harness_disabled_test: ok");
  return 0;
}
