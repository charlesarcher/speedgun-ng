// ============================================================================
// TDD test for the aggregate-only modes (T024; US6, FR-016 to FR-018).
//
// `BenchmarkHandle::reportAggregatesOnly` restricts the report of its family
// to the aggregate row, and `displayAggregatesOnly` trims the console the same
// way while the rows themselves stay in the result (FR-016, FR-017). The
// console is the only output channel today, so both modes trim it and this
// suite reads the identical trim in each mode (FR-017). Neither mode hides a
// row that has no aggregate, so a family of one repetition prints its single
// row under either mode (FR-018), and the trim belongs to the family that
// asked for it: a second family of the same run keeps every repetition row.
//
// The scripted provider steps each machine time leaf by a constant per
// sampling action, so a three-repetition family prints three measured rows
// beside its aggregate and a one-repetition family prints one row with no
// aggregate at all.
//
// Each mode owns one provider script, so it runs in a re-exec of this binary.
// Frameworkless check()/fail() convention.
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

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The rows one family of the run owns: three repetitions beside one aggregate.
constexpr std::size_t kRepetitions = 3;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS AGGREGATES ONLY TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Registers the two machine leaves every scenario needs, each stepping by a
// constant per sampling action so every run of every family measures.
auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->setPoints("machine", "monotonic", {0}, 1000);
  owned->setPoints("machine", "thread_cpu", {0}, 800);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

auto bmMode(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int& status) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_aggregates_only_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_aggregates_only_out.txt";
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

auto captureGoodRun(const std::vector<std::string>& arguments) -> std::string
{
  int status = 0;
  const std::string report = captureRun(arguments, status);
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

auto countRows(const std::vector<std::string>& lines) -> std::size_t
{
  std::size_t rows = 0;
  for (const auto& line : lines) {
    if (line.contains("iterations=")) {
      ++rows;
    }
  }
  return rows;
}

auto hasAggregate(const std::vector<std::string>& lines) -> bool
{
  for (const auto& line : lines) {
    if (line.contains("AGGREGATE")) {
      return true;
    }
  }
  return false;
}

// FR-016: the family that asked to report aggregates alone shows no
// repetition row, and the family beside it keeps all of its rows (E-07).
auto scenarioReportOnly() -> int
{
  scriptProvider();
  auto trimmed = sg::registerBenchmark(&bmMode, "bmTrimmed");
  check(trimmed.name() == "bmTrimmed", "the trimmed benchmark registers");
  trimmed.reportAggregatesOnly();
  auto plain = sg::registerBenchmark(&bmMode, "bmPlain");
  check(plain.name() == "bmPlain", "the plain benchmark registers");

  const std::string report =
      captureGoodRun({"--filter", "^bm", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> trimmedLines = linesOf(report, "bmTrimmed");
  check(countRows(trimmedLines) == 0,
        "the report mode prints no repetition row (FR-016)");
  check(hasAggregate(trimmedLines),
        "the report mode prints the aggregate row (FR-016)");

  const std::vector<std::string> plainLines = linesOf(report, "bmPlain");
  check(countRows(plainLines) == kRepetitions,
        "the trim belongs to the family that asked for it (FR-016, E-07)");
  check(hasAggregate(plainLines),
        "the plain family keeps its aggregate row (FR-027)");
  return 0;
}

// FR-017: the console display mode trims the console the same way. The console
// is the only channel the mode is defined against today, so the stored rows
// reach no other reader and the printed trim is the whole observable rule.
auto scenarioDisplayOnly() -> int
{
  scriptProvider();
  auto trimmed = sg::registerBenchmark(&bmMode, "bmTrimmed");
  check(trimmed.name() == "bmTrimmed", "the trimmed benchmark registers");
  trimmed.displayAggregatesOnly();
  auto plain = sg::registerBenchmark(&bmMode, "bmPlain");
  check(plain.name() == "bmPlain", "the plain benchmark registers");

  const std::string report =
      captureGoodRun({"--filter", "^bm", "--iterations=4", "--repetitions=3"});
  const std::vector<std::string> trimmedLines = linesOf(report, "bmTrimmed");
  check(countRows(trimmedLines) == 0,
        "the display mode trims the repetition rows of the console (FR-017)");
  check(hasAggregate(trimmedLines),
        "the display mode prints the aggregate row (FR-017)");
  check(countRows(linesOf(report, "bmPlain")) == kRepetitions,
        "the plain family keeps every repetition row (FR-017)");
  return 0;
}

// FR-018: a family of one run holds no aggregate, and neither mode may hide
// the row it does have.
auto scenarioSingleRun(const bool displayMode) -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmMode, "bmSingle");
  check(handle.name() == "bmSingle", "the single-run benchmark registers");
  if (displayMode) {
    handle.displayAggregatesOnly();
  } else {
    handle.reportAggregatesOnly();
  }

  const std::string report = captureGoodRun(
      {"--filter", "^bmSingle$", "--iterations=4", "--repetitions=1"});
  const std::vector<std::string> lines = linesOf(report, "bmSingle");
  check(countRows(lines) == 1,
        "a single-run instance prints its one row under the mode (FR-018)");
  check(!hasAggregate(lines),
        "a single-run instance holds no aggregate row (FR-018)");
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::vector<std::string> arguments(argv, argv + argc);
  if (arguments.size() == 2) {
    const std::string& mode = arguments[1];
    if (mode == "reportOnly") {
      return scenarioReportOnly();
    }
    if (mode == "displayOnly") {
      return scenarioDisplayOnly();
    }
    if (mode == "singleReport") {
      return scenarioSingleRun(false);
    }
    if (mode == "singleDisplay") {
      return scenarioSingleRun(true);
    }
    fail("unknown mode");
  }

  const std::string self = argv[0];
  for (const auto& scenario :
       {"reportOnly", "displayOnly", "singleReport", "singleDisplay"})
  {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (FR-016 to FR-018)");
  }

  std::puts("harness_aggregates_only_test: ok");
  return 0;
}
