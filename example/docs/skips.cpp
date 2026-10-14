#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin skips
auto bmGuard(sg::State& state) -> void
{
  if (state.range(0) == 0) {
    state.skipWithMessage("no cells configured");
    return;
  }
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0));
  }
}

SG_BENCHMARK(bmGuard).denseRange(0, 4, 4);

// sg-example:end skips

// A fixture instance skipped by its method carries the same pairing, with the
// suite named by the class the macro took.
class CellFixture : public sg::Fixture
{
};

SG_BENCHMARK_DEFINE_F(CellFixture, bmFill)(sg::State& state)
{
  if (state.range(0) == 0) {
    state.skipWithMessage("fixture has no cells");
    return;
  }
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0));
  }
}

SG_BENCHMARK_REGISTER_F(CellFixture, bmFill).arg(0).arg(4);

// example-run: expect-skip bmGuard/0: no cells configured
// example-run: expect-skip CellFixture/bmFill/0: fixture has no cells

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
