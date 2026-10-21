# Implementation Plan: Multi-pass metric collection

**Branch**: `024-multipass-collection` | **Spec**: [spec.md](spec.md) |
**Audit point**: `7a75e95` (master, 2026-10-20) | **Date**: 2026-10-20 |
**Constitution**: v2.20.0 (ratified 2026-09-06, amended 2026-10-14)

## Summary

A run whose metric columns ask the core PMU for more events than one context
can hold splits the columns into passes by the published programmable-counter
limit, runs the benchmark once per pass with the iteration count calibration
settled beforehand, and publishes one result built from all passes. The
counters library learns the limit from the CPUID leaves its per-vendor rules
name and publishes it as a fact of each device it enumerates; a device whose
rule answers nothing publishes nothing, and the kernel rotation plus the
`ratio=` disclosure stays as the disclosed answer there. The split itself lives in the harness: a pure first-fit
planner, a pure merge, and a pure spread sit in one new translation unit
beside `percentiles.cpp`, so the fake provider drives every branch with no
hardware, no clock and no sleep. The caller chooses the method with
`--metric-method=auto|passes|multiplex` or the `metricMethod` handle selector,
the command line winning, and the report discloses the method, the pass count
and the pass spread. A one-pass run under the default method prints what
master prints today, byte for byte, which is what keeps the specs/021
percentiles lane and this lane from disturbing each other's recorded runs
(spec Assumptions D-9).

## Technical Context

**Language/Version**: C++23, ISO-strict (`CMAKE_CXX_EXTENSIONS OFF`,
`CXX_EXTENSIONS OFF`), the GCC and Clang release toolchains of the two Linux
presets (`CMakePresets.json:139,143`)
**Primary Dependencies**: Linux `perf_event_open` (unchanged call shape);
`<cpuid.h>` `__get_cpuid` (already used, `table_parse.cpp:46,71,90`);
vendor simdjson, yaml, quill, hdrhistogram, hwloc (unchanged). The pinned
event-table trees stay the only event-name source (spec PC-3)
**Storage**: n/a. Recorded outputs: one new golden `.expected` beside the
others (`example/docs/percentiles.expected` is the pattern)
**Testing**: GoogleTest registered through `add_test` per executable
(`test/CMakeLists.txt:561-565`), `.expected` golden fixtures diffed by
`test/example_run.sh` (`example/docs/percentiles.expected`), gate scripts
under `test/`
**Target Platforms**: Linux x86-64 only; Ubuntu 26.04 and Rocky Linux 10,
containerised runners (the 022 plan records the fleet; the presets stand at
`CMakePresets.json:139,143`); development and recorded runs on `corsairone`
(the spec's host record)
**Project Type**: library (two shared objects: `speedgun-ng` and `harness`) +
documentation site + example suite
**Performance Goals**: no allocation or vendor-library call inside the timed
window; one-pass timed path byte- and read-count-identical to master; no
counter read, no clock read in planning, merging or disclosure
(SC-06, SC-07, SC-09; spec FR-008, FR-036; Principle VII)
**Startup Time**: seven CPUID reads and one `/proc` read at PMU provider
construction, each once for the process; no per-run cost
**Memory-Bound**: n/a; the pass plan holds one index per metric column
**Scale / Scope**: per run, one metric family's column set: the spec's PC-3
measured a 64-event oversize set on the development host, so the planner
tables treat a 64-column set as its large input. corsairone publishes a
4-counter limit on its `cpu` device (CPUID leaf 0AH `EAX[15:8]` = 4, measured
2026-10-20;
spec PC-3 records the oversize-group rotation on that host)

## Constitution Check

**Status**: PASS

- **I. Standard-First** | PASS | `std::string_view` keys, `std::optional` for
  the limit fact and the spread, `std::size_t` counts, the closed
  `enum class MetricMethod` for the selector; no new third-party
  dependency (research D-01, D-02, D-07)
- **II. DbC** | PASS | each new public name states its contract in the house
  form; the planner, the merge and the spread are pure functions with stated
  pre- and post-conditions; `SG_ASSERT` guards the internal invariants the
  code relies on (data-model.md invariants I-1 to I-6; `harness/detail/percentiles.hpp`)
- **III. R-DCUT** | PASS | the pure decision units are separate files,
  reachable from their own tests; no test sleeps or waits, the threaded
  pass-order test subscribes to window open and close counts on the fake
  provider; the merge holds no lock (research D-05, D-14)
- **IV. Documentation** | PASS | README quick reference, one `###` section and
  the option table each gain the new names, the detail page carries the full
  contract, the `example_drift` option check keeps the option table honest,
  `example_run` runs the new docs example through its own binary,
  `snippet_compile` compiles the copied block over the canonical includes,
  `report_shape` compares the printed report with the sample block by shape,
  and the `spell-check` and `prose-lint` targets bind the prose (research
  D-13; constitution IV.1 as amended in 2.20.0)
- **V. Style** | PASS | camelBack functions, PascalCase types, `k`-prefix
  constants, members with `m`, no abbreviations, include-what-you-use, Doxygen
  on every public header name
- **VI. Coverage** | PASS | every split, merge, spread and disclosure decision
  is a pure function of a fakeable input (the limit is a fake parameter), so
  the fake provider reaches 100 percent line and branch on every new file in
  both CI configurations; no new `LCOV_EXCL` marker anywhere in the tree
  (spec PC-5, FR-032; research D-04, D-08, D-14)
- **VII. Performance** | PASS | the split completes before the first window
  opens, the timed path is untouched for a one-pass run, the plan compile per
  pass reuses the one existing call site (`runner.cpp:1253` single-context,
  `runner.cpp:1727` per context threaded), and the disclosure reads published
  fields (research D-06; SC-09)
- **VIII. CI Gates** | PASS | the `format-check`, `spell-check`, `prose-lint`
  and `dbc-gate` targets, the commit template carried by `prose-lint`, the
  `dbc_dependency_scan`, `time_source_gate`, `cli_shape`, `loop_shape` and
  `barrier_shape` tests, the docs chain `example_drift`, `snippet_compile`,
  `report_shape` and `example_run` with their fixture probes, clang-tidy and
  cppcheck from the `ci-ubuntu` preset, the sanitizers from the `ci-sanitize`
  preset and the `coverage` target at 100 percent line, 100 percent branch
  and 100 percent DBC all pass with the change; the real-PMU comparison is a
  recorded run outside CI, so the fleet's rootless CI hosts gate nothing
  hardware-shaped (research D-15; spec FR-035, FR-037; constitution VIII as
  amended in 2.20.0)
- **IX. Spec-Driven** | PASS | spec approved with a clarifications session;
  this plan is its Phase 0 and Phase 1 output; the user stories map to phases
  below
- **X. Anti-Slop** | PASS | no new dependency, no wrapper type over a
  published `std::optional<std::size_t>`, no interface or policy tag, no
  speculative multi-provider scheduling beyond what FR-005 and the two
  provider edge cases state; the whole mechanism is three pure functions, one
  runner loop, one printer and one parser (research D-01 to D-15, Complexity
  Tracking empty)
- **XI. Discourse and Prose** | PASS | every artifact passes
  `prose_gate.py --check prose`; requirement statements use *shall*; the
  measured limit, spread percentages and thresholds carry their units

**Requested gate weakening**: none. The feature adds no exclusion rule, no
`-D` flag and no suppression; it inherits the two inherited CI host limits
recorded in the 022 plan (rootless CI hosts without hardware
`perf_event_open` access); this feature's own fact design (a fakeable limit)
turns that wall into a fake-provider parameter (spec PC-3; FR-006).

## Project Structure

### Documentation

```text
specs/024-multipass-collection/
├── spec.md              # approved; clarifications session 2026-10-20
├── plan.md              # this document
├── research.md          # D-01 to D-15
├── data-model.md        # E-01 to E-08, invariants, state tables
├── quickstart.md        # build, test and recorded-run guide
└── contracts/
    ├── metric-method-option.md      # CLI option, exit status 2 refusals
    ├── metric-method-selector.md    # handle selector and enum
    ├── programmable-counters-fact.md # the published limit, fake parameter
    └── pass-disclosure.md           # printed lines, pass tag, golden shape
```

### Source

The tree grows by four new source files and one new test directory entry set;
no directory is restructured.

```text
include/speedgun-ng/
├── counters_provider.hpp   # E-02: ObjectSeed gains the published limit
├── counters_system.hpp     # E-02: Object::programmableCounters()
├── counters_fake.hpp       # E-02: FakeProvider::setProgrammableCounters()
└── benchmark.hpp           # E-01 MetricMethod, metricMethod selector,
                            # E-07 result fields, E-08 MetricValue pass tag

source/counters/
├── linux_pmu/counter_limits.cpp   # E-02: CPUID and watchdog reads, the
│                                  # per-vendor and per-device decode
├── linux_pmu/provider.cpp         # publish the fact on PMU objects
├── linux_pmu/table_parse.cpp      # unchanged read site; probe reuse
└── fake_provider.cpp              # publish the scripted limit

source/harness/
├── detail/pass_plan.hpp    # planner, merge, spread: pure decision units
├── pass_plan.cpp           # their definitions, same shape as percentiles.cpp
├── runner.cpp              # method resolution, pass loop, merge call
├── cli.cpp                 # --metric-method parsing
└── report.cpp              # E-07: printPassDisclosure, pass tag field

example/docs/
├── metric_passes.cpp       # the docs example, owner of README's new section
└── metric_passes.expected  # golden of its recorded run (fake-free, so it
                            # runs on the dev host; see research D-13)

test/source/
├── counters_counter_limits_test.cpp  # decode decision table (synthetic regs)
├── counters_objects_limit_test.cpp   # publication via fake and PMU providers
├── harness_pass_plan_test.cpp        # split/merge/spread decision tables
├── harness_pass_collect_test.cpp     # fake-provider runs, threaded order
└── harness_pass_report_test.cpp      # disclosure, warning, byte-identity

tools/pmu_events/
└── multipass_comparison.sh  # corsairone real-PMU script, outside CI
```

**Structure decision**: the library keeps measurement and publishes facts;
planning stays in the harness. The limit fact travels the same
provider-to-`Object` publication path every other catalog fact travels
(`counters_provider.hpp:62`, `counters_system.hpp:34`), which is the dogfood
rule the 022 plan states at its D-01: the harness reads the machine through
the `counters` surface, never through `/proc`, sysfs or a CPUID call of its
own. The planner lives in `source/harness/` beside `percentiles.cpp` because
the plan, the merge and the disclosure are run-policy the library deliberately
does not own: the library compiles whatever leaf set it is given and lets the
kernel rotate (the behaviour FR-024 pins for the `multiplex` method). Splitting
into passes changes what the harness runs, the same way 021's chunking lives
in the harness (021 plan D-14).

## Design

### Decision index

Full rationale and alternatives in [research.md](research.md).

Decision numbering: a bare `D-n` is one of the spec decisions `D-1` to
`D-12` in [spec.md](spec.md); a zero-padded `D-01` to `D-09` is one of the
research decisions `D-01` to `D-15` in [research.md](research.md) that this
plan's index reproduces. The two ranges meet at 10, so a reference of 10 or
above names its file, as in `research D-10`.

| # | Subject | Decision |
|---|---------|----------|
| D-01 | Learning the limit | the vendor from the leaf 0 words, the core answer from leaf 0AH for Intel and Zhaoxin and from the extended leaves for AMD, answered per device and adjusted for the watchdog; a hybrid host that answers leaf 0x23 publishes nothing for its core devices; measured on corsairone: `EAX=0x07300404`, four programmable counters |
| D-02 | Publishing the limit | `ObjectSeed` gains `programmableCounters` (`std::optional<std::size_t>`), `Object::programmableCounters()` reads it |
| D-03 | Fake parameter | `FakeProvider::setProgrammableCounters(objectPath, limit)`, fluent, unset objects publish nothing |
| D-04 | Read site and decode | branchless probe of every leaf the rules read; pure `vendorOf`, `intelCoreLimit`, `amdCoreLimit`, `fixedCycles`, `isHybrid`, `limitForDevice` and `usableCounters` decision functions |
| D-05 | Planner location | harness-internal `detail/pass_plan.hpp` + `pass_plan.cpp`; pure first-fit over columns in request order, per-source limits |
| D-06 | Pass execution | pass loop wraps the measured attempts; per pass the existing `compileCore` sites compile that pass's leaf list; windows close with the pass's plan |
| D-07 | Merge | metric and counter columns from the measuring pass; time columns, floor, endpoint samples, contexts, percentiles and chunk disclosure from pass 0 |
| D-08 | Spread | pure `(max - min) / median` over merged reference times; empty for fewer than two passes or a zero median; no gcov-hostile shape |
| D-09 | Method plumbing | `--metric-method` wins over `metricMethod(...)` wins over `auto`; last flag wins; bad value prints one line and exits 2 |
| D-10 | Disclosure rule | console line when the run carries metric columns and the settled method names itself; `auto` one-pass fits print nothing new; fields always published |
| D-11 | Report printer | new `printPassDisclosure`; `printContextDisclosure` untouched; pass tag rides the metric field |
| D-12 | Version and SOVERSIONs | `0.13.0`; counters archive 2 to 3; harness 7 to 8 |
| D-13 | Documentation | `metric_passes.cpp` owns a README section under the 25-line cap; `PAGE_LINE_CAP` raise recorded with its measured number |
| D-14 | Tests | fake limit drives every decision table; threaded pass-order test with no sleep and no clock; byte-identity fixture captured from master |
| D-15 | Real-PMU script | `tools/pmu_events/multipass_comparison.sh` on corsairone, recorded, not a CI gate |

### Risks

- **Overlapping edits with the percentiles and contexts lanes.** The runner,
  `benchmark.hpp`, `report.cpp` and `cli.cpp` are shared files, and the
  percentiles lane is live. Rebase onto master before touching them, re-run
  the byte-identity capture after the rebase, and touch
  `printContextDisclosure` not at all (spec N4, D-8).
- **A metric that needs more events than the host has counters.** It keeps one
  pass with kernel rotation, the `ratio=` disclosure states the shortfall, and
  the test suite pins that scenario with the fake (spec D-1, spec D-2, US1
  scenario 2).
- **Pass times disagreeing because the machine drifted.** The spread warning
  exists exactly for this; the warning never refuses the run (spec D-6,
  FR-018).
- **Threading order.** Pass windows never overlap across contexts because the
  pass loop sits outside context dispatch and reuses the one existing
  start/stop barrier pair per pass (`runner.cpp:1709-1710`); the covering test
  counts window opens per pass on the fake provider (FR-013, FR-034).
- **gcov attributing phantom branches to new code.** Every decision is a pure
  function over plain values with one entry and one exit; the shape rules
  022 D-16 records apply (lcov carries `--filter exception,branch,brace`,
  `cmake/coverage.cmake:83`); no new exclusion marker, checked by grep in the
  coverage phase.

### Logical view

The decision spine of one metric-carrying run (E-numbers refer to
[data-model.md](data-model.md)):

```text
options + entry + published limits (E-02)
        |
        v
 resolve the method (E-01)          pass plan (E-03)
   auto + no limit  -> MULTIPLEX     first-fit over columns, request order:
   forced MULTIPLEX -> MULTIPLEX       a column joins the first pass whose
   limit published   -> PASSES          limit its PMU leaves still leave;
                                        a column over limit stands alone;
                                        sources with no limit ride along
        |                                   |
        v                                   v
 calibration once -> for each pass: compile, open, run every repetition,
 fold, close -> merge (E-04/E-05): metrics from their pass, time / floor /
 endpoint samples / contexts / percentiles from pass 0 -> spread (E-06)
        |
        v
 disclosure (E-07, E-08): fields on the result; console line when the
 settled method names itself; pass tag on each metric field
```

The whole figure left of the pass loop is the shape master has; the loop body
grows the pass dimension, and the three new pure functions decide it.

### Physical view

```text
  cli.cpp ----------- options (method value, E-01) -----------+
  benchmark.hpp ----- entry default (E-01 selector) ---------+
                                                             |
  counters_system.hpp Object::programmableCounters() <--+    v
        ^           (E-02, published at registration)   | counters_provider.hpp
        |                                               | ObjectSeed field
        | pmu provider: per-device limits publish there |
        +----------------------- runner.cpp <-----------+
                     (resolve, plan, loop, merge: pass_plan.hpp)
                              |
                              v
                    BenchmarkResult (E-07 fields)
                              |
                              v
                    report.cpp printPassDisclosure -> console
```

The counters archive publishes the fact; the harness archive reads it and
plans. The direction mirrors 022's physical view: the harness consumes the
library's published surface and holds its own policy.

### Public API surface added

| Name | Header | Role |
|---|---|---|
| `enum class MetricMethod` | `speedgun-ng/benchmark.hpp` | E-01: `AUTO`, `PASSES`, `MULTIPLEX`, named per constitution V |
| `auto BenchmarkHandle::metricMethod(MetricMethod) -> BenchmarkHandle&` | `speedgun-ng/benchmark.hpp` | the family's method default |
| `auto Object::programmableCounters() const -> std::optional<std::size_t>` | `speedgun-ng/counters_system.hpp` | E-02: the event source's answer |
| `ObjectSeed::programmableCounters` | `speedgun-ng/counters_provider.hpp` | the provider's published answer |
| `auto FakeProvider::setProgrammableCounters(std::string_view, std::size_t) -> FakeProvider&` | `speedgun-ng/counters_fake.hpp` | E-02: the scripted limit |
| `BenchmarkResult::metricMethod`, `passCount`, `passSpread` | `speedgun-ng/benchmark.hpp` | E-07: published disclosure |
| `MetricValue::pass` | `speedgun-ng/benchmark.hpp` | E-08: the measuring pass of one metric value |
| `--metric-method=auto\|passes\|multiplex` | CLI | E-01: the caller's method |
| `printPassDisclosure` | internal (`report.cpp`) | E-07: the console line (not exported) |

`ResultRow`, `Statistics` and `MetricView` are untouched, so no specs/021
percentiles member and no placement-lane member is re-read by this feature
(specs/021, spec N4, D-8).

### Version and SOVERSIONs

`CMakeLists.txt` carries the version rules: the project version moves from
`0.12.0` (CMakeLists.txt:7) to the next free minor, `0.13.0`, rebased against
CMakeLists.txt at landing (spec FR-031). Both archive `SOVERSION`s move. The
hand-kept number follows the 0.x rule stated at CMakeLists.txt:41-46 ("the
shared-object version carries the ABI number, which a consumer links
against"), and the trigger is the one the harness comment at
CMakeLists.txt:215-224 records twice: 020 moved the number to 6 because
"BenchmarkResult gains the contexts disclosure ... the exported layouts
change and the hand-kept number moves", and 021 moved it to 7 because
"BenchmarkHandle gains the percentile selector, so the exported layouts move
again":

| Target | Now | Next | Names that moved the layout |
|---|---|---|---|
| `speedgun-ng` archive | 2 (CMakeLists.txt:47) | 3 | `ObjectSeed` gains `programmableCounters`; `Object` answers it; `FakeProvider` gains the scripted fact (`counters_provider.hpp:62`, `counters_system.hpp:34`, `counters_fake.hpp:103`) |
| `harness` archive | 7 (CMakeLists.txt:225) | 8 | `BenchmarkHandle::metricMethod`; `BenchmarkResult` gains `metricMethod`, `passCount`, `passSpread`; `MetricValue` gains `pass` (`benchmark.hpp:211,440,1795`) |

## Phases

Task-level detail belongs to `tasks.md`; these are the phases, their binding
decisions (D-x), risks (R-x, from Risks above) and gates (V-x).

| Phase | Deliverable | Decisions | Risks | Gate (observable end state) |
|---|---|---|---|---|
| P1 Setup | `0.13.0` with the two `SOVERSION` raises, the new translation unit and its internal header, the five test executables registered, the shared fake fixture | D-05, research D-12 | R-1 | V-1: `cmake --preset=ci-rocky && cmake --build build` builds with the new files present and `ctest --test-dir build -j2` still passes |
| P2 Limit fact (counters) | CPUID probe + pure decode published on PMU objects; fake publishes its scripted limit | D-01, D-02, D-03, D-04 | R-2 | V-2: `counters_counter_limits_test.cpp` drives every decode arm with synthetic registers; `counters_objects_limit_test.cpp` passes |
| P3 US1 planner, passes, merge | first-fit plan, merge and spread as pure functions, the pass loop with one calibration, the threaded pass order | D-05, D-06, D-07 | R-2, R-3, R-4 | V-3: `harness_pass_plan_test.cpp` and `harness_pass_collect_test.cpp` pass, including the two-context order test with no sleep |
| P4 US2 method | enum, handle selector, `--metric-method` parse with the status-2 refusal, `settleMethod` wired before the first window | D-09, research D-10 | R-1 | V-4: `harness_pass_plan_test.cpp`, `harness_cli_test.cpp` and `harness_pass_collect_test.cpp` pass over every E-01 row, and `bash test/cli_shape.sh` stays clean |
| P5 US3 report | disclosure printer, warning line, pass tag, published fields, byte-identity fixture | D-08, research D-11 | R-3 | V-5: `harness_pass_report_test.cpp` and `harness_context_report_test.cpp` pass; the byte-identity fixture matches master's capture |
| P6 US4 documentation | README section with its quick-reference and option rows, `docs/pages/harness.md` detail, the docs example registered | research D-13 | R-5 | V-6: `example_drift`, `snippet_compile`, `report_shape` and `example_run` pass, and the prose gates pass on the page |
| P7 Polish and landing | coverage with `gcov -b` evidence, the determinism scan, the recorded `multipass_comparison.sh` run, the analyzer build, the whole gate set, the rebase and the push | research D-12, research D-15 | R-5 | V-7: line, branch and DBC coverage recorded at 100 percent, every Principle VIII gate green on the pushed state, the CI logs of the pushed commit read |

Step-and-verify (X.4): after every phase, `ci-rocky` build + `ctest -j2` +
the prose gate on changed artifacts pass before the next phase starts; the
`ci-ubuntu` analyzer build runs once in P2 (first library change) and once
after P3 (first harness behavior change).

## Test Plan

**Execution mode**: the library and harness scenarios run against the fake
provider (`FakeProvider` is the existing fixture mechanism); the PMU host
scenario runs only on `corsairone` through the recorded script (FR-033,
FR-035). Nothing in CI needs hardware.

| User story | Verification approach | Preconditions exercised |
|---|---|---|
| US1 split and merge (P3) | planner decision tables over column-count lists and fake limits 1, 4, 8; first-fit order; over-limit column alone; no-limit fallback; fake runs that check which pass measured each column | limit 1, n, n+1; empty set; all-one-column; duplicate leaves across columns; the 64-column set; a gap folded in one column; multi-source ride-along |
| US2 option (P4) | CLI in-process tests of the refusal lines and status 2; handle-selector defaults; command-line-wins and last-wins | `passes`, `multiplex`, `auto`, bad word, repeated flags, no-limit host, `--list` and `--catalog` runs |
| US3 one-pass identity (P5) | golden captured from master at the audit point; byte-for-byte compare; `printContextDisclosure` untouched check; forced-method one-pass disclosure | fit set; forced methods with metrics; run without metric columns |
| US3 spread and warning (P5) | fake runs over the fixture table of [data-model.md](data-model.md) E-06; the warning predicate arms; percentiles from the measuring pass; merge provenance | the silent, boundary, warning and zero-median fixtures; a two-pass set |
| US1 threads (P3, FR-034) | two contexts, two passes, window open and close counts per pass on the fake; barrier reused per pass | overlap attempt; placement identical across passes |
| US4 page (P6) | `snippet_compile` compiles the copied block, `example_run` runs the example binary, `example_drift` checks the section, the quick-reference rows and the option row | the recorded `PAGE_LINE_CAP` raise; the no-PMU shape |
| Limit fact (P2) | decode with synthetic `eax`/`ebx`/`maxLeaf`; fake publication; PMU host fact | version 0; gp count 0; leaf absent; corsairone answers 4 |

**Existing tests touched**: `harness_cli_test.cpp` (the new option path),
`test/CMakeLists.txt` and `example/CMakeLists.txt` (registration blocks per
`test/CMakeLists.txt:561-565` and the `add_docs_example` pattern at
`example/CMakeLists.txt:40-48`), `tools/docs/example_drift.py` (`SECTIONS` at
its line 30, `SECTION_OWNERS` at its line 57, `PAGE_LINE_CAP` at its line 124),
`README.md` (quick reference at its line 556, option table at its line 646, one
new `###` section). Option-table parity is one gate: `example_drift` compares
the README option rows with the table in `source/harness/cli.cpp`, and T053,
T056 and T057 of `tasks.md` carry it. No existing test is deleted, skipped or
weakened; no golden is regenerated (SC-03; spec Assumption "the percentiles
lane owns its own rebakes").

**Registration pattern**: each new `test/source/*.cpp` gets an explicit
`add_executable` block plus `add_test(NAME … COMMAND …)`. A golden under
`test/source/detail/` reaches its test binary as the first command-line
argument: the `add_test` call passes the golden's path through a generator
expression over `CMAKE_CURRENT_SOURCE_DIR`, and the test opens the file with
`std::ifstream`.

**Gates that must stay green**: the `format-check`, `spell-check`, `prose-lint`,
`dbc-gate` and `coverage` build targets; the `dbc_dependency_scan`,
`time_source_gate`, `cli_shape`, `loop_shape` and `barrier_shape` tests; the
docs chain `example_drift`, `example_drift_fixtures`, `snippet_compile`,
`report_shape`, `example_run` and `example_run_fixtures`; clang-tidy and
cppcheck from the `ci-ubuntu` preset; the sanitizers from the `ci-sanitize`
preset; and the commit-message gate `prose-lint` carries. Coverage is 100
percent line, 100 percent branch and 100 percent DBC. The coverage preset keeps
`-fprofile-update=atomic` and passes no `-D_GLIBCXX_ASSERTIONS` (spec FR-037;
the `SPEEDGUN_COVERAGE` fixture convention the 022 plan records).

**Determinism**: no sleep, no wall clock, no real PMU inside any test; the
fake's scripted values are fixed; the spread fixtures of data-model E-06 are
exact integers, so every printed decimal is stable (SC-10, FR-036).

**Intentionally untouched**: `printContextDisclosure` (N4 lane), the plan
internal layout in `source/counters/detail/core.hpp`, `ResultRow`,
percentile computation, `metric_format` and metric tables, the catalog and
list modes, the JSON report (another lane owns it).

## Constitution Check (re-evaluated after design)

**Status**: PASS. No gate weakened and no exception requested. The Complexity
Tracking table stays empty: the mechanism is three pure functions, one loop,
one printer, one parser, one fact and one fake parameter; every item names its
FR.

## Complexity Tracking

| Violation | Why it is necessary | Simpler alternative rejected |
|---|---|---|
| (none) | | |
