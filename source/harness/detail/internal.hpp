#ifndef SG_HARNESS_INTERNAL_HPP
#define SG_HARNESS_INTERNAL_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jitter_probe.hpp"
#include "memory_bind.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_metrics.hpp"
#include "speedgun-ng/execution_context.hpp"
#include "speedgun-ng/hunt.hpp"

// specs/029 data-model.md preamble: `detail::RetriedPass` is the record of one
// pass that fell back to lone-event seats, the account an instance carries.
#include "hunt_plan.hpp"

/**
 * @file internal.hpp
 * @brief The harness internals behind the public surface: the registry
 * record, the parsed options, the runner, the report, and the catalog
 * listing. Nothing here is public API.
 */

namespace sg
{

// FR-007 bounds the instances of one family; above the bound expansion
// warns and keeps every instance (R-14).
inline constexpr std::size_t kMaxFamilySize = 100;

namespace detail
{

// specs/025 data-model.md E-01: the ordinal that names no core and no
// group, the same maximum `kNoPin` of the public header holds for a cpu
// index (FR-001).
inline constexpr std::uint32_t kNoGroup =
    std::numeric_limits<std::uint32_t>::max();

/// @brief specs/025 data-model.md E-02: one hardware thread inside the
/// process mask (FR-001, FR-005). `coreOrdinal` indexes
/// `MachineLayout::cores` and `groupOrdinal` indexes
/// `MachineLayout::groups`; either stands at `kNoGroup` when the read
/// found no core or no L3 cache.
struct LayoutThread
{
  std::uint32_t cpuIndex = 0;
  std::uint32_t coreOrdinal = kNoGroup;
  std::uint32_t groupOrdinal = kNoGroup;
};

/// @brief specs/025 data-model.md E-03: one core, with its hardware
/// threads in ascending cpu index so the first entry is the
/// lowest-index sibling (FR-011, FR-013).
struct LayoutCore
{
  std::uint32_t groupOrdinal = kNoGroup;
  std::vector<std::uint32_t> threads;
};

/// @brief specs/025 data-model.md E-04: one L3 cache group, with its
/// cores in ascending order, the walk order of `compact` (FR-012,
/// FR-014).
struct LayoutGroup
{
  std::uint32_t lowestCpu = 0;
  std::vector<std::uint32_t> cores;
};

/// @brief specs/035 data-model.md: one memory node the machine read
/// states (FR-005, R-01). The cpus stand in ascending order and stand
/// unfiltered; the process mask filters at plan and listing time. A
/// node with an empty cpu set stands in the vector and the plan gives
/// it no context (FR-005, data-model.md).
struct LayoutNode
{
  std::uint32_t number = 0;
  std::vector<std::uint32_t> cpus;
};

/**
 * @brief specs/025 data-model.md E-01: the layout of one process, read
 * once and reused (FR-001, FR-002, FR-005).
 *
 * `threads` is ordered by ascending cpu index, `cores` by the lowest
 * cpu of the core, `groups` by the lowest cpu of the group. `mask`
 * holds the cpu indices of the process affinity mask intersected with
 * the topology set, the set the walk built the layout from. A failed
 * host read leaves `readable` false with no partial layout; an
 * installed layout always stands readable.
 */
struct MachineLayout
{
  std::vector<LayoutThread> threads;
  std::vector<LayoutCore> cores;
  std::vector<LayoutGroup> groups;
  std::uint32_t packages = 0;
  std::vector<std::uint32_t> mask;
  bool readable = false;
  // specs/035 FR-005, R-01: the memory nodes the read states, in the
  // read's order, beside the groups; empty when the read states no
  // memory information, which `memoryReadable` states beside them.
  std::vector<LayoutNode> nodes;
  bool memoryReadable = false;
};

/// @brief specs/025 data-model.md E-05: the six placement presets,
/// parsed from text that equals one name in full (FR-007, FR-016;
/// specs/035 FR-001).
enum class Preset
{
  NONE,
  CORE,
  L3,
  SMT,
  COMPACT,
  NODE,
};

/**
 * @brief specs/025 data-model.md E-06: the plan of one preset over one
 * layout for one context count (FR-008, FR-009).
 *
 * An empty `cpu` means the context count exceeds `places`; otherwise
 * `cpu.size()` equals the count, every entry is distinct, and `group`
 * names the group of each cpu. Two calls with one layout and one count
 * return equal vectors (SC-03).
 */
struct PlacementPlan
{
  std::vector<std::uint32_t> cpu;
  std::vector<std::uint32_t> group;
  // specs/035 data-model.md: the node each context binds to, parallel
  // to `cpu` and filled by the `node` plan alone; empty for every other
  // plan (FR-009, D-2).
  std::vector<std::uint32_t> node;
  std::uint32_t places = 0;
};

/**
 * @brief specs/029 data-model.md E-05, D-18, T073, T118: one hunt event whose
 * solo retry pass refused too, kept with the refusal that refused it and the
 * availability the run published for its column.
 *
 * The pass loop cannot name the kind of a refusal - the reading of the state a
 * refusal names, and the kind that state answers, belong to `hunt.cpp` - so it
 * hands the refusal over whole and records the state it published for the
 * column beside it, so the row of that address and the skipped line of that
 * event answer one state (T128).
 */
struct RefusedRetry
{
  // The address of the event the retry pass measured alone.
  std::string address;
  // The error the seat of that pass answered, the counters-layer text with
  // the availability word the skipped kind is read from.
  counters::Error refusal;
  // The availability the run published for that column: the state
  // `detail::availabilityOfRefusal` read off `refusal`, with
  // `Availability::ABSENT` for a refusal that names no state (T118, T128).
  counters::Availability availability = counters::Availability::ABSENT;
};

}  // namespace detail

/**
 * @brief One registry record in registration order (E-01).
 *
 * Beside the H1 run control, the record carries the family surface the
 * handle accumulates: the argument lists the family calls append, one
 * list per instance (FR-001, FR-002); the `argName` labels, one per
 * position (E-04); the range multiplier (E-05); the setup and teardown
 * callback pair (E-07); the fixture factory, null for a plain family
 * (E-06); and the timing selection the four handle calls set before the
 * first run (FR-004, FR-005, FR-006, FR-015).
 */
class RegistryEntry
{
public:
  std::string name;
  std::function<void(State&)> callable;
  std::optional<std::int64_t> minTimeNs;
  std::optional<std::int64_t> warmupTimeNs;
  std::optional<std::uint64_t> repetitions;
  std::optional<std::uint64_t> fixedIterations;
  std::vector<detail::MetricSeed> metrics;
  // The statistics the family attached, in registration order (FR-013).
  std::vector<detail::StatisticSeed> statistics;
  // The two aggregate-only modes the family asked for before its first run
  // (FR-016, FR-017).
  bool reportAggregatesOnly = false;
  bool displayAggregatesOnly = false;
  // The complexity selection the family asked for: the curve class, or the
  // curve function when the family supplied one (FR-019, FR-020).
  BigO bigO = BigO::NONE;
  BigOFun bigOFun;
  bool runStarted = false;

  std::vector<std::vector<std::int64_t>> args;
  std::vector<std::string> argNames;
  std::int64_t rangeMultiplier = kDefaultRangeMultiplier;
  std::function<void(State&)> setup;
  std::function<void(State&)> teardown;
  // The factory of the one fixture object each run builds, through the
  // public `sg::Fixture` base; null for a plain family (E-06, R-09).
  std::function<std::unique_ptr<Fixture>()> fixtureFactory;

  // The timing selection of data-model.md E-01: family state settled
  // before the first run of the family, and read once per benchmark
  // when the runner builds the run plan.
  bool processCpuTime = false;
  bool realTime = false;
  bool manualTime = false;
  // specs/021 FR-019: the percentile selection the family asked for. The
  // command line wins over it (FR-020).
  bool showPercentiles = false;
  // specs/024 FR-023, FR-025: the collection method the family asked for.
  // The metric-method flag on the command line wins over it, and the run
  // settles from it to the method it publishes (E-01).
  MetricMethod metricMethod = MetricMethod::AUTO;
  // specs/021 FR-005: the per-iteration time of the last attempt of this
  // instance, the read that calibrates the chunk size of the next one.
  // Zero names an instance with no measured attempt behind it.
  double iterationNs = 0.0;
  // FR-027: the registration stores no unit; the record holds the
  // `unit` call alone, and a run resolves the unit it prints.
  std::optional<TimeUnit> timeUnit;

  // E-08, FR-051: the family's pause history, the largest count of
  // closed pause pairs one iteration of the family closed in an
  // earlier attempt. Zero is no history: the recorder then reserves
  // the endpoint shape alone and the run folds (D-10).
  std::uint64_t pausePairsPerIteration = 0;

  // E-05, FR-016: the thread counts the thread calls stated, one
  // instance of the family per count. The default of one count of one
  // keeps every family that calls no thread method on its single
  // context and its printed bytes (FR-022).
  std::vector<int> threadCounts {1};
  // specs/025 data-model.md E-08, FR-015: the placement preset name the
  // handle stated, the last call winning the slot the way the thread
  // calls replace an earlier set. Empty when the family named none.
  std::optional<std::string> placement;
  // specs/027 FR-015: the jitter probe options the handle stated for
  // this family's runs, the last call winning the slot the way the
  // thread calls replace an earlier set. Empty when the family named
  // none.
  std::optional<JitterProbeOptions> jitterProbeOptions;
  // FR-021: the dispatch function the family attached for its threaded
  // runs, empty when the harness dispatches the bodies itself.
  ThreadRunnerFn threadRunner;
};

/**
 * @brief One expanded instance of one family record (E-02, R-02).
 *
 * The instance owns its argument storage, so a run reads it with no
 * allocation (R-04). The callable, the run control, the callback pair,
 * and the fixture factory stay on the family record the `family`
 * pointer reaches (R-01, R-02).
 *
 * \pre expansion built the record, and `family` points at the family
 *      record the instance came from (R-02).
 * \post the name, the suite, and the case name name this instance, and
 *       `arguments` holds the tuple it carries (FR-010, FR-021).
 * \invariant the argument storage outlives every run of this instance,
 *            because the instance owns it (R-04).
 */
class Instance
{
public:
  std::string name;
  std::string suite;
  std::string caseName;
  std::vector<std::int64_t> arguments;
  // E-05, FR-016: the thread count expansion selected for this
  // instance, the count of contexts its threaded run opens. The count
  // of 1 a family that called no thread method carries keeps the
  // single-context path and today's row bytes (FR-022).
  int threadCount = 1;
  // specs/025 data-model.md E-08, FR-015: the placement preset resolved
  // for this instance: the handle name when the family stated one, the
  // command-line preset otherwise, `NONE` when neither stands.
  detail::Preset placement = detail::Preset::NONE;
  // specs/039-context-groups FR-003, D-2: the layout path the entry point
  // settled for the batch, empty when no source named the row. The runner
  // reads this file for the partition beside the contexts it groups, the way
  // it reads the preset above, and the fit walk opened it once already
  // (FR-020).
  std::string groupLayout;
  RegistryEntry* family = nullptr;

  // specs/029 data-model.md E-04, D-15: the plan of the hunt this instance
  // runs, set by `runHunt` before the run and empty for a run `--hunt` did not
  // select. The pass loop reads it only to tell a hunt run from every other
  // run, so a run with no hunt keeps its present route (PD-11).
  std::optional<hunt::HuntPlan> huntPlan;

  // specs/029 PD-12, D-18: one entry per pass whose group open refused and
  // fell back to one pass per event of that pass.
  std::vector<detail::RetriedPass> retriedHuntPasses;

  // specs/029 D-18, FR-021: the events whose own retry pass refused too, each
  // with the refusal that refused it: the run keeps going and `runHunt` turns
  // each into a skipped entry with the kind of that refusal.
  std::vector<detail::RefusedRetry> refusedHuntEvents;

  /// @brief specs/029 PD-12, D-18: the passes of this instance's hunt run that
  /// fell back to one event per pass, in the order the pass loop appended them.
  ///
  /// \pre none
  /// \post one entry per pass whose group open refused, and no entry for a pass
  ///       that seated; the list stands empty for a run that carried no hunt
  ///       (I-8)
  [[nodiscard]] auto huntRetriedPasses() const noexcept
      -> const std::vector<detail::RetriedPass>&
  {
    return retriedHuntPasses;
  }

  /// @brief specs/029 D-18, FR-021, T073: the events that still refused after
  /// their solo retry pass, each with the refusal that refused it.
  ///
  /// \pre none
  /// \post one entry per refused solo retry, in refusal order, each holding the
  ///       error its seat answered; empty for a run whose retries all seated
  ///       (D-18)
  [[nodiscard]] auto huntRefusedEvents() const noexcept
      -> const std::vector<detail::RefusedRetry>&
  {
    return refusedHuntEvents;
  }
};

/// @brief The process registry, in registration order (R-09).
[[nodiscard]] auto registry() -> std::vector<std::unique_ptr<RegistryEntry>>&;

/**
 * @brief Append one entry, or report the duplicate name (FR-002).
 *
 * The first registration of a name stays, and the report carries the
 * recoverable-error text.
 */
[[nodiscard]] auto addRegistryEntry(std::function<void(State&)> fn,
                                    std::string_view name)
    -> std::expected<RegistryEntry*, std::string>;

namespace detail
{

/**
 * @brief The fold-path sink of one run (E-03, FR-052): one running
 * total per leaf column, fed at the commit of every folded pause pair
 * and read when the runner folds the row.
 *
 * \invariant `columns` names the run's cores in fold order, so column
 *            `i` of the totals stands beside core `i` of the plan.
 */
struct PauseFoldSink
{
  std::vector<const counters::detail::ExprCore*> columns;
  std::vector<double> values;
  std::vector<sg::counters::Availability> availability;
  std::uint64_t foldedPairs = 0;

  /// @brief Append one column, reading the core the caller keeps alive
  /// for the run (FR-052).
  ///
  /// \pre `core` outlives this sink's run.
  auto addColumn(const counters::detail::ExprCore& core) noexcept -> void
  {
    columns.push_back(&core);
    values.push_back(0.0);
    availability.push_back(sg::counters::Availability::COUNTABLE);
  }
};

/**
 * @brief The recorder reservation of the most recent sampled attempt
 * (E-08): the endpoint points and the pause room the recorder that
 * attempt reserved held. The pause suite reads these two seams to pin
 * the reservation rule (FR-051, FR-053).
 */
struct RecorderReservation
{
  std::size_t endpointPoints = 0;
  std::size_t pauseRoomPoints = 0;
  // specs/021 FR-008: the chunk points the attempt's recorder reserved,
  // one per boundary plus the exit point. Zero names an attempt that
  // captured no chunks.
  std::size_t chunkPoints = 0;
};

/// @brief The reservation of the most recent attempt of this process
/// (FR-051).
[[nodiscard]] auto lastRecorderReservation() -> RecorderReservation&;

/// @brief The largest pause room any reservation of this process held
/// (FR-051).
[[nodiscard]] auto maxRecorderPauseRoom() -> std::size_t&;

/// @brief The thread counts `threadRange` states for these bounds: one count
/// per doubling that stays within them (FR-016, FR-024). Bounds that stand
/// out of order, and a lower bound below one, name no count: the require of
/// the call answers that mistake where contracts stand, and this account
/// holds what a build without contracts reaches (FR-024).
[[nodiscard]] auto buildThreadCounts(int minCount,
                                     int maxCount) -> std::vector<int>;

/// @brief The thread counts `denseThreadRange` states for these bounds: one
/// count per step that stays within them (FR-016, FR-024). A step below one
/// and bounds out of order name no count, for the same reason (FR-024).
[[nodiscard]] auto buildDenseThreadCounts(int minCount,
                                          int maxCount,
                                          int step) -> std::vector<int>;

/// @brief FR-006, FR-050: the dispatch account of one context: every accept
/// stands matched by its dispatch, either linked into the inbox or refused
/// after the stop, which is the state of equal counts.
[[nodiscard]] auto countsSettled(std::uint64_t accepted,
                                 std::uint64_t dispatched) noexcept -> bool;

/// @brief FR-006, FR-050: the end test of one context loop: the stop stands
/// and the dispatch account is settled. The two count reads close the window
/// between another thread's accept and its exchange, so a test that finds the
/// pair apart turns away and stands again, and no accepted node is left
/// behind (FR-050).
[[nodiscard]] auto loopEnds(bool stopRequested,
                            std::uint64_t accepted,
                            std::uint64_t dispatched) noexcept -> bool;

/// @brief FR-020: the skip rule of the threaded merge: the merged record
/// takes the skip of a skipped record where it carries no skip of its own,
/// so the reason of the first skipped context of the attempt stands.
[[nodiscard]] auto takesSkip(bool recordSkipped,
                             bool mergedSkipped) noexcept -> bool;

// FR-003, D-01: the outcome of one pass of the private drain head's walk.
// `NODE` names a successor to take, `EMPTY` the inbox that provably stands
// empty, `PENDING` the pass that finds a producer between its exchange and
// its link, and `REANCHOR` the walked node that stands at the tail.
// NOLINTNEXTLINE(misc-use-internal-linkage) the step the context loop reads
enum class WalkStep : std::uint8_t
{
  NODE,
  EMPTY,
  PENDING,
  REANCHOR,
};

/// @brief FR-003, FR-006: the walk's decision for one pass, read on its own
/// from the three words the pass reads: the successor link of the walked
/// node, whether the inbox tail stands on that node, and whether the walk
/// stands on the sentinel. A producer mid-enqueue answers `PENDING`, which
/// the loop reads together with the empty inbox: the pass holds no node to
/// run (FR-003, FR-006, FR-050).
[[nodiscard]] auto walkStep(bool linkPublished,
                            bool tailAtWalk,
                            bool walkAtSentinel) noexcept -> WalkStep;

/// @brief FR-006, FR-050: the state `drain()` waits for: the tail at the
/// sentinel, the loop standing outside its work, and the dispatch account
/// settled. The caller reads the tail before the phase, the order the proof
/// of FR-006 names.
[[nodiscard]] auto inboxIdle(bool tailAtSentinel,
                             bool working,
                             std::uint64_t accepted,
                             std::uint64_t dispatched) noexcept -> bool;

/// @brief The cpu count the noise warning compares spinning contexts
/// against: the count the affinity mask reported where that read answered,
/// the host count of the standard library where it answered nothing
/// (FR-026).
[[nodiscard]] auto selectRunnableCpuCount(bool maskAnswered,
                                          std::uint64_t masked,
                                          std::uint64_t host) noexcept
    -> std::uint64_t;

/// @brief The disclosure account of one pin attempt: the slot text the pin
/// landed on where the platform accepted it, the unapplied text where it
/// did not (FR-005, FR-025).
[[nodiscard]] auto selectPlacementAccount(
    bool landed, std::string_view landedText) -> PlacementResult;

/// @brief The placement field of the disclosure line: the text whole where
/// it fits the field, cut to the field with a mark of the cut where it does
/// not (FR-025).
[[nodiscard]] auto placementField(std::string_view placement) -> std::string;

/**
 * @brief The batch snapshot `speedgunMain` takes of the process default
 * at its entry (FR-027). Empty outside `speedgunMain`, the case of the
 * run that reads the process default at its own start.
 */
[[nodiscard]] auto batchDefaultTimeUnit() -> std::optional<TimeUnit>&;

/**
 * @brief The suite-level jitter probe binding `runJitterProbe` writes
 * (FR-015). The batch reads it where the command line named none, and
 * the command-line value wins when both stand (FR-015, R-01).
 */
[[nodiscard]] auto suiteJitterProbe() -> std::optional<JitterProbeOptions>&;

/**
 * @brief Expand every family record in the registry into its instance
 * set (FR-003, R-03).
 *
 * `speedgunMain` calls this once, before the filter selection and the
 * first run, and the runner walks what it leaves (R-03, R-12).
 *
 * specs/025 FR-015, FR-016: `processPreset` is the command-line preset
 * resolved at the entry point, and each instance carries the handle
 * name when the family stated one, `processPreset` otherwise. A stored
 * name that parses to nothing prints one line naming the family and
 * the rejected text and exits with status 2 before any instance runs.
 *
 * \pre every family call of every registration has returned, and no run
 *      has started (FR-003, E-01).
 * \post the instance list holds one instance per argument tuple of
 *       every family record, and each instance carries its name, its
 *       suite, its case, and the arguments it owns (FR-010, R-02).
 * \invariant no run performs expansion work (FR-003).
 */
auto expandRegistry(std::optional<Preset> processPreset = std::nullopt) -> void;

/// @brief The expanded instance list, in the order the runner walks it
/// (R-12).
[[nodiscard]] auto instances() -> std::vector<Instance>&;

/**
 * @brief Write the promoted counter set of the run to `path`, one catalog
 * address per line (specs/033-hunter-budgets FR-011, FR-012, R-07).
 *
 * The entry point calls this after the batch, beside the report writer, so the
 * file holds every entry's ranked list. The lines are the deduplicated union of
 * each result's `toCounterSet(topN)`, in result order and, within a result, in
 * rank order.
 *
 * \pre `topN` is the `--hunt-top` the run hunted under
 * \post the file holds one address per line and no header, an empty ranking
 *       writing an empty file; on failure one stderr line names the path and
 *       the reason the file system gave, and the answer is false
 */
auto writeHuntEmitSet(const std::string& path,
                      std::span<const BenchmarkResult> results,
                      std::size_t topN) -> bool;

/**
 * @brief Write the promoted events' comparison rules of the run to `path`,
 * one rule line per promoted event (specs/040-hunter-sensitivity FR-021,
 * FR-022, D-7).
 *
 * The entry point calls this after the batch stamping pass, beside the
 * report writer, so the lines carry the stamped figures (B-10). The lines
 * are the deduplicated union of each result's
 * `toCompareRules(topN, noisePercent)`, in result order and, within a
 * result, in rank order; the stamps leave one figure per address, so one
 * address stands once.
 *
 * \pre `topN` is the `--hunt-top` and `noisePercent` the `--hunt-noise` the
 *      run hunted under
 * \post the file holds one rule line per promoted event and no header, a run
 *       that promotes no event writing an empty file; on failure one stderr
 *       line names the path and the reason the file system gave, and the
 *       answer is false
 */
auto writeHuntEmitRules(const std::string& path,
                        std::span<const BenchmarkResult> results,
                        std::size_t topN,
                        double noisePercent) -> bool;

/**
 * @brief One counter address beside the origin of the source that named it
 * (FR-038, FR-087).
 *
 * The host resolves an address, so the entry point walks the addresses of
 * every source and not the settled list alone: an address a higher source
 * dropped still answers with its own line, behind the origin that names its
 * source. The empty origin is the command line, whose own line names the
 * option instead.
 */
struct CounterAddress
{
  std::string origin;
  std::string address;
};

/**
 * @brief The parsed command line (E-09).
 */
struct RunOptions
{
  std::string filter;
  bool listMode = false;
  bool catalogMode = false;
  bool dryRun = false;
  std::optional<std::int64_t> minTimeNs;
  std::optional<std::int64_t> warmupTimeNs;
  std::optional<std::uint64_t> repetitions;
  std::optional<std::uint64_t> fixedIterations;
  std::vector<std::string> counterAddresses;
  // specs/021 FR-020, FR-021: the two percentile options. A set
  // `showPercentiles` wins over the family's selection; `percentilePoints`
  // carries the list
  // `--percentiles=` stated, empty when the option named none.
  std::optional<bool> showPercentiles;
  std::vector<double> percentilePoints;
  // specs/024 FR-023, FR-025, research D-09: the word of `--metric-method`.
  // A set value wins over the family's request the way `showPercentiles` wins
  // over its selection, and a repeated flag keeps its last word.
  std::optional<MetricMethod> metricMethod;
  std::vector<std::string> metricNames;
  // specs/025 data-model.md E-07, FR-015, FR-016, FR-036: the raw
  // placement option text, parsed once before the first run, and the
  // topology listing flag.
  std::optional<std::string> placement;
  bool topologyMode = false;
  // specs/023 FR-001, FR-029: the path `--json=` named, empty when the run
  // writes no document, and the chunk content `--json-raw=` asked for.
  std::string jsonPath;
  RawContent rawContent = RawContent::NONE;
  // specs/026-regression-gate data-model.md E-11, FR-045, FR-047,
  // FR-049, FR-050: the compare mode's inputs. `compareBaseline` is the
  // path before the first comma of `--compare=`, empty when the mode
  // stands off; `compareContender` the path after that comma, a missing
  // comma or an empty side being a usage fault at the scan.
  // `compareRules` carries one entry per `--compare-rule=`, in order;
  // empty means the default rule. `compareStrict` is the
  // `--compare-strict` flag.
  std::string compareBaseline;
  std::string compareContender;
  std::vector<CompareRule> compareRules;
  bool compareStrict = false;
  // specs/028-full-command-line FR-061, FR-063, D-24: the path the
  // `--compare-rules` word named on the command line, empty when it named
  // none. The rules of that file join the list after the command line's own
  // rules, which is the second position of the order spec D-23 states.
  std::string compareRulesFile;
  // specs/028-full-command-line data-model.md E-11, FR-044, FR-045,
  // FR-050: the counts `--threads=`, an option-file line and
  // `SPEEDGUN_THREADS` settled, in item order, a `min:max` range already
  // expanded by `buildThreadCounts`. An empty list means no source named
  // the option, and the counts a family registered then stand.
  std::vector<int> threadCounts;
  // specs/028-full-command-line FR-029, FR-034, E-03: the path the
  // `--options` word named, empty when it named none. The entry point owns
  // the read and hands the text to the gather (D-05).
  std::string optionsFile;
  // specs/028-full-command-line FR-028, D-04: the word of `--env`. Only
  // `off` stands, and it settles the run with no environment source, so the
  // gather is handed no rows at all.
  bool environmentOff = false;
  // specs/027 FR-015, R-01, R-06: the parsed jitter probe options, parsed
  // once before the expansion and before any run. Empty when the option
  // stood absent.
  std::optional<JitterProbeOptions> jitterProbe;
  // specs/027 FR-015, R-06: the option's raw text as the command line
  // reads it, standing only while the scan carried it; the option
  // file's settle arm parses its own value in place, so the entry
  // point parses the command line's text alone (FR-027, R-06).
  std::optional<std::string> jitterProbeText;
  // specs/036 FR-002, FR-005, R-06: the engine the `--os-noise` value
  // settled, empty when the option stood absent. The value rides the
  // batch options beside the probe option, so the missing-window
  // refusal reads it on a batch that names no probe option.
  std::optional<TracerEngine> noiseTracer;
  // specs/036 FR-002, FR-005, R-06: the cpu list the `--os-noise`
  // value settled; an empty vector names the pre-check's candidate
  // set (specs/036 FR-008, R-09).
  std::vector<std::uint32_t> noiseTracerCpus;
  // specs/029 data-model.md E-09, FR-080 to FR-082, D-22: the hunt selection
  // and its settled values, empty when the run carries no hunt.
  std::optional<hunt::HuntOptions> hunt;
  // specs/033-hunter-budgets data-model.md N-08, FR-010, R-07: the path
  // `--hunt-emit-set=` settled, empty when the run writes no promoted set. The
  // The write is a run-level union over every entry's list, so the path rides
  // the run record.
  std::string huntEmitSet;
};

/// @brief The one interrupt flag: SIGINT sets it, the runner reads it
/// after a run completes (R-06).
[[nodiscard]] auto interruptFlag() -> std::atomic<bool>&;

/**
 * @brief Run one instance and produce its result value (E-04, E-07).
 */
class Runner
{
public:
  explicit Runner(const RunOptions& options,
                  std::optional<bool> stampWindows = std::nullopt) noexcept
      : m_options(options)
      ,
      // R-04: the runner stamps the timed windows when the batch
      // carries a probe, the split's one consumer; a per-family probe
      // stamps the same way, and the caller states it (FR-015).
      m_windowStamping(stampWindows.value_or(options.jitterProbe.has_value()))
  {
  }

  [[nodiscard]] auto run(Instance& instance) -> BenchmarkResult;

  /// @brief The timed windows the runs of this runner recorded since the
  /// last take, moved out (R-04, E-06): the window-stamp access the
  /// split reads, behind the join edge the probe's stop gives.
  [[nodiscard]] auto takeRunWindows() -> std::vector<sg::detail::TimedWindow>
  {
    return std::move(m_runWindows);
  }

  /// @brief The complexity fit of the family the runner has been collecting,
  /// when that family ends here: `next` names the instance the run turns to
  /// next, and a null `next` ends the run and the last family with it
  /// (FR-022, FR-023).
  [[nodiscard]] auto fitForFamilyEnd(const Instance* next)
      -> std::optional<BenchmarkResult>;

  /// @brief The complexity fit of `family` out of every instance of it this
  /// runner ran, whatever order the instances ran in, and once only: the
  /// second call for the same family answers nothing (FR-022, FR-023,
  /// specs/028 FR-056).
  [[nodiscard]] auto fitForFamily(RegistryEntry* family)
      -> std::optional<BenchmarkResult>;

  /// @brief The chunk columns the runs of this runner retained, one entry
  /// per row of the result that run handed back, in the order the rows
  /// stand (specs/023 E-01, FR-029).
  [[nodiscard]] auto rowRaw() const noexcept
      -> const std::vector<std::vector<RowRaw>>&
  {
    return m_rowRaw;
  }

  /// @brief The per-context chunk columns of the merged rows, one entry
  /// per context of each row that merged (specs/023 E-01, FR-029).
  [[nodiscard]] auto contextRaw() const noexcept
      -> const std::vector<std::vector<std::vector<RowRaw>>>&
  {
    return m_contextRaw;
  }

private:
  const RunOptions& m_options;

  // FR-022: the instances of one family since its fit was handed out. The
  // sizes and the values of every quantity stand in run order, and a
  // quantity a later instance opens joins the lists with the earlier
  // instances backfilled. specs/028 FR-056: a shuffled run hands the
  // instances of one family in any order and the walk of another family
  // stands between them, so the samples of a family keep their own
  // accumulator. One family holds one record, because the expansion groups
  // the instances of a family in one block and each print path names a
  // family once.
  struct FitSamples
  {
    RegistryEntry* family = nullptr;
    std::optional<TimeUnit> unit;
    std::vector<std::int64_t> sizes;
    std::vector<double> times;
    std::vector<std::string> counterLabels;
    std::vector<std::vector<double>> counterValues;
    std::vector<std::string> metricLabels;
    std::vector<std::vector<double>> metricValues;
  };

  std::vector<FitSamples> m_fits;

  // The family of the last sample, the one `fitForFamilyEnd` compares with
  // the instance the run turns to. An index, because the list of families
  // grows as a family opens.
  std::optional<std::size_t> m_pending;

  // specs/023 E-01: the chunk columns each run retained, one entry per
  // row of that run's result; the aggregate row and every row of a run
  // that captured nothing stand for an empty entry.
  std::vector<std::vector<RowRaw>> m_rowRaw;
  std::vector<std::vector<std::vector<RowRaw>>> m_contextRaw;

  // specs/027 E-06, R-04: the timed windows the current run's attempts
  // stamped, cleared at each run's start and taken by the caller after
  // it; the repetition index the windows name; and the stamping gate the
  // probe option sets.
  std::vector<sg::detail::TimedWindow> m_runWindows;
  std::uint64_t m_repetition = 0;
  bool m_windowStamping = false;

  void noteFitSample(const Instance& instance,
                     const BenchmarkResult& result,
                     std::optional<std::int64_t> statedSize);
  [[nodiscard]] auto accumulatorFor(RegistryEntry* family) -> FitSamples&;
  [[nodiscard]] auto outcomeOf(FitSamples& fit) -> BenchmarkResult;
};

// specs/025 data-model.md E-01, E-10: the internal seams of topology
// placement. `layout()` reads the machine at most once per process and
// caches the result in one function-local static, so an install stands
// before the first read (FR-001, FR-002, FR-006, Q-5).

/// @brief The layout of this process: the installed layout when one
/// stands, the single host read otherwise (FR-001, FR-002, FR-006).
[[nodiscard]] auto layout() -> const MachineLayout&;

/// @brief Install a layout in place of the host read; an empty optional
/// clears the installed one (FR-006, Q-5).
auto installTestLayout(std::optional<MachineLayout>) -> void;

/// @brief Install (or clear, with an empty optional) the online cpu set
/// the placement bound and the option value grammar read, in place of
/// the list the kernel owns, the way `installTestLayout` stands in for
/// the layout read (FR-005, FR-041, Q-5).
auto installTestOnlineCpus(std::optional<std::vector<std::uint32_t>>) -> void;

/// @brief Install (or clear, with an empty optional) the text the
/// online read answers in place of the line the kernel owns, the way
/// `installTestLayout` stands in for the layout read (D-02, FR-021).
/// An installed empty string stands for the unreadable line, the case
/// the affinity-mask fallback owns.
auto installTestOnlineLine(std::optional<std::string>) -> void;

/// @brief The online cpu set the placement bound and the option value
/// grammar read: the installed set when one stands, the list
/// `/sys/devices/system/cpu/online` carries otherwise, and the process
/// affinity mask where that list stands unreadable (D-02, FR-021). The
/// set is consulted at each call, so an install decides the call that
/// follows it (FR-005, FR-041).
[[nodiscard]] auto onlineCpuSet() -> std::vector<std::uint32_t>;

/// @brief Append the cpus one kernel cpu-list line spells: comma-
/// separated lone indices and `low-high` spans in the order the line
/// carries them, a token that parses as no number naming no cpu
/// (FR-005, FR-041).
auto appendOnlineCpuList(std::vector<std::uint32_t>& cpus,
                         std::string_view text) -> void;

/// @brief Install a synthetic description the read hands to
/// `hwloc_topology_set_synthetic`; an empty optional clears it
/// (FR-006, Q-5, R-04).
auto installTestTopologyDescription(std::optional<std::string>) -> void;

/// @brief Install the answer of the load call in place of the hwloc
/// answer; an empty optional restores the real call (review decision
/// D-1, FR-005).
auto installTestLoadFailure(std::optional<int>) -> void;

/// @brief specs/035 FR-016, R-06: install (or clear, with an empty
/// optional) the memory bind seam the run binds memory through, in
/// place of the production binder the one hwloc translation unit
/// carries, the way `installTestLayout` stands in for the layout read.
auto installTestMemoryBinder(std::optional<MemoryBindOps>) -> void;

/// @brief specs/035 FR-016, R-06: the binder the run binds memory
/// through: the installed one when one stands, the production binder
/// otherwise. Consulted at each call, so an install decides the call
/// that follows it (Q-5).
[[nodiscard]] auto memoryBinder() -> const MemoryBindOps&;

/// @brief specs/035 FR-011, R-08: the resolved node plan of the run in
/// flight, the runner installs ahead of the contexts it spawns and
/// clears for every other run. The bind at the context start reads the
/// node it names for the context's cpu, and the disclosure composes the
/// node tail beside it (R-05, R-08).
[[nodiscard]] auto resolvedNodePlan() -> std::optional<PlacementPlan>&;

/// @brief specs/035 FR-013: the one flag a denied start bind sets; the
/// entry point answers it with the status-2 refusal the fit walk
/// prints, and no benchmark row stands ahead of the line.
[[nodiscard]] auto bindRefusalFlag() -> std::atomic<bool>&;

/// @brief specs/035 FR-013, R-07: the node whose bind this context's
/// start recorded as denied, empty when the bind landed or no node
/// plan stood.
[[nodiscard]] auto bindRefusalOf(const ExecutionContext& context) noexcept
    -> std::optional<std::uint32_t>;

/// @brief The preset one text names: the five names match in full and
/// nothing else (FR-007, FR-016, FR-017).
[[nodiscard]] auto parsePreset(std::string_view) -> std::optional<Preset>;

/// @brief The name one preset prints in a message, the table read in
/// enum order (FR-016).
[[nodiscard]] auto presetText(Preset) -> std::string_view;

/// @brief The plan of one preset over one layout for one context count:
/// the ordered walk of the resolver contract, an empty `cpu` when the
/// count exceeds the place count (FR-008 to FR-014, FR-018).
[[nodiscard]] auto resolvePreset(Preset,
                                 const MachineLayout&,
                                 int contexts) -> PlacementPlan;

/**
 * @brief specs/039-context-groups data-model.md: one group of one
 * instance's partition: the name its source spells beside the context
 * positions that start and pace as one unit (FR-001, FR-002, D-4).
 *
 * \pre `name` stands unique within its partition and `slots` holds at
 *      least one position of the instance (A-2)
 * \post the slots of one partition's groups form the set `0` to
 *       `contexts - 1` with no repeat (A-1)
 */
struct ContextGroup
{
  /// @brief the name the source spells: the unit and its ordinal for a
  /// preset group, the entry's own text for a layout group (FR-008,
  /// FR-015, A-6)
  std::string name;
  /// @brief the positions of the instance the group holds, ascending
  /// (FR-002)
  std::vector<std::size_t> slots;
};

/**
 * @brief specs/039-context-groups FR-006 to FR-010, D-3, D-4: the
 * partition the preset names for one instance, read off the plan
 * `resolvePreset` wrote for that preset.
 *
 * The unit is the plan's node list under `node` and its L3 group list
 * under `core`, `l3`, `smt` and `compact`. Each group takes the name of
 * its unit beside the unit's ordinal, and holds its positions in plan
 * order. No path here reads a clock, starts a thread, or opens a file
 * (A-5).
 *
 * \pre `contexts >= 1`, and the unit list the preset reads answers
 *      `contexts` entries, the plan standing from the resolver for this
 *      preset, layout and count (B-1)
 * \post the groups cover the positions once each, in the order the
 *      first position of each group appears, and one plan with one
 *      count answers one partition (A-1, A-4, FR-001, FR-002)
 * \post `Preset::NONE`, one context, and a unit the read left at
 *      `kNoGroup` answer the empty vector, the run keeping the
 *      ungrouped shape (FR-004, FR-006, FR-007, FR-010)
 * \post `plan` keeps the cpu list, the group list, the node list and
 *      the place count the resolver wrote (FR-009)
 */
[[nodiscard]] auto groupPartition(Preset preset,
                                  const PlacementPlan& plan,
                                  std::size_t contexts)
    -> std::vector<ContextGroup>;

/**
 * @brief specs/039-context-groups FR-022, B-2: the index of the group
 * of `partition` that holds one context position.
 *
 * \pre `partition` satisfies A-1 and `position` stands below the count
 *      of positions it covers (B-2)
 * \post the answer stands below `partition.size()`, and the group it
 *      names holds `position` (B-2)
 */
[[nodiscard]] auto groupOfContext(const std::vector<ContextGroup>& partition,
                                  std::size_t position) -> std::size_t;

/**
 * @brief specs/039-context-groups FR-012 to FR-018, FR-021, R-3: the
 * partition one layout file names for one instance.
 *
 * The grammar is the whole vocabulary: one document, one root mapping
 * holding the one key `groups`, and one entry per group carrying an
 * optional `name` beside one `slots` sequence. The reader opens `path`
 * and nothing else: it trims no text, folds no case, expands no path
 * and reads no environment variable (A-4). The vendored reader stands
 * inside `source/harness/group_layout.cpp`, behind the harness target's
 * private link (FR-016).
 *
 * \pre `contexts >= 1`, and `path` stands as the winning option source
 *      spelled it (B-1)
 * \post the value satisfies A-1, A-2, A-6 and FR-002: the groups stand
 *       in document order, each group's slots ascend, and the slots
 *       cover `0` to `contexts - 1` exactly once (B-1)
 * \post an entry that names none answers `g` plus its zero-based
 *       document index (FR-015)
 * \post on the error form: one line with no newline naming the path,
 *       and the entry the fault stands in by its `name` where it has
 *       one and by its index otherwise (A-3, FR-018, FR-021)
 * \post a fault about the key `cpus` names the key and states that this
 *       release groups by slots (FR-014)
 */
[[nodiscard]] auto parseGroupLayout(const std::string& path,
                                    std::size_t contexts)
    -> std::expected<std::vector<ContextGroup>, std::string>;

/**
 * @brief specs/027 data-model.md D-4: the `idle` preset's resolution
 * over one layout (FR-016, FR-017, R-07).
 *
 * \pre `machine` stands readable (FR-020)
 * \post every cpu of `cpus` stands inside one group a planned cpu
 *       names, is the lowest cpu of that group that names no planned
 *       cpu and no SMT sibling of a planned cpu, and the list carries
 *       one cpu per non-skipped touched group in group order (FR-016,
 *       D-4, R-07)
 * \post `skippedGroups` carries exactly the touched groups that name
 *       no free cpu, ascending (FR-017, D-4)
 */
struct IdleResolution
{
  std::vector<std::uint32_t> cpus;
  std::vector<std::uint32_t> skippedGroups;
};

/// @brief The `idle` resolution: the lowest free cpu of each L3 group
/// the planned cpus name, with the skip record (FR-016, FR-017, R-07).
[[nodiscard]] auto resolveIdleCpus(
    const MachineLayout& machine,
    const std::vector<std::uint32_t>& plannedCpus) -> IdleResolution;

/// @brief specs/027 data-model.md D-3: one probe cpu the plans refuse,
/// with the benchmark whose plan it meets (FR-018, D-9).
struct ProbeOverlap
{
  std::uint32_t cpu = 0;
  std::string benchmark;
};

/// @brief The overlap check of the alongside phase: the first probe cpu
/// that names a planned cpu of a selected instance or an SMT sibling of
/// one; the `precheck` mode allows every overlap, because it stops
/// before the benchmarks start (FR-018, D-3, R-10).
///
/// \pre `machine` stands readable (FR-020)
/// \post a `precheck` mode names no overlap, and every other mode
///       names the first probe cpu a plan refuses, if one stands
///       (D-3, R-10)
/// @brief The planned cpus the idle resolution skips: every instance
/// with more than one context and a preset contributes its plan's cpus
/// (D-4, FR-016). An instance with one context contributes nothing, and
/// a multi-context instance the preset names none contributes nothing,
/// its threads running unpinned beside the probe (FR-019, R-10).
/// \pre `machine` stands readable (FR-020)
[[nodiscard]] auto plannedCpusOf(const std::vector<Instance*>& selected,
                                 const MachineLayout& machine)
    -> std::vector<std::uint32_t>;

/// @brief The narrowing rule: the calling thread's mask narrows by the
/// probe cpus for an instance the runner does not pin, beside a live
/// probe that runs beside the benchmark; the precheck mode runs no
/// thread beside one (FR-019, FR-006, R-10).
[[nodiscard]] auto narrowingApplies(const Instance& entry,
                                    bool probeLive,
                                    JitterProbeMode mode) -> bool;

[[nodiscard]] auto probeOverlapOf(const MachineLayout& machine,
                                  const std::vector<Instance*>& selected,
                                  const std::vector<std::uint32_t>& probeCpus,
                                  JitterProbeMode mode)
    -> std::optional<ProbeOverlap>;

/**
 * @brief specs/027 data-model.md D-5: the affinity-mask narrowing of
 * one benchmark that carries no placement (FR-019, FR-036).
 *
 * \post `remaining` carries the mask cpus the probe cpus leave, in the
 *       mask's order, and the empty list is the status-2 condition
 *       (FR-019, D-5)
 * \post `limited` stands when a probe cpu names no mask cpu, the
 *       cpuset limit FR-036 discloses
 */
struct MaskNarrowing
{
  std::vector<std::uint32_t> remaining;
  bool limited = false;
};

/// @brief The mask decision: the mask minus the probe cpus, with the
/// cpuset-limit answer (FR-019, D-5, FR-036).
[[nodiscard]] auto maskNarrowingOf(const std::vector<std::uint32_t>& probeCpus,
                                   const std::vector<std::uint32_t>& maskCpus)
    -> MaskNarrowing;

/// @brief The planned-cpus rule the idle resolution's precondition
/// reads: every planned cpu names a thread the layout's mask holds
/// (R-07).
[[maybe_unused]] [[nodiscard]] auto plannedCpusResolve(
    const MachineLayout& machine,
    const std::vector<std::uint32_t>& plannedCpus) -> bool;

/// @brief The duration-division relation of data-model.md E-03: the
/// derived fields the fold filled answer the covered span, read against
/// the scale of the values (E-03).
[[maybe_unused]] [[nodiscard]] auto durationDivisionsHold(
    const JitterStats& stats) -> bool;

/// @brief The resolution's postcondition predicate, read only inside
/// the SG_ENSURE contract of the idle resolution: the covered groups
/// equal the touched ones across the skip record and the yielded cpus,
/// and every yielded cpu stands the lowest free cpu of its group
/// (FR-016, FR-017, D-4).
[[maybe_unused]] [[nodiscard]] auto idleResolutionValid(
    const MachineLayout& machine,
    const std::vector<std::uint32_t>& plannedCpus,
    const IdleResolution& resolution) -> bool;

/// @brief specs/035 FR-009, FR-012, R-04: the node plan's postcondition
/// predicate, read only inside the SG_ENSURE contract of the resolver:
/// every planned cpu stands inside the mask and stands on the node the
/// context binds to, and the cpus stand pairwise distinct.
[[maybe_unused]] [[nodiscard]] auto planNodesBind(
    const MachineLayout& layout, const PlacementPlan& plan) -> bool;

/// @brief The overlap answer's postcondition predicate, read only
/// inside the SG_ENSURE contract of the overlap check: the answer
/// names a probe cpu, and the precheck mode names none (FR-018, D-9,
/// R-10).
[[maybe_unused]] [[nodiscard]] auto probeOverlapValid(
    const std::vector<std::uint32_t>& probeCpus,
    sg::JitterProbeMode mode,
    const std::optional<ProbeOverlap>& overlap) -> bool;

/// @brief The skipped-groups line the placement disclosure carries
/// (FR-017, D-4, E-09): the empty vector names no line, one group
/// stands alone, and two join on the comma.
[[nodiscard]] auto skippedGroupsText(const std::vector<std::uint32_t>& groups)
    -> std::string;

/// @brief The narrowing's postcondition predicate, read only inside
/// the SG_ENSURE contract of the mask decision: the narrowed mask
/// drops the probe cpus only, and the limit answer follows the mask
/// (FR-019, FR-036).
[[maybe_unused]] [[nodiscard]] auto maskNarrowingValid(
    const std::vector<std::uint32_t>& probeCpus,
    const std::vector<std::uint32_t>& maskCpus,
    const MaskNarrowing& narrowing) -> bool;

/// @brief Print the layout listing `--topology` states (FR-036).
auto printTopologyListing(const MachineLayout&) -> void;

/// @brief The fit walk of the selected instances before the first run:
/// the first value is 2 when a preset stops the batch and 0 when the
/// walk stands clear, and the second states whether a preset places the
/// run (FR-005, FR-018, Q-6, Q-7).
[[nodiscard]] auto placementFitStatus(const std::vector<Instance*>& selected)
    -> std::pair<int, bool>;

/**
 * @brief specs/039-context-groups FR-019 to FR-021, D-2, D-8, B-3: the layout
 * arm of the fit walk: the read of the one file the `--group-layout` row
 * settled, beside the placement walk and ahead of the first instance.
 *
 * The file reads once, at the context count of the first selected instance.
 * Nothing here runs a benchmark, reads a clock, or starts a thread.
 *
 * \pre `selected` stands non-empty, and `path` stands as the winning option
 *      source spelled the row's value, empty when no source named the row. The
 *      placement fit walk stood clear, so an instance that carries a preset
 *      above `none` reads a layout the resolver can walk (B-3)
 * \post the answer is 0 with an empty line when no source named a layout, when
 *       the file's slot count answers every instance's context count, and when
 *       no instance's preset names a partition beside it (FR-019, FR-020)
 * \post the answer is 2 with the one line the entry point prints when the slot
 *       count differs from an instance's count, when the file stands
 *       unreadable, or when a preset names a partition beside the file; no
 *       instance ran (FR-019, FR-020)
 * \post the line names the layout path, and a line about a slot names the
 *       count it failed against (FR-021)
 */
[[nodiscard]] auto groupLayoutFitStatus(const std::string& path,
                                        const std::vector<Instance*>& selected)
    -> std::pair<int, std::string>;

/// @brief The disclosure text of one slot: `cpu@group` when the layout
/// names a group for that cpu, the cpu index alone when it names none,
/// `NO_PIN` for `kNoPin`; rendering only, with the text in storage that
/// outlives the call (FR-021, data-model.md E-09).
[[nodiscard]] auto placementText(const PlacementSlot&) -> std::string_view;

/**
 * @brief Print the context lines above the rows (FR-035).
 *
 * specs/025 FR-019, FR-020: `placementInEffect` states that a selected
 * instance carries a preset other than `none` with a context count
 * above 1; the topology line then stands below the version line, once
 * per report.
 */
auto printContext(bool placementInEffect) -> void;

/**
 * @brief Format one benchmark result in fixed columns (FR-035, FR-036).
 *
 * specs/029 T126, FR-040, E-08, PD-10: the hunt block of a result that
 * carries a hunt prints from the run-level record handed over beside it,
 * so the plan line and the lines that rank read the record of the hunt
 * that built `result.hunt`. The record stands empty for a result that
 * carried no hunt, and such a result prints the bytes it printed before
 * the hunt existed (I-8).
 */
auto printResult(const BenchmarkResult& result,
                 const std::optional<hunt::RunHunt>& run = std::nullopt)
    -> void;

/// @brief Print the console probe block, one column per probe cpu in the
/// E-03 order, and the run-level disclosure lines in the end-of-report
/// region (FR-023, FR-036, R-08, E-09). A false `available` prints the
/// not-available disclosure only.
auto printJitterReport(bool available,
                       const std::vector<const ProbeCpuState*>& states) -> void;

/// @brief specs/034 FR-023, D-8: the pre-check block, printed before the
/// benchmark tables: one block per measured cpu, the host settings
/// once, the warning lines, and the pinning-candidate line. The strict
/// refusal never reaches this printer (FR-014). The tracer lines of
/// specs/036 stand after the host settings and before the warning
/// lines (specs/036 R-14). The record leaves `tracer` empty for a
/// tracer-free run and for a not-available run alike (specs/036 E-17),
/// so `requestedEngine` carries the not-available signal: an engine
/// that names a tracer prints the one disclosure line (specs/036
/// FR-016), and a run that requests no tracer prints no tracer line
/// (specs/036 FR-012).
///
/// \pre none
/// \post none
auto printPrecheckReport(
    const PrecheckRecord& record,
    sg::TracerEngine requestedEngine = sg::TracerEngine::NONE) -> void;
/// @brief specs/029 T035, FR-012, FR-013, data-model E-07: the plan line of
/// one hunt run, and the one line a cut plan adds below it.
///
/// Every figure is a field of the record the caller hands over, the cut
/// reason included: the printer decides no plan shape and spells no reason.
///
/// \pre none
/// \post the plan line of the record stands on stdout, and a budgeted record
///       that holds one stage is followed by the cut line naming
///       `RunHunt::cutReason`
auto printHuntPlan(const hunt::RunHunt& run) -> void;

/// @brief specs/033-hunter-budgets FR-020, FR-023, R-10, R-11, N-06: the
/// canonical clause text of the two budgets, rebuilt from the stored figures.
///
/// The plan line prints these texts and the document writes them, so one
/// builder answers both and the two never state the same budget two ways. The
/// `e` text is the shortest decimal word of the seconds the nanoseconds hold
/// (`500000000` answers `0.5e`); the `g` text is the whole count (`32g`).
///
/// \pre none
/// \post the answer ends in its clause letter, holds no leading zero and no
///       trailing zero in its fraction, and reads again through
///       `parseHuntBudget` to the figure it came from
[[nodiscard]] auto huntSampleClauseText(std::uint64_t budgetNs) -> std::string;

/// @brief specs/033-hunter-budgets FR-020, FR-023, R-10, R-11: the canonical
/// `g` clause text of a multiplex cap.
///
/// \pre none
/// \post the answer is the count and the letter `g`
[[nodiscard]] auto huntCapClauseText(std::size_t cap) -> std::string;

/// @brief Print the catalog listing; false when the listing failed
/// (FR-037).
auto printCatalog() -> void;

/// @brief Print one object's counter lines, then the metric lines whose path
/// is that object's, in table order (FR-021, FR-022, D-11).
auto printObject(const sg::counters::Object& object,
                 const std::vector<sg::counters::MetricListing>& metrics)
    -> void;

/// @brief A metric column of one run: the label, the FR-022 per-iteration
/// decision, the erased spine, and the availability the catalog disclosed
/// before the run (FR-023).
struct MetricColumn
{
  std::string label;
  bool perIteration = false;
  bool unavailable = false;
  sg::counters::Availability availability =
      sg::counters::Availability::COUNTABLE;
  sg::counters::detail::ExprCore core;
  // The sink column of this leaf: its index among the run's cores,
  // which is the order of the fold totals (FR-052).
  std::size_t foldColumn = 0;
};

/// @brief The column one metric-name lookup answers: a resolved view
/// carries its spine across untouched, and a refusal prints its note and
/// stands the column unavailable while the benchmark runs (FR-025,
/// FR-026, FR-027).
[[nodiscard]] auto metricColumnFor(
    const std::string& name,
    const std::expected<sg::counters::MetricView, sg::counters::Error>& view,
    const std::vector<sg::counters::MetricListing>& listed) -> MetricColumn;

/// @brief The note one unavailable metric column prints: the lookup's message
/// with the refusal kind of its row in parentheses, when the listing carries
/// the name as a refusal (FR-027).
[[nodiscard]] auto metricColumnNote(
    const std::string& name,
    const sg::counters::Error& error,
    const std::vector<sg::counters::MetricListing>& listed) -> std::string;

/// @brief The one line `--metric` prints for a name that carries nothing:
/// the name, the lookup's message, and its near-miss names in parentheses
/// when it has any (FR-028, FR-042).
[[nodiscard]] auto metricNameFailureLine(
    const std::string& name, const sg::counters::Error& error) -> std::string;

/**
 * @brief Walk the counter addresses every source named and print the first
 * refusal: the origin of the source that held the address (or the option
 * name, for the command line), the address, and what the host found there.
 * The answer is the usage status, or zero when every address resolves to a
 * leaf this host can count (FR-087).
 *
 * The listing and the catalog answer before this walk, the way the metric
 * names keep them, and a library caller that hands addresses to the runner
 * directly keeps the runner's own absent column (FR-023, FR-087).
 */
[[nodiscard]] auto counterAddressFailure(
    std::span<const CounterAddress> addresses) -> int;

/**
 * @brief The text of one file, for every source that needs a whole
 * file: the report loader, the option file and the rules file
 * (research D-05, FR-029, FR-061).
 *
 * The answer is the text, or one fault line naming the path and the
 * reason the system refused: `the file '<path>' cannot be opened:
 * <reason>` where the open fails, `the file '<path>' cannot be read:
 * <reason>` where the read itself fails. The reader moved out of the
 * anonymous namespace of `json_load.cpp` with both texts byte-identical,
 * so the loader tests keep their expectations (D-05).
 */
[[nodiscard]] auto readTextFile(const std::string& path)
    -> std::expected<std::string, std::string>;

/// @brief The refusal kind of an availability state, as the report and
/// the listing print it (FR-023, FR-037).
[[nodiscard]] auto availabilityName(sg::counters::Availability availability)
    -> const char*;

/// @brief Apply the flags of one repetition to its counter columns, one
/// value per name in the order the names opened (FR-003, FR-005, FR-006).
/// A divisor that stands at zero leaves the flag-neutral value of that
/// column, so the count belongs to the input a caller passes. The run
/// loop leaves one at the least, and the counter test hands this pass a
/// count of zero to reach the case FR-005 discloses.
auto appendCounterColumns(std::vector<double>& printed,
                          const std::vector<std::string>& names,
                          const std::vector<Counter>& store,
                          const double primarySeconds,
                          const std::uint64_t iterations) -> void;

}  // namespace detail

namespace hunt
{

/**
 * @brief specs/029 T126: the pair of records one hunt answered: the entry
 * record a result carries at `BenchmarkResult::hunt`, and the run-level
 * record the printers and `ReportData::hunt` read (E-06, E-08, PD-10).
 * The record travels with the answer, so a printer reads the record of
 * the hunt that built the entry standing beside it.
 */
struct Hunted
{
  /// @brief the ranked rows, the skipped list and the pass figures of the
  /// hunt that ran (E-06)
  EntryHunt entry;
  /// @brief the plan the hunt ran under and the stages that ran (E-08)
  RunHunt run;
};

/**
 * @brief specs/029 D-23: the name-addressable driver behind the public
 * `runHunt`, so the entry point hunts the instances the filter selected
 * without a handle of its own (FR-040).
 *
 * `hunt.cpp` defines it. `timedPass` states which route calls: the
 * harness run path prints the figures of E-07, holds no calibration data
 * of its own, and measures one pass of the plan to price it (T119,
 * FR-012). A hunt that ran answers both records; a hunt that refuses
 * answers the error whose text the caller prints (FR-081).
 */
[[nodiscard]] auto runHuntNamed(
    std::string_view name,
    const HuntOptions& options,
    std::optional<std::uint64_t> calibrationNsPerPass,
    std::optional<MetricMethod> forcedMethod = std::nullopt,
    bool dryRun = false,
    bool timedPass = false) -> std::expected<Hunted, counters::Error>;

}  // namespace hunt

}  // namespace sg

#endif  // SG_HARNESS_INTERNAL_HPP
