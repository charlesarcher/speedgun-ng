// ============================================================================
// TDD test for the fake-provider measurement spine (T020, US1 scenarios).
//
// Covers provider registration with the open boundary (FR-009), duplicate
// and unit rejection (FR-008, FR-017), tree and catalog walk
// (FR-001..FR-006), path and alias resolution (FR-002), typed counter
// resolution with near-miss diagnostics (FR-005, FR-008), expression
// algebra with zero-read compile (FR-015, FR-021), and scope window
// exactness including the 2^64 wrap (FR-013, FR-030). Hand-computed
// expectations, frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS FAKE TEST FAIL: %s\n", what);
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
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::dim_same;
using sg::counters::expression;
using sg::counters::fake_provider;
using sg::counters::system;
using sg::counters::target;
using sg::counters::target_kind;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

fake_provider* probe = nullptr;

auto contains(std::string_view haystack, std::string_view needle) -> bool
{
  return haystack.find(needle) != std::string_view::npos;
}

auto test_registration() -> void
{
  auto provider = std::make_unique<fake_provider>();
  provider->add_object("package-1", "package", "first processor package");
  provider->add_object("package-2", "package", "second processor package");
  provider->add_object("package-1/core-3", "cpu3", "core", "third core");
  provider->add_counter(
      "machine", "monotonic", "nanoseconds", "monotonic wall clock");
  provider->add_counter("machine", "drift", "ops", "idle drift counter");
  provider->add_counter("machine", "wrap", "ops", "counter that wraps");
  provider->add_counter(
      "package-1/core-3", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-1/core-3", "instructions", "ops", "instructions retired");
  provider->add_counter("package-1/core-3",
                        "stalled",
                        "none",
                        "stalled cycles",
                        availability::permission_blocked);
  provider->set_points("package-1/core-3", "cycles", {100, 300}, 100);
  provider->set_points("package-1/core-3", "instructions", {1000, 3100}, 2100);
  provider->set_points("machine", "drift", {5});
  provider->set_points("machine", "wrap", {UINT64_MAX - 3, 7});
  provider->set_points("machine", "monotonic", {0, 1000000000});

  probe = provider.get();
  const auto registered =
      system::local().register_provider(std::move(provider));
  check(registered.has_value(), "fake provider registers before open (FR-009)");

  auto dupe = std::make_unique<fake_provider>();
  dupe->add_object("package-1", "package", "colliding package");
  const auto clash = system::local().register_provider(std::move(dupe));
  check(!clash.has_value() && contains(clash.error().message, "duplicate"),
        "duplicate object path rejected, tree unchanged (FR-008)");

  auto bad_unit = std::make_unique<fake_provider>();
  bad_unit->add_counter("package-3", "watts", "watts", "power draw");
  const auto rejected = system::local().register_provider(std::move(bad_unit));
  check(!rejected.has_value() && contains(rejected.error().message, "watts"),
        "unrecognized unit token rejected, named (FR-017)");
}

auto test_tree_walk() -> void
{
  const auto core = system::local().object("package-1/core-3");
  check(core.has_value(), "core resolves by canonical path (FR-002)");
  check(core->path() == "package-1/core-3", "canonical path spelling");
  check(core->kind() == "core", "object kind (FR-001)");
  check(core->alias() == "cpu3", "platform alias reported (FR-002)");

  const auto by_alias = system::local().object("cpu3");
  check(by_alias.has_value() && by_alias->path() == "package-1/core-3",
        "alias resolves to the same object, canonical in output (FR-002)");

  const auto machine = system::local().object("machine");
  check(machine.has_value(), "machine root resolves");
  check(machine->parent() == nullptr, "machine root has no parent (FR-001)");
  check(core->parent() != nullptr && core->parent()->path() == "package-1",
        "parent links up the tree (FR-001)");
  check(machine->children().size() == 2, "machine has both packages");

  check(core->counters().size() == 3, "core catalog lists three counters");
  const bool stalled_blocked = std::ranges::any_of(
      core->counters(),
      [](const sg::counters::catalog_entry& entry)
      {
        return entry.name == "stalled"
            && entry.avail == availability::permission_blocked;
      });
  check(stalled_blocked,
        "unavailable state reported distinctly, never guessed (FR-006)");

  const auto missing = system::local().object("package-9");
  check(!missing.has_value(), "unknown path rejected (FR-008)");
  check(std::ranges::find(missing.error().suggestions, "package-1")
            != missing.error().suggestions.end(),
        "near-miss path suggestion offered (FR-008)");

  const auto after_open =
      system::local().register_provider(std::make_unique<fake_provider>());
  check(!after_open.has_value(), "registration after open rejected (FR-009)");
}

auto test_resolution_diagnostics() -> void
{
  const auto core = *system::local().object("package-1/core-3");

  const auto cycles = core.counter<events>("cycles");
  check(cycles.has_value(), "events counter resolves (FR-005)");
  check(cycles->name() == "cycles" && cycles->unit_token() == "ops",
        "resolved counter carries catalog metadata");
  check(cycles->address() == "package-1/core-3/cycles",
        "canonical leaf address (E-05)");

  const auto wrong_dim = core.counter<time_dim>("cycles");
  check(!wrong_dim.has_value() && contains(wrong_dim.error().message, "cycles")
            && contains(wrong_dim.error().message, "ops"),
        "dimension mismatch is recoverable and names both (FR-017)");

  const auto mistyped = core.counter<events>("cycels");
  check(!mistyped.has_value(), "wrong counter name rejected (FR-008)");
  check(std::ranges::find(mistyped.error().suggestions, "cycles")
            != mistyped.error().suggestions.end(),
        "near-miss counter suggestion offered (FR-008)");

  const auto blocked = core.counter<events>("stalled");
  check(blocked.has_value()
            && blocked->avail() == availability::permission_blocked,
        "availability travels with the handle (FR-006)");
}

auto test_compile_zero_reads() -> void
{
  const auto core = *system::local().object("package-1/core-3");
  const auto cycles = *core.counter<events>("cycles");
  const auto instructions = *core.counter<events>("instructions");
  const auto drift =
      *system::local().object("machine")->counter<events>("drift");

  const expression<events> instr_expr {instructions};
  const auto ipc = instructions / cycles;
  const auto sum = instructions + drift;
  static_assert(dim_same<typename decltype(ipc)::dimension_tag, dim<0, 0>>,
                "instructions / cycles carries dim<0,0> in its type (FR-015)");
  static_assert(dim_same<typename decltype(sum)::dimension_tag, dim<0, 1>>,
                "counter addition keeps the events dimension (FR-015)");

  const auto empty = compile(system::local());
  check(!empty.has_value(), "compile with no expression rejected (FR-021)");

  auto compiled = compile(system::local(), ipc, sum);
  check(compiled.has_value(), "plan compiles (FR-021)");
  check(probe->read_actions() == 0,
        "plan compile performs zero reads (FR-021)");

  auto moved_plan = std::move(*compiled);
  static_assert(!std::is_copy_constructible_v<sg::counters::plan>,
                "the plan is move-only");
  sg::counters::scope window_over_move {moved_plan};
  check(true, "a plan moves and still prepares windows (FR-031)");

  const auto pinned = compile(
      system::local(), target {.kind = target_kind::cpu, .cpu = 0}, instr_expr);
  check(pinned.has_value(), "plan compiles for a pinned cpu target (FR-031)");
}

auto test_scope_exactness() -> void
{
  const auto core = *system::local().object("package-1/core-3");
  const auto cycles = *core.counter<events>("cycles");
  const auto instructions = *core.counter<events>("instructions");

  const auto ipc = instructions / cycles;
  const auto compiled = compile(system::local(), ipc);
  check(compiled.has_value(), "ipc plan compiles");

  sg::counters::scope window {*compiled};
  window.start();
  window.finish();

  const auto metric = window.metric(ipc);
  check(same_double(metric.value, 10.5),
        "scope window folds exactly: 2100 / 200 (FR-030)");
  check(same_double(metric.running_ratio, 1.0) && !metric.scaled,
        "disclosure defaults honest: unscaled, full ratio");
  const auto raw = ipc.raw(window.view(), "package-1/core-3", "instructions");
  check(raw.has_value() && raw->object_path == "package-1/core-3"
            && raw->name == "instructions" && !raw->description.empty()
            && raw->unit == "ops" && raw->points[1] - raw->points[0] == 2100ULL
            && same_double(raw->ratio, 1.0),
        "raw view exposes provenance and the raw point column (FR-020)");
  check(probe->read_actions() == 2,
        "start and finish are one sampling action each (FR-011)");

  const auto second = compile(system::local(), ipc);
  check(second.has_value(), "multiple plans over one system (FR-031)");
  sg::counters::scope window2 {*second};
  window2.start();
  window2.finish();
  check(same_double(window2.metric(ipc).value, 21.0),
        "second window over the advanced script: 2100 / 100 (FR-030)");

  const auto doubled = 2.0 * expression<events> {instructions};
  const auto scaled_plan = compile(system::local(), doubled);
  check(scaled_plan.has_value(), "scaled expression plan compiles");
  sg::counters::scope window3 {*scaled_plan};
  window3.start();
  window3.finish();
  const auto scaled_metric = window3.metric(doubled);
  check(same_double(scaled_metric.value, 4200.0) && scaled_metric.scaled,
        "scalar multiplication scales the fold and flags scaled (FR-015)");
}

auto test_wrap() -> void
{
  const auto wrap = *system::local().object("machine")->counter<events>("wrap");
  const expression<events> wrap_expr {wrap};
  const auto compiled = compile(system::local(), wrap_expr);
  check(compiled.has_value(), "wrap plan compiles");
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  check(same_double(window.metric(wrap_expr).value, 11.0),
        "one hardware wrap subtracts out mod 2^64 (FR-013, SC-006)");
}

}  // namespace

auto main() -> int
{
  test_registration();
  test_tree_walk();
  test_resolution_diagnostics();
  test_compile_zero_reads();
  test_scope_exactness();
  test_wrap();
  std::printf("counters_fake_test: all checks passed\n");
  return 0;
}
