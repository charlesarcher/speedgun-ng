// ============================================================================
// TDD test for the throughput setters (T016; US2, FR-009, FR-010).
//
// `setBytesProcessed` opens `bytes_per_second` with the `RATE` flag and the
// `K1024` base, `setItemsProcessed` opens `items_per_second` with the `RATE`
// flag and the `K1000` base, and each sets the value the body passes (FR-009,
// E-03). Both columns therefore divide by the primary time of the row, which
// is the column the family selects (FR-004, FR-005, FR-006), and the printed
// value carries the declared base (FR-006).
//
// The scenarios run on the fake provider with a constant step per sampling
// action, so every divisor is a count of scripted steps and every expected
// column is an exact decimal:
//
//   iterations=4
//   machine/monotonic  step 1000 ns -> real fold 1000 ns, 250.000 ns per iter
//   machine/thread_cpu step  800 ns -> cpu  fold  800 ns, 200.000 ns per iter
//   manual             2000 ns per iteration   -> manual sum 8000 ns
//
// The default family takes the CPU column, so a body that passes 4096 bytes
// prints 4096 / 800e-9 / 1024 = 5000000.000 and the same 4096 items print
// 4096 / 800e-9 / 1000 = 5120000.000: one value, two bases, two printed
// numbers, which is what fixes the bases apart (FR-009, FR-006). The
// `useRealTime` family divides by the real column instead, and the
// `useManualTime` family divides by the sum of the times the body reports,
// 2000 ns over four iterations, so 500 items print 500 / 8000e-9 / 1000 =
// 62500.000 (FR-005, FR-006, FR-009).
//
// Each mode owns one provider script, so it runs in a re-exec of this binary.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 800;
constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kRepetitions = 3;

// The fold of each primary the scenarios divide by, in seconds.
constexpr double kCpuFoldSeconds = static_cast<double>(kThreadCpuStep) * 1e-9;
constexpr double kRealFoldSeconds = static_cast<double>(kMonotonicStep) * 1e-9;
constexpr std::int64_t kManualIterationNs = 2000;
constexpr double kManualFoldSeconds = static_cast<double>(kManualIterationNs)
    * static_cast<double>(kIterations) * 1e-9;

constexpr std::int64_t kProcessed = 4096;
constexpr std::int64_t kRealBytes = 8192;
constexpr std::int64_t kItems = 500;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS THROUGHPUT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Registers the two machine leaves every scenario reads, one step per
// sampling action of a run window.
auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// FR-009, FR-006: one value opened twice, once as bytes on the K1024 base
// and once as items on the K1000 base, over the CPU fold of the default
// family. Neither base divides: both columns print the rate as it stands.
auto bmBases(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setBytesProcessed(kProcessed);
      state.setItemsProcessed(kProcessed);
    }
    sg::doNotOptimize(index);
  }
}

// FR-005, FR-009: the family selects real time, so the rate divides by the
// real column of the row.
auto bmRealBytes(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setBytesProcessed(kRealBytes);
    }
    sg::doNotOptimize(index);
  }
}

// FR-006, FR-009: the body reports 2000 ns per iteration, so the primary is
// the manual sum of the four values and the rate divides by that sum.
auto bmManualItems(sg::State& state) -> void
{
  for (auto index : state) {
    state.setIterationTime(kManualIterationNs);
    state.setItemsProcessed(kItems);
    sg::doNotOptimize(index);
  }
}

// FR-009: the body totals its units after the timed loop, reading the
// iteration count off the state, so the printed rate divides that total by the
// primary time of the row.
constexpr std::int64_t kUnitsPerIteration = 4096;

auto bmTotalAfterLoop(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
    for (int spin = 0; spin != 64; ++spin) {
      sg::doNotOptimize(spin);
    }
  }
  state.setItemsProcessed(kUnitsPerIteration
                          * static_cast<std::int64_t>(state.iterations()));
}

// Captures one run of one benchmark and the exit status of the entry point.
// Every mode of this binary shares the one output file, so the modes run one
// after another.
auto captureRun(const std::vector<std::string>& arguments,
                int& status) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_throughput_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_throughput_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The report of a run that has to succeed.
auto captureGoodRun(const std::string& filter) -> std::string
{
  int status = 0;
  const std::string report = captureRun(
      {"--filter", filter, "--iterations=4", "--repetitions=3"}, status);
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("the run did not exit zero");
  }
  return report;
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

// The measured rows of a run, the aggregate row and its statistic lines
// excluded by the `iterations=` field the measured rows carry.
auto measuredRowsOf(const std::vector<std::string>& lines)
    -> std::vector<std::string>
{
  std::vector<std::string> rows;
  for (const auto& line : lines) {
    if (line.contains("iterations=")) {
      rows.push_back(line);
    }
  }
  return rows;
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

auto indexOf(const std::string& haystack,
             const std::string& needle) -> std::size_t
{
  const std::size_t at = haystack.find(needle);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no '%s' in '%s'\n", needle.c_str(), haystack.c_str());
    fail("the report carries no such text");
  }
  return at;
}

auto closeTo(const double actual, const double expected) -> bool
{
  return std::abs(actual - expected) <= 1e-3 + 1e-9 * std::abs(expected);
}

// FR-005: no printed column of the run carries an infinity or a not-a-number.
auto checkFinite(const std::vector<std::string>& lines) -> void
{
  for (const auto& line : lines) {
    check(!line.contains("inf") && !line.contains("nan"),
          "no printed value of the run is an infinity or a not-a-number "
          "(FR-005)");
  }
}

auto statisticLineOf(const std::vector<std::string>& lines,
                     const std::string& label) -> std::string
{
  const std::string key = label + " mean=";
  for (const auto& line : lines) {
    if (line.contains(key)) {
      return line;
    }
  }
  std::fprintf(stderr, "no statistic line '%s'\n", key.c_str());
  fail("the aggregate row carries no statistic line for that counter");
  return {};
}

// FR-009, FR-006, FR-010: the same value under the two bases prints one
// number twice over the CPU fold, in the order the columns opened, after
// the metric fields.
auto scenarioBases() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmBases, "bmBases");
  check(handle.name() == "bmBases", "the base benchmark registers");

  const std::string report = captureGoodRun("^bmBases$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmBases"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double bytes = kProcessed / kCpuFoldSeconds;
  const double items = kProcessed / kCpuFoldSeconds;
  for (const auto& row : rows) {
    check(indexOf(row, "endpoint samples") < indexOf(row, "bytes_per_second="),
          "the throughput columns follow the metric fields (FR-006)");
    check(indexOf(row, "bytes_per_second=") < indexOf(row, "items_per_second="),
          "the columns stand in the order they opened (FR-006)");
    check(closeTo(fieldOf(row, "bytes_per_second="), bytes),
          "the bytes column is the value over the primary time (FR-009, "
          "FR-006)");
    check(closeTo(fieldOf(row, "items_per_second="), items),
          "the items column is the value over the primary time (FR-009, "
          "FR-006)");
  }

  const std::vector<std::string> lines = linesOf(report, "bmBases");
  check(closeTo(fieldOf(statisticLineOf(lines, "bytes_per_second"), "mean="),
                bytes),
        "the bytes statistic summarizes the repetitions (FR-010)");
  check(closeTo(fieldOf(statisticLineOf(lines, "items_per_second"), "mean="),
                items),
        "the items statistic summarizes the repetitions (FR-010)");
  return 0;
}

// FR-005, FR-009: the family selects real time, so the byte rate divides by
// the real column of the row.
auto scenarioRealTime() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmRealBytes, "bmRealBytes");
  check(handle.name() == "bmRealBytes", "the real-time benchmark registers");
  handle.useRealTime();

  const std::string report = captureGoodRun("^bmRealBytes$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmRealBytes"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double expected = kRealBytes / kRealFoldSeconds;
  for (auto& row : rows) {
    check(closeTo(fieldOf(row, "bytes_per_second="), expected),
          "the real-time family divides the byte rate by the real column "
          "(FR-005, FR-009)");
  }
  return 0;
}

// FR-006, FR-009: the primary of the family is the sum of the times the body
// reports, so the item rate divides by 8000 ns and not by either machine
// column.
auto scenarioManualTime() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmManualItems, "bmManualItems");
  check(handle.name() == "bmManualItems", "the manual benchmark registers");
  handle.useManualTime();

  const std::string report = captureGoodRun("^bmManualItems$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmManualItems"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double expected = kItems / kManualFoldSeconds;
  for (auto& row : rows) {
    check(closeTo(fieldOf(row, "items_per_second="), expected),
          "the manual family divides the item rate by the manual sum "
          "(FR-006, FR-009)");
  }
  return 0;
}

}  // namespace

// FR-009, D-4: a total stated after the loop is divided by the primary time of
// the row, so the rate matches the units of one iteration over the time of one
// iteration even though the body never speaks per iteration.
auto scenarioTotalAfterLoop() -> int
{
  auto handle = sg::registerBenchmark(&bmTotalAfterLoop, "bmTotalAfterLoop");
  check(handle.name() == "bmTotalAfterLoop",
        "the totalled benchmark registers");
  handle.useRealTime();

  const std::string report = captureGoodRun("^bmTotalAfterLoop$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmTotalAfterLoop"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (auto& row : rows) {
    const double iterations = fieldOf(row, "iterations=");
    const double seconds = fieldOf(row, "real time/iter (ns)=") * 1e-9;
    check(iterations >= 1.0, "the row states the iterations it ran (FR-009)");
    const double expected = static_cast<double>(kUnitsPerIteration) / seconds;
    const double printed = fieldOf(row, "items_per_second=");
    check(printed > 0.0, "the item rate is a positive number (FR-009)");
    const double ratio = printed / expected;
    check(ratio > 0.8 && ratio < 1.2,
          "the total over the primary time lands within twenty percent of the "
          "per-iteration rate (FR-009)");
  }
  return 0;
}

auto main(int argc, char** argv) -> int
{
  const std::vector<std::string> arguments(argv, argv + argc);
  if (arguments.size() == 2) {
    const std::string& mode = arguments[1];
    if (mode == "bases") {
      return scenarioBases();
    }
    if (mode == "realTime") {
      return scenarioRealTime();
    }
    if (mode == "manualTime") {
      return scenarioManualTime();
    }
    if (mode == "totalAfterLoop") {
      return scenarioTotalAfterLoop();
    }
    fail("unknown mode");
  }

  const std::string self = argv[0];
  for (const auto& scenario :
       {"bases", "realTime", "manualTime", "totalAfterLoop"})
  {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (FR-009, FR-010)");
  }

  std::puts("harness_throughput_test: ok");
  return 0;
}
