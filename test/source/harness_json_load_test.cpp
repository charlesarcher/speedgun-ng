// ============================================================================
// T022, T023, T026: the format closes on itself (specs/023-json-reports;
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
#include <string>
#include <string_view>

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

  const auto withVersion = [&text](const std::string& value)
  {
    std::string mutated = text;
    mutated.replace(mutated.find("\"schemaVersion\":") + 16, 5, value);
    return mutated;
  };

  sg::test::writeText("harness_json_load_minor.json", withVersion("\"1.7\""));
  check(sg::loadReport("harness_json_load_minor.json").has_value(),
        "a later minor of the same major loads (FR-010)");

  sg::test::writeText("harness_json_load_major.json", withVersion("\"2.0\""));
  const auto refused = sg::loadReport("harness_json_load_major.json");
  check(
      !refused.has_value() && refused.error().find("2.0") != std::string::npos,
      "a later major is refused, naming the version it carries (FR-023)");

  sg::test::writeText("harness_json_load_current.json", withVersion("\"1.1\""));
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
 "benchmarks":[{"name":"bmRich","suite":"bmRich","case":"bmRich",
  "outcome":"SKIPPED","reason":"the scripted skip","hideRepetitionRows":true,
  "timeUnit":"MILLISECONDS","primaryIsCpu":false,"pauseSamplesFolded":true,
  "percentilePoints":[50.0],
  "contexts":[{"waitMode":"SPIN","placement":"l3 group 0","applied":true},
              {"waitMode":"BLOCK","placement":"unpinned","applied":false}],
  "noiseWarning":{"spinning":3,"runnableCpus":16,"sharesOneCpu":true},
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

// FR-024, SC-007: one planted disagreement per shape of the reader, each
// refused with the JSON pointer of the field it stands in.
struct Mutation
{
  const char* find;
  const char* replace;
  const char* pointer;
};

// One planted field per position of every conversion chain the reader
// runs, planted on the document that carries every optional field: each
// position answers a refusal naming the pointer it stands at (FR-024,
// FR-025, SC-007). The written document holds no statistics record, no
// fit, no context and no statistic value, so these plants stand on the
// hand-built one.
constexpr Mutation kRichMutations[] = {
    // The seven fields of a statistics record (E-07, FR-025).
    {"\"samples\":2",
     "\"samples\":\"x\"",
     "/benchmarks/0/rows/0/timeStats/samples"},
    {"\"mean\":1.0", "\"mean\":\"x\"", "/benchmarks/0/rows/0/timeStats/mean"},
    {"\"median\":1.5",
     "\"median\":\"x\"",
     "/benchmarks/0/rows/0/timeStats/median"},
    {"\"sampleStdDev\":0.5",
     "\"sampleStdDev\":\"x\"",
     "/benchmarks/0/rows/0/timeStats/sampleStdDev"},
    {"\"coefficientOfVariation\":0.25",
     "\"coefficientOfVariation\":\"x\"",
     "/benchmarks/0/rows/0/timeStats/coefficientOfVariation"},
    {"\"min\":0.5", "\"min\":\"x\"", "/benchmarks/0/rows/0/timeStats/min"},
    {"\"max\":1.5", "\"max\":\"x\"", "/benchmarks/0/rows/0/timeStats/max"},
    // The five fields of a fit record and its reason (E-08, FR-012).
    {"\"quantity\":\"real time\"",
     "\"quantity\":5",
     "/benchmarks/0/rows/0/fit/quantity"},
    {"\"curve\":\"N\"", "\"curve\":5", "/benchmarks/0/rows/0/fit/curve"},
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
    // The three fields of a context disclosure, wrong of kind and then
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
    // The three fields of a statistic value, wrong of kind and missing
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
    // The three fields of the noise warning, each taken away (E-10).
    {"{\"spinning\":3,", "{", "/benchmarks/0/noiseWarning"},
    {"\"runnableCpus\":16,", "", "/benchmarks/0/noiseWarning"},
    {",\"sharesOneCpu\":true", "", "/benchmarks/0/noiseWarning"},
    // The chunk columns of the context span, one metric value, and one
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
    // The pass disclosure of specs/024 FR-038: each key of the wrong kind,
    // the method word outside the closed list, the count below zero, and a
    // warning that disagrees with the spread beside it. A key taken away is
    // the one shape that loads, and the case after this one covers it
    // (FR-024, FR-038).
    {"\"metricMethod\":\"PASSES\"",
     "\"metricMethod\":1",
     "/benchmarks/0/metricMethod"},
    {"\"metricMethod\":\"PASSES\"",
     "\"metricMethod\":\"PASSESSED\"",
     "/benchmarks/0/metricMethod"},
    {"\"passCount\":2", "\"passCount\":\"two\"", "/benchmarks/0/passCount"},
    {"\"passCount\":2", "\"passCount\":-1", "/benchmarks/0/passCount"},
    {"\"passSpread\":3.5",
     "\"passSpread\":\"wide\"",
     "/benchmarks/0/passSpread"},
    {"\"spreadWarning\":false",
     "\"spreadWarning\":\"yes\"",
     "/benchmarks/0/spreadWarning"},
    {"\"spreadWarning\":false",
     "\"spreadWarning\":true",
     "/benchmarks/0/spreadWarning"},
    // The pass index of one metric column at a value of the wrong kind
    // (specs/024 E-08, FR-019); the key taken away loads, as the case below
    // shows.
    {"\"pass\":1", "\"pass\":\"one\"", "/benchmarks/0/rows/0/metrics/0/pass"},
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
    {"\"schemaVersion\":\"1.1\"", "\"schemaVersion\":1", "/schemaVersion"},
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
  other.replace(other.find("\"schemaVersion\":") + 16, 5, "\"1.9\"");
  sg::test::writeText("harness_json_load_headertext.json", other);
  const auto minor = sg::loadReport("harness_json_load_headertext.json");
  check(minor.has_value() && minor->schemaVersion == "1.9",
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

auto main() -> int
{
  roundTripClosesTheFormat();
  versionRuleGatesTheReader();
  everyFieldOfTheDocumentLoads();
  const std::string written = pinOverheadFloor(sg::test::readText(kWritten));
  theHeaderFieldsAndTheStoredWarningsLoad(written);
  eachExtensionErrorCodeRefuses(written);
  anUnknownKeyInAnOpenObjectLoads(written);
  plantedFieldsOfEveryRecordRefuse();
  aDocumentPredatingThePassFieldsLoads();
  theRootListAndTheWarningKeyAnswerTheirOwnRules(written);
  brokenDocumentsRefuseWithPointers(written);
  brokenSpansRefuseWithPointers(written);
  theContextSpansRefuseWithPointers();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
