# Quickstart: Harness User Counters and Statistics

Verification walkthrough for `specs/019-harness-user-counters-and-statistics/plan.md`.
Every command ran from the repository root.

## Prerequisites

- Linux, GCC and Clang of the CI matrix, CMake and CTest.
- `python3` for the prose and drift gates.
- The coverage preset for the FR-047 assertion; `ci-sanitize` and
  `ci-tsan` for the Principle VIII matrix.

## 1. Configure and build (SC-06)

```sh
cmake --preset=ci-rocky
cmake --build build
```

The build produces the `run_*` example targets including the eighteen
`run_docs_*` targets of the `example/docs` files (FR-032, FR-033).

## 2. The two fixes first (SC-08)

```sh
ctest --test-dir build --output-on-failure \
  -R 'harness_pause_test|harness_time_unit_test'
grep -rn 'kMaxPausePerRun' source test docs && echo FAIL || echo gone
```

The H3 fixes are the first working phase (Q-7), so they verify first:
the pause test completes a run that pauses in every iteration, keeps
the reservation at or under `kMaxPauseSamplesPerLeaf` points per leaf
column for a pause-heavy scripted run, reserves no pause room for a
non-pausing body, and prints the fold path's columns equal to the raw
path's (FR-051 to FR-054, SC-11 to SC-13); the time
unit test shows a `setDefaultTimeUnit` call in `main` reaching macro
registrations while `unit(...)` still wins (FR-024 to FR-027).

## 3. The new test executables (SC-01 to SC-05)

```sh
ctest --test-dir build --output-on-failure \
  -R 'harness_user_counters_test|harness_throughput_test|harness_label_test|harness_custom_stats_test|harness_aggregates_only_test|harness_complexity_test'
```

Each executable scripts the fake provider, so no PMU is needed. The
abort scenarios re-exec their own binary and read back the signal
(FR-001, FR-008, FR-012, FR-015).

## 4. The docs examples run (SC-09)

```sh
ctest --test-dir build --output-on-failure -R example_run
```

The script runs every `docs_` binary through its `run_docs_` target
and fails when a registered
benchmark of a file prints no result row or prints `FAILED`; the
"Skips" example declares its intentional skip with an
`// example-run: expect-skip <reason>` marker, and the check fails on
an undeclared skip or an unprinted expectation (FR-034, FR-035,
D-15, SC-14).

## 5. The gates stay green (SC-06, SC-07, SC-10)

```sh
ctest --test-dir build --output-on-failure   # the full suite
python3 tools/docs/example_drift.py .        # drift, 550-line cap, ownership
python3 tools/prose/prose_gate.py --check prose --mode diff
```

`loop_shape.sh` runs with a body that sets counters inside the loop and
stays green (FR-007); `time_source_gate.sh` stays clean (FR-050);
`report_shape` pins the new fields (SC-02).

## 6. Sanitizers, threads, coverage (SC-06, FR-047)

```sh
cmake --preset=ci-sanitize && cmake --build build && ctest --test-dir build
cmake --preset=ci-tsan && cmake --build build && ctest --test-dir build
cmake --preset=ci-coverage && cmake --build build && ctest --test-dir build
```

The coverage report keeps 100% line and branch over
`include/speedgun-ng/*` and `source/*` with no `LCOV_EXCL` marker under
`source/harness/` (FR-047).

## 7. The constitution amendment (FR-036, SC-07)

```sh
grep -n '2.20.0' .specify/memory/constitution.md
```

IV.1 reads the compiles-and-runs obligation, Principle VIII names the
`example_run` check, the amendment record and lineage row are present,
and the version footer and sync impact line state 2.20.0.

## 8. What is out of scope

Threads, JSON and CSV reports, the raw-data report, the full command
line, percentile histograms, and reloaded pass-or-fail judgements each
stand as a roadmap spec; `grep -rjn json source/harness` stays empty.
