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
// start of the attempt (`source/harness/runner.cpp:1398-1400`). Every other
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

// T-1, FR-061, FR-074: the report path a run cannot open names itself in
// the one line that fault answers, and a byte a terminal acts on inside
// that path reaches the line as text.
auto aRefusedOpenQuotesTheReportPath() -> void
{
  sg::ReportData data;
  data.results.emplace_back();
  data.rowRaw.push_back({});
  data.contextRaw.push_back({});

  const char* const path = "harness_json_report_stderr.txt";
  const int saved = dup(STDERR_FILENO);
  if (saved < 0 || std::freopen(path, "w", stderr) == nullptr) {
    fail("cannot redirect the fault stream");
  }
  const bool written =
      sg::detail::writeReport("/no/such\ndir/report.json", data);
  std::fflush(stderr);
  if (dup2(saved, STDERR_FILENO) < 0) {
    fail("cannot restore the fault stream");
  }
  close(saved);

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string text = buffer.str();
  std::size_t breaks = 0;
  for (const char letter : text) {
    breaks += static_cast<std::size_t>(letter == '\n');
  }
  check(!written,
        "a report path the file system refuses answers false (FR-074)");
  check(breaks == 1, "that fault is one line (FR-061, SC-004)");
  check(text.find("such\\ndir") != std::string::npos,
        "the report path reaches the line as text (FR-024)");
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
  check(text.starts_with("{\"schemaVersion\":\"1.16\""),
        "the written document stamps schema minor 1.16, the minor "
        "specs/048-hwlat-probe takes past the 1.15 of "
        "specs/041-counter-probe (FR-086, FR-043, specs/039 FR-032, specs/037 "
        "FR-005, specs/043 FR-006, specs/042-topdown FR-036, "
        "specs/046-hunter-topdown FR-020, specs/041-counter-probe FR-051)");
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
                      "{\"schemaVersion\":\"1.4\",\"speedgun\":{},"
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
  const sg::detail::OptionRow* const row = sg::detail::rowNamed(name);
  if (row == nullptr) {
    fail("the option table holds no row of the name the case asks for");
  }
  return row;
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

// FR-015: the shared word-table reader of the writer. An index inside the
// table answers its own word, and an index at or past the end answers the
// guard word the document carries for a value outside the enumeration.
auto theGuardWordStandsPastTheTable() -> void
{
  static constexpr std::array<std::string_view, 3> kWords {"A", "B", "C"};
  check(sg::detail::enumWord(kWords, 0) == "A"
            && sg::detail::enumWord(kWords, 2) == "C",
        "an index inside the table answers its own word (FR-015)");
  check(sg::detail::enumWord(kWords, 3) == "UNKNOWN"
            && sg::detail::enumWord(kWords, 99) == "UNKNOWN",
        "theGuardWordStandsPastTheTable: an index at or past the table's end "
        "answers the guard word (FR-015)");
}

// FR-020, E-23, FR-006: the arm that skips a fold with no series inside a
// run that carries one. The run record and the entry each carry two probe
// cpus, the first with a series and the second with none, so each series
// array writes one item and names the cpu that kept samples.
auto seriesOfCpu(const std::uint64_t base) -> sg::JitterSeriesStats
{
  sg::JitterSeriesStats series;
  series.workCount = base + 1;
  series.unitTimeNs = base + 2;
  series.targetNs = base + 3;
  series.quantumNs = base + 4;
  series.calibrationSpanNs = base + 5;
  series.sampleCount = base + 6;
  series.minValue = base + 7;
  series.medianValue = base + 8;
  series.meanValue = static_cast<double>(base) + 9.5;
  series.p90Value = base + 10;
  series.p99Value = base + 11;
  series.p999Value = base + 12;
  series.p9999Value = base + 13;
  series.maxValue = base + 14;
  series.noisePercent = static_cast<double>(base) + 0.5;
  series.seriesRuntimeNs = base + 16;
  series.droppedSamples = base + 17;
  series.resampleStepNs = base + 18;
  series.spectrumPoints = base + 19;
  return series;
}

auto theFoldWithNoSeriesStandsSkipped() -> void
{
  constexpr const char* const kPath = "harness_json_report_series_skip.json";
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  const auto run = sg::test::scriptedRun(options);

  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  data.results.back().jitterFolds.push_back(
      sg::JitterEntryFold {.cpu = 2, .series = seriesOfCpu(100)});
  data.results.back().jitterFolds.push_back(sg::JitterEntryFold {.cpu = 3});

  sg::detail::JitterReportRecord record;
  record.method = sg::JitterMethod::FWQ;
  record.thresholdNs = 200;
  record.loopPeriodNs = 20;
  record.cpus = {2, 3};
  for (const std::uint32_t cpu : record.cpus) {
    sg::detail::JitterReportCpu line;
    line.cpu = cpu;
    line.stats.gapCount = 1;
    if (cpu == 2) {
      line.series = seriesOfCpu(200);
    }
    record.stats.push_back(line);
  }
  record.disclosure = "the run's disclosure";

  check(sg::detail::writeReport(kPath, data, {}, record),
        "the writer wrote the document of the mixed record (FR-020)");
  const auto document = sg::test::readText(kPath);

  const auto root = document.find("\"jitter\":{");
  const auto rootSeries = document.find("\"series\":[", root);
  const auto rootEnd = document.find(']', rootSeries);
  check(
      rootSeries != std::string::npos,
      "the run record of a method run writes a series array (FR-019, FR-020)");
  check(document.find("\"cpu\":2", rootSeries) < rootEnd
            && document.find("\"cpu\":3", rootSeries) > rootEnd,
        "theFoldWithNoSeriesStandsSkipped: the run array writes the cpu that "
        "kept samples and skips the one that kept none (FR-020, E-23)");

  const auto entry = document.find("\"benchmarks\":[");
  const auto entrySeries = document.find("\"series\":[", entry);
  const auto entryEnd = document.find(']', entrySeries);
  check(entrySeries != std::string::npos,
        "the entry of a run that carries a series writes its own array (E-23)");
  check(document.find("\"cpu\":2", entrySeries) < entryEnd
            && document.find("\"cpu\":3", entrySeries) > entryEnd,
        "theFoldWithNoSeriesStandsSkipped: the entry array writes one item, "
        "the fold that carries a series (FR-020, E-23)");

  const auto loaded = sg::loadReport(kPath);
  check(loaded.has_value(), "the document of the mixed record loads (FR-020)");
  if (loaded.has_value()) {
    check(loaded->results.size() == 1
              && loaded->results[0].jitterFolds.size() == 2
              && loaded->results[0].jitterFolds[0].series.has_value()
              && !loaded->results[0].jitterFolds[1].series.has_value(),
          "theFoldWithNoSeriesStandsSkipped: the read leaves the bare fold "
          "without a series (FR-020, E-08)");
    check(loaded->runJitterStats.has_value()
              && loaded->runJitterStats->size() == 2
              && loaded->runJitterStats->at(0).series.has_value()
              && !loaded->runJitterStats->at(1).series.has_value(),
          "theFoldWithNoSeriesStandsSkipped: the run record reads back the "
          "same shape (FR-020, E-07)");
  }
  std::remove(kPath);
}

// specs/041-counter-probe FR-051, FR-052: the run-level record of where each
// needed device's counter limit came from. One device carries a figure the
// search answered, one carries no figure at all, and the reader of the written
// document answers both back. A device the run already seated keeps one entry,
// because one process publishes one figure and one finder per path, and a run
// that needed no device writes no key.
auto theCounterLimitRecordRoundTrips() -> void
{
  const auto searched = sg::detail::CounterLimitEntry {
      .device = "cpu",
      .limit = std::optional<std::size_t> {6},
      .source = sg::counters::LimitSource::PROBE};
  const auto silent =
      sg::detail::CounterLimitEntry {.device = "uncore_imc_77",
                                     .limit = {},
                                     .source = sg::counters::LimitSource::NONE};
  // A scripted run of this binary already seated its own device, so the
  // seating is weighed against the table the run left behind.
  const auto before = sg::detail::counterLimitEntries().size();
  sg::detail::noteCounterLimit(searched);
  sg::detail::noteCounterLimit(searched);
  sg::detail::noteCounterLimit(silent);
  const auto& table = sg::detail::counterLimitEntries();
  check(table.size() == before + 2 && table[before].device == "cpu"
            && table.back().device == "uncore_imc_77",
        "the seating names each needed device once (FR-051)");

  const std::vector<sg::detail::CounterLimitEntry> record {searched, silent};
  sg::ReportData data;
  check(sg::detail::writeReport("harness_json_limits.json",
                                data,
                                {},
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                record),
        "the writer wrote the document of two needed devices (FR-051)");
  const std::string text = sg::test::readText("harness_json_limits.json");
  check(text.find("\"counterLimits\":[{\"device\":\"cpu\",\"limit\":6,"
                  "\"source\":\"probe\"},{\"device\":\"uncore_imc_77\","
                  "\"limit\":null,\"source\":\"none\"}]")
            != std::string::npos,
        "the record writes the device, the figure and the finder word of each "
        "needed device (FR-051)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  if (!clean.has_value()) {
    std::printf("  the limit record: %s\n", clean.error().c_str());
  }
  check(clean.has_value(),
        "the document that carries the record passes the schema (FR-051)");

  const auto read = sg::detail::loadCounterLimits("harness_json_limits.json");
  check(read.has_value() && read->size() == 2 && read->front().device == "cpu"
            && read->front().limit == std::optional<std::size_t> {6}
            && read->front().source == sg::counters::LimitSource::PROBE
            && read->back().device == "uncore_imc_77"
            && !read->back().limit.has_value()
            && read->back().source == sg::counters::LimitSource::NONE,
        "the reader answers the entries the writer wrote (FR-052)");
  if (!read.has_value()) {
    std::printf("  the limit read: %s\n", read.error().c_str());
  }
  std::remove("harness_json_limits.json");

  check(sg::detail::writeReport("harness_json_nolimits.json", data),
        "the writer wrote the document of an empty record");
  check(
      sg::test::readText("harness_json_nolimits.json").find("\"counterLimits\"")
          == std::string::npos,
      "an empty record writes no key (FR-051)");
  const auto none = sg::detail::loadCounterLimits("harness_json_nolimits.json");
  check(none.has_value() && none->empty(),
        "a document without the record answers no entry (FR-052)");
  std::remove("harness_json_nolimits.json");
}

// specs/048-hwlat-probe T033: the eleven keys of the detector surfaces.
// The rows below write one document per carrier, so each search reads the
// one object its message names. The method word, the three stats figures,
// the gap mark, the three tracer settings, the three detector figures and
// the limit text ride the `hwlat` method or the `hwlat` engine alone, so a
// run of any other method or engine writes the bytes it writes today
// (FR-025 to FR-027, FR-029, FR-031, D-5, D-13, P-7, P-13).

// The one sentence the console prints and the document carries, the shape
// `contracts/console-report.md` states for the limits line (FR-030, FR-031).
constexpr std::string_view kLimitsText = "probe runs with interrupts enabled";

// The probe record of the detector rows: two cpus, the three attribution
// figures of FR-025 on each fold, and the kept gap records with the change
// of each span beside them (E-02, E-03, E-05). The first cpu's fold carries
// both deltas and two changes; the second fold carries neither delta and no
// change, the shape of a cpu whose SMI source stood unavailable (FR-011,
// FR-015).
auto detectorRecord(bool rawGaps) -> sg::detail::JitterReportRecord
{
  sg::detail::JitterReportRecord record;
  record.method = sg::JitterMethod::HWLAT;
  record.thresholdNs = 200;
  record.loopPeriodNs = 20;
  record.cpus = {2, 3};
  for (std::size_t index = 0; index < record.cpus.size(); ++index) {
    sg::detail::JitterReportCpu line;
    line.cpu = record.cpus[index];
    line.stats.gapCount = 2;
    line.stats.gapsPerSecond = 1.5;
    line.stats.minNs = 210;
    line.stats.maxNs = 280;
    line.stats.runtimeNs = 52000;
    line.stats.thresholdNs = 200;
    line.stats.loopPeriodNs = 20;
    line.stats.droppedGaps = 1;
    line.stats.smiGapCount = 2;
    line.gaps = {sg::detail::GapRecord {100, 210},
                 sg::detail::GapRecord {300, 280}};
    if (index == 0) {
      line.stats.smiDelta = 1;
      line.stats.nmiDelta = 4;
      line.smiChanges = {3, 0};
    }
    record.stats.push_back(line);
  }
  record.droppedGaps = 1;
  record.rawGaps = rawGaps;
  record.disclosure = "the run's disclosure";
  return record;
}

// The entry of a run that carried the detector method, with the fold of one
// benchmark's own windows: the count figure answers that fold's kept gaps,
// and neither delta stands, because the fold holds no window edge (FR-025,
// E-08).
auto detectorData() -> sg::ReportData
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  const auto run = sg::test::scriptedRun(options);
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  sg::JitterEntryFold fold;
  fold.cpu = 2;
  fold.stats.gapCount = 2;
  fold.stats.maxNs = 280;
  fold.stats.droppedGaps = 0;
  fold.stats.smiGapCount = 2;
  data.results.back().jitterFolds.push_back(fold);
  return data;
}

// The pre-check record of the tracer rows: one measured cpu, and the tracer
// record of the engine named, with two cpus ascending. The second cpu
// carries no figure, the file form of a buffer that holds no detector line
// (FR-027, E-04, A-6).
auto tracerRecord(const std::string& engine) -> sg::PrecheckRecord
{
  sg::PrecheckRecord record;
  sg::PrecheckCpuRecord cpu;
  cpu.cpu = 2;
  cpu.planned = true;
  cpu.stats.gapCount = 2;
  cpu.stats.smiDelta = 1;
  cpu.stats.nmiDelta = 4;
  cpu.stats.smiGapCount = 2;
  record.cpus.push_back(cpu);
  sg::PrecheckTracerRecord tracer;
  tracer.engine = engine;
  sg::PrecheckTracerCpuRecord first;
  first.cpu = 2;
  first.hardware = 2;
  first.maxGapNs = 7000;
  first.gapCount = 3;
  first.nmiCount = 2;
  sg::PrecheckTracerCpuRecord second;
  second.cpu = 5;
  second.hardware = 1;
  tracer.cpus = {first, second};
  if (engine == "hwlat") {
    tracer.windowUs = 10000;
    tracer.widthUs = 5000;
    tracer.thresholdUs = 1000;
  }
  record.tracer = tracer;
  return record;
}

// The written text of one hand-built pair of records, read back whole.
auto writtenText(const char* path,
                 const sg::ReportData& data,
                 const std::optional<sg::detail::JitterReportRecord>& jitter,
                 const std::optional<sg::PrecheckRecord>& precheck)
    -> std::string
{
  check(sg::detail::writeReport(path, data, {}, jitter, precheck),
        "the writer wrote the document of the detector record (FR-001)");
  return sg::test::readText(path);
}

// The three names of FR-025, spelled as the document spells them.
constexpr std::string_view kStatsKeys[] {
    "\"smiDelta\":1",
    "\"nmiDelta\":4",
    "\"smiGapCount\":2",
};

// Whether each key of the list stands in `text` after the key before it, from
// `from` to the end of the text.
auto standsInOrder(const std::string& text,
                   std::size_t from,
                   const std::vector<std::string_view>& keys) -> bool
{
  auto at = from;
  for (const auto key : keys) {
    const auto found = text.find(key, at);
    if (found == std::string::npos) {
      return false;
    }
    at = found + 1;
  }
  return true;
}

// FR-025, FR-004: the three figures stand in `jitter.stats[]` after
// `droppedGaps`, in the order the contract states, and the method word
// stands upper case beside them.
auto theDetectorStatsStandAfterTheStandingCount() -> void
{
  const auto text = writtenText("harness_json_hwlat_stats.json",
                                sg::ReportData {},
                                detectorRecord(false),
                                std::nullopt);
  check(text.find("\"method\":\"HWLAT\"") != std::string::npos,
        "the detector method writes its word to the document (FR-002, FR-004)");
  const auto dropped = text.find("\"droppedGaps\":1");
  std::vector<std::string_view> order {std::begin(kStatsKeys),
                                       std::end(kStatsKeys)};
  check(
      dropped != std::string::npos && standsInOrder(text, dropped, order),
      "the detector stats stand after the standing count key (FR-025, FR-004)");
  std::remove("harness_json_hwlat_stats.json");
}

// FR-025, D-1: the method keeps the standing gap loop, so it keeps no
// series. The `series` key stands out of the document the way it stands
// out of a `tsc` document.
auto theDetectorMethodWritesNoSeriesKey() -> void
{
  const auto text = writtenText("harness_json_hwlat_series.json",
                                sg::ReportData {},
                                detectorRecord(false),
                                std::nullopt);
  check(text.find("\"series\"") == std::string::npos,
        "the detector method writes no series key (FR-002, FR-025, D-1)");
  std::remove("harness_json_hwlat_series.json");
}

// FR-025: the per-benchmark item answers `null` for both deltas, because its
// fold covers one benchmark's windows and holds no window edge, and its
// count figure answers that item's own kept gaps.
auto thePerEntryStatsReadNullForTheTwoDeltas() -> void
{
  const auto text = writtenText("harness_json_hwlat_entry.json",
                                detectorData(),
                                detectorRecord(false),
                                std::nullopt);
  const auto entry = text.find("\"jitter\":{\"stats\":[{");
  check(entry != std::string::npos,
        "the entry of the detector run carries its own stats item (FR-025)");
  check(text.find("\"smiDelta\":null", entry) != std::string::npos
            && text.find("\"nmiDelta\":null", entry) != std::string::npos,
        "the per-entry item answers null for both deltas (FR-025, E-02)");
  check(text.find("\"smiGapCount\":2", entry) != std::string::npos,
        "the per-entry item folds the count of its own kept gaps (FR-025)");
  std::remove("harness_json_hwlat_entry.json");
}

// FR-025: `precheck.cpus[].stats` carries the three figures after the
// standing sixteen, the carrier the console pre-check block shares.
auto thePrecheckStatsCarryTheThreeFigures() -> void
{
  const auto record = tracerRecord("hwlat");
  const auto text = writtenText("harness_json_hwlat_precheck.json",
                                sg::ReportData {},
                                detectorRecord(false),
                                record);
  const auto stats = text.find("\"stats\":{\"gapCount\"");
  const auto dropped = text.find("\"droppedGaps\":0", stats);
  std::vector<std::string_view> order {std::begin(kStatsKeys),
                                       std::end(kStatsKeys)};
  check(stats != std::string::npos && dropped != std::string::npos
            && standsInOrder(text, dropped, order),
        "the pre-check stats carry the three figures after the standing "
        "sixteen (FR-025, FR-004)");
  std::remove("harness_json_hwlat_precheck.json");
}

// FR-025, FR-029, SC-006: a run of a standing method writes none of the
// three figures in any of the three carriers, because one writer fills all
// three.
auto aStandingMethodWritesNoAttributionFigure() -> void
{
  auto record = detectorRecord(false);
  record.method = sg::JitterMethod::FWQ;
  const auto text = writtenText("harness_json_fwq_stats.json",
                                detectorData(),
                                record,
                                tracerRecord("osnoise"));
  check(text.find("\"smiDelta\"") == std::string::npos
            && text.find("\"nmiDelta\"") == std::string::npos
            && text.find("\"smiGapCount\"") == std::string::npos,
        "a run of a standing method writes none of the three figures "
        "(FR-025, FR-029, SC-006)");
  std::remove("harness_json_fwq_stats.json");
}

// FR-026, FR-013: each raw gap carries the mark of its own span change:
// `SMI` above a change of zero, `NOT_SMI` at a change of zero, and `null`
// for the cpu whose SMI source stood unavailable.
auto theSmiMarkStandsBesideEachRawGap() -> void
{
  const auto text = writtenText("harness_json_hwlat_gaps.json",
                                sg::ReportData {},
                                detectorRecord(true),
                                std::nullopt);
  const auto first = text.find("\"stamp\":100");
  const auto second = text.find("\"stamp\":300");
  check(first != std::string::npos && second != std::string::npos
            && text.find("\"smi\":\"SMI\"", first) < second,
        "the gap whose span carried a change marks SMI after its gap figure "
        "(FR-026, FR-013, FR-014)");
  check(text.find("\"smi\":\"NOT_SMI\"", second) != std::string::npos,
        "the gap whose span carried no change marks NOT_SMI (FR-026, FR-014)");
  const auto third = text.find("\"cpu\":3");
  check(third != std::string::npos
            && text.find("\"smi\":null", third) != std::string::npos,
        "the cpu whose SMI source stood unavailable marks every gap null "
        "(FR-026, FR-015)");
  std::remove("harness_json_hwlat_gaps.json");
}

// FR-026, FR-029: the mark stands under the standing raw-gaps gate alone, so
// a run without it keeps the two names of the gap item.
auto aRunWithoutRawGapsWritesNoMark() -> void
{
  const auto text = writtenText("harness_json_hwlat_noraw.json",
                                sg::ReportData {},
                                detectorRecord(false),
                                std::nullopt);
  check(text.find("\"smi\"") == std::string::npos,
        "a run without the raw-gaps gate writes no gap mark (FR-026, FR-029)");
  std::remove("harness_json_hwlat_noraw.json");
}

// FR-027, FR-004: the three settings stand beside the engine word, in the
// order the contract states, and the values answer FR-020 in microseconds.
auto theTracerSettingsStandBesideTheEngineWord() -> void
{
  const auto record = tracerRecord("hwlat");
  const auto text = writtenText("harness_json_hwlat_tracer.json",
                                sg::ReportData {},
                                std::nullopt,
                                record);
  std::vector<std::string_view> order {
      "\"engine\":\"hwlat\"",
      "\"windowUs\":10000",
      "\"widthUs\":5000",
      "\"thresholdUs\":1000",
      "\"cpus\":[",
  };
  const auto tracer = text.find("\"tracer\":{");
  check(tracer != std::string::npos && standsInOrder(text, tracer, order),
        "the tracer settings stand beside the engine word, ahead of the cpus "
        "array (FR-027, FR-020, FR-004)");
  std::remove("harness_json_hwlat_tracer.json");
}

// FR-027, D-9: the three detector figures stand after the standing
// `hardware` figure of each tracer cpu, and a cpu whose buffer holds no
// detector line answers `null` for all three.
auto theDetectorFiguresStandBesideTheHardwareFigure() -> void
{
  const auto record = tracerRecord("hwlat");
  const auto text = writtenText("harness_json_hwlat_figures.json",
                                sg::ReportData {},
                                std::nullopt,
                                record);
  const auto first = text.find("\"hardware\":2");
  const auto second = text.find("\"cpu\":5");
  std::vector<std::string_view> order {
      "\"maxGapNs\":7000",
      "\"gapCount\":3",
      "\"nmiCount\":2",
  };
  check(first != std::string::npos && standsInOrder(text, first, order),
        "the three detector figures stand after the standing hardware figure "
        "(FR-027, FR-004)");
  check(second != std::string::npos
            && text.find("\"maxGapNs\":null", second) != std::string::npos
            && text.find("\"gapCount\":null", second) != std::string::npos
            && text.find("\"nmiCount\":null", second) != std::string::npos,
        "a cpu whose buffer holds no detector line answers null for the three "
        "figures (FR-027, A-6)");
  std::remove("harness_json_hwlat_figures.json");
}

// FR-029, T090: the three detector settings stand together or not at all. A
// record that carries a window and no width writes none of the three names, and
// a record that carries the window and the width and no threshold writes none
// either. The record that carries all three stands beside them as the positive
// control of both.
auto aPartialDetectorSettingWritesNoName() -> void
{
  for (const int dropped : {1, 2, 0}) {
    auto record = tracerRecord("hwlat");
    if (dropped == 1) {
      record.tracer->widthUs = std::nullopt;
    }
    if (dropped == 2) {
      record.tracer->thresholdUs = std::nullopt;
    }
    const char* const path = dropped == 1
        ? "harness_json_hwlat_nowidth.json"
        : (dropped == 2 ? "harness_json_hwlat_nothreshold.json"
                        : "harness_json_hwlat_allthree.json");
    const auto text =
        writtenText(path, sg::ReportData {}, std::nullopt, record);
    const bool window = text.find("\"windowUs\"") != std::string::npos;
    const bool width = text.find("\"widthUs\"") != std::string::npos;
    const bool threshold = text.find("\"thresholdUs\"") != std::string::npos;
    check(dropped == 0 ? (window && width && threshold)
                       : (!window && !width && !threshold),
          "the three detector settings are written together or not at all, so "
          "a " "record missing one of them writes none (FR-029, T090)");
    std::remove(path);
  }
}

// FR-027, P-7: the six new tracer names ride the `hwlat` engine alone, so an
// `osnoise` record that carries the member values writes none of them.
auto aStandingEngineWritesNoDetectorName() -> void
{
  auto record = tracerRecord("hwlat");
  record.tracer->engine = "timerlat";
  const auto text = writtenText("harness_json_timerlat_tracer.json",
                                sg::ReportData {},
                                std::nullopt,
                                record);
  // The stats object of the measured cpu carries a `gapCount` of its own, so
  // that name reads inside the tracer object alone.
  const auto tracer = text.find("\"tracer\":{");
  const auto tail = text.find("\"benchmarks\"", tracer);
  const auto limit = tail == std::string::npos ? text.size() : tail;
  check(text.find("\"windowUs\"") == std::string::npos
            && text.find("\"widthUs\"") == std::string::npos
            && text.find("\"thresholdUs\"") == std::string::npos
            && text.find("\"maxGapNs\"") == std::string::npos
            && text.find("\"nmiCount\"") == std::string::npos
            && tracer != std::string::npos
            && text.find("\"gapCount\":", tracer) >= limit,
        "a standing engine writes none of the six tracer names (FR-027, P-7)");
  std::remove("harness_json_timerlat_tracer.json");
}

// FR-030, FR-031: the limit text stands beside `disclosure` for the detector
// method, and a run of a standing method writes no such key.
auto theLimitsKeyStandsBesideTheDisclosure() -> void
{
  const auto written = writtenText("harness_json_hwlat_limits.json",
                                   sg::ReportData {},
                                   detectorRecord(false),
                                   std::nullopt);
  const auto disclosure = written.find("\"disclosure\"");
  check(disclosure != std::string::npos
            && written.find("\"limits\"", disclosure) != std::string::npos,
        "the limit text stands beside the disclosure key (FR-031, FR-004)");
  check(written.find(std::string("\"limits\":\"") + std::string(kLimitsText)
                         + "\"")
                != std::string::npos,
        "the limit key carries the one sentence the console prints (FR-030, "
        "FR-031)");
  auto standing = detectorRecord(false);
  standing.method = sg::JitterMethod::FTQ;
  const auto text = writtenText("harness_json_ftq_limits.json",
                                sg::ReportData {},
                                standing,
                                std::nullopt);
  check(text.find("\"limits\"") == std::string::npos,
        "a run of a standing method writes no limit key (FR-031, SC-006)");
  std::remove("harness_json_hwlat_limits.json");
  std::remove("harness_json_ftq_limits.json");
}

// FR-029, D-6: the schema carries every key the detector run writes, so the
// written document passes the checker at the moved minor.
auto theDetectorDocumentPassesTheSchema() -> void
{
  const auto record = tracerRecord("hwlat");
  const auto text = writtenText("harness_json_hwlat_schema.json",
                                detectorData(),
                                detectorRecord(true),
                                record);
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  check(clean.has_value(),
        "the document of the detector run passes the schema (FR-029, FR-004)");
  if (!clean.has_value()) {
    std::printf("  %s\n", clean.error().c_str());
  }
  std::remove("harness_json_hwlat_schema.json");
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
  aRefusedOpenQuotesTheReportPath();
  aRowWithoutARawEntryWritesNoColumns();
  theFoldWithNoSeriesStandsSkipped();
  theCounterLimitRecordRoundTrips();
  theGuardWordStandsPastTheTable();
  // specs/048-hwlat-probe T033: the eleven keys of the two detector
  // surfaces. The rows stand red until T040 writes the keys and T041 moves
  // the schema, so they close the unit.
  theDetectorStatsStandAfterTheStandingCount();
  theDetectorMethodWritesNoSeriesKey();
  thePerEntryStatsReadNullForTheTwoDeltas();
  thePrecheckStatsCarryTheThreeFigures();
  aStandingMethodWritesNoAttributionFigure();
  theSmiMarkStandsBesideEachRawGap();
  aRunWithoutRawGapsWritesNoMark();
  theTracerSettingsStandBesideTheEngineWord();
  theDetectorFiguresStandBesideTheHardwareFigure();
  aStandingEngineWritesNoDetectorName();
  aPartialDetectorSettingWritesNoName();
  theLimitsKeyStandsBesideTheDisclosure();
  theDetectorDocumentPassesTheSchema();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
