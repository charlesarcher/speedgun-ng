// ============================================================================
// specs/034 T005, T007, T009, T011, T013, T015, T018: the host noise
// pre-check. The file-root seam (FR-019, R-02), the measured set
// (FR-010, R-01), the serial windows and the mode matrix (FR-007,
// FR-009, R-06), the strict refusal (FR-014, R-05), the warnings and
// the pinning line (FR-012, FR-013), the attribution deltas and the
// host settings over fake files (FR-016 to FR-018, R-03, R-04), and
// the JSON record (FR-006, FR-024, R-08). Frameworkless check()/fail()
// convention, the way harness_jitter_probe_test.cpp carries it. No
// test sleeps and no test reads a clock: the windows close through the
// scripted reader of `installTestReader`, and the attribution deltas
// come from the fake files (FR-011, VI).
// ============================================================================

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/jitter_probe.hpp"
#include "detail/proc_root.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PRECHECK TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// T005: the seam's root stands restored when the scenario ends, the
// way the standing install/clear seams stand cleared by the test.
class ProcRootGuard
{
public:
  explicit ProcRootGuard(const std::filesystem::path& root)
  {
    sg::detail::installTestProcRoot(root);
  }
  ~ProcRootGuard() { sg::detail::installTestProcRoot(std::nullopt); }
  ProcRootGuard(const ProcRootGuard&) = delete;
  auto operator=(const ProcRootGuard&) -> ProcRootGuard& = delete;
};

// T005: one unique fake root per run, under the working directory the
// test binaries own, named with the pid so parallel runs stand apart.
auto fakeRoot() -> std::filesystem::path
{
  const auto path = std::filesystem::path("harness_precheck_fake_root_")
    / ("pid" + std::to_string(::getpid()));
  const bool created = std::filesystem::create_directories(path);
  check(created || std::filesystem::is_directory(path),
        "the fake root stands created (T005)");
  return path;
}

// T007: the synthetic layouts the measured-set scenarios run over, the
// shape the standing preset tests install (T007, FR-010, R-01).
auto twoGroupLayout() -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout machine;
  machine.readable = true;
  machine.packages = 1;
  for (std::uint32_t core = 0; core < 4; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = core / 2;
    sg::detail::LayoutThread thread;
    thread.cpuIndex = core;
    thread.coreOrdinal = core;
    thread.groupOrdinal = core / 2;
    layoutCore.threads.push_back(core);
    machine.threads.push_back(thread);
    machine.mask.push_back(core);
    machine.cores.push_back(layoutCore);
  }
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * 2;
    layoutGroup.cores = {group * 2U, group * 2U + 1U};
    machine.groups.push_back(layoutGroup);
  }
  return machine;
}

// One group of two sibling cpus: the planned cpu names the sibling, so
// the group stands no free cpu for a candidate (T007, R-01).
auto oneGroupSmtLayout() -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout machine;
  machine.readable = true;
  machine.packages = 1;
  sg::detail::LayoutCore layoutCore;
  layoutCore.groupOrdinal = 0;
  for (const auto cpu : {0U, 1U}) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpu;
    thread.coreOrdinal = 0;
    thread.groupOrdinal = 0;
    layoutCore.threads.push_back(cpu);
    machine.threads.push_back(thread);
    machine.mask.push_back(cpu);
  }
  machine.cores.push_back(layoutCore);
  sg::detail::LayoutGroup layoutGroup;
  layoutGroup.lowestCpu = 0;
  layoutGroup.cores.push_back(0);
  machine.groups.push_back(layoutGroup);
  return machine;
}

// T009: the per-thread stamp series the serial scenarios read through
// the reader-install seam (R-02, R-8 of research.md): each thread's
// series stands one read per call, and the reads' 1025th, the first
// read the pre-check phase takes, jumps one billion ns. The parallel
// shape reads the jump as its window's first delta and keeps it; the
// serial shape re-reads the jump as its window's own baseline and
// keeps none, so the two shapes stand apart with no sleep and no
// clock (FR-009, FR-011, R-06).
class SerialProbeReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    auto& thread = threadState();
    const auto index = thread.reads++;
    thread.stamp += index == 1024 ? kJump : 1U;
    sink.put(thread.stamp);
  }

  static inline constexpr std::uint64_t kJump = 1000000000ULL;

private:
  struct ThreadState
  {
    std::uint64_t reads = 0;
    std::uint64_t stamp = 1000000ULL;
  };
  [[nodiscard]] static auto threadState() -> ThreadState&
  {
    thread_local ThreadState state;
    return state;
  }
};

// The scripted provider: one FakeProvider declaring the machine/tsc
// leaf at ReadMode::FAST_TSC (R-02), registered once per process, the
// way harness_jitter_probe_test.cpp carries it.
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

// The cpu indexes the scenarios pin probe threads to, taken from the
// process affinity mask, the way harness_jitter_probe_test.cpp carries
// the pick (VI). No cpu number stands fixed in this file.
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (VI)");
  }
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index) {
    if (CPU_ISSET(index, &mask)) {
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (VI)");
  return cpus;
}

// The benchmark the full-run refusal scenario pins: more than one
// context and a preset, the planned-cpu rule's own shape (T011, the
// plannedCpusOf rule of specs/027).
auto bmPinned(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}


// T013: the console capture the block tests read: the printer's own
// bytes, taken through the redirect the standing capture carries.
auto capturePrecheck(const sg::PrecheckRecord& record) -> std::string
{
  const char* const path = "harness_precheck_block.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0
      || std::freopen(path, "w", stdout) == nullptr)
  {
    fail("cannot redirect the pre-check block");
  }
  sg::detail::printPrecheckReport(record);
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path);
  return buffer.str();
}

// T013: one record the block tests read: two planned cpus, the gap
// rates standing apart so the rank stands unambiguous, and one
// candidate cpu.
auto noisyRecord(std::vector<std::uint32_t> cpus)
    -> sg::PrecheckRecord
{
  sg::PrecheckRecord record;
  for (const auto cpu : cpus) {
    sg::PrecheckCpuRecord entry;
    entry.cpu = cpu;
    entry.planned = true;
    entry.stats.gapCount = 1;
    entry.stats.gapsPerSecond = cpu == cpus.front() ? 12.5 : 3.25;
    entry.stats.maxNs = 1000 + cpu;
    record.cpus.push_back(entry);
  }
  return record;
}

}  // namespace

// T011: the exec'd child of the full-run refusal scenario: the layout
// install stands before the child's own first layout read, so the
// cached read answers the install, and the run's refusal stands the
// exit status the parent checks (T011, FR-014).
auto runStrictRefusalChild() -> int
{
  cpu_set_t held {};
  CPU_ZERO(&held);
  if (sched_getaffinity(0, sizeof(held), &held) != 0) {
    fail("the process affinity mask is unreadable (FR-021)");
  }
  std::vector<std::uint32_t> maskCpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && maskCpus.size() < 4;
       ++index)
  {
    if (CPU_ISSET(index, &held) != 0) {
      maskCpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(maskCpus.size() == 4,
        "the mask holds the four cpus the scenario asks for (FR-021)");
  sg::detail::MachineLayout machine;
  machine.readable = true;
  machine.packages = 1;
  for (std::uint32_t core = 0; core < 4; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = core / 2;
    sg::detail::LayoutThread thread;
    thread.cpuIndex = maskCpus.at(core);
    thread.coreOrdinal = core;
    thread.groupOrdinal = core / 2;
    layoutCore.threads.push_back(maskCpus.at(core));
    machine.threads.push_back(thread);
    machine.mask.push_back(maskCpus.at(core));
    machine.cores.push_back(layoutCore);
  }
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = maskCpus.at(group * 2);
    layoutGroup.cores = {group * 2U, group * 2U + 1U};
    machine.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(machine);
  auto handle = sg::registerBenchmark(&bmPinned, "bmPinned");
  handle.threads(2).placement("l3");
  sg::detail::installTestReader(std::make_unique<SerialProbeReader>());
  if (std::freopen("harness_precheck_refusal_out.txt", "w", stdout) == nullptr
      || std::freopen("harness_precheck_refusal_err.txt", "w", stderr)
          == nullptr)
  {
    return 1;
  }
  const std::vector<std::string> arguments {
      "--filter=bmPinned",
      "--repetitions=1",
      "--iterations=2",
      "--jitter-probe=idle,mode=precheck,strict"};
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_precheck_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  return status;
}

auto main(int argc, char** argv) -> int
{
  scriptedProvider();
  if (argc > 1 && std::string(argv[1]) == "strict-refusal") {
    return runStrictRefusalChild();
  }
  // T009, FR-009, R-06: the serial shape re-reads the jump as its own
  // baseline, so neither cpu keeps the gap; the parallel shape keeps
  // one gap per cpu (the standing shape, re-pinned beside the serial
  // one).
  {
    const auto cpus = pickCpus(2);
    sg::JitterProbeOptions options;
    options.cpus = cpus;
    options.mode = sg::JitterProbeMode::PRECHECK;
    options.precheckMs = 1;
    options.precheckSerial = true;
    sg::detail::installTestReader(std::make_unique<SerialProbeReader>());
    sg::detail::JitterProbe probe(options);
    probe.start();
    sg::detail::installTestReader(nullptr);
    const auto& states = probe.states();
    check(states.size() == 2, "the serial run measures both cpus (FR-009)");
    for (const auto& state : states) {
      check(state.kept == 0,
            "the serial window re-reads the jump as its baseline "
            "(FR-009, R-06)");
    }
  }
  {
    const auto cpus = pickCpus(2);
    sg::JitterProbeOptions options;
    options.cpus = cpus;
    options.mode = sg::JitterProbeMode::PRECHECK;
    options.precheckMs = 1;
    sg::detail::installTestReader(std::make_unique<SerialProbeReader>());
    sg::detail::JitterProbe probe(options);
    probe.start();
    sg::detail::installTestReader(nullptr);
    for (const auto& state : probe.states()) {
      check(state.kept == 1,
            "the parallel window keeps the jump once per cpu (FR-009)");
    }
  }

  // T011, FR-014, FR-015, R-05: the noisy rule stands one function
  // over the joined states and the planned list: a planned cpu stands
  // noisy when its pre-check window kept at least one above-threshold
  // gap, and a cpu the planned list does not name never stands in the
  // answer.
  {
    const auto cpus = pickCpus(2);
    sg::JitterProbeOptions options;
    options.cpus = cpus;
    options.mode = sg::JitterProbeMode::PRECHECK;
    options.precheckMs = 1;
    sg::detail::installTestReader(std::make_unique<SerialProbeReader>());
    sg::detail::JitterProbe probe(options);
    probe.start();
    sg::detail::installTestReader(nullptr);
    const auto noisy =
        sg::detail::precheckNoisyCpus(probe.states(), cpus);
    check(noisy == cpus,
          "the parallel window stands both planned cpus noisy (FR-014)");
    const auto one = sg::detail::precheckNoisyCpus(
        probe.states(), std::vector<std::uint32_t> {cpus.at(1)});
    check(one == std::vector<std::uint32_t> {cpus.at(1)},
          "a cpu the planned list does not name stands no candidate "
          "noise (FR-015)");
    const auto none = sg::detail::precheckNoisyCpus(
        probe.states(), std::vector<std::uint32_t> {});
    check(none.empty(),
          "a planned list that names no cpu stands none noisy (FR-014)");
  }
  {
    const auto cpus = pickCpus(2);
    sg::JitterProbeOptions options;
    options.cpus = cpus;
    options.mode = sg::JitterProbeMode::PRECHECK;
    options.precheckMs = 1;
    options.precheckSerial = true;
    sg::detail::installTestReader(std::make_unique<SerialProbeReader>());
    sg::detail::JitterProbe probe(options);
    probe.start();
    sg::detail::installTestReader(nullptr);
    const auto noisy =
        sg::detail::precheckNoisyCpus(probe.states(), cpus);
    check(noisy.empty(),
          "the serial window stands no planned cpu noisy (FR-014, R-06)");
  }

  // T011, FR-014: the full-run refusal: a strict run whose planned cpu
  // stands noisy stops before any benchmark starts, one line on
  // standard error naming the option and the cpu, the pre-check block
  // unprinted. The scenario runs on a mask that holds the four cpus
  // the synthetic layout names, the standing skip the small-mask
  // hosts take (FR-021).
  {
    cpu_set_t held {};
    CPU_ZERO(&held);
    check(sched_getaffinity(0, sizeof(held), &held) == 0,
          "the process affinity mask is readable (FR-021)");
    if (CPU_COUNT(&held) < 4) {
      std::printf("the full-run refusal scenario: SKIPPED (the mask "
                  "holds fewer than four cpus)\n");
    } else {
      // The scenario runs in a fresh exec of this binary: layout()
      // caches the read once per process, and the parent's scenarios
      // read the host layout before this one stands installed, so the
      // exec'd child installs its layout before its own first read,
      // the pattern harness_jitter_cli_test.cpp carries (T011).
      const std::string outPath = "harness_precheck_refusal_out.txt";
      const std::string errPath = "harness_precheck_refusal_err.txt";
      const pid_t pid = fork();
      check(pid >= 0, "the fork stands (T011)");
      if (pid == 0) {
        const auto self = std::filesystem::path("/proc/self/exe");
        std::vector<char*> childArgv {const_cast<char*>(self.c_str()),
                                      const_cast<char*>("strict-refusal"),
                                      nullptr};
        execv(self.c_str(), childArgv.data());
        std::_Exit(127);
      }
      int childStatus = 0;
      waitpid(pid, &childStatus, 0);
      check(WIFEXITED(childStatus) && WEXITSTATUS(childStatus) == 2,
            "the strict refusal exits 2 before any benchmark (FR-014)");
      auto read = [](const std::string& path)
      {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::remove(path.c_str());
        return buffer.str();
      };
      const auto refusalOut = read(outPath);
      const auto refusalErr = read(errPath);
      // The coverage build's profiler prints one bookkeeping line of
      // its own into the exec'd child's standard error; the run's own
      // refusal stands the one line the contract counts, so the
      // profiler's lines stand out of the count (FR-014, the exec-self
      // pattern the CLI tests carry).
      std::size_t refusalLines = 0;
      std::size_t scan = 0;
      while (scan <= refusalErr.size()) {
        const auto stop = refusalErr.find('\n', scan);
        const auto line = refusalErr.substr(
            scan, stop == std::string::npos ? std::string::npos
                                            : stop - scan);
        if (!line.empty()
            && line.find("libgcov profiling error:") != 0)
        {
          ++refusalLines;
        }
        if (stop == std::string::npos) {
          break;
        }
        scan = stop + 1;
      }
      if (refusalLines != 1) {
        std::fprintf(stderr,
                     "the refusal's standard error stands <%s>\n",
                     refusalErr.c_str());
        fail("the strict refusal stands one line (FR-014)");
      }
      check(refusalErr.find("--jitter-probe: cpu ") == 0,
            "the refusal names the option and the cpu (FR-014)");
      check(refusalOut.find("bmPinned") == std::string::npos,
            "no benchmark row prints beside the refusal (FR-014)");
    }
  }

  // T007, FR-010, R-01: the measured set carries the planned cpus
  // ascending, then the candidates ascending, the candidates the
  // standing idle resolution takes over the plans; a skipped group
  // stands carried beside (T007, FR-010, R-01).
  {
    const auto set = sg::detail::precheckMeasuredSet(
        twoGroupLayout(), std::vector<std::uint32_t> {2, 0});
    const std::vector<std::uint32_t> expected {0, 2, 1, 3};
    check(set.cpus == expected,
          "the measured set carries planned ascending then candidates "
          "ascending (FR-010, R-01)");
    check(set.skippedGroups.empty(),
          "a plan that leaves cpus free skips no group (FR-010)");
  }
  {
    const auto set = sg::detail::precheckMeasuredSet(
        twoGroupLayout(), std::vector<std::uint32_t> {0, 1, 2, 3});
    check(set.cpus == std::vector<std::uint32_t> {0, 1, 2, 3},
          "a plan that names every cpu adds no candidate (FR-010)");
    check(set.skippedGroups == std::vector<std::uint32_t> {0, 1},
          "the touched groups with no free cpu stand skipped (FR-010)");
  }
  {
    const auto set = sg::detail::precheckMeasuredSet(
        oneGroupSmtLayout(), std::vector<std::uint32_t> {0});
    check(set.cpus == std::vector<std::uint32_t> {0},
          "an SMT sibling of a planned cpu stands no candidate (FR-010)");
    check(set.skippedGroups == std::vector<std::uint32_t> {0},
          "the skipped group stands carried (FR-010)");
  }
  {
    const auto set = sg::detail::precheckMeasuredSet(
        twoGroupLayout(), std::vector<std::uint32_t> {});
    check(set.cpus.empty(),
          "a plan-free set stands empty for the caller's refusal "
          "(FR-010)");
  }

  // T005, FR-019, R-02: the seam installs a root, `procRootPath`
  // resolves under it, and the default stands restored at the guard's
  // scope exit.
  {
    const auto root = fakeRoot();
    const ProcRootGuard guard(root);
    const auto resolved =
        sg::detail::procRootPath("proc/interrupts");
    check(resolved == root / "proc" / "interrupts",
          "the installed root resolves the pre-check reads (FR-019)");
  }
  {
    const auto restored = sg::detail::procRootPath("proc/interrupts");
    check(restored == std::filesystem::path("/proc/interrupts"),
          "the default root stands restored after the clear (FR-019)");
  }

  // T013, FR-012, FR-013, SC-002: the console block's warning lines:
  // one line per noisy planned cpu naming the cpu and the two figures,
  // in cpu order, the pinning line after the warnings naming every
  // planned cpu with its gap rate, lowest first, tied cpus in cpu
  // order; a run whose planned cpus stand quiet prints no warning.
  {
    const auto cpus = pickCpus(2);
    auto record = noisyRecord(cpus);
    const auto block = capturePrecheck(record);
    const auto firstWarning = block.find(
        "warning: cpu " + std::to_string(cpus.at(0))
        + " gapsPerSecond 12.50 maxNs " + std::to_string(1000 + cpus.at(0)));
    const auto secondWarning = block.find(
        "warning: cpu " + std::to_string(cpus.at(1))
        + " gapsPerSecond 3.25 maxNs " + std::to_string(1000 + cpus.at(1)));
    check(firstWarning != std::string::npos
              && secondWarning != std::string::npos,
          "each noisy planned cpu stands one warning line with its "
          "figures (FR-012)");
    check(firstWarning < secondWarning,
          "the warnings stand in cpu order (FR-012)");
    const auto pinning = block.find(
        "pinning candidates: cpu " + std::to_string(cpus.at(1))
        + " 3.25 cpu " + std::to_string(cpus.at(0)) + " 12.50");
    check(pinning != std::string::npos,
          "the pinning line names every planned cpu lowest rate first "
          "(FR-013)");
    check(secondWarning < pinning,
          "the pinning line stands after the warnings (R-07)");
    check(block.find("precheck candidates: none") != std::string::npos,
          "a record whose cpus stand all planned states none (FR-010)");

    sg::PrecheckCpuRecord candidate;
    candidate.cpu = cpus.at(0) + 7;
    record.cpus.push_back(candidate);
    const auto withCandidate = capturePrecheck(record);
    check(withCandidate.find("warning: cpu "
                             + std::to_string(candidate.cpu))
              == std::string::npos,
          "a candidate never stands a warning line (FR-015)");
    check(withCandidate.find("pinning candidates: cpu "
                             + std::to_string(candidate.cpu))
              == std::string::npos,
          "the pinning line names no candidate (FR-013)");
  }
  {
    const auto cpus = pickCpus(2);
    auto record = noisyRecord(cpus);
    for (auto& entry : record.cpus) {
      entry.stats.gapCount = 0;
    }
    const auto block = capturePrecheck(record);
    check(block.find("warning:") == std::string::npos,
          "a run whose planned cpus stand quiet prints no warning "
          "(FR-012, SC-002)");
    check(block.find("pinning candidates:") != std::string::npos,
          "the quiet run still ranks its planned cpus (FR-013)");
    check(block.find("precheck candidates: none") != std::string::npos,
          "a record without candidates states none (FR-010)");
  }

  // T015, FR-016, R-03, R-4 to R-6 of research.md: the attribution
  // deltas over fake files through the seam: the interrupt delta the
  // sum of the cpu column over every row, one softirq delta per kind
  // the file states, the schedstat mapping naming the run-queue delay
  // figure the switch figure and the timeslices figure the schedule
  // count, and a source that stands missing, unreadable or unparsable
  // stands the figure absent for that source alone.
  {
    const auto rootDir = fakeRoot();
    const ProcRootGuard guard(rootDir);
    const auto procDir = rootDir / "proc";
    std::filesystem::create_directories(procDir);
    const auto write = [](const std::filesystem::path& path,
                          const std::string& text)
    {
      std::ofstream file(path);
      check(static_cast<bool>(file), "the fake file opens (T015)");
      file << text;
    };
    const auto cpus = pickCpus(2);
    const auto cpu0 = cpus.at(0);
    const auto cpu1 = cpus.at(1);
    write(procDir / "interrupts",
          " 0: " + std::to_string(10) + " " + std::to_string(20)
              + "\n 1: 1 2\nNMI: 3 4\n");
    write(procDir / "softirqs",
          "  TIMER: 5 6\n  NET_RX: 7 8\n");
    write(procDir / "schedstat",
          "cpu" + std::to_string(cpu0) + " 1 2 3\ncpu"
              + std::to_string(cpu1) + " 4 5 6\n");
    const auto before = sg::detail::precheckReadSnapshot(cpus);
    write(procDir / "interrupts",
          " 0: 15 30\n 1: 1 2\nNMI: 3 6\n");
    write(procDir / "softirqs",
          "  TIMER: 8 11\n  NET_RX: 7 8\n");
    write(procDir / "schedstat",
          "cpu" + std::to_string(cpu0) + " 1 4 7\ncpu"
              + std::to_string(cpu1) + " 4 5 9\n");
    const auto after = sg::detail::precheckReadSnapshot(cpus);
    const auto delta0 = sg::detail::precheckAttributionDelta(
        before.at(cpu0), after.at(cpu0));
    check(delta0.interrupts.has_value() && *delta0.interrupts == 5,
          "the interrupt delta stands the cpu column sum (FR-016)");
    const auto delta1 = sg::detail::precheckAttributionDelta(
        before.at(cpu1), after.at(cpu1));
    check(delta1.interrupts.has_value() && *delta1.interrupts == 12,
          "the second cpu's interrupt delta stands its own column "
          "(FR-016)");
    check(delta0.softirqs.size() == 2
              && delta0.softirqs.at(0).first == "TIMER"
              && delta0.softirqs.at(0).second.has_value()
              && *delta0.softirqs.at(0).second == 3
              && delta0.softirqs.at(1).first == "NET_RX"
              && delta0.softirqs.at(1).second.has_value()
              && *delta0.softirqs.at(1).second == 0,
          "one softirq delta per kind the file states, the kind named "
          "as the file states it (FR-016)");
    check(delta0.switches.has_value() && *delta0.switches == 2
              && delta0.schedules.has_value() && *delta0.schedules == 4,
          "the schedstat mapping stands the delay figure the switch "
          "delta and the timeslices figure the schedule delta "
          "(FR-016)");
    check(delta1.switches.has_value() && *delta1.switches == 0
              && delta1.schedules.has_value() && *delta1.schedules == 3,
          "each cpu's schedstat line stands its own deltas (FR-016)");
  }
  {
    // The root stands cleared before the read: the fake root names the
    // pid, so the scenarios of one process share the directory, and a
    // missing-source read needs the files the earlier scenario wrote
    // gone (T005).
    const auto rootDir = fakeRoot();
    std::filesystem::remove_all(rootDir);
    std::filesystem::create_directories(rootDir);
    const ProcRootGuard guard(rootDir);
    const auto cpus = pickCpus(1);
    const auto before = sg::detail::precheckReadSnapshot(cpus);
    const auto& state = before.at(cpus.at(0));
    check(!state.interrupts.has_value()
              && !state.switches.has_value()
              && !state.schedules.has_value(),
          "a missing source stands the figure absent (FR-018)");
    const auto host = sg::detail::precheckReadHostSettings(cpus);
    check(!host.isolcpus.has_value() && !host.nohzFull.has_value()
              && !host.rcuNocbs.has_value(),
          "a missing command line stands the settings unstated "
          "(FR-017)");
  }

  // T015, FR-018: every unparsable shape stands the figure absent for
  // the source alone: an interrupt row that names fewer columns than
  // the cpu asks for, a column that parses as no number, a softirq
  // kind row beside a blank kind line and a kind whose column stands
  // short, a schedstat line whose figure parses as no number, a cpu
  // list that names an empty token, a lone token no number, a span
  // that stands reversed, a mask token past eight digits, and the
  // delta that stands absent when one side stands absent.
  {
    const auto rootDir = fakeRoot();
    const ProcRootGuard guard(rootDir);
    const auto procDir = rootDir / "proc";
    std::filesystem::create_directories(procDir);
    const auto write = [](const std::filesystem::path& path,
                          const std::string& text)
    {
      std::ofstream file(path);
      check(static_cast<bool>(file), "the fake file opens (T015)");
      file << text;
    };
    const auto cpus = pickCpus(2);
    const auto cpu1 = cpus.at(1);
    write(procDir / "interrupts", " 0: 10 20\n 1: 1\n");
    // The blank kind line the file carries between kind rows, and the
    // kind row short of the cpu's column, stand absent for that kind
    // alone (FR-018).
    write(procDir / "softirqs", ": 1 2\n   : 3 4\n TIMER: 5\n");
    write(procDir / "schedstat",
          "cpu" + std::to_string(cpu1) + " 1 2 x\n");
    const auto shortRead = sg::detail::precheckReadSnapshot(cpus);
    const auto& state1 = shortRead.at(cpu1);
    check(!state1.interrupts.has_value(),
          "an interrupt row short of the cpu's column stands absent "
          "(FR-018)");
    check(std::any_of(state1.softirqs.begin(),
                      state1.softirqs.end(),
                      [](const auto& kind) { return kind.second.has_value(); })
              == false,
          "a short softirq kind stands absent for that kind alone "
          "(FR-018)");
    check(!state1.switches.has_value() && !state1.schedules.has_value(),
          "a schedstat line that parses as no figure stands absent "
          "(FR-018)");
    write(procDir / "interrupts", " 0: 10 20\n 1: 1 xx\n");
    const auto unparsable = sg::detail::precheckReadSnapshot(cpus);
    check(!unparsable.at(cpu1).interrupts.has_value(),
          "an interrupt column that parses as no number stands absent "
          "(FR-018)");

    // The delta stands absent when one side stands absent: the
    // interrupt file stands deleted between the snapshots.
    write(procDir / "interrupts", " 0: 10 20\n 1: 1 2\n");
    write(procDir / "softirqs", " TIMER: 5 6\n");
    write(procDir / "schedstat",
          "cpu" + std::to_string(cpu1) + " 1 2 3\n");
    const auto before = sg::detail::precheckReadSnapshot(cpus);
    std::filesystem::remove(procDir / "interrupts");
    write(procDir / "softirqs", " TIMER: 5 9\n");
    const auto after = sg::detail::precheckReadSnapshot(cpus);
    const auto delta = sg::detail::precheckAttributionDelta(
        before.at(cpu1), after.at(cpu1));
    check(!delta.interrupts.has_value(),
          "a delta with one side absent stands absent (FR-016, FR-018)");
    check(delta.softirqs.size() == 1
              && delta.softirqs.at(0).second.has_value()
              && *delta.softirqs.at(0).second == 3,
          "the sources that stand on both sides still carry their "
          "deltas (FR-016)");
  }

  // T015, FR-018: the cpu-list and mask parsers refuse the shapes no
  // affinity file spells: the empty token, the lone token no number,
  // the reversed span, the mask token past eight digits; the
  // multi-group mask names its cpus; a blank line names no governor;
  // an irq directory entry whose name parses as no number stands out.
  {
    const auto rootDir = fakeRoot();
    const ProcRootGuard guard(rootDir);
    const auto procDir = rootDir / "proc";
    const auto sysDir = rootDir / "sys/devices/system/cpu";
    std::filesystem::create_directories(procDir);
    const auto write = [](const std::filesystem::path& path,
                          const std::string& text)
    {
      std::filesystem::create_directories(path.parent_path());
      std::ofstream file(path);
      check(static_cast<bool>(file), "the fake file opens (T015)");
      file << text;
    };
    const auto cpus = pickCpus(2);
    const auto cpu0 = cpus.at(0);
    const auto cpu1 = cpus.at(1);
    // The list form carries the unparsable shapes beside the mask
    // form: the empty token, the lone token no number, the reversed
    // span (FR-018).
    write(procDir / "irq/9/smp_affinity_list", "0,,1\n");
    write(procDir / "irq/10/smp_affinity_list", "x\n");
    write(procDir / "irq/11/smp_affinity_list", "3-2\n");
    // The multi-group mask: the picked cpus' own groups beside the
    // low group's bit zero, so the two-group parse stands on any
    // host (VI).
    std::map<std::uint32_t, std::uint64_t> words;
    words[cpu0 / 32U] |= std::uint64_t {1} << (cpu0 % 32U);
    words[cpu1 / 32U] |= std::uint64_t {1} << (cpu1 % 32U);
    words[1U] |= std::uint64_t {1};
    std::string mask;
    for (auto it = words.rbegin(); it != words.rend(); ++it) {
      if (!mask.empty()) {
        mask += ',';
      }
      char group[9] = {};
      std::snprintf(group, sizeof(group), "%08llx",
                    static_cast<unsigned long long>(it->second));
      mask += group;
    }
    write(procDir / "irq/12/smp_affinity", mask + "\n");
    write(procDir / "irq/13/smp_affinity", "000000000\n");
    write(procDir / "irq/README", "nothing\n");
    // An empty governor file names no line, the blank text the
    // `not available` reading stands for (FR-017, FR-018).
    write(sysDir / ("cpu" + std::to_string(cpu0) + "/cpufreq/"
                    "scaling_governor"),
          "\n");
    const auto host = sg::detail::precheckReadHostSettings(cpus);
    const auto namedBy = [&host](std::uint32_t cpu, std::uint32_t irq)
    {
      for (const auto& [named, irqs] : host.irqAffinity) {
        if (named != cpu) {
          continue;
        }
        return std::find(irqs.begin(), irqs.end(), irq) != irqs.end();
      }
      return false;
    };
    check(!namedBy(cpu0, 9U) && !namedBy(cpu1, 9U)
              && !namedBy(cpu0, 10U) && !namedBy(cpu1, 10U)
              && !namedBy(cpu0, 11U) && !namedBy(cpu1, 11U)
              && !namedBy(cpu0, 13U) && !namedBy(cpu1, 13U),
          "the unparsable affinity shapes name no cpu (FR-018)");
    check(namedBy(cpu0, 12U) && namedBy(cpu1, 12U),
          "the multi-group mask names its cpus (FR-018)");
    check(host.governors.size() == 2
              && !host.governors.at(0).second.has_value(),
          "a blank governor line reads `not available` (FR-017)");
  }

  // T015, FR-015: a planned cpu the states do not name stands out of
  // the noisy answer, the way a candidate does: the state list names
  // the measured cpus alone, and a plan that reaches past it finds no
  // gap record to read.
  {
    const auto cpus = pickCpus(2);
    std::vector<sg::detail::ProbeCpuState> states(1);
    states.front().cpu = cpus.at(0);
    const std::vector<std::uint32_t> planned {cpus.at(0), cpus.at(1) + 9U};
    const auto noisy = sg::detail::precheckNoisyCpus(states, planned);
    check(noisy.empty(),
          "a planned cpu the states do not name stands no noise "
          "(FR-015)");
  }

  // T015, FR-017, R-04: the host settings over fake files: the kernel
  // command line's three settings, the governor file per cpu, the IRQ
  // affinity list form with the mask fallback.
  {
    // The root stands cleared before the read: the fake root names
    // the pid, so the scenarios of one process share the directory,
    // and this scenario's figures stand the files it writes alone
    // (T005).
    const auto rootDir = fakeRoot();
    std::filesystem::remove_all(rootDir);
    std::filesystem::create_directories(rootDir);
    const ProcRootGuard guard(rootDir);
    const auto procDir = rootDir / "proc";
    const auto sysDir = rootDir / "sys/devices/system/cpu";
    std::filesystem::create_directories(procDir);
    const auto write = [](const std::filesystem::path& path,
                          const std::string& text)
    {
      std::filesystem::create_directories(path.parent_path());
      std::ofstream file(path);
      check(static_cast<bool>(file), "the fake file opens (T015)");
      file << text;
    };
    const auto cpus = pickCpus(2);
    const auto cpu0 = cpus.at(0);
    const auto cpu1 = cpus.at(1);
    write(procDir / "cmdline",
          "BOOT_IMAGE=/vmlinuz isolcpus=2-3 nohz_full=1 rcu_nocbs=0-1 "
          "quiet\n");
    write(sysDir / ("cpu" + std::to_string(cpu1) + "/cpufreq/"
                    "scaling_governor"),
          "performance\n");
    // The fake files name the cpus the process picked, so the scenario
    // stands on any mask, the small-mask hosts included (VI).
    const auto mask = [cpu0, cpu1]
    {
      std::map<std::uint32_t, std::uint64_t> words;
      words[cpu0 / 32U] |= std::uint64_t {1} << (cpu0 % 32U);
      words[cpu1 / 32U] |= std::uint64_t {1} << (cpu1 % 32U);
      std::string text;
      for (auto it = words.rbegin(); it != words.rend(); ++it) {
        if (!text.empty()) {
          text += ',';
        }
        char group[9] = {};
        std::snprintf(group, sizeof(group), "%08llx",
                      static_cast<unsigned long long>(it->second));
        text += group;
      }
      return text;
    }();
    std::string list = std::to_string(cpu0) + "," + std::to_string(cpu1);
    write(procDir / "irq/24/smp_affinity_list", list + "\n");
    write(procDir / "irq/7/smp_affinity", mask + "\n");
    const auto host = sg::detail::precheckReadHostSettings(cpus);
    check(host.isolcpus.has_value() && *host.isolcpus == "2-3"
              && host.nohzFull.has_value() && *host.nohzFull == "1"
              && host.rcuNocbs.has_value() && *host.rcuNocbs == "0-1",
          "the command line's settings stand disclosed (FR-017)");
    check(host.governors.size() == 2
              && host.governors.at(0).first == cpu0
              && !host.governors.at(0).second.has_value()
              && host.governors.at(1).first == cpu1
              && host.governors.at(1).second.has_value()
              && *host.governors.at(1).second == "performance",
          "each measured cpu stands one governor, absent where the file "
          "stands missing (FR-017)");
    const auto namesBoth = [](const std::vector<std::uint32_t>& irqs)
    {
      return irqs.size() == 2
          && std::find(irqs.begin(), irqs.end(), 7U) != irqs.end()
          && std::find(irqs.begin(), irqs.end(), 24U) != irqs.end();
    };
    check(host.irqAffinity.size() == 2
              && host.irqAffinity.at(0).first == cpu0
              && host.irqAffinity.at(1).first == cpu1
              && namesBoth(host.irqAffinity.at(0).second)
              && namesBoth(host.irqAffinity.at(1).second),
          "each measured cpu names every irq that names it, the mask "
          "form beside the list form (FR-017)");
  }

  // T015, FR-018: the console block prints `not available` for every
  // absent figure, the source named beside it, and the run never
  // fails on it.
  {
    const auto cpus = pickCpus(1);
    sg::PrecheckRecord record;
    sg::PrecheckCpuRecord entry;
    entry.cpu = cpus.at(0);
    entry.planned = true;
    record.cpus.push_back(entry);
    record.host.governors.emplace_back(cpus.at(0), std::nullopt);
    record.host.irqAffinity.emplace_back(cpus.at(0),
                                         std::vector<std::uint32_t> {});
    const auto block = capturePrecheck(record);
    check(
        block.find("interrupts delta: not available (source: "
                   "/proc/interrupts)") != std::string::npos,
        "the absent interrupt delta prints `not available` beside its "
        "source (FR-018)");
    check(block.find("switch delta: not available, schedule delta: not "
                     "available (source: /proc/schedstat)")
              != std::string::npos,
          "the absent schedstat deltas print `not available` (FR-018)");
    check(block.find("host settings: isolcpus=not available "
                     "nohz_full=not available rcu_nocbs=not available")
              != std::string::npos,
          "the unstated settings print `not available` (FR-018)");
    check(block.find("governor: cpu "
                     + std::to_string(cpus.at(0)) + " not available")
              != std::string::npos,
          "the absent governor prints `not available` (FR-018)");
    check(block.find("irq affinity: cpu "
                     + std::to_string(cpus.at(0)) + " not available")
              != std::string::npos,
          "the absent affinity list prints `not available` (FR-018)");
  }

  check(std::fprintf(stdout, "harness_precheck_test: ok\n") > 0, "the run prints");
  return 0;
}
