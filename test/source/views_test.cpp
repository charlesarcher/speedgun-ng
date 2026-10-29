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

  std::printf("views_test PASS\n");
  return 0;
}

}  // namespace

auto main() -> int
{
  try {
    return run();
  } catch (...) {
    std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: unhandled exception\n");
    return 1;
  }
}
