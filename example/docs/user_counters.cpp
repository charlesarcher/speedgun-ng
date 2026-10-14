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
  const std::vector<std::uint64_t> source(cells, 1);
  std::vector<std::uint64_t> sink(cells, 0);
  for (auto _ : state) {
    for (std::size_t cell = 0; cell < cells; ++cell) {
      sink[cell] = source[cell] + 1;
    }
    state.setCounter("cells", {static_cast<double>(cells)});
    state.setCounter("bytes", {16.0 * static_cast<double>(cells),
                               sg::CounterFlag::NONE,
                               sg::CounterBase::K1024});
    sg::doNotOptimize(sink);
  }
}

SG_BENCHMARK(bmCopyCells).argNames({"cells"}).argsProduct({{64, 1024}});
// sg-example:end user-counters

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
