#ifndef SG_VIEWS_HPP
#define SG_VIEWS_HPP

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file views.hpp
 * @brief The report view surface: the resolved series of one column,
 * the SVG histogram, the SVG plot, the static HTML page and the
 * terminal text view over one loaded report document.
 *
 * Feature 038-report-view-generators. Namespace `sg`. Every function
 * takes the document value `loadReport` answered and answers the
 * `std::expected` pair of the views-api contract: the value form
 * carries the view bytes, the error form names the input the document
 * does not carry (FR-004, FR-005, D-10). No function reads a file,
 * writes a file, reads a clock, throws to the caller or aborts.
 */

namespace sg
{

/**
 * @brief The resolved series one view renders (specs/038
 * data-model.md).
 *
 * `values` holds the recorded values of one column of one benchmark
 * in document order, the unavailable rows filtered out and the time
 * columns scaled per A-5 of
 * specs/038-report-view-generators/contracts/views-api.md. `unit`
 * names the label the values print beside; a metric column prints as
 * stored and names no unit, so `unit` stands empty for a metric
 * label.
 */
struct ViewSeries
{
  std::vector<double> values;
  std::string unit;
};

/**
 * @brief Resolve the recorded values of one column of one benchmark
 * (B-1, FR-004).
 *
 * The column selector reads `time` for the primary time column, `cpu`
 * for the CPU-time column, and a metric label of the benchmark for a
 * metric column (A-3); a counter label stands outside the selector.
 *
 * \pre `benchmark` and `column` name no empty string (an empty name is
 *      a contract violation)
 * \post on the value form: `values` holds the column's recorded values
 *      in document order, the unavailable rows filtered out and scaled
 *      per A-5, and `unit` names the unit the values print in; an
 *      empty `values` answers on the value form, since the column
 *      resolved and the run recorded no measured value for it
 * \post on the error form: `benchmark` matches no result, or `column`
 *      stands outside the selector enumeration; the string names the
 *      name and the document's recorded set
 * \post the call throws nothing to the caller, aborts nothing, reads
 *      no file and writes no file (A-1)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto viewSeries(const ReportData& report,
                                                 std::string_view benchmark,
                                                 std::string_view column)
    -> std::expected<ViewSeries, std::string>;

/**
 * @brief Render the SVG histogram of one column of one benchmark
 * (B-2, FR-007).
 *
 * The histogram answers one bar per bin beside its range and count
 * label (clarify 2/Q3).
 *
 * \pre `binCount` is at least 2 (a smaller count is a contract
 *      violation)
 * \post on the value form: the bins cover the recorded value range
 *      with the last bin closed on the maximum, and the bars stand
 *      proportional to the counts (FR-007)
 * \post on the error form: fewer than two recorded values stand for
 *      the column; the string names `not available` and the sample
 *      count (FR-008, D-6)
 * \post the call throws nothing to the caller, aborts nothing, reads
 *      no file and writes no file (A-1)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto renderHistogram(
    const ReportData& report,
    std::string_view benchmark,
    std::string_view column,
    std::size_t binCount) -> std::expected<std::string, std::string>;

/**
 * @brief Render the SVG plot of one column of one benchmark (B-3,
 * FR-009).
 *
 * \pre none
 * \post on the value form: one point per recorded value in document
 *      order, the axes labeled with the row and the column names; a
 *      multipass run's rows stand in pass order, and a single recorded
 *      value renders one point (FR-009, D-7)
 * \post on the error form: the series stands empty; the string names
 *      the column and the sample count of zero (FR-010)
 * \post the call throws nothing to the caller, aborts nothing, reads
 *      no file and writes no file (A-1)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto renderPlot(const ReportData& report,
                                                 std::string_view benchmark,
                                                 std::string_view column)
    -> std::expected<std::string, std::string>;

/**
 * @brief Render one static HTML page carrying the run's identity and
 * the two figures (B-5, FR-011).
 *
 * \pre `binCount` is at least 2 (a smaller count is a contract
 *      violation)
 * \post on the value form: the identity fields as the document records
 *      them, the histogram and the plot inline as the two SVG figures,
 *      the text escaped, and no script; an identity field the document
 *      does not carry renders `not available` in its place, and a
 *      figure that answers the disclosure renders the disclosure text
 *      in the figure's place (FR-011, FR-012, D-8); the embedded
 *      figure bytes stand equal to the standalone bytes (D-3)
 * \post the call throws nothing to the caller, aborts nothing, reads
 *      no file and writes no file (A-1)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto renderPage(const ReportData& report,
                                                 std::string_view benchmark,
                                                 std::string_view column,
                                                 std::size_t binCount)
    -> std::expected<std::string, std::string>;

/**
 * @brief Render the terminal text of both figures (B-4, FR-013).
 *
 * \pre `binCount` is at least 2 (a smaller count is a contract
 *      violation)
 * \post on the value form: the histogram as one line per bin naming
 *      the range and the count with a proportional character bar, the
 *      plot as one line per recorded value in row order; the view
 *      composes per part, a part whose figure answers the disclosure
 *      rendering the disclosure line in the part's place (clarify
 *      2/Q1)
 * \post the call throws nothing to the caller, aborts nothing, reads
 *      no file and writes no file (A-1)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto renderText(const ReportData& report,
                                                 std::string_view benchmark,
                                                 std::string_view column,
                                                 std::size_t binCount)
    -> std::expected<std::string, std::string>;

}  // namespace sg

#endif
