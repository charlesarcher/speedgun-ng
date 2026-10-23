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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "detail/hunt_fixture.hpp"
#include "detail/statistics.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/hunt.hpp"

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
// contributes no rate at all: the cycle norm over a window with no cycle
// reading and over one whose cycle reading is 0, and the iter norm over a
// window that counted no iteration. Every excluded window carries an event
// figure that would move the median and the deviation, so no exclusion here
// can pass by coincidence (E-08, contracts/hunt-api.md).
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
