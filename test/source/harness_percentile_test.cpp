// ============================================================================
// TDD test for the percentile reads (specs/021 T012; FR-011, FR-013, FR-024).
//
// The rank rule, the availability bound and the point list are pure reads
// over scripted columns of chunk values, so the cases stand without a run:
// the nearest rank of FR-011, the least sample count of FR-013, the default
// point list and the sorted, duplicate-free list of FR-024, and the
// not-available answer of FR-016 for a column whose fold disclosed a gap.
// The chunk windows themselves are scripted by the run-level suites, which
// drive the fake provider and read the report. Frameworkless check()/fail()
// convention.
// ============================================================================

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/percentiles.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"

// GCC answers `__SANITIZE_THREAD__`; Clang answers `__has_feature`. The thread
// sanitizer runtime defines the replaceable allocation operators itself, so a
// test binary that defines them cannot link there (the pattern of
// harness_execution_context_test.cpp, which keeps its target in the ci-tsan
// tree at the preprocessor level).
#if defined(__SANITIZE_THREAD__)
#  define SG_TEST_HAS_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define SG_TEST_HAS_TSAN 1
#  else
#    define SG_TEST_HAS_TSAN 0
#  endif
#else
#  define SG_TEST_HAS_TSAN 0
#endif

// SC-004: the allocation counter of the capture path. The test binary
// replaces the global operators so a run can state what its window cost
// in allocations.
#if !SG_TEST_HAS_TSAN
static std::atomic<std::uint64_t> allocations {0};

// The count one benchmark body takes between the opening of its timed
// window and the close of its last iteration (SC-004).
static std::atomic<std::uint64_t> windowAllocations {0};

auto operator new(std::size_t size) -> void*
{
  ++allocations;
  return std::malloc(size);
}

auto operator new[](std::size_t size) -> void*
{
  ++allocations;
  return std::malloc(size);
}

auto operator delete(void* pointer) noexcept -> void
{
  std::free(pointer);
}

auto operator delete(void* pointer, std::size_t) noexcept -> void
{
  std::free(pointer);
}

auto operator delete[](void* pointer) noexcept -> void
{
  std::free(pointer);
}

auto operator delete[](void* pointer, std::size_t) noexcept -> void
{
  std::free(pointer);
}
#endif  // !SG_TEST_HAS_TSAN

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PERCENTILE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The reported value is selected from the column by rank; no arithmetic
// forms it, and
// -Werror=float-equal rules out `==` on a double (the house convention).
auto sameValue(const double actual, const double expected) -> bool
{
  return std::abs(actual - expected) <= 1e-9 + 1e-9 * std::abs(expected);
}

// FR-013: the least chunk count that can answer a point is
// ceil(100 / (100 - p)).
auto leastSampleCountOfEachPoint() -> void
{
  check(sg::detail::minSamplesFor(50.0) == 2, "p50 needs two chunks");
  check(sg::detail::minSamplesFor(90.0) == 10, "p90 needs ten chunks");
  check(sg::detail::minSamplesFor(99.0) == 100, "p99 needs a hundred chunks");
  check(sg::detail::minSamplesFor(99.9) == 1000, "p99.9 needs a thousand");
  check(sg::detail::minSamplesFor(1.0) == 2, "p1 needs two chunks");
  check(sg::detail::minSamplesFor(99.99) == 10000, "p99.99 needs ten thousand");
}

// FR-011, FR-013: the nearest rank names the value at ceil(p * n / 100),
// 1-based, and answers only once the column holds the least count.
auto nearestRankOfASortedColumn() -> void
{
  const std::vector<double> empty;
  check(!sg::detail::percentileAt(empty, 50.0).available,
        "an empty column answers nothing");

  const std::vector<double> one {7.0};
  check(!sg::detail::percentileAt(one, 50.0).available,
        "one chunk cannot answer p50");

  const std::vector<double> two {3.0, 9.0};
  const sg::PercentileValue p50 = sg::detail::percentileAt(two, 50.0);
  check(p50.available && sameValue(p50.value, 3.0),
        "p50 of two names the lower rank");

  const std::vector<double> four {1.0, 2.0, 3.0, 4.0};
  const sg::PercentileValue mid = sg::detail::percentileAt(four, 50.0);
  check(mid.available && sameValue(mid.value, 2.0),
        "p50 of four names rank two");

  const std::vector<double> ten {
      1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  check(!sg::detail::percentileAt(ten, 99.0).available,
        "ten chunks cannot answer p99");
  const sg::PercentileValue high = sg::detail::percentileAt(ten, 90.0);
  check(high.available && sameValue(high.value, 9.0),
        "p90 of ten names rank nine");
  const sg::PercentileValue top = sg::detail::percentileAt(ten, 99.9);
  check(!top.available, "ten chunks cannot answer p99.9");
}

// FR-024: an empty request answers with the default points, and a stated
// list comes back sorted with its duplicates collapsed.
auto pointListDefaultsAndNormalizes() -> void
{
  const std::vector<double> defaults =
      sg::detail::normalizePercentilePoints({});
  check(defaults.size() == 4 && sameValue(defaults[0], 50.0)
            && sameValue(defaults[1], 90.0) && sameValue(defaults[2], 99.0)
            && sameValue(defaults[3], 99.9),
        "the empty request answers with the four default points");

  const std::vector<double> stated =
      sg::detail::normalizePercentilePoints({99.0, 10.0, 99.0, 50.0});
  check(stated.size() == 3 && sameValue(stated[0], 10.0)
            && sameValue(stated[1], 50.0) && sameValue(stated[2], 99.0),
        "the stated list comes back sorted and duplicate-free");
}

// FR-010, FR-016: one output column per input column, one value per point,
// and a column that disclosed a gap answers not available everywhere.
auto columnsAnswerPerPoint() -> void
{
  const std::vector<double> points {50.0, 90.0};
  const std::vector<std::vector<double>> columns {
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0},
      {1.0, 2.0},
  };
  const std::vector<bool> countable {true, true};
  const auto built =
      sg::detail::buildPercentileColumns(columns, points, countable);
  check(built.size() == 2, "one output column per input column");
  check(built[0].size() == 2 && built[1].size() == 2,
        "one value per reported point");
  check(built[0][0].available && sameValue(built[0][0].value, 5.0),
        "the wide column answers p50 with rank five");
  check(built[0][1].available && sameValue(built[0][1].value, 9.0),
        "the wide column answers p90 with rank nine");
  check(built[1][0].available && sameValue(built[1][0].value, 1.0),
        "the narrow column answers p50 with its first value");
  check(!built[1][1].available, "the narrow column answers no p90");

  const std::vector<bool> gapped {true, false};
  const auto gapBuilt =
      sg::detail::buildPercentileColumns(columns, points, gapped);
  check(gapBuilt[0][0].available && sameValue(gapBuilt[0][0].value, 5.0),
        "a countable column keeps its answer beside a gapped one");
  check(!gapBuilt[1][0].available,
        "the column whose fold disclosed a gap answers nothing");
  const std::vector<bool> allGapped {false, false};
  const auto allBuilt =
      sg::detail::buildPercentileColumns(columns, points, allGapped);
  check(!allBuilt[0][0].available && !allBuilt[0][1].available,
        "a gapped column answers not available for every point");
}

// The scripted window of FR-001 and FR-010. The plan's own sampling-cost
// calibration reads the machine leaves a fixed count of times before the
// benchmark's window opens, so the script carries that prefix and then the
// deltas the assertions read.
constexpr std::uint64_t kCalibrationReads = 321;
constexpr std::uint64_t kCalibrationStep = 1'000;
constexpr std::uint64_t kWindowIterations = 4;

// The four chunk deltas of the scripted window, in the order the run
// closes them. Sorted, the time column reads 1, 2, 3, 4 million and the
// cpu column 10, 20, 30, 40 million, so the two columns answer different
// values.
constexpr std::array<std::uint64_t, 4> kTimeDeltas {
    1'000'000, 4'000'000, 2'000'000, 3'000'000};
constexpr std::array<std::uint64_t, 4> kCpuDeltas {
    10'000'000, 40'000'000, 20'000'000, 30'000'000};
constexpr std::array<std::uint64_t, 4> kMetricDeltas {10, 40, 20, 30};

auto windowPoints(const std::array<std::uint64_t, 4>& deltas)
    -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> points;
  points.reserve(kCalibrationReads + kWindowIterations + 1);
  for (std::uint64_t read = 0; read < kCalibrationReads; ++read) {
    points.push_back(read * kCalibrationStep);
  }
  std::uint64_t running = (kCalibrationReads - 1) * kCalibrationStep;
  points.push_back(running);
  for (const std::uint64_t delta : deltas) {
    running += delta;
    points.push_back(running);
  }
  return points;
}

auto bmWindow(sg::State& state) -> void
{
  // SC-004: the first read stands inside the timed window after the
  // recorder minted its reservation, and the second after the body of the
  // last iteration returns. What the window takes in between is the cost
  // of the capture path itself.
#if !SG_TEST_HAS_TSAN
  const std::uint64_t opened = allocations.load();
  for (auto _ : state) {
  }
  windowAllocations.store(allocations.load() - opened);
#else
  for (auto _ : state) {
  }
#endif
}

// The expanded instance of one name, or null when the expansion left no
// such instance.
auto findInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  return nullptr;
}

auto needInstance(const std::string& name) -> sg::Instance*
{
  sg::Instance* const found = findInstance(name);
  check(found != nullptr, "the expansion produced the named instance");
  return found;
}

auto printedShapeFollowsTheOutputSwitch(const sg::BenchmarkResult& on,
                                        const sg::BenchmarkResult& off) -> void;
auto pausedRunWithPercentilesClosesInsideItsReservation() -> void;
auto captureReservesItsPointsAndAllocatesNothingPerChunk() -> void;

// T012, T016, T027, T028: the run-level oracle over a scripted window
// (FR-001 to FR-005, FR-010, FR-014, FR-016, SC-001, SC-010, D-14).
auto scriptedRunAnswersTheRankOfTheWindow() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", windowPoints(kTimeDeltas));
  provider.setPoints("machine", "thread_cpu", windowPoints(kCpuDeltas));

  provider.addObject("scripted", "scripted", "scripted metric object");
  provider.addCounter("scripted", "work", "none", "scripted work");
  provider.setPoints("scripted", "work", windowPoints(kMetricDeltas));
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmWindow, "bmWindow");
  const auto object = sg::counters::System::local().object("scripted");
  check(object.has_value(), "the scripted object resolves");
  const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
  check(work.has_value(), "the scripted leaf resolves");
  handle.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*work),
                   "work");

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  options.showPercentiles = true;
  options.percentilePoints = {50.0, 90.0};
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmWindow");
  check(instance != nullptr, "the expansion produced the registered instance");
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.rows.size() == 1, "the run produces one measured row");
  const sg::ResultRow& row = result.rows[0];
  // FR-005, FR-017: the iteration time of the window stands far above the
  // cost of one sample, so the plan gives one iteration to the chunk and
  // the row discloses the plan it ran.
  check(row.chunkIterations == 1 && row.chunkCount == kWindowIterations,
        "the row discloses the chunk plan the run used");
  check(row.percentiles.size() == 3,
        "one percentile column per reported column (FR-016, SC-001)");
  // SC-001: the printed value is the nearest rank of the window's own
  // chunk values, computed here from the scripted deltas. The default
  // primary is the cpu fold (D-05), so the first column answers the cpu
  // deltas and the second the real ones.
  check(row.percentiles[0][0].available
            && sameValue(row.percentiles[0][0].value, 20'000'000.0),
        "the primary column answers p50 with rank two of the window");
  check(row.percentiles[1][0].available
            && sameValue(row.percentiles[1][0].value, 2'000'000.0),
        "the time column the primary is not answers from its own values");
  check(!row.percentiles[0][1].available, "four chunks answer no p90 (FR-013)");
  check(row.percentiles[2][0].available
            && sameValue(row.percentiles[2][0].value, 20.0),
        "the metric column answers from its own values (FR-016)");
  check(!row.percentiles[2][1].available,
        "the metric column answers no p90 either");

  // SC-010, D-14: the attempt recorder is gone by the time run() returns,
  // and the columns of the finished run stay readable on the row.
  check(row.percentiles[0][0].available
            && sameValue(row.percentiles[0][0].value, 20'000'000.0),
        "the columns stay readable after the recorder is gone");

  // FR-023, SC-002: with the output off the row carries no percentile
  // column and no chunk plan.
  sg::detail::RunOptions off;
  off.fixedIterations = kWindowIterations;
  off.showPercentiles = false;
  sg::detail::Runner offRunner(off);
  const sg::BenchmarkResult offResult = offRunner.run(*instance);
  check(offResult.rows.size() == 1, "the off run produces one measured row");
  check(offResult.rows[0].percentiles.empty()
            && offResult.rows[0].chunkCount == 0,
        "the run that reports nothing carries no percentile column");

  printedShapeFollowsTheOutputSwitch(result, offResult);
}

// The report text one result prints, captured by redirecting standard
// output around the print.
auto capturePrint(const sg::BenchmarkResult& result) -> std::string
{
  const char* const path = "harness_percentile_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  sg::detail::printResult(result);
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// T015: the printed shape of a run with the output off (FR-018, FR-023,
// SC-002).
auto printedShapeFollowsTheOutputSwitch(const sg::BenchmarkResult& on,
                                        const sg::BenchmarkResult& off) -> void
{
  const std::string onReport = capturePrint(on);
  const std::string offReport = capturePrint(off);

  check(onReport.find("  chunks  chunks=") != std::string::npos,
        "a run that reports percentiles discloses its chunk plan");
  check(onReport.find("  percentile  primary time/iter (ns)  p50=")
            != std::string::npos,
        "the percentile line names the primary column and its points");
  check(onReport.find("real time/iter (ns)  p50=") != std::string::npos,
        "the second line carries the label of the time column the primary "
        "is not (SC-001)");
  check(offReport.find("chunks=") == std::string::npos
            && offReport.find("percentile") == std::string::npos,
        "the run with the output off prints no chunk line and no percentile "
        "line (FR-023, SC-002)");
  for (const char* const field : {"iterations=",
                                  "real time/iter (ns)=",
                                  "cpu time/iter (ns)=",
                                  "overhead floor (ns)=",
                                  "endpoint samples="})
  {
    check(offReport.find(field) != std::string::npos,
          "the run row keeps every field it carries today");
  }
  // FR-018: the count of the new cadence is the endpoint pair plus one
  // point for each chunk.
  check(offReport.find("endpoint samples=2") != std::string::npos,
        "a run with no chunk owns the two endpoint samples");
  check(onReport.find("endpoint samples=5") != std::string::npos,
        "four chunks and the endpoint give five points (FR-018)");
}

// T007: the window of a run that pauses once per iteration. The plan
// reserves two points per pause pair per iteration, so the scripted column
// carries the pair of every chunk. The pairs of chunks one, three and four
// hold no time at all, and the pair of chunk two holds five million, so the
// four chunk values read 2, 3, 4 and 5 million with the intervals netted
// out, and 2, 8, 4 and 5 million without them (FR-015).
constexpr std::array<std::uint64_t, 13> kPausedWindow {0,
                                                       1'000'000,
                                                       1'000'000,
                                                       2'000'000,
                                                       3'000'000,
                                                       8'000'000,
                                                       10'000'000,
                                                       11'000'000,
                                                       11'000'000,
                                                       14'000'000,
                                                       15'000'000,
                                                       15'000'000,
                                                       19'000'000};

auto windowPointsAfter(const std::array<std::uint64_t, 13>& points)
    -> std::vector<std::uint64_t>
{
  // The first run of the family teaches it the pause history the next run
  // reserves room from, so the scripted column carries the cost calibration
  // of both runs, the window of the first, and the window of the run the
  // assertions read.
  const std::size_t prefix = 2 * kCalibrationReads + points.size();
  std::vector<std::uint64_t> scripted;
  scripted.reserve(prefix + points.size());
  for (std::size_t read = 0; read < prefix; ++read) {
    scripted.push_back(read * kCalibrationStep);
  }
  scripted.insert(scripted.end(), points.begin(), points.end());
  return scripted;
}

auto bmPausedWindow(sg::State& state) -> void
{
  for (auto _ : state) {
    state.pauseTiming();
    state.resumeTiming();
  }
}

// T007: the fold of the scripted column nets each pause interval into the
// one chunk whose window holds it, and into no other (FR-015). The point
// column is scripted straight into a recorder, so the case stands in every
// build configuration; the run-level case drives the same fold through a
// real run.
auto chunkFoldNetsEachPauseIntoItsOwnChunk() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.setPoints(
      "machine",
      "monotonic",
      std::vector<std::uint64_t>(kPausedWindow.begin(), kPausedWindow.end()));
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  const auto object = *sg::counters::System::local().object("machine");
  const sg::counters::Expression<sg::counters::Dim<1, 0>> clock =
      *object.counter<sg::counters::Dim<1, 0>>("monotonic");
  auto compiled = sg::counters::compile(sg::counters::System::local(), clock);
  check(compiled.has_value(), "the fold fixture compiles its clock expression");
  auto recorder = compiled->recorder(kPausedWindow.size());
  for (std::size_t point = 0; point < kPausedWindow.size(); ++point) {
    recorder.sample();
  }
  check(recorder.count() == kPausedWindow.size(),
        "the fixture recorder holds every scripted point");

  const std::vector<std::pair<std::uint64_t, std::uint64_t>> pairs {
      {1, 2}, {4, 5}, {7, 8}, {10, 11}};
  const auto netted =
      sg::detail::buildChunkValues(clock.core, recorder.view(), 0, 4, pairs);
  check(netted.size() == 4, "the fold answers one value per chunk (FR-002)");
  check(sameValue(netted[0], 2'000'000.0) && sameValue(netted[1], 3'000'000.0)
            && sameValue(netted[2], 4'000'000.0)
            && sameValue(netted[3], 5'000'000.0),
        "the four chunk values read the scripted column with each pause "
        "netted into its own chunk (FR-002, FR-015)");
  // The same four windows read 2, 8, 4 and 5 million straight from the
  // recorder, so the one value that moves is the chunk whose window holds
  // the five million of its pause pair.
  const auto& api = recorder.view();
  const double window0 =
      sg::counters::detail::foldCore(clock.core, api, 0, 3).value;
  const double window1 =
      sg::counters::detail::foldCore(clock.core, api, 3, 6).value;
  const double window2 =
      sg::counters::detail::foldCore(clock.core, api, 6, 9).value;
  const double window3 =
      sg::counters::detail::foldCore(clock.core, api, 9, 12).value;
  check(sameValue(window1, 8'000'000.0),
        "the scripted window of chunk two holds its pause interval");
  check(sameValue(netted[1], window1 - 5'000'000.0),
        "the pause interval nets out of the chunk that holds it (FR-015)");
  check(sameValue(netted[0], window0) && sameValue(netted[2], window2)
            && sameValue(netted[3], window3),
        "the pause interval leaves the other three chunks alone (FR-015)");
}

// T007, FR-027: a run that pauses inside every iteration and reports
// percentiles keeps its pause points inside the reservation its history
// earned, so it closes without a capacity violation and still discloses
// the plan it ran.
auto pausedRunWithPercentilesClosesInsideItsReservation() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, kCalibrationStep);
  provider.setPoints("machine", "thread_cpu", windowPointsAfter(kPausedWindow));
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmPausedWindow, "bmPausedWin");
  static_cast<void>(handle.showPercentiles(true));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  options.percentilePoints = {25.0, 75.0};
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmPausedWin");
  check(instance != nullptr, "the expansion produced the registered instance");
  // The first run leaves the family its pause history; the run the
  // assertions read reserves the room that history earned.
  static_cast<void>(runner.run(*instance));
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.rows.size() == 1, "the run produces one measured row");
  const sg::ResultRow& row = result.rows[0];
  check(row.chunkCount >= 1 && row.chunkIterations >= 1
            && row.chunkCount * row.chunkIterations >= row.iterations,
        "the paused run discloses a chunk plan that covers its iterations");
  check(capturePrint(result).find("  chunks  chunks=") != std::string::npos,
        "the paused run prints its chunk line (FR-021)");
}

// T011: the capture of a wide chunk plan reserves its points up front and
// takes no allocation per chunk (FR-001, FR-002, FR-006, FR-008, SC-004).
auto captureReservesItsPointsAndAllocatesNothingPerChunk() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, kCalibrationStep);
  provider.setPoints("machine", "thread_cpu", {0}, kCalibrationStep);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmWindow, "bmWide");
  static_cast<void>(handle.showPercentiles(true));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = 300;
  options.percentilePoints = {50.0};
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmWide");
  check(instance != nullptr, "the expansion produced the registered instance");
#if !SG_TEST_HAS_TSAN
  const std::uint64_t before = allocations.load();
  const sg::BenchmarkResult result = runner.run(*instance);
  const std::uint64_t taken = allocations.load() - before;
#else
  const sg::BenchmarkResult result = runner.run(*instance);
#endif

  check(result.rows.size() == 1, "the run produces one measured row");
  const sg::ResultRow& row = result.rows[0];
  check(row.chunkCount == 300 && row.chunkIterations == 1,
        "the wide plan holds one iteration per chunk");
  // FR-002: two consecutive chunks share the point between them, so the
  // column holds one point per boundary plus the exit point, and the run
  // reports the same count it reserved.
  check(row.endpointSamples == row.chunkCount + 1,
        "the run took one point per boundary plus the exit point (FR-002)");
  check(sg::detail::lastRecorderReservation().chunkPoints
            == row.chunkCount + 1,
        "the recorder reserved the whole column before the window opened "
        "(FR-008)");
  // SC-004: the point column stands in the reservation, so the count of
  // allocations a run takes stays far below the count of chunks it closed.
  // Storage reserved as the chunks arrive would answer at the chunk count
  // or above it.
#if SG_TEST_HAS_TSAN
  std::puts("harness_percentile_test window: the thread sanitizer runtime owns "
            "the replaceable allocation operators, so the counting hooks stand "
            "aside there and the run itself still runs (SC-004)");
#else
  // SC-004: the window answers zero. Every point of the column stands in
  // the reservation the recorder minted before the window opened, so the
  // capture path takes no allocation while the run is timed.
  check(windowAllocations.load() == 0,
        "the timed window took no allocation at all (SC-004)");
  // The run outside the window does allocate: the plan, the result and the
  // report. Storage taken as the chunks arrived would answer at the chunk
  // count or above it. The count also answers above zero, which is what
  // makes the window answer of zero a reading and not a dead hook.
  check(taken > 0,
        "the counting hooks answer for this run, so the window zero is a "
        "reading and not a hook that never fired (SC-004)");
  check(taken < row.chunkCount,
        "the run allocated far below one allocation per chunk (FR-008)");
#endif
}

// T037: the columns a percentile reads that are not the two time columns.
// A metric whose dimension is not per-iteration keeps the window value its
// fold reached (FR-022), and a leaf the catalog does not list carries no
// fold and answers nothing (FR-016, FR-023).
auto bmColumned(sg::State& state) -> void
{
  for (std::uint64_t index : state) {
    sg::doNotOptimize(index);
  }
}

// A manual run supplies its own iteration time, so no fold stands behind
// its primary time (FR-006).
auto bmManualTime(sg::State& state) -> void
{
  for (std::uint64_t index : state) {
    state.setIterationTime(1000);
    sg::doNotOptimize(index);
  }
}

auto scriptClocks(sg::counters::FakeProvider& provider) -> void
{
  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, kCalibrationStep);
  provider.setPoints("machine", "thread_cpu", {0}, kCalibrationStep);
}

auto addQuotientMetric(sg::BenchmarkHandle& handle) -> void
{
  const auto object = *sg::counters::System::local().object("machine");
  const auto num = *object.counter<sg::counters::Dim<0, 1>>("num");
  const auto den = *object.counter<sg::counters::Dim<0, 1>>("den");
  static_cast<void>(handle.addMetric(num / den, "num/den"));
}

auto scriptQuotientLeaves(sg::counters::FakeProvider& provider) -> void
{
  provider.addCounter("machine", "num", "none", "numerator");
  provider.addCounter("machine", "den", "none", "denominator");
  provider.setPoints("machine", "num", {0}, 7);
  provider.setPoints("machine", "den", {0}, 3);
}

auto metricAndAbsentColumnsAnswerTheirOwnWay() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  scriptClocks(provider);
  scriptQuotientLeaves(provider);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmColumned, "bmColumns");
  addQuotientMetric(handle);
  static_cast<void>(handle.showPercentiles(true));
  auto quiet = sg::registerBenchmark(&bmColumned, "bmOff");
  static_cast<void>(quiet.showPercentiles(false));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  // The fortified libstdc++ 16 list assignment trips a `-Wnonnull` at -O3
  // over a copy it cannot prove non-null; the append states one point.
  options.percentilePoints.push_back(50.0);
  options.counterAddresses = {"machine/nosuch"};
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmColumns");
  check(instance != nullptr, "the expansion produced the registered instance");
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.rows.size() == 1, "the run produces one measured row");
  const sg::ResultRow& row = result.rows[0];
  // The two time columns, the metric the family attached, and the leaf the
  // catalog does not list.
  check(row.percentiles.size() == 4,
        "the run answers one percentile column per reported column (FR-010)");
  check(row.percentiles[2].size() == 1 && row.percentiles[2][0].available,
        "the metric that is not per-iteration answers its point (FR-022)");
  check(row.percentiles[3].size() == 1 && !row.percentiles[3][0].available,
        "the leaf the catalog does not list answers nothing (FR-016, FR-023)");
  check(capturePrint(result).find("num/den") != std::string::npos,
        "the report names the metric column the percentile reads (FR-010)");

  const sg::BenchmarkResult quietResult = runner.run(*needInstance("bmOff"));
  check(quietResult.percentilePoints.empty(),
        "the opted-out family reports no percentile points (FR-023)");
  check(capturePrint(quietResult).find("chunks=") == std::string::npos,
        "the opted-out run prints no chunk line and no percentile line "
        "(FR-023)");
}

auto manualRunAnswersNoPrimaryPercentile() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  scriptClocks(provider);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmManualTime, "bmManualPct");
  static_cast<void>(handle.useManualTime().showPercentiles(true));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  options.percentilePoints.push_back(50.0);
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmManualPct");
  check(instance != nullptr, "the expansion produced the registered instance");
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.rows.size() == 1, "the run produces one measured row");
  const sg::ResultRow& row = result.rows[0];
  check(row.percentiles.size() == 2,
        "the manual run answers the two time columns only (FR-010)");
  check(!row.percentiles[0][0].available,
        "a manual run carries no fold behind its primary time (FR-006)");
  check(row.percentiles[1][0].available,
        "the manual run still answers the time column its clock measured "
        "(FR-004)");
}

auto realTimeRunLabelsItsOtherTimeColumn() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  scriptClocks(provider);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmColumned, "bmRealPct");
  static_cast<void>(handle.useRealTime().showPercentiles(true));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  options.percentilePoints.push_back(50.0);
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmRealPct");
  check(instance != nullptr, "the expansion produced the registered instance");
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.rows.size() == 1, "the run produces one measured row");
  check(result.primaryIsCpu == false, "the run made the real column primary");
  check(capturePrint(result).find("cpu time/iter (") != std::string::npos,
        "the real-time run names the column the real column is not by its "
        "own clock (FR-005)");
}

// FR-005, FR-018: a threaded row folds each context's own plan and merges
// the columns; the merged percentile and the per-context percentiles answer
// the same four columns the row prints.
auto threadedColumnsFoldOnTheirOwnPlans() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  scriptClocks(provider);
  scriptQuotientLeaves(provider);
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmColumned, "bmThreadedPct");
  addQuotientMetric(handle);
  static_cast<void>(handle.threads(2).showPercentiles(true));
  auto manual = sg::registerBenchmark(&bmManualTime, "bmThreadedManual");
  static_cast<void>(manual.useManualTime().threads(2).showPercentiles(true));
  auto quiet = sg::registerBenchmark(&bmColumned, "bmThreadedOff");
  static_cast<void>(quiet.threads(2).showPercentiles(false));
  auto real = sg::registerBenchmark(&bmColumned, "bmThreadedReal");
  addQuotientMetric(real);
  static_cast<void>(real.useRealTime().threads(2).showPercentiles(true));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  options.percentilePoints.push_back(50.0);
  options.counterAddresses = {"machine/nosuch"};
  sg::detail::Runner runner(options);

  sg::Instance* const instance = findInstance("bmThreadedPct/threads:2");
  check(instance != nullptr, "the expansion produced the registered instance");
  const sg::BenchmarkResult result = runner.run(*instance);
  check(result.rows.size() == 1, "the threaded run produces one merged row");
  const sg::ResultRow& row = result.rows[0];
  check(row.percentiles.size() == 4,
        "the merged row answers one percentile column per reported column "
        "(FR-005)");
  check(row.percentiles[2][0].available && !row.percentiles[3][0].available,
        "the merged row answers its metric and refuses the absent leaf "
        "(FR-016, FR-023)");
  check(result.contextPercentiles.size() == 2,
        "the run discloses the percentiles of each of the two contexts "
        "(FR-018)");
  check(result.contextPercentiles[0].size() == 4
            && result.contextPercentiles[1].size() == 4,
        "each context answers the four columns of its own fold (FR-018)");

  sg::Instance* const other = findInstance("bmThreadedManual/threads:2");
  check(other != nullptr, "the expansion produced the manual instance");
  const sg::BenchmarkResult manualResult = runner.run(*other);
  check(manualResult.rows.size() == 1,
        "the threaded manual run produces one merged row");
  check(!manualResult.rows[0].percentiles[0][0].available,
        "the threaded manual run carries no fold behind its primary time "
        "(FR-006)");

  const sg::BenchmarkResult quietResult =
      runner.run(*needInstance("bmThreadedOff/threads:2"));
  check(
      quietResult.percentilePoints.empty(),
      "the opted-out threaded family reports no percentile points " "(FR-023)");

  const sg::BenchmarkResult realResult =
      runner.run(*needInstance("bmThreadedReal/threads:2"));
  check(realResult.primaryIsCpu == false,
        "the threaded run made the real column its primary (FR-005)");
  check(realResult.rows[0].percentiles.size() == 4,
        "the real-time threaded row answers its four columns (FR-005)");
}

// FR-016: a leaf that measures nothing leaves the fold of its column
// gapped, and a gapped column answers nothing, whether the column holds a
// time or a metric.
auto gappedThreadedRunAnswersNothing() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  scriptClocks(provider);
  scriptQuotientLeaves(provider);
  std::vector<std::size_t> everyAction(64);
  std::iota(everyAction.begin(), everyAction.end(), 1);
  provider.setGapActions("machine", "num", everyAction);
  provider.setGapActions("machine", "thread_cpu", std::move(everyAction));
  static_cast<void>(
      sg::counters::System::local().registerProvider(std::move(owned)));

  auto handle = sg::registerBenchmark(&bmColumned, "bmThreadedGap");
  addQuotientMetric(handle);
  static_cast<void>(handle.useRealTime().threads(2).showPercentiles(true));

  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = kWindowIterations;
  options.percentilePoints.push_back(50.0);
  sg::detail::Runner runner(options);

  const sg::BenchmarkResult result =
      runner.run(*needInstance("bmThreadedGap/threads:2"));
  check(result.rows.size() == 1, "the gapped threaded run produces one row");
  const sg::ResultRow& row = result.rows[0];
  // The scripted clock column is shared by the two contexts, so this
  // fixture leaves both time columns without a count; the assertion reads
  // the gapped one, which is what the scenario is about.
  check(!row.percentiles[1][0].available,
        "the column whose fold disclosed a gap answers nothing (FR-016)");
  check(row.percentiles.size() == 3,
        "the gapped run answers one column per reported column (FR-016)");
  check(!row.percentiles[2][0].available,
        "the metric over the gapped leaf answers nothing (FR-016)");
}

// FR-023: a row that captured no chunk prints neither the chunk line nor a
// percentile line, whatever points the run reports.
auto aRowWithNoChunkPrintsNoPercentile() -> void
{
  sg::BenchmarkResult result;
  result.name = "bmNoChunk";
  result.suite = "bmNoChunk";
  result.caseName = "bmNoChunk";
  result.percentilePoints.push_back(50.0);
  sg::ResultRow row;
  row.kind = sg::RowKind::REPETITION;
  row.iterations = 4;
  result.rows.push_back(row);

  const std::string printed = capturePrint(result);
  check(printed.find("chunks=") == std::string::npos
            && printed.find("p50") == std::string::npos,
        "a row that captured no chunk prints no chunk line and no percentile "
        "line (FR-023)");
}

// The run scenarios each script one provider and run one plan, so
// each stands in a child process of its own: the scripted point column of
// a leaf advances across the runs of one process, and a second provider
// cannot replace the first.
auto runInChild(const std::string& mode) -> void
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_percentile_test"));
  argv.push_back(const_cast<char*>(mode.c_str()));
  argv.push_back(nullptr);
  const pid_t child = fork();
  if (child < 0) {
    fail("cannot start the run scenario");
  }
  if (child == 0) {
    execv("/proc/self/exe", argv.data());
    ::_exit(127);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)
      || WEXITSTATUS(status) != 0)
  {
    std::fprintf(
        stderr, "the '%s' run scenario did not exit zero\n", mode.c_str());
    fail("a run scenario failed");
  }
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc > 1) {
    const std::string mode = argv[1];
    if (mode == "window") {
      scriptedRunAnswersTheRankOfTheWindow();
    } else if (mode == "fold") {
      chunkFoldNetsEachPauseIntoItsOwnChunk();
    } else if (mode == "paused") {
      pausedRunWithPercentilesClosesInsideItsReservation();
    } else if (mode == "wide") {
      captureReservesItsPointsAndAllocatesNothingPerChunk();
    } else if (mode == "columns") {
      metricAndAbsentColumnsAnswerTheirOwnWay();
    } else if (mode == "manual") {
      manualRunAnswersNoPrimaryPercentile();
    } else if (mode == "real") {
      realTimeRunLabelsItsOtherTimeColumn();
    } else if (mode == "threaded") {
      threadedColumnsFoldOnTheirOwnPlans();
    } else if (mode == "gapped") {
      gappedThreadedRunAnswersNothing();
    } else {
      std::fprintf(stderr, "unknown mode '%s'\n", mode.c_str());
      return 2;
    }
    return 0;
  }

  leastSampleCountOfEachPoint();
  nearestRankOfASortedColumn();
  pointListDefaultsAndNormalizes();
  columnsAnswerPerPoint();
  aRowWithNoChunkPrintsNoPercentile();
  runInChild("fold");
  runInChild("window");
  runInChild("paused");
  runInChild("wide");
  runInChild("columns");
  runInChild("manual");
  runInChild("real");
  runInChild("threaded");
  runInChild("gapped");
  std::printf("harness_percentile_test: ok\n");
  return 0;
}
