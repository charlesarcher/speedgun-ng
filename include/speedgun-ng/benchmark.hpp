#ifndef SG_BENCHMARK_HPP
#define SG_BENCHMARK_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/dbc.hpp"
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
}  // namespace detail

// FR-004 sets the multiplier a range call grows by when the family
// names none (E-05, R-17).
inline constexpr std::int64_t kDefaultRangeMultiplier = 8;

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
  AGGREGATE
};

/**
 * @brief One metric of one row, with the disclosure beside the value
 * (FR-024).
 *
 * `availability` carries the gap state: a window whose endpoint
 * measured nothing reports `sg::counters::Availability::GAP`, and a
 * leaf the host cannot count keeps its refusal kind (FR-023, FR-025).
 */
struct MetricValue
{
  double value = 0.0;
  double runningRatio = 1.0;
  bool scaled = false;
  // The field name is the contract name. The type spelling is Availability.
  sg::counters::Availability availability =
      sg::counters::Availability::COUNTABLE;
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
  double timePerIterationNs = 0.0;
  std::vector<MetricValue> metrics;
  double overheadFloorNs = 0.0;
  Statistics timeStats;
  std::vector<Statistics> metricStats;
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
    [[nodiscard]] auto operator!=(
        [[maybe_unused]] const Cursor& other) const noexcept -> bool
    {
      if (m_remaining != 0) {
        return true;
      }
      if (m_state != nullptr) [[unlikely]] {
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
     * \pre none
     * \post none
     */
    [[nodiscard]] auto operator*() const noexcept -> std::uint64_t
    {
      return m_start - m_remaining;
    }

  private:
    friend class State;

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
    if (m_skipRequested) {
      return Cursor {0};
    }
    ++m_loopsStarted;
    openWindow();
    SG_ENSURE(m_windowOpened || m_recorder == nullptr,
              "the loop entry takes the entry sample (FR-017)");
    return Cursor {*this, m_iterations};
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

private:
  friend class detail::Runner;

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

  auto openWindow() noexcept -> void
  {
    SG_ASSERT(m_recorder != nullptr,
              "the harness constructs every state with the recorder that "
              "samples its window");
    if (!m_windowOpened) {
      m_recorder->sample();
      m_windowOpened = true;
    }
  }

  auto takeExitSample() noexcept -> void
  {
    SG_ASSERT(m_recorder != nullptr,
              "the harness constructs every state with the recorder that "
              "samples its window");
    if (m_windowOpened && !m_windowClosed) {
      m_recorder->sample();
      m_windowClosed = true;
    }
  }

  [[gnu::cold]] auto closeWindow() noexcept -> void
  {
    m_loopCompleted = true;
    takeExitSample();
  }

  // A run owns exactly one point pair, however it ended, so the pair
  // index of the next run stays aligned with the recorder (FR-017).
  auto topUpWindow() noexcept -> void
  {
    openWindow();
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
   * what its method measures on.
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
   * what `setUp` built.
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

}  // namespace detail

/**
 * @brief The registration result: the caller's view of one registry
 * entry (E-02).
 *
 * Every setter writes one option of the entry before the run starts.
 * A handle copied or moved names the same entry.
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
   * @brief Append the label of the next free argument position of this
   * family (FR-001, E-04).
   *
   * \pre the handle names a registered entry, the run has not started
   *      for this benchmark, and the family arity stands unset or at
   *      one (E-02, FR-006)
   * \post the entry carries the label at that position, and the
   *       returned handle is this handle (FR-001)
   */
  auto argName(std::string_view label) -> BenchmarkHandle&;

  /**
   * @brief Append the labels of every argument position of this family
   * (FR-001, E-04).
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
 * @brief Register `fn` at namespace scope before `main` begins, under
 * the name of `fn` (FR-001, R-09, R-10).
 */
#define SG_BENCHMARK(fn) \
  namespace \
  { \
  struct SgBenchmarkRegistrar_##fn \
  { \
    SgBenchmarkRegistrar_##fn() \
    { \
      (void)::sg::registerBenchmark(&(fn), #fn); \
    } \
  }; \
  const SgBenchmarkRegistrar_##fn sgBenchmarkRegistrar_##fn {}; \
  }

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
 * \pre `fn` takes a `State&` followed by the captured arguments, and
 *      the registration runs before the run starts (E-02)
 * \post the registry holds one family record named `fn/captureName`
 *       whose callable passes the captured values to `fn` (FR-013)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance `fn/captureName` (FR-013)
 */
#define SG_BENCHMARK_CAPTURE(fn, captureName, ...) \
  static const struct SgBenchmarkRegistrar_##fn##_##captureName \
  { \
    SgBenchmarkRegistrar_##fn##_##captureName() \
    { \
      (void)::sg::registerBenchmark( \
          [](::sg::State& state) { (fn)(state __VA_OPT__(, ) __VA_ARGS__); }, \
          #fn "/" #captureName); \
    } \
  } sgBenchmarkRegistrar_##fn##_##captureName {};

// The paste chain of `SG_BENCHMARK_TEMPLATE`. The generated identifier
// cannot spell a type list, so it carries the source line of the
// registration: `__LINE__` is standard C++, and two registrations of
// one function template on one source line would collide, which no
// suite of this feature does. The middle link expands that id before
// it is pasted, because a parameter that stands next to `##` is
// pasted unexpanded.
#define SG_BENCHMARK_TEMPLATE_REGISTRAR(fn, id, ...) \
  static const struct SgBenchmarkRegistrar_##fn##_##id \
  { \
    SgBenchmarkRegistrar_##fn##_##id() \
    { \
      (void)::sg::registerBenchmark(&fn<__VA_ARGS__>, \
                                    #fn "<" #__VA_ARGS__ ">"); \
    } \
  } sgBenchmarkRegistrar_##fn##_##id {};

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
 * \post the registry holds one family record named `fn<types>` whose
 *       callable is the instantiation for exactly those types
 *       (FR-014)
 * \invariant one macro site registers one family, and that family
 *            expands to the one instance its name states (FR-014)
 */
#define SG_BENCHMARK_TEMPLATE(fn, ...) \
  SG_BENCHMARK_TEMPLATE_EXPAND(fn, __LINE__, __VA_ARGS__)

// The shared machinery of the seven fixture macros. A macro site
// generates one derived class, named by pasting
// SgBenchmarkFixture_ plus the class plus the method, and one
// registrar object in the `SgBenchmarkRegistrar_##fn` shape of
// `SG_BENCHMARK` (R-13); the identifiers carry no leading
// underscore and no `__` (FR-023). The derived class holds the method
// the macro site defines, the instance name the registrar registers,
// and the one slot naming the fixture object of the run in progress:
// the registered factory stores the object it builds into that slot,
// and the registered callable reads it, so the method runs on the
// object the run owns (R-09). The harness runs one thread (FR-019), so
// one slot per macro site stands; the threads spec of the roadmap
// moves the slot when that spec lands.

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
  static const struct SgBenchmarkRegistrar_##FixtureClass##_##Method \
  { \
    SgBenchmarkRegistrar_##FixtureClass##_##Method() \
    { \
      (void)::sg::registerFixtureBenchmark( \
          [](::sg::State& state) \
          { DerivedClass::sgCurrentCase->sgBenchmarkCase(state); }, \
          DerivedClass::sgFixtureName, \
          []() -> std::unique_ptr<::sg::Fixture> \
          { \
            auto object = std::make_unique<DerivedClass>(); \
            DerivedClass::sgCurrentCase = object.get(); \
            return object; \
          }); \
    } \
  } sgBenchmarkRegistrar_##FixtureClass##_##Method {}

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
 * under the name that macro's generated class carries (FR-016).
 *
 * \pre `SG_BENCHMARK_DEFINE_F` named the same pair earlier in the same
 *      scope, and the registration runs before the run starts (E-02)
 * \post the registry holds one family record named
 *       `FixtureClass/Method` whose callable is the defined method
 *       (FR-016)
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
  static const struct SgBenchmarkRegistrar_##FixtureClass##_##Method##_##id \
  { \
    SgBenchmarkRegistrar_##FixtureClass##_##Method##_##id() \
    { \
      (void)::sg::registerFixtureBenchmark( \
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
          }); \
    } \
  } sgBenchmarkRegistrar_##FixtureClass##_##Method##_##id {}

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
