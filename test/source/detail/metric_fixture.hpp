#ifndef SG_TEST_DETAIL_METRIC_FIXTURE_HPP
#define SG_TEST_DETAIL_METRIC_FIXTURE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../../../source/counters/linux_pmu/metric_formula.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

// The tables and the host the metric tests share (specs/022-metric-formulas
// T007). The event names are the fixture's own, so a host that publishes a
// real PMU catalog answers none of them and every resolution reaches the
// scripted leaves (FR-039). The scripted leaves live on two objects whose
// paths carry the tokens the scope rule reads: `cpu` for the core class,
// `uncore_` plus a class for the memory controller. The package, core and
// thread objects are the tree the literal constants count.
namespace sg::test
{

inline constexpr std::string_view kCycles {"fixture_cycles"};
inline constexpr std::string_view kInstructions {"fixture_instructions"};
inline constexpr std::string_view kRefCycles {"fixture_ref_cycles"};
inline constexpr std::string_view kL2Requests {"fixture_l2_requests"};
inline constexpr std::string_view kTime {"fixture_time"};
inline constexpr std::string_view kDataReads {"fixture_data_reads"};

// The scope rule of the event table reaches a core device by its kernel
// name alone, so the core path is that name; the memory instance number is
// one no host publishes, so the fixture cannot collide with a real catalog
// path.
inline constexpr std::string_view kCoreObject {"cpu_core"};
inline constexpr std::string_view kMemoryObject {"uncore_imc_77"};

using MetricRow = sg::counters::detail::PmuTableEntry;
using MetricTable = sg::counters::detail::ParsedMetricTable;
using MetricHost = sg::counters::detail::MetricHost;
using MetricDefinition = sg::counters::detail::MetricDefinition;

// The fold's scripted steps: the deltas every value check divides.
inline constexpr double kCyclesDelta = 1000.0;
inline constexpr double kInstructionsDelta = 2000.0;
inline constexpr double kRefCyclesDelta = 500.0;
inline constexpr double kL2Delta = 10.0;
inline constexpr double kTimeDelta = 1000000.0;
inline constexpr double kDataReadsDelta = 4096.0;

// One event row of a synthetic directory. `scope` is the row's `Unit` text.
[[nodiscard]] inline auto fixtureEvent(std::string_view name,
                                       std::string_view scope = {}) -> MetricRow
{
  MetricRow entry;
  entry.name = std::string(name);
  entry.unit = std::string(scope);
  entry.description = "fixture event " + std::string(name);
  return entry;
}

// One metric-definition row of a synthetic directory: the four texts the
// loader reads, beside the scope the reference resolution places by.
[[nodiscard]] inline auto fixtureMetric(std::string_view name,
                                        std::string_view expr,
                                        std::string_view scaleUnit = {},
                                        std::string_view scope = {},
                                        std::string_view group = "fixture")
    -> MetricRow
{
  MetricRow entry;
  entry.name = std::string(name);
  entry.metricName = std::string(name);
  entry.metricExpr = std::string(expr);
  entry.scaleUnit = std::string(scaleUnit);
  entry.group = std::string(group);
  entry.unit = std::string(scope);
  entry.description = "fixture metric " + std::string(name);
  return entry;
}

// The rows every metric test shares: the cycle pair, the duplicated-name
// pair, the conditional pair, the value rows and one row per refusal.
[[nodiscard]] inline auto fixtureRows() -> std::vector<MetricRow>
{
  return {
      fixtureEvent(kCycles),
      fixtureEvent(kInstructions),
      fixtureEvent(kRefCycles),
      fixtureEvent(kL2Requests),
      fixtureEvent(kTime),
      fixtureEvent(kDataReads, "iMC"),
      // The value rows (FR-015).
      fixtureMetric("cpi", "fixture_cycles / fixture_instructions"),
      fixtureMetric("l2_mpi",
                    "fixture_l2_requests / fixture_instructions",
                    "1e3per_1k_instr"),
      // A metric whose reference is another metric (FR-008).
      fixtureMetric("tma_info_thread_clks", "fixture_cycles * 2"),
      fixtureMetric("tma_info_thread_ipc",
                    "fixture_instructions / tma_info_thread_clks"),
      // The scale case (FR-018).
      fixtureMetric("percent", "fixture_cycles / fixture_ref_cycles", "100%"),
      // The cycle pair (FR-009).
      fixtureMetric("loop_a", "loop_b + 1"),
      fixtureMetric("loop_b", "loop_a + 1"),
      // The duplicated-name pair: the first row answers (FR-011).
      fixtureMetric("dupe", "fixture_cycles"),
      fixtureMetric("dupe", "fixture_instructions"),
      // The conditional pair, one row on each side of a host fact (FR-005).
      fixtureMetric("smt_row",
                    "fixture_cycles if #SMT_on else fixture_instructions"),
      fixtureMetric("core_row", "fixture_cycles / #num_cores"),
      // Conditional tests the load-time evaluator folds: every operator of
      // the fact grammar, a subtree whose constant names no reader, and a
      // condition that names one event directly. Both branches are
      // literals, so the fold needs no counted leaf and the evaluator runs
      // even where no pmu object is published (FR-005, FR-006).
      fixtureMetric("fact_mul", "1 if #num_cores * 2 else 2"),
      fixtureMetric("fact_div", "1 if #num_cpus_online / 2 else 2"),
      fixtureMetric("fact_sub", "1 if #num_cores - 1 else 2"),
      fixtureMetric("fact_add", "1 if #num_cores + #num_packages else 2"),
      fixtureMetric("fact_min", "1 if min(#num_cores, #num_packages) else 2"),
      fixtureMetric("fact_max", "1 if max(#num_cores, #num_packages) else 2"),
      fixtureMetric("fact_left_bad", "1 if #has_pmem + 1 else 2"),
      fixtureMetric("fact_right_bad", "1 if 1 + #has_pmem else 2"),
      fixtureMetric("test_greater", "1 if #num_cores > 1 else 2"),
      fixtureMetric("test_less", "1 if #num_cores < 999999 else 2"),
      fixtureMetric("test_event", "1 if fixture_cycles else 2"),
      fixtureMetric("test_bad", "1 if #has_pmem else 2"),
      // A comparison outside a conditional strands text the root refuses
      // malformed (FR-002, FR-005), and a test whose nested parse fails
      // refuses malformed at the propagation edge of the comparison.
      fixtureMetric("bare_cmp", "fixture_cycles > fixture_ref_cycles"),
      fixtureMetric("bad_test_close", "1 if ( < 1) else 2"),
      fixtureMetric("bad_test_star", "1 if 1 < * 2 else 2"),
      // A helper that settles to one scales its operand spine (FR-004), a
      // `source_count` test settles on the table's own rows (FR-004), a
      // settled test whose right constant this host lacks refuses (FR-006),
      // and a product of two leaves folds on both operands (FR-010).
      fixtureMetric("helper_operand",
                    "fixture_cycles * has_event(fixture_instructions)"),
      fixtureMetric("test_source",
                    "1 if source_count(fixture_instructions) else 2"),
      fixtureMetric("test_missing_right", "1 if #num_cores < #has_pmem else 2"),
      fixtureMetric("prod_row", "fixture_cycles * fixture_instructions"),
      // A conditional seated in a parenthesized arm walks through the
      // spine (FR-005), and a guarded division with a counted left
      // operand keeps the left dimension (FR-008).
      fixtureMetric("cond_paren",
                    "(1 if #num_cores > 0 else 2) * fixture_cycles"),
      fixtureMetric("guard_div",
                    "fixture_cycles / (fixture_cycles / fixture_instructions)"),
      // A hexadecimal literal loads like its decimal spelling (FR-003),
      // a character the grammar does not seat refuses, and a two-argument
      // helper refuses malformed (FR-002).
      fixtureMetric("hex_row", "fixture_cycles * 0x2"),
      // An escaped name in a bare reference decodes to the row it names
      // (FR-003).
      fixtureMetric("esc_ref", "fixture\\_cycles"),
      fixtureMetric("unterminated", "[open_cycles + 1"),
      fixtureMetric("bad_arity",
                    "has_event(fixture_cycles, fixture_instructions)"),
      // The scanner's own edges: a dot or a dash joins a reference's
      // characters, an escape at the text's end and a colon without a
      // privilege word after it strand the scan, and an identifier may
      // open on an underscore (FR-003).
      fixtureMetric("dotted_ref", "fixture.cycles"),
      fixtureMetric("dashed_ref", "fixture-cycles"),
      fixtureMetric("dangling", "fixture_cycles + 1\\"),
      fixtureMetric("colon_tail", "fixture_cycles:"),
      fixtureMetric("colon_slash", "fixture_cycles:/x"),
      fixtureMetric("underscore_ref", "_hidden + 1"),
      // The number scanner's edges: an uppercase hexadecimal prefix, a
      // hexadecimal digit run that hands the next character back, and an
      // exponent carrying its own sign in either spelling of the letter
      // (FR-003).
      fixtureMetric("hex_upper", "fixture_cycles * 0X2"),
      fixtureMetric("hex_mid", "fixture_cycles * 0xF+1"),
      fixtureMetric("exp_pos", "fixture_cycles * 1e+2"),
      fixtureMetric("exp_neg", "fixture_cycles * 1e-2"),
      fixtureMetric("exp_upper", "fixture_cycles * 1E2"),
      fixtureMetric("exp_sign_upper", "fixture_cycles * 1E+2"),
      // The conditional rides an operand's every seat of a pairing and
      // nests in its own else-branch (FR-005, FR-010).
      fixtureMetric(
          "both_cond",
          "(1 if #num_cores < 0 else 2) * (3 if #num_cores > 0 else " "1) * " "fixture_" "cycles"),
      fixtureMetric(
          "third_cond",
          "(1 if #num_cores < 0 else (4 if #num_cores > 0 else 1)) * " "fixture" "_cycle" "s"),
      // A helper call closes with nothing between its parentheses
      // (FR-002), and a pairing's literal may sit on the left (FR-004).
      fixtureMetric("empty_call", "has_event()"),
      fixtureMetric("min_left", "min(2, fixture_cycles)"),
      // The seconds leaf: a duration unit of the closed set, and a time
      // dimension to pair against the cycle count (FR-006, FR-010).
      fixtureMetric("seconds_row", "fixture_seconds"),
      fixtureMetric("time_mix", "fixture_seconds + fixture_cycles"),
      // A leaf the host refuses to count answers `has_event` false
      // (FR-011, FR-012).
      fixtureMetric("event_blocked",
                    "0 if has_event(fixture_blocked) else fixture_cycles"),
      // A device token seats itself before the at-reference (FR-003).
      fixtureMetric("dev_core", "core@fixture_cycles@"),
      fixtureMetric("dev_cpu", "cpu@fixture_cycles@"),
      // A scale the table spells in the leaf's own unit rides the row
      // (FR-003, FR-020).
      fixtureMetric("scaled_row", "fixture_cycles", "4Bytes"),
      // The parser's own refusals: an escape stranded at the end of
      // a reference, a conditional standing inside the test of another
      // conditional on either side of its comparison, and a helper or a
      // predicate riding inside a guard's arguments (FR-003, FR-005).
      fixtureMetric("trailing_escape", "fixture_cycles\\"),
      fixtureMetric("test_cond", "5 if (1 if #num_cores > 0 else 2) else 3"),
      fixtureMetric("guard_left_cmp",
                    "5 if (1 if #num_cores > 0 else 2) < 1 else 7"),
      fixtureMetric("guard_right_cmp",
                    "5 if #num_cores < (1 if #num_cores > 0 else 2) " "else 7"),
      // A conditional buried inside a sum operand of the comparison, on
      // either seat (FR-005).
      fixtureMetric("guard_left_deep",
                    "5 if (1 if #num_cores > 0 else 2) + 1 < 1 else 7"),
      fixtureMetric("guard_right_deep",
                    "5 if 1 + (1 if #num_cores > 0 else 2) < 1 else 7"),
      fixtureMetric("nested_arg", "min(has_event(fixture_cycles), 1)"),
      fixtureMetric("nest_left", "min(has_event(fixture_cycles) + 1, 2)"),
      fixtureMetric("nest_right",
                    "min(fixture_cycles, max(1, has_event(fixture_cycles)))"),
      fixtureMetric(
          "nest_add_right",
          "min(fixture_cycles + has_event(fixture_instructions), " "1)"),
      // A helper settled outside a test walks as its literal on both
      // answers, and a guard's literal may sit on either side (FR-004).
      fixtureMetric("he_mul_true",
                    "has_event(fixture_instructions) * fixture_cycles"),
      fixtureMetric("he_mul_false",
                    "has_event(fixture_no_such_event) * fixture_cycles"),
      fixtureMetric("guard_right_dim", "max(fixture_cycles, 1)"),
      fixtureMetric("guard_left_dim", "max(2, fixture_cycles)"),
      // A digit hands the dash back mid-token; a plus rides the same test
      // after a signed exponent's digits; a squared duration refuses into a
      // plain one; a helper hides under two guards (FR-003, FR-005, FR-010).
      fixtureMetric("tight_dash", "fixture_cycles * 1-2"),
      fixtureMetric("plus_mid", "fixture_cycles * 2e3+1"),
      fixtureMetric("time_stack",
                    "fixture_seconds * fixture_seconds + fixture_seconds"),
      fixtureMetric(
          "nest_deep",
          "min(fixture_cycles + max(1, has_event(fixture_cycles)), " "2)"),
      // Conditions standing in the test without a comparison, inside an
      // addition on either side (FR-005).
      fixtureMetric("test_add_plain",
                    "fixture_cycles + 1 if #num_cores > 0 else 0"),
      fixtureMetric(
          "test_guard_left",
          "(1 if #num_cores > 0 else 2) + 1 if #num_cores > 0 " "else 0"),
      fixtureMetric(
          "test_guard_right",
          "1 + (1 if #num_cores > 0 else 2) if #num_cores > 0 " "else 0"),
      fixtureMetric("blocked_pair", "fixture_blocked + fixture_cycles"),
      // The helpers, each on the side this host answers (FR-004).
      fixtureMetric(
          "present",
          "fixture_cycles if has_event(fixture_instructions) " "else " "fixture" "_ref_" "cycles"),
      fixtureMetric(
          "absent",
          "fixture_cycles if has_event(fixture_no_such_event) " "else " "fixtur" "e_ref_" "cycles"),
      fixtureMetric("sources",
                    "fixture_cycles / source_count(fixture_instructions)"),
      fixtureMetric(
          "floors",
          "min(fixture_cycles, fixture_instructions, " "fixture_ref_cycles)"),
      fixtureMetric("ratio", "d_ratio(fixture_l2_requests, fixture_cycles)"),
      // The scope case: a row of the memory class (FR-013, FR-014).
      fixtureMetric("memory", "fixture_data_reads * 2", {}, "iMC"),
      // The duration case: a row of the memory class whose time leaf names
      // its own device (FR-010, FR-013).
      fixtureMetric("bandwidth",
                    "fixture_data_reads / cpu_core@fixture_time@",
                    {},
                    "iMC"),
      fixtureMetric("bandwidth", "fixture_data_reads / fixture_time"),
      fixtureMetric("bad_scale", "fixture_cycles", "1furlong"),
      fixtureMetric("bad_leaf", "fixture_no_such_event / fixture_instructions"),
      fixtureMetric("bad_add", "fixture_cycles + cpi"),
      fixtureMetric("bad_pmem", "fixture_cycles / #has_pmem"),
      fixtureMetric("bad_divide", "fixture_cycles / 0"),
      // The separator comma stands unescaped, as the tables spell it; the
      // `=` of the modifier does not.
      fixtureMetric("bad_modifier", "cpu_core@fixture_cycles,cmask\\=1@"),
  };
}

// Registers the scripted host. Registration precedes the first catalog use,
// which is how every test of this suite opens the tree (FR-036); a system
// that opened before the fixture registered answers false, and the caller
// names that reason and skips.
// The object tree the fixture provider publishes. The default is the
// simultaneous-multithreading tree: two threads on one core, so a host
// fact reads SMT on. The other shapes let another binary read the same
// facts off a tree that answers differently: equal thread and core
// counts, or a tree that carries threads but no cores.
enum class MetricTree
{
  SMT_ON,
  SMT_OFF,
  CORELESS,
  THREADLESS,
};

[[nodiscard]] inline auto registerMetricHost(
    const MetricTree tree = MetricTree::SMT_ON) -> bool
{
  static const bool kAccepted = [&tree]
  {
    auto provider = std::make_unique<sg::counters::FakeProvider>();
    provider->addObject(
        kCoreObject, "pmu", "core counting device of the metric fixture");
    provider->addObject(
        kMemoryObject, "pmu", "uncore memory device of the metric fixture");
    if (tree != MetricTree::CORELESS) {
      provider->addObject(
          "metric-package-0", "package", "package of the metric fixture");
      provider->addObject("metric-package-0/metric-core-0",
                          "core",
                          "core of the metric fixture");
    }
    if (tree != MetricTree::THREADLESS) {
      provider->addObject("metric-package-0/metric-core-0/metric-thread-0",
                          "thread",
                          "thread of the metric fixture");
    }
    if (tree == MetricTree::SMT_ON) {
      provider->addObject("metric-package-0/metric-core-0/metric-thread-1",
                          "thread",
                          "thread of the metric fixture");
    }
    provider->addCounter(kCoreObject, kCycles, "none", "fixture cycles");
    provider->addCounter(
        kCoreObject, kInstructions, "none", "fixture instructions");
    provider->addCounter(
        kCoreObject, kRefCycles, "none", "fixture reference cycles");
    provider->addCounter(
        kCoreObject, kL2Requests, "none", "fixture l2 requests");
    provider->addCounter(
        kCoreObject, kTime, "nanoseconds", "fixture time in nanoseconds");
    provider->addCounter(
        kCoreObject, "fixture_seconds", "seconds", "fixture seconds");
    provider->addCounter(kCoreObject,
                         "fixture_blocked",
                         "none",
                         "fixture leaf the host will not count",
                         sg::counters::Availability::PERMISSION_BLOCKED);
    provider->addCounter(
        kMemoryObject, kDataReads, "bytes", "fixture memory reads");
    provider->setPoints(kCoreObject, kCycles, {0, 1000}, 1000);
    provider->setPoints(kCoreObject, kInstructions, {0, 2000}, 2000);
    provider->setPoints(kCoreObject, kRefCycles, {0, 500}, 500);
    provider->setPoints(kCoreObject, kL2Requests, {0, 10}, 10);
    provider->setPoints(kCoreObject, kTime, {0, 1000000}, 1000000);
    provider->setPoints(kCoreObject, "fixture_seconds", {0, 250}, 250);
    provider->setPoints(kMemoryObject, kDataReads, {0, 4096}, 4096);
    const auto settled =
        sg::counters::System::local().registerProvider(std::move(provider));
    return settled.has_value();
  }();
  return kAccepted;
}

// The host a fixture table resolves against. The directory names no event
// table, because the resolution is handed the rows and reads no path
// (FR-002, FR-019).
[[nodiscard]] inline auto fixtureHost() -> MetricHost
{
  MetricHost host;
  host.system = &sg::counters::System::local();
  host.directory = "fixture/021/";
  return host;
}

// The identity of a CPU the mapping file names no directory for: a host
// with no metric table at all (FR-017).
[[nodiscard]] inline auto noMetricHost() -> MetricHost
{
  MetricHost host;
  host.system = &sg::counters::System::local();
  return host;
}

// The rows of a fixture table, event rows and metric rows apart (FR-002).
[[nodiscard]] inline auto fixtureEvents() -> std::vector<MetricRow>
{
  std::vector<MetricRow> events;
  for (const auto& row : fixtureRows()) {
    if (row.metricName.empty()) {
      events.push_back(row);
    }
  }
  return events;
}

// The parsed fixture table: the same parse pass the loader runs over a
// directory's bytes (FR-001).
[[nodiscard]] inline auto fixtureTable() -> MetricTable
{
  return sg::counters::detail::parseMetricRows(fixtureRows());
}

}  // namespace sg::test

#endif  // SG_TEST_DETAIL_METRIC_FIXTURE_HPP
