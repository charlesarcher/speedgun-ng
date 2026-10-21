# Quickstart: Regression Gate

**Date:** 2026-10-21 · **Spec:** [spec.md](./spec.md)

A reviewer drives the whole feature from a fresh clone with the
checked-in release preset. The `dev` presets are machine-local and
absent from a fresh clone, so every command here runs `ci-rocky`.

## Prerequisites

A CMake 3.20 or newer generator for a release developer-mode build.
Nothing else: simdjson 4.6.11 is vendored at `external/simdjson`.

## Configure and build

```sh
cmake --preset=ci-rocky
cmake --build build
```

The `speedgun-ng_harness` archive grows by one translation unit,
`source/harness/compare.cpp`, through the `GLOB_RECURSE` at
`CMakeLists.txt:191`; no CMake edit is needed for it.

## The feature's own tests

```sh
ctest --test-dir build --output-on-failure -R 'harness_compare'
ctest --test-dir build --output-on-failure -R 'harness_json_load_test'
```

`harness_compare_test` covers the grammar, the value resolution, the
threshold arithmetic, the noise guard and the validation pass over
`ReportData` values built in code. `harness_compare_cli_test` covers
the option scan, the exit statuses, the one-line faults, the table and
the comparison document, and it calls `checkReport` with
`docs/schema/compare-v1.json` over the written bytes. The fixture
documents live under `test/data/` and reach the tests as a path
argument in `add_test`, the shape of `test/CMakeLists.txt:82`.

## The mode, by hand

The quick-start program links the harness archive, so it carries the
compare mode. The fixture pairs under `test/data/` are committed
report documents, so every run below is deterministic.

```sh
./build/example/first_benchmark \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json
echo $?
```

The table prints its header naming both files, both speedgun versions
and both schema versions, then one row per benchmark with the baseline
value, the contender value, the change in percent and in absolute
units, and the word `OK`. The echo answers 0.

```sh
./build/example/first_benchmark \
  --compare=test/data/gate_fail_base.json,test/data/gate_fail_contender.json
echo $?
```

The contender sits 9 percent behind the default rule, the row prints
`REGRESSED` and the echo answers 1.

The other forms, each with the answer the contract carries:

| Command tail | Printed line and status |
| --- | --- |
| `--compare=test/data/gate_fit_base.json,test/data/gate_fit_contender.json` | the fit entry prints `SKIPPED`, the readable benchmark prints its word, status 0 |
| `--compare=test/data/gate_ok_base.json,test/data/gate_schema_11.json` | the `1.0` baseline against the `1.1` contender shares its major, so the row prints `OK`, status 0 |
| `--compare=test/data/gate_ok_base.json,test/data/gate_schema_20.json` | one line naming the schema version the reader refuses, status 2 |
| `--compare-rule='time@median:lower:5' --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json` | one line naming the rule text and the threshold fault, status 2 |
| `--compare-rule='time@median:lower:5%:nothing-matches' --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json` | one line naming the rule and its no-match filter, status 2 |
| `--compare-rule='p99:lower:10%' --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json` | the value resolves whole, status 0 |
| `--filter=zzz --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json` | one line saying the pair matched nothing, status 0 |
| `--compare=test/data/gate_ok_base.json` | one usage line naming the option, status 2 |

## The document

```sh
./build/example/first_benchmark \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --json=compare-a.json
./build/example/first_benchmark \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --json=compare-b.json
cmp compare-a.json compare-b.json
```

`cmp` answers nothing: the document carries no date, so the two runs
write the same bytes (SC-009). The keys stand in the `x-order` of
`docs/schema/compare-v1.json`, and `harness_compare_cli_test` runs the
written bytes through `checkReport` with that schema.

## The gates

```sh
ctest --test-dir build -j 2 --output-on-failure
bash test/cli_shape.sh
bash test/loop_shape.sh build
bash test/time_source_gate.sh
cmake --build build --target format-check
cmake -P cmake/spell.cmake
cmake -P cmake/prose-lint.cmake
python3 tools/docs/example_drift.py .
```

`test/cli_shape.sh` locates the benchmark loop of `speedgunMain` by its
`runner.run(*entry)` line; the compare branch returns before that loop,
so the gate reads its base anchors. `test/loop_shape.sh` must print its
base reading: the feature touches no per-iteration path (FR-072). The
script names its build directory, so the manual run passes `build`.
`example_drift.py` checks the new `Regression gate` section against
`example/docs/regression_gate.cpp`, the option rows against the
`longOptions` table, and the page against the raised `PAGE_LINE_CAP`.

## The sanitizer and coverage runs

```sh
cmake --preset=ci-sanitize && cmake --build build/sanitize \
  && ctest --test-dir build/sanitize -j 2 --output-on-failure
cmake --preset=ci-tsan && cmake --build build/tsan \
  && ctest --test-dir build/tsan -j 2 --output-on-failure
cmake --preset=ci-coverage && cmake --build build/coverage -j 2 \
  && ctest --test-dir build/coverage -j 2 --output-on-failure \
  && cmake --build build/coverage -t coverage
```

The coverage preset keeps `-fprofile-update=atomic` and no
`-D_GLIBCXX_ASSERTIONS`. `tools/dbc/coverage_gate.sh` requires 100%
lines and 100% branches with no `LCOV_EXCL` marker, so every optional
field read and every fault branch of the comparison carries a test.

## What each story proves on its own

- US1: the two hand runs above answer 0 and 1 with the table between
  them, and no benchmark ran (SC-001, SC-002, SC-005).
- US2: the `p99` row resolves whole and the filter forms answer their
  scopes (SC-003).
- US3: each fault row prints one line and answers 2 with no table
  (SC-004, SC-013).
- US4: the noisy fixture pair prints `NOISY` with its reason and
  answers 0, and `--compare-strict` turns that answer into 1 (SC-006).
- US5: the `cmp` above proves the byte rule and the checker proves the
  schema (SC-009).
- US6: `example_drift.py` and `example_run.sh` prove the page, the
  example and the budget (SC-011).
