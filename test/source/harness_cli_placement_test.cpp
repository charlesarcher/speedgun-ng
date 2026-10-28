// ============================================================================
// TDD test for the placement option and the handle preset (T019; US3,
// FR-015, FR-016, FR-017, FR-036, contracts/cli-placement.md). prose-lint:
// allow reason="verbatim file name"
//
// A rejected option value exits the process from inside the entry point,
// the way the usage-error cases of harness_cli_test.cpp do, so every
// scenario runs in a re-exec of this binary with a mode argument, the
// child's standard output and standard error captured to files the parent
// reads after the wait.
//
// Scenario map:
//   badName             `--placement=l2` prints one line naming the
//                       rejected value and exits 2 with no benchmark run
//                       (FR-016)
//   cpuList             `--placement=0-3` prints one line naming the
//                       rejected value and exits 2 with no benchmark run
//                       (FR-017)
//   noArgument          `--placement` with no argument keeps the standing
//                       `getopt_long` missing-argument path and exit
//                       status 2 (contracts/cli-placement.md)
//   handleWins          a handle preset wins over the command-line
//                       preset: the fit line names the handle's preset
//                       (FR-015)
//   handleBadName       a handle name the system does not provide prints
//                       one line naming the rejected text and exits 2
//                       with no benchmark run (FR-016)
//   unknownFlag         an unknown flag keeps the standing line and
//                       status 2, the guard for behavior the audit-point
//                       tree already has
//   fitStop             five contexts under `l3` on a four-group layout
//                       print one line naming the preset, the places
//                       and the ask, and exit 2 with no row (FR-018)
//   fitRange            a `threadRange` family stops the whole batch
//                       before the first run when one count does not
//                       fit (FR-018, Q-4)
//   topoListing         `--topology` prints the block and exits 0 with
//                       no benchmark run (FR-036)
//   topoUnreadable      `--topology` over a description the library
//                       rejects prints one line and exits 2 (R-07)
//   noneSkip            `--placement=none` takes the skip path: no
//                       topology line, the block feature 020 ships
//                       (FR-010, FR-022)
//   singleCtx           `--placement=l3` on one context keeps feature
//                       020's bytes with no plan and no pin (FR-019)
//   singleCtxUnreadable the same one-context run over a rejected
//                       description prints one line and exits 2 (Q-6)
//   topoFilter          `--topology` with a filter matching nothing
//                       still prints the listing and exits 0 (FR-036)
//   nodeName            `--placement=node` stands accepted, the
//                       one-context grammar run (specs/035 FR-001)
//   nodeUnknown         `nodes`, `NODE` and `node=` stand refused by
//                       the standing unknown-name refusal (specs/035
//                       FR-015)
//   nodeHandle          the handle name `node` stands accepted
//                       (specs/035 FR-001)
//   topoNodes           the node lines stand under the L3 lines in the
//                       read's node order (specs/035 T013, FR-006)
//   topoNodeExcluded    a node the mask excludes prints no line
//                       (specs/035 T013, FR-006)
//   topoNodeNoMemory    a read with no memory information prints
//                       `node: not available` alone (specs/035 T013,
//                       FR-007)
// ============================================================================

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CLI PLACEMENT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmCliBadName(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliCpuList(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliNoArgument(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliHandleSmt(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliHandleBad(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliUnknownFlag(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliFitStop(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliFitRange(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliTopoListing(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliTopoUnreadable(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliNoneSkip(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliSingleCtx(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliSingleCtxUnreadable(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliTopoFilter(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliNodeName(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliNodeUnknown(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliNodeHandle(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliTopoNodes(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliTopoNodeExcluded(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCliTopoNodeNoMemory(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// Each child registers exactly the benchmark its scenario filters for, so
// a stop line can only come from the input that scenario names (FR-015,
// FR-016).
auto setupBadName() -> void
{
  (void)sg::registerBenchmark(&bmCliBadName, "bmCliBadName");
}

auto setupCpuList() -> void
{
  (void)sg::registerBenchmark(&bmCliCpuList, "bmCliCpuList");
}

auto setupNoArgument() -> void
{
  (void)sg::registerBenchmark(&bmCliNoArgument, "bmCliNoArgument");
}

auto setupHandleWins() -> void
{
  auto handle = sg::registerBenchmark(&bmCliHandleSmt, "bmCliHandleSmt");
  handle.threads(4).placement("smt");
}

auto setupHandleBad() -> void
{
  auto handle = sg::registerBenchmark(&bmCliHandleBad, "bmCliHandleBad");
  handle.placement("l2");
}

auto setupUnknownFlag() -> void
{
  (void)sg::registerBenchmark(&bmCliUnknownFlag, "bmCliUnknownFlag");
}

// The four-group, 24-core, 48-thread layout of SC-04, so the fit check
// and the listing answer the counts the scenarios read (FR-018, FR-036).
auto fourGroupShape(bool withNodes) -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = withNodes;
  installed.packages = 1;
  for (std::uint32_t group = 0; group < 4; ++group) {
    for (std::uint32_t core = 0; core < 6; ++core) {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = group;
      for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
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
    for (std::uint32_t core = 0; core < 6; ++core) {
      layoutGroup.cores.push_back(group * 6 + core);
    }
    installed.groups.push_back(layoutGroup);
  }
  // specs/035 FR-005: node 0 holds groups 0 and 1, node 1 holds groups
  // 2 and 3, the cpus the read states unfiltered.
  for (std::uint32_t node = 0; withNodes && node < 2; ++node) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = node;
    for (std::uint32_t cpu = node * 24; cpu < (node + 1) * 24; ++cpu) {
      layoutNode.cpus.push_back(cpu);
    }
    installed.nodes.push_back(layoutNode);
  }
  return installed;
}

auto installFourGroupShape() -> void
{
  sg::detail::installTestLayout(fourGroupShape(false));
}

// The same shape with the two memory nodes the node lines read
// (specs/035 T013, FR-006).
auto installFourGroupShapeWithNodes() -> void
{
  sg::detail::installTestLayout(fourGroupShape(true));
}

// The two-node shape whose mask holds node 0's cpus alone, so node 1
// prints no line the way the L3 group it holds prints none (specs/035
// T013, FR-006).
auto installTwoNodeShapeNarrowMask() -> void
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = true;
  installed.packages = 1;
  for (const auto cpu : {0U, 1U}) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = cpu;
    thread.groupOrdinal = 0;
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  sg::detail::LayoutGroup layoutGroup;
  layoutGroup.lowestCpu = 0;
  layoutGroup.cores = {0, 1};
  installed.groups.push_back(layoutGroup);
  for (std::uint32_t node = 0; node < 2; ++node) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = node;
    layoutNode.cpus = {node * 2, static_cast<std::uint32_t>(node * 2 + 1)};
    installed.nodes.push_back(layoutNode);
  }
  sg::detail::installTestLayout(installed);
}

// Stands in place of an installed layout: the description reaches the
// host read and the library rejects it, leaving the layout unreadable
// (Q-6, R-07).
auto installRejectedDescription() -> void
{
  sg::detail::installTestTopologyDescription(
      std::string {"bogus layout description"});
}

auto setupFitStop() -> void
{
  installFourGroupShape();
  auto handle = sg::registerBenchmark(&bmCliFitStop, "bmCliFitStop");
  handle.threads(5);
}

auto setupFitRange() -> void
{
  installFourGroupShape();
  auto handle = sg::registerBenchmark(&bmCliFitRange, "bmCliFitRange");
  handle.threadRange(2, 8);
}

auto setupTopoListing() -> void
{
  installFourGroupShape();
  (void)sg::registerBenchmark(&bmCliTopoListing, "bmCliTopoListing");
}

auto setupTopoUnreadable() -> void
{
  installRejectedDescription();
  (void)sg::registerBenchmark(&bmCliTopoUnreadable, "bmCliTopoUnreadable");
}

auto setupNoneSkip() -> void
{
  installFourGroupShape();
  auto handle = sg::registerBenchmark(&bmCliNoneSkip, "bmCliNoneSkip");
  handle.threads(2);
}

auto setupSingleCtx() -> void
{
  installFourGroupShape();
  (void)sg::registerBenchmark(&bmCliSingleCtx, "bmCliSingleCtx");
}

auto setupSingleCtxUnreadable() -> void
{
  installRejectedDescription();
  (void)sg::registerBenchmark(&bmCliSingleCtxUnreadable,
                              "bmCliSingleCtxUnreadable");
}

auto setupTopoFilter() -> void
{
  (void)sg::registerBenchmark(&bmCliTopoFilter, "bmCliTopoFilter");
}

auto setupNodeName() -> void
{
  (void)sg::registerBenchmark(&bmCliNodeName, "bmCliNodeName");
}

auto setupNodeUnknown() -> void
{
  (void)sg::registerBenchmark(&bmCliNodeUnknown, "bmCliNodeUnknown");
}

auto setupNodeHandle() -> void
{
  auto handle = sg::registerBenchmark(&bmCliNodeHandle, "bmCliNodeHandle");
  handle.threads(1).placement("node");
}

auto setupTopoNodes() -> void
{
  installFourGroupShapeWithNodes();
  (void)sg::registerBenchmark(&bmCliTopoNodes, "bmCliTopoNodes");
}

auto setupTopoNodeExcluded() -> void
{
  installTwoNodeShapeNarrowMask();
  (void)sg::registerBenchmark(&bmCliTopoNodeExcluded,
                              "bmCliTopoNodeExcluded");
}

auto setupTopoNodeNoMemory() -> void
{
  installFourGroupShape();
  (void)sg::registerBenchmark(&bmCliTopoNodeNoMemory,
                              "bmCliTopoNodeNoMemory");
}

auto readFile(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Fork a child that captures its two streams, registers the handle
// presets when `withHandles` stands, runs the entry point with
// `arguments`, and exits with its status; return the status and both
// captures. A rejection may exit from inside the entry point, so the
// work stands in a child either way.
struct Captured
{
  int status;
  std::string out;
  std::string err;
};

auto runChild(const char* mode,
              const std::vector<std::string>& arguments,
              void (*setup)()) -> Captured
{
  const std::string outPath = std::string("cli-placement-") + mode + "-out.txt";
  const std::string errPath = std::string("cli-placement-") + mode + "-err.txt";

  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    if (std::freopen(outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
    {
      _exit(3);
    }
    if (setup != nullptr) {
      setup();
    }
    std::vector<std::string> argvStorage {"harness_cli_placement_test"};
    for (const auto& argument : arguments) {
      argvStorage.push_back(argument);
    }
    std::vector<char*> argv;
    for (auto& argument : argvStorage) {
      argv.push_back(argument.data());
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    (void)std::fflush(stdout);
    (void)std::fflush(stderr);
    // std::exit runs the atexit handlers, and the coverage preset writes
    // its counters from one; the child's walk of the placement paths must
    // reach that handler (T032, SC-08). The two streams are flushed
    // first, so the inherited stdio state contributes nothing twice and
    // the captured files hold exactly the child's bytes.
    std::exit(status);
  }
  int waitStatus = 0;
  if (waitpid(pid, &waitStatus, 0) < 0) {
    fail("cannot wait for the child");
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the child did not exit normally");
  }
  return Captured {
      WEXITSTATUS(waitStatus), readFile(outPath), readFile(errPath)};
}

// FR-016: a name outside the five stops the batch before any run.
auto scenarioBadName() -> int
{
  const Captured run =
      runChild("badName",
               {"--placement=l2", "--filter", "^bmCliBadName$"},
               &setupBadName);
  check(run.status == 2, "a rejected preset name exits 2 (FR-016)");
  check(run.err.find("l2") != std::string::npos,
        "the line names the rejected value (FR-016)");
  check(run.out.find("bmCliBadName") == std::string::npos,
        "no benchmark ran (FR-016)");
  return 0;
}

// FR-017: a value naming cpus is refused the same way.
auto scenarioCpuList() -> int
{
  const Captured run =
      runChild("cpuList",
               {"--placement=0-3", "--filter", "^bmCliCpuList$"},
               &setupCpuList);
  check(run.status == 2, "a cpu list exits 2 (FR-017)");
  check(run.err.find("0-3") != std::string::npos,
        "the line names the rejected value (FR-017)");
  check(run.out.find("bmCliCpuList") == std::string::npos,
        "no benchmark ran (FR-017)");
  return 0;
}

// contracts/cli-placement.md: the standing missing-argument path stands.
auto scenarioNoArgument() -> int
{
  const Captured run =
      runChild("noArgument",
               {"--placement", "--filter", "^bmCliNoArgument$"},
               &setupNoArgument);
  check(run.status == 2, "the missing argument exits 2");
  check(run.out.find("bmCliNoArgument") == std::string::npos,
        "no benchmark ran");
  return 0;
}

// FR-015: the fit check reads the handle's preset in place of the
// command-line default. On any host of the matrix `smt` places fewer than
// four contexts, so the handle winning shows as the line naming `smt`.
auto scenarioHandleWins() -> int
{
  const Captured run =
      runChild("handleWins",
               {"--placement=l3", "--filter", "^bmCliHandleSmt"},
               &setupHandleWins);
  check(run.status == 2, "the unfittable handle preset exits 2 (FR-015)");
  check(
      run.err.find("smt") != std::string::npos,
      "the line names the handle's preset, not the command line's " "(FR-015)");
  check(run.out.find("bmCliHandleSmt") == std::string::npos,
        "no benchmark ran (FR-015)");
  return 0;
}

// FR-016: a handle name outside the five stops the expansion.
auto scenarioHandleBadName() -> int
{
  const Captured run = runChild(
      "handleBadName", {"--filter", "^bmCliHandleBad$"}, &setupHandleBad);
  check(run.status == 2, "a rejected handle name exits 2 (FR-016)");
  check(run.err.find("l2") != std::string::npos,
        "the line names the rejected text (FR-016)");
  check(run.out.find("bmCliHandleBad") == std::string::npos,
        "no benchmark ran (FR-016)");
  return 0;
}

// The standing unknown-option line stands beside the new option.
auto scenarioUnknownFlag() -> int
{
  const Captured run =
      runChild("unknownFlag",
               {"--placements=l3", "--filter", "^bmCliUnknownFlag$"},
               &setupUnknownFlag);
  check(run.status == 2, "an unknown flag exits 2");
  check(run.err.find("unknown option") != std::string::npos,
        "the standing unknown-option line stands");
  return 0;
}

// One line naming the preset, the place count and the requested count
// answers a plan the layout cannot hold; the stop precedes every run,
// so no row stands (FR-018).
auto lineNaming(const std::string& text,
                const std::vector<std::string>& tokens) -> bool
{
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    const bool names =
        std::ranges::all_of(tokens,
                            [&](const std::string& token)
                            { return line.find(token) != std::string::npos; });
    if (names) {
      return true;
    }
  }
  return false;
}

auto countLines(const std::string& text) -> std::size_t
{
  std::size_t count = 0;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      ++count;
    }
  }
  return count;
}

// FR-018: five contexts under four places stop the batch.
auto scenarioFitStop() -> int
{
  const Captured run = runChild("fitStop",
                                {"--placement=l3", "--filter", "^bmCliFitStop"},
                                &setupFitStop);
  check(run.status == 2, "five contexts under four places exit 2 (FR-018)");
  check(lineNaming(run.err, {"l3", "4", "5"}),
        "one line names the preset, the places and the ask (FR-018)");
  check(countLines(run.err) == 1, "the stop prints one line (FR-018)");
  check(run.out.find("bmCliFitStop") == std::string::npos,
        "no context started and no row printed (FR-018)");
  return 0;
}

// FR-018, Q-4: the batch counts 2, 4 and 8; the unfittable 8 stops the
// whole family before the first run, the fitting counts included.
auto scenarioFitRange() -> int
{
  const Captured run =
      runChild("fitRange",
               {"--placement=l3", "--filter", "^bmCliFitRange"},
               &setupFitRange);
  check(run.status == 2, "the unfittable count stops the whole batch (Q-4)");
  check(run.out.find("bmCliFitRange") == std::string::npos,
        "no count of the range ran (Q-4)");
  check(lineNaming(run.err, {"l3"}), "the line names the preset (FR-018)");
  return 0;
}

// FR-036: the listing block of data-model.md E-10, then exit 0.
auto scenarioTopoListing() -> int
{
  const Captured run =
      runChild("topoListing",
               {"--topology", "--filter", "^bmCliTopoListing$"},
               &setupTopoListing);
  check(run.status == 0, "the listing exits 0 (FR-036)");
  check(run.out.find("topology: packages=1 l3-groups=4 cores=24 threads=48")
            != std::string::npos,
        "the counts line answers the installed layout (FR-036, SC-12)");
  check(run.out.find("mask=0-47") != std::string::npos,
        "the counts line carries the mask it was built from (FR-036)");
  check(run.out.find("l3 0: 0-11") != std::string::npos
            && run.out.find("l3 3: 36-47") != std::string::npos,
        "one line per L3 group names its cpus inside the mask (FR-036)");
  check(run.out.find("bmCliTopoListing") == std::string::npos,
        "no benchmark ran (FR-036)");
  return 0;
}

// R-07: an unreadable layout makes the listing one line and exit 2.
auto scenarioTopoUnreadable() -> int
{
  const Captured run =
      runChild("topoUnreadable",
               {"--topology", "--filter", "^bmCliTopoUnreadable$"},
               &setupTopoUnreadable);
  check(run.status == 2, "the unreadable listing exits 2 (R-07)");
  check(countLines(run.err) == 1,
        "the unreadable listing prints one line (R-07)");
  check(run.out.find("bmCliTopoUnreadable") == std::string::npos,
        "no benchmark ran (R-07)");
  return 0;
}

// FR-010, FR-022: the none preset skips the fit check and the topology
// line, and the block stands as feature 020 ships it.
auto scenarioNoneSkip() -> int
{
  const Captured run =
      runChild("noneSkip",
               {"--placement=none", "--filter", "^bmCliNoneSkip"},
               &setupNoneSkip);
  check(run.status == 0, "the none preset exits 0 (FR-010)");
  check(run.out.find("topology:") == std::string::npos,
        "none prints no topology line (FR-010, FR-019)");
  check(run.out.find("ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN") != std::string::npos,
        "the block stands as feature 020 ships it (FR-022)");
  return 0;
}

// FR-019: one context takes no plan and no pin, and the bytes of
// feature 020 stand.
auto scenarioSingleCtx() -> int
{
  const Captured run =
      runChild("singleCtx",
               {"--placement=l3", "--filter", "^bmCliSingleCtx$"},
               &setupSingleCtx);
  check(run.status == 0, "a one-context run with a preset exits 0 (FR-019)");
  check(run.out.find("topology:") == std::string::npos,
        "one context takes no topology line (FR-019)");
  check(run.out.find("contexts=") == std::string::npos,
        "one context keeps the disclosure silence of feature 020 (FR-019)");
  check(run.out.find("bmCliSingleCtx") != std::string::npos,
        "the benchmark ran (FR-019)");
  return 0;
}

// FR-005, Q-6: the unreadable layout stops a preset run whatever the
// context count.
auto scenarioSingleCtxUnreadable() -> int
{
  const Captured run =
      runChild("singleCtxUnreadable",
               {"--placement=l3", "--filter", "^bmCliSingleCtxUnreadable$"},
               &setupSingleCtxUnreadable);
  check(run.status == 2, "the unreadable layout exits 2 at one context (Q-6)");
  check(countLines(run.err) == 1, "the stop prints one line (Q-6)");
  check(run.out.find("bmCliSingleCtxUnreadable") == std::string::npos,
        "no benchmark ran (Q-6)");
  return 0;
}

// FR-036: the listing runs before selection, the way `--catalog` does.
auto scenarioTopoFilter() -> int
{
  const Captured run = runChild("topoFilter",
                                {"--topology", "--filter", "^nothingMatches$"},
                                &setupTopoFilter);
  check(run.status == 0, "the listing precedes selection (FR-036)");
  check(run.out.find("topology:") != std::string::npos,
        "the listing prints with a filter that matches nothing (FR-036)");
  check(run.out.find("bmCliTopoFilter") == std::string::npos,
        "no benchmark ran (FR-036)");
  return 0;
}

// specs/035 FR-001: the name `node` stands accepted from the command
// line; a one-context run takes no plan and no bind, so the grammar
// alone stands here.
auto scenarioNodeName() -> int
{
  const Captured run = runChild("nodeName",
                                {"--placement=node", "--filter",
                                 "^bmCliNodeName$"},
                                &setupNodeName);
  check(run.status == 0, "the name node exits 0 (FR-001)");
  check(run.out.find("bmCliNodeName") != std::string::npos,
        "the benchmark ran (FR-001)");
  return 0;
}

// specs/035 FR-015: the standing unknown-name refusal covers the near
// names the node grammar invites, matched in full and nothing else.
auto scenarioNodeUnknown() -> int
{
  for (const auto* value : {"nodes", "NODE", "node="}) {
    const std::string mode = std::string("nodeUnknown-") + value;
    const std::string option = std::string("--placement=") + value;
    const Captured run = runChild(mode.c_str(),
                                  {option, "--filter", "^bmCliNodeUnknown$"},
                                  &setupNodeUnknown);
    check(run.status == 2, "a near name of node exits 2 (FR-015)");
    check(run.err.find(value) != std::string::npos,
          "the line names the rejected value (FR-015)");
    check(run.out.find("bmCliNodeUnknown") == std::string::npos,
          "no benchmark ran (FR-015)");
  }
  return 0;
}

// specs/035 FR-001: the handle carries the name `node` beside the five
// standing names.
auto scenarioNodeHandle() -> int
{
  const Captured run = runChild("nodeHandle",
                                {"--filter", "^bmCliNodeHandle$"},
                                &setupNodeHandle);
  check(run.status == 0, "the handle name node exits 0 (FR-001)");
  check(run.out.find("bmCliNodeHandle") != std::string::npos,
        "the benchmark ran (FR-001)");
  return 0;
}

// specs/035 T013, FR-006, D-3: the node lines stand under the L3 lines,
// in the read's node order, in the L3 lines' shape.
auto scenarioTopoNodes() -> int
{
  const Captured run = runChild("topoNodes",
                                {"--topology", "--filter",
                                 "^bmCliTopoNodes$"},
                                &setupTopoNodes);
  check(run.status == 0, "the listing with nodes exits 0 (FR-006)");
  const auto l3 = run.out.find("l3 3: 36-47");
  const auto node0 = run.out.find("node 0: 0-23");
  const auto node1 = run.out.find("node 1: 24-47");
  check(l3 != std::string::npos && node0 != std::string::npos
            && node1 != std::string::npos && l3 < node0 && node0 < node1,
        "one line per node stands under the L3 lines, in the read's "
        "node order (FR-006, D-3)");
  check(run.out.find("bmCliTopoNodes") == std::string::npos,
        "no benchmark ran (FR-006)");
  return 0;
}

// specs/035 T013, FR-006: a node whose cpus all stand outside the mask
// prints no line, the way the L3 group it holds prints none.
auto scenarioTopoNodeExcluded() -> int
{
  const Captured run = runChild("topoNodeExcluded",
                                {"--topology", "--filter",
                                 "^bmCliTopoNodeExcluded$"},
                                &setupTopoNodeExcluded);
  check(run.status == 0, "the masked listing exits 0 (FR-006)");
  check(run.out.find("node 0: 0-1") != std::string::npos,
        "the mask-touched node prints its cpus (FR-006)");
  check(run.out.find("node 1") == std::string::npos,
        "the excluded node prints no line (FR-006)");
  return 0;
}

// specs/035 T013, FR-007, R-02, R-09: a read with no memory information
// prints the not-available line in the node family's place and keeps
// the rest of the listing, exit status 0.
auto scenarioTopoNodeNoMemory() -> int
{
  const Captured run = runChild("topoNodeNoMemory",
                                {"--topology", "--filter",
                                 "^bmCliTopoNodeNoMemory$"},
                                &setupTopoNodeNoMemory);
  check(run.status == 0, "the no-memory listing exits 0 (FR-007)");
  check(run.out.find("node: not available") != std::string::npos,
        "the node family prints not available (FR-007)");
  check(run.out.find("l3 0: 0-11") != std::string::npos
            && run.out.find("l3 3: 36-47") != std::string::npos,
        "the rest of the listing keeps its bytes (FR-007, R-09)");
  return 0;
}

}  // namespace

auto main() -> int
{
  struct Scenario
  {
    const char* name;
    int (*run)();
  };

  const std::array<Scenario, 20> scenarios {
      Scenario {"badName", &scenarioBadName},
      Scenario {"cpuList", &scenarioCpuList},
      Scenario {"noArgument", &scenarioNoArgument},
      Scenario {"handleWins", &scenarioHandleWins},
      Scenario {"handleBadName", &scenarioHandleBadName},
      Scenario {"unknownFlag", &scenarioUnknownFlag},
      Scenario {"fitStop", &scenarioFitStop},
      Scenario {"fitRange", &scenarioFitRange},
      Scenario {"topoListing", &scenarioTopoListing},
      Scenario {"topoUnreadable", &scenarioTopoUnreadable},
      Scenario {"noneSkip", &scenarioNoneSkip},
      Scenario {"singleCtx", &scenarioSingleCtx},
      Scenario {"singleCtxUnreadable", &scenarioSingleCtxUnreadable},
      Scenario {"topoFilter", &scenarioTopoFilter},
      Scenario {"nodeName", &scenarioNodeName},
      Scenario {"nodeUnknown", &scenarioNodeUnknown},
      Scenario {"nodeHandle", &scenarioNodeHandle},
      Scenario {"topoNodes", &scenarioTopoNodes},
      Scenario {"topoNodeExcluded", &scenarioTopoNodeExcluded},
      Scenario {"topoNodeNoMemory", &scenarioTopoNodeNoMemory}};
  for (const auto& scenario : scenarios) {
    check(scenario.run() == 0, scenario.name);
  }

  std::printf("harness_cli_placement_test: all scenarios passed\n");
  return 0;
}
