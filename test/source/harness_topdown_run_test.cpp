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
  provider.setPoints(
      kCoreObject, sg::test::kCycles, {0}, sg::test::kCyclesDelta);
  provider.setPoints(
      kCoreObject, sg::test::kInstructions, {0}, sg::test::kInstructionsDelta);
  provider.setPoints(
      kCoreObject, sg::test::kRefCycles, {0}, sg::test::kRefCyclesDelta);
  provider.setPoints(
      kCoreObject, sg::test::kL2Requests, {0}, sg::test::kL2Delta);
  provider.setPoints(kCoreObject, sg::test::kTime, {0}, sg::test::kTimeDelta);
  provider.setPoints(
      kMemoryObject, sg::test::kDataReads, {0}, sg::test::kDataReadsDelta);
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

// Register one instance of the plain body with no column of its own: the tree
// is the only thing such a run collects.
auto bareBenchmark(const char* name) -> void
{
  static_cast<void>(sg::registerBenchmark(&plainBody, name));
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

// The detail a mapping-file miss states: the header defines the field as the
// directory the selection read, and a miss read none. The sentence FR-040 names
// belongs to `TopdownUnsupported::message()`, which T079 publishes, so no
// caller composes it and a mid-feature record carries this detail alone
// (FR-040, FR-041).
constexpr std::string_view kRefusalDetail {};

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

// The two files one redirected call writes: the names carry this unit, the way
// `harness_aggregates_only_test.cpp` names the one file it redirects. Running
// the binary from the repository root leaves them there, and ctest runs it from
// the build's test directory.
constexpr const char* kOutPath = "harness_topdown_run_out.txt";
constexpr const char* kErrPath = "harness_topdown_run_err.txt";

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

  check(record.reason == kRefusalDetail,
        "the reason holds the detail the planted refusal states, and the "
        "sentence of a refusal arrives with the library's own line (FR-040, FR-041)");

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
  if (vendored.source.directory.empty()) {
    skip("the vendored mapping file names no table for this identity");
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

struct Scenario
{
  const char* name;
  int (*run)();
};

constexpr std::array<Scenario, 18> kScenarios {{
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
