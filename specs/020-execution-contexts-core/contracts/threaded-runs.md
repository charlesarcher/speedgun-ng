# Contract: Threaded runs on contexts

The Google Benchmark thread calls ride the context core: the harness
creates N contexts, hands each thread's body to its context through
the context surface and the barrier, and publishes one merged row
(FR-016 to FR-023).

## The handle surface (FR-016, FR-017, FR-021)

```cpp
// BenchmarkHandle, behind the standing setter precondition (FR-024)
auto threads(std::int32_t count) -> BenchmarkHandle&;
auto threadRange(std::int32_t minCount, std::int32_t maxCount)
    -> BenchmarkHandle&;                            // doubling counts
auto denseThreadRange(std::int32_t minCount, std::int32_t maxCount,
                      std::int32_t step) -> BenchmarkHandle&;
auto threadRunner(ThreadRunnerFn runner) -> BenchmarkHandle&;

class ThreadRun                       // FR-021: the harness-owned view
{                                     // of one threaded attempt
public:
  [[nodiscard]] auto contextCount() const noexcept -> std::int32_t;
  [[nodiscard]] auto context(std::int32_t index) const noexcept
      -> const ExecutionContext&;
  auto dispatch(std::int32_t index) -> void;  // body index to context index
};
using ThreadRunnerFn = std::function<void (ThreadRun&)>;
// FR-021: the runner receives the run's contexts through ThreadRun and
// dispatches the per-thread body nodes itself, one dispatch per index.

// State
[[nodiscard]] auto threadIndex() const noexcept -> std::int32_t;
[[nodiscard]] auto threads() const noexcept -> std::int32_t;
```

| Rule | Requirement |
| --- | --- |
| Each instance a thread call selects shall run on that many contexts, with the instance name carrying a `/threads:N` segment; thread ranges produce one instance per count. | FR-016 |
| The body shall see its thread index and the thread count through `State`; a single-context run reports index `0` and count `1`. | FR-017 |
| Each context of a threaded run shall open its own thread-target plan on its own thread; no fold shall read another context's plan; the fake provider shall script each context's leaves. | FR-018 |
| The run's start barrier and stop barrier shall stand outside the timed window of every context; no barrier wait shall fall inside a window. | FR-019 |
| The published row shall aggregate as: real time the slowest context, cpu time the sum, iterations the per-context count times N, user counters summed before the flag pass, and the flag pass with the real N (research D-06). | FR-020 |
| A function the user supplies may receive the run's `ThreadRun` view and dispatch the per-thread bodies themselves; the harness builds the body nodes, keeps them alive across the runner call, and requires one dispatch per body index before the runner returns, a gap or a duplicate ending the run with an `SG_REQUIRE` violation at that point; the barriers, the plans, and the merge stay the harness's. | FR-021 |
| A benchmark that calls no thread method shall keep its single-context behavior and its printed bytes. | FR-022 |
| `setUp` and `tearDown` shall run once per run on context 0, outside both barriers. | FR-023 |
| A thread count a call cannot honor, zero included, shall end in a contract violation at that call, beside the standing setter precondition. | FR-024 |

## Differences from the cited revision (FR-046)

The pinned revision stands in research D-10: `google/benchmark` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c`. Each kept semantic stands
with its citations, and each difference beside it:

| Semantic | Citations (file and function) | Difference this feature keeps |
| --- | --- | --- |
| One runner per instance over N threads, index and count visible to the body | `src/thread_manager.h`, `ThreadManager`; `src/benchmark_runner.cc`, `RunInThread` | no difference: the harness keeps one runner per instance and the body reads index and count from `State` (FR-017) |
| Start the threads, hold the start signal outside the timed region, join, aggregate per-thread results | `src/benchmark_runner.cc`, `ThreadRunnerDefault::RunThreads` against `ThreadRunnerBase` of `include/benchmark/benchmark_api.h`; `BenchmarkRunner::DoNIterations`; `src/thread_manager.h`, `StartStopBarrier` | the runner thread runs no body: all N bodies are dispatched to library-owned contexts (FR-001, D-05) |
| `AVG_THREADS` divides by the real thread count after the per-thread merge; rate divides by the thread-seconds | `src/counter.cc`, `internal::Finish`; `src/benchmark_runner.cc`, `CreateRunReport` | none: the merged row repeats the cited divisors at the real N (FR-020, D-06) |
| Fixture rule | `include/benchmark/benchmark_api.h`, `Fixture::Run`; `src/benchmark_runner.cc`, `DoOneRepetition` around `DoNIterations` | `setUp` and `tearDown` run once per run on context 0, outside both barriers (FR-023, Q-4, D-07) |
| Barrier mechanism | `src/mutex.h`, `Barrier` (condition-variable rendezvous) | the barrier waits in the arriving context's own wait mode on an atomic counter and generation (FR-014, Q-3, D-03) |

## Consumers

`source/harness/registry.cpp` carries the handle calls;
`source/harness/detail/internal.hpp` carries the family-record
selection; `source/harness/family.cpp` composes the `/threads:N` name;
`source/harness/runner.cpp` runs the threaded attempt and the merge;
`test/source/harness_threads_test.cpp` asserts each rule on scripted
per-context leaves, the `/threads:4` row against the cited aggregation
values, the abort at zero, the barrier-outside-window property, and the
custom runner (SC-01, SC-07, SC-08).
