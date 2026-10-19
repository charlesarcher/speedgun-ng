# Implementation Plan: Metric Formulas

**Feature branch**: `021-metric-formulas` | **Spec**: [spec.md](./spec.md)

**Audit point**: `ab32400b86321f98f13b9c66d4488e15831b47f6`

**Date**: 2026-10-19 | **Constitution**: 2.20.0, unchanged by this
plan

## Summary

The vendored performance-monitoring tables carry 5958 rows with a
`MetricExpr`, and the table loader drops every one of them: the first test
in `addEntry` returns early for a row with a metric expression or a metric
name. This feature makes those rows countable. It parses the formula
language the pinned tree uses, resolves a metric reference to the
events underneath it, compiles the result onto the expression machinery
the library already folds, and publishes one entry point that answers a
metric by its perf name. The harness lists those metrics in
`--catalog` and turns one named on its command line into a report column,
and the page gains a copied block that CI builds and runs.

Two supporting pieces come with it. A pinned conversion table turns the
`ScaleUnit` text into the factor the value carries. The mapping-file key
gains the CPU stepping, the defect that leaves a model-55 part with no
selected table at all, because a metric feature that lists nothing on its
development host cannot be accepted on it.

The plan holds to the counters lane. Public API grows by one header of
additive declarations and two appended node kind codes inside
`detail::ExprCore`, which no existing caller can produce. `MetricResult`
keeps its four members, `Availability` keeps its six values, the counters
archive `SOVERSION` stays 2 and the harness `SOVERSION` stays 6.

## Technical Context

**Language/Version**: C++23, the pinned standard of Principle I. The
parser is header-internal C++, no new language dependency, no new third
party dependency for the grammar.

**Primary Dependencies**: the counters library at the audit point
(`detail::ExprCore`, `Expression<D>`, `compileCore`, `metricCore`,
`foldPairsCore` in `include/speedgun-ng/counters_measurement.hpp`), the
vendored table tree under `external/pmu-events/arch/x86` reached through
`embedded_tables.hpp`, and the object tree and calibration facts the
library already publishes for the literal constants.

**Storage**: no filesystem read at run time. The tables stay embedded; the
mapping file stays the pinned copy the build embeds. A parsed metric lives
in the per-directory table image the loader already caches.

**Testing**: CTest executables in `test/source`, named on the
`counters_*` and `harness_*` pattern, plus the shell gates under `test/`.
Every figure a test asserts comes from `include/speedgun-ng/counters_fake.hpp`
(`FakeScript`, `FakeWindow`, `FakeCounterData`) or from the pinned table
itself. No `sleep`, no wall clock, per the spec's Additional Constraints.

**Target Platform**: Linux, the CI matrix of Principle VIII: GCC and
Clang, sanitizers, coverage. The selection logic is exercised on x86 by
fixtures for both sides of the model-55 stepping boundary.

**Project Type**: library plus the harness archive, with the documentation
example under `example/docs/` that the drift gate compares with the page.

**Performance Goals**: all parsing happens once per directory at load, so
a metric read costs what a counter read costs today and `compileCore`
sees a spine of the same shape it sees now. Loading every metric of the
`skylakex` directory, 279 rows, stays inside the load-time budget the plan
states in D-14, and the timed loop keeps its current instruction count.

**Constraints**: no existing public declaration changes shape; two node kind
codes are appended inside `detail::ExprCore`, which is additive and
unreachable from an existing caller. `MetricResult`
keeps `value`, `runningRatio`, `availability` and `scaled`. The counters
archive `SOVERSION` stays 2 and the harness `SOVERSION` stays 6. The
feature touches no file owned by the percentile feature on gary, the
topology and NUMA features on thrawn, or N11: the shared list in the spec's
Constraints section is the boundary. 100% line and branch coverage with no
exclusion marker.

**Scale/Scope**: 42 requirements across 6 capabilities, 5 preconditions,
9 success criteria, 14 research decisions, one new public header, one new
parser translation unit, three harness files, one example and one page
section. The pinned tree holds 46 directories, 5958 expression rows, 267
conditional rows, 275 rows with a literal constant, 707 rows with a
backslash escape, 250 `max` calls, 190 `min` calls and 74 `d_ratio` calls.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **I, standard-first with P0-P3**: PASS. The parser uses the standard
  library only. `std::from_chars` carries the numeric literals and no
  locale enters the path.
- **II, Design By Contract**: PASS. Every parse entry point states its
  preconditions on the input range and returns a refusal object on a
  violation, the pattern `test/dbc-gate-fixture` checks. A reference to a
  metric of the same directory is the caller's contract with the table, so
  a cycle is a load-time refusal with the cycle named.
- **III, R-DCUT**: PASS, TDD mode. Each new test file is written first,
  observed failing, then made green. The failing observation is recorded
  per phase in `tasks.md`.
- **IV, documentation**: PASS. The page block comes from
  `example/docs/counter_metrics.cpp`, which CI builds and runs, so the run
  obligation of IV.1 holds. The section stays within `SECTION_LINE_CAP`.
- **V, style**: PASS. No new naming scheme: the parser types live in
  `detail` inside the counters library and the public entry point reads
  like its neighbours in `include/speedgun-ng`.
- **VI, coverage**: PASS by construction. Every refusal branch is driven
  by a fixture and every accepted construct is driven by a sweep over the
  pinned rows, so no branch is unreachable. No `LCOV_EXCL` marker, no
  waiver, no residual row.
- **VII, performance discipline**: PASS. Parsing is load-time. The plan
  records the load budget in D-14 and the check that reads it.
- **VIII, hard gates**: PASS. `counters_header_purity.sh`, `cli_shape.sh`,
  `loop_shape.sh`, `time_source_gate.sh`, `example_run`,
  `example_run_fixtures`, `snippet_compile`, the pmu-events gate fixture
  and the prose gate all stay green; `cli_shape.sh` gains the new option
  row in step with `README.md`.
- **IX, spec-driven**: PASS. This plan follows `spec.md` with its five
  clarifications, and `tasks.md` covers every FR.
- **X, anti-slop**: PASS. No abstraction without a caller: the parser has
  one caller, the loader, and no extension point beyond the fact table
  literal constants need.
- **XI, prose**: PASS. `tools/prose/prose_gate.py` reports zero findings
  over these artifacts.

**Gate weakening requested**: no. **Gate strengthening requested**: the
stepping-class sweep of FR-036 joins `counters_pmu_test.cpp`, which the
pmu-events gate already drives, so a re-pin that adds a stepping class fails a
test the matrix already runs.

## Project Structure

### Documentation (this feature)

```text
specs/021-metric-formulas/
├── spec.md                   # Requirements, clarifications of 2026-10-19
├── plan.md                   # This file
├── research.md               # Phase 0: D-01 to D-14
├── data-model.md             # Phase 1: formula image, refusal kinds, tables
├── quickstart.md             # Phase 1: the shortest path a reader takes
├── contracts/
│   ├── metric-by-name.md     # C-1 and C-2, the parse and lookup seam
│   ├── catalog-metrics.md    # C-3, the listing line shape
│   ├── metric-option.md      # C-4, the option and its status-2 path
│   └── table-selection.md    # C-6, the stepping in the mapping key
├── checklists/requirements.md
└── tasks.md                  # Phase 2 (written by /speckit.tasks)
```

### Source (this feature)

```text
include/speedgun-ng/
└── counters_metrics.hpp                 # NEW: metric-by-name entry point,
                                         #      metric view, refusal kinds

source/counters/
├── detail/pmu.hpp                       # PmuIdent gains the stepping field;
│                                        # PmuTableEntry gains the metric fields
├── linux_pmu/
│   ├── metric_formula.hpp               # NEW: token grammar and parser, detail
│   ├── metric_formula.cpp               # NEW: parse and resolve, detail
│   ├── metric_tables.hpp                # NEW: ScaleUnit table and literal
│   │                                    #      constant fact table
│   ├── table_parse.cpp                  # addEntry accepts metric rows;
│   │                                    # mapfileKey and pmuSelectDirectory
│   │                                    # carry the stepping
│   └── embedded_tables.hpp              # registry unchanged in shape

source/harness/
├── catalog.cpp                          # printObject emits metric lines
├── cli.cpp                              # the metric option row and the
│                                        # fail-fast status-2 path
└── runner.cpp                           # metric columns from named metrics

example/docs/
└── counter_metrics.cpp                  # second region the page quotes

test/
├── source/counters_metric_formula_test.cpp   # NEW: grammar and refusals
├── source/counters_metric_lookup_test.cpp    # NEW: by-name, scale, cycle
├── source/counters_pmu_test.cpp              # stepping fixtures, both sides
├── source/harness_catalog_test.cpp           # metric lines
├── source/harness_cli_test.cpp               # option and status 2
├── source/detail/metric_fixture.hpp          # NEW: shared fixture tables
├── cli_shape.sh                              # option row parity
└── CMakeLists.txt                            # registration

README.md / docs/pages/harness.md        # page section and the harness page
tools/docs/example_drift.py              # PAGE_LINE_CAP raise, section owner
```

**Structure decision**: the grammar lives behind one internal translation
unit inside the counters library, because the only caller is the table
loader and Principle X forbids a seam without a second caller. The public
surface is one new header of additive declarations: a name lookup that
yields the folded expression, its group text and its unit text. Inside
`include/speedgun-ng/counters_measurement.hpp` the only change is two
appended kind codes on `detail::ExprCore`, so no existing declaration
collides with anything, and the archive
`SOVERSION` question has one answer and it is 2.

## Design

### Decision index

| Decision | Question | Answer |
| --- | --- | --- |
| D-01 | What grammar does the parser accept? | The closed list FR-003 to FR-006 spell out, refuse the rest |
| D-02 | Where does parsing run? | Once per directory at load, inside the loader |
| D-03 | How does a metric reference resolve? | Two passes: parse to a placeholder leaf, resolve by name with a coloured DFS |
| D-04 | What happens on a reference cycle? | Every member is refused at load and names the cycle |
| D-05 | How does `ScaleUnit` become a factor? | One pinned conversion table, swept against the tree |
| D-06 | Where do `#` literals get values? | From host facts the library already publishes, never guessed |
| D-07 | How do `if`/`else`, `min`, `max`, `d_ratio`, `has_event` and `source_count` fold? | Conditionals and the two helpers resolve at load; `min` and `max` become nodes; `d_ratio` lowers to a guarded quotient |
| D-08 | How do `@modifier@` and escapes tokenise? | One token kind with the escape decoded, privilege suffix kept |
| D-09 | Where does a compiled metric plug in? | `ExprCore` leaves spliced into the existing spine, two appended kind codes, `compileCore` |
| D-10 | What does the public entry point look like? | One header, name in, view out, recoverable refusal |
| D-11 | What does the catalog line carry? | Name, group, unit text, then availability or refusal reason |
| D-12 | How does the command line name a metric? | A repeatable option, order-preserving, fail-fast at status 2 |
| D-13 | How does the stepping reach the key? | A private field on `PmuIdent`, appended to the key |
| D-14 | What is the load and coverage budget? | Parse cost table, corpus plus sweep for every branch |

### Risks

- The grammar is a moving target: a re-pin of `external/pmu-events` can
  introduce a construct the closed list rejects. The sweep test of FR-038
  turns that into a failing test with the row printed, which is the
  designed behaviour and the reason the refusal names the construct.
- The literal constants depend on host facts. A fact that is unavailable
  on a machine refuses the metric, so a host can legitimately list fewer
  metrics than the table holds. D-06 keeps the refusal text precise.
- The stepping fix changes selection for model-55 parts. Any machine that
  today selects nothing starts selecting a directory, which changes which
  leaves it exposes. D-13 states the verification for both sides.
- `README.md` stands at the line cap, so the documentation move is a
  budget edit, and the quick-reference table is shared with the other two
  lanes. D-14 states the recorded raise and the exact section size.

### Logical view: the load pipeline

```text
embedded table image (JSON text, one directory)
   |
   +-- row split -------------------------------+
   |                                            |
 counter row                              metric row
   |                                            |
 addEntry (existing)                     addEntry (extended)
   |                                       |
   |                                 parse formula  ......... D-01, D-02
   |                                       |
   |                                 unresolved reference leaves
   |                                       |
   |                                 resolve pass, coloured DFS ... D-03, D-04
   |                                       |
   |                                 fold literals, conditionals, min/max, helpers  D-06, D-07
   |                                       |
   |                                 apply ScaleUnit factor ..... D-05
   |                                       |
   +----------------+----------------------+
                    |
        per-directory table image (parsed, cached)   ......... D-09
                    |
        name index for lookup and catalog
```

A row that cannot be parsed never reaches the image. It is recorded as a
refusal with a kind and a one-line reason, and the catalog prints it, so
the image plus the refusals account for every row of the directory.

### Logical view: asking for a metric

```text
caller: metricByName(name)
   |
   +-- host selects no directory -> refusal: this host carries no metric table
   +-- name absent              -> refusal with near-miss names, one line
   +-- name refused at load     -> refusal with the load reason and its kind
   +-- name resolves
          |
          view { expression, group text, unit text }
          |
          fold over a recorder, existing path -> MetricResult
```

The expression in the view is built from the same leaves a hand-composed
metric uses, so `foldPairsCore` and `compileCore` see a spine of the shape
they already handle. Availability is decided at load and at plan time: an
event the host cannot encode gives `NOT_ENCODABLE`, an event the host does
not have gives `ABSENT`, and a metric inherits the refusal kind of the
constituent that failed. Nothing in this path reads a counter and nothing
parses a string after load.

### Logical view: selection with the stepping

```text
CPUID vendor + family + model + stepping
   |
 mapfileKey -> "<vendor>-<family>-<model>-<stepping>"
   |
 mapfile rows, first column is a pattern
   |
 full match against the key -> directory name + revision + counter kind
```

Two fixtures, one with a stepping below the model-55 boundary and one at or
above it, select `skylakex` and `cascadelakex`. The sweep of FR-036 walks
every pattern row of the pinned mapping file, so a re-pin that adds a
stepping class fails a test in front of a reader, where silence selected
nothing before.

### Physical view

The counters archive keeps its boundary: the parser and the tables are
internal, reached through `detail` and the new public header, so
`counters_header_purity.sh` sees no new dependency and the symbol scan
gains only the new exported entry point. The harness archive stays the only
place that reads `argv`; it reaches the library through the public header.
`runner.cpp` builds metric columns after the attached metrics and before the
counter addresses, which is the column order the report prints. The example
under `example/docs/` compiles against the public header alone, so the block
a reader copies needs nothing the page leaves unnamed.

### Public API surface added

One header, `include/speedgun-ng/counters_metrics.hpp`, declaring:

- a lookup that takes a name and returns either a metric view or a
  recoverable refusal carrying a `MetricRefusal` kind from the closed set in
  `data-model.md`, on the pattern the object lookup already follows;
- a metric view carrying the expression, the group text and the unit text;
- the load-time refusal kinds the catalog prints, a new closed set inside
  the feature, while `Availability` keeps its six members so no new value
  reaches a report column.
- two appended kind codes on `detail::ExprCore`, 5 for the minimum node and 6
  for the maximum node, listed beside the codes the header already documents.

No existing declaration changes shape and no existing enum value changes.
`MetricResult` keeps its four members, so
every caller that reads a metric today keeps compiling unchanged, and the
unit text arrives beside the result.

### Version and `SOVERSION`

Both stay. The counters archive holds 2 and the harness archive holds 6,
because the change is additive at the public boundary and `MetricResult`,
`Availability` and every existing signature keep their shape. The two
appended kind codes are the one change inside a public header: an expression
built by an existing caller cannot carry them, so an old object file folds
exactly as before, and the additive public header of 020 sets the house
precedent for keeping a `SOVERSION` through an additive change. The rule
stated beside the archive `SOVERSION` in `CMakeLists.txt` is the authority,
and the check is the ABI-facing gate already in the matrix: a shape change
would show in the header-purity and nm proof gates before a human had to
notice. `VERSION` stays 0.10.0 for this feature on its own.

## Test Plan

**Execution mode**: TDD (Principle III). Each new test file is
written first and observed failing for the missing behaviour, then made
green. The observation line goes into the task that carries it. Value tests
use the fake provider (FR-039), no test sleeps or reads a clock (FR-041) and
new code holds 100% line and branch coverage (FR-040).

**Scenario and check mapping**:

| Spec surface | Test | Check |
| --- | --- | --- |
| Grammar of every accepted construct | `test/source/counters_metric_formula_test.cpp` | One case per construct in D-01, driven by rows quoted from the pinned tree |
| Every refusal branch | same file | One malformed input per branch: unbalanced parenthesis, trailing operator, empty text, doubled operator, unknown construct, unknown literal, missing `else` |
| Sweep of the pinned tree | `test/source/counters_metric_formula_test.cpp` | Every expression row of every directory parses or refuses with a reason; the account equals the row count (FR-038) |
| `min`, `max`, `d_ratio` lowering | `test/source/counters_metric_formula_test.cpp` | Both runtime arguments, one runtime argument, and the `d_ratio` zero denominator through `max(y, 1.0)` (FR-004) |
| Duplicate name and dimension refusal | `test/source/counters_metric_lookup_test.cpp` | The first row publishes and the second refuses as a duplicate (FR-011); terms that cannot compose refuse with the operands named (FR-010) |
| Metric rows stay out of the event path | `test/source/counters_pmu_test.cpp` | A metric name never encodes as an event and the existing refusal stays in force (FR-013) |
| Metric by name, scale, group, unit | `test/source/counters_metric_lookup_test.cpp` | `cpi`, a `100%` row, `tma_info_thread_ipc` through its inner metric, near-miss refusal text |
| Cycle refusal | same file | A two-metric cycle built in a fixture table refuses both and names both |
| Conditional and `has_event` | same file | A fake host above and below its core count selects the two branches |
| Selection with stepping | `test/source/counters_pmu_test.cpp` | Both sides of the model-55 boundary select the directory their row names; every stepping pattern of the pinned file matches a fixture key |
| Catalog lines | `test/source/harness_catalog_test.cpp` | Counter lines keep their shape, one metric line per row with group, unit text, availability or reason |
| Option, order, dedup, status 2 | `test/source/harness_cli_test.cpp` | One column per named metric in command-line order, a repeated name gives one column, an unknown name prints one line and exits 2 before measurement |
| Column values | `test/source/harness_catalog_test.cpp` plus the fake provider | The metric column value matches the same metric composed by hand over identical fake counts |
| Page block | `example_run`, `snippet_compile`, `example-drift-fixture` | The region matches the page, the block builds, the run prints the row the page names |
| Option parity | `test/cli_shape.sh` | The option table in `README.md` carries the new row |

**Precondition violations** follow the established pattern: a parser entry
point given an empty or invalid range trips the contract check the
`dbc-gate-fixture` builds catch, and the recoverable refusals are returned
to callers as values. No exception is raised in this path.

**Existing tests the changes touch**: `counters_pmu_test.cpp` for the key
shape, `harness_catalog_test.cpp` for the listing shape, `harness_cli_test.cpp`
for the option table, `cli_shape.sh` for the page parity, and any fixture
that counts catalog lines after metric lines join the listing.

**Registration pattern**: each new test file follows the existing block in
`test/CMakeLists.txt` that declares a `counters_*_test` or
`harness_*_test` executable and adds it with `add_test`, so the CTest name
matches the file name.

**Gates that must stay green**: `speedgun-ng_test`, `dbc_test`,
`counters_header_purity.sh`, `counters_pmu_test`, `cli_shape.sh`,
`loop_shape.sh`, `time_source_gate.sh`, `example_run`,
`example_run_fixtures`, `snippet_compile`, `report_shape`, the pmu-events
gate fixture, the example-drift fixture and the prose gate.

**Determinism**: parsed order is table order, and every assertion that
depends on order compares against the order of the pinned file, which
stands still between re-pins. No test reads a clock. No test sleeps. The
fake provider carries every count a test asserts.

**Deliberately untouched**: multi-pass collection (N6), the top-down metric
tree (N10), percentiles (N1), topology and NUMA placement (N4, N12), the
execution graph (N11), and any event list outside the selected directory.

## Constitution Check

Re-checked after Phase 1. Every gate above holds against the design: the
public surface grows additively, so I, V and VIII are unchanged in kind.
Coverage is designed in and carried by fixtures, which is VI. Documentation
carries the run obligation of IV.1 through the existing example owner.
Principle II is served by refusals at every contract edge, and III by the
test-first order recorded in `tasks.md`. No violation needs an entry in
Complexity Tracking.

## Complexity Tracking

No gate is weakened and no violation needs justification, so this section
stays empty by design. The two pieces a reviewer may read as extra carry
their reason in the spec: the conversion table exists because 3222 rows
carry a `ScaleUnit` and an unconverted factor prints a wrong number, and the
stepping fix exists because the development host selects no table at all
without it.
