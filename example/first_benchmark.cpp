// sg-example:begin program
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"  // sg::doNotOptimize is declared here
#include "speedgun-ng/benchmark.hpp"

auto bmSum(sg::State& state) -> void
{
  std::vector<std::uint64_t> cells(1 << 12, 1);
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t cell : cells) {
      total += cell;
    }
    sg::doNotOptimize(total);
  }
}

SG_BENCHMARK(bmSum);

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
// sg-example:end program
