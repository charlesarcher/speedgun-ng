#ifndef SG_HARNESS_INTERNAL_HPP
#define SG_HARNESS_INTERNAL_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/execution_context.hpp"

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
  RegistryEntry* family = nullptr;
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
 * \pre every family call of every registration has returned, and no run
 *      has started (FR-003, E-01).
 * \post the instance list holds one instance per argument tuple of
 *       every family record, and each instance carries its name, its
 *       suite, its case, and the arguments it owns (FR-010, R-02).
 * \invariant no run performs expansion work (FR-003).
 */
auto expandRegistry() -> void;

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

private:
  const RunOptions& m_options;

  // FR-022: the instances of one family since its last fit. The sizes and the
  // values of every quantity stand in instance order, and a quantity a later
  // instance opens joins the lists with the earlier instances backfilled.
  RegistryEntry* m_pendingFamily = nullptr;
  std::optional<TimeUnit> m_pendingUnit;
  std::vector<std::int64_t> m_sizes;
  std::vector<double> m_times;
  std::vector<std::string> m_counterLabels;
  std::vector<std::vector<double>> m_counterValues;
  std::vector<std::string> m_metricLabels;
  std::vector<std::vector<double>> m_metricValues;

  void noteFitSample(const Instance& instance,
                     const BenchmarkResult& result,
                     std::optional<std::int64_t> statedSize);
  void clearPending() noexcept;
};

/// @brief Print the context lines above the rows (FR-035).
auto printContext() -> void;

/// @brief Format one benchmark result in fixed columns (FR-035, FR-036).
auto printResult(const BenchmarkResult& result) -> void;

/// @brief Print the catalog listing; false when the listing failed
/// (FR-037).
auto printCatalog() -> void;

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
