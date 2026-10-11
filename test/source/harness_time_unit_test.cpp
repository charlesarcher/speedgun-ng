// ============================================================================
// TDD test for the printed time unit (T023; US5, C-6, FR-015, FR-016, SC-005).
//
// Each scripted leaf steps by a constant per sampling action, and a run with
// no pause owns two endpoint samples, so its window fold is one step and the
// printed per-iteration value one quarter of that step at iterations=4:
//
//   machine/monotonic  step 2'500'000 ns -> 625'000 ns per iteration
//   machine/thread_cpu step 1'500'000 ns -> 375'000 ns per iteration
//
// Under TimeUnit::MILLISECONDS the same row prints those values divided by
// 1'000'000, each under the ms label, and the aggregate of three repetitions
// summarizes the primary column at the same scale:
//
//   real time/iter (ms)=0.625   cpu time/iter (ms)=0.375
//   primary time/iter (ms) mean=0.375  n=3
//
// The overhead floor is the plan's own wall-clock figure, so its value stands
// under no test here and only its scale does. The ceiling below is 0.01 ms,
// a sampling overhead of ten microseconds; a floor printed at the
// nanosecond scale would sit four orders above it.
//
// Each mode owns one provider script, so it runs in a re-exec of this binary,
// the way the calibration test's scripted scenarios do. Frameworkless
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

#include <sys/wait.h>
#include <unistd.h>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

constexpr std::uint64_t kMonotonicStep = 2'500'000;
constexpr std::uint64_t kThreadCpuStep = 1'500'000;
constexpr std::uint64_t kIterations = 4;

constexpr double kRealPerIterationNs =
    static_cast<double>(kMonotonicStep) / static_cast<double>(kIterations);
constexpr double kCpuPerIterationNs =
    static_cast<double>(kThreadCpuStep) / static_cast<double>(kIterations);
constexpr double kRealPerIterationMs = kRealPerIterationNs / 1'000'000.0;
constexpr double kCpuPerIterationMs = kCpuPerIterationNs / 1'000'000.0;
constexpr double kFloorCeilingMs = 0.01;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS TIME UNIT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmUnit(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The callable of the benchmark that `SG_BENCHMARK` registers during static
// initialization, before any mode of this process can set a default.
auto bmCaptured(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

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

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_time_unit_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_time_unit_out.txt";
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

// FR-015, SC-005: one line carries one scale, and every time value on it
// carries the millisecond scale of the scripted figure.
auto checkMillisecondRow(const std::string& line) -> void
{
  check(line.contains("real time/iter (ms)="),
        "the real column carries the ms label (FR-015)");
  check(line.contains("cpu time/iter (ms)="),
        "the CPU column carries the ms label (FR-015)");
  check(line.contains("overhead floor (ms)="),
        "the overhead floor carries the ms label (FR-015)");
  check(!line.contains("(ns)="),
        "one line carries one scale (FR-015, plan R-08)");
  check(
      closeTo(doubleFieldOf(line, "real time/iter (ms)="), kRealPerIterationMs),
      "the real column divides the scripted fold by 1000000 (FR-015, SC-005)");
  check(closeTo(doubleFieldOf(line, "cpu time/iter (ms)="), kCpuPerIterationMs),
        "the CPU column divides the scripted fold by 1000000 (FR-015, SC-005)");
  check(doubleFieldOf(line, "overhead floor (ms)=") < kFloorCeilingMs,
        "the overhead floor carries the millisecond scale (FR-015)");
}

// FR-015, FR-002: the aggregate line scales with the benchmark's unit and
// summarizes the primary column.
auto checkMillisecondAggregate(const std::string& line) -> void
{
  check(line.contains("primary time/iter (ms) mean="),
        "the aggregate line carries the ms label (FR-002, FR-015)");
  check(closeTo(doubleFieldOf(line, "primary time/iter (ms) mean="),
                kCpuPerIterationMs),
        "the aggregate summarizes the primary column at the ms scale "
        "(FR-002, FR-015)");
  check(countFieldOf(line, " n=") == 3,
        "three repetitions feed the aggregate (FR-012)");
}

// FR-015: the handle call sets the unit of that benchmark, and the scale
// reaches both columns, the floor, and the aggregate.
auto modeUnitCall() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmUnit, "bmMs");
  handle.unit(sg::TimeUnit::MILLISECONDS);

  const std::string report =
      captureRun({"--filter", "^bmMs$", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> rows = linesOf(report, "bmMs");
  check(rows.size() == 4,
        "three repetitions print three rows and the aggregate (FR-012)");
  for (std::size_t row = 0; row + 1 < rows.size(); ++row) {
    checkMillisecondRow(rows[row]);
  }
  checkMillisecondAggregate(rows.back());
  std::puts("harness_time_unit_test unitCall: ok");
  return 0;
}

// FR-016: the library default reaches a benchmark that sets none, because the
// registration captures the default of its own moment.
auto modeDefault() -> int
{
  scriptProvider();
  sg::setDefaultTimeUnit(sg::TimeUnit::MILLISECONDS);
  check(sg::getDefaultTimeUnit() == sg::TimeUnit::MILLISECONDS,
        "the library reads back the default it was given (FR-016)");
  const auto handle = sg::registerBenchmark(&bmUnit, "bmDefault");
  check(handle.name() == "bmDefault", "the benchmark registers");

  const std::string report =
      captureRun({"--filter", "^bmDefault$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmDefault");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");
  checkMillisecondRow(rows[0]);
  std::puts("harness_time_unit_test default: ok");
  return 0;
}

// FR-016, spec Edge Cases: `bmCaptured` registered during static
// initialization, while the process default still stood at nanoseconds. The
// default set here arrives after that registration, so the row keeps ns.
auto modeCaptured() -> int
{
  scriptProvider();
  sg::setDefaultTimeUnit(sg::TimeUnit::MILLISECONDS);
  check(sg::getDefaultTimeUnit() == sg::TimeUnit::MILLISECONDS,
        "the default reads back after the registration it missed (FR-016)");

  const std::string report =
      captureRun({"--filter", "^bmCaptured$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmCaptured");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");
  check(rows[0].contains("real time/iter (ns)="),
        "the registered benchmark keeps the unit it captured (FR-016)");
  check(!rows[0].contains("(ms)="),
        "a later default reaches no earlier registration (FR-016)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="),
                kRealPerIterationNs),
        "the real column keeps the nanosecond scale (FR-015, FR-016)");
  check(closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="),
                kCpuPerIterationNs),
        "the CPU column keeps the nanosecond scale (FR-015, FR-016)");
  std::puts("harness_time_unit_test captured: ok");
  return 0;
}

}  // namespace

// The registration stands at static initialization, before any mode of this
// process can set a default (FR-016).
SG_BENCHMARK(bmCaptured);

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "unitCall") {
    return modeUnitCall();
  }
  if (mode == "default") {
    return modeDefault();
  }
  if (mode == "captured") {
    return modeCaptured();
  }

  const std::string self = argv[0];
  for (const auto& scenario : {"unitCall", "default", "captured"}) {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    // A signaled child carries no exit code: WEXITSTATUS of that wait status
    // reads zero, so the exit test stands only for a child that ran to
    // completion.
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (SC-005)");
  }

  std::puts("harness_time_unit_test: ok");
  return 0;
}
