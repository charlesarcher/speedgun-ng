# Implementation Plan: Hunter mode core (event ranking)

**Branch**: `029-hunter-core` | **Spec**: [spec.md](spec.md) |
**Audit point**: `bd10d76` (master, 2026-10-21) | **Date**: 2026-10-22 |
**Constitution**: v2.20.0 (ratified 2026-09-06, amended 2026-10-14)

## Summary

A run named by `--hunt` takes its event list from the `sg::counters` catalog,
plans one column per event through the specs/024 planner, and ranks the events
by the rate each one reaches against the cycles and instructions anchors of its
own pass. The column shape is fixed: one event plus the two anchors of the same
device, so a device that publishes a programmable-counter limit `L` carries
`L - 2` hunt events per pass, and a device that publishes no limit settles
`MULTIPLEX` through the planner path master already has. The median and the
coefficient of variation come from the one statistics helper master already
computes, moved out of the anonymous namespace of `runner.cpp` into a named
internal header, so a hunt CV equals the printed `cv=` column by construction.
Ranking is pure post-processing over the collected samples: no clock, no
counter read, no new pass loop, no second counter path.

Two facts of the audit change existing code directly. The
per-pass seat of the N6 pass loop collapses a group-open refusal into an
`SG_INVARIANT`, which under the default contract semantic aborts the process
(spec PC-9), so the seat returns its error to the loop and a hunt run retries
the failed pass one event at a time inside that same loop. The scripted seam
cannot express a group-open refusal at all today (`FakeProvider` is `final` and
`open` refuses only an undeclared object or counter, which the spine compile
rejects before `open` runs, so no harness-level test can reach it), so the
scripted refusal joins the exported `FakeProvider` (spec PC-3). Both facts move
an ABI: `hunt.hpp` adds public calls and the run record gains the hunt data
(harness `SOVERSION` 10 to 11), and the scripted seam joins an exported class
(archive `SOVERSION` 3 to 4).

A run without `--hunt` is untouched: same console bytes, same JSON bytes apart
from the `schemaVersion` value, same exit status, same invariant route for a
seat that fails after pass 0.

## Technical Context

**Language/Version**: C++23, ISO-strict (`CMAKE_CXX_EXTENSIONS OFF`), the GCC
and Clang release toolchains of the two Linux presets (`ci-rocky`, `ci-ubuntu`)
**Primary Dependencies**: the `sg::counters` catalog and the specs/024 planner
(`System::object`, `Object::counters`, `Object::programmableCounters`,
`planPasses`, `mergePasses`), the specs/023 JSON writer and reader with the
vendored `simdjson` (private link, `CMakeLists.txt:216-222`), the vendored
`external/pmu-events` tables embedded by `cmake/EmbedPmuEvents.cmake`. No new
dependency (D-4, hard rule 15)
**Storage**: no new store. Two edited files: `docs/schema/report-v1.json` and
`tools/docs/example_drift.py`. One new recorded golden:
`example/docs/hunt_rank.expected`
**Testing**: assertion-helper test translation units registered one executable
per file with `add_executable` plus `add_test`
(`test/CMakeLists.txt:1079-1083` is the block pattern; the golden-path
hand-off at `test/CMakeLists.txt:1089-1091` is the pattern for a recorded file,
and `SG_SCHEMA_PATH` is the pattern for a compiled-in source path)
**Target Platform**: Linux x86-64, Ubuntu 26.04 and Rocky Linux 10 container
runners. Recorded hardware runs: `corsairone` (Intel, one socket) and `gary`
(Zen 5, AC-10, when the N7 lane has a gap)
**Project Type**: library (two shared objects: `speedgun-ng` and `harness`) +
command line + documentation site + example suite
**Performance Goals**: no clock read and no counter read in discovery, planning,
normalization or ranking; the calibration estimate reaches the planner as data;
a run with no `--hunt` keeps its present read count and its present bytes
(SC-06, SC-07; FR-012, FR-052; Principle VII)
**Constraints**: 100 percent line and 100 percent branch coverage with no
exclusion marker and no waiver (FR-091); no sleep, no clock and no fixed CPU
count in any test, CPU sets from `sched_getaffinity` (FR-092); one line naming
the value plus exit 2 for every invalid option (FR-081); option values 270 to
279 as named constants with no short letters (FR-080); schema minor 1.4;
harness `SOVERSION` 11, archive `SOVERSION` 4 (FR-090)
**Scale/Scope**: the core catalog of the development host, 279 entries with 151
refusals recorded in the specs/021 hardware record (spec Assumptions);
`--hunt-top` accepts 1 to 10000 and `--hunt-repeats` 1 to 1000;
`kMaxUserCounters` 16 (`runner.cpp:866,873`) bounds named `--counter` columns
alone and bounds no hunt column

## Constitution Check

**Status**: PASS

- **I. Standard-First** | PASS | `std::expected` for the recoverable refusals,
  `std::optional` for the budget, the estimate and the rank, `std::string_view`
  for catalog addresses, one closed `enum class` per hunt choice; no new
  third-party dependency and no compiler extension
- **II. Design By Contract** | PASS | every name in `hunt.hpp` carries `\pre`,
  `\post` and, where it owns state, `\invariant` in the house form
  (`pass_plan.hpp:63-107` is the model); `planHunt`, `rankEvents` and the
  budget split are pure functions with stated pre- and post-conditions;
  `SG_REQUIRE`, `SG_ENSURE` and `SG_ASSERT` pair one-for-one with the doc lines
  under `dbc-gate`, and every contract predicate names a function instead of a
  lambda (FR-093, hard rules 5 and 6)
- **III. R-DCUT** | PASS | logical and physical views below; the test plan
  below; the decision units are separate translation units reachable from their
  own tests; the threaded hunt test subscribes to window-open counts on the
  scripted provider; the case waits on nothing
- **IV. Documentation** | PASS | the "Hunter mode" section of the main page
  comes from the marked regions of `example/docs/hunt_rank.cpp`, which
  `add_docs_example` builds and `example_run` runs; `snippet_compile` compiles
  the copied block over the canonical includes; `report_shape` compares the
  recorded run of the example with `hunt_rank.expected` by shape;
  `docs/pages/harness.md` carries the option detail; every hunt option gains
  its quick-reference row, which `example_drift --options` requires
- **V. Style** | PASS | `sg::hunt` namespace, camelBack functions, PascalCase
  types, `k`-prefixed constants, `m_`-prefixed members, one public data member
  per N-12 rule, `lower_case` file names, `.clang-format` clean
- **VI. Coverage** | PASS | every decision is a pure function of a scripted or
  literal input (the limit, the availability, the refusal, the sample list), so
  the scripted provider reaches 100 percent line and 100 percent branch on
  every new file; `lcov --filter brace` stays as master configures it; no
  `LCOV_EXCL` marker, no waiver and no residual row anywhere in the tree
  (FR-091; a gap stops the work with `gcov -b` evidence attached)
- **VII. Performance** | PASS | the hunt plan completes before the first window
  opens; the hunt columns are ordinary spine columns inside the pass loop, so
  no read enters a timed window that the same run would not read today;
  `rankEvents` reads published values; the estimate is arithmetic over the
  calibration data
- **VIII. CI Gates** | PASS | `format-check`, `spell-check`, `prose-lint`,
  `dbc-gate`, `dbc_dependency_scan`, `time_source_gate`, `cli_shape`,
  `loop_shape`, `barrier_shape`, `example_drift`, `example_drift_fixtures`,
  `snippet_compile`, `report_shape`, `report_shape_fixtures`, `example_run`,
  `example_run_fixtures`, clang-tidy and cppcheck from `ci-ubuntu`, the
  `ci-sanitize` sanitizers, the `ci-tsan` thread sanitizer, and the `coverage`
  target at 100 percent line, branch and DBC all pass with the change; the
  `ci-rocky` and `ci-ubuntu` preset builds both run once per feature
  (Principle IX)
- **IX. Spec-Driven** | PASS | this plan is the Phase 0 and Phase 1 output of
  an approved spec; `research.md`, `data-model.md`, `contracts/` and
  `quickstart.md` accompany it; `tasks.md` follows from it
- **X. Anti-Slop** | PASS | no wrapper type over a published
  `std::optional<std::size_t>`, no policy tag, no injection seam beyond the one
  scripted refusal D-09 adds because D-18 cannot be tested without it, no
  extension point for N14a-2, N14b or N14c (D-5 fixes the scope); the seat
  branch of D-11 replaces an `SG_INVARIANT` whose stated assumption the
  group-open refusal breaks, so it handles a runtime condition and not a
  violated contract; Complexity Tracking stays empty
- **XI. Discourse and Prose** | PASS | every artifact passes
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  specs/029-hunter-core/`; requirement text uses *shall*; every threshold
  carries its unit (`10` percent CV, `120` seconds, `8` passes, `20` rows)

**Additional Constraints**:

- **Counters-only time and counters** | PASS | every event value and every
  anchor value of a hunt comes from `sg::counters` through the existing spine
  compile and pass loop; `hunt.cpp` and `hunt_cli.cpp` include no time header and
  call no counter syscall, so `test/time_source_gate.sh` reports no hit; the
  planner reads `Object::programmableCounters()` and never sysfs or CPUID
  (D-23, hard rule 15)
- **Library-first** | PASS | the public surface is one header,
  `include/speedgun-ng/hunt.hpp`, exported through the generated export macro;
  the planner, the run driver, the ranker and the printers live under
  `source/`; `install(DIRECTORY include/ ...)` in
  `cmake/install-rules.cmake:16-17` carries the new header with no new rule
- **simdjson-only JSON** | PASS | the two hunt objects go through the existing
  `Builder` and the RAII `JsonObjectWriter` of `json_report.cpp`; the reader
  uses the `.get(out)` error-code form through `field` and `optionalField`
  (`json_load.cpp:83,103`) with one test per refused code (FR-051, hard rule 7)
- **Dependencies** | PASS | zero new runtime dependencies
- **Build system** | PASS | preset-driven configuration only; no
  `CMakeUserPresets.json` in the tree; the harness source glob
  (`CMakeLists.txt:196-200`) picks up the two new translation units with no
  source-list edit
- **Licensing** | PASS | BSD 3-Clause throughout; no new event table, no table
  generator and no licence change (D-4, FR-003)

## Project Structure

### Documentation

```text
specs/029-hunter-core/
├── spec.md                        # approved; Decisions D-1 to D-23, PCs PC-1 to PC-9
├── plan.md                        # this document
├── research.md                    # D-01 to D-20, with alternatives
├── data-model.md                  # E-01 to E-10, invariants I-1 to I-8, state tables
├── quickstart.md                  # build, test and recorded-run guide
├── contracts/
│   ├── hunt-api.md                # the public hunt.hpp surface
│   ├── hunt-cli.md                # the eight options, the refusals, the exit statuses
│   ├── hunt-json.md               # the two hunt objects and their round trip
│   └── report-schema-key.md       # the schema keys, x-order and the 1.4 reservation
├── results/                       # AC-10 hardware records (implement phase)
└── tasks.md                       # /speckit.tasks output
```

### Source

Four new source files, one new public header, one new internal header, six new
test translation units, one new docs example. No directory is restructured and
no existing file is moved.

```text
include/speedgun-ng/
├── hunt.hpp                       # NEW  E-01..E-08, sg::hunt public surface
└── benchmark.hpp                  # EDIT BenchmarkResult gains `hunt`,
                                   #      ReportData gains `hunt`, includes hunt.hpp

source/harness/
├── detail/statistics.hpp          # NEW  statisticsOf, named, out of runner.cpp
├── statistics.cpp                 # NEW  its definition (was runner.cpp:49-82)
├── detail/hunt_plan.hpp           # NEW  planHunt, splitStages, rankEvents, budget parse core
├── hunt_plan.cpp                  # NEW  their definitions
├── hunt.cpp                       # NEW  EventCatalog discovery, runHunt driver
├── hunt_cli.cpp                   # NEW  the eight options, value ranges, refusals,
                                   #      raw-code refusal, list file, conflicts
├── detail/internal.hpp            # EDIT RunOptions gains the hunt selection (E-09)
├── runner.cpp                     # EDIT statisticsOf call sites (2569, 2601, 2634);
                                   #      seatPass error into the pass loop (2286-2292);
                                   #      hunt column build and disclosure
├── cli.cpp                        # EDIT eight table rows and eight named constants only
├── report.cpp                     # EDIT printHuntPlan, printHuntRanking, printHuntSkips
├── json_report.cpp                # EDIT kSchemaVersion, writeHunt, hunt in writeEntry
└── json_load.cpp                  # EDIT readHunt, hunt in readEntry through optionalField

source/counters/
└── fake_provider.cpp              # EDIT scripted group-leaf refusal in open()

include/speedgun-ng/
└── counters_fake.hpp              # EDIT FakeProvider::setGroupLeafLimit (exported)

example/docs/
├── hunt_rank.cpp                  # NEW  owner of the README section
└── hunt_rank.expected             # NEW  host-independent recorded run

test/source/
├── harness_hunt_plan_test.cpp     # NEW  column shape, limit rows, budget split, seed order
├── harness_hunt_rank_test.cpp     # NEW  normalization, CV, noise, rank keys
├── harness_hunt_collect_test.cpp  # NEW  scripted runs: skips, no-limit, retry, threads
├── harness_hunt_cli_test.cpp      # NEW  every exit-2 refusal, conflicts, list file, raw code
├── harness_hunt_report_test.cpp   # NEW  console order, plan print, no-hunt byte identity
├── harness_hunt_json_test.cpp     # NEW  round trip, optional keys, refused codes
└── harness_cli_table_test.cpp     # NEW  name and value uniqueness (created here, D-21)

test/source/detail/
└── hunt_fixture.hpp               # NEW  scripted host: core objects, 200 events, limits

tools/docs/
└── example_drift.py               # EDIT SECTIONS, SECTION_OWNERS, PAGE_LINE_CAP
```

**Structure decision**: the library keeps measurement and publishes facts; the
hunt owns selection, planning policy and ranking, and it holds no counter path
of its own. Discovery reads the catalog through
`System::object(path)` and `Object::counters()` and composes each address as
`Object::path() + "/" + entry.name` (`CatalogEntry` carries no address field,
spec PC-1, PC-8); the catalog walk follows the same children walk
`catalog.cpp:81-88` already performs. Planning emits `ColumnLeaves` for the
existing `planPasses` (D-23), which is width-agnostic and holds no column-count
constant (spec PC-2). The statistics helper moves to the shared header and one
stand-alone definition serves both callers, which
is what keeps a hunt CV equal to the printed `cv=` column (PC-4). The two new
`source/harness/*.cpp` files need no CMake source-list edit, because the harness
target globs with `CONFIGURE_DEPENDS` (`CMakeLists.txt:196-200`); the counters
library globs the same way for its own directory (`CMakeLists.txt:886-891`).

## Design

### Decision index

Full rationale and alternatives stand in [research.md](research.md). A bare
`D-n` names a spec decision in [spec.md](spec.md); a zero-padded `D-0n` names a
plan decision below and in [research.md](research.md).

| # | Subject | Decision |
|---|---------|----------|
| D-01 | Shape of the feature | hunt is a harness column family driven through `Runner` and `planPasses`; no standalone counter driver, no reuse of the named-counter slot |
| D-02 | Core device rule | exact device parts `cpu`, `cpu_core`, `cpu_atom`, plus the `armv8_` prefix rule written hunt-side; uncore enters only by list or glob |
| D-03 | Discovery | walk `object("machine")` and its children, compose `path + "/" + entry.name`, keep catalog order, deduplicate by address |
| D-04 | Anchors | `cpu-cycles` and `instructions` resolved on the event's own device, address-composed, no invented alias; a device without both anchors normalizes by `iter` |
| D-05 | Statistics reuse | `statisticsOf` moves to `source/harness/detail/statistics.hpp` plus `statistics.cpp`; `runner.cpp` calls it; `passSpread` and the counters median stay untouched |
| D-06 | Seed order | one in-repo 64-bit linear congruential Fisher-Yates shuffle over the deduplicated catalog order, so every CI compiler agrees on the permutation |
| D-07 | Rank inputs | `rankEvents` is pure over the per-event sample list; median and CV come from `statisticsOf`; ties break by address |
| D-08 | Normalization | rate per 1000 anchor units of the same pass (`instr` default, `cycle`), or the raw per-iteration median (`iter`); per pass, before the median |
| D-09 | Scripted open refusal | `FakeProvider::setGroupLeafLimit(objectPath, leaves)`: `open` refuses a leaf set wider than the limit for that object, so a wide group fails where one event with the two anchors opens |
| D-10 | Hunt data location | public `hunt::RunPlan` on `ReportData` and public `hunt::EntryHunt` on `BenchmarkResult`, because `loadReport` must read both back |
| D-11 | Seat error route | `seatPass` returns its error into the pass loop; a hunt run appends one-event passes and continues, a run with no hunt columns keeps the `SG_INVARIANT` fuse |
| D-12 | Retry inside the loop | a failed pass appends its single-event passes to the pass list the loop is reading, so one loop runs the whole run |
| D-13 | Two-stage run | one `Runner::run` per stage over the same instance, stage 1 at one repeat, stage 2 at `--hunt-repeats`; the row set the report prints comes from the last stage |
| D-14 | Single-sample events | an event with one sample reports `cv` 0.0 and cannot enter the top N; stage-1 selection ranks those samples by rate |
| D-15 | Console order | plan before the run; then the ranked table, the marked excluded tail, the skipped list and the plan summary after the benchmark tables |
| D-16 | Refusal point | option values are parsed at the scan; list, glob and `@file` expansion runs after the scan, where the catalog answers, and exits 2 naming the value |
| D-17 | Option wiring | eight named constants 270 to 279 and eight rows in the one `option longOptions[]` table of `cli.cpp`; every parse and refusal body in `hunt_cli.cpp` |
| D-18 | Table test | `harness_cli_table_test.cpp` reads `cli.cpp` through a compiled-in path and asserts unique names and unique values |
| D-19 | Docs ownership | `hunt_rank.cpp` owns the "Hunter mode" section with a scripted host that refuses its events, so the skipped list prints on every host |
| D-20 | Version and schema | `0.19.0` planned, next free minor at rebase; harness `SOVERSION` 11, archive `SOVERSION` 4; schema minor 1.4 |

### Risks

- **The seat change reaches every run.** `seatPass` at `runner.cpp:1400` and its
  use at `runner.cpp:2286-2292` serve the non-hunt path too. Mitigation: the
  hunt branch is taken only when the run carries hunt columns, the
  `SG_INVARIANT` route stays byte-for-byte for every other run, and
  `harness_pass_report_test.cpp` keeps its captured byte-identity golden as a
  covering check for the untouched route.
- **Shared files with the two live lanes.** `cli.cpp`, `runner.cpp`,
  `report.cpp`, `benchmark.hpp`, `internal.hpp`, `README.md`,
  `example_drift.py`, `docs/pages/harness.md` and `docs/schema/report-v1.json`
  are all shared with specs/027 (N13a) and specs/028 (N7). Mitigation: option
  parsing lives in `hunt_cli.cpp` and `cli.cpp` gains table rows only (D-17,
  spec D-22); the schema keys and the README rows are appended, never
  reordered; the rebase rules below fix the version, the schema minor and the
  page cap.
- **The hunt width against the group limit.** A pass carries `2 + (L - 2)`
  leaves, and a device with no published limit plans `MULTIPLEX` (spec PC-2).
  Mitigation: `harness_hunt_plan_test.cpp` drives the limit rows `1`, `3`, `4`,
  `6`, `8`, no-limit, and the scripted group refusal proves the retry path
  without hardware.
- **Arm hosts plan as `MULTIPLEX`.** `limitForDevice` answers for `cpu`,
  `cpu_core` and `cpu_atom` only, so an `armv8_` device publishes no limit and
  the `L - 2` shape of AC-1 is reachable only through the scripted limit
  (spec PC-2 note). The plan states this in the disclosure instead of
  pretending a limit.
- **A noisy or unrepeated event crowding the ranked table.** Mitigation: the
  top N holds only events with at least two samples and a CV at or under
  `--hunt-noise`; everything else prints in the marked tail with `rank=-`
  (D-14, D-15), so the table size stays `--hunt-top`.
- **gcov phantom branches.** Every new decision unit is a pure function with
  one entry and one exit; the shape rules specs/022 records apply under
  `lcov --filter brace`; no new exclusion marker, checked by grep in the
  coverage phase (FR-091).

### Logical view

The hunt decisions of one run (E-numbers refer to
[data-model.md](data-model.md)):

```text
option values (E-09, hunt_cli.cpp)
    |  parse at the scan: norm, rank, top, noise, repeats, seed, budget
    |  expand after the scan: all | list | glob | @file   (exit 2 names the value)
    v
EventCatalog (E-01)  ---- machine children walk, address = path + "/" + name
    |                    dedup by address, catalog order, seed shuffle (D-06)
    |                    COUNTABLE at discovery -> columns; other kinds -> skipped
    v
planHunt (E-04)      one column per event: { event, cycles, instructions } of
    |                the event's own device -> ColumnLeaves -> planPasses
    |                limit L -> at most L - 2 events per pass
    |                no limit -> empty plan -> settleMethod settles MULTIPLEX
    |                budget -> splitStages: stage 1 all events, stage 2 top 3N
    v
print plan (E-07)    events, passes, method, estimate = nsPerPass x passes x repeats
    |                (estimate is calibration data; no clock read)
    v
runHunt (E-05)       Runner::run over the plan through the N6 pass loop
    |                  seat of a pass fails -> return the error -> append the
    |                  one-event passes of that pass -> a lone event that still
    |                  fails joins the skipped list with its kind
    |                  a window that discloses GAP -> skipped list
    v
rankEvents (E-06)    per pass: value / anchor of the same pass x 1000 (or raw)
    |                median + CV from statisticsOf; noisy above --hunt-noise
    |                top N over events with >= 2 samples and a clean CV
    v
console (E-07) + JSON (E-08)  ranked table, marked tail, skipped list, plan
                              summary;  top-level hunt object and per-entry hunt
                              object through the 023 writer and reader
```

### Physical view

```text
cli.cpp  --(8 table rows, 8 named constants)--> hunt_cli.cpp  (parse, refuse,
                                                   expand, conflict check)
                                                       | RunOptions hunt fields
                                                       v
   counters_system.hpp Object::counters()/path()  -->  hunt.cpp (EventCatalog)
     counters_system.hpp Object::programmableCounters()
                                                       |  ColumnLeaves
                                                       v
                              detail/pass_plan.hpp planPasses  (unchanged)
                                                       |  PassPlan
                                                       v
                    runner.cpp pass loop  <-- seat error route (D-11)
                    detail/statistics.hpp statisticsOf  (shared with 3 sites)
                                                       |
                                                       v
                    hunt_plan.cpp rankEvents  ->  BenchmarkResult::hunt
                                                       |
                        +------------------------------+------------------------------+
                        v                              v                              v
              report.cpp hunt printers      json_report.cpp hunt objects      json_load.cpp hunt readers
                        |                              |                              |
                        v                              v                              v
                    console                      report-v1.json schema 1.4        loadReport
```

The counters archive publishes the facts the hunt reads; the harness archive
owns selection, planning and ranking. `hunt.hpp` forward-declares
`enum class MetricMethod : std::uint8_t;` in `namespace sg`, the same shape
`pass_plan.hpp:27` already carries, so `hunt.hpp` needs no include of
`benchmark.hpp` and `benchmark.hpp` includes `hunt.hpp` in one direction.

### Public API surface added

| Name | Header | Role |
|---|---|---|
| `struct HuntBudget` | `speedgun-ng/hunt.hpp` | E-03: `kind` (`SECONDS` or `PASSES`) and `amount` |
| `struct HuntOptions` | `speedgun-ng/hunt.hpp` | E-03: norm, rank, top, noise, repeats, seed, budget |
| `enum class HuntNorm`, `HuntRank` | `speedgun-ng/hunt.hpp` | E-02: `INSTR`, `CYCLE`, `ITER` and `RATE`, `CV` |
| `class EventCatalog` | `speedgun-ng/hunt.hpp` | E-01: selection, expansion, dedup, availability split |
| `auto planHunt(...) -> std::expected<HuntPlan, Error>` | `speedgun-ng/hunt.hpp` | E-04: columns, passes, method, stages, estimate |
| `auto runHunt(...) -> std::expected<EntryHunt, Error>` | `speedgun-ng/hunt.hpp` | E-05: runs the plan through the pass loop |
| `auto rankEvents(...) -> std::vector<RankedEvent>` | `speedgun-ng/hunt.hpp` | E-06: pure normalization, median, CV, rank |
| `struct HuntPlan`, `RankedEvent`, `SkippedEvent`, `EntryHunt` | `speedgun-ng/hunt.hpp` | E-04 to E-06: the published records |
| `BenchmarkResult::hunt` | `speedgun-ng/benchmark.hpp` | E-06: the per-entry hunt record |
| `ReportData::hunt` | `speedgun-ng/benchmark.hpp` | E-08: the run-level hunt record |
| `auto FakeProvider::setGroupLeafLimit(std::string_view, std::size_t) -> FakeProvider&` | `speedgun-ng/counters_fake.hpp` | E-10: the scripted group refusal |
| `--hunt`, `--hunt-norm`, `--hunt-rank`, `--hunt-top`, `--hunt-noise`, `--hunt-repeats`, `--hunt-budget`, `--hunt-seed` | CLI | E-09: the caller's selection, values 270 to 279 |

`statisticsOf`, `planHunt` internals, the printers and the budget split core
stay internal: no `detail/` type reaches a public signature (spec D-15), and
`source/harness/detail/statistics.hpp` plus
`source/harness/detail/hunt_plan.hpp` stay inside the harness archive.

### Version, SOVERSIONs and schema

`CMakeLists.txt:9` carries `VERSION 0.16.0`; the planned move is `0.19.0` past
the `0.17.0` specs/027 and `0.18.0` specs/028 reserve. The number is a
reservation: at rebase, read `CMakeLists.txt` on master and take the next free
minor, and record the value with its reason in the merge notes (FR-090, hard
rule 11). Both hand-kept ABI numbers move, each for a measured reason in the
form the comments at `CMakeLists.txt:43-52` and `:238-260` record:

| Target | Now | Next | Names that moved the layout |
|---|---|---|---|
| `speedgun-ng` archive | 3 (`CMakeLists.txt:53`) | 4 | `FakeProvider` gains the scripted group refusal of D-09: the class is exported (`counters_fake.hpp:103`) and the scripted state joins its members (spec PC-3; hard rule 12) |
| `harness` archive | 10 (`CMakeLists.txt:261`) | 11 | `hunt.hpp` adds the exported hunt calls; `BenchmarkResult` gains `hunt`; `ReportData` gains `hunt` (hard rule 12) |

`docs/schema/report-v1.json` gains both hunt keys at schema minor `1.4`,
`kSchemaVersion` at `json_report.cpp:31` moves from `1.1` to `1.4`, and a rebase
takes the next free minor while carrying the keys of the lanes that merged
first (FR-053). The full key rule, the `x-order` positions and the `required`
exception are in
[contracts/report-schema-key.md](contracts/report-schema-key.md).

### Shared-list ledger

Every file the change touches outside its own new files, with the edit:

| File | Edit |
|---|---|
| `CMakeLists.txt` | `VERSION` 0.16.0 to 0.19.0 (line 9), archive `SOVERSION` 3 to 4 (line 53) with its comment, harness `SOVERSION` 10 to 11 (line 261) with its comment |
| `test/CMakeLists.txt` | seven `add_executable` blocks with their `add_test` lines (pattern at 1079-1091), `SG_CLI_SOURCE` on the table test, `SG_SCHEMA_PATH` on the hunt JSON test, one `report_shape_hunt_rank_sample` entry (pattern at 1185-1192) |
| `example/CMakeLists.txt` | one `add_docs_example(hunt_rank)` line after line 74 |
| `README.md` | new `### Hunter mode` section after `### Regression gate` (line 621), at or under 25 section lines; eight option rows in the option table (ends at line 766); one quick-reference name row per new public type |
| `docs/pages/harness.md` | the "Hunter mode" option detail and the hunt object shape; line 1067 states the version a run writes and moves with the schema minor |
| `test/source/harness_json_load_test.cpp` | line 113 builds the current-version document and line 529 plants the `schemaVersion` kind refusal, both beside the bumped version |
| `test/source/harness_compare_cli_test.cpp` | line 516 expects the console words `schema 1.1`; the value moves with the bump and the pre-1.1 baseline comparison of line 492 stays as it is |
| `tools/docs/example_drift.py` | `SECTIONS` gains `"Hunter mode"` (line 32), `SECTION_OWNERS` gains its owner (line 63), `PAGE_LINE_CAP` rises from 773 to the measured length with the recorded move note at its comment block (lines 100-155, FR-072) |
| `docs/schema/report-v1.json` | root `x-order` and `properties` gain `hunt`; entry `x-order` and `properties` gain `hunt`; `required` unchanged in both |
| `test/source/harness_cli_table_test.cpp` | created here: master has no such file at `bd10d76`, and hard rule 10 gives it to the first lane that merges |
| `test/source/harness_pass_report_test.cpp` | unchanged expectation; its byte-identity golden is the covering check that the non-hunt seat route did not move |

## Phases

Task-level detail belongs to `tasks.md`. Each phase names its decisions
(`D-0n`), risks (`R-n`) and gate (`V-n`).

| Phase | Deliverable | Decisions | Risks | Gate (observable end state) |
|---|---|---|---|---|
| P1 Setup | version 0.19.0 with the two `SOVERSION` raises, the new headers and translation units present, the seven test executables registered, `hunt_fixture.hpp` | D-20, D-18 | R-2 | V-1: `cmake --preset=ci-rocky && cmake --build build` builds with the new files present, `ctest --test-dir build -j 2` passes |
| P2 Shared statistics | `statisticsOf` moved to `detail/statistics.hpp` plus `statistics.cpp`, three call sites repointed, no behaviour move | D-05 | R-1 | V-2: `harness_statistics_test.cpp` and `harness_pass_report_test.cpp` pass unchanged, and the byte-identity golden still matches |
| P3 Scripted refusal | `setGroupLeafLimit` in the exported fake and in `fake_provider.cpp` | D-09 | R-3 | V-3: `counters_fake_test.cpp` refuses a wide leaf set and accepts a narrow one, and `counters_objects_limit_test.cpp` still passes |
| P4 US1 discovery and plan | `EventCatalog`, `planHunt`, the core device rule, the anchors, the limit rows | D-01 to D-04, D-06 | R-3 | V-4: `harness_hunt_plan_test.cpp` passes every limit row (1, 3, 4, 6, 8, no limit, 200 events with limit 6 gives 4 events per pass with both anchors once each) |
| P5 US1 rank and run | `rankEvents`, `runHunt`, the skipped list, the seat error route, the in-loop retry | D-07, D-08, D-11, D-12, D-14 | R-1, R-3 | V-5: `harness_hunt_rank_test.cpp` and `harness_hunt_collect_test.cpp` pass, including the scripted group-refusal retry and the threaded window-count test |
| P6 US2 budget and plan print | `HuntBudget` parse, the two-stage split, the printed plan and estimate | D-13, D-06 | R-4 | V-6: `harness_hunt_plan_test.cpp` reproduces the fixed two-stage plan for the fixed seed and estimate (AC-7), and `harness_hunt_cli_test.cpp` refuses a malformed budget with exit 2 |
| P7 US3 JSON and schema | both hunt objects, the readers, `report-v1.json` at 1.4, `kSchemaVersion` | D-10 | R-2 | V-7: `harness_hunt_json_test.cpp` round-trips every field, `harness_json_schema_test.cpp` passes, and a non-hunt document differs from the master capture only in `schemaVersion` |
| P8 Console and CLI | the eight options, `hunt_cli.cpp`, `cli.cpp` rows, the printers in `report.cpp`, `harness_cli_table_test.cpp` | D-15, D-16, D-17, D-18 | R-2 | V-8: `harness_hunt_cli_test.cpp` and `harness_hunt_report_test.cpp` pass, `harness_cli_table_test.cpp` reports unique names and unique values, and `bash test/cli_shape.sh` stays clean |
| P9 US4 documentation | the docs example with its golden, the README section, the quick-reference and option rows, `docs/pages/harness.md`, `example_drift.py` lists and cap | D-19 | R-2 | V-9: `example_drift`, `example_drift_fixtures`, `snippet_compile`, `report_shape`, `report_shape_hunt_rank_sample`, `example_run` and `example_run_fixtures` pass, and the prose gates pass on every page |
| P10 Polish and landing | coverage with `gcov -b` evidence, the determinism scan, the `ci-ubuntu` analyzer build, the sanitizers, the thread sanitizer, the AC-10 hardware records, the rebase and one `--force-with-lease` push | D-20 | R-2, R-5 | V-10: line, branch and DBC coverage recorded at 100 percent with no exclusion, every Principle VIII gate green on the pushed state, `git ls-remote origin 029-hunter-core` shown in the record, every CI job read after the push including the coverage artifact |

Step-and-verify (X.4): after every phase the `ci-rocky` build, `ctest -j 2` and
the prose gate over the changed artifacts pass before the next phase starts.
The `ci-ubuntu` analyzer build runs once in P3 (first library change) and once
after P5 (first harness behaviour change), as Principle IX requires.

## Test Plan

**Hard rules bound this plan.** No test sleeps, reads a clock, or assumes a CPU
count; a test that needs a CPU set calls `sched_getaffinity` (the precedent
`harness_threads_test.cpp` carries). Every event value comes from the scripted
provider, so CI needs no PMU access and no host with `perf_event_paranoid` set
below 2. Coverage is 100 percent line and 100 percent branch on every new file
with no exclusion marker, no waiver and no residual row, measured under the
unchanged `coverage` preset with `lcov --filter brace`; a gap stops the work and
reports the cause with `gcov -b` evidence (FR-091, FR-092).

| User story | Verification approach | Preconditions exercised |
|---|---|---|
| US1 plan (P4) | `planHunt` decision tables over scripted limits and event counts; one column per event; the anchors claimed once per pass | limit 1 (no event fits), 3 (one event), 4, 6 with 200 events (50 passes of 4), 8, no limit, empty selection, duplicate addresses, two core devices, an uncore address by list |
| US1 rank and run (P5) | scripted values through the run: a high-rate event ranks first, a flat event above the CV threshold prints `noisy` and stays out of the top N; the skipped list carries each `Availability` kind | scripted high rate, flat high CV, `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`, mid-run `GAP`, all-refused host (D-12) |
| US1 retry (P5) | `setGroupLeafLimit` refuses the wide pass and admits the three-leaf pass; the retried event that still fails joins the skipped list; exit status 0 | pass-0 refusal (still `fail`), a later pass refusal, a lone-event refusal, a non-hunt run keeping the invariant route |
| US1 threads (P5) | two contexts, window-open counts per pass read from the scripted provider, no sleep and no clock | overlap attempt, anchor claimed once per context pass |
| US2 budget (P6) | fixed seed plus fixed estimate against the expected two-stage plan (AC-7); budget parse rows | `120s`, `8p`, `0s`, `1p`, no suffix, negative, non-numeric, budget larger than the plan, budget smaller than one pass |
| US2 plan print (P6) | captured stdout: plan before the run, estimate line naming the budget, `MULTIPLEX` disclosure on a no-limit device | estimate given, estimate absent (`estimate=not available`), no-limit host |
| US3 JSON (P7) | write through the writer, read with `loadReport`, compare field by field; schema check through `SG_SCHEMA_PATH`; one test per refused reader code | full hunt document, absent hunt object, wrong type per key, missing nested key, pre-024 document still loads, unknown key in the open root |
| US3 no-hunt identity (P7) | byte compare against the master capture with `schemaVersion` excluded; console and status byte compare | a plain run, a run with named counters, a `--compare` run |
| CLI refusals (P8) | in-process `speedgunMain` for the status-2 returns, forked children for the `std::exit(2)` refusals with the captured single stderr line (the `harness_cli_test.cpp` pattern at its lines 800, 1166 and 1417) | bad glob, unknown address, bad budget, bad norm, bad rank key including `sensitivity`, `delta` and `score`, repeats 0 and 1001, top 0 and 10001, `r4a03`, `--hunt` with `--counter`, `--metric` or `--compare`, missing `@file`, empty address line, `--hunt=none`, `--hunt` with no value |
| CLI table (P8) | `harness_cli_table_test.cpp` parses the one `option longOptions[]` table of `cli.cpp` through `SG_CLI_SOURCE` and asserts unique names and unique values, then asserts 270 to 279 are taken and 280 to 289 free | the planted duplicate in the fixture twin of the table |
| US4 page (P9) | `snippet_compile` compiles the copied block, `example_run` runs `docs_hunt_rank`, `report_shape_hunt_rank_sample` compares it with `hunt_rank.expected` by shape, `example_drift` checks the section, its owner, the name rows and the eight option rows | the recorded `PAGE_LINE_CAP` move, the 25-line section cap, the host-independent skipped list |
| Statistics (P2) | existing `harness_statistics_test.cpp` cases plus the hunt CV equality check: the CV a hunt reports equals the `cv=` column of the same samples | empty list guard, one sample, even count, zero mean |

**Existing tests touched**: `test/CMakeLists.txt`, `example/CMakeLists.txt`,
`tools/docs/example_drift.py`, `README.md`, `docs/pages/harness.md`,
`docs/schema/report-v1.json`. No test is deleted, skipped or weakened, and no
golden is regenerated.

**Determinism**: the scripted values are fixed; the seed shuffle is an
in-repo linear congruential Fisher-Yates (D-06), so the permutation stands
across GCC and Clang and across standard library revisions; every decimal the
tests compare comes from exact integer fixture values; no test reads a clock,
so the estimate line is driven by a supplied estimate and by a missing estimate
and by nothing else.

**Intentionally untouched**: `compare.cpp` and every comparison rule key (spec
Scope), `passSpread` and the upper-median convention of
`source/counters/plan.cpp`, `printContextDisclosure`, `kMaxUserCounters` and the
named-counter slot arithmetic at `runner.cpp:866-874`, `MetricMethod` and
`settleMethod`, the `coverage` preset and the CI workflow files (hard rule 14),
and the counters encode path (raw codes stay refused, spec D-11).

## Rebase and lane rules

Three lanes move at once: specs/027 (N13a, thrawn), specs/028 (N7, gary) and
this one. The rules the spec records bind the landing (FR-094):

- Rebase onto master, never merge master in; one `--force-with-lease` push per
  round, with `git ls-remote origin 029-hunter-core` shown in the record; the PR
  is never merged by this work.
- `CMakeLists.txt`: take the next free minor over what master states at rebase
  time, keep both `SOVERSION` comments, and record the value with its reason in
  the merge notes. If this lane merges first it takes `0.17.0` and the other two
  lanes rebase.
- `docs/schema/report-v1.json`: take the next free minor and carry the keys of
  the lanes that merged earlier in the same document (hard rule 11, FR-053).
- `cli.cpp`: specs/028 moves every option into one internal table. This lane
  keeps every parse and refusal body in `hunt_cli.cpp`, so a rebase onto that
  table moves eight rows and eight named constants and no logic (spec D-22).
- `README.md` and `tools/docs/example_drift.py`: keep every lane's section, name
  rows and option rows, then re-measure the page and set `PAGE_LINE_CAP` to the
  measured length with the move note beside it (FR-072).
- `test/source/harness_cli_table_test.cpp`: hard rule 10 gives the file to the
  first lane that merges. Master holds no such file at `bd10d76`, so this lane
  creates it; a lane that merged it first is absorbed by keeping that file and
  deleting only this lane's duplicate registration.
- Every gate runs after each rebase, including `ci-sanitize`, `ci-tsan`, the
  `ci-ubuntu` analyzer build and the coverage target, and every CI job is read
  after each push, the coverage artifact included (hard rule 13).

## Constitution Check (re-evaluated after design)

**Status**: PASS. No gate weakened, no exclusion added, no waiver requested and
no suppression introduced. Principles VI, VII, VIII, X and XI and the Additional
Constraints entries for counters-only, library-first and simdjson-only hold with
the design as written above, each at the evidence its line names. The
Complexity Tracking table stays empty: the feature is one public header, four
translation units, one moved helper, one scripted parameter, eight option rows
and one schema minor, and every item names the requirement it serves.

## Complexity Tracking

| Violation | Why it is necessary | Simpler alternative rejected |
|---|---|---|
| (none) | | |
