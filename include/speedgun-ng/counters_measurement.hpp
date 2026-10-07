#ifndef SG_COUNTERS_MEASUREMENT_HPP
#define SG_COUNTERS_MEASUREMENT_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_measurement.hpp
 * @brief The measurement spine: typed counters and expressions under
 * the dimension algebra, plan compile, folds with disclosure, and the
 * scope sugar (FR-014..FR-030).
 *
 * @version 0.5.0
 *
 * Commit `cd5cbd1` removed two public declarations, and 0.4.0 records
 * both removals. The removed declarations are the non-member
 * multiplication of an expression by a double, and the provider concept.
 * Both declarations remain absent from the public headers (FR-032).
 * 0.4.1 changes no public signature; `SOVERSION` stayed 1. 0.5.0
 * renames the whole owned surface, a breaking change on the 0.x line,
 * and the hand-kept `SOVERSION` moves to 2.
 *
 * Dimensions live in types and are erased before the point buffer (FR-016).
 * The read path holds no expression tree and no name lookup; a window that
 * installed no thunk pays one vtable lookup per sample (FR-022).
 *
 * @section cadence The cadence idiom
 *
 * A tight loop pays for every `sample()`, so the caller chooses a
 * cadence. For a loop of `N` known iterations sampling every `K` of
 * them, mint `plan::recorder(N / K + 1)` and call `sample()` once per
 * chunk; the extra row holds the endpoint that closes the last
 * chunk. The first sample lands before the work, so the count is
 * `N / K + 1` exactly.
 *
 * `expression::foldPairs(recorder)` then yields one `MetricResult`
 * per adjacent interval, which is the per-chunk series. A
 * first-to-last `fold()` answers the single total. A window
 * from `i` to `j` costs the sampling actions at both endpoints, and
 * `K = 1` spends one action per iteration, so an `N`-iteration window
 * pays `N` of them. The per-action medians behind those figures are
 * the ones `docs/pages/counters-overhead.md` publishes for its
 * reference host. Its release build (`-O3 -DNDEBUG`, contracts
 * `ignore`) measures 40 ns for the clock plan's one clock read and
 * 70 ns for a two-event-counter plan, one read per group leader per
 * action, so sampling every iteration adds `N * 40 ns` and
 * `N * 70 ns` across the window and the two endpoints of one window
 * cost `2 * 40 = 80 ns` and `2 * 70 = 140 ns`. Its correctness build
 * (`-g`, contracts `enforce`) measures 70 ns and 180 ns for those two
 * plans, so the same arithmetic gives `N * 70 ns` and `N * 180 ns`
 * across the window, with 140 ns and 360 ns at the endpoints. The
 * page records the host, the load under it, and the commands, and the
 * page owns the refresh.
 * `plan::sampleOverheadNsMedian()` reports the figure for the plan
 * in hand (FR-032). A benchmark whose measured work is shorter than
 * that reads its own instrumentation, so the cadence must be coarse
 * enough for the work.
 *
 * @section exactness Numeric exactness
 *
 * Points are `uint64` and deltas subtract modularly at `2^64`, so a
 * counter wrap subtracts out and a long window stays exact (FR-013).
 * Folds then convert to `double`, which carries 53 bits of mantissa:
 * a delta above `2^53` loses its low bits. A count reaches `2^53` at
 * roughly 9007199 events per nanosecond sustained for one second, so any
 * counter-backed delta is exact on every host this feature targets. A
 * scaled expression, such as a per-iteration rate, can reach the
 * threshold through its scale factor alone; `MetricResult::scaled`
 * reports that the value carries a scale the caller applied.
 */

namespace sg::counters
{

class System;
class Object;
class Plan;
class Scope;

namespace detail
{

// A resolved catalog leaf carried by value between resolution, plan
// compile, and fold. Internal spine data behind public interfaces.
struct LeafCore
{
  std::string address;  // "<object path>/<name>", canonical spelling
  std::string name;
  std::string description;
  std::string unit;  // canonical unit token
  Availability avail = Availability::COUNTABLE;
  ReadMode mode = ReadMode::SYSCALL;
  std::uint64_t frequencyHz = 0;  // fixed-rate calibration, 0 elsewhere
  bool scaled = false;  // platform-scaled tick source disclosure
};

// One node of the erased expression spine: nodes reference leaves and
// each other by index; the dimension lives only in the wrapper type.
struct ExprNode
{
  // Kind codes: 0 leaf, 1 add, 2 subtract, 3 divide, 4 scale.
  std::uint8_t kind = 0;
  int left = -1;  // node index; -1 unless kind is binary or 4
  int right = -1;  // node index; -1 unless kind is binary
  int leaf = -1;  // leaf index for kind 0; else -1
  double scale = 1.0;  // factor a kind 4 node applies to its operand
};

struct ExprCore
{
  std::vector<LeafCore> leaves;
  std::vector<ExprNode> nodes;

  [[nodiscard]] auto empty() const noexcept -> bool { return nodes.empty(); }

  auto addLeaf(const LeafCore& leaf) -> int
  {
    for (std::size_t i = 0; i < leaves.size(); ++i) {
      if (leaves[i].address == leaf.address) {
        return static_cast<int>(i);
      }
    }
    leaves.push_back(leaf);
    return static_cast<int>(leaves.size() - 1);
  }

  auto addNode(const ExprNode& node) -> int
  {
    nodes.push_back(node);
    return static_cast<int>(nodes.size() - 1);
  }

  // Index of the spine's root node; spines are built with the root
  // last, so it is the final node.
  [[nodiscard]] auto root() const noexcept -> int
  {
    return static_cast<int>(nodes.size()) - 1;
  }

  // Scales the spine's folded value by `k` (scalar multiplication):
  // one node over the root, so a composite's own arithmetic keeps every
  // operand at full weight. Scaling the leaves instead folds
  // `2.0 * (a / b)` to `a / b`, because the quotient divides the two
  // scaled operands back into each other.
  auto scaleAll(double k) -> void
  {
    addNode(ExprNode {
        .kind = 4, .left = root(), .right = -1, .leaf = -1, .scale = k});
  }
};

// Splices a whole spine into `dst` with index remapping and returns
// the spliced root index.
//
// Leaf indices are remapped through the slot `addLeaf` assigns,
// because `addLeaf` deduplicates by address: two operands that share a
// leaf contribute one slot between them, so the pre-splice leaf count is
// not the offset the second operand's nodes must use. An operand built
// as `x / x` has both operands carrying the same leaf, and remapping by
// the leaf count would point its second node past the end of the leaf
// vector, so the fold read out of bounds.
inline auto splice(detail::ExprCore& dst, const detail::ExprCore& src) -> int
{
  const int nodeBase = static_cast<int>(dst.nodes.size());
  std::vector<int> leafRemap;
  leafRemap.reserve(src.leaves.size());
  for (const auto& leaf : src.leaves) {
    leafRemap.push_back(dst.addLeaf(leaf));
  }
  for (auto node : src.nodes) {
    if (node.left >= 0) {
      node.left += nodeBase;
    }
    if (node.right >= 0) {
      node.right += nodeBase;
    }
    if (node.leaf >= 0) {
      node.leaf = leafRemap[static_cast<std::size_t>(node.leaf)];
    }
    dst.addNode(node);
  }
  return nodeBase + src.root();
}

// Stand-in for std::IsSpecializationOf (C++23 reflection; absent
// from both supported standard libraries): true when `T` names
// `Primary` specialized on its own argument list.
template<class T, template<class...> class Primary>
struct IsSpecializationOf : std::false_type
{
};

template<template<class...> class Primary, class... Args>
struct IsSpecializationOf<Primary<Args...>, Primary> : std::true_type
{
};

}  // namespace detail

/**
 * @brief A resolved leaf handle: slot identity, catalog metadata, and
 * the compile-time dimension tag (E-05).
 *
 * The metadata is a value copy taken at resolution; the catalog is
 * immutable after open, so the handle stays valid (FR-009).
 */
template<class D>
class Counter
{
public:
  using DimensionTag = D;

  detail::LeafCore leaf;

  /**
   * @brief The counter name within its owning object.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto name() const noexcept -> std::string_view
  {
    return leaf.name;
  }

  /**
   * @brief The catalog description.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto description() const noexcept -> std::string_view
  {
    return leaf.description;
  }

  /**
   * @brief The canonical unit token.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto unitToken() const noexcept -> std::string_view
  {
    return leaf.unit;
  }

  /**
   * @brief The availability state at resolution (FR-006).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto avail() const noexcept -> Availability
  {
    return leaf.avail;
  }

  /**
   * @brief The canonical leaf address `<object path>/<name>`.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto address() const noexcept -> std::string_view
  {
    return leaf.address;
  }
};

/**
 * @brief Hot-path handle of a push counter: a plain, non-atomic
 * `uint64` increment on the creating thread, sampled by plain load
 * (FR-035, R-008).
 *
 * Declared through `PushProvider` before registration. Cross-thread
 * `add` is a tier-3 violation, checked by default and elided under
 * `SG_CONTRACTS_IGNORE`.
 */
class SPEEDGUN_NG_EXPORT PushCounter
{
public:
  /**
   * @brief Copies of a handle name the same counter.
   *
   * \pre none
   * \post none
   */
  PushCounter(const PushCounter&) = default;

  /**
   * @brief Moves of a handle name the same counter.
   *
   * \pre none
   * \post none
   */
  PushCounter(PushCounter&&) noexcept = default;

  /**
   * @brief Assignment names the same counter.
   *
   * \pre none
   * \post none
   */
  auto operator=(const PushCounter&) -> PushCounter& = default;

  /**
   * @brief Move assignment names the same counter.
   *
   * \pre none
   * \post none
   */
  auto operator=(PushCounter&&) noexcept -> PushCounter& = default;

  /**
   * @brief The trivial destruction of a handle.
   *
   * \pre none
   * \post none
   */
  ~PushCounter() = default;

  /**
   * @brief Adds `n` to the counter's running total: a plain
   * non-atomic increment on the hot path (FR-035).
   *
   * \pre Called on the thread that declared this counter.
   * \post none
   */
  auto add(const std::uint64_t n) const noexcept -> void
  {
    SG_REQUIRE(std::this_thread::get_id() == m_owner,
               "push counter add runs on its creating thread (FR-035)");
    *m_value += n;
  }

  /**
   * @brief The counter name within the machine object.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto name() const noexcept -> std::string_view
  {
    return m_name;
  }

private:
  friend class PushProvider;

  PushCounter(std::uint64_t* const value,
              const std::thread::id owner,
              std::string_view name) noexcept
      : m_value(value)
      , m_owner(owner)
      , m_name(name.data(), name.size())
  {
  }

  std::uint64_t* m_value = nullptr;
  std::thread::id m_owner;
  std::string m_name;
};

/**
 * @brief The recorded-window view a fold reads (FR-018): the compiled
 * layout pointer, the point columns, and the bookkeeping a fold
 * consults (extent, wrap, drops).
 *
 * Plain data: the recorder handle and the scope both produce one. The
 * view borrows columns from its producer for as long as the producer
 * lives.
 */
struct RecorderApi
{
  const void* impl = nullptr;  // compiled layout, owned by the plan
  const std::uint64_t* columns = nullptr;
  std::size_t stride = 0;  // rows reserved per column
  std::size_t count = 0;  // committed point columns
  bool wrapped = false;
  std::uint64_t dropped = 0;
};

/**
 * @brief The raw per-leaf provenance view of a composite (FR-020,
 * E-10): catalog facts, the raw point column, point identity, and the
 * multiplex ratio.
 *
 * `availability` is the disclosure state of the window's end point
 * only. An interior row can hold a zero from a gap while this field
 * stays `Availability::COUNTABLE`, because the end point of that window
 * measured a count. A caller that needs the state of each point uses
 * `fold` or `foldPairs`, which disclose each window's own gap state
 * (FR-004, FR-005).
 *
 * `ratio` is the multiplex fraction over the window this view spans, and
 * it is meaningful only when `availability != Availability::GAP`. A view
 * whose end point is an action that measured nothing publishes `1.0` and
 * discloses no fraction, because no measured time covers an action that
 * measured no count. The state beside the ratio names that condition, so a
 * caller reads the ratio only after the state (FR-005, FR-019, FR-020).
 */
struct PointsView
{
  std::string_view objectPath;
  std::string_view name;
  std::string_view description;
  std::string_view unit;
  std::size_t slot = 0;  // point identity: column index in the plan
  const std::uint64_t* points = nullptr;
  std::size_t count = 0;
  double ratio = 1.0;  // the multiplex fraction; 1.0 discloses no fraction
  // The field carries the contract's name, and the type is qualified for
  // the same reason as the one on `MetricResult`: a member named as a
  // type already in this namespace changes that name's meaning for the
  // rest of the class body, which is ill-formed (FR-004, FR-035).
  ::sg::counters::Availability availability =
      ::sg::counters::Availability::COUNTABLE;
};

/**
 * @brief Overflow policy tag: a sample past capacity is a violation
 * (FR-025). The default policy.
 *
 * \pre none
 * \post none
 */
struct HardStop
{
};

/**
 * @brief Overflow policy tag: writes mask into the ring, drop with
 * accounting (FR-025, FR-028).
 *
 * \pre none
 * \post none
 */
struct Ring
{
};

inline constexpr HardStop hardStop {};
inline constexpr Ring ring {};

namespace detail
{

// Sampling cores behind RecorderHandle::sample: the bounds-checked
// hardStop path and the masked ring path (FR-026, FR-027, FR-028).
SPEEDGUN_NG_EXPORT auto hardStopSampleCore(const void* impl,
                                           std::uint64_t* columns,
                                           std::size_t capacity,
                                           std::size_t& head) noexcept -> void;

SPEEDGUN_NG_EXPORT auto ringSampleCore(const void* impl,
                                       std::uint64_t* columns,
                                       std::size_t capacity,
                                       std::size_t& head,
                                       bool& wrapped,
                                       std::uint64_t& dropped) noexcept -> void;

}  // namespace detail

/**
 * @brief The recorder value handle (E-08, FR-029): a trivially
 * copyable cursor over one plan-arena buffer of `capacity` point
 * columns, one per compiled leaf. `P` is the overflow policy tag:
 * `HardStop` or `Ring`, chosen by the plan factory (FR-025).
 * The plan owns the buffer; the plan outlives its recorders.
 */
template<class P>
class RecorderHandle
{
public:
  const void* mImpl = nullptr;
  std::uint64_t* mColumns = nullptr;
  std::size_t mCapacity = 0;
  std::size_t mHead = 0;
  bool mWrapped = false;
  std::uint64_t mDropped = 0;

  /**
   * @brief THE critical path: one sampling action appends one point
   * per column (FR-026). Zero allocation, zero lock; the five shipped
   * windows reach `readPoints` with no virtual call, and a window
   * that installed no thunk pays one vtable lookup per sampling action
   * on the seam's documented fallback (FR-022). hardStop overrun is
   * an `SG_REQUIRE_ALWAYS` violation in every build configuration
   * (FR-027); ring masks into the buffer and records wrapped plus
   * dropped (FR-028).
   *
   * \pre none
   * \post none
   */
  auto sample() noexcept -> void
  {
    if constexpr (std::is_same_v<P, HardStop>) {
      detail::hardStopSampleCore(mImpl, mColumns, mCapacity, mHead);
    } else {
      detail::ringSampleCore(
          mImpl, mColumns, mCapacity, mHead, mWrapped, mDropped);
    }
  }

  /**
   * @brief The recorded-window view a fold reads: extent, wrap, and
   * drops of the retained window (FR-018, FR-028).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto view() const noexcept -> RecorderApi
  {
    const auto committed =
        mHead < mCapacity ? mHead : static_cast<std::size_t>(mCapacity);
    return RecorderApi {
        .impl = mImpl,
        .columns = mColumns,
        .stride = mCapacity,
        .count = committed,
        .wrapped = mWrapped,
        .dropped = mDropped,
    };
  }

  /**
   * @brief The stored point columns fixed at construction (FR-025).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto capacity() const noexcept -> std::size_t
  {
    return mCapacity;
  }

  /**
   * @brief The retained point count: samples committed, capped at
   * capacity (FR-028).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto count() const noexcept -> std::size_t
  {
    return view().count;
  }

  /**
   * @brief True once a ring write has overwritten a retained point
   * (FR-028).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto wrapped() const noexcept -> bool { return mWrapped; }

  /**
   * @brief The count of overwritten points (FR-028).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto dropped() const noexcept -> std::uint64_t
  {
    return mDropped;
  }
};

// A recorder is a plain cursor, so copying one copies the six members
// and calls nothing; a non-trivial copy would reach the allocator on
// the read path (FR-029).
static_assert(std::is_trivially_copyable_v<RecorderHandle<HardStop>>);
static_assert(std::is_trivially_copyable_v<RecorderHandle<Ring>>);

namespace detail
{

// Core operations behind the public template wrappers.
[[nodiscard]] SPEEDGUN_NG_EXPORT auto foldCore(const ExprCore& core,
                                               const RecorderApi& rec,
                                               std::size_t i,
                                               std::size_t j) -> MetricResult;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto foldPairsCore(
    const ExprCore& core, const RecorderApi& rec) -> std::vector<MetricResult>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto rawCore(const ExprCore& core,
                                              const RecorderApi& rec,
                                              std::string_view objectPath,
                                              std::string_view leafName)
    -> std::expected<PointsView, Error>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto resolveLeafCore(
    const Object& obj, std::string_view name) -> std::expected<LeafCore, Error>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto compileCore(
    const System& sys,
    const Target& tg,
    const std::vector<const ExprCore*>& exprs) -> std::expected<Plan, Error>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto metricCore(
    const Scope& scopeObj, const ExprCore& core) -> MetricResult;

}  // namespace detail

/**
 * @brief A typed composition of resolved leaves (E-05).
 *
 * Construction is compile-time algebra (FR-015): `+` and `-` require
 * identical tags, `/` subtracts exponents, scalar multiplication is
 * unrestricted. Construction performs zero hardware reads (FR-021).
 * Accepted, documented limitation (A10): `events^1 + events^1`
 * compiles; the system catches the time-versus-count class.
 */
template<class D>
class Expression
{
public:
  using DimensionTag = D;

  detail::ExprCore core;

  /**
   * @brief An empty spine; compile rejects a zero-leaf expression.
   *
   * \pre none
   * \post none
   */
  Expression() = default;

  /**
   * @brief Builds an expression from one resolved leaf: the leaf's
   * delta, unscaled.
   *
   * \pre none
   * \post none
   */
  template<class C>
    requires kDimSame<D, C>
  // The suppressed check asks for `explicit` on this single-argument
  // constructor. Converting a resolved counter into its expression is
  // the public composition spelling, and `explicit` would force every
  // operand form to name `expression<D>(leaf)` by hand.
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  Expression(const Counter<C>& leaf)
  {
    const int leafIndex = core.addLeaf(leaf.leaf);
    core.addNode(detail::ExprNode {
        .kind = 0, .left = -1, .right = -1, .leaf = leafIndex, .scale = 1.0});
  }

  /**
   * @brief Window fold over recorded points `i` and `j` (FR-018).
   *
   * Deltas subtract modularly at 2^64, so one hardware wrap subtracts
   * out (FR-013). The fold never reads providers, is pure, and is
   * repeatable (FR-021).
   *
   * \pre `i < j` within the recorder's recorded extent (tier-3).
   * \post none
   */
  [[nodiscard]] auto fold(const RecorderApi& rec,
                          std::size_t i,
                          std::size_t j) const -> MetricResult
  {
    SG_REQUIRE(i < j && j < rec.count,
               "fold window lies within the recorded extent (FR-018)");
    return detail::foldCore(core, rec, i, j);
  }

  /**
   * @brief First-to-last fold (FR-018).
   *
   * \pre at least two committed points (tier-3).
   * \post none
   */
  [[nodiscard]] auto fold(const RecorderApi& rec) const -> MetricResult
  {
    SG_REQUIRE(rec.count >= 2,
               "first-to-last fold needs two committed points (FR-018)");
    return detail::foldCore(core, rec, 0, rec.count - 1);
  }

  /**
   * @brief One metric per adjacent recorded interval (FR-018).
   *
   * One metric per adjacent pair, so a recorder with `n` committed
   * points yields `n - 1` metrics. The core behind this wrapper checks
   * the same bound; the check here pairs the precondition with the
   * public entry point, so this signature keeps a deduced return type
   * for the pairing gate to attribute.
   *
   * \pre the recorder holds two or more committed points (tier-3).
   * \post none
   */
  [[nodiscard]] auto foldPairs(const RecorderApi& rec) const
  {
    SG_REQUIRE(rec.count >= 2,
               "pair folds need at least two committed points (FR-018)");
    return detail::foldPairsCore(core, rec);
  }

  /**
   * @brief Raw provenance view of one constituent leaf (FR-020).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto raw(const RecorderApi& rec,
                         std::string_view objectPath,
                         std::string_view leaf) const
      -> std::expected<PointsView, Error>
  {
    return detail::rawCore(core, rec, objectPath, leaf);
  }

  // Hidden friends: non-template, so counter arguments convert and
  // same-typed operands match exactly.
  friend auto operator+(const Expression& a, const Expression& b) -> Expression
  {
    auto merged = a.core;
    const int left = merged.root();
    const int right = detail::splice(merged, b.core);
    merged.addNode(detail::ExprNode {
        .kind = 1, .left = left, .right = right, .leaf = -1, .scale = 1.0});
    auto out = Expression();
    out.core = std::move(merged);
    return out;
  }

  friend auto operator-(const Expression& a, const Expression& b) -> Expression
  {
    auto merged = a.core;
    const int left = merged.root();
    const int right = detail::splice(merged, b.core);
    merged.addNode(detail::ExprNode {
        .kind = 2, .left = left, .right = right, .leaf = -1, .scale = 1.0});
    auto out = Expression();
    out.core = std::move(merged);
    return out;
  }

  friend auto operator*(const double k, const Expression& e) -> Expression
  {
    auto merged = e.core;
    merged.scaleAll(k);
    auto out = Expression();
    out.core = std::move(merged);
    return out;
  }
};

/**
 * @brief Cross-dimension division: exponents subtract (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const Expression<D1>& a, const Expression<D2>& b)
    -> Expression<DimQuotient<D1, D2>>
{
  detail::ExprCore merged;
  const int left = detail::splice(merged, a.core);
  const int right = detail::splice(merged, b.core);
  merged.addNode(detail::ExprNode {
      .kind = 3, .left = left, .right = right, .leaf = -1, .scale = 1.0});
  auto out = Expression<DimQuotient<D1, D2>>();
  out.core = std::move(merged);
  return out;
}

// Same-dimension addition and subtraction across distinct spellings.
// The body is dimension-independent: same tags means same spine
// algebra, and the static_assert names the violation.
template<class D1, class D2>
[[nodiscard]] auto operator+(const Expression<D1>& a,
                             const Expression<D2>& b) -> Expression<D1>
{
  static_assert(kDimSame<D1, D2>,
                "expression addition requires identical dimension tags");
  auto merged = a.core;
  const int left = merged.root();
  const int right = detail::splice(merged, b.core);
  merged.addNode(detail::ExprNode {
      .kind = 1, .left = left, .right = right, .leaf = -1, .scale = 1.0});
  auto out = Expression<D1>();
  out.core = std::move(merged);
  return out;
}

template<class D1, class D2>
[[nodiscard]] auto operator-(const Expression<D1>& a,
                             const Expression<D2>& b) -> Expression<D1>
{
  static_assert(kDimSame<D1, D2>,
                "expression subtraction requires identical dimension tags");
  auto merged = a.core;
  const int left = merged.root();
  const int right = detail::splice(merged, b.core);
  merged.addNode(detail::ExprNode {
      .kind = 2, .left = left, .right = right, .leaf = -1, .scale = 1.0});
  auto out = Expression<D1>();
  out.core = std::move(merged);
  return out;
}

// Counter operand forms: resolve to expression algebra.
/**
 * @brief Divides two resolved counters (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const Counter<D1>& a, const Counter<D2>& b)
    -> Expression<DimQuotient<D1, D2>>
{
  return Expression<D1>(a) / Expression<D2>(b);
}

/**
 * @brief Divides an expression by a resolved counter (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const Expression<D1>& a, const Counter<D2>& b)
    -> Expression<DimQuotient<D1, D2>>
{
  return a / Expression<D2>(b);
}

/**
 * @brief Divides a resolved counter by an expression (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const Counter<D1>& a, const Expression<D2>& b)
    -> Expression<DimQuotient<D1, D2>>
{
  return Expression<D1>(a) / b;
}

template<class D1, class D2>
[[nodiscard]] auto operator+(const Counter<D1>& a,
                             const Counter<D2>& b) -> Expression<D1>
{
  static_assert(kDimSame<D1, D2>,
                "counter addition requires identical dimension tags");
  return Expression<D1>(a) + Expression<D1>(b);
}

template<class D1, class D2>
[[nodiscard]] auto operator-(const Counter<D1>& a,
                             const Counter<D2>& b) -> Expression<D1>
{
  static_assert(kDimSame<D1, D2>,
                "counter subtraction requires identical dimension tags");
  return Expression<D1>(a) - Expression<D1>(b);
}

/**
 * @brief The compiled read plan (E-07): flat leaf slots, per-provider
 * read groups, arena geometry, and the per-thread binding.
 *
 * Move-only; the plan owns the compiled layout. Plans and recorders
 * are per-thread objects (FR-031); multiple plans over one system are
 * first-class.
 */
class SPEEDGUN_NG_EXPORT Plan
{
public:
  Plan(const Plan&) = delete;
  auto operator=(const Plan&) -> Plan& = delete;

  /**
   * @brief Moves the compiled layout.
   *
   * \pre none
   * \post `other` holds no layout.
   */
  Plan(Plan&& other) noexcept;

  /**
   * @brief Move assignment: `other` holds no layout.
   *
   * \pre none
   * \post none
   */
  auto operator=(Plan&& other) noexcept -> Plan&;

  /**
   * @brief Releases the compiled layout and provider windows; every
   * recorder or scope over this plan must be destroyed first.
   *
   * \pre none
   * \post none
   */
  ~Plan();

  /**
   * @brief Mints a hardStop recorder: `capacity` point columns are
   * allocated now from the plan arena; sampling allocates nothing
   * later (FR-025, FR-029). The plan outlives its recorders. A zero
   * capacity mints a recorder whose first sampling action reports the
   * sampling bound `hardStopSampleCore` enforces (FR-025).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto recorder(std::size_t capacity) const
      -> RecorderHandle<HardStop>;

  /**
   * @brief Mints a ring recorder: `capacity` point columns are
   * allocated now. A capacity that is not a power of two is a
   * recoverable construction error (FR-025, R-006).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto recorder(std::size_t capacity, Ring) const
      -> std::expected<RecorderHandle<Ring>, Error>;

  /**
   * @brief The cheapest `sample()` in the recorded distribution, in
   * nanoseconds (FR-032).
   *
   * The first call on a plan runs the calibration: it samples the
   * plan's own read sequence over an empty workload and stores the
   * distribution on the plan. Compile itself performs no hardware read
   * (FR-021), so the reads happen here, on the caller's request. Fold a
   * window from `i` to `j` and its two endpoints cost
   * `2 * sampleOverheadNsMedian()` of sampling on top of the work
   * the window measured (FR-032, FR-048).
   *
   * \pre none
   * \post The returned nanosecond count is at least 0.
   */
  [[nodiscard]] auto sampleOverheadNsMin() const -> double;

  /**
   * @brief The median `sample()` in the recorded distribution, in
   * nanoseconds (FR-032). The value a fold window's two endpoints cost.
   *
   * \pre none
   * \post The returned nanosecond count is at least 0.
   */
  [[nodiscard]] auto sampleOverheadNsMedian() const -> double;

  /**
   * @brief The dearest `sample()` in the recorded distribution, in
   * nanoseconds (FR-032). A long tail is investigated, never assumed
   * away (Principle VII).
   *
   * \pre none
   * \post The returned nanosecond count is at least `min()`.
   */
  [[nodiscard]] auto sampleOverheadNsMax() const -> double;

private:
  friend auto detail::compileCore(const System& sys,
                                  const Target& tg,
                                  const std::vector<const detail::ExprCore*>&
                                      exprs) -> std::expected<Plan, Error>;
  friend class Scope;

  explicit Plan(void* impl) noexcept
      : m_impl(impl)
  {
  }

  void* m_impl = nullptr;  // the compiled layout
};

/**
 * @brief Exactly a two-point recorder (FR-030): `start()` samples
 * point zero, `finish()` samples point one, and `metric(expr)` folds
 * the window. One semantics, two spellings with the recorder.
 *
 * The two point columns live in the scope object; the plan behind the
 * scope must outlive it. A constructor cannot read a lifetime. That
 * obligation is the caller's and this note carries it.
 *
 * A composite reaches a scope through the plan it was compiled into,
 * in the untimed region, and that plan is fixed before the window
 * opens. A scope registry would be the only way to register a
 * composite into a running scope, and the API has none: the misuse is
 * unrepresentable. The three misuse sequences the type refuses are
 * `metric` on a window that is not closed, `finish` without `start`,
 * and a second `start`, each a contract violation (FR-046, the
 * scope-misuse edge case), and a finished scope is a settled window.
 */
class SPEEDGUN_NG_EXPORT Scope
{
public:
  /**
   * @brief Prepares a two-point window over a compiled plan.
   *
   * \pre none
   * \post The scope awaits `start()`.
   */
  explicit Scope(const Plan& compiled);

  Scope(const Scope&) = delete;
  auto operator=(const Scope&) -> Scope& = delete;
  Scope(Scope&&) = delete;
  auto operator=(Scope&&) -> Scope& = delete;

  /**
   * @brief Releases the two point columns.
   *
   * \pre none
   * \post none
   */
  ~Scope();

  /**
   * @brief Samples the window's first point (FR-030).
   *
   * \pre `start` has not been called on this scope yet (tier-3).
   * \post The first point of one sampling action is recorded.
   */
  void start();

  /**
   * @brief Samples the window's second point (FR-030).
   *
   * \pre `start` has been called and `finish` has not (tier-3).
   * \post The second point of one sampling action is recorded; the
   *       window is closed.
   */
  void finish();

  /**
   * @brief Folds the expression over the closed window (FR-030);
   * `start` and `finish` must have been called (tier-3). The fold layer
   * enforces that bound (FR-046).
   *
   * \pre none
   * \post none
   */
  template<class D>
  [[nodiscard]] auto metric(const Expression<D>& e) const -> MetricResult
  {
    return detail::metricCore(*this, e.core);
  }

  /**
   * @brief The recorded-window view of the scope's two points.
   *
   * Folds and raw views read through it; it stays provider-read-free
   * like every fold (FR-021).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto view() const noexcept -> RecorderApi;

private:
  friend auto detail::metricCore(const Scope& scopeObj,
                                 const detail::ExprCore& core) -> MetricResult;

  void* m_core = nullptr;  // the scope internals
};

/**
 * @brief Compiles expressions into a plan (FR-021, FR-022): name
 * resolution through the system tree, dimension checking, group
 * layout, and arena geometry, once in the untimed region. The read
 * mode each leaf opens through is the one the enumeration-time probe
 * recorded and the catalog disclosed before that open boundary
 * (FR-023). Zero hardware reads (FR-021).
 *
 * \pre none
 * \post none
 */
template<class... E>
  requires(detail::IsSpecializationOf<std::remove_cvref_t<E>, Expression>::value
           && ...)
[[nodiscard]] auto compile(const System& sys,
                           const E&... exprs) -> std::expected<Plan, Error>
{
  const std::vector<const detail::ExprCore*> cores {&exprs.core...};
  return detail::compileCore(sys, Target {}, cores);
}

/**
 * @brief Compiles expressions into a plan bound to a target.
 *
 * \pre none
 * \post none
 */
template<class... E>
  requires(detail::IsSpecializationOf<std::remove_cvref_t<E>, Expression>::value
           && ...)
[[nodiscard]] auto compile(const System& sys,
                           const Target& tg,
                           const E&... exprs) -> std::expected<Plan, Error>
{
  const std::vector<const detail::ExprCore*> cores {&exprs.core...};
  return detail::compileCore(sys, tg, cores);
}

/**
 * @brief One object's result of a fan-out fold (SC-007): the canonical
 * path and the metric over that object's instantiated leaves.
 */
struct FanoutResult
{
  std::string objectPath;  // canonical spelling (FR-002)
  MetricResult metric;
};

class Object;
class FanoutPlan;

namespace detail
{

SPEEDGUN_NG_EXPORT auto compileFanoutCore(
    const System& sys,
    const Target& tg,
    const ExprCore& exemplar,
    const std::vector<const Object*>& selection)
    -> std::expected<FanoutPlan, Error>;

SPEEDGUN_NG_EXPORT auto fanoutFoldCore(const void* fanout,
                                       const ExprCore& core,
                                       const RecorderApi& rec)
    -> std::vector<FanoutResult>;

}  // namespace detail

/**
 * @brief A fan-out plan (US3 scenario 5, SC-007): the exemplar
 * expression instantiated per selected object; every instantiated
 * leaf is read inside each shared sampling action (FR-047).
 * Move-only; the plan owns the layout and the recorder arenas.
 */
class SPEEDGUN_NG_EXPORT FanoutPlan
{
public:
  FanoutPlan(const FanoutPlan&) = delete;
  auto operator=(const FanoutPlan&) -> FanoutPlan& = delete;

  /**
   * @brief Moves the compiled fan-out layout.
   *
   * \pre none
   * \post `other` holds no layout.
   */
  FanoutPlan(FanoutPlan&& other) noexcept;

  /**
   * @brief Move assignment: `other` holds no layout.
   *
   * \pre none
   * \post none
   */
  auto operator=(FanoutPlan&& other) noexcept -> FanoutPlan&;

  /**
   * @brief Releases the fan-out layout and arenas; every recorder
   * over this plan must be destroyed first.
   *
   * \pre none
   * \post none
   */
  ~FanoutPlan();

  /**
   * @brief Mints a hardStop recorder over the shared window:
   * `capacity` point columns per instantiated leaf; one sampling
   * action reads every instance (FR-047, FR-029). The plan outlives
   * its recorders.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto recorder(std::size_t capacity) const
      -> RecorderHandle<HardStop>;

  /**
   * @brief The selected objects' canonical paths, selection order.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto objectPaths() const -> std::vector<std::string>;

  /**
   * @brief First-to-last fold per selected object in selection order
   * (SC-007): the expression instantiated at each object, keyed by
   * canonical path. Pure like every fold (FR-021); the window follows
   * the fold range rules (FR-018).
   *
   * \pre none
   * \post none
   */
  template<class D>
  [[nodiscard]] auto fold(const Expression<D>& e, const RecorderApi& rec) const
      -> std::vector<FanoutResult>
  {
    return detail::fanoutFoldCore(m_impl, e.core, rec);
  }

private:
  friend auto detail::compileFanoutCore(
      const System& sys,
      const Target& tg,
      const detail::ExprCore& exemplar,
      const std::vector<const Object*>& selection)
      -> std::expected<FanoutPlan, Error>;
  friend auto detail::fanoutFoldCore(const void* fanout,
                                     const detail::ExprCore& core,
                                     const RecorderApi& rec)
      -> std::vector<FanoutResult>;

  explicit FanoutPlan(void* impl) noexcept
      : m_impl(impl)
  {
  }

  void* m_impl = nullptr;  // the compiled fan-out layout
};

/**
 * @brief Compiles one fan-out plan: the exemplar expression
 * instantiated at every selected object, all leaves read within one
 * sampling action (FR-047, US3 scenario 5). An empty selection, a
 * duplicate selection entry, or a leaf missing on a selected object
 * is a recoverable construction error (FR-024).
 *
 * \pre none
 * \post none
 */
template<class D>
[[nodiscard]] auto compile(const System& sys,
                           const Expression<D>& expr,
                           const std::vector<const Object*>& selection)
    -> std::expected<FanoutPlan, Error>
{
  return detail::compileFanoutCore(sys, Target {}, expr.core, selection);
}

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
