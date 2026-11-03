// ============================================================================
// The collection one `--topdown` run makes (specs/042-topdown T027; US1;
// FR-016, FR-018, FR-019, FR-021, FR-022, FR-023; PC-9, PC-10).
//
// Every scenario drives `detail::Runner` over one scripted provider the way
// `harness_pass_collect_test.cpp` drives it, and reads a field of the result
// value beside the provider's own count of sampling actions. The leaves answer
// from a frozen script, so no hardware is read, no scenario sleeps, and no
// clock stands in this file, which `test/time_source_gate.sh` bars.
//
// The rows the selection reads stand in the table a scenario plants at
// `RunOptions::topdownTable`, the seam
// `specs/042-topdown/contracts/topdown-selection.md` gives a test. A planted
// table holds the fixture rows of `detail/topdown_fixture.hpp` as the metric
// layer's own verdicts: the rows pass through `parseMetricRows` and
// `metricRows` over the scripted host of `detail/metric_fixture.hpp`, so a
// planted row carries the spine, the leaf addresses, and the refusal that host
// answers with (FR-010, FR-016, FR-048). The leaves those spines read live on
// the scripted objects this file lays beside the pass fixture's own, so a run
// of this unit reads no real counter and no vendored table.
//
// The identity the runner reads belongs to the cpu it runs on, and this build
// vendors no table for a vendor outside Intel and AMD. A host of another vendor
// answers no tree for that reason alone, so every scenario of this unit states
// that reason and exits 2, the skip code the two counters units of this feature
// reserve.
//
// Each scenario scripts its own provider, and the counters system takes one
// provider per object path per process, so each scenario runs in its own
// process: the parent below walks the scenario list and re-executes this binary
// once per entry, the pattern of `harness_pass_collect_test.cpp`. The list is
// what CTest runs, so a scenario absent from it never runs.
// ============================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/pass_fixture.hpp"
#include "detail/topdown_fixture.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_topdown.hpp"

namespace
{

using sg::counters::CpuIdentity;
using sg::counters::detail::metricRows;
using sg::counters::detail::parseMetricRows;
using sg::test::fixtureHost;
using sg::test::kCoreObject;
using sg::test::kIntelDirectory;
using sg::test::kMemoryObject;
using sg::test::MetricDefinition;
using sg::test::MetricRow;

// The iteration count every run pins: the planted rows divide a count by a
// count, so the iteration count cancels out of every figure a scenario reads.
constexpr std::uint64_t kRunIterations = 4;

// The repetition count of the runs whose rows a scenario compares. One
// repetition leaves no aggregate row, so those rows are the measured rows.
constexpr std::uint64_t kRunRepetitions = 1;

// The repetition count pair whose runs differ by exactly one repetition of
// every pass, the pair `harness_pass_collect_test.cpp` takes for the same
// reason: the difference of their action counts is the window cost of that one
// repetition alone, and nothing else a run sets up rides in the difference.
constexpr std::uint64_t kLowRepetitions = 2;
constexpr std::uint64_t kHighRepetitions = 3;

// The sampling actions one window of one pass costs, the count the audit point
// states for a window that folds nothing.
constexpr std::uint64_t kWindowReads = sg::test::kAuditPointReadsPerIteration;

// The limit of the pass fixture's own source. Eight leaves hold the two
// scripted pairs of a run at once, so that source never splits a run of this
// unit and every split a scenario reads comes from the source the node columns
// bring into the plan.
constexpr std::size_t kWideSourceLimit = sg::test::kWidePassLimit;

// The limit of the core object, the source the planted rows read and no metric
// column of this unit touches. Two leaves is the narrowest limit that splits a
// run of the four level-1 columns.
constexpr std::size_t kNarrowCoreLimit = 2;

// The pass count of the two scripted pairs on their own. Both hold two leaves
// of a source of eight, so one pass measures the pair (specs/024 E-03 rule 3).
constexpr std::size_t kMetricOnlyPassCount = 1;

// The pass count of the four level-1 columns of the Intel fixture table at
// `kNarrowCoreLimit`, with one price for the core source: the first column
// opens pass 0 and fills it, the second opens pass 1 and fills it, the third
// reads four leaves over the limit and stands alone in pass 2, and the fourth
// finds every open pass full and opens pass 3 (specs/024 E-03 rules 3 and 4).
constexpr std::size_t kLevelOnePassCount = 4;

// The pass count of the two scripted pairs and the four level-1 columns in one
// plan. The two pairs leave their own source with room, the first node column
// rides their pass, and the core source of the rest splits as it does alone, so
// the metric columns cost no pass of the plan the tree alone settles.
constexpr std::size_t kOnePlanPassCount = 4;

// The pass count of the ten published level-2 columns of the Intel fixture
// table at `kNarrowCoreLimit`. A pass holds the distinct leaves of every column
// it measures, so the columns that read cycles with reference cycles share one
// pass, the columns that read the l2 pair share another, the two columns of
// four leaves each stand alone over the limit, and the memory column opens a
// pass of its own because its time leaf joins no pass that still has room
// (specs/024 E-03 rules 2 to 5).
constexpr std::size_t kLevelTwoPassCount = 7;

// The two levels the scenarios ask for.
constexpr std::uint64_t kLevelOne = 1;
constexpr std::uint64_t kLevelTwo = 2;

// The repetition count of the record and strip cases. Two repetitions stand one
// aggregate row beside the two measured rows, so those cases read a run's
// statistic view and its value view (FR-027).
constexpr std::uint64_t kRecordRepetitions = 2;

// The node columns the level-1 ask brings into a run, and the two time columns
// that stand ahead of every metric column of a percentile grid and of a
// retained chunk column (specs/021 FR-010, specs/023 E-02).
constexpr std::size_t kLevelOneNodeColumns = 4;
constexpr std::size_t kTimeColumns = 2;

// The four level-1 figures the scripted leaves owe the planted rows. Each row
// carries the `100%` scale, and the scale of a row applies inside that row's
// own spine, so an inlined sub-metric arrives with its own scale already
// applied (specs/022 FR-018). The backend row therefore reads one minus the
// three percents and carries its own scale over that difference.
constexpr double kFrontendPercent =
    100.0 * sg::test::kCyclesDelta / sg::test::kRefCyclesDelta;
constexpr double kBadSpeculationPercent =
    100.0 * sg::test::kL2Delta / sg::test::kInstructionsDelta;
constexpr double kRetiringPercent =
    100.0 * sg::test::kInstructionsDelta / sg::test::kRefCyclesDelta;
constexpr double kBackendPercent = 100.0
    * (1.0 - kFrontendPercent - kBadSpeculationPercent - kRetiringPercent);

// The metric columns every scenario attaches: the two scripted pairs of the
// pass fixture, which are all the metric view a stripped top-down run keeps.
constexpr std::size_t kMetricColumnCount = 2;

// The four vendor names of those rows, in the table order the fixture spells.
constexpr std::array<std::string_view, 4> kLevelOneVendorNames {{
    "tma_frontend_bound",
    "tma_bad_speculation",
    "tma_backend_bound",
    "tma_retiring",
}};

// The four neutral names beside those rows, in the same order.
constexpr std::array<std::string_view, 4> kLevelOneNeutralNames {{
    sg::test::kNeutralFrontendBound,
    sg::test::kNeutralBadSpeculation,
    sg::test::kNeutralBackendBound,
    sg::test::kNeutralRetiring,
}};

// The four figures beside those names, in the same order.
constexpr std::array<double, 4> kLevelOnePercents {{
    kFrontendPercent,
    kBadSpeculationPercent,
    kBackendPercent,
    kRetiringPercent,
}};

// The deeper levels of the Intel fixture, in the order its rows stand. The
// neutral names are the ones `topdownTree` gives a row below level 1, the
// vendor names are the row's own, and each parent is the neutral name of the
// row one level above (FR-013, FR-014, FR-018).
constexpr std::array<std::string_view, 6> kLevelTwoNeutralNames {{
    "topdown.intel.tma_fetch_latency",
    "topdown.intel.tma_fetch_bandwidth",
    "topdown.intel.tma_branch_mispredicts",
    "topdown.intel.tma_core_bound",
    "topdown.intel.tma_memory_bound",
    "topdown.intel.tma_memory_bandwidth",
}};
constexpr std::array<std::string_view, 6> kLevelTwoVendorNames {{
    "tma_fetch_latency",
    "tma_fetch_bandwidth",
    "tma_branch_mispredicts",
    "tma_core_bound",
    "tma_memory_bound",
    "tma_memory_bandwidth",
}};
constexpr std::array<std::string_view, 6> kLevelTwoParentNames {{
    "topdown.frontend_bound",
    "topdown.frontend_bound",
    "topdown.bad_speculation",
    "topdown.backend_bound",
    "topdown.backend_bound",
    "topdown.backend_bound",
}};

// The level-3 rows of the same fixture, in table order, and the level-2 parent
// each one names.
constexpr std::array<std::string_view, 4> kLevelThreeNeutralNames {{
    "topdown.intel.tma_branch_resteers",
    "topdown.intel.tma_divider",
    "topdown.intel.tma_dram_bound",
    "topdown.intel.tma_dsb",
}};
constexpr std::array<std::string_view, 4> kLevelThreeVendorNames {{
    "tma_branch_resteers",
    "tma_divider",
    "tma_dram_bound",
    "tma_dsb",
}};
constexpr std::array<std::string_view, 4> kLevelThreeParentNames {{
    "topdown.intel.tma_fetch_latency",
    "topdown.intel.tma_core_bound",
    "topdown.intel.tma_memory_bound",
    "topdown.intel.tma_fetch_bandwidth",
}};

// The pass counts the one plan of each asked level settles: the two scripted
// pairs, the four level-1 columns, and the columns of each deeper level, all
// priced together against the two programmable counters of the scripted source
// (FR-021, FR-023).
constexpr std::size_t kLevelTwoPlanPassCount = 7;
constexpr std::size_t kLevelThreePlanPassCount = 8;

// SC-03's two tolerances, in percentage points: one figure against the fixture
// figure it stands for, and the five AMD level-1 values against the hundred
// percent they close.
constexpr double kFigureTolerance = 0.05;
constexpr double kSumTolerance = 0.1;

// The threshold text one planted row carries, spelled with the spaces the table
// file spells it, so a reader can tell an unchanged print from a trimmed one
// (FR-017, FR-029).
constexpr std::string_view kPlantedThreshold {"&gt; 65.0"};

// The AMD identity a scripted run hands the selection, and the ref-cycles delta
// this unit scripts for the AMD run alone. The five rows of the Zen 4 table
// each take something over six slots of the cycle count, and with the 500 the
// metric fixture scripts for that leaf the five come to 91.67 percent. The AMD
// case therefore lays that leaf down at 1000, so the five level-1 figures close
// the hundred percent FR-056 states and the case measures the closing instead
// of assuming it. Both numbers are scripted values of this unit's source and no
// fact about a host (FR-011, FR-056, spec D-3).
constexpr std::string_view kAmdVendorText {"AuthenticAMD"};
constexpr double kAmdRefCyclesDelta = 1000.0;

// The five figures the run answers for the Zen 4 table over the scripted leaves
// above, in table order. They are written out and not computed here: the fold
// that turns a row's formula and the scripted leaves into a percentage is the
// library's, and a case that redid that arithmetic could not fail. Each is the
// figure of a row of the fixture over scripted leaves, which are scripted
// values and no fact about a host.
constexpr std::array<double, 5> kAmdLevelOneFigures {{
    16.666667,
    33.166667,
    16.666667,
    0.166667,
    33.333333,
}};

// The deepest level of the shallow fixture: the level an ask deeper than that
// clamps to, and the level the run reports (FR-020).
constexpr std::uint64_t kDeepestPlantedLevel = 4;

// One level-4 node of that fixture, by the neutral name the selection gives it.
constexpr std::string_view kLevelFourNeutralName {
    "topdown.intel.tma_clears_resteers"};

// The tag of the line a clamped ask states, naming the deepest level the table
// holds.
constexpr std::string_view kClampLineTag {
    "topdown: the table holds no level deeper than "};

// The lines the AMD statement costs, beside the header and the node lines.
constexpr std::size_t kAmdStatementLines = 1;

// The two vendors this build vendors a table for, spelled the way CPUID leaf 0
// spells them. The identity a run reads belongs to the host, so a scenario of
// this unit stands only where that identity names one of these two texts.
constexpr std::string_view kIntelVendor {"GenuineIntel"};
constexpr std::string_view kAmdVendor {"AuthenticAMD"};

// The skip code the two counters units of this feature reserve: a host the
// selection vendors no table for states its reason and leaves with this code,
// which the `SKIP_RETURN_CODE` property of the registration reads.
constexpr int kSkipStatus = 2;

sg::counters::FakeProvider* fake = nullptr;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS TOPDOWN RUN TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// True when a figure stands within a tenth of a percent of the figure the
// scripted deltas state: the fold of a window divides a count by a count, so
// the figures a scenario reads are exact and this tolerance only absorbs the
// printed round of a double.
auto closeTo(const double figure, const double expected) -> bool
{
  return std::abs(figure - expected) <= 0.001 * (std::abs(expected) + 1.0);
}

// True when two figures carry the same bytes: the equality one column of a
// top-down run owes the same column of the run without the option. A tolerance
// would let a figure that moved by a unit of the last place pass as unchanged,
// and no part of this comparison divides, sums or rounds.
auto sameBytes(const double first, const double second) -> bool
{
  return std::memcmp(&first, &second, sizeof(double)) == 0;
}

// True when two statistic sets carry the same bytes in every field: the
// equality one aggregate view of a run owes the same view of the run without
// the option.
auto sameStats(const sg::Statistics& first,
               const sg::Statistics& second) -> bool
{
  return first.samples == second.samples && sameBytes(first.mean, second.mean)
      && sameBytes(first.median, second.median)
      && sameBytes(first.sampleStdDev, second.sampleStdDev)
      && sameBytes(first.coefficientOfVariation, second.coefficientOfVariation)
      && sameBytes(first.min, second.min) && sameBytes(first.max, second.max);
}

// A host this build vendors no table for: the scenario states the reason it
// cannot stand here and leaves with the skip code of the registration.
[[noreturn]] auto skip(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS TOPDOWN RUN TEST SKIP: %s\n", what);
  std::exit(kSkipStatus);
}

// True where the identity this process reads selects a table at all. The
// selection reads the identity ahead of the table a scenario plants, so a host
// outside the two vendors answers no tree whatever the planted rows hold
// (FR-004).
[[nodiscard]] auto hostSelectsATable() -> bool
{
  const CpuIdentity identity = sg::counters::cpuIdentity();
  return identity.vendor == kIntelVendor || identity.vendor == kAmdVendor;
}

// True where the identity of this host maps to the directory the fixture's
// Intel table stands in. The selection reads the family, the model and the
// stepping of the host ahead of any row a scenario plants, and a host another
// directory serves answers its own level-2 rows, whose names and order differ
// between the directories of one vendor, so a scenario that names the
// fixture's rows stands where those rows stand (FR-002, FR-011, FR-027).
[[nodiscard]] auto hostSelectsTheFixtureDirectory() -> bool
{
  const sg::counters::TopdownTable table =
      sg::counters::topdownTable(sg::counters::cpuIdentity());
  return table.source.directory == kIntelDirectory;
}

// One scripted leaf at one reading: the fake provider keeps the delta of a
// counter as a count of readings, so the fixture's reading stands cast to that
// count where it enters the script.
auto setLeaf(sg::counters::FakeProvider& provider,
             std::string_view objectPath,
             std::string_view name,
             const double reading) -> void
{
  provider.setPoints(
      objectPath, name, {0}, static_cast<std::uint64_t>(reading));
}

// The scripted leaves of the planted rows, on the two objects
// `metric_fixture.hpp` names and at the deltas that file states. The four PMU
// leaves carry the level rows of both levels. `fixture_time` and
// `fixture_data_reads` carry the memory row of level 2, whose own scope seats
// it on the memory object beside the core rows of its level (FR-010, FR-016).
// The memory object publishes no limit on purpose: that is the shape of a
// source that cannot answer, whose leaves cost nothing and ride every pass
// whose columns need them (specs/024 FR-005).
auto scriptTopdownLeaves(sg::counters::FakeProvider& provider) -> void
{
  provider.addObject(
      kCoreObject, "pmu", "core object of the planted topdown table");
  provider.addObject(
      kMemoryObject, "pmu", "memory object of the planted topdown table");
  provider.addCounter(kCoreObject, sg::test::kCycles, "none", "fixture cycles");
  provider.addCounter(
      kCoreObject, sg::test::kInstructions, "none", "fixture instructions");
  provider.addCounter(
      kCoreObject, sg::test::kRefCycles, "none", "fixture reference cycles");
  provider.addCounter(
      kCoreObject, sg::test::kL2Requests, "none", "fixture l2 requests");
  provider.addCounter(
      kCoreObject, sg::test::kTime, "nanoseconds", "fixture time");
  provider.addCounter(
      kMemoryObject, sg::test::kDataReads, "bytes", "fixture memory reads");
  setLeaf(provider, kCoreObject, sg::test::kCycles, sg::test::kCyclesDelta);
  setLeaf(provider,
          kCoreObject,
          sg::test::kInstructions,
          sg::test::kInstructionsDelta);
  setLeaf(
      provider, kCoreObject, sg::test::kRefCycles, sg::test::kRefCyclesDelta);
  setLeaf(provider, kCoreObject, sg::test::kL2Requests, sg::test::kL2Delta);
  setLeaf(provider, kCoreObject, sg::test::kTime, sg::test::kTimeDelta);
  setLeaf(
      provider, kMemoryObject, sg::test::kDataReads, sg::test::kDataReadsDelta);
}

// One scripted provider for one scenario: the pass fixture's sources and clock
// leaves, the leaves of the planted rows, and the one limit the core object
// publishes. Every scripted fact precedes the registration, because the system
// stages a provider's whole catalog as it registers and skips a fact laid down
// after the open.
auto registerFixture(const std::size_t coreLimit) -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::scriptPassFixture(*provider, kWideSourceLimit);
  scriptTopdownLeaves(*provider);
  provider->setProgrammableCounters(kCoreObject, coreLimit);
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(),
        "the topdown fixture registers on the local system");
  fake = scripted;
  return *scripted;
}

// The scripted source of the AMD run, and no other: the leaves of the planted
// rows and the one limit, with this unit's ref-cycles leaf laid down at
// `kAmdRefCyclesDelta` instead of the value the metric fixture scripts, so the
// five level-1 figures of the Zen 4 table close the hundred percent FR-056
// states. The leaves are scripted readings, no fact about a host, and the
// fixture header keeps the value every other case of this unit reads. Every
// scripted fact still precedes the registration.
auto registerAmdFixture(const std::size_t coreLimit)
    -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::scriptPassFixture(*provider, kWideSourceLimit);
  scriptTopdownLeaves(*provider);
  setLeaf(*provider, kCoreObject, sg::test::kRefCycles, kAmdRefCyclesDelta);
  provider->setProgrammableCounters(kCoreObject, coreLimit);
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(), "the AMD fixture registers on the local system");
  fake = scripted;
  return *scripted;
}

// Two leaves of the planted table frozen at one reading. A frozen leaf carries
// the same value at every sampling action, so its delta measures nothing and a
// formula that divides by it has a denominator that measured no delta
// (specs/042-topdown PC-17). The leaf stays countable: the fold receives the
// delta of zero and divides, and answers a figure no report can print. Every
// scripted fact precedes the registration, as the two fixtures above lay them.
auto registerFrozenLeafFixture() -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::scriptPassFixture(*provider, kWideSourceLimit);
  scriptTopdownLeaves(*provider);
  provider->setPoints(kCoreObject, sg::test::kCycles, {0}, 0.0);
  provider->setPoints(kCoreObject, sg::test::kRefCycles, {0}, 0.0);
  provider->setProgrammableCounters(kCoreObject, kNarrowCoreLimit);
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(),
        "the frozen fixture registers on the local system");
  fake = scripted;
  return *scripted;
}

// The two words a double reaches for when the arithmetic answered no number.
// They are the defect this case names: a line of the block and a field of the
// record are both places a figure reaches a reader, and neither may spell them
// (specs/042-topdown PC-17).
constexpr std::array<std::string_view, 2> kNonFiniteSpellings {{"nan", "inf"}};

// The hundred percent the four level-1 shares of the planted table divide, as a
// figure of the scale the block prints them in.
constexpr double kShareCeiling = 100.0;

// True where no text of the report spells a figure that is no number.
[[nodiscard]] auto spellsNoNumber(const std::string& text) -> bool
{
  for (const std::string_view spelling : kNonFiniteSpellings) {
    if (text.find(spelling) != std::string::npos) {
      return false;
    }
  }
  return true;
}

// The identity of an AMD cpu, handed to the selection in place of the one this
// host states. `topdownTree` reads the vendor text alone: the family, the model
// and the stepping choose the table of a host that has to read one, and a run
// that is handed its table reads no mapping.
auto amdIdentity() -> sg::counters::CpuIdentity
{
  sg::counters::CpuIdentity identity;
  identity.vendor = std::string(kAmdVendorText);
  return identity;
}

// The sum of one column's leaves, the shape `data-model.md` E-01 gives a metric
// column of the pass fixture: one same-dimension core over the PMU events it
// names.
auto sumLeaves(const std::string_view objectPath,
               const std::vector<std::string_view>& names)
    -> sg::counters::Expression<sg::counters::Dim<0, 1>>
{
  const auto object = sg::counters::System::local().object(objectPath);
  check(object.has_value(), "the object of the metric column resolves");
  const auto first = object->counter<sg::counters::Dim<0, 1>>(names.front());
  check(first.has_value(), "the first leaf of the column resolves");
  auto total = sg::counters::Expression<sg::counters::Dim<0, 1>>(*first);
  for (std::size_t index = 1; index < names.size(); ++index) {
    const auto leaf = object->counter<sg::counters::Dim<0, 1>>(names[index]);
    check(leaf.has_value(), "every leaf of the column resolves");
    total = total + *leaf;
  }
  return total;
}

// The body of every run: nothing but the loop, so a window holds its endpoint
// samples and nothing else.
auto plainBody(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// Register one instance of the plain body with the two scripted pairs attached:
// the metric columns a run would report before the option stood.
auto metricBenchmark(const char* name) -> void
{
  auto handle = sg::registerBenchmark(&plainBody, name);
  for (const std::size_t index : {std::size_t {0}, std::size_t {1}}) {
    const sg::test::PassColumn column = sg::test::passColumns()[index];
    handle.addMetric(sumLeaves(column.object, column.events), column.label);
  }
}

// The fixture rows with one threshold text planted on one metric row, leaving
// every other row as the fixture spells it.
[[nodiscard]] auto withThreshold(const std::vector<MetricRow>& rows,
                                 const std::string_view metricName,
                                 const std::string_view threshold)
    -> std::vector<MetricRow>
{
  std::vector<MetricRow> spelled = rows;
  for (MetricRow& row : spelled) {
    if (row.metricName == metricName) {
      row.threshold = std::string(threshold);
    }
  }
  return spelled;
}

// True when one node of a record carries `name`.
auto holdsName(const std::vector<sg::TopdownNode>& nodes,
               const std::string_view name) -> bool
{
  for (const sg::TopdownNode& node : nodes) {
    if (node.name == name) {
      return true;
    }
  }
  return false;
}

// Register one instance of the plain body with no column of its own: the tree
// is the only thing such a run collects.
auto bareBenchmark(const char* name) -> void
{
  static_cast<void>(sg::registerBenchmark(&plainBody, name));
}

// Register one instance that declares one metric column whose label is the
// vendor name of a level-1 node and whose formula is that node's formula scaled
// by hand to the percent unit the row's scale text spells: the column a user
// names with `--metric=` in a production run. A scripted run cannot reach a
// planted row through the option's name, because the option resolves a name in
// the table of the host and the rows of a planted table stand in no host. What
// FR-023 and SC-03 fix is a column of a node's formula and label, whichever way
// it enters the run, and a declared column is the way a scripted run has.
auto nodeColumnBenchmark(const char* name) -> void
{
  auto handle = sg::registerBenchmark(&plainBody, name);
  handle.addMetric(100.0
                       * (sumLeaves(kCoreObject, {sg::test::kCycles})
                          / sumLeaves(kCoreObject, {sg::test::kRefCycles})),
                   std::string(kLevelOneVendorNames[0]));
}

// The run shape every scenario shares. The warm-up stands off so every sampling
// action of a run belongs to a window of a pass, and the iteration count stands
// fixed so no calibration runs ahead of the pass loop.
[[nodiscard]] auto optionsFor(const std::uint64_t repetitions)
    -> sg::detail::RunOptions
{
  sg::detail::RunOptions options;
  options.fixedIterations = kRunIterations;
  options.repetitions = repetitions;
  options.warmupTimeNs = 0;
  return options;
}

// One fixture table as the selection reads it: the event rows apart from the
// metric rows, and the metric rows parsed and resolved against the scripted
// host, so a planted row carries the verdict, the spine, the leaf addresses and
// the reason that host answers with (FR-010, FR-016).
[[nodiscard]] auto plantedTable(const std::string_view directory,
                                const std::vector<MetricRow>& rows)
    -> sg::counters::TopdownTable
{
  std::vector<MetricRow> events;
  for (const MetricRow& row : rows) {
    if (row.metricName.empty()) {
      events.push_back(row);
    }
  }
  const auto parsed = parseMetricRows(rows);
  const auto resolved = metricRows(parsed, events, fixtureHost());
  sg::counters::TopdownTable table;
  table.source.directory = std::string {directory};
  for (const MetricDefinition& def : resolved) {
    sg::counters::TopdownRow row;
    row.name = def.name;
    row.group = def.group;
    row.unit = def.unit;
    row.threshold = def.threshold;
    row.formula.core = def.core;
    row.published = def.published;
    row.refusal = def.refusal;
    row.reason = def.reason;
    table.rows.push_back(std::move(row));
  }
  return table;
}

// The Intel fixture table without its ten refused rows: the same directory, the
// same published rows, and no row the scripted host refuses (FR-016).
[[nodiscard]] auto publishedOnlyRows() -> std::vector<MetricRow>
{
  std::vector<MetricRow> rows = sg::test::topdownIntelEventRows();
  const auto append = [&rows](const std::vector<MetricRow>& part)
  { rows.insert(rows.end(), part.begin(), part.end()); };
  append(sg::test::topdownIntelLevelOneRows());
  append(sg::test::topdownIntelLevelTwoRows());
  append(sg::test::topdownIntelDeepRows());
  return rows;
}

// The options of a run that asks for one level of one planted table.
[[nodiscard]] auto askedTable(const sg::counters::TopdownTable& table,
                              const std::uint64_t level,
                              const std::uint64_t repetitions)
    -> sg::detail::RunOptions
{
  sg::detail::RunOptions options = optionsFor(repetitions);
  options.topdownLevel = level;
  options.topdownTable = table;
  return options;
}

// The options of a run that asks for one level of one fixture table.
[[nodiscard]] auto askedOptions(const std::vector<MetricRow>& rows,
                                const std::uint64_t level,
                                const std::uint64_t repetitions)
    -> sg::detail::RunOptions
{
  return askedTable(plantedTable(kIntelDirectory, rows), level, repetitions);
}

[[nodiscard]] auto needInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  check(false, "the expansion produced the named instance");
  return nullptr;
}

// One run, its result, and the sampling actions it cost: the ledger
// `harness_pass_collect_test.cpp` keeps for the same reason (FR-021).
struct RunAccount
{
  sg::BenchmarkResult result;
  std::uint64_t reads = 0;
};

auto runMeasured(sg::Instance& instance,
                 const sg::detail::RunOptions& options) -> RunAccount
{
  const std::uint64_t before = fake->readActions();
  sg::detail::Runner runner(options);
  auto result = runner.run(instance);
  check(result.outcome == sg::RunOutcome::MEASURED,
        "the scripted run measured its row");
  check(!result.rows.empty(), "the measured result carries a row");
  return RunAccount {std::move(result), fake->readActions() - before};
}

// One run beside the retained chunk columns it left behind: the ledger of row
// `k` stands at `rowRaw[k]` and the ledger of context `j` of that row at
// `contextRaw[k][j]`, the pairing `harness_pass_collect_test.cpp` reads for the
// same view (specs/023 E-01, FR-029).
struct LedgerAccount
{
  RunAccount run;
  std::vector<sg::RowRaw> rowRaw;
  std::vector<std::vector<sg::RowRaw>> contextRaw;
};

auto runMeasuredLedger(sg::Instance& instance,
                       const sg::detail::RunOptions& options) -> LedgerAccount
{
  const std::uint64_t before = fake->readActions();
  sg::detail::Runner runner(options);
  auto result = runner.run(instance);
  check(result.outcome == sg::RunOutcome::MEASURED,
        "the scripted run measured its row");
  check(!result.rows.empty(), "the measured result carries a row");
  return LedgerAccount {
      RunAccount {std::move(result), fake->readActions() - before},
      runner.rowRaw().back(),
      runner.contextRaw().back()};
}

// The refusal one set of options hands the library, so a case can read the line
// the run owes its record and the command line owes its console: the same three
// fields the runner hands `topdownTree`, the identity of this process where the
// case hands none (FR-040, FR-041).
[[nodiscard]] auto refusalOf(const sg::detail::RunOptions& options)
    -> sg::counters::TopdownUnsupported
{
  sg::counters::TopdownSelection selection;
  selection.identity = options.topdownIdentity.has_value()
      ? *options.topdownIdentity
      : sg::counters::cpuIdentity();
  selection.level = static_cast<int>(*options.topdownLevel);
  selection.table = options.topdownTable;
  const auto answered = sg::counters::topdownTree(selection);
  check(!answered.has_value(),
        "the options a case hands the selection answer no tree (FR-004 to "
        "FR-007)");
  return answered.error();
}

// The tag every line of the top-down block carries, the way the jitter note
// carries `jitter:` in the same report (specs/042-topdown FR-026).
constexpr std::string_view kBlockTag {"topdown:"};

// The source record a planted table carries, so the block has a reference and a
// file digest to print: the loader fills both beside the directory, and a
// planted table states them for the same view (FR-015).
constexpr std::string_view kPlantedReference {"v2024.10-events"};
constexpr std::string_view kPlantedFileName {"pipeline.json"};
constexpr std::string_view kPlantedFileHash {
    "9f2b1c4d7e6a5038b1c2d3e4f5061728a9b0c1d2e3f405162738495a6b7c8d9e"};

// The two files one redirected call writes. Both names carry the
// `harness_json_` head: `harness_topdown_report_test.cpp` writes its scratch
// documents under that head, the ignore rules of the repository cover it, and
// running this binary from the repository root leaves the tree clean. ctest
// runs it from the build's test directory.
constexpr const char* kOutPath = "harness_json_topdown_run_out.txt";
constexpr const char* kErrPath = "harness_json_topdown_run_err.txt";

[[nodiscard]] auto textOf(const char* const path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The lines of one captured console text.
[[nodiscard]] auto linesOf(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::istringstream stream(text);
  for (std::string line; std::getline(stream, line);) {
    lines.push_back(line);
  }
  return lines;
}

// The two streams of one call, held apart from the test's own output. A `check`
// of the redirect itself prints to the redirected stream, because nothing else
// stands between the two, so both restores are checked before the text is read.
struct CapturedStreams
{
  std::string out;
  std::string err;
};

template<typename Body>
auto captureStreams(Body&& body) -> CapturedStreams
{
  const int savedOut = ::dup(STDOUT_FILENO);
  const int savedErr = ::dup(STDERR_FILENO);
  check(savedOut >= 0 && savedErr >= 0,
        "the test holds its own two streams aside");
  check(std::freopen(kOutPath, "w", stdout) != nullptr
            && std::freopen(kErrPath, "w", stderr) != nullptr,
        "the test redirects both streams of the call");
  body();
  std::fflush(stdout);
  std::fflush(stderr);
  check(::dup2(savedOut, STDOUT_FILENO) >= 0
            && ::dup2(savedErr, STDERR_FILENO) >= 0,
        "the test gives its own two streams back");
  ::close(savedOut);
  ::close(savedErr);
  return CapturedStreams {textOf(kOutPath), textOf(kErrPath)};
}

// One run beside the two streams it could have written: the account of
// `runMeasured` and the whole text each stream received while the run ran
// (specs/042-topdown FR-040).
struct CapturedRun
{
  RunAccount run;
  std::string out;
  std::string err;
};

auto runMeasuredQuietly(sg::Instance& instance,
                        const sg::detail::RunOptions& options) -> CapturedRun
{
  RunAccount account {sg::BenchmarkResult {}, 0};
  const std::uint64_t before = fake->readActions();
  CapturedStreams streams = captureStreams(
      [&]
      {
        sg::detail::Runner runner(options);
        account.result = runner.run(instance);
      });
  account.reads = fake->readActions() - before;
  check(account.result.outcome == sg::RunOutcome::MEASURED,
        "the scripted run measured its row");
  check(!account.result.rows.empty(), "the measured result carries a row");
  return CapturedRun {
      std::move(account), std::move(streams.out), std::move(streams.err)};
}

// The report of one result, as the console prints it. `detail::printResult` is
// declared in `detail/internal.hpp` for this lane, so each case reads the block
// in the report of the run that collected it.
[[nodiscard]] auto printedLines(const sg::BenchmarkResult& result)
    -> std::vector<std::string>
{
  const CapturedStreams streams =
      captureStreams([&] { sg::detail::printResult(result); });
  return linesOf(streams.out);
}

// The lines of a printed report that carry the block's tag, and the index of
// the first line a report carries of a given prefix, or nothing where none
// stands.
[[nodiscard]] auto blockLinesOf(const std::vector<std::string>& lines)
    -> std::vector<std::string>
{
  std::vector<std::string> found;
  for (const std::string& line : lines) {
    if (line.rfind(kBlockTag, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

[[nodiscard]] auto lineAt(const std::vector<std::string>& lines,
                          const std::string_view prefix)
    -> std::optional<std::size_t>
{
  const auto found = std::find_if(lines.begin(),
                                  lines.end(),
                                  [&](const std::string& line)
                                  { return line.rfind(prefix, 0) == 0; });
  if (found == lines.end()) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(found - lines.begin());
}

// How many fields one printed row line carries for `label`. A node column whose
// vendor name equals the label of a column the run already reported must add no
// field of its own (FR-023).
[[nodiscard]] auto fieldsOf(const std::string& line,
                            const std::string_view label) -> std::size_t
{
  const std::string mark = std::string(label) + "=";
  std::size_t seen = 0;
  for (std::size_t at = line.find(mark); at != std::string::npos;
       at = line.find(mark, at + mark.size()))
  {
    ++seen;
  }
  return seen;
}

// The figure text of the field one printed row line carries for `label`, or an
// empty text where the line carries no such field.
[[nodiscard]] auto figureFieldOf(const std::string& line,
                                 const std::string_view label) -> std::string
{
  const std::string mark = std::string(label) + "=";
  const std::size_t at = line.find(mark);
  if (at == std::string::npos) {
    return {};
  }
  std::size_t start = at + mark.size();
  while (start < line.size() && line[start] == ' ') {
    ++start;
  }
  const std::size_t stop = line.find_first_of(" \t", start);
  return line.substr(
      start, stop == std::string::npos ? std::string::npos : stop - start);
}

// The first line a report carries of `prefix`, or an empty text where none
// stands.
[[nodiscard]] auto lineOf(const std::vector<std::string>& lines,
                          const std::string_view prefix) -> std::string
{
  const std::optional<std::size_t> found = lineAt(lines, prefix);
  return found.has_value() ? lines[*found] : std::string {};
}

// The block line of one node of one level: the block tag, the indentation the
// level of the node buys, and the neutral name. An empty text is the run
// printing no line for that node.
[[nodiscard]] auto nodeLineOf(const std::vector<std::string>& lines,
                              const std::uint64_t level,
                              const std::string_view name) -> std::string
{
  const std::string prefix = std::string(kBlockTag) + "  "
      + std::string(2 * static_cast<std::size_t>(level - 1), ' ')
      + std::string(name);
  for (const std::string& line : lines) {
    if (line.starts_with(prefix)) {
      return line;
    }
  }
  return {};
}

// The deepest level one record's nodes stand at, or 0 when it holds none.
[[nodiscard]] auto deepestLevelOf(const sg::TopdownRecord& record)
    -> std::uint64_t
{
  std::uint64_t deepest = 0;
  for (const sg::TopdownNode& node : record.nodes) {
    deepest = std::max(deepest, node.level);
  }
  return deepest;
}

// The options of a run that reports its percentiles, the view the strip of the
// percentile grid is read against (FR-024).
[[nodiscard]] auto percentilesOn(sg::detail::RunOptions options)
    -> sg::detail::RunOptions
{
  options.showPercentiles = true;
  return options;
}

// The position of one label in a result's metric columns, or nothing where the
// run reports no column of that name.
[[nodiscard]] auto columnAt(const sg::BenchmarkResult& result,
                            const std::string_view label)
    -> std::optional<std::size_t>
{
  const auto found =
      std::find(result.metricLabels.begin(), result.metricLabels.end(), label);
  if (found == result.metricLabels.end()) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(found - result.metricLabels.begin());
}

// True when every label of `front` stands at the head of `labels`, in the same
// order: the columns a run reported before the option keep their own positions.
[[nodiscard]] auto labelsLeadWith(const std::vector<std::string>& labels,
                                  const sg::BenchmarkResult& front) -> bool
{
  if (labels.size() < front.metricLabels.size()) {
    return false;
  }
  for (std::size_t index = 0; index < front.metricLabels.size(); ++index) {
    if (labels[index] != front.metricLabels[index]) {
      return false;
    }
  }
  return true;
}

// True when no label of `labels` names one of `names`.
[[nodiscard]] auto holdsNoneOf(const std::vector<std::string>& labels,
                               const std::vector<std::string>& names) -> bool
{
  for (const std::string& name : names) {
    if (std::find(labels.begin(), labels.end(), name) != labels.end()) {
      return false;
    }
  }
  return true;
}

// The names one planted table holds refused and no published row of the same
// table carries. The collision row shares its name with the row that took that
// name first, and the first row is the one that publishes, so that name stands
// as a column of the run and names no collected refusal (FR-016).
[[nodiscard]] auto refusedOnlyNames(const sg::counters::TopdownTable& table)
    -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (const sg::counters::TopdownRow& row : table.rows) {
    if (row.published) {
      continue;
    }
    const bool taken =
        std::any_of(table.rows.begin(),
                    table.rows.end(),
                    [&row](const sg::counters::TopdownRow& other)
                    { return other.published && other.name == row.name; });
    if (!taken) {
      names.push_back(row.name);
    }
  }
  return names;
}

// The first scenario: the formulas of one top-down run and the metric columns
// of the same run reach the pass planner as one set, and the run states the
// passes that one plan produced (FR-021, FR-023).
auto scenarioNodesShareOnePlanWithTheMetricColumns() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownMetricOnly");
  bareBenchmark("bmTopdownTreeOnly");
  metricBenchmark("bmTopdownOnePlan");
  sg::detail::expandRegistry();

  const std::vector<MetricRow> rows = sg::test::topdownIntelRows();
  const RunAccount metricRun = runMeasured(*needInstance("bmTopdownMetricOnly"),
                                           optionsFor(kRunRepetitions));
  const RunAccount treeRun =
      runMeasured(*needInstance("bmTopdownTreeOnly"),
                  askedOptions(rows, kLevelOne, kRunRepetitions));
  sg::Instance& combined = *needInstance("bmTopdownOnePlan");
  const RunAccount bothRun =
      runMeasured(combined, askedOptions(rows, kLevelOne, kRunRepetitions));
  const RunAccount bothLow =
      runMeasured(combined, askedOptions(rows, kLevelOne, kLowRepetitions));
  const RunAccount bothHigh =
      runMeasured(combined, askedOptions(rows, kLevelOne, kHighRepetitions));
  const std::uint64_t marginal = bothHigh.reads - bothLow.reads;

  check(metricRun.result.passCount == kMetricOnlyPassCount,
        "the two scripted pairs alone fill one pass of their own source "
        "(FR-021)");
  check(treeRun.result.passCount == kLevelOnePassCount,
        "the four level-1 columns alone split at the limit their source states "
        "(FR-021)");
  check(bothRun.result.passCount == kOnePlanPassCount,
        "the metric columns and the node columns share one plan of passes "
        "(FR-021, FR-023)");
  check(bothRun.result.passCount
            < metricRun.result.passCount + treeRun.result.passCount,
        "one plan costs one pass set, which two plans over the same columns "
        "cannot answer (FR-021)");
  check(marginal == kOnePlanPassCount * kWindowReads,
        "one more repetition costs one window per pass of the one plan, so the "
        "metric columns open no window of their own (FR-021)");
  check(bothRun.result.metricMethod == sg::MetricMethod::PASSES,
        "the run states the passes its one plan produced (FR-021)");
  check(labelsLeadWith(bothRun.result.metricLabels, metricRun.result),
        "the run keeps every metric column it would have reported before the "
        "option, in the order it attached them (FR-023)");
  check(columnAt(bothRun.result, "pair_a").has_value()
            && columnAt(bothRun.result, "pair_b").has_value(),
        "both scripted pairs stand among the columns of the top-down run "
        "(FR-023)");

  std::puts("nodes-share-one-plan-with-the-metric-columns: ok");
  return 0;
}

// The second scenario: the metric columns of a top-down run keep the bytes of
// the same columns in the same run without the option (FR-023).
auto scenarioTheMetricColumnsKeepTheirBytes() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownPlainBytes");
  metricBenchmark("bmTopdownAskedBytes");
  sg::detail::expandRegistry();

  const RunAccount plain = runMeasured(*needInstance("bmTopdownPlainBytes"),
                                       optionsFor(kRunRepetitions));
  const RunAccount asked = runMeasured(
      *needInstance("bmTopdownAskedBytes"),
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));

  check(asked.result.passCount == kLevelOnePassCount,
        "the run this case compares against collected the four level-1 columns "
        "and split at the limit their source states (FR-023)");
  check(asked.result.rows.size() == plain.result.rows.size(),
        "the option leaves the run's row count where it stood (FR-023)");
  check(labelsLeadWith(asked.result.metricLabels, plain.result),
        "every metric column of the plain run stands in the same position of "
        "the top-down run (FR-023)");
  for (std::size_t row = 0; row < plain.result.rows.size(); ++row) {
    const sg::ResultRow& before = plain.result.rows[row];
    const sg::ResultRow& after = asked.result.rows[row];
    check(
        after.iterations == before.iterations,
        "the row reports the iteration count the plain run reported (FR-024)");
    check(sameBytes(after.timePerIterationNs, before.timePerIterationNs)
              && sameBytes(after.cpuTimePerIterationNs,
                           before.cpuTimePerIterationNs),
          "the option changes no time column of the row (FR-024)");
    for (std::size_t column = 0; column < before.metrics.size(); ++column) {
      check(column < after.metrics.size(),
            "the top-down run reports a value for every metric column of the "
            "plain run (FR-023)");
      check(sameBytes(after.metrics[column].value,
                      before.metrics[column].value),
            "a metric column folds the value the same column folds without the "
            "option (FR-023)");
      check(after.metrics[column].availability
                == before.metrics[column].availability,
            "a metric column keeps the availability the same column keeps "
            "without the option (FR-023)");
      check(sameBytes(after.metrics[column].runningRatio,
                      before.metrics[column].runningRatio),
            "a metric column keeps the running ratio the same column discloses "
            "without the option (FR-023)");
    }
  }

  std::puts("the-metric-columns-keep-their-bytes: ok");
  return 0;
}

// The third scenario: a row the host refuses collects no column and moves no
// position of the column vector. Two planted tables of one directory differ by
// their refused rows alone, so one column set is the only answer (FR-016).
auto scenarioARefusedRowCollectsNoColumn() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  bareBenchmark("bmTopdownWithRefusals");
  bareBenchmark("bmTopdownWithoutRefusals");
  sg::detail::expandRegistry();

  const sg::counters::TopdownTable withRefusals =
      plantedTable(kIntelDirectory, sg::test::topdownIntelRows());
  const sg::counters::TopdownTable publishedOnly =
      plantedTable(kIntelDirectory, publishedOnlyRows());
  const RunAccount refusedRun =
      runMeasured(*needInstance("bmTopdownWithRefusals"),
                  askedTable(withRefusals, kLevelTwo, kRunRepetitions));
  const RunAccount publishedRun =
      runMeasured(*needInstance("bmTopdownWithoutRefusals"),
                  askedTable(publishedOnly, kLevelTwo, kRunRepetitions));

  check(refusedRun.result.passCount == kLevelTwoPassCount,
        "the published level-2 columns split at the limit their source states "
        "(FR-016)");
  check(publishedRun.result.passCount == refusedRun.result.passCount,
        "the refused rows move no pass of the plan their published rows share "
        "(FR-016)");
  check(refusedRun.result.metricLabels == publishedRun.result.metricLabels,
        "the refused rows collect no column and move no position of the column "
        "vector (FR-016)");
  check(holdsNoneOf(refusedRun.result.metricLabels,
                    refusedOnlyNames(withRefusals)),
        "no row the scripted host refuses stands as a column of the run "
        "(FR-016)");
  check(refusedRun.result.rows.front().metrics.size()
            == refusedRun.result.metricLabels.size(),
        "the row holds one value per metric label of the run (FR-016)");

  std::puts("a-refused-row-collects-no-column: ok");
  return 0;
}

// The fourth scenario: the source the node columns bring into the run prices
// the plan through the published object fact, and that one price is what the
// plan splits at (FR-022). The run with no column of its own settles to `auto`
// and states no pass, so the settling and the pass count of the top-down run
// read the one limit the fact published.
auto scenarioANewSourceReadsItsLimitOnce() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  bareBenchmark("bmTopdownUnaskedSource");
  bareBenchmark("bmTopdownNewSource");
  sg::detail::expandRegistry();

  const RunAccount unasked = runMeasured(
      *needInstance("bmTopdownUnaskedSource"), optionsFor(kRunRepetitions));
  sg::Instance& askedInstance = *needInstance("bmTopdownNewSource");
  const RunAccount asked = runMeasured(
      askedInstance,
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));
  const RunAccount askedLow = runMeasured(
      askedInstance,
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kLowRepetitions));
  const RunAccount askedHigh = runMeasured(
      askedInstance,
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kHighRepetitions));
  const std::uint64_t marginal = askedHigh.reads - askedLow.reads;

  check(unasked.result.metricMethod == sg::MetricMethod::AUTO
            && unasked.result.passCount == 0,
        "a run that collects nothing settles to `auto` and states no pass "
        "(FR-022)");
  check(asked.result.metricMethod == sg::MetricMethod::PASSES,
        "the source the node columns bring in publishes its limit through the "
        "object fact alone, and that limit settles the run to `passes` (FR-022)");
  check(asked.result.passCount == kLevelOnePassCount,
        "one price for the new source splits the four level-1 columns at the "
        "limit it publishes (FR-022)");
  check(marginal == kLevelOnePassCount * kWindowReads,
        "the plan that one price produced opens one window per pass of the run "
        "(FR-022)");

  std::puts("a-new-source-reads-its-limit-once: ok");
  return 0;
}

// The fifth scenario: the record of a run carries every collected node figure
// with the availability the merge published, beside the names, the level and
// the table the rows came from (FR-032, FR-030, FR-017, FR-020, FR-033).
auto scenarioTheNodeFigureStandsInTheRecord() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownRecordFigures");
  metricBenchmark("bmTopdownRecordSingle");
  sg::detail::expandRegistry();

  const std::vector<MetricRow> rows = sg::test::topdownIntelRows();
  const RunAccount figures =
      runMeasured(*needInstance("bmTopdownRecordFigures"),
                  askedOptions(rows, kLevelOne, kRecordRepetitions));
  const RunAccount single =
      runMeasured(*needInstance("bmTopdownRecordSingle"),
                  askedOptions(rows, kLevelOne, kRunRepetitions));

  check(figures.result.topdown.has_value(),
        "a run with the option carries a top-down record (FR-032)");
  check(single.result.topdown.has_value(),
        "a run of one repetition carries the same record shape (FR-032)");
  const sg::TopdownRecord& record = *figures.result.topdown;
  const sg::TopdownRecord& alone = *single.result.topdown;

  check(record.supported,
        "the record states that the selection answered a tree (FR-033)");
  check(record.reason.empty(),
        "a record that answers a tree states no reason (FR-033, FR-041)");
  check(record.askedLevel == kLevelOne && record.level == kLevelOne,
        "the record carries the level the run asked for and the level it shows "
        "(FR-020)");
  check(record.directory == kIntelDirectory,
        "the record names the directory its rows came from (FR-015, FR-032)");
  check(record.nodes.size() == kLevelOneNodeColumns,
        "the record holds one node record per collected node (FR-032)");
  check(record.refused.empty(),
        "the level-1 ask over the fixture table leaves no refused row (FR-016, "
        "FR-033)");

  constexpr std::array<std::string_view, 4> kNeutralNames {{
      sg::test::kNeutralFrontendBound,
      sg::test::kNeutralBadSpeculation,
      sg::test::kNeutralBackendBound,
      sg::test::kNeutralRetiring,
  }};
  for (std::size_t node = 0; node < record.nodes.size(); ++node) {
    const sg::TopdownNode& entry = record.nodes[node];
    check(entry.vendorName == kLevelOneVendorNames[node],
          "the node carries its own table name, in the order the table spells "
          "the rows (FR-032)");
    check(entry.name == kNeutralNames[node],
          "the node carries the neutral name beside its vendor name (FR-032)");
    check(entry.parent.empty(),
          "a node of the level the run shows names no parent (FR-032)");
    check(entry.level == kLevelOne, "the node states its own level (FR-014)");
    check(entry.threshold.empty(),
          "a row that carries no threshold text leaves the record field empty "
          "(FR-017, FR-029)");
    check(entry.availability == sg::counters::Availability::COUNTABLE,
          "the node carries the availability the merge published (FR-030)");
    check(entry.value.has_value()
              && closeTo(*entry.value, kLevelOnePercents[node]),
          "the node's figure is the percentage the scripted leaves fold to "
          "(FR-030, SC-03)");
  }

  check(alone.nodes.size() == record.nodes.size(),
        "one record stands per run, whatever the repetition count (FR-032)");
  for (std::size_t node = 0; node < record.nodes.size(); ++node) {
    check(alone.nodes[node].value.has_value()
              && closeTo(*alone.nodes[node].value, *record.nodes[node].value),
          "one node figure answers a run of two repetitions and a run of one, "
          "so the record summarizes the repetitions the merge published (FR-030)");
  }
  check(figures.result.rows.size() == kRecordRepetitions + 1,
        "the two repetitions stand one aggregate row beside their own rows "
        "(FR-027)");

  std::puts("the-node-figure-stands-in-the-record: ok");
  return 0;
}

// The sixth scenario: the strip leaves every metric view of the result aligned
// on the columns the run reports as metrics, with its percentiles on and off
// (FR-023, FR-024).
auto scenarioTheStripKeepsEveryMetricViewAligned() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownStripPlainGrid");
  metricBenchmark("bmTopdownStripAskedGrid");
  metricBenchmark("bmTopdownStripPlainValue");
  metricBenchmark("bmTopdownStripAskedValue");
  sg::detail::expandRegistry();

  const std::vector<MetricRow> rows = sg::test::topdownIntelRows();
  const LedgerAccount grid =
      runMeasuredLedger(*needInstance("bmTopdownStripPlainGrid"),
                        percentilesOn(optionsFor(kRecordRepetitions)));
  const LedgerAccount askedGrid = runMeasuredLedger(
      *needInstance("bmTopdownStripAskedGrid"),
      percentilesOn(askedOptions(rows, kLevelOne, kRecordRepetitions)));
  const RunAccount values =
      runMeasured(*needInstance("bmTopdownStripPlainValue"),
                  optionsFor(kRecordRepetitions));
  const RunAccount askedValues =
      runMeasured(*needInstance("bmTopdownStripAskedValue"),
                  askedOptions(rows, kLevelOne, kRecordRepetitions));

  check(askedGrid.run.result.passCount == kOnePlanPassCount,
        "the run whose views this case walks collected the node columns and "
        "planned them with the metric columns (FR-023)");
  check(askedGrid.run.result.metricLabels == grid.run.result.metricLabels,
        "no node figure stays in the metric labels of the run (FR-023)");
  check(askedGrid.run.result.metricLabels.size() == kMetricColumnCount,
        "the labels of the run hold the metric columns it attached (FR-023)");
  check(askedGrid.run.result.rows.size() == grid.run.result.rows.size(),
        "the strip leaves the row count where the run left it (FR-024)");

  const std::size_t expectedGrid = kTimeColumns + kMetricColumnCount;
  for (std::size_t row = 0; row < grid.run.result.rows.size(); ++row) {
    const sg::ResultRow& before = grid.run.result.rows[row];
    const sg::ResultRow& after = askedGrid.run.result.rows[row];
    const sg::ResultRow& afterValues = askedValues.result.rows[row];
    check((after.metrics.empty()
           || after.metrics.size() == askedGrid.run.result.metricLabels.size())
              && afterValues.metrics.size() == before.metrics.size(),
          "every row that carries metric values carries one per metric label "
          "of " "the run, with its percentiles on and off (FR-023)");
    check(after.metricStats.size() == before.metricStats.size(),
          "the row holds one statistic set per metric column of the run "
          "(FR-023)");
    check(after.percentiles.size() == before.percentiles.size(),
          "the percentile grid of the row holds the columns the row prints, "
          "the " "node positions gone (FR-023, FR-024)");
    check(after.percentiles.empty() || after.percentiles.size() == expectedGrid,
          "the percentile grid stands two time slots beside the metric columns "
          "of the run (FR-024)");
    check(
        before.percentiles.empty() || before.percentiles.size() == expectedGrid,
        "the same grid answers the run without the option (FR-024)");
    check(askedGrid.rowRaw[row].columns.size()
              == grid.rowRaw[row].columns.size(),
          "the retained chunk columns of the row hold the same width as the "
          "same " "row without the option (FR-023, FR-029)");
    check(askedGrid.rowRaw[row].columns.empty()
              || askedGrid.rowRaw[row].columns.size() == expectedGrid,
          "the retained chunk columns stand two time slots beside the metric "
          "columns of the run (specs/023 E-02)");
    check(askedGrid.contextRaw[row].size() == grid.contextRaw[row].size(),
          "the strip leaves the per-context ledgers of the row as they stood "
          "(FR-023)");
    for (std::size_t column = 0; column + 2 < after.percentiles.size();
         ++column)
    {
      check(after.percentiles[column + 2].size()
                == before.percentiles[column + 2].size(),
            "one metric column of the grid holds one value per percentile "
            "point " "of the run (FR-024)");
    }
  }
  check(askedGrid.run.result.contextPercentiles.empty()
            && grid.run.result.contextPercentiles.empty(),
        "a single-context run answers no context percentiles, with the option "
        "standing and without it (FR-023)");

  std::puts("the-strip-keeps-every-metric-view-aligned: ok");
  return 0;
}

// The seventh scenario: the strip changes no time column and no row field of
// the run. Every column of every row, and every statistic set beside it,
// answers byte for byte what the same run answers without the option. Three
// views stand outside that bar because the plan names them: the pass tag of a
// metric value, the overhead floor, which measures what the reference window
// costs to sample, and the chunk pair, whose size follows the floor through the
// 1% rule (specs/024 FR-013, specs/021 FR-005). The percentile grid and the
// retained chunk columns are folds over those chunks, so this case runs with no
// percentile reported and the previous case holds the width of both views
// (FR-023, FR-024).
auto scenarioTheStripChangesNoTimeColumn() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownTimePlain");
  metricBenchmark("bmTopdownTimeAsked");
  sg::detail::expandRegistry();

  const std::vector<MetricRow> rows = sg::test::topdownIntelRows();
  const LedgerAccount plain = runMeasuredLedger(
      *needInstance("bmTopdownTimePlain"), optionsFor(kRecordRepetitions));
  const LedgerAccount asked =
      runMeasuredLedger(*needInstance("bmTopdownTimeAsked"),
                        askedOptions(rows, kLevelOne, kRecordRepetitions));

  check(asked.run.result.rows.size() == plain.run.result.rows.size(),
        "the strip leaves the run's row count where it stood (FR-024)");
  for (std::size_t row = 0; row < plain.run.result.rows.size(); ++row) {
    const sg::ResultRow& before = plain.run.result.rows[row];
    const sg::ResultRow& after = asked.run.result.rows[row];
    check(after.metrics.size() == before.metrics.size()
              && after.metricStats.size() == before.metricStats.size(),
          "a row reports as many metric columns and as many statistic sets as "
          "the same row of the run without the option (FR-023)");
    check(after.percentiles.size() == before.percentiles.size()
              && asked.rowRaw[row].columns.size()
                     == plain.rowRaw[row].columns.size(),
          "a run that reports no percentile leaves the grid and the retained "
          "chunk columns of the row empty in both arms (FR-024)");
    check(after.kind == before.kind, "the row keeps its kind (FR-024)");
    check(after.iterations == before.iterations,
          "the row reports the iteration count the run without the option "
          "reported (FR-024)");
    check(sameBytes(after.timePerIterationNs, before.timePerIterationNs)
              && sameBytes(after.cpuTimePerIterationNs,
                           before.cpuTimePerIterationNs),
          "the option changes no time column of the row (FR-024)");
    check(after.timeAvailability == before.timeAvailability
              && after.pauseSamplesFolded == before.pauseSamplesFolded,
          "the primary fold keeps its availability and its pause disclosure "
          "(FR-024)");
    check(after.endpointSamples == before.endpointSamples,
          "the row discloses the endpoint samples the same row discloses "
          "without " "the option (FR-024)");
    check(after.chunkCount == before.chunkCount
              && after.chunkIterations == before.chunkIterations,
          "the chunk pair of the row stands where it stood (FR-024)");
    check(after.label == before.label,
          "the row keeps the label its body wrote (FR-024)");
    check(sameStats(after.timeStats, before.timeStats),
          "the aggregate time statistics of the row answer the same bytes "
          "(FR-024, FR-027)");
    check(after.counterDisclosure == before.counterDisclosure
              && after.counters == before.counters,
          "the counter view of the row stands untouched (FR-024)");
    for (std::size_t column = 0; column < before.metrics.size(); ++column) {
      check(sameBytes(after.metrics[column].value, before.metrics[column].value)
                && sameBytes(after.metrics[column].runningRatio,
                             before.metrics[column].runningRatio)
                && after.metrics[column].scaled == before.metrics[column].scaled
                && after.metrics[column].availability
                       == before.metrics[column].availability,
            "a metric column folds the bytes the same column folds without the "
            "option, the pass tag excepted (FR-023)");
    }
    for (std::size_t column = 0; column < before.metricStats.size(); ++column) {
      check(sameStats(after.metricStats[column], before.metricStats[column]),
            "the statistic set of a metric column answers the same bytes "
            "(FR-023, FR-027)");
    }
  }

  std::puts("the-strip-changes-no-time-column: ok");
  return 0;
}

// The eighth scenario: a selection that answers no tree records itself and
// moves nothing else. The run collects no node column, keeps the plan its own
// metric columns settled, answers the status the same run answers without the
// option, and prints no line on either stream, because the one message line
// belongs to the command line and T080 owns it (FR-040, FR-041, spec D-14).
auto scenarioAnUnsupportedHostCollectsNothingAndStaysItself() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownRefusalPlain");
  metricBenchmark("bmTopdownRefusalAsked");
  sg::detail::expandRegistry();

  // The rows stand published and the directory stands absent: the mapping file
  // answered no row of its own for this identity, so no row of the table
  // reaches a column and no node reaches the record (FR-040, FR-016).
  const sg::detail::RunOptions asked =
      askedTable(plantedTable("", sg::test::topdownIntelRows()),
                 kLevelOne,
                 kRunRepetitions);
  const RunAccount plain = runMeasured(*needInstance("bmTopdownRefusalPlain"),
                                       optionsFor(kRunRepetitions));
  const CapturedRun refusal =
      runMeasuredQuietly(*needInstance("bmTopdownRefusalAsked"), asked);

  check(refusal.out.empty() && refusal.err.empty(),
        "the refusal prints no line on either stream, because the one message "
        "line stands in the command line (FR-040)");
  check(refusal.run.result.outcome == plain.result.outcome
            && refusal.run.result.reason == plain.result.reason,
        "the run answers the status the same run answers without the option "
        "(FR-041)");
  check(refusal.run.result.topdown.has_value(),
        "the run carries the record of the refusal it met (FR-040, FR-041)");
  const sg::TopdownRecord& record = *refusal.run.result.topdown;
  check(
      !record.supported,
      "the record states that the selection answered no tree (FR-040, FR-033)");
  check(record.nodes.empty() && record.refused.empty() && record.files.empty(),
        "a record that states no support carries no node, no refused row and "
        "no " "file (FR-041, FR-033)");
  check(record.directory.empty() && record.reference.empty(),
        "no table stands behind a refusal (FR-015)");
  check(record.askedLevel == kLevelOne && record.level == 0,
        "the record states the level the run asked for and shows no level "
        "(FR-020, FR-041)");

  check(record.reason == refusalOf(asked).message(),
        "the reason of the record is the one line the selection answered, so a "
        "later reader of the document learns why the tree stands absent "
        "(FR-033, FR-040)");
  check(record.reason.starts_with("topdown: ")
            && record.reason.contains(
                "the mapping file names no table for this identity"),
        "the reason of the record carries the tag and the reason of a "
        "mapping-file miss on one line (FR-033, FR-040)");

  check(refusal.run.result.metricLabels == plain.result.metricLabels,
        "the refusal collects no node column (FR-040, FR-023)");
  check(refusal.run.result.passCount == plain.result.passCount
            && refusal.run.result.metricMethod == plain.result.metricMethod,
        "the refusal leaves the plan the metric columns settled where it stood "
        "(FR-040, FR-023)");
  check(refusal.run.reads == plain.reads,
        "the refusal costs the run no sampling action of its own (FR-040, "
        "FR-009)");
  check(refusal.run.result.rows.size() == plain.result.rows.size(),
        "the refusal leaves the run the rows it had (FR-041)");
  for (std::size_t row = 0; row < plain.result.rows.size(); ++row) {
    const sg::ResultRow& before = plain.result.rows[row];
    const sg::ResultRow& after = refusal.run.result.rows[row];
    check(after.iterations == before.iterations
              && sameBytes(after.timePerIterationNs, before.timePerIterationNs)
              && sameBytes(after.cpuTimePerIterationNs,
                           before.cpuTimePerIterationNs)
              && after.metrics.size() == before.metrics.size(),
          "the row of the refusal is the row of the run without the option "
          "(FR-041)");
  }

  std::puts("an-unsupported-host-collects-nothing-and-stays-itself: ok");
  return 0;
}

// One planted table with the source record a loader fills beside the directory,
// so the block's reference and digest lines have text to print (FR-015).
[[nodiscard]] auto tableWithSource(sg::counters::TopdownTable table)
    -> sg::counters::TopdownTable
{
  table.source.reference = std::string {kPlantedReference};
  table.source.files.push_back(sg::counters::TableFile {
      .name = std::string {kPlantedFileName},
      .manifestHash = std::string {kPlantedFileHash},
  });
  return table;
}

// The figure text one node line carries: the printed percent of the figure, to
// two decimals, so a reader can add the four level-1 values (FR-027, SC-03).
[[nodiscard]] auto figureText(const double figure) -> std::string
{
  char text[64] {};
  std::snprintf(text, sizeof(text), "%.2f%%", figure);
  return text;
}

// The first scenario of the block: one block stands per benchmark under that
// benchmark's own tables, and a threaded run stands in the same rule as a
// single-context run (FR-025).
auto scenarioTheBlockStandsAfterTheResultTables() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownBlockFirst");
  metricBenchmark("bmTopdownBlockSecond");
  metricBenchmark("bmTopdownBlockThreaded");
  sg::detail::expandRegistry();

  const std::vector<MetricRow> rows = sg::test::topdownIntelRows();
  const RunAccount first =
      runMeasured(*needInstance("bmTopdownBlockFirst"),
                  askedOptions(rows, kLevelOne, kRecordRepetitions));
  const RunAccount second =
      runMeasured(*needInstance("bmTopdownBlockSecond"),
                  askedOptions(rows, kLevelOne, kRecordRepetitions));
  sg::Instance& threaded = *needInstance("bmTopdownBlockThreaded");
  threaded.threadCount = 2;
  const RunAccount merged =
      runMeasured(threaded, askedOptions(rows, kLevelOne, kRunRepetitions));

  const auto checkBlock =
      [](const char* const name, const sg::BenchmarkResult& result)
  {
    const std::vector<std::string> lines = printedLines(result);
    const std::string headerPrefix = std::string {kBlockTag} + " level=";
    const auto header = lineAt(lines, headerPrefix);
    check(header.has_value(),
          "the report of a run with the option carries one top-down header "
          "under " "its benchmark (FR-025)");
    std::size_t lastRow = 0;
    bool anyRow = false;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      if (lines[index].rfind(name, 0) == 0
          && lines[index].contains("iterations="))
      {
        lastRow = index;
        anyRow = true;
      }
    }
    check(anyRow && header.has_value() && *header > lastRow,
          "the block stands under the rows of its own benchmark (FR-025)");
    std::size_t headers = 0;
    for (const std::string& line : blockLinesOf(lines)) {
      headers += line.rfind(headerPrefix, 0) == 0 ? 1U : 0U;
    }
    check(headers == 1, "one block stands per benchmark (FR-025)");
  };
  checkBlock("bmTopdownBlockFirst", first.result);
  checkBlock("bmTopdownBlockSecond", second.result);
  checkBlock("bmTopdownBlockThreaded", merged.result);

  const std::vector<std::string> mergedLines = printedLines(merged.result);
  const auto mergedHeader =
      lineAt(mergedLines, std::string {kBlockTag} + " level=");
  const auto contexts = [&]() -> std::optional<std::size_t>
  {
    const auto found = std::find_if(mergedLines.begin(),
                                    mergedLines.end(),
                                    [](const std::string& line)
                                    { return line.contains("contexts="); });
    if (found == mergedLines.end()) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(found - mergedLines.begin());
  }();
  check(contexts.has_value() && mergedHeader.has_value()
            && *contexts > *mergedHeader,
        "the threaded run's context disclosure stands under its block, so the "
        "block reached the threaded path too (FR-025)");

  std::puts("the-block-stands-after-the-result-tables: ok");
  return 0;
}

// The second scenario of the block: the header names the level the run shows,
// the passes its one plan produced and the table the rows came from, and the
// source record follows with the pinned reference and the file digest (FR-026,
// FR-015).
auto scenarioTheBlockStatesTheLevelThePassesAndTheTable() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownBlockHeader");
  sg::detail::expandRegistry();

  const RunAccount run = runMeasured(
      *needInstance("bmTopdownBlockHeader"),
      askedTable(tableWithSource(plantedTable(kIntelDirectory,
                                              sg::test::topdownIntelRows())),
                 kLevelOne,
                 kRunRepetitions));
  const std::vector<std::string> block = blockLinesOf(printedLines(run.result));
  check(block.size() == 1 + 1 + 1 + kLevelOneNodeColumns,
        "the block carries one header, the reference, one line per file that "
        "contributed a row, and one line per node (FR-015, FR-026)");

  char header[160] {};
  std::snprintf(header,
                sizeof(header),
                "topdown: level=%llu passes=%zu table=%s",
                static_cast<unsigned long long>(kLevelOne),
                kOnePlanPassCount,
                std::string {kIntelDirectory}.c_str());
  check(block.front() == header,
        "the header states the level shown, the passes the plan produced and "
        "the " "table the rows came from (FR-026, FR-021, FR-015)");
  const auto reference = lineAt(block, "topdown: reference=");
  std::size_t fileLines = 0;
  for (const std::string& line : block) {
    fileLines +=
        line.rfind(std::string {kBlockTag} + " file ", 0) == 0 ? 1U : 0U;
  }
  check(reference.has_value()
            && block[*reference]
                   == std::string {kBlockTag} + " reference="
                          + std::string {kPlantedReference}
            && fileLines == 1,
        "the block carries the reference the manifest states and one line per "
        "file that contributed a row (FR-015)");
  const auto file = lineAt(block, "topdown: file ");
  check(file.has_value() && block[*file].contains(kPlantedFileName)
            && block[*file].contains(kPlantedFileHash),
        "the block carries the file and the digest that contributed the rows "
        "(FR-015)");

  std::puts("the-block-states-the-level-the-passes-and-the-table: ok");
  return 0;
}

// The third scenario of the block: the four level-1 nodes print in the order
// the table spells them, each with its neutral name and the vendor name it came
// from (FR-027).
auto scenarioTheFourLevelOneNamesPrintWithTheirVendorNames() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownBlockNames");
  sg::detail::expandRegistry();

  const RunAccount run = runMeasured(
      *needInstance("bmTopdownBlockNames"),
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));
  const std::vector<std::string> block = blockLinesOf(printedLines(run.result));
  check(
      block.size() == 4 + 1,
      "the block carries one header and one line per collected node (FR-026)");

  for (std::size_t node = 0; node < kLevelOneNeutralNames.size(); ++node) {
    const std::string& line = block[node + 1];
    check(line.contains(kLevelOneNeutralNames[node])
              && line.contains(kLevelOneVendorNames[node]),
          "the node line carries the neutral name and the vendor name of the "
          "row, " "in the order the table spells the rows (FR-027)");
    check(line.contains(figureText(kLevelOnePercents[node])),
          "the node line carries its figure in percent (FR-027)");
  }

  std::puts("the-four-level-one-names-print-with-their-vendor-names: ok");
  return 0;
}

// The fourth scenario of the block: each printed figure is the figure the
// scripted leaves fold to, at the rounding the block prints (SC-03).
auto scenarioEachFigureEqualsWhatTheFixtureScripts() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  metricBenchmark("bmTopdownBlockFigures");
  sg::detail::expandRegistry();

  const RunAccount run = runMeasured(
      *needInstance("bmTopdownBlockFigures"),
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));
  const std::vector<std::string> block = blockLinesOf(printedLines(run.result));
  check(block.size() == 4 + 1,
        "the four nodes of the run print their own lines (FR-026)");
  for (std::size_t node = 0; node < kLevelOnePercents.size(); ++node) {
    const std::string expected = std::string {kBlockTag} + "  "
        + std::string {kLevelOneNeutralNames[node]} + "  "
        + std::string {kLevelOneVendorNames[node]} + "  "
        + figureText(kLevelOnePercents[node]);
    check(block[node + 1] == expected,
          "one node line answers the figure the scripted leaves fold to, at "
          "the " "two-decimal rounding the block prints (SC-03, FR-027)");
  }

  std::puts("each-figure-equals-what-the-fixture-scripts: ok");
  return 0;
}

// The ninth scenario of the block: a record that states no support prints no
// block, because the one line an unsupported host owes the reader belongs to
// the command line, where the refusal is answered before any benchmark runs
// (FR-040, FR-041, T080). The runner publishes that record for the report
// writers, so the block has to leave it alone here.
auto scenarioAnUnsupportedRecordPrintsNoBlock() -> int
{
  sg::BenchmarkResult result;
  result.name = "bmTopdownUnsupportedRecord";
  sg::ResultRow row;
  row.iterations = 1;
  row.timePerIterationNs = 100.0;
  row.cpuTimePerIterationNs = 100.0;
  row.endpointSamples = 2;
  result.rows.push_back(row);
  sg::TopdownRecord record;
  record.supported = false;
  record.askedLevel = 3;
  record.reason = "the mapping file names no row for this identity";
  result.topdown = std::move(record);

  const std::vector<std::string> lines = printedLines(result);
  check(blockLinesOf(lines).empty(),
        "the report of a refusal record prints no top-down block (FR-040, "
        "FR-041)");
  check(lineAt(lines, "bmTopdownUnsupportedRecord").has_value(),
        "the report of a refusal record still prints its own row (FR-040)");

  std::puts("an-unsupported-record-prints-no-block: ok");
  return 0;
}

// The eighth scenario of the block: a tree asked one level deeper prints its
// children indented two spaces per level below one, and a node whose leaves the
// fixture publishes as a gap prints `not available` where its figure stands.
// Both belong to the shape D-12 states for the block, and the scripted leaves
// settle a gap only through a hand-built record, because a planted table
// answers the same availability for every node of the run (FR-027, FR-030,
// D-12).
auto scenarioTheDeeperTreePrintsItsIndentationAndItsGap() -> int
{
  sg::BenchmarkResult result;
  result.name = "bmTopdownDeepTree";
  result.passCount = kOnePlanPassCount;
  sg::TopdownRecord record;
  record.supported = true;
  record.level = 2;
  record.askedLevel = 2;
  record.directory = std::string {kIntelDirectory};
  record.nodes.push_back(sg::TopdownNode {
      .name = std::string {sg::test::kNeutralFrontendBound},
      .vendorName = "tma_frontend_bound",
      .parent = {},
      .level = 1,
      .value = 200.0,
      .availability = sg::counters::Availability::COUNTABLE,
      .threshold = {},
  });
  record.nodes.push_back(sg::TopdownNode {
      .name = "topdown.frontend_bound.latency",
      .vendorName = "tma_fetch_latency",
      .parent = std::string {sg::test::kNeutralFrontendBound},
      .level = 2,
      .value = 120.0,
      .availability = sg::counters::Availability::COUNTABLE,
      .threshold = {},
  });
  record.nodes.push_back(sg::TopdownNode {
      .name = "topdown.frontend_bound.bandwidth",
      .vendorName = "tma_fetch_bandwidth",
      .parent = std::string {sg::test::kNeutralFrontendBound},
      .level = 2,
      .value = std::nullopt,
      .availability = sg::counters::Availability::GAP,
      .threshold = {},
  });
  result.topdown = std::move(record);

  const std::vector<std::string> block = blockLinesOf(printedLines(result));
  check(block.size() == 1 + 3,
        "the block prints one line per node of the tree (FR-026)");
  check(block.front()
            == std::string {kBlockTag}
                + " level=2 passes=" + std::to_string(kOnePlanPassCount)
                + " table=" + std::string {kIntelDirectory},
        "the header states the deeper level the run shows (FR-026)");
  check(block[1]
            == std::string {kBlockTag} + "  "
                + std::string {sg::test::kNeutralFrontendBound} + "  "
                + "tma_frontend_bound  " + figureText(200.0),
        "a level-1 node prints with no indent beyond the tag (FR-027)");
  check(block[2]
            == std::string {kBlockTag} + "    topdown.frontend_bound.latency  "
                + "tma_fetch_latency  " + figureText(120.0),
        "a level-2 node prints indented two spaces below its parent (FR-027, "
        "D-12)");
  check(block[3]
            == std::string {kBlockTag}
                + "    topdown.frontend_bound.bandwidth  "
                + "tma_fetch_bandwidth  not available",
        "a node the leaves answer as a gap prints `not available` in place of "
        "its " "figure (FR-030)");

  std::puts("the-deeper-tree-prints-its-indentation-and-its-gap: ok");
  return 0;
}

// The fifth scenario of the block: an AMD table prints the fifth level-1 value
// beside the four shared ones and states that the four add to less than 100
// percent without it. The record carries no vendor flag, so the block reads the
// fifth name (FR-028, FR-056).
auto scenarioTheAmdBlockPrintsTheFifthValueAndTheStatement() -> int
{
  sg::BenchmarkResult result;
  result.name = "bmTopdownAmdBlock";
  result.passCount = kOnePlanPassCount;
  sg::TopdownRecord record;
  record.supported = true;
  record.level = kLevelOne;
  record.askedLevel = kLevelOne;
  record.directory = std::string {sg::test::kZen4Directory};
  constexpr std::array<std::string_view, 5> kNeutral {{
      sg::test::kNeutralFrontendBound,
      sg::test::kNeutralBadSpeculation,
      sg::test::kNeutralBackendBound,
      sg::test::kNeutralSmtContention,
      sg::test::kNeutralRetiring,
  }};
  constexpr std::array<double, 5> kValues {{30.0, 5.0, 50.0, 10.0, 5.0}};
  for (std::size_t node = 0; node < kNeutral.size(); ++node) {
    record.nodes.push_back(sg::TopdownNode {
        .name = std::string {kNeutral[node]},
        .vendorName = std::string {sg::test::kAmdLevelOneNames[node]},
        .parent = {},
        .level = kLevelOne,
        .value = kValues[node],
        .availability = sg::counters::Availability::COUNTABLE,
        .threshold = {},
    });
  }
  result.topdown = std::move(record);

  const std::vector<std::string> block = blockLinesOf(printedLines(result));
  check(block.size() == 5 + 2,
        "the block carries the header, the five nodes and the statement "
        "(FR-028)");
  check(block[4]
            == std::string {kBlockTag} + "  "
                + std::string {sg::test::kNeutralSmtContention} + "  "
                + "smt_contention  " + figureText(10.0),
        "the fifth level-1 value prints with its neutral name and its vendor "
        "name (FR-028, FR-056)");
  check(block.back().contains(sg::test::kNeutralSmtContention)
            && block.back().contains("100 percent"),
        "the block states that the four shared values add to less than 100 "
        "percent " "without the fifth (FR-028)");
  double shared = 0.0;
  for (std::size_t node = 0; node < kValues.size(); ++node) {
    shared += kValues[node];
  }
  check(closeTo(shared, 100.0),
        "the five values of the scripted AMD row set close the hundred percent "
        "(FR-056)");

  std::puts("the-amd-block-prints-the-fifth-value-and-the-statement: ok");
  return 0;
}

// The sixth scenario of the block: a run that collected the tree and no metric
// column still states the passes its one plan produced, because the plan is the
// tree's own (FR-021).
auto scenarioARunWithNoMetricColumnStillStatesItsPasses() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);

  bareBenchmark("bmTopdownBarePasses");
  sg::detail::expandRegistry();

  const RunAccount run = runMeasured(
      *needInstance("bmTopdownBarePasses"),
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));
  check(run.result.metricLabels.empty(),
        "the run attached no metric column of its own (FR-021)");
  check(run.result.passCount == kLevelOnePassCount,
        "the tree alone settled to the passes its one plan produced (FR-021)");

  const std::vector<std::string> lines = printedLines(run.result);
  const auto passLine = [&]() -> std::optional<std::size_t>
  {
    const auto found = std::find_if(lines.begin(),
                                    lines.end(),
                                    [](const std::string& line) {
                                      return line.contains("  passes  method=");
                                    });
    if (found == lines.end()) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(found - lines.begin());
  }();
  const auto header = lineAt(lines, std::string {kBlockTag} + " level=");
  check(passLine.has_value(),
        "a run with no metric column states the passes its plan produced "
        "(FR-021)");
  check(passLine.has_value() && header.has_value() && *passLine < *header,
        "the pass line stands above the block, where it stands for a run with "
        "metric columns (FR-025, FR-021)");
  check(header.has_value()
            && lines[*header]
                == std::string {kBlockTag}
                    + " level=1 passes=" + std::to_string(kLevelOnePassCount)
                    + " table=" + std::string {kIntelDirectory},
        "the block of the tree-only run states the passes beside the level "
        "(FR-021, FR-026)");

  std::puts("a-run-with-no-metric-column-still-states-its-passes: ok");
  return 0;
}

// The mode that re-executes this binary as a child driving the entry point, and
// the marker that child prints before the entry point reaches its own report:
// the bare-word case reads both, because SC-05's fifth case is about the entry
// point and not about a runner driven directly.
constexpr std::string_view kEntryPointMode {"entry-point-child"};
constexpr std::string_view kEntryPointMarker {"ENTRY_POINT_RAN"};

// argv[0] of this binary, which the bare-word case re-executes.
const char* selfPath = "harness_topdown_run_test";

auto runEntryPointChild() -> int
{
  registerFixture(kNarrowCoreLimit);
  static_cast<void>(sg::registerBenchmark(&plainBody, "bmTopdownEntry"));
  std::printf("%s\n", std::string {kEntryPointMarker}.c_str());
  std::fflush(stdout);
  std::vector<std::string> arguments {
      "--topdown", "--iterations=1", "--repetitions=1"};
  std::vector<char*> argv {const_cast<char*>(selfPath)};
  for (std::string& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The mode that re-executes this binary to read the scope of a run off the
// whole console. The variant names whether the option stands. The child prints
// one probe line with what `coreWideCoverage()` answered before the entry point
// ran, because FR-042 lets an unreadable cpu set answer nothing and a case may
// not assume which way this host settled; two benchmarks stand so that a line
// printed once per benchmark is visible as what it is (FR-042).
constexpr std::string_view kScopeChildMode {"scope-child"};
constexpr std::string_view kScopeOptionVariant {"option"};
constexpr std::string_view kScopeProbeMarker {"PROBE_COVERAGE="};
constexpr std::string_view kScopeLinePrefix {"topdown: core-wide coverage="};

// The two words the console carries for the two readings in the enumerator's
// own spelling, and `none` for a host that states no sibling lists. The probe
// line and the console line carry the same words, because one call of
// `coreWideCoverage()` settles the cached value D-06 describes: the child's
// probe and the line the entry point prints are one reading, so the case
// compares a statement against the value it restates.
[[nodiscard]] auto coverageText(
    const std::optional<sg::counters::CoreCoverage>& coverage) -> std::string
{
  if (!coverage.has_value()) {
    return "none";
  }
  return *coverage == sg::counters::CoreCoverage::WHOLE_CORE ? "whole_core"
                                                             : "one_thread";
}

auto runScopeChild(const std::string& variant) -> int
{
  registerFixture(kNarrowCoreLimit);
  static_cast<void>(sg::registerBenchmark(&plainBody, "bmTopdownScopeFirst"));
  static_cast<void>(sg::registerBenchmark(&plainBody, "bmTopdownScopeSecond"));
  std::printf("%s%s\n",
              std::string {kScopeProbeMarker}.c_str(),
              coverageText(sg::counters::coreWideCoverage()).c_str());
  std::fflush(stdout);
  std::vector<std::string> arguments {};
  if (variant == kScopeOptionVariant) {
    arguments.push_back("--topdown");
  }
  arguments.push_back("--iterations=1");
  arguments.push_back("--repetitions=1");
  std::vector<char*> argv {const_cast<char*>(selfPath)};
  for (std::string& argument : arguments) {
    argv.push_back(argument.data());
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
}

// The whole console of one scope child: its exit status, its lines, and the
// reading its own probe answered.
struct ScopeConsole
{
  int status = -1;
  std::vector<std::string> lines;
  std::string probe;
};

[[nodiscard]] auto scopeConsole(const std::string& variant) -> ScopeConsole
{
  const std::string command = "\"" + std::string {selfPath} + "\" "
      + std::string {kScopeChildMode} + " " + variant + " > " + kOutPath
      + " 2>&1";
  ScopeConsole console;
  console.status = std::system(command.c_str());
  std::istringstream stream(textOf(kOutPath));
  const std::string marker {kScopeProbeMarker};
  for (std::string line; std::getline(stream, line);) {
    if (line.rfind(marker, 0) == 0) {
      console.probe = line.substr(marker.size());
      continue;
    }
    console.lines.push_back(line);
  }
  return console;
}

// The lines of one console that carry the scope statement.
[[nodiscard]] auto scopeLines(const ScopeConsole& console)
    -> std::vector<std::string>
{
  std::vector<std::string> found;
  const std::string prefix {kScopeLinePrefix};
  for (const std::string& line : console.lines) {
    if (line.rfind(prefix, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

// The seventh scenario of the block: the option word with no attached value
// prints a tree at level one and leaves the run's status at zero, the fifth
// case SC-05 names beside its four refusal statuses (FR-018, FR-025; SC-05).
auto scenarioTheBareWordPrintsATreeAtStatusZero() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  const sg::counters::TopdownTable vendored =
      sg::counters::topdownTable(sg::counters::cpuIdentity());
  // The library answers whether any tree stands on this host: an identity no
  // table serves, a table that names no directory, and a directory that holds
  // no row of any top-down group each answer one `TopdownUnsupported` in place
  // of a tree, and the child of this case prints no block for any of them. The
  // skip states the line the library owns (FR-004 to FR-007, FR-040).
  const sg::counters::TopdownSelection asked {
      .identity = sg::counters::cpuIdentity(),
      .level = static_cast<int>(kLevelOne),
      .table = std::nullopt};
  const auto answered = sg::counters::topdownTree(asked);
  if (!answered.has_value()) {
    skip(answered.error().message().c_str());
  }

  const std::string command = "\"" + std::string {selfPath} + "\" "
      + std::string {kEntryPointMode} + " > " + kOutPath + " 2>&1";
  const int status = std::system(command.c_str());
  const std::string output = textOf(kOutPath);
  check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "the option word with no attached value leaves the run's status at "
        "zero (SC-05)");
  check(output.contains(kEntryPointMarker),
        "the run under the bare word reaches its benchmark (SC-05)");
  const std::string header = std::string {kBlockTag} + " level=1 ";
  if (!output.contains(header)
      && output.contains(std::string {kBlockTag} + "  refused  "))
  {
    // The child prints a refusal line per row its host reaches no leaf of, and
    // a host that refuses the level rows prints no tree whatever the option
    // asks: the claim of this case stands where a tree stands (FR-016, FR-031).
    skip("the host of the child refuses its own level rows, so no tree stands "
         "here");
  }
  if (!output.contains(header)) {
    // The block the option prints reaches a file the case reads, so a fault
    // that names the missing header carries the console that lacks it: a
    // reader of the CI log sees what the child stated without a debugger.
    const std::string tail =
        output.size() <= 2000 ? output : output.substr(output.size() - 2000);
    std::fprintf(stderr,
                 "child console of %s, status %d, exit %d:\n%s\n--- end\n",
                 kOutPath,
                 status,
                 WIFEXITED(status) ? WEXITSTATUS(status) : -1,
                 tail.c_str());
  }
  check(output.contains(header),
        "the bare word prints a tree at the level it shows, which is one, and the "
        "block of a run driven through the entry point reaches the console (FR-018, FR-025)");
  // The directory, the reference and the contributing files are the ones this
  // identity's record states, so the case reads its own run's source and no
  // constant. The loader fills all three for a production table, so the print
  // rule of FR-015 stands on the vendored path beside the planted one (FR-015,
  // T101).
  check(output.contains(header + "passes=")
            && output.contains("table=" + vendored.source.directory),
        "the header states the table this identity selects (FR-026, FR-015)");
  const std::vector<std::string> lines = linesOf(output);
  const std::vector<std::string> block = blockLinesOf(lines);
  std::size_t fileLines = 0;
  for (const std::string& entry : block) {
    fileLines +=
        entry.rfind(std::string {kBlockTag} + " file ", 0) == 0 ? 1U : 0U;
  }
  check(!vendored.source.reference.empty()
            && blockLinesOf(block).size() == block.size()
            && lineAt(block, "topdown: reference=").has_value()
            && block[*lineAt(block, "topdown: reference=")]
                == std::string {kBlockTag}
                    + " reference=" + vendored.source.reference,
        "the block of a production run names the reference its own record "
        "states (FR-015)");
  check(fileLines == vendored.source.files.size() && fileLines > 0,
        "the block of a production run carries one line per contributing file "
        "of " "its record (FR-015)");

  std::puts("the-bare-word-prints-a-tree-at-status-zero: ok");
  return 0;
}

// The scope of the run, stated once on the console ahead of the first
// measurement. The child reads the whole console of a run of two benchmarks
// through the entry point, because the claim is about order on the console: the
// line stands once for the process, after the banner the entry point prints and
// before the row of the first run that measured. The probe line names the
// reading the one reader settled, so the case holds what this host answered,
// which way it settled, and the rule that an unreadable cpu set states nothing
// (FR-042).
auto scenarioTheRunStatesTheCoreWideScopeBeforeTheFirstMeasurement() -> int
{
  const ScopeConsole withOption =
      scopeConsole(std::string {kScopeOptionVariant});
  const ScopeConsole withoutOption = scopeConsole("plain");
  check(WIFEXITED(withOption.status) && WEXITSTATUS(withOption.status) == 0,
        "the run that states its scope leaves the status at zero (FR-042)");
  check(
      WIFEXITED(withoutOption.status) && WEXITSTATUS(withoutOption.status) == 0,
      "the run that states no scope leaves the status at zero (FR-042)");
  check(!withOption.probe.empty() && !withoutOption.probe.empty(),
        "both children state the reading their own call settled (FR-042)");
  check(withOption.probe == withoutOption.probe,
        "one reading stands for the process, so the option prints no reading "
        "of " "its own (FR-042)");

  const std::vector<std::string> stated = scopeLines(withOption);
  if (withOption.probe == "none") {
    check(
        stated.empty(),
        "an unreadable cpu set or sibling list states no scope line (FR-042)");
  } else {
    check(stated.size() == 1,
          "the run states its scope once for the process, beside two "
          "benchmarks " "(FR-042)");
    check(stated.size() == 1
              && stated.front()
                  == std::string {kScopeLinePrefix} + withOption.probe,
          "the line names the way the flag settled, whole core or one thread "
          "of it (FR-042)");
    const auto banner = lineAt(withOption.lines, "speedgun-ng ");
    const auto firstRow = [&]() -> std::optional<std::size_t>
    {
      const auto found =
          std::find_if(withOption.lines.begin(),
                       withOption.lines.end(),
                       [](const std::string& line)
                       {
                         return line.rfind("bmTopdownScope", 0) == 0
                             && line.contains("iterations=");
                       });
      if (found == withOption.lines.end()) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(found - withOption.lines.begin());
    }();
    const auto scope = lineAt(withOption.lines, std::string {kScopeLinePrefix});
    check(banner.has_value() && firstRow.has_value() && scope.has_value()
              && *scope > *banner && *scope < *firstRow,
          "the scope line stands ahead of the first measurement (FR-042)");
  }

  check(blockLinesOf(withoutOption.lines).empty()
            && scopeLines(withoutOption).empty(),
        "a run without the option prints the scope line not at all (FR-042)");

  std::puts(
      "the-run-states-the-core-wide-scope-before-the-first-measurement: ok");
  return 0;
}

// Every node above the level a run asks for stands in the run, in the order the
// table spells its rows, each naming its parent, and one plan prices the whole
// set: FR-018, FR-021 and FR-027. The printed half asks for the indentation of
// a deeper level, the vendor name beside the figure, and the line a clamped ask
// states (FR-020, FR-026, FR-027).
auto scenarioTheDeeperLevelsCollectEveryNodeAboveThem() -> int
{
  if (!hostSelectsATable()) {
    skip("no vendored vendor on this host");
  }
  registerFixture(kNarrowCoreLimit);
  // The guard follows the registration on purpose: the selection reads the
  // table of the host through the counters system, and a provider laid down
  // after that read finds the catalog settled.
  if (!hostSelectsTheFixtureDirectory()) {
    skip("the identity of this host names a directory beside the fixture's");
  }
  metricBenchmark("bmTopdownLevelTwo");
  metricBenchmark("bmTopdownLevelThree");
  metricBenchmark("bmTopdownClampedLevel");
  sg::detail::expandRegistry();

  const RunAccount two = runMeasured(
      *needInstance("bmTopdownLevelTwo"),
      askedOptions(sg::test::topdownIntelRows(), 2, kRunRepetitions));
  check(two.result.topdown.has_value(), "the level-2 ask is answered");
  if (!two.result.topdown.has_value()) {
    return 1;
  }
  const sg::TopdownRecord& deep = *two.result.topdown;
  check(deep.askedLevel == 2, "the run asked level 2");
  check(deep.level == 2, "the table answers level 2");
  check(
      deep.nodes.size() == kLevelOneNodeColumns + kLevelTwoNeutralNames.size(),
      "the level-2 run holds the four level-1 nodes beside six level-2 nodes");
  for (std::size_t index = 0; index < kLevelTwoNeutralNames.size()
       && index + kLevelOneNodeColumns < deep.nodes.size();
       ++index)
  {
    const sg::TopdownNode& node = deep.nodes[kLevelOneNodeColumns + index];
    check(node.level == 2, "the node of the row stands at level 2");
    check(node.name == kLevelTwoNeutralNames[index],
          "the node carries the neutral name the fixture spells");
    check(node.vendorName == kLevelTwoVendorNames[index],
          "the node carries the vendor name of its row");
    check(!node.parent.empty(), "the level-2 node names a parent");
    if (!node.parent.empty()) {
      check(node.parent == kLevelTwoParentNames[index],
            "the node names the level-1 parent of its row");
      check(holdsName(deep.nodes, node.parent),
            "every node above the level asked for stands in the run");
    }
  }
  check(two.result.passCount == kLevelTwoPlanPassCount,
        "one plan covers the deeper set and states its pass count");

  const std::vector<std::string> twoLines =
      blockLinesOf(printedLines(two.result));
  const std::string twoHeader =
      lineOf(twoLines, std::string(kBlockTag) + " level=");
  check(twoHeader.find("level=2 ") != std::string::npos,
        "the block states the level the run answered");
  check(twoHeader.find("passes=" + std::to_string(two.result.passCount))
            != std::string::npos,
        "the block states the pass count the one plan settled");
  for (std::size_t index = 0; index < kLevelTwoNeutralNames.size(); ++index) {
    const std::string printed =
        nodeLineOf(twoLines, 2, kLevelTwoNeutralNames[index]);
    check(!printed.empty(),
          "a level-2 line stands at the indentation of its level, below its "
          "own " "parent (FR-026, FR-027)");
    check(printed.find("  " + std::string(kLevelTwoVendorNames[index]) + "  ")
              != std::string::npos,
          "the deeper line carries the vendor name of its row as a field of "
          "its " "own (FR-027)");
  }

  const RunAccount three = runMeasured(
      *needInstance("bmTopdownLevelThree"),
      askedOptions(sg::test::topdownIntelRows(), 3, kRunRepetitions));
  check(three.result.topdown.has_value(), "the level-3 ask is answered");
  if (!three.result.topdown.has_value()) {
    return 1;
  }
  const sg::TopdownRecord& below = *three.result.topdown;
  check(below.level == 3, "the table answers level 3");
  check(below.nodes.size()
            == kLevelOneNodeColumns + kLevelTwoNeutralNames.size()
                + kLevelThreeNeutralNames.size(),
        "the level-3 run holds every node of the three levels");
  std::size_t seen = 0;
  for (std::size_t index = 0; index < kLevelThreeNeutralNames.size(); ++index) {
    for (const sg::TopdownNode& node : below.nodes) {
      if (node.name != kLevelThreeNeutralNames[index]) {
        continue;
      }
      ++seen;
      check(node.level == 3, "the node of the row stands at level 3");
      check(node.vendorName == kLevelThreeVendorNames[index],
            "the node carries the vendor name of its row");
      check(node.parent == kLevelThreeParentNames[index],
            "the level-3 node names its level-2 parent");
      check(holdsName(below.nodes, kLevelThreeParentNames[index]),
            "every node above the level asked for stands in the run");
    }
  }
  check(seen == kLevelThreeNeutralNames.size(),
        "the run holds each level-3 node of the fixture once");
  check(three.result.passCount == kLevelThreePlanPassCount,
        "one plan covers the three levels and states its pass count");

  const std::vector<std::string> threeLines =
      blockLinesOf(printedLines(three.result));
  check(!nodeLineOf(threeLines, 3, kLevelThreeNeutralNames[0]).empty(),
        "a level-3 line stands at the indentation of its level");

  // Ask deeper than the table goes: the run reports the level it answered and
  // states the deepest level the table holds (FR-020).
  const RunAccount clamped = runMeasured(
      *needInstance("bmTopdownClampedLevel"),
      askedTable(
          plantedTable(kIntelDirectory, sg::test::topdownIntelShallowRows()),
          6,
          kRunRepetitions));
  check(clamped.result.topdown.has_value(), "the ask of level 6 is answered");
  if (!clamped.result.topdown.has_value()) {
    return 1;
  }
  check(clamped.result.topdown->askedLevel == 6, "the run asked level 6");
  check(clamped.result.topdown->level == kDeepestPlantedLevel,
        "the run reports the deepest level the table holds");
  check(deepestLevelOf(*clamped.result.topdown) == kDeepestPlantedLevel,
        "no node of the run stands deeper than the level it reports");
  check(holdsName(clamped.result.topdown->nodes, kLevelFourNeutralName),
        "the run holds the nodes of the level it clamped to");
  const std::vector<std::string> clampLines =
      blockLinesOf(printedLines(clamped.result));
  check(lineOf(clampLines, std::string(kBlockTag) + " level=").find("level=4 ")
            != std::string::npos,
        "the block states the level the run answered, not the level asked");
  check(!lineOf(clampLines, kClampLineTag).empty(),
        "the block states the deepest level the table holds");

  // The line belongs to a clamped ask alone: a run the table answers at the
  // level it was given states no such thing.
  check(lineOf(twoLines, kClampLineTag).empty(),
        "a run that got the level it asked states no deepest level");
  std::puts("the-deeper-levels-collect-every-node-above-them: ok");
  return 0;
}

// A run that carries a node's formula as a metric column reports one figure for
// the node and the column (SC-03), keeps the column it reported before the
// option stood even though its label is the node's vendor name (FR-023), and
// prints the row's threshold text unchanged and with no verdict beside it
// (FR-029, spec D-09).
auto scenarioTheNodeAndItsMetricColumnAnswerOneFigure() -> int
{
  if (!hostSelectsATable()) {
    skip("no vendored vendor on this host");
  }
  registerFixture(kNarrowCoreLimit);
  const std::string benchmarkName {"bmTopdownNodeColumn"};
  nodeColumnBenchmark(benchmarkName.c_str());
  sg::detail::expandRegistry();
  sg::Instance& instance = *needInstance(benchmarkName);

  const RunAccount plain = runMeasured(instance, optionsFor(kRunRepetitions));
  check(!plain.result.topdown.has_value(),
        "the run given no option records nothing");
  const std::string vendorName(kLevelOneVendorNames[0]);
  check(plain.result.metricLabels == std::vector<std::string> {vendorName},
        "the run reports the column of the node's formula before the option");
  const std::size_t column = columnAt(plain.result, vendorName)
                                 .value_or(plain.result.metricLabels.size());
  check(column < plain.result.metricLabels.size(),
        "the column stands in the row");
  check(!plain.result.rows.empty(), "the run before the option reports a row");
  if (plain.result.rows.empty()) {
    return 1;
  }
  const double plainFigure = plain.result.rows[0].metrics[column].value;

  const RunAccount asked =
      runMeasured(instance,
                  percentilesOn(askedTable(
                      plantedTable(kIntelDirectory,
                                   withThreshold(sg::test::topdownIntelRows(),
                                                 vendorName,
                                                 kPlantedThreshold)),
                      1,
                      kRunRepetitions)));
  check(asked.result.topdown.has_value(), "the level-1 ask is answered");
  if (!asked.result.topdown.has_value()) {
    return 1;
  }
  check(
      asked.result.metricLabels == plain.result.metricLabels,
      "the run keeps every metric column it reported before the option stood");
  const std::size_t kept = columnAt(asked.result, vendorName)
                               .value_or(asked.result.metricLabels.size());
  check(kept < asked.result.metricLabels.size(),
        "the column whose label is a node's vendor name stays in the row");
  check(sameBytes(asked.result.rows[0].metrics[kept].value, plainFigure),
        "the kept column carries the figure it carried before the option");
  check(!asked.result.rows.empty()
            && asked.result.rows[0].metrics.size()
                == plain.result.rows[0].metrics.size()
            && asked.result.rows[0].metricStats.size()
                == plain.result.rows[0].metricStats.size(),
        "the row keeps the number of columns it reported before the option");
  if (asked.result.rows.empty()) {
    return 1;
  }
  const std::string before = lineOf(printedLines(plain.result), benchmarkName);
  const std::string after = lineOf(printedLines(asked.result), benchmarkName);
  check(!before.empty() && !after.empty(), "each run prints its row");
  check(fieldsOf(before, vendorName) == 1
            && fieldsOf(after, vendorName) == 1,
        "the row of the asked run carries one field of the label, the node "
        "column adding no field of its own (FR-023)");
  check(!figureFieldOf(before, vendorName).empty()
            && figureFieldOf(after, vendorName)
                   == figureFieldOf(before, vendorName),
        "the kept column prints the figure it printed before the option stood "
        "(FR-023)");

  const sg::TopdownNode& node = asked.result.topdown->nodes[0];
  check(node.vendorName == vendorName, "the node of the column stands first");
  check(node.value.has_value(), "the node answers a figure");
  if (!node.value.has_value()) {
    return 1;
  }
  check(std::abs(*node.value - plainFigure) <= kFigureTolerance,
        "node and column answer one figure within SC-03's tolerance");
  check(node.threshold == kPlantedThreshold,
        "the record carries the threshold text the row spells, spaces and all");

  const std::vector<std::string> lines =
      blockLinesOf(printedLines(asked.result));
  const std::string printed = nodeLineOf(lines, 1, node.name);
  check(printed.find(figureText(*node.value)) != std::string::npos,
        "the node line states the figure");
  check(printed.find(std::string(kPlantedThreshold)) != std::string::npos,
        "the node line states the threshold text of the row");
  check(printed.find("pass=") == std::string::npos
            && printed.find("PASS") == std::string::npos
            && printed.find("FAIL") == std::string::npos,
        "the report sets no verdict by the threshold");
  std::puts("the-node-and-its-metric-column-answer-one-figure: ok");
  return 0;
}

// The five values of an AMD table answer the fixture figures the rows script
// and close the hundred percent the five are meant to fill: FR-011, FR-056 and
// SC-03. The printed half states the fifth value beside the four shared names,
// names the AMD table it read, and prints a deeper AMD level under its parent
// at its indentation (FR-026, FR-027, spec D-03).
auto scenarioTheAmdFiveValuesAnswerTheFixtureAndSumToHundred() -> int
{
  if (!hostSelectsATable()) {
    skip("no vendored vendor on this host");
  }
  registerAmdFixture(kNarrowCoreLimit);
  metricBenchmark("bmTopdownAmdLevelOne");
  metricBenchmark("bmTopdownAmdLevelTwo");
  sg::detail::expandRegistry();

  sg::detail::RunOptions levelOne = askedTable(
      plantedTable(sg::test::kZen4Directory, sg::test::topdownZen4Rows()),
      1,
      kRunRepetitions);
  levelOne.topdownIdentity = amdIdentity();
  const RunAccount first =
      runMeasured(*needInstance("bmTopdownAmdLevelOne"), levelOne);
  check(first.result.topdown.has_value(), "the AMD ask is answered");
  if (!first.result.topdown.has_value()) {
    return 1;
  }
  const sg::TopdownRecord& record = *first.result.topdown;
  check(record.directory == sg::test::kZen4Directory,
        "the record names the AMD table the run read");
  check(record.nodes.size() == kAmdLevelOneFigures.size(),
        "the AMD level-1 set holds the four shared names beside the fifth");
  double sum = 0.0;
  for (std::size_t index = 0; index < kAmdLevelOneFigures.size(); ++index) {
    if (index >= record.nodes.size()) {
      break;
    }
    const sg::TopdownNode& node = record.nodes[index];
    check(node.level == 1, "the node stands at level 1");
    check(node.parent.empty(), "a level-1 node names no parent");
    check(node.name
              == std::string("topdown.")
                  + std::string(sg::test::kAmdLevelOneNames[index]),
          "the node carries the shared neutral name of its row");
    check(node.vendorName == sg::test::kAmdLevelOneNames[index],
          "the node carries the vendor name of its row");
    check(node.value.has_value(), "the AMD node answers a figure");
    if (node.value.has_value()) {
      sum += *node.value;
      check(std::abs(*node.value - kAmdLevelOneFigures[index])
                <= kFigureTolerance,
            "the figure answers the fixture figure within SC-03's tolerance");
    }
  }
  check(std::abs(sum - 100.0) <= kSumTolerance,
        "the five values close the hundred percent FR-056 states");

  const std::vector<std::string> lines =
      blockLinesOf(printedLines(first.result));
  check(lines.size() == kAmdLevelOneFigures.size() + kAmdStatementLines + 1,
        "the AMD block states the five values, the table it read and the "
        "statement");
  check(!nodeLineOf(lines, 1, sg::test::kNeutralSmtContention).empty(),
        "the block prints the fifth value under its shared name");

  sg::detail::RunOptions levelTwo = askedTable(
      plantedTable(sg::test::kZen4Directory, sg::test::topdownZen4Rows()),
      2,
      kRunRepetitions);
  levelTwo.topdownIdentity = amdIdentity();
  const RunAccount deeper =
      runMeasured(*needInstance("bmTopdownAmdLevelTwo"), levelTwo);
  check(deeper.result.topdown.has_value(), "the AMD level-2 ask is answered");
  if (!deeper.result.topdown.has_value()) {
    return 1;
  }
  const std::string childName = std::string("topdown.amd.")
      + std::string(sg::test::kZenLevelTwoNames[0].first);
  check(holdsName(deeper.result.topdown->nodes, childName),
        "the AMD run holds the level-2 node of the table");
  const std::vector<std::string> deepLines =
      blockLinesOf(printedLines(deeper.result));
  const std::string childLine = nodeLineOf(deepLines, 2, childName);
  check(!childLine.empty(),
        "the block prints the deeper AMD level under its parent at its "
        "indentation");
  std::puts("the-amd-five-values-answer-the-fixture-and-sum-to-hundred: ok");
  return 0;
}

// The check of one claim whose fault text carries the part it names: the loop
// below walks the four identities SC-04 counts, and a fault that names none of
// them sends a reader to a debugger (FR-040).
auto checked(const bool cond, const std::string& what) -> void
{
  if (!cond) {
    std::fprintf(stderr, "HARNESS TOPDOWN RUN TEST FAIL: %s\n", what.c_str());
    std::exit(1);
  }
}

// The identity of one part SC-04 counts, handed to the print and to the run in
// place of the identity this host states. `what` names the part, and the three
// texts hold what FR-040 asks the line to name: the vendor, the family and
// model or the `MIDR`, and the reason (FR-004 to FR-007, FR-040, SC-04).
struct RefusalCase
{
  const char* what;
  CpuIdentity identity;
  std::string_view vendor;
  std::string_view identityText;
  std::string_view reason;
};

// The four identities SC-04 counts: the AMD part whose table holds no top-down
// row, the Intel hybrid part, an Arm part by its `MIDR`, and a vendor no
// vendored table carries. Each answers out of the table this build embeds and
// the identity it is handed, so no scenario below reads a cpu, a path or a
// counter of the host it runs on (SC-04, spec D-13).
[[nodiscard]] auto sc04Refusals() -> std::vector<RefusalCase>
{
  CpuIdentity unknown;
  unknown.vendor = "NoSuchVendorXXXXX";
  unknown.family = 999;
  unknown.model = 999;

  CpuIdentity arm;
  arm.midr = std::optional<std::uint32_t> {0x410fd490};

  CpuIdentity hybrid;
  hybrid.vendor = std::string {kIntelVendor};
  hybrid.family = 6;
  hybrid.model = 0x97;
  hybrid.stepping = 2;

  CpuIdentity zen3;
  zen3.vendor = std::string {kAmdVendor};
  zen3.family = 25;
  zen3.model = 15;

  return {
      RefusalCase {"a vendor no vendored table carries",
                   unknown,
                   "NoSuchVendorXXXXX",
                   "family 999 model 999",
                   "no vendored table carries this vendor"},
      RefusalCase {"an Arm part by its `MIDR`",
                   arm,
                   "Arm",
                   "0x410fd490",
                   "no Arm table is vendored"},
      RefusalCase {"an Intel hybrid part",
                   hybrid,
                   "GenuineIntel",
                   "family 6 model 151 stepping 2",
                   "two core scopes"},
      RefusalCase {"an AMD part whose table holds no top-down row",
                   zen3,
                   "AuthenticAMD",
                   "family 25 model 15",
                   "holds no row of any top-down group"},
  };
}

// The options of a run that asks one level of one planted identity and hands no
// planted table (FR-001, FR-044, spec D-20).
[[nodiscard]] auto askedIdentity(const CpuIdentity& identity,
                                 const std::uint64_t level)
    -> sg::detail::RunOptions
{
  sg::detail::RunOptions options = optionsFor(kRunRepetitions);
  options.topdownLevel = level;
  options.topdownIdentity = identity;
  return options;
}

// T076: the console half of SC-04. One message names the vendor, the identity
// and the reason, the run answers the status the same run answers without the
// option, and the record says the tree stands absent (FR-040, FR-041, SC-04).
auto scenarioAnUnsupportedHostPrintsOneLineAndKeepsTheStatus() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);
  metricBenchmark("bmTopdownNoticePlain");
  metricBenchmark("bmTopdownNoticeAsked");
  sg::detail::expandRegistry();

  for (const RefusalCase& item : sc04Refusals()) {
    const sg::detail::RunOptions options =
        askedIdentity(item.identity, kLevelOne);
    const CapturedStreams stated =
        captureStreams([&] { sg::detail::printTopdownRefusal(options); });
    const std::vector<std::string> lines = linesOf(stated.out);
    checked(lines.size() == 1 && stated.err.empty(),
            std::string {"the refusal for "} + item.what
                + " prints one message on standard output and nothing on the "
                  "error stream (FR-040)");
    const sg::counters::TopdownUnsupported refusal = refusalOf(options);
    checked(lines.front() == sg::detail::topdownRefusalLine(refusal),
            std::string {"the line printed for "} + item.what
                + " is the line the command line composes for it (FR-040)");
    checked(lines.front() == refusal.message(),
            std::string {"the line printed for "} + item.what
                + " is the library's own line, so one owner states the sentence "
                  "(FR-040, spec D-14)");
    checked(lines.front().starts_with("topdown: ")
                && lines.front().contains(item.vendor)
                && lines.front().contains(item.identityText)
                && lines.front().contains(item.reason),
            std::string {"the line printed for "} + item.what
                + " names the vendor, the identity and the reason (FR-040)");
  }

  // The first identity SC-04 counts, spelled whole: the text the answer table
  // of `contracts/topdown-selection.md` states for a table that holds no
  // top-down row (FR-040, SC-04).
  const std::vector<std::string> zen3 =
      linesOf(captureStreams(
                  [&]
                  {
                    sg::detail::printTopdownRefusal(askedIdentity(
                        sc04Refusals().back().identity, kLevelOne));
                  })
                  .out);
  check(zen3.front()
            == "topdown: AuthenticAMD family 25 model 15: the selected table "
               "'arch/x86/amdzen3/' holds no row of any top-down group",
        "the console line of the AMD part with no top-down row stands as the "
        "contract spells it (FR-040, SC-04)");

  // The run beside the line: the option stands, the identity names a part with
  // no top-down row, and the run stays itself (FR-041).
  const CapturedRun refusal = runMeasuredQuietly(
      *needInstance("bmTopdownNoticeAsked"),
      askedIdentity(sc04Refusals().back().identity, kLevelOne));
  const RunAccount plain = runMeasured(*needInstance("bmTopdownNoticePlain"),
                                       optionsFor(kRunRepetitions));
  check(refusal.out.empty() && refusal.err.empty(),
        "the run prints nothing for its own refusal, because the one message "
        "line stands in the command line (FR-040)");
  check(refusal.run.result.outcome == plain.result.outcome
            && refusal.run.result.reason == plain.result.reason,
        "the run whose refusal printed a line answers the status the same run "
        "answers without the option (FR-041)");
  check(refusal.run.result.topdown.has_value()
            && !refusal.run.result.topdown->supported,
        "the record says the tree stands absent (FR-033, FR-040)");
  check(refusal.run.reads == plain.reads
            && refusal.run.result.metricLabels == plain.result.metricLabels,
        "the refusal and its line cost the run no column and no sampling "
        "action " "of its own (FR-040, FR-009)");

  // Two arms print no line: a selection that answers a tree, and a run that
  // names no option at all (FR-040, FR-035).
  const CapturedStreams tree = captureStreams(
      [&]
      {
        sg::detail::printTopdownRefusal(askedOptions(
            sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));
      });
  check(tree.out.empty() && tree.err.empty(),
        "a run whose selection answers a tree prints no refusal line (FR-040)");
  const CapturedStreams unasked = captureStreams(
      [&] { sg::detail::printTopdownRefusal(optionsFor(kRunRepetitions)); });
  check(unasked.out.empty() && unasked.err.empty(),
        "a run that names no option prints no refusal line, and its bytes "
        "stand " "where they stood before this feature (FR-040, FR-035)");

  std::puts("an-unsupported-host-prints-one-line-and-keeps-the-status: ok");
  return 0;
}

// The row of the block one refused row prints: the tag, the two spaces per
// level below one of the level the block shows, the word that names the row a
// refused one, and the three fields the record carries for it unchanged
// (FR-016, FR-031).
[[nodiscard]] auto refusedLine(const std::uint64_t levelShown,
                               const sg::TopdownRefusal& row) -> std::string
{
  const std::string indent(levelShown > 1 ? std::size_t {2}
                                   * static_cast<std::size_t>(levelShown - 1)
                                          : 0,
                           ' ');
  return std::string {kBlockTag} + "  " + indent + "refused  " + row.name + "  "
      + row.refusal + "  " + row.reason;
}

// T077: the block lists each refused row once at the level it shows, with its
// UPPER SNAKE kind and its reason unchanged, and prints no node for it (FR-016,
// FR-031).
auto scenarioRefusedRowsPrintAtTheLevelShown() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFixture(kNarrowCoreLimit);
  metricBenchmark("bmTopdownRefusedRows");
  sg::detail::expandRegistry();

  const RunAccount deep = runMeasured(
      *needInstance("bmTopdownRefusedRows"),
      askedOptions(sg::test::topdownIntelRows(), kLevelTwo, kRunRepetitions));
  check(deep.result.topdown.has_value() && deep.result.topdown->supported,
        "the planted table answers a tree at the deeper level (FR-018)");
  const sg::TopdownRecord& record = *deep.result.topdown;
  check(record.level == kLevelTwo,
        "the record shows the level two the run asked for (FR-020)");
  check(record.refused.size() == 8,
        "the level-2 run carries the eight refused rows of its own level and of "
        "the level above it, and the pair of level 3 stands out of the record "
        "(FR-016)");

  const std::vector<std::string> block =
      blockLinesOf(printedLines(deep.result));
  check(block.size() == std::size_t {1} + record.nodes.size()
                            + record.refused.size(),
        "the block prints its header, one line per node and one line per refused "
        "row, so a refused row builds no node of its own (FR-016, FR-031)");
  for (const sg::TopdownRefusal& row : record.refused) {
    check(std::count(block.begin(), block.end(), refusedLine(record.level, row))
              == 1,
          "each refused row prints once at the level the block shows, with its "
          "kind word and its reason unchanged (FR-016, FR-031)");
  }
  std::size_t stated = 0;
  for (const std::string& line : block) {
    stated += line.contains("refused  ") ? std::size_t {1} : std::size_t {0};
  }
  check(stated == record.refused.size(),
        "one refused line stands per refused row of the record, and no node "
        "line " "carries the word (FR-031)");
  check(block[std::size_t {1} + record.nodes.size()]
            == refusedLine(kLevelTwo, record.refused.front()),
        "the first refused row of the block stands indented at the level two "
        "the " "block shows (FR-031)");

  // The same two rows under a block that shows level one: the indent belongs to
  // the level the block shows, so the same row prints with none beyond the tag
  // (FR-031).
  const sg::TopdownRefusal& first = record.refused.front();
  const sg::TopdownRefusal& last = record.refused.back();
  sg::BenchmarkResult shallow;
  shallow.name = "bmTopdownRefusedRowsLevelOne";
  shallow.passCount = kOnePlanPassCount;
  sg::TopdownRecord shown;
  shown.supported = true;
  shown.askedLevel = kLevelOne;
  shown.level = kLevelOne;
  shown.directory = std::string {kIntelDirectory};
  shown.nodes.push_back(sg::TopdownNode {
      .name = std::string {sg::test::kNeutralBackendBound},
      .vendorName = "tma_backend_bound",
      .parent = {},
      .level = kLevelOne,
      .value = 50.0,
      .availability = sg::counters::Availability::COUNTABLE,
      .threshold = {},
  });
  shown.refused.push_back(first);
  shown.refused.push_back(last);
  shallow.topdown = std::move(shown);

  const std::vector<std::string> shownBlock =
      blockLinesOf(printedLines(shallow));
  check(shownBlock.size() == 4,
        "the block that shows level one carries its header, its one node and "
        "one " "line for each of the two refused rows (FR-031)");
  check(shownBlock[1]
            == std::string {kBlockTag} + "  "
                + std::string {sg::test::kNeutralBackendBound}
                + "  tma_backend_bound  " + figureText(50.0),
        "the node line keeps its bytes beside the refused rows (FR-027)");
  check(shownBlock[2] == refusedLine(kLevelOne, first)
            && shownBlock[3] == refusedLine(kLevelOne, last),
        "a refused row prints with no indent beyond the tag where the block "
        "shows level one, and the two rows keep their own order (FR-031)");

  std::puts("refused-rows-print-at-the-level-shown: ok");
  return 0;
}

// The refusal PC-17 names. Two leaves of the planted table stand frozen, so
// three of the four level-1 formulas divide by a denominator that measured no
// delta: `tma_frontend_bound` takes zero over zero, `tma_retiring` takes a
// count over zero, and `tma_backend_bound` carries both of them. The arithmetic
// of a fold answers a figure that is no number for those three, and the fourth
// row keeps the figure its two moving leaves drove. Neither surface that
// carries a figure to a reader may spell it: the block prints the two names of
// such a node and the words that stand where a figure stands, and the record
// carries no figure for it, so the console and the record refuse the same nodes
// (FR-030, specs/042-topdown PC-17).
auto scenarioAFigureThatIsNoNumberReachesNeitherSurface() -> int
{
  if (!hostSelectsATable()) {
    skip("this host states no vendor this build vendors a table for");
  }
  registerFrozenLeafFixture();
  bareBenchmark("bmTopdownFrozenDenominator");
  sg::detail::expandRegistry();

  const RunAccount run = runMeasured(
      *needInstance("bmTopdownFrozenDenominator"),
      askedOptions(sg::test::topdownIntelRows(), kLevelOne, kRunRepetitions));
  check(run.result.topdown.has_value(), "the level-1 ask is answered");
  if (!run.result.topdown.has_value()) {
    return 1;
  }
  const sg::TopdownRecord& record = *run.result.topdown;
  const auto named = [&record](const std::string_view name)
  {
    for (const sg::TopdownNode& node : record.nodes) {
      if (node.name == name) {
        return &node;
      }
    }
    return static_cast<const sg::TopdownNode*>(nullptr);
  };

  // The three nodes the frozen leaves starve, and the one node whose two leaves
  // both moved. Which formula divides by which leaf is the fixture's own text,
  // stated at `topdownIntelLevelOneRows`, and the case names the nodes it
  // built.
  const std::array<std::string_view, 3> starved {{
      "topdown.frontend_bound",
      "topdown.backend_bound",
      "topdown.retiring",
  }};
  for (const std::string_view name : starved) {
    const sg::TopdownNode* node = named(name);
    check(node != nullptr, "the record carries the node of a starved formula");
    if (node == nullptr) {
      return 1;
    }
    check(!node->value.has_value(),
          "the record carries no figure for the node whose denominator "
          "measured " "no delta");
    // `ABSENT` is the state the library already gives a denominator that is
    // not there: `hunt.cpp:273` states it for a column no window gave a rate,
    // and a node whose denominator measured no delta is that condition one
    // level up.
    check(node->availability == sg::counters::Availability::ABSENT,
          "the record answers for the node the state a missing denominator "
          "answers");
  }
  const sg::TopdownNode* live = named("topdown.bad_speculation");
  check(live != nullptr && live->value.has_value()
            && live->availability == sg::counters::Availability::COUNTABLE,
        "the node whose two leaves moved keeps its figure and its state");

  for (const sg::TopdownNode& node : record.nodes) {
    check(!node.value.has_value() || std::isfinite(*node.value),
          "no node of the record carries a figure that is no number");
    // The four shares of the level divide one hundred percent between them, so
    // a figure past it is no measurement. A writer that turns a non-finite
    // figure into a finite magnitude of this size is the regression this
    // assertion stands against, and a case that only forbade the text `nan`
    // would read it as a figure.
    check(!node.value.has_value() || *node.value <= kShareCeiling,
          "no node of the record carries a figure past the hundred percent the "
          "level divides");
  }

  const std::vector<std::string> lines = blockLinesOf(printedLines(run.result));
  check(!lines.empty(), "the run prints its block");
  for (const std::string& line : lines) {
    check(spellsNoNumber(line),
          "no line of the block spells a figure that is no number");
  }
  for (const std::string_view name : starved) {
    const std::string line = nodeLineOf(lines, kLevelOne, name);
    check(!line.empty(), "the block prints the line of the starved node");
    check(line.find("not available") != std::string::npos,
          "the line of the starved node prints the words that stand where a "
          "figure stands");
  }
  const std::string liveLine =
      nodeLineOf(lines, kLevelOne, "topdown.bad_speculation");
  check(liveLine.find("not available") == std::string::npos
            && liveLine.find('%') != std::string::npos,
        "the line of the node that answered prints its figure");

  // The print makes the same test the assembly of the node makes. The record of
  // the copy below reaches the print by another route, with the figure of the
  // starved nodes put back by hand and the countable state left on them, which
  // is what a record read back from an older document looks like. The block
  // still prints no figure for them (PC-17).
  auto handBuilt = run.result;
  for (sg::TopdownNode& node : handBuilt.topdown->nodes) {
    if (std::find(starved.begin(), starved.end(), node.name) != starved.end()) {
      node.value = std::numeric_limits<double>::quiet_NaN();
      node.availability = sg::counters::Availability::COUNTABLE;
    }
  }
  const std::vector<std::string> handLines =
      blockLinesOf(printedLines(handBuilt));
  check(handLines.size() == lines.size(),
        "the hand built record prints as many lines as the run printed");
  for (const std::string& line : handLines) {
    check(spellsNoNumber(line),
          "no line of the hand built block spells a figure that is no number");
  }
  for (const std::string_view name : starved) {
    check(nodeLineOf(handLines, kLevelOne, name)
              .find("not available")
          != std::string::npos,
          "the hand built block prints the words that stand where a figure "
          "stands");
  }

  std::puts("a-figure-that-is-no-number-reaches-neither-surface: ok");
  return 0;
}

struct Scenario
{
  const char* name;
  int (*run)();
};

constexpr std::array<Scenario, 24> kScenarios {{
    Scenario {"nodes-share-one-plan-with-the-metric-columns",
              &scenarioNodesShareOnePlanWithTheMetricColumns},
    Scenario {"the-metric-columns-keep-their-bytes",
              &scenarioTheMetricColumnsKeepTheirBytes},
    Scenario {"a-refused-row-collects-no-column",
              &scenarioARefusedRowCollectsNoColumn},
    Scenario {"a-new-source-reads-its-limit-once",
              &scenarioANewSourceReadsItsLimitOnce},
    Scenario {"the-node-figure-stands-in-the-record",
              &scenarioTheNodeFigureStandsInTheRecord},
    Scenario {"the-strip-keeps-every-metric-view-aligned",
              &scenarioTheStripKeepsEveryMetricViewAligned},
    Scenario {"the-strip-changes-no-time-column",
              &scenarioTheStripChangesNoTimeColumn},
    Scenario {"an-unsupported-host-collects-nothing-and-stays-itself",
              &scenarioAnUnsupportedHostCollectsNothingAndStaysItself},
    Scenario {"the-block-stands-after-the-result-tables",
              &scenarioTheBlockStandsAfterTheResultTables},
    Scenario {"the-block-states-the-level-the-passes-and-the-table",
              &scenarioTheBlockStatesTheLevelThePassesAndTheTable},
    Scenario {"the-four-level-one-names-print-with-their-vendor-names",
              &scenarioTheFourLevelOneNamesPrintWithTheirVendorNames},
    Scenario {"each-figure-equals-what-the-fixture-scripts",
              &scenarioEachFigureEqualsWhatTheFixtureScripts},
    Scenario {"the-amd-block-prints-the-fifth-value-and-the-statement",
              &scenarioTheAmdBlockPrintsTheFifthValueAndTheStatement},
    Scenario {"a-run-with-no-metric-column-still-states-its-passes",
              &scenarioARunWithNoMetricColumnStillStatesItsPasses},
    Scenario {"the-bare-word-prints-a-tree-at-status-zero",
              &scenarioTheBareWordPrintsATreeAtStatusZero},
    Scenario {"the-deeper-tree-prints-its-indentation-and-its-gap",
              &scenarioTheDeeperTreePrintsItsIndentationAndItsGap},
    Scenario {"an-unsupported-record-prints-no-block",
              &scenarioAnUnsupportedRecordPrintsNoBlock},
    Scenario {"the-run-states-the-core-wide-scope-before-the-first-measurement",
              &scenarioTheRunStatesTheCoreWideScopeBeforeTheFirstMeasurement},
    Scenario {"the-deeper-levels-collect-every-node-above-them",
              &scenarioTheDeeperLevelsCollectEveryNodeAboveThem},
    Scenario {"the-node-and-its-metric-column-answer-one-figure",
              &scenarioTheNodeAndItsMetricColumnAnswerOneFigure},
    Scenario {"the-amd-five-values-answer-the-fixture-and-sum-to-hundred",
              &scenarioTheAmdFiveValuesAnswerTheFixtureAndSumToHundred},
    Scenario {"an-unsupported-host-prints-one-line-and-keeps-the-status",
              &scenarioAnUnsupportedHostPrintsOneLineAndKeepsTheStatus},
    Scenario {"refused-rows-print-at-the-level-shown",
              &scenarioRefusedRowsPrintAtTheLevelShown},
    Scenario {"a-figure-that-is-no-number-reaches-neither-surface",
              &scenarioAFigureThatIsNoNumberReachesNeitherSurface},
}};

}  // namespace

auto main(int argc, char** argv) -> int
{
  // The command line of the child is built out of `std::string` and `const
  // char*` only, the spelling `harness_pass_collect_test.cpp` uses: a
  // `std::string` built from a `std::string_view` whose length the optimizer
  // cannot bound trips GCC's `-Wrestrict` on the copy inside that constructor.
  const std::string self =
      argc > 0 && argv[0] != nullptr ? argv[0] : "harness_topdown_run_test";
  const std::string requested =
      argc > 1 && argv[1] != nullptr ? argv[1] : std::string();
  selfPath = self.c_str();
  for (const Scenario& scenario : kScenarios) {
    if (requested == scenario.name) {
      return scenario.run();
    }
  }
  if (requested == kEntryPointMode) {
    return runEntryPointChild();
  }
  if (requested == kScopeChildMode) {
    return runScopeChild(argc > 2 && argv[2] != nullptr ? argv[2]
                                                        : std::string());
  }
  if (!requested.empty()) {
    std::fprintf(stderr,
                 "harness_topdown_run_test: unknown scenario '%s'\n",
                 requested.c_str());
    return 2;
  }

  for (const Scenario& scenario : kScenarios) {
    const std::string command = "\"" + self + "\" " + scenario.name;
    const int status = std::system(command.c_str());
    if (WIFEXITED(status) && WEXITSTATUS(status) == kSkipStatus) {
      std::fprintf(stderr,
                   "harness_topdown_run_test: the scenario '%s' skipped\n",
                   scenario.name);
      return kSkipStatus;
    }
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scenario of the in-main list passes");
  }

  std::puts("harness_topdown_run_test: ok");
  return 0;
}
