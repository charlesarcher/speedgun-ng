# Research: Percentiles and Raw Capture

Decisions D-01 to D-16, each with the rationale and the alternatives the plan
rejected. Every decision names the FR it serves and the code it reads.

## D-01 The chunk boundary stands in the loop cursor (FR-001, FR-002)

**Decision**: `State::Cursor` keeps one countdown counter and gains the run's
remaining total. The counter runs down the current chunk. When it reaches
zero and the run still holds iterations, the cursor takes one sample, refills
the counter with the next chunk size, and the loop continues. When the run
holds no more iterations, the cursor closes the window the way it does today.

**Rationale**: the per-iteration code path stays exactly what it is today:
one decrement in `operator++` and one compare in `operator!=`
(`include/speedgun-ng/benchmark.hpp:509`, `:530`). The sample sits on the
path that already runs once per chunk, the same path that calls
`closeWindow()` today. `test/loop_shape.sh` compares the shipped loop against
a hand-written count-down loop with the same body and fails a loop that pays
an extra load per iteration, and this shape pays none.

**Alternatives considered**: a chunk check inside `operator++` adds a second
decrement and compare per iteration, which is the exact shape the loop gate's
liveness probe plants to prove the detector works. Rejected: it charges the
cost to every benchmark, Principle VII's floor rule makes that cost the
reported number, and the gate would report it. A second nested loop the
benchmark body writes itself is rejected: the body is the user's code, and
`for (auto _ : state)` is the documented loop of the main page.

## D-02 One point per boundary, hardStop capacity of chunks + 1 + pause room (FR-002, FR-006, FR-008)

**Decision**: a run of N chunks leaves N+1 points per column. The attempt
mints `plan->recorder(chunkPoints + 1 + pauseRoom)` with the `HardStop`
policy it uses today (`source/harness/runner.cpp:1553`), so an overrun is an
`SG_REQUIRE_ALWAYS` violation in every configuration and no point is ever
masked away.

**Rationale**: the counters library already ships both overflow policies, and
`Ring` is the one that drops points with accounting. FR-006 forbids dropping,
so `HardStop` is the policy that states the rule in code. The capacity is
computed before the window opens, so the allocation stands outside the timed
window (FR-008), the same place the current capacity stands.

**Alternatives considered**: `ring` with a `dropped()` check after the run is
rejected: it hides the mistake until after the measurement, and the run's
percentiles would be computed from a column with holes. Two points per chunk
is rejected by the clarification of 2026-10-19: it doubles the memory for a
value the shared boundary already gives.

## D-03 The chunk size comes from the plan's own sample calibration (FR-005)

**Decision**: `chunkIterations = max(1, ceil(100 * plan.sampleOverheadNsMedian() / iterationNs))`,
where `iterationNs` is the per-iteration estimate the attempt already holds
from the previous run's fold. The factor 100 is the 1% bound of FR-005.

**Rationale**: `Plan::sampleOverheadNsMin/Median/Max` already calibrate one
`sample()` action over the plan's own read sequence
(`include/speedgun-ng/counters_measurement.hpp:954-973`), and the runner
already reads the median into `ResultRow::overheadFloorNs`
(`source/harness/runner.cpp:1065`). The 1% rule is then one division against
a number the plan measured, and the same number is the observer cost of
FR-017, so the two requirements read one source.

**Alternatives considered**: measuring the sample cost inside the run is
rejected: it pays for the measurement in the measured region. A fixed chunk
size is rejected: it breaks the 1% bound on a fast loop and wastes resolution
on a slow one.

## D-04 One constant bounds the points of a column (FR-006)

**Decision**: `kMaxChunkPointsPerLeaf = 32'768` in
`source/harness/detail/calibration.hpp`, beside `kMaxPauseSamplesPerLeaf`,
and the chunk count of an attempt is
`min(iterations / chunkIterations, kMaxChunkPointsPerLeaf - 1 - pauseRoom)`.

**Rationale**: the neighbor constant is the established shape for a per-leaf
point bound in this code base, carries the same 256 KiB per column at one
`std::uint64_t` per point, and its comment records the same reasoning. The
bound is a ceiling on memory, and the chunk count falls out of it, so the
calibration raises the chunk size; it drops no points.

**Alternatives considered**: an unbounded column is rejected by FR-006 and by
Principle VII's memory discipline. Reusing `kMaxPauseSamplesPerLeaf` for both
rooms is rejected: the two rooms answer two different rules, and one constant
would make a pause change the percentile resolution.

## D-05 Chunk values come from one explicit fold per chunk, with the pause netted per chunk (FR-002, FR-015)

**Decision**: after the run, the runner folds each chunk with
`detail::foldCore(core, recorderApi, base + i, base + i + 1)` and nets the
pause pairs whose ordinals fall inside that window, reusing the netting the
whole-run fold already performs (`nettedFold`, `source/harness/runner.cpp:116`).

**Rationale**: `Expression::foldPairs` folds every adjacent pair of the
column, and a column that holds pause points has intervals that are not
chunks. Folding the chunk windows by ordinal keeps the chunk set exactly the
set FR-002 defines, and the netting rule stays the one rule the repository
already states for a paused run.

**Alternatives considered**: `foldPairs` over the whole column is rejected for
the pause case above; it would report a pause interval as a chunk. Excluding
pause points from the column is rejected: the pause machinery owns the column
and the room rule of 019 already reserves it.

## D-06 Exact nearest-rank; the vendored histogram stays out (FR-011, FR-013)

**Decision**: sort the chunk values and read rank
`ceil(p * n / 100)` in 1-based order. A percentile reports as not available
when the column holds no chunk value or when `n` falls below
`ceil(100 / (100 - p))`.

**Rationale**: the chunk count is bounded by D-04 at a few tens of thousands,
so a sort after the run is cheap and exact, and an exact value is what a
benchmarking framework owes the reader (Principle VII). The minimum sample
count is the arithmetic form of the clarification of 2026-10-19: p50 needs 2,
p90 needs 10, p99 needs 100, p99.9 needs 1000.

**Alternatives considered**: the vendored HdrHistogram
(`external/hdrhistogram_c`, gated in `source/hdrhistogram/hdrhistogram_gate.cpp`)
is rejected for this feature: it records into buckets and reports an
approximate value, and the gate translation unit carries zero runtime lines,
so no harness code links it today. It is the natural instrument for the N8
histogram, and nothing here blocks that. A linear-interpolation percentile is
rejected: it invents a value between two measurements, and FR-013 forbids
extrapolation.

## D-07 One pure translation unit holds the arithmetic (FR-011, FR-032)

**Decision**: `source/harness/percentiles.cpp` with the internal header
`source/harness/detail/percentiles.hpp`: a chunk value builder, a rank
function, and the availability rule. No class, no interface, no policy tag.

**Rationale**: the arithmetic is three free functions over a span of values.
A class would be an abstraction with one implementation, which Principle X.2
forbids. Keeping it in one translation unit gives the coverage gates and the
sanitizer presets the same flags as the runner and keeps the runner's file
from growing a second concern.

**Alternatives considered**: putting the arithmetic in `runner.cpp` is
rejected on file size and on the coverage story: the rank rule needs its own
table of cases. Putting it in the counters library is rejected: the counters
library reports folds and points, and a percentile is a harness report
choice.

## D-08 The row carries the chunk value columns (FR-009, FR-010)

**Decision**: `ResultRow` gains `std::vector<PercentileColumn> percentiles`,
one entry per reported column with the column's chunk values and the
percentile values the run resolved, plus `std::uint64_t chunkCount` and
`std::uint64_t chunkIterations`.

**Rationale**: the report and the tests read the values, and N3 needs the
same columns for a machine-readable report. Keeping them on the row is the
smallest place that survives the run, and the row already carries the
per-column `MetricValue` and `Statistics` vectors.

**Alternatives considered**: keeping the `RecorderHandle` alive for the whole
family is rejected: the plan arena and the recorder are per attempt, and
holding them across attempts would raise the memory the D-04 bound exists to
cap. Recomputing the values in the report is rejected: the report is a
printer, and the tests would then need the recorder.

## D-09 The handle call selects the output; the list is a command-line option (FR-019, FR-020, FR-021)

**Decision**: `BenchmarkHandle::showPercentiles(bool)` on the family, and two
options: `--percentiles=<list>` turns the output on for every family and sets
the list, and `--no-percentiles` turns it off for the run. A command-line
setting wins over the family setting, the precedence `--repetitions` already
takes.

**Rationale**: the handle pair of the harness is a setter that writes one
`RegistryEntry` field, the shape `minTime`, `repetitions` and
`iterations` use (`include/speedgun-ng/benchmark.hpp:1446-1474`). The list
belongs to the run: it is a reporting choice a reader changes per invocation,
and one list per family would need a parser in the public header for no
caller. `getopt_long` cannot carry two spellings of one name, so the off
switch is its own option, the way `--dry-run` stands beside the value
options.

**Alternatives considered**: `showPercentiles(std::span<const double>)` is
rejected: it puts the list in two places and gives the family a second rule
the command line then overrides. A single `--percentiles` with an optional
argument is rejected: `getopt_long` handles an optional argument with a
separate short option and no `=` form, which reads worse than two names.

## D-10 The percentile lines and the field whose value changes (FR-007, FR-010, FR-023)

**Decision**: with the output on, the report prints, after the run row, one
percentile line per reported column and one chunk disclosure line carrying
`chunks=` and `iterations/chunk=`. With the output off, no percentile line
prints. The `endpoint samples=` field keeps its name and now counts the
points the chunked cadence leaves, so its value changes for every run.

**Rationale**: the field counts sampling actions, and the cadence changed, so
the value that matches the run is the new count. Keeping the old number would
report a measurement the run did not take. The spec's FR-023 and SC-002 claim
the
whole report keeps its bytes; the claim that survives is that no percentile
line prints when the output stands off, and the analyze pass records that
correction.

**Alternatives considered**: printing the old count is rejected as a false
number. Hiding the field when the output is off is rejected: the field is
already on the row today, and dropping it is a larger report change than the
one the cadence forces.

## D-11 The observer cost rides the field that already holds it (FR-017)

**Decision**: no new field. `ResultRow::overheadFloorNs` already holds
`plan->sampleOverheadNsMedian()`, the median cost of one `sample()` action,
and that is the cost of one sample point.

**Rationale**: the clarification of 2026-10-19 chose this, and the code
already states it: the runner fills the field from the plan's calibration at
`source/harness/runner.cpp:1065`. The chunk disclosure line of D-10 gives the
reader the chunk size, so the two numbers together answer "what does the
capture pay".

**Alternatives considered**: a second field beside the percentiles is
rejected: it would print the same number twice under two names.

## D-12 Threaded runs fold per context and merge the chunk values (FR-004, FR-014)

**Decision**: each context keeps its own recorder with its own chunk points,
folds its own chunk values on its own thread, and publishes them. The merged
row's percentile set is the union of the contexts' chunk values; each context
also gets its own percentile line under the context disclosure the report
already prints.

**Rationale**: the per-context fold of 020 is the rule FR-004 inherits, and
the merged row is built from per-context values today
(`foldThreadedContext`, `source/harness/runner.cpp:1500`). The union is the
definition the clarification chose, and it needs no new synchronization: the
merge already happens after the stop barrier.

**Alternatives considered**: a shared column across contexts is rejected by
FR-004 and by the thread-target plan. Averaging the per-context percentiles is
rejected: an average of two percentiles is not a percentile.

## D-13 Manual time and unavailable columns (FR-013, FR-016)

**Decision**: a manual-time family has no primary fold, so its primary time
percentiles report as not available. A column whose fold reports a gap
reports every percentile of that column as not available.

**Rationale**: the manual path already has no primary fold
(`source/harness/runner.cpp:168`), and the availability of a fold is the
disclosure the counters library publishes. A percentile over a gap would
report a number the run did not measure.

**Alternatives considered**: reporting the manual per-chunk sums is rejected
for this feature: the manual path supplies one value per run, and a chunk
value needs two points of the same column.

## D-14 The residue N3 reads is the chunk value columns (FR-009)

**Decision**: the chunk value columns on the row are the residue this feature
keeps. The raw point columns die with the attempt recorder.

**Rationale**: FR-009 asks that the capture stay readable in the process
after the run and that no file be written. The chunk values are the capture
in the form every later reader wants, and they cost one `double` per chunk
against the `uint64` point they come from.

**Alternatives considered**: keeping the point columns is rejected by the
memory bound of D-04. Writing them to a file is rejected by FR-009.

## D-15 Version and SOVERSION (FR-030, FR-031)

**Decision**: version 0.11.0 when this feature merges first. The harness
`SOVERSION` moves 6 to 7. The `speedgun-ng` archive `SOVERSION` stays 2.

**Rationale**: the constitution's Refactoring and Evolution rule states that
public API changes are breaking changes and that `VERSION` and `SOVERSION`
stay consistent with the change. This feature adds a `BenchmarkHandle` method
and fields to `ResultRow`, both public in the harness archive, and 020 moved
the same `SOVERSION` from 5 to 6 for the same kind of addition. The counters
archive gains nothing.

**Alternatives considered**: holding the `SOVERSION` because the change is
additive is rejected: the rule keys on the public API, and source
compatibility plays no part in it; the repository's own precedent moved it for
an addition.

## D-16 The docs example reaches the percentile it names (FR-025, FR-026)

**Decision**: `example/docs/percentiles.cpp` runs a body slow enough, and for
long enough, that the run leaves at least 100 chunk values, so the section's
result sentence can name p50, p90 and p99. The `.expected` file holds the
rows, and the drift gate keeps the region and the page in step.

**Rationale**: the 1% rule of D-03 sets a floor on the chunk size, so a
two-nanosecond body reaches the p99 minimum only with a run in the tens of
millions of iterations. The example picks a body in the microseconds, where
100 chunks cost a few hundred milliseconds, and the `example_run` gate runs it
in CI.

**Alternatives considered**: naming only p50 in the section is rejected: the
default list is the feature's headline, and a section that cannot show p99
teaches the reader the not-available rule first. Lowering the 1% bound for the
example is rejected: the bound is the feature's cost rule, and an example that
breaks it is a counter-example.
