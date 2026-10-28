// ============================================================================
// specs/035-numa-placement T003, T005, T007, T008, T009, T016: the node
// walk, the binder seam, the node plan, the bind at the context start and
// the refusal set (FR-005, FR-009 to FR-013, FR-016, R-01, R-03, R-05 to
// R-07).
//
// One layout read per process (FR-002) means each installed layout serves
// exactly one scenario, so every scenario that installs a layout runs in a
// fresh process by the fork and execv pattern of
// harness_placement_preset_test.cpp, naming the scenario in argv and
// re-installing the seams in the child. A scenario that drives a run
// through the entry point forks once more, the way
// harness_cli_placement_test.cpp captures the child's two streams.
//
// No test binds real memory: the binder seam stands installed with a fake
// that records the node the call names and denies the bind on demand
// (FR-016). The synthetic layouts carry every node shape the scenarios
// read, so no test carries a fixed node number of the host (FR-017), and
// no test sleeps and no test reads a clock (FR-018).
//
// Scenario map:
//   walkOneNode      a layout of one node states the node number and the
//                    cpu set the read carries (FR-005, R-01)
//   walkTwoNodes     a layout of two nodes states both nodes in the
//                    read's order (FR-005, R-01)
//   walkFourNodes    a layout of four nodes states all four (FR-005,
//                    FR-016)
//   walkNoMemory     a layout with no memory information states none
//                    (FR-005, R-01)
//   binderRecords    the installed fake records the node the call names
//                    (FR-016, R-06)
//   binderDenies     the fake denies the call on demand (FR-016, R-06)
//   binderClear      a null installer restores the production binder,
//                    the clear-on-null shape `installTestLayout` uses
//                    (FR-016, R-06)
//   nodeCycles       eight contexts over two nodes cycle the nodes,
//                    one cpu at a time (FR-009, R-03)
//   nodeFills        a count below the node count fills the nodes in
//                    the read's node order (FR-009)
//   nodeSkipsExcluded a node the mask excludes takes no context, and
//                    the places stand at the distinct-cpu count
//                    (FR-009, R-03)
//   nodeUnfittable   the empty plan carries the distinct-cpu count
//                    (FR-009, D-2)
//   nodeOneNode      a one-node host plans every context onto the one
//                    node (FR-009)
//   nodeDisclose     the rows and the JSON field read `cpu@group@node`,
//                    and the binder records validation, then the starts
//                    (T008, T009, FR-010, FR-011, R-05)
//   noNodePresetBytes a run whose preset plans no node keeps the
//                    standing bytes and records no bind (FR-002, R-08)
//   bindBypass       a user-installed PlacementFn cannot bypass the
//                    bind (T009, FR-010, R-05)
//   oneNode          the one-node run binds and names the one node
//                    (T009, FR-010)
//   refuseValidation a denial in the eager validation pass refuses one
//                    line naming the context and the node, exit 2
//                    (T016, FR-013, R-07)
//   refuseStart      a bind that fails at a context start stands the
//                    same refusal (T016, FR-013, R-07)
//   refuseNoMemory   the node preset over no memory information
//                    refuses one line before any context (T016,
//                    FR-014, R-10)
//   refuseUnfittable the standing unfittable-count refusal covers the
//                    node plan (T016, FR-013, D-2)
// ===========================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS NUMA PLACEMENT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto runChildScenario(const char* self, const char* mode) -> int
{
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    char* const execArgv[] = {
        const_cast<char*>(self), const_cast<char*>(mode), nullptr};
    (void)execv(self, execArgv);
    _exit(2);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the child");
  }
  return status;
}

// Installs a layout whose cpus stand in one node per pair of groups, the
// node numbers in the read's order, with `groups` L3 groups of
// `cpusPerGroup` cpus each and one node holding every `nodeSpan` groups.
auto installNodeLayout(const std::uint32_t groups,
                       const std::uint32_t cpusPerGroup,
                       const std::uint32_t groupsPerNode) -> void
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = groupsPerNode > 0;
  installed.packages = 1;
  const std::uint32_t cpus = groups * cpusPerGroup;
  for (std::uint32_t cpu = 0; cpu < cpus; ++cpu) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = cpu;
    thread.groupOrdinal = cpu / cpusPerGroup;
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  for (std::uint32_t group = 0; group < groups; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * cpusPerGroup;
    for (std::uint32_t cpu = 0; cpu < cpusPerGroup; ++cpu) {
      layoutGroup.cores.push_back(group * cpusPerGroup + cpu);
    }
    installed.groups.push_back(layoutGroup);
  }
  for (std::uint32_t node = 0;
       groupsPerNode > 0 && node * groupsPerNode < groups;
       ++node) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = node;
    for (std::uint32_t group = node * groupsPerNode;
         group < (node + 1) * groupsPerNode && group < groups;
         ++group)
    {
      for (std::uint32_t cpu = 0; cpu < cpusPerGroup; ++cpu) {
        layoutNode.cpus.push_back(group * cpusPerGroup + cpu);
      }
    }
    installed.nodes.push_back(layoutNode);
  }
  sg::detail::installTestLayout(installed);
}

// The fake binder of R-06: the statics record the nodes the calls name
// and the restores they follow, and deny the call on demand, so a
// scenario reads the record after the run it drove. The record stands
// behind one mutex, for the binds of the context threads that run
// beside each other (FR-010); the scenario reads it after the run
// joined them.
auto binderMutex() -> std::mutex&
{
  static std::mutex mutex;
  return mutex;
}

auto binderCalls() -> std::vector<std::uint32_t>&
{
  static std::vector<std::uint32_t> calls;
  return calls;
}

auto binderRestores() -> std::uint32_t&
{
  static std::uint32_t restores = 0;
  return restores;
}

// The node a bind is denied from: the default denies none.
auto binderDenyNodeFrom() -> std::uint32_t&
{
  static std::uint32_t denyFrom = ~std::uint32_t {0};
  return denyFrom;
}

// The call index a bind is denied from: the default denies none.
auto binderDenyCallFrom() -> std::optional<std::size_t>&
{
  static std::optional<std::size_t> denyFrom;
  return denyFrom;
}

auto fakeBind(std::uint32_t node) -> bool
{
  const std::lock_guard<std::mutex> guard(binderMutex());
  const std::size_t index = binderCalls().size();
  binderCalls().push_back(node);
  const auto byCall = binderDenyCallFrom();
  if (byCall.has_value() && index >= *byCall) {
    return false;
  }
  return node < binderDenyNodeFrom();
}

auto fakeRestore() -> void
{
  const std::lock_guard<std::mutex> guard(binderMutex());
  ++binderRestores();
}

auto installFakeBinder() -> void
{
  binderCalls().clear();
  binderRestores() = 0;
  binderDenyNodeFrom() = ~std::uint32_t {0};
  binderDenyCallFrom().reset();
  sg::detail::installTestMemoryBinder(
      sg::detail::MemoryBindOps {&fakeBind, &fakeRestore});
}

}  // namespace

// T003, FR-005: a layout of one node states the node number and the cpu
// set the read carries.
auto childWalkOneNode() -> int
{
  installNodeLayout(2, 4, 2);
  const auto& machine = sg::detail::layout();
  if (!machine.memoryReadable || machine.nodes.size() != 1) {
    std::fprintf(stderr, "the one-node layout did not read as one node\n");
    return 1;
  }
  const auto& node = machine.nodes.front();
  const std::vector<std::uint32_t> expected {0, 1, 2, 3, 4, 5, 6, 7};
  if (node.number != 0 || node.cpus != expected) {
    std::fprintf(stderr, "the one-node read carried the wrong node\n");
    return 1;
  }
  return 0;
}

// T003, FR-005: a layout of two nodes states both nodes in the read's
// order.
auto childWalkTwoNodes() -> int
{
  installNodeLayout(4, 4, 2);
  const auto& machine = sg::detail::layout();
  if (!machine.memoryReadable || machine.nodes.size() != 2) {
    std::fprintf(stderr, "the two-node layout did not read as two nodes\n");
    return 1;
  }
  const std::vector<std::uint32_t> first {0, 1, 2, 3, 4, 5, 6, 7};
  const std::vector<std::uint32_t> second {8, 9, 10, 11, 12, 13, 14, 15};
  if (machine.nodes[0].number != 0 || machine.nodes[0].cpus != first
      || machine.nodes[1].number != 1 || machine.nodes[1].cpus != second)
  {
    std::fprintf(stderr, "the two-node read carried the wrong nodes\n");
    return 1;
  }
  return 0;
}

// T003, FR-016: a layout of four nodes states all four.
auto childWalkFourNodes() -> int
{
  installNodeLayout(4, 2, 1);
  const auto& machine = sg::detail::layout();
  if (!machine.memoryReadable || machine.nodes.size() != 4) {
    std::fprintf(stderr, "the four-node layout did not read as four nodes\n");
    return 1;
  }
  for (std::uint32_t index = 0; index < 4; ++index) {
    const std::vector<std::uint32_t> expected {index * 2,
                                               static_cast<std::uint32_t>(
                                                   index * 2 + 1)};
    if (machine.nodes[index].number != index
        || machine.nodes[index].cpus != expected)
    {
      std::fprintf(stderr, "the four-node read carried the wrong node\n");
      return 1;
    }
  }
  return 0;
}

// T003, FR-005: a layout with no memory information states none.
auto childWalkNoMemory() -> int
{
  installNodeLayout(2, 4, 0);
  const auto& machine = sg::detail::layout();
  if (machine.memoryReadable || !machine.nodes.empty()) {
    std::fprintf(stderr,
                 "the layout with no memory information stated nodes\n");
    return 1;
  }
  return 0;
}

// T005, FR-016: the installed fake records the node the call names.
auto childBinderRecords() -> int
{
  installFakeBinder();
  const auto& binder = sg::detail::memoryBinder();
  if (!binder.bind(1U) || !binder.bind(0U)) {
    std::fprintf(stderr, "the fake binder denied a call it allows\n");
    return 1;
  }
  const std::vector<std::uint32_t> expected {1, 0};
  if (binderCalls() != expected) {
    std::fprintf(stderr, "the fake binder recorded the wrong nodes\n");
    return 1;
  }
  return 0;
}

// T005, FR-016: the fake denies the call on demand.
auto childBinderDenies() -> int
{
  installFakeBinder();
  binderDenyNodeFrom() = 1;
  const auto& binder = sg::detail::memoryBinder();
  if (!binder.bind(0U)) {
    std::fprintf(stderr, "the fake denied the node below the denial\n");
    return 1;
  }
  if (binder.bind(1U)) {
    std::fprintf(stderr, "the fake did not deny the node of the denial\n");
    return 1;
  }
  return 0;
}

// T005, FR-016: a null installer restores the production binder, the
// clear-on-null shape `installTestLayout` uses.
auto childBinderClear() -> int
{
  installFakeBinder();
  if (sg::detail::memoryBinder().bind != &fakeBind) {
    std::fprintf(stderr, "the fake binder did not install\n");
    return 1;
  }
  sg::detail::installTestMemoryBinder(std::nullopt);
  if (sg::detail::memoryBinder().bind == &fakeBind
      || sg::detail::memoryBinder().bind == nullptr
      || sg::detail::memoryBinder().restore == nullptr)
  {
    std::fprintf(stderr,
                 "the null installer did not restore the production binder\n");
    return 1;
  }
  return 0;
}

// T007, FR-009, R-03: eight contexts over two nodes cycle the nodes,
// one cpu at a time in the deal order the cycling rule states.
auto childNodeCycles() -> int
{
  installNodeLayout(4, 4, 2);
  const auto& layout = sg::detail::layout();
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 8);
  const std::vector<std::uint32_t> cpus {0, 8, 1, 9, 2, 10, 3, 11};
  const std::vector<std::uint32_t> nodes {0, 1, 0, 1, 0, 1, 0, 1};
  const std::vector<std::uint32_t> groups {0, 2, 0, 2, 0, 2, 0, 2};
  if (plan.places != 16 || plan.cpu != cpus || plan.node != nodes
      || plan.group != groups)
  {
    std::fprintf(stderr, "the node plan did not cycle the two nodes\n");
    return 1;
  }
  return 0;
}

// T007, FR-009: a count below the node count fills the nodes in the
// read's node order, one context each.
auto childNodeFills() -> int
{
  installNodeLayout(4, 4, 2);
  const auto& layout = sg::detail::layout();
  const auto one =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 1);
  if (one.cpu != std::vector<std::uint32_t> {0}
      || one.node != std::vector<std::uint32_t> {0})
  {
    std::fprintf(stderr, "one context did not fill the first node\n");
    return 1;
  }
  const auto two =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 2);
  const std::vector<std::uint32_t> twoCpus {0, 8};
  const std::vector<std::uint32_t> twoNodes {0, 1};
  if (two.cpu != twoCpus || two.node != twoNodes) {
    std::fprintf(stderr, "two contexts did not fill the two nodes\n");
    return 1;
  }
  const auto three =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 3);
  const std::vector<std::uint32_t> threeCpus {0, 8, 1};
  const std::vector<std::uint32_t> threeNodes {0, 1, 0};
  if (three.cpu != threeCpus || three.node != threeNodes) {
    std::fprintf(stderr, "three contexts did not cycle past the fill\n");
    return 1;
  }
  return 0;
}

// T007, FR-009, R-03: a node whose cpus all stand outside the mask
// takes no context, and the plan's places stand at the distinct-cpu
// count of the mask-touching nodes.
auto childNodeSkipsExcluded() -> int
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.memoryReadable = true;
  installed.packages = 1;
  for (const auto cpu : {0U, 1U, 4U, 5U}) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = cpu;
    thread.groupOrdinal = cpu < 2 ? 0 : 1;
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * 4;
    layoutGroup.cores = {group * 4, group * 4 + 1};
    installed.groups.push_back(layoutGroup);
  }
  const std::array<std::vector<std::uint32_t>, 3> nodeCpus {
      std::vector<std::uint32_t> {0, 1},
      std::vector<std::uint32_t> {2, 3},
      std::vector<std::uint32_t> {4, 5}};
  for (std::uint32_t number = 0; number < 3; ++number) {
    sg::detail::LayoutNode layoutNode;
    layoutNode.number = number;
    layoutNode.cpus = nodeCpus[number];
    installed.nodes.push_back(layoutNode);
  }
  sg::detail::installTestLayout(installed);
  const auto& layout = sg::detail::layout();
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 4);
  const std::vector<std::uint32_t> cpus {0, 4, 1, 5};
  const std::vector<std::uint32_t> nodes {0, 2, 0, 2};
  if (plan.places != 4 || plan.cpu != cpus || plan.node != nodes) {
    std::fprintf(stderr,
                 "the excluded node did not stand without a context\n");
    return 1;
  }
  return 0;
}

// T007, FR-009, D-2: the standing fit check bounds the node plan at the
// distinct-cpu count of the mask-touching nodes, and the empty plan
// carries that count.
auto childNodeUnfittable() -> int
{
  installNodeLayout(4, 4, 2);
  const auto& layout = sg::detail::layout();
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 17);
  if (!plan.cpu.empty() || !plan.group.empty() || !plan.node.empty()
      || plan.places != 16)
  {
    std::fprintf(stderr,
                 "the unfittable count did not return the empty plan\n");
    return 1;
  }
  return 0;
}

// T007, FR-009: a single-node host plans every context onto that one
// node's cpus, and the disclosure names the node.
auto childNodeOneNode() -> int
{
  installNodeLayout(2, 4, 2);
  const auto& layout = sg::detail::layout();
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::NODE, layout, 8);
  std::vector<std::uint32_t> cpus;
  for (std::uint32_t cpu = 0; cpu < 8; ++cpu) {
    cpus.push_back(cpu);
  }
  std::vector<std::uint32_t> nodes(8, 0);
  if (plan.places != 8 || plan.cpu != cpus || plan.node != nodes) {
    std::fprintf(stderr, "the one-node plan did not fill the node\n");
    return 1;
  }
  return 0;
}

auto bmNumaSpin(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The two-node shape of the run scenarios: two nodes holding two L3
// groups of four cpus each, so ctx0 reads cpu 0 in group 0 on node 0
// and ctx1 reads cpu 8 in group 2 on node 1.
auto setupTwoNodeRun() -> void
{
  installNodeLayout(4, 4, 2);
  installFakeBinder();
  auto handle = sg::registerBenchmark(&bmNumaSpin, "bmNumaSpin");
  handle.threads(2);
}

// The one-node shape of the run scenarios: one node holding both L3
// groups, so both contexts cycle the one node's cpus.
auto setupOneNodeRun() -> void
{
  installNodeLayout(2, 4, 2);
  installFakeBinder();
  auto handle = sg::registerBenchmark(&bmNumaSpin, "bmNumaSpin");
  handle.threads(2);
}

// The layout with no memory information, for the refusal that stands
// before any context starts.
auto setupNoMemoryRun() -> void
{
  installNodeLayout(2, 4, 0);
  installFakeBinder();
  auto handle = sg::registerBenchmark(&bmNumaSpin, "bmNumaSpin");
  handle.threads(2);
}

// A placement function the user installed: it names a slot as landed
// and answers its own text, the seam a user replaces the pin with.
auto userPlacementText() -> std::string_view
{
  return "USER";
}

auto userPlacement([[maybe_unused]] const sg::PlacementSlot& slot,
                   std::thread::native_handle_type)
    -> sg::PlacementResult
{
  return {.applied = true, .description = userPlacementText()};
}

auto setupTwoNodeBypass() -> void
{
  setupTwoNodeRun();
  sg::setPlacementFunction(&userPlacement);
}

// Fork a child that captures its two streams, installs the seams when
// `setup` stands, runs the entry point with `arguments`, and hands the
// `verifyInChild` hook the state the run left before the exit, the way
// the binder record travels with the child that drove it.
struct Captured
{
  int status;
  std::string out;
  std::string err;
};

auto readFile(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto runCaptured(const char* mode,
                 const std::vector<std::string>& arguments,
                 void (*setup)(),
                 void (*verifyInChild)()) -> Captured
{
  const std::string outPath = std::string("numa-") + mode + "-out.txt";
  const std::string errPath = std::string("numa-") + mode + "-err.txt";

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
    std::vector<std::string> argvStorage {"harness_numa_placement_test"};
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
    if (verifyInChild != nullptr) {
      verifyInChild();
    }
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

// T008, FR-011, D-5: a bound context's field reads `cpu@group@node` in
// the console row and in the JSON context field, and the binder
// received the nodes the plan states, validation first, then the
// context starts (T009, FR-010, R-05).
auto verifyNodeDisclose() -> void
{
  // The validation pass binds in plan order; the two context starts
  // run beside each other, so their binds stand in either order.
  const std::vector<std::uint32_t> first {0, 1, 0, 1};
  const std::vector<std::uint32_t> second {0, 1, 1, 0};
  if (binderCalls() != first && binderCalls() != second) {
    std::fprintf(stderr, "the binder recorded the wrong nodes\n");
    std::exit(1);
  }
  if (binderRestores() != 2) {
    std::fprintf(stderr, "the validation pass restored wrong\n");
    std::exit(1);
  }
}

auto scenarioNodeDisclose() -> int
{
  const Captured run = runCaptured(
      "disclose",
      {"--placement=node", "--json=numa-disclose.json", "--filter",
       "^bmNumaSpin"},
      &setupTwoNodeRun,
      &verifyNodeDisclose);
  check(run.status == 0, "the node-planned run exits 0 (FR-009)");
  check(run.out.find("ctx0=SPIN/0@0@0/applied") != std::string::npos
            && run.out.find("ctx1=SPIN/8@2@1/applied") != std::string::npos,
        "the rows name the node beside the group (FR-011)");
  const std::string document = readFile("numa-disclose.json");
  check(document.find("\"placement\":\"0@0@0\"") != std::string::npos
            && document.find("\"placement\":\"8@2@1\"")
                != std::string::npos,
        "the JSON context field names the node (FR-011, D-5)");
  return 0;
}

// T008, FR-002, R-08: a run whose preset plans no node keeps the
// standing `cpu@group` bytes, and the binder records no call.
auto verifyNoNodePlanNoBind() -> void
{
  if (!binderCalls().empty()) {
    std::fprintf(stderr, "a run with no node plan recorded binds\n");
    std::exit(1);
  }
}

auto scenarioNoNodePresetBytes() -> int
{
  const Captured run = runCaptured("noNodePreset",
                                   {"--placement=l3", "--filter",
                                    "^bmNumaSpin"},
                                   &setupTwoNodeRun,
                                   &verifyNoNodePlanNoBind);
  check(run.status == 0, "the run with the l3 preset exits 0 (FR-002)");
  check(run.out.find("ctx0=SPIN/0@0/applied") != std::string::npos
            && run.out.find("ctx1=SPIN/4@1/applied") != std::string::npos,
        "the fields keep the standing bytes (FR-002, FR-011)");
  check(run.out.find("@0@") == std::string::npos
            && run.out.find("@1@") == std::string::npos,
        "no node tail stands without the preset (FR-002)");
  return 0;
}

// T009, FR-010, R-05: a user-installed PlacementFn cannot bypass the
// bind: the binder receives the planned nodes all the same.
auto verifyBindBypass() -> void
{
  const std::vector<std::uint32_t> first {0, 1, 0, 1};
  const std::vector<std::uint32_t> second {0, 1, 1, 0};
  if (binderCalls() != first && binderCalls() != second) {
    std::fprintf(stderr, "the user function bypassed the bind\n");
    std::exit(1);
  }
}

auto scenarioBindBypass() -> int
{
  const Captured run = runCaptured("bypass",
                                   {"--placement=node", "--filter",
                                    "^bmNumaSpin"},
                                   &setupTwoNodeBypass,
                                   &verifyBindBypass);
  check(run.status == 0, "the run with a user placement function exits 0");
  check(run.out.find("ctx0=SPIN/USER/applied") != std::string::npos,
        "the user function's text stands (FR-010, R-05)");
  return 0;
}

// T009, FR-010: the one-node host plans every context onto the one
// node, and the disclosure names it.
auto verifyOneNodeBinds() -> void
{
  const std::vector<std::uint32_t> expected {0, 0, 0, 0};
  if (binderCalls() != expected) {
    std::fprintf(stderr, "the one-node binder recorded the wrong nodes\n");
    std::exit(1);
  }
  if (binderRestores() != 2) {
    std::fprintf(stderr, "the validation pass restored wrong\n");
    std::exit(1);
  }
}

auto scenarioOneNode() -> int
{
  const Captured run = runCaptured("oneNode",
                                   {"--placement=node", "--filter",
                                    "^bmNumaSpin"},
                                   &setupOneNodeRun,
                                   &verifyOneNodeBinds);
  check(run.status == 0, "the one-node run exits 0 (FR-009)");
  check(run.out.find("ctx0=SPIN/0@0@0/applied") != std::string::npos
            && run.out.find("ctx1=SPIN/1@0@0/applied") != std::string::npos,
        "the one-node rows name the one node (FR-011)");
  return 0;
}

// T016, FR-013, R-07: a denial in the eager validation pass refuses
// one line naming the context and the node, exit status 2, ahead of
// every context.
auto scenarioRefuseValidation() -> int
{
  auto denyNodeOne = []() -> void
  {
    setupTwoNodeRun();
    binderDenyNodeFrom() = 1;
  };
  const Captured run = runCaptured("refuseValidation",
                                   {"--placement=node", "--filter",
                                    "^bmNumaSpin"},
                                   denyNodeOne,
                                   nullptr);
  check(run.status == 2, "the denied validation bind exits 2 (FR-013)");
  check(countLines(run.err) == 1, "the refusal prints one line (FR-013)");
  check(run.err.find("ctx1") != std::string::npos
            && run.err.find("node 1") != std::string::npos,
        "the line names the context and the node (FR-013)");
  check(run.out.find("ctx0=") == std::string::npos
            && run.out.find("bmNumaSpin") == std::string::npos,
        "no benchmark output stands ahead of the refusal (SC-003)");
  return 0;
}

// T016, FR-013, R-07: a bind that fails at a context start stands the
// same refusal; the fake denies every start bind and allows the
// validation pass.
auto scenarioRefuseStart() -> int
{
  auto denyStartBinds = []() -> void
  {
    setupTwoNodeRun();
    binderDenyCallFrom() = 2;
  };
  const Captured run = runCaptured("refuseStart",
                                   {"--placement=node", "--filter",
                                    "^bmNumaSpin"},
                                   denyStartBinds,
                                   nullptr);
  check(run.status == 2, "the denied start bind exits 2 (FR-013)");
  check(countLines(run.err) == 1, "the refusal prints one line (FR-013)");
  check(run.err.find("ctx0") != std::string::npos
            && run.err.find("node 0") != std::string::npos,
        "the line names the context and the node (FR-013)");
  check(run.out.find("ctx0=SPIN") == std::string::npos,
        "no benchmark output stands ahead of the refusal (SC-003)");
  return 0;
}

// T016, FR-014, R-10: a node preset over a read with no memory
// information refuses one line before any context starts.
auto scenarioRefuseNoMemory() -> int
{
  const Captured run = runCaptured("refuseNoMemory",
                                   {"--placement=node", "--filter",
                                    "^bmNumaSpin"},
                                   &setupNoMemoryRun,
                                   nullptr);
  check(run.status == 2, "the node preset over no memory information "
        "exits 2 (FR-014)");
  check(countLines(run.err) == 1, "the refusal prints one line (FR-014)");
  check(run.err.find("node") != std::string::npos,
        "the line names the preset (FR-014)");
  check(run.out.find("ctx0=") == std::string::npos,
        "no context started (FR-014)");
  return 0;
}

// T016, FR-013, D-2: the standing unfittable-count refusal covers the
// node plan.
auto setupSeventeenThreads() -> void
{
  installNodeLayout(4, 4, 2);
  installFakeBinder();
  auto handle = sg::registerBenchmark(&bmNumaSpin, "bmNumaSpin");
  handle.threads(17);
}

auto scenarioRefuseUnfittable() -> int
{
  const Captured run = runCaptured("refuseUnfittable",
                                   {"--placement=node", "--filter",
                                    "^bmNumaSpin"},
                                   &setupSeventeenThreads,
                                   nullptr);
  check(run.status == 2, "the unfittable count exits 2 (FR-013)");
  check(countLines(run.err) == 1, "the stop prints one line (FR-013)");
  check(run.err.find("node") != std::string::npos
            && run.err.find("16") != std::string::npos
            && run.err.find("17") != std::string::npos,
        "the line names the preset, the places and the ask (FR-013)");
  check(run.out.find("ctx0=") == std::string::npos,
        "no context started (FR-013)");
  return 0;
}

auto main(int argc, char** argv) -> int
{
  struct Scenario
  {
    const char* name;
    int (*run)();
  };

  const std::array<Scenario, 12> children {
      Scenario {"walkOneNode", &childWalkOneNode},
      Scenario {"walkTwoNodes", &childWalkTwoNodes},
      Scenario {"walkFourNodes", &childWalkFourNodes},
      Scenario {"walkNoMemory", &childWalkNoMemory},
      Scenario {"binderRecords", &childBinderRecords},
      Scenario {"binderDenies", &childBinderDenies},
      Scenario {"binderClear", &childBinderClear},
      Scenario {"nodeCycles", &childNodeCycles},
      Scenario {"nodeFills", &childNodeFills},
      Scenario {"nodeSkipsExcluded", &childNodeSkipsExcluded},
      Scenario {"nodeUnfittable", &childNodeUnfittable},
      Scenario {"nodeOneNode", &childNodeOneNode}};

  if (argc > 1) {
    for (const auto& scenario : children) {
      if (std::string_view(argv[1]) == scenario.name) {
        return scenario.run();
      }
    }
    fail("unknown scenario mode");
  }

  for (const auto& scenario : children) {
    check(runChildScenario(argv[0], scenario.name) == 0, scenario.name);
  }

  // The run scenarios fork one child each: this process installs no
  // seam of its own, so every child installs the layout and the binder
  // it reads, the way the CLI placement tests capture their children.
  const std::array<Scenario, 8> captured {
      Scenario {"nodeDisclose", &scenarioNodeDisclose},
      Scenario {"noNodePresetBytes", &scenarioNoNodePresetBytes},
      Scenario {"bindBypass", &scenarioBindBypass},
      Scenario {"oneNode", &scenarioOneNode},
      Scenario {"refuseValidation", &scenarioRefuseValidation},
      Scenario {"refuseStart", &scenarioRefuseStart},
      Scenario {"refuseNoMemory", &scenarioRefuseNoMemory},
      Scenario {"refuseUnfittable", &scenarioRefuseUnfittable}};
  for (const auto& scenario : captured) {
    check(scenario.run() == 0, scenario.name);
  }

  std::printf("harness_numa_placement_test: all scenarios passed\n");
  return 0;
}
