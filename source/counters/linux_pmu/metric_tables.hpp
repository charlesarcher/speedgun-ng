// ============================================================================
// The two pinned tables a metric row needs to publish
// (specs/022-metric-formulas D-05, D-06).
//
// The first turns a row's `ScaleUnit` text into the factor the fold carries.
// It is a table because an unlisted spelling must refuse the row: a numeric
// prefix scan would print a number no source supports (FR-018). The sweep of
// T056 reports every distinct spelling of the pinned tree and this table
// carries all of them.
//
// The second answers the `#`-prefixed literals. Each reader asks the open
// counters system for a fact it already publishes: the object tree by kind
// for the counts, and a catalog entry's calibration frequency for the
// reference clock (FR-006). Two readers go to the host beside the tree, for
// the sibling list of the cpu a call runs on, under the file root of the seam
// (specs/042-topdown FR-054, FR-042). A constant with
// no reader here refuses the metric and names the constant.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_METRIC_TABLES_HPP
#define SG_COUNTERS_LINUX_PMU_METRIC_TABLES_HPP

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include <sched.h>

#include "../../detail/proc_root.hpp"
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
/// The reader asks the open system for a fact its object tree publishes, and
/// the two host readers below read the sibling list of one cpu and the cpu
/// set of the process under the file root of the seam (specs/042-topdown
/// FR-054, FR-042). An unavailable fact is a false result, which is what
/// makes the constant refuse (FR-006).
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
  if (!objects.has_value()) {
    // A kind the tree carries nothing for answers an error, never an
    // empty list: the selection is the error (FR-003).
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

#ifdef __linux__

namespace metric_tables_detail
{

/// The first line of one file under the file root, and nothing when the file
/// stands unreadable or carries no line. The kernel ends each topology file
/// with a newline, and the line comes back without it.
///
/// \pre none
/// \post A answered text holds the characters before the first newline.
[[nodiscard]] inline auto firstLineOf(const std::filesystem::path& path)
    -> std::optional<std::string>
{
  std::ifstream file(path);
  auto text = std::string {};
  (void)std::getline(file, text);
  if (text.empty()) {
    return std::nullopt;
  }
  return text;
}

}  // namespace metric_tables_detail

/// @brief The sibling list of one cpu, read under the file root the seam
/// installs: `core_cpus_list` first, and `thread_siblings_list` when that
/// file answers nothing (specs/042-topdown FR-054, D-19).
///
/// The two files stand under `/sys`, which is the root the pre-check reads
/// too, so a test drives both answers through `installTestProcRoot`.
///
/// \pre none
/// \post The answered text is the first of the two files that stands
///       readable. Nothing answers a cpu whose two files both stand
///       unreadable, and a cpu number the host carries no directory for
///       reads as one of them.
[[nodiscard]] inline auto siblingListFor(const int cpu)
    -> std::optional<std::string>
{
  const std::string topology =
      "sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/";
  auto list = metric_tables_detail::firstLineOf(
      sg::detail::procRootPath(topology + "core_cpus_list"));
  if (list.has_value()) {
    return list;
  }
  return metric_tables_detail::firstLineOf(
      sg::detail::procRootPath(topology + "thread_siblings_list"));
}

/// @brief The SMT fact one cpu's sibling list answers, the fallback of the
/// object tree below (specs/042-topdown FR-054, D-19).
///
/// The cpu is the one `::sched_getcpu()` names. That call answers -1 where
/// the host reads no cpu number, and no directory carries the name `cpu-1`,
/// so the fallback answers nothing and no cpu is invented.
///
/// \pre none
/// \post A true result leaves 1.0 in `value` when the list names more than
///       one cpu, in the `a,b` or the `a-b` spelling, and 0.0 when it names
///       one id. A false result leaves `value` as it stood.
[[nodiscard]] inline auto smtStateFromSiblingList(double& value) -> bool
{
  const auto list = siblingListFor(::sched_getcpu());
  if (!list.has_value()) {
    return false;
  }
  const auto namesTwo = list->find(',') != std::string::npos
      || list->find('-') != std::string::npos;
  value = namesTwo ? 1.0 : 0.0;
  return true;
}

/// @brief The coverage a sibling list answers for a run whose counters come
/// from one thread (specs/042-topdown FR-042, D-6).
///
/// An affinity mask states a scheduling permission and says nothing about
/// what the run counted, and a coverage flag must describe the measurement.
/// A thread's sibling count is a topology fact the kernel publishes per cpu,
/// so the flag reads the sibling list of the cpu carrying the measurement: a
/// list naming one cpu leaves no sibling that the run could miss, and a list
/// naming more than one leaves a thread of that core whose counters the run
/// never opens.
///
/// \pre `list` is one cpu's sibling list as the kernel prints it, in the
///       `a,b` or the `a-b` spelling, without the newline.
/// \post A true result leaves 1.0 in `value` when the list names one cpu, and
///       0.0 when it names more than one. The flag states no slot width,
///       because the width is a literal of the table. A false result, which a
///       list naming no cpu id answers, leaves `value` as it stood.
[[nodiscard]] inline auto coreWideFromSiblingList(const std::string_view list,
                                                  double& value) -> bool
{
  if (list.find_first_of("0123456789") == std::string_view::npos) {
    return false;
  }
  const bool namesMoreThanOne = list.find(',') != std::string_view::npos
      || list.find('-') != std::string_view::npos;
  value = namesMoreThanOne ? 0.0 : 1.0;
  return true;
}

/// The `#core_wide` flag of the cpu this call runs on (FR-042, D-6). The
/// sibling list comes from `siblingListFor`, so `#SMT_on` and `#core_wide`
/// read one file of one cpu through one seam. That call answers -1 where the
/// host reads no cpu number, and no directory carries the name `cpu-1`, so
/// the read refuses there and the constant keeps its standing refusal.
///
/// \pre none
/// \post A true result leaves the flag in `value`. A cpu number that reads
///       nothing, an unreadable sibling list, and a list spelling no cpu id
///       each answer nothing and leave `value` as it stood.
[[nodiscard]] inline auto coreWideFlag(double& value) -> bool
{
  const auto list = siblingListFor(::sched_getcpu());
  if (!list.has_value()) {
    return false;
  }
  return coreWideFromSiblingList(*list, value);
}

#else

/// A host outside Linux carries no `/sys` tree, answers no cpu number and
/// publishes no sibling list, so the two host readers answer nothing there
/// and `#SMT_on` and `#core_wide` keep their refusals.
///
/// \pre none
/// \post A false result always, and `value` stays as it stood.
[[nodiscard]] inline auto smtStateFromSiblingList(double& value) -> bool
{
  static_cast<void>(value);
  return false;
}

/// The non-Linux twin of the coverage flag above, under the same contract.
///
/// \pre none
/// \post A false result always, and `value` stays as it stood.
[[nodiscard]] inline auto coreWideFlag(double& value) -> bool
{
  static_cast<void>(value);
  return false;
}

#endif

/// Reports whether simultaneous multithreading is on. The object tree
/// decides whenever it answers a core object, and a tree that answers no
/// core object reaches one cpu's sibling list under the file root
/// (FR-006, specs/042-topdown FR-054, D-19).
///
/// \pre none
/// \post A true result leaves 1.0 or 0.0 in `value`. A host that answers a
/// core object and no thread count, and a host whose two sibling files both
/// stand unreadable, answer nothing.
[[nodiscard]] inline auto readSmtState(System& system, double& value) -> bool
{
  double cores = 0.0;
  if (!readCoreCount(system, cores)) {
    return smtStateFromSiblingList(value);
  }
  double threads = 0.0;
  if (!readThreadCount(system, threads)) {
    return false;
  }
  value = threads > cores ? 1.0 : 0.0;
  return true;
}

/// Reports the coverage flag `#core_wide`: whether the cpu carrying the run
/// has a sibling the run leaves unread (FR-042, D-6). A cpu whose list names
/// one cpu answers 1, because that thread's counters span the core, and a cpu
/// whose list names more answers 0, because the run reads one thread of that
/// core. The flag settles at the first read of a metric table and stands for
/// the process after it, because a conditional over the constant folds when
/// the table resolves and the resolved table is shared. The flag states no
/// slot width: `4 * tma_info_core_core_clks` spells the width, and the table
/// keeps that literal (FR-042).
///
/// \pre none
/// \post A true result leaves 1.0 or 0.0 in `value`; an unreadable sibling
///       list, and a list spelling no cpu id, answer nothing, which keeps the
///       standing refusal of the constant.
[[nodiscard]] inline auto readCoreWide(System& /*system*/,
                                       double& value) -> bool
{
  return coreWideFlag(value);
}

/// The facts the literal constants read, by the name the table spells
/// them. Three constants of the pinned tree carry no entry and so refuse,
/// naming themselves: `#has_pmem` and `#num_dies` name a host property this
/// library publishes no object for, and `#SYSTEM_TSC_FREQ` names a rate,
/// which the time-stamp-counter entry publishes as a count asserting no rate
/// at all (D-06; specs/008-timestamp-counter FR-011, specs/042-topdown
/// FR-043). `#core_wide` reads the sibling list of the cpu a call runs on,
/// beside the object tree (FR-042).
inline constexpr std::array<HostFact, 5> kHostFacts {
    HostFact {"num_cpus_online", &readThreadCount},
    HostFact {"num_cores", &readCoreCount},
    HostFact {"num_packages", &readPackageCount},
    HostFact {"SMT_on", &readSmtState},
    HostFact {"core_wide", &readCoreWide},
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
