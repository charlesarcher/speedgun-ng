// ============================================================================
// The two pinned tables a metric row needs to publish
// (specs/021-metric-formulas D-05, D-06).
//
// The first turns a row's `ScaleUnit` text into the factor the fold carries.
// It is a table because an unlisted spelling must refuse the row: a numeric
// prefix scan would print a number no source supports (FR-018). The sweep of
// T056 reports every distinct spelling of the pinned tree and this table
// carries all of them.
//
// The second answers the `#`-prefixed literals. Each reader asks the open
// counters system for a fact it already publishes - the object tree by kind
// for the counts, a catalog entry's calibration frequency for the reference
// clock - so no reader opens a path or reads a clock of its own (FR-006).
// A constant with no reader here refuses the metric and names the constant.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_METRIC_TABLES_HPP
#define SG_COUNTERS_LINUX_PMU_METRIC_TABLES_HPP

#include <cstddef>
#include <string_view>

#include "../detail/pmu.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace sg::counters::detail
{

/// @brief One pinned `ScaleUnit` spelling and the factor it carries.
///
/// The text is the table's own spelling, kept verbatim so the sweep can
/// match it; the factor is what the fold multiplies the metric by. The
/// unit half of the text reaches the caller as the metric's unit text, so
/// the factor here carries no unit of its own (FR-018).
struct ScaleRule
{
  std::string_view spelling;
  double factor;
};

/// Every distinct `ScaleUnit` spelling of the pinned tree, in the order the
/// sweep of T056 reported them, with the row count each one carries.
inline constexpr std::array<ScaleRule, 25> kScaleRules {
    ScaleRule {"100%", 100.0},
    ScaleRule {"100%ops", 100.0},
    ScaleRule {"100%slots", 100.0},
    ScaleRule {"1GHz", 1.0},
    ScaleRule {"1MB/s", 1.0},
    ScaleRule {"1SMI#", 1.0},
    ScaleRule {"1core clocks", 1.0},
    ScaleRule {"1cycles / elision", 1.0},
    ScaleRule {"1cycles / transaction", 1.0},
    ScaleRule {"1e3per_1k_instr", 1e3},
    ScaleRule {"1ns", 1.0},
    ScaleRule {"1per_branch", 1.0},
    ScaleRule {"1per_instr", 1.0},
    ScaleRule {"1per_memclk", 1.0},
    ScaleRule {"1per_sec", 1.0},
    ScaleRule {"1slots", 1.0},
    ScaleRule {"3.0517578125e-5MiB", 3.0517578125e-5},
    ScaleRule {"3.2e-5MB/s", 3.2e-5},
    ScaleRule {"3e-5MiB", 3e-5},
    ScaleRule {"4Bytes", 4.0},
    ScaleRule {"6.103515625E-5MiB/sec", 6.103515625e-5},
    ScaleRule {"6.103515625e-5MiB", 6.103515625e-5},
    ScaleRule {"6.1e-5MiB", 6.1e-5},
    ScaleRule {"6.4e-5MB/s", 6.4e-5},
    ScaleRule {"6000000000ns", 6e9},
};

/// The rule for one `ScaleUnit` spelling, or null when the table lacks it.
///
/// \pre none
/// \post A null result names a spelling the metric must refuse (FR-018).
[[nodiscard]] inline auto scaleRuleFor(std::string_view spelling)
    -> const ScaleRule*
{
  for (const auto& rule : kScaleRules) {
    if (rule.spelling == spelling) {
      return &rule;
    }
  }
  return nullptr;
}

/// @brief One host fact a `#`-prefixed literal reads.
///
/// The reader asks the open system, never a path: an unavailable fact is a
/// false result, which is what makes the constant refuse (FR-006).
struct HostFact
{
  std::string_view name;
  bool (*read)(System& system, double& value);
};

/// Counts the objects one kind publishes. A kind the host publishes no
/// object for answers nothing, because the selection is an error rather
/// than an empty list.
///
/// \pre none
/// \post A true result leaves the object count in `value`.
[[nodiscard]] inline auto countObjects(System& system,
                                       std::string_view kind,
                                       double& value) -> bool
{
  const auto objects = system.objects(kind);
  if (!objects.has_value() || objects->empty()) {
    return false;
  }
  value = static_cast<double>(objects->size());
  return true;
}

/// The three count readers below answer the object count of the kind their
/// constant names; \pre none, and a true result leaves that count in
/// `value`.
[[nodiscard]] inline auto readThreadCount(System& system, double& value) -> bool
{
  return countObjects(system, "thread", value);
}

[[nodiscard]] inline auto readCoreCount(System& system, double& value) -> bool
{
  return countObjects(system, "core", value);
}

[[nodiscard]] inline auto readPackageCount(System& system,
                                           double& value) -> bool
{
  return countObjects(system, "package", value);
}

/// Reports whether simultaneous multithreading is on, from the thread and
/// core counts the tree publishes.
///
/// \pre none
/// \post A true result leaves 1.0 or 0.0 in `value`; a host that publishes
/// either count alone answers nothing.
[[nodiscard]] inline auto readSmtState(System& system, double& value) -> bool
{
  double threads = 0.0;
  double cores = 0.0;
  if (!readThreadCount(system, threads) || !readCoreCount(system, cores)) {
    return false;
  }
  value = threads > cores ? 1.0 : 0.0;
  return true;
}

/// The facts the literal constants read, by the name the table spells
/// them. Four constants of the pinned tree carry no entry and so refuse,
/// naming themselves: `#has_pmem`, `#num_dies` and `#core_wide` name a host
/// property this library publishes no object for, and `#SYSTEM_TSC_FREQ`
/// names a rate, which the time-stamp-counter entry publishes as a count
/// asserting no rate at all (D-06; specs/008-timestamp-counter FR-011).
inline constexpr std::array<HostFact, 4> kHostFacts {
    HostFact {"num_cpus_online", &readThreadCount},
    HostFact {"num_cores", &readCoreCount},
    HostFact {"num_packages", &readPackageCount},
    HostFact {"SMT_on", &readSmtState},
};

/// The fact one constant names, or null when no reader answers it.
///
/// \pre none
/// \post A null result names a constant the metric must refuse (FR-006).
[[nodiscard]] inline auto hostFactFor(std::string_view name) -> const HostFact*
{
  for (const auto& fact : kHostFacts) {
    if (fact.name == name) {
      return &fact;
    }
  }
  return nullptr;
}

}  // namespace sg::counters::detail

#endif
