// ============================================================================
// TDD test for the command line (T020; US2, FR-009, FR-015, FR-021, FR-022,
// FR-034, FR-036, SC-007).
//
// A scripted FakeProvider owns the machine time leaves and one events leaf,
// so the option effects are read as exact numbers in the captured report and
// no run can grow into a long calibration. The usage-error cases exit the
// process from inside the entry point, so each is checked in a re-exec of
// this binary with a mode argument. Frameworkless check()/fail() convention.
// ============================================================================

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CLI TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::abs(lhs - rhs) < 1e-9;
}

int alphaRuns = 0;

auto bmAlpha(sg::State& state) -> void
{
  ++alphaRuns;
  for (auto _ : state) {
  }
}

auto bmBeta(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmSkips(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  state.skipWithMessage("the scripted reason");
}

auto bmThrows(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  throw std::runtime_error("the scripted failure");
}

int interruptRuns = 0;

auto bmInterrupts(sg::State& state) -> void
{
  ++interruptRuns;
  for (auto _ : state) {
  }
  (void)std::raise(SIGINT);
}

auto bmAfter(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The counts SC-006 compares around the failed runs. The listing holds one
// descriptor of its own while it reads, in both measurements, so the
// difference is what the comparison sees.
auto openDescriptorCount() -> std::size_t
{
  std::error_code failure;
  std::size_t count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator("/proc/self/fd", failure))
  {
    static_cast<void>(entry);
    ++count;
  }
  if (failure) {
    fail("the process's own descriptor listing is readable");
  }
  return count;
}

auto mappingCount() -> std::size_t
{
  std::ifstream maps("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_cli_out.txt";
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

auto subprocessOutput(const std::string& command, std::string& output) -> int
{
  FILE* const pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    fail("the usage subprocess starts");
  }
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  return pclose(pipe);
}

auto scriptedProvider() -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  provider->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider->setPoints("machine", "monotonic", {0}, 4000);
  provider->setPoints("machine", "thread_cpu", {0}, 2000);
  provider->addObject("fake", "fake", "scripted events object");
  provider->addCounter("fake", "events", "none", "scripted events");
  provider->setPoints("fake", "events", {0}, 900);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the fake provider registers");
}

}  // namespace

SG_BENCHMARK(bmAlpha)
SG_BENCHMARK(bmBeta)
SG_BENCHMARK(bmSkips)
SG_BENCHMARK(bmThrows)
SG_BENCHMARK(bmInterrupts)
SG_BENCHMARK(bmAfter)

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "arg" && argc > 2) {
    scriptedProvider();
    const std::vector<std::string> arguments = {
        "--filter", "^bmAlpha$", "--iterations=1", argv[2]};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    const int status =
        sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
    // A usage error exits from inside the entry point, so a missing
    // marker is itself the observation that no benchmark ran.
    std::fprintf(
        stderr, "%s\n", alphaRuns > 0 ? "BENCHMARK_RAN" : "BENCHMARK_NO_RUN");
    return status;
  }

  scriptedProvider();
  auto fixedHandle = sg::registerBenchmark(&bmFixed, "bmFixed");
  fixedHandle.iterations(7);

  // FR-009: the filter selects the matching set, and the run reports only
  // the selected benchmarks.
  const std::string filtered =
      captureRun({"--filter", "^bmAlpha$", "--iterations=2"});
  check(linesOf(filtered, "bmAlpha").size() == 1,
        "the filter runs the matching benchmark (FR-009)");
  check(linesOf(filtered, "bmBeta").empty(),
        "the filter leaves the unmatched benchmark out (FR-009)");

  // FR-036: a filter that matches nothing reports and exits zero.
  const std::string unmatched = captureRun({"--filter", "^nothingHere$"}, 0);
  check(unmatched.find("bmAlpha") == std::string::npos
            && unmatched.find("bmBeta") == std::string::npos,
        "a filter matching nothing runs no benchmark (FR-036)");

  // FR-009: list mode prints the matching names and runs nothing.
  const std::string listed = captureRun({"--filter", "^bm", "--list"});
  check(linesOf(listed, "bmAlpha").size() == 1
            && linesOf(listed, "bmBeta").size() == 1,
        "list mode prints the matching names (FR-009)");
  check(listed.find("time/iter") == std::string::npos,
        "list mode runs nothing (FR-009)");

  // FR-015: a handle option wins over the command line.
  const std::string handleWins =
      captureRun({"--filter", "^bmFixed$", "--iterations=3"});
  check(countFieldOf(linesOf(handleWins, "bmFixed")[0], "iterations=") == 7,
        "the handle option wins over the command line (FR-015)");

  // FR-012: the repetition count is the number of measured rows.
  const std::string repeated = captureRun(
      {"--filter", "^bmAlpha$", "--iterations=2", "--repetitions=4"});
  check(linesOf(repeated, "bmAlpha").size() == 5,
        "the repetition count drives the row count (FR-012)");

  // FR-021 and FR-022: an address-attached leaf reaches the run, and an
  // events^1 leaf reports per iteration.
  const std::string counter = captureRun(
      {"--filter", "^bmAlpha$", "--iterations=3", "--counter", "fake/events"});
  const std::string counterLine = linesOf(counter, "bmAlpha")[0];
  check(counterLine.find("fake/events=") != std::string::npos,
        "the address-attached leaf reaches the run (FR-021)");
  check(sameDouble(doubleFieldOf(counterLine, "fake/events="), 900.0 / 3.0),
        "the events^1 leaf reports per iteration (FR-022)");

  // FR-034, SC-018: every invalid value of every numeric option fails
  // before any benchmark runs, and the report names the option. A zero
  // warm-up time is the valid case of the same table, and the unknown
  // `--counter` leaf is the FR-023 edge case: it names the leaf and the
  // benchmark still runs.
  struct UsageCase
  {
    const char* argument;
    const char* needle;
    bool runs;
  };

  const std::string self = argv[0];
  for (const UsageCase& testCase :
       {UsageCase {"--repetitions=abc", "--repetitions", false},
        UsageCase {"--repetitions=-1", "--repetitions", false},
        UsageCase {"--repetitions=0", "--repetitions", false},
        UsageCase {"--iterations=abc", "--iterations", false},
        UsageCase {"--iterations=-5", "--iterations", false},
        UsageCase {"--iterations=0", "--iterations", false},
        UsageCase {"--min-time=abc", "--min-time", false},
        UsageCase {"--min-time=-1", "--min-time", false},
        UsageCase {"--min-time=0", "--min-time", false},
        UsageCase {"--warmup-time=abc", "--warmup-time", false},
        UsageCase {"--warmup-time=-1", "--warmup-time", false},
        UsageCase {"--nonsense=1", "unknown option", false},
        UsageCase {"--warmup-time=0", "", true},
        UsageCase {"--counter=machine/nosuchleaf", "no such leaf", true}})
  {
    std::string output;
    const int status = subprocessOutput(
        "\"" + self + "\" arg \"" + testCase.argument + "\" 2>&1", output);
    if (testCase.runs) {
      check(status == 0, "a valid value exits zero (SC-018, FR-023)");
    } else {
      check(status != 0, "an invalid value of FR-034 exits nonzero (SC-018)");
    }
    check((output.find("BENCHMARK_RAN") != std::string::npos)
              == testCase.runs,
          "the invalid value runs no benchmark function and the valid one "
          "does (SC-018, FR-023)");
    if (testCase.needle[0] != '\0') {
      check(output.find(testCase.needle) != std::string::npos,
            "the report names the rejected option or leaf (FR-034, FR-023)");
    }
  }

  // FR-031: a skip prints its reason, and no statistics print.
  const std::string skipped = captureRun(
      {"--filter", "^bmSkips$", "--iterations=1", "--repetitions=3"});
  check(skipped.find("SKIPPED: the scripted reason") != std::string::npos,
        "the skip prints its reason (FR-031)");
  check(skipped.find("AGGREGATE") == std::string::npos,
        "a skipped benchmark prints no statistics (FR-031)");

  // FR-032, SC-006: a throw reports the exception text, releases every
  // descriptor and mapping, and the run continues. The leaf of the real
  // pmu provider is what opens a descriptor per run, so the failed runs
  // are counted against it.
  const std::vector<std::string> failing = {"--filter",
                                            "^bmThrows$",
                                            "--iterations=1",
                                            "--counter",
                                            "cpu/instructions"};
  const std::string thrown = captureRun(failing, 1);
  check(thrown.find("FAILED: the scripted failure") != std::string::npos,
        "the failure reports the exception text (FR-032)");

  captureRun(failing, 1);
  const std::size_t descriptors = openDescriptorCount();
  const std::size_t mappings = mappingCount();
  for (int run = 0; run < 1000; ++run) {
    captureRun(failing, 1);
  }
  check(openDescriptorCount() == descriptors,
        "1,000 failed runs return the descriptor count (SC-006)");
  check(mappingCount() == mappings,
        "1,000 failed runs return the mapping count (SC-006)");

  const std::string after =
      captureRun({"--filter", "^bmAfter$", "--iterations=1"});
  check(after.find("iterations=") != std::string::npos,
        "the next benchmark runs after a failure (FR-032)");

  // SC-017: SIGINT inside the function completes the current run, starts
  // no later run, reports skipped with the interrupt reason, releases the
  // same resources, and exits nonzero.
  const std::string interrupted = captureRun(
      {"--filter", "^bmInterrupts$", "--iterations=1", "--repetitions=3"}, 1);
  check(interrupted.find("SKIPPED: interrupted") != std::string::npos,
        "the interrupt reports skipped with its reason (SC-017)");
  check(interruptRuns == 1,
        "the current run completes and no later run starts (SC-017)");
  check(openDescriptorCount() == descriptors && mappingCount() == mappings,
        "the interrupted run releases the same resources (SC-017)");

  // IF-07, FR-032: the flag is read after every run, the qualifying
  // warm-up run included, so no measured run starts behind it.
  interruptRuns = 0;
  const std::string warmInterrupted = captureRun({"--filter",
                                                  "^bmInterrupts$",
                                                  "--iterations=1",
                                                  "--warmup-time=0.000000001"},
                                                 1);
  check(warmInterrupted.find("SKIPPED: interrupted") != std::string::npos,
        "the warm-up interrupt reports skipped with its reason (FR-032)");
  check(interruptRuns == 1,
        "the flag read after the warm-up run stops the measured run (FR-032)");

  std::puts("harness_cli_test: ok");
  return 0;
}
