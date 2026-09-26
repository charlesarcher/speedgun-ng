#ifndef SPEEDGUN_NG_COUNTERS_FAKE_HPP
#define SPEEDGUN_NG_COUNTERS_FAKE_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_fake.hpp
 * @brief The shipped fake provider: hand-driven deterministic point
 * sequences over arbitrary object trees, the exactness spine of the
 * test suite (FR-036, R-009).
 *
 * The fake is an ordinary provider: it registers objects, and yields
 * scripted cumulative points through the same window contract as any
 * out-of-tree source. Sequences may walk a leaf across 2^64 to
 * exercise the wrap (SC-006).
 */

namespace sg::counters
{

/**
 * @brief One scripted leaf: an explicit point sequence, optionally
 * followed by a constant per-sample delta (FR-036).
 *
 * When the explicit sequence is exhausted, the script keeps stepping
 * by `tail_delta` (zero holds the last value, freezing the leaf).
 */
struct fake_script
{
  std::vector<std::uint64_t> points;
  std::uint64_t tail_delta = 0;
};

namespace detail
{
struct fake_window;
}

/**
 * @brief One scripted counter: catalog metadata, script, and playback
 * position.
 */
struct fake_counter_data
{
  std::string description;
  std::string unit;
  availability avail = availability::countable;
  read_mode mode = read_mode::syscall;
  fake_script script;
  std::size_t position = 0;
  std::uint64_t last = 0;
};

/**
 * @brief The scripted provider (FR-036).
 *
 * Objects and counters are declared on the provider before
 * registration; the machine root object exists implicitly whenever a
 * counter is attached to it.
 */
class SPEEDGUN_NG_EXPORT fake_provider final : public provider_iface
{
public:
  /**
   * @brief An empty provider with no objects.
   *
   * \pre none
   * \post none
   */
  fake_provider();

  fake_provider(const fake_provider&) = delete;
  auto operator=(const fake_provider&) -> fake_provider& = delete;
  fake_provider(fake_provider&&) = delete;
  auto operator=(fake_provider&&) -> fake_provider& = delete;

  /**
   * @brief Releases the scripted objects.
   *
   * \pre none
   * \post none
   */
  ~fake_provider() override;

  /**
   * @brief Declares an object with its kind and description.
   *
   * \pre `path` is a canonical structured path; the machine root may
   *      not be redeclared.
   * \post Enumeration will report the object with the given kind and
   *       description.
   */
  auto add_object(std::string_view path,
                  std::string_view kind,
                  std::string_view description) -> fake_provider&;

  /**
   * @brief Declares an object with a platform alias.
   *
   * \pre `path` is canonical; `alias` is a platform instance name.
   * \post Enumeration will report the alias, and both spellings
   *       resolve to the same object.
   */
  auto add_object(std::string_view path,
                  std::string_view alias,
                  std::string_view kind,
                  std::string_view description) -> fake_provider&;

  /**
   * @brief Declares one named counter on one object.
   *
   * `unit` is the unit token mapped through the closed switch at
   * registration. The counter starts with a frozen script at zero.
   * Declaring a counter on an undeclared path auto-creates that
   * object with default kind and description.
   *
   * \pre `object_path` is non-empty.
   * \post Enumeration will report the entry with the given unit,
   *       availability, and mode.
   */
  auto add_counter(std::string_view object_path,
                   std::string_view name,
                   std::string_view unit,
                   std::string_view description,
                   availability avail = availability::countable,
                   read_mode mode = read_mode::syscall) -> fake_provider&;

  /**
   * @brief Scripts one leaf with an explicit cumulative sequence.
   *
   * \pre the counter was declared.
   * \post Sampling yields the scripted points in order, one per
   *       sampling action.
   */
  auto set_points(std::string_view object_path,
                  std::string_view name,
                  std::vector<std::uint64_t> points,
                  std::uint64_t tail_delta = 0) -> fake_provider&;

  /**
   * @brief The number of `read_points` actions performed by readers
   * this provider opened (US1 scenario 5).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto read_actions() const noexcept -> std::uint64_t
  {
    return m_read_actions;
  }

  void enumerate(object_sink& sink) const override;

  std::unique_ptr<window_reader> open(const leaf_set& leaves,
                                      const target& where) override;

private:
  friend struct detail::fake_window;

  struct object_seed_data
  {
    std::string kind;
    std::string alias;
    std::string description;
    std::map<std::string, fake_counter_data> counters;
  };

  std::map<std::string, object_seed_data> m_objects;
  std::uint64_t m_read_actions = 0;

  [[nodiscard]] auto counter(const std::string& object_path,
                             const std::string& name) -> fake_counter_data&;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_FAKE_HPP
