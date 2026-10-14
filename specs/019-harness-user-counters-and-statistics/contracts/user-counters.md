# Contract: The user counter surface

`State` (`include/speedgun-ng/benchmark.hpp:208`) gains the counter
calls, and the `sg` namespace gains `Counter`, `CounterFlag`,
`CounterBase`, `kMaxUserCounters`, and `kMaxCounterNameBytes`. Each call
runs inside a timed loop or a body callback state of the run (FR-001,
FR-009).

## The calls (FR-001, FR-009)

```cpp
// State members
auto setCounter(std::string_view name, const Counter& counter) -> void;
auto setBytesProcessed(std::int64_t bytes) -> void;
auto setItemsProcessed(std::int64_t items) -> void;

// The value type
class Counter
{
public:
  explicit Counter(double value = 0.0,
                   CounterFlag flags = CounterFlag::NONE,
                   CounterBase base = CounterBase::K1000);
};
```

| Rule | Requirement |
| --- | --- |
| `setCounter` shall set or open the named counter with the value, flags, and base of the argument. | FR-001, FR-002 |
| `setBytesProcessed` shall set `bytes_per_second` with the `RATE` flag and the `K1024` base. | FR-009 |
| `setItemsProcessed` shall set `items_per_second` with the `RATE` flag and the `K1000` base. | FR-009 |
| The printed value of a counter shall divide by the declared base, as the cited console scales by `oneK`. | FR-006; `src/console_reporter.cc:196` |

## The bounds (FR-001, FR-008)

| Rule | Requirement |
| --- | --- |
| A benchmark shall carry at most `kMaxUserCounters` (16) named counters; a further name fails the run with a reason stating the cap. | FR-001 |
| A name longer than `kMaxCounterNameBytes` minus one shall fail the run with a reason stating the bound. | research D-01 |
| A second `setCounter` for a used name shall carry equal flags and base; a difference fails the run naming that name. | FR-008 |
| A counter name equal to an attached metric label shall fail the run naming the collision. | spec Edge Cases |

## The flag pass (FR-003, FR-004, FR-005)

| Rule | Requirement |
| --- | --- |
| The flags shall apply after the loop, once per repetition, in the order rate, thread average, iteration-invariant, average-over-iterations, invert. | FR-003 |
| The rate divisor shall be the row's primary time in seconds: the CPU fold, the real fold under `useRealTime`, the manual sum under `useManualTime`. | FR-004 |
| The thread-average divisor shall be one while threads are out of scope. | FR-004, Q-3 |
| A row whose primary fold carries a disclosure or a zero divisor shall print the flag-neutral value with the disclosure, and shall stand aside from that counter's aggregate statistics. | FR-005 |
| The aggregate row shall carry one statistic line per named counter over the clean repetitions. | FR-006 |

## The loop rule (FR-007)

The store stands in fixed storage with inline name buffers, so
`setCounter` allocates nothing (research D-01). `test/loop_shape.sh`
compiles a body that calls `setCounter` inside `for (auto _ : state)` and
finds no allocation call and no out-of-line call inside the backward
jump beyond the body's own call, keeping the gate green (FR-007).

## Consumers

`test/source/harness_user_counters_test.cpp` and
`test/source/harness_throughput_test.cpp` assert each rule against
scripted points; `test/source/harness_statistics_test.cpp` keeps the
built-in aggregate fields byte-stable beside the new lines (SC-01,
SC-02).
