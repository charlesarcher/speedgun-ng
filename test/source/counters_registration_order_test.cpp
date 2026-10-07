// ============================================================================
// TDD test for the per-window disclosure column under both provider
// registration orders (T012, T013; US2).
//
// A plan holding a PMU leaf and a clock leaf must reach the fold result
// with the same result whichever provider registered first. At the audit
// point the compile named the disclosure column to the one window owning
// the plan's last managed leaf, so a PMU gap was lost whenever the clock
// provider registered second (F-01, D-03).
//
// The order is fixed at build time by SG_REGISTRATION_ORDER, because
// provider registration happens once per process and the two orders cannot
// share one binary. The build defines it twice and runs both binaries, so
// each order is a test that can fail.
//
// Every assertion reads the public availability field and names no column
// index (FR-004, FR-005, SC-002, SC-003).
// ============================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

#ifndef SG_REGISTRATION_ORDER
#  error "SG_REGISTRATION_ORDER must name the registration order under test"
#endif

using sg::counters::ClockProvider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::FakeProvider;
using sg::counters::System;

using Events = Dim<0, 1>;
using TimeDim = Dim<1, 0>;

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS REGISTRATION ORDER TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool holds, const char* what) -> void
{
  if (!holds) {
    fail(what);
  }
}

auto sameDouble(const double left, const double right) -> bool
{
  return std::fabs(left - right) <= 1e-9 * (1.0 + std::fabs(right));
}

// The clock provider, registered in the order this binary was built for.
// It is the one provider in the suite with no scripted points, so its leaf
// needs no fixture.
auto registerClock() -> void
{
  if (!System::local()
           .registerProvider(std::make_unique<ClockProvider>())
           .has_value())
  {
    fail("the clock provider registers");
  }
}

// The scripted PMU provider. Its one core leaf is the window driven to a
// gap on the second sampling action, and the counts above zero separate
// the zero the gap writes from a first point (FR-001, D-13).
auto registerScripted() -> std::unique_ptr<FakeProvider>
{
  auto provider = std::make_unique<FakeProvider>();
  provider->addCounter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->addCounter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  // Sampling actions are counted from one. Action two measured nothing, so
  // both leaves disclose a gap beside a zero count there and neither script
  // advances, which leaves the third action reading the second scripted
  // point (FR-007).
  provider->setPoints("package-1/core-3", "cycles", {100, 300, 600}, 0);
  provider->setPoints(
      "package-1/core-3", "instructions", {1000, 4100, 7200}, 0);
  provider->setGapActions("package-1/core-3", "cycles", {2});
  provider->setGapActions("package-1/core-3", "instructions", {2});
  provider->addCounter("package-1/core-3", "rate_cycles", "ops", "rate cycles");
  provider->setPoints("package-1/core-3", "rate_cycles", {100, 300, 600}, 0);
  provider->setGapActions("package-1/core-3", "rate_cycles", {2});
  if (!System::local().registerProvider(std::move(provider)).has_value()) {
    fail("the scripted provider registers");
  }
  return nullptr;
}

// The two end points of the gap window, and the window spanning it. Both
// read the fold result's own availability field, so no assertion below
// names a column index (FR-004, FR-005, SC-003).
auto gapScenario() -> void
{
  const auto core = *System::local().object("package-1/core-3");
  const Expression<Events> cycles {*core.counter<Events>("cycles")};
  const Expression<Events> instructions {*core.counter<Events>("instructions")};
  const auto ipc = instructions / cycles;
  const auto compiled = compile(System::local(), ipc);
  if (!compiled.has_value()) {
    fail("the PMU plan compiles");
  }
  auto rec = compiled->recorder(3);
  rec.sample();
  rec.sample();
  rec.sample();

  const auto gap = sg::counters::Availability::GAP;
  const auto overGap = ipc.fold(rec.view(), 0, 1);
  check(overGap.availability == gap,
        "a window whose end point is a scripted gap discloses the gap "
        "(FR-001)");
  const auto spanning = ipc.fold(rec.view(), 0, 2);
  check(spanning.availability != gap,
        "a window with the gap strictly inside has two measured end points "
        "and is not a gap window (FR-001)");
  check(sameDouble(spanning.value, 3100.0 / 200.0),
        "the spanning window folds only the delta the two measured actions "
        "drove (FR-001, FR-006)");
}

// A plan holding leaves from both providers, folded across the gap. The
// disclosure the PMU window wrote must survive whichever window owns the
// plan's last managed leaf, because each window discloses into a column of
// its own (FR-002, D-03).
auto mixedProviderScenario() -> void
{
  const auto core = *System::local().object("package-1/core-3");
  const auto machine = *System::local().object("machine");
  const Expression<Events> cycles {*core.counter<Events>("rate_cycles")};
  const Expression<TimeDim> stamp {*machine.counter<TimeDim>("monotonic")};
  // The PMU leaf carries an events unit and the clock leaf a time unit, so
  // the rate divides the core's cycles by the clock's elapsed time.
  const auto perSecond = cycles / stamp;
  const auto compiled = compile(System::local(), perSecond, stamp);
  if (!compiled.has_value()) {
    fail("a plan holding a PMU leaf and a clock leaf compiles");
  }
  auto rec = compiled->recorder(3);
  rec.sample();
  rec.sample();
  rec.sample();

  // The gap sits at sampling action two, so the window ending there is a
  // gap window. The clock window's own mark is countable. The result names
  // the gap, because the disclosure beside the leaf the fold reads is the
  // one its own group wrote (FR-002, FR-004).
  const auto gap = sg::counters::Availability::GAP;
  const auto overGap = perSecond.fold(rec.view(), 0, 1);
  check(overGap.availability == gap,
        "a mixed plan reaching the fold result discloses the scripted gap in "
        "whichever order the providers registered (FR-002, SC-002, D-03)");
  check(sameDouble(overGap.value, 0.0),
        "a gap window in a mixed plan reports no value, because no measured "
        "count backs one (FR-001, FR-006)");

  // The window spanning the gap has two measured end points, so it folds
  // the real interval, and the ratio beside it is a measured fraction (the
  // clock leaf contributes a time pair).
  const auto spanning = perSecond.fold(rec.view(), 0, 2);
  check(spanning.availability != gap,
        "the spanning window of a mixed plan is not a gap window, because "
        "both of its end points measured (FR-001)");
  check(spanning.value > 0.0,
        "the spanning window of a mixed plan folds the interval the measured "
        "actions drove (FR-001)");

  // The raw view reads the same state through its own field, and its
  // source names no column index (FR-005, SC-003).
  const auto view =
      perSecond.raw(rec.view(), "package-1/core-3", "rate_cycles");
  if (!view.has_value()) {
    fail("the mixed plan exposes the PMU leaf's raw view");
  }
  check(view->availability != gap,
        "the raw view's end point is the last sampling action, which "
        "measured, so it is not a gap view (FR-004, FR-005)");
}

}  // namespace

auto main() -> int
{
#if SG_REGISTRATION_ORDER == 0
  registerClock();
  registerScripted();
  constexpr const char* kOrder = "clock first, PMU second";
#else
  registerScripted();
  registerClock();
  constexpr const char* kOrder = "PMU first, clock second";
#endif
  gapScenario();
  mixedProviderScenario();
  std::printf("counters registration order tests passed: %s\n", kOrder);
  return 0;
}
