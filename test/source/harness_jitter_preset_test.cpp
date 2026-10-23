// ============================================================================
// TDD test for the idle resolver, the overlap check and the mask decision
// (T017; US2, FR-016 to FR-020, D-3 to D-5, D-9, R-07, PC-2).
//
// The scenarios run the three pure functions of placement_preset.cpp over
// one synthetic layout installed through `installTestLayout`
// (source/harness/detail/internal.hpp:538), the injection pattern the
// specs/025 placement tests carry. One layout serves every scenario, so
// the whole file runs in one process: `layout()` reads the installed
// layout once and the functions take it as a parameter, so no scenario
// re-reads the host.
//
// The installed layout is quickstart.md's thrawn shape: 4 L3 groups, 24
// cores, 48 threads, two siblings per core, where cpu n and cpu n + 24
// share one core and group n / 6 holds the cores 6n through 6n + 5. The
// `l3` plan over it names cpus 0, 6, 12 and 18, one lowest cpu per group.
//
// Scenario map:
//   idlePerGroup        the `idle` resolution returns, for each L3 group
//                       the plans touch, the lowest cpu no plan uses and
//                       no SMT sibling of a planned cpu names (FR-016,
//                       D-4, R-07)
//   idleNoFree          no free cpu at all is the status-2 condition: the
//                       resolution names no cpu and carries every touched
//                       group as a skip (FR-017, D-4)
//   idleSkip            a group with no free cpu is skipped and the
//                       resolution carries the skip (FR-017, D-4)
//   overlapPlanned      the alongside overlap check names the cpu and the
//                       benchmark for a probe cpu that is a planned cpu,
//                       and names no overlap for a free cpu (FR-018, D-3,
//                       D-9)
//   overlapSibling      the check refuses an SMT sibling of a planned cpu
//                       the same way (FR-018, D-3, D-9)
//   overlapPrecheck     the precheck mode allows the overlap, because it
//                       stops before the benchmark starts; `both` carries
//                       the alongside refusal (D-3, R-10)
//   presetUnchanged     the resolver functions change no preset rule: the
//                       specs/025 plans stand over the same layout
//                       (FR-020, PC-2)
//   maskDrop            the mask decision for an unpinned benchmark drops
//                       the probe cpus, and the empty intersection is the
//                       status-2 condition (FR-019, D-5)
//   maskLimited         the decision carries the cpuset-limit answer a
//                       probe cpu outside the mask sets (FR-036, D-12)
// ============================================================================

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/gcov_flush.hpp"
#include "detail/internal.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER PRESET TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// quickstart.md's thrawn shape: 4 L3 groups, 24 cores, 48 threads, two
// siblings per core; cpu n and cpu n + 24 share one core, and group n / 6
// holds the cores 6n through 6n + 5 (FR-020, PC-2).
auto installThrawnShape() -> void
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 24; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = core / 6;
    sg::detail::LayoutThread thread;
    thread.cpuIndex = core;
    thread.coreOrdinal = core;
    thread.groupOrdinal = core / 6;
    layoutCore.threads.push_back(core);
    installed.threads.push_back(thread);
    installed.mask.push_back(core);
    installed.cores.push_back(layoutCore);
  }
  for (std::uint32_t core = 0; core < 24; ++core) {
    sg::detail::LayoutThread thread;
    thread.cpuIndex = core + 24;
    thread.coreOrdinal = core;
    thread.groupOrdinal = core / 6;
    installed.threads.push_back(thread);
    installed.mask.push_back(core + 24);
    installed.cores.at(core).threads.push_back(core + 24);
  }
  for (std::uint32_t group = 0; group < 4; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * 6;
    for (std::uint32_t core = group * 6; core < group * 6 + 6; ++core) {
      layoutGroup.cores.push_back(core);
    }
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// FR-018, D-3, D-9: one instance whose `l3` plan names the lowest cpu of
// each group.
auto overlapInstance() -> sg::Instance
{
  sg::Instance instance;
  instance.name = "bmPresetOverlap";
  instance.threadCount = 4;
  instance.placement = sg::detail::Preset::L3;
  return instance;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  if (argc > 1) {
    // The breach children die at the contract call; the counters the
    // lines ran reach the coverage report only when the handler writes
    // them first (R-07).
    sg::test::flushCountersOnAbort();
  }
  if (argc > 1 && std::string(argv[1]) == "idle-unresolved") {
    // The planned cpu stands outside the layout's mask: the resolver's
    // precondition is a violation, and the checked semantic terminates
    // the process (R-07).
    sg::detail::MachineLayout machine;
    machine.readable = true;
    machine.packages = 1;
    sg::detail::LayoutThread thread;
    thread.cpuIndex = 0;
    thread.coreOrdinal = 0;
    thread.groupOrdinal = 0;
    machine.threads.push_back(thread);
    machine.mask.push_back(0);
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = 0;
    layoutCore.threads = {0U};
    machine.cores.push_back(layoutCore);
    sg::detail::LayoutGroup group;
    group.lowestCpu = 0;
    group.cores = {0U};
    machine.groups.push_back(group);
    (void)sg::detail::resolveIdleCpus(machine, std::vector<std::uint32_t> {7});
    return 0;
  }
  if (argc > 1 && std::string(argv[1]) == "mask-gap-unresolved") {
    // The planned cpu stands below the set's bound and inside the
    // index range, and the layout's mask names cpus 0 and 9 only, so
    // the mask membership reads the gap where the cpu stands (R-07).
    sg::detail::MachineLayout machine;
    machine.readable = true;
    machine.packages = 1;
    sg::detail::LayoutThread thread;
    thread.cpuIndex = 0;
    thread.coreOrdinal = 0;
    thread.groupOrdinal = 0;
    machine.threads.push_back(thread);
    machine.mask.push_back(0);
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = 0;
    layoutCore.threads = {0U};
    machine.cores.push_back(layoutCore);
    sg::detail::LayoutGroup group;
    group.lowestCpu = 0;
    group.cores = {0U};
    machine.groups.push_back(group);
    machine.mask.push_back(9);
    (void)sg::detail::resolveIdleCpus(machine, std::vector<std::uint32_t> {7});
    return 0;
  }

  installThrawnShape();
  const auto& machine = sg::detail::layout();

  // FR-016, D-4, R-07: the `l3` plan names cpus 0, 6, 12 and 18, and the
  // resolution gives each group's lowest cpu that no plan uses and no
  // SMT sibling of a planned cpu names.
  {
    const auto resolution = sg::detail::resolveIdleCpus(
        machine, std::vector<std::uint32_t> {0, 6, 12, 18});
    const std::vector<std::uint32_t> expected {1, 7, 13, 19};
    check(resolution.cpus == expected,
          "idle gives the lowest free cpu of each planned group (FR-016)");
    check(resolution.skippedGroups.empty(),
          "a plan that leaves cpus free skips no group (FR-017)");
  }

  // FR-017, D-4: plans that name every cpu leave no group a free cpu;
  // the resolution names no cpu and carries every touched group as a
  // skip, the status-2 condition.
  {
    const auto resolution = sg::detail::resolveIdleCpus(
        machine,
        std::vector<std::uint32_t> {
            0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
            32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47});
    check(resolution.cpus.empty(),
          "no free cpu at all names no probe cpu (FR-017)");
    const std::vector<std::uint32_t> expected {0, 1, 2, 3};
    check(resolution.skippedGroups == expected,
          "every touched group stands in the skip record (FR-017, D-4)");
  }

  // FR-017, D-4: a group whose planned cpus and their siblings cover it
  // stands skipped, and a touched group with cpus left contributes its
  // lowest free cpu.
  {
    const auto resolution = sg::detail::resolveIdleCpus(
        machine, std::vector<std::uint32_t> {0, 1, 2, 3, 4, 5, 6});
    const std::vector<std::uint32_t> expected {7};
    check(
        resolution.cpus == expected,
        "the group with cpus left contributes its lowest free cpu " "(FR-016)");
    const std::vector<std::uint32_t> skipped {0};
    check(resolution.skippedGroups == skipped,
          "the covered group stands skipped (FR-017, D-4)");
  }

  // FR-018, D-3, D-9: the alongside overlap check names a planned cpu
  // and the benchmark, refuses an SMT sibling the same way, and names no
  // overlap for a free cpu.
  {
    sg::Instance instance = overlapInstance();
    const std::vector<sg::Instance*> selected {&instance};

    const auto planned =
        sg::detail::probeOverlapOf(machine,
                                   selected,
                                   std::vector<std::uint32_t> {0},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(planned.has_value() && planned->cpu == 0
              && planned->benchmark == "bmPresetOverlap",
          "a planned probe cpu names the cpu and the benchmark (FR-018)");

    const auto sibling =
        sg::detail::probeOverlapOf(machine,
                                   selected,
                                   std::vector<std::uint32_t> {24},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(sibling.has_value() && sibling->cpu == 24
              && sibling->benchmark == "bmPresetOverlap",
          "an SMT sibling of a planned cpu is refused the same way "
          "(FR-018, D-9)");

    const auto free =
        sg::detail::probeOverlapOf(machine,
                                   selected,
                                   std::vector<std::uint32_t> {7},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(!free.has_value(), "a free probe cpu names no overlap (FR-018)");
  }

  // D-3, R-10: the precheck mode allows every overlap, because it stops
  // before the benchmarks start; `both` carries the alongside refusal.
  {
    sg::Instance instance = overlapInstance();
    const std::vector<sg::Instance*> selected {&instance};

    const auto allowed =
        sg::detail::probeOverlapOf(machine,
                                   selected,
                                   std::vector<std::uint32_t> {0, 24},
                                   sg::JitterProbeMode::PRECHECK);
    check(!allowed.has_value(),
          "the precheck mode allows the overlap (D-3, R-10)");

    const auto both =
        sg::detail::probeOverlapOf(machine,
                                   selected,
                                   std::vector<std::uint32_t> {24},
                                   sg::JitterProbeMode::BOTH);
    check(both.has_value() && both->cpu == 24,
          "the alongside phase of `both` refuses the overlap (D-3, R-10)");
  }

  // FR-020, PC-2: the new functions change no preset rule; the specs/025
  // plans stand over the same layout.
  {
    const auto l3 =
        sg::detail::resolvePreset(sg::detail::Preset::L3, machine, 4);
    const std::vector<std::uint32_t> expectedL3 {0, 6, 12, 18};
    check(l3.cpu == expectedL3, "the `l3` rule stands (FR-020, PC-2)");

    const auto core =
        sg::detail::resolvePreset(sg::detail::Preset::CORE, machine, 24);
    check(
        core.cpu.size() == 24 && core.cpu.front() == 0 && core.cpu.back() == 23,
        "the `core` rule stands (FR-020, PC-2)");

    const auto smt =
        sg::detail::resolvePreset(sg::detail::Preset::SMT, machine, 2);
    const std::vector<std::uint32_t> expectedSmt {0, 24};
    check(smt.cpu == expectedSmt, "the `smt` rule stands (FR-020, PC-2)");

    const auto compact =
        sg::detail::resolvePreset(sg::detail::Preset::COMPACT, machine, 48);
    check(compact.cpu.size() == 48 && compact.cpu.front() == 0
              && compact.cpu.at(1) == 24 && compact.cpu.back() == 47,
          "the `compact` rule stands (FR-020, PC-2)");
  }

  // FR-019, D-5: the mask decision drops the probe cpus, and the empty
  // intersection is the status-2 condition.
  {
    const auto drop =
        sg::detail::maskNarrowingOf(std::vector<std::uint32_t> {1, 3},
                                    std::vector<std::uint32_t> {0, 1, 2, 3, 4});
    const std::vector<std::uint32_t> expected {0, 2, 4};
    check(drop.remaining == expected,
          "the mask decision drops the probe cpus (FR-019, D-5)");
    check(!drop.limited,
          "a probe cpu inside the mask names no cpuset limit (FR-036)");

    const auto empty = sg::detail::maskNarrowingOf(
        std::vector<std::uint32_t> {0, 1}, std::vector<std::uint32_t> {0, 1});
    check(empty.remaining.empty(),
          "the empty intersection is the status-2 condition (FR-019, D-5)");
  }

  // FR-036, D-12: a probe cpu that names no mask cpu sets the
  // cpuset-limit answer.
  {
    const auto limited = sg::detail::maskNarrowingOf(
        std::vector<std::uint32_t> {5}, std::vector<std::uint32_t> {0, 1});
    check(limited.limited,
          "a probe cpu outside the mask sets the cpuset limit (FR-036)");
    const std::vector<std::uint32_t> expected {0, 1};
    check(limited.remaining == expected,
          "the mask cpus stand in the narrowed mask (FR-019)");
  }

  // D-4, R-07: a planned cpu the layout carries outside every group and
  // core: the resolution's walk and the postcondition predicate's walk
  // both skip its group and sibling collections, the touched record
  // stands empty, and the resolution names no cpu and no skip, whose
  // covered set equals the touched one.
  {
    sg::detail::MachineLayout outside;
    outside.readable = true;
    outside.packages = 1;
    for (const auto cpu : {0U, 1U}) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = cpu;
      thread.coreOrdinal = 0;
      thread.groupOrdinal = 0;
      outside.threads.push_back(thread);
      outside.mask.push_back(cpu);
    }
    sg::detail::LayoutThread lone;
    lone.cpuIndex = 2;
    lone.coreOrdinal = sg::detail::kNoGroup;
    lone.groupOrdinal = sg::detail::kNoGroup;
    outside.threads.push_back(lone);
    outside.mask.push_back(2);
    sg::detail::LayoutCore shared;
    shared.groupOrdinal = 0;
    shared.threads = {0U, 1U};
    outside.cores.push_back(shared);
    sg::detail::LayoutGroup group;
    group.lowestCpu = 0;
    group.cores = {0U};
    outside.groups.push_back(group);

    const auto resolution =
        sg::detail::resolveIdleCpus(outside, std::vector<std::uint32_t> {2});
    check(resolution.cpus.empty()
              && resolution.skippedGroups.empty(),
          "a planned cpu outside every group and core leaves the "
          "resolution empty (FR-016, D-4)");
  }

  // FR-018, D-9, R-10: the overlap walk reads each planned cpu against
  // the mask: one planned cpu inside the mask's range that the mask
  // lacks, one past the mask's end, and the SMT siblings of the ones
  // the mask holds. The probe cpu naming a sibling stands refused with
  // the benchmark's name, and a probe cpu naming none stands allowed.
  {
    sg::detail::MachineLayout outside;
    outside.readable = true;
    outside.packages = 1;
    // The mask holds cpus 0, 1 and 3; cpu 2 stands inside the mask's
    // range without being in it, and cpu 5 stands past its end. The
    // `core` preset plans the front thread of each core: cpu 0, cpu 2
    // and cpu 5.
    const std::array<std::uint32_t, 5> cpuIndexes {0U, 1U, 2U, 3U, 5U};
    for (const auto cpu : cpuIndexes) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = cpu;
      thread.coreOrdinal = cpu == 5U ? 2U : cpu / 2U;
      thread.groupOrdinal = 0;
      outside.threads.push_back(thread);
    }
    outside.mask = {0U, 1U, 3U};
    const std::array<std::uint32_t, 3> coreThreadCounts {3U, 2U, 1U};
    for (std::size_t ordinal = 0; ordinal < coreThreadCounts.size(); ++ordinal)
    {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = 0;
      for (std::uint32_t index = 0; index < coreThreadCounts.at(ordinal);
           ++index)
      {
        layoutCore.threads.push_back(static_cast<std::uint32_t>(ordinal) * 2U
                                     + index);
      }
      outside.cores.push_back(layoutCore);
    }
    sg::detail::LayoutGroup group;
    group.lowestCpu = 0;
    group.cores = {0U, 1U, 2U};
    outside.groups.push_back(group);

    sg::Instance instance;
    instance.name = "bmPresetMaskWalk";
    instance.threadCount = 3;
    instance.placement = sg::detail::Preset::CORE;
    const std::vector<sg::Instance*> selected {&instance};

    const auto planned =
        sg::detail::probeOverlapOf(outside,
                                   selected,
                                   std::vector<std::uint32_t> {1},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(planned.has_value() && planned->cpu == 1
              && planned->benchmark == "bmPresetMaskWalk",
          "a probe cpu that names a planned sibling stands refused "
          "(FR-018, D-9)");

    const auto free =
        sg::detail::probeOverlapOf(outside,
                                   selected,
                                   std::vector<std::uint32_t> {9},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(!free.has_value(),
          "a probe cpu that names no planned cpu and no sibling "
          "stands allowed (FR-018)");
  }

  // The contract-only predicates the SG_ENSURE contracts read, with the
  // bad input each reachable false arm names (D-4, D-9, D-5).
  {
    // The planned-cpus rule: a cpu the mask holds stands true, a cpu
    // the mask lacks stands false (R-07).
    check(
        sg::detail::plannedCpusResolve(machine, std::vector<std::uint32_t> {0}),
        "a planned cpu inside the mask stands resolved (R-07)");
    check(!sg::detail::plannedCpusResolve(machine,
                                          std::vector<std::uint32_t> {48}),
          "a planned cpu past the mask stands unresolved (R-07)");

    // The resolution's predicate: the true answer over the planned pair
    // the free-cpu block above reads, and the false arms each refusal
    // names (FR-016, FR-017, D-4).
    const sg::detail::IdleResolution genuine =
        sg::detail::resolveIdleCpus(machine, std::vector<std::uint32_t> {0});
    check(sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {0}, genuine),
          "the true resolution stands valid (D-4)");

    sg::detail::IdleResolution unsortedSkips = genuine;
    unsortedSkips.skippedGroups = {2, 1};
    check(!sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {0}, unsortedSkips),
          "a skip record out of order stands invalid (D-4)");

    sg::detail::IdleResolution pastMask = genuine;
    pastMask.cpus = {1000};
    check(!sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {0}, pastMask),
          "a yielded cpu the mask lacks stands invalid (D-4)");

    sg::detail::IdleResolution wrongGroup = genuine;
    wrongGroup.cpus = {0};
    check(!sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {6}, wrongGroup),
          "a covered set apart from the touched one stands invalid " "(D-4)");

    sg::detail::IdleResolution plannedYield = genuine;
    plannedYield.cpus = {0};
    check(!sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {0}, plannedYield),
          "a yielded planned cpu stands invalid (D-4)");

    sg::detail::IdleResolution siblingYield = genuine;
    siblingYield.cpus = {24};
    siblingYield.skippedGroups = {};
    check(!sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {0}, siblingYield),
          "a yielded sibling of a planned cpu stands invalid (D-4)");

    sg::detail::IdleResolution notLowest = genuine;
    notLowest.cpus = {25};
    notLowest.skippedGroups = {};
    check(!sg::detail::idleResolutionValid(
              machine, std::vector<std::uint32_t> {0}, notLowest),
          "a yielded cpu above the group's free one stands invalid " "(D-4)");

    // The mask's own gap: a yielded cpu below the mask's top stands
    // found against a later entry, the arm the lower-bound read
    // carries (D-4). The layout holds the mask {0, 1, 3} with cpu 2
    // absent, and the resolution yields the absent index.
    {
      sg::detail::MachineLayout gapped;
      gapped.readable = true;
      gapped.packages = 1;
      sg::detail::LayoutCore core;
      core.groupOrdinal = 0;
      for (const auto cpu : std::vector<std::uint32_t> {0, 1, 3}) {
        sg::detail::LayoutThread thread;
        thread.cpuIndex = cpu;
        thread.coreOrdinal = 0;
        thread.groupOrdinal = 0;
        core.threads.push_back(
            static_cast<std::uint32_t>(gapped.threads.size()));
        gapped.threads.push_back(thread);
        gapped.mask.push_back(cpu);
      }
      gapped.cores.push_back(core);
      sg::detail::LayoutGroup group;
      group.lowestCpu = 0;
      group.cores = {0U};
      gapped.groups.push_back(group);
      sg::detail::IdleResolution gapYield;
      gapYield.cpus = {2};
      check(!sg::detail::idleResolutionValid(
                gapped, std::vector<std::uint32_t> {0}, gapYield),
            "a yielded cpu in the mask's gap stands invalid (D-4)");
    }

    // A yielded cpu whose thread names no group stands refused by the
    // record the walk reads (D-4). The layout holds one thread with no
    // core and no group, the mask carries its cpu, and the resolution
    // yields it.
    {
      sg::detail::MachineLayout bare;
      bare.readable = true;
      bare.packages = 1;
      sg::detail::LayoutThread thread;
      thread.cpuIndex = 5;
      thread.coreOrdinal = sg::detail::kNoGroup;
      thread.groupOrdinal = sg::detail::kNoGroup;
      bare.threads.push_back(thread);
      bare.mask.push_back(5);
      sg::detail::IdleResolution bareYield;
      bareYield.cpus = {5};
      check(!sg::detail::idleResolutionValid(
                bare, {}, bareYield),
            "a yielded cpu with no group stands invalid (D-4)");
    }

    // The overlap answer's predicate: the answer names a probe cpu the
    // list holds, a free probe cpu names no overlap, and the precheck
    // mode names none (FR-018, D-9, R-10, D-3).
    check(sg::detail::probeOverlapValid(
              std::vector<std::uint32_t> {5},
              sg::JitterProbeMode::ALONGSIDE,
              std::optional<sg::detail::ProbeOverlap>()),
          "no overlap stands valid (FR-018)");
    check(sg::detail::probeOverlapValid(
              std::vector<std::uint32_t> {5},
              sg::JitterProbeMode::ALONGSIDE,
              sg::detail::ProbeOverlap {5, "bmPresetOverlap"}),
          "an overlap inside the probe cpus stands valid (FR-018)");
    check(!sg::detail::probeOverlapValid(
              std::vector<std::uint32_t> {7},
              sg::JitterProbeMode::ALONGSIDE,
              sg::detail::ProbeOverlap {5, "bmPresetOverlap"}),
          "an overlap outside the probe cpus stands invalid (FR-018)");
    check(!sg::detail::probeOverlapValid(
              std::vector<std::uint32_t> {5},
              sg::JitterProbeMode::PRECHECK,
              sg::detail::ProbeOverlap {5, "bmPresetOverlap"}),
          "an overlap under the precheck mode stands invalid (D-3, R-10)");

    // The narrowing's predicate: the remaining cpus name no probe cpu,
    // stand inside the mask, and the limit answer follows the mask
    // (FR-019, FR-036, D-5).
    check(sg::detail::maskNarrowingValid(
              std::vector<std::uint32_t> {1},
              std::vector<std::uint32_t> {0, 1, 2},
              sg::detail::MaskNarrowing {{0, 2}, false}),
          "the plain narrowing stands valid (D-5)");
    check(!sg::detail::maskNarrowingValid(
              std::vector<std::uint32_t> {1},
              std::vector<std::uint32_t> {0, 1, 2},
              sg::detail::MaskNarrowing {{1, 2}, false}),
          "a remaining probe cpu stands invalid (D-5)");
    check(
        !sg::detail::maskNarrowingValid(std::vector<std::uint32_t> {1},
                                        std::vector<std::uint32_t> {0, 1, 2},
                                        sg::detail::MaskNarrowing {{3}, false}),
        "a remaining cpu outside the mask stands invalid (D-5)");
    check(!sg::detail::maskNarrowingValid(
              std::vector<std::uint32_t> {5},
              std::vector<std::uint32_t> {0, 1},
              sg::detail::MaskNarrowing {{0, 1}, false}),
          "a limit answer the mask contradicts stands invalid (FR-036)");
  }

  // FR-017, D-4: the two single-thread groups the plan covers whole
  // stand skipped, and the touched group with cpus left yields its
  // lowest free cpu, so the run over the skipped groups continues
  // (FR-017, D-4) - the rule the removed compact-25 CLI case carried,
  // read as a pure function (VI).
  {
    sg::detail::MachineLayout threeGroup;
    threeGroup.readable = true;
    threeGroup.packages = 1;
    const std::array<std::uint32_t, 6> cpuIndexes {0U, 1U, 2U, 3U, 4U, 5U};
    const std::array<std::uint32_t, 6> coreOf {{0U, 1U, 2U, 4U, 2U, 4U}};
    const std::array<std::uint32_t, 6> groupOf {{0U, 1U, 2U, 2U, 2U, 2U}};
    for (std::size_t index = 0; index < cpuIndexes.size(); ++index) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = cpuIndexes.at(index);
      thread.coreOrdinal = coreOf.at(index);
      thread.groupOrdinal = groupOf.at(index);
      threeGroup.threads.push_back(thread);
      threeGroup.mask.push_back(cpuIndexes.at(index));
    }
    const std::array<std::uint32_t, 4> coreGroup {{0U, 1U, 2U, 2U}};
    for (std::uint32_t core = 0; core < 4; ++core) {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = coreGroup.at(core);
      for (std::size_t index = 0; index < cpuIndexes.size(); ++index) {
        if (coreOf.at(index) == core) {
          layoutCore.threads.push_back(static_cast<std::uint32_t>(index));
        }
      }
      threeGroup.cores.push_back(layoutCore);
    }
    const std::array<std::uint32_t, 3> groupLowest {{0U, 1U, 2U}};
    const std::array<std::uint32_t, 3> groupCores {{1U, 1U, 2U}};
    std::uint32_t coreCursor = 0;
    for (std::uint32_t group = 0; group < 3; ++group) {
      sg::detail::LayoutGroup layoutGroup;
      layoutGroup.lowestCpu = groupLowest.at(group);
      for (std::uint32_t offset = 0; offset < groupCores.at(group); ++offset) {
        layoutGroup.cores.push_back(coreCursor + offset);
      }
      coreCursor += groupCores.at(group);
      threeGroup.groups.push_back(layoutGroup);
    }

    const auto mixed = sg::detail::resolveIdleCpus(
        threeGroup, std::vector<std::uint32_t> {0, 1, 2});
    const std::vector<std::uint32_t> freeCpus {3};
    check(mixed.cpus == freeCpus,
          "the touched group with cpus left yields its lowest free cpu "
          "(FR-016)");
    const std::vector<std::uint32_t> skippedBoth {0, 1};
    check(mixed.skippedGroups == skippedBoth,
          "the two covered groups stand in the skip record (FR-017, D-4)");
  }

  // FR-018, D-9: a planned cpu the layout carries with an L3 group and
  // no core record: the read leaves the PU's coreOrdinal at kNoGroup
  // while the group's lowest names it, so the `l3` plan names it and
  // the overlap walk collects no core mates through it (FR-018, R-07).
  {
    sg::detail::MachineLayout puWithoutCore;
    puWithoutCore.readable = true;
    puWithoutCore.packages = 1;
    sg::detail::LayoutThread bare;
    bare.cpuIndex = 0;
    bare.coreOrdinal = sg::detail::kNoGroup;
    bare.groupOrdinal = 0;
    puWithoutCore.threads.push_back(bare);
    puWithoutCore.mask.push_back(0);
    sg::detail::LayoutThread paired;
    paired.cpuIndex = 2;
    paired.coreOrdinal = 0;
    paired.groupOrdinal = 1;
    puWithoutCore.threads.push_back(paired);
    puWithoutCore.mask.push_back(2);
    sg::detail::LayoutThread mate;
    mate.cpuIndex = 3;
    mate.coreOrdinal = 0;
    mate.groupOrdinal = 1;
    puWithoutCore.threads.push_back(mate);
    puWithoutCore.mask.push_back(3);
    sg::detail::LayoutCore shared;
    shared.groupOrdinal = 1;
    shared.threads = {1U, 2U};
    puWithoutCore.cores.push_back(shared);
    sg::detail::LayoutGroup bareGroup;
    bareGroup.lowestCpu = 0;
    bareGroup.cores = {};
    puWithoutCore.groups.push_back(bareGroup);
    sg::detail::LayoutGroup pairedGroup;
    pairedGroup.lowestCpu = 2;
    pairedGroup.cores = {0U};
    puWithoutCore.groups.push_back(pairedGroup);

    const auto plan =
        sg::detail::resolvePreset(sg::detail::Preset::L3, puWithoutCore, 2);
    check(plan.cpu == std::vector<std::uint32_t> {0, 2},
          "the group's lowest names the coreless PU (FR-018)");

    sg::Instance corelessInstance;
    corelessInstance.name = "bmPresetCoreless";
    corelessInstance.threadCount = 2;
    corelessInstance.placement = sg::detail::Preset::L3;
    std::vector<sg::Instance*> corelessSelected {&corelessInstance};
    const auto refused =
        sg::detail::probeOverlapOf(puWithoutCore,
                                   corelessSelected,
                                   std::vector<std::uint32_t> {0},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(refused.has_value() && refused->cpu == 0,
          "a probe on the coreless PU's lowest stands refused (FR-018)");

    const auto mateRefused =
        sg::detail::probeOverlapOf(puWithoutCore,
                                   corelessSelected,
                                   std::vector<std::uint32_t> {3},
                                   sg::JitterProbeMode::ALONGSIDE);
    check(mateRefused.has_value() && mateRefused->cpu == 3,
          "a probe on a planned cpu's core mate stands refused (FR-018)");
  }

  // R-07: the self-exec the registry test carries: a planned cpu that
  // names no thread of the layout violates the resolver's precondition,
  // and the checked semantic terminates the child. The breach reads the
  // exact death: the child stands signaled by SIGABRT, the way master's
  // breach tests read it. The second breach names the planned cpu 7
  // against a mask that holds cpus 0 and 9: the gap in the index range
  // is the same violation one run lower (R-07).
  const std::string self = argc > 0 ? argv[0] : "harness_jitter_preset_test";
  for (const auto* mode : {"idle-unresolved", "mask-gap-unresolved"}) {
    const char* const path = "harness_jitter_preset_breach.txt";
    const pid_t pid = fork();
    if (pid < 0) {
      fail("cannot fork the breached child");
    }
    if (pid == 0) {
      if (std::freopen(path, "w", stderr) == nullptr) {
        _exit(2);
      }
      char* const execArgv[] = {
          const_cast<char*>(self.c_str()), const_cast<char*>(mode), nullptr};
      (void)execv(self.c_str(), execArgv);
      _exit(2);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
      fail("cannot wait for the breached child");
    }
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
          "a planned cpu outside the mask is a violation (R-07)");
    std::ifstream breachFile(path);
    std::stringstream breachBuffer;
    breachBuffer << breachFile.rdbuf();
    check(breachBuffer.str().find("every planned cpu names a thread")
              != std::string::npos,
          "the breach message names the planned-cpu rule (R-07)");
  }

  std::puts("harness_jitter_preset_test: ok");
  return 0;
}
