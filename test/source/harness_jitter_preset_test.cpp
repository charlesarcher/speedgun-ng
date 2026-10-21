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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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

auto main() -> int
{
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
    const auto resolution =
        sg::detail::resolveIdleCpus(machine,
                                    std::vector<std::uint32_t> {
                                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                        12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                        22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                                        32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
                                        42, 43, 44, 45, 46, 47});
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
    check(resolution.cpus == expected,
          "the group with cpus left contributes its lowest free cpu "
          "(FR-016)");
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

    const auto planned = sg::detail::probeOverlapOf(
        machine, selected, std::vector<std::uint32_t> {0},
        sg::JitterProbeMode::ALONGSIDE);
    check(planned.has_value() && planned->cpu == 0
              && planned->benchmark == "bmPresetOverlap",
          "a planned probe cpu names the cpu and the benchmark (FR-018)");

    const auto sibling = sg::detail::probeOverlapOf(
        machine, selected, std::vector<std::uint32_t> {24},
        sg::JitterProbeMode::ALONGSIDE);
    check(sibling.has_value() && sibling->cpu == 24
              && sibling->benchmark == "bmPresetOverlap",
          "an SMT sibling of a planned cpu is refused the same way "
          "(FR-018, D-9)");

    const auto free = sg::detail::probeOverlapOf(
        machine, selected, std::vector<std::uint32_t> {7},
        sg::JitterProbeMode::ALONGSIDE);
    check(!free.has_value(),
          "a free probe cpu names no overlap (FR-018)");
  }

  // D-3, R-10: the precheck mode allows every overlap, because it stops
  // before the benchmarks start; `both` carries the alongside refusal.
  {
    sg::Instance instance = overlapInstance();
    const std::vector<sg::Instance*> selected {&instance};

    const auto allowed = sg::detail::probeOverlapOf(
        machine, selected, std::vector<std::uint32_t> {0, 24},
        sg::JitterProbeMode::PRECHECK);
    check(!allowed.has_value(),
          "the precheck mode allows the overlap (D-3, R-10)");

    const auto both = sg::detail::probeOverlapOf(
        machine, selected, std::vector<std::uint32_t> {24},
        sg::JitterProbeMode::BOTH);
    check(both.has_value() && both->cpu == 24,
          "the alongside phase of `both` refuses the overlap (D-3, R-10)");
  }

  // FR-020, PC-2: the new functions change no preset rule; the specs/025
  // plans stand over the same layout.
  {
    const auto l3 = sg::detail::resolvePreset(sg::detail::Preset::L3,
                                              machine, 4);
    const std::vector<std::uint32_t> expectedL3 {0, 6, 12, 18};
    check(l3.cpu == expectedL3, "the `l3` rule stands (FR-020, PC-2)");

    const auto core = sg::detail::resolvePreset(sg::detail::Preset::CORE,
                                                machine, 24);
    check(core.cpu.size() == 24 && core.cpu.front() == 0
              && core.cpu.back() == 23,
          "the `core` rule stands (FR-020, PC-2)");

    const auto smt = sg::detail::resolvePreset(sg::detail::Preset::SMT,
                                               machine, 2);
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
    const auto drop = sg::detail::maskNarrowingOf(
        std::vector<std::uint32_t> {1, 3},
        std::vector<std::uint32_t> {0, 1, 2, 3, 4});
    const std::vector<std::uint32_t> expected {0, 2, 4};
    check(drop.remaining == expected,
          "the mask decision drops the probe cpus (FR-019, D-5)");
    check(!drop.limited,
          "a probe cpu inside the mask names no cpuset limit (FR-036)");

    const auto empty = sg::detail::maskNarrowingOf(
        std::vector<std::uint32_t> {0, 1},
        std::vector<std::uint32_t> {0, 1});
    check(empty.remaining.empty(),
          "the empty intersection is the status-2 condition (FR-019, D-5)");
  }

  // FR-036, D-12: a probe cpu that names no mask cpu sets the
  // cpuset-limit answer.
  {
    const auto limited = sg::detail::maskNarrowingOf(
        std::vector<std::uint32_t> {5},
        std::vector<std::uint32_t> {0, 1});
    check(limited.limited,
          "a probe cpu outside the mask sets the cpuset limit (FR-036)");
    const std::vector<std::uint32_t> expected {0, 1};
    check(limited.remaining == expected,
          "the mask cpus stand in the narrowed mask (FR-019)");
  }

  std::puts("harness_jitter_preset_test: ok");
  return 0;
}
