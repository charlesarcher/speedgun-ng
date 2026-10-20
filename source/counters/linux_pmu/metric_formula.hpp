// ============================================================================
// Metric-formula grammar of the vendored event table
// (specs/021-metric-formulas D-01, D-02, D-03, D-08, D-09, D-10).
//
// Parsing happens once per architecture directory, when that directory's
// event table is first read, and the trees live beside the table for the
// life of the process. Resolving a row against one host's catalog is a
// separate pass, so the by-name lookup copies a spine and parses no text
// (FR-019). A row that will not publish keeps the refusal kind and reason
// the catalog prints (FR-020, FR-021).
//
// Private seam: nothing here is exported, so the only way in from outside
// the archive is the public lookup in `speedgun-ng/counters_metrics.hpp`.
// A test reaches these declarations the way the PMU seam test reaches the
// other private headers of this directory.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_METRIC_FORMULA_HPP
#define SG_COUNTERS_LINUX_PMU_METRIC_FORMULA_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "../detail/pmu.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_metrics.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace sg::counters::detail
{

/// @brief The operator one formula node carries.
///
/// The kinds the erased spine already carries map onto it directly; the
/// rest are what only the host can answer, which is why they sit in the
/// tree and never reach a spine.
enum class FormulaKind : std::uint8_t
{
  REFERENCE,  ///< An event name or another metric's name.
  CONSTANT,  ///< A numeric literal.
  HOST_CONSTANT,  ///< A `#`-prefixed host fact.
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  MINIMUM,
  MAXIMUM,
  HAS_EVENT,  ///< A predicate over what the host can count.
  SOURCE_COUNT,  ///< A count of the directory's own rows.
  CONDITIONAL,  ///< `<value> if <test> else <value>`.
  GREATER,  ///< The `>` form of a conditional test.
  LESS,  ///< The `<` form of a conditional test.
};

/// @brief A parsed formula tree node, before host resolution.
///
/// The tree keeps the operators the load pass evaluates away (`has_event`,
/// `source_count`, the conditional arms, a literal constant) beside the
/// arithmetic the erased spine carries, so one walk lowers the spine and
/// another evaluates the parts only the host can answer.
struct FormulaNode
{
  FormulaKind kind = FormulaKind::REFERENCE;
  int left = -1;  ///< Operand index; -1 when the node has none.
  int right = -1;  ///< Second operand index; -1 for a unary node.
  int third = -1;  ///< The `else` arm of a conditional; else -1.
  std::string spelling;  ///< Reference or constant text as written.
  std::string event;  ///< The decoded event name a reference asks for.
  std::string device;  ///< Sub-event prefix of a `device@event@` form.
  std::string modifier;  ///< Modifier text of an at-style reference.
  std::string privilege;  ///< `:u`, `:k` or `:R` suffix of a reference.
  double value = 0.0;  ///< Literal value of a constant node.
};

/// @brief One parsed formula: the tree and the index of its root.
struct FormulaTree
{
  std::vector<FormulaNode> nodes;
  int root = -1;
};

/// @brief Why a metric row will not publish, with the text that says so.
struct MetricFailure
{
  MetricRefusal kind = MetricRefusal::MALFORMED;
  std::string reason;
  std::vector<std::string> suggestions;  ///< Near-miss names, when a leaf
                                         /// lookup produced them.
};

/// Parses one `MetricExpr` text into the formula tree.
///
/// \pre `text` is the table's own formula text.
/// \post A tree's root is its last node; a refusal names the offending
/// text and leaves no partially built tree behind.
[[nodiscard]] auto parseFormula(std::string_view text)
    -> std::expected<FormulaTree, MetricFailure>;

/// @brief One metric row of a directory, as the parse left it.
///
/// `scope` is the row's `Unit` text, which names the device class the
/// row's events belong to; `scaleUnit` is the row's `ScaleUnit` text, the
/// figure the catalog prints.
struct ParsedMetricRow
{
  std::string name;
  std::string group;
  std::string scope;
  std::string scaleUnit;
  std::string description;
  FormulaTree tree;
  bool parsed = false;
  MetricRefusal refusal = MetricRefusal::MALFORMED;
  std::string reason;
};

/// @brief A directory's metric rows, parsed, in table order.
///
/// The first row of a duplicated name is the one the index reaches and the
/// later rows are `COLLISION` refusals (FR-011).
struct ParsedMetricTable
{
  std::vector<ParsedMetricRow> rows;
  std::map<std::string, std::size_t> index;
};

/// Parses the metric rows of an already-parsed event table (T026: the
/// definitions live beside the table of the same directory).
///
/// \pre `table` is a directory's parsed table, metric rows included.
/// \post The result holds one entry per row carrying a metric name, in
/// table order, with the parse-stage refusals recorded.
[[nodiscard]] auto parseMetricRows(const std::vector<PmuTableEntry>& table)
    -> ParsedMetricTable;

/// The parsed metric rows of one embedded directory, parsed on the first
/// call and shared by every later call (FR-019).
///
/// \pre `directory` names an embedded table directory.
/// \post The table lives for the life of the process.
[[nodiscard]] auto parsedMetricTable(const std::string& directory)
    -> const ParsedMetricTable&;

/// @brief The host a metric row is resolved against.
///
/// `system` is the open counters system whose catalog carries the leaves,
/// and `directory` the event table the row's references resolve against and
/// whose `source_count` rows are counted, so a fixture resolves a synthetic
/// table against a synthetic host.
struct MetricHost
{
  System* system = nullptr;
  std::string directory;
};

/// The host this process runs on: the open system, the sysfs root the
/// library's own loaders read, and the directory the mapping file selected.
///
/// \pre none
/// \post An empty `directory` names a CPU the mapping file matched no row
/// for, which is a catalog fact and not a refusal of any row.
[[nodiscard]] auto metricHostCurrent() -> MetricHost;

/// @brief One metric row resolved against one host, published or refused.
struct MetricDefinition
{
  std::string name;
  std::string group;  ///< The table's `MetricGroup` text, verbatim.
  std::string unit;  ///< The table's `ScaleUnit` text, verbatim.
  std::string description;
  /// The object path the listing prints the row under: the catalog object
  /// of the spine's first leaf, or the machine root while no leaf resolved.
  std::string objectPath;
  ExprCore core;  ///< Published spine; empty for a refusal.
  /// The spine's dimension exponents, kept so an expansion of this metric
  /// composes without recomputing them.
  int time = 0;
  int events = 0;
  Availability avail = Availability::COUNTABLE;
  bool published = false;
  MetricRefusal refusal = MetricRefusal::MALFORMED;
  std::string reason;
  std::vector<std::string> suggestions;
};

/// @brief A directory's metric rows resolved against one host.
struct ResolvedMetricTable
{
  std::vector<MetricDefinition> rows;
  std::map<std::string, std::size_t> index;
};

/// The error a metric lookup returns for a name no row of the table
/// carries, with the near-miss metric names beside it (FR-016).
///
/// \pre none
/// \post The message names the queried text; the suggestions hold at most
/// five names, the closest first.
[[nodiscard]] auto noSuchMetric(
    std::string_view name,
    const std::vector<std::pair<std::string, std::string>>& candidates)
    -> Error;

/// Resolves every row of `table` against `host`, published and refused
/// alike, in table order (FR-021).
///
/// \pre `host.system` names an open system and `events` is its directory's
/// parsed table.
/// \post One entry per metric row, in table order; the walk resolves each
/// reference once, so a shared inner metric is expanded once per walk and
/// a cycle leaves every member refused.
[[nodiscard]] auto metricRows(const ParsedMetricTable& table,
                              const std::vector<PmuTableEntry>& events,
                              const MetricHost& host)
    -> std::vector<MetricDefinition>;

/// Resolves one metric by name against `host` (D-10: the single resolution
/// pass the public lookup and the catalog share).
///
/// \pre `host.system` names an open system.
/// \post A name the table lacks yields an error whose suggestions are the
/// near-miss metric names, at most five.
[[nodiscard]] auto metricDefinition(const ParsedMetricTable& table,
                                    const std::vector<PmuTableEntry>& events,
                                    const MetricHost& host,
                                    std::string_view name)
    -> std::expected<MetricDefinition, Error>;

/// The metric table of the directory `host` names, resolved on the first
/// call and shared by every later call (FR-019).
///
/// \pre `host.system` names an open system.
/// \post The table lives for the life of the process, so the text a view
/// points at stays valid.
[[nodiscard]] auto resolvedMetricTable(const MetricHost& host)
    -> const ResolvedMetricTable&;

/// The metric rows of the directory `host` names, shared with every later
/// call (FR-019), and none when `host` names no directory (FR-017).
///
/// \pre `host.system` names an open system.
/// \post An empty `directory` yields no rows; otherwise one entry per metric
/// row of the table, in table order.
[[nodiscard]] auto metricRowsOnHost(const MetricHost& host)
    -> const std::vector<MetricDefinition>&;

/// The catalog line of every metric row `host` lists (FR-021, FR-022).
///
/// \pre `host.system` names an open system.
/// \post No line when `host` names no directory; one line per metric row,
/// in table order, carrying the row's own published or refused answer.
[[nodiscard]] auto metricListingsOnHost(const MetricHost& host)
    -> std::vector<MetricListing>;

/// The definition of `name` inside an already-resolved table, the answer a
/// published row, a refused row and a missing name each give (FR-016, FR-021).
///
/// \pre `table` holds the rows of one directory, resolved against one host.
/// \post The first row that carried the name answers it; a refused row answers
/// with its own reason and suggestions, and a name the index lacks answers
/// with the near-miss names beside the queried text.
[[nodiscard]] auto metricDefinitionIn(const ResolvedMetricTable& table,
                                      std::string_view name)
    -> std::expected<MetricDefinition, Error>;

/// The definition of `name` in the directory `host` names (FR-017).
///
/// \pre `host.system` names an open system.
/// \post A host that names no directory answers with the reason that the CPU
/// matched no event-table directory; any other host answers through the table
/// of that directory.
[[nodiscard]] auto metricDefinitionOnHost(const MetricHost& host,
                                          std::string_view name)
    -> std::expected<MetricDefinition, Error>;

/// The public answer to naming a metric: the view of a published row, or the
/// error the lookup produced (FR-016).
///
/// \pre none
/// \post A value converts to a view carrying its spine, group and unit text;
/// an error passes through with its message and suggestions intact.
[[nodiscard]] auto metricViewResponse(
    const std::expected<MetricDefinition, Error>& def)
    -> std::expected<MetricView, Error>;

}  // namespace sg::counters::detail

#endif
