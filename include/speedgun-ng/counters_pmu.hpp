#ifndef SPEEDGUN_NG_COUNTERS_PMU_HPP
#define SPEEDGUN_NG_COUNTERS_PMU_HPP

/**
 * @file counters_pmu.hpp
 * @brief The shipped hardware event provider: the platform's event
 * sources as a catalog of described hardware events, vendored tables
 * merged with kernel-discovered aliases (FR-037, FR-042).
 */

#include <memory>

#include "speedgun-ng/counters_provider.hpp"

namespace sg::counters
{

namespace detail
{
struct pmu_state;
}

/**
 * @brief The hardware event provider: the rich catalog and its probed
 * availability (FR-037..FR-041).
 *
 * On Linux each event source device becomes an object whose catalog
 * merges the vendored event tables with kernel-discovered aliases,
 * kernel discoveries winning conflicts, every entry described.
 * Availability is a probe fact per entry: `countable`,
 * `permission_blocked`, or `not_encodable` (FR-039). Every read mode
 * models the enabled and running times as ordinary cumulative leaves
 * beside their group, so each fold computes its multiplex ratio as the
 * pair's delta quotient (FR-041). Off Linux the provider keeps the
 * identical interface and seeds no objects: the reduced catalog is the
 * whole difference (FR-042).
 */
class SPEEDGUN_NG_EXPORT pmu_provider final : public ProviderIface
{
public:
  /**
   * @brief Constructs the provider, building the merged catalog:
   * sysfs discovery, vendored table selection by CPU identification
   * (FR-038), and per-entry probe (FR-039).
   *
   * \pre none
   * \post none
   */
  pmu_provider();

  /**
   * @brief The released provider state.
   *
   * \pre none
   * \post none
   */
  ~pmu_provider() override;

  pmu_provider(const pmu_provider&) = delete;
  pmu_provider(pmu_provider&&) = delete;
  auto operator=(const pmu_provider&) -> pmu_provider& = delete;
  auto operator=(pmu_provider&&) -> pmu_provider& = delete;

  /**
   * @brief Reports every event-source object with its merged,
   * described catalog entries.
   *
   * \pre none
   * \post none
   */
  void enumerate(ObjectSink& sink) const override;

  /**
   * @brief Opens the syscall-mode window managing the requested
   * leaves; null when a leaf is not this provider's.
   *
   * \pre none
   * \post none
   */
  std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                     const Target& where) override;

private:
  std::unique_ptr<detail::pmu_state> m_state;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_PMU_HPP
