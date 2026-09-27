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

}  // namespace

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
  return metric_result {
      .value = eval(ctx, core, core.root()),
      .running_ratio = 1.0,
      .scaled = carries_scale(core),
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
