# Data Model: Hunter mode core (event ranking)

**Feature**: specs/029-hunter-core | **Audit point**: `bd10d76` | **Date**: 2026-10-22

Entities are E-01 to E-11, invariants I-1 to I-8. Public types live in
`include/speedgun-ng/hunt.hpp` in `namespace sg::hunt`; the two records the run
path keeps to itself, `detail::StageSplit` (the stage split with its cut
verdict) and `detail::RetriedPass` (one pass that fell back to lone-event
windows), live in `source/harness/detail/hunt_plan.hpp` in
`namespace sg::detail`. Field spellings are the spellings the contract files and
the JSON keys derive from, and a JSON key spells in `snake_case` while its
member spells in `camelBack`, the way the specs/024 pass keys do.

## E-01 `EventCatalog`

The selection of events a hunt run takes, expanded against the catalog.
Public class in `hunt.hpp`; one instance per hunt run; no clock, no counter read.

| Field or call | Type | Meaning |
|---|---|---|
| `expand` | `auto expand(std::string_view selection) -> std::expected<void, Error>` | `all`, one glob pattern, or a catalog address list whose entries are separated by commas, newlines or both (`@file` arrives as its address lines under the same rule); stores the expanded addresses in catalog order |
| `addresses` | `auto addresses() const -> const std::vector<std::string>&` | the deduplicated selection in catalog order; `split` applies the seed shuffle to it |
| `split` | `auto split(std::uint64_t seed) const -> std::pair<std::vector<CatalogAddress>, std::vector<SkippedEvent>>` | the countable addresses that become columns, and the entries refused at discovery with their kind |
| `devicePart` | `auto devicePart(std::string_view address) -> std::string_view` | the part before the last `/` (`splitAddress` in `runner.cpp:39-47` is the existing shape) |
| `isCoreDevice` | `auto isCoreDevice(std::string_view address) const -> bool` | D-1's rule: device part `cpu`, `cpu_core`, `cpu_atom`, or a device part starting with `armv8_` |

**Rules the entity owns**

- An address is `Object::path() + "/" + entry.name` (spec PC-1: `CatalogEntry`
  holds no address field).
- Discovery walks `System::local().object("machine")` and its `children()`, the
  shape `catalog.cpp:81-88` already uses.
- `all` takes a device when `isCoreDevice` answers for it; a list or a pattern
  reaches any device, uncore included (D-1).
- An address that repeats in one selection or across `@file` lines keeps its
  first position and drops the repeat.
- An entry whose `avail` is not `Availability::COUNTABLE` becomes a skipped
  record with that `Availability` as its kind (`counters_core.hpp:88-95`).

## E-02 `HuntNorm` and `HuntRank`

| Type | Enumerators | Source |
|---|---|---|
| `enum class HuntNorm : std::uint8_t` | `INSTR`, `CYCLE`, `ITER` | D-6, `--hunt-norm=` |
| `enum class HuntRank : std::uint8_t` | `RATE`, `CV` | D-3, `--hunt-rank=` |

No `SENSITIVITY`, `DELTA` or `SCORE` enumerator exists: specs/031 adds those
when its method exists (D-3). The names are printed in lower case by the
console and the JSON writer through one internal `huntNormName` and
`huntRankName` table each, indexed by the enumerator under a `static_assert` on
the enumerator count, the pattern `catalog.cpp:23-38` carries.

## E-03 `HuntOptions` and `HuntBudget`

The settled values of one hunt run. Public aggregate in `hunt.hpp`; every value
is inside the range its option states, because the CLI refuses a value outside
it before this record exists.

| Field | Type | Default | Range |
|---|---|---|---|
| `selection` | `std::string` | `"all"` | `all`, list, pattern, or the joined text of an `@file` |
| `norm` | `std::optional<HuntNorm>` | empty, which plans `instr` | the three of E-02; a set value marks the word explicit (D-13) |
| `rank` | `HuntRank` | `RATE` | the two of E-02 |
| `top` | `std::size_t` | `20` | 1 to 10000 |
| `noise` | `double` | `10.0` | percent CV threshold, greater than zero |
| `repeats` | `std::size_t` | `3` | 1 to 1000 |
| `seed` | `std::uint64_t` | `0` | any 64-bit value |
| `budget` | `std::optional<HuntBudget>` | empty | E-03 budget |

`HuntBudget` holds `kind` of `enum class BudgetKind : std::uint8_t { SECONDS,
PASSES }` and `amount` of `std::uint64_t`. A budget of `120s` is
`{SECONDS, 120}` and `8p` is `{PASSES, 8}`; a value with no suffix is refused,
so no default kind exists (FR-013).

## E-04 `HuntPlan`, `HuntColumn` and `HuntStage`

The plan one hunt run executes. Public records in `hunt.hpp`; `planHunt` builds
them and reads no clock and no counter (D-2).

| `HuntColumn` field | Type | Meaning |
|---|---|---|
| `event` | `std::string` | the event catalog address |
| `cycles` | `std::string` | the anchor address of the same device, empty when the device has none |
| `instructions` | `std::string` | the anchor address of the same device, empty when the device has none |
| `source` | `std::size_t` | the index of the event's device in the limit list handed to `planPasses` |

| `HuntStage` field | Type | Meaning |
|---|---|---|
| `columns` | `std::vector<std::size_t>` | the column indices this stage measures, in column order |
| `repeats` | `std::size_t` | 1 for stage 1, `options.repeats` for stage 2 |

| `HuntPlan` field | Type | Meaning |
|---|---|---|
| `columns` | `std::vector<HuntColumn>` | one column per event, in the selected order |
| `passes` | `std::vector<std::vector<std::size_t>>` | the column indices of each pass, from `planPasses` |
| `method` | `std::optional<MetricMethod>` | the method `settleMethod` settled for the plan |
| `stages` | `std::vector<HuntStage>` | one or two stages, E-04 rule below |
| `estimateNs` | `std::optional<std::uint64_t>` | `calibrationNsPerPass x passes x repeats`, the stage-1 figures on a split plan, empty without an estimate |
| `anchorsPerPass` | `std::size_t` | 2 when both anchors exist, else 0 |
| `normalized` | `bool` | false when the plan normalized by iterations because a device lacked an anchor |

**Stage rule** (D-2, FR-013): a plan with no budget holds one stage: every event
at `options.repeats`. A budgeted plan holds two stages: stage 1 covers every
event at one repeat, stage 2 covers the top `3 x top` events, capped at the event
count, at `options.repeats`. When the two-stage estimate does not fit the budget,
the plan drops stage 2 entirely and the run prints one `budget cut` line naming
the reason. Stage 1 never shrinks and no stage keeps a reduced repeat count.

**Limit rule**: a device that publishes `L` admits `max(L - 2, 0)` events per
pass; the three-leaf shape of one event with both anchors is the smallest plan
and fits at `L` of 3 or more. `L` below 3 with both anchors present means no
event fits: `planHunt` answers a recoverable error naming the device and the
limit, the caller prints one line and the run exits 2 (FR-081). A device with no
published limit leaves `sourceLimits` empty for it, `planPasses` returns an
empty plan and `settleMethod` settles `MULTIPLEX` (spec PC-2); the plan then
carries one pass listing every column, `method` reads `MULTIPLEX`, and every
event reports `multiplexed` true (D-19).

## E-05 `EventSamples` and `HuntSample`

The records the run collects and the ranker consumes. Public in `hunt.hpp`: a
test that scripted a machine builds them directly, which is how the rank
decision table reaches every branch without a PMU. One `HuntSample` per measured
repetition of one column of one pass.

| `HuntSample` field | Type | Meaning |
|---|---|---|
| `pass` | `std::size_t` | the pass that measured it |
| `stage` | `std::size_t` | 1 or 2 |
| `eventValue` | `std::uint64_t` | the event count of that window |
| `anchorInstr` | `std::optional<std::uint64_t>` | the `instructions` count of that window, empty on a device without the anchor |
| `anchorCycle` | `std::optional<std::uint64_t>` | the `cpu-cycles` count of that window |
| `iterations` | `std::uint64_t` | the iteration count of the attempt that measured it |
| `disclosure` | `std::optional<counters::Availability>` | set when the window disclosed a gap or a refusal |

| `EventSamples` field | Type | Meaning |
|---|---|---|
| `address` | `std::string` | the event address the rows belong to |
| `samples` | `std::vector<HuntSample>` | the collected rows, in pass then stage order |
| `skipped` | `std::optional<SkippedKind>` | set when the open of the address was refused or one of its windows disclosed a non-`COUNTABLE` availability, the kind E-06 names |

A sample with a disclosure stands out of the rate list, the way a gap
repetition stands out of `Statistics::samples` today.

## E-06 `RankedEvent`, `SkippedEvent` and `EntryHunt`

The published per-benchmark result. Public records in `hunt.hpp`; `rankEvents`
and `runHunt` fill them; `BenchmarkResult::hunt` holds the result.

| `RankedEvent` field | Type | Meaning |
|---|---|---|
| `address` | `std::string` | the event address |
| `rate` | `double` | the median of the per-pass normalized rates, in the unit E-08 states |
| `cv` | `double` | the coefficient of variation as a percent: one hundred times the `coefficientOfVariation` of `statisticsOf`; `0.0` for a single sample (PD-14) |
| `samples` | `std::size_t` | the number of clean samples that produced it |
| `noisy` | `bool` | `cv` above `options.noise` |
| `repeated` | `bool` | false when the event has one sample |
| `multiplexed` | `bool` | true when the plan settled `MULTIPLEX` |
| `rank` | `std::optional<std::size_t>` | 1-based position in the top N, empty for an excluded event |

| `SkippedEvent` field | Type | Meaning |
|---|---|---|
| `address` | `std::string` | the refused address |
| `kind` | `SkippedKind` | the refusal kind, E-06 rule below |

| `EntryHunt` field | Type | Meaning |
|---|---|---|
| `method` | `std::optional<MetricMethod>` | the settled method of the run |
| `passCount` | `std::size_t` | the pass count the run measured |
| `top` | `std::size_t` | the `--hunt-top` value the run used |
| `entries` | `std::vector<RankedEvent>` | every planned column, ranked ones first; an event only stage 1 measured holds a record built from its stage 1 samples |
| `skipped` | `std::vector<SkippedEvent>` | every event whose open was refused or whose window disclosed a non-`COUNTABLE` availability (D-10) |
| `retriedPasses` | `std::size_t` | passes that fell back to one event per pass (PD-12) |

`enum class SkippedKind : std::uint8_t` holds `PERMISSION_BLOCKED`,
`NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED` and `GAP`: the five `Availability`
values a skipped entry can carry (`counters_core.hpp:88-95`), through one named
mapping function (D-10, FR-021, FR-050). `GAP` names the mid-run window
disclosure (`counters_measurement.hpp:406-413`, disclosed by `fold` and
`foldPairs`). A group-open refusal that still
fails on the solo retry carries the kind of the catalog state its refusal
names, the state the counters discovery probe settled for the entry
(`provider.cpp:773-786`, spec PC-1, T118): `EINVAL`, `EOPNOTSUPP` and `ENOENT`
settle `NOT_ENCODABLE`, every other errno settles `PERMISSION_BLOCKED`, and a
refusal that names no state settles `ABSENT` (D-18). The console prints every
kind with the spelling the `--catalog` listing
prints for an `Availability` (`report.cpp:38 availabilityName`).

## E-07 Printed records

The console shapes, in the order PD-15 fixes. All hunt lines go to stdout behind
the `hunt:` prefix.

| Line | Shape |
|---|---|
| plan | `hunt: plan events=<n> passes=<p> method=<WORD> estimate=<ns>ns budget=<text>|none`, `estimate=` being the nanoseconds one measured pass cost times the stage-1 pass count at one repeat (FR-012, FR-013) |
| plan, no anchor | the plan line with ` normalized=iter` appended |
| plan, two stages | the plan line with ` stage1=<n> stage2=<n>` after `budget=`, each field the event count that stage measured (`report.cpp:774-778`) |
| plan, budget cut | `hunt: plan budget cut stage=2 reason=over-budget`, one line when the plan drops stage 2; `over-budget` covers a seconds budget no calibration figure prices, where no estimate stands to compare (FR-012, FR-013) |
| ranking row | `hunt: rank <n> <address> <norm>=<value> cv=<percent> [multiplexed]` |
| excluded tail | `hunt: rank - <address> <norm>=<value> cv=<percent> mark=<noisy|unrepeated|beyond-top>` |
| skipped | `hunt: skipped <address> <kind>` |
| summary | `hunt: plan summary events=<ran>/<planned> passes=<p> budget=<text>|none`, the counts and the budget the plan stood under; the line carries no spend figure, because a hunt holds no wall-clock reading (FR-040, D-6) |

The table ends at `--hunt-top` rows plus the tail of excluded events, and a host
that refuses everything prints the plan line, no ranking row, the skipped list
and the summary (D-12).

## E-08 `RunHunt`

The run-level record on `ReportData::hunt`. Public in `hunt.hpp`.

| Field | Type | Meaning |
|---|---|---|
| `selection` | `std::string` | the text `--hunt=` carried |
| `options` | `HuntOptions` | the settled values of E-03 |
| `passes` | `std::size_t` | the stage-1 pass count |
| `method` | `std::optional<MetricMethod>` | the settled method, empty for a plan nothing settled |
| `estimateNs` | `std::optional<std::uint64_t>` | the estimate the plan printed: one timed hunt pass times the stage-1 pass count at one repeat, the figure a seconds budget is weighed against; empty when the hunt timed no pass (`hunt.cpp:measurePassNs`) |
| `plannedAddresses` | `std::vector<std::string>` | the planned event addresses, in plan order |
| `stages` | `std::vector<HuntStageRange>` | the stages that ran; each holds its first index into `plannedAddresses`, its event count and its repeats |
| `ranEvents` | `std::size_t` | columns that produced at least one clean sample |
| `stageTwoEvents` | `std::size_t` | columns stage 2 measured |
| `cutReason` | `std::string` | the reason the planner recorded for the stage it dropped, the word the budget cut line prints; the planner records `over-budget` for an estimate above the budget and for a seconds budget no calibration figure prices (`hunt.hpp:467`, `report.cpp:788-791`) |

## E-09 `RunOptions` hunt fields

`source/harness/detail/internal.hpp` adds one member beside the specs/024
`metricMethod` field (its block runs to line 552):

```text
// specs/029 FR-080 to FR-082, D-22: the hunt selection and its settled
// values, empty when the run carries no hunt.
std::optional<HuntOptions> hunt;
```

`hunt` stands empty for a run with no `--hunt`, which is the fact the printers,
the writer and the runner test. The conflict check reads the existing members it
shares the struct with: `counterAddresses` (line 516), `metricNames` (line 527)
and `compareBaseline` (line 545).

## E-10 Scripted group refusal

`FakeProvider` gains one scripted machine fact beside `programmableCounters`
(`groupLeafLimit` at `counters_fake.hpp:330` in the private `ObjectSeedData`
at `315`, `setGroupLeafLimit` in the public block at `195`):

```text
auto setGroupLeafLimit(std::string_view objectPath, std::size_t leaves)
    -> FakeProvider&;
```

`open` refuses a leaf set that carries more than `leaves` leaves of that object,
and the refusal message names the object path and the leaf count. An object with
no scripted limit keeps the present behaviour of `open`, which refuses an
undeclared object or counter only (spec PC-3). The scripted state joins the
members of an exported `final` class, so the archive `SOVERSION` moves to 4
(hard rule 12).

## E-11 `statisticsOf`

The shared summary unit `rankEvents` calls. It moves out of the anonymous
namespace of `runner.cpp:49-82` into `source/harness/detail/statistics.hpp` with
its definition in `source/harness/statistics.cpp`, unchanged in behaviour.

| Call | Signature |
|---|---|
| `statisticsOf` | `auto statisticsOf(std::vector<double> values) -> sg::Statistics` |

Its contract moves with it: `SG_ASSERT(!values.empty(), ...)` and
`SG_ASSERT(n > 1, ...)` stay as the two guards `statistics.cpp:23-25` and
`statistics.cpp:46-48` hold, so every caller keeps the empty-list and
single-value guard at its own call site, and `rankEvents` keeps one sample out of
the call (PD-14).

| `Statistics` field | Spelling at `benchmark.hpp:261-272` |
|---|---|
| `samples`, `mean`, `median`, `sampleStdDev`, `coefficientOfVariation`, `min`, `max` | the seven fields of `sg::Statistics`, unchanged |

## Invariants

- **I-1** One column holds one event address and the two anchor addresses of the
  same device, with both anchors empty for a device that lacks either.
- **I-2** An address claimed as an anchor by one column of a pass is not counted
  twice in that pass: `planPasses` deduplicates leaves per pass, the fact the
  specs/024 invariant I-1 records for the planner.
- **I-3** For a device publishing a limit `L`, a pass that is not exclusive
  holds at most `max(L - 2, 0)` hunt events, and at most `L` leaves including
  the anchors.
- **I-4** An address stands at most once in `addresses()`, at most once in
  `entries`, and at most once in `skipped` for one run. Every planned address
  holds one `entries` record: an address that only stage 1 measured holds a
  record built from its stage 1 samples.
- **I-5** `rank` holds a value for at most `top` entries, the values of a top N
  are `1` to `k` with no gap, and an entry with `noisy` or `repeated` false has
  no rank.
- **I-6** The `cv` of an entry is one hundred times the `coefficientOfVariation`
  `statisticsOf` produces for the same list of samples, because it is the same
  call. The printed `cv=` column shows that ratio un-scaled, so an entry `cv`
  and the column differ by exactly the factor one hundred.
- **I-7** `planHunt`, `splitStages` and `rankEvents` read no clock, no counter
  and no `/proc`; every time figure they hold comes from an argument.
- **I-8** A run whose `RunOptions::hunt` stands empty leaves
  `BenchmarkResult::hunt` and `ReportData::hunt` empty, prints no `hunt:` line,
  and writes no `hunt` key.

## State tables

### Event disposition

| State | Entered when | Leaves to | Disposition |
|---|---|---|---|
| selected | `expand` accepts the address | seated, skipped | catalog order kept |
| skipped (discovery) | `avail` is not `COUNTABLE` | terminal | `SkippedEvent` with that kind |
| column | `avail` is `COUNTABLE` | seated, skipped | one `HuntColumn` |
| seated | the pass's `seatPass` succeeded | sampled | columns of that pass measured |
| skipped (retry refusal) | the pass fell back and the lone event refused too | terminal | `SkippedEvent` with the kind of the state the refusal names, run continues, exit 0 |
| sampled | the window closed clean | terminal | one `HuntSamples` row |
| skipped (`GAP`) | the window disclosed a gap | terminal | sample excluded, `SkippedEvent` |
| ranked | stage 2 or stage 1 produced samples | terminal | `RankedEvent` with or without a rank |

### Option value disposition

| Input | Result |
|---|---|
| `--hunt` with no value | selection `all` (D-1) |
| `--hunt=all` | selection `all` |
| `--hunt=none` | hunt disabled; the run behaves exactly like a run with no `--hunt` (FR-001, spec Assumptions) |
| list, pattern, `@file` | expanded after the scan, refusal on the first bad entry |
| `r4a03` in any selection form | refusal naming the value, before the catalog is consulted |
| `--hunt-budget=120s` | `{SECONDS, 120}` |
| `--hunt-budget=8p` | `{PASSES, 8}` |
| `--hunt-budget=120`, `0s`, `-5p`, `abc` | refusal, exit 2 |
| `--hunt-repeats=0`, `1001` | refusal, exit 2 |
| `--hunt-top=0`, `10001` | refusal, exit 2 |
| `--hunt-rank=sensitivity` | refusal naming the value and the accepted words |
| `--hunt` with `--counter`, `--metric` or `--compare` | refusal naming both options, exit 2 |
| repeated hunt option | last value wins |

### Run lifecycle

| Stage | Passes | Repeats | Output |
|---|---|---|---|
| plan | from `planPasses` | none | printed plan line |
| stage 1 (budgeted run) | every event | 1 | `HuntSamples` with `stage` 1 |
| stage 2 (budgeted run whose two-stage estimate fits) | the top `3 x top` set | `options.repeats` | `HuntSamples` with `stage` 2 |
| single stage (no budget) | every event | `options.repeats` | `HuntSamples` with `stage` 1 |
| rank | none | none | `EntryHunt`, `RunHunt`, console table; `entries` covers every planned column, a stage-1-only event through its stage-1-built record |

## Normalization and rank formulas

| Name | Definition |
|---|---|
| rate of one sample, `INSTR` | `eventValue / anchorInstr x 1000` |
| rate of one sample, `CYCLE` | `eventValue / anchorCycle x 1000` |
| rate of one sample, `ITER` | `eventValue / iterations` |
| `rate` of an event | median of its clean sample rates, the even-count median being the average of the two central values, which is the `statisticsOf` convention `statistics.cpp:39-40` carries and E-11 states |
| `cv` of an event | `sampleStdDev / mean x 100` from the same `statisticsOf` call |
| noisy | `cv` above `options.noise`, read on the unrounded value |
| rank order, `RATE` | `rate` descending, ties by address ascending |
| rank order, `CV` | `cv` ascending, ties by address ascending |
| estimate | `calibrationNsPerPass x passes x repeats`. The plan line of a split plan states the stage-1 figure: `calibrationNsPerPass` times stage 1's pass count times one repeat. |

A sample whose anchor reads zero contributes no rate for that pass and is
excluded, counted out the way a gap repetition is counted out of
`Statistics::samples`.

## JSON mapping

The in-memory name, the document key, and the reader path. Every per-entry key
is read through `optionalField` (`json_load.cpp:107`); a present key keeps its
own kind and its own refusal (hard rule 7).

| In memory | Document key | Kind | In `required` |
|---|---|---|---|
| `RunHunt` | top-level `hunt` | object | no, the root is open |
| `RunHunt::selection` | `hunt.selection` | string | no |
| `HuntOptions::norm`, `rank`, `top`, `noise`, `repeats`, `seed` | `hunt.norm`, `rank`, `top`, `noisePercent`, `repeats`, `seed` | string, string, integer, number, integer, integer | no |
| `RunHunt::plannedAddresses` | `hunt.events` | array of strings, the planned addresses in plan order | no |
| `RunHunt::stages` | `hunt.stages` | array of closed objects `first`, `count`, `repeats`: the first index into `hunt.events`, the event count, the repeats | no |
| `HuntBudget` | `hunt.budget` | string such as `120s`, or null | no |
| `RunHunt::passes`, `plannedAddresses.size()`, `ranEvents`, `stageTwoEvents` | `hunt.passes`, `planned_events`, `ran_events`, `stage_two_events` | integer | no |
| `RunHunt::method` | `hunt.method` | string | no |
| `RunHunt::estimateNs` | `hunt.estimate_ns` | integer, or null | no |
| `EntryHunt` | per-entry `hunt` | object | no, PD-20 records the exception |
| `EntryHunt::entries` | `hunt.entries` | array of objects | no |
| `RankedEvent::address`, `rate`, `cv`, `noisy`, `multiplexed`, `rank` | the same words | string, number, number, boolean, boolean, integer or null | no |
| `EntryHunt::skipped` | `hunt.skipped` | array of `{address, kind}` | no |
| `EntryHunt::method`, `passCount`, `top`, `retriedPasses` | `hunt.method`, `pass_count`, `top`, `retried_passes` | string, integer, integer, integer | no |

`RankedEvent::samples` and `repeated` stay in memory: PD-20 names the six entry
keys, and the console derives the `unrepeated` mark from `samples`. The full key
rule with its `x-order` positions is in
[contracts/report-schema-key.md](contracts/report-schema-key.md).
