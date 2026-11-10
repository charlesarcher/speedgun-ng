// ============================================================================
// specs/043-fwq-ftq T026: the bytes a tsc run owes, unchanged (FR-026,
// FR-031, SC-004, R-19).
//
// A run that names no method writes neither new key, so its document
// matches the bytes master's writer produced apart from the schema minor
// and the version figures of the build, each mask checking that the span it
// takes out holds a number the way `maskHostFloor` does. The new keys stand
// absent from the document, the console block of the same probe state
// matches the bytes it recorded, and the document passes the schema at the
// minor the writer stamps. Frameworkless check()/fail() convention, the way
// `harness_json_schema_test.cpp` carries it. The suite carries no timed
// wait of its own (VI).
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/json_fixture.hpp"

using sg::test::check;

namespace
{
#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH"
#endif

constexpr const char* const kDocument = "harness_jitter_bytes_document.json";
constexpr const char* const kOsnoiseDocument =
    "harness_jitter_bytes_osnoise.json";
constexpr const char* const kOsnoiseBare = "harness_jitter_bytes_bare.json";
constexpr const char* const kBlockFile = "harness_jitter_bytes_block.txt";
constexpr std::string_view kDigits = "0123456789";
// The cpu index a synthetic record carries. The scenarios build the
// state and the fold in process, so no thread stands pinned to it, and
// the host-free rule of FR-013 reads no fixed cpu number in this file.
constexpr std::uint32_t kSyntheticProbeCpu = 0;
constexpr std::string_view kMasked = "\"?\"";

// The version figures of the build that wrote the file, held out of the
// identity the way `harness_noise_report_test` holds them out: the span the
// mask takes out carries the version of the build, so it holds a digit
// (FR-031).
auto maskSpeedgun(std::string text) -> std::string
{
  static constexpr std::string_view kHeader = "\"speedgun\":";
  const auto at = text.find(kHeader);
  check(at != std::string::npos,
        "the document carries the speedgun object (FR-031)");
  const auto start = at + kHeader.size();
  const auto end = text.find('}', start) + 1;
  const auto hidden = text.substr(start, end - start);
  check(hidden.find_first_of(kDigits) != std::string::npos,
        "the speedgun span holds the version figures of the build (FR-031)");
  text.replace(start, end - start, kMasked);
  return text;
}

// The schema minor, the one value the writer moves past master's: the span
// the mask takes out carries the version the writer stamps, so it holds a
// digit (FR-026).
auto maskSchemaVersion(std::string text) -> std::string
{
  static constexpr std::string_view kHeader = "\"schemaVersion\":\"";
  const auto at = text.find(kHeader);
  check(at != std::string::npos,
        "the document carries its schema version (FR-026)");
  const auto start = at + kHeader.size();
  const auto end = text.find('"', start);
  const auto hidden = text.substr(start, end - start);
  check(hidden.find_first_of(kDigits) != std::string::npos,
        "the schema version span holds the minor the writer stamps (FR-026)");
  text.replace(start, end - start, "?");
  return text;
}

// The run-level record of a tsc run: the mode, the two figures, the two
// probe cpus with their standing gap folds, and the disclosure text
// (E-07). The figures stand fixed, so the bytes the writer owes stand
// fixed.
auto tscRecord() -> sg::detail::JitterReportRecord
{
  sg::detail::JitterReportRecord record;
  record.thresholdNs = 200;
  record.loopPeriodNs = 20;
  record.cpus = {0, 1};
  for (const std::uint32_t cpu : record.cpus) {
    sg::detail::JitterReportCpu line;
    line.cpu = cpu;
    line.stats.gapCount = 3;
    line.stats.gapsPerSecond = 1.5;
    line.stats.minNs = 210;
    line.stats.medianNs = 220;
    line.stats.meanNs = 230.5;
    line.stats.p90Ns = 240;
    line.stats.p99Ns = 250;
    line.stats.p999Ns = 260;
    line.stats.p9999Ns = 270;
    line.stats.maxNs = 280;
    line.stats.totalNs = 710;
    line.stats.totalPercent = 1.25;
    line.stats.runtimeNs = 52000;
    line.stats.thresholdNs = 200;
    line.stats.loopPeriodNs = 20;
    line.stats.droppedGaps = 1;
    record.stats.push_back(line);
  }
  record.droppedGaps = 1;
  record.disclosure = "the run's disclosure";
  return record;
}

// The results of a scripted run, the entry carrying its own fold of the
// same covered windows: the per-entry object the per-entry series joins
// beside (E-08).
auto tscData() -> sg::ReportData
{
  sg::detail::RunOptions options;
  options.fixedIterations = sg::test::kWindowIterations;
  const auto run = sg::test::scriptedRun(options);
  sg::ReportData data;
  data.results.push_back(run.result);
  data.rowRaw = run.rowRaw;
  data.contextRaw = run.contextRaw;
  sg::JitterEntryFold fold;
  fold.cpu = kSyntheticProbeCpu;
  fold.stats = tscRecord().stats.at(0).stats;
  data.results.back().jitterFolds.push_back(fold);
  return data;
}

// The host-calibrated overhead floor figure of the row, the figure
// `maskHostFloor` masks in the console report: the span the mask takes out
// holds the printed figure, so it carries a dot (FR-026, FR-031).
auto maskOverheadFloor(std::string text) -> std::string
{
  static constexpr std::string_view kMark = "\"overheadFloorNs\":";
  for (auto at = text.find(kMark); at != std::string::npos;
       at = text.find(kMark, at))
  {
    const auto start = at + kMark.size();
    const auto end = text.find_first_of(",}", start);
    const auto hidden = text.substr(start, end - start);
    check(hidden.find('.') != std::string::npos,
          "the overhead floor span holds the printed floor figure (FR-026)");
    text.replace(start, end - start, "?");
    at = start;
  }
  return text;
}

// The document of the tsc run, the bytes as the writer wrote them.
auto tscDocument() -> std::string
{
  const auto record = tscRecord();
  const auto data = tscData();
  check(sg::detail::writeReport(kDocument, data, {}, record),
        "the writer wrote the document of the tsc run (FR-026)");
  return sg::test::readText(kDocument);
}

// The bytes master's writer produced for this record, the speedgun object
// and the schema minor held out (FR-026, SC-004).
constexpr std::string_view kMasterBytes =
    "{\"schemaVersion\":\"?\",\"speedgun\":\"?\",\"jitter\":{\"mode\":\"AL"
    "ONGSIDE\",\"thresholdNs\":200,\"loopPeriodNs\":20,\"cpus\":[0,1"
    "],\"stats\":[{\"cpu\":0,\"gapCount\":3,\"gapsPerSecond\":1.5,\"mi"
    "nNs\":210,\"medianNs\":220,\"meanNs\":230.5,\"p90Ns\":240,\"p99N"
    "s\":250,\"p999Ns\":260,\"p9999Ns\":270,\"maxNs\":280,\"totalNs\":"
    "710,\"totalPercent\":1.25,\"runtimeNs\":52000,\"thresholdNs\":"
    "200,\"loopPeriodNs\":20,\"droppedGaps\":1},{\"cpu\":1,\"gapCoun"
    "t\":3,\"gapsPerSecond\":1.5,\"minNs\":210,\"medianNs\":220,\"mea"
    "nNs\":230.5,\"p90Ns\":240,\"p99Ns\":250,\"p999Ns\":260,\"p9999Ns"
    "\":270,\"maxNs\":280,\"totalNs\":710,\"totalPercent\":1.25,\"run"
    "timeNs\":52000,\"thresholdNs\":200,\"loopPeriodNs\":20,\"dropp"
    "edGaps\":1}],\"droppedGaps\":1,\"disclosure\":\"the run's disc"
    "losure\"},\"benchmarks\":[{\"name\":\"bmJsonWindow\",\"suite\":\"b"
    "mJsonWindow\",\"case\":\"bmJsonWindow\",\"outcome\":\"MEASURED\","
    "\"reason\":null,\"hideRepetitionRows\":false,\"timeUnit\":\"NAN"
    "OSECONDS\",\"primaryIsCpu\":true,\"pauseSamplesFolded\":false"
    ",\"percentilePoints\":[],\"contexts\":[],\"noiseWarning\":null"
    ",\"jitter\":{\"stats\":[{\"cpu\":0,\"gapCount\":3,\"gapsPerSecond"
    "\":1.5,\"minNs\":210,\"medianNs\":220,\"meanNs\":230.5,\"p90Ns\":"
    "240,\"p99Ns\":250,\"p999Ns\":260,\"p9999Ns\":270,\"maxNs\":280,\""
    "totalNs\":710,\"totalPercent\":1.25,\"runtimeNs\":52000,\""
    "thresholdNs\":200,\"loopPeriodNs\":20,\"droppedGaps\":1}]},\"rows\""
    ":[{\"kind\":\"REPETITION\",\"label\":null,\"iterations\":4,\"time"
    "PerIterationNs\":250000.0,\"cpuTimePerIterationNs\":2500000"
    ".0,\"endpointSamples\":2,\"pauseSamplesFolded\":false,\"timeA"
    "vailability\":\"COUNTABLE\",\"metrics\":[{\"label\":\"work\",\""
    "value\":2.5,\"runningRatio\":1.0,\"scaled\":false,\"availability\""
    ":\"COUNTABLE\",\"pass\":null}],\"counters\":[],\"counterDisclosure\""
    ":null,\"fit\":null,\"overheadFloorNs\":?,\"percentiles\":["
    "],\"chunkCount\":0,\"chunkIterations\":0,\"timeStats\":null,\"m"
    "etricStats\":[]}],\"metricLabels\":[\"work\"],\"counterLabels\""
    ":[],\"counterStats\":[],\"statisticValues\":[],\"metricMethod"
    "\":\"MULTIPLEX\",\"passCount\":1,\"passSpread\":null,\"spreadWar"
    "ning\":false}]}";

// The console block the standing printer prints for this probe state
// (FR-026, FR-031).
constexpr std::string_view kConsoleBytes =
    "jitter probe: cpus 0\n"
    "gapCount: 3\n"
    "gapsPerSecond: 7500000.00\n"
    "minNs: 210\n"
    "medianNs: 220\n"
    "meanNs: 236.67\n"
    "p90Ns: 280\n"
    "p99Ns: 280\n"
    "p999Ns: 280\n"
    "p9999Ns: 280\n"
    "maxNs: 280\n"
    "totalNs: 710\n"
    "totalPercent: 177.50\n"
    "runtimeNs: 400\n"
    "thresholdNs: 200\n"
    "loopPeriodNs: 20\n"
    "droppedGaps: 1\n"
    "jitter: probe on cpus 0\n"
    "jitter: loop period 20 ns, threshold 200 ns\n"
    "jitter: cpu 0 kept 3 dropped 1\n"
    "jitter: split and percentiles use the kept gaps only\n";

// The keys specs/043 adds, each spelled as the document spells it
// (FR-019, FR-026).
constexpr std::string_view kNewKeys[] = {
    "\"method\"",          "\"series\"",
    "\"periods\"",         "\"workCount\"",
    "\"unitTimeNs\"",      "\"targetNs\"",
    "\"quantumNs\"",       "\"calibrationSpanNs\"",
    "\"sampleCount\"",     "\"minValue\"",
    "\"medianValue\"",     "\"meanValue\"",
    "\"p90Value\"",        "\"p99Value\"",
    "\"p999Value\"",       "\"p9999Value\"",
    "\"maxValue\"",        "\"noisePercent\"",
    "\"seriesRuntimeNs\"", "\"droppedSamples\"",
    "\"resampleStepNs\"",  "\"spectrumPoints\"",
};

// The printer's own bytes, taken through the redirect the standing capture
// carries.
auto captureBlock(const std::function<void()>& body) -> std::string
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kBlockFile, "w", stdout) == nullptr) {
    sg::test::fail("cannot redirect the probe block");
  }
  body();
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    sg::test::fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(kBlockFile);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(kBlockFile);
  return buffer.str();
}

// FR-026, SC-004, R-19: the document of a run that names no method matches
// the bytes master's writer produced, apart from the two masked spans.
auto aTscRunMatchesTheFixtureBytes() -> void
{
  const std::string bytes =
      maskSchemaVersion(maskSpeedgun(maskOverheadFloor(tscDocument())));
  if (bytes != kMasterBytes) {
    std::printf("  the masked bytes: %s\n", bytes.c_str());
  }
  check(bytes == kMasterBytes,
        "aTscRunMatchesTheFixtureBytes: the tsc run's bytes match the master "
        "fixture (FR-026, SC-004, R-19)");
}

// FR-026, FR-031: each mask takes out a span that holds a number, the way
// `maskHostFloor` checks the span it masks.
auto eachMaskedSpanHoldsANumber() -> void
{
  const std::string document = tscDocument();
  check(maskSpeedgun(document) != document,
        "eachMaskedSpanHoldsANumber: the speedgun mask takes out the version "
        "figures (FR-031)");
  check(maskSchemaVersion(document) != document,
        "eachMaskedSpanHoldsANumber: the schema mask takes out the stamped "
        "minor (FR-026)");
  check(maskOverheadFloor(document) != document,
        "eachMaskedSpanHoldsANumber: the floor mask takes out the calibrated "
        "figure (FR-026, FR-031)");
}

// FR-019, FR-026: a run with no method key writes neither new key, in the
// run-level object or in the per-entry object. The keys specs/048 adds ride
// the same rule for the standing methods, so the same document answers for
// them (FR-025, FR-029).
constexpr std::string_view kDetectorKeys[] = {
    "\"smiDelta\":",
    "\"nmiDelta\":",
    "\"smiGapCount\":",
    "\"smi\":",
    "\"limits\":",
    "\"windowUs\":",
    "\"widthUs\":",
    "\"thresholdUs\":",
    "\"maxGapNs\":",
    "\"nmiCount\":",
};

auto theNewKeysStandAbsentFromTheDocument() -> void
{
  const std::string document = tscDocument();
  for (const std::string_view key : kNewKeys) {
    if (document.find(key) != std::string::npos) {
      std::printf("  the document carries %s\n", std::string(key).c_str());
    }
    check(document.find(key) == std::string::npos,
          "theNewKeysStandAbsentFromTheDocument: the document carries no new "
          "key (FR-019, FR-026)");
  }
  for (const std::string_view key : kDetectorKeys) {
    if (document.find(key) != std::string::npos) {
      std::printf("  the document carries %s\n", std::string(key).c_str());
    }
    check(document.find(key) == std::string::npos,
          "theNewKeysStandAbsentFromTheDocument: the tsc document carries none "
          "of the detector keys (FR-025, FR-029, SC-006)");
  }
}

// specs/045-merge-fixes phase 16 (FR-039): the record's disclosure carries
// the time-base clause the probe measured, and the writer puts the text
// beside the jitter object verbatim, so the document answers the record
// unchanged in structure (FR-026).
auto theTimeBaseClauseRidesTheDisclosureUnchanged() -> void
{
  auto record = tscRecord();
  record.disclosure =
      "probe on cpus 0,1; loop period 20 ns, threshold 200 ns; time base "
      "tsc ticks scaled to ns at 250000 Hz; cpu 0 kept 3 dropped 1; cpu 1 "
      "kept 3 dropped 1; split and percentiles use the kept gaps only";
  const auto data = tscData();
  check(sg::detail::writeReport(kDocument, data, {}, record),
        "the writer wrote the document of the tsc run (FR-026)");
  const std::string document = sg::test::readText(kDocument);
  const std::string clause =
      std::string("\"disclosure\"") + ":" + "\"" + record.disclosure + "\"";
  check(document.find(clause) != std::string::npos,
        "theTimeBaseClauseRidesTheDisclosureUnchanged: the document carries "
        "the disclosure the record carries, the time-base clause included, "
        "verbatim beside the jitter object (FR-039, FR-026)");
}

// The pre-check record of a standing tracer engine: one measured cpu that
// carries the three detector figures of the record, and the tracer entry of
// `osnoise` with the settings and the detector figures set as well. The
// writer answers none of them for a standing engine, so the two documents
// below fold to one byte string (FR-025, FR-027, P-7).
auto osnoiseRecord() -> sg::PrecheckRecord
{
  sg::PrecheckRecord record;
  record.serial = true;
  record.windowMs = 10;
  sg::PrecheckCpuRecord cpu;
  cpu.cpu = 2;
  cpu.planned = true;
  cpu.stats.gapCount = 2;
  cpu.stats.gapsPerSecond = 1.5;
  cpu.stats.maxNs = 280;
  cpu.stats.smiDelta = 1;
  cpu.stats.nmiDelta = 4;
  cpu.stats.smiGapCount = 2;
  cpu.attribution.interrupts = 5;
  record.cpus.push_back(cpu);
  sg::PrecheckTracerRecord tracer;
  tracer.engine = "osnoise";
  tracer.windowUs = 10000;
  tracer.widthUs = 5000;
  tracer.thresholdUs = 1000;
  sg::PrecheckTracerCpuRecord entry;
  entry.cpu = 2;
  entry.nmi = 3;
  entry.hardware = 2;
  entry.maxGapNs = 7000;
  entry.gapCount = 3;
  entry.nmiCount = 2;
  tracer.cpus = {entry};
  record.tracer = tracer;
  return record;
}

// The same record with none of the detector figures: the record of a run that
// names neither new word. The two documents stand the two halves of the byte
// claim below (FR-025, FR-029, P-7).
auto bareRecord(sg::PrecheckRecord record) -> sg::PrecheckRecord
{
  auto& stats = record.cpus.at(0).stats;
  stats.smiDelta = std::nullopt;
  stats.nmiDelta = std::nullopt;
  stats.smiGapCount = 0;
  auto& tracer = *record.tracer;
  tracer.windowUs = std::nullopt;
  tracer.widthUs = std::nullopt;
  tracer.thresholdUs = std::nullopt;
  tracer.cpus.at(0).maxGapNs = std::nullopt;
  tracer.cpus.at(0).gapCount = std::nullopt;
  tracer.cpus.at(0).nmiCount = std::nullopt;
  return record;
}

// The document the writer writes for one pre-check record, the speedgun
// object and the schema minor held out the way the standing rows hold them
// out.
auto maskedDocument(const sg::PrecheckRecord& record,
                    const char* const path) -> std::string
{
  sg::ReportData data;
  data.precheck = record;
  check(
      sg::detail::writeReport(path, data, {}, std::nullopt, record),
      "the writer wrote the document of the standing tracer record " "(FR-"
                                                                     "029)");
  return maskSchemaVersion(maskSpeedgun(sg::test::readText(path)));
}

// FR-025, FR-029, FR-027, SC-006: an `osnoise` document loses no byte and no
// name, because the six new tracer names and the three stats names ride the
// detector surfaces alone.
auto anOsnoiseDocumentLosesNoByteAndNoName() -> void
{
  const auto record = osnoiseRecord();
  const std::string filled = maskedDocument(record, kOsnoiseDocument);
  const std::string bare = maskedDocument(bareRecord(record), kOsnoiseBare);
  if (filled != bare) {
    std::printf("  the record with the figures: %s\n", filled.c_str());
    std::printf("  the record without them:   %s\n", bare.c_str());
  }
  check(filled == bare,
        "anOsnoiseDocumentLosesNoByteAndNoName: an osnoise document of the "
        "detector figures folds to the bytes of the same record without them "
        "(FR-025, FR-027, FR-029, SC-006)");
  for (const std::string_view key : kDetectorKeys) {
    if (filled.find(key) != std::string::npos) {
      std::printf("  the document carries %s\n", std::string(key).c_str());
    }
    check(filled.find(key) == std::string::npos,
          "anOsnoiseDocumentLosesNoByteAndNoName: the osnoise document carries "
          "none of the detector keys (FR-025, FR-027, P-7)");
  }
  const auto tracer = filled.find("\"tracer\":{");
  check(tracer != std::string::npos
            && filled.find("\"gapCount\":", tracer) == std::string::npos,
        "anOsnoiseDocumentLosesNoByteAndNoName: the tracer entry of a standing "
        "engine carries no detector gap count (FR-027, P-7)");
}

// FR-026, FR-031: the console block of the same probe state matches the
// bytes it recorded.
auto theStandingConsoleLinesMatchTheRecordedBytes() -> void
{
  // The probe state whose block the standing printer prints: one cpu,
  // three kept gaps, one dropped, the two figures of the run.
  sg::detail::ProbeCpuState state;
  state.cpu = kSyntheticProbeCpu;
  state.method = sg::JitterMethod::TSC;
  state.buffer = {sg::detail::GapRecord {100, 210},
                  sg::detail::GapRecord {300, 220},
                  sg::detail::GapRecord {500, 280}};
  state.kept = 3;
  state.droppedGaps = 1;
  state.droppedTotalNs = 90;
  state.droppedMaxNs = 90;
  state.loopPeriodNs = 20;
  state.thresholdNs = 200;
  const std::vector<const sg::detail::ProbeCpuState*> states {&state};
  const auto block =
      captureBlock([&] { sg::detail::printJitterReport(true, states); });
  if (block != kConsoleBytes) {
    std::printf("  the block: %s\n", block.c_str());
  }
  check(block == kConsoleBytes,
        "theStandingConsoleLinesMatchTheRecordedBytes: the block of the tsc "
        "run matches the recorded bytes (FR-026, FR-031, SC-004)");
}

// FR-026, R-16: the document of the tsc run passes the schema at the minor
// the writer stamps.
auto aTscRunPassesTheSchemaAtTheStampedMinor() -> void
{
  const std::string document = tscDocument();
  // specs/048-hwlat-probe FR-004, FR-029, D-5: the eleven detector keys move
  // the stamp to 1.16, the next free minor, because specs/041-counter-probe
  // landed 1.15 on master while this branch stood.
  check(document.find("\"schemaVersion\":\"1.16\"") != std::string::npos,
        "aTscRunPassesTheSchemaAtTheStampedMinor: the writer stamps the minor "
        "1.16 (FR-004, FR-026, FR-029, R-16, specs/042-topdown FR-036, "
        "specs/046-hunter-topdown FR-020, specs/041-counter-probe FR-051)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, document);
  check(clean.has_value(),
        "aTscRunPassesTheSchemaAtTheStampedMinor: the tsc document passes the "
        "schema (FR-008, FR-026)");
  if (!clean.has_value()) {
    std::printf("  %s\n", clean.error().c_str());
  }
}

}  // namespace

auto main() -> int
{
  aTscRunMatchesTheFixtureBytes();
  eachMaskedSpanHoldsANumber();
  theNewKeysStandAbsentFromTheDocument();
  theTimeBaseClauseRidesTheDisclosureUnchanged();
  theStandingConsoleLinesMatchTheRecordedBytes();
  aTscRunPassesTheSchemaAtTheStampedMinor();
  // specs/048-hwlat-probe T035: the byte identity a standing run owes while
  // the detector keys land. The row stands red until T040 stamps the moved
  // minor, so it closes the unit.
  anOsnoiseDocumentLosesNoByteAndNoName();
  std::remove(kDocument);
  std::remove(kOsnoiseDocument);
  std::remove(kOsnoiseBare);
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
