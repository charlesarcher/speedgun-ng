# Research: Harness User Counters and Statistics

Phase 0 output for `specs/019-harness-user-counters-and-statistics/spec.md`.
Every entry follows Decision, Rationale, Alternatives considered. The
cited revision is `google/benchmark` `main` at `e662de9a`; every
`file:line` beside a decision was read at that revision or at the audit
point `a7776b6`.

## D-01: A fixed sixteen-slot counter store inside the state

**Decision**: One run's counters live in a `std::array<CounterSlot,
kMaxUserCounters>` of sixteen slots inside the state detail, each slot
holding a 32-byte inline name buffer, the double value, the flag set,
the base, and a used mark. `setCounter` scans, matches, or opens; the
seventeenth name and a thirty-third name byte each fail the run with a
reason that states the bound (FR-001, clarification 2026-10-14).

**Rationale**: Q-2 and FR-007 require opening and updating names inside
the timed loop with no allocation and no loop-shape finding; an inline
buffer with a linear scan over sixteen slots is two stores and a scan.
The thirty-two-byte bound keeps each slot a plain trivially-copyable
record, which is what lets the store sit in fixed storage.

**Alternatives considered**: the cited revision's `std::map` of
`UserCounters` (`counter.h:66`), rejected because every new name
allocates on the timed path; a heap array grown at the first set,
rejected because the first set is often the first loop iteration; a
registry-side store keyed at registration, rejected because counters are
body-opened names the handle cannot know in advance (FR-001).

## D-02: One flag pass after the loop, in the cited order

**Decision**: The runner applies the flags once per repetition after the
loop, in the order of the cited `Finish` (`src/counter.cc:22-44`): rate
over the row's primary time in seconds, thread average over one,
iteration-invariant times the iteration count, average-over-iterations
divided by it, and invert last. The rate divisor reads the decision
value of the run: CPU fold, real fold under `useRealTime`, manual sum
under `useManualTime` (FR-003, FR-004).

**Rationale**: the cited order is what SC-01 pins, and the decision
value is the same number the calibration and the aggregate already read
(`runner.cpp:463-466`, PC-2). On one thread the cited total-seconds
divisor and the per-iteration divisor give the same rate, so the spec's
per-row rule and the cited behavior agree (Assumptions).

**Alternatives considered**: applying flags at `setCounter` time,
rejected because the divisor does not exist until the loop ends and the
order of FR-003 would leak into the body; dividing rate by the printed
column after unit scaling, rejected because the value stays nanoseconds
internally, the standing scaling rule of the report (017 D-12).

## D-03: Disclosures are per row for counters

**Decision**: When the row's primary fold carries a gap disclosure or a
zero divisor, every counter column of that row keeps the flag-neutral
value, prints the disclosure token beside it, and the row stands aside
from every counter's aggregate statistics (FR-005, spec Edge Cases).

**Rationale**: the counters are divisors of that same primary time, so
one disclosure covers the whole set; the aggregate rule copies the shape
the time statistics already use at `runner.cpp:574-590`, keeping one
skip rule per row for the report to consume.

**Alternatives considered**: per-counter availability records shaped
like `MetricValue`, rejected because no rule distinguishes two counters
of one row; computing rates anyway and printing infinities, rejected by
the Edge Cases entry that names the zero divisor.

## D-04: One label string per row, one shared-label rule per aggregate

**Decision**: `setLabel` stores one string on the state; the runner
copies it to `ResultRow::label`; the report prints ` label=<text>` after
the name column when the string is non-empty; the aggregate keeps the
label only when every repetition's string is equal, matching the cited
`ComputeStats` (`src/statistics.cc:178-186`). A newline in the string is
a precondition violation at the call (FR-011, FR-012).

**Rationale**: the trailing field is the clarification of 2026-10-14,
and the newline bound keeps the one-line-per-row report shape the
report-shape checks assert. One string with one copy per repetition
costs nothing on a cold path.

**Alternatives considered**: appending the label into the instance name,
the cited name-composition shape (`benchmark_runner.cc:100`), rejected by
the clarification that keeps the name field byte-stable; a per-column
label table, rejected for a rule that names one field per row.

## D-05: A statistic is a (name, unit, function) triple on the family

**Decision**: `addStatistic(name, unit, function)` appends the triple to
the family record; the runner computes each statistic over the clean
repetition values of the primary time after `Statistics` is built; a
function that returns false prints nothing for that statistic while the
row still reports; a name equal to a built-in token (`mean`, `median`,
`sd`, `cv`, `min`, `max`) is a precondition violation at the call
(FR-013 to FR-015, spec Edge Cases).

**Rationale**: the cited `ComputeStatistics` adds exactly this triple
over the real-time values (`benchmark_api.h:137-139`,
`src/statistics.cc:195-200`); the clean-value set is the same list the
built-in statistics already consume, keeping one skip rule (D-03).

**Alternatives considered**: a statistic over counters too, rejected
because FR-013 names the repetition values of the primary time; storing
statistics per repetition, rejected because the statistic is an
aggregate-level value by the cited shape.

## D-06: Modes are family booleans consumed at print time

**Decision**: `reportAggregatesOnly` and `displayAggregatesOnly` set two
booleans on the family record behind the standing setter precondition.
The runner copies the effective restriction to `BenchmarkResult`, and
the report drops that result's repetition rows when the restriction
stands and an aggregate row exists (FR-016 to FR-018).

**Rationale**: while the console is the only report output the two modes
print the same trimmed console, which is exactly what the spec's FR-017
paragraph states; consuming at print time keeps every stored row intact
for the future file output that inherits the mode (FR-016).

**Alternatives considered**: deleting repetition rows in the runner,
rejected because the future raw-data output must keep them (Out list of
the spec Scope); a run-level option, rejected by Q-5's handle-only
decision.

## D-07: One closed-form relative-error fit per family

**Decision**: `complexity.cpp` computes, per family with a fit selected
and at least two usable instances, one least-squares coefficient per
curve by the closed form `k = sum(f(N)/y) / sum((f(N)/y)^2)` over the
instances, and the RMS as the root-mean relative error times 100.
`AUTOMATIC` tries every curve, keeps the least RMS, and falls back to
the constant curve (FR-021; `src/complexity.cc:90,133-163`).

**Rationale**: minimizing relative error is the cited rule, and the
closed form needs no matrix solver: each curve fits by one scalar.
Scripted test values reproduce the coefficient exactly, which is what
SC-05 asserts.

**Alternatives considered**: absolute-error least squares, rejected
because the cited fit and the RMS percentage of FR-022 are relative;
the cited `Vector` matrix solver, rejected as machinery a scalar
equation does not need (X.1).

## D-08: Instance N is the complexity-n value, falling back to the range value

**Decision**: An instance contributes to a fit with `State`'s
`setComplexityN` value when the body set one, and with the family's
range value of that instance otherwise. A family with no positive N for
an instance, or with fewer than two instances, reports the named reason
for the fit and prints no rows (FR-019, FR-023, D-09).

**Rationale**: the cited revision takes N from the argument list or from
`complexity_n` (`state.h:99-105`, `statistics.cc` usage), and speedgun
families are built by range expansion (H2), so the range value is the
problem size the sweep already prints in the instance name.

**Alternatives considered**: accepting only `setComplexityN`, rejected
because the spec's own scenario sweeps N through the family with no
call; aborting like the cited hard check, rejected by FR-023's reason
rule.

## D-09: One synthetic fit result per family carries every row pair

**Decision**: After a family's instances report, the runner assembles
one synthetic `BenchmarkResult` named after the family and gives it
`BIG_O` and `RMS` rows: one pair for the primary time, one pair per
named user counter present in every instance, and one pair per attached
metric (clarification 2026-10-14, FR-022). The quantity name rides the
row's `label` field; the coefficient rides the time field for the time
row and `counters[0]` otherwise; the RMS percentage prints unscaled with
a `%` token.

**Rationale**: the report consumes `BenchmarkResult` values in order, so
a synthetic result reuses the whole printing pipeline for family-level
rows; this is the cited shape where one BigO row rides per fitted
quantity (`src/complexity.cc:220-250`).

**Alternatives considered**: printing fits from a separate family-level
loop beside the results, rejected because the future file output would
need a second consumer; fitting only the time, the pre-clarification
default, replaced by the 2026-10-14 decision.

## D-10: The point bound follows the loop, so the allowance dies

**Decision**: A run's recorder points cannot exceed 2 + 2 × iterations,
because the loop shape allows at most one pause and one resume per
iteration; the runner reserves that many points per leaf per attempt
when it builds the plan between attempts. `kMaxPausePerRun`, its guard,
its capacity term at `runner.cpp:356-359`, and its tests die (FR-024,
FR-025).

**Rationale**: the bound is a theorem of the loop shape the loop-shape
gate pins, so it is as static as the old fixed allowance while stating
no cap a calibrated run can hit; the reservation stays outside every
timed loop.

**Alternatives considered**: an on-demand growing recorder, rejected
because an overrun is detected inside the loop, where allocation is
forbidden; keeping a larger fixed allowance, rejected by FR-024's
searchable removal and by a calibrated run that needs one pause per
iteration at any N (FR-026).

## D-11: The unit resolves at run start against the process default

**Decision**: `RegistryEntry` stores `std::optional<TimeUnit>`; the
registration-time capture dies. `speedgunMain` snapshots
`getDefaultTimeUnit()` at its start; each run resolves the result's
unit as the entry value, the batch snapshot, the process default read
at that run. A benchmark driven through the test hook without
`speedgunMain` reads the process default at its run start (FR-027).

**Rationale**: the shipped main-page sentence is the target behavior
(PC-4), and resolution at run start is the point where every registration
is already known and no printed value exists yet; the batch snapshot
keeps one run set consistent even against a mid-run default change.

**Alternatives considered**: stamping every entry at `speedgunMain`
start, rejected because the test-hook path bypasses `speedgunMain` and
FR-027 names its documented default; keeping the capture and fixing the
docs instead, rejected by the Q-7 decision recorded in the spec.

## D-12: One example file per section, and a gate that runs them

**Decision**: `example/docs/<slug>.cpp` per feature section of the main
page, eighteen files, each with its own `main` calling `speedgunMain`,
each added through `add_example` and listed in `run-examples`. The new
`test/example_run.sh` runs each and passes when every registered
benchmark of the file prints a result row, with no `FAILED` and no
`SKIPPED` row (FR-032 to FR-035). `docs_snippets.cpp` is deleted as its
regions move.

**Rationale**: the Q-9 recommendation adopted in the spec, and the run
check is the obligation FR-034 states: a snippet that drifts into a
fragment cannot hide behind a compiling-only block.

**Alternatives considered**: one merged snippets binary (the Q-9
fallback), rejected here because a reader's pasted block must show the
canonical includes and `main` of its own file; asserting only the exit
code, rejected because a `FAILED` row still exits clean today.

## D-13: The drift tool learns ownership, a sixth title set, and 550 lines

**Decision**: `tools/docs/example_drift.py` gains the six new section
titles to `SECTIONS`, raises `PAGE_LINE_CAP` from 400 to 550 while the
25-line `SECTION_LINE_CAP` holds, and checks that each page snippet
region is a marked region of the example file named for its section
(FR-028, FR-044). The D1 budget note in `specs/018-docs-main-page/spec.md`
is annotated superseded in the same commit as the constant.

**Rationale**: the README stands at 394 lines against the 400 cap, and
six new sections plus the C-8 additions cannot fit below 550 with the
snippets intact (Q-8 evidence in the spec); ownership is what turns
"compiles somewhere" into "runs in CI" that IV.1 now requires.

**Alternatives considered**: trimming prose to stay under 400, rejected
by Q-8's recorded decision and by the reader cost; a second drift tool,
rejected because one tool must own one page.

## D-14: The constitution amendment of FR-036

**Decision**: Amend IV.1 so a main-page snippet must come from an
example CI builds and runs, and name `example_run` in Principle VIII's
example gate. Bump the constitution to 2.20.0 with the footer, the
amendment record row, and the lineage row, the same shape 2.19.0 shows
for D1.

**Rationale**: FR-036 requires it, and the precedent of the same file
(2.19.0 added IV.1 itself) fixes the record format; a plan may not carry
a gate change silently.

**Alternatives considered**: leaving IV.1 at compiles while CI runs
more, rejected because the constitution is the authority the review
reads; a patch bump, rejected by the amendment history's minor-bump
pattern for added obligations.
