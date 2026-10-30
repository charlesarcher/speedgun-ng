// ============================================================================
// TDD test for the group name of one context's disclosure record
// (specs/039-context-groups T001; FR-005, FR-031, A-1 of the partition
// contract).
//
// Wave 1 reads the public record alone: the aggregate the header owns, the
// empty-group answer, and the four fields a filled record carries. No run
// starts here, so no provider registers, no thread starts, and no clock
// reads. Wave 5 adds the console segment and the report key to this
// executable (FR-028, FR-029).
//
// The record stays an aggregate and `group` stands last, so the positional
// form below is the order proof: a member written elsewhere stops this file
// from compiling (FR-005).
//
// The console block wave 5 adds reads `sg::detail::printResult`, the seam the
// report runs through. Each scenario builds the disclosure records a run
// leaves on its result and compares every line the print emits, so no
// benchmark runs, no provider registers, and no thread starts. The line
// follows C of `contracts/group-partition.md`: the group stands last among
// the segments of one context, after the placement flag where that flag
// prints, and a run that carries no partition prints no segment and keeps
// master's bytes (FR-028, FR-033). The machine line keeps its standing rule
// of printing where the fit step reports a preset in effect, which a run the
// layout file grouped alone is not (FR-028). The rows keep their standing
// shape, the group riding no column among them (FR-035).
//
// The machine-line scenarios read a layout file and an installed machine
// layout, both installed in the scenario's own process, the way the standing
// placement report test installs a shape before the one cached read.
//
// Scenario map:
//   record            the public record, its empty group, and its field order
//                     (FR-005, FR-031, A-1)
//   groupAfterFlag    the group stands after the flag of a pin that landed
//                     and of one that did not (FR-028)
//   groupAfterNoPin   a context that named no cpu prints the group after its
//                     placement, where no flag stands (FR-028)
//   standingOrder     the count, the index, the wait mode, the placement, the
//                     flag and the group stand in that order (FR-028, D-4)
//   ungroupedBytes    the three standing shapes print the bytes master
//                     prints for a run that carries no partition (FR-033)
//   machineLineAbsent a run grouped by a file that carries no preset leaves
//                     the machine line out and names the file's groups (FR-028)
//   machineLinePrints the allowed pairing, a preset that names no partition
//                     beside the file, prints the machine line (FR-028)
//   rowsKeepShape     the merged row, its divisor and the per-context rows
//                     carry no group column (FR-035)
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

auto fail(char const* what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(bool cond, char const* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The record stays an aggregate, and the group name is a `std::string`
// spelled `group` with no prefix (FR-005, N-12).
static_assert(std::is_aggregate_v<sg::ContextDisclosure>);
static_assert(
    std::is_same_v<decltype(sg::ContextDisclosure::group), std::string>);

// ---------------------------------------------------------------------------
// The console segment block (T023, T024; FR-028, FR-033, FR-035, C of the
// partition contract).
// ---------------------------------------------------------------------------

// The values of the one measured row every scenario prints. The row exists to
// give the disclosure line something to stand under, and it carries no group
// field of its own (FR-035).
constexpr std::uint64_t kIterations = 1000;
constexpr double kRealNs = 250.0;
constexpr double kCpuNs = 200.0;
constexpr std::uint64_t kEndpointSamples = 2;

// The width of the name field the report prints for `%-24s`, and the bytes it
// prints after that field of a repetition row.
constexpr std::size_t kNameFieldWidth = 24;
constexpr auto kRowTail =
    " iterations=      1000  real time/iter (ns)=  250.000  "
    "cpu time/iter (ns)=  200.000  overhead floor (ns)=    0.000  "
    "endpoint samples=2";

// The scratch files, named for this test so a suite run in parallel holds no
// file in common with it: the captured report, and the layout file the
// machine-line scenarios hand the fit walk.
constexpr auto kScratchFile = "harness_group_report_out.txt";
constexpr auto kLayoutFile = "harness_group_report_layout.yaml";

// The file of the machine-line scenarios: four slots in two named groups, the
// partition one instance paces by (FR-002, FR-012, FR-015).
constexpr auto kLayoutText =
    "groups:\n"
    "  - name: pairA\n"
    "    slots: [0, 1]\n"
    "  - name: pairB\n"
    "    slots: [2, 3]\n";
constexpr int kLayoutContexts = 4;

// The machine line over the shape the machine-line scenarios install: the
// read of four cores that states no L3 unit, which is why the `core` preset
// names no partition for it (FR-006).
constexpr auto kMachineLine =
    "topology: packages=1 l3-groups=0 cores=4 threads=8";

// One context of the disclosure vector a scenario builds: the three standing
// fields beside the group name the run's partition answers (FR-005).
struct Context
{
  sg::WaitMode waitMode;
  std::string placement;
  bool applied = true;
  std::string group;
};

// The body of the families the machine-line scenarios register. No scenario
// runs it: the fit walk reads the instance the expansion produced.
auto bmSegmentBody(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// The line the standing report prints for one repetition row: the name field
// of twenty-four and the bytes that follow it.
auto rowLine(const std::string& name) -> std::string
{
  check(name.size() <= kNameFieldWidth,
        "the scenario names the result within the field");
  auto field = name;
  field.append(kNameFieldWidth - name.size(), ' ');
  return field + kRowTail;
}

// The result of one scenario: the row above, beside the disclosure records a
// run left on it.
auto makeResult(const std::string& name,
                const std::vector<Context>& contexts) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;

  auto row = sg::ResultRow {};
  row.iterations = kIterations;
  row.timePerIterationNs = kRealNs;
  row.cpuTimePerIterationNs = kCpuNs;
  row.endpointSamples = kEndpointSamples;
  result.rows.push_back(row);

  for (const auto& context : contexts) {
    result.contexts.push_back(
        sg::ContextDisclosure {.waitMode = context.waitMode,
                               .placement = context.placement,
                               .applied = context.applied,
                               .group = context.group});
  }
  return result;
}

// The percentile record of a run that captured chunks: the four default
// points, the merged column, and one column of its own per context
// (specs/021 FR-010, specs/021 FR-014).
auto addPercentiles(sg::BenchmarkResult& result) -> void
{
  auto column = std::vector<sg::PercentileValue> {};
  for (std::size_t point = 0; point < 4; ++point) {
    column.push_back(sg::PercentileValue {
        .available = true, .value = kRealNs + static_cast<double>(point)});
  }
  result.percentilePoints = {50.0, 90.0, 99.0, 99.9};
  result.rows.front().chunkCount = 1000;
  result.rows.front().chunkIterations = 1;
  result.rows.front().percentiles = {column};
  for (std::size_t context = 0; context < result.contexts.size(); ++context) {
    result.contextPercentiles.push_back({column});
  }
}

// The print of `body`, captured to the end of the print.
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

  auto file = std::ifstream(kScratchFile);
  auto buffer = std::stringstream();
  buffer << file.rdbuf();
  auto lines = std::vector<std::string> {};
  for (auto line = std::string(); std::getline(buffer, line);) {
    lines.push_back(line);
  }
  return lines;
}

// The printed report of `results`, captured to the end of the print.
auto printCaptured(const std::vector<sg::BenchmarkResult>& results)
    -> std::vector<std::string>
{
  return captureThrough(
      [&]
      {
        for (const auto& result : results) {
          sg::detail::printResult(result);
        }
      });
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

auto linesHolding(const std::vector<std::string>& lines,
                  const std::string_view needle) -> std::vector<std::string>
{
  auto found = std::vector<std::string> {};
  for (const auto& line : lines) {
    if (line.find(needle) != std::string::npos) {
      found.push_back(line);
    }
  }
  return found;
}

auto lineHolding(const std::vector<std::string>& lines,
                 const std::string_view needle) -> std::string
{
  const auto found = linesHolding(lines, needle);
  check(
      found.size() == 1,
      ("exactly one line of the report holds " + std::string(needle)).c_str());
  return found.empty() ? std::string() : found.front();
}

auto countLinesWith(const std::vector<std::string>& lines,
                    const std::string_view needle) -> std::size_t
{
  return linesHolding(lines, needle).size();
}

// The shape the machine-line scenarios install: four cores of two threads
// each, the read that states no L3 unit, beside one memory node (FR-006). The
// `core` preset seats four contexts on it and names no partition for it, the
// one pairing the layout file stands beside (FR-019, D-2).
auto noUnitShape() -> sg::detail::MachineLayout
{
  auto installed = sg::detail::MachineLayout {};
  installed.readable = true;
  installed.memoryReadable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 4; ++core) {
    auto layoutCore = sg::detail::LayoutCore {};
    for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
      auto thread = sg::detail::LayoutThread {};
      thread.cpuIndex = core * 2 + sibling;
      thread.coreOrdinal = core;
      layoutCore.threads.push_back(
          static_cast<std::uint32_t>(installed.threads.size()));
      installed.threads.push_back(thread);
      installed.mask.push_back(thread.cpuIndex);
    }
    installed.cores.push_back(layoutCore);
  }
  auto node = sg::detail::LayoutNode {};
  node.number = 0;
  for (std::uint32_t cpu = 0; cpu < 8; ++cpu) {
    node.cpus.push_back(cpu);
  }
  installed.nodes.push_back(node);
  return installed;
}

// The layout file of the two machine-line scenarios.
auto writeLayoutFile() -> void
{
  auto file = std::ofstream(kLayoutFile);
  file << kLayoutText;
  file.close();
  check(!file.fail(), "the scenario can write its layout file");
}

// The instance of one family the expansion produced. A family that stated a
// context count expands to one instance whose name carries that count, so the
// family name and the count name it.
auto needInstance(const std::string_view family,
                  const int contexts) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.threadCount == contexts && instance.name.rfind(family, 0) == 0)
    {
      return &instance;
    }
  }
  check(false, "the expansion produced the instance the scenario named");
  return nullptr;
}

// The disclosure vector a run carries under one partition: one record per
// position, each naming the group that holds it, no pin asked for (B-2).
auto namedBy(const std::vector<sg::detail::ContextGroup>& partition)
    -> std::vector<Context>
{
  auto slots = std::size_t();
  for (const auto& group : partition) {
    slots += group.slots.size();
  }
  auto contexts = std::vector<Context> {};
  for (std::size_t position = 0; position < slots; ++position) {
    const auto group = sg::detail::groupOfContext(partition, position);
    contexts.push_back(
        {sg::WaitMode::BLOCK, "NO_PIN", true, partition[group].name});
  }
  return contexts;
}

// FR-028, C of the partition contract: a grouped run names the group as the
// last segment of a pinned context, after the flag that accounts for its pin.
auto scenarioGroupAfterFlag() -> void
{
  const auto contexts = std::vector<Context> {
      {sg::WaitMode::SPIN, "3", true, "l3-0"},
      {sg::WaitMode::BLOCK, "7", false, "l3-1"},
  };
  const auto lines = printCaptured({makeResult("bmGroupFlag", contexts)});
  checkLines(lines,
             {rowLine("bmGroupFlag"),
              "  contexts=2  ctx0=SPIN/3/applied/l3-0"
              "  ctx1=BLOCK/7/unapplied/l3-1"},
             "a grouped run names the group last, after the flag of each"
             " placement account (FR-028)");
}

// FR-028, FR-015: a context whose slot named no cpu prints no flag, so its
// group stands right behind the placement word. The names are a layout file's
// own text, which rides the same slot as a preset's name does.
auto scenarioGroupAfterNoPin() -> void
{
  const auto contexts = std::vector<Context> {
      {sg::WaitMode::SPIN, "NO_PIN", true, "pairA"},
      {sg::WaitMode::BLOCK, "NO_PIN", false, "pairB"},
  };
  const auto lines = printCaptured({makeResult("bmGroupNoPin", contexts)});
  checkLines(
      lines,
      {rowLine("bmGroupNoPin"),
       "  contexts=2  ctx0=SPIN/NO_PIN/pairA  ctx1=BLOCK/NO_PIN/pairB"},
      "a context with no pin prints its group behind the placement word,"
      " where no flag stands (FR-028)");
}

// FR-028, D-4: the standing segments keep their order, and the group stands
// behind its own separator, so a placement text carrying the `@group` and the
// `@node` tails never reads as one token with the group name.
auto scenarioStandingOrder() -> void
{
  const auto contexts = std::vector<Context> {
      {sg::WaitMode::SPIN, "0@0@0", true, "node-0"},
      {sg::WaitMode::BLOCK, "24@1", true, "l3-1"},
  };
  const auto lines = printCaptured({makeResult("bmGroupOrder", contexts)});
  check(lineHolding(lines, "  contexts=")
            == "  contexts=2  ctx0=SPIN/0@0@0/applied/node-0"
                   "  ctx1=BLOCK/24@1/applied/l3-1",
        "the count, the context index, the wait mode, the placement, the flag"
        " and the group stand in that order (FR-028, D-4)");
}

// FR-033, D-12: a run that carries no partition prints the bytes master
// prints for the same three shapes: no empty segment, no separator, and no
// space the standing line did not carry.
auto scenarioUngroupedBytes() -> void
{
  const auto lines =
      printCaptured({makeResult("bmPlainPinned",
                                {{sg::WaitMode::SPIN, "3", true, ""},
                                 {sg::WaitMode::BLOCK, "7", false, ""}}),
                     makeResult("bmPlainNoPin",
                                {{sg::WaitMode::SPIN, "NO_PIN", true, ""},
                                 {sg::WaitMode::BLOCK, "NO_PIN", false, ""}}),
                     makeResult("bmPlainPair",
                                {{sg::WaitMode::SPIN, "0@0@0", true, ""},
                                 {sg::WaitMode::BLOCK, "24@1", true, ""}})});
  checkLines(lines,
             {rowLine("bmPlainPinned"),
              "  contexts=2  ctx0=SPIN/3/applied  ctx1=BLOCK/7/unapplied",
              rowLine("bmPlainNoPin"),
              "  contexts=2  ctx0=SPIN/NO_PIN  ctx1=BLOCK/NO_PIN",
              rowLine("bmPlainPair"),
              "  contexts=2  ctx0=SPIN/0@0@0/applied  ctx1=BLOCK/24@1/applied"},
             "an ungrouped run's line keeps the bytes master prints for each"
             " of the three shapes (FR-033)");
}

// FR-028: a run the layout file grouped and no preset names leaves the
// machine line out. The fit step answers no preset in effect for it, and the
// console line names the file's groups.
auto scenarioMachineLineAbsent() -> void
{
  sg::detail::installTestLayout(noUnitShape());
  writeLayoutFile();
  auto handle = sg::registerBenchmark(&bmSegmentBody, "bmFitLayout");
  handle.threads(kLayoutContexts);
  sg::detail::expandRegistry();
  const auto selected =
      std::vector<sg::Instance*> {needInstance("bmFitLayout", kLayoutContexts)};

  const auto layout = sg::detail::groupLayoutFitStatus(kLayoutFile, selected);
  check(layout.first == 0,
        "a file that fits the batch answers no refusal line (FR-020)");
  const auto fit = sg::detail::placementFitStatus(selected);
  check(fit.first == 0, "the fit walk carries an instance with no preset");
  check(!fit.second,
        "a run the file grouped alone left the fit step no preset in effect"
        " (FR-028)");

  const auto header =
      captureThrough([&fit] { sg::detail::printContext(fit.second); });
  check(header.size() == 1,
        "the block of a layout run stands one line deep, the machine line"
        " staying absent (FR-028)");
  check(!header.empty() && header.front().starts_with("speedgun-ng "),
        "the version line stands alone above a layout run (FR-028)");

  const auto partition =
      sg::detail::parseGroupLayout(kLayoutFile, kLayoutContexts);
  check(partition.has_value(),
        "the file the fit walk read answers a partition (FR-012)");
  const auto lines =
      printCaptured({makeResult("bmFitLayout", namedBy(partition.value()))});
  checkLines(lines,
             {rowLine("bmFitLayout"),
              "  contexts=4  ctx0=BLOCK/NO_PIN/pairA  ctx1=BLOCK/NO_PIN/pairA"
              "  ctx2=BLOCK/NO_PIN/pairB  ctx3=BLOCK/NO_PIN/pairB"},
             "the run the file grouped names the file's groups on its line"
             " (FR-028)");

  sg::detail::installTestLayout(std::nullopt);
  std::remove(kLayoutFile);
}

// FR-028, D-2: the allowed pairing, a preset above `none` that names no
// partition for the read in hand beside the file, keeps the machine line: the
// fit step reports a preset in effect for it.
auto scenarioMachineLinePrints() -> void
{
  sg::detail::installTestLayout(noUnitShape());
  writeLayoutFile();
  auto handle = sg::registerBenchmark(&bmSegmentBody, "bmFitPairing");
  handle.threads(kLayoutContexts).placement("core");
  sg::detail::expandRegistry();
  const auto selected = std::vector<sg::Instance*> {
      needInstance("bmFitPairing", kLayoutContexts)};

  const auto layout = sg::detail::groupLayoutFitStatus(kLayoutFile, selected);
  check(layout.first == 0,
        "a preset that names no partition for the read stands beside the file"
        " (FR-019, D-2)");
  const auto fit = sg::detail::placementFitStatus(selected);
  check(fit.first == 0, "the preset seats every context of the instance");
  check(fit.second,
        "the fit step reports a preset in effect for the allowed pairing"
        " (FR-028)");

  const auto header =
      captureThrough([&fit] { sg::detail::printContext(fit.second); });
  check(header.size() == 2,
        "the machine line stands directly below the version line for the"
        " pairing (FR-028)");
  check(header.size() == 2 && header[1] == kMachineLine,
        "the machine line names the counts of the read the plan came from"
        " (FR-020, FR-028)");

  sg::detail::installTestLayout(std::nullopt);
  std::remove(kLayoutFile);
}

// FR-035: the group reaches the disclosure line and no column. The same run
// printed with and without its groups differs on that line alone, and the
// per-context percentile rows keep their flat labels.
auto scenarioRowsKeepShape() -> void
{
  auto grouped = makeResult("bmShape",
                            {{sg::WaitMode::SPIN, "0", true, "l3-0"},
                             {sg::WaitMode::BLOCK, "1", true, "l3-1"}});
  auto ungrouped = makeResult("bmShape",
                              {{sg::WaitMode::SPIN, "0", true, ""},
                               {sg::WaitMode::BLOCK, "1", true, ""}});
  addPercentiles(grouped);
  addPercentiles(ungrouped);

  const auto withGroup = printCaptured({grouped});
  const auto withoutGroup = printCaptured({ungrouped});
  check(withGroup.size() == withoutGroup.size(),
        "the group added a line to the report of the same run (FR-035)");
  for (std::size_t index = 0;
       index < withGroup.size() && index < withoutGroup.size();
       ++index)
  {
    if (withGroup[index].find("contexts=") != std::string::npos) {
      continue;
    }
    check(withGroup[index] == withoutGroup[index],
          "the group moved the merged row, its divisor, or a percentile"
          " column (FR-035)");
  }

  const auto own = linesHolding(withGroup, "  percentile  ctx");
  check(own.size() == 2,
        "a grouped run prints one percentile row per context, the standing"
        " count (specs/021 FR-014, FR-035)");
  for (std::size_t index = 0; index < own.size(); ++index) {
    const auto flat = own[index].find("ctx" + std::to_string(index)
                                      + " primary time/iter (ns)")
        != std::string::npos;
    check(flat,
          "the per-context percentile row keeps its flat column label"
          " (FR-035)");
    check(own[index].find("l3-") == std::string::npos,
          "a group name rode a per-context percentile row (FR-035)");
  }
  check(countLinesWith(withGroup, "  percentile  primary time/iter (ns)") == 1,
        "the merged row prints its one percentile line, the standing shape"
        " (specs/021 FR-010, FR-035)");
}

auto scenarioRecord() -> void
{
  // ---- The disclosure block (specs/039 T001; FR-005, FR-031, A-1) ----

  // A record nobody filled names no group: the shape of a run that carries
  // no partition, and of a report document that carries no `group` key
  // (FR-004, FR-031).
  {
    const sg::ContextDisclosure ungrouped {};
    check(ungrouped.group.empty(),
          "disclosure: a default record names no group");
    check(ungrouped.waitMode == sg::WaitMode::BLOCK,
          "disclosure: a default record waits by blocking");
  }

  // A record built from the three standing fields names no group, the
  // shape every ungrouped run keeps (FR-004, FR-033).
  {
    const sg::ContextDisclosure three {sg::WaitMode::SPIN, "cpu@3", true};
    check(three.group.empty(),
          "disclosure: a three-field record names no group");
    check(three.waitMode == sg::WaitMode::SPIN,
          "disclosure: a three-field record keeps its wait mode");
  }

  // A filled record carries the name beside the wait mode, the placement
  // text and the applied flag, and the positional form proves the member
  // stands last (FR-005).
  {
    const sg::ContextDisclosure filled {
        sg::WaitMode::BLOCK, "NO_PIN", false, "l3-1"};
    check(filled.waitMode == sg::WaitMode::BLOCK,
          "disclosure: the filled record keeps its wait mode");
    check(filled.placement == "NO_PIN",
          "disclosure: the filled record keeps its placement text");
    check(!filled.applied,
          "disclosure: the filled record keeps its applied flag");
    check(filled.group == "l3-1",
          "disclosure: the filled record carries the group name");
  }

  // The designated form spells the member by its public name, the shape
  // the report reader writes for a document that carries the key (FR-005,
  // FR-031).
  {
    const sg::ContextDisclosure named {
        .waitMode = sg::WaitMode::SPIN,
        .placement = "cpu@7",
        .applied = true,
        .group = "node-0",
    };
    check(named.waitMode == sg::WaitMode::SPIN && named.placement == "cpu@7"
              && named.applied && named.group == "node-0",
          "disclosure: the designated form carries the four fields");
  }

  std::printf("harness_group_report_test record: ok\n");
}

// One scenario of the suite with the name the verdict line prints.
struct Scenario
{
  const char* name;
  void (*body)();
};

// Run one scenario in a child of this process and read its wait status, so one
// failing scenario reports beside the verdict of every other scenario, and so
// each scenario stands before the one machine read the harness caches (Q-5).
auto runIsolated(const Scenario& scenario) -> bool
{
  std::fflush(stdout);
  const pid_t child = fork();
  if (child < 0) {
    fail("cannot start the scenario process");
  }
  if (child == 0) {
    try {
      scenario.body();
    } catch (...) {
      std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: unhandled exception\n");
      std::exit(1);
    }
    std::fflush(stdout);
    std::exit(0);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    fail("cannot read the status of the scenario process");
  }
  const bool passed = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  std::printf("harness_group_report_test %s: %s\n",
              scenario.name,
              passed ? "pass" : "FAIL");
  std::fflush(stdout);
  return passed;
}

}  // namespace

auto main() -> int
{
  const std::vector<Scenario> scenarios = {
      {"record", scenarioRecord},
      {"groupAfterFlag", scenarioGroupAfterFlag},
      {"groupAfterNoPin", scenarioGroupAfterNoPin},
      {"standingOrder", scenarioStandingOrder},
      {"ungroupedBytes", scenarioUngroupedBytes},
      {"machineLineAbsent", scenarioMachineLineAbsent},
      {"machineLinePrints", scenarioMachineLinePrints},
      {"rowsKeepShape", scenarioRowsKeepShape},
  };
  std::size_t failed = 0;
  for (const auto& scenario : scenarios) {
    if (!runIsolated(scenario)) {
      ++failed;
    }
  }
  std::printf("harness_group_report_test: %zu scenarios, %zu fail\n",
              scenarios.size(),
              failed);
  return failed == 0 ? 0 : 1;
}
