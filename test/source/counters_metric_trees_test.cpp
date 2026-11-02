// ============================================================================
// TDD test for the host-fact readers over an object tree that answers
// differently (specs/022-metric-formulas T058; specs/042-topdown T004,
// T005; FR-006, FR-042, FR-043, FR-054).
//
// The lookup binary reads `#SMT_on` over the two-thread, one-core tree
// where the fact stands. This binary opens its own process and registers a
// tree that carries a thread but no core, so the core count is the missing
// half of the SMT fact and the sibling list of one cpu answers the fact
// under the root the seam installs. One child process registers the
// two-thread, one-core tree and shows that tree keep priority over a list
// naming one id. The `#core_wide` scenarios read this process's own cpu set
// beside the sibling lists they drive, and the three constants no top-down
// row reaches keep their refusal. A wrong answer never reaches the fold.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "speedgun-ng/counters.hpp"

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../source/counters/linux_pmu/metric_tables.hpp"
#include "../../source/detail/proc_root.hpp"
#include "detail/metric_fixture.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "METRIC TREE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The two SMT scenarios below read files under a fake root, and the seam
// installs that root in place of `/` (specs/042-topdown D-19, FR-054). The
// root stands cleared when the scenario ends, the way the standing install
// seams stand cleared by the test (R-02).
class ProcRootGuard
{
public:
  explicit ProcRootGuard(const std::filesystem::path& root)
      : m_root(root)
  {
    sg::detail::installTestProcRoot(root);
  }

  ~ProcRootGuard()
  {
    sg::detail::installTestProcRoot(std::nullopt);
    std::error_code removeFailed;
    std::filesystem::remove_all(m_root, removeFailed);
  }

  ProcRootGuard(const ProcRootGuard&) = delete;
  auto operator=(const ProcRootGuard&) -> ProcRootGuard& = delete;

private:
  std::filesystem::path m_root;
};

// One fake root per scenario, under the working directory the test binaries
// own and named with the pid and the scenario, so no scenario reads the
// files another wrote and a parallel run stands apart (the shape of the
// pre-check suite's fake root).
[[nodiscard]] auto fakeRoot(const std::string_view name)
    -> std::filesystem::path
{
  const auto path = std::filesystem::path("counters_metric_trees_root_")
      / ("pid" + std::to_string(::getpid()) + "-" + std::string(name));
  std::error_code stale;
  std::filesystem::remove_all(path, stale);
  check(std::filesystem::create_directories(path),
        "the fake root stands created (T004)");
  return path;
}

// The cpus this process may run on. The reader asks for the cpu
// `::sched_getcpu()` names, and the scenarios write a file for every cpu the
// affinity mask holds, so the read lands on a file whichever cpu the
// scheduler answers.
[[nodiscard]] auto processCpus() -> std::vector<std::size_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  check(::sched_getaffinity(0, sizeof(mask), &mask) == 0,
        "the process answers its own affinity mask (T004)");
  std::vector<std::size_t> cpus;
  // `/usr/include/bits/cpu-set.h` binds the operand of `__CPU_ISSET_S` to
  // `size_t`, so the walk counts in that type and no cast stands in it.
  for (std::size_t cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
    if (CPU_ISSET(cpu, &mask) != 0) {
      cpus.push_back(cpu);
    }
  }
  check(!cpus.empty(), "the affinity mask holds one cpu at least (T004)");
  return cpus;
}

// One sibling list of one cpu under one fake root, the text written whole.
// The reader joins the path under the installed root, so the file stands at
// `sys/devices/system/cpu/cpu<N>/topology/<file>` (D-19).
auto writeSiblingList(const std::filesystem::path& root,
                      const std::size_t cpu,
                      const std::string_view file,
                      const std::string_view text) -> void
{
  const auto directory = root / "sys/devices/system/cpu"
      / ("cpu" + std::to_string(cpu)) / "topology";
  std::error_code missed;
  std::filesystem::create_directories(directory, missed);
  std::ofstream out(directory / std::string(file));
  out << text << "\n";
  check(out.good() && std::filesystem::is_directory(directory),
        "the fake sibling list stands written (T004)");
}

// The value one `#` constant leaves on the fold. Two probe rows read the
// constant, and each divides its else-arm by `#has_pmem`, a constant no host
// answers. The fold walks the if-arm when a test settles true and the
// else-arm when it settles false (FR-005, FR-006), so the `nonzero` row
// publishes over every value but 0 and the `past_one` row over every value
// but 1. The pair separates 0 from 1 from a wider number, and a constant
// that answers nothing refuses both rows with its own name in the text.
enum class FactAnswer
{
  NOTHING,
  ZERO,
  ONE,
  OTHER,
};

[[nodiscard]] auto answerOf(const std::string_view constant) -> FactAnswer
{
  const std::string name {constant};
  const std::string refused = "fixture_cycles / #has_pmem";
  const auto rows = std::vector<sg::test::MetricRow> {
      sg::test::fixtureEvent(sg::test::kCycles),
      sg::test::fixtureMetric(
          "nonzero", "fixture_cycles if #" + name + " else " + refused),
      sg::test::fixtureMetric(
          "past_one", "fixture_cycles if #" + name + " - 1 else " + refused),
  };
  const auto table = sg::counters::detail::parseMetricRows(rows);
  const auto nonzero = sg::counters::detail::metricDefinition(
      table, sg::test::fixtureEvents(), sg::test::fixtureHost(), "nonzero");
  const auto pastOne = sg::counters::detail::metricDefinition(
      table, sg::test::fixtureEvents(), sg::test::fixtureHost(), "past_one");
  const auto namesConstant = [&name](const auto& settled)
  {
    return !settled.has_value()
        && settled.error().message.find(name) != std::string::npos;
  };
  if (namesConstant(nonzero) && namesConstant(pastOne)) {
    return FactAnswer::NOTHING;
  }
  if (nonzero.has_value() && pastOne.has_value()) {
    return FactAnswer::OTHER;
  }
  return nonzero.has_value() ? FactAnswer::ONE : FactAnswer::ZERO;
}

auto smtFactRefusalScenario() -> void
{
  if (!sg::test::registerMetricHost(sg::test::MetricTree::CORELESS)) {
    fail("the fixture provider could not register before the system opened");
  }
  // T015 puts a sibling list behind this fact, so the root stands empty and
  // both of the two files answer nothing. The refusal below then stands on
  // every host, because a host tree reaches no further than the installed
  // root (FR-006, specs/042-topdown FR-054).
  const auto root = fakeRoot("no-list");
  const ProcRootGuard guard {root};
  const auto rows = std::vector<sg::test::MetricRow> {
      sg::test::fixtureEvent(sg::test::kCycles),
      sg::test::fixtureMetric(
          "smt_row", "fixture_cycles if #SMT_on else fixture_instructions"),
  };
  const auto table = sg::counters::detail::parseMetricRows(rows);
  const auto settled = sg::counters::detail::metricDefinition(
      table, sg::test::fixtureEvents(), sg::test::fixtureHost(), "smt_row");
  check(!settled.has_value()
            && settled.error().message.find("SMT_on") != std::string::npos,
        "a tree that carries threads but no cores answers no SMT fact, and "
        "the row that asks refuses (FR-006)");
}

// T004: the sibling-list fallback of `readSmtState` (FR-054, D-19). The
// coreless tree answers no core object, so the reader reaches for the
// sibling list of one cpu under the installed root.
auto smtStateFallsBackToTheSiblingList() -> void
{
  const auto cpus = processCpus();
  {
    const auto root = fakeRoot("core-list");
    const ProcRootGuard guard {root};
    for (const std::size_t cpu : cpus) {
      writeSiblingList(root, cpu, "core_cpus_list", "0,12");
    }
    check(answerOf("SMT_on") == FactAnswer::ONE,
          "a core_cpus_list of 0,12 names two cpus, so the SMT fact answers "
          "1 and the fold takes the on-arm (FR-054)");
  }
  {
    const auto root = fakeRoot("one-cpu");
    const ProcRootGuard guard {root};
    for (const std::size_t cpu : cpus) {
      writeSiblingList(root, cpu, "core_cpus_list", std::to_string(cpu));
    }
    check(answerOf("SMT_on") == FactAnswer::ZERO,
          "a sibling list naming one id answers the SMT fact 0 and the fold "
          "takes the off-arm (FR-054)");
  }
  {
    const auto root = fakeRoot("siblings-file");
    const ProcRootGuard guard {root};
    for (const std::size_t cpu : cpus) {
      writeSiblingList(root, cpu, "thread_siblings_list", "0-12");
    }
    check(answerOf("SMT_on") == FactAnswer::ONE,
          "thread_siblings_list answers when core_cpus_list stands "
          "unreadable, and a 0-12 run names two cpus (D-19, FR-054)");
  }
  {
    const auto root = fakeRoot("unreadable");
    const ProcRootGuard guard {root};
    check(answerOf("SMT_on") == FactAnswer::NOTHING,
          "a cpu whose two sibling files stand unreadable answers no SMT "
          "fact, and the constant keeps its refusal (FR-054)");
  }
}

// T005: `#core_wide` reads the cpu set of this process and the sibling list
// of every cpu that set holds (specs/042-topdown FR-042, FR-043, D-6). The
// mask is the one the host hands the run, so the lists name ids the mask
// holds and the answer stands still on a narrowed host.
auto coreWideAnswersFromTheProcessCpuSet() -> void
{
  const auto cpus = processCpus();
  const std::size_t lowest = cpus.front();
  const std::size_t highest = cpus.back();
  const auto holds = [&cpus](const std::size_t cpu)
  { return std::find(cpus.begin(), cpus.end(), cpu) != cpus.end(); };
  // The id past the widest this process holds, so it stands outside the set
  // whoever the host is.
  const auto outside = std::to_string(highest + 1);
  {
    // Every sibling of every cpu stands inside the set. The list of a cpu
    // whose successor also stands inside carries the range spelling, so one
    // run walks both spellings the kernel prints (FR-042).
    const auto root = fakeRoot("wide-whole");
    const ProcRootGuard guard {root};
    for (const std::size_t cpu : cpus) {
      const auto text = holds(cpu + 1)
          ? std::to_string(cpu) + "-" + std::to_string(cpu + 1)
          : std::to_string(cpu);
      writeSiblingList(root, cpu, "core_cpus_list", text);
    }
    check(answerOf("core_wide") == FactAnswer::ONE,
          "a set that holds every sibling of every cpu it holds answers "
          "#core_wide 1, and no width beside it (FR-042)");
  }
  {
    const auto root = fakeRoot("wide-outside");
    const ProcRootGuard guard {root};
    for (const std::size_t cpu : cpus) {
      writeSiblingList(root,
                       cpu,
                       "core_cpus_list",
                       cpu == lowest ? std::to_string(cpu) + "," + outside
                                     : std::to_string(cpu));
    }
    check(answerOf("core_wide") == FactAnswer::ZERO,
          "one sibling standing outside the set answers #core_wide 0 and the "
          "conditional takes the other arm (FR-042)");
  }
  {
    const auto root = fakeRoot("wide-unreadable");
    const ProcRootGuard guard {root};
    check(answerOf("core_wide") == FactAnswer::NOTHING,
          "an unreadable sibling list answers no #core_wide, and the "
          "constant keeps its refusal (FR-042)");
  }
  {
    const auto root = fakeRoot("wide-no-id");
    const ProcRootGuard guard {root};
    writeSiblingList(root, lowest, "core_cpus_list", "none");
    check(answerOf("core_wide") == FactAnswer::NOTHING,
          "a list that names no id answers no #core_wide, which keeps the "
          "standing refusal (FR-042)");
  }
  {
    // An id at or above the width of the set type stands outside every set,
    // so the digit walk settles there and the flag answers 0 (FR-042).
    const auto root = fakeRoot("wide-past-set");
    const ProcRootGuard guard {root};
    for (const std::size_t cpu : cpus) {
      writeSiblingList(root,
                       cpu,
                       "core_cpus_list",
                       cpu == lowest ? std::to_string(cpu) + ",1048576"
                                     : std::to_string(cpu));
    }
    check(answerOf("core_wide") == FactAnswer::ZERO,
          "a sibling id past the width the set carries stands outside it, "
          "and the flag answers 0 (FR-042)");
  }
  {
    // The shape a refused affinity read leaves behind: an empty set answers
    // nothing, so the reader invents no coverage for a process it cannot see.
    cpu_set_t empty {};
    CPU_ZERO(&empty);
    double value = 0.0;
    check(!sg::counters::detail::coreWideFromCpuSet(empty, value),
          "a cpu set holding no cpu answers no #core_wide (FR-042)");
  }
  for (const auto* constant : {"num_dies", "SYSTEM_TSC_FREQ", "has_pmem"}) {
    check(answerOf(constant) == FactAnswer::NOTHING,
          "no top-down row reaches this constant, so it keeps its refusal "
          "(FR-043)");
  }
}

// T004: the object tree keeps priority. Over the two-thread, one-core tree
// the two counts answer, so they decide the fact while a list naming one id
// stands under the root (FR-054).
[[nodiscard]] auto smtTreeKeepsPriority() -> int
{
  if (!sg::test::registerMetricHost(sg::test::MetricTree::SMT_ON)) {
    fail("the fixture provider could not register before the system opened");
  }
  const auto root = fakeRoot("tree-priority");
  const ProcRootGuard guard {root};
  for (const std::size_t cpu : processCpus()) {
    writeSiblingList(root, cpu, "core_cpus_list", std::to_string(cpu));
  }
  check(answerOf("SMT_on") == FactAnswer::ONE,
        "a tree that answers a core object keeps priority over a sibling "
        "list naming one id (FR-054)");
  return 0;
}

}  // namespace

auto main() -> int
{
  // One process carries the one tree it registers, because registration
  // closes when the catalog opens (FR-009). The priority scenario reads a
  // second tree, so it runs in a child forked before either process
  // registers, and the parent keeps the coreless tree the other scenarios
  // read.
  const pid_t child = ::fork();
  check(child >= 0, "the priority scenario forks");
  if (child == 0) {
    return smtTreeKeepsPriority();
  }
  smtFactRefusalScenario();
  smtStateFallsBackToTheSiblingList();
  coreWideAnswersFromTheProcessCpuSet();
  int status = 0;
  check(::waitpid(child, &status, 0) == child,
        "the priority child is waited for");
  check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "the priority child saw the tree keep priority (FR-054)");
  std::printf("PASS\n");
  return 0;
}
