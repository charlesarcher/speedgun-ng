# Implementation Plan: Percentiles and Raw Capture

**Branch**: `021-percentiles-raw-capture` | **Date**: 2026-10-19 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/021-percentiles-raw-capture/spec.md`

## Summary

The timed path already samples the counters plan: `State::openWindow` and
`State::takeExitSample` each call `RecorderHandle::sample()`
(`include/speedgun-ng/benchmark.hpp:980`, `:998`), and a run of no pauses
leaves two points per column in the hardStop recorder the attempt mints
(`source/harness/runner.cpp:1553`). The chunked capture keeps that machinery
whole and changes one thing: the loop cursor counts down one chunk, takes one
sample at the boundary, and refills from the run's remaining count. A run of
N chunks then leaves N+1 points per column, `detail::foldCore` produces the N
chunk values after the run, and one new pure helper turns the sorted chunk
values into nearest-rank percentiles the console report prints under the run
row. `Plan::sampleOverheadNsMedian()` already measures one `sample()` action
and already fills `ResultRow::overheadFloorNs` (`source/harness/runner.cpp:1065`),
so the observer cost needs no new measurement and no new report field, and the
same value sets the chunk-size floor of the 1% rule.

## Technical Context

**Language/Version**: C++23, the GCC and Clang versions the CI matrix pins; the
standard-first rule of Principle I decides every construct

**Primary Dependencies**: the counters library of this repository: `Plan`,
`RecorderHandle<HardStop>`, `RecorderApi`, `Expression::fold`,
`detail::foldCore`, `PointsView`
(`include/speedgun-ng/counters_measurement.hpp`). No new external dependency.
The vendored HdrHistogram stays unused (research D-06)

**Storage**: none. The capture lives in memory, and the row keeps its chunk
value columns as the residue a later JSON report reads (research D-14)

**Testing**: the repository's harness tests in `test/source/` on
`FakeProvider`, and the shape gates `test/loop_shape.sh`,
`test/time_source_gate.sh`, `test/barrier_shape.sh`, `test/cli_shape.sh`

**Target Platform**: Linux x86-64; `ci-ubuntu` and `ci-rocky`

**Project Type**: library plus harness, one package, no service

**Performance Goals**: one sample point costs at most 1% of the chunk it
bounds (FR-005); the timed loop keeps the shape the loop gate measures

**Constraints**: no allocation inside the timed window (FR-008); no raw point
dropped to fit (FR-006); no fold crosses contexts (FR-004); the coverage pair
gate stays at zero drifted interfaces

**Scale/Scope**: 3 new source files, 6 edited source files, 5 new test files,
1 new docs example pair, 2 new command-line options, 1 new constant

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Requirement | Status | Evidence / plan reference |
|---|---|---|---|
| I. Standard-first | No non-standard construct where the standard library serves | PASS | the percentile helper uses `std::ranges::sort` and integer rank arithmetic; the chunk path calls the counters seam the repository already ships; no new `asm` site and no intrinsic header, so no P2 exception is registered |
| II. Design by Contract | Contracts on every new function; zero drifted interfaces | PASS | contracts in `contracts/capture-and-chunks.md`; the cursor refill and the percentile helper carry `\pre` and `\post`; the DBC pair gate covers the new signatures |
| III. R-DCUT | Requirements, design, code, tests, docs traceable | PASS | FR to D to E to T to contract mapping in this plan, `research.md`, `data-model.md`, `contracts/`, `tasks.md` |
| IV. Documentation | Docs with the feature; runnable snippets; main-page impact | PASS | `contracts/page-and-examples.md`; the Percentiles section, its `example/docs/percentiles.cpp` region, its quick-reference rows, and the `docs/pages/harness.md` detail; the drift gate enforces them |
| V. Code style | clang-format, clang-tidy, cppcheck, naming | PASS | new files carry the license header; `readability-identifier-naming` stays clean |
| VI. Coverage | 100% line and branch, no exclusion | PASS | every new branch is a test case in `tasks.md` Phase 9; no `LCOV_EXCL` under `source/harness/` |
| VII. Performance discipline | Measured claims, distributions, long tails | PASS | the feature is the distribution rule; the chunk rule and the 1% bound are measured in `test/source/harness_percentile_test.cpp` and the overhead suite |
| VIII. Hard CI gates | Every gate green | PASS | `quickstart.md` runs the full set |
| IX. Spec-driven | EARS requirements, user stories, this gate | PASS | spec.md carries 35 FRs, 6 stories, 11 SCs; this gate is the record |
| X. Anti-slop | No speculative abstraction; smallest fit | PASS | research D-01, D-07, D-14: the cursor keeps its one counter, the percentile helper is one free function, no histogram, no interface, no policy tag |
| XI. Discourse | Prose and commit rules | PASS | `prose-lint` over every artifact; commit template in `tasks.md` |
| Additional Constraints | Counters-only | PASS | the chunk sample calls `RecorderHandle::sample()`; the rank helper holds no clock read; `test/time_source_gate.sh` stays clean |

**Gate outcome**: PASS, no violations, so the Complexity Tracking table stays
empty. No new `asm volatile` site, no intrinsic header, no fourth project.

**Post-Design Re-check**: PASS, with two consequences the design surfaces and
the spec has to carry.

1. The harness `SOVERSION` moves 6 to 7 (research D-15): `ResultRow` gains
   fields and `BenchmarkHandle` gains a method, both public ABI in the harness
   archive, and the constitution's Refactoring and Evolution rule makes a
   public API change a versioned change. The `speedgun-ng` archive
   `SOVERSION` stays 2.
2. The `endpoint samples=` field of every run row changes value, because the
   cadence it counts changed (research D-10). FR-023 and SC-002 of the spec
   read as though the whole report kept its bytes; the claim that holds is
   that no percentile line prints when the output stands off. The
   `/speckit.analyze`
   pass of 2026-10-19 rewrote FR-023 and SC-002 to carry that claim, and
   FR-031 to carry the `SOVERSION` move.

## Project Structure

### Documentation (this feature)

```text
specs/021-percentiles-raw-capture/
├── spec.md
├── plan.md
├── research.md          # D-01 to D-16
├── data-model.md        # E-01 to E-10
├── quickstart.md
├── checklists/requirements.md
├── contracts/
│   ├── capture-and-chunks.md   # the cursor, the recorder, the chunk fold
│   ├── percentile-options.md   # the handle call and the two options
│   ├── report-shape.md         # the percentile lines and the changed field
│   └── page-and-examples.md    # the main page, the example, the budget
└── tasks.md             # T001 to T0nn
```

### Source Code (repository root)

```text
include/speedgun-ng/
  benchmark.hpp                 # ResultRow fields, BenchmarkHandle::showPercentiles,
                                # State::Cursor chunk fields (public, SOVERSION 7)

source/harness/
  percentiles.cpp               # NEW: the rank helper and the chunk value
                                # builder, pure, no clock, no allocation policy
  detail/percentiles.hpp        # NEW: the internal declarations
  detail/calibration.hpp        # kMaxChunkPointsPerLeaf and the chunk plan
  runner.cpp                    # the recorder capacity, the chunk plan per
                                # attempt, the chunk value build, the merge
  report.cpp                    # the percentile lines
  cli.cpp                       # --percentiles= and --no-percentiles
  detail/internal.hpp           # RegistryEntry and RunOptions fields, the
                                # RecorderReservation seam

example/docs/
  percentiles.cpp               # NEW: the docs example with its region
  percentiles.expected          # NEW: the rows its result sentence names
  reading_the_report.expected   # the endpoint-samples value the new cadence gives

test/source/
  harness_percentile_test.cpp   # NEW: rank rule, availability, merged sets
  harness_capture_test.cpp      # the chunked cadence, the point count, the bound
  harness_calibration_test.cpp  # the chunk plan and the 1% rule
  harness_cli_test.cpp          # the two options and the exit-2 forms
  harness_context_report_test.cpp # the per-context and merged percentile lines

test/loop_shape.sh              # the chunk-boundary probe
```

**Structure Decision**: the rank arithmetic is one pure translation unit in
the harness archive, because it is the feature's only new algorithm and the
coverage and sanitizer gates need it under the same flags as the runner. The
cursor change stays inside `benchmark.hpp`, because the cursor is the timed
loop and the loop shape gate reads that code back. No new library target: the
harness archive carries the feature, the same call 020 made.

## Sources & References

- **Spec**: `specs/021-percentiles-raw-capture/spec.md` - FR-001 to FR-035,
  SC-001 to SC-011, the five clarifications of 2026-10-19
- **Constitution**: `.specify/memory/constitution.md` - Principle I, II, IV.1,
  VI, VII, VIII, X, the Additional Constraints, and the Refactoring and
  Evolution rule that moves the `SOVERSION`
- **Prior specs**: `specs/020-execution-contexts-core/` for the per-context
  fold and the no-cross-thread rule; `specs/019-harness-user-counters-and-statistics/`
  for the pause room, the fold disclosure, and the aggregate statistics;
  `specs/015-benchmark-harness-core/contracts/api.md` for the loop shape
- **Code anchors read at `ab32400`**: `State::Cursor` and the window seams in
  `include/speedgun-ng/benchmark.hpp:485-1050`; `RecorderHandle`,
  `RecorderApi`, `PointsView`, `Plan::recorder`, `Plan::sampleOverheadNs*` in
  `include/speedgun-ng/counters_measurement.hpp:386-980`; `foldCore`,
  `foldPairsCore`, `rawCore` in `source/counters/fold.cpp`; `RunRecord`,
  `growUntilQualified`, `kMaxPauseSamplesPerLeaf` in
  `source/harness/detail/calibration.hpp`; the attempt samplers and the
  reservation seam in `source/harness/runner.cpp:1390-1620`; `printResult` in
  `source/harness/report.cpp:255-400`; the option scanner in
  `source/harness/cli.cpp:40-300`
- **Reference read**: Google Benchmark's console report carries the aggregate
  statistics and no percentile row, so this feature has no upstream row shape
  to copy. The plan pins the revision at implementation time and records any
  upstream shape it finds, the way 020 does.

## Phase 0: Design Decisions

See [research.md](research.md) for D-01 to D-16 with rationale and the
rejected alternatives.

## Phase 1: Data Model & Contracts

- [data-model.md](data-model.md): E-01 to E-10 with fields, invariants, and
  the memory arithmetic.
- [contracts/capture-and-chunks.md](contracts/capture-and-chunks.md): the
  cursor refill, the recorder capacity, the chunk fold, and the loop shape.
- [contracts/percentile-options.md](contracts/percentile-options.md): the
  handle call, the two options, the precedence, and the exit-2 forms.
- [contracts/report-shape.md](contracts/report-shape.md): the percentile
  lines, the changed `endpoint samples=` field, and the threaded disclosure.
- [contracts/page-and-examples.md](contracts/page-and-examples.md): the
  Percentiles section, the runnable region, the quick-reference rows, and the
  line budget.

## Task Planning Summary

See [tasks.md](tasks.md) for T001 to T041. Summary:

- **Phase 1 Setup**: 3 tasks - the constant, the internal types, the
  registration of the new translation unit.
- **Phase 2 Foundation**: 8 tasks - the chunk plan, the rank helper, the
  per-chunk pause netting, the cursor refill, the recorder capacity, the loop
  gate probe, and the capture tests.
- **Phase 3 to 8 User stories**: 23 tasks - US1 the time percentiles, US2 the
  counter columns, US3 the threaded rows, US4 the options, US5 the observer
  cost and the residue, US6 the page.
- **Phase 9 Polish**: 7 tasks - the gates, the version and `SOVERSION`, the
  budget, the sanitizer and coverage runs, the final prose pass.

## Implementation Strategy

MVP: Phases 1 and 2 plus US1: a single-context run whose primary time column
carries the four default percentiles, printed under the run row, with the
chunk size disclosed. That slice is independently testable through
`test/source/harness_percentile_test.cpp` and the docs example, and it
carries the whole capture mechanism, so every later story adds a column, a
row, or an option to it and changes nothing else.

Delivery order: US1, then US2 (the counter columns ride the same fold), then
US4 (the options decide who sees it), then US3 (the threaded rows), then US5
(the cost disclosure already stands, so its story is a test and a line), then
US6 (the page, last, because the section must quote the final shape).

## Implementation notes

Five decisions the artifacts could not settle before the code ran, each
settled against a measurement taken during implementation.

- **The null-body cost is accepted.** The chunk-boundary cursor cycle costs
  0.089 to 0.174 ns per iteration on the machines this branch was measured
  on, against the 100 x overhead floor that sets the chunk size. The loop
  gate (`test/loop_shape.sh`) carries the boundary probe, so a per-iteration
  sample fails it and the accepted cost is the measured cost.
- **Capture is gated on the output.** Chunk points are taken only when the
  percentile output is on for that instance, so a run that prints nothing
  pays no capture. `endpoint samples=` therefore stays at 2 for a run with
  the output off, which is what `reading_the_report.expected` records.
- **The chunk plan contract is amended.** `chunkPlanOf` takes the pause room
  the history earned and answers a plan whose recorder capacity is
  `chunkCount + 1 + pauseRoom`; the ceiling on the chunk count keeps the
  capacity inside `kMaxRecorderPointsPerLeaf`. The contract in
  [contracts/capture-and-chunks.md](contracts/capture-and-chunks.md) states
  the ceiling and the capacity together.
- **A run prints one percentile line per reported column.** The primary
  column first, then the time column the primary is not, then each metric
  column in report order. A plain benchmark reports both times, so it prints
  two lines; the cpu-time line appears when the primary is the real time.
  [contracts/report-shape.md](contracts/report-shape.md) and
  `docs/pages/harness.md` carry the same wording.
- **`example/docs/percentiles.expected` stands unreferenced.** Each docs page
  allows exactly one sample-output block per example, and the Percentiles
  section quotes the README region instead, so the recorded file has no
  region pointing at it. The drift tool reports it as recorded and unused.
- **The 100% bar removed the defensive guards.** A guard no test can reach
  is a branch the gate reads as uncovered, so the row builders dropped the
  bounds skip on a column index, the label fallback behind the metric
  column, the finite test on an all-digit option token, and the second
  operand of the capture gate. A run captures chunks only where the
  percentile output is on, so a nonzero chunk count already carries the
  points, and the iteration count reaching those sites is fixed by
  `parseCount`, which refuses `--iterations=0`.
- **gcov charges a function's exception cleanup to its closing brace.**
  `geninfo_unexecuted_blocks=1` turns that block into a line the gate
  reads, and a function whose only local is its own return value has no
  normal-path block to cover it. Two shapes clear it on their own merits:
  the column builder keeps one sort buffer alive for every column of the
  run (one allocation per run, one per column before), and the point
  parser fills the caller's vector through a reference parameter.
- **The percentile suite forks per scenario.** Each run scenario runs in a
  child that re-executes the test binary with `execv("/proc/self/exe",
  ...)` and dispatches on a mode argument, so every child writes its own
  `.gcda`. A child that leaves through the C++ runtime without an exec
  loses its libgcov counters, and the lines behind it read as uncovered.
- **The fortified standard library needs an append here.** Assigning a
  braced `std::vector<double>` list trips a false `-Wnonnull` inside the
  libstdc++ 16 fortified copy at `-O3 -D_FORTIFY_SOURCE=3`; the sites
  append their one point with `push_back`. It is the same toolchain defect
  that breaks `ci-sanitize` over `<regex>`.
- **The dbc pairing gate reads the enforcement at the documented function.**
  Outlining the entry half of `begin()` for the codegen-shape gate carried the
  two contract checks with it, and the pairing half then read `State::begin` as
  documented-not-enforced while every local gate stayed green. The checks stand
  in `begin()`; the outlined half keeps the skip test, the loop counter and the
  window open.
- **The thread sanitizer runtime owns the allocation operators on Clang.** CI
  builds the tsan tree with Clang, whose C++ runtime defines `operator new` and
  `operator delete`, so the percentile binary's counting hooks collided with
  them at link. The local GCC tree links a runtime that defines neither, which
  is why the collision waited for CI. The file answers `SG_TEST_HAS_TSAN` the
  way `harness_execution_context_test.cpp` does, so the target keeps its place
  in the preset and its run still runs there.

