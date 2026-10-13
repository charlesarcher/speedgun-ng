// The capability snippets the root page quotes, one region per capability
// the other example files do not carry. The file builds as a target so a
// pasted block cannot rot: each region holds a complete declaration plus
// its own registration, and no region is a fragment.
//
// The entry point never enters the runner, so no family below reaches a
// report or the `run-examples` target: see the comment beside the target in
// example/CMakeLists.txt.

#include <cstdint>
#include <vector>

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

// sg-example:begin registration
auto bmCells(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmCells);

auto bmTable(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kTable = sg::registerBenchmark(&bmTable, "Table/cells");

// sg-example:end registration

// sg-example:begin ranges
auto bmSquares(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) * state.range(0));
  }
}

SG_BENCHMARK(bmSquares)
    .argNames({"cells"})
    .arg(32)
    .args({96})
    .argsProduct({{4, 8}})
    .rangeMultiplier(2)
    .range(64, 512)
    .ranges({{4096, 8192}})
    .denseRange(1024, 2048, 512);

// sg-example:end ranges

// sg-example:begin fixture
class TableFixture : public sg::Fixture
{
protected:
  std::vector<std::uint64_t> m_values {1, 2, 3};
};

SG_BENCHMARK_F(TableFixture, bmTableRead)(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.front());
  }
}

SG_BENCHMARK_DEFINE_F(TableFixture, bmTableSize)(sg::State&) {}

SG_BENCHMARK_REGISTER_F(TableFixture, bmTableSize)
    .setup([](sg::State&) {})
    .teardown([](sg::State&) {});

// sg-example:end fixture

// sg-example:begin run-control
auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmFixed)
    .minTime(200'000'000)  // 200 ms, in nanoseconds
    .warmupTime(50'000'000)
    .iterations(1000)
    .repetitions(5);

// sg-example:end run-control

// sg-example:begin timing-modes
auto bmIo(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmIo).useRealTime().measureProcessCpuTime();

auto bmHand(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1500);
  }
}

SG_BENCHMARK(bmHand).useManualTime();

// sg-example:end timing-modes

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

// sg-example:begin time-unit
auto bmMillis(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kMillis = sg::registerBenchmark(&bmMillis, "Table/millis")
                         .unit(sg::TimeUnit::MILLISECONDS);

auto configureUnits() -> void
{
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);
}

// sg-example:end time-unit

auto bmTick(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

// sg-example:begin metrics
#include "speedgun-ng/counters.hpp"

auto attachMetric(sg::BenchmarkHandle& handle) -> void
{
  const auto cpu = sg::counters::System::local().object("cpu");
  if (!cpu.has_value()) {
    return;
  }
  const auto cycles = cpu->counter<sg::counters::Dim<0, 1>>("cpu-cycles");
  const auto retired = cpu->counter<sg::counters::Dim<0, 1>>("instructions");
  if (cycles.has_value() && retired.has_value()) {
    handle.addMetric(*retired / *cycles, "instructions/cycle");
  }
}

// sg-example:end metrics

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

SG_BENCHMARK(bmGuard).arg(0);

// sg-example:end skips

auto main() -> int
{
  configureUnits();
  auto handle = sg::registerBenchmark(&bmTick, "Table/tick");
  attachMetric(handle);
  return 0;
}
