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
auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::Availability;
using sg::counters::CatalogEntry;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::dimensionOf;
using sg::counters::DimQuotient;
using sg::counters::FakeProvider;
using sg::counters::kDimSame;
using sg::counters::ReadMode;
using sg::counters::System;
using sg::counters::Unit;
using sg::counters::unitFromToken;
using sg::counters::unitName;

using Events = Dim<0, 1>;

// Compile-time dimension algebra (T005, FR-015).
static_assert(kDimSame<Dim<0, 1>, Dim<0, 1>>, "identical tags are the same");
static_assert(!kDimSame<Dim<0, 1>, Dim<1, 0>>, "events^1 differs from time^1");
static_assert(kDimSame<DimQuotient<Dim<0, 1>, Dim<0, 1>>, Dim<0, 0>>,
              "instructions / cycles is dim<0,0>");
static_assert(kDimSame<DimQuotient<Dim<0, 1>, Dim<1, 0>>, Dim<-1, 1>>,
              "bytes / monotonic is dim<-1,1>");
static_assert(kDimSame<DimQuotient<Dim<1, 0>, Dim<1, 0>>, Dim<0, 0>>,
              "monotonic / monotonic is dim<0,0>");

auto testUnitSwitch() -> void
{
  const Unit seconds = *unitFromToken("seconds");
  check(seconds == Unit::SECONDS, "seconds token maps to unit::seconds");

  const auto dims = dimensionOf(*unitFromToken("nanoseconds"));
  check(dims.has_value(), "nanoseconds resolves");
  check(dims->time == 1 && dims->events == 0, "nanoseconds is time^1");

  const auto bytes = dimensionOf(*unitFromToken("bytes"));
  check(bytes.has_value() && bytes->time == 0 && bytes->events == 1,
        "bytes is events^1");

  const auto ops = dimensionOf(*unitFromToken("ops"));
  check(ops.has_value() && ops->time == 0 && ops->events == 1,
        "ops is events^1");

  const auto none = dimensionOf(*unitFromToken("none"));
  check(none.has_value() && none->time == 0 && none->events == 1,
        "none is events^1");

  const auto secondsDim = dimensionOf(Unit::SECONDS);
  check(secondsDim.has_value() && secondsDim->time == 1
            && secondsDim->events == 0,
        "seconds is time^1");
}

auto testUnknownUnitNamesTheUnit() -> void
{
  const auto bad = unitFromToken("furlongs");
  check(!bad.has_value(), "unrecognized unit is rejected");
  check(bad.error().message.find("furlongs") != std::string::npos,
        "error message names the rejected unit");
  check(bad.error().suggestions.empty(),
        "unit errors carry no catalog suggestions");
}

auto testUnitNamesRoundTrip() -> void
{
  check(unitName(Unit::SECONDS) == "seconds", "seconds spelling");
  check(unitName(Unit::NANOSECONDS) == "nanoseconds", "nanoseconds spelling");
  check(unitName(Unit::BYTES) == "bytes", "bytes spelling");
  check(unitName(Unit::OPS) == "ops", "ops spelling");
  check(unitName(Unit::NONE) == "none", "none spelling");
}

// One provider registered before the open boundary, so the three shape
// tests below read the entries, errors, and results the library built
// (T007, FR-005, FR-008, FR-019).
auto registerShapeFixture() -> void
{
  auto provider = std::make_unique<FakeProvider>();
  provider->addObject("core-0", "cpu0", "core", "the core under test");
  provider->addCounter("core-0", "cycles", "ops", "core cycles elapsed");
  provider->addCounter("core-0", "retired", "ops", "instructions retired");
  provider->setPoints("core-0", "cycles", {0, 200}, 200);
  provider->setPoints("core-0", "retired", {0, 6300}, 6300);
  const auto registered = System::local().registerProvider(std::move(provider));
  check(registered.has_value(),
        "the shape fixture registers before open (FR-009)");
}

// The fold output: the value is the hand-computed quotient of the two
// scripted window deltas, and the two disclosure fields carry the
// window's multiplex state (FR-019, FR-030).
auto testMetricResultFields() -> void
{
  const auto core = *System::local().object("core-0");
  const auto cycles = *core.counter<Events>("cycles");
  const auto retired = *core.counter<Events>("retired");
  const auto perCycle = retired / cycles;
  const auto compiled = compile(System::local(), perCycle);
  check(compiled.has_value(), "the quotient plan compiles (FR-021)");

  sg::counters::Scope window {*compiled};
  window.start();
  window.finish();
  // 6300 retired over 200 cycles, the two scripted steps committed
  // beside the fixture above.
  const auto metric = window.metric(perCycle);
  check(sameDouble(metric.value, 31.5),
        "the fold value is the hand-computed quotient 6300 / 200 (FR-030)");
  check(sameDouble(metric.runningRatio, 1.0) && !metric.scaled,
        "a window with no multiplexed source discloses ratio 1.0 and "
        "unscaled ticks (FR-019)");
}

// The recoverable error the resolution path builds: it names the object
// and the rejected name, and the suggestion list is the near-miss scan
// the library ranked (FR-008).
auto testErrorShape() -> void
{
  const auto core = *System::local().object("core-0");
  const auto mistyped = core.counter<Events>("cycle");
  check(!mistyped.has_value(), "an undeclared counter name is refused");
  const auto& reported = mistyped.error();
  check(reported.message.find("core-0") != std::string::npos
            && reported.message.find("cycle") != std::string::npos,
        "the error names the object and the rejected name (FR-008)");
  check(reported.suggestions.size() == 1
            && reported.suggestions[0] == "cycles",
        "the ranked list offers the one name within the edit distance "
        "(FR-008)");
  const auto byWord = core.counter<Events>("elapsed");
  check(!byWord.has_value() && byWord.error().suggestions.size() == 1
            && byWord.error().suggestions[0] == "cycles",
        "a distant query falls back to the description word overlap (FR-008)");
}

// The catalog entry the system builds from one provider counter: the
// declared fields travel, and the two disclosure defaults a provider
// leaves unset travel as the header declares them (FR-005, FR-017).
auto testCatalogEntryShape() -> void
{
  const auto core = *System::local().object("core-0");
  const auto entries = core.counters();
  check(entries.size() == 2,
        "the object reports its two registered counters (FR-001)");
  const auto found = std::ranges::find_if(entries,
                                          [](const CatalogEntry& entry)
                                          { return entry.name == "retired"; });
  check(found != entries.end(), "the registered counter is in the catalog");
  check(found->description == "instructions retired"
            && found->unit == Unit::OPS
            && found->avail == Availability::COUNTABLE
            && found->mode == ReadMode::SYSCALL,
        "the entry carries the declared description, unit, state, and read "
        "mode (FR-005, FR-006)");
  check(found->frequencyHz == 0 && !found->scaled,
        "a counter declaring neither calibration nor a scaled tick source "
        "discloses frequency 0 and unscaled (FR-019)");
}

// The defensive close of the closed unit enumeration, which the header
// documents as the failure branch of `dimensionOf` and as the fallback
// `unitName` returns. A value outside the enumeration is reachable only
// through a cast. `dimensionOf` then refuses with an error and
// `unitName` then answers "unknown"; neither reads past the switch
// (T066).
auto testClosedEnumerationDefensiveClose() -> void
{
  const auto outside = static_cast<Unit>(99);
  const auto mapped = dimensionOf(outside);
  check(!mapped.has_value(),
        "a unit value outside the enumeration maps to " "nothing");
  check(mapped.error().message == "unit value outside the closed enumeration",
        "the defensive close names the closed enumeration");
  check(mapped.error().suggestions.empty(),
        "the defensive close carries no suggestion");
  check(unitName(outside) == "unknown",
        "a unit value outside the enumeration has no canonical name");
}

}  // namespace

auto main() -> int
{
  registerShapeFixture();
  testUnitSwitch();
  testUnknownUnitNamesTheUnit();
  testUnitNamesRoundTrip();
  testMetricResultFields();
  testErrorShape();
  testCatalogEntryShape();
  testClosedEnumerationDefensiveClose();
  std::printf("counters_core_test PASS\n");
  return 0;
}
