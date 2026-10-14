#include <cstdint>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin run-loop
auto bmLoop(sg::State& state) -> void
{
  std::uint64_t total = 0;
  for (auto _ : state) {
    total += 1;
  }
  sg::doNotOptimize(total);
  sg::clobberMemory();
}

SG_BENCHMARK(bmLoop);

// sg-example:end run-loop

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
