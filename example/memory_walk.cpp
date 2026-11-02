// The memory-bound case the acceptance run of specs/042-topdown asks for
// (FR-058, SC-10): a chase over a working set that stands above the
// last-level cache, so every iteration waits on memory.
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

// The working set: eight million four-byte cells make 32 MiB, a height above
// the 19.3 MiB last-level cache of the acceptance host. The largest set a
// shipped example holds measures 64 KiB (`example/docs/throughput.cpp:25`).
constexpr std::size_t kCells = std::size_t {1} << 23;

auto shuffledCells() -> std::vector<std::uint32_t>
{
  std::vector<std::uint32_t> cells(kCells);
  std::iota(cells.begin(), cells.end(), std::uint32_t {0});
  auto engine = std::mt19937_64(std::uint64_t {0x4257});
  std::shuffle(cells.begin(), cells.end(), engine);
  return cells;
}

// The chase order: the cell indices in the order one seeded shuffle gives
// them, so the next address stands far from the last one and a prefetcher
// reads no step ahead. The seed is a constant, so every run walks the same
// order. The table stands in one static object, so the shuffle runs once.
auto chaseOrder() -> const std::vector<std::uint32_t>&
{
  static const std::vector<std::uint32_t> kOrder = shuffledCells();
  return kOrder;
}

}  // namespace

// One load per iteration, and the value it reads names the cell the next load
// reads, so the loads cannot overlap and each one waits for its own miss. The
// walk reaches nothing else, so the barrier stands after the loop on the cell
// the walk ends on, the shape docs/pages/barrier-rule.md states.
auto bmMemoryWalk(sg::State& state) -> void
{
  const std::vector<std::uint32_t>& order = chaseOrder();
  std::uint32_t cell = 0;
  for (auto _ : state) {
    cell = order[cell];
  }
  sg::doNotOptimize(cell);
  const auto loads = static_cast<std::int64_t>(state.iterations());
  const auto bytes = static_cast<std::int64_t>(sizeof(std::uint32_t));
  state.setItemsProcessed(loads);
  state.setBytesProcessed(loads * bytes);
}

SG_BENCHMARK(bmMemoryWalk);

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
