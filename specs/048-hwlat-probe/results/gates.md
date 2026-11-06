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
dead seam tallies removed. At the cut of this record its run answers `lint` pass and
`shared-audit` pass with the remaining jobs queued behind a congested runner pool; the
rows for `test`, `test-rocky`, `sanitize`, `tsan`, `coverage`, `dbc-gate`, `prose-lint`
and `static-analysis` join here when they report. Its proof here is a clang build of the whole tree with
`CXX=clang++ cmake --preset=ci-sanitize -B build/sanclang` and
`cmake --build build/sanclang -j 12`, which answers exit 0 with no `error:` line.
Before the three commits the same build stopped at the disclosure unit, so the specs/041
finding stood behind this lane's.

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

## Pending

- The root-only acceptance of SC-005 and SC-007 (T054, T055, T056) stands unrun: this
  session holds no root, and `/sys/kernel/tracing` stands `0700`. The real-counter
  reading, the planted system-management-interrupt reading, and the reading that every
  tracer setting returns on each exit path need a root run on this machine.
- The local analyzer, sanitizer, thread-sanitizer, coverage and pinned-lane re-runs at
  the rebased head were stopped once CI reported the same lanes: the analyzer build
  reached 46 percent with no finding. The CI jobs stand as the reading for those five
  tasks, and the `coverage` job of pull request #87 carries the line and branch figures
  of record.
