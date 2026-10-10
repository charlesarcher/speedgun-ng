# Contract: The sample span and the recorder capacity

One run owns one contiguous block of recorder points. H1 sized that block
at two points; H3 sizes it at two points plus two per paused interval,
and the capacity rule has to cover the largest block a run can take
(FR-003, FR-009, FR-013, FR-014).

## The span (FR-003, FR-009, FR-013)

| Piece | Rule |
| --- | --- |
| entry sample | `openWindow()` (`benchmark.hpp:426`) takes it, unchanged from H1 |
| exit sample | `takeExitSample()` (`benchmark.hpp:437`) takes it, unchanged from H1 |
| pause sample | `pauseTiming()` takes one, and `resumeTiming()` takes one |
| span | `basePoint` to `basePoint + samples - 1` |
| printed count | `endpoint samples=<samples>`, the run's own count |
| a run with no pause | `samples == 2`, the H1 pair, and no bookkeeping in the loop (SC-004) |

## The fold (FR-010)

```text
net(core) = foldCore(core, view, base, base + samples - 1)
            − Σ foldCore(core, view, base + p0, base + p1)
```

| Rule | Requirement |
| --- | --- |
| One helper in `source/harness/runner.cpp` nets the real core, the CPU core, and every metric core. | FR-010, research D-08 |
| A pause pair needs `p1 == p0 + 1`, which the two calls guarantee, so each inner fold satisfies the window rule the public fold documents at `counters_measurement.hpp:669`. | FR-010 |
| The availability of the netted value is the worst availability of the folds it reads, so a gap on a pause pair reaches the row. | spec Edge Cases |
| The cited revision accumulates the running segments the same way: a pause closes the open segment and a resume opens the next one (`src/thread_timer.h:23-38`, `src/benchmark.cc:239-255`). | reference read |

## The point cursor (FR-013)

| Piece | H1 | H3 |
| --- | --- | --- |
| runner cursor | `std::uint64_t nextPair` at `runner.cpp:311`, advanced by `++nextPair` at `:372`, `:376`, `:380` | a point cursor advanced by the samples that run took |
| fold index | `2 * record.pair` and `2 * record.pair + 1` at `runner.cpp:118` | `record.basePoint` and `record.basePoint + record.samples - 1` |
| `State` comment | "A run owns exactly one point pair" at `benchmark.hpp:454-455` | the comment states the span rule and the allowance |

## The capacity rule (FR-014)

```text
H1, runner.cpp:307-308
  capacity = 2 * (kRunBound + kRunBound + repetitions)

H3
  capacity = 2 * (1 + kMaxPausePerRun) * (kRunBound + kRunBound + repetitions)
  kMaxPausePerRun = 8
```

| Rule | Requirement |
| --- | --- |
| The capacity shall cover the pause pairs of every run the phase rule can start. | FR-014 |
| `kMaxPausePerRun` shall be one value in namespace `sg` in `include/speedgun-ng/benchmark.hpp`, read by the guard in the header and the capacity rule in `runner.cpp`, the pattern `kDefaultRangeMultiplier` at `benchmark.hpp:42` sets. | FR-014 |
| `pauseTiming` shall carry `SG_REQUIRE(m_pauseCount < kMaxPausePerRun, ...)` before it samples, so the breach is named before the recorder loses a point. | FR-014 |
| The recorder is a hard-stop handle (`counters_measurement.hpp:507`), so an uncovered overrun would drop points without a diagnostic. | FR-014 |
| The allowance shall bound one run at 18 points, so the 193 runs of the H1 rule need 3474 points per leaf. | FR-014, plan R-07 |

## The exception and skip path

| Piece | Rule |
| --- | --- |
| `topUpWindow()` (`benchmark.hpp:456`) | closes an open pause pair with one sample, then takes the exit sample |
| why | the run is already ending in error, and every recorded pair needs two points before a fold reads it (research D-11) |
| the loop discipline of FR-017 | unchanged: the runner still checks the loop rules after the callable returns, and the pause-open check stands in `closeWindow()` on the normal path |
