# Quickstart: 045-merge-fixes validation

Spec: [spec.md](./spec.md). Plan: [plan.md](./plan.md). Every command
runs from the repository root on Linux. The gates prove the spec's
success criteria; the design details sit in
[data-model.md](./data-model.md) and the research annexes.

## Prerequisites

- Linux x86-64, GCC or Clang with CMake and Ninja.
- lcov with a working perl XS (the corsairone repair is host-local;
  see the spec's acceptance section).
- A clean tree at the fix head; the two commits carry one topic each.

## Build and test the suite

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: every entry passes (SC-001). Then pin the suite to two
cpus:

```sh
ctest --test-dir build --output-on-failure -j2   --tests-regex '.*' --parallel 2   # under taskset -c 0,1
taskset -c 0,1 ctest --test-dir build --output-on-failure
```

Expected: the same green result on thrawn and on corsairone; a
failure that stays carries its host-artifact reason in the record.

## Sanitizers and threads

```sh
cmake --preset=ci-sanitize && cmake --build build/sanitize
ctest --test-dir build/sanitize --output-on-failure
cmake --preset=ci-tsan && cmake --build build/tsan
ctest --test-dir build/tsan --output-on-failure
```

Expected: no ASan/UBSan finding; no TSan race in the jitter tests
(FR-007, FR-008, SC-004).

## Static analysis and the standing tools

```sh
cmake --preset=ci-ubuntu && cmake --build build.ci-ubuntu
cmake -P cmake/lint.cmake
cmake -P cmake/spell.cmake
tools/docs/example_drift.py
tools/prose/prose_gate.py --base origin/master
tools/dbc/dbc_pair_gate.py
```

Expected: zero new clang-tidy and cppcheck findings; all five tools
pass (SC-005).

## Coverage

```sh
cmake --preset=ci-coverage
cmake --build build/coverage
ctest --test-dir build/coverage --output-on-failure -R coverage
```

Expected: 100 percent lines; branch shortfall 21 or less, the number
recorded in the PR body; no unexplained dark line; the marker count
by file reads 84 at the Part A head (SC-002, SC-003, FR-005).

## The fixed behaviors, one check each

| Requirement | Check | Expected |
| --- | --- | --- |
| FR-001 | Run `counters_pmu_provider_test` on a host whose CPU matches no vendored table | Passes; the test scripts the identity |
| FR-002 | Run `counters_kernel_calls_test` with `perf_event_paranoid = -1` | Passes; the refusal rides the seam |
| FR-003 | `git grep -c LCOV_EXCL -- source include` | The count fell; no net-new exclusion line |
| FR-006 | Run `harness_jitter_window_test` | Every FWQ window holds its samples; the document carries a real jitter record |
| FR-007 | Run `harness_fwq_ftq_test` under `ci-sanitize` | No negative-delta finding |
| FR-008 | Run `harness_jitter_cli_test` under `ci-tsan` | No race at the mode=both folds |
| FR-009 | `ctest -j` with `prose_gate_fixtures` | The entry finishes inside 10 seconds |
| FR-010 | Read `specs/043-fwq-ftq/tasks.md` Phase 10 | T048-T049, T051-T056 ticked; T050, T057, T061 open with reasons |
| FR-012 | Read the `CMakeLists.txt` SOVERSION comments | PR #70 named; no number changed |

## Record the acceptance

Write the per-host counts into `specs/045-merge-fixes/results/` and
mirror them in the PR body, in the order the spec's acceptance
section states: corsairone first, then thrawn.

Phase 15 (fwq-example-n13e): [the closeout record](./results/fwq-example-closeout.md).
Phase 16 (probe-unit-labels): [the closeout record](./results/probe-unit-closeout.md).

## Phase 13: the coverage-data repair, one reproducible route

The fault: a closure written inside an `SG_*` contract predicate gains a
gcov function record whose only line the extract step's
`--omit-lines "SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?"` pattern
removes, so the written trace holds a hit function with no hit line
inside it, and `lcov --list`, `lcov --summary` and `genhtml` refuse the
trace. The repair moved each such closure into a named helper outside
the macro, and the class left the three reads of the written trace.

Run the lane on Linux with lcov 2.4 or newer. Keep any host lcov
configuration out of the route, because `check_data_consistency = 0` in
a host file hides the fault:

```sh
test -f ~/.lcovrc && mv ~/.lcovrc ~/scratch/coverage-data-repair/lcovrc.host-local
rm -rf build/coverage
cmake --preset=ci-coverage -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
cmake --build build/coverage -j 24
ctest --test-dir build/coverage --output-on-failure --no-tests=error -j 24
cmake --build build/coverage --target coverage
```

Expected: the capture prints seven `(inconsistent)` end-line records for
the fixture macros and exits 0; the extract step prints nothing; the
`--list` step prints the table; the gate reads
`lines.......: 100.0%` and names its branch shortfall. The `genhtml`
step exits 0. No read prints
`is hit but no contained lines are hit`.

Two habits the lane needs. An instrumented configure with the compiler
launchers empty, because a ccache hit for an instrumented translation
unit leaves a `.gcda` whose stamp disagrees with its `.gcno` and the
capture stops with `lcov: ERROR: (gcov) GCOV failed`. And a run of
`harness_jitter_cli_test` that reaches its end: the entry needs about
285 s on thrawn against the 300 s `ctest` ceiling, and a capture that
cuts it leaves 33 dark lines in `source/harness/jitter_probe.cpp`.

Restore the host file afterwards:

```sh
mv ~/scratch/coverage-data-repair/lcovrc.host-local ~/.lcovrc
```
