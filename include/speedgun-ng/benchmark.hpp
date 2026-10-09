#ifndef SG_BENCHMARK_HPP
#define SG_BENCHMARK_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
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
 */
struct BenchmarkResult
{
  std::string name;
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
 * untimed region (FR-004, FR-005).
 *
 * \invariant The loop cursor never passes the iteration count of the
 *            current run, and the loop reads no interrupt flag: the
 *            runner reads that flag after a run completes (R-06).
 */
class SPEEDGUN_NG_EXPORT State
{
public:
  /**
   * @brief The range-for cursor of the timed loop (E-03).
   */
  class Cursor
  {
  public:
    /**
     * @brief True while the loop still runs: the cursor has not reached
     * the end and no skip ended the run.
     *
     * \pre none
     * \post none
     */
    [[nodiscard]] auto operator!=(const Cursor& other) const noexcept -> bool
    {
      return !m_state->loopEndRequested() && m_index != other.m_index;
    }

    /**
     * @brief Advances the cursor by one iteration.
     *
     * \pre the cursor sits inside the timed loop (class invariant)
     * \post the cursor names the next iteration
     */
    auto operator++() noexcept -> Cursor&
    {
      SG_INVARIANT(m_index < m_state->m_iterations,
                   "the loop cursor stays inside the timed loop");
      m_index = m_index + 1;
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
      return m_index;
    }

  private:
    friend class State;

    Cursor(State& state, const std::uint64_t index) noexcept
        : m_state(&state)
        , m_index(index)
    {
    }

    State* m_state;
    std::uint64_t m_index;
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
   * \pre none
   * \post the returned cursor names iteration zero of this run
   */
  auto begin() noexcept -> Cursor
  {
    SG_INVARIANT(m_index <= m_iterations,
                 "the loop cursor stays inside the timed loop");
    return Cursor {*this, 0};
  }

  /**
   * @brief The cursor one past the last iteration of the timed loop.
   *
   * \pre none
   * \post the returned cursor names the end of this run's loop
   */
  auto end() noexcept -> Cursor { return Cursor {*this, m_iterations}; }

  /**
   * @brief Skip the benchmark with an error reason (FR-031).
   *
   * \pre called from inside the benchmark function
   * \post the timed loop ends, the reason is recorded, and no
   *       statistics print for this benchmark
   */
  auto skipWithError(std::string_view reason) noexcept -> void
  {
    m_skipRequested = true;
    m_outcome = RunOutcome::SKIPPED;
    m_reason.assign(reason);
    SG_ENSURE(m_outcome == RunOutcome::SKIPPED,
              "a skip records the skipped outcome (FR-031)");
  }

  /**
   * @brief Skip the benchmark with a message reason (FR-031).
   *
   * \pre called from inside the benchmark function
   * \post the timed loop ends, the reason is recorded, and no
   *       statistics print for this benchmark
   */
  auto skipWithMessage(std::string_view reason) noexcept -> void
  {
    m_skipRequested = true;
    m_outcome = RunOutcome::SKIPPED;
    m_reason.assign(reason);
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

private:
  friend class detail::Runner;

  explicit State(const std::uint64_t iterations) noexcept
      : m_iterations(iterations)
  {
  }

  [[nodiscard]] auto loopEndRequested() const noexcept -> bool
  {
    return m_skipRequested;
  }

  std::uint64_t m_iterations = 0;
  std::uint64_t m_index = 0;
  bool m_skipRequested = false;
  RunOutcome m_outcome = RunOutcome::MEASURED;
  std::string m_reason;
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
   * \pre the run has not started for this benchmark, and the
   *      expression's leaves resolve through the system catalog; a
   *      leaf the catalog lacks is a recoverable error naming the
   *      leaf (FR-023)
   * \post the entry carries the metric, and the report prints one
   *       column for it under `label`
   */
  template<class D>
  auto addMetric(const sg::counters::Expression<D>& expression,
                 std::string_view label) -> BenchmarkHandle&
  {
    return addMetricCore(detail::MetricSeed {
        expression.core, std::string(label), reportsPerIteration<D>()});
  }

private:
  friend class RegistryEntry;
  friend auto registerBenchmark(std::function<void(State&)> fn,
                                std::string_view name) -> BenchmarkHandle;

  explicit BenchmarkHandle(class RegistryEntry& entry) noexcept;

  auto addMetricCore(const detail::MetricSeed& seed) -> BenchmarkHandle&;

  RegistryEntry* m_entry = nullptr;
};

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
 * @brief Run the registered benchmarks named by the command line and
 * print the report (FR-033).
 *
 * A suite links `speedgun-ng::harness` and calls this from `main`
 * (D-1).
 *
 * \pre the command line follows the option table of
 *      `specs/015-benchmark-harness-core/contracts/cli.md`
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

}  // namespace sg

#endif  // SG_BENCHMARK_HPP
