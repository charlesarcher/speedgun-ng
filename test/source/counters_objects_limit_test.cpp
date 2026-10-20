// ============================================================================
// The programmable-counter limit as a published fact of an object (specs/024
// T010; FR-005, FR-006, data-model E-02,
// contracts/programmable-counters-fact.md).
//
// A scripted fake object publishes through `Object::programmableCounters()`
// the limit scripted for its path; an object nobody scripted publishes
// nothing; the `machine` clock object publishes nothing, because a clock
// consumes no programmable counter; two scripted sources carry two different
// limits, which is the fake's per-object-path rule. Every case reads a
// scripted fake, so no case needs a kernel counter or a privilege.
// Registration closes at the first catalog read (FR-009), so every provider
// this suite needs is registered before its first assertion. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <utility>

#include "speedgun-ng/counters.hpp"

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS OBJECTS LIMIT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

using sg::counters::FakeProvider;
using sg::counters::System;

// The scripted world: one provider owning a PMU-shaped object with a scripted
// limit, a device object nobody scripts, and the machine clock root; a second
// provider owning a second PMU-shaped object with a different scripted limit.
auto registerEverything() -> void
{
  auto pmu = std::make_unique<FakeProvider>();
  pmu->addObject("cpu", "cpu", "performance core cluster");
  pmu->addCounter("cpu", "cycles", "ops", "core cycles elapsed");
  pmu->setProgrammableCounters("cpu", 4);
  pmu->addObject("uncore_imc_0", "imc", "memory controller");
  pmu->addCounter("uncore_imc_0", "bytes", "bytes", "DRAM bytes read");
  pmu->addCounter(
      "machine", "monotonic", "nanoseconds", "monotonic wall clock");
  const auto first = System::local().registerProvider(std::move(pmu));
  check(first.has_value(), "the scripted provider registers");

  // The second scripted source, carrying its own limit: the script names an
  // object path, so each source answers with the limit scripted on it.
  auto atom = std::make_unique<FakeProvider>();
  atom->addObject("cpu_atom", "cpu", "efficient core cluster");
  atom->addCounter("cpu_atom", "cycles", "ops", "core cycles elapsed");
  atom->setProgrammableCounters("cpu_atom", 2);
  const auto second = System::local().registerProvider(std::move(atom));
  check(second.has_value(), "the second scripted provider registers");
}

// FR-005, FR-006: what the fake scripted for a path is what that object
// publishes.
auto scriptedObjectPublishesItsLimit() -> void
{
  const auto cpu = *System::local().object("cpu");
  const auto limit = cpu.programmableCounters();
  check(limit.has_value(),
        "a scripted fake object publishes a limit (FR-005, FR-006)");
  check(limit.value_or(0) == 4,
        "the published limit is the scripted count (FR-006)");
}

// FR-005: a source nobody scripted answers nothing.
auto unscriptedObjectPublishesNothing() -> void
{
  const auto imc = *System::local().object("uncore_imc_0");
  check(imc.programmableCounters() == std::nullopt,
        "an unscripted object publishes no limit (FR-005)");
}

// Data-model E-02: a clock object consumes no programmable counter, so it
// answers nothing whatever the other scripts of its provider say.
auto machineClockObjectPublishesNothing() -> void
{
  const auto machine = *System::local().object("machine");
  check(machine.kind() == "machine", "the clock root is the machine object");
  check(machine.programmableCounters() == std::nullopt,
        "the machine clock object publishes no limit (E-02)");
}

// The two-provider shape: each scripted source carries its own limit.
auto twoScriptedSourcesCarryTwoDifferentLimits() -> void
{
  const auto cores = *System::local().object("cpu");
  const auto atoms = *System::local().object("cpu_atom");
  const auto coreLimit = cores.programmableCounters();
  const auto atomLimit = atoms.programmableCounters();
  check(coreLimit.has_value() && atomLimit.has_value(),
        "both scripted sources publish a limit (FR-006)");
  check(coreLimit.value_or(0) == 4 && atomLimit.value_or(0) == 2,
        "each scripted source publishes its own scripted count (FR-006)");
  check(coreLimit != atomLimit,
        "two scripted sources carry two different limits (FR-006)");
}

}  // namespace

auto main() -> int
{
  registerEverything();
  scriptedObjectPublishesItsLimit();
  unscriptedObjectPublishesNothing();
  machineClockObjectPublishesNothing();
  twoScriptedSourcesCarryTwoDifferentLimits();
  std::printf("counters_objects_limit_test: ok\n");
  return 0;
}
