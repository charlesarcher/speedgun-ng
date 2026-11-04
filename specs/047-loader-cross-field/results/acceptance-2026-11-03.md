# Acceptance run: the loader pass-pair rule (specs/047-loader-cross-field)

Run date: 2026-11-03. Host: chilly. Kernel: `Linux 7.3.1-1-cachyos x86_64`.
The gate logs of this run sit machine-local at `build/logs/`, which the repository
ignores; the record names the command and the count behind every figure below.
Machine: AMD Ryzen 9 7950X3D, 16 cores, 32 threads. Toolchain: gcc 16.2.1
(20260810), clang-format 18.1.8 pinned by `FORMAT_COMMAND`, cmake 4.4.4, lcov
2.x from `~/.local/share/sg-tools/lcov2`. Base: `b1dd7670` (`master`). The merged
lane stands at `561095df` in `master`, 14 commits over the base (`db91c935` ..
`561095df`). PR #75 carries 9 record commits over `561095df`.

The sha measured on chilly for the suite, sanitizer and coverage readings is a lane
head, and it is not the merged tree: `2ffba1dd` and `76e76667` before the merge, and
`bc6399a6` after it. The merged tree is `561095df`, and the master reading of PM-002
runs there.

## The lane

At `561095df`: 20 files changed, 3240 insertions, 17 deletions. Source and tests:
5 files, 603 insertions. The reader change is 48 lines at the end of `readEntry` in
`source/harness/json_load.cpp`. The tests add nine functions to
`test/source/harness_json_load_test.cpp` and one to
`test/source/harness_compare_cli_test.cpp`. Version `0.30.1`, schema `1.13`,
harness `SOVERSION 20`, counters archive `SOVERSION 5`: the patch move touches
five sites, and no archive number and no schema minor moves.

The 18 commits carry two git identities: author `OmO <omo@speedgun-ng.local>` on all
18, and the committer identity of the merge (not OmO) on 17 with `OmO` on 1.
The Constraints block of `spec.md` binds the author identity. The committer of the
17 follows the standing message rewrite that shortened two titles and cleared one
prose finding, and the merged history carries that rewrite.

The lane merged into `master` at `561095df` through PR #74, rebased. The three
record commits that followed the merge ride PR #75, and they change no source,
test, build file or documentation page.

## The red wave, then green

With the reader absent, `./build/test/harness_json_load_test` printed `13 checks
failed`, and every line named one of the five refusal cases. The compare command
answered two failures. The four accepted-form cases stood green before the
change. With the reader present, both binaries print `ok`: 36 cases in the load
test, counted as 27 base calls plus nine new ones in the `main` call list, and 38
cases in the compare test.

## The suites

| Tree | Command | Tests | Passed | Failed | Skipped |
| --- | --- | --- | --- | --- | --- |
| `build` (release) | `ctest --test-dir build --output-on-failure` | 178 | 176 | 1 | 1 |
| `build`, pinned | `taskset -c 0,1 ctest --test-dir build --output-on-failure` | 178 | 176 | 1 | 1 |
| `build/sanitize` (ASan, UBSan) | `ctest --test-dir build/sanitize -j 1 --output-on-failure --no-tests=error` | 173 | 171 | 1 | 1 |
| `build/tsan` (ThreadSanitizer) | `ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error` | 174 | 171 | 2 | 1 |
| `build/coverage` | `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | 173 | 171 | 1 | 1 |

The release tree reads the same at the head as at the base: the same one entry
fails, `harness_jitter_window_test`, on the same check,
`theSeriesRoundTripsThroughLoadReport`, and the same entry skips,
`harness_topdown_run_test`, which needs a PMU. The sanitizer trees add nothing
to that list. The tsan tree adds one entry, named in the host conditions below.

The readings stand against three verify clauses, and each line carries its base
reading. The full and the pinned `ctest` each answer 176 passed, 1 failed and 1
skipped of 178 with exit 8, where the clause asks exit 0; the base tree answers the
same row. The ASan and UBSan suite answers 171 of 173 with one failure, and the
tsan suite answers 171 of 174 with two failures and one race at
`jitter_probe.cpp:708 in seriesFigures`; the base tsan tree answers the same two
failures and the same race. The coverage clause asks `json_load.cpp` at 100 percent
and the union shortfall at 21 rows: the file reads 99.2 percent of 2194 lines with
its 81 dark lines standing at the base, and the union has no measurement point.

Three re-runs on a quiet host, the load average 1.27 at the end, read the same:
the full suite, the pinned suite, and a second pinned run each answer 177 of 178
with the one failure at `harness_jitter_window_test` on
`theSeriesRoundTripsThroughLoadReport`, and ctest exits 8. The pinned run does not
reach exit 0 on this host, and the base tree answers the same row.

## The gates

| Gate | Command | Exit |
| --- | --- | --- |
| Lint, pinned formatter | `FORMAT_COMMAND=$HOME/.local/bin/clang-format cmake -P cmake/lint.cmake` | 0 |
| Spell | `cmake -P cmake/spell.cmake` | 0 |
| Example drift | `python3 tools/docs/example_drift.py .` | 0 |
| Prose, over the range | `python3 tools/prose/prose_gate.py --check all --mode range --base origin/master` | 0, 15 sources |
| Prose, over the feature tree | `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/047-loader-cross-field` | 0 |
| DBC doc gate and pair gate | `cmake --build build/dev --target dbc-gate` | 0, 345 interfaces, 0 gaps |
| clang-tidy and cppcheck | `cmake --build build/dev -j 24` | 0 |

The formatter fix runs as `cmake -DFIX=YES -P cmake/lint.cmake`. The documented
`FIX=YES cmake -P cmake/lint.cmake` sets an environment variable, and CMake
script mode reads only `$ENV{}`, so the fix does nothing and the gate keeps
failing.

## The coverage

The measurement point is the coverage job's union in CI. The host reading sits
here as a local reading of the same tree.

| Reading | Lines | Functions | Branches |
| --- | --- | --- | --- |
| Base `b1dd7670`, host | 99.5% (17485 of 17566) | 97.3% (1804 of 1854) | 98.8% (8593 of 8701) |
| Head, host | 99.5% (17519 of 17600) | 97.3% (1804 of 1854) | 98.8% (8635 of 8743) |
| Head, `source/harness/json_load.cpp` | 99.2% (2194 lines) | 100.0% (178 of 178) | see below |

The lane adds 34 instrumented lines and 42 branches, and the uncovered counts
stand unchanged: 81 lines, 50 functions, 108 branches. Every line and every
branch the lane adds is covered. The branch shortfall of 108 rows is the standing
host figure at the base, and the gate's allowance is 21 rows
(`tools/dbc/coverage_gate.sh:102`). D-12 of the spec carries the written
justification for that allowance against Principles VI and VIII.

The host gate exits 3 at the head: `lcov: ERROR: (inconsistent)
source/harness/runner.cpp:707: function ... is hit but no contained lines are
hit`, and the gate reads the tracefile with no ignore class. The capture target
carries the project's own three classes (`cmake/coverage.cmake`) and writes the
tracefile; the gate's own read then stops on that record. A coverage tree built
at `b1dd7670` answers the same way on this host: the same record at the same
line, the same exit 3, and the same uncovered counts of 81 lines, 50 functions
and 108 branches. Clearing that record needs an ignore class the spec bars this
lane from adding, and `runner.cpp` stands untouched by this lane, so the record
stands as a host condition and the union figure stays unavailable at base and
head. The figure this lane can carry is its own delta, and it is zero rows. The two
standing conditions stand filed: #76 for the `lcov` record at the gate's read, and
#77 for the jitter row that stops the suite at the base.

## The matrix

Run `41727692423` reads at the head, and the five stopped jobs ran a second time.
These jobs pass on both readings: `lint`, `prose-lint`, `dbc-gate`,
`shared-audit`, `downstream-consumer`, `consumer-release`, and all three
`static-analysis` shards.

The stopped jobs read on suite entries, and no loader unit and no compare unit
stands among them: `harness_json_load_test` and `harness_compare_cli_test` pass
in every job of both readings.

| Job | First reading | Second reading |
| --- | --- | --- |
| `test` | `counters_pmu_provider_test`, `harness_jitter_window_test`, `example_run` | `harness_jitter_window_test` |
| `test-rocky` | `harness_jitter_window_test` | `harness_jitter_window_test` |
| `sanitize` | `counters_pmu_provider_test`, `harness_jitter_window_test`, `harness_fwq_ftq_test`, `example_run` | `harness_jitter_window_test`, `harness_fwq_ftq_test` |
| `tsan` | `counters_pmu_provider_test`, `harness_jitter_window_test`, `harness_jitter_cli_test` | the same three, plus `example_run` |
| `coverage` | `harness_jitter_window_test` | `harness_jitter_window_test` |

The base reads the same set on the same matrix: run `41580787441` at `b1dd7670`
stops on `harness_jitter_window_test` in all five jobs, on
`harness_jitter_cli_test` in `tsan`, and on `harness_fwq_ftq_test` in `sanitize`.
The two entries the head adds over that set are PMU-permission rows on the
runners: `counters_pmu_provider_test` answers `the winning entry describes
itself with the kernel's text`, and `example_run` answers `docs_metric_passes`
with `counter 'cpu/cache-misses' is not countable on this host: the catalog
reports permission_blocked`. Both move between jobs across the two readings, and
each passes in a job where the other reading stops it.

Run `41740583420` reads at the head after the records landed, and reads the same
job set: the seven audit and analysis jobs pass, and `test`, `test-rocky`,
`sanitize`, `tsan` and `coverage` stop on suite entries.

Run `41772312542` reads at the record head `bc6399a6`, and reads the same set: the
nine lint, audit and analysis jobs pass, `docs` stands skipped, and the five suite
jobs stop on standing entries. `harness_jitter_window_test` fails in all five;
`loop_shape` joins in `test`; `harness_fwq_ftq_test` joins in `sanitize`; `tsan`
answers `harness_jitter_cli_test` and the race at `jitter_probe.cpp:708`, the same
race the host tree answers; and the coverage job's upload step answers `No files
were found with the provided path: build/coverage/coverage.info`. No loader unit
and no compare unit stands among them.

The coverage job's capture does not run on this matrix, at the base or at the
head: the job stops on the standing jitter row, and its upload step answers `No
files were found with the provided path: build/coverage/coverage.info`. The
union figure therefore stands unavailable from CI for both trees, and the host
reading above carries the coverage evidence for the merge.

## The host conditions

- `harness_jitter_window_test` fails on `theSeriesRoundTripsThroughLoadReport` in
  every tree, at the base and at the head, in the release build and under
  sanitizers. The row folds a jitter series and reads it back, and the host load
  moves it.
- `harness_topdown_run_test` skips: the host PMU path stands unavailable.
- The tsan tree fails one more entry, `harness_jitter_cli_test`, on the row `a
  cpu outside the child's mask stands rejected`, and reports one data race:
  `jitter_probe.cpp:708 in seriesFigures`. Five runs of that binary under tsan
  reproduce both, five of five. The race is a read of the probe's per-cpu state
  by the main thread through `seriesStatsOf` at `cli.cpp:1384` against a write by
  the probe thread at `jitter_probe.cpp:350`. `git diff b1dd7670..HEAD` over
  `jitter_probe.cpp`, `cli.cpp`, `execution_context.cpp`,
  `execution_context.hpp` and `harness_jitter_cli_test.cpp` is empty, and the
  lane's only source change stands on neither stack. The base reading confirms it:
  a tsan tree built at `b1dd7670` on this host answers 2 failures of 174 and one
  race, the same `jitter_probe.cpp:708 in seriesFigures`.
- The host pins: `FORMAT_COMMAND` at clang-format 18.1.8, `PERL5LIB` for the lcov
  Perl modules, `-DPython3_EXECUTABLE=/usr/bin/python3` for the dbc gate's
  PyYAML, and `-D LCOV_EXECUTABLE` and `-D GENHTML_EXECUTABLE` at the lcov 2.x
  that carries `geninfo`. A `~/.local/bin/lcov` shim on `PATH` resolves at
  configure time and cannot exec `geninfo`.
- The load test and the compare test write scratch documents in the working
  directory. The suite runs them in their own directories; a direct run needs a
  scratch directory.
