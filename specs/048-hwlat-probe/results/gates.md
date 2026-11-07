# The gates over the rebased head (T070 to T085)

Head `a8bde813`, base `origin/master` `d4fc50a3`, host `corsairone` (`Linux 7.3.0-4-cachyos
x86_64`, 24 cores of i9-9920X), compiler `g++ (GCC) 16.2.1`, `clang 23.1.3`, `cmake
4.4.2`. Each row names the command and what it answered at this head, in a release
developer-mode tree from `cmake --preset=ci-rocky`. The branch was rebased onto the
merge of pull request #86 before these readings, so every row below reads the merged
base.

## The cheap gates

| Gate | Command | Answer |
| --- | --- | --- |
| example drift | `python3 tools/docs/example_drift.py .` | exits 0 |
| format | `cmake -P cmake/lint.cmake` | exits 0 |
| spell | `cmake -P cmake/spell.cmake` | exits 0 |
| time source | `bash test/time_source_gate.sh .` | exits 0, `time_source_gate: clean` |
| prose, whole tree | `python3 tools/prose/prose_gate.py --check prose --mode tree` | exits 0, 0 findings over 1081 sources and 207,896 units |
| prose, commits | `python3 tools/prose/prose_gate.py --check commit --base origin/master --head HEAD` | exits 0, 0 findings over 62 messages |
| DBC doc gate | `python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml --registry tools/dbc/macros.yaml --out build/dbc-gate/doc-matrix.json` | exits 0, 349 interfaces, 0 gaps |
| DBC pair gate | `python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools --registry tools/dbc/macros.yaml --doc-matrix build/dbc-gate/doc-matrix.json --out build/dbc-gate/pair-matrix.json` | exits 0, 349 interfaces, 0 gaps |
| suite | `ctest --test-dir build -j 12 --output-on-failure` | exits 0: 190 of 190 passed, 0 failed |
| entry count | `ctest --test-dir build -N \| tail -n 1` | reads `Total Tests: 190` |
| exclusion markers | `grep -rn "LCOV_EXCL" source test example include tools \| wc -l` | reads 84, the count the branch inherited, and none under `source/harness` |
| no sleep, no clock | `grep -rlnE "sleep_for\|steady_clock\|system_clock\|gettimeofday" <the three new test files>` | reads no match |
| code discipline | the three greps of T083 | read 0, 0 and 0 |
| guide budget | `wc -l README.md` | reads 1002 of the 1004 the gate allows |

Two commands ran in a form other than the one the task names, for host reasons. The DBC
pair gate ran through its build target first: `cmake --build build -t dbc-gate` answers
`dbc gates require PyYAML (install python3-yaml): No module named 'yaml'`, because the
python that target resolves carries no `yaml`. The same two scripts then ran with a
python that does, over the Doxygen XML that target's own `Doxyfile` writes, and both
answered 0 gaps. The formatting fix form is `cmake -DFIX=YES -P cmake/lint.cmake`, the
cache variable, whatever the message text names.

## The instrumented lanes

Readings that predate the rebase stand labeled as such. The rebase moved the base, so
the comparison base moved from `858c55d9` to `d4fc50a3`.

- Coverage, pre-rebase head, `ctest -j 1` in `build/coverage` in CI's own form: 176 of
  180 passed, exit 8, and the four failures are entries at their ceilings
  (`base_cli_matrix` at 120 s, `harness_jitter_cli_test`, `harness_hunt_cli_test` and
  `example_run` at 300 s). No entry failed on its data.
- The same tree's trace read 99.1 percent of 18,400 lines and 97.8 percent of 9,147
  branches, and `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` answered
  `coverage-gate: line coverage is not 100%`. Of the 167 cold lines, 68 stood in
  `source/harness/hunt_cli.cpp`, a file this branch never edits. Run without its
  ceiling, `harness_hunt_cli_test` answers ok in 565 s against its 300 s ceiling, and a
  per-target capture then reads all 68 lines hot. In the serial suite the same entry is
  one of the four cut off, and `harness_cli_test` clears its ceiling by 6.15 s, at
  293.85 s.
- The 28 cold lines that stood on paths this branch lands read hot after the rows of
  `test: Close the lines the trace leaves cold`: five in `noise_tracer.cpp`, eleven in
  `json_load.cpp`, twelve in `compare.cpp`, each verified by a capture over the
  instrumented objects of its own unit.
- specs/046 records the same six entries past their ceilings on this host, and specs/033
  records a coverage-lane failure naming `harness_jitter_cli_test` as the load of its
  machine. The ceiling of `ctest` is a property of the entry, so `--timeout` does not
  widen it.
- Sanitizer and thread-sanitizer lanes, pre-rebase head: the six entries named above
  answer as timeouts and nothing else fails. A worktree at the then-base `858c55d9`
  answered the same six at the same ceilings, so the reading stands with the host.
  `counters_overhead` fails only under load.

## The CI run of the merged head

The head pushed as pull request #87 merged as `2919f5f1`. Its run answered:

| Job | Answer |
| --- | --- |
| `lint` | pass |
| `prose-lint` | pass |
| `test` | pass, 14m6s |
| `test-rocky` | **fail in `Build`**, 4m44s |
| `dbc-gate` | pass |
| `shared-audit` | pass |
| `downstream-consumer`, `consumer-release` | pass |
| `static-analysis (library)` | pass, 14m22s |
| `sanitize` | **fail in `Build`**, 8m21s |
| `tsan` | **fail in `Build`**, 5m28s |
| `coverage` | reported after this record's cut |

Each `Build` step reads the compiler that step names, and the release job that passed
builds with gcc while the sanitizer jobs build with clang. The three findings were:

- `test-rocky`, gcc 14: `test/source/harness_latency_counters_test.cpp:127` reads a
  planted file as a string of `std::istreambuf_iterator`s, and the compiler follows
  those iterators into `/usr/include/c++/14/streambuf:495` and reports `potential null
  pointer dereference [-Werror=null-dereference]`.
- `sanitize` and `tsan`, clang: `test/source/harness_hwlat_probe_test.cpp:95` holds a
  file-scope constant that no row reads, and clang reports `unused variable
  'kPhaseOpeningCall' [-Werror,-Wunused-const-variable]` where gcc reads nothing.
- The same two lanes then reach `test/source/harness_limit_disclosure_test.cpp:131`,
  the unit specs/041-counter-probe landed, and report `variable 'gOpened' set but not
  used [-Werror,-Wunused-but-set-global]`, and the same for the close count. That
  finding entered master with pull request #86 and stands behind this branch's own.

## The lanes that followed

`fix/clang-and-rocky-lanes`, as pull request #88, merged as `39bd33d0` and carries three
commits: the planted read through the buffer, the duplicate constant removed, and the two
dead seam tallies removed. Its run answered `lint`, `prose-lint`, `test`, `sanitize`,
`tsan` at its build, `dbc-gate`, `shared-audit`, all three `static-analysis` scopes and
both consumer jobs pass, which carries the two clang findings and the gcc 14 finding that
branch cleared. Three jobs report failures at the test step or later: Its proof here is a clang build of the whole tree with
`CXX=clang++ cmake --preset=ci-sanitize -B build/sanclang` and
`cmake --build build/sanclang -j 12`, which answers exit 0 with no `error:` line.
Before the three commits the same build stopped at the disclosure unit, so the specs/041
finding stood behind this lane's.

| Job | Row | Reading |
| --- | --- | --- |
| `test-rocky` | `harness_latency_counters_test` | `FAILED: a node without rights answers no count (FR-007, FR-011)` |
| `test-rocky` | `harness_hwlat_probe_test` | `FAILED: a node without rights leaves every gap unmarked, the marked count at 0 and the SMI figure with no value (FR-011, FR-015)` |
| `test-rocky` | `report_shape_hwlat_probe_sample` | `line 33: header line differs: block 'nmi delta: 0 (source: /proc/interrupts)' against run 'nmi delta: not available (source: /proc/interrupts)'`, the pair again at 65, 97 and 128, and `line 137: the block omits this header line` |
| `test-rocky` | `prose_gate_fixtures` | the row stands failed at pull request #86 too |
| `test-rocky` | `harness_hunt_topdown_test` | `HARNESS HUNT TOPDOWN TEST FAIL: three calls leave the process holding the mappings it held before`, the row behind it failed at pull request #86 too |
| `tsan` | `harness_hunt_cli_test` | `HARNESS HUNT CLI TEST FAIL: one run of one benchmark answers one block, so the named level admits no second block of the companion (FR-016, FR-034)` |
| `coverage` | the extract step | `lcov: ERROR: (inconsistent) "/home/runner/work/speedgun-ng/speedgun-ng/source/harness/runner.cpp":708: function '_ZZN2sg6detail19stripTopdownColumnsE..._clEm' is hit but no contained lines are hit`, the identical line at pull request #86 |

The Rocky and sanitizer runners read as root, so the rights bits of a planted file bind
nothing there. The two rights rows belong to this lane: they planted a node with no rights
and expected a refusal. Both now read the kernel's answer through the same open the reader
uses and assert the figure that answer owes, the rule specs/036 T010 states for the
denied-path rows of this tree. The sample row is a fold the shape gate lacked: the recorded
block came from a host that holds an interrupts row and withholds the node, and the runner
holds the node and answers no non-maskable-interrupt row. The three rows behind them are
base work that stands failed ahead of this lane: specs/042 landed the two hunt rows, and
the prose fixture row stands failed on that runner since pull request #86.

`fix/ci-lane-rows`, as pull request #89, carries the two rights rows, the four fold rules
the sample needs, and the `inconsistent` class the extract step repeats from the capture.
Its readings join here when it reports.

## The finding this lane's own gate let through

The lane's warning-set check ran with `g++` over each new file with the full warning
list and `-Werror`, and it answered 0 on every one of them. Two of the three CI
findings are invisible to that command: clang's unused-const-variable and
unused-but-set-global families have no gcc twin at the same spelling, and gcc 14's
null-dereference analysis is sharper than the gcc 16 the lane host carries. The gate
that would have caught them is the same command run with `CXX=clang++`, which is what
the sanitizer lanes use. A future lane of this repository should run the warning-set
check on both compilers, and the cheap-gate table above records gcc only because that
is what ran.

## The lanes over the branch that fixes the lanes (T092)

Head `b8204413`, run `42289059046`, and head `602730e6`, run `42297681487`, both on the branch
`fix/ci-lane-rows` over base `39bd33d0`. The runners answer as root. The Rocky lane builds with
gcc 14, the two sanitizer lanes build with clang, and the delta between the two heads is two
`lcov` reads that carry one tolerated error class.

| Job | Run at `b8204413` | Run at `602730e6` |
| --- | --- | --- |
| `test` | 190 of 190 | 190 of 190 |
| `test-rocky` | 188 of 190 | 188 of 190 |
| `sanitize` | passes | fails one entry |
| `tsan` | passes | fails one entry |
| `coverage` | fails at the gate | fails at the gate |
| `lint`, `prose-lint`, `dbc-gate`, `shared-audit`, the two consumer jobs, the three analyzer scopes | pass | pass |

`test-rocky` names the same two entries in both runs: `prose_gate_fixtures` and the mapping row
of `harness_hunt_topdown_test`. Both stand on base work, because the same two entries fail at
the head of pull request #86.

The second run adds two entries that fail under instrumentation alone. `sanitize` fails
`harness_hunt_cli_test` in 20.77 seconds on two checks, `one run of one benchmark answers one
block, so the named level admits no second block of the companion (FR-016, FR-034)` and `the
re-executed scenario passes`. `tsan` fails `example_run` in 71.37 seconds with `FAIL:
docs_argument_families registered bmSquares/cells:128 but printed no row`. The uninstrumented
`test` job of that same run passes both, in 4.16 seconds and 44.06 seconds. Both names also
fail on the lane host under load, so the reading of record stays with the uninstrumented job
until a run separates them.

## The coverage figures the gate reads (T090)

The coverage lane carries 185 entries and passes every one of them in the second run. The gate
then reads the trace that run wrote and answers `coverage-gate: line coverage is not 100%` with
`lines.......: 99.9% (18637 of 18639 lines)` and `branches....: 99.5% (9747 of 9793 branches)`.
That is two cold lines and 46 cold branch records against the allowance of 21, so the branch
shortfall stands at 25 records over. The two cold lines are `group_io.cpp:452` and `:454`.

The first run's trace read `lines.......: 99.9% (18633 of 18638 lines)` and `branches....:
99.5% (9746 of 9793 branches)`, three lines colder and one record colder: the print of the
probe-source limit line of specs/050 at `source/harness/runner.cpp:1609` to `:1611` stood cold
there and reads covered in the second run. Nothing in the source moved between those two heads,
so the two traces differ by a reading of the runner and not by a change of code.

Before these two runs the gate never reached a figure. It stopped on `lcov` reading a function
as hit while every line inside it stood omitted, and the tool answered with no numbers at all.
Four reads of the trace carry the tolerated class now: the capture, the extract, the `--list`
step of the coverage target, and the `--summary` read of the gate script.

## The pinned lane on the lane host (T093)

`taskset -c 0,1 ctest --test-dir build -L quick -j 2 --output-on-failure` over the tree of
`fix/048-convergence` at `ef49f14d`, on the lane host: 183 of 183 passed, exit 0,
`Total Test time (real) = 763.93 sec` and `quick = 1569.51 sec*proc`. Two workers on two cpus
over the standing quick entry set, so no entry needs a ceiling for the pinned lane, and the
readings of the CI jobs stand in for nothing here.

One deviation stands on the record. The run used that tree's warm developer build, because the
`prepush` preset resolves to `build/prepush` and that tree needs a configure and a build pass
beside the coverage work that was running. The pinning, the worker count and the entry set are
the preset's, and the compiler and the flags are the developer tree's.

## Pending

- The docs example of the detector engine requests `.tracerEngine = sg::TracerEngine::HWLAT`,
  and the recorded block carries no tracer lines: the tree that recorded it holds no usable
  tracefs, so the run answers `tracer: not available`, keeps its user-space figures and holds
  status 0. A host that holds tracefs prints the tracer lines the recorded block lacks beside
  the same figures. specs/036-osnoise-timerlat-tracers carries the same limit for its own
  example, so the example keeps its shape and this note carries the limit (per FR-037, SC-011,
  T109).
- The root-only acceptance of SC-005 and SC-007 (T054, T055, T056) stands unrun: this
  session holds no root, and `/sys/kernel/tracing` stands `0700`. The real-counter
  reading, the planted system-management-interrupt reading, and the reading that every
  tracer setting returns on each exit path need a root run on this machine.
- The local analyzer, sanitizer, thread-sanitizer, coverage and pinned-lane re-runs at
  the rebased head were stopped once CI reported the same lanes: the analyzer build
  reached 46 percent with no finding. The CI jobs stand as the reading for those five
  tasks, and the `coverage` job of pull request #87 carries the line and branch figures
  of record.
