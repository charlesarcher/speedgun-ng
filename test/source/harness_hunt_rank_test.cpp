// ============================================================================
// The hunt rank decision table (specs/029-hunter-core T006, filled by T016;
// US1; FR-014, FR-030; SC-03; research D-3, D-6, D-7, PD-07, PD-08, PD-14;
// data-model E-05, E-06, E-11, I-5, I-6, I-7).
//
// The seam is `sg::hunt::rankEvents` over literal `EventSamples` records: the
// ranker takes the sample list and the settled options and reads no clock, no
// counter and no `/proc`, so every row of the table stands without a provider
// (I-7). The rate and CV expectations are the figures that
// `sg::detail::statisticsOf` produces for the same sample list, never values
// mirrored here, which is what makes the factor-of-one-hundred claim of I-6
// able to fail.
//
// The literal windows carry the fixture's own figures - `huntValue`,
// `huntCycle` and `huntInstr` of `detail/hunt_fixture.hpp` - and the fixture's
// address spelling, so no number below is invented and no host figure enters a
// case. A scenario scales a fixture figure by its pass index when it needs one
// anchor reading per pass, because the per-pass normalization of PD-08 is the
// fact under test and a constant anchor cannot show it.
//
// The settled method enters this seam as an argument, so the `multiplexed` mark
// is drivable here without a PMU: `multiplexed-method` hands `MULTIPLEX` and
// reads that word back off the record and off every row of the table (AC-2,
// D-19, SC-02). What stays outside the seam is how a run comes to settle that
// method, which `harness_hunt_collect_test.cpp` pins end to end from a scripted
// host that publishes no counter limit.
//
// `toCounterSet` shares this file's seam (033 T005, FR-013,
// contracts/counter-file.md): its cases build the `EntryHunt` record in place,
// so the call reads the ranks a scenario states and nothing else.
//
// The surface block of specs/040-hunter-sensitivity stands the two
// declarations the feature adds: a default `RankedEvent` answers the three
// new rank fields empty, and `toCompareRules` declares beside
// `toCounterSet` with the public signature FR-024 states (D-1).
//
// The baseline block of specs/040-hunter-sensitivity stands the rows
// of the two compare-path functions the header declares: the extraction
// of the matched entry's hunt rates from a loaded baseline document,
// and the norm-pair check that answers the mismatch text the refusal
// prints (B-5, D-6). The rows load the fixtures of
// `test/data/hunt_ranks/` through `loadReport` and read the figures
// the documents carry.
//
// The list at the bottom is what CTest runs, so an entry absent from it never
// runs, and `argv[1]` selects one entry of it. Frameworkless check()/fail()
// convention, the one `harness_pass_plan_test.cpp` carries.
// ============================================================================

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/hunt_fixture.hpp"
#include "detail/hunt_ranks.hpp"
#include "detail/statistics.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/hunt.hpp"

// ---------------------------------------------------------------------------
// The family surface of specs/040-hunter-sensitivity (T018, FR-001, FR-002;
// contracts/hunt-ranks-api.md B-4, D-2)
//
// `detail/hunt_ranks.hpp` declares the family grouping, the sensitivity
// stamp, the delta stamp, the baseline extraction, the norm check and
// the rank assignment beside `detail/statistics.hpp`, and
// `source/harness/hunt_ranks.cpp` defines them; T022 lifted the family
// block of this file into the header, T023 lifted the baseline block,
// T024 lifted the delta declaration and T025 lifted the rank
// declaration, each with the signature the rows pinned.
// ---------------------------------------------------------------------------

namespace
{

// The factor of the `instr` and `cycle` rates of D-6 and PD-08, and the factor
// between `RankedEvent::cv` and the `cv=` column ratio of I-6.
constexpr double kRateScale = 1000.0;
constexpr double kPercentScale = 100.0;

// The clean-sample count a repeated event needs in these cases: three windows
// give an odd-count median, and the even-count median is the case the
// exclusion scenarios reach by dropping one window out of three.
constexpr std::size_t kWindows = 3;

// The iteration count of a window whose rate is the fixture figure itself, and
// the per-pass scale of an anchor reading (D-6 normalizes per pass, so the
// readings have to differ to show which one a rate used).
constexpr std::uint64_t kOneIteration = 1;

// The `--hunt-top` of the top-N and entry-order cases: two ranks, so a third
// clean event is beyond the top and the tail of excluded events is visible.
constexpr std::size_t kTopLimit = 2;

// The `topN` of the cases that stand above a table's ranked count: nine, so a
// table of three or four ranks sits below it.
constexpr std::size_t kWideTop = 9;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT RANK TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// Exact double comparison through the bit pattern: the ranker calls the same
// summary the expectation calls, so a tolerance band would hide a second,
// different computation (I-6).
auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

// ---------------------------------------------------------------------------
// The literal sample list
// ---------------------------------------------------------------------------

// One measured window of one pass, in the spelling E-05 states. The caller
// hands the figures it wants the ranker to read; a disclosure is set by the
// scenario that needs one after the window is built.
[[nodiscard]] auto window(const std::size_t pass,
                          const std::uint64_t eventValue,
                          const std::optional<std::uint64_t> anchorInstr,
                          const std::optional<std::uint64_t> anchorCycle,
                          const std::uint64_t iterations)
    -> sg::hunt::HuntSample
{
  auto row = sg::hunt::HuntSample {};
  row.pass = pass;
  row.stage = 1;
  row.eventValue = eventValue;
  row.anchorInstr = anchorInstr;
  row.anchorCycle = anchorCycle;
  row.iterations = iterations;
  return row;
}

// One repeated event of `kWindows` windows with one rate: the event of catalog
// index `index` of the fixture's cpu device, over `iterations` iterations each.
// The anchors are the fixture figures of that device, so a rate a scenario
// derives from an anchor derives from the fixture too.
[[nodiscard]] auto repeatedEvent(const std::size_t index,
                                 const std::size_t device,
                                 const std::uint64_t eventValue,
                                 const std::uint64_t iterations)
    -> sg::hunt::EventSamples
{
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
  for (std::size_t pass = 0; pass < kWindows; ++pass) {
    collected.samples.push_back(window(pass,
                                       eventValue,
                                       sg::test::huntInstr(device),
                                       sg::test::huntCycle(device),
                                       iterations));
  }
  return collected;
}

// The windows of one event with explicit figures, over `iterations` iterations
// each and no anchor reading: the shape a noisy or single-sample row needs.
[[nodiscard]] auto windowsOf(std::size_t index,
                             const std::vector<std::uint64_t>& values,
                             const std::uint64_t iterations)
    -> sg::hunt::EventSamples
{
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
  for (std::size_t pass = 0; pass < values.size(); ++pass) {
    collected.samples.push_back(
        window(pass, values[pass], std::nullopt, std::nullopt, iterations));
  }
  return collected;
}

[[nodiscard]] auto cpuAddressOf(const std::size_t index) -> std::string
{
  return sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
}

// The index of one fixture device in `huntDevices()` order, which is the order
// `huntCycle` and `huntInstr` fold their figures in.
[[nodiscard]] auto deviceIndex(std::string_view device) -> std::size_t
{
  const auto devices = sg::test::huntDevices();
  const auto found = std::ranges::find(devices, device);
  check(found != devices.end(), "the fixture host carries that device");
  return static_cast<std::size_t>(found - devices.begin());
}

// The summary `statisticsOf` answers for one list of rates: the expectation
// every rate and CV below compares against, so no median and no deviation is
// recomputed here (I-6, FR-030). The guard is the caller-side guard E-11 states
// for the single-value case.
[[nodiscard]] auto summaryOf(const std::vector<double>& rates) -> sg::Statistics
{
  check(rates.size() > 1, "the expectation list holds more than one rate");
  return sg::detail::statisticsOf(rates);
}

// The rate list of the windows of one event under one normalization, in the
// formula of data-model.md (E-08 "Normalization and rank formulas"), with the
// two exclusions of contracts/hunt-api.md applied. The list is what the
// expectation calls `statisticsOf` with; the ranker reaches the same list from
// the same windows through its own normalization.
[[nodiscard]] auto ratesOf(const sg::hunt::EventSamples& collected,
                           const sg::hunt::HuntNorm norm) -> std::vector<double>
{
  std::vector<double> rates;
  for (const auto& row : collected.samples) {
    if (row.disclosure.has_value()) {
      continue;
    }
    if (norm == sg::hunt::HuntNorm::INSTR) {
      if (row.anchorInstr.has_value() && *row.anchorInstr != 0) {
        rates.push_back(static_cast<double>(row.eventValue)
                        / static_cast<double>(*row.anchorInstr) * kRateScale);
      }
      continue;
    }
    if (norm == sg::hunt::HuntNorm::CYCLE) {
      if (row.anchorCycle.has_value() && *row.anchorCycle != 0) {
        rates.push_back(static_cast<double>(row.eventValue)
                        / static_cast<double>(*row.anchorCycle) * kRateScale);
      }
      continue;
    }
    if (row.iterations != 0) {
      rates.push_back(static_cast<double>(row.eventValue)
                      / static_cast<double>(row.iterations));
    }
  }
  return rates;
}

[[nodiscard]] auto entryOf(const sg::hunt::EntryHunt& result,
                           const std::string& address)
    -> const sg::hunt::RankedEvent&
{
  const auto found = std::ranges::find(
      result.entries, address, &sg::hunt::RankedEvent::address);
  check(found != result.entries.end(), "the address stands in the ranked list");
  return *found;
}

// The order the addresses of the ranked list carry, so a scenario states the
// order it expects as a list of addresses.
[[nodiscard]] auto addressesOf(const sg::hunt::EntryHunt& result)
    -> std::vector<std::string>
{
  std::vector<std::string> addresses;
  for (const auto& entry : result.entries) {
    addresses.push_back(entry.address);
  }
  return addresses;
}

// I-5 and I-4 over one record: unique addresses, at most `options.top` ranks,
// the ranks `1` to `k` with no gap, and no rank for a noisy or unrepeated
// entry. Every scenario reads the figures it names on its own rows; this is the
// shape rule those figures have to stand inside.
auto checkRecordShape(const sg::hunt::EntryHunt& result,
                      const sg::hunt::HuntOptions& options) -> void
{
  for (std::size_t first = 0; first < result.entries.size(); ++first) {
    const auto& entry = result.entries[first];
    for (std::size_t second = first + 1; second < result.entries.size();
         ++second)
    {
      check(entry.address != result.entries[second].address,
            "no address stands twice in the ranked list");
    }
    check(!entry.rank.has_value() || !entry.noisy,
          "a noisy entry carries no rank");
    check(!entry.rank.has_value() || entry.repeated,
          "an unrepeated entry carries no rank");
  }
  std::vector<std::size_t> ranks;
  for (const auto& entry : result.entries) {
    if (entry.rank.has_value()) {
      ranks.push_back(*entry.rank);
    }
  }
  check(ranks.size() <= options.top, "at most --hunt-top entries carry a rank");
  std::ranges::sort(ranks);
  for (std::size_t index = 0; index < ranks.size(); ++index) {
    check(ranks[index] == index + 1, "the ranks are 1 to k with no gap");
  }
  for (const auto& skip : result.skipped) {
    for (const auto& other : result.skipped) {
      check(&skip == &other || skip.address != other.address,
            "no address stands twice in the skipped list");
    }
  }
}

// The options of a rank case: the settled defaults of E-03 with the
// normalization, the rank key and the top of the scenario.
[[nodiscard]] auto optionsFor(
    const sg::hunt::HuntNorm norm,
    const sg::hunt::HuntRank rank = sg::hunt::HuntRank::RATE,
    const std::size_t top = 20,
    const double noise = 10.0) -> sg::hunt::HuntOptions
{
  auto options = sg::hunt::HuntOptions {};
  options.norm = norm;
  options.rank = rank;
  options.top = top;
  options.noise = noise;
  return options;
}

// ---------------------------------------------------------------------------
// The normalization rows (D-6, PD-08)
// ---------------------------------------------------------------------------

// `instr` divides the event figure by the `instructions` reading of the same
// pass and scales by 1000. The anchor of each pass doubles, so the pooled
// figure (one division over the sums) and the median of the per-pass rates
// are different numbers, and the cycle anchor of the same windows differs
// from both.
auto normInstr() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(3));
  for (std::size_t pass = 0; pass < kWindows; ++pass) {
    const auto scale = static_cast<std::uint64_t>(1) << pass;
    collected.samples.push_back(window(pass,
                                       sg::test::huntValue(3),
                                       sg::test::huntInstr(device) * scale,
                                       sg::test::huntCycle(device) * scale,
                                       kOneIteration));
  }
  const auto options = optionsFor(sg::hunt::HuntNorm::INSTR);
  const auto result = sg::hunt::rankEvents({collected}, options, std::nullopt);

  check(result.entries.size() == 1, "one address gives one ranked entry");
  const auto& entry = result.entries.front();
  check(entry.address == collected.address, "the entry names the address");
  check(entry.samples == kWindows, "every window contributed a rate");
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::INSTR));
  check(sameDouble(entry.rate, stats.median),
        "the rate is the statisticsOf median of the per-pass instr rates");

  // The pooled figure a ranker that normalized after summing would report, and
  // the median over the cycle anchor of the same windows: the per-pass `instr`
  // median is neither, so a rate built from the wrong anchor or the wrong order
  // of operations cannot match it.
  std::uint64_t anchorSum = 0;
  std::uint64_t eventSum = 0;
  for (const auto& row : collected.samples) {
    anchorSum += *row.anchorInstr;
    eventSum += row.eventValue;
  }
  const auto pooled = static_cast<double>(eventSum)
      / static_cast<double>(anchorSum) * kRateScale;
  const auto cycleStats =
      summaryOf(ratesOf(collected, sg::hunt::HuntNorm::CYCLE));
  check(!sameDouble(entry.rate, pooled),
        "the rate normalizes per pass before pooling");
  check(!sameDouble(entry.rate, cycleStats.median),
        "the instr rate divides by the instructions anchor");
}

// `cycle` divides by the `cpu-cycles` reading of the same pass, over the very
// windows above: the two anchors of every window carry different figures, so
// the two normalizations answer different medians and each entry has to carry
// its own.
auto normCycle() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(3));
  for (std::size_t pass = 0; pass < kWindows; ++pass) {
    const auto scale = static_cast<std::uint64_t>(pass + 1);
    collected.samples.push_back(window(pass,
                                       sg::test::huntValue(3),
                                       sg::test::huntInstr(device) * scale,
                                       sg::test::huntCycle(device) * scale,
                                       kOneIteration));
  }
  const auto options = optionsFor(sg::hunt::HuntNorm::CYCLE);
  const auto result = sg::hunt::rankEvents({collected}, options, std::nullopt);

  const auto& entry = entryOf(result, collected.address);
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::CYCLE));
  check(sameDouble(entry.rate, stats.median),
        "the rate is the statisticsOf median of the per-pass cycle rates");
  check(entry.samples == kWindows, "every window contributed a rate");
  const auto instrStats =
      summaryOf(ratesOf(collected, sg::hunt::HuntNorm::INSTR));
  check(!sameDouble(entry.rate, instrStats.median),
        "the cycle rate divides by the cycles anchor");
}

// `iter` reports the raw median per iteration: no anchor enters the figure, so
// the anchors of the windows carry the values that would answer a different
// median if the ranker read them.
auto normIter() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(2));
  for (std::size_t pass = 0; pass < kWindows; ++pass) {
    const auto scale = static_cast<std::uint64_t>(pass + 1);
    collected.samples.push_back(window(pass,
                                       sg::test::huntValue(2),
                                       sg::test::huntInstr(device),
                                       sg::test::huntCycle(device),
                                       sg::test::kHuntValueStep * scale));
  }
  const auto options = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto result = sg::hunt::rankEvents({collected}, options, std::nullopt);

  const auto& entry = entryOf(result, collected.address);
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::ITER));
  check(sameDouble(entry.rate, stats.median),
        "the rate is the statisticsOf median of the per-iteration figures");
  check(!sameDouble(entry.rate, stats.median * kRateScale),
        "the iter rate carries no per-thousand scale");

  // The anchors are laid on the same windows for the contrast: an anchor-read
  // rate over them answers a different median.
  const auto anchorStats =
      summaryOf(ratesOf(collected, sg::hunt::HuntNorm::INSTR));
  check(!sameDouble(entry.rate, anchorStats.median),
        "the iter rate reads no anchor");
}

// ---------------------------------------------------------------------------
// The two exclusion rows of contracts/hunt-api.md
// ---------------------------------------------------------------------------

// A window whose anchor read zero contributes no rate for its pass: three
// windows, one of them with a zero `instructions` reading, leave the even-count
// median of the two that remain, where the list of three would answer another
// figure and a division by zero a third.
auto zeroAnchorExcluded() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto anchor = sg::test::huntInstr(device);
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(0));
  collected.samples.push_back(window(0,
                                     sg::test::huntValue(0),
                                     anchor,
                                     sg::test::huntCycle(device),
                                     kOneIteration));
  collected.samples.push_back(window(1,
                                     sg::test::huntValue(1),
                                     std::uint64_t {0},
                                     sg::test::huntCycle(device),
                                     kOneIteration));
  collected.samples.push_back(window(2,
                                     sg::test::huntValue(0),
                                     anchor * 2,
                                     sg::test::huntCycle(device),
                                     kOneIteration));

  const auto options = optionsFor(sg::hunt::HuntNorm::INSTR);
  const auto result = sg::hunt::rankEvents({collected}, options, std::nullopt);

  const auto& entry = entryOf(result, collected.address);
  check(entry.samples == kWindows - 1,
        "the zero-anchor window contributes no sample to the record");
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::INSTR));
  check(stats.samples == kWindows - 1,
        "the expectation list drops the zero-anchor window");
  check(std::isfinite(entry.rate),
        "the excluded window leaves no infinite rate");
  check(sameDouble(entry.rate, stats.median),
        "the median reads the windows that kept an anchor reading");
  check(sameDouble(entry.cv, kPercentScale * stats.coefficientOfVariation),
        "the CV reads the windows that kept an anchor reading");
  checkRecordShape(result, options);
}

// A window whose divisor for the norm of the run carries no reading
// contributes no rate at all: the instr norm over a window with no
// instructions reading, the cycle norm over a window with no cycle reading and
// over one whose cycle reading is 0, and the iter norm over a window that
// counted no iteration. Every excluded window carries an event figure that
// would move the median and the deviation, so no exclusion here can pass by
// coincidence (E-08, contracts/hunt-api.md).
auto zeroDivisorExcluded() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto cycle = sg::test::huntCycle(device);
  const auto wide = sg::test::huntValue(sg::test::kHuntEvents - 1);

  // One window of three carries a cycle reading, and it is the narrowest of
  // the three: the median of the run is that one rate, and the two windows
  // without a divisor would answer another figure each if they stood in the
  // list.
  auto anchorless = sg::hunt::EventSamples {};
  anchorless.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(4));
  anchorless.samples.push_back(window(0,
                                      sg::test::huntValue(4),
                                      sg::test::huntInstr(device),
                                      cycle * 2,
                                      kOneIteration));
  anchorless.samples.push_back(window(
      1, wide, sg::test::huntInstr(device), std::nullopt, kOneIteration));
  anchorless.samples.push_back(
      window(2, wide, sg::test::huntInstr(device), 0, kOneIteration));

  const auto cycleOptions = optionsFor(sg::hunt::HuntNorm::CYCLE);
  const auto cycled =
      sg::hunt::rankEvents({anchorless}, cycleOptions, std::nullopt);
  const auto& cycleEntry = entryOf(cycled, anchorless.address);
  const auto kept = static_cast<double>(sg::test::huntValue(4))
      / static_cast<double>(cycle * 2) * kRateScale;
  check(cycleEntry.samples == 1,
        "the windows with no cycle divisor contribute no rate");
  check(sameDouble(cycleEntry.rate, kept),
        "the rate is the one window that carries a cycle reading");
  check(!cycleEntry.repeated, "one rate leaves the entry unrepeated");
  check(!cycleEntry.rank.has_value(), "an unrepeated entry carries no rank");
  checkRecordShape(cycled, cycleOptions);

  // The instr norm over the same shape, the norm `--hunt-norm` leaves unset:
  // the window with no instructions reading stands out of the list and the one
  // that divided answers the rate (contracts/hunt-api.md: a denominator absent
  // or at zero answers no rate for that pass).
  auto noInstr = sg::hunt::EventSamples {};
  noInstr.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(6));
  const auto keptInstr = sg::test::huntInstr(device) * 2;
  noInstr.samples.push_back(
      window(0, sg::test::huntValue(6), keptInstr, cycle, kOneIteration));
  noInstr.samples.push_back(
      window(1, wide, std::nullopt, cycle * 2, kOneIteration));

  const auto instrOptions = optionsFor(sg::hunt::HuntNorm::INSTR);
  const auto instrRated =
      sg::hunt::rankEvents({noInstr}, instrOptions, std::nullopt);
  const auto& instrEntry = entryOf(instrRated, noInstr.address);
  const auto keptRate = static_cast<double>(sg::test::huntValue(6))
      / static_cast<double>(keptInstr) * kRateScale;
  check(instrEntry.samples == 1,
        "the window with no instructions divisor contributes no rate");
  check(sameDouble(instrEntry.rate, keptRate),
        "the rate is the one window that carries an instructions reading");
  check(!instrEntry.repeated, "one rate leaves the entry unrepeated");
  check(!instrEntry.rank.has_value(), "an unrepeated entry carries no rank");
  checkRecordShape(instrRated, instrOptions);

  // The iter norm over the same shape: the window that counted no iteration is
  // out of the list, and the two that stand in it answer different rates, so
  // the median and the deviation both read the exclusion.
  auto uncounted = sg::hunt::EventSamples {};
  uncounted.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(2));
  uncounted.samples.push_back(window(0,
                                     sg::test::huntValue(2),
                                     sg::test::huntInstr(device),
                                     cycle,
                                     kOneIteration));
  uncounted.samples.push_back(
      window(1, wide, sg::test::huntInstr(device), cycle, 0));
  uncounted.samples.push_back(window(2,
                                     sg::test::huntValue(2),
                                     sg::test::huntInstr(device),
                                     cycle,
                                     kOneIteration * 2));

  const auto iterOptions = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto iterated =
      sg::hunt::rankEvents({uncounted}, iterOptions, std::nullopt);
  const auto& iterEntry = entryOf(iterated, uncounted.address);
  check(iterEntry.samples == kWindows - 1,
        "the window that counted no iteration contributes no rate");
  const auto stats = summaryOf(ratesOf(uncounted, sg::hunt::HuntNorm::ITER));
  check(stats.samples == kWindows - 1,
        "the expectation list drops the window with no iteration count");
  check(sameDouble(iterEntry.rate, stats.median),
        "the median reads the windows that counted iterations");
  check(sameDouble(iterEntry.cv, kPercentScale * stats.coefficientOfVariation),
        "the deviation reads the windows that counted iterations");
  checkRecordShape(iterated, iterOptions);
}

// A window that carries a disclosure contributes none at all, and the address
// that measured and gapped stands in the ranked list and in the skipped list
// (I-4, D-10). The gapped window carries a rate that would move the median, so
// the exclusion cannot pass by coincidence.
auto disclosureExcluded() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto anchor = sg::test::huntInstr(device);
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(0));
  collected.samples.push_back(window(0,
                                     sg::test::huntValue(0),
                                     anchor,
                                     sg::test::huntCycle(device),
                                     kOneIteration));
  auto gapped = window(1,
                       sg::test::huntValue(sg::test::kHuntEvents - 1),
                       anchor,
                       sg::test::huntCycle(device),
                       kOneIteration);
  gapped.disclosure = sg::counters::Availability::GAP;
  collected.samples.push_back(gapped);
  collected.samples.push_back(window(2,
                                     sg::test::huntValue(0),
                                     anchor * 2,
                                     sg::test::huntCycle(device),
                                     kOneIteration));
  collected.skipped = sg::hunt::SkippedKind::GAP;

  const auto options = optionsFor(sg::hunt::HuntNorm::INSTR);
  const auto result = sg::hunt::rankEvents({collected}, options, std::nullopt);

  const auto& entry = entryOf(result, collected.address);
  check(entry.samples == kWindows - 1,
        "the disclosed window is out of the rate list");
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::INSTR));
  check(sameDouble(entry.rate, stats.median),
        "the median reads the windows free of a disclosure");
  check(sameDouble(entry.cv, kPercentScale * stats.coefficientOfVariation),
        "the CV reads the windows free of a disclosure");
  check(result.entries.size() == 1, "one address gives one ranked entry");
  check(result.skipped.size() == 1,
        "the gapped address stands once in skipped");
  check(result.skipped.front().address == collected.address,
        "the skipped entry names the address that measured and gapped");
  check(result.skipped.front().kind == sg::hunt::SkippedKind::GAP,
        "the skipped entry carries the kind the address carried");
  checkRecordShape(result, options);
}

// ---------------------------------------------------------------------------
// The shared-summary row (I-6, FR-030)
// ---------------------------------------------------------------------------

// One `statisticsOf` call answers both figures: the reported `rate` is that
// call's median and the reported `cv` is that call's coefficient of variation
// times one hundred, the factor between the record and the printed `cv=` column
// (I-6). Four windows make the median the even-count one.
auto statisticsAgreement() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto anchor = sg::test::huntInstr(device);
  auto collected = sg::hunt::EventSamples {};
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(7));
  const std::size_t kEventIndices[] = {1, 4, 9, 2};
  for (std::size_t pass = 0; pass < std::size(kEventIndices); ++pass) {
    collected.samples.push_back(window(pass,
                                       sg::test::huntValue(kEventIndices[pass]),
                                       anchor,
                                       sg::test::huntCycle(device),
                                       kOneIteration));
  }
  const auto options = optionsFor(sg::hunt::HuntNorm::INSTR);
  const auto result = sg::hunt::rankEvents({collected}, options, std::nullopt);

  const auto& entry = entryOf(result, collected.address);
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::INSTR));
  check(entry.samples == std::size(kEventIndices), "every window contributed");
  check(stats.samples == entry.samples, "the summary read the same list");
  check(sameDouble(entry.rate, stats.median),
        "the rate is the median of the one statisticsOf call");
  check(sameDouble(entry.cv, kPercentScale * stats.coefficientOfVariation),
        "the cv is one hundred times the coefficientOfVariation of that call");
  check(stats.coefficientOfVariation > 0.0, "the list carries a deviation");
  check(!sameDouble(entry.cv, stats.coefficientOfVariation),
        "the record carries the percent form of the ratio");
  check(entry.repeated, "more than one window makes the event repeated");
}

// ---------------------------------------------------------------------------
// The noise row (D-7)
// ---------------------------------------------------------------------------

// `noisy` is the comparison of the unrounded `cv` against `--hunt-noise`: the
// threshold of every row is derived from the `coefficientOfVariation`
// `statisticsOf` answers for the same windows, so the equality row pins the
// strictness of `above`, and the row between the unrounded value and its
// one-decimal rounding is the row a comparison on a rounded value fails.
auto noiseThreshold() -> void
{
  const auto collected = []
  {
    auto event = sg::hunt::EventSamples {};
    event.address =
        sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(1));
    const std::size_t indices[] = {0, 1, 3};
    for (std::size_t pass = 0; pass < std::size(indices); ++pass) {
      event.samples.push_back(window(pass,
                                     sg::test::huntValue(indices[pass]),
                                     std::nullopt,
                                     std::nullopt,
                                     sg::test::kHuntValueStep));
    }
    return event;
  }();
  const auto stats = summaryOf(ratesOf(collected, sg::hunt::HuntNorm::ITER));
  const auto percent = kPercentScale * stats.coefficientOfVariation;
  // The one-decimal rounding of the same figure, the print of the `cv=` column.
  const auto rounded = std::round(percent * 10.0) / 10.0;
  check(!sameDouble(rounded, percent),
        "the threshold rows need an unrounded figure");
  const auto between = (percent + rounded) / 2.0;

  struct Row
  {
    double noise;
    bool noisy;
  };

  const Row rows[] = {
      Row {percent, false},
      Row {percent * 1.001, false},
      Row {percent * 0.999, true},
      Row {between, false},
  };
  for (const auto& row : rows) {
    const auto options = optionsFor(
        sg::hunt::HuntNorm::ITER, sg::hunt::HuntRank::RATE, 20, row.noise);
    const auto result =
        sg::hunt::rankEvents({collected}, options, std::nullopt);
    const auto& entry = entryOf(result, collected.address);
    check(sameDouble(entry.cv, percent), "the record cv is the derived figure");
    check(entry.noisy == row.noisy,
          "noisy reads the unrounded cv against noise");
  }
}

// ---------------------------------------------------------------------------
// The rank-order rows (PD-07)
// ---------------------------------------------------------------------------

// The `rate` key orders by median rate descending, a tie broken by address
// ascending: the tie pair carries one figure at two addresses, and the input
// order is scrambled, so neither the input order nor the address order alone
// answers the expected table.
auto rankRateOrder() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto high =
      repeatedEvent(5, device, sg::test::huntValue(8), kOneIteration);
  const auto tieFirst =
      repeatedEvent(3, device, sg::test::huntValue(4), kOneIteration);
  const auto tieSecond =
      repeatedEvent(11, device, sg::test::huntValue(4), kOneIteration);
  const auto low =
      repeatedEvent(2, device, sg::test::huntValue(0), kOneIteration);
  const auto options = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto result = sg::hunt::rankEvents(
      {tieSecond, high, tieFirst, low}, options, std::nullopt);

  check(sameDouble(entryOf(result, tieFirst.address).rate,
                   entryOf(result, tieSecond.address).rate),
        "the tie pair carries one rate");
  const std::vector<std::string> kExpected {
      cpuAddressOf(5),
      cpuAddressOf(3),
      cpuAddressOf(11),
      cpuAddressOf(2),
  };
  check(addressesOf(result) == kExpected,
        "rate descending, ties by address ascending");
  for (std::size_t index = 0; index < kExpected.size(); ++index) {
    const auto rank = entryOf(result, kExpected[index]).rank;
    check(rank.has_value() && *rank == index + 1,
          "the ordered entries carry the ranks 1 to k");
  }
  checkRecordShape(result, options);
}

// The `cv` key orders by coefficient of variation ascending, ties by address
// ascending. All three events share one median rate, so the `rate` key of the
// same sample list answers a different table: the order below can only come
// from the CV key.
auto rankCvOrder() -> void
{
  const auto steady = windowsOf(9,
                                {sg::test::huntValue(50),
                                 sg::test::huntValue(50),
                                 sg::test::huntValue(50)},
                                sg::test::kHuntValueStep);
  const auto spread = std::vector<std::uint64_t> {sg::test::huntValue(49),
                                                  sg::test::huntValue(50),
                                                  sg::test::huntValue(51)};
  const auto wideFirst = windowsOf(4, spread, sg::test::kHuntValueStep);
  const auto wideSecond = windowsOf(12, spread, sg::test::kHuntValueStep);

  const auto byCv =
      optionsFor(sg::hunt::HuntNorm::ITER, sg::hunt::HuntRank::CV);
  const auto input = {wideSecond, steady, wideFirst};
  const auto result = sg::hunt::rankEvents(input, byCv, std::nullopt);

  const auto median = entryOf(result, steady.address).rate;
  check(sameDouble(entryOf(result, wideFirst.address).rate, median),
        "the three events share one median rate");
  const std::vector<std::string> kCvOrder {
      cpuAddressOf(9),
      cpuAddressOf(4),
      cpuAddressOf(12),
  };
  check(addressesOf(result) == kCvOrder,
        "cv ascending, ties by address ascending");
  for (std::size_t index = 0; index < kCvOrder.size(); ++index) {
    const auto rank = entryOf(result, kCvOrder[index]).rank;
    check(rank.has_value() && *rank == index + 1,
          "the cv-ordered entries carry the ranks 1 to k");
  }
  check(!sameDouble(entryOf(result, steady.address).cv,
                    entryOf(result, wideFirst.address).cv),
        "the steady event and the spread events carry different cvs");

  const auto byRate = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto rateTable = sg::hunt::rankEvents(input, byRate, std::nullopt);
  const std::vector<std::string> kRateOrder {
      cpuAddressOf(4),
      cpuAddressOf(9),
      cpuAddressOf(12),
  };
  check(addressesOf(rateTable) == kRateOrder,
        "the rate key of the same list orders the tied medians by address");
}

// ---------------------------------------------------------------------------
// The top-N, single-sample and mark rows (I-5, PD-14)
// ---------------------------------------------------------------------------

// At most `--hunt-top` entries carry a rank, the ranks run 1 to k with no gap,
// and a noisy or unrepeated entry carries none - even when its rate would sit
// inside the top. The two excluded events carry the lowest medians, so the
// numbered rows are the two clean ones under any reading of the rule.
auto topLimit() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto first =
      repeatedEvent(1, device, sg::test::huntValue(20), kOneIteration);
  const auto second =
      repeatedEvent(6, device, sg::test::huntValue(10), kOneIteration);
  const auto third =
      repeatedEvent(14, device, sg::test::huntValue(5), kOneIteration);
  const auto flat = windowsOf(
      8,
      {sg::test::huntValue(0), sg::test::huntValue(30), sg::test::huntValue(0)},
      kOneIteration);
  auto lone = windowsOf(0, {sg::test::huntValue(0)}, 2);

  const auto options =
      optionsFor(sg::hunt::HuntNorm::ITER, sg::hunt::HuntRank::RATE, kTopLimit);
  const auto result = sg::hunt::rankEvents(
      {lone, flat, third, second, first}, options, std::nullopt);

  check(result.entries.size() == 5, "every address holds one ranked record");
  check(result.top == kTopLimit, "the record states the --hunt-top it used");
  const auto firstRank = entryOf(result, first.address).rank;
  const auto secondRank = entryOf(result, second.address).rank;
  check(firstRank.has_value() && *firstRank == 1, "the best rate ranks first");
  check(secondRank.has_value() && *secondRank == 2,
        "the second rate ranks second");
  check(!entryOf(result, third.address).rank.has_value(),
        "an event beyond --hunt-top carries no rank");
  const auto& flatEntry = entryOf(result, flat.address);
  check(flatEntry.noisy, "the flat event stands above the noise threshold");
  check(!flatEntry.rank.has_value(), "a noisy event carries no rank");
  const auto& loneEntry = entryOf(result, lone.address);
  check(!loneEntry.repeated, "one window makes the event unrepeated");
  check(!loneEntry.rank.has_value(), "an unrepeated event carries no rank");
  checkRecordShape(result, options);
}

// One sample is reported, never summarized: `cv` is 0.0, the event is
// unrepeated, it carries no rank, and the rate is the one window's figure
// (PD-14). A deviation over one value would be an invented fact and the shared
// summary asserts against it.
auto singleSample() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto anchor = sg::test::huntInstr(device);
  auto lone = sg::hunt::EventSamples {};
  lone.address = cpuAddressOf(1);
  lone.samples.push_back(window(0,
                                sg::test::huntValue(2),
                                anchor,
                                sg::test::huntCycle(device),
                                kOneIteration));

  const auto options = optionsFor(sg::hunt::HuntNorm::INSTR);
  const auto result = sg::hunt::rankEvents({lone}, options, std::nullopt);

  const auto& entry = entryOf(result, lone.address);
  check(sameDouble(entry.rate,
                   static_cast<double>(sg::test::huntValue(2))
                       / static_cast<double>(anchor) * kRateScale),
        "the rate of one sample is that sample's figure");
  check(sameDouble(entry.cv, 0.0), "one sample reports no deviation");
  check(entry.samples == 1, "the record counts the one sample");
  check(!entry.repeated, "one sample is not a repeated event");
  check(!entry.noisy, "an unknown deviation is not a noisy event");
  check(!entry.rank.has_value(), "one sample carries no rank");
  check(result.skipped.empty(), "a measured event is not a skipped event");
}

// With no plan behind the samples the caller hands no method, so the record
// states none and no row may read multiplexed (AC-2): the mark is the method of
// the record and never a figure of the sample list.
auto multiplexedMark() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto options = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto result = sg::hunt::rankEvents(
      {repeatedEvent(1, device, sg::test::huntValue(8), kOneIteration),
       repeatedEvent(2, device, sg::test::huntValue(4), kOneIteration)},
      options,
      std::nullopt);

  check(!result.method.has_value(),
        "samples no plan produced leave the record with no settled method");
  check(!result.entries.empty(), "the table carries the events it was given");
  for (const auto& entry : result.entries) {
    check(!entry.multiplexed,
          "a record that states no method marks no row multiplexed");
  }
}

// The rank route settles the method its caller hands: `MULTIPLEX` is the method
// the record states and the word every row of its table carries (AC-2, D-19,
// SC-02), `PASSES` states itself with no row marked, and the figures of a row
// stand the same under either method. How a run settles `MULTIPLEX` on its own
// stays pinned at the scripted-run seam of `harness_hunt_collect_test.cpp`.
auto multiplexedMethod() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto options = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto collected = std::vector<sg::hunt::EventSamples> {
      repeatedEvent(1, device, sg::test::huntValue(8), kOneIteration),
      repeatedEvent(2, device, sg::test::huntValue(4), kOneIteration),
  };

  const auto multiplex =
      sg::hunt::rankEvents(collected, options, sg::MetricMethod::MULTIPLEX);
  check(multiplex.method.has_value()
            && multiplex.method.value() == sg::MetricMethod::MULTIPLEX,
        "the record states the MULTIPLEX method its caller handed");
  check(!multiplex.entries.empty(),
        "the table carries the events it was given");
  const bool allMarked = std::all_of(multiplex.entries.begin(),
                                     multiplex.entries.end(),
                                     [](const sg::hunt::RankedEvent& entry)
                                     { return entry.multiplexed; });
  check(allMarked,
        "every row of a MULTIPLEX table reads multiplexed (D-19, SC-02)");

  const auto passes =
      sg::hunt::rankEvents(collected, options, sg::MetricMethod::PASSES);
  check(passes.method.has_value()
            && passes.method.value() == sg::MetricMethod::PASSES,
        "the record states the PASSES method its caller handed");
  const bool noneMarked = std::all_of(passes.entries.begin(),
                                      passes.entries.end(),
                                      [](const sg::hunt::RankedEvent& entry)
                                      { return !entry.multiplexed; });
  check(noneMarked, "a PASSES table marks no row multiplexed");

  // The method marks the table and moves nothing in it: the figures of both
  // tables are those of the table built with no method at all.
  const auto plain = sg::hunt::rankEvents(collected, options, std::nullopt);
  check(multiplex.entries.size() == plain.entries.size()
            && passes.entries.size() == plain.entries.size(),
        "the method settles no row in or out of the table");
  for (std::size_t index = 0; index < plain.entries.size(); ++index) {
    const auto& expected = plain.entries[index];
    const auto& marked = multiplex.entries[index];
    check(sameDouble(marked.rate, expected.rate)
              && sameDouble(marked.cv, expected.cv)
              && sameDouble(passes.entries[index].rate, expected.rate),
          "the rate and the deviation of a row read no method");
  }
  checkRecordShape(multiplex, options);
}

// Entries come first in rank order and then in address order: the two excluded
// events are the two lowest addresses and the lowest medians come last in the
// ranked block, so the tail order can only be the address order.
auto entryOrder() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  const auto ranked =
      repeatedEvent(1, device, sg::test::huntValue(8), kOneIteration);
  const auto runnerUp =
      repeatedEvent(6, device, sg::test::huntValue(4), kOneIteration);
  const auto flat = windowsOf(
      3,
      {sg::test::huntValue(0), sg::test::huntValue(30), sg::test::huntValue(0)},
      kOneIteration);
  auto lone = windowsOf(0, {sg::test::huntValue(0)}, 2);

  const auto options =
      optionsFor(sg::hunt::HuntNorm::ITER, sg::hunt::HuntRank::RATE, kTopLimit);
  const auto result = sg::hunt::rankEvents(
      {flat, lone, runnerUp, ranked}, options, std::nullopt);

  const std::vector<std::string> kExpected {
      cpuAddressOf(1),
      cpuAddressOf(6),
      cpuAddressOf(0),
      cpuAddressOf(3),
  };
  check(addressesOf(result) == kExpected,
        "the ranked entries first, the excluded ones in address order");
  check(entryOf(result, lone.address).rate < entryOf(result, flat.address).rate,
        "the tail order is not the rate order of the same two records");
  checkRecordShape(result, options);
}

// Two calls over equal arguments give equal results, and the table does not
// depend on the order the addresses arrived in (I-7, FR-014): the same records
// handed over in reverse order answer the same table, field for field.
auto pureRepeat() -> void
{
  const auto device = deviceIndex(sg::test::kHuntCpu);
  auto collected = std::vector<sg::hunt::EventSamples> {
      repeatedEvent(1, device, sg::test::huntValue(8), kOneIteration),
      windowsOf(3,
                {sg::test::huntValue(0),
                 sg::test::huntValue(30),
                 sg::test::huntValue(0)},
                kOneIteration),
      windowsOf(0, {sg::test::huntValue(0)}, 2),
  };
  const auto options = optionsFor(sg::hunt::HuntNorm::ITER);
  const auto first = sg::hunt::rankEvents(collected, options, std::nullopt);
  const auto second = sg::hunt::rankEvents(collected, options, std::nullopt);
  auto reversed =
      std::vector<sg::hunt::EventSamples>(collected.rbegin(), collected.rend());
  const auto third = sg::hunt::rankEvents(reversed, options, std::nullopt);

  check(first.entries.size() == second.entries.size(),
        "two calls over equal arguments answer the same count");
  for (const auto& other : {second, third}) {
    check(other.entries.size() == first.entries.size(),
          "the table does not depend on the order of the argument list");
    check(other.skipped.size() == first.skipped.size(),
          "the skipped list repeats across calls");
    for (std::size_t index = 0; index < first.entries.size(); ++index) {
      const auto& lhs = first.entries[index];
      const auto& rhs = other.entries[index];
      check(lhs.address == rhs.address, "the order of the table repeats");
      check(sameDouble(lhs.rate, rhs.rate), "the rate repeats bit for bit");
      check(sameDouble(lhs.cv, rhs.cv), "the cv repeats bit for bit");
      check(lhs.samples == rhs.samples, "the sample count repeats");
      check(lhs.noisy == rhs.noisy, "the noise mark repeats");
      check(lhs.repeated == rhs.repeated, "the repeat mark repeats");
      check(lhs.multiplexed == rhs.multiplexed, "the multiplexed mark repeats");
      check(lhs.rank == rhs.rank, "the rank repeats");
    }
  }
}

// ---------------------------------------------------------------------------
// The emitted set of `toCounterSet` (033 T005; FR-013;
// contracts/counter-file.md)
// ---------------------------------------------------------------------------

// One row of a ranked table built in place: the address of fixture event
// `index` under the rank the scenario states, with the marks I-5 pairs with
// that rank. The rate and the deviation stay at their defaults, because the
// emitted set reads the rank and the address.
[[nodiscard]] auto rankRow(const std::size_t index,
                           const std::optional<std::size_t> rank)
    -> sg::hunt::RankedEvent
{
  auto entry = sg::hunt::RankedEvent {};
  entry.address = cpuAddressOf(index);
  entry.rank = rank;
  entry.repeated = rank.has_value();
  return entry;
}

// The ranked entries come back in ascending rank, each carrying its own
// address. The four ranks stand scrambled in the record's order, and the
// address order answers a third table, so neither the vector order nor the
// address order can match the expectation. `topN` holds the ranked count, which
// leaves the order as the single fact under test.
auto counterSetRankOrder() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    hunt.entries = {rankRow(14, std::size_t {3}),
                    rankRow(2, std::size_t {1}),
                    rankRow(11, std::size_t {4}),
                    rankRow(9, std::size_t {2})};
    return hunt;
  }();

  const std::vector<std::string> kExpected {
      cpuAddressOf(2),
      cpuAddressOf(9),
      cpuAddressOf(14),
      cpuAddressOf(11),
  };
  check(record.toCounterSet(record.entries.size()) == kExpected,
        "the ranked addresses come back in ascending rank");
}

// A `topN` below the ranked count answers exactly the leading `topN` addresses
// of the ascending list, for every cap below that count. The record states the
// default `--hunt-top` of 20 above its own four ranks, so the argument of the
// call and no field of the record cuts the answer.
auto counterSetCap() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    hunt.entries = {rankRow(6, std::size_t {4}),
                    rankRow(2, std::size_t {1}),
                    rankRow(9, std::size_t {3}),
                    rankRow(1, std::size_t {2})};
    return hunt;
  }();

  const std::vector<std::string> kAll {
      cpuAddressOf(2),
      cpuAddressOf(1),
      cpuAddressOf(9),
      cpuAddressOf(6),
  };
  check(record.top > record.entries.size(),
        "the record states a --hunt-top above its own ranked count");
  const auto full = record.toCounterSet(record.entries.size());
  check(full == kAll, "the whole table answers every ranked address");
  for (std::size_t cap = 1; cap < full.size(); ++cap) {
    const auto cut = record.toCounterSet(cap);
    check(cut.size() == cap, "the answer holds at most topN addresses");
    check(std::equal(cut.begin(), cut.end(), full.begin()),
          "a cap below the ranked count answers the leading ranks");
  }
}

// An entry whose `rank` stands empty never enters, with `topN` above the ranked
// count: the table holds one row above the noise threshold, one row the run did
// not repeat, and one row it also lists as skipped. Each stands between two
// ranked addresses, so an answer that carried a null rank would show it.
auto counterSetNullRank() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    auto loud = rankRow(3, std::nullopt);
    loud.noisy = true;
    loud.repeated = true;
    auto gap = sg::hunt::SkippedEvent {};
    gap.address = cpuAddressOf(8);
    gap.kind = sg::hunt::SkippedKind::GAP;
    hunt.entries = {rankRow(2, std::size_t {2}),
                    rankRow(0, std::nullopt),
                    rankRow(1, std::size_t {1}),
                    loud,
                    rankRow(8, std::nullopt)};
    hunt.skipped.push_back(gap);
    return hunt;
  }();

  const std::vector<std::string> kExpected {
      cpuAddressOf(1),
      cpuAddressOf(2),
  };
  check(record.toCounterSet(record.entries.size() + 2) == kExpected,
        "an entry with no rank stands out of the answer");
}

// `topN` zero answers none: a record of three ranks answers an empty list, and
// the record keeps its rows. The standing table is what makes the empty answer
// the argument's verdict on a ranked record.
auto counterSetZeroTop() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    hunt.entries = {rankRow(1, std::size_t {1}),
                    rankRow(6, std::size_t {2}),
                    rankRow(9, std::size_t {3})};
    return hunt;
  }();

  check(record.toCounterSet(0).empty(), "topN zero answers no address");
  check(record.entries.size() == 3, "the record keeps its three ranked rows");
}

// `topN` above the ranked count answers them all: the answer carries every
// ranked address once and stops at the ranked count, so the wide argument pads
// nothing and repeats nothing.
auto counterSetAboveCount() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    hunt.entries = {rankRow(4, std::size_t {2}),
                    rankRow(12, std::size_t {3}),
                    rankRow(1, std::size_t {1})};
    return hunt;
  }();

  const std::vector<std::string> kExpected {
      cpuAddressOf(1),
      cpuAddressOf(4),
      cpuAddressOf(12),
  };
  const auto answer = record.toCounterSet(kWideTop);
  check(answer == kExpected, "a topN above the ranked count answers them all");
  check(answer.size() == record.entries.size(),
        "the answer stops at the ranked count");
}

// Two calls over one record answer equal lists, and the record stands as
// built: its entry order and its ranks keep the figures the scenario laid down,
// so the second call answers the same list from the same rows and the first
// call changed nothing to read.
auto counterSetRepeat() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    hunt.entries = {rankRow(9, std::size_t {2}),
                    rankRow(1, std::size_t {1}),
                    rankRow(4, std::size_t {3})};
    return hunt;
  }();

  const std::vector<std::string> kBuilt {
      cpuAddressOf(9),
      cpuAddressOf(1),
      cpuAddressOf(4),
  };
  const std::vector<std::size_t> kBuiltRanks {2, 1, 3};
  const std::vector<std::string> kExpected {
      cpuAddressOf(1),
      cpuAddressOf(9),
      cpuAddressOf(4),
  };

  const auto first = record.toCounterSet(kWideTop);
  const auto second = record.toCounterSet(kWideTop);
  check(first == kExpected, "the first call answers the ascending ranks");
  check(second == first, "a second call answers an equal list");
  check(addressesOf(record) == kBuilt, "the record keeps its entry order");
  for (std::size_t index = 0; index < record.entries.size(); ++index) {
    const auto& kept = record.entries[index].rank;
    check(kept.has_value() && *kept == kBuiltRanks[index],
          "the record keeps the rank of every row");
  }
}

// ---------------------------------------------------------------------------
// The public surface of specs/040-hunter-sensitivity (T004, red; FR-024,
// D-1)
// ---------------------------------------------------------------------------

// A default `RankedEvent` answers the three rank figures of N14b empty
// (D-1, the record half of FR-030): the fields stand optional, so a run
// that measured none stands null on the record, and the console and the
// document read their `n/a` cells and null keys from that empty state.
auto surfaceDefaultFields() -> void
{
  const auto row = sg::hunt::RankedEvent {};
  check(!row.sensitivityCorrelation.has_value(),
        "a default record answers the sensitivity correlation empty");
  check(!row.sensitivitySlope.has_value(),
        "a default record answers the sensitivity slope empty");
  check(!row.delta.has_value(),
        "a default record answers the delta empty");
}

// `toCompareRules` declares beside `toCounterSet` (FR-024, D-1): the call
// takes `(std::size_t, double)` arguments of public types and binds the
// answer to `std::vector<std::string>`, so a `detail/` type in the
// signature breaks the compile of this call, which is the red the pair
// shape asks for. The record carries one unranked entry, so the empty
// answer is the pair rule's verdict and the grammar rows of the CLI wave
// pin the line texts.
auto surfaceCompareRules() -> void
{
  const auto record = []
  {
    auto hunt = sg::hunt::EntryHunt {};
    hunt.entries = {rankRow(4, std::nullopt)};
    return hunt;
  }();

  const std::vector<std::string> rules = record.toCompareRules(kWideTop, 10.0);
  check(rules.empty(), "a record with no ranked entry answers no rule line");
}

// ---------------------------------------------------------------------------
// The statistics rows (specs/040-hunter-sensitivity T010, red; FR-006;
// contracts/hunt-ranks-api.md B-2, B-3)
//
// The two helpers stand over one point set of plain pairs: the first
// coordinate carries the time per iteration, the second the event's rate
// (D-2, the point record of data-model.md). Every expectation below derives
// from the ranks and the sums the point set itself stands from. The figures
// the arithmetic carries exactly, the rank variance 42 of eight distinct
// ranks and the quarter-step sums of the slope sets, let the closed forms of
// the correlation and the slope answer one bit pattern, so the rows read
// their answers with no tolerance band (I-6).
// ---------------------------------------------------------------------------

// The strictly monotone set answers the correlation `1` (B-2): the rate
// rises with the time per iteration over eight points, the rank pairs stand
// identical, and the rank variance 42 of eight distinct ranks carries its
// square root exactly, so every closed form answers the same figure.
auto correlationMonotone() -> void
{
  std::vector<std::pair<double, double>> points;
  for (std::size_t step = 1; step <= 8; ++step) {
    points.push_back(
        {static_cast<double>(step), static_cast<double>(step)});
  }
  const auto answer = sg::detail::spearmanCorrelation(points);
  check(answer.has_value(), "a monotone set answers a correlation");
  check(sameDouble(*answer, 1.0), "the strictly monotone set answers 1");
}

// The anti-monotone set answers `-1` (B-2): the rate falls with the time
// per iteration over the same eight steps, the rank pairs stand reversed,
// and the figure carries the sign of the fall.
auto correlationAntiMonotone() -> void
{
  std::vector<std::pair<double, double>> points;
  for (std::size_t step = 1; step <= 8; ++step) {
    points.push_back(
        {static_cast<double>(step), static_cast<double>(9 - step)});
  }
  const auto answer = sg::detail::spearmanCorrelation(points);
  check(answer.has_value(), "an anti-monotone set answers a correlation");
  check(sameDouble(*answer, -1.0), "the anti-monotone set answers -1");
}

// The tie set averages the ranks (B-2): the two equal time figures carry
// the average of the ranks they span, 2.5 each, and the rank pairs of this
// set stand uncorrelated, so the correlation answers `0` exactly. The
// ordinal reading of the same set, the first tied figure ranked ahead of
// the second, answers one half, so the row separates the average ranks
// from an arbitrary tie-break.
auto correlationTieRanks() -> void
{
  const std::vector<std::pair<double, double>> points {
      {1.0, 2.0}, {2.0, 1.0}, {2.0, 3.0}};
  const auto answer = sg::detail::spearmanCorrelation(points);
  check(answer.has_value(), "a tied set answers a correlation");
  check(sameDouble(*answer, 0.0),
        "the tied time figures average their ranks");
  check(!sameDouble(*answer, 0.5),
        "the tied set does not read the ordinal ranks");
}

// A set with no spread in either coordinate answers nothing (B-2, FR-002):
// one constant column leaves no rank order to correlate, and the empty
// optional is the source of the `n/a` disclosure (A-2).
auto correlationNoSpread() -> void
{
  const std::vector<std::pair<double, double>> flatTime {
      {2.0, 1.0}, {2.0, 2.0}, {2.0, 3.0}};
  const std::vector<std::pair<double, double>> flatRate {
      {1.0, 2.0}, {2.0, 2.0}, {3.0, 2.0}};
  check(!sg::detail::spearmanCorrelation(flatTime).has_value(),
        "a constant time coordinate answers no correlation");
  check(!sg::detail::spearmanCorrelation(flatRate).has_value(),
        "a constant rate coordinate answers no correlation");
}

// The set under three points answers nothing (B-2, FR-002): two points
// stand below the count the contract correlates, whatever their figures.
auto correlationUnderThree() -> void
{
  const std::vector<std::pair<double, double>> points {
      {1.0, 1.0}, {2.0, 3.0}};
  check(!sg::detail::spearmanCorrelation(points).has_value(),
        "a set under three points answers no correlation");
}

// The exact line answers its slope (B-3): the four points stand on a rate
// of 2 times the time plus 3, and the least-squares slope of an exact line
// is the line's own slope. Every sum of this set is exactly representable,
// so the closed forms answer `2` on one bit pattern.
auto slopeExactLine() -> void
{
  const std::vector<std::pair<double, double>> points {
      {1.0, 3.0}, {2.0, 5.0}, {3.0, 7.0}, {4.0, 9.0}};
  const auto answer = sg::detail::leastSquaresSlope(points);
  check(answer.has_value(), "an exact line answers a slope");
  check(sameDouble(*answer, 2.0), "the exact line answers its slope");
}

// The slope reads the sums the complexity fit shares (B-3, FR-006, D-8):
// the quotient the four shared sums answer, the count times the cross sum
// minus the time sum times the rate sum, over the count times the square
// sum minus the time sum squared. This set stands off any line, so the
// figure separates the shared closed form from the through-origin quotient
// of the cross and square sums alone.
auto slopeSharedSums() -> void
{
  const std::vector<std::pair<double, double>> points {
      {1.0, 2.0}, {2.0, 1.0}, {3.0, 4.0}, {4.0, 3.0}};
  double cross = 0.0;
  double square = 0.0;
  double timeSum = 0.0;
  double rateSum = 0.0;
  for (const auto& [time, rate] : points) {
    cross += time * rate;
    square += time * time;
    timeSum += time;
    rateSum += rate;
  }
  const auto count = static_cast<double>(points.size());
  const auto expected = (count * cross - timeSum * rateSum)
      / (count * square - timeSum * timeSum);
  const auto answer = sg::detail::leastSquaresSlope(points);
  check(answer.has_value(), "the point set answers a slope");
  check(sameDouble(*answer, expected),
        "the slope reads the shared least-squares sums");
  check(!sameDouble(*answer, cross / square),
        "the slope is not the quotient of two sums alone");
}

// ---------------------------------------------------------------------------
// The family and stamp rows (specs/040-hunter-sensitivity T018, red;
// FR-001, FR-002; contracts/hunt-ranks-api.md B-4, D-2)
//
// The surface stands declared beside the includes; its definitions land
// with T022 and the link of this file answers the red until then. The
// rows run the surface over records built in place, the figures the
// fixture document `test/data/hunt_ranks/family.json` carries: the four
// entries `bmHuntFamily/args:1` to `args:4`, the aggregate times 100.0
// to 250.0 ns, the `event.alpha` rates 100.0 to 400.0 and the
// `event.beta` rates of the three entries that measured it. The case
// names stand in the spelling `deriveSuiteAndCase` answers, the args
// segment included, the form the family of D-2 spans over distinct
// args. The stamp rows derive the correlation and the slope from the
// standing helpers over the points the grouping assembled, the way the
// shared-summary row reads its figures (I-6), and the exact-line
// figures of the fixture carry one bit pattern, so the rows read their
// answers with no tolerance band.
// ---------------------------------------------------------------------------

// One family entry the rows read: the suite and the case name of the
// record, one aggregate row at the time per iteration given, and one
// hunt record per (address, rate) pair given. The shape is the shape
// the run's stamping pass reads (B-4).
[[nodiscard]] auto familyEntry(
    const std::string& suite,
    const std::string& caseName,
    const double timePerIteration,
    const std::vector<std::pair<std::string, double>>& events)
    -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = suite + "/" + caseName;
  result.suite = suite;
  result.caseName = caseName;
  auto aggregate = sg::ResultRow {};
  aggregate.kind = sg::RowKind::AGGREGATE;
  aggregate.timePerIterationNs = timePerIteration;
  result.rows.push_back(aggregate);
  auto hunt = sg::hunt::EntryHunt {};
  for (const auto& [address, rate] : events) {
    auto record = sg::hunt::RankedEvent {};
    record.address = address;
    record.rate = rate;
    hunt.entries.push_back(record);
  }
  result.hunt = hunt;
  return result;
}

// The four entries of the fixture document family.json, built in place
// in entry order.
[[nodiscard]] auto fixtureFamily() -> std::vector<sg::BenchmarkResult>
{
  return {familyEntry("bmHuntFamily",
                      "args:1",
                      100.0,
                      {{"event.alpha", 100.0}, {"event.beta", 150.0}}),
          familyEntry("bmHuntFamily", "args:2", 150.0, {{"event.alpha", 200.0}}),
          familyEntry("bmHuntFamily",
                      "args:3",
                      200.0,
                      {{"event.alpha", 300.0}, {"event.beta", 450.0}}),
          familyEntry("bmHuntFamily",
                      "args:4",
                      250.0,
                      {{"event.alpha", 400.0}, {"event.beta", 600.0}})};
}

// The points of one event address sorted by the time figure, the form
// the row comparisons read.
[[nodiscard]] auto pointsByTime(std::vector<std::pair<double, double>> points)
    -> std::vector<std::pair<double, double>>
{
  std::ranges::sort(points, std::less<double> {},
                    &std::pair<double, double>::first);
  return points;
}

// The entries of one suite and case name group stand one family (B-4,
// D-2): the fixture's four entries, whose case names carry the args the
// family spans, answer one family whose alpha points cover every entry
// and whose beta points stand over the three entries that measured the
// event.
auto familyGroupOneFamily() -> void
{
  const auto families = sg::detail::familyGroups(fixtureFamily());

  check(families.size() == 1,
        "the four entries of one family stand one family");
  check(families.front().events.size() == 2,
        "the family carries the two event addresses its entries measured");
  const auto& alpha = families.front().events.at("event.alpha");
  check(alpha.size() == 4,
        "every entry that measured the event stands one point");
  const std::vector<std::pair<double, double>> kAlphaPoints {
      {100.0, 100.0}, {150.0, 200.0}, {200.0, 300.0}, {250.0, 400.0}};
  check(pointsByTime(alpha) == kAlphaPoints,
        "an alpha point carries the aggregate time and the rate of one entry");
  const auto& beta = families.front().events.at("event.beta");
  check(beta.size() == 3,
        "the three entries that measured the event stand three points");
  const std::vector<std::pair<double, double>> kBetaPoints {
      {100.0, 150.0}, {200.0, 450.0}, {250.0, 600.0}};
  check(pointsByTime(beta) == kBetaPoints,
        "the beta points stand over the entries that measured the event");
}

// The grouping reads the suite and the case name (B-4): two cases of
// one suite stand two families, and two suites stand two families
// however their case names read.
auto familyGroupSuiteAndCase() -> void
{
  const std::vector<sg::BenchmarkResult> cases {
      familyEntry("bmFixture", "caseA/args:1", 100.0,
                  {{"event.alpha", 300.0}}),
      familyEntry("bmFixture", "caseB/args:1", 200.0,
                  {{"event.alpha", 500.0}}),
  };
  check(sg::detail::familyGroups(cases).size() == 2,
        "two cases of one suite stand two families");

  const std::vector<sg::BenchmarkResult> suites {
      familyEntry("bmOne", "args:1", 100.0, {{"event.alpha", 300.0}}),
      familyEntry("bmTwo", "args:1", 200.0, {{"event.alpha", 500.0}}),
  };
  const auto families = sg::detail::familyGroups(suites);
  check(families.size() == 2, "two suites stand two families");

  auto points = std::vector<std::pair<double, double>> {};
  for (const auto& family : families) {
    const auto found = family.events.find("event.alpha");
    check(found != family.events.end() && found->second.size() == 1,
          "each family carries the one point its one entry measured");
    points.push_back(found->second.front());
  }
  const std::vector<std::pair<double, double>> kTwoPoints {
      {100.0, 300.0}, {200.0, 500.0}};
  check(pointsByTime(points) == kTwoPoints,
        "each family carries its own entry's figures");
}

// The stamp answers the family's figures on every record that measured
// the event (FR-001, B-4): the alpha records of all four entries carry
// the alpha figures and the beta records the beta figures, each the
// figure the standing helpers answer over the points the grouping
// assembled, and the fixture's exact lines answer one bit pattern.
auto familyStampFigures() -> void
{
  const auto results = fixtureFamily();
  const auto families = sg::detail::familyGroups(results);
  const auto& alphaPoints = families.front().events.at("event.alpha");
  const auto& betaPoints = families.front().events.at("event.beta");
  const auto alphaCorrelation = sg::detail::spearmanCorrelation(alphaPoints);
  const auto alphaSlope = sg::detail::leastSquaresSlope(alphaPoints);
  const auto betaCorrelation = sg::detail::spearmanCorrelation(betaPoints);
  const auto betaSlope = sg::detail::leastSquaresSlope(betaPoints);
  check(alphaCorrelation.has_value() && alphaSlope.has_value(),
        "the alpha point set answers both figures");
  check(betaCorrelation.has_value() && betaSlope.has_value(),
        "the beta point set answers both figures");

  auto stamped = fixtureFamily();
  sg::detail::stampSensitivity(stamped);
  check(stamped.size() == results.size(),
        "the stamp keeps the entries the run measured");
  for (std::size_t index = 0; index < stamped.size(); ++index) {
    const auto& source = results[index].hunt->entries;
    auto& records = stamped[index].hunt->entries;
    check(source.size() == records.size(),
          "the stamp keeps the records of every entry");
    for (std::size_t row = 0; row < records.size(); ++row) {
      check(source[row].address == records[row].address,
            "the stamp keeps the address of every record");
      check(sameDouble(source[row].rate, records[row].rate),
            "the stamp changes no rate of any record");
      if (records[row].address == "event.alpha") {
        check(records[row].sensitivityCorrelation == alphaCorrelation,
              "the alpha record carries the family's correlation");
        check(records[row].sensitivitySlope == alphaSlope,
              "the alpha record carries the family's slope");
        check(sameDouble(*records[row].sensitivityCorrelation, 1.0),
              "the strictly monotone alpha points answer 1");
        check(sameDouble(*records[row].sensitivitySlope, 2.0),
              "the exact alpha line answers its slope 2");
      }
      if (records[row].address == "event.beta") {
        check(records[row].sensitivityCorrelation == betaCorrelation,
              "the beta record carries the family's correlation");
        check(records[row].sensitivitySlope == betaSlope,
              "the beta record carries the family's slope");
        check(sameDouble(*records[row].sensitivitySlope, 3.0),
              "the exact beta line answers its slope 3");
      }
    }
  }
}

// A family under 3 points leaves both sensitivity fields empty (FR-002,
// B-4): two entries of one family stand no figure on either record, and
// an event two of four entries measured stands no figure beside the
// filled alpha records of the same family.
auto familyStampUnderThree() -> void
{
  auto pair = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntFamily", "args:1", 100.0, {{"event.alpha", 100.0}}),
      familyEntry("bmHuntFamily", "args:2", 150.0, {{"event.alpha", 400.0}}),
  };
  sg::detail::stampSensitivity(pair);
  for (const auto& result : pair) {
    for (const auto& entry : result.hunt->entries) {
      check(!entry.sensitivityCorrelation.has_value()
                && !entry.sensitivitySlope.has_value(),
            "two points stand both sensitivity fields empty");
    }
  }

  auto four = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntFamily",
                  "args:1",
                  100.0,
                  {{"event.alpha", 100.0}, {"event.gamma", 150.0}}),
      familyEntry("bmHuntFamily",
                  "args:2",
                  150.0,
                  {{"event.alpha", 200.0}, {"event.gamma", 450.0}}),
      familyEntry("bmHuntFamily", "args:3", 200.0, {{"event.alpha", 300.0}}),
      familyEntry("bmHuntFamily", "args:4", 250.0, {{"event.alpha", 400.0}}),
  };
  sg::detail::stampSensitivity(four);
  for (const auto& result : four) {
    for (const auto& entry : result.hunt->entries) {
      if (entry.address == "event.gamma") {
        check(!entry.sensitivityCorrelation.has_value()
                  && !entry.sensitivitySlope.has_value(),
              "two measured points stand the gamma fields empty");
      }
      if (entry.address == "event.alpha") {
        check(entry.sensitivityCorrelation.has_value()
                  && entry.sensitivitySlope.has_value(),
              "the four alpha points stand filled beside the gamma empties");
      }
    }
  }
}

// A point set with no spread in either coordinate stands both
// sensitivity fields empty (FR-002, D-2): a family of four entries
// whose alpha rates stand equal, and one whose alpha times stand equal,
// each leave the fields of every alpha record empty.
auto familyStampNoSpread() -> void
{
  auto flatRates = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntFamily", "args:1", 100.0, {{"event.alpha", 300.0}}),
      familyEntry("bmHuntFamily", "args:2", 150.0, {{"event.alpha", 300.0}}),
      familyEntry("bmHuntFamily", "args:3", 200.0, {{"event.alpha", 300.0}}),
      familyEntry("bmHuntFamily", "args:4", 250.0, {{"event.alpha", 300.0}}),
  };
  sg::detail::stampSensitivity(flatRates);
  for (const auto& result : flatRates) {
    for (const auto& entry : result.hunt->entries) {
      check(!entry.sensitivityCorrelation.has_value()
                && !entry.sensitivitySlope.has_value(),
            "a constant rate stands both sensitivity fields empty");
    }
  }

  auto flatTimes = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntFamily", "args:1", 150.0, {{"event.alpha", 100.0}}),
      familyEntry("bmHuntFamily", "args:2", 150.0, {{"event.alpha", 200.0}}),
      familyEntry("bmHuntFamily", "args:3", 150.0, {{"event.alpha", 300.0}}),
      familyEntry("bmHuntFamily", "args:4", 150.0, {{"event.alpha", 400.0}}),
  };
  sg::detail::stampSensitivity(flatTimes);
  for (const auto& result : flatTimes) {
    for (const auto& entry : result.hunt->entries) {
      check(!entry.sensitivityCorrelation.has_value()
                && !entry.sensitivitySlope.has_value(),
            "a constant time stands both sensitivity fields empty");
    }
  }
}

// ---------------------------------------------------------------------------
// The baseline rows (specs/040-hunter-sensitivity T019, red; FR-003,
// FR-005; contracts/hunt-ranks-api.md B-5, D-6)
//
// The surface stands declared beside the includes; its definitions land
// with T023 in `source/harness/hunt_ranks.cpp` and the link of this file
// answers the red until then. The rows load the T001 fixtures of
// `test/data/hunt_ranks/` through `loadReport` and read the figures the
// documents carry, the way the family rows read theirs.
// ---------------------------------------------------------------------------

// The directory the T001 fixtures stand in, reached from the compiled
// path of this source file: the guard is the shape the
// `dataPath` helper of `harness_hunt_json_test.cpp` runs, the schema
// tail swapped for this file's own.
[[nodiscard]] auto huntRanksFixturePath(const std::string& name)
    -> std::string
{
  constexpr std::string_view kSourceTail =
      "/test/source/harness_hunt_rank_test.cpp";
  const std::string_view file = __FILE__;
  check(file.size() > kSourceTail.size() && file.ends_with(kSourceTail),
        "the compiled source path stands in the source tree");
  const auto directory =
      std::filesystem::path(file.substr(0, file.size() - kSourceTail.size()))
      / "test" / "data" / "hunt_ranks";
  return (directory / name).string();
}

// One loaded fixture document: the value `loadReport` answers for the
// fixture `name`. A document the reader refuses fails the row at the
// check above.
[[nodiscard]] auto loadHuntRanksFixture(const std::string& name)
    -> sg::ReportData
{
  auto loaded = sg::loadReport(huntRanksFixturePath(name));
  if (!loaded.has_value()) {
    std::fprintf(stderr, "FIXTURE LOAD ERROR [%s]: %s\n", name.c_str(),
                 loaded.error().c_str());
  }
  check(loaded.has_value(), "the fixture loads through loadReport");
  return std::move(loaded).value();
}

// The extraction reads the matched entry's hunt rates by catalog
// address (B-5, FR-003): the three rates of the rates fixture stand the
// document's own figures, and the match runs by the full instance name
// the compare mode pairs under (R-2).
auto baselineRatesByAddress() -> void
{
  const auto baseline = loadHuntRanksFixture("baseline_rates.json");
  check(baseline.results.size() == 1,
        "the document carries one benchmark");
  const auto& matched = baseline.results.front();
  check(matched.hunt.has_value(),
        "the matched entry carries a hunt object");
  const auto rates = sg::detail::baselineHuntRates(baseline, matched.name);
  check(rates.size() == 3,
        "the extraction reads every hunt entry the match carries");
  check(sameDouble(rates.at("event.alpha"), 1000.0),
        "the alpha rate stands the document's own figure");
  check(sameDouble(rates.at("event.beta"), 2000.0),
        "the beta rate stands the document's own figure");
  check(sameDouble(rates.at("event.gamma"), 500.0),
        "the gamma rate stands the document's own figure");
}

// A matched entry whose hunt object stands absent answers an empty map
// (B-5, D-5): the document carries no rate for the extraction to read,
// the shape the delta disclosure of B-6 reads.
auto baselineRatesNoHunt() -> void
{
  const auto baseline = loadHuntRanksFixture("baseline_no_entry_hunt.json");
  check(baseline.results.size() == 1,
        "the document carries one benchmark");
  check(!baseline.results.front().hunt.has_value(),
        "the matched entry carries no hunt object");
  const auto rates = sg::detail::baselineHuntRates(baseline, "bmHuntRanks");
  check(rates.empty(), "no hunt object leaves no rate to read");
}

// A rate at zero stands read at its address (B-5, D-5): the figure the
// delta stamp of B-6 leaves the field empty on, read here as the value
// the document carries.
auto baselineRatesZeroRate() -> void
{
  const auto baseline = loadHuntRanksFixture("baseline_zero_rate.json");
  const auto rates = sg::detail::baselineHuntRates(baseline, "bmHuntRanks");
  check(rates.size() == 2,
        "the extraction reads both hunt entries the match carries");
  check(sameDouble(rates.at("event.zero"), 0.0),
        "the rate at zero stands the document's own figure");
  check(sameDouble(rates.at("event.alpha"), 1000.0),
        "the alpha rate stands the document's own figure");
}

// The norm-pair check answers the mismatch text the refusal prints
// (B-5, FR-005): a baseline whose top-level hunt object names `cycle`
// against a run that settled `instr` answers one line naming the
// mismatch and both words, the pair that names the settled norm
// answers empty, and the rates fixture pairs with the run that settled
// its own norm.
auto baselineNormMismatchText() -> void
{
  const auto mismatch = loadHuntRanksFixture("baseline_norm_mismatch.json");
  check(mismatch.hunt.has_value()
            && mismatch.hunt->options.norm.has_value()
            && *mismatch.hunt->options.norm == sg::hunt::HuntNorm::CYCLE,
        "the fixture's top-level hunt object names the cycle norm");
  const auto text =
      sg::detail::baselineNormMismatch(mismatch, sg::hunt::HuntNorm::INSTR);
  check(text.has_value(), "the mismatched pair answers one line");
  check(text->find("cycle") != std::string::npos
            && text->find("instr") != std::string::npos,
        "the mismatch line names both norms the pair carries");
  check(!sg::detail::baselineNormMismatch(mismatch, sg::hunt::HuntNorm::CYCLE)
             .has_value(),
        "the pair that names the settled norm answers no mismatch");

  const auto rates = loadHuntRanksFixture("baseline_rates.json");
  check(!sg::detail::baselineNormMismatch(rates, sg::hunt::HuntNorm::INSTR)
             .has_value(),
        "the rates fixture pairs with the run that settled instr");
}

// ---------------------------------------------------------------------------
// The delta rows (specs/040-hunter-sensitivity T020, red; FR-004;
// contracts/hunt-ranks-api.md B-6, A-5, D-5)
//
// The stamp stands declared beside the includes; its definition lands
// with T024 in `source/harness/hunt_ranks.cpp` and the link of this
// file answers the red until then. The rows reuse the extraction of
// T019 over the same T001 fixtures: the baseline side stands the map
// `baselineHuntRates` answers, and the contender side stands built in
// place the way the family rows build theirs.
// ---------------------------------------------------------------------------

// The matched pair stands the signed percent of A-5 (B-6, FR-003): the
// alpha record stands above its baseline rate, the beta and the gamma
// records stand below theirs, each delta the formula answers over the
// two figures the pair carries, and every record of the matched
// address the run's entries hold carries the figure.
auto deltaStampMatchedPair() -> void
{
  const auto document = loadHuntRanksFixture("baseline_rates.json");
  const auto baseline = sg::detail::baselineHuntRates(document, "bmHuntRanks");
  check(baseline.size() == 3,
        "the extraction reads the three rates the match carries");

  auto stamped = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntRanks",
                  "args:1",
                  100.0,
                  {{"event.alpha", 1500.0},
                   {"event.beta", 1000.0},
                   {"event.gamma", 750.0}}),
      familyEntry("bmHuntRanks",
                  "args:2",
                  150.0,
                  {{"event.alpha", 1500.0}, {"event.gamma", 750.0}}),
  };
  sg::detail::stampDelta(stamped, baseline);

  for (const auto& result : stamped) {
    for (const auto& record : result.hunt->entries) {
      const double baselineRate = baseline.at(record.address);
      check(record.delta.has_value(),
            "the matched record carries the delta the pair answers");
      check(sameDouble(*record.delta,
                       kPercentScale * (record.rate - baselineRate)
                           / baselineRate),
            "the delta stands the signed percent of A-5");
    }
  }
  const std::string kAlpha {"event.alpha"};
  const std::string kBeta {"event.beta"};
  check(*entryOf(stamped.front().hunt.value(), kAlpha).delta > 0.0,
        "the contender above its baseline stands a positive delta");
  check(*entryOf(stamped.front().hunt.value(), kBeta).delta < 0.0,
        "the contender below its baseline stands a negative delta");
}

// A figure the match does not find stands the disclosure (B-6, D-5):
// the contender address the baseline holds none of leaves its field
// empty, the baseline address no contender record carries fills
// nothing, and the call changes no other field of any record.
auto deltaStampMissingMatch() -> void
{
  const auto document = loadHuntRanksFixture("baseline_rates.json");
  const auto baseline = sg::detail::baselineHuntRates(document, "bmHuntRanks");

  const auto source = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntRanks",
                  "args:1",
                  100.0,
                  {{"event.alpha", 1500.0}, {"event.other", 400.0}}),
  };
  auto stamped = source;
  auto widened = baseline;
  widened["event.lonely"] = 900.0;
  sg::detail::stampDelta(stamped, widened);

  const double kBaselineAlpha = baseline.at("event.alpha");
  const std::string kAlpha {"event.alpha"};
  const std::string kOther {"event.other"};
  const auto& other = entryOf(stamped.front().hunt.value(), kOther);
  check(!other.delta.has_value(),
        "the contender address the baseline holds none of stands empty");
  const auto& alpha = entryOf(stamped.front().hunt.value(), kAlpha);
  check(alpha.delta.has_value()
            && sameDouble(*alpha.delta,
                          kPercentScale * (1500.0 - kBaselineAlpha)
                              / kBaselineAlpha),
        "the matched address stands its delta beside the missing one");
  for (std::size_t index = 0; index < stamped.size(); ++index) {
    const auto& kept = source[index].hunt->entries;
    auto& records = stamped[index].hunt->entries;
    check(kept.size() == records.size(),
          "the stamp keeps the records of every entry");
    for (std::size_t row = 0; row < records.size(); ++row) {
      check(kept[row].address == records[row].address,
            "the stamp keeps the address of every record");
      check(sameDouble(kept[row].rate, records[row].rate),
            "the stamp changes no rate of any record");
    }
  }
  check(stamped.front().hunt->entries.size() == 2,
        "the baseline address no record carries adds none");
}

// The baseline side's missing figures stand the disclosure (B-6, D-5):
// an extraction that answers no rate, the answer a matched entry
// without a hunt object produces, leaves every delta empty, and a
// baseline rate at or below zero leaves the delta of its record empty.
auto deltaStampBaselineGaps() -> void
{
  const auto noHunt = loadHuntRanksFixture("baseline_no_entry_hunt.json");
  const auto empty = sg::detail::baselineHuntRates(noHunt, "bmHuntRanks");
  check(empty.empty(), "the extraction answers no rate for the match");
  auto stamped = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntRanks", "args:1", 100.0, {{"event.alpha", 1500.0}}),
  };
  sg::detail::stampDelta(stamped, empty);
  for (const auto& result : stamped) {
    for (const auto& record : result.hunt->entries) {
      check(!record.delta.has_value(),
            "an empty baseline answer stands every delta empty");
    }
  }

  const auto zeroed = loadHuntRanksFixture("baseline_zero_rate.json");
  const auto baseline = sg::detail::baselineHuntRates(zeroed, "bmHuntRanks");
  auto paired = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntRanks",
                  "args:1",
                  100.0,
                  {{"event.alpha", 1500.0}, {"event.zero", 300.0}}),
  };
  sg::detail::stampDelta(paired, baseline);
  const double kBaselineAlpha = baseline.at("event.alpha");
  const std::string kAlpha {"event.alpha"};
  const std::string kZero {"event.zero"};
  const auto& alpha = entryOf(paired.front().hunt.value(), kAlpha);
  check(alpha.delta.has_value()
            && sameDouble(*alpha.delta,
                          kPercentScale * (1500.0 - kBaselineAlpha)
                              / kBaselineAlpha),
        "the pair at a positive baseline rate stands its delta");
  const auto& zero = entryOf(paired.front().hunt.value(), kZero);
  check(!zero.delta.has_value(),
        "the record at a baseline rate of zero stands its delta empty");

  auto below = std::vector<sg::BenchmarkResult> {
      familyEntry("bmHuntRanks", "args:1", 100.0, {{"event.alpha", 1500.0}}),
  };
  const std::map<std::string, double> negative {{"event.alpha", -500.0}};
  sg::detail::stampDelta(below, negative);
  check(!entryOf(below.front().hunt.value(), kAlpha).delta.has_value(),
        "the record at a negative baseline rate stands its delta empty");
}

// ---------------------------------------------------------------------------
// The rank rows (specs/040-hunter-sensitivity T021, T025; FR-011;
// contracts/hunt-ranks-api.md B-7, A-3, A-4, D-3)
//
// The header declares the stamp and `source/harness/hunt_ranks.cpp`
// defines it (T025). The rows run the rank stamp over
// records built in place, the figures set on the records the way the
// stamping pass of B-7 leaves them, and read the ranks the stamp
// assigns. The standing `rankEvents` keeps its `rate` and `cv` keys,
// so the rows pin the batch pass that ranks under the two new keys and
// no change inside `rankEvents`.
// ---------------------------------------------------------------------------

// The nominal aggregate time of a rank-row entry: the batch pass reads
// no time figure, so one value stands the row the entry shape carries.
constexpr double kRankRowTime = 100.0;

// One record of the rank rows: the address and the figures the record
// carries, each field left empty when the case names none.
struct RankCase
{
  std::string address;
  std::optional<double> correlation;
  std::optional<double> slope;
  std::optional<double> delta;
  bool noisy = false;
};

// One entry of the rank rows: one aggregate row at the nominal time,
// and one record per case given, the figures the case names set on it.
// The shape is the shape the batch pass reads (B-7).
[[nodiscard]] auto rankEntry(const std::string& suite,
                             const std::string& caseName,
                             const std::vector<RankCase>& cases)
    -> sg::BenchmarkResult
{
  auto result = familyEntry(suite, caseName, kRankRowTime, {});
  auto& records = result.hunt.value().entries;
  for (const auto& figure : cases) {
    auto record = sg::hunt::RankedEvent {};
    record.address = figure.address;
    record.sensitivityCorrelation = figure.correlation;
    record.sensitivitySlope = figure.slope;
    record.delta = figure.delta;
    record.noisy = figure.noisy;
    records.push_back(record);
  }
  return result;
}

// The `sensitivity` key orders by the correlation descending, ties by
// the slope and then the catalog address (B-7, D-3): two records carry
// one correlation and one slope at two addresses, a third tie carries
// the same correlation at a lower slope, and the lowest correlation
// carries the highest slope, so neither the slope alone nor the
// address order answers the table. The address both entries measured
// carries its rank on both records, and the record that carries only
// the delta figure stands unranked (A-4).
auto rankSensitivityOrder() -> void
{
  const std::string kAlpha {"event.alpha"};
  const std::string kBeta {"event.beta"};
  const std::string kGamma {"event.gamma"};
  const std::string kTheta {"event.theta"};
  const std::string kIota {"event.iota"};
  const std::string kKappa {"event.kappa"};

  const auto results = std::vector<sg::BenchmarkResult> {
      rankEntry("bmHuntRank",
                "args:1",
                {RankCase {kAlpha, 0.9, 1.0, std::nullopt},
                 RankCase {kBeta, 0.5, 2.0, std::nullopt},
                 RankCase {kGamma, 0.5, 2.0, std::nullopt},
                 RankCase {kTheta, 0.5, 1.0, std::nullopt},
                 RankCase {kIota, -0.5, 9.0, std::nullopt},
                 RankCase {kKappa, std::nullopt, std::nullopt, 40.0}}),
      rankEntry("bmHuntRank",
                "args:2",
                {RankCase {kAlpha, 0.9, 1.0, std::nullopt},
                 RankCase {kKappa, std::nullopt, std::nullopt, 40.0}}),
  };
  auto stamped = results;
  sg::detail::stampRank(stamped, sg::hunt::HuntRank::SENSITIVITY);

  for (const auto& result : stamped) {
    const auto& alpha = entryOf(result.hunt.value(), kAlpha);
    check(alpha.rank.has_value() && *alpha.rank == 1,
          "every record of the top address carries the first rank");
  }
  const std::vector<std::string> kOrder {kBeta, kGamma, kTheta, kIota};
  const std::vector<std::size_t> kRanks {2, 3, 4, 5};
  for (std::size_t index = 0; index < kOrder.size(); ++index) {
    const auto& rank =
        entryOf(stamped.front().hunt.value(), kOrder[index]).rank;
    check(rank.has_value() && *rank == kRanks[index],
          "the sensitivity order ranks the correlations descending, "
          "ties by the slope and then the address");
  }
  check(!entryOf(stamped.front().hunt.value(), kKappa).rank.has_value(),
        "the record that carries no correlation stands unranked");
  check(*entryOf(stamped.front().hunt.value(), kBeta).rank == 2
            && *entryOf(stamped.front().hunt.value(), kGamma).rank == 3,
        "the tie on one correlation and one slope breaks by the address");
  check(*entryOf(stamped.front().hunt.value(), kTheta).rank == 4,
        "the tie on one correlation alone breaks by the slope");
  check(*entryOf(stamped.front().hunt.value(), kIota).rank == 5,
        "the lowest correlation ranks last beside the highest slope");
  check(sameDouble(*entryOf(stamped.front().hunt.value(), kAlpha)
                       .sensitivityCorrelation,
                   0.9)
            && sameDouble(*entryOf(stamped.back().hunt.value(), kKappa).delta,
                          40.0),
        "the stamp changes no figure of any record");
}

// The `delta` key orders by the magnitude of the delta descending,
// ties by the catalog address (B-7, D-3): the largest magnitude
// carries a negative delta, so the sign stands beside the magnitude
// and the order reads the magnitude alone, and the tie pair carries
// one magnitude at two addresses under opposite signs. The record
// that carries only the sensitivity figures stands unranked (A-4).
auto rankDeltaOrder() -> void
{
  const std::string kDrop {"event.drop"};
  const std::string kRise {"event.rise"};
  const std::string kTieFirst {"event.alpha"};
  const std::string kTieSecond {"event.beta"};
  const std::string kQuiet {"event.quiet"};

  const auto results = std::vector<sg::BenchmarkResult> {
      rankEntry("bmHuntRank",
                "args:1",
                {RankCase {kDrop, std::nullopt, std::nullopt, -75.0},
                 RankCase {kRise, std::nullopt, std::nullopt, 50.0},
                 RankCase {kTieFirst, 0.8, 2.0, 25.0},
                 RankCase {kTieSecond, std::nullopt, std::nullopt, -25.0},
                 RankCase {kQuiet, 0.8, 2.0, std::nullopt}}),
  };
  auto stamped = results;
  sg::detail::stampRank(stamped, sg::hunt::HuntRank::DELTA);

  const std::vector<std::string> kOrder {kDrop, kRise, kTieFirst, kTieSecond};
  const std::vector<std::size_t> kRanks {1, 2, 3, 4};
  for (std::size_t index = 0; index < kOrder.size(); ++index) {
    const auto& rank =
        entryOf(stamped.front().hunt.value(), kOrder[index]).rank;
    check(rank.has_value() && *rank == kRanks[index],
          "the delta order ranks the magnitudes descending");
  }
  check(!entryOf(stamped.front().hunt.value(), kQuiet).rank.has_value(),
        "the record that carries no delta stands unranked");
  check(*entryOf(stamped.front().hunt.value(), kTieFirst).rank == 3
            && *entryOf(stamped.front().hunt.value(), kTieSecond).rank == 4,
        "the tie on one magnitude breaks by the address");
  check(sameDouble(*entryOf(stamped.front().hunt.value(), kDrop).delta, -75.0),
        "the stamp changes no figure of any record");
}

// A noisy event stands unranked under both keys (A-3): the loud
// record carries the top figure of either key, and both calls leave
// its rank empty while the clean records carry the ranks 1 and 2 with
// no gap, so the noisy event stands out of the ranked set entirely.
auto rankNoisyUnranked() -> void
{
  const std::string kLoud {"event.loud"};
  const std::string kFirst {"event.first"};
  const std::string kSecond {"event.second"};

  const auto results = std::vector<sg::BenchmarkResult> {
      rankEntry("bmHuntRank",
                "args:1",
                {RankCase {kLoud, 0.99, 9.0, 90.0, true},
                 RankCase {kFirst, 0.5, 1.0, 10.0},
                 RankCase {kSecond, 0.1, 1.0, 5.0}}),
  };

  auto bySensitivity = results;
  sg::detail::stampRank(bySensitivity, sg::hunt::HuntRank::SENSITIVITY);
  auto byDelta = results;
  sg::detail::stampRank(byDelta, sg::hunt::HuntRank::DELTA);

  for (const auto& stamped : {bySensitivity, byDelta}) {
    const auto& entries = stamped.front().hunt->entries;
    check(entries.size() == 3, "the stamp keeps the records of the entry");
    check(!entryOf(stamped.front().hunt.value(), kLoud).rank.has_value(),
          "a noisy event stands unranked beside the top figure");
    const auto& first = entryOf(stamped.front().hunt.value(), kFirst).rank;
    const auto& second = entryOf(stamped.front().hunt.value(), kSecond).rank;
    check(first.has_value() && *first == 1 && second.has_value()
              && *second == 2,
          "the clean records carry the ranks 1 to 2 with no gap");
  }
}

// An event set that stands empty under the key answers the standing
// order with empty rank fields (B-7): no record carries a figure of
// either key, so both calls rank nothing and reorder nothing, the
// shape the console prints in the standing order with exit 0.
auto rankEmptySet() -> void
{
  const std::string kAlpha {"event.alpha"};
  const std::string kBeta {"event.beta"};
  const std::string kGamma {"event.gamma"};

  const auto results = std::vector<sg::BenchmarkResult> {
      rankEntry("bmHuntRank",
                "args:1",
                {RankCase {kAlpha, std::nullopt, std::nullopt, std::nullopt},
                 RankCase {kBeta, std::nullopt, std::nullopt, std::nullopt}}),
      rankEntry("bmHuntRank",
                "args:2",
                {RankCase {kGamma, std::nullopt, std::nullopt, std::nullopt}}),
  };

  auto bySensitivity = results;
  sg::detail::stampRank(bySensitivity, sg::hunt::HuntRank::SENSITIVITY);
  auto byDelta = results;
  sg::detail::stampRank(byDelta, sg::hunt::HuntRank::DELTA);

  for (const auto& stamped : {bySensitivity, byDelta}) {
    for (std::size_t index = 0; index < results.size(); ++index) {
      check(addressesOf(stamped[index].hunt.value())
                == addressesOf(results[index].hunt.value()),
            "the empty set stands the entries in the order they hold");
      for (const auto& record : stamped[index].hunt->entries) {
        check(!record.rank.has_value(),
              "no record of an empty set carries a rank");
      }
    }
  }
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"norm-instr", normInstr},
      Scenario {"norm-cycle", normCycle},
      Scenario {"norm-iter", normIter},
      Scenario {"zero-anchor-excluded", zeroAnchorExcluded},
      Scenario {"zero-divisor-excluded", zeroDivisorExcluded},
      Scenario {"disclosure-excluded", disclosureExcluded},
      Scenario {"statistics-agreement", statisticsAgreement},
      Scenario {"noise-threshold", noiseThreshold},
      Scenario {"rank-rate-order", rankRateOrder},
      Scenario {"rank-cv-order", rankCvOrder},
      Scenario {"top-limit", topLimit},
      Scenario {"single-sample", singleSample},
      Scenario {"multiplexed-mark", multiplexedMark},
      Scenario {"multiplexed-method", multiplexedMethod},
      Scenario {"entry-order", entryOrder},
      Scenario {"pure-repeat", pureRepeat},
      Scenario {"counter-set-rank-order", counterSetRankOrder},
      Scenario {"counter-set-cap", counterSetCap},
      Scenario {"counter-set-null-rank", counterSetNullRank},
      Scenario {"counter-set-zero-top", counterSetZeroTop},
      Scenario {"counter-set-above-count", counterSetAboveCount},
      Scenario {"counter-set-repeat", counterSetRepeat},
      Scenario {"surface-default-fields", surfaceDefaultFields},
      Scenario {"surface-compare-rules", surfaceCompareRules},
      Scenario {"correlation-monotone", correlationMonotone},
      Scenario {"correlation-anti-monotone", correlationAntiMonotone},
      Scenario {"correlation-tie-ranks", correlationTieRanks},
      Scenario {"correlation-no-spread", correlationNoSpread},
      Scenario {"correlation-under-three", correlationUnderThree},
      Scenario {"slope-exact-line", slopeExactLine},
      Scenario {"slope-shared-sums", slopeSharedSums},
      Scenario {"family-group-one-family", familyGroupOneFamily},
      Scenario {"family-group-suite-and-case", familyGroupSuiteAndCase},
      Scenario {"family-stamp-figures", familyStampFigures},
      Scenario {"family-stamp-under-three", familyStampUnderThree},
      Scenario {"family-stamp-no-spread", familyStampNoSpread},
      Scenario {"baseline-rates-by-address", baselineRatesByAddress},
      Scenario {"baseline-rates-no-hunt", baselineRatesNoHunt},
      Scenario {"baseline-rates-zero-rate", baselineRatesZeroRate},
      Scenario {"baseline-norm-mismatch-text", baselineNormMismatchText},
      Scenario {"delta-stamp-matched-pair", deltaStampMatchedPair},
      Scenario {"delta-stamp-missing-match", deltaStampMissingMatch},
      Scenario {"delta-stamp-baseline-gaps", deltaStampBaselineGaps},
      Scenario {"rank-sensitivity-order", rankSensitivityOrder},
      Scenario {"rank-delta-order", rankDeltaOrder},
      Scenario {"rank-noisy-unranked", rankNoisyUnranked},
      Scenario {"rank-empty-set", rankEmptySet},
  };
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const auto& list = scenarios();
  // One name selects one scenario through `argv[1]`, so a name that stands
  // twice would hide the scenario it repeats.
  for (std::size_t first = 0; first < list.size(); ++first) {
    check(list[first].body != nullptr, "every scenario carries a body");
    for (std::size_t second = first + 1; second < list.size(); ++second) {
      check(list[first].name != list[second].name,
            "no two scenarios of this file carry the same name");
    }
  }
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : list) {
      if (scenario.name == mode) {
        scenario.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : list) {
    scenario.body();
  }
  std::printf("harness_hunt_rank_test: ok\n");
  return 0;
}
