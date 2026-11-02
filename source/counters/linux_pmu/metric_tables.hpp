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
// the sibling list of one cpu and the cpu set of the process, both under the
// file root of the seam (specs/042-topdown FR-054, FR-042). A constant with
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

namespace metric_tables_detail
{

/// The width the set type `cpu_set_t` carries. An id at or above it names no
/// bit of any set, so it stands outside every set the reader compares
/// against.
///
/// \invariant `kCpuSetWidth` equals the `CPU_SETSIZE` of the set type.
inline constexpr std::size_t kCpuSetWidth = CPU_SETSIZE;

/// Whether one character is a digit of a cpu id.
///
/// \pre none
/// \post none
[[nodiscard]] inline auto isCpuDigit(const char character) -> bool
{
  return character >= '0' && character <= '9';
}

/// The id the digits at `at` spell, with the cursor left on the first
/// character past the run. The sum stops growing at `kCpuSetWidth`, so a
/// digit run longer than a cpu id settles on an id no set holds.
///
/// \pre none
/// \post A run of no digit leaves the id at 0 and leaves the cursor where it
///       stood.
[[nodiscard]] inline auto decimalAt(std::string_view text,
                                    std::size_t& at) -> std::size_t
{
  std::size_t value = 0;
  while (at < text.size() && isCpuDigit(text[at])) {
    if (value < kCpuSetWidth) {
      value = value * 10U + static_cast<std::size_t>(text[at] - '0');
    }
    ++at;
  }
  return value;
}

/// @brief Whether every id one sibling-list text names stands inside `mask`.
///
/// The kernel prints a cpu list as ids and `low-high` runs separated by
/// commas, and a run names every id between its ends. A dash whose second
/// end names no digit leaves the run at its first end.
///
/// \pre `mask` is the set the list was read for
/// \post Nothing answers a text that names no id, which keeps the standing
///       refusal of the constant (FR-042). A false answer means the set
///       leaves one named id out, and a true answer means it leaves none out.
[[nodiscard]] inline auto siblingsInside(
    std::string_view list, const cpu_set_t& mask) -> std::optional<bool>
{
  bool namesAnId = false;
  while (!list.empty()) {
    auto token = list;
    const auto comma = list.find(',');
    if (comma != std::string_view::npos) {
      token = list.substr(0, comma);
      list = list.substr(comma + 1);
    } else {
      list = {};
    }
    std::size_t at = 0;
    const std::size_t low = decimalAt(token, at);
    if (at == 0) {
      // A token naming no digit names no id, so it leaves the walk where it
      // stood. The refusal of a text that names no id at all waits below.
      continue;
    }
    namesAnId = true;
    std::size_t high = low;
    if (at < token.size() && token[at] == '-') {
      ++at;
      high = decimalAt(token, at);
    }
    for (std::size_t id = low; id <= high; ++id) {
      if (CPU_ISSET(id, &mask) == 0) {
        return false;
      }
    }
  }
  if (!namesAnId) {
    return std::nullopt;
  }
  return true;
}

/// The cpu set of this process, read into `mask`. A refused read leaves the
/// set empty, and an empty set is the answer nothing FR-042 asks for, so the
/// verdict of the call needs no second test here.
///
/// \pre none
/// \post `mask` holds the cpus the kernel answers, and no cpu at all when
///       the read refuses.
inline auto readProcessCpuSet(cpu_set_t& mask) -> void
{
  CPU_ZERO(&mask);
  (void)sched_getaffinity(0, sizeof(mask), &mask);
}

}  // namespace metric_tables_detail

/// @brief The `#core_wide` coverage flag over one cpu set: 1 when the set
/// holds every sibling of every cpu it holds, and 0 when one sibling stands
/// outside it (specs/042-topdown FR-042, D-6).
///
/// The read of the set stands in `metric_tables_detail::readProcessCpuSet`
/// and the verdict stands here, where a test drives each answer. Each list
/// comes from `siblingListFor`, so one reader and one seam serve `#SMT_on`
/// and `#core_wide` alike.
///
/// \pre none
/// \post A true result leaves 1.0 or 0.0 in `value`. The flag states no slot
///       width, because the width is a literal of the table. A false result,
///       which an empty set, an unreadable list and a list naming no id each
///       answer, leaves `value` as it stood.
[[nodiscard]] inline auto coreWideFromCpuSet(const cpu_set_t& mask,
                                             double& value) -> bool
{
  bool holdsOne = false;
  // The walk counts in `size_t`, which is the type
  // `/usr/include/bits/cpu-set.h` binds the operand of `__CPU_ISSET_S` to,
  // so an `int` cursor would be the conversion, not this one.
  for (std::size_t cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
    if (CPU_ISSET(cpu, &mask) == 0) {
      continue;
    }
    holdsOne = true;
    const auto list = siblingListFor(static_cast<int>(cpu));
    if (!list.has_value()) {
      return false;
    }
    const auto inside = metric_tables_detail::siblingsInside(*list, mask);
    if (!inside.has_value()) {
      return false;
    }
    if (!*inside) {
      value = 0.0;
      return true;
    }
  }
  if (!holdsOne) {
    return false;
  }
  value = 1.0;
  return true;
}

/// The `#core_wide` flag over the cpu set this process holds (FR-042).
///
/// \pre none
/// \post A true result leaves the flag in `value`; a set, a list, or a list
///       spelling that reads nothing answers nothing and leaves `value` as it
///       stood.
[[nodiscard]] inline auto coreWideFlag(double& value) -> bool
{
  cpu_set_t mask {};
  metric_tables_detail::readProcessCpuSet(mask);
  return coreWideFromCpuSet(mask, value);
}

#else

/// A host outside Linux carries no `/sys` tree, answers no cpu number and
/// publishes no cpu set, so the two host readers answer nothing there and
/// `#SMT_on` and `#core_wide` keep their refusals.
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

/// Reports the coverage flag `#core_wide`: whether the cpu set of this
/// process holds every sibling of every cpu it holds (FR-042, D-6). The flag
/// settles at the first read of a metric table and stands for the process
/// after it, because a conditional over the constant folds when the table
/// resolves and the resolved table is shared. The flag states no slot width:
/// `4 * tma_info_core_core_clks` spells the width, and the table keeps that
/// literal (FR-042).
///
/// \pre none
/// \post A true result leaves 1.0 or 0.0 in `value`; a set, a list, or a list
///       spelling that reads nothing answers nothing, which keeps the
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
/// FR-043). `#core_wide` reads the cpu set of the process beside the object
/// tree (FR-042).
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
