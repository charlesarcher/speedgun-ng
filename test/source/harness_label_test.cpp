// ============================================================================
// TDD test for the row label (T018; US4, FR-011, FR-012, SC-03 pattern).
//
// `State::setLabel` stores one string per repetition, the runner copies it to
// the row, and the report prints ` label=<text>` as the trailing field of the
// row, separated by two spaces and present only when the row carries a label
// (FR-011, the clarification of 2026-10-14, research D-04). The aggregate row
// carries the label only when every repetition shares it, the shared-label
// rule of the cited `ComputeStats` (FR-012).
//
// Placement decides what a row without a label looks like: FR-011 makes the
// field trailing, so an unlabelled row keeps the bytes of the name column and
// of every built-in field after it, which is what the report-shape checks and
// the README sample block stand on (FR-011, FR-046, SC-02).
//
// A label with a newline breaks the one-line-per-row shape, so it is a
// precondition violation at the call: the child dies through the contract
// facility, which writes the message to stderr and aborts, and the parent
// reads the wait status and the message (spec Edge Cases).
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
constexpr std::uint64_t kRepetitions = 3;

// The name column of the repetition row, padded as the printer pads it, and
// the one space the printer puts between it and the next field. The
// unlabelled row keeps these bytes (FR-011).
constexpr std::size_t kNameColumn = 24;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS LABEL TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
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

// FR-011, FR-012: every repetition carries the same label.
auto bmLabelled(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setLabel("avx2");
    }
    sg::doNotOptimize(index);
  }
}

// FR-011: the row carries no label at all.
auto bmPlain(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// FR-012: the first repetition labels one way, the rest another, so the
// repetitions share no label. Each repetition re-enters the body from the
// first index, so the counter below counts repetitions.
auto bmDisagree(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      static std::uint64_t repetition = 0;
      state.setLabel(repetition++ == 0 ? "avx2" : "avx512");
    }
    sg::doNotOptimize(index);
  }
}

// Spec Edge Cases: the label carries a newline.
auto bmNewline(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setLabel("two\nlines");
    }
    sg::doNotOptimize(index);
  }
}

// Captures one run of one benchmark and the exit status of the entry point.
auto captureRun(const std::vector<std::string>& arguments,
                int& status) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_label_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_label_out.txt";
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

auto aggregateLineOf(const std::vector<std::string>& lines) -> std::string
{
  for (const auto& line : lines) {
    if (line.contains("AGGREGATE")) {
      return line;
    }
  }
  fail("the batch prints no aggregate row");
  return {};
}

auto endsWith(const std::string& line, const std::string& tail) -> bool
{
  return line.size() >= tail.size()
      && line.compare(line.size() - tail.size(), tail.size(), tail) == 0;
}

// FR-011, FR-012: the trailing field on every repetition row and on the
// aggregate row that shares it.
auto scenarioTrailing() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmLabelled, "bmLabelled");
  check(handle.name() == "bmLabelled", "the labeled benchmark registers");

  const std::string report = captureGoodRun("^bmLabelled$");
  const std::vector<std::string> lines = linesOf(report, "bmLabelled");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");

  const std::string field = " label=avx2";
  for (const auto& row : rows) {
    check(endsWith(row, field), "the row ends with the label field (FR-011)");
  }
  check(
      endsWith(aggregateLineOf(lines), field),
      "the aggregate row carries the label every repetition shares " "(FR-"
                                                                     "012)");
  return 0;
}

// FR-011: with no label the row prints no label field and the name column
// keeps its bytes, so the field the label would take leaves the built-in
// fields where they stand.
auto scenarioPlain() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmPlain, "bmPlain");
  check(handle.name() == "bmPlain", "the plain benchmark registers");

  const std::string report = captureGoodRun("^bmPlain$");
  const std::vector<std::string> lines = linesOf(report, "bmPlain");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");

  const std::string nameColumn =
      std::string("bmPlain") + std::string(kNameColumn - 7, ' ');
  for (const auto& row : rows) {
    check(!row.contains("label="),
          "an unlabelled row prints no label field (FR-011)");
    check(row.rfind(nameColumn + " iterations=", 0) == 0,
          "the unlabelled row keeps the bytes of the name column (FR-011)");
  }
  check(
      !aggregateLineOf(lines).contains("label="),
      "the aggregate row of unlabelled repetitions prints no label " "(FR-"
                                                                     "012)");
  return 0;
}

// FR-012: the repetitions disagree, so the aggregate stands unlabeled while
// each repetition keeps its own label.
auto scenarioDisagree() -> int
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmDisagree, "bmDisagree");
  check(handle.name() == "bmDisagree", "the disagreeing benchmark registers");

  const std::string report = captureGoodRun("^bmDisagree$");
  const std::vector<std::string> lines = linesOf(report, "bmDisagree");
  const std::vector<std::string> rows = measuredRowsOf(lines);
  check(rows.size() == kRepetitions,
        "the batch prints one measured row per repetition (FR-006)");

  check(endsWith(rows.front(), " label=avx2"),
        "the first repetition carries the label it set (FR-011)");
  check(endsWith(rows.back(), " label=avx512"),
        "a later repetition carries the label it set, so the last write of "
        "its repetition wins (D-04)");
  check(
      !aggregateLineOf(lines).contains("label="),
      "disagreeing repetition labels leave the aggregate unlabeled " "(FR-"
                                                                     "012)");
  return 0;
}

// Spec Edge Cases: a label with a newline aborts at the call. The mode runs in
// the breached child, so the parent only has to keep the run away.
auto scenarioNewline() -> int
{
  scriptProvider();
  const auto handle = sg::registerBenchmark(&bmNewline, "bmNewline");
  check(handle.name() == "bmNewline", "the newline benchmark registers");
  int status = 0;
  (void)captureRun(
      {"--filter", "^bmNewline$", "--iterations=4", "--repetitions=3"}, status);
  return 0;
}

// The parent forks and execs the breach and reads the wait status and the
// message the contract facility wrote.
auto runBreach(const std::string& self,
               const char* mode,
               const char* const expected) -> void
{
  const char* const path = "harness_label_breach.txt";
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
        "the newline label ends the signaled child (FR-011)");

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string message = buffer.str();
  if (!message.contains(expected)) {
    std::fprintf(stderr, "no '%s' in '%s'\n", expected, message.c_str());
    fail("the breach message states no such rule");
  }
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  const std::vector<std::string> arguments(argv, argv + argc);
  if (arguments.size() == 2) {
    sg::test::flushCountersOnAbort();
    const std::string& mode = arguments[1];
    if (mode == "trailing") {
      return scenarioTrailing();
    }
    if (mode == "plain") {
      return scenarioPlain();
    }
    if (mode == "disagree") {
      return scenarioDisagree();
    }
    if (mode == "newlineLabel") {
      return scenarioNewline();
    }
    fail("unknown mode");
  }

  const std::string self = argv[0];
  for (const auto& scenario : {"trailing", "plain", "disagree"}) {
    const int status = std::system(("\"" + self + "\" " + scenario).c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes (FR-011, FR-012)");
  }

  runBreach(self, "newlineLabel", "newline");

  std::puts("harness_label_test: ok");
  return 0;
}
