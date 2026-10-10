#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto table() -> std::vector<std::uint64_t>&
{
  static std::vector<std::uint64_t> values(1 << 12, 1);
  return values;
}

auto bmSum(sg::State& state) -> void
{
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t value : table()) {
      total += value;
    }
    sg::doNotOptimize(total);
  }
}

auto bmTouch(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(table().front());
  }
}

// FR-027: an argument family of arity two. Each position carries an
// `argNames` label, so its instance names read `bmArgs/width:N/depth:M`.
auto bmArgs(sg::State& state) -> void
{
  const std::size_t cells = static_cast<std::size_t>(state.range(0))
      * static_cast<std::size_t>(state.range(1));
  std::vector<std::uint64_t> values(cells, 1);
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t value : values) {
      total += value;
    }
    sg::doNotOptimize(total);
  }
}

// FR-027: the function template `SG_BENCHMARK_TEMPLATE` instantiates
// over one type argument.
template<class Value>
auto bmFill(sg::State& state) -> void
{
  std::vector<Value> values(64);
  for (auto _ : state) {
    for (Value& value : values) {
      value = Value {1};
    }
    sg::doNotOptimize(values);
  }
}

// FR-027: the fixture whose object one run builds: the harness runs the
// `setUp` and `tearDown` pair around the method in the untimed region
// (FR-017).
class TableFixture : public sg::Fixture
{
public:
  auto setUp(sg::State&) -> void override { m_values.assign(64, 1); }

  auto tearDown(sg::State&) -> void override { m_values.clear(); }

protected:
  std::vector<std::uint64_t> m_values;
};

}  // namespace

SG_BENCHMARK(bmTouch);
SG_BENCHMARK_TEMPLATE(bmFill, std::uint64_t);

// FR-013: the chained registration site. The macro yields the handle,
// the family calls chain after it, and the site ends with ';'.
SG_BENCHMARK(bmArgs).argNames({"width", "depth"}).args({8, 16}).args({16, 32});

// FR-027: the fixture method, registered by `SG_BENCHMARK_F` as the
// instance `TableFixture/bmTableTouch`, whose suite is the fixture class
// (FR-016, FR-021).
SG_BENCHMARK_F(TableFixture, bmTableTouch)(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.back());
  }
}

auto main(int argc, char** argv) -> int
{
  // The catalog freezes at its first use, so a suite that resolves a
  // counter itself registers the providers it needs first. The entry
  // point registers them for every other suite (R-03).
  auto& system = sg::counters::System::local();
  (void)system.registerProvider(
      std::make_unique<sg::counters::ClockProvider>());
  (void)system.registerProvider(std::make_unique<sg::counters::PmuProvider>());

  auto handle = sg::registerBenchmark(&bmSum, "bmSum");
  if (!handle.name().empty()) {
    const auto cpu = system.object("cpu");
    if (cpu.has_value()) {
      const auto instructions =
          cpu->counter<sg::counters::Dim<0, 1>>("instructions");
      const auto cycles = cpu->counter<sg::counters::Dim<0, 1>>("cpu-cycles");
      if (instructions.has_value() && cycles.has_value()) {
        handle.addMetric(*instructions / *cycles, "instructions/cycle");
      }
    }
  }

  // FR-020: the prefix gate, visible in the same listing. The family
  // registers and expands, and selection drops it, so list mode prints no
  // such name and a run selects nothing that is disabled.
  (void)sg::registerBenchmark(&bmTouch, "DISABLED_bmTouch");

  return sg::speedgunMain(argc, argv);
}
