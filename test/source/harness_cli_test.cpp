// ============================================================================
// TDD test for the command line (T020, T023; US2, US4, FR-009, FR-015,
// FR-020, FR-021, FR-022, FR-024, FR-034, FR-036, SC-006, SC-007).
//
// A scripted FakeProvider owns the machine time leaves and one events leaf,
// so the option effects are read as exact numbers in the captured report and
// no run can grow into a long calibration. The usage-error cases exit the
// process from inside the entry point, so each is checked in a re-exec of
// this binary with a mode argument. The percentile option cases read the tag
// field, the point labels and the field counts of the printed lines, so none
// of them depends on a measured value. Frameworkless check()/fail()
// convention.
// ============================================================================

#include <cctype>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "../source/harness/detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_metrics.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CLI TEST FAIL: %s\n", what);
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
  // A host whose kernel denies CPU counting (`perf_event_paranoid` 2) leaves
  // every leaf absent, and a ratio of two absent leaves folds to a NaN: the
  // two columns of FR-029 then agree on that NaN, so it counts as equal.
  return (std::isnan(lhs) && std::isnan(rhs)) || std::abs(lhs - rhs) < 1e-9;
}

int alphaRuns = 0;

auto bmAlpha(sg::State& state) -> void
{
  ++alphaRuns;
  for (auto _ : state) {
  }
}

auto bmBeta(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmMetric(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmSkips(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  state.skipWithMessage("the scripted reason");
}

auto bmThrows(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  throw std::runtime_error("the scripted failure");
}

int interruptRuns = 0;

auto bmInterrupts(sg::State& state) -> void
{
  ++interruptRuns;
  for (auto _ : state) {
  }
  (void)std::raise(SIGINT);
}

auto bmAfter(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

int skipBeforePasses = 0;
int skipInsidePasses = 0;

// IF-04: a skip before the loop leaves the loop with no pass at all.
auto bmSkipBefore(sg::State& state) -> void
{
  state.skipWithError("the scripted skip before the loop");
  for (auto _ : state) {
    ++skipBeforePasses;
  }
}

// IF-04: a skip inside the loop records its reason, and the function
// leaves the loop itself.
auto bmSkipInside(sg::State& state) -> void
{
  for (auto _ : state) {
    ++skipInsidePasses;
    if (skipInsidePasses == 3) {
      state.skipWithMessage("the scripted skip inside the loop");
      break;
    }
  }
}

// PR-2 (PR #32): the three ways a run can end without a completed
// timed loop. Each owns its pair, and each reports the reason that
// names the case.
auto bmNoLoop(sg::State& state) -> void
{
  (void)state;
}

int breakNoSkipPasses = 0;

auto bmBreakNoSkip(sg::State& state) -> void
{
  for (auto _ : state) {
    ++breakNoSkipPasses;
    if (breakNoSkipPasses == 2) {
      break;
    }
  }
}

auto bmThrowInside(sg::State& state) -> void
{
  for (auto _ : state) {
    throw std::runtime_error("the scripted throw inside the loop");
  }
}

// The counts SC-006 compares around the failed runs. The listing holds one
// descriptor of its own while it reads, in both measurements, so the
// difference is what the comparison sees.
auto openDescriptorCount() -> std::size_t
{
  std::error_code failure;
  std::size_t count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator("/proc/self/fd", failure))
  {
    static_cast<void>(entry);
    ++count;
  }
  if (failure) {
    fail("the process's own descriptor listing is readable");
  }
  return count;
}

auto mappingCount() -> std::size_t
{
  std::ifstream maps("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

// FR-020, FR-023: a family that asked for the tail, for the precedence pair.
auto bmAsked(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_cli_out.txt";
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
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
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

// The lines of one benchmark's report that carry a tag field. The tag stands
// right-aligned in its own field between the name and the fields that follow
// it, so the needle takes the field on both sides.
auto taggedLinesOf(const std::string& report,
                   const std::string& name,
                   const std::string& tag) -> std::vector<std::string>
{
  const std::string needle = " " + tag + "  ";
  std::vector<std::string> found;
  for (const auto& line : linesOf(report, name)) {
    if (line.find(needle) != std::string::npos) {
      found.push_back(line);
    }
  }
  return found;
}

// FR-024: the point labels of one percentile line, in the order they print.
auto pointLabelsOf(const std::string& line) -> std::vector<std::string>
{
  std::vector<std::string> labels;
  for (std::size_t at = line.find("  p"); at != std::string::npos;
       at = line.find("  p", at + 1))
  {
    std::size_t end = at + 3;
    while (end < line.size()
           && (std::isdigit(static_cast<unsigned char>(line[end])) != 0
               || line[end] == '.'))
    {
      ++end;
    }
    if (end < line.size() && line[end] == '=') {
      labels.push_back(line.substr(at + 2, end - at - 2));
    }
  }
  return labels;
}

auto countOf(const std::string& text, const std::string& needle) -> std::size_t
{
  std::size_t count = 0;
  for (std::size_t at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + needle.size()))
  {
    ++count;
  }
  return count;
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

// How many times `needle` appears in `line`.
[[nodiscard]] auto occurrences(const std::string& line,
                               const std::string& needle) -> std::size_t
{
  std::size_t count = 0;
  for (std::size_t at = line.find(needle); at != std::string::npos;
       at = line.find(needle, at + needle.size()))
  {
    ++count;
  }
  return count;
}

auto subprocessOutput(const std::string& command, std::string& output) -> int
{
  FILE* const pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    fail("the usage subprocess starts");
  }
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  return pclose(pipe);
}

auto scriptedProvider() -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  provider->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider->setPoints("machine", "monotonic", {0}, 4000);
  provider->setPoints("machine", "thread_cpu", {0}, 2000);
  provider->addObject("fake", "fake", "scripted events object");
  provider->addCounter("fake", "events", "none", "scripted events");
  provider->setPoints("fake", "events", {0}, 900);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the fake provider registers");
}

// The two texts the metric option writes when a name carries nothing: the
// failure line with and without near-miss names, and the note of a column
// whose row stands refused (FR-027, FR-028, FR-042).
auto metricFailureTextScenario() -> void
{
  sg::counters::Error near;
  near.message = "no metric named 'cpi_x' in the event table of this host";
  near.suggestions = {"cpi", "ipc"};
  check(sg::detail::metricNameFailureLine("cpi_x", near)
            == "--metric cpi_x: " + near.message + " (near: cpi, ipc)",
        "the failure line names the miss and its near-miss names (FR-042)");

  sg::counters::Error bare;
  bare.message = "this CPU matched no event-table directory";
  check(sg::detail::metricNameFailureLine("cpi", bare)
            == "--metric cpi: " + bare.message,
        "a failure with no near-miss name states none (FR-042)");

  sg::counters::MetricListing refused;
  refused.name = "slots";
  refused.published = false;
  refused.refusal = sg::counters::MetricRefusal::UNKNOWN_CONSTANT;
  const std::vector<sg::counters::MetricListing> listed {refused};
  const std::string kind =
      std::string(sg::counters::metricRefusalName(refused.refusal));
  check(sg::detail::metricColumnNote("slots", bare, listed)
            == bare.message + " (" + kind + ")",
        "an unavailable column names the refusal kind of its row (FR-027)");
  check(sg::detail::metricColumnNote("other", bare, listed) == bare.message,
        "a name the listing carries nothing for states no kind (FR-027)");

  // The listing's published side and the column builder of the run's loop,
  // driven beside the run: the runner reaches them only through a name this
  // machine's table resolves, which no matrix host answers (FR-025, FR-027).
  sg::counters::MetricListing published;
  published.name = "cpi";
  published.published = true;
  const std::vector<sg::counters::MetricListing> both {refused, published};
  check(sg::detail::metricColumnNote("cpi", bare, both) == bare.message,
        "a row the table published carries no refusal kind (FR-027)");

  const std::expected<sg::counters::MetricView, sg::counters::Error> good {
      sg::counters::MetricView {}};
  const auto taken = sg::detail::metricColumnFor("cpi", good, both);
  check(!taken.unavailable && taken.label == "cpi",
        "a resolved view builds an available column of its name (FR-025)");

  const std::expected<sg::counters::MetricView, sg::counters::Error> missing {
      std::unexpected(bare)};
  const char* const notePath = "harness_cli_column_note.txt";
  const int savedNote = dup(STDERR_FILENO);
  if (savedNote < 0 || std::freopen(notePath, "w", stderr) == nullptr) {
    fail("cannot redirect the column note");
  }
  const auto dropped = sg::detail::metricColumnFor("slots", missing, listed);
  std::fflush(stderr);
  if (dup2(savedNote, STDERR_FILENO) < 0) {
    fail("cannot restore stderr for the column note");
  }
  close(savedNote);
  std::ifstream noteFile(notePath);
  std::stringstream noteBuffer;
  noteBuffer << noteFile.rdbuf();
  check(dropped.unavailable && dropped.label == "slots"
            && dropped.availability == sg::counters::Availability::ABSENT,
        "a refusal stands the column unavailable and absent (FR-026)");
  check(noteBuffer.str() == "slots: " + bare.message + " (" + kind + ")\n",
        "the run's line names the refusal kind of the row (FR-026, FR-027)");

  const char* const path = "harness_cli_name_out.txt";
  const int saved = dup(STDERR_FILENO);
  if (saved < 0 || std::freopen(path, "w", stderr) == nullptr) {
    fail("cannot redirect the failure line");
  }
  const int status = sg::detail::metricNameFailure({"cpi_x_no_such_metric"});
  std::fflush(stderr);
  if (dup2(saved, STDERR_FILENO) < 0) {
    fail("cannot restore stderr");
  }
  close(saved);
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  check(status == 2,
        "an unknown metric name ends the walk with status two (FR-028)");
  check(
      buffer.str().find("--metric cpi_x_no_such_metric:") != std::string::npos,
      "the walk prints the failure of the first bad name (FR-042)");
}

}  // namespace

SG_BENCHMARK(bmAlpha);
SG_BENCHMARK(bmBeta);
SG_BENCHMARK(bmSkips);
SG_BENCHMARK(bmThrows);
SG_BENCHMARK(bmInterrupts);
SG_BENCHMARK(bmAfter);
SG_BENCHMARK(bmSkipBefore);
SG_BENCHMARK(bmSkipInside);
SG_BENCHMARK(bmNoLoop);
SG_BENCHMARK(bmBreakNoSkip);
SG_BENCHMARK(bmThrowInside);

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "failthenrun") {
    // IF-01: one failed benchmark and one measured benchmark in a single
    // entry-point call. The run continues past the failure and the exit
    // status stays nonzero.
    scriptedProvider();
    const std::vector<std::string> arguments = {
        "--filter", "^bm(Throws|After)$", "--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  if (mode == "arg" && argc > 2) {
    scriptedProvider();
    std::vector<std::string> arguments = {
        "--filter", "^bmAlpha$", "--iterations=1", argv[2]};
    if (argc > 3) {
      arguments.push_back(argv[3]);
    }
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    const int status =
        sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
    // A usage error exits from inside the entry point, so a missing
    // marker is itself the observation that no benchmark ran.
    std::fprintf(
        stderr, "%s\n", alphaRuns > 0 ? "BENCHMARK_RAN" : "BENCHMARK_NO_RUN");
    return status;
  }

  scriptedProvider();
  auto fixedHandle = sg::registerBenchmark(&bmFixed, "bmFixed");
  fixedHandle.iterations(7);
  auto askedHandle = sg::registerBenchmark(&bmAsked, "bmAsked");
  askedHandle.showPercentiles(true);

  // FR-009: the filter selects the matching set, and the run reports only
  // the selected benchmarks.
  const std::string filtered =
      captureRun({"--filter", "^bmAlpha$", "--iterations=2"});
  check(linesOf(filtered, "bmAlpha").size() == 1,
        "the filter runs the matching benchmark (FR-009)");
  check(linesOf(filtered, "bmBeta").empty(),
        "the filter leaves the unmatched benchmark out (FR-009)");

  // FR-036: a filter that matches nothing reports and exits zero.
  const std::string unmatched = captureRun({"--filter", "^nothingHere$"}, 0);
  check(unmatched.find("bmAlpha") == std::string::npos
            && unmatched.find("bmBeta") == std::string::npos,
        "a filter matching nothing runs no benchmark (FR-036)");

  // FR-009: list mode prints the matching names and runs nothing.
  const std::string listed = captureRun({"--filter", "^bm", "--list"});
  check(linesOf(listed, "bmAlpha").size() == 1
            && linesOf(listed, "bmBeta").size() == 1,
        "list mode prints the matching names (FR-009)");
  check(listed.find("time/iter") == std::string::npos,
        "list mode runs nothing (FR-009)");

  // FR-015: a handle option wins over the command line.
  const std::string handleWins =
      captureRun({"--filter", "^bmFixed$", "--iterations=3"});
  check(countFieldOf(linesOf(handleWins, "bmFixed")[0], "iterations=") == 7,
        "the handle option wins over the command line (FR-015)");

  // FR-012: the repetition count is the number of measured rows.
  const std::string repeated = captureRun(
      {"--filter", "^bmAlpha$", "--iterations=2", "--repetitions=4"});
  check(linesOf(repeated, "bmAlpha").size() == 5,
        "the repetition count drives the row count (FR-012)");

  // FR-021 and FR-022: an address-attached leaf reaches the run, and an
  // events^1 leaf reports per iteration.
  const std::string counter = captureRun(
      {"--filter", "^bmAlpha$", "--iterations=3", "--counter", "fake/events"});
  const std::string counterLine = linesOf(counter, "bmAlpha")[0];
  check(counterLine.find("fake/events=") != std::string::npos,
        "the address-attached leaf reaches the run (FR-021)");
  check(sameDouble(doubleFieldOf(counterLine, "fake/events="), 900.0 / 3.0),
        "the events^1 leaf reports per iteration (FR-022)");

  metricFailureTextScenario();

  // T044 to T047: the `--metric` option. The names come from the table this
  // host selected, so a host that carries no metric row skips the family
  // (FR-025 to FR-029).
  const auto metrics = sg::counters::catalogMetrics();
  std::string first;
  std::string second;
  std::string refusedName;
  std::string refusedKind;
  // A published row can name leaves this host's catalog does not publish,
  // and a spine with no leaf reaches no run plan (FR-046). The family picks
  // names whose spine resolves here.
  const auto resolvable = [](const sg::counters::MetricListing& row)
  {
    const auto view = sg::counters::metricByName(row.name);
    return view.has_value() && !view->expression.core.leaves.empty();
  };
  for (const auto& row : metrics) {
    if (row.published) {
      if (first.empty() && resolvable(row)) {
        first = std::string(row.name);
      } else if (!first.empty() && second.empty() && row.name != first
                 && resolvable(row))
      {
        second = std::string(row.name);
      }
    } else if (refusedName.empty()) {
      refusedName = std::string(row.name);
      refusedKind = std::string(sg::counters::metricRefusalName(row.refusal));
    }
  }

  if (first.empty()) {
    std::printf("SKIP the metric columns: no metric row of this host's table "
                "resolves to a leaf its catalog publishes (FR-025, FR-046)\n");
  } else {
    const auto view = sg::counters::metricByName(first);
    check(view.has_value(), "a published metric resolves (FR-019)");

    auto metricHandle = sg::registerBenchmark(&bmMetric, "bmMetric");
    metricHandle.iterations(3);
    metricHandle.addMetric(view->expression, "attached");

    auto dedupHandle = sg::registerBenchmark(&bmMetric, "bmDedup");
    dedupHandle.iterations(3);
    dedupHandle.addMetric(view->expression, first);

    const std::string columns = captureRun({"--filter",
                                            "^bm(Metric|Dedup)$",
                                            "--iterations=3",
                                            "--metric",
                                            first,
                                            "--metric",
                                            second.empty() ? first : second,
                                            "--metric",
                                            first});
    const std::string namedRow = linesOf(columns, "bmMetric")[0];
    check(namedRow.find(first + "=") != std::string::npos,
          "a named metric adds one column (FR-025, FR-026)");
    check(occurrences(namedRow, first + "=") == 1,
          "the same name twice yields one column (FR-026)");
    if (!second.empty()) {
      check(namedRow.find(second + "=") != std::string::npos,
            "the second name adds its own column (FR-026)");
      check(namedRow.find(first + "=") < namedRow.find(second + "="),
            "the columns stand in command-line order (FR-026)");
    }
    // FR-025, FR-029: the option column and the column attached in code
    // fold the same spine over the same window, so their values agree.
    check(sameDouble(doubleFieldOf(namedRow, "attached="),
                     doubleFieldOf(namedRow, first + "=")),
          "the option column value is the metric the library composes "
          "(FR-025, FR-029)");
    const std::string dedupRow = linesOf(columns, "bmDedup")[0];
    check(occurrences(dedupRow, first + "=") == 1,
          "a metric attached in code and named again yields one column "
          "(FR-026)");
  }

  // FR-034, SC-018: every invalid value of every numeric option fails
  // before any benchmark runs, and the report names the option. A zero
  // warm-up time is the valid case of the same table, and the unknown
  // `--counter` leaf is the FR-023 edge case: it names the leaf and the
  // benchmark still runs.
  struct UsageCase
  {
    const char* argument;
    const char* needle;
    bool runs;
  };

  const std::string self = argv[0];
  for (const UsageCase& testCase :
       {UsageCase {"--repetitions=abc", "--repetitions", false},
        UsageCase {"--repetitions=-1", "--repetitions", false},
        UsageCase {"--repetitions=0", "--repetitions", false},
        UsageCase {"--iterations=abc", "--iterations", false},
        UsageCase {"--iterations=-5", "--iterations", false},
        UsageCase {"--iterations=0", "--iterations", false},
        UsageCase {"--min-time=abc", "--min-time", false},
        UsageCase {"--min-time=-1", "--min-time", false},
        UsageCase {"--min-time=0", "--min-time", false},
        UsageCase {"--warmup-time=abc", "--warmup-time", false},
        UsageCase {"--warmup-time=-1", "--warmup-time", false},
        UsageCase {"--warmup-time=nan", "--warmup-time", false},
        UsageCase {"--warmup-time=inf", "--warmup-time", false},
        UsageCase {"--warmup-time=1e300", "--warmup-time", false},
        UsageCase {"--min-time=inf", "--min-time", false},
        UsageCase {"--min-time=nan", "--min-time", false},
        UsageCase {"--min-time=1e300", "--min-time", false},
        UsageCase {"--warmup-time=1x", "--warmup-time", false},
        UsageCase {"--min-time=1x", "--min-time", false},
        UsageCase {"--warmup-time=1e400", "--warmup-time", false},
        UsageCase {"--min-time=1e400", "--min-time", false},
        UsageCase {"--filter=(", "invalid regular expression", false},
        UsageCase {"--nonsense=1", "unknown option", false},
        UsageCase {"--warmup-time=0", "", true},
        UsageCase {"--counter=machine/nosuchleaf", "no such leaf", true},
        UsageCase {"--metric=nosuchmetric", "no metric named", false},
        UsageCase {"--metric", "unknown option", false}})
  {
    std::string output;
    const int status = subprocessOutput(
        "\"" + self + "\" arg \"" + testCase.argument + "\" 2>&1", output);
    if (testCase.runs) {
      check(status == 0, "a valid value exits zero (SC-018, FR-023)");
    } else {
      check(status != 0, "an invalid value of FR-034 exits nonzero (SC-018)");
    }
    check((output.find("BENCHMARK_RAN") != std::string::npos)
              == testCase.runs,
          "the invalid value runs no benchmark function and the valid one "
          "does (SC-018, FR-023)");
    if (testCase.needle[0] != '\0') {
      check(output.find(testCase.needle) != std::string::npos,
            "the report names the rejected option or leaf (FR-034, FR-023)");
    }
  }

  // FR-042: a metric name the host's table does not carry ends the run with
  // the usage status and one printed line, before any measured output. The
  // subprocess reports its own marker behind that line, so the marker bounds
  // what the command line itself printed.
  std::string unknownOutput;
  const int unknownStatus = subprocessOutput(
      "\"" + self + "\" arg \"--metric=nosuchmetric\" 2>&1", unknownOutput);
  check(WEXITSTATUS(unknownStatus) == 2,
        "an unknown metric name exits with the usage status (FR-042)");
  const auto unknownMarker = unknownOutput.find("BENCHMARK_NO_RUN");
  const std::string unknownPrinted = unknownOutput.substr(0, unknownMarker);
  check(!unknownPrinted.empty()
            && unknownPrinted.find('\n') == unknownPrinted.size() - 1,
        "the unknown metric name prints a single line (FR-042)");
  check(unknownMarker != std::string::npos
            && unknownOutput.find("BENCHMARK_RAN") == std::string::npos,
        "the unknown metric name runs no benchmark function (FR-028)");
  std::string bareOutput;
  const int bareStatus =
      subprocessOutput("\"" + self + "\" arg \"--metric\" 2>&1", bareOutput);
  check(WEXITSTATUS(bareStatus) == 2
            && bareOutput.find("BENCHMARK_RAN") == std::string::npos,
        "a --metric with no value ends the same way (FR-028, FR-042)");

  // FR-027: a metric the host carries but cannot count prints its refusal
  // kind, the benchmark still runs, and the exit status is the run's own.
  if (!refusedName.empty()) {
    std::string output;
    // The name is given twice: one column per named metric holds also for a
    // metric that cannot be counted, so the refusal states itself once
    // (FR-026, FR-027).
    const int status =
        subprocessOutput("\"" + self + "\" arg \"--metric=" + refusedName
                             + "\" \"--metric=" + refusedName + "\" 2>&1",
                         output);
    check(status == 0,
          "a refused metric column keeps the run's own status (FR-027)");
    check(output.find("BENCHMARK_RAN") != std::string::npos,
          "the benchmark runs past a metric it cannot count (FR-027)");
    check(output.find(refusedKind) != std::string::npos,
          "the refusal kind of the metric is printed (FR-027)");
    check(occurrences(output, refusedKind) == 1,
          "the name repeated on the command line yields one refusal (FR-026)");
  }

  // FR-031: a skip prints its reason, and no statistics print.
  const std::string skipped = captureRun(
      {"--filter", "^bmSkips$", "--iterations=1", "--repetitions=3"});
  check(skipped.find("SKIPPED: the scripted reason") != std::string::npos,
        "the skip prints its reason (FR-031)");
  check(skipped.find("AGGREGATE") == std::string::npos,
        "a skipped benchmark prints no statistics (FR-031)");

  // FR-032, SC-006: a throw reports the exception text, releases every
  // descriptor and mapping, and the run continues. The leaf of the real
  // pmu provider is what opens a descriptor per run, so the failed runs
  // are counted against it.
  const std::vector<std::string> failing = {"--filter",
                                            "^bmThrows$",
                                            "--iterations=1",
                                            "--counter",
                                            "cpu/instructions"};
  const std::string thrown = captureRun(failing, 1);
  check(thrown.find("FAILED: the scripted failure") != std::string::npos,
        "the failure reports the exception text (FR-032)");

  captureRun(failing, 1);
  // A first window pays the costs that belong to the process rather than
  // to a run: the lazy symbols of the loader, the locale data, and the
  // first extra chunk of the allocator under a sanitizer. The baseline is
  // read after it, so the measured window still has to hand back every
  // descriptor and mapping it took, which is what a leak of one per run
  // would break.
  constexpr int kFailedRunWindow = 1000;
  for (int run = 0; run < kFailedRunWindow; ++run) {
    captureRun(failing, 1);
  }
  const std::size_t descriptors = openDescriptorCount();
  const std::size_t mappings = mappingCount();
  for (int run = 0; run < kFailedRunWindow; ++run) {
    captureRun(failing, 1);
  }
  check(openDescriptorCount() == descriptors,
        "1,000 failed runs return the descriptor count (SC-006)");
  check(mappingCount() == mappings,
        "1,000 failed runs return the mapping count (SC-006)");

  const std::string after =
      captureRun({"--filter", "^bmAfter$", "--iterations=1"});
  check(after.find("iterations=") != std::string::npos,
        "the next benchmark runs after a failure (FR-032)");

  // SC-017: SIGINT inside the function completes the current run, starts
  // no later run, reports skipped with the interrupt reason, releases the
  // same resources, and exits nonzero.
  const std::string interrupted = captureRun(
      {"--filter", "^bmInterrupts$", "--iterations=1", "--repetitions=3"}, 1);
  check(interrupted.find("SKIPPED: interrupted") != std::string::npos,
        "the interrupt reports skipped with its reason (SC-017)");
  check(interruptRuns == 1,
        "the current run completes and no later run starts (SC-017)");

  // ThreadSanitizer maps its shadow ranges when the process takes its
  // first signal, so the resource pair is read across two identical
  // interrupted runs: the first pays for the runtime, the second has to
  // give everything back.
  interruptRuns = 0;
  const auto heldDescriptors = openDescriptorCount();
  const auto heldMappings = mappingCount();
  const std::string interruptedAgain = captureRun(
      {"--filter", "^bmInterrupts$", "--iterations=1", "--repetitions=3"}, 1);
  check(interruptedAgain.find("SKIPPED: interrupted") != std::string::npos,
        "a second interrupted run reports the same reason (SC-017)");
  check(interruptRuns == 1,
        "the second run completes and no later run starts (SC-017)");
  check(openDescriptorCount() == heldDescriptors
            && mappingCount() == heldMappings,
        "the interrupted run releases the same resources (SC-017)");

  // IF-07, FR-032: the flag is read after every run, the qualifying
  // warm-up run included, so no measured run starts behind it.
  interruptRuns = 0;
  const std::string warmInterrupted = captureRun({"--filter",
                                                  "^bmInterrupts$",
                                                  "--iterations=1",
                                                  "--warmup-time=0.000000001"},
                                                 1);
  check(warmInterrupted.find("SKIPPED: interrupted") != std::string::npos,
        "the warm-up interrupt reports skipped with its reason (FR-032)");
  check(interruptRuns == 1,
        "the flag read after the warm-up run stops the measured run (FR-032)");

  // IF-01, FR-032 and FR-036: a failed benchmark and a measured
  // benchmark share one entry-point call. The run continues, and the
  // status stays nonzero for the whole run.
  std::string failThenRun;
  const int failThenRunStatus =
      subprocessOutput("\"" + self + "\" failthenrun 2>&1", failThenRun);
  check(WEXITSTATUS(failThenRunStatus) == 1,
        "a failure followed by a measured benchmark exits nonzero (FR-036)");
  check(failThenRun.find("FAILED: the scripted failure") != std::string::npos,
        "the failed benchmark reports its exception (FR-032)");
  check(failThenRun.find("bmAfter") != std::string::npos
            && failThenRun.find("iterations=") != std::string::npos,
        "the measured benchmark after the failure carries its row (FR-032)");

  // IF-04: a skip before the loop leaves no pass, and a skip inside the
  // loop reports its reason once the function leaves the loop.
  skipBeforePasses = 0;
  const std::string skipBefore =
      captureRun({"--filter", "^bmSkipBefore$", "--iterations=5"});
  check(skipBefore.find("SKIPPED: the scripted skip before the loop")
            != std::string::npos,
        "the skip before the loop reports its reason (FR-031)");
  check(skipBeforePasses == 0,
        "the skip before the loop runs no pass (FR-031)");

  skipInsidePasses = 0;
  const std::string skipInside =
      captureRun({"--filter", "^bmSkipInside$", "--iterations=9"});
  check(skipInside.find("SKIPPED: the scripted skip inside the loop")
            != std::string::npos,
        "the skip inside the loop reports its reason (FR-031)");
  check(skipInsidePasses == 3,
        "the function that breaks leaves the loop at once (FR-031)");

  // PR-2 (PR #32): a run without a completed timed loop fails with the
  // reason that names its case, and the measured benchmark that follows
  // it keeps the scripted time it reports on its own, so the failed
  // run left the sampling pairs aligned.
  const std::string noLoop =
      captureRun({"--filter", "^bmNoLoop$", "--iterations=5"}, 1);
  check(noLoop.find("FAILED: the benchmark never entered the timed loop")
            != std::string::npos,
        "the function with no loop names the missing loop (FR-017)");

  breakNoSkipPasses = 0;
  const std::string broken =
      captureRun({"--filter", "^bmBreakNoSkip$", "--iterations=9"}, 1);
  check(
      broken.find("FAILED: the benchmark left the timed loop without a skip")
          != std::string::npos,
      "the function that breaks without a skip names the early exit (FR-017)");
  check(breakNoSkipPasses == 2,
        "the loop ran to the break and no further (FR-017)");

  const std::string thrownInside =
      captureRun({"--filter", "^bmThrowInside$", "--iterations=5"}, 1);
  check(thrownInside.find("FAILED: the scripted throw inside the loop")
            != std::string::npos,
        "the throw inside the loop keeps its own reason (FR-032)");

  const std::string alone =
      captureRun({"--filter", "^bmBeta$", "--iterations=4"});
  const std::string afterFailure =
      captureRun({"--filter",
                  "^bm(NoLoop|BreakNoSkip|ThrowInside|Beta)$",
                  "--iterations=4"},
                 1);
  // A runner with no performance monitor resolves the clock leaves from the
  // host, so two captures of the same scripted benchmark carry different
  // elapsed figures. What the pair has to keep is its shape: the columns it
  // carries and the order they stand in, so the digits are read out before
  // the comparison, and a difference in the raw rows is printed.
  const auto betaAlone = linesOf(alone, "bmBeta");
  const auto betaAfter = linesOf(afterFailure, "bmBeta");
  if (betaAlone != betaAfter) {
    for (const auto& line : betaAlone) {
      std::fprintf(stderr, "alone: %s\n", line.c_str());
    }
    for (const auto& line : betaAfter) {
      std::fprintf(stderr, "after: %s\n", line.c_str());
    }
  }
  const auto shapeOf = [](const std::vector<std::string>& rows)
  {
    std::vector<std::string> shapes;
    shapes.reserve(rows.size());
    for (const auto& row : rows) {
      std::string shape;
      // One run of digits stands for one printed number and one run of
      // spaces for the padding that number's field carries. The overhead
      // floor of a row is measured per run, so a sanitizer capture can
      // print it one digit wider than another, and the field's padding
      // shifts with it. The columns and their order are what the check
      // owns; any per-character mask would also compare widths, and that
      // is what the load of a sanitizer run moves.
      for (const char symbol : row) {
        if (std::isdigit(static_cast<unsigned char>(symbol)) != 0) {
          if (shape.empty() || shape.back() != '#') {
            shape.push_back('#');
          }
          continue;
        }
        if (std::isspace(static_cast<unsigned char>(symbol)) != 0) {
          if (shape.empty() || shape.back() != ' ') {
            shape.push_back(' ');
          }
          continue;
        }
        shape.push_back(symbol);
      }
      shapes.push_back(std::move(shape));
    }
    return shapes;
  };
  check(shapeOf(betaAlone) == shapeOf(betaAfter),
        "a measured benchmark after three failed benchmarks keeps its "
        "scripted row (FR-017)");

  // FR-021, FR-024: the accepted list forms of the contract run, and the
  // printed labels stand ascending with the duplicates collapsed.
  struct ListCase
  {
    const char* argument;
    std::vector<std::string> labels;
  };

  for (const ListCase& testCase :
       {ListCase {"--percentiles=50", {"p50"}},
        ListCase {"--percentiles=50,90,99,99.9",
                  {"p50", "p90", "p99", "p99.9"}},
        ListCase {"--percentiles=50, 90", {"p50", "p90"}},
        ListCase {"--percentiles=50 , 90", {"p50", "p90"}},
        ListCase {"--percentiles=99,50,99", {"p50", "p99"}},
        ListCase {"--percentiles=99,10,50", {"p10", "p50", "p99"}}})
  {
    const std::string report = captureRun(
        {"--filter", "^bmAlpha$", "--iterations=100", testCase.argument});
    check(taggedLinesOf(report, "bmAlpha", "chunks").size() == 1,
          "an accepted list discloses one chunk line (FR-021)");
    const auto printed = taggedLinesOf(report, "bmAlpha", "percentile");
    // FR-021: one line for the primary column and one for the other time
    // column, because a plain benchmark reports both times and no metric.
    check(printed.size() == 2,
          "an accepted list prints one percentile line per reported column "
          "(FR-016, FR-021)");
    if (pointLabelsOf(printed[0]) != testCase.labels) {
      std::fprintf(
          stderr, "%s printed:\n%s\n", testCase.argument, printed[0].c_str());
    }
    check(pointLabelsOf(printed[0]) == testCase.labels
              && pointLabelsOf(printed[1]) == testCase.labels,
          "the labels print ascending with duplicates collapsed on both "
          "columns (FR-024)");
    check(
        linesOf(report, "bmAlpha")[0].find("iterations=") != std::string::npos,
        "the accepted list keeps the run row (FR-023)");
  }

  // FR-020, FR-023, SC-002: with the output off, no chunk line and no
  // percentile line print, and the run row keeps the fields it carries.
  const std::string off = captureRun(
      {"--filter", "^bmAlpha$", "--iterations=100", "--no-percentiles"});
  check(taggedLinesOf(off, "bmAlpha", "chunks").empty(),
        "the run with the output off prints no chunk line (FR-023)");
  check(taggedLinesOf(off, "bmAlpha", "percentile").empty(),
        "the run with the output off prints no percentile line (FR-023)");
  const std::string offRow = linesOf(off, "bmAlpha")[0];
  check(offRow.find("iterations=") != std::string::npos
            && offRow.find("real time/iter") != std::string::npos
            && offRow.find("cpu time/iter") != std::string::npos
            && offRow.find("overhead floor") != std::string::npos
            && offRow.find("endpoint samples=") != std::string::npos,
        "the run row keeps every field it carries today (FR-023, SC-002)");

  // FR-020, FR-023: the family setting stands with neither option, and the
  // run-wide off switch wins over a family that asked.
  const std::string asked =
      captureRun({"--filter", "^bmAsked$", "--iterations=100"});
  const auto askedLines = taggedLinesOf(asked, "bmAsked", "percentile");
  check(askedLines.size() == 2,
        "the family that asked prints one percentile line per reported "
        "column (FR-016, FR-023)");
  check(pointLabelsOf(askedLines[0])
            == std::vector<std::string> {"p50", "p90", "p99", "p99.9"},
        "the family that asked gets the four default points (FR-023)");
  const std::string askedOff = captureRun(
      {"--filter", "^bmAsked$", "--iterations=100", "--no-percentiles"});
  check(taggedLinesOf(askedOff, "bmAsked", "percentile").empty(),
        "the run-wide off switch wins over the family (FR-020)");

  // FR-020, FR-021: the option sets the points of a family that never
  // called the handle method.
  const std::string stated = captureRun(
      {"--filter", "^bmBeta$", "--iterations=100", "--percentiles=25,75"});
  const auto statedLines = taggedLinesOf(stated, "bmBeta", "percentile");
  check(statedLines.size() == 2,
        "the option turns the output on for a silent family (FR-020)");
  check(
      pointLabelsOf(statedLines[0]) == std::vector<std::string> {"p25", "p75"},
      "the option sets the points of a silent family (FR-021)");

  // FR-021: the dry run reports one iteration, keeps the row shape, and
  // answers no percentile field with a value.
  const std::string dry =
      captureRun({"--filter", "^bmAlpha$", "--dry-run", "--percentiles=50,90"});
  check(countFieldOf(linesOf(dry, "bmAlpha")[0], "iterations=") == 1,
        "the dry run reports one iteration (FR-021)");
  check(
      linesOf(dry, "bmAlpha")[0].find("endpoint samples=") != std::string::npos,
      "the dry run row keeps its fields (FR-021)");
  for (const auto& line : taggedLinesOf(dry, "bmAlpha", "percentile")) {
    check(countOf(line, "not available") == pointLabelsOf(line).size(),
          "no percentile field of the dry run carries a value (FR-021)");
  }

  // FR-022, SC-006: every exit-2 form of contracts/percentile-options.md
  // prints one line naming the option, exits 2, and reaches no benchmark
  // function.
  for (const char* argument :
       {"--percentiles=",
        "--percentiles=99,,50",
        "--percentiles=0",
        "--percentiles=100",
        "--percentiles=-5",
        "--percentiles=150",
        "--percentiles=abc",
        "--percentiles=.5",
        "--percentiles=50.",
        "--percentiles=50x",
        "--percentiles=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17"})
  {
    std::string output;
    const int status = subprocessOutput(
        "\"" + self + "\" arg \"" + argument + "\" 2>&1", output);
    if (WEXITSTATUS(status) != 2) {
      std::fprintf(stderr,
                   "%s exited %d with:\n%s\n",
                   argument,
                   WEXITSTATUS(status),
                   output.c_str());
    }
    check(WEXITSTATUS(status) == 2,
          "an invalid percentile list exits 2 (FR-022, SC-006)");
    check(countOf(output, "--percentiles") == 1,
          "the invalid list prints one line naming the option (FR-022)");
    check(output.find("BENCHMARK_RAN") == std::string::npos
              && output.find("iterations=") == std::string::npos,
          "the invalid list runs no benchmark (FR-022, SC-006)");
  }

  // specs/023 T031, T035, SC-004: the report options answer the status the
  // contract names, and the console keeps its bytes ahead of the file.
  {
    std::remove("harness_cli_none.json");
    const std::string plain =
        captureRun({"--filter", "^bmAlpha$", "--iterations=2"});
    check(!linesOf(plain, "bmAlpha").empty(), "the plain run prints its row");
    std::ifstream absent("harness_cli_none.json");
    check(!absent.is_open(), "no option writes no file (FR-002)");
  }
  {
    // FR-004, FR-030: an unwritable path prints the console report first
    // and answers 2.
    const std::string unwritten = captureRun({"--filter",
                                              "^bmAlpha$",
                                              "--iterations=2",
                                              "--json=/proc/nope/report.json"},
                                             2);
    check(!linesOf(unwritten, "bmAlpha").empty(),
          "the console report stands ahead of the failed write (FR-004)");
  }
  {
    // FR-034, E-05: `none` is a spelling the option names, and as the
    // document sees it it is the whole of the raw setting: the file is
    // written and no row carries a chunk column.
    std::remove("harness_cli_rawnone.json");
    captureRun({"--filter",
                "^bmAlpha$",
                "--iterations=2",
                "--json=harness_cli_rawnone.json",
                "--json-raw=none"});
    const auto loaded = sg::loadReport("harness_cli_rawnone.json");
    bool bare = loaded.has_value() && loaded->results.size() == 1
        && loaded->rowRaw.size() == 1;
    for (const auto& rows : loaded->rowRaw) {
      for (const auto& row : rows) {
        bare = bare && row.columns.empty();
      }
    }
    check(bare,
          "--json-raw=none parses and writes a document with no chunk "
          "columns (FR-034, E-05)");
  }
  // FR-004: a failed write beside a failed benchmark still answers 2.
  captureRun({"--filter",
              "^bmThrows$",
              "--iterations=1",
              "--json=/proc/nope/report.json"},
             2);
  for (const std::string argument :
       {"--json=", "--json-raw=", "--json-raw=bogus"})
  {
    std::string output;
    const int status = subprocessOutput(
        "\"" + self + "\" arg \"" + argument + "\" 2>&1", output);
    check(WEXITSTATUS(status) == 2,
          "an invalid report option exits 2 (FR-006, FR-030, SC-004)");
    check(output.find("BENCHMARK_RAN") == std::string::npos
              && output.find("iterations=") == std::string::npos,
          "the invalid report option runs no benchmark (FR-006, SC-004)");
  }
  {
    // FR-001, T035: an interrupted batch still writes the file, and the
    // status stays the one the interrupt earns.
    std::remove("harness_cli_interrupt.json");
    captureRun({"--filter",
                "^bmInterrupts$",
                "--iterations=1",
                "--repetitions=3",
                "--json=harness_cli_interrupt.json"},
               1);
    const auto loaded = sg::loadReport("harness_cli_interrupt.json");
    check(loaded.has_value() && loaded->results.size() == 1
              && loaded->results[0].outcome == sg::RunOutcome::SKIPPED
              && loaded->results[0].reason.find("interrupted")
                     != std::string::npos,
          "the interrupted batch writes the file with the outcome the run "
          "earned (FR-001, FR-004)");
  }

  std::puts("harness_cli_test: ok");
  return 0;
}
