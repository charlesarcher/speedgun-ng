#ifndef SPEEDGUN_NG_COUNTERS_CLOCK_HPP
#define SPEEDGUN_NG_COUNTERS_CLOCK_HPP

/**
 * @file counters_clock.hpp
 * @brief The shipped system clock provider: monotonic wall time,
 * thread CPU time, process CPU time, and the raw time-stamp counter
 * wherever the build executes the instruction (FR-033; FR-001).
 */

#include <memory>

#include "speedgun-ng/counters_provider.hpp"

namespace sg::counters
{

namespace detail
{
struct clock_window;
}

/**
 * @brief The shipped clock provider: always countable, zero
 * privileges (FR-033, R-007).
 *
 * Seeds the machine object with `monotonic`, `thread_cpu`, and
 * `process_cpu` leaves in nanoseconds at syscall read mode, plus a
 * `tsc` leaf at fast tick mode wherever the build executes the
 * time-stamp instruction (FR-001, FR-011). The count carries no rate:
 * the entry's frequency field and its scaled flag keep their zero
 * defaults (FR-002). A build without the instruction omits the `tsc`
 * leaf entirely, which is a catalog fact with no API difference
 * (R-007).
 */
class SPEEDGUN_NG_EXPORT clock_provider final : public provider_iface
{
public:
  /**
   * @brief Constructs the provider.
   *
   * \pre none
   * \post none
   */
  clock_provider();

  /**
   * @brief The released provider state.
   *
   * \pre none
   * \post none
   */
  ~clock_provider() override;

  clock_provider(const clock_provider&) = delete;
  clock_provider(clock_provider&&) = delete;
  auto operator=(const clock_provider&) -> clock_provider& = delete;
  auto operator=(clock_provider&&) -> clock_provider& = delete;

  /**
   * @brief Seeds the machine object with the clock leaves.
   *
   * \pre none
   * \post none
   */
  void enumerate(object_sink& sink) const override;

  /**
   * @brief Opens a reader managing the requested clock leaves.
   *
   * \pre none
   * \post none
   */
  std::unique_ptr<window_reader> open(const leaf_set& leaves,
                                      const target& where) override;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_CLOCK_HPP
