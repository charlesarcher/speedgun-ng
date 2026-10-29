#ifndef SG_BENCHMARK_HPP
#define SG_BENCHMARK_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/execution_context.hpp"
#include "speedgun-ng/hunt.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file benchmark.hpp
 * @brief The benchmark harness surface: registration, the state object
 * of the timed loop, the run-control options, the result value, and the
 * entry point every speedgun executable calls.
 *
 * Feature 015-benchmark-harness-core. Namespace `sg`. Every time and
 * counter value the harness measures or reports comes from
 * `sg::counters` (FR-038); this header declares no clock and no
 * platform term (FR-050).
 */

namespace sg
{

namespace detail
{
class Runner;
struct PauseFoldSink;

// FR-052: the fold path's commit. A pause pair the run's recorder
// holds no room for stands in the run's two-point slot instead, and
// this core folds the slot's pair once, at the commit of the pair's
// second point, adding each column's fold to that column's running
// total. The sink's definition and this core's stand in
// source/harness.
SPEEDGUN_NG_EXPORT auto commitPauseFoldPair(
    PauseFoldSink& sink, const counters::RecorderApi& slot) noexcept -> void;
}  // namespace detail

// FR-004 sets the multiplier a range call grows by when the family
// names none (E-05, R-17).
inline constexpr std::int64_t kDefaultRangeMultiplier = 8;

/**
 * @brief The unit the report scales a printed time value to (E-02).
 *
 * A stored time value stays nanoseconds and the report applies the
 * divisor of the benchmark's unit at print time. `NANOSECONDS` carries
 * the label `ns` with the divisor 1, `MICROSECONDS` `us` with 1000,
 * `MILLISECONDS` `ms` with 1000000, `SECONDS` `s` with 1000000000
 * (FR-015, FR-016).
 */
enum class TimeUnit : std::uint8_t
{
  NANOSECONDS,
  MICROSECONDS,
  MILLISECONDS,
  SECONDS
};

/**
 * @brief The outcome of one benchmark within a run (E-07).
 */
enum class RunOutcome : std::uint8_t
{
  MEASURED,
  SKIPPED,
  FAILED
};

/**
 * @brief Whether a result row carries one repetition or one aggregate
 * (E-06).
 */
enum class RowKind : std::uint8_t
{
  REPETITION,
  AGGREGATE,
  // The two kinds a family's complexity fit prints: one row per fitted
  // quantity naming its curve and coefficient, and one row carrying the
  // root-mean relative error of that fit (FR-022, FR-030).
  BIG_O,
  RMS,
  // The row a fitted quantity prints in place of its pair when that quantity
  // does not stand in every instance of the family (FR-023, spec Edge Cases).
  SKIPPED
};

/**
 * @brief The flag bits that turn a counter's stored value into the
 * value its column prints (E-02, FR-002).
 *
 * `NONE` prints the value the body set. `RATE` divides that value by the
 * row's primary time in seconds. `AVG_THREADS` divides by the thread
 * count of the run, one while threads stay out of scope.
 * `ITERATION_INVARIANT` multiplies by the iteration count ahead of a
 * rate division, and `AVG_ITERATIONS` divides by the iteration count.
 * `INVERT` takes the reciprocal last, after every other flag (FR-003,
 * FR-004, research D-02).
 *
 * The bits combine through `operator|`, so a rate that averages over the
 * threads of the run is `CounterFlag::RATE | CounterFlag::AVG_THREADS`,
 * a rate over a per-iteration measure is `CounterFlag::RATE
 * | CounterFlag::ITERATION_INVARIANT`, and the mean of a per-iteration
 * measure is `CounterFlag::AVG_ITERATIONS`. A combined set is one value
 * and the flags apply in the order above; the reference implementation
 * spellings `kAvgThreadsRate`, `kIsIterationInvariantRate` and
 * `kAvgIterationsRate` have no single-name counterpart here (FR-003,
 * FR-004).
 *
 * `INVERT` of a value that settled at zero keeps zero. The reference
 * implementation prints an infinity at that point; a counter column that
 * never prints an infinity is the contract, so the reciprocal is taken
 * only where the value stands apart from zero (FR-005).
 */
enum class CounterFlag : std::uint32_t
{
  NONE = 0,
  RATE = 1u << 0,
  AVG_THREADS = 1u << 1,
  ITERATION_INVARIANT = 1u << 2,
  AVG_ITERATIONS = 1u << 3,
  INVERT = 1u << 31
};

/**
 * @brief Test whether a counter flag set carries a flag (FR-002).
 *
 * \pre none
 * \post none
 */
[[nodiscard]] constexpr auto operator&(
    const CounterFlag left, const CounterFlag right) noexcept -> CounterFlag
{
  return static_cast<CounterFlag>(static_cast<std::uint32_t>(left)
                                  & static_cast<std::uint32_t>(right));
}

/**
 * @brief Combine two counter flag sets (FR-002).
 *
 * \pre none
 * \post none
 */
[[nodiscard]] constexpr auto operator|(
    const CounterFlag left, const CounterFlag right) noexcept -> CounterFlag
{
  return static_cast<CounterFlag>(static_cast<std::uint32_t>(left)
                                  | static_cast<std::uint32_t>(right));
}

/**
 * @brief The suffix a rendered counter value carries (FR-006).
 *
 * `K1000` renders with `k` and `K1024` with `Ki`. The console report
 * prints raw numbers, so the base never divides the value a column
 * prints: a counter that holds 64 prints 64 under either base, and the
 * base is the suffix a human-readable renderer would carry (FR-009,
 * FR-010).
 */
enum class CounterBase : std::uint8_t
{
  K1000,
  K1024
};

/**
 * @brief One named counter: the value the body set, its flags, and the
 * base its column scales by (E-02, FR-001, FR-002).
 *
 * A name a run opens keeps the flags and base of the call that opened
 * it, so a later call for that name carrying a different pair is a
 * precondition violation (FR-001, FR-008).
 */
struct Counter
{
  double value = 0.0;
  CounterFlag flags = CounterFlag::NONE;
  CounterBase base = CounterBase::K1000;
};

// The counter names one benchmark may carry: a further name fails the run
// with a reason stating this cap (FR-001).
inline constexpr std::size_t kMaxUserCounters = 16;

// The bytes a counter name may hold, so the longest name a run may open
// is one shorter, and a longer name fails with a reason stating this
// bound (FR-002).
inline constexpr std::size_t kMaxCounterNameBytes = 32;

/**
 * @brief How a run that carries metric columns collects its events
 * (E-01, FR-023).
 *
 * `AUTO` leaves the choice to the run, which settles it per the
 * resolution table of data-model E-01: `PASSES` wherever a source the run
 * needs publishes a programmable-counter limit, `MULTIPLEX` where none
 * does. `PASSES` names the planner path, one pass per group of columns;
 * `MULTIPLEX` names the single kernel-rotated plan. No fourth value
 * exists, so a caller cannot ask for a method the run cannot settle to
 * (FR-023, FR-024, research D-10).
 *
 * The definition stands above `MetricValue` and `BenchmarkResult`, the two
 * types that publish the settled method (specs/024 E-07).
 */
enum class MetricMethod : std::uint8_t
{
  AUTO,
  PASSES,
  MULTIPLEX
};

/**
 * @brief One metric of one row, with the disclosure beside the value
 * (FR-024).
 *
 * `availability` carries the gap state: a window whose endpoint
 * measured nothing reports `sg::counters::Availability::GAP`, and a
 * leaf the host cannot count keeps its refusal kind (FR-023, FR-025).
 * `pass` names the pass that measured the value, empty when no split
 * named one (specs/024 E-08).
 */
struct MetricValue
{
  double value = 0.0;
  double runningRatio = 1.0;
  bool scaled = false;
  // The field name is the contract name. The type spelling is Availability.
  sg::counters::Availability availability =
      sg::counters::Availability::COUNTABLE;
  // specs/024 E-08, FR-019: the index of the pass that measured this value.
  // The merge of a multi-pass run names one for every metric column; a run
  // that measured one pass leaves the field empty, which is what holds the
  // report to the four tokens master prints (FR-021).
  std::optional<std::size_t> pass;
};

/**
 * @brief The aggregate form of Principle VII over the repetitions that
 * produced a measured value (FR-027, R-05).
 *
 * The standard deviation is the sample form with the `n - 1`
 * denominator, and the coefficient of variation divides it by the mean.
 * `samples` counts the repetitions that contributed, so a repetition
 * whose run carried a gap is visible as an excluded sample (FR-025).
 */
struct Statistics
{
  std::uint64_t samples = 0;
  double mean = 0.0;
  double median = 0.0;
  double sampleStdDev = 0.0;
  double coefficientOfVariation = 0.0;
  double min = 0.0;
  double max = 0.0;
};

/**
 * @brief The unit a custom statistic declares (FR-014).
 *
 * A `TIME` statistic holds nanoseconds like every other stored time value
 * and prints in the unit the row prints in; a `PERCENTAGE` statistic
 * prints unscaled by the unit (FR-014).
 */
enum class StatUnit : std::uint8_t
{
  TIME,
  PERCENTAGE
};

/**
 * @brief The function of a custom statistic (FR-013, FR-015).
 *
 * It receives the primary time per iteration of every clean repetition,
 * the same list the built-in `Statistics` summarizes, in nanoseconds, and
 * writes its value to `result`. Returning true prints that value among the
 * built-in fields of the aggregate row; returning false leaves the
 * statistic out of the row while the run still reports (FR-015).
 *
 * \pre the callable is callable: the list it receives can be empty when
 *      every repetition carried a disclosure (FR-025)
 */
using StatisticFunc =
    std::function<bool(const std::vector<double>& values, double& result)>;

/**
 * @brief One custom statistic of an aggregate row, with the value its
 * function produced (FR-013, FR-014, FR-015).
 */
struct StatisticValue
{
  std::string name;
  StatUnit unit = StatUnit::TIME;
  double value = 0.0;
};

/**
 * @brief The curve class a complexity fit tries or keeps (FR-019, FR-020).
 *
 * `NONE` asks for no fit and stands as the default, so a family that never
 * calls `complexity` prints no fit rows. `AUTOMATIC` tries each curve and
 * keeps the least root-mean relative error, with the constant curve as the
 * fallback (FR-021).
 */
enum class BigO : std::uint8_t
{
  NONE,
  ONE,
  N,
  N_SQUARED,
  N_CUBED,
  LOG_N,
  N_LOG_N,
  AUTOMATIC
};

/**
 * @brief The curve function a family supplies for its complexity fit
 * (FR-020).
 *
 * The harness fits its coefficient over the instances by the same closed form
 * it uses for the built-in curves and prints the row under the name `f(N)`.
 */
using BigOFun = std::function<double(double n)>;

/**
 * @brief One row of a family's complexity fit (FR-022, FR-023).
 *
 * A fitted quantity carries its curve token, its coefficient and its
 * root-mean relative error as a percentage. A quantity that does not stand in
 * every instance carries the reason its pair is absent instead.
 */
struct FitRow
{
  std::string quantity;
  std::string curve;
  double coefficient = 0.0;
  double rmsPercent = 0.0;
  // True for the primary time, whose coefficient the report scales by the
  // unit of the family; a counter or metric coefficient stands in that
  // quantity's value (FR-022).
  bool scalesWithUnit = false;
  std::string reason;
};

/**
 * @brief One reported percentile of one column (specs/021 E-04, FR-013).
 *
 * A value stands available when the run left enough chunk values to answer
 * its point; a value it cannot answer carries `available` false and the
 * report prints the token `not available` in its field. No value is ever
 * interpolated or extrapolated (FR-011, FR-013).
 */
struct PercentileValue
{
  bool available = false;
  double value = 0.0;
};

/**
 * @brief One repetition row or one aggregate row (E-06).
 *
 * A repetition row carries the values of that repetition's measured
 * run. An aggregate row carries `timeStats` and one `metricStats` entry
 * per metric, and appears only when the repetition count exceeds one
 * (FR-027, FR-035).
 */
struct ResultRow
{
  RowKind kind = RowKind::REPETITION;
  std::uint64_t iterations = 0;
  // The real-time column: the netted monotonic fold of the run, or the
  // manual sum when the benchmark supplies its own time (FR-007). The
  // H1 name stands unchanged (FR-018).
  double timePerIterationNs = 0.0;
  // The CPU-time column: the netted fold of the chosen CPU leaf,
  // `machine/thread_cpu` unless the family switched it (FR-001, FR-004).
  double cpuTimePerIterationNs = 0.0;
  // The endpoint samples that run took: two when it paused nothing, two
  // more per pause interval, the folded pairs included (FR-013, FR-052).
  std::uint64_t endpointSamples = 0;
  // True when the run netted any pause pair through the fold path: the
  // report discloses such a row with a token, and a raw row keeps the
  // bytes it prints today (FR-054).
  bool pauseSamplesFolded = false;
  // The availability of the primary fold of this row. The aggregate
  // statistics skip a row whose primary fold reported a gap (FR-002,
  // research D-13).
  sg::counters::Availability timeAvailability =
      sg::counters::Availability::COUNTABLE;
  std::vector<MetricValue> metrics;
  // The counter values of this row in `BenchmarkResult::counterLabels`
  // order, after the runner applied the flags of each name (FR-006).
  std::vector<double> counters;
  // The token the report prints beside every counter column of this row when
  // a counter could not take its flags: `zero-divisor` when the primary time
  // of the row is zero. Empty when the columns stand as computed (FR-005).
  std::string counterDisclosure;
  // The text `State::setLabel` wrote during this row's run, empty when
  // the body wrote none. The report prints the field as a trailing
  // label, and an aggregate carries it only when every repetition
  // agrees (FR-011, FR-012).
  std::string label;
  // The quantity, curve, coefficient and RMS of a fit row, or the reason a
  // fitted quantity stands without a pair (FR-022, FR-023). Only the rows of
  // the synthetic fit result carry this.
  FitRow fit;
  double overheadFloorNs = 0.0;
  // specs/021 FR-010: the percentiles of this row's run, one slot per
  // printed column in the order the row prints its columns: the primary
  // time, the time column the primary is not, then each metric, then each
  // counter. Each slot holds one value per point of
  // `BenchmarkResult::percentilePoints`, in that order. Empty when the run
  // reports no percentile (FR-023).
  std::vector<std::vector<PercentileValue>> percentiles;
  // specs/021 FR-002, FR-007: the chunk count of the run and the iteration
  // count one chunk holds. The report discloses the pair beside the
  // percentile lines; a run that reports no percentile leaves both zero.
  std::uint64_t chunkCount = 0;
  std::uint64_t chunkIterations = 0;
  Statistics timeStats;
  std::vector<Statistics> metricStats;
};

/**
 * @brief The policy disclosure of one context of a threaded run (E-07,
 * FR-025).
 *
 * The runner fills one record per context of a threaded instance from
 * that context's fixed policy when the instance joins its contexts, and
 * the report prints the pair after the rows of the result (FR-025). A
 * record is the policy the context was created with: a policy is fixed
 * at creation and readable for the context's life (FR-007), so the pair
 * names what that context ran under. `placement` holds the printable
 * placement, `NO_PIN` or one cpu index, the description the installed
 * placement function returned for that context's thread (FR-005,
 * FR-025).
 */
struct ContextDisclosure
{
  WaitMode waitMode = WaitMode::BLOCK;
  std::string placement;
  /// @brief Whether the installed placement function reported its slot as
  /// landed (FR-005). The report prints it beside `placement`, so a pin that
  /// did not land shows whatever text its function returned (FR-025).
  bool applied = true;
};

/**
 * @brief The C++ value the harness exposes for one benchmark; the
 * console report formats it and adds nothing to it (FR-036, E-07).
 *
 * The suite and case pair splits the instance name the way
 * `contracts/result-fields.md` derives it, and a later JSON report of
 * the roadmap reuses the pair (FR-021, R-07). `ResultRow` carries
 * neither field: it is one repetition or aggregate (FR-021).
 */
/**
 * @brief The jitter result of one probe cpu over one set of covered
 * windows: the sixteen fields of data-model.md E-03 in the E-03 order
 * (FR-002, R-05).
 *
 * The percentile fields follow the nearest-rank rule of
 * `source/harness/percentiles.cpp` over the sorted kept gaps (PC-4),
 * the rule the report columns fill through. The fold of an empty kept
 * set leaves every field at 0. The fold functions of the probe carry
 * the field relations this struct's fills establish; the aggregate
 * itself imposes nothing at construction.
 *
 * \pre none
 * \post none
 */
struct JitterStats
{
  std::uint64_t gapCount = 0;
  double gapsPerSecond = 0.0;
  std::uint64_t minNs = 0;
  std::uint64_t medianNs = 0;
  double meanNs = 0.0;
  std::uint64_t p90Ns = 0;
  std::uint64_t p99Ns = 0;
  std::uint64_t p999Ns = 0;
  std::uint64_t p9999Ns = 0;
  std::uint64_t maxNs = 0;
  std::uint64_t totalNs = 0;
  double totalPercent = 0.0;
  std::uint64_t runtimeNs = 0;
  std::uint64_t thresholdNs = 0;
  std::uint64_t loopPeriodNs = 0;
  std::uint64_t droppedGaps = 0;
};

/**
 * @brief The attribution figures one pre-check measured cpu discloses,
 * beside its gap figures: the window delta of the interrupt count, one
 * delta per softirq kind the host states, and the context-switch and
 * schedule counts (specs/034 FR-016, D-5). A figure the host does not
 * state, states unreadable, or states unparsable stands `not
 * available`, carried as an absent optional, and the run never fails
 * on it (FR-018).
 */
struct PrecheckAttribution
{
  std::optional<std::uint64_t> interrupts;
  std::vector<std::pair<std::string, std::optional<std::uint64_t>>> softirqs;
  std::optional<std::uint64_t> switches;
  std::optional<std::uint64_t> schedules;
};

/**
 * @brief The host facts one run discloses once (specs/034 FR-017,
 * D-6): the kernel command line's `isolcpus`, `nohz_full` and
 * `rcu_nocbs` values, the cpufreq governor of each measured cpu, and
 * the IRQ numbers whose affinity names each measured cpu. A figure
 * the host does not state stands `not available`.
 */
struct PrecheckHostSettings
{
  std::optional<std::string> isolcpus;
  std::optional<std::string> nohzFull;
  std::optional<std::string> rcuNocbs;
  std::vector<std::pair<std::uint32_t, std::optional<std::string>>> governors;
  std::vector<std::pair<std::uint32_t, std::vector<std::uint32_t>>> irqAffinity;
};

/**
 * @brief One measured cpu of the pre-check: its index, whether the
 * run plans it (the planned cpus stand the run's own; the candidates
 * stand the cpus the `idle` preset would take), the sixteen-field
 * stats of its pre-check window, and its attribution deltas
 * (specs/034 FR-010, FR-012).
 */
struct PrecheckCpuRecord
{
  std::uint32_t cpu = 0;
  bool planned = false;
  JitterStats stats {};
  PrecheckAttribution attribution;
};

/**
 * @brief One measured cpu of the tracer: its index and the five
 * noise-by-source counts the engine's count files state (specs/036
 * FR-010, data-model.md E-17). A figure the source states nothing for
 * stands absent, the file form of the console's `not available`
 * wording (specs/036 FR-013).
 *
 * \pre none
 * \post none
 */
struct PrecheckTracerCpuRecord
{
  std::uint32_t cpu = 0;
  std::optional<std::uint64_t> nmi;
  std::optional<std::uint64_t> irq;
  std::optional<std::uint64_t> softirq;
  std::optional<std::uint64_t> thread;
  std::optional<std::uint64_t> hardware;
};

/**
 * @brief The tracer record one run carries: the engine word the
 * value named, and one entry per tracer cpu ascending (specs/036
 * FR-014, data-model.md E-17). The entry names its cpu, so the cpus
 * the tracer ran on stand the array itself (specs/036 D-3).
 *
 * \pre none
 * \post none
 */
struct PrecheckTracerRecord
{
  std::string engine;
  std::vector<PrecheckTracerCpuRecord> cpus;
};

/**
 * @brief The pre-check record one run carries: the window shape, one
 * entry per measured cpu ascending, the host settings, and the
 * skipped groups the candidate resolution left (specs/034 FR-012,
 * FR-016, FR-017, D-8). The optional `tracer` stands when the tracer
 * ran; a run that requests no tracer and a run whose tracer stands
 * not available leave it empty (specs/036 FR-015, R-10). The document
 * carries it under the top-level `precheck` key, and `loadReport`
 * reads it back (FR-024).
 */
struct PrecheckRecord
{
  bool serial = false;
  std::uint32_t windowMs = 0;
  std::vector<PrecheckCpuRecord> cpus;
  PrecheckHostSettings host;
  std::vector<std::uint32_t> skippedGroups;
  std::optional<PrecheckTracerRecord> tracer;
};

/**
 * @brief One probe cpu's fold over one benchmark's timed windows
 * (E-08): the cpu index and the fold the split placed into them. A
 * probe cpu whose kept gaps landed inside no window of the benchmark
 * carries no entry here (E-06, FR-021). The split reads the probe's
 * buffers only after the probe contexts are joined (R-04).
 *
 * \pre none
 * \post none
 */
struct JitterEntryFold
{
  std::uint32_t cpu = 0;
  JitterStats stats {};
};

struct BenchmarkResult
{
  std::string name;
  // The family name up to its first '/', the fixture class name for a
  // fixture instance (FR-021, R-08).
  std::string suite;
  // The instance name with the leading suite and its '/' removed; an
  // instance name equal to its suite keeps that name (`case` is a C++
  // keyword, N-12) (FR-021, R-08).
  std::string caseName;
  RunOutcome outcome = RunOutcome::MEASURED;
  std::vector<ResultRow> rows;
  std::string reason;
  // The column label of every metric position of `rows`, in the order
  // the metrics were attached. The report prints a metric column under
  // this label (FR-026); the values stay in MetricValue (E-05).
  std::vector<std::string> metricLabels;
  // The label of every counter column of `rows`, in the order the names
  // were first opened. The report prints a counter column under this
  // label (FR-006, E-07).
  std::vector<std::string> counterLabels;
  // The aggregate form of each named counter over the clean
  // repetitions, one entry per `counterLabels` entry (FR-006, E-07).
  std::vector<Statistics> counterStats;
  // The value of every statistic the family attached, in `addStatistic`
  // order over the clean primary values; a statistic whose function refused
  // its input holds no entry (FR-013, FR-014, FR-015). The aggregate row
  // prints them, so a run with no aggregate row prints none.
  std::vector<StatisticValue> statisticValues;
  // True when the family asked for its aggregate alone: the report
  // prints the aggregate row and holds back the repetition rows this
  // result carries, a result with no aggregate row keeps its rows
  // (FR-016, FR-017, FR-018).
  bool hideRepetitionRows = false;
  // The unit the report scales this result's printed time values to:
  // the `unit` call when the family made one, else the batch snapshot
  // `speedgunMain` took of the process default at its entry, else the
  // process default read at this run's start (E-02, FR-015, FR-027).
  // The stored row values stay nanoseconds; only the printed scale
  // follows this field.
  TimeUnit timeUnit = TimeUnit::NANOSECONDS;
  // True when a measured repetition's run folded a pause pair (E-06,
  // FR-054).
  bool pauseSamplesFolded = false;
  // The policy disclosure of the run: one record per context a threaded
  // instance ran, empty for a single-context run, so a non-empty vector
  // names exactly a threaded result and the report prints its disclosure
  // line (FR-021, FR-025, E-07).
  std::vector<ContextDisclosure> contexts;
  // specs/021 FR-024: the percentile points this result reports, ascending
  // with duplicates collapsed, the order every `percentiles` entry follows.
  // Empty when the run reports no percentile (FR-023).
  std::vector<double> percentilePoints;
  // specs/024 E-07, FR-017: the method this run settled to, one of
  // `PASSES` or `MULTIPLEX` once the run carries a metric column, and
  // `AUTO` while it does not (E-01, I-6). The merge of the passes names
  // `PASSES`; the console prints the token beside it (FR-020).
  MetricMethod metricMethod = MetricMethod::AUTO;
  // specs/024 E-07, FR-017: the number of passes the run measured. Zero
  // means the pass mechanism did not run, which is the reading a settling
  // of `AUTO` leaves; the `multiplex` settling runs the one plan holding
  // every PMU event, so it discloses one.
  std::size_t passCount = 0;
  // specs/024 E-06, FR-017: the relative spread of the reference time per
  // iteration across the passes, as a percentage of their median and
  // unrounded, the print being the one rounding (research D-08). Empty when
  // fewer than two passes ran or their median is zero, the case the report
  // prints as `spread=not available`.
  std::optional<double> passSpread;
  // specs/024 FR-021, clarification Q2, research D-10: true when the caller
  // forced the collection method, `--metric-method=` on the command line or
  // the `metricMethod` selector on the family. The settled method cannot
  // carry this record: a forced one-pass `passes` run and an identical
  // `auto` one settle to the same method and count yet only the forced one
  // prints the disclosure line, so the runner fills it and the report reads
  // it.
  bool metricMethodForced = false;
  // specs/027 E-08, FR-027: one entry per probe cpu whose kept gaps
  // landed inside this benchmark's windows, in ascending cpu order, the
  // value the per-benchmark attribution line names and the document's
  // per-entry `jitter` object writes; empty when no kept gap landed
  // inside the windows or the run carried no probe (E-08, FR-027).
  std::vector<JitterEntryFold> jitterFolds;
  // specs/021 FR-001, FR-010: true when the primary time of this result is
  // the CPU fold, the default, and false when the family asked for the real
  // fold. The row reports both time columns, so the percentile column that
  // follows the primary answers the one the primary is not, and the report
  // labels it with that field's name (SC-001).
  bool primaryIsCpu = true;
  // specs/021 FR-014: for a threaded result, one entry per context in
  // context order, each holding that context's own percentiles in the
  // column order of the merged row. Empty for a single-context run.
  std::vector<std::vector<std::vector<PercentileValue>>> contextPercentiles;
  // specs/029 data-model.md E-06, FR-031: the hunt record of this benchmark
  // (PD-10). It stands empty for a run that carried no hunt, which is the
  // fact the printer, the writer and the reader test (I-8).
  std::optional<hunt::EntryHunt> hunt;
};

/**
 * @brief The chunk content a JSON report carries (specs/023 E-01,
 * FR-029).
 *
 * `NONE` writes the ranks the run computed and no chunk value; `CHUNKS`
 * writes the chunk values themselves beside them. `CHUNKS` also turns
 * the chunk capture on for a run whose percentile output stands off, so
 * the file can hold values the console never ranked (FR-020). `GAPS`
 * writes the kept gap records of the jitter probe beside the stats, and
 * no chunk value (specs/027 FR-028).
 */
enum class RawContent : std::uint8_t
{
  NONE,
  CHUNKS,
  GAPS
};

/**
 * @brief The retained chunk columns of one row or one context
 * (specs/023 E-02).
 *
 * One entry per reported column, in the column order the row prints:
 * the primary time, then the time column the primary is not, then each
 * metric column, then each user counter column. Each inner vector holds
 * that column's per-iteration chunk values in chunk order, the same
 * numbers the percentile ranks are computed from and no further
 * scaling. A column the run could not fold carries an empty inner
 * vector, and every non-empty one holds `ResultRow::chunkCount` values
 * (specs/023 E-02).
 */
struct RowRaw
{
  std::vector<std::vector<double>> columns;
};

/**
 * @brief The noise warning of one result, in the form the document
 * carries (specs/023 E-10, specs/026 E-06, FR-070, D-05).
 *
 * The three fields are the account `printNoiseWarning` reaches on its way
 * to its line, so the document and the console answer from one predicate
 * (FR-017, D-11). The record is data and stands in the public header;
 * the predicate `noiseWarningOf` stays internal to the harness.
 */
struct NoiseWarning
{
  std::uint64_t spinning = 0;
  std::uint64_t runnableCpus = 0;
  bool sharesOneCpu = false;
};

/**
 * @brief The value the harness writes as one JSON report and reads back
 * (specs/023 E-01).
 *
 * `results` holds one entry per benchmark in run order, the order the
 * console prints them. `rowRaw[i][k]` carries the chunk columns of row
 * `k` of `results[i]`, and `contextRaw[i][k][j]` those of context `j` of
 * that row, so both answer a threaded run whose every repetition
 * captured its own chunks. Both stand empty when the run retained no
 * chunk values, which is what the raw setting at `NONE` leaves behind.
 * Each carries one outer entry per entry of `results`, so a reader
 * indexes by result with no search (specs/023 E-01); `loadReport`
 * answers that pairing as its postcondition.
 */
struct ReportData
{
  std::vector<BenchmarkResult> results;
  std::vector<std::vector<RowRaw>> rowRaw;
  std::vector<std::vector<std::vector<RowRaw>>> contextRaw;

  // The document texts the load call read (specs/026 E-07, D-04): the
  // `schemaVersion` key, and the text under the `speedgun` header at
  // `version`, each empty for a value no document produced. The report
  // writer ignores both and stamps its own, so the 023 byte-identical
  // round trip stands; the run path leaves them empty.
  std::string schemaVersion;
  std::string speedgunVersion;
  // One entry per entry of `results` when the load call fills it; the
  // entry holds nothing when the document carried no warning for that
  // result (specs/026 E-07, FR-070).
  std::vector<std::optional<NoiseWarning>> noiseWarnings;
  // specs/027 E-07, FR-031: the run-level jitter stats the document
  // carried, one entry per probe cpu. The load call fills it when the
  // document carried the root `jitter` object; absent stands for a
  // document that carries no probe. The run path never fills it and
  // never reads it: the value a `jitter:run:` comparison rule reads.
  std::optional<std::vector<JitterEntryFold>> runJitterStats;
  // specs/029 data-model.md E-08, FR-031: the hunt record of the run (PD-10).
  // It stands empty when no benchmark of the document carried a hunt, the
  // reading a run with no `--hunt` leaves behind (I-8).
  std::optional<hunt::RunHunt> hunt;
  // specs/034 FR-024: the pre-check record the document carried, one
  // entry per measured cpu with its role, its stats and its
  // attribution deltas. The load call fills it when the document
  // carried the root `precheck` object; absent stands for a document
  // that carries no pre-check, the reading a run without the
  // pre-check leaves behind.
  std::optional<PrecheckRecord> precheck;
};

/**
 * @brief The outcome word of one benchmark under one comparison rule
 * (specs/026 E-04, FR-054).
 *
 * The seven words the comparison table prints and the comparison
 * document writes; the document writes the enumerator name.
 */
enum class CompareState : std::uint8_t
{
  OK,
  REGRESSED,
  IMPROVED,
  NOISY,
  ADDED,
  REMOVED,
  SKIPPED
};

/**
 * @brief The statistics record a comparison rule reads (specs/026 E-05,
 * FR-016).
 *
 * The default when a statistics name carries no `@` suffix is MEDIAN.
 */
enum class Aggregate : std::uint8_t
{
  MEDIAN,
  MEAN,
  MIN,
  MAX
};

/**
 * @brief Which way of moving a value is worse (specs/026 E-05, FR-012).
 */
enum class Direction : std::uint8_t
{
  LOWER_IS_BETTER,
  HIGHER_IS_BETTER
};

/**
 * @brief One parsed comparison rule (specs/026 E-01, D-01, D-08).
 *
 * The value the rule parse answers and the comparison consumes.
 * `text` is the exact text the parse call read, the form the table, the
 * fault lines and the document print (D-08); `valueName` is the value
 * name as written; `aggregate` stands empty when the text wrote no `@`
 * suffix, and a written suffix on a `p<point>` or `stat:` name is a
 * parse fault (spec D-21); `relativeThreshold` is the fraction the
 * percent spelled, `5%` answering 0.05, empty when the text wrote none;
 * `absoluteThreshold` is the number the `abs=` marker spelled in the
 * stored unit of the value (spec D-22), empty when the text wrote none;
 * `filter` is the rule's filter text, empty when the rule wrote none.
 *
 * At least one of the two thresholds stands in every parsed rule: a
 * rule with neither is a parse fault (FR-022, FR-023).
 */
struct CompareRule
{
  std::string text;
  std::string valueName;
  std::optional<Aggregate> aggregate;
  Direction direction = Direction::LOWER_IS_BETTER;
  std::optional<double> relativeThreshold;
  std::optional<double> absoluteThreshold;
  std::string filter;
};

/**
 * @brief One benchmark under one comparison rule (specs/026 E-02,
 * FR-002, FR-044, FR-054).
 *
 * The table row and the document entry. `name` is the full instance
 * name; `ruleText` is the `text` of the rule that produced the entry;
 * `baseline` and `contender` are the two values in the stored unit,
 * empty for ADDED, REMOVED and SKIPPED; `changePercent` is the signed
 * change as a percent of the baseline magnitude, empty when that
 * magnitude is zero or no value stands; `changeAbsolute` is the signed
 * change in the stored unit, empty when no value stands, and positive
 * when the contender is worse for the rule's direction, so the word and
 * the sign agree; `reason` stands empty except on NOISY, where it names
 * the bound, the carried warning or the running ratio that fired
 * (FR-044).
 */
struct CompareEntry
{
  std::string name;
  std::string ruleText;
  std::optional<double> baseline;
  std::optional<double> contender;
  std::optional<double> changePercent;
  std::optional<double> changeAbsolute;
  CompareState state = CompareState::OK;
  std::string reason;
};

/**
 * @brief The comparison table and its verdict (specs/026 E-03, FR-002,
 * FR-044, FR-045).
 *
 * `entries` holds one entry per benchmark per rule, in baseline order,
 * then the names the contender adds in contender order, and a rule's filter
 * decides which benchmarks that rule admits (FR-002, FR-024).
 * `passed` is false when any entry is REGRESSED, or when the strict
 * setting stands and any entry is NOISY; NOISY passes while strict
 * stands off (FR-044, FR-045).
 */
struct CompareResult
{
  std::vector<CompareEntry> entries;
  bool passed = true;
};

/**
 * @brief The settings one comparison applies (specs/026 FR-045, FR-064).
 *
 * `strict` decides what a `NOISY` row does to the verdict: the row
 * fails the run while the setting stands and passes while it stays off,
 * and the row keeps its word either way (FR-044, FR-045). The settings
 * stand in one struct, so a later setting grows the call with the
 * exported signature unchanged.
 */
struct CompareOptions
{
  bool strict = false;
};

/**
 * @brief The object the harness hands the benchmark function (E-03).
 *
 * The function runs its setup, the timed loop as `for (auto _ : state)`,
 * and its teardown; setup and teardown sit outside the loop, in the
 * untimed region (FR-004, FR-005). The sampling window opens in
 * `begin()` and closes when the loop runs out, so the window covers the
 * loop and not the setup or the teardown (FR-017). The loop cursor
 * counts down from the iteration count of the current run, so it cannot
 * pass that count by construction, and the loop reads no interrupt
 * flag: the runner reads that flag after a run completes (R-06).
 */
class SPEEDGUN_NG_EXPORT State
{
public:
  /**
   * @brief The range-for cursor of the timed loop (E-03).
   *
   * The cursor holds the count of the iterations that remain in the run
   * and only decrements it.
   */
  class Cursor
  {
  public:
    /**
     * @brief True while the loop still runs: the local count of the
     * remaining iterations has not reached zero.
     *
     * The fast path reads that one local count and does no other work.
     * The count starts at the run's iteration count and only
     * decrements, so it reaches zero and no lower: no check guards it.
     * At zero the cursor closes the run's sampling window once, on the
     * path that leaves the loop, and returns false (FR-017).
     *
     * The timed loop holds no contract check, so this comparison states
     * no condition of its own: the count-down construction of the
     * cursor, and the single close of the window in `closeWindow`, are
     * what make the result what it is.
     *
     * \pre none
     * \post none
     */
    [[nodiscard]] auto operator!=([[maybe_unused]] const Cursor& other) noexcept
        -> bool
    {
      if (m_remaining != 0) {
        return true;
      }
      if (m_state != nullptr) [[unlikely]] {
        if (m_runRemaining != 0) {
          // The sample closes the chunk that ran and opens the next, and
          // the refill is the one write this path performs.
          const ChunkRun filled =
              refillRun(m_state, m_runRemaining, m_chunkIterations);
          m_remaining = filled.remaining;
          m_runRemaining = filled.runRemaining;
          return true;
        }
        m_state->closeWindow();
      }
      return false;
    }

    /**
     * @brief Advances the cursor by one iteration.
     *
     * The decrement is unconditional: the loop body holds no contract
     * check, so the pre and post conditions of this step are stated
     * here and guarded by the count-down construction of the cursor.
     *
     * \pre none
     * \post none
     */
    auto operator++() noexcept -> Cursor&
    {
      m_remaining = m_remaining - 1;
      return *this;
    }

    /**
     * @brief The iteration index of the current loop pass.
     *
     * The index counts the iterations the run completed: the ones the
     * finished chunks hold and the ones the current chunk has run.
     * Without chunking the run count stands zero and the read is the
     * difference it has always been.
     *
     * \pre none
     * \post none
     */
    [[nodiscard]] auto operator*() const noexcept -> std::uint64_t
    {
      return m_start - m_runRemaining - m_remaining;
    }

  private:
    friend class State;

    // specs/021 FR-001, D-01: the chunk boundary lives apart from the
    // per-iteration comparison, and it reaches the cursor's counts by value
    // so that no call holds the cursor itself: a call that held it would
    // leave the compiler reloading the iteration count from memory on every
    // iteration (FR-004, the codegen-shape gate of specs/015 IF-04).
    struct ChunkRun
    {
      std::uint64_t remaining;
      std::uint64_t runRemaining;
    };

    [[gnu::noinline]] static auto refillRun(
        State* state,
        std::uint64_t runRemaining,
        std::uint64_t chunkIterations) noexcept -> ChunkRun
    {
      state->takeChunkSample();
      const std::uint64_t take =
          runRemaining < chunkIterations ? runRemaining : chunkIterations;
      return {take, runRemaining - take};
    }

    Cursor(State& state, const std::uint64_t remaining) noexcept
        : m_state(&state)
        , m_start(remaining)
        , m_remaining(remaining)
    {
    }

    explicit Cursor(const std::uint64_t remaining) noexcept
        : m_state(nullptr)
        , m_start(remaining)
        , m_remaining(remaining)
    {
    }

    State* m_state;
    std::uint64_t m_start;
    std::uint64_t m_remaining;
    // specs/021 FR-001: the iterations the run still holds beyond the
    // current chunk, and the size one chunk refills to. Both stand zero for
    // a run that chunks nothing, and the fast path of `operator!=` reads
    // neither.
    std::uint64_t m_runRemaining = 0;
    std::uint64_t m_chunkIterations = 0;
  };

  /**
   * @brief The iteration count of the current run (FR-005).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto iterations() const noexcept -> std::uint64_t
  {
    return m_iterations;
  }

  /**
   * @brief The cursor at the first iteration of the timed loop.
   *
   * The skip flag is read here, once for the loop. A skip requested
   * before the loop leaves the loop with no pass at all and takes no
   * sample. Otherwise this is where the run's sampling window opens,
   * before the first pass of the loop and after the setup the function
   * already ran (FR-005, FR-017).
   *
   * \pre this state belongs to a run: a callback state opens no
   *      sampling window, so opening one here is a precondition
   *      violation (R-05)
   * \post the returned cursor names iteration zero of this run with
   *       the entry sample taken, or an empty cursor when the function
   *       already requested a skip
   */
  auto begin() noexcept -> Cursor
  {
    SG_REQUIRE(!m_callbackState,
               "begin() in a callback state violates the callback-state rule "
               "that the callback state opens no sampling window (R-05)");
    if (!openRun()) [[unlikely]] {
      return Cursor {0};
    }
    Cursor cursor {*this, m_iterations};
    // specs/021 FR-001: the chunk cadence, when the runner set one. The
    // first chunk starts the run and the rest follow at the boundary in
    // `operator!=`; a run with no chunking keeps the cursor it had.
    if (m_chunkIterations != 0 && m_iterations > m_chunkIterations) {
      cursor.m_remaining = m_chunkIterations;
      cursor.m_runRemaining = m_iterations - m_chunkIterations;
      cursor.m_chunkIterations = m_chunkIterations;
      m_chunkPointsLeft =
          (m_iterations + m_chunkIterations - 1) / m_chunkIterations;
    } else {
      m_chunkPointsLeft = 1;
    }
    SG_ENSURE(m_windowOpened || m_recorder == nullptr,
              "the loop entry takes the entry sample (FR-017)");
    return cursor;
  }

  /**
   * @brief The cursor one past the last iteration of the timed loop.
   *
   * The end cursor owns no sampling: the window closes on the exit of
   * the loop, in the cursor `begin()` returns.
   *
   * \pre none
   * \post none
   */
  static auto end() noexcept -> Cursor { return Cursor {0}; }

  /**
   * @brief Skip the benchmark with an error reason (FR-031).
   *
   * A skip inside the loop closes the sampling window at once, so the
   * measured window stops where the function stops, the way the
   * upstream timer stops its clock (FR-017).
   *
   * \pre this state belongs to a run: a skip in a callback state is a
   *      precondition violation (R-05)
   * \post the reason is recorded, the function leaves the timed loop
   *       with break or return, and no statistics print for this
   *       benchmark
   */
  auto skipWithError(std::string_view reason) noexcept -> void
  {
    SG_REQUIRE(!m_callbackState,
               "skipWithError in a callback state violates the callback-state "
               "rule that the callback state owns no run to skip (R-05)");
    m_skipRequested = true;
    m_outcome = RunOutcome::SKIPPED;
    m_reason.assign(reason);
    takeExitSample();
    SG_ENSURE(m_outcome == RunOutcome::SKIPPED,
              "a skip records the skipped outcome (FR-031)");
  }

  /**
   * @brief Skip the benchmark with a message reason (FR-031).
   *
   * A skip inside the loop closes the sampling window at once, as the
   * error skip does (FR-017).
   *
   * \pre this state belongs to a run: a skip in a callback state is a
   *      precondition violation (R-05)
   * \post the reason is recorded, the function leaves the timed loop
   *       with break or return, and no statistics print for this
   *       benchmark
   */
  auto skipWithMessage(std::string_view reason) noexcept -> void
  {
    SG_REQUIRE(!m_callbackState,
               "skipWithMessage in a callback state violates the callback-state "
               "rule that the callback state owns no run to skip (R-05)");
    m_skipRequested = true;
    m_outcome = RunOutcome::SKIPPED;
    m_reason.assign(reason);
    takeExitSample();
    SG_ENSURE(m_outcome == RunOutcome::SKIPPED,
              "a skip records the skipped outcome (FR-031)");
  }

  /**
   * @brief Add one iteration time to the manual accumulator (FR-007).
   *
   * A manual benchmark supplies its own time: this call takes one
   * nanosecond count per iteration and the harness records the sum of
   * the accepted values by summation alone, with no clock behind it.
   * The real column of the run shows that sum divided by the iteration
   * count (FR-006, FR-007, D-05). The call stands in the benchmark
   * body, inside the timed loop or outside it, and the run state owns
   * the accumulator.
   *
   * \pre this state belongs to a run: a callback state owns no run to
   *      accumulate for, so accepting a value there is a precondition
   *      violation (R-05, D-15)
   * \pre the count is at least zero: a duration has no negative
   *      nanoseconds, and a negative value is accepted nowhere without
   *      naming its caller (FR-007)
   * \post the manual accumulator carries the sum of every accepted
   *       value, this one included (FR-007)
   */
  auto setIterationTime(std::int64_t nanoseconds) -> void;

  /**
   * @brief Set the value, flags, and base of one named counter
   * (FR-001, FR-002).
   *
   * The call stands in the timed loop or in a body callback state of the
   * run (FR-001). A name the run has not opened takes the flags and the
   * base of this call and keeps them for the run's life, and a later
   * call for that name writes the value alone (FR-001, FR-008). The
   * runner applies the flags once per repetition after the loop, and the
   * report prints one column per name in the order the names opened
   * (FR-003, FR-006). The name copies into fixed storage, so opening a
   * name allocates nothing (FR-007).
   *
   * A second call for a used name with another flags value or another
   * base fails the run with a reason naming that name: the reason is a
   * runtime text, and an `SG_REQUIRE` message is a literal, so the state
   * records the failure and the run ends with it (FR-008).
   *
   * \pre `name` holds at most `kMaxCounterNameBytes` minus one bytes, a
   *      free slot stands when `name` is new, and a used `name` carries
   *      the flags and the base it opened with (FR-001, FR-002, FR-008)
   * \post the run carries `name` with the value of the argument, and a
   *       used name keeps the flags and base it opened with (FR-001)
   */
  auto setCounter(std::string_view name, const Counter& counter) -> void;

  /**
   * @brief Set the byte-throughput column to the count this call passes
   * (FR-009).
   *
   * The call takes the total bytes the run did, so a body that works a
   * known amount per iteration passes that amount times the iteration
   * count after the loop (FR-009). The call opens `bytes_per_second`
   * with the `RATE` flag and the `K1024` base, so the printed column is
   * this count divided by the primary time of the row in seconds; the
   * base carries the `Ki` suffix a rendered value would show and divides
   * nothing further (FR-003, FR-006,
   * FR-009). A later call writes the count alone, as for any used name
   * (FR-001).
   *
   * \pre the run has opened at least one free counter name besides the
   *      throughput columns it already opened, as the two throughput
   *      setters share the cap of sixteen names (FR-001)
   * \post the run carries `bytes_per_second` with the value of the
   *       argument, the `RATE` flag and the `K1024` base (FR-009)
   */
  auto setBytesProcessed(std::int64_t bytes) -> void;

  /**
   * @brief Set the item-throughput column to the count this call passes
   * (FR-009).
   *
   * The call takes the total items the run did, so a body that works a
   * known number per iteration passes that number times the iteration
   * count after the loop (FR-009). The call opens `items_per_second`
   * with the `RATE` flag and the `K1000` base, so the printed column is
   * this count divided by the primary time of the row in seconds; the
   * base carries the `k` suffix a rendered value would show and divides
   * nothing further (FR-003, FR-006, FR-009). A later
   * call writes the count alone, as for any used name (FR-001).
   *
   * \pre the run has opened at least one free counter name besides the
   *      throughput columns it already opened, as the two throughput
   *      setters share the cap of sixteen names (FR-001)
   * \post the run carries `items_per_second` with the value of the
   *       argument, the `RATE` flag and the `K1000` base (FR-009)
   */
  auto setItemsProcessed(std::int64_t items) -> void;

  /**
   * @brief State the problem size this instance measures (FR-019).
   *
   * The complexity fit of the family reads this value as its N; an instance
   * that states none falls back to its range value, and an instance with no
   * positive size of either kind leaves the family without a fit (FR-019,
   * FR-023).
   *
   * \pre none
   * \post the run carries `problemSize` as the complexity size of this
   *       instance (FR-019)
   */
  auto setComplexityN(std::int64_t problemSize) -> void;

  /**
   * @brief Set the label of this repetition (FR-011).
   *
   * The runner copies the string to the row after the loop, and the
   * report prints it as the trailing ` label=<text>` field of that row,
   * present only when the row carries a label (FR-011, the clarification
   * of 2026-10-14). The last call of a repetition wins, and the
   * aggregate row takes the label only when every repetition shares it
   * (FR-012, D-04).
   *
   * \pre `label` holds no newline character: the report prints one line
   *      per row, so a label that breaks the line is a violation at the
   *      call (spec Edge Cases, FR-011)
   * \post this repetition carries `label` in place of any label an
   *       earlier call of it set (FR-011, D-04)
   */
  auto setLabel(std::string_view label) -> void;

  /**
   * @brief Stop the timed window from this point (FR-009).
   *
   * The call stands in the benchmark body. It takes one sampling action,
   * the same action the endpoint pair takes, and opens one pause
   * interval that `resumeTiming` closes (FR-009). The runner folds every
   * closed interval and subtracts it from each column of the row
   * (FR-010). When the run's recorder holds no room for the pair, the
   * pair stands on the fold path instead: the run's two-point slot
   * takes the points and the commit folds them into the row's running
   * totals, so no closed pair is lost and no fixed allowance bounds
   * the run (FR-025, FR-052, Q-10).
   *
   * \pre this state belongs to a run, its sampling window stands open
   *      and unclosed, and no pause stands open on it (FR-011, D-15)
   * \post one pause stands open, and one sampling action stands in the
   *       run's recorder or in the run's fold slot (FR-009, FR-052)
   */
  auto pauseTiming() -> void
  {
    SG_REQUIRE(!m_callbackState,
               "pauseTiming in a callback state violates the callback-state "
               "rule that the callback state opens no sampling window "
               "(R-05, D-15)");
    SG_REQUIRE(m_windowOpened && !m_windowClosed,
               "pauseTiming needs the open sampling window of a run (FR-011)");
    SG_REQUIRE(m_pauseDepth == 0,
               "a second pauseTiming beside an open pause violates the bound "
               "of one open pause per run (FR-011)");
    // FR-052: the room decision belongs to the recorder alone. The sink of a
    // closed pair stands wherever a sampling window stands: the runner hands
    // one to its state at the attempt site beside the recorder it plans, and
    // the callback state, the sole holder of a null sink, opens no window and
    // falls to the precondition above.
    // specs/021 FR-001, FR-008: the chunk points the run still owes stand
    // outside the pause room. A pair that would leave fewer points than the
    // boundaries and the exit still need closes through the slot, so the
    // boundary sample never passes the capacity.
    if (m_recorder->count() + 2 + m_chunkPointsLeft > m_recorder->capacity()) {
      // The recorder keeps its room for the window's own points, and
      // the pair closes through the slot (FR-052).
      m_pauseFoldPair = true;
      m_slot = m_slotReset;
      m_slot.sample();
      ++m_samplesTaken;
    } else {
      m_pauseFoldPair = false;
      m_recorder->sample();
      ++m_samplesTaken;
      ++m_foldPoints;
    }
    m_pauseDepth = 1;
    SG_ENSURE(m_pauseDepth == 1,
              "one pause stands open after the pause sample (FR-009)");
  }

  /**
   * @brief Restart the timed window and record the interval it paused
   * (FR-009).
   *
   * The call takes the sample that closes the open pause, records the
   * pair of sample ordinals of that interval inside the run's span, or,
   * on the fold path, folds the pair into the row's running totals at
   * this commit, and counts one interval for the run (E-03, FR-018,
   * FR-052).
   *
   * \pre this state belongs to a run and one pause stands open on it
   *      (FR-011, D-15)
   * \post one further sample stands in the run's span, the ordinal pair
   *       of the closed interval stands in the run's pause list, and no
   *       pause is open (FR-009, E-03)
   */
  auto resumeTiming() -> void
  {
    SG_REQUIRE(!m_callbackState,
               "resumeTiming in a callback state violates the callback-state "
               "rule that the callback state opens no sampling window "
               "(R-05, D-15)");
    SG_REQUIRE(m_pauseDepth == 1,
               "resumeTiming with no open pause violates the bound of one "
               "open pause per run (FR-011)");
    closePausePair();
    SG_ENSURE(m_pauseDepth == 0,
              "no pause stands open after the resume sample (FR-009)");
  }

  /**
   * @brief The outcome the function requested, if any.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto outcome() const noexcept -> RunOutcome
  {
    return m_outcome;
  }

  /**
   * @brief The reason a skip requested, empty otherwise.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto reason() const noexcept -> const std::string&
  {
    return m_reason;
  }

  /**
   * @brief The argument of the running instance at `index` (FR-008).
   *
   * The instance owns the storage this reads, so a read is one span
   * index: no allocation, no lock, no recorder work (R-04, FR-009).
   *
   * \pre `index` stands below `rangeCount()` (FR-008)
   * \post none
   */
  [[nodiscard]] auto range(const std::size_t index) const -> std::int64_t
  {
    SG_REQUIRE(index < m_arguments.size(),
               "a range index at or above the argument count violates the "
               "bound index < rangeCount() (FR-008)");
    return m_arguments[index];
  }

  /**
   * @brief The argument count of the running instance (FR-008).
   *
   * A family that states no family call has one instance with zero
   * arguments, and this reports that zero.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto rangeCount() const noexcept -> std::size_t
  {
    return m_arguments.size();
  }

  /**
   * @brief The index of the context that runs this body (FR-017).
   *
   * A threaded instance dispatches one body to each of its contexts and
   * the index names the one this state belongs to, from `0` to
   * `threads() - 1`. A run on a single context reports `0` (FR-017,
   * E-05).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto threadIndex() const noexcept -> int
  {
    return m_threadIndex;
  }

  /**
   * @brief The thread count of the running instance (FR-017).
   *
   * The count is the context count of the attempt: the number of bodies
   * the harness dispatched, each on its own context. A benchmark that
   * called no thread method runs on one context and reads 1 (FR-017,
   * FR-022). No count of a running instance stands below 1 (FR-024).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto threads() const noexcept -> int { return m_threads; }

private:
  friend class detail::Runner;
  // The guard reads the skip flag of the run so its destructor can skip
  // the resume on a skip path (FR-012).
  friend class ScopedPauseTiming;

  // A callback state names the state the harness hands a setup or
  // teardown callback: it carries the instance arguments, holds no
  // recorder, and opens no sampling window (R-05, E-07).
  State(const std::uint64_t iterations,
        counters::RecorderHandle<counters::HardStop>* recorder,
        std::span<const std::int64_t> arguments,
        const bool callbackState = false) noexcept
      : m_iterations(iterations)
      , m_recorder(recorder)
      , m_callbackState(callbackState)
      , m_arguments(arguments)
  {
  }

  // The entry half of `begin()`, held out of line so that `begin()` itself
  // inlines: an outlined `begin()` hands the cursor's storage to a call,
  // the compiler then treats that storage as address-taken, and the timed
  // loop reloads its count from memory on every iteration (FR-004, the
  // codegen-shape gate of specs/015 IF-04). The two contract checks of this
  // entry stay in `begin()`, where the contract is documented: Principle II
  // puts the enforcement at the documented interface, and the pairing half
  // of the dbc gate reads it there.
  [[gnu::noinline]] auto openRun() noexcept -> bool
  {
    if (m_skipRequested) {
      return false;
    }
    ++m_loopsStarted;
    openWindow();
    return true;
  }

  auto openWindow() noexcept -> void
  {
    SG_ASSERT(m_recorder != nullptr,
              "the harness constructs every state with the recorder that "
              "samples its window");
    if (!m_windowOpened) {
      m_recorder->sample();
      ++m_samplesTaken;
      ++m_foldPoints;
      m_windowOpened = true;
    }
  }

  // specs/021 FR-001, FR-002: the sample one chunk boundary takes. The
  // point closes the chunk that ran and opens the next, so the fold after
  // the run reads one window per chunk. The runner sets the cadence and
  // reserves the recorder for it, so this path never passes the capacity.
  auto takeChunkSample() noexcept -> void
  {
    SG_ASSERT(m_recorder != nullptr,
              "the harness constructs every state with the recorder that "
              "samples its window");
    m_recorder->sample();
    ++m_samplesTaken;
    ++m_foldPoints;
    --m_chunkPointsLeft;
  }

  // specs/021 FR-001: the chunk size of this run, set by the runner before
  // `begin()` opens the window. Zero leaves the loop in its unchunked shape
  // and the recorder at the endpoint pair.
  auto setChunking(const std::uint64_t chunkIterations) noexcept -> void
  {
    m_chunkIterations =
        chunkIterations < m_iterations ? chunkIterations : m_iterations;
  }

  auto takeExitSample() noexcept -> void
  {
    SG_ASSERT(m_recorder != nullptr,
              "the harness constructs every state with the recorder that "
              "samples its window");
    if (m_windowOpened && !m_windowClosed) {
      m_recorder->sample();
      ++m_samplesTaken;
      ++m_foldPoints;
      --m_chunkPointsLeft;
      m_windowClosed = true;
    }
  }

  // The closing half of one pause interval: the sample, the ordinal pair
  // and the interval count (FR-009). The pause sample is the last one
  // taken, so its ordinal stands one below the sample this call takes
  // (E-03). `resumeTiming` and the throw and skip path below share it.
  // A pair the pause routed to the fold path commits through the slot:
  // the fold runs at this commit, on the same two raw counter points,
  // and lands in the row's running totals; the recorder holds no point
  // of that pair (FR-052).
  auto closePausePair() noexcept -> void
  {
    if (m_pauseFoldPair) {
      m_slot.sample();
      detail::commitPauseFoldPair(*m_pauseFold, m_slot.view());
      ++m_samplesTaken;
      ++m_foldedPairs;
    } else {
      m_recorder->sample();
      m_pausePairs.emplace_back(m_foldPoints - 1, m_foldPoints);
      ++m_foldPoints;
      ++m_samplesTaken;
    }
    m_pauseDepth = 0;
    ++m_pauseCount;
  }

  [[gnu::cold]] auto closeWindow() noexcept -> void
  {
    SG_ASSERT(m_pauseDepth == 0,
              "the loop leaves with no pause open, so every recorded pause "
              "pair holds two samples (FR-011, D-10)");
    m_loopCompleted = true;
    takeExitSample();
  }

  // A run owns its exit sample however it ended, and an interval left
  // open by a throw or a skip owns its closing sample too, so every
  // recorded pair holds two points before a fold reads it (FR-017, D-11).
  auto topUpWindow() noexcept -> void
  {
    openWindow();
    if (m_pauseDepth != 0) {
      closePausePair();
    }
    takeExitSample();
  }

  [[nodiscard]] auto loopsStarted() const noexcept -> std::uint64_t
  {
    return m_loopsStarted;
  }

  [[nodiscard]] auto loopCompleted() const noexcept -> bool
  {
    return m_loopCompleted;
  }

  std::uint64_t m_iterations = 0;
  // specs/021 FR-001: the iteration count one chunk holds, zero for a run
  // that chunks nothing.
  std::uint64_t m_chunkIterations = 0;
  // specs/021 FR-001, FR-008: the recorder points the window still owes,
  // the chunk boundaries and the exit point, counted from the moment the
  // window opens. The pause room test reads it so a pause pair never takes
  // the room a boundary needs.
  std::uint64_t m_chunkPointsLeft = 1;
  bool m_skipRequested = false;
  RunOutcome m_outcome = RunOutcome::MEASURED;
  std::string m_reason;
  counters::RecorderHandle<counters::HardStop>* m_recorder = nullptr;
  std::uint64_t m_loopsStarted = 0;
  bool m_windowOpened = false;
  bool m_windowClosed = false;
  bool m_loopCompleted = false;
  // True for the state a setup or teardown callback receives; the
  // R-05 guards read it (R-05).
  bool m_callbackState = false;
  // The arguments of the running instance, owned by that instance
  // (E-03, R-04).
  std::span<const std::int64_t> m_arguments;
  // E-05, FR-017: the threaded run this state belongs to, the index of
  // the context running the body and the count of contexts of the
  // attempt. The runner writes both for a threaded body and leaves the
  // defaults on a single-context run, whose row keeps today's bytes
  // (FR-022).
  int m_threadIndex = 0;
  int m_threads = 1;

  // E-01: one counter name of the run's store. The name stands in a
  // fixed buffer, so opening a name allocates nothing inside the timed
  // loop (FR-007), and the flags and the base stand fixed from the
  // slot's first open (FR-008).
  struct CounterSlot
  {
    std::array<char, kMaxCounterNameBytes> name {};
    std::uint8_t nameSize = 0;
    Counter counter {};
    bool used = false;
  };

  // The run's counter store, scanned in slot order, so the printed
  // column order is the order the names opened (FR-006).
  std::array<CounterSlot, kMaxUserCounters> m_counters {};

  // D-04, FR-011: the label of this repetition. One string on the state,
  // one copy into the row after the loop; the last write of a repetition
  // wins (D-04).
  std::string m_label;

  // The pause bookkeeping of one run. A pause pair holds the sample
  // ordinals, zero-based, of one paused interval inside that run's
  // sample span, so the runner adds the run's base point when it
  // folds (E-03, R-05).
  std::uint8_t m_pauseDepth = 0;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> m_pausePairs;
  std::uint64_t m_pauseCount = 0;
  // Every sampling action of the run, the slot's points included: the
  // count the row prints (FR-013, FR-052).
  std::uint64_t m_samplesTaken = 0;
  // The points the run's recorder holds: the span every fold reads
  // (E-06). The slot's points stand outside it, so the raw ordinals
  // stay in span when a later pair folds (FR-052).
  std::uint64_t m_foldPoints = 0;
  // The fold path of one run (FR-052, Q-10): the reset copy of the
  // run's two-point slot, the working cursor, the sink holding each
  // column's running total, and the count of pairs that folded. The
  // runner owns the buffer and the sink behind these members.
  counters::RecorderHandle<counters::HardStop> m_slotReset {};
  counters::RecorderHandle<counters::HardStop> m_slot {};
  detail::PauseFoldSink* m_pauseFold = nullptr;
  std::uint64_t m_foldedPairs = 0;
  bool m_pauseFoldPair = false;
  // The manual accumulator: the sum of the iteration times a manual
  // benchmark supplies, in nanoseconds (E-04, FR-007).
  std::int64_t m_manualNs = 0;
  // The problem size the body stated for this instance, the N of the
  // complexity fit when it stands (FR-019).
  std::optional<std::int64_t> m_complexityN;
};

/**
 * @brief The scoped form of the pause pair: the region from this
 * object's construction to the end of its scope stands outside the
 * timed window (FR-012).
 *
 * The constructor calls `State::pauseTiming` and the destructor calls
 * `State::resumeTiming`, so the interval the guard bounds is the
 * interval the explicit pair bounds. The preconditions and
 * postconditions of those two calls stand once on the calls themselves
 * (FR-011). The destructor skips the resume when the run was skipped,
 * so a guard that leaves its scope on a skip path cannot turn one
 * breach into a second (FR-012).
 *
 * \pre none
 * \post none
 */
class SPEEDGUN_NG_EXPORT ScopedPauseTiming
{
public:
  /**
   * @brief Open one pause interval on `state` (FR-012).
   *
   * \pre none
   * \post none
   */
  explicit ScopedPauseTiming(State& state) noexcept
      : m_state(state)
  {
    m_state.pauseTiming();
  }

  /**
   * @brief Close the pause interval, and leave it open when the run was
   * skipped (FR-012).
   *
   * The destructor calls resumeTiming, and its contract guards run
   * under this class's implicit noexcept. The trap semantics of the
   * default contract build ends the process on a violation, the message
   * naming the broken pause pair. In a build whose handler throws, a
   * violation surfacing from this destructor ends the process while it
   * unwinds.
   *
   * \pre none
   * \post none
   */
  ~ScopedPauseTiming()
  {
    if (!m_state.m_skipRequested) {
      m_state.resumeTiming();
    }
  }

  ScopedPauseTiming(const ScopedPauseTiming&) = delete;
  ScopedPauseTiming(ScopedPauseTiming&&) = delete;
  auto operator=(const ScopedPauseTiming&) -> ScopedPauseTiming& = delete;
  auto operator=(ScopedPauseTiming&&) -> ScopedPauseTiming& = delete;

private:
  State& m_state;
};

/**
 * @brief The base of a fixture: the `setUp` and `tearDown` pair around
 * each run (E-06, FR-015).
 *
 * A user class derives from this and overrides the pair; the fixture
 * macros of FR-016 attach one method of the class to a family record,
 * and the harness builds one fixture object per run, runs the pair
 * around the callable in the untimed region, and destroys the object
 * after `tearDown` (FR-017, R-09). The state handed to the pair
 * follows the callback-state rule: reading the instance arguments and
 * the iteration count is legal there, and opening the timed loop is a
 * precondition violation (R-05).
 *
 * \pre none
 * \post none
 */
class SPEEDGUN_NG_EXPORT Fixture
{
public:
  Fixture() = default;
  Fixture(const Fixture&) = default;
  Fixture(Fixture&&) = default;
  auto operator=(const Fixture&) -> Fixture& = default;
  auto operator=(Fixture&&) -> Fixture& = default;

  /**
   * @brief Destruction through the `Fixture` base the factory yields.
   *
   * \pre none
   * \post none
   */
  virtual ~Fixture() = default;

  /**
   * @brief The hook that runs before the timed loop of one run
   * (FR-017).
   *
   * The empty default does nothing; a fixture overrides it to build
   * what its method measures on. The state the harness hands the pair
   * is the callback state of FR-018: reading the instance arguments
   * and the iteration count is legal there, and `begin()`,
   * `skipWithError` and `skipWithMessage` are precondition violations
   * (FR-017).
   *
   * \pre none
   * \post none
   */
  virtual auto setUp([[maybe_unused]] State& state) -> void {}

  /**
   * @brief The hook that runs after the timed loop of one run
   * (FR-017).
   *
   * The empty default does nothing; a fixture overrides it to release
   * what `setUp` built. The state follows the same callback-state rule
   * as `setUp` (FR-017).
   *
   * \pre none
   * \post none
   */
  virtual auto tearDown([[maybe_unused]] State& state) -> void {}
};

/**
 * @brief True when a metric of dimension `D` reports per iteration:
 * dimension events^1 or time^1 divides by N, every other dimension
 * keeps its window value (FR-022).
 *
 * \pre none
 * \post none
 */
template<class D>
[[nodiscard]] constexpr auto reportsPerIteration() noexcept -> bool
{
  return (D::kTimeExponent == 1 && D::kEventsExponent == 0)
      || (D::kTimeExponent == 0 && D::kEventsExponent == 1);
}

namespace detail
{

/**
 * @brief The erased metric record a handle hands the registry (E-01,
 * E-05): the expression spine, its column label, and the FR-022
 * per-iteration decision taken from the dimension tag.
 */
struct MetricSeed
{
  sg::counters::detail::ExprCore core;
  std::string label;
  bool perIteration = false;
};

// One statistic the handle accumulated before the run, the triple
// `addStatistic` appends to the registry record (FR-013).
struct StatisticSeed
{
  std::string name;
  StatUnit unit = StatUnit::TIME;
  StatisticFunc function;
};

}  // namespace detail

namespace detail
{

class ThreadRunCore;

}  // namespace detail

/**
 * @brief The harness-owned view of one threaded attempt and the
 * contexts it runs on (FR-021, E-08).
 *
 * The harness builds the attempt, its body nodes and its two barrier
 * rounds, and hands this view to the `threadRunner` callable of a family
 * that attached one. The callable dispatches each body index onto the
 * context of that index; the barriers, the per-context plans and the
 * merge stay the harness's, so a runner the harness did not write cannot
 * move a barrier wait inside a measured window (FR-019, FR-021).
 *
 * \invariant the context count stands at 1 or above, and every index of
 *            the half-open range from 0 to `contextCount()` names one
 *            body node of the attempt and the context that runs it, for
 *            the whole of the runner call (FR-021, FR-024)
 */
class SPEEDGUN_NG_EXPORT ThreadRun
{
public:
  /**
   * @brief The context count of the attempt, the thread count of its
   * instance (FR-021).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto contextCount() const noexcept -> int;

  /**
   * @brief The context that runs body `index` (FR-021).
   *
   * The harness owns that context: it created it when the instance
   * started and joins it when the instance finishes (D-05).
   *
   * \pre `index` stands in the half-open range from 0 to
   *      `contextCount()` (FR-021)
   * \post none
   */
  [[nodiscard]] auto context(int index) const noexcept
      -> const ExecutionContext&;

  /**
   * @brief Hand body `index` to the context of that index (FR-021).
   *
   * The body node is the harness's: it opens that context's own
   * thread-target plan, arrives at the start barrier, runs the timed
   * loop, arrives at the stop barrier and folds that plan (FR-018,
   * FR-019). The call returns without waiting, the way `dispatch()` of
   * the context surface does (FR-010).
   *
   * \pre `index` stands in the half-open range from 0 to
   *      `contextCount()` and no earlier call of this attempt dispatched
   *      that index, so an undispatched or twice-dispatched index ends
   *      the run with a violation at this point (FR-021, FR-024)
   * and the two barrier rounds of the attempt hold every body of the run
   * (FR-019, FR-021). No postcondition of this call can re-read that
   * hand-off: the loop of the context takes the node as soon as it stands
   * idle, so an inbox read here is not a stable observation (FR-010).
   *
   * \post none
   */
  auto dispatch(int index) -> void;

private:
  friend class detail::Runner;

  // E-08, FR-021: the view reads the harness-owned attempt record it
  // stands on. The runner builds that record with the attempt, so it
  // outlives the view for the whole of the runner call.
  explicit ThreadRun(detail::ThreadRunCore& core) noexcept
      : m_core(&core)
  {
  }

  detail::ThreadRunCore* m_core;
};

/**
 * @brief The dispatch function a family supplies for its threaded runs
 * (FR-021).
 *
 * The callable receives the run's `ThreadRun` view and dispatches the
 * per-thread bodies itself, one dispatch per body index; the harness
 * keeps the body nodes alive across the call (FR-021, E-08).
 *
 * \pre the callable is callable with a `ThreadRun&` and dispatches every
 *      body index of the run before it returns (FR-021)
 * \post every body of the attempt stands dispatched once, so the barrier
 *       rounds complete and the run publishes its merged row (FR-020,
 *       FR-021)
 * \invariant the harness owns the `ThreadRun` the callable receives for
 *            the whole of the call (FR-021)
 */
using ThreadRunnerFn = std::function<void(ThreadRun&)>;

// specs/027 data-model.md E-02, R-10: the three closed values of the
// jitter probe mode. The pre-check phase runs when the mode is PRECHECK
// or BOTH, and the alongside phase runs when the mode is ALONGSIDE or
// BOTH; the overlap check binds the alongside phase only (FR-014, D-3).
enum class JitterProbeMode : std::uint8_t
{
  ALONGSIDE,
  PRECHECK,
  BOTH,
};

// specs/027 data-model.md E-01, contracts/public-api.md: the bounds of
// the option fields (FR-007 to FR-009, D-13).
inline constexpr std::uint64_t kDefaultThresholdNs = 200;
inline constexpr std::uint32_t kDefaultGapCount = 65536;
inline constexpr std::uint32_t kMinGapCount = 1;
inline constexpr std::uint32_t kMaxGapCount = 16777216;
inline constexpr std::uint32_t kDefaultPrecheckMs = 200;

/**
 * @brief The engine the `--os-noise` value names (specs/036 FR-002,
 * D-4, data-model.md E-16, R-06). The enum closes the engine set at
 * the type level the way `JitterProbeMode` closes the modes; `NONE`
 * names no tracer, and a run whose value stands absent carries `NONE`.
 *
 * \pre none
 * \post none
 */
enum class TracerEngine : std::uint8_t
{
  NONE,
  OSNOISE,
  TIMERLAT,
};

/**
 * @brief The one argument of every jitter probe entry point (FR-003,
 * FR-015).
 *
 * `cpus` carries the probe cpu request: an empty vector names the
 * `idle` preset, and a non-empty vector names the probe cpus in the
 * order given (FR-016, D-4). `thresholdNs` carries the gap threshold
 * in ns as `std::optional<std::uint64_t>`: a field with no value names
 * the default rule, the 200 ns default rising to 10 times the measured
 * loop period when that stands larger, and a value stands as given, an
 * explicit `threshold=200` staying exactly 200 ns (FR-007, FR-008,
 * D-2). `mode` carries the default when the option
 * names no mode value (FR-012, D-1). `gapCount` carries the per-cpu
 * buffer size in records (FR-009). `precheckMs` carries the pre-check
 * duration in ms (FR-014, D-13). `strict` stops the run with status 2
 * when the time-stamp counter stands unavailable; a non-strict run
 * discloses and continues with status 0 (FR-037, D-10).
 * `precheckSerial` runs the pre-check one measured cpu per window,
 * ascending cpu order, the planned cpus before the candidates; the
 * parallel shape stands the default (FR-009, D-2).
 * `tracerEngine` carries the engine the `--os-noise` value names;
 * `NONE` names no tracer (specs/036 FR-005, R-06). `tracerCpus`
 * carries the tracer cpu request: an empty vector names the
 * pre-check's candidate set, and a non-empty list names cpus inside
 * the process affinity mask and the online set (specs/036 FR-008,
 * R-09). The entry
 * points that take the options require a `thresholdNs` value at or
 * above 1 when the field carries one, `gapCount` inside the
 * `kMinGapCount` through `kMaxGapCount` range,
 * and `precheckMs` at or above 1, and enforce the bounds where they
 * parse and bind (FR-008, FR-009, D-13).
 *
 * \pre none
 * \post none
 */
struct JitterProbeOptions
{
  std::vector<std::uint32_t> cpus = {};
  std::optional<std::uint64_t> thresholdNs = std::nullopt;
  JitterProbeMode mode = JitterProbeMode::ALONGSIDE;
  std::uint32_t gapCount = kDefaultGapCount;
  std::uint32_t precheckMs = kDefaultPrecheckMs;
  bool strict = false;
  bool precheckSerial = false;
  TracerEngine tracerEngine = TracerEngine::NONE;
  std::vector<std::uint32_t> tracerCpus = {};
};

/**
 * @brief State the jitter probe options of the suite's next run
 * (FR-015).
 *
 * The options bind the suite the way the command-line option does.
 * The availability rule of FR-037 binds the probe the run starts,
 * whatever entry point configured it: the run discloses `not
 * available` and continues with status 0, and `strict` stops it with
 * status 2.
 *
 * \pre the run has not started for any benchmark (E-02, FR-015)
 * \pre `options.cpus` carries the probe cpu request: an empty vector
 *      names the `idle` preset, and a non-empty vector names cpus
 *      inside the process affinity mask (FR-011, FR-016, D-4)
 * \pre `options` carries a `thresholdNs` value at or above 1 when the
 *      field carries one, `gapCount` inside the `kMinGapCount` through
 *      `kMaxGapCount` range, and `precheckMs` at or above 1 (FR-008,
 *      FR-009, D-13)
 * \post the suite's next run starts the probe before its first
 *       selected instance and stops, drains and joins it after the
 *       last, over these options (FR-006, R-01)
 * \post the option sources stand in order: the command line's value,
 *       then the option file's, then the suite call's, each answering
 *       only where no higher source named the probe (FR-015, R-01)
 */
SPEEDGUN_NG_EXPORT auto runJitterProbe(const JitterProbeOptions& options)
    -> void;

/**
 * @brief The registration result: the caller's view of one registry
 * entry (E-02).
 *
 * Every setter writes one option of the entry before the run starts.
 * A handle copied or moved names the same entry.
 *
 * \invariant a label call leaves the entry carrying exactly the labels
 *            that call set, whatever label list stood before (FR-006);
 *            every thread count of the entry stands at 1 or above, and
 *            the entry holds at most one thread runner, so the last
 *            thread call wins the set and the last runner call wins the
 *            slot (FR-021, FR-024, R-10)
 */
class SPEEDGUN_NG_EXPORT BenchmarkHandle
{
public:
  /**
   * @brief An empty handle, the default state of a handle that names
   * no entry.
   *
   * \pre none
   * \post none
   */
  BenchmarkHandle() = default;

  /**
   * @brief The registered name, empty for an empty handle.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto name() const noexcept -> std::string_view;

  /**
   * @brief Set the minimum time per run in nanoseconds (FR-007,
   * FR-015).
   *
   * \pre the run has not started for this benchmark (E-02)
   * \post the entry carries this value, and it wins over a
   *       command-line value (FR-015)
   */
  auto minTime(std::int64_t nanoseconds) -> BenchmarkHandle&;

  /**
   * @brief Set the minimum warm-up time in nanoseconds (FR-011,
   * FR-015).
   *
   * \pre the run has not started for this benchmark (E-02)
   * \post the entry carries this value, and it wins over a
   *       command-line value (FR-015)
   */
  auto warmupTime(std::int64_t nanoseconds) -> BenchmarkHandle&;

  /**
   * @brief Set the repetition count (FR-012, FR-015).
   *
   * \pre the run has not started for this benchmark (E-02)
   * \post the entry carries this value, and it wins over a
   *       command-line value (FR-015)
   */
  auto repetitions(std::uint64_t count) -> BenchmarkHandle&;

  /**
   * @brief Fix the iteration count, which skips calibration (FR-013).
   *
   * \pre the run has not started for this benchmark (E-02)
   * \post the entry carries this value, calibration is skipped, and a
   *       set warm-up time still warms up from this count (FR-011)
   */
  auto iterations(std::uint64_t count) -> BenchmarkHandle&;

  /**
   * @brief Move the CPU column of this family to the process CPU leaf
   * (FR-004).
   *
   * The selection stands for every instance of the family, and it
   * combines with the real-time call and with the manual-time call
   * (FR-004, FR-008).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry selects `machine/process_cpu` for the CPU column of
   *       that family, and the returned handle is this handle (FR-004)
   */
  auto measureProcessCpuTime() -> BenchmarkHandle&;

  /**
   * @brief Make real time the primary time of this family (FR-005).
   *
   * The qualification test, the growth rule, and the aggregate time
   * statistics read the real column from this call on. The selection
   * excludes the manual-time selection, so the second of the two calls
   * is the violation, in either order (FR-008, D-04).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and this family selects no manual time
   *      (E-02, FR-008)
   * \post the entry makes the real column the primary time of that
   *       family, and the returned handle is this handle (FR-005)
   */
  auto useRealTime() -> BenchmarkHandle&;

  /**
   * @brief Make the manual sum the primary time of this family (FR-006).
   *
   * The benchmark supplies one iteration time per iteration through
   * `State::setIterationTime`, and the primary time of a run becomes
   * the sum of the accepted values. The qualification test, the growth
   * rule, and the aggregate time statistics read that sum, and the
   * fivefold real-time stop leaves a manual run alone (FR-006, D-05).
   * The selection excludes the real-time selection, so the second of
   * the two calls is the violation, in either order (FR-008, D-04).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and this family selects no real time
   *      (E-02, FR-008)
   * \post the entry makes the manual sum the primary time of that
   *       family, and the returned handle is this handle (FR-006)
   */
  auto useManualTime() -> BenchmarkHandle&;

  /**
   * @brief Set the unit the report scales the printed time values of
   * this family to (FR-015).
   *
   * A stored time value stays nanoseconds; the report divides each
   * printed time value of that family by the unit's divisor and prints
   * the unit's label. The call wins over the default every run of this
   * family resolves to (FR-015, FR-016, FR-027, D-12).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry carries `timeUnit` as the unit of that family, and
   *       the returned handle is this handle (FR-015)
   */
  auto unit(TimeUnit timeUnit) -> BenchmarkHandle&;

  /**
   * @brief Attach a counter expression built with the 007 arithmetic
   * (FR-021).
   *
   * A leaf the catalog lacks is not a rejected call here: the plan
   * compile of the run reports it as a recoverable error naming the
   * leaf, and that benchmark fails (FR-023).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the returned handle is this handle, and the entry carries
   *       the metric under `label` (FR-021)
   */
  template<class D>
  auto addMetric(const sg::counters::Expression<D>& expression,
                 std::string_view label) -> BenchmarkHandle&
  {
    SG_REQUIRE(m_entry != nullptr,
               "a metric attaches to a registered entry (E-02)");
    auto& attached = addMetricCore(detail::MetricSeed {
        expression.core, std::string(label), reportsPerIteration<D>()});
    SG_ENSURE(&attached == this, "the handle carries the metric (FR-021)");
    return attached;
  }

  /**
   * @brief Attach one custom statistic to the aggregate of this family
   * (FR-013, FR-014, FR-015).
   *
   * The function computes over the primary time per iteration of the clean
   * repetitions, the same list the built-in statistics summarize, and its
   * value prints among the built-in fields of the aggregate row in the
   * order the calls came in. A `TIME` statistic prints in the unit of the
   * row, a `PERCENTAGE` statistic unscaled by it.
   *
   * \pre the handle names a registered entry, the run has not started for
   *      this benchmark, and `name` holds none of the reserved built-in
   *      aggregate names `mean`, `median`, `sd`, `cv`, `min`, `max`, `n`
   *      (E-02, FR-013)
   * \post the entry carries the name, unit and function of that statistic,
   *       and the returned handle is this handle (FR-013)
   */
  auto addStatistic(std::string_view name,
                    StatUnit unit,
                    const StatisticFunc& function) -> BenchmarkHandle&;

  /**
   * @brief Fit the family's values against the problem size of its instances
   * (FR-019, FR-020, FR-021).
   *
   * The fit runs over the instances of the family and reports one pair of rows
   * per fitted quantity: the primary time, each named user counter and each
   * attached metric. A family with fewer than two instances, or with an
   * instance that holds no positive size, states the reason in place of the
   * rows (FR-023).
   *
   * \pre the handle names a registered entry and the run has not started for
   *      this benchmark (E-02)
   * \post the entry carries that curve selection, and the returned handle is
   *       this handle (FR-019, FR-020)
   */
  auto complexity(BigO bigO) -> BenchmarkHandle&;

  /**
   * @brief Fit the family's values against a curve the caller supplies
   * (FR-020).
   *
   * The function is the curve of the fit: its coefficient and root-mean
   * relative error come from the same closed form the built-in curves use.
   *
   * \pre the handle names a registered entry, the run has not started for
   *      this benchmark, and the callable is callable (E-02, FR-020)
   * \post the entry carries that curve function as the selection of the fit,
   *       and the returned handle is this handle (FR-020)
   */
  auto complexity(BigOFun function) -> BenchmarkHandle&;

  /**
   * @brief Restrict the report of this family to its aggregate rows
   * (FR-016).
   *
   * The console is the only report output today, so the restriction reaches
   * it; a later file output inherits the mode (FR-016). A family with no
   * aggregate row keeps its rows: the mode hides no row that has no
   * aggregate beside it (FR-018).
   *
   * \pre the handle names a registered entry and the run has not started for
   *      this benchmark (E-02)
   * \post the entry carries the report mode of that family, and the returned
   *       handle is this handle (FR-016)
   */
  auto reportAggregatesOnly() -> BenchmarkHandle&;

  /**
   * @brief Turn the percentile output of this family on or off
   * (specs/021 FR-019).
   *
   * The call selects the output and carries no percentile list: the run
   * reports the default points. The command line wins over the family, the
   * way `--repetitions` does (FR-020).
   *
   * \pre the handle names a registered entry and the run has not started
   *      for this benchmark (E-02)
   * \post the entry carries the percentile selection of that family, and
   *       the returned handle is this handle (FR-019)
   */
  auto showPercentiles(bool show) -> BenchmarkHandle&;

  /**
   * @brief Set the collection method this family asks for (FR-023,
   * FR-025).
   *
   * The request names a method and nothing else: no plan, no limit, no
   * column list, and no timing change of its own. The `--metric-method`
   * flag wins over it the way `--repetitions` wins over `repetitions`, so
   * one run settles one method for every family it runs, and a request of
   * `AUTO` settles at run time per the table of data-model E-01 (FR-024,
   * FR-025, research D-09).
   *
   * \pre the handle names a registered entry and the run has not started
   *      for this benchmark (E-02)
   * \post the entry carries `method` as the request of that family, and
   *       the returned handle is this handle; the method the run settles
   *       to is published as `BenchmarkResult::metricMethod` whatever it
   *       settled to (FR-025)
   */
  auto metricMethod(MetricMethod method) -> BenchmarkHandle&;

  /**
   * @brief Restrict the console display of this family to its aggregate rows
   * (FR-017).
   *
   * The stored result keeps every repetition row; the mode trims the display
   * alone, and while the console is the only channel the two modes print the
   * same trimmed console (FR-017, FR-018).
   *
   * \pre the handle names a registered entry and the run has not started for
   *      this benchmark (E-02)
   * \post the entry carries the display mode of that family, and the returned
   *       handle is this handle (FR-017)
   */
  auto displayAggregatesOnly() -> BenchmarkHandle&;

  /**
   * @brief Append one instance carrying the single argument `value`
   * (FR-001).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry carries one more argument list of one argument,
   *       and the returned handle is this handle (FR-001)
   */
  auto arg(std::int64_t value) -> BenchmarkHandle&;

  /**
   * @brief Append one instance carrying the arguments of `values`, one
   * list per call (FR-001).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry carries one more argument list holding `values`,
   *       and the returned handle is this handle (FR-001)
   */
  auto args(std::initializer_list<std::int64_t> values) -> BenchmarkHandle&;

  /**
   * @brief Append one instance carrying `values` as its argument list,
   * one list per call (FR-001).
   *
   * A list that `createRange` or `createDenseRange` built is one such
   * value (FR-002).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry carries one more argument list holding `values`,
   *       and the returned handle is this handle (FR-001)
   */
  auto args(std::vector<std::int64_t> values) -> BenchmarkHandle&;

  /**
   * @brief Append one instance per argument of the range from `low` to
   * `high`: both bounds, plus every power of the family multiplier
   * strictly between them (FR-001, FR-004).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and `low` stands at or below `high`
   *      (E-02, FR-006)
   * \post the entry carries one more argument list per value of
   *       `createRange(low, high, multiplier)`, and the returned handle
   *       is this handle (FR-004)
   */
  auto range(std::int64_t low, std::int64_t high) -> BenchmarkHandle&;

  /**
   * @brief Set the multiplier the later range calls of this family grow
   * by (FR-004).
   *
   * \pre the handle names a registered entry and `multiplier` stands at
   *      or above 2 (E-02, FR-006)
   * \post the entry carries the multiplier, every later `range` and
   *       `ranges` call of this family grows by it, and the returned
   *       handle is this handle (FR-004)
   */
  auto rangeMultiplier(std::int64_t multiplier) -> BenchmarkHandle&;

  /**
   * @brief Append the instances of one range per bound pair of
   * `bounds`, the pairs grown and then taken as one product (FR-001,
   * FR-004).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and every bound pair holds its low bound
   *      at or below its high bound (E-02, FR-006)
   * \post the entry carries one more argument list per combination of
   *       the grown pairs, and the returned handle is this handle
   *       (FR-001)
   */
  auto ranges(std::vector<std::pair<std::int64_t, std::int64_t>> bounds)
      -> BenchmarkHandle&;

  /**
   * @brief Append one instance per value from `low` to `high` in steps
   * of `step` (FR-001, FR-005).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, `low` stands at or below `high`, and
   *      `step` stands at or above 1 (E-02, FR-006)
   * \post the entry carries one more argument list per value of
   *       `createDenseRange(low, high, step)`, and the returned handle
   *       is this handle (FR-005)
   */
  auto denseRange(std::int64_t low,
                  std::int64_t high,
                  std::int64_t step = 1) -> BenchmarkHandle&;

  /**
   * @brief Append the product of `lists`: one instance per combination
   * in the odometer order of the cited revision, where the first list
   * advances fastest (FR-001).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and every list of `lists` carries at least
   *      one argument (E-02, FR-006)
   * \post the entry carries one more argument list per combination, and
   *       the returned handle is this handle (FR-001)
   */
  auto argsProduct(std::vector<std::vector<std::int64_t>> lists)
      -> BenchmarkHandle&;

  /**
   * @brief Run `fn` on this handle, the way `Apply` does at the cited
   * revision, so `fn` states the family calls of the whole family on
   * it (FR-001).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the returned handle is this handle, `fn` ran on it once, and
   *       every family call `fn` stated stands on the entry (FR-001)
   */
  template<class F>
  auto apply(F&& fn) -> BenchmarkHandle&
  {
    SG_REQUIRE(m_entry != nullptr,
               "a family call runs before the run starts (E-02)");
    applyGuard();
    fn(*this);
    SG_ENSURE(m_entry != nullptr,
              "the handle still names its entry after apply (FR-001)");
    return *this;
  }

  /**
   * @brief Set the label list of this family to the one label
   * (FR-001, E-04).
   *
   * A later `argName` or `argNames` call replaces the list again; the
   * entry never carries more labels than this last call states.
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and the family arity stands unset or at
   *      one (E-02, FR-006)
   * \post the entry carries the label list of exactly this label, and
   *       the returned handle is this handle (FR-001)
   */
  auto argName(std::string_view label) -> BenchmarkHandle&;

  /**
   * @brief Set the label list of this family to `labels`, one per
   * argument position (FR-001, E-04).
   *
   * The call replaces the whole list, whatever list stood before.
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and `labels` holds one label per argument
   *      position of the family arity (E-02, FR-006)
   * \post the entry carries the labels in order, and the returned
   *       handle is this handle (FR-001)
   */
  auto argNames(std::vector<std::string> labels) -> BenchmarkHandle&;

  /**
   * @brief Attach the setup callback of this family (FR-018).
   *
   * The callback runs once before each run of every instance of the
   * family, the warm-up, calibration, and measured runs included, in
   * the untimed region, so its work adds nothing to the reported time
   * (FR-018). One callback stands per slot and the last attachment
   * wins it (E-07, R-10). The state the callback receives is a
   * harness-built state carrying the instance arguments and the run's
   * iteration count, and it opens no sampling window: reading
   * `range(index)`, `rangeCount()` and `iterations()` is legal there,
   * while `begin()`, `skipWithError` and `skipWithMessage` are
   * `SG_REQUIRE` precondition violations, and `end()` stays legal
   * because it is static and touches no state (R-05).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry's setup slot carries `callback`, the callback this
   *       call attached wins the slot, and the returned handle is this
   *       handle (FR-018, R-10)
   */
  auto setup(std::function<void(State&)> callback) -> BenchmarkHandle&;

  /**
   * @brief Attach the teardown callback of this family (FR-018).
   *
   * The callback runs once after each run of every instance of the
   * family, after the timed loop and the fixture `tearDown`, in the
   * untimed region, and it carries the same restricted state as the
   * setup callback (R-05). One callback stands per slot and the last
   * attachment wins it (E-07, R-10).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry's teardown slot carries `callback`, the callback
   *       this call attached wins the slot, and the returned handle is
   *       this handle (FR-018, R-10)
   */
  auto teardown(std::function<void(State&)> callback) -> BenchmarkHandle&;

  /**
   * @brief Run every instance of this family on `count` contexts
   * (FR-016).
   *
   * The call states the thread count set of the family as the single
   * count, the way `ThreadRange` of the cited revision states one count
   * (research D-09). Each instance then runs on that many contexts, the
   * harness creating them once for the instance and reusing them for
   * every attempt, and the instance name carries a `/threads:N` segment
   * for a count above 1 (FR-016, D-05).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and `count` stands at 1 or above: a count a
   *      call cannot honor, zero included, is a violation at the call
   *      (E-02, FR-024)
   * \post the entry carries `{count}` as the thread count set of that
   *       family, in place of any set an earlier thread call stated, and
   *       the returned handle is this handle (FR-016)
   */
  auto threads(int count) -> BenchmarkHandle&;

  /**
   * @brief State the placement preset of this family's threaded runs
   * (FR-015).
   *
   * The name is one of the six placement presets `none`, `core`, `l3`,
   * `smt`, `compact` and `node`; the last call wins the slot, the way
   * the thread calls replace an earlier set. Each instance of this
   * family then runs its contexts on the cpus the preset plans for the
   * machine, and the run discloses the placement of each context
   * (FR-008, FR-021).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02)
   * \post the entry carries `presetName` in place of any name an
   *       earlier placement call stated, and the returned handle is
   *       this handle (FR-015)
   */
  auto placement(std::string_view presetName) -> BenchmarkHandle&;

  /**
   * @brief State one thread count per doubling from `minCount` up to
   * `maxCount` (FR-016).
   *
   * The counts are `minCount`, `minCount * 2` and each further doubling
   * that stays at or below `maxCount`, the growth of `ThreadRange` at
   * the cited revision; the expansion adds one instance per count (FR-016,
   * research D-09).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and both bounds stand at 1 or above with
   *      `minCount` at or below `maxCount` (E-02, FR-024)
   * \post the entry carries one thread count per doubling in the bounds,
   *       in place of any set an earlier thread call stated, and the
   *       returned handle is this handle (FR-016)
   */
  auto threadRange(int minCount, int maxCount) -> BenchmarkHandle&;

  /**
   * @brief State one thread count per step from `minCount` up to
   * `maxCount` (FR-016).
   *
   * The counts are `minCount`, `minCount + step` and each further step
   * that stays at or below `maxCount`, the shape of `DenseThreadRange`
   * at the cited revision; the expansion adds one instance per count
   * (FR-016, research D-09).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, both bounds stand at 1 or above with
   *      `minCount` at or below `maxCount`, and `step` stands at 1 or
   *      above (E-02, FR-024)
   * \post the entry carries one thread count per step in the bounds, in
   *       place of any set an earlier thread call stated, and the
   *       returned handle is this handle (FR-016)
   */
  auto denseThreadRange(int minCount,
                        int maxCount,
                        int step) -> BenchmarkHandle&;

  /**
   * @brief Attach the dispatch function of this family's threaded runs
   * (FR-021).
   *
   * The callable receives the run's `ThreadRun` view and dispatches the
   * per-thread bodies itself, one dispatch per body index before it
   * returns. The harness builds the body nodes and keeps them alive
   * across the call, and the two barrier rounds, the per-context plans
   * and the merged row stay the harness's (FR-021, E-08). One runner
   * stands per family and the last attachment wins the slot, the way the
   * setup callback does (R-10).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and `runner` is callable with a `ThreadRun&`
   *      (E-02, FR-021)
   * \post the entry's runner slot carries `runner` in place of any runner
   *       an earlier call attached, and the returned handle is this
   *       handle (FR-021)
   */
  auto threadRunner(ThreadRunnerFn runner) -> BenchmarkHandle&;

  /**
   * @brief State the jitter probe options of this family's runs
   * (FR-015).
   *
   * Where the handle and the command line both stand, the handle wins;
   * where the suite-level call and the command line both stand, the
   * command line wins; where the suite-level call and the option file
   * both stand, the option file's value wins (FR-015, R-01).
   *
   * \pre the handle names a registered entry and the run has not
   *      started for this benchmark (E-02, FR-015)
   * \pre `options` carries the validation the suite-level call
   *      states (FR-008, FR-009, D-13)
   * \post the entry carries the options in place of any set an
   *       earlier jitterProbe call stated, and the returned handle
   *       is this handle (FR-015)
   * \post the run of that family starts the probe before its first
   *       instance and stops it after the last (FR-006, FR-015)
   */
  auto jitterProbe(const JitterProbeOptions& options) -> BenchmarkHandle&;

private:
  friend class RegistryEntry;
  friend auto registerBenchmark(std::function<void(State&)> fn,
                                std::string_view name) -> BenchmarkHandle;
  friend auto registerFixtureBenchmark(
      std::function<void(State&)> fn,
      std::string_view name,
      std::function<std::unique_ptr<Fixture>()> factory) -> BenchmarkHandle;

  explicit BenchmarkHandle(class RegistryEntry& entry) noexcept;

  auto addMetricCore(const detail::MetricSeed& seed) -> BenchmarkHandle&;

  // The E-02 guard of `apply`: the header cannot read a field of the
  // incomplete RegistryEntry, so the check lives with the other family
  // calls in source/harness/registry.cpp.
  auto applyGuard() -> void;

  RegistryEntry* m_entry = nullptr;
};

/**
 * @brief The argument list a range call accepts: `low`, `high`, and
 * every power of `multiplier` strictly between them, in increasing
 * order (FR-002, FR-004).
 *
 * `range`, `ranges` and `args` each take a list this builder returns
 * (FR-002).
 *
 * \pre `multiplier` stands at or above 2 and `low` stands at or below
 *      `high` (FR-006, R-15)
 * \post the returned list holds both bounds and every power of
 *       `multiplier` strictly between them in increasing order, and a
 *       negative span follows the mirror of `AddRange` at the cited
 *       revision (FR-004)
 * \invariant the returned list is never empty, because both bounds
 *            stand in it (FR-004)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto createRange(
    std::int64_t low,
    std::int64_t high,
    std::int64_t multiplier = kDefaultRangeMultiplier)
    -> std::vector<std::int64_t>;

/**
 * @brief The argument list a dense range call accepts: every value
 * from `low` to `high` in steps of `step` (FR-002, FR-005).
 *
 * \pre `step` stands at or above 1 and `low` stands at or below `high`
 *      (FR-006, R-15)
 * \post the returned list holds `low`, `low + step`, and each further
 *       step that stays at or below `high`, in increasing order
 *       (FR-005)
 * \invariant the returned list is never empty, because `low` stands in
 *            it (FR-005)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto createDenseRange(std::int64_t low,
                                                       std::int64_t high,
                                                       std::int64_t step = 1)
    -> std::vector<std::int64_t>;

/**
 * @brief Register a benchmark function under a unique name (FR-001).
 *
 * A runtime registration reaches the registry from any code that runs
 * before the run starts. A repeated name reports a recoverable error,
 * keeps the first registration, and leaves the rest runnable (FR-002).
 *
 * \pre called before the run starts
 * \post the name is registered in registration order, and the returned
 *       handle names that entry (FR-003)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto registerBenchmark(
    std::function<void(State&)> fn, std::string_view name) -> BenchmarkHandle;

/**
 * @brief Register a fixture method: the callable plus the factory of
 * the one fixture object each run builds (FR-015, FR-016, R-09).
 *
 * The fixture macros of FR-016 call this; the factory yields the
 * derived object through the `Fixture` base, and the runner builds one
 * object per run with it, runs the pair around the callable, and
 * destroys the object after `tearDown` (FR-017, R-09).
 *
 * \pre called before the run starts, and both `fn` and `factory` name
 *      a callable (FR-001)
 * \post the registry holds one family record under `name` whose
 *       callable is `fn` and whose fixture factory is `factory`, and
 *       the returned handle names that entry (FR-003, FR-016)
 * \invariant the family record's factory yields one fixture object per
 *            run of that family (R-09)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto registerFixtureBenchmark(
    std::function<void(State&)> fn,
    std::string_view name,
    std::function<std::unique_ptr<Fixture>()> factory) -> BenchmarkHandle;

/**
 * @brief Set the process default unit that a later registration
 * captures (FR-016).
 *
 * The default reaches the registrations that follow it; a benchmark
 * registered earlier keeps the unit it captured at its own moment
 * (FR-016, D-12). The command-line unit option stays with the full
 * command-line spec of the roadmap (FR-016).
 *
 * \pre none
 * \post `getDefaultTimeUnit` reads back `timeUnit`, and the entry point
 *       of `speedgunMain` snapshots it at its entry, so the call
 *       reaches every registration that names no `unit` (FR-016, FR-027)
 */
SPEEDGUN_NG_EXPORT auto setDefaultTimeUnit(TimeUnit timeUnit) -> void;

/**
 * @brief Read the process default unit of this moment (FR-016).
 *
 * The default starts at `TimeUnit::NANOSECONDS`, the unit a run falls
 * back to when neither a `unit` call nor the entry point's batch
 * snapshot names one (FR-016, FR-027).
 *
 * \pre none
 * \post the returned unit is the value of the last
 *       `setDefaultTimeUnit` call, or `TimeUnit::NANOSECONDS` when no
 *       such call has run (FR-016)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto getDefaultTimeUnit() -> TimeUnit;

/**
 * @brief Run the registered benchmarks named by the command line and
 * print the report (FR-033).
 *
 * A suite links `speedgun-ng::harness` and calls this from `main`
 * (D-1).
 *
 * A command line that does not follow the option table of
 * `specs/015-benchmark-harness-core/contracts/cli.md` is a usage error:
 * the entry point reports it and exits with status two from inside the
 * parse (FR-035). That path leaves no status to state a postcondition
 * about, so this function takes no precondition of its own.
 *
 * \pre none
 * \post the returned status follows the exit-status table of that
 *       contract (FR-036)
 */
SPEEDGUN_NG_EXPORT auto speedgunMain(int argc, char** argv) -> int;

/**
 * @brief Read one JSON report document this harness wrote (specs/023
 * FR-022, D-08).
 *
 * The call answers the same result types the run produced, so handing
 * the value back to the writer with the raw setting the file carries
 * gives the same bytes the file already holds (FR-026). A field the
 * format allows to stand absent answers the empty value the field
 * documents.
 *
 * @pre `path` is not empty: an empty path is a contract violation. A
 *     path the file system refuses is an input error the call answers
 *     (specs/023 contracts/report-loader.md)
 * @post when the call answers a value, that value holds one entry per
 *       benchmark of the document in document order, with the raw chunk
 *       columns of the document in its `rowRaw` and `contextRaw` fields
 * @post when the call answers an error, the string names the JSON
 *       pointer of the fault and what the reader expected there; no
 *       error form throws to the caller or aborts (FR-023, FR-024)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto loadReport(const std::string& path)
    -> std::expected<ReportData, std::string>;

/**
 * @brief Parse one rule text into one rule value (specs/026 FR-004,
 * D-01, D-08).
 *
 * The command line and a library caller read the same grammar through
 * this one call: the value name, the optional `@` aggregate suffix, the
 * direction word, the threshold field and the optional filter, in that
 * order (FR-012, FR-021). A prefixed value name reads its label to the
 * first unescaped colon or at sign, and a backslash escapes a colon, a
 * comma, an at sign or a backslash inside the label (FR-015, FR-020,
 * FR-021, D-06, spec D-15).
 * The threshold field carries a percent, an `abs=` number, or the two
 * in that order, the percent answering a fraction (FR-022, FR-023,
 * D-07, spec D-22). The call consults no report, defaults nothing but
 * the aggregate of a statistics name, and repairs nothing.
 *
 * \pre `text` is the whole rule text of one `--compare-rule=` or of
 *      one library caller's rule, and carries no NUL byte inside it
 * \post when the call answers a value, every field holds what the text
 *       spelled and `text` holds the input whole (FR-004)
 * \post when the call answers an error, the string names the field of
 *       the fault and the text read; the caller owns the exit status
 *       (FR-005)
 * \post the call throws nothing to the caller and aborts nothing
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto parseCompareRule(std::string_view text)
    -> std::expected<CompareRule, std::string>;

/**
 * @brief Compare two loaded report values under a list of rules.
 *
 * An empty `rules` list applies the default rule
 * `time@median:lower:5%` (specs/026 spec D-5, FR-033), and given rules
 * replace it (FR-034). `options.strict` moves the verdict only: a NOISY
 * row fails the run when it stands and passes when it does not, and the
 * state word never changes for it (FR-044, FR-045). The entries follow
 * the run order of the baseline, then the names the contender adds in
 * the order of the contender (FR-002), and each rule answers one entry per
 * benchmark of the pair that its filter admits (FR-024). The call reads no
 * file, runs no
 * benchmark, reads no clock, samples no sensor, repairs no value and
 * prints nothing (FR-006, FR-007), and it throws nothing to the caller
 * and aborts nothing.
 *
 * \pre `baseline` and `contender` are values `loadReport` answered, or
 *      values built to the same shape: `noiseWarnings`, when non-empty,
 *      holds one entry per entry of `results`
 * \post When the call answers a value, every entry of `entries` carries the
 *       text of one of the rules the call applied and `passed` states the
 *       verdict of FR-044 and FR-045
 * \post When the call answers an error, the text names the fault and
 *       stands non-empty, and no partial entries reach the caller
 *       (FR-003)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto compareReports(
    const ReportData& baseline,
    const ReportData& contender,
    const std::vector<CompareRule>& rules,
    const CompareOptions& options) -> std::expected<CompareResult, std::string>;

/**
 * @brief Register `fn` at namespace scope before `main` begins, under
 * the name of `fn`, and yield the registration handle (FR-001, R-09,
 * R-10, FR-013).
 *
 * The macro declares one handle initialized by the registration, and
 * the registration call is the last token sequence, so a call site
 * chains family calls after the macro and ends with `;`:
 * `SG_BENCHMARK(fn).range(8, 1024);`. A site with no chain reads
 * `SG_BENCHMARK(fn);`.
 */
// The paste chain of `SG_BENCHMARK`: the generated identifier carries
// the source line, `__LINE__` is standard C++ with no compiler
// extension (FR-023, FR-028), and the middle link expands the id
// before the last link pastes it, because a parameter standing next to
// `##` is pasted unexpanded. The declaration is `static`, so two
// translation units each register their own without an ODR clash.

#define SG_BENCHMARK_REGISTRAR(fn, id) \
  [[maybe_unused]] static const ::sg::BenchmarkHandle \
      sgBenchmarkRegistrar_##fn##_##id = ::sg::registerBenchmark(&(fn), #fn)

#define SG_BENCHMARK_EXPAND(fn, id) SG_BENCHMARK_REGISTRAR(fn, id)

#define SG_BENCHMARK(fn) SG_BENCHMARK_EXPAND(fn, __LINE__)

/**
 * @brief Register `fn` with captured arguments as the one instance
 * `fn/captureName` (FR-013, R-13).
 *
 * The capture name is the one name segment the instance adds; the
 * captured arguments draw no segment of their own, so the family
 * carries the single name `fn/captureName` and expands to one
 * instance with no argument (FR-013). The values stand inside the
 * registered callable, which calls `fn(state, ...)` the way
 * `BENCHMARK_CAPTURE` does at the cited revision
 * (`registration.h:69-75`).
 *
 * The macro declares one handle initialized by the registration, so
 * a call site chains family calls after the macro and ends with `;`.
 *
 * \pre `fn` takes a `State&` followed by the captured arguments, and
 *      the registration runs before the run starts (E-02)
 * \post the registry holds one family record named `fn/captureName`
 *       whose callable passes the captured values to `fn`, and the
 *       yielded handle points at it (FR-013)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance `fn/captureName` (FR-013)
 */
// The paste chain of `SG_BENCHMARK_CAPTURE`, the same three links as
// `SG_BENCHMARK`: two registrations of one function and capture name
// on one source line collide.

#define SG_BENCHMARK_CAPTURE_REGISTRAR(fn, captureName, id, ...) \
  [[maybe_unused]] static const ::sg::BenchmarkHandle \
      sgBenchmarkRegistrar_##fn##_##captureName##_##id = \
          ::sg::registerBenchmark([](::sg::State& state) \
                                  { (fn)(state __VA_OPT__(, ) __VA_ARGS__); }, \
                                  #fn "/" #captureName)

#define SG_BENCHMARK_CAPTURE_EXPAND(fn, captureName, id, ...) \
  SG_BENCHMARK_CAPTURE_REGISTRAR(fn, captureName, id, __VA_ARGS__)

#define SG_BENCHMARK_CAPTURE(fn, captureName, ...) \
  SG_BENCHMARK_CAPTURE_EXPAND(fn, captureName, __LINE__, __VA_ARGS__)

// The paste chain of `SG_BENCHMARK_TEMPLATE`. The generated identifier
// cannot spell a type list, so it carries the source line of the
// registration: `__LINE__` is standard C++, and two registrations of
// one function template on one source line would collide, which no
// suite of this feature does. The middle link expands that id before
// it is pasted, because a parameter that stands next to `##` is
// pasted unexpanded.
#define SG_BENCHMARK_TEMPLATE_REGISTRAR(fn, id, ...) \
  [[maybe_unused]] static const ::sg::BenchmarkHandle \
      sgBenchmarkRegistrar_##fn##_##id = \
          ::sg::registerBenchmark(&fn<__VA_ARGS__>, #fn "<" #__VA_ARGS__ ">")

#define SG_BENCHMARK_TEMPLATE_EXPAND(fn, id, ...) \
  SG_BENCHMARK_TEMPLATE_REGISTRAR(fn, id, __VA_ARGS__)

/**
 * @brief Register the instantiation `fn<types>` as the instance named
 * `fn<` plus the type list plus `>` (FR-014, R-13).
 *
 * The type list enters the name stringified from the macro site, so a
 * qualified type argument written with a space keeps that space: the
 * preprocessor splits such an argument at its comma, and the
 * stringification joins the pieces back as the site spells them
 * (FR-014, R-13). The instantiation the name states is the callable
 * the registry holds, the way `BENCHMARK_TEMPLATE` does at the cited
 * revision (`registration.h:101-107`).
 *
 * \pre `fn` is a function template taking a `State&`, and the
 *      registration runs before the run starts (E-02)
 * The macro declares one handle initialized by the registration, so
 * a call site chains family calls after the macro and ends with `;`.
 *
 * \post the registry holds one family record named `fn<types>` whose
 *       callable is the instantiation for exactly those types, and
 *       the yielded handle points at it (FR-014)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance its name states (FR-014)
 */
#define SG_BENCHMARK_TEMPLATE(fn, ...) \
  SG_BENCHMARK_TEMPLATE_EXPAND(fn, __LINE__, __VA_ARGS__)

// The shared machinery of the seven fixture macros. A macro site
// generates one derived class, named by pasting
// SgBenchmarkFixture_ plus the class plus the method, and one
// handle variable in the `sgBenchmarkRegistrar_##fn` shape of
// `SG_BENCHMARK` (R-13); the identifiers carry no leading
// underscore and no `__` (FR-023). The derived class holds the method
// the macro site defines, the instance name the registrar registers,
// and the one slot naming the fixture object of the run in progress:
// the registered factory stores the object it builds into that slot,
// and the registered callable reads it, so the method runs on the
// object the run owns (R-09). A threaded run writes the slot inside the
// body of context 0, before the start barrier releases, and every other
// context reads it after that edge; the fixture pair of the run stands
// once, on context 0, outside both barriers (FR-023, D-07).

#define SG_BENCHMARK_FIXTURE_DECLARE(FixtureClass, Method) \
  class SgBenchmarkFixture_##FixtureClass##_##Method : public FixtureClass \
  { \
  public: \
    inline static SgBenchmarkFixture_##FixtureClass##_##Method* \
        sgCurrentCase = nullptr; \
    static constexpr std::string_view sgFixtureName = \
        #FixtureClass "/" #Method; \
    void sgBenchmarkCase(::sg::State&); \
  };

#define SG_BENCHMARK_FIXTURE_REGISTER(FixtureClass, Method, DerivedClass) \
  [[maybe_unused]] static const ::sg::BenchmarkHandle \
      sgBenchmarkRegistrar_##FixtureClass##_##Method = \
          ::sg::registerFixtureBenchmark( \
              [](::sg::State& state) \
              { DerivedClass::sgCurrentCase->sgBenchmarkCase(state); }, \
              DerivedClass::sgFixtureName, \
              []() -> std::unique_ptr<::sg::Fixture> \
              { \
                auto object = std::make_unique<DerivedClass>(); \
                DerivedClass::sgCurrentCase = object.get(); \
                return object; \
              })

/**
 * @brief Define and register the fixture method as the instance
 * `FixtureClass/Method` (FR-016, R-13).
 *
 * The macro declares the class derived from `FixtureClass`, registers
 * it, and ends with the method definition header: the site follows the
 * macro with `(state) { body }`, the way `BENCHMARK_F` does at the
 * cited revision (`registration.h:196-200`). The instance name starts
 * with the fixture class, so the suite of FR-021 is that class and a
 * method named `DISABLED_x` is no disabled instance (R-08, R-11).
 *
 * \pre `FixtureClass` derives from `::sg::Fixture`, `Method` is a new
 *      name in that generated class, and the registration runs before
 *      the run starts (E-02)
 * \post the registry holds one family record named
 *       `FixtureClass/Method` with the method as its callable and a
 *       factory of the derived object (FR-016)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance `FixtureClass/Method`
 *            (FR-016)
 */
#define SG_BENCHMARK_F(FixtureClass, Method) \
  SG_BENCHMARK_FIXTURE_DECLARE(FixtureClass, Method) \
  SG_BENCHMARK_FIXTURE_REGISTER( \
      FixtureClass, Method, SgBenchmarkFixture_##FixtureClass##_##Method); \
  void SgBenchmarkFixture_##FixtureClass##_##Method::sgBenchmarkCase

/**
 * @brief Define the fixture method now, registering it later with
 * `SG_BENCHMARK_REGISTER_F` under the one name `FixtureClass/Method`
 * (FR-016).
 *
 * \pre `FixtureClass` derives from `::sg::Fixture` and `Method` is a
 *      new name in the generated class (E-02)
 * \post the derived class stands defined once its method body follows
 *       the macro; nothing is registered yet (FR-016)
 * \invariant `SG_BENCHMARK_DEFINE_F` and `SG_BENCHMARK_REGISTER_F`
 *            name the same generated class, so the later registration
 *            reaches this definition (FR-016)
 */
#define SG_BENCHMARK_DEFINE_F(FixtureClass, Method) \
  SG_BENCHMARK_FIXTURE_DECLARE(FixtureClass, Method) \
  void SgBenchmarkFixture_##FixtureClass##_##Method::sgBenchmarkCase

/**
 * @brief Register the method that `SG_BENCHMARK_DEFINE_F` defined,
 * under the name that macro's generated class carries, and yield the
 * registration handle (FR-016).
 *
 * The macro declares one handle initialized by the registration, so a
 * fixture family chains its family calls at the site:
 * `SG_BENCHMARK_REGISTER_F(F, m).range(8, 64);`. The fixture record
 * expands like any family record, one instance per argument list.
 *
 * \pre `SG_BENCHMARK_DEFINE_F` named the same pair earlier in the same
 *      scope, and the registration runs before the run starts (E-02)
 * \post the registry holds one family record named
 *       `FixtureClass/Method` whose callable is the defined method and
 *       the yielded handle points at it (FR-016)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance `FixtureClass/Method`
 *            (FR-016)
 */
#define SG_BENCHMARK_REGISTER_F(FixtureClass, Method) \
  SG_BENCHMARK_FIXTURE_REGISTER( \
      FixtureClass, Method, SgBenchmarkFixture_##FixtureClass##_##Method)

/**
 * @brief Define and register the method of the fixture class template
 * instantiated over `types`, as the instance `FixtureClass<types>/Method`
 * (FR-016, R-13).
 *
 * The type list enters the name stringified from the macro site, the
 * way `SG_BENCHMARK_TEMPLATE` carries it (`registration.h:154-163`).
 *
 * \pre `FixtureClass` is a class template of fixtures, `Method` is a
 *      new name in the generated class, and the registration runs
 *      before the run starts (E-02)
 * \post the registry holds one family record named
 *       `FixtureClass<types>/Method` with the method as its callable
 *       and a factory of the derived object (FR-016)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance its name states (FR-016)
 */
#define SG_BENCHMARK_TEMPLATE_F(FixtureClass, Method, ...) \
  class SgBenchmarkFixture_##FixtureClass##_##Method \
      : public FixtureClass<__VA_ARGS__> \
  { \
  public: \
    inline static SgBenchmarkFixture_##FixtureClass##_##Method* \
        sgCurrentCase = nullptr; \
    static constexpr std::string_view sgFixtureName = \
        #FixtureClass "<" #__VA_ARGS__ ">/" #Method; \
    void sgBenchmarkCase(::sg::State&); \
  }; \
  SG_BENCHMARK_FIXTURE_REGISTER( \
      FixtureClass, Method, SgBenchmarkFixture_##FixtureClass##_##Method); \
  void SgBenchmarkFixture_##FixtureClass##_##Method::sgBenchmarkCase

/**
 * @brief Define the method of the instantiated fixture class template
 * now, registering it later with `SG_BENCHMARK_REGISTER_F` under the
 * name `FixtureClass<types>/Method` (FR-016).
 *
 * \pre `FixtureClass` is a class template of fixtures and `Method` is
 *      a new name in the generated class (E-02)
 * \post the derived class stands defined once its method body follows
 *       the macro; nothing is registered yet (FR-016)
 * \invariant `SG_BENCHMARK_TEMPLATE_DEFINE_F` and
 *            `SG_BENCHMARK_REGISTER_F` name the same generated class,
 *            so the later registration reaches this definition
 *            (FR-016)
 */
#define SG_BENCHMARK_TEMPLATE_DEFINE_F(FixtureClass, Method, ...) \
  class SgBenchmarkFixture_##FixtureClass##_##Method \
      : public FixtureClass<__VA_ARGS__> \
  { \
  public: \
    inline static SgBenchmarkFixture_##FixtureClass##_##Method* \
        sgCurrentCase = nullptr; \
    static constexpr std::string_view sgFixtureName = \
        #FixtureClass "<" #__VA_ARGS__ ">/" #Method; \
    void sgBenchmarkCase(::sg::State&); \
  }; \
  void SgBenchmarkFixture_##FixtureClass##_##Method::sgBenchmarkCase

// The paste chain of `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`, the same
// three links as `SG_BENCHMARK_TEMPLATE`: the generated identifier
// cannot spell a type list, so it carries the source line, `__LINE__`
// is standard C++ with no compiler extension (FR-023, FR-028), and the
// middle link expands that id before the last link pastes it.

#define SG_BENCHMARK_TEMPLATE_INSTANTIATE_REGISTRAR( \
    FixtureClass, Method, id, ...) \
  struct SgBenchmarkFixtureInstance_##FixtureClass##_##Method##_##id \
      : SgBenchmarkFixtureTemplate_##FixtureClass##_##Method<__VA_ARGS__> \
  { \
    static constexpr std::string_view sgFixtureName = \
        #FixtureClass "<" #__VA_ARGS__ ">/" #Method; \
  }; \
  [[maybe_unused]] static const ::sg::BenchmarkHandle \
      sgBenchmarkRegistrar_##FixtureClass##_##Method##_##id = \
          ::sg::registerFixtureBenchmark( \
              [](::sg::State& state) \
              { \
                SgBenchmarkFixtureTemplate_##FixtureClass##_##Method< \
                    __VA_ARGS__>::sgCurrentCase->sgBenchmarkCase(state); \
              }, \
              SgBenchmarkFixtureInstance_##FixtureClass##_##Method##_##id :: \
                  sgFixtureName, \
              []() -> std::unique_ptr<::sg::Fixture> \
              { \
                auto object = std::make_unique< \
                    SgBenchmarkFixtureInstance_##FixtureClass##_##Method##_##id>(); \
                SgBenchmarkFixtureTemplate_##FixtureClass##_##Method< \
                    __VA_ARGS__>::sgCurrentCase = object.get(); \
                return object; \
              })

#define SG_BENCHMARK_TEMPLATE_INSTANTIATE_EXPAND( \
    FixtureClass, Method, id, ...) \
  SG_BENCHMARK_TEMPLATE_INSTANTIATE_REGISTRAR( \
      FixtureClass, Method, id, __VA_ARGS__)

/**
 * @brief Define the method of a fixture class template once for every
 * instantiation (FR-016, R-13).
 *
 * The macro declares a class template derived from
 * `FixtureClass<Args...>` and ends with the method definition header
 * the way `BENCHMARK_TEMPLATE_METHOD_F` does at the cited revision
 * (`registration.h:184-193`); `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`
 * then registers one instantiation per call.
 *
 * \pre `FixtureClass` is a class template of fixtures, and the
 *      definition completes before the run starts (E-02)
 * \post the method template stands defined once its body follows the
 *       macro; nothing is registered yet (FR-016)
 * \invariant one method definition serves every instantiation of the
 *            pair of macros (FR-016)
 */
#define SG_BENCHMARK_TEMPLATE_METHOD_F(FixtureClass, Method) \
  template<class... Args> \
  class SgBenchmarkFixtureTemplate_##FixtureClass##_##Method \
      : public FixtureClass<Args...> \
  { \
  public: \
    inline static SgBenchmarkFixtureTemplate_##FixtureClass##_##Method* \
        sgCurrentCase = nullptr; \
    void sgBenchmarkCase(::sg::State&); \
  }; \
  template<class... Args> \
  void SgBenchmarkFixtureTemplate_##FixtureClass##_##Method< \
      Args...>::sgBenchmarkCase

/**
 * @brief Register one instantiation of the method that
 * `SG_BENCHMARK_TEMPLATE_METHOD_F` defined, as the instance
 * `FixtureClass<types>/Method` (FR-016, R-13).
 *
 * \pre `SG_BENCHMARK_TEMPLATE_METHOD_F` named the same pair earlier in
 *      the same scope, and the registration runs before the run
 *      starts (E-02)
 * \post the registry holds one family record named
 *       `FixtureClass<types>/Method` whose callable is that
 *       instantiation's method (FR-016)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance its name states (FR-016)
 */
#define SG_BENCHMARK_TEMPLATE_INSTANTIATE_F(FixtureClass, Method, ...) \
  SG_BENCHMARK_TEMPLATE_INSTANTIATE_EXPAND( \
      FixtureClass, Method, __LINE__, __VA_ARGS__)

}  // namespace sg

#endif  // SG_BENCHMARK_HPP
