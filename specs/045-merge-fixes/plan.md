# Implementation Plan: Fix the counters and FWQ/FTQ gates

**Branch**: `045-merge-fixes` | **Date**: 2026-11-03 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/045-merge-fixes/spec.md`

## Summary

Master `b1dd7670` carries red CI and coverage gaps that PRs #71, #72
and #73 left. The fix lands as two parts, one commit per topic, plus
the spec-file commits the commit rules require. Part A makes the counters
tests host-free: one internal seam field carries the CPU identity, the
ident cache drops, refusals ride the existing scripted errno knobs,
and the marker ledger falls from 94 to 84. Part B repairs the jitter
probe: the scripted clock pace puts every sample inside its window, the
sine arm computes signed and clamps, a latch-time per-cpu snapshot
removes the tsan race at the mode=both folds, the prose fixture runner
runs one process and self-times against its 10-second bound, and the
spec 043 Phase 10 rows carry their gate results. Every hard gate of
Principle VIII passes at the PR head, with no CI change and no new
coverage exception.

## Technical Context

**Language/Version**: C++23, GCC and Clang on Linux; the pinned Core
Guidelines baseline of Principle I rides `.clang-tidy`.

**Primary Dependencies**: CMake presets (`ci-rocky`, `ci-ubuntu`,
`ci-sanitize`, `ci-tsan`, the coverage preset), lcov with perl, CTest,
Python 3 for the prose gate and its fixtures.

**Storage**: N/A (no data store; the artifacts are source, tests and
the spec directory).

**Testing**: `ctest` over the registered suite; the coverage gate
(100 percent lines, branch shortfall allowance 21); ASan/UBSan and
TSan presets; the five standing tools of SC-005.

**Target Platform**: Linux x86-64. thrawn (AMD Zen 5, 24 cores) is the
primary host and corsairone (Intel Skylake-X) the second-vendor host,
per the acceptance section of the spec.

**Project Type**: library and command-line benchmarking framework.

**Performance Goals**: no regression against the standing baselines
(VII). The only hot-path-adjacent change, the per-call ident read,
sits on cold paths (one call per device inside provider
construction).

**Constraints**: no `.github/`, `tools/dbc/`, `tools/ci/` or `cmake/`
change; no version, schema or SOVERSION change; no new coverage
exception; part A and part B in separate commits, each one topic.

**Execution mode**: TDD where a test can fail first (III); `tasks.md`
records each red observation beside the task that turns it green.

**Scale/Scope**: 15 functional requirements, nine code commits over
two parts plus the spec-file commits, nineteen source, test and
script files, one new internal struct.


## Design

### Logical view

- The seam keeps its shape: `KernelCalls` stays an options struct of
  function references. The ninth field, `cpuidWords`, follows the
  pattern of the other eight: one real function fills it from the
  CPUID probe, every test install fills it with scripted words.
- The ident flow loses its cache: `pmuIdentCurrent` reads the field on
  each call and decodes through the pure `pmuIdentFromWords`. The
  sequence stays: read words, decode, select the directory, merge the
  vendored rows.
- The snapshot hand-over carries the one concurrent design. The
  sequence per probe cpu: the probe thread latches (it stores
  `seriesKeptAtLatch`), it copies the prefix, the kept and dropped
  counts, the calibration figures and the method into its
  `JitterLatchSnapshot`, and it release-stores the publish flag; the
  mid-run fold sites acquire-load the flag and read the snapshot
  only; the run-level and per-benchmark folds read the state behind
  the join of `stopAndJoin`. The hot loops touch neither the flag nor
  the snapshot.
- The scripted clock keeps its hand-off shape: the pace rides one
  named constant, the jump sets the large step, the restore sets the
  paced step, and every expected magnitude stands unchanged.

### Test plan

| Requirement | The gate that proves it |
| --- | --- |
| FR-001, FR-002 | `counters_pmu_provider_test`, `counters_kernel_calls_test`, `counters_linux_pmu_seam_test` |
| FR-003, FR-004 | `counters_pmu_window_test` plus the coverage run at the commit head |
| FR-005 | `git grep -c LCOV_EXCL -- source include` against the ledger |
| FR-006 | `harness_jitter_window_test` |
| FR-007 | the sanitize and tsan builds over `harness_fwq_ftq_test` |
| FR-008 | `harness_jitter_cli_test` under `ci-tsan` |
| FR-009 | `prose_gate_fixtures` under a parallel `ctest -j` |
| FR-010 | the Phase 10 rows of `specs/043-fwq-ftq/tasks.md` |
| FR-011 | the coverage preset with the gate |
| FR-012 | the two `CMakeLists.txt` comment sites |
| FR-013 to FR-015 | the standing gates: the suite under `taskset -c 0,1`, the option-error entry, the marker scan, the time-source gate |

## Constitution Check


*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Check | Verdict |
| --- | --- | --- |
| I Standard-first | Every changed line keeps the pinned baseline; no new construct leaves the standard | Pass |
| II Design By Contract | The new snapshot type and the new fold carry `\pre`/`\post` pairs; no public interface moves | Pass |
| III R-DCUT | This plan and `tasks.md` carry the design and the task decomposition; the fix lane changes behavior, so the full workflow runs | Pass |
| IV Documentation | No main-page section changes: no user-visible feature lands; the comment fixes stand inside `CMakeLists.txt` | Pass |
| V Style | Naming and format gates run at the head; the new identifiers are the seam field `cpuidWords`, the real function that fills it, the type `JitterLatchSnapshot`, the test constant `kPacedStepNs` and nothing else | Pass |
| VI Coverage | 100 percent lines, branch shortfall 21 or less, kept-marker ledger recorded; DBC completeness stands | Pass |
| VII Performance | Cold paths only; the scripted clock change lives in tests | Pass |
| VIII CI gates | All hard gates run at the PR head: build, ctest, sanitize, tsan, static analysis, name check, format, spell, prose, time-source, page drift, coverage | Pass |
| IX SDD | The four artifacts stand in `specs/045-merge-fixes/` | Pass |
| X Anti-slop | Surgical scope: the file list of the spec binds; no speculative machinery; one decision per requirement in the research annexes | Pass |
| XI Discourse | Every artifact this lane writes follows XI; the research annexes carry zero gate findings | Pass |

Re-check after Phase 1 design: the design adds one internal struct and
one seam field, both inside `sg::` internal namespaces; no gate
verdict moves.

## Project Structure

### Documentation (this feature)

```text
specs/045-merge-fixes/
├── spec.md                # The feature specification (clarified, 3 rounds)
├── plan.md                # This file
├── research.md            # Phase 0 index (routes into the annexes)
├── research-part-a.md     # Phase 0 annex: counters lane
├── research-part-b.md     # Phase 0 annex: jitter lane
├── data-model.md          # Phase 1 output
├── quickstart.md          # Phase 1 output
├── checklists/requirements.md
├── results/               # Acceptance counts per host (implement phase)
└── tasks.md               # Phase 2 output (/speckit.tasks)
```

No `contracts/` directory: the spec records no interface change
(decision D-8), and the new types live in internal namespaces.

### Source Code (repository root)

```text
# Part A: counters
source/counters/linux_pmu/
├── kernel_calls.hpp       # The seam: one new field, real fill from CPUID
├── kernel_calls.cpp       # The real set gains the ninth function
├── group_io.cpp           # Marker repairs at 306, 432, 288, 402, 426
├── provider.cpp           # Marker removal at 157-163; errno matrix drive
└── table_parse.cpp        # pmuIdentCurrent reads the field per call
include/speedgun-ng/counters_provider.hpp   # Marker removal at 111
test/source/
├── counters_pmu_provider_test.cpp   # Scripted identities, errno matrix
├── counters_kernel_calls_test.cpp   # Field round trip, kept real call
└── counters_linux_pmu_seam_test.cpp # Identity installs at the helpers

# Part B: jitter probe
source/harness/
├── jitter_probe.cpp       # Latch-time snapshot build and publish
└── detail/jitter_probe.hpp # The snapshot type and the snapshot fold
source/harness/cli.cpp     # The mid-run folds read the snapshot
test/source/
├── harness_jitter_window_test.cpp  # Paced scripted clock
├── harness_fwq_ftq_test.cpp        # Signed sine-arm delta
└── harness_jitter_cli_test.cpp     # Join-ruled reads the tsan job names
test/prose-gate-fixture/run_prose_gate_fixtures.py  # One process, self-timed
test/CMakeLists.txt        # Comment fix rides with the entry (comments only)
CMakeLists.txt             # Two comment sites credit PR #70
specs/043-fwq-ftq/tasks.md # Phase 10 ticks and open reasons
```

**Structure Decision**: the fix touches the two subsystems the spec
binds, counters and harness, and adds no new module, target or
directory. The one new type stands beside the probe state it
snapshots, in the same detail header.

## Complexity Tracking

No violation to justify: the design adds one internal field and one
internal snapshot type, keeps every public layout still, and moves no
gate configuration.

## Phase 13 plan: Repair the coverage-data fault (2026-11-07)

**Base**: `origin/master` at `615c0964547e4e9768b8e2f2d79991033a0eb5bd`.
**Branch**: `fix/coverage-data-repair`. **Spec**: Phase 13 of [spec.md](./spec.md).
**Host of record**: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
threads, all 48 cpus in the affinity mask, kernel `7.3.0-5-cachyos`,
GCC and gcov `16.2.1 20260810`, clang `23.1.3`, lcov and genhtml
`2.6-0`, cmake and ctest `4.4.4`, ccache `4.14.1`.

### Summary of the phase

The coverage route reads one trace five times. Four of those reads carry
`--ignore-errors inconsistent` (capture `cmake/coverage.cmake:69`,
extract `:114`, list `:155`, summary `tools/dbc/coverage_gate.sh:32`)
because the trace holds a record
lcov refuses: a function gcov marks hit with no hit line inside it.
The record comes from a closure written inside an `SG_*` contract
predicate. gcov records such a closure at the macro invocation line,
and the standing `--omit-lines "SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?"`
pattern removes that line's data, so the function record survives with
no contained hit line. The repair moves each closure body into a named
semantic helper declared where a test can reach it, and then removes
the class from the three reads.

Measured at `615c0964` on thrawn, fresh instrumented tree, whole suite,
the host lcovrc out of reach:

| Reading | Figure |
| --- | --- |
| Capture with the class tolerated | exit 0, seven `(inconsistent)` end-line records |
| Capture without the class | exit 1, first end-line record, no trace written |
| Extract without the class | exit 0 (lcov 2.6 reads the raw trace, which stands consistent; the base comment names the stop under lcov 2.3) |
| `--list` without the class | exit 1, `hunt_topdown.cpp:155: function ... is hit but no contained lines are hit` |
| Summary without the class | exit 1, the same record |
| Suite in `build/coverage` | `99% tests passed, 1 tests failed out of 185`, `harness_jitter_cli_test ***Timeout 300.11 sec`, `183 - harness_topdown_run_test (Skipped)` |
| Trace with that entry cut | `lines.......: 99.8% (18595 of 18628)`, 33 cold lines, 32 of them in `jitter_probe.cpp`; 55 cold branch rows |
| Trace with that entry run to its end | `lines.......: 100.0% (18627 of 18627)`, `branches....: 99.7% (9223 of 9249)`, 26 cold branch rows |

Two causes stand, and only one is in scope. The contract-closure cause
stands in three predicates at `source/harness/runner.cpp:708`,
`source/harness/runner.cpp:821` and
`source/harness/hunt_topdown.cpp:155`, and those predicates hold six
executed closures: `runner.cpp:711`, `:823`, `:834`, `:841`, `:845` and
`hunt_topdown.cpp:159`. The capture's cause is the
end-line record of the function `SG_BENCHMARK_F` generates, recorded by
`0eed39c2` for specs/016; its repair reaches the public macro family,
so the phase reports it and leaves the capture list alone.

### Design

Logical view, the mechanism end to end:

1. `SG_REQUIRE(pred, msg)` expands to a `do { ... if (!(pred)) ... } while`
   block. A closure written in `pred` is a function of its own.
2. gcov records that closure with its start line at the macro
   invocation line and no line records of its own: the probe trace
   holds `FNL:1,10,10` and `FNA:1,2,...` with hit count 2 and no `DA`
   record for the closure's body lines.
3. The extract step's `--omit-lines` pattern matches the macro line
   and drops its `DA` record. The function record stays.
4. The written trace then holds a hit function with no hit contained
   line. `lcov --list`, `lcov --summary` and `genhtml` each run
   `lcovutil::_checkConsistency`, refuse the trace, and exit 1.
5. The repository answered that refusal by tolerating the class at
   each read. The phase removes the refusal instead: no closure stands
   inside a contract predicate, so no such record is written.

The repair shape, at each of the three sites:

- The predicate keeps its conjunction, its evaluation order and its
  message. The closure body moves into a named function that states
  what the predicate asserts.
- The helper stands declared in the internal header that declares the
  enclosing function, so a test calls it directly:
  `source/harness/detail/internal.hpp` for the two `runner.cpp` sites,
  `source/harness/detail/hunt_topdown.hpp` for the `hunt_topdown.cpp`
  site. `cmake/dbc-gate.cmake:28` reads `include/speedgun-ng` for the
  doc gate, so an internal header gains no doc tag.
- The repository already carries this shape at `hunt_plan.cpp:128`,
  `json_report.cpp:1060` and `pass_plan.cpp:90`. Those three sit in
  anonymous namespaces, so their false arms stand past a test's reach.
  The phase's helpers stand reachable, and a test takes each arm.
- The contract violation record keeps its kind, message, file and
  line. Its predicate text names the call, and the record quotes the
  text before and after.

Physical view, the files the phase touches:

| File | Change |
| --- | --- |
| `cmake/coverage.cmake` | the extract step and the `--list` step lose `--ignore-errors inconsistent`; the two rationale comments state the repaired cause and the capture's standing cause |
| `tools/dbc/coverage_gate.sh` | the `--summary` read loses the class; its comment states the reading that keeps the check live |
| `source/harness/runner.cpp` | the five closures of the two `stripTopdownColumns` predicates move into four named helpers, where the nested context-row closure stands covered by the context helper |
| `source/harness/detail/internal.hpp` | those helper declarations with their contracts |
| `source/harness/hunt_topdown.cpp` | one closure moves into a named helper |
| `source/harness/detail/hunt_topdown.hpp` | that helper declaration with its contract |
| `test/source/harness_topdown_report_test.cpp` | the arms of the four `stripTopdownColumns` helpers |
| `test/source/harness_hunt_report_test.cpp` | the arms of the `hunt_topdown.cpp` helper |
| `source/harness/compare.cpp`, `latency_counters.cpp`, `jitter_probe.cpp`, `noise_tracer.cpp` | no edit; the cold rows these files hold stand covered by new input tests |
| `test/source/*` for the cold rows | the legitimate inputs that close the rows the ledger lists |
| `specs/045-merge-fixes/` | this phase's spec, plan, tasks, checklist, `quickstart.md` and `results/coverage-data-repair.md` |

No public header, no option, no schema, no version and no SOVERSION
moves. No marker, omit pattern, remove pattern, filter or allowance
moves.

### Test plan

| Requirement | The gate that proves it |
| --- | --- |
| FR-016 | the reproduction log: capture exit 1 without the class, `--list` and `--summary` exit 1 naming the contract record, the tool versions and the commands |
| FR-017 | `grep -rn 'ignore-errors' cmake tools` answers no `inconsistent`; the coverage target completes with the check live |
| FR-018 | the helper arms in `harness_topdown_report_test` and `harness_hunt_report_test`; the run and report tests that reach the two `stripTopdownColumns` contracts; the analyzer build |
| FR-019 | the clean coverage lane: configure, build, whole suite, capture, extract, list, summary, gate exit 0 |
| FR-020 | the cold-row ledger at the head against the ledger at the base; the new input tests |
| FR-021 | `git grep -c LCOV_EXCL -- source include` at 84 or fewer, and the two `switch (slot.source)` rows at `group_io.cpp:306` and `:426` |
| FR-022 | the open rows in the ledger with file, line, arms and evidence |
| SC-011 | full `ctest`, `taskset -c 0,1 ctest`, `ci-sanitize`, `ci-tsan`, each with its four counts |
| SC-012 | `ci-rocky` release developer build, format, spell, prose range and tree, example drift, time-source, DBC doc and pair gates, `ci-ubuntu` analyzer build, downstream consumer build |

### Constitution Check

| Principle | Check | Verdict |
| --- | --- | --- |
| I Standard-first | No new construct leaves C++23; the helpers are ordinary functions | Pass |
| II Design By Contract | Each helper carries `\pre`, `\post` and `\invariant` in its internal declaration, and the calling predicate enforces the same answer; the helper body holds no check of its own, because the coverage tree builds with `enforce` and a helper that aborted could not be called with a false predicate by a test. The doc gate reads `include/speedgun-ng` alone and exempts a `detail` scope, and the pair gate carries no row for these declarations, so the contract text stands checked by review | Pass |
| III R-DCUT | This section is the design; `tasks.md` Phase 13 carries the decomposition with its covering tests | Pass |
| IV Documentation | No main-page section changes: no user-visible behavior lands; the comment rewrites sit in the two coverage commands | Pass |
| V Style | The new names follow N-2 and N-5; the format gate runs at the head | Pass |
| VI Coverage | 100 percent lines and a shortfall of 21 or fewer at the head, with the ledger as the record; no new exception | Pass |
| VII Performance | The helpers sit on contract paths that a run already walks. They hold external linkage in `sg::detail`, unlike the three contract-only helpers of the anonymous namespace this phase names, so every configuration emits their bodies into the library: five functions of a few instructions each, called on a contract boundary and never on a measured inner loop. The release build and the analyzer build stand the check | Pass |
| VIII CI gates | Every hard gate runs at the head; the coverage target gains no tolerated class at the three reads | Pass |
| IX SDD | Spec, plan, tasks and results stand in `specs/045-merge-fixes/` | Pass |
| X Anti-slop | Six source files, four command lines, the tests that cover them; the capture list stands untouched and reported | Pass |
| XI Discourse | This phase's prose follows XI; the prose gate runs over the range | Pass |

### Risks and the one blocked item

- The capture list keeps `inconsistent`. A capture without it ends on
  the end-line record of a function `SG_BENCHMARK_F` generates, in
  `test/` and `example/`, and the repair reaches the public macro
  family. The record states the three routes an owner can take: keep
  the class for that shape, narrow the capture scope, or change the
  macro family. This stands as O-4.
- The instrumented lane stops `harness_jitter_cli_test` at the 300 s
  quick ceiling on thrawn; the entry needs 392 s there. The lane runs
  the entry to its end outside `ctest` and records both readings. No
  ceiling moves.
- A warm ccache leaves a stale `.gcda` for an instrumented
  translation unit. The lane configures with both compiler launchers
  empty, as `results/acceptance-thrawn.md` and `results/close-rows.md`
  already record.
- The repair moves branch records back into the denominator. A closure
  inside a contract macro contributes no readable branch row, because
  gcov 16 collapses the macro statement onto the invocation line and the
  omission removes it. The moved bodies hold six `||` operators, twelve
  arms, and the base trace leaves seven of them dark: three in the
  `ResultRow` closure at `runner.cpp:821`, one in the `RowRaw` closure,
  two in the nested context-row closure, and one in the
  `hunt_topdown.cpp:155` closure. COV-003 drives every arm of every new
  helper from a direct test, so the shortfall moves only through the rows
  the phase closes.


## Phase 14 plan: Prove the barrier wait entry and repair the FWQ sample (2026-11-08)

**Base**: `origin/master` at `2d4d8c57975a86db6c77536f7090aba2bdaf6417`.
**Branch**: `fix/barrier-entry-fwq`. **Spec**: Phase 14 of [spec.md](./spec.md).
**Host of record**: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
threads, all 48 cpus in the affinity mask, kernel `7.3.0-5-cachyos`,
GCC and gcov `16.2.1 20260810`, clang `23.1.3`, lcov and genhtml
`2.6-0`, cmake and ctest `4.4.4`.

### Summary of the phase

Two rows stand open.

The barrier fixture proves a flat voluntary-switch count across a SPIN
or YIELD round. The only edge it holds before the observer arrives is
`observer.entered`, stored before `barrier.arriveAndWait()`
(`test/source/harness_context_barrier_test.cpp:participantDrive`). A run
exists where the observer, which runs no context, takes the BLOCK arm
first, the participant arrives second, fills the round, and returns
through the release branch without entering any wait arm
(`source/harness/execution_context.cpp:859-871`). Every assertion of
`spinWait` and `yieldWait` passes on that run. The record of the last
lane measured the vacuity: 12 of 20 rounds took no wait arm at all
before the bracket moved into the participant, and 20 of 20 after
(`results/coverage-data-repair.md:621-623`). The rate rose; the proof
did not arrive.

The FWQ sample spends its wall in the spectrum transform. The example
selects three probe cpus, the series ring holds `kDefaultGapCount =
65536` samples (`include/speedgun-ng/benchmark.hpp:2464`), and the
transform cap stands at the same 65536
(`source/harness/detail/spectrum.hpp:26`), so a window that fills the
ring runs `dftPeriods` at the cap: `N x N/2` is 2.15e9 inner steps
(`source/harness/spectrum.cpp:147-167`). The pair
`seriesStatsOf` plus `addSpectrumTo` runs twice per probe cpu, once for
the report record (`source/harness/cli.cpp:582-583`) and once for the
console fold (`source/harness/report.cpp:908-909`), so the example pays
six cap transforms. Measured on this host: 20.5 s standalone in the
release tree, three runs from 20.55 s to 21.58 s, while the single
`ctest` reading of the clarification stands at 19.18 s; 24.29 s to
25.52 s inside a full `-j 24` suite; 50.31 s standalone in the
instrumented tree, and 55.92 s for that entry inside the full
instrumented suite against the 58.59 s the same tree's cost entry
records. The
ring sweep separates the mechanism from the alternatives: with
everything else fixed, `gaps=1024` answers 155 ms, `gaps=8192` answers
454 ms, `gaps=32768` answers 4.89 s and the default answers 17.81 s.

### Design

The barrier half, in order:

1. `ContextBarrier::arriveAndWait` counts the arrival, and the branch
   at `:859` answers whether this arrival fills the round. A call that
   reaches the mode chain at `:886` stands proved to be a non-last
   arrival, so the barrier state requires a wait.
2. The seam is one installed observer, the shape D-13 and the
   `installTest*` family carry: `sg::detail::installTestWaitArmObserver`
   declared in `source/harness/detail/internal.hpp`, defined in
   `source/harness/execution_context.cpp` over a function-local static
   slot beside the barrier, the slot shape `placementSlot()` gives at
   `:128-132`. The observer is a `noexcept` function pointer over
   `sg::WaitMode`, and an empty optional clears it.
3. One call stands at the head of each arm, inside the arm's block, and
   names that arm: `SPIN` in the pause arm, `YIELD` in the yield arm,
   `BLOCK` in the standard-wait arm. The argument names the arm the path
   enters, and the mode chain decides which arm runs, so a chain that
   hands a mode the wrong arm answers with the wrong word.
4. The test's callback stores the word into a test atomic with release
   order. The observer's `watch` on that atomic is the acquire load. The
   observer arrives only after that load, which puts the participant's
   `fetch_add` ahead of the observer's in the modification order of
   `m_arrivalCount`: the participant is a waiter, and the observer is
   the filling arrival. That is the happens-before edge the fixture
   lacks today.
5. The fixture gates on `armEntered || departed`. The second half turns
   a missing record into a named failure, so a removed arm answers with
   a line, and the barrier's own postcondition at `:897-899` ends the
   child by `SIGABRT` in the same run.
6. Cost: one static load and one branch per waiting arrival, nothing
   per spin iteration. `:873-874` fixes the cost model of the arm, and
   `specs/020-execution-contexts-core/plan.md:97` names the barrier a
   P0 path, so a per-iteration hook would be a regression and a
   per-arrival hook stands below the existing floor.
7. Coverage: three call sites, one per arm, each reached by one
   scenario; the installed-or-clear branch reached both ways by the
   fixture. Net new cold rows: zero, against a ledger at 20 of an
   allowance of 21.
8. No public header, no public layout, no exported symbol and no
   SOVERSION moves. `internal.hpp` stands uninstalled and the harness
   target is visibility-hidden (`CMakeLists.txt:130`), the reading the
   last lane took for the two entry functions it added.

The FWQ half, in order:

1. The transform runs once per probe cpu, not twice. `addSpectrumTo`
   (`source/harness/jitter_probe.cpp`) keeps the answer it computed in
   the probe state that owns the series, and the second fold reads that
   answer. The two call sites in `source/harness/cli.cpp` and
   `source/harness/report.cpp` stand untouched, so the console block,
   the report record and every figure keep their bytes.
2. The state's slot is the record of the transform of that state's
   series. The series stands written only inside the window, ahead of
   the latch line, and both folds run after the probe stops, so the
   answer cannot go stale while a fold reads it. Coverage: the
   reuse-or-compute branch of `addSpectrumTo` stands taken both ways by
   the SC-028 case, so this half adds no cold row either, and both lanes
   share one coverage run against a one-row headroom.
3. The unit proof drives a scripted window, folds the state twice, and
   reads the slot between the two folds. The proof names the second
   fold's reading of the stored answer, so a build that recomputes
   fails the scenario. The slot is a `mutable` member of
   `ProbeCpuState`, because both fold sites hold a `const` pointer to
   that state (`source/harness/cli.cpp:583`,
   `source/harness/report.cpp:909`), so those two files stay out of the
   change.
4. The example's own bound, `gapCount` at 4096, measures 17.6x on the
   instrumented binary (50.31 s to 2.86 s) and 92x in release (19.5 s
   to 212 ms). It needs one line in `example/docs/fwq_ftq.cpp` and the
   same line in the block that quotes that region at `README.md:766`,
   because `tools/docs/example_drift.py` compares the block and the
   region line for line. `README.md` stands outside this phase's scope,
   so the bound stands reported as O-7 and the phase takes no example
   edit.
5. The failure itself stands unreproduced on this host: three full
   release suites, one full instrumented suite, 20 pinned runs of the
   entry at 13.36 s to 18.50 s (`~/scratch/bfw/pinned-fwq-run-1.log` to
   `pinned-fwq-run-20.log`, section 7 of
   `~/scratch/bfw/fwq-research.md`), and 8, 24 and 48-way process
   contention all pass. The row stays open under SC-022 with the next
   experiment named.

Work added during implementation, recorded so the plan and the code answer
the same shape: the blocking arm of the chain now re-checks its exit
condition and waits again inside the loop. The two spinning arms already
hold their exit condition in their loop, and the blocking arm waited once
and trusted the wake. A contract check of the barrier caught runs in which
that wake answered nothing, three full suite runs and one protocol of 480
copies at 48 in flight beside a running suite. D-22 and BFW-008 carry that
row, and section 5 of the record carries the counts.

### Test plan

| Case | File | What it asserts |
| --- | --- | --- |
| `spinWait`, `yieldWait`, `insideOutside` entry | `test/source/harness_context_barrier_test.cpp` | one record naming the arm the policy names, published before the observer arrives (SC-019) |
| `round` filling arrival | same file | three worker records, and no record for the arrival that fills the round (SC-019) |
| the four substitutions | the same scenarios, plus the record of the substitution runs | each fails its scenario in all 240 runs in the release tree (SC-020) |
| one transform per probe cpu | `test/source/harness_fwq_ftq_test.cpp` | the second fold reads the stored answer over a scripted window (SC-028) |
| the real example | `report_shape_fwq_ftq_sample` in each lane | the entry exits 0 and the block matches by shape (SC-021) |
| the seam stays internal | the SC-027 symbol reading | no new `sg::detail` name in `.dynsym`, and the standing count of 250 exports (SC-027) |

The substitution runs stand recorded with the patch, the command, the
scenario line and the count: the four rows FR-029 names, 240 runs each
in the release tree, and 240 clean runs of the unmodified head in the
release tree and in the thread-sanitizer tree.

### Constitution Check

- I: the seam is P2 (an internal observation with a written
  justification at the site); the arms keep their P0 shape.
- II: the seam carries no contract of its own, the D-13 condition, so a
  test can install it without standing a violation.
- VI: each new arm and branch stands covered by a named scenario, and
  the bug fix ships with the reproducing case.
- VII: one load and one branch per waiting arrival; the cost model of
  the arm at `:873-874` stands unchanged.
- VIII: no gate, filter, allowance, marker or timeout moves.
- X.2: the seam serves the one proof this phase states; no general
  event bus, no per-iteration hook, no new public surface.
- X.3: the touched files stand inside the phase's scope list; the
  example and `README.md` stand untouched and reported.
- XI: the record, the comments and the commit follow ASD-STE100.

### Risks and the open items

- The entry gate turns a scheduling-dependent pass into a hard
  dependency on the record. A build whose arm never runs hangs nowhere:
  the `|| departed` half opens the gate and the scenario names the
  missing record.
- The thread-sanitizer tree slows every instrumented access. The record
  rides one release store and one acquire load, and the stress matrix
  runs 240 runs at 8 in flight in that tree.
- The FWQ row can end unresolved. The cost mechanism stands proved with
  the ring sweep; the failure needs a slower lane than this host
  provides, and the corsairone reading of 110.21 s stands as that
  host's limit.
- O-6: an arm-body swap needs a disassembly gate, and that needs a new
  entry in `test/CMakeLists.txt`.
- O-7: the example's ring bound needs one line in `README.md`.
- O-8: the probe's time base reads raw TSC ticks while its fields are
  named ns (`source/counters/clock_provider.cpp:286`), so every probe
  duration and threshold on this host runs 4.19x shorter than it names.
  A fix there lengthens the example, so it stands apart from this
  phase.
