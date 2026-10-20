// ============================================================================
// TDD test for the machine layout read (T007; US1, FR-001, FR-002, FR-003,
// FR-005, FR-006, R-04, R-11, R-13, SC-04, Q-5, Q-8).
//
// The layout is one read per process, so every scenario that reads the
// layout or installs a seam runs in a fresh process by the fork and execv
// pattern of harness_context_barrier_test.cpp:116-126, naming the scenario
// in argv and re-installing the seam in the child: one cached read per
// process serves exactly one layout (T006, data-model.md).
//
// The synthetic descriptions ride the vendored hwloc through
// installTestTopologyDescription (R-04): the description `1` names one
// processing unit, and a malformed description must leave the layout
// unreadable, with no partial layout standing. The installed-layout seam
// (Q-5) is
// checked in the process that installs it, standing shapes the host read
// of this machine could not produce.
//
// Scenario map:
//   hostRead            one real host read: the package and thread counts
//                       stand at 1 or above while the core and group counts
//                       stand at 0 or above with the matching thread
//                       ordinals unset when the host publishes neither
//                       (data-model.md); every thread of the layout sits
//                       inside the process mask, and groups, cores and
//                       threads are ordered by ascending cpu index
//                       (FR-001, FR-005, R-11, R-13)
//   maskNarrowed        a mask narrowed in-process with sched_setaffinity
//                       lowers the counts to what the mask holds and the
//                       layout's mask equals the narrowed set (FR-005)
//   installedOne        an installed layout of one group holding one core
//                       with no sibling reports the counts the injection
//                       stated (SC-04)
//   installedFour       an installed layout of four groups holding 24 cores
//                       with two siblings each reports the counts the
//                       injection stated (SC-04)
//   installedSiblings   the same four-group layout with siblings off
//                       reports the counts the injection stated (SC-04)
//   syntheticOne        an installed synthetic description `1` yields no
//                       core and no group and a thread count equal to the
//                       weight of the intersection of that description's
//                       cpu set with the process mask: 1 under a mask
//                       holding cpu 0 (R-04, Q-8)
//   syntheticNarrowed   the same description under a mask holding cpus 4
//                       to 7 yields a thread count of 0, the walk rule
//                       skipping every unit outside the mask (FR-005, R-13)
//   malformed           a malformed description leaves readable false
//                       (FR-005, R-04)
//   noGroup             an installed synthetic description
//                       `Package:2 Core:2 PU:2` yields cores with no
//                       group, the walk rule closing a core that names no
//                       cache ancestor (FR-002, E-09)
//   listingGap          a listing of a mask holding single cpus prints
//                       each cpu alone, the range rule closing a span of
//                       one (FR-036)
//   readOnce            layout() called twice in one process yields the
//                       same layout, and an install after the first call
//                       changes nothing (FR-002, R-04)
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS TOPOLOGY TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto runChild(const char* self, const char* mode) -> int
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

// The mask the operating system reports for this process, as cpu indices.
auto processMask() -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("sched_getaffinity failed");
  }
  std::vector<std::uint32_t> cpus;
  for (std::size_t cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
    if (CPU_ISSET(cpu, &mask)) {
      cpus.push_back(static_cast<std::uint32_t>(cpu));
    }
  }
  return cpus;
}

// Builds `groups` groups of `coresPerGroup` cores with `siblings` threads
// each, spaced 12 cpus apart per group, and installs it (SC-04, Q-5).
auto installShape(const std::uint32_t groups,
                  const std::uint32_t coresPerGroup,
                  const std::uint32_t siblings) -> void
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t group = 0; group < groups; ++group) {
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      sg::detail::LayoutCore layoutCore;
      layoutCore.groupOrdinal = group;
      for (std::uint32_t sibling = 0; sibling < siblings; ++sibling) {
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
    for (std::uint32_t core = 0; core < coresPerGroup; ++core) {
      layoutGroup.cores.push_back(group * coresPerGroup + core);
    }
    installed.groups.push_back(layoutGroup);
  }
  sg::detail::installTestLayout(installed);
}

// The injected shapes report the counts the injection stated, with no host
// read happening in this process (FR-006, Q-5, SC-04).
auto childInstalled(const std::uint32_t groups,
                    const std::uint32_t coresPerGroup,
                    const std::uint32_t siblings) -> int
{
  installShape(groups, coresPerGroup, siblings);
  const auto& layout = sg::detail::layout();
  const std::size_t threads =
      static_cast<std::size_t>(groups) * coresPerGroup * siblings;
  if (!layout.readable || layout.threads.size() != threads
      || layout.cores.size() != static_cast<std::size_t>(groups) * coresPerGroup
      || layout.groups.size() != groups)
  {
    std::fprintf(stderr, "the installed layout did not report its shape\n");
    return 1;
  }
  return 0;
}

// FR-001, FR-005, R-11, R-13: the host read answers the counts, the mask
// rule and the ordering.
auto childHostRead() -> int
{
  const auto& layout = sg::detail::layout();
  if (!layout.readable) {
    std::fprintf(stderr, "the host read failed on a host that must answer\n");
    return 1;
  }
  if (layout.packages < 1 || layout.threads.empty()) {
    std::fprintf(stderr, "the host read gave no package or no thread\n");
    return 1;
  }
  for (const auto& thread : layout.threads) {
    if (!layout.cores.empty() && thread.coreOrdinal == sg::detail::kNoGroup) {
      std::fprintf(stderr, "a thread names no core while cores stand\n");
      return 1;
    }
    if (!layout.groups.empty() && thread.groupOrdinal == sg::detail::kNoGroup) {
      std::fprintf(stderr, "a thread names no group while groups stand\n");
      return 1;
    }
    if (thread.coreOrdinal < layout.cores.size()
        && layout.cores[thread.coreOrdinal].groupOrdinal < layout.groups.size()
        && thread.groupOrdinal != layout.cores[thread.coreOrdinal].groupOrdinal)
    {
      std::fprintf(stderr, "a thread's group disagrees with its core\n");
      return 1;
    }
  }
  const auto mask = processMask();
  for (const auto& thread : layout.threads) {
    if (std::find(mask.begin(), mask.end(), thread.cpuIndex) == mask.end()) {
      std::fprintf(stderr, "a thread sits outside the process mask\n");
      return 1;
    }
  }
  for (std::size_t i = 1; i < layout.threads.size(); ++i) {
    if (layout.threads[i].cpuIndex <= layout.threads[i - 1].cpuIndex) {
      std::fprintf(stderr, "threads are not ordered by ascending cpu index\n");
      return 1;
    }
  }
  for (const auto& core : layout.cores) {
    if (core.threads.empty()) {
      std::fprintf(stderr, "a core holds no thread\n");
      return 1;
    }
    for (std::size_t i = 1; i < core.threads.size(); ++i) {
      if (layout.threads[core.threads[i]].cpuIndex
          <= layout.threads[core.threads[i - 1]].cpuIndex)
      {
        std::fprintf(stderr, "a core's siblings are not ascending\n");
        return 1;
      }
    }
  }
  for (std::size_t g = 1; g < layout.groups.size(); ++g) {
    if (layout.groups[g].lowestCpu <= layout.groups[g - 1].lowestCpu) {
      std::fprintf(stderr, "groups are not ordered by their lowest cpu\n");
      return 1;
    }
    if (layout.groups[g].cores.empty()) {
      std::fprintf(stderr, "a group holds no core\n");
      return 1;
    }
  }
  if (!layout.cores.empty()) {
    std::size_t siblings = 0;
    for (const auto& core : layout.cores) {
      siblings += core.threads.size();
    }
    if (siblings != layout.threads.size()) {
      std::fprintf(stderr, "the sibling sum disagrees with the thread count\n");
      return 1;
    }
  }
  return 0;
}

// FR-005: a mask narrowed in-process lowers the counts to what the mask
// holds, and the layout's mask equals the narrowed set.
auto childMaskNarrowed() -> int
{
  cpu_set_t narrow {};
  CPU_ZERO(&narrow);
  for (const std::size_t cpu : {0U, 1U, 2U, 3U}) {
    CPU_SET(cpu, &narrow);
  }
  if (sched_setaffinity(0, sizeof(narrow), &narrow) != 0) {
    std::fprintf(stderr, "sched_setaffinity failed\n");
    return 1;
  }
  const auto& layout = sg::detail::layout();
  if (!layout.readable) {
    std::fprintf(stderr, "the narrowed host read failed\n");
    return 1;
  }
  if (layout.mask != std::vector<std::uint32_t> {0, 1, 2, 3}) {
    std::fprintf(stderr, "the layout mask differs from the narrowed set\n");
    return 1;
  }
  if (layout.threads.size() != 4 || layout.cores.empty()
      || layout.cores.size() > 4 || layout.groups.empty()
      || layout.groups.size() > 4)
  {
    std::fprintf(stderr, "the narrowed counts did not lower to the mask\n");
    return 1;
  }
  for (const auto& thread : layout.threads) {
    if (thread.cpuIndex > 3) {
      std::fprintf(stderr, "a thread escaped the narrowed mask\n");
      return 1;
    }
  }
  return 0;
}

// R-04, Q-8: the synthetic description `1` yields no core and no group and
// one thread under a mask holding cpu 0.
auto childSyntheticOne() -> int
{
  sg::detail::installTestTopologyDescription(std::string("1"));
  const auto& layout = sg::detail::layout();
  if (!layout.readable || layout.threads.size() != 1 || !layout.cores.empty()
      || !layout.groups.empty())
  {
    std::fprintf(stderr, "the synthetic description 1 did not yield one\n");
    return 1;
  }
  return 0;
}

// FR-005, R-13: the same description under a mask holding every host cpu
// except cpu 0 intersects to nothing, so the walk yields a thread count
// of 0. The mask is read from the host because a fixed high range names
// cpus a four-cpu runner lacks.
auto childSyntheticNarrowed() -> int
{
  cpu_set_t current {};
  if (sched_getaffinity(0, sizeof(current), &current) != 0) {
    std::fprintf(stderr, "sched_getaffinity failed\n");
    return 1;
  }
  cpu_set_t narrow {};
  CPU_ZERO(&narrow);
  for (std::size_t cpu = 1U; cpu < CPU_SETSIZE; ++cpu) {
    if (CPU_ISSET(cpu, &current)) {
      CPU_SET(cpu, &narrow);
    }
  }
  if (CPU_COUNT(&narrow) == 0) {
    std::fprintf(stderr, "the host holds no cpu beside cpu 0\n");
    return 1;
  }
  if (sched_setaffinity(0, sizeof(narrow), &narrow) != 0) {
    std::fprintf(stderr, "sched_setaffinity failed\n");
    return 1;
  }
  sg::detail::installTestTopologyDescription(std::string("1"));
  const auto& layout = sg::detail::layout();
  if (!layout.readable || !layout.threads.empty() || !layout.mask.empty()) {
    std::fprintf(stderr, "the narrowed synthetic read kept a thread\n");
    return 1;
  }
  return 0;
}

// FR-005, R-04: a malformed description leaves readable false with no
// partial layout.
auto childMalformed() -> int
{
  sg::detail::installTestTopologyDescription(std::string("not a topology"));
  const auto& layout = sg::detail::layout();
  if (layout.readable || !layout.threads.empty() || !layout.mask.empty()) {
    std::fprintf(stderr, "a malformed description produced a partial layout\n");
    return 1;
  }
  return 0;
}

// Review decision D-1: the installed failure answers the load call in
// place of hwloc, so the failed-load arm leaves readable false with no
// partial layout on every platform.
auto childLoadFailed() -> int
{
  sg::detail::installTestLoadFailure(1);
  const auto& layout = sg::detail::layout();
  if (layout.readable || !layout.threads.empty() || !layout.mask.empty()) {
    std::fprintf(stderr, "a failed load produced a partial layout\n");
    return 1;
  }
  return 0;
}

// FR-002, E-09: the synthetic description `Package:2 Core:2 PU:2`
// describes packages, cores and processing units but no cache level, so
// every core closes with no group and the group list stays empty. The
// description scales with the host: eight cpus keep the two-sibling
// shape, four cpus take the single-sibling shape, two cpus take the
// one-package shape, because the synthetic cpus intersect with the
// process affinity like any other read.
auto childNoGroup() -> int
{
  cpu_set_t host {};
  if (sched_getaffinity(0, sizeof(host), &host) != 0) {
    std::fprintf(stderr, "sched_getaffinity failed\n");
    return 1;
  }
  const auto hostCpus = static_cast<std::size_t>(CPU_COUNT(&host));
  if (hostCpus < 2) {
    std::fprintf(stderr, "the host holds fewer than two cpus\n");
    return 1;
  }
  const char* const description = hostCpus >= 8
      ? "Package:2 Core:2 PU:2"
      : (hostCpus >= 4 ? "Package:2 Core:2 PU:1" : "Package:1 Core:2 PU:1");
  const auto expectedThreads = hostCpus >= 8 ? 8U : (hostCpus >= 4 ? 4U : 2U);
  const auto expectedCores = hostCpus >= 4 ? 4U : 2U;
  sg::detail::installTestTopologyDescription(std::string(description));
  const auto& layout = sg::detail::layout();
  if (!layout.readable || layout.cores.size() != expectedCores
      || !layout.groups.empty() || layout.threads.size() != expectedThreads)
  {
    std::fprintf(stderr, "the groupless synthetic did not yield its cores\n");
    return 1;
  }
  return 0;
}

// FR-036: a mask of single cpus prints each one alone, and a group whose
// cpus are single prints the same way.
auto childListingGap() -> int
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (const std::uint32_t cpu : {0U, 2U, 5U}) {
    sg::detail::LayoutCore core;
    core.groupOrdinal = 0;
    sg::detail::LayoutThread thread {
        cpu, static_cast<std::uint32_t>(installed.cores.size()), 0};
    core.threads.push_back(
        static_cast<std::uint32_t>(installed.threads.size()));
    installed.threads.push_back(thread);
    installed.cores.push_back(core);
    installed.mask.push_back(cpu);
  }
  sg::detail::LayoutGroup group;
  group.lowestCpu = 0;
  group.cores = {0, 1, 2};
  installed.groups.push_back(group);
  sg::detail::installTestLayout(installed);
  sg::detail::printTopologyListing(sg::detail::layout());
  return 0;
}

// FR-002, R-04: two calls yield the same layout, and an install after the
// first call changes nothing.
auto childReadOnce() -> int
{
  const auto* const first = &sg::detail::layout();
  const auto* const second = &sg::detail::layout();
  if (first != second) {
    std::fprintf(stderr, "layout() read twice\n");
    return 1;
  }
  installShape(4, 24, 2);
  if (&sg::detail::layout() != first
      || sg::detail::layout().threads.size() != first->threads.size())
  {
    std::fprintf(stderr,
                 "an install after the first read changed the layout\n");
    return 1;
  }
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc > 1) {
    const std::string_view mode {argv[1]};
    if (mode == "hostRead") {
      return childHostRead();
    }
    if (mode == "maskNarrowed") {
      return childMaskNarrowed();
    }
    if (mode == "installedOne") {
      return childInstalled(1, 1, 1);
    }
    if (mode == "installedFour") {
      return childInstalled(4, 6, 2);
    }
    if (mode == "installedSiblings") {
      return childInstalled(4, 6, 1);
    }
    if (mode == "syntheticOne") {
      return childSyntheticOne();
    }
    if (mode == "syntheticNarrowed") {
      return childSyntheticNarrowed();
    }
    if (mode == "malformed") {
      return childMalformed();
    }
    if (mode == "noGroup") {
      return childNoGroup();
    }
    if (mode == "listingGap") {
      return childListingGap();
    }
    if (mode == "loadFailed") {
      return childLoadFailed();
    }
    if (mode == "readOnce") {
      return childReadOnce();
    }
    fail("unknown child mode");
  }

  struct Expected
  {
    const char* mode;
    const char* what;
  };

  const std::array<Expected, 12> scenarios {
      Expected {
          "hostRead",
          "the host read scenario ended clean (FR-001, " "FR-005, R-11, R-13)"},
      Expected {"maskNarrowed",
                "the narrowed-mask scenario ended clean (FR-005)"},
      Expected {"installedOne",
                "the one-group installed scenario ended clean (SC-04)"},
      Expected {"installedFour",
                "the four-group installed scenario ended clean (SC-04)"},
      Expected {"installedSiblings",
                "the siblings-off installed scenario ended clean (SC-04)"},
      Expected {"syntheticOne",
                "the synthetic description scenario ended clean (R-04, Q-8)"},
      Expected {"syntheticNarrowed",
                "the narrowed synthetic scenario ended clean (FR-005, R-13)"},
      Expected {
          "malformed",
          "the malformed description scenario ended clean (FR-005, R-04)"},
      Expected {"loadFailed",
                "the failed load scenario ended clean (FR-005, D-1)"},
      Expected {"noGroup",
                "the groupless synthetic scenario ended clean (FR-002, E-09)"},
      Expected {"listingGap",
                "the single-cpu listing scenario ended clean (FR-036)"},
      Expected {"readOnce",
                "the read-once scenario ended clean (FR-002, R-04)"},
  };
  for (const auto& scenario : scenarios) {
    const int status = runChild(argv[0], scenario.mode);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, scenario.what);
  }

  std::printf("harness_topology_test: all scenarios passed\n");
  return 0;
}
