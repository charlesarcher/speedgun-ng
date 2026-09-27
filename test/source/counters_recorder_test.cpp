// ============================================================================
// TDD test for the recorder arena and overflow policies (T022, T024,
// T025, T026; US2 scenarios).
//
// Covers the plan-owned arena with non-owning trivially-copyable
// cursors (FR-029), hard_stop capacity semantics (FR-027), ring
// masking with drop accounting and the wrapped fold window (FR-025,
// FR-028), per-interval fold_pairs and first-to-last folds over
// recorder windows (FR-018), independent cursors on one plan, and the
// 2^64 wrap through a recorder-sampled ratio (FR-013). Hand-computed
// expectations, frameworkless check()/fail() convention. Each scenario
// owns dedicated leaves: a leaf script advances one point per sampling
// action, so scenarios sharing leaves would consume each other's
// scripts.
// ============================================================================

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS RECORDER TEST FAIL: %s\n", what);
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

using sg::counters::compile;
using sg::counters::dim;
using sg::counters::expression;
using sg::counters::fake_provider;
using sg::counters::object;
using sg::counters::plan;
using sg::counters::recorder_handle;
using sg::counters::system;

using events = dim<0, 1>;

static_assert(
    std::is_trivially_copyable_v<recorder_handle<sg::counters::hard_stop_t>>,
    "the hard_stop recorder handle is a copyable cursor (FR-029)");
static_assert(
    std::is_trivially_copyable_v<recorder_handle<sg::counters::ring_t>>,
    "the ring recorder handle is a copyable cursor (FR-029)");

constexpr std::uint64_t wrap_base = UINT64_MAX - 9;

auto core_object() -> object
{
  return *system::local().object("package-1/core-3");
}

// Compiled plan plus its ipc expression, per scenario leaf pair.
struct scenario
{
  plan compiled;
  expression<dim<0, 0>> ipc;
};

auto ipc_scenario(const char* cycles_name, const char* instr_name) -> scenario
{
  const auto core = core_object();
  const auto cycles = *core.counter<events>(cycles_name);
  const auto instructions = *core.counter<events>(instr_name);
  const auto ipc = instructions / cycles;
  auto compiled = compile(system::local(), ipc);
  if (!compiled.has_value()) {
    fail("ipc plan compiles");
  }
  return scenario {
      .compiled = std::move(*compiled),
      .ipc = ipc,
  };
}

auto test_hard_stop_extent() -> void
{
  const auto sc = ipc_scenario("cyc0", "ins0");
  auto rec = sc.compiled.recorder(4);
  check(rec.capacity() == 4, "hard_stop recorder reports its capacity");
  check(!rec.wrapped() && rec.dropped() == 0, "a fresh recorder is empty");
  for (int i = 0; i < 4; ++i) {
    rec.sample();
  }
  check(rec.count() == 4, "hard_stop fills to capacity (FR-026)");
  // The full recorder's own state cannot report a wrap, so the retained
  // window is checked where it can fail: through the columns. The
  // fixture scripts cyc0 as {100, 300, 600, 1000, 1500} and ins0 as
  // {1000, 3100, 5200, 7300, 9500}, and four sampling actions take the
  // first four of each, so a recorder that held fewer points, or a
  // different four, fails here (FR-027).
  const auto cycles = sc.ipc.raw(rec.view(), "package-1/core-3", "cyc0");
  const auto instructions = sc.ipc.raw(rec.view(), "package-1/core-3", "ins0");
  check(cycles.has_value() && instructions.has_value(),
        "the full recorder exposes both retained columns (FR-020)");
  check(cycles->count == 4 && cycles->points[0] == 100
            && cycles->points[1] == 300 && cycles->points[2] == 600
            && cycles->points[3] == 1000,
        "a full hard_stop recorder retains exactly its capacity window "
        "(FR-027)");
  check(instructions->points[0] == 1000 && instructions->points[1] == 3100
            && instructions->points[2] == 5200
            && instructions->points[3] == 7300,
        "the second member's column is retained whole (FR-027)");
  const auto pairs = sc.ipc.fold_pairs(rec.view());
  check(pairs.size() == 3, "four points fold into three intervals (FR-018)");
  check(same_double(pairs[0].value, 10.5) && same_double(pairs[1].value, 7.0)
            && same_double(pairs[2].value, 5.25),
        "per-interval folds are exact: 2100/200, 2100/300, 2100/400 (FR-018)");
  check(same_double(sc.ipc.fold(rec.view()).value, 7.0),
        "first-to-last fold spans endpoints: 6300/900 (FR-018)");
}

auto test_ring_overflow() -> void
{
  const auto sc = ipc_scenario("cyc1", "ins1");
  auto rec = *sc.compiled.recorder(2, sg::counters::ring);
  for (int i = 0; i < 4; ++i) {
    rec.sample();
  }
  check(rec.count() == 2, "a ring never exceeds capacity (FR-025)");
  check(rec.wrapped(), "the ring promotes to wrapped (FR-028)");
  check(rec.dropped() == 2, "two samples were overwritten (FR-028)");
  const auto whole = sc.ipc.fold(rec.view());
  check(same_double(whole.value, 5.25),
        "the wrapped fold reads the retained window: 2100/400 (FR-028)");
  const auto pairs = sc.ipc.fold_pairs(rec.view());
  check(pairs.size() == 1 && same_double(pairs[0].value, 5.25),
        "a retained window of two folds into one interval (FR-018)");
  // The raw view of a wrapped recorder spans the oldest to the newest
  // retained row, physically at `dropped % stride` (FR-020, FR-028).
  // Two samples were dropped, so the retained rows sit at physical
  // slots 0 and 1 and the view reports them in logical order.
  const auto column = sc.ipc.raw(rec.view(), "package-1/core-3", "ins1");
  check(column.has_value() && column->count == 2
            && column->points[0] == 5200 && column->points[1] == 7300,
        "a wrapped raw view spans the retained window oldest first "
        "(FR-020, FR-028)");
  check(column.has_value() && same_double(column->ratio, 1.0),
        "a source with no time pair discloses ratio 1.0 in its raw view "
        "(FR-020)");
}

auto test_ring_capacity_guard() -> void
{
  const auto sc = ipc_scenario("cyc1", "ins1");
  check(!sc.compiled.recorder(3, sg::counters::ring).has_value(),
        "ring capacity 3 is rejected (FR-025)");
  check(!sc.compiled.recorder(0, sg::counters::ring).has_value(),
        "ring capacity 0 is rejected (FR-025)");
  check(sc.compiled.recorder(4, sg::counters::ring).has_value(),
        "ring capacity 4 is accepted (FR-025)");
}

auto test_independent_cursors() -> void
{
  const auto sc = ipc_scenario("cyc2", "ins2");
  auto first = sc.compiled.recorder(2);
  first.sample();
  first.sample();
  auto second = sc.compiled.recorder(2);
  second.sample();
  second.sample();
  check(same_double(sc.ipc.fold(first.view()).value, 10.5),
        "the first cursor holds actions one and two (FR-029)");
  check(same_double(sc.ipc.fold(second.view()).value, 5.25),
        "the second cursor holds actions three and four (FR-029)");
}

auto test_wrap_through_recorder() -> void
{
  const auto wrap = *system::local().object("machine")->counter<events>("wrap");
  const auto tick = *system::local().object("machine")->counter<events>("tick");
  const auto ratio = wrap / tick;
  auto compiled = compile(system::local(), ratio);
  if (!compiled.has_value()) {
    fail("wrap ratio plan compiles");
  }
  auto rec = compiled->recorder(3);
  rec.sample();
  rec.sample();
  check(same_double(ratio.fold(rec.view(), 0, 1).value, 15.0),
        "the first interval folds across 2^64 (FR-013)");
  rec.sample();
  check(same_double(ratio.fold(rec.view(), 0, 1).value, 15.0),
        "the earlier interval survives a later sample");
  check(same_double(ratio.fold(rec.view(), 1, 2).value, 15.0),
        "the post-wrap interval folds plainly (FR-013)");
}

auto register_everything() -> void
{
  auto provider = std::make_unique<fake_provider>();
  provider->add_object("package-1/core-3", "cpu3", "core", "third core");
  // One leaf pair per sampling scenario, and no pair beyond them: a
  // registered leaf nothing samples is dead fixture weight.
  for (int scenario = 0; scenario < 3; ++scenario) {
    const char cycles_name[] = {'c', 'y', 'c', char('0' + scenario), '\0'};
    const char instr_name[] = {'i', 'n', 's', char('0' + scenario), '\0'};
    provider->add_counter(
        "package-1/core-3", cycles_name, "ops", "scenario cycles");
    provider->add_counter(
        "package-1/core-3", instr_name, "ops", "scenario instructions");
    provider->set_points(
        "package-1/core-3", cycles_name, {100, 300, 600, 1000, 1500}, 0);
    provider->set_points(
        "package-1/core-3", instr_name, {1000, 3100, 5200, 7300, 9500}, 0);
  }
  provider->add_counter("machine", "wrap", "ops", "counter that wraps");
  provider->set_points("machine", "wrap", {wrap_base, 5, 20, 40}, 0);
  provider->add_counter("machine", "tick", "ops", "denominator ticks");
  provider->set_points("machine", "tick", {0, 1, 2, 3}, 0);
  const auto registered =
      system::local().register_provider(std::move(provider));
  if (!registered.has_value()) {
    fail("the scripted provider registers");
  }
}

// A plan assigned to itself. The assignment guards on identity, so the
// layout survives and the plan still folds (FR-022).
auto test_self_move_assignment() -> void
{
  auto sc = ipc_scenario("cyc0", "ins0");
  auto rec = sc.compiled.recorder(2);
  rec.sample();
  rec.sample();
  const auto before = sc.ipc.fold(rec.view()).value;
  // A plan holds an owning layout pointer, so the assignment guards on
  // identity: assigning a plan to itself keeps the layout (FR-022).
  auto& alias = sc.compiled;
  alias = std::move(sc.compiled);
  auto rec2 = sc.compiled.recorder(2);
  rec2.sample();
  rec2.sample();
  check(same_double(sc.ipc.fold(rec2.view()).value, before),
        "a self-move-assigned plan still folds its own window (FR-022)");
}

}  // namespace

auto main() -> int
{
  register_everything();
  test_hard_stop_extent();
  test_ring_overflow();
  test_ring_capacity_guard();
  test_independent_cursors();
  test_wrap_through_recorder();
  test_self_move_assignment();
  std::printf("counters recorder tests passed\n");
  return 0;
}
