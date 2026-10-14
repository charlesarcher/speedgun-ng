#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin complexity-fit
auto bmSwapPasses(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint64_t> values(cells, 1);
  for (auto _ : state) {
    for (std::size_t row = 0; row < cells; ++row) {
      for (std::size_t col = 0; col + 1 < cells; ++col) {
        std::swap(values[col], values[col + 1]);
      }
    }
    sg::doNotOptimize(values);
  }
}

SG_BENCHMARK(bmSwapPasses)
    .argsProduct({{8, 16, 32}})
    .complexity(sg::BigO::AUTOMATIC);
// sg-example:end complexity-fit

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
