// specs/024: the pass planner, merge and spread declared in
// detail/pass_plan.hpp (FR-032).

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "detail/pass_plan.hpp"

#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// The leaf addresses one column asks, bucketed by the source that answers for
// them (E-03 rule 2). A bucket is the column's ask as its caller wrote it:
// the deduplication that makes a shared leaf claim one counter is
// `claimedAddresses` below, so an address a column asks twice stands once in
// what its pass ends up holding.
[[nodiscard]] auto askedAddresses(const std::vector<ColumnLeaves>& groups,
                                  const std::size_t sources)
    -> std::vector<std::vector<std::string>>
{
  std::vector<std::vector<std::string>> asked(sources);
  for (const auto& group : groups) {
    for (const auto& address : group.addresses) {
      asked[group.source].push_back(address);
    }
  }
  // A `return asked;` would make `asked` the return object itself, and gcov
  // would then attach an unexecutable count-zero record to the closing brace.
  // Returning a separate object leaves the destructor of `asked` to run at the
  // brace, which every call executes: the rule `createRange` states in
  // source/harness/family.cpp.
  return std::vector<std::vector<std::string>>(asked);
}

// The addresses one source holds in one pass once a column has joined it:
// the addresses the pass holds, plus every asked address outside them. A leaf
// two columns of the pass share therefore claims one counter, and so does an
// address one column asks twice (E-03 rule 2, research D-05).
[[nodiscard]] auto claimedAddresses(const std::vector<std::string>& held,
                                    const std::vector<std::string>& asked)
    -> std::vector<std::string>
{
  std::vector<std::string> claims(held);
  for (const auto& address : asked) {
    if (std::find(claims.begin(), claims.end(), address) == claims.end()) {
      claims.push_back(address);
    }
  }
  return std::vector<std::string>(claims);
}

// True when one column still fits one pass: after its addresses join what the
// pass has claimed, every source that publishes a limit stays within it
// (E-03 rule 3, FR-002). A source that publishes no limit costs nothing and
// rides the pass (E-03 rule 5, FR-005), and a pass already over a limit - the
// exclusive pass of E-03 rule 4 - answers false for every column, which is
// what keeps an over-limit column alone (FR-004).
[[nodiscard]] auto holdsColumn(
    const std::vector<std::vector<std::string>>& claims,
    const std::vector<std::vector<std::string>>& asked,
    const std::vector<std::optional<std::size_t>>& sourceLimits) -> bool
{
  for (std::size_t source = 0; source < sourceLimits.size(); ++source) {
    const auto limit = sourceLimits[source];
    if (limit.has_value()
        && claimedAddresses(claims[source], asked[source]).size() > *limit)
    {
      return false;
    }
  }
  return true;
}

}  // namespace

auto settleMethod(const MetricMethod requested,
                  const bool anyLimit,
                  const std::size_t metricColumns) noexcept -> MetricMethod
{
  // E-01 row 1, clarification Q2: a run that carries no metric column settles
  // nothing, whatever its caller asked for and whatever the host publishes.
  // The answer stays `AUTO` so the report fields keep their default and no
  // line prints: a method needs PMU events to collect.
  if (metricColumns == 0u) {
    return MetricMethod::AUTO;
  }
  // E-01 row 3, FR-005: no needed source publishes a limit, so no pass can
  // hold a bound on its own counters and the planner has nothing to decide.
  // Every request settles `MULTIPLEX`, a forced `passes` included.
  if (!anyLimit) {
    return MetricMethod::MULTIPLEX;
  }
  // E-01 rows 6 and 7, FR-017: an `auto` request settles `PASSES` whether the
  // columns fit one pass or need a split. The planner's own count tells those
  // two rows apart, and the print rule decides which of them prints.
  if (requested == MetricMethod::AUTO) {
    return MetricMethod::PASSES;
  }
  // E-01 rows 4 and 5, FR-024: a forced request settles itself, because a
  // caller that named a method chose it over what resolution would have read.
  return requested;
}

auto planPasses(const std::vector<std::vector<ColumnLeaves>>& columnLeaves,
                const std::vector<std::optional<std::size_t>>& sourceLimits)
    -> PassPlan
{
  // E-03 rule 7, FR-005: a run whose needed sources all answer no limit gets
  // no plan at all, the row of E-01 that settles `MULTIPLEX`. No pass exists
  // to measure anything, so no column carries a measuring pass either.
  const bool anyLimit = std::any_of(sourceLimits.begin(),
                                    sourceLimits.end(),
                                    [](const std::optional<std::size_t>& limit)
                                    { return limit.has_value(); });
  if (!anyLimit) {
    return PassPlan();
  }

  // The columns of each pass and its exclusivity flag, beside the addresses
  // each pass has claimed per source: the used counts of invariant I-1,
  // deduplicated by address within the pass (E-03 rule 2).
  std::vector<std::vector<std::size_t>> columns;
  std::vector<bool> exclusive;
  std::vector<std::vector<std::vector<std::string>>> claims;
  std::vector<std::size_t> measuringPass(columnLeaves.size(), 0);

  // E-03 rule 1: the columns arrive in request order - the metric columns in
  // the order `addMetric` attached them, then the user-counter columns in the
  // order they opened - and stay in it (invariant I-2).
  for (std::size_t column = 0; column < columnLeaves.size(); ++column) {
    const auto asked =
        askedAddresses(columnLeaves[column], sourceLimits.size());
    // E-03 rule 4, FR-004: a column over some source's published limit on its
    // own stands alone in a pass of its own, opened whole so the kernel
    // rotates it and the running ratio discloses (spec D-2). The cost of the
    // ask against an empty pass is the column's own deduplicated count.
    bool alone = false;
    for (std::size_t source = 0; source < sourceLimits.size(); ++source) {
      const auto limit = sourceLimits[source];
      if (limit.has_value()
          && claimedAddresses({}, asked[source]).size() > *limit)
      {
        alone = true;
      }
    }
    // E-03 rule 3, FR-002: first fit over the passes the run has already
    // opened, in the order they opened. Determinism follows: the answer is a
    // function of these two inputs alone (SC-02).
    std::size_t pass = 0;
    while (!alone && pass < columns.size()
           && !holdsColumn(claims[pass], asked, sourceLimits))
    {
      ++pass;
    }
    // E-03 rule 4, FR-004: the over-limit column opens a pass whatever the fit
    // search answered, and so does a column no open pass holds. The pass then
    // is the new one, the only pass that may stand over a limit.
    if (alone || pass == columns.size()) {
      pass = columns.size();
      columns.emplace_back();
      exclusive.push_back(alone);
      claims.emplace_back(sourceLimits.size());
    }
    for (std::size_t source = 0; source < sourceLimits.size(); ++source) {
      claims[pass][source] =
          claimedAddresses(claims[pass][source], asked[source]);
    }
    columns[pass].push_back(column);
    measuringPass[column] = pass;
  }

  // Invariant I-2: no pass stands empty, the columns of a pass keep request
  // order, and the passes follow the order of their first column.
  [[maybe_unused]] std::size_t previousFirst = 0;
  for (std::size_t pass = 0; pass < columns.size(); ++pass) {
    SG_ASSERT(!columns[pass].empty(),
              "no pass of a plan stands empty, because only a column opens a "
              "pass (FR-002)");
    for (std::size_t slot = 1; slot < columns[pass].size(); ++slot) {
      SG_ASSERT(columns[pass][slot - 1] < columns[pass][slot],
                "the columns of a pass keep request order (I-2)");
    }
    if (pass > 0) {
      SG_ASSERT(previousFirst < columns[pass].front(),
                "the passes follow the order of their first column (I-2)");
    }
    previousFirst = columns[pass].front();
    if (exclusive[pass]) {
      continue;
    }
    // Invariant I-1, FR-001: a pass that is not exclusive stays within every
    // published limit with the leaves it shares across its columns claimed
    // once; only the exclusive pass of rule 4 runs over a limit.
    for (std::size_t source = 0; source < sourceLimits.size(); ++source) {
      const auto limit = sourceLimits[source];
      if (limit.has_value()) {
        SG_ASSERT(claims[pass][source].size() <= *limit,
                  "a pass that is not exclusive stays within every published "
                  "limit (I-1, FR-001)");
      }
    }
  }

  // The separate return object keeps the destructors of the three locals
  // running at the closing brace, which every call executes: the rule
  // `createRange` states in source/harness/family.cpp.
  return PassPlan {
      std::move(columns), std::move(exclusive), std::move(measuringPass)};
}

auto mergePasses(const PassPlan& plan,
                 const std::vector<BenchmarkResult>& passResults)
    -> BenchmarkResult
{
  // I-5, FR-016: the merge reads the fold results the passes already hold and
  // consults no clock and no counter, so it publishes no value a pass did not
  // measure. Its contract is the plan's own shape.
  SG_ASSERT(!passResults.empty(),
            "a run that settled to the passes method folded its reference pass "
            "(FR-014)");
  for ([[maybe_unused]] const auto measuring : plan.measuringPass) {
    SG_ASSERT(
        measuring < passResults.size(),
        "every measuring pass of the plan names one fold result (FR-014)");
  }

  // I-4, FR-015: pass 0 is the reference pass (spec D-5), so its identity,
  // its labels, its point list, its time columns, its overhead floor, its
  // endpoint samples, its context disclosures, its chunk disclosure and its
  // primary percentile columns are what the merged result publishes (E-05).
  BenchmarkResult merged(passResults[0]);
  const std::size_t metricColumns = merged.metricLabels.size();
  for (std::size_t row = 0; row < merged.rows.size(); ++row) {
    auto& mergedRow = merged.rows[row];
    // FR-014: each metric column and each user-counter column carries the
    // values of its measuring pass at its own position. The plan indexes
    // columns in the same order the row holds them - the metric columns, then
    // the user-counter columns - so one index names the column throughout.
    for (std::size_t metric = 0; metric < mergedRow.metrics.size(); ++metric) {
      auto& tagged = mergedRow.metrics[metric];
      tagged =
          passResults[plan.measuringPass[metric]].rows[row].metrics[metric];
      // FR-019, E-08: the value carries the pass that measured it, so the
      // report can name the provenance inside the existing metric field
      // spelling. Every metric column of a merged run carries one, the
      // reference pass included.
      tagged.pass = plan.measuringPass[metric];
    }
    for (std::size_t counter = 0; counter < mergedRow.counters.size();
         ++counter)
    {
      mergedRow.counters[counter] =
          passResults[plan.measuringPass[metricColumns + counter]]
              .rows[row]
              .counters[counter];
    }
    // FR-015, E-05: the primary percentile column answers the reference pass
    // and rides the copy; every other percentile column answers the pass that
    // measured its column, because chunk times of another pass would state a
    // distribution the printed column never saw.
    for (std::size_t slot = 1; slot < mergedRow.percentiles.size(); ++slot) {
      mergedRow.percentiles[slot] =
          passResults[plan.measuringPass[slot - 1]].rows[row].percentiles[slot];
    }
    // FR-016: a value whose measuring pass folded a gap keeps
    // `Availability::GAP` with the number that pass folded, and a refusal
    // kind, its running ratio and its scaled disclosure ride unchanged - the
    // whole value of the measuring pass stands in the merged row, so nothing
    // here invents a number or drops a disclosure.
  }

  // FR-017, E-07: the three published disclosure fields ride the merged
  // result beside the values the merge placed. Only a settling of
  // `passes` reaches a merge, so the merge publishes that method; the count
  // is the passes whose folds stood merged.
  merged.metricMethod = MetricMethod::PASSES;
  merged.passCount = passResults.size();
  // E-06, D-6, clarification Q3: the spread reads the reference time per
  // iteration each pass folded - the primary time column of the pass's own
  // reference row, the figure the report shows for that pass. The precondition
  // has every pass fold the row count of pass 0, so a run with a row gives
  // every pass a reference row; a run that folded no row at all leaves no
  // figure to spread, and the published spread stays the empty answer FR-017
  // gives a run of fewer than two passes.
  if (!merged.rows.empty()) {
    std::vector<double> passTimes;
    passTimes.reserve(passResults.size());
    for (const BenchmarkResult& passResult : passResults) {
      const auto& referenceRow = passResult.rows.front();
      passTimes.push_back(merged.primaryIsCpu
                              ? referenceRow.cpuTimePerIterationNs
                              : referenceRow.timePerIterationNs);
    }
    merged.passSpread = passSpread(passTimes);
  }

  // A separate return object, for the reason `createRange` states in
  // source/harness/family.cpp.
  return BenchmarkResult(std::move(merged));
}

auto passSpread(const std::vector<double>& passTimes) -> std::optional<double>
{
  // FR-017, D-6: one pass has no spread across which to speak - one sample
  // carries no spread - so fewer than two times answer nothing.
  if (passTimes.size() < 2u) {
    return std::nullopt;
  }
  // Ordering a copy of the pass times - the pass count, a handful of values
  // - gives the minimum, the maximum and the median from one list, so no
  // iterator arithmetic stands between the question and its answer (research
  // D-08). The median of an even count is the lower of its two central
  // values (D-6), which the index `(n - 1) / 2` names for every count.
  std::vector<double> ordered(passTimes);
  std::sort(ordered.begin(), ordered.end());
  const double median = ordered[(ordered.size() - 1u) / 2u];
  // FR-017's edge: a zero median carries no relative spread, and the report
  // prints `spread=not available` in its place rather than divide (research
  // D-08). `-Wfloat-equal` bars the direct comparison, so the classification
  // answers - the reading `statisticsOf` gives the mean at
  // source/harness/runner.cpp:79.
  if (std::fpclassify(median) == FP_ZERO) {
    return std::nullopt;
  }
  // The percentage stands unrounded: the print is the one rounding, and the
  // warning of FR-018 reads the 5.0 predicate on this figure (research D-08).
  return (ordered.back() - ordered.front()) / median * 100.0;
}

}  // namespace sg::detail
