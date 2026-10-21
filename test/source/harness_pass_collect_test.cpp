// ============================================================================
// The pass collection runs (specs/024-multipass-collection T022 to T025 and
// T032 to T034; US1, US2; FR-008 to FR-016, FR-023 to FR-025, FR-034; SC-01,
// SC-09, SC-10; research D-06, D-09, D-10).
//
// Every scenario runs the real collection path over the scripted pass fixture
// of `detail/pass_fixture.hpp`: the leaves answer from a frozen script, so no
// hardware is read and no scenario sleeps. The runs are driven through the
// runner's internal seam, the way `harness_percentile_test.cpp` and
// `harness_instance_name_test.cpp` drive it, and every claim reads either a
// field of the result value or a count of the fake's sampling actions, read
// after the run joined. No clock stands in this file, as
// `test/time_source_gate.sh` bars one from a harness test.
//
// The two percentile arms read the report surfaces beside the result value:
// `sg::detail::printResult` behind the stdout capture of
// `harness_pass_report_test.cpp`, `sg::detail::writeReport` read back through
// `sg::loadReport`, and the chunk columns the run retained behind its rows.
//
// Each scenario scripts its own provider, and the counters system takes one
// provider per path per process, so each scenario runs in its own process:
// the parent below walks the scenario list and re-executes this binary once
// per entry, the pattern of `harness_pause_test.cpp`. The list is what CTest
// runs, so a scenario absent from it never runs.
//
// Beside the splits that run to their last pass, the file holds the ones
// that end early and the two that widen the merged row: the calibration
// ahead of an adaptive split's pass loop answers a failing, a skipping and a
// signaled body each with its own stop, the interrupt read between a pass's
// repetitions ends the loop with two passes measured and no merge, a
// percentile run and a body that stores a user counter each stand a row
// wider than the plan the merge reads, and the three-pass arms read the
// disclosure a loop that leaves early publishes: the passes whose folds
// stand, with no spread and no pass tag
// (FR-016, FR-031, FR-032, FR-014, FR-017; specs/021 FR-016).
// ============================================================================

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "detail/pass_fixture.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/execution_context.hpp"

namespace
{

// The iteration count every value arm pins: one timed window then holds a
// known count of sampling actions and the per-iteration rule divides by a
// known number.
constexpr std::uint64_t kValueIterations = 4;

// The two repetition counts whose runs differ by exactly one repetition of
// every pass. The difference of their action counts is the window cost of
// that one repetition alone: the calibration a compiled plan runs for itself
// is the same in both runs, so it cancels.
constexpr std::uint64_t kCountHighRepetitions = 3;
constexpr std::uint64_t kCountLowRepetitions = 2;

// The scale pair of the read-count arm (SC-09), the pair
// `harness_capture_test.cpp` takes for the audit point's own run.
constexpr std::uint64_t kScaleHighRepetitions = 10000;
constexpr std::uint64_t kScaleLowRepetitions = 9999;

// The sampling actions of one timed window that folds nothing: the two
// endpoints, the audit point's per-iteration count for a metric run of one
// iteration (FR-001, `kAuditPointReadsPerIteration`).
constexpr std::uint64_t kPlainWindowReads =
    sg::test::kAuditPointReadsPerIteration;

// The sampling actions of one timed window that folds one pause pair: the two
// endpoints beside the two points of the interval.
constexpr std::uint64_t kPausedWindowReads = 4;

// The sampling steps one leaf of a window advances net of what the fold
// subtracts: one step where the window folds nothing, two where it folds one
// interval out of the three the four actions span.
constexpr double kPlainSteps = 1.0;
constexpr double kPausedSteps = 2.0;

// The pass counts the fixture's column shapes owe the first-fit planner at
// the scripted limit of 4 (FR-004, data-model E-03 rule 2 and rule 4).
constexpr std::uint64_t kTwoPassCount = 2;
constexpr std::uint64_t kThreePassCount = 3;

// The contexts of the threaded arm (FR-013, FR-034, SC-10).
constexpr int kThreadCount = 2;

// The running ratio the scripted pair discloses, and the steps of its two
// leaves: the fold of a declared leaf multiplies `running/enabled` of the
// pair into its disclosure (feature 006 FR-019, FR-041).
constexpr std::uint64_t kEnabledStep = 200;
constexpr std::uint64_t kRunningStep = 100;
constexpr double kScriptedRatio =
    static_cast<double>(kRunningStep) / static_cast<double>(kEnabledStep);

// The names the pair links by: a declared leaf of one object takes that
// object's `<object>/enabled` and `<object>/running` slots of the same plan.
constexpr std::string_view kRatioCarrierLeaf = "pass_w0";
constexpr std::string_view kEnabledLeaf = "enabled";
constexpr std::string_view kRunningLeaf = "running";
constexpr std::string_view kRatioPairUnit = "none";

// The fixture leaf names the over-limit column borrows: the wide column of
// index 4 steps `(4 + 1) * kPassEventStep` per sampling action.
constexpr std::size_t kWideColumnIndex = 4;

// The attempt log: one entry per timed attempt, written by the benchmark
// body at its first iteration, read after the run joined. The table stands
// fixed so a body adds nothing to the heap inside a timed window.
struct AttemptRecord
{
  // The run-wide sequence of this attempt, from the arrival counter.
  std::uint64_t arrival = 0;
  // The count of attempts this context has run: with one repetition and the
  // pass loop, ordinal 1 is pass 0 and ordinal 2 is pass 1.
  std::uint64_t ordinal = 0;
  // The provider actions the body saw when it opened its first iteration.
  std::uint64_t reads = 0;
  int threadIndex = -1;
  int threads = 0;
  // The identity of the live context this attempt ran on.
  std::uint64_t contextSerial = 0;
  bool hasContext = false;
};

constexpr std::size_t kMaxAttempts = 16;
constexpr std::size_t kMaxContexts = 4;

std::array<AttemptRecord, kMaxAttempts> attempts {};
std::atomic<std::uint64_t> arrivals {0};
std::array<std::atomic<std::uint64_t>, kMaxContexts> contextAttempts {};

sg::counters::FakeProvider* fake = nullptr;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PASS COLLECT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto closeTo(const double value, const double expected) -> bool
{
  return std::abs(value - expected) <= 0.001 * (std::abs(expected) + 1.0);
}

// Lay the enabled/running pair of the fixture's cpu object on a provider, so
// a column that carries one of the object's declared leaves discloses the
// scripted ratio `kScriptedRatio` instead of the full rate. The carrier leaf
// keeps the sequence the fixture scripted for it: `addCounter` writes the
// entry's metadata and `setPoints` owns the sequence apart from it.
auto scriptRatioPair(sg::counters::FakeProvider& provider) -> void
{
  provider.addCounter(sg::test::kPassCpuObject,
                      kEnabledLeaf,
                      kRatioPairUnit,
                      "scripted enabled time of the cpu object",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::SYSCALL,
                      false);
  provider.addCounter(sg::test::kPassCpuObject,
                      kRunningLeaf,
                      kRatioPairUnit,
                      "scripted running time of the cpu object",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::SYSCALL,
                      false);
  // The scripted tail steps by the leaf's own delta: the fifth argument of
  // `setPoints` is the seed of the pseudo-random tail, so it stays unset.
  provider.setPoints(sg::test::kPassCpuObject, kEnabledLeaf, {0}, kEnabledStep);
  provider.setPoints(sg::test::kPassCpuObject, kRunningLeaf, {0}, kRunningStep);
  provider.addCounter(sg::test::kPassCpuObject,
                      kRatioCarrierLeaf,
                      kRatioPairUnit,
                      "fixture leaf carrying the scripted ratio pair",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::SYSCALL,
                      true);
}

// The scripted pass fixture with its limit, keeping the provider so a
// scenario can read its action counts. The counters system stages the
// provider's whole catalog at registration (`system.cpp:216`), so every
// scripted fact, the ratio pair of the over-limit arm included, precedes the
// registration: a leaf declared afterwards is invisible to the tree. No leaf
// may be scripted after the first `object` call either, which is why every
// scenario scripts through this seam before it attaches a metric.
auto registerFixtureProvider(const std::size_t limit,
                             const bool withRatioPair = false)
    -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::scriptPassFixture(*provider, limit);
  if (withRatioPair) {
    scriptRatioPair(*provider);
  }
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(), "the pass fixture registers on the local system");
  fake = scripted;
  return *scripted;
}

// The sum of one column's leaves, the shape `data-model.md` E-01 gives a
// metric column: one same-dimension core over the PMU events it names.
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

auto attachColumn(sg::BenchmarkHandle& handle,
                  const sg::test::PassColumn& column) -> void
{
  handle.addMetric(sumLeaves(column.object, column.events), column.label);
}

// Attach the fixture columns the scenario asks for, in the order listed: the
// request order the planner fills in is the attach order (FR-001).
auto attachColumns(sg::BenchmarkHandle& handle,
                   const std::vector<std::size_t>& indexes) -> void
{
  for (const std::size_t index : indexes) {
    attachColumn(handle, sg::test::passColumns()[index]);
  }
}

auto optionsFor(const std::uint64_t iterations,
                const std::uint64_t repetitions) -> sg::detail::RunOptions
{
  sg::detail::RunOptions options;
  options.fixedIterations = iterations;
  options.repetitions = repetitions;
  return options;
}

auto needInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  check(false, "the expansion produced the named instance");
  return nullptr;
}

// One run, its result, and the provider actions it cost (FR-001, D-10).
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

// One run beside the chunk columns it retained: the ledger of row `k` stands
// at `rowRaw[k]` and the ledger of context `j` of that row at
// `contextRaw[k][j]`, the pairing the document writes beside the row it
// printed (specs/023 E-01, FR-029). A split run remaps the row's own ledger to
// the pass that measured each column, so the ledger is a surface of its own
// for a merged figure.
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

// The file a scenario's printed report is captured to, and the document one
// scenario writes: the scratch files of `harness_pass_report_test.cpp` and
// `harness_json_report_test.cpp`, spelled for this binary.
constexpr const char* kScratchPath = "harness_pass_collect_out.txt";
constexpr const char* kReportPath = "harness_pass_collect_report.json";

// `call` prints to the process stdout, so the capture redirects that stream to
// a scratch file and restores it: the verdict lines of this file stay on the
// real stdout. The pattern of `captureThrough` in `harness_pause_test.cpp` and
// `harness_pass_report_test.cpp`.
template<class Call>
auto captureThrough(const std::string& path, Call&& call) -> void
{
  std::FILE* saved = nullptr;
  const int savedFd = dup(STDOUT_FILENO);
  if (savedFd >= 0) {
    saved = fdopen(savedFd, "w");
  }
  check(saved != nullptr, "the capture kept the real stdout");

  std::FILE* replacement = std::fopen(path.c_str(), "we");
  check(replacement != nullptr, "the capture opened its scratch file");
  const int replacementFd = fileno(replacement);
  check(replacementFd >= 0, "the capture file carries a descriptor");
  const bool redirected = dup2(replacementFd, STDOUT_FILENO) == STDOUT_FILENO;
  check(redirected, "the capture redirected stdout");
  if (std::setvbuf(stdout, nullptr, _IONBF, 0) != 0) {
    fail("the capture could not unbuffer the redirected stdout");
  }
  std::fflush(stdout);

  call();

  std::fflush(stdout);
  const bool restored = dup2(savedFd, STDOUT_FILENO) == STDOUT_FILENO;
  check(restored, "the capture restored stdout");
  if (std::setvbuf(stdout, nullptr, _IOLBF, BUFSIZ) != 0) {
    fail("the restored stdout could not set its buffering mode");
  }
  std::fclose(saved);
  std::fclose(replacement);
}

auto readAll(const std::string& path) -> std::string
{
  std::ifstream file(path);
  check(static_cast<bool>(file), "the captured file opens for reading");
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The percentile lines of a printed report, in the order they stand: a row's
// own lines come first and one per slot of the row, so line `slot` of a
// single-row report answers slot `slot` of the row (SC-001).
auto percentileLines(const std::string& report) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::istringstream stream(report);
  for (std::string line; std::getline(stream, line);) {
    if (line.find("  percentile") != std::string::npos) {
      lines.push_back(line);
    }
  }
  return lines;
}

// The figure one point of a printed percentile line carries, or nothing where
// the line answers `not available` in its place. The read stops at the first
// byte the number does not carry, so a unit suffix changes nothing.
auto printedPoint(const std::string& line,
                  const std::string_view token) -> std::optional<double>
{
  const auto at = line.find(token);
  check(at != std::string::npos, "the printed line carries the point's token");
  const char* text = line.c_str() + at + token.size();
  char* end = nullptr;
  const double value = std::strtod(text, &end);
  if (end == text) {
    return std::nullopt;
  }
  return value;
}

// The value a fixture column owes a run: every leaf of the column steps
// `(index + 1) * kPassEventStep` per sampling action, the column sums its
// leaves, and the per-iteration rule divides the net fold by the iteration
// count of the row.
auto expectedColumnValue(const std::size_t index,
                         const double steps,
                         const std::uint64_t iterations) -> double
{
  const std::size_t leaves = sg::test::passColumns()[index].events.size();
  return static_cast<double>(leaves)
      * static_cast<double>((index + 1) * sg::test::kPassEventStep) * steps
      / static_cast<double>(iterations);
}

// The real-time column a run owes: the monotonic leaf of the scripted clock
// steps `kPassTimeStep` per sampling action, net of what the fold subtracts.
auto expectedTimePerIteration(const double steps,
                              const std::uint64_t iterations) -> double
{
  return static_cast<double>(sg::test::kPassTimeStep) * steps
      / static_cast<double>(iterations);
}

// Every column of the row folds a countable value, reports no scaling and
// reports the running ratio of 1.0, and every column carries the value its
// own pass folded (FR-009, FR-010, FR-014, SC-01). `indexes` names the
// positions to compare against the fixture and `columns` the count the whole
// row owes, which a run with a column of its own states apart.
auto checkColumnsAtFullRatio(const sg::ResultRow& row,
                             const std::vector<std::size_t>& indexes,
                             const double steps,
                             const std::uint64_t iterations,
                             const std::size_t columns = 0) -> void
{
  check(row.metrics.size() == (columns == 0 ? indexes.size() : columns),
        "the merged row carries one value per attached metric column (FR-009)");
  for (std::size_t column = 0; column < indexes.size(); ++column) {
    const auto& value = row.metrics[column];
    check(value.availability == sg::counters::Availability::COUNTABLE,
          "every metric column folds a countable value (FR-009)");
    check(closeTo(value.runningRatio, 1.0),
          "every metric column reports its value at running ratio 1.0 (SC-01)");
    check(!value.scaled,
          "a column that shares its source reports no scaling (FR-009)");
    check(closeTo(value.value, expectedColumnValue(indexes[column],
                                                   steps,
                                                   iterations)),
          "the merged row carries the value of the pass that measured the "
          "column (FR-014)");
  }
}

auto resetAttempts() -> void
{
  for (auto& record : attempts) {
    record = AttemptRecord {};
  }
  arrivals.store(0);
  for (auto& count : contextAttempts) {
    count.store(0);
  }
}

// The body of the plain runs: nothing but the loop, so a window holds the two
// endpoint samples and nothing else.
auto bmPlain(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// The body of the pause arm: one `pauseTiming` and `resumeTiming` pair,
// folded inside the window of every pass the run opens.
auto bmPaused(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.pauseTiming();
      sg::doNotOptimize(index);
      state.resumeTiming();
    }
    sg::doNotOptimize(index);
  }
}

// The body of the window-lifetime arm: the provider action count as the first
// iteration of each attempt opens, the count the window's opening read already
// stands in.
auto bmAttemptLog(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      const std::uint64_t arrival = arrivals.fetch_add(1);
      check(arrival < kMaxAttempts, "the attempt log holds every attempt");
      AttemptRecord& record = attempts[arrival];
      record.arrival = arrival;
      record.ordinal = arrival + 1;
      record.reads = fake->readActions();
      record.threadIndex = state.threadIndex();
      record.threads = state.threads();
    }
    sg::doNotOptimize(index);
  }
}

// The body of the threaded arm: the arrival sequence, the count of attempts
// this context has run, and the identity of the live context it ran on.
auto bmContextLog(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      const std::uint64_t arrival = arrivals.fetch_add(1);
      check(arrival < kMaxAttempts, "the attempt log holds every attempt");
      const int context = state.threadIndex();
      check(context >= 0 && static_cast<std::size_t>(context) < kMaxContexts,
            "the context index of a threaded attempt stands in the log");
      AttemptRecord& record = attempts[arrival];
      record.arrival = arrival;
      record.reads = fake->readActions();
      record.threadIndex = context;
      record.threads = state.threads();
      record.ordinal =
          contextAttempts[static_cast<std::size_t>(context)].fetch_add(1) + 1;
      const auto identity = sg::currentContext();
      record.hasContext = identity.has_value();
      record.contextSerial = identity.has_value() ? identity->serial : 0;
    }
    sg::doNotOptimize(index);
  }
}

// A body that never enters the timed loop: the record build of any sample
// that ran it carries the FR-017 failure, so the sample of a calibration is
// where that failure reaches the collection.
auto bmNeverLoops(sg::State& /*state*/) -> void {}

// A body that skips before its loop takes no sample: the loop walks the
// empty cursor the skip left behind, and the record of the run carries the
// skip's own reason (FR-031).
auto bmSkipBeforeLoop(sg::State& state) -> void
{
  state.skipWithMessage("the scripted ahead skip");
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// A body that raises the interrupt flag inside its window leaves the stop
// rule of the growth reading a set flag once the sample closes (FR-032).
auto bmInterruptInWindow(sg::State& state) -> void
{
  for (auto index : state) {
    sg::detail::interruptFlag().store(true);
    sg::doNotOptimize(index);
  }
}

// A body that raises the interrupt flag when the second timed window of the
// process opens: in a fixed-iteration split the first window belongs to pass
// 0 and the second to pass 1, so the flag stands while the collection's
// second pass measures its repetition (FR-032).
auto bmInterruptSecondWindow(sg::State& state) -> void
{
  for (auto index : state) {
    if (arrivals.fetch_add(1) == 1) {
      sg::detail::interruptFlag().store(true);
    }
    sg::doNotOptimize(index);
  }
}

// The window whose body skips, counted in the windows one process opens: the
// arms below give a window the one iteration, so the arrival counter counts
// windows. A scenario sets it before its run, which it may do because every
// scenario runs in its own process.
std::uint64_t skipWindow = 0;

// A body that skips the window `skipWindow`: the record of that window carries
// the skip's own reason, so the pass that ran it closes with the rows of its
// earlier repetitions, or with none (FR-031).
auto bmSkipWindow(sg::State& state) -> void
{
  if (arrivals.fetch_add(1) == skipWindow) {
    state.skipWithMessage("the scripted pass skip");
  }
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// T022: the end-to-end two-pass run (FR-009, FR-010, SC-01, US1 scenario 1).
auto scenarioTwoPassRun() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto four = sg::registerBenchmark(&bmPlain, "bmPassFour");
  attachColumns(four, {0, 1, 2, 3});
  auto two = sg::registerBenchmark(&bmPlain, "bmPassTwo");
  attachColumns(two, {0, 1});
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  const RunAccount wide =
      runMeasured(*needInstance("bmPassFour"), valueOptions);
  const RunAccount narrow =
      runMeasured(*needInstance("bmPassTwo"), valueOptions);
  const sg::ResultRow& row = wide.result.rows.front();

  check(wide.result.metricLabels == std::vector<std::string> {"pair_a",
                                                              "pair_b",
                                                              "pair_c",
                                                              "pair_d"},
        "no metric column is dropped, and the merged row keeps the attach "
        "order (FR-009)");
  checkColumnsAtFullRatio(row, {0, 1, 2, 3}, kPlainSteps, kValueIterations);
  check(row.iterations == kValueIterations,
        "the merged row reports the iteration count every pass ran (FR-010)");
  check(closeTo(row.timePerIterationNs,
                expectedTimePerIteration(kPlainSteps, kValueIterations)),
        "the merged row reads the time columns of the reference pass (FR-015)");
  check(row.endpointSamples == kPlainWindowReads,
        "the merged row discloses the endpoint samples of the reference pass, "
        "so the passes beyond it add no sampling to the row (FR-015)");
  check(closeTo(row.metrics[0].value,
                narrow.result.rows.front().metrics[0].value),
        "a column of the reference pass reads the figure a one-pass run of the "
        "same body reads (FR-014)");

  // The window account of FR-009 and FR-010: one more repetition of a
  // two-pass run costs exactly the endpoint samples of the two windows it
  // opens, and one more repetition of a one-pass run costs one window. The
  // equality of the two marginals at the same pinned iteration count is the
  // statement that both passes ran every repetition at that count.
  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount wideHigh =
      runMeasured(*needInstance("bmPassFour"), highOptions);
  const RunAccount wideLow =
      runMeasured(*needInstance("bmPassFour"), lowOptions);
  const RunAccount narrowHigh =
      runMeasured(*needInstance("bmPassTwo"), highOptions);
  const RunAccount narrowLow =
      runMeasured(*needInstance("bmPassTwo"), lowOptions);
  const std::uint64_t wideMarginal = wideHigh.reads - wideLow.reads;
  const std::uint64_t narrowMarginal = narrowHigh.reads - narrowLow.reads;
  check(narrowMarginal == kPlainWindowReads,
        "a one-pass run costs the two endpoint samples of its window per "
        "repetition (FR-009)");
  check(wideMarginal == kTwoPassCount * kPlainWindowReads,
        "a limit of 4 with the four two-event columns runs two passes, each "
        "running every repetition at the same iteration count (FR-009, FR-010, "
        "SC-01)");
  check(wideMarginal == kTwoPassCount * narrowMarginal,
        "the pass beyond the first adds the endpoint samples of its own window "
        "and no sampling of its own (FR-009, FR-010)");
  check(wideHigh.result.rows.front().iterations == kValueIterations,
        "both passes reuse the iteration count the run settled on (FR-010)");

  // FR-010, D-06: the calibration stands outside the pass loop. The reads a
  // run owes to nothing but its own setup are its action count minus the
  // endpoint samples of every window it opened, and that residue is the same
  // however many repetitions the run took. A calibration re-run per
  // repetition or per pass inside the loop would widen it.
  const std::uint64_t narrowSetupLow =
      narrowLow.reads - kCountLowRepetitions * kPlainWindowReads;
  const std::uint64_t narrowSetupHigh =
      narrowHigh.reads - kCountHighRepetitions * kPlainWindowReads;
  const std::uint64_t wideSetupLow =
      wideLow.reads - kCountLowRepetitions * kTwoPassCount * kPlainWindowReads;
  const std::uint64_t wideSetupHigh = wideHigh.reads
      - kCountHighRepetitions * kTwoPassCount * kPlainWindowReads;
  check(narrowSetupLow == narrowSetupHigh
            && wideSetupLow == wideSetupHigh,
        "the setup reads of a run do not grow with its repetitions: the "
        "calibration runs once, before the first pass, and never inside the "
        "pass loop (FR-010, D-06)");

  std::puts("two-pass-run: ok");
  return 0;
}

// T022: the pause-and-resume arm. One folded pair per pass, measured in the
// two-pass run and in two one-pass mirrors, so each pass's net stands beside
// the merged row (FR-009, FR-014, FR-015).
auto scenarioPauseAcrossPasses() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto firstPass = sg::registerBenchmark(&bmPaused, "bmPauseFirstPass");
  attachColumns(firstPass, {0, 1});
  auto secondPass = sg::registerBenchmark(&bmPaused, "bmPauseSecondPass");
  attachColumns(secondPass, {2, 3});
  auto bothPasses = sg::registerBenchmark(&bmPaused, "bmPauseBothPasses");
  attachColumns(bothPasses, {0, 1, 2, 3});
  auto unfolded = sg::registerBenchmark(&bmPlain, "bmPlainBothPasses");
  attachColumns(unfolded, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  const RunAccount folded =
      runMeasured(*needInstance("bmPauseBothPasses"), valueOptions);
  const RunAccount passZero =
      runMeasured(*needInstance("bmPauseFirstPass"), valueOptions);
  const RunAccount passOne =
      runMeasured(*needInstance("bmPauseSecondPass"), valueOptions);
  const RunAccount plain =
      runMeasured(*needInstance("bmPlainBothPasses"), valueOptions);

  const sg::ResultRow& row = folded.result.rows.front();
  check(folded.result.pauseSamplesFolded && row.pauseSamplesFolded,
        "the passes of the run folded their pause pairs");
  check(row.endpointSamples == kPausedWindowReads,
        "the merged row discloses the samples of the reference pass's window: "
        "the two endpoints beside the folded interval (FR-015)");
  check(closeTo(row.timePerIterationNs,
                expectedTimePerIteration(kPausedSteps, kValueIterations)),
        "each pass nets the interval it excludes, and the merged row reads the "
        "reference pass's net time per iteration (FR-009, FR-014, FR-015)");
  checkColumnsAtFullRatio(row, {0, 1, 2, 3}, kPausedSteps, kValueIterations);

  const sg::ResultRow& zeroRow = passZero.result.rows.front();
  const sg::ResultRow& oneRow = passOne.result.rows.front();
  check(closeTo(row.timePerIterationNs, zeroRow.timePerIterationNs),
        "the merged time per iteration is the reference pass's own net figure, "
        "measured alone (FR-014, FR-015)");
  check(row.endpointSamples == zeroRow.endpointSamples,
        "the merged row's endpoint samples are the reference pass's (FR-015)");
  check(closeTo(row.metrics[0].value, zeroRow.metrics[0].value)
            && closeTo(row.metrics[1].value, zeroRow.metrics[1].value),
        "the columns of the reference pass net their interval as the mirror "
        "run nets its own (FR-009, FR-014)");
  check(closeTo(row.metrics[2].value, oneRow.metrics[0].value)
            && closeTo(row.metrics[3].value, oneRow.metrics[1].value),
        "the second pass nets the interval inside its own window, so its "
        "columns read the figures of a one-pass run of the same body (FR-009, "
        "FR-014)");

  const sg::ResultRow& plainRow = plain.result.rows.front();
  check(!plainRow.pauseSamplesFolded && !plain.result.pauseSamplesFolded,
        "the contrast run folded no interval");
  check(plainRow.endpointSamples == kPlainWindowReads,
        "the contrast run holds the audit point's sample pair");
  check(!closeTo(row.metrics[0].value, plainRow.metrics[0].value),
        "the folded interval stands out of the figures the folded run reports");

  // The window account of the pause arm: a repetition of a two-pass run whose
  // every window folds one interval costs the four sampling actions of each of
  // its two windows, and no window of one pass samples inside the other
  // (FR-009).
  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount high =
      runMeasured(*needInstance("bmPauseBothPasses"), highOptions);
  const RunAccount low =
      runMeasured(*needInstance("bmPauseBothPasses"), lowOptions);
  check(high.reads - low.reads == kTwoPassCount * kPausedWindowReads,
        "every pass opens its window once per repetition and folds its own "
        "interval inside it (FR-009, FR-014)");

  std::puts("pause-across-passes: ok");
  return 0;
}

// T023: the over-limit run (FR-004, SC-01, US1 scenario 2). The five-event
// column carries the scripted enabled/running pair, which is the fake's only
// running-ratio mechanism, so its pass reports a ratio below 1.0 while the
// columns that fit keep 1.0.
auto scenarioOverLimitRun() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit, true);

  const sg::test::PassColumn wide = sg::test::passColumns()[kWideColumnIndex];
  // Five PMU events on an object with a programmed count of 4: over the limit,
  // so the planner gives it a pass of its own (FR-004, E-03 rule 4).
  const std::vector<std::string_view> overLimitEvents {wide.events[0],
                                                       wide.events[1],
                                                       wide.events[2],
                                                       kEnabledLeaf,
                                                       kRunningLeaf};

  auto all = sg::registerBenchmark(&bmPlain, "bmOverLimit");
  attachColumns(all, {0, 1, 2, 3});
  all.addMetric(sumLeaves(wide.object, overLimitEvents), wide.label);
  auto alone = sg::registerBenchmark(&bmPlain, "bmOverLimitAlone");
  alone.addMetric(sumLeaves(wide.object, overLimitEvents), wide.label);
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  const RunAccount merged =
      runMeasured(*needInstance("bmOverLimit"), valueOptions);
  const RunAccount single =
      runMeasured(*needInstance("bmOverLimitAlone"), valueOptions);
  const sg::ResultRow& row = merged.result.rows.front();

  check(merged.result.metricLabels == std::vector<std::string> {"pair_a",
                                                                "pair_b",
                                                                "pair_c",
                                                                "pair_d",
                                                                "wide"},
        "the over-limit column stands beside the columns that fit, in request "
        "order (FR-004)");
  checkColumnsAtFullRatio(row, {0, 1, 2, 3}, kPlainSteps, kValueIterations, 5);

  const auto& overLimit = row.metrics[4];
  check(overLimit.availability == sg::counters::Availability::COUNTABLE,
        "the column over the limit still folds a countable value (FR-004)");
  check(closeTo(overLimit.runningRatio, kScriptedRatio),
        "the over-limit column reports a running ratio below 1.0 from the "
        "fake's rotation while the other columns keep 1.0 (FR-004, SC-01)");
  check(
      overLimit.runningRatio < 1.0,
      "the ratio below 1.0 discloses the pass that oversubscribed its source");
  check(overLimit.scaled,
        "the column whose pass reports the ratio discloses it as scaled");
  const double overLimitFold = 3.0
          * static_cast<double>((kWideColumnIndex + 1)
                                * sg::test::kPassEventStep)
      + static_cast<double>(kEnabledStep) + static_cast<double>(kRunningStep);
  check(closeTo(overLimit.value,
                overLimitFold / static_cast<double>(kValueIterations)),
        "the over-limit column carries the value its own pass folded (FR-014)");
  check(closeTo(overLimit.value,
                single.result.rows.front().metrics[0].value),
        "the merged figure is the one the same column reads when it stands "
        "alone in its pass (FR-014)");

  // Four two-event columns fill two passes and the column over the limit
  // stands alone in a third: the window account names that count, because
  // each pass costs its two endpoint samples per repetition and nothing else.
  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount high =
      runMeasured(*needInstance("bmOverLimit"), highOptions);
  const RunAccount low = runMeasured(*needInstance("bmOverLimit"), lowOptions);
  check(high.reads - low.reads == kThreePassCount * kPlainWindowReads,
        "the four columns that fit take two passes and the column over the "
        "limit takes a pass of its own (FR-004, SC-01)");

  std::puts("over-limit-run: ok");
  return 0;
}

// T024: the lifetime of a pass window (FR-008, FR-012, FR-016, SC-09, D-10).
// The function spells its arm `WindowSpan` rather than that noun: the
// time-source gate of `test/time_source_gate.sh` matches a banned clock term
// as a substring, and an arm name that ends in the noun followed by an open
// parenthesis is such a hit.
auto scenarioWindowSpan() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto fitting = sg::registerBenchmark(&bmAttemptLog, "bmWindowFit");
  attachColumns(fitting, {0, 1});
  auto wide = sg::registerBenchmark(&bmAttemptLog, "bmWindowTwoPass");
  attachColumns(wide, {0, 1, 2, 3});
  // The scale arm runs ten thousand repetitions, far past the attempt log,
  // so it runs the plain body: the read count of the timed path is what the
  // arm measures and the log stands outside it.
  auto scale = sg::registerBenchmark(&bmPlain, "bmWindowScale");
  attachColumns(scale, {0, 1});
  sg::detail::expandRegistry();

  // One iteration and one repetition, so a window holds exactly its two
  // endpoint samples and the action count of a run separates into the reads
  // a plan takes for itself and the reads of the windows it opened.
  const auto singleOptions = optionsFor(1, 1);

  resetAttempts();
  const std::uint64_t onePassBase = fake->readActions();
  const RunAccount onePass =
      runMeasured(*needInstance("bmWindowFit"), singleOptions);
  check(arrivals.load() == 1, "the one-pass run opened one window");
  const std::uint64_t onePassOpen = attempts[0].reads - onePassBase;
  check(onePass.reads - onePassOpen == 1,
        "the only read after the body of a window is that window's closing "
        "sample: the plan of the pass opens before the window and closes after "
        "it (FR-012)");

  resetAttempts();
  const std::uint64_t twoPassBase = fake->readActions();
  const RunAccount twoPass =
      runMeasured(*needInstance("bmWindowTwoPass"), singleOptions);
  check(arrivals.load() == kTwoPassCount,
        "the two-pass run opened one window per pass (FR-009, FR-012)");
  const std::uint64_t firstOpen = attempts[0].reads - twoPassBase;
  const std::uint64_t secondOpen = attempts[1].reads - twoPassBase;
  check(firstOpen == onePassOpen,
        "the plan is computed before the first timed window and reads no "
        "counter: the actions a two-pass run has spent when its first window "
        "opens are the actions a one-pass run has spent (FR-008, FR-012)");
  check(secondOpen - firstOpen >= kPlainWindowReads,
        "each pass closes its plan after its last timed window and the next "
        "pass opens its plan before its first: at least the closing sample of "
        "the first window and the opening sample of the second stand between "
        "the two bodies (FR-012)");
  check(twoPass.reads - secondOpen == 1,
        "the merge runs after the last pass closes and reads no counter: one "
        "closing sample stands after the last body of the last pass (FR-016)");

  // Nothing but the window endpoints stands inside a window, per repetition:
  // the account of FR-009 over the logging body.
  const auto highOptions = optionsFor(1, kCountHighRepetitions);
  const auto lowOptions = optionsFor(1, kCountLowRepetitions);
  const RunAccount high =
      runMeasured(*needInstance("bmWindowTwoPass"), highOptions);
  const RunAccount low =
      runMeasured(*needInstance("bmWindowTwoPass"), lowOptions);
  check(high.reads - low.reads == kTwoPassCount * kPlainWindowReads,
        "one more repetition costs the endpoint samples of the two windows and "
        "no read inside a window (FR-009, FR-012, D-10)");

  // SC-09, D-10: a one-pass metric run keeps the audit point's per-iteration
  // read count. The two runs differ by one repetition, so the reads a plan
  // takes for itself cancel and the difference is one window of one iteration.
  const auto scaleHigh = optionsFor(1, kScaleHighRepetitions);
  const auto scaleLow = optionsFor(1, kScaleLowRepetitions);
  const RunAccount many =
      runMeasured(*needInstance("bmWindowScale"), scaleHigh);
  const RunAccount almostMany =
      runMeasured(*needInstance("bmWindowScale"), scaleLow);
  check(many.reads - almostMany.reads == sg::test::kAuditPointReadsPerIteration,
        "a one-pass metric run costs the audit point's provider reads per "
        "iteration: two, the pair of endpoint samples (SC-09, D-10)");

  std::puts("window-lifetime: ok");
  return 0;
}

// T025: the pass order of a threaded run (FR-011, FR-013, FR-034, SC-10).
// The step the clock leaves of a percentile arm carry. The chunk plan sizes a
// chunk from the iteration time against the cost of one sampling action, so a
// scripted iteration time far above any real sampling action leaves one
// iteration to each chunk, in every pass of the run, whatever the machine
// measures for the sample: the shape `harness_percentile_test.cpp` scripts for
// the same reason.
constexpr std::uint64_t kPercentileTimeStep = 20'000'000;

// The iteration count of a percentile arm. One iteration per chunk means one
// chunk per iteration, and twelve of them answer both points of the arm: p50
// needs two chunks and p90 needs ten (FR-013).
constexpr std::uint64_t kPercentileIterations = 12;

// The points of the percentile arms and the token each prints by, the option
// list and the printed spelling of the same two points (FR-011).
constexpr double kPoint50 = 50.0;
constexpr double kPoint90 = 90.0;
constexpr std::array<std::pair<double, std::string_view>, 2> kPercentilePoints {
    std::pair {kPoint50, std::string_view {"p50="}},
    std::pair {kPoint90, std::string_view {"p90="}},
};

// The slots of a percentile grid that belong to the row before its columns do:
// the primary time and the time column the primary is not (SC-001).
constexpr std::size_t kTimeSlotCount = 2;

// Stand the clock leaves of the fixture at the percentile step. `setPoints`
// rewinds the leaf it scripts, so this runs before the scenario attaches a
// metric, which is where the tree first opens an object.
auto scriptPercentileClock() -> void
{
  fake->setPoints(sg::test::kPassClockObject,
                  sg::test::kPassRealTimeLeaf,
                  {0},
                  kPercentileTimeStep);
  fake->setPoints(sg::test::kPassClockObject,
                  sg::test::kPassCpuTimeLeaf,
                  {0},
                  kPercentileTimeStep);
}

auto scenarioThreadedPassOrder() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);
  scriptPercentileClock();

  auto threaded = sg::registerBenchmark(&bmContextLog, "bmThreadedTwoPass");
  threaded.threads(kThreadCount);
  attachColumns(threaded, {0, 1, 2, 3});
  auto single = sg::registerBenchmark(&bmPlain, "bmSingleTwoPass");
  attachColumns(single, {0, 1, 2, 3});
  // The window account runs three and two repetitions, whose attempts far
  // outnumber the four the order arm logs, so it runs the plain body on its
  // own instances: the body of the order arm belongs to the order arm's run.
  auto account = sg::registerBenchmark(&bmPlain, "bmThreadedAccount");
  account.threads(kThreadCount);
  attachColumns(account, {0, 1, 2, 3});
  auto narrowAccount =
      sg::registerBenchmark(&bmPlain, "bmThreadedNarrowAccount");
  narrowAccount.threads(kThreadCount);
  attachColumns(narrowAccount, {0, 1});
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  resetAttempts();
  // A count above one names the contexts in the instance name (FR-016), the
  // suffix `harness_percentile_test.cpp` spells the same way.
  const std::string threadedInstance = "bmThreadedTwoPass/threads:2";
  const std::string accountInstance = "bmThreadedAccount/threads:2";
  const std::string narrowAccountInstance = "bmThreadedNarrowAccount/threads:2";
  const RunAccount threadedRun =
      runMeasured(*needInstance(threadedInstance), valueOptions);
  const RunAccount singleRun =
      runMeasured(*needInstance("bmSingleTwoPass"), valueOptions);

  // Every claim below reads the log and the action counts after the run
  // returned, which is after the harness joined every context of every pass
  // (FR-034). No sleep stands in this scenario and no clock does.
  const std::uint64_t arrived = arrivals.load();
  check(arrived == kTwoPassCount * static_cast<std::uint64_t>(kThreadCount),
        "every context ran every pass of the run (FR-011, FR-013)");

  std::uint64_t firstPassLastArrival = 0;
  std::uint64_t secondPassFirstArrival = kMaxAttempts;
  std::uint64_t firstPassContexts = 0;
  std::uint64_t secondPassContexts = 0;
  for (std::uint64_t entry = 0; entry < arrived; ++entry) {
    const AttemptRecord& record = attempts[entry];
    check(record.threads == kThreadCount,
          "every pass of the run ran on the same count of contexts (FR-011)");
    check(record.hasContext,
          "every attempt of a threaded run ran on a live context");
    const std::uint64_t contextBit = 1ULL
        << static_cast<std::size_t>(record.threadIndex);
    check((record.ordinal == 1 || record.ordinal == 2)
              && (record.ordinal == 1 ? (firstPassContexts & contextBit) == 0
                                      : (secondPassContexts & contextBit) == 0),
          "each context opened one window per pass, so no context ran a pass "
          "twice (FR-013)");
    if (record.ordinal == 1) {
      firstPassContexts |= contextBit;
      firstPassLastArrival = record.arrival;
    } else if (record.ordinal == 2) {
      secondPassContexts |= contextBit;
      if (record.arrival < secondPassFirstArrival) {
        secondPassFirstArrival = record.arrival;
      }
    }
  }
  check(firstPassContexts == 0b11 && secondPassContexts == 0b11,
        "both passes ran on both contexts, the same contexts the run chose "
        "once (FR-011, FR-013)");
  check(secondPassFirstArrival > firstPassLastArrival,
        "pass 0 completes in both contexts before pass 1 opens in either, so "
        "no pass window stays open when a later pass window opens (FR-013, "
        "SC-10, FR-034)");

  std::uint64_t firstPassSerial = 0;
  std::uint64_t secondPassSerial = 0;
  for (std::uint64_t entry = 0; entry < arrived; ++entry) {
    const AttemptRecord& record = attempts[entry];
    if (record.ordinal == 1) {
      firstPassSerial ^= record.contextSerial + 1;
    } else if (record.ordinal == 2) {
      secondPassSerial ^= record.contextSerial + 1;
    }
  }
  check(firstPassSerial == secondPassSerial && firstPassSerial != 0,
        "the run chooses its contexts once, so every pass reports the same "
        "placement (FR-011)");

  const sg::ResultRow& row = threadedRun.result.rows.front();
  check(threadedRun.result.contexts.size()
            == static_cast<std::size_t>(kThreadCount),
        "the run discloses one context policy per context, not one per pass "
        "(FR-011, FR-015)");
  check(row.iterations
            == kValueIterations * static_cast<std::uint64_t>(kThreadCount),
        "the threaded row sums the iteration count of both contexts of the "
        "reference pass, the count every context ran in every pass (FR-011, "
        "FR-015)");
  checkColumnsAtFullRatio(row, {0, 1, 2, 3}, kPlainSteps, kValueIterations);
  for (std::size_t column = 0; column < row.metrics.size(); ++column) {
    check(closeTo(row.metrics[column].value,
                  singleRun.result.rows.front().metrics[column].value),
          "a per-iteration column of the threaded two-pass row reads the "
          "figure the single-context two-pass run reads, so both contexts "
          "contributed to both passes (FR-011, FR-013)");
  }

  // The per-pass window account: a repetition of the two-pass threaded run
  // opens one window per context per pass and no window anywhere else. The
  // count of those windows is what a pass that overlapped another, or a
  // context that re-ran a pass, would widen.
  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount high =
      runMeasured(*needInstance(accountInstance), highOptions);
  const RunAccount low =
      runMeasured(*needInstance(accountInstance), lowOptions);
  const RunAccount narrowHigh =
      runMeasured(*needInstance(narrowAccountInstance), highOptions);
  const RunAccount narrowLow =
      runMeasured(*needInstance(narrowAccountInstance), lowOptions);
  check(narrowHigh.reads - narrowLow.reads
            == static_cast<std::uint64_t>(kThreadCount) * kPlainWindowReads,
        "a one-pass threaded run opens one window per context per repetition "
        "(FR-011, FR-013)");
  check(high.reads - low.reads == static_cast<std::uint64_t>(kTwoPassCount)
                                     * static_cast<std::uint64_t>(kThreadCount)
                                     * kPlainWindowReads,
        "the two-pass threaded run opens exactly one window per context per "
        "pass: the window counts of the passes show no overlap and no window "
        "held open past its pass (FR-011, FR-013, SC-10)");

  // The chunk fold of a chunked threaded split run walks the columns this
  // pass leaves unmeasured, so the percentile row answers as many columns as
  // the run opened. The arm runs last: it widens the arrival log, which the
  // claims above read.
  auto chunkedOptions = valueOptions;
  chunkedOptions.showPercentiles = true;
  chunkedOptions.percentilePoints = {kPoint50};
  const LedgerAccount chunked =
      runMeasuredLedger(*needInstance(threadedInstance), chunkedOptions);
  const sg::BenchmarkResult& chunkedResult = chunked.run.result;
  const sg::ResultRow& chunkedRow = chunkedResult.rows.front();
  check(chunkedRow.percentiles.size()
            == kTimeSlotCount + chunkedResult.metricLabels.size(),
        "the threaded chunk fold answers every column the run opened, the "
        "columns this pass leaves unmeasured included (FR-014, FR-019)");
  check(chunkedResult.contexts.size()
            == static_cast<std::size_t>(kThreadCount),
        "the merged result names both contexts of the run beside its row "
        "(FR-016)");
  // The chunk plan of the arm, pinned so every claim below reads answers and
  // not the absence of them: each context takes one iteration to a chunk, so
  // the merged row holds the chunks of both contexts (FR-006, FR-013).
  check(chunkedRow.chunkIterations == 1,
        "the chunk plan gives one iteration to a chunk in every context of "
        "the run (FR-006)");
  check(
      chunkedRow.chunkCount
          == kValueIterations * static_cast<std::uint64_t>(kThreadCount),
      "the merged row holds one chunk per iteration of each context (FR-006)");
  // The per-context answers stand one block per context of the row, in the
  // flattened order the result holds them, so context `context` of the single
  // row of this run stands at block `context` (specs/021 FR-014).
  check(chunkedResult.contextPercentiles.size()
            == static_cast<std::size_t>(kThreadCount),
        "the merged result carries one percentile block per context of its row "
        "(FR-014)");
  for (std::size_t context = 0;
       context < chunkedResult.contextPercentiles.size();
       ++context)
  {
    const auto& folds = chunkedResult.contextPercentiles[context];
    check(folds.size() == chunkedRow.percentiles.size(),
          "a context answers as many slots as the merged row does, the columns "
          "its pass leaves unmeasured included (FR-014, FR-019)");
    for (std::size_t index = 0; index < chunkedResult.metricLabels.size();
         ++index)
    {
      const std::size_t slot = kTimeSlotCount + index;
      // The pass tags of the merged row state the plan this run ran, columns 0
      // and 1 in pass 0 and columns 2 and 3 in pass 1, so exactly one pass
      // holds chunk values for every column. A context of the pass that did
      // not measure a column holds none at all, so the answer there is no
      // number: a slot read from the wrong pass cannot hide behind a figure of
      // its own (FR-015, E-05).
      check(folds[slot][0].available,
            "the slot of a context answers from the pass that measured its "
            "column, in both contexts of the run (FR-015, FR-014)");
      check(closeTo(folds[slot][0].value,
                    expectedColumnValue(index, kPlainSteps, 1)),
            "the slot of a context states the chunk figure of its own column "
            "(FR-015)");
    }
  }
  // The chunk ledger a threaded run retains: the merged row of a threaded run
  // carries no copy of the union its contexts answer from (FR-020), so the
  // join has no column to move onto it, and the entries that stand beside the
  // row are the per-context ledgers of the reference pass, the segment the join
  // leaves behind. A column of another pass therefore stands there empty, the
  // same absence the per-context slot above must not read: the slot of a column
  // pass 1 measured answers with a figure, where no ledger of this result holds
  // a chunk of it (FR-020, FR-015).
  check(chunked.rowRaw.size() == chunkedResult.rows.size(),
        "the retained ledger carries one entry per row of the merged result "
        "(FR-029)");
  check(chunked.rowRaw.front().columns.empty(),
        "the merged row of a threaded run retains no copy of the union its "
        "contexts carry (FR-020)");
  check(chunked.contextRaw.size() == chunkedResult.rows.size(),
        "the per-context ledger carries one entry per row of the merged result "
        "(FR-029)");
  for (std::size_t context = 0; context < chunkedResult.contexts.size();
       ++context)
  {
    const auto& ledger = chunked.contextRaw.front()[context];
    check(ledger.columns.size() == chunkedRow.percentiles.size(),
          "a context's ledger stands one column per slot of the merged row "
          "(FR-029)");
    for (std::size_t index = 0; index < chunkedResult.metricLabels.size();
         ++index)
    {
      const std::size_t slot = kTimeSlotCount + index;
      const auto& chunks = ledger.columns[slot];
      const double owed = expectedColumnValue(index, kPlainSteps, 1);
      if (chunkedRow.metrics[index].pass == 0) {
        check(chunks.size() == kValueIterations,
              "the context ledger of a column the reference pass measured holds "
              "the chunks behind that context's percentile (FR-029, FR-015)");
        for (const double chunk : chunks) {
          check(closeTo(chunk, owed),
                "the context chunk states the figure of its own column, the "
                "figure the context's slot above answers with (FR-029, FR-015)");
        }
      } else {
        check(chunks.empty(),
              "the context ledger stands as the reference pass folded it, empty "
              "for a column its pass left unmeasured, so the slot above cannot "
              "have been read off this ledger (FR-020, FR-015)");
      }
    }
  }

  std::puts("threaded-pass-order: ok");
  return 0;
}

// ============================================================================
// The collection method one run settles to (T032 to T034; US2; FR-023 to
// FR-025; data-model E-01; research D-09, D-10).
//
// `harness_pass_plan_test.cpp` drives the resolution table of E-01 as a pure
// function; the three arms here drive the same rows through a real collection
// run over the scripted fixture. Two observables carry every claim below, on
// the figures a run publishes today:
//
//   * the count of the fake's sampling actions per repetition, which names the
//     windows the run opened and therefore the passes it ran (FR-009);
//   * the running ratio of each metric column, which only a plan holding a
//     carrier leaf beside the scripted `cpu/enabled` and `cpu/running` leaves
//     can set below 1.0: `linkRatioSlots` (`source/counters/plan.cpp:142`) ties
//     the pair to a carrier slot by plan, so a pass that holds no pair leaves
//     discloses 1.0.
//
// The published `BenchmarkResult::metricMethod`, `passCount` and
// `MetricValue::pass` fields arrive with T047 and their printed bytes are
// US3's, so no claim here reads a field the result does not carry yet: the pass
// count and the settled method are read off the window account and the ratio
// disclosure.
// ============================================================================

// The passes the first-fit planner owes the rotated column set at the scripted
// limit of 4: the first two pairs fill pass 0, `rot_c` opens pass 1, and the
// pair that carries the enabled/running leaves opens pass 2 (E-03 rule 3).
constexpr std::uint64_t kRotatedPassCount = 3;

// The column of the rotated set that names the enabled/running leaves, so the
// plan that measures it holds the pair its carrier links to: the spelling
// T023's over-limit column uses for the same purpose.
constexpr std::size_t kPairCarrierColumn = 3;

// The fixture leaves the `multiplex` arm promotes to ratio-pair carriers, one
// per column. The fold of a column multiplies the ratio of every carrier leaf
// it holds (`windowRatio` in `source/counters/fold.cpp`), so two carriers of
// one object in one column would disclose the square of the scripted ratio.
constexpr std::array<std::string_view, 4> kRotatedCarrierLeaves {
    "pass_a0", "pass_b0", "pass_c0", "pass_d0"};

// The step the leaves of the no-limit host carry. These leaves stand outside
// `passColumns()`, whose steps are weighted by column index, so they hold one
// step of their own.
constexpr std::uint64_t kNoLimitLeafStep = 3 * sg::test::kPassEventStep;

// The columns of the rotated run in request order: the four fixture pairs over
// `cpu`, the last of them also summing the scripted pair leaves. Ten distinct
// PMU events against the scripted limit of 4, so the planner splits them and
// the forced `multiplex` method has to hold them whole.
[[nodiscard]] auto rotatedColumns() -> std::vector<sg::test::PassColumn>
{
  auto columns = std::vector<sg::test::PassColumn> {
      sg::test::PassColumn {
          "rot_a", sg::test::kPassCpuObject, {"pass_a0", "pass_a1"}},
      sg::test::PassColumn {
          "rot_b", sg::test::kPassCpuObject, {"pass_b0", "pass_b1"}},
      sg::test::PassColumn {
          "rot_c", sg::test::kPassCpuObject, {"pass_c0", "pass_c1"}},
      sg::test::PassColumn {"rot_d",
                            sg::test::kPassCpuObject,
                            {"pass_d0", "pass_d1", kEnabledLeaf, kRunningLeaf}},
  };
  // A `return columns;` would make the local the return object itself, the
  // shape `createRange` states in `source/harness/family.cpp`.
  return std::vector<sg::test::PassColumn>(columns);
}

// The columns of the no-limit host: three pairs over the source the fixture
// scripts no limit for, six PMU events costed against nothing (E-03 rule 5).
[[nodiscard]] auto noLimitColumns() -> std::vector<sg::test::PassColumn>
{
  auto columns = std::vector<sg::test::PassColumn> {
      sg::test::PassColumn {
          "free_a", sg::test::kPassRideObject, {"free_a0", "free_a1"}},
      sg::test::PassColumn {
          "free_b", sg::test::kPassRideObject, {"free_b0", "free_b1"}},
      sg::test::PassColumn {
          "free_c", sg::test::kPassRideObject, {"free_c0", "free_c1"}},
  };
  return std::vector<sg::test::PassColumn>(columns);
}

// Lay the scripted ratio pair on `cpu` and promote one leaf of every rotated
// column to a carrier of it. `addCounter` rewrites the catalog entry of a leaf
// it already holds and leaves the scripted sequence alone
// (`source/counters/fake_provider.cpp:199-221`), so a promoted leaf keeps the
// steps the fixture scripted for it. Every scripted fact precedes the
// registration, the rule `registerFixtureProvider` states.
auto registerCarrierProvider() -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::scriptPassFixture(*provider, sg::test::kHostPassLimit);
  scriptRatioPair(*provider);
  for (const std::string_view leaf : kRotatedCarrierLeaves) {
    provider->addCounter(sg::test::kPassCpuObject,
                         leaf,
                         kRatioPairUnit,
                         "rotated fixture leaf carrying the scripted pair",
                         sg::counters::Availability::COUNTABLE,
                         sg::counters::ReadMode::SYSCALL,
                         true);
  }
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(),
        "the carrier fixture registers on the local system");
  fake = scripted;
  return *scripted;
}

// Register the fixture and script the no-limit host on the source
// `scriptPassFixture` publishes no limit for. The `cpu` source keeps its
// scripted limit for the contrast arm, which needs a source that answers: the
// row of E-01 that settles `MULTIPLEX` reads only the sources a run needs
// (FR-005, FR-024).
auto registerNoLimitProvider() -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::scriptPassFixture(*provider, sg::test::kHostPassLimit);
  for (const auto& column : noLimitColumns()) {
    for (const std::string_view leaf : column.events) {
      provider->addCounter(
          column.object, leaf, "none", "leaf of a source that publishes none");
      provider->setPoints(column.object, leaf, {0}, kNoLimitLeafStep);
    }
  }
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(),
        "the no-limit fixture registers on the local system");
  fake = scripted;
  return *scripted;
}

auto attachRotatedColumns(sg::BenchmarkHandle& handle) -> void
{
  for (const auto& column : rotatedColumns()) {
    attachColumn(handle, column);
  }
}

auto attachNoLimitColumns(sg::BenchmarkHandle& handle) -> void
{
  for (const auto& column : noLimitColumns()) {
    attachColumn(handle, column);
  }
}

// The value one column of the rotated run owes: the fixture pair of its index,
// beside the two scripted pair leaves for the column that names them. The pair
// leaves are leaves of the expression like any other and step by their own
// scripted delta, the shape T023's over-limit column folds.
auto expectedRotatedValue(const std::size_t index) -> double
{
  auto expected = expectedColumnValue(index, kPlainSteps, kValueIterations);
  if (index == kPairCarrierColumn) {
    expected += static_cast<double>(kEnabledStep + kRunningStep)
        / static_cast<double>(kValueIterations);
  }
  return expected;
}

// The value one column of the no-limit host owes: the two leaves it sums, each
// stepping `kNoLimitLeafStep` once over the single window of the attempt.
auto expectedNoLimitValue() -> double
{
  return 2.0 * static_cast<double>(kNoLimitLeafStep) * kPlainSteps
      / static_cast<double>(kValueIterations);
}

// T032: the forced `multiplex` run (FR-024, FR-025, US2 scenario 1, E-01 row
// 4). The ten PMU events the `auto` run spreads over three passes stand in the
// one plan the forced method compiles, so the rotation of the scripted pair
// reaches every column of the run.
auto scenarioMultiplexRun() -> int
{
  registerCarrierProvider();

  auto split = sg::registerBenchmark(&bmPlain, "bmRotatedAuto");
  attachRotatedColumns(split);
  auto forced = sg::registerBenchmark(&bmPlain, "bmRotatedMultiplex");
  forced.metricMethod(sg::MetricMethod::MULTIPLEX);
  attachRotatedColumns(forced);
  // The single-plan mirror of the pair column: four events against a limit of
  // 4, so `auto` gives it the one plan of the audit point and the ratio it
  // discloses there is the figure the forced run has to match.
  auto alone = sg::registerBenchmark(&bmPlain, "bmRotatedPairAlone");
  attachColumn(alone, rotatedColumns()[kPairCarrierColumn]);
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  const RunAccount splitRun =
      runMeasured(*needInstance("bmRotatedAuto"), valueOptions);
  const RunAccount forcedRun =
      runMeasured(*needInstance("bmRotatedMultiplex"), valueOptions);
  const RunAccount aloneRun =
      runMeasured(*needInstance("bmRotatedPairAlone"), valueOptions);
  const sg::ResultRow& row = forcedRun.result.rows.front();
  const sg::ResultRow& splitRow = splitRun.result.rows.front();
  const sg::ResultRow& aloneRow = aloneRun.result.rows.front();

  check(forcedRun.result.metricLabels
            == std::vector<std::string> {"rot_a", "rot_b", "rot_c", "rot_d"},
        "the forced method measures every attached column of its one plan "
        "(FR-024)");
  for (std::size_t column = 0; column < row.metrics.size(); ++column) {
    const auto& value = row.metrics[column];
    check(value.availability == sg::counters::Availability::COUNTABLE,
          "every column of the rotated run folds a countable value (FR-024)");
    check(closeTo(value.runningRatio, kScriptedRatio),
          "every metric column of the forced run discloses the scripted ratio "
          "below 1.0 that the single-plan path discloses (FR-024, SC-01, US2 "
          "scenario 1)");
    check(value.runningRatio < 1.0,
          "the rotated run reports a fraction below full rate for every column "
          "of its one plan (FR-024)");
    check(value.scaled,
          "a column of a rotated window discloses as scaled (FR-024)");
    check(closeTo(value.value, expectedRotatedValue(column)),
          "the rotated column carries the value its leaves fold over the one "
          "plan (FR-024, FR-014)");
    check(closeTo(value.value, splitRow.metrics[column].value),
          "the same column reads the figure the split run reads, so the forced "
          "method changes the plan and nothing the fold returns (FR-024)");
  }
  check(closeTo(row.metrics[kPairCarrierColumn].runningRatio,
                aloneRow.metrics[0].runningRatio),
        "the ratio of the column of the one plan equals the ratio the same "
        "column discloses alone in a single plan (FR-024, US2 scenario 1)");
  check(closeTo(row.metrics[kPairCarrierColumn].value,
                aloneRow.metrics[0].value),
        "the value of that column equals its single-plan figure as well "
        "(FR-024, FR-014)");

  // The contrast with the planner path: under `auto` only the column whose pass
  // holds the pair leaves discloses the fraction, because the pair is tied to a
  // carrier slot by plan (FR-004, FR-009).
  check(closeTo(splitRow.metrics[0].runningRatio, 1.0),
        "a column of a pass that holds no pair leaves keeps running ratio 1.0 "
        "under `auto` (FR-004, SC-01)");
  check(!splitRow.metrics[0].scaled,
        "a column that ran its pass at full rate reports no scaling (FR-009)");
  check(closeTo(splitRow.metrics[kPairCarrierColumn].runningRatio,
                kScriptedRatio),
        "the one column whose pass holds the scripted pair is the one column "
        "that discloses it under `auto` (FR-004)");

  // The window account of the two methods at one pinned iteration count: three
  // windows per repetition for the split run and one for the forced run. That
  // single window is the statement that one plan holds every PMU event of the
  // run and the planner stayed out of it (FR-024, research D-10).
  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount splitHigh =
      runMeasured(*needInstance("bmRotatedAuto"), highOptions);
  const RunAccount splitLow =
      runMeasured(*needInstance("bmRotatedAuto"), lowOptions);
  const RunAccount forcedHigh =
      runMeasured(*needInstance("bmRotatedMultiplex"), highOptions);
  const RunAccount forcedLow =
      runMeasured(*needInstance("bmRotatedMultiplex"), lowOptions);
  check(splitHigh.reads - splitLow.reads
            == kRotatedPassCount * kPlainWindowReads,
        "the `auto` run of the same column set opens three windows per "
        "repetition, so the fixture does split at the scripted limit (FR-001, "
        "FR-009)");
  check(forcedHigh.reads - forcedLow.reads == kPlainWindowReads,
        "the forced run opens one window per repetition: the forced method "
        "compiles one plan holding every PMU event of the run and the planner "
        "stays out of it (FR-024, US2 scenario 1)");
  check(row.endpointSamples == kPlainWindowReads,
        "the row of the rotated run discloses the endpoint samples of its "
        "single window (FR-024, FR-015)");
  check(closeTo(row.timePerIterationNs,
                expectedTimePerIteration(kPlainSteps, kValueIterations)),
        "the time columns of the one plan read the reference figures (FR-015, "
        "FR-024)");

  std::puts("multiplex-run: ok");
  return 0;
}

// T033: the host whose needed sources publish no limit (FR-024, FR-025, spec
// D-11, E-01 row 3). Both the `auto` run and a run whose family forces
// `passes` keep the single kernel-multiplexed plan, because the planner has no
// published limit to split against.
auto scenarioNoLimitHost() -> int
{
  registerNoLimitProvider();

  auto automatic = sg::registerBenchmark(&bmPlain, "bmNoLimitAuto");
  attachNoLimitColumns(automatic);
  auto forced = sg::registerBenchmark(&bmPlain, "bmNoLimitPasses");
  forced.metricMethod(sg::MetricMethod::PASSES);
  attachNoLimitColumns(forced);
  // The contrast arm: the same six PMU events on the source that publishes 4,
  // which is a set the planner does split. Without it the single plan below
  // could come from a fixture that never splits at all.
  auto limited = sg::registerBenchmark(&bmPlain, "bmLimitedPairs");
  attachColumns(limited, {0, 1, 2});
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  const RunAccount automaticRun =
      runMeasured(*needInstance("bmNoLimitAuto"), valueOptions);
  const RunAccount forcedRun =
      runMeasured(*needInstance("bmNoLimitPasses"), valueOptions);
  const sg::ResultRow& row = forcedRun.result.rows.front();
  const sg::ResultRow& automaticRow = automaticRun.result.rows.front();

  check(forcedRun.result.metricLabels == std::vector<std::string> {"free_a",
                                                                  "free_b",
                                                                  "free_c"},
        "the run of the no-limit host measures every attached column (FR-024, "
        "FR-009)");
  for (std::size_t column = 0; column < row.metrics.size(); ++column) {
    const auto& value = row.metrics[column];
    check(value.availability == sg::counters::Availability::COUNTABLE,
          "every column of a source that publishes no limit folds a countable "
          "value (FR-005, FR-024)");
    check(closeTo(value.runningRatio, 1.0),
          "every column of the one kernel-multiplexed plan reports running "
          "ratio 1.0, the disclosure of the single plan (FR-024, SC-01)");
    check(!value.scaled,
          "a column of a plan the split never touched reports no scaling "
          "(FR-024)");
    check(closeTo(value.value, expectedNoLimitValue()),
          "the column folds the steps of its two leaves over the single window "
          "(FR-024, FR-014)");
    check(closeTo(value.value, automaticRow.metrics[column].value),
          "the forced run reads the figures the `auto` run reads, so the "
          "request settled the method the host can offer (FR-024, FR-025, "
          "E-01 row 3)");
  }
  check(row.iterations == kValueIterations,
        "the row of the single plan reports the iteration count of the run "
        "(FR-010, FR-024)");
  check(row.endpointSamples == kPlainWindowReads,
        "the row discloses the endpoint samples of one window (FR-015, "
        "FR-024)");

  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount automaticHigh =
      runMeasured(*needInstance("bmNoLimitAuto"), highOptions);
  const RunAccount automaticLow =
      runMeasured(*needInstance("bmNoLimitAuto"), lowOptions);
  const RunAccount forcedHigh =
      runMeasured(*needInstance("bmNoLimitPasses"), highOptions);
  const RunAccount forcedLow =
      runMeasured(*needInstance("bmNoLimitPasses"), lowOptions);
  const RunAccount limitedHigh =
      runMeasured(*needInstance("bmLimitedPairs"), highOptions);
  const RunAccount limitedLow =
      runMeasured(*needInstance("bmLimitedPairs"), lowOptions);
  check(automaticHigh.reads - automaticLow.reads == kPlainWindowReads,
        "the `auto` run of a host that publishes no limit opens one window per "
        "repetition: no plan stands, so the run keeps the single "
        "kernel-multiplexed plan (FR-024, spec D-11, E-01 row 3)");
  check(forcedHigh.reads - forcedLow.reads == kPlainWindowReads,
        "a forced `passes` run on that host settles `multiplex` and keeps the "
        "one plan: the request names a method the host cannot offer (FR-024, "
        "FR-025, E-01 row 3)");
  check(limitedHigh.reads - limitedLow.reads
            == kTwoPassCount * kPlainWindowReads,
        "the same count of PMU events on a source that publishes a limit does "
        "split into two passes, so the single plan above is the answer of the "
        "missing limit (FR-001, FR-005)");

  std::puts("no-limit-host: ok");
  return 0;
}

// T034: the `metricMethod` selector on a family with a fitting set (FR-023,
// FR-025, US2 scenario 2, E-01 row 5). The scripted limit of 8 holds all four
// pairs, so the planner answers one pass and the forced request adds a record
// to the run and no work to it.
auto scenarioSelectorOnePass() -> int
{
  registerFixtureProvider(sg::test::kWidePassLimit);

  // The chaining of the selector, the property `showPercentiles` has: it
  // returns the handle it was called on, so a family states its selections in
  // one expression (FR-023). The family stands unrun: the claim is the shape
  // of the call, and a percentile run would widen the window account below.
  auto chained = sg::registerBenchmark(&bmPlain, "bmSelectorChained");
  const sg::BenchmarkHandle& handle =
      chained.metricMethod(sg::MetricMethod::PASSES).showPercentiles(true);
  check(&handle == &chained,
        "the selector returns the handle it was called on, so it chains like "
        "showPercentiles (FR-023)");

  auto selected = sg::registerBenchmark(&bmPlain, "bmSelectedOnePass");
  selected.metricMethod(sg::MetricMethod::PASSES);
  attachColumns(selected, {0, 1, 2, 3});
  auto automatic = sg::registerBenchmark(&bmPlain, "bmAutomaticOnePass");
  attachColumns(automatic, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  const auto valueOptions = optionsFor(kValueIterations, 1);
  const RunAccount selectedRun =
      runMeasured(*needInstance("bmSelectedOnePass"), valueOptions);
  const RunAccount automaticRun =
      runMeasured(*needInstance("bmAutomaticOnePass"), valueOptions);
  const sg::ResultRow& row = selectedRun.result.rows.front();

  check(selectedRun.result.metricLabels == std::vector<std::string> {"pair_a",
                                                                    "pair_b",
                                                                    "pair_c",
                                                                    "pair_d"},
        "the selected family measures the fitting set in request order "
        "(FR-023, FR-009)");
  checkColumnsAtFullRatio(row, {0, 1, 2, 3}, kPlainSteps, kValueIterations);
  check(row.endpointSamples == kPlainWindowReads,
        "the row of a forced `passes` run of a fitting set discloses the "
        "endpoint samples of one window (FR-023, FR-015)");

  // The pass count the run publishes, read the way a run publishes it today:
  // one more repetition costs the endpoint samples of one window, the figure a
  // two-pass run doubles (FR-009, E-07). A forced `passes` run of a set that
  // fits the published limit therefore runs one pass and states a pass count of
  // 1; the `metricMethod`, `passCount` and `pass` fields that carry the same
  // three figures land with T047.
  const auto highOptions = optionsFor(kValueIterations, kCountHighRepetitions);
  const auto lowOptions = optionsFor(kValueIterations, kCountLowRepetitions);
  const RunAccount selectedHigh =
      runMeasured(*needInstance("bmSelectedOnePass"), highOptions);
  const RunAccount selectedLow =
      runMeasured(*needInstance("bmSelectedOnePass"), lowOptions);
  const RunAccount automaticHigh =
      runMeasured(*needInstance("bmAutomaticOnePass"), highOptions);
  const RunAccount automaticLow =
      runMeasured(*needInstance("bmAutomaticOnePass"), lowOptions);
  check(selectedHigh.reads - selectedLow.reads == kPlainWindowReads,
        "a forced `passes` run of a set that fits the published limit runs one "
        "pass: one more repetition costs the two endpoint samples of one "
        "window (FR-023, FR-025, US2 scenario 2, E-01 row 5)");
  check(automaticHigh.reads - automaticLow.reads == kPlainWindowReads,
        "the identical family with no request runs the same single pass: the "
        "request adds a record to the run and no work to it (FR-025, SC-03)");
  for (std::size_t column = 0; column < row.metrics.size(); ++column) {
    check(closeTo(row.metrics[column].value,
                  automaticRun.result.rows.front().metrics[column].value),
          "every column of the selected run reads the figure the `auto` run "
          "reads for it (FR-023, FR-014)");
  }

  std::puts("selector-one-pass: ok");
  return 0;
}

// ============================================================================
// What ends a split collection before its last pass, and what widens the
// row the merge reads: the calibration that stands ahead of the pass loop of
// an adaptive split answers a failure, a skip and a signal each with its own
// ending; the interrupt read between the repetitions of a pass stops the
// loop with more than one pass already measured; and a percentile run stands
// a column beside every column of the row, wider than the plan the planner
// costed (FR-016, FR-031, FR-032, FR-014; specs/021 FR-016; T058b).
// ============================================================================

// E-03 rule 6, FR-016: an adaptive split calibrates once ahead of pass 0,
// and every pass measures the count that calibration produced. A body that
// fails the record build of that one sample ends the collection with the
// calibration's own error, before any pass measures a row.
auto scenarioCalibrationAheadFails() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmNeverLoops, "bmAheadFailure");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  // The absence of a fixed iteration count is what stands the calibration
  // ahead of the pass loop; the failing sample ends that path on its first
  // run, whatever the growth target is.
  sg::detail::RunOptions options;
  options.repetitions = 1;
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmAheadFailure"));
  check(result.outcome == sg::RunOutcome::FAILED,
        "the split collection whose ahead calibration fails reports the "
        "failure (FR-016)");
  check(result.reason == "the benchmark never entered the timed loop (FR-017)",
        "the calibration's own error stands as the run's reason (FR-016, "
        "FR-017)");
  check(result.rows.empty(), "no pass measured a row after the failure");

  std::puts("calibration-ahead-fails: ok");
  return 0;
}

// FR-031: a body that skips before its loop hands the ahead calibration a
// skipped record, and the collection ends with the skip's own reason. The
// empty cursor the skip leaves is the shape of a run that takes no sample.
auto scenarioCalibrationAheadSkipped() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmSkipBeforeLoop, "bmAheadSkip");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  sg::detail::RunOptions options;
  options.repetitions = 1;
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result = runner.run(*needInstance("bmAheadSkip"));
  check(result.outcome == sg::RunOutcome::SKIPPED,
        "the ahead skip ends the split collection (FR-031)");
  check(result.reason == "the scripted ahead skip",
        "the skip's own reason stands as the run's reason, the way the "
        "warm-up skip carries its message (FR-031)");
  check(result.rows.empty(), "the skipped collection measured no row");

  std::puts("calibration-ahead-skipped: ok");
  return 0;
}

// FR-032: the flag read after the calibration run ends the collection the
// way it ends the warm-up: the run carries the skip outcome and the signal
// as its reason, and no pass opens.
auto scenarioCalibrationAheadInterrupted() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmInterruptInWindow, "bmAheadInterrupt");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  sg::detail::RunOptions options;
  options.repetitions = 1;
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmAheadInterrupt"));
  sg::detail::interruptFlag().store(false);
  check(
      result.outcome == sg::RunOutcome::SKIPPED,
      "the signal inside the ahead calibration ends the collection " "(FR-"
                                                                     "032)");
  check(result.reason == "interrupted by the user (SIGINT)",
        "the interrupted run states the signal as its reason (FR-032)");
  check(result.rows.empty(), "the interrupted collection measured no row");

  std::puts("calibration-ahead-interrupted: ok");
  return 0;
}

// A body that raises the interrupt flag once its timed window closed: the
// sample answers measured, so the flag reads as the interrupt of the phase
// that ran it (FR-032).
auto bmInterruptAfterWindow(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
  sg::detail::interruptFlag().store(true);
}

// E-03 rule 6, FR-010, D-06: a split collection with no fixed count takes its
// iteration count from the one calibration ahead of pass 0, and every pass
// measures that count. The count itself is the harness's, so the check is the
// invariant: one row per repetition, every row at the same positive count.
auto scenarioCalibrationAheadGrows() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmPlain, "bmAheadGrow");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  sg::detail::RunOptions options;
  options.repetitions = 2;
  options.minTimeNs = 1;
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result = runner.run(*needInstance("bmAheadGrow"));

  check(result.outcome == sg::RunOutcome::MEASURED,
        "the split collection whose ahead calibration qualifies measures "
        "every pass (FR-010)");
  check(result.passCount == 2,
        "the source limit split the columns across two passes (FR-009)");
  std::size_t measured = 0;
  bool oneCount = true;
  for (const auto& row : result.rows) {
    if (row.kind != sg::RowKind::REPETITION) {
      continue;
    }
    oneCount = oneCount && row.iterations > 0
        && (measured == 0 || row.iterations == result.rows[0].iterations);
    ++measured;
  }
  check(oneCount && measured == 2,
        "every repetition measured the count the ahead calibration gave "
        "(FR-010, D-06)");

  std::puts("calibration-ahead-grows: ok");
  return 0;
}

// FR-032: the flag raised after the calibration window closes reaches the
// phase itself: the collection ends on the signal, and no pass opens.
auto scenarioCalibrationAheadSignal() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmInterruptAfterWindow, "bmAheadSignal");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  sg::detail::RunOptions options;
  options.repetitions = 1;
  options.minTimeNs = 1;
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result = runner.run(*needInstance("bmAheadSignal"));
  sg::detail::interruptFlag().store(false);

  check(
      result.outcome == sg::RunOutcome::SKIPPED,
      "the signal after the calibration window ends the collection " "(FR-"
                                                                     "032)");
  check(
      result.reason == "interrupted by the user (SIGINT)",
      "the interrupted calibration states the signal as its reason " "(FR-"
                                                                     "032)");
  check(result.rows.empty(), "the interrupted calibration opened no pass");

  std::puts("calibration-ahead-signal: ok");
  return 0;
}

// FR-019, FR-020: the columns the plan does not cost stand outside the pass
// split. A `--counter` address the catalog cannot answer for, and a second
// source with a limit of its own, ride the split: the run measures every
// pass, and the unanswerable address prints its refusal at the column site.
// The merged row answers from the reference pass, so a counter column a
// later pass measured stands outside it - the gap the pull-request record
// carries (FR-014, D-5).
auto scenarioUnplannedColumnsAcrossPasses() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmPlain, "bmPassUnplanned");
  attachColumns(wide, {0, 1, 2});
  attachColumn(wide, sg::test::passRideColumns()[0]);
  sg::detail::expandRegistry();

  auto options = optionsFor(kValueIterations, 1);
  options.showPercentiles = true;
  options.percentilePoints = {50.0, 90.0};
  options.counterAddresses = {"cpu/pass_a0", "absent_source/no_such_leaf"};
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmPassUnplanned"));

  check(result.outcome == sg::RunOutcome::MEASURED,
        "the split collection with columns outside its plan measures its "
        "passes (FR-019, FR-020)");
  check(result.passCount > 1,
        "the limits of the two sources split the columns across passes "
        "(FR-009)");
  check(result.metricLabels.size() == 6,
        "the two time columns and the four requested columns stand in the "
        "order the request opened them (FR-001, FR-015)");
  check(!result.rows.empty()
            && result.rows.front().percentiles.size()
                == 2 + result.metricLabels.size(),
        "the chunk fold answers every column the run opened, the address "
        "the catalog cannot answer for included (FR-019, FR-020)");

  std::puts("unplanned-columns-across-passes: ok");
  return 0;
}

// FR-014, FR-032: the flag read between the repetitions ends the pass loop
// mid-collection. With two passes measured and the run stopped, the merge
// stays unrun and the published result is the stopped pass's own row, whose
// columns from the reference pass stand unread.
auto scenarioInterruptInsideSplit() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide =
      sg::registerBenchmark(&bmInterruptSecondWindow, "bmInterruptSplit");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  resetAttempts();
  const auto options = optionsFor(1, 1);
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmInterruptSplit"));
  sg::detail::interruptFlag().store(false);

  check(arrivals.load() == kTwoPassCount,
        "the collection ran one window of pass 0 and one of pass 1 before the "
        "flag ended it (FR-032)");
  check(result.outcome == sg::RunOutcome::SKIPPED,
        "the flag between the repetitions ends the run as a skip (FR-032)");
  check(result.reason == "interrupted by the user (SIGINT)",
        "the stopped collection states the signal as its reason (FR-032)");
  check(result.rows.size() == 1,
        "the published result is the stopped pass's own single row, because "
        "the merge of an interrupted collection stays unrun (FR-014)");
  const sg::ResultRow& row = result.rows.front();
  check(row.metrics[0].availability == sg::counters::Availability::GAP
            && row.metrics[1].availability == sg::counters::Availability::GAP,
        "the columns pass 0 measured stand unread on the stopped pass's row, "
        "the figures the merge would have carried (FR-014)");
  check(row.metrics[2].availability == sg::counters::Availability::COUNTABLE
            && closeTo(row.metrics[2].value,
                       expectedColumnValue(2, kPlainSteps, 1)),
        "the columns of the stopped pass answer from that pass's own folds "
        "(FR-014)");
  check(!result.passSpread.has_value(),
        "the interrupted collection publishes no spread (FR-017)");

  std::puts("interrupt-inside-split: ok");
  return 0;
}

// The columns of the three-pass plan the arms below stop short of: the four
// pairs the scripted limit lays across two passes, beside the column that
// stands over the limit on its own (FR-004, E-03 rule 4).
auto attachThreePassPlan(sg::BenchmarkHandle& handle) -> void
{
  attachColumns(handle, {0, 1, 2, 3, kWideColumnIndex});
}

// The plan a stopped arm stops short of, read from a run of the same columns
// that reaches its last pass: a stopped run publishes the folds that stand, so
// its own result names the collection it ran, and the plan it was given
// reaches this file through a run that completed.
auto checkWholeThreePassPlan(sg::Instance& instance) -> void
{
  const auto options = optionsFor(1, 1);
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult whole = runner.run(instance);
  check(whole.outcome == sg::RunOutcome::MEASURED,
        "the run of these columns that reaches its last pass measures "
        "(FR-009)");
  check(whole.passCount == kThreePassCount,
        "the scripted limit lays these columns across three passes, the plan a "
        "stopped run stops short of (FR-004, FR-017)");
}

// What a collection stopped before its last pass publishes, on the result and
// on both report surfaces: the method it settled to and the count of the
// passes whose folds stand, with no spread and no pass tag, because the merge
// that writes both stayed unrun (FR-017, FR-019, FR-031, FR-038).
// `standing` is that count and `printed` the console the stop printed.
auto checkStoppedCollection(const sg::BenchmarkResult& result,
                            const std::size_t standing,
                            const std::string& printed,
                            sg::detail::Runner& runner) -> void
{
  check(result.metricMethod == sg::MetricMethod::PASSES,
        "the stopped collection publishes the method it settled to (FR-017)");
  check(result.passCount == standing,
        "the published pass count names the passes whose folds stand, and the "
        "plan the run was given stays unpublished (FR-017, E-07)");
  check(!result.passSpread.has_value(),
        "a partial set of passes publishes no spread (FR-017)");
  for (const auto& row : result.rows) {
    for (const sg::MetricValue& value : row.metrics) {
      check(
          !value.pass.has_value(),
          "no value carries a pass tag, because nothing was merged " "(FR-"
                                                                     "019)");
    }
  }

  // The console surface: a stopped run prints its reason line alone, so no
  // field of the pass disclosure reaches it (FR-031).
  check(printed.find("SKIPPED:") != std::string::npos,
        "the console states the stop and its reason");
  check(printed.find("count=") == std::string::npos
            && printed.find("spread=") == std::string::npos
            && printed.find("pass=") == std::string::npos,
        "the console of a stopped run prints no disclosure line, no spread and "
        "no pass tag (FR-019, FR-021)");

  // The document surface, written beside the ledger the run retained.
  sg::ReportData written;
  written.results = {result};
  written.rowRaw = {runner.rowRaw().back()};
  written.contextRaw = {runner.contextRaw().back()};
  check(sg::detail::writeReport(kReportPath, written),
        "the JSON writer wrote the stopped run (FR-038)");
  const auto read = sg::loadReport(kReportPath);
  check(read.has_value(),
        "the document of the stopped run reads back through the public reader "
        "(FR-038)");
  const sg::BenchmarkResult& documented = read->results.front();
  check(documented.metricMethod == sg::MetricMethod::PASSES
            && documented.passCount == standing,
        "the document carries the settled method and the count of the passes "
        "whose folds stand (FR-017, FR-038)");
  check(!documented.passSpread.has_value(),
        "the document carries no spread for a stopped collection (FR-038)");
  for (const auto& row : documented.rows) {
    for (const sg::MetricValue& value : row.metrics) {
      check(!value.pass.has_value(),
            "the document carries no pass tag for a collection that merged "
            "nothing (FR-019, FR-038)");
    }
  }
}

// FR-017, FR-019, FR-032: the flag raised while the second pass of a
// three-pass plan measures its repetition. Two folds stand when the loop
// leaves, so the run publishes a pass count of 2 where the plan held 3, and
// neither report surface carries a spread or a pass tag.
auto scenarioInterruptInSecondPass() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto whole = sg::registerBenchmark(&bmPlain, "bmThreePassWhole");
  attachThreePassPlan(whole);
  auto stopped =
      sg::registerBenchmark(&bmInterruptSecondWindow, "bmStopInPassOne");
  attachThreePassPlan(stopped);
  sg::detail::expandRegistry();

  checkWholeThreePassPlan(*needInstance("bmThreePassWhole"));

  resetAttempts();
  const auto options = optionsFor(1, 1);
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmStopInPassOne"));
  sg::detail::interruptFlag().store(false);

  check(arrivals.load() == kTwoPassCount,
        "the collection ran one window of pass 0 and one of pass 1 and opened "
        "no window of the third pass (FR-032)");
  check(result.outcome == sg::RunOutcome::SKIPPED,
        "the flag inside the second pass ends the run as a skip (FR-032)");
  check(result.reason == "interrupted by the user (SIGINT)",
        "the stopped collection states the signal as its reason (FR-032)");
  captureThrough(kScratchPath, [&result] { sg::detail::printResult(result); });
  checkStoppedCollection(result, kTwoPassCount, readAll(kScratchPath), runner);

  std::puts("interrupt-in-second-pass: ok");
  return 0;
}

// FR-017, FR-019, FR-031: the body skip inside the second pass of the same
// plan, on that pass's second repetition. The stopped pass has a fold
// standing, so the published count is the 2 the signal leaves as well.
auto scenarioSkipInSecondPass() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto whole = sg::registerBenchmark(&bmPlain, "bmSkipWholePlan");
  attachThreePassPlan(whole);
  auto stopped = sg::registerBenchmark(&bmSkipWindow, "bmSkipInPassOne");
  attachThreePassPlan(stopped);
  sg::detail::expandRegistry();

  checkWholeThreePassPlan(*needInstance("bmSkipWholePlan"));

  // One iteration to a window makes the arrival counter name the windows: 3
  // is the second repetition of pass 1, whose first repetition folded a row.
  skipWindow = 3;
  resetAttempts();
  const auto options = optionsFor(1, 2);
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmSkipInPassOne"));

  check(arrivals.load() == skipWindow + 1,
        "the collection ran both repetitions of both passes it reached and "
        "opened no window of the third pass (FR-031)");
  check(result.outcome == sg::RunOutcome::SKIPPED,
        "the skip inside the second pass ends the run as a skip (FR-031)");
  check(result.reason == "the scripted pass skip",
        "the skip's own reason stands as the run's reason (FR-031)");
  check(result.rows.size() == 1,
        "the published result carries the rows of the stopped pass alone, the "
        "merge of a stopped collection staying unrun (FR-014)");
  check(!result.rows.empty()
            && result.rows.front().metrics[2].availability
                == sg::counters::Availability::COUNTABLE,
        "the row that stands answers from the stopped pass's own folds, the "
        "fold that keeps that pass in the published count (FR-014, FR-017)");
  captureThrough(kScratchPath, [&result] { sg::detail::printResult(result); });
  checkStoppedCollection(result, kTwoPassCount, readAll(kScratchPath), runner);

  std::puts("skip-in-second-pass: ok");
  return 0;
}

// FR-017: the same skip on the first repetition of the second pass, where the
// stopped pass folds nothing. The pass before it stands alone in the published
// count, and the method stays the one the run settled to.
auto scenarioSkipStopsSecondPassFold() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto stopped = sg::registerBenchmark(&bmSkipWindow, "bmSkipPassOneFold");
  attachThreePassPlan(stopped);
  sg::detail::expandRegistry();

  skipWindow = 1;
  resetAttempts();
  const auto options = optionsFor(1, 1);
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmSkipPassOneFold"));

  check(result.rows.empty(),
        "the stopped pass folded no row of its own, and the rows of the pass "
        "before it stay in the collection that closed (FR-014)");
  check(result.metricMethod == sg::MetricMethod::PASSES,
        "the run publishes the method it settled to (FR-017)");
  check(result.passCount == 1,
        "only the pass that closed stands in the published count, the pass "
        "that folded nothing staying out of it (FR-017, E-07)");
  check(!result.passSpread.has_value(),
        "a partial set of passes publishes no spread (FR-017)");

  std::puts("skip-stops-second-pass-fold: ok");
  return 0;
}

// specs/021 FR-016 beside FR-014 of specs/024: a percentile run over a split
// stands one percentile slot beside every column the row prints, which is
// wider than the columns the planner costed. The merge pads the plan it reads
// so those slots answer, and every slot past the two time slots of the row
// states the pass that measured its column (FR-015, E-05). The pass tags of
// the row below state the plan this run ran: the {0, 0, 1, 1} of four columns
// over the scripted limit of 4, the first two columns measured by pass 0 and
// the last two by pass 1. A slot read from a pass that measured neither the
// column nor its chunks states no figure at all, so every surface of the
// merged slot is read here: the result, the printed report, the document and
// the chunk columns the run retained beside the row.
auto scenarioPercentilesAcrossPasses() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);
  scriptPercentileClock();

  auto wide = sg::registerBenchmark(&bmPlain, "bmPercentileSplit");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  sg::detail::RunOptions options = optionsFor(kPercentileIterations, 1);
  options.showPercentiles = true;
  options.percentilePoints = {kPoint50, kPoint90};
  const LedgerAccount merged =
      runMeasuredLedger(*needInstance("bmPercentileSplit"), options);
  const sg::BenchmarkResult& result = merged.run.result;
  const sg::ResultRow& row = result.rows.front();

  check(result.metricLabels.size() == 4,
        "the widened run labels every attached column (FR-001)");
  check(row.percentiles.size() == kTimeSlotCount + result.metricLabels.size(),
        "a percentile row stands one slot beside the time columns and every "
        "metric column, wider than the plan the planner costed (FR-016, "
        "SC-001)");
  for (std::size_t index = 0; index < 4; ++index) {
    check(row.metrics[index].availability
              == sg::counters::Availability::COUNTABLE,
          "every column stays countable through the widened merge (FR-014)");
    const std::optional<std::size_t> expectedPass = index < 2
        ? std::optional<std::size_t> {0}
        : std::optional<std::size_t> {1};
    check(row.metrics[index].pass == expectedPass,
          "every column keeps the pass that measured it through the widened "
          "merge (FR-019)");
  }
  check(result.passCount == kTwoPassCount,
        "the widened run still publishes its two passes (E-07)");

  // The chunk plan of the arm, pinned so the claims below read answers and not
  // the absence of them: one iteration to a chunk, so the row holds twelve
  // chunks and both points reach their least sample count (FR-006, FR-013).
  check(row.chunkIterations == 1,
        "the chunk plan gives one iteration to a chunk in every pass of the "
        "run (FR-006)");
  check(row.chunkCount == kPercentileIterations,
        "the merged row holds one chunk per iteration of its run (FR-006)");
  for (std::size_t point = 0; point < kPercentilePoints.size(); ++point) {
    check(result.percentilePoints.size() == kPercentilePoints.size()
              && closeTo(result.percentilePoints[point],
                         kPercentilePoints[point].first),
          "the row reports the points the arm asked for, the order its printed "
          "tokens spell them in (FR-010, FR-011)");
  }

  // The printed report of the merged result, and the document written beside
  // it with the chunk columns the run retained.
  captureThrough(kScratchPath, [&result] { sg::detail::printResult(result); });
  const std::vector<std::string> printed =
      percentileLines(readAll(kScratchPath));
  check(printed.size() == row.percentiles.size(),
        "the report prints one percentile line per slot of the merged row, the "
        "slots of the columns the plan never costed included (FR-010, SC-001)");
  sg::ReportData written;
  written.results = {result};
  written.rowRaw = {merged.rowRaw};
  written.contextRaw = {merged.contextRaw};
  check(sg::detail::writeReport(kReportPath, written),
        "the JSON writer wrote the widened run (FR-030)");
  const auto read = sg::loadReport(kReportPath);
  check(read.has_value(),
        "the document of the widened run reads back through the public reader "
        "(FR-024)");
  const sg::ResultRow& documented = read->results.front().rows.front();
  const sg::RowRaw& documentedRaw = read->rowRaw.front().front();

  for (std::size_t index = 0; index < result.metricLabels.size(); ++index) {
    const std::size_t slot = kTimeSlotCount + index;
    // The figure one chunk of the column owes: the column's leaves over the one
    // iteration of the chunk, the per-iteration rule dividing by that one
    // iteration. It is the figure of the column whoever measured it, and the
    // pass that did not measure it holds no chunk of it at all.
    const double owed = expectedColumnValue(index, kPlainSteps, 1);
    check(row.percentiles[slot].size() == kPercentilePoints.size(),
          "the merged slot carries one value per point of the run (FR-010)");
    // The surface of the result value.
    for (std::size_t point = 0; point < kPercentilePoints.size(); ++point) {
      check(row.percentiles[slot][point].available,
            "the merged slot of a measured column answers, so it cannot state "
            "the empty distribution of a pass that never sampled the column "
            "(FR-015, E-05)");
      check(closeTo(row.percentiles[slot][point].value, owed),
            "the merged slot states the chunk figure of its own column "
            "(FR-015, FR-016)");
      // The surface of the printed report.
      const std::string& line = printed[slot];
      check(line.find(result.metricLabels[index]) != std::string::npos,
            "the printed line of the slot names the column of the slot "
            "(FR-010)");
      check(line.find("not available") == std::string::npos,
            "the printed line of the slot answers, where the pass of the copy "
            "had nothing to answer with (FR-015)");
      const auto printedValue =
          printedPoint(line, kPercentilePoints[point].second);
      check(printedValue.has_value() && closeTo(*printedValue, owed),
            "the printed point of the slot states the figure of its own column "
            "(FR-015, FR-016)");
      // The surface of the document.
      check(documented.percentiles[slot][point].available
                && closeTo(documented.percentiles[slot][point].value, owed),
            "the document carries the same answer for the same slot and point "
            "as the merged result (FR-015, FR-022)");
    }
    // The surface of the chunk columns the run retained beside the row: the
    // remap pulls each column's chunks out of the ledger of the pass that
    // measured it, where the reference pass left an empty column (FR-029).
    const auto& chunks = merged.rowRaw.front().columns[slot];
    check(chunks.size() == kPercentileIterations,
          "the retained chunk column of the slot holds the chunks of the pass "
          "that measured the column (FR-029, FR-015)");
    for (const double chunk : chunks) {
      check(closeTo(chunk, owed),
            "the retained chunk of the slot states the figure of its own "
            "column (FR-029)");
    }
    check(documentedRaw.columns[slot].size() == chunks.size(),
          "the document carries the chunk column of the slot beside the row "
          "(FR-029)");
    for (const double chunk : documentedRaw.columns[slot]) {
      check(closeTo(chunk, owed),
            "the chunk column of the document states the figure of its own "
            "column (FR-029, FR-022)");
    }
  }

  std::puts("percentiles-across-passes: ok");
  return 0;
}

// The user counter of the stored-counter arm, and the step its value carries.
// The value of a store names the window that stored it, so the two passes of a
// split store two different figures and the merged column states which of them
// the run publishes.
constexpr std::string_view kCounterName {"widgets"};
constexpr double kCounterStep = 64.0;

// The body of the stored-counter arm: the plain loop beside one `setCounter`
// call, made once per window at its first iteration, so the arrival counter
// names the window the store belongs to.
auto bmStoredCounter(sg::State& state) -> void
{
  for (auto index : state) {
    if (index == 0) {
      state.setCounter(
          kCounterName,
          {kCounterStep * static_cast<double>(arrivals.fetch_add(1) + 1)});
    }
    sg::doNotOptimize(index);
  }
}

// FR-014, FR-016: the merge of a split whose body stores a user counter. The
// planner costs the metric columns before the run opens a counter name, so the
// widest fold row of this run stands one column wider than the plan the merge
// indexes. The merge reads the plan at the index of that column, so the padding
// of the plan is what keeps the read inside it: the padding writes 0 at the new
// slot, which names the reference pass, and the row below publishes the figure
// the reference pass stored beside the figures of the passes that measured the
// metric columns.
auto scenarioCounterColumnAcrossPasses() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmStoredCounter, "bmCounterSplit");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  resetAttempts();
  const RunAccount widened = runMeasured(*needInstance("bmCounterSplit"),
                                         optionsFor(kValueIterations, 1));
  const sg::BenchmarkResult& result = widened.result;
  const sg::ResultRow& row = result.rows.front();

  check(arrivals.load() == kTwoPassCount,
        "the four columns split across two passes and each pass opened one "
        "window, so the two stores of the counter belong one to each pass "
        "(FR-009)");
  check(result.metricLabels.size() == 4 && row.counters.size() == 1,
        "the widest fold row stands five wide against the four columns the "
        "planner costed: the counter name the body opened is the column past "
        "the last one the plan names (FR-014)");
  check(result.counterLabels.size() == 1
            && result.counterLabels.front() == kCounterName,
        "the merged result publishes the counter name the body opened, in the "
        "order the name opened (FR-006)");
  // The window of pass 0 stored `kCounterStep` and the window of pass 1 stored
  // twice that figure. The first window of a fixed-iteration split belongs to
  // pass 0, which `window-lifetime` states, so this figure can only be the one
  // the reference pass stored.
  check(closeTo(row.counters.front(), kCounterStep),
        "the merged counter column carries the figure the reference pass "
        "stored, where the pass after it stored twice that (FR-014, E-05)");

  // The columns the plan did cost keep their own measuring pass beside the
  // column it never costed (FR-014, FR-019).
  checkColumnsAtFullRatio(row, {0, 1, 2, 3}, kPlainSteps, kValueIterations);
  for (std::size_t index = 0; index < 4; ++index) {
    const std::optional<std::size_t> expectedPass = index < 2
        ? std::optional<std::size_t> {0}
        : std::optional<std::size_t> {1};
    check(row.metrics[index].pass == expectedPass,
          "the counter column of the padded slot leaves every metric column on "
          "the pass that measured it (FR-014, FR-019)");
  }
  check(closeTo(row.timePerIterationNs,
                expectedTimePerIteration(kPlainSteps, kValueIterations)),
        "the time columns of the widened row still read the reference pass "
        "(FR-015)");

  // The fields the run publishes are the plan it ran, the padded slot being a
  // field of that plan and no part of the settlement (FR-017, E-07).
  check(result.metricMethod == sg::MetricMethod::PASSES
            && result.passCount == kTwoPassCount,
        "the widened run publishes the method it settled to and the two passes "
        "it ran (FR-017, E-07)");

  std::puts("counters-across-passes: ok");
  return 0;
}

// FR-014 beside FR-010: the dry run of a split collection keeps the shape of
// the audit point, one iteration, one repetition and no warm-up, so the ahead
// calibration stands aside while the split still runs every pass.
auto scenarioDryRunAcrossPasses() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

  auto wide = sg::registerBenchmark(&bmPlain, "bmDryRunSplit");
  attachColumns(wide, {0, 1, 2, 3});
  sg::detail::expandRegistry();

  sg::detail::RunOptions options;
  options.repetitions = 2;
  options.dryRun = true;
  sg::detail::Runner runner(options);
  const sg::BenchmarkResult result = runner.run(*needInstance("bmDryRunSplit"));

  check(result.outcome == sg::RunOutcome::MEASURED,
        "the dry run of a split collection measures (FR-014)");
  check(result.passCount == kTwoPassCount,
        "the dry run still splits its columns across the passes (FR-009)");
  std::size_t measured = 0;
  for (const auto& row : result.rows) {
    if (row.kind != sg::RowKind::REPETITION) {
      continue;
    }
    check(row.iterations == 1,
          "a dry run measures the one iteration of the audit point (FR-014)");
    ++measured;
  }
  check(
      measured == 1,
      "a dry run publishes the one repetition the audit point takes (FR-014)");
  check(result.rows.front().metrics.back().availability
            == sg::counters::Availability::COUNTABLE,
        "the column of the second pass carries a value through the dry-run "
        "merge " "(FR-014, FR-019)");

  std::puts("dry-run-across-passes: ok");
  return 0;
}

struct Scenario
{
  const char* name;
  int (*run)();
};

constexpr std::array<Scenario, 21> kScenarios {{
    Scenario {"two-pass-run", &scenarioTwoPassRun},
    Scenario {"pause-across-passes", &scenarioPauseAcrossPasses},
    Scenario {"over-limit-run", &scenarioOverLimitRun},
    Scenario {"window-lifetime", &scenarioWindowSpan},
    Scenario {"threaded-pass-order", &scenarioThreadedPassOrder},
    Scenario {"multiplex-run", &scenarioMultiplexRun},
    Scenario {"no-limit-host", &scenarioNoLimitHost},
    Scenario {"selector-one-pass", &scenarioSelectorOnePass},
    Scenario {"calibration-ahead-fails", &scenarioCalibrationAheadFails},
    Scenario {"calibration-ahead-skipped", &scenarioCalibrationAheadSkipped},
    Scenario {"calibration-ahead-interrupted",
              &scenarioCalibrationAheadInterrupted},
    Scenario {"calibration-ahead-grows", &scenarioCalibrationAheadGrows},
    Scenario {"calibration-ahead-signal", &scenarioCalibrationAheadSignal},
    Scenario {"unplanned-columns-across-passes",
              &scenarioUnplannedColumnsAcrossPasses},
    Scenario {"interrupt-inside-split", &scenarioInterruptInsideSplit},
    Scenario {"interrupt-in-second-pass", &scenarioInterruptInSecondPass},
    Scenario {"skip-in-second-pass", &scenarioSkipInSecondPass},
    Scenario {"skip-stops-second-pass-fold", &scenarioSkipStopsSecondPassFold},
    Scenario {"percentiles-across-passes", &scenarioPercentilesAcrossPasses},
    Scenario {"counters-across-passes", &scenarioCounterColumnAcrossPasses},
    Scenario {"dry-run-across-passes", &scenarioDryRunAcrossPasses},
}};

}  // namespace

auto main(int argc, char** argv) -> int
{
  // The command line of the child is built out of `std::string` and `const
  // char*` only, the spelling `harness_pause_test.cpp` uses: a `std::string`
  // built from a `std::string_view` whose length the optimizer cannot bound
  // trips GCC's `-Wrestrict` on the copy inside that constructor.
  const std::string self =
      argc > 0 && argv[0] != nullptr ? argv[0] : "harness_pass_collect_test";
  const std::string requested =
      argc > 1 && argv[1] != nullptr ? argv[1] : std::string();
  for (const auto& scenario : kScenarios) {
    if (requested == scenario.name) {
      return scenario.run();
    }
  }
  if (!requested.empty()) {
    std::fprintf(stderr,
                 "harness_pass_collect_test: unknown scenario '%s'\n",
                 requested.c_str());
    return 2;
  }

  for (const auto& scenario : kScenarios) {
    const std::string command = "\"" + self + "\" " + scenario.name;
    const int status = std::system(command.c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the scenario of the in-main list passes");
  }

  std::puts("harness_pass_collect_test: ok");
  return 0;
}
