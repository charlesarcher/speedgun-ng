# Data Model: Harness Timing Modes

Phase 1 output for `specs/017-harness-timing-modes/spec.md`. The entities
are the Key Entities of the spec, plus the records the run and the report
move between them. Every anchor is the audit-point line, and each `+`
marked field is new in this spec.

## E-01: Timing selection (the family record, extended)

`RegistryEntry` (`source/harness/detail/internal.hpp:43`) holds the
selection for one family, settled before the first run of that family
(FR-004, FR-005, FR-006, FR-015).

| Field | Type | Source |
| --- | --- | --- |
| `processCpuTime` | `bool`, default `false` | + H3 C-2; `measureProcessCpuTime` sets it (FR-004) |
| `realTime` | `bool`, default `false` | + H3 C-3; `useRealTime` sets it (FR-005) |
| `manualTime` | `bool`, default `false` | + H3 C-4; `useManualTime` sets it (FR-006) |
| `timeUnit` | `TimeUnit`, written at entry creation from the process default | + H3 C-6; `unit` overwrites it (FR-015, FR-016) |
| `name`, `callable`, the run-control optionals, `metrics`, `args`, `argNames`, `rangeMultiplier`, `setup`, `teardown`, `fixtureFactory`, `runStarted` | H1 and H2 fields | unchanged (FR-018) |

Relationships: the `BenchmarkHandle` writes these fields through `m_entry`
before the run starts, and `Runner::run` reads them once per benchmark
when it builds the run plan.

Validation: a setter after the run starts is the existing contract
violation (`source/harness/registry.cpp:59` and its siblings). `realTime`
and `manualTime` cannot both stand: the second of the two calls is a
precondition violation (FR-008, research D-04). The three fields are
family state, so every instance of the family shares one selection
(FR-004, FR-015).

## E-02: Time unit

`TimeUnit` is a scoped enumeration in namespace `sg` in
`include/speedgun-ng/benchmark.hpp` (FR-015, FR-016).

| Enumerator | Label | Divisor applied to a stored nanosecond value |
| --- | --- | --- |
| `NANOSECONDS` | `ns` | 1 |
| `MICROSECONDS` | `us` | 1000 |
| `MILLISECONDS` | `ms` | 1000000 |
| `SECONDS` | `s` | 1000000000 |

| Piece | Type | Source |
| --- | --- | --- |
| process default | one value in `source/harness/registry.cpp`, read at entry creation | + H3 C-6 (FR-016) |
| `setDefaultTimeUnit(TimeUnit)` | free function | + H3 C-6 (FR-016) |
| `getDefaultTimeUnit()` | free function | + H3 C-6 (FR-016) |

Relationships: the default is captured into E-01 at registration, the
entry's value reaches `BenchmarkResult` (E-06) through the runner, and
`report.cpp` reads it for the scaling and the label.

Validation: the unit is a scoped enumeration, so a value outside the four
enumerators cannot be named by a caller. A default set after registration
leaves an earlier benchmark at its captured unit (spec Edge Cases,
FR-016). The stored values stay nanoseconds; only the printed form scales
(FR-015, research D-12).

## E-03: Pause interval and the state timing record

`State` (`include/speedgun-ng/benchmark.hpp:161`) gains the pause
bookkeeping (FR-009, FR-011, FR-018).

| Field | Type | Source |
| --- | --- | --- |
| `m_pauseDepth` | `std::uint8_t`, 0 or 1 | + H3 C-5; the FR-011 guard |
| `m_pausePairs` | `std::vector<std::pair<std::uint64_t, std::uint64_t>>` | + H3 C-5; sample ordinals inside the run, zero-based |
| `m_pauseCount` | `std::uint64_t` | + H3 C-5; the FR-014 allowance guard |
| `m_samplesTaken` | `std::uint64_t` | + H3 C-5; the run's sample count, printed by FR-013 |
| `m_manualNs` | `std::int64_t` | + H3 C-4; the manual accumulator (FR-007) |

| Method | Contract |
| --- | --- |
| `pauseTiming()` | `\pre` a run state, an open window, `m_pauseDepth == 0`, `m_pauseCount < kMaxPausePerRun`; `\post` one more sample stands in the run's span and the pause depth is 1 (FR-009, FR-011, FR-014) |
| `resumeTiming()` | `\pre` a run state, `m_pauseDepth == 1`; `\post` one more sample stands, the pair of the interval is recorded, and the depth is 0 (FR-009, FR-011) |
| `setIterationTime(std::int64_t)` | `\pre` a run state; `\post` `m_manualNs` carries the sum of every accepted value (FR-007) |
| `closeWindow()` | `\invariant`-shaped check at loop exit: `SG_ASSERT(m_pauseDepth == 0, ...)` (FR-011) |
| `topUpWindow()` | the exception and skip path closes an open pause pair before the exit sample (research D-11) |

Relationships: the pairs are ordinals inside one run's sample span, so
`State` names no recorder layout; the runner adds the run's base point
when it folds (E-07). `ScopedPauseTiming` holds a `State&`, calls
`pauseTiming` in the constructor and `resumeTiming` in the destructor,
and skips the destructor call when the run was skipped (FR-012).

State transitions of `m_pauseDepth`:

```text
0 --pauseTiming--> 1 --resumeTiming--> 0
1 --pauseTiming--> SG_REQUIRE violation (FR-011)
0 --resumeTiming--> SG_REQUIRE violation (FR-011)
1 --loop exit-----> SG_ASSERT violation (FR-011)
```

Validation: the three breach paths above are contract violations reported
through the contract facility, and the process ends (FR-011, Q-6). In a
callback or fixture-hook state, `pauseTiming`, `resumeTiming`, and
`setIterationTime` are `SG_REQUIRE` violations, the R-05 rule of H2
extended (research D-15).

## E-04: Manual accumulator

One value per run, fed by the benchmark and read by the runner
(FR-007).

| Piece | Rule |
| --- | --- |
| accepted value | a nanosecond count, one call per iteration of the benchmark's own timing (FR-007) |
| accumulation | `m_manualNs += nanoseconds`, no count check (research D-05) |
| reported real column | `m_manualNs / iterations` (FR-007) |
| decision value | `m_manualNs`, the value the growth rule reads (FR-007) |
| fivefold stop | exempt for a manual run (FR-006) |
| no accepted value | the sum stays 0, the column shows 0, and the run qualifies at the iteration cap (spec Edge Cases, FR-007) |

Validation: the accumulator belongs to a run state, so a call in a
callback state is a precondition violation (research D-15). A benchmark
that selects manual time and never calls the method is legal and reports
zero (spec Edge Cases).

## E-05: Run record (extended)

`RunRecord` (`source/harness/detail/calibration.hpp:24`) carries one
run's measured values to the growth rule, the stop tests, and the row
builder.

| Field | Type | Source |
| --- | --- | --- |
| `iterations` | `std::uint64_t` | H1, unchanged |
| `basePoint` | `std::size_t` | + H3; the run's first recorder point, replacing the pair index `pair` at `runner.cpp:330` |
| `samples` | `std::uint64_t` | + H3; the run's sample count, printed by FR-013 |
| `pausePairs` | `std::vector<std::pair<std::uint64_t, std::uint64_t>>` | + H3; the ordinals from E-03 |
| `realNs` | `std::int64_t` | H1 name; the netted monotonic fold, or the manual sum under FR-007 |
| `cpuNs` | `std::int64_t` | + H3 C-1; the netted fold of the chosen CPU leaf |
| `decisionNs` | `std::int64_t` | H1 name; the primary time: manual sum, real fold, or CPU fold (FR-002) |
| `manual` | `bool` | + H3; the fivefold exemption of FR-006 |
| `skipped`, `reason` | H1 | unchanged |

Relationships: `nextIterationCount`, `qualifies`, and the fivefold bound
read `decisionNs`; `appendMeasuredRow` reads `basePoint`, `samples`, and
`pausePairs` to net every column.

Validation: the growth rule and the stop tests keep the H1 rule, with the
manual exemption of FR-006 added. A run that ends through `skipWithError`
or an exception keeps the H1 outcome path.

## E-06: Result row and benchmark result (extended)

`ResultRow` (`include/speedgun-ng/benchmark.hpp:110`) and
`BenchmarkResult` (`benchmark.hpp:131`) gain the time fields FR-018
states (FR-001, FR-013, FR-015, FR-018).

| Field | Type | Source |
| --- | --- | --- |
| `timePerIterationNs` | `double` | H1 name kept; the real-time column, the manual sum under FR-007 |
| `cpuTimePerIterationNs` | `double` | + H3 C-1; the CPU column of the chosen leaf |
| `endpointSamples` | `std::uint64_t` | + H3 C-1, C-5; the count FR-013 prints |
| `timeAvailability` | `sg::counters::Availability` | + H3; the availability of the primary fold, the gap rule of the Edge Cases |
| `overheadFloorNs`, `iterations`, `metrics`, `outcome`, `message` | H1 | unchanged |
| `BenchmarkResult::timeUnit` | `TimeUnit` | + H3 C-6; the entry's unit, read by the report |

Relationships: the runner fills the fields from E-05 and E-01, and
`report.cpp` scales the two time values, the overhead floor, and the
aggregate line by E-02's divisor.

Validation: a row whose `timeAvailability` is `GAP` stays out of the
aggregate time statistics, the H1 gap rule carried to the time column
(spec Edge Cases, research D-13). A failed run keeps the H1 `FAILED` row
shape with no time values.

## E-07: Sample span and recorder capacity

One run owns one contiguous block of recorder points (FR-003, FR-009,
FR-013, FR-014).

| Piece | Rule |
| --- | --- |
| span | `basePoint` to `basePoint + samples - 1` |
| no pause | `samples == 2`, the H1 pair, and the row prints `endpoint samples=2` (FR-003, SC-004) |
| one interval | `samples == 4`, and the row prints `endpoint samples=4` (FR-013) |
| netted value | `fold(base, last) − Σ fold(base + p0, base + p1)` per core (FR-010) |
| point cursor | the runner advances by `samples`, replacing the `++nextPair` of `runner.cpp:372-380` |
| capacity | `2 * (1 + kMaxPausePerRun) * (kRunBound + kRunBound + repetitions)` points, the H1 rule of `runner.cpp:307-308` extended (FR-014) |
| allowance | `kMaxPausePerRun = 8` intervals per run, enforced in `pauseTiming` before the sample (FR-014) |

Validation: a pause count above the allowance is a precondition violation
named before the recorder overruns (FR-014). The recorder is a hard-stop
handle, so the guard is what keeps a point from being lost. A fold needs
`i < j`, which every recorded pair satisfies by construction.
