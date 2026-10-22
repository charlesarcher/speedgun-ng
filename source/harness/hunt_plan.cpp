// specs/029 FR-011 to FR-013, FR-030: the hunt plan, the rank table and the
// stage split. T023 and T024 define `planHunt` and `rankEvents` here in
// `sg::hunt`; T021 declares and T033 defines the internal split and budget
// cores in `sg::detail` behind those calls.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "detail/hunt_plan.hpp"

#include "detail/pass_plan.hpp"
#include "detail/statistics.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/hunt.hpp"

namespace sg::hunt
{

namespace
{

// The per-thousand scale of an `instr` or `cycle` rate (D-6).
constexpr double kRateScale = 1000.0;

// The factor between `RankedEvent::cv` and the `cv=` column ratio of the same
// samples (I-6).
constexpr double kPercentScale = 100.0;

// The anchor leaves one column asks of its device besides the event itself,
// the leaves that make the limit of a device below 3 admit no event (E-04).
constexpr std::size_t kAnchorLeaves = 2;

// The width of the stage-2 set as a multiple of `--hunt-top` (D-2).
constexpr std::size_t kStageTwoFactor = 3;

[[nodiscard]] auto refusal(const std::string& message) -> counters::Error
{
  return counters::Error {.message = message, .suggestions = {}};
}

// The rate one window contributes under one normalization, and nothing when it
// contributes none: a window that disclosed a gap or a refusal and a window
// whose divisor reads zero both stand out of the rate list, the way a gap
// repetition stands out of `Statistics::samples` today.
[[nodiscard]] auto rateOf(const HuntSample& sample,
                          const HuntNorm norm) -> std::optional<double>
{
  const auto counted = static_cast<double>(sample.eventValue);
  if (sample.disclosure.has_value()) {
    return std::nullopt;
  }
  if (norm == HuntNorm::INSTR) {
    if (!sample.anchorInstr.has_value() || *sample.anchorInstr == 0) {
      return std::nullopt;
    }
    return counted / static_cast<double>(*sample.anchorInstr) * kRateScale;
  }
  if (norm == HuntNorm::CYCLE) {
    if (!sample.anchorCycle.has_value() || *sample.anchorCycle == 0) {
      return std::nullopt;
    }
    return counted / static_cast<double>(*sample.anchorCycle) * kRateScale;
  }
  if (sample.iterations == 0) {
    return std::nullopt;
  }
  return counted / static_cast<double>(sample.iterations);
}

// The column indices of one plan in order, the shape both stages take.
[[nodiscard]] auto columnIndices(const std::size_t count)
    -> std::vector<std::size_t>
{
  auto indices = std::vector<std::size_t> {};
  indices.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    indices.push_back(index);
  }
  return indices;
}

}  // namespace

auto planHunt(const std::vector<CatalogAddress>& events,
              const std::vector<std::optional<std::size_t>>& sourceLimits,
              const std::vector<std::pair<std::string, std::string>>& anchors,
              const HuntOptions& options,
              const std::optional<std::uint64_t> calibrationNsPerPass)
    -> std::expected<HuntPlan, counters::Error>
{
  SG_REQUIRE(sourceLimits.size() == anchors.size(),
             "the caller builds one limit entry and one anchor pair per "
             "device, " "in the same device order, from the catalog walk");
  auto plan = HuntPlan {};
  auto leaves = std::vector<std::vector<detail::ColumnLeaves>> {};
  leaves.reserve(events.size());

  for (const auto& entry : events) {
    if (entry.source >= sourceLimits.size()) {
      return std::unexpected(refusal(
          "hunt: event '" + entry.address + "' names counter source "
          + std::to_string(entry.source) + " of a limit list that holds "
          + std::to_string(sourceLimits.size()) + " devices"));
    }
    const auto& pair = anchors[entry.source];
    const auto limit = sourceLimits[entry.source];
    if (limit.has_value() && *limit <= kAnchorLeaves && !pair.first.empty()) {
      return std::unexpected(
          refusal("hunt: device '" + entry.device + "' publishes a limit of "
                  + std::to_string(*limit) + ", below the "
                  + std::to_string(kAnchorLeaves + 1)
                  + " leaves one event and its two anchors need"));
    }
    if (options.norm == HuntNorm::INSTR && pair.second.empty()) {
      return std::unexpected(refusal(
          "hunt: norm 'instr' needs the instructions " "anchor of device '"
          + entry.device + "', which the device does not publish"));
    }
    if (options.norm == HuntNorm::CYCLE && pair.first.empty()) {
      return std::unexpected(refusal(
          "hunt: norm 'cycle' needs the cpu-cycles " "anchor of device '"
          + entry.device + "', which the device does not publish"));
    }

    auto& column = plan.columns.emplace_back();
    column.event = entry.address;
    column.cycles = pair.first;
    column.instructions = pair.second;
    column.source = entry.source;

    // One source per hunt column: the event and the two anchors of its own
    // device, the leaf set `planPasses` deduplicates per pass (I-2).
    auto columnLeaves = detail::ColumnLeaves {};
    columnLeaves.source = entry.source;
    columnLeaves.addresses.push_back(entry.address);
    if (!pair.first.empty()) {
      columnLeaves.addresses.push_back(pair.first);
    }
    if (!pair.second.empty()) {
      columnLeaves.addresses.push_back(pair.second);
    }
    leaves.emplace_back(
        std::vector<detail::ColumnLeaves> {std::move(columnLeaves)});
  }

  // A device no column needs publishes nothing to this run: only the devices
  // the columns stand on decide the method (FR-011, spec PC-2).
  const auto anyLimit =
      std::ranges::any_of(plan.columns,
                          [&sourceLimits](const HuntColumn& column)
                          { return sourceLimits[column.source].has_value(); });
  plan.method =
      detail::settleMethod(MetricMethod::AUTO, anyLimit, plan.columns.size());

  const auto anchored = !plan.columns.empty()
      && std::ranges::all_of(
          plan.columns,
          [](const HuntColumn& column)
          { return !column.cycles.empty() && !column.instructions.empty(); });
  plan.anchorsPerPass = anchored ? kAnchorLeaves : 0;
  plan.normalized = plan.anchorsPerPass == kAnchorLeaves;

  if (!plan.columns.empty()) {
    const auto passPlan = detail::planPasses(leaves, sourceLimits);
    if (passPlan.columns.empty()) {
      // No needed device publishes a limit, so the planner answers no plan and
      // the run measures every column in one multiplexed window (D-19).
      plan.passes.push_back(columnIndices(plan.columns.size()));
    } else {
      plan.passes = passPlan.columns;
    }

    // shortcut: plan order stands in for the stage-1 rate order, which no rate
    // exists for before the run measured stage 1; splitStages (T033) owns the
    // ordering and the budget cut verdict.
    auto stages = std::vector<HuntStage> {};
    if (!options.budget.has_value()) {
      stages.push_back(
          HuntStage {columnIndices(plan.columns.size()), options.repeats});
    } else {
      stages.push_back(HuntStage {columnIndices(plan.columns.size()), 1});
      const auto width =
          std::min(kStageTwoFactor * options.top, plan.columns.size());
      stages.push_back(HuntStage {columnIndices(width), options.repeats});
    }
    plan.stages = std::move(stages);

    if (calibrationNsPerPass.has_value() && !plan.stages.empty()) {
      plan.estimateNs = *calibrationNsPerPass * plan.passes.size()
          * plan.stages.front().repeats;
    }
  }

  auto seated = std::size_t {0};
  for (const auto& pass : plan.passes) {
    seated += pass.size();
  }
  SG_ENSURE(seated == plan.columns.size(),
            "the planner seats every column of the request in one pass and "
            "leaves none out");
  SG_ENSURE(plan.normalized == (plan.anchorsPerPass == kAnchorLeaves),
            "the plan normalizes by an anchor exactly when both anchors ride "
            "its passes");
  return plan;
}

auto rankEvents(const std::vector<EventSamples>& collected,
                const HuntOptions& options) -> EntryHunt
{
  SG_REQUIRE(options.top > 0 && options.noise > 0.0,
             "the caller hands the settled options, every value inside the "
             "range its own option states");
  // An unset norm plans `instr` (E-03), the normalization a plan falls back to
  // only when a device lacks its anchor.
  const auto norm = options.norm.value_or(HuntNorm::INSTR);
  auto result = EntryHunt {};
  result.top = options.top;
  // `rankEvents` takes no plan and settles no method of its own, so the record
  // states none and the mark reads false; the run that settled `MULTIPLEX`
  // marks its entries with that method (AC-2).
  const auto multiplexed = result.method == MetricMethod::MULTIPLEX;
  result.entries.reserve(collected.size());

  for (const auto& event : collected) {
    auto rates = std::vector<double> {};
    rates.reserve(event.samples.size());
    for (const auto& sample : event.samples) {
      if (sample.disclosure.has_value()) {
        // A disclosed window measured nothing, so it joins neither the
        // median nor the spread, exactly as the pass disclosure keeps it
        // out of the aggregate (E-05, FR-022).
        continue;
      }
      if (const auto rate = rateOf(sample, norm); rate.has_value()) {
        rates.push_back(*rate);
      }
    }
    SG_ASSERT(!rates.empty() || event.skipped.has_value(),
              "every address with no refusal kind holds at least one sample "
              "free of a disclosure, the guard `statisticsOf` carries");

    auto& entry = result.entries.emplace_back();
    entry.address = event.address;
    entry.samples = rates.size();
    entry.multiplexed = multiplexed;
    if (rates.size() > 1) {
      const auto stats = detail::statisticsOf(rates);
      entry.rate = stats.median;
      entry.cv = kPercentScale * stats.coefficientOfVariation;
      entry.noisy = entry.cv > options.noise;
      entry.repeated = true;
    } else if (!rates.empty()) {
      // One sample is reported and never summarized, so no deviation exists
      // for it and an unknown deviation is not a noisy event (PD-14).
      entry.rate = rates.front();
    }
    if (event.skipped.has_value()) {
      result.skipped.push_back(
          SkippedEvent {event.address, *event.skipped, std::nullopt});
    }
  }

  // The rank order of `--hunt-rank` with the catalog address as the tie break,
  // so one sample list gives one order whatever order it arrived in (PD-07).
  auto ranked = std::vector<std::size_t> {};
  for (std::size_t index = 0; index < result.entries.size(); ++index) {
    if (!result.entries[index].noisy && result.entries[index].repeated) {
      ranked.push_back(index);
    }
  }
  std::ranges::sort(
      ranked,
      [&result, &options](const std::size_t lhs, const std::size_t rhs)
      {
        const auto& left = result.entries[lhs];
        const auto& right = result.entries[rhs];
        if (options.rank == HuntRank::RATE) {
          if (left.rate > right.rate) {
            return true;
          }
          if (left.rate < right.rate) {
            return false;
          }
        } else {
          if (left.cv < right.cv) {
            return true;
          }
          if (left.cv > right.cv) {
            return false;
          }
        }
        return left.address < right.address;
      });

  const auto rankedCount = std::min(options.top, ranked.size());
  for (std::size_t position = 0; position < rankedCount; ++position) {
    result.entries[ranked[position]].rank = position + 1;
  }

  // The ranked entries in rank order, then every excluded one - noisy,
  // unrepeated or beyond the top - in address order.
  auto tail = std::vector<std::size_t>(
      ranked.begin() + static_cast<std::ptrdiff_t>(rankedCount), ranked.end());
  for (std::size_t index = 0; index < result.entries.size(); ++index) {
    if (!result.entries[index].noisy && result.entries[index].repeated) {
      continue;
    }
    tail.push_back(index);
  }
  std::ranges::sort(
      tail,
      [&](const std::size_t lhs, const std::size_t rhs)
      { return result.entries[lhs].address < result.entries[rhs].address; });

  auto ordered = std::vector<RankedEvent> {};
  ordered.reserve(result.entries.size());
  for (std::size_t position = 0; position < rankedCount; ++position) {
    ordered.push_back(std::move(result.entries[ranked[position]]));
  }
  for (const auto index : tail) {
    ordered.push_back(std::move(result.entries[index]));
  }
  result.entries = std::move(ordered);

  SG_ENSURE(result.entries.size() == collected.size(),
            "one record stands in the table per address the run handed over");
  return result;
}

}  // namespace sg::hunt
