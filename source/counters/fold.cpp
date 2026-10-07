// Folds over recorded point columns: the pure evaluator, pair folds,
// raw provenance, and the scope metric (specs/007-counters-and-timers,
// FR-013, FR-018, FR-020, FR-030).

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <optional>
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

struct FoldContext
{
  const PlanImpl& layout;
  const RecorderApi& rec;
  std::size_t i;
  std::size_t j;
};

// Evaluate one spine node over the window [i, j]. Leaf deltas
// subtract modularly at 2^64, so one hardware wrap subtracts out of
// every fold (FR-013).
[[nodiscard]] auto eval(const FoldContext& ctx,
                        const ExprCore& core,
                        const int index) -> double
{
  const auto& node = core.nodes[static_cast<std::size_t>(index)];
  switch (node.kind) {
    case 0: {
      const auto& leaf = core.leaves[static_cast<std::size_t>(node.leaf)];
      const std::size_t slot = ctx.layout.byAddress.at(leaf.address);
      const auto* column = ctx.rec.columns + slot * ctx.rec.stride;
      const auto delta = column[ctx.j] - column[ctx.i];
      return static_cast<double>(delta);
    }
    case 1:
      return eval(ctx, core, node.left) + eval(ctx, core, node.right);
    case 2:
      return eval(ctx, core, node.left) - eval(ctx, core, node.right);
    case 4:
      return eval(ctx, core, node.left) * node.scale;
    default:
      return eval(ctx, core, node.left) / eval(ctx, core, node.right);
  }
}

[[nodiscard]] auto sameDouble(const double a, const double b) -> bool
{
  return std::bit_cast<std::uint64_t>(a) == std::bit_cast<std::uint64_t>(b);
}

// Whether a scale the caller applied multiplied a value in this
// window. A scale node multiplies the value its operand folds to, so
// every node carrying a factor other than 1.0 reaches a value (FR-019).
[[nodiscard]] auto carriesScale(const ExprCore& core) -> bool
{
  for (const auto& node : core.nodes) {
    if (!sameDouble(node.scale, 1.0)) {
      return true;
    }
  }
  return false;
}

// The algebraic exponent of one leaf in the spine, accumulated over
// every occurrence: a quotient inverts the sign of its right operand,
// an addition and a subtraction leave it alone, and a scalar multiple
// passes it through. A leaf quoted on both sides of a subtraction, as
// in `(a - b) / (a + b)`, therefore has exponent 0. "instructions /
// cycles" weighs the instructions ratio with +1 and the cycles ratio
// with -1, which is what the composite ratio product needs (FR-019,
// measurement-contract clarification 2). Zero for any other leaf.
[[nodiscard]] auto leafSign(const ExprCore& core,
                            const int nodeIndex,
                            const int targetLeaf,
                            const int sign) -> int
{
  const auto& node = core.nodes[static_cast<std::size_t>(nodeIndex)];
  if (node.kind == 0) {
    return node.leaf == targetLeaf ? sign : 0;
  }
  if (node.kind == 4) {
    return leafSign(core, node.left, targetLeaf, sign);
  }
  const int left = leafSign(core, node.left, targetLeaf, sign);
  const int right =
      leafSign(core, node.right, targetLeaf, node.kind == 3 ? -sign : sign);
  return left + right;
}

// The disclosure column that owns one leaf's slot. Each read group has its
// own column, laid out past that group's own slot range, so a fold reads the
// column beside the leaf it is folding. One column for the whole plan
// described only the group owning the plan's last leaf (FR-001, FR-002,
// FR-004).
[[nodiscard]] auto disclosureSlotFor(const FoldContext& ctx,
                                     const std::size_t slot) -> std::size_t
{
  // Groups are laid out in offset order from zero, so a leaf slot is at
  // or past every group the scan has already passed. The owning group is
  // the first whose range end lies past the slot (FR-002).
  for (const auto& group : ctx.layout.groups) {  // LCOV_EXCL_BR_LINE
    if (slot < group.offset + group.count) {
      return group.disclosureSlot;
    }
  }
  // Every leaf slot belongs to a group, so the scan returns inside the
  // loop. The plan's own column is the single-group answer a caller reads
  // when it names no leaf.
  return ctx.layout.disclosureSlot;  // LCOV_EXCL_LINE
}

// One leaf's measured fraction of the window it was enabled for. The
// fold over a composite and the raw view of a single leaf disclose the
// same pair, so both read it here and the two disclosures cannot drift
// (FR-019, FR-020). No value means the leaf discloses no measured
// fraction: it carries no enabled/running pair, or no enabled time
// elapsed across the window.
[[nodiscard]] auto leafRatio(const FoldContext& ctx,
                             const std::size_t slot) -> std::optional<double>
{
  const auto& entry = ctx.layout.slots[slot];
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the second operand can
  // never be the deciding one. `link_ratio_slots` (`plan.cpp:135-158`) writes
  // `ratio_enabled` and `ratio_running` together at `plan.cpp:150-156` or
  // leaves both at `no_ratio_slot`, so a slot that passes the first test
  // always passes the second.
  if (entry.ratioEnabled == PlanImpl::kNoRatioSlot
      || entry.ratioRunning == PlanImpl::kNoRatioSlot)  // LCOV_EXCL_BR_LINE
  {
    return std::nullopt;  // LCOV_EXCL_LINE
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
  const auto* enabled = ctx.rec.columns + entry.ratioEnabled * ctx.rec.stride;
  const auto* running = ctx.rec.columns + entry.ratioRunning * ctx.rec.stride;
  const auto elapsed = enabled[ctx.j] - enabled[ctx.i];
  if (elapsed == 0) {
    return std::nullopt;
  }
  return static_cast<double>(running[ctx.j] - running[ctx.i])
      / static_cast<double>(elapsed);
}

// A disclosed fraction, and whether the window ran any source below
// full rate.
struct RatioResult
{
  double ratio = 1.0;
  bool multiplexed = false;
};

// The multiplex ratio of one window: the product of the constituent
// ratios, each raised to its algebraic exponent (FR-019). A source
// without an enabled/running pair contributes 1.0 by construction. A
// pair with no elapsed enabled time contributes 1.0; the fold has no
// measured fraction to report and states full rate.
[[nodiscard]] auto windowRatio(const FoldContext& ctx,
                               const ExprCore& core) -> RatioResult
{
  RatioResult out;
  for (std::size_t index = 0; index < core.leaves.size(); ++index) {
    const auto& leaf = core.leaves[index];
    const auto located = ctx.layout.byAddress.find(leaf.address);
    // LCOV_EXCL_BR_START : coverage exclusion (T066): a fan-out
    // instantiates the exemplar for every selected path and compiles those
    // instances (`plan.cpp:551-572`), so every address a fan-out fold looks
    // up is in the layout address table by construction. A plain plan
    // compiles the caller own expression, so its leaves are in the table
    // as well.
    if (located == ctx.layout.byAddress.end()) {  // LCOV_EXCL_BR_LINE
      continue;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_BR_STOP
    const auto one = leafRatio(ctx, located->second);
    if (!one.has_value()) {
      continue;
    }
    const int sign = leafSign(core, core.root(), static_cast<int>(index), 1);
    // A leaf quoted on both sides of a subtraction has exponent 0 and
    // contributes ratio^0 = 1.0 to the product (FR-019).
    if (sign != 0) {
      out.ratio *= sign > 0 ? *one : (1.0 / *one);
    }
    if (*one < 1.0) {
      out.multiplexed = true;
    }
  }
  return out;
}

// The state a window's disclosure column holds at one of its end points,
// read as the enumeration the column encodes. The column is the plan's
// managed disclosure column for this window, and the row is the sampling
// action named by the caller's logical index (FR-001).
[[nodiscard]] auto endPointState(const FoldContext& ctx,
                                 const std::size_t row,
                                 const std::size_t slot) -> Availability
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto disclosureIndex =
      (disclosureSlotFor(ctx, slot) * ctx.rec.stride) + row;
  // The record's column base is a pointer into one contiguous buffer and
  // the index is computed from the layout that buffer was built for, so
  // the subscript is in range by construction.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return static_cast<Availability>(ctx.rec.columns[disclosureIndex]);
}

// Whether either end point of the window is an action that measured
// nothing. A fold subtracts across its two end points, so a gap at either
// one subtracts a count the read never produced, and the result is a number
// no measurement supports. A gap strictly inside the window changes
// nothing: the recorded counts are cumulative, so a window with two
// measured end points has an exact delta between them (FR-001).
[[nodiscard]] auto windowIsGap(const FoldContext& ctx,
                               const ExprCore& core) -> bool
{
  const auto gap = Availability::GAP;
  // Every leaf's own group decides, so a plan drawing leaves from two
  // providers reads each provider's own disclosure column (FR-001,
  // FR-002).
  return std::ranges::any_of(core.leaves,
                             [&](const auto& leaf) -> bool
                             {
                               const std::size_t slot =
                                   ctx.layout.byAddress.at(leaf.address);
                               return endPointState(ctx, ctx.i, slot) == gap
                                   || endPointState(ctx, ctx.j, slot) == gap;
                             });
}

}  // namespace

auto foldCore(const ExprCore& core,
              const RecorderApi& rec,
              const std::size_t i,
              const std::size_t j) -> MetricResult
{
  SG_REQUIRE(i < j && j < rec.count,
             "fold window lies within the recorded extent (FR-018)");
  if (core.empty()) {
    return MetricResult {};
  }
  // A wrapped ring stores the retained window from the oldest point
  // at physical slot `dropped % stride`; folds address it logically
  // and consult the drops (FR-028).
  const std::size_t offset =
      rec.wrapped ? static_cast<std::size_t>(rec.dropped % rec.stride) : 0;
  const FoldContext ctx {
      .layout = *static_cast<const PlanImpl*>(rec.impl),
      .rec = rec,
      .i = rec.wrapped ? (offset + i) % rec.stride : i,
      .j = rec.wrapped ? (offset + j) % rec.stride : j,
  };
  // Push counters carry no hardware monotonicity guarantee: a value
  // that decreased between the two folded points is user misuse,
  // tier-3 (FR-035).
  for (const auto& leaf : core.leaves) {
    if (leaf.mode == ReadMode::PUSH_LOAD) {
      const std::size_t slot = ctx.layout.byAddress.at(leaf.address);
      const auto* column = ctx.rec.columns + slot * ctx.rec.stride;
      SG_REQUIRE(column[ctx.j] >= column[ctx.i],
                 "push counters never decrease between folded points (FR-035)");
      // The check is semantic-gated, so an ignoring build emits no code
      // for it and never reads `column`. The discard keeps the committed
      // warning set intact (T174); writing the address lookup into the
      // predicate instead would evaluate it twice per leaf.
      static_cast<void>(column);
    }
  }
  // A window with a gap at either end point reports no measured value.
  // The value stays at its default, so a caller that ignores the state
  // reads zero and never reads a fabricated delta. The state says why
  // (FR-001, FR-004).
  if (windowIsGap(ctx, core)) {
    return MetricResult {
        .value = 0.0,
        .runningRatio = 1.0,
        .availability = Availability::GAP,
        .scaled = false,
    };
  }
  const RatioResult disclosure = windowRatio(ctx, core);
  // Both end points were measured. The state is the one the column beside
  // the spine's first leaf holds at the end-point row, which is the leaf a
  // caller names when it reads a raw view of the same window (FR-004).
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  const std::size_t rootSlot = ctx.layout.byAddress.at(core.leaves[0].address);
  return MetricResult {
      .value = eval(ctx, core, core.root()),
      .runningRatio = disclosure.ratio,
      .availability = endPointState(ctx, ctx.j, rootSlot),
      // A window whose sources were multiplexed carries the kernel's
      // scaled estimate, and a fold the caller scaled carries one too;
      // a window that ran to completion at full rate carries neither
      // (FR-019).
      .scaled = disclosure.multiplexed || carriesScale(core),
  };
}

auto foldPairsCore(const ExprCore& core,
                   const RecorderApi& rec) -> std::vector<MetricResult>
{
  SG_REQUIRE(rec.count >= 2,
             "pair folds need at least two committed points (FR-018)");
  std::vector<MetricResult> out;
  for (std::size_t k = 0; k + 1 < rec.count; ++k) {
    out.push_back(foldCore(core, rec, k, k + 1));
  }
  return out;
}  // LCOV_EXCL_LINE

auto rawCore(const ExprCore& core,
             const RecorderApi& rec,
             const std::string_view objectPath,
             const std::string_view leafName)
    -> std::expected<PointsView, Error>
{
  const std::string address =
      std::string(objectPath) + "/" + std::string(leafName);
  for (const auto& leaf : core.leaves) {
    if (leaf.address == address) {
      const auto* layout = static_cast<const PlanImpl*>(rec.impl);
      const std::size_t slot = layout->byAddress.at(address);
      // The ratio covers the window this view spans: the first to the
      // last recorded point, and, for a wrapped ring, the oldest to the
      // newest retained row (FR-020, FR-028). A view with no recorded
      // point has no window, so both ends are row zero and no elapsed
      // time discloses a fraction.
      const std::size_t oldest =
          rec.wrapped ? static_cast<std::size_t>(rec.dropped % rec.stride) : 0;
      const FoldContext ctx {
          .layout = *layout,
          .rec = rec,
          .i = oldest,
          .j = rec.wrapped ? (oldest + rec.stride - 1) % rec.stride
                           : (rec.count == 0 ? 0 : rec.count - 1),
      };
      // The window's own end-point state, read from the disclosure column
      // beside this leaf's own group. A caller reading raw points reads the
      // state those points were taken under without naming a column index
      // in its own source (FR-004, FR-005).
      const auto state = endPointState(ctx, ctx.j, slot);
      return PointsView {
          .objectPath = objectPath,
          .name = leaf.name,
          .description = leaf.description,
          .unit = leaf.unit,
          .slot = slot,
          .points = rec.columns + slot * rec.stride,
          .count = rec.count,
          // The fraction this leaf ran for. The state decides the
          // fallback, so the ratio a caller reads and the state beside it
          // cannot disagree (FR-004, FR-005, FR-019, FR-020).
          // LCOV_EXCL_BR_START : coverage exclusion (T140): the gap
          // fallback. A runner that never records a gap takes only one arm.
          .ratio = (state == Availability::GAP)
              ? 1.0
              : leafRatio(ctx, slot).value_or(1.0),
          // LCOV_EXCL_BR_STOP
          .availability = state,
      };
    }
  }
  return std::unexpected(
      Error {.message = "expression does not contain leaf '" + address + "'",
             .suggestions = {}});
}

auto metricCore(const Scope& scopeObj, const ExprCore& core) -> MetricResult
{
  const auto* coreObj = static_cast<const ScopeCore*>(scopeObj.m_core);
  SG_REQUIRE(coreObj->started && coreObj->finished,
             "scope metric folds a closed window (FR-046)");
  return foldCore(core, coreObj->view(), 0, 1);
}

}  // namespace sg::counters::detail
