# Contract: The state timing surface

`State` (`include/speedgun-ng/benchmark.hpp:161`) gains the pause pair,
the manual accumulator, and the scoped guard. The calls stand outside the
timed loop body: the benchmark body calls them, and the loop shape of H1
FR-004 and FR-017 stays as it is.

## The calls (FR-007, FR-009)

```cpp
// State members
auto pauseTiming() -> void;
auto resumeTiming() -> void;
auto setIterationTime(std::int64_t nanoseconds) -> void;
```

| Rule | Requirement |
| --- | --- |
| `pauseTiming` shall stop the timed window from that point, and `resumeTiming` shall restart it. | FR-009 |
| Each call shall cost one sampling action of the run's plan, the same action the endpoint pair costs. | FR-009 |
| A run with no pause shall add no sample pair and no bookkeeping to the loop. | FR-003 |
| `setIterationTime` shall accept one iteration time per iteration as a nanosecond count. | FR-007 |
| The cited revision's `PauseTiming`, `ResumeTiming`, and `SetIterationTime` (`include/benchmark/state.h:60-72`, `:84`) set the behavior; the names take the V.1 spelling. | reference read, FR-019 |

## The contract blocks (FR-011, FR-020)

```cpp
auto pauseTiming() -> void;
//   \pre  this state belongs to a run, its window is open and not closed,
//         no pause is open, and its pause count stands below
//         kMaxPausePerRun
//   \post one further sample stands in the run's span and one pause is open
auto resumeTiming() -> void;
//   \pre  this state belongs to a run and one pause is open
//   \post one further sample stands in the run's span, the interval of the
//         closed pause is recorded, and no pause is open
auto setIterationTime(std::int64_t nanoseconds) -> void;
//   \pre  this state belongs to a run
//   \post the manual accumulator carries the sum of every accepted value
```

| Breach | Enforcement | Requirement |
| --- | --- | --- |
| `pauseTiming` while a pause is open | `SG_REQUIRE` in `pauseTiming` | FR-011 |
| `resumeTiming` with no open pause | `SG_REQUIRE` in `resumeTiming` | FR-011 |
| a pause still open at loop exit | `SG_ASSERT` in `closeWindow()` (`benchmark.hpp:448`) | FR-011 |
| a pause count above `kMaxPausePerRun` | `SG_REQUIRE` in `pauseTiming`, before the sample | FR-014 |
| any of the three calls in a callback or fixture-hook state | `SG_REQUIRE`, the R-05 rule of H2 extended | FR-011, research D-15 |

Each breach is a contract violation reported through the contract
facility, and the process ends: the same reporting path as every other
breach of this repository (FR-011, Q-6).

## The scoped guard (FR-012)

```cpp
class SPEEDGUN_NG_EXPORT ScopedPauseTiming
{
public:
  explicit ScopedPauseTiming(State& state);
  ~ScopedPauseTiming();
};
```

| Rule | Requirement |
| --- | --- |
| The constructor shall call `state.pauseTiming()`. | FR-012 |
| The destructor shall call `state.resumeTiming()`. | FR-012 |
| The destructor shall skip the resume when the run was skipped, so the guard cannot turn a skip into a second breach. | FR-012 |
| The guard shall cover the region from its construction to the end of its scope. | FR-012 |
| The guard is a class in namespace `sg` under the N-1 name; no new macro enters. | FR-012, FR-019 |
| The cited revision has no such guard; its `PauseTiming` and `ResumeTiming` stand as a pair in the body. | reference read |

## The loop rule (FR-003)

| Piece | Placement | Why |
| --- | --- | --- |
| `pauseTiming`, `resumeTiming`, `setIterationTime` | the benchmark body, inside the loop or outside it | the calls are the benchmark's own, so the cursor keeps no new member |
| the pause-open check | `closeWindow()`, the `[[gnu::cold]]` path the cursor takes at count zero | the check stands outside the shortest backward jump `test/loop_shape.sh` measures |
| the exception and skip path | `topUpWindow()` (`benchmark.hpp:456`) closes an open pause pair, then takes the exit sample | every recorded pair needs two points before a fold reads it (research D-11) |

## The reported effect (FR-010)

A paused interval is one recorder pair, and every column subtracts it:

```text
reported = fold(base, base + samples - 1) − Σ fold(base + p0, base + p1)
```

The rule covers the real column, the CPU column, and every attached
metric column, so an attached metric excludes paused work (FR-010, Q-4).
A fold that reports a gap disclosure makes the row's primary time carry
that availability, and the aggregate statistics skip the row (spec Edge
Cases, research D-13).
