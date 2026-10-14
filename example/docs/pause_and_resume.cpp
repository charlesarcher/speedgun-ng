#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin paused
auto bmPaused(sg::State& state) -> void
{
  std::vector<std::uint64_t> scratch(64, 1);
  for (auto _ : state) {
    state.pauseTiming();
    scratch.assign(128, 2);
    state.resumeTiming();
    {
      const sg::ScopedPauseTiming guard {state};
      scratch.shrink_to_fit();
    }
    sg::doNotOptimize(scratch.front());
  }
}

SG_BENCHMARK(bmPaused).iterations(3);
// sg-example:end paused

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
