#ifndef SPEEDGUN_NG_COUNTERS_FAKE_HPP
#define SPEEDGUN_NG_COUNTERS_FAKE_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
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
 * followed by a per-sample tail (FR-036, T014).
 *
 * When the explicit sequence is exhausted, the script keeps stepping
 * the tail. A `tail_delta` of zero holds the last value, freezing the
 * leaf. A `delta_seed` steps the seeded per-sample delta sequence
 * documented beside `set_points` and supersedes `tail_delta`, so one
 * seed reproduces a whole workload.
 */
struct fake_script
{
  std::vector<std::uint64_t> points;
  std::uint64_t tail_delta = 0;
  std::optional<std::uint64_t> delta_seed;
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
  Availability avail = Availability::COUNTABLE;
  ReadMode mode = ReadMode::SYSCALL;
  bool ratio_pair = false;
  fake_script script;
  std::size_t position = 0;
  std::uint64_t last = 0;
  // The seeded tail's running state, held across sampling actions so
  // the sequence resumes where the previous one stopped.
  std::uint64_t delta_state = 0;
  // The one-based sampling actions this leaf measures nothing on. The
  // window reads them per leaf, so a leaf that gaps marks only the plans
  // that sample it (FR-007).
  std::vector<std::size_t> gaps;

  /**
   * @brief Whether this leaf measures nothing on the one-based
   * sampling `action`.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto gaps_at(const std::uint64_t action) const -> bool
  {
    for (const auto scripted : gaps) {
      if (scripted == action) {
        return true;
      }
    }
    return false;
  }
};

/**
 * @brief The scripted provider (FR-036).
 *
 * Objects and counters are declared on the provider before
 * registration; the machine root object exists implicitly whenever a
 * counter is attached to it.
 */
class SPEEDGUN_NG_EXPORT fake_provider final : public ProviderIface
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
   * `ratio_pair` declares that the owning object discloses a time
   * pair, which the seam reads as the counters named `enabled` and
   * `running` on the same object. A fold over such a leaf multiplies
   * their delta ratio into its disclosure (FR-019, FR-041).
   *
   * \pre `objectPath` is non-empty.
   * \post Enumeration will report the entry with the given unit,
   *       availability, mode, and ratio-pair declaration.
   */
  auto add_counter(std::string_view objectPath,
                   std::string_view name,
                   std::string_view unit,
                   std::string_view description,
                   Availability avail = Availability::COUNTABLE,
                   ReadMode mode = ReadMode::SYSCALL,
                   bool ratio_pair = false) -> fake_provider&;

  /**
   * @brief Scripts one leaf with an explicit cumulative sequence.
   *
   * Once the sequence is exhausted the tail steps by `tail_delta`.
   * A `delta_seed` puts the tail on the seeded per-sample delta
   * sequence instead: the leaf adds `step(seed)`, then
   * `step(step(seed))`, then `step(step(step(seed)))`, where one step is
   * `state = (state * 37 + 11) mod 2^16`. A single-digit multiplier
   * and a power-of-two modulus keep the whole tail reproducible by
   * hand, which the suite's exactness tests need (T014).
   *
   * \pre the counter was declared.
   * \post Sampling yields the scripted points in order, one per
   *       sampling action.
   */
  auto set_points(std::string_view objectPath,
                  std::string_view name,
                  std::vector<std::uint64_t> points,
                  std::uint64_t tail_delta = 0,
                  std::optional<std::uint64_t> delta_seed = std::nullopt)
      -> fake_provider&;

  /**
   * @brief The number of `readPoints` actions performed by readers
   * this provider opened (US1 scenario 5).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto read_actions() const noexcept -> std::uint64_t
  {
    return m_read_actions.load(std::memory_order_relaxed);
  }

  /**
   * @brief Scripts the actions one leaf measures nothing on (FR-007).
   *
   * `actions` names the one-based sampling actions at which this leaf
   * publishes a zero beside a disclosure of `Availability::GAP`, which is
   * how a caller distinguishes a measured zero from an action that
   * measured nothing. Every other action publishes the leaf's scripted
   * point beside a disclosure of the entry's own countability value. The
   * script is per leaf, so only a plan that samples this leaf sees its
   * gaps.
   *
   * \pre the counter was declared.
   * \post Sampling this leaf at a named action publishes a zero and marks
   *       the action; the script does not advance there.
   */
  auto set_gap_actions(std::string_view objectPath,
                       std::string_view name,
                       std::vector<std::size_t> actions) -> fake_provider&;

  void enumerate(ObjectSink& sink) const override;

  std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                     const Target& where) override;

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
  std::atomic<std::uint64_t> m_read_actions {0};

  [[nodiscard]] auto counter(const std::string& objectPath,
                             const std::string& name) -> fake_counter_data&;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_FAKE_HPP
