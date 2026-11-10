# The cheap gates (T028 to T032)

Lane head `a3c465ac71e73f5fd84c674bfdcadaa1164fea26`, the built head
that `results/commits.md` records. Host `Gary` (`Linux 7.3.3-1-cachyos
x86_64`), `cmake version 4.4.5`, `Python 3.14.9`, `clang-format version
18.1.8`, the version the lint job pins at `.github/workflows/ci.yml:34`.
No build was run for these five gates, and none of them needs a tree.
Each row names the task, the command exactly as the task writes it, the
verdict line copied from that command's own output, and the log path
holding the whole output.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029.log` |
| T030 prose, commit range | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake` | `prose-lint: 47 sources, 4665 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a.log` |
| T030 prose, lane tree | `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | `prose-lint: 16 sources, 3920 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030b.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032.log` |

Six commands, six exits 0, no finding in a tracked file.

T031 carries three readings the task names: the option-row table at
`tools/docs/example_drift.py:567` to `:581`, the page-line cap at
`tools/docs/example_drift.py:256`, and the forbidden-class scan of the
two User-guide pages. The four ctest entries `example_drift`,
`snippet_compile`, `example_run` and `report_shape` ride the T024 suite
row: that suite log `/tmp/sg051-t024-suite.log` stood absent at this
read, so no per-entry line is copied here.

## The cheap gates re-read at the pin-move head (T028 to T032)

The six command rows above, read at head
`a3c465ac71e73f5fd84c674bfdcadaa1164fea26`, stand superseded. T043 moved
the two standing version pins to 0.35.0, so the built head for every
wave 7 read from here on is `b0cd97e2e23f605724d3d2593490df60a71a3cd8`,
copied from `results/commits.md` and titled `test: Move the version rows
to 0.35.0`. The same five tasks ran their commands again at that head,
into logs carrying an `r2` suffix. The working tree matches that head
over `source`, `test`, `include`, `docs`, `README.md`, `cmake` and
`CMakeLists.txt`, and the T043 verify read `grep -rn '0\.34\.0' test
example docs README.md tools cmake include source` prints nothing. Host
reads match the block above: `Gary`, `cmake version 4.4.5`, `Python
3.14.9`, `clang-format version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r2.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029-r2.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 52 sources, 4754 units examined, 0 findings, 0 skipped`, then `prose-lint: 17 sources, 3974 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r2.log`, `/tmp/sg051-t030b-r2.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r2.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r2.log` |

The counts moved with the head. The range read over the same base
`0fce6651` went from 47 sources and 4665 units to 52 sources and 4754
units as the lane's later commits entered the range, and the tree read
went from 16 sources to 17 because `results/gates.md` now stands
tracked, so this read is the first to hold this record itself to
Principle XI. Both counts answer 0 findings. T031 carries the three
readings the task names, the option-row table at
`tools/docs/example_drift.py:567` to `:581`, the page-line cap at
`tools/docs/example_drift.py:256`, and the forbidden-class scan of the
two User-guide pages. The four ctest entries `example_drift`,
`snippet_compile`, `example_run` and `report_shape` still ride the T024
suite row: `/tmp/sg051-t024-suite.log` existed at this read and stood in
progress with 2 of 192 entries reported, so none of the four had printed
a result line yet.


# The build legs (T024 to T027)

Built head `b0cd97e2e23f605724d3d2593490df60a71a3cd8`, copied from the
section "The built head after the pin carriers moved" of
`results/commits.md`. The checkout these legs ran in reads
`483b42f60df4c6948ff00118c826dd5be5673d8a`, and the commits between the
two heads touch `specs/051-raw-events` paths only, the reading
`git diff --name-only` over the two heads gives: `tasks.md`,
`results/commits.md`, `results/base-readings.md` and this file. Source,
test, build and docs surfaces match the built head. Host `Gary`, 32
cores, one build at a time, this chain the only builder.

Each leg ran in its own shell handle, whole output in the named log, and
the exit figure is that command's own line copied.

## T024, the full suite in the shared `build` tree

Tree `build`, configuration `ci-rocky`: a release developer-mode tree,
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`, `CMAKE_BUILD_TYPE=Release`,
and the analyzer cache variables absent from the cache at the purge
read.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0`, over `Configuring done` and `Generating done` | `/tmp/sg051-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, two test units rebuilt, 0 `warning:` lines | `/tmp/sg051-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 192`, no `Failed` line, 0 `Not Run` lines | `/tmp/sg051-t024-suite-r2.log` |

The count read `ctest --test-dir build -N` closes with
`Total Tests: 192`. The one did-not-run entry is
`190 - harness_topdown_run_test (Skipped)`, the standing skip the base
carries, and the `Not Run` grep over the log answers 0.

Superseded read at the first built head
`a3c465ac71e73f5fd84c674bfdcadaa1164fea26`: the same four commands, and
the suite leg answered `99% tests passed, 2 tests failed out of 192`
over `83 - harness_cli_table_test (Failed)` and
`84 - views_test (Failed)` (`/tmp/sg051-t024-suite.log`). The alone read
of the two entries answers `exit:8` with the two lines
`FAILED: the project version reads 0.34.0 (FR-050)` and
`SPEEDGUN-NG TEST FAIL: pass: the project version reads 0.34.0`
(`/tmp/sg051-t024-tworead.log`), so the read stands on the two standing
version pins, and the pin carriers at the built head above move both
rows.

Second superseded read, one leg of that same suite:
`30 - prose_gate_fixtures (Failed)` over the verdict
`name=work.whole-tree.counts`, `expected=1 runs exit:0|1
sources:1121 units:227149 skipped:0`, observed
`1 runs exits:[0] counters:[(1121, 227115, 0)]`. That assert scans the
live tree twice, and this file gained the 34 units between the two scans
while the cheap-gate worker appended its block. The alone read answers
`exit:0` (`/tmp/sg051-t024-prose-fixture-read2.log`). The suite row
above is the read taken while the tree stood still, the checkout and the
file mtime read identical before and after it.

## T025, the sanitizer leg in `build/sanitize`

Tree `build/sanitize`, configuration `ci-sanitize` with `CXX=clang++`:
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`, `clang version 23.1.4`,
`CMAKE_BUILD_TYPE=Sanitize`, and the sanitizer flags of
`CMAKE_CXX_FLAGS_SANITIZE` at `CMakePresets.json:121` to `:128`. The
`CXX=` stands in the recorded command line because the host GCC cannot
build this leg.

Preparation read: `build/sanitize` stood present from 2026-11-05 as an
older lane's tree with
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`. A cache that already names
a compiler keeps it, so the first run of the recorded configure answered
`exit:0` over a cache naming no Clang: the grep of the leg answered
`/usr/bin/c++`. The stale tree was removed with `rm -rf build/sanitize`
(`exit:0`, empty log `/tmp/sg051-sanitize-rmtree.log`) and the configure
was read again. The three rows below are the read of record, in order.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-sanitize` | `exit:0`, `The CXX compiler identification is Clang 23.1.4` | `/tmp/sg051-sanitize-cfg.log` |
| `cmake --build build/sanitize -j 10` | `exit:0`, 326 objects compiled, the only `warning:` lines are five vendored hwloc rows | `/tmp/sg051-sanitize-build.log` |
| `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 187`, no `Failed` line, 0 `Not Run` lines | `/tmp/sg051-sanitize-suite.log` |

The instrumentation read of `build/sanitize/test/harness_raw_events_test`
counts 179 `__asan` entries through `nm -D`, and that binary carries the
AddressSanitizer runtime statically. The cache read for the leg answers
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`. The verdict greps over
the suite log answer `ERROR: AddressSanitizer` 0, `runtime error:` 0,
`LeakSanitizer` 0, `detected memory leaks` 0, `Not Run` 0 and `Failed`
0. The one did-not-run entry is
`185 - harness_topdown_run_test (Skipped)`.

The tree registers 187 entries against the 192 of the shared `build`
tree. `ctest -N` over both trees names the difference: the five `dbc`
matrix entries the `SPEEDGUN_DBC_MATRIX=ON` of the `ci-rocky` preset
adds (`/tmp/sg051-sanitize-N.log`, `/tmp/sg051-N.log`).

## T026, the thread sanitizer leg in `build/tsan`

Tree `build/tsan`, configuration `ci-tsan` with `CXX=clang++`:
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`, `clang version 23.1.4`,
the thread sanitizer flags of `CMAKE_CXX_FLAGS_RELEASE` carried onto the
Release configuration at `CMakePresets.json:130` to `:141`, and
`SPEEDGUN_DBC_MATRIX=ON`.

Preparation read: `build/tsan` stood present from 2026-11-05 as an older
lane's tree with `CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`. The stale
tree was removed with `rm -rf build/tsan` (`exit:0`, empty log
`/tmp/sg051-tsan-rmtree.log`) so the recorded `CXX=` reaches a cold
cache. The three rows below are the read of record, in order, with the
host idle of other compilers at the `ps` read that opens the leg.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-tsan` | `exit:0`, `The CXX compiler identification is Clang 23.1.4` | `/tmp/sg051-tsan-cfg.log` |
| `cmake --build build/tsan -j 10` | `exit:0`, 322 objects compiled, the only `warning:` lines are five vendored hwloc rows | `/tmp/sg051-tsan-build.log` |
| `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 188`, no `Failed` line, 0 `Not Run` lines, 194.97 sec of wall time | `/tmp/sg051-tsan-suite.log` |

The verdict grep for `WARNING: ThreadSanitizer` over the suite log
answers 0. The instrumentation read of
`build/tsan/test/harness_threads_test` counts 137 `__tsan` entries
through `nm -D`. The one did-not-run entry is
`186 - harness_topdown_run_test (Skipped)`.

`harness_context_barrier_test` passed in that read, so the standing host
flake note, about one run in ten for that entry under a thread sanitizer
on this machine, stands unexercised at this leg: the isolation re-read
the task frames, and the P2 exception clause Principle VIII carries,
stand recorded as the agreed handling and find no row here.

The tree registers 188 entries against the 192 of the shared `build`
tree. `ctest -N` over both trees names the four allocation-operator
units the thread sanitizer guards out, `counters_noalloc_test`,
`harness_argument_test`, `harness_capture_test` and
`harness_jitter_probe_alloc_test`, where the sanitizer runtime owns
those operators (`test/CMakeLists.txt:328` to `:334`, `:1085` to
`:1087`), and the five `dbc` matrix entries ride in through the preset
(`CMakePresets.json:130` to `:141`, `/tmp/sg051-tsan-N.log`).

## T027, the coverage leg in `build/coverage`

Tree `build/coverage`, configuration `ci-coverage`:
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++` over `GNU 16.2.1`,
`CMAKE_BUILD_TYPE=Coverage`, and the capture of
`cmake/coverage.cmake:73` with `--ignore-errors mismatch,negative`. The
disk read gives `/home` 781G free at this leg.

Preparation read: `build/coverage` stood present from 2026-11-05 as an
older lane's tree, the `stat` reading over its `CMakeCache.txt`. The
stale tree was removed with `rm -rf build/coverage` (`exit:0`, empty log
`/tmp/sg051-coverage-rmtree.log`). The five rows below are the read of
record, in order.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-coverage` | `exit:0`, `The CXX compiler identification is GNU 16.2.1` | `/tmp/sg051-coverage-cfg.log` |
| `cmake --build build/coverage -j 10` | `exit:0`, 326 objects, five vendored hwloc `warning:` rows, 294 `.gcno` files | `/tmp/sg051-coverage-build.log` |
| `find build/coverage -name '*.gcda' -delete` | `exit:0` over an empty log: the tree was cold, so the trace names one run | `/tmp/sg051-coverage-gcda.log` |
| `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `exit:0`, `100% tests passed out of 187`, 187 registered, 186 executed, 1 skipped, 0 `Not Run` lines, 576.63 sec of wall time | `/tmp/sg051-coverage-suite.log` |
| `cmake --build build/coverage -t coverage` | `exit:2`, the gate verdict `coverage-gate: line coverage is not 100%` | `/tmp/sg051-coverage-gate.log` |

The red verdict of the coverage target, copied from that log:

```
Summary coverage rate:
  source files: 87
  lines.......: 99.9% (18917 of 18930 lines)
  functions...: 99.0% (1983 of 2003 functions)
  branches....: 99.6% (9937 of 9974 branches)
coverage-gate: line coverage is not 100%
make[3]: *** [CMakeFiles/coverage.dir/build.make:74: CMakeFiles/coverage] Error 1
```

The line arm sits 13 rows under the strict check, 18930 minus 18917.
The trace names them, read through the `DA:` rows of
`build/coverage/coverage.info` that hold a zero hit count:
`source/counters/linux_pmu/provider.cpp` at 891, 892 and 897, the
no-object refusal arm a raw address reaches at a path holding no PMU
device; `source/counters/system.cpp` at 625 and 681; and
`source/harness/runner.cpp` at 1021 and at 1518 to 1525. The gate's own
table rows read `counters/linux_pmu/provider.cpp 99.3% 450`,
`counters/system.cpp 99.4% 315` and `harness/runner.cpp 99.5% 1654`.

The branch arm sits above the recorded allowance as well: 9974 minus
9937 answers 37 rows against the 21 of `tools/dbc/coverage_gate.sh:108`.
The gate exits at the line arm, so the D-14 message of
`tools/dbc/coverage_gate.sh:121` to `:123` never reaches the log.

The verify reads of the leg that answer clean: the marker read answers
84, the ceiling of 84; the new-marker read answers 0, so the lane adds no
marker; and the read over `cmake/coverage.cmake` and
`tools/dbc/coverage_gate.sh` prints no path. The two new units ran green
inside this suite, entry 50 `counters_raw_event_test` and entry 150
`harness_raw_events_test`, which places the 13 rows in arms neither unit
drives.

`genhtml` never reached this run: the target stops at the gate verdict,
so the perl GD host note that `docs/pages/building.md` names finds
nothing to record at this read.

## Skip entries across the build legs

Each leg registers its own count, and one skip stands in all four.

| Leg | Registered | Executed | Skip entry |
| --- | --- | --- | --- |
| T024, `build` | 192 | 191 | `190 - harness_topdown_run_test (Skipped)` |
| T025, `build/sanitize` | 187 | 186 | `185 - harness_topdown_run_test (Skipped)` |
| T026, `build/tsan` | 188 | 187 | `186 - harness_topdown_run_test (Skipped)` |
| T027, `build/coverage` | 187 | 186 | `185 - harness_topdown_run_test (Skipped)` |

The skip carries one reason on this host, the reason the binary states
itself: `HARNESS TOPDOWN RUN TEST SKIP: the identity of this host names
a directory beside the fixture's`
(`test/source/harness_topdown_run_test.cpp:2428`), and the registration
reserves the exit through `SKIP_RETURN_CODE 2`
(`test/CMakeLists.txt:2433`). The alone read of the coverage-tree binary
answers `exit:2` with
`harness_topdown_run_test: 1 of 29 scenarios skipped` over the scenario
`the-deeper-levels-collect-every-node-above-them`
(`/tmp/sg051-coverage-skipread.log`). The base suite carries the same
entry, so the skip reads as a fact about this machine's vendor, the
shape the CI matrix shares. No leg names a `Not Run` entry, the clause
T024 to T027 keep.

## The cheap gates re-read at the coverage-closure head (T028 to T032)

The five rows of the pin-move block above, read at
`b0cd97e2e23f605724d3d2593490df60a71a3cd8`, stand superseded. The T044
closure moved source and tests, so the wave 7 built head moved to
`9033535eb95ed0d1065360ef4c6bd9cae5c8f287`, copied from
`results/commits.md` and titled `test: Drive the raw arms the trace left
dark`, with `bb45fa52 Counters: Witness the stamp and the hex test` one
below it. The same five tasks ran their commands again against that
tree, into logs carrying an `r3` suffix. `git diff --stat` between that
head and `564d7dcd`, the checkout at this read, prints nothing over
`source`, `test`, `include`, `docs`, `README.md`, `cmake` and
`CMakeLists.txt`. Host reads match the blocks above: `Gary`, `cmake
version 4.4.5`, `Python 3.14.9`, `clang-format version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r3.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029-r3.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 55 sources, 5100 units examined, 0 findings, 0 skipped`, then `prose-lint: 17 sources, 4232 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r3.log`, `/tmp/sg051-t030b-r3.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r3.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r3.log` |

The counts moved again. The range read over the same base `0fce6651`
went to 55 sources and 5100 units as the two closure commits entered
the range, and the tree read went to 4232 units over the same 17
sources. T031 carries the three readings the task names: the option row
table at `tools/docs/example_drift.py:567` to `:581`, the page line cap
at `tools/docs/example_drift.py:256`, and the forbidden class scan of
the two User-guide pages. The four ctest entries now print their own
lines in the T024 suite log `/tmp/sg051-t024-suite.log`:
`snippet_compile` Passed 21.83 sec, `example_run` Passed 4.24 sec,
`report_shape` Passed 1.30 sec and `example_drift` Passed 0.09 sec.
That same log answers:

```
99% tests passed, 1 tests failed out of 192
The following tests FAILED:
 30 - prose_gate_fixtures (Failed)                      quick
```

`harness_topdown_run_test` stands skipped, and that suite reading
belongs to the T024 row, so nothing here turns on it.

### The suite log named in that note, corrected

### The suite log named in that note, corrected  The note above cites
the wrong suite log. The T024 row of record in this file reads
`/tmp/sg051-t024-suite-r2.log` and answers `100% tests passed out of
192`, and the four entries T031 names print their lines there:
`snippet_compile` Passed 17.72 sec, `example_run` Passed 3.62 sec,
`report_shape` Passed 0.87 sec and `example_drift` Passed 0.06 sec.
`/tmp/sg051-t024-suite.log` is the superseded read at
`a3c465ac71e73f5fd84c674bfdcadaa1164fea26`, the one holding the two
version-pin failures and the fixture race that an append to this file
caused. The five rows above stand unchanged: the drift gate exits 0 with
an empty log at every one of the three heads.  One timing note for the
suite legs. This block landed at 03:00:54, while the leg writing
`/tmp/sg051-b2-t024-suite.log` was reading the tree, and that leg's
`prose_gate_fixtures` entry answers `Passed    4.46 sec`, so no scan
raced this append.

### The suite log named in the closure note, corrected

That note cites the wrong suite log, and the paragraph printed after it
stands void: a duplicated heading from a bad write. The reading it meant
to carry is here.

The T024 row of record in this file reads `/tmp/sg051-t024-suite-r2.log`
and answers `100% tests passed out of 192`, and the four entries T031
names print their lines there: `snippet_compile` Passed 17.72 sec,
`example_run` Passed 3.62 sec, `report_shape` Passed 0.87 sec and
`example_drift` Passed 0.06 sec. `/tmp/sg051-t024-suite.log` is the
superseded read at `a3c465ac71e73f5fd84c674bfdcadaa1164fea26`, the one
holding the two version-pin failures and the fixture race that an append
to this file caused. The five rows of the closure block stand unchanged:
the drift gate exits 0 with an empty log at all three heads.

One timing note for the suite legs. The closure block landed at
03:00:54, while the leg writing `/tmp/sg051-b2-t024-suite.log` was
reading the tree, and that leg's `prose_gate_fixtures` entry answers
`Passed 4.46 sec`, so no scan raced it.

# The build legs at the coverage-closure head

Built head `9033535eb95ed0d1065360ef4c6bd9cae5c8f287`, copied from the
section "The built head after the coverage closure" of
`results/commits.md`. That commit is the test-scenario landing, and the
transform commit `bb45fa52` stands one below it. The checkout these legs
run in reads `564d7dcd03538744f6cfb1b7fad09deeaac0eeae`, and
`git diff --name-only` over the two heads lists `tasks.md`,
`results/commits.md` and this file, so the source, test, build and docs
surfaces match the built head. Every leg below supersedes its
counterpart at the earlier built head `b0cd97e2e23f605724d3d2593490df60a71a3cd8`.

## T024, the full suite in the shared `build` tree

Tree `build`, configuration `ci-rocky`: `CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`,
`CMAKE_BUILD_TYPE=Release`, and the analyzer cache variables absent from
the cache at the purge read.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-b2-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0` | `/tmp/sg051-b2-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, 4 objects recompiled for the closure, 0 `warning:` lines | `/tmp/sg051-b2-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 192`, no `Failed` line, 0 `Not Run` lines, 76.77 sec | `/tmp/sg051-b2-t024-suite.log` |

The count read `ctest --test-dir build -N` closes with
`Total Tests: 192`. The two closure units ride inside that count: entry
55 `counters_raw_event_test` and entry 155 `harness_raw_events_test`
both `Passed`. The registered count stands at 192 through the new cases
and scenarios.

Superseded: the T024 leg at `b0cd97e2` above, green on its own terms, is
superseded for the closure since its binaries carry no closure case.

Skip entry, named as the base row names it: `190 -
harness_topdown_run_test (Skipped)`, the host identity guard reading
`HARNESS TOPDOWN RUN TEST SKIP: the identity of this host names a
directory beside the fixture's`, the `SKIP_RETURN_CODE 2` registration
carrying the exit. 192 registered, 191 executed, 1 skipped.

## T025, the sanitizer leg in `build/sanitize`

Tree `build/sanitize`, configuration `ci-sanitize` with `CXX=clang++`:
the cache names `/usr/bin/clang++`, `clang version 23.1.4`,
`CMAKE_BUILD_TYPE=Sanitize`, and the sanitizer flags of
`CMakePresets.json:121` to `:128`. The tree carries the cold-start
history of the leg above, so the cache read was taken fresh here.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-sanitize` | `exit:0` over a warm Clang cache | `/tmp/sg051-b2-sanitize-cfg.log` |
| `cmake --build build/sanitize -j 10` | `exit:0`, 4 objects recompiled for the closure, 0 `warning:` lines | `/tmp/sg051-b2-sanitize-build.log` |
| `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 187`, no `Failed` line, 0 `Not Run` lines, 62.42 sec | `/tmp/sg051-b2-sanitize-suite.log` |

The cache read for the leg answers
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`. The verdict greps over
the suite log answer `ERROR: AddressSanitizer` 0, `runtime error:` 0,
`LeakSanitizer` 0 and `detected memory leaks` 0. The suite registers
187 entries and executes 186: the skip entry is
`185 - harness_topdown_run_test (Skipped)`, the host identity guard of
the coverage-closure row above.

Superseded: the T025 leg at `b0cd97e2`.

## T026, the thread sanitizer leg in `build/tsan`

Tree `build/tsan`, configuration `ci-tsan` with `CXX=clang++`: the cache
names `/usr/bin/clang++`, `clang version 23.1.4`, the thread sanitizer
flags on the Release configuration at `CMakePresets.json:130` to `:141`,
and `SPEEDGUN_DBC_MATRIX=ON`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-tsan` | `exit:0` over a warm Clang cache | `/tmp/sg051-b2-tsan-cfg.log` |
| `cmake --build build/tsan -j 10` | `exit:0`, 4 objects recompiled for the closure, 0 `warning:` lines | `/tmp/sg051-b2-tsan-build.log` |
| `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 188`, no `Failed` line, 0 `Not Run` lines, 181.44 sec | `/tmp/sg051-b2-tsan-suite.log` |

The verdict grep for `WARNING: ThreadSanitizer` answers 0. Entry 131
`harness_context_barrier_test` reads `Passed    0.17 sec` in the run, so
the standing host flake of about one run in ten for that entry under a
thread sanitizer finds no use at this leg, and the P2 exception clause
Principle VIII carries stays a recorded allowance with no row to hang on.

The suite registers 188 entries and executes 187: the skip entry is
`186 - harness_topdown_run_test (Skipped)`, the host identity guard of
the coverage-closure row above.

Superseded: the T026 leg at `b0cd97e2`.

## T027, the coverage leg in `build/coverage`

Tree `build/coverage`, configuration `ci-coverage`: the cache names
`/usr/bin/c++` over `GNU 16.2.1`, `CMAKE_BUILD_TYPE=Coverage`, the
capture of `cmake/coverage.cmake:73` keeps `--ignore-errors
mismatch,negative`, and the disk read gives `/home` 754G free.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-coverage` | `exit:0` over a warm GCC cache | `/tmp/sg051-b2-coverage-cfg.log` |
| `cmake --build build/coverage -j 10` | `exit:0`, 4 objects recompiled for the closure, 0 `warning:` lines, 294 `.gcno` files | `/tmp/sg051-b2-coverage-build.log` |
| `find build/coverage -name '*.gcda' -delete` | `exit:0` over an empty log, so the trace names one run | `/tmp/sg051-b2-coverage-gcda.log` |
| `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `exit:0`, `100% tests passed out of 187`, 187 registered, 186 executed, 1 skipped, 0 `Not Run` lines, 574.04 sec | `/tmp/sg051-b2-coverage-suite.log` |
| `cmake --build build/coverage -t coverage` | `exit:2`, the line arm clean, the branch arm red by 4 rows | `/tmp/sg051-b2-coverage-gate.log` |

The gate verdict block, copied:

```
Summary coverage rate:
  source files: 87
  lines.......: 100.0% (18929 of 18929 lines)
  functions...: 99.0% (1983 of 2003 functions)
  branches....: 99.7% (9944 of 9969 branches)
coverage-gate: branch shortfall 25 rows exceeds the  D-14 recorded allowance of 21
```

The line arm reads `lines.......: 100.0% (18929 of 18929 lines)`, and a
read of the `DA:` rows of `build/coverage/coverage.info` counts zero
rows held at zero hits, so the 13 dark lines of the leg at `b0cd97e2`
stand closed.

The arithmetic of the branch arm: 9969 minus 9944 answers 25 rows. The
prior reading at `b0cd97e2` answered 37 rows. The lane share moved from
15 rows to 3. The standing share reads 22 rows at both heads, the host
base of this machine. The allowance constant reads `allowance=21` at
`tools/dbc/coverage_gate.sh:108`, so the arm sits 4 rows over. The
within-allowance message of `tools/dbc/coverage_gate.sh:121` to `:123`
never printed; the printed message is the exceeds arm of
`tools/dbc/coverage_gate.sh:125` to `:126`, quoted above, and the awk
class of `tools/dbc/coverage_gate.sh:132` to `:139` printed the 25 rows
below into the same log.

| File | Rows | Branch rows, as the awk class names them | Class |
| --- | --- | --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp` | 3 | line 128 branch 2, line 128 branch 3, line 492 branch 5 | host skip |
| `source/harness/runner.cpp` | 4 | line 839 branches 2, 3, 4 and 5 | host skip |
| `source/counters/system.cpp` | 3 | line 618 branch 1, line 618 branch 2, line 641 branch 1 | lane |
| `source/harness/cli.cpp` | 4 | line 280 branches 8, 9, 10 and 11 | standing |
| `source/harness/noise_tracer.cpp` | 3 | line 287 branch 2, line 686 branches 10 and 11 | standing |
| `source/harness/options.cpp` | 2 | line 1541 branch 8, line 1541 branch 9 | standing |
| `source/harness/host_record_check.cpp` | 2 | line 266 branch 8, line 266 branch 9 | standing, ledger row two |
| `source/counters/plan.cpp` | 1 | line 476 branch 1 | standing |
| `source/harness/hunt_ranks.cpp` | 1 | line 127 branch 3 | standing |
| `source/harness/report.cpp` | 1 | line 815 branch 2 | standing |
| `source/harness/statistics.cpp` | 1 | line 84 branch 1 | standing |

Totals: 25 rows over 11 files, 7 rows in the host-skip class, 15 rows in
the standing accepted classes, 3 rows in the lane.

Host-skip class. This host carries one skipped entry, entry 185
`harness_topdown_run_test (Skipped)`, the identity guard reading
`HARNESS TOPDOWN RUN TEST SKIP: the identity of this host names a
directory beside the fixture's`. `runner.cpp:839` sits inside the
TopdownNode fill at the `static_cast` level row, and the
`topdown_select.cpp` rows sit in the selection that entry drives. The 21
rows of the allowance were calibrated on the 2026-10-30 Intel lane host,
where that suite ran; this machine is the AMD host `Gary`.

Lane share. `source/counters/system.cpp:618` is the partition predicate
`{ return one.core.name != "enabled" && one.core.name != "running"; }`,
and `source/counters/system.cpp:641` is the shaped reading
`leaf.size() >= 2U && leaf.front() == 'r' && ...`. Both lines fall
inside the lane diff of `git diff -U0 0fce6651 -- source include`.

Standing share. The 15 rows keep the classes the D-14 ledger carries, and
`host_record_check.cpp:266` carries the two rows the script names as its
row two. The reading supports no further attribution for the rest.

The verify reads of the leg: the marker read answers 84, the ceiling;
the new-marker read answers 0; the read over `cmake/coverage.cmake` and
`tools/dbc/coverage_gate.sh` prints no path. `genhtml` never reached
this run: the target order at `cmake/coverage.cmake:150` to `:159` puts
the HTML command after the gate, the gate exits 1, and the perl GD note
of `docs/pages/building.md` finds no read to carry.

Superseded: the T027 leg at `b0cd97e2` above, red on the line arm.

# The legs at the final built head

Built head `2f56e9fc06055d10a0ed5857623c6830a64fe731`, copied from the
section "The final built head" of `results/commits.md`. The checkout
reads `a32df6ad2c611f28a74f497a5582f57ffb05c598`, and
`git diff --name-only` over the two heads lists `tasks.md`,
`results/commits.md` and this file.

## T024, the full suite in the shared `build` tree: red

Tree `build`, configuration `ci-rocky`, compiler `/usr/bin/c++`,
`CMAKE_BUILD_TYPE=Release`.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-b3-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0` | `/tmp/sg051-b3-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, 1 object recompiled, 0 `warning:` lines | `/tmp/sg051-b3-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:8`, `99% tests passed, 1 tests failed out of 192`, 0 `Not Run` lines, 69.69 sec | `/tmp/sg051-b3-t024-suite.log` |

The failing entry, copied:

```
154/192 Test  #55: counters_raw_event_test ........................Subprocess aborted***Exception:   0.07 sec
[assertion] the provider drops an entry-less device at `provider.cpp:1006`, so the node behind a settled row holds an event leaf to read (predicate: eventLeaf != node.leaves.end()) at /home/archerc/code/speedgun-ng/source/counters/system.cpp:624
	 55 - counters_raw_event_test (Subprocess aborted)      quick
```

The alone read of the binary answers `exit:134` with the same assertion
line, and the case line printed before it names the case that reaches
it: `case: rawStampSkipsControlLeafNames`
(`/tmp/sg051-b3-raw-unit-alone.log`,
`/tmp/sg051-b3-raw-unit-alone2.log`). 192 registered, 189 passed, 1
aborted, 1 skipped: entry `190 - harness_topdown_run_test (Skipped)`,
the host identity guard.

The arm that aborts is the stamp of a raw row at
`source/counters/system.cpp:613` to `:630`. The partition at `:615` to
`:618` skips the `enabled` and `running` control names, and `:624`
carries `SG_ASSERT(eventLeaf != node.leaves.end(), ...)`. At
`9033535e` that position carried an early `return` over the same
predicate, the arm the coverage trace held dark at line 625. The new
case scripts a device whose only leaves carry the two control names, so
the partition returns the end iterator and the assertion aborts the
unit. `source/counters/system.cpp:618` branch 1 and branch 2 stand in
that predicate.

T025, T026 and T027 stand not run at this head: the same entry fails in
each of those configurations, and the chain owns one build at a time.
The green legs at `9033535e` above stay the readings of record until a
closure head lands.

## The cheap gates re-read at the final built head (T028 to T032)

The five rows of the coverage-closure block, read at
`9033535eb95ed0d1065360ef4c6bd9cae5c8f287`, stand superseded. The last
two counters cases moved a test file again, so the built head for the
remaining legs is `2f56e9fc06055d10a0ed5857623c6830a64fe731`, copied
from `results/commits.md` and titled `test: Light the last two shape
arms`. The same five tasks ran their commands again against that tree,
into logs carrying an `r4` suffix. `git diff --stat` between that head
and `a32df6ad`, the checkout at this read, prints nothing over `source`,
`test`, `include`, `docs`, `README.md`, `cmake` and `CMakeLists.txt`.
Host reads match the blocks above: `Gary`, `cmake version 4.4.5`,
`Python 3.14.9`, `clang-format version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r4.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029-r4.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 59 sources, 5378 units examined, 0 findings, 0 skipped`, then `prose-lint: 17 sources, 4477 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r4.log`, `/tmp/sg051-t030b-r4.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r4.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r4.log` |

The counts moved again: the range read over the same base `0fce6651`
reached 59 sources and 5378 units as the four commits since the pin move
entered the range, and the tree read reached 4477 units over the same 17
sources. Both answer 0 findings. T031 carries the three readings the
task names: the option row table at `tools/docs/example_drift.py:567` to
`:581`, the page line cap at `tools/docs/example_drift.py:256`, and the
forbidden class scan of the two User-guide pages.

The four ctest entries T031 names print their lines in the suite rows of
this file, and the fresh read at this head,
`/tmp/sg051-b3-t024-suite.log`, holds them too: `snippet_compile` Passed
20.15 sec, `example_run` Passed 4.27 sec, `report_shape` Passed 1.28 sec
and `example_drift` Passed 0.06 sec. That same log answers:

```
99% tests passed, 1 tests failed out of 192
The following tests FAILED:
	 55 - counters_raw_event_test (Subprocess aborted)      quick
```

`prose_gate_fixtures` answers `Passed 3.31 sec` in that log, so the
fixture scan held no race at this head. The failed entry is the lane's
own raw event unit, and that reading belongs to the T024 row.

One timing note for the suite legs. This block landed only after that
fixture entry reported, because an append to this file while a suite
reads the live tree moves the unit counts that entry compares.

# The legs at the stamp-revert head

Built head `da32efe9e2d22bbab900b633d8adfd20e6af9b4a`, copied from the
section "The built head after the stamp revert" of `results/commits.md`.
That commit returns the stamp miss arm at `source/counters/system.cpp`
to its early return. The checkout reads
`082c94c9381091159f1a79a0a66f6fabe46a8df7`, and `git diff --name-only`
over the two heads lists `tasks.md`, `results/commits.md` and this file,
so the source, test, build and docs surfaces match the built head. Each
leg below supersedes its counterpart at `2f56e9fc06055d10a0ed5857623c6830a64fe731`,
and through that the legs at `9033535eb95ed0d1065360ef4c6bd9cae5c8f287`.

## T024, the full suite in the shared `build` tree

Tree `build`, configuration `ci-rocky`: `CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`,
`CMAKE_BUILD_TYPE=Release`, the analyzer cache variables absent after the
purge read.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-b5-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0` | `/tmp/sg051-b5-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, 1 object recompiled for the revert, 0 `warning:` lines | `/tmp/sg051-b5-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 192`, no `Failed` line, no `Exception` line, 0 `Not Run` lines, 64.32 sec | `/tmp/sg051-b5-t024-suite.log` |

The count read `ctest --test-dir build -N` closes with
`Total Tests: 192`. Entry 55 `counters_raw_event_test` and entry 155
`harness_raw_events_test` both read `Passed`, so the eighteen case and
scenario load of the closure stands inside the registered count. Skip
entry: `190 - harness_topdown_run_test (Skipped)`, the host identity
guard. 192 registered, 191 executed, 1 skipped.

Superseded: the red T024 leg at `2f56e9fc` above, whose entry 55
aborted on the stamp assert, and the T024 leg at `9033535e` before it.

Read correction for the T024 block above: the sentence naming eighteen
figures the wrong count. The two units read 19 cases in
`counters_raw_event_test` and 12 scenarios in `harness_raw_events_test`,
copied from the `case` lines each binary prints at this head, and each
binary exits 0 with `counters_raw_event_test PASS: raw addresses over
the scripted kernel and sysfs trees` and `harness_raw_events_test: ok`
(`/tmp/sg051-b5-raw-cases.log`, `/tmp/sg051-b5-raw-scenarios.log`).

## The cheap gates re-read after the stamp revert (T028 to T032)

The five rows of the final-head block, read at
`2f56e9fc06055d10a0ed5857623c6830a64fe731`, stand superseded. The stamp
revert moves source, so the built head for the remaining legs is
`da32efe9e2d22bbab900b633d8adfd20e6af9b4a`, copied from
`results/commits.md` and titled `Counters: Keep the stamp return live`;
the diff between the two heads carries 18 lines of
`source/counters/system.cpp`. The same five tasks ran their commands
again against that tree, into logs carrying an `r5` suffix. `git diff
--stat` between that head and `082c94c9`, the checkout at this read,
prints nothing over `source`, `test`, `include`, `docs`, `README.md`,
`cmake` and `CMakeLists.txt`. Host reads match the blocks above: `Gary`,
`cmake version 4.4.5`, `Python 3.14.9`, `clang-format version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r5.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029-r5.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 61 sources, 5490 units examined, 0 findings, 0 skipped`, then `prose-lint: 17 sources, 4572 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r5.log`, `/tmp/sg051-t030b-r5.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r5.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r5.log` |

The counts moved again: the range read over the same base `0fce6651`
reached 61 sources and 5490 units as the revert and the record commit
entered the range, and the tree read reached 4572 units over the same 17
sources. Both answer 0 findings. T031 carries the three readings the
task names: the option row table at `tools/docs/example_drift.py:567` to
`:581`, the page line cap at `tools/docs/example_drift.py:256`, and the
forbidden class scan of the two User-guide pages.

The four ctest entries T031 names print their lines in the fresh suite
read at this head, `/tmp/sg051-b5-t024-suite.log`: `snippet_compile`
Passed 20.64 sec, `example_run` Passed 3.61 sec, `report_shape` Passed
0.88 sec and `example_drift` Passed 0.06 sec. That log answers:

```
100% tests passed out of 192
	190 - harness_topdown_run_test (Skipped)
```

so the raw event unit that aborted at `2f56e9fc` reports no failure
here, and the four entries T031 names clear at the same reading. That
suite reading belongs to the T024 row of this file.

One timing note for the suite legs. This block landed after that leg's
`prose_gate_fixtures` entry answered `Passed 2.84 sec`, because an
append to this file while a suite reads the live tree moves the unit
counts that entry compares.

Alone reads behind the T024 suite line, so the `0.00 sec` figure on
entry 55 carries its weight. The binaries stand at
`build/test/counters_raw_event_test` and
`build/test/harness_raw_events_test`. Each ran once at this head, direct
from the shell with its own log.

| Read | Verdict | Log |
| --- | --- | --- |
| `build/test/counters_raw_event_test` | `exit:0`, 19 `case` lines, 0 `FAIL` lines, closing line `counters_raw_event_test PASS: raw addresses over the scripted kernel and sysfs trees` | `/tmp/sg051-b5-raw-cases.log` |
| `build/test/harness_raw_events_test` | `exit:0`, 12 `case` lines, 0 `FAIL` lines, closing line `harness_raw_events_test: ok` | `/tmp/sg051-b5-raw-scenarios.log` |

The 19 and 12 figures are the `case` lines each binary prints, so the
case and scenario load of the closure ran to its end on this host. The
unit makes no real counters syscall: the scripted kernel and sysfs trees
answer in place, which is the shape of the `0.00 sec` suite figure.

## T025, the sanitizer leg in `build/sanitize`

Tree `build/sanitize`, configuration `ci-sanitize` with `CXX=clang++`:
the cache read answers `CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-sanitize` | `exit:0` over a warm Clang cache | `/tmp/sg051-b5-sanitize-cfg.log` |
| `cmake --build build/sanitize -j 10` | `exit:0`, 2 objects recompiled for the revert, 0 `warning:` lines | `/tmp/sg051-b5-sanitize-build.log` |
| `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 187`, no `Failed` line, 0 `Not Run` lines, 61.70 sec | `/tmp/sg051-b5-sanitize-suite.log` |

The verdict greps over the suite log answer `ERROR: AddressSanitizer`
0, `runtime error:` 0, `LeakSanitizer` 0 and `detected memory leaks` 0.
The suite registers 187 entries and executes 186: the skip entry is
`185 - harness_topdown_run_test (Skipped)`, the host identity guard.

Superseded: the T025 leg at `9033535e`, and the not-run state at
`2f56e9fc`.

## T026, the thread sanitizer leg in `build/tsan`

Tree `build/tsan`, configuration `ci-tsan` with `CXX=clang++`: the cache
read answers `CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-tsan` | `exit:0` over a warm Clang cache | `/tmp/sg051-b5-tsan-cfg.log` |
| `cmake --build build/tsan -j 10` | `exit:0`, 2 objects recompiled for the revert, 0 `warning:` lines | `/tmp/sg051-b5-tsan-build.log` |
| `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 188`, no `Failed` line, 0 `Not Run` lines, 181.27 sec | `/tmp/sg051-b5-tsan-suite.log` |

The verdict grep for `WARNING: ThreadSanitizer` answers 0. Entry 131
`harness_context_barrier_test` reads `Passed    0.16 sec`, so the
standing host flake of about one run in ten for that entry under a
thread sanitizer finds no use at this leg, and the P2 exception clause
Principle VIII carries stays a recorded allowance with no row to hang
on. The suite registers 188 entries and executes 187: the skip entry is
`186 - harness_topdown_run_test (Skipped)`, the host identity guard.

Superseded: the T026 leg at `9033535e`, and the not-run state at
`2f56e9fc`.

## T027, the coverage leg in `build/coverage`

Tree `build/coverage`, configuration `ci-coverage`: the cache read answers
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`, `CMAKE_BUILD_TYPE=Coverage`,
and the disk read gives `/home` 748G free.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-coverage` | `exit:0` over a warm GCC cache | `/tmp/sg051-b5-coverage-cfg.log` |
| `cmake --build build/coverage -j 10` | `exit:0`, 2 objects recompiled for the revert, 0 `warning:` lines, 294 `.gcno` files | `/tmp/sg051-b5-coverage-build.log` |
| `find build/coverage -name '*.gcda' -delete` | `exit:0` over an empty log, and the tree holds 0 `.gcda` files after it, so the trace names one run | `/tmp/sg051-b5-coverage-gcda.log` |
| `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `exit:0`, `100% tests passed out of 187`, 187 registered, 186 executed, 1 skipped, 0 `Not Run` lines, 591.04 sec | `/tmp/sg051-b5-coverage-suite.log` |
| `cmake --build build/coverage -t coverage` | `exit:2`, the line arm at 100.0% and the branch arm over the constant by one row | `/tmp/sg051-b5-coverage-gate.log` |

The gate block, copied:

```
Summary coverage rate:
  source files: 87
  lines.......: 100.0% (18931 of 18931 lines)
  functions...: 99.0% (1983 of 2003 functions)
  branches....: 99.8% (9949 of 9971 branches)
coverage-gate: branch shortfall 22 rows exceeds the  D-14 recorded allowance of 21
```

The line arm reads 100.0% over 18931 of 18931, and a read of the `DA:`
rows of `build/coverage/coverage.info` counts zero rows held at zero
hits. The arithmetic of the branch arm: 9971 minus 9949 answers 22 rows
on this host. The constant reads `allowance=21` at
`tools/dbc/coverage_gate.sh:108`, so the host read sits one row over the
constant, and the printed verdict is the exceeds arm of
`tools/dbc/coverage_gate.sh:125` to `:126`, quoted above. The
within-allowance message of `tools/dbc/coverage_gate.sh:121` to `:123`
stands unprinted at this read.

The lane share of the branch rows is 0 rows at this head. The prior
readings stand in the record above: 37 rows at `b0cd97e2`, of which 15
sat on lane-added lines, and 25 rows at `9033535e`, of which 3 sat on
lane-added lines. The standing share of this read is the 22 rows in the
census below.

The stamp arm the closure moved is live and taken. The trace reads
`source/counters/system.cpp:618` at branch outcomes 48, 1, 1 and 47, so
both false arms of the partition predicate stand reached; the guard at
`:626` reads 1 and 47 across its two outcomes, and the `return;` at `:627`
holds 1 hit. The shaped reading now sits at `:642` and `:643`, where the
line moved with the comment, and its outcomes read 1574, 22304, 23878,
1, 1521 and 53. The 641 question closed with it: the row named
`source/counters/system.cpp line 641 branch 1` at `9033535e` holds a
taken count at this head.

Census of the 22 dark rows, per file and line with the branch positions
as lcov names them, and the attribution each row carries. The counts
match the awk class of `tools/dbc/coverage_gate.sh:132` to `:139`, which
printed these rows into the gate log.

| File and line | Branch positions | Rows | Attribution read |
| --- | --- | --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:128` | 2 and 3 | 2 | Allocation landing pad on the two `std::string` constructions inside one `push_back`, `specs/045-merge-fixes/results/close-rows.md:80` and `:86` |
| `source/counters/linux_pmu/topdown_select.cpp:492` | 5 | 1 | The gcov arm past a closed enumeration of `TopdownUnsupport`, `close-rows.md:96` and `:103` |
| `source/counters/plan.cpp:476` | 1 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/cli.cpp:280` | 8, 9, 10 and 11 | 4 | Allocation landing pad on the concatenation behind the ternary of the failure line, `close-rows.md:81` and `:87` |
| `source/harness/host_record_check.cpp:266` | 8 and 9 | 2 | The unwind tail of the `EvidenceRow` construction, the ledger's own row two at `tools/dbc/coverage_gate.sh:69` to `:90` |
| `source/harness/hunt_ranks.cpp:127` | 3 | 1 | The rank spread and the least-squares denominator of the same points, `close-rows.md:99` and `:106` |
| `source/harness/noise_tracer.cpp:287` | 2 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/noise_tracer.cpp:686` | 10 and 11 | 2 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/options.cpp:1541` | 8 and 9 | 2 | Allocation landing pad inside the refusal lambda of `applySourceValue`, `close-rows.md:83` and `:89` |
| `source/harness/report.cpp:815` | 2 | 1 | The second operand of the fold guard, read at `close-rows.md:98` and `:105` for the same guard at line 814 of that head |
| `source/harness/runner.cpp:839` | 2, 3, 4 and 5 | 4 | Host gated: the `TopdownNode` fill at the `static_cast` level row, driven by `harness_topdown_run_test`, entry 185 here and entry 190 in the shared tree, skipped by the identity guard on this AMD host |
| `source/harness/statistics.cpp:84` | 1 | 1 | The comparison that closes the tie scan, `close-rows.md:97` and `:104` |

Delta against the `9033535e` read: three rows closed and none joined.
The closed rows are `source/counters/system.cpp line 618 branch 1`,
`line 618 branch 2` and `line 641 branch 1`, the 618 and 624 family of
the stamp, closed by the return at `:626` to `:628` returning live
together with the case `rawStampSkipsControlLeafNames`. The line count
of the trace moved from 18929 to 18931 as the two lines of that arm came
back, and the branch record count moved from 9969 to 9971.

Projected CI union, a projection until the CI run exists: the host read
of 22 minus the seven topdown-family rows (the three at
`topdown_select.cpp:128` and `:492` and the four at `runner.cpp:839`)
answers 15. The ledger reading classes the three
`topdown_select.cpp` rows as allocation and enumeration arms, which
stand dark on any host, and that reading answers 18. Both figures sit
under the constant of 21 and at or under the recorded union high-water
of 19, so the pull request's `coverage` job is expected green. The gate
script frames the same arithmetic at `tools/dbc/coverage_gate.sh:91` to
`:107`: "The allowance counts the union of the CI coverage job, where
every entry runs to its end. That union read 16 rows at `bea6af5b` (run
40983302766, artifact `coverage-info`), 16 at the head of specs/033, 18
at specs/032 and 19 at specs/035, so the accepted classes carry the
reading within two rows of the constant." The same paragraph closes "A
host run that reports them reports the host." The host read of 22 with
the seven host rows named above is the reading this machine gives; the
local figure carries no claim about the CI run on its own.

The verify reads of the leg: the marker read answers 84, the ceiling;
the new-marker read answers 0; the read over `cmake/coverage.cmake` and
`tools/dbc/coverage_gate.sh` prints no path. `genhtml` never reached
this run: the target order at `cmake/coverage.cmake:150` to `:159` puts
the HTML command after the gate, the gate exits 1, and the perl GD note
of `docs/pages/building.md` finds no read to carry.

Superseded: the T027 leg at `9033535e` above, at 25 rows with three lane
rows, and the not-run state at `2f56e9fc`.

Citation correction for the T027 census above: the ledger's row two runs
at `tools/dbc/coverage_gate.sh:70` to `:90`. The census row named `:69`,
one line early.

## T034, the head analyzer read in `build-tidy`: red

Built head `da32efe9e2d22bbab900b633d8adfd20e6af9b4a`, the head of the
coverage-closure rows above. Tree `build-tidy` only, configuration
`ci-ubuntu`: clang-tidy and cppcheck front every compile through
`__run_co_compile`, ccache sits behind them, and `SPEEDGUN_DBC_MATRIX`
reads OFF there. The shared `build` tree stands untouched by this leg.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-ubuntu -B build-tidy` | `exit:0` | `/tmp/sg051-analyzer-cfg.log` |
| `cmake --build build-tidy --target clean` | `exit:0`, an empty log | `/tmp/sg051-analyzer-clean.log` |
| `cmake --build build-tidy -j 10` | `exit:2`, 81328 log lines, 183 `Building CXX object` lines against the 324 of the base read | `/tmp/sg051-head-analyzer.log` |

The build ran as `cmake --build build-tidy -j 10` with its whole output
redirected to `/tmp/sg051-head-analyzer.log`, the recorded tee form
aimed at the same path so the exit line names the build and not the
pipe.

The failure, copied from the log:

```
/home/archerc/code/speedgun-ng/test/source/counters_raw_event_test.cpp:543:22: error: invalid case style for constexpr variable 'rows' [readability-identifier-naming,-warnings-as-errors]
/home/archerc/code/speedgun-ng/test/source/counters_raw_event_test.cpp:897:22: error: invalid case style for constexpr variable 'rows' [readability-identifier-naming,-warnings-as-errors]
make[2]: *** [test/CMakeFiles/counters_raw_event_test.dir/source/counters_raw_event_test.cpp.o] Error 1
make[1]: *** [CMakeFiles/Makefile2:6653: test/CMakeFiles/counters_raw_event_test.dir/all] Error 2
make: *** [Makefile:166: all] Error 2
```

Both sites declare `constexpr ErrnoRow rows[]`, at
`test/source/counters_raw_event_test.cpp:543` inside the case
`rawProbeErrnoArms` and at `:897` inside `rawProbeScopeEncodingArms`.
The naming check is the one check `.clang-tidy` holds as an error, which
is the shape the task names, and the unit stops at 47 percent of the
build.

The name greps of the verify, copied:

```
grep -cF '[readability-identifier-naming]' /tmp/sg051-head-analyzer.log
0
grep -c 'readability-identifier-naming' /tmp/sg051-head-analyzer.log
2
grep -c ': warning:.*readability-identifier-naming' /tmp/sg051-head-analyzer.log
0
```

The bracketed read answers 0 while the check fired twice, because
clang-tidy writes the check name with a suffix inside the same
brackets, `[readability-identifier-naming,-warnings-as-errors]`, so the
literal `[readability-identifier-naming]` of the recorded grep matches
nothing at this head. The plain read answers 2, and both of those lines
carry the suffix. The base read of the same plain grep answered 10, all
echoed source text from the `NOLINT` line of
`test/source/detail/gcov_flush.hpp:18`; the head read holds 0 of those
echoes, the units that carry them standing unreached.

Error severity at this read answers 3 lines:

```
/home/archerc/code/speedgun-ng/test/source/counters_raw_event_test.cpp:543:22: error: invalid case style for constexpr variable 'rows' [readability-identifier-naming,-warnings-as-errors]
/home/archerc/code/speedgun-ng/test/source/counters_raw_event_test.cpp:897:22: error: invalid case style for constexpr variable 'rows' [readability-identifier-naming,-warnings-as-errors]
/home/archerc/code/speedgun-ng/source/harness/runner.cpp:3081:53: error: syntax error [syntaxError]
```

Of the three lines the base read names, the syntax error reproduces at
`source/harness/runner.cpp:3081`, the position that line reached after
the lane edits, the base reading having named it at `:3005`. The two
`danglingLifetime` lines of `test/source/harness_jitter_probe_alloc_test.cpp:513`
and `test/source/harness_topdown_run_test.cpp:3558` stand absent: greps
for a `Building CXX object` line and a `Checking` line of each of those
two units answer 0 at this read, the build having stopped before them.

Per-file warning counts at this read, beside the base figures of
`results/base-readings.md`. The counts stand partial: 183 of the 324
units compiled, so a header counted through fewer units reads lower and a
unit counted through more of its own diagnostics reads higher.

| File | Base | Head | Read |
| --- | --- | --- | --- |
| `source/counters/detail/pmu.hpp` | 117 | 90 | fewer units reached the header |
| `source/counters/system.cpp` | 22 | 25 | above base |
| `source/counters/plan.cpp` | 38 | 38 | level |
| `source/counters/linux_pmu/provider.cpp` | 28 | 34 | above base |
| `source/counters/linux_pmu/group_io.cpp` | 47 | 49 | above base |
| `source/harness/runner.cpp` | 325 | 329 | above base |
| `source/harness/options.cpp` | 235 | 235 | level |

The base caveat for the runner figure stands quoted from
`results/base-readings.md`: "clang-tidy stops at a syntax error on
`source/harness/runner.cpp:3005`, so the 325 count covers the part of
that unit the analyzer reached." The head read carries the same stop one
line deeper, at `:3081`.

Two reads outside the named set, for the record:
`test/source/counters_raw_event_test.cpp` answers 118 `: warning:` lines
and carries the two error lines above, while `test/source/harness_raw_events_test.cpp`,
`test/source/harness_cli_table_test.cpp` and `test/source/views_test.cpp`
answer 0, the last two having been compiled and the first compiled with
the errors. The chain stopped at this leg: no analyzer read ran again
after the failure.

# The legs at the constant-case rename head

Built head `b77ebf2ab11e02c6411b8789453f548d148c4a6e`, copied from the
section "The built head after the constant-case rename" of
`results/commits.md`. That commit puts the two errno tables of
`test/source/counters_raw_event_test.cpp` into constant case. The
checkout reads `a9642a523624267d7c3e8077d5c30305c0bf6c54`, and
`git diff --name-only` over the two heads lists `tasks.md`,
`results/commits.md`, `results/acceptance-host.md` and this file, so the
source, test, build and docs surfaces match the built head. Each leg
below supersedes its counterpart at
`da32efe9e2d22bbab900b633d8adfd20e6af9b4a`.

## T024, the full suite in the shared `build` tree

Tree `build`, configuration `ci-rocky`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`, `CMAKE_BUILD_TYPE=Release`,
the analyzer cache variables absent after the purge read.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-b6-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0` | `/tmp/sg051-b6-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, 1 object recompiled for the rename, 0 `warning:` lines | `/tmp/sg051-b6-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 192`, no `Failed` line, no `Exception` line, 0 `Not Run` lines, 77.17 sec | `/tmp/sg051-b6-t024-suite.log` |

The count read `ctest --test-dir build -N` closes with
`Total Tests: 192`. Skip entry: `190 - harness_topdown_run_test
(Skipped)`, the host identity guard. 192 registered, 191 executed, 1
skipped.

Alone reads of the two lane units, so the suite timing carries its
weight: `build/test/counters_raw_event_test` answers `exit:0` with 19
`case` lines and 0 `FAIL` lines, closing
`counters_raw_event_test PASS: raw addresses over the scripted kernel
and sysfs trees` (`/tmp/sg051-b6-raw-cases.log`), and
`build/test/harness_raw_events_test` answers `exit:0` with 12 `case`
lines and 0 `FAIL` lines, closing `harness_raw_events_test: ok`
(`/tmp/sg051-b6-raw-scenarios.log`).

Superseded: the T024 leg at `da32efe9`, and the legs at `2f56e9fc` and
`9033535e` before it.

## The cheap gates re-read after the constant-case rename (T028 to T032)

The five rows of the stamp-revert block, read at
`da32efe9e2d22bbab900b633d8adfd20e6af9b4a`, stand superseded. The
constant-case rename moves a test file, so the built head for the
remaining legs is `b77ebf2ab11e02c6411b8789453f548d148c4a6e`, copied
from `results/commits.md` and titled `test: Put the errno tables in
constant case`; the diff between the two heads carries eight lines of
`test/source/counters_raw_event_test.cpp`. The same five tasks ran their
commands again against that tree, into logs carrying an `r6` suffix.
`git diff --stat` between that head and `a9642a52`, the checkout at this
read, prints nothing over `source`, `test`, `include`, `docs`,
`README.md`, `cmake` and `CMakeLists.txt`. Host reads match the blocks
above: `Gary`, `cmake version 4.4.5`, `Python 3.14.9`, `clang-format
version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r6.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029-r6.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 66 sources, 6176 units examined, 0 findings, 0 skipped`, then `prose-lint: 18 sources, 5232 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r6.log`, `/tmp/sg051-t030b-r6.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r6.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r6.log` |

The counts moved again: the range read over the same base `0fce6651`
reached 66 sources and 6176 units as the rename and the record commits
entered the range, and the tree read reached 18 sources and 5232 units,
one source more since `results/acceptance-host.md` became tracked. Both
answer 0 findings. T031 carries the three readings the task names: the
option row table at `tools/docs/example_drift.py:567` to `:581`, the
page line cap at `tools/docs/example_drift.py:256`, and the forbidden
class scan of the two User-guide pages.

The four ctest entries T031 names print their lines in the fresh suite
read at this head, `/tmp/sg051-b6-t024-suite.log`: `snippet_compile`
Passed 22.67 sec, `example_run` Passed 4.42 sec, `report_shape` Passed
1.25 sec and `example_drift` Passed 0.12 sec. That log answers:

```
100% tests passed out of 192
	190 - harness_topdown_run_test (Skipped)
```

and `55 - counters_raw_event_test` answers `Passed 0.02 sec` there. That
suite reading belongs to the T024 row of this file.

One timing note for the suite legs. This block landed after that leg's
`prose_gate_fixtures` entry answered `Passed 3.93 sec` and the leg
ended, because an append to this file while a suite reads the live tree
moves the unit counts that entry compares.

## T025, the address and undefined-behavior sanitizer leg

Tree `build/sanitize`, configuration `ci-sanitize`. The compiler read
`grep -m1 'CMAKE_CXX_COMPILER:FILEPATH' build/sanitize/CMakeCache.txt`
names `/usr/bin/clang++`, which is what the recorded
`CXX=clang++ cmake --preset=ci-sanitize` carries into the tree.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-sanitize -B build/sanitize` | `exit:0` | `/tmp/sg051-b6-t025-cfg.log` |
| `cmake --build build/sanitize -j 10` | `exit:0`, 1 object recompiled for the rename | `/tmp/sg051-b6-t025-build.log` |
| `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 187`, no `Failed` line, 0 `Not Run` lines, 62.81 sec | `/tmp/sg051-b6-t025-suite.log` |

The sanitizer greps over that log answer 0 for `ERROR: AddressSanitizer`,
0 for `runtime error:`, 0 for `LeakSanitizer`, 0 for
`SUMMARY: AddressSanitizer` and 0 for `detected memory leaks`. The
count read `ctest --test-dir build/sanitize -N` closes with
`Total Tests: 187`: five `dbc_matrix_*` entries stand absent there, the
`SPEEDGUN_DBC_MATRIX` variable reading OFF outside `ci-rocky` and
`ci-tsan`. Skip entry: `185 - harness_topdown_run_test (Skipped)`, the
host identity guard.

Superseded: the T025 leg at `da32efe9`.

## T026, the thread-sanitizer leg

Tree `build/tsan`, configuration `ci-tsan`. The compiler read
`grep -m1 'CMAKE_CXX_COMPILER:FILEPATH' build/tsan/CMakeCache.txt` names
`/usr/bin/clang++`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-tsan -B build/tsan` | `exit:0` | `/tmp/sg051-b6-t026-cfg.log` |
| `cmake --build build/tsan -j 10` | `exit:0`, 1 object recompiled for the rename | `/tmp/sg051-b6-t026-build.log` |
| `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 188`, no `Failed` line, 0 `Not Run` lines, 183.10 sec | `/tmp/sg051-b6-t026-suite.log` |

The grep for `WARNING: ThreadSanitizer` over that log answers 0. The
entry `131 - harness_context_barrier_test` reads `Passed 0.17 sec` in
the shared run, so the flake clause of the task needs no alone read at
this head. `ctest --test-dir build/tsan -N` closes with
`Total Tests: 188`, four allocation-operator units guarded out under the
thread sanitizer at `test/CMakeLists.txt:328-334` and `:1085-1087`. Skip
entry: `186 - harness_topdown_run_test (Skipped)`, the host identity
guard.

Superseded: the T026 leg at `da32efe9`.

## T027, the coverage gate at the constant-case rename head

Tree `build/coverage`, configuration `ci-coverage`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++` and `CMAKE_BUILD_TYPE=Coverage`.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-coverage -B build/coverage` | `exit:0` | `/tmp/sg051-b6-t027-cfg.log` |
| `cmake --build build/coverage -j 10` | `exit:0`, 1 object recompiled for the rename, 294 `.gcno` files | `/tmp/sg051-b6-t027-build.log` |
| `find build/coverage -name '*.gcda' -delete` | `exit:0` over an empty log, and the tree holds 0 `.gcda` files after it, so the trace names one run | `/tmp/sg051-b6-t027-gcda.log` |
| `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `exit:0`, `100% tests passed out of 187`, 187 registered, 186 executed, 1 skipped, 0 `Not Run` lines, 578.10 sec | `/tmp/sg051-b6-t027-suite.log` |
| `cmake --build build/coverage -t coverage` | `exit:2`, the line arm at `100.0%` and the branch arm one row over the constant | `/tmp/sg051-b6-t027-coverage.log` |

The gate block, copied:

```
Summary coverage rate:
  source files: 87
  lines.......: 100.0% (18931 of 18931 lines)
  functions...: 99.0% (1983 of 2003 functions)
  branches....: 99.8% (9949 of 9971 branches)
coverage-gate: branch shortfall 22 rows exceeds the  D-14 recorded allowance of 21
```

The line arm reads `100.0%` over 18931 of 18931, and a read of the `DA:`
rows of `build/coverage/coverage.info` counts zero rows held at zero hits.
The branch arithmetic: 9971 minus 9949 answers 22 rows on this host, and
the constant reads `allowance=21` at `tools/dbc/coverage_gate.sh:108`, so
the host read sits one row over the constant. The printed verdict is the
exceeds arm of `tools/dbc/coverage_gate.sh:125` to `:126`, quoted above;
the within-allowance message of `tools/dbc/coverage_gate.sh:121` to
`:123` stands unprinted at this read.

The row set is identical to the read at `da32efe9`: 22 rows against 22,
none joined and none closed, and every summary figure matches. The
rename touched a test file, so no source line moved and no branch record
changed position. The lane share stands at 0 rows. The prior readings
stay in the record above: 37 rows at `b0cd97e2` with 15 on lane-added
lines, and 25 rows at `9033535e` with 3 on lane-added lines.

Census of the 22 rows, the positions as lcov names them, copied from the
list the gate itself printed with the awk class of
`tools/dbc/coverage_gate.sh:132` to `:139`, each row beside the
attribution it carries.

| File and line | Branch positions | Rows | Attribution read |
| --- | --- | --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:128` | 2 and 3 | 2 | Allocation landing pad on the two `std::string` constructions inside one `push_back`, `specs/045-merge-fixes/results/close-rows.md:80` and `:86` |
| `source/counters/linux_pmu/topdown_select.cpp:492` | 5 | 1 | The gcov arm past a closed enumeration of `TopdownUnsupport`, `close-rows.md:96` and `:103` |
| `source/counters/plan.cpp:476` | 1 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/cli.cpp:280` | 8, 9, 10 and 11 | 4 | Allocation landing pad on the concatenation behind the ternary of the failure line, `close-rows.md:81` and `:87` |
| `source/harness/host_record_check.cpp:266` | 8 and 9 | 2 | The unwind tail of the `EvidenceRow` construction, the ledger's own row two at `tools/dbc/coverage_gate.sh:70` to `:90` |
| `source/harness/hunt_ranks.cpp:127` | 3 | 1 | The rank spread and the least-squares denominator of the same points, `close-rows.md:99` and `:106` |
| `source/harness/noise_tracer.cpp:287` | 2 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/noise_tracer.cpp:686` | 10 and 11 | 2 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/options.cpp:1541` | 8 and 9 | 2 | Allocation landing pad inside the refusal lambda of `applySourceValue`, `close-rows.md:83` and `:89` |
| `source/harness/report.cpp:815` | 2 | 1 | The second operand of the fold guard, read at `close-rows.md:98` and `:105` for the same guard at line 814 of that head |
| `source/harness/runner.cpp:839` | 2, 3, 4 and 5 | 4 | Host gated: the `TopdownNode` fill at the `static_cast` level row, driven by `harness_topdown_run_test`, entry 185 here and entry 190 in the shared tree, skipped by the identity guard on this AMD host |
| `source/harness/statistics.cpp:84` | 1 | 1 | The comparison that closes the tie scan, `close-rows.md:97` and `:104` |

Projected CI union, a projection until the CI run exists: the host read
of 22 minus the seven topdown-family rows (the three at
`topdown_select.cpp:128` and `:492` and the four at `runner.cpp:839`)
answers 15. The ledger reading classes the three `topdown_select.cpp`
rows as allocation and enumeration arms, which stand dark on any host,
and that reading answers 18. Both figures sit under the constant of 21
and at or under the recorded union high-water of 19, so the pull
request's `coverage` job is expected green. The gate script frames the
same arithmetic at `tools/dbc/coverage_gate.sh:91` to `:107`: "The
allowance counts the union of the CI coverage job, where every entry
runs to its end. That union read 16 rows at `bea6af5b` (run
40983302766, artifact `coverage-info`), 16 at the head of specs/033, 18
at specs/032 and 19 at specs/035, so the accepted classes carry the
reading within two rows of the constant." The same paragraph closes "A
host run that reports them reports the host." The host read of 22 with
the seven host rows named above is the reading this machine gives, and
the local figure carries no claim about the CI run on its own.

The verify reads of the leg: the marker read
`git grep -c LCOV_EXCL -- source include | awk -F: '{s+=$2} END {print s}'`
answers 84, the ceiling; the new-marker read
`git diff -U0 0fce6651 -- source include | grep -c '^+.*LCOV_EXCL'`
answers 0; and `git diff --name-only 0fce6651 -- cmake/coverage.cmake
tools/dbc/coverage_gate.sh` prints nothing, which keeps the capture at
the tracked `--ignore-errors mismatch,negative` of
`cmake/coverage.cmake:73`. The `inconsistent` class is absent from the
gate log. `genhtml` never ran at this read, the gate exiting before the
HTML command of `cmake/coverage.cmake:150` to `:159`, so the perl GD host
note of `docs/pages/building.md` carries nothing to record.

Superseded: the T027 leg at `da32efe9`, and the legs at `2f56e9fc`,
`9033535e` and `b0cd97e2` before it.

## T034 rerun, the analyzer read at the constant-case rename head: red

Built head `b77ebf2ab11e02c6411b8789453f548d148c4a6e`, the head of the
T024 to T027 rows above. Tree `build-tidy` only, configuration
`ci-ubuntu`, the shared `build` tree untouched.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-ubuntu -B build-tidy` | `exit:0` | `/tmp/sg051-analyzer-cfg-r2.log` |
| `cmake --build build-tidy --target clean` | `exit:0`, an empty log | `/tmp/sg051-analyzer-clean-r2.log` |
| `cmake --build build-tidy -j 10` | `exit:2`, 242347 log lines, 312 `Building CXX object` lines against the 324 of the base read | `/tmp/sg051-head-analyzer-r2.log` |

The recorded tee form ran as a redirect to that same log path, so the
exit line names the build and not the pipe.

The rename of the two errno tables cleared the two findings of the
earlier read. Two findings of the same kind stand in the other lane
unit, copied from the log:

```
/home/archerc/code/speedgun-ng/test/source/harness_raw_events_test.cpp:1237:34: error: invalid case style for static constant 'cases' [readability-identifier-naming,-warnings-as-errors]
/home/archerc/code/speedgun-ng/test/source/harness_raw_events_test.cpp:1264:49: error: invalid case style for static constant 'modes' [readability-identifier-naming,-warnings-as-errors]
make[2]: *** [test/CMakeFiles/harness_raw_events_test.dir/build.make:79: test/CMakeFiles/harness_raw_events_test.dir/source/harness_raw_events_test.cpp.o] Error 1
make[1]: *** [CMakeFiles/Makefile2:10189: test/CMakeFiles/harness_raw_events_test.dir/all] Error 2
make: *** [Makefile:166: all] Error 2
```

The first site declares `static const std::vector<Case> cases` inside
`allCases()` at `test/source/harness_raw_events_test.cpp:1237`, the
table of the twelve scenario names, and the second declares
`static const std::array<std::string_view, 16> modes` at `:1264` inside
`isChildMode`. Both hold function-scope static storage in lower case, the shape
the check promotes to an error. The same unit reads green under the
suite at T024: the check only runs in this tree.

Both grep reads of the amended task text, copied:

```
grep -cF '[readability-identifier-naming]' /tmp/sg051-head-analyzer-r2.log
0
grep -c 'readability-identifier-naming' /tmp/sg051-head-analyzer-r2.log
12
grep -c ': warning:.*readability-identifier-naming' /tmp/sg051-head-analyzer-r2.log
0
```

clang-tidy writes a promoted finding with the suffix
`-warnings-as-errors` inside the same brackets, as
`[readability-identifier-naming,-warnings-as-errors]`, so the literal
bracketed form of the recorded grep matches a run with no finding at
all. The plain read answers 12 here: the ten standing echoed source
lines of the `NOLINT` comment at `test/source/detail/gcov_flush.hpp:18`,
the figure the base read gave, plus the two finding lines above.

Error severity at this read answers 4 lines:

```
/home/archerc/code/speedgun-ng/source/harness/runner.cpp:3081:53: error: syntax error [syntaxError]
/home/archerc/code/speedgun-ng/test/source/harness_jitter_probe_alloc_test.cpp:513:11: error: Non-local variable 'plantedEscapeSink' will use pointer to local variable 'allocation'. [danglingLifetime]
/home/archerc/code/speedgun-ng/test/source/harness_raw_events_test.cpp:1237:34: error: invalid case style for static constant 'cases' [readability-identifier-naming,-warnings-as-errors]
/home/archerc/code/speedgun-ng/test/source/harness_raw_events_test.cpp:1264:49: error: invalid case style for static constant 'modes' [readability-identifier-naming,-warnings-as-errors]
```

Of the three lines the base read names: the syntax error reproduces at
`source/harness/runner.cpp:3081`, the position it reached after the lane
edits, the base having named it at `:3005`. The `danglingLifetime` line
of `test/source/harness_jitter_probe_alloc_test.cpp:513` reproduces
identically, same line, same columns, same wording. The
`danglingLifetime` line of `test/source/harness_topdown_run_test.cpp:3558`
is absent from this read, and this read is not a complete run: greps for
the `Building CXX object` line and the `Checking` line of that unit both
answer 0, and 14 units of the base list stand unreached after the stop,
the topdown, graph, host-record and cli-graph family together with the
two `host_record_check` and `host_record_write` tool units. The third
base line therefore stands unverified at this head, and it needs a run
that reaches that unit.

Per-file warning counts, beside the base counts of
`results/base-readings.md`. The six source units all compiled before the
stop, so their figures carry the comparison; the header count stands
partial, the 14 unreached units including it.

| File | Base | This read | Read |
| --- | --- | --- | --- |
| `source/counters/detail/pmu.hpp` | 117 | 90 | partial: 14 including units unreached |
| `source/counters/system.cpp` | 22 | 25 | above base by 3 |
| `source/counters/plan.cpp` | 38 | 38 | level |
| `source/counters/linux_pmu/provider.cpp` | 28 | 34 | above base by 6 |
| `source/counters/linux_pmu/group_io.cpp` | 47 | 49 | above base by 2 |
| `source/harness/runner.cpp` | 325 | 329 | above base by 4 |
| `source/harness/options.cpp` | 235 | 235 | level |

The base caveat for the runner figure stands quoted from
`results/base-readings.md`: "clang-tidy stops at a syntax error on
`source/harness/runner.cpp:3005`, so the 325 count covers the part of
that unit the analyzer reached." This read carries the same stop one line
deeper at `:3081`, so both counts cover the same part of the unit.

The unit count reads 312 against the base 324: 14 base units unreached
and the two lane units of `counters_raw_event_test.cpp` and
`harness_raw_events_test.cpp` new to this tree. The chain stopped at this
leg again; no further analyzer read ran.

Superseded: the T034 read at `da32efe9`, whose four above-base counts the
partial run voided.

# The legs at the second constant-case rename head

Built head `b2763369963f761b5138cc97dd375876dd03eb0d`, copied from the
section "The built head after the second constant-case rename" of
`results/commits.md`. That commit puts the scenario table and the child
mode table of `test/source/harness_raw_events_test.cpp` into constant
case. The checkout reads `1dc647e938e60bbf6e9e8782f289c699afb5e014`, and
`git diff --name-only` over the two heads lists
`specs/051-raw-events/results/commits.md` alone, so the source, test,
build and docs surfaces match the built head. Each leg below supersedes
its counterpart at `b77ebf2ab11e02c6411b8789453f548d148c4a6e`.

## T024, the full suite in the shared `build` tree

Tree `build`, configuration `ci-rocky`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`, `CMAKE_BUILD_TYPE=Release`,
the analyzer cache variables absent after the purge read.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-b7-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0` | `/tmp/sg051-b7-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, 1 object recompiled for the rename, 0 `warning:` lines | `/tmp/sg051-b7-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 192`, no `Failed` line, no `Exception` line, 0 `Not Run` lines, 73.13 sec | `/tmp/sg051-b7-t024-suite.log` |

The count read `ctest --test-dir build -N` closes with
`Total Tests: 192`. Skip entry: `190 - harness_topdown_run_test
(Skipped)`, the host identity guard. 192 registered, 191 executed, 1
skipped. Entries `55 - counters_raw_event_test` and
`155 - harness_raw_events_test` read `Passed`.

Alone reads of the two lane units: `build/test/counters_raw_event_test`
answers `exit:0` with 19 `case` lines and 0 `FAIL` lines, closing
`counters_raw_event_test PASS: raw addresses over the scripted kernel
and sysfs trees` (`/tmp/sg051-b7-raw-cases.log`), and
`build/test/harness_raw_events_test` answers `exit:0` with 12 `case`
lines and 0 `FAIL` lines, closing `harness_raw_events_test: ok`
(`/tmp/sg051-b7-raw-scenarios.log`).

Superseded: the T024 leg at `b77ebf2a`, and the legs at `da32efe9`,
`2f56e9fc` and `9033535e` before it.

## The cheap gates re-read at the second constant-case rename (T028 to T032)

The five rows of the rename block, read at
`b77ebf2ab11e02c6411b8789453f548d148c4a6e`, stand superseded. The
harness unit tables move case, so the built head is
`b2763369963f761b5138cc97dd375876dd03eb0d`, copied from
`results/commits.md` and titled `test: Put the tables in constant case`;
the diff between the two heads carries 40 lines of
`test/source/harness_raw_events_test.cpp`. The same five tasks ran their
commands again against that tree, into logs carrying an `r7` suffix.
`git diff --stat` between that head and `1dc647e9`, the checkout at this
read, prints nothing over `source`, `test`, `include`, `docs`,
`README.md`, `cmake` and `CMakeLists.txt`. Host reads match the blocks
above: `Gary`, `cmake version 4.4.5`, `Python 3.14.9`, `clang-format
version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r7.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 1, one hit, the leg stands red | `/tmp/sg051-t029-r7.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 69 sources, 6317 units examined, 0 findings, 0 skipped`, then `prose-lint: 18 sources, 5555 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r7.log`, `/tmp/sg051-t030b-r7.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r7.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r7.log` |

One leg reports red at this head, and it names this file. `codespell`
returns 65 over one line, `cmake/spell.cmake` maps that to a fatal
message and the leg exits 1:

```
Used config files:
    1: .codespellrc
CMake Error at cmake/spell.cmake:24 (message):
  Run again with FIX=YES to fix these errors.
```

The reported line is this record's line 1282, inside the analyzer
block's sentence about the two function-scope tables, and the checker
reads the plural noun there as a short form of `statistics`. Three runs
at this head printed the same line, so the reading is not a scan
artifact. This block cites the line and leaves the pair out,
because a copied pair would hand the next run a second hit in the same
file. The checker's own line stands verbatim in
`/tmp/sg051-t029-r7.log`. Rails keep a tracked-file hit with the lane
and out of this hand: nothing was edited here, and the wording fix
belongs to the leg that wrote the sentence.

The other four legs read green. The counts moved again: the range read
over the same base `0fce6651` reached 69 sources and 6317 units, and the
tree read reached 18 sources and 5555 units. T031 carries the three
readings the task names: the option row table at
`tools/docs/example_drift.py:567` to `:581`, the page line cap at
`tools/docs/example_drift.py:256`, and the forbidden class scan of the
two User-guide pages. The four ctest entries T031 names pass in the
fresh suite read at this head, `/tmp/sg051-b7-t024-suite.log`:
`snippet_compile` 20.73 sec, `example_run` 4.25 sec, `report_shape` 0.87
sec and `example_drift` 0.07 sec, over `100% tests passed out of 192`
with the standing skip of `harness_topdown_run_test`. That suite reading
belongs to the T024 row of this file.

One timing note for the suite legs. This block landed after that leg's
`prose_gate_fixtures` entry answered `Passed 3.29 sec` and the leg
ended, because an append while a suite reads the live tree moves the
unit counts that entry compares.

## T025, the sanitizer leg at the rename head: red on a timing flake

Tree `build/sanitize`, configuration `ci-sanitize`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++` from the recorded
`CXX=clang++ cmake --preset=ci-sanitize`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-sanitize -B build/sanitize` | `exit:0` | `/tmp/sg051-b7-t025-cfg.log` |
| `cmake --build build/sanitize -j 10` | `exit:0` | `/tmp/sg051-b7-t025-build.log` |
| `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10` | `exit:8`, `99% tests passed, 1 tests failed out of 187`, 91.84 sec | `/tmp/sg051-b7-t025-suite.log` |

The one failing entry, copied:

```
48/187 Test  #58: counters_overhead ..............................***Failed    0.10 sec
99% tests passed, 1 tests failed out of 187
Total Test time (real) =  91.84 sec
The following tests FAILED:
	 58 - counters_overhead (Failed)                        quick
```

The unit's own two diagnostics, the only failure text it printed:

```
clock, syscall (vDSO): published floor median 90.0 ns against 150.0 ns measured under the identical bracketing
clock raw, syscall (vDSO): published floor median 130.0 ns against 80.0 ns measured under the identical bracketing
```

No sanitizer line appears anywhere in the log: the greps answer 0 for
`ERROR: AddressSanitizer`, 0 for `runtime error:`, 0 for
`LeakSanitizer`, 0 for `SUMMARY: AddressSanitizer` and 0 for
`detected memory leaks`. The leg holds no sanitizer finding. Skip entry:
`185 - harness_topdown_run_test (Skipped)`, the host identity guard. The
other 185 executed entries read `Passed`.

Isolation reads of the failing entry under the same environment, the
recorded line narrowed with `-R '^counters_overhead$'`: five runs, each
`exit:0` with `100% tests passed out of 1` and no published-floor line
(`/tmp/sg051-b7-t025-overhead-1.log` through
`/tmp/sg051-b7-t025-overhead-5.log`).

Host state at the read: `uptime` answered `04:54:07 up 1 day, 6:37, 1
user, load average: 20.59, 16.14, 10.70`, and the compiler and ctest
process read found no sibling build holding the host. Two records frame
this entry and this figure. `specs/048-hwlat-probe/results/gates.md:65`
reads "`counters_overhead` fails only under load."
`specs/015-benchmark-harness-core/tasks.md:331` frames the published
floor as "a wall-clock figure that moves with the host". The shared-tree
run of T024 at this head read entry 58 `Passed`, and the same leg at
`b77ebf2a` read entry 58 `Passed 0.19 sec` with no floor line.

Verdict: 1 fail at load average 20.59, 5 of 5 green in isolation, an
entry the record at `specs/048-hwlat-probe/results/gates.md:65` names as
failing only under load. Disposition from the record owner: red as a
timing flake, no suite rerun. A later leg that fails this entry at a
quiet load snapshot stops the chain and reports.

Superseded: the T025 leg at `b77ebf2a`.

## T026, the thread-sanitizer leg: red on a stalled entry

Tree `build/tsan`, configuration `ci-tsan`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++` from the recorded
`CXX=clang++ cmake --preset=ci-tsan`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-tsan -B build/tsan` | `exit:0` | `/tmp/sg051-b7-t026-cfg.log` |
| `cmake --build build/tsan -j 10` | `exit:0` | `/tmp/sg051-b7-t026-build.log` |
| `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10` | `exit:8`, `99% tests passed, 1 tests failed out of 188`, 481.45 sec | `/tmp/sg051-b7-t026-suite.log` |

The one failing entry, copied:

```
185/188 Test #130: harness_execution_context_test .................***Timeout 300.03 sec
99% tests passed, 1 tests failed out of 188
Total Test time (real) = 481.45 sec
The following tests FAILED:
	130 - harness_execution_context_test (Timeout)          quick
```

The grep for `WARNING: ThreadSanitizer` over that log answers 0, so the
leg holds no sanitizer finding. Entry `131 - harness_context_barrier_test`
reads `Passed 0.17 sec`, so the standing note of about one run in ten for
that entry under a thread sanitizer on this machine stands unexercised at
this leg. Skip entry: `186 - harness_topdown_run_test (Skipped)`, the
host identity guard. `ctest --test-dir build/tsan -N` closes with
`Total Tests: 188`.

Where the stalled entry stopped: the unit prints one line per scenario
from the `kScenarios` table of
`test/source/harness_execution_context_test.cpp`, 29 rows. Both stalled
reads printed 25 of those lines, through `drainPendingInbox`, and printed
nothing more inside the budget. The next row of the table names
`acceptedNotLinked`. The 300.03 sec figure is the project default for an
entry whose registration names no timeout, set by the loop at
`test/CMakeLists.txt:2508` to `:2513`.

Isolation reads under the same environment, the recorded line narrowed
with `-R '^harness_execution_context_test$'`: read 1 at 05:03 with load
average 5.35 answered `***Timeout 300.09 sec` with the same 25 scenario
lines (`/tmp/sg051-b7-t026-context-1.log`); reads 2 through 6 at 05:09
with load average 6.13 to 7.40 each answered `exit:0` and `Passed` at
0.15, 0.13, 0.13, 0.14 and 0.15 sec
(`/tmp/sg051-b7-t026-context-2.log` through
`/tmp/sg051-b7-t026-context-6.log`). A direct run of
`build/tsan/test/harness_execution_context_test` printed all 29 scenario
lines and `harness_execution_context_test: ok` in under 6 sec
(`/tmp/sg051-b7-t026-context-direct.log`). In the `ci-sanitize` tree at
this head the same unit reads `Passed 0.15 sec`.

Host state: the shared run began with `load average: 20.59, 16.14,
10.70` at 04:54, and its wall time read 481.45 sec against 183.10 sec for
the same leg at `b77ebf2a`. The isolation reads ran at load average 5.35
to 7.40. No sibling compiler process stood at any snapshot.

Verdict: 1 stall in the shared `-j 10` run at load average 20.59, and 1
stall in 6 isolation reads with 5 of 6 green in isolation at 0.13 to 0.15
sec. Two house records name this entry and this preset with no report
lines: `specs/028-full-command-line/tasks.md:808` to `:811` reads that
the `ci-tsan` suite passed "after one earlier pass stalled once on the
test `harness_execution_context_test` under heavy parallel load; that
test passed twice on its own afterwards and carries no `TIMEOUT`
property." `specs/041-counter-probe/results/sanitizers.md:39` carries the
row "`harness_execution_context_test`, verdict `Timeout`, 0 report lines",
framed there as "Each earlier reading failed on a host carrying two
suites at once, and the unit that failed was never a file this branch
touches", with "Those units spawn child runs and measure spans, so they
are the first entries a contended machine stops."
`test/source/harness_execution_context_test.cpp` sits outside the lane
path set.

Corroboration for contention: the leg's wall time reads 481.45 sec
against 183.10 sec for the same leg at `b77ebf2a`, and the same unit
reads `Passed 0.15 sec` in the `ci-sanitize` tree at this head while a
direct run of its binary printed all 29 scenario lines in under 6 sec.
The unit forks and execs its own binary at
`test/source/harness_execution_context_test.cpp:174` and `:184` for its
contract-breach scenarios, the class
`specs/041-counter-probe/results/sanitizers.md` describes as spawning
child runs and measuring spans. Caveat the lane owes: the lane edits
`source/harness/runner.cpp`, and the lane's raw arms there sit behind
the predicate `hasRawCodeShape(leafName)`, while the stalled scenario
path takes no raw-shaped address, so the lane reading carries no
explanation of the stall.

Disposition from the record owner: red, recorded with these reads, and
the chain continues. A later leg that stalls this entry again at a quiet
load snapshot stops the chain and reports.

Superseded: the T026 leg at `b77ebf2a`.

## T027, the coverage gate at the second rename head

Tree `build/coverage`, configuration `ci-coverage`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++` and `CMAKE_BUILD_TYPE=Coverage`.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-coverage -B build/coverage` | `exit:0` | `/tmp/sg051-b7-t027-cfg.log` |
| `cmake --build build/coverage -j 10` | `exit:0`, 1 object recompiled for the rename, 294 `.gcno` files | `/tmp/sg051-b7-t027-build.log` |
| `find build/coverage -name '*.gcda' -delete` | `exit:0` over an empty log, and the tree holds 0 `.gcda` files after it, so the trace names one run | `/tmp/sg051-b7-t027-gcda.log` |
| `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `exit:0`, `100% tests passed out of 187`, 187 registered, 186 executed, 1 skipped, 0 `Not Run` lines, 574.34 sec | `/tmp/sg051-b7-t027-suite.log` |
| `cmake --build build/coverage -t coverage` | `exit:2`, the line arm at `100.0%` and the branch arm one row over the constant | `/tmp/sg051-b7-t027-coverage.log` |

The gate block, copied:

```
Summary coverage rate:
  source files: 87
  lines.......: 100.0% (18931 of 18931 lines)
  functions...: 99.0% (1983 of 2003 functions)
  branches....: 99.8% (9949 of 9971 branches)
coverage-gate: branch shortfall 22 rows exceeds the  D-14 recorded allowance of 21
```

The line arm reads `100.0%` over 18931 of 18931, and a read of the `DA:`
rows of `build/coverage/coverage.info` counts zero rows held at zero hits.
The branch arithmetic: 9971 minus 9949 answers 22 rows on this host, and
the constant reads `allowance=21` at `tools/dbc/coverage_gate.sh:108`, so
the host read sits one row over the constant. The printed verdict is the
exceeds arm of `tools/dbc/coverage_gate.sh:125` to `:126`, quoted above;
the within-allowance message of `tools/dbc/coverage_gate.sh:121` to
`:123` stands unprinted at this read. Entry `58 - counters_overhead` read
`Passed 0.28 sec` in this suite, so the timing entry that failed the
sanitizer leg at this head stands green here.

The row set matches the read at `b77ebf2a` and the read at `da32efe9`:
22 rows against 22, none joined and none closed, and every summary
figure matches. The lane share stands at 0 rows. The prior readings stay
in the record above: 37 rows at `b0cd97e2` with 15 on lane-added lines,
and 25 rows at `9033535e` with 3 on lane-added lines.

Census of the 22 rows, the positions as lcov names them, copied from the
list the gate itself printed with the awk class of
`tools/dbc/coverage_gate.sh:132` to `:139`, each row beside the
attribution it carries.

| File and line | Branch positions | Rows | Attribution read |
| --- | --- | --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:128` | 2 and 3 | 2 | Allocation landing pad on the two `std::string` constructions inside one `push_back`, `specs/045-merge-fixes/results/close-rows.md:80` and `:86` |
| `source/counters/linux_pmu/topdown_select.cpp:492` | 5 | 1 | The gcov arm past a closed enumeration of `TopdownUnsupport`, `close-rows.md:96` and `:103` |
| `source/counters/plan.cpp:476` | 1 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/cli.cpp:280` | 8, 9, 10 and 11 | 4 | Allocation landing pad on the concatenation behind the ternary of the failure line, `close-rows.md:81` and `:87` |
| `source/harness/host_record_check.cpp:266` | 8 and 9 | 2 | The unwind tail of the `EvidenceRow` construction, the ledger's own row two at `tools/dbc/coverage_gate.sh:70` to `:90` |
| `source/harness/hunt_ranks.cpp:127` | 3 | 1 | The rank spread and the least-squares denominator of the same points, `close-rows.md:99` and `:106` |
| `source/harness/noise_tracer.cpp:287` | 2 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/noise_tracer.cpp:686` | 10 and 11 | 2 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/options.cpp:1541` | 8 and 9 | 2 | Allocation landing pad inside the refusal lambda of `applySourceValue`, `close-rows.md:83` and `:89` |
| `source/harness/report.cpp:815` | 2 | 1 | The second operand of the fold guard, read at `close-rows.md:98` and `:105` for the same guard at line 814 of that head |
| `source/harness/runner.cpp:839` | 2, 3, 4 and 5 | 4 | Host gated: the `TopdownNode` fill at the `static_cast` level row, driven by `harness_topdown_run_test`, entry 185 here and entry 190 in the shared tree, skipped by the identity guard on this AMD host |
| `source/harness/statistics.cpp:84` | 1 | 1 | The comparison that closes the tie scan, `close-rows.md:97` and `:104` |

Projected CI union, a projection until the CI run exists: the host read
of 22 minus the seven topdown-family rows (the three at
`topdown_select.cpp:128` and `:492` and the four at `runner.cpp:839`)
answers 15. The ledger reading classes the three `topdown_select.cpp`
rows as allocation and enumeration arms, which stand dark on any host,
and that reading answers 18. Both figures sit under the constant of 21
and at or under the recorded union high-water of 19, so the pull
request's `coverage` job is expected green. The gate script frames the
same arithmetic at `tools/dbc/coverage_gate.sh:91` to `:107`: "The
allowance counts the union of the CI coverage job, where every entry
runs to its end. That union read 16 rows at `bea6af5b` (run
40983302766, artifact `coverage-info`), 16 at the head of specs/033, 18
at specs/032 and 19 at specs/035, so the accepted classes carry the
reading within two rows of the constant." The same paragraph closes "A
host run that reports them reports the host." The host read of 22 with
the seven host rows named above is the reading this machine gives, and
the local figure carries no claim about the CI run on its own.

The verify reads of the leg: the marker read
`git grep -c LCOV_EXCL -- source include | awk -F: '{s+=$2} END {print s}'`
answers 84, the ceiling; the new-marker read
`git diff -U0 0fce6651 -- source include | grep -c '^+.*LCOV_EXCL'`
answers 0; and `git diff --name-only 0fce6651 -- cmake/coverage.cmake
tools/dbc/coverage_gate.sh` prints nothing, which keeps the capture at
the tracked `--ignore-errors mismatch,negative` of
`cmake/coverage.cmake:73`. A grep for `inconsistent` over the gate log
answers 0. `genhtml` never ran at this read, the gate exiting before the
HTML command of `cmake/coverage.cmake:150` to `:159`, so the perl GD host
note of `docs/pages/building.md` carries nothing to record.

Superseded: the T027 leg at `b77ebf2a`, and the legs at `da32efe9`,
`2f56e9fc`, `9033535e` and `b0cd97e2` before it.

## T034 rerun, the complete analyzer read at the second rename head: green

Built head `b2763369963f761b5138cc97dd375876dd03eb0d`, the head of the
T024 to T027 rows above. Tree `build-tidy` only, configuration
`ci-ubuntu`, the shared `build` tree untouched. The rename of the two
tables in `test/source/harness_raw_events_test.cpp` cleared the two
promoted findings, and this read reaches every unit.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-ubuntu -B build-tidy` | `exit:0` | `/tmp/sg051-analyzer-cfg-r3.log` |
| `cmake --build build-tidy --target clean` | `exit:0`, an empty log | `/tmp/sg051-analyzer-clean-r3.log` |
| `cmake --build build-tidy -j 10` | `exit:0`, 265141 log lines, 326 `Building CXX object` lines: the 324 of the base read plus the two lane units `counters_raw_event_test.cpp` and `harness_raw_events_test.cpp` | `/tmp/sg051-head-analyzer-r3.log` |

The recorded tee form ran as a redirect to that same log path, so the
exit line names the build and not the pipe. No unit stands unreached at
this read.

Both grep reads of the amended task text, copied:

```
grep -cF '[readability-identifier-naming]' /tmp/sg051-head-analyzer-r3.log
0
grep -c 'readability-identifier-naming' /tmp/sg051-head-analyzer-r3.log
11
grep -c ': warning:.*readability-identifier-naming' /tmp/sg051-head-analyzer-r3.log
0
```

clang-tidy writes a promoted finding with the suffix
`-warnings-as-errors` inside the same brackets, as
`[readability-identifier-naming,-warnings-as-errors]`, so the literal
bracketed form of the recorded grep matches a run that holds no finding.
The plain read answers 11 here, and the read of
`NOLINT(readability-identifier-naming)` answers 11: every hit is an
echoed source line of the standing suppression comment at
`test/source/detail/gcov_flush.hpp:18`, quoted inside an unrelated
diagnostic. The base read of the same pair answered 10; the extra echo
comes from a lane unit that includes that header. The naming arm of
Principle VIII reads clean at this head.

Error severity at this read answers 3 lines, and the three lines of the
base read all reproduce:

```
/home/archerc/code/speedgun-ng/source/harness/runner.cpp:3081:53: error: syntax error [syntaxError]
/home/archerc/code/speedgun-ng/test/source/harness_jitter_probe_alloc_test.cpp:513:11: error: Non-local variable 'plantedEscapeSink' will use pointer to local variable 'allocation'. [danglingLifetime]
/home/archerc/code/speedgun-ng/test/source/harness_topdown_run_test.cpp:3558:3: error: Non-local variable 'selfPath' will use pointer to local variable 'self'. [danglingLifetime]
```

The syntax error sits at `source/harness/runner.cpp:3081`, the position
the base line at `:3005` reached after the lane edits. The
`harness_jitter_probe_alloc_test.cpp:513` line matches the base read at
the same line, columns and wording. The
`harness_topdown_run_test.cpp:3558` line matches the base read at the
same line, columns and wording: the statement there reads
`selfPath = self.c_str();` where `self` is a local `std::string`. The
shadow from the two earlier partial runs closes with this read, which
reaches the unit: the greps for its `Building CXX object` line and its
`Checking` line each answer 1. That file sits outside the lane path set,
and it is not in the lane diff, so it is not fixed here; the record owner
refers it to the owner of that code as a separate future item.

Per-file warning counts at a complete run, beside the base counts of
`results/base-readings.md`.

| File | Base | This read | Read |
| --- | --- | --- | --- |
| `source/counters/detail/pmu.hpp` | 117 | 126 | above base by 9, two lane units include the header |
| `source/counters/system.cpp` | 22 | 25 | above base by 3 |
| `source/counters/plan.cpp` | 38 | 38 | level |
| `source/counters/linux_pmu/provider.cpp` | 28 | 34 | above base by 6 |
| `source/counters/linux_pmu/group_io.cpp` | 47 | 49 | above base by 2 |
| `source/harness/runner.cpp` | 325 | 329 | above base by 4 |
| `source/harness/options.cpp` | 235 | 235 | level |

The base caveat for the runner figure stands quoted from
`results/base-readings.md`: "clang-tidy stops at a syntax error on
`source/harness/runner.cpp:3005`, so the 325 count covers the part of
that unit the analyzer reached." This read carries the same stop at
`:3081`, so the pair covers the same part of the unit. The four files
above base are files the lane edits, and the counts rise where the lane
adds code; the arm of the task compares each figure to its base reading,
and each stands recorded here for the owner's ruling.

One line for the file history: the record owner fixed a codespell hit in
the analyzer block above at `gates.md:1282` in place, a wording change
that moves no reading.

Superseded: the T034 read at `b77ebf2a` and the read at `da32efe9`, both
partial runs.

## The squashed head re-read (T040)

Head `a82144f3419809759e519ffda64b8c360c7d4c2d`, copied from a fresh read at
this write:

```
git rev-parse HEAD
a82144f3419809759e519ffda64b8c360c7d4c2d
```

T039 squashed the lane onto `0fce6651`, and the T040 amend folds this block
and the squash section of `results/commits.md` into that same commit. No
rebuild ran here. The shared `build` tree holds the `ci-rocky` binaries of
`b2763369963f761b5138cc97dd375876dd03eb0d`, and the first read of the next
section is the proof that the code surface of that built head equals the
code surface at this head.

### The tree readings that carry the heavy verdicts

```
git diff --name-only b2763369963f761b5138cc97dd375876dd03eb0d HEAD | grep -v '^specs/051-raw-events/' | wc -l
0
```

```
git log --oneline 0fce6651..HEAD | wc -l
1
```

```
cat /tmp/051-presquash.txt
257e89f43cb211b8190d807a401ffd6489032f3e
```

```
git diff --name-only 257e89f43cb211b8190d807a401ffd6489032f3e HEAD | wc -l
0
```

The first reading answers 0: the delta from the built head named in
`results/commits.md` reaches paths under `specs/051-raw-events/`. The last
reading answers 0: the squash left every byte of the tree where the
pre-squash head had it, the identity kept in `/tmp/051-presquash.txt`.
Principle VIII asks a recorded verdict to name the head it ran at, so these
two readings carry every T024, T025, T026, T027 and T034 verdict of this
file, each named at `b2763369`, up to head `a82144f3`. The middle reading
answers 1, the count of commits above `0fce6651`, which is the T039 verify
reading.

### The marker read

```
grep -rn LCOV_EXCL source test include | wc -l
84
```

The reading matches the ceiling recorded at T027. The split of the same 84:
84 over `source` and `include`, 0 over `test`. The new-marker read of the T027
leg answered 0, so the lane adds no marker.

### The four number greps of T022

```
grep -n '^    VERSION 0.35.0' CMakeLists.txt
87:    VERSION 0.35.0
```

```
grep -n '^    SOVERSION 8' CMakeLists.txt
182:    SOVERSION 8
```

```
grep -n '^    SOVERSION 22' CMakeLists.txt
466:    SOVERSION 22
```

```
grep -n 'kSchemaVersion = "1.16"' source/harness/json_report.cpp
58:constexpr std::string_view kSchemaVersion = "1.16";
```

Each line matches `results/numbers.md`: the project version 0.35.0 at
`CMakeLists.txt:87`, the counters number 8 at `:182`, the harness number 22
at `:466`, and the schema constant 1.16 at `source/harness/json_report.cpp:58`.

### The two-column acceptance run of T033

The binary in the shared `build` tree, run at this head:

```
./build/example/docs_command_line --iterations=1 --counter=cpu/instructions --counter=cpu/rc0
speedgun-ng 0.35.0 build=Release
bmRead/cells:256         iterations=         1  real time/iter (ns)=  270.000  cpu time/iter (ns)=  270.000  overhead floor (ns)=  150.000  endpoint samples=2  cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
bmWrite/cells:256        iterations=         1  real time/iter (ns)=  180.000  cpu time/iter (ns)=  180.000  overhead floor (ns)=  150.000  endpoint samples=2  cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
status 0
```

Both columns print on both report rows and each carries a positive figure,
`cpu/instructions=  773.000` and `cpu/rc0=  773.000`, the pair
`results/acceptance-host.md` records for its first read. The real-time,
cpu-time and floor figures are the figures of this host at this minute, and
the record frames them as host figures.

The build stamp of the binary that served the run matches the recorded one:

```
grep CMAKE_BUILD_TYPE:STRING build/CMakeCache.txt
CMAKE_BUILD_TYPE:STRING=Release
```

```
stat -c '%y %n' build/example/docs_command_line
2026-11-10 03:37:12.722375110 -0600 build/example/docs_command_line
```

`results/acceptance-host.md` records that same stamp beside that same binary
path, so the binary serving these reads is the binary of the built head that
file names. Log `/tmp/sg051-t033-r8.log`.

### The four fast legs

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0, an empty log | `/tmp/sg051-t028-r8.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0, the config line and nothing else | `/tmp/sg051-t029-r8.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0, an empty log | `/tmp/sg051-t031-r8.log` |
| T032 time source | `bash test/time_source_gate.sh .` | exit 0, `time_source_gate: clean`, no `file:line` hit | `/tmp/sg051-t032-r8.log` |

The spell leg reads green at this head, and that answer differs from the
recorded figure. The r7 cheap-gate block above reports exit 1 there over one
hit in this file at `gates.md:1282`, and the file-history line of the
analyzer block names the wording change that cleared that line. The whole
output of the leg at this read:

```
Used config files:
    1: .codespellrc
```

The liveness probe rows of the time-source leg, copied:

```
=== time-source gate liveness probe ===
  planted the banned call in the scratch root, the gate reported it
  planted the banned call in a .h header, the gate reported it

time_source_gate: clean
```

### The prose gate over the whole tree

```
python3 tools/prose/prose_gate.py --check prose --mode tree
prose-lint: 1124 sources, 229339 units examined, 0 findings, 0 skipped
```

exit 0. This read carries no `--paths` filter, so it covers every path
`git ls-files` lists at this head. The two whole-tree reads of
`specs/048-hwlat-probe/results/acceptance-final-intel.md` answered 1090 and
1093 sources, so the source count here moves with the records the lane adds.

### The screen line for this block

The standing rule for a chunk appended to a tracked record: screen it first.

```
python3 /tmp/sg051-tools/chunk-screen.py /tmp/sg051-tools/squashed-head-reread.md
/tmp/sg051-tools/squashed-head-reread.md: codespell exit 0, 140 units, clean
```

### Ordering and supersession

Every read in this block ran against the tree at `a82144f3` before this block
landed, so the block holds no reading of its own text, and the screener above
covers that text. The screener counts the units of the chunk, and this file's
unit count moves when the block lands, which is the reading the next leg
takes.

Superseded: the r7 cheap-gate rows of the second constant-case rename block,
T028 to T032. The heavy legs keep their named head `b2763369`, and the tree
readings above carry them to `a82144f3`.

## The cheap gates re-read at the squashed head (T028 to T032)

Superseded: the four fast-leg rows of the squashed-head block, which
read `a82144f3`, and the five rows of the second constant-case rename
block that those rows superseded in turn. The five rows below read
`c0a73e98e159026b97740c0950956ab891e4286f`, copied from
`results/commits.md` and titled `Harness: Add raw event-code counter
columns`. `git rev-parse HEAD` answers that same sha, so the checkout
stands at the built head. The diff between `b2763369` and this head
carries seven lines of `source/counters/linux_pmu/provider.cpp`, one
attribute, so the format leg re-read moved source. Host reads match the
blocks above: `Gary`, `cmake version 4.4.5`, `Python 3.14.9`,
`clang-format version 18.1.8`.

| Task | Command | Verdict line | Log |
| --- | --- | --- | --- |
| T028 format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with an empty log; `clang-format --version` reads `clang-format version 18.1.8` | `/tmp/sg051-t028-r9.log` |
| T029 spell | `cmake -P cmake/spell.cmake` | exit 0; the log reads `Used config files:` over one config, `.codespellrc`, and names no misspelling in any file | `/tmp/sg051-t029-r9.log` |
| T030 prose, commit range then lane tree | `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`, then `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | exit 0 and exit 0: `prose-lint: 36 sources, 6958 units examined, 0 findings, 0 skipped`, then `prose-lint: 20 sources, 6273 units examined, 0 findings, 0 skipped` | `/tmp/sg051-t030a-r9.log`, `/tmp/sg051-t030b-r9.log` |
| T031 page drift | `python3 tools/docs/example_drift.py .` | exit 0 with an empty log | `/tmp/sg051-t031-r9.log` |
| T032 time source | `bash test/time_source_gate.sh .` | `time_source_gate: clean`, with no `file:line` hit; the probe rows read the planted banned call reported in the scratch root and in a `.h` header | `/tmp/sg051-t032-r9.log` |

The counts moved with the squash. The range read over the base
`0fce6651` now walks one commit and answers 36 sources and 6958 units,
and the tree read over the lane covers 20 sources and 6273 units, two
record files more than the r6 and r7 reads: `results/host-acceptance.md`
and `results/pr-body.md`. T031 carries the three readings the task
names: the option row table at `tools/docs/example_drift.py:567` to
`:581`, the page line cap at `tools/docs/example_drift.py:256`, and the
forbidden class scan of the two User-guide pages.

The standing screen for a chunk bound for this file, taken over the
chunk below before it landed:

```
python3 /tmp/sg051-tools/chunk-screen.py /tmp/sg051-tools/cheap-r9.md
/tmp/sg051-tools/cheap-r9.md: codespell exit 0, 33 units, clean
```

# The legs at the squashed head with the consumer-release fix

Built head `c0a73e98e159026b97740c0950956ab891e4286f`, named in the
section "The squash" of `results/commits.md` as the identity after the
second amend, and read from `git rev-parse HEAD` as the checkout itself.
The lane stands as one commit titled `Harness: Add raw event-code
counter columns` on top of `0fce6651`. The read
`git diff --name-only b2763369963f761b5138cc97dd375876dd03eb0d HEAD`
lists `source/counters/linux_pmu/provider.cpp` beside five record paths:
the one attribute `[[maybe_unused]]` at
`source/counters/linux_pmu/provider.cpp:808` on `rawStateRule`, the
contract-only helper whose single call sits inside an `SG_ENSURE`. Each
leg below supersedes its counterpart at
`b2763369963f761b5138cc97dd375876dd03eb0d`.

## T024, the full suite in the shared `build` tree

Tree `build`, configuration `ci-rocky`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++`, `CMAKE_BUILD_TYPE=Release`,
the analyzer cache variables absent after the purge read.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK` | `exit:0` | `/tmp/sg051-c8-t024-purge.log` |
| `cmake --preset=ci-rocky` | `exit:0` | `/tmp/sg051-c8-t024-cfg.log` |
| `cmake --build build -j 10` | `exit:0`, 1 object recompiled for the attribute, 0 `warning:` lines | `/tmp/sg051-c8-t024-build.log` |
| `ctest --test-dir build --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 192`, no `Failed` line, no `Exception` line, 0 `Not Run` lines, 69.86 sec | `/tmp/sg051-c8-t024-suite.log` |

The count read `ctest --test-dir build -N` closes with
`Total Tests: 192`. Skip entry: `190 - harness_topdown_run_test
(Skipped)`, the host identity guard. 192 registered, 191 executed, 1
skipped. Entries `55 - counters_raw_event_test` and
`155 - harness_raw_events_test` read `Passed`.

Alone reads of the two lane units: `build/test/counters_raw_event_test`
answers `exit:0` with 19 `case` lines and 0 `FAIL` lines, closing
`counters_raw_event_test PASS: raw addresses over the scripted kernel
and sysfs trees` (`/tmp/sg051-c8-raw-cases.log`), and
`build/test/harness_raw_events_test` answers `exit:0` with 12 `case`
lines and 0 `FAIL` lines, closing `harness_raw_events_test: ok`
(`/tmp/sg051-c8-raw-scenarios.log`).

Host state for the timing entries: `load average: 18.55, 14.12, 12.85` at
06:11:38 when the suite finished.

Superseded: the T024 leg at `b2763369`, and the legs at `b77ebf2a`,
`da32efe9` and `9033535e` before it.

## T025, the address and undefined-behavior sanitizer leg

Tree `build/sanitize`, configuration `ci-sanitize`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++` from the recorded
`CXX=clang++ cmake --preset=ci-sanitize`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-sanitize -B build/sanitize` | `exit:0` | `/tmp/sg051-c8-t025-cfg.log` |
| `cmake --build build/sanitize -j 10` | `exit:0`, 1 object recompiled for the attribute | `/tmp/sg051-c8-t025-build.log` |
| `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 187`, no `Failed` line, 0 `Not Run` lines, 87.30 sec | `/tmp/sg051-c8-t025-suite.log` |

The sanitizer greps over that log answer 0 for `ERROR: AddressSanitizer`,
0 for `runtime error:`, 0 for `LeakSanitizer`, 0 for
`SUMMARY: AddressSanitizer` and 0 for `detected memory leaks`. Entry
`58 - counters_overhead` reads `Passed 0.20 sec` and the grep for
`published floor` answers 0, at a host reading
`load average: 17.79, 15.13, 13.49` at 06:17:41. The count read
`ctest --test-dir build/sanitize -N` closes with `Total Tests: 187`: five
`dbc_matrix_*` entries stand absent there, the `SPEEDGUN_DBC_MATRIX`
variable reading OFF outside `ci-rocky` and `ci-tsan`. Skip entry:
`185 - harness_topdown_run_test (Skipped)`, the host identity guard.

Superseded: the T025 leg at `b2763369`, the leg at `b77ebf2a`, and the
legs at `da32efe9` and earlier heads before them.

## T026, the thread-sanitizer leg

Tree `build/tsan`, configuration `ci-tsan`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++` from the recorded
`CXX=clang++ cmake --preset=ci-tsan`.

| Command | Verdict | Log |
| --- | --- | --- |
| `CXX=clang++ cmake --preset=ci-tsan -B build/tsan` | `exit:0` | `/tmp/sg051-c8-t026-cfg.log` |
| `cmake --build build/tsan -j 10` | `exit:0`, 1 object recompiled for the attribute | `/tmp/sg051-c8-t026-build.log` |
| `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10` | `exit:0`, `100% tests passed out of 188`, no `Failed` line, no `Timeout` line, 0 `Not Run` lines, 201.73 sec | `/tmp/sg051-c8-t026-suite.log` |

The grep for `WARNING: ThreadSanitizer` over that log answers 0. Entry
`130 - harness_execution_context_test` reads `Passed 0.19 sec`, the entry
that stalled at `b2763369`, and entry `131 -
harness_context_barrier_test` reads `Passed 0.26 sec`, so the standing
note of about one run in ten for that entry under a thread sanitizer on
this machine stands unexercised at this leg. The count read
`ctest --test-dir build/tsan -N` closes with `Total Tests: 188`, four
allocation-operator units guarded out under the thread sanitizer at
`test/CMakeLists.txt:328-334` and `:1085-1087`. Skip entry:
`186 - harness_topdown_run_test (Skipped)`, the host identity guard.
Host state: `load average: 18.29, 16.27, 14.39` at 06:22:50.

Superseded: the T026 leg at `b2763369`, the leg at `b77ebf2a`, and the
legs at `da32efe9` and earlier heads before them.

## T034, the analyzer read at the squashed head

Built head `c0a73e98e159026b97740c0950956ab891e4286f`. Tree `build-tidy`
only, configuration `ci-ubuntu`, the shared `build` tree untouched. The
read is a complete run: every unit reaches the analyzer.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-ubuntu -B build-tidy` | `exit:0` | `/tmp/sg051-analyzer-cfg-r4.log` |
| `cmake --build build-tidy --target clean` | `exit:0`, an empty log | `/tmp/sg051-analyzer-clean-r4.log` |
| `cmake --build build-tidy -j 10` | `exit:0`, 265147 log lines, 326 `Building CXX object` lines: the 324 of the base read plus the two lane units | `/tmp/sg051-head-analyzer-r4.log` |

The recorded tee form ran as a redirect to that same log path, so the
exit line names the build and not the pipe.

Both grep reads of the amended task text, copied:

```
grep -cF '[readability-identifier-naming]' /tmp/sg051-head-analyzer-r4.log
0
grep -c 'readability-identifier-naming' /tmp/sg051-head-analyzer-r4.log
11
grep -c ': warning:.*readability-identifier-naming' /tmp/sg051-head-analyzer-r4.log
0
```

clang-tidy writes a promoted finding with the suffix
`-warnings-as-errors` inside the same brackets, as
`[readability-identifier-naming,-warnings-as-errors]`, so the literal
bracketed form of the recorded grep matches a run that holds no finding.
The plain read answers 11, and the read of
`NOLINT(readability-identifier-naming)` answers 11: each hit is an echoed
source line of the standing suppression comment at
`test/source/detail/gcov_flush.hpp:18`. The naming arm reads clean.

Error severity answers 3 lines, and the three lines of the base read
reproduce at the same positions:

```
/home/archerc/code/speedgun-ng/source/harness/runner.cpp:3081:53: error: syntax error [syntaxError]
/home/archerc/code/speedgun-ng/test/source/harness_jitter_probe_alloc_test.cpp:513:11: error: Non-local variable 'plantedEscapeSink' will use pointer to local variable 'allocation'. [danglingLifetime]
/home/archerc/code/speedgun-ng/test/source/harness_topdown_run_test.cpp:3558:3: error: Non-local variable 'selfPath' will use pointer to local variable 'self'. [danglingLifetime]
```

The `harness_topdown_run_test.cpp:3558` line sits outside the lane path
set and outside the lane diff, and stands unfixed here as the record
owner ruled for that separate item.

Per-file warning counts, beside the base counts of
`results/base-readings.md`:

| File | Base | This read | Read |
| --- | --- | --- | --- |
| `source/counters/detail/pmu.hpp` | 117 | 126 | above base by 9, two lane units include the header |
| `source/counters/system.cpp` | 22 | 25 | above base by 3 |
| `source/counters/plan.cpp` | 38 | 38 | level |
| `source/counters/linux_pmu/provider.cpp` | 28 | 34 | above base by 6 |
| `source/counters/linux_pmu/group_io.cpp` | 47 | 49 | above base by 2 |
| `source/harness/runner.cpp` | 325 | 329 | above base by 4 |
| `source/harness/options.cpp` | 235 | 235 | level |

The base caveat for the runner figure stands quoted from
`results/base-readings.md`: "clang-tidy stops at a syntax error on
`source/harness/runner.cpp:3005`, so the 325 count covers the part of
that unit the analyzer reached." This read carries the same stop at
`:3081`. The `[[maybe_unused]]` attribute on `rawStateRule` at
`source/counters/linux_pmu/provider.cpp:808` answers 0 diagnostic lines
for that helper in this tree, where the helper was already reachable: the
attribute addresses the consumer-release shape, read next.

Superseded: the T034 reads at `b2763369`, `b77ebf2a` and `da32efe9`.

## T027, the coverage gate at the squashed head

Tree `build/coverage`, configuration `ci-coverage`, cache read
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++` and `CMAKE_BUILD_TYPE=Coverage`.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-coverage -B build/coverage` | `exit:0` | `/tmp/sg051-c8-t027-cfg.log` |
| `cmake --build build/coverage -j 10` | `exit:0`, 1 object recompiled for the attribute, 294 `.gcno` files | `/tmp/sg051-c8-t027-build.log` |
| `find build/coverage -name '*.gcda' -delete` | `exit:0` over an empty log, and the tree holds 0 `.gcda` files after it, so the trace names one run | `/tmp/sg051-c8-t027-gcda.log` |
| `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `exit:0`, `100% tests passed out of 187`, 187 registered, 186 executed, 1 skipped, 0 `Not Run` lines, 593.43 sec | `/tmp/sg051-c8-t027-suite.log` |
| `cmake --build build/coverage -t coverage` | `exit:2`, the line arm at `100.0%` and the branch arm one row over the constant | `/tmp/sg051-c8-t027-coverage.log` |

The gate block on this host, copied:

```
Summary coverage rate:
  source files: 87
  lines.......: 100.0% (18930 of 18930 lines)
  functions...: 99.0% (1983 of 2003 functions)
  branches....: 99.8% (9949 of 9971 branches)
coverage-gate: branch shortfall 22 rows exceeds the  D-14 recorded allowance of 21
```

The line arm reads `100.0%` over 18930 of 18930, and a read of the `DA:`
rows of `build/coverage/coverage.info` counts zero rows held at zero hits.
The branch arithmetic on this host: 9971 minus 9949 answers 22 rows, and
the constant reads `allowance=21` at `tools/dbc/coverage_gate.sh:108`. The
printed verdict is the exceeds arm of `tools/dbc/coverage_gate.sh:125` to
`:126`, quoted above; the within-allowance message of
`tools/dbc/coverage_gate.sh:121` to `:123` stands unprinted. Entry
`58 - counters_overhead` reads `Passed 0.30 sec` in this suite.

The 22-row set matches the reads at `b2763369`, `b77ebf2a` and
`da32efe9`: none joined, none closed. The lane share stands at 0 rows.
The prior readings stay in the record above: 37 rows at `b0cd97e2` with 15
on lane-added lines, and 25 rows at `9033535e` with 3 on lane-added lines.

Census of the 22 rows, the positions as lcov names them, copied from the
list the gate itself printed with the awk class of
`tools/dbc/coverage_gate.sh:132` to `:139`, each row beside the
attribution it carries.

| File and line | Branch positions | Rows | Attribution read |
| --- | --- | --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:128` | 2 and 3 | 2 | Allocation landing pad on the two `std::string` constructions inside one `push_back`, `specs/045-merge-fixes/results/close-rows.md:80` and `:86` |
| `source/counters/linux_pmu/topdown_select.cpp:492` | 5 | 1 | The gcov arm past a closed enumeration of `TopdownUnsupport`, `close-rows.md:96` and `:103` |
| `source/counters/plan.cpp:476` | 1 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/cli.cpp:280` | 8, 9, 10 and 11 | 4 | Allocation landing pad on the concatenation behind the ternary of the failure line, `close-rows.md:81` and `:87` |
| `source/harness/host_record_check.cpp:266` | 8 and 9 | 2 | The unwind tail of the `EvidenceRow` construction, the ledger's own row two at `tools/dbc/coverage_gate.sh:70` to `:90` |
| `source/harness/hunt_ranks.cpp:127` | 3 | 1 | The rank spread and the least-squares denominator of the same points, `close-rows.md:99` and `:106` |
| `source/harness/noise_tracer.cpp:287` | 2 | 1 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/noise_tracer.cpp:686` | 10 and 11 | 2 | No ledger row names this position in the records read here; the trace holds it at taken zero |
| `source/harness/options.cpp:1541` | 8 and 9 | 2 | Allocation landing pad inside the refusal lambda of `applySourceValue`, `close-rows.md:83` and `:89` |
| `source/harness/report.cpp:815` | 2 | 1 | The second operand of the fold guard, read at `close-rows.md:98` and `:105` for the same guard at line 814 of that head |
| `source/harness/runner.cpp:839` | 2, 3, 4 and 5 | 4 | Host gated: the `TopdownNode` fill at the `static_cast` level row, driven by `harness_topdown_run_test`, entry 185 here and entry 190 in the shared tree, skipped by the identity guard on this AMD host |
| `source/harness/statistics.cpp:84` | 1 | 1 | The comparison that closes the tie scan, `close-rows.md:97` and `:104` |

### The measured CI union, and why the projection is gone

An earlier row in this file projected the CI union by subtracting the
seven host-gated topdown rows from the host read. That projection is
void: the CI coverage job answers the union directly, and its reading at
the base of this branch already stands above the constant. Step-wise,
with run and job ids.

1. Base of this branch, `0fce6651`, run `42753235724`, job `129621827979`:
   `lines.......: 100.0% (18793 of 18793 lines)` and
   `branches....: 99.7% (9874 of 9900 branches)`, a union of 26 rows, with
   the verdict `coverage-gate: branch shortfall 26 rows exceeds the  D-14
   recorded allowance of 21`. That run skips
   `183 - harness_topdown_run_test (Skipped)`, so the topdown family stands
   unreached on the runner as it does here.
2. Master before the base, `1f1f9b20`, run `42734353129`, job
   `129560071404`: `branches....: 99.8% (9876 of 9900 branches)`, a union
   of 24 rows, verdict `coverage-gate: branch shortfall 24 rows exceeds
   the  D-14 recorded allowance of 21`.
3. This head, `c0a73e98`, run `42812457393`, job `129817529424`:
   `lines.......: 100.0% (18945 of 18945 lines)`,
   `functions...: 98.8% (1978 of 2003 functions)` and
   `branches....: 99.7% (9948 of 9973 branches)`, a union of 25 rows,
   verdict `coverage-gate: branch shortfall 25 rows exceeds the  D-14
   recorded allowance of 21`. That run skips
   `185 - harness_topdown_run_test (Skipped)`, and its ctest step answers
   `100% tests passed out of 187`.

The lane moved the CI union from 26 rows to 25, one row better, and the
excess over the constant stands at both heads: it predates the lane.
The constant `allowance=21` reads stale against current master. The
owner ruling packet is the census above, and the lane delta it carries is
zero rows at the host read (22 rows, lane share 0) and one row better at
the CI read (26 to 25).

The host and the runner instrument different surfaces at the same head,
which the numbers state directly: 18930 lines and 9971 branch records
here, 18945 lines and 9973 branch records on the runner.

Parity, step by step: the coverage job answers red at master
(`42734353129`, 24 rows), red at the base of this branch
(`42753235724`, 26 rows) and red at this head (`42812457393`, 25 rows),
each with the same exceeds verdict, and the coverage leg on this host
answers red at the same shape. A green coverage job exists at no head in
this set, so the job red at this head mirrors the job red at master.

The verify reads of the leg: the marker read answers 84, the ceiling; the
new-marker read answers 0; and `git diff --name-only 0fce6651` over
`cmake/coverage.cmake` and `tools/dbc/coverage_gate.sh` prints nothing,
which keeps the capture at the tracked
`--ignore-errors mismatch,negative` of `cmake/coverage.cmake:73`. A grep
for `inconsistent` over the gate log answers 0. `genhtml` never ran at
this read, the gate exiting before the HTML command of
`cmake/coverage.cmake:150` to `:159`.

Superseded: the T027 leg at `b2763369`, and the legs at `b77ebf2a`,
`da32efe9`, `9033535e` and `b0cd97e2` before it.

## The consumer-release reproduce and the CI parity reads at the squashed head

### The consumer-release leg, run locally

Tree `build/consumer-release`, the consumer shape: the `ci-linux-ignore`
preset, which carries `speedgun-ng_CONTRACTS:STRING=ignore`, beside
`CMAKE_BUILD_TYPE=Release` and `speedgun-ng_DEVELOPER_MODE=OFF`. No
earlier leg in this file builds that semantic on this host.

| Command | Verdict | Log |
| --- | --- | --- |
| `cmake --preset=ci-linux-ignore -B build/consumer-release` with `-D CMAKE_BUILD_TYPE=Release` and `-D speedgun-ng_DEVELOPER_MODE=OFF` | `exit:0` | `/tmp/sg051-consumer-cfg2.log` |
| `cmake --build build/consumer-release -j 10` | `exit:2`, 28 log lines, 10 `Building CXX object` lines | `/tmp/sg051-consumer-build2.log` |

The one error line the build prints:

```
/home/archerc/code/speedgun-ng/source/harness/noise_tracer.cpp:499:6: error: ‘bool sg::detail::{anonymous}::activationAnswered(const sg::PrecheckTracerCpuRecord&, bool)’ defined but not used [-Werror=unused-function]
```

The grep for `rawStateRule` over that log answers 0: the attribute at
`source/counters/linux_pmu/provider.cpp:808` cleared the diagnostic that
the CI consumer-release job printed at the pre-amend head. The mechanism
stands recorded in `specs/051-raw-events/results/converge-check-1.md:214`
to `:219`: at that cache value `SG_ENSURE` expands to `((void)0)` at
`include/speedgun-ng/dbc.hpp:463`, and the only call of `rawStateRule`
sat inside the `SG_ENSURE` at `provider.cpp:852`.

The helper in the error above is base debt. `specs/048-hwlat-probe`
records it as a downstream build defect deferred under ruling 36, at
`results/acceptance-final-intel.md:1690` to `:1706`, where the same
compiler line is copied, `git log -S activationAnswered` places the
helper in `c9187ba3`, and the reproduction pair of commands is the pair
above. `source/harness/noise_tracer.cpp` sits outside the lane path set,
so the row stands referred to the owner of that code.

### The CI parity reads, step by step

Three jobs answer red at this head, and each one answers red at master
for the same reason. Reads come from run `42812457393` at
`c0a73e98`, the base run `42753235724` at `0fce6651`, and the master run
`42734353129` at `1f1f9b20`.

1. `consumer-release`. This head, job `129817529444`, prints one error
   line in the `Build consumer library` step. It names
   `source/harness/noise_tracer.cpp` at line 499, column 6, carries the
   `activationAnswered` text and the `-Werror=unused-function` tag, and
   sits under the runner checkout path
   `/home/runner/work/speedgun-ng/speedgun-ng/`. The master job
   `129560071616` prints the same line, byte for byte, same file, line
   499, column 6, same message. The local read above
   prints the same message with this checkout's path in place of the
   runner path. The CI configure step names the same preset and the same
   two cache values as the local command; the CI build step runs
   `cmake --build build/consumer-release -j "$(nproc)"` where the local
   read used `-j 10`. Parity stands, and this head adds no diagnostic of
   its own to that job.
2. `coverage`. Covered in the T027 row above: 25 rows at this head, 26 at
   the base, 24 at the older master head, each over the constant of 21.
   Parity stands.
3. `test-rocky`. This head, job `129817529375`, answers
   `99% tests passed, 2 tests failed out of 192`, with the step line
   `##[error]Process completed with exit code 8.` The two failing entries
   read `30 - prose_gate_fixtures (Failed)` and
   `192 - harness_hunt_topdown_test (Failed)`. The base job
   `129621828079` answers `99% tests passed, 2 tests failed out of 190`
   with `30 - prose_gate_fixtures (Failed)` and
   `190 - harness_hunt_topdown_test (Failed)`. The master job
   `129560071461` answers `99% tests passed, 2 tests failed out of 190`
   with the same two names. The failing-name set at this head holds those
   two names, so the lane adds no entry name; the entry numbers shift by
   the two entries the lane registers.

Producer reads for the two names, read in all three logs, identical
there:

- `prose_gate_fixtures`: the fixture line
   `git ls-files failed in /__w/speedgun-ng/speedgun-ng (exit 128): fatal:
   detected dubious ownership in repository at
   '/__w/speedgun-ng/speedgun-ng'`, the runner checkout ownership state.
- `harness_hunt_topdown_test`: the case line
   `harness_hunt_topdown_test oneCallReadsNoSourceOfItsOwn: FAIL` with the
   summary `HARNESS HUNT TOPDOWN TEST FAIL: three calls leave the process
   holding the mappings it held before, so no sampled page stands behind
   an area (FR-001, D-7)`, a mapping-residency claim the Rocky runner does
   not settle.

Three entries skip in each of those jobs: `counters_pmu_test`,
`counters_overhead` and `harness_topdown_run_test`, the numbers shifting
with the registered count. `specs/049-names-old-records` carries the same
comparison at `results/converge-check-4.md:180` to `:192`, naming the
three red jobs at the base run and the same three red at the master run
with their job numbers.

Note on one log read: the job number `129560071458` in the master run
names the Ubuntu `test` job, which answers
`100% tests passed out of 190`; the master `test-rocky` job in that run
is `129560071461`, read above.

## Identity chain note for the rows above (fold amend)

Amends move the branch identity. The rows above that state `git
rev-parse HEAD` answering `c0a73e98...` were true at their read times;
the converge fold amend moved the identity again, so the current
identity answers a fresh `git rev-parse HEAD`, the full chain from
`257e89f4` through `a82144f3`, `f91d6b6f` and `c0a73e98` to the fold
stands in `results/commits.md`, and the tree-equality hop that carries
every row above is the read of `git diff --name-only` over
`c0a73e98e159026b97740c0950956ab891e4286f` against `HEAD` with a
`grep -vc` over the specs prefix, answering 0 at the fold identity. No
leg was re-run by that amend, and none needs to be: the delta is
record paths.

## The prose legs at the fold identity: a correction

The identity-chain note above closes with the sentence that no leg was
re-run by the fold amend, the delta being record paths. That holds for
the heavy legs, whose inputs are source, test and build files: the
shared suite, the two sanitizer legs, the coverage leg and the analyzer
leg read no record file. The prose legs are the exception, because those
scan the record files themselves, so both prose forms were read again at
the fold identity.

Identity read fresh: `git rev-parse HEAD` answers
`bd82911853056a71d735c2d0e01edbc4102fa9ec`, one commit titled
`Harness: Add raw event-code counter columns` on top of `0fce6651`.

| Read | Verdict | Log |
| --- | --- | --- |
| `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` | `exit:0`, `prose-lint: 20 sources, 6666 units examined, 0 findings, 0 skipped` | `/tmp/sg051-c8-prose-tree.log` |
| `python3 tools/prose/prose_gate.py --check prose --mode range --base 0fce6651` | `exit:0`, `prose-lint: 35 sources, 7339 units examined, 0 findings, 0 skipped` | `/tmp/sg051-c8-prose-range.log` |
| `codespell specs/051-raw-events` | `exit:0`, 0 findings | read over the record directory |

The CI `prose-lint` job answers `success` at run `42812457393`, the run
the parity reads above come from.

Untracked records: `git status --porcelain` reads five converge-check
records in the worktree as untracked, the three of
`specs/051-raw-events/results/` numbered 1, 2 and 3, and the two of
`specs/049-names-old-records/results/` numbered 3 and 4. The two my rows
quote stand in that set: the lane's `converge-check-1.md`, quoted in the
consumer-release block above, and the 049 record
`converge-check-4.md`, quoted in the `test-rocky` parity text. Those
paths leave the prose scans because both forms work from tracking: tree
mode lists `git ls-files`, and range mode keeps only the paths present
in `git ls-tree` at the head, both reads inside `collect_candidates` of
`tools/prose/prose_gate.py`. The same posture stands in the earlier
records at `specs/049-names-old-records/results/converge-check-3.md:17`
to `:19` and `converge-check-4.md:25` to `:27`.

The spell form behaves otherwise: codespell walks the working tree and
reads those records tracked or not, under the skip list of
`.codespellrc`, so untracking changes the prose scan and leaves the
spell read as it stands.

## The coverage union at the fold identity, and the analyzer legs at that run

The fold coverage read comes from run `42823530279` at
`bd82911853056a71d735c2d0e01edbc4102fa9ec`, job `129855338186`, log
`/tmp/sg051-ci-cov-fold.log` at 2918 lines. The suite arm reads
`100% tests passed out of 187`, `Total Test time (real) = 647.23 sec`,
and one skip entry, `185/187 Test #185: harness_topdown_run_test`. The
capture arm reads:

```
coverage	Process coverage info	2026-11-10T13:55:58.5688996Z   lines.......: 100.0% (18944 of 18944 lines)
coverage	Process coverage info	2026-11-10T13:55:58.5689514Z   functions...: 98.8% (1978 of 2003 functions)
coverage	Process coverage info	2026-11-10T13:55:58.5690025Z   branches....: 99.7% (9948 of 9973 branches)
coverage	Process coverage info	2026-11-10T13:55:58.5846362Z coverage-gate: branch shortfall 25 rows exceeds the  D-14 recorded allowance of 21
```

The gate line carries two spaces between `the` and `D-14`, copied as the
tool writes it. The trace behind the read holds 0 dark line rows.

### The dark branch rows across the CI reads and the host read

The row sets come from each run's `coverage-info` artifact, which
GitHub serves before the log of an unfinished run: artifact
`13771090246` at this head, `13735946243` at the base run
`42753235724`, `13727074116` at the master run `42734353129`, and
`13765032205` at run `42812457393`. The three CI sets are saved at
`/tmp/sg051-cov-fold-darkbr.txt`, `/tmp/sg051-cov-base-darkbr.txt` and
`/tmp/sg051-cov-master-darkbr.txt`. Counts: 26 rows at the base, 24 rows
at that master head, 25 rows at this head, 22 rows on the host tree.

This lane turns two dark rows green, both in threaded-graph gating:
`source/harness/graph_gate.cpp line 98 branch 3` and
`source/harness/graph_gate.cpp line 206 branch 1` stand dark at the base
and taken at this head. This lane puts one row dark:
`source/counters/plan.cpp line 476 branch 1` is taken at the base and
dark at this head, and it stands among the 22 host rows too. Eight
further rows are the same statements at new numbers: 395 to 410 and 439
to 454 in `group_io.cpp`, 785 to 1005 in `provider.cpp`, and 838 to 839
in `runner.cpp` across branches 2, 3, 4 and 5. Net movement: two rows
down, one row up, one row of the base union left standing at a new
number for each of the eight.

Pairing a dark row by file and line number across heads is not a
pairing by statement, since this lane adds lines in four of the files
named above. The blame read settles the question at the head: `git
blame` at `HEAD` places every line named in that list on a commit that
precedes the lane, the five commits being `4677356ec5`, `3639c4a53a`,
`a075ba7096`, `9a79fc021b` and `0c37f2fd2f`. The reading the evidence
supports is that no dark row at this head sits on a line the lane
wrote, and what the lane changes is which statements execute.

### Three rows the CI union reports that the host union does not

At this tree the CI set holds three dark rows absent from the host
union: `source/counters/linux_pmu/group_io.cpp line 410 branch 1`,
`source/counters/linux_pmu/group_io.cpp line 454 branch 0` and
`source/counters/linux_pmu/provider.cpp line 1005 branch 1`. Every host
dark row stands in the CI set as well, so the two sets differ by those
three rows alone. The shortfall reads 22 rows against the constant on
the host tree and 25 rows against the same constant in CI.

### The line total moves by one between two CI reads of one tree

Run `42812457393` at `c0a73e98` reported `18945 of 18945 lines` and this
run reports `18944 of 18944 lines`, while the branch arm matches
exactly: both traces hold 9973 branch points with 9948 taken. A
key-by-key read of the two traces finds one `DA` record present in the
earlier run and absent here, at `source/harness/execution_context.cpp`
line 63, and no `DA` record added. In that file at `HEAD`, line 62 is
the `#endif` that closes the architecture conditional inside
`pauseHint()` and line 63 is that function's closing brace, so the
movement is in where the return-path counter attaches. The lane touched
that file nowhere: the path list from `git diff --name-only 0fce6651..HEAD`
holds no `execution_context.cpp`.

### The static-analysis legs at this run and at the earlier run

Run `42823530279` concludes `cancelled`. The library leg
`129855338732` and the examples leg `129855339013` completed `success`
from 13:20:56Z to 13:31:25Z and from 13:20:56Z to 13:41:10Z. The tests
leg `129855338839` completed `cancelled` from 13:21:06Z to 14:13:41Z,
and the `docs` job `129883987581` went with it. That leg's `updatedAt`
never moved past 13:21:06Z while it stood in progress, and that is the
reading that makes it look stalled. The same leg at run `42812457393`,
job `129817529348`, ran from 12:04:07Z to 12:57:24Z and completed
`success`, 53 minutes 17 seconds against the 52 minutes 35 seconds this
run reached before the cancel. No tests-scope artifact exists at this
identity; the artifacts of this run are `dbc-matrix`,
`analyzer-findings-library` and `analyzer-findings-examples`.

On the same code surface, the earlier tests leg carries two of the three
error-severity lines my local analyzer reads report, copied from
`/tmp/sg051-ci-sat-ours.log` at 188675 lines:

```
/home/runner/work/speedgun-ng/speedgun-ng/test/source/harness_jitter_probe_alloc_test.cpp:513:11: error: Non-local variable 'plantedEscapeSink' will use pointer to local variable 'allocation'. [danglingLifetime]
/home/runner/work/speedgun-ng/speedgun-ng/test/source/harness_topdown_run_test.cpp:3558:3: error: Non-local variable 'selfPath' will use pointer to local variable 'self'. [danglingLifetime]
```

Both match my local reads byte for byte past the checkout path prefix.
The third line, the `syntaxError` finding my local reads place at
`source/harness/runner.cpp:3081`, stands in neither CI analyzer leg of
that run: `grep -c 'syntax error'` and `grep -c 'syntaxError'` answer
0 in the tests log and 0 in the library leg `129817529440`, whose log is
`/tmp/sg051-ci-sal-ours.log` at 49254 lines. That finding is a local
instrument reading at this tree.

## Cancellation note for run 42823530279

The run was cancelled by the orchestrator on a frozen-updatedAt signal
read as a wedge, against a mistaken ten-minute premise for the
static-analysis tests leg. The block above corrects the premise: the
norm is fifty-three minutes and the leg was inside it. The cancel was
therefore hasty and its only effect was to free the finished-job logs
early and end a healthy leg; the tree, the branch, and every recorded
verdict stand untouched, and the legs identity carries the same analyzer
readings from run 42812457393.

## The CI parity read at the live identity

Run `42832748234` at `d61e0b185daaad9ed9dab503a1d49429be89fc16`
completed `failure` from 14:18:28Z to 14:53:57Z. The three red jobs are
consumer-release `129887350147`, test-rocky `129887350133` and coverage
`129887350019`. Every other job repeats its earlier verdict: `lint`,
`test`, `sanitize`, `tsan`, `dbc-gate`, `downstream-consumer`,
`shared-audit`, `prose-lint`, `static-analysis (library)`,
`static-analysis (tests)` and `static-analysis (examples)` completed
`success`, and the `docs` job `129905655194` was `skipped`. That tests
analyzer leg, `129887349990`, ran 14:19:26Z to 14:50:02Z to `success`,
so the cancellation note above stands as the reading of that leg at the
earlier identity.

### The consumer-release stop, copied

```
[ 91%] Building CXX object CMakeFiles/speedgun-ng_harness.dir/source/harness/noise_tracer.cpp.o
/home/runner/work/speedgun-ng/speedgun-ng/source/harness/noise_tracer.cpp:499:6: error: ‘bool sg::detail::{anonymous}::activationAnswered(const sg::PrecheckTracerCpuRecord&, bool)’ defined but not used [-Werror=unused-function]
gmake[2]: *** [CMakeFiles/speedgun-ng_harness.dir/build.make:415: CMakeFiles/speedgun-ng_harness.dir/source/harness/noise_tracer.cpp.o] Error 1
```

The log is `/tmp/sg051-ci2-cons.log` at 1388 lines. `grep -c
rawStateRule` answers 0 in it and 0 in the consumer-release log of run
`42812457393`, so the attribute fix of that head holds here too. The
error lines and the make lines of that step match the earlier log
line for line.

### The test-rocky failures, keyed by name

`99% tests passed, 2 tests failed out of 192`, log
`/tmp/sg051-ci2-rocky.log` at 3594 lines. The failing names are
`prose_gate_fixtures` and `harness_hunt_topdown_test`, at entry numbers
`30` and `192` in this run. Entry numbers are run-local: the base run
`42753235724` and the master run `42734353129` register 190 tests and
name those two failures at `30` and `190`, while this head registers
192, the hunt entry sitting one place later because this lane adds two
tests. The names carry the identity. The producer lines of the two
failures are unchanged at this tree, copied:

```
git ls-files failed in /__w/speedgun-ng/speedgun-ng (exit 128): fatal: detected dubious ownership in repository at '/__w/speedgun-ng/speedgun-ng'
harness_hunt_topdown_test oneCallReadsNoSourceOfItsOwn: FAIL
```

### The coverage figures at this identity

Job `129887350019`, log `/tmp/sg051-ci2-cov.log` at 2916 lines, suite
`100% tests passed out of 187` with skip entry `185/187 Test #185:
harness_topdown_run_test`. The capture arm and the gate line:

```
  lines.......: 100.0% (18945 of 18945 lines)
  functions...: 98.8% (1978 of 2003 functions)
  branches....: 99.7% (9948 of 9973 branches)
coverage-gate: branch shortfall 25 rows exceeds the  D-14 recorded allowance of 21
```

Those summary and gate lines answer 9 in the log and match the same 9
lines of run `42812457393` byte for byte, the double space inside the
gate sentence included. The line total reads 18945 here after 18944 at
the previous identity `bd829118`, one association of a return-path
counter having moved back on a file this lane never touched, which is
what the section above predicts for two runs of one tree.

### What this parity read settles

All three red jobs match their counterparts at run `42812457393` on the
lines that carry verdicts, timings aside, which is what the delta
between the two identities predicts: `git diff --name-only bd829118..HEAD`
piped through `grep -v ^specs/` answers 0, so only record paths moved.
The dark branch row set, the host figure of 22 rows and the CI figure
of 25 rows against the constant of 21 stand as the ruling packet had
them.

## Rebase onto the moved master

Master moved while the PR stood: the part-one lane merged as
628624d42c9982e84af08b0e76bd35e0746f4df9 through the owner squash at
16:14:43Z, and a probe change landed on top as
f17814716da4a66f0ae5103a707da91429f48bda, `Harness: Scale the probe's ticks to true ns`, touching the jitter probe source, four jitter test
files, cli.cpp and the 045 records. PR 106 then read CONFLICTING.

The lane replayed onto the new master with
`git rebase --onto origin/master 0fce6651 051-raw-events`, one commit
replayed, no manual conflict. The single overlapping path was
`.specify/feature.json`, the Spec Kit pointer: master carries the
049 value, the lane the 051 value, and git resolved ours because the
base and master agree on the 049 value. Post-rebase reads follow from
fresh commands at the new identity.

Consequence for the chain: the probe change is a new code surface
below the lane tree, so the whole leg chain re-runs at the rebased
identity, host and CI, before any parity sentence is reused. The
master-side red set is re-read too: the part-one merge may have
retired the prose fixture red, and the probe change can move the
branch census, so the allowance packet re-measures rather than
re-quotes.

The second hop. Master moved again the same hour, 2055fb54,
`Docs: Record Zen 4 Acceptance On Chilly`, eight record paths under
specs/041-counter-probe and specs/046-hunter-topdown, no source, test
or build path. The lane replayed once more with
`git rebase --onto origin/master f1781471 051-raw-events`, one commit,
no conflict. The tree delta from the previous lane identity to the new
one is exactly those eight record files, read from
`git diff --name-only` across the two identities, and those files
pass the prose gate at 6 sources and 5182 units with 0 findings when
scanned alone before the replay, so the leg chain in flight at the
previous identity stays valid for every source, test and build path.
