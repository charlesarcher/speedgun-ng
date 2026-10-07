// ============================================================================
// TDD test for multi-object trees, selection, and fan-out (T029, T033;
// US3 scenarios 1..6).
//
// Covers enumeration facts (FR-001, FR-005), alias and canonical
// resolution to one object with canonical spelling in resolution,
// provenance, and diagnostics (FR-002, C-SYS-1, US3 scenario 2),
// objects(kind, filters) with AND-combined equality predicates and
// recoverable unknown-kind/key errors (FR-003, C-SYS-2), cross-object
// composition read within one sampling action (US3 scenario 4), fan-out
// plans folding one metric per object with instruction deltas
// reconciling the shared total (FR-047, SC-007), and duplicate
// registration rejection leaving the tree unchanged (FR-008, scenario
// 6). Hand-computed expectations, frameworkless check()/fail()
// convention. Duplicate-registration runs first: provider registration
// must precede the open boundary.
// ============================================================================

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS OBJECTS TEST FAIL: %s\n", what);
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

using sg::counters::Availability;
using sg::counters::CatalogEntry;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::FakeProvider;
using sg::counters::LeafSet;
using sg::counters::Object;
using sg::counters::ObjectSeed;
using sg::counters::ObjectSink;
using sg::counters::ProviderIface;
using sg::counters::System;
using sg::counters::Target;
using sg::counters::WindowReader;

using Events = Dim<0, 1>;
using TimeDim = Dim<1, 0>;

FakeProvider* probe = nullptr;

auto contains(std::string_view haystack, std::string_view needle) -> bool
{
  return haystack.find(needle) != std::string_view::npos;
}

auto joined(const std::vector<const Object*>& objs) -> std::string
{
  std::string out;
  for (const auto* one : objs) {
    out += std::string(one->path());
    out += ';';
  }
  return out;
}

auto testDuplicateRegistration() -> void
{
  auto colliding = std::make_unique<FakeProvider>();
  colliding->addObject("package-1", "package", "colliding package");
  const auto rejectedPath =
      System::local().registerProvider(std::move(colliding));
  check(!rejectedPath.has_value(), "duplicate path is rejected (FR-008)");
  check(contains(rejectedPath.error().message, "duplicate"),
        "the path rejection names the duplicate (FR-008)");

  auto doubled = std::make_unique<FakeProvider>();
  doubled->addCounter(
      "machine", "monotonic", "nanoseconds", "redeclared clock");
  const auto rejectedName =
      System::local().registerProvider(std::move(doubled));
  check(!rejectedName.has_value(),
        "duplicate counter name is rejected (FR-008)");
  check(contains(rejectedName.error().message, "duplicate"),
        "the name rejection names the duplicate (FR-008)");

  // Two objects of one registration claiming the same alias. The batch
  // search refuses the second, so the first keeps the alias (FR-002,
  // FR-008).
  auto batchClash = std::make_unique<FakeProvider>();
  batchClash->addObject("package-8/core-1", "cpu8", "core", "eighth core");
  batchClash->addObject("package-8/core-2", "cpu8", "core", "other core");
  const auto rejectedBatch =
      System::local().registerProvider(std::move(batchClash));
  check(!rejectedBatch.has_value(),
        "an alias held twice inside one registration is rejected (FR-002)");
  check(contains(rejectedBatch.error().message, "cpu8")
            && contains(rejectedBatch.error().message, "duplicate"),
        "the batch rejection names the colliding alias (FR-002)");

  // A second object claiming an alias another object already holds: the
  // alias resolves to the first registrant, so the collision is refused
  // and the tree stands unchanged (FR-002, FR-008).
  auto aliasClash = std::make_unique<FakeProvider>();
  aliasClash->addObject("package-9/core-1", "cpu3", "core", "ninth core");
  const auto rejectedAlias =
      System::local().registerProvider(std::move(aliasClash));
  check(!rejectedAlias.has_value(),
        "a duplicate platform alias is rejected (FR-002)");
  check(contains(rejectedAlias.error().message, "cpu3")
            && contains(rejectedAlias.error().message, "duplicate"),
        "the alias rejection names the colliding alias (FR-002)");

  // One seed batch declaring a canonical path twice, which
  // `FakeProvider` cannot express because it keys its objects by path.
  // The batch is refused whole, so no object lands and neither alias
  // survives (US3 scenario 6, FR-008).
  class Twice final : public ProviderIface
  {
  public:
    void enumerate(ObjectSink& sink) const override
    {
      sink.addObject(ObjectSeed {
          .kind = "core",
          .path = "package-3/core-11",
          .alias = "cpu11",
          .description = "eleventh core, first declaration",
          .entries = {},
      });
      sink.addObject(ObjectSeed {
          .kind = "core",
          .path = "package-3/core-11",
          .alias = "cpu12",
          .description = "eleventh core, second declaration",
          .entries = {},
      });
    }

    std::unique_ptr<WindowReader> open(const LeafSet& /*leaves*/,
                                       const Target& /*where*/) override
    {
      return nullptr;
    }
  };

  const auto rejectedBatchPath =
      System::local().registerProvider(std::make_unique<Twice>());
  check(!rejectedBatchPath.has_value(),
        "a canonical path declared twice in one batch is rejected "
        "(US3 scenario 6)");
  check(contains(rejectedBatchPath.error().message, "package-3/core-11")
            && contains(rejectedBatchPath.error().message, "duplicate"),
        "the batch rejection names the duplicated path (US3 scenario 6)");
  check(!System::local().object("package-3/core-11").has_value(),
        "the refused batch left no object at the duplicated path (FR-008)");
  check(!System::local().object("cpu11").has_value()
            && !System::local().object("cpu12").has_value(),
        "the refused batch left neither of its aliases behind (FR-008)");

  const auto held = System::local().object("cpu3");
  check(held.has_value() && held->path() == "package-1/core-3",
        "the held alias still resolves to its first registrant (FR-002)");

  const auto pkgs = System::local().objects("package");
  check(pkgs.has_value() && pkgs->size() == 2,
        "the tree stands unchanged after rejection (FR-008)");
  check(!System::local().object("package-9").has_value()
            && !System::local().object("package-8").has_value(),
        "the rejected registrations left no object behind (FR-008)");
}

auto testEnumeration() -> void
{
  const auto machine = *System::local().object("machine");
  check(machine.kind() == "machine", "machine is the root kind");
  check(machine.parent() == nullptr, "only the machine has no parent (FR-001)");
  check(machine.children().size() == 3,
        "machine children list the packages and the imc (FR-001)");

  const auto pkg1 = *System::local().object("package-1");
  check(pkg1.kind() == "package", "package kind (FR-001)");
  check(pkg1.path() == "package-1", "canonical path (FR-002)");
  check(pkg1.description() == "first package", "description (FR-001)");
  check(pkg1.alias().empty(), "an unaliased object prints no alias");
  check(pkg1.children().size() == 2, "package-1 has two core children");

  const auto core3 = *System::local().object("package-1/core-3");
  check(core3.kind() == "core", "core kind (FR-001)");
  check(core3.parent() != nullptr && core3.parent()->path() == "package-1",
        "the parent link points at the package (FR-001)");
  const auto entries = core3.counters();
  check(entries.size() == 2, "the catalog lists both counters (FR-005)");
  const auto cycles = std::ranges::find_if(
      entries, [](const CatalogEntry& e) { return e.name == "cycles"; });
  check(cycles != entries.end() && !cycles->description.empty()
            && cycles->avail == Availability::COUNTABLE,
        "the cycles entry carries description and state (FR-005, FR-006)");
}

// US3 scenario 2: both spellings resolve to one object, and every
// output the API produces for that object names the canonical path.
auto testAliasResolution() -> void
{
  const auto byAlias = *System::local().object("cpu3");
  check(byAlias.path() == "package-1/core-3",
        "the alias resolves to the canonical path (FR-002)");
  const auto byCanon = *System::local().object("package-1/core-3");
  check(byCanon.path() == byAlias.path(),
        "canonical and alias hits name one object (FR-002, C-SYS-1)");
  check(byAlias.alias() == "cpu3", "the declared alias is kept (FR-001)");
  check(byCanon.children().empty(), "a core has no children");

  // US3 scenario 2: the diagnostic and the provenance line both print
  // the canonical path, neither the alias the caller typed. Both come
  // from the fourth core, reached through its alias.
  const auto core4 = *System::local().object("cpu4");
  check(core4.path() == "package-1/core-4",
        "the cpu4 alias resolves to the canonical path (FR-002)");

  const auto missing = core4.counter<Events>("no_such_counter");
  check(!missing.has_value(), "an unknown counter name is refused (FR-008)");
  const std::string& message = missing.error().message;
  check(contains(message, "package-1/core-4"),
        "the resolution diagnostic names the canonical path (US3 scenario 2)");
  check(!contains(message, "cpu4"),
        "the resolution diagnostic prints no platform alias (US3 scenario 2)");

  // A dedicated leaf: every sampling action advances a leaf's script one
  // point, so this scenario must not touch another scenario's leaves.
  // Scripted {400, 1000}, so the fold is 1000 - 400 = 600.
  const Expression<Events> misses {*core4.counter<Events>("cache_misses")};
  auto compiled = compile(System::local(), misses);
  check(compiled.has_value(), "the alias-resolved plan compiles");
  auto rec = compiled->recorder(2);
  rec.sample();
  rec.sample();
  const auto provenance =
      misses.raw(rec.view(), "package-1/core-4", "cache_misses");
  check(provenance.has_value(),
        "the raw view resolves the canonical leaf address (FR-020)");
  check(provenance->objectPath == "package-1/core-4",
        "the provenance line reports the canonical path (US3 scenario 2)");
  check(!contains(provenance->objectPath, "cpu4"),
        "the provenance line prints no platform alias (US3 scenario 2)");
  check(provenance->count == 2 && provenance->points[0] == 400
            && provenance->points[1] == 1000,
        "the provenance line carries the raw scripted column (FR-020)");
  check(sameDouble(misses.fold(rec.view()).value, 600.0),
        "the same window folds 1000 - 400 to 600 (FR-018)");
}

auto testSelection() -> void
{
  const auto all = System::local().objects("core");
  check(all.has_value(), "core selection succeeds (FR-003)");
  check(joined(*all) == "package-1/core-3;package-1/core-4;package-2/core-7;",
        "selection returns exactly the cores (C-SYS-2)");
  const auto p1 = System::local().objects("core", {{"package", "1"}});
  check(p1.has_value() && joined(*p1) == "package-1/core-3;package-1/core-4;",
        "the package filter narrows to package-1 (FR-003)");
  const auto one =
      System::local().objects("core", {{"package", "2"}, {"core", "7"}});
  check(one.has_value() && joined(*one) == "package-2/core-7;",
        "filters combine with AND (FR-003)");
  const auto none =
      System::local().objects("core", {{"package", "1"}, {"core", "7"}});
  check(none.has_value() && none->empty(),
        "an unsatisfiable AND returns exactly nothing (FR-003)");
  const auto bogus = System::local().objects("accelerator");
  check(!bogus.has_value() && contains(bogus.error().message, "kind"),
        "unknown kind is a recoverable error (FR-003)");
  const auto badkey = System::local().objects("core", {{"socket", "1"}});
  check(!badkey.has_value() && contains(badkey.error().message, "filter"),
        "unknown filter key is a recoverable error (FR-003)");

  // A key the tree declares through a path component. The declaration
  // is per kind, so a key one kind spells is still unknown to another.
  const auto engines = System::local().objects("engine");
  check(engines.has_value()
            && joined(*engines) == "package-1/socket-0/accelerator-1;",
        "the deeper object is selectable by its own kind (FR-003)");
  const auto onSocket = System::local().objects("engine", {{"socket", "0"}});
  check(onSocket.has_value()
            && joined(*onSocket) == "package-1/socket-0/accelerator-1;",
        "a provider-declared attribute key selects its own object (FR-003)");
  const auto offSocket = System::local().objects("engine", {{"socket", "9"}});
  check(
      offSocket.has_value() && offSocket->empty(),
      "a declared key whose value matches nothing selects nothing " "(FR-003)");
  const auto socketOnCore = System::local().objects("core", {{"socket", "0"}});
  check(!socketOnCore.has_value(),
        "a key another kind declares stays unknown to this one (FR-003)");

  // A top-level object outside the package hierarchy hangs off the
  // machine root, which is its parent (FR-001).
  const auto imc = *System::local().object("uncore_imc_0");
  check(imc.parent() != nullptr && imc.parent()->path() == "machine",
        "a top-level object outside any package parents at the machine "
        "(FR-001)");
}

auto testCrossObjectComposition() -> void
{
  const auto imc = *System::local().object("uncore_imc_0");
  const auto bytes = *imc.counter<Events>("bytes");
  const auto machine = *System::local().object("machine");
  const auto mono = *machine.counter<TimeDim>("monotonic");
  const auto rate = bytes / mono;
  const auto compiled = compile(System::local(), rate);
  check(compiled.has_value(), "the cross-object plan compiles (FR-011)");
  const auto before = probe->readActions();
  sg::counters::Scope window {*compiled};
  window.start();
  window.finish();
  check(probe->readActions() - before == 2,
        "each window action reads both objects together (scenario 4)");
  const auto m = window.metric(rate);
  check(sameDouble(m.value, 0.002),
        "imc bytes over monotonic folds to 4000 / 2000000 (scenario 4)");
}

auto testFanoutReconciliation() -> void
{
  const auto all = System::local().objects("core");
  const auto core3 = *System::local().object("package-1/core-3");
  const auto c3 = *core3.counter<Events>("cycles");
  const auto i3 = *core3.counter<Events>("instructions");
  const auto ipc = i3 / c3;
  auto fanout = compile(System::local(), ipc, *all);
  check(fanout.has_value(), "the fan-out ipc plan compiles (FR-047)");
  const auto paths = fanout->objectPaths();
  check(paths.size() == 3 && paths[0] == "package-1/core-3"
            && paths[2] == "package-2/core-7",
        "fan-out keys objects by canonical path in selection order (FR-002)");
  auto rec = fanout->recorder(2);
  rec.sample();
  rec.sample();
  const auto results = fanout->fold(ipc, rec.view());
  check(results.size() == 3, "one metric per selected object (SC-007)");
  check(results[0].objectPath == "package-1/core-3"
            && sameDouble(results[0].metric.value, 10.5),
        "core-3 ipc folds 2100 / 200 (FR-047)");
  check(sameDouble(results[1].metric.value, 4.5),
        "core-4 ipc folds 900 / 200 (FR-047)");
  check(sameDouble(results[2].metric.value, 14.0),
        "core-7 ipc folds 1400 / 100 (FR-047)");

  // A fan-out plan assigned to itself: the assignment guards on identity,
  // so the layout and its selection survive and the same recorder folds to
  // the same numbers afterwards (FR-022).
  auto rec2 = fanout->recorder(2);
  rec2.sample();
  rec2.sample();
  const auto before = fanout->fold(ipc, rec2.view());
  auto& alias = *fanout;
  alias = std::move(*fanout);
  const auto after = fanout->fold(ipc, rec2.view());
  check(after.size() == before.size() && after.size() == 3,
        "a self-move-assigned fan-out keeps its selection (FR-022)");
  bool identical = true;
  for (std::size_t index = 0; index < after.size(); ++index) {
    identical = identical && after[index].objectPath == before[index].objectPath
        && sameDouble(after[index].metric.value, before[index].metric.value);
  }
  check(identical,
        "a self-move-assigned fan-out folds its per-object metrics exactly "
        "as before (FR-022)");

  const auto d3 = Expression<Events> {i3}.fold(rec.view());
  const auto core4 = *System::local().object("package-1/core-4");
  const auto i4 = *core4.counter<Events>("instructions");
  const auto d4 = Expression<Events> {i4}.fold(rec.view());
  const auto core7 = *System::local().object("package-2/core-7");
  const auto i7 = *core7.counter<Events>("instructions");
  const auto d7 = Expression<Events> {i7}.fold(rec.view());
  check(sameDouble(d3.value, 2100.0) && sameDouble(d4.value, 900.0)
            && sameDouble(d7.value, 1400.0),
        "per-core instruction deltas come from the shared window (FR-047)");

  const auto machine = *System::local().object("machine");
  const auto total = *machine.counter<Events>("total_instructions");
  const auto tplan = compile(System::local(), Expression<Events> {total});
  check(tplan.has_value(), "the shared-total plan compiles");
  auto trec = tplan->recorder(2);
  trec.sample();
  trec.sample();
  const auto tv = Expression<Events> {total}.fold(trec.view());
  check(sameDouble(d3.value + d4.value + d7.value, tv.value),
        "core deltas reconcile the shared total (SC-007)");
  check(sameDouble(tv.value, 4400.0), "the shared delta is 4400 (SC-007)");
}

auto registerEverything() -> void
{
  auto provider = std::make_unique<FakeProvider>();
  provider->addObject("package-1", "package", "first package");
  provider->addObject("package-2", "package", "second package");
  provider->addObject("package-1/core-3", "cpu3", "core", "third core");
  provider->addObject("package-1/core-4", "cpu4", "core", "fourth core");
  provider->addObject("package-2/core-7", "core", "seventh core");
  provider->addObject("uncore_imc_0", "imc", "memory controller");
  // A deeper path carrying provider-declared attribute keys: the path
  // components spell the attributes, so a filter on either key is a
  // defined key for this kind (FR-003, T097).
  provider->addObject(
      "package-1/socket-0/accelerator-1", "engine", "first engine");
  provider->addCounter(
      "machine", "monotonic", "nanoseconds", "monotonic wall clock");
  provider->addCounter(
      "machine", "total_instructions", "ops", "shared instruction total");
  provider->addCounter("uncore_imc_0", "bytes", "bytes", "DRAM bytes read");
  provider->addCounter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->addCounter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  provider->setPoints("package-1/core-3", "cycles", {100, 300}, 0);
  provider->setPoints("package-1/core-3", "instructions", {1000, 3100}, 0);
  provider->addCounter(
      "package-1/core-4", "cycles", "ops", "core cycles elapsed");
  provider->addCounter(
      "package-1/core-4", "instructions", "ops", "instructions retired");
  provider->addCounter(
      "package-1/core-4", "cache_misses", "ops", "last-level cache misses");
  provider->setPoints("package-1/core-4", "cycles", {50, 250}, 0);
  provider->setPoints("package-1/core-4", "instructions", {2000, 2900}, 0);
  provider->setPoints("package-1/core-4", "cache_misses", {400, 1000, 1600}, 0);
  provider->addCounter(
      "package-2/core-7", "cycles", "ops", "core cycles elapsed");
  provider->addCounter(
      "package-2/core-7", "instructions", "ops", "instructions retired");
  provider->setPoints("package-2/core-7", "cycles", {10, 110}, 0);
  provider->setPoints("package-2/core-7", "instructions", {500, 1900}, 0);
  provider->setPoints("machine", "monotonic", {0, 2000000}, 0);
  provider->setPoints("machine", "total_instructions", {100000, 104400}, 0);
  provider->setPoints("uncore_imc_0", "bytes", {1000, 5000}, 0);
  probe = provider.get();
  const auto registered = System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the scripted provider registers");
}

}  // namespace

auto main() -> int
{
  registerEverything();
  testDuplicateRegistration();
  testEnumeration();
  testAliasResolution();
  testSelection();
  testCrossObjectComposition();
  testFanoutReconciliation();
  std::printf("counters objects tests passed\n");
  return 0;
}
