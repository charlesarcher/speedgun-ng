# Data Model: Benchmark Harness Core

Phase 1 output for `specs/015-benchmark-harness-core/spec.md`. The
entities are the Key Entities of the spec plus the two runtime records
the runner moves between them. Field spellings follow Principle V.1;
the contracts of each field are the requirement that fixes it.

## E-01: BenchmarkEntry (the registry record)

The registry holds one entry per registered name, in registration
order.

| Field | Type | Rule |
| --- | --- | --- |
| `name` | `std::string` | unique among entries; a duplicate is a recoverable error at the 007 FR-046 tier, the first entry stays, the rest run (FR-001, FR-002) |
| `callable` | type-erased callable taking `State&` | the benchmark function (FR-001, FR-004) |
| `minTimeNs` | `std::optional<std::int64_t>` | a benchmark value wins over the command line (FR-015) |
| `warmupTimeNs` | `std::optional<std::int64_t>` | same precedence rule (FR-011, FR-015) |
| `repetitions` | `std::optional<std::uint64_t>` | same precedence rule (FR-012, FR-015) |
| `fixedIterations` | `std::optional<std::uint64_t>` | set means calibration is skipped (FR-013); warm-up still starts from it and grows by the FR-010 rule (FR-011) |
| `expressions` | type-erased metric expressions, one per attached metric | built with the 007 arithmetic (FR-021) |

Relationships: the `BenchmarkHandle` (E-02) is the caller's view of
one entry; the registry owns the entries.

## E-02: BenchmarkHandle

The registration result (FR-003). It names its entry and sets the
run-control options FR-007 through FR-015 and the metric options of
FR-021.

| Field | Type | Rule |
| --- | --- | --- |
| `m_entry` | pointer into the registry | lives as long as the registry |
| option setters | `minTime`, `warmupTime`, `repetitions`, `iterations`, `addMetric` | each writes one E-01 field before the run starts |

Validation: an option setter after the run starts is a contract
violation (the run reads the entry once). A metric expression whose
leaf the catalog lacks is a recoverable resolution failure at the 007
FR-046 tier (edge case, FR-023).

## E-03: State

The object the harness passes to the benchmark function (FR-004).

| Field | Type | Rule |
| --- | --- | --- |
| `m_iterations` | `std::uint64_t` | the iteration count of the current run; the function reads it through `iterations()` (FR-005) |
| `m_index` | loop cursor | the range-for `begin`/`end` pair spans `m_iterations` steps; the iterator reads no interrupt flag (R-06) |
| `m_outcome` | `RunOutcome` plus reason text | set by `skipWithError` or `skipWithMessage` (FR-031) |

Validation: `skipWithError` and `skipWithMessage` record the reason, and
the function leaves the loop with `break` or `return`; the reason prints
and no statistics print (FR-031). Setup and
teardown live in the function, outside the range-for, in the untimed
region (007 FR-050).

## E-04: Run

One pass of the timed loop over N iterations (FR-006). It owns exactly
two recorded points, the window endpoints (FR-017).

| Field | Type | Rule |
| --- | --- | --- |
| `iterations` | `std::uint64_t` | the N of this pass |
| `pointPair` | indices `2k`, `2k+1` in the benchmark's recorder | entry sample and exit sample, nothing else (FR-017, FR-052) |
| `decisionTimeNs` | `std::int64_t` | the `machine/thread_cpu` fold over the pair (FR-009) |
| `realTimeNs` | `std::int64_t` | the `machine/monotonic` fold over the pair (FR-008, FR-020) |

State transition: warm-up runs and calibration runs feed the growth
rule and are discarded (FR-011, FR-018); a qualifying run of the
measured phase feeds one repetition row.

## E-05: Metric

A counter expression built with the 007 arithmetic, or a catalog leaf
attached by address (FR-021). Its value is the fold of the two points
of a run (E-04).

| Field | Type | Rule |
| --- | --- | --- |
| `expression` | 007 `Expression` spine | compiled into the benchmark's plan (R-11) |
| `perIteration` | `bool` | true for dimension events^1 or time^1, false for every other dimension (FR-022); the `Dim` tag or `dimensionOf` sets it |
| `availability` | `Availability` | read before the run for every leaf the metric needs; a leaf that cannot count marks the metric unavailable with its refusal kind (FR-023, PC-3) |

Relationships: one metric folds into every ResultRow (E-06); the
address-attached form resolves through the catalog unit's dimension
(R-04).

## E-06: ResultRow

One repetition row or one aggregate row (FR-035).

| Field | Type | Rule |
| --- | --- | --- |
| `kind` | repetition or aggregate | aggregates appear only when repetitions exceed 1 (US5) |
| `iterations` | `std::uint64_t` | the N of the measured run |
| `timePerIterationNs` | `double` | the monotonic fold divided by N (FR-020) |
| `metrics` | one `MetricValue` per metric | each carries `value`, `runningRatio`, `scaled`, `availability` from the fold (FR-024, `MetricResult` of 007) |
| `overheadFloorNs` | `double` | `Plan::sampleOverheadNsMedian()`; the row states that the window includes the cost of its two endpoint samples (FR-026, PC-5) |

Validation: a run with a gap reports its value unavailable and adds no
sample to the statistics (FR-025, PC-4). The gap belongs to the
quantity whose fold measured nothing: that quantity's aggregate drops
the repetition, and a quantity whose own window measured cleanly keeps
its sample (the per-quantity reading of FR-025's run-level wording,
settled at T050). The aggregate fields are
mean, median, sample standard deviation, coefficient of variation,
min, and max over the qualifying repetitions (FR-027, R-05). The
percentile form stays out (FR-028).

## E-07: BenchmarkResult

The C++ value the harness exposes; the console report formats it and
adds nothing to it (FR-036).

| Field | Type | Rule |
| --- | --- | --- |
| `name` | `std::string` | the registered name |
| `outcome` | `RunOutcome`: measured, skipped, failed | skipped carries the skip or interrupt reason; failed carries the exception text (FR-031, FR-032) |
| `rows` | `std::vector<ResultRow>` | one per repetition, aggregates appended (FR-027, FR-035) |
| `reason` | `std::string` | the skip or failure text; a skipped outcome prints it and prints no statistics |
| `metricLabels` | `std::vector<std::string>` | the column label of each metric position in attachment order; the report prints each metric under its label and takes no value from it (FR-026, FR-035) |

## E-08: ReportContext

The context lines above the rows (FR-035).

| Field | Source | Rule |
| --- | --- | --- |
| `version` | the build-written version macro | always printed; the harness target's private `SG_PROJECT_VERSION` compile definition, the sibling of `SG_BUILD_TYPE` (T002, T051) |
| `buildType` | `SG_BUILD_TYPE` (R-14) | always printed |
| `host`, `cpu` | the counters catalog, where it publishes them | a field the library does not publish stays out of the line (FR-035, R-08) |

## E-09: RunOptions (the parsed command line)

| Field | Type | Rule |
| --- | --- | --- |
| `filter` | `std::optional<std::string>` | a regular expression over names; a miss says so, runs nothing, exits zero (FR-034, edge case) |
| `listMode` | `bool` | prints matching names, runs nothing, exits zero (US2, clarification) |
| `repetitions`, `minTime`, `warmupTime`, `iterations` | numeric | an unparseable or negative value is a recoverable error; a zero repetition count, iteration count, or minimum time is a recoverable error; a zero warm-up time is valid (FR-034, clarification) |
| `dryRun` | `bool` | one iteration, one repetition, no warm-up (FR-014) |
| `leafAddresses` | `std::vector<std::string>` | attached to every selected benchmark (FR-021) |
| `listCatalog` | `bool` | prints the catalog and exits without running a benchmark (FR-037) |

## E-10: CatalogLine

One line of the catalog listing (FR-037), built from `CatalogEntry`
(PC-9): the object path, the counter name, the description, the unit,
the read mode, and the availability state with its refusal kind. The
`frequencyHz` and `scaled` fields of `CatalogEntry` stay out of the
line in this spec; FR-037 names the six printed fields.

## Relationships

```text
registry 1──* BenchmarkEntry 1──1 BenchmarkHandle
BenchmarkEntry 1──* Metric          BenchmarkEntry 1──* Run (per phase)
Run 1──2 recorded points            Run *──1 recorder (per benchmark)
Run 1──* MetricValue (fold)         repetition *──1 ResultRow
BenchmarkResult 1──* ResultRow      report formats BenchmarkResult + ReportContext
```

## State transitions

The benchmark outcome machine is the plan's Logical view:
`registered → running → measured | skipped | failed`, with `skipped`
reached by `skipWithError`, `skipWithMessage`, or SIGINT, and
`failed` reached by an exception. The runner reads the SIGINT flag
after each run. A `failed` or `skipped` outcome
releases the plan, the recorder, and every descriptor and mapping
through the runner frame's RAII destruction (FR-032, PC-6).

The recorder of one benchmark moves through: `minted (capacity
fixed)` → `sampling (2 points per run)` → `folded (post-run)` →
`released`. Overflow of a hardStop recorder is a loud failure of the
sizing rule (R-02), never a silent drop.
