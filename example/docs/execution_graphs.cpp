#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// The chain the graph states: three nodes over the three contexts of one
// threaded family, each node behind the one before it. The example writes the
// graph file, hands the entry point its path, and removes the file after the
// run.
// sg-example:begin execution-graphs
auto bmChain(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.threadIndex());
  }
}

SG_BENCHMARK(bmChain).threads(3);

// sg-example:end execution-graphs

namespace
{

// The file's bytes, in the key order the graph schema states: `nodes` at the
// top level, and `name`, `slot` and `deps` on each node.
constexpr std::string_view kGraphText {
    R"({
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "c", "slot": 2, "deps": ["b"] }
  ]
}
)"};

// The file's path, named by the process number so two runs never share one.
auto graphPath() -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("speedgun-execution-graphs-"
         + std::to_string(static_cast<long>(getpid())) + ".json");
}

// Whether the command line names a graph already: a caller that hands one owns
// the run, and the example adds its own option to no such line.
auto namesGraph(const std::vector<std::string>& args) -> bool
{
  for (const auto& arg : args) {
    if (arg.starts_with("--graph")) {
      return true;
    }
  }
  return false;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  auto args = std::vector<std::string> {argv, argv + argc};
  const auto path = graphPath();
  auto file = std::ofstream {path};
  file << kGraphText;
  file.close();
  if (!namesGraph(args)) {
    args.push_back("--graph=" + path.string());
  }
  auto pointers = std::vector<char*> {};
  for (auto& arg : args) {
    pointers.push_back(arg.data());
  }
  const int status =
      sg::speedgunMain(static_cast<int>(pointers.size()), pointers.data());
  std::filesystem::remove(path);
  return status;
}
