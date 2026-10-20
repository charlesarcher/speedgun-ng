// ============================================================================
// TDD test for the host-fact readers over an object tree that answers
// differently (specs/022-metric-formulas T058; FR-006).
//
// The lookup binary reads `#SMT_on` over the two-thread, one-core tree
// where the fact stands. This binary opens its own process and registers
// a tree that carries a thread but no core: the core count is then the
// missing half of the SMT fact, so the constant answers nothing and the
// row that asks for it refuses rather than folding a wrong answer.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "speedgun-ng/counters.hpp"

#include "detail/metric_fixture.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "METRIC TREE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto smtFactRefusalScenario() -> void
{
  if (!sg::test::registerMetricHost(sg::test::MetricTree::CORELESS)) {
    fail("the fixture provider could not register before the system opened");
  }
  const auto rows = std::vector<sg::test::MetricRow> {
      sg::test::fixtureEvent(sg::test::kCycles),
      sg::test::fixtureMetric(
          "smt_row", "fixture_cycles if #SMT_on else fixture_instructions"),
  };
  const auto table = sg::counters::detail::parseMetricRows(rows);
  const auto settled = sg::counters::detail::metricDefinition(
      table, sg::test::fixtureEvents(), sg::test::fixtureHost(), "smt_row");
  check(!settled.has_value()
            && settled.error().message.find("SMT_on") != std::string::npos,
        "a tree that carries threads but no cores answers no SMT fact, and "
        "the row that asks refuses (FR-006)");
}

}  // namespace

auto main() -> int
{
  smtFactRefusalScenario();
  std::printf("PASS\n");
  return 0;
}
