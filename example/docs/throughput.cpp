#include <cstddef>
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin throughput
auto bmEncode(sg::State& state) -> void
{
  const auto items = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint8_t> buffer(items);
  for (auto _ : state) {
    for (std::size_t item = 0; item < items; ++item) {
      buffer[item] = static_cast<std::uint8_t>(item * 7);
    }
    state.setItemsProcessed(static_cast<std::int64_t>(items));
    state.setBytesProcessed(static_cast<std::int64_t>(items));
    sg::doNotOptimize(buffer);
  }
}

SG_BENCHMARK(bmEncode).argNames({"items"}).argsProduct({{1024, 65536}});
// sg-example:end throughput

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
