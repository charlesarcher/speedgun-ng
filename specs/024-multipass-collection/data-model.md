# Phase 1 Data Model: Multi-pass metric collection

Entities (E-numbers), their fields, invariants (I-numbers), validation rules,
and state transitions. FR numbers refer to `spec.md`. Decision numbering:
`spec.md` numbers its decisions `D-1` to `D-12` and [research.md](research.md)
numbers its own `D-01` to `D-15`. A bare `D-n` is a `spec.md` decision; a
zero-padded `D-01` to `D-09` is a `research.md` decision. The two ranges meet
at 10, so a reference of 10 or above names its file, as in `research D-10` or
`spec D-10`.

## E-01 - Metric collection method

The concrete collection strategy one run settles to. Closed enumeration,
public in `include/speedgun-ng/benchmark.hpp`.

| Enumerator | Meaning | Printed token |
|---|---|---|
| `AUTO` | Caller left the choice; never a settled value | (never printed) |
| `PASSES` | The planner path split the columns into passes, one or more (FR-001) | `passes` |
| `MULTIPLEX` | One plan with every PMU event; the kernel rotates and the `ratio=` field discloses (FR-024) | `multiplex` |

Fields carried per run:

| Field | Where | Type | Rule |
|---|---|---|---|
| requested method | entry (family state) | `MetricMethod` | The `metricMethod` selector's value; default `AUTO` (FR-025) |
| CLI method set | `HarnessOptions` | `bool` + `MetricMethod` | Set by `--metric-method`; the flag names one of the three words (FR-024) |
| forced flag | entry | `bool` | True when the caller's own choice (flag or selector) named a method; decides printing (research D-10; FR-021) |
| settled method | `BenchmarkResult::metricMethod` | `MetricMethod` | `PASSES` or `MULTIPLEX` once the run carries metric columns; `AUTO` while it does not (FR-017) |

Resolution table (input: CLI word, entry value, whether any needed PMU source
publishes a limit, whether the run carries metric columns; output: settled
method and whether the planner runs). Every row is driven by a test.

| Metric cols | Any limit | CLI / entry | Settled | Planner runs | Console line |
|---|---|---|---|---|---|
| 0 | any | any | `AUTO` (fields stay default) | no | no |
| any | any | any, with `--list` or `--catalog` | nothing settles: the run ends before settlement, after the word is checked (FR-025, FR-026) | no | no |
| >=1 | no | any | `MULTIPLEX` | no | yes (FR-020, research D-10) |
| >=1 | yes | `multiplex` (forced) | `MULTIPLEX` | no | yes (FR-024) |
| >=1 | yes | `passes` (forced) | `PASSES` | yes | yes (FR-024, US2 s2) |
| >=1 | yes | `auto` + set fits | `PASSES` | yes (1 pass) | no (FR-021, D-9) |
| >=1 | yes | `auto` + set does not fit | `PASSES` | yes (n>1) | yes (FR-001, FR-017) |

Priority: the CLI word wins over the entry value (FR-025; research D-09, the
`--percentiles` precedent at `source/harness/cli.cpp:345-348`); `auto` means
"the entry's value if one was set, else `AUTO` resolution here". A bad CLI
word never reaches this table: the process prints one line and exits 2 before
the first run (FR-026; `contracts/metric-method-option.md`).

State transition of one run: `AUTO` (nothing settled) -> settled method fixed
before the first window opens (spec: "One run settles to exactly one concrete
method before the first window"), one way, never re-decided mid-run (FR-025,
FR-007).

## E-02 - Programmable-counter limit fact

The per-event-source answer: how many programmable counters one context can
hold open at once on that source.

| Field | Where | Type | Rule |
|---|---|---|---|
| published limit | `ObjectSeed::programmableCounters` (`counters_provider.hpp:62`) | `std::optional<std::size_t>` | Provider fills at `enumerate`; empty means the source cannot answer (FR-005) |
| query | `Object::programmableCounters()` (`counters_system.hpp:34`) | `std::optional<std::size_t>` | Answers the published value; `\pre` none |
| scripted limit | `FakeProvider::setProgrammableCounters(objectPath, limit)` (`counters_fake.hpp:103`) | fluent setter | Must precede `System::open`, like every scripted fake fact (FR-006; D-03) |

Source of the real value: CPUID leaf 0AH, `EAX[7:0]` version, `EAX[15:8]`
count; the answer exists when version and count are both nonzero (D-01).
Measured on `corsairone` on 2026-10-20: version 4, count 4
(`EAX=0x07300404`).

Validation rules:

| Rule | Consequence | Source |
|---|---|---|
| A source that cannot answer publishes nothing | The planner applies no limit to its leaves; the run keeps kernel rotation and `ratio=` | FR-005, FR-024 |
| The fake limit applies to fake objects only | Real runs are unchanged by test scripts | FR-006 |
| Zero is not a published count | Version-0 or count-0 hosts publish nothing, never a zero limit | D-01 decode table |

## E-03 - Pass plan

The planner's output: what each pass measures. Harness-internal, produced by
`detail::planPasses` (D-05).

| Field | Type | Rule |
|---|---|---|
| `columns` | `std::vector<std::vector<std::size_t>>`, index `k` = pass `k`, entries = column indices | Every PMU-bearing column appears in exactly one pass (FR-002) |
| `exclusive` | `std::vector<bool>` per pass | True for a pass holding one over-limit column alone (FR-003) |
| `measuringPass` per column | `std::size_t` | The pass that measures that column; feeds E-08 and the merge |

Placement rules, in this order, deterministic (FR-001, FR-004):

1. Columns are placed in request order: metric columns in `metricLabels`
   order (`benchmark.hpp:456`), then user-counter columns in open order
   (`benchmark.hpp:460`).
2. A column's cost at a source is its PMU leaf addresses on that source,
   deduplicated by address; leaves shared by earlier columns of the same pass
   are already counted (shared events claim one counter, research D-05).
3. A column joins the first pass where every source's used-plus-cost stays
   within that source's published limit.
4. A column whose cost exceeds some source's limit on its own stands alone in
   its own pass, still opened whole: the kernel rotates it and `ratio=`
   discloses (FR-003, D-2).
5. Leaves whose source publishes no limit are not costed anywhere and ride
   every pass whose columns need them, keeping their own `ratio=` disclosure
   (D-4; FR-005's edge; the two-provider edge of US1).
6. The two fixed time leaves (`machine/monotonic`, vendor cpu leaf;
   `runner.cpp:1119,1127-1135`) belong to clock sources that publish no limit
   and ride every pass; they consume no programmable counter (D-4).
7. With no limit published at all, no plan is produced: the run settles
   `MULTIPLEX` (E-01 row 2).

Validation rules:

| Rule | Consequence | Source |
|---|---|---|
| Plan computed before any counter read or window | Planning reads only published facts and catalog leaf tables | FR-008, SC-06 |
| 64-column set | Every column is placed; pass count grows; no limit on pass count | edge, FR-001; the PC-3 oversize measurement |
| Empty metric set | No planner run; master path | edge; E-01 row 1 |

Invariant I-1: pass `k`'s per-source used counts never exceed the published
limits except inside an exclusive pass. Invariant I-2: column order within a
pass equals request order; passes are ordered by their first column's request
position.

## E-04 - Pass (execution record)

One pass's run of the benchmark. Harness-internal, lifetime: from its plan
compile to the close of its last window (FR-012).

| Field | Rule |
|---|---|
| plan compile | same `compileCore` site as today's single compile: `runner.cpp:1252-1253` single-context, `runner.cpp:1727-1730` per context threaded (D-06) |
| repetitions | every repetition of the family runs in every pass (FR-009) |
| iteration count and chunk plan | calibrated once before pass 0 and reused by every pass (FR-010) |
| fold results | this pass's columns' `MetricValue`s, time columns, overhead floor, endpoint samples, chunks and percentiles, per row |
| windows | open at the pass's first window, closed when the pass's plan dies; pass `k` closed in every context before pass `k+1` opens anywhere (FR-012, FR-013; threaded dispatch sits inside the pass loop, D-06) |

Invariant I-3: at any instant, at most one pass has an open window anywhere in
the run. Invariant I-4: the reference pass is pass 0 (D-5).

## E-05 - Merged result

The single `BenchmarkResult` the run publishes. The merge is a pure function
over the per-pass fold results (D-07).

| Field of the merged result | Taken from | Rule |
|---|---|---|
| each metric column value | its measuring pass | position preserved (FR-014) |
| each user-counter column | its measuring pass | FR-014 |
| time columns | pass 0 | FR-015 |
| overhead floor, endpoint samples | pass 0 | FR-015 |
| context disclosures (`contexts`) | pass 0 | research D-06 threaded rule; `printContextDisclosure` untouched (N4) |
| chunk disclosure, time-column percentiles | pass 0's chunk times | percentiles edge of US3 s2 |
| metric-column percentiles | each metric's own pass chunks | percentiles edge of US3 s2 |
| per-value availability | measuring pass | `GAP` stays `GAP`; refusals ride (FR-016) |
| `metricMethod`, `passCount`, `passSpread` | settlement and E-06 | FR-017 |

Validation rules:

| Rule | Consequence | Source |
|---|---|---|
| A measuring pass window folded a gap for a column | The published value is `GAP`, no invented number | FR-016, SC-05 |
| A column's source refused | its `MetricRefusal`-shaped kind rides unchanged | FR-016; existing field semantics |
| Threaded run | one row set as single-context runs; per-context percentiles from pass 0 | US1 threaded pass order; 020 semantics untouched |

Invariant I-5: merging re-reads no counter: every value comes from a fold
result E-04 already holds (FR-014's `\pre`).

## E-06 - Pass spread

| Field | Type | Rule |
|---|---|---|
| value | `std::optional<double>` percent | `(max - min) / median * 100` over each pass's merged reference time per iteration; empty when passes < 2 or median is 0 (FR-017; D-6, Q3) |
| median | pure | the median of an even count is the lower of the two central values (D-6) |
| warning predicate | pure | value exists and `> 5.0` percent (FR-018) |

Spread fixtures, the one table every test and document cites (FR-017, FR-018,
SC-05):

| Pass times per iteration | Spread | Printed | Warning |
|---|---|---|---|
| 100, 102, 104 | 3.9 percent | `spread=3.9%` | no |
| 100, 100, 105 | exactly 5.0 percent | `spread=5.0%` | no: the predicate is a strict greater-than |
| 100, 104, 108 | 7.7 percent | `spread=7.7%` | yes |
| 100, 108 | 8.0 percent | `spread=8.0%` | yes: the two-pass case of the even-count median |
| 100, 108, 112 | 11.1 percent | `spread=11.1%` | yes |
| 100, 100, 100 (equal times) | 0.0 percent | `spread=0.0%` | no |
| 0, 0, 0 (zero median) | no value | `spread=not available` | no |

Threaded runs take the merged per-pass value (the number the report shows for
that pass), so the spread answers the run the user sees (D-6, Q3). The
lower-of-two rule is what makes the two-pass row read 8.0 (D-6). Rounding
happens once, at print time (research D-08). The warning prints a line and
changes nothing: no refusal, no rerun, no outcome change (FR-018, US3 s3). A
zero median prints `spread=not available` and warns never (edge; research
D-08).

## E-07 - Pass disclosure (published fields and console line)

| Field | Where | Rule |
|---|---|---|
| `BenchmarkResult::metricMethod` | `benchmark.hpp` (near `percentilePoints`, line 492 neighborhood) | settled method (FR-017) |
| `BenchmarkResult::passCount` | same | number of passes run; metric-bearing runs only carry a meaningful value; `0` means the mechanism did not run; a pass loop left early publishes the passes whose folds stand, so a stopped pass that folded no row stays out of the count (edge, FR-017) |
| `BenchmarkResult::passSpread` | same, `std::optional<double>` | E-06 value (FR-017); the merge is its only writer, so a run stopped inside the pass loop publishes none (edge) |

Console rule (research D-10, FR-020, FR-021, FR-024): the `passes` line prints when the
run carries at least one metric column and (the settled method is `MULTIPLEX`
or the pass count exceeds 1 or the caller forced the method). The exact
printed shape, the `spread=not available` token, and the warning line are the
contract of `contracts/pass-disclosure.md`; they are golden-tested.

JSON: the fields join the JSON schema when that lane's schema file lands; no
shared report-schema file is touched here (N3; Q4; FR-017).

## E-08 - Pass tag on a metric value

| Field | Where | Rule |
|---|---|---|
| `MetricValue::pass` | `benchmark.hpp:211` struct | index of the measuring pass; printed as `pass=N` inside the existing metric field spelling when `passCount > 1` (FR-019; research D-11); only the merge writes a tag, so a run stopped inside the pass loop carries none (edge) |

A one-pass run prints no `pass=` token: master's field list stands untouched
(FR-021, D-9). The tag is per value: rows of one result may print the same
pass index for one column across all rows, and that equality is checked by
the report test (per-row merge provenance).

## Entity relationships

```text
E-02 limit facts --feeds--> E-03 pass plan <--settled by E-01 method--+
        |                        |                                    |
        | registered by          | drives E-04 pass execution        |
        | providers / fake       |  (pass 0 = reference)             |
        v                        v                                    |
   Object fact          per-pass folds --merged--> E-05 result <------+
                              |                        |
                              |                        +-- E-06 spread --> warning line
                              +-- E-08 pass tags <-----+-- E-07 fields --> passes line
```

## State transitions beyond E-01

One run: `settle (E-01)` -> `calibrate (once)` -> `[pass 0: compile -> open ->
repetitions -> fold -> close]` -> `[pass 1: ...]` -> ... -> `merge (E-05)` ->
`spread (E-06)` -> `disclose (E-07, E-08)`. The transition graph has no cycle:
a pass never reopens after closing, a settled method never changes, and the
merge runs once after the last pass closes (FR-007, FR-012).

One fake provider object: `no scripted limit` -> `setProgrammableCounters` ->
`scripted limit published at next open` (one-way per provider lifetime, D-03).

## Invariants (index)

| # | Invariant | Enforced by |
|---|---|---|
| I-1 | non-exclusive passes fit every source limit | planner post-condition (`SG_ASSERT` in `pass_plan.cpp`) |
| I-2 | column order is request order | planner post-condition; test |
| I-3 | at most one pass holds windows at any instant | structure: pass loop outside dispatch, join between passes (FR-013); test counts per-pass opens |
| I-4 | pass 0 is the reference | merge input order |
| I-5 | merge re-reads no counter | merge signature takes fold results only (FR-014) |
| I-6 | a metric-bearing settled run names one method | E-01 table is total; `metricMethod != AUTO` asserted at disclosure for metric-bearing runs |
