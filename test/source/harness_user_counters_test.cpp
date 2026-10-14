// ============================================================================
// TDD test for user counters (T010; US1, FR-001 to FR-008, SC-006).
//
// Every scenario runs on the fake provider with a constant step per sampling
// action, so the fold of a run is a count of scripted steps and every counter
// column has an exact expected decimal. A fixed iteration count leaves the
// plan no warm-up and no calibration phase, and one step of each machine leaf
// stands in the window of a run:
//
//   iterations=4
//   machine/monotonic  step 1000 ns -> real time/iter 250.000 ns
//   machine/thread_cpu step  800 ns -> cpu  time/iter 200.000 ns
//
// The CPU column is the primary of a default run (FR-004), so the divisor a
// `RATE` counter divides by is the CPU fold of the row in seconds, four
// iterations of 200 ns written as 800 ns netted over the window: 800e-9 s.
//
// The flag pass applies the flags once per repetition after the loop in the
// order rate, thread average, iteration-invariant, average-over-iterations,
// invert, and the printed column carries that result as it stands: the declared
// base carries the suffix a rendered value shows and divides nothing
// (FR-003, FR-006). The
// expected numbers below follow that order; the two invert modes are the ones
// that fix the position of `INVERT` at the end, since an earlier invert
// shifts the printed value by orders of magnitude.
//
// A `RATE` run whose primary divisor is zero prints the flag-neutral value
// with a disclosure naming the zero divisor and stands aside from that
// counter's aggregate statistics (FR-005). No printed counter column of any
// scenario carries `inf` or `nan` (FR-005).
//
// The name bound and the sixteen-name cap are preconditions at the call, so
// their child dies through the contract facility, which writes the message to
// stderr and aborts; the parent forks, execs, reads the wait status and reads
// the number out of the captured message (FR-001, FR-002). A second call for
// a used name that carries another flags value or another base fails the run
// naming that name, and a counter name that equals an attached metric label
// fails the run naming the collision; both print a `FAILED:` line and leave
// the entry point with exit status 1 (FR-008, FR-036).
//
// Each mode owns one provider script, so it runs in a re-exec of this binary,
// the way the pause test's scripted scenarios do. Frameworkless check()/fail()
// convention.
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

#include <sys/wait.h>
#include <unistd.h>

#include "detail/gcov_flush.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 800;
constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kRepetitions = 3;

// The primary of a default run is the CPU column, so the divisor of a `RATE`
// counter is the CPU fold of the row in seconds (FR-004).
constexpr double kCpuFoldSeconds = static_cast<double>(kThreadCpuStep) * 1e-9;

// One step of each leaf over the iteration count.
constexpr double kCpuPerIteration =
    static_cast<double>(kThreadCpuStep) / static_cast<double>(kIterations);
constexpr double kRealPerIteration =
    static_cast<double>(kMonotonicStep) / static_cast<double>(kIterations);

// The longest name a run may open is `kMaxCounterNameBytes` minus one bytes
// (FR-002). Sixteen names is the cap of one benchmark (FR-001).
constexpr std::size_t kNameBound =
    static_cast<std::size_t>(sg::kMaxCounterNameBytes);
constexpr std::size_t kNameCap = static_cast<std::size_t>(sg::kMaxUserCounters);

sg::counters::FakeProvider* fake = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS USER COUNTERS TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Registers the two machine leaves every scenario needs. The CPU step is a
// parameter so the zero-divisor scenario can script a fold of zero while the
// real column keeps its measurement.
auto scriptProvider(const std::uint64_t cpuStep) -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addObject("pause", "pause", "the counter test object");
  owned->addCounter("pause", "ops", "none", "an event leaf");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, cpuStep);
  owned->setPoints("pause", "ops", {0}, 200);
  fake = owned.get();
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// Attaches a metric carrying the given label, the shape that makes a counter
// name collide with a metric label (FR-008).
auto attachMetric(sg::BenchmarkHandle& handle, const char* label) -> void
{
  const auto object = sg::counters::System::local().object("pause");
  check(object.has_value(), "the metric object resolves");
  const auto leaf = object->counter<sg::counters::Dim<0, 1>>("ops");
  check(leaf.has_value(), "the metric leaf resolves");
  handle.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*leaf),
                   label);
}

// FR-001, FR-006: three names opened in one run, one flag-neutral column each
// apart from the thread average, which divides by one while threads stand out
// of scope (FR-004, Q-3). The open order is the column order.
auto bmNeutral(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("widgets", {64.0});
      state.setCounter("kib",
                       {64.0, sg::CounterFlag::NONE, sg::CounterBase::K1024});
      state.setCounter("thr", {3000.0, sg::CounterFlag::AVG_THREADS});
    }
    sg::doNotOptimize(index);
  }
}

// FR-003, FR-004: the rate divides by the primary time of the row in seconds.
auto bmRate(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("bytes",
                       {4096.0, sg::CounterFlag::RATE, sg::CounterBase::K1024});
    }
    sg::doNotOptimize(index);
  }
}

// FR-003: the iteration-invariant factor multiplies by the iteration count in
// its own place in the order.
auto bmRateInvariant(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter(
          "bytes",
          {4096.0,
           sg::CounterFlag::RATE | sg::CounterFlag::ITERATION_INVARIANT,
           sg::CounterBase::K1024});
    }
    sg::doNotOptimize(index);
  }
}

// FR-003: the average-over-iterations divisor, alone and beside the rate.
auto bmAvgIterations(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("bytes",
                       {4096.0,
                        sg::CounterFlag::RATE | sg::CounterFlag::AVG_ITERATIONS,
                        sg::CounterBase::K1024});
      state.setCounter("per", {400.0, sg::CounterFlag::AVG_ITERATIONS});
    }
    sg::doNotOptimize(index);
  }
}

// FR-003: the iteration-invariant factor on a counter of no other flag.
auto bmInvariant(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("total", {5000.0, sg::CounterFlag::ITERATION_INVARIANT});
    }
    sg::doNotOptimize(index);
  }
}

// FR-003: the invert reciprocal runs last, and nothing scales after it. A
// value of 0.001 inverts to 1000 under the K1024 base, while the same
// value with no flag leaves the column at zero to three decimals.
auto bmInvert(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("inv", {0.001, sg::CounterFlag::INVERT});
      state.setCounter("thousand", {1000.0});
    }
    sg::doNotOptimize(index);
  }
}

// FR-003: `INVERT` stands after the rate. The value is the primary fold over
// 1024, so rate then invert prints exactly 1.000. An invert applied ahead
// of the rate would print 1.6e15 instead.
auto bmInvertRate(sg::State& state) -> void
{
  constexpr double kValue = kCpuFoldSeconds;
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("invrate",
                       {kValue,
                        sg::CounterFlag::RATE | sg::CounterFlag::INVERT,
                        sg::CounterBase::K1024});
    }
    sg::doNotOptimize(index);
  }
}

// FR-002: two flag sets combined while the row runs. Both operands of the join
// arrive through the call below, so `operator|` executes on this path and the
// column prints the reciprocal of its own rate, as the constant spelling does.
// The call keeps the join a value of the run, which the optimizer cannot fold.
[[gnu::noinline]] auto joinOperand(const bool rate) -> sg::CounterFlag
{
  return rate ? sg::CounterFlag::RATE : sg::CounterFlag::INVERT;
}

auto bmInvertRateJoin(sg::State& state) -> void
{
  const auto rate = joinOperand(true);
  const auto invert = joinOperand(false);
  for (auto index : state) {
    if (index == 0) {
      state.setCounter(
          "invrun", {kCpuFoldSeconds, rate | invert, sg::CounterBase::K1024});
    }
    sg::doNotOptimize(index);
  }
}

// FR-005: the primary divisor of the row is zero, so the column carries the
// flag-neutral value and a disclosure naming the zero divisor.
auto bmZeroDivisor(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("bytes",
                       {4096.0, sg::CounterFlag::RATE, sg::CounterBase::K1024});
    }
    sg::doNotOptimize(index);
  }
}

// FR-005: a zero primary divisor stands aside for the time divisor alone, so
// the two iteration flags still scale their values on the same row.
auto bmZeroDivisorFlags(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("cells", {1024.0, sg::CounterFlag::ITERATION_INVARIANT});
      state.setCounter("shared", {1024.0, sg::CounterFlag::AVG_ITERATIONS});
    }
    sg::doNotOptimize(index);
  }
}

// FR-005: `INVERT` of a value of zero has no reciprocal the row could print,
// so the flag-neutral value stands and the column stays finite.
auto bmInvertZero(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("nothing", {0.0, sg::CounterFlag::INVERT});
    }
    sg::doNotOptimize(index);
  }
}

// FR-005: the primary fold of the row carries a disclosure, so the column
// carries the flag-neutral value beside the disclosure of the fold.
auto bmDisclosedRate(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("bytes",
                       {4096.0, sg::CounterFlag::RATE, sg::CounterBase::K1024});
    }
    sg::doNotOptimize(index);
  }
}

// FR-001: the value of a used name is the last write of the run, and a used
// name keeps the flags and the base of its first open.
auto bmLastWrite(sg::State& state) -> void
{
  for (auto index : state) {
    state.setCounter("ticks", {static_cast<double>(index + 1)});
  }
}

// FR-008: the second open of a used name carries other flags.
auto bmFlagChange(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("flips", {5000.0});
    }
    if (index == 1) {
      state.setCounter("flips", {5000.0, sg::CounterFlag::RATE});
    }
    sg::doNotOptimize(index);
  }
}

// FR-008: the second open of a used name carries another base.
auto bmBaseChange(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("scales",
                       {5000.0, sg::CounterFlag::NONE, sg::CounterBase::K1024});
    }
    if (index == 1) {
      state.setCounter("scales",
                       {5000.0, sg::CounterFlag::NONE, sg::CounterBase::K1000});
    }
    sg::doNotOptimize(index);
  }
}

// FR-008: the counter name equals the label of the attached metric.
auto bmCollision(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter("widgets", {64.0});
    }
    sg::doNotOptimize(index);
  }
}

// FR-001: the seventeenth distinct name of one benchmark.
auto bmSeventeen(sg::State& state) -> void
{
  static const char* const kNames[] = {"c00",
                                       "c01",
                                       "c02",
                                       "c03",
                                       "c04",
                                       "c05",
                                       "c06",
                                       "c07",
                                       "c08",
                                       "c09",
                                       "c10",
                                       "c11",
                                       "c12",
                                       "c13",
                                       "c14",
                                       "c15",
                                       "c16"};
  for (auto index : state) {
    if (index == 0) {
      for (const char* const name : kNames) {
        state.setCounter(name, {1000.0});
      }
    }
    sg::doNotOptimize(index);
  }
}

// FR-002: a name of `kMaxCounterNameBytes` bytes.
auto bmLongName(sg::State& state) -> void
{
  const std::string name(kNameBound, 'n');
  for (auto index : state) {
    if (index == 0) {
      state.setCounter(name, {1000.0});
    }
    sg::doNotOptimize(index);
  }
}

// Captures one run of one benchmark and the exit status of the entry point.
// Every mode of this binary shares the one output file, so the modes run one
// after another.
auto captureRun(const std::vector<std::string>& arguments,
                int& status) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_user_counters_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_user_counters_out.txt";
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

// The measured rows of a run, the aggregate row and the per-counter statistic
// lines excluded by the absence of the `iterations=` field they all carry
// apart from the aggregate family.
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

// FR-006, SC-006: the aggregate family carries one statistic line per named
// counter, in the order the names opened.
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

// FR-001, FR-006: three columns after the metric fields, in open order, each
// the flag-applied value over its base, and three statistic lines in the same
// order over the three clean repetitions.
auto scenarioNeutral() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmNeutral, "bmNeutral");
  check(handle.name() == "bmNeutral", "the neutral benchmark registers");

  const std::string report = captureGoodRun("^bmNeutral$");
  const std::vector<std::string> lines = linesOf(report, "bmNeutral");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "widgets="), 64.0),
          "a flag-neutral counter prints its value under the K1000 base "
          "(FR-006)");
    check(closeTo(fieldOf(row, "kib="), 64.0),
          "a K1024 counter prints the same value as it stands (FR-006)");
    check(closeTo(fieldOf(row, "thr="), 3000.0),
          "the thread average divides by one while threads stand out of scope "
          "(FR-004, Q-3)");
    const std::size_t samples = indexOf(row, "endpoint samples=");
    const std::size_t widgets = indexOf(row, "widgets=");
    const std::size_t kib = indexOf(row, "kib=");
    const std::size_t thr = indexOf(row, "thr=");
    check(samples < widgets && widgets < kib && kib < thr,
          "the counter columns follow the metric fields in the order the "
          "names opened (FR-006)");
  }

  const std::string widgetsLine = statisticLineOf(lines, "widgets");
  const std::string kibLine = statisticLineOf(lines, "kib");
  const std::string thrLine = statisticLineOf(lines, "thr");
  check(closeTo(fieldOf(widgetsLine, "mean="), 64.0)
            && closeTo(fieldOf(kibLine, "mean="), 64.0)
            && closeTo(fieldOf(thrLine, "mean="), 3000.0),
        "each named counter carries the statistic set over the clean "
        "repetitions (FR-006)");
  check(indexOf(widgetsLine, "n=3") < widgetsLine.size(),
        "the counter statistic set counts the clean repetitions (FR-006)");
  check(linesOf(report, "bmNeutral").size() == kRepetitions + 4,
        "the aggregate row prints one statistic line per named counter "
        "(FR-006)");
  return 0;
}

// FR-003, FR-004: the rate divisor is the primary time of the row in seconds,
// the CPU fold of a default run.
auto scenarioRate() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmRate, "bmRate");
  check(handle.name() == "bmRate", "the rate benchmark registers");

  const std::string report = captureGoodRun("^bmRate$");
  const std::vector<std::string> lines = linesOf(report, "bmRate");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double expected = 4096.0 / kCpuFoldSeconds;
  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "bytes="), expected),
          "the rate column divides by the primary time of the row in seconds "
          "and by the base (FR-003, FR-004)");
    check(closeTo(fieldOf(row, "cpu time/iter (ns)="), kCpuPerIteration),
          "the CPU column of the script is 200 ns per iteration (FR-001)");
  }
  return 0;
}

// FR-003: the iteration-invariant factor in its place in the order.
auto scenarioRateInvariant() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmRateInvariant, "bmRateInvariant");
  check(handle.name() == "bmRateInvariant",
        "the invariant benchmark registers");

  const std::string report = captureGoodRun("^bmRateInvariant$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmRateInvariant"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double expected =
      4096.0 / kCpuFoldSeconds * static_cast<double>(kIterations);
  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "bytes="), expected),
          "the iteration-invariant factor multiplies by the iteration count "
          "in the rate pass (FR-003)");
  }
  return 0;
}

// FR-003: the average-over-iterations divisor.
auto scenarioAvgIterations() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmAvgIterations, "bmAvgIterations");
  check(handle.name() == "bmAvgIterations", "the average benchmark registers");

  const std::string report = captureGoodRun("^bmAvgIterations$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmAvgIterations"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double rate =
      4096.0 / kCpuFoldSeconds / static_cast<double>(kIterations);
  const double plain = 400.0 / static_cast<double>(kIterations);
  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "bytes="), rate),
          "the average-over-iterations divisor runs after the rate (FR-003)");
    check(closeTo(fieldOf(row, "per="), plain),
          "the average-over-iterations divisor runs on its own too (FR-003)");
  }
  return 0;
}

// FR-003: the iteration-invariant factor on a counter of no other flag.
auto scenarioInvariant() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmInvariant, "bmInvariant");
  check(handle.name() == "bmInvariant", "the invariant benchmark registers");

  const std::string report = captureGoodRun("^bmInvariant$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmInvariant"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double expected = 5000.0 * static_cast<double>(kIterations);
  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "total="), expected),
          "the iteration-invariant factor multiplies by the iteration count "
          "(FR-003)");
  }
  return 0;
}

// FR-003: the invert reciprocal runs last, ahead of the base scaling alone.
auto scenarioInvert() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmInvert, "bmInvert");
  check(handle.name() == "bmInvert", "the invert benchmark registers");

  const std::string report = captureGoodRun("^bmInvert$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmInvert"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (const auto& row : rows) {
    check(
        closeTo(fieldOf(row, "inv="), 1000.0),
        "the invert column is the reciprocal of the value it holds " "(FR-"
                                                                     "003)");
    check(closeTo(fieldOf(row, "thousand="), 1000.0),
          "a flag-neutral counter of the same run prints the same value "
          "(FR-006)");
  }
  return 0;
}

// FR-003: `INVERT` stands after the rate, the position that fixes the order.
auto scenarioInvertRate() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmInvertRate, "bmInvertRate");
  check(handle.name() == "bmInvertRate", "the invert rate benchmark registers");

  const std::string report = captureGoodRun("^bmInvertRate$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmInvertRate"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "invrate="), 1.0),
          "the invert reciprocal runs after the rate division (FR-003)");
  }
  return 0;
}

// FR-002, FR-003: a flag set the row builds at run time reaches the same
// order: the rate division first, the reciprocal last, one printed column.
auto scenarioInvertRateJoin() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmInvertRateJoin, "bmInvertRateJoin");
  check(handle.name() == "bmInvertRateJoin",
        "the run-time join benchmark registers");

  const std::string report = captureGoodRun("^bmInvertRateJoin$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmInvertRateJoin"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "invrun="), 1.0),
          "the joined flags scale the column in the order of the flags "
          "(FR-002, FR-003)");
  }
  return 0;
}

// FR-005: a zero primary divisor leaves the flag-neutral value with a
// disclosure, and the row stands aside from the counter's statistics.
auto scenarioZeroDivisor() -> int
{
  scriptProvider(0);
  auto handle = sg::registerBenchmark(&bmZeroDivisor, "bmZeroDivisor");
  check(handle.name() == "bmZeroDivisor",
        "the zero divisor benchmark registers");

  const std::string report = captureGoodRun("^bmZeroDivisor$");
  const std::vector<std::string> lines = linesOf(report, "bmZeroDivisor");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "cpu time/iter (ns)="), 0.0),
          "the script leaves the CPU fold at zero (FR-005)");
    check(closeTo(fieldOf(row, "real time/iter (ns)="), kRealPerIteration),
          "the real column keeps its measurement (FR-005)");
    check(closeTo(fieldOf(row, "bytes="), 4096.0),
          "the column carries the flag-neutral value over the base (FR-005)");
    check(row.contains("zero-divisor"),
          "the row discloses the zero divisor (FR-005)");
  }

  const std::string bytesLine = statisticLineOf(lines, "bytes");
  check(bytesLine.contains("n=0"),
        "a disclosed row stands aside from that counter's statistics (FR-005)");
  return 0;
}

// FR-005: with the primary divisor at zero the iteration flags still bind, so
// one column shows the value times the iterations and the other the value over
// them, and neither turns into an infinity.
auto scenarioZeroDivisorFlags() -> int
{
  scriptProvider(0);
  auto handle =
      sg::registerBenchmark(&bmZeroDivisorFlags, "bmZeroDivisorFlags");
  check(handle.name() == "bmZeroDivisorFlags",
        "the zero divisor flag benchmark registers");

  const std::string report = captureGoodRun("^bmZeroDivisorFlags$");
  const std::vector<std::string> lines = linesOf(report, "bmZeroDivisorFlags");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-005)");
  checkFinite(rows);

  for (const auto& row : rows) {
    const double iterations = fieldOf(row, "iterations=");
    check(iterations > 0.0, "the row states the iterations it ran (FR-005)");
    check(closeTo(fieldOf(row, "cells="), 1024.0 * iterations),
          "the invariant column scales by the iterations (FR-005)");
    check(closeTo(fieldOf(row, "shared="), 1024.0 / iterations),
          "the average column divides by the iterations (FR-005)");
    check(row.contains("zero-divisor"),
          "the row still discloses the zero divisor (FR-005)");
  }
  return 0;
}

// FR-005: an inverted zero prints the flag-neutral value and stays finite.
auto scenarioInvertZero() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmInvertZero, "bmInvertZero");
  check(handle.name() == "bmInvertZero",
        "the inverted zero benchmark registers");

  const std::string report = captureGoodRun("^bmInvertZero$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmInvertZero"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "nothing="), 0.0),
          "a value of zero under INVERT prints the flag-neutral value "
          "(FR-005)");
  }
  return 0;
}

// FR-005: a repetition whose primary fold carries a disclosure prints the
// flag-neutral value with the disclosure and stands aside from the counter's
// statistics. One sampling window holds the whole plan, and two captures of
// one plan hold the same number of actions, so the length of the first window
// names the second window's measured endpoints: the calibration reads come
// first and the endpoint pairs of the repetitions are the last two actions of
// each, so the opening endpoint of the last repetition is one action before
// the end of the window. There the fold keeps a value the row can name beside
// its disclosure; on a closing endpoint the fold would read zero and disclose
// the zero divisor instead.
auto scenarioFoldDisclosed() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmDisclosedRate, "bmDisclosedRate");
  check(handle.name() == "bmDisclosedRate",
        "the disclosed benchmark registers");

  const double rate = 4096.0 / kCpuFoldSeconds;
  const std::uint64_t before = fake->readActions();
  const std::vector<std::string> clean = measuredRowsOf(
      linesOf(captureGoodRun("^bmDisclosedRate$"), "bmDisclosedRate"));
  const std::uint64_t window = fake->readActions() - before;
  check(clean.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  for (const auto& row : clean) {
    check(closeTo(fieldOf(row, "bytes="), rate),
          "a run whose fold carries no gap keeps the rate value (FR-003)");
    check(!row.contains("disclosed") && !row.contains("zero-divisor"),
          "a clean row carries no disclosure (FR-005)");
  }

  fake->setGapActions("machine", "thread_cpu", {window - 1});
  const std::string report = captureGoodRun("^bmDisclosedRate$");
  const std::vector<std::string> lines = linesOf(report, "bmDisclosedRate");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  std::size_t disclosed = 0;
  for (const auto& row : rows) {
    if (row.contains("fold-disclosed")) {
      ++disclosed;
    }
  }
  check(disclosed == 1, "the gapped repetition discloses its fold (FR-005)");
  for (const auto& row : rows) {
    const double value = fieldOf(row, "bytes=");
    check(closeTo(value, rate) || closeTo(value, 4096.0),
          "every row prints either the rate or the flag-neutral value "
          "(FR-003, FR-005)");
    if (row.contains("fold-disclosed")) {
      check(closeTo(value, 4096.0),
            "the disclosed row prints the flag-neutral value (FR-005)");
    }
  }

  const std::string bytesLine = statisticLineOf(lines, "bytes");
  check(bytesLine.contains("n=2"),
        "the disclosed repetition stands aside from that counter's "
        "statistics (FR-005)");
  check(
      closeTo(fieldOf(bytesLine, "mean="), rate),
      "the counter statistic set summarizes the clean repetitions " "(FR-006)");
  return 0;
}

// FR-001: the value of a used name is the last write, and the flags and the
// base of the first open stand.
auto scenarioLastWrite() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmLastWrite, "bmLastWrite");
  check(handle.name() == "bmLastWrite", "the last write benchmark registers");

  const std::string report = captureGoodRun("^bmLastWrite$");
  const std::vector<std::string> rows =
      measuredRowsOf(linesOf(report, "bmLastWrite"));
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");
  checkFinite(rows);

  const double expected = static_cast<double>(kIterations);
  for (const auto& row : rows) {
    check(closeTo(fieldOf(row, "ticks="), expected),
          "the column carries the last value written to the name (FR-001)");
  }
  return 0;
}

// FR-008: a used name written with other flags fails the run naming it.
auto scenarioFlagChange() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmFlagChange, "bmFlagChange");
  check(handle.name() == "bmFlagChange", "the breach benchmark registers");

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmFlagChange$", "--iterations=4", "--repetitions=3"},
      status);
  check(status == 1,
        "a failed run leaves the entry point with status 1 " "(FR-036)");
  const std::vector<std::string> lines = linesOf(report, "bmFlagChange");
  check(lines.size() == 1, "the failed run prints its one failure line");
  check(lines[0].contains("FAILED:"),
        "the failure line names the run as failed (FR-008)");
  check(lines[0].contains("flips"),
        "the failure reason names the counter name (FR-008)");
  return 0;
}

auto scenarioBaseChange() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmBaseChange, "bmBaseChange");
  check(handle.name() == "bmBaseChange", "the base breach benchmark registers");

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmBaseChange$", "--iterations=4", "--repetitions=3"},
      status);
  check(status == 1,
        "a failed run leaves the entry point with status 1 (FR-036)");
  const std::vector<std::string> lines = linesOf(report, "bmBaseChange");
  check(lines.size() == 1, "the failed run prints its one failure line");
  check(lines[0].contains("FAILED:"),
        "the failure line names the run as failed (FR-008)");
  check(lines[0].contains("scales"),
        "the failure reason names the counter name (FR-008)");
  return 0;
}

// FR-008: a counter name equal to an attached metric label fails the run.
auto scenarioCollision() -> int
{
  scriptProvider(kThreadCpuStep);
  auto handle = sg::registerBenchmark(&bmCollision, "bmCollision");
  attachMetric(handle, "widgets");
  check(handle.name() == "bmCollision", "the collision benchmark registers");

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bmCollision$", "--iterations=4", "--repetitions=3"},
      status);
  check(status == 1,
        "a failed run leaves the entry point with status 1 " "(FR-036)");
  const std::vector<std::string> lines = linesOf(report, "bmCollision");
  check(lines.size() == 1, "the failed run prints its one failure line");
  check(lines[0].contains("FAILED:"),
        "the failure line names the run as failed (FR-008)");
  check(lines[0].contains("widgets"),
        "the failure reason names the colliding name (FR-008)");
  return 0;
}

// FR-001, FR-002: each breach ends its child through the contract facility,
// which writes the message to stderr and aborts. The parent forks and execs
// directly and reads the number out of the captured message.
auto runBreach(const std::string& self,
               const char* mode,
               const char* const expected) -> void
{
  const char* const path = "harness_user_counters_breach.txt";
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the breached child");
  }
  if (pid == 0) {
    if (std::freopen(path, "w", stderr) == nullptr) {
      _exit(2);
    }
    char* const execArgv[] = {
        const_cast<char*>(self.c_str()), const_cast<char*>(mode), nullptr};
    (void)execv(self.c_str(), execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the breached child");
  }
  check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
        "the breach ends the signaled child (FR-001, FR-002)");

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string message = buffer.str();
  if (!message.contains(expected)) {
    std::fprintf(stderr, "no '%s' in '%s'\n", expected, message.c_str());
    fail("the breach message states no such number");
  }
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::vector<std::string> arguments(argv, argv + argc);
  if (arguments.size() == 2) {
    sg::test::flushCountersOnAbort();
    const std::string& mode = arguments[1];
    if (mode == "neutral") {
      return scenarioNeutral();
    }
    if (mode == "rate") {
      return scenarioRate();
    }
    if (mode == "rateInvariant") {
      return scenarioRateInvariant();
    }
    if (mode == "avgIterations") {
      return scenarioAvgIterations();
    }
    if (mode == "invariant") {
      return scenarioInvariant();
    }
    if (mode == "invert") {
      return scenarioInvert();
    }
    if (mode == "invertRate") {
      return scenarioInvertRate();
    }
    if (mode == "invertRateJoin") {
      return scenarioInvertRateJoin();
    }
    if (mode == "zeroDivisor") {
      return scenarioZeroDivisor();
    }
    if (mode == "invertZero") {
      return scenarioInvertZero();
    }
    if (mode == "zeroDivisorFlags") {
      return scenarioZeroDivisorFlags();
    }
    if (mode == "foldDisclosed") {
      return scenarioFoldDisclosed();
    }
    if (mode == "lastWrite") {
      return scenarioLastWrite();
    }
    if (mode == "flagChange") {
      return scenarioFlagChange();
    }
    if (mode == "baseChange") {
      return scenarioBaseChange();
    }
    if (mode == "collision") {
      return scenarioCollision();
    }
    if (mode == "seventeen") {
      scriptProvider(kThreadCpuStep);
      const auto handle = sg::registerBenchmark(&bmSeventeen, "bmSeventeen");
      check(handle.name() == "bmSeventeen", "the cap benchmark registers");
      int status = 0;
      (void)captureRun(
          {"--filter", "^bmSeventeen$", "--iterations=4", "--repetitions=3"},
          status);
      return 0;
    }
    if (mode == "longName") {
      scriptProvider(kThreadCpuStep);
      const auto handle = sg::registerBenchmark(&bmLongName, "bmLongName");
      check(handle.name() == "bmLongName", "the bound benchmark registers");
      int status = 0;
      (void)captureRun(
          {"--filter", "^bmLongName$", "--iterations=4", "--repetitions=3"},
          status);
      return 0;
    }
    fail("unknown mode");
  }

  const std::string self = argv[0];
  for (const auto& scenario : {"neutral",
                               "rate",
                               "rateInvariant",
                               "avgIterations",
                               "invariant",
                               "invert",
                               "invertRate",
                               "invertRateJoin",
                               "invertZero",
                               "foldDisclosed",
                               "zeroDivisor",
                               "zeroDivisorFlags",
                               "lastWrite",
                               "flagChange",
                               "baseChange",
                               "collision"})
  {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (FR-001 to FR-008)");
  }

  check(kNameCap == 16, "the cap of one benchmark is sixteen names (FR-001)");
  check(kNameBound == 32, "the name bound is 32 bytes (FR-002)");
  runBreach(self, "seventeen", "sixteen");
  runBreach(self, "longName", "31");

  std::puts("harness_user_counters_test: ok");
  return 0;
}
