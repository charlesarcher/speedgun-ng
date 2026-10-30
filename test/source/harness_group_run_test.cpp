// ============================================================================
// TDD test for the rounds of a grouped threaded attempt
// (specs/039-context-groups T017, T052; FR-001 to FR-005, FR-010, FR-022,
// FR-024 to FR-027, D-1, D-2, D-7, D-9).
//
// An attempt of a run that carries a partition meets on one start round and
// one stop round per group, each pair standing at its own group's member count,
// and a context arrives at its own group's rounds only (FR-022). A round that
// stood above the count of its own group would wait on arrivals that group
// never brings, and a round that stood below them would answer the barrier's
// own invariant, so the completion of each scenario below reads its
// participant counts off the partition (FR-024). The name each context
// discloses is the group that holds it, and an empty name is the ungrouped
// shape (FR-004).
//
// A run whose partition holds more than one group publishes context 0's setup
// outcome on a route that carries no round across groups, and runs the pair's
// second half once every body of the attempt has settled on its own thread's
// far side of the stop round (FR-026, D-7). A run that carries no partition,
// and a run whose one group holds every context, keep the standing route
// where the standing pair of rounds carries the publication (FR-004, FR-025,
// D-9).
//
// The scenarios install their machine shape through the layout seam, state its
// cpus online, script both time leaves on the fake provider, take the memory
// bind through the binder seam where a preset binds, and read no /sys file, no
// clock and no sleep (FR-036, FR-037). Each scenario owns one shape, one family
// and one provider script, so each runs in a re-exec of this binary and the
// parent reads its wait status. The stranded-member scenario stands in a child
// of its own that the dispatch ledger ends, so its abort reaches the parent as
// a signal and the parent reads that the fault came before any round was waited
// on (FR-027).
//
// T052 adds the scenarios that pace by a file. The path the `--group-layout`
// row settled reaches the instance the way the preset text does, so the
// groups, the names and the memberships one file states pace the attempt and
// reach the disclosure records the console block prints from (FR-002, FR-003,
// FR-005, D-2). The run reads that file itself: the fit checkpoint read it
// first, and a file that changed between the two reads ends that run with the
// reader's own line, so no run paces by a partition no reader accepted.
//
// Scenario map:
//   core-four-groups eight contexts over four L3 groups of two cores each,
//                    under `core`, over two repetitions: the run completes and
//                    discloses four groups of two (FR-001, FR-006, FR-008,
//                    FR-022, FR-024)
//   node-two-nodes   four contexts over the two-node shape under `node`: the
//                    run completes and alternates into `node-0` and `node-1`
//                    (FR-006, FR-008, FR-022)
//   one-group        four contexts of one L3 group stand one group holding
//                    every context, and the run completes (FR-002, D-9)
//   l3-single-groups the `l3` arm's four single-member groups complete
//                    (FR-006, FR-022, FR-024)
//   single-context   one context under a preset carries no group and meets no
//                    threaded round (FR-010)
//   ungrouped        four contexts with no preset stated complete behind the
//                    standing pair of rounds and name no group (FR-004,
//                    FR-007, FR-025)
//   file-four-groups eight contexts of no preset over the four groups one
//                    file states: the run completes and discloses the file's
//                    names and memberships (FR-002, FR-003, FR-005, D-2)
//   file-defaults    four contexts over two entries that name none disclose
//                    `g0` and `g1` in document order (FR-003, D-4)
//   file-preset-pace the same read paces in the same four groups of two under
//                    `core` and under the file that states them (FR-002,
//                    FR-003, D-1, D-2)
//   file-refused     a file the run's own read cannot parse ends that run
//                    with the reader's line and starts no body
//   entry-point-file the entry point hands the settled path to the instance it
//                    runs, and that run reaches its console block (FR-003)
//   setup-failure    a setup half that throws reaches every body of every
//                    group, leaves no body in the measured work, and skips the
//                    second half (FR-026, D-7)
//   teardown-order   the second half of the pair runs once every body of the
//                    attempt has settled (FR-026, D-7)
//   stranded-ledger  a runner callable that leaves one member undispatched
//                    meets the ledger fault in its own process (FR-027)
// ============================================================================

#include <algorithm>
#include <atomic>
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

#include "detail/gcov_flush.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

constexpr std::uint64_t kIterations = 4;
constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 800;

auto fail(const std::string_view what) -> void
{
  std::fprintf(stderr,
               "SPEEDGUN-NG TEST FAIL: %.*s\n",
               static_cast<int>(what.size()),
               what.data());
  std::exit(1);
}

auto check(const bool condition, const std::string_view what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The shape of `groups` L3 groups with `coresPerGroup` cores of `siblings`
// threads each, cpus ascending within the group order, beside `memoryNodes`
// memory nodes that split the cpu range in equal slices. The nodes stand
// unfiltered, the way the read states them, so the mask filters them at plan
// time, and the builder is the miniature of `fourGroupShape` at
// test/source/harness_cli_placement_test.cpp:260-297 (FR-036).
auto machineShape(const std::uint32_t groups,
                  const std::uint32_t coresPerGroup,
                  const std::uint32_t siblings,
                  const std::uint32_t memoryNodes) -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = memoryNodes > 0;
  installed.packages = 1;
  const std::uint32_t perGroup = coresPerGroup * siblings;
  for (std::uint32_t group = 0; group < groups; ++group) {
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = group;
      for (std::uint32_t sibling = 0; sibling < siblings; ++sibling) {
        sg::detail::LayoutThread thread;
        thread.cpuIndex = group * perGroup + core * siblings + sibling;
        thread.coreOrdinal = static_cast<std::uint32_t>(installed.cores.size());
        thread.groupOrdinal = group;
        layoutCore.threads.push_back(
            static_cast<std::uint32_t>(installed.threads.size()));
        installed.threads.push_back(thread);
        installed.mask.push_back(thread.cpuIndex);
      }
      installed.cores.push_back(layoutCore);
    }
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * perGroup;
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      layoutGroup.cores.push_back(group * coresPerGroup + core);
    }
    installed.groups.push_back(layoutGroup);
  }
  const std::uint32_t total = groups * perGroup;
  for (std::uint32_t node = 0; node < memoryNodes; ++node) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = node;
    for (std::uint32_t cpu = (node * total) / memoryNodes;
         cpu < ((node + 1) * total) / memoryNodes;
         ++cpu)
    {
      layoutNode.cpus.push_back(cpu);
    }
    installed.nodes.push_back(layoutNode);
  }
  return installed;
}

// Install the shape and state its cpus online, so the context start of a
// pinned context reads the cpu it names as one the host carries (FR-036).
auto installShape(const sg::detail::MachineLayout& shape) -> void
{
  sg::detail::installTestLayout(shape);
  sg::detail::installTestOnlineCpus(shape.mask);
}

// The binder of the `node` scenarios: every bind lands, so the run reaches
// its rounds and meets no refusal of a denied bind.
auto bindEveryNode(std::uint32_t /*node*/) -> bool
{
  return true;
}

auto restoreEveryNode() -> void {}

auto installBinder() -> void
{
  sg::detail::installTestMemoryBinder(
      sg::detail::MemoryBindOps {&bindEveryNode, &restoreEveryNode});
}

// The two time leaves, stepped by a constant per sampling action, so the
// attempt folds a scripted value and reads no clock of the host (FR-037).
auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the scripted provider registers");
}

// The layout files the file scenarios write, each beside this process number,
// so two readers of one name never share a file.
const std::string kProcessTag = std::to_string(static_cast<long>(getpid()));
const std::string kFourPath =
    "/tmp/harness_group_run_four_" + kProcessTag + ".yaml";
const std::string kDefaultsPath =
    "/tmp/harness_group_run_defaults_" + kProcessTag + ".yaml";
const std::string kPacePath =
    "/tmp/harness_group_run_pace_" + kProcessTag + ".yaml";
const std::string kRefusedPath =
    "/tmp/harness_group_run_refused_" + kProcessTag + ".yaml";

// The three names of the entry-point case: the file the parent writes before
// the fork and the two streams the exec'd child writes, the way the stranded
// child's ledger line reaches its parent.
constexpr const char* kEntryPath = "harness_group_run_entry.yaml";
constexpr const char* kEntryOutPath = "harness_group_run_entry.out";
constexpr const char* kEntryErrPath = "harness_group_run_entry.err";

// One layout file of the grammar FR-012 fixes, written whole.
auto writeLayout(const std::string& path, const std::string_view body) -> void
{
  std::ofstream file(path, std::ios::trunc);
  file << body;
  if (!file) {
    fail("the scenario can write its layout file " + path);
  }
}

// The whole text of one file.
auto readText(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The disclosure block of one console capture: the line that names the count
// of contexts, empty when the run printed none.
auto disclosureLine(const std::string& text) -> std::string
{
  std::istringstream stream(text);
  for (std::string line; std::getline(stream, line);) {
    if (line.contains("contexts=")) {
      return line;
    }
  }
  return {};
}

// The count of times one text stands inside another.
auto countText(const std::string& text,
               const std::string_view needle) -> std::size_t
{
  std::size_t found = 0;
  for (auto at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + needle.size()))
  {
    ++found;
  }
  return found;
}

// The bodies whose callables returned, one arrival per body. The arrival
// stands past the body's own timed work, so a reader that finds the count
// full stands past every body of the attempt.
std::atomic<int> bodyReturns {0};
std::atomic<int> setupHalves {0};
std::atomic<int> teardownHalves {0};
int settledAtTearDown = 0;

auto bmGroupBody(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
  bodyReturns.fetch_add(1, std::memory_order_release);
}

// The pair of the ordering scenario: the second half reads how many bodies
// had returned when it ran.
class OrderingFixture : public sg::Fixture
{
public:
  auto setUp([[maybe_unused]] sg::State& state) -> void override
  {
    ++setupHalves;
  }

  auto tearDown([[maybe_unused]] sg::State& state) -> void override
  {
    ++teardownHalves;
    settledAtTearDown = bodyReturns.load(std::memory_order_acquire);
  }
};

std::atomic<int> failingSetupHalves {0};

// The pair of the failed-setup scenario: the first half throws, so the
// attempt owes every body the failure and owes no second half.
class FailingSetupFixture : public sg::Fixture
{
public:
  auto setUp([[maybe_unused]] sg::State& state) -> void override
  {
    ++failingSetupHalves;
    throw std::runtime_error("the scripted group setup failure");
  }

  auto tearDown([[maybe_unused]] sg::State& state) -> void override
  {
    ++teardownHalves;
  }
};

auto needInstance(const std::string_view family) -> sg::Instance*
{
  sg::detail::expandRegistry();
  for (auto& instance : sg::detail::instances()) {
    if (instance.name.starts_with(family)) {
      return &instance;
    }
  }
  check(false, "the expansion produced the named instance");
  return nullptr;
}

// One run of one instance, with the group name each context disclosed. The
// layout path stands on the instance the way the entry point leaves it, so the
// scenario of a file run hands the runner the file it reads (FR-003).
struct GroupRun
{
  sg::BenchmarkResult result;
  std::vector<std::string> groups;
};

auto runInstance(const std::string_view family,
                 const std::uint64_t repetitions,
                 const std::string& layout = {}) -> GroupRun
{
  sg::detail::RunOptions options;
  options.fixedIterations = kIterations;
  options.repetitions = repetitions;
  sg::detail::Runner runner(options);

  auto* instance = needInstance(family);
  instance->groupLayout = layout;

  GroupRun run;
  run.result = runner.run(*instance);
  for (const auto& context : run.result.contexts) {
    run.groups.push_back(context.group);
  }
  return run;
}

// The partition shape the disclosed names answer: each position reads as the
// ordinal of its group among the names first met, so two runs of one partition
// shape read as equal whatever names their two sources spell (D-2).
auto paceShape(const std::vector<std::string>& names) -> std::vector<int>
{
  std::vector<std::string> seen;
  std::vector<int> shape;
  for (const auto& name : names) {
    const auto found = std::ranges::find(seen, name);
    if (found == seen.end()) {
      shape.push_back(static_cast<int>(seen.size()));
      seen.push_back(name);
      continue;
    }
    shape.push_back(static_cast<int>(found - seen.begin()));
  }
  return shape;
}

// The names one run disclosed, printed where a check fails, so a wrong
// partition reaches the log as the names it answered.
auto reportGroups(const std::vector<std::string>& groups) -> void
{
  for (const auto& name : groups) {
    std::fprintf(stderr, "group '%s'\n", name.c_str());
  }
}

// FR-001, FR-006, FR-008, FR-022, FR-024: eight contexts of one instance over
// four L3 groups of two cores each stand in four groups of two under the
// `core` arm. The run completes over two repetitions, so each repetition
// builds the rounds of one attempt per group and every group's two contexts
// reach both of them (FR-001, FR-022).
auto scenarioCoreFourGroups() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupCore");
  handle.threads(8).placement("core");

  const auto run = runInstance("bmGroupCore", 2);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run over four groups of two measured its row (FR-022)");
  check(std::ranges::count(run.result.rows,
                           sg::RowKind::REPETITION,
                           &sg::ResultRow::kind)
            == 2,
        "the instance of four groups measured both of its repetitions "
        "(FR-001)");
  const std::vector<std::string> expected {
      "l3-0", "l3-0", "l3-1", "l3-1", "l3-2", "l3-2", "l3-3", "l3-3"};
  if (run.groups != expected) {
    reportGroups(run.groups);
    fail("eight contexts over four L3 groups disclose no four groups of two "
         "(FR-001, FR-006, FR-008)");
  }
  std::puts("harness_group_run_test core-four-groups: ok");
  return 0;
}

// FR-006, FR-008, FR-022, D-3: four contexts over the shape of two memory
// nodes cycle the nodes, so the partition holds two groups of two and the
// names spell the node number the plan carries.
auto scenarioNodeTwoNodes() -> int
{
  installShape(machineShape(4, 2, 2, 2));
  installBinder();
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupNode");
  handle.threads(4).placement("node");

  const auto run = runInstance("bmGroupNode", 1);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run over two node groups measured its row (FR-022)");
  const std::vector<std::string> expected {
      "node-0", "node-1", "node-0", "node-1"};
  if (run.groups != expected) {
    reportGroups(run.groups);
    fail("four contexts over two memory nodes alternate into no node groups "
         "(FR-006, FR-008, D-3)");
  }
  std::puts("harness_group_run_test node-two-nodes: ok");
  return 0;
}

// FR-002, D-9: a preset whose contexts all land in one unit gives one group
// holding every context. The run's rounds then carry the participant set the
// standing shape carries, and the run completes with one name disclosed.
auto scenarioOneGroup() -> int
{
  installShape(machineShape(1, 4, 1, 0));
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupOne");
  handle.threads(4).placement("core");

  const auto run = runInstance("bmGroupOne", 1);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run of one group holding every context completes (D-9)");
  const std::vector<std::string> expected {"l3-0", "l3-0", "l3-0", "l3-0"};
  if (run.groups != expected) {
    reportGroups(run.groups);
    fail("four contexts of one L3 group name no single group (FR-002, D-9)");
  }
  std::puts("harness_group_run_test one-group: ok");
  return 0;
}

// FR-006, FR-022, FR-024: the `l3` arm seats one context per L3 unit, so the
// four groups of this shape stand single-member and each round releases on
// one arrival.
auto scenarioL3SingleGroups() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupL3");
  handle.threads(4).placement("l3");

  const auto run = runInstance("bmGroupL3", 1);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run of four single-member groups completes (FR-022, FR-024)");
  const std::vector<std::string> expected {"l3-0", "l3-1", "l3-2", "l3-3"};
  if (run.groups != expected) {
    reportGroups(run.groups);
    fail("the `l3` arm discloses no four single-member groups "
         "(FR-006, FR-008)");
  }
  std::puts("harness_group_run_test l3-single-groups: ok");
  return 0;
}

// FR-010: a partition applies above one context, so the instance of one
// context carries no group and meets no threaded round at all. The run keeps
// the standing single-context shape and discloses no context record.
auto scenarioSingleContext() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupSingle");
  handle.threads(1).placement("core");

  const auto run = runInstance("bmGroupSingle", 1);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the instance of one context keeps the standing single-context run "
        "(FR-010)");
  check(run.groups.empty(),
        "the instance of one context discloses no group of its own (FR-010)");
  std::puts("harness_group_run_test single-context: ok");
  return 0;
}

// FR-004, FR-007, FR-025: a run no source named a partition for keeps the
// standing shape: the pair of rounds over all of its contexts, and no group
// name on any of its records.
auto scenarioUngrouped() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupPlain");
  handle.threads(4);

  const auto run = runInstance("bmGroupPlain", 1);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the ungrouped run completes behind the standing pair of rounds "
         "(FR-004, FR-025)");
  check(run.groups == std::vector<std::string> {"", "", "", ""},
        "the run of no partition names no group on any context (FR-004)");
  std::puts("harness_group_run_test ungrouped: ok");
  return 0;
}

// FR-002, FR-003, FR-005, D-2: one file is this run's one partition source.
// The eight contexts stand with no preset stated, so the groups, the names and
// the memberships the file spells are the ones the attempt paces behind, and
// each name reaches the disclosure record of the two contexts that stand in
// its group (FR-002). The run completes over the four pairs of rounds, each
// pair released by its own two arrivals (D-1).
auto scenarioFileFourGroups() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  writeLayout(kFourPath,
              "groups:\n"
              "  - name: west\n"
              "    slots: [0, 4]\n"
              "  - name: east\n"
              "    slots: [1, 5]\n"
              "  - name: north\n"
              "    slots: [2, 6]\n"
              "  - name: south\n"
              "    slots: [3, 7]\n");
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupFileFour");
  handle.threads(8);

  const auto run = runInstance("bmGroupFileFour", 1, kFourPath);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run one file groups measured its row (FR-003)");
  const std::vector<std::string> expected {
      "west", "east", "north", "south", "west", "east", "north", "south"};
  if (run.groups != expected) {
    reportGroups(run.groups);
    fail("the file's four groups and their memberships reach no run "
         "(FR-002, FR-003)");
  }
  std::puts("harness_group_run_test file-four-groups: ok");
  return 0;
}

// FR-003, D-4: an entry that names none takes `g` beside its document index,
// and the run discloses that name for the slots the entry holds.
auto scenarioFileDefaults() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  writeLayout(kDefaultsPath, "groups:\n  - slots: [0, 2]\n  - slots: [1, 3]\n");
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupFileDefaults");
  handle.threads(4);

  const auto run = runInstance("bmGroupFileDefaults", 1, kDefaultsPath);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run of the default-named groups measured its row (FR-003)");
  const std::vector<std::string> expected {"g0", "g1", "g0", "g1"};
  if (run.groups != expected) {
    reportGroups(run.groups);
    fail("the entries that name none carry no document order (D-4)");
  }
  std::puts("harness_group_run_test file-defaults: ok");
  return 0;
}

// FR-002, FR-003, D-1, D-2: the two sources name one partition shape. The same
// read of four L3 groups over eight contexts paces in four groups of two under
// the `core` arm, and paces the same way under the file that states those four
// groups, so the two attempts build rounds of the same participant counts and
// each completes on its own arrivals (D-1). The file's run carries the file's
// names, the one partition the row settled (FR-003).
auto scenarioFilePresetPace() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  writeLayout(kPacePath,
              "groups:\n"
              "  - name: pair-a\n"
              "    slots: [0, 1]\n"
              "  - name: pair-b\n"
              "    slots: [2, 3]\n"
              "  - name: pair-c\n"
              "    slots: [4, 5]\n"
              "  - name: pair-d\n"
              "    slots: [6, 7]\n");
  auto presetHandle = sg::registerBenchmark(&bmGroupBody, "bmGroupPresetPace");
  presetHandle.threads(8).placement("core");
  auto fileHandle = sg::registerBenchmark(&bmGroupBody, "bmGroupFilePace");
  fileHandle.threads(8);

  const auto presetRun = runInstance("bmGroupPresetPace", 1);
  const auto fileRun = runInstance("bmGroupFilePace", 1, kPacePath);
  check(presetRun.result.outcome == sg::RunOutcome::MEASURED,
        "the preset run of the shared shape measured its row");
  check(fileRun.result.outcome == sg::RunOutcome::MEASURED,
        "the file run of the shared shape measured its row (FR-003, D-1)");
  const std::vector<int> fourPairsOfTwo {0, 0, 1, 1, 2, 2, 3, 3};
  check(paceShape(presetRun.groups) == fourPairsOfTwo,
        "the `core` arm paces the four groups its plan names (D-3)");
  check(paceShape(fileRun.groups) == fourPairsOfTwo,
        "the file paces the same four groups of two on the same read "
        "(FR-002, D-2)");
  check(std::ranges::none_of(fileRun.groups,
                             [](const std::string& name)
                             { return name.starts_with("l3-"); }),
        "the run the file groups names no preset unit (FR-003)");
  std::puts("harness_group_run_test file-preset-pace: ok");
  return 0;
}

// The run's own read of a file the fit checkpoint never saw in this state: the
// state a file that changed after that read leaves behind. The reader's line
// ends the run, no body starts, and the attempt paces by no partition the
// grammar refused.
auto scenarioFileRefused() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  writeLayout(kRefusedPath,
              "groups:\n"
              "  - name: wide\n"
              "    slots: [0, 1, 2]\n"
              "  - name: twice\n"
              "    slots: [2, 3]\n");
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupFileRefused");
  handle.threads(4);

  const auto run = runInstance("bmGroupFileRefused", 1, kRefusedPath);
  check(run.result.outcome == sg::RunOutcome::FAILED,
        "the run whose file no longer answers the grammar ends failed");
  check(run.result.reason.contains(kRefusedPath)
            && run.result.reason.contains("two entries"),
        "the failed run carries the reader's line naming the file and the "
        "slot: "
        + run.result.reason);
  check(bodyReturns.load(std::memory_order_acquire) == 0,
        "the file the run cannot read starts no body of its attempt");
  check(run.groups.empty(),
        "the run the reader refused discloses no context of its own");
  std::puts("harness_group_run_test file-refused: ok");
  return 0;
}

// FR-003: the child runs the entry point with the settled row, so the path
// reaches the instance through the entry point, and the run it groups reaches
// the block its four contexts stand in.
auto childEntryPointFile() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupEntryPoint");
  handle.threads(4);
  std::vector<std::string> words {"harness_group_run_test",
                                  std::string("--group-layout=") + kEntryPath,
                                  "--iterations=4",
                                  "--filter",
                                  "^bmGroupEntryPoint"};
  std::vector<char*> argv;
  for (auto& word : words) {
    argv.push_back(word.data());
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  return status;
}

// The entry point's file run as the parent reads it: the row settled, the run
// completed, and the disclosure block of its four contexts stands in the
// captured output. The names the block prints stand read off the same
// partition in `file-four-groups`.
auto readEntryPointFile(const std::string& self) -> void
{
  writeLayout(kEntryPath,
              "groups:\n"
              "  - name: front\n"
              "    slots: [0, 2]\n"
              "  - name: back\n"
              "    slots: [1, 3]\n");
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the entry-point child");
  }
  if (pid == 0) {
    if (std::freopen(kEntryOutPath, "w", stdout) == nullptr
        || std::freopen(kEntryErrPath, "w", stderr) == nullptr)
    {
      _exit(2);
    }
    char* const execArgv[] = {const_cast<char*>(self.c_str()),
                              const_cast<char*>("entry-point-file"),
                              nullptr};
    (void)execv(self.c_str(), execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the entry-point child");
  }
  const std::string out = readText(kEntryOutPath);
  const std::string err = readText(kEntryErrPath);
  check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "the entry point runs the file its row settled: " + err);
  check(out.contains("bmGroupEntryPoint"),
        "the run the entry point grouped reaches its row: " + err);
  check(!err.contains("group layout"),
        "a file that fits the batch answers no refusal line: " + err);
  const std::string block = disclosureLine(out);
  check(block.starts_with("  contexts=4"),
        "the grouped run prints the block of its four contexts: " + block);
  check(countText(block, "ctx") == 4,
        "the block carries one field for each context of the run: " + block);
  std::remove(kEntryPath);
  std::remove(kEntryOutPath);
  std::remove(kEntryErrPath);
}

// FR-026, D-7: the first half of the pair runs on context 0 before any body's
// measured work, so its failure reaches every body of the attempt, every body
// stands out of the measured work, and the second half never runs. The eight
// contexts of this shape stand in four groups, so the failure crosses three
// rounds that carry no context of another group.
auto scenarioSetupFailure() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerFixtureBenchmark(
      &bmGroupBody,
      "bmGroupSetupFail",
      [] { return std::make_unique<FailingSetupFixture>(); });
  handle.threads(8).placement("core");

  const auto run = runInstance("bmGroupSetupFail", 1);
  check(run.result.outcome == sg::RunOutcome::FAILED,
        "the attempt whose setup half threw ends failed (FR-026)");
  check(run.result.reason.contains("the scripted group setup failure"),
        "the failure names the reason the setup half threw (FR-026)");
  check(failingSetupHalves.load(std::memory_order_acquire) == 1,
        "the setup half of the pair runs once for the attempt (D-7)");
  check(bodyReturns.load(std::memory_order_acquire) == 0,
        "no body of a failed setup reaches the measured work of its group "
        "(FR-026, D-7)");
  check(teardownHalves.load(std::memory_order_acquire) == 0,
        "a failed setup leaves the attempt no second half of the pair "
        "(FR-026, D-7)");
  std::puts("harness_group_run_test setup-failure: ok");
  return 0;
}

// FR-026, D-7: the second half runs on context 0's own thread once every body
// of the attempt has settled past its group's stop round, so the count of
// bodies that had returned their work reads full when that half runs.
auto scenarioTeardownOrder() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerFixtureBenchmark(
      &bmGroupBody,
      "bmGroupOrder",
      [] { return std::make_unique<OrderingFixture>(); });
  handle.threads(8).placement("core");

  const auto run = runInstance("bmGroupOrder", 1);
  check(run.result.outcome == sg::RunOutcome::MEASURED,
        "the run of the ordering fixture measured its row (FR-026)");
  check(setupHalves.load(std::memory_order_acquire) == 1,
        "the first half of the pair runs once for the attempt (D-7)");
  check(teardownHalves.load(std::memory_order_acquire) == 1,
        "the second half of the pair runs once for the attempt (D-7)");
  check(settledAtTearDown == 8,
        "the second half of the pair follows the last settled body of the "
        "attempt (FR-026, D-7)");
  std::puts("harness_group_run_test teardown-order: ok");
  return 0;
}

// FR-027: this callable returns with the last index of the attempt
// undispatched. The ledger under the call answers before the harness waits on
// any group round, so the child ends at the fault and never at a round its own
// partition left short of a member.
auto leaveTheLastIndex(sg::ThreadRun& run) -> void
{
  for (int index = 0; index + 1 < run.contextCount(); ++index) {
    run.dispatch(index);
  }
}

auto childStrandedLedger() -> int
{
  installShape(machineShape(4, 2, 1, 0));
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGroupBody, "bmGroupStranded");
  handle.threads(4).placement("core").threadRunner(&leaveTheLastIndex);

  (void)runInstance("bmGroupStranded", 1);
  fail("a runner that left a member undispatched reached no ledger fault "
       "(FR-027)");
  return 0;
}

// The ledger fault as the parent reads it: the signal of the child that the
// contract ended, and the line that names the body index it ended on.
auto readStrandedLedger(const std::string& self) -> void
{
  const char* const path = "harness_group_run_ledger.txt";
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the stranded child");
  }
  if (pid == 0) {
    if (std::freopen(path, "w", stderr) == nullptr) {
      _exit(2);
    }
    char* const execArgv[] = {const_cast<char*>(self.c_str()),
                              const_cast<char*>("stranded-ledger"),
                              nullptr};
    (void)execv(self.c_str(), execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the stranded child");
  }
  check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
        "the undispatched member ends the child at the contract before any "
        "group round is waited on (FR-027)");

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string message = buffer.str();
  if (!message.contains("[precondition]") || !message.contains("undispatched"))
  {
    std::fprintf(stderr, "no ledger line in '%s'\n", message.c_str());
    fail("the stranded member answered no ledger fault (FR-027)");
  }
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (!mode.empty()) {
    sg::test::flushCountersOnAbort();
  }
  if (mode == "core-four-groups") {
    return scenarioCoreFourGroups();
  }
  if (mode == "node-two-nodes") {
    return scenarioNodeTwoNodes();
  }
  if (mode == "one-group") {
    return scenarioOneGroup();
  }
  if (mode == "l3-single-groups") {
    return scenarioL3SingleGroups();
  }
  if (mode == "single-context") {
    return scenarioSingleContext();
  }
  if (mode == "ungrouped") {
    return scenarioUngrouped();
  }
  if (mode == "file-four-groups") {
    return scenarioFileFourGroups();
  }
  if (mode == "file-defaults") {
    return scenarioFileDefaults();
  }
  if (mode == "file-preset-pace") {
    return scenarioFilePresetPace();
  }
  if (mode == "file-refused") {
    return scenarioFileRefused();
  }
  if (mode == "entry-point-file") {
    return childEntryPointFile();
  }
  if (mode == "setup-failure") {
    return scenarioSetupFailure();
  }
  if (mode == "teardown-order") {
    return scenarioTeardownOrder();
  }
  if (mode == "stranded-ledger") {
    return childStrandedLedger();
  }

  // Each scenario owns one shape, one family and one provider script, so each
  // runs in a re-exec of this binary; a failed check exits there and shows
  // here.
  const std::string self = argc > 0 ? argv[0] : "harness_group_run_test";
  for (const auto& scenario : {"core-four-groups",
                               "node-two-nodes",
                               "one-group",
                               "l3-single-groups",
                               "single-context",
                               "ungrouped",
                               "file-four-groups",
                               "file-defaults",
                               "file-preset-pace",
                               "file-refused",
                               "setup-failure",
                               "teardown-order"})
  {
    const int status =
        std::system(("\"" + self + "\" " + scenario + " 2>/dev/null").c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes");
  }

  readStrandedLedger(self);
  readEntryPointFile(self);

  std::puts("harness_group_run_test: ok");
  return 0;
}
