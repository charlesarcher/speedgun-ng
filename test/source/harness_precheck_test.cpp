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

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <sched.h>
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
    ref.addCounter("machine",
                   "tsc",
                   "none",
                   "raw tick counter",
                   sg::counters::Availability::COUNTABLE,
                   sg::counters::ReadMode::FAST_TSC);
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

}  // namespace

auto main() -> int
{
  scriptedProvider();
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

  check(std::fprintf(stdout, "harness_precheck_test: ok\n") > 0, "the run prints");
  return 0;
}
