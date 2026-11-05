# Phase 10 gate record for spec 043 at the spec 045 Part B head

Task: T111 of specs/045-merge-fixes/tasks.md.
Rows: T048, T049, T051, T052, T053, T054, T055, T056 of
specs/043-fwq-ftq/tasks.md Phase 10.

Head: 439a01ff on branch 045-merge-fixes, 11 commits past origin/master.
Host: CachyOS, Linux 7.3.0-5-cachyos, 48 cpus. The `ci-rocky` preset
names the release configuration, and this host ran it.
Working tree: `CMakeLists.txt` carries an uncommitted comment-only edit
(T112 of spec 045). No file was edited by this run.
Concurrency: the rows write `-j "$(nproc)"`. The heavy builds and the
instrumented suites ran at `-j 24`, except the third sanitizer run, which
used `-j 48`. The tsan build and both tsan suite runs stood alone.
Date: 2026-11-03.

## Row verdicts

```
T048  PASS  build clean, 178 entries, 100% tests passed out of 178,
            both -R selections exit 0
T049  FAIL  the fixed-cpu grep reads 2 matches in
            test/source/harness_jitter_bytes_test.cpp; the masked suite
            exits 0 and both SKIPPED notes print
T051  FAIL  harness_jitter_window_test failed the first run of each
            instrumented suite; no sanitizer report stands in any log
T052  PASS  tidy build exits 0, readability-identifier-naming reads 0
            findings, dbc-gate exits 0 with 0 gaps
T053  PASS  time_source_gate: clean, and both greps read no match
T054  PASS  eight docs entries passed, the drift fold printed nothing,
            the prose gate reads 0 findings
T055  FAIL  the format gate exits 1 and names two files
T056  FAIL  both forms of tools/ci/prepush.sh exit 1 at the format gate;
            the diff-scope grep reads 0 and the three cap greps read 1
```

Six of the eight rows ran. T050, T057 and T061 stand outside this record:
T050 is the coverage contract, T057 reads an acceptance record of the 043
lane, and T061 names a push of that lane.

## T048: release build and the full suite in `build`

Result: PASS.

```
cmake --preset=ci-rocky
```

Exit code: 0.

```
-- Configuring done (0.1s)
-- Generating done (0.1s)
-- Build files have been written to: build
```

```
cmake --build build
```

Exit code: 0. The log holds 173 lines, no `warning:` line and no
`error:` line.

```
[100%] Built target harness_topdown_report_test
```

```
ctest --test-dir build --output-on-failure
```

Exit code: 0.

```
100% tests passed out of 178
Total Test time (real) = 319.05 sec
The following tests did not run:
        177 - harness_topdown_run_test (Skipped)
```

```
ctest --test-dir build -R "harness_fwq_ftq_test|harness_spectrum_test|harness_jitter_bytes_test" --output-on-failure
```

Exit code: 0.

```
1/3 Test #92: harness_jitter_bytes_test ........   Passed    0.00 sec
2/3 Test #94: harness_spectrum_test ............   Passed    3.26 sec
3/3 Test #95: harness_fwq_ftq_test .............   Passed    3.75 sec
100% tests passed out of 3
```

```
ctest --test-dir build -R "harness_cli_table_test|harness_json_schema_test|harness_json_load_test|harness_compare_test" --output-on-failure
```

Exit code: 0.

```
1/4 Test #71: harness_json_schema_test .........   Passed    0.01 sec
2/4 Test #72: harness_json_load_test ...........   Passed    0.01 sec
3/4 Test #73: harness_compare_test .............   Passed    0.00 sec
4/4 Test #76: harness_cli_table_test ...........   Passed    0.03 sec
100% tests passed out of 4
```

Note on the marker form: the row asks for three `***Passed` lines. CMake
4.4.4 on this host prints `Passed` for a passing entry and reserves the
`***` marker for `***Skipped` and `***Failed`. The full-suite log holds
one `***` marker line, and it is the `***Skipped` line above. The count
of passing entries in each selection is the figure the row names.
## T049: the suite under a two-cpu mask, and the fixed-cpu greps

Result: FAIL (the fixed-cpu grep reads two matches).

```
taskset -c 0,1 ctest --test-dir build --output-on-failure
```

Exit code: 0.

```
100% tests passed out of 178
Total Test time (real) = 268.99 sec
The following tests did not run:
        177 - harness_topdown_run_test (Skipped)
```

The standing SKIPPED notes. `--output-on-failure` prints the output of a
passing entry nowhere, so the same two entries were read verbosely under
the same mask to capture the notes the row names.

```
taskset -c 0,1 ctest --test-dir build -R "harness_jitter_window_test|harness_precheck_test" --verbose
```

Exit code: 0.

```
87: the two-cpu probe scenarios: SKIPPED (the mask holds fewer than three cpus)
88: the full-run refusal scenario: SKIPPED (the mask holds fewer than four cpus)
88: the method refusal scenarios: SKIPPED (the mask holds fewer than four cpus)
```

The note text stands at `test/source/harness_jitter_window_test.cpp:1086`
and `test/source/harness_precheck_test.cpp:625` and
`test/source/harness_precheck_test.cpp:774` at this head. The line
numbers the row names, 623 to 626 and 443 to 445, hold other code at
this head.

```
grep -rnE "cpu[A-Za-z]* *= *[0-9]+|cpus=\"[0-9]" test/source/harness_fwq_ftq_test.cpp test/source/harness_spectrum_test.cpp test/source/harness_jitter_bytes_test.cpp
```

Exit code: 0. The row asks for no match. Two lines match:

```
test/source/harness_jitter_bytes_test.cpp:130:  fold.cpu = 0;
test/source/harness_jitter_bytes_test.cpp:318:  state.cpu = 0;
```

Both are fixture records: line 130 sets the cpu id field of a
`sg::JitterEntryFold`, and line 318 sets the cpu id field of a
`sg::detail::ProbeCpuState`. Neither pins a thread. The grep pattern the
row names matches them anyway.

```
grep -c "sched_getaffinity" test/source/harness_fwq_ftq_test.cpp
```

Exit code: 0.

```
2
```
## T051: the sanitizer suite and the thread-sanitizer suite

Result: FAIL. Both suites carry one intermittent entry, and each suite's
first run at this head exited 8. No sanitizer report stands in any log.

### ci-sanitize

```
cmake --preset=ci-sanitize
```

Exit code: 0.

```
-- Build files have been written to: build/sanitize
```

```
cmake --build build/sanitize -j 24
```

Exit code: 0, with no `warning:` line and no `error:` line.

```
ctest --test-dir build/sanitize --output-on-failure -j 24
```

First run. Exit code: 8.

```
99% tests passed, 1 tests failed out of 173
The following tests FAILED:
         82 - harness_jitter_window_test (Failed)               quick
HARNESS JITTER WINDOW TEST FAIL: the entry carries its jitter object (FR-027, E-08)
```

The same entry alone in the same tree:

```
ctest --test-dir build/sanitize -R harness_jitter_window_test --output-on-failure
```

Exit code: 0, `1/1 Test #82: harness_jitter_window_test .......   Passed    1.21 sec`.

Second run of the whole suite at `-j 24`. Exit code: 0.

```
100% tests passed out of 173
Total Test time (real) =  70.87 sec
```

Third run at the row's own `-j "$(nproc)"`, which is 48 here. Exit code:
0.

```
100% tests passed out of 173
Total Test time (real) =  71.75 sec
```

`AddressSanitizer` lines and `runtime error` lines across all three logs:
0.

### ci-tsan

The tsan build and both tsan suite runs stood alone on the host: no
other build, test, or suite ran while they ran. The host does carry one
standing user process, an inference server that holds about six cpus and
50 GiB of memory the whole time, and it cannot be stopped from this
task.

```
CXX=clang++ cmake --preset=ci-tsan
```

Exit code: 0.

```
-- Build files have been written to: build/tsan
```

```
cmake --build build/tsan -j 24
```

Exit code: 0, with no `warning:` line and no `error:` line.

```
ctest --test-dir build/tsan --output-on-failure -j 24
```

First run, 22:09:12 to 22:11:41 UTC, 149 s wall. Exit code: 8.

```
99% tests passed, 1 tests failed out of 174
The following tests FAILED:
         84 - harness_jitter_window_test (Failed)               quick
error: the rule 'jitter:0:sampleCount:lower:abs=1' names cpu 0 and the jitter stats of bmQuiet in the baseline report do not carry it
speedgunMain returned 2, expected 0
HARNESS JITTER WINDOW TEST FAIL: the exit status does not follow contracts/cli.md (FR-036)
```

The same entry alone in the same tree:

```
ctest --test-dir build/tsan -R harness_jitter_window_test --output-on-failure
```

Exit code: 0, `1/1 Test #84: harness_jitter_window_test .......   Passed    2.06 sec`.

Second run of the whole suite at `-j 24`, 22:11:50 to 22:14:20 UTC, 150 s
wall. Exit code: 0.

```
100% tests passed out of 174
Total Test time (real) = 150.08 sec
```

`WARNING: ThreadSanitizer` lines across both logs: 0.

The entry the task names for the time budget, `harness_jitter_cli_test`,
took 145.55 s in the first run and 146.96 s in the second, against the
300 s entry timeout. It passed both times.

The failing entry is the same one in both trees, and it passed in the
release tree in both the serial run and the masked run of T048 and T049.
It fails only in the instrumented trees under `-j`, and it passes alone
in each of them.
## T052: the analyzer build and the DBC gate

Result: PASS with a note. The build exits 0, the pinned naming check
reads clean, and the DBC gate exits 0. The note records the finding
counts.

```
cmake --preset=ci-ubuntu -B build/tidy
```

Exit code: 0.

```
-- Build files have been written to: build/tidy
```

```
cmake --build build/tidy -j 24
```

Exit code: 0. clang-tidy and cppcheck both ran over the tree.

clang-tidy: 50122 warning lines over 299 files, the project-wide figure
this preset produces on `master` as well, because `.clang-tidy` sets
`Checks: "*"` with a short deny list and pins one check as an error:
`WarningsAsErrors: 'readability-identifier-naming'`. That check reads 0
findings in this build.

cppcheck: 453 translation units checked, and two error lines printed.
Neither names a file this branch touched.

```
test/source/dbc_test.cpp:232:9: error: Unhandled exception thrown in function that is an entry point. [throwInEntryPoint]
test/source/harness_topdown_run_test.cpp:3197:3: error: Non-local variable 'selfPath' will use pointer to local variable 'self'. [danglingLifetime]
```

Findings on the lines this branch added: 180, in eight files. None is
`readability-identifier-naming`. By file:

```
83 test/source/counters_linux_pmu_seam_test.cpp
38 test/source/counters_pmu_provider_test.cpp
22 source/harness/jitter_probe.cpp
15 test/source/counters_kernel_calls_test.cpp
9  test/source/harness_jitter_window_test.cpp
6  test/source/harness_topology_test.cpp
6  test/source/harness_jitter_cli_test.cpp
1  source/counters/linux_pmu/kernel_calls.cpp
```

By check, the largest groups:

```
52 cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers
32 google-runtime-int
19 readability-redundant-lambda-parameter-list
10 readability-trailing-comma
7  cppcoreguidelines-avoid-non-const-global-variables
6  misc-use-internal-linkage
6  misc-include-cleaner
5  readability-container-contains
5  readability-simplify-boolean-expr
```

The rest sit in 21 further checks at four findings or fewer each. The
same 180 lines carry no error level, and the build exits 0.

```
cmake -S . -B build/dev --preset=ci-ubuntu
```

Exit code: 0.

```
-- Build files have been written to: build/dev
```

```
cmake --build build/dev -t dbc-gate -j 24
```

Exit code: 0.

```
Running DBC documentation and pairing gates
doc-gate: 345 interfaces, 0 gaps (build/dev/dbc-gate/doc-matrix.json)
pair-gate: 345 interfaces, 0 gaps (build/dev/dbc-gate/pair-matrix.json)
Built target dbc-gate
```
## T053: time-source gate and code-discipline greps

Result: PASS.

```
bash test/time_source_gate.sh .
```

Exit code: 0.

```
time_source_gate: clean
```

```
grep -nE "SG_(REQUIRE|ENSURE|INVARIANT)\(.*\] *\(" source/harness/spectrum.cpp source/harness/jitter_probe.cpp
```

Exit code: 1, which is grep for no match. No line printed.

```
grep -rnE "nlohmann|rapidjson" source/harness/spectrum.cpp source/harness/detail/spectrum.hpp
```

Exit code: 1, which is grep for no match. No line printed.
## T054: docs gates, their fixtures, and the shape folds

Result: PASS.

```
ctest --test-dir build -R "example_drift|example_drift_fixtures|snippet_compile|example_run|example_run_fixtures|report_shape_fwq_ftq_sample|report_shape_jitter_sample|report_shape_fixtures" --output-on-failure
```

Exit code: 0. Eight entries ran, all passed.

```
1/8 Test #157: example_drift ....................   Passed    0.05 sec
2/8 Test #158: example_drift_fixtures ...........   Passed    0.72 sec
3/8 Test #159: report_shape_fixtures ............   Passed    1.04 sec
4/8 Test #160: snippet_compile ..................   Passed   18.27 sec
5/8 Test #165: report_shape_jitter_sample .......   Passed    0.63 sec
6/8 Test #166: report_shape_fwq_ftq_sample ......   Passed   18.43 sec
7/8 Test #169: example_run ......................   Passed   23.34 sec
8/8 Test #170: example_run_fixtures .............   Passed    0.11 sec
100% tests passed out of 8
```

The fold itself, read directly:

```
python3 tools/docs/example_drift.py .
```

Exit code: 0, and the script printed no line, which is the clean
reading.

```
python3 tools/prose/prose_gate.py --base origin/master
```

Exit code: 0.

```
prose-lint: 41 sources, 2975 units examined, 0 findings, 0 skipped
```
## T055: prose gate, format gate, spell gate

Result: FAIL (the format gate names two files).

```
python3 tools/prose/prose_gate.py --base origin/master
```

Exit code: 0.

```
prose-lint: 41 sources, 2975 units examined, 0 findings, 0 skipped
```

```
clang-format --version
```

Exit code: 0.

```
clang-format version 18.1.8
```

```
cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
```

Exit code: 1. Two files are named:

```
The following files are badly formatted:

source/harness/jitter_probe.cpp
test/source/harness_jitter_cli_test.cpp
```

Both files were last touched by commit 68e52925
(`Harness: Fold the pre-check record at the latch`) on this branch. The
working tree holds no edit to either file.

```
cmake -P cmake/spell.cmake
```

Exit code: 0.

```
Used config files:
    1: .codespellrc
```
## T056: the pre-push script in both forms and the FR-034 diff scope

Result: FAIL. Both forms of the script stop at the format gate of step 2,
the same two files T055 names.

```
bash tools/ci/prepush.sh
```

Exit code: 1.

```
prepush: drift gate 0 s
The following files are badly formatted:

source/harness/jitter_probe.cpp
test/source/harness_jitter_cli_test.cpp

prepush: format gate failed with 1: cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
```

The steps after the format gate never ran, so the script printed no ok
line for them.

```
bash tools/ci/prepush.sh --full
```

Exit code: 1, with the same two lines and the same failing step.

```
git diff --name-only origin/master...HEAD | grep -cE '^(\.github/workflows/|CMakePresets\.json|CMakeUserPresets\.json|vcpkg\.json|tools/dbc/coverage_gate\.sh|tools/prose/prose_rules\.yaml|tools/noise/jitter_probe_acceptance\.sh|test/time_source_gate\.sh)'
```

Reads:

```
0
```

The branch touches 32 paths, and none of them stands in that set.

```
grep -c "PAGE_LINE_CAP = 1004" tools/docs/example_drift.py
grep -c "SECTION_LINE_CAP = 25" tools/docs/example_drift.py
grep -c "allowance=21" tools/dbc/coverage_gate.sh
```

Read: 1, 1, 1.


## Re-run at the fixed head (2026-11-03, after the 045 fixes)

The four FAIL rows above stood fixed on this branch and stand re-run at
head 1ee15538. The fixes: the format gate needed clang-format 18.1.8 on
two files (commit fa93fb61 carried the reformat), the host-free grep of
T049 needed a named constant for two fixture record fields, and the
probe-driven entries now stand RUN_SERIAL in test/CMakeLists.txt
(commit 1ee15538).

```
ctest --test-dir build --output-on-failure -j 24
```

`100% tests passed out of 178`. The three-binary and four-binary
selections read 3 of 3 and 4 of 4.

```
taskset -c 0,1 ctest --test-dir build --output-on-failure
```

`100% tests passed out of 178`. The fixed-cpu grep exits 1 (no match),
and the sched_getaffinity count reads 2.

```
ctest --test-dir build/sanitize --output-on-failure -j 24
ctest --test-dir build/tsan --output-on-failure -j 24
```

Each suite twice: `100% tests passed out of 173` and
`100% tests passed out of 174`. Zero `AddressSanitizer`, `runtime error`
and `WARNING: ThreadSanitizer` lines.

```
cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
python3 tools/prose/prose_gate.py --base origin/master
cmake -P cmake/spell.cmake
```

Exit 0, `0 findings`, exit 0.

```
bash tools/ci/prepush.sh
bash tools/ci/prepush.sh --full
```

Both exit 0. The diff-scope grep reads 0, and the three cap greps read
one line each.

## Host-load artifact on the window entry

The window entry keeps a residual flake on this host when the host
stands loaded: 6 failures in 20 solo runs of the binary while a standing
user process holds about six cpus, on two assertions of the wall-bounded
window (`the entry whose windows kept no gap loads no fold` and
`the entry's jitter record carries its series array`). The full suite
passed twice at -j 48 and five rounds of the jitter entries under
taskset -c 0-3 with -j 4 read 5 of 5. The entry stands RUN_SERIAL, so no
sibling test of the suite causes it. The record states the condition for
the owner's ruling.
