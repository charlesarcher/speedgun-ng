#ifndef SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
#define SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP

#include <concepts>
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
 * Dimensions live in types and are erased before the point buffer
 * (FR-016). The read path holds no expression tree, no dispatch, and
 * no name lookup (FR-022).
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
 * `expression::fold_pairs(recorder)` then yields one `metric_result`
 * per adjacent interval, which is the per-chunk series. A
 * first-to-last `fold()` answers the single total. A window
 * from `i` to `j` costs the sampling actions at both endpoints, so
 * `K = 1` charges `N` actions: on the reference host published in
 * `docs/pages/counters-overhead.md`, a clock plan costs 60 ns per
 * action and a core-PMU group 210 ns, so sampling every iteration adds
 * 120 ns and 420 ns per measured window respectively. A benchmark whose
 * measured work is shorter than that reads its own instrumentation, so
 * the cadence must be coarse enough for the work.
 * `plan::sample_overhead_ns_median()` reports the figure for the plan in
 * hand (FR-032).
 *
 * @section exactness Numeric exactness
 *
 * Points are `uint64` and deltas subtract modularly at `2^64`, so a
 * counter wrap subtracts out and a long window stays exact (FR-013).
 * Folds then convert to `double`, which carries 53 bits of mantissa:
 * a delta above `2^53` loses its low bits. A count reaches `2^53` at
 * roughly 285 events per nanosecond sustained for one second, so any
 * counter-backed delta is exact on every host this feature targets. A
 * scaled expression, such as a per-iteration rate, can reach the
 * threshold through its scale factor alone; `metric_result::scaled`
 * reports that the value carries a scale the caller applied.
 */

namespace sg::counters
{

class system;
class object;
class plan;
class scope;

namespace detail
{

// A resolved catalog leaf carried by value between resolution, plan
// compile, and fold. Internal spine data behind public interfaces.
struct leaf_core
{
  std::string address;  // "<object path>/<name>", canonical spelling
  std::string name;
  std::string description;
  std::string unit;  // canonical unit token
  availability avail = availability::countable;
  read_mode mode = read_mode::syscall;
  std::uint64_t frequency_hz = 0;  // fixed-rate calibration, 0 elsewhere
  bool scaled = false;  // platform-scaled tick source disclosure
};

// One node of the erased expression spine: nodes reference leaves and
// each other by index; the dimension lives only in the wrapper type.
struct expr_node
{
  // Kind codes: 0 leaf, 1 add, 2 subtract, 3 divide.
  std::uint8_t kind = 0;
  int left = -1;  // node index; -1 unless kind is binary
  int right = -1;  // node index; -1 unless kind is binary
  int leaf = -1;  // leaf index for kind 0; else -1
  double scale = 1.0;
};

struct expr_core
{
  std::vector<leaf_core> leaves;
  std::vector<expr_node> nodes;

  [[nodiscard]] auto empty() const noexcept -> bool { return nodes.empty(); }

  auto add_leaf(const leaf_core& leaf) -> int
  {
    for (std::size_t i = 0; i < leaves.size(); ++i) {
      if (leaves[i].address == leaf.address) {
        return static_cast<int>(i);
      }
    }
    leaves.push_back(leaf);
    return static_cast<int>(leaves.size() - 1);
  }

  auto add_node(const expr_node& node) -> int
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

  // Scales every leaf node by `k` (scalar multiplication).
  auto scale_all(double k) noexcept -> void
  {
    for (auto& node : nodes) {
      if (node.kind == 0) {
        node.scale *= k;
      }
    }
  }
};

// Splices a whole spine into `dst` with index remapping and returns
// the spliced root index.
//
// Leaf indices are remapped through the slot `add_leaf` actually assigns,
// because `add_leaf` deduplicates by address: two operands that share a
// leaf contribute one slot between them, so the pre-splice leaf count is
// not the offset the second operand's nodes must use. An operand built
// as `x / x` has both operands carrying the same leaf, and remapping by
// the leaf count would point its second node past the end of the leaf
// vector, so the fold read out of bounds.
inline auto splice(detail::expr_core& dst, const detail::expr_core& src) -> int
{
  const int node_base = static_cast<int>(dst.nodes.size());
  std::vector<int> leaf_remap;
  leaf_remap.reserve(src.leaves.size());
  for (const auto& leaf : src.leaves) {
    leaf_remap.push_back(dst.add_leaf(leaf));
  }
  for (auto node : src.nodes) {
    if (node.left >= 0) {
      node.left += node_base;
    }
    if (node.right >= 0) {
      node.right += node_base;
    }
    if (node.leaf >= 0) {
      node.leaf = leaf_remap[static_cast<std::size_t>(node.leaf)];
    }
    dst.add_node(node);
  }
  return node_base + src.root();
}

// Stand-in for std::is_specialization_of (C++23 reflection; absent
// from both supported standard libraries): true when `T` names
// `Primary` specialized on its own argument list.
template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type
{
};

template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type
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
class counter
{
public:
  using dimension_tag = D;

  detail::leaf_core leaf;

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
  [[nodiscard]] auto unit_token() const noexcept -> std::string_view
  {
    return leaf.unit;
  }

  /**
   * @brief The availability state at resolution (FR-006).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto avail() const noexcept -> availability
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
 * Declared through `push_provider` before registration. Cross-thread
 * `add` is a tier-3 violation, checked by default and elided under
 * `SG_CONTRACTS_IGNORE`.
 */
class SPEEDGUN_NG_EXPORT push_counter
{
public:
  /**
   * @brief Copies of a handle name the same counter.
   *
   * \pre none
   * \post none
   */
  push_counter(const push_counter&) = default;

  /**
   * @brief Moves of a handle name the same counter.
   *
   * \pre none
   * \post none
   */
  push_counter(push_counter&&) noexcept = default;

  /**
   * @brief Assignment names the same counter.
   *
   * \pre none
   * \post none
   */
  auto operator=(const push_counter&) -> push_counter& = default;

  /**
   * @brief Move assignment names the same counter.
   *
   * \pre none
   * \post none
   */
  auto operator=(push_counter&&) noexcept -> push_counter& = default;

  /**
   * @brief The trivial destruction of a handle.
   *
   * \pre none
   * \post none
   */
  ~push_counter() = default;

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
  friend class push_provider;

  push_counter(std::uint64_t* const value,
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
struct recorder_api
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
 */
struct points_view
{
  std::string_view object_path;
  std::string_view name;
  std::string_view description;
  std::string_view unit;
  std::size_t slot = 0;  // point identity: column index in the plan
  const std::uint64_t* points = nullptr;
  std::size_t count = 0;
  double ratio = 1.0;
};

/**
 * @brief Overflow policy tag: a sample past capacity is a violation
 * (FR-025). The default policy.
 *
 * \pre none
 * \post none
 */
struct hard_stop_t
{
};

/**
 * @brief Overflow policy tag: writes mask into the ring, drop with
 * accounting (FR-025, FR-028).
 *
 * \pre none
 * \post none
 */
struct ring_t
{
};

inline constexpr hard_stop_t hard_stop {};
inline constexpr ring_t ring {};

namespace detail
{

// Sampling cores behind recorder_handle::sample: the bounds-checked
// hard_stop path and the masked ring path (FR-026, FR-027, FR-028).
SPEEDGUN_NG_EXPORT auto hard_stop_sample_core(const void* impl,
                                              std::uint64_t* columns,
                                              std::size_t capacity,
                                              std::size_t& head) noexcept
    -> void;

SPEEDGUN_NG_EXPORT auto ring_sample_core(const void* impl,
                                         std::uint64_t* columns,
                                         std::size_t capacity,
                                         std::size_t& head,
                                         bool& wrapped,
                                         std::uint64_t& dropped) noexcept
    -> void;

}  // namespace detail

/**
 * @brief The recorder value handle (E-08, FR-029): a trivially
 * copyable cursor over one plan-arena buffer of `capacity` point
 * columns, one per compiled leaf. `P` is the overflow policy tag:
 * `hard_stop_t` or `ring_t`, chosen by the plan factory (FR-025).
 * The plan owns the buffer; the plan outlives its recorders.
 */
template<class P>
class recorder_handle
{
public:
  const void* m_impl = nullptr;
  std::uint64_t* m_columns = nullptr;
  std::size_t m_capacity = 0;
  std::size_t m_head = 0;
  bool m_wrapped = false;
  std::uint64_t m_dropped = 0;

  /**
   * @brief THE critical path: one sampling action appends one point
   * per column (FR-026). Zero allocation, zero lock, zero virtual
   * call. hard_stop overrun is an `SG_REQUIRE_ALWAYS` violation in
   * every build configuration (FR-027); ring masks into the buffer
   * and records wrapped plus dropped (FR-028).
   *
   * \pre none
   * \post none
   */
  auto sample() noexcept -> void
  {
    if constexpr (std::is_same_v<P, hard_stop_t>) {
      detail::hard_stop_sample_core(m_impl, m_columns, m_capacity, m_head);
    } else {
      detail::ring_sample_core(
          m_impl, m_columns, m_capacity, m_head, m_wrapped, m_dropped);
    }
  }

  /**
   * @brief The recorded-window view a fold reads: extent, wrap, and
   * drops of the retained window (FR-018, FR-028).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto view() const noexcept -> recorder_api
  {
    const auto committed =
        m_head < m_capacity ? m_head : static_cast<std::size_t>(m_capacity);
    return recorder_api {
        .impl = m_impl,
        .columns = m_columns,
        .stride = m_capacity,
        .count = committed,
        .wrapped = m_wrapped,
        .dropped = m_dropped,
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
    return m_capacity;
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
  [[nodiscard]] auto wrapped() const noexcept -> bool { return m_wrapped; }

  /**
   * @brief The count of overwritten points (FR-028).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto dropped() const noexcept -> std::uint64_t
  {
    return m_dropped;
  }
};

namespace detail
{

// Core operations behind the public template wrappers.
[[nodiscard]] SPEEDGUN_NG_EXPORT auto fold_core(const expr_core& core,
                                                const recorder_api& rec,
                                                std::size_t i,
                                                std::size_t j) -> metric_result;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto fold_pairs_core(const expr_core& core,
                                                      const recorder_api& rec)
    -> std::vector<metric_result>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto raw_core(const expr_core& core,
                                               const recorder_api& rec,
                                               std::string_view object_path,
                                               std::string_view leaf_name)
    -> std::expected<points_view, error>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto resolve_leaf_core(const object& obj,
                                                        std::string_view name)
    -> std::expected<leaf_core, error>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto compile_core(
    const system& sys,
    const target& tg,
    const std::vector<const expr_core*>& exprs) -> std::expected<plan, error>;

[[nodiscard]] SPEEDGUN_NG_EXPORT auto metric_core(const scope& scope_obj,
                                                  const expr_core& core)
    -> metric_result;

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
class expression
{
public:
  using dimension_tag = D;

  detail::expr_core core;

  /**
   * @brief An empty spine; compile rejects a zero-leaf expression.
   *
   * \pre none
   * \post none
   */
  expression() = default;

  /**
   * @brief Builds an expression from one resolved leaf: the leaf's
   * delta, unscaled.
   *
   * \pre none
   * \post none
   */
  template<class C>
    requires dim_same<D, C>
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  expression(const counter<C>& leaf)
  {
    const int leaf_index = core.add_leaf(leaf.leaf);
    core.add_node(detail::expr_node {
        .kind = 0, .left = -1, .right = -1, .leaf = leaf_index, .scale = 1.0});
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
  [[nodiscard]] auto fold(const recorder_api& rec,
                          std::size_t i,
                          std::size_t j) const -> metric_result
  {
    SG_REQUIRE(i < j && j < rec.count,
               "fold window lies within the recorded extent (FR-018)");
    return detail::fold_core(core, rec, i, j);
  }

  /**
   * @brief First-to-last fold (FR-018).
   *
   * \pre at least two committed points (tier-3).
   * \post none
   */
  [[nodiscard]] auto fold(const recorder_api& rec) const -> metric_result
  {
    SG_REQUIRE(rec.count >= 2,
               "first-to-last fold needs two committed points (FR-018)");
    return detail::fold_core(core, rec, 0, rec.count - 1);
  }

  /**
   * @brief One metric per adjacent recorded interval (FR-018); the
   * recorder holds at least two committed points (tier-3).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto fold_pairs(const recorder_api& rec) const
      -> std::vector<metric_result>
  {
    return detail::fold_pairs_core(core, rec);
  }

  /**
   * @brief Raw provenance view of one constituent leaf (FR-020).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto raw(const recorder_api& rec,
                         std::string_view object_path,
                         std::string_view leaf) const
      -> std::expected<points_view, error>
  {
    return detail::raw_core(core, rec, object_path, leaf);
  }

  // Hidden friends: non-template, so counter arguments convert and
  // same-typed operands match exactly.
  friend auto operator+(const expression& a, const expression& b) -> expression
  {
    auto merged = a.core;
    const int left = merged.root();
    const int right = detail::splice(merged, b.core);
    merged.add_node(detail::expr_node {
        .kind = 1, .left = left, .right = right, .leaf = -1, .scale = 1.0});
    auto out = expression();
    out.core = std::move(merged);
    return out;
  }

  friend auto operator-(const expression& a, const expression& b) -> expression
  {
    auto merged = a.core;
    const int left = merged.root();
    const int right = detail::splice(merged, b.core);
    merged.add_node(detail::expr_node {
        .kind = 2, .left = left, .right = right, .leaf = -1, .scale = 1.0});
    auto out = expression();
    out.core = std::move(merged);
    return out;
  }

  friend auto operator*(const double k, const expression& e) -> expression
  {
    auto merged = e.core;
    merged.scale_all(k);
    auto out = expression();
    out.core = std::move(merged);
    return out;
  }

  friend auto operator*(const expression& e, const double k) -> expression
  {
    return k * e;
  }
};

/**
 * @brief Cross-dimension division: exponents subtract (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const expression<D1>& a, const expression<D2>& b)
    -> expression<dim_quotient<D1, D2>>
{
  detail::expr_core merged;
  const int left = detail::splice(merged, a.core);
  const int right = detail::splice(merged, b.core);
  merged.add_node(detail::expr_node {
      .kind = 3, .left = left, .right = right, .leaf = -1, .scale = 1.0});
  auto out = expression<dim_quotient<D1, D2>>();
  out.core = std::move(merged);
  return out;
}

// Same-dimension addition and subtraction across distinct spellings.
// The body is dimension-independent: same tags means same spine
// algebra, and the static_assert names the violation.
template<class D1, class D2>
[[nodiscard]] auto operator+(const expression<D1>& a, const expression<D2>& b)
    -> expression<D1>
{
  static_assert(dim_same<D1, D2>,
                "expression addition requires identical dimension tags");
  auto merged = a.core;
  const int left = merged.root();
  const int right = detail::splice(merged, b.core);
  merged.add_node(detail::expr_node {
      .kind = 1, .left = left, .right = right, .leaf = -1, .scale = 1.0});
  auto out = expression<D1>();
  out.core = std::move(merged);
  return out;
}

template<class D1, class D2>
[[nodiscard]] auto operator-(const expression<D1>& a, const expression<D2>& b)
    -> expression<D1>
{
  static_assert(dim_same<D1, D2>,
                "expression subtraction requires identical dimension tags");
  auto merged = a.core;
  const int left = merged.root();
  const int right = detail::splice(merged, b.core);
  merged.add_node(detail::expr_node {
      .kind = 2, .left = left, .right = right, .leaf = -1, .scale = 1.0});
  auto out = expression<D1>();
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
[[nodiscard]] auto operator/(const counter<D1>& a, const counter<D2>& b)
    -> expression<dim_quotient<D1, D2>>
{
  return expression<D1>(a) / expression<D2>(b);
}

/**
 * @brief Divides an expression by a resolved counter (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const expression<D1>& a, const counter<D2>& b)
    -> expression<dim_quotient<D1, D2>>
{
  return a / expression<D2>(b);
}

/**
 * @brief Divides a resolved counter by an expression (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
[[nodiscard]] auto operator/(const counter<D1>& a, const expression<D2>& b)
    -> expression<dim_quotient<D1, D2>>
{
  return expression<D1>(a) / b;
}

template<class D1, class D2>
[[nodiscard]] auto operator+(const counter<D1>& a, const counter<D2>& b)
    -> expression<D1>
{
  static_assert(dim_same<D1, D2>,
                "counter addition requires identical dimension tags");
  return expression<D1>(a) + expression<D1>(b);
}

template<class D1, class D2>
[[nodiscard]] auto operator-(const counter<D1>& a, const counter<D2>& b)
    -> expression<D1>
{
  static_assert(dim_same<D1, D2>,
                "counter subtraction requires identical dimension tags");
  return expression<D1>(a) - expression<D1>(b);
}

/**
 * @brief The compiled read plan (E-07): flat leaf slots, per-provider
 * read groups, arena geometry, and the per-thread binding.
 *
 * Move-only; the plan owns the compiled layout. Plans and recorders
 * are per-thread objects (FR-031); multiple plans over one system are
 * first-class.
 */
class SPEEDGUN_NG_EXPORT plan
{
public:
  plan(const plan&) = delete;
  auto operator=(const plan&) -> plan& = delete;

  /**
   * @brief Moves the compiled layout.
   *
   * \pre none
   * \post `other` holds no layout.
   */
  plan(plan&& other) noexcept;

  /**
   * @brief Move assignment: `other` holds no layout.
   *
   * \pre none
   * \post none
   */
  auto operator=(plan&& other) noexcept -> plan&;

  /**
   * @brief Releases the compiled layout and provider windows; every
   * recorder or scope over this plan must be destroyed first.
   *
   * \pre none
   * \post none
   */
  ~plan();

  /**
   * @brief Mints a hard_stop recorder: `capacity` point columns are
   * allocated now from the plan arena; sampling allocates nothing
   * later (FR-025, FR-029). The plan outlives its recorders.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto recorder(std::size_t capacity) const
      -> recorder_handle<hard_stop_t>;

  /**
   * @brief Mints a ring recorder: `capacity` point columns are
   * allocated now. A capacity that is not a power of two is a
   * recoverable construction error (FR-025, R-006).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto recorder(std::size_t capacity, ring_t) const
      -> std::expected<recorder_handle<ring_t>, error>;

  /**
   * @brief The cheapest `sample()` in the recorded distribution, in
   * nanoseconds (FR-032).
   *
   * The first call on a plan runs the calibration: it samples the
   * plan's own read sequence over an empty workload and stores the
   * distribution on the plan. Compile itself performs no hardware read
   * (FR-021), so the reads happen here, on the caller's request. Fold a
   * window from `i` to `j` and its two endpoints cost
   * `2 * sample_overhead_ns_median()` of sampling on top of the work
   * the window measured (FR-032, FR-048).
   *
   * \pre none
   * \post The returned nanosecond count is at least 0.
   */
  [[nodiscard]] auto sample_overhead_ns_min() const -> double;

  /**
   * @brief The median `sample()` in the recorded distribution, in
   * nanoseconds (FR-032). The value a fold window's two endpoints cost.
   *
   * \pre none
   * \post The returned nanosecond count is at least 0.
   */
  [[nodiscard]] auto sample_overhead_ns_median() const -> double;

  /**
   * @brief The dearest `sample()` in the recorded distribution, in
   * nanoseconds (FR-032). A long tail is investigated, never assumed
   * away (Principle VII).
   *
   * \pre none
   * \post The returned nanosecond count is at least `min()`.
   */
  [[nodiscard]] auto sample_overhead_ns_max() const -> double;

private:
  friend auto detail::compile_core(
      const system& sys,
      const target& tg,
      const std::vector<const detail::expr_core*>& exprs)
      -> std::expected<plan, error>;
  friend class scope;

  explicit plan(void* impl) noexcept
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
 * scope must outlive it.
 */
class SPEEDGUN_NG_EXPORT scope
{
public:
  /**
   * @brief Prepares a two-point window over a compiled plan.
   *
   * \pre `compiled` outlives the scope.
   * \post The scope awaits `start()`.
   */
  explicit scope(const plan& compiled);

  scope(const scope&) = delete;
  auto operator=(const scope&) -> scope& = delete;
  scope(scope&&) = delete;
  auto operator=(scope&&) -> scope& = delete;

  /**
   * @brief Releases the two point columns.
   *
   * \pre none
   * \post none
   */
  ~scope();

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
   * `start` and `finish` must have been called (tier-3).
   *
   * \pre none
   * \post none
   */
  template<class D>
  [[nodiscard]] auto metric(const expression<D>& e) const -> metric_result
  {
    return detail::metric_core(*this, e.core);
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
  [[nodiscard]] auto view() const noexcept -> recorder_api;

private:
  friend auto detail::metric_core(const scope& scope_obj,
                                  const detail::expr_core& core)
      -> metric_result;

  void* m_core = nullptr;  // the scope internals
};

/**
 * @brief Compiles expressions into a plan (FR-021, FR-022): name
 * resolution through the system tree, dimension checking, group
 * layout, mode probing, and arena geometry, once in the untimed
 * region. Zero hardware reads (FR-021).
 *
 * \pre none
 * \post none
 */
template<class... E>
  requires(
      detail::is_specialization_of<std::remove_cvref_t<E>, expression>::value
      && ...)
[[nodiscard]] auto compile(const system& sys, const E&... exprs)
    -> std::expected<plan, error>
{
  const std::vector<const detail::expr_core*> cores {&exprs.core...};
  return detail::compile_core(sys, target {}, cores);
}

/**
 * @brief Compiles expressions into a plan bound to a target.
 *
 * \pre none
 * \post none
 */
template<class... E>
  requires(
      detail::is_specialization_of<std::remove_cvref_t<E>, expression>::value
      && ...)
[[nodiscard]] auto compile(const system& sys,
                           const target& tg,
                           const E&... exprs) -> std::expected<plan, error>
{
  const std::vector<const detail::expr_core*> cores {&exprs.core...};
  return detail::compile_core(sys, tg, cores);
}

/**
 * @brief One object's result of a fan-out fold (SC-007): the canonical
 * path and the metric over that object's instantiated leaves.
 */
struct fanout_result
{
  std::string object_path;  // canonical spelling (FR-002)
  metric_result metric;
};

class object;
class fanout_plan;

namespace detail
{

SPEEDGUN_NG_EXPORT auto compile_fanout_core(
    const system& sys,
    const target& tg,
    const expr_core& exemplar,
    const std::vector<const object*>& selection)
    -> std::expected<fanout_plan, error>;

SPEEDGUN_NG_EXPORT auto fanout_fold_core(const void* fanout,
                                         const expr_core& core,
                                         const recorder_api& rec)
    -> std::vector<fanout_result>;

}  // namespace detail

/**
 * @brief A fan-out plan (US3 scenario 5, SC-007): the exemplar
 * expression instantiated per selected object; every instantiated
 * leaf is read inside each shared sampling action (FR-047).
 * Move-only; the plan owns the layout and the recorder arenas.
 */
class SPEEDGUN_NG_EXPORT fanout_plan
{
public:
  fanout_plan(const fanout_plan&) = delete;
  auto operator=(const fanout_plan&) -> fanout_plan& = delete;

  /**
   * @brief Moves the compiled fan-out layout.
   *
   * \pre none
   * \post `other` holds no layout.
   */
  fanout_plan(fanout_plan&& other) noexcept;

  /**
   * @brief Move assignment: `other` holds no layout.
   *
   * \pre none
   * \post none
   */
  auto operator=(fanout_plan&& other) noexcept -> fanout_plan&;

  /**
   * @brief Releases the fan-out layout and arenas; every recorder
   * over this plan must be destroyed first.
   *
   * \pre none
   * \post none
   */
  ~fanout_plan();

  /**
   * @brief Mints a hard_stop recorder over the shared window:
   * `capacity` point columns per instantiated leaf; one sampling
   * action reads every instance (FR-047, FR-029). The plan outlives
   * its recorders.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto recorder(std::size_t capacity) const
      -> recorder_handle<hard_stop_t>;

  /**
   * @brief The selected objects' canonical paths, selection order.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto object_paths() const -> std::vector<std::string>;

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
  [[nodiscard]] auto fold(const expression<D>& e, const recorder_api& rec) const
      -> std::vector<fanout_result>
  {
    return detail::fanout_fold_core(m_impl, e.core, rec);
  }

private:
  friend auto detail::compile_fanout_core(
      const system& sys,
      const target& tg,
      const detail::expr_core& exemplar,
      const std::vector<const object*>& selection)
      -> std::expected<fanout_plan, error>;
  friend auto detail::fanout_fold_core(const void* fanout,
                                       const detail::expr_core& core,
                                       const recorder_api& rec)
      -> std::vector<fanout_result>;

  explicit fanout_plan(void* impl) noexcept
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
[[nodiscard]] auto compile(const system& sys,
                           const expression<D>& expr,
                           const std::vector<const object*>& selection)
    -> std::expected<fanout_plan, error>
{
  return detail::compile_fanout_core(sys, target {}, expr.core, selection);
}

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
