// ============================================================================
// TDD test for the preset resolver and the runner wiring (T008, T009; US1,
// FR-008 to FR-019, FR-021, E-09, SC-02, SC-03, SC-04, R-05).
//
// One layout read per process (FR-002) means each installed layout serves
// exactly one scenario, so every scenario that installs a layout runs in a
// fresh process by the fork and execv pattern of
// harness_context_barrier_test.cpp:116-126, naming the scenario in argv and
// re-installing the seam in the child.
//
// The four injected layouts are the ones SC-04 names: one group holding one
// core with no sibling; four groups holding 24 cores in total with two
// siblings each (48 threads); the same layout with siblings off; and a mask
// holding part of the machine. The place counts of
// contracts/topology-placement.md's table apply to the second: l3 4, smt 2,
// core 24, compact 48 (SC-02).
//
// Scenario map:
//   coreFour            `core` gives the lowest-index thread of each
//                       distinct core, cores ascending by that thread
//                       (FR-011); the place count is 24 and a count of 25
//                       returns the empty plan naming 24 (FR-018)
//   l3Four              `l3` gives the lowest-index thread of each distinct
//                       group, groups ordered by their lowest thread
//                       (FR-012); the place count is 4 and a count of 5
//                       returns the empty plan naming 4 (FR-018)
//   smtFour             `smt` gives the siblings of the core holding the
//                       lowest-index thread and no other core (FR-013); the
//                       place count is 2 and a count of 3 returns the empty
//                       plan naming 2 (FR-018)
//   compactFour         `compact` fills every sibling of a core before the
//                       next core and every core of a group before the next
//                       group (FR-014); the place count is 48
//   singleCore          over one group holding one core with no sibling
//                       every preset places exactly one context (SC-04)
//   siblingsOff         over the four-group layout with siblings off, smt
//                       places 1 and compact places 24 (SC-04)
//   partialMask         over a mask holding part of the machine the place
//                       counts lower to what the mask holds (FR-005, SC-04)
//   determinism         two calls with one layout and one count return
//                       equal vectors (SC-03, FR-009), and every plan
//                       holds pairwise distinct cpus (FR-008)
//   nonePreset          `none` produces no plan (FR-010)
//   textGroup           placementText over a layout whose thread names a
//                       group returns `cpu@group` (FR-021, E-09)
//   textNoGroup         placementText over a layout with no group for that
//                       cpu returns the cpu index alone (FR-021, E-09)
//   textNoPin           placementText for kNoPin returns NO_PIN (FR-021,
//                       E-09)
//   runnerL3            with an installed layout and an installed
//                       placement function, a run of four contexts under
//                       `l3` calls the replacement with slots naming the
//                       four planned cpus in context-index order (FR-008,
//                       FR-010, R-05)
//   runnerNoPreset      a run with no preset calls the replacement with
//                       noPin() as feature 020 ships (FR-010, FR-019)
//   parsePreset         the five names parse, and `l2`, `0-3`, `0` and an
//                       empty text are refused (FR-016, FR-017)
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PLACEMENT PRESET TEST FAIL: %s\n", what);
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

// The four-group, 24-core, 48-thread layout of SC-04 and the contract table.
auto installFourGroupShape() -> void
{
  installShape(4, 6, 2);
}

auto distinctCpus(const std::vector<std::uint32_t>& cpus) -> bool
{
  for (std::size_t i = 1; i < cpus.size(); ++i) {
    const auto head = cpus.begin() + static_cast<std::ptrdiff_t>(i);
    if (std::find(cpus.begin(), head, cpus[i]) != head) {
      return false;
    }
  }
  return true;
}

// FR-011: `core` gives the lowest-index thread of each distinct core, cores
// ascending by that thread; the fit rule at 25 names 24 (FR-018).
auto childCoreFour() -> int
{
  installFourGroupShape();
  const auto& layout = sg::detail::layout();
  std::vector<std::uint32_t> expected;
  for (std::uint32_t core = 0; core < 24; ++core) {
    expected.push_back((core / 6) * 12 + (core % 6) * 2);
  }
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::CORE, layout, 24);
  if (plan.places != 24 || plan.cpu != expected
      || plan.places != plan.cpu.size())
  {
    std::fprintf(stderr, "core did not give the lowest thread of each core\n");
    return 1;
  }
  const auto over =
      sg::detail::resolvePreset(sg::detail::Preset::CORE, layout, 25);
  if (!over.cpu.empty() || !over.group.empty() || over.places != 24) {
    std::fprintf(stderr,
                 "the fit rule did not return the empty plan naming 24\n");
    return 1;
  }
  return 0;
}

// FR-012: `l3` gives the lowest-index thread of each distinct group, groups
// ordered by their lowest thread; the fit rule at 5 names 4 (FR-018).
auto childL3Four() -> int
{
  installFourGroupShape();
  const auto& layout = sg::detail::layout();
  const std::vector<std::uint32_t> expected {0, 12, 24, 36};
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::L3, layout, 4);
  if (plan.places != 4 || plan.cpu != expected) {
    std::fprintf(stderr, "l3 did not give the lowest thread of each group\n");
    return 1;
  }
  const auto over =
      sg::detail::resolvePreset(sg::detail::Preset::L3, layout, 5);
  if (!over.cpu.empty() || over.places != 4) {
    std::fprintf(stderr,
                 "the fit rule did not return the empty plan naming 4\n");
    return 1;
  }
  return 0;
}

// FR-013: `smt` gives the siblings of the core holding the lowest-index
// thread and no other core; the fit rule at 3 names 2 (FR-018).
auto childSmtFour() -> int
{
  installFourGroupShape();
  const auto& layout = sg::detail::layout();
  const std::vector<std::uint32_t> expected {0, 1};
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::SMT, layout, 2);
  if (plan.places != 2 || plan.cpu != expected) {
    std::fprintf(stderr, "smt did not give the siblings of one core\n");
    return 1;
  }
  const auto over =
      sg::detail::resolvePreset(sg::detail::Preset::SMT, layout, 3);
  if (!over.cpu.empty() || over.places != 2) {
    std::fprintf(stderr,
                 "the fit rule did not return the empty plan naming 2\n");
    return 1;
  }
  return 0;
}

// FR-014: `compact` fills every sibling of a core before the next core and
// every core of a group before the next group; the place count is 48.
auto childCompactFour() -> int
{
  installFourGroupShape();
  const auto& layout = sg::detail::layout();
  std::vector<std::uint32_t> expected;
  for (std::uint32_t cpu = 0; cpu < 48; ++cpu) {
    expected.push_back(cpu);
  }
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 48);
  if (plan.places != 48 || plan.cpu != expected) {
    std::fprintf(stderr, "compact did not fill cores before groups\n");
    return 1;
  }
  const auto three =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 3);
  const std::vector<std::uint32_t> firstThree {0, 1, 2};
  if (three.cpu != firstThree) {
    std::fprintf(stderr,
                 "compact did not name both siblings before the next core\n");
    return 1;
  }
  return 0;
}

// SC-04: over one group holding one core with no sibling every preset
// places exactly one context.
auto childSingleCore() -> int
{
  installShape(1, 1, 1);
  const auto& layout = sg::detail::layout();
  const std::array<sg::detail::Preset, 4> presets {sg::detail::Preset::CORE,
                                                   sg::detail::Preset::L3,
                                                   sg::detail::Preset::SMT,
                                                   sg::detail::Preset::COMPACT};
  for (const auto preset : presets) {
    const auto plan = sg::detail::resolvePreset(preset, layout, 1);
    const std::vector<std::uint32_t> expected {0};
    if (plan.places != 1 || plan.cpu != expected) {
      std::fprintf(stderr, "a preset did not place exactly one context\n");
      return 1;
    }
    const auto over = sg::detail::resolvePreset(preset, layout, 2);
    if (!over.cpu.empty() || over.places != 1) {
      std::fprintf(stderr, "the fit rule did not lower to the single place\n");
      return 1;
    }
  }
  return 0;
}

// SC-04: over the four-group layout with siblings off, smt places 1 and
// compact places 24.
auto childSiblingsOff() -> int
{
  installShape(4, 6, 1);
  const auto& layout = sg::detail::layout();
  const auto smt =
      sg::detail::resolvePreset(sg::detail::Preset::SMT, layout, 1);
  const std::vector<std::uint32_t> one {0};
  if (smt.places != 1 || smt.cpu != one) {
    std::fprintf(stderr, "smt did not place one context with siblings off\n");
    return 1;
  }
  const auto compact =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 24);
  if (compact.places != 24 || compact.cpu.size() != 24
      || !distinctCpus(compact.cpu))
  {
    std::fprintf(stderr,
                 "compact did not place 24 contexts with siblings off\n");
    return 1;
  }
  return 0;
}

// FR-005, SC-04: over a mask holding part of the machine the place counts
// lower to what the mask holds.
auto childPartialMask() -> int
{
  installShape(1, 6, 2);
  const auto& layout = sg::detail::layout();
  const auto core =
      sg::detail::resolvePreset(sg::detail::Preset::CORE, layout, 6);
  const std::vector<std::uint32_t> coreExpected {0, 2, 4, 6, 8, 10};
  if (core.places != 6 || core.cpu != coreExpected) {
    std::fprintf(stderr, "core did not lower to the partial mask\n");
    return 1;
  }
  const auto l3 = sg::detail::resolvePreset(sg::detail::Preset::L3, layout, 1);
  const std::vector<std::uint32_t> one {0};
  if (l3.places != 1 || l3.cpu != one) {
    std::fprintf(stderr, "l3 did not lower to the partial mask\n");
    return 1;
  }
  const auto smt =
      sg::detail::resolvePreset(sg::detail::Preset::SMT, layout, 2);
  const std::vector<std::uint32_t> pair {0, 1};
  if (smt.places != 2 || smt.cpu != pair) {
    std::fprintf(stderr, "smt did not lower to the partial mask\n");
    return 1;
  }
  const auto compact =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 12);
  if (compact.places != 12 || compact.cpu.size() != 12
      || !distinctCpus(compact.cpu))
  {
    std::fprintf(stderr, "compact did not lower to the partial mask\n");
    return 1;
  }
  return 0;
}

// SC-03, FR-009, FR-008: two calls return equal vectors, and every plan
// holds pairwise distinct cpus.
auto childDeterminism() -> int
{
  installFourGroupShape();
  const auto& layout = sg::detail::layout();
  const auto first =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 48);
  const auto second =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 48);
  if (first.cpu != second.cpu || first.group != second.group) {
    std::fprintf(stderr,
                 "two calls with one layout returned different plans\n");
    return 1;
  }
  const auto core =
      sg::detail::resolvePreset(sg::detail::Preset::CORE, layout, 24);
  if (!distinctCpus(core.cpu)) {
    std::fprintf(stderr, "a plan repeated a cpu\n");
    return 1;
  }
  for (const auto cpu : core.cpu) {
    if (std::find(layout.mask.begin(), layout.mask.end(), cpu)
        == layout.mask.end())
    {
      std::fprintf(stderr, "a plan cpu stands outside the layout mask\n");
      return 1;
    }
  }
  return 0;
}

// FR-010: `none` produces no plan.
auto childNonePreset() -> int
{
  installFourGroupShape();
  const auto& layout = sg::detail::layout();
  const auto plan =
      sg::detail::resolvePreset(sg::detail::Preset::NONE, layout, 4);
  if (!plan.cpu.empty() || !plan.group.empty()) {
    std::fprintf(stderr, "none produced a plan\n");
    return 1;
  }
  return 0;
}

// FR-018 at its extreme: a readable layout holding no thread places
// nothing for every preset, the empty plan carrying the count zero the
// fit message prints.
auto childEmptyLayout() -> int
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  sg::detail::installTestLayout(installed);
  const auto& layout = sg::detail::layout();
  const std::array<sg::detail::Preset, 4> presets {sg::detail::Preset::CORE,
                                                   sg::detail::Preset::L3,
                                                   sg::detail::Preset::SMT,
                                                   sg::detail::Preset::COMPACT};
  for (const auto preset : presets) {
    const auto plan = sg::detail::resolvePreset(preset, layout, 1);
    if (!plan.cpu.empty() || plan.places != 0) {
      std::fprintf(stderr, "an empty layout placed a context\n");
      return 1;
    }
  }
  return 0;
}

// FR-021, E-09: a thread that names a group discloses `cpu@group`.
auto childTextGroup() -> int
{
  installFourGroupShape();
  if (sg::detail::placementText(sg::PlacementSlot::cpu(0)) != "0@0"
      || sg::detail::placementText(sg::PlacementSlot::cpu(24)) != "24@2"
      || sg::detail::placementText(sg::PlacementSlot::cpu(35)) != "35@2")
  {
    std::fprintf(stderr, "placementText did not disclose cpu@group\n");
    return 1;
  }
  return 0;
}

// FR-021, E-09: a cpu with no group discloses the cpu index alone.
auto childTextNoGroup() -> int
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  const sg::detail::LayoutThread thread {5, 0, sg::detail::kNoGroup};
  installed.threads.push_back(thread);
  installed.mask.push_back(5);
  sg::detail::installTestLayout(installed);
  if (sg::detail::placementText(sg::PlacementSlot::cpu(5)) != "5"
      || sg::detail::placementText(sg::PlacementSlot::cpu(99)) != "99")
  {
    std::fprintf(stderr, "placementText did not disclose the bare cpu index\n");
    return 1;
  }
  return 0;
}

// FR-021, E-09: kNoPin discloses NO_PIN.
auto childTextNoPin() -> int
{
  installFourGroupShape();
  if (sg::detail::placementText(sg::PlacementSlot::noPin()) != "NO_PIN") {
    std::fprintf(stderr, "placementText did not disclose NO_PIN\n");
    return 1;
  }
  return 0;
}

constexpr std::string_view kRecordedSlot = "RECORDED";

// FR-011, FR-013, FR-014, FR-018 (T023, US4): on one two-sibling layout,
// smt names the two siblings of one core with two contexts, core names two
// different cores with two contexts, compact with three contexts names both
// siblings of one core plus the lowest thread of the next core, and smt
// with three contexts returns no plan.
auto childThreeShapes() -> int
{
  installShape(2, 3, 2);
  const auto& layout = sg::detail::layout();
  const auto smt =
      sg::detail::resolvePreset(sg::detail::Preset::SMT, layout, 2);
  const std::vector<std::uint32_t> siblings {0, 1};
  if (smt.cpu != siblings) {
    std::fprintf(stderr, "smt did not name the two siblings of one core\n");
    return 1;
  }
  const auto core =
      sg::detail::resolvePreset(sg::detail::Preset::CORE, layout, 2);
  const std::vector<std::uint32_t> twoCores {0, 2};
  if (core.cpu != twoCores) {
    std::fprintf(stderr, "core did not name two different cores\n");
    return 1;
  }
  const auto compact =
      sg::detail::resolvePreset(sg::detail::Preset::COMPACT, layout, 3);
  const std::vector<std::uint32_t> three {0, 1, 2};
  if (compact.cpu != three) {
    std::fprintf(stderr, "compact did not fill one core before the next\n");
    return 1;
  }
  const auto over =
      sg::detail::resolvePreset(sg::detail::Preset::SMT, layout, 3);
  if (!over.cpu.empty()) {
    std::fprintf(stderr, "smt with three contexts returned a plan\n");
    return 1;
  }
  return 0;
}

std::atomic<int> slotCalls {0};
std::array<std::uint32_t, 8> slotRecord {};

auto recordSlot(const sg::PlacementSlot& slot,
                std::thread::native_handle_type) -> sg::PlacementResult
{
  const int index = slotCalls.fetch_add(1);
  if (index >= 0 && static_cast<std::size_t>(index) < slotRecord.size()) {
    slotRecord[static_cast<std::size_t>(index)] = slot.cpuIndex;
  }
  return {.applied = true, .description = kRecordedSlot};
}

auto bmPresetBody(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto findInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  return nullptr;
}

// The scripted clock the runner scenarios need: the runner asserts the
// host publishes machine/monotonic, and a threaded run reads thread_cpu
// beside it (FR-002).
auto installScriptedClock() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, 1'000);
  provider.setPoints("machine", "thread_cpu", {0}, 1'000);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));
}

// FR-008, FR-010, R-05: a run of four contexts under `l3` calls the
// installed placement function with slots naming the four planned cpus in
// context-index order.
auto childRunnerL3() -> int
{
  installFourGroupShape();
  installScriptedClock();
  sg::setPlacementFunction(&recordSlot);
  auto handle = sg::registerBenchmark(&bmPresetBody, "bmPresetL3");
  handle.threads(4).placement("l3");
  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = 8;
  sg::detail::Runner runner(options);
  sg::Instance* const instance = findInstance("bmPresetL3/threads:4");
  if (instance == nullptr) {
    std::fprintf(stderr, "the expansion produced no instance\n");
    return 1;
  }
  (void)runner.run(*instance);
  std::vector<std::uint32_t> recorded;
  for (int index = 0; index < slotCalls.load(); ++index) {
    recorded.push_back(slotRecord[static_cast<std::size_t>(index)]);
  }
  std::ranges::sort(recorded);
  const std::vector<std::uint32_t> expected {0, 12, 24, 36};
  if (recorded != expected) {
    std::fprintf(stderr, "the run did not place the four planned cpus\n");
    return 1;
  }
  return 0;
}

// FR-010, FR-019: a run with no preset calls the placement function with
// noPin() as feature 020 ships.
auto childRunnerNoPreset() -> int
{
  installFourGroupShape();
  installScriptedClock();
  sg::setPlacementFunction(&recordSlot);
  auto handle = sg::registerBenchmark(&bmPresetBody, "bmPresetNoPreset");
  handle.threads(4);
  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = 8;
  sg::detail::Runner runner(options);
  sg::Instance* const instance = findInstance("bmPresetNoPreset/threads:4");
  if (instance == nullptr) {
    std::fprintf(stderr, "the expansion produced no instance\n");
    return 1;
  }
  (void)runner.run(*instance);
  if (slotCalls.load() != 4) {
    std::fprintf(stderr, "the run placed a different number of contexts\n");
    return 1;
  }
  for (int index = 0; index < slotCalls.load(); ++index) {
    if (slotRecord[static_cast<std::size_t>(index)]
        != sg::PlacementSlot::noPin().cpuIndex)
    {
      std::fprintf(stderr, "a run with no preset named a cpu\n");
      return 1;
    }
  }
  return 0;
}

// FR-016, FR-017: the five names parse; `l2`, `0-3`, `0` and an empty text
// are refused.
auto parsePresetScenarios() -> int
{
  struct Named
  {
    const char* text;
    sg::detail::Preset preset;
  };

  const std::array<Named, 5> accepted {
      Named {"none", sg::detail::Preset::NONE},
      Named {"core", sg::detail::Preset::CORE},
      Named {"l3", sg::detail::Preset::L3},
      Named {"smt", sg::detail::Preset::SMT},
      Named {"compact", sg::detail::Preset::COMPACT}};
  for (const auto& named : accepted) {
    const auto parsed = sg::detail::parsePreset(named.text);
    check(parsed.has_value() && *parsed == named.preset,
          "the preset name parses");
  }
  const std::array<const char*, 6> refused {
      "l2", "0-3", "0", "", " core", "core "};
  for (const char* text : refused) {
    check(!sg::detail::parsePreset(text).has_value(),
          "a name outside the five is refused");
  }
  std::puts("harness_placement_preset_test parsePreset: ok");
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc > 1) {
    const std::string_view mode {argv[1]};
    if (mode == "coreFour") {
      return childCoreFour();
    }
    if (mode == "l3Four") {
      return childL3Four();
    }
    if (mode == "smtFour") {
      return childSmtFour();
    }
    if (mode == "compactFour") {
      return childCompactFour();
    }
    if (mode == "singleCore") {
      return childSingleCore();
    }
    if (mode == "siblingsOff") {
      return childSiblingsOff();
    }
    if (mode == "partialMask") {
      return childPartialMask();
    }
    if (mode == "determinism") {
      return childDeterminism();
    }
    if (mode == "nonePreset") {
      return childNonePreset();
    }
    if (mode == "threeShapes") {
      return childThreeShapes();
    }
    if (mode == "textGroup") {
      return childTextGroup();
    }
    if (mode == "textNoGroup") {
      return childTextNoGroup();
    }
    if (mode == "textNoPin") {
      return childTextNoPin();
    }
    if (mode == "emptyLayout") {
      return childEmptyLayout();
    }
    if (mode == "runnerL3") {
      return childRunnerL3();
    }
    if (mode == "runnerNoPreset") {
      return childRunnerNoPreset();
    }
    fail("unknown child mode");
  }

  struct Expected
  {
    const char* mode;
    const char* what;
  };

  const std::array<Expected, 16> scenarios {
      Expected {"coreFour", "the core scenario ended clean (FR-011, FR-018)"},
      Expected {"l3Four", "the l3 scenario ended clean (FR-012, FR-018)"},
      Expected {"smtFour", "the smt scenario ended clean (FR-013, FR-018)"},
      Expected {"compactFour", "the compact scenario ended clean (FR-014)"},
      Expected {"singleCore", "the single-core scenario ended clean (SC-04)"},
      Expected {"siblingsOff", "the siblings-off scenario ended clean (SC-04)"},
      Expected {"partialMask",
                "the partial-mask scenario ended clean (FR-005)"},
      Expected {
          "determinism",
          "the determinism scenario ended clean (SC-03, " "FR-008, " "FR-009)"},
      Expected {"nonePreset", "the none scenario ended clean (FR-010)"},
      Expected {
          "threeShapes",
          "the three-shape scenario ended clean (FR-011, FR-013, " "FR-014," " " "FR-" "018" ")"},
      Expected {"textGroup",
                "the cpu@group scenario ended clean (FR-021, E-09)"},
      Expected {"textNoGroup",
                "the bare-cpu scenario ended clean (FR-021, E-09)"},
      Expected {"textNoPin", "the NO_PIN scenario ended clean (FR-021, E-09)"},
      Expected {"emptyLayout",
                "the empty-layout scenario ended clean (FR-018)"},
      Expected {"runnerL3",
                "the l3 runner scenario ended clean (FR-008, FR-010, R-05)"},
      Expected {"runnerNoPreset",
                "the no-preset runner scenario ended clean (FR-010, FR-019)"},
  };
  for (const auto& scenario : scenarios) {
    const int status = runChild(argv[0], scenario.mode);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, scenario.what);
  }

  check(parsePresetScenarios() == 0, "the parsePreset scenario ended clean");

  std::printf("harness_placement_preset_test: all scenarios passed\n");
  return 0;
}
