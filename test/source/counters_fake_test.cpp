// ============================================================================
// TDD test for the fake-provider measurement spine (T020, US1 scenarios).
//
// Covers provider registration with the open boundary (FR-009), duplicate
// and unit rejection (FR-008, FR-017), tree and catalog walk
// (FR-001..FR-006), path and alias resolution (FR-002), typed counter
// resolution with near-miss diagnostics (FR-005, FR-008), expression
// algebra with zero-read compile (FR-015, FR-021), scope window
// exactness including the 2^64 wrap (FR-013, FR-030), the composite
// multiplex ratio product (FR-019), the assembled provenance record
// (SC-008), and the seeded per-sample tail (T014). Hand-computed
// expectations, frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
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
using sg::counters::leaf_set;
using sg::counters::metric_result;
using sg::counters::points_view;
using sg::counters::read_mode;
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

// The shortest round-trip spelling of a number, so a hand-computed
// expectation can be compared as a string with no format drift.
template<class T>
auto number(const T value) -> std::string
{
  std::array<char, 32> buffer {};
  const auto written =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  return std::string(buffer.data(), written.ptr);
}

// The provenance record SC-008 names, assembled from the public surface
// alone: the folded value, each constituent's raw window delta, and the
// ratio disclosure. No artifact fixes the spelling, so the shape is
// derived from the one example in the tree, spec.md:315's
// `IPC = 1.31 <- instructions 12.3e9 / cycles 9.4e9, ratio 0.98`:
//
//   <label> = <value> <- <leaf> <delta> / <leaf> <delta>, ratio <ratio>
auto provenance_record(const std::string_view label,
                       const metric_result& metric,
                       const std::vector<points_view>& leaves) -> std::string
{
  std::string record =
      std::string(label) + " = " + number(metric.value) + " <- ";
  for (std::size_t index = 0; index < leaves.size(); ++index) {
    if (index > 0) {
      record += " / ";
    }
    const auto& leaf = leaves[index];
    record += std::string(leaf.name) + " ";
    record += number(leaf.points[leaf.count - 1] - leaf.points[0]);
  }
  record += ", ratio " + number(metric.running_ratio);
  return record;
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

  // Each scenario below reads its own counters, so the scripted position
  // starts at zero however many actions the earlier scenarios spent
  // (FR-036).
  provider->add_object(
      "package-1/algebra", "scratch", "additive algebra object");
  provider->add_counter("package-1/algebra", "addend_a", "ops", "first addend");
  provider->add_counter(
      "package-1/algebra", "addend_b", "ops", "second addend");
  provider->set_points("package-1/algebra", "addend_a", {0, 2100}, 2100);
  provider->set_points("package-1/algebra", "addend_b", {0, 200}, 200);

  provider->add_object("package-1/splice", "scratch", "splice object");
  provider->add_counter(
      "package-1/splice", "only", "ops", "leaf spliced first");
  provider->add_counter(
      "package-1/splice", "numerator", "ops", "quotient numerator");
  provider->add_counter(
      "package-1/splice", "denominator", "ops", "quotient denominator");
  // A fourth leaf whose delta no other leaf of this object carries, so
  // the scale node of the spliced-operand scenario reads a value of its
  // own (T176).
  provider->add_counter("package-1/splice",
                        "scaled_operand",
                        "ops",
                        "leaf the spliced scale multiplies");
  provider->set_points("package-1/splice", "only", {0, 200}, 200);
  provider->set_points("package-1/splice", "numerator", {0, 2100}, 2100);
  provider->set_points("package-1/splice", "denominator", {0, 200}, 200);
  provider->set_points("package-1/splice", "scaled_operand", {0, 400}, 400);

  provider->add_object("package-1/edge", "scratch", "fold edge object");
  provider->add_counter("package-1/edge", "single", "ops", "the only leaf");
  provider->set_points("package-1/edge", "single", {0, 700}, 700);

  provider->add_object(
      "package-1/move/core-1", "core", "core for move assignment");
  provider->add_counter(
      "package-1/move/core-1", "cycles", "ops", "core cycles elapsed");
  provider->add_counter(
      "package-1/move/core-1", "instructions", "ops", "instructions retired");
  provider->add_counter(
      "package-1/move/core-1", "deep_only", "ops", "counter on one core only");
  provider->set_points("package-1/move/core-1", "cycles", {0, 200}, 200);
  provider->set_points("package-1/move/core-1", "deep_only", {0, 700}, 700);
  provider->set_points(
      "package-1/move/core-1", "instructions", {0, 2100}, 2100);

  // Two objects each disclosing an enabled/running time pair, so a
  // composite over both discloses a multiplex ratio. The pair-carrying
  // leaf sits on opposite sides of the quotient, which is what puts a
  // negative exponent on one constituent ratio (FR-019).
  provider->add_object("package-1/ratio-a", "scratch", "multiplexed source a");
  provider->add_counter("package-1/ratio-a",
                        "enabled",
                        "nanoseconds",
                        "nanoseconds the event counter was enabled",
                        availability::countable,
                        read_mode::syscall,
                        true);
  provider->add_counter("package-1/ratio-a",
                        "running",
                        "nanoseconds",
                        "nanoseconds the event counter was scheduled");
  provider->set_points("package-1/ratio-a", "enabled", {0, 200});
  provider->set_points("package-1/ratio-a", "running", {0, 50});
  provider->add_object("package-1/ratio-b", "scratch", "multiplexed source b");
  provider->add_counter("package-1/ratio-b",
                        "enabled",
                        "nanoseconds",
                        "nanoseconds the event counter was enabled");
  provider->add_counter("package-1/ratio-b",
                        "running",
                        "nanoseconds",
                        "nanoseconds the event counter was scheduled",
                        availability::countable,
                        read_mode::syscall,
                        true);
  provider->set_points("package-1/ratio-b", "enabled", {0, 100});
  provider->set_points("package-1/ratio-b", "running", {0, 50});

  // A pair-carrying leaf whose enabled leaf never advances: the pair
  // carries no measured fraction over the window, so the fold states
  // full rate (FR-019, T066).
  provider->add_object("package-1/ratio-frozen", "scratch", "frozen pair");
  provider->add_counter("package-1/ratio-frozen",
                        "enabled",
                        "nanoseconds",
                        "nanoseconds the event counter was enabled",
                        availability::countable,
                        read_mode::syscall,
                        true);
  provider->add_counter("package-1/ratio-frozen",
                        "running",
                        "nanoseconds",
                        "nanoseconds the event counter was scheduled");
  provider->set_points("package-1/ratio-frozen", "enabled", {7, 7});
  provider->set_points("package-1/ratio-frozen", "running", {0, 50});

  // A pair-carrying leaf whose enabled and running halves advance by the
  // same amount, the ordinary un-multiplexed source: the fold discloses
  // full rate and no scale (FR-019).
  provider->add_object("package-1/ratio-full", "scratch", "full-rate pair");
  provider->add_counter("package-1/ratio-full",
                        "enabled",
                        "nanoseconds",
                        "nanoseconds the event counter was enabled",
                        availability::countable,
                        read_mode::syscall,
                        true);
  provider->add_counter("package-1/ratio-full",
                        "running",
                        "nanoseconds",
                        "nanoseconds the event counter was scheduled");
  provider->set_points("package-1/ratio-full", "enabled", {0, 900});
  provider->set_points("package-1/ratio-full", "running", {0, 900});

  // A pair-carrying leaf quoted on both sides of one fold, so its two
  // occurrences carry opposite exponents and the exponent of the leaf
  // sums to zero (FR-019).
  provider->add_object(
      "package-1/ratio-shared", "scratch", "leaf on both sides of a fold");
  provider->add_counter("package-1/ratio-shared",
                        "enabled",
                        "nanoseconds",
                        "nanoseconds the event counter was enabled",
                        availability::countable,
                        read_mode::syscall,
                        true);
  provider->add_counter("package-1/ratio-shared",
                        "running",
                        "nanoseconds",
                        "nanoseconds the event counter was scheduled");
  provider->set_points("package-1/ratio-shared", "enabled", {0, 300});
  provider->set_points("package-1/ratio-shared", "running", {0, 100});

  // A leaf with no explicit sequence at all, whose whole trace comes
  // from the seeded per-sample delta generator (T014).
  provider->add_object(
      "package-1/seeded", "scratch", "seeded delta generator object");
  provider->add_counter(
      "package-1/seeded", "walk", "ops", "seeded per-sample delta walk");
  provider->set_points("package-1/seeded", "walk", {}, 0, 1);

  probe = provider.get();
  const auto registered =
      system::local().register_provider(std::move(provider));
  check(registered.has_value(), "fake provider registers before open (FR-009)");

  auto dupe = std::make_unique<fake_provider>();
  dupe->add_object("package-1", "package", "colliding package");
  const auto clash = system::local().register_provider(std::move(dupe));
  check(!clash.has_value() && contains(clash.error().message, "duplicate"),
        "duplicate object path rejected, tree unchanged (FR-008)");

  // A unit token outside the closed mapping is refused where the tree
  // is built, and the tree keeps the invariant `object::counters`
  // enforces at `source/counters/system.cpp:417`: every stored catalog
  // unit maps. Deferring the refusal to resolution would leave that
  // accessor a fuse on a tree the library had accepted.
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
  check(cycles->description() == "core cycles elapsed",
        "the handle carries the catalog description (US1 scenario 1)");
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

  // A query carrying an empty word (a doubled separator) splits into
  // words with an empty one between them; the near-miss scan drops the
  // empty word and still offers the real name (FR-008).
  const auto doubled = core.counter<events>("cyc__les");
  check(!doubled.has_value(),
        "a query with a doubled separator resolves to nothing (FR-008)");
  check(
      std::ranges::find(doubled.error().suggestions, "cycles")
          != doubled.error().suggestions.end(),
      "a query with a doubled separator still finds its near miss " "(FR-008)");
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

  // The moved plan is compiled over the object the suite reserves for its
  // own move case, whose scripted steps repeat at every cursor position,
  // so the window below costs the later move test nothing (FR-036).
  const auto move_core = *system::local().object("package-1/move/core-1");
  const auto move_ipc = *move_core.counter<events>("instructions")
      / *move_core.counter<events>("cycles");
  auto move_compiled = compile(system::local(), move_ipc);
  check(move_compiled.has_value(), "the move-case plan compiles (FR-031)");

  auto moved_plan = std::move(*move_compiled);
  static_assert(!std::is_copy_constructible_v<sg::counters::plan>,
                "the plan is move-only");
  sg::counters::scope window_over_move {moved_plan};
  window_over_move.start();
  window_over_move.finish();
  // 2100 instructions over 200 cycles, the two steps scripted beside
  // `package-1/move/core-1` in the fixture: the moved plan sampled the
  // window, so folding it proves the move carried the measurement across
  // (FR-030, FR-031).
  const auto over_move = window_over_move.metric(move_ipc);
  check(same_double(over_move.value, 10.5),
        "a plan moves and folds the window it sampled (FR-031)");
  check(same_double(over_move.running_ratio, 1.0) && !over_move.scaled,
        "the moved plan discloses the same ratio and scale (FR-019)");

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

  const auto reads_at_entry = probe->read_actions();
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();

  const auto metric = window.metric(ipc);
  check(same_double(metric.value, 10.5),
        "scope window folds exactly: 2100 / 200 (FR-030)");
  check(same_double(metric.running_ratio, 1.0) && !metric.scaled,
        "disclosure defaults: unscaled, full ratio");
  const auto raw = ipc.raw(window.view(), "package-1/core-3", "instructions");
  check(raw.has_value() && raw->object_path == "package-1/core-3"
            && raw->name == "instructions" && !raw->description.empty()
            && raw->unit == "ops" && raw->points[1] - raw->points[0] == 2100ULL
            && same_double(raw->ratio, 1.0),
        "raw view exposes provenance and the raw point column (FR-020)");
  // The quotient spine lists instructions first, so it takes slot 0 of
  // the plan's two point columns; the scope window holds exactly the
  // points start() and finish() committed.
  check(raw.has_value() && raw->slot == 0 && raw->count == 2,
        "the raw view names the point column and extent it read "
        "(US1 scenario 7)");
  check(probe->read_actions() - reads_at_entry == 2,
        "start and finish are one sampling action each (FR-011)");

  const auto cycles_raw = ipc.raw(window.view(), "package-1/core-3", "cycles");
  check(cycles_raw.has_value(), "the quotient discloses its second leaf");
  // SC-008: 2100 / 200 folds to 10.5, both leaves carry no enabled/
  // running pair so the disclosure is ratio 1.
  check(provenance_record("IPC", metric, {*raw, *cycles_raw})
            == "IPC = 10.5 <- instructions 2100 / cycles 200, ratio 1",
        "the assembled provenance record carries value, leaves, and ratio "
        "(SC-008)");

  const auto reads_after_first_fold = probe->read_actions();
  for (int repeat = 0; repeat < 10; ++repeat) {
    const auto again = window.metric(ipc);
    check(same_double(again.value, 10.5)
              && same_double(again.running_ratio, 1.0) && !again.scaled,
          "a repeated metric re-folds the stored points exactly "
          "(US1 scenario 5, FR-021)");
  }
  check(probe->read_actions() == reads_after_first_fold,
        "ten further metrics perform zero provider reads (US1 scenario 5)");

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

// The additive and subtractive halves of the algebra, which the
// quotient-only tests elsewhere never reach: both operands must share a
// tag, and the fold evaluates the binary node (FR-015, FR-018).
auto test_additive_algebra() -> void
{
  const auto scratch = *system::local().object("package-1/algebra");
  const auto addend_a = *scratch.counter<events>("addend_a");
  const auto addend_b = *scratch.counter<events>("addend_b");

  const auto sum = addend_a + addend_b;
  const auto difference = addend_a - addend_b;
  check(dim_same<decltype(sum)::dimension_tag, events>,
        "addition preserves the shared tag");
  const auto compiled = compile(system::local(), sum, difference);
  check(compiled.has_value(), "the additive plan compiles");

  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  // addend_a steps 2100 and addend_b steps 200 across the window.
  check(same_double(window.metric(sum).value, 2300.0),
        "addition folds the sum of both deltas exactly");
  check(same_double(window.metric(difference).value, 1900.0),
        "subtraction folds the difference of both deltas exactly");
}

// The construction-error surface compile owes the caller, and the
// two corner cases the folds guard: a default-constructed spine and a
// raw view for a leaf the expression does not contain (FR-018, FR-020,
// FR-024, FR-046).
auto test_construction_and_fold_edges() -> void
{
  const auto edge = *system::local().object("package-1/edge");
  const auto single = *edge.counter<events>("single");
  const auto ipc = single / single;

  const auto empty = compile(system::local(), expression<events> {});
  check(!empty.has_value(),
        "an expression carrying no resolved leaves is refused (FR-046)");

  // A scalar multiple adds its scale node over whatever root the spine
  // holds, so scaling a zero-leaf spine leaves one node over no operand.
  // A spine needs a leaf to measure, so the refusal reads the leaf vector
  // and the spine never reaches a window (FR-046).
  const auto scaled_empty =
      compile(system::local(), 2.0 * expression<events> {});
  check(!scaled_empty.has_value()
            && contains(scaled_empty.error().message, "no resolved leaves"),
        "a scalar multiple of a zero-leaf expression is refused at "
        "construction (FR-046)");

  // The same spine as a fan-out exemplar. A scale node gives it one
  // node, so an exemplar guard reading the node count passes it and the
  // refusal reaches the span check, whose message names an exemplar
  // spanning several objects for a spine holding no leaf. The leaf count
  // is the vector the refusal reads (FR-046, FR-024, T166).
  const auto cores = system::local().objects("core");
  const auto scaled_empty_fanout =
      compile(system::local(), 2.0 * expression<events> {}, *cores);
  check(!scaled_empty_fanout.has_value()
            && contains(scaled_empty_fanout.error().message,
                        "carries no leaves")
            && !contains(scaled_empty_fanout.error().message,
                         "several objects"),
        "a scalar multiple of a zero-leaf exemplar is refused as leafless "
        "(FR-046, FR-024)");

  const auto compiled = compile(system::local(), ipc);
  check(compiled.has_value(), "the quotient plan compiles");
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  check(same_double(expression<events> {}.fold(window.view(), 0, 1).value, 0.0),
        "an empty spine folds to the zeroed result shape (FR-018)");
  // Both operands of this quotient carry the same leaf, so the spine
  // holds one leaf slot for the pair. Folding it must read that slot
  // twice and stay inside the leaf vector.
  check(same_double(window.metric(ipc).value, 1.0),
        "a quotient of one counter by itself folds inside the leaf vector "
        "(FR-015, FR-018)");

  const auto absent =
      ipc.raw(window.view(), "package-1/core-3", "instructions");
  check(!absent.has_value() && contains(absent.error().message, "instructions"),
        "a raw view for a leaf the expression lacks names it (FR-020)");
}

// Compiling several expressions into one plan splices each spine into
// the shared one, remapping node indices and reusing a leaf that two
// expressions share (FR-021, FR-022).
auto test_multi_expression_splice() -> void
{
  const auto scratch = *system::local().object("package-1/splice");
  const auto only = *scratch.counter<events>("only");
  const auto numerator = *scratch.counter<events>("numerator");
  const auto denominator = *scratch.counter<events>("denominator");
  const auto ipc = numerator / denominator;
  // The binary expression lands in the second position, so its node
  // indices need remapping into the shared spine.
  const expression<events> leaf_expr {only};
  // Two ratios share a tag, so their sum splices the second quotient's
  // binary node into the first one's spine, re-addressing that node's
  // operands there.
  const auto numerator_ratio = numerator / numerator;
  const auto denominator_ratio = denominator / denominator;
  const auto ratio_sum = numerator_ratio + denominator_ratio;
  const auto compiled = compile(system::local(), leaf_expr, ipc, ratio_sum);
  check(compiled.has_value(), "leaf, quotient, and their sum share a plan");
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  check(same_double(window.metric(ipc).value, 10.5),
        "the spliced quotient still folds exactly");
  check(same_double(window.metric(leaf_expr).value, 200.0),
        "the leaf spliced ahead of the quotient still folds exactly");
  // (2100 / 2100) + (200 / 200) over the same window.
  check(same_double(window.metric(ratio_sum).value, 2.0),
        "the sum of two spliced ratios folds exactly");
}

// A plan and a fan-out plan are move-only values: move assignment
// releases the target's layout and takes the source's (FR-022, FR-031).
auto test_move_assignment() -> void
{
  const auto core = *system::local().object("package-1/move/core-1");
  const auto cycles = *core.counter<events>("cycles");
  const auto instructions = *core.counter<events>("instructions");
  const auto ipc = instructions / cycles;

  auto first = compile(system::local(), ipc);
  auto second = compile(system::local(), ipc);
  check(first.has_value() && second.has_value(), "both plans compile");
  // A plan is move-only and its constructor is private to compile, so
  // the target is reached through the public move constructor.
  sg::counters::plan target = std::move(*first);
  target = std::move(*second);
  sg::counters::scope window {target};
  window.start();
  window.finish();
  check(same_double(window.metric(ipc).value, 10.5),
        "the move-assigned plan folds exactly");

  const auto cores = system::local().objects("core", {{"core", "1"}});
  check(cores.has_value() && cores->size() == 1,
        "the dedicated core is selected on its own");
  auto fanout = compile(system::local(), ipc, *cores);
  check(fanout.has_value(), "the fan-out plan compiles");
  auto other = compile(system::local(), ipc, *cores);
  check(other.has_value(), "a second fan-out plan compiles");
  sg::counters::fanout_plan fanout_target = std::move(*fanout);
  fanout_target = std::move(*other);
  check(fanout_target.object_paths().size() == 1,
        "the move-assigned fan-out plan keeps the selection");
  auto rec = fanout_target.recorder(2);
  rec.sample();
  rec.sample();
  const auto folded = fanout_target.fold(ipc, rec.view());
  check(folded.size() == 1 && same_double(folded.front().metric.value, 10.5),
        "the move-assigned fan-out plan folds exactly");
}

// A three-level path resolves its parent through the separator, and a
// near-miss query reports the two diagnostic families the catalog
// distinguishes: names within the edit distance, then description word
// overlaps (FR-001, FR-008).
auto test_nested_parent_and_suggestions() -> void
{
  const auto deep = *system::local().object("package-1/move/core-1");
  check(deep.kind() == "core", "the nested object reports its kind");
  // The path names an intermediate object the catalog never declared, so
  // the parent link has nothing to resolve to and reports none
  // (FR-001).
  check(deep.parent() == nullptr,
        "a path whose intermediate object is absent reports no parent "
        "(FR-001)");

  const auto scratch = *system::local().object("package-1/algebra");
  check(scratch.counters().size() == 2,
        "the scratch object carries exactly its two declared counters");

  // "addend_x" sits within the edit distance of both addends, so the
  // suggestions come from the name family.
  const auto near = scratch.counter<events>("addend_x");
  check(!near.has_value() && near.error().suggestions.size() == 2
            && near.error().suggestions[0] == "addend_a"
            && near.error().suggestions[1] == "addend_b",
        "a one-character miss suggests both near names (FR-008)");

  // "elapsed" is far from every counter name and shares a word with one
  // description, so the suggestions come from the description family.
  const auto core = *system::local().object("package-1/core-3");
  const auto by_word = core.counter<events>("elapsed");
  check(!by_word.has_value() && by_word.error().suggestions.size() == 1
            && by_word.error().suggestions[0] == "cycles",
        "a distant query falls back to description word overlap (FR-008)");
}

// The recoverable construction errors a fan-out owes its caller, each one
// refused in the untimed region before any window opens (FR-024).
auto test_fanout_construction_errors() -> void
{
  const auto core = *system::local().object("package-1/core-3");
  const auto cycles = *core.counter<events>("cycles");
  const auto ipc = cycles / cycles;
  const auto cores = system::local().objects("core");

  const auto no_leaves =
      compile(system::local(), expression<events> {}, *cores);
  check(!no_leaves.has_value()
            && contains(no_leaves.error().message, "carries no leaves"),
        "an empty exemplar spine is refused (FR-024)");

  const std::vector<const sg::counters::object*> nothing;
  const auto no_selection = compile(system::local(), ipc, nothing);
  check(!no_selection.has_value()
            && contains(no_selection.error().message, "non-empty selection"),
        "an empty selection is refused (FR-024)");

  const auto scratch = *system::local().object("package-1/splice");
  const auto numerator = *scratch.counter<events>("numerator");
  // The exemplar draws one leaf from each of two objects, so no single
  // prefix can instantiate it across the selection.
  const auto spanning = numerator / cycles;
  const auto not_same_object = compile(system::local(), spanning, *cores);
  check(!not_same_object.has_value()
            && contains(not_same_object.error().message, "several objects"),
        "an exemplar spanning two objects is refused (FR-024)");

  const std::vector<const sg::counters::object*> with_null {nullptr};
  const auto null_member = compile(system::local(), ipc, with_null);
  check(!null_member.has_value()
            && contains(null_member.error().message, "null object"),
        "a selection holding a null object is refused (FR-024)");

  const std::vector<const sg::counters::object*> twice {&core, &core};
  const auto duplicated = compile(system::local(), ipc, twice);
  check(!duplicated.has_value()
            && contains(duplicated.error().message, "duplicates"),
        "a selection repeating an object is refused (FR-024)");

  // An exemplar whose leaves the selected objects do not carry resolves
  // against the tree and is refused by name (FR-024, FR-017).
  const auto deep = *system::local().object("package-1/move/core-1");
  const auto deep_only = *deep.counter<events>("deep_only");
  const auto moved = deep_only / deep_only;
  const auto wrong_object = compile(system::local(), moved, *cores);
  check(!wrong_object.has_value()
            && contains(wrong_object.error().message, "not in the system tree"),
        "an exemplar missing from a selected object is refused by name "
        "(FR-024)");
}

// A leaf the catalog reports as permission-blocked resolves and carries
// its state, and compiling over it names that state. Nothing is guessed
// (FR-006, FR-007, FR-024).
auto test_permission_blocked_leaf() -> void
{
  const auto core = *system::local().object("package-1/core-3");
  const auto blocked = core.counter<events>("stalled");
  check(blocked.has_value()
            && blocked->avail() == availability::permission_blocked,
        "a blocked leaf resolves and carries the probed state (FR-007)");
  const expression<events> over {*blocked};
  const auto refused = compile(system::local(), over);
  check(!refused.has_value()
            && contains(refused.error().message, "permission_blocked"),
        "compiling over a blocked leaf names its catalog state (FR-024)");
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

// The composite multiplex ratio, the one product no fold of a
// pairless source can reach: each constituent contributes its
// enabled/running delta ratio raised to its algebraic exponent, and the
// pair-carrying leaves sit on opposite sides of the quotient so one
// exponent is negative (FR-019).
auto test_ratio_product() -> void
{
  const auto source_a = *system::local().object("package-1/ratio-a");
  const auto source_b = *system::local().object("package-1/ratio-b");
  const auto a_enabled = *source_a.counter<time_dim>("enabled");
  const auto a_running = *source_a.counter<time_dim>("running");
  const auto b_running = *source_b.counter<time_dim>("running");
  const auto b_enabled = *source_b.counter<time_dim>("enabled");

  // The pair leaves are quoted by the same object, so every ratio in
  // the product has both halves in the spine. Scripted deltas:
  // a/enabled 200, a/running 50, b/running 50, b/enabled 100.
  const auto composite = a_enabled / (b_running - a_running + b_enabled);
  const auto compiled = compile(system::local(), composite);
  check(compiled.has_value(), "the ratio plan compiles");

  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  const auto metric = window.metric(composite);
  // 200 / (50 - 50 + 100) = 2.
  check(same_double(metric.value, 2.0),
        "the composite folds its own arithmetic exactly (FR-019)");
  // 50 / 200 = 0.25 at exponent +1, and 50 / 100 = 0.5 at exponent -1,
  // so the product is 0.25 * (1 / 0.5) = 0.5, strictly between the two
  // bounds a single-source product would report.
  check(same_double(metric.running_ratio, 0.5) && metric.running_ratio < 1.0
            && metric.running_ratio > 0.0 && metric.scaled,
        "the folded ratio is the product of the constituent ratios raised "
        "to their exponents (FR-019)");
}

// One pair-carrying leaf quoted by both operands of a quotient, the case
// where a per-occurrence sign cannot give the leaf's exponent: the leaf
// enters the numerator's subtraction at +1 and the denominator's sum at
// -1, so the exponent is 0 and the product is ratio^0 = 1.0. A sign
// product reports the leaf at +1 or -1 there and moves the composite
// ratio off 1.0 (FR-019).
auto test_shared_leaf_exponent() -> void
{
  const auto source = *system::local().object("package-1/ratio-shared");
  const auto enabled = *source.counter<time_dim>("enabled");
  const auto running = *source.counter<time_dim>("running");
  // Scripted deltas: enabled 300, running 100. The enabled slot carries
  // the pair, so it discloses 100 / 300 on its own.
  const auto composite = (enabled - running) / (enabled + running);
  const auto doubled = 2.0 * composite;
  const auto compiled = compile(system::local(), composite, doubled);
  check(compiled.has_value(), "the shared-leaf ratio plan compiles");

  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  const auto metric = window.metric(composite);
  // (300 - 100) / (300 + 100) = 200 / 400.
  check(same_double(metric.value, 0.5),
        "a leaf on both sides of the fold folds its own arithmetic exactly "
        "(FR-019)");
  // The two occurrences carry exponents +1 and -1, whose sum is 0, so
  // the enabled leaf's 100 / 300 is raised to the zeroth power and the
  // composite discloses 1.0.
  check(same_double(metric.running_ratio, 1.0),
        "a leaf on both sides of the fold has exponent zero, so the "
        "composite ratio is 1.0 (FR-019)");
  // A scalar multiple multiplies the folded value and leaves every
  // exponent alone, so the disclosure is the 1.0 above.
  const auto scaled = window.metric(doubled);
  check(same_double(scaled.value, 1.0) && same_double(scaled.running_ratio, 1.0),
        "a scalar multiple of the composite scales the value and leaves the "
        "exponents alone (FR-015, FR-019)");
}

// The seeded per-sample delta generator, exercised over a whole
// recorder trace: one seed reproduces the sequence a reader can redo
// on paper (T014, FR-036).
auto test_seeded_tail() -> void
{
  const auto walk =
      *system::local().object("package-1/seeded")->counter<events>("walk");
  const expression<events> walk_expr {walk};
  const auto compiled = compile(system::local(), walk_expr);
  check(compiled.has_value(), "the seeded plan compiles");

  auto recorder = compiled->recorder(4);
  recorder.sample();
  recorder.sample();
  recorder.sample();
  recorder.sample();
  // One step is (state * 37 + 11) mod 2^16. From seed 1: 48, 1787,
  // 66130 reduced to 594, then 21989. Cumulative points: 48, 1835,
  // 2429, 24418.
  const auto column =
      walk_expr.raw(recorder.view(), "package-1/seeded", "walk");
  check(column.has_value() && column->points[0] == 48
            && column->points[1] == 1835 && column->points[2] == 2429
            && column->points[3] == 24418,
        "the seeded tail walks its whole trace from one seed (T014)");
  // 24418 - 48 = 48 + 1787 + 594 + 21989, the sum of every seeded step
  // the window spans.
  check(same_double(walk_expr.fold(recorder.view()).value, 24370.0),
        "the first-to-last fold over the seeded tail sums every step (T014)");
  const auto pairs = walk_expr.fold_pairs(recorder.view());
  check(pairs.size() == 3 && same_double(pairs[0].value, 1787.0)
            && same_double(pairs[1].value, 594.0)
            && same_double(pairs[2].value, 21989.0),
        "each pair fold yields its own seeded step (T014, FR-018)");
}

// A pair-carrying leaf whose enabled time never advances across the
// window. The fold has no measured fraction to report and states full
// rate, scaled false (FR-019).
auto test_frozen_pair_ratio() -> void
{
  const auto source = *system::local().object("package-1/ratio-frozen");
  const auto enabled = *source.counter<time_dim>("enabled");
  const auto running = *source.counter<time_dim>("running");
  const auto ratio = enabled / running;
  const auto compiled = compile(system::local(), ratio);
  check(compiled.has_value(), "the frozen-pair plan compiles");
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  const auto metric = window.metric(ratio);
  // 7 / 50 across the window: enabled did not move, so the leaf
  // discloses no measured fraction and the fold states full rate.
  // (7 - 7) / (50 - 0) = 0: the enabled delta is zero, so the
  // quotient folds to zero while the disclosure states full rate.
  check(same_double(metric.value, 0.0),
        "the frozen-pair composite folds its own arithmetic exactly");
  check(same_double(metric.running_ratio, 1.0) && !metric.scaled,
        "a pair with no elapsed enabled time discloses full rate, scaled "
        "false (FR-019)");
}

// A pair-carrying leaf whose two halves advance by the same amount, the
// ordinary un-multiplexed source. The fold compiles both halves, so the
// pair discloses a full fraction: the rate test takes its full-rate edge
// and the disclosure carries no scale (FR-019).
auto test_full_rate_pair_ratio() -> void
{
  const auto source = *system::local().object("package-1/ratio-full");
  const auto enabled = *source.counter<time_dim>("enabled");
  const auto running = *source.counter<time_dim>("running");
  const auto sum = enabled + running;
  const auto compiled = compile(system::local(), sum);
  check(compiled.has_value(), "the full-rate plan compiles");
  sg::counters::scope window {*compiled};
  window.start();
  window.finish();
  const auto metric = window.metric(sum);
  // Both scripted halves step by 900 across the window, so the sum folds
  // 900 + 900 and the pair discloses 900 / 900 = 1.
  check(same_double(metric.value, 1800.0),
        "the full-rate pair folds its own scripted arithmetic exactly");
  check(same_double(metric.running_ratio, 1.0) && !metric.scaled,
        "a source that ran at full rate discloses full rate, scaled false "
        "(FR-019)");
}

// A window reader opened directly over addresses the scripted provider
// does not serve. Every scripted address the system resolves passes this
// path, so the two refusals below are reachable only through the open
// contract itself: a leaf set the provider cannot serve opens no window,
// and an address carrying no separator names no object at all
// (FR-011, FR-036, T066).
auto open_refusal_scenario() -> void
{
  fake_provider provider;
  provider.add_object("package-1/core-3", "core", "a scripted core");
  static_cast<void>(provider.add_counter(
      "package-1/core-3", "cycles", "ops", "cycles elapsed"));
  const target where {};
  check(
      provider.open(leaf_set {.addresses = {"package-1/core-3/cycles"}}, where)
          != nullptr,
      "a scripted address opens a window");
  check(provider.open(leaf_set {.addresses = {"nosuchobject/cycles"}}, where)
            == nullptr,
        "an address naming an undeclared object opens no window");
  check(provider.open(leaf_set {.addresses = {"package-1/core-3/nosuchleaf"}},
                      where)
            == nullptr,
        "an address naming an undeclared leaf opens no window");
  check(
      provider.open(leaf_set {.addresses = {"nodelimiter"}}, where) == nullptr,
      "an address carrying no separator names no object at all");
}

// A scalar multiple of a composite: the scale is one node over the
// spine root, so the fold multiplies the sum the addition node folds to
// and that node keeps both operands at full weight (FR-016, T066).
auto scaled_composite_scenario() -> void
{
  using sg::counters::expression;
  using sg::counters::scope;
  const auto core = *system::local().object("package-1/core-3");
  const auto work = core.counter<events>("instructions");
  const auto cycle = core.counter<events>("cycles");
  check(work.has_value() && cycle.has_value(),
        "the scripted core serves the two leaves the composite spans");
  const expression<events> a {*work};
  const expression<events> b {*cycle};
  const auto sum = a + b;
  const auto scaled = 3.0 * sum;
  auto compiled = compile(system::local(), scaled, a, b);
  if (!compiled.has_value()) {
    fail("the scaled composite plan compiles");
  }
  scope window {*compiled};
  window.start();
  window.finish();
  check(
      same_double(window.metric(scaled).value,
                  3.0 * (window.metric(a).value + window.metric(b).value)),
      "a scalar multiple of a composite scales the sum its operands fold "
      "to exactly (FR-016)");
}

// A scalar multiple of a composite whose operands are themselves
// arithmetic: the scale multiplies the composite's folded value, so the
// quotient and the sum keep their own arithmetic (FR-015).
auto scaled_quotient_scenario() -> void
{
  using sg::counters::expression;
  using sg::counters::scope;
  const auto core = *system::local().object("package-1/core-3");
  const auto work = core.counter<events>("instructions");
  const auto cycle = core.counter<events>("cycles");
  check(work.has_value() && cycle.has_value(),
        "the scripted core serves the two leaves the quotient spans");
  const expression<events> a {*work};
  const expression<events> b {*cycle};
  const auto quotient = a / b;
  const auto sum = a + b;
  const auto doubled_quotient = 2.0 * quotient;
  const auto doubled_sum = 2.0 * sum;
  auto compiled = compile(
      system::local(), a, b, quotient, sum, doubled_quotient, doubled_sum);
  if (!compiled.has_value()) {
    fail("the scaled quotient plan compiles");
  }
  scope window {*compiled};
  window.start();
  window.finish();
  // Both scripted sequences are past their explicit points, so the
  // window spans the tail deltas: instructions 2100, cycles 100. The
  // two undoubled folds pin them.
  check(same_double(window.metric(quotient).value, 21.0)
            && same_double(window.metric(sum).value, 2200.0),
        "the undoubled folds pin the window's scripted deltas (FR-036)");
  // 2 * (2100 / 100) and 2 * (2100 + 100).
  check(same_double(window.metric(doubled_quotient).value, 42.0)
            && same_double(window.metric(doubled_sum).value, 4400.0),
        "a scalar multiple of a composite scales its folded value (FR-015)");
  check(window.metric(doubled_quotient).scaled
            && window.metric(doubled_sum).scaled
            && !window.metric(quotient).scaled && !window.metric(sum).scaled,
        "the scaled flag reports a scale the caller applied and no other "
        "(FR-019)");
}

// A scaled expression taken as a spliced operand: the division splices
// the scale node into a position past the three nodes the left operand
// contributed, so the fold reaches it through a remapped left index. The
// scaled spine is the right operand on purpose, because a left operand
// splices at base zero and a remap there changes nothing
// (FR-015, US1 scenario 3).
auto scaled_spliced_operand_scenario() -> void
{
  using sg::counters::expression;
  using sg::counters::scope;
  const auto scratch = *system::local().object("package-1/splice");
  const auto only = *scratch.counter<events>("only");
  const auto numerator = *scratch.counter<events>("numerator");
  const auto scaled_operand = *scratch.counter<events>("scaled_operand");
  // Every window of this object sees the same deltas, because each
  // leaf's tail step repeats its scripted step: only 200, numerator
  // 2100, scaled_operand 400 (FR-036). The scaled leaf's delta differs
  // from every other leaf of the composition, so a scale node reaching
  // the wrong operand folds a value of its own: node 0 is the leaf
  // `only` at 200, which divides as 2300 / 400 = 5.75.
  const auto sum = only + numerator;
  const auto doubled_operand = 2.0 * expression<events> {scaled_operand};
  const auto folded = sum / doubled_operand;
  auto compiled = compile(system::local(), folded);
  if (!compiled.has_value()) {
    fail("the spliced scaled-operand plan compiles");
  }
  scope window {*compiled};
  window.start();
  window.finish();
  // (200 + 2100) / (2 * 400) = 2300 / 800.
  check(same_double(window.metric(folded).value, 2.875),
        "a scaled expression spliced as an operand folds its own "
        "arithmetic exactly (FR-015, US1 scenario 3)");
}

}  // namespace

// A fan-out over every core opens one window per core, so the plan holds
// several groups. Only the group owning the plan's last leaf writes the
// disclosure column, which leaves each earlier group with no column of
// its own (FR-007). A provider reading such a group writes no disclosure
// beside its points.
auto test_multi_group_disclosure() -> void
{
  const auto all_cores = system::local().objects("core");
  check(all_cores.has_value() && all_cores->size() > 1,
        "the fixture publishes more than one core");
  if (!all_cores.has_value() || all_cores->size() <= 1) {
    return;
  }
  const auto core = *system::local().object("package-1/core-3");
  const auto cycles = *core.counter<events>("cycles");
  const auto instructions = *core.counter<events>("instructions");
  const auto ipc = instructions / cycles;

  auto fanout = compile(system::local(), ipc, *all_cores);
  check(fanout.has_value(), "a fan-out over every core compiles");
  if (!fanout.has_value()) {
    return;
  }
  sg::counters::fanout_plan target = std::move(*fanout);
  auto rec = target.recorder(2);
  rec.sample();
  rec.sample();
  const auto folded = target.fold(ipc, rec.view());
  check(folded.size() == all_cores->size(),
        "the fan-out reports one metric per selected core");
}

auto main() -> int
{
  test_registration();
  test_tree_walk();
  test_resolution_diagnostics();
  test_compile_zero_reads();
  test_scope_exactness();
  test_additive_algebra();
  test_construction_and_fold_edges();
  test_multi_expression_splice();
  test_move_assignment();
  test_nested_parent_and_suggestions();
  test_fanout_construction_errors();
  test_permission_blocked_leaf();
  test_wrap();
  test_ratio_product();
  test_shared_leaf_exponent();
  test_frozen_pair_ratio();
  test_full_rate_pair_ratio();
  test_seeded_tail();
  test_multi_group_disclosure();
  open_refusal_scenario();
  scaled_composite_scenario();
  scaled_quotient_scenario();
  scaled_spliced_operand_scenario();
  std::printf("counters_fake_test: all checks passed\n");
  return 0;
}
