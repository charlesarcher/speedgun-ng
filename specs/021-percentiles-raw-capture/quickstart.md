# Quickstart: Percentiles and Raw Capture

Runnable validation for the feature. Every command runs from the repository
root. The `dev` presets are machine-local and absent from a fresh clone, so
this guide drives `ci-rocky`, the checked-in release developer build.

## Prerequisites

- Linux x86-64, GCC or Clang at the versions the CI matrix pins
- Submodules initialized: `git submodule update --init --recursive`
- `python3` for the docs and prose gates

## Configure and build

```bash
cmake --preset=ci-rocky
cmake --build build
```

## The feature's own tests

```bash
ctest --test-dir build --output-on-failure -R 'harness_percentile|harness_capture|harness_cli|harness_context_report|harness_calibration'
```

Expected: every test passes. `harness_percentile_test` covers the rank rule,
the minimum sample counts, the merged threaded set, and the not-available
token. `harness_capture_test` covers the point count, the hardStop capacity,
and the bound of FR-006.

## The chunked cadence, by hand

```bash
./build/example/docs/reading_the_report
```

Expected: each run row prints `endpoint samples=` with the chunked count, the
`chunks + 1` value of a run with no pause, and no percentile line, because the
output stands off by default (FR-023).

```bash
./build/example/docs/percentiles
```

Expected: the run row, then the chunk disclosure line with `chunks=` and
`iterations/chunk=`, then one percentile line per reported column. The values
match `example/docs/percentiles.expected` line for line.

## The observer cost

```bash
./build/example/docs/percentiles | grep 'overhead floor'
```

Expected: one field per run row, the median cost of one `sample()` action for
that plan, and the chunk size of the same row at least 100 times that cost
(FR-005, FR-017).

## The option forms

```bash
./build/example/docs/percentiles --percentiles=99.9
./build/example/docs/percentiles --no-percentiles
./build/example/docs/percentiles --percentiles=100; echo "status=$?"
```

Expected: the first prints p99.9 alone, or `not available` when the run holds
fewer than 1000 chunk values; the second prints no percentile line over a
family that asked for one; the third prints one line naming the option and the
text, and reports `status=2` with no benchmark run (FR-022).

## The gates

```bash
ctest --test-dir build --output-on-failure          # the full suite
python3 tools/docs/example_drift.py .               # the page and its regions
python3 tools/prose/prose_gate.py --check all       # prose and commits
bash test/time_source_gate.sh                       # counters-only
bash test/loop_shape.sh build                       # the timed loop shape
```

Expected: no findings from any of them. The loop gate now carries the
chunk-boundary probe, so a per-iteration sample fails it.

## The sanitizer and coverage runs

```bash
cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize
ctest --test-dir build/sanitize --output-on-failure
cmake --preset=ci-tsan && cmake --build --preset=ci-tsan
ctest --test-dir build/tsan --output-on-failure
cmake --preset=ci-coverage && cmake --build build/coverage
ctest --test-dir build/coverage
cmake --build build/coverage -t coverage
```

Expected: clean under all three presets, and the coverage gate reports 100%
line and 100% branch over `source/harness/percentiles.cpp` with no exclusion
marker (FR-032).

## What each story proves on its own

| Story | Command | Proof |
|---|---|---|
| US1 the time tail | `harness_percentile_test`, `docs/percentiles` | the four defaults print and equal the oracle's nearest-rank values |
| US2 the counter columns | `harness_percentile_test`, `docs/percentiles` | every reported column carries a line in its own unit |
| US3 the threaded rows | `harness_context_report_test` | the merged line equals the union, each context equals its own |
| US4 the options | `harness_cli_test` | the accepted forms print, the invalid forms exit 2 |
| US5 the cost | the grep above | the chunk size stands at least 100 times the disclosed cost |
| US6 the page | `example_drift.py`, `example_run` | the section, the region, and the rows agree |
