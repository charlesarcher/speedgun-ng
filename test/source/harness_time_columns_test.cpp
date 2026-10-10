// ============================================================================
// TDD test for the two time columns (T008; US1, capability C-1, FR-001,
// FR-002, FR-003, FR-013, FR-018, SC-001, SC-004).
//
// A scripted FakeProvider owns the three machine time leaves and one
// scripted work leaf, and every leaf steps by a constant amount per
// sampling action, so the fold of a run's endpoint pair is that constant
// whatever length the plan's own sampling-cost calibration has. Over the
// fixed iteration count of four, each figure the row prints is the
// scripted step over N:
//
//   machine/monotonic   1000 ns  -> real time/iter (ns)=250.000
//   machine/thread_cpu   400 ns  -> cpu time/iter (ns)=100.000
//   machine/process_cpu  900 ns  -> a process-fed column would read 225
//   fake/work           1200 ops -> work=300.000
//
// The process leaf stands in the same script while the CPU column reads
// 100.000, so that figure names `machine/thread_cpu` as the leaf the
// runner resolved for a benchmark that selects no mode (FR-001). The
// aggregate line names the primary time and summarizes that same CPU
// selection (FR-002). A run with no pause call keeps the H1 endpoint
// pair and the row prints its sample count (FR-003, FR-013, SC-004). An
// attached metric keeps the fold and the disclosure fields H1 printed,
// beside the two time columns (FR-018).
//
// The C++ value carries the same two figures as two fields of the row
// struct, which the static_assert below names; the printed rows carry
// the values. Hand-computed expectations, frameworkless check()/fail()
// convention, one provider script per process.
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

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS TIME COLUMNS TEST FAIL: %s\n", what);
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
constexpr std::uint64_t kWorkStep = 1200;
constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kRepetitions = 3;

// FR-018: the row struct gained the CPU column and the run's sample
// count beside the real column, so a consumer of the C++ value reads the
// two figures the row prints. The printed rows below carry the values;
// this check names the fields.
static_assert(
    requires(const sg::ResultRow& row) {
      row.cpuTimePerIterationNs;
      row.endpointSamples;
    },
    "ResultRow carries cpuTimePerIterationNs and endpointSamples " "(FR-018)");

auto bmColumns(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmMetric(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmAggregate(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto scriptedProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;

  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.addCounter("machine", "process_cpu", "nanoseconds", "process cpu");
  provider.addObject("fake", "fake", "scripted workload object");
  provider.addCounter("fake", "work", "ops", "scripted work counter");
  provider.setPoints("machine", "monotonic", {0}, kRealStep);
  provider.setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  provider.setPoints("machine", "process_cpu", {0}, kProcessCpuStep);
  provider.setPoints("fake", "work", {0}, kWorkStep);

  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_time_columns_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_time_columns_out.txt";
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

auto countFieldOf(const std::string& line, const std::string& key) -> long long
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtoll(line.c_str() + at + key.size(), nullptr, 10);
}

// The scripted step of one leaf divided by the fixed iteration count:
// the value the column of that leaf owes.
auto perIteration(const std::uint64_t step) -> double
{
  return static_cast<double>(step) / static_cast<double>(kIterations);
}

// Both time columns print, each exact to its scripted delta, and the
// CPU column follows machine/thread_cpu.
auto columnsScenario() -> void
{
  const std::string report =
      captureRun({"--filter", "^bmColumns$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmColumns");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");

  check(sameDouble(fieldOf(rows[0], "real time/iter (ns)="),
                   perIteration(kRealStep)),
        "the real column is the scripted monotonic delta over N (FR-001, "
        "SC-001)");
  check(sameDouble(fieldOf(rows[0], "cpu time/iter (ns)="),
                   perIteration(kThreadCpuStep)),
        "the CPU column follows machine/thread_cpu while the process leaf "
        "stands in the same script (FR-001, SC-001)");
  checkHas(rows[0],
           "endpoint samples=2",
           "a run with no pause call keeps the endpoint pair and prints its "
           "count (FR-003, FR-013, SC-004)");
}

// An attached metric keeps its H1 fold and its disclosure fields beside
// the two time columns.
auto metricScenario() -> void
{
  auto handle = sg::registerBenchmark(&bmMetric, "bmMetric");
  {
    auto& system = sg::counters::System::local();
    const auto object = system.object("fake");
    check(object.has_value(), "the scripted work object resolves");
    const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
    check(work.has_value(), "the scripted work leaf resolves");
    handle.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*work),
                     "work");
  }

  const std::string row = lineOf(
      captureRun({"--filter", "^bmMetric$", "--iterations=4"}), "bmMetric");
  check(
      sameDouble(fieldOf(row, "real time/iter (ns)="), perIteration(kRealStep)),
      "the real column keeps its scripted delta beside a metric (FR-001)");
  check(sameDouble(fieldOf(row, "cpu time/iter (ns)="),
                   perIteration(kThreadCpuStep)),
        "the CPU column keeps its scripted delta beside a metric (FR-001)");
  check(sameDouble(fieldOf(row, "work="), perIteration(kWorkStep)),
        "the metric keeps the H1 fold over the scripted step (FR-018)");
  const std::string metric = row.substr(row.find("work="));
  check(row.find("work=") > row.find("cpu time/iter (ns)="),
        "the metric column stands beside the two time columns (FR-018)");
  checkHas(
      metric, "ratio=1.000", "the metric keeps its H1 ratio field (FR-018)");
  checkHas(metric, "scaled=0", "the metric keeps its H1 scaled field (FR-018)");
  checkHas(metric, "gap=0", "the metric keeps its H1 gap field (FR-018)");
}

// The aggregate of FR-002 names the primary time and its statistics
// summarize the CPU selection.
auto aggregateScenario() -> void
{
  const std::string report = captureRun(
      {"--filter", "^bmAggregate$", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> rows = linesOf(report, "bmAggregate");
  check(rows.size() == kRepetitions + 1,
        "three repetitions print three rows and the aggregate (FR-012)");

  checkHas(
      rows.back(),
      "primary time/iter (ns) mean=",
      "the aggregate line names the primary time in the row unit (FR-002)");
  check(sameDouble(fieldOf(rows.back(), "primary time/iter (ns) mean="),
                   perIteration(kThreadCpuStep)),
        "the aggregate summarizes the CPU selection (FR-002)");
  check(
      countFieldOf(rows.back(), " n=") == static_cast<long long>(kRepetitions),
      "every repetition carries the primary time (FR-002)");
}

}  // namespace

auto main() -> int
{
  scriptedProvider();
  (void)sg::registerBenchmark(&bmColumns, "bmColumns");
  (void)sg::registerBenchmark(&bmAggregate, "bmAggregate");

  columnsScenario();
  metricScenario();
  aggregateScenario();

  std::puts("harness_time_columns_test: ok");
  return 0;
}
