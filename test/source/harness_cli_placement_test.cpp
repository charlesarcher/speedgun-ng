// ============================================================================
// TDD test for the placement option and the handle preset (T019; US3,
// FR-015, FR-016, FR-017, FR-036, contracts/cli-placement.md). prose-lint: allow reason="verbatim file name"
//
// A rejected option value exits the process from inside the entry point,
// the way the usage-error cases of harness_cli_test.cpp do, so every
// scenario runs in a re-exec of this binary with a mode argument, the
// child's standard output and standard error captured to files the parent
// reads after the wait.
//
// Scenario map:
//   badName             `--placement=l2` prints one line naming the
//                       rejected value and exits 2 with no benchmark run
//                       (FR-016)
//   cpuList             `--placement=0-3` prints one line naming the
//                       rejected value and exits 2 with no benchmark run
//                       (FR-017)
//   noArgument          `--placement` with no argument keeps the standing
//                       `getopt_long` missing-argument path and exit
//                       status 2 (contracts/cli-placement.md)
//   handleWins          a handle preset wins over the command-line
//                       preset: the fit line names the handle's preset
//                       (FR-015)
//   handleBadName       a handle name the system does not provide prints
//                       one line naming the rejected text and exits 2
//                       with no benchmark run (FR-016)
//   unknownFlag         an unknown flag keeps the standing line and
//                       status 2, the guard for behavior the audit-point
//                       tree already has
// ============================================================================

#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CLI PLACEMENT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmCliBadName(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliCpuList(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliNoArgument(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliHandleSmt(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliHandleBad(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliUnknownFlag(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// Each child registers exactly the benchmark its scenario filters for, so
// a stop line can only come from the input that scenario names (FR-015,
// FR-016).
auto setupBadName() -> void
{
  (void)sg::registerBenchmark(&bmCliBadName, "bmCliBadName");
}

auto setupCpuList() -> void
{
  (void)sg::registerBenchmark(&bmCliCpuList, "bmCliCpuList");
}

auto setupNoArgument() -> void
{
  (void)sg::registerBenchmark(&bmCliNoArgument, "bmCliNoArgument");
}

auto setupHandleWins() -> void
{
  auto handle = sg::registerBenchmark(&bmCliHandleSmt, "bmCliHandleSmt");
  handle.threads(4).placement("smt");
}

auto setupHandleBad() -> void
{
  auto handle = sg::registerBenchmark(&bmCliHandleBad, "bmCliHandleBad");
  handle.placement("l2");
}

auto setupUnknownFlag() -> void
{
  (void)sg::registerBenchmark(&bmCliUnknownFlag, "bmCliUnknownFlag");
}

auto readFile(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Fork a child that captures its two streams, registers the handle
// presets when `withHandles` stands, runs the entry point with
// `arguments`, and exits with its status; return the status and both
// captures. A rejection may exit from inside the entry point, so the
// work stands in a child either way.
struct Captured
{
  int status;
  std::string out;
  std::string err;
};

auto runChild(const char* mode, const std::vector<std::string>& arguments,
              void (*setup)()) -> Captured
{
  const std::string outPath = std::string("cli-placement-") + mode + "-out.txt";
  const std::string errPath = std::string("cli-placement-") + mode + "-err.txt";

  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    if (std::freopen(outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(errPath.c_str(), "w", stderr) == nullptr) {
      _exit(3);
    }
    if (setup != nullptr) {
      setup();
    }
    std::vector<std::string> argvStorage {"harness_cli_placement_test"};
    for (const auto& argument : arguments) {
      argvStorage.push_back(argument);
    }
    std::vector<char*> argv;
    for (auto& argument : argvStorage) {
      argv.push_back(argument.data());
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    (void)std::fflush(stdout);
    (void)std::fflush(stderr);
    _exit(status);
  }
  int waitStatus = 0;
  if (waitpid(pid, &waitStatus, 0) < 0) {
    fail("cannot wait for the child");
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the child did not exit normally");
  }
  return Captured {WEXITSTATUS(waitStatus), readFile(outPath),
                   readFile(errPath)};
}

// FR-016: a name outside the five stops the batch before any run.
auto scenarioBadName() -> int
{
  const Captured run =
      runChild("badName", {"--placement=l2", "--filter", "^bmCliBadName$"},
               &setupBadName);
  check(run.status == 2, "a rejected preset name exits 2 (FR-016)");
  check(run.err.find("l2") != std::string::npos,
        "the line names the rejected value (FR-016)");
  check(run.out.find("bmCliBadName") == std::string::npos,
        "no benchmark ran (FR-016)");
  return 0;
}

// FR-017: a value naming cpus is refused the same way.
auto scenarioCpuList() -> int
{
  const Captured run =
      runChild("cpuList", {"--placement=0-3", "--filter", "^bmCliCpuList$"},
               &setupCpuList);
  check(run.status == 2, "a cpu list exits 2 (FR-017)");
  check(run.err.find("0-3") != std::string::npos,
        "the line names the rejected value (FR-017)");
  check(run.out.find("bmCliCpuList") == std::string::npos,
        "no benchmark ran (FR-017)");
  return 0;
}

// contracts/cli-placement.md: the standing missing-argument path stands.
auto scenarioNoArgument() -> int
{
  const Captured run =
      runChild("noArgument", {"--placement", "--filter", "^bmCliNoArgument$"},
               &setupNoArgument);
  check(run.status == 2, "the missing argument exits 2");
  check(run.out.find("bmCliNoArgument") == std::string::npos,
        "no benchmark ran");
  return 0;
}

// FR-015: the fit check reads the handle's preset in place of the
// command-line default. On any host of the matrix `smt` places fewer than
// four contexts, so the handle winning shows as the line naming `smt`.
auto scenarioHandleWins() -> int
{
  const Captured run =
      runChild("handleWins", {"--placement=l3", "--filter", "^bmCliHandleSmt$"},
               &setupHandleWins);
  check(run.status == 2, "the unfittable handle preset exits 2 (FR-015)");
  check(run.err.find("smt") != std::string::npos,
        "the line names the handle's preset, not the command line's "
        "(FR-015)");
  check(run.out.find("bmCliHandleSmt") == std::string::npos,
        "no benchmark ran (FR-015)");
  return 0;
}

// FR-016: a handle name outside the five stops the expansion.
auto scenarioHandleBadName() -> int
{
  const Captured run =
      runChild("handleBadName", {"--filter", "^bmCliHandleBad$"},
               &setupHandleBad);
  check(run.status == 2, "a rejected handle name exits 2 (FR-016)");
  check(run.err.find("l2") != std::string::npos,
        "the line names the rejected text (FR-016)");
  check(run.out.find("bmCliHandleBad") == std::string::npos,
        "no benchmark ran (FR-016)");
  return 0;
}

// The standing unknown-option line stands beside the new option.
auto scenarioUnknownFlag() -> int
{
  const Captured run =
      runChild("unknownFlag", {"--placements=l3", "--filter",
                               "^bmCliUnknownFlag$"},
               &setupUnknownFlag);
  check(run.status == 2, "an unknown flag exits 2");
  check(run.err.find("unknown option") != std::string::npos,
        "the standing unknown-option line stands");
  return 0;
}

}  // namespace

auto main() -> int
{
  struct Scenario
  {
    const char* name;
    int (*run)();
  };
  const std::array<Scenario, 6> scenarios {
      Scenario {"badName", &scenarioBadName},
      Scenario {"cpuList", &scenarioCpuList},
      Scenario {"noArgument", &scenarioNoArgument},
      Scenario {"handleWins", &scenarioHandleWins},
      Scenario {"handleBadName", &scenarioHandleBadName},
      Scenario {"unknownFlag", &scenarioUnknownFlag}};
  for (const auto& scenario : scenarios) {
    check(scenario.run() == 0, scenario.name);
  }

  std::printf("harness_cli_placement_test: all scenarios passed\n");
  return 0;
}
