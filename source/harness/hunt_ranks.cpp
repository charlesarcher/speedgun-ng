// specs/040-hunter-sensitivity T022 (FR-001, FR-002;
// contracts/hunt-ranks-api.md B-4, D-2): the family grouping, the point
// assembly and the sensitivity stamp. The grouping reads the suite the registry
// registered, one suite's cases standing its args or labeled sizes (D-2), and
// answers the families in first-appearance order; one point stands one entry
// that measured the event, the aggregate row's time per iteration first and the
// event's normalized rate second. The stamp reads the standing statistics
// surface over the grouped points and fills the two sensitivity fields of every
// record of the event the family's entries hold (FR-006, D-8, R-4).

#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/hunt_ranks.hpp"

#include "detail/compare.hpp"
#include "detail/statistics.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/dbc.hpp"
#include "speedgun-ng/hunt.hpp"

namespace sg::detail
{

namespace
{

// The family key of one entry: the suite the registry registered (D-2). One
// suite's cases stand its args or labeled sizes, so the suite alone names
// the family, and the case name carries the args or the labels.
auto familyKey(const sg::BenchmarkResult& result) -> std::string
{
  return result.suite;
}

// The point's time figure: the aggregate row's median (D-2). The row of a
// run carries its median in `timeStats` and the flat field stands zero
// there; a record built in place carries the flat figure and no
// statistics, so the flat field stands for it. A run whose stage measured
// one repetition carries no aggregate row, so the entry's last repetition
// row stands the figure; an entry no row prices stands no point.
auto aggregateTime(const sg::BenchmarkResult& result) -> std::optional<double>
{
  const auto found = std::ranges::find(
      result.rows, sg::RowKind::AGGREGATE, &sg::ResultRow::kind);
  if (found != result.rows.end()) {
    const auto& row = *found;
    return row.timeStats.samples > 0 ? std::optional(row.timeStats.median)
                                     : std::optional(row.timePerIterationNs);
  }
  for (auto row = result.rows.rbegin(); row != result.rows.rend(); ++row) {
    if (row->kind == sg::RowKind::REPETITION) {
      return std::optional(row->timePerIterationNs);
    }
  }
  return std::nullopt;
}

// One family under construction: the entry indices that stand in it, in the
// order the run measured them, and the points per event address the entries'
// hunt records carry. `std::map` keeps no address twice, and every appended
// point leaves its list non-empty, the invariants of `HuntFamily`.
struct FamilyBuild
{
  std::vector<std::size_t> entries;
  std::map<std::string, std::vector<std::pair<double, double>>> events;
};

// The grouping of the run's entries into families (B-4, D-2): the families in
// first-appearance order, each holding its entries in `results` order and one
// point per entry and event address the entry's hunt records carry.
auto groupEntries(const std::vector<sg::BenchmarkResult>& results)
    -> std::vector<FamilyBuild>
{
  std::vector<FamilyBuild> families;
  std::map<std::string, std::size_t> index;
  for (std::size_t entry = 0; entry < results.size(); ++entry) {
    const auto& result = results[entry];
    SG_ASSERT(result.hunt.has_value(),
              "the entry carries its hunt record, the stamping pass running "
              "after every entry's hunt stands ranked");
    auto [slot, fresh] = index.try_emplace(familyKey(result), families.size());
    if (fresh) {
      families.emplace_back();
    }
    auto& family = families[slot->second];
    const auto time = aggregateTime(result);
    if (!time.has_value()) {
      continue;
    }
    family.entries.push_back(entry);
    for (const auto& record : result.hunt->entries) {
      family.events[record.address].emplace_back(*time, record.rate);
    }
  }
  return families;
}

}  // namespace

auto familyGroups(const std::vector<sg::BenchmarkResult>& results)
    -> std::vector<HuntFamily>
{
  auto builds = groupEntries(results);
  auto families = std::vector<HuntFamily> {};
  families.reserve(builds.size());
  for (auto& build : builds) {
    families.push_back(HuntFamily {std::move(build.events)});
  }
  return families;
}

auto stampSensitivity(std::vector<sg::BenchmarkResult>& results) -> void
{
  for (const auto& family : groupEntries(results)) {
    for (const auto& [address, points] : family.events) {
      const auto correlation = spearmanCorrelation(points);
      const auto slope = leastSquaresSlope(points);
      if (!correlation.has_value() || !slope.has_value()) {
        continue;
      }
      for (const auto entry : family.entries) {
        for (auto& record : results[entry].hunt->entries) {
          if (record.address == address) {
            record.sensitivityCorrelation = correlation;
            record.sensitivitySlope = slope;
          }
        }
      }
    }
  }
}

// specs/040-hunter-sensitivity T023 (FR-003, FR-005;
// contracts/hunt-ranks-api.md B-5, D-6): the baseline extraction and the
// norm-pair check. The extraction reads the matched entry's hunt rates
// through the name-pairing rule the compare mode runs (R-2), and the
// check answers the mismatch line the refusal prints before the entries
// run (D-6).

namespace
{

// The norm words the option spells and the documents carry (E-02): the
// table the reader and the writer pair with the closed `HuntNorm`. The
// static_assert ties the table to the last enumerator, so the index of a
// value of that enumeration sits inside the table.
constexpr std::array<std::string_view, 3> kHuntNormWords {
    "instr",
    "cycle",
    "iter",
};
static_assert(kHuntNormWords.size()
              == std::to_underlying(sg::hunt::HuntNorm::ITER) + 1);

// The word one norm value spells (E-02): the word the mismatch line names
// beside the settled norm's own word.
[[nodiscard]] auto huntNormWord(const sg::hunt::HuntNorm norm)
    -> std::string_view
{
  return kHuntNormWords.at(std::to_underlying(norm));
}

}  // namespace

auto baselineHuntRates(const sg::ReportData& baseline,
                       const std::string_view matchedName)
    -> std::map<std::string, double>
{
  const auto index = indexOfResult(baseline.results, matchedName);
  if (index >= baseline.results.size()) {
    return {};
  }
  const auto& matched = baseline.results[index];
  if (!matched.hunt.has_value()) {
    return {};
  }
  auto rates = std::map<std::string, double> {};
  for (const auto& record : matched.hunt->entries) {
    const auto placed = rates.emplace(record.address, record.rate);
    SG_ASSERT(placed.second,
              "no address stands twice in the ranked list, the invariant "
              "the hunt record carries (I-4)");
  }
  return rates;
}

auto baselineNormMismatch(const sg::ReportData& baseline,
                          const sg::hunt::HuntNorm settled)
    -> std::optional<std::string>
{
  if (!baseline.hunt.has_value() || !baseline.hunt->options.norm.has_value()
      || *baseline.hunt->options.norm == settled)
  {
    return std::nullopt;
  }
  const auto baselineNorm = *baseline.hunt->options.norm;
  auto line = std::string {"the baseline hunt norm '"};
  line += huntNormWord(baselineNorm);
  line += "' does not pair with the run's settled norm '";
  line += huntNormWord(settled);
  line += '\'';
  SG_ENSURE(line.contains(huntNormWord(baselineNorm))
                && line.contains(huntNormWord(settled)),
            "the mismatch line names both norms the pair carries (D-6)");
  return line;
}

// specs/040-hunter-sensitivity T024 (FR-004; contracts/hunt-ranks-api.md
// B-6, A-5, D-5): the delta stamp. The stamp reads the map the extraction
// of B-5 answers for the matched entry and fills the signed percent of
// A-5 on every record of the addresses the map holds at a rate above
// zero; the records the map holds none of keep their delta empty.
auto stampDelta(std::vector<sg::BenchmarkResult>& results,
                const std::map<std::string, double>& baseline) -> void
{
  for (auto& result : results) {
    SG_ASSERT(result.hunt.has_value(),
              "the entry carries its hunt record, the stamping pass "
              "running after every entry's hunt stands ranked");
    for (auto& record : result.hunt->entries) {
      const auto found = baseline.find(record.address);
      if (found == baseline.end() || found->second <= 0.0) {
        continue;
      }
      record.delta = 100.0 * (record.rate - found->second) / found->second;
    }
  }
}

// specs/040-hunter-sensitivity T025 (FR-011; contracts/hunt-ranks-api.md
// B-7, A-3, A-4, D-3): the rank assignment under the two new keys. The
// stamp reads the figures the two stamps above left, orders the events
// that carry the key's figure the key orders by, and writes the ranks
// 1 to k onto every clean record of the ranked addresses; a noisy
// record's rank stands empty even at an address the key ranks, and the
// records the key ranks none of keep no rank, so the events that carry
// a rank are the clean events the key reads (A-3, A-4).

auto stampRank(std::vector<sg::BenchmarkResult>& results,
               const sg::hunt::HuntRank key) -> void
{
  SG_ASSERT(key == sg::hunt::HuntRank::SENSITIVITY
                || key == sg::hunt::HuntRank::DELTA,
            "the batch stamp ranks under the two new keys alone (B-7)");
  const bool bySensitivity = key == sg::hunt::HuntRank::SENSITIVITY;

  // One candidate per ranked address, in first-appearance order: the
  // figure the key reads. The stamping passes leave one figure for an
  // address, so the first record's figure stands for every record of it.
  struct Candidate
  {
    std::string address;
    double correlation = 0.0;
    double slope = 0.0;
    double delta = 0.0;
  };

  std::vector<Candidate> candidates;
  std::map<std::string, std::size_t> indexOf;
  for (const auto& result : results) {
    SG_ASSERT(result.hunt.has_value(),
              "the entry carries its hunt record, the stamping pass "
              "running after every entry's hunt stands ranked");
    for (const auto& record : result.hunt->entries) {
      const bool carries = bySensitivity
          ? record.sensitivityCorrelation.has_value()
          : record.delta.has_value();
      if (record.noisy || !carries) {
        continue;
      }
      if (bySensitivity) {
        SG_ASSERT(record.sensitivitySlope.has_value(),
                  "the sensitivity stamp leaves the slope beside the "
                  "correlation (D-2)");
      }
      const auto [slot, fresh] =
          indexOf.try_emplace(record.address, candidates.size());
      if (fresh) {
        candidates.push_back(
            Candidate {record.address,
                       record.sensitivityCorrelation.value_or(0.0),
                       record.sensitivitySlope.value_or(0.0),
                       record.delta.value_or(0.0)});
      }
    }
  }

  // The order the key states (D-3): the correlation descending with
  // the slope and then the address breaking a tie under
  // `SENSITIVITY`, the delta magnitude descending with the address
  // breaking a tie under `DELTA`.
  std::ranges::sort(candidates,
                    [bySensitivity](const Candidate& lhs, const Candidate& rhs)
                    {
                      if (bySensitivity) {
                        if (lhs.correlation != rhs.correlation) {
                          return lhs.correlation > rhs.correlation;
                        }
                        if (lhs.slope != rhs.slope) {
                          return lhs.slope > rhs.slope;
                        }
                      } else if (std::abs(lhs.delta) != std::abs(rhs.delta)) {
                        return std::abs(lhs.delta) > std::abs(rhs.delta);
                      }
                      return lhs.address < rhs.address;
                    });

  std::map<std::string, std::size_t> rankOf;
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    rankOf.emplace(candidates[index].address, index + 1);
  }
  // The write honors the rule the candidate set carries (A-3): a noisy
  // record's rank stands empty even at an address the key ranks.
  for (auto& result : results) {
    for (auto& record : result.hunt->entries) {
      const auto ranked = rankOf.find(record.address);
      if (record.noisy || ranked == rankOf.end()) {
        record.rank.reset();
      } else {
        record.rank = ranked->second;
      }
    }
  }
}

}  // namespace sg::detail
