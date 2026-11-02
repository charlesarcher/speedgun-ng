// ============================================================================
// Metric formulas of the vendored event table, read by name
// (specs/022-metric-formulas; FR-001, FR-002, FR-019, FR-021).
//
// A metric is one row of the architecture event table: a name, a formula
// over event names and other metric names, and the table's own group,
// scale-and-unit, description and threshold texts. This header publishes the
// row in the same shape a hand-composed expression has, so a metric folds,
// plans and reports exactly like an expression the caller wrote by hand.
//
// The text is parsed when the directory's event table is first read, which
// the catalog or a first lookup triggers; the lookup itself parses nothing
// and reads no counter. A row that will not publish stays visible as a
// refusal, in the catalog and in the error this lookup returns.
// ============================================================================

#ifndef SG_COUNTERS_METRICS_HPP
#define SG_COUNTERS_METRICS_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"

namespace sg::counters
{

/// Why a metric row of the event table will not publish. One per refusal
/// shape of the specification's refusal table.
enum class MetricRefusal : std::uint8_t
{
  MALFORMED,  ///< Text the formula grammar cannot read.
  UNSUPPORTED,  ///< An operator or construct the grammar excludes.
  CYCLE,  ///< A reference chain that returns to the metric.
  UNKNOWN_SCALE,  ///< A `ScaleUnit` text the scale table lacks.
  UNKNOWN_CONSTANT,  ///< A `#Name` constant this host has no reader for.
  NESTED,  ///< A conditional or predicate nested in an argument.
  DIMENSION,  ///< Terms that cannot compose into one expression.
  UNRESOLVED,  ///< A reference that names no leaf this host counts.
  COLLISION,  ///< A second row carrying a name the first row took.
};

/// The refusal kind as the catalog and a test print it, in UPPER SNAKE. The
/// text is static and never empty.
///
/// \pre Every value of `MetricRefusal` has a name in the table (FR-021).
/// \post none
[[nodiscard]] SPEEDGUN_NG_EXPORT auto metricRefusalName(
    MetricRefusal kind) noexcept -> std::string_view;

/// A metric row of the event table, published as a composition.
///
/// `expression` is the metric's spine with the table's scale factor
/// applied inside it, so the folded value is the value the table means
/// (FR-018). The formula's own dimension is checked when the row loads
/// (FR-010) and the tag here is the fold's carrier: a fold erases
/// dimensions before the point buffer (FR-016), and every leaf of the
/// spine is a leaf of the open system's catalog, so the same
/// compile-and-fold path a hand-written expression takes applies.
struct SPEEDGUN_NG_EXPORT MetricView
{
  Expression<Dim<0, 0>> expression;  ///< The table's formula, scaled.
  std::string_view group;  ///< The table's group text, verbatim.
  std::string_view unit;  ///< The table's scale-and-unit text.
  /// The table's `MetricThreshold` text, byte for byte, and empty where the
  /// row carries no such key. The formula grammar reads no threshold shape,
  /// so no part of the library evaluates this text or marks a row by it, and
  /// a caller prints it as the table spells it (specs/042-topdown FR-017,
  /// spec D-9, D-11).
  std::string_view threshold;
};

/// Resolves one metric by name against the event table of the CPU this
/// process runs on.
///
/// The name is the table's `MetricName`, case-sensitive. The first call
/// for a directory parses that directory's metric rows and every later
/// call reuses the result, so the call parses no text and reads no
/// counter; the view it returns holds its own spine, and its leaves are
/// catalog leaves of the open system.
///
/// \pre none
/// \post none
[[nodiscard]] SPEEDGUN_NG_EXPORT auto metricByName(std::string_view name)
    -> std::expected<MetricView, Error>;

/// How much of a core the cpus of this process cover
/// (specs/042-topdown FR-042, spec D-6).
///
/// A top-down table states its slots over a whole core. A row that names
/// `#core_wide` asks whether the cpus this process may run on cover every
/// core they touch, and the answer decides whether those slots mean what the
/// table says. The value states no slot width: the width is a literal of the
/// table and no host fact changes it.
enum class CoreCoverage : std::uint8_t
{
  WHOLE_CORE,  ///< Every sibling of every cpu of the set stands inside it.
  ONE_THREAD,  ///< One sibling of one cpu of the set stands outside it.
};

/// Answers how much of a core the cpus of this process cover, and answers
/// nothing where the host states no sibling lists
/// (specs/042-topdown FR-042, FR-043, spec D-6).
///
/// The answer comes from the cpu set this process holds and the sibling list
/// of every cpu that set holds. That is the one reader the `#core_wide` row
/// of a table reads through, so the query and a folded row answer one thing
/// and a caller that states the scope of a run asks here.
///
/// \pre none
/// \post Nothing stands settled before the first read of a metric table, and
///       one value stands for the life of the process afterwards, because a
///       conditional over the constant folds when the table resolves and the
///       resolved table is cached per directory (FR-042, spec D-6). A read of
///       this query is a read of the fact, so the first call settles the
///       value this process keeps and a second call answers it again: the
///       caller needs no reader of its own. The value names a whole core with
///       every sibling of it, or one thread of a core, and states no slot
///       width: the width is a literal of the table (FR-042). An absent
///       answer keeps the standing refusal of `#core_wide`, so a row that
///       needs the fact stays refused (FR-043).
/// \invariant A settled value never moves inside one process (FR-042, spec
///       D-6).
[[nodiscard]] SPEEDGUN_NG_EXPORT auto coreWideCoverage()
    -> std::optional<CoreCoverage>;

/// One metric row of the table this CPU selected, published or refused,
/// as the catalog prints it.
///
/// `expression` is not carried here: the catalog lists the row, its group
/// and unit text, its availability, and the refusal kind and reason where
/// the row has one. `reason` is empty for a published row; `refusal` names
/// the kind for a refused one.
struct SPEEDGUN_NG_EXPORT MetricListing
{
  /// The catalog object the row prints under: the object of the spine's
  /// first leaf, or the machine root while no leaf resolved (FR-020).
  std::string_view objectPath;
  std::string_view name;
  std::string_view group;
  std::string_view unit;
  std::string_view description;  ///< The table's own description text.
  /// The table's `MetricThreshold` text, byte for byte, and empty where the
  /// row carries no such key. A published row and a refused row carry their
  /// own text, and the line states no verdict on it (specs/042-topdown
  /// FR-017, spec D-9).
  std::string_view threshold;
  Availability avail = Availability::COUNTABLE;
  MetricRefusal refusal = MetricRefusal::MALFORMED;
  bool published = false;
  std::string_view reason;
};

/// Every metric row of the table this CPU selected, in table order: the
/// published rows and the refusals alike (FR-020, FR-021). An empty result
/// means this host selected no event table, which is a catalog fact and it
/// states no refusal of a single row.
///
/// \pre none
/// \post none
[[nodiscard]] SPEEDGUN_NG_EXPORT auto catalogMetrics()
    -> std::vector<MetricListing>;

}  // namespace sg::counters

#endif
