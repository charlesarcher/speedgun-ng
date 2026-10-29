#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

// sg-example:begin report-views
#include <cstdio>

#include "speedgun-ng/views.hpp"

// The document the run wrote at its json path loads back, and the
// text view prints the terminal figures of the primary time column.
auto printTimeView(const std::string& path) -> void
{
  const auto document = sg::loadReport(path);
  if (!document.has_value()) {
    std::printf("%s\n", document.error().c_str());
    return;
  }
  const auto text = sg::renderText(*document, "bmViews", "time", 4);
  std::printf("%s\n", text.has_value() ? text->c_str() : text.error().c_str());
}

// sg-example:end report-views

// The manual times stand fixed per repetition, so the figures the views
// render from the written document read the same on each host.
auto bmViews(sg::State& state) -> void
{
  static constexpr std::array<std::int64_t, 5> kIterationNs {
      100, 100, 250, 400, 500};
  static std::size_t repetition = 0;
  const std::int64_t iterationNs =
      kIterationNs[repetition % kIterationNs.size()];
  ++repetition;
  for (auto _ : state) {
    state.setIterationTime(iterationNs);
  }
}

SG_BENCHMARK(bmViews).iterations(4).repetitions(5).useManualTime();

auto main(int argc, char** argv) -> int
{
  const auto path =
      (std::filesystem::temp_directory_path() / "report_views.json").string();
  std::string json = "--json=" + path;
  std::vector<char*> arguments(argv, argv + argc);
  arguments.push_back(json.data());
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
  const auto run =
      sg::speedgunMain(static_cast<int>(arguments.size()), arguments.data());
  if (std::filesystem::exists(path)) {
    printTimeView(path);
  }
  return run;
}
