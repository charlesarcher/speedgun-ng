// ============================================================================
// The multi-pass disclosure of the console report
// (specs/024-multipass-collection T039 to T045; US3; FR-017 to FR-021;
// SC-05, SC-06; research D-06, D-08, D-10, D-14).
//
// Two internal seams, the pair the report tests already use:
// `sg::detail::Runner` runs the scripted pass fixture of
// `detail/pass_fixture.hpp`, and `sg::detail::printResult` prints the result
// that run returned, which is the byte-for-byte console a caller sees (the
// capture pattern of `harness_context_report_test.cpp`). Every claim reads
// those printed bytes, a field of the result value, or both.
//
// The spread figures come from the fixture table of `data-model.md` E-06:
// the times stand in this file, the decimals they owe stand beside them, and
// the arithmetic in between is `sg::detail::passSpread`, the function that
// computes it. No expected percentage is written twice here.
//
// One scenario shares nothing with the others and one host figure escapes:
// the `overhead floor (ns)=` field of a real run is the plan's wall-clock
// calibration (`Plan::sampleOverheadNsMedian`), which no second run
// reproduces. `maskHostFloor` masks that one span the way the byte-identity
// fixture of T042 masks it, and the digits it hides are checked to be a
// number, so the mask cannot succeed over a missing field. Every other byte
// of every line is compared whole. No scenario sleeps, reads a clock or
// needs a PMU (FR-033).
//
// The scenarios share one process, because they share one provider
// registration: `registerPassFixture` publishes the paths the counters system
// takes once per process, and every scenario here wants the same scripted
// limit of 4. `harness_pass_collect_test.cpp` re-executes per scenario
// because each of its scenarios scripts a provider of its own.
//
// `argv[1]` selects one scenario; with no argument the list at the bottom
// walks every one of them. A scenario absent from that list never runs.
// ============================================================================

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/pass_fixture.hpp"
#include "detail/pass_plan.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The file one scenario's report is captured to, the scratch file of
// `harness_context_report_test.cpp`.
constexpr const char* kScratchPath = "harness_pass_report_out.txt";

// One timed window of one iteration: the shape every scenario runs, so a
// window holds its two endpoint samples and the row divides by one.
constexpr std::uint64_t kRunIterations = 1;
constexpr std::uint64_t kRunRepetitions = 1;

// The endpoint samples of a window that folds nothing, and the figures the
// fixture's leaves owe a run of one iteration: the clock leaf steps
// `kPassTimeStep`, and the column of index k sums two leaves that step
// `(k + 1) * kPassEventStep` (`harness_pass_collect_test.cpp` states both).
constexpr std::uint64_t kWindowSamples = sg::test::kAuditPointReadsPerIteration;
constexpr double kPassTimePerIterationNs =
    static_cast<double>(sg::test::kPassTimeStep);
constexpr double kFirstColumnValue =
    static_cast<double>(sg::test::kPassEventStep) * 2.0;
constexpr double kColumnValueStep =
    static_cast<double>(sg::test::kPassEventStep) * 2.0;
constexpr double kNoFloor = 0.0;

// The field widths the report prints: the name field of the rows, and the
// width a disclosure tag or an iteration count stands in right-aligned.
constexpr std::size_t kNameFieldWidth = 24;
constexpr std::size_t kCountFieldWidth = 10;

// Every column of every fixture row folds a leaf set that declares no ratio
// pair, so the kernel-multiplexing disclosure beside it prints its full ratio
// (FR-020).
constexpr const char* kFullRatioField = "ratio=1.000";

// The warning limit of FR-018, read on the unrounded spread (I-3).
constexpr double kSpreadWarningLimitPercent = 5.0;

// The span `maskHostFloor` writes in the host-calibrated field, and the two
// marks that bound it in the order the row prints them.
constexpr const char* kMaskedFloor = "overhead floor (ns)= <floor>  ";
constexpr std::string_view kFloorMark = "overhead floor (ns)=";
constexpr std::string_view kSamplesMark = "endpoint samples=";

// The label of the one metric column a built result carries.
constexpr const char* kBuiltColumnLabel = "pair_a";

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS PASS REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto closeTo(const double value, const double expected) -> bool
{
  return std::abs(value - expected) <= 0.001 * (std::abs(expected) + 1.0);
}

// One number of a printed row, in the width and precision the report prints
// it in. The conversion is `std::snprintf` with the report's own specifier,
// so an expectation and the printed row round the same figure the same way.
// The bound of the buffer is checked: a truncated expectation would compare
// against a full line and fail for the wrong reason.
auto fixedField(const double value) -> std::string
{
  std::array<char, 32> buffer {};
  const int written =
      std::snprintf(buffer.data(), buffer.size(), "%9.3f", value);
  check(written > 0 && static_cast<std::size_t>(written) < buffer.size(),
        "the printed figure fits its buffer");
  return std::string(buffer.data(), static_cast<std::size_t>(written));
}

// One number of a disclosure field, to the one decimal the report prints.
auto decimalField(const double value) -> std::string
{
  std::array<char, 32> buffer {};
  const int written =
      std::snprintf(buffer.data(), buffer.size(), "%.1f", value);
  check(written > 0 && static_cast<std::size_t>(written) < buffer.size(),
        "the printed decimal fits its buffer");
  return std::string(buffer.data(), static_cast<std::size_t>(written));
}

// The name field of the rows, and the right-aligned field of `width` a count
// or a tag stands in (the widths `printResult` and
// `contracts/pass-disclosure.md` state).
auto nameField(const std::string& name) -> std::string
{
  return name.size() >= kNameFieldWidth
      ? name
      : name + std::string(kNameFieldWidth - name.size(), ' ');
}

auto rightAligned(const std::string& text,
                  const std::size_t width) -> std::string
{
  return text.size() >= width ? text
                              : std::string(width - text.size(), ' ') + text;
}

// ---------------------------------------------------------------------------
// The capture seam
// ---------------------------------------------------------------------------

// `call` prints to the process stdout, so the capture redirects that stream
// to a scratch file and restores it: the verdict lines of this file stay on
// the real stdout. The pattern of `captureThrough` in
// `harness_context_report_test.cpp`.
template<class Call>
auto captureThrough(const std::string& path, Call&& call) -> void
{
  std::FILE* saved = nullptr;
  const int savedFd = dup(STDOUT_FILENO);
  if (savedFd >= 0) {
    saved = fdopen(savedFd, "w");
  }
  check(saved != nullptr, "the capture kept the real stdout");

  std::FILE* replacement = std::fopen(path.c_str(), "w");
  check(replacement != nullptr, "the capture opened its scratch file");
  const int replacementFd = fileno(replacement);
  check(replacementFd >= 0, "the capture file carries a descriptor");
  const bool redirected = dup2(replacementFd, STDOUT_FILENO) == STDOUT_FILENO;
  check(redirected, "the capture redirected stdout");
  if (std::setvbuf(stdout, nullptr, _IONBF, 0) != 0) {
    fail("the capture could not unbuffer the redirected stdout");
  }
  std::fflush(stdout);

  call();

  std::fflush(stdout);
  const bool restored = dup2(savedFd, STDOUT_FILENO) == STDOUT_FILENO;
  check(restored, "the capture restored stdout");
  if (std::setvbuf(stdout, nullptr, _IOLBF, BUFSIZ) != 0) {
    fail("the restored stdout could not set its buffering mode");
  }
  std::fclose(saved);
  std::fclose(replacement);
}

auto readAll(const std::string& path) -> std::string
{
  std::ifstream stream(path);
  check(static_cast<bool>(stream), "the captured file opens for reading");
  return std::string((std::istreambuf_iterator<char>(stream)),
                     std::istreambuf_iterator<char>());
}

auto splitLines(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::string current;
  for (const char symbol : text) {
    if (symbol == '\n') {
      lines.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(symbol);
  }
  check(current.empty(), "every captured line ends with its newline");
  return lines;
}

auto linesHolding(const std::vector<std::string>& lines,
                  const std::string& needle) -> std::vector<std::string>
{
  std::vector<std::string> held;
  for (const auto& line : lines) {
    if (line.find(needle) != std::string::npos) {
      held.push_back(line);
    }
  }
  return held;
}

// ---------------------------------------------------------------------------
// The bytes the report owes
// ---------------------------------------------------------------------------

// The row of a real run with the host-calibrated floor figure written as the
// token the expectation holds. The span is bounded by the two fields the row
// prints around it, so a moved or missing field fails here rather than
// masking its way past the comparison.
auto maskHostFloor(const std::string& line) -> std::string
{
  const auto floorAt = line.find(kFloorMark);
  const auto samplesAt = line.find(kSamplesMark);
  check(floorAt != std::string::npos,
        "the row carries the overhead floor field");
  check(samplesAt != std::string::npos && floorAt < samplesAt,
        "the row carries the endpoint samples field after the floor field");
  const std::string hidden = line.substr(
      floorAt + kFloorMark.size(), samplesAt - floorAt - kFloorMark.size());
  check(hidden.find('.') != std::string::npos,
        "the masked span holds the printed floor figure");
  return std::string(line).substr(0, floorAt) + kMaskedFloor
      + line.substr(samplesAt);
}

// One metric column of an expected row: the label, the folded value, and the
// index of the pass that measured it. The index prints only when the run
// measured more than one pass, so a column of a one-pass row carries nothing
// (FR-019, FR-021).
struct ExpectedMetric
{
  std::string label;
  double value = 0.0;
  std::optional<std::size_t> pass;
};

// The repetition row: the fixed time columns, the floor field as the caller
// names it (the masked token for a real run, the scripted zero for a built
// result), the sample count, then the metric fields in the spelling the
// report carries today, `label= value= ratio= scaled= gap=`, with the
// `pass=` tag inside that same field group (FR-019, FR-020, FR-021).
auto expectedRow(const std::string& name,
                 const double timePerIterationNs,
                 const std::string& floorField,
                 const std::vector<ExpectedMetric>& metrics) -> std::string
{
  auto line = nameField(name) + " iterations="
      + rightAligned(std::to_string(kRunIterations), kCountFieldWidth)
      + "  real time/iter (ns)=" + fixedField(timePerIterationNs)
      + "  cpu time/iter (ns)=" + fixedField(timePerIterationNs) + "  "
      + floorField + "endpoint samples=" + std::to_string(kWindowSamples);
  for (const auto& metric : metrics) {
    line += "  " + metric.label + "=" + fixedField(metric.value) + " "
        + kFullRatioField + " scaled=0 gap=0";
    if (metric.pass.has_value()) {
      line += " pass=" + std::to_string(metric.pass.value());
    }
  }
  return line;
}

// The disclosure line of `contracts/pass-disclosure.md`: the name field of
// the rows, the `passes` tag right-aligned in the width the repetition row
// prints its iteration count in, then the method, the count and the spread
// field (FR-017, FR-020).
auto expectedPassLine(const std::string& name,
                      const std::string& word,
                      const std::size_t passCount,
                      const std::string& spreadField) -> std::string
{
  return nameField(name) + " " + rightAligned("passes", kCountFieldWidth)
      + "  method=" + word + "  count=" + std::to_string(passCount)
      + spreadField;
}

auto expectedMultiplexPassLine(const std::string& name,
                               const std::size_t passCount) -> std::string
{
  return expectedPassLine(name, "multiplex", passCount, "");
}

// The `spread=` field as the report prints it: one decimal and a percent
// sign, or the words that stand where no answer exists (FR-017).
auto expectedSpreadField(const std::optional<double>& spread) -> std::string
{
  if (!spread.has_value()) {
    return "  spread=not available";
  }
  return "  spread=" + decimalField(spread.value()) + "%";
}

// The warning line, in the voice of `printNoiseWarning`: one line, no name
// field, the same one decimal as the field above it (FR-018).
auto expectedWarningLine(const double spread) -> std::string
{
  check(spread > kSpreadWarningLimitPercent,
        "only a spread above the limit warns");
  return "warning: pass spread " + decimalField(spread) + "% exceeds 5%";
}

auto methodWord(const sg::MetricMethod method) -> std::string
{
  switch (method) {
    case sg::MetricMethod::AUTO:
      return "auto";
    case sg::MetricMethod::PASSES:
      return "passes";
    case sg::MetricMethod::MULTIPLEX:
      return "multiplex";
  }
  return "auto";
}

// ---------------------------------------------------------------------------
// The scripted runs
// ---------------------------------------------------------------------------

auto sumLeaves(const std::string_view objectPath,
               const std::vector<std::string_view>& names)
    -> sg::counters::Expression<sg::counters::Dim<0, 1>>
{
  const auto object = sg::counters::System::local().object(objectPath);
  check(object.has_value(), "the object of the metric column resolves");
  const auto first = object->counter<sg::counters::Dim<0, 1>>(names.front());
  check(first.has_value(), "the first leaf of the column resolves");
  auto total = sg::counters::Expression<sg::counters::Dim<0, 1>>(*first);
  for (std::size_t index = 1; index < names.size(); ++index) {
    const auto leaf = object->counter<sg::counters::Dim<0, 1>>(names[index]);
    check(leaf.has_value(), "every leaf of the column resolves");
    total = total + *leaf;
  }
  return total;
}

auto attachColumns(sg::BenchmarkHandle& handle,
                   const std::vector<std::size_t>& indexes) -> void
{
  const auto& columns = sg::test::passColumns();
  for (const std::size_t index : indexes) {
    handle.addMetric(sumLeaves(columns[index].object, columns[index].events),
                     columns[index].label);
  }
}

auto bmPassBody(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

auto needInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  check(false, "the expansion produced the named instance");
  return nullptr;
}

// The fixture at the scripted limit of 4 (SC-08, `kHostPassLimit`), and the
// families every scenario runs. The four pairs of two events over a limit of
// 4 split into two passes; the two pairs fit in one pass; the column-free
// family carries no metric column for the planner to place.
auto prepare() -> void
{
  check(sg::test::registerPassFixture(sg::test::kHostPassLimit),
        "the pass fixture registers on the local system");

  auto wide = sg::registerBenchmark(&bmPassBody, "bmPassShape");
  attachColumns(wide, {0, 1, 2, 3});

  auto multiplex = sg::registerBenchmark(&bmPassBody, "bmPassMultiplex");
  attachColumns(multiplex, {0, 1, 2, 3});
  multiplex.metricMethod(sg::MetricMethod::MULTIPLEX);

  auto forced = sg::registerBenchmark(&bmPassBody, "bmPassForcedFit");
  attachColumns(forced, {0, 1});
  forced.metricMethod(sg::MetricMethod::PASSES);

  auto bare = sg::registerBenchmark(&bmPassBody, "bmPassNoMetric");
  bare.metricMethod(sg::MetricMethod::PASSES);

  // The mirror of the second pass of `bmPassShape`: the same body with the
  // two columns that pass measures, so the run reports that pass's own
  // reference time (the mirror technique of T022).
  auto mirror = sg::registerBenchmark(&bmPassBody, "bmPassSecondMirror");
  attachColumns(mirror, {2, 3});

  sg::detail::expandRegistry();
}

auto runInstance(const std::string& name) -> sg::BenchmarkResult
{
  sg::detail::RunOptions options;
  options.fixedIterations = kRunIterations;
  options.repetitions = kRunRepetitions;
  sg::detail::Runner runner(options);
  auto result = runner.run(*needInstance(name));
  check(result.outcome == sg::RunOutcome::MEASURED,
        "the scripted run measured its row");
  check(!result.rows.empty(), "the measured result carries a row");
  return result;
}

auto printedLines(const sg::BenchmarkResult& result) -> std::vector<std::string>
{
  captureThrough(kScratchPath, [&result] { sg::detail::printResult(result); });
  return splitLines(readAll(kScratchPath));
}

// The values of the fixture columns a family carries, in request order, each
// carrying the `pass=` tag that `tags` names for it and nothing where the run
// measured one pass. The tags run one per attached column, so their count is
// the count of the row's metric fields (FR-001, FR-019).
auto shapeMetrics(const std::vector<std::optional<std::size_t>>& tags)
    -> std::vector<ExpectedMetric>
{
  std::vector<ExpectedMetric> metrics;
  const auto& columns = sg::test::passColumns();
  for (std::size_t index = 0; index < tags.size(); ++index) {
    metrics.push_back(ExpectedMetric {
        std::string(columns[index].label),
        kFirstColumnValue + static_cast<double>(index) * kColumnValueStep,
        tags[index]});
  }
  return metrics;
}

// The one disclosure line of a captured report; empty when the run printed
// none.
auto disclosureLineOf(const std::vector<std::string>& lines) -> std::string
{
  const auto found = linesHolding(lines, "  method=");
  check(found.size() <= 1,
        "the report prints at most one disclosure line (FR-017)");
  return found.empty() ? std::string() : found.front();
}

// The spread a printed line names: nothing when it carries no `spread=`
// field, or names the words that stand where no answer exists.
auto printedSpreadOf(const std::string& line) -> std::optional<double>
{
  const std::string key = "spread=";
  const auto at = line.find(key);
  if (at == std::string::npos) {
    return std::nullopt;
  }
  const auto valueAt = at + key.size();
  const auto after = line.find("  ", valueAt);
  const std::string field = line.substr(
      valueAt,
      after == std::string::npos ? std::string::npos : after - valueAt);
  if (field == "not available") {
    return std::nullopt;
  }
  return std::strtod(field.c_str(), nullptr);
}

// ---------------------------------------------------------------------------
// T039: the two-pass disclosure shape
// ---------------------------------------------------------------------------

// US3 scenario 1, FR-017, FR-019. The run is the fixture's four columns at
// the scripted limit of 4: the planner gives columns 0 and 1 to pass 0 and
// columns 2 and 3 to pass 1, so the row tags each column with the pass that
// measured it and one line below the row names the run.
auto scenarioTwoPassShape() -> void
{
  const auto result = runInstance("bmPassShape");
  const auto lines = printedLines(result);

  check(lines.size() == 2,
        "TODO-FAIL: the report prints the row and one line below it, and "
        "nothing else (FR-017, T049)");
  check(maskHostFloor(lines[0])
            == expectedRow(result.name,
                           kPassTimePerIterationNs,
                           kMaskedFloor,
                           shapeMetrics({std::size_t(0),
                                         std::size_t(0),
                                         std::size_t(1),
                                         std::size_t(1)})),
        "TODO-FAIL: the row tags every metric column with pass=0 or pass=1 in "
        "the existing metric field spelling (FR-019, T050)");

  const auto mirror = runInstance("bmPassSecondMirror");
  const std::vector<double> passTimes {result.rows.front().timePerIterationNs,
                                       mirror.rows.front().timePerIterationNs};
  const auto spread = sg::detail::passSpread(passTimes);
  check(lines[1] == expectedPassLine(result.name,
                                     "passes",
                                     passTimes.size(),
                                     expectedSpreadField(spread)),
        "TODO-FAIL: one line names method=passes, the count of two and the "
        "spread across the passes (FR-017, T049)");
  check(linesHolding(lines, "warning:").empty(),
        "a spread within the limit prints no warning line (FR-018)");

  // The median of an even count is the lower of its two central times (D-6):
  // 100 and 108 spread against 100, so the two-pass figure is 8.0, where the
  // upper of the two central values would give 7.4.
  const auto even = sg::detail::passSpread({100.0, 108.0});
  check(even.has_value() && closeTo(even.value(), 8.0),
        "TODO-FAIL: the median of an even pass count is the lower of its two "
        "central times (D-6, T048)");
}

// ---------------------------------------------------------------------------
// T040, T041: the spread fixture table of data-model E-06
// ---------------------------------------------------------------------------

// One result of the fixture table: `passTimes` are the reference times of its
// passes, the method is the one the runs that measured more than one pass
// carry, and the spread is what the function that computes it answers for
// those times (E-06, E-07). The column the row carries stands at ratio 1.0
// with no scaling and no gap, so the fields beside `pass=` hold their full
// shape.
auto builtSpreadResult(const std::string& name,
                       const std::vector<double>& passTimes)
    -> sg::BenchmarkResult
{
  sg::BenchmarkResult result;
  result.name = name;
  result.outcome = sg::RunOutcome::MEASURED;
  result.metricLabels.push_back(kBuiltColumnLabel);
  result.metricMethod = sg::MetricMethod::PASSES;
  result.passCount = passTimes.size();
  result.passSpread = sg::detail::passSpread(passTimes);

  sg::ResultRow row;
  row.iterations = kRunIterations;
  row.timePerIterationNs = passTimes.front();
  row.cpuTimePerIterationNs = passTimes.front();
  row.endpointSamples = kWindowSamples;
  sg::MetricValue column;
  column.value = kFirstColumnValue;
  column.pass = 0;
  row.metrics.push_back(column);
  result.rows.push_back(row);
  return result;
}

// The row, the disclosure line and the warning line one row of the E-06 table
// owes. `expectedField` is the table's `Required_printed_decimal` in the
// field's own spelling, so the fixture states its own answer and the spread
// that produces it stands unrounded in the result.
auto checkSpreadFixture(const std::vector<double>& passTimes,
                        const std::string& expectedField,
                        const bool expectWarning) -> sg::BenchmarkResult
{
  const std::string name = "bmSpreadFixture";
  const auto result = builtSpreadResult(name, passTimes);
  const auto lines = printedLines(result);
  const auto expectedCount = expectWarning ? 3u : 2u;

  check(lines.size() == expectedCount,
        "TODO-FAIL: the report prints the row, the disclosure line and the "
        "warning line it owes, and nothing else (FR-017, FR-018, T049)");
  check(lines.front()
            == expectedRow(name,
                           passTimes.front(),
                           "overhead floor (ns)=" + fixedField(kNoFloor) + "  ",
                           {ExpectedMetric {
                               kBuiltColumnLabel, kFirstColumnValue, 0}}),
        "the row carries the built column with its measuring pass inside the "
        "existing metric field spelling (FR-019)");
  check(lines[1] == expectedPassLine(name,
                                     "passes",
                                     passTimes.size(),
                                     expectedField),
        "TODO-FAIL: the disclosure line prints the spread of the fixture to "
        "one decimal, the print being the only rounding (FR-017, D-08, T049)");
  if (!expectWarning) {
    return result;
  }
  check(lines[2] == expectedWarningLine(result.passSpread.value()),
        "TODO-FAIL: the warning line stands directly after the disclosure "
        "line and names the same figure (FR-018, T049)");
  return result;
}

// T040, E-06 IN_BAND.
auto scenarioSpreadInBand() -> void
{
  const auto result =
      checkSpreadFixture({100.0, 102.0, 104.0}, "  spread=3.9%", false);
  check(result.passSpread.has_value()
            && result.passSpread.value() < kSpreadWarningLimitPercent,
        "the in-band spread stands below the limit the warning reads, which is "
        "why its report holds no warning line (FR-018, I-3)");
}

// T040, E-06 BOUNDARY: the printed figure is 5.0 and no warning prints, so
// the predicate reads the unrounded value and is strict.
auto scenarioSpreadBoundary() -> void
{
  const auto result =
      checkSpreadFixture({100.0, 100.0, 105.0}, "  spread=5.0%", false);
  check(result.passSpread.has_value()
            && closeTo(result.passSpread.value(),
                       kSpreadWarningLimitPercent)
            && result.passSpread.value() <= kSpreadWarningLimitPercent,
        "the boundary spread is the exact 5.0 the print rounds to 5.0, and the "
        "warning reads the unrounded figure (I-3, FR-018)");
}

// T040 and T041, E-06 WARNING: 100, 104 and 108 print 7.7 and warn.
auto scenarioSpreadWarning() -> void
{
  const auto result =
      checkSpreadFixture({100.0, 104.0, 108.0}, "  spread=7.7%", true);
  check(result.passSpread.has_value()
            && result.passSpread.value() > kSpreadWarningLimitPercent
            && result.passSpread.value() < 7.7,
        "the warning reads the unrounded 7.69 that the print rounds up to 7.7 "
        "(FR-018, D-08)");
}

// T041, E-06 ZERO_MEDIAN: the formula answers nothing, so the field names
// that, no warning prints and the run still reports its row.
auto scenarioSpreadZeroMedian() -> void
{
  const auto result =
      checkSpreadFixture({0.0, 0.0, 0.0}, "  spread=not available", false);
  check(!result.passSpread.has_value(),
        "a zero median answers nothing (E-06 ZERO_MEDIAN, FR-017)");
  check(result.outcome == sg::RunOutcome::MEASURED,
        "the zero median discloses and fails nothing (E-06, FR-017)");
}

// T041, E-06 TWO_PASS: the lower-of-two median of D-6 is what gives the
// two-pass fixture its 8.0, and 8.0 is above the limit, so it warns.
auto scenarioSpreadTwoPass() -> void
{
  const auto result = checkSpreadFixture({100.0, 108.0}, "  spread=8.0%", true);
  check(result.passSpread.has_value() && closeTo(result.passSpread.value(), 8.0),
        "the two-pass spread is 8.0 against the lower of its two central "
        "times, where the upper would print 7.4 (D-6, E-06 TWO_PASS)");
}

// ---------------------------------------------------------------------------
// T043: the `multiplex` settling shape
// ---------------------------------------------------------------------------

// US3 scenario 4, FR-020. The same over-limit set, with the method forced to
// `multiplex`: one plan holds all eight events, the kernel-multiplexing
// disclosure keeps its spelling, and the run measured one pass, so no column
// carries a pass tag and no spread prints.
auto scenarioMultiplexShape() -> void
{
  const auto result = runInstance("bmPassMultiplex");
  const auto lines = printedLines(result);

  check(lines.size() == 2,
        "TODO-FAIL: the row and one disclosure line, and nothing else "
        "(FR-020, T049)");
  check(maskHostFloor(lines[0])
            == expectedRow(result.name,
                           kPassTimePerIterationNs,
                           kMaskedFloor,
                           shapeMetrics({std::nullopt,
                                         std::nullopt,
                                         std::nullopt,
                                         std::nullopt})),
        "ratio=, scaled= and gap= keep their current spelling, and one pass "
        "prints no pass tag (FR-020, FR-019)");
  check(lines[1] == expectedMultiplexPassLine(result.name, 1),
        "TODO-FAIL: the disclosure names method=multiplex with count=1 and "
        "carries no spread field (FR-020, T049)");
}

// ---------------------------------------------------------------------------
// T044: the print rule
// ---------------------------------------------------------------------------

// FR-021, clarification Q2: the caller's own choice prints even when the run
// measured one pass, because that pass is the run's whole answer.
auto scenarioForcedMethodPrintsOnePass() -> void
{
  const auto result = runInstance("bmPassForcedFit");
  const auto lines = printedLines(result);

  check(lines.size() == 2,
        "TODO-FAIL: a forced method with one pass prints its line (FR-021, "
        "Q2, T049)");
  check(maskHostFloor(lines[0])
            == expectedRow(result.name,
                           kPassTimePerIterationNs,
                           kMaskedFloor,
                           shapeMetrics(
                               {std::nullopt, std::nullopt})),
        "the row of the one-pass run keeps the audit point's field list, with "
        "no pass tag (FR-019, FR-021)");
  check(lines[1] == expectedPassLine(result.name, "passes", 1, ""),
        "TODO-FAIL: the line names method=passes with count=1 and no spread, "
        "because one pass has nothing to spread (FR-017, FR-021, T049)");
}

// FR-021, clarification Q2: the pass machinery discloses only when the run
// carries at least one metric column, forced or not.
auto scenarioNoMetricColumnPrintsNothing() -> void
{
  const auto result = runInstance("bmPassNoMetric");
  const auto lines = printedLines(result);

  check(lines.size() == 1,
        "a run with no metric column prints no disclosure line even under a "
        "forced method (FR-021, Q2)");
  check(disclosureLineOf(lines).empty(),
        "no line names a method on a run that measured no metric column "
        "(FR-021, Q2)");
  check(maskHostFloor(lines.front())
            == expectedRow(result.name,
                           kPassTimePerIterationNs,
                           kMaskedFloor,
                           {}),
        "the row of the column-free run is the bytes the audit point prints "
        "for it (FR-021)");
}

// ---------------------------------------------------------------------------
// T045: the published fields are the values the console printed
// ---------------------------------------------------------------------------

// SC-06, clarification Q4, E-07: the settled method, the pass count and the
// spread stand on the merged result beside the bytes the report printed from
// them. The console is the printed rounding; the field is the unrounded
// figure, so the two agree within that one decimal.
auto checkFieldsMatchConsole(const char* what,
                             const sg::BenchmarkResult& result,
                             const std::vector<std::string>& lines,
                             const std::string& expectedWord,
                             const std::size_t expectedCount) -> void
{
  const auto line = disclosureLineOf(lines);
  check(!line.empty(), what);
  check(line == expectedPassLine(result.name,
                                 methodWord(result.metricMethod),
                                 result.passCount,
                                 result.passCount > 1
                                     ? expectedSpreadField(result.passSpread)
                                     : std::string()),
        "the console's method, count and spread are the result's published "
        "fields, printed (FR-017, SC-06, Q4)");
  check(result.metricMethod != sg::MetricMethod::AUTO,
        "a run that printed a line settled to a concrete method (E-01)");
  const auto printed = printedSpreadOf(line);
  check(printed.has_value() == result.passSpread.has_value(),
        "the console and the published field answer the spread the same way "
        "(E-07, SC-06)");
  if (printed.has_value()) {
    check(std::abs(printed.value() - result.passSpread.value()) <= 0.05,
          "the printed decimal is the one rounding of the published unrounded "
          "spread (D-08, E-07)");
  }
  check(expectedWord == methodWord(result.metricMethod)
            && expectedCount == result.passCount,
        "the fixture's expectation is the value the result publishes (E-07)");
}

auto scenarioPublishedFields() -> void
{
  const auto wide = runInstance("bmPassShape");
  checkFieldsMatchConsole(
      "TODO-FAIL: a two-pass run prints its disclosure to compare against "
      "(FR-017, T049)",
      wide,
      printedLines(wide),
      "passes",
      2);

  const auto multiplex = runInstance("bmPassMultiplex");
  checkFieldsMatchConsole(
      "TODO-FAIL: the multiplex settling prints its disclosure to compare "
      "against (FR-020, T037, T049)",
      multiplex,
      printedLines(multiplex),
      "multiplex",
      1);

  const auto bare = runInstance("bmPassNoMetric");
  const auto bareLines = printedLines(bare);
  check(disclosureLineOf(bareLines).empty(),
        "the column-free run prints no line, so there is nothing to compare "
        "against (FR-021, Q2)");
  check(bare.metricMethod == sg::MetricMethod::AUTO,
        "TODO-FAIL: with no metric column the mechanism does not run, so the "
        "method published is AUTO (E-01, E-07, T047)");
  check(bare.passCount == 0,
        "TODO-FAIL: a run that measured no pass publishes a pass count of "
        "zero (E-07, T047)");
  check(!bare.passSpread.has_value(),
        "TODO-FAIL: a run that measured no pass publishes no spread (E-07, "
        "T047)");
}

// ---------------------------------------------------------------------------
// The scenario list CTest walks
// ---------------------------------------------------------------------------

struct Scenario
{
  const char* name;
  void (*body)();
};

constexpr std::array<Scenario, 10> kScenarios {{
    Scenario {"two-pass-shape", &scenarioTwoPassShape},
    Scenario {"spread-in-band", &scenarioSpreadInBand},
    Scenario {"spread-boundary", &scenarioSpreadBoundary},
    Scenario {"spread-warning", &scenarioSpreadWarning},
    Scenario {"spread-zero-median", &scenarioSpreadZeroMedian},
    Scenario {"spread-two-pass", &scenarioSpreadTwoPass},
    Scenario {"multiplex-shape", &scenarioMultiplexShape},
    Scenario {"forced-one-pass", &scenarioForcedMethodPrintsOnePass},
    Scenario {"no-metric-column", &scenarioNoMetricColumnPrintsNothing},
    Scenario {"published-fields", &scenarioPublishedFields},
}};

// One scenario of the no-argument walk in a child of its own, so that a red
// scenario is one line in the log and the rest of the list still runs. The
// pass fixture's provider is a process-wide registration `prepare()` makes
// before the fork, so every child inherits the same scripted facts and shares
// the parent's state without sharing its failure.
auto runIsolated(const Scenario& scenario) -> bool
{
  std::fflush(stdout);
  std::fflush(stderr);
  const pid_t child = fork();
  if (child < 0) {
    fail("the scenario list could not fork");
  }
  if (child == 0) {
    scenario.body();
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(0);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    fail("waiting for the scenario failed");
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  prepare();

  const std::string_view requested =
      argc > 1 && argv[1] != nullptr ? argv[1] : std::string_view();
  if (!requested.empty()) {
    for (const auto& scenario : kScenarios) {
      if (requested == scenario.name) {
        scenario.body();
        std::printf("%s: ok\n", scenario.name);
        return 0;
      }
    }
    std::fprintf(stderr,
                 "harness_pass_report_test: unknown scenario '%.*s'\n",
                 static_cast<int>(requested.size()),
                 requested.data());
    return 2;
  }

  // What ctest runs: the whole list, each scenario isolated, one verdict line
  // per scenario. Before the report work of feature 024 lands, the TODO-FAIL
  // markers are the expected verdicts, and naming them one by one is what tells
  // the implementer which of T047 to T050 is still missing.
  std::size_t failures = 0;
  for (const auto& scenario : kScenarios) {
    const bool passed = runIsolated(scenario);
    std::printf("harness_pass_report_test %s: %s\n",
                scenario.name,
                passed ? "pass" : "FAIL");
    if (!passed) {
      ++failures;
    }
  }
  std::printf("harness_pass_report_test: %zu scenarios, %zu fail\n",
              kScenarios.size(),
              failures);
  return failures == 0 ? 0 : 1;
}
