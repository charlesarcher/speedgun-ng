#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// The block the main page quotes: the level-1 ask over one identity, and the
// two answers a caller gets.
// sg-example:begin topdown
#include <cstdio>

#include "speedgun-ng/counters_topdown.hpp"

auto printTopdownLevel1(const sg::counters::CpuIdentity& identity) -> void
{
  const auto tree = sg::counters::topdownTree({identity, 1, {}});
  if (!tree.has_value()) {
    std::printf("%s\n", tree.error().message().c_str());
    return;
  }
  for (const auto& node : tree->nodes) {
    std::printf("%s  %s\n", node.name.c_str(), node.vendorName.c_str());
  }
}

// sg-example:end topdown

// The registered benchmark gives the run gate one row, and the run prints the
// level-1 figures of that row beside it.
auto bmTopdown(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmTopdown);

auto holdsWord(int argc, char** argv, std::string_view prefix) -> bool
{
  for (int index = 0; index < argc; ++index) {
    if (std::string_view(argv[index]).starts_with(prefix)) {
      return true;
    }
  }
  return false;
}

// The word this program adds stands after the words it was given, so a reader
// who names another level keeps that level. The tree ask stands after the run,
// because the run registers the clock provider the counters system opens with,
// and a listing answers names alone.
auto main(int argc, char** argv) -> int
{
  std::string option = "--topdown=1";
  std::vector<char*> arguments(argv, argv + argc);
  if (!holdsWord(argc, argv, "--topdown")) {
    arguments.push_back(option.data());
  }
  const auto status =
      sg::speedgunMain(static_cast<int>(arguments.size()), arguments.data());
  if (!holdsWord(argc, argv, "--list")) {
    printTopdownLevel1(sg::counters::cpuIdentity());
  }
  return status;
}
