#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin fixture
class TableFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override
  {
    m_values.assign(3, 1);
    sg::doNotOptimize(&state);
  }

  auto tearDown(sg::State& state) -> void override
  {
    m_values.clear();
    sg::doNotOptimize(&state);
  }

protected:
  std::vector<std::uint64_t> m_values {1, 2, 3};
};
// sg-example:end fixture

SG_BENCHMARK_F(TableFixture, bmTableRead)(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.front());
  }
}

SG_BENCHMARK_DEFINE_F(TableFixture, bmTableSize)(sg::State& state)

{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.size());
  }
}

SG_BENCHMARK_REGISTER_F(TableFixture, bmTableSize)
    .setup([](sg::State&) {})
    .teardown([](sg::State&) {});

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
