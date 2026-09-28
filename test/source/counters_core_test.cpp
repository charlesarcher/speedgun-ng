// ============================================================================
// TDD test for the counters core vocabulary (T009).
//
// Covers the dimension tag algebra (T005), the closed unit switch (T006),
// and the catalog/result/error shapes as the system builds them from a
// registered provider (T007). Hand-computed expectations, frameworkless
// check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS CORE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// Exact double comparison through the bit pattern: these are exactness
// tests, and the tolerance band has no place in them.
auto same_double(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::dim_quotient;
using sg::counters::dim_same;
using sg::counters::dimension_of;
using sg::counters::fake_provider;
using sg::counters::read_mode;
using sg::counters::system;
using sg::counters::unit;
using sg::counters::unit_from_token;
using sg::counters::unit_name;

using events = dim<0, 1>;

// Compile-time dimension algebra (T005, FR-015).
static_assert(dim_same<dim<0, 1>, dim<0, 1>>, "identical tags are the same");
static_assert(!dim_same<dim<0, 1>, dim<1, 0>>, "events^1 differs from time^1");
static_assert(dim_same<dim_quotient<dim<0, 1>, dim<0, 1>>, dim<0, 0>>,
              "instructions / cycles is dim<0,0>");
static_assert(dim_same<dim_quotient<dim<0, 1>, dim<1, 0>>, dim<-1, 1>>,
              "bytes / monotonic is dim<-1,1>");
static_assert(dim_same<dim_quotient<dim<1, 0>, dim<1, 0>>, dim<0, 0>>,
              "monotonic / monotonic is dim<0,0>");

auto test_unit_switch() -> void
{
  const unit seconds = *unit_from_token("seconds");
  check(seconds == unit::seconds, "seconds token maps to unit::seconds");

  const auto dims = dimension_of(*unit_from_token("nanoseconds"));
  check(dims.has_value(), "nanoseconds resolves");
  check(dims->time == 1 && dims->events == 0, "nanoseconds is time^1");

  const auto bytes = dimension_of(*unit_from_token("bytes"));
  check(bytes.has_value() && bytes->time == 0 && bytes->events == 1,
        "bytes is events^1");

  const auto ops = dimension_of(*unit_from_token("ops"));
  check(ops.has_value() && ops->time == 0 && ops->events == 1,
        "ops is events^1");

  const auto none = dimension_of(*unit_from_token("none"));
  check(none.has_value() && none->time == 0 && none->events == 1,
        "none is events^1");

  const auto seconds_dim = dimension_of(unit::seconds);
  check(seconds_dim.has_value() && seconds_dim->time == 1
            && seconds_dim->events == 0,
        "seconds is time^1");
}

auto test_unknown_unit_names_the_unit() -> void
{
  const auto bad = unit_from_token("furlongs");
  check(!bad.has_value(), "unrecognized unit is rejected");
  check(bad.error().message.find("furlongs") != std::string::npos,
        "error message names the rejected unit");
  check(bad.error().suggestions.empty(),
        "unit errors carry no catalog suggestions");
}

auto test_unit_names_round_trip() -> void
{
  check(unit_name(unit::seconds) == "seconds", "seconds spelling");
  check(unit_name(unit::nanoseconds) == "nanoseconds", "nanoseconds spelling");
  check(unit_name(unit::bytes) == "bytes", "bytes spelling");
  check(unit_name(unit::ops) == "ops", "ops spelling");
  check(unit_name(unit::none) == "none", "none spelling");
}

// One provider registered before the open boundary, so the three shape
// tests below read the entries, errors, and results the library built
// (T007, FR-005, FR-008, FR-019).
auto register_shape_fixture() -> void
{
  auto provider = std::make_unique<fake_provider>();
  provider->add_object("core-0", "cpu0", "core", "the core under test");
  provider->add_counter("core-0", "cycles", "ops", "core cycles elapsed");
  provider->add_counter("core-0", "retired", "ops", "instructions retired");
  provider->set_points("core-0", "cycles", {0, 200}, 200);
  provider->set_points("core-0", "retired", {0, 6300}, 6300);
  const auto registered =
      system::local().register_provider(std::move(provider));
  check(registered.has_value(),
        "the shape fixture registers before open (FR-009)");
}

// The fold output: the value is the hand-computed quotient of the two
// scripted window deltas, and the two disclosure fields carry the
// window's multiplex state (FR-019, FR-030).
auto test_metric_result_fields() -> void
{
  const auto core = *system::local().object("core-0");
  const auto cycles = *core.counter<events>("cycles");
  const auto retired = *core.counter<events>("retired");
  const auto per_cycle = retired / cycles;
  const auto compiled = compile(system::local(), per_cycle);
  check(compiled.has_value(), "the quotient plan compiles (FR-021)");

  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  // 6300 retired over 200 cycles, the two scripted steps committed
  // beside the fixture above.
  const auto metric = window.metric(per_cycle);
  check(same_double(metric.value, 31.5),
        "the fold value is the hand-computed quotient 6300 / 200 (FR-030)");
  check(same_double(metric.running_ratio, 1.0) && !metric.scaled,
        "a window with no multiplexed source discloses ratio 1.0 and "
        "unscaled ticks (FR-019)");
}

// The recoverable error the resolution path builds: it names the object
// and the rejected name, and the suggestion list is the near-miss scan
// the library ranked (FR-008).
auto test_error_shape() -> void
{
  const auto core = *system::local().object("core-0");
  const auto mistyped = core.counter<events>("cycle");
  check(!mistyped.has_value(), "an undeclared counter name is refused");
  const auto& reported = mistyped.error();
  check(reported.message.find("core-0") != std::string::npos
            && reported.message.find("cycle") != std::string::npos,
        "the error names the object and the rejected name (FR-008)");
  check(reported.suggestions.size() == 1
            && reported.suggestions[0] == "cycles",
        "the ranked list offers the one name within the edit distance "
        "(FR-008)");
  const auto by_word = core.counter<events>("elapsed");
  check(!by_word.has_value() && by_word.error().suggestions.size() == 1
            && by_word.error().suggestions[0] == "cycles",
        "a distant query falls back to the description word overlap (FR-008)");
}

// The catalog entry the system builds from one provider counter: the
// declared fields travel, and the two disclosure defaults a provider
// leaves unset travel as the header declares them (FR-005, FR-017).
auto test_catalog_entry_shape() -> void
{
  const auto core = *system::local().object("core-0");
  const auto entries = core.counters();
  check(entries.size() == 2,
        "the object reports its two registered counters (FR-001)");
  const auto found = std::ranges::find_if(entries,
                                          [](const catalog_entry& entry)
                                          { return entry.name == "retired"; });
  check(found != entries.end(), "the registered counter is in the catalog");
  check(found->description == "instructions retired"
            && found->unit == unit::ops
            && found->avail == availability::countable
            && found->mode == read_mode::syscall,
        "the entry carries the declared description, unit, state, and read "
        "mode (FR-005, FR-006)");
  check(found->frequency_hz == 0 && !found->scaled,
        "a counter declaring neither calibration nor a scaled tick source "
        "discloses frequency 0 and unscaled (FR-019)");
}

// The defensive close of the closed unit enumeration, which the header
// documents as the failure branch of `dimension_of` and as the fallback
// `unit_name` returns. A value outside the enumeration is reachable only
// through a cast. `dimension_of` then refuses with an error and
// `unit_name` then answers "unknown"; neither reads past the switch
// (T066).
auto test_closed_enumeration_defensive_close() -> void
{
  const auto outside = static_cast<unit>(99);
  const auto mapped = dimension_of(outside);
  check(!mapped.has_value(),
        "a unit value outside the enumeration maps to " "nothing");
  check(mapped.error().message == "unit value outside the closed enumeration",
        "the defensive close names the closed enumeration");
  check(mapped.error().suggestions.empty(),
        "the defensive close carries no suggestion");
  check(unit_name(outside) == "unknown",
        "a unit value outside the enumeration has no canonical name");
}

}  // namespace

auto main() -> int
{
  register_shape_fixture();
  test_unit_switch();
  test_unknown_unit_names_the_unit();
  test_unit_names_round_trip();
  test_metric_result_fields();
  test_error_shape();
  test_catalog_entry_shape();
  test_closed_enumeration_defensive_close();
  std::printf("counters_core_test PASS\n");
  return 0;
}
