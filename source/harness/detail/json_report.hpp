#ifndef SG_HARNESS_JSON_REPORT_HPP
#define SG_HARNESS_JSON_REPORT_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "options.hpp"
#include "jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/execution_context.hpp"

/**
 * @file json_report.hpp
 * @brief The JSON report of specs/023: the writer, the schema checker,
 * and the noise-warning predicate the console report and the document
 * share. Nothing here is public API; the reader is, and it stands in
 * `speedgun-ng/benchmark.hpp`.
 */

namespace sg::detail
{

// The name words of the document. Each table is a closed list indexed by
// the enumerator, the shape the console report gives its own names, and
// the spelling is the C++ enumerator itself (specs/023 E-06, D-06). The
// console table of `report.cpp` spells the availability words in lower
// case with hyphens and the wait modes in upper case, so the two tables
// stand apart and each is tied to its last enumerator by its own
// assertion; the writer and the reader of the document share these.
inline constexpr std::array<std::string_view, 3> kOutcomeWords {
    "MEASURED",
    "SKIPPED",
    "FAILED",
};
static_assert(kOutcomeWords.size()
              == std::to_underlying(sg::RunOutcome::FAILED) + 1);

inline constexpr std::array<std::string_view, 5> kRowKindWords {
    "REPETITION",
    "AGGREGATE",
    "BIG_O",
    "RMS",
    "SKIPPED",
};
static_assert(kRowKindWords.size()
              == std::to_underlying(sg::RowKind::SKIPPED) + 1);

inline constexpr std::array<std::string_view, 6> kAvailabilityWords {
    "COUNTABLE",
    "PERMISSION_BLOCKED",
    "NOT_ENCODABLE",
    "ABSENT",
    "SCOPE_REFUSED",
    "GAP",
};
static_assert(kAvailabilityWords.size()
              == std::to_underlying(sg::counters::Availability::GAP) + 1);

inline constexpr std::array<std::string_view, 4> kTimeUnitWords {
    "NANOSECONDS",
    "MICROSECONDS",
    "MILLISECONDS",
    "SECONDS",
};
static_assert(kTimeUnitWords.size()
              == std::to_underlying(sg::TimeUnit::SECONDS) + 1);

inline constexpr std::array<std::string_view, 3> kJitterModeWords {
    "ALONGSIDE",
    "PRECHECK",
    "BOTH",
};
static_assert(kJitterModeWords.size()
              == std::to_underlying(sg::JitterProbeMode::BOTH) + 1);

inline constexpr std::array<std::string_view, 2> kStatUnitWords {
    "TIME",
    "PERCENTAGE",
};
static_assert(kStatUnitWords.size()
              == std::to_underlying(sg::StatUnit::PERCENTAGE) + 1);

inline constexpr std::array<std::string_view, 3> kWaitModeWords {
    "SPIN",
    "YIELD",
    "BLOCK",
};
static_assert(kWaitModeWords.size()
              == std::to_underlying(sg::WaitMode::BLOCK) + 1);

inline constexpr std::array<std::string_view, 3> kMetricMethodWords {
    "AUTO",
    "PASSES",
    "MULTIPLEX",
};
static_assert(kMetricMethodWords.size()
              == std::to_underlying(sg::MetricMethod::MULTIPLEX) + 1);

// The word one value of a closed enumeration writes. A value outside the
// enumeration writes the one word the document carries for it, the guard
// `availabilityName` stands beside the console table. The span form keeps
// one function for every table: two instantiations inlined into one caller
// read as one subscript over another table's storage under the sanitizer
// build, which is a warning this tree carries as an error.
inline auto enumWord(std::span<const std::string_view> words,
                     std::size_t index) -> std::string_view
{
  if (index >= words.size()) {
    return "UNKNOWN";
  }
  // The guard above holds the index within the table, and the table is
  // the caller's own word list.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  return words[index];
}

// The enumerator one word of the document names, or nothing when the
// word stands outside the closed list (specs/023 FR-024).
template<class Enum, std::size_t Size>
auto enumOf(const std::array<std::string_view, Size>& words,
            std::string_view word) -> std::optional<Enum>
{
  for (std::size_t index = 0; index < words.size(); ++index) {
    if (words[index] == word) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      return static_cast<Enum>(index);
    }
  }
  return std::nullopt;
}

// The placement text a context carries when it pins to no cpu, and the
// text of a slot the placement function did not land. The disclosure
// line, the noise condition and the document all read these two words.
inline constexpr std::string_view kNoPinPlacement = "NO_PIN";
inline constexpr std::string_view kUnappliedPlacement = "UNAPPLIED";

// The warning record itself, `sg::NoiseWarning`, stands in the public
// header (specs/026 E-06, D-05); only the predicate below is internal.

/**
 * @brief The noise warning of one result, or nothing when the console
 * prints no warning for it (FR-017, FR-026, FR-028).
 *
 * The condition is the one `printNoiseWarning` applies: two spinning
 * contexts name one cpu index, or the spinning contexts outnumber the
 * cpus this process may run on. The runnable count reads the affinity
 * mask of this process and falls back to the host count, so the call
 * reaches no clock and samples nothing (FR-027).
 */
[[nodiscard]] auto noiseWarningOf(const BenchmarkResult& result)
    -> std::optional<NoiseWarning>;

// The limit the pass warning reads, as a percentage of the median of the
// pass times, on the unrounded figure (specs/024 FR-018). The console names
// this figure in its own line and the document carries the answer below it.
inline constexpr double kSpreadWarningLimitPercent = 5.0;

/**
 * @brief Whether one pass spread carries the warning of specs/024 FR-018.
 *
 * The console line, the `spreadWarning` key of the document and the
 * reader's check of that key all answer this one predicate, so the limit is
 * stated once (FR-018). A spread the run could not compute carries no
 * warning, the reading of `spread=not available`.
 */
[[nodiscard]] auto spreadWarned(std::optional<double> spread) noexcept -> bool;

/**
 * @brief The major part of one `schemaVersion` text (specs/026 D-13).
 *
 * The writer moves the minor for a key it adds inside major 1, so the
 * reader of a document and the gate that pairs two documents read the
 * major from this one split. A text with no dot, or with an empty part on
 * either side of the dot it carries, states no major at all.
 *
 * @return the text before the dot, or nothing when the text states no major
 */
[[nodiscard]] inline constexpr auto schemaMajor(
    std::string_view version) noexcept -> std::optional<std::string_view>
{
  const auto dot = version.find('.');
  if (dot == std::string_view::npos || dot == 0 || dot + 1 == version.size()) {
    return std::nullopt;
  }
  return version.substr(0, dot);
}

/**
 * @brief What the writer is given beside the results: the option uses a
 * source other than the command line settled, and the interleaving seed
 * (specs/028 E-08, FR-040, D-24).
 *
 * `uses` holds the disclosed uses in table order, and `interleave` holds
 * the seed only when `--interleave` stands given. `ReportData` carries no
 * field of either, so the public headers stay as they are (FR-076).
 */
struct RunDisclosure
{
  std::span<const OptionUse> uses;
  std::optional<std::uint64_t> interleave;
};

/**
 * @brief The per-cpu line of the run-level jitter record (E-07): the
 * cpu index, the fold over that cpu's covered windows, and the kept
 * gap records the `--json-raw=gaps` condition asks for (E-10, FR-028).
 */
struct JitterReportCpu
{
  std::uint32_t cpu = 0;
  sg::JitterStats stats {};
  std::vector<GapRecord> gaps;
};

/**
 * @brief The run-level jitter record the document carries (E-07),
 * filled from the probe states after the holder is joined (R-04).
 *
 * `disclosure` carries the record data-model.md E-09 canonically lists
 * as one text, the content the console disclosure lines print (FR-025,
 * D-8). `rawGaps` stands when `--json-raw=gaps` named the run, and it
 * is the one condition under which a stats object carries `gaps`
 * (FR-028); an empty list then stands for a cpu that kept no gap.
 */
struct JitterReportRecord
{
  sg::JitterProbeMode mode = sg::JitterProbeMode::ALONGSIDE;
  std::uint64_t thresholdNs = 0;
  std::uint64_t loopPeriodNs = 0;
  std::vector<std::uint32_t> cpus;
  std::vector<JitterReportCpu> stats;
  std::uint64_t droppedGaps = 0;
  bool rawGaps = false;
  std::string disclosure;
};

/**
 * @brief Write the report document of a finished batch to `path`
 * (specs/023 FR-001, D-02).
 *
 * The writer reads no clock and samples nothing, and it adds nothing to
 * the values it is handed (FR-005). A row whose `rowRaw` entry holds
 * columns writes them under `raw`; every other row writes no `raw` key,
 * which is the whole of the raw setting as the document sees it (E-05).
 *
 * `disclosure` carries the run facts of specs/028: a disclosure that names
 * a use or a seed writes the top-level `run` object between `speedgun` and
 * `benchmarks`, and an empty one writes no such key (specs/028 FR-039 to
 * FR-041, D-18).
 *
 * @pre `path` is not empty, and `data.rowRaw.size() ==
 *      data.results.size() == data.contextRaw.size()`
 * @pre every `disclosure.uses` entry points at a row of
 *      `detail::optionRows()` and names a source other than the command
 *      line (specs/028 E-05)
 * @pre `jitter`, when filled, carries one stats object per probe cpu,
 *      the cpus ascending (E-07)
 * @post true when the file holds the whole document, false when the file
 *       system refused it; the call reports the refusal on stderr and
 *       throws nothing (FR-030)
 * @post a filled `jitter` writes the root `jitter` object in the E-07
 *       key order after `speedgun` and before `benchmarks`, and the
 *       nullable per-entry key after `noiseWarning` of every entry
 *       (FR-026, FR-027, E-08); an empty one writes neither, so a
 *       probe-free run keeps the fields and the shape master's document
 *       carries (FR-013, D-1)
 */
[[nodiscard]] auto writeReport(
    const std::string& path,
    const ReportData& data,
    const RunDisclosure& disclosure = {},
    const std::optional<JitterReportRecord>& jitter = std::nullopt) -> bool;

/**
 * @brief Check one written document against the schema file at
 * `schemaPath` (specs/023 FR-008, D-04).
 *
 * The check reads the schema with the vendored parser and compares, at
 * every object of the document, the key set, the key order and the value
 * kinds the schema states. No schema validator library enters the tree.
 *
 * @pre both paths name files this process can read
 * @post when the call answers an error, the string names the JSON
 *       pointer of the first disagreement and what the schema asked for
 *       there
 */
[[nodiscard]] auto checkReport(std::string_view schemaPath,
                               std::string_view document)
    -> std::expected<void, std::string>;

}  // namespace sg::detail

#endif  // SG_HARNESS_JSON_REPORT_HPP
