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

using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::expression;
using sg::counters::fake_provider;
using sg::counters::system;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

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

auto same_double(const double left, const double right) -> bool
{
  return std::fabs(left - right) <= 1e-9 * (1.0 + std::fabs(right));
}

// The clock provider, registered in the order this binary was built for.
// It is the one provider in the suite with no scripted points, so its leaf
// needs no fixture.
auto register_clock() -> void
{
  if (!system::local()
           .register_provider(std::make_unique<clock_provider>())
           .has_value())
  {
    fail("the clock provider registers");
  }
}

// The scripted PMU provider. Its one core leaf is the window driven to a
// gap on the second sampling action, and the counts above zero separate
// the zero the gap writes from a first point (FR-001, D-13).
auto register_scripted() -> std::unique_ptr<fake_provider>
{
  auto provider = std::make_unique<fake_provider>();
  provider->add_counter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  // Sampling actions are counted from one. Action two measured nothing, so
  // both leaves disclose a gap beside a zero count there and neither script
  // advances, which leaves the third action reading the second scripted
  // point (FR-007).
  provider->set_points("package-1/core-3", "cycles", {100, 300, 600}, 0);
  provider->set_points(
      "package-1/core-3", "instructions", {1000, 4100, 7200}, 0);
  provider->set_gap_actions("package-1/core-3", "cycles", {2});
  provider->set_gap_actions("package-1/core-3", "instructions", {2});
  provider->add_counter(
      "package-1/core-3", "rate_cycles", "ops", "rate cycles");
  provider->set_points("package-1/core-3", "rate_cycles", {100, 300, 600}, 0);
  provider->set_gap_actions("package-1/core-3", "rate_cycles", {2});
  if (!system::local().register_provider(std::move(provider)).has_value()) {
    fail("the scripted provider registers");
  }
  return nullptr;
}

// The two end points of the gap window, and the window spanning it. Both
// read the fold result's own availability field, so no assertion below
// names a column index (FR-004, FR-005, SC-003).
auto gap_scenario() -> void
{
  const auto core = *system::local().object("package-1/core-3");
  const expression<events> cycles {*core.counter<events>("cycles")};
  const expression<events> instructions {*core.counter<events>("instructions")};
  const auto ipc = instructions / cycles;
  const auto compiled = compile(system::local(), ipc);
  if (!compiled.has_value()) {
    fail("the PMU plan compiles");
  }
  auto rec = compiled->recorder(3);
  rec.sample();
  rec.sample();
  rec.sample();

  const auto gap = sg::counters::availability::gap;
  const auto over_gap = ipc.fold(rec.view(), 0, 1);
  check(over_gap.availability == gap,
        "a window whose end point is a scripted gap discloses the gap "
        "(FR-001)");
  const auto spanning = ipc.fold(rec.view(), 0, 2);
  check(spanning.availability != gap,
        "a window with the gap strictly inside has two measured end points "
        "and is not a gap window (FR-001)");
  check(same_double(spanning.value, 3100.0 / 200.0),
        "the spanning window folds only the delta the two measured actions "
        "drove (FR-001, FR-006)");
}

// A plan holding leaves from both providers, folded across the gap. The
// disclosure the PMU window wrote must survive whichever window owns the
// plan's last managed leaf, because each window discloses into a column of
// its own (FR-002, D-03).
auto mixed_provider_scenario() -> void
{
  const auto core = *system::local().object("package-1/core-3");
  const auto machine = *system::local().object("machine");
  const expression<events> cycles {*core.counter<events>("rate_cycles")};
  const expression<time_dim> stamp {*machine.counter<time_dim>("monotonic")};
  // The PMU leaf carries an events unit and the clock leaf a time unit, so
  // the rate divides the core's cycles by the clock's elapsed time.
  const auto per_second = cycles / stamp;
  const auto compiled = compile(system::local(), per_second, stamp);
  if (!compiled.has_value()) {
    fail("a plan holding a PMU leaf and a clock leaf compiles");
  }
  auto rec = compiled->recorder(3);
  rec.sample();
  rec.sample();
  rec.sample();

  // The gap sits at sampling action two, so the window ending there is a
  // gap window. The clock window's own mark is countable, and the result
  // names the gap rather than the clock's countability, because the
  // disclosure beside the leaf the fold reads is the one its own group
  // wrote (FR-002, FR-004).
  const auto gap = sg::counters::availability::gap;
  const auto over_gap = per_second.fold(rec.view(), 0, 1);
  check(over_gap.availability == gap,
        "a mixed plan reaching the fold result discloses the scripted gap in "
        "whichever order the providers registered (FR-002, SC-002, D-03)");
  check(same_double(over_gap.value, 0.0),
        "a gap window in a mixed plan reports no value, because no measured "
        "count backs one (FR-001, FR-006)");

  // The window spanning the gap has two measured end points, so it folds
  // the real interval, and the ratio beside it is a measured fraction (the
  // clock leaf contributes a time pair).
  const auto spanning = per_second.fold(rec.view(), 0, 2);
  check(spanning.availability != gap,
        "the spanning window of a mixed plan is not a gap window, because "
        "both of its end points measured (FR-001)");
  check(spanning.value > 0.0,
        "the spanning window of a mixed plan folds the interval the measured "
        "actions drove (FR-001)");

  // The raw view reads the same state through its own field, and its
  // source names no column index (FR-005, SC-003).
  const auto view =
      per_second.raw(rec.view(), "package-1/core-3", "rate_cycles");
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
  register_clock();
  register_scripted();
  constexpr const char* kOrder = "clock first, PMU second";
#else
  register_scripted();
  register_clock();
  constexpr const char* kOrder = "PMU first, clock second";
#endif
  gap_scenario();
  mixed_provider_scenario();
  std::printf("counters registration order tests passed: %s\n", kOrder);
  return 0;
}
