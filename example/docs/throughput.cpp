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
    sg::doNotOptimize(buffer);
  }
  const auto total = static_cast<std::int64_t>(state.iterations())
      * static_cast<std::int64_t>(items);
  state.setItemsProcessed(total);
  state.setBytesProcessed(total);
}

SG_BENCHMARK(bmEncode).argNames({"items"}).argsProduct({{1024, 65536}});

// sg-example:end throughput

// sg-example:begin throughput-call
auto bmScale(sg::State& state) -> void
{
  std::int64_t work = 0;
  for (auto _ : state) {
    work += static_cast<std::int64_t>(state.iterations());
    sg::doNotOptimize(work);
  }
  state.setItemsProcessed(work);
  state.setBytesProcessed(work * 4);
}

SG_BENCHMARK(bmScale).argsProduct({{1}}).repetitions(3);

// sg-example:end throughput-call

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
