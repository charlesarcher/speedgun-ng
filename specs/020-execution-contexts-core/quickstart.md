# Quickstart: Execution Contexts Core

Validation guide for the implemented feature. Every command runs from
the repository root; every time figure is scripted on the fake
provider. This guide runs scenarios, it does not restate the design:
the decisions live in [research.md](./research.md), the fields in
[data-model.md](./data-model.md), and the rules in
[contracts/](./contracts/).

## Configure and build

```bash
cmake --preset=ci-rocky
cmake --build build
```

## C-1 to C-3: context, policy, and typed work

```bash
ctest --test-dir build --output-on-failure -R harness_execution_context_test
```

Expect: work runs on the context's thread and `currentContext()` names
it (SC-01); the three wait modes pass the same dispatch test and the
`BLOCK` mode wakes on a dispatch (SC-02); the lifecycle trio runs every
queued node exactly once and the re-exec rows show the destructor
aborting on an unjoined context (SC-03); the allocation-hook test
counts zero allocations in `dispatch()` (SC-04); the replaced
placement function records one call per thread at thread start and an
invalid cpu index aborts its re-exec (SC-05); the compile-fixture shows
an unapproved item type refused before the program builds (FR-008);
the order rows show single-dispatcher FIFO and per-dispatcher order
(FR-013).

## C-4: the context barrier

```bash
ctest --test-dir build --output-on-failure -R harness_context_barrier_test
```

Expect: one round releases every participant at once; a spinning
participant waits in its own mode (SC-08's mechanism); a barrier used
outside running work waits in `BLOCK` (FR-014, FR-015); a participant
that leaves a round early aborts its re-exec (FR-015); the barrier
reads no clock, so no round times out (FR-014).

## C-5: threads on contexts

```bash
ctest --test-dir build --output-on-failure -R harness_threads_test
```

Expect: `threads(4)` reports `threadIndex()` values 0 to 3, `threads()`
4, and a row named with the `/threads:4` suffix (SC-06); with scripted
per-context leaves the merged row equals real-max, cpu-sum,
iterations-times-N, and every counter flag equals the cited `Finish`
result at `N = 4` (SC-07, contracts/threaded-runs.md); `threads(0)`
aborts at the call (FR-024); the window bounds recorded around the
barrier arrives show no barrier wait inside a window (SC-08); the noise
load on one context leaves the other context's plan the only one its
row folds (SC-09); the custom runner's dispatches carry the bodies and
the merged row still stands, and a runner that returns with a body
undispatched aborts its re-exec (FR-021); a benchmark with no thread
call prints today's bytes (FR-022).

## C-7: disclosure and noise warning

```bash
ctest --test-dir build --output-on-failure -R harness_context_report_test
ctest --test-dir build --output-on-failure -R report_shape
```

Expect: threaded results print the context count and each context's
wait mode and placement; single-context results print no disclosure
line; the warning appears after the result whose condition holds, one
line per such result, for the shared-cpu spinner pair and the spinner
count above the online count, and stands absent for the pinned
one-per-cpu run and the all-blocking run (SC-12, FR-026 to FR-028); the
report-shape checks carry the disclosure line in the same commit as
the printer.

## C-6: docs and examples

```bash
ctest --test-dir build --output-on-failure -R example_run
python3 tools/docs/example_drift.py .
```

Expect: `run_docs_execution_contexts` and `run_docs_threads` run clean
with no undeclared skip and no `FAILED` row, and every registered
benchmark prints its row (FR-030, FR-035); the drift tool passes with
the two new titles in `SECTIONS`, each owning example holding its
marked region within the 25-line cap, and the page inside the moved
`PAGE_LINE_CAP` (FR-034); `README.md` shows the two D1-shaped sections,
the quick-reference rows, and a `/threads:N` row in "Reading the
report" (FR-029 to FR-032); `docs/pages/harness.md` carries the
aggregation table and the lifecycle rules without spec numbers
(FR-033).

## The full suite and the standing gates

```bash
ctest --test-dir build --output-on-failure   # the full suite
```

Expect, with the new paths included: `loop_shape`, `cli_shape`,
`barrier_shape`, and `time_source_gate` green (no clock entered any
new path, FR-039; no option landed, FR-044); the prose gate and the
spell check report zero findings (FR-048); the dbc pair gate shows zero
drifted interfaces. Then the analyzer-bearing release build:

```bash
cmake --preset=ci-ubuntu && cmake --build build
```

Expect: the build completes with clang-tidy and cppcheck wired in, and
`readability-identifier-naming` reports zero findings (FR-036);
Principle IX calls for this release build once per feature before the
tasks are called done.

## Sanitizers and coverage

```bash
cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize && \
  ctest --preset=ci-sanitize
cmake --preset=ci-tsan && cmake --build --preset=ci-tsan && \
  ctest --preset=ci-tsan
cmake --preset=ci-coverage && cmake --build build/coverage && \
  ctest --test-dir build/coverage --output-on-failure && \
  cmake --build build/coverage --target coverage
```

Expect: the sanitizer presets run clean, the ThreadSanitizer preset
reports zero findings on the inbox, the barrier, the disclosure, and
every threaded path (FR-041, SC-10), and the coverage gates hold at
100% lines and 100% branches over `source/harness/` with no
`LCOV_EXCL` marker (FR-038). The sanitize and tsan presets write their
own trees (named by the presets, like the coverage preset names
`build/coverage`), so the `--preset` forms name the tree the gate is
about.

## The version table

```bash
grep -n 'VERSION 0.10.0' CMakeLists.txt        # FR-043
grep -n 'SOVERSION 6' CMakeLists.txt           # the harness archive
grep -n 'SOVERSION 2' CMakeLists.txt           # the counters archive, unchanged
```

Expect: the project version at 0.10.0, the harness archive at
`SOVERSION 6`, and the counters archive still at `SOVERSION 2`; the
install rules keep `SameMinorVersion`.

## Out of scope for this validation

The execution graph (H16), topology-aware placement (H10), a
`--threads` option (H9), long-lived context pools, JSON and CSV
reports, and the raw-data report: none is implemented here (the spec's
Out list), and none has a scenario above. The four H16 hooks ship and
are tested for their own meanings (FR-045), nothing beyond them.
