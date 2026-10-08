#ifndef SG_COUNTERS_PROVIDER_HPP
#define SG_COUNTERS_PROVIDER_HPP

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
 * asks it to manage (`open`, then `readPoints` per action). Virtual
 * calls belong to setup; the compiled read path reaches a provider
 * through the `WindowReader::ReadThunk` slot its window fills in the
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
struct CatalogSeed
{
  std::string_view name;
  std::string_view description;
  std::string_view unit;
  Availability avail = Availability::COUNTABLE;
  ReadMode mode = ReadMode::SYSCALL;
  std::uint64_t frequencyHz = 0;  // fixed-rate calibration, 0 elsewhere
  bool scaled = false;  // platform-scaled tick source disclosure
  // True when the source discloses a time pair (`enabled` and
  // `running` leaves on the same object) so folds can compute a
  // multiplex ratio (FR-019, FR-041).
  bool hasRatioPair = false;
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
struct ObjectSeed
{
  std::string_view kind;
  std::string_view path;
  std::string_view alias;
  std::string_view description;
  std::vector<CatalogSeed> entries;
};

/**
 * @brief Receiver for provider enumeration at registration time.
 *
 * The system implements this; a provider calls `addObject` once per
 * object it owns. A duplicate canonical path under one parent, or a
 * duplicate counter name within one object, is reported after
 * `enumerate` returns, through the `registerProvider` result, with the
 * tree left unchanged (FR-008).
 *
 */
class SPEEDGUN_NG_EXPORT ObjectSink
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
  ObjectSink() = default;  // LCOV_EXCL_LINE

  ObjectSink(const ObjectSink&) = default;
  ObjectSink(ObjectSink&&) = delete;
  auto operator=(const ObjectSink&) -> ObjectSink& = default;
  auto operator=(ObjectSink&&) -> ObjectSink& = delete;

  /**
   * @brief Destruction through the base pointer.
   *
   * \pre none
   * \post none
   */
  virtual ~ObjectSink() = default;

  /**
   * @brief Accepts one object with its catalog entries.
   *
   * The seed's strings are copied before the call returns.
   *
   * \pre none
   * \post none
   */
  virtual void addObject(const ObjectSeed& seed) = 0;
};

/**
 * @brief The leaves one provider is asked to manage.
 *
 * Addresses are canonical leaf spellings, `<object path>/<name>`, in
 * stable order; `readPoints` yields one point per address in exactly
 * this order (C-PRO-2).
 *
 * `disclosureColumn` names the managed column the sampling action writes
 * last, beside the counts and the ratio pair's two columns, and it is the
 * column no leaf address resolves to. The request that owns the plan's
 * last group carries it; every earlier request carries
 * `kNoDisclosureColumn`, because a window writes the column once per
 * action (FR-007).
 *
 */
struct LeafSet
{
  /// @brief No request writes the disclosure column (FR-007).
  static constexpr std::size_t kNoDisclosureColumn =
      static_cast<std::size_t>(-1);

  std::vector<std::string> addresses;
  std::size_t disclosureColumn = kNoDisclosureColumn;
};

/**
 * @brief What a plan samples on: the current thread, or a pinned cpu.
 */
enum class TargetKind : std::uint8_t
{
  THREAD,
  CPU
};

/**
 * @brief Sampling target handed to a provider at open (FR-031).
 *
 * `CPU` is meaningful only for `TargetKind::CPU`; a `THREAD` target
 * ignores it.
 *
 */
struct Target
{
  TargetKind kind = TargetKind::THREAD;
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
class PointSink
{
public:
  /**
   * @brief Positions the cursor over `columns`, `leafCount` columns of
   * `stride` rows, at row `row`.
   *
   * \pre columns is non-null and holds `leafCount` columns of `stride`
   *      `uint64` cells; `row` is within `stride`.
   * \post none
   */
  PointSink(std::uint64_t* columns,
            std::size_t leafCount,
            std::size_t columnCount,
            std::size_t stride,
            std::size_t row) noexcept
      : m_columns(columns)
      , m_leafCount(leafCount)
      , m_columnCount(columnCount)
      , m_stride(stride)
      , m_row(row)
  {
    SG_REQUIRE(columns != nullptr && row < stride,
               "point sink positioned over a valid column block and row");
  }

  /**
   * @brief Appends one cumulative point to the next managed column.
   *
   * \pre Fewer than `leafCount` points have been put this action.
   * \post The point lands in the column matching the call index, at the
   *       constructed row; the call index advances by one.
   */
  void put(const std::uint64_t value) noexcept
  {
    const std::size_t index = m_index;
    SG_REQUIRE(index < m_leafCount,
               "point sink filled beyond the managed leaf count");
    m_columns[index * m_stride + m_row] = value;
    ++m_index;
    SG_ENSURE(
        m_columns[index * m_stride + m_row] == value && m_index == index + 1,
        "the point lands in the column matching the call index");
  }

  /**
   * @brief Writes one disclosure point into a named column.
   *
   * A plan with more than one read group gives each group its own
   * disclosure column, and the columns follow the leaves rather than
   * interleaving with them, so a provider's disclosure does not land in
   * the sequential cursor's next cell. The cursor is unchanged: the
   * disclosure is not a leaf and no leaf follows it, so `put` and
   * `checkAction` see exactly the leaf obligations (FR-002).
   *
   * \pre `column` is within the managed columns and this action has not
   *      already written that column.
   * \post the disclosure lands in `column` at the constructed row.
   */
  void putDisclosure(std::size_t column, std::uint64_t value) noexcept
  {
    SG_REQUIRE(column < m_columnCount,
               "disclosure column is within the managed columns");
    // The column base is a pointer into one contiguous buffer and the
    // index is the column the plan named, so the subscript is in range
    // by the requirement above.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    m_columns[(column * m_stride) + m_row] = value;
    m_disclosureWritten = true;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    SG_ENSURE(m_columns[(column * m_stride) + m_row] == value,
              "the disclosure lands in the column the plan named for it");
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
  void checkAction() const noexcept
  {
    // The obligation is one point per managed leaf. A plan with more than
    // one read group has a disclosure column per group, and each is written
    // through `putDisclosure`, which does not advance the cursor, so the
    // cursor ends at the leaf count while the column count is larger by the
    // number of groups (FR-002, FR-047).
    SG_INVARIANT(m_index == m_leafCount,
                 "one action writes one point per managed leaf (FR-047)");
  }

private:
  std::uint64_t* m_columns = nullptr;
  std::size_t m_leafCount = 0;
  // Every managed column, leaves and disclosures alike. The cursor is
  // bounded by the leaves; a disclosure names a column by index and is
  // bounded by this (FR-002).
  std::size_t m_columnCount = 0;
  std::size_t m_stride = 0;
  std::size_t m_row = 0;
  std::size_t m_index = 0;
  bool m_disclosureWritten = false;
};

/**
 * @brief The sampling primitive a provider implements for one open
 * window.
 *
 * `readPoints` fills exactly one cumulative point per managed leaf, in
 * `LeafSet` order, within one sampling action (C-PRO-2). Every window
 * constructor calls `setThunk` with a static function that reads that
 * window's own points, so the compiled plan reaches the read with no
 * vtable lookup (FR-022, R-004). A window that supplies no thunk keeps
 * `defaultThunk`, which routes to `readPoints`; that fallback costs
 * one vtable lookup per sampling action and serves a provider written
 * against this interface alone.
 *
 * \invariant `thunk` is non-null.
 */
class SPEEDGUN_NG_EXPORT WindowReader
{
public:
  /**
   * @brief A reader at its initial state.
   *
   * \pre none
   * \post none
   */
  WindowReader() = default;

  /**
   * @brief Direct-call signature the compiled plan stores; equal in
   * effect to `readPoints`.
   */
  using ReadThunk = void (*)(WindowReader&, PointSink&) noexcept;

  WindowReader(const WindowReader&) = default;
  WindowReader(WindowReader&&) = delete;
  auto operator=(const WindowReader&) -> WindowReader& = default;
  auto operator=(WindowReader&&) -> WindowReader& = delete;

  /**
   * @brief Destruction through the base pointer.
   *
   * \pre none
   * \post none
   */
  virtual ~WindowReader() = default;

  /**
   * @brief Yields one cumulative `uint64` point per managed leaf, in
   * `LeafSet` order, for one sampling action.
   *
   * The sink receives exactly one point per managed leaf, read within
   * this action (FR-011, FR-047); the obligation binds the
   * implementation.
   *
   * \pre none
   * \post none
   */
  virtual void readPoints(PointSink& sink) noexcept = 0;

  /**
   * @brief Resolves the direct-call slot for the compiled read path.
   *
   * Called once per reader at plan finalization (setup region). The
   * slot holds the static function the window constructor installed
   * through `setThunk`, and `defaultThunk` when the window installed
   * none.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto resolveThunk() noexcept -> ReadThunk
  {
    checkThunk();
    return m_thunk;
  }

protected:
  /**
   * @brief Replaces the direct-call slot; for provider constructors.
   *
   * \pre fn is non-null and equivalent to `readPoints` for this
   *      reader.
   * \post `resolveThunk` returns fn.
   */
  void setThunk(const ReadThunk fn) noexcept
  {
    SG_REQUIRE(fn != nullptr, "window reader thunk is non-null");
    m_thunk = fn;
    SG_ENSURE(m_thunk == fn, "resolve_thunk returns the installed thunk");
  }

private:
  static auto defaultThunk(WindowReader& reader,
                           PointSink& sink) noexcept -> void
  {
    reader.readPoints(sink);
  }

  auto checkThunk() const noexcept -> void
  {
    SG_INVARIANT(m_thunk != nullptr, "window reader thunk is set");
  }

  ReadThunk m_thunk = &defaultThunk;
};

/**
 * @brief The registration base every provider derives from.
 *
 * The system stores providers through this base; virtual calls happen
 * only during registration, system open, and plan compile (R-004).
 *
 */
class SPEEDGUN_NG_EXPORT ProviderIface
{
public:
  /**
   * @brief The unregistered provider state.
   *
   * \pre none
   * \post none
   */
  ProviderIface() = default;

  ProviderIface(const ProviderIface&) = default;
  ProviderIface(ProviderIface&&) = delete;
  auto operator=(const ProviderIface&) -> ProviderIface& = default;
  auto operator=(ProviderIface&&) -> ProviderIface& = delete;

  /**
   * @brief Destruction through the base pointer.
   *
   * \pre none
   * \post none
   */
  virtual ~ProviderIface() = default;

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
  virtual void enumerate(ObjectSink& sink) const = 0;

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
  virtual std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                             const Target& where) = 0;
};

}  // namespace sg::counters

#endif  // SG_COUNTERS_PROVIDER_HPP
