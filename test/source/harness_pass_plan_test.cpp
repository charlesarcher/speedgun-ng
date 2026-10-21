// ============================================================================
// The pass plan and merge tables (specs/024-multipass-collection T016 to T021;
// US1; FR-001 to FR-008, FR-014 to FR-016, FR-033; SC-02; research D-05, D-07)
// and the resolution table of the collection method (T029; US2; FR-023 to
// FR-025; research D-10).
//
// The split, the merge and the settling are pure functions, so every table
// here stands without a run, without a provider and without a clock:
// `planPasses` decides over published limits and column leaf tables alone and
// reads no counter (FR-008), `mergePasses` decides over the fold results of
// the passes alone (I-5), and `settleMethod` decides over the request, the
// limit facts and the metric-column count (E-01). The columns, the sources and
// the limits are the ones `test/source/detail/pass_fixture.hpp` scripts, so no
// figure below is invented here; where a table needs a column the fixture does
// not carry, the comment names the fixture entries it is composed from.
//
// `argv[1]` selects one scenario; with no argument the list at the bottom
// runs, and that list is what CTest runs, so a scenario absent from it never
// runs. Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <bit>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/gcov_flush.hpp"
#include "detail/pass_fixture.hpp"
#include "detail/pass_plan.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PASS PLAN TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// Exact double comparison through the bit pattern: the merge table states
// which pass produced a value, and a tolerance band has no place in that.
auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

// The fixture's sources, in the order a table's limit list carries them: the
// core cpu object whose limit the scenario asks for, the second source with
// its own scripted limit, the source scripted no limit at all, and the clock
// object (E-02, E-03 rules 5 and 6).
constexpr std::size_t kCpuSource = 0;
constexpr std::size_t kSecondSource = 1;
constexpr std::size_t kRideSource = 2;
constexpr std::size_t kFixtureSourceCount = 4;

// The column count of the spec's large-set edge (E-03's validation rule, the
// PC-3 oversize measurement): wider than any counter budget, so the plan has
// to place every column with no cap on the pass count (FR-001).
constexpr std::size_t kLargeColumnCount = 64;

// The repetitions of every fold result of the merge table: two rows give the
// merge per-row provenance to prove (research D-07).
constexpr std::size_t kMergeRows = 2;

// The iteration count of every row of the merge table. Every pass reuses the
// one calibration count (FR-010), so the passes agree on it and the merge has
// nothing to choose.
constexpr std::uint64_t kMergeIterations = 4;

// The value of a metric or counter slot a pass did not measure. A merge that
// reads the wrong pass puts this figure where the measuring pass's value
// belongs.
constexpr double kNotMeasured = -1.0;

// The running ratio an oversize pass discloses for a column the kernel
// rotated (FR-004). The merge has to let it ride (FR-016).
constexpr double kRotatedRatio = 0.5;

// The four value fields of a fold entry, so a slot number means one field
// only and a merge that reads the wrong field cannot coincide with the right
// answer.
constexpr std::size_t kMetricField = 0;
constexpr std::size_t kCounterField = 1;
constexpr std::size_t kPercentileField = 2;
constexpr std::size_t kRealTimeField = 3;
constexpr std::size_t kCpuTimeField = 4;
constexpr std::size_t kFloorField = 5;

// The object of fixture source `source`, spelled as the fixture spells it.
// The last source is the clock object, which publishes no limit.
[[nodiscard]] auto sourceObject(const std::size_t source) -> std::string_view
{
  if (source == kCpuSource) {
    return sg::test::kPassCpuObject;
  }
  if (source == kSecondSource) {
    return sg::test::kPassSecondObject;
  }
  if (source == kRideSource) {
    return sg::test::kPassRideObject;
  }
  return sg::test::kPassClockObject;
}

// The limit fixture source `source` publishes: the cpu object answers with
// the limit the scenario asks for, the second source with its own scripted
// count, and the ride source and the clock object with nothing, which is the
// source that cannot answer (E-02, FR-005).
[[nodiscard]] auto publishedLimit(const std::size_t source,
                                  const std::size_t cpuLimit)
    -> std::optional<std::size_t>
{
  if (source == kCpuSource) {
    return cpuLimit;
  }
  if (source == kSecondSource) {
    return sg::test::kSecondSourceLimit;
  }
  return std::nullopt;
}

// The sources one column list needs, in the order above, with the limit each
// publishes. A source no column names is left out, because the planner's
// no-plan rule answers the needed sources alone (E-03 rule 7). Index `i` of
// `limits` is the `source` index of every leaf of `objects[i]`.
struct SourceList
{
  std::vector<std::string_view> objects;
  std::vector<std::optional<std::size_t>> limits;
};

[[nodiscard]] auto sourceListOf(
    const std::vector<sg::test::PassColumn>& columns,
    const std::size_t cpuLimit) -> SourceList
{
  SourceList sources;
  for (std::size_t source = 0; source < kFixtureSourceCount; ++source) {
    const auto object = sourceObject(source);
    for (const auto& column : columns) {
      if (column.object == object) {
        sources.objects.push_back(object);
        sources.limits.push_back(publishedLimit(source, cpuLimit));
        break;
      }
    }
  }
  return sources;
}

// The index a source's leaves carry in the limit list of one table. Every
// caller names a source its list carries, because the list is built from the
// objects of the same columns.
[[nodiscard]] auto indexOfSource(const SourceList& sources,
                                 const std::string_view object) -> std::size_t
{
  std::size_t index = 0;
  while (index < sources.objects.size() && sources.objects[index] != object) {
    ++index;
  }
  check(index < sources.objects.size(),
        "every column names a source its table carries");
  return index;
}

// The canonical leaf address of one scripted event, spelled the way
// `counters::detail::LeafCore::address` spells it: "<object path>/<name>"
// (`counters_measurement.hpp:98`). The planner deduplicates by this string,
// the way `ExprCore::addLeaf` does, so a leaf two columns share has to share
// its spelling to claim one counter (E-03 rule 2, research D-05).
[[nodiscard]] auto leafAddress(const std::string_view object,
                               const std::string_view name) -> std::string
{
  return std::string(object) + "/" + std::string(name);
}

// The leaves one fixture column asks, grouped by source: a fixture column
// lives on one source, so it is one group (E-03 rule 2).
[[nodiscard]] auto leavesOf(const sg::test::PassColumn& column,
                            const SourceList& sources)
    -> std::vector<sg::detail::ColumnLeaves>
{
  sg::detail::ColumnLeaves group;
  group.source = indexOfSource(sources, column.object);
  for (const auto& event : column.events) {
    group.addresses.push_back(leafAddress(column.object, event));
  }
  return {group};
}

// The planner's column list, one entry per fixture column in request order
// (E-03 rule 1).
[[nodiscard]] auto leavesOf(const std::vector<sg::test::PassColumn>& columns,
                            const SourceList& sources)
    -> std::vector<std::vector<sg::detail::ColumnLeaves>>
{
  std::vector<std::vector<sg::detail::ColumnLeaves>> leaves;
  for (const auto& column : columns) {
    leaves.push_back(leavesOf(column, sources));
  }
  return leaves;
}

// The plan of one fixture column list under one cpu limit, over the source
// list those columns need.
[[nodiscard]] auto planOf(const std::vector<sg::test::PassColumn>& columns,
                          const std::size_t cpuLimit) -> sg::detail::PassPlan
{
  const SourceList sources = sourceListOf(columns, cpuLimit);
  return sg::detail::planPasses(leavesOf(columns, sources), sources.limits);
}

// The first `count` fixture columns in request order: `pair_a`, `pair_b`,
// `pair_c` and `pair_d` of two events each, then `wide` of five
// (`passColumns()`).
[[nodiscard]] auto fixtureColumns(const std::size_t count)
    -> std::vector<sg::test::PassColumn>
{
  auto all = sg::test::passColumns();
  check(count <= all.size(), "the table asks only for columns the fixture has");
  std::vector<sg::test::PassColumn> columns;
  for (std::size_t index = 0; index < count; ++index) {
    columns.push_back(all[index]);
  }
  return columns;
}

// A composed column on the object of `shared`: its first event followed by
// every event of `next`. It is how a table states that two columns share one
// leaf address (E-03 rule 2) and, with `next` on the object of `shared`, how
// a table states a column over a source's limit.
[[nodiscard]] auto sharingFirst(const sg::test::PassColumn& shared,
                                const sg::test::PassColumn& next)
    -> sg::test::PassColumn
{
  sg::test::PassColumn column;
  column.label = shared.label + "+" + next.label;
  column.object = shared.object;
  column.events.push_back(shared.events.front());
  for (const auto& event : next.events) {
    column.events.push_back(event);
  }
  return column;
}

// The two fixed time leaves as a column, so a planner that costed them
// against a published limit is caught here: they are not columns of a run
// (FR-007), they belong to the clock source, which publishes no limit, and
// they consume no programmable counter (E-03 rule 6).
[[nodiscard]] auto clockColumn() -> sg::test::PassColumn
{
  sg::test::PassColumn column;
  column.label = "time";
  column.object = sg::test::kPassClockObject;
  column.events.push_back(sg::test::kPassRealTimeLeaf);
  column.events.push_back(sg::test::kPassCpuTimeLeaf);
  return column;
}

// The leaf names of the large column set, in the fixture's spelling. The set
// needs an address of its own per leaf, because a repeated column that shared
// its addresses would cost nothing and the table would test nothing.
// `planPasses` reads no counter (FR-008), so an address with no script behind
// it is sound here. The names stand in a static so that the string views a
// `PassColumn` carries stay alive as long as the columns do.
[[nodiscard]] auto largeSetNames() -> const std::vector<std::string>&
{
  static const std::vector<std::string> kNames = []
  {
    std::vector<std::string> names;
    for (std::size_t index = 0; index < kLargeColumnCount; ++index) {
      names.push_back("pass_l" + std::to_string(index) + "0");
      names.push_back("pass_l" + std::to_string(index) + "1");
    }
    return names;
  }();
  return kNames;
}

// `kLargeColumnCount` columns of two PMU events each on the cpu object, the
// shape of the fixture's pairs at the width of the spec's large-set edge.
[[nodiscard]] auto largeColumnSet() -> std::vector<sg::test::PassColumn>
{
  const auto& names = largeSetNames();
  const auto pairs = fixtureColumns(4);
  std::vector<sg::test::PassColumn> columns;
  for (std::size_t index = 0; index < kLargeColumnCount; ++index) {
    sg::test::PassColumn column;
    column.label =
        pairs[index % pairs.size()].label + "_" + std::to_string(index);
    column.object = sg::test::kPassCpuObject;
    column.events.push_back(names[(2 * index)]);
    column.events.push_back(names[(2 * index) + 1]);
    columns.push_back(column);
  }
  return columns;
}

// The table of a plan: the pass lists, then one exclusivity flag per pass.
auto checkAssignment(const sg::detail::PassPlan& plan,
                     const std::vector<std::vector<std::size_t>>& expected,
                     const std::vector<bool>& expectedExclusive,
                     const char* what) -> void
{
  check(plan.columns.size() == expected.size(), what);
  check(plan.columns == expected, what);
  check(plan.exclusive == expectedExclusive, what);
}

// The measuring pass of every column, read off the pass lists, so a table
// states its assignment once.
[[nodiscard]] auto measuringPassOf(const std::vector<std::vector<std::size_t>>&
                                       passes) -> std::vector<std::size_t>
{
  std::vector<std::size_t> measuring;
  std::size_t columns = 0;
  for (const auto& pass : passes) {
    for (const auto column : pass) {
      columns = std::max(columns, column + 1);
    }
  }
  measuring.resize(columns, 0);
  for (std::size_t pass = 0; pass < passes.size(); ++pass) {
    for (const auto column : passes[pass]) {
      measuring[column] = pass;
    }
  }
  return measuring;
}

// The first-fit minimum pass count of a one-source column list under one
// limit, computed from the fixture's own costs: no pass that is not exclusive
// may exceed a source's limit (FR-001), so the count is at least the number of
// columns whose own cost exceeds it plus the ceiling of the rest over it; and
// no column may be split across passes (FR-003). The fixture's columns divide
// the limit exactly, so first fit reaches that bound (FR-002, SC-02).
[[nodiscard]] auto firstFitMinimum(
    const std::vector<sg::test::PassColumn>& columns,
    const std::size_t limit) -> std::size_t
{
  std::size_t exclusive = 0;
  std::size_t events = 0;
  for (const auto& column : columns) {
    if (column.events.size() > limit) {
      ++exclusive;
    } else {
      events += column.events.size();
    }
  }
  return exclusive + ((events + limit - 1) / limit);
}

// What every plan of this file owes whatever table it came from: every column
// stands in exactly one pass and `measuringPass` agrees with the pass lists
// (FR-002); the columns of a pass keep request order and the passes follow the
// order of their first column (I-2); a pass that is not exclusive asks no more
// of a source than that source publishes, with a shared address counted once
// per pass (I-1, E-03 rule 2).
auto checkPlanIsSound(const std::vector<sg::test::PassColumn>& columns,
                      const std::size_t cpuLimit,
                      const sg::detail::PassPlan& plan) -> void
{
  const SourceList sources = sourceListOf(columns, cpuLimit);
  check(plan.measuringPass.size() == columns.size(),
        "the plan answers one measuring pass per column (FR-002)");
  check(plan.exclusive.size() == plan.columns.size(),
        "the plan answers one exclusivity flag per pass (FR-003)");
  std::vector<bool> seen(columns.size(), false);
  std::size_t previousFirst = 0;
  for (std::size_t pass = 0; pass < plan.columns.size(); ++pass) {
    check(!plan.columns[pass].empty(), "no pass of a plan stands empty");
    for (std::size_t slot = 0; slot < plan.columns[pass].size(); ++slot) {
      const auto column = plan.columns[pass][slot];
      check(column < columns.size(), "a pass names a column of the list");
      check(!seen[column], "every column stands in exactly one pass (FR-002)");
      seen[column] = true;
      check(plan.measuringPass[column] == pass,
            "measuringPass names the pass that holds its column (E-08)");
      if (slot > 0) {
        check(plan.columns[pass][slot - 1] < column,
              "the columns of a pass keep request order (I-2)");
      }
    }
    if (pass > 0) {
      check(previousFirst < plan.columns[pass].front(),
            "the passes follow the order of their first column (I-2)");
    }
    previousFirst = plan.columns[pass].front();
    if (plan.exclusive[pass]) {
      continue;
    }
    for (std::size_t source = 0; source < sources.limits.size(); ++source) {
      const auto limit = sources.limits[source];
      if (!limit.has_value()) {
        continue;
      }
      std::vector<std::string> used;
      for (const auto column : plan.columns[pass]) {
        for (const auto& group : leavesOf(columns[column], sources)) {
          if (group.source != source) {
            continue;
          }
          for (const auto& address : group.addresses) {
            if (std::find(used.begin(), used.end(), address) == used.end()) {
              used.push_back(address);
            }
          }
        }
      }
      check(used.size() <= *limit,
            "a pass that is not exclusive stays within the limit it needs "
            "(I-1)");
    }
  }
  for (const auto column : seen) {
    check(column, "no column of the list is dropped from the plan (FR-002)");
  }
}

// T016, FR-001, FR-002, US1 scenario 1: the split table. The four fixture
// columns of two PMU events each, under the limit the development host
// publishes (CPUID leaf 0AH count 4), fill two passes of two columns each in
// request order.
auto splitTable() -> void
{
  const auto columns = fixtureColumns(4);
  const auto plan = planOf(columns, sg::test::kHostPassLimit);
  const std::vector<std::vector<std::size_t>> expected {{0, 1}, {2, 3}};
  checkAssignment(plan,
                  expected,
                  {false, false},
                  "a limit of four gives pass 0 columns 0 and 1 and pass 1 "
                  "columns 2 and 3, in request order (FR-001, FR-002)");
  check(plan.measuringPass == measuringPassOf(expected),
        "the measuring pass of each column names the pass that holds it "
        "(FR-014)");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, plan);
}

// T017, FR-002, SC-02, US1 scenario 4: the same column list, method and limit
// give the same assignment. The method is no input of the planner, which runs
// only under the `passes` method (E-01), so two calls over the same list and
// limit are the whole determinism claim at this seam.
auto determinism() -> void
{
  const auto columns = fixtureColumns(4);
  const auto first = planOf(columns, sg::test::kHostPassLimit);
  const auto second = planOf(columns, sg::test::kHostPassLimit);
  check(first.columns == second.columns,
        "two calls over the same list and limit give an identical assignment "
        "(FR-002, SC-02)");
  check(first.exclusive == second.exclusive,
        "two calls over the same list and limit flag the same passes "
        "exclusive (FR-004)");
  check(first.measuringPass == second.measuringPass,
        "two calls over the same list and limit measure each column in the "
        "same pass (FR-014)");
  // A plan of another limit between the two calls: the answer is a function
  // of its inputs alone, and the order the calls ran in changes nothing.
  const auto narrow = planOf(columns, sg::test::kNarrowPassLimit);
  const auto third = planOf(columns, sg::test::kHostPassLimit);
  check(third.columns == first.columns,
        "a plan between two calls leaves the assignment identical (SC-02)");
  check(narrow.columns != first.columns,
        "the two limits of the determinism pair do differ, so the checks "
        "above can fail");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, third);
}

// T017, FR-002, SC-02: the pass count equals the first-fit minimum on the
// fixture limits 1, 4 and 8.
auto minimumCount() -> void
{
  const auto columns = fixtureColumns(4);
  for (const auto limit : {
           sg::test::kNarrowPassLimit,
           sg::test::kHostPassLimit,
           sg::test::kWidePassLimit,
       })
  {
    const auto plan = planOf(columns, limit);
    check(plan.columns.size() == firstFitMinimum(columns, limit),
          "the pass count equals the first-fit minimum on the fixture limits "
          "1, 4 and 8 (FR-002, SC-02)");
    checkPlanIsSound(columns, limit, plan);
  }
}

// T018, FR-003, FR-004, US1 scenario 2: the fixture's five-event column over
// a limit of four stands alone in a pass flagged exclusive, and the four
// pairs keep the passes the split table gives them.
auto overLimit() -> void
{
  const auto columns = fixtureColumns(5);
  const auto plan = planOf(columns, sg::test::kHostPassLimit);
  const std::vector<std::vector<std::size_t>> expected {{0, 1}, {2, 3}, {4}};
  checkAssignment(plan,
                  expected,
                  {false, false, true},
                  "one column of five events on a limit of four stands alone "
                  "in a pass flagged exclusive (FR-004)");
  check(plan.measuringPass == measuringPassOf(expected),
        "the over-limit column is measured in its own pass and nowhere else "
        "(FR-003)");
  const auto withoutWide = planOf(fixtureColumns(4), sg::test::kHostPassLimit);
  check(withoutWide.columns.size() == 2,
        "the four pairs alone still fill two passes (FR-002)");
  check(plan.columns[0] == withoutWide.columns[0]
            && plan.columns[1] == withoutWide.columns[1],
        "the other columns keep their own passes beside the over-limit column "
        "(FR-003)");
  check(plan.exclusive[2],
        "the pass that runs oversubscribed is flagged exclusive (FR-004)");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, plan);
}

// T019, E-03: an empty column set produces no plan. Nothing is needed, so the
// source list is empty and the planner answers no pass at all.
auto emptyColumnSet() -> void
{
  const std::vector<sg::test::PassColumn> none;
  const auto plan = planOf(none, sg::test::kHostPassLimit);
  check(plan.columns.empty(), "an empty column set produces no plan (E-03)");
  check(plan.exclusive.empty(), "no plan means no exclusivity flag (FR-003)");
  check(plan.measuringPass.empty(),
        "no plan means no column with a measuring pass (FR-014)");
  checkPlanIsSound(none, sg::test::kHostPassLimit, plan);
}

// T019, FR-001, E-03 rule 4: a limit of one places one column per pass. Every
// fixture column costs two on its own, so each stands alone and every pass is
// exclusive.
auto limitOne() -> void
{
  const auto columns = fixtureColumns(5);
  const auto plan = planOf(columns, sg::test::kNarrowPassLimit);
  checkAssignment(plan,
                  {{0}, {1}, {2}, {3}, {4}},
                  {true, true, true, true, true},
                  "a limit of one places one column per pass, each flagged "
                  "exclusive (FR-001, FR-004)");
  check(plan.columns.size() == columns.size(),
        "one column per pass at the narrow fixture limit (FR-001)");
  checkPlanIsSound(columns, sg::test::kNarrowPassLimit, plan);
}

// T019, FR-001, the spec's large-set edge: the 64-column set places every
// column, with no limit on the pass count.
auto sixtyFourColumns() -> void
{
  const auto columns = largeColumnSet();
  const auto plan = planOf(columns, sg::test::kHostPassLimit);
  check(plan.columns.size() == firstFitMinimum(columns,
                                              sg::test::kHostPassLimit),
        "the 64-column set reaches the first-fit minimum pass count, which no "
        "cap may hold down (FR-001)");
  check(plan.exclusive == std::vector<bool>(plan.columns.size(), false),
        "no pass of the large set is exclusive, because every column fits on "
        "its own (FR-004)");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, plan);
}

// T019, E-03 rule 2, research D-05: two columns that share one leaf address
// claim that counter once inside their shared pass. Column 1 asks `pass_a0`
// again, which column 0 already opened, plus the two leaves of `pair_b`; the
// four distinct counters of pass 0 then leave no room for column 2, which
// would have fitted had the shared address cost nothing at all or cost twice.
auto sharedLeaf() -> void
{
  const auto pairs = fixtureColumns(4);
  std::vector<sg::test::PassColumn> columns;
  columns.push_back(pairs[0]);
  columns.push_back(sharingFirst(pairs[0], pairs[1]));
  columns.push_back(pairs[2]);
  const auto plan = planOf(columns, sg::test::kHostPassLimit);
  checkAssignment(plan,
                  {{0, 1}, {2}},
                  {false, false},
                  "two columns sharing one leaf address share the pass the "
                  "counter is claimed in (E-03 rule 2)");
  check(plan.columns[1].size() == 1,
        "the shared claim fills the first pass at four distinct counters, so "
        "the next column opens the second one (FR-001)");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, plan);
}

// T020, FR-005, E-03 rule 5, the two-provider edge: leaves whose source
// publishes no limit are costed against nothing. The ride column of
// `passRideColumns()` asks two leaves of a source scripted no limit, so it
// joins a pass whose cpu counters already fill the limit, and the same leaf
// pair asked again after `pair_c` still joins the first pass: the cost
// decides the placement and the position of the column has no part in it.
// Which passes' plan compiles carry a no-limit leaf is a rule of the runner
// (FR-007); the plan itself holds no field for it.
auto noLimitSource() -> void
{
  const auto pairs = fixtureColumns(4);
  const auto ride = sg::test::passRideColumns();
  auto ridesAgain = ride[1];
  ridesAgain.label = "ride_again";
  std::vector<sg::test::PassColumn> columns;
  columns.push_back(pairs[0]);
  columns.push_back(ride[1]);
  columns.push_back(pairs[1]);
  columns.push_back(pairs[2]);
  columns.push_back(ridesAgain);
  columns.push_back(pairs[3]);
  const auto plan = planOf(columns, sg::test::kHostPassLimit);
  checkAssignment(plan,
                  {{0, 1, 2, 4}, {3, 5}},
                  {false, false},
                  "a column whose source publishes no limit is costed against "
                  "nothing and joins a pass that is already full (FR-005, "
                  "E-03 rule 5)");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, plan);

  // E-03 rule 7: a column set whose needed sources all publish no limit gives
  // no plan at all, the row of E-01 that settles `multiplex`.
  const std::vector<sg::test::PassColumn> onlyUnlimited {
      ride[1],
      clockColumn(),
  };
  const auto none = planOf(onlyUnlimited, sg::test::kHostPassLimit);
  check(none.columns.empty(),
        "a set whose needed sources publish no limit produces no plan "
        "(FR-005, E-03 rule 7)");
}

// T020, FR-007, E-03 rule 6: the two fixed machine clock leaves ride every
// pass and count against none, because they consume no programmable counter.
auto clockLeaves() -> void
{
  const auto pairs = fixtureColumns(4);

  // At the narrow fixture limit every cpu column stands alone; the clock
  // column costs nothing, so it is not one of the exclusive passes and no
  // pass exists for it alone.
  std::vector<sg::test::PassColumn> narrow;
  narrow.push_back(clockColumn());
  narrow.push_back(pairs[0]);
  narrow.push_back(pairs[1]);
  const auto narrowPlan = planOf(narrow, sg::test::kNarrowPassLimit);
  checkAssignment(narrowPlan,
                  {{0}, {1}, {2}},
                  {false, true, true},
                  "the two fixed time leaves count against no pass, not even "
                  "at a limit of one (FR-007)");

  // At the host limit the clock column rides the pass the cpu pairs fill.
  std::vector<sg::test::PassColumn> wide;
  wide.push_back(clockColumn());
  for (std::size_t index = 0; index < 4; ++index) {
    wide.push_back(pairs[index]);
  }
  const auto widePlan = planOf(wide, sg::test::kHostPassLimit);
  checkAssignment(widePlan,
                  {{0, 1, 2}, {3, 4}},
                  {false, false},
                  "the time leaves ride a full pass without costing a counter "
                  "(FR-007, E-03 rule 6)");
  checkPlanIsSound(wide, sg::test::kHostPassLimit, widePlan);
}

// T020, FR-007: a PMU `cycles` event a metric names counts in the pass that
// carries it. The leaf lives on the cpu source, which publishes the limit, so
// it takes a counter like any other PMU leaf and the pairs pack around it.
// `cycles` is the leaf name the counters suites script for a core cycle leaf
// (`counters_fake_test.cpp:126`); `planPasses` reads no counter (FR-008), so
// the address is the whole fact here.
auto cyclesCounts() -> void
{
  sg::test::PassColumn cycles;
  cycles.label = "cycles";
  cycles.object = sg::test::kPassCpuObject;
  cycles.events.emplace_back("cycles");
  const auto pairs = fixtureColumns(4);
  std::vector<sg::test::PassColumn> columns;
  columns.push_back(cycles);
  for (std::size_t index = 0; index < 4; ++index) {
    columns.push_back(pairs[index]);
  }
  const auto plan = planOf(columns, sg::test::kHostPassLimit);
  checkAssignment(
      plan,
      {{0, 1}, {2, 3}, {4}},
      {false, false, false},
      "a PMU cycles event counts in the pass that carries it " "(FR-007)");
  check(!plan.exclusive[0],
        "the pass that carries the cycles leaf is not exclusive: one counter "
        "plus one pair fits the limit (FR-001)");
  checkPlanIsSound(columns, sg::test::kHostPassLimit, plan);
}

// T020, E-02, E-03 rule 3, the two-provider edge: each source's published
// limit is answered for its own leaves, never summed with another source's.
auto twoProviderLimits() -> void
{
  const auto pairs = fixtureColumns(4);
  const auto ride = sg::test::passRideColumns();

  // Four cpu counters and two of the second source stand open in one pass at
  // the limits both publish. A planner that summed every leaf against the cpu
  // limit of four would split this set in two.
  std::vector<sg::test::PassColumn> together;
  together.push_back(pairs[0]);
  together.push_back(ride[0]);
  together.push_back(pairs[1]);
  const auto onePass = planOf(together, sg::test::kHostPassLimit);
  checkAssignment(onePass,
                  {{0, 1, 2}},
                  {false},
                  "two sources at their own limits hold their columns in one "
                  "pass, instead of a summed limit (E-02, FR-001)");
  checkPlanIsSound(together, sg::test::kHostPassLimit, onePass);

  // A column over the second source's limit of two alone stands alone and
  // exclusive, although the cpu limit of four would have held it: the fit
  // test is per source (FR-001, FR-004). The column is `second`'s `pass_s0`
  // followed by the two leaves of `pair_a`, all on the second source.
  std::vector<sg::test::PassColumn> apart;
  apart.push_back(pairs[0]);
  apart.push_back(sharingFirst(ride[0], pairs[0]));
  const auto perSource = planOf(apart, sg::test::kHostPassLimit);
  checkAssignment(perSource,
                  {{0}, {1}},
                  {false, true},
                  "a column over one source's limit stands alone and "
                  "exclusive though another source has room (FR-004)");
  checkPlanIsSound(apart, sg::test::kHostPassLimit, perSource);
}

// The column shape of the merge table: the metric columns in request order,
// then the user-counter columns in open order (E-03 rule 1), with the plan
// over them.
struct MergeTable
{
  std::vector<sg::test::PassColumn> columns;
  std::size_t metricColumns = 0;
  sg::detail::PassPlan plan;
};

// A user-counter column: one leaf on the cpu object, whose address is the
// column's label, the rule of the runner's own counter columns
// (`runner.cpp:1156`). The fixture scripts no user-counter leaf and the merge
// reads no counter (I-5), so an address with no script behind it is sound
// here. `name` has to be a literal: the column carries it as a view.
[[nodiscard]] auto counterColumn(const std::string_view name)
    -> sg::test::PassColumn
{
  sg::test::PassColumn column;
  column.object = sg::test::kPassCpuObject;
  column.label = leafAddress(column.object, name);
  column.events.push_back(name);
  return column;
}

// The merge table: the four fixture pairs as the metric columns and two
// user-counter columns after them, under the limit the host publishes. First
// fit gives pass 0 the first two pairs, pass 1 the last two and pass 2 both
// counters, so no column of the table is measured by the pass it would sit in
// by default.
[[nodiscard]] auto mergeTable() -> MergeTable
{
  MergeTable table;
  table.columns = fixtureColumns(4);
  table.metricColumns = table.columns.size();
  table.columns.push_back(counterColumn("pass_u0"));
  table.columns.push_back(counterColumn("pass_u1"));
  table.plan = planOf(table.columns, sg::test::kHostPassLimit);
  return table;
}

// The value of one fold entry, distinct per pass, row, field and column, so
// every provenance claim of the merge table reads a figure only one of them
// could have produced.
[[nodiscard]] auto foldValue(const std::size_t pass,
                             const std::size_t row,
                             const std::size_t field,
                             const std::size_t slot) -> double
{
  return (100000.0 * static_cast<double>(pass))
      + (1000.0 * static_cast<double>(row))
      + (100.0 * static_cast<double>(field)) + static_cast<double>(slot);
}

// True when pass `pass` measures the run's column `column`.
[[nodiscard]] auto measuredBy(const MergeTable& table,
                              const std::size_t pass,
                              const std::size_t column) -> bool
{
  return table.plan.measuringPass[column] == pass;
}

// One row of one pass's fold result, in the shape `mergePasses` takes: every
// column of the run holds a slot at its own position of every pass, because
// the signature carries no metric/counter split and position is the only
// provenance a merge can read (research D-07, E-05). A slot this pass did not
// measure holds `kNotMeasured` and answers no percentile.
[[nodiscard]] auto foldRow(const MergeTable& table,
                           const std::size_t pass,
                           const std::size_t row) -> sg::ResultRow
{
  const std::size_t counters = table.columns.size() - table.metricColumns;
  sg::ResultRow result;
  result.kind = sg::RowKind::REPETITION;
  result.iterations = kMergeIterations;
  // The time columns, the floor and the samples stand per pass and differ
  // between passes, so the pass-0 rule of FR-015 is observable.
  result.timePerIterationNs = foldValue(pass, row, kRealTimeField, 0);
  result.cpuTimePerIterationNs = foldValue(pass, row, kCpuTimeField, 0);
  result.overheadFloorNs = foldValue(pass, row, kFloorField, 0);
  result.endpointSamples = 2 + pass;
  result.chunkCount = 4 + pass;
  result.chunkIterations = 2 + pass;
  for (std::size_t column = 0; column < table.metricColumns; ++column) {
    sg::MetricValue value;
    const auto index = measuredBy(table, pass, column)
        ? foldValue(pass, row, kMetricField, column)
        : kNotMeasured;
    value.value = index;
    if (measuredBy(table, pass, column)) {
      // The gap edge (FR-016): column 3 is measured by the pass whose window
      // folded a gap, which invents no number.
      if (column == table.metricColumns - 1) {
        value.availability = sg::counters::Availability::GAP;
        value.value = 0.0;
      }
      // The refusal edge (FR-016): column 2 keeps its kind, its rotated ratio
      // and its scaled disclosure.
      if (column == 2) {
        value.availability = sg::counters::Availability::ABSENT;
        value.runningRatio = kRotatedRatio;
        value.scaled = true;
      }
    }
    result.metrics.push_back(value);
  }
  for (std::size_t counter = 0; counter < counters; ++counter) {
    result.counters.push_back(
        measuredBy(table, pass, table.metricColumns + counter)
            ? foldValue(pass, row, kCounterField, counter)
            : kNotMeasured);
  }
  // The percentile grid follows the print order of the row: the primary time,
  // the time column the primary is not, then every metric, then every counter
  // (SC-001). Both time columns belong to every pass; a metric or counter slot
  // answers only in the pass that measured its column, so the slot past the
  // second answers the column two below it.
  result.percentiles.resize(2 + table.columns.size());
  for (std::size_t slot = 0; slot < 2 + table.columns.size(); ++slot) {
    const auto owned = slot < 2 || measuredBy(table, pass, slot - 2);
    auto& point = result.percentiles[slot];
    point.resize(1);
    point[0].available = owned;
    point[0].value = owned ? foldValue(pass, row, kPercentileField, slot) : 0.0;
  }
  return result;
}

// One pass's fold result: the rows of the run, the run's labels and point
// list, and the context disclosures of the pass. The passes of the table carry
// different context records so that the pass-0 rule of E-05 is observable; a
// real run's passes share their contexts (FR-011).
[[nodiscard]] auto foldOfPass(const MergeTable& table,
                              const std::size_t pass) -> sg::BenchmarkResult
{
  sg::BenchmarkResult result;
  result.name = "bmPassMerge";
  result.suite = "bmPassMerge";
  result.caseName = "bmPassMerge";
  for (std::size_t column = 0; column < table.columns.size(); ++column) {
    if (column < table.metricColumns) {
      result.metricLabels.push_back(table.columns[column].label);
    } else {
      result.counterLabels.push_back(table.columns[column].label);
    }
  }
  result.percentilePoints.push_back(50.0);
  for (std::size_t context = 0; context <= pass; ++context) {
    sg::ContextDisclosure disclosure;
    disclosure.placement = std::to_string(context);
    result.contexts.push_back(disclosure);
  }
  for (std::size_t row = 0; row < kMergeRows; ++row) {
    result.rows.push_back(foldRow(table, pass, row));
  }
  return result;
}

// T058, FR-017, E-07: the two arms of the merge's spread computation. A merge
// of folds that hold no row has no reference time to spread, so it publishes
// the same empty answer a run of fewer than two passes publishes. The spread
// reads the time column the run names primary, and `primaryIsCpu` defaults to
// true, so the wall-clock arm stands nowhere else in this file.
auto mergeSpreadField() -> void
{
  const MergeTable table = mergeTable();
  std::vector<sg::BenchmarkResult> rowless;
  for (std::size_t pass = 0; pass < table.plan.columns.size(); ++pass) {
    sg::BenchmarkResult fold = foldOfPass(table, pass);
    fold.rows.clear();
    rowless.push_back(fold);
  }
  const sg::BenchmarkResult mergedRowless =
      sg::detail::mergePasses(table.plan, rowless);
  check(mergedRowless.rows.empty(),
        "a merge of folds that hold no row publishes no row (FR-014)");
  check(!mergedRowless.passSpread.has_value(),
        "a merge with no reference time publishes no spread (FR-017, E-07)");
  check(mergedRowless.passCount == table.plan.columns.size()
            && mergedRowless.metricMethod == sg::MetricMethod::PASSES,
        "the merge still publishes the count and the method it settled to "
        "(FR-017)");

  std::vector<sg::BenchmarkResult> wall;
  for (std::size_t pass = 0; pass < table.plan.columns.size(); ++pass) {
    wall.push_back(foldOfPass(table, pass));
  }
  wall.front().primaryIsCpu = false;
  const sg::BenchmarkResult mergedWall =
      sg::detail::mergePasses(table.plan, wall);
  std::vector<double> wallTimes;
  std::vector<double> cpuTimes;
  for (const sg::BenchmarkResult& fold : wall) {
    wallTimes.push_back(fold.rows[0].timePerIterationNs);
    cpuTimes.push_back(fold.rows[0].cpuTimePerIterationNs);
  }
  check(!mergedWall.primaryIsCpu,
        "the merged result keeps the primary column of the reference pass "
        "(FR-015)");
  check(mergedWall.passSpread.has_value()
            && sameDouble(*mergedWall.passSpread,
                          *sg::detail::passSpread(wallTimes)),
        "the published spread is the spread of the wall times the report "
        "prints, the column this run names primary (FR-017, E-07)");
  check(!sameDouble(*mergedWall.passSpread,
                    *sg::detail::passSpread(cpuTimes)),
        "the wall times differ from the cpu times of the same folds, so a "
        "merge that read the other column cannot pass (E-07)");
}

// T021, FR-014, FR-015, FR-016, E-05: the merge table. Every metric column
// and every user-counter column takes its measuring pass's values at its own
// position; the time columns, the overhead floor, the endpoint samples, the
// context disclosures, the chunk disclosure and the primary percentile column
// come from pass 0; a metric percentile comes from that metric's own pass; a
// column whose measuring pass folded a gap keeps `Availability::GAP`.
auto mergeProvenance() -> void
{
  const MergeTable table = mergeTable();
  check(table.plan.columns.size() == 3,
        "the merge table needs three passes so that a counter column is "
        "measured away from pass 0 (FR-014)");
  std::vector<sg::BenchmarkResult> passes;
  for (std::size_t pass = 0; pass < table.plan.columns.size(); ++pass) {
    passes.push_back(foldOfPass(table, pass));
  }
  // The table can fail: pass 1 and pass 2 state different time columns, so a
  // merge that read the wrong pass cannot coincide with the answer.
  check(!sameDouble(passes[0].rows[0].timePerIterationNs,
                    passes[1].rows[0].timePerIterationNs),
        "the passes of the merge table differ in the fields FR-015 gives to "
        "pass 0");
  const sg::BenchmarkResult merged =
      sg::detail::mergePasses(table.plan, passes);
  check(merged.name == passes[0].name,
        "the merged result keeps the identity of the run (FR-014)");
  check(merged.metricLabels == passes[0].metricLabels,
        "the merged result keeps every metric label at its own position "
        "(FR-014)");
  check(merged.counterLabels == passes[0].counterLabels,
        "the merged result keeps every user-counter label at its own position "
        "(FR-014)");
  check(merged.percentilePoints == passes[0].percentilePoints,
        "the merged result reports the point list of the run (FR-015)");
  check(merged.rows.size() == kMergeRows,
        "the merged result carries one row per repetition (FR-014)");
  const std::size_t counters = table.columns.size() - table.metricColumns;
  for (std::size_t row = 0; row < kMergeRows; ++row) {
    const auto& mergedRow = merged.rows[row];
    check(mergedRow.metrics.size() == table.metricColumns,
          "the merged row carries every metric column at its own position "
          "(FR-014)");
    check(mergedRow.counters.size() == counters,
          "the merged row carries every user-counter column at its own "
          "position (FR-014)");
    for (std::size_t column = 0; column < table.metricColumns; ++column) {
      const auto pass = table.plan.measuringPass[column];
      check(sameDouble(mergedRow.metrics[column].value,
                       passes[pass].rows[row].metrics[column].value),
            "each metric column takes the value of its measuring pass at its "
            "own position (FR-014)");
      check(mergedRow.metrics[column].availability
                == passes[pass].rows[row].metrics[column].availability,
            "each metric column keeps the availability of its measuring pass "
            "(FR-016)");
      check(
          sameDouble(mergedRow.metrics[column].runningRatio,
                     passes[pass].rows[row].metrics[column].runningRatio),
          "the running ratio of the measuring pass rides unchanged " "(FR-"
                                                                     "016)");
      // The grid of the row stands two slots before its first metric column:
      // the primary time and the time column the primary is not (SC-001).
      check(sameDouble(mergedRow.percentiles[column + 2][0].value,
                       passes[pass].rows[row].percentiles[column + 2][0].value),
            "a metric percentile comes from that metric's own pass "
            "(FR-015, E-05)");
      check(mergedRow.percentiles[column + 2][0].available,
            "the percentile of a measured metric column answers (FR-015)");
      // The metric columns of this table are measured by the first two passes,
      // so the other one of them states nothing in the slot.
      const auto otherPass = pass == 0 ? std::size_t {1} : std::size_t {0};
      check(!passes[otherPass].rows[row].percentiles[column + 2][0].available,
            "a pass that did not measure the column answers nothing in its "
            "slot, so the claim above reads a figure one pass holds alone "
            "(FR-015)");
    }
    for (std::size_t counter = 0; counter < counters; ++counter) {
      const auto pass = table.plan.measuringPass[table.metricColumns + counter];
      const std::size_t slot = table.metricColumns + counter + 2;
      check(sameDouble(mergedRow.counters[counter],
                       passes[pass].rows[row].counters[counter]),
            "each user-counter column takes the value of its measuring pass at "
            "its own position (FR-014)");
      check(sameDouble(mergedRow.percentiles[slot][0].value,
                       passes[pass].rows[row].percentiles[slot][0].value),
            "a user-counter percentile comes from its measuring pass, the slot "
            "past the metric slots of the grid (FR-015, E-05)");
      check(pass != 0,
            "the table measures the counter columns away from pass 0, so the "
            "claim above cannot pass by default");
      check(!passes[0].rows[row].percentiles[slot][0].available,
            "the reference pass folded nothing for a counter column it did not "
            "measure, so the slot above cannot read the copy (FR-015)");
    }
    // FR-015: the reference pass is pass 0 (I-4).
    check(sameDouble(mergedRow.timePerIterationNs,
                     passes[0].rows[row].timePerIterationNs),
          "the real time column comes from pass 0 (FR-015)");
    check(sameDouble(mergedRow.cpuTimePerIterationNs,
                     passes[0].rows[row].cpuTimePerIterationNs),
          "the cpu time column comes from pass 0 (FR-015)");
    check(sameDouble(mergedRow.overheadFloorNs,
                     passes[0].rows[row].overheadFloorNs),
          "the overhead floor comes from pass 0 (FR-015)");
    check(mergedRow.endpointSamples == passes[0].rows[row].endpointSamples,
          "the endpoint samples come from pass 0 (FR-015)");
    check(
        mergedRow.chunkCount == passes[0].rows[row].chunkCount
            && mergedRow.chunkIterations == passes[0].rows[row].chunkIterations,
        "the chunk disclosure comes from pass 0 (FR-015)");
    check(sameDouble(mergedRow.percentiles[0][0].value,
                     passes[0].rows[row].percentiles[0][0].value),
          "the primary percentile column comes from pass 0 (FR-015)");
    // SC-001: the slot beside the primary time is the time column the primary
    // is not, and it is a time column: the plan splits columns of the run and
    // never a time column, so no plan can move this slot away from the
    // reference pass.
    check(sameDouble(mergedRow.percentiles[1][0].value,
                     passes[0].rows[row].percentiles[1][0].value),
          "the secondary time percentile comes from pass 0 whatever the plan "
          "says (FR-015, E-05)");
    check(!sameDouble(passes[1].rows[row].percentiles[1][0].value,
                      passes[0].rows[row].percentiles[1][0].value),
          "the passes of the table state different secondary time "
          "percentiles, so the claim above cannot read any pass but pass 0");
  }
  check(merged.contexts.size() == passes[0].contexts.size(),
        "the context disclosures come from pass 0 (E-05, research D-06)");
  for (std::size_t context = 0; context < merged.contexts.size(); ++context) {
    check(merged.contexts[context].placement
              == passes[0].contexts[context].placement,
          "every context disclosure names pass 0's record (FR-015)");
  }
  // FR-016, the gap edge: the column whose measuring pass folded a gap keeps
  // the gap, with no invented number.
  const auto gapColumn = table.metricColumns - 1;
  const auto gapPass = table.plan.measuringPass[gapColumn];
  check(passes[gapPass].rows[0].metrics[gapColumn].availability
            == sg::counters::Availability::GAP,
        "the table folds a gap for the column under the gap edge (FR-016)");
  for (std::size_t row = 0; row < kMergeRows; ++row) {
    check(merged.rows[row].metrics[gapColumn].availability
              == sg::counters::Availability::GAP,
          "a column whose pass folded a gap keeps Availability::GAP (FR-016)");
    check(sameDouble(merged.rows[row].metrics[gapColumn].value,
                     passes[gapPass].rows[row].metrics[gapColumn].value),
          "the gap column carries the pass's own value, with no number "
          "invented (FR-016, SC-05)");
  }
  // FR-016, the refusal edge: the kind, the ratio and the scaled disclosure
  // of a refused column ride unchanged.
  const auto refusedPass = table.plan.measuringPass[2];
  check(merged.rows[0].metrics[2].availability
            == passes[refusedPass].rows[0].metrics[2].availability,
        "a refusal kind rides unchanged (FR-016)");
  check(merged.rows[0].metrics[2].scaled
            == passes[refusedPass].rows[0].metrics[2].scaled,
        "the scaled disclosure of a refused column rides unchanged (FR-016)");
}

// ============================================================================
// The resolution table of data-model E-01 (T029, US2).
// ============================================================================

// The pass count one settling discloses (E-07, contracts/pass-disclosure.md):
// a settling of nothing discloses none, the `multiplex` settling runs the one
// plan holding every PMU event of the run, and a `passes` settling discloses
// the passes of its plan (FR-017, FR-020).
[[nodiscard]] auto disclosedPassCount(const sg::MetricMethod settled,
                                      const sg::detail::PassPlan& plan)
    -> std::size_t
{
  if (settled == sg::MetricMethod::AUTO) {
    return 0;
  }
  if (settled == sg::MetricMethod::MULTIPLEX) {
    return 1;
  }
  return plan.columns.size();
}

// The print rule of E-07, FR-020, FR-021 and research D-10, which
// `printPassDisclosure` applies from the settled method, the pass count and
// the forced record. The forced bit is caller state that the settled method
// cannot recover, so it is an input of its own: a forced `passes` run that
// fits in one pass prints the line and the identical `auto` run prints
// nothing (research D-10, SC-03). The printed bytes are golden-tested in
// `harness_pass_report_test.cpp`; this table pins what the rule reads.
[[nodiscard]] auto printsPassLine(const std::size_t metricColumns,
                                  const sg::MetricMethod settled,
                                  const std::size_t passCount,
                                  const bool forced) -> bool
{
  if (metricColumns == 0 || settled == sg::MetricMethod::AUTO) {
    return false;
  }
  return settled == sg::MetricMethod::MULTIPLEX || passCount > 1 || forced;
}

// The columns of the host that publishes no limit: one pair on the source
// scripted without a limit, beside the two fixed time leaves, so every source
// the run needs answers nothing (E-03 rules 5 and 7, spec D-11). The time
// leaves ride the run and are columns of no metric, which is why the row that
// carries them counts one metric column.
[[nodiscard]] auto unlimitedColumns() -> std::vector<sg::test::PassColumn>
{
  std::vector<sg::test::PassColumn> columns;
  columns.push_back(sg::test::passRideColumns()[1]);
  columns.push_back(clockColumn());
  // A `return columns;` would make the local the return object itself and
  // leave gcov an unexecutable record at the closing brace, the rule
  // `createRange` states in source/harness/family.cpp.
  return std::vector<sg::test::PassColumn>(columns);
}

// One row of the resolution table of data-model E-01: the request that won the
// priority of research D-09, the forced record, what the run carries, and the
// three figures the row pins. `plannedPasses` is what `planPasses` answers for
// the row's columns whenever it is called, a figure the run may leave unused.
struct ResolutionRow
{
  std::string_view name {};
  sg::MetricMethod requested = sg::MetricMethod::AUTO;
  bool forced = false;
  std::size_t metricColumns = 0;
  std::vector<sg::test::PassColumn> columns;
  sg::MetricMethod settled = sg::MetricMethod::AUTO;
  bool plannerRuns = false;
  std::size_t plannedPasses = 0;
  std::size_t passes = 0;
  bool printsLine = false;
};

// The rows of the table, in data-model order. The listing run and the catalog
// run end before anything settles, so the command-line suite drives that row
// (T031).
[[nodiscard]] auto resolutionRows() -> const std::vector<ResolutionRow>&
{
  static const std::vector<ResolutionRow> kRows {
      ResolutionRow {
          .name = "a host with no limit",
          .requested = sg::MetricMethod::AUTO,
          .forced = false,
          .metricColumns = 1,
          .columns = unlimitedColumns(),
          .settled = sg::MetricMethod::MULTIPLEX,
          .plannerRuns = false,
          .plannedPasses = 0,
          .passes = 1,
          .printsLine = true,
      },
      ResolutionRow {
          .name = "a forced multiplex",
          .requested = sg::MetricMethod::MULTIPLEX,
          .forced = true,
          .metricColumns = 4,
          .columns = fixtureColumns(4),
          .settled = sg::MetricMethod::MULTIPLEX,
          .plannerRuns = false,
          .plannedPasses = 2,
          .passes = 1,
          .printsLine = true,
      },
      ResolutionRow {
          .name = "a forced passes on a fitting set",
          .requested = sg::MetricMethod::PASSES,
          .forced = true,
          .metricColumns = 2,
          .columns = fixtureColumns(2),
          .settled = sg::MetricMethod::PASSES,
          .plannerRuns = true,
          .plannedPasses = 1,
          .passes = 1,
          .printsLine = true,
      },
      ResolutionRow {
          .name = "an auto set that fits",
          .requested = sg::MetricMethod::AUTO,
          .forced = false,
          .metricColumns = 2,
          .columns = fixtureColumns(2),
          .settled = sg::MetricMethod::PASSES,
          .plannerRuns = true,
          .plannedPasses = 1,
          .passes = 1,
          .printsLine = false,
      },
      ResolutionRow {
          .name = "an auto set that does not fit",
          .requested = sg::MetricMethod::AUTO,
          .forced = false,
          .metricColumns = 4,
          .columns = fixtureColumns(4),
          .settled = sg::MetricMethod::PASSES,
          .plannerRuns = true,
          .plannedPasses = 2,
          .passes = 2,
          .printsLine = true,
      },
  };
  return kRows;
}

// One check of one row: the row names itself in the failure, so a red run
// states which row of E-01 broke.
auto checkRow(const ResolutionRow& row,
              const bool condition,
              const std::string_view what) -> void
{
  if (condition) {
    return;
  }
  const std::string message = std::string(row.name) + ": " + std::string(what);
  fail(message.c_str());
}

// T029, FR-023, FR-024, FR-025, E-01, research D-10: the resolution table.
// Every row states the settled method, whether the planner runs, the pass
// count the report discloses and whether the console line prints. The settling
// is `settleMethod`'s answer over the row's request, the limit facts its
// columns read and its metric-column count; the plan is `planPasses`'s answer
// over the same columns; the console column follows the print rule from those
// two answers and the forced record.
auto resolutionTable() -> void
{
  for (const auto& row : resolutionRows()) {
    const SourceList sources =
        sourceListOf(row.columns, sg::test::kHostPassLimit);
    // True when some source the row's columns need publishes a limit (E-02).
    // The host is read off the fixture through the same objects the runner
    // resolves its leaves against (`runner.cpp:1296`).
    bool anyLimit = false;
    for (const auto& limit : sources.limits) {
      anyLimit = anyLimit || limit.has_value();
    }
    const auto settled =
        sg::detail::settleMethod(row.requested, anyLimit, row.metricColumns);
    checkRow(row,
             settled == row.settled,
             "the row settles the method E-01 names (FR-023, FR-024, FR-025)");
    checkRow(row,
             (settled == sg::MetricMethod::PASSES) == row.plannerRuns,
             "the planner runs on the rows whose settling names passes "
             "(FR-001, FR-024)");
    const auto plan =
        sg::detail::planPasses(leavesOf(row.columns, sources), sources.limits);
    checkRow(row,
             plan.columns.size() == row.plannedPasses,
             "the planner's answer for the row's columns is the row's "
             "(E-03, FR-001)");
    const auto passes = disclosedPassCount(settled, plan);
    checkRow(row,
             passes == row.passes,
             "the run discloses the pass count of its row (FR-017, E-07)");
    checkRow(row,
             printsPassLine(
                 row.metricColumns, settled, passes, row.forced)
                 == row.printsLine,
             "the console line follows the print rule for the row (FR-020, "
             "FR-021, research D-10)");
  }
  // The table can fail: the forced `multiplex` row leaves a two-pass split of
  // its own columns unclaimed, so its count of one answers the settling and
  // the planner answer cannot stand in for it; the two one-pass `passes` rows
  // differ in the forced record and in the console line, so the print rule is
  // the only thing that tells them apart.
  const auto& rows = resolutionRows();
  check(rows[1].plannedPasses > rows[1].passes,
        "the forced `multiplex` row's columns do want a split the row leaves "
        "unplanned (FR-024)");
  check(rows[2].forced != rows[3].forced
            && rows[2].printsLine != rows[3].printsLine
            && rows[2].settled == rows[3].settled
            && rows[2].passes == rows[3].passes,
        "the two one-pass `passes` rows differ in the forced record and in the "
        "line, so one can print while the other stays silent (FR-021)");
}

// T029, E-01 row 1, FR-021, clarification Q2: a run that carries no metric
// column settles nothing, plans nothing and prints nothing, whatever its
// caller asked for and whatever the host publishes. A forced method needs PMU
// events to collect, so the fields keep their default and the report stays
// master's bytes.
auto noMetricColumn() -> void
{
  const std::vector<sg::test::PassColumn> none;
  const SourceList sources = sourceListOf(none, sg::test::kHostPassLimit);
  const auto plan =
      sg::detail::planPasses(leavesOf(none, sources), sources.limits);
  check(plan.columns.empty(),
        "a run with no metric column has no plan of its own (E-03)");
  for (const auto requested : {
           sg::MetricMethod::AUTO,
           sg::MetricMethod::PASSES,
           sg::MetricMethod::MULTIPLEX,
       })
  {
    for (const bool hostPublishes : {false, true}) {
      const auto settled =
          sg::detail::settleMethod(requested, hostPublishes, 0);
      const auto passes = disclosedPassCount(settled, plan);
      check(settled == sg::MetricMethod::AUTO,
            "a run with no metric column settles nothing, whatever it asked "
            "for and whatever the host publishes (E-01, FR-025)");
      check(passes == 0,
            "a settling of nothing discloses no pass (E-07, " "FR-017)");
      check(!printsPassLine(0, settled, passes,
                            requested != sg::MetricMethod::AUTO),
            "a run with no metric column prints no line even when a method is "
            "forced (FR-021, clarification Q2)");
    }
  }
}

// T058c, FR-017, research D-6: the spread guard on a single sample.
// `passSpread` speaks across the pass times of a collection: one time
// carries no spread, so the answer for a lone sample is the empty one.
// The pair answers with a figure, so the guard reads the count.
auto spreadSingleSample() -> void
{
  check(!sg::detail::passSpread({100.0}).has_value(),
        "one pass time carries no spread to speak across (FR-017, D-6)");
  check(sg::detail::passSpread({100.0, 110.0}).has_value(),
        "a pair of pass times answers with a figure (FR-017, D-6)");
}

// T058c, FR-017's edge, research D-6: the spread guard on a zero median.
// The median of the ordered pass times scales the spread; when the lower
// central value stands exactly zero, the answer is the empty one and the
// report prints `spread=not available` in its place. The widened row keeps
// its zero and still answers, so the guard reads the median.
auto spreadZeroMedian() -> void
{
  check(!sg::detail::passSpread({0.0, 5.0}).has_value(),
        "the pair whose lower central value is zero answers no spread "
        "(FR-017, D-6)");
  check(!sg::detail::passSpread({0.0, 0.0, 5.0}).has_value(),
        "the triple whose central value is zero answers no spread "
        "(FR-017, D-6)");
  check(sg::detail::passSpread({0.0, 5.0, 6.0}).has_value(),
        "a widened row keeps its zero and reaches the median that divides "
        "(FR-017, D-6)");
}

// The child's own stderr, kept for the report the contract facility prints
// before it aborts. The list below runs one breach at a time, so one file
// serves them all.
constexpr std::string_view kBreachLog = "harness_pass_plan_breach.log";

// A contract breach ends its process through the contract facility, which
// reports the requirement it broke and then aborts, so no scenario of the list
// below can run one in process. Each breach therefore runs in a process of its
// own, chosen the way every scenario here is chosen, through `argv[1]` (the
// pattern `harness_context_barrier_test.cpp` carries). The signal alone is not
// the verdict: the release presets compile with `-D_GLIBCXX_ASSERTIONS`, whose
// standard-library range check aborts too, and an out-of-range read is exactly
// what a breach left behind does once its requirement is taken back out of the
// source. So the child's report, which names the requirement by its own
// message, is read beside the signal. The abort of the semantic this build
// configures (`speedgun-ng_CONTRACTS=enforce`) is what reports here; the
// quick semantic traps without a report and is not this build.
auto breachAborts(const char* const name, const std::string_view marker) -> void
{
  const std::string log {kBreachLog};
  const pid_t child = ::fork();
  check(child >= 0, "the breach child was created");
  if (child == 0) {
    const int report = ::open(
        log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, static_cast<mode_t>(0644));
    static_cast<void>(::dup2(report, STDERR_FILENO));
    const std::array<char*, 3> argv {
        const_cast<char*>("/proc/self/exe"), const_cast<char*>(name), nullptr};
    ::execv("/proc/self/exe", argv.data());
    ::_exit(127);
  }
  int status = 0;
  check(::waitpid(child, &status, 0) == child, "the breach child was waited");
  const std::string signal =
      std::string("the breach of ")
          .append(name)
          .append(" ends in the contract facility's abort");
  check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, signal.c_str());
  std::ifstream stream {log};
  std::ostringstream text;
  text << stream.rdbuf();
  const std::string
      reported = std::string("the breach of ")
                     .append(name)
                     .append(
                         " is the abort of the requirement it names, not of "
                         "the " "out-of-range read the breach leaves behind: ")
                     .append(marker);
  check(text.str().find(marker) != std::string::npos, reported.c_str());
}

// FR-014, FR-015: the merge reads the plan at `slot - 2` for every percentile
// slot past the two time slots, so a grid wider than the plan names passes for
// has no measuring pass for its last slot, and the precondition of the merge
// closes over it. The breach is one slot more than the table's columns.
auto breachGridPastPlan() -> void
{
  const MergeTable table = mergeTable();
  std::vector<sg::BenchmarkResult> passes;
  for (std::size_t pass = 0; pass < table.plan.columns.size(); ++pass) {
    passes.push_back(foldOfPass(table, pass));
  }
  passes.front().rows.front().percentiles.resize(3 + table.columns.size());
  (void)sg::detail::mergePasses(table.plan, passes);
}

// E-03 rule 1, FR-005: a leaf group whose `source` index runs past the limits
// the caller priced names a source no pass can be bound by. The empty plan a
// run with no published limit gets is no answer for such a group either, so
// the breach stands even where the planner would have returned early.
auto breachSourcePastLimits() -> void
{
  sg::detail::ColumnLeaves group;
  group.source = 1;
  group.addresses = {"0x309"};
  const std::vector<std::vector<sg::detail::ColumnLeaves>> leaves {{group}};
  const std::vector<std::optional<std::size_t>> limits {std::size_t {4}};
  (void)sg::detail::planPasses(leaves, limits);
}

// FR-014: the merge reads pass 0 as the reference pass, so a run that folded
// no fold result at all has nothing to merge and nothing to publish.
auto breachNoFoldResults() -> void
{
  (void)sg::detail::mergePasses(sg::detail::PassPlan {}, {});
}

// FR-014: one fold result per pass of the plan, so a plan that names more
// passes than the run folded leaves a pass whose fold result no one read.
auto breachColumnsPastFolds() -> void
{
  const MergeTable table = mergeTable();
  std::vector<sg::BenchmarkResult> passes;
  for (std::size_t pass = 0; pass + 1 < table.plan.columns.size(); ++pass) {
    passes.push_back(foldOfPass(table, pass));
  }
  (void)sg::detail::mergePasses(table.plan, passes);
}

// FR-014: every measuring pass of the plan names one fold result. The column
// count still agrees with the fold count, so this is the arm that reads past
// the last fold result and no other.
auto breachMeasuringPastFolds() -> void
{
  const MergeTable table = mergeTable();
  std::vector<sg::BenchmarkResult> passes;
  for (std::size_t pass = 0; pass < table.plan.columns.size(); ++pass) {
    passes.push_back(foldOfPass(table, pass));
  }
  sg::detail::PassPlan plan = table.plan;
  plan.measuringPass.push_back(passes.size());
  (void)sg::detail::mergePasses(plan, passes);
}

struct Breach
{
  const char* name;
  void (*body)();
  // A run of the message of the requirement this breach trips, the text the
  // contract facility reports for it.
  std::string_view marker;
};

// The breaches of this file: what the list below runs in a child of its own,
// what `argv[1]` selects for that child, and the requirement each one trips.
// A breach absent here never runs.
[[nodiscard]] auto breaches() -> const std::vector<Breach>&
{
  static const std::vector<Breach> kList {
      Breach {"breach-grid-past-plan",
              breachGridPastPlan,
              "names a measuring pass for every percentile"},
      Breach {"breach-source-past-limits",
              breachSourcePastLimits,
              "names a counter source the caller priced"},
      Breach {"breach-no-fold-results",
              breachNoFoldResults,
              "folded its reference pass"},
      Breach {"breach-columns-past-folds",
              breachColumnsPastFolds,
              "one pass for every fold result the run folded"},
      Breach {"breach-measuring-past-folds",
              breachMeasuringPastFolds,
              "every measuring pass of the plan names one fold result"},
  };
  return kList;
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from. A scenario absent here never runs.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"split-table", splitTable},
      Scenario {"determinism", determinism},
      Scenario {"minimum-count", minimumCount},
      Scenario {"over-limit", overLimit},
      Scenario {"empty-column-set", emptyColumnSet},
      Scenario {"limit-one", limitOne},
      Scenario {"sixty-four-columns", sixtyFourColumns},
      Scenario {"shared-leaf", sharedLeaf},
      Scenario {"no-limit-source", noLimitSource},
      Scenario {"clock-leaves", clockLeaves},
      Scenario {"cycles-counts", cyclesCounts},
      Scenario {"two-provider-limits", twoProviderLimits},
      Scenario {"merge-table", mergeProvenance},
      Scenario {"merge-spread-field", mergeSpreadField},
      Scenario {"no-metric-column", noMetricColumn},
      Scenario {"resolution-table", resolutionTable},
      Scenario {"spread-single-sample", spreadSingleSample},
      Scenario {"spread-zero-median", spreadZeroMedian},
  };
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : scenarios()) {
      if (scenario.name == mode) {
        scenario.body();
        return 0;
      }
    }
    for (const auto& breach : breaches()) {
      if (breach.name == mode) {
        // The mode of a breach stands for the child's own entry point: the
        // counters of the path that aborts still have to reach the coverage
        // record, which the abort of the contract facility would not flush.
        sg::test::flushCountersOnAbort();
        breach.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : scenarios()) {
    scenario.body();
  }
  for (const auto& breach : breaches()) {
    breachAborts(breach.name, breach.marker);
  }
  std::printf("harness_pass_plan_test: ok\n");
  return 0;
}
