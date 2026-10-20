// ============================================================================
// The pass collection runs (specs/024-multipass-collection T022 to T025; US1;
// FR-008 to FR-016, FR-034; SC-01, SC-09, SC-10; research D-06, D-10).
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
// Each scenario scripts its own provider, and the counters system takes one
// provider per path per process, so each scenario runs in its own process:
// the parent below walks the scenario list and re-executes this binary once
// per entry, the pattern of `harness_pause_test.cpp`. The list is what CTest
// runs, so a scenario absent from it never runs.
// ============================================================================

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/wait.h>

#include "detail/internal.hpp"
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
auto scenarioThreadedPassOrder() -> int
{
  registerFixtureProvider(sg::test::kHostPassLimit);

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

  std::puts("threaded-pass-order: ok");
  return 0;
}

struct Scenario
{
  const char* name;
  int (*run)();
};

constexpr std::array<Scenario, 5> kScenarios {{
    Scenario {"two-pass-run", &scenarioTwoPassRun},
    Scenario {"pause-across-passes", &scenarioPauseAcrossPasses},
    Scenario {"over-limit-run", &scenarioOverLimitRun},
    Scenario {"window-lifetime", &scenarioWindowSpan},
    Scenario {"threaded-pass-order", &scenarioThreadedPassOrder},
}};

}  // namespace

auto main(int argc, char** argv) -> int
{
  // The command line of the child is built out of `std::string` and `const
  // char*` only, the spelling `harness_pause_test.cpp` uses: a `std::string`
  // built from a `std::string_view` whose length the optimiser cannot bound
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
