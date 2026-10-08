#ifndef SG_COUNTERS_CLOCK_HPP
#define SG_COUNTERS_CLOCK_HPP

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
struct ClockWindow;
}

/**
 * @brief The shipped clock provider: always countable, zero
 * privileges (FR-033, R-007).
 *
 * Seeds the machine object with `monotonic`, `monotonic_raw`,
 * `thread_cpu`, and `process_cpu` leaves in nanoseconds at syscall
 * read mode, plus a `tsc` leaf at fast tick mode wherever the build
 * executes the time-stamp instruction (FR-001, FR-011). The count
 * carries no rate: the entry's frequency field and its scaled flag
 * keep their zero defaults (FR-002). A build without the instruction
 * omits the `tsc` leaf entirely, which is a catalog fact with no API
 * difference (R-007).
 *
 * This clause states no order on any leaf's behalf. Each leaf states
 * its own guarantee below, and every guarantee a leaf states has a
 * registered test that exercises it (FR-028, FR-029).
 *
 * - `machine/monotonic` reads `CLOCK_MONOTONIC` through the vDSO. A
 *   sample does not fall below an earlier sample taken on the same
 *   thread, across the process's life.
 * - `machine/monotonic_raw` reads the same clock with no NTP
 *   adjustment, and keeps the guarantee `machine/monotonic` states.
 * - `machine/thread_cpu` reads per-thread CPU time. A sample does not
 *   fall below an earlier sample taken on the same thread. A new
 *   thread's first sample can fall below an earlier sample taken on
 *   another thread, because the counter belongs to the thread
 *   (FR-030).
 * - `machine/process_cpu` reads per-process CPU time. A sample does
 *   not fall below an earlier sample taken on another thread of the
 *   same process, because the counter belongs to the process.
 * - `machine/tsc` reads the processor's cycle counter with no ordering
 *   fence. Its value is ordered only where one thread takes both
 *   endpoints of the window it measures, and that is a precondition on
 *   the caller: one thread takes both endpoints. The leaf claims no
 *   order across threads and no order across processors, and it gains
 *   no fence: a fence helps only a caller reading across threads, and
 *   the harness takes both endpoints on one thread (FR-031, Principle
 *   VII).
 */
class SPEEDGUN_NG_EXPORT ClockProvider final : public ProviderIface
{
public:
  /**
   * @brief Constructs the provider.
   *
   * \pre none
   * \post none
   */
  ClockProvider();

  /**
   * @brief The released provider state.
   *
   * \pre none
   * \post none
   */
  ~ClockProvider() override;

  ClockProvider(const ClockProvider&) = delete;
  ClockProvider(ClockProvider&&) = delete;
  auto operator=(const ClockProvider&) -> ClockProvider& = delete;
  auto operator=(ClockProvider&&) -> ClockProvider& = delete;

  /**
   * @brief Seeds the machine object with the clock leaves.
   *
   * \pre none
   * \post none
   */
  void enumerate(ObjectSink& sink) const override;

  /**
   * @brief Opens a reader managing the requested clock leaves.
   *
   * \pre none
   * \post none
   */
  std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                     const Target& where) override;
};

}  // namespace sg::counters

#endif  // SG_COUNTERS_CLOCK_HPP
