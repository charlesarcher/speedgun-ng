// ============================================================================
// TDD test for the rounds of a grouped threaded attempt
// (specs/039-context-groups T017; FR-001, FR-004, FR-010, FR-022, FR-024 to
// FR-027, D-7, D-9).
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

auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* const what) -> void
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

// One run of one instance, with the group name each context disclosed.
struct GroupRun
{
  sg::BenchmarkResult result;
  std::vector<std::string> groups;
};

auto runInstance(const std::string_view family,
                 const std::uint64_t repetitions) -> GroupRun
{
  sg::detail::RunOptions options;
  options.fixedIterations = kIterations;
  options.repetitions = repetitions;
  sg::detail::Runner runner(options);

  GroupRun run;
  run.result = runner.run(*needInstance(family));
  for (const auto& context : run.result.contexts) {
    run.groups.push_back(context.group);
  }
  return run;
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
                               "setup-failure",
                               "teardown-order"})
  {
    const int status =
        std::system(("\"" + self + "\" " + scenario + " 2>/dev/null").c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scripted scenario passes");
  }

  readStrandedLedger(self);

  std::puts("harness_group_run_test: ok");
  return 0;
}
