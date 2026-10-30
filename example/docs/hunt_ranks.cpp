#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"

// sg-example:begin hunt-ranks
// The two rank figures of one event of a ranked hunt, read back from the
// record the run filled: the sensitivity over the benchmark family's arg
// points, the delta against the baseline (FR-012, FR-030).
auto rankCell(const std::optional<double>& figure) -> std::string
{
  return figure.has_value() ? std::to_string(*figure) : std::string {"n/a"};
}

auto printRankCells(const sg::hunt::RankedEvent& event) -> void
{
  std::printf("%s correlation=%s slope=%s delta=%s\n",
              event.address.c_str(),
              rankCell(event.sensitivityCorrelation).c_str(),
              rankCell(event.sensitivitySlope).c_str(),
              rankCell(event.delta).c_str());
}

// sg-example:end hunt-ranks

auto bmHuntRanks(sg::State& state) -> void
{
  const auto cells = static_cast<std::uint64_t>(state.range(0));
  std::uint64_t sum = 0;
  for (auto _ : state) {
    for (std::uint64_t cell = 0; cell < cells; ++cell) {
      sum += cell;
    }
    sg::doNotOptimize(sum);
  }
}

namespace sg::detail
{
// The stamps of specs/040 and the table printer beside them: the
// definitions stand in `source/harness/hunt_ranks.cpp` and
// `source/harness/report.cpp`, and none carries a header of its own, so
// this file names the functions the way
// `test/source/harness_hunt_rank_test.cpp` names them.
auto printHuntRanking(const sg::hunt::EntryHunt& hunt,
                      const sg::hunt::HuntNorm norm) -> void;
auto stampSensitivity(std::vector<sg::BenchmarkResult>& results) -> void;
auto stampDelta(std::vector<sg::BenchmarkResult>& results,
                const std::map<std::string, double>& baseline) -> void;
auto stampRank(std::vector<sg::BenchmarkResult>& results,
               const sg::hunt::HuntRank key) -> void;
}  // namespace sg::detail

namespace
{

/// One entry of the demo family, built in place the way the tests build
/// theirs (B-4): an aggregate row at the time per iteration given and one
/// hunt record per address and rate, each record repeated.
///
/// \pre none
/// \post the record names `hunt/<caseName>` and carries one aggregate row
///       at `timePerIteration` and one hunt record per pair of `events`,
///       so the figures the family holds are the figures the call states
/// \invariant the record reads the arguments alone: no clock, no counter
///            and no file enters it (FR-006)
auto familyEntry(std::string_view caseName,
                 double timePerIteration,
                 const std::vector<std::pair<std::string_view, double>>& events)
    -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.suite = "hunt";
  result.caseName = std::string(caseName);
  result.name = result.suite + '/' + result.caseName;
  auto row = sg::ResultRow {};
  row.kind = sg::RowKind::AGGREGATE;
  row.timePerIterationNs = timePerIteration;
  result.rows.push_back(row);
  auto hunt = sg::hunt::EntryHunt {};
  for (const auto& [address, rate] : events) {
    auto record = sg::hunt::RankedEvent {};
    record.address = std::string(address);
    record.rate = rate;
    record.repeated = true;
    hunt.entries.push_back(record);
  }
  result.hunt = hunt;
  SG_ENSURE(result.rows.size() == 1,
            "the record carries the aggregate row the call states (B-4)");
  SG_ENSURE(result.hunt->entries.size() == events.size(),
            "the record carries one hunt record per stated pair (B-4)");
  return result;
}

/// The demo run of the two rank columns: the family stamps in the order
/// the run path stamps it (B-10), the console table prints under each
/// key, and the compare rules of the promoted events print after the
/// tables.
///
/// \pre none
/// \post the events whose points spread carry the correlation and the
///       slope of the family, the event with no spread in its rates
///       carries its delta alone (A-2, B-6), each table prints under the
///       key its label names (D-3), and the rules lines of the promoted
///       events print beside the tables (D-7)
/// \invariant every figure the run prints reads the family's records and
///            the baseline map alone (A-1, FR-006)
auto printHuntRanks() -> void
{
  auto entries = std::vector<sg::BenchmarkResult> {
      familyEntry("args:1",
                  100.0,
                  {{"cpu/hunt_branches", 1000.0},
                   {"cpu/hunt_loads", 1500.0},
                   {"cpu/hunt_stores", 2000.0}}),
      familyEntry("args:2",
                  150.0,
                  {{"cpu/hunt_branches", 2000.0},
                   {"cpu/hunt_loads", 4500.0},
                   {"cpu/hunt_stores", 2000.0}}),
      familyEntry("args:3",
                  200.0,
                  {{"cpu/hunt_branches", 3000.0},
                   {"cpu/hunt_loads", 3000.0},
                   {"cpu/hunt_stores", 2000.0}}),
  };
  // The stamps run in the order the run path runs them (B-10): the
  // sensitivity figures over the family's points, the delta against the
  // rates the run before this one carried, then the ranks under the key
  // `--hunt-rank` settled.
  const auto baseline =
      std::map<std::string, double> {{"cpu/hunt_branches", 2500.0},
                                     {"cpu/hunt_loads", 3000.0},
                                     {"cpu/hunt_stores", 1000.0}};
  sg::detail::stampSensitivity(entries);
  sg::detail::stampDelta(entries, baseline);
  const auto& records = entries.back().hunt->entries;
  SG_ENSURE(records.front().sensitivityCorrelation.has_value()
                && !records.back().sensitivityCorrelation.has_value()
                && records.back().delta.has_value(),
            "the spread answers both figures and the no-spread event keeps "
            "its delta alone (A-2, B-6)");
  // The records stand in the order the family states them, and the rank
  // cell carries the key's rank (I-5).
  sg::detail::stampRank(entries, sg::hunt::HuntRank::SENSITIVITY);
  std::printf("hunt_ranks: --hunt-rank=sensitivity\n");
  sg::detail::printHuntRanking(entries.back().hunt.value(),
                               sg::hunt::HuntNorm::INSTR);
  sg::detail::stampRank(entries, sg::hunt::HuntRank::DELTA);
  std::printf("hunt_ranks: --hunt-rank=delta\n");
  sg::detail::printHuntRanking(entries.back().hunt.value(),
                               sg::hunt::HuntNorm::INSTR);
  // The rules lines of the promoted events, one per ranked entry in rank
  // order (D-7): the text `--hunt-emit-rules` writes, the grammar
  // `--compare-rules` reads again.
  for (const auto& line : entries.back().hunt->toCompareRules(3, 10.0)) {
    std::printf("%s\n", line.c_str());
  }
}

}  // namespace

SG_BENCHMARK(bmHuntRanks).arg(16);

// The tables print after the benchmark rows, and the figures the stamps
// leave are built in place, so the hunt lines read the same on each host
// (PD-19).
auto main(int argc, char** argv) -> int
{
  const auto status = sg::speedgunMain(argc, argv);
  printHuntRanks();
  return status;
}
