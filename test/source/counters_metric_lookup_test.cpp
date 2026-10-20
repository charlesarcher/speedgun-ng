// ============================================================================
// TDD test for the metric lookup and the fold it feeds
// (specs/022-metric-formulas T027 to T034, T056, T057; FR-008 to FR-021,
// FR-038).
//
// Every case resolves the fixture table against the fixture host and folds
// the spine through the same compile-and-window path a hand-composed
// expression takes, so the assertion is a value of its own (FR-039).
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/counters.hpp"

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/embedded_tables.hpp"
#include "../source/counters/linux_pmu/metric_tables.hpp"
#include "detail/metric_fixture.hpp"

namespace
{

using Events = sg::counters::Dim<0, 1>;
using Duration = sg::counters::Dim<1, 0>;

using sg::counters::Availability;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::MetricRefusal;
using sg::counters::Object;
using sg::counters::Scope;
using sg::counters::System;
using sg::counters::detail::MetricDefinition;
using sg::counters::detail::metricDefinition;
using sg::counters::detail::metricRows;
using sg::counters::detail::parsedMetricTable;
using sg::counters::detail::parseMetricRows;
using sg::counters::detail::ResolvedMetricTable;
using sg::counters::detail::resolvedMetricTable;

using RowList = std::vector<sg::test::MetricDefinition>;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "METRIC LOOKUP TEST FAIL: %s\n", what);
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

// The value comparisons print both figures: a fold that differs by one
// unit in the last place is a real difference here, and the two numbers
// say which operand drifted.
auto checkSame(const double got, const double want, const char* what) -> void
{
  if (!sameDouble(got, want)) {
    std::fprintf(
        stderr,
        "METRIC LOOKUP TEST FAIL: %s: folded %.17g, expected " "%.17g\n",
        what,
        got,
        want);
    std::exit(1);
  }
}

[[nodiscard]] auto objectAt(std::string_view path) -> Object
{
  const auto settled = System::local().object(path);
  if (!settled.has_value()) {
    std::fprintf(stderr, "METRIC LOOKUP TEST FAIL: no fixture object\n");
    std::exit(1);
  }
  return *settled;
}

template<class E>
[[nodiscard]] auto foldValue(const E& expr) -> double
{
  const auto compiled = compile(System::local(), expr);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "METRIC LOOKUP TEST FAIL: the plan refused\n");
    std::exit(1);
  }
  Scope window {*compiled};
  window.start();
  window.finish();
  const auto result = window.metric(expr);
  check(result.availability == Availability::COUNTABLE,
        "the fixture host answers the leaves the spine holds (FR-039)");
  return result.value;
}

[[nodiscard]] auto coreOf(const sg::test::MetricDefinition& def)
    -> Expression<Dim<0, 0>>
{
  Expression<Dim<0, 0>> expr;
  expr.core = def.core;
  return expr;
}

// The rows whose spine keeps a counted dimension fold through an
// expression of that dimension: the helper-scaled cycle row and the
// product of two leaves.
[[nodiscard]] auto eventsOf(const sg::test::MetricDefinition& def)
    -> Expression<Dim<0, 1>>
{
  Expression<Dim<0, 1>> expr;
  expr.core = def.core;
  return expr;
}

[[nodiscard]] auto productOf(const sg::test::MetricDefinition& def)
    -> Expression<Dim<0, 2>>
{
  Expression<Dim<0, 2>> expr;
  expr.core = def.core;
  return expr;
}

// The whole fixture table, resolved once for the name-rule cases.
[[nodiscard]] auto allRows() -> const RowList&
{
  static const RowList kRows = metricRows(sg::test::fixtureTable(),
                                          sg::test::fixtureEvents(),
                                          sg::test::fixtureHost());
  return kRows;
}

[[nodiscard]] auto resolved(const std::string_view name)
    -> sg::test::MetricDefinition
{
  auto settled = metricDefinition(sg::test::fixtureTable(),
                                  sg::test::fixtureEvents(),
                                  sg::test::fixtureHost(),
                                  name);
  if (!settled.has_value()) {
    std::fprintf(stderr,
                 "METRIC LOOKUP TEST FAIL: '%s' refused: %s\n",
                 std::string(name).c_str(),
                 settled.error().message.c_str());
    std::exit(1);
  }
  check(settled->published, "the fixture row publishes");
  return std::move(*settled);
}

[[nodiscard]] auto refused(const std::string_view name)
    -> const sg::test::MetricDefinition&
{
  for (const auto& row : allRows()) {
    if (row.name == name) {
      if (row.published) {
        std::fprintf(stderr,
                     "METRIC LOOKUP TEST FAIL: %s published: %s\n",
                     std::string(name).c_str(),
                     row.reason.c_str());
        std::exit(1);
      }
      check(!row.reason.empty(), "a refusal carries the text of its rule");
      return row;
    }
  }
  fail("a fixture row of the refused name");
}

// T027, T031: the value rows, against the same expression written by hand.
auto valueScenario() -> void
{
  const auto core = objectAt(sg::test::kCoreObject);
  const auto memory = objectAt(sg::test::kMemoryObject);
  const auto cycles = *core.counter<Events>(sg::test::kCycles);
  const auto instructions = *core.counter<Events>(sg::test::kInstructions);
  const auto l2 = *core.counter<Events>(sg::test::kL2Requests);
  const auto time = *core.counter<Duration>(sg::test::kTime);
  const auto reads = *memory.counter<Events>(sg::test::kDataReads);

  const auto cpi = resolved("cpi");
  const Expression<Dim<0, 0>> hand = cycles / instructions;
  checkSame(foldValue(coreOf(cpi)),
            foldValue(hand),
            "cpi folds to the value the same hand-composed quotient folds to "
            "(FR-015)");
  checkSame(foldValue(coreOf(cpi)),
            sg::test::kCyclesDelta / sg::test::kInstructionsDelta,
            "cpi is the scripted delta ratio (FR-015)");
  check(cpi.group == "fixture" && cpi.unit.empty(),
        "the row's group and unit text travel verbatim (FR-021)");
  check(cpi.objectPath == sg::test::kCoreObject,
        "the listing prints the row under its first leaf's object (FR-020)");

  const auto mpi = resolved("l2_mpi");
  check(sameDouble(foldValue(coreOf(mpi)),
                   sg::test::kL2Delta * 1000.0 / sg::test::kInstructionsDelta),
        "the table's scale factor enters the folded value once (FR-018)");
  check(mpi.unit == "1e3per_1k_instr",
        "the scale-and-unit text stays the table's own (FR-018, FR-021)");

  const auto percent = resolved("percent");
  check(sameDouble(foldValue(coreOf(percent)),
                   100.0 * sg::test::kCyclesDelta / sg::test::kRefCyclesDelta),
        "the 100% row carries the table's factor of 100 (FR-018)");
  check(percent.unit == "100%",
        "the scale text is printed as written (FR-018)");

  const auto operand = resolved("helper_operand");
  check(sameDouble(foldValue(eventsOf(operand)), sg::test::kCyclesDelta),
        "a helper settled to one scales its operand spine (FR-004)");
  const auto product = resolved("prod_row");
  check(sameDouble(foldValue(productOf(product)),
                   sg::test::kCyclesDelta * sg::test::kInstructionsDelta),
        "a product of two leaves folds on both operands (FR-010)");
  const auto paren = resolved("cond_paren");
  check(sameDouble(foldValue(eventsOf(paren)), sg::test::kCyclesDelta),
        "a settled conditional in a parenthesized arm walks as its taken "
        "literal (FR-005)");
  const auto guard = resolved("guard_div");
  check(sameDouble(foldValue(eventsOf(guard)), sg::test::kInstructionsDelta),
        "a guarded division of a counted left keeps the counted dimension "
        "(FR-008)");
  const auto hex = resolved("hex_row");
  check(sameDouble(foldValue(eventsOf(hex)), 2.0 * sg::test::kCyclesDelta),
        "a hexadecimal literal loads like its decimal spelling (FR-003)");
  const auto escaped = resolved("esc_ref");
  check(sameDouble(foldValue(eventsOf(escaped)), sg::test::kCyclesDelta),
        "an escaped name in a bare reference decodes to the row it names "
        "(FR-003)");
  const auto hexUpper = resolved("hex_upper");
  check(sameDouble(foldValue(eventsOf(hexUpper)), 2.0 * sg::test::kCyclesDelta),
        "an uppercase hexadecimal prefix loads like the lowercase one "
        "(FR-003)");
  const auto expPos = resolved("exp_pos");
  check(sameDouble(foldValue(eventsOf(expPos)), 100.0 * sg::test::kCyclesDelta),
        "an exponent carrying its sign loads like its product (FR-003)");
  const auto expNeg = resolved("exp_neg");
  check(sameDouble(foldValue(eventsOf(expNeg)), sg::test::kCyclesDelta / 100.0),
        "a negative exponent divides by its power (FR-003)");
  const auto expUpper = resolved("exp_upper");
  check(
      sameDouble(foldValue(eventsOf(expUpper)), 100.0 * sg::test::kCyclesDelta),
      "the exponent letter may stand uppercase (FR-003)");
  const auto expSignUpper = resolved("exp_sign_upper");
  check(sameDouble(foldValue(eventsOf(expSignUpper)),
                   100.0 * sg::test::kCyclesDelta),
        "an uppercase exponent letter carries its sign too (FR-003)");
  const auto bothCond = resolved("both_cond");
  check(sameDouble(foldValue(eventsOf(bothCond)),
                   6.0 * sg::test::kCyclesDelta),
        "a conditional rides the right operand's seat of a pairing "
        "(FR-005, FR-010)");
  const auto thirdCond = resolved("third_cond");
  check(
      sameDouble(foldValue(eventsOf(thirdCond)), 4.0 * sg::test::kCyclesDelta),
      "a conditional nests in its own else-branch (FR-005)");
  const auto minLeft = resolved("min_left");
  check(sameDouble(foldValue(eventsOf(minLeft)), 2.0),
        "a guard's literal may sit on the helper's left (FR-004)");
  const auto scaled = resolved("scaled_row");
  check(scaled.unit == "4Bytes",
        "the row keeps the scale unit it spells (FR-020)");
  const auto eventBlocked = resolved("event_blocked");
  check(sameDouble(foldValue(eventsOf(eventBlocked)), sg::test::kCyclesDelta),
        "a leaf the host will not count answers has_event false (FR-011)");
  const auto devCore = resolved("dev_core");
  check(sameDouble(foldValue(eventsOf(devCore)), sg::test::kCyclesDelta),
        "a device token seats the reference on its object (FR-003)");
  const auto devCpu = resolved("dev_cpu");
  check(sameDouble(foldValue(eventsOf(devCpu)), sg::test::kCyclesDelta),
        "the cpu token reaches the same counted leaves (FR-003)");
  const auto heTrue = resolved("he_mul_true");
  check(sameDouble(foldValue(eventsOf(heTrue)), sg::test::kCyclesDelta),
        "a settled helper outside a test walks as one (FR-004)");
  const auto heFalse = resolved("he_mul_false");
  check(sameDouble(foldValue(eventsOf(heFalse)), 0.0),
        "a helper the host answers no walks as zero (FR-004, FR-011)");
  const auto guardRight = resolved("guard_right_dim");
  check(sameDouble(foldValue(eventsOf(guardRight)), sg::test::kCyclesDelta),
        "a guard's literal may sit on the helper's right (FR-004)");
  const auto guardLeftDim = resolved("guard_left_dim");
  check(sameDouble(foldValue(eventsOf(guardLeftDim)), sg::test::kCyclesDelta),
        "a guard's literal may sit on the helper's left (FR-004)");
  check(guardLeftDim.events == 1 && guardLeftDim.time == 0,
        "a left guard takes its sibling's dimension too (FR-004, FR-010)");
  const auto blockedPair = resolved("blocked_pair");
  check(blockedPair.published,
        "a blocked leaf rides a pairing and the row still publishes "
        "(FR-011, FR-022)");
  const auto guardLeft = resolved("test_guard_left");
  check(guardLeft.published,
        "a parenthesized conditional rides an addition's left (FR-005)");
  const auto guardRight2 = resolved("test_guard_right");
  check(guardRight2.published,
        "a parenthesized conditional rides an addition's right (FR-005)");

  const auto bandwidth = resolved("bandwidth");
  const Expression<Dim<-1, 1>> handRate = reads / time;
  check(sameDouble(foldValue(coreOf(bandwidth)), foldValue(handRate)),
        "a row whose leaf carries the time exponent folds as the same "
        "hand-composed rate does (FR-010)");
}

// T028: a reference to another metric expands into that metric's spine.
auto nestedScenario() -> void
{
  const auto clks = resolved("tma_info_thread_clks");
  check(sameDouble(foldValue(coreOf(clks)), 2.0 * sg::test::kCyclesDelta),
        "the inner metric folds on its own (FR-008)");

  const auto core = objectAt(sg::test::kCoreObject);
  const auto cycles = *core.counter<Events>(sg::test::kCycles);
  const auto instructions = *core.counter<Events>(sg::test::kInstructions);
  const Expression<Dim<0, 0>> hand =
      instructions / (2.0 * Expression<Events> {cycles});
  const auto ipc = resolved("tma_info_thread_ipc");
  check(sameDouble(foldValue(coreOf(ipc)), foldValue(hand)),
        "the outer metric inlines the inner one, so the value is the "
        "hand-composed expansion (FR-008)");
}

// T029, T030: the table's own name rules.
auto tableScenario() -> void
{
  const auto& rows = allRows();
  const auto& loopA = refused("loop_a");
  check(loopA.refusal == MetricRefusal::CYCLE,
        "a reference chain that returns to the metric refuses (FR-012)");
  check(loopA.reason.find("loop_b") != std::string::npos,
        "the cycle reason names the chain (FR-021)");

  std::size_t dupes = 0;
  std::size_t cycles = 0;
  for (const auto& row : rows) {
    dupes += row.name == "dupe" ? 1U : 0U;
    cycles += row.refusal == MetricRefusal::CYCLE ? 1U : 0U;
  }
  check(dupes == 2, "both rows of a duplicated name stay visible (FR-021)");
  check(cycles == 2,
        "every member of a cycle is refused, not only the entry point "
        "(FR-012)");

  const auto dupe = resolved("dupe");
  check(sameDouble(foldValue(coreOf(dupe)), sg::test::kCyclesDelta),
        "the first row of a duplicated name is the one the lookup reaches "
        "(FR-011)");
  for (const auto& row : rows) {
    if (row.name == "dupe" && !row.published) {
      check(row.refusal == MetricRefusal::COLLISION,
            "the later row carries the collision refusal (FR-011)");
    }
  }
}

// T032, T033: the miss text, the no-table case and the once-only load.
auto lookupScenario() -> void
{
  const auto miss = metricDefinition(sg::test::fixtureTable(),
                                     sg::test::fixtureEvents(),
                                     sg::test::fixtureHost(),
                                     "cp1");
  check(!miss.has_value(), "a name the table lacks refuses (FR-016)");
  if (miss.has_value()) {
    return;
  }
  check(miss.error().message.find("cp1") != std::string::npos,
        "the miss names the queried text (FR-016)");
  const auto& names = miss.error().suggestions;
  check(std::find(names.begin(), names.end(), "cpi") != names.end(),
        "the near-miss name rides beside the miss (FR-016)");

  const auto empty = metricDefinition(parseMetricRows({}),
                                      std::vector<sg::test::MetricRow> {},
                                      sg::test::noMetricHost(),
                                      "cpi");
  check(!empty.has_value(),
        "a host that selected no event table has no metric to name (FR-017)");

  const auto directory = std::string("fixture/021/");
  check(&parsedMetricTable(directory) == &parsedMetricTable(directory),
        "one directory parses its metric rows once (FR-019)");
  const auto host = sg::test::fixtureHost();
  check(&resolvedMetricTable(host) == &resolvedMetricTable(host),
        "one host resolves its table once and reuses it (FR-019)");
  // The shared table of the directory this CPU selected is the same rows the
  // catalog prints, so the lookup and the listing read one table (FR-021).
  const auto listed = sg::counters::catalogMetrics();
  check(
      resolvedMetricTable(sg::counters::detail::metricHostCurrent()).rows.size()
          == listed.size(),
      "the shared table holds the resolved rows (FR-021)");
}

// T034 and the remaining refusals of FR-003, FR-006, FR-010, FR-012, FR-013
// and FR-018.
auto refusalScenario() -> void
{
  const auto core = objectAt(sg::test::kCoreObject);
  const auto cycles = *core.counter<Events>(sg::test::kCycles);
  const auto l2 = *core.counter<Events>(sg::test::kL2Requests);

  check(refused("bad_scale").refusal == MetricRefusal::UNKNOWN_SCALE,
        "a ScaleUnit text the scale table lacks refuses (FR-018)");
  check(refused("bad_pmem").refusal == MetricRefusal::UNKNOWN_CONSTANT,
        "a constant with no reader refuses (FR-006)");
  const auto& leaf = refused("bad_leaf");
  check(leaf.refusal == MetricRefusal::UNRESOLVED,
        "a reference that names no leaf this host counts refuses (FR-013)");
  check(leaf.reason.find("fixture_no_such_event") != std::string::npos,
        "the unresolved reason names the reference (FR-021)");
  const auto& add = refused("bad_add");
  check(add.refusal == MetricRefusal::DIMENSION,
        "terms of different dimensions refuse the row (FR-010)");
  check(add.reason.find("terms do not compose") != std::string::npos,
        "the reason says which rule refused (FR-021)");
  check(refused("bad_divide").refusal == MetricRefusal::UNSUPPORTED,
        "a formula that divides by zero refuses (FR-012)");
  check(refused("bad_modifier").refusal == MetricRefusal::UNSUPPORTED,
        "a modifier the catalog entry does not carry refuses (FR-003)");
  check(refused("bare_cmp").refusal == MetricRefusal::MALFORMED,
        "a comparison outside a conditional strands text the root refuses "
        "malformed (FR-002, FR-005)");
  check(refused("bad_test_close").refusal == MetricRefusal::MALFORMED,
        "a test that fails its nested parse refuses malformed (FR-002)");
  check(refused("bad_test_star").refusal == MetricRefusal::MALFORMED,
        "a comparison whose right operand fails its parse refuses malformed "
        "(FR-002)");
  check(
      refused("unterminated").refusal == MetricRefusal::UNSUPPORTED,
      "a character the grammar does not seat refuses unsupported " "(FR-005)");
  check(refused("bad_arity").refusal == MetricRefusal::MALFORMED,
        "a helper called with two arguments refuses malformed (FR-002)");
  // The scanner's edges, each stranded its own way (FR-003, FR-002).
  check(refused("dotted_ref").refusal == MetricRefusal::UNRESOLVED,
        "a dot joins a reference's characters (FR-003)");
  check(refused("dashed_ref").refusal == MetricRefusal::UNRESOLVED,
        "a dash joins a reference's characters (FR-003)");
  check(refused("underscore_ref").refusal == MetricRefusal::UNRESOLVED,
        "an identifier may open on an underscore (FR-003)");
  check(refused("dangling").refusal == MetricRefusal::UNSUPPORTED,
        "an escape at the text's end strands the scan (FR-003)");
  check(refused("colon_tail").refusal == MetricRefusal::UNSUPPORTED,
        "a colon without a privilege word after it refuses (FR-003)");
  check(refused("colon_slash").refusal == MetricRefusal::UNSUPPORTED,
        "a colon before a non-word character refuses (FR-003)");
  check(refused("hex_mid").refusal == MetricRefusal::DIMENSION,
        "a hexadecimal digit run hands the next character back (FR-003)");
  check(refused("empty_call").refusal == MetricRefusal::MALFORMED,
        "a helper call that closes empty refuses malformed (FR-002)");
  check(refused("time_mix").refusal == MetricRefusal::DIMENSION,
        "a duration term does not add into a counted one (FR-006, FR-010)");
  check(refused("tight_dash").refusal == MetricRefusal::DIMENSION,
        "a digit hands the dash back mid-token and the difference reads "
        "(FR-003, FR-010)");
  check(refused("plus_mid").refusal == MetricRefusal::DIMENSION,
        "a plus after an exponent's digits reads and the sum refuses "
        "(FR-003, FR-010)");
  check(refused("time_stack").refusal == MetricRefusal::DIMENSION,
        "a squared duration does not add into a plain one (FR-006, FR-010)");
  check(refused("nest_deep").refusal == MetricRefusal::NESTED,
        "a helper under a guard inside a guard refuses nested (FR-002)");
  check(refused("test_add_plain").refusal == MetricRefusal::DIMENSION,
        "a sum as the test composes no pair and refuses (FR-005, FR-010)");
  // The parser's own refusals, each named by the rule that bites
  // (FR-003, FR-005).
  check(refused("trailing_escape").refusal == MetricRefusal::UNSUPPORTED,
        "an escape with no partner at the text's edge refuses (FR-003)");
  check(refused("test_cond").refusal == MetricRefusal::UNSUPPORTED,
        "a conditional inside a test without a comparison refuses " "(FR-005)");
  check(refused("guard_left_cmp").refusal == MetricRefusal::UNSUPPORTED,
        "a conditional on a comparison's left refuses (FR-005)");
  check(refused("guard_right_cmp").refusal == MetricRefusal::UNSUPPORTED,
        "a conditional on a comparison's right refuses (FR-005)");
  check(refused("guard_left_deep").refusal == MetricRefusal::UNSUPPORTED,
        "a conditional under a sum on a comparison's left refuses (FR-005)");
  check(refused("guard_right_deep").refusal == MetricRefusal::UNSUPPORTED,
        "a conditional under a sum on a comparison's right refuses (FR-005)");
  check(refused("nested_arg").refusal == MetricRefusal::NESTED,
        "a helper inside a guard's argument refuses nested (FR-002)");
  check(refused("nest_left").refusal == MetricRefusal::NESTED,
        "a helper under an arithmetic argument refuses nested (FR-002)");
  check(refused("nest_right").refusal == MetricRefusal::NESTED,
        "a helper two guards deep refuses nested (FR-002)");
  check(
      refused("nest_add_right").refusal == MetricRefusal::NESTED,
      "a helper on a sum's right seat under a guard refuses nested " "(FR-"
                                                                     "002)");
  const auto& nofact = refused("test_missing_right");
  check(
      nofact.refusal == MetricRefusal::UNKNOWN_CONSTANT,
      "a settled test whose right constant this host lacks refuses " "(FR-"
                                                                     "006)");
  check(resolved("test_source").published,
        "a source_count test settles on the table's own rows (FR-004)");

  const auto memory = resolved("memory");
  check(memory.objectPath == sg::test::kMemoryObject,
        "a row of the memory class resolves against that class's object "
        "(FR-013, FR-014)");

  const auto present = resolved("present");
  check(sameDouble(foldValue(coreOf(present)), sg::test::kCyclesDelta),
        "the then arm answers a predicate the host satisfies (FR-004)");
  const auto absent = resolved("absent");
  check(sameDouble(foldValue(coreOf(absent)), sg::test::kRefCyclesDelta),
        "the else arm answers a predicate the host fails (FR-004)");
  const auto sources = resolved("sources");
  check(sameDouble(foldValue(coreOf(sources)), sg::test::kCyclesDelta),
        "source_count counts the directory's own rows (FR-004)");
  const auto floors = resolved("floors");
  check(sameDouble(foldValue(coreOf(floors)), sg::test::kRefCyclesDelta),
        "the min of three rows folds to the least (FR-004)");
  const auto ratio = resolved("ratio");
  const Expression<Dim<0, 0>> handRatio = l2 / cycles;
  check(sameDouble(foldValue(coreOf(ratio)), foldValue(handRatio)),
        "d_ratio's guard never fires against a running counter (FR-004)");

  check(resolved("smt_row").published,
        "the conditional pair resolves on the side the host fact answers "
        "(FR-005)");
  check(resolved("core_row").published,
        "a literal constant inside a formula resolves (FR-006)");
}

// T058: the resolver paths a bare reference never walks, held apart from the
// pinned fixture table so the catalog listing that table makes stays the one
// the drift fixtures count (FR-040).
auto factScenario() -> void
{
  const auto host = sg::test::fixtureHost();
  const auto events = sg::test::fixtureEvents();
  const auto define = [&](const std::string_view formula)
  {
    const std::vector<sg::test::MetricRow> rows {
        sg::test::fixtureEvent(sg::test::kCycles),
        sg::test::fixtureEvent(sg::test::kRefCycles),
        sg::test::fixtureMetric("row", std::string(formula)),
    };
    const auto table = sg::counters::detail::parseMetricRows(rows);
    return sg::counters::detail::metricDefinition(table, events, host, "row");
  };
  const auto valueOf = [](const auto& settled)
  { return foldValue(coreOf(*settled)); };

  const auto cores = define("#num_cores");
  const auto packages = define("#num_packages");
  check(cores.has_value() && packages.has_value(),
        "two host facts publish on their own (FR-006)");
  // A spine of facts alone compiles to no counter plan, so the arithmetic is
  // folded behind a leaf the window can open: the identity holds on either
  // shape, and the leaf keeps the fold observable.
  const auto wrapped = [&](const std::string_view fact) {
    return define(std::string("fixture_cycles * (") + std::string(fact) + ")");
  };
  const auto sum = wrapped("#num_cores + #num_packages");
  const auto swapped = wrapped("#num_packages + #num_cores");
  check(sum.has_value() && swapped.has_value(),
        "a host fact combined by an operator publishes (FR-006)");
  check(sameDouble(valueOf(sum), valueOf(swapped)),
        "a fact sum answers the same total in either order (FR-009)");
  const auto scaled = wrapped("#num_cores * 2");
  const auto twice = wrapped("2 * #num_cores");
  check(scaled.has_value() && twice.has_value()
            && sameDouble(valueOf(scaled), valueOf(twice)),
        "a fact scaled by a constant answers the same value (FR-009)");
  const auto least = define("fixture_cycles * min(#num_cores, #num_packages)");
  const auto great = define("fixture_cycles * max(#num_cores, #num_packages)");
  check(least.has_value() && great.has_value()
            && sameDouble(valueOf(least) + valueOf(great), valueOf(sum)),
        "the least and the greatest of two facts add to their sum (FR-004)");
  const auto difference = wrapped("#num_cores - #num_packages");
  const auto reversed = wrapped("#num_packages - #num_cores");
  check(difference.has_value() && reversed.has_value()
            && sameDouble(valueOf(difference) + valueOf(reversed), 0.0),
        "a fact difference and its reverse cancel to zero (FR-009)");
  const auto quotient = wrapped("#num_cores / (#num_packages + 1)");
  check(quotient.has_value(),
        "a fact quotient over a guarded divisor publishes (FR-009, FR-012)");

  const auto failedTest =
      define("fixture_cycles if #num_cores < 0 else fixture_ref_cycles");
  check(failedTest.has_value()
            && sameDouble(valueOf(failedTest), sg::test::kRefCyclesDelta),
        "the else arm answers a fact test the host fails (FR-005)");
  const auto rightFact =
      define("fixture_cycles if 0 < #num_cores else fixture_ref_cycles");
  const auto rightValue = rightFact.has_value() ? valueOf(rightFact) : 0.0;
  check(rightFact.has_value()
            && (sameDouble(rightValue, sg::test::kCyclesDelta)
                || sameDouble(rightValue, sg::test::kRefCyclesDelta)),
        "a conditional whose test compares against a fact on the right "
        "answers one of its arms (FR-005)");
}

// T058: the expansion limit of FR-012, reached by a chain no table spells.
auto chainScenario() -> void
{
  const auto host = sg::test::fixtureHost();
  const auto events = sg::test::fixtureEvents();
  std::vector<sg::test::MetricRow> rows {
      sg::test::fixtureEvent(sg::test::kCycles),
  };
  for (int here = 0; here < 66; ++here) {
    rows.push_back(sg::test::fixtureMetric(
        "chain" + std::to_string(here),
        here == 65 ? std::string(sg::test::kCycles)
                   : "chain" + std::to_string(here + 1)));
  }
  const auto table = sg::counters::detail::parseMetricRows(rows);
  const auto list = sg::counters::detail::metricRows(table, events, host);
  const sg::test::MetricDefinition* found = nullptr;
  for (const auto& row : list) {
    if (row.name == "chain0") {
      found = &row;
    }
  }
  check(found != nullptr,
        "the head of a long chain still reaches a verdict (FR-012)");
  if (found != nullptr) {
    check(!found->published,
          "a reference chain past the expansion limit refuses (FR-012)");
    check(found->refusal == MetricRefusal::CYCLE,
          "the expansion limit refuses as a cycle (FR-012)");
    check(found->reason.find("longer than") != std::string::npos,
          "the limit reason names the limit (FR-021)");
  }
}

// The host-parameterised lookup: the answers one table gives on its own, the
// answer of a host that named no directory, and the pass-through into the
// public view (FR-016, FR-017, FR-021).
auto hostScenario() -> void
{
  const auto noTable = sg::counters::detail::metricDefinitionOnHost(
      sg::test::noMetricHost(), "cpi");
  check(!noTable.has_value(),
        "a host that named no directory names no metric (FR-017)");
  check(noTable.error().message.find("no event-table directory")
            != std::string::npos,
        "the reason says the CPU matched no directory (FR-017)");
  check(
      sg::counters::detail::metricRowsOnHost(sg::test::noMetricHost()).empty(),
      "a host that named no directory lists no metric row (FR-017)");
  check(sg::counters::detail::metricListingsOnHost(sg::test::noMetricHost())
            .empty(),
        "a host that named no directory lists no catalog line (FR-017)");
  // The lines of the directory this CPU selected mirror its rows row for
  // row; where no directory was selected both sides are empty and the
  // equality holds the same way (FR-021, FR-022).
  const auto currentLines = sg::counters::detail::metricListingsOnHost(
      sg::counters::detail::metricHostCurrent());
  const auto currentRows = sg::counters::detail::metricRowsOnHost(
      sg::counters::detail::metricHostCurrent());
  check(currentLines.size() == currentRows.size(),
        "the catalog lines mirror the metric rows of this host (FR-021)");
  bool linesMatchRows = true;
  for (std::size_t i = 0; i != currentLines.size(); ++i) {
    linesMatchRows = linesMatchRows
        && currentLines[i].name == currentRows[i].name
        && currentLines[i].objectPath == currentRows[i].objectPath
        && currentLines[i].published == currentRows[i].published;
  }
  check(linesMatchRows,
        "each line carries its row's name, object path and verdict (FR-022)");

  // A table of two rows, hand-built and resolved here: what the lookup
  // answers is a property of the table, one every host shares
  // (FR-016, FR-021).
  ResolvedMetricTable table;
  MetricDefinition row;
  row.name = "cpi_core";
  row.group = "pipeline";
  row.unit = "ratio";
  row.description = "a row of no directory";
  row.published = true;
  table.rows.push_back(row);
  table.index[row.name] = 0;
  row.name = "cpi_package";
  row.published = false;
  row.refusal = sg::counters::MetricRefusal::UNRESOLVED;
  row.reason = "names no leaf this host counts";
  table.rows.push_back(row);
  table.index[row.name] = 1;

  const auto shown =
      sg::counters::detail::metricDefinitionIn(table, "cpi_core");
  check(shown.has_value() && shown->published && shown->group == "pipeline",
        "a published row answers its own name (FR-016)");
  const auto view = sg::counters::detail::metricViewResponse(shown);
  check(view.has_value() && view->group == "pipeline" && view->unit == "ratio",
        "the view carries the group and unit text of the row (FR-016, FR-018)");

  const auto denied =
      sg::counters::detail::metricDefinitionIn(table, "cpi_package");
  check(!denied.has_value(), "a refused row answers with an error (FR-012)");
  check(
      denied.error().message.find(':') != std::string::npos
          && denied.error().message.find("names no leaf") != std::string::npos,
      "the refusal message names the row beside its reason (FR-021)");

  const auto miss = sg::counters::detail::metricDefinitionIn(table, "cpi_cord");
  check(!miss.has_value(), "a name the table lacks has no answer (FR-016)");
  check(miss.error().message.find("cpi_cord") != std::string::npos,
        "the miss names the queried text (FR-021)");
  check(!miss.error().suggestions.empty(),
        "the miss carries the near-miss names (FR-016)");

  const auto refusedView = sg::counters::detail::metricViewResponse(denied);
  check(!refusedView.has_value()
            && refusedView.error().message == denied.error().message,
        "the lookup error reaches the public answer unchanged (FR-021)");

  check(sg::counters::detail::metricRowsOnHost(
            sg::counters::detail::metricHostCurrent())
                .size()
            == sg::counters::catalogMetrics().size(),
        "the shared rows of this host are the rows the catalog lists (FR-021)");
  const auto absent = sg::counters::metricByName("metric_no_host_names");
  check(
      !absent.has_value(),
      "the public lookup of a name no table carries answers an error (FR-016)");

  double count = -1.0;
  check(!sg::counters::detail::countObjects(
            *sg::test::fixtureHost().system, "no-such-kind", count),
        "a kind the tree publishes nothing for answers no count (FR-006)");
}

}  // namespace

// T056, T057: the verdict sweep of FR-038. Every metric row of every
// vendored directory has to reach a verdict, published or refused with a
// reason, and every `ScaleUnit` spelling it reads has to stand in the
// conversion table, so no row and no spelling of the tree is dropped
// without being accounted for. The rows resolve against the fixture host, so
// a directory published on a real processor refuses here by design: the
// sweep reports verdicts.
auto verdictSweepScenario() -> void
{
  std::size_t directories = 0;
  std::size_t rows = 0;
  std::size_t published = 0;
  std::size_t refused = 0;
  std::size_t reasonless = 0;
  std::size_t unknownScale = 0;
  const auto host = sg::test::fixtureHost();
  for (std::size_t here = 0; here < sg::counters::detail::kEmbeddedDirCount;
       ++here)
  {
    const std::string_view path {
        sg::counters::detail::kEmbeddedDirs[here].path};
    if (!path.starts_with("arch/x86/")
        || path.find('/', std::string_view {"arch/x86/"}.size())
            != std::string_view::npos)
    {
      continue;
    }
    ++directories;
    const std::string directory {path};
    const auto& table = parsedMetricTable(directory);
    const auto definitions =
        metricRows(table, sg::counters::detail::pmuLoadTable(directory), host);
    check(definitions.size() == table.rows.size(),
          "no metric row of a vendored directory is dropped (FR-038)");
    for (const auto& def : definitions) {
      ++rows;
      if (def.published) {
        ++published;
        continue;
      }
      ++refused;
      if (def.reason.empty()) {
        ++reasonless;
        std::printf("SWEEP: %s refuses '%s' with no reason\n",
                    directory.c_str(),
                    def.name.c_str());
      }
    }
    for (const auto& row : table.rows) {
      if (!row.scaleUnit.empty()
          && sg::counters::detail::scaleRuleFor(row.scaleUnit) == nullptr)
      {
        ++unknownScale;
        std::printf("SWEEP: %s spells ScaleUnit '%s' with no rule\n",
                    directory.c_str(),
                    row.scaleUnit.c_str());
      }
    }
  }
  check(directories >= 40,
        "the sweep walks the directories of the vendored tree (FR-038)");
  check(rows >= 5000, "the sweep reads every metric row of the tree (FR-038)");
  check(reasonless == 0,
        "no refused row is left without its reason (FR-021, FR-038)");
  check(unknownScale == 0,
        "no ScaleUnit spelling of the tree lacks a rule (FR-018, FR-038)");
  std::printf(
      "sweep: %zu directories, %zu metric rows, %zu published, " "%zu "
                                                                 "refused\n",
      directories,
      rows,
      published,
      refused);
}

// The verdict of one fixture row under the full row list, where
// metric-definition entries sit beside the events. The source count
// passes the named entries by without counting them (FR-011).
auto wideListScenario() -> void
{
  const auto wide = metricRows(sg::test::fixtureTable(),
                               sg::test::fixtureRows(),
                               sg::test::fixtureHost());
  for (const auto& row : wide) {
    if (row.name == "sources") {
      check(row.published,
            "the source count scans metric entries without counting them "
            "(FR-011)");
    }
  }
}

auto main() -> int
{
  // The scripted leaves are what a folded value or a stated refusal reads,
  // so only those scenarios need the fixture provider. The sweep over the
  // vendored directories and the host-parameterised lookup answer from the
  // fixture rows and the host's own table wherever the process stands
  // (FR-036). Registration must precede the system's first use, which is
  // what `registerMetricHost` states; opening `machine` before it would
  // lock the fixture out and silently skip every scripted scenario.
  const bool scripted = sg::test::registerMetricHost();
  (void)sg::counters::System::local().object("machine");
  if (!scripted) {
    std::printf("SKIP metric lookup: the counters system had already opened "
                "when the fixture registered, so no scripted leaves exist to "
                "fold (FR-036)\n");
  }
  if (scripted) {
    valueScenario();
    nestedScenario();
    tableScenario();
    lookupScenario();
    refusalScenario();
    factScenario();
    chainScenario();
    wideListScenario();
  }
  hostScenario();
  verdictSweepScenario();
  std::printf("counters_metric_lookup_test PASS: the value rows against "
              "hand-composed expressions, the nested expansion, the cycle and "
              "duplicate-name rules, the scale factor, the miss and "
              "once-only load, and every refusal kind\n");
  return 0;
}
