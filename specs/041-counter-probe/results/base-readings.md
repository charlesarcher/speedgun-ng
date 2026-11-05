# Base readings at `858c55d9`

**Branch**: `041-counter-probe` | **Date**: 2026-11-05 | **Host**: `gary`
**Base**: `858c55d9d036e74284dc48cd6fb7e42988cc8cff`, `origin/master` at branch time.
**Build**: `cmake --preset=ci-rocky`, `cmake --build build`, Release developer mode.

These are the left sides every delta of this lane compares against (T001 to T004).

## The figures

| Item | Base answer | Command |
|---|---|---|
| Head sha | `858c55d9d036e74284dc48cd6fb7e42988cc8cff` | `git rev-parse HEAD` |
| Project version | 0.31.0 (`CMakeLists.txt:64`) | `grep -n 'VERSION 0\.' CMakeLists.txt` |
| Archive `SOVERSION`, harness `SOVERSION` | 6 (`:143`), 21 (`:419`) | `sed -n '143p;419p' CMakeLists.txt` |
| `LCOV_EXCL` markers | 84 | `grep -c LCOV_EXCL -r source include` |
| `README.md` lines | 1001, cap 1004 (ruling G-7) | `wc -l README.md` |
| Registered entries | 181 | `ctest --test-dir build -N` |
| Version line | `speedgun-ng 0.31.0 build=Release` | the example's own first line |
| DBC gates | doc-gate 347 interfaces, 0 gaps; pair-gate 347 interfaces, 0 gaps | `cmake --build build --target dbc-gate` |
| `cmake -P cmake/lint.cmake` | exit 0 | T003 |
| `cmake -P cmake/spell.cmake` | exit 0 | T003 |
| `python3 tools/docs/example_drift.py` | exit 0 | T003 |
| `prose_gate --check prose --mode tree` | exit 0, 1011 sources, 191898 units, 0 findings | T003 |
| `test/time_source_gate.sh` | `time_source_gate: clean`, exit 0 | T003 |
| `tools/dbc/coverage_gate.sh` allowance | 21 branch rows (`:102`, ruling G-8) | `grep -n allowance tools/dbc/coverage_gate.sh` |

## The host

```text
Linux 7.3.1-1-cachyos x86_64
gcc (GCC) 16.2.1 20260810
2
0
```

`uname -srm`, `gcc --version`, `kernel.perf_event_paranoid`, `kernel.nmi_watchdog`.

## The suites at the base (T002)

| Command | Entries | Passed | Failed | Skipped | Wall |
|---|---|---|---|---|---|
| `ctest --test-dir build --output-on-failure` | 181 | 180 | 0 | 1 | 308.12 s |
| `taskset -c 0,1 ctest --test-dir build --output-on-failure` | 181 | 180 | 0 | 1 | 257.39 s |

No entry failed. `harness_topdown_run_test` is reported skipped because it exits with
the registration's `SKIP_RETURN_CODE 2` after one of its 27 scenarios skips on its own
host-identity guard; its other 26 scenarios passed. specs/046's AMD acceptance record
names the same reading, on the branch of pull request 81.

## The known host blocker

`--hunt` on the whole catalog aborts on this host at this commit, on the passes path,
at the ratio-pair contract of `source/counters/fold.cpp:166`. The full reading, the
`strace` counts and the two `perf_event_paranoid` values stand in the record above.
Two consequences bind this lane:

- No acceptance row of this lane runs a full live hunt through the passes path. The
  scenarios of M5 drive scripted objects, and FR-045's hunt-refusal test drives the
  CLI refusal, which answers before any collection.
- The row belongs to specs/024's pass planner and fold, and this lane changes no code
  there.

## Coverage at the base (T004)

The `ci-coverage` row ran in `build/coverage` with the stale `.gcda` counters deleted
before the suite: 181 entries, 180 passed, 1 skipped, 0 `Not Run`, and the trace reads
lines 100.0% (17812 of 17812) with branches 99.4% (9298 of 9351), a shortfall of 53
rows. The `coverage` target stops before its gate on this host, and the 53 rows are a
host reading against a CI-union allowance: both facts, the read route and the
per-file table stand in `results/coverage-base.txt`.
