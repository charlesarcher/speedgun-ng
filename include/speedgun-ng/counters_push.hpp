#ifndef SG_COUNTERS_PUSH_HPP
#define SG_COUNTERS_PUSH_HPP

/**
 * @file counters_push.hpp
 * @brief The shipped push provider: user hot-path counters, confined
 * to the creating thread, sampled by plain load (FR-035, R-008).
 */

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"

namespace sg::counters
{

namespace detail
{
struct PushWindow;
}

/**
 * @brief The shipped push provider: declared counters are ordinary
 * machine-object entries at push read mode (FR-035).
 *
 * Each declared counter lives on the thread that declared it. The
 * hot path adds through the returned handle (a plain non-atomic
 * increment); sampling reads a plain load, never an atomic RMW
 * (R-008).
 */
class SPEEDGUN_NG_EXPORT PushProvider final : public ProviderIface
{
public:
  /**
   * @brief The empty provider state.
   *
   * \pre none
   * \post none
   */
  PushProvider();

  /**
   * @brief The released provider state; handles to declared counters
   * must not be used afterwards.
   *
   * \pre none
   * \post none
   */
  ~PushProvider() override;

  PushProvider(const PushProvider&) = delete;
  PushProvider(PushProvider&&) = delete;
  auto operator=(const PushProvider&) -> PushProvider& = delete;
  auto operator=(PushProvider&&) -> PushProvider& = delete;

  /**
   * @brief Declares one push counter on the machine object, bound to
   * the calling thread, and yields its hot-path handle.
   *
   * `unit` is the unit token mapped through the closed switch at
   * registration.
   *
   * \pre `name` is non-empty.
   * \post Enumeration will report the entry at push read mode, and
   *       the returned handle adds to the declared counter.
   */
  [[nodiscard]] auto addCounter(std::string_view name,
                                std::string_view unit,
                                std::string_view description) -> PushCounter;

  /**
   * @brief Seeds the machine object with the declared counters.
   *
   * \pre none
   * \post none
   */
  void enumerate(ObjectSink& sink) const override;

  /**
   * @brief Opens a reader loading the requested push leaves; null when
   * a leaf is not this provider's.
   *
   * \pre Every address in `leaves` was declared on one thread
   *      (FR-035).
   * \post none
   */
  std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                     const Target& where) override;

private:
  friend struct detail::PushWindow;

  struct PushPoint
  {
    std::uint64_t value = 0;
    std::thread::id owner {};
    std::string name;
    std::string description;
    std::string unit;
  };

  std::deque<PushPoint> m_points;  // deque: handle addresses stay stable
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_PUSH_HPP
