#ifndef SG_COUNTERS_FAKE_HPP
#define SG_COUNTERS_FAKE_HPP

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
 * the tail. A `tailDelta` of zero holds the last value, freezing the
 * leaf. A `deltaSeed` steps the seeded per-sample delta sequence
 * documented beside `setPoints` and supersedes `tailDelta`, so one
 * seed reproduces a whole workload.
 */
struct FakeScript
{
  std::vector<std::uint64_t> points;
  std::uint64_t tailDelta = 0;
  std::optional<std::uint64_t> deltaSeed;
};

namespace detail
{
struct FakeWindow;
}

/**
 * @brief One scripted counter: catalog metadata, script, and playback
 * position.
 */
struct FakeCounterData
{
  std::string description;
  std::string unit;
  Availability avail = Availability::COUNTABLE;
  ReadMode mode = ReadMode::SYSCALL;
  bool ratioPair = false;
  FakeScript script;
  std::size_t position = 0;
  std::uint64_t last = 0;
  // The seeded tail's running state, held across sampling actions so
  // the sequence resumes where the previous one stopped.
  std::uint64_t deltaState = 0;
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
  [[nodiscard]] auto gapsAt(const std::uint64_t action) const -> bool
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
class SPEEDGUN_NG_EXPORT FakeProvider final : public ProviderIface
{
public:
  /**
   * @brief An empty provider with no objects.
   *
   * \pre none
   * \post none
   */
  FakeProvider();

  FakeProvider(const FakeProvider&) = delete;
  auto operator=(const FakeProvider&) -> FakeProvider& = delete;
  FakeProvider(FakeProvider&&) = delete;
  auto operator=(FakeProvider&&) -> FakeProvider& = delete;

  /**
   * @brief Releases the scripted objects.
   *
   * \pre none
   * \post none
   */
  ~FakeProvider() override;

  /**
   * @brief Declares an object with its kind and description.
   *
   * \pre `path` is a canonical structured path; the machine root may
   *      not be redeclared.
   * \post Enumeration will report the object with the given kind and
   *       description.
   */
  auto addObject(std::string_view path,
                 std::string_view kind,
                 std::string_view description) -> FakeProvider&;

  /**
   * @brief Declares an object with a platform alias.
   *
   * \pre `path` is canonical; `alias` is a platform instance name.
   * \post Enumeration will report the alias, and both spellings
   *       resolve to the same object.
   */
  auto addObject(std::string_view path,
                 std::string_view alias,
                 std::string_view kind,
                 std::string_view description) -> FakeProvider&;

  /**
   * @brief Declares one named counter on one object.
   *
   * `unit` is the unit token mapped through the closed switch at
   * registration. The counter starts with a frozen script at zero.
   * Declaring a counter on an undeclared path auto-creates that
   * object with default kind and description.
   *
   * `ratioPair` declares that the owning object discloses a time
   * pair, which the seam reads as the counters named `enabled` and
   * `running` on the same object. A fold over such a leaf multiplies
   * their delta ratio into its disclosure (FR-019, FR-041).
   *
   * \pre `objectPath` is non-empty.
   * \post Enumeration will report the entry with the given unit,
   *       availability, mode, and ratio-pair declaration.
   */
  auto addCounter(std::string_view objectPath,
                  std::string_view name,
                  std::string_view unit,
                  std::string_view description,
                  Availability avail = Availability::COUNTABLE,
                  ReadMode mode = ReadMode::SYSCALL,
                  bool ratioPair = false) -> FakeProvider&;

  /**
   * @brief Scripts one leaf with an explicit cumulative sequence.
   *
   * Once the sequence is exhausted the tail steps by `tailDelta`.
   * A `deltaSeed` puts the tail on the seeded per-sample delta
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
  auto setPoints(std::string_view objectPath,
                 std::string_view name,
                 std::vector<std::uint64_t> points,
                 std::uint64_t tailDelta = 0,
                 std::optional<std::uint64_t> deltaSeed = std::nullopt)
      -> FakeProvider&;

  /**
   * @brief The number of `readPoints` actions performed by readers
   * this provider opened (US1 scenario 5).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto readActions() const noexcept -> std::uint64_t
  {
    return m_readActions.load(std::memory_order_relaxed);
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
  auto setGapActions(std::string_view objectPath,
                     std::string_view name,
                     std::vector<std::size_t> actions) -> FakeProvider&;

  void enumerate(ObjectSink& sink) const override;

  std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                     const Target& where) override;

private:
  friend struct detail::FakeWindow;

  struct ObjectSeedData
  {
    std::string kind;
    std::string alias;
    std::string description;
    std::map<std::string, FakeCounterData> counters;
  };

  std::map<std::string, ObjectSeedData> m_objects;
  std::atomic<std::uint64_t> m_readActions {0};

  [[nodiscard]] auto counter(const std::string& objectPath,
                             const std::string& name) -> FakeCounterData&;
};

}  // namespace sg::counters

#endif  // SG_COUNTERS_FAKE_HPP
