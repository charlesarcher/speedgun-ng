// ============================================================================
// specs/036-osnoise-timerlat-tracers T013 to T016: the record surfaces of
// the tracer (FR-013, FR-014, FR-015, FR-006, FR-010, FR-012, FR-016,
// R-10, R-11, R-14).
//
// The console rows print a hand-built record through
// `detail::printPrecheckReport` and fold its bytes against the fixtures of
// the standing block: the tracer lines stand after the host settings and
// before the warning lines, and the not-available signal rides the
// printer's second parameter, because the record leaves the member empty
// for a tracer-free run and for a not-available run alike. The JSON rows
// round-trip the tracer record through the writer and `loadReport`, the
// schema rows read `docs/schema/report-v1.json` and check a written
// document against it, and the byte-identity rows fold the document of a
// record without the tracer member against the fixture bytes. The schema
// path enters through SG_SCHEMA_PATH, the way `harness_json_report_test`
// takes it. Frameworkless check()/fail() convention, the way
// `harness_json_schema_test.cpp` carries it. The suite carries no
// timed wait of its own (VI).
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "detail/json_fixture.hpp"

using sg::test::check;
using sg::test::fail;

namespace
{

#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH"
#endif

// The console capture the block tests read: the printer's own bytes, taken
// through the redirect the standing capture carries (T013).
auto capturePrecheck(const sg::PrecheckRecord& record,
                     sg::TracerEngine requestedEngine) -> std::string
{
  const char* const path = "harness_noise_report_block.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the pre-check block");
  }
  sg::detail::printPrecheckReport(record, requestedEngine);
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path);
  return buffer.str();
}

// One record the block tests read: one planned cpu with a warning, one
// candidate, the host settings with an absent value, and the skipped
// group. The tracer member stands empty until a test fills it.
auto standingRecord() -> sg::PrecheckRecord
{
  sg::PrecheckRecord record;
  sg::PrecheckCpuRecord planned;
  planned.cpu = 0;
  planned.planned = true;
  planned.stats.gapCount = 1;
  planned.stats.gapsPerSecond = 2.5;
  planned.stats.maxNs = 100;
  planned.attribution.interrupts = 5;
  planned.attribution.softirqs.emplace_back("timer",
                                            std::optional<std::uint64_t> {2});
  planned.attribution.switches = 1;
  planned.attribution.schedules = 3;
  record.cpus.push_back(planned);
  sg::PrecheckCpuRecord candidate;
  candidate.cpu = 1;
  candidate.planned = false;
  record.cpus.push_back(candidate);
  record.host.isolcpus = "2-3";
  record.host.rcuNocbs = "1";
  record.host.governors.emplace_back(
      0U, std::optional<std::string> {"performance"});
  record.host.irqAffinity.emplace_back(0U, std::vector<std::uint32_t> {5, 7});
  record.skippedGroups = {2};
  return record;
}

// The tracer record of the figure rows: two cpus ascending, the second
// carrying absent figures beside the counts, the file form of the
// console's `not available` (FR-010, E-17).
auto tracerRecord() -> sg::PrecheckTracerRecord
{
  sg::PrecheckTracerRecord tracer;
  tracer.engine = "osnoise";
  sg::PrecheckTracerCpuRecord first;
  first.cpu = 2;
  first.nmi = 3;
  first.irq = 1;
  first.softirq = 4;
  first.thread = 7;
  first.hardware = 2;
  sg::PrecheckTracerCpuRecord second;
  second.cpu = 5;
  second.nmi = 1;
  second.softirq = 2;
  tracer.cpus = {first, second};
  return tracer;
}

// The standing block of the fixture record, byte for byte: the sixteen
// stat lines and the attribution deltas of each cpu, the host settings
// with the absent `nohz_full`, and the warning and pinning lines. The
// tracer lines join after the `irq affinity` line and before the warning
// line (R-14), so each fixture below splices its own lines there.
constexpr std::string_view kBlockHead =
    "precheck cpu 0 (planned)\n"
    "gapCount: 1\n"
    "gapsPerSecond: 2.50\n"
    "minNs: 0\n"
    "medianNs: 0\n"
    "meanNs: 0.00\n"
    "p90Ns: 0\n"
    "p99Ns: 0\n"
    "p999Ns: 0\n"
    "p9999Ns: 0\n"
    "maxNs: 100\n"
    "totalNs: 0\n"
    "totalPercent: 0.00\n"
    "runtimeNs: 0\n"
    "thresholdNs: 0\n"
    "loopPeriodNs: 0\n"
    "droppedGaps: 0\n"
    "interrupts delta: 5 (source: /proc/interrupts)\n"
    "softirq timer delta: 2 (source: /proc/softirqs)\n"
    "switch delta: 1, schedule delta: 3 (source: /proc/schedstat)\n"
    "precheck cpu 1 (candidate)\n"
    "gapCount: 0\n"
    "gapsPerSecond: 0.00\n"
    "minNs: 0\n"
    "medianNs: 0\n"
    "meanNs: 0.00\n"
    "p90Ns: 0\n"
    "p99Ns: 0\n"
    "p999Ns: 0\n"
    "p9999Ns: 0\n"
    "maxNs: 0\n"
    "totalNs: 0\n"
    "totalPercent: 0.00\n"
    "runtimeNs: 0\n"
    "thresholdNs: 0\n"
    "loopPeriodNs: 0\n"
    "droppedGaps: 0\n"
    "interrupts delta: not available (source: /proc/interrupts)\n"
    "switch delta: not available, schedule delta: not available "
    "(source: /proc/schedstat)\n"
    "host settings: isolcpus=2-3 nohz_full=not available rcu_nocbs=1\n"
    "governor: cpu 0 performance (source: /sys devices system cpu "
    "scaling_governor)\n"
    "irq affinity: cpu 0 5,7 (source: /proc/irq)\n";

constexpr std::string_view kBlockTail =
    "warning: cpu 0 gapsPerSecond 2.50 maxNs 100\n"
    "pinning candidates: cpu 0 2.50\n";

// The tracer lines of the fixture record, byte for byte: the header line
// naming the engine word and the cpus, then one figure line per source per
// tracer cpu in the NMI, IRQ, softirq, thread, hardware order, cpus
// ascending, each absent figure standing the `not available` wording in
// its figure position (FR-013, FR-010, R-14).
constexpr std::string_view kTracerLines =
    "tracer: engine osnoise cpus 2,5\n"
    "tracer cpu 2 nmi: 3\n"
    "tracer cpu 2 irq: 1\n"
    "tracer cpu 2 softirq: 4\n"
    "tracer cpu 2 thread: 7\n"
    "tracer cpu 2 hardware: 2\n"
    "tracer cpu 5 nmi: 1\n"
    "tracer cpu 5 irq: not available\n"
    "tracer cpu 5 softirq: 2\n"
    "tracer cpu 5 thread: not available\n"
    "tracer cpu 5 hardware: not available\n";

constexpr std::string_view kNotAvailableLine = "tracer: not available\n";

// T013: the tracer lines stand after the host settings and before the
// warning lines, in the order the contract states, and an absent figure
// prints the standing wording in its figure position.
auto theTracerLinesStandBesideTheStandingBlock() -> void
{
  auto record = standingRecord();
  record.tracer = tracerRecord();
  const auto block = capturePrecheck(record, sg::TracerEngine::OSNOISE);
  check(block == std::string(kBlockHead) + std::string(kTracerLines)
              + std::string(kBlockTail),
        "the tracer lines stand after the host settings and before the "
        "warnings, byte for byte (FR-013, R-14)");
  check(block.find("tracer: engine osnoise cpus 2,5\n") != std::string::npos,
        "the header line names the engine word and the cpus (FR-013, D-3)");
  // The figure lines stand per source per tracer cpu in the contract
  // order: the find positions over the block ascend line by line.
  std::array<std::string_view, 10> lines {
      "tracer cpu 2 nmi: 3\n",
      "tracer cpu 2 irq: 1\n",
      "tracer cpu 2 softirq: 4\n",
      "tracer cpu 2 thread: 7\n",
      "tracer cpu 2 hardware: 2\n",
      "tracer cpu 5 nmi: 1\n",
      "tracer cpu 5 irq: not available\n",
      "tracer cpu 5 softirq: 2\n",
      "tracer cpu 5 thread: not available\n",
      "tracer cpu 5 hardware: not available\n",
  };
  std::size_t at = 0;
  bool ordered = true;
  for (const auto line : lines) {
    const auto found = block.find(line, at);
    if (found == std::string::npos) {
      ordered = false;
      break;
    }
    at = found + 1;
  }
  check(ordered,
        "one figure line stands per source per tracer cpu, the sources in "
        "the NMI, IRQ, softirq, thread, hardware order, cpus ascending "
        "(FR-013)");
  check(block.find("tracer cpu 5 irq: not available\n")
              != std::string::npos,
        "an absent figure prints the standing wording in its figure "
        "position (FR-010)");
}

// T013: the not-available record prints the one disclosure line, and no
// figure line stands beside it (FR-016, R-14).
auto theNotAvailableRecordPrintsOneDisclosureLine() -> void
{
  auto record = standingRecord();
  const auto block = capturePrecheck(record, sg::TracerEngine::OSNOISE);
  check(block == std::string(kBlockHead) + std::string(kNotAvailableLine)
              + std::string(kBlockTail),
        "the not-available record prints the one disclosure line and the "
        "standing block stands byte-identical (FR-016, R-14)");
  check(block.find("tracer cpu") == std::string::npos,
        "the not-available case stands one line, no figure line beside it "
        "(FR-016)");
}

// T016: a run that requests no tracer prints no tracer line, and every
// standing line folds byte-identical to the fixture (FR-012, FR-15, R-10).
auto aTracerFreeRunPrintsNoTracerLine() -> void
{
  auto record = standingRecord();
  const auto block = capturePrecheck(record, sg::TracerEngine::NONE);
  check(block == std::string(kBlockHead) + std::string(kBlockTail),
        "a tracer-free run prints no tracer line and every standing line "
        "folds byte-identical to the fixture (FR-012, FR-015, R-10)");
  check(block.find("tracer") == std::string::npos,
        "a run that requests no tracer prints no tracer line (FR-012)");
}

// T014: the loaded tracer record answers the written record field by
// field. The shared fixture comparison leaves the optional member out,
// so the tracer record compares here, beside it.
auto sameTracerRecord(const sg::PrecheckTracerRecord& actual,
                      const sg::PrecheckTracerRecord& want) -> bool
{
  const auto sameCpu = [](const sg::PrecheckTracerCpuRecord& got,
                          const sg::PrecheckTracerCpuRecord& asked)
  {
    return got.cpu == asked.cpu && got.nmi == asked.nmi && got.irq == asked.irq
        && got.softirq == asked.softirq && got.thread == asked.thread
        && got.hardware == asked.hardware;
  };
  return actual.engine == want.engine && actual.cpus.size() == want.cpus.size()
      && std::equal(actual.cpus.begin(),
                    actual.cpus.end(),
                    want.cpus.begin(),
                    sameCpu);
}

// T014: the writer emits the tracer object inside `precheck` after
// `skippedGroups`, and `loadReport` reads it back with equal values
// (FR-014, R-08, R-11).
auto theTracerRecordRoundTrips() -> void
{
  auto record = standingRecord();
  record.tracer = tracerRecord();
  sg::ReportData data;
  data.precheck = record;
  check(sg::detail::writeReport(
            "harness_noise_report.json", data, {}, std::nullopt, record),
        "the writer wrote the document (FR-014)");
  const std::string text = sg::test::readText("harness_noise_report.json");
  const auto skippedAt = text.find("\"skippedGroups\"");
  const auto tracerAt = text.find("\"tracer\":{\"engine\":\"osnoise\"");
  check(skippedAt != std::string::npos && tracerAt != std::string::npos
            && skippedAt < tracerAt,
        "the writer emits the tracer object inside precheck after "
        "skippedGroups, carrying engine and the cpus entries (R-11)");
  // The written fixture document carries the six-key precheck order the
  // schema states (T019).
  const auto precheckAt = text.find("\"precheck\":{");
  std::size_t at = precheckAt;
  bool ordered = true;
  for (const auto* key : {"\"serial\"",
                          "\"windowMs\"",
                          "\"cpus\"",
                          "\"hostSettings\"",
                          "\"skippedGroups\"",
                          "\"tracer\""})
  {
    const auto found = text.find(key, at);
    if (found == std::string::npos) {
      ordered = false;
      break;
    }
    at = found + 1;
  }
  check(ordered,
        "the written document carries the six-key precheck order (R-11)");
  const auto loaded = sg::loadReport("harness_noise_report.json");
  check(loaded.has_value(), "the reader read the written document (FR-014)");
  if (loaded.has_value()) {
    check(loaded->precheck.has_value(),
          "the document carries the pre-check record (FR-014)");
    if (loaded->precheck.has_value()) {
      check(sg::test::samePrecheckRecord(*loaded->precheck, record),
            "the loaded standing record answers the run (FR-014)");
      check(loaded->precheck->tracer.has_value(),
            "the document carries the tracer record (FR-014)");
      if (loaded->precheck->tracer.has_value()) {
        check(sameTracerRecord(*loaded->precheck->tracer, *record.tracer),
              "the loaded tracer record answers the run field by field "
              "(FR-014)");
      }
    }
  }
}

// T014: a document written before the key reads as the tracer-less record
// (FR-014, R-11).
auto aDocumentWrittenBeforeTheKeyReadsTracerLess() -> void
{
  auto record = standingRecord();
  sg::ReportData data;
  data.precheck = record;
  check(sg::detail::writeReport(
            "harness_noise_report_plain.json", data, {}, std::nullopt, record),
        "the writer wrote the tracer-less document (FR-014)");
  const auto loaded = sg::loadReport("harness_noise_report_plain.json");
  check(loaded.has_value() && loaded->precheck.has_value(),
        "the reader read the tracer-less document (FR-014)");
  if (loaded.has_value() && loaded->precheck.has_value()) {
    check(!loaded->precheck->tracer.has_value(),
          "a document written before the key reads as the tracer-less "
          "record (FR-014, R-11)");
  }
}

// T015: the schema carries the tracer key in the precheck object's
// properties and in its x-order after skippedGroups, outside its required
// list; the top-level x-order stands unchanged (FR-006, D-3, R-11).
auto theSchemaCarriesTheTracerKey() -> void
{
  const std::string schema = sg::test::readText(SG_SCHEMA_PATH);
  check(!schema.empty(), "the schema file reads");
  constexpr std::
      string_view
          kOrder =
              "\"x-order\": [\"serial\", \"windowMs\", \"cpus\", "
              "\"hostSettings\", " "\"skippedGroups\", \"tracer\"]";
  check(schema.find(kOrder) != std::string::npos,
        "the precheck x-order reads the six keys (FR-006, R-11)");
  const auto orderAt = schema.find(kOrder);
  const auto requiredAt = schema.find("\"required\"", orderAt);
  check(requiredAt != std::string::npos,
        "the precheck required list stands read (R-11)");
  if (requiredAt != std::string::npos) {
    const auto requiredEnd = schema.find(']', requiredAt);
    const std::string required =
        schema.substr(requiredAt, requiredEnd - requiredAt);
    check(required.find("tracer") == std::string::npos,
          "the tracer key stands outside the required list (R-11)");
  }
  check(schema.find("\"tracer\": {", orderAt) != std::string::npos,
        "the precheck properties carry the tracer key (D-3)");
  constexpr std::
      string_view
          kTopOrder =
              "\"x-order\": [\"schemaVersion\", \"speedgun\", \"run\", "
              "\"jitter\", " "\"precheck\", \"benchmarks\", \"hunt\"]";
  check(schema.find(kTopOrder) != std::string::npos,
        "the top-level x-order stands unchanged (FR-006)");
}

// T015, T019: the checker takes a written document that carries the tracer
// record against the extended schema, and the writer stands the minor at
// 1.7 (FR-006, D-3).
auto theWrittenTracerDocumentPassesTheSchema() -> void
{
  auto record = standingRecord();
  record.tracer = tracerRecord();
  sg::ReportData data;
  data.precheck = record;
  check(sg::detail::writeReport(
            "harness_noise_report_schema.json", data, {}, std::nullopt, record),
        "the writer wrote the document (FR-006)");
  const std::string document =
      sg::test::readText("harness_noise_report_schema.json");
  check(document.find("\"schemaVersion\":\"1.7\"") != std::string::npos,
        "the writer stands the schema minor at 1.7 (D-3, R-11)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, document);
  check(clean.has_value(),
        "a document carrying the tracer record passes the schema (FR-006)");
  if (!clean.has_value()) {
    std::printf("  %s\n", clean.error().c_str());
  }
}

// T016: the values that state which build wrote the file, held out of the
// identity the way `harness_hunt_json_test` holds them out of its capture
// (FR-015): the `speedgun` object names the build that runs this test.
auto maskSpeedgun(std::string text) -> std::string
{
  const std::string header = "\"speedgun\":";
  const auto at = text.find(header);
  if (at == std::string::npos) {
    return text;
  }
  const auto start = at + header.size();
  const auto end = text.find('}', start) + 1;
  text.replace(start, end - start, "\"?\"");
  return text;
}

// T016: a record without the tracer member writes a document whose bytes
// fold equal to the fixture bytes, the fixture carrying master's key set
// with `schemaVersion` at 1.7 (FR-012, FR-015, R-10).
constexpr std::string_view kDocumentFixture =
    "{\"schemaVersion\":\"1.7\",\"speedgun\":\"?\",\"precheck\":{"
    "\"serial\":false,\"windowMs\":0,\"cpus\":[],"
    "\"hostSettings\":{\"isolcpus\":null,\"nohz_full\":null,"
    "\"rcu_nocbs\":null,\"governors\":{},\"irqAffinity\":{}},"
    "\"skippedGroups\":[]},\"benchmarks\":[]}";

auto theTracerLessDocumentMatchesTheFixtureBytes() -> void
{
  sg::PrecheckRecord record;
  sg::ReportData data;
  data.precheck = record;
  check(sg::detail::writeReport(
            "harness_noise_report_bytes.json", data, {}, std::nullopt, record),
        "the writer wrote the document (FR-015)");
  const std::string bytes =
      maskSpeedgun(sg::test::readText("harness_noise_report_bytes.json"));
  check(bytes == kDocumentFixture,
        "a record without the tracer member writes a document whose bytes "
        "fold equal to the fixture bytes (FR-015, R-10)");
}

// T016: a not-available record writes no tracer key and prints the one
// disclosure line (FR-015, R-10).
auto theNotAvailableRecordWritesNoTracerKey() -> void
{
  auto record = standingRecord();
  sg::ReportData data;
  data.precheck = record;
  check(sg::detail::writeReport(
            "harness_noise_report_na.json", data, {}, std::nullopt, record),
        "the writer wrote the document (FR-015)");
  const std::string document =
      sg::test::readText("harness_noise_report_na.json");
  check(document.find("\"tracer\"") == std::string::npos,
        "a not-available record writes no tracer key (FR-015, R-10)");
  const auto block = capturePrecheck(record, sg::TracerEngine::OSNOISE);
  check(block.find(kNotAvailableLine) != std::string::npos,
        "the not-available run prints the one disclosure line (FR-016)");
}

}  // namespace

// The reader's error arms of the optional tracer record: a document
// whose tracer object stands malformed in one key or kind each names
// the fault at its own pointer, the way the standing record readers
// name theirs (FR-014, specs/036 R-08).
auto theTracerReaderErrorArms() -> void
{
  const std::string head =
      R"({"schemaVersion":"1.7","benchmarks":[],"precheck":{"serial":false,)" R"("windowMs":1,"cpus":[],"hostSettings":{"isolcpus":null,)" R"("nohz_full":null,"rcu_nocbs":null,"governors":{},"irqAffinity":{}},)" R"("skippedGroups":[],"tracer":)";
  const std::string tail = "}}";
  const std::pair<const char*, const char*> variants[] {
      {R"({"cpus":[]})", "the key 'engine' stands missing"},
      {R"({"engine":5,"cpus":[]})", "expected a string"},
      {R"({"engine":"hrtimers","cpus":[]})", "stands outside the names"},
      {R"({"engine":"osnoise"})", "the key 'cpus' stands missing"},
      {R"({"engine":"osnoise","cpus":{}})", "expected an array"},
      {R"({"engine":"osnoise","cpus":[5]})", "INCORRECT_TYPE"},
      {R"({"engine":"osnoise","cpus":[{"nmi":1}]})",
       "the key 'cpu' stands missing"},
      {R"({"engine":"osnoise","cpus":[{"cpu":"2"}]})", "expected an integer"},
      {R"({"engine":"osnoise","cpus":[{"cpu":2}]})",
       "the key 'nmi' stands missing"},
      {R"({"engine":"osnoise","cpus":[{"cpu":2,"nmi":"x","irq":1,"softirq":1,"thread":1,"hardware":1}]})",
       "expected an integer"},
      {R"({"engine":"osnoise","cpus":[{"cpu":2,"nmi":1,"irq":2,"softirq":3,"thread":4}]})",
       "the key 'hardware' stands missing"},
      {R"([])", "INCORRECT_TYPE"},
  };
  for (const auto& [tracer, fragment] : variants) {
    const std::string document = head + tracer + tail;
    const std::string path = "harness_noise_reader_arm.json";
    {
      std::ofstream file(path);
      file << document;
    }
    const auto loaded = sg::loadReport(path);
    check(
        !loaded.has_value(),
        (std::string("the malformed tracer record stands refused " "(FR-014): ")
         + fragment)
            .c_str());
    if (!loaded.has_value()) {
      check(loaded.error().find("/precheck/tracer") != std::string::npos
                && loaded.error().find(fragment) != std::string::npos,
            ("the refusal names the tracer pointer and the fault (FR-014): "
             + loaded.error())
                .c_str());
    }
    std::remove(path.c_str());
  }
}

auto main() -> int
{
  theTracerLinesStandBesideTheStandingBlock();
  theNotAvailableRecordPrintsOneDisclosureLine();
  aTracerFreeRunPrintsNoTracerLine();
  theTracerRecordRoundTrips();
  aDocumentWrittenBeforeTheKeyReadsTracerLess();
  theSchemaCarriesTheTracerKey();
  theWrittenTracerDocumentPassesTheSchema();
  theTracerLessDocumentMatchesTheFixtureBytes();
  theNotAvailableRecordWritesNoTracerKey();
  theTracerReaderErrorArms();
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
