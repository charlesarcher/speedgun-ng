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
// The unit resolves when a run starts, after the benchmark registered
// (FR-027): the entry's own unit call wins, else the default speedgunMain
// snapshotted for the batch, else the process default read at the run's
// start. A default outside the closed enumeration reaches the report through
// that resolution, because the setter holds no range check, so the scale guard
// answers it at the nanosecond scale, the one divisor that cannot divide by
// zero (FR-015, spec Edge Cases).
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

#include "detail/internal.hpp"
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
constexpr double kRealPerIterationUs = kRealPerIterationNs / 1'000.0;
constexpr double kCpuPerIterationUs = kCpuPerIterationNs / 1'000.0;
constexpr double kFloorCeilingMs = 0.01;
constexpr double kFloorCeilingUs = 10.0;

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

// The field key of one column at one unit scale, as the report prints it.
auto scaleKey(const char* const column, const char* const unit) -> std::string
{
  std::string key(column);
  key.append(" (");
  key.append(unit);
  key.append(")");
  key.push_back('=');
  return key;
}

// One unit scale and the scripted figures it divides. The ceiling is the same
// overhead of ten microseconds read at that scale.
struct ScaleExpectation
{
  const char* unit;
  double realValue;
  double cpuValue;
  double floorCeiling;
};

constexpr ScaleExpectation kMsScale {
    "ms", kRealPerIterationMs, kCpuPerIterationMs, kFloorCeilingMs};
constexpr ScaleExpectation kUsScale {
    "us", kRealPerIterationUs, kCpuPerIterationUs, kFloorCeilingUs};

// FR-015, SC-005: one line carries one scale, and every time value on it
// carries the divisor of the unit under test.
auto checkScaledRow(const std::string& line,
                    const ScaleExpectation& scale) -> void
{
  const std::string realKey = scaleKey("real time/iter", scale.unit);
  const std::string cpuKey = scaleKey("cpu time/iter", scale.unit);
  const std::string floorKey = scaleKey("overhead floor", scale.unit);
  check(
      line.contains(realKey) && line.contains(cpuKey)
          && line.contains(floorKey),
      "every time field carries the label of the unit under test " "(FR-015)");
  for (const char* const other : {"ns", "us", "ms", "s"}) {
    if (std::string(other) == std::string(scale.unit)) {
      continue;
    }
    check(!line.contains(scaleKey("real time/iter", other)),
          "one line carries one scale (FR-015, plan R-08)");
  }
  check(closeTo(doubleFieldOf(line, realKey), scale.realValue),
        "the real column divides the scripted fold by the unit divisor "
        "(FR-015, SC-005)");
  check(closeTo(doubleFieldOf(line, cpuKey), scale.cpuValue),
        "the CPU column divides the scripted fold by the unit divisor "
        "(FR-015, SC-005)");
  check(doubleFieldOf(line, floorKey) < scale.floorCeiling,
        "the overhead floor carries the scale of the unit (FR-015)");
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
    checkScaledRow(rows[row], kMsScale);
  }
  checkMillisecondAggregate(rows.back());
  std::puts("harness_time_unit_test unitCall: ok");
  return 0;
}

// FR-016: the library default reaches a benchmark that sets none, because the
// unit resolves when the run starts and reads that default.
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
  checkScaledRow(rows[0], kMsScale);
  std::puts("harness_time_unit_test default: ok");
  return 0;
}

// FR-027: the default a main sets before speedgunMain reaches the
// registrations the macro made during static initialization, which stand from
// a moment no mode of this process could have set. The unit call of bmSeconds
// still wins over that default.
auto modeCaptured() -> int
{
  scriptProvider();
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);
  check(sg::getDefaultTimeUnit() == sg::TimeUnit::MICROSECONDS,
        "the default reads back after the registration it used to miss "
        "(FR-027)");
  auto seconds = sg::registerBenchmark(&bmUnit, "bmSeconds");
  seconds.unit(sg::TimeUnit::SECONDS);

  const std::string report = captureRun({"--filter", "^bm", "--iterations=4"});

  const std::vector<std::string> rows = linesOf(report, "bmCaptured");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");
  checkScaledRow(rows[0], kUsScale);

  // The second-scale figure of this scripted step, 0.000625, stands below the
  // three decimals the column prints, so the precedence of the unit call is
  // the label's claim and not the value's.
  const std::vector<std::string> secondsRows = linesOf(report, "bmSeconds");
  check(secondsRows.size() == 1,
        "the benchmark that calls unit prints one measured row (FR-012)");
  const std::string secondsScale = scaleKey("real time/iter", "s");
  check(secondsRows[0].contains(secondsScale)
            && secondsRows[0].contains(scaleKey("cpu time/iter", "s"))
            && secondsRows[0].contains(scaleKey("overhead floor", "s")),
        "the unit the handle calls wins over the default of the batch "
        "(FR-016, FR-027)");
  check(!secondsRows[0].contains(scaleKey("real time/iter", "us")),
        "one line carries one scale (FR-015, plan R-08)");
  std::puts("harness_time_unit_test captured: ok");
  return 0;
}

// The expanded instance of one name, or null when the expansion left no such
// instance.
auto findInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  return nullptr;
}

// FR-027: the test-hook path, the one harness_gap_test.cpp and
// harness_instance_name_test.cpp reach without speedgunMain. No batch
// snapshot exists there, so the unit resolves to the process default read at
// the start of the run, which the registration above could not have seen.
auto modeTestHook() -> int
{
  scriptProvider();
  const auto handle = sg::registerBenchmark(&bmUnit, "bmHook");
  check(handle.name() == "bmHook", "the benchmark registers");
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kIterations;
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmHook");
  check(instance != nullptr, "the expansion produced the registered instance");
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.timeUnit == sg::TimeUnit::MICROSECONDS,
        "the run reads the process default at its start; the registration "
        "keeps no unit of its own (FR-027)");
  check(result.rows.size() == 1, "the hook run produces one measured row");
  check(closeTo(static_cast<double>(result.rows[0].timePerIterationNs),
                kRealPerIterationNs)
            && closeTo(
                   static_cast<double>(result.rows[0].cpuTimePerIterationNs),
                   kCpuPerIterationNs),
        "the result keeps the scripted fold in nanoseconds; the unit scales "
        "the printing alone (FR-015, FR-027)");
  std::puts("harness_time_unit_test testHook: ok");
  return 0;
}

// FR-015, spec Edge Cases: a value outside the closed enumeration of the unit
// prints the nanosecond label and the raw fold, because the guard falls back
// to the scale whose divisor is one.
auto modeOutOfRange() -> int
{
  scriptProvider();
  constexpr auto kBogusUnit = static_cast<sg::TimeUnit>(9);
  sg::setDefaultTimeUnit(kBogusUnit);
  check(sg::getDefaultTimeUnit() == kBogusUnit,
        "the library reads back the value it was given (FR-016)");
  const auto handle = sg::registerBenchmark(&bmUnit, "bmBogus");
  check(handle.name() == "bmBogus", "the benchmark registers");

  const std::string report =
      captureRun({"--filter", "^bmBogus$", "--iterations=4"});
  const std::vector<std::string> rows = linesOf(report, "bmBogus");
  check(rows.size() == 1, "one repetition prints one measured row (FR-012)");
  check(rows[0].contains("real time/iter (ns)=")
            && rows[0].contains("cpu time/iter (ns)=")
            && rows[0].contains("overhead floor (ns)="),
        "the out-of-range unit carries the ns label on every time field "
        "(FR-015)");
  check(!rows[0].contains("(ms)=") && !rows[0].contains("(us)="),
        "one line carries one scale (FR-015, plan R-08)");
  check(closeTo(doubleFieldOf(rows[0], "real time/iter (ns)="),
                kRealPerIterationNs)
            && closeTo(doubleFieldOf(rows[0], "cpu time/iter (ns)="),
                       kCpuPerIterationNs),
        "the fallback divisor is one, so the printed value keeps the scripted "
        "fold (FR-015, SC-005)");
  std::puts("harness_time_unit_test outOfRange: ok");
  return 0;
}

}  // namespace

// The registration stands at static initialization, before any mode of this
// process can set a default, so the default this entry stands beside at
// registration is the nanosecond one of the library. FR-027 is the claim that
// its run reads a later default instead.
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
  if (mode == "testHook") {
    return modeTestHook();
  }
  if (mode == "outOfRange") {
    return modeOutOfRange();
  }

  const std::string self = argv[0];
  for (const auto& scenario : {
           "unitCall",
           "default",
           "captured",
           "testHook",
           "outOfRange",
       })
  {
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
