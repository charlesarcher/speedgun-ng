#include <algorithm>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "speedgun-ng/views.hpp"

#include "escape.hpp"

namespace
{

auto fail(char const* what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(bool cond, char const* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The house double comparison: -Werror=float-equal rules out `==` on a
// double (the convention counters_core_test and the counters tests run).
auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

// The text of one file, read whole, the way harness_cli_table_test
// reads the sources it checks.
auto readText(std::filesystem::path const& path) -> std::string
{
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

// The root CMakeLists.txt: the walk starts at the working directory
// and climbs to the directory whose CMakeLists.txt carries the
// project( block, the shape harness_cli_table_test's projectVersion()
// runs (specs/038 FR-002, D-2).
auto rootCMakeText() -> std::string
{
  auto directory = std::filesystem::absolute(std::filesystem::current_path());
  for (int hop = 0; hop <= 8 && !directory.empty(); ++hop) {
    auto const text = readText(directory / "CMakeLists.txt");
    if (text.find("project(") != std::string::npos) {
      return text;
    }
    directory = directory.parent_path();
  }
  return {};
}

// The version of the VERSION line, read the way
// harness_cli_table_test's projectVersion() reads it.
auto projectVersion(std::string const& cmake) -> std::string
{
  std::istringstream lines(cmake);
  std::string line;
  while (std::getline(lines, line)) {
    auto const word = line.find_first_not_of(" \t");
    if (word == std::string::npos || line.compare(word, 8, "VERSION ") != 0) {
      continue;
    }
    std::istringstream value(line.substr(word + 8));
    std::string version;
    if ((value >> version) && !version.empty()
        && std::isdigit(static_cast<unsigned char>(version.front())) != 0)
    {
      return version;
    }
  }
  return {};
}

// The fixture directory the registered command passes (the shape
// harness_compare_test's registered command gives its fixture
// directory), empty when nothing was passed.
auto fixtureDirectory = std::string {};

// The path of one fixture document under the fixture directory.
auto fixturePath(std::string_view name) -> std::string
{
  return fixtureDirectory + "/" + std::string(name);
}

// One fixture document loaded through `loadReport`, the only way the
// resolver block reaches a report value (B-1 of the views-api
// contract).
auto loadedFixture(std::string_view name) -> sg::ReportData
{
  auto const loaded = sg::loadReport(fixturePath(name));
  check(loaded.has_value(), "resolver: the fixture document loads");
  return std::move(loaded).value();
}

// The number of times one text stands inside another: the histogram
// block counts the elements of a figure this way.
auto occurrences(std::string const& text,
                 std::string const& piece) -> std::size_t
{
  auto count = std::size_t {};
  for (auto at = text.find(piece); at != std::string::npos;
       at = text.find(piece, at + 1))
  {
    ++count;
  }
  return count;
}

// The bar widths of a figure's rect elements, in document order: the
// proportionality check compares these against the counts the labels
// name.
auto barWidths(std::string const& svg) -> std::vector<std::size_t>
{
  auto widths = std::vector<std::size_t> {};
  std::string const attribute {"width=\""};
  for (auto at = svg.find("<rect"); at != std::string::npos;
       at = svg.find("<rect", at + 1))
  {
    auto const digits = svg.find(attribute, at);
    check(digits != std::string::npos, "histogram: every bar carries a width");
    auto const closed = svg.find('"', digits + attribute.size());
    check(closed != std::string::npos,
          "histogram: every width stands closed on its quote");
    widths.push_back(std::stoull(svg.substr(
        digits + attribute.size(), closed - digits - attribute.size())));
  }
  return widths;
}

// The coordinate pairs of a figure's polyline, in document order: the
// point-count check compares these against the values the series
// carries.
auto polylinePoints(std::string const& svg) -> std::vector<std::string>
{
  auto const attribute = std::string {"points=\""};
  auto const open = svg.find(attribute);
  check(open != std::string::npos, "plot: the figure carries a polyline");
  auto const closed = svg.find('"', open + attribute.size());
  check(closed != std::string::npos,
        "plot: the points attribute stands closed on its quote");
  auto pairs = std::vector<std::string> {};
  std::istringstream listed(
      svg.substr(open + attribute.size(), closed - open - attribute.size()));
  std::string pair;
  while (listed >> pair) {
    pairs.push_back(pair);
  }
  return pairs;
}

// The lines of one text view answer, in document order: the text
// block counts the lines the two parts compose this way.
auto textLines(std::string const& text) -> std::vector<std::string>
{
  auto lines = std::vector<std::string> {};
  std::istringstream listed(text);
  std::string line;
  while (std::getline(listed, line)) {
    lines.push_back(line);
  }
  return lines;
}

// The number of '#' characters one line carries: the proportionality
// check compares the bin lines' character bars this way.
auto barLength(std::string const& line) -> std::size_t
{
  return static_cast<std::size_t>(std::count(line.begin(), line.end(), '#'));
}

// The argument lists the header declares: every view takes the loaded
// document value `loadReport` answered by const reference, the
// benchmark name and the column selector as `std::string_view`, and
// the view's own arguments (A-1 of the views-api contract).
using ReportArg = const sg::ReportData&;

static_assert(
    std::is_same_v<decltype(sg::viewSeries(std::declval<ReportArg>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::string_view>())),
                   std::expected<sg::ViewSeries, std::string>>);

static_assert(std::is_same_v<
              decltype(sg::renderHistogram(std::declval<ReportArg>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::size_t>())),
              std::expected<std::string, std::string>>);

static_assert(
    std::is_same_v<decltype(sg::renderPlot(std::declval<ReportArg>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::string_view>())),
                   std::expected<std::string, std::string>>);

static_assert(
    std::is_same_v<decltype(sg::renderPage(std::declval<ReportArg>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::size_t>())),
                   std::expected<std::string, std::string>>);

static_assert(
    std::is_same_v<decltype(sg::renderText(std::declval<ReportArg>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::string_view>(),
                                           std::declval<std::size_t>())),
                   std::expected<std::string, std::string>>);

static_assert(
    std::is_same_v<decltype(sg::ViewSeries::values), std::vector<double>>);
static_assert(std::is_same_v<decltype(sg::ViewSeries::unit), std::string>);

auto run() -> int
{
  // ViewSeries default-constructs empty (specs/038 data-model.md).
  {
    auto const series = sg::ViewSeries {};
    check(series.values.empty(), "pass: default values empty");
    check(series.unit.empty(), "pass: default unit empty");
  }

  // The version and the SOVERSION record of the move this task lands
  // (specs/040-hunter-sensitivity FR-050): the project block reads
  // 0.26.0, the harness target answers SOVERSION 17, and the archive
  // target keeps SOVERSION 4.
  {
    auto const cmake = rootCMakeText();
    check(!cmake.empty(), "pass: the root CMakeLists.txt can be read");
    check(projectVersion(cmake) == "0.26.0",
          "pass: the project version reads 0.26.0");
    check(cmake.find("SOVERSION 17") != std::string::npos,
          "pass: the harness SOVERSION line reads SOVERSION 17");
    check(cmake.find("SOVERSION 4") != std::string::npos,
          "pass: the archive SOVERSION line reads SOVERSION 4");
  }

  // ---- The resolver block (specs/038 T004; FR-004 to FR-006, B-1,
  // A-3 to A-5 of the views-api contract) ----

  auto const multi = loadedFixture("multi.json");

  // Row 1: the three column kinds over multi.json (FR-004, FR-006,
  // A-3). The fixture stands five repetitions plus the aggregate; row
  // 2 carries the time GAP, row 3 the metric GAP, so the countable
  // rows a value stands for differ per column and the aggregate
  // stands out of every set (A-4).
  {
    auto const time = sg::viewSeries(multi, "bmViews/threads:2", "time");
    check(time.has_value(), "resolver: time answers the value form");
    check(time->values.size() == 4,
          "resolver: time keeps the four countable rows");
    check(sameDouble(time->values[0], 120.0 / 1000.0),
          "resolver: time resolves row 0 in document order");
    check(sameDouble(time->values[1], 145.0 / 1000.0),
          "resolver: time resolves row 1 in document order");
    check(sameDouble(time->values[2], 185.0 / 1000.0),
          "resolver: time skips the GAP row in document order");
    check(sameDouble(time->values[3], 240.0 / 1000.0),
          "resolver: time resolves the last countable row");

    auto const cpu = sg::viewSeries(multi, "bmViews/threads:2", "cpu");
    check(cpu.has_value(), "resolver: cpu answers the value form");
    check(cpu->values.size() == 4,
          "resolver: cpu keeps the four countable rows");
    check(sameDouble(cpu->values[0], 118.0 / 1000.0),
          "resolver: cpu resolves row 0 in document order");
    check(sameDouble(cpu->values[1], 143.0 / 1000.0),
          "resolver: cpu resolves row 1 in document order");
    check(sameDouble(cpu->values[2], 182.0 / 1000.0),
          "resolver: cpu skips the GAP row in document order");
    check(sameDouble(cpu->values[3], 236.0 / 1000.0),
          "resolver: cpu resolves the last countable row");

    auto const ipc = sg::viewSeries(multi, "bmViews/threads:2", "ipc");
    check(ipc.has_value(), "resolver: the metric label answers the value form");
    check(ipc->values.size() == 4,
          "resolver: ipc keeps the four countable rows");
    check(sameDouble(ipc->values[0], 1.1),
          "resolver: ipc resolves row 0 in document order");
    check(sameDouble(ipc->values[1], 1.4),
          "resolver: ipc resolves row 1 in document order");
    check(sameDouble(ipc->values[2], 1.9),
          "resolver: ipc counts the row the time column skips");
    check(sameDouble(ipc->values[3], 3.2),
          "resolver: ipc skips the GAP metric in document order");
  }

  // Row 2: the time columns scale by timeUnit with the unit named
  // (A-5). The fixture stands MICROSECONDS, whose divisor is 1000, and
  // the harness prints that unit as `us` (the UnitScale table of
  // source/harness/report.cpp).
  {
    auto const time = sg::viewSeries(multi, "bmViews/threads:2", "time");
    auto const cpu = sg::viewSeries(multi, "bmViews/threads:2", "cpu");
    check(time.has_value() && cpu.has_value(),
          "resolver: the time columns answer the value form");
    check(time->unit == "us" && cpu->unit == "us",
          "resolver: the time columns name the unit MICROSECONDS prints as");
    check(sameDouble(time->values[0], 120.0 / 1000.0),
          "resolver: the stored 120 ns prints as 0.12 us");
    check(sameDouble(time->values[1], 145.0 / 1000.0),
          "resolver: the stored 145 ns prints as 0.145 us");
    check(sameDouble(time->values[2], 185.0 / 1000.0),
          "resolver: the stored 185 ns prints as 0.185 us");
    check(sameDouble(time->values[3], 240.0 / 1000.0),
          "resolver: the stored 240 ns prints as 0.24 us");
  }

  // Row 3: the metric column prints as stored and names no unit (A-5,
  // the data-model ruling).
  {
    auto const ipc = sg::viewSeries(multi, "bmViews/threads:2", "ipc");
    check(ipc.has_value(), "resolver: ipc answers the value form");
    check(sameDouble(ipc->values[0], 1.1) && sameDouble(ipc->values[1], 1.4)
              && sameDouble(ipc->values[2], 1.9)
              && sameDouble(ipc->values[3], 3.2),
          "resolver: the metric values stand as stored");
    check(ipc->unit.empty(), "resolver: a metric column names no unit");
  }

  // Row 4: single.json answers the value form with the one value; the
  // resolver discloses nothing on a small series, the views decide
  // (B-1, the empty-values post).
  {
    auto const report = loadedFixture("single.json");
    auto const series = sg::viewSeries(report, "bmViewsSingle", "time");
    check(series.has_value(),
          "resolver: the single-repetition series answers the value form");
    check(series->values.size() == 1,
          "resolver: the one repetition stands as the one value");
    check(sameDouble(series->values[0], 55.0),
          "resolver: the one value stands stored");
    check(series->unit == "ns",
          "resolver: the unit the document records names the values");
  }

  // Row 5: a benchmark the document does not carry refuses, the error
  // form naming the name and the recorded set (B-1, A-2).
  {
    auto const missing = sg::viewSeries(multi, "nonexistent", "time");
    check(!missing.has_value(),
          "resolver: a benchmark outside the document refuses");
    check(missing.error().find("nonexistent") != std::string::npos,
          "resolver: the refusal names the requested name");
    check(missing.error().find("bmViews/threads:2") != std::string::npos,
          "resolver: the refusal names the recorded set");
  }

  // Row 6: the markup label resolves as a metric label; a
  // counter-label-shaped name stands outside the selector (A-3).
  {
    auto const markup = loadedFixture("markup.json");
    auto const labeled =
        sg::viewSeries(markup, "bmViewsMarkup", "branch<miss>&rate");
    check(labeled.has_value(), "resolver: the markup label resolves");
    check(labeled->values.size() == 4,
          "resolver: the markup label keeps the four countable rows");
    check(sameDouble(labeled->values[0], 0.4)
              && sameDouble(labeled->values[1], 0.6)
              && sameDouble(labeled->values[2], 0.7)
              && sameDouble(labeled->values[3], 1.1),
          "resolver: the markup label resolves the stored values");

    auto const counter = sg::viewSeries(multi, "bmViews/threads:2", "cycles");
    check(!counter.has_value(),
          "resolver: a counter-label-shaped name refuses");
    check(counter.error().find("cycles") != std::string::npos,
          "resolver: the refusal names the counter label");
    check(counter.error().find("ipc") != std::string::npos,
          "resolver: the refusal names the recorded set");
  }

  // Row 7: a document with zero benchmarks refuses the request, the
  // scratch document standing in the temporary directory beside the
  // test's own name (B-1).
  {
    auto const scratch = std::filesystem::temp_directory_path()
        / "views_test_zero_benchmarks.json";
    {
      std::ofstream out(scratch);
      out << R"({"schemaVersion":"1.8","benchmarks":[]})";
    }
    auto const empty = sg::loadReport(scratch.string());
    check(empty.has_value(), "resolver: the zero-benchmark document loads");
    auto const refused = sg::viewSeries(*empty, "nothing", "time");
    check(!refused.has_value(),
          "resolver: a zero-benchmark document refuses the request");
    check(refused.error().find("nothing") != std::string::npos,
          "resolver: the refusal names the requested name");
    std::error_code ignored;
    std::filesystem::remove(scratch, ignored);
  }

  // Row 8: multipass.json stands its metric values in pass order (D-7,
  // the document order of the passes).
  {
    auto const report = loadedFixture("multipass.json");
    auto const faults = sg::viewSeries(report, "bmViewsMultipass", "faults");
    check(faults.has_value(), "resolver: the multipass series answers");
    check(faults->values.size() == 3,
          "resolver: the multipass series keeps the three passes");
    check(sameDouble(faults->values[0], 8.0)
              && sameDouble(faults->values[1], 8.5)
              && sameDouble(faults->values[2], 9.1),
          "resolver: the values stand in pass order");
    check(faults->unit.empty(), "resolver: the metric names no unit");
  }

  // ---- The escape block (specs/038 T006; A-6, D-8 of the views-api
  // contract, the escaper of M-C) ----

  // Row 1: the markup characters stand escaped (A-6). The answer
  // spells the HTML/SVG standard named entities, and the ampersand
  // stands rewritten first, since the entity spellings the other two
  // rules introduce carry an `&` themselves.
  {
    check(sg::detail::escapeMarkup("&<>") == "&amp;&lt;&gt;",
          "escape: the markup characters stand escaped");
  }

  // Row 2: the plain text stands byte-identical (A-6): the string
  // carries no markup character, so the escaper rewrites nothing.
  {
    auto const plain = std::string("bmViews/threads:2");
    check(sg::detail::escapeMarkup(plain) == plain,
          "escape: the plain text stands byte-identical");
  }

  // Row 3: the empty string answers the empty string.
  {
    check(sg::detail::escapeMarkup(std::string_view {}).empty(),
          "escape: the empty string answers the empty string");
  }

  // Row 4: the fixture's markup metric label stands escaped through
  // the escaper (A-6, D-8): the label markup.json records reads
  // `branch<miss>&rate` and escapes to `branch&lt;miss&gt;&amp;rate`.
  {
    auto const markup = loadedFixture("markup.json");
    check(
        !markup.results.empty() && !markup.results.front().metricLabels.empty(),
        "escape: the markup fixture carries its labeled metric");
    auto const& label = markup.results.front().metricLabels.front();
    check(sg::detail::escapeMarkup(label) == "branch&lt;miss&gt;&amp;rate",
          "escape: the fixture's markup label stands escaped");
  }

  // ---- The histogram block (specs/038 T008; FR-007, FR-008, B-2 of
  // the views-api contract) ----

  // Row 1: the figure carries one bar per bin beside its range and
  // count label, the bins cover the recorded value range with the last
  // bin closed on the maximum, and the bars stand proportional to the
  // counts (FR-007, B-2). multi.json stands four countable time values
  // in us, 0.12 to 0.24, so the four bins stand 0.12-0.15, 0.15-0.18,
  // 0.18-0.21 and the last bin 0.21-0.24 closed on the maximum, and
  // the counts stand 2, 0, 1 and 1.
  {
    auto const svg = sg::renderHistogram(multi, "bmViews/threads:2", "time", 4);
    check(svg.has_value(), "histogram: the time series answers the value form");
    check(occurrences(*svg, "<rect") == 4, "histogram: one bar stands per bin");
    check(svg->find("0.12-0.15 us: 2") != std::string::npos,
          "histogram: the first bin names its range and count");
    check(svg->find("0.15-0.18 us: 0") != std::string::npos,
          "histogram: the empty bin names its zero count");
    check(svg->find("0.18-0.21 us: 1") != std::string::npos,
          "histogram: the third bin names its range and count");
    check(svg->find("0.21-0.24 us: 1") != std::string::npos,
          "histogram: the last bin stands closed on the maximum");
    auto const widths = barWidths(*svg);
    check(widths.size() == 4, "histogram: every bar carries its width");
    check(widths[1] == 0, "histogram: the empty bin carries no bar");
    check(widths[0] == 2 * widths[2] && widths[0] == 2 * widths[3],
          "histogram: the bars stand proportional to the counts");
  }

  // Row 2: the bin count argument bounds the bars (FR-007): two bins
  // over the same series answer two bars, 0.12-0.18 and the last bin
  // 0.18-0.24 closed on the maximum, the counts 2 and 2.
  {
    auto const two = sg::renderHistogram(multi, "bmViews/threads:2", "time", 2);
    check(two.has_value(),
          "histogram: the two-bin answer stands the value form");
    check(occurrences(*two, "<rect") == 2,
          "histogram: the bin count bounds the bars");
    check(two->find("0.12-0.18 us: 2") != std::string::npos
              && two->find("0.18-0.24 us: 2") != std::string::npos,
          "histogram: the two bins cover the recorded range");
  }

  // Row 3: the one-value disclosure (FR-008, B-2): single.json stands
  // one repetition row, so the histogram answers the error form naming
  // `not available` and the sample count.
  {
    auto const report = loadedFixture("single.json");
    auto const disclosed =
        sg::renderHistogram(report, "bmViewsSingle", "time", 4);
    check(!disclosed.has_value(),
          "histogram: the one-value series answers the error form");
    check(disclosed.error().find("not available") != std::string::npos,
          "histogram: the disclosure names not available");
    check(disclosed.error().find("sample count 1") != std::string::npos,
          "histogram: the disclosure names the sample count");
  }

  // Row 4: the all-unavailable disclosure (FR-008, D-6): every
  // repetition row of the scratch document stands a GAP for the
  // metric, so the series stands empty and the histogram answers the
  // error form naming the sample count of zero.
  {
    auto const scratch = std::filesystem::temp_directory_path()
        / "views_test_all_unavailable.json";
    {
      std::ofstream out(scratch);
      out << R"({"schemaVersion":"1.8","benchmarks":[
          {"name":"bmViewsGap","suite":"bmViews","case":"bmViewsGap",
           "outcome":"MEASURED","reason":null,"hideRepetitionRows":false,
           "timeUnit":"NANOSECONDS","primaryIsCpu":false,
           "pauseSamplesFolded":false,"percentilePoints":[],"contexts":[],
           "rows":[{"kind":"REPETITION","label":null,"iterations":8,
                    "timePerIterationNs":10.0,"cpuTimePerIterationNs":10.0,
                    "endpointSamples":8,"pauseSamplesFolded":false,
                    "timeAvailability":"COUNTABLE",
                    "metrics":[{"label":"gap","value":0.0,
                                "runningRatio":1.0,"scaled":false,
                                "availability":"GAP","pass":null}],
                    "counters":[],"counterDisclosure":null,"fit":null,
                    "overheadFloorNs":0.0,"percentiles":[],"chunkCount":0,
                    "chunkIterations":0,"timeStats":null,"metricStats":[]}],
           "metricLabels":["gap"],"counterLabels":[],"counterStats":[],
           "statisticValues":[]}]})";
    }
    auto const empty = sg::loadReport(scratch.string());
    check(empty.has_value(), "histogram: the all-unavailable document loads");
    auto const disclosed = sg::renderHistogram(*empty, "bmViewsGap", "gap", 4);
    check(!disclosed.has_value(),
          "histogram: the empty series answers the error form");
    check(disclosed.error().find("sample count 0") != std::string::npos,
          "histogram: the disclosure names the sample count of zero");
    std::error_code ignored;
    std::filesystem::remove(scratch, ignored);
  }

  // Row 5: the markup label (A-6, D-8): the figure names its column
  // through the escaper, and the metric values stand as stored with no
  // unit beside them (A-5). The four countable values 0.4 to 1.1 stand
  // in the bins 0.4-0.575, 0.575-0.75, 0.75-0.925 and 0.925-1.1.
  {
    auto const report = loadedFixture("markup.json");
    auto const markup =
        sg::renderHistogram(report, "bmViewsMarkup", "branch<miss>&rate", 4);
    check(markup.has_value(),
          "histogram: the metric series answers the value form");
    check(markup->find("branch&lt;miss&gt;&amp;rate") != std::string::npos,
          "histogram: the column label stands escaped through the escaper");
    check(markup->find("branch<miss>&rate") == std::string::npos,
          "histogram: the raw label stands nowhere in the figure");
    check(markup->find("0.4-0.575: 1") != std::string::npos
              && markup->find("0.925-1.1: 1") != std::string::npos,
          "histogram: the metric values stand as stored, no unit beside");
  }

  // Row 6: the contract rows (A-1, B-2). The pre binCount at least 2
  // stands enforced through the contract facility in the
  // implementation; the standing tests of this file assert no
  // contract-violation abort, so the block leaves the violation
  // untested, the way the resolver block leaves its empty-name pre.
  // The no-file and no-throw rules stand in the answer forms: every
  // call above answers the value or the error form, the document value
  // arrives by const reference as the whole input surface, and no
  // exception escapes the try of main.

  // ---- The plot block (specs/038 T010; FR-009, FR-010, B-3 of the
  // views-api contract) ----

  // Row 1: the figure carries one point per recorded value in document
  // order with the axes labeled and the values printed scaled (FR-009,
  // B-3, A-5). multi.json stands five repetition rows, so the four
  // countable time values 0.12, 0.145, 0.185 and 0.24 us stand four
  // points and the row the time column leaves unavailable stands out.
  {
    auto const svg = sg::renderPlot(multi, "bmViews/threads:2", "time");
    check(svg.has_value(), "plot: the time series answers the value form");
    check(polylinePoints(*svg).size() == 4,
          "plot: one point stands per recorded value");
    check(svg->find("0 us") == std::string::npos,
          "plot: the GAP row stands out of the points");
    check(svg->find("0.12 us") != std::string::npos
              && svg->find("0.145 us") != std::string::npos
              && svg->find("0.185 us") != std::string::npos
              && svg->find("0.24 us") != std::string::npos,
          "plot: the values print scaled with the unit named");
    check(svg->find("0.12 us") < svg->find("0.145 us")
              && svg->find("0.145 us") < svg->find("0.185 us")
              && svg->find("0.185 us") < svg->find("0.24 us"),
          "plot: the points stand in document order");
    check(svg->find("time (us)") != std::string::npos,
          "plot: the column axis names the column and the unit");
    check(svg->find("row") != std::string::npos,
          "plot: the row axis names the row");
  }

  // Row 2: the multipass order (FR-009, D-7): multipass.json stands
  // its metric values in pass order, so the figure renders the three
  // passes in that order, the aggregate row standing out of the
  // points, and the values print as stored with no unit beside (A-5).
  {
    auto const report = loadedFixture("multipass.json");
    auto const svg = sg::renderPlot(report, "bmViewsMultipass", "faults");
    check(svg.has_value(), "plot: the multipass series answers the value form");
    check(polylinePoints(*svg).size() == 3,
          "plot: the multipass figure renders the three passes");
    check(svg->find(">8<") != std::string::npos
              && svg->find(">8.5<") != std::string::npos
              && svg->find(">9.1<") != std::string::npos,
          "plot: the metric values stand as stored, no unit beside");
    check(svg->find(">8<") < svg->find(">8.5<")
              && svg->find(">8.5<") < svg->find(">9.1<"),
          "plot: the passes stand in pass order");
  }

  // Row 3: the single-point case (B-3, FR-010): single.json stands one
  // repetition row, so the figure renders one point and answers no
  // disclosure.
  {
    auto const report = loadedFixture("single.json");
    auto const svg = sg::renderPlot(report, "bmViewsSingle", "time");
    check(svg.has_value(),
          "plot: the single-value series answers the value form");
    check(polylinePoints(*svg).size() == 1,
          "plot: the single value renders one point");
    check(svg->find("55 ns") != std::string::npos,
          "plot: the single point names its value and unit");
  }

  // Row 4: the empty-series disclosure (FR-010, clarify 2/Q2): every
  // repetition row of the scratch document stands a GAP for the
  // metric, so the series stands empty and the plot answers the error
  // form naming the column and the sample count of zero.
  {
    auto const scratch =
        std::filesystem::temp_directory_path() / "views_test_plot_empty.json";
    {
      std::ofstream out(scratch);
      out << R"({"schemaVersion":"1.8","benchmarks":[
          {"name":"bmViewsPlotGap","suite":"bmViews",
           "case":"bmViewsPlotGap","outcome":"MEASURED","reason":null,
           "hideRepetitionRows":false,"timeUnit":"NANOSECONDS",
           "primaryIsCpu":false,"pauseSamplesFolded":false,
           "percentilePoints":[],"contexts":[],
           "rows":[{"kind":"REPETITION","label":null,"iterations":8,
                    "timePerIterationNs":10.0,"cpuTimePerIterationNs":10.0,
                    "endpointSamples":8,"pauseSamplesFolded":false,
                    "timeAvailability":"COUNTABLE",
                    "metrics":[{"label":"gap","value":0.0,
                                "runningRatio":1.0,"scaled":false,
                                "availability":"GAP","pass":null}],
                    "counters":[],"counterDisclosure":null,"fit":null,
                    "overheadFloorNs":0.0,"percentiles":[],"chunkCount":0,
                    "chunkIterations":0,"timeStats":null,"metricStats":[]}],
           "metricLabels":["gap"],"counterLabels":[],"counterStats":[],
           "statisticValues":[]}]})";
    }
    auto const empty = sg::loadReport(scratch.string());
    check(empty.has_value(), "plot: the all-unavailable document loads");
    auto const disclosed = sg::renderPlot(*empty, "bmViewsPlotGap", "gap");
    check(!disclosed.has_value(),
          "plot: the empty series answers the error form");
    check(disclosed.error().find("not available") != std::string::npos,
          "plot: the disclosure names not available");
    check(disclosed.error().find("gap") != std::string::npos,
          "plot: the disclosure names the column");
    check(disclosed.error().find("sample count 0") != std::string::npos,
          "plot: the disclosure names the sample count of zero");
    std::error_code ignored;
    std::filesystem::remove(scratch, ignored);
  }

  // Row 5: the markup label (A-6, D-8): the figure names its column
  // through the escaper, and the metric values stand as stored with no
  // unit beside them (A-5). The four countable values 0.4 to 1.1 stand
  // one point each in document order.
  {
    auto const report = loadedFixture("markup.json");
    auto const markup =
        sg::renderPlot(report, "bmViewsMarkup", "branch<miss>&rate");
    check(markup.has_value(), "plot: the metric series answers the value form");
    check(markup->find("branch&lt;miss&gt;&amp;rate") != std::string::npos,
          "plot: the column label stands escaped through the escaper");
    check(markup->find("branch<miss>&rate") == std::string::npos,
          "plot: the raw label stands nowhere in the figure");
    check(markup->find(">0.4<") != std::string::npos
              && markup->find(">0.6<") != std::string::npos
              && markup->find(">0.7<") != std::string::npos
              && markup->find(">1.1<") != std::string::npos,
          "plot: the metric values stand as stored, no unit beside");
    check(markup->find(">0.4<") < markup->find(">0.6<")
              && markup->find(">0.6<") < markup->find(">0.7<")
              && markup->find(">0.7<") < markup->find(">1.1<"),
          "plot: the metric points stand in document order");
  }

  // Row 6: the contract rows (A-1, B-3). The plot carries no pre, so
  // the block enforces no contract facility call, and the standing
  // tests of this file assert no contract-violation abort. The no-file
  // and no-throw rules stand in the answer forms: every call above
  // answers the value or the error form, the document value arrives by
  // const reference as the whole input surface, and no exception
  // escapes the try of main.

  // ---- The text block (specs/038 T012; FR-013, B-4 of the views-api
  // contract) ----

  // Row 1: the histogram part renders one line per bin naming the
  // range and count the graphic histogram renders, the unit scaled
  // beside the values (FR-013, B-4, A-5), and the character bars stand
  // proportional to the counts. multi.json stands the four countable
  // time values 0.12 to 0.24 us, so the four bins stand 0.12-0.15,
  // 0.15-0.18, 0.18-0.21 and the last bin 0.21-0.24 closed on the
  // maximum, the counts 2, 0, 1 and 1.
  {
    auto const text = sg::renderText(multi, "bmViews/threads:2", "time", 4);
    check(text.has_value(), "text: the time series answers the value form");
    auto const lines = textLines(*text);
    check(
        lines.size() == 9,
        "text: the column, the four bin lines and the four value lines stand");
    check(lines[0] == "time", "text: the view names its column");
    check(lines[1].starts_with("0.12-0.15 us: 2"),
          "text: the first bin names its range and count");
    check(lines[2].starts_with("0.15-0.18 us: 0"),
          "text: the empty bin names its zero count");
    check(lines[3].starts_with("0.18-0.21 us: 1"),
          "text: the third bin names its range and count");
    check(lines[4].starts_with("0.21-0.24 us: 1"),
          "text: the last bin stands closed on the maximum");
    check(barLength(lines[1]) == 2 * barLength(lines[3])
              && barLength(lines[3]) == barLength(lines[4])
              && barLength(lines[1]) > 0,
          "text: the character bars stand proportional to the counts");
    check(barLength(lines[2]) == 0, "text: the empty bin carries no bar");
  }

  // Row 2: the plot part carries one line per recorded value in row
  // order (FR-013, B-4, D-7), the values in the scaled unit (A-5) and
  // the GAP row standing out (D-6). multi.json stands the four
  // countable time values 0.12, 0.145, 0.185 and 0.24 us; the row the
  // time column leaves unavailable stands out of the lines.
  {
    auto const text = sg::renderText(multi, "bmViews/threads:2", "time", 4);
    check(text.has_value(), "text: the time series answers the value form");
    auto const lines = textLines(*text);
    check(lines[5] == "0.12 us" && lines[6] == "0.145 us"
              && lines[7] == "0.185 us" && lines[8] == "0.24 us",
          "text: one line stands per recorded value in row order");
    check(text->find("0 us") == std::string::npos,
          "text: the GAP row stands out of the lines");
  }

  // Row 3: the one-repetition composition (FR-013, clarify 2/Q1, the
  // rule FR-012 states for the page): single.json stands one
  // repetition row, so the histogram part renders the disclosure line
  // in the histogram's place, the plot part renders the single
  // value's line, and the whole answers the value form.
  {
    auto const report = loadedFixture("single.json");
    auto const text = sg::renderText(report, "bmViewsSingle", "time", 4);
    check(text.has_value(),
          "text: the one-repetition view answers the value form");
    check(text->find("not available") != std::string::npos,
          "text: the disclosed part names not available");
    check(text->find("sample count 1") != std::string::npos,
          "text: the disclosure names the sample count");
    auto const lines = textLines(*text);
    check(lines.size() == 3,
          "text: the column, the disclosure line and the value line stand");
    check(lines[1].find("not available") != std::string::npos
              && lines[1].find("sample count 1") != std::string::npos,
          "text: the disclosure stands in the histogram part's place");
    check(lines[2] == "55 ns",
          "text: the plot part renders the single value's line");
  }

  // Row 3b: the all-unavailable series (FR-013, D-6): every repetition
  // row stands a GAP for the column, so both parts answer the
  // disclosure and each part's line stands in its place, the whole
  // answering the value form.
  {
    auto const scratch = std::filesystem::temp_directory_path()
        / "views_test_text_all_unavailable.json";
    {
      std::ofstream out(scratch);
      out << R"({"schemaVersion":"1.8","benchmarks":[
          {"name":"bmViewsGap","suite":"bmViews","case":"bmViewsGap",
           "outcome":"MEASURED","reason":null,"hideRepetitionRows":false,
           "timeUnit":"NANOSECONDS","primaryIsCpu":false,
           "pauseSamplesFolded":false,"percentilePoints":[],"contexts":[],
           "rows":[{"kind":"REPETITION","label":null,"iterations":8,
                    "timePerIterationNs":10.0,"cpuTimePerIterationNs":10.0,
                    "endpointSamples":8,"pauseSamplesFolded":false,
                    "timeAvailability":"COUNTABLE",
                    "metrics":[{"label":"gap","value":0.0,
                                "runningRatio":1.0,"scaled":false,
                                "availability":"GAP","pass":null}],
                    "counters":[],"counterDisclosure":null,"fit":null,
                    "overheadFloorNs":0.0,"percentiles":[],"chunkCount":0,
                    "chunkIterations":0,"timeStats":null,"metricStats":[]}],
           "metricLabels":["gap"],"counterLabels":[],"counterStats":[],
           "statisticValues":[]}]})";
    }
    auto const empty = sg::loadReport(scratch.string());
    check(empty.has_value(), "text: the all-unavailable document loads");
    auto const text = sg::renderText(*empty, "bmViewsGap", "gap", 4);
    check(text.has_value(), "text: the all-gap view answers the value form");
    const auto lines = textLines(*text);
    check(lines.size() == 3,
          "text: the column and the two disclosure lines stand");
    check(lines[1].find("not available") != std::string::npos
              && lines[1].find("sample count 0") != std::string::npos,
          "text: the histogram part's disclosure stands in its place");
    check(lines[2].find("not available") != std::string::npos
              && lines[2].find("sample count 0") != std::string::npos,
          "text: the plot part's disclosure stands in its place");
    std::error_code ignored;
    std::filesystem::remove(scratch, ignored);
  }

  // Row 4: the markup label (A-6, D-8): the view names its column
  // through the escaper, the way the figures name theirs.
  {
    auto const report = loadedFixture("markup.json");
    auto const text =
        sg::renderText(report, "bmViewsMarkup", "branch<miss>&rate", 4);
    check(text.has_value(), "text: the markup series answers the value form");
    check(text->find("branch&lt;miss&gt;&amp;rate") != std::string::npos,
          "text: the column label stands escaped through the escaper");
    check(text->find("branch<miss>&rate") == std::string::npos,
          "text: the raw label stands nowhere in the view");
  }

  // Row 5: the contract rows (A-1, B-4). The pre binCount at least 2
  // stands enforced through the contract facility in the
  // implementation; the standing tests of this file assert no
  // contract-violation abort, so the block leaves the violation
  // untested, the way the histogram block leaves its. The no-file and
  // no-throw rules stand in the answer forms: every call above answers
  // the value or the error form, the document value arrives by const
  // reference as the whole input surface, and no exception escapes the
  // try of main.

  // ---- The page block (specs/038 T014; FR-011, FR-012, B-5 of the
  // views-api contract) ----

  // Row 1: the identity (FR-011, B-5): the page answers one static
  // HTML document carrying the speedgun version, the schemaVersion,
  // the benchmark name and the column, each in the document's escaped
  // spelling. multi.json records the version 0.25.0 and the
  // schemaVersion 1.8, and the document shell stands the doctype, the
  // html element with its head and its body.
  {
    auto const page = sg::renderPage(multi, "bmViews/threads:2", "time", 4);
    check(page.has_value(), "page: the request answers the value form");
    check(page->starts_with("<!DOCTYPE html>"),
          "page: the document opens with its doctype");
    check(page->find("<html>") != std::string::npos
              && page->find("<head>") != std::string::npos
              && page->find("<body>") != std::string::npos,
          "page: the html element carries its head and its body");
    check(page->find("0.25.0") != std::string::npos,
          "page: the speedgun version the document records stands");
    check(page->find("1.8") != std::string::npos,
          "page: the schemaVersion the document records stands");
    check(page->find("bmViews/threads:2") != std::string::npos,
          "page: the benchmark name stands");
    check(page->find("time") != std::string::npos, "page: the column stands");
  }

  // Row 2: the embedded figures stand the standalone bytes (B-5, D-3):
  // the page embeds the histogram and the plot figures the standalone
  // calls answer for the same request, the page adding only its
  // document shell around them.
  {
    auto const page = sg::renderPage(multi, "bmViews/threads:2", "time", 4);
    auto const histogram =
        sg::renderHistogram(multi, "bmViews/threads:2", "time", 4);
    auto const plot = sg::renderPlot(multi, "bmViews/threads:2", "time");
    check(page.has_value() && histogram.has_value() && plot.has_value(),
          "page: the page and the two figures answer the value form");
    check(page->find(*histogram) != std::string::npos,
          "page: the histogram stands the standalone bytes");
    check(page->find(*plot) != std::string::npos,
          "page: the plot stands the standalone bytes");
  }

  // Row 3: the absent-field disclosure (FR-012, clarify 1/Q4): the
  // document of nospeedgun.json carries no speedgun header, so the
  // version field renders the not available disclosure in its place,
  // the version string standing nowhere, and the page answers the
  // value form.
  {
    auto const report = loadedFixture("nospeedgun.json");
    auto const page = sg::renderPage(report, "bmViewsNoHeader", "time", 4);
    check(page.has_value(),
          "page: the no-header document answers the value form");
    check(page->find("not available") != std::string::npos,
          "page: the absent identity field renders the disclosure");
    check(page->find("0.25.0") == std::string::npos,
          "page: the version the document does not carry stands nowhere");
  }

  // Row 4: the disclosed figure in place (FR-012, clarify 1/Q4):
  // single.json stands one repetition row, so the page embeds the plot
  // figure and renders the histogram's disclosure text in the
  // histogram's place, the whole answering the value form.
  {
    auto const report = loadedFixture("single.json");
    auto const page = sg::renderPage(report, "bmViewsSingle", "time", 4);
    auto const plot = sg::renderPlot(report, "bmViewsSingle", "time");
    auto const histogram =
        sg::renderHistogram(report, "bmViewsSingle", "time", 4);
    check(page.has_value(),
          "page: the one-repetition page answers the value form");
    check(plot.has_value(), "page: the plot answers the value form");
    check(!histogram.has_value(), "page: the histogram answers the disclosure");
    check(page->find(*plot) != std::string::npos,
          "page: the plot figure stands inline");
    check(page->find(histogram.error()) != std::string::npos,
          "page: the disclosure text stands in the histogram's place");
  }

  // Row 5: the markup label (A-6, D-8): the page names its column
  // through the escaper, the way the figures name theirs.
  {
    auto const report = loadedFixture("markup.json");
    auto const page =
        sg::renderPage(report, "bmViewsMarkup", "branch<miss>&rate", 4);
    check(page.has_value(), "page: the markup page answers the value form");
    check(page->find("branch&lt;miss&gt;&amp;rate") != std::string::npos,
          "page: the column label stands escaped through the escaper");
    check(page->find("branch<miss>&rate") == std::string::npos,
          "page: the raw label stands nowhere in the page");
  }

  // Row 6: the no-script shape (FR-011, D-8): the page carries no
  // script element and no event attribute.
  {
    auto const page = sg::renderPage(multi, "bmViews/threads:2", "time", 4);
    check(page.has_value(), "page: the page answers the value form");
    check(page->find("<script") == std::string::npos,
          "page: the page carries no script element");
    check(page->find("onclick") == std::string::npos,
          "page: the page carries no event attribute");
  }

  // Row 7: the contract rows (A-1, B-5). The pre binCount at least 2
  // stands enforced through the contract facility in the
  // implementation; the standing tests of this file assert no
  // contract-violation abort, so the block leaves the violation
  // untested, the way the text block leaves its. The no-file and
  // no-throw rules stand in the answer forms: every call above answers
  // the value or the error form, the document value arrives by const
  // reference as the whole input surface, and no exception escapes the
  // try of main.

  // ---- The refusal and edge rows (specs/038 T023; the branches the
  // coverage gate reads) ----

  // The two-benchmark document: the refusal of an unmatched benchmark
  // names the recorded set with the separator, the metric-labels
  // refusal names both labels, and the equal-values range stands one
  // bin covering the value. The second repetition row carries no
  // metric slot for the label, so the row stands out of the metric
  // series the way an unavailable row stands out (D-6).
  {
    auto const scratch =
        std::filesystem::temp_directory_path() / "views_test_edge_cases.json";
    {
      std::ofstream out(scratch);
      out << R"({"schemaVersion":"1.8","benchmarks":[
          {"name":"bmEdge","suite":"bmEdge","case":"bmEdge",
           "outcome":"MEASURED","reason":null,"hideRepetitionRows":false,
           "timeUnit":"NANOSECONDS","primaryIsCpu":false,
           "pauseSamplesFolded":false,"percentilePoints":[],"contexts":[],
           "rows":[
            {"kind":"REPETITION","label":null,"iterations":8,
             "timePerIterationNs":50.0,"cpuTimePerIterationNs":60.0,
             "endpointSamples":8,"pauseSamplesFolded":false,
             "timeAvailability":"COUNTABLE",
             "metrics":[{"label":"m1","value":1.0,"runningRatio":1.0,
                         "scaled":false,"availability":"COUNTABLE",
                         "pass":null}],"counters":[],
             "counterDisclosure":null,"fit":null,"overheadFloorNs":0.0,
             "percentiles":[],"chunkCount":0,"chunkIterations":0,
             "timeStats":null,"metricStats":[]},
            {"kind":"REPETITION","label":null,"iterations":8,
             "timePerIterationNs":50.0,"cpuTimePerIterationNs":60.0,
             "endpointSamples":8,"pauseSamplesFolded":false,
             "timeAvailability":"COUNTABLE","metrics":[],
             "counters":[],"counterDisclosure":null,"fit":null,
             "overheadFloorNs":0.0,"percentiles":[],"chunkCount":0,
             "chunkIterations":0,"timeStats":null,"metricStats":[]}],
           "metricLabels":["m1","m2"],"counterLabels":[],
           "counterStats":[],"statisticValues":[]},
          {"name":"bmOther","suite":"bmOther","case":"bmOther",
           "outcome":"MEASURED","reason":null,"hideRepetitionRows":false,
           "timeUnit":"NANOSECONDS","primaryIsCpu":false,
           "pauseSamplesFolded":false,"percentilePoints":[],"contexts":[],
           "rows":[{"kind":"REPETITION","label":null,"iterations":8,
                    "timePerIterationNs":70.0,
                    "cpuTimePerIterationNs":70.0,
                    "endpointSamples":8,"pauseSamplesFolded":false,
                    "timeAvailability":"COUNTABLE","metrics":[],
                    "counters":[],"counterDisclosure":null,"fit":null,
                    "overheadFloorNs":0.0,"percentiles":[],
                    "chunkCount":0,"chunkIterations":0,
                    "timeStats":null,"metricStats":[]}],
           "metricLabels":[],"counterLabels":[],"counterStats":[],
           "statisticValues":[]}]})";
    }
    auto const edge = sg::loadReport(scratch.string());
    check(edge.has_value(), "edge: the two-benchmark document loads");

    check(!sg::viewSeries(*edge, "absent", "time").has_value()
              && sg::viewSeries(*edge, "absent", "time")
                      .error()
                      .find("bmEdge, bmOther")
                  != std::string::npos,
          "edge: the benchmark refusal names both recorded names");
    check(
        !sg::viewSeries(*edge, "bmEdge", "cycles").has_value()
            && sg::viewSeries(*edge, "bmEdge", "cycles").error().find("m1, m2")
                != std::string::npos,
        "edge: the column refusal names both recorded labels");
    check(!sg::viewSeries(*edge, "bmOther", "cycles").has_value()
              && sg::viewSeries(*edge, "bmOther", "cycles").error().find("none")
                  != std::string::npos,
          "edge: the column refusal over no labels names none");
    auto const equal = sg::viewSeries(*edge, "bmEdge", "time");
    check(equal.has_value() && equal->values.size() == 2
              && sameDouble(equal->values.front(), 50.0),
          "edge: the equal-values series answers the value form");
    auto const oneBin = sg::renderHistogram(*edge, "bmEdge", "time", 4);
    check(oneBin.has_value(), "edge: the equal-values histogram renders");
    check(oneBin->find("50-50 ns: 2") != std::string::npos,
          "edge: the equal-values range stands one bin covering the value");
    auto const shortMetrics = sg::viewSeries(*edge, "bmEdge", "m1");
    check(shortMetrics.has_value() && shortMetrics->values.size() == 1,
          "edge: the row without the metric slot stands out of the series");
    std::error_code edgeIgnored;
    std::filesystem::remove(scratch, edgeIgnored);
  }

  // The renderer error forms name the resolver's refusal: a benchmark
  // the document does not carry rides the one error shape to every
  // view's answer (FR-005, A-2).
  {
    auto const report = loadedFixture("multi.json");
    const auto histogram = sg::renderHistogram(report, "absent", "time", 4);
    check(!histogram.has_value()
              && histogram.error().find("absent") != std::string::npos,
          "edge: the histogram answers the resolver's refusal");
    const auto plot = sg::renderPlot(report, "absent", "time");
    check(!plot.has_value() && plot.error().find("absent") != std::string::npos,
          "edge: the plot answers the resolver's refusal");
    const auto text = sg::renderText(report, "absent", "time", 4);
    check(!text.has_value() && text.error().find("absent") != std::string::npos,
          "edge: the text view answers the resolver's refusal");
    const auto page = sg::renderPage(report, "absent", "time", 4);
    check(page.has_value(),
          "edge: the page answers the value form over a refused request");
    check(page->find("absent") != std::string::npos,
          "edge: the page carries the refusals in the figures' places");
  }

  std::printf("views_test PASS\n");
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (argc > 1 && argv[1] != nullptr) {
    fixtureDirectory = argv[1];
  }
  try {
    return run();
  } catch (...) {
    std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: unhandled exception\n");
    return 1;
  }
}
