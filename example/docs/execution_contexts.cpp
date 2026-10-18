#include "speedgun-ng/execution_context.hpp"

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmQuiet);

// sg-example:begin noise-load
const auto kNoiseLoad = [](sg::WorkContext& work) noexcept
{
  while (!work.stopToken().stopRequested()) {
  }
};

auto main(int argc, char** argv) -> int
{
  const sg::ContextPolicy spin {.waitMode = sg::WaitMode::SPIN};
  sg::ExecutionContext load {spin};
  sg::WorkNode<decltype(kNoiseLoad)> node {kNoiseLoad};
  sg::dispatch(load, node);
  const int status = sg::speedgunMain(argc, argv);
  load.requestStop();
  load.drain();
  load.join();
  return status;
}

// sg-example:end noise-load
