// Folds over recorded point columns: the pure evaluator, pair folds,
// raw provenance, and the scope metric (specs/007-counters-and-timers,
// FR-013, FR-018, FR-020, FR-030).

#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "detail/core.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters::detail
{
namespace
{

struct fold_context
{
  const plan_impl& layout;
  const recorder_api& rec;
  std::size_t i;
  std::size_t j;
};

// Evaluate one spine node over the window [i, j]. Leaf deltas
// subtract modularly at 2^64, so one hardware wrap subtracts out of
// every fold (FR-013).
[[nodiscard]] auto eval(const fold_context& ctx,
                        const expr_core& core,
                        const int index) -> double
{
  const auto& node = core.nodes[static_cast<std::size_t>(index)];
  switch (node.kind) {
    case 0: {
      const auto& leaf = core.leaves[static_cast<std::size_t>(node.leaf)];
      const std::size_t slot = ctx.layout.by_address.at(leaf.address);
      const auto* column = ctx.rec.columns + slot * ctx.rec.stride;
      const auto delta = column[ctx.j] - column[ctx.i];
      return static_cast<double>(delta) * node.scale;
    }
    case 1:
      return eval(ctx, core, node.left) + eval(ctx, core, node.right);
    case 2:
      return eval(ctx, core, node.left) - eval(ctx, core, node.right);
    default:
      return eval(ctx, core, node.left) / eval(ctx, core, node.right);
  }
}

[[nodiscard]] auto same_double(const double a, const double b) -> bool
{
  return std::bit_cast<std::uint64_t>(a) == std::bit_cast<std::uint64_t>(b);
}

[[nodiscard]] auto carries_scale(const expr_core& core) -> bool
{
  for (const auto& node : core.nodes) {
    if (node.kind == 0 && !same_double(node.scale, 1.0)) {
      return true;
    }
  }
  return false;
}

// The algebraic exponent of one leaf in the spine, as a sign: a
// quotient inverts the sign of its right operand, an addition and a
// subtraction leave it alone. "instructions / cycles" therefore weighs
// the instructions ratio with +1 and the cycles ratio with -1, which is
// what the composite ratio product needs (FR-019, measurement-contract
// clarification 2). One for any other leaf.
[[nodiscard]] auto leaf_sign(const expr_core& core,
                             const int node_index,
                             const int target_leaf,
                             const int sign) -> int
{
  const auto& node = core.nodes[static_cast<std::size_t>(node_index)];
  if (node.kind == 0) {
    return node.leaf == target_leaf ? sign : 1;
  }
  const int left = leaf_sign(core, node.left, target_leaf, sign);
  const int right =
      leaf_sign(core, node.right, target_leaf, node.kind == 3 ? -sign : sign);
  return left * right;
}

}  // namespace

// The multiplex ratio of one window: the product of the constituent
// ratios, each raised to its algebraic exponent (FR-019). A source
// without an enabled/running pair contributes 1.0 by construction. A
// pair with no elapsed enabled time contributes 1.0; the fold has no
// measured fraction to report and states full rate.
struct ratio_result
{
  double ratio = 1.0;
  bool multiplexed = false;
};

[[nodiscard]] auto window_ratio(const fold_context& ctx, const expr_core& core)
    -> ratio_result
{
  ratio_result out;
  for (std::size_t index = 0; index < core.leaves.size(); ++index) {
    const auto& leaf = core.leaves[index];
    const auto located = ctx.layout.by_address.find(leaf.address);
    if (located == ctx.layout.by_address.end()) {
      continue;
    }
    const auto& slot = ctx.layout.slots[located->second];
    if (slot.ratio_enabled == plan_impl::no_ratio_slot
        || slot.ratio_running == plan_impl::no_ratio_slot)
    {
      continue;
    }
    const auto* enabled = ctx.rec.columns + slot.ratio_enabled * ctx.rec.stride;
    const auto* running = ctx.rec.columns + slot.ratio_running * ctx.rec.stride;
    const auto elapsed = enabled[ctx.j] - enabled[ctx.i];
    if (elapsed == 0) {
      continue;
    }
    const auto counted = running[ctx.j] - running[ctx.i];
    const double one =
        static_cast<double>(counted) / static_cast<double>(elapsed);
    const int sign = leaf_sign(core, core.root(), static_cast<int>(index), 1);
    out.ratio *= sign > 0 ? one : (1.0 / one);
    if (one < 1.0) {
      out.multiplexed = true;
    }
  }
  return out;
}

auto fold_core(const expr_core& core,
               const recorder_api& rec,
               const std::size_t i,
               const std::size_t j) -> metric_result
{
  SG_REQUIRE(i < j && j < rec.count,
             "fold window lies within the recorded extent (FR-018)");
  if (core.empty()) {
    return metric_result {};
  }
  // A wrapped ring stores the retained window from the oldest point
  // at physical slot `dropped % stride`; folds address it logically
  // and consult the drops (FR-028).
  const std::size_t offset =
      rec.wrapped ? static_cast<std::size_t>(rec.dropped % rec.stride) : 0;
  const fold_context ctx {
      .layout = *static_cast<const plan_impl*>(rec.impl),
      .rec = rec,
      .i = rec.wrapped ? (offset + i) % rec.stride : i,
      .j = rec.wrapped ? (offset + j) % rec.stride : j,
  };
  // Push counters carry no hardware monotonicity guarantee: a value
  // that decreased between the two folded points is user misuse,
  // tier-3 (FR-035).
  for (const auto& leaf : core.leaves) {
    if (leaf.mode == read_mode::push_load) {
      const std::size_t slot = ctx.layout.by_address.at(leaf.address);
      const auto* column = ctx.rec.columns + slot * ctx.rec.stride;
      SG_REQUIRE(column[ctx.j] >= column[ctx.i],
                 "push counters never decrease between folded points (FR-035)");
    }
  }
  const ratio_result disclosure = window_ratio(ctx, core);
  return metric_result {
      .value = eval(ctx, core, core.root()),
      .running_ratio = disclosure.ratio,
      // A window whose sources were multiplexed carries the kernel's
      // scaled estimate, and a fold the caller scaled carries one too;
      // a window that ran to completion at full rate carries neither
      // (FR-019).
      .scaled = disclosure.multiplexed || carries_scale(core),
  };
}

auto fold_pairs_core(const expr_core& core, const recorder_api& rec)
    -> std::vector<metric_result>
{
  SG_REQUIRE(rec.count >= 2,
             "pair folds need at least two committed points (FR-018)");
  std::vector<metric_result> out;
  for (std::size_t k = 0; k + 1 < rec.count; ++k) {
    out.push_back(fold_core(core, rec, k, k + 1));
  }
  return out;
}

auto raw_core(const expr_core& core,
              const recorder_api& rec,
              const std::string_view object_path,
              const std::string_view leaf_name)
    -> std::expected<points_view, error>
{
  const std::string address =
      std::string(object_path) + "/" + std::string(leaf_name);
  for (const auto& leaf : core.leaves) {
    if (leaf.address == address) {
      const auto* layout = static_cast<const plan_impl*>(rec.impl);
      const std::size_t slot = layout->by_address.at(address);
      return points_view {
          .object_path = object_path,
          .name = leaf.name,
          .description = leaf.description,
          .unit = leaf.unit,
          .slot = slot,
          .points = rec.columns + slot * rec.stride,
          .count = rec.count,
          .ratio = 1.0,
      };
    }
  }
  return std::unexpected(
      error {.message = "expression does not contain leaf '" + address + "'",
             .suggestions = {}});
}

auto metric_core(const scope& scope_obj, const expr_core& core) -> metric_result
{
  const auto* core_obj = static_cast<const scope_core*>(scope_obj.m_core);
  SG_REQUIRE(core_obj->started && core_obj->finished,
             "scope metric folds a closed window (FR-046)");
  return fold_core(core, core_obj->view(), 0, 1);
}

}  // namespace sg::counters::detail
