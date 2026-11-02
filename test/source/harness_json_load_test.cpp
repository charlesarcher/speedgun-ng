// ============================================================================
// T022, T023, T024, T026: the format closes on itself (specs/023-json-reports;
// FR-010, FR-011, FR-022, FR-023, FR-026, SC-002, SC-010).
//
// One scripted run writes a document; the reader answers the run's own
// values field by field, and the writer over the loaded values answers the
// same bytes. The same holds with the chunk columns taken out, which is
// the raw setting at `none`. The version rule then gates the reader: a
// later minor loads, a later major is refused naming its version, and a
// document whose version stands missing or unspelled is refused.
// The reader extension of specs/026 (T010) then adds the three fields of
// E-07: the two header texts the document carries, and the noise record
// of each entry stored beside the result it warns of. Each error code the
// extension can answer carries its own plant. The unknown-key rule of
// specs/026 (T047) then shows a key the node table does not list, inside
// an object the schema leaves open, loading with no fault.
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cctype>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "detail/json_fixture.hpp"

using sg::test::check;

namespace

{
constexpr const char* const kWritten = "harness_json_load_written.json";
constexpr const char* const kAgain = "harness_json_load_again.json";

auto roundTripClosesTheFormat() -> void
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  options.showPercentiles = true;
  options.rawContent = sg::RawContent::CHUNKS;
  const auto run = sg::test::scriptedRun(options);

  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  check(sg::detail::writeReport(kWritten, data),
        "the writer wrote the document (FR-001)");
  const std::string first = sg::test::readText(kWritten);

  const auto loaded = sg::loadReport(kWritten);
  check(loaded.has_value(), "the reader read the written document (FR-022)");
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    return;
  }
  check(sg::test::sameReport(*loaded, data),
        "each loaded value answers the run that wrote it (FR-022, SC-010)");

  check(sg::detail::writeReport(kAgain, *loaded),
        "the writer wrote the loaded values again (FR-011)");
  check(sg::test::readText(kAgain) == first,
        "the second writing is byte for byte the first (FR-011, SC-002)");

  sg::ReportData bare = *loaded;
  for (auto& rows : bare.rowRaw) {
    for (auto& row : rows) {
      row.columns.clear();
    }
  }
  for (auto& rows : bare.contextRaw) {
    for (auto& contexts : rows) {
      for (auto& context : contexts) {
        context.columns.clear();
      }
    }
  }
  check(sg::detail::writeReport("harness_json_load_bare.json", bare),
        "the writer wrote the same values with no chunk columns");
  const auto reloaded = sg::loadReport("harness_json_load_bare.json");
  check(reloaded.has_value() && sg::test::sameReport(*reloaded, bare),
        "a document with no chunk columns answers the same values (FR-026)");
  check(sg::detail::writeReport("harness_json_load_bare2.json", *reloaded),
        "the bare document wrote again");
  check(sg::test::readText("harness_json_load_bare2.json")
            == sg::test::readText("harness_json_load_bare.json"),
        "the bare document round-trips byte for byte (FR-011, SC-002)");
}

auto versionRuleGatesTheReader() -> void
{
  const std::string text = sg::test::readText(kWritten);
  check(!text.empty(), "the written file stands where the test left it");

  // The read takes the extent of the value the document stamps, so a minor
  // of any width stands replaced.
  const auto withVersion = [&text](const std::string& value)
  {
    std::string mutated = text;
    const auto start = mutated.find("\"schemaVersion\":") + 16;
    mutated.replace(start, mutated.find('"', start + 1) + 1 - start, value);
    return mutated;
  };

  sg::test::writeText("harness_json_load_minor.json", withVersion("\"1.13\""));
  check(sg::loadReport("harness_json_load_minor.json").has_value(),
        "a later minor of the same major loads (FR-010)");

  sg::test::writeText("harness_json_load_major.json", withVersion("\"2.0\""));
  const auto refused = sg::loadReport("harness_json_load_major.json");
  check(
      !refused.has_value() && refused.error().find("2.0") != std::string::npos,
      "a later major is refused, naming the version it carries (FR-023)");

  sg::test::writeText("harness_json_load_current.json", withVersion("\"1.4\""));
  check(sg::loadReport("harness_json_load_current.json").has_value(),
        "the version this build writes loads (FR-010)");

  // Three texts state no major, each for its own reason: "1" carries no dot,
  // "1." leaves nothing after the dot it carries, ".1" leaves nothing before
  // it. The refusal names the text it was given, so the three read as three
  // checks and not one (specs/026 D-13).
  struct NoMajorRow
  {
    const char* const value;
    const char* const file;
    const char* const marker;
  };

  static constexpr NoMajorRow kNoMajor[] {
      {"\"1\"", "harness_json_load_bare.json", "carries schema version '1'"},
      {"\"1.\"",
       "harness_json_load_dotlast.json",
       "carries schema version '1.'"},
      {"\".1\"",
       "harness_json_load_dotfirst.json",
       "carries schema version '.1'"},
  };
  for (const NoMajorRow& row : kNoMajor) {
    sg::test::writeText(row.file, withVersion(row.value));
    const auto missing = sg::loadReport(row.file);
    check(!missing.has_value()
              && missing.error().find(row.marker) != std::string::npos,
          "a version that states no major is refused, naming that version "
          "(FR-023, specs/026 D-13)");
  }

  sg::test::writeText("harness_json_load_number.json", withVersion("1"));
  check(!sg::loadReport("harness_json_load_number.json").has_value(),
        "a version that is not a string is refused (FR-023)");

  std::string noVersion = text;
  const auto speedgunAt = noVersion.find(",\"speedgun\"");
  noVersion.erase(1, speedgunAt);
  sg::test::writeText("harness_json_load_nokey.json", noVersion);
  check(!sg::loadReport("harness_json_load_nokey.json").has_value(),
        "a document whose version key stands missing is refused (FR-023)");

  sg::test::writeText("harness_json_load_broken.json",
                      "{\"schemaVersion\":\"1.0\"");
  check(!sg::loadReport("harness_json_load_broken.json").has_value(),
        "a file that holds no JSON is refused (FR-024)");

  check(!sg::loadReport("harness_json_load_absent.json").has_value(),
        "a file that stands absent is refused (FR-024)");

  const auto unreadable = sg::loadReport("/proc/self/mem");
  check(!unreadable.has_value()
            && unreadable.error().find("cannot be read") != std::string::npos,
        "a file that opens but cannot be read answers its refusal (FR-024)");
}

// Every optional field the format carries, in one document the writer
// never produces: a skipped entry with two contexts and a noise warning,
// an aggregate row with a label, counters, a disclosure, a fit, three
// statistics records and the two context-level blocks (FR-012 to FR-017,
// FR-025, FR-026, SC-010).
constexpr const char* const kRich = R"JSON(
{"schemaVersion":"1.0","speedgun":{"version":"0.12.0","buildType":"Release"},
 "jitter":{"mode":"ALONGSIDE","thresholdNs":200,"loopPeriodNs":20,
  "cpus":[2],"stats":[{"cpu":2,"gapCount":1,"gapsPerSecond":1000.0,"minNs":200,"medianNs":200,"meanNs":200.0,"p90Ns":210,"p99Ns":220,"p999Ns":230,"p9999Ns":240,"maxNs":250,"totalNs":260,"totalPercent":0.5,"runtimeNs":270,"thresholdNs":280,"loopPeriodNs":29,"droppedGaps":30,"gaps":[{"stamp":100,"gapNs":200}]}],
  "droppedGaps":0,"disclosure":"the run's disclosure"},
 "benchmarks":[{"name":"bmRich","suite":"bmRich","case":"bmRich",
  "outcome":"SKIPPED","reason":"the scripted skip","hideRepetitionRows":true,
  "timeUnit":"MILLISECONDS","primaryIsCpu":false,"pauseSamplesFolded":true,
  "percentilePoints":[50.0],
  "contexts":[{"waitMode":"SPIN","placement":"l3 group 0","applied":true},
              {"waitMode":"BLOCK","placement":"unpinned","applied":false}],
  "noiseWarning":{"spinning":3,"runnableCpus":16,"sharesOneCpu":true},
  "jitter":{"stats":[{"cpu":3,"gapCount":4,"gapsPerSecond":400.0,"minNs":300,"medianNs":310,"meanNs":320.0,"p90Ns":330,"p99Ns":340,"p999Ns":350,"p9999Ns":360,"maxNs":370,"totalNs":380,"totalPercent":0.6,"runtimeNs":390,"thresholdNs":400,"loopPeriodNs":41,"droppedGaps":42}]},
  "rows":[{"kind":"AGGREGATE","label":"aggregate","iterations":6,
    "timePerIterationNs":1.5,"cpuTimePerIterationNs":2.5,"endpointSamples":7,
    "pauseSamplesFolded":true,"timeAvailability":"GAP",
    "metrics":[{"label":"work","value":2.5,"runningRatio":1.0,
                "scaled":true,"availability":"COUNTABLE","pass":1}],
    "counters":[1.5,2.5],"counterDisclosure":"two contexts",
    "fit":{"quantity":"real time","curve":"N","coefficient":2.5,
           "rmsPercent":1.5,"scalesWithUnit":true,"reason":"the scripted fit"},
    "overheadFloorNs":0.5,
    "percentiles":[[{"available":true,"value":3.5}]],
    "chunkCount":2,"chunkIterations":3,
    "timeStats":{"samples":2,"mean":1.0,"median":1.5,"sampleStdDev":0.5,
                 "coefficientOfVariation":0.25,"min":0.5,"max":1.5},
    "metricStats":[{"samples":1,"mean":9.0,"median":9.0,"sampleStdDev":0.0,
                    "coefficientOfVariation":0.0,"min":9.0,"max":9.0}],
    "contextPercentiles":[[[{"available":false,"value":null}]]],
    "contextRaw":[{"columns":[[1.0,2.0]]},{"columns":[[3.0,4.0]]}],
    "raw":{"columns":[[5.0,6.0]]}}],
  "metricLabels":["work"],"counterLabels":["ctr"],
  "counterStats":[{"samples":4,"mean":4.0,"median":4.5,"sampleStdDev":4.25,
                    "coefficientOfVariation":4.5,"min":4.75,"max":5.0}],
  "statisticValues":[{"name":"custom","unit":"TIME","value":4.5}],
  "metricMethod":"PASSES","passCount":2,
  "passSpread":3.5,"spreadWarning":false}]}
)JSON";

auto everyFieldOfTheDocumentLoads() -> void
{
  sg::test::writeText("harness_json_load_rich.json", kRich);
  const auto loaded = sg::loadReport("harness_json_load_rich.json");
  check(loaded.has_value(), "the document carrying every field loads (FR-022)");
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    return;
  }

  const sg::BenchmarkResult& entry = loaded->results[0];
  check(entry.outcome == sg::RunOutcome::SKIPPED
            && entry.reason == "the scripted skip" && entry.hideRepetitionRows
            && entry.timeUnit == sg::TimeUnit::MILLISECONDS
            && !entry.primaryIsCpu && entry.pauseSamplesFolded,
        "the entry answers its outcome, reason, unit and flags (FR-012)");
  check(entry.percentilePoints.size() == 1
            && sg::test::sameValue(entry.percentilePoints[0], 50.0),
        "the entry answers its percentile points (FR-013)");
  check(entry.contexts.size() == 2
            && entry.contexts[0].waitMode == sg::WaitMode::SPIN
            && entry.contexts[0].placement == "l3 group 0"
            && entry.contexts[0].applied
            && entry.contexts[1].waitMode == sg::WaitMode::BLOCK
            && !entry.contexts[1].applied,
        "the entry answers the disclosure of each context (FR-016)");
  check(entry.metricLabels.size() == 1 && entry.metricLabels[0] == "work"
            && entry.counterLabels.size() == 1
            && entry.counterLabels[0] == "ctr",
        "the entry answers its two label lists (FR-014)");
  check(entry.counterStats.size() == 1 && entry.counterStats[0].samples == 4,
        "the entry answers its counter statistics (FR-025)");
  check(entry.statisticValues.size() == 1
            && entry.statisticValues[0].name == "custom"
            && entry.statisticValues[0].unit == sg::StatUnit::TIME
            && sg::test::sameValue(entry.statisticValues[0].value, 4.5),
        "the entry answers its custom statistic (FR-015)");
  check(loaded->runJitterStats.has_value()
            && loaded->runJitterStats->size() == 1
            && (*loaded->runJitterStats)[0].cpu == 2
            && (*loaded->runJitterStats)[0].stats.gapCount == 1
            && (*loaded->runJitterStats)[0].stats.thresholdNs == 280,
        "the run-level jitter record reads into the carrier (E-07, FR-031)");
  check(entry.jitterFolds.size() == 1 && entry.jitterFolds[0].cpu == 3
            && entry.jitterFolds[0].stats.gapCount == 4,
        "the entry's jitter scope reads into its carrier (E-08, FR-027)");

  check(entry.rows.size() == 1, "the entry answers its one row (FR-012)");
  const sg::ResultRow& row = entry.rows[0];
  check(row.kind == sg::RowKind::AGGREGATE && row.label == "aggregate"
            && row.endpointSamples == 7 && row.pauseSamplesFolded
            && row.timeAvailability == sg::counters::Availability::GAP,
        "the row answers its kind, label and availability (FR-012)");
  check(row.counters.size() == 2 && sg::test::sameValue(row.counters[1], 2.5)
            && row.counterDisclosure == "two contexts",
        "the row answers its counter values and its disclosure (FR-014)");
  check(row.fit.quantity == "real time" && row.fit.curve == "N"
            && sg::test::sameValue(row.fit.coefficient, 2.5)
            && sg::test::sameValue(row.fit.rmsPercent, 1.5)
            && row.fit.scalesWithUnit && row.fit.reason == "the scripted fit",
        "the row answers its fit (FR-017)");
  check(row.timeStats.samples == 2
            && sg::test::sameValue(row.timeStats.median, 1.5),
        "the row answers its time statistics (FR-025)");
  check(row.metricStats.size() == 1
            && sg::test::sameValue(row.metricStats[0].mean, 9.0),
        "the row answers its metric statistics (FR-025)");
  check(row.percentiles.size() == 1 && row.percentiles[0].size() == 1
            && row.percentiles[0][0].available
            && sg::test::sameValue(row.percentiles[0][0].value, 3.5),
        "the row answers its percentile cell (FR-013)");
  check(
      entry.contextPercentiles.size() == 1
          && entry.contextPercentiles[0].size() == 1
          && !entry.contextPercentiles[0][0][0].available
          && sg::test::sameValue(entry.contextPercentiles[0][0][0].value, 0.0),
      "the row answers its per-context percentile block (E-09)");
  check(loaded->rowRaw.size() == 1 && loaded->rowRaw[0].size() == 1
            && loaded->rowRaw[0][0].columns.size() == 1
            && sg::test::sameValue(loaded->rowRaw[0][0].columns[0][1], 6.0),
        "the row answers its own chunk columns (FR-018)");
  check(loaded->contextRaw.size() == 1 && loaded->contextRaw[0].size() == 1
            && loaded->contextRaw[0][0].size() == 2
            && loaded->contextRaw[0][0][0].columns.size() == 1
            && sg::test::sameValue(loaded->contextRaw[0][0][1].columns[0][0],
                                   3.0),
        "the row answers the chunk columns of each context (E-09)");
}

// The three rank figures of specs/040 (T013, FR-031, B-9): the document
// the writer carries loads back with `correlation`, `slope` and `delta`
// equal, `null` for a figure the run did not measure. The input stands
// through the record types, the round-trip form of this file.
auto theHuntRankFieldsLoadBackEqual() -> void
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  auto run = sg::test::scriptedRun(options);

  sg::hunt::EntryHunt hunt;
  hunt.method = sg::MetricMethod::AUTO;
  hunt.passCount = 1;
  hunt.top = 8;
  hunt.retriedPasses = 0;

  sg::hunt::RankedEvent measured;
  measured.address = "event.alpha";
  measured.rate = 1000.0;
  measured.cv = 0.5;
  measured.multiplexed = false;
  measured.rank = 1;
  measured.sensitivityCorrelation = 0.75;
  measured.sensitivitySlope = 2.5;
  measured.delta = -12.5;
  hunt.entries.push_back(measured);

  // The figures the run did not measure: the writer spells them `null`
  // (FR-030, B-9) and the reader answers the empty figure.
  sg::hunt::RankedEvent unstamped;
  unstamped.address = "event.beta";
  unstamped.rate = 500.0;
  unstamped.cv = 0.25;
  unstamped.noisy = true;
  unstamped.multiplexed = false;
  hunt.entries.push_back(unstamped);

  run.result.hunt = hunt;
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;

  check(sg::detail::writeReport("harness_json_load_ranks.json", data),
        "the writer wrote the document with the hunt (FR-030)");
  const auto loaded = sg::loadReport("harness_json_load_ranks.json");
  check(loaded.has_value(), "the reader read the written document (FR-031)");
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    return;
  }

  check(loaded->results.size() == 1 && loaded->results[0].hunt.has_value()
            && loaded->results[0].hunt->entries.size() == 2,
        "the loaded result answers the hunt record and its two events "
        "(FR-031)");
  if (loaded->results.size() != 1 || !loaded->results[0].hunt.has_value()
      || loaded->results[0].hunt->entries.size() != 2)
  {
    return;
  }

  const sg::hunt::RankedEvent& first = loaded->results[0].hunt->entries[0];
  check(
      first.sensitivityCorrelation.has_value()
          && sg::test::sameValue(*first.sensitivityCorrelation, 0.75),
      "the correlation reads back the figure the run measured (FR-031, " "B-"
                                                                         "9)");
  check(first.sensitivitySlope.has_value()
            && sg::test::sameValue(*first.sensitivitySlope, 2.5),
        "the slope reads back the figure the run measured (FR-031, B-9)");
  check(first.delta.has_value() && sg::test::sameValue(*first.delta, -12.5),
        "the delta reads back the figure the run measured (FR-031, B-9)");

  const sg::hunt::RankedEvent& second = loaded->results[0].hunt->entries[1];
  check(!second.sensitivityCorrelation.has_value()
            && !second.sensitivitySlope.has_value()
            && !second.delta.has_value(),
        "a figure the run did not measure reads back `null` (FR-031, B-9)");
}

// FR-024, SC-007: one planted disagreement per shape of the reader, each
// refused with the JSON pointer of the field it stands in.
struct Mutation
{
  const char* find;
  const char* replace;
  const char* pointer;
};

// Every plant of one record, driven in turn: the planted text stands in the
// written document, the reader refuses the document it writes, and the
// refusal names the pointer of the field the plant stands at (FR-024,
// FR-029, SC-007).
auto plantsRefuseAtTheirPointers(const std::string& text,
                                 const std::span<const Mutation>& plants,
                                 const char* const fr) -> void
{
  for (const Mutation& plant : plants) {
    const std::size_t at = text.find(plant.find);
    if (at == std::string::npos) {
      std::printf("  no '%s' in the written document\n", plant.find);
      sg::test::fail("the planted field has no place to stand");
      continue;
    }
    std::string broken = text;
    broken.replace(at, std::strlen(plant.find), plant.replace);
    sg::test::writeText(kAgain, broken);
    const auto refused = sg::loadReport(kAgain);
    if (refused.has_value()) {
      std::printf("  '%s' loaded\n", plant.find);
      sg::test::fail(
          ("the planted field loads anyway (" + std::string(fr) + ")").c_str());
      continue;
    }
    if (refused.error().find(plant.pointer) == std::string::npos) {
      std::printf("  '%s' refused without '%s': %s\n",
                  plant.find,
                  plant.pointer,
                  refused.error().c_str());
      sg::test::fail(("the refusal names no pointer for the field ("
                      + std::string(fr) + ")")
                         .c_str());
    }
  }
}

// One planted field per position of every conversion chain the reader
// runs, planted on the document that carries every optional field: each
// position answers a refusal naming the pointer it stands at (FR-024,
// FR-025, SC-007). The written document holds no statistics record, no
// fit, no context and no statistic value, so these plants stand on the
// hand-built one.
constexpr Mutation
    kRichMutations
        [] =
            {
                // The seven fields of a statistics record (E-07, FR-025).
                {"\"samples\":2",
                 "\"samples\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/samples"},
                {"\"mean\":1.0",
                 "\"mean\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/mean"},
                {"\"median\":1.5",
                 "\"median\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/median"},
                {"\"sampleStdDev\":0.5",
                 "\"sampleStdDev\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/sampleStdDev"},
                {"\"coefficientOfVariation\":0.25",
                 "\"coefficientOfVariation\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/coefficientOfVariation"},
                {"\"min\":0.5",
                 "\"min\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/min"},
                {"\"max\":1.5",
                 "\"max\":\"x\"",
                 "/benchmarks/0/rows/0/timeStats/max"},
                // The five fields of a fit record and its reason (E-08,
                // FR-012).
                {"\"quantity\":\"real time\"",
                 "\"quantity\":5",
                 "/benchmarks/0/rows/0/fit/quantity"},
                {"\"curve\":\"N\"",
                 "\"curve\":5",
                 "/benchmarks/0/rows/0/fit/curve"},
                {"\"coefficient\":2.5",
                 "\"coefficient\":\"x\"",
                 "/benchmarks/0/rows/0/fit/coefficient"},
                {"\"rmsPercent\":1.5",
                 "\"rmsPercent\":\"x\"",
                 "/benchmarks/0/rows/0/fit/rmsPercent"},
                {"\"scalesWithUnit\":true",
                 "\"scalesWithUnit\":\"x\"",
                 "/benchmarks/0/rows/0/fit/scalesWithUnit"},
                {"\"reason\":\"the scripted fit\"",
                 "\"reason\":5",
                 "/benchmarks/0/rows/0/fit/reason"},
                // The three fields of a context disclosure, wrong of kind and
                // then
                // each taken away (E-09, FR-013).
                {"\"waitMode\":\"SPIN\"",
                 "\"waitMode\":5",
                 "/benchmarks/0/contexts/0/waitMode"},
                {"\"placement\":\"l3 group 0\"",
                 "\"placement\":5",
                 "/benchmarks/0/contexts/0/placement"},
                {"\"applied\":true",
                 "\"applied\":\"x\"",
                 "/benchmarks/0/contexts/0/applied"},
                {"{\"waitMode\":\"SPIN\",", "{", "/benchmarks/0/contexts"},
                {"\"placement\":\"l3 group 0\",", "", "/benchmarks/0/contexts"},
                {",\"applied\":true}", "}", "/benchmarks/0/contexts"},
                // The three fields of a statistic value, wrong of kind and
                // missing
                // (E-11, FR-015).
                {"\"name\":\"custom\"",
                 "\"name\":5",
                 "/benchmarks/0/statisticValues/0/name"},
                {"\"unit\":\"TIME\"",
                 "\"unit\":\"BOGUS\"",
                 "/benchmarks/0/statisticValues/0/unit"},
                {"\"value\":4.5",
                 "\"value\":\"x\"",
                 "/benchmarks/0/statisticValues/0/value"},
                {"{\"name\":\"custom\",", "{", "/benchmarks/0/statisticValues"},
                {"\"unit\":\"TIME\",", "", "/benchmarks/0/statisticValues"},
                {",\"value\":4.5}", "}", "/benchmarks/0/statisticValues"},
                // The three fields of the noise warning, each taken away
                // (E-10).
                {"{\"spinning\":3,", "{", "/benchmarks/0/noiseWarning"},
                {"\"runnableCpus\":16,", "", "/benchmarks/0/noiseWarning"},
                {",\"sharesOneCpu\":true", "", "/benchmarks/0/noiseWarning"},
                // The chunk columns of the context span, one metric value, and
                // one
                // percentile cell missing its availability.
                {"\"contextRaw\":[{\"columns\":",
                 "\"contextRaw\":[{\"columnz\":",
                 "/benchmarks/0/rows/0/contextRaw"},
                {"\"value\":2.5",
                 "\"value\":\"x\"",
                 "/benchmarks/0/rows/0/metrics/0/value"},
                {"[{\"available\":true,\"value\":3.5}]",
                 "[{\"value\":3.5}]",
                 "/benchmarks/0/rows/0/percentiles/0/0"},
                // The pass disclosure of specs/024 FR-038: each key of the
                // wrong kind,
                // the method word outside the closed list, the count below
                // zero, and a
                // warning that disagrees with the spread beside it. A key taken
                // away is
                // the one shape that loads, and the case after this one covers
                // it
                // (FR-024, FR-038).
                {"\"metricMethod\":\"PASSES\"",
                 "\"metricMethod\":1",
                 "/benchmarks/0/metricMethod"},
                {"\"metricMethod\":\"PASSES\"",
                 "\"metricMethod\":\"PASSESSED\"",
                 "/benchmarks/0/metricMethod"},
                {"\"passCount\":2",
                 "\"passCount\":\"two\"",
                 "/benchmarks/0/passCount"},
                {"\"passCount\":2",
                 "\"passCount\":-1",
                 "/benchmarks/0/passCount"},
                {"\"passSpread\":3.5",
                 "\"passSpread\":\"wide\"",
                 "/benchmarks/0/passSpread"},
                {"\"spreadWarning\":false",
                 "\"spreadWarning\":\"yes\"",
                 "/benchmarks/0/spreadWarning"},
                {"\"spreadWarning\":false",
                 "\"spreadWarning\":true",
                 "/benchmarks/0/spreadWarning"},
                // The pass index of one metric column at a value of the wrong
                // kind
                // (specs/024 E-08, FR-019); the key taken away loads, as the
                // case below
                // shows.
                {"\"pass\":1",
                 "\"pass\":\"one\"",
                 "/benchmarks/0/rows/0/metrics/0/pass"},
                // The eight keys of the entry header: name, suite, case,
                // outcome,
                // hideRepetitionRows, timeUnit, primaryIsCpu and
                // pauseSamplesFolded.
                // Each wrong of kind, so the fold of the entry read takes every
                // part's own refusal edge (FR-024, SC-007, E-05).
                {"\"case\":\"bmRich\"", "\"case\":5", "/benchmarks/0/case"},
                // The jitter scopes of E-07 and E-08: the seventeen keys of a
                // per-cpu
                // stats object, each wrong of kind and two taken away; the kept
                // gap
                // records the document carries; the seven keys of the run-level
                // record; and the per-entry scope's stats array. Each refusal
                // names
                // the pointer it stands at (FR-029, E-03, E-07, E-08, E-10).
                {"\"cpu\":2,", "\"cpu\":\"x\",", "/jitter/stats/0/cpu"},
                {"\"gapCount\":1,",
                 "\"gapCount\":\"x\",",
                 "/jitter/stats/0/gapCount"},
                {"\"gapsPerSecond\":1000.0,",
                 "\"gapsPerSecond\":\"x\",",
                 "/jitter/stats/0/gapsPerSecond"},
                {"\"minNs\":200,", "\"minNs\":\"x\",", "/jitter/stats/0/minNs"},
                {"\"medianNs\":200,",
                 "\"medianNs\":\"x\",",
                 "/jitter/stats/0/medianNs"},
                {"\"meanNs\":200.0,",
                 "\"meanNs\":\"x\",",
                 "/jitter/stats/0/meanNs"},
                {"\"p90Ns\":210,", "\"p90Ns\":\"x\",", "/jitter/stats/0/p90Ns"},
                {"\"p99Ns\":220,", "\"p99Ns\":\"x\",", "/jitter/stats/0/p99Ns"},
                {"\"p999Ns\":230,",
                 "\"p999Ns\":\"x\",",
                 "/jitter/stats/0/p999Ns"},
                {"\"p9999Ns\":240,",
                 "\"p9999Ns\":\"x\",",
                 "/jitter/stats/0/p9999Ns"},
                {"\"maxNs\":250,", "\"maxNs\":\"x\",", "/jitter/stats/0/maxNs"},
                {"\"totalNs\":260,",
                 "\"totalNs\":\"x\",",
                 "/jitter/stats/0/totalNs"},
                {"\"totalPercent\":0.5,",
                 "\"totalPercent\":\"x\",",
                 "/jitter/stats/0/totalPercent"},
                {"\"runtimeNs\":270,",
                 "\"runtimeNs\":\"x\",",
                 "/jitter/stats/0/runtimeNs"},
                {"\"thresholdNs\":280,",
                 "\"thresholdNs\":\"x\",",
                 "/jitter/stats/0/thresholdNs"},
                {"\"loopPeriodNs\":29,",
                 "\"loopPeriodNs\":\"x\",",
                 "/jitter/stats/0/loopPeriodNs"},
                {"\"droppedGaps\":30,",
                 "\"droppedGaps\":\"x\",",
                 "/jitter/stats/0/droppedGaps"},
                {"\"cpu\":2,", "", "/jitter/stats/0"},
                {"\"meanNs\":200.0,", "", "/jitter/stats/0"},
                {"\"gaps\":[{\"stamp\":100,\"gapNs\":200}]",
                 "\"gaps\":5",
                 "/jitter/stats/0/gaps"},
                {"\"stamp\":100", "\"stemp\":100", "/jitter/stats/0/gaps/0"},
                {"\"stamp\":100",
                 "\"stamp\":\"x\"",
                 "/jitter/stats/0/gaps/0/stamp"},
                {"\"gapNs\":200}]",
                 "\"gapNz\":200}]",
                 "/jitter/stats/0/gaps/0"},
                {"\"gapNs\":200",
                 "\"gapNs\":\"x\"",
                 "/jitter/stats/0/gaps/0/gapNs"},
                {"\"thresholdNs\":200,",
                 "\"thresholdNs\":\"x\",",
                 "/jitter/thresholdNs"},
                {"\"loopPeriodNs\":20,",
                 "\"loopPeriodNs\":\"x\",",
                 "/jitter/loopPeriodNs"},
                {"\"cpus\":[2]", "\"cpus\":5", "/jitter/cpus"},
                {"\"cpus\":[2]", "\"cpus\":[\"x\"]", "/jitter/cpus"},
                {",\"stats\":[{\"cpu\":2,\"gapCount\":1,\"gapsPerSecond\":1000." "0,\"minNs\":" "200,\"medianNs\":200,\"meanNs\":200.0," "\"p90Ns\":210,\"p99Ns\":220," "\"p999Ns\":230," "\"p9999Ns\":" "240,\"maxNs\":" "250," "\"totalNs\":" "260," "\"totalP" "ercent\"" ":0.5," "\"runtim" "eNs\":" "270," "\"thresh" "oldNs\":" "280," "\"loopPeriodNs\":29,\"droppedGaps\":30,\"gaps\":[{\"stamp\":100," "\"gapNs\":200}]}]",
                 "",
                 "/jitter"},
                {"\"droppedGaps\":0,\"disclosure\"",
                 "\"droppedGaps\":\"x\",\"disclosure\"",
                 "/jitter/droppedGaps"},
                {"\"disclosure\":\"the run's disclosure\"",
                 "\"disclosure\":5",
                 "/jitter/disclosure"},
                {"\"cpu\":3,",
                 "\"cpu\":\"x\",",
                 "/benchmarks/0/jitter/stats/0/cpu"},
                {"\"stats\":[{\"cpu\":3,\"gapCount\":4,\"gapsPerSecond\":400.0," "\"minNs\":" "300,\"medianNs\":310,\"meanNs\":320.0,\"p90Ns\":" "330,\"p99Ns\":340," "\"p999Ns\":350,\"p9999Ns\":" "360,\"maxNs\":370," "\"totalNs\":380," "\"totalP" "ercent\"" ":0.6," "\"runtim" "eNs\":" "390," "\"thresh" "oldNs\":" "400," "\"loopPeriodNs\":41,\"droppedGaps\":42}]",
                 "",
                 "/benchmarks/0/jitter"},
                {"\"stats\":[{\"cpu\":3,\"gapCount\":4,\"gapsPerSecond\":400.0," "\"minNs\":" "300,\"medianNs\":310,\"meanNs\":320.0,\"p90Ns\":" "330,\"p99Ns\":340," "\"p999Ns\":350,\"p9999Ns\":" "360,\"maxNs\":370," "\"totalNs\":380," "\"totalP" "ercent\"" ":0.6," "\"runtim" "eNs\":" "390," "\"thresh" "oldNs\":" "400," "\"loopPeriodNs\":41,\"droppedGaps\":42}]",
                 "\"stats\":5",
                 "/benchmarks/0/jitter/stats"},
};

auto plantedFieldsOfEveryRecordRefuse() -> void
{
  const std::string document = kRich;
  for (const Mutation& mutation : kRichMutations) {
    const std::size_t at = document.find(mutation.find);
    if (at == std::string::npos) {
      std::printf("  no %s in the hand-built document\n", mutation.find);
      sg::test::fail("no planted field in the hand-built document");
      continue;
    }
    std::string broken = document;
    broken.replace(at, std::strlen(mutation.find), mutation.replace);
    sg::test::writeText("harness_json_load_richmutant.json", broken);
    const auto refused = sg::loadReport("harness_json_load_richmutant.json");
    if (refused.has_value()) {
      std::printf("  %s loaded after the plant\n", mutation.find);
      sg::test::fail("a planted field of a record loads (FR-024)");
      continue;
    }
    if (refused.error().find(mutation.pointer) == std::string::npos) {
      std::printf("  %s refused without %s: %s\n",
                  mutation.find,
                  mutation.pointer,
                  refused.error().c_str());
      sg::test::fail("the refusal names no pointer (FR-024, SC-007)");
    }
  }
}

// Two wrong kinds in one metric column: the fold over the parts of a record
// records the first answer that carries an error and stops there, so the
// refusal names the running ratio and says nothing of the availability beside
// it. One plant per document cannot show the stop, so this document carries
// two (FR-024, SC-007).
auto firstFailingPartStopsTheFold() -> void
{
  constexpr std::string_view kRatio = "\"runningRatio\":1.0";
  constexpr std::string_view kState = "\"availability\":\"COUNTABLE\"";
  constexpr const char* const kWanted =
      "/benchmarks/0/rows/0/metrics/0/runningRatio"
      ": expected a number, found string";
  std::string broken = kRich;
  const std::size_t ratio = broken.find(kRatio);
  const std::size_t state = broken.find(kState);
  if (ratio == std::string::npos || state == std::string::npos) {
    sg::test::fail("the rich document holds no metric column to plant twice");
    return;
  }
  // The later key first, so the first replace leaves the offset of the second
  // one standing where it was.
  broken.replace(state, kState.size(), "\"availability\":\"MAYBE\"");
  broken.replace(ratio, kRatio.size(), "\"runningRatio\":\"slow\"");
  sg::test::writeText("harness_json_load_firstpart.json", broken);
  const auto refused = sg::loadReport("harness_json_load_firstpart.json");
  if (refused.has_value()) {
    sg::test::fail("a metric column of two wrong kinds loads (FR-024)");
    return;
  }
  if (refused.error().find(kWanted) == std::string::npos) {
    std::printf("  the two-plant document refused with: %s\n",
                refused.error().c_str());
  }
  check(refused.error().find(kWanted) != std::string::npos,
        "the refusal carries the running ratio, the first part to fail");
  check(refused.error().find("availability") == std::string::npos,
        "the fold stops, so the availability beside it stands unreported");
  std::printf("ok: the first failing part of a metric column stops the fold\n");
}

// The five keys specs/024 FR-038 added, four to the entry and one to a metric
// column: a document written before that feature carries none of them, and
// the comparison of specs/026 reads such a stored baseline (FR-051), so a key
// that stands missing loads and leaves its member at the default of a run
// that collected no pass. Each key alone first, then all five together.
auto aDocumentPredatingThePassFieldsLoads() -> void
{
  constexpr const char* const kPassKeys[] = {
      "\"metricMethod\":\"PASSES\",",
      "\"passCount\":2,",
      "\"passSpread\":3.5,",
      ",\"spreadWarning\":false",
      ",\"pass\":1",
  };

  for (const char* const key : kPassKeys) {
    std::string stripped = kRich;
    const std::size_t at = stripped.find(key);
    if (at == std::string::npos) {
      sg::test::fail("the rich document holds no such pass key");
      continue;
    }
    stripped.erase(at, std::strlen(key));
    sg::test::writeText("harness_json_load_pre024.json", stripped);
    const auto loaded = sg::loadReport("harness_json_load_pre024.json");
    if (!loaded.has_value()) {
      std::printf("  without '%s': %s\n", key, loaded.error().c_str());
    }
    check(loaded.has_value(),
          "a document missing one pass key loads (FR-038, FR-051)");
  }

  constexpr std::string_view kWarning = "\"spreadWarning\":false";
  std::string pre024 = kRich;
  pre024.erase(pre024.find(kPassKeys[4]), std::strlen(kPassKeys[4]));
  const std::size_t method = pre024.find("\"metricMethod\"");
  const std::size_t warning = pre024.find(kWarning);
  if (method == std::string::npos || warning == std::string::npos) {
    sg::test::fail("the rich document holds no pass block to take away");
    return;
  }
  const std::size_t comma = pre024.rfind(',', method);
  pre024.erase(comma, warning + kWarning.size() - comma);
  sg::test::writeText("harness_json_load_pre024.json", pre024);
  const auto loaded = sg::loadReport("harness_json_load_pre024.json");
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    sg::test::fail("a document written before the pass fields refuses");
    return;
  }
  check(loaded->results.size() == 1
            && loaded->results[0].name == "bmRich"
            && loaded->results[0].metricMethod == sg::MetricMethod::AUTO
            && loaded->results[0].passCount == 0
            && !loaded->results[0].passSpread.has_value()
            && !loaded->results[0].rows[0].metrics[0].pass.has_value(),
        "the document of before specs/024 answers the defaults of a run that "
        "collected no pass (FR-038, FR-051)");
}

// The benchmark list is the one key of the root the reader asks for by
// name, and the noise warning is the one key a document may leave out
// entirely (FR-024, E-10).
auto theRootListAndTheWarningKeyAnswerTheirOwnRules(const std::string& text)
    -> void
{
  sg::test::writeText("harness_json_load_nobenchmarks.json",
                      "{\"schemaVersion\":\"1.0\",\"speedgun\":{\"version\":"
                      "\"0.12.0\"," "\"buildType\":\"Release\"}}");
  const auto refused = sg::loadReport("harness_json_load_nobenchmarks.json");
  check(
      !refused.has_value()
          && refused.error().find("'benchmarks'") != std::string::npos,
      "a document whose benchmark list stands missing is refused " "(FR-024)");

  std::string noWarning = text;
  const std::size_t at = noWarning.find(",\"noiseWarning\":null");
  if (at == std::string::npos) {
    sg::test::fail("no noise-warning key in the written document");
    return;
  }
  noWarning.erase(at, std::strlen(",\"noiseWarning\":null"));
  sg::test::writeText("harness_json_load_nowarning.json", noWarning);
  const auto quiet = sg::loadReport("harness_json_load_nowarning.json");
  check(quiet.has_value() && quiet->results.size() == 1,
        "a document that carries no noise-warning key loads quietly " "(E-10)");
}

constexpr Mutation kMutations[] = {
    {"\"schemaVersion\":\"1.13\"", "\"schemaVersion\":1", "/schemaVersion"},
    {"\"name\":\"bmJsonWindow\"", "\"name\":[]", "/benchmarks/0/name"},
    // The two kinds the naming word has no other caller for: a document
    // that holds null, and one that holds a flag, at a name.
    {"\"name\":\"bmJsonWindow\"", "\"name\":null", "/benchmarks/0/name"},
    {"\"name\":\"bmJsonWindow\"", "\"name\":true", "/benchmarks/0/name"},
    {"\"suite\":\"bmJsonWindow\"", "\"suite\":{}", "/benchmarks/0/suite"},
    {"\"case\":\"bmJsonWindow\",", "", "case"},
    {"\"outcome\":\"MEASURED\"",
     "\"outcome\":\"BOGUS\"",
     "/benchmarks/0/outcome"},
    {"\"reason\":null", "\"reason\":5", "/benchmarks/0/reason"},
    {"\"hideRepetitionRows\":false",
     "\"hideRepetitionRows\":\"x\"",
     "/benchmarks/0/hideRepetitionRows"},
    {"\"timeUnit\":\"NANOSECONDS\"",
     "\"timeUnit\":\"BOGUS\"",
     "/benchmarks/0/timeUnit"},
    {"\"primaryIsCpu\":true",
     "\"primaryIsCpu\":1",
     "/benchmarks/0/primaryIsCpu"},
    {"\"pauseSamplesFolded\":false,\"percentilePoints\"",
     "\"pauseSamplesFolded\":\"x\",\"percentilePoints\"",
     "/benchmarks/0/pauseSamplesFolded"},
    {"\"percentilePoints\":[50.0,90.0,99.0,99.9]",
     "\"percentilePoints\":{}",
     "/benchmarks/0/percentilePoints"},
    {"\"percentilePoints\":[50.0,90.0,99.0,99.9]",
     "\"percentilePoints\":[\"x\"]",
     "/benchmarks/0/percentilePoints"},
    {"\"contexts\":[]", "\"contexts\":[{}]", "/benchmarks/0/contexts/0"},
    {"\"contexts\":[]", "\"contexts\":[5]", "/benchmarks/0/contexts/0"},
    {"\"noiseWarning\":null",
     "\"noiseWarning\":{}",
     "/benchmarks/0/noiseWarning"},
    {"\"rows\":[{", "\"rows\":[5,{", "/benchmarks/0/rows/0"},
    {"\"metricLabels\":[\"work\"]",
     "\"metricLabels\":[5]",
     "/benchmarks/0/metricLabels"},
    {"\"counterLabels\":[]",
     "\"counterLabels\":{}",
     "/benchmarks/0/counterLabels"},
    {"\"counterStats\":[]",
     "\"counterStats\":[{}]",
     "/benchmarks/0/counterStats/0"},
    {"\"statisticValues\":[]",
     "\"statisticValues\":[{}]",
     "/benchmarks/0/statisticValues"},
    {"\"kind\":\"REPETITION\"",
     "\"kind\":\"BOGUS\"",
     "/benchmarks/0/rows/0/kind"},
    {"\"label\":null", "\"label\":5", "/benchmarks/0/rows/0/label"},
    {"\"iterations\":4",
     "\"iterations\":-3",
     "/benchmarks/0/rows/0/iterations"},
    {"\"timePerIterationNs\":2500000.0",
     "\"timePerIterationNs\":\"x\"",
     "/benchmarks/0/rows/0/timePerIterationNs"},
    {"\"cpuTimePerIterationNs\":25000000.0",
     "\"cpuTimePerIterationNs\":\"x\"",
     "/benchmarks/0/rows/0/cpuTimePerIterationNs"},
    {"\"endpointSamples\":5",
     "\"endpointSamples\":1.5",
     "/benchmarks/0/rows/0/endpointSamples"},
    {"\"pauseSamplesFolded\":false,\"timeAvailability\"",
     "\"pauseSamplesFolded\":1,\"timeAvailability\"",
     "/benchmarks/0/rows/0/pauseSamplesFolded"},
    {"\"timeAvailability\":\"COUNTABLE\",\"metrics\"",
     "\"timeAvailability\":\"BOGUS\",\"metrics\"",
     "/benchmarks/0/rows/0/timeAvailability"},
    {"\"metrics\":[{", "\"metrics\":[5,{", "/benchmarks/0/rows/0/metrics"},
    {"\"label\":\"work\",\"value\":25.0",
     "\"label\":5,\"value\":25.0",
     "/benchmarks/0/rows/0/metrics/0/label"},
    {"\"runningRatio\":1.0",
     "\"runningRatio\":\"x\"",
     "/benchmarks/0/rows/0/metrics/0/runningRatio"},
    {"\"scaled\":false",
     "\"scaled\":\"x\"",
     "/benchmarks/0/rows/0/metrics/0/scaled"},
    {",\"availability\":\"COUNTABLE\",\"pass\"",
     ",\"availability\":\"BOGUS\",\"pass\"",
     "/benchmarks/0/rows/0/metrics/0/availability"},
    {"\"counters\":[]",
     "\"counters\":[\"x\"]",
     "/benchmarks/0/rows/0/counters"},
    {"\"counterDisclosure\":null",
     "\"counterDisclosure\":5",
     "/benchmarks/0/rows/0/counterDisclosure"},
    {"\"fit\":null", "\"fit\":{}", "/benchmarks/0/rows/0/fit"},
    {"\"overheadFloorNs\":10.0",
     "\"overheadFloorNs\":\"x\"",
     "/benchmarks/0/rows/0/overheadFloorNs"},
    {"\"percentiles\":[[",
     "\"percentiles\":[5,[",
     "/benchmarks/0/rows/0/percentiles/0"},
    {"\"available\":true,\"value\":20000000.0",
     "\"available\":\"x\",\"value\":20000000.0",
     "/benchmarks/0/rows/0/percentiles/0/0"},
    {"\"available\":true,\"value\":20000000.0",
     "\"available\":true,\"value\":\"x\"",
     "/benchmarks/0/rows/0/percentiles/0/0"},
    {"\"chunkCount\":4",
     "\"chunkCount\":\"x\"",
     "/benchmarks/0/rows/0/chunkCount"},
    {"\"chunkIterations\":1",
     "\"chunkIterations\":-1",
     "/benchmarks/0/rows/0/chunkIterations"},
    {"\"timeStats\":null",
     "\"timeStats\":{}",
     "/benchmarks/0/rows/0/timeStats"},
    {"\"metricStats\":[]",
     "\"metricStats\":[5]",
     "/benchmarks/0/rows/0/metricStats"},
    {"\"raw\":{\"columns\":",
     "\"raw\":{\"columnz\":",
     "/benchmarks/0/rows/0/raw"},
    {"\"raw\":{\"columns\":[",
     "\"raw\":{\"columns\":[5,",
     "/benchmarks/0/rows/0/raw/columns/0"},
    {"\"raw\":{\"columns\":[[",
     "\"raw\":{\"columns\":[[\"x\",",
     "/benchmarks/0/rows/0/raw/columns/0"},
    {"\"chunkCount\":4", "\"chunkCount\":3", "a chunk column holds"},
};

// One planted value in place of the whole value the key carries, so a
// field that stands where an array or an object belongs stays a readable
// document (FR-024).
// One past the value that stands at `valueAt`: an array or an object
// closes at its own bracket, a scalar ends at the next separator.
auto valueEnd(const std::string& text, std::size_t valueAt) -> std::size_t
{
  std::size_t end = valueAt;
  const char open = text[end];
  if (open == '[' || open == '{') {
    const char close = open == '[' ? ']' : '}';
    std::size_t depth = 0;
    bool inside = false;
    for (; end < text.size(); ++end) {
      const char symbol = text[end];
      if (inside) {
        if (symbol == '"' && text[end - 1] != '\\') {
          inside = false;
        }
        continue;
      }
      if (symbol == '"') {
        inside = true;
      } else if (symbol == open) {
        ++depth;
      } else if (symbol == close) {
        --depth;
        if (depth == 0) {
          ++end;
          break;
        }
      }
    }
  } else {
    while (end < text.size() && text[end] != ',' && text[end] != '}'
           && text[end] != ']')
    {
      ++end;
    }
  }
  return end;
}

auto replaceValue(std::string& text,
                  const char* key,
                  const char* replacement) -> bool
{
  const std::string needle = std::string("\"") + key + "\":";
  const std::size_t at = text.find(needle);
  if (at == std::string::npos) {
    return false;
  }
  const std::size_t end = valueEnd(text, at + needle.size());
  text.replace(at, end - at, needle + replacement);
  return true;
}

// One key taken out of an object whole, its separating comma with it, so
// a document can stand without a key the reader only reads leniently.
auto eraseKey(std::string& text, const char* key) -> bool
{
  const std::string needle = std::string("\"") + key + "\":";
  const std::size_t at = text.find(needle);
  if (at == std::string::npos) {
    return false;
  }
  std::size_t start = at;
  std::size_t end = valueEnd(text, at + needle.size());
  if (start > 0 && text[start - 1] == ',') {
    --start;
  } else if (end < text.size() && text[end] == ',') {
    ++end;
  }
  text.erase(start, end - start);
  return true;
}

constexpr struct Span
{
  const char* key;
  const char* pointer;
} kSpans[] = {
    {"benchmarks", "/benchmarks"},
    {"contexts", "/benchmarks/0/contexts"},
    {"rows", "/benchmarks/0/rows"},
    {"metricLabels", "/benchmarks/0/metricLabels"},
    {"counterLabels", "/benchmarks/0/counterLabels"},
    {"counterStats", "/benchmarks/0/counterStats"},
    {"statisticValues", "/benchmarks/0/statisticValues"},
    {"metrics", "/benchmarks/0/rows/0/metrics"},
    {"counters", "/benchmarks/0/rows/0/counters"},
    {"percentiles", "/benchmarks/0/rows/0/percentiles"},
    {"metricStats", "/benchmarks/0/rows/0/metricStats"},
    {"raw", "/benchmarks/0/rows/0/raw"},
    {"columns", "/benchmarks/0/rows/0/raw/columns"},
};

auto brokenSpansRefuseWithPointers(const std::string& text) -> void
{
  for (const Span& span : kSpans) {
    std::string broken = text;
    if (!replaceValue(broken, span.key, "0")) {
      std::printf("  no '%s' in the written document\n", span.key);
      sg::test::fail("the planted span has no place to stand");
      continue;
    }
    sg::test::writeText("harness_json_load_span.json", broken);
    const auto refused = sg::loadReport("harness_json_load_span.json");
    if (refused.has_value()) {
      std::printf("  '%s' holding a number loaded\n", span.key);
      sg::test::fail("a field that stands where a container belongs loads "
                     "anyway (FR-024)");
      continue;
    }
    if (refused.error().find(span.pointer) == std::string::npos) {
      std::printf("  '%s' refused without '%s': %s\n",
                  span.key,
                  span.pointer,
                  refused.error().c_str());
      sg::test::fail(
          "the refusal names no pointer for the field (FR-024, " "SC-007)");
    }
  }
}

auto brokenDocumentsRefuseWithPointers(const std::string& text) -> void
{
  check(!text.empty(), "the written file stands where the round trip left it");
  for (const Mutation& mutation : kMutations) {
    const std::size_t at = text.find(mutation.find);
    if (at == std::string::npos) {
      std::printf("  no '%s' in the written document\n", mutation.find);
      sg::test::fail("the planted disagreement has no place to stand");
      continue;
    }
    std::string broken = text;
    broken.replace(at, std::strlen(mutation.find), mutation.replace);
    sg::test::writeText("harness_json_load_mutant.json", broken);
    const auto refused = sg::loadReport("harness_json_load_mutant.json");
    if (refused.has_value()) {
      std::printf("  '%s' loaded\n", mutation.find);
      sg::test::fail("the planted disagreement loads anyway (FR-024)");
      continue;
    }
    if (refused.error().find(mutation.pointer) == std::string::npos) {
      std::printf("  '%s' refused without '%s': %s\n",
                  mutation.find,
                  mutation.pointer,
                  refused.error().c_str());
      sg::test::fail(
          "the refusal names no pointer for the field (FR-024, " "SC-007)");
    }
  }
}

// The two per-context blocks of the hand-built document answer the same
// refusal as every other container: the block array, one context block
// inside it, and the chunk columns of each context each stand where a
// number cannot (FR-024).
constexpr struct ContextSpan
{
  const char* key;
  const char* replacement;
  const char* pointer;
} kContextSpans[] = {
    {"contextPercentiles", "0", "/benchmarks/0/rows/0/contextPercentiles"},
    {"contextPercentiles", "[{}]", "/benchmarks/0/rows/0/contextPercentiles"},
    {"contextRaw", "0", "/benchmarks/0/rows/0/contextRaw"},
};

auto theContextSpansRefuseWithPointers() -> void
{
  for (const ContextSpan& span : kContextSpans) {
    std::string broken = kRich;
    if (!replaceValue(broken, span.key, span.replacement)) {
      std::printf("  no '%s' in the hand-built document\n", span.key);
      sg::test::fail("the planted span has no place to stand");
      continue;
    }
    sg::test::writeText("harness_json_load_span.json", broken);
    const auto refused = sg::loadReport("harness_json_load_span.json");
    if (refused.has_value()) {
      std::printf("  '%s' holding %s loaded\n", span.key, span.replacement);
      sg::test::fail("a field that stands where a container belongs loads "
                     "anyway (FR-024)");
      continue;
    }
    if (refused.error().find(span.pointer) == std::string::npos) {
      std::printf("  '%s' refused without '%s': %s\n",
                  span.key,
                  span.pointer,
                  refused.error().c_str());
      sg::test::fail(
          "the refusal names no pointer for the field (FR-024, " "SC-007)");
    }
  }
}

// The string one key carries in the written document, read from its bytes
// so a version bump moves no test here (specs/026 E-07).
auto quotedValue(const std::string& text, const char* key) -> std::string
{
  const std::string needle = std::string("\"") + key + "\":\"";
  const std::size_t at = text.find(needle);
  if (at == std::string::npos) {
    return {};
  }
  const std::size_t start = at + needle.size();
  const std::size_t end = text.find('"', start);
  return end == std::string::npos ? std::string()
                                  : text.substr(start, end - start);
}

// The three fields the load call gains (specs/026 E-07, FR-070, D-04):
// the two header texts the document carries, and one warning entry beside
// every result. The writer stamps the header texts itself, so the loaded
// text answers the document and not this build.
auto theHeaderFieldsAndTheStoredWarningsLoad(const std::string& text) -> void
{
  const auto loaded = sg::loadReport(kWritten);
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    sg::test::fail("the written document stopped loading");
    return;
  }
  check(loaded->schemaVersion == quotedValue(text, "schemaVersion")
            && !loaded->schemaVersion.empty(),
        "the loaded value answers the schemaVersion text the document "
        "carries (FR-070, E-07)");
  check(loaded->speedgunVersion == quotedValue(text, "version")
            && !loaded->speedgunVersion.empty(),
        "the loaded value answers the version text under the speedgun "
        "header (FR-070, E-07)");
  check(loaded->noiseWarnings.size() == loaded->results.size()
            && loaded->noiseWarnings.size() == 1
            && !loaded->noiseWarnings[0].has_value(),
        "a null noise warning answers one entry per result, holding "
        "nothing (FR-070, E-07)");

  // The field answers the text the document carried. The build stamps
  // its own number elsewhere; a later minor loads and answers its own
  // number in the field.
  std::string other = text;
  const auto stamp = other.find("\"schemaVersion\":") + 16;
  other.replace(stamp, other.find('"', stamp + 1) + 1 - stamp, "\"1.13\"");
  sg::test::writeText("harness_json_load_headertext.json", other);
  const auto minor = sg::loadReport("harness_json_load_headertext.json");
  check(minor.has_value() && minor->schemaVersion == "1.13",
        "the field answers the version of a later minor (FR-070, FR-010)");

  // A document that omits the speedgun object entirely stays readable and
  // answers an empty text: the reader is lenient on a header it prints.
  std::string noHeader = text;
  if (!eraseKey(noHeader, "speedgun")) {
    sg::test::fail("no speedgun header in the written document");
    return;
  }
  sg::test::writeText("harness_json_load_noheader.json", noHeader);
  const auto bare = sg::loadReport("harness_json_load_noheader.json");
  check(bare.has_value() && bare->speedgunVersion.empty()
            && bare->schemaVersion == quotedValue(text, "schemaVersion"),
        "a document that omits the speedgun object answers an empty "
        "speedgunVersion with no fault (FR-070, E-07)");

  // The speedgun object stands and the `version` key inside it does not:
  // the same lenience one level deeper, an empty text and no fault.
  std::string noVersionKey = text;
  if (!eraseKey(noVersionKey, "version")) {
    sg::test::fail("no version key in the written document");
    return;
  }
  sg::test::writeText("harness_json_load_noversionkey.json", noVersionKey);
  const auto keyless = sg::loadReport("harness_json_load_noversionkey.json");
  check(keyless.has_value() && keyless->speedgunVersion.empty()
            && keyless->schemaVersion == quotedValue(text, "schemaVersion"),
        "a speedgun header that carries no version key answers an empty "
        "speedgunVersion with no fault (FR-070, E-07)");

  // The warning key taken away entirely, which is the one key an entry
  // may leave out (E-10): the entry beside that result still stands and
  // holds nothing.
  std::string noWarning = text;
  const std::size_t absent = noWarning.find(",\"noiseWarning\":null");
  if (absent == std::string::npos) {
    sg::test::fail("no noise-warning key in the written document");
    return;
  }
  noWarning.erase(absent, std::strlen(",\"noiseWarning\":null"));
  sg::test::writeText("harness_json_load_noheaderwarning.json", noWarning);
  const auto quiet = sg::loadReport("harness_json_load_noheaderwarning.json");
  check(quiet.has_value() && quiet->noiseWarnings.size() == 1
            && !quiet->noiseWarnings[0].has_value(),
        "a document that carries no noise-warning key answers an entry "
        "that holds nothing (FR-070, E-07, E-10)");

  // The record the hand-built document carries, stored beside its result
  // and answering its three values (E-06, E-07).
  sg::test::writeText("harness_json_load_noise.json", kRich);
  const auto rich = sg::loadReport("harness_json_load_noise.json");
  check(rich.has_value(), "the document that carries a noise record loads");
  if (!rich.has_value()) {
    std::printf("  %s\n", rich.error().c_str());
    return;
  }
  check(rich->noiseWarnings.size() == rich->results.size()
            && rich->noiseWarnings.size() == 1
            && rich->noiseWarnings[0].has_value(),
        "the entry carries a warning beside its result (FR-070, E-07)");
  const sg::NoiseWarning warning =
      rich->noiseWarnings.size() == 1 && rich->noiseWarnings[0].has_value()
      ? *rich->noiseWarnings[0]
      : sg::NoiseWarning {};
  check(warning.spinning == 3 && warning.runnableCpus == 16
            && warning.sharesOneCpu,
        "the stored warning answers its three values (FR-070, E-06)");
}

// Each error code the extension can answer, planted once in a document
// that carries a noise record: the header object, the version text under
// it, and the three values of the record the reader now stores. Every
// refusal names the pointer of the field it stands in (FR-067, FR-070,
// specs/026 contracts/reader-extension.md).
constexpr struct ExtensionPlant
{
  const char* key;
  const char* replacement;
  const char* pointer;
} kExtensionPlants[] = {
    {"speedgun", "5", "/speedgun"},
    {"version", "5", "/speedgun/version"},
    {"spinning", "\"x\"", "/benchmarks/0/noiseWarning/spinning"},
    {"runnableCpus", "\"x\"", "/benchmarks/0/noiseWarning/runnableCpus"},
    {"sharesOneCpu", "\"x\"", "/benchmarks/0/noiseWarning/sharesOneCpu"},
};

auto eachExtensionErrorCodeRefuses(const std::string& text) -> void
{
  std::string base = text;
  const std::size_t at = base.find(",\"noiseWarning\":null");
  if (at == std::string::npos) {
    sg::test::fail("no noise-warning key in the written document");
    return;
  }
  base.replace(at,
               std::strlen(",\"noiseWarning\":null"),
               ",\"noiseWarning\":{\"spinning\":3,\"runnableCpus\":16,"
               "\"sharesOneCpu\":true}");
  sg::test::writeText("harness_json_load_extension.json", base);
  check(sg::loadReport("harness_json_load_extension.json").has_value(),
        "the document that carries a noise record loads (FR-070)");

  for (const ExtensionPlant& plant : kExtensionPlants) {
    std::string broken = base;
    if (!replaceValue(broken, plant.key, plant.replacement)) {
      std::printf("  no '%s' in the document\n", plant.key);
      sg::test::fail("the planted key has no place to stand");
      continue;
    }
    sg::test::writeText("harness_json_load_extension.json", broken);
    const auto refused = sg::loadReport("harness_json_load_extension.json");
    if (refused.has_value()) {
      std::printf("  '%s' holding %s loaded\n", plant.key, plant.replacement);
      sg::test::fail("a value of the wrong kind the extension reads loads "
                     "anyway (FR-067, FR-070)");
      continue;
    }
    if (refused.error().find(plant.pointer) == std::string::npos) {
      std::printf("  '%s' refused without '%s': %s\n",
                  plant.key,
                  plant.pointer,
                  refused.error().c_str());
      sg::test::fail(
          "the refusal names no pointer for the field (FR-067, " "SC-007)");
    }
  }
}

// A key the reader's node table does not list, planted in the one object
// the report schema leaves open: the document root. The pass fields the
// parallel lane adds arrive this way, so a document carrying one loads
// and answers the values of the same document without it (FR-073, PC-4).
auto anUnknownKeyInAnOpenObjectLoads(const std::string& text) -> void
{
  constexpr const char* const kPlant = ",\"passCount\":3";

  sg::test::writeText("harness_json_load_plainkey.json", text);
  const auto plain = sg::loadReport("harness_json_load_plainkey.json");
  if (!plain.has_value()) {
    std::printf("  %s\n", plain.error().c_str());
    sg::test::fail("the written document stopped loading");
    return;
  }

  const std::size_t open = text.find(",\"speedgun\"");
  if (open == std::string::npos) {
    sg::test::fail("the planted key has no place to stand");
    return;
  }

  std::string extended = text;
  extended.insert(open, kPlant);
  sg::test::writeText("harness_json_load_unknownkey.json", extended);
  const auto loaded = sg::loadReport("harness_json_load_unknownkey.json");
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    sg::test::fail("a key the node table does not list inside an open "
                   "object refuses the document (FR-073, PC-4)");
    return;
  }
  check(sg::test::sameReport(*loaded, *plain),
        "the document carrying the unknown key answers the values of the "
        "document without it (FR-073, PC-4)");
}

// The written document carries the sampling overhead floor the plan
// measures from the machine at the start of the attempt, so the digits
// of that one number stand nowhere the planted tables can name. The
// tables read a document with that number pinned to the value they
// plant against; every other byte stands as the writer left it
// (specs/023 SC-007, SC-009).
auto pinOverheadFloor(std::string text) -> std::string
{
  constexpr std::string_view kKey = "\"overheadFloorNs\":";
  for (std::size_t at = text.find(kKey); at != std::string::npos;) {
    const auto start = at + kKey.size();
    std::size_t end = start;
    while (end < text.size()
           && (std::isdigit(static_cast<unsigned char>(text[end])) != 0
               || text[end] == '-' || text[end] == '+' || text[end] == '.'
               || text[end] == 'e' || text[end] == 'E'))
    {
      ++end;
    }
    text.replace(start, end - start, "10.0");
    at = text.find(kKey, start + 4);
  }
  return text;
}

}  // namespace

// specs/034 T018, FR-006, FR-024, SC-004, R-08: the pre-check record
// of the document closes on itself: the key stands after `jitter` and
// before `benchmarks`, the round trip through the reader answers the
// record field by field, the absent figures carry their nulls, and a
// run without the pre-check writes no key at all.
auto thePrecheckRecordRoundTrips() -> void
{
  sg::PrecheckRecord record;
  record.serial = true;
  record.windowMs = 200;
  record.skippedGroups = {1};
  sg::PrecheckCpuRecord planned;
  planned.cpu = 3;
  planned.planned = true;
  planned.stats.gapCount = 2;
  planned.stats.gapsPerSecond = 11.5;
  planned.stats.maxNs = 9000;
  planned.attribution.interrupts = 42;
  planned.attribution.softirqs.emplace_back("TIMER",
                                            std::optional<std::uint64_t>(7));
  planned.attribution.softirqs.emplace_back("NET_RX", std::nullopt);
  planned.attribution.switches = 5;
  record.cpus.push_back(planned);
  sg::PrecheckCpuRecord candidate;
  candidate.cpu = 4;
  record.cpus.push_back(candidate);
  record.host.isolcpus = "2-3";
  record.host.governors.emplace_back(3U,
                                     std::optional<std::string>("performance"));
  record.host.irqAffinity.emplace_back(3U,
                                       std::vector<std::uint32_t> {24U, 25U});
  sg::ReportData data;
  data.precheck = record;
  check(sg::detail::writeReport(kWritten, data, {}, std::nullopt, record),
        "the writer wrote the document with the record (FR-006)");
  const std::string text = sg::test::readText(kWritten);
  const auto precheckAt = text.find("\"precheck\"");
  const auto benchmarksAt = text.find("\"benchmarks\"");
  check(precheckAt != std::string::npos && precheckAt < benchmarksAt,
        "the precheck key stands before benchmarks (D-8); the key order "
        "the x-order list states the reader checks itself");
  sg::test::writeText("harness_json_load_precheck.json", text);
  const auto loaded = sg::loadReport(kWritten);
  check(loaded.has_value(),
        "the reader read the document with the record (FR-024)");
  if (loaded.has_value()) {
    check(sg::test::sameReport(*loaded, data),
          "the record reads back field by field (FR-024, SC-004)");
  }

  sg::ReportData bare;
  check(sg::detail::writeReport(kAgain, bare),
        "the writer wrote the record-free document (FR-024)");
  const auto reloaded = sg::loadReport(kAgain);
  check(reloaded.has_value() && sg::test::sameReport(*reloaded, bare),
        "the record-free document loads with no record (FR-024)");
  check(sg::test::readText(kAgain).find("\"precheck\"") == std::string::npos,
        "a run without the pre-check writes no key (FR-024)");

  // The record whose optional shapes stand empty writes and reads
  // back: the host settings unstated, no governor, no affinity, the
  // candidate figures absent (FR-024).
  sg::PrecheckRecord minimal;
  minimal.serial = false;
  minimal.windowMs = 1;
  sg::PrecheckCpuRecord quiet;
  quiet.cpu = 1;
  minimal.cpus.push_back(quiet);
  minimal.host.governors.emplace_back(1U, std::nullopt);
  minimal.host.irqAffinity.emplace_back(1U, std::vector<std::uint32_t> {});
  sg::PrecheckRecord sparse;
  sparse.windowMs = 1;
  sg::PrecheckCpuRecord sparseCpu;
  sparseCpu.cpu = 1;
  sparse.cpus.push_back(sparseCpu);
  sparse.host.irqAffinity.emplace_back(
      1U, std::vector<std::uint32_t> {24U, 25U, 26U});
  sg::ReportData sparseData;
  sparseData.precheck = sparse;
  check(sg::detail::writeReport(kAgain, sparseData, {}, std::nullopt, sparse),
        "the writer wrote the governor-free record (FR-006)");
  const auto sparseLoaded = sg::loadReport(kAgain);
  check(sparseLoaded.has_value(),
        "the reader read the governor-free record (FR-024)");
  if (sparseLoaded.has_value()) {
    check(sg::test::sameReport(*sparseLoaded, sparseData),
          "the governor-free record reads back field by field (FR-024)");
  }
  sg::ReportData minimalData;
  minimalData.precheck = minimal;
  check(sg::detail::writeReport(kAgain, minimalData, {}, std::nullopt, minimal),
        "the writer wrote the minimal record (FR-006)");
  const auto minimalLoaded = sg::loadReport(kAgain);
  check(minimalLoaded.has_value(),
        "the reader read the minimal record (FR-024)");
  if (minimalLoaded.has_value()) {
    check(sg::test::sameReport(*minimalLoaded, minimalData),
          "the minimal record reads back field by field (FR-024)");
  }
}

// specs/034 T018, FR-024, FR-029: every key of the pre-check record
// refuses its own wrong kind, its own missing presence, and its own
// container kind, and every refusal names the pointer of the key it
// answers, the way the standing records refuse (FR-029).
auto thePrecheckRefusalsNameTheirFields() -> void
{
  struct Plant
  {
    const char* find;
    const char* instead;
    const char* pointer;
  };

  const std::string text =
      sg::test::readText("harness_json_load_precheck.json");
  const std::string plannedStats =
      R"json("stats":{"gapCount":2,"gapsPerSecond":11.5,"minNs":0,"medianNs":0,"meanNs":0.0,"p90Ns":0,"p99Ns":0,"p999Ns":0,"p9999Ns":0,"maxNs":9000,"totalNs":0,"totalPercent":0.0,"runtimeNs":0,"thresholdNs":0,"loopPeriodNs":0,"droppedGaps":0})json";
  const std::string fullAttr =
      R"json("attribution":{"interrupts":42,"softirqs":{"TIMER":7,"NET_RX":null},"switchDelta":5,"scheduleDelta":null})json";
  const std::string fullHost =
      R"json("hostSettings":{"isolcpus":"2-3","nohz_full":null,"rcu_nocbs":null,"governors":{"3":"performance"},"irqAffinity":{"3":"24,25"}})json";
  const std::string cpusFull =
      R"json("cpus":[{"cpu":3,"role":"planned","stats":{"gapCount":2,"gapsPerSecond":11.5,"minNs":0,"medianNs":0,"meanNs":0.0,"p90Ns":0,"p99Ns":0,"p999Ns":0,"p9999Ns":0,"maxNs":9000,"totalNs":0,"totalPercent":0.0,"runtimeNs":0,"thresholdNs":0,"loopPeriodNs":0,"droppedGaps":0},"attribution":{"interrupts":42,"softirqs":{"TIMER":7,"NET_RX":null},"switchDelta":5,"scheduleDelta":null}},{"cpu":4,"role":"candidate","stats":{"gapCount":0,"gapsPerSecond":0.0,"minNs":0,"medianNs":0,"meanNs":0.0,"p90Ns":0,"p99Ns":0,"p999Ns":0,"p9999Ns":0,"maxNs":0,"totalNs":0,"totalPercent":0.0,"runtimeNs":0,"thresholdNs":0,"loopPeriodNs":0,"droppedGaps":0},"attribution":{"interrupts":null,"softirqs":{},"switchDelta":null,"scheduleDelta":null}}])json";
  const std::string plannedStatsComma = plannedStats + ",";
  const std::string cpusFullComma = cpusFull + ",";
  const std::string fullHostComma = fullHost + ",";
  const std::string fullAttrLead = "," + fullAttr;
  const std::string irqAffinityLead = ",\"irqAffinity\":{\"3\":\"24,25\"}";
  const Mutation plants[] = {
      {"\"serial\":true,", "", "/precheck"},
      {"\"serial\":true", "\"serial\":1", "/precheck/serial"},
      {"\"windowMs\":200,", "", "/precheck"},
      {"\"windowMs\":200", "\"windowMs\":\"x\"", "/precheck/windowMs"},
      {cpusFull.c_str(), "\"cpus\":{}", "/precheck"},
      {cpusFullComma.c_str(), "", "/precheck"},
      {"\"cpu\":3", "\"cpu\":\"x\"", "/precheck/cpus/0/cpu"},
      {"\"cpu\":3,", "", "/precheck/cpus/0"},
      {"\"role\":\"planned\"", "\"role\":1", "/precheck/cpus/0/role"},
      {"\"role\":\"planned\"", "\"role\":\"bogus\"", "/precheck/cpus/0/role"},
      {"\"role\":\"planned\",", "", "/precheck/cpus/0"},
      {plannedStats.c_str(), "\"stats\":[]", "/precheck/cpus/0/stats"},
      {plannedStatsComma.c_str(), "", "/precheck/cpus/0"},
      {"\"gapCount\":2",
       "\"gapCount\":\"x\"",
       "/precheck/cpus/0/stats/gapCount"},
      {"\"gapCount\":2,", "", "/precheck/cpus/0/stats"},
      {"\"gapsPerSecond\":11.5",
       "\"gapsPerSecond\":\"x\"",
       "/precheck/cpus/0/stats/gapsPerSecond"},
      {"\"minNs\":0", "\"minNs\":\"x\"", "/precheck/cpus/0/stats/minNs"},
      {"\"medianNs\":0",
       "\"medianNs\":\"x\"",
       "/precheck/cpus/0/stats/medianNs"},
      {"\"meanNs\":0.0", "\"meanNs\":\"x\"", "/precheck/cpus/0/stats/meanNs"},
      {"\"p90Ns\":0", "\"p90Ns\":\"x\"", "/precheck/cpus/0/stats/p90Ns"},
      {"\"p99Ns\":0", "\"p99Ns\":\"x\"", "/precheck/cpus/0/stats/p99Ns"},
      {"\"p999Ns\":0", "\"p999Ns\":\"x\"", "/precheck/cpus/0/stats/p999Ns"},
      {"\"p9999Ns\":0", "\"p9999Ns\":\"x\"", "/precheck/cpus/0/stats/p9999Ns"},
      {"\"maxNs\":9000", "\"maxNs\":\"x\"", "/precheck/cpus/0/stats/maxNs"},
      {"\"totalNs\":0", "\"totalNs\":\"x\"", "/precheck/cpus/0/stats/totalNs"},
      {"\"totalPercent\":0.0",
       "\"totalPercent\":\"x\"",
       "/precheck/cpus/0/stats/totalPercent"},
      {"\"runtimeNs\":0",
       "\"runtimeNs\":\"x\"",
       "/precheck/cpus/0/stats/runtimeNs"},
      {"\"thresholdNs\":0",
       "\"thresholdNs\":\"x\"",
       "/precheck/cpus/0/stats/thresholdNs"},
      {"\"loopPeriodNs\":0",
       "\"loopPeriodNs\":\"x\"",
       "/precheck/cpus/0/stats/loopPeriodNs"},
      {"\"droppedGaps\":0",
       "\"droppedGaps\":\"x\"",
       "/precheck/cpus/0/stats/droppedGaps"},
      {fullAttr.c_str(), "\"attribution\":[]", "/precheck/cpus/0/attribution"},
      {fullAttrLead.c_str(), "", "/precheck/cpus/0"},
      {"\"interrupts\":42",
       "\"interrupts\":\"x\"",
       "/precheck/cpus/0/attribution/interrupts"},
      {"\"interrupts\":42,", "", "/precheck/cpus/0/attribution"},
      {"\"softirqs\":{\"TIMER\":7,\"NET_RX\":null}",
       "\"softirqs\":1",
       "/precheck/cpus/0/attribution/softirqs"},
      {"\"softirqs\":{\"TIMER\":7,\"NET_RX\":null},",
       "",
       "/precheck/cpus/0/attribution"},
      {"\"TIMER\":7",
       "\"TIMER\":\"x\"",
       "/precheck/cpus/0/attribution/softirqs/TIMER"},
      {"\"switchDelta\":5",
       "\"switchDelta\":true",
       "/precheck/cpus/0/attribution/switchDelta"},
      {"\"switchDelta\":5,\"scheduleDelta\":null",
       "\"switchDelta\":5,\"scheduleDelta\":\"x\"",
       "/precheck/cpus/0/attribution/scheduleDelta"},
      {fullHost.c_str(), "\"hostSettings\":[]", "/precheck/hostSettings"},
      {fullHostComma.c_str(), "", "/precheck"},
      {"\"isolcpus\":\"2-3\"",
       "\"isolcpus\":1",
       "/precheck/hostSettings/isolcpus"},
      {"\"isolcpus\":\"2-3\",", "", "/precheck/hostSettings"},
      {"\"governors\":{\"3\":\"performance\"}",
       "\"governors\":[]",
       "/precheck/hostSettings/governors"},
      {"\"governors\":{\"3\":\"performance\"}",
       "\"governors\":{\"x\":\"performance\"}",
       "/precheck/hostSettings/governors/x"},
      {"\"governors\":{\"3\":\"performance\"},", "", "/precheck/hostSettings"},
      {"\"3\":\"performance\"",
       "\"3\":1",
       "/precheck/hostSettings/governors/3"},
      {"\"nohz_full\":null",
       "\"nohz_full\":1",
       "/precheck/hostSettings/nohz_full"},
      {"\"rcu_nocbs\":null",
       "\"rcu_nocbs\":1",
       "/precheck/hostSettings/rcu_nocbs"},
      {"\"governors\":{\"3\":\"performance\"}",
       "\"governors\":{\"3x\":\"p\"}",
       "/precheck/hostSettings/governors/3x"},
      {"\"irqAffinity\":{\"3\":\"24,25\"}",
       "\"irqAffinity\":1",
       "/precheck/hostSettings/irqAffinity"},
      {"\"irqAffinity\":{\"3\":\"24,25\"}",
       "\"irqAffinity\":{\"x\":\"24,25\"}",
       "/precheck/hostSettings/irqAffinity/x"},
      {irqAffinityLead.c_str(), "", "/precheck/hostSettings"},
      {"\"irqAffinity\":{\"3\":\"24,25\"}",
       "\"irqAffinity\":{\"3\":1}",
       "/precheck/hostSettings/irqAffinity/3"},
      {"\"24,25\"", "\"a\"", "/precheck/hostSettings/irqAffinity/3"},
      {"\"24,25\"", "\"2x5\"", "/precheck/hostSettings/irqAffinity/3"},
      {"\"skippedGroups\":[1]",
       "\"skippedGroups\":1",
       "/precheck/skippedGroups"},
      {",\"skippedGroups\":[1]", "", "/precheck"},
      {"\"skippedGroups\":[1]",
       "\"skippedGroups\":[\"x\"]",
       "/precheck/skippedGroups/0"},
  };
  plantsRefuseAtTheirPointers(text, plants, "FR-024, FR-029");
}

// specs/037-execution-graphs FR-023, FR-028, FR-029, SC-007: every key of the
// graph record refuses its own missing presence, its own wrong kind and its
// own container kind, and every refusal names the pointer of the field it
// answers, the shape the standing pre-check arms give. The record of one
// threaded run stands written whole first, so each plant moves one key of the
// written bytes and the reader answers the record's own read.
auto theGraphRefusalsNameTheirFields() -> void
{
  const std::string nodesArray =
      R"json([{"name":"a","slot":0,"outcome":"finished"},{"name":"b","slot":1,"outcome":"failed"},{"name":"c","slot":2,"outcome":"held"},{"name":"d","slot":3,"outcome":"stopped"}])json";
  const std::string edgesArray =
      R"json([{"from":"a","to":"b"},{"from":"b","to":"c"}])json";
  const std::string heldArray = R"json([{"node":"c","reason":"b"}])json";
  const std::string stoppedArray = R"json(["d"])json";
  const std::string runsArray = "[{\"benchmark\":\"bmGraphLoad\",\"nodes\":"
      + nodesArray + ",\"edges\":" + edgesArray + ",\"held\":" + heldArray
      + ",\"stopped\":" + stoppedArray + "}]";

  auto run = sg::GraphRunRecord {};
  run.benchmark = "bmGraphLoad";
  run.nodes = {
      sg::GraphNodeRecord {.name = "a", .slot = 0U, .outcome = "finished"},
      sg::GraphNodeRecord {.name = "b", .slot = 1U, .outcome = "failed"},
      sg::GraphNodeRecord {.name = "c", .slot = 2U, .outcome = "held"},
      sg::GraphNodeRecord {.name = "d", .slot = 3U, .outcome = "stopped"},
  };
  run.edges = {
      sg::GraphEdgeRecord {.from = "a", .to = "b"},
      sg::GraphEdgeRecord {.from = "b", .to = "c"},
  };
  run.held = {sg::GraphHeldRecord {.node = "c", .reason = "b"}};
  run.stopped = {"d"};
  auto record = sg::GraphRecord {};
  record.runs.push_back(run);

  auto data = sg::ReportData {};
  data.graph = record;
  check(sg::detail::writeReport(
            kAgain, data, {}, std::nullopt, std::nullopt, record),
        "the writer wrote the document with the graph record (FR-023)");
  const std::string text = sg::test::readText(kAgain);
  check(text.find("\"graph\":{\"runs\":" + runsArray) != std::string::npos,
        "the written bytes carry the record every plant stands in (FR-023)");
  const auto loaded = sg::loadReport(kAgain);
  check(loaded.has_value() && loaded->graph.has_value(),
        "the reader reads the record back (FR-023)");
  if (loaded.has_value() && loaded->graph.has_value()) {
    const auto& entry = loaded->graph->runs[0];
    check(loaded->graph->runs.size() == 1 && entry.nodes.size() == 4
              && entry.edges.size() == 2 && entry.held.size() == 1
              && entry.stopped.size() == 1,
          "every array of the run reads back (FR-023)");
  }

  const std::string runsKey = "\"runs\":" + runsArray;
  const std::string nodesKey = "\"nodes\":" + nodesArray;
  const std::string edgesKey = "\"edges\":" + edgesArray;
  const std::string heldKey = "\"held\":" + heldArray;
  const std::string stoppedKey = "\"stopped\":" + stoppedArray;
  // The plants that drop one key stand at the key plus its separator, and
  // the separator lives in the row for as long as the row does.
  const std::string nodesComma = nodesKey + ",";
  const std::string edgesComma = edgesKey + ",";
  const std::string heldComma = heldKey + ",";
  const std::string commaStopped = "," + stoppedKey;
  const Mutation plants[] = {
      {runsKey.c_str(), "", "/graph"},
      {runsKey.c_str(), "\"runs\":1", "/graph/runs"},
      {"\"benchmark\":\"bmGraphLoad\",", "", "/graph/runs/0"},
      {"\"benchmark\":\"bmGraphLoad\"",
       "\"benchmark\":1",
       "/graph/runs/0/benchmark"},
      {nodesComma.c_str(), "", "/graph/runs/0"},
      {nodesKey.c_str(), "\"nodes\":1", "/graph/runs/0/nodes"},
      {edgesComma.c_str(), "", "/graph/runs/0"},
      {edgesKey.c_str(), "\"edges\":1", "/graph/runs/0/edges"},
      {heldComma.c_str(), "", "/graph/runs/0"},
      {heldKey.c_str(), "\"held\":1", "/graph/runs/0/held"},
      {commaStopped.c_str(), "", "/graph/runs/0"},
      {stoppedKey.c_str(), "\"stopped\":1", "/graph/runs/0/stopped"},
      {stoppedKey.c_str(), "\"stopped\":[1]", "/graph/runs/0/stopped/0"},
      {"\"name\":\"a\",", "", "/graph/runs/0/nodes/0"},
      {"\"name\":\"a\"", "\"name\":1", "/graph/runs/0/nodes/0/name"},
      {"\"slot\":0,", "", "/graph/runs/0/nodes/0"},
      {"\"slot\":0", "\"slot\":\"x\"", "/graph/runs/0/nodes/0/slot"},
      {",\"outcome\":\"finished\"", "", "/graph/runs/0/nodes/0"},
      {"\"outcome\":\"finished\"",
       "\"outcome\":1",
       "/graph/runs/0/nodes/0/outcome"},
      {"\"from\":\"a\",", "", "/graph/runs/0/edges/0"},
      {"\"from\":\"a\"", "\"from\":1", "/graph/runs/0/edges/0/from"},
      {",\"to\":\"b\"", "", "/graph/runs/0/edges/0"},
      {"\"to\":\"b\"", "\"to\":1", "/graph/runs/0/edges/0/to"},
      {"\"node\":\"c\",", "", "/graph/runs/0/held/0"},
      {"\"node\":\"c\"", "\"node\":1", "/graph/runs/0/held/0/node"},
      {",\"reason\":\"b\"", "", "/graph/runs/0/held/0"},
      {"\"reason\":\"b\"", "\"reason\":1", "/graph/runs/0/held/0/reason"},
  };
  plantsRefuseAtTheirPointers(text, plants, "FR-023, FR-029");
}

// specs/043 T024, US4, FR-020, R-17: the series record the reader reads
// back. The document carries the method word, two closed series items with
// their 21 keys in the written order, and the period list of the first cpu;
// the entry carries the same item with no method and no periods. Every
// figure reads back equal, the two keys the method leaves out read back at
// 0, a key of the wrong kind refuses with its own pointer, a key the closed
// item does not list stands refused by the checker, and the two keys
// standing absent leave the run record and the folds empty (FR-006, FR-011,
// FR-018, FR-020).
constexpr const char* const kSeries = R"JSON(
{"schemaVersion":"1.12","speedgun":{"version":"0.29.0","buildType":"Release"},
 "jitter":{"mode":"ALONGSIDE","thresholdNs":200,"loopPeriodNs":20,"method":"FWQ",
  "series":[{"cpu":2,"workCount":7,"unitTimeNs":1000,"targetNs":1000,"quantumNs":100000,"calibrationSpanNs":500,"sampleCount":4096,"minValue":900,"medianValue":1000,"meanValue":1000.5,"p90Value":1100,"p99Value":1200,"p999Value":1300,"p9999Value":1400,"maxValue":1500,"noisePercent":0.25,"seriesRuntimeNs":4096000,"droppedSamples":3,"resampleStepNs":1000,"spectrumPoints":2048,"periods":[{"periodNs":1000000,"powerRatio":1.0}]},
   {"cpu":3,"workCount":8,"unitTimeNs":1001,"targetNs":1001,"quantumNs":100001,"calibrationSpanNs":501,"sampleCount":1,"minValue":2,"medianValue":3,"meanValue":4.5,"p90Value":5,"p99Value":6,"p999Value":7,"p9999Value":8,"maxValue":9,"noisePercent":1.5,"seriesRuntimeNs":10,"droppedSamples":0,"resampleStepNs":0,"spectrumPoints":0,"periods":[]}],
  "cpus":[2,3],"stats":[{"cpu":2,"gapCount":1,"gapsPerSecond":1000.0,"minNs":200,"medianNs":200,"meanNs":200.0,"p90Ns":210,"p99Ns":220,"p999Ns":230,"p9999Ns":240,"maxNs":250,"totalNs":260,"totalPercent":0.5,"runtimeNs":270,"thresholdNs":280,"loopPeriodNs":29,"droppedGaps":30},
   {"cpu":3,"gapCount":0,"gapsPerSecond":0.0,"minNs":0,"medianNs":0,"meanNs":0.0,"p90Ns":0,"p99Ns":0,"p999Ns":0,"p9999Ns":0,"maxNs":0,"totalNs":0,"totalPercent":0.0,"runtimeNs":0,"thresholdNs":280,"loopPeriodNs":29,"droppedGaps":0}],
  "droppedGaps":0,"disclosure":"the run's disclosure"},
 "benchmarks":[{"name":"bmSeries","suite":"bmSeries","case":"bmSeries",
  "outcome":"MEASURED","reason":null,"hideRepetitionRows":false,
  "timeUnit":"NANOSECONDS","primaryIsCpu":true,"pauseSamplesFolded":false,
  "percentilePoints":[],"contexts":[],"noiseWarning":null,
  "jitter":{"stats":[{"cpu":3,"gapCount":4,"gapsPerSecond":400.0,"minNs":300,"medianNs":310,"meanNs":320.0,"p90Ns":330,"p99Ns":340,"p999Ns":350,"p9999Ns":360,"maxNs":370,"totalNs":380,"totalPercent":0.6,"runtimeNs":390,"thresholdNs":400,"loopPeriodNs":41,"droppedGaps":42}],
   "series":[{"cpu":3,"workCount":11,"unitTimeNs":12,"targetNs":13,"quantumNs":14,"calibrationSpanNs":15,"sampleCount":16,"minValue":17,"medianValue":18,"meanValue":19.5,"p90Value":20,"p99Value":21,"p999Value":22,"p9999Value":23,"maxValue":24,"noisePercent":2.5,"seriesRuntimeNs":26,"droppedSamples":27,"resampleStepNs":28,"spectrumPoints":29}]},
  "rows":[],"metricLabels":[],"counterLabels":[],"counterStats":[],
  "statisticValues":[],"metricMethod":"AUTO","passCount":0,
  "passSpread":null,"spreadWarning":false}]}
)JSON";

// The span of the series array the document carries at or after `from`,
// the whole array with its own key: the plants that take the array out, and
// the plant that gives it a container of another kind, both need the span
// (FR-020).
auto seriesSpan(const std::string& text,
                std::size_t from) -> std::pair<std::size_t, std::size_t>
{
  const auto at = text.find("\"series\":[", from);
  check(at != std::string::npos,
        "the document carries a series array (FR-020)");
  std::size_t depth = 0;
  std::size_t end = at + std::strlen("\"series\":");
  for (; end < text.size(); ++end) {
    if (text.at(end) == '[') {
      ++depth;
      continue;
    }
    if (text.at(end) == ']') {
      --depth;
      if (depth == 0) {
        ++end;
        break;
      }
    }
  }
  // The separator beside the array rides the span, so the text the plant
  // leaves stands whole: the root object carries a comma after the array,
  // and the per-entry object carries the comma before it.
  auto after = end;
  while (after < text.size()
         && std::isspace(static_cast<unsigned char>(text.at(after))) != 0)
  {
    ++after;
  }
  if (after < text.size() && text.at(after) == ',') {
    return {at, after + 1 - at};
  }
  auto before = at;
  while (before > 0
         && std::isspace(static_cast<unsigned char>(text.at(before - 1))) != 0)
  {
    --before;
  }
  if (before > 0 && text.at(before - 1) == ',') {
    --before;
  }
  return {before, end - before};
}

// The document with every series array taken out, the reading of a document
// written by a build that kept no series (FR-020).
auto withoutSeries(std::string text) -> std::string
{
  for (std::size_t at = text.find("\"series\":["); at != std::string::npos;
       at = text.find("\"series\":[", at))
  {
    const auto span = seriesSpan(text, at);
    text.erase(span.first, span.second);
  }
  return text;
}

// FR-020, R-17: every key of the run-level series record reads back.
auto theSeriesRecordReadsBackEveryKey() -> void
{
  sg::test::writeText("harness_json_load_series.json", kSeries);
  const auto loaded = sg::loadReport("harness_json_load_series.json");
  check(loaded.has_value(), "the document carrying the series loads (FR-020)");
  if (!loaded.has_value()) {
    std::printf("  %s\n", loaded.error().c_str());
    return;
  }
  check(loaded->jitterRun.has_value()
            && loaded->jitterRun->method == sg::JitterMethod::FWQ,
        "theSeriesRecordReadsBackEveryKey: the run record reads back the "
        "method word (FR-020, E-25)");
  const auto& folds = loaded->runJitterStats;
  check(folds.has_value() && folds->size() == 2,
        "theSeriesRecordReadsBackEveryKey: the run record carries one fold "
        "per probe cpu (E-07)");
  if (!folds.has_value() || folds->size() != 2) {
    return;
  }
  check(folds->at(0).cpu == 2 && folds->at(1).cpu == 3,
        "the run-level folds carry the probe cpus ascending (E-07)");
  const auto& first = folds->at(0).series;
  check(first.has_value(),
        "theSeriesRecordReadsBackEveryKey: the fold reads back its series "
        "(FR-020)");
  if (!first.has_value()) {
    return;
  }
  check(first->workCount == 7 && first->unitTimeNs == 1000
            && first->targetNs == 1000 && first->quantumNs == 100000
            && first->calibrationSpanNs == 500,
        "theSeriesRecordReadsBackEveryKey: the five calibration figures read "
        "back (FR-011, E-22)");
  check(first->sampleCount == 4096 && first->minValue == 900
            && first->medianValue == 1000 && first->p90Value == 1100
            && first->p99Value == 1200 && first->p999Value == 1300
            && first->p9999Value == 1400 && first->maxValue == 1500,
        "theSeriesRecordReadsBackEveryKey: the ten integral figures read "
        "back (FR-007)");
  check(sg::test::sameValue(first->meanValue, 1000.5)
            && sg::test::sameValue(first->noisePercent, 0.25),
        "theSeriesRecordReadsBackEveryKey: the two real figures read back "
        "(FR-007, FR-009)");
  check(first->seriesRuntimeNs == 4096000 && first->droppedSamples == 3
            && first->resampleStepNs == 1000 && first->spectrumPoints == 2048,
        "theSeriesRecordReadsBackEveryKey: the runtime, the drop count and "
        "the two spectrum keys read back (FR-017, FR-022)");
  const auto& second = folds->at(1).series;
  check(second.has_value() && second->workCount == 8
            && second->unitTimeNs == 1001 && second->targetNs == 1001
            && second->quantumNs == 100001 && second->calibrationSpanNs == 501
            && second->sampleCount == 1 && second->minValue == 2
            && second->medianValue == 3 && second->p90Value == 5
            && second->p99Value == 6 && second->p999Value == 7
            && second->p9999Value == 8 && second->maxValue == 9
            && second->seriesRuntimeNs == 10 && second->droppedSamples == 0
            && second->resampleStepNs == 0 && second->spectrumPoints == 0
            && sg::test::sameValue(second->meanValue, 4.5)
            && sg::test::sameValue(second->noisePercent, 1.5),
        "theSeriesRecordReadsBackEveryKey: the second cpu's item reads back "
        "every key (FR-020)");
}

// FR-018, E-24: the period list reads back, and an empty list reads back
// empty.
auto thePeriodListReadsBack() -> void
{
  const auto loaded = sg::loadReport("harness_json_load_series.json");
  if (!loaded.has_value() || !loaded->runJitterStats.has_value()
      || loaded->runJitterStats->size() != 2)
  {
    sg::test::fail("the series folds stand unread (FR-020)");
    return;
  }
  const auto& first = loaded->runJitterStats->at(0).series;
  check(first.has_value() && first->periods.size() == 1
            && first->periods[0].periodNs == 1000000
            && sg::test::sameValue(first->periods[0].powerRatio, 1.0),
        "thePeriodListReadsBack: the period list reads back item by item "
        "(FR-018, E-24)");
  const auto& second = loaded->runJitterStats->at(1).series;
  check(second.has_value() && second->periods.empty(),
        "thePeriodListReadsBack: an empty list reads back empty (FR-018)");
}

// FR-006, FR-020: the two keys stand out of the schema's required list, so
// a document that carries neither reads, and the two members it leaves
// behind stand empty.
auto aMissingMethodKeyLeavesTheRunRecordEmpty() -> void
{
  std::string text(kSeries);
  const auto at = text.find("\"method\":\"FWQ\",");
  check(at != std::string::npos,
        "the document carries the method key (FR-006)");
  text.erase(at, std::strlen("\"method\":\"FWQ\","));
  sg::test::writeText("harness_json_load_nomethod.json", text);
  const auto loaded = sg::loadReport("harness_json_load_nomethod.json");
  check(loaded.has_value(),
        "aMissingMethodKeyLeavesTheRunRecordEmpty: a document with no method "
        "key loads (FR-006)");
  if (!loaded.has_value()) {
    return;
  }
  check(!loaded->jitterRun.has_value(),
        "aMissingMethodKeyLeavesTheRunRecordEmpty: the run record stands "
        "empty (FR-020, E-25)");
  check(loaded->runJitterStats.has_value()
            && loaded->runJitterStats->size() == 2
            && loaded->runJitterStats->at(0).series.has_value(),
        "aMissingMethodKeyLeavesTheRunRecordEmpty: the series reads on "
        "without the method key (FR-020)");
}

auto aMissingSeriesKeyLeavesTheFoldsEmpty() -> void
{
  sg::test::writeText("harness_json_load_noseries.json",
                      withoutSeries(std::string(kSeries)));
  const auto loaded = sg::loadReport("harness_json_load_noseries.json");
  check(loaded.has_value(),
        "aMissingSeriesKeyLeavesTheFoldsEmpty: a document with no series "
        "array loads (FR-006)");
  if (!loaded.has_value()) {
    return;
  }
  check(loaded->runJitterStats.has_value()
            && loaded->runJitterStats->size() == 2
            && !loaded->runJitterStats->at(0).series.has_value()
            && !loaded->runJitterStats->at(1).series.has_value(),
        "aMissingSeriesKeyLeavesTheFoldsEmpty: the run-level folds carry no "
        "series (FR-020)");
  check(loaded->results.size() == 1
            && loaded->results[0].jitterFolds.size() == 1
            && !loaded->results[0].jitterFolds.at(0).series.has_value(),
        "aMissingSeriesKeyLeavesTheFoldsEmpty: the entry's fold carries no "
        "series (FR-020, E-08)");
  check(loaded->jitterRun.has_value()
            && loaded->jitterRun->method == sg::JitterMethod::FWQ,
        "aMissingSeriesKeyLeavesTheFoldsEmpty: the run record reads the "
        "method the document carries (FR-020, E-25)");
}

// FR-020, FR-029, SC-007: one plant per new key, each of a kind the reader
// does not take, each refused with the pointer of the key it stands at.
constexpr Mutation kPlants[] = {
    {"\"method\":\"FWQ\"", "\"method\":1", "/jitter/method"},
    {"\"workCount\":7", "\"workCount\":\"x\"", "/jitter/series/0/workCount"},
    {"\"unitTimeNs\":1000",
     "\"unitTimeNs\":\"x\"",
     "/jitter/series/0/unitTimeNs"},
    {"\"targetNs\":1000", "\"targetNs\":\"x\"", "/jitter/series/0/targetNs"},
    {"\"quantumNs\":100000",
     "\"quantumNs\":\"x\"",
     "/jitter/series/0/quantumNs"},
    {"\"calibrationSpanNs\":500",
     "\"calibrationSpanNs\":\"x\"",
     "/jitter/series/0/calibrationSpanNs"},
    {"\"sampleCount\":4096",
     "\"sampleCount\":\"x\"",
     "/jitter/series/0/sampleCount"},
    {"\"minValue\":900", "\"minValue\":\"x\"", "/jitter/series/0/minValue"},
    {"\"medianValue\":1000",
     "\"medianValue\":\"x\"",
     "/jitter/series/0/medianValue"},
    {"\"meanValue\":1000.5",
     "\"meanValue\":\"x\"",
     "/jitter/series/0/meanValue"},
    {"\"p90Value\":1100", "\"p90Value\":\"x\"", "/jitter/series/0/p90Value"},
    {"\"p99Value\":1200", "\"p99Value\":\"x\"", "/jitter/series/0/p99Value"},
    {"\"p999Value\":1300", "\"p999Value\":\"x\"", "/jitter/series/0/p999Value"},
    {"\"p9999Value\":1400",
     "\"p9999Value\":\"x\"",
     "/jitter/series/0/p9999Value"},
    {"\"maxValue\":1500", "\"maxValue\":\"x\"", "/jitter/series/0/maxValue"},
    {"\"noisePercent\":0.25",
     "\"noisePercent\":\"x\"",
     "/jitter/series/0/noisePercent"},
    {"\"seriesRuntimeNs\":4096000",
     "\"seriesRuntimeNs\":\"x\"",
     "/jitter/series/0/seriesRuntimeNs"},
    {"\"droppedSamples\":3",
     "\"droppedSamples\":\"x\"",
     "/jitter/series/0/droppedSamples"},
    {"\"resampleStepNs\":1000",
     "\"resampleStepNs\":\"x\"",
     "/jitter/series/0/resampleStepNs"},
    {"\"spectrumPoints\":2048",
     "\"spectrumPoints\":\"x\"",
     "/jitter/series/0/spectrumPoints"},
    // The period list rides the item last, and one period item
    // carries the two figures the transform answers.
    {"\"periods\":[{\"periodNs\":1000000,\"powerRatio\":1.0}]",
     "\"periods\":{}",
     "/jitter/series/0/periods"},
    {"\"periodNs\":1000000",
     "\"periodNs\":\"x\"",
     "/jitter/series/0/periods/0/periodNs"},
    {"\"powerRatio\":1.0",
     "\"powerRatio\":\"x\"",
     "/jitter/series/0/periods/0/powerRatio"},
};

auto eachNewKeyOfTheWrongKindStandsRefused() -> void
{
  plantsRefuseAtTheirPointers(kSeries, kPlants, "FR-020, FR-029");

  // The array itself, standing an object: the container kind the reader
  // names beside the item's own keys.
  const auto span = seriesSpan(std::string(kSeries), 0);
  std::string broken(kSeries);
  broken.replace(span.first, span.second, "\"series\":1,");
  sg::test::writeText("harness_json_load_serieskind.json", broken);
  const auto refused = sg::loadReport("harness_json_load_serieskind.json");
  check(!refused.has_value()
            && refused.error().find("/jitter/series") != std::string::npos,
        "eachNewKeyOfTheWrongKindStandsRefused: the series array of another "
        "kind refuses with its pointer (FR-020, FR-029)");
}

// FR-006: the closed item refuses a key the schema does not list, the
// standing checker path the standing closed objects take (FR-010).
auto aClosedSeriesItemRefusesAnUnknownKey() -> void
{
  std::string added(kSeries);
  const auto at = added.find("\"series\":[{");
  check(at != std::string::npos,
        "the document carries the series item (FR-006)");
  added.insert(at + std::strlen("\"series\":[{"), "\"extra\":true,");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, added);
  check(!answer.has_value(),
        "aClosedSeriesItemRefusesAnUnknownKey: the closed series item refuses "
        "a key the schema does not list (FR-006)");
  if (!answer.has_value()) {
    check(answer.error().find("/jitter/series/0") != std::string::npos
              && answer.error().find("lists no key") != std::string::npos,
          "aClosedSeriesItemRefusesAnUnknownKey: the refusal names the item "
          "and the key it lists nowhere (FR-006, FR-010)");
  }
}

auto aClosedPeriodItemRefusesAnUnknownKey() -> void
{
  std::string added(kSeries);
  const auto at = added.find("{\"periodNs\"");
  check(at != std::string::npos, "the document carries a period item (FR-018)");
  added.insert(at + 1, "\"extra\":true,");
  const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, added);
  check(!answer.has_value(),
        "aClosedPeriodItemRefusesAnUnknownKey: the closed period item refuses "
        "a key the schema does not list (FR-006)");
  if (!answer.has_value()) {
    check(answer.error().find("/jitter/series/0/periods/0") != std::string::npos
              && answer.error().find("lists no key") != std::string::npos,
          "aClosedPeriodItemRefusesAnUnknownKey: the refusal names the period "
          "item and the key it lists nowhere (FR-006, FR-010)");
  }
}

// FR-020, FR-029, SC-007: the reader's own refusal arms beside a series
// item. Each row takes out one key the item must carry, and the refusal
// names the pointer of the key the read reached: the unsigned arm, the real
// arm, the two keys of a period item, and the same unsigned arm beside the
// entry's own series (FR-029).
constexpr Mutation kSeriesKeyRefusals[] = {
    {"\"sampleCount\":4096,",
     "",
     "/jitter/series/0: the key 'sampleCount' stands missing"},
    {"\"noisePercent\":0.25,",
     "",
     "/jitter/series/0: the key 'noisePercent' stands missing"},
    {"\"periodNs\":1000000,",
     "",
     "/jitter/series/0/periods/0: the key 'periodNs' stands missing"},
    {",\"powerRatio\":1.0",
     "",
     "/jitter/series/0/periods/0: the key 'powerRatio' stands missing"},
    {"\"sampleCount\":16,",
     "",
     "/benchmarks/0/jitter/series/0: the key 'sampleCount' stands missing"},
};

auto theSeriesKeyRefusalsNameTheirArms() -> void
{
  plantsRefuseAtTheirPointers(kSeries, kSeriesKeyRefusals, "FR-020, FR-029");
}

// FR-020, FR-029, E-07, E-08: the arm of the fold that refuses a series
// item naming a cpu the stats array does not carry, at the run level and
// beside one entry. The refusal names the cpu and the array that lacks it.
auto theSeriesCpuOutsideTheStatsRefuses() -> void
{
  constexpr const char* const kLacks = ", which the stats array does not carry";

  struct CpuPlant
  {
    const char* find;
    const char* replace;
    const char* where;
  };

  constexpr CpuPlant plants[] = {
      {"\"cpu\":3,\"workCount\":8",
       "\"cpu\":9,\"workCount\":8",
       "/jitter/series: the series item names cpu 9"},
      {"\"cpu\":3,\"workCount\":11",
       "\"cpu\":9,\"workCount\":11",
       "/benchmarks/0/jitter/series: the series item names cpu 9"},
  };
  for (const auto& plant : plants) {
    std::string text(kSeries);
    const auto at = text.find(plant.find);
    check(at != std::string::npos,
          "the document carries the series item the row plants (FR-020)");
    text.replace(at, std::strlen(plant.find), plant.replace);
    sg::test::writeText("harness_json_load_seriescpu.json", text);
    const auto refused = sg::loadReport("harness_json_load_seriescpu.json");
    if (refused.has_value()) {
      std::printf("  the series item naming an unplanted cpu loaded\n");
      sg::test::fail("the fold accepts a series item the stats lack (FR-020)");
      continue;
    }
    const std::string names = std::string(plant.where) + kLacks;
    const bool named = refused.error().find(names) != std::string::npos;
    check(named,
          "theSeriesCpuOutsideTheStatsRefuses: the refusal names the cpu and "
          "the array that lacks it (FR-020, FR-029)");
    if (!named) {
      std::printf("  '%s'\n", refused.error().c_str());
    }
  }
}

// FR-020, E-08: the entry's series reads into the fold the comparison
// reads, the period list standing out of the per-entry copy.
auto thePerEntrySeriesReadsIntoTheFolds() -> void
{
  const auto loaded = sg::loadReport("harness_json_load_series.json");
  if (!loaded.has_value() || loaded->results.empty()) {
    sg::test::fail("the series document stands unread (FR-020)");
    return;
  }
  const auto& folds = loaded->results[0].jitterFolds;
  check(folds.size() == 1 && folds.at(0).cpu == 3
            && folds.at(0).series.has_value(),
        "thePerEntrySeriesReadsIntoTheFolds: the entry's fold reads back its "
        "series (FR-020, E-08)");
  if (folds.empty() || !folds.at(0).series.has_value()) {
    return;
  }
  const auto& series = folds.at(0).series.value();
  check(series.workCount == 11 && series.unitTimeNs == 12
            && series.targetNs == 13 && series.quantumNs == 14
            && series.calibrationSpanNs == 15 && series.sampleCount == 16
            && series.minValue == 17 && series.medianValue == 18
            && series.p90Value == 20 && series.p99Value == 21
            && series.p999Value == 22 && series.p9999Value == 23
            && series.maxValue == 24 && series.seriesRuntimeNs == 26
            && series.droppedSamples == 27 && series.resampleStepNs == 28
            && series.spectrumPoints == 29
            && sg::test::sameValue(series.meanValue, 19.5)
            && sg::test::sameValue(series.noisePercent, 2.5),
        "thePerEntrySeriesReadsIntoTheFolds: the entry's series reads back "
        "every key (FR-020)");
  check(series.periods.empty(),
        "thePerEntrySeriesReadsIntoTheFolds: the per-entry copy carries no "
        "period list (FR-020, E-24)");
  check(folds.at(0).stats.gapCount == 4 && folds.at(0).stats.maxNs == 370,
        "thePerEntrySeriesReadsIntoTheFolds: the standing gap stats read "
        "beside the series (FR-027, E-08)");
}

auto main() -> int
{
  roundTripClosesTheFormat();
  versionRuleGatesTheReader();
  everyFieldOfTheDocumentLoads();
  theHuntRankFieldsLoadBackEqual();
  const std::string written = pinOverheadFloor(sg::test::readText(kWritten));
  theHeaderFieldsAndTheStoredWarningsLoad(written);
  eachExtensionErrorCodeRefuses(written);
  anUnknownKeyInAnOpenObjectLoads(written);
  plantedFieldsOfEveryRecordRefuse();
  firstFailingPartStopsTheFold();
  aDocumentPredatingThePassFieldsLoads();
  theRootListAndTheWarningKeyAnswerTheirOwnRules(written);
  brokenDocumentsRefuseWithPointers(written);
  brokenSpansRefuseWithPointers(written);
  theContextSpansRefuseWithPointers();
  thePrecheckRecordRoundTrips();
  thePrecheckRefusalsNameTheirFields();
  theGraphRefusalsNameTheirFields();
  theSeriesRecordReadsBackEveryKey();
  thePeriodListReadsBack();
  aMissingMethodKeyLeavesTheRunRecordEmpty();
  aMissingSeriesKeyLeavesTheFoldsEmpty();
  eachNewKeyOfTheWrongKindStandsRefused();
  aClosedSeriesItemRefusesAnUnknownKey();
  aClosedPeriodItemRefusesAnUnknownKey();
  thePerEntrySeriesReadsIntoTheFolds();
  theSeriesKeyRefusalsNameTheirArms();
  theSeriesCpuOutsideTheStatsRefuses();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
