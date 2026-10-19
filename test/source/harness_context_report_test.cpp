// ============================================================================
// TDD test for the threaded report disclosure and the noise warning
// (T022; US4, SC-12, FR-025, FR-026, FR-028, E-07).
//
// Each scenario builds the `{waitMode, placement}` vector of a run and hands
// the result to `sg::detail::printResult`, the seam the console report runs
// through. No benchmark runs, so no provider registers, no thread starts, and
// no clock reads.
//
// The disclosure line follows the block of
// `contracts/report-disclosure.md`:
//
//   contexts=N  ctx0=<MODE>/<PLACEMENT>  ctx1=<MODE>/<PLACEMENT> ...
//
// A slot that named a cpu carries the account flag after its
// placement, as in `ctx0=SPIN/3/applied`. The no-pin slot names no
// cpu, so its field ends with the placement word (FR-025).
//
// The warning line follows the figure of plan.md,
// `warning: spinning contexts oversubscribed (<spin> spin, <cpus> cpus)`,
// counting the spinning contexts of that result and the cpus the host has
// online. The FR-026 cases take their context counts from that host fact
// through `std::thread::hardware_concurrency()`, so the oversubscribed case
// holds one more spinner than the host has cpus on any host and the pinned
// one-per-cpu case names each online cpu once (D-08).
//
// Every printed line of a scenario is compared, so the position each
// requirement names is asserted beside the text: the disclosure after the
// rows of its result, the warning after that disclosure line.
//
// Each scenario runs in a forked child and the parent reads its wait status,
// so one failing scenario reports beside the verdict of every other scenario.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace
{

// The values of the one measured row every scenario prints. The row exists
// to give the disclosure line something to stand after.
constexpr std::uint64_t kIterations = 1000;
constexpr double kRealNs = 250.0;
constexpr double kCpuNs = 200.0;
constexpr std::uint64_t kEndpointSamples = 2;
constexpr std::uint64_t kAggregateSamples = 2;

// The width of the name field the report prints for `%-24s`.
constexpr std::size_t kNameFieldWidth = 24;

// The bytes the shipped report prints after the 24-character name field of
// the row and of the aggregate the scenarios build (FR-025: those bytes do
// not move for a threaded result, and they do not move at all for a
// single-context result).
constexpr auto kRowTail =
    " iterations=      1000  real time/iter (ns)=  250.000  "
    "cpu time/iter (ns)=  200.000  overhead floor (ns)=    0.000  "
    "endpoint samples=2";
constexpr auto kAggregateTail =
    "  AGGREGATE  primary time/iter (ns) mean=  250.000 median=  250.000 "
    "sd=    0.000 cv=  0.0000 min=  250.000 max=  250.000 n=2";

// The scratch file the captured report lands in, named for this test so a
// suite run in parallel holds no file in common with it.
constexpr auto kScratchFile = "harness_context_report_out.txt";

// The widest cpu index text the default placement function can
// render: the decimal of the largest value the header's
// `PlacementSlot::cpuIndex` holds, the value the `std::to_chars` call
// of that function is handed (FR-005). The field must print it whole
// (FR-025).
constexpr auto kWidestCpuIndex = "4294967295";

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CONTEXT REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// FR-026, D-08: the one host fact the warning reads, the count of cpus this
// process may run on: the affinity mask, which a cpuset or a taskset narrows
// below the count of cpus the host has online.
auto onlineCpuCount() -> std::uint64_t
{
  cpu_set_t mask {};
  const int read = sched_getaffinity(0, sizeof(mask), &mask);
  check(read == 0,
        "the process reads the affinity mask it runs under (FR-026)");
  const auto count = static_cast<std::uint64_t>(CPU_COUNT(&mask));
  check(count > 0,
        "the affinity mask names a cpu the process may run on (FR-026)");
  return count;
}

// One entry of the disclosure vector a scenario builds (E-07).
struct Policy
{
  sg::WaitMode waitMode;
  std::string placement;
  bool applied = true;
};

auto spinCount(const std::vector<Policy>& policies) -> std::uint64_t
{
  std::uint64_t spinning = 0;
  for (const auto& policy : policies) {
    spinning += policy.waitMode == sg::WaitMode::SPIN ? 1 : 0;
  }
  return spinning;
}

auto modeName(const sg::WaitMode mode) -> const char*
{
  switch (mode) {
    case sg::WaitMode::SPIN:
      return "SPIN";
    case sg::WaitMode::YIELD:
      return "YIELD";
    case sg::WaitMode::BLOCK:
      return "BLOCK";
  }
  return "UNKNOWN";
}

// The field the report prints for one name: `%-24s`.
auto nameField(const std::string& name) -> std::string
{
  check(name.size() <= kNameFieldWidth,
        "the scenario names the result within the field");
  auto field = name;
  field.append(kNameFieldWidth - name.size(), ' ');
  return field;
}

auto repetitionLine(const std::string& name) -> std::string
{
  return nameField(name) + kRowTail;
}

auto aggregateLine(const std::string& name) -> std::string
{
  return nameField(name) + kAggregateTail;
}

// The disclosure line of `contracts/report-disclosure.md`, and the
// warning line of the plan's report figure, as the report owes them.
// The account flag stands after the placement of a slot that named a
// cpu, the slot whose pin can land.
auto disclosureLine(const std::vector<Policy>& policies) -> std::string
{
  auto line = std::string("  contexts=") + std::to_string(policies.size());
  for (std::size_t index = 0; index < policies.size(); ++index) {
    line += "  ctx" + std::to_string(index) + "="
        + modeName(policies[index].waitMode) + "/"
        + sg::detail::placementField(policies[index].placement);
    if (policies[index].placement != "NO_PIN") {
      line += policies[index].applied ? "/applied" : "/unapplied";
    }
  }
  return line;
}

auto warningLine(const std::vector<Policy>& policies) -> std::string
{
  return "warning: spinning contexts oversubscribed ("
      + std::to_string(spinCount(policies)) + " spin, "
      + std::to_string(onlineCpuCount()) + " cpus)";
}

auto makeResult(const std::string& name,
                const std::vector<Policy>& policies,
                const bool withAggregate) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;

  auto row = sg::ResultRow {};
  row.iterations = kIterations;
  row.timePerIterationNs = kRealNs;
  row.cpuTimePerIterationNs = kCpuNs;
  row.endpointSamples = kEndpointSamples;
  result.rows.push_back(row);

  if (withAggregate) {
    auto aggregate = sg::ResultRow {};
    aggregate.kind = sg::RowKind::AGGREGATE;
    aggregate.timeStats = sg::Statistics {.samples = kAggregateSamples,
                                          .mean = kRealNs,
                                          .median = kRealNs,
                                          .sampleStdDev = 0.0,
                                          .coefficientOfVariation = 0.0,
                                          .min = kRealNs,
                                          .max = kRealNs};
    result.rows.push_back(aggregate);
  }

  for (const auto& policy : policies) {
    result.contexts.push_back(
        sg::ContextDisclosure {.waitMode = policy.waitMode,
                               .placement = policy.placement,
                               .applied = policy.applied});
  }
  return result;
}

// The four context sets of the FR-026 cases, in the order the contract names
// them.
auto sharedCpuSpinPair() -> std::vector<Policy>
{
  return {{sg::WaitMode::SPIN, "0"}, {sg::WaitMode::SPIN, "0"}};
}

auto oversubscribedSpinners() -> std::vector<Policy>
{
  std::vector<Policy> policies;
  for (std::uint64_t index = 0; index <= onlineCpuCount(); ++index) {
    policies.push_back({sg::WaitMode::SPIN, "NO_PIN"});
  }
  return policies;
}

auto pinnedOnePerCpu() -> std::vector<Policy>
{
  std::vector<Policy> policies;
  for (std::uint64_t index = 0; index < onlineCpuCount(); ++index) {
    policies.push_back({sg::WaitMode::SPIN, std::to_string(index)});
  }
  return policies;
}

auto blockingPair() -> std::vector<Policy>
{
  return {{sg::WaitMode::BLOCK, "0"}, {sg::WaitMode::BLOCK, "0"}};
}

auto blockingOversubscribed() -> std::vector<Policy>
{
  std::vector<Policy> policies;
  for (std::uint64_t index = 0; index <= onlineCpuCount(); ++index) {
    policies.push_back({sg::WaitMode::BLOCK, "NO_PIN"});
  }
  return policies;
}

// The printed report of `results`, captured to the end of the print.
auto printCaptured(const std::vector<sg::BenchmarkResult>& results)
    -> std::vector<std::string>
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kScratchFile, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  for (const auto& result : results) {
    sg::detail::printResult(result);
  }
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

  std::ifstream file(kScratchFile);
  std::stringstream buffer;
  buffer << file.rdbuf();
  auto lines = std::vector<std::string> {};
  auto line = std::string();
  while (std::getline(buffer, line)) {
    lines.push_back(line);
  }
  return lines;
}

auto checkLines(const std::vector<std::string>& actual,
                const std::vector<std::string>& expected,
                const char* what) -> void
{
  if (actual == expected) {
    return;
  }
  std::fprintf(stderr,
               "the report printed %zu lines, the contract owes %zu\n",
               actual.size(),
               expected.size());
  const std::size_t span =
      actual.size() > expected.size() ? actual.size() : expected.size();
  for (std::size_t index = 0; index < span; ++index) {
    const char* left =
        index < expected.size() ? expected[index].c_str() : "<absent>";
    const char* right =
        index < actual.size() ? actual[index].c_str() : "<absent>";
    std::fprintf(stderr,
                 "  line %2zu: contract %s\n            report %s\n",
                 index + 1,
                 left,
                 right);
  }
  fail(what);
}

// FR-025, SC-12: a threaded result prints the context count and the mode and
// placement of each context after its rows, and its rows keep their bytes.
// Two spinning contexts with no pin stand under no warning condition.
auto scenarioDisclosure() -> void
{
  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, "NO_PIN"},
      {sg::WaitMode::SPIN, "NO_PIN"},
  };
  const auto lines = printCaptured({makeResult("bmPair", policies, true)});
  checkLines(lines,
             {repetitionLine("bmPair"),
              aggregateLine("bmPair"),
              disclosureLine(policies)},
             "a threaded result prints the count and each policy after its "
             "rows, with no warning (FR-025, SC-12)");
}

// FR-025, Assumptions: the single-context result keeps the bytes it prints
// today, so its report holds the two rows and no disclosure line.
auto scenarioSingleContext() -> void
{
  const auto policies = std::vector<Policy> {};
  const auto lines = printCaptured({makeResult("bmSingle", policies, true)});
  checkLines(lines,
             {repetitionLine("bmSingle"), aggregateLine("bmSingle")},
             "a single-context result keeps its bytes and prints no "
             "disclosure line (FR-025)");
}

// FR-025: the line names the mode and the placement of every context, so the
// three wait modes and both placement shapes are readable in it. One
// spinner, no shared index and no excess over the online count stand under
// no warning condition (FR-028).
auto scenarioModesAndPlacements() -> void
{
  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, "0"},
      {sg::WaitMode::YIELD, "1"},
      {sg::WaitMode::BLOCK, "NO_PIN"},
  };
  const auto lines = printCaptured({makeResult("bmModes", policies, false)});
  checkLines(lines,
             {repetitionLine("bmModes"), disclosureLine(policies)},
             "the disclosure line names each context's wait mode and "
             "placement (FR-025)");
}

// FR-026, FR-028, SC-12: the first of the four cases, two spinning contexts
// of one result naming one cpu index. The warning stands after the
// disclosure line of that result.
auto scenarioSharedCpuSpinPair() -> void
{
  const auto policies = sharedCpuSpinPair();
  const auto lines = printCaptured({makeResult("bmShared", policies, false)});
  checkLines(lines,
             {repetitionLine("bmShared"),
              disclosureLine(policies),
              warningLine(policies)},
             "two spinning contexts pinned to one cpu print the noise "
             "warning after the result (FR-026, FR-028)");
}

// FR-026, FR-028, SC-12: the second of the four cases, a result with more
// spinning contexts than the host has online cpus. The count is the online
// count plus one, so the condition holds on every host.
auto scenarioOversubscribed() -> void
{
  const auto policies = oversubscribedSpinners();
  check(spinCount(policies) > onlineCpuCount(),
        "the scripted spinner count stands above the online cpu count");
  const auto lines = printCaptured({makeResult("bmOver", policies, false)});
  checkLines(lines,
             {repetitionLine("bmOver"),
              disclosureLine(policies),
              warningLine(policies)},
             "a spinning count above the online cpu count prints the noise "
             "warning after the result (FR-026, FR-028)");
}

// FR-026, FR-028: the field of a pin that never landed names no cpu, so two
// spinning contexts carrying it share no cpu and stand under the count of
// cpus: neither condition of the noise rule holds, and no warning prints.
auto scenarioUnappliedPair() -> void
{
  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, "UNAPPLIED"},
      {sg::WaitMode::SPIN, "UNAPPLIED"},
  };
  const auto lines =
      printCaptured({makeResult("bmUnapplied", policies, false)});
  checkLines(lines,
             {repetitionLine("bmUnapplied"), disclosureLine(policies)},
             "two spinning contexts whose pin never landed print no noise "
             "warning (FR-026, FR-028)");
}

// FR-026, FR-028, D-08: the count the warning compares the spinning contexts
// against is the affinity mask of the process. The scenario narrows its own
// mask to one cpu, the shape a cpuset or a taskset leaves, and two spinning
// contexts stand above that count while the host answers more cpus.
auto scenarioNarrowedAffinity() -> void
{
  cpu_set_t wider {};
  const int read = sched_getaffinity(0, sizeof(wider), &wider);
  check(read == 0,
        "the scenario reads the affinity mask it starts under (FR-026)");

  cpu_set_t single {};
  CPU_ZERO(&single);
  CPU_SET(0, &single);
  const int narrowed = sched_setaffinity(0, sizeof(single), &single);
  check(narrowed == 0,
        "the scenario narrows its own affinity to one cpu (FR-026)");

  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, "NO_PIN"},
      {sg::WaitMode::SPIN, "NO_PIN"},
  };
  // The expected lines stand while the mask holds one cpu, so the count the
  // warning prints is the narrowed one.
  const auto expected = std::vector<std::string> {
      repetitionLine("bmAffinity"),
      disclosureLine(policies),
      warningLine(policies),
  };
  const auto lines = printCaptured({makeResult("bmAffinity", policies, false)});

  const int restored = sched_setaffinity(0, sizeof(wider), &wider);
  check(restored == 0, "the scenario puts the affinity mask back (FR-026)");

  checkLines(lines,
             expected,
             "two spinning contexts above the cpus this process may run on "
             "print the noise warning (FR-026, FR-028)");
}

// FR-026, FR-028, SC-12: the third of the four cases, one pinned spinning
// context per online cpu. Neither condition holds, so no warning prints.
auto scenarioPinnedOnePerCpu() -> void
{
  const auto policies = pinnedOnePerCpu();
  const auto lines = printCaptured({makeResult("bmPinned", policies, false)});
  checkLines(lines,
             {repetitionLine("bmPinned"), disclosureLine(policies)},
             "a pinned setup with one context per cpu prints no warning "
             "(FR-026, FR-028)");
}

// FR-026, FR-028, SC-12: the fourth of the four cases, a blocking setup. The
// shared index and the excess over the online count name spinning contexts,
// so a blocking setup prints its disclosure and no warning.
auto scenarioBlocking() -> void
{
  const auto pair = blockingPair();
  const auto excess = blockingOversubscribed();
  const auto lines = printCaptured({makeResult("bmBlock", pair, false),
                                    makeResult("bmBlockMany", excess, false)});
  checkLines(lines,
             {repetitionLine("bmBlock"),
              disclosureLine(pair),
              repetitionLine("bmBlockMany"),
              disclosureLine(excess)},
             "a blocking setup prints no warning under either condition "
             "(FR-026, FR-028)");
}

// FR-026: one warning line for each result whose condition holds, printed
// after that result. The middle result holds no condition, so it prints its
// disclosure and the run holds two warnings in all.
auto scenarioOneWarningPerResult() -> void
{
  const auto shared = sharedCpuSpinPair();
  const auto blockers = blockingPair();
  const auto excess = oversubscribedSpinners();
  const auto lines = printCaptured({makeResult("bmFirst", shared, false),
                                    makeResult("bmMiddle", blockers, false),
                                    makeResult("bmLast", excess, false)});
  checkLines(lines,
             {repetitionLine("bmFirst"),
              disclosureLine(shared),
              warningLine(shared),
              repetitionLine("bmMiddle"),
              disclosureLine(blockers),
              repetitionLine("bmLast"),
              disclosureLine(excess),
              warningLine(excess)},
             "each result whose condition holds prints one warning line "
             "after it, and no other result prints one (FR-026)");
}

// One scenario of the suite with the name the verdict line prints.
struct Scenario
{
  const char* name;
  void (*body)();
};

// Run one scenario in a child of this process and read its wait status, so a
// scenario that fails leaves the verdict of every other scenario visible.
auto runIsolated(const Scenario& scenario) -> bool
{
  std::fflush(stdout);
  const pid_t child = fork();
  if (child < 0) {
    fail("cannot start the scenario process");
  }
  if (child == 0) {
    scenario.body();
    std::fflush(stdout);
    std::exit(0);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    fail("cannot read the status of the scenario process");
  }
  const bool passed = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  std::printf("harness_context_report_test %s: %s\n",
              scenario.name,
              passed ? "pass" : "FAIL");
  std::fflush(stdout);
  return passed;
}

// FR-005, FR-025: the flag of the placement account stands beside the field,
// so a replacement placement function that returns its own text for a slot it
// did not land still shows the failure in the report.
auto scenarioPlacementFlagShows() -> void
{
  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, "DECLINED", false},
      {sg::WaitMode::BLOCK, "7", true},
  };
  const auto lines =
      printCaptured({makeResult("bmPlacementFlag", policies, false)});
  checkLines(lines,
             {repetitionLine("bmPlacementFlag"),
              "  contexts=2  ctx0=SPIN/DECLINED/unapplied"
              "  ctx1=BLOCK/7/applied"},
             "the disclosure names the flag of each placement account, so a "
             "pin that did not land shows under the text of its own function "
             "(FR-005, FR-025)");
}

// FR-025: a slot that named no cpu asked for nothing to land, so its
// field ends with the placement word. The account flag of its record
// stands in no position of the line, whatever that record says.
auto scenarioNoPinCarriesNoFlag() -> void
{
  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, "NO_PIN", true},
      {sg::WaitMode::BLOCK, "NO_PIN", false},
  };
  const auto lines =
      printCaptured({makeResult("bmNoPinFlag", policies, false)});
  checkLines(lines,
             {repetitionLine("bmNoPinFlag"),
              "  contexts=2  ctx0=SPIN/NO_PIN  ctx1=BLOCK/NO_PIN"},
             "a context with no pin prints its wait mode and placement with "
             "no account flag after them (FR-025)");
}

// FR-025: the field is wide enough for every text the default placement
// function produces, so the widest cpu index that function can render
// reads whole in the printed line.
auto scenarioWidestPlacementWhole() -> void
{
  check(std::to_string(std::numeric_limits<std::uint32_t>::max())
            == kWidestCpuIndex,
        "the text above is the widest cpu index the slot type of the header "
        "holds (FR-005)");
  const auto policies = std::vector<Policy> {
      {sg::WaitMode::SPIN, kWidestCpuIndex, true},
      {sg::WaitMode::BLOCK, "NO_PIN", false},
  };
  const auto lines =
      printCaptured({makeResult("bmWideIndex", policies, false)});
  checkLines(lines,
             {repetitionLine("bmWideIndex"),
              "  contexts=2  ctx0=SPIN/4294967295/applied"
              "  ctx1=BLOCK/NO_PIN"},
             "the disclosure prints the widest placement text the default "
             "placement function produces with no cut (FR-025)");
}

// FR-025: the placement field holds whole every text the default
// placement function produces, and cuts a longer one, marking the cut
// in the last position of the field.
auto scenarioPlacementField() -> void
{
  const auto widest = sg::detail::placementField(kWidestCpuIndex);
  check(widest == kWidestCpuIndex,
        "the widest text the default placement function produces stands "
        "whole in the field (FR-025)");
  check(sg::detail::placementField("NO_PIN") == "NO_PIN",
        "the no-pin word stands whole in the field (FR-025)");
  check(sg::detail::placementField("UNAPPLIED") == "UNAPPLIED",
        "the unapplied word stands whole in the field (FR-025)");
  const auto cut = sg::detail::placementField("topology-node-3");
  check(cut == "topology-~",
        "a text longer than the field stands cut, with the mark of the cut "
        "in the last position of the field (FR-025)");
  check(cut.size() == widest.size(),
        "a cut field fills the width of the field (FR-025)");
}

// FR-026, FR-035: the two counts the report reads answer twice each, and
// both answers drive here at their own call.
auto scenarioSelectorArms() -> void
{
  check(sg::detail::selectRunnableCpuCount(true, 4, 9) == 4,
        "an affinity mask that answered gives its own cpu count (FR-026)");
  check(sg::detail::selectRunnableCpuCount(false, 4, 9) == 9,
        "an affinity mask that stayed unread leaves the host count (FR-026)");
  const auto landed = sg::detail::selectPlacementAccount(true, "CPU-3");
  check(landed.applied && landed.description == "CPU-3",
        "a placement that landed names the slot it landed on (FR-035)");
  const auto missed = sg::detail::selectPlacementAccount(false, "CPU-3");
  check(!missed.applied && missed.description == "UNAPPLIED",
        "a placement that missed says so in its own words (FR-035)");
}

}  // namespace

auto main() -> int
{
  const std::vector<Scenario> scenarios = {
      {"disclosure", scenarioDisclosure},
      {"singleContext", scenarioSingleContext},
      {"modesAndPlacements", scenarioModesAndPlacements},
      {"sharedCpuSpinPair", scenarioSharedCpuSpinPair},
      {"oversubscribed", scenarioOversubscribed},
      {"unappliedPair", scenarioUnappliedPair},
      {"narrowedAffinity", scenarioNarrowedAffinity},
      {"pinnedOnePerCpu", scenarioPinnedOnePerCpu},
      {"blocking", scenarioBlocking},
      {"oneWarningPerResult", scenarioOneWarningPerResult},
      {"placementFlagShows", scenarioPlacementFlagShows},
      {"noPinCarriesNoFlag", scenarioNoPinCarriesNoFlag},
      {"widestPlacementWhole", scenarioWidestPlacementWhole},
      {"placementField", scenarioPlacementField},
      {"selectorArms", scenarioSelectorArms},
  };
  std::size_t failed = 0;
  for (const auto& scenario : scenarios) {
    if (!runIsolated(scenario)) {
      ++failed;
    }
  }
  std::printf("harness_context_report_test: %zu scenarios, %zu fail\n",
              scenarios.size(),
              failed);
  return failed == 0 ? 0 : 1;
}
