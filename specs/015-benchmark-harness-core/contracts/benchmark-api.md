# Contract: the public benchmark API

Phase 1 output. The surface of `include/speedgun-ng/benchmark.hpp` and
`include/speedgun-ng/barrier.hpp`, namespace `sg`. Every declaration
carries doxygen `\pre`, `\post`, and `\invariant`, enforced through the
`dbc` macros (FR-053); this file records the contract each carries.

## Registration (FR-001, FR-002, FR-003)

```cpp
auto registerBenchmark(std::function<void(State&)> fn,
                       std::string_view name) -> BenchmarkHandle;

#define SG_BENCHMARK(fn)   // namespace-scope object; registers fn
                           // before main, under the name of fn
```

- `registerBenchmark` shall accept any callable from code that runs
  before the run starts.
- A repeated name shall report a recoverable error at the 007 FR-046
  tier, keep the first registration, and leave the rest runnable.
- The returned handle shall expose the run-control setters
  (`minTime`, `warmupTime`, `repetitions`, `iterations`) and
  `addMetric` (FR-003, FR-021).
- `\pre` for `addMetric`: the expression's leaves resolve through the
  system catalog; a miss is a recoverable error naming the leaf.

## State (FR-004, FR-005, FR-031)

```cpp
class State {
public:
  [[nodiscard]] auto iterations() const noexcept -> std::uint64_t;
  auto begin() noexcept;   // range-for cursor over the timed loop
  auto end() noexcept;
  auto skipWithError(std::string_view reason) noexcept -> void;
  auto skipWithMessage(std::string_view reason) noexcept -> void;
};
```

- The harness shall call the benchmark function with a `State&`; the
  function runs setup, the timed loop as `for (auto _ : state)`, and
  teardown (FR-004).
- `iterations()` shall report the iteration count of the current run
  (FR-005).
- A skip shall end the timed loop, record the reason, and suppress the
  statistics for that benchmark (FR-031).
- `\invariant`: the range-for cursor advances only inside the timed
  loop, and the loop reads no interrupt flag (R-06).

## Run-control precedence (FR-007, FR-013, FR-015)

- The defaults shall be: minimum time 0.5 s; minimum warm-up time 0 s;
  repetitions 1; calibration start 1 iteration (FR-007, D-3).
- A value set on the handle shall win over the command line; a
  command-line value applies where the benchmark sets none (FR-015).
- `iterations(n)` shall skip calibration and run n iterations in each
  measured run. A set warm-up time still runs warm-up from n, grown by
  the FR-010 rule within the FR-016 bound. The harness discards the
  warm-up results (FR-011, FR-013).

## Result value (FR-036)

```cpp
enum class RunOutcome : std::uint8_t { MEASURED, SKIPPED, FAILED };

struct MetricValue {
  double value;
  double runningRatio;
  bool scaled;
  sg::counters::Availability availability;
};

struct ResultRow {
  // kind: repetition or aggregate; aggregates appear only when
  // repetitions exceed 1 (FR-027, FR-035)
  std::uint64_t iterations;          // the N of the measured run
  double timePerIterationNs;         // the monotonic fold / N (FR-020)
  std::vector<MetricValue> metrics;  // one per metric (FR-024)
  double overheadFloorNs;            // Plan::sampleOverheadNsMedian()
};

struct BenchmarkResult {
  std::string name;
  RunOutcome outcome;
  std::vector<ResultRow> rows;   // repetitions, then aggregates
  std::string reason;            // skip or failure text
};
```

- The harness shall expose the results as this value, and the console
  report shall format it and add nothing to it (FR-036).
- Each `MetricValue` shall carry the running ratio, the scaled flag,
  and the gap state beside its value (FR-024).
- Every `ResultRow` shall carry `overheadFloorNs` beside its values
  (FR-026); aggregate rows append after the repetition rows (FR-027,
  FR-035).

## Barriers (FR-029, FR-055)

```cpp
template<class T>
auto doNotOptimize(T&& value) noexcept -> void;   // one extended-asm
                                                  // statement per
                                                  // overload (D-4)

auto clobberMemory() noexcept -> void;            // std::atomic_signal_fence
```

- The semantics shall match the Google Benchmark barriers at the D-3
  revision; the const-reference overload Google Benchmark marks
  deprecated stays out (D-4).
- The documentation shall state the Principle X.2 rule: a barrier
  stands only where the compiler would otherwise eliminate the
  measured work (FR-030).
- The contract deviation (no runtime `SG_ENSURE`; the codegen gate
  `test/barrier_shape.sh` enforces the property) is recorded in the
  plan's Complexity Tracking.

## Entry point (FR-033)

```cpp
auto speedgunMain(int argc, char** argv) -> int;
```

- A suite shall link `speedgun-ng::harness` and call `speedgunMain`
  from `main` (D-1).
- `speedgunMain` shall register `ClockProvider` and treat a
  duplicate-machine refusal as a pass, which is the fake-provider
  substitution of FR-042 (R-03).
- The return value shall follow the exit-status rule of `cli.md`.

## Counters-only rule (FR-038, FR-040)

No declaration in this surface measures time. Every time and counter
value the runner reports comes from a plan, a recorder, a fold, or a
value the plan publishes (FR-038). The time-source gate contract
(`time-source-gate.md`) enforces the banned list over the
implementation.
