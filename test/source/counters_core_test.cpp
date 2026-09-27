// ============================================================================
// TDD test for the counters core vocabulary (T009).
//
// Covers the dimension tag algebra (T005), the closed unit switch (T006),
// and the catalog/result/error shapes (T007). Hand-computed expectations,
// frameworkless check()/fail() convention.
// ============================================================================

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "speedgun-ng/counters_core.hpp"

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
using sg::counters::dim;
using sg::counters::dim_quotient;
using sg::counters::dim_same;
using sg::counters::dimension_of;
using sg::counters::error;
using sg::counters::metric_result;
using sg::counters::read_mode;
using sg::counters::unit;
using sg::counters::unit_from_token;
using sg::counters::unit_name;

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

auto test_metric_result_fields() -> void
{
  const metric_result result {
      .value = 3.25, .running_ratio = 0.5, .scaled = true};
  check(same_double(result.value, 3.25), "value field");
  check(same_double(result.running_ratio, 0.5), "running_ratio field");
  check(result.scaled, "scaled field");

  const metric_result defaults {};
  check(same_double(defaults.running_ratio, 1.0) && !defaults.scaled,
        "a default result discloses ratio 1.0 scaled false");
}

auto test_error_shape() -> void
{
  error e {.message = "no such counter 'instruction'",
           .suggestions = {"instructions", "cache-instructions"}};
  check(e.message == "no such counter 'instruction'", "error message");
  check(e.suggestions.size() == 2, "suggestion list size");
  check(e.suggestions[0] == "instructions", "first suggestion");
}

auto test_catalog_entry_shape() -> void
{
  const catalog_entry entry {.name = "cycles",
                             .description = "core cycles elapsed",
                             .unit = unit::none,
                             .avail = availability::countable,
                             .mode = read_mode::syscall};
  check(entry.name == "cycles", "entry name");
  check(entry.description == "core cycles elapsed", "entry description");
  check(entry.unit == unit::none, "entry unit");
  check(entry.avail == availability::countable, "entry availability");
  check(entry.mode == read_mode::syscall, "entry read mode");
}

// The defensive close of the closed unit enumeration, which the header
// documents as the failure branch of `dimension_of` and as the fallback
// `unit_name` returns. A value outside the enumeration is reachable only
// through a cast, and both functions must then refuse or say "unknown"
// rather than read past the switch (T066).
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
