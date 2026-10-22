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

#ifndef SG_CLI_SOURCE
#  error "the registration passes the CLI source path the scan reads (R-06)"
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

// The cpu index the scenarios pin probe threads to, taken from the
// process affinity mask, below the online count (FR-040).
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  const int online = CPU_COUNT(&mask);
  check(online > 0, "the affinity mask holds cpus (FR-040)");
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask)) {
      check(index < static_cast<std::size_t>(online),
            "a picked cpu stands below the online count (FR-040)");
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

// The syscall refusal seam: one seccomp filter that answers the named
// syscall number with EPERM for the rest of the child's life. The
// filter is the injection route the unreadable-mask refusal of the
// parse and the skipped narrowing read carry (FR-011, D-5), the way
// the load-failure seam stands in for the load call's refusal.
auto blockSyscallWithEperm(const int number) -> void
{
  const struct sock_filter program[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,
               static_cast<__u32>(number),
               0,
               1),
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

// The thrawn shape built from the two cpus the mask gives: 4 L3 groups,
// 24 cores, 48 threads, two siblings per core. Cpus `base` and
// `sibling` share one core, so the `l3` plan, which names each group's
// lowest cpu, plans `base`, and the sibling of a planned cpu the FR-018
// line names stands on the second of the two real cpus; group n / 6
// holds the cores 6n through 6n + 5.
auto installThrawnShape(std::uint32_t base, std::uint32_t sibling) -> void
{
  // The 46 filler cpus: the consecutive ones from base + 1 the sibling
  // value leaves free. Core 0 carries the two picked cpus and the
  // fillers fill the other cores, two a core, so the shape spans base
  // through base + 47 whatever the mask holds.
  std::vector<std::uint32_t> fillers;
  for (std::uint32_t cpu = base + 1; fillers.size() < 46; ++cpu) {
    if (cpu != sibling) {
      fillers.push_back(cpu);
    }
  }
  std::vector<std::pair<std::uint32_t, std::uint32_t>> placed;
  placed.emplace_back(base, 0);
  placed.emplace_back(sibling, 0);
  for (std::uint32_t core = 1; core < 24; ++core) {
    placed.emplace_back(fillers.at(2 * (core - 1)), core);
    placed.emplace_back(fillers.at(2 * (core - 1) + 1), core);
  }
  // The thread walk emits the cpus ascending, the order the layout read
  // builds and the resolver's lookups read through (R-07).
  std::ranges::sort(placed);
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 24; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = core / 6;
    installed.cores.push_back(layoutCore);
  }
  for (const auto& [cpu, core] : placed) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = core;
    thread.groupOrdinal = core / 6;
    installed.cores.at(core).threads.push_back(
        static_cast<std::uint32_t>(installed.threads.size()));
    installed.threads.push_back(thread);
    installed.mask.push_back(cpu);
  }
  for (std::uint32_t group = 0; group < 4; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    const auto first = std::ranges::find_if(
        placed,
        [group](const std::pair<std::uint32_t, std::uint32_t>& slot) {
          return slot.second / 6 == group;
        });
    layoutGroup.lowestCpu = first->first;
    for (std::uint32_t core = group * 6; core < group * 6 + 6; ++core) {
      layoutGroup.cores.push_back(core);
    }
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// Two groups of one core with two siblings each: the `l3` plan names
// base and base + 2, and the siblings cover the rest, so no cpu stays
// free (FR-017).
auto installTwoGroupPairsShape(std::uint32_t base) -> void
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = group;
    for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
      const std::uint32_t cpu = base + group * 2 + sibling;
      sg::detail::LayoutThread thread;
      thread.cpuIndex = cpu;
      thread.coreOrdinal = group;
      thread.groupOrdinal = group;
      layoutCore.threads.push_back(
          static_cast<std::uint32_t>(installed.threads.size()));
      installed.threads.push_back(thread);
      installed.mask.push_back(cpu);
    }
    installed.cores.push_back(layoutCore);
  }
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = base + group * 2;
    layoutGroup.cores.push_back(group);
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// The two cpus the layout scenarios build their shapes from, picked
// from the process affinity mask (VI): the planned cpu `baseCpu` and
// its SMT sibling `siblingCpu`, the second of the two the mask holds;
// the statics carry them into the setups the forked children run.
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
  installTwoGroupPairsShape(baseCpu);
  auto handle = sg::registerBenchmark(&bmJitIdleNoFree, "bmJitIdleNoFree");
  handle.threads(2);
}

auto setupOverlap() -> void
{
  installThrawnShape(baseCpu, siblingCpu);
  auto handle = sg::registerBenchmark(&bmJitOverlap, "bmJitOverlap");
  handle.threads(4);
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

auto setupSingle() -> void
{
  auto handle = sg::registerBenchmark(&bmJitSingle, "bmJitSingle");
  handle.placement("l3");
}

auto setupCpusetLimit() -> void
{
  // The child's cpuset holds the two picked cpus, the way a cpuset
  // tighter than the layout stands at the run (FR-036); the shape the
  // child installs spans cpus the cpuset does not hold, so the idle
  // resolution names probe cpus outside it.
  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  CPU_SET(baseCpu, &cpus);
  CPU_SET(siblingCpu, &cpus);
  if (sched_setaffinity(0, sizeof(cpus), &cpus) != 0) {
    fail("the child cannot hold the picked cpuset (FR-036)");
  }
  installThrawnShape(baseCpu, siblingCpu);
  auto handle =
      sg::registerBenchmark(&bmJitLimitPlanned, "bmJitLimitPlanned");
  handle.placement("l3");
  handle.threads(2);
  (void)sg::registerBenchmark(&bmJitLimitProbe, "bmJitLimitProbe");
  // A second unpinned instance: the first unpinned instance's
  // narrowing latches the cpuset answer, and the second's takes the
  // latched short-circuit (FR-036).
  (void)sg::registerBenchmark(&bmJitLimitFree, "bmJitLimitFree");
}

auto setupSkip() -> void
{
  installThrawnShape(baseCpu, siblingCpu);
  auto handle = sg::registerBenchmark(&bmJitSkip, "bmJitSkip");
  handle.threads(13);
}

auto bmJitPlannedNone(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmJitPlannedL3(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The idle resolution's plan walk skips the multi-context instance
// whose preset names none (FR-017), and the narrowing narrows the
// calling thread's mask for it (D-5).
auto setupMultiIdle() -> void
{
  auto none = sg::registerBenchmark(&bmJitPlannedNone, "bmJitPlannedNone");
  none.threads(2);
  auto placed = sg::registerBenchmark(&bmJitPlannedL3, "bmJitPlannedL3");
  placed.placement("l3");
  placed.threads(2);
}

auto bmJitSkip25(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The `compact` plan of twenty-five contexts covers groups 0 and 1 of
// the thrawn shape fully and one cpu of group 2, so the idle
// resolution skips two groups and the disclosure names both (FR-017,
// D-4).
auto setupSkipTwoGroups() -> void
{
  auto handle = sg::registerBenchmark(&bmJitSkip25, "bmJitSkip25");
  handle.placement("compact");
  handle.threads(25);
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

// FR-011, FR-017: the idle value parses, and the resolution over a run
// whose instances carry no plan names no free cpu: one line naming the
// preset word and status 2. The child installs no layout, so the
// resolution answers the host read its own first read caches.
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
  check(run.out.find("bmJitOverlap") != std::string::npos,
        "the benchmark ran under the precheck allowance (D-3)");
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
  const Captured run = runChildScenario("singleContext",
                                        {"--filter",
                                         "^bmJitSingle$",
                                         "--repetitions=1",
                                         "--iterations=2",
                                         "--jitter-probe=" + probe},
                                        &setupSingle);
  check(run.status == 0,
        "the single-context preset run continues (FR-019)");
  check(run.out.find("affinity mask narrowed for 'bmJitSingle'")
            != std::string::npos,
        "the disclosure names the narrowed mask (FR-019, D-5)");
  check(run.out.find("bmJitSingle") != std::string::npos,
        "the benchmark ran (FR-019)");
  return 0;
}

// FR-017, D-4: a group with no free cpu stands skipped and disclosed,
// and the run continues over the free cpu the other touched group
// yields; the `compact` plan of thirteen contexts covers group 0
// fully and names group 1's lowest cpu.
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
  check(run.status == 0, "a skipped group continues the run (FR-017)");
  check(run.out.find("skipped L3 group(s) 0") != std::string::npos,
        "the disclosure names the skipped group (FR-017, D-4)");
  check(run.out.find("bmJitSkip") != std::string::npos,
        "the benchmark ran (FR-017)");
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
                                         "--jitter-probe=idle"},
                                        &setupCpusetLimit);
  check(run.status == 0, "the cpuset limit discloses and continues (FR-036)");
  check(run.out.find("cpusets limit the probe cpus") != std::string::npos,
        "the disclosure names the cpuset limit (FR-036, D-12)");
  check(run.out.find("bmJitLimitProbe") != std::string::npos,
        "the second benchmark ran (FR-036)");
  return 0;
}

// FR-017, D-4: the plan walk skips the multi-context instance whose
// preset names none, and the narrowing narrows the calling thread's
// mask for that unpinned instance, whose name the disclosure carries
// (D-5).
auto scenarioMultiIdle() -> int
{
  const Captured run = runChildScenario("multiIdle",
                                        {"--filter=^bmJitPlanned",
                                         "--repetitions=1",
                                         "--iterations=2",
                                         "--jitter-probe=idle"},
                                        &setupMultiIdle);
  check(run.status == 0, "the preset-none instance continues the run (D-5)");
  check(run.out.find("affinity mask narrowed for 'bmJitPlannedNone")
            != std::string::npos,
        "the disclosure names the narrowed mask (FR-019, D-5)");
  check(run.out.find("bmJitPlannedNone") != std::string::npos,
        "the unpinned instance ran (D-5)");
  return 0;
}

// FR-011, D-5: the parse's unreadable-mask refusal answers one line
// and status 2, and the narrowing that skips its own read lets the run
// continue over the full mask. The refusal filter stands in the child
// before the entry point reads the value (FR-011).
auto scenarioUnreadableMask() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run =
      runChildScenario("unreadableMask",
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

auto scenarioSkipTwoGroups() -> int
{
  const Captured run = runChildScenario(
      "skipTwoGroups",
      {"--filter=^bmJitSkip25",
       "--repetitions=1",
       "--iterations=2",
       "--jitter-probe=idle"},
      &setupSkipTwoGroups);
  check(run.status == 0, "the two skipped groups continue the run (FR-017)");
  check(run.out.find("skipped L3 group(s) 0,1") != std::string::npos,
        "the disclosure names both skipped groups (FR-017, D-4)");
  check(run.out.find("bmJitSkip25") != std::string::npos,
        "the benchmark ran (FR-017)");
  return 0;
}

// FR-022, R-08: the fit of the family that ended prints between the
// rows of two families, behind the join edge the probe owns, so the
// probed run defers every table to the stop (R-08).
auto scenarioFitBesideProbe() -> int
{
  const std::string probe = std::to_string(baseCpu);
  const Captured run = runChildScenario(
      "fitBesideProbe",
      {"--filter=^bmFit",
       "--repetitions=1",
       "--iterations=2",
       "--jitter-probe=" + probe},
      &setupFitFamilies);
  check(run.status == 0, "the two families run beside the probe (R-08)");
  check(run.out.find("bmFitA") != std::string::npos
            && run.out.find("bmFitB") != std::string::npos,
        "the batch carries both families (FR-022)");
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
  check(run.out.find("not available") != std::string::npos,
        "the run discloses the unavailable answer (FR-037)");
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

// FR-037, D-10: the same unavailable answer under `strict` stops the
// batch before any run, one line and status 2.
auto scenarioUnavailableStrict() -> int
{
  const Captured run = runExecChild(
      "--jitter-cli-child=unavailable-strict", "unavailable-strict");
  check(run.status == 2, "the strict refusal stops the batch (FR-037)");
  checkOneLine(run.err, "the strict refusal prints one line (FR-037)");
  check(run.err.find("the machine/tsc leaf is not available")
            != std::string::npos,
        "the line names the unavailable leaf (FR-037)");
  check(run.out.find("bmQuiet") == std::string::npos,
        "no benchmark ran (FR-037)");
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
    auto redirect = [](const char* tag) -> bool {
      const std::string outPath =
          std::string("harness_jitter_cli_") + tag + "-out.txt";
      const std::string errPath =
          std::string("harness_jitter_cli_") + tag + "-err.txt";
      return std::freopen(outPath.c_str(), "w", stdout) != nullptr
          && std::freopen(errPath.c_str(), "w", stderr) != nullptr;
    };
    if (mode == "--jitter-cli-child=unavailable" && redirect("unavailable")) {
      return childUnavailable(false);
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
  check(scenarioBothOverlap() == 0, "bothOverlap");
  check(scenarioMaskEmpty() == 0, "maskEmpty");
  check(scenarioSingleContext() == 0, "singleContext");
  check(scenarioSkipDisclosure() == 0, "skipDisclosure");
  check(scenarioCpusetLimit() == 0, "cpusetLimit");
  check(scenarioMultiIdle() == 0, "multiIdle");
  check(scenarioUnreadableMask() == 0, "unreadableMask");
  check(scenarioSkipTwoGroups() == 0, "skipTwoGroups");
  check(scenarioFitBesideProbe() == 0, "fitBesideProbe");
  check(scenarioUnavailableProbe() == 0, "unavailableProbe");
  check(scenarioUnavailableStrict() == 0, "unavailableStrict");

  // FR-011: the cpu list, `idle`, and every key parse once, each key
  // standing once in the value; the accepted value proceeds into the
  // batch.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",threshold=500,"
                        "mode=alongside,precheck=50,gaps=4,strict");
    const auto report = captureRun(arguments);
    check(report.find("bmQuiet") != std::string::npos,
          "the accepted value runs the batch (FR-011)");
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
    accepted.push_back("--jitter-probe=" + cpu + "," + second);
    check(captureRun(accepted).find("bmQuiet") != std::string::npos,
          "a bare cpu extension joins the head's list (FR-011)");
    auto range = quickRun;
    range.push_back("--jitter-probe=" + cpu + "-" + second);
    check(captureRun(range).find("bmQuiet") != std::string::npos,
          "a closed cpu range joins both cpus (FR-011)");

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
    const std::string outside = pickCpuOutsideMask();
    auto beyond = quickRun;
    beyond.push_back("--jitter-probe=" + outside);
    checkOneLine(captureError(beyond, 2),
                 "a cpu outside the mask stands rejected (FR-011)");
    auto rangeBeyond = quickRun;
    rangeBeyond.push_back("--jitter-probe=" + cpu + "-" + outside);
    checkOneLine(captureError(rangeBeyond, 2),
                 "a range that ends outside the mask stands rejected "
                 "(FR-011)");

    // Each end of the range that reads no number stands rejected
    // (FR-011).
    auto lowEnd = quickRun;
    lowEnd.push_back("--jitter-probe=zz-" + cpu);
    checkOneLine(captureError(lowEnd, 2),
                 "a range whose lower end reads no number stands rejected "
                 "(FR-011)");
    auto highEnd = quickRun;
    highEnd.push_back("--jitter-probe=" + cpu + "-zz");
    checkOneLine(captureError(highEnd, 2),
                 "a range whose upper end reads no number stands rejected "
                 "(FR-011)");

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
    std::ifstream source(SG_CLI_SOURCE);
    check(source.is_open(), "the CLI source stands readable (R-06)");
    std::stringstream buffer;
    buffer << source.rdbuf();
    const auto text = buffer.str();
    const auto tableStart = text.find("const option kLongOptions[]");
    check(tableStart != std::string::npos,
          "the option table stands in the source (R-06)");
    const auto tableEnd = text.find("\n};", tableStart);
    check(tableEnd != std::string::npos, "the option table closes (R-06)");

    std::set<std::string> values;
    auto cursor = text.find('{', tableStart);
    while (cursor != std::string::npos && cursor < tableEnd) {
      const auto entryEnd = text.find('}', cursor);
      if (entryEnd == std::string::npos || entryEnd > tableEnd) {
        break;
      }
      const auto entry = text.substr(cursor, entryEnd - cursor);
      const auto nameStart = entry.find('"');
      const auto nameEnd = entry.find('"', nameStart + 1);
      if (nameStart != std::string::npos && nameEnd != std::string::npos) {
        const auto name = entry.substr(nameStart, nameEnd - nameStart);
        if (name.empty()) {
          // The terminator's entry: {"nullptr", 0, nullptr, 0}.
          break;
        }
        const auto valueStart = entry.rfind(", ");
        check(valueStart != std::string::npos,
              "the entry carries its value (R-06)");
        const auto value = entry.substr(valueStart + 2);
        const auto inserted = values.insert(value);
        check(inserted.second,
              "every value in the option table is unique (FR-011, R-06)");
      }
      cursor = text.find('{', entryEnd + 1);
    }
    check(values.size() > 15, "the scan read the whole table (FR-011, R-06)");
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
    arguments.push_back("--jitter-probe=" + cpu);
    captureRun(arguments);
    const auto document = readDocument(path);
    std::remove(path.c_str());
    check(document.find("\"mode\":\"ALONGSIDE\"") != std::string::npos,
          "no mode= value stands alongside (FR-012, D-1)");
  }

  std::puts("harness_jitter_cli_test: ok");
  return 0;
}
