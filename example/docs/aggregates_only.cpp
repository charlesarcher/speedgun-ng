#include <cstddef>
#include <string>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin aggregates-only
auto bmCountWords(sg::State& state) -> void
{
  const std::string line =
      "speedgun-ng reports one row per repetition and one aggregate";
  for (auto _ : state) {
    std::size_t spaces = 0;
    for (const char letter : line) {
      spaces += letter == ' ' ? 1 : 0;
    }
    sg::doNotOptimize(spaces);
  }
}

SG_BENCHMARK(bmCountWords).repetitions(5).reportAggregatesOnly();

// sg-example:end aggregates-only

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
