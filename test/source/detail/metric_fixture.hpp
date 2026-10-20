#ifndef SG_TEST_DETAIL_METRIC_FIXTURE_HPP
#define SG_TEST_DETAIL_METRIC_FIXTURE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../../../source/counters/linux_pmu/metric_formula.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

// The tables and the host the metric tests share (specs/021-metric-formulas
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
[[nodiscard]] inline auto registerMetricHost() -> bool
{
  static const bool kAccepted = []
  {
    auto provider = std::make_unique<sg::counters::FakeProvider>();
    provider->addObject(
        kCoreObject, "pmu", "core counting device of the metric fixture");
    provider->addObject(
        kMemoryObject, "pmu", "uncore memory device of the metric fixture");
    provider->addObject(
        "metric-package-0", "package", "package of the metric fixture");
    provider->addObject(
        "metric-package-0/metric-core-0", "core", "core of the metric fixture");
    provider->addObject("metric-package-0/metric-core-0/metric-thread-0",
                        "thread",
                        "thread of the metric fixture");
    provider->addObject("metric-package-0/metric-core-0/metric-thread-1",
                        "thread",
                        "thread of the metric fixture");
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
        kMemoryObject, kDataReads, "bytes", "fixture memory reads");
    provider->setPoints(kCoreObject, kCycles, {0, 1000}, 1000);
    provider->setPoints(kCoreObject, kInstructions, {0, 2000}, 2000);
    provider->setPoints(kCoreObject, kRefCycles, {0, 500}, 500);
    provider->setPoints(kCoreObject, kL2Requests, {0, 10}, 10);
    provider->setPoints(kCoreObject, kTime, {0, 1000000}, 1000000);
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
