#ifndef SG_HARNESS_DETAIL_COMPARE_HPP
#define SG_HARNESS_DETAIL_COMPARE_HPP

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "internal.hpp"
#include "speedgun-ng/benchmark.hpp"

/**
 * @file compare.hpp
 * @brief The regression gate of specs/026: the compare mode, the table
 * it prints, and the comparison document it writes. Nothing here is
 * public API; the comparison calls and their rule, option and result
 * values stand in `speedgun-ng/benchmark.hpp` (FR-064, D-02).
 */

namespace sg::detail
{

/// @brief The rule the gate applies when `--compare-rule=` gave none:
/// given rules replace it (specs/026 spec D-5, FR-033, FR-034).
inline constexpr std::string_view kDefaultCompareRule = "time@median:lower:5%";

/**
 * @brief What the compare mode answers the command line
 * (specs/026 E-11, FR-052, D-15).
 *
 * `status` is the exit status the mode returns, taken from the table of
 * `specs/015-benchmark-harness-core/contracts/cli.md`; `failed` is set from
 * the comparison verdict or from the fault. The two are computed
 * independently, which is what gives the closing `SG_ENSURE` of the
 * compare branch its content.
 */
struct CompareExit
{
  int status;
  bool failed;
};

/**
 * @brief The index of the result whose full instance name equals
 *        `name`, or the size of `results` when none does
 *        (specs/026 FR-002, FR-008; specs/040 R-2).
 *
 * The one name-pairing rule the compare mode and the baseline
 * extraction of specs/040 share: a report holds a handful of
 * benchmarks, so the match scans the list directly (FR-008).
 *
 * \pre `results` holds no two results under one name, which is what
 *      the registry guarantees for a run and the reader for a
 *      document
 * \post the answer is the index of the one result of `results` whose
 *       full instance name equals `name`, and stands
 *       `results.size()` when none does; the call measures nothing
 *       and reads no clock
 */
[[nodiscard]] auto indexOfResult(
    const std::vector<BenchmarkResult>& results, std::string_view name)
    -> std::size_t;

/**
 * @brief The comparison core the public call wraps: the version pairing,
 * the matching and the validation walk of
 * `contracts/compare-call.md` (specs/026 FR-002, FR-003, FR-008 to FR-011,
 * FR-035 to FR-040, FR-051, D-13, D-19, spec D-20).
 *
 * The default rule of FR-033 stands in `compareReports`, so an empty rule
 * list here answers no entry; the whole `\pre` and `\post` block of the
 * core stands above its definition in `source/harness/compare.cpp`.
 */
[[nodiscard]] auto compareReportValues(const ReportData& baseline,
                                       const ReportData& contender,
                                       const std::vector<CompareRule>& rules,
                                       bool strict)
    -> std::expected<CompareResult, std::string>;

/**
 * @brief Run the whole compare mode for one parsed command line
 * (specs/026 FR-048, FR-050, FR-052, FR-057, D-12, D-19).
 *
 * The mode loads both documents through `loadReport`, scopes the loaded
 * values with the batch filter, compares them, writes the comparison
 * document when `--json=` names a path, prints the table or the one fault
 * line, and answers the status. The document stands before the table so a
 * refused write leaves one line naming the path and no table (FR-074).
 * It runs no benchmark, reads no clock and samples no sensor
 * (FR-007, SC-005).
 *
 * \pre `options.compareBaseline` is not empty: the caller stands the
 *      mode up only when `--compare` named a baseline
 * \post the answer holds a status from the table of
 *       `specs/015-benchmark-harness-core/contracts/cli.md`, and `failed`
 *       set exactly when that status is not 0
 */
[[nodiscard]] auto runCompareMode(const RunOptions& options) -> CompareExit;

/**
 * @brief Print the comparison table of a finished comparison
 * (specs/026 FR-053, FR-054, FR-055, FR-056).
 *
 * One row per benchmark per rule: the name, the rule text, the two
 * values, the change in percent and in absolute terms, and the state
 * word. The header names the two files and, for each report, the
 * speedgun version and the schema version the load call read. A time
 * value prints, in each value column and in the absolute change, in the
 * unit the baseline report declares.
 *
 * \pre `result` is the comparison of `baseline` against `contender`
 * \post one row per entry of `result.entries` on stdout, and nothing
 *       else; a NOISY row carries its reason as a trailing field, which is
 *       what names the condition that fired (SC-006); the call throws
 *       nothing
 */
auto printCompareTable(const CompareResult& result,
                       const ReportData& baseline,
                       const ReportData& contender,
                       const std::string& baselinePath,
                       const std::string& contenderPath) -> void;

/**
 * @brief Write the comparison document of one finished comparison
 * (specs/026 FR-057, FR-058, FR-074, E-10, D-14).
 *
 * The document holds the comparison the call answered and nothing else:
 * no per-iteration values, no chunk columns, no recomputed statistic,
 * no substituted default, and no date, host or pid, so two runs of one
 * command write the same bytes.
 *
 * \pre `path` is not empty and `result` is the value the comparison
 *      answered for the rules named here
 * \post true when the file holds the whole document, false when the
 *       file system refused it; the call reports the refusal on stderr
 *       and throws nothing
 */
[[nodiscard]] auto writeCompareReport(const std::string& path,
                                      const CompareResult& result,
                                      const std::vector<CompareRule>& rules,
                                      const std::string& baselinePath,
                                      const std::string& contenderPath) -> bool;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_COMPARE_HPP
