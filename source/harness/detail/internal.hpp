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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
};

/// @brief specs/025 data-model.md E-05: the five placement presets,
/// parsed from text that equals one name in full (FR-007, FR-016).
enum class Preset
{
  NONE,
  CORE,
  L3,
  SMT,
  COMPACT,
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
  std::uint32_t places = 0;
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
  RegistryEntry* family = nullptr;

  // specs/029 data-model.md E-04, D-15: the plan of the hunt this instance
  // runs, set by `runHunt` before the run and empty for a run `--hunt` did not
  // select. The pass loop reads it only to tell a hunt run from every other
  // run, so a run with no hunt keeps its present route (PD-11).
  std::optional<hunt::HuntPlan> huntPlan;

  // specs/029 PD-12, D-18: one entry per pass whose group open refused and
  // fell back to one pass per event of that pass.
  std::vector<detail::RetriedPass> retriedHuntPasses;

  // specs/029 D-18, FR-021: the addresses of the events whose own retry pass
  // refused too: the run keeps going and `runHunt` turns each into a skipped
  // entry with the kind of the refusal.
  std::vector<std::string> refusedHuntEvents;

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

  /// @brief specs/029 D-18, FR-021: the addresses of the events that still
  /// refused after their solo retry pass.
  ///
  /// \pre none
  /// \post one address per refused solo retry, in refusal order; empty for a
  ///       run whose retries all seated (D-18)
  [[nodiscard]] auto huntRefusedEvents() const noexcept
      -> const std::vector<std::string>&
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

/// @brief specs/029 data-model.md E-08, FR-031: the record of the hunt this
/// process last ran.
///
/// \pre none
/// \post the record `runHunt` last assembled, and empty in a process that ran
///       no hunt (I-8); `hunt.cpp` owns the storage, the report lane reads the
///       plan fields through this name (D-15)
auto huntRunRecord() -> std::optional<hunt::RunHunt>&;

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
  // specs/029 data-model.md E-09, FR-080 to FR-082, D-22: the hunt selection
  // and its settled values, empty when the run carries no hunt.
  std::optional<hunt::HuntOptions> hunt;
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
  explicit Runner(const RunOptions& options) noexcept
      : m_options(options)
  {
  }

  [[nodiscard]] auto run(Instance& instance) -> BenchmarkResult;

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

/// @brief Install a synthetic description the read hands to
/// `hwloc_topology_set_synthetic`; an empty optional clears it
/// (FR-006, Q-5, R-04).
auto installTestTopologyDescription(std::optional<std::string>) -> void;

/// @brief Install the answer of the load call in place of the hwloc
/// answer; an empty optional restores the real call (review decision
/// D-1, FR-005).
auto installTestLoadFailure(std::optional<int>) -> void;

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

/// @brief Print the layout listing `--topology` states (FR-036).
auto printTopologyListing(const MachineLayout&) -> void;

/// @brief The fit walk of the selected instances before the first run:
/// the first value is 2 when a preset stops the batch and 0 when the
/// walk stands clear, and the second states whether a preset places the
/// run (FR-005, FR-018, Q-6, Q-7).
[[nodiscard]] auto placementFitStatus(const std::vector<Instance*>& selected)
    -> std::pair<int, bool>;

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

/// @brief Format one benchmark result in fixed columns (FR-035, FR-036).
auto printResult(const BenchmarkResult& result) -> void;

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

/// @brief The exit status when a `--metric` name carries nothing: the first
/// failure is stated once, with its near-miss names (FR-028, FR-042).
[[nodiscard]] auto metricNameFailure(const std::vector<std::string>& names)
    -> int;

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

}  // namespace sg

#endif  // SG_HARNESS_INTERNAL_HPP
