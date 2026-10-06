#ifndef SPEEDGUN_NG_COUNTERS_PROVIDER_HPP
#define SPEEDGUN_NG_COUNTERS_PROVIDER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_provider.hpp
 * @brief The provider seam: what every count source implements and what
 * the system calls, once at setup and once per sampling action.
 *
 * A provider registers objects with named catalog entries
 * (`enumerate`), and yields cumulative points for the leaves the system
 * asks it to manage (`open`, then `read_points` per action). Virtual
 * calls belong to setup; the compiled read path reaches a provider
 * through the `window_reader::read_thunk` slot its window fills in the
 * window constructor, one static function naming that window's own read
 * (R-004, FR-022).
 */

namespace sg::counters
{

/**
 * @brief One catalog entry as a provider seeds it.
 *
 * `unit` is the provider's unit token; the system maps it through the
 * closed switch at registration (FR-017).
 *
 */
struct catalog_seed
{
  std::string_view name;
  std::string_view description;
  std::string_view unit;
  availability avail = availability::countable;
  read_mode mode = read_mode::syscall;
  std::uint64_t frequency_hz = 0;  // fixed-rate calibration, 0 elsewhere
  bool scaled = false;  // platform-scaled tick source disclosure
  // True when the source discloses a time pair (`enabled` and
  // `running` leaves on the same object) so folds can compute a
  // multiplex ratio (FR-019, FR-041).
  bool has_ratio_pair = false;
};

/**
 * @brief One countable object as a provider seeds it, with its entries.
 *
 * `path` is the canonical structured spelling; `alias` is the optional
 * platform instance name, empty when the object has none (FR-002).
 * String views must stay valid for the duration of the `enumerate`
 * call; the system copies what it keeps.
 *
 */
struct object_seed
{
  std::string_view kind;
  std::string_view path;
  std::string_view alias;
  std::string_view description;
  std::vector<catalog_seed> entries;
};

/**
 * @brief Receiver for provider enumeration at registration time.
 *
 * The system implements this; a provider calls `add_object` once per
 * object it owns. A duplicate canonical path under one parent, or a
 * duplicate counter name within one object, is reported after
 * `enumerate` returns, through the `register_provider` result, with the
 * tree left unchanged (FR-008).
 *
 */
class SPEEDGUN_NG_EXPORT object_sink
{
public:
  /**
   * @brief The empty receiver state.
   *
   * \pre none
   * \post none
   */
  // LCOV_EXCL_LINE : coverage exclusion (T066, P2 recorded in
  // specs/007-counters-and-timers/plan.md Complexity Tracking): a defaulted
  // default constructor emits no code, so gcov attaches a line record no
  // execution can ever advance. Verified with `gcov -b`: this line reports
  // an unexecuted block on every build while the copy constructor on the
  // next line reports a count.
  object_sink() = default;  // LCOV_EXCL_LINE

  object_sink(const object_sink&) = default;
  object_sink(object_sink&&) = delete;
  auto operator=(const object_sink&) -> object_sink& = default;
  auto operator=(object_sink&&) -> object_sink& = delete;

  /**
   * @brief Destruction through the base pointer.
   *
   * \pre none
   * \post none
   */
  virtual ~object_sink() = default;

  /**
   * @brief Accepts one object with its catalog entries.
   *
   * The seed's strings are copied before the call returns.
   *
   * \pre none
   * \post none
   */
  virtual void add_object(const object_seed& seed) = 0;
};

/**
 * @brief The leaves one provider is asked to manage.
 *
 * Addresses are canonical leaf spellings, `<object path>/<name>`, in
 * stable order; `read_points` yields one point per address in exactly
 * this order (C-PRO-2).
 *
 * `disclosure_column` names the managed column the sampling action writes
 * last, beside the counts and the ratio pair's two columns, and it is the
 * column no leaf address resolves to. The request that owns the plan's
 * last group carries it; every earlier request carries
 * `no_disclosure_column`, because a window writes the column once per
 * action (FR-007).
 *
 */
struct leaf_set
{
  /// @brief No request writes the disclosure column (FR-007).
  static constexpr std::size_t no_disclosure_column =
      static_cast<std::size_t>(-1);

  std::vector<std::string> addresses;
  std::size_t disclosure_column = no_disclosure_column;
};

/**
 * @brief What a plan samples on: the current thread, or a pinned cpu.
 */
enum class target_kind : std::uint8_t
{
  thread,
  cpu
};

/**
 * @brief Sampling target handed to a provider at open (FR-031).
 *
 * `cpu` is meaningful only for `target_kind::cpu`; a `thread` target
 * ignores it.
 *
 */
struct target
{
  target_kind kind = target_kind::thread;
  int cpu = -1;
};

/**
 * @brief The per-action write cursor a provider fills.
 *
 * A concrete, non-virtual cursor over the column buffer being recorded
 * for one sampling action: each `put` appends the next managed leaf's
 * cumulative point. The cursor advances; the recorder commits the row
 * after the action completes.
 *
 * \invariant A finished sampling action wrote one point per managed
 *            column, which is the one-sampling-action obligation
 *            FR-047 binds the reader to.
 */
class point_sink
{
public:
  /**
   * @brief Positions the cursor over `columns`, `leaf_count` columns of
   * `stride` rows, at row `row`.
   *
   * \pre columns is non-null and holds `leaf_count` columns of `stride`
   *      `uint64` cells; `row` is within `stride`.
   * \post none
   */
  point_sink(std::uint64_t* columns,
             std::size_t leaf_count,
             std::size_t stride,
             std::size_t row) noexcept
      : m_columns(columns)
      , m_leaf_count(leaf_count)
      , m_stride(stride)
      , m_row(row)
  {
    SG_REQUIRE(columns != nullptr && row < stride,
               "point sink positioned over a valid column block and row");
  }

  /**
   * @brief Appends one cumulative point to the next managed column.
   *
   * \pre Fewer than `leaf_count` points have been put this action.
   * \post The point lands in the column matching the call index, at the
   *       constructed row; the call index advances by one.
   */
  void put(const std::uint64_t value) noexcept
  {
    const std::size_t index = m_index;
    SG_REQUIRE(index < m_leaf_count,
               "point sink filled beyond the managed leaf count");
    m_columns[index * m_stride + m_row] = value;
    ++m_index;
    SG_ENSURE(
        m_columns[index * m_stride + m_row] == value && m_index == index + 1,
        "the point lands in the column matching the call index");
  }

  /**
   * @brief Checks that a finished action wrote one point per managed
   * column (FR-047).
   *
   * The read path calls this where the row is committed, so a reader
   * that filled fewer columns than the plan compiled fails the
   * obligation where the shortfall is visible.
   *
   * \pre none
   * \post none
   */
  void check_action() const noexcept
  {
    SG_INVARIANT(m_index == m_leaf_count,
                 "one action writes one point per managed column (FR-047)");
  }

private:
  std::uint64_t* m_columns = nullptr;
  std::size_t m_leaf_count = 0;
  std::size_t m_stride = 0;
  std::size_t m_row = 0;
  std::size_t m_index = 0;
};

/**
 * @brief The sampling primitive a provider implements for one open
 * window.
 *
 * `read_points` fills exactly one cumulative point per managed leaf, in
 * `leaf_set` order, within one sampling action (C-PRO-2). Every window
 * constructor calls `set_thunk` with a static function that reads that
 * window's own points, so the compiled plan reaches the read with no
 * vtable lookup (FR-022, R-004). A window that supplies no thunk keeps
 * `default_thunk`, which routes to `read_points`; that fallback costs
 * one vtable lookup per sampling action and serves a provider written
 * against this interface alone.
 *
 * \invariant `thunk` is non-null.
 */
class SPEEDGUN_NG_EXPORT window_reader
{
public:
  /**
   * @brief A reader at its initial state.
   *
   * \pre none
   * \post none
   */
  window_reader() = default;

  /**
   * @brief Direct-call signature the compiled plan stores; equal in
   * effect to `read_points`.
   */
  using read_thunk = void (*)(window_reader&, point_sink&) noexcept;

  window_reader(const window_reader&) = default;
  window_reader(window_reader&&) = delete;
  auto operator=(const window_reader&) -> window_reader& = default;
  auto operator=(window_reader&&) -> window_reader& = delete;

  /**
   * @brief Destruction through the base pointer.
   *
   * \pre none
   * \post none
   */
  virtual ~window_reader() = default;

  /**
   * @brief Yields one cumulative `uint64` point per managed leaf, in
   * `leaf_set` order, for one sampling action.
   *
   * The sink receives exactly one point per managed leaf, read within
   * this action (FR-011, FR-047); the obligation binds the
   * implementation.
   *
   * \pre none
   * \post none
   */
  virtual void read_points(point_sink& sink) noexcept = 0;

  /**
   * @brief Resolves the direct-call slot for the compiled read path.
   *
   * Called once per reader at plan finalization (setup region). The
   * slot holds the static function the window constructor installed
   * through `set_thunk`, and `default_thunk` when the window installed
   * none.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto resolve_thunk() noexcept -> read_thunk
  {
    check_thunk();
    return m_thunk;
  }

protected:
  /**
   * @brief Replaces the direct-call slot; for provider constructors.
   *
   * \pre fn is non-null and equivalent to `read_points` for this
   *      reader.
   * \post `resolve_thunk` returns fn.
   */
  void set_thunk(const read_thunk fn) noexcept
  {
    SG_REQUIRE(fn != nullptr, "window reader thunk is non-null");
    m_thunk = fn;
    SG_ENSURE(m_thunk == fn, "resolve_thunk returns the installed thunk");
  }

private:
  static auto default_thunk(window_reader& reader,
                            point_sink& sink) noexcept -> void
  {
    reader.read_points(sink);
  }

  auto check_thunk() const noexcept -> void
  {
    SG_INVARIANT(m_thunk != nullptr, "window reader thunk is set");
  }

  read_thunk m_thunk = &default_thunk;
};

/**
 * @brief The registration base every provider derives from.
 *
 * The system stores providers through this base; virtual calls happen
 * only during registration, system open, and plan compile (R-004).
 *
 */
class SPEEDGUN_NG_EXPORT provider_iface
{
public:
  /**
   * @brief The unregistered provider state.
   *
   * \pre none
   * \post none
   */
  provider_iface() = default;

  provider_iface(const provider_iface&) = default;
  provider_iface(provider_iface&&) = delete;
  auto operator=(const provider_iface&) -> provider_iface& = default;
  auto operator=(provider_iface&&) -> provider_iface& = delete;

  /**
   * @brief Destruction through the base pointer.
   *
   * \pre none
   * \post none
   */
  virtual ~provider_iface() = default;

  /**
   * @brief Reports every object this provider owns, with catalog
   * entries.
   *
   * Each owned object is handed to the sink exactly once, with
   * canonical paths and described entries (C-PRO-1); the obligation
   * binds the implementation.
   *
   * \pre none
   * \post none
   */
  virtual void enumerate(object_sink& sink) const = 0;

  /**
   * @brief Opens the sampling window for the given leaves and target.
   *
   * The reader yields points for every address in `leaves`, in order,
   * per sampling action. A provider that cannot manage the leaves
   * returns null; the system reports a recoverable error. The
   * obligation binds the implementation.
   *
   * \pre none
   * \post none
   */
  virtual std::unique_ptr<window_reader> open(const leaf_set& leaves,
                                              const target& where) = 0;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_PROVIDER_HPP
