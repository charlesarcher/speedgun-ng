# speedgun-ng

<p align="center">
  <img src="docs/images/sg.jpg" alt="speedgun-ng" width="280">
</p>

A benchmark framework in the lineage of Google Benchmark, written in
C++23 for Linux. A benchmark is a function over a `State` object: you
write the loop, register the function, and read a report of time per
iteration.

## What speedgun does

- Written in C++23.
- Runs on Linux, the one supported platform.
- Continues the lineage of Google Benchmark.
- Draws every time value and every counter value from the counters library of this project.
- Attaches hardware counter metrics to a benchmark, printed beside the timing columns of the same run.
- Guards its public API with design-by-contract checks at the boundaries.

## Quick start

Linux, a C++23 compiler, and CMake 3.20 or newer. Configure, build, and install the library:

<!-- sg-example: example/quickstart.sh#configure -->
```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
```

<!-- sg-example: example/quickstart.sh#build -->
```sh
cmake --build build -j
```

<!-- sg-example: example/quickstart.sh#install -->
```sh
cmake --install build --prefix "$HOME/.local"
```

The consumer project's `CMakeLists.txt`, whole: the package and the two targets a benchmark links:

<!-- sg-example: example/quickstart/CMakeLists.txt#package -->
```cmake
cmake_minimum_required(VERSION 3.20)

project(speedgun-ngQuickStart CXX)

find_package(speedgun-ng REQUIRED)

add_executable(first_benchmark first_benchmark.cpp)
target_link_libraries(first_benchmark
                      PRIVATE speedgun-ng::speedgun-ng speedgun-ng::harness)
```

The first benchmark program, whole:

<!-- sg-example: example/first_benchmark.cpp#program -->
```cpp
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

constexpr std::uint64_t kCells = 1 << 12;

auto bmSum(sg::State& state) -> void
{
  std::vector<std::uint64_t> cells(kCells, 1);
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t cell : cells) {
      total += cell;
    }
    sg::doNotOptimize(total);
  }
}

}  // namespace

SG_BENCHMARK(bmSum);

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
```

Configure, build, and run it:

<!-- sg-example: example/quickstart.sh#consumer-configure -->
```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
```

<!-- sg-example: example/quickstart.sh#consumer-build -->
```sh
cmake --build build
```

<!-- sg-example: example/quickstart.sh#run -->
```sh
./build/first_benchmark
```

Its output on one host's run, with the numbers of that machine:

<!-- sg-sample-output: example/first_benchmark.expected#report -->
```text
speedgun-ng 0.8.0 build=Release
bmSum                    iterations=   1069106  real time/iter (ns)=  675.230  cpu time/iter (ns)=  674.173  overhead floor (ns)=  688.000  endpoint samples=2
```

## How you do it

### The run loop

A benchmark is a function over `State`: the loop body runs once per iteration, and the harness settles the count it needs.
<!-- sg-example: example/docs_snippets.cpp#run-loop -->
```cpp
auto bmLoop(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
  sg::clobberMemory();
}

[[maybe_unused]] const auto kLoopRegistrar =
    sg::registerBenchmark(&bmLoop, "DISABLED_bmLoop");
```
The row prints the count and the time per iteration. `doNotOptimize` keeps a value live across the barrier, and `clobberMemory` pushes cached values back to memory.

### Registration

`SG_BENCHMARK` registers a function under its own name, `SG_BENCHMARK_CAPTURE` registers one that takes captured arguments, and `SG_BENCHMARK_TEMPLATE` registers one instantiation of a function template; `registerBenchmark` is the form that names the family itself and returns a `BenchmarkHandle` whose calls chain.
<!-- sg-example: example/docs_snippets.cpp#registration -->
```cpp
auto bmNamed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmNamed);

[[maybe_unused]] const auto kNamedRegistrar =
    sg::registerBenchmark(&bmNamed, "DISABLED_Suite/bmNamed").arg(8);
```
A name splits at its first `/` into suite and case, so `SG_BENCHMARK_CAPTURE(bmCapture, wide, 8)` is the case `wide` of the suite `bmCapture` and `"DISABLED_Suite/bmNamed"` is the case `bmNamed` of the suite `DISABLED_Suite`.
`DISABLED_` at the front of the expanded instance name keeps that instance registered, out of every `--filter` match, out of `--list`, and out of the run.

### Argument families and ranges

`arg`, `args`, `range`, `ranges`, `denseRange`, and `argsProduct` build the instance list, and `argName` labels each position.
<!-- sg-example: example/docs_snippets.cpp#ranges -->
```cpp
auto bmRanges(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) + state.range(1));
  }
}

[[maybe_unused]] const auto kRangesRegistrar =
    sg::registerBenchmark(&bmRanges, "DISABLED_bmRanges")
        .argNames({"width", "depth"})
        .rangeMultiplier(2)
        .ranges({{8, 8}, {16, 32}});
```
Each instance prints as its own row, named by the labels.

### Fixtures

A `Fixture` subclass builds what its method measures, with `setUp` and `tearDown` either side of the loop.
<!-- sg-example: example/docs_snippets.cpp#fixture -->
```cpp
class TableFixture : public sg::Fixture
{
public:
  auto setUp(sg::State&) -> void override { m_values.assign(64, 1); }

  auto tearDown(sg::State&) -> void override { m_values.clear(); }

protected:
  std::vector<std::uint64_t> m_values;
};
```
`SG_BENCHMARK_F` defines the method and registers it in one step, `SG_BENCHMARK_DEFINE_F` defines one to register later, and `SG_BENCHMARK_REGISTER_F` registers a defined method and carries the `setup` and `teardown` callbacks that run outside the loop.
The template forms are `SG_BENCHMARK_TEMPLATE_F`, `SG_BENCHMARK_TEMPLATE_DEFINE_F`, and the pair `SG_BENCHMARK_TEMPLATE_METHOD_F` with `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`: a fixture instance is named `FixtureClass/Method`, or `FixtureClass<types>/Method` over a class template, so the fixture class is its suite.

### Run control

`minTime`, `warmupTime`, `iterations`, and `repetitions` bound the measuring window and repeat it.
<!-- sg-example: example/docs_snippets.cpp#run-control -->
```cpp
auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

[[maybe_unused]] const auto kFixedRegistrar =
    sg::registerBenchmark(&bmFixed, "DISABLED_bmFixed")
        .minTime(200'000'000)
        .warmupTime(50'000'000)
        .repetitions(5)
        .iterations(1000);
```
Each repetition prints a row, and more than one ends with an aggregate row.

### Timing modes

Real time is the default. `measureProcessCpuTime` moves the CPU column to process CPU time, and `useManualTime` lets the body report its own time.
<!-- sg-example: example/docs_snippets.cpp#timing-modes -->
```cpp
auto bmHand(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1500);
  }
}

[[maybe_unused]] const auto kHandRegistrar =
    sg::registerBenchmark(&bmHand, "DISABLED_bmHand").useManualTime();
```
`setIterationTime` states that time in nanoseconds.

### Pause and resume

`ScopedPauseTiming` takes a scope out of the measured window, and `pauseTiming` and `resumeTiming` are the manual pair.
<!-- sg-example: example/docs_snippets.cpp#paused -->
```cpp
auto bmPaused(sg::State& state) -> void
{
  std::vector<std::uint64_t> scratch(64, 1);
  for (auto _ : state) {
    {
      const sg::ScopedPauseTiming guard {state};
      scratch.assign(64, 1);
    }
    sg::doNotOptimize(scratch.front());
  }
}

[[maybe_unused]] const auto kPausedRegistrar =
    sg::registerBenchmark(&bmPaused, "DISABLED_bmPaused");
```
Excluded work stays out of both time columns, and each pause adds a sample pair.

### The time unit

`unit` scales the columns of one family; `setDefaultTimeUnit` sets the process default a later registration captures.
<!-- sg-example: example/docs_snippets.cpp#time-unit -->
```cpp
auto bmMillis(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

[[maybe_unused]] const auto kMillisRegistrar =
    sg::registerBenchmark(&bmMillis, "DISABLED_bmMillis")
        .unit(sg::TimeUnit::MILLISECONDS);
```
A registration keeps the unit it captured, so the call order decides.

### Hardware counter metrics

`addMetric` attaches a counter expression to a family, behind the availability check the counter lookup returns.
<!-- sg-example: example/docs_snippets.cpp#metrics -->
```cpp
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
```
The row carries the metric label with `ratio=`, `scaled=`, and `gap=` fields. On a host where the leaf is unavailable the row prints the refusal kind in place of the number and the run continues.

### Skips

`skipWithMessage` ends a run early when the work cannot be done, and `skipWithError` ends it as a failure.
<!-- sg-example: example/docs_snippets.cpp#skips -->
```cpp
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

[[maybe_unused]] const auto kGuardRegistrar =
    sg::registerBenchmark(&bmGuard, "DISABLED_bmGuard").arg(0);
```
The report prints `SKIPPED: <reason>` for the first kind and `FAILED: <reason>` for the second.

### The command line

One entry point, `speedgunMain`, reads the long options; the short letters stay inert because the short-option string is empty.
<!-- sg-example: example/quickstart.sh#options -->
```sh
./build/first_benchmark --list
./build/first_benchmark --filter='bmSum' --min-time=200000000
./build/first_benchmark --counter=machine/monotonic --dry-run
./build/first_benchmark --catalog
```
A usage error reports one line and exits with status 2.

### Reading the report

A run prints a version line, then one row per instance and repetition.
<!-- sg-example: example/first_benchmark.expected#aggregate -->
```text
bmSum                     AGGREGATE  primary time/iter (ns) mean=  656.219 median=  650.596 sd=   10.314 cv=  0.0157 min=  649.938 max=  668.122 n=3
```
A row gives `iterations=`, `real time/iter (ns)=`, `cpu time/iter (ns)=`, `overhead floor (ns)=`, and `endpoint samples=`. The floor is the measured cost of the sampling itself, and the count is the sample pair taken at the window ends. With repetitions, a final `AGGREGATE` row gives mean, median, `sd=`, `cv=`, min, max, and `n=`.

## Quick reference

The API names this page teaches:

| Name | What it does | Teaches |
| --- | --- | --- |
| `SG_BENCHMARK` | Registers one function as one family | [Registration](#registration) |
| `SG_BENCHMARK_CAPTURE` | Registers a function with captured arguments | [Registration](#registration) |
| `SG_BENCHMARK_TEMPLATE` | Registers one instantiation of a template | [Registration](#registration) |
| `SG_BENCHMARK_F` | Defines and registers a fixture method | [Fixtures](#fixtures) |
| `SG_BENCHMARK_DEFINE_F` | Defines a fixture method to register later | [Fixtures](#fixtures) |
| `SG_BENCHMARK_REGISTER_F` | Registers a defined fixture method | [Fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_F` | Defines and registers a template fixture | [Fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_DEFINE_F` | Defines a template fixture method | [Fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_METHOD_F` | Defines one method for many instantiations | [Fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` | Registers one fixture instantiation | [Fixtures](#fixtures) |
| `State` | The object a benchmark loop iterates over | [The run loop](#the-run-loop) |
| `BenchmarkHandle` | The handle a registration returns; chains the options of one family | [Registration](#registration) |
| `Fixture` | Builds state before the loop and releases it after | [Fixtures](#fixtures) |
| `ScopedPauseTiming` | Takes a scope out of the measured window | [Pause and resume](#pause-and-resume) |
| `BenchmarkResult` | One measured run of one instance | [Reading the report](#reading-the-report) |
| `setDefaultTimeUnit` | Sets the time unit a later registration captures | [The time unit](#the-time-unit) |
| `speedgunMain` | Runs the registry against the command line | [The command line](#the-command-line) |
| `doNotOptimize` | Keeps a value live across the barrier | [The run loop](#the-run-loop) |
| `clobberMemory` | Pushes cached values back to memory | [The run loop](#the-run-loop) |

| Option | What it does |
| --- | --- |
| `--filter` | Selects the instances whose names match |
| `--list` | Prints the registry and stops |
| `--repetitions` | Sets how many times one instance runs |
| `--min-time` | Sets the least time one instance spends measuring |
| `--iterations` | Fixes the iteration count and skips calibration |
| `--warmup-time` | Sets the untimed warm-up window |
| `--dry-run` | Runs one iteration and prints the row shape |
| `--counter` | Adds one catalog counter to the report columns |
| `--catalog` | Prints the counter catalog and stops |

## Planned work

- Counters on the state
- Bytes and items throughput
- Labels
- Custom statistics
- Aggregate-only report output
- Complexity fitting
- Structured machine-readable output
- Histogram output

The build page covers the platform matrix, the vendored dependencies, and the quality gates: [Building speedgun-ng](docs/pages/building.md).

The name marks this library as a rethink of Speedgun, a tool its author built at Akuna: it shares no code with that project, and everything here came from the author and a local AI army.
