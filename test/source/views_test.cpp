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
  // (specs/038 FR-002, D-2): the project block reads 0.25.0, the
  // harness target answers SOVERSION 16, and the archive target keeps
  // SOVERSION 4.
  {
    auto const cmake = rootCMakeText();
    check(!cmake.empty(), "pass: the root CMakeLists.txt can be read");
    check(projectVersion(cmake) == "0.25.0",
          "pass: the project version reads 0.25.0");
    check(cmake.find("SOVERSION 16") != std::string::npos,
          "pass: the harness SOVERSION line reads SOVERSION 16");
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
    auto const labelled =
        sg::viewSeries(markup, "bmViewsMarkup", "branch<miss>&rate");
    check(labelled.has_value(), "resolver: the markup label resolves");
    check(labelled->values.size() == 4,
          "resolver: the markup label keeps the four countable rows");
    check(sameDouble(labelled->values[0], 0.4)
              && sameDouble(labelled->values[1], 0.6)
              && sameDouble(labelled->values[2], 0.7)
              && sameDouble(labelled->values[3], 1.1),
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
