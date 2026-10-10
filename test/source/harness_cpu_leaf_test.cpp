// ============================================================================
// TDD test for the process CPU leaf (T014; US2, capability C-2, FR-004,
// FR-002, SC-001).
//
// A scripted FakeProvider owns the machine time leaves, and each leaf
// steps by a constant amount per sampling action, so the fold of a run's
// endpoint pair is that constant whatever length the plan's own
// sampling-cost calibration has. Over the fixed iteration count of four:
//
//   machine/monotonic   1000 ns  -> real time/iter (ns)=250.000
//   machine/thread_cpu   400 ns  -> cpu time/iter (ns)=100.000
//   machine/process_cpu  900 ns  -> cpu time/iter (ns)=225.000
//
// `measureProcessCpuTime` moves the CPU column to the process fold, so
// the 225.000 figure names `machine/process_cpu` as the leaf the runner
// resolved (FR-004, SC-001). The same call stands beside `useRealTime`:
// the CPU column keeps the process fold while the aggregate line
// summarizes the real column, the primary time that call selects
// (FR-004, FR-002).
//
// A provider that publishes no `machine/process_cpu` leaves that one
// benchmark failed with the resolution line the runner already prints
// for a missing leaf, and a plain benchmark of the same run still
// measures its two columns (FR-004, FR-001). That script differs from
// the parent's, so the scenario runs in a re-exec of this binary and the
// parent reads the wait status: a signaled child carries a
// `WEXITSTATUS` of zero, so the `WIFEXITED` gate comes first.
//
// Hand-computed expectations, frameworkless check()/fail() convention,
// one provider script per process.
// ============================================================================

#include <bit>
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

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CPU LEAF TEST FAIL: %s\n", what);
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
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

constexpr std::uint64_t kRealStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 400;
constexpr std::uint64_t kProcessCpuStep = 900;
constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kRepetitions = 3;

auto bmProcessCpu(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCombined(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmPlain(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The one script of one process. `publishProcessCpu` decides whether the
// machine object holds the process leaf at all, which is the difference
// between the parent's host and the re-exec's.
auto scriptedProvider(const bool publishProcessCpu) -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;

  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, kRealStep);
  provider.setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  if (publishProcessCpu) {
    provider.addCounter("machine", "process_cpu", "nanoseconds", "process cpu");
    provider.setPoints("machine", "process_cpu", {0}, kProcessCpuStep);
  }

  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// `expectZero` states the exit status the scenario owes: a run with a
// failed benchmark exits nonzero by the table of contracts/cli.md.
auto captureRun(const std::vector<std::string>& arguments,
                const bool expectZero = true) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cpu_leaf_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_cpu_leaf_out.txt";
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
  if ((status == 0) != expectZero) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("the run did not hold the exit status the scenario owes");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto checkHas(const std::string& line,
              const std::string& needle,
              const char* what) -> void
{
  if (line.find(needle) == std::string::npos) {
    std::fprintf(stderr, "no '%s' in '%s'\n", needle.c_str(), line.c_str());
    fail(what);
  }
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

auto fieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

// The scripted step of one leaf divided by the fixed iteration count:
// the value the column of that leaf owes.
auto perIteration(const std::uint64_t step) -> double
{
  return static_cast<double>(step) / static_cast<double>(kIterations);
}

// The switch moves the CPU column to the scripted process fold, and the
// real column keeps the monotonic fold.
auto processCpuScenario() -> void
{
  auto handle = sg::registerBenchmark(&bmProcessCpu, "bmProcessCpu");
  handle.measureProcessCpuTime();

  const std::string row =
      lineOf(captureRun({"--filter", "^bmProcessCpu$", "--iterations=4"}),
             "bmProcessCpu");
  check(
      sameDouble(fieldOf(row, "real time/iter (ns)="), perIteration(kRealStep)),
      "the real column keeps the monotonic fold under the switch (FR-001)");
  check(sameDouble(fieldOf(row, "cpu time/iter (ns)="),
                   perIteration(kProcessCpuStep)),
        "the CPU column shows the scripted machine/process_cpu fold (FR-004, "
        "SC-001)");
}

// The switch combines with the real-time call: the CPU column keeps the
// process fold while the aggregate summarizes the real column.
auto combinedScenario() -> void
{
  auto handle = sg::registerBenchmark(&bmCombined, "bmCombined");
  handle.measureProcessCpuTime().useRealTime();

  const std::string report = captureRun(
      {"--filter", "^bmCombined$", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> rows = linesOf(report, "bmCombined");
  check(rows.size() == kRepetitions + 1,
        "three repetitions print three rows and the aggregate (FR-012)");

  check(sameDouble(fieldOf(rows[0], "cpu time/iter (ns)="),
                   perIteration(kProcessCpuStep)),
        "the process CPU column stands under the real-time call (FR-004)");
  checkHas(
      rows.back(),
      "primary time/iter (ns) mean=",
      "the aggregate line names the primary time in the row unit (FR-002)");
  check(sameDouble(fieldOf(rows.back(), "primary time/iter (ns) mean="),
                   perIteration(kRealStep)),
        "the real-time call makes the real column the primary time beside the "
        "process CPU column (FR-004, FR-002)");
}

// The re-exec: a machine object with no process leaf fails the switched
// benchmark with the resolution line, and the plain benchmark of the
// same run keeps its two columns.
auto missingProcessCpuScenario() -> int
{
  scriptedProvider(false);

  auto handle = sg::registerBenchmark(&bmProcessCpu, "bmProcessCpu");
  handle.measureProcessCpuTime();
  (void)sg::registerBenchmark(&bmPlain, "bmPlain");

  const std::string report =
      captureRun({"--filter", "^bm", "--iterations=4"}, false);
  const std::vector<std::string> switched = linesOf(report, "bmProcessCpu");
  check(switched.size() == 1, "the switched benchmark prints one row (FR-004)");
  checkHas(switched[0], "FAILED:", "the missing leaf fails that benchmark");
  checkHas(switched[0],
           "machine/process_cpu does not resolve",
           "the failure carries the resolution line the runner prints for a "
           "missing leaf (FR-004)");

  const std::string plain = lineOf(report, "bmPlain");
  check(sameDouble(fieldOf(plain, "real time/iter (ns)="),
                   perIteration(kRealStep)),
        "the plain benchmark of the same run keeps its real column (FR-001)");
  check(sameDouble(fieldOf(plain, "cpu time/iter (ns)="),
                   perIteration(kThreadCpuStep)),
        "the plain benchmark of the same run keeps the thread CPU column "
        "(FR-001, FR-004)");

  std::puts("harness_cpu_leaf_test missingProcessCpu: ok");
  return 0;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self = argc > 0 ? argv[0] : "harness_cpu_leaf_test";
  const std::string mode = argc > 1 ? argv[1] : "";

  if (mode == "missingProcessCpu") {
    return missingProcessCpuScenario();
  }

  scriptedProvider(true);
  processCpuScenario();
  combinedScenario();

  // The missing-leaf script belongs to its own process, so the parent
  // reads that scenario's wait status. A child that dies by a signal
  // reports a zero `WEXITSTATUS`, so `WIFEXITED` gates the read.
  const int status =
      std::system(("\"" + self + "\" missingProcessCpu 2>/dev/null").c_str());
  check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "the missing-leaf scenario passes in its own process (FR-004)");

  std::puts("harness_cpu_leaf_test: ok");
  return 0;
}
