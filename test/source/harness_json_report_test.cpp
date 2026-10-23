// ============================================================================
// T016, T021, T036, T037: the written report answers the run
// (specs/023-json-reports;
// FR-001, FR-003, FR-005, FR-009, FR-011, FR-021, FR-029, FR-031,
// FR-041, FR-043, FR-086, SC-001, SC-003, SC-009, SC-011).
// specs/028-full-command-line adds the run object of the same document
// (FR-039, FR-040, FR-041, FR-043, FR-084, FR-086, SC-009).
//
// One scripted run of the shared window feeds the writer; the reader reads
// the file back and every field of the loaded value answers the run that
// wrote it. The chunk columns of the run stand in the document beside the
// rows they answer, in the order the run closed them. The console keeps
// its bytes: the same result printed beside the written document still
// prints its chunk line and its percentile line. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/json_fixture.hpp"

using sg::test::check;
using sg::test::fail;

namespace
{

auto capturePrint(const sg::BenchmarkResult& result) -> std::string
{
  const char* const path = "harness_json_report_print.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  sg::detail::printResult(result);
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto columnAnswers(const std::vector<double>& column,
                   const std::array<std::uint64_t, 4>& want) -> bool
{
  if (column.size() != want.size()) {
    return false;
  }
  for (std::size_t chunk = 0; chunk < column.size(); ++chunk) {
    if (!sg::test::sameValue(column[chunk], static_cast<double>(want[chunk]))) {
      return false;
    }
  }
  return true;
}

// FR-001, FR-003, SC-001: the document holds the run, field by field.
void writtenDocumentAnswersTheRun(const sg::test::ScriptedRun& run,
                                  const sg::ReportData& data);

auto runAndWrite(const char* path) -> void
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  options.showPercentiles = true;
  options.rawContent = sg::RawContent::CHUNKS;
  const auto run = sg::test::scriptedRun(options);

  check(run.result.rows.size() == 1, "the run produced one measured row");
  check(run.rowRaw.size() == 1 && run.rowRaw[0].size() == 1,
        "the run retained one raw entry for that row");

  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;

  check(sg::detail::writeReport(path, data),
        "the writer wrote the document (FR-001)");

  if (path == std::string("harness_json_report.json")) {
    writtenDocumentAnswersTheRun(run, data);
  }
}

void writtenDocumentAnswersTheRun(const sg::test::ScriptedRun& run,
                                  const sg::ReportData& data)
{
  const auto loaded = sg::loadReport("harness_json_report.json");
  check(loaded.has_value(), "the reader read the written document (FR-022)");
  if (loaded.has_value()) {
    check(sg::test::sameReport(*loaded, data),
          "every field of the loaded document answers the run (FR-003, "
          "SC-001)");
  }

  // FR-029: the chunk columns travel in the order the run closed them,
  // not the sorted order the percentile line reads.
  const auto& columns = data.rowRaw[0][0].columns;
  bool timeSeen = false;
  bool cpuSeen = false;
  for (const auto& column : columns) {
    timeSeen = timeSeen || columnAnswers(column, sg::test::kTimeDeltas);
    cpuSeen = cpuSeen || columnAnswers(column, sg::test::kCpuDeltas);
  }
  check(timeSeen, "the time chunk column answers the scripted deltas (FR-029)");
  check(cpuSeen, "the cpu chunk column answers the scripted deltas (FR-029)");
  check(!columns.empty(), "the measured row carries its chunk columns (E-05)");

  // FR-003, FR-031, SC-003: the console keeps its bytes beside the file.
  const std::string kPrinted = capturePrint(run.result);
  check(kPrinted.find("chunks=") != std::string::npos
            && kPrinted.find("p50=") != std::string::npos,
        "the console still prints the chunk line and the percentile line "
        "beside the written document (FR-031)");
}

// FR-009, FR-011, SC-009: two runs of one suite answer the same bytes.
// The scripted point column of a leaf advances across the runs of one
// process, so the second run stands in a child forked before the parent
// takes its own: both start from the same scripted state.

// The one number of the document a scripted suite cannot fix: the
// sampling overhead floor the plan measures from the machine at the
// start of the attempt (`source/harness/runner.cpp:1228`). Every other
// byte answers the scripted values. The comparison holds out the digits
// of that key alone, so a loaded machine cannot break the identity of
// the rest (specs/023 SC-009, D-13).
auto holdOutOverheadFloor(std::string text) -> std::string
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
    text.replace(start, end - start, "0.0");
    at = text.find(kKey, start + 3);
  }
  return text;
}

auto twoRunsWriteIdenticalBytes() -> void
{
  const pid_t child = fork();
  if (child < 0) {
    fail("cannot start the second run");
    return;
  }
  if (child == 0) {
    runAndWrite("harness_json_b.json");
    ::_exit(sg::test::failures == 0 ? 0 : 1);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)
      || WEXITSTATUS(status) != 0)
  {
    fail("the second run did not exit zero");
    return;
  }
  runAndWrite("harness_json_a.json");
  check(holdOutOverheadFloor(sg::test::readText("harness_json_a.json"))
            == holdOutOverheadFloor(sg::test::readText("harness_json_b.json")),
        "two runs of one scripted suite write byte-identical files apart "
        "from the measured overhead floor (FR-009, FR-011, SC-009)");
}

// Every optional field of the format in one hand-built value. The
// scripted suite never produces a skipped entry, a fit row that stands
// without its pair, a noise warning, a custom statistic or per-context
// raw columns, and each of those shapes takes its own path through the
// writer and the reader (specs/023 E-04 to E-11, FR-013 to FR-019,
// FR-029).
auto everyFieldOfTheFormatRoundTrips() -> void
{
  sg::ReportData data;

  sg::BenchmarkResult measured;
  measured.name = "family/case";
  measured.suite = "family";
  measured.caseName = "case";
  measured.outcome = sg::RunOutcome::MEASURED;
  measured.metricLabels = {"bytes", "count"};
  measured.counterLabels = {"rate"};
  measured.counterStats = {{2, 1.5, 1.5, 0.7, 0.5, 1.0, 2.0},
                           {2, 3.0, 3.0, 0.0, 0.0, 3.0, 3.0}};
  measured.statisticValues = {{"p95", sg::StatUnit::TIME, 12.5},
                              {"share", sg::StatUnit::PERCENTAGE, 40.0}};
  measured.hideRepetitionRows = true;
  measured.timeUnit = sg::TimeUnit::MICROSECONDS;
  measured.pauseSamplesFolded = true;
  measured.percentilePoints = {50.0, 99.0};
  measured.contexts = {{sg::WaitMode::SPIN, "3", true},
                       {sg::WaitMode::SPIN, "3", false}};
  measured.primaryIsCpu = false;
  measured.contextPercentiles = {
      {{{true, 1.0}, {false, 0.0}}, {{true, 2.0}, {true, 3.0}}},
      {{{false, 0.0}, {false, 0.0}}, {{true, 4.0}, {true, 5.0}}}};

  sg::ResultRow fitted;
  fitted.kind = sg::RowKind::REPETITION;
  fitted.iterations = 6;
  fitted.timePerIterationNs = 250.0;
  fitted.cpuTimePerIterationNs = 200.0;
  fitted.endpointSamples = 4;
  fitted.pauseSamplesFolded = true;
  fitted.timeAvailability = sg::counters::Availability::GAP;
  fitted.metrics = {
      {1.0, 2.0, true, sg::counters::Availability::COUNTABLE, std::nullopt},
      {0.0, 0.0, false, sg::counters::Availability::ABSENT, std::nullopt}};
  fitted.counters = {4.0};
  fitted.counterDisclosure = "zero-divisor";
  fitted.label = "labeled";
  fitted.fit = {"time", "N", 1.25, 3.5, true, ""};
  fitted.overheadFloorNs = 12.5;
  fitted.percentiles = {{{true, 1.0}, {false, 0.0}}};
  fitted.chunkCount = 2;
  fitted.chunkIterations = 3;

  sg::ResultRow aggregate;
  aggregate.kind = sg::RowKind::AGGREGATE;
  aggregate.iterations = 12;
  aggregate.timePerIterationNs = 260.0;
  aggregate.cpuTimePerIterationNs = 210.0;
  aggregate.endpointSamples = 8;
  aggregate.timeAvailability = sg::counters::Availability::COUNTABLE;
  aggregate.fit = {"bytes", "LOG_N", 0.0, 0.0, false, "no pair stands"};
  aggregate.percentiles = {{{true, 7.0}, {true, 9.0}}};
  aggregate.chunkCount = 4;
  aggregate.chunkIterations = 3;
  aggregate.timeStats = {2, 255.0, 255.0, 7.0, 0.03, 250.0, 260.0};
  // The empty record of a metric column that sampled nothing: the writer
  // writes null there and the reader answers the empty record back
  // (specs/023 E-07, FR-025).
  aggregate.metricStats = {{2, 1.5, 1.5, 0.7, 0.5, 1.0, 2.0}, {}};
  measured.rows = {fitted, aggregate};

  sg::BenchmarkResult skipped;
  skipped.name = "family/other";
  skipped.suite = "family";
  skipped.caseName = "other";
  skipped.outcome = sg::RunOutcome::SKIPPED;
  skipped.reason = "the leaf stands uncountable";

  data.results = {measured, skipped};

  sg::RowRaw fittedColumns;
  fittedColumns.columns = {{1.0, 2.0}, {3.0, 4.0}};
  data.rowRaw.push_back({fittedColumns, sg::RowRaw {}});
  data.rowRaw.push_back({});

  sg::RowRaw firstContext;
  firstContext.columns = {{5.0, 6.0}};
  data.contextRaw.push_back({{firstContext, sg::RowRaw {}}, {}});
  data.contextRaw.push_back({});

  check(sg::detail::writeReport("harness_json_rich.json", data),
        "the writer wrote a document holding every field of the format");
  const std::string text = sg::test::readText("harness_json_rich.json");
  const auto schema = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (schema.has_value()) {
    check(true, "the document of every field answers the schema (FR-008)");
  } else {
    std::printf("  the rich document disagrees: %s\n", schema.error().c_str());
    fail("the document of every field disagrees with the schema (FR-008)");
  }

  const auto loaded = sg::loadReport("harness_json_rich.json");
  if (!loaded.has_value()) {
    std::printf("  the rich document refused: %s\n", loaded.error().c_str());
    fail("the document of every field refuses the reader (FR-022)");
    return;
  }
  check(loaded->results.size() == 2 && loaded->results[0].contexts.size() == 2
            && loaded->results[0].statisticValues.size() == 2
            && loaded->results[0].counterStats.size() == 2
            && loaded->results[0].rows.size() == 2
            && loaded->results[0].rows[1].timeStats.samples == 2
            && loaded->results[0].rows[1].metricStats.size() == 2
            && loaded->results[0].rows[1].metricStats[1].samples == 0
            && loaded->results[0].rows[0].fit.reason.empty()
            && loaded->results[0].rows[1].fit.reason == "no pair stands"
            && loaded->results[1].outcome == sg::RunOutcome::SKIPPED
            && loaded->results[1].reason == "the leaf stands uncountable",
        "the reader answers every field of the hand-built document (FR-022)");
  check(loaded->rowRaw.size() == 2 && loaded->rowRaw[0].size() == 2
            && loaded->rowRaw[0][0].columns.size() == 2
            && sg::test::sameValue(loaded->rowRaw[0][0].columns[1][1], 4.0)
            && loaded->rowRaw[0][1].columns.empty()
            && loaded->contextRaw[0][0].size() == 2
            && loaded->contextRaw[0][1].empty(),
        "the raw columns of each row and each context answer the document "
        "(FR-029, E-01)");
}

// The noise warning of two spinning contexts pinned to one cpu index
// stands in the document as the E-10 object, and the reader takes its
// non-null path (specs/023 E-10, FR-016).
auto theNoiseWarningRoundTrips() -> void
{
  const std::string text = sg::test::readText("harness_json_rich.json");
  check(text.find("\"noiseWarning\":{\"") != std::string::npos,
        "two spinning contexts on one cpu write the warning object (E-10)");
  const auto loaded = sg::loadReport("harness_json_rich.json");
  check(loaded.has_value() && loaded->results[0].contexts.size() == 2
            && loaded->results[0].contexts[0].waitMode == sg::WaitMode::SPIN
            && loaded->results[0].contexts[1].waitMode == sg::WaitMode::SPIN
            && loaded->results[0].contexts[0].placement
                == loaded->results[0].contexts[1].placement,
        "the reader answers the two contexts the warning answers (E-10)");
}

// One entry of the pass shapes: a single row with a single metric column,
// carrying the pass disclosure the run settled to (specs/024 FR-019).
auto passShape(const char* name,
               sg::MetricMethod method,
               std::size_t passCount,
               std::optional<double> spread,
               std::optional<std::size_t> pass) -> sg::BenchmarkResult
{
  sg::BenchmarkResult entry;
  entry.name = name;
  entry.suite = name;
  entry.caseName = name;
  entry.metricLabels = {"work"};
  entry.metricMethod = method;
  entry.passCount = passCount;
  entry.passSpread = spread;

  sg::ResultRow row;
  row.kind = sg::RowKind::REPETITION;
  row.iterations = 1;
  row.metrics = {{2.5, 1.0, true, sg::counters::Availability::COUNTABLE, pass}};
  entry.rows = {row};
  return entry;
}

// C-6, FR-038: the pass disclosure of the run stands in the document. A
// one-pass run keeps the keys the format carried and adds the four new ones
// at the end of its entry, with a null spread, no warning and a null pass
// on each metric column (E-08). Two passes write the count and the spread
// between them beside the index of the pass that measured each column. A
// spread above the limit carries the warning the console prints. The reader
// answers each of them back.
auto thePassDisclosureRoundTrips() -> void
{
  sg::ReportData data;
  data.results.push_back(passShape(
      "bmOne", sg::MetricMethod::AUTO, 0, std::nullopt, std::nullopt));
  data.results.push_back(
      passShape("bmTwo", sg::MetricMethod::PASSES, 2, 1.5, std::size_t {1}));
  data.results.push_back(
      passShape("bmWarn", sg::MetricMethod::PASSES, 3, 7.5, std::size_t {2}));
  data.results.push_back(passShape(
      "bmMix", sg::MetricMethod::MULTIPLEX, 1, std::nullopt, std::nullopt));
  for (std::size_t entry = 0; entry < data.results.size(); ++entry) {
    data.rowRaw.push_back({sg::RowRaw {}});
    data.contextRaw.push_back({{sg::RowRaw {}}});
  }

  check(sg::detail::writeReport("harness_json_pass.json", data),
        "the writer wrote the four pass shapes (FR-038)");
  const std::string text = sg::test::readText("harness_json_pass.json");
  const auto schema = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (schema.has_value()) {
    check(true, "the four pass shapes answer the schema (FR-008)");
  } else {
    std::printf("  the pass document disagrees: %s\n", schema.error().c_str());
    fail("the pass document disagrees with the schema (FR-008)");
  }

  check(text.find("\"metricMethod\":\"AUTO\",\"passCount\":0,"
                  "\"passSpread\":null,\"spreadWarning\":false")
            != std::string::npos,
        "a one-pass run writes the auto method, no count, a null spread and "
        "no warning (FR-016, FR-038)");
  check(text.find("\"spreadWarning\":false}") != std::string::npos,
        "the four keys stand at the end of the entry, after the keys the "
        "format already carried (FR-038)");
  check(text.find("\"availability\":\"COUNTABLE\",\"pass\":null}")
            != std::string::npos,
        "a metric column of a one-pass run writes a null pass (E-08, FR-019)");
  check(text.find("\"passCount\":2,\"passSpread\":1.5,\"spreadWarning\":false")
            != std::string::npos,
        "two passes write their count and their spread with no warning "
        "(FR-017, FR-038)");
  check(text.find("\"availability\":\"COUNTABLE\",\"pass\":1}")
            != std::string::npos,
        "a metric column of a two-pass run names the pass that measured it "
        "(FR-019)");
  check(text.find("\"passCount\":3,\"passSpread\":7.5,\"spreadWarning\":true")
            != std::string::npos,
        "a spread above the limit carries the warning the console prints "
        "(FR-018, FR-038)");
  check(text.find("\"metricMethod\":\"MULTIPLEX\",\"passCount\":1,"
                  "\"passSpread\":null")
            != std::string::npos,
        "a multiplex run writes its method with a null spread (FR-016, "
        "FR-038)");

  const auto loaded = sg::loadReport("harness_json_pass.json");
  if (!loaded.has_value()) {
    std::printf("  the pass document refused: %s\n", loaded.error().c_str());
    fail("the reader refused the pass document (FR-022)");
    return;
  }
  check(
      sg::test::sameReport(*loaded, data),
      "the reader answers every pass field of the document (FR-022, " "FR-"
                                                                      "038)");
  check(loaded->results[1].passCount == 2
            && loaded->results[1].passSpread.has_value()
            && sg::test::sameValue(*loaded->results[1].passSpread, 1.5)
            && loaded->results[2].rows[0].metrics[0].pass == 2
            && loaded->results[3].metricMethod == sg::MetricMethod::MULTIPLEX,
        "the loaded count, spread, pass index and method answer the run "
        "(FR-017, FR-019, FR-038)");
}

// FR-030: a file system that refuses the write answers false, and the
// refusal reaches stderr; no exception escapes. The full device opens
// and refuses every byte, so the write itself is what fails.
auto aRefusedWriteAnswersFalse() -> void
{
  sg::ReportData data;
  data.results.emplace_back();
  data.rowRaw.push_back({});
  data.contextRaw.push_back({});
  check(!sg::detail::writeReport("/dev/full", data),
        "a file system that refuses the bytes answers false (FR-030)");
}

// A row with no retained chunk-value entry reads the two empty column
// sets rather than running past the vector, and writes no `raw` key
// (specs/023 E-05, D-02).
auto aRowWithoutARawEntryWritesNoColumns() -> void
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  options.rawContent = sg::RawContent::CHUNKS;
  const auto run = sg::test::scriptedRun(options);
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw.push_back({});
  data.contextRaw.push_back({});
  check(sg::detail::writeReport("harness_json_noraw.json", data),
        "the writer wrote the document of a row with no retained entry");
  const auto loaded = sg::loadReport("harness_json_noraw.json");
  bool bare = loaded.has_value() && loaded->results.size() == 1
      && !loaded->results[0].rows.empty() && loaded->rowRaw.size() == 1
      && loaded->rowRaw[0].size() == loaded->results[0].rows.size()
      && loaded->contextRaw.size() == 1;
  for (const auto& row : loaded->rowRaw[0]) {
    bare = bare && row.columns.empty();
  }
  for (const auto& row : loaded->contextRaw[0]) {
    bare = bare && row.empty();
  }
  check(bare, "a row with no retained entry writes no raw key (E-05)");
}

// The guard of `enumWord`: a value outside the closed enumeration writes
// the one word the guard carries, so no write runs off the word table
// (specs/023 FR-015).
auto aValueOutsideTheTableWritesTheGuardWord() -> void
{
  sg::ReportData data;
  data.results.emplace_back();
  data.rowRaw.push_back({});
  data.contextRaw.push_back({});
  data.results.back().outcome = static_cast<sg::RunOutcome>(99);
  check(sg::detail::writeReport("harness_json_unknown.json", data),
        "the writer wrote a document for an out-of-range outcome");
  const std::string text = sg::test::readText("harness_json_unknown.json");
  check(text.find("\"UNKNOWN\"") != std::string::npos,
        "a value outside the enumeration writes the guard word (FR-015)");
}

// FR-086, FR-043: the document stamps the schema minor this feature takes.
// The writer holds one constant for the stamp (`source/harness/json_report.cpp`
// `kSchemaVersion`), so the value the document carries is that constant read
// back, and the schema file names the same minor.
auto theDocumentStampsTheMinorThisFeatureRequires() -> void
{
  const std::string text = sg::test::readText("harness_json_report.json");
  check(text.starts_with("{\"schemaVersion\":\"1.3\""),
        "the written document stamps schema minor 1.3, the next free minor "
        "this feature takes (FR-086, FR-043)");
}

// FR-041, FR-043: the run that names no option file and no environment
// source writes no `run` key, so the key stays the optional top-level key
// the schema describes. A writer that emitted the object for every run
// would answer `"run":` here.
auto aRunOfNoFileOrEnvironmentWritesNoRunKey() -> void
{
  const std::string text = sg::test::readText("harness_json_report.json");
  check(text.find("\"run\":") == std::string::npos,
        "a document written with no file or environment source carries no "
        "top-level run key (FR-041, FR-043)");
  // The planted document that does carry the object keeps the needle live, so
  // the guard above cannot pass by searching for a shape no writer emits.
  sg::test::writeText("harness_json_runkey.json",
                      "{\"schemaVersion\":\"1.3\",\"speedgun\":{},"
                      "\"run\":{\"interleave\":7},\"benchmarks\":[]}");
  const std::string planted = sg::test::readText("harness_json_runkey.json");
  check(planted.find("\"run\":") != std::string::npos,
        "the guard reads a run key when one stands in the document (FR-041)");
}

// FR-005, FR-021, D-13: nothing the file carries changes between two runs
// of the same values. No key names a date, a time, a host or a process,
// and the header holds the two compile-definition values alone.
auto headerCarriesNoVolatileKey() -> void
{
  const std::string text = sg::test::readText("harness_json_report.json");
  for (const char* key : {"\"date\"",
                          "\"time\"",
                          "\"timestamp\"",
                          "\"host\"",
                          "\"hostname\"",
                          "\"pid\"",
                          "\"clock\"",
                          "\"epoch\"",
                          "\"utc\"",
                          "\"generatedAt\""})
  {
    check(text.find(key) == std::string::npos,
          "no key of the document names a date, a time, a host or a process "
          "(FR-005, FR-021)");
  }
  check(text.find("\"speedgun\":{\"version\":") != std::string::npos
            && text.find("\",\"buildType\":") != std::string::npos
            && text.find("},\"benchmarks\"") != std::string::npos,
        "the header carries the version and the build type and nothing else "
        "(FR-005)");
}

// The option row a disclosure entry points at, read from the one table so
// the test carries no option name of its own (specs/028 E-05).
auto rowNamed(const std::string_view name) -> const sg::detail::OptionRow*
{
  for (const sg::detail::OptionRow& row : sg::detail::optionRows()) {
    if (name == row.name) {
      // optionRows() is static constexpr storage, so this address is stable.
      // cppcheck-suppress returnDanglingLifetime
      return &row;
    }
  }
  fail("the option table holds no row of the name the case asks for");
  return nullptr;
}

// One scripted run written out beside the disclosure the case hands over,
// read back as the bytes of the file (specs/028 FR-040).
auto writeWithDisclosure(const char* path,
                         const sg::detail::RunDisclosure& disclosure)
    -> std::string
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  const auto run = sg::test::scriptedRun(options);
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  check(sg::detail::writeReport(path, data, disclosure),
        "the writer wrote the document beside the disclosure (FR-040)");
  return sg::test::readText(path);
}

// FR-039, FR-040, FR-043, FR-084, SC-009: the seed and one entry per use
// stand in the `run` object, and the object sits between `speedgun` and
// `benchmarks` the way the schema states the order. The written document
// passes the checker, and the reader answers no fault for a key it reads
// as nothing.
auto theRunObjectHoldsTheDisclosedUses() -> void
{
  const std::vector<sg::detail::OptionUse> uses {
      sg::detail::OptionUse {
          rowNamed("filter"), sg::detail::OptionSource::ENVIRONMENT, "bm"},
      sg::detail::OptionUse {
          rowNamed("min-time"), sg::detail::OptionSource::OPTION_FILE, "20ms"},
  };
  const char* const path = "harness_json_run.json";
  const std::string text = writeWithDisclosure(
      path, sg::detail::RunDisclosure {uses, std::uint64_t {7}});
  check(
      text.find("\"speedgun\":{") < text.find("\"run\":{")
          && text.find("\"run\":{") < text.find(",\"benchmarks\":"),
      "the run object stands between speedgun and benchmarks (FR-040, " "D-"
                                                                        "18)");
  check(text.find("\"run\":{\"interleave\":7,\"options\":["
                  "{\"option\":\"--filter\",\"source\":\"environment\","
                  "\"value\":\"bm\"},"
                  "{\"option\":\"--min-time\",\"source\":\"file\","
                  "\"value\":\"20ms\"}]}")
            != std::string::npos,
        "the object holds the seed and one entry per use in table order "
        "(FR-040, SC-009)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  check(clean.has_value(),
        "a document that carries the run object passes the schema "
        "(FR-043, FR-084)");
  if (!clean.has_value()) {
    std::printf("  %s\n", clean.error().c_str());
  }
  const auto loaded = sg::loadReport(path);
  check(loaded.has_value() && loaded->results.size() == 1,
        "the reader answers no fault for the object it reads as nothing "
        "(FR-043, D-30)");
}

// FR-040: the seed key follows the option. The bare form answers seed 0 and
// still writes the key; a run that gives no interleave writes none.
auto theSeedKeyFollowsTheOption() -> void
{
  const std::string seeded =
      writeWithDisclosure("harness_json_run_seed.json",
                          sg::detail::RunDisclosure {{}, std::uint64_t {0}});
  check(seeded.find("\"run\":{\"interleave\":0,\"options\":[]}")
            != std::string::npos,
        "the bare interleave form writes seed 0 and an empty use list "
        "(FR-040, E-10)");

  const std::vector<sg::detail::OptionUse> uses {
      sg::detail::OptionUse {
          rowNamed("iterations"), sg::detail::OptionSource::ENVIRONMENT, "4"},
  };
  const std::string unseeded =
      writeWithDisclosure("harness_json_run_unseeded.json",
                          sg::detail::RunDisclosure {uses, std::nullopt});
  check(unseeded.find("\"run\":{\"options\":[{\"option\":\"--iterations\"")
                != std::string::npos
            && unseeded.find("\"interleave\"") == std::string::npos,
        "a run that gives no interleave writes no seed key (FR-040)");
}

// FR-041, FR-043: an empty disclosure writes no object at all, and the
// document with the key absent keeps passing the schema the way the base
// document does.
auto anEmptyDisclosureWritesNoRunObject() -> void
{
  const std::string text = writeWithDisclosure("harness_json_run_none.json",
                                               sg::detail::RunDisclosure {});
  check(text.find("\"run\":") == std::string::npos,
        "an empty disclosure writes no run key (FR-041)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  check(clean.has_value(),
        "the document with the key absent passes the schema (FR-043)");
  if (!clean.has_value()) {
    std::printf("  %s\n", clean.error().c_str());
  }
}

}  // namespace

auto main() -> int
{
  runAndWrite("harness_json_report.json");
  twoRunsWriteIdenticalBytes();
  headerCarriesNoVolatileKey();
  theDocumentStampsTheMinorThisFeatureRequires();
  aRunOfNoFileOrEnvironmentWritesNoRunKey();
  theRunObjectHoldsTheDisclosedUses();
  theSeedKeyFollowsTheOption();
  anEmptyDisclosureWritesNoRunObject();
  aValueOutsideTheTableWritesTheGuardWord();
  everyFieldOfTheFormatRoundTrips();
  theNoiseWarningRoundTrips();
  thePassDisclosureRoundTrips();
  aRefusedWriteAnswersFalse();
  aRowWithoutARawEntryWritesNoColumns();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
