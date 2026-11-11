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

#include "jitter_probe.hpp"
#include "options.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_system.hpp"
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

/// @brief Whether a metric column carries a figure of its own.
///
/// A counted value and a gap carry the figure their windows folded. A refusal
/// kind (`PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`)
/// measured nothing, so the console prints `null` in place of its value and
/// its running ratio and the document writes `null` for both, never the
/// defaults of `sg::MetricValue` (FR-023, FR-024).
///
/// \post true for `COUNTABLE` and `GAP`, false for every refusal kind
[[nodiscard]] constexpr auto metricCarriesFigure(
    const sg::counters::Availability availability) noexcept -> bool
{
  return availability == sg::counters::Availability::COUNTABLE
      || availability == sg::counters::Availability::GAP;
}

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

// The method words of the series record (specs/043 E-21, FR-019). The
// document spells the method with the upper-case word the enumerator
// names, and the first word stands in no written document: a run that
// names no method writes no method key (specs/043 FR-006, FR-019, D-7).
// The fourth word answers the specs/048 method, whose document word stands
// upper case while the option and the console spell it `hwlat` (specs/048
// FR-002, FR-029, E-01).
inline constexpr std::array<std::string_view, 4> kJitterMethodWords {
    "TSC",
    "FWQ",
    "FTQ",
    "HWLAT",
};
static_assert(kJitterMethodWords.size()
              == std::to_underlying(sg::JitterMethod::HWLAT) + 1);

// The engine words of the optional tracer record (specs/036 E-20,
// contracts/report-json.md). The document spells the engine words the way
// the `--os-noise` value names them, so this table stands apart from the
// enumerator spellings above; the first word answers the `NONE` enumerator
// and stands in no written document, since a run that requests no tracer
// writes no key (specs/048 FR-026, D-2).
inline constexpr std::array<std::string_view, 4> kTracerEngineWords {
    "NONE",
    "osnoise",
    "timerlat",
    "hwlat",
};
static_assert(kTracerEngineWords.size()
              == std::to_underlying(sg::TracerEngine::HWLAT) + 1);

// The two words of the gap mark of specs/048 FR-026, the document's own
// spellings. The writer spells one of them for each kept gap the `hwlat`
// method marks, and the reader tests membership and keeps no record, so no
// second table of the two words stands in the tree (FR-026, FR-013).
inline constexpr std::array<std::string_view, 2> kGapMarkWords {
    "NOT_SMI",
    "SMI",
};

// The word the detector engine writes, read from the table above so the
// writer and the printer compare the record's text against one name
// (specs/048 FR-018, FR-027, FR-029).
inline constexpr std::string_view kHwlatEngineWord =
    kTracerEngineWords.at(std::to_underlying(sg::TracerEngine::HWLAT));

// The one limit the user-space probe carries: its loop runs with the cpu's
// interrupts enabled. The `jitter.limits` text and the console limits line read
// this one sentence, so the two states cannot drift apart (specs/048 FR-030,
// FR-031, P-13).
inline constexpr std::string_view kJitterLimitsText =
    "probe runs with interrupts enabled";

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

// The finder words of the run-level `counterLimits` record
// (specs/041-counter-probe FR-051). The document spells the source with the
// lower-case word FR-051 names, which is why this table stands apart from the
// enumerator spellings above; the first word answers the `NONE` enumerator and
// stands beside a `null` limit, the pair a device that published no figure
// carries.
inline constexpr std::array<std::string_view, 3> kLimitSourceWords {
    "none",
    "decode",
    "probe",
};
static_assert(kLimitSourceWords.size()
              == std::to_underlying(sg::counters::LimitSource::PROBE) + 1);

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
  // specs/048-hwlat-probe FR-013, FR-026, E-03, P-3: the SMI change of each
  // kept gap's span, index-paired with `gaps`, one element per entry of that
  // list. The writer pairs the two lists to write one gap's `smi` mark, and
  // a cpu whose SMI source stood unavailable leaves the list empty, so every
  // gap of that cpu marks nothing (FR-015).
  std::vector<std::uint64_t> smiChanges {};
  // specs/043 FR-020, E-23: the series of the same covered windows, the
  // record the `series` array carries beside the standing `stats` array.
  // A `TSC` run keeps no series, so the member stands empty for it
  // (FR-006).
  std::optional<sg::JitterSeriesStats> series {};
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
  // specs/043 FR-019, E-21: the method the run measured. The `TSC` value
  // is the standing spelling, and the writer writes no method key for it
  // (FR-006, D-7).
  sg::JitterMethod method = sg::JitterMethod::TSC;
  std::uint64_t thresholdNs = 0;
  std::uint64_t loopPeriodNs = 0;
  std::vector<std::uint32_t> cpus;
  std::vector<JitterReportCpu> stats;
  std::uint64_t droppedGaps = 0;
  bool rawGaps = false;
  std::string disclosure;
};

/**
 * @brief specs/041-counter-probe FR-051: one entry of the run-level
 * `counterLimits` record: one device the run needed, the limit it published,
 * and the finder that answered that limit.
 *
 * The entry stands for a device a metric leaf of the run resolved against. A
 * device that published no figure carries no `limit` and the `NONE` source,
 * the pair `objectLimitSource` and `Object::programmableCounters()` agree on
 * (FR-040a). The record stands inside the harness: no field of it joins
 * `include/speedgun-ng/benchmark.hpp`, so no exported layout moves and the
 * harness `SOVERSION` stays (FR-052, D-3).
 */
struct CounterLimitEntry
{
  std::string device;  ///< the object path a leaf resolved against
  std::optional<std::size_t> limit {};  ///< the figure that path published
  sg::counters::LimitSource source {
      sg::counters::LimitSource::NONE};  ///< the finder that answered the
                                         ///< figure
};

/**
 * @brief Seat one needed device of this process's runs beside its finder
 * (specs/041-counter-probe FR-051).
 *
 * The runner's limit gathering is the caller, and it calls once per device per
 * run. A path this process already seated keeps the entry it has: one process
 * publishes one figure and one finder per path, so a second seating of that
 * path states the fact the first seating stated.
 *
 * @pre `entry.device` names the object a metric leaf of the run resolved
 *      against, and `entry.source` is the finder `objectLimitSource` answers
 *      for that path (FR-040a)
 * @post `counterLimitEntries()` answers one entry for `entry.device`; a second
 *       seating of that path changes no entry (FR-051)
 */
auto noteCounterLimit(CounterLimitEntry entry) -> void;

/**
 * @brief The devices this process's runs needed, in the order the first run
 * needed them (specs/041-counter-probe FR-051).
 *
 * @return the seated entries, empty for runs that resolved no metric leaf
 *         against any device; the writer of the run-level record reads it once
 */
[[nodiscard]] auto counterLimitEntries()
    -> const std::vector<CounterLimitEntry>&;

/**
 * @brief specs/041-counter-probe FR-052: the `counterLimits` entries one
 * written document carries, read back inside the harness.
 *
 * The read answers no entry for a document older than minor 1.15, which carries
 * no key, and it guesses no finder for a device the document leaves out. The
 * answer stands in this header and in `source/harness/json_load.cpp`: no
 * exported layout grows a member for it (FR-052, D-3).
 *
 * @pre `path` is not empty
 * @post a document with the record answers one entry per device it names, in
 *       the order the document lists them; a document without the key answers
 *       no entry (FR-052)
 * @post an entry the reader cannot read answers an error naming the JSON
 *       pointer of the key at fault
 */
[[nodiscard]] auto loadCounterLimits(const std::string& path)
    -> std::expected<std::vector<CounterLimitEntry>, std::string>;

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
 * @pre `graph`, when filled, carries one entry per threaded run of the
 *      batch, in the batch's order, each entry naming its run by the
 *      benchmark name and carrying that run's nodes in ascending slot
 *      order, its edges in the file's order, its held nodes with their
 *      reasons and its stopped nodes (specs/037 FR-023, D-5)
 * @post true when the file holds the whole document, false when the file
 *       system refused it; the call reports the refusal on stderr and
 *       throws nothing (FR-030)
 * @post a filled `jitter` writes the root `jitter` object in the E-07
 *       key order after `speedgun` and before `benchmarks`, and the
 *       nullable per-entry key after `noiseWarning` of every entry
 *       (FR-026, FR-027, E-08); an empty one writes neither, so a
 *       probe-free run keeps the fields and the shape master's document
 *       carries (FR-013, D-1)
 * @post a `jitter` record whose method stands `TSC` writes neither the
 *       `method` key nor a `series` array, in the root object or in a
 *       per-entry object, so its bytes match the bytes this writer
 *       produced before specs/043 apart from the `schemaVersion` value
 *       (specs/043 FR-006, FR-020, D-7, SC-004). A record whose method
 *       stands `FWQ` or `FTQ` writes the method word after
 *       `loopPeriodNs`, the run-level `series` array after it with the
 *       period list of each cpu, and the per-entry `series` array beside
 *       the standing `stats` array with no method key and no period list
 *       (specs/043 FR-019, FR-020, E-23)
 * @post a filled `precheck` writes the root `precheck` object after
 *       `jitter` and before `benchmarks` (specs/034 FR-024, D-8); an
 *       empty one writes no such key, so a run without the pre-check
 *       writes the same bytes apart from the `schemaVersion` value
 *       (specs/034 FR-024). A filled `precheck->tracer` writes the
 *       `tracer` object inside the `precheck` object after
 *       `skippedGroups` (specs/036 FR-014, E-20); an empty one writes
 *       no such key, so a run that requests no tracer and a run whose
 *       tracer stands not available write the same bytes apart from the
 *       `schemaVersion` value (specs/036 FR-015, R-10)
 * @post a filled `graph` writes the root `graph` object after
 *       `precheck` and before `benchmarks`, with one entry of the
 *       `runs` array per threaded run in the batch's order (specs/037
 *       FR-023, R-11); an empty one writes no key, so a run with no
 *       graph writes the same bytes apart from the `schemaVersion`
 *       value (specs/037 FR-006, SC-005). A single-context run gains
 *       no entry (specs/037 FR-012).
 * @post an entry whose `topdown` record stands writes the `topdown`
 *       object of that entry after its `hunt` key, one object that holds
 *       the fields of the record: the support flag and its reason, the
 *       table reference and directory, the file rows, the asked level and
 *       the level, the node rows and the refused rows (specs/042-topdown
 *       FR-032, FR-034); an entry whose record stands empty writes no such
 *       key, so a run that named no `--topdown` writes the same bytes apart
 *       from the `schemaVersion` value (specs/042-topdown FR-035, FR-036)
 * @post a non-empty `counterLimits` writes the run-level `counterLimits`
 *       array after `graph` and before `benchmarks`, one object per device
 *       with `device`, `limit` and `source` in that order (specs/041-counter-
 *       probe FR-051); an empty one writes no key, so a run that needed no
 *       device writes the bytes this writer wrote before the record existed
 *       apart from the `schemaVersion` value (FR-051)
 */
[[nodiscard]] auto writeReport(
    const std::string& path,
    const ReportData& data,
    const RunDisclosure& disclosure = {},
    const std::optional<JitterReportRecord>& jitter = std::nullopt,
    const std::optional<PrecheckRecord>& precheck = std::nullopt,
    const std::optional<GraphRecord>& graph = std::nullopt,
    const std::vector<CounterLimitEntry>& counterLimits = {}) -> bool;

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
