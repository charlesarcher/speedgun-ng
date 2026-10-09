#ifndef SG_HARNESS_INTERNAL_HPP
#define SG_HARNESS_INTERNAL_HPP

#include <atomic>
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

/**
 * @brief One registry record in registration order (E-01).
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
 * @brief Run one registry entry and produce its result value (E-04,
 * E-07).
 */
class Runner
{
public:
  explicit Runner(const RunOptions& options) noexcept
      : m_options(options)
  {
  }

  [[nodiscard]] auto run(RegistryEntry& entry) -> BenchmarkResult;

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
