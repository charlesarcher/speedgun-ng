// ============================================================================
// T008: the option grammar of contracts/cli-options.md, driven directly.
//
// Every value's rejection stands one line on stderr with the probe
// option's prefix and exit status 2, before any benchmark runs
// (FR-008, FR-009, FR-011, FR-014, FR-038, R-06); the standing
// getopt_long paths keep their shapes (contracts/cli-options.md); and
// the option-value uniqueness test scans the parser's own table through
// the source path the registration hands the binary (FR-011, R-06), the
// way the schema tests read their schema path. The mode-machine
// scenario reads the JSON document, so it stands red against the writer
// T013 lands, and runs after the green scenarios. Frameworkless
// check()/fail() convention.
//
// T018 appends the placement status-2 cases of US2 (FR-017 to FR-019,
// D-3 to D-5): the idle stop, the alongside overlap stop for a planned
// cpu and for an SMT sibling, the precheck allowance, the `both` stop
// after the pre-check phase, and the emptied-mask stop. A rejected run
// may return 2 from inside the entry point while probe threads stand
// live, so every scenario of this group runs in a fork of this binary
// that captures its two streams, the pattern
// harness_cli_placement_test.cpp carries; the synthetic layouts install
// in the child before the entry point reads them.
//
// The coverage wave appends the refusal arms the first rounds left
// dark: the parse rejections of the value grammar's remaining tokens
// (the empty and overflowing numbers, the trailing text, the cpu
// ranges, the cpu outside the mask, the bare extension, the valued
// strict and the unknown key), the unreadable-mask refusal, which one
// seccomp filter drives at the syscall boundary, the multi-context
// preset-none narrowing, the two skipped L3 groups, the mid-batch fit
// beside the probe, and the unavailable answers, which run in a fresh
// exec of this binary whose fake publishes no tsc leaf (FR-037, E-07).
// ============================================================================

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

#ifndef SG_OPTIONS_SOURCE
#  error "the registration passes the option table's source path (R-06)"
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER CLI TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// A trivial benchmark: an accepted value proceeds into the batch, so
// the acceptance scenarios need one registered function.
auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The scripted provider: the leaves the runner resolves its plans
// through, plus the machine/tsc leaf the probe's availability check
// reads (R-02, Q-1). The tsc series steps by one tick, so the probe
// records no gap and the batch stays quiet.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
    ref.setPoints("machine", "monotonic", {0}, 4000);
    ref.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    ref.setPoints("machine", "thread_cpu", {0}, 2000);
    ref.addCounter("machine",
                   "tsc",
                   "none",
                   "raw tick counter",
                   sg::counters::Availability::COUNTABLE,
                   sg::counters::ReadMode::FAST_TSC);
    ref.setPoints("machine", "tsc", {0}, 1);
    const auto registered =
        sg::counters::System::local().registerProvider(std::move(owned));
    check(registered.has_value(), "the scripted provider registers (R-02)");
    return &ref;
  }();
  return *provider;
}

// The cpu indexes the scenarios pin probe threads to, each standing
// in the process affinity mask. Membership in the mask is the rule
// the pins obey; the position inside it names nothing (FR-040).
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask)) {
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (FR-040)");
  return cpus;
}

// The first cpu index the process affinity mask does not hold, below
// the set's own bound: the token the outside-the-mask refusal reads
// (FR-011).
auto pickCpuOutsideMask() -> std::string
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE); ++cpu)
  {
    if (CPU_ISSET(cpu, &mask) == 0) {
      return std::to_string(cpu);
    }
  }
  fail("the mask leaves no cpu index free (FR-040)");
  return {};
}

// The layout cpus nothing pins: the lowest indexes the process
// affinity mask does not hold, each a layout-only cpu that no
// benchmark or probe pins, no value parses, and no mask check reads
// (VI). A filler needs no real cpu, so the shapes build on any mask, a
// two-cpu host included, and the count the threads report stands
// unread (VI).
auto pickFillers(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  std::vector<std::uint32_t> fillers;
  for (std::size_t index = 0; fillers.size() < count; ++index) {
    if (CPU_ISSET(index, &mask) != 0) {
      continue;
    }
    fillers.push_back(static_cast<std::uint32_t>(index));
  }
  return fillers;
}

// The syscall refusal seam: one seccomp filter that answers the named
// syscall number with EPERM for the rest of the child's life. The
// filter is the injection route the unreadable-mask refusal of the
// parse and the skipped narrowing read carry (FR-011, D-5), the way
// the load-failure seam stands in for the load call's refusal.
auto blockSyscallWithEperm(const int number) -> void
{
  const struct sock_filter program[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<__u32>(number), 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
  };
  const struct sock_fprog programArg = {
      .len = static_cast<unsigned short>(sizeof(program) / sizeof(program[0])),
      .filter = const_cast<struct sock_filter*>(program),
  };
  check(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0,
        "the child accepts its no-new-privs state (FR-011)");
  check(syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &programArg) == 0,
        "the child installs its refusal filter (FR-011)");
}

// One speedgunMain run with stdout captured and the status checked
// (FR-036); the acceptance scenarios read the console text.
auto captureRun(const std::vector<std::string>& arguments,
                const int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_cli_out_" + std::to_string(++runIndex) + ".txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
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
  std::remove(path.c_str());
  return buffer.str();
}

// One speedgunMain run with stderr captured: the option's rejections
// print their one line there (FR-038, R-06).
auto captureError(const std::vector<std::string>& arguments,
                  const int expected) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_cli_err_" + std::to_string(++runIndex) + ".txt";
  const int saved = dup(STDERR_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stderr) == nullptr) {
    fail("cannot redirect the failure note");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stderr);
  if (dup2(saved, STDERR_FILENO) < 0) {
    fail("cannot restore stderr");
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
  std::remove(path.c_str());
  return buffer.str();
}

// The one-line rule: a rejection prints exactly one line on stderr and
// names the option (FR-011, FR-038, R-06).
auto checkOneLine(const std::string& note, const char* what) -> void
{
  check(note.rfind("--jitter-probe: ", 0) == 0, what);
  check(std::count(note.begin(), note.end(), '\n') == 1, what);
}

auto readDocument(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The comma-joined list of every cpu the process affinity mask holds,
// read from sched_getaffinity (VI); the mask-emptied scenario probes
// every one of them.
auto allMaskCpus() -> std::string
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  std::string joined;
  for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE); ++cpu)
  {
    if (CPU_ISSET(cpu, &mask) != 0) {
      if (!joined.empty()) {
        joined.push_back(',');
      }
      joined += std::to_string(cpu);
    }
  }
  check(!joined.empty(), "the affinity mask holds cpus (FR-040)");
  return joined;
}

// The overlap shape: two L3 groups of one core each, the picked pair
// standing as the two groups' lowest cpus, so the `l3` plan of two
// contexts pins exactly the two cpus pickCpus returned, and the core
// mates the FR-018 line names stand on the fillers nothing pins (VI).
auto installAcceptanceShape(std::uint32_t base, std::uint32_t sibling) -> void
{
  const auto fillers = pickFillers(2);
  std::vector<std::pair<std::uint32_t, std::uint32_t>> placed;
  placed.emplace_back(base, 0);
  placed.emplace_back(fillers.at(0), 0);
  placed.emplace_back(sibling, 1);
  placed.emplace_back(fillers.at(1), 1);
  std::ranges::sort(placed);
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 2; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = core;
    installed.cores.push_back(layoutCore);
  }
  for (const auto& [cpu, core] : placed) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = core;
    thread.groupOrdinal = core;
    installed.cores.at(core).threads.push_back(
        static_cast<std::uint32_t>(installed.threads.size()));
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  // The l3 preset pins each group's lowestCpu, so the groups stand
  // lowest at the picked cpus, the ones the scenarios pin and the
  // probe names, on any mask (FR-040, VI).
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group == 0 ? base : sibling;
    layoutGroup.cores.push_back(group);
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// The cpuset-limit shape: group 0 carries two cores with the picked
// `base` lowest, group 1 one core with the picked `sibling` lowest, so
// the `l3` plan of two contexts pins exactly the two cpus the child's
// cpuset holds; group 0's second core stands planned by nothing, so
// the free cpu the idle resolution yields stands in the touched group
// 0, outside the cpuset the disclosure names (FR-036). The fillers
// stand the layout-only cpus pickFillers offers outside the mask (VI).
auto installCpusetPairShape(std::uint32_t base, std::uint32_t sibling) -> void
{
  const auto fillers = pickFillers(2);
  std::vector<std::pair<std::uint32_t, std::uint32_t>> placed;
  placed.emplace_back(base, 0);
  placed.emplace_back(fillers.at(0), 0);
  placed.emplace_back(fillers.at(1), 1);
  placed.emplace_back(sibling, 2);
  std::ranges::sort(placed);
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  // The group each core stands in: group 0 carries the two cores the
  // l3 walk's first places and the idle resolution's free cpu come
  // from, group 1 carries the one core the picked sibling lowest
  // names, so the group list indexes cores the shape built (VI).
  const std::array<std::uint32_t, 3> coreGroup {{0, 0, 1}};
  for (std::uint32_t core = 0; core < 3; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = coreGroup.at(core);
    installed.cores.push_back(layoutCore);
  }
  for (const auto& [cpu, core] : placed) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = core;
    thread.groupOrdinal = coreGroup.at(core);
    installed.cores.at(core).threads.push_back(
        static_cast<std::uint32_t>(installed.threads.size()));
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  // The l3 preset pins each group's lowestCpu, so the groups stand
  // lowest at the picked pair, the cpus the child's cpuset holds, on
  // any mask (FR-036, FR-040, VI).
  const std::array<std::uint32_t, 2> groupLowest {{base, sibling}};
  const std::array<std::uint32_t, 2> groupCoreCounts {{2, 1}};
  std::uint32_t coreCursor = 0;
  for (std::uint32_t group = 0; group < 2; ++group) {
    const auto count = groupCoreCounts.at(group);
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = groupLowest.at(group);
    for (std::uint32_t offset = 0; offset < count; ++offset) {
      const auto core = coreCursor + offset;
      layoutGroup.cores.push_back(core);
    }
    coreCursor += count;
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// The two cpus the layout scenarios build their shapes from, picked
// from the process affinity mask (VI): the planned cpu `baseCpu` and
// its SMT sibling `siblingCpu`, the second of the two the mask holds;
// the two file-scope definitions carry them into the setups the
// forked children run.
std::uint32_t baseCpu = 0;
std::uint32_t siblingCpu = 0;

// The probe cpus of the mask-emptied scenario: every cpu the process
// mask holds (FR-019).
std::string allCpus;

auto bmJitIdleNoFree(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitOverlap(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitMaskEmpty(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitSingle(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The pinned planner of the cpuset-limit scenario: two contexts on
// the `l3` plan give the idle resolution its touched groups, and the
// free cpus the resolution leaves stand outside the cpuset the child
// holds, the cpuset limit the disclosure names (FR-036).
auto bmJitLimitPlanned(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitLimitProbe(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitLimitFree(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitSkip(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto setupNoLayout() -> void {}

auto setupIdleNoFree() -> void
{
  installAcceptanceShape(baseCpu, siblingCpu);
  auto handle = sg::registerBenchmark(&bmJitIdleNoFree, "bmJitIdleNoFree");
  handle.threads(2);
}

auto setupOverlap() -> void
{
  installAcceptanceShape(baseCpu, siblingCpu);
  auto handle = sg::registerBenchmark(&bmJitOverlap, "bmJitOverlap");
  handle.threads(2);
}

auto setupMaskEmpty() -> void
{
  (void)sg::registerBenchmark(&bmJitMaskEmpty, "bmJitMaskEmpty");
}

// The unreadable-mask refusal of the parse: the child blocks the
// affinity read before the entry point reads the value (FR-011).
auto setupSeccompGetaffinity() -> void
{
  blockSyscallWithEperm(SYS_sched_getaffinity);
}

// The single-cpu affinity child: the child holds the picked base cpu
// alone in its process affinity mask, the way a host whose cpuset
// holds fewer cpus than the online count stands at the run, so a cpu
// the host mask held stands outside the child's (FR-011).
auto setupOutsideMask() -> void
{
  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  CPU_SET(baseCpu, &cpus);
  if (sched_setaffinity(0, sizeof(cpus), &cpus) != 0) {
    fail("the child cannot hold its single-cpu mask (FR-011)");
  }
}

auto setupSingle() -> void
{
  auto handle = sg::registerBenchmark(&bmJitSingle, "bmJitSingle");
  handle.placement("l3");
}

auto setupCpusetLimit() -> void
{
  // The child's cpuset holds the two picked cpus, the way a cpuset
  // tighter than the layout stands at the run (FR-036); the shape the
  // child installs plans exactly the picked pair, so the pinned
  // benchmark names only cpus the cpuset holds, and the free cpu the
  // idle resolution yields stands outside it (FR-036).
  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  CPU_SET(baseCpu, &cpus);
  CPU_SET(siblingCpu, &cpus);
  if (sched_setaffinity(0, sizeof(cpus), &cpus) != 0) {
    fail("the child cannot hold the picked cpuset (FR-036)");
  }
  installCpusetPairShape(baseCpu, siblingCpu);
  auto handle = sg::registerBenchmark(&bmJitLimitPlanned, "bmJitLimitPlanned");
  handle.placement("l3");
  handle.threads(2);
  (void)sg::registerBenchmark(&bmJitLimitProbe, "bmJitLimitProbe");
  // A second unpinned instance: the first unpinned instance's
  // narrowing latches the cpuset answer, and the second's takes the
  // latched short-circuit (FR-036).
  (void)sg::registerBenchmark(&bmJitLimitFree, "bmJitLimitFree");
}

// The skipped-group shape: one L3 group of one core holding the
// picked pair, so the `compact` plan of two contexts covers the
// touched group whole, the idle resolution stands the skip record
// with no free cpu, and the status-2 stop names the condition
// (FR-017, D-4). Two pins, both picked (VI).
auto installSkipShape(std::uint32_t base, std::uint32_t sibling) -> void
{
  std::vector<std::pair<std::uint32_t, std::uint32_t>> placed;
  placed.emplace_back(base, 0);
  placed.emplace_back(sibling, 0);
  std::ranges::sort(placed);
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  sg::detail::LayoutCore layoutCore;
  layoutCore.groupOrdinal = 0;
  installed.cores.push_back(layoutCore);
  for (const auto& [cpu, core] : placed) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = core;
    thread.groupOrdinal = 0;
    installed.cores.at(core).threads.push_back(
        static_cast<std::uint32_t>(installed.threads.size()));
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  sg::detail::LayoutGroup layoutGroup;
  layoutGroup.lowestCpu = base;
  layoutGroup.cores = {0U};
  installed.groups.push_back(layoutGroup);
  sg::detail::installTestLayout(installed);
}

auto setupSkip() -> void
{
  installSkipShape(baseCpu, siblingCpu);
  auto handle = sg::registerBenchmark(&bmJitSkip, "bmJitSkip");
  handle.placement("compact");
  handle.threads(2);
}

auto bmFit(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  for (auto _ : state) {
    state.setIterationTime(static_cast<std::int64_t>(100.0 * size));
  }
}

// Two families of planted manual times: the batch carries a family
// boundary, so the first family's fit prints between the rows of the
// two families, behind the join edge the probe owns (FR-022, R-08).
auto setupFitFamilies() -> void
{
  auto first = sg::registerBenchmark(&bmFit, "bmFitA");
  first.useManualTime().arg(2).arg(4).complexity(sg::BigO::ONE);
  auto second = sg::registerBenchmark(&bmFit, "bmFitB");
  second.useManualTime().arg(2).arg(4).complexity(sg::BigO::ONE);
}

// The exit status and both captures of one forked scenario child.
struct Captured
{
  int status;
  std::string out;
  std::string err;
};

// One speedgunMain run in a forked child with both streams captured:
// a stop may return 2 from inside the entry point, so the work stands
// in a child either way (the harness_cli_placement_test pattern).
auto runChildScenario(const char* mode,
                      const std::vector<std::string>& arguments,
                      void (*setup)()) -> Captured
{
  const std::string outPath =
      std::string("harness_jitter_cli_") + mode + "-out.txt";
  const std::string errPath =
      std::string("harness_jitter_cli_") + mode + "-err.txt";

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
    setup();
    std::vector<std::string> argvStorage {"harness_jitter_cli_test"};
    for (const auto& argument : arguments) {
      argvStorage.push_back(argument);
    }
    std::vector<char*> argv;
    for (auto& argument : argvStorage) {
      argv.push_back(argument.data());
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    std::fflush(stdout);
    std::fflush(stderr);
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
      WEXITSTATUS(waitStatus), readDocument(outPath), readDocument(errPath)};
}

// One speedgunMain run in a fresh exec of this binary: the
// availability scenarios need a provider catalog of their own, and the
// process-wide system holds the scripted one already, so the scenario
// re-enters main with its mode and redirects both streams itself (the
// harness_context_barrier_test pattern).
auto runExecChild(const char* mode, const char* tag) -> Captured
{
  const std::string outPath =
      std::string("harness_jitter_cli_") + tag + "-out.txt";
  const std::string errPath =
      std::string("harness_jitter_cli_") + tag + "-err.txt";
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the exec child");
  }
  if (pid == 0) {
    char* const execArgv[] = {const_cast<char*>("harness_jitter_cli_test"),
                              const_cast<char*>(mode),
                              nullptr};
    (void)execv("/proc/self/exe", execArgv);
    _exit(3);
  }
  int waitStatus = 0;
  if (waitpid(pid, &waitStatus, 0) < 0) {
    fail("cannot wait for the exec child");
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the exec child did not exit normally");
  }
  return Captured {
      WEXITSTATUS(waitStatus), readDocument(outPath), readDocument(errPath)};
}

// The fresh process of the suite-call precedence scenario: the
// suite-level call requires a process that has run no benchmark, so
// the scenario re-enters this binary, binds the call before its entry
// point reads the option file, and runs with the file's value standing
// (E-02, FR-015).
auto childFileOverSuite() -> int
{
  scriptedProvider();
  const std::string probe = std::to_string(pickCpus(1).front());
  {
    std::ofstream file("harness_jitter_cli_file2.txt");
    file << "jitter-probe=" << probe << "\n";
  }
  auto suiteOptions = sg::JitterProbeOptions {};
  suiteOptions.mode = sg::JitterProbeMode::PRECHECK;
  sg::runJitterProbe(suiteOptions);
  std::vector<std::string> arguments {"--filter=bmQuiet",
                                      "--repetitions=1",
                                      "--iterations=2",
                                      "--options=harness_jitter_cli_file2.txt",
                                      "--json=harness_jitter_cli_file2.json"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The fresh process of the availability scenarios: the fake it
// registers publishes the leaves the runner resolves its plans
// through and no tsc leaf, so the catalog the availability check
// reads carries no qualifying entry (Q-1, R-02).
auto childUnavailable(const bool strict) -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fresh process registers its fake (R-02)");
  const std::string probe = std::to_string(pickCpus(1).front());
  std::vector<std::string> arguments {
      "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};
  if (strict) {
    arguments.push_back("--jitter-probe=" + probe + ",strict");
  } else {
    arguments.push_back("--json=harness_jitter_cli_unavailable.json");
    arguments.push_back("--jitter-probe=" + probe);
  }
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  return status;
}

// The fresh process of the blocked-count scenario: the fake publishes
// the tsc leaf at the fast mode with a count the catalog marks
// blocked, so the leaf stands in the tree the runner's window stamps
// read and the plan the stamp compiles refuses on the availability
// gate (FR-024), the run disclosing the unavailable answer beside it.
auto childUnavailableBlocked() -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  provider.addCounter("machine",
                      "tsc",
                      "none",
                      "blocked tsc leaf",
                      sg::counters::Availability::PERMISSION_BLOCKED,
                      sg::counters::ReadMode::FAST_TSC);
  provider.setPoints("machine", "tsc", {0}, 1);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fresh process registers its fake (R-02)");
  const std::string probe = std::to_string(pickCpus(1).front());
  std::vector<std::string> arguments {"--filter=bmQuiet",
                                      "--repetitions=1",
                                      "--iterations=2",
                                      "--jitter-probe=" + probe};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  return status;
}

// The fresh process of the threaded unavailable scenario: the same
// tsc-less catalog, and the batch's instance carries two contexts, so
// the threaded body's window edges stamp against the leaf the catalog
// lacks and stand unrecorded while the batch continues (FR-037, E-06).
auto childUnavailableThreaded() -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fresh process registers its fake (R-02)");
  auto handle = sg::registerBenchmark(&bmQuiet, "bmThreaded");
  handle.threads(2);
  const std::string probe = std::to_string(pickCpus(1).front());
  std::vector<std::string> arguments {"--filter=bmThreaded",
                                      "--repetitions=1",
                                      "--iterations=2",
                                      "--jitter-probe=" + probe};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  return status;
}

// FR-011, FR-017: the idle value parses, and the resolution over a run
// whose instances carry no plan names no free cpu: one line naming the
// preset word and status 2. The child installs no layout, so the
// resolution answers the host read its own first read caches. The
// value names no precheck key, so the parse assigns the 200 ms default
// the option table carries; the stop precedes any phase, so the
// default stands a parsed value and no run (FR-014).
auto scenarioIdleNoPlans() -> int
{
  const Captured run = runChildScenario("idleNoPlans",
                                        {"--filter=bmQuiet",
                                         "--repetitions=1",
                                         "--iterations=2",
                                         "--jitter-probe=idle"},
                                        &setupNoLayout);
  check(run.status == 2, "a plan-free idle run stops (FR-017)");
  checkOneLine(run.err, "the idle stop prints one line (FR-017)");
  check(run.err.find("idle") != std::string::npos,
        "the stop names the preset word (FR-017)");
  check(run.out.find("bmQuiet") == std::string::npos,
        "no benchmark ran (FR-017)");
  // FR-017, FR-006: the precheck mode with no pinned instance plans no
  // cpu at all. The stop names that cause; the free-cpu absence stands
  // a different line (FR-017).
  const Captured precheckRun =
      runChildScenario("idleNoPlansPrecheck",
                       {"--filter=bmQuiet",
                        "--repetitions=1",
                        "--iterations=2",
                        "--jitter-probe=idle,mode=precheck"},
                       &setupNoLayout);
  check(precheckRun.status == 2,
        "a plan-free precheck idle run stops (FR-017)");
  checkOneLine(precheckRun.err, "the idle stop prints one line (FR-017)");
  check(precheckRun.err.find("no cpu is planned") != std::string::npos,
        "the stop names the absent plan (FR-017)");
  return 0;
}

// FR-017, FR-038: `idle` over a run whose plans leave no cpu free
// prints one line naming the preset word and exits 2 before any
// benchmark runs.
auto scenarioIdleNoFree() -> int
{
  const Captured run = runChildScenario(
      "idleNoFree",
      {"--placement=l3", "--filter", "^bmJitIdleNoFree", "--jitter-probe=idle"},
      &setupIdleNoFree);
  check(run.status == 2, "no free cpu under idle exits 2 (FR-017)");
  checkOneLine(run.err, "the idle stop prints one line (FR-017, FR-038)");
  check(run.err.find("idle") != std::string::npos,
        "the idle stop names the preset word (FR-017)");
  check(run.out.find("bmJitIdleNoFree") == std::string::npos,
        "no benchmark ran (FR-017)");
  return 0;
}

// FR-018, D-3: a probe cpu that is a planned cpu of the `l3` plan
// prints one line naming the cpu and the benchmark and exits 2 before
// any benchmark runs.
auto scenarioOverlapPlanned() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run = runChildScenario("overlapPlanned",
                                        {"--placement=l3",
                                         "--filter",
                                         "^bmJitOverlap",
                                         "--jitter-probe=" + probe},
                                        &setupOverlap);
  check(run.status == 2, "a planned probe cpu exits 2 (FR-018)");
  checkOneLine(run.err, "the overlap stop prints one line (FR-018)");
  check(run.err.find(probe) != std::string::npos
            && run.err.find("bmJitOverlap") != std::string::npos,
        "the line names the cpu and the benchmark (FR-018)");
  check(run.out.find("bmJitOverlap") == std::string::npos,
        "no benchmark ran (FR-018)");
  return 0;
}

// FR-018, D-3, D-9: the SMT sibling of a planned cpu is refused the
// same way. The sibling stands on the second of the two cpus the mask
// gives, the core mate the shape builds for the planned cpu `baseCpu`,
// so the parse accepts the token and the check reads the refusal off
// the core thread lists (FR-018, D-9).
auto scenarioOverlapSibling() -> int
{
  const std::string sibling = std::to_string(siblingCpu);
  const Captured run = runChildScenario("overlapSibling",
                                        {"--placement=l3",
                                         "--filter",
                                         "^bmJitOverlap",
                                         "--jitter-probe=" + sibling},
                                        &setupOverlap);
  check(run.status == 2, "an SMT sibling of a planned cpu exits 2 (FR-018)");
  checkOneLine(run.err, "the sibling stop prints one line (FR-018)");
  check(run.err.find(sibling) != std::string::npos
            && run.err.find("bmJitOverlap") != std::string::npos,
        "the sibling line names the cpu and the benchmark (FR-018)");
  check(run.out.find("bmJitOverlap") == std::string::npos,
        "no benchmark ran (FR-018)");
  return 0;
}

// D-3, R-10: the same overlap under `precheck` runs, because the probe
// stops before the benchmarks start.
auto scenarioPrecheckAllows() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run = runChildScenario(
      "precheckAllows",
      {"--placement=l3",
       "--filter",
       "^bmJitOverlap",
       "--repetitions=1",
       "--iterations=2",
       "--jitter-probe=" + probe + ",mode=precheck,precheck=1"},
      &setupOverlap);
  check(run.status == 0, "the precheck mode allows the overlap (D-3, R-10)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("bmJitOverlap") != std::string::npos,
        "the benchmark ran under the precheck allowance (D-3)");
  check(run.out.find("affinity mask narrowed") == std::string::npos,
        "the precheck run narrows no mask (FR-006, R-10)");
  return 0;
}

// FR-006, R-10, FR-019: a precheck run whose probe cpus cover the
// whole mask exits 0, because no probe thread runs beside a benchmark
// and no narrowing stands between them (D-5).
auto scenarioPrecheckWholeMask() -> int
{
  const Captured run = runChildScenario(
      "precheckWholeMask",
      {"--filter",
       "^bmJitSingle$",
       "--repetitions=1",
       "--iterations=2",
       "--jitter-probe=" + allCpus + ",mode=precheck,precheck=1"},
      &setupSingle);
  check(run.status == 0,
        "the precheck run over the whole mask continues (FR-006, R-10)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("affinity mask narrowed") == std::string::npos,
        "the precheck run prints no narrowing line (D-5)");
  check(run.out.find("bmJitSingle") != std::string::npos,
        "the benchmark ran beside the precheck probe (R-10)");
  return 0;
}

// D-3, R-10: under `both` the pre-check phase uses the cpu and the
// alongside stop lands with the overlap line before any row.
auto scenarioBothOverlap() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run =
      runChildScenario("bothOverlap",
                       {"--placement=l3",
                        "--filter",
                        "^bmJitOverlap",
                        "--jitter-probe=" + probe + ",mode=both,precheck=1"},
                       &setupOverlap);
  check(run.status == 2, "the alongside phase of `both` refuses (FR-018)");
  checkOneLine(run.err, "the both-mode stop prints one line (FR-018)");
  check(run.err.find(probe) != std::string::npos
            && run.err.find("bmJitOverlap") != std::string::npos,
        "the both-mode line names the cpu and the benchmark (FR-018)");
  check(run.out.find("bmJitOverlap") == std::string::npos,
        "no row printed (FR-018)");
  return 0;
}

// FR-019, D-5: an unpinned benchmark whose affinity mask the probe
// cpus empty prints one line naming the benchmark and exits 2.
auto scenarioMaskEmpty() -> int
{
  const Captured run = runChildScenario(
      "maskEmpty",
      {"--filter", "^bmJitMaskEmpty$", "--jitter-probe=" + allCpus},
      &setupMaskEmpty);
  check(run.status == 2, "the emptied mask exits 2 (FR-019)");
  checkOneLine(run.err, "the mask stop prints one line (FR-019)");
  check(run.err.find("bmJitMaskEmpty") != std::string::npos,
        "the mask stop names the benchmark (FR-019)");
  check(run.out.find("bmJitMaskEmpty") == std::string::npos,
        "no benchmark ran (FR-019)");
  return 0;
}

// FR-019, D-5: a single-context instance that carries a preset is
// unpinned (the runner pins only instances with more than one
// context), so the probe narrows the main thread's mask and the run
// continues.
auto scenarioSingleContext() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run =
      runChildScenario("singleContext",
                       {"--filter",
                        "^bmJitSingle$",
                        "--repetitions=1",
                        "--iterations=2",
                        "--jitter-probe=" + probe + ",precheck=1"},
                       &setupSingle);
  check(run.status == 0, "the single-context preset run continues (FR-019)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("affinity mask narrowed for 'bmJitSingle'")
            != std::string::npos,
        "the disclosure names the narrowed mask (FR-019, D-5)");
  check(run.out.find("bmJitSingle") != std::string::npos,
        "the benchmark ran (FR-019)");
  return 0;
}

// FR-017, D-4: the touched group whose planned cpus and their
// siblings cover it whole stands in the skip record, the resolution
// names no free cpu, and the status-2 stop names the condition; the
// continuing run over a skipped group stands the cpuset-limit
// scenario's own proof (FR-017, D-4).
auto scenarioSkipDisclosure() -> int
{
  const Captured run = runChildScenario("skipDisclosure",
                                        {"--placement=compact",
                                         "--filter",
                                         "^bmJitSkip",
                                         "--repetitions=1",
                                         "--iterations=2",
                                         "--jitter-probe=idle"},
                                        &setupSkip);
  check(run.status == 2,
        "the touched group with no free cpu stops the run (FR-017)");
  checkOneLine(run.err, "the idle stop prints one line (FR-017)");
  check(run.err.find("the idle preset names no free cpu") != std::string::npos,
        "the stop names the empty resolution (FR-017, D-4)");
  check(run.out.find("bmJitSkip") == std::string::npos,
        "no benchmark ran (FR-017)");
  return 0;
}

// FR-036, D-12: cpusets limiting the probe cpus reach the disclosure
// and the run continues; the child's cpuset holds the two picked
// cpus, the `idle` resolution names free cpus of the shape outside
// it, and the unpinned second instance's narrowing discloses the
// limit.
auto scenarioCpusetLimit() -> int
{
  const Captured run = runChildScenario("cpusetLimit",
                                        {"--filter",
                                         "^bmJitLimit",
                                         "--repetitions=1",
                                         "--iterations=2",
                                         "--jitter-probe=idle,precheck=1"},
                                        &setupCpusetLimit);
  check(run.status == 0, "the cpuset limit discloses and continues (FR-036)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("cpusets limit the probe cpus") != std::string::npos,
        "the disclosure names the cpuset limit (FR-036, D-12)");
  check(run.out.find("bmJitLimitProbe") != std::string::npos,
        "the second benchmark ran (FR-036)");
  return 0;
}

// FR-011, D-5: the parse's unreadable-mask refusal answers one line
// and status 2, and the narrowing that skips its own read leaves the
// run over the full mask. The refusal filter stands in the child
// before the entry point reads the value (FR-011).
auto scenarioUnreadableMask() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run = runChildScenario("unreadableMask",
                                        {"--filter=bmQuiet",
                                         "--repetitions=1",
                                         "--iterations=2",
                                         "--jitter-probe=" + probe},
                                        &setupSeccompGetaffinity);
  check(run.status == 2, "the unreadable mask refuses the value (FR-011)");
  checkOneLine(run.err, "the unreadable mask prints one line (FR-011)");
  check(run.err.find("unreadable") != std::string::npos,
        "the line names the unreadable mask (FR-011)");
  return 0;
}

// FR-022, R-08: the fit of the family that ended prints between the
// rows of two families, behind the join edge the probe owns, so the
// probed run defers every table to the stop (R-08).
auto scenarioFitBesideProbe() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run =
      runChildScenario("fitBesideProbe",
                       {"--filter=^bmFit",
                        "--repetitions=1",
                        "--iterations=2",
                        "--jitter-probe=" + probe + ",precheck=1"},
                       &setupFitFamilies);
  check(run.status == 0, "the two families run beside the probe (R-08)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("bmFitA") != std::string::npos
            && run.out.find("bmFitB") != std::string::npos,
        "the batch carries both families (FR-022)");
  return 0;
}

// FR-056, R-08: an interleaved probed run holds every row and the fit
// for the print pass the probe's stop owns, so each prints one time,
// in registration order, after the probe join and before the probe
// block. The document carries the rows and the fits in the same order.
auto countOf(const std::string& text, const std::string& needle) -> std::size_t
{
  std::size_t found = 0;
  for (std::size_t at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + 1))
  {
    ++found;
  }
  return found;
}

auto scenarioInterleaveProbe() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run =
      runChildScenario("interleaveProbe",
                       {"--filter=^bmFit",
                        "--repetitions=1",
                        "--iterations=2",
                        "--interleave=1",
                        "--jitter-probe=" + probe,
                        "--json=harness_jitter_cli_interleaveProbe.json"},
                       &setupFitFamilies);
  check(run.status == 0,
        "the interleaved probed run stands status 0 (FR-056, R-08)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  // Two instances per family print once each, and the BIG_O and RMS
  // fits of each family print once: a doubled print pass would carry
  // eight row lines (FR-056, R-08). The narrowing disclosure quotes
  // the rows' names, so the counts read the shapes; the disclosure's
  // own quotes stand beside them.
  check(countOf(run.out, "iterations=") == 4 && countOf(run.out, "BIG_O") == 2
            && countOf(run.out, "RMS") == 2,
        "each row and the fit print one time (FR-056, R-08)");
  // The last fit line stands before the probe block; the narrowing
  // disclosure quotes the rows' names after it, so the anchor reads
  // the fits; the disclosure's quotes stand past them (R-08).
  check(run.out.rfind("RMS") < run.out.find("jitter probe:"),
        "the rows and the fit stand before the probe block (R-08)");
  const auto document = readDocument("harness_jitter_cli_interleaveProbe.json");
  check(countOf(document, "\"kind\":\"BIG_O\"") == 2,
        "the document carries the two fits (FR-022, R-08)");
  return 0;
}

// FR-037, D-10: a probe the machine/tsc leaf is missing for stands
// disclosed and the batch continues at status 0; the document carries
// the unavailable record with the default threshold (E-07).
auto scenarioUnavailableProbe() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=unavailable", "unavailable");
  check(run.status == 0,
        "the unavailable probe discloses and continues (FR-037, D-10)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("not available") != std::string::npos,
        "the run discloses the unavailable answer (FR-037)");
  check(run.out.find("affinity mask narrowed") == std::string::npos,
        "the unavailable probe narrows no mask (FR-037, D-5)");
  check(run.out.find("bmQuiet") != std::string::npos,
        "the batch ran beside the disclosure (FR-037, D-10)");
  const auto document = readDocument("harness_jitter_cli_unavailable.json");
  check(document.find("not available") != std::string::npos,
        "the document carries the unavailable record (E-07)");
  check(document.find("\"thresholdNs\":200") != std::string::npos,
        "the record carries the default threshold (E-07)");
  std::remove("harness_jitter_cli_unavailable.json");
  return 0;
}

// FR-037, D-10: the same unavailable answer where the tsc leaf stands
// in the tree at a count the catalog marks blocked: the runner's
// window stamps refuse on the availability gate the compile answers,
// and the batch continues at status 0 (FR-024).
auto scenarioUnavailableBlocked() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=unavailable-blocked",
                                    "unavailable-blocked");
  check(run.status == 0,
        "the blocked leaf discloses and continues (FR-037, D-10)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("not available") != std::string::npos,
        "the run discloses the unavailable answer (FR-037)");
  check(run.out.find("bmQuiet") != std::string::npos,
        "the batch ran beside the disclosure (FR-037, D-10)");
  return 0;
}

// FR-037, D-10, E-06: the same unavailable answer under a threaded
// batch: the threaded body's window edges stamp against the leaf the
// catalog lacks, stand unrecorded, and the batch continues at status
// 0 (E-06).
auto scenarioUnavailableThreaded() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=unavailable-threaded",
                                    "unavailable-threaded");
  check(run.status == 0,
        "the threaded unavailable run discloses and continues (FR-037)");
  check(run.out.find("UNAPPLIED") == std::string::npos,
        "no pin stands unapplied (FR-005, VI)");
  check(run.out.find("not available") != std::string::npos,
        "the run discloses the unavailable answer (FR-037)");
  check(run.out.find("bmThreaded") != std::string::npos,
        "the threaded batch ran beside the disclosure (FR-037, E-06)");
  return 0;
}

// FR-037, D-10: the same unavailable answer under `strict` stops the
// batch before any run, one line and status 2.
auto scenarioUnavailableStrict() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=unavailable-strict",
                                    "unavailable-strict");
  check(run.status == 2, "the strict refusal stops the batch (FR-037)");
  checkOneLine(run.err, "the strict refusal prints one line (FR-037)");
  check(run.err.find("the machine/tsc leaf is not available")
            != std::string::npos,
        "the line names the unavailable leaf (FR-037)");
  check(run.out.find("bmQuiet") == std::string::npos,
        "no benchmark ran (FR-037)");
  return 0;
}

// The fresh process of the per-family probe scenarios: the fake it
// registers scripts the tsc series the runner's window stamps and the
// probe's own reads share, with a jump far above the threshold every
// tenth read in the points and a tail that jumps on every read past
// them, so any window a probe read falls inside carries the gap
// whatever the runner consumed (R-02, E-06). The points' steps stand
// at one tick, so the measurement pass reads the small minDelta the
// adaptive threshold rises from. The buffer holds every read, so no
// kept gap stands dropped. The handle names the probe on the first
// cpu the mask holds; the command line's value, when the caller names
// one, stands on the second (FR-015).
auto childFamilyProbe(const bool withCommandLineProbe) -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  provider.addCounter("machine",
                      "tsc",
                      "none",
                      "raw tick counter",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::FAST_TSC);
  // The leaf carries the seeded delta tail and no explicit points: each
  // read steps the orbit `state = state * 37 + 11 (mod 2^16)`, which
  // walks all 65,536 values, so a read stands at or above the probe's
  // threshold once every 123 reads and never farther apart than 775
  // reads (T014, FR-015). The orbit never exhausts, so the gap rate
  // stands the same in the last window as in the first. Two properties
  // make a fold's presence stand still across builds and loads. The
  // threshold stands explicit, because the default is ten times the
  // smallest delta of the probe's own measurement pass, and that pass
  // reads the same leaf the window stamps and the probe read, so a
  // default would follow how far each reader walked (E-05, FR-007). And
  // the gap rate stands low against the buffer's 2,000,000 slots, which
  // hold about a hundred and twenty times as many reads as they hold
  // gaps, so no gap stands dropped and every gap a window places is a
  // gap the buffer kept: a buffer that fills ahead of the first window
  // keeps only the gaps of the spin in front of it, and no later window
  // can place one (FR-010, D-6, E-06).
  provider.setPoints("machine", "tsc", {}, 0, 1);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fresh process registers its fake (R-02)");
  const auto cpus = pickCpus(2);
  // The timed window has to span the interval in which the scheduler may
  // leave the probe thread unscheduled. The probe samples on its own cpu
  // beside whatever else the machine runs, and `start` promises only
  // that the item stands at the start latch when the batch begins
  // (R-04): when the alongside loop is next read is the scheduler's
  // decision. A window that holds no probe read places no gap, folds
  // nothing, and reports "jitter":null, and a batch that ends before the
  // probe thread is scheduled again keeps no gap at all. One iteration of
  // this spin is 600,000,000 volatile adds, so a window spans many times
  // that interval on every build and the probe's share of its cpu falls
  // inside it (FR-015, R-04; VI: no sleep and no clock).
  constexpr int kSpinSteps = 600000000;
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
          // The unsigned wrap stands defined, so the sanitizer build
          // keeps the spin's work without a signed overflow (VI).
          volatile std::uint32_t sink = 0;
          for (int step = 0; step < kSpinSteps; ++step) {
            sink += static_cast<std::uint32_t>(step);
          }
          (void)sink;
        }
      },
      "bmProbed");
  auto probeOptions = sg::JitterProbeOptions {};
  probeOptions.cpus = {cpus.at(0)};
  // The threshold stands explicit for the reason the script above
  // states: a default is set from the measurement pass's own minDelta
  // over the leaf the window stamps share (E-05, FR-007).
  probeOptions.thresholdNs = 65000;
  // The buffer holds every gap a run of this shape makes at the
  // script's gap rate, so no kept gap stands dropped and the folds the
  // scenarios read stand the run's own (FR-010, D-6).
  probeOptions.gapCount = 2000000;
  handle.jitterProbe(probeOptions);
  std::vector<std::string> arguments {
      "--repetitions=1",
      "--iterations=2",
      "--json=harness_jitter_cli_"
          + std::string(withCommandLineProbe ? "family-vs-cli" : "family-probe")
          + ".json"};
  if (withCommandLineProbe) {
    arguments.push_back("--jitter-probe=" + std::to_string(cpus.at(1)));
  }
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The text of one benchmark entry's object in the document: the span
// from the entry's name to the next entry's name, the span the entry's
// own keys read in (E-10).
auto entrySegment(const std::string& document,
                  const std::string& name) -> std::string
{
  const auto begin = document.find("\"name\":\"" + name + "\"");
  if (begin == std::string::npos) {
    return {};
  }
  const auto next = document.find("\"name\":\"", begin + 1);
  return document.substr(
      begin, next == std::string::npos ? std::string::npos : next - begin);
}

// The fresh process of the empty-intersection scenario: the child pins
// its own two-cpu mask, registers a pinned family ahead of the
// unpinned one, and runs with the probe cpus naming the whole mask, so
// the unpinned entry's narrowing would leave no cpu (FR-019, D-5,
// R-10).
auto childEmptyMask() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(2);
  cpu_set_t narrow {};
  CPU_ZERO(&narrow);
  CPU_SET(cpus.at(0), &narrow);
  CPU_SET(cpus.at(1), &narrow);
  check(sched_setaffinity(0, sizeof(narrow), &narrow) == 0,
        "the child pins its two-cpu mask (FR-019)");
  auto pinned = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmPinned");
  pinned.threads(2).placement("core");
  std::vector<std::string> arguments {
      "--repetitions=1",
      "--iterations=2",
      "--jitter-probe=" + std::to_string(cpus.at(0)) + ","
          + std::to_string(cpus.at(1))};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The fresh process of the online-set scenario: the fake it registers
// publishes the leaves the runner resolves its plans through and a tsc
// leaf the probe reads, and the layout it installs carries the online
// set 0-3 and 8-11 with the count seam at 8, the shape the M-5 host
// reports. The pinned benchmark plans the first block's leads, so the
// idle resolution yields the second block's lowest free cpu, and the
// probe starts on it with no failed contract (FR-016, FR-041, R-02).
auto childIdleHighIndex() -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  provider.addCounter("machine",
                      "tsc",
                      "none",
                      "raw tick counter",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::FAST_TSC);
  provider.setPoints("machine", "tsc", {0}, 1);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fresh process registers its fake (R-02)");
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  const std::array<std::pair<std::uint32_t, std::uint32_t>, 4> cores {
      {{0, 1}, {2, 3}, {8, 9}, {10, 11}}};
  for (const auto& pair : cores) {
    const auto core = static_cast<std::uint32_t>(installed.cores.size());
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = 0;
    installed.cores.push_back(layoutCore);
    for (const auto cpu : {pair.first, pair.second}) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = cpu;
      thread.coreOrdinal = core;
      thread.groupOrdinal = 0;
      installed.cores.at(core).threads.push_back(
          static_cast<std::uint32_t>(installed.threads.size()));
      installed.threads.push_back(thread);
      installed.mask.push_back(cpu);
    }
  }
  sg::detail::LayoutGroup layoutGroup;
  layoutGroup.lowestCpu = 0;
  for (std::uint32_t core = 0; core < 4; ++core) {
    layoutGroup.cores.push_back(core);
  }
  installed.groups.push_back(layoutGroup);
  sg::detail::installTestLayout(installed);
  sg::detail::installTestOnlineCount(8);
  auto pinned = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmIdlePinned");
  pinned.threads(2).placement("core");
  std::vector<std::string> arguments {
      "--repetitions=1", "--iterations=2", "--jitter-probe=idle"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// FR-015: the family probe with no cpu list. The handle names no cpu, so
// the idle preset resolves over the plans of the family's own instances.
// The child installs no layout, so that resolution answers no cpu and the
// run stops with the preset's own line and status 2 (FR-015, FR-017).
auto childFamilyIdle() -> int
{
  scriptedProvider();
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamIdle");
  sg::JitterProbeOptions probeOptions;
  probeOptions.thresholdNs = 65000;
  probeOptions.gapCount = 2000000;
  handle.jitterProbe(probeOptions);
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// FR-015, FR-019: the family probe naming a cpu the family's own plan
// already holds. The instance stands pinned and the handle names every
// cpu of the affinity mask, so the overlap of the two sets refuses the
// run with one line naming the benchmark.
auto childFamilyOverlap() -> int
{
  scriptedProvider();
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamOverlap");
  handle.threads(2).placement("core");
  sg::JitterProbeOptions probeOptions;
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the child cannot read its own affinity mask");
  }
  for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE); ++cpu)
  {
    if (CPU_ISSET(cpu, &mask) != 0) {
      probeOptions.cpus.push_back(static_cast<std::uint32_t>(cpu));
    }
  }
  probeOptions.thresholdNs = 65000;
  probeOptions.gapCount = 2000000;
  handle.jitterProbe(probeOptions);
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// FR-015, FR-019: the family probe whose cpus fill the affinity mask.
// The instance asks for two threads and stands unpinned, so the walk
// narrows the mask to what the probe leaves. Nothing stands left, so
// the run refuses with one line naming the benchmark.
auto childFamilyNarrowedEmpty() -> int
{
  scriptedProvider();
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamNarrow");
  handle.threads(2);
  sg::JitterProbeOptions probeOptions;
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the child cannot read its own affinity mask");
  }
  for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE); ++cpu)
  {
    if (CPU_ISSET(cpu, &mask) != 0) {
      probeOptions.cpus.push_back(static_cast<std::uint32_t>(cpu));
    }
  }
  probeOptions.thresholdNs = 65000;
  probeOptions.gapCount = 2000000;
  handle.jitterProbe(probeOptions);
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The fresh process of the family availability scenarios: the fake
// publishes a monotonic and a thread-cpu leaf with no machine/tsc leaf,
// so the catalog the availability check reads carries no qualifying
// entry. The handle names the probe, so the family owns the session. Strict
// stops the run; the plain run discloses and goes on (FR-024, FR-037).
auto childFamilyUnavailable(const bool strict) -> int
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
  provider.setPoints("machine", "monotonic", {0}, 4000);
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fresh process registers its fake (R-02)");
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamQuiet");
  sg::JitterProbeOptions probeOptions;
  probeOptions.cpus.push_back(pickCpus(1).front());
  probeOptions.thresholdNs = 65000;
  probeOptions.gapCount = 2000000;
  probeOptions.strict = strict;
  handle.jitterProbe(probeOptions);
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// FR-015: two families, each with a handle of its own. The walk starts
// the second family's probe as the first family ends, so the live
// session of the first meets the stop there and one report carries both
// families' reads.
auto childFamilyTwoFamilies() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(1);
  auto first = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamOne");
  auto second = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamTwoFam");
  for (auto handle : {first, second}) {
    sg::JitterProbeOptions probeOptions;
    probeOptions.cpus.push_back(cpus.front());
    probeOptions.thresholdNs = 65000;
    probeOptions.gapCount = 2000000;
    handle.jitterProbe(probeOptions);
  }
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The fresh process of the interrupted family scenario: the body of the
// first family raises the interrupt signal once, so the walk stops with
// that family's probe live and the second family's probe never starts
// (R-01, FR-015).
auto childFamilyInterrupted() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(1);
  auto first = sg::registerBenchmark(
      [](sg::State& state)
      {
        static bool fired = false;
        for (auto _ : state) {
          if (!fired) {
            fired = true;
            std::raise(SIGINT);
          }
        }
      },
      "bmFamInterrupt");
  auto second = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamAfter");
  for (auto handle : {first, second}) {
    sg::JitterProbeOptions probeOptions;
    probeOptions.cpus.push_back(cpus.front());
    probeOptions.thresholdNs = 65000;
    probeOptions.gapCount = 2000000;
    handle.jitterProbe(probeOptions);
  }
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The fresh process of the source-order scenario: the option file names
// one cpu and the command line names another. The words are scanned
// first and the file follows them, so the file's row settles nothing
// while the command line's text stands (FR-037, FR-038).
auto childFileVersusCommandLine() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(2);
  {
    std::ofstream file("harness_jitter_cli_probe-order.txt");
    file << "jitter-probe=" << cpus[0] << "\n";
  }
  const std::string path = "harness_jitter_cli_probe-order.txt";
  std::vector<std::string> arguments {
      "--repetitions=1",
      "--iterations=2",
      "--options=" + path,
      "--jitter-probe=" + std::to_string(cpus[1])};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::remove(path.c_str());
  return status;
}

// FR-015: one family, two argument lists, and the family's own probe.
// The probe stands once for the family: it starts before the first
// instance and stops behind the last, and the second instance joins
// the record the first opened.
auto childFamilyTwoInstances() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(1);
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamTwo");
  sg::JitterProbeOptions probeOptions;
  probeOptions.cpus = {cpus.at(0)};
  probeOptions.thresholdNs = 65000;
  probeOptions.gapCount = 2000000;
  handle.arg(2).arg(4).jitterProbe(probeOptions);
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

auto scenarioFamilyProbe() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=family-probe", "family-probe");
  check(run.status == 0, "the family-probed run stands status 0 (FR-015)");
  const auto document = readDocument("harness_jitter_cli_family-probe.json");
  const auto cpus = pickCpus(2);
  const std::string familyCpu = std::to_string(cpus.at(0));
  const std::string probed = entrySegment(document, "bmProbed");
  const std::string plain = entrySegment(document, "bmQuiet");
  check(!probed.empty() && !plain.empty(),
        "the document carries both families' entries (FR-015)");
  check(probed.find("\"jitter\":{\"stats\":[{\"cpu\":" + familyCpu)
            != std::string::npos,
        "the probed family's entries carry its own probe's folds " "(FR-015)");
  check(plain.find("\"jitter\":null") != std::string::npos,
        "the family the handle named not carries no fold (FR-015)");
  check(
      run.out.find("affinity mask narrowed for 'bmProbed'")
          != std::string::npos,
      "the probed family's instances narrow by the family's cpus " "(FR-015)");
  check(
      run.out.find("affinity mask narrowed for 'bmQuiet'") == std::string::npos,
      "the family the handle named not narrows by nothing (FR-015)");
  check(document.find("\"cpus\":[" + familyCpu + "]") != std::string::npos,
        "the run-level record names the handle's cpus (FR-015)");
  check(run.out.find("jitter probe: cpus " + familyCpu) != std::string::npos,
        "the console block names the handle's cpus (FR-015)");
  check(probed.find("\"droppedGaps\":0") != std::string::npos,
        "the family's fold keeps every gap the run made (FR-010)");
  check(document.find("\"gapCount\":0") == std::string::npos,
        "the family's own probe kept a gap the entries could fold " "(FR-015)");
  return 0;
}

// FR-015: the handle and the command line both stand, and the handle
// wins for its family: the probed family's folds carry the handle's
// cpu and no command-line cpu, the run-level record names the handle's
// cpus, and the family the handle named not carries no fold, the one
// session the process holds running the family's own probe (FR-015,
// R-01, R-06).
// FR-015: the family probe with no cpu list runs on the idle preset.
// An entry the handle leaves unpinned plans no cpu, so the resolution
// answers none for the probe and the run refuses with the line for a
// preset that names no free cpu.
// FR-015, FR-019: the family probe asks for a precheck with no cpu
// list. The precheck takes the selected instances' own plan, and an
// entry the handle leaves unpinned plans nothing, so the run refuses
// with the line for a preset left without a plan.
auto childFamilyPrecheckIdle() -> int
{
  scriptedProvider();
  auto handle = sg::registerBenchmark(
      [](sg::State& state)
      {
        for (auto _ : state) {
        }
      },
      "bmFamPrecheck");
  sg::JitterProbeOptions probeOptions;
  probeOptions.mode = sg::JitterProbeMode::PRECHECK;
  probeOptions.thresholdNs = 65000;
  probeOptions.gapCount = 2000000;
  handle.jitterProbe(probeOptions);
  std::vector<std::string> arguments {"--repetitions=1", "--iterations=2"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

auto scenarioFamilyIdleRun() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=family-idle", "family-idle");
  check(run.status == 2, "the family run on an empty idle resolution stops "
                         "with status 2 (FR-015)");
  check(run.err.find("the idle preset names no free cpu") != std::string::npos,
        "the refusal names the idle preset it could not resolve (FR-015)");
  check(run.err.find("no cpu is planned") == std::string::npos,
        "the family branch answers the preset line, not the batch one "
        "(FR-015)");
  return 0;
}

// FR-015, FR-019: a family probe naming a cpu the family's own plan
// holds answers one refusal line naming the benchmark, and the run
// leaves no entries behind.
auto scenarioFamilyOverlapRun() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=family-overlap", "family-overlap");
  check(run.status == 2,
        "the overlapping family probe stops the run with " "status 2 (FR-019)");
  check(run.err.find("overlaps the planned cpus of" " 'bmFamOverlap/threads:2'")
            != std::string::npos,
        "the refusal names the benchmark whose plan it overlaps (FR-019)");
  return 0;
}

// FR-015: the family's own probe stands once for two argument lists:
// both instances fold beside it, and the family's cpus narrow the
// walk's mask for each of them.
auto scenarioFamilyTwoInstances() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=family-two", "family-two");
  check(run.status == 0,
        "the two-instance family run stands status 0 " "(FR-015)");
  check(run.out.find("affinity mask narrowed for 'bmFamTwo/2")
            != std::string::npos,
        "the family's cpus narrow the first instance's mask (FR-015)");
  check(run.out.find("affinity mask narrowed for 'bmFamTwo/4")
            != std::string::npos,
        "the family's cpus narrow the last instance's mask (FR-015)");
  check(run.out.find("jitter: cpu ") != std::string::npos,
        "the family's own probe reports once for its instances (FR-015)");
  return 0;
}

// FR-015, FR-019: a family precheck with no cpu list answers the plan
// line for the preset it could not resolve. The run leaves status two
// behind.
auto scenarioFamilyPrecheckIdle() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=family-precheck-idle",
                                    "family-precheck-idle");
  check(run.status == 2,
        "the family precheck with no plan stops with status 2 (FR-019)");
  check(run.err.find("no cpu is planned for the idle preset")
            != std::string::npos,
        "the refusal names the preset left without a plan (FR-019)");
  return 0;
}

// FR-015, FR-019: a family probe that fills the mask leaves the
// two-thread instance no cpu to run on. The run answers one line naming
// that benchmark and leaves status two behind.
auto scenarioFamilyNarrowedEmpty() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=family-narrowed-empty",
                                    "family-narrowed-empty");
  check(run.status == 2,
        "the family probe that fills the mask stops with status 2 (FR-019)");
  check(run.err.find("the probe cpus leave no cpu for 'bmFamNarrow/threads:2'")
            != std::string::npos,
        "the refusal names the benchmark left without a cpu (FR-019)");
  return 0;
}

// FR-024, FR-037: a family probe over a machine with no machine/tsc
// leaf answers the availability line and leaves status two behind when
// the handle asks for strict.
auto scenarioFamilyUnavailableStrict() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=family-unavailable-strict",
                   "family-unavailable-strict");
  check(run.status == 2,
        "the strict family probe with no leaf stops with status 2 (FR-024)");
  check(run.err.find("the machine/tsc leaf is not available")
            != std::string::npos,
        "the refusal names the missing leaf (FR-024)");
  return 0;
}

// FR-037: the same machine with a plain handle discloses the answer and
// the run goes on to finish.
auto scenarioFamilyUnavailable() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=family-unavailable",
                                    "family-unavailable");
  check(run.status == 0,
        "the plain family probe with no leaf finishes the run (FR-037)");
  check(run.out.find("jitter: not available") != std::string::npos,
        "the report discloses the unavailable answer (FR-037)");
  return 0;
}

// FR-015: the two-family run finishes with one report that carries the
// reads of both families' probes.
auto scenarioFamilyTwoFamilies() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=family-two-families",
                                    "family-two-families");
  check(run.status == 0, "two probed families finish the run (FR-015)");
  check(run.out.find("jitter: cpu ") != std::string::npos,
        "one report carries the probe reads (FR-015)");
  return 0;
}

// R-01, FR-015: the interrupt stops the walk beside the live session of
// the first family. The second family never starts, so its probe keeps
// no session and its benchmark stands outside the record.
auto scenarioFamilyInterrupted() -> int
{
  const Captured run = runExecChild("--jitter-cli-child=family-interrupted",
                                    "family-interrupted");
  check(run.status == 1, "the interrupt leaves the run with status 1 (R-01)");
  check(run.out.find("jitter: probe on cpus ") != std::string::npos,
        "the report carries the family probe that ran (FR-015)");
  check(run.out.find("bmFamAfter") == std::string::npos,
        "the pending family never starts (R-01)");
  return 0;
}

// FR-037, FR-038: the words are scanned first, so the file's row
// settles nothing while the command line's text stands. The report
// carries the probe of the command line.
auto scenarioFileVersusCommandLine() -> int
{
  const auto cpus = pickCpus(2);
  const Captured run =
      runExecChild("--jitter-cli-child=file-versus-command-line",
                   "file-versus-command-line");
  check(run.status == 0, "the two sources finish the run (FR-037)");
  check(run.out.find("jitter: cpu " + std::to_string(cpus[1]) + " kept")
            != std::string::npos,
        "the command line's probe cpu stands in the report (FR-038)");
  check(run.out.find("jitter: cpu " + std::to_string(cpus[0]) + " kept")
            == std::string::npos,
        "the file's row settles nothing here (FR-038)");
  return 0;
}

auto scenarioFamilyVersusCommandLine() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=family-vs-cli", "family-vs-cli");
  check(run.status == 0, "the two-probe run stands status 0 (FR-015)");
  const auto document = readDocument("harness_jitter_cli_family-vs-cli.json");
  const auto cpus = pickCpus(2);
  const std::string familyCpu = std::to_string(cpus.at(0));
  const std::string cliCpu = std::to_string(cpus.at(1));
  const std::string probed = entrySegment(document, "bmProbed");
  const std::string plain = entrySegment(document, "bmQuiet");
  check(!probed.empty() && !plain.empty(),
        "the document carries both families' entries (FR-015)");
  check(probed.find("\"cpu\":" + familyCpu + ",") != std::string::npos,
        "the family's folds carry the handle's cpu (FR-015)");
  check(probed.find("\"cpu\":" + cliCpu + ",") == std::string::npos,
        "the family's folds carry no command-line cpu (FR-015)");
  check(plain.find("\"jitter\":null") != std::string::npos,
        "the family the handle named not carries no fold (FR-015)");
  check(document.find("\"cpus\":[" + familyCpu + "]") != std::string::npos,
        "the run-level record names the handle's cpus (FR-015)");
  check(probed.find("\"droppedGaps\":0") != std::string::npos,
        "the family's fold keeps every gap the run made (FR-010)");
  check(document.find("\"gapCount\":0") == std::string::npos,
        "the family's own probe kept a gap the entries could fold " "(FR-015)");
  return 0;
}

// FR-019, D-5, R-10: the empty-intersection refusal stands before the
// walk: the pinned entry ahead of the failing unpinned one runs no row,
// the refusal prints one line, and the status stands 2 (FR-036). The
// pinned entry's own plans overlap the whole-mask probe cpus, so the
// overlap refusal names it before the walk, the same pre-walk region
// the empty-intersection check reads (FR-018, R-10).
auto scenarioEmptyMaskBeforeWalk() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=empty-mask", "empty-mask");
  check(run.status == 2,
        "the empty-intersection refusal stands status 2 (FR-019, R-10)");
  check(std::count(run.err.begin(), run.err.end(), '\n') == 1,
        "the refusal stands one line (FR-038)");
  check(run.out.find("bmPinned") == std::string::npos,
        "the pinned entry ahead ran no row (FR-036, R-10)");
  check(run.out.find("bmQuiet") == std::string::npos, "no row prints (R-10)");
  return 0;
}

// FR-016, FR-041: the idle resolution yields the second block's lowest
// free cpu, an index the count the threads report stands at or above,
// and the probe starts on it with no failed contract: the run stands
// status 0 and the console block names the picked cpu (FR-005).
auto scenarioIdleHighIndex() -> int
{
  const Captured run =
      runExecChild("--jitter-cli-child=idle-high-index", "idle-high-index");
  check(run.status == 0, "the idle run stands status 0 (FR-016, FR-041)");
  check(run.out.find("jitter probe: cpus 8") != std::string::npos,
        "the probe starts on the cpu the set holds past the count (FR-005)");
  return 0;
}

}  // namespace

SG_BENCHMARK(bmQuiet);

auto main(int argc, char** argv) -> int
{
  // The fresh-process scenarios re-enter this binary with their mode
  // and answer before any provider stands registered (FR-037, R-02).
  if (argc > 1) {
    const std::string_view mode = argv[1];
    auto redirect = [](const char* tag) -> bool
    {
      const std::string outPath =
          std::string("harness_jitter_cli_") + tag + "-out.txt";
      const std::string errPath =
          std::string("harness_jitter_cli_") + tag + "-err.txt";
      return std::freopen(outPath.c_str(), "w", stdout) != nullptr
          && std::freopen(errPath.c_str(), "w", stderr) != nullptr;
    };
    if (mode == "--jitter-cli-child=empty-mask" && redirect("empty-mask")) {
      return childEmptyMask();
    }
    if (mode == "--jitter-cli-child=idle-high-index"
        && redirect("idle-high-index"))
    {
      return childIdleHighIndex();
    }
    if (mode == "--jitter-cli-child=family-probe" && redirect("family-probe")) {
      return childFamilyProbe(false);
    }
    if (mode == "--jitter-cli-child=family-vs-cli" && redirect("family-vs-cli"))
    {
      return childFamilyProbe(true);
    }
    if (mode == "--jitter-cli-child=family-idle" && redirect("family-idle")) {
      return childFamilyIdle();
    }
    if (mode == "--jitter-cli-child=family-overlap"
        && redirect("family-overlap"))
    {
      return childFamilyOverlap();
    }
    if (mode == "--jitter-cli-child=file-versus-command-line"
        && redirect("file-versus-command-line"))
    {
      return childFileVersusCommandLine();
    }
    if (mode == "--jitter-cli-child=family-interrupted"
        && redirect("family-interrupted"))
    {
      return childFamilyInterrupted();
    }
    if (mode == "--jitter-cli-child=family-two-families"
        && redirect("family-two-families"))
    {
      return childFamilyTwoFamilies();
    }
    if (mode == "--jitter-cli-child=family-narrowed-empty"
        && redirect("family-narrowed-empty"))
    {
      return childFamilyNarrowedEmpty();
    }
    if (mode == "--jitter-cli-child=family-unavailable-strict"
        && redirect("family-unavailable-strict"))
    {
      return childFamilyUnavailable(true);
    }
    if (mode == "--jitter-cli-child=family-unavailable"
        && redirect("family-unavailable"))
    {
      return childFamilyUnavailable(false);
    }
    if (mode == "--jitter-cli-child=family-precheck-idle"
        && redirect("family-precheck-idle"))
    {
      return childFamilyPrecheckIdle();
    }
    if (mode == "--jitter-cli-child=family-two" && redirect("family-two")) {
      return childFamilyTwoInstances();
    }
    if (mode == "--jitter-cli-child=file-over-suite"
        && redirect("fileOverSuite"))
    {
      return childFileOverSuite();
    }
    if (mode == "--jitter-cli-child=unavailable" && redirect("unavailable")) {
      return childUnavailable(false);
    }
    if (mode == "--jitter-cli-child=unavailable-blocked"
        && redirect("unavailable-blocked"))
    {
      return childUnavailableBlocked();
    }
    if (mode == "--jitter-cli-child=unavailable-threaded"
        && redirect("unavailable-threaded"))
    {
      return childUnavailableThreaded();
    }
    if (mode == "--jitter-cli-child=unavailable-strict"
        && redirect("unavailable-strict"))
    {
      return childUnavailable(true);
    }
    return 3;
  }

  scriptedProvider();
  const auto cpus = pickCpus(2);
  const std::string cpu = std::to_string(cpus.at(0));
  const std::vector<std::string> quickRun {
      "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};

  // T018: the placement status-2 cases of US2 run first, in children
  // forked before the entry point's first layout read: `layout()`
  // caches the read once per process and an install cannot invalidate
  // it after, so a child that installs its synthetic layout before any
  // read answers the install, and the parent's later in-process
  // scenarios answer the host (FR-002, Q-5). The base cpu and its
  // sibling come from the process affinity mask (VI), and the
  // mask-emptied scenario probes every cpu the mask holds.
  baseCpu = cpus.at(0);
  siblingCpu = cpus.at(1);
  allCpus = allMaskCpus();
  check(scenarioIdleNoPlans() == 0, "idleNoPlans");
  check(scenarioIdleNoFree() == 0, "idleNoFree");
  check(scenarioOverlapPlanned() == 0, "overlapPlanned");
  check(scenarioOverlapSibling() == 0, "overlapSibling");
  check(scenarioPrecheckAllows() == 0, "precheckAllows");
  check(scenarioPrecheckWholeMask() == 0, "precheckWholeMask");
  check(scenarioBothOverlap() == 0, "bothOverlap");
  check(scenarioMaskEmpty() == 0, "maskEmpty");
  check(scenarioSingleContext() == 0, "singleContext");
  check(scenarioSkipDisclosure() == 0, "skipDisclosure");
  check(scenarioCpusetLimit() == 0, "cpusetLimit");
  check(scenarioUnreadableMask() == 0, "unreadableMask");
  check(scenarioFitBesideProbe() == 0, "fitBesideProbe");
  check(scenarioInterleaveProbe() == 0, "interleaveProbe");
  check(scenarioUnavailableProbe() == 0, "unavailableProbe");
  check(scenarioUnavailableBlocked() == 0, "unavailableBlocked");
  check(scenarioUnavailableThreaded() == 0, "unavailableThreaded");
  check(scenarioUnavailableStrict() == 0, "unavailableStrict");
  check(scenarioFamilyProbe() == 0, "familyProbe");
  check(scenarioFamilyIdleRun() == 0, "familyIdleRun");
  check(scenarioFamilyOverlapRun() == 0, "familyOverlapRun");
  check(scenarioFamilyTwoInstances() == 0, "familyTwoInstances");
  check(scenarioFamilyInterrupted() == 0, "familyInterrupted");
  check(scenarioFileVersusCommandLine() == 0, "fileVersusCommandLine");
  check(scenarioFamilyTwoFamilies() == 0, "familyTwoFamilies");
  check(scenarioFamilyUnavailableStrict() == 0, "familyUnavailableStrict");
  check(scenarioFamilyUnavailable() == 0, "familyUnavailable");
  check(scenarioFamilyNarrowedEmpty() == 0, "familyNarrowedEmpty");
  check(scenarioFamilyPrecheckIdle() == 0, "familyPrecheckIdle");
  check(scenarioFamilyVersusCommandLine() == 0, "familyVersusCommandLine");
  check(scenarioEmptyMaskBeforeWalk() == 0, "emptyMaskBeforeWalk");
  check(scenarioIdleHighIndex() == 0, "idleHighIndex");

  // FR-011: the cpu list, `idle`, and every key parse once, each key
  // standing once in the value; the accepted value proceeds into the
  // batch.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",threshold=500,"
                        "mode=precheck,precheck=1,gaps=4,strict");
    const auto report = captureRun(arguments);
    check(report.find("bmQuiet") != std::string::npos,
          "the accepted value runs the batch (FR-011)");
    // The alongside mode stands named in the value the same way; the
    // parse assigns it like the other two names (FR-011).
    auto alongsideArguments = quickRun;
    alongsideArguments.push_back("--jitter-probe=" + cpu
                                 + ",mode=alongside,precheck=1");
    const auto alongsideReport = captureRun(alongsideArguments);
    check(alongsideReport.find("bmQuiet") != std::string::npos,
          "the named alongside mode stands accepted (FR-011)");
  }

  // FR-011: a repeated key stands rejected on one line and status 2,
  // before any benchmark runs.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",threshold=1,threshold=2");
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a repeated key stands rejected (FR-011)");
  }

  // FR-011: the value grammar's remaining rejections. The bare cpu
  // extension, the head token that names no cpu list, the cpu range,
  // the reversed range, the cpu outside the mask, the valued strict
  // and the unknown key each stand one line and status 2.
  {
    const std::string second = std::to_string(cpus.at(1));
    auto accepted = quickRun;
    accepted.push_back("--jitter-probe=" + cpu + "," + second
                       + ",mode=precheck,precheck=1");
    check(captureRun(accepted).find("bmQuiet") != std::string::npos,
          "a bare cpu extension joins the head's list (FR-011)");
    auto range = quickRun;
    range.push_back("--jitter-probe=" + cpu + "-" + second
                    + ",mode=precheck,precheck=1");
    // A closed range joins both cpus when the mask holds every cpu
    // between them; under a gapped mask the range's interior stands
    // outside, and the same grammar rejects it (FR-011).
    if (std::stoi(second) == std::stoi(cpu) + 1) {
      check(captureRun(range).find("bmQuiet") != std::string::npos,
            "a closed cpu range joins both cpus (FR-011)");
    } else {
      checkOneLine(captureError(range, 2),
                   "a range over the mask's gap stands rejected (FR-011)");
    }

    auto extension = quickRun;
    extension.push_back("--jitter-probe=" + cpu + ",zzz");
    checkOneLine(captureError(extension, 2),
                 "a bare token outside the grammar stands rejected (FR-011)");
    auto head = quickRun;
    head.push_back("--jitter-probe=zzz");
    checkOneLine(captureError(head, 2),
                 "a head token that names no cpu stands rejected (FR-011)");
    auto reversed = quickRun;
    reversed.push_back("--jitter-probe=" + second + "-" + cpu);
    checkOneLine(captureError(reversed, 2),
                 "a reversed cpu range stands rejected (FR-011)");
    // The mask-membership refusal: on a host whose mask holds every
    // online cpu the count bound stands first, so the bit read's own
    // refusal reads in a child that holds one cpu of the mask alone
    // and names the other, a cpu below the count the threads report
    // (FR-011).
    const Captured outside =
        runChildScenario("outside",
                         {"--filter=bmQuiet",
                          "--repetitions=1",
                          "--iterations=2",
                          "--jitter-probe=" + std::to_string(siblingCpu)},
                         &setupOutsideMask);
    check(outside.status == 2,
          "a cpu outside the child's mask stands rejected (FR-011)");
    checkOneLine(outside.err,
                 "the mask-membership refusal prints one line (FR-011)");
    check(outside.err.find("stands outside the process affinity mask")
              != std::string::npos,
          "the line names the mask the child holds (FR-011)");
    // The set type's own bound stands in the bit read itself, and the
    // count the threads report refuses after it, on the line of its
    // own: an index the set cannot hold, and the count-as-index that
    // names no cpu the threads run on (FR-011, FR-041).
    auto bound = quickRun;
    bound.push_back("--jitter-probe=" + std::to_string(CPU_SETSIZE));
    checkOneLine(captureError(bound, 2),
                 "a cpu at the set's own bound stands rejected (FR-011)");
    // The online-set bound reads the seam, the way the test layout
    // installs: the installed set holds the mask's first cpu and lacks
    // its second, so the bound refuses the second on the line of its
    // own. The count seam stands at zero beside it and leaves the answer
    // of the cpu the set holds unchanged; the seams clear after, so the
    // cases below read the list the kernel owns (FR-041, FR-011, R-11).
    sg::detail::installTestOnlineCpus(std::vector<std::uint32_t> {cpus.at(0)});
    sg::detail::installTestOnlineCount(0);
    auto pastSet = quickRun;
    pastSet.push_back("--jitter-probe=" + second);
    checkOneLine(captureError(pastSet, 2),
                 "a cpu the installed set lacks stands rejected (FR-041)");
    check(captureError(pastSet, 2).find("stands outside the online cpu set")
              != std::string::npos,
          "the set bound names the online cpu set (FR-041)");
    auto countSeamRun = quickRun;
    countSeamRun.push_back("--jitter-probe=" + cpu);
    const auto countSeam = captureRun(countSeamRun);
    check(countSeam.find("bmQuiet") != std::string::npos,
          "the count seam at zero leaves a cpu the set holds accepted "
          "(FR-041, R-11)");
    sg::detail::installTestOnlineCount({});
    sg::detail::installTestOnlineCpus(std::nullopt);
    auto rangeBeyond = quickRun;
    rangeBeyond.push_back("--jitter-probe=" + cpu + "-" + pickCpuOutsideMask());
    checkOneLine(
        captureError(rangeBeyond, 2),
        "a range that ends outside the mask stands rejected " "(FR-011)");

    // Each end of the range that reads no number stands rejected
    // (FR-011).
    auto lowEnd = quickRun;
    lowEnd.push_back("--jitter-probe=zz-" + cpu);
    checkOneLine(
        captureError(lowEnd, 2),
        "a range whose lower end reads no number stands rejected " "(FR-011)");
    auto highEnd = quickRun;
    highEnd.push_back("--jitter-probe=" + cpu + "-zz");
    checkOneLine(
        captureError(highEnd, 2),
        "a range whose upper end reads no number stands rejected " "(FR-011)");

    auto valuedStrict = quickRun;
    valuedStrict.push_back("--jitter-probe=" + cpu + ",strict=5");
    checkOneLine(captureError(valuedStrict, 2),
                 "a valued strict key stands rejected (FR-011)");
    auto unknown = quickRun;
    unknown.push_back("--jitter-probe=" + cpu + ",bogus=1");
    checkOneLine(captureError(unknown, 2),
                 "a key the value does not name stands rejected (FR-011)");

    // A bare cpu after an `idle` head joins the value as a key-shaped
    // token the key switch refuses (FR-011).
    auto idleExtend = quickRun;
    idleExtend.push_back("--jitter-probe=idle," + cpu);
    checkOneLine(captureError(idleExtend, 2),
                 "a bare cpu after the idle head stands rejected (FR-011)");
  }

  // FR-008, FR-038: the number reads that refuse: the empty argument,
  // the count the conversion overflows, and the trailing text.
  for (const auto* value : {"", "99999999999999999999999", "5x"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",threshold=" + value);
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a bad threshold stands rejected (FR-008, FR-038)");
  }

  // FR-008, FR-038: threshold=0, a negative value, a non-number each
  // stand one line and status 2.
  for (const auto* value : {"0", "-5", "abc"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",threshold=" + value);
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a bad threshold stands rejected (FR-008, FR-038)");
  }

  // FR-009: gaps=0, a value above 16777216, a non-number.
  for (const auto* value : {"0", "16777217", "abc"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",gaps=" + value);
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a bad gaps value stands rejected (FR-009)");
  }

  // FR-014, FR-038: a mode word outside the three, and precheck=0 or a
  // non-number.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",mode=waltz");
    checkOneLine(captureError(arguments, 2),
                 "a mode word outside the three stands rejected (FR-014)");
  }
  for (const auto* value : {"0", "abc"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",precheck=" + value);
    checkOneLine(captureError(arguments, 2),
                 "a bad precheck value stands rejected (FR-014, FR-038)");
  }

  // The option-file surface of the same grammar (FR-027, FR-033): the
  // value parses where it settles, so a file line's refusal names the
  // path and the line the file faults name, the command line's keeps
  // the option's own prefix, a bad file line is read under a good
  // command-line value, and every echoed token stands quoted (R-06).
  {
    const std::string path = "harness_jitter_cli_file.txt";
    const auto write = [&path](const std::string& line)
    {
      std::ofstream file(path);
      file << line << "\n";
    };

    // An empty value on the command line: the head token is empty, so
    // the refusal names the empty token (FR-014).
    checkOneLine(captureError({"--jitter-probe="}, 2),
                 "an empty value stands rejected (FR-014)");

    // The same value from an option file line: the refusal names the
    // path and the line (FR-027).
    write("jitter-probe=");
    {
      auto arguments = quickRun;
      arguments.push_back("--options=" + path);
      const auto note = captureError(arguments, 2);
      check(note.find(path + ":1: --jitter-probe: ") == 0,
            "a file line's refusal names the path and the line (FR-027)");
      check(std::count(note.begin(), note.end(), '\n') == 1,
            "a file line's refusal stands one line (FR-027)");
    }

    // A bad file line under a good command-line value: the line is
    // still read, so its own fault reaches the operator and the run
    // stops before the expansion (FR-024, FR-038).
    {
      auto arguments = quickRun;
      arguments.push_back("--jitter-probe=" + cpu + ",precheck=1");
      arguments.push_back("--options=" + path);
      const auto note = captureError(arguments, 2);
      check(note.find(path + ":1: --jitter-probe: ") == 0,
            "a bad file line is read under a good command-line value, "
            "named with the path and the line (FR-024, FR-027, FR-038)");
      check(std::count(note.begin(), note.end(), '\n') == 1,
            "a bad file line's refusal stands one line (FR-024, FR-038)");
    }

    // A line break inside one token: the refusal stands one line and
    // names the break with its own words (FR-038).
    checkOneLine(captureError({std::string("--jitter-probe=0\n1")}, 2),
                 "a break inside one token stands quoted (FR-038)");

    // A precheck value past the field's own bound: the precheck line
    // stands for it (FR-014).
    for (const auto* value : {"4294967296", "4294967297"}) {
      auto arguments = quickRun;
      arguments.push_back(std::string("--jitter-probe=") + cpu
                          + ",precheck=" + value);
      checkOneLine(
          captureError(arguments, 2),
          "a precheck value past the field's bound stands " "rejected "
                                                            "(FR-014)");
    }

    // A cpu that stands twice in one value, in either form the tokens
    // name: the index and the whole value stand named (FR-011).
    for (const auto* value : {"0,0", "0-1,1"}) {
      auto arguments = quickRun;
      arguments.push_back(std::string("--jitter-probe=") + value);
      checkOneLine(captureError(arguments, 2),
                   "a cpu repeated in one value stands rejected (FR-011)");
    }

    // The same refusal from an option file line (FR-027, FR-038).
    write("jitter-probe=0,0");
    {
      auto arguments = quickRun;
      arguments.push_back("--options=" + path);
      const auto note = captureError(arguments, 2);
      check(
          note.find(path + ":1: --jitter-probe: ") == 0,
          "a file line's repeated cpu names the path and the line " "(FR-027)");
    }
    std::remove(path.c_str());
  }

  // contracts/cli-options.md: `--jitter-probe` with no argument keeps
  // the standing getopt_long missing-argument path, and an unknown flag
  // the standing line; both exit status 2 and no benchmark runs.
  {
    const auto note = captureError({"--jitter-probe"}, 2);
    check(!note.empty(),
          "the missing argument keeps the standing getopt path "
          "(contracts/cli-options.md)");
  }
  {
    const auto note = captureError({"--jitter-probe-nonsense"}, 2);
    check(
        note.find("unknown option") != std::string::npos,
        "an unknown flag keeps the standing line " "(contracts/"
                                                   "cli-options.md)");
  }

  // FR-011, R-06: every value in the option table is unique. The scan
  // reads the parser's own table through the source path the
  // registration hands the binary, the way the schema tests read their
  // schema path.
  {
    std::ifstream source(SG_OPTIONS_SOURCE);
    check(source.is_open(), "the option table stands readable (R-06)");
    std::stringstream buffer;
    buffer << source.rdbuf();
    const auto text = buffer.str();
    const auto tableStart =
        text.find("static constexpr auto kRows = std::to_array<OptionRow>");
    check(tableStart != std::string::npos,
          "the option table stands in the source (R-06)");
    const auto tableEnd = text.find("  });", tableStart);
    check(tableEnd != std::string::npos, "the option table closes (R-06)");

    std::set<std::string> values;
    auto cursor = text.find("{\"", tableStart);
    while (cursor != std::string::npos && cursor < tableEnd) {
      const auto nameStart = cursor + 1;
      const auto nameEnd = text.find('"', nameStart);
      if (nameEnd == std::string::npos) {
        break;
      }
      const auto valueComma = text.find(',', nameEnd);
      if (valueComma == std::string::npos || valueComma > tableEnd) {
        break;
      }
      auto valueEnd = valueComma + 1;
      while (valueEnd < tableEnd && text.at(valueEnd) != ',') {
        ++valueEnd;
      }
      auto value = text.substr(valueComma + 1, valueEnd - valueComma - 1);
      // The token carries the row's internal value: a letter literal or
      // the named constant of the option's block.
      const auto trimStart = value.find_first_not_of(" \t\n\r");
      const auto trimEnd = value.find_last_not_of(" \t\n\r");
      if (trimStart != std::string::npos) {
        value = value.substr(trimStart, trimEnd - trimStart + 1);
        const auto inserted = values.insert(value);
        check(inserted.second,
              "every value in the option table is unique (FR-011, R-06)");
      }
      cursor = text.find("{\"", valueEnd);
    }
    check(values.size() > 15, "the scan read the whole table (FR-011, R-06)");
  }

  // FR-017, D-4, E-09: the skipped-groups line the disclosure carries
  // reads one group alone, two groups joined on the comma, and no
  // group to no line, off the one function the console line and the
  // JSON text read (E-09).
  {
    check(sg::detail::skippedGroupsText({5}) == "skipped L3 group(s) 5",
          "one group names the group alone (E-09)");
    check(sg::detail::skippedGroupsText({3, 9}) == "skipped L3 group(s) 3,9",
          "two groups join on the comma (E-09)");
    check(sg::detail::skippedGroupsText({}).empty(),
          "no group names no line (E-09)");
  }

  // FR-013, SC-003: a run with no probe option prints the console
  // report with the fields and the shape master prints.
  {
    const auto report = captureRun(quickRun);
    check(report.find("bmQuiet") != std::string::npos,
          "the probe-free run prints the benchmark (FR-013, SC-003)");
    check(report.find("jitter probe:") == std::string::npos
              && report.find("jitter: ") == std::string::npos,
          "the probe-free run prints no probe block and no disclosure "
          "(FR-013, SC-003)");
  }

  // FR-012, D-1: the mode machine. No mode= value stands ALONGSIDE.
  // Red against the JSON writer T013 lands: the document carries no
  // jitter scope, so the mode word stands missing.
  {
    const std::string path = "harness_jitter_cli_mode.json";
    auto arguments = quickRun;
    arguments.push_back("--json=" + path);
    arguments.push_back("--jitter-probe=" + cpu + ",precheck=1");
    captureRun(arguments);
    const auto document = readDocument(path);
    std::remove(path.c_str());
    check(document.find("\"mode\":\"ALONGSIDE\"") != std::string::npos,
          "no mode= value stands alongside (FR-012, D-1)");
  }

  // FR-015: the option sources stand in order: the command line's
  // value, then the option file's, then the suite-level call's; the
  // call answers only where no higher source named the probe. The
  // file's value stands, and the mode word the suite call named
  // stands nowhere in the document (FR-015). The scenario runs in a
  // fresh exec, because the suite-level call requires a process that
  // has run no benchmark yet (E-02, FR-015).
  {
    const auto captured =
        runExecChild("--jitter-cli-child=file-over-suite", "fileOverSuite");
    check(captured.status == 0,
          "the run with the file's value and the suite call stands "
          "status 0 (FR-015)");
    const auto document = readDocument("harness_jitter_cli_file2.json");
    std::remove("harness_jitter_cli_file2.json");
    std::remove("harness_jitter_cli_file2.txt");
    check(document.find("\"mode\":\"ALONGSIDE\"") != std::string::npos,
          "the option file's value stands over the suite call's (FR-015)");
  }

  std::puts("harness_jitter_cli_test: ok");
  return 0;
}
