# Data Model: Percentiles and Raw Capture

Entities E-01 to E-10. Each carries its fields, the rules that bind it, and
the FR or decision it comes from.

## E-01 Chunk plan

Computed per attempt, before the window opens.

| Field | Type | Rule |
|---|---|---|
| `chunkIterations` | `std::uint64_t` | `max(1, ceil(100 * sampleOverheadNsMedian() / iterationNs))`, the 1% bound of FR-005 (D-03) |
| `chunkCount` | `std::uint64_t` | `min(iterations / chunkIterations, kMaxChunkPointsPerLeaf - 1 - pauseRoom)` (D-03, D-04) |
| `pointCapacity` | `std::size_t` | `chunkCount + 1 + pauseRoom`, the recorder capacity of D-02 |

**Invariants**: `chunkCount >= 1` for a measured run; `pointCapacity <=
kMaxChunkPointsPerLeaf`; `chunkIterations * chunkCount <= iterations`; the
plan is computed from the per-iteration estimate of the previous attempt, so
the same family keeps a stable chunk size across the attempts of one phase.

## E-02 Raw point column

One column per compiled leaf of the attempt's plan, the shape
`RecorderApi::columns` already gives: `stride` rows per column, `count`
committed points.

| Field | Type | Rule |
|---|---|---|
| points per column | `std::uint64_t` | `chunkCount + 1` for a run with no pause, plus the pause points the room rule of 019 reserves |
| policy | `HardStop` | an overrun is an `SG_REQUIRE_ALWAYS` violation, so no point is dropped (FR-006, D-02) |
| reservation | before the window | the recorder is minted before `entry.callable(state)` runs (FR-008) |

**Invariants**: consecutive chunks share the point between them (FR-002);
`wrapped()` stays false and `dropped()` stays 0 for every run of this feature.

## E-03 Chunk value

One value per column per chunk, produced after the run.

| Field | Type | Rule |
|---|---|---|
| value | `double` | `foldCore(core, api, base + i, base + i + 1)` with the pause pairs inside the window netted out (FR-015, D-05) |
| availability | `Availability` | the fold's own disclosure for that window (FR-016) |

**Invariants**: a run of N chunks yields N values per column; the primary time
chunk value divides by `chunkIterations` before it prints (FR-002); a value
whose window holds a gap carries that gap's availability.

## E-04 Percentile column

One per reported column of a row, the residue D-14 keeps.

| Field | Type | Rule |
|---|---|---|
| `name` | `std::string` | the column's report name, the primary time, the cpu time, or the counter name |
| `unit` | `std::string_view` | the unit the column already prints in |
| `values` | `std::vector<double>` | the chunk values, sorted for the rank read and kept for N3 (FR-009) |
| `percentiles` | `std::vector<PercentileValue>` | one per requested percentile |
| `availability` | `Availability` | the worst availability across the column's windows |

`PercentileValue` holds `double percentile`, `double value`, and
`bool available`.

**Invariants**: `values.size() == chunkCount`; a `PercentileValue` stands
unavailable when `values` is empty or when `values.size()` falls below
`ceil(100 / (100 - p))` (FR-013); no value is ever extrapolated.

## E-05 Percentile set

The list a run reports, from the family default or the command line.

| Field | Type | Rule |
|---|---|---|
| `values` | `std::vector<double>` | each in the open interval (0, 100), distinct, ascending (FR-021, FR-024) |
| default | constant | p50, p90, p99, p99.9 (FR-012) |
| minimum count of a value p | `std::uint64_t` | `ceil(100 / (100 - p))`: 2, 10, 100, 1000 for the defaults (FR-013) |

**Invariants**: a duplicate collapses to one entry; the order printed is
ascending whatever order the option carried.

## E-06 Percentile option state

| Field | Type | Rule |
|---|---|---|
| `RegistryEntry::showPercentiles` | `std::optional<bool>` | the family setting, written by `BenchmarkHandle::showPercentiles` (FR-019) |
| `RegistryEntry::percentileList` | `std::vector<double>` | the family list, empty for the default |
| `RunOptions::showPercentiles` | `std::optional<bool>` | `--percentiles=` sets true, `--no-percentiles` sets false (FR-020) |
| `RunOptions::percentileList` | `std::vector<double>` | `--percentiles=` fills it (FR-021) |

**Invariants**: the command line wins over the family, the precedence
`--repetitions` takes (D-09); the default of both stands off, so a run with no
option prints no percentile line (FR-023).

## E-07 ResultRow extension

| Field | Type | Rule |
|---|---|---|
| `percentiles` | `std::vector<PercentileColumn>` | empty when the output stands off (D-08) |
| `chunkCount` | `std::uint64_t` | the chunks the run took (FR-007) |
| `chunkIterations` | `std::uint64_t` | the iterations per chunk the plan chose (FR-007) |

**Invariants**: `endpointSamples` keeps its name and counts the points the
cadence leaves (D-10); the `AGGREGATE` row carries no percentile column
(FR-010).

## E-08 Cursor chunk state

`State::Cursor` today holds `m_state`, `m_start`, `m_remaining`
(`include/speedgun-ng/benchmark.hpp:540-544`).

| Field | Type | Rule |
|---|---|---|
| `m_remaining` | `std::uint64_t` | the iterations left in the current chunk, the one counter the loop reads |
| `m_runRemaining` | `std::uint64_t` | the iterations left in the run, read only on the chunk-boundary path |
| `m_chunkIterations` | `std::uint64_t` | the refill size, from E-01 |

**Invariants**: the per-iteration path holds one decrement and one compare,
the shape the loop gate measures (D-01); the sample at a boundary happens
once per chunk.

## E-09 Recorder reservation seam

`RecorderReservation` in `source/harness/detail/internal.hpp:181` gains a
field.

| Field | Type | Rule |
|---|---|---|
| `endpointPoints` | `std::size_t` | today's field, now `chunkCount + 1` |
| `pauseRoomPoints` | `std::size_t` | unchanged, the room rule of 019 |
| `chunkPoints` | `std::size_t` | new: the chunk points of the attempt, the seam the capture test reads (FR-006) |

**Invariants**: the seam is written once per attempt on the thread that runs
it; the pause suite keeps reading the two fields it reads today.

## E-10 Report lines

| Line | Shape | Rule |
|---|---|---|
| chunk disclosure | `chunks=<n> iterations/chunk=<n>` | one line per run with the output on (FR-007) |
| percentile line | `<name> p50=<v> p90=<v> p99=<v> p99.9=<v>` | one per reported column, `not available` in a value field that FR-013 makes unavailable (FR-010) |
| context percentile line | the same line under the context disclosure | one per context of a threaded run (FR-014) |

**Invariants**: no percentile line prints when the output stands off; the
`AGGREGATE` row prints none; the observer cost stays in the run row's
`overhead floor (ns)=` field (D-11).

## Memory arithmetic

One column costs `pointCapacity * 8` bytes. At the D-04 bound of 32'768
points that is 256 KiB per column, the same ceiling the pause room of 019
already carries. A run with the primary time, the cpu time and two counter
columns costs four columns, so 1 MiB per context at the bound. The chunk value
columns the row keeps cost `chunkCount * 8` bytes per column, under the same
ceiling.
