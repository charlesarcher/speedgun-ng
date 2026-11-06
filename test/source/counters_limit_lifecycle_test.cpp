// ============================================================================
// The programmable-counter limit table as the state of one `System` (specs/050
// T003, T011, T014; FR-002, FR-004, FR-005, FR-006, FR-008, FR-009,
// data-model E-01, contracts/limit-table-ownership.md).
//
// One `System` owns one limit table, and that table stands inside the private
// `System::Impl` of the `System` that filled it. A scripted limit therefore
// answers the reads of its own `System` alone, and it reaches no table of
// another `System`. An object whose event source answered nothing holds no
// record, so its read answers no value. FR-008 puts the `System` constructor
// on the normal interface, so each case below builds its own objects. Each
// case registers its fake providers before its first read, and every read
// names a path its own fake declared. No case asserts a timing, and no case
// sleeps, because the table answers from catalog state.
//
// The three cross-`System` cases declare the path they read on the `System`
// that reads it. That shape makes a red run measure the table behind the read
// (SC-001, Clarification 1). Each of the five cases keeps its whole world
// inside one function, and `main` calls each of them exactly once.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstddef>
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
  std::fprintf(stderr, "COUNTERS LIMIT LIFECYCLE TEST FAIL: %s\n", what);
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

// The limit that leaves one object unscripted, because
// `setProgrammableCounters` takes a count above zero at every call (FR-006).
constexpr std::size_t kUnscripted = 0;

// One script for one `System`: one fake owns one object of kind `kind`, and
// the call scripts `limit` on it. The object carries one leaf named
// `cycles`, and a `limit` of `kUnscripted` leaves it unscripted (FR-006).
auto script(System& system,
            const char* path,
            const char* kind,
            const std::size_t limit) -> void
{
  auto provider = std::make_unique<FakeProvider>();
  provider->addObject(path, kind, "scripted object");
  provider->addCounter(path, "cycles", "ops", "core cycles elapsed");
  if (limit != kUnscripted) {
    provider->setProgrammableCounters(path, limit);
  }
  const auto registered = system.registerProvider(std::move(provider));
  check(registered.has_value(), "the scripted provider registers");
}

// This read answers the limit `Object::programmableCounters` gives for
// `path`, so every case reaches the fact through the shipped interface. The
// `object` call opens the catalog of that `System`, so each read follows the
// open of its own catalog (D-6).
auto readLimit(System& system, const char* path) -> std::optional<std::size_t>
{
  const auto object = system.object(path);
  check(object.has_value(), "the declared path resolves to an object");
  return (*object).programmableCounters();
}

// FR-004: the first `System` scripts 4 and the second scripts 2 on one path,
// and each one reads back the count it scripted.
auto caseTwoSystemsCarryTheirOwnLimits() -> void
{
  System first;
  script(first, "cpu", "cpu", 4);
  System second;
  script(second, "cpu", "cpu", 2);

  const auto firstLimit = readLimit(first, "cpu");
  check(firstLimit.has_value(), "the first System answers a limit (FR-004)");
  check(firstLimit.value_or(0) == 4,
        "the first System answers the count it scripted (FR-004)");

  const auto secondLimit = readLimit(second, "cpu");
  check(secondLimit.has_value(), "the second System answers a limit (FR-004)");
  check(secondLimit.value_or(0) == 2,
        "the second System answers the count it scripted (FR-004)");

  const auto firstAgain = readLimit(first, "cpu");
  check(firstAgain.value_or(0) == 4,
        "the first System keeps its count while both live (FR-004)");
}

// FR-006: the second `System` declares `cpu` with one leaf and scripts
// nothing while the first scripts 4, and the read of the second answers
// nothing.
auto caseDeclaredPathWithoutScriptAnswersNothing() -> void
{
  System first;
  script(first, "cpu", "cpu", 4);
  System second;
  script(second, "cpu", "cpu", kUnscripted);

  const auto secondLimit = readLimit(second, "cpu");
  check(!secondLimit.has_value(),
        "a declared path with no script answers nothing (FR-006)");

  const auto firstLimit = readLimit(first, "cpu");
  check(firstLimit.value_or(0) == 4,
        "the first System still answers its own count (FR-004)");
}

// FR-002 keeps the single-`System` answers where they stand, so this case
// reads one built `System`. A scripted object, an unscripted object and the
// `machine` root answer exactly the base readings.
auto caseOneSystemAnswersAsAtTheBase() -> void
{
  System system;
  script(system, "cpu", "cpu", 4);

  // The second provider carries the two objects no script limits: one device
  // object with one leaf, and one leaf under the machine root.
  auto extra = std::make_unique<FakeProvider>();
  extra->addObject("uncore_imc_0", "imc", "memory controller");
  extra->addCounter("uncore_imc_0", "bytes", "bytes", "DRAM bytes read");
  extra->addCounter(
      "machine", "monotonic", "nanoseconds", "monotonic wall clock");
  const auto registered = system.registerProvider(std::move(extra));
  check(registered.has_value(), "the second scripted provider registers");

  const auto cpu = readLimit(system, "cpu");
  check(cpu.value_or(0) == 4, "a scripted object answers its count (FR-002)");

  const auto imc = readLimit(system, "uncore_imc_0");
  check(!imc.has_value(), "an unscripted object answers nothing (FR-002)");

  const auto root = system.object("machine");
  check(root.has_value(), "a built System holds the machine root");
  const auto machine = *root;
  check(machine.kind() == "machine", "the clock root is the machine object");
  check(!machine.programmableCounters().has_value(),
        "the machine clock root answers nothing (FR-002)");
}

// FR-009: each walk read answers the tree of the `System` that owns the
// handle's node. One built `System` holds `machine`, `cpu` and `cpu/core-0`,
// and the process-shared local `System` holds its `machine` root alone.
auto caseTheWalkReadsStayOnTheirOwnTree() -> void
{
  System built;
  auto tree = std::make_unique<FakeProvider>();
  tree->addObject("cpu", "cpu", "one scripted cluster");
  tree->addCounter("cpu", "cycles", "ops", "core cycles elapsed");
  tree->addObject("cpu/core-0", "core", "one core under the cluster");
  tree->addCounter("cpu/core-0", "cycles", "ops", "core cycles elapsed");
  const auto registered = built.registerProvider(std::move(tree));
  check(registered.has_value(), "the two-object walk provider registers");

  const auto core = built.object("cpu/core-0");
  check(core.has_value(), "the built tree holds the core object");
  const auto cpu = built.object("cpu");
  check(cpu.has_value(), "the built tree holds the cpu object");
  const auto root = built.object("machine");
  check(root.has_value(), "the built tree holds the machine root");

  const auto* coreParent = (*core).parent();
  check(coreParent != nullptr && coreParent->path() == "cpu",
        "the built core answers its own cpu as parent (FR-009)");
  check((*cpu).children().size() == 1,
        "the built cpu answers one child of its own tree (FR-009)");
  check((*root).parent() == nullptr,
        "the built machine root answers no parent (FR-009)");
  check((*core).children().empty(),
        "the built core answers no child of its own tree (FR-009)");

  // No case registers a provider into the local `System`, so that tree holds
  // the `machine` root alone and both walk reads answer an empty tree.
  const auto shared = System::local().object("machine");
  check(shared.has_value(), "the local System holds the machine root");
  check((*shared).parent() == nullptr,
        "the local machine root answers no parent (FR-009)");
  check((*shared).children().empty(),
        "the local machine root answers no child (FR-009)");
}

// FR-005 and FR-006: the table one `System` filled ends with it, so a fresh
// `System` over that path reads an empty table. One scoped `System` scripts 8
// for `cpu` and then ends, and the fresh `System` afterwards declares that
// path with one leaf and scripts nothing. That fresh read answers no value,
// and a third `System` that scripts 1 answers 1, the second scenario of US-2.
auto caseAnEndedSystemLeavesNoEntry() -> void
{
  {
    System ended;
    script(ended, "cpu", "cpu", 8);
  }

  System fresh;
  script(fresh, "cpu", "cpu", kUnscripted);
  const auto freshLimit = readLimit(fresh, "cpu");
  check(!freshLimit.has_value(),
        "the ended System left no entry behind (FR-005, FR-006)");

  System later;
  script(later, "cpu", "cpu", 1);
  const auto laterLimit = readLimit(later, "cpu");
  check(laterLimit.value_or(0) == 1,
        "a later System answers its own scripted count (US-2)");
}

}  // namespace

auto main() -> int
{
  caseTwoSystemsCarryTheirOwnLimits();
  caseDeclaredPathWithoutScriptAnswersNothing();
  caseOneSystemAnswersAsAtTheBase();
  caseTheWalkReadsStayOnTheirOwnTree();
  caseAnEndedSystemLeavesNoEntry();
  std::printf("counters_limit_lifecycle_test: ok\n");
  return 0;
}
