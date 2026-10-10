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
 * callback pair (E-07); and the fixture factory, null for a plain
 * family (E-06).
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
  bool runStarted = false;

  std::vector<std::vector<std::int64_t>> args;
  std::vector<std::string> argNames;
  std::int64_t rangeMultiplier = kDefaultRangeMultiplier;
  std::function<void(State&)> setup;
  std::function<void(State&)> teardown;
  // The factory of the one fixture object each run builds, through the
  // public `sg::Fixture` base; null for a plain family (E-06, R-09).
  std::function<std::unique_ptr<Fixture>()> fixtureFactory;
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

private:
  const RunOptions& m_options;
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

}  // namespace detail

}  // namespace sg

#endif  // SG_HARNESS_INTERNAL_HPP
