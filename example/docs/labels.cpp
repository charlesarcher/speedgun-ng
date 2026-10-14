#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin labels
auto bmScanBlocks(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint64_t> block(cells, 3);
  for (auto _ : state) {
    std::uint64_t total = 0;
    for (const std::uint64_t cell : block) {
      total += cell;
    }
    sg::doNotOptimize(total);
  }
  state.setLabel("summed");
}

SG_BENCHMARK(bmScanBlocks).argNames({"cells"}).range(256, 4096).repetitions(3);

// sg-example:end labels

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
