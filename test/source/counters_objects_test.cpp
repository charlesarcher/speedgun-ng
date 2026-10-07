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

auto same_double(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::Availability;
using sg::counters::CatalogEntry;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::expression;
using sg::counters::fake_provider;
using sg::counters::leaf_set;
using sg::counters::object;
using sg::counters::object_seed;
using sg::counters::object_sink;
using sg::counters::provider_iface;
using sg::counters::system;
using sg::counters::target;
using sg::counters::window_reader;

using events = Dim<0, 1>;
using time_dim = Dim<1, 0>;

fake_provider* probe = nullptr;

auto contains(std::string_view haystack, std::string_view needle) -> bool
{
  return haystack.find(needle) != std::string_view::npos;
}

auto joined(const std::vector<const object*>& objs) -> std::string
{
  std::string out;
  for (const auto* one : objs) {
    out += std::string(one->path());
    out += ';';
  }
  return out;
}

auto test_duplicate_registration() -> void
{
  auto colliding = std::make_unique<fake_provider>();
  colliding->add_object("package-1", "package", "colliding package");
  const auto rejected_path =
      system::local().register_provider(std::move(colliding));
  check(!rejected_path.has_value(), "duplicate path is rejected (FR-008)");
  check(contains(rejected_path.error().message, "duplicate"),
        "the path rejection names the duplicate (FR-008)");

  auto doubled = std::make_unique<fake_provider>();
  doubled->add_counter(
      "machine", "monotonic", "nanoseconds", "redeclared clock");
  const auto rejected_name =
      system::local().register_provider(std::move(doubled));
  check(!rejected_name.has_value(),
        "duplicate counter name is rejected (FR-008)");
  check(contains(rejected_name.error().message, "duplicate"),
        "the name rejection names the duplicate (FR-008)");

  // Two objects of one registration claiming the same alias. The batch
  // search refuses the second, so the first keeps the alias (FR-002,
  // FR-008).
  auto batch_clash = std::make_unique<fake_provider>();
  batch_clash->add_object("package-8/core-1", "cpu8", "core", "eighth core");
  batch_clash->add_object("package-8/core-2", "cpu8", "core", "other core");
  const auto rejected_batch =
      system::local().register_provider(std::move(batch_clash));
  check(!rejected_batch.has_value(),
        "an alias held twice inside one registration is rejected (FR-002)");
  check(contains(rejected_batch.error().message, "cpu8")
            && contains(rejected_batch.error().message, "duplicate"),
        "the batch rejection names the colliding alias (FR-002)");

  // A second object claiming an alias another object already holds: the
  // alias resolves to the first registrant, so the collision is refused
  // and the tree stands unchanged (FR-002, FR-008).
  auto alias_clash = std::make_unique<fake_provider>();
  alias_clash->add_object("package-9/core-1", "cpu3", "core", "ninth core");
  const auto rejected_alias =
      system::local().register_provider(std::move(alias_clash));
  check(!rejected_alias.has_value(),
        "a duplicate platform alias is rejected (FR-002)");
  check(contains(rejected_alias.error().message, "cpu3")
            && contains(rejected_alias.error().message, "duplicate"),
        "the alias rejection names the colliding alias (FR-002)");

  // One seed batch declaring a canonical path twice, which
  // `fake_provider` cannot express because it keys its objects by path.
  // The batch is refused whole, so no object lands and neither alias
  // survives (US3 scenario 6, FR-008).
  class twice final : public provider_iface
  {
  public:
    void enumerate(object_sink& sink) const override
    {
      sink.add_object(object_seed {
          .kind = "core",
          .path = "package-3/core-11",
          .alias = "cpu11",
          .description = "eleventh core, first declaration",
          .entries = {},
      });
      sink.add_object(object_seed {
          .kind = "core",
          .path = "package-3/core-11",
          .alias = "cpu12",
          .description = "eleventh core, second declaration",
          .entries = {},
      });
    }

    std::unique_ptr<window_reader> open(const leaf_set& /*leaves*/,
                                        const target& /*where*/) override
    {
      return nullptr;
    }
  };

  const auto rejected_batch_path =
      system::local().register_provider(std::make_unique<twice>());
  check(!rejected_batch_path.has_value(),
        "a canonical path declared twice in one batch is rejected "
        "(US3 scenario 6)");
  check(contains(rejected_batch_path.error().message, "package-3/core-11")
            && contains(rejected_batch_path.error().message, "duplicate"),
        "the batch rejection names the duplicated path (US3 scenario 6)");
  check(!system::local().object("package-3/core-11").has_value(),
        "the refused batch left no object at the duplicated path (FR-008)");
  check(!system::local().object("cpu11").has_value()
            && !system::local().object("cpu12").has_value(),
        "the refused batch left neither of its aliases behind (FR-008)");

  const auto held = system::local().object("cpu3");
  check(held.has_value() && held->path() == "package-1/core-3",
        "the held alias still resolves to its first registrant (FR-002)");

  const auto pkgs = system::local().objects("package");
  check(pkgs.has_value() && pkgs->size() == 2,
        "the tree stands unchanged after rejection (FR-008)");
  check(!system::local().object("package-9").has_value()
            && !system::local().object("package-8").has_value(),
        "the rejected registrations left no object behind (FR-008)");
}

auto test_enumeration() -> void
{
  const auto machine = *system::local().object("machine");
  check(machine.kind() == "machine", "machine is the root kind");
  check(machine.parent() == nullptr, "only the machine has no parent (FR-001)");
  check(machine.children().size() == 3,
        "machine children list the packages and the imc (FR-001)");

  const auto pkg1 = *system::local().object("package-1");
  check(pkg1.kind() == "package", "package kind (FR-001)");
  check(pkg1.path() == "package-1", "canonical path (FR-002)");
  check(pkg1.description() == "first package", "description (FR-001)");
  check(pkg1.alias().empty(), "an unaliased object prints no alias");
  check(pkg1.children().size() == 2, "package-1 has two core children");

  const auto core3 = *system::local().object("package-1/core-3");
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
auto test_alias_resolution() -> void
{
  const auto by_alias = *system::local().object("cpu3");
  check(by_alias.path() == "package-1/core-3",
        "the alias resolves to the canonical path (FR-002)");
  const auto by_canon = *system::local().object("package-1/core-3");
  check(by_canon.path() == by_alias.path(),
        "canonical and alias hits name one object (FR-002, C-SYS-1)");
  check(by_alias.alias() == "cpu3", "the declared alias is kept (FR-001)");
  check(by_canon.children().empty(), "a core has no children");

  // US3 scenario 2: the diagnostic and the provenance line both print
  // the canonical path, neither the alias the caller typed. Both come
  // from the fourth core, reached through its alias.
  const auto core4 = *system::local().object("cpu4");
  check(core4.path() == "package-1/core-4",
        "the cpu4 alias resolves to the canonical path (FR-002)");

  const auto missing = core4.counter<events>("no_such_counter");
  check(!missing.has_value(), "an unknown counter name is refused (FR-008)");
  const std::string& message = missing.error().message;
  check(contains(message, "package-1/core-4"),
        "the resolution diagnostic names the canonical path (US3 scenario 2)");
  check(!contains(message, "cpu4"),
        "the resolution diagnostic prints no platform alias (US3 scenario 2)");

  // A dedicated leaf: every sampling action advances a leaf's script one
  // point, so this scenario must not touch another scenario's leaves.
  // Scripted {400, 1000}, so the fold is 1000 - 400 = 600.
  const expression<events> misses {*core4.counter<events>("cache_misses")};
  auto compiled = compile(system::local(), misses);
  check(compiled.has_value(), "the alias-resolved plan compiles");
  auto rec = compiled->recorder(2);
  rec.sample();
  rec.sample();
  const auto provenance =
      misses.raw(rec.view(), "package-1/core-4", "cache_misses");
  check(provenance.has_value(),
        "the raw view resolves the canonical leaf address (FR-020)");
  check(provenance->object_path == "package-1/core-4",
        "the provenance line reports the canonical path (US3 scenario 2)");
  check(!contains(provenance->object_path, "cpu4"),
        "the provenance line prints no platform alias (US3 scenario 2)");
  check(provenance->count == 2 && provenance->points[0] == 400
            && provenance->points[1] == 1000,
        "the provenance line carries the raw scripted column (FR-020)");
  check(same_double(misses.fold(rec.view()).value, 600.0),
        "the same window folds 1000 - 400 to 600 (FR-018)");
}

auto test_selection() -> void
{
  const auto all = system::local().objects("core");
  check(all.has_value(), "core selection succeeds (FR-003)");
  check(joined(*all) == "package-1/core-3;package-1/core-4;package-2/core-7;",
        "selection returns exactly the cores (C-SYS-2)");
  const auto p1 = system::local().objects("core", {{"package", "1"}});
  check(p1.has_value() && joined(*p1) == "package-1/core-3;package-1/core-4;",
        "the package filter narrows to package-1 (FR-003)");
  const auto one =
      system::local().objects("core", {{"package", "2"}, {"core", "7"}});
  check(one.has_value() && joined(*one) == "package-2/core-7;",
        "filters combine with AND (FR-003)");
  const auto none =
      system::local().objects("core", {{"package", "1"}, {"core", "7"}});
  check(none.has_value() && none->empty(),
        "an unsatisfiable AND returns exactly nothing (FR-003)");
  const auto bogus = system::local().objects("accelerator");
  check(!bogus.has_value() && contains(bogus.error().message, "kind"),
        "unknown kind is a recoverable error (FR-003)");
  const auto badkey = system::local().objects("core", {{"socket", "1"}});
  check(!badkey.has_value() && contains(badkey.error().message, "filter"),
        "unknown filter key is a recoverable error (FR-003)");

  // A key the tree declares through a path component. The declaration
  // is per kind, so a key one kind spells is still unknown to another.
  const auto engines = system::local().objects("engine");
  check(engines.has_value()
            && joined(*engines) == "package-1/socket-0/accelerator-1;",
        "the deeper object is selectable by its own kind (FR-003)");
  const auto on_socket = system::local().objects("engine", {{"socket", "0"}});
  check(on_socket.has_value()
            && joined(*on_socket) == "package-1/socket-0/accelerator-1;",
        "a provider-declared attribute key selects its own object (FR-003)");
  const auto off_socket = system::local().objects("engine", {{"socket", "9"}});
  check(
      off_socket.has_value() && off_socket->empty(),
      "a declared key whose value matches nothing selects nothing " "(FR-003)");
  const auto socket_on_core =
      system::local().objects("core", {{"socket", "0"}});
  check(!socket_on_core.has_value(),
        "a key another kind declares stays unknown to this one (FR-003)");

  // A top-level object outside the package hierarchy hangs off the
  // machine root, which is its parent (FR-001).
  const auto imc = *system::local().object("uncore_imc_0");
  check(imc.parent() != nullptr && imc.parent()->path() == "machine",
        "a top-level object outside any package parents at the machine "
        "(FR-001)");
}

auto test_cross_object_composition() -> void
{
  const auto imc = *system::local().object("uncore_imc_0");
  const auto bytes = *imc.counter<events>("bytes");
  const auto machine = *system::local().object("machine");
  const auto mono = *machine.counter<time_dim>("monotonic");
  const auto rate = bytes / mono;
  const auto compiled = compile(system::local(), rate);
  check(compiled.has_value(), "the cross-object plan compiles (FR-011)");
  const auto before = probe->read_actions();
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  check(probe->read_actions() - before == 2,
        "each window action reads both objects together (scenario 4)");
  const auto m = window.metric(rate);
  check(same_double(m.value, 0.002),
        "imc bytes over monotonic folds to 4000 / 2000000 (scenario 4)");
}

auto test_fanout_reconciliation() -> void
{
  const auto all = system::local().objects("core");
  const auto core3 = *system::local().object("package-1/core-3");
  const auto c3 = *core3.counter<events>("cycles");
  const auto i3 = *core3.counter<events>("instructions");
  const auto ipc = i3 / c3;
  auto fanout = compile(system::local(), ipc, *all);
  check(fanout.has_value(), "the fan-out ipc plan compiles (FR-047)");
  const auto paths = fanout->object_paths();
  check(paths.size() == 3 && paths[0] == "package-1/core-3"
            && paths[2] == "package-2/core-7",
        "fan-out keys objects by canonical path in selection order (FR-002)");
  auto rec = fanout->recorder(2);
  rec.sample();
  rec.sample();
  const auto results = fanout->fold(ipc, rec.view());
  check(results.size() == 3, "one metric per selected object (SC-007)");
  check(results[0].object_path == "package-1/core-3"
            && same_double(results[0].metric.value, 10.5),
        "core-3 ipc folds 2100 / 200 (FR-047)");
  check(same_double(results[1].metric.value, 4.5),
        "core-4 ipc folds 900 / 200 (FR-047)");
  check(same_double(results[2].metric.value, 14.0),
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
    identical = identical
        && after[index].object_path == before[index].object_path
        && same_double(after[index].metric.value, before[index].metric.value);
  }
  check(identical,
        "a self-move-assigned fan-out folds its per-object metrics exactly "
        "as before (FR-022)");

  const auto d3 = expression<events> {i3}.fold(rec.view());
  const auto core4 = *system::local().object("package-1/core-4");
  const auto i4 = *core4.counter<events>("instructions");
  const auto d4 = expression<events> {i4}.fold(rec.view());
  const auto core7 = *system::local().object("package-2/core-7");
  const auto i7 = *core7.counter<events>("instructions");
  const auto d7 = expression<events> {i7}.fold(rec.view());
  check(same_double(d3.value, 2100.0) && same_double(d4.value, 900.0)
            && same_double(d7.value, 1400.0),
        "per-core instruction deltas come from the shared window (FR-047)");

  const auto machine = *system::local().object("machine");
  const auto total = *machine.counter<events>("total_instructions");
  const auto tplan = compile(system::local(), expression<events> {total});
  check(tplan.has_value(), "the shared-total plan compiles");
  auto trec = tplan->recorder(2);
  trec.sample();
  trec.sample();
  const auto tv = expression<events> {total}.fold(trec.view());
  check(same_double(d3.value + d4.value + d7.value, tv.value),
        "core deltas reconcile the shared total (SC-007)");
  check(same_double(tv.value, 4400.0), "the shared delta is 4400 (SC-007)");
}

auto register_everything() -> void
{
  auto provider = std::make_unique<fake_provider>();
  provider->add_object("package-1", "package", "first package");
  provider->add_object("package-2", "package", "second package");
  provider->add_object("package-1/core-3", "cpu3", "core", "third core");
  provider->add_object("package-1/core-4", "cpu4", "core", "fourth core");
  provider->add_object("package-2/core-7", "core", "seventh core");
  provider->add_object("uncore_imc_0", "imc", "memory controller");
  // A deeper path carrying provider-declared attribute keys: the path
  // components spell the attributes, so a filter on either key is a
  // defined key for this kind (FR-003, T097).
  provider->add_object(
      "package-1/socket-0/accelerator-1", "engine", "first engine");
  provider->add_counter(
      "machine", "monotonic", "nanoseconds", "monotonic wall clock");
  provider->add_counter(
      "machine", "total_instructions", "ops", "shared instruction total");
  provider->add_counter("uncore_imc_0", "bytes", "bytes", "DRAM bytes read");
  provider->add_counter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  provider->set_points("package-1/core-3", "cycles", {100, 300}, 0);
  provider->set_points("package-1/core-3", "instructions", {1000, 3100}, 0);
  provider->add_counter(
      "package-1/core-4", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-1/core-4", "instructions", "ops", "instructions retired");
  provider->add_counter(
      "package-1/core-4", "cache_misses", "ops", "last-level cache misses");
  provider->set_points("package-1/core-4", "cycles", {50, 250}, 0);
  provider->set_points("package-1/core-4", "instructions", {2000, 2900}, 0);
  provider->set_points(
      "package-1/core-4", "cache_misses", {400, 1000, 1600}, 0);
  provider->add_counter(
      "package-2/core-7", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-2/core-7", "instructions", "ops", "instructions retired");
  provider->set_points("package-2/core-7", "cycles", {10, 110}, 0);
  provider->set_points("package-2/core-7", "instructions", {500, 1900}, 0);
  provider->set_points("machine", "monotonic", {0, 2000000}, 0);
  provider->set_points("machine", "total_instructions", {100000, 104400}, 0);
  provider->set_points("uncore_imc_0", "bytes", {1000, 5000}, 0);
  probe = provider.get();
  const auto registered =
      system::local().register_provider(std::move(provider));
  check(registered.has_value(), "the scripted provider registers");
}

}  // namespace

auto main() -> int
{
  register_everything();
  test_duplicate_registration();
  test_enumeration();
  test_alias_resolution();
  test_selection();
  test_cross_object_composition();
  test_fanout_reconciliation();
  std::printf("counters objects tests passed\n");
  return 0;
}
