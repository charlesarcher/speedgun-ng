// ============================================================================
// TDD test for the registry and the handle options (T014; US1).
//
// Covers SG_BENCHMARK registration before main (FR-001, R-09), a runtime
// registration through registerBenchmark, the duplicate-name rule of FR-002
// that keeps the first registration and leaves the rest runnable, the E-01
// option fields the handle setters write (FR-013, FR-015), the calibration
// run bound of FR-010, and the contract violation a setter after the run
// starts reports (E-02). Hand-computed expectations, frameworkless
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
  std::fprintf(stderr, "HARNESS REGISTRY TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmRegistered(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmBounded(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmViolating(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

sg::BenchmarkHandle violatingHandle;

auto captureRun(int argc, char** argv) -> std::string
{
  const char* const path = "harness_registry_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status = sg::speedgunMain(argc, argv);
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  return [path, status]()
  {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();
    if (status != 0) {
      std::fprintf(
          stderr, "speedgunMain returned %d: %s\n", status, contents.c_str());
      fail("the run did not exit zero");
    }
    return contents;
  }();
}

auto countLines(const std::string& text, const std::string& needle) -> int
{
  int found = 0;
  std::size_t at = text.find(needle);
  while (at != std::string::npos) {
    ++found;
    at = text.find(needle, at + needle.size());
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

}  // namespace

SG_BENCHMARK(bmRegistered);

auto main(const int argc, char** argv) -> int
{
  if (argc > 1 && std::string(argv[1]) == "setter-after-run") {
    violatingHandle = sg::registerBenchmark(&bmViolating, "bmViolating");
    std::vector<std::string> arguments = {
        "--filter", "bmViolating", "--dry-run", "--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (auto& argument : arguments) {
      inner.push_back(argument.data());
    }
    (void)sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
    // The run has started, so this setter is a contract violation and the
    // checked semantic terminates the process.
    violatingHandle.minTime(1);
    return 0;
  }

  // The run uses the real clock provider the entry point registers: the
  // growth rule of FR-016 has to see a decision time that follows the
  // iteration count, which a scripted constant step never does.
  const auto duplicate = sg::registerBenchmark(&bmRegistered, "bmRegistered");
  check(duplicate.name().empty(),
        "a repeated name returns a handle that names nothing (FR-002)");

  auto fixedHandle = sg::registerBenchmark(&bmFixed, "bmFixed");
  check(fixedHandle.name() == "bmFixed",
        "the runtime registration names itself");
  fixedHandle.iterations(5).minTime(1'000'000);

  auto boundedHandle = sg::registerBenchmark(&bmBounded, "bmBounded");
  boundedHandle.minTime(2'000'000);

  std::vector<std::string> listArguments = {"--list"};
  std::vector<char*> list;
  list.push_back(argv[0]);
  for (auto& argument : listArguments) {
    list.push_back(argument.data());
  }
  const std::string listed =
      captureRun(static_cast<int>(list.size()), list.data());
  check(lineOf(listed, "bmRegistered").size() > 0,
        "SG_BENCHMARK registered before main (FR-001)");
  check(countLines(listed, "bmRegistered") == 1,
        "the duplicate name kept one registration (FR-002)");

  std::vector<std::string> runArguments = {"--filter", "bmFixed"};
  std::vector<char*> run;
  run.push_back(argv[0]);
  for (auto& argument : runArguments) {
    run.push_back(argument.data());
  }
  const std::string report =
      captureRun(static_cast<int>(run.size()), run.data());

  // FR-013: the handle's fixed iteration count skips calibration.
  check(countFieldOf(lineOf(report, "bmFixed"), "iterations=") == 5,
        "the handle's iterations option reached the run (FR-013)");

  // FR-010 and FR-016: an unreachable minimum-time target grows the count
  // and stops at the run bound.
  std::vector<std::string> boundedArguments = {"--filter", "bmBounded"};
  std::vector<char*> bounded;
  bounded.push_back(argv[0]);
  for (auto& argument : boundedArguments) {
    bounded.push_back(argument.data());
  }
  const std::string boundedReport =
      captureRun(static_cast<int>(bounded.size()), bounded.data());
  const long long grown =
      countFieldOf(lineOf(boundedReport, "bmBounded"), "iterations=");
  check(grown > 1, "the calibration grew the iteration count (FR-016)");
  check(grown <= static_cast<long long>(1'000'000'000'000LL),
        "the growth stays inside the bound (FR-016)");

  const std::string self = argc > 0 ? argv[0] : "harness_registry_test";
  const int violated =
      std::system(("\"" + self + "\" setter-after-run " "2>/dev/null").c_str());
  check(violated != 0, "a setter after the run starts is a violation (E-02)");

  std::puts("harness_registry_test: ok");
  return 0;
}
