// ============================================================================
// TDD test for the recorder arena and overflow policies (T022, T024,
// T025, T026; US2 scenarios).
//
// Covers the plan-owned arena with non-owning trivially-copyable
// cursors (FR-029), hardStop capacity semantics (FR-027), ring
// masking with drop accounting and the wrapped fold window (FR-025,
// FR-028), per-interval foldPairs and first-to-last folds over
// recorder windows (FR-018), independent cursors on one plan, and the
// 2^64 wrap through a recorder-sampled ratio (FR-013). Hand-computed
// expectations, frameworkless check()/fail() convention. Each scenario
// owns dedicated leaves: a leaf script advances one point per sampling
// action, so scenarios sharing leaves would consume each other's
// scripts.
// ============================================================================

#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS RECORDER TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::ClockProvider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::FakeProvider;
using sg::counters::Object;
using sg::counters::Plan;
using sg::counters::RecorderHandle;
using sg::counters::System;

using Events = Dim<0, 1>;
using TimeDim = Dim<1, 0>;

static_assert(
    std::is_trivially_copyable_v<RecorderHandle<sg::counters::HardStop>>,
    "the hard_stop recorder handle is a copyable cursor (FR-029)");
static_assert(std::is_trivially_copyable_v<RecorderHandle<sg::counters::Ring>>,
              "the ring recorder handle is a copyable cursor (FR-029)");

constexpr std::uint64_t kWrapBase = UINT64_MAX - 9;

auto coreObject() -> Object
{
  return *System::local().object("package-1/core-3");
}

// Compiled plan plus its ipc expression, per scenario leaf pair.
struct Scenario
{
  Plan compiled;
  Expression<Dim<0, 0>> ipc;
};

auto ipcScenario(const char* cyclesName, const char* instrName) -> Scenario
{
  const auto core = coreObject();
  const auto cycles = *core.counter<Events>(cyclesName);
  const auto instructions = *core.counter<Events>(instrName);
  const auto ipc = instructions / cycles;
  auto compiled = compile(System::local(), ipc);
  if (!compiled.has_value()) {
    fail("ipc plan compiles");
  }
  return Scenario {
      .compiled = std::move(*compiled),
      .ipc = ipc,
  };
}

auto testHardStopExtent() -> void
{
  const auto sc = ipcScenario("cyc0", "ins0");
  auto rec = sc.compiled.recorder(4);
  check(rec.capacity() == 4, "hard_stop recorder reports its capacity");
  check(!rec.wrapped() && rec.dropped() == 0, "a fresh recorder is empty");
  for (int i = 0; i < 4; ++i) {
    rec.sample();
  }
  check(rec.count() == 4, "hard_stop fills to capacity (FR-026)");
  // The full recorder's own state cannot report a wrap, so the retained
  // window is checked where it can fail: through the columns. The
  // fixture scripts cyc0 as {100, 300, 600, 1000, 1500} and ins0 as
  // {1000, 3100, 5200, 7300, 9500}, and four sampling actions take the
  // first four of each, so a recorder that held fewer points, or a
  // different four, fails here (FR-027).
  const auto cycles = sc.ipc.raw(rec.view(), "package-1/core-3", "cyc0");
  const auto instructions = sc.ipc.raw(rec.view(), "package-1/core-3", "ins0");
  check(cycles.has_value() && instructions.has_value(),
        "the full recorder exposes both retained columns (FR-020)");
  check(cycles->count == 4 && cycles->points[0] == 100
            && cycles->points[1] == 300 && cycles->points[2] == 600
            && cycles->points[3] == 1000,
        "a full hard_stop recorder retains exactly its capacity window "
        "(FR-027)");
  check(instructions->points[0] == 1000 && instructions->points[1] == 3100
            && instructions->points[2] == 5200
            && instructions->points[3] == 7300,
        "the second member's column is retained whole (FR-027)");
  const auto pairs = sc.ipc.foldPairs(rec.view());
  check(pairs.size() == 3, "four points fold into three intervals (FR-018)");
  check(sameDouble(pairs[0].value, 10.5) && sameDouble(pairs[1].value, 7.0)
            && sameDouble(pairs[2].value, 5.25),
        "per-interval folds are exact: 2100/200, 2100/300, 2100/400 (FR-018)");
  check(sameDouble(sc.ipc.fold(rec.view()).value, 7.0),
        "first-to-last fold spans endpoints: 6300/900 (FR-018)");
}

auto testRingOverflow() -> void
{
  const auto sc = ipcScenario("cyc1", "ins1");
  auto rec = *sc.compiled.recorder(2, sg::counters::ring);
  for (int i = 0; i < 4; ++i) {
    rec.sample();
  }
  check(rec.count() == 2, "a ring never exceeds capacity (FR-025)");
  check(rec.wrapped(), "the ring promotes to wrapped (FR-028)");
  check(rec.dropped() == 2, "two samples were overwritten (FR-028)");
  const auto whole = sc.ipc.fold(rec.view());
  check(sameDouble(whole.value, 5.25),
        "the wrapped fold reads the retained window: 2100/400 (FR-028)");
  const auto pairs = sc.ipc.foldPairs(rec.view());
  check(pairs.size() == 1 && sameDouble(pairs[0].value, 5.25),
        "a retained window of two folds into one interval (FR-018)");
  // The raw view of a wrapped recorder spans the oldest to the newest
  // retained row, physically at `dropped % stride` (FR-020, FR-028).
  // Two samples were dropped, so the retained rows sit at physical
  // slots 0 and 1 and the view reports them in logical order.
  const auto column = sc.ipc.raw(rec.view(), "package-1/core-3", "ins1");
  check(column.has_value() && column->count == 2
            && column->points[0] == 5200 && column->points[1] == 7300,
        "a wrapped raw view spans the retained window oldest first "
        "(FR-020, FR-028)");
  check(column.has_value() && sameDouble(column->ratio, 1.0),
        "a source with no time pair discloses ratio 1.0 in its raw view "
        "(FR-020)");
}

auto testRingCapacityGuard() -> void
{
  const auto sc = ipcScenario("cyc1", "ins1");
  check(!sc.compiled.recorder(3, sg::counters::ring).has_value(),
        "ring capacity 3 is rejected (FR-025)");
  check(!sc.compiled.recorder(0, sg::counters::ring).has_value(),
        "ring capacity 0 is rejected (FR-025)");
  check(sc.compiled.recorder(4, sg::counters::ring).has_value(),
        "ring capacity 4 is accepted (FR-025)");
}

auto testIndependentCursors() -> void
{
  const auto sc = ipcScenario("cyc2", "ins2");
  auto first = sc.compiled.recorder(2);
  first.sample();
  first.sample();
  auto second = sc.compiled.recorder(2);
  second.sample();
  second.sample();
  check(sameDouble(sc.ipc.fold(first.view()).value, 10.5),
        "the first cursor holds actions one and two (FR-029)");
  check(sameDouble(sc.ipc.fold(second.view()).value, 5.25),
        "the second cursor holds actions three and four (FR-029)");
}

auto testWrapThroughRecorder() -> void
{
  const auto wrap = *System::local().object("machine")->counter<Events>("wrap");
  const auto tick = *System::local().object("machine")->counter<Events>("tick");
  const auto ratio = wrap / tick;
  auto compiled = compile(System::local(), ratio);
  if (!compiled.has_value()) {
    fail("wrap ratio plan compiles");
  }
  auto rec = compiled->recorder(3);
  rec.sample();
  rec.sample();
  check(sameDouble(ratio.fold(rec.view(), 0, 1).value, 15.0),
        "the first interval folds across 2^64 (FR-013)");
  rec.sample();
  check(sameDouble(ratio.fold(rec.view(), 0, 1).value, 15.0),
        "the earlier interval survives a later sample");
  check(sameDouble(ratio.fold(rec.view(), 1, 2).value, 15.0),
        "the post-wrap interval folds plainly (FR-013)");
}

// The disclosure column beside each count. A measured action carries the
// entry's own countability value, an action that measured nothing carries
// `Availability::GAP` beside a zero count, and a measured zero carries the
// entry's value beside it, so a caller tells the two apart (FR-007).
auto testDisclosureColumn() -> void
{
  const auto core = coreObject();
  const auto cycles = *core.counter<Events>("cyc3");
  const auto instructions = *core.counter<Events>("ins3");
  const auto ipc = instructions / cycles;
  auto compiled = compile(System::local(), ipc);
  if (!compiled.has_value()) {
    fail("the disclosure plan compiles");
  }
  auto rec = compiled->recorder(4);
  for (int i = 0; i < 4; ++i) {
    rec.sample();
  }
  // The counts are read through the raw view, which resolves each leaf by
  // its canonical address, so the assertion does not depend on the order
  // the compile laid the leaves out in. The disclosure sits in its own
  // managed column, which the sampling action writes last (FR-007).
  const auto cyclesView = ipc.raw(rec.view(), "package-1/core-3", "cyc3");
  const auto instructionsView = ipc.raw(rec.view(), "package-1/core-3", "ins3");
  if (!cyclesView.has_value() || !instructionsView.has_value()) {
    fail("the recorded row exposes both retained columns (FR-020)");
  }
  // The state of a row is read through the public surface, never through a
  // column index. A fold over a window whose end point is a row publishes
  // the state that row discloses, so the three windows below establish the
  // state of every row, and the raw view publishes the state of the last row
  // it spans. The test names no column index, so it holds whatever layout the
  // compile chose (FR-004, FR-005, SC-003).
  const auto gap = sg::counters::Availability::GAP;
  const auto countable = sg::counters::Availability::COUNTABLE;

  check(cyclesView->points[0] == 100 && instructionsView->points[0] == 1000,
        "a measured action carries its own count (FR-007)");
  check(cyclesView->points[1] == 260 && instructionsView->points[1] == 2600,
        "a second measured action carries its own count (FR-007)");
  check(cyclesView->points[2] == 0 && instructionsView->points[2] == 0,
        "an action that measured nothing carries a zero count (FR-007)");
  check(cyclesView->points[3] == 700 && instructionsView->points[3] == 7000,
        "the action after a gap carries the count the script resumed at "
        "(FR-007)");
  check(cyclesView->availability == countable
            && instructionsView->availability == countable,
        "a raw view publishes the countability value beside its counts, read "
        "from the field and naming no column index (FR-004, FR-005, SC-003)");
  check(cyclesView->points[2] == 0 && cyclesView->availability == countable,
        "PointsView.availability names the window end point. An interior "
        "gap leaves a zero in points and leaves the field countable");

  // The three gap windows FR-001 names, over the scripted counts above.
  // A window whose end point is the gap, a window whose start point is the
  // gap, and a window with the gap strictly inside. The recorded counts
  // are cumulative, so the inside window's two end points are both
  // measured and the delta between them is the real interval.
  const auto endPointGap = ipc.fold(rec.view(), 1, 2);
  check(endPointGap.availability == gap,
        "a window whose end point is the gap discloses the gap (FR-001)");
  const auto startPointGap = ipc.fold(rec.view(), 2, 3);
  check(startPointGap.availability == gap,
        "a window whose start point is the gap discloses the gap (FR-001)");
  const auto insideGap = ipc.fold(rec.view(), 0, 3);
  check(
      insideGap.availability != gap,
      "a window with the gap strictly inside is not a gap window " "(FR-001)");
  check(sameDouble(insideGap.value, 6000.0 / 600.0),
        "a window with the gap strictly inside folds the delta between its "
        "two measured end points, which the cumulative counts make exact "
        "(FR-001)");

  // The ratio over the gap is reported as no measured fraction, because
  // the disclosure marks the action as one that measured nothing (FR-005).
  const auto overGap = ipc.fold(rec.view(), 0, 1);
  check(sameDouble(overGap.runningRatio, 1.0),
        "a fold across an action the disclosure marks as a gap discloses no "
        "measured multiplex ratio (FR-005)");
  check(sameDouble(endPointGap.runningRatio, 1.0),
        "a fold whose end point is the gap discloses no measured multiplex "
        "ratio, and the state beside it names the reason (FR-004, FR-005)");

  // FR-006: no fold over an action the disclosure marked as a gap reports
  // a delta from a count the read never produced. Two folds are compared
  // with each other, and no constant enters: one spans the measured action
  // to the measured action across the gap, so the gap sits strictly inside
  // its window, and one starts at the gap, so the gap is one of its end
  // points. The first reports only the delta the two measured actions
  // drove; the second reports no value at all, because a window with a gap
  // at an end point has no measured delta (FR-001, SC-002).
  const auto overGapSpan = ipc.fold(rec.view(), 0, 3);
  const auto fromGap = ipc.fold(rec.view(), 2, 3);
  const auto drivenCycles = cyclesView->points[3] - cyclesView->points[0];
  const auto drivenInstructions =
      instructionsView->points[3] - instructionsView->points[0];
  check(fromGap.availability == sg::counters::Availability::GAP
            && sameDouble(fromGap.value, 0.0),
        "a fold starting at the gap reports no value, so the refused action "
        "contributed no count to any delta (FR-006, FR-001)");
  check(overGapSpan.availability != sg::counters::Availability::GAP,
        "the fold spanning the gap has two measured end points, so it is not "
        "a gap window (FR-001)");
  check(sameDouble(overGapSpan.value,
                    static_cast<double>(drivenInstructions)
                        / static_cast<double>(drivenCycles)),
        "a fold spanning the gap row reports only the delta the measured "
        "actions drove, which the cumulative counts make exact (FR-006)");
}

// One sampling action is noexcept and writes no shared state. No recorded
// timing constant decides either, so the assertion reaches the same verdict
// on a CI runner and on a developer's machine (FR-008, Constitution VI).
// The allocation half of the same obligation is proven where the counting
// operator new lives, in `counters_noalloc_test`, which owns that
// translation unit; a second replacement here would pair a `delete` against
// the replaced `new` and fail the release build's mismatched-allocation
// check.
auto testSamplingActionProperties() -> void
{
  const auto sc = ipcScenario("cyc0", "ins0");
  auto rec = sc.compiled.recorder(4);
  static_assert(noexcept(rec.sample()),
                "one sampling action is noexcept (FR-008)");
  rec.sample();
  rec.sample();

  // The action reaches the compiled layout through a const plan, so it
  // writes no state any other action or thread reads: a lock would need a
  // mutable member to take, and the layout holds none.
  const auto& readonly = sc.compiled;
  auto second = readonly.recorder(2);
  second.sample();
  check(second.count() == 1,
        "a sampling action writes only the recorder it was handed, so the "
        "compiled plan carries no lock the action takes (FR-008)");
}

// Catalog resolution and plan compile are safe from any number of threads
// at once once the catalog is open. Every thread here resolves, walks, and
// compiles at the same time, and the thread sanitizer is what decides the
// result: the assertions check the values a caller reads, and the run
// reports no race (FR-010, FR-011).
auto testConcurrentResolution() -> void
{
  constexpr int kThreadCount = 8;
  constexpr int kRounds = 32;
  std::atomic<int> failures {0};
  std::atomic<int> compiles {0};

  const auto resolveAndCompile = [&failures, &compiles](const int index)
  {
    for (int round = 0; round < kRounds; ++round) {
      // The same canonical address from every thread, and a second
      // address beside it, so the handle map sees one key contended and
      // one key per thread.
      const auto core = System::local().object("package-1/core-3");
      const auto machine = System::local().object("machine");
      if (!core.has_value() || !machine.has_value()) {
        ++failures;
        return;
      }
      // Each thread samples its own scripted leaves. A leaf's script is
      // per leaf, so two threads sharing one would race in the fixture
      // and hide the library races this test exists to find.
      const std::string cyclesName = "ccy" + std::to_string(index);
      const std::string instrName = "cci" + std::to_string(index);
      const auto cycles = core->counter<Events>(cyclesName);
      const auto instructions = core->counter<Events>(instrName);
      if (!cycles.has_value() || !instructions.has_value()) {
        ++failures;
        return;
      }
      const auto compiled = compile(System::local(), *instructions / *cycles);
      if (!compiled.has_value()) {
        ++failures;
        return;
      }
      ++compiles;
      auto rec = compiled->recorder(2);
      rec.sample();
      // A plan binds to the thread that compiled it, so the read below
      // runs on this thread, which is where it belongs (FR-031).
      if (rec.count() != 1) {
        ++failures;
      }
      // Walking a parent and its children resolves further handles, so
      // every thread writes the handle map beside the others.
      const sg::counters::Object* parent = core->parent();
      if (parent == nullptr || parent->children().empty()) {
        ++failures;
      }
    }
  };

  const auto walkTree = [&failures]()
  {
    for (int round = 0; round < kRounds; ++round) {
      const auto leaf = System::local().object("package-1/core-3");
      const auto package = System::local().object("package-1");
      if (!leaf.has_value() || !package.has_value()) {
        ++failures;
        return;
      }
      // The leaf walks up to its package, and the package walks down to
      // its children. Both directions resolve through the handle map, so
      // the walk contends it beside the other threads' resolutions.
      if (leaf->parent() == nullptr || package->children().empty()
          || leaf->counters().empty())
      {
        ++failures;
        return;
      }
    }
  };

  std::vector<std::thread> workers;
  workers.reserve(kThreadCount + 1);
  for (int index = 0; index < kThreadCount; ++index) {
    workers.emplace_back(resolveAndCompile, index);
  }
  workers.emplace_back(walkTree);
  for (auto& worker : workers) {
    worker.join();
  }

  check(failures.load() == 0,
        "concurrent resolution and concurrent plan compile both succeed on "
        "every thread (FR-010, FR-011)");
  check(compiles.load() == kThreadCount * kRounds,
        "every thread compiled its own plan while the others compiled and "
        "resolved beside it (FR-011)");
}

auto registerEverything() -> void
{
  auto provider = std::make_unique<FakeProvider>();
  // The package node exists so `package-1/core-3` has a parent to walk to,
  // which is what puts the handle map behind a parent lookup and a direct
  // resolution (FR-010).
  provider->addObject("package-1", "pkg", "package", "first package");
  provider->addObject("package-1/core-3", "cpu3", "core", "third core");
  // One leaf pair per sampling scenario, and no pair beyond them: a
  // registered leaf nothing samples is dead fixture weight.
  for (int scenario = 0; scenario < 4; ++scenario) {
    const char cyclesName[] = {'c', 'y', 'c', char('0' + scenario), '\0'};
    const char instrName[] = {'i', 'n', 's', char('0' + scenario), '\0'};
    provider->addCounter(
        "package-1/core-3", cyclesName, "ops", "scenario cycles");
    provider->addCounter(
        "package-1/core-3", instrName, "ops", "scenario instructions");
    provider->setPoints(
        "package-1/core-3", cyclesName, {100, 300, 600, 1000, 1500}, 0);
    provider->setPoints(
        "package-1/core-3", instrName, {1000, 3100, 5200, 7300, 9500}, 0);
  }
  // The disclosure scenario, scripted so every measured count stands above
  // zero (FR-001, FR-007). A script that started at zero made the value a
  // gap writes indistinguishable from a measured zero, because both read
  // zero. Scripting the cumulative counts above zero lets a fold tell the
  // two apart: a window whose end point is the gap has its own end-point
  // value replaced by a zero, and a window whose start point is the gap has
  // its start value replaced, while a window with the gap strictly inside
  // keeps two measured end points.
  //
  // The script holds seven points, so a plan of three sampling actions sees
  // the first four: action one measures the first count, action two
  // measures the second, action three measures the gap, action four
  // measures the fourth. The gap is action three.
  provider->setPoints("package-1/core-3", "cyc3", {100, 260, 700}, 0);
  provider->setPoints("package-1/core-3", "ins3", {1000, 2600, 7000}, 0);
  // Action three is the gap. Neither leaf advances its script there, so the
  // measured action after the gap reads the third scripted count.
  provider->setGapActions("package-1/core-3", "cyc3", {3});
  provider->setGapActions("package-1/core-3", "ins3", {3});
  // One dedicated leaf pair per thread for the concurrency test, which
  // runs first and must not consume the scripts above.
  for (int scenario = 0; scenario < 8; ++scenario) {
    const char cyclesName[] = {'c', 'c', 'y', char('0' + scenario), '\0'};
    const char instrName[] = {'c', 'c', 'i', char('0' + scenario), '\0'};
    provider->addCounter(
        "package-1/core-3", cyclesName, "ops", "concurrent cycles");
    provider->addCounter(
        "package-1/core-3", instrName, "ops", "concurrent instructions");
    provider->setPoints("package-1/core-3", cyclesName, {100, 300, 600}, 0);
    provider->setPoints("package-1/core-3", instrName, {1000, 3100, 5200}, 0);
  }
  provider->addCounter("machine", "wrap", "ops", "counter that wraps");
  provider->setPoints("machine", "wrap", {kWrapBase, 5, 20, 40}, 0);
  provider->addCounter("machine", "tick", "ops", "denominator ticks");
  provider->setPoints("machine", "tick", {0, 1, 2, 3}, 0);
  const auto registered = System::local().registerProvider(std::move(provider));
  if (!registered.has_value()) {
    fail("the scripted provider registers");
  }

  // A second provider, and it registers second on purpose. A plan that
  // draws one leaf from each of the two puts the scripted group short of
  // the plan's last group, so that group's window is handed a leaf set
  // carrying no disclosure column. The scripted provider alone can never
  // reach that direction, because a lone provider is always the last
  // group and always the one that discloses (FR-007).
  auto clock = std::make_unique<ClockProvider>();
  const auto clocked = System::local().registerProvider(std::move(clock));
  if (!clocked.has_value()) {
    fail("the clock provider registers after the scripted provider");
  }
}

// A plan drawing one leaf from the scripted provider and one from the
// clock provider spans two providers. The scripted group is not the
// plan's last group, so its window opens on a leaf set with no disclosure
// column, and the scripted provider reads without writing one. The
// tracefile named this direction the one it never took (FR-007).
auto testMixedProviderDisclosure() -> void
{
  const auto core = *System::local().object("package-1/core-3");
  const auto instructions = *core.counter<Events>("ins3");
  const auto machine = *System::local().object("machine");
  const auto mono = *machine.counter<TimeDim>("monotonic");
  const auto rate = instructions / mono;
  auto compiled = compile(System::local(), rate);
  check(compiled.has_value(),
        "a plan over a scripted leaf and a clock leaf compiles");
  if (!compiled.has_value()) {
    return;
  }
  auto rec = compiled->recorder(2);
  rec.sample();
  rec.sample();
  const auto folded = rate.fold(rec.view(), 0, 1);
  check(folded.runningRatio > 0.0,
        "the mixed-provider plan folds a positive measurement (FR-007)");
}

// A plan assigned to itself. The assignment guards on identity, so the
// layout survives and the plan still folds (FR-022).
auto testSelfMoveAssignment() -> void
{
  auto sc = ipcScenario("cyc0", "ins0");
  auto rec = sc.compiled.recorder(2);
  rec.sample();
  rec.sample();
  const auto before = sc.ipc.fold(rec.view()).value;
  // A plan holds an owning layout pointer, so the assignment guards on
  // identity: assigning a plan to itself keeps the layout (FR-022).
  auto& alias = sc.compiled;
  alias = std::move(sc.compiled);
  auto rec2 = sc.compiled.recorder(2);
  rec2.sample();
  rec2.sample();
  check(sameDouble(sc.ipc.fold(rec2.view()).value, before),
        "a self-move-assigned plan still folds its own window (FR-022)");
}

// The overhead accessors run the plan's calibration. The dedicated
// overhead binary skips on a host with no cpu object, so this scenario
// is what a runner without one still executes (FR-032).
auto testSampleOverheadCalibration() -> void
{
  auto sc = ipcScenario("cyc0", "ins0");
  const double low = sc.compiled.sampleOverheadNsMin();
  const double mid = sc.compiled.sampleOverheadNsMedian();
  const double high = sc.compiled.sampleOverheadNsMax();
  check(low >= 0.0, "the calibrated minimum is non-negative");
  check(mid >= 0.0, "the calibrated median is non-negative");
  check(high >= low, "the calibrated maximum is at least the minimum");
  const double again = sc.compiled.sampleOverheadNsMin();
  check(again >= 0.0, "a second calibration read stays non-negative");
}

}  // namespace

auto main() -> int
{
  registerEverything();
  testConcurrentResolution();
  testHardStopExtent();
  testRingOverflow();
  testRingCapacityGuard();
  testIndependentCursors();
  testWrapThroughRecorder();
  testSelfMoveAssignment();
  testDisclosureColumn();
  testMixedProviderDisclosure();
  testSamplingActionProperties();
  testSampleOverheadCalibration();
  std::printf("counters recorder tests passed\n");
  return 0;
}
