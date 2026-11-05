# Acceptance record: corsairone, the instrumented items (specs/045-merge-fixes T201)

Heads under test: `d214be37` for the full suite, `6025a71f` for the coverage,
sanitize and tsan trees, `76fd4aa3` for the two-cpu reading and the window
entry. The branch moved from `d214be37` to `76fd4aa3` through two test fixes
(`e1c1b4ef`, `6025a71f`) and two Docs commits. `76fd4aa3` differs from
`6025a71f` by Docs commits only, so the instrumented figures below are the
figures for the code the pull request carries.

Host: CorsairOne, kernel `7.3.0-4-cachyos`, gcc `16.2.1 20260810`, gcov
`16.2.1`, cmake `4.4.2` (`~/.local/bin/cmake`), ctest `4.4.2`, lcov
`LCOV version 2.5.0-beta` (`~/.local/bin/lcov`), clang `23.1.3`, clang-format
`18.1.8`, codespell `2.4.3`, git `2.56.0`, python3 `3.14.8`, 24 cpus.
`/proc/sys/kernel/perf_event_paranoid` reads `-1`. Date: 2026-11-03.

Worktrees on the host: `~/sgng-045-accept`, detached at `d214be37` from
19:04, and `~/sgng-045-t201`, detached at `76fd4aa3`. The standing checkout
`~/code/speedgun-ng` stayed on `046-hunter-topdown` throughout.

## 0. Host-local measures, none of them in the repository

1. The lcov repair. `/usr/bin/perl` reads 5.44.0. The `DateTime` XS under
   `~/.local/lib/perl5` was built for 5.42, and `lcovutil.pm` loads it
   unconditionally. `lcov --version` died with `Perl API version v5.42.0 of
   DateTime.c does not match v5.44.0` and exited 1, so every coverage command
   failed before it read a counter. `DateTime.pm` keeps a pure-perl path behind
   the `PERL_DATETIME_PP` environment variable, and `genhtml` lacks the
   `use lib` line that `lcov` carries. The run exported `PERL_DATETIME_PP=1`
   and `PERL5LIB=$HOME/.local/lib/perl5/x86_64-linux-thread-multi:$HOME/.local/lib/perl5`.
   `lcov --version` and `genhtml --version` then both read
   `LCOV version 2.5.0-beta`. No file on the host changed for this measure.
2. `~/.lcovrc` with `check_data_consistency = 0`. lcov refused the extracted
   trace: `ERROR: (corrupt) unable to read trace file ... inconsistent
   "source/harness/runner.cpp":707: function
   _ZZN2sg6detail19stripTopdownColumns... is hit but no contained lines are
   hit`. The file stands on the host alone.
3. `PYTHONPATH=$HOME/scratch/speedgun-ng-045/venv/lib/python3.11/site-packages`
   for the `dbc-gate` target. The interpreter CMake selected,
   `$HOME/.local/bin/python3.11`, carries no PyYAML, and the gate
   stopped with `dbc gates require PyYAML`. A venv under
   `~/scratch/speedgun-ng-045` supplies it; `/tmp` stands out of bounds for
   storage and temporary builds.
4. A fresh worktree needs
   `git -c protocol.file.allow=always submodule update --init --recursive`.
   Git 2.56 refuses the file transport for the vendored submodules, and
   configure stops with `the vendored tree 'external/hwloc' is missing or
   empty`.
5. No gcov stamp mismatch appeared. The coverage build kept the ccache launcher
   of the preset, and gcov 16.2.1 read every `.gcda`, the
   `source/counters/linux_pmu/table_parse.cpp` pair among them. No `.gcda`
   stood deleted.

## 1. Full suite at `d214be37`

`cmake --preset=ci-rocky`: exit 0.
`cmake --build build -j 24`: exit 0.
`ctest --test-dir build --output-on-failure --no-tests=error`: exit 8.

```
98% tests passed, 3 tests failed out of 178
Total Test time (real) = 2029.13 sec
The following tests FAILED:
     30 - prose_gate_fixtures (Failed)                      quick
     48 - counters_kernel_calls_test (Failed)               quick
     87 - harness_jitter_window_test (Failed)               quick
```

`prose_gate_fixtures`, 16.79 s. One FAIL verdict:

```
VERDICT  name=timing.whole-tree  check=timing  expected=exit:0|1 wall<10.0s
  observed=exit:0 wall:12.157s prose-lint: 978 sources, 183504 units examined,
  0 findings, 0 skipped  FAIL
prose gate fixture runner over the bound: 10.0s allowed, 16.615s measured
```

`counters_kernel_calls_test`, 0.03 s. Two FAIL lines:

```
KERNEL CALLS TEST FAIL: the real open refuses a malformed attribute
KERNEL CALLS TEST FAIL: the real open sets an errno on its refusal
```

The entry planted a `perf_event_attr` with a zero size field and expected
`EINVAL`. Probed on this host with the same call the seam makes
(`pid 0`, `group_fd -1`, `flags 0`):

| `attr.size` | `cpu -1` | `cpu 0` |
| --- | --- | --- |
| 0 | granted, fd 3, errno 0 | granted, fd 3, errno 0 |
| 8 | `E2BIG` | `E2BIG` |
| 144 | granted, fd 3 | granted, fd 3 |

`perf_event_paranoid` reads `-1` here, so the kernel opens the call and grants
a descriptor for a zero size. The premise is a host fact. `e1c1b4ef` replaced
the premise with an attribute that names a pmu no host registers, and that
refusal holds on both hosts.

`harness_jitter_window_test`, 0.40 s. One FAIL line:

```
HARNESS JITTER WINDOW TEST FAIL: the entry whose windows kept no gap loads no
fold (FR-027, E-08)
```

## 2. Two-cpu reading: the discarded pass and the kept pass

Discarded. The pass that ran from 19:47 to 20:17 reported
`99% tests passed, 1 tests failed out of 178`, with
`harness_jitter_window_test` the only failure. The worktree moved under it: the
reflog records checkouts to `8650e6c7` at 19:49:26, to `e1c1b4ef` at 19:49:35
and to `6025a71f` at 19:50:53, and `build/test/counters_kernel_calls_test`
rebuilt at 19:49. The reading mixes `d214be37` binaries with `6025a71f`
binaries, so it records nothing.

Kept pass, `~/sgng-045-t201` at `76fd4aa3`, a worktree no other session holds.
`cmake --preset=ci-rocky`: exit 0. `cmake --build build -j 24`: exit 0.
`taskset -c 0,1 ctest --test-dir build --output-on-failure`: exit 8.

```
load average before: 2.71, 3.59, 4.48
99% tests passed, 1 tests failed out of 178
Total Test time (real) = 2009.37 sec
The following tests FAILED:
    166 - report_shape_fwq_ftq_sample (Failed)              quick
load average after: 11.82, 5.66, 3.82
```

`harness_jitter_window_test` passed in that run. The one failure is the sample
block comparison, 21.98 s:

```
sample output block holds 156 lines, the run printed 125
line 102: noise warning line differs: block 'warning: cpu 16 noisePercent 37.03
  maxValue 4914' against run 'pinning candidates: cpu 1 0.94 cpu 0 14.17'
```

The sample carries a 24-cpu report, and the run under the two-cpu mask prints a
shorter one. The entry stands reported as a mask artifact of this host.

An earlier attempt at these two readings died on the host: `ctest` took SIGTERM
at test 79 with exit 143, and one standalone run of the window entry took
SIGTERM. Nothing in the log names a cause, memory stood free, and the kernel
ring is unreadable without root. The kept pass ran under `setsid`, and nothing
touched it.

## 2b. `harness_jitter_window_test` standalone, five runs, `76fd4aa3`

| Run | Load average before | Result |
| --- | --- | --- |
| 1 | 11.82, 5.66, 3.82 | FAIL: `theSeriesRoundTripsThroughLoadReport: the entry's jitter record carries its series array (FR-006, FR-020, R-11)` |
| 2 | 11.03, 5.60, 3.81 | FAIL: `the exit status does not follow contracts/cli.md (FR-036)`, beside `error: the rule 'jitter:0:sampleCount:lower:abs=1' names cpu 0 and the jitter stats of bmQuiet in the baseline report do not carry it` |
| 3 | 10.31, 5.54, 3.80 | `harness_jitter_window_test: ok` |
| 4 | 9.56, 5.47, 3.78 | `harness_jitter_window_test: ok` |
| 5 | 8.96, 5.41, 3.77 | `harness_jitter_window_test: ok` |

Two failures in five runs, on two different assertions, with the load average
falling from 11.8 to 9.0 across the five. The same entry passed inside the
two-cpu suite of the same tree minutes earlier. The window assertions bound a
probe window by wall time, and the entry stands `RUN_SERIAL`, so the residual
flake of the thrawn record stands on this host too.

## 3. Coverage at `6025a71f`

`cmake --preset=ci-coverage`: exit 0.
`cmake --build build/coverage -j 24`: exit 0.
`ctest -j 24 --output-on-failure --no-tests=error` in `build/coverage`: exit 8.

```
95% tests passed, 9 tests failed out of 173
```

| Entry | Time | Reason |
| --- | --- | --- |
| `prose_gate_fixtures` | 13.55 s | the 10 s entry bound |
| `counters_overhead` | 6.27 s | the published floor against the measured cost |
| `harness_cli_test` | 300.06 s | wall bound |
| `harness_cli_sources_test` | 300.02 s | wall bound |
| `base_cli_matrix` | 120.01 s | wall bound |
| `harness_jitter_cli_test` | 300.04 s | wall bound |
| `harness_hunt_cli_test` | 300.08 s | wall bound |
| `example_run` | 300.04 s | wall bound |
| `report_shape_fwq_ftq_sample` | 110.21 s | `docs_fwq_ftq` timed out after 110 seconds |

`counters_overhead` names its own host figure:

```
COUNTERS OVERHEAD FAIL: the published floor stays at or below the median
measured with the two clock reads that bracket each sampling action
(FR-025, FR-026)
clock, syscall (vDSO): published floor median 392.0 ns against 284.0 ns
measured under the identical bracketing
```

`cmake --build build/coverage -t coverage`, first attempt: exit 2. lcov refused
the trace on the data-consistency check named in section 0.

Second attempt with `~/.lcovrc` in place: exit 2, and the gate judged the trace.

```
  source files: 82
  lines.......: 98.6% (17359 of 17601 lines)
  functions...: 97.9% (1821 of 1860 functions)
  branches....: 96.6% (8415 of 8709 branches)
coverage-gate: line coverage is not 100%
```

The gate stops at the line check, so it prints no branch shortfall line. The
branch reading carries 294 positions at taken zero (8709 minus 8415) against
the recorded allowance of 21.

Dark lines by file, from the `DA` records with a zero count in
`build/coverage/coverage.info`:

| File | Dark lines | Lines in trace | Dark branch records |
| --- | --- | --- | --- |
| `source/harness/hunt_cli.cpp` | 82 | 366 | 38 |
| `source/harness/options.cpp` | 74 | 859 | 114 |
| `source/harness/cli.cpp` | 36 | 896 | 44 |
| `source/harness/jitter_probe.cpp` | 36 | 932 | 53 |
| `source/harness/detail/hunt_selection.hpp` | 5 | 50 | 6 |
| `source/harness/hunt_ranks.cpp` | 4 | 125 | 3 |
| `source/harness/report.cpp` | 3 | 672 | 19 |
| `source/counters/linux_pmu/topdown_select.cpp` | 1 | 241 | 25 |
| `source/harness/hunt.cpp` | 1 | 447 | 6 |
| Total | 242 | 17601 | 516 |

The dark set sits in the command-line surfaces. Seven of the nine failures
above are entries that never reached their end, and those entries own the
command-line paths. The same entries read these times in the Release tree of
the same head: `harness_cli_test` 129.61 s, `harness_cli_sources_test` 89.00 s,
`base_cli_matrix` 68.91 s, `harness_jitter_cli_test` 103.55 s,
`harness_hunt_cli_test` 226.54 s, `example_run` 159.67 s,
`report_shape_fwq_ftq_sample` 28.61 s. The instrumented build
(`-Og --coverage -fno-inline -fkeep-inline-functions`) runs them past the 300 s
and 120 s bounds on this host.

Quieter rerun of those seven entries, `-j 2`, load average 1.94 before the run:
`harness_cli_sources_test` and `report_shape_fwq_ftq_sample` passed, and
`harness_cli_test`, `base_cli_matrix`, `harness_jitter_cli_test`,
`harness_hunt_cli_test` and `example_run` timed out again. The five stand
reported as a host and toolchain limit of this box, and the load of a second
session explains the other two.

## 4. Sanitizer and thread trees at `6025a71f`

`cmake --preset=ci-sanitize`: exit 0.
`cmake --build build/sanitize -j 24`: exit 0.
`ctest --test-dir build/sanitize --output-on-failure -j 24`: exit 8.

```
97% tests passed, 6 tests failed out of 173
Total Test time (real) = 730.29 sec
The following tests FAILED:
     70 - harness_cli_test (Timeout)                        slow
     74 - harness_cli_sources_test (Timeout)                quick
     77 - base_cli_matrix (Timeout)                         quick
     84 - harness_jitter_cli_test (Timeout)                 quick
    139 - harness_hunt_cli_test (Timeout)                   quick
    164 - example_run (Timeout)                             quick
```

No `AddressSanitizer` line, no `runtime error` line, no `SUMMARY:` line and no
`SEGV` line stands in the log. `prose_gate_fixtures` passed there in 12.01 s.

`CXX=clang++ cmake --preset=ci-tsan`: exit 0, clang `23.1.3`.
`cmake --build build/tsan -j 24`: exit 0.
`ctest --test-dir build/tsan --output-on-failure -j 24`: exit 8.

```
97% tests passed, 5 tests failed out of 174
Total Test time (real) = 699.29 sec
The following tests FAILED:
     30 - prose_gate_fixtures (Failed)                      quick
     80 - base_cli_matrix (Timeout)                         quick
     86 - harness_jitter_cli_test (Timeout)                 quick
    162 - report_shape_fwq_ftq_sample (Failed)              quick
    165 - example_run (Timeout)                             quick
```

No `WARNING: ThreadSanitizer` line and no `SUMMARY: ThreadSanitizer` line
stands in that log. The two non-timeout failures read:

```
VERDICT  name=timing.whole-tree  check=timing  expected=exit:0|1 wall<10.0s
  observed=exit:0 wall:10.058s prose-lint: 979 sources, 183585 units examined,
  0 findings, 0 skipped  FAIL
$HOME/sgng-045-accept/build/tsan/example/docs_fwq_ftq: the run failed:
  Command '[.../build/tsan/example/docs_fwq_ftq]' timed out after 110 seconds
```

## 5. The five tools of SC-005 at `6025a71f`

| Command | Exit | Decisive lines |
| --- | --- | --- |
| `python3 tools/prose/prose_gate.py --base origin/master` | 0 | `prose-lint: 56 sources, 3728 units examined, 0 findings, 0 skipped` |
| `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | 0 | clang-format `18.1.8`, the version CI pins |
| `cmake -P cmake/spell.cmake` | 0 | codespell `2.4.3`, `Used config files: 1: .codespellrc` |
| `python3 tools/docs/example_drift.py` | 0 | no difference printed |
| `cmake --build build -t dbc-gate` | 2, then 0 | first attempt: `dbc gates require PyYAML (install python3-yaml): No module named 'yaml'`; with the `~/scratch/speedgun-ng-045` venv on `PYTHONPATH`: `doc-gate: 345 interfaces, 0 gaps`, `pair-gate: 345 interfaces, 0 gaps` |

## 6. Marker count by file

`git grep -c LCOV_EXCL -- source include`: exit 0, 84 markers.

| File | Markers |
| --- | --- |
| `source/counters/clock_provider.cpp` | 17 |
| `source/counters/plan.cpp` | 13 |
| `source/counters/system.cpp` | 12 |
| `source/counters/linux_pmu/group_io.cpp` | 11 |
| `source/counters/linux_pmu/fast_read.cpp` | 7 |
| `source/counters/fold.cpp` | 5 |
| `include/speedgun-ng/dbc.hpp` | 4 |
| `source/counters/linux_pmu/provider.cpp` | 4 |
| `source/counters/linux_pmu/table_parse.cpp` | 4 |
| `include/speedgun-ng/counters_provider.hpp` | 2 |
| `source/counters/detail/core.hpp` | 2 |
| `source/simulation/marker.cpp` | 2 |
| `source/counters/linux_pmu/encode.cpp` | 1 |

The count matches the thrawn record at `d214be37`.
