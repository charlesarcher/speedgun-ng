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
