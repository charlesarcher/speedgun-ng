#ifndef SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
#define SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
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
inline auto splice(detail::expr_core& dst, const detail::expr_core& src) -> int
{
  const int leaf_base = static_cast<int>(dst.leaves.size());
  const int node_base = static_cast<int>(dst.nodes.size());
  for (const auto& leaf : src.leaves) {
    dst.add_leaf(leaf);
  }
  for (auto node : src.nodes) {
    if (node.left >= 0) {
      node.left += node_base;
    }
    if (node.right >= 0) {
      node.right += node_base;
    }
    if (node.leaf >= 0) {
      node.leaf += leaf_base;
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

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP
