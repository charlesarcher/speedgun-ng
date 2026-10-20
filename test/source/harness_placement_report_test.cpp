// ============================================================================
// specs/022-topology-placement T015: the topology line and the placement
// field of the console report (US2, FR-020 to FR-022, R-06, R-09, SC-05).
//
// Each scenario builds the `{waitMode, placement, applied}` records of a
// run and hands the result to `sg::detail::printResult`, the seam the
// console report runs through, beside `sg::detail::printContext` for the
// block above the rows. No benchmark runs, so no provider registers and
// no clock reads (FR-024).
//
// The block follows `contracts/report-topology.md`:
//
//   speedgun-ng <version> build=<type>
//   topology: packages=1 l3-groups=4 cores=24 threads=48
//   <rows>
//     contexts=4  ctx0=SPIN/0@0/applied ...
//
// The topology line stands only when a preset stands (FR-019, FR-022),
// and the no-preset block keeps the bytes feature 020 ships (SC-05).
//
// Each scenario runs in a forked child and the parent reads its wait
// status, so one failing scenario reports beside every other verdict.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace
{

// The values of the one measured row every scenario prints, and the
// bytes the shipped report prints after the 24-character name field,
// copied from the standing report test so the byte-for-byte scenario
// compares the whole block (FR-022, SC-05).
constexpr std::uint64_t kIterations = 1000;
constexpr double kRealNs = 250.0;
constexpr double kCpuNs = 200.0;
constexpr std::uint64_t kEndpointSamples = 2;
constexpr auto kRowTail =
    " iterations=      1000  real time/iter (ns)=  250.000  "
    "cpu time/iter (ns)=  200.000  overhead floor (ns)=    0.000  "
    "endpoint samples=2";

constexpr auto kScratchFile = "harness_placement_report_out.txt";

// The topology line FR-020 prints over the four-group layout of SC-04.
constexpr auto kTopologyLine =
    "topology: packages=1 l3-groups=4 cores=24 threads=48";

struct Policy
{
  sg::WaitMode waitMode;
  std::string placement;
  bool applied = true;
};

struct Scenario
{
  const char* name;
  std::function<void()> body;
};

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PLACEMENT REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Builds `groups` groups of `coresPerGroup` cores with `siblings`
// threads each, spaced 12 cpus apart per group, and installs it.
auto installShape(const std::uint32_t groups,
                  const std::uint32_t coresPerGroup,
                  const std::uint32_t siblings) -> void
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t group = 0; group < groups; ++group) {
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = group;
      for (std::uint32_t sibling = 0; sibling < siblings; ++sibling) {
        const std::uint32_t cpu = group * 12 + core * 2 + sibling;
        sg::detail::LayoutThread thread;
        thread.cpuIndex = cpu;
        thread.coreOrdinal = static_cast<std::uint32_t>(installed.cores.size());
        thread.groupOrdinal = group;
        layoutCore.threads.push_back(
            static_cast<std::uint32_t>(installed.threads.size()));
        installed.threads.push_back(thread);
        installed.mask.push_back(cpu);
      }
      installed.cores.push_back(layoutCore);
    }
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * 12;
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      layoutGroup.cores.push_back(group * coresPerGroup + core);
    }
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// The four-group, 24-core, 48-thread layout of SC-04, installed so the
// topology line has the counts FR-020 names (T015 reads them through
// the same seam the printer reads them).
auto installFourGroupShape() -> void
{
  installShape(4, 6, 2);
}

// The scripted clock the run scenario needs: the runner asserts the
// host publishes machine/monotonic, and a threaded run reads
// thread_cpu beside it (FR-002).
auto installScriptedClock() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, 1'000);
  provider.setPoints("machine", "thread_cpu", {0}, 1'000);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));
}

auto bmSmtBody(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto makeResult(const std::string& name,
                const std::vector<Policy>& policies) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;
  auto row = sg::ResultRow {};
  row.iterations = kIterations;
  row.timePerIterationNs = kRealNs;
  row.cpuTimePerIterationNs = kCpuNs;
  row.endpointSamples = kEndpointSamples;
  result.rows.push_back(row);
  for (const auto& policy : policies) {
    result.contexts.push_back(
        sg::ContextDisclosure {.waitMode = policy.waitMode,
                               .placement = policy.placement,
                               .applied = policy.applied});
  }
  return result;
}

auto rowLine(const std::string& name) -> std::string
{
  auto line = name;
  line.resize(24, ' ');
  line += kRowTail;
  return line;
}

auto captureThrough(const std::function<void()>& body)
    -> std::vector<std::string>
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kScratchFile, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  body();
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

auto countLinesWith(const std::vector<std::string>& lines,
                    const std::string_view text) -> std::size_t
{
  auto count = std::size_t {0};
  for (const auto& line : lines) {
    if (line.find(text) != std::string::npos) {
      ++count;
    }
  }
  return count;
}

// FR-020, R-09: with a preset in effect the block carries exactly one
// topology line, directly below the version line. Red before T017: the
// shipped printer prints no topology line at all.
auto scenarioTopologyLine() -> void
{
  installFourGroupShape();
  const auto result = makeResult("bmTopo",
                                 {{sg::WaitMode::SPIN, "0@0"},
                                  {sg::WaitMode::SPIN, "12@1"},
                                  {sg::WaitMode::SPIN, "24@2"},
                                  {sg::WaitMode::SPIN, "36@3"}});
  const auto lines = captureThrough(
      [&]
      {
        sg::detail::printContext(true);
        sg::detail::printResult(result);
      });
  check(countLinesWith(lines, "topology:") == 1,
        "the block did not carry exactly one topology line (FR-020, R-09)");
  check(!lines.empty() && lines[0].starts_with("speedgun-ng "),
        "the version line did not stand first");
  check(lines.size() > 1 && lines[1] == kTopologyLine,
        "the topology line did not stand directly below the version line "
        "with the layout counts (FR-020)");
}

// FR-021: each context prints its planned `cpu@group` pair between the
// wait mode and the account flag.
auto scenarioCtxFields() -> void
{
  installFourGroupShape();
  const auto result = makeResult("bmCtx",
                                 {{sg::WaitMode::SPIN, "0@0"},
                                  {sg::WaitMode::SPIN, "12@1"},
                                  {sg::WaitMode::SPIN, "24@2"},
                                  {sg::WaitMode::SPIN, "36@3"}});
  const auto lines = captureThrough(
      [&]
      {
        sg::detail::printContext(true);
        sg::detail::printResult(result);
      });
  const auto expected =
      "  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/12@1/applied  "
      "ctx2=SPIN/24@2/applied  ctx3=SPIN/36@3/applied";
  check(countLinesWith(lines, expected) == 1,
        "the disclosure line did not print the planned cpu@group pairs "
        "(FR-021)");
}

// R-06, Risks: a pair longer than the field meets the standing cut mark.
auto scenarioCutMark() -> void
{
  installFourGroupShape();
  const auto result =
      makeResult("bmCut", {{sg::WaitMode::SPIN, "4294967295@0"}});
  const auto lines = captureThrough(
      [&]
      {
        sg::detail::printContext(true);
        sg::detail::printResult(result);
      });
  const auto expected = "  contexts=1  ctx0=SPIN/429496729~/applied";
  check(countLinesWith(lines, expected) == 1,
        "the pair over the field width did not print the cut mark (R-06)");
}

// FR-022, SC-05: with no preset the block stands byte for byte as
// feature 020 ships it, `NO_PIN` included and no topology line.
auto scenarioNoPresetByteIdentical() -> void
{
  installFourGroupShape();
  const auto result = makeResult("bmNoPreset",
                                 {{sg::WaitMode::SPIN, "NO_PIN", false},
                                  {sg::WaitMode::SPIN, "NO_PIN", false}});
  const auto lines = captureThrough(
      [&]
      {
        sg::detail::printContext(false);
        sg::detail::printResult(result);
      });
  const std::vector<std::string> expected {
      lines.empty() ? std::string {} : lines[0],
      rowLine("bmNoPreset"),
      "  contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN"};
  check(lines == expected,
        "the no-preset block did not stand byte for byte as feature 020 "
        "ships it (FR-022, SC-05)");
  check(countLinesWith(lines, "topology:") == 0,
        "a report with no preset printed a topology line (FR-022)");
}

// FR-021, FR-023 (T024): the `smt` pair of one core prints two
// cpu@group fields naming one group and the two cpus of one core,
// and the noise warning stays silent. Red against the bytes feature
// 020 ships: its default function renders the cpu index alone.
auto scenarioSmtDisclosure() -> void
{
  installShape(1, 1, 2);
  installScriptedClock();
  auto handle = sg::registerBenchmark(&bmSmtBody, "bmSmtReport");
  handle.threads(2).placement("smt");
  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = 8;
  sg::detail::Runner runner(options);
  sg::Instance* instance = nullptr;
  for (auto& candidate : sg::detail::instances()) {
    if (candidate.name == "bmSmtReport/threads:2") {
      instance = &candidate;
    }
  }
  check(instance != nullptr, "the expansion produced no instance");
  const auto result = runner.run(*instance);
  const auto lines = captureThrough([&] { sg::detail::printResult(result); });
  check(countLinesWith(lines, "ctx0=SPIN/0@0/applied") == 1
            && countLinesWith(lines, "ctx1=SPIN/1@0/applied") == 1,
        "the smt pair did not print two cpu@group fields for one core "
        "(FR-021)");
  check(
      countLinesWith(lines, "warning:") == 0,
      "the noise warning fired for a pair on two cpus of one core " "(FR-023)");
}

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
  std::printf("harness_placement_report_test %s: %s\n",
              scenario.name,
              passed ? "pass" : "FAIL");
  std::fflush(stdout);
  return passed;
}

}  // namespace

auto main() -> int
{
  const std::vector<Scenario> scenarios = {
      {"topologyLine", scenarioTopologyLine},
      {"ctxFields", scenarioCtxFields},
      {"cutMark", scenarioCutMark},
      {"noPresetByteIdentical", scenarioNoPresetByteIdentical},
      {"smtDisclosure", scenarioSmtDisclosure},
  };
  std::size_t failed = 0;
  for (const auto& scenario : scenarios) {
    if (!runIsolated(scenario)) {
      ++failed;
    }
  }
  std::printf("harness_placement_report_test: %zu scenarios, %zu fail\n",
              scenarios.size(),
              failed);
  return failed == 0 ? 0 : 1;
}
