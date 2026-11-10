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

## Phase 15 plan: Land the approved FWQ example widen and measure the gain (2026-11-09)

**Base**: `origin/master` at `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the
PR #100 merge, which carries the single-fold spectrum cache.
**Branch**: `fix/fwq-example-n13e`, cut from the latest `origin/master`.
**Spec**: Phase 15 of [spec.md](./spec.md), rows BFW-009 to BFW-012.
**Host of record**: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
threads, all 48 cpus in the affinity mask, kernel `7.3.0-5-cachyos`, GCC and
gcov `16.2.1 20260810`, clang `23.1.3`, clang-format `18.1.8`, lcov and
genhtml `2.6-0`, cmake and ctest `4.4.4`.

Line numbers stand taken at the branch base, before the widen lands, the way
the earlier phases of this file take theirs.

### Summary of the phase

One row stands open. O-7 of phase 14 names the cause of the FWQ sample's
cost and the approved remedy: the example leaves the probe ring at
`kDefaultGapCount = 65536` (`include/speedgun-ng/benchmark.hpp:2464`), the
window fills that ring, `resampleUnitTimes` caps the transform at
`min(sum(unit times)/stepNs, cap)` (`source/harness/spectrum.cpp:62`) with
`kSpectrumCap = 65536` (`source/harness/detail/spectrum.hpp:26`), and
`dftPeriods` then runs `N x N/2` inner steps (`source/harness/spectrum.cpp:147-167`).
The ring capacity equals the transform cap, so the example pays the cap on
every run. The owner approved the example line and the `README.md` quote
together; PR #100 left that widen unlanded because `README.md` stood outside
that phase's scope.

This phase lands the widen, keeps the page pair in step, and measures the
gain on this host at the final squashed head. It changes two tracked lines
and adds one record file. It moves no version, no schema, no `SOVERSION`, no
public header, no gate, no filter, no allowance and no timeout. The barrier
contract and the probe contract stand unchanged apart from the one quoted
example line, and this phase reopens no proof of the wait-arm set that stands
landed at the base.

### Design

The example half, in order:

1. `example/docs/fwq_ftq.cpp` gains one line in the `fwq-ftq` region. The
   region markers stand at `:33` and `:46` and its body runs `:34-44`, eleven
   lines. The new line stands after the `.mode` line at `:37` and before
   `.precheckMs`:

   ```cpp
                         .gapCount = 4096,
   ```

   In the file the line carries 22 leading spaces, the alignment
   `AlignAfterOpenBracket: Align` gives after the open brace of the
   braced-init-list, and the fence above adds the list's three. The file sets
   `ColumnLimit: 80`, and the longest line of the call stands at 60 columns.
   A candidate carrying that line answers unchanged from
   `clang-format --style=file` with clang-format `18.1.8`, so the format gate
   reads clean on the first pass. The region body goes to twelve lines and
   the file to 47.
2. The designator stands in declaration order. `JitterProbeOptions` declares
   `mode` at `include/speedgun-ng/benchmark.hpp:2558`, `gapCount` at `:2559`
   and `precheckMs` at `:2560`, and a designated initializer list follows
   declaration order, so `.gapCount` stands between `.mode` and
   `.precheckMs`. The value 4096 stands inside `kMinGapCount` and
   `kMaxGapCount` at `:2465-2466`, so the require at
   `source/harness/cli.cpp:413-415` holds.
3. The figure 4096 is the one the owner's ruling names (D-24, clarification
   round 1). Phase 14 item 4 names it the example's own bound, and the 17.6
   and 92 estimates stand measured there. The 1024 sweep row stands a lower
   sweep point.
4. `README.md` carries the same bytes in the block that quotes the region.
   The marker stands at `:765`, the fence at `:766`, the block body at
   `:767-777` and the closing fence at `:778`. The new line stands after the
   `.mode` line at `:770`. `example_drift.py`'s `compare` (`:423-446`)
   requires the block body and the region lines to be equal, so a one-sided
   edit fails with `block 'fwq-ftq' at line 766 differs from its region`, and
   both edits stand in one commit.
5. The page keeps its budgets. The page stands at 1002 lines and goes to
   1003 against `PAGE_LINE_CAP = 1004` (`tools/docs/example_drift.py:256`).
   The section `Fixed work and time quanta` spans `:763-780`, eighteen lines,
   and goes to nineteen against `SECTION_LINE_CAP = 25` (`:257`). No other
   page line moves, and the feature-section rows of `--sections` keep their
   readings.
6. `docs/pages/harness.md` and `example/docs/fwq_ftq.expected` stand
   untouched. That page's marker at `:2039` quotes the recorded run, which is
   a sample block, and no page quotes the `fwq-ftq` region as code.

The shape half, in order:

1. The recorded block keeps its bytes (D-26). `example/docs/fwq_ftq.expected`
   carries 156 body lines, quoted at `docs/pages/harness.md:2039`.
   `report_shape.py`'s `mask` folds a digit run to `#` and a whitespace run
   to one space, so `sampleCount`, `spectrumPoints`, `droppedSamples` and the
   period figures move as values while every field name, line class and order
   keeps its bytes. The fold keeps the 30 series lines and the five period
   lines, because those lines come from the field names of the series block
   and the ring bound answers figures.
2. The warning-line count stands the one part of the shape a bound can move.
   The two `warning: cpu <n> noisePercent <x> maxValue <y>` lines of the
   recorded block are classified lines (`classify` at `:185-201` answers
   `noise warning line`) and stand outside `HOST_CONDITION` (`:129-133`), so
   their count rides the compared shape. They answer for the planned cpus,
   and the example's bound leaves `noisePercent = 1` far behind on both of
   them: 8.70 and 16.49 in the release tree at the bound, 88.45 and 72.70 in
   the instrumented tree. A final-head run whose count differs from the two
   recorded lines answers with `sample output block holds 156 lines, the run
   printed <n>` and a per-line finding (`compare`, `:301-322`), stands a
   shape finding under D-26, stops the work, and stands reported with both
   counts.
3. The two grants stand unchanged (O-5): the 110 s subprocess grant at
   `tools/docs/report_shape.py:368` and the 120 s `ctest` ceiling of
   `report_shape_fwq_ftq_sample` at `test/CMakeLists.txt:2197`. The widen
   shortens the run, so neither grant carries a new dependency.
4. `example_run`, `snippet_compile`, the `docs_fwq_ftq` target and the
   `example/docs/fwq_ftq.expected` fold keep their standing readings, because
   the calibration group, the method word and the series lines keep their
   shape.

The measure half, in order:

1. The host reads stand first: `sched_getaffinity` and the online cpu list,
   `python3 -c 'import os; print(len(os.sched_getaffinity(0)))'` and
   `cat /sys/devices/system/cpu/online`. Both stand in the record beside the
   timing table.
2. The baseline side stands built from the branch base itself in a named
   second tree, `build-base`: `cmake --preset=ci-rocky -B build-base`,
   `cmake --build build-base -j 48`, then the five runs against
   `--test-dir build-base`. The batch runs first and stands fixed; interim
   rows may ride the last pre-widen interim commit under the first diff
   proof below, and every interim SHA stands in the record.
3. Two diff proofs stand in the record. The first compares the branch base
   with each pre-widen interim commit:
   `git diff --name-only a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a <pre-widen-sha> -- source include example test cmake tools docs external CMakeLists.txt CMakePresets.json`
   answers empty, so an interim baseline binary carries the branch base's
   code and the phase 14 "This tree" column stands the branch-base shape.
   The second compares the pre-widen commit with the measured head over
   `source include test cmake tools docs external` and the CMake files and
   answers empty save the spec 036 lane's owned files, named in the record:
   `source/harness/noise_tracer.cpp`, `source/harness/detail/noise_tracer.hpp`,
   `test/source/harness_noise_test.cpp` and
   `test/source/harness_noise_report_test.cpp`. Those files stand off the
   sample's path: the example names no tracer engine, so `noiseTracerFigures`
   answers no call in the timed run. And `git diff --unified=0 <pre-widen-sha> <head> -- example/ README.md`
   answers exactly the two added lines, so the two sides of the ratio differ
   in the example region, the page block and the other lane's tracer read
   alone.
4. The protocol is the one the spec predeclares, run once per side, unpinned
   on the full affinity mask, one `ctest` invocation per run:

   ```sh
   seq 1 5 | xargs -I{} ctest --test-dir build-base -R '^report_shape_fwq_ftq_sample$'
   seq 1 5 | xargs -I{} ctest --test-dir build -R '^report_shape_fwq_ftq_sample$'
   ```

   The first batch runs against the fixed baseline tree. The second batch
   runs only after lane 1 has configured and built `build` fresh at the
   measured head. The record gives all ten wall times, the median of each
   side, and the ratio of the medians as a measured figure at the measured
   head. Only the final-head side re-runs at a new squashed head, and an
   interim row names its pre-squash SHA.
5. A preliminary baseline set stands taken already: 11.35 s, 12.42 s,
   12.24 s, 11.95 s, 11.84 s, median 11.95 s, at
   `$HOME/scratch/fwq-example-n13e/baseline-timings.md`. Its commit
   `9f1daf31` stands unreachable from the branch after the amend, so the
   record re-runs the five baseline runs against `build-base` at the branch
   base and carries the earlier set as a preliminary row.
6. Each earlier figure stands cited with its tree, its bound, its transform
   count and its invocation form, and none stands compared as an equal:
   - 17.6x: the instrumented tree at `2d4d8c57`, the standalone binary
     `build/coverage/example/docs_fwq_ftq` with the CLI key `gaps=4096`, six
     transforms, 50.31 s to 2.86 s, file `cov-gaps4096.txt`. Phase 14 wrote
     "measured at 17.6x"; this record re-labels that figure as an estimate at
     the 4096 bound on the instrumented binary.
   - 92x: the release tree at `2d4d8c57`, the same standalone CLI form with
     `precheck=500, gaps=4096`, six transforms, 19.5 s to 212 ms.
   - The ring sweep: the same tree and form with a non-serial pre-check,
     `gaps=1024` 155 ms, `gaps=2048` 159 ms, `gaps=4096` 212 ms,
     `gaps=8192` 454 ms, `gaps=16384` 2.76 s, `gaps=32768` 4.89 s, the
     default 17.81 s.
   - The branch-base alone-entry readings: 11.17 s, 11.37 s and 11.40 s
     (`results/barrier-fwq-closeout.md:174`), the three-transform tree.
   No tracked line calls an ~18 figure a fact, and no threshold stands
   invented against the measured ratio.
7. The full-suite wall of the same entry stands recorded in each required
   suite lane. Three lanes stand compared with the phase 14 "This tree"
   column, the branch-base shape at the same lane shape: 15.84 s release at
   `-j 48`, 8.93 s pinned at `-j 2`, 13.14 s four-cpu at `-j 4`. The
   sanitizer, thread-sanitizer and instrumented lanes carry the entry's wall
   as a reading with no comparison, because phase 14 names no base figure
   there. SC-031 stands a no-regression check beside the alone-entry gain
   measure.

The lane set, with the rows this phase amends:

| Lane | Command |
| --- | --- |
| release suite | `cmake --preset=ci-rocky`, `cmake --build build -j 48`, `ctest --test-dir build --output-on-failure -j 48` |
| two-cpu pinned | `taskset -c 0,1 ctest --test-dir build --output-on-failure -j 2`; a host that leaves cpus 0 and 1 outside the allowed set records the refusal, and a run on another allowed pair stands extra evidence |
| four-cpu | `taskset -c 0,1,2,3 ctest --test-dir build --output-on-failure -j 4` |
| sanitizer | `cmake --preset=ci-sanitize`, `cmake --build build/sanitize`, `ctest --test-dir build/sanitize --output-on-failure -j 48` |
| thread sanitizer | `cmake --preset=ci-tsan`, `cmake --build build/tsan`, `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure -j 48`, the standing value of `.github/workflows/ci.yml:299` |
| instrumented | `cmake --preset=ci-coverage`, `cmake --build build/coverage`, `ctest --test-dir build/coverage --output-on-failure -j 48`, then `cmake --build build/coverage --target coverage` |
| coverage gate | the same run answers its numerator, denominator and branch shortfall; the shortfall stands at or below 21 with no new marker, filter, allowance, omit or ignore-error class, and `git grep -c LCOV_EXCL -- source include` stands at or below 84 |
| cheap gates | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`, `python3 tools/prose/prose_gate.py --mode tree`, `python3 tools/prose/prose_gate.py --mode range --base a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, `python3 test/prose-gate-fixture/run_prose_gate_fixtures.py --check both --out $HOME/scratch/fwq-example-n13e/prose-fixtures test/prose-gate-fixture`, `python3 tools/docs/example_drift.py .`, the same with `--names --options --forbidden --sections`, `python3 test/example-drift-fixture/run_example_drift_fixtures.py .`, `python3 test/example-drift-fixture/run_report_shape_fixtures.py .`, `bash test/example_run.sh build`, `bash test/example-run-fixture/run_example_run_fixtures.sh`, `bash test/snippet-compile/run.sh . clang++`, `bash test/time_source_gate.sh .`, `cmake --build build -t dbc-gate`, `ctest --test-dir build -R dbc_gate_fixtures` |
| analyzer build | `cmake --preset=ci-ubuntu -B build-an` and a whole build, so no reconfiguration displaces `build`; the base reading comes from a worktree at the branch base under `$HOME/scratch/fwq-example-n13e/base-audit`, configured `ci-ubuntu` into its own build dir, so the comparison of finding counts on the touched files has both sides |
| exported names | `cmake --preset=ci-linux-audit -B build-shared -D BUILD_SHARED_LIBS=ON`, install into `$HOME/scratch/fwq-example-n13e/prefix-shared`, then `nm -D --defined-only` over the installed objects; the standing counts stand answered, harness 210 and `libspeedgun-ng.so` 250 with no third-party name |
| consumer build | `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$HOME/scratch/fwq-example-n13e/prefix-shared`; the spec's lane 7 carries this build beside the cheap gates, and this table lists it apart |
| shared rows | the N13E-004 acceptance rows the spec 036 phase names, run at the same head in the same lanes |

The analyzer row and the exported-names row stand required here for the
reasons the clarification names: the audit carries the no-symbol-change
claim, and the prose range command names the branch base so the range covers
the whole branch. The analyzer build stands in its own `build-an` directory
because `ci-rocky` and `ci-ubuntu` share `build`.

### Record plan

The record stands at `specs/045-merge-fixes/results/fwq-example-closeout.md`,
and the quickstart gains one link row to it. Its sections:

1. Host, base and head: the tool versions, the affinity and online-cpu
   reads, the base SHA, the branch, and the rule that a tracked file carries
   no SHA of its own commit while each dated interim row names its pre-squash
   SHA.
2. The widen and its page pair (FR-034, SC-029): both diffs, the byte form,
   the `clang-format` reading, and the drift gate in both forms.
3. The shape reading (US-11 scenario 3, SC-031, D-26): the entry's exit and
   wall in the release, pinned and four-cpu lanes beside the phase 14 column,
   the entry's wall in the sanitizer, thread-sanitizer and
   instrumented lanes as readings with no
   comparison, the recorded block's 156 lines against the run's count, and
   the warning-line count on both sides.
4. The timing protocol and the measured gain (FR-035, SC-030): the
   `build-base` baseline at the branch base, the interim SHAs, the two diff
   proofs, the host reads, the ten wall times, the two medians and the
   ratio.
5. The earlier figures with their provenance (FR-035): the 17.6 estimate, the
   92 reading, the sweep rows and the branch-base alone-entry readings, each
   with its tree, bound, transform count and invocation form.
6. The recorded block's provenance (D-26): one dated line naming the ring
   bound and the fact that `example/docs/fwq_ftq.expected` stands recorded
   from the pre-widen example. The block stands not re-recorded.
7. The lanes at the measured head (FR-037, SC-032, SC-033): the lane table
   above with the head, the exit and the counts beside each command, the
   coverage figures, the marker count, the export counts, the analyzer counts
   and the N13E-004 rows.
8. The push and the pull request (SC-033): the push command, the lease rule,
   the `git ls-remote` reading, and the path of the untracked body artifact
   under `$HOME/scratch/fwq-example-n13e/` that names the measured head
   beside every required lane.
9. Rows left open (D-27): O-5 and O-6 as phase 14 wrote them, O-7 closed by
   FR-034 and FR-035, and O-8 deferred with the reading D-27 states.

The task rows BFW-009 to BFW-012 stand written into `tasks.md` by the tasks
phase before implement. No test of this phase uses a sleep, a wall-clock
threshold or a fake cpu.

### Test plan

| Case | File or gate | What it asserts |
| --- | --- | --- |
| the page pair | `python3 tools/docs/example_drift.py .` and the four-flag form | exit 0, and the changed line byte-identical in the region and in the block at `README.md:766` (FR-036, SC-029) |
| the block still builds and runs | the `docs_fwq_ftq` target, `bash test/example_run.sh build`, `bash test/snippet-compile/run.sh . clang++` | the region stands in a translation unit the suite builds, and the program exits 0 (IV.1) |
| the recorded run keeps its shape | `report_shape_fwq_ftq_sample` in the release, pinned and four-cpu lanes | exit 0 in each, the 156 recorded lines against the run's count, the warning pair, and the wall beside the phase 14 column (SC-031, D-26) |
| the gain | the record's timing table | five alone-entry walls per side at the shared cpu set and run count, the medians, and the ratio as a measured figure (FR-035, SC-030) |
| no symbol moves | the exported-names audit | harness 210 and `libspeedgun-ng.so` 250 with no third-party name, the lists answering no difference against the base |
| coverage unchanged | the `ci-coverage` tree and the `coverage` target | 100.0 percent lines, a branch shortfall at or below 21, and at or below 84 markers, with no new exclusion (SC-032) |
| the measured head | `git rev-parse HEAD`, `git status --porcelain`, `git ls-remote origin refs/heads/fix/fwq-example-n13e` | one SHA beside every required lane, a clean tracked tree, and the remote equal to the local head (FR-037, SC-033) |

### Constitution Check

- IV.1: the region and the page block change in one commit, the block stays a
  program the `docs_fwq_ftq` target builds and `example_run` runs, and the
  result sentence at `README.md:779` keeps its claim because the fold keeps
  the calibration group, the 30 series lines and the five periods.
- VI: no test moves. The coverage reading at the measured head answers its
  numerator, denominator, shortfall and marker count, and the phase adds no
  cold row: the denominator covers `include/speedgun-ng/*`, `source/*` and
  `tools/hostcheck/*` (`cmake/coverage.cmake:118-120`), and the change stands
  in an example region.
- VII: the speedup the page implies stands a measured figure at the measured
  head, and each earlier figure stands cited with the tree, the bound, the
  transform count and the form it was taken at.
- VIII: no gate, filter, allowance, marker, timeout or continuous-integration
  change moves. The 110 s grant and the 120 s ceiling stand unchanged.
- X.2: the change is one designator in one call. No option, no helper, no
  new test scaffolding and no new public surface stands added.
- X.3: two tracked lines carry the change, in the example region and in the
  block that quotes it. The probe sources, the barrier files, the recorded
  block and `docs/pages/harness.md` stand untouched.
- XI: the record, the comments and the commit follow ASD-STE100, and the
  prose gate runs over the tree and over the range with `--base` naming the
  branch base.

### Risks and the open items

- The warning-line count. The pair is a classified line, so a final-head run
  where a planned cpu stays under `noisePercent = 1` drops a line and fails
  the fold. Measured readings at the bound keep both lines by a wide margin.
  The record gives both counts, and a difference stands a shape finding under
  D-26 that stops the work and reports the scope it needs.
- The page budgets. The page goes to 1003 lines against a cap of 1004, and
  the section goes to 19 lines against a cap of 25. A second page line in the
  same change would need the drift gate's section rows, so the phase adds no
  other page line.
- Timing noise on a loaded host. The same release binary measured 20.5 s
  under a load average of 10 to 14 and 11.4 s on a quieter machine
  (`$HOME/scratch/bfw/fwq-research.md:52`). The record names the affinity and
  online-cpu reads beside the ten walls, and a side taken under load answers
  nothing under VII and stands re-run.
- The measured head. A tracked change after a measurement produces a new
  squashed head and a fresh run of every required lane, the widened timing
  side included. The baseline side stands taken once in `build-base` from the branch
  base itself, and each interim row stands tied to the base by the first
  diff proof.
- The remote. A failed `--force-with-lease` stops the work and stands
  reported, and the lease stands never refreshed and retried. A remote that
  refuses the push stands the push row blocked with the refusal, and the
  local half, a clean tracked tree and `git rev-parse HEAD`, stands proved
  either way. The `gh` credential of this machine stands absent, so the
  branch, the push proof and the body text stand delivered through one local
  file for the operator.
- O-5, O-6 and O-8 stand as phase 14 wrote them. O-7 closes with FR-034 and
  FR-035.

## Phase 16 plan: Match the probe's printed units to the counter ticks (2026-11-10)

**Base**: `origin/master` at `1f1f9b20cec97198e5cd88778167431eef2c6eb6`,
the PR #101 merge.
**Branch**: `fix/probe-unit-labels`, cut from the latest
`origin/master`.
**Spec**: Phase 16 of [spec.md](./spec.md), rows BFW-013 to BFW-017.
**Host of record**: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
threads, all 48 cpus in the affinity mask, kernel `7.3.0-5-cachyos`,
GCC and gcov `16.2.1 20260810`, clang `23.1.3`, clang-format `18.1.8`,
lcov and genhtml `2.6-0`, cmake and ctest `4.4.4`.

Line numbers stand taken at the branch base, before the fix lands, the
way the earlier phases of this file take theirs.

### Summary of the phase

One row stands open. O-8 of phases 14 and 15 names the defect, and the
owner ruling G-35 orders it closed. The probe's one-leaf plan reads the
catalog's `machine/tsc` leaf (`source/harness/jitter_probe.cpp:262`, the
leaf the availability gate matched at Q-1), whose values stand in raw
counter ticks (`source/counters/clock_provider.cpp:176-185`, `tscTicks`
over `__rdtsc`), while every figure the probe stores and prints carries
a name in nanoseconds: `unitTimeNs`, `targetNs`, `quantumNs`,
`thresholdNs`, `loopPeriodNs`, `calibrationSpanNs`
(`include/speedgun-ng/benchmark.hpp:618-640`; the constants
`kDefaultThresholdNs = 200` at `:2463` and `kTargetUnitTimeNs = 1000`
at `:2481`). On this host the two stand apart by a factor of about
4.19: the tsc answers about 4.19 ticks per true nanosecond, so a
figure named `unitTimeNs` carries about 4.19 times its name, and the
calibration target of 1000 ns lands near 239 true ns, where the probe
contract of specs/043 FR-010 names 1 us.

The defect reaches four behaviors at one root, because one read fills
them all:

1. The printed figures carry ticks under ns names: the `jitter probe:`
   block, the precheck per-cpu blocks and the per-benchmark worst-cpu
   line of `source/harness/report.cpp` (`:1001`, `:816`, `:822`), and
   the JSON document's jitter keys
   (`source/harness/json_report.cpp:381-470`).
2. The gap threshold comparison (`gap >= state->thresholdNs` at
   `source/harness/jitter_probe.cpp:735` and `:825`) compares ticks
   against nanoseconds: the kept-gap set stands about 4.19 times too
   sensitive.
3. The FWQ calibration target stands read in tick space
   (`source/harness/jitter_probe.cpp:308` and `:375`), so the unit
   lands near 239 true ns.
4. The FTQ quantum edges (`edge += state->quantumNs` inside
   `runFtqUnit`, the edges seeded at `:723` and `:816`) and the
   precheck spans (`span` at `:693`, `precheckMs * 1000000`) compare
   ticks against ns figures.

This phase lands one fix at one point: the probe's own read move
gains an exact integer projection from ticks to nanoseconds, at the
tick rate the run measured at setup against the catalog's
`machine/monotonic` leaf, and every figure, comparison and span
downstream keeps the ns reading its name and its contract state. The
fix moves no public header, no schema version, no `SOVERSION`, no
gate, no filter, no allowance and no timeout, and it adds no console
line, so every recorded block and `.expected` fold keeps its line
count and its field names.

### Design

The fix stands at the read boundary, for four reasons the code fixes:

- One point covers every behavior the defect reaches. The calibration
  (`calibrationTrial` at `:335`, `calibrateUnit` at `:368`), the
  measurement pass (`:668-680`), both gap loops (`:725-743`,
  `:822-831`), both series loops (`runFwqUnit` at `:478` and
  `runFtqUnit` at `:506`, the precheck and alongside calls) and the
  hwlat stamp path all read through the one `readStamp` lambda the
  item builds at `:640`. A projection inside that lambda scales every
  consumer at once, and no comparison, no constant and no report site
  moves: `kDefaultThresholdNs` and `kTargetUnitTimeNs` keep their ns
  values and gain true ns readings, the FTQ edges accumulate in ns,
  the precheck span counts ns, and the stored figures
  (`state.unitTimeNs` at `:422` and `:489`, `loopPeriodNs` at `:676`)
  carry the unit their names state before any fold or writer reads
  them.
- The contracts stay whole. specs/027 and specs/043 state the
  threshold, the target, the quantum and the spans in nanoseconds;
  the tick-space core was the deviation. The projection restores the
  contracts, and no requirement row of either spec moves: D-2,
  FR-007, FR-008 and FR-010 stand as written.
- The disclosure shrinks to a sentence. After the fix the printed
  units match the values they carry, which is the row G-35 orders
  closed; the one statement the run still owes is the time base
  itself, and the JSON document's free-text `jitter.disclosure`
  (`source/harness/cli.cpp:539`, the record the writer puts at
  `source/harness/json_report.cpp:1342`) carries it. No console line
  moves, so the recorded blocks, the `.expected` folds and the shape
  masks keep their bytes' structure: the mask folds a digit run to
  one `#`, so every scaled value passes the fold unchanged.
- The scripted tests stay byte-identical. The fake provider's tsc
  leaf scripts stamps the tests treat as nanoseconds today. The fake
  monotonic leaf stands scripted beside it so the measured rate
  answers 1 tick per ns, and the projection of a 1 tick per ns rate
  is the identity: every existing scripted stamp, figure and byte
  keeps its reading, and the new tests script a rate other than one
  to lock the scaling itself.

In order:

1. **The rate source** (BFW-014). The run measures its tick rate once,
   at the holder's start, before the barrier stands arrived and
   outside every window: two one-leaf recorders minted from the same
   catalog the probe's gate read, one over `machine/tsc` and one over
   `machine/monotonic` (`source/counters/clock_provider.cpp:10-16`,
   `:68-85`, the ns leaf `monotonicNs` serves from the vDSO), a
   bounded spin that reads both leaves until the ns delta passes
   1,000,000 or a read-count cap of 100,000 pairs stands reached, no
   sleep and no clock call anywhere, and the rate as
   `deltaTicks * 1000000000 / deltaNs` in one 64-bit form the spin's
   bound keeps in range. The rate stands one figure per run, stored
   beside the session the items read, because the tsc rate stands
   constant across the cpus of one host and the record states the
   reading. The unavailable path stands unchanged: a host whose gate
   refuses the tsc leaf opens no probe and measures no rate.
2. **The projector** (BFW-014). One small type in the probe's detail
   header, `TickProjector`: three fields (`lastTick`, `lastNs`, and a
   carried remainder `rem`) and one method. The method takes the raw
   tick read, computes `delta`, then
   `(delta * 1000000000 + rem) / rateHz` into the running ns value,
   folding the division's remainder back into `rem`, so the
   projection stands drift-free over any run: the carried remainder
   stays below `rateHz`, the per-read product stays in range because
   the deltas between consecutive reads stand far below the 4-second
   bound the form carries, and the require that reads stand monotonic
   (a repeated or backward read stands a violated contract, the fuse
   of II) makes the delta's sign hold. The first read seeds `lastTick`
   and answers 0. The ns values the run carries therefore start at
   the window's base and stand exact to the tick, with no
   accumulated drift and no floating-point step.
3. **The wiring** (BFW-014). `ProbeItem::operator()` builds one
   projector beside the read move, and the `readStamp` lambda at
   `:640` becomes the projection wrapper: the raw read (the
   installed reader's thunk through the stack point sink, or the
   system recorder's column) stands unchanged, and its answer passes
   through the projector once. Both paths scale identically, so the
   reader-install seam keeps its signature and its contract (R-02,
   FR-004 of specs/027: the seam installs one counters `WindowReader`
   and no second time source): the rate never arrives through the
   seam, it stands measured through the catalog, and the fake
   provider scripts both leaves in the tests. The projector and the
   rate stand per run, so no option, no CLI key and no public
   surface moves.
4. **The disclosure** (BFW-014). The JSON document's
   `jitter.disclosure` free text gains one clause stating the time
   base and the measured rate, in the form the existing clauses
   spell (a `;`-separated statement beside the loop period and
   threshold clauses at `source/harness/cli.cpp:539-556`). The
   unavailable path's disclosure stands unchanged. The console
   prints no new line: the figures carry the unit their names state
   after the fix, and the record states the rate. The JSON figure
   keys stand unchanged and carry the scaled values, so the
   document's readers read nanoseconds under ns names at schema
   1.16, the schema unmoved.
4a. **The window-sided folds' edges** (2026-11-10, the implement
phase's dated refinement): the runner's `TimedWindow` edges stand in
raw ticks (`windowStampNow` reads the catalog's `machine/tsc` leaf
directly), while the records' stamps stand projected, so the
window-sided folds of `gapStatsOf` and `seriesStatsOf` convert each
edge at the state's own origin and rate, the mapping the item's reads
used, and the placement compares one time base; a rate of 0 stands at
the identity and the edges pass through, the reading the mono-less
fixtures keep. The item captures the origin at its first read. The
runner stands untouched.

5. **The factor measurement** (BFW-013, before any code edit). The
   record carries the factor measured at the branch base, taken with
   the tree's own counters library: a scratch program under
   `$HOME/scratch/probe-unit-labels/` links the built counters
   library, reads the `machine/tsc` and `machine/monotonic` leaves
   over the same bounded spin the rate source uses, and prints the
   ratio. The ~4.19 reading of the PR #100 and PR #101 discussions
   stands cited as the estimate this measurement verifies or
   corrects, and the rate the landed fix prints cross-checks it. No
   tracked file carries the scratch program.

What stands untouched, with the rows that hold it closed:

- `include/speedgun-ng/`: the two constants stay, and no declaration
  moves (FR-039).
- `example/docs/`, its `.expected` folds and `README.md`: the mask
  folds digits, so the scaled values pass; no line count and no
  field name moves.
- The fwq sample's recorded block and `docs/pages/harness.md`: the
  same fold, the same line count, the 156 lines and the warning pair
  stand as phase 15 recorded them.
- The hwlat detector's kernel-counter reads: the SMI and NMI deltas
  stand counts, and the projection reaches the stamps alone.
- Every gate, filter, allowance, timeout, preset and
  continuous-integration file (O-5, O-6, G-34).

### Record plan

The record stands at
`specs/045-merge-fixes/results/probe-unit-closeout.md`, and the
quickstart gains one link row to it. Its sections:

1. Host, base and head: the tool versions, the affinity and
   online-cpu reads, the base SHA, the branch, and the rule that a
   tracked file carries no SHA of its own commit while each dated
   interim row names its pre-squash SHA.
2. The factor (FR-038, SC-034): the scratch measurement's commands
   and output at the branch base, the ~4.19 estimate beside its
   verified or corrected reading, and the landed rate the disclosure
   prints at the final head beside it.
3. The before and after rows (US-13, SC-035): the base probe block's
   sample output and the final-head sample output of the same entry,
   the scaled figures beside the tick figures, and the JSON
   disclosure clause.
4. The tests (FR-040, SC-036): the new test names, the files they
   stand in, and the failing-first reading against the pre-fix
   behavior.
5. The hot-path reading (edge case, VII): the alone-entry wall of the
   `docs_jitter_probe` entry, five runs per side on an idle host,
   base against final head, the medians and the ratio as a measured
   figure with no threshold invented, and the projection's per-read
   cost stated beside it.
6. The lanes at the measured head (FR-042, SC-037, SC-038): the lane
   table with the head, the exit and the counts beside each command,
   the coverage figures, the marker count, the export counts and the
   analyzer counts.
7. The push and the pull request (SC-038): the push command, the
   lease rule, the `git ls-remote` reading, and the path of the
   untracked body artifact under
   `$HOME/scratch/probe-unit-labels/` that names the measured head
   beside every required lane.
8. Rows left open (D-31): O-5 and O-6 as phases 14 and 15 wrote
   them, O-7 closed by phase 15, and O-8 closed by FR-039 and
   FR-040.

The task rows BFW-013 to BFW-017 stand written into `tasks.md` by the
tasks phase before implement. No test of this phase uses a sleep, a
wall-clock threshold or a fake cpu.

### Test plan

TDD mode (III): every code task's covering test stands written and
observed failing against the pre-fix behavior before the
implementation turns it green, and the pairs stand recorded. The
existing scripted tests keep their bytes at the 1 tick per ns fake
rate, so the failing set names the new contract alone.

| Case | File or gate | What it asserts |
| --- | --- | --- |
| the rate stands measured | `test/source/harness_jitter_probe_test.cpp`, the fake provider's two leaves | the run carries one rate figure, taken from the scripted pair, with the spin bounded and no sleep (FR-039) |
| the unit time stands scaled | the same file, a scripted rate beside 1 tick per ns | the printed `unitTimeNs`-named figure equals the scripted ticks scaled at the measured rate, exact to the tick (SC-035) |
| the threshold stands true | the same file, scripted gaps around the converted threshold | a gap the tick space kept at the old behavior stands dropped at the new, and a gap the ns contract keeps stands kept (US-14) |
| the FWQ target stands true | the same file, the calibrated work count at a scripted rate | the unit the search lands stands near 1000 true ns, the contract specs/043 FR-010 names (SC-035) |
| the FTQ edges stand true | the same file, scripted stamps and quanta | the counts per quantum match the ns contract at a rate beside 1 (US-14) |
| the projection stands drift-free | the same file, a long scripted run at a rate beside 1 | the projected ns of the run's final stamp stands exact against the scripted pair, with the remainder carried (SC-036) |
| the disclosure carries the rate | `test/source/harness_jitter_cli_test.cpp` | the JSON `jitter.disclosure` text carries the time-base clause with the measured rate, and the unavailable path keeps its bytes (FR-039) |
| the document keeps its schema | `test/source/harness_jitter_bytes_test.cpp` | the jitter keys stand unchanged and carry the scaled values at schema 1.16 (SC-035) |
| the shape folds keep their lines | `report_shape_jitter_sample` and `report_shape_fwq_ftq_sample` | exit 0: no line count and no field name moves (edge cases) |
| coverage | the `ci-coverage` tree and the gate | 100 percent lines on the touched code, branch shortfall at or below 21, no new marker (G-25) |
| no symbol moves | the exported-names audit | harness 210 and `libspeedgun-ng.so` 250 with no third-party name, the lists answering no difference against the base |
| the measured head | `git rev-parse HEAD`, `git status --porcelain`, `git ls-remote origin refs/heads/fix/probe-unit-labels` | one SHA beside every required lane, a clean tracked tree, and the remote equal to the local head (FR-042, SC-038) |

### Constitution Check

- II: the projector's monotonic-read require stands a contract the
  facility enforces, stated once in the detail header beside the
  method it guards, and no comment copy stands beside it.
- IV.1: no page change. The probe's figures change values alone, the
  shape gates fold digits, and the quickstart gains one link row to
  the record.
- VI: the new tests ship with the fix in the same change, registered
  with CTest, deterministic, and the coverage reading at the final
  head answers the gates with no new exclusion.
- VII: the projection touches the per-unit hot path, so the record
  carries the alone-entry protocol of the `docs_jitter_probe` entry,
  five runs per side on an idle host, and states the per-read cost.
  No baseline moves: the probe's figures change by the scale
  correction itself, which the record states.
- VIII: no gate, filter, allowance, marker, timeout or
  continuous-integration change moves. The time-source gate reads
  clean: the ns leaf the rate reads stands a counters leaf, and no
  forbidden source arrives anywhere.
- X.2: one type with one method, one rate figure, one wrapper at one
  lambda, one disclosure clause. No option, no helper family, no
  configuration point and no new public surface.
- X.3: the diff touches the probe's owned sources, its detail
  header, `cli.cpp`'s disclosure assembly and the test files the
  test plan names. The report writer, the JSON writer, the public
  header, the examples and the docs pages stand untouched.
- XI: the record, the comments and the commit follow ASD-STE100, and
  the prose gate runs over the tree and over the range with `--base`
  naming the branch base.

### Risks and the open items

- The fake provider's second leaf. The existing scripts declare the
  tsc leaf alone; the monotonic leaf joins the registration in the
  files the test plan names, and a file that scripts no monotonic
  leaf stands the rate's require answering loud at the first probe
  the file runs. The amendment traces to FR-040 in the tasks rows.
- The rate's precision. The spin's ns bound of 1,000,000 stands the
  reading at about one part in a million of relative error; the
  record states the bound and the reading, and the scratch
  measurement at the base cross-checks the landed rate.
- The hot path. The projection adds two divisions and a multiply to
  a read the unit body dwarfs, and the record's alone-entry protocol
  states the measured cost. A reading that stands a regression
  beyond noise stands re-measured on an idle host before any form
  change, and a form change stands recorded with its reason (D-28).
- The scripted bytes. Tests that assert the disclosure text or the
  JSON bytes stand amended where the time-base clause lands, and
  each amendment traces to FR-039 in the tasks rows. The stamp
  scripts keep their bytes at the 1 tick per ns rate.
- The measured head. A tracked change after a measurement produces a
  new squashed head and a fresh run of every required lane. The
  factor measurement stands taken once at the base and stands fixed.
- The remote. A failed `--force-with-lease` stops the work and
  stands reported, and the lease stands never refreshed and retried.
  A remote that refuses the push stands the push row blocked with
  the refusal, and the local half, a clean tracked tree and
  `git rev-parse HEAD`, stands proved either way. The `gh`
  credential of this machine stands absent, so the branch, the push
  proof and the body text stand delivered through one local file for
  the operator.
- O-5, O-6 and O-7 stand as the earlier phases wrote them. O-8
  closes with FR-039 and FR-040.
