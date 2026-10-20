// ============================================================================
// TDD test for the metric formula grammar (specs/022-metric-formulas T013 to
// T019; FR-003 to FR-007).
//
// The parse pass is a pure function over a row's text, so the cases are
// driven as text in, tree or refusal out: no host, no counter, no clock
// (FR-039, FR-041). The literal-constant cases need the object tree the
// constants count, so they run over the fixture host, and the loader case
// shows the load continuing past a row it refuses. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

#include "detail/metric_fixture.hpp"

namespace
{

using sg::counters::MetricRefusal;
using sg::counters::detail::FormulaKind;
using sg::counters::detail::FormulaNode;
using sg::counters::detail::FormulaTree;
using sg::counters::detail::parseFormula;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "METRIC FORMULA TEST FAIL: %s\n", what);
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

[[nodiscard]] auto parsed(std::string_view text) -> FormulaTree
{
  auto tree = parseFormula(text);
  if (!tree.has_value()) {
    std::fprintf(stderr,
                 "METRIC FORMULA TEST FAIL: '%s' refused: %s\n",
                 std::string(text).c_str(),
                 tree.error().reason.c_str());
    std::exit(1);
  }
  return std::move(*tree);
}

[[nodiscard]] auto refusalOf(std::string_view text) -> MetricRefusal
{
  const auto tree = parseFormula(text);
  if (tree.has_value()) {
    std::fprintf(stderr,
                 "METRIC FORMULA TEST FAIL: '%s' parsed and had to refuse\n",
                 std::string(text).c_str());
    std::exit(1);
  }
  return tree.error().kind;
}

[[nodiscard]] auto reasonOf(std::string_view text) -> std::string
{
  const auto tree = parseFormula(text);
  if (tree.has_value()) {
    std::fprintf(stderr,
                 "METRIC FORMULA TEST FAIL: '%s' parsed and had to refuse\n",
                 std::string(text).c_str());
    std::exit(1);
  }
  return tree.error().reason;
}

[[nodiscard]] auto at(const FormulaTree& tree,
                      const int index) -> const FormulaNode&
{
  check(index >= 0, "a node index the tree holds");
  return tree.nodes[static_cast<std::size_t>(index)];
}

[[nodiscard]] auto countNodes(const FormulaTree& tree) -> std::size_t
{
  return tree.nodes.size();
}

// T013: one case per literal form of FR-003.
auto literalFormsScenario() -> void
{
  const auto half = parsed("0.5");
  check(half.root == 0, "a lone literal is the whole tree (FR-003)");
  check(at(half, half.root).kind == FormulaKind::CONSTANT
            && sameDouble(at(half, half.root).value, 0.5),
        "a decimal literal carries its value (FR-003)");

  const auto hex = parsed("0x10");
  check(at(hex, hex.root).kind == FormulaKind::CONSTANT
            && sameDouble(at(hex, hex.root).value, 16.0),
        "a hexadecimal literal reads as the count it spells (FR-003)");

  const auto exponent = parsed("1.5e-2");
  check(at(exponent, exponent.root).kind == FormulaKind::CONSTANT
            && sameDouble(at(exponent, exponent.root).value, 0.015),
        "an exponent literal reads as the count it spells (FR-003)");

  const auto negative = parsed("-fixture_cycles");
  check(at(negative, negative.root).kind == FormulaKind::MULTIPLY
            && sameDouble(at(negative, at(negative, negative.root).left).value,
                          -1.0)
            && at(negative, at(negative, negative.root).right).kind
                == FormulaKind::REFERENCE,
        "a unary minus lowers onto a factor of minus one (FR-003)");

  const auto dotted = parsed("cpu.clock_uclk");
  check(at(dotted, dotted.root).kind == FormulaKind::REFERENCE
            && at(dotted, dotted.root).event == "cpu.clock_uclk",
        "a dotted event name holds one token (FR-003)");

  const auto privileged = parsed("fixture_cycles:u");
  check(at(privileged, privileged.root).kind == FormulaKind::REFERENCE
            && at(privileged, privileged.root).event == "fixture_cycles"
            && at(privileged, privileged.root).privilege == "u",
        "a privilege suffix is parsed beside the reference and reaches no "
        "config (FR-003, D-01)");

  const auto constant = parsed("#SMT_on");
  check(at(constant, constant.root).kind == FormulaKind::HOST_CONSTANT
            && at(constant, constant.root).spelling == "SMT_on",
        "a hash-prefixed constant names the fact it reads (FR-006)");
}

// T014: the `unit@modifier@` forms, each with its escape.
auto atFormScenario() -> void
{
  const auto hyphen = parsed("cstate_pkg@c6\\-residency@");
  check(countNodes(hyphen) == 1, "an escaped hyphen holds one token (FR-003)");
  check(at(hyphen, hyphen.root).device == "cstate_pkg"
            && at(hyphen, hyphen.root).event == "c6-residency",
        "the device half and the escape-decoded event half separate (FR-003)");
  check(at(hyphen, hyphen.root).spelling == "cstate_pkg@c6\\-residency@",
        "the spelling as written travels with the node (D-08)");

  const auto escaped =
      parsed("cpu_core@DTLB_LOAD_MISSES.STLB_HIT\\,cmask\\=1@");
  check(countNodes(escaped) == 1,
        "an escaped comma and equals hold one token (FR-003)");
  check(at(escaped, escaped.root).device == "cpu_core"
            && at(escaped, escaped.root).event
                == "DTLB_LOAD_MISSES.STLB_HIT,cmask=1"
            && at(escaped, escaped.root).modifier.empty(),
        "the escaped config text stays inside the event name (FR-003)");

  const auto modifier = parsed("cpu_core@DTLB_LOAD_MISSES.STLB_HIT,cmask=1@");
  check(at(modifier, modifier.root).event == "DTLB_LOAD_MISSES.STLB_HIT"
            && at(modifier, modifier.root).modifier == "cmask=1",
        "an unescaped comma separates the modifier text (FR-003)");

  check(refusalOf("cpu_core@fixture_cycles\\,cmask\\=1")
            == MetricRefusal::MALFORMED,
        "an unterminated modifier refuses (FR-003)");
}

// T015: operators, precedence and parentheses.
auto operatorScenario() -> void
{
  const auto precedence = parsed("a + b * c");
  check(at(precedence, precedence.root).kind == FormulaKind::ADD
            && at(precedence, at(precedence, precedence.root).right).kind
                == FormulaKind::MULTIPLY,
        "multiplication binds tighter than addition (FR-003)");

  const auto parenthesised = parsed("(a + b) * c");
  check(
      at(parenthesised, parenthesised.root).kind == FormulaKind::MULTIPLY
          && at(parenthesised, at(parenthesised, parenthesised.root).left).kind
              == FormulaKind::ADD,
      "parentheses carry the addition under the multiplication (FR-003)");

  const auto quotient = parsed("a / (b / c)");
  check(at(quotient, quotient.root).kind == FormulaKind::DIVIDE
            && at(quotient, at(quotient, quotient.root).right).kind
                == FormulaKind::DIVIDE,
        "a nested parenthesised quotient holds (FR-003)");

  const auto difference = parsed("a - b");
  check(at(difference, difference.root).kind == FormulaKind::SUBTRACT,
        "a subtraction that can yield a negative term parses (FR-003)");

  const auto running = parsed("a + b - c");
  check(at(running, running.root).kind == FormulaKind::SUBTRACT
            && at(running, at(running, running.root).left).kind
                == FormulaKind::ADD,
        "addition and subtraction run left to left (FR-003)");
}

// T016: the five functions of FR-004.
auto functionScenario() -> void
{
  const auto minimum = parsed("min(a, b)");
  check(at(minimum, minimum.root).kind == FormulaKind::MINIMUM
            && at(minimum, at(minimum, minimum.root).left).kind
                == FormulaKind::REFERENCE
            && at(minimum, at(minimum, minimum.root).right).kind
                == FormulaKind::REFERENCE,
        "min takes two runtime arguments (FR-004)");

  const auto maximum = parsed("max(a, 1)");
  check(at(maximum, maximum.root).kind == FormulaKind::MAXIMUM
            && at(maximum, at(maximum, maximum.root).right).kind
                == FormulaKind::CONSTANT,
        "max takes one runtime and one constant argument (FR-004)");

  const auto three = parsed("min(a, b, c)");
  check(
      at(three, three.root).kind == FormulaKind::MINIMUM
          && at(three, at(three, three.root).left).kind == FormulaKind::MINIMUM
          && at(three, at(three, three.root).right).spelling == "c",
      "a min of three arguments folds pairwise, as 23 rows spell it (FR-004)");

  const auto ratio = parsed("d_ratio(x, y)");
  const int guard = at(ratio, at(ratio, ratio.root).right).right;
  check(
      at(ratio, ratio.root).kind == FormulaKind::DIVIDE
          && at(ratio, at(ratio, ratio.root).right).kind == FormulaKind::MAXIMUM
          && sameDouble(at(ratio, guard).value, 1.0),
      "d_ratio lowers to the quotient guarded by max(y, 1.0) (FR-004)");

  const auto event = parsed("has_event(a)");
  check(at(event, event.root).kind == FormulaKind::HAS_EVENT
            && at(event, event.root).spelling == "a",
        "has_event takes one reference (FR-004)");

  const auto source = parsed("source_count(a)");
  check(at(source, source.root).kind == FormulaKind::SOURCE_COUNT
            && at(source, source.root).spelling == "a",
        "source_count takes one reference (FR-004)");

  check(refusalOf("log(a)") == MetricRefusal::UNSUPPORTED,
        "a function outside the five refuses (FR-004)");
  check(refusalOf("min(a)") == MetricRefusal::MALFORMED,
        "a min of one argument refuses (FR-004)");
  check(refusalOf("has_event(a + b)") == MetricRefusal::NESTED,
        "a predicate of an expression refuses (FR-004, D-09)");
}

// T017: the conditional form of FR-005.
auto conditionalScenario() -> void
{
  const auto fact = parsed("a if #SMT_on else b");
  check(
      at(fact, fact.root).kind == FormulaKind::CONDITIONAL
          && at(fact, fact.root).third >= 0
          && at(fact, at(fact, fact.root).third).kind == FormulaKind::REFERENCE,
      "the else arm rides beside the then arm (FR-005)");

  const auto greater = parsed("a if b > 1 else c");
  check(
      at(greater, at(greater, greater.root).right).kind == FormulaKind::GREATER,
      "a greater-than test parses (FR-005)");

  const auto less = parsed("a if b < 1 else c");
  check(at(less, at(less, less.root).right).kind == FormulaKind::LESS,
        "a less-than test parses (FR-005)");

  const auto event = parsed("a if has_event(b) else c");
  check(at(event, at(event, event.root).right).kind == FormulaKind::HAS_EVENT,
        "a has_event test parses (FR-005)");

  const auto source = parsed("a if source_count(b) else c");
  check(at(source, at(source, source.root).right).kind
            == FormulaKind::SOURCE_COUNT,
        "a source_count test parses (FR-005)");

  check(refusalOf("a if #SMT_on") == MetricRefusal::MALFORMED,
        "a conditional without else refuses (FR-005)");

  check(refusalOf("a if (b if #SMT_on else c) else d")
            == MetricRefusal::UNSUPPORTED,
        "a conditional inside a conditional test refuses (FR-005, D-09)");
}

// T019: the refusal corpus of FR-007, each with its kind.
auto refusalCorpusScenario() -> void
{
  // The scanner stops at the operator before the walk can see a comparison
  // outside a conditional's test, which is where FR-005 draws its line.
  check(refusalOf("a > b") == MetricRefusal::MALFORMED
            && reasonOf("a > b").find("trailing text") != std::string::npos,
        "a top-level comparison refuses as trailing text (FR-005)");
  check(refusalOf("(a > b)") == MetricRefusal::MALFORMED
            && reasonOf("(a > b)").find("expected ')'") != std::string::npos,
        "a comparison inside parentheses refuses at the operator (FR-005)");
  check(refusalOf("(a + b") == MetricRefusal::MALFORMED,
        "an unbalanced parenthesis refuses (FR-007)");
  check(refusalOf("a +") == MetricRefusal::MALFORMED,
        "a trailing operator refuses (FR-007)");
  check(refusalOf("a * / b") == MetricRefusal::MALFORMED,
        "a doubled operator refuses (FR-007)");
  check(refusalOf("1.2.3") == MetricRefusal::MALFORMED,
        "a number the decimal scanner cannot settle refuses (FR-007)");
  check(refusalOf("") == MetricRefusal::MALFORMED,
        "an empty expression refuses (FR-007)");
  check(refusalOf("a ? b : c") == MetricRefusal::UNSUPPORTED,
        "a ternary, which the tables never spell, refuses (FR-007)");
  check(reasonOf("(a + b").find("')'") != std::string::npos,
        "the reason names what the parse expected (FR-007)");
  check(refusalOf("a b") == MetricRefusal::MALFORMED,
        "text after a complete expression refuses (FR-007)");
  check(reasonOf("a b").find("trailing text") != std::string::npos,
        "the trailing-text reason says what was left over (FR-021)");
  check(refusalOf("max(a b)") == MetricRefusal::MALFORMED,
        "arguments with no comma between them refuse (FR-004)");
  check(reasonOf("max(a b)").find("expected ',' or ')'") != std::string::npos,
        "the argument-list reason names the separator it wanted (FR-021)");
  check(refusalOf("max(a +)") == MetricRefusal::MALFORMED,
        "a malformed argument refuses the call (FR-004, FR-007)");
  check(refusalOf("has_event(a, b)") == MetricRefusal::MALFORMED,
        "a helper given two references refuses (FR-004)");
  check(reasonOf("has_event(a, b)").find("takes one argument")
            != std::string::npos,
        "the helper reason names the arity it holds (FR-021)");
  check(refusalOf("(a +") == MetricRefusal::MALFORMED,
        "a parenthesised term that fails refuses (FR-007)");
  check(refusalOf("-(a +") == MetricRefusal::MALFORMED,
        "a negation of a failing term refuses (FR-007)");
  check(refusalOf("#foo@bar") == MetricRefusal::MALFORMED,
        "a constant whose at-form never closes refuses (FR-006)");
  check(reasonOf("#foo@bar").find("unterminated") != std::string::npos,
        "the at-form reason says the marker never closed (FR-021)");
  check(refusalOf("#") == MetricRefusal::MALFORMED,
        "a constant marker with no name refuses (FR-006)");
  check(refusalOf("1e") == MetricRefusal::MALFORMED,
        "an exponent with no digits refuses (FR-006)");
  check(refusalOf("a if b > c else +") == MetricRefusal::MALFORMED,
        "a malformed else arm refuses the conditional (FR-005)");
  check(refusalOf("a if (b if 1 > 0 else c) > 1 else d")
            == MetricRefusal::UNSUPPORTED,
        "a conditional inside the test of a conditional refuses (FR-005)");
}

// T018: the literal constants, against the host the constants count.
auto constantScenario() -> void
{
  if (!sg::test::registerMetricHost()) {
    std::printf("SKIP literal constants: the counters system had already "
                "opened when the fixture registered, so no scripted object "
                "tree exists to count (FR-036)\n");
    return;
  }
  const auto host = sg::test::fixtureHost();
  const auto events = sg::test::fixtureEvents();
  for (const auto& name : {
           "SMT_on",
           "num_packages",
           "num_cores",
           "num_cpus_online",
       })
  {
    const auto rows = std::vector<sg::test::MetricRow> {
        sg::test::fixtureEvent(sg::test::kCycles),
        sg::test::fixtureMetric("row",
                                std::string("fixture_cycles / #") + name),
    };
    const auto table = sg::counters::detail::parseMetricRows(rows);
    const auto settled =
        sg::counters::detail::metricDefinition(table, events, host, "row");
    check(settled.has_value(),
          "a constant the object tree answers resolves the row (FR-006)");
  }
  for (const auto& name : {
           "has_pmem",
           "num_dies",
           "core_wide",
           "SYSTEM_TSC_FREQ",
       })
  {
    const auto rows = std::vector<sg::test::MetricRow> {
        sg::test::fixtureEvent(sg::test::kCycles),
        sg::test::fixtureMetric("row",
                                std::string("fixture_cycles / #") + name),
    };
    const auto table = sg::counters::detail::parseMetricRows(rows);
    const auto settled =
        sg::counters::detail::metricDefinition(table, events, host, "row");
    check(!settled.has_value(),
          "a constant with no source refuses the row (FR-006)");
    if (!settled.has_value()
        && settled.error().message.find(name) == std::string::npos)
    {
      std::fprintf(stderr,
                   "METRIC FORMULA TEST FAIL: the refusal of '#%s' does not "
                   "name it: %s\n",
                   name,
                   settled.error().message.c_str());
      std::exit(1);
    }
  }
}

// T024: the load keeps going over the rest of the table (FR-007).
auto loaderContinuesScenario() -> void
{
  if (!sg::test::registerMetricHost()) {
    std::printf("SKIP the continuing load: the counters system had already "
                "opened when the fixture registered (FR-036)\n");
    return;
  }
  const auto rows = std::vector<sg::test::MetricRow> {
      sg::test::fixtureEvent(sg::test::kCycles),
      sg::test::fixtureMetric("first", "a +"),
      sg::test::fixtureMetric("second", "min(a, has_event(b), c)"),
      sg::test::fixtureMetric("third", "log(a)"),
      sg::test::fixtureMetric("fourth", "fixture_cycles"),
  };
  const auto table = sg::counters::detail::parseMetricRows(rows);
  check(table.rows.size() == 4,
        "every row of the table keeps its place (FR-007)");
  check(!table.rows[0].parsed
            && table.rows[0].refusal == MetricRefusal::MALFORMED,
        "the first row carries its parse refusal (FR-007)");
  check(!table.rows[1].parsed && table.rows[1].refusal == MetricRefusal::NESTED,
        "the second row carries its own refusal, not the first's (FR-007)");
  check(!table.rows[2].parsed
            && table.rows[2].refusal == MetricRefusal::UNSUPPORTED,
        "the third row carries its own refusal (FR-007)");
  const auto settled = sg::counters::detail::metricDefinition(
      table, sg::test::fixtureEvents(), sg::test::fixtureHost(), "fourth");
  check(settled.has_value(),
        "a row after three refusals still resolves (FR-007)");
}

}  // namespace

auto main() -> int
{
  literalFormsScenario();
  atFormScenario();
  operatorScenario();
  functionScenario();
  conditionalScenario();
  refusalCorpusScenario();
  constantScenario();
  loaderContinuesScenario();
  std::printf("counters_metric_formula_test PASS: every literal form, the "
              "at-form with its escapes, precedence and parentheses, the "
              "five functions, the conditional, the refusal corpus and the "
              "literal constants over the scripted host\n");
  return 0;
}
