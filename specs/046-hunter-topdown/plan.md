# Implementation Plan: Hunter Top-Down Area

**Branch**: `046-hunter-topdown` | **Date**: 2026-11-03 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/046-hunter-topdown/spec.md`

**Mode**: Test-driven. FR-029 puts each new unit in the same commit as the code it
covers, and each unit reads red at `b1dd7670` before that code lands.

## Summary

Hunter mode ranks catalog addresses. This feature names the top-down area of each
ranked event: the deepest node of the running host's `N10` tree whose formula uses
the event, else the vendored list file that holds the event, else `unmapped`. One
`area=<path>` cell joins the ranked row, one `topdownArea` key joins each event of
the report document, one read-only counters accessor publishes the vendor grouping,
and the same run prints its level-1 top-down block after the ranked rows. No counter
path, no formula grammar and no new option appear (D-1, D-2).

## Technical Context

**Language/Version**: C++23, the standard the tree pins (`CMakeLists.txt`)

**Primary Dependencies**: the counters library through `include/speedgun-ng/counters_topdown.hpp`,
the vendored `external/pmu-events` tables, `hwloc` for the host topology, and the
`N10` selection path of specs/042

**Storage**: the report document (`docs/schema/report-v1.json`), two committed
console and document fixtures under `test/data/`, and the docs pages

**Testing**: frameworkless units with their own `main` and a scenario table, driven
by `ctest` tiers, plus the gate scripts `tools/ci/prepush.sh`,
`tools/docs/example_drift.py`, `tools/prose/prose_gate.py` and the two DBC gates

**Target Platform**: Linux only. `ci-rocky` (GCC 16.2.1) and `ci-ubuntu` (clang with
clang-tidy and cppcheck) are the checked-in release presets. macOS and Windows stay
unsupported.

**Project Type**: library plus a benchmark harness binary, with a generated docs page
set

**Performance Goals**: the area adds no measurement and no input/output. The second
`topdownTree` ask of a process answers in 0.57 ms from the read the run already pays
for (AP-12), and no hunt rank, budget or emitted byte moves (FR-010).

**Constraints**: 100 percent line coverage with the branch shortfall at 21 or lower
(`tools/dbc/coverage_gate.sh:102`), no new exclusion marker, no clock read or sleep in
a new unit, no invented cpu device, and no `perf_event_open` from a new unit
(FR-030, FR-031).

**Scale/Scope**: two new harness files, one new counters accessor in one existing
file, one public header pair, six harness source files touched, one schema, one
example, two docs pages, and four version bindings (FR-028).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate (Principle VIII) | Reading for this change | Status |
|---|---|---|
| Preset builds, Linux, GCC and Clang | `ci-rocky` and `ci-ubuntu` build this tree at the base with no finding | design passes, measured at base |
| All tests pass | the base answers three failures, all in files the 045 lane owns (AP-1), so SC-005 is a base-to-head name diff | design passes |
| ASan/UBSan clean (`ci-sanitize`) | the mapper reads three in-memory records and opens nothing, so no new path reaches a sanitizer fault | design passes |
| TSan clean (`ci-tsan`) | the companion pass runs on the run's own thread through the runner that `N10` already drives | design passes, measured by `ci-sanitize` and the `tsan` tier of `tools/ci/prepush.sh --full` |
| clang-tidy and cppcheck | CI fails a finding that names a check in `WarningsAsErrors`, and the constitution also asks for no new finding of any check, so the new pair is written to the baseline: designated initializers, no magic number in a comparison, trailing commas, no varargs, no raw pointer arithmetic | design passes, delta measured |
| `readability-identifier-naming` zero | the base answers 0 of that check, and the new names follow the pinned cases | design passes |
| `format-check` and `spell-check` | `cmake -P cmake/lint.cmake` and `cmake -P cmake/spell.cmake` answer 0 at the base | design passes |
| Prose (XI) | the base tree answers 0 findings over 966 sources, and the branch point range is a void run, so every prose claim names its form (AP-2b) | design passes |
| Time-source clean | the mapper reads no clock, so `test/time_source_gate.sh` stays green | design passes |
| Page-drift clean | the new example block rides `docs/pages/harness.md`, where no cap reaches it, and `README.md` gains one quick-reference row at 1001 of 1004 lines; `SECTION_LINE_CAPS` stands unchanged (FR-025, FR-026) | design passes, measured in a copy of the tree |
| Coverage (VI) | the mapper is pure, so every arm reaches a unit case, and the two new files carry no exclusion marker | design passes |
| Docs shape gate | `tools/docs/report_shape.py` gains no fold rule. FR-027 rules that no recorded region holds a real level-1 block, because that block carries a table path, a kernel reference and digests whose count the table owns. The example registers its own recorded region as its machine (FR-025, FR-027) | design passes, no tool work in scope |
| Performance (VII) | one extra tree ask costs 0.57 ms and one short level-1 pass joins the run beside the stages (AP-12, D-9) | design passes |
| Gate configuration | no cap, allowance, omit pattern or rule file moves | design passes |

Design By Contract (II) and R-DCUT (III) hold by shape: the mapper is a pure function
over three inputs, its preconditions and postconditions appear in
`contracts/area-mapper.md`, and the grouping accessor is a read of build-time data.
Anti-slop (X) holds: the mapper adds no abstraction above the tree it reads, and the
one new public function replaces no existing path. Principle XI holds in every line
this plan writes.

**One scope amendment, an addition the request implies.** The request allowed read-only use of
`include/speedgun-ng/counters_topdown.hpp`, and D-16 adds one public read-only
accessor there. The cost is one declaration, one definition, and archive SOVERSION 5
to 6. The refused alternative was a second vendored-json reader inside the harness,
which would parse the same vendor data a second time. The Scope section of the spec
carries the amendment, and the pull request body names it.

## Project Structure

### Documentation (this feature)

```text
specs/046-hunter-topdown/
├── plan.md                    # this file
├── research.md                # Phase 0: decisions D-1 to D-17 with their evidence
├── data-model.md              # Phase 1: records, values, index, document shape
├── quickstart.md              # Phase 1: clone to a screen that shows the cell
├── contracts/
│   ├── area-mapper.md         # the pure function and its unit cases
│   ├── counters-grouping.md   # the accessor, its doc form, its arms
│   ├── console-cell.md        # the cell, the one-block rule, the bytes that stand
│   └── document-key.md        # the key, the reader rule, the specs/047 rebase point
├── checklists/requirements.md # requirements quality pass
└── tasks.md                   # Phase 2 (/speckit.tasks)
```

### Source Code (repository root)

```text
include/speedgun-ng/
├── hunt.hpp                        # RankedEvent gains topdownArea after delta
└── counters_topdown.hpp            # TopdownEventGroup and topdownEventGroups

source/harness/
├── hunt_topdown.cpp                # NEW: the mapper, the index, the stamp
├── detail/hunt_topdown.hpp         # NEW: internal declarations, no installed header
├── cli.cpp                         # stamp pass, refusal line, one-block rule
├── report.cpp                      # area cell, companion print, suppressed call
├── json_report.cpp                 # kSchemaVersion, the written key
├── json_load.cpp                   # the key read (specs/047 rebase point)
└── detail/internal.hpp             # RunOptions seam for the planted identity

source/counters/linux_pmu/
└── topdown_select.cpp              # the grouping accessor, one function

test/source/
├── harness_hunt_topdown_test.cpp   # NEW: mapper, index, stamp, arms
├── counters_topdown_select_test.cpp# grouping accessor cases
├── harness_hunt_report_test.cpp    # cell bytes, one-block rule
├── harness_hunt_json_test.cpp      # key round trip, older document, version pins
└── data/hunt_no_hunt_*             # two fixtures that change by zero lines

example/docs/
├── hunt_topdown.cpp                # NEW: the area example region
└── hunt_ranks.cpp                  # unchanged, re-homed block on harness.md

docs/
├── schema/report-v1.json           # 1.14, x-order and properties, not required
└── pages/harness.md                # hunter subsections, the new example block

tools/docs/
├── report_shape.py                 # the fold rule for the level-1 block
└── (fixtures)                      # the planted pair the rule needs

CMakeLists.txt                      # version 0.31.0, SOVERSION 21 and 6
README.md                           # one quick-reference row, no new block
```

**Structure Decision**: the mapper lives in the harness, in a new pair under
`source/harness/`, because the area is a reading of a report record and no other
component reads one. The level-1 companion keeps its own internal option record, so
no line of the process option record moves (FR-035). The vendor grouping comes from the counters library, which
already parses the pinned tables, and reaches the harness through one read-only
function (D-16). The `detail` directory keeps the mapper out of the interface the DBC
gates count, so the count moves by two interfaces and not by more. No new target,
directory or dependency appears.

## Complexity Tracking

No constitution gate needs a justification to pass, so this table stays empty. Three
items a reader may mistake for a violation are recorded above: the D-16 scope
amendment, the docs-page home that avoids a cap change, and the level-1 block that
stands outside every recorded region, which is why the shape tool gains no rule.


---

## Acceptance plan: Intel final acceptance (2026-11-07) - the hunter part

The shared plan, the command table and the risk list stand in
`specs/048-hwlat-probe/plan.md`. This part runs two commands, three times each, on a
tree built at the measured head, and writes one record.

- **Commands**: `./build/example/memory_walk --hunt --hunt-budget 120s` and
  `./build/example/first_benchmark --hunt --hunt-budget 120s`, the pair that
  `results/acceptance-hunt.md` records. No `--timeout`, and no method word in the
  normal runs.
- **What each run leaves**: the console text, the exit code, the `events=` summary
  line, the plan line with its method word, every `area=` cell, the `topdown:` block
  with its `level=1` line and its file lines, and the count of lines the run printed.
- **What the record may say**: the three counts over the printed rows with their
  arithmetic, the distinct cell texts with a count each, the table the block names with
  the digests it prints, and the mapping of each figure to SC-007, SC-010 and AP-9.
- **What it may not say**: that the run read a level no line prints, that a rank order
  holds across runs, that an AMD, Arm or hybrid figure stands for an Intel reading, or
  that a base re-run happened. A base build stands out of scope and the record names it
  as not run.
- **Failure route**: where the normal method fails, the record carries the failure with
  its exit code before any separately labeled alternative.


## Acceptance closeout: measured Intel evidence (2026-11-08) - the hunter part

**Spec**: [spec.md](./spec.md), `IH-001` and `IH-002`. The shared context, the
order of work and the lane table stand in
[../048-hwlat-probe/plan.md](../048-hwlat-probe/plan.md) under
`Plan: Acceptance closeout, measured Intel evidence (2026-11-08)`. This section
names only what the hunter record carries, and repeats no command from that
table.

### What the closeout runs here

- Three normal-method consoles per command at the final head, each alone on an
  idle host with its load witness printed: the two hunt commands of
  `results/acceptance-hunt.md:14-16`. A run through an alternative console is
  supplementary and never counts toward the three (048 ruling 12).
- Per command, the rank movement on two denominators: among the 20 ranked rows,
  and across every printed row (048 ruling 15). A cross-binary comparison gets
  its own labeled line, so a within-binary spread never reads as a cross-binary
  one.
- The `topdown: level=` line the run prints, at the value the binary prints, and
  the 13 `topdown: file` digest lines at their full length from the binary
  (048 ruling 18). The ask levels stand named where the record discusses them:
  `kTopdownAreaLevel` value 6 at `source/harness/detail/hunt_topdown.hpp:42`,
  and the companion pass at level 1.
- No hardware row runs a tracer, and no figure from `87d745d6` or `cbeb678c`
  carries into a row that names the closeout head (048 ruling 10).

### What the closeout corrects in this record

- `results/acceptance-hunt.md:4-5` and `results/pr-body.md:56-60` say every run
  "ranked 497 events". A run ranks `min(top, ranked)` rows with `top` at 20
  (`include/speedgun-ng/hunt.hpp:135`, `source/harness/hunt_plan.cpp:371-374`)
  and prints the rest at `hunt: rank -` (`source/harness/report.cpp:1478`). The
  replacement states 497 printed rows, 20 of them ranked.
- `results/acceptance-hunt.md:39-40` states a rank-to-class reading that does not
  hold across the captures. The replacement names the captures that hold it and
  the ones that do not.
- `results/acceptance-hunt.md:57-58` cites `source/harness/cli.cpp:521` for the
  block's `not available`. That line is the jitter answer; the block's line is
  `source/harness/report.cpp:404-411`.
- `results/acceptance-hunt.md:45-46` describes the 13 digests instead of printing
  them. The closeout section prints them.
- `results/acceptance-final-intel.md:159-160` and `:210-212` carry pending rows
  and no Intel row for `PERF_METRICS`. The closeout adds it as pending, with the
  reason.
- Every table row a closeout writes names the criterion it answers (IH-002).
  Where no `SC-` id owns the claim, the row names the audit point or ruling that
  does (048 ruling 16).

### Verify

Three consoles per command with their own capture path, exit and witness; the
level line and 13 digests printed; both denominators reported per command; one
grep per correction, each answering the replaced text absent and the replacement
present.

## Acceptance closeout at the merged head (2026-11-08) - the hunter part

**Spec**: [spec.md](./spec.md), the section `Hunter work in the merged-head
closeout (2026-11-08)`, with `IH-001` and `IH-002`. The shared rules for this
phase stand in [../048-hwlat-probe/spec.md](../048-hwlat-probe/spec.md) under
`Intel acceptance closeout at the merged head (2026-11-08)`: the ten lanes of
`The final-head set`, the two passes of ruling 29 of this phase's list, the
load witness of ruling 30 of that list, the two bounded widths `IC-003`
states, and the seven entries of `Files this phase may change`. This section
adds the hunter rows, and each row names the ruling or the requirement that
owns it.

Ruling 29 of this phase's list names two heads, so every figure row of this part
names the head Pass 2 runs at and the head the pull request carries, and prints
that ruling's markdown-only proof clause:
`git diff --name-only <head Pass 2 ran at> <head the pull request carries>`
prints files under `specs/` alone, which leaves every figure in place, and a diff
that names any other path voids the figures and forces a fresh Pass 2.

A cite into a spec or a plan from this section names that file, and then the
requirement id, the ruling number and its list, the heading, or the row name. A
numeric line range is the last choice, and it stands only for source code and
for a record row no id names. `specs/048-hwlat-probe/spec.md` holds three
numbered ruling lists: the 2026-11-07 list of 1 to 43 under `Session
2026-11-07, round 1` and `Session 2026-11-07, round 2`, the 2026-11-08 closeout
list of 1 to 28 under `Session 2026-11-08, round 1` and its two continuations,
and this phase's list of 29 to 42 under `Rulings - this phase (29 to 42)`. One
bare ruling number lands in more than one of them, so every ruling cite names
the ruling and the list it stands in, and a list name carries that file with
it. The ids that start `IC-` and `SC-IC-` belong to that file.

### The base, and the re-take it forces

| Line | Value at the check on 2026-11-08 | Owner |
|---|---|---|
| Base head | `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the master head that carries pull request #99 and pull request #100 (`Intel acceptance closeout at the merged head (2026-11-08)` of `specs/048-hwlat-probe/spec.md`) | `IC-001` |
| Final head | `no figure`. Ruling 29 of this phase's list names the two heads: the squash after the Pass 1 writes produces the head Pass 2 runs at and every figure names, and the squash after the Pass 2 writes produces the head the pull request carries. Neither squash of `fix/intel-acceptance-closeout` stands made at this writing, and a record prints the short sha of the head Pass 2 ran at beside each console it ran | ruling 29 of this phase's list, `IC-007` |
| Host | `corsairone`, Intel Core i9-9920X, Skylake-X, non-hybrid, one socket (the `Host` row of `The base this work reads` in `spec.md`) | `IC-001` |

Pull request #100 moved the ten paths that section names, and two of them sit
in the harness path a hunt console runs through.
`source/harness/execution_context.cpp` holds the barrier wait arms, and a run
reaches them through the contexts the runner builds at
`source/harness/runner.cpp:1958`. The second change holds the spectrum answer:
`spectrumAnswer` at `source/harness/detail/jitter_probe.hpp:226`, filled at
`source/harness/jitter_probe.cpp:1731`, entered through `addSpectrumTo` at
`source/harness/jitter_probe.cpp:1683`, and read by the report path at
`source/harness/report.cpp:903-909`. Both commands therefore run again at the
final head (the `The base this work reads` section of `spec.md`).

A console prices one pass in the `estimate=` field of its plan line, and the run
that pays for that pass reaches the wait arms through its own contexts, so the
estimate, the rate figures and the rank figures of a console all stand re-taken
at the final head. The standing record prints three `estimate=` figures that
differ across three consoles of one binary at head `615c0964`
(`results/acceptance-final-intel.md:39-46`), and that reading keeps its head as
its label here (ruling 35 of this phase's list).

| Pass | Head | Hunter duty | Owner |
|---|---|---|---|
| Pass 1 | a candidate head | the six consoles may guide edits, and a Pass 1 figure enters a record row labeled with the head that printed it (`IA48-039` of `specs/048-hwlat-probe/tasks.md` lands it there), and Pass 2 replaces it | ruling 29 of this phase's list, `SC-IC-7`, `IA48-039` |
| Pass 2 | the head Pass 2 runs at, which the squash after the Pass 1 writes produces | the six consoles answer every row below, and each record prints that head's short sha beside its console and names the head the pull request carries beside it | ruling 29, `IC-007` |

A tracked change after Pass 1 forces a new squash, and that squash produces the
head Pass 2 runs at again (ruling 29 of this phase's list). A figure measured at the earlier base
`2d4d8c57` keeps that sha as its label and answers no row here (ruling 10 of the
2026-11-08 closeout list), and each figure names the head whose run answered it
(ruling 35 of this phase's list). The lane builds no earlier head (ruling 15 of
the 2026-11-07 list).

### The commands and their working directory

```sh
./build/example/memory_walk --hunt --hunt-budget 120s
./build/example/first_benchmark --hunt --hunt-budget 120s
```

Both strings copy verbatim from `results/acceptance-hunt.md:15-16`, and the
shared lane table carries the same pair in its `Hunt, walk` row and its
`Hunt, benchmark` row. Each command runs three times at the
final head in a release tree built from that head, and the normal method stands
first in each set of three (`IH-001`; ruling 12 of the 2026-11-08 closeout
list).

| Command | Working directory | Consoles | Owner |
|---|---|---|---|
| `./build/example/memory_walk --hunt --hunt-budget 120s` | the checkout root that holds the release tree `build`, because the relative path in the command resolves there | `walk-1`, `walk-2`, `walk-3` | `IH-001` |
| `./build/example/first_benchmark --hunt --hunt-budget 120s` | the same checkout root | `bench-1`, `bench-2`, `bench-3` | `IH-001` |
| `cat /proc/loadavg` | the same checkout root, read at the start of the console it witnesses | one line beside its console | ruling 17 of the 2026-11-08 closeout list |

Each console writes its capture to a file outside the repository, and the record
prints that path as the run printed it (`IC-007`). Where the
normal method fails, the record carries that failure with its exit code before
any alternative, and an alternative console joins no set of three (`IH-001`;
ruling 12 of the 2026-11-08 closeout list).

### The reading pairs the record must print

No hunter console has run at the final head, so every figure below reads
`no figure`. This table is the one place that states the eight reading pairs,
and `Rows the final head must answer` of `spec.md` names these rows by number.
The second command answers the same eight rows on its own three consoles.

| Row | Pair of readings, each from one set of three consoles | Run that answers it | Figure | Owner |
|---|---|---|---|---|
| 1 | The count of rows the run printed, and the count of rows that carry a rank | the three Pass 2 `walk` consoles | `no figure` | `IH-001` |
| 2 | The count of `area=` cells over printed rows, and the count of `area=` cells over ranked rows | the same three consoles | `no figure` | `FR-013`, `FR-014` |
| 3 | The node, list-group and unmapped counts over printed rows, and the same three counts over ranked rows, each beside the `level=1` companion block lines | the same three consoles | `no figure` | `SC-007`, AP-9 |
| 4 | The `topdown: level=` line with its `passes=` and `table=` fields, and the `topdown: reference=` line with the `topdown: file` lines that follow it | the same three consoles | `no figure` | `IH-001`, ruling 18 of the 2026-11-08 closeout list, `IA46-002` |
| 5 | Rank movement among the ranked rows, and rank movement across every row the run printed | the same three consoles, read pairwise | `no figure` | ruling 15 of the 2026-11-08 closeout list, `IH-001` |
| 6 | The spread of rank positions within one binary, and the spread between the two binaries, each on its own labeled line | the same three consoles beside the three `bench` consoles | `no figure` | ruling 15 of the 2026-11-08 closeout list |
| 7 | The `method=` word of the console's plan line, and the `estimate=` figure of that same line | each console of both commands | `no figure` | ruling 16 of the 2026-11-07 list, ruling 35 of this phase's list |
| 8 | The exit code of each console, and the count of lines that console printed | each console of both commands | `no figure` | `SC-IC-7`, `IH-001` |

Row 4 cites no digest count. `source/harness/report.cpp:381-384` prints one line
per file the record carries, and
`source/counters/linux_pmu/topdown_select.cpp:122-129` admits a file only where
that file contributes a row of the table the host loaded, so the row prints the
lines the binary printed (the `IH-001` restatement under `Requirements restated
for the merged base` in `spec.md`). Row 5 and row 6 state a spread, and `IH-001`
bars a stable-ranking claim. The standing spread at
head `cbeb678c` reads 75 of 497 positions between two runs of one binary
(`results/acceptance-hunt.md:70-71`), and that reading keeps its head label.

### The two readings of one console

- Reading one names every row the run printed. A row that carries no rank also
  reaches the console, and it opens with the text `hunt: rank -` at
  `source/harness/report.cpp:1478`.
- Reading two names the rows that carry a rank. That count stands at or below
  the `--hunt-top` default of 20 (`include/speedgun-ng/hunt.hpp:135`), and
  `source/harness/hunt_plan.cpp:371-374` decides which addresses earn one.
- Row 5 states movement on both denominators, and each denominator carries its
  own number (ruling 15 of the 2026-11-08 closeout list of
  `specs/048-hwlat-probe/spec.md`).
- The standing split at head `cbeb678c` reads 58 node paths, 412 list groupings
  and 27 unmapped over 497 printed rows (`results/acceptance-hunt.md:29-41`).
  It answers no Pass 2 row (ruling 35 of this phase's list).

### The area cell and its condition

`source/harness/report.cpp:1471` prints the cell on a ranked row, and
`source/harness/report.cpp:1488` prints the same cell on the row that closes
with the `mark=` cell at `source/harness/report.cpp:1489`. The body at
`source/harness/report.cpp:1314-1319` prints the cell only where the record's
`topdownArea` member holds a value, so an empty member prints no cell
(`FR-014`). The two counts of row 2 answer two questions, and each stands over
its own rows (`FR-013`; `IH-001`).

### The load witness

- Each console prints `cat /proc/loadavg` at its start, beside a count of the
  other compiler, test, capture and trace processes on the host. `spec.md`
  states that shape under `Hunter work in the merged-head closeout (2026-11-08)`,
  and ruling 30 of this phase's list sets it.
- A figure stands where that count reads zero and the one-minute load average
  reads below 2.0. Any other witness voids the figure, and the lane re-takes that
  console alone with its own witness beside it (ruling 30 of this phase's list,
  and ruling 17 of the 2026-11-08 closeout list of
  `specs/048-hwlat-probe/spec.md`).
- No mask stands in either command, and both copy verbatim from
  `results/acceptance-hunt.md:15-16`. The two bounded widths belong to other
  lanes: `taskset -c 0,1` with `-j 2`, and `taskset -c 0,1,2,3` with `-j 4`
  (`IC-003`; the `Two-cpu release` row and the `Four-cpu release` row of the
  `Lane Table` of `specs/048-hwlat-probe/plan.md`).

### The pending hardware rows

`SC-010` owns the class, and `IH-002` names the criterion beside each row.

| Pending row | Part it waits for | Reason it stays pending | Owner |
|---|---|---|---|
| Intel parts measured through `PERF_METRICS` | a host that carries the `PERF_METRICS` facility | no host on this lane carries the facility | `SC-010`, `IH-002` |
| Intel hybrid parts | an Intel hybrid part | the standing host is non-hybrid (the `Host` row of `The base this work reads` in `spec.md`) | `SC-010`, `IH-002` |
| Arm parts | an Arm host | no Arm host stands in reach | `SC-010`, `IH-002` |
| AMD Zen 4 parts | an AMD Zen 4 host | no Zen 4 host stands in reach | `SC-010`, `IH-002` |

Each row keeps its synthetic seam rows and carries no hardware figure
(`IH-002`; the `Pending rows this base keeps` section of `spec.md`). Zen 5
stands measured under `SC-009` at head `858c55d9`, and that reading keeps that
head as its label (ruling 35 of this phase's list). This base moves none of
these rows (that same section of `spec.md`).

### The anchor re-read

`IC-006` asks every code anchor inside a dated row to stand
re-read at the measured head, because pull request #100 moved two anchors this
branch already cites, one in `source/harness/detail/internal.hpp` and one in
`source/harness/jitter_probe.cpp`. The hunter record cites the first kind at
`results/acceptance-hunt.md:201`, and the Pass 2 pass prints the line the tree
answers there. Owner: `IC-006`, `IH-002`.

### What this part leaves untouched

- No source file, test file, gate script, preset or CI file moves, because the
  hunter entry of `Files this phase may change` of
  `specs/048-hwlat-probe/spec.md` names the markdown artifacts of
  `specs/046-hunter-topdown`.
- No kernel control changes: `perf_event_paranoid`, `nmi_watchdog`, the tracefs
  mode and the `msr` mode stay as the host holds them (`Out of scope for this
  phase` of `specs/048-hwlat-probe/spec.md`).
- No timeout, ceiling, retry, coverage marker, allowance or suppression moves
  (that same `Out of scope for this phase` list).
- No automatic rank choice, which `G-32` leaves as future work in the
  `Scope (phase 13)` list of `specs/045-merge-fixes/spec.md`, and no raw event
  code, which `G-31` leaves as future work there and under `Out of scope for
  this phase` of `specs/048-hwlat-probe/spec.md` (`IH-002`).
- The `harness_hunt_cli_test` scenario and the `hunt: plan events=` reading it
  answers stand in the rows `IC-004` owns, in
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, and this part cites
  them and rewrites none of them (ruling 9 of the 2026-11-08 closeout list, and
  the citation duty under `Files this phase may change` of
  `specs/048-hwlat-probe/spec.md`).
- No figure moves onto a Pass 2 row from `2d4d8c57`, `92bacc03`, `615c0964`,
  `87d745d6`, `858c55d9` or `cbeb678c` (ruling 35 of this phase's list; ruling
  10 of the 2026-11-08 closeout list).

### Verify

Six Pass 2 consoles, each with its own capture path, exit code, line count and
witness line; eight reading pairs per command, each answered or reading
`no figure`; both denominators per command with their own movement numbers; the
level line, the reference line and the digest lines at the length the binary
printed them; four pending rows with their part and their reason; one grep for
the re-read anchor.
