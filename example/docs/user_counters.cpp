// The named counters one run opens, each with the flags that scale its column.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin user-counters
auto bmCopyCells(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint64_t> sink(cells, 0);
  for (auto _ : state) {
    for (std::size_t cell = 0; cell < cells; ++cell) {
      sink[cell] = cell + 1;
    }
    state.setCounter("cells", {static_cast<double>(cells)});
    state.setBytesProcessed(static_cast<std::int64_t>(cells) * 16);
    sg::doNotOptimize(sink);
  }
}

SG_BENCHMARK(bmCopyCells).argsProduct({{64, 1024}}).repetitions(3);

// sg-example:end user-counters

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
