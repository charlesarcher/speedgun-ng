#ifndef SG_HARNESS_DETAIL_PASS_PLAN_HPP
#define SG_HARNESS_DETAIL_PASS_PLAN_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

/**
 * @file pass_plan.hpp
 * @brief The multi-pass collection decisions of specs/024: the method a run
 * settles to, the pass plan, the merged result and the pass spread. Every
 * one is a pure decision over published facts. Nothing here is public API.
 */

namespace sg
{

// specs/024 E-01, FR-023: the closed vocabulary of collection strategies.
// The definition stands in `speedgun-ng/benchmark.hpp` beside the
// `metricMethod` selector; this opaque declaration carries the underlying
// type `contracts/metric-method-selector.md` pins, so this seam names the
// enum before the selector lands and redeclares it compatibly after.
enum class MetricMethod : std::uint8_t;

}  // namespace sg

namespace sg::detail
{

// specs/024 E-03 rule 2: the PMU leaves one column asks of one counter
// source. `source` is the index of that source in the limit list given to
// `planPasses`, and `addresses` holds the address of every PMU leaf of the
// column that lives on that source, in the spelling
// `counters::detail::LeafCore::address` carries. A leaf two columns share
// stands in both lists: the pass that holds both counts it once (research
// D-05).
struct ColumnLeaves
{
  std::size_t source = 0;
  std::vector<std::string> addresses;
};

// specs/024 E-03: the planner's output, what each pass measures. Pass
// indices run from 0, pass 0 is the reference pass (I-4), and the passes
// stand in the order of their first column's request position (I-2).
struct PassPlan
{
  // Index k names the column indices pass k measures, in request order.
  std::vector<std::vector<std::size_t>> columns;
  // True for the pass that holds one over-limit column alone (FR-003).
  std::vector<bool> exclusive;
  // The pass that measures each column, indexed by column index. It feeds
  // the `MetricValue::pass` tag (E-08) and the merge (E-05).
  std::vector<std::size_t> measuringPass;
};

/// @brief specs/024 E-01, FR-024, FR-025: the concrete method one run
/// settles to, over the request that won the priority of research D-09,
/// whether any needed source publishes a programmable-counter limit, and how
/// many metric columns the run carries.
///
/// `requested` is that winner already resolved: the `--metric-method` word
/// when the command line carried one, else the family's own request, else
/// `MetricMethod::AUTO` (FR-025). The caller resolves the priority; one call
/// settles one run, before its first window opens (FR-007).
///
/// \pre no precondition: every combination of the three inputs is a row of
///      the resolution table of data-model E-01
/// \post the answer is `MetricMethod::AUTO` when `metricColumns` is zero, and
///       `MetricMethod::MULTIPLEX` when `anyLimit` is false; otherwise it is
///       the requested method, and `MetricMethod::PASSES` under a request of
///       `MetricMethod::AUTO` whether or not the columns need a split
///       (FR-017, I-6)
[[nodiscard]] auto settleMethod(MetricMethod requested,
                                bool anyLimit,
                                std::size_t metricColumns) noexcept
    -> MetricMethod;

/// @brief specs/024 E-03, FR-001, FR-004: the pass plan of one run's columns,
/// by first fit in request order (D-1, research D-05).
///
/// `columnLeaves` holds one entry per column in request order - the metric
/// columns in the order `addMetric` attached them, then the user-counter
/// columns in the order they opened - each holding that column's leaves
/// grouped by source. `sourceLimits` holds one entry per counter source: the
/// programmable-counter count that source publishes, or nothing when it
/// cannot answer (E-02, FR-005).
///
/// \pre the `source` index of every `ColumnLeaves` entry stands within
///      `sourceLimits`
/// \post every column of `columnLeaves` stands in exactly one pass (FR-002);
///       a pass that is not exclusive stays within every published limit
///       with its leaves deduplicated per pass (I-1); the columns of a pass
///       keep request order and the passes keep the order of their first
///       column (I-2); a column over some limit on its own stands alone in a
///       pass flagged exclusive (FR-003); a source with no published limit
///       costs nothing and rides every pass whose columns need it (FR-005);
///       the plan is empty exactly when no needed source publishes a limit,
///       the row of E-01 that settles `MULTIPLEX`; two calls over the same
///       inputs give the same assignment (FR-004, SC-02)
[[nodiscard]] auto planPasses(
    const std::vector<std::vector<ColumnLeaves>>& columnLeaves,
    const std::vector<std::optional<std::size_t>>& sourceLimits) -> PassPlan;

/// @brief specs/024 E-05, FR-014, FR-015, FR-016: the one result a run
/// publishes, merged from the fold results of its passes (research D-07).
///
/// `passResults` holds the fold result of each pass in pass order, each in
/// the shape the runner folds one pass today: its rows with their metric
/// values, the time columns, the overhead floor, the endpoint samples, the
/// chunk disclosure and the percentile columns. The merge reads provenance
/// from `plan` and consults no clock and no counter (I-5).
///
/// \pre `passResults` holds at least one fold result and its count equals
///      `plan.columns.size()`; every index of `plan.measuringPass` names one
///      of them, and `plan.measuringPass` carries an entry for every
///      percentile slot past the two time slots of the widest grid
///      `passResults` holds, of a row and of a context block alike
/// \post every metric and user-counter column value comes from its measuring
///       pass at its own position (FR-014); the time columns, the overhead
///       floor, the endpoint samples, the context disclosures, the chunk
///       disclosure and the time-column percentiles come from pass 0
///       (FR-015), and a metric column's percentiles come from its own pass;
///       a value whose measuring pass folded a gap keeps
///       `counters::Availability::GAP` and every refusal kind rides
///       unchanged (FR-016); each value's `pass` names its measuring pass
///       (E-08)
[[nodiscard]] auto mergePasses(const PassPlan& plan,
                               const std::vector<BenchmarkResult>& passResults)
    -> BenchmarkResult;

/// @brief specs/024 E-06, FR-017: the spread between the passes of one run,
/// as a percentage of their median (research D-08).
///
/// `passTimes` holds the merged reference time per iteration of each pass, in
/// pass order - the number the report shows for that pass, which is the
/// merged value of a threaded run (D-6, clarification Q3). The warning of
/// FR-018 is the predicate on this value standing above 5.0, read on the
/// unrounded figure; it lives with the report that prints it.
///
/// \pre no precondition: fewer than two passes and a zero median are each an
///      answer of their own (FR-017)
/// \post nothing answers when `passTimes` holds fewer than two entries or its
///       median is zero, the median of an even count being the lower of the
///       two central values (D-6); the value otherwise is
///       `(max - min) / median * 100` unrounded, the print being the one
///       rounding (research D-08)
[[nodiscard]] auto passSpread(const std::vector<double>& passTimes)
    -> std::optional<double>;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_PASS_PLAN_HPP
