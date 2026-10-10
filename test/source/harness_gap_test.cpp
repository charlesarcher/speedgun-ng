// ============================================================================
// T071 (PR-4b): the harness paths that the feature's own tests leave
// unmeasured, gathered into one file so each carries its own provider
// script. Every mode names the rule it measures: the metric leaves the
// run resolves and refuses (FR-023), the availability names the report
// prints (FR-024), the run that ends by an exception outside
// `std::exception` or by a second timed loop (FR-017), the cursor that
// names an iteration and the count it reads (FR-005), and the single
// repetition whose spread is zero by definition (FR-034).
//
// Each mode owns one provider script, so it runs in a re-exec of this
// binary, the way the calibration test's scripted scenarios do.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS GAP TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmPlain(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// FR-005: the cursor names the iteration it is on, and the run control
// reads the count of the current run.
auto bmIndex(sg::State& state) -> void
{
  std::uint64_t sum = 0;
  for (auto index : state) {
    sum += index + state.iterations();
  }
  sg::doNotOptimize(sum);
  sg::clobberMemory();
}

// FR-032: a throw outside `std::exception` still ends the run with a
// reason, and FR-017 keeps its pair whole.
auto bmUnknownThrow(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  // The run has to end through the handler for what lies outside
  // std::exception (FR-032).
  // NOLINTNEXTLINE(bugprone-std-exception-baseclass)
  throw 7;
}

// FR-017: a function with two timed loops has no single measured window.
auto bmTwoLoops(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  for (auto _ : state) {
  }
}

auto bmAlwaysFail(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  throw std::runtime_error("always");
}

auto bmAlwaysSkip(sg::State& state) -> void
{
  state.skipWithMessage("the scripted calibration skip");
}

auto bmSignal(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  raise(SIGINT);
}

auto baseProvider(const std::string& threadCpuUnit) -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  if (!threadCpuUnit.empty()) {
    provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    provider.setPoints("machine", "thread_cpu", {0}, 2000);
  }
  provider.addObject("gap", "gap", "gap test object");
  provider.addCounter("gap", "seconds", "nanoseconds", "a time leaf");
  provider.setPoints("gap", "seconds", {0}, 500);
  provider.addCounter("gap", "counts", "none", "an event leaf");
  provider.setPoints("gap", "counts", {0}, 7);
  provider.addCounter("gap",
                      "blocked",
                      "none",
                      "a leaf the host refuses",
                      sg::counters::Availability::PERMISSION_BLOCKED);
  provider.addCounter("gap",
                      "unencodable",
                      "none",
                      "a leaf the encoding refuses",
                      sg::counters::Availability::NOT_ENCODABLE);
  provider.addCounter("gap",
                      "refused",
                      "none",
                      "a leaf the scope refuses",
                      sg::counters::Availability::SCOPE_REFUSED);
  provider.addCounter("gap",
                      "gapped",
                      "none",
                      "a leaf the catalog marks gapped",
                      sg::counters::Availability::GAP);
  provider.addCounter("gap",
                      "pushed",
                      "none",
                      "a pushed leaf",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::PUSH_LOAD);
  provider.setPoints("gap", "pushed", {0}, 3);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the gap provider registers");
}

auto captureRun(const std::vector<std::string>& arguments,
                const int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_gap_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_gap_out.txt";
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

auto lineWith(const std::string& report, const std::string& name) -> std::string
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

auto aggregateOf(const std::string& report,
                 const std::string& name) -> std::string
{
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0 && line.contains("AGGREGATE")) {
      return line;
    }
  }
  return {};
}

auto countOf(const std::string& line, const std::string& key) -> long long
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtoll(line.c_str() + at + key.size(), nullptr, 10);
}

auto attachMetric(const char* name,
                  const char* leaf,
                  const char* label,
                  const bool timeLeaf) -> void
{
  auto handle = sg::registerBenchmark(&bmPlain, name);
  const auto object = sg::counters::System::local().object("gap");
  if (!object.has_value()) {
    fail("the gap object does not resolve");
  }
  if (timeLeaf) {
    const auto resolved = object->counter<sg::counters::Dim<1, 0>>(leaf);
    if (!resolved.has_value()) {
      fail("the metric leaf does not resolve");
    }
    handle.addMetric(
        sg::counters::Expression<sg::counters::Dim<1, 0>>(*resolved), label);
    return;
  }
  const auto resolved = object->counter<sg::counters::Dim<0, 1>>(leaf);
  if (!resolved.has_value()) {
    fail("the metric leaf does not resolve");
  }
  handle.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*resolved),
                   label);
}

auto modeNoThreadCpu() -> int
{
  baseProvider("");
  const std::string report =
      captureRun({"--filter", "^bmPlain$", "--iterations=2"}, 1);
  check(report.contains("machine/thread_cpu does not resolve"),
        "a run without the decision leaf fails with its own reason (FR-038)");
  return 0;
}

auto modeMetricLeaves() -> int
{
  baseProvider("nanoseconds");
  attachMetric("bmTimeMetric", "seconds", "seconds", true);
  attachMetric("bmBlockedMetric", "blocked", "blocked", false);
  attachMetric("bmUnencMetric", "unencodable", "unencodable", false);
  attachMetric("bmRefusedMetric", "refused", "refused", false);
  attachMetric("bmGappedMetric", "gapped", "gapped", false);
  attachMetric("bmPushMetric", "pushed", "pushed", false);
  const std::string report =
      captureRun({"--filter",
                  "^bm(Time|Blocked|Unenc|Refused|Gapped|Push)Metric$",
                  "--iterations=4"},
                 1);
  check(lineWith(report, "bmTimeMetric").contains("seconds="),
        "a time leaf reaches the report as its own column (FR-021)");
  check(lineWith(report, "bmBlockedMetric")
            .contains("FAILED: the run plan does not compile"),
        "a leaf the host refuses stops the plan with its own reason (FR-024)");
  check(lineWith(report, "bmUnencMetric").contains("not_encodable"),
        "an unencodable leaf names its availability in the reason (FR-024)");
  check(lineWith(report, "bmRefusedMetric").contains("scope_refused"),
        "a scope-refused leaf names its availability in the reason (FR-024)");
  check(lineWith(report, "bmGappedMetric").contains("the catalog reports gap"),
        "a gapped leaf names its availability in the reason (FR-024)");
  check(lineWith(report, "bmPushMetric").contains("pushed="),
        "a pushed leaf reaches the report (FR-021)");
  return 0;
}

auto modeRunEnds() -> int
{
  baseProvider("nanoseconds");
  const std::string thrown =
      captureRun({"--filter", "^bmUnknownThrow$", "--iterations=3"}, 1);
  check(thrown.contains("FAILED: the benchmark threw an unknown exception"),
        "a throw outside std::exception keeps a reason (FR-032)");
  const std::string twice =
      captureRun({"--filter", "^bmTwoLoops$", "--iterations=3"}, 1);
  check(twice.contains("the benchmark entered the timed loop twice"),
        "a second timed loop fails with the reason that names it (FR-017)");
  return 0;
}

auto modeIndexAndSpread() -> int
{
  baseProvider("nanoseconds");
  const std::string report = captureRun(
      {"--filter", "^bmIndex$", "--iterations=6", "--repetitions=2"}, 0);
  const std::string row = lineWith(report, "bmIndex");
  check(countOf(row, "iterations=") == 6,
        "the cursor and the count reached the run (FR-005)");
  check(report.contains("sd=    0.000"),
        "two identical repetitions carry a spread of zero (FR-034)");
  return 0;
}

// FR-034: the coefficient of variation of a run set whose mean is zero
// is zero by definition.
auto modeZeroMean() -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.setPoints("machine", "monotonic", {0}, 0);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 0);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the still provider registers");

  const std::string report = captureRun(
      {"--filter", "^bmPlain$", "--iterations=4", "--repetitions=2"}, 0);
  const std::string aggregate = aggregateOf(report, "bmPlain");
  check(!aggregate.empty(), "a still run set prints its aggregate (FR-034)");
  check(aggregate.contains("mean=    0.000") && aggregate.contains("cv="),
        "a zero mean keeps a defined coefficient of variation (FR-034)");
  return 0;
}

// FR-037: the catalog listing names the read mode of every leaf, the
// pushed form among them.
auto modeCatalog() -> int
{
  baseProvider("nanoseconds");
  const std::string report = captureRun({"--catalog"}, 0);
  check(report.contains("push-load"),
        "the listing names the pushed read mode (FR-037)");
  return 0;
}

// FR-035: a count that overflows the signed read, and a count with
// trailing text, are usage errors that report and stop. The entry point
// exits from inside the parse, so each form runs as its own mode and the
// parent reads status two from it.
auto modeHugeCount() -> int
{
  baseProvider("nanoseconds");
  captureRun({"--iterations=99999999999999999999"}, 2);
  return 0;
}

auto modeJunkCount() -> int
{
  baseProvider("nanoseconds");
  captureRun({"--iterations=5x"}, 2);
  return 0;
}

// FR-042: a host whose machine object carries no monotonic leaf leaves
// the entry point with no time source, and it reports that and stops.
auto modeNoClock() -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 100);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the half provider registers");
  const std::string report = captureRun({"--filter", "^bmPlain$"}, 1);
  check(report.empty(),
        "the entry point printed no report without a time source (FR-042)");
  return 0;
}

// FR-032, FR-016: the calibration run itself can fail, skip, or stop on
// a signal, and each ending carries its own report line.
auto modeCalibrationEnds() -> int
{
  baseProvider("nanoseconds");
  const std::string thrown = captureRun({"--filter", "^bmAlwaysFail$"}, 1);
  check(thrown.contains("FAILED: always"),
        "a failing calibration run keeps its reason (FR-032)");
  const std::string skipped = captureRun({"--filter", "^bmAlwaysSkip$"}, 0);
  check(skipped.contains("SKIPPED:"),
        "a skipping calibration run reports the skip (FR-016)");
  const std::string signaled = captureRun({"--filter", "^bmSignal$"}, 1);
  check(signaled.contains("interrupted by the user (SIGINT)"),
        "a signal during calibration stops the growth (FR-016)");
  return 0;
}

// FR-016: the warm-up run can fail, skip, or stop on a signal before
// calibration starts, and each ending carries its own report line.
auto modeWarmupEnds() -> int
{
  baseProvider("nanoseconds");
  captureRun({"--warmup-time=0.05", "--filter", "^bmAlwaysFail$"}, 1);
  captureRun({"--warmup-time=0.05", "--filter", "^bmAlwaysSkip$"}, 0);
  const std::string signaled =
      captureRun({"--warmup-time=0.05", "--filter", "^bmSignal$"}, 1);
  check(signaled.contains("interrupted by the user (SIGINT)"),
        "a signal during the warm-up stops the run (FR-016)");
  return 0;
}

// FR-015: the per-benchmark option setters on the handle carry the
// warm-up and the repetition count into the run without the command
// line.
auto modeHandleOptions() -> int
{
  baseProvider("nanoseconds");
  auto handle = sg::registerBenchmark(&bmPlain, "bmOptioned");
  handle.warmupTime(1'000).repetitions(2).iterations(4);
  const std::string report = captureRun({"--filter", "^bmOptioned$"}, 0);
  check(aggregateOf(report, "bmOptioned").contains("n=2"),
        "the handle's repetition count reaches the aggregate (FR-015)");
  return 0;
}

// FR-024: the report's name of an availability state, including the
// value outside the closed enumeration that the switch's tail answers.
auto modeNamesAndListing() -> int
{
  using sg::counters::Availability;
  check(std::string_view(sg::detail::availabilityName(Availability::GAP))
            == "gap",
        "the report names the gap availability (FR-024)");
  check(std::string_view(
            sg::detail::availabilityName(static_cast<Availability>(200)))
            == "unknown",
        "a value outside the closed enumeration names unknown (FR-024)");
  return 0;
}

}  // namespace

auto modeCounterAddress() -> int
{
  baseProvider("nanoseconds");
  const auto report = captureRun({"--filter",
                                  "^bmPlain$",
                                  "--repetitions",
                                  "1",
                                  "--iterations",
                                  "4",
                                  "--counter",
                                  "gap/seconds",
                                  "--counter",
                                  "gap/counts"});
  const auto row = lineWith(report, "bmPlain");
  check(row.contains("seconds="),
        "a time address resolves through the catalog (R-04)");
  check(row.contains("counts="),
        "an event address resolves through the catalog (R-04)");

  const auto refused = captureRun({"--filter",
                                   "^bmPlain$",
                                   "--repetitions",
                                   "1",
                                   "--iterations",
                                   "4",
                                   "--counter",
                                   "gap/blocked"},
                                  1);
  check(refused.contains("gap/blocked"),
        "an address the host refuses stops the run (FR-023)");

  const auto bare = captureRun({"--filter",
                                "^bmPlain$",
                                "--repetitions",
                                "1",
                                "--iterations",
                                "4",
                                "--counter",
                                "seconds"});
  check(lineWith(bare, "bmPlain").contains("iterations="),
        "an address with no object leaves the run running (R-04)");
  return 0;
}

SG_BENCHMARK(bmPlain);
SG_BENCHMARK(bmIndex);
SG_BENCHMARK(bmUnknownThrow);
SG_BENCHMARK(bmTwoLoops);
SG_BENCHMARK(bmAlwaysFail);
SG_BENCHMARK(bmAlwaysSkip);
SG_BENCHMARK(bmSignal);

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "noThreadCpu") {
    return modeNoThreadCpu();
  }
  if (mode == "metricLeaves") {
    return modeMetricLeaves();
  }
  if (mode == "runEnds") {
    return modeRunEnds();
  }
  if (mode == "indexSpread") {
    return modeIndexAndSpread();
  }
  if (mode == "zeroMean") {
    return modeZeroMean();
  }
  if (mode == "catalog") {
    return modeCatalog();
  }
  if (mode == "hugeCount") {
    return modeHugeCount();
  }
  if (mode == "junkCount") {
    return modeJunkCount();
  }
  if (mode == "noClock") {
    return modeNoClock();
  }
  if (mode == "calibrationEnds") {
    return modeCalibrationEnds();
  }
  if (mode == "warmupEnds") {
    return modeWarmupEnds();
  }
  if (mode == "handleOptions") {
    return modeHandleOptions();
  }
  if (mode == "namesListing") {
    return modeNamesAndListing();
  }
  if (mode == "counterAddress") {
    return modeCounterAddress();
  }

  const std::string self = argv[0];
  for (const auto& name : {
           "noThreadCpu",
           "metricLeaves",
           "runEnds",
           "indexSpread",
           "zeroMean",
           "catalog",
           "hugeCount",
           "junkCount",
           "noClock",
           "calibrationEnds",
           "warmupEnds",
           "handleOptions",
           "namesListing",
           "counterAddress",
       })
  {
    const int status = std::system(("\"" + self + "\" " + name).c_str());
    // A usage error exits from inside the entry point, so the child of
    // the two count modes ends with status two by design (FR-035).
    const std::string_view modeName {name};
    const int wanted =
        (modeName == "hugeCount" || modeName == "junkCount") ? 2 : 0;
    // A signaled child carries no exit code: WEXITSTATUS of that
    // status reads zero, so the exit test stands only for a child
    // that ran to completion (FR-035).
    check(WIFEXITED(status) && WEXITSTATUS(status) == wanted,
          "the gap mode passes (FR-023, FR-024, FR-032)");
  }
  std::puts("harness_gap_test: ok");
  return 0;
}
