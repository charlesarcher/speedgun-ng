# T022: the acceptance record of the limit table move

This record holds what the lane proved on the lane host, and it names the head
sha that `git rev-parse HEAD` answered for each reading. Three lane records
carry the raw figures: `results/base-state.md` for the base pair,
`results/sanitizers.md` for the two sanitizer legs, and `results/coverage.md`
for the coverage pass. `tasks.md` T025 squashes the lane into one commit, and
`tasks.md` T026 and T027 fold their writes into that same commit. Each step
moves that sha, so this record states each reading at the head it tested. It
claims nothing about the squashed head. `tasks.md` T028 moves no head, because
its reading comes after the push. The head CI verdict stands on the pull request
that `T028` opens, and no reading here claims it.

## The host and its toolchains

`uname` and `nproc` answer the first three rows, and the `CMakeCache.txt` of
each build tree answers its compiler line. `results/base-state.md` carries the
same host shape at the base, and `results/sanitizers.md` carries the clang
probe of the two sanitizer presets.

| Fact | Reading |
| --- | --- |
| Host (`uname -n`) | `kennykiller` |
| Kernel and machine (`uname -sm`) | `Linux 7.3.1-1-cachyos x86_64` |
| Cores (`nproc`) | 20 |
| Compiler of `build` and `build/dev` | GCC `16.2.1` |
| Compiler of `build/coverage` | GCC `16.2.1` |
| Compiler of `build/sanitize` and `build/tsan` | clang `23.1.4` |

## The head each reading stands at

`git log --format='%h %cI %s' origin/master..HEAD` answers one commit today,
because T025 squashed the lane and T026 rebased it. The shas below are the ones
the readings were taken at, and they stood on the branch before those two tasks
rewrote it. The first row names the base commit that stood below the whole lane,
and the clock below carries every time in the lane host zone. The two last
sections state which readings were re-measured on the shipped head, and the
four records named `rebase-*.md` carry those re-measurements.

A lane sha named in this record identifies a tree this lane measured. No ref
carries such a sha today: T025 squashed 23 commits into one, T026 rebased it, and
T027 amended it once more, so `500a7940`, `a41fc034`, `dfdc39cf` and `fba814d2`
stand in no history. Every reading therefore names a log path or a record, and
that file holds the command that produced its number.

```text
858c55d9 Harness: Add the top-down area of a ranked event, the base
500a7940 2026-11-05T06:56:46-06:00 Docs: Clear the 050 round nine readings
a41fc034 2026-11-05T20:01:43-06:00 CMake: Move the version to 0.31.1
```

| Head | Readings that stand at it |
| --- | --- |
| `858c55d9` | the two base suite readings |
| `500a7940` | the wave gate pair of `T010` |
| `a41fc034` | five gates, checked build, two sanitizer legs, coverage |

The wave gate pair ran with the wave-3 edits present in the working tree, so its
binaries hold the code that `d9a4790e` later committed. The five code and test
files of that commit read byte-identical at `d9a4790e` and at `a41fc034`.
`git rev-parse` answers one blob name for each file at both heads.
`git diff --name-only d9a4790e..a41fc034` names six paths, and none of them
holds counters code. The version move touched the build file, the docs page,
one example record and two test files that read the version string. One commit
reworded a line of `tasks.md`.

## The two plain-tree suite readings (SC-001)

The plain tree is `build`, configured through `ci-rocky` and built with the head
checked out. Two commands read it, one wide and one held to two cores:

```text
ctest --test-dir build -j 20 --output-on-failure
taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure
```

The first line is the `-j 20` reading, and the second is the pinned-core
reading. Each row below names the sha whose code that run measured, and each
row states its measure beside its figure.

| Head | Command | Verdict | Wall time |
| --- | --- | --- | --- |
| `858c55d9` | `-j 20` | 180 of 181 entries | 94.3 s |
| `858c55d9` | pinned core | 180 of 181 entries | 367.4 s |
| `500a7940` | `-j 20` | 181 passed of 182 entries | 113.7 s |
| `500a7940` | pinned core | 181 passed of 182 entries | 361.4 s |

The verdict column keeps the convention of `results/base-state.md`. An entry
that did not fail stands on the passing side, so the three skips below join it.
The two head logs print `99% tests passed, 1 tests failed out of 182`.

The four logs stand at `/tmp/050-base-ctest.log`,
`/tmp/050-base-ctest-taskset.log`, `/tmp/050-rerun-suite20.log` and
`/tmp/050-rerun-suite-pinned.log`. `results/base-state.md` states that
`git diff --name-only 858c55d9..HEAD` names nine spec artifacts and no code
file, so the two base rows measure base code. The entry count moves from 181
to 182, because `d9a4790e` registers `counters_limit_lifecycle_test`, and that
new entry passes under both head commands. Three entries report `Skipped`
under all four commands: `counters_pmu_test`, `counters_overhead` and
`harness_topdown_run_test`. Each one carries a `SKIP_RETURN_CODE 2` property,
and each one skips for the host reason `results/base-state.md` states.

The wave gate ran those two commands twice at `500a7940`. The first pass, from
19:28 to 19:39, read 182 entries with two failures, in 120.3 s and 359.2 s. One
failure was `harness_hunt_cli_test`, the standing base fault, and the other was
`test_tier_labels`. That second fault came from a copied `SKIP_RETURN_CODE 2`
property on the new entry, which `test/tier_labels.sh` bars, and the removal of
that copy cleared it. The re-run from 19:41 to 19:49 left
`harness_hunt_cli_test` as the only failure under both commands, and it reads
113.7 s and 361.4 s. The named run of 19:41 shows `test_tier_labels` passing in
0.04 sec beside `counters_limit_lifecycle_test`, the new entry of the lane.

## The five gates of SC-003 at head `a41fc034`

Five commands carry the gate verdicts, and each one exits 0 at `a41fc034`. The
two pins name the tools that `results/base-state.md` records, at
`clang-format` `18.1.8` and `codespell` `2.4.3`. `cmake` reads `-D` before `-P`,
so each flag below sets the cache entry that its script reads.

```text
cmake -D FORMAT_COMMAND=/tmp/050-tools-venv/bin/clang-format -P cmake/lint.cmake
cmake -D SPELL_COMMAND=/tmp/050-tools-venv/bin/codespell -P cmake/spell.cmake
python3 tools/docs/example_drift.py .
python3 tools/prose/prose_gate.py --check all --mode range --base origin/master
cmake --build build/dev --target dbc-gate
```

| Gate | Verdict at `a41fc034` |
| --- | --- |
| format, with the pinned formatter | exits 0, names no file |
| spell, with the pinned speller | exits 0, no misspelling |
| example drift | exits 0 |
| prose, range mode over `origin/master` | exits 0 over 40 sources |
| `dbc-gate` target | exits 0, 348 interfaces, 0 gaps |

The `dbc-gate` target needs a configured checked tree, and one line configures
that tree:

```text
cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3
```

Two gate artifacts under `build/dev/dbc-gate/` carry the reading: the doc matrix
and the pair matrix each hold 348 interface rows, and every row reads `ok`. That
count stands one above the base figure of 347 interfaces, because the now public
`System::~System` joins the documented surface (D-5, FR-010).

## The checked build of T019 at head `a41fc034` (SC-004)

The checked build is the `ci-ubuntu` tree, the one that wires in clang-tidy and
cppcheck. A finding counts once per file, line, column and check name, so a
header finding repeats in every unit that reaches that header. The unit count
therefore stands beside every figure in the table.

| Edited file | Head, distinct | Base, distinct | Base units |
| --- | --- | --- | --- |
| `source/counters/system.cpp` | 22 in 1 unit | 22 | 1 |
| `source/counters/detail/core.hpp` | 19 in 1 unit | 19 | 4 |
| `include/speedgun-ng/counters_system.hpp` | 5 in 1 unit | 5 | 110 |

`/tmp/050-head-dev-build-full.log` carries that reading, and it holds one
`Building CXX object` line. That run compiled one translation unit, and the one
unit reaches both edited headers. `/tmp/050-gate-devbuild.log` corroborates the
same three distinct figures over 111 translation units, where the raw line
counts read 22, 95 and 555. The 95 lines of `core.hpp` split 76 and 19 over two
path spellings of one file, as `results/base-state.md` records for the base.
No figure rises above its base figure of 22, 19 and 5. The checked build states
no new finding in any file this lane edited.

## The two sanitizer legs of T020 at head `a41fc034` (SC-001)

Both legs ran at `a41fc034`, one after the other, and each command alone.
`results/sanitizers.md` carries every command, exit code, entry count and log
path of both legs. This record states the two verdicts, and it cites that record
for the commands.

| Leg | Entries | Verdict, entries named |
| --- | --- | --- |
| `ci-sanitize` | 177 | 173 of 177 entries, one failure |
| `ci-tsan` | 178 | 174 of 178 entries, one failure |

The one failing entry of each leg is `harness_hunt_cli_test`, the standing base
fault named below. Neither leg reports a race, and neither prints an
`AddressSanitizer` line, a `LeakSanitizer` line, a `SUMMARY:` line or a
`runtime error` line. D-6 keeps the thread sanitizer the arbiter of a race, and
that arbiter answers none on this head.

## The coverage pass of T021 at head `a41fc034` (SC-002)

`results/coverage.md` carries the five steps of the pass, the exit code of each
step, the two named `lcov` commands and every row count. The pairs below come
from the `DA:` and `BRDA:` rows of `build/coverage/coverage.info`, and each row
states its measure beside its figure.

- `source/counters/system.cpp`: 281 of 281 lines, 137 of 140 branches.
  The base pair reads 282 of 282 lines and 137 of 140 branches.
- `source/counters/detail/core.hpp`: 79 of 79 lines, 31 of 34 branches.
  The base pair reads 79 of 79 lines and 31 of 34 branches.
- `include/speedgun-ng/counters_system.hpp`: 20 of 20 lines, 6 of 6 branches.
  The base pair reads 20 of 20 lines and 6 of 6 branches.

Every line this lane wrote reads covered, so the line bar of D-9 holds. Six dark
branch rows stand at the head: three in `source/counters/system.cpp` and three
in `source/counters/detail/core.hpp`. The base carries the same six, so no dark
row joined either file. Each moved figure stands at the same source statement as
its base figure. The tree-wide pair of the host trace reads 97.6 percent of
17785 lines and 94.9 percent of 8829 branches, which leaves 446 dark branch
rows. `results/coverage.md` carries that pair, the base figure of 450 dark rows,
and the host `lcov` defect behind both. The in-tree gate cannot run its measure
checks on this host, so the CI `coverage` job carries that verdict.

## The base failures stand pre-existing

`harness_hunt_cli_test` fails at the base on this host under both commands, and
`results/base-state.md` carries both readings at `858c55d9`. That entry prints
two failing assertions, at `test/source/harness_hunt_cli_test.cpp:3129` and at
`:3464`. The first counts the `topdown: level=` headers of a run that names two
hunt addresses. The second re-executes each scenario and needs every child to
exit cleanly. CI run `41946446748` fails the same entry in its `sanitize` job.
The same run fails `test-rocky` on three entries, and it fails `coverage` on the
inconsistent record at `source/harness/hunt_topdown.cpp:155`. Eleven jobs pass,
and its `docs` job stands skipped. None of those jobs reads the counters code,
so the lane carries every one of them forward unchanged.

## The squashed head `dfdc39cf` (T025)

`T025` cut one commit whose parent is the base `858c55d9`, and
`git rev-list --count origin/master..HEAD` answers 1. Three readings show that
the cut moved history and left content alone. `git status --porcelain` reads
empty before the cut and after it. The tree hash `11b8a60f` stands both for the
staged content and for the new commit. `git diff --stat 9b6ad201 HEAD` names
only the tick lines of `specs/050-limit-table-lifecycle/tasks.md`.

The cut names no barred command. Four lines do the work:

```text
git add specs/050-limit-table-lifecycle/tasks.md
git write-tree
git commit-tree <tree> -p 858c55d9 -F <message file>
git update-ref refs/heads/050-limit-table-lifecycle <new commit>
```

`cmake --preset=ci-rocky` exits 0, and `cmake --build build` exits 0 with no
compilation line and no link line in its 177-line log. The tree carries the
bytes the wave built, so the build rechecks every target and recompiles nothing.

Two suite readings stand at this head, each taken alone on an idle host:

| Head | Command | Verdict | Wall time |
| --- | --- | --- | --- |
| `dfdc39cf` | `-j 20` | 181 passed of 182 entries | 115.33 s |
| `dfdc39cf` | pinned core | 181 passed of 182 entries | 362.33 s |

Both logs print `99% tests passed, 1 tests failed out of 182`, and both exit 8.
`harness_hunt_cli_test` is the only failure, and the same three entries report
`Skipped`. The verdict column keeps the convention stated above, so the skips
join the passing side. The logs stand at `/tmp/050-T025-ctest-j20.log` and
`/tmp/050-T025-ctest-pinned.log`.

The five gates ran again on this head, and each one exits 0:

| Gate | Verdict at `dfdc39cf` |
| --- | --- |
| format, with the pinned formatter | exits 0, names no file |
| spell, with the pinned speller | exits 0, no misspelling |
| example drift | exits 0 |
| prose, range mode over `origin/master` | exits 0 over 23 sources |
| `dbc-gate` target | exits 0, 348 interfaces, 0 gaps |

The prose reading covers 23 sources and 3774 units, because one commit stands
inside the range here. The two matrices under `build/dev/dbc-gate/` each hold
348 interface rows, and every row reads `ok`.

## The rebase onto `02892b57` (T026)

`git fetch origin` found nine commits above the base this lane measured, so the
lane rebased. The one commit of `T025` became `1b5d8b70`, and one correction to
`test/source/harness_cli_table_test.cpp` moved the head to `fba814d2`. Every
reading below stands at `fba814d2`, the answer of `git rev-parse HEAD` at its
run time.

Four lines cut the rebase, and no barred command stands among them:

```text
git merge-tree --write-tree origin/master HEAD
git commit-tree <tree> -p origin/master -F <message file>
git update-ref refs/heads/050-limit-table-lifecycle <new commit>
git read-tree -u --reset <new commit>
```

`git merge-tree` named four conflicting paths, and each one took the base text
plus this lane's move. `git status --porcelain` answers nothing after the last
line, and `git rev-list --count origin/master..HEAD` answers 1.
`source/counters/system.cpp` and `source/counters/detail/core.hpp` stand
unchanged in the nine commits, so the moved code and the new code never met in
one file.

Four commands prepare the plain tree for these readings, and each one exits 0:

```text
cmake --preset=ci-rocky
cmake --build build -j 20
cmake --build build --target harness_cli_table_test
ctest --test-dir build -R 'harness_cli_table_test|views_test|counters_limit_lifecycle_test'
```

The build log holds no `error:` line. The correction moved one comparison in
the version binding of `harness_cli_table_test`, so that target was rebuilt
before any reading here. The last command answers 100 percent passed out of 3.

Two suite readings stand at this head, each alone on an idle host:

| Head | Command | Verdict | Wall time |
| --- | --- | --- | --- |
| `fba814d2` | `-j 20` | 185 passed of 186 entries | 117.67 s |
| `fba814d2` | pinned core | 185 passed of 186 entries | 367.25 s |

Both logs print `99% tests passed, 1 tests failed out of 186`, and both exit 8.
`harness_hunt_cli_test` is the only failure, and the same three entries report
`Skipped`. The entry count moves from 182 to 186, because the nine commits
register four entries that the pre-rebase tree did not carry. The logs stand at
`/tmp/050-R-suite20.log` and `/tmp/050-R-suitepinned.log`.

The five gates ran again on this head, and each one exits 0:

| Gate | Verdict at `fba814d2` |
| --- | --- |
| format, with the pinned formatter | exits 0, names no file |
| spell, with the pinned speller | exits 0, no misspelling |
| example drift | exits 0 |
| prose, range mode over `origin/master` | exits 0 over 24 sources |
| `dbc-gate` target | exits 0, 349 interfaces, 0 gaps |

The prose reading covers 24 sources and 3813 units. The interface count stands
one above the base figure of 348, which `results/rebase-base.md` measures at
`02892b57`. The one new interface is the public pair `System()` and `~System()`
(D-5, FR-010).

Five new records carry the deeper readings of this head and of its base, and each
states its own commands. `results/ci-base.md` reads the CI leg at `02892b57`:
eleven jobs pass there, `test-rocky`, `sanitize` and `coverage` fail, `docs`
stands skipped, and that `sanitize` job configures `Clang 21.1.8`. The other
four carry the host readings. `results/rebase-base.md` measures the base at `02892b57`, with 185
entries and `dbc-gate` at 348 interfaces and 0 gaps;
`results/rebase-analyzer.md` answers 22, 19 and 5 distinct positions for the
three changed files at both head and base, and zero for the fourth changed file;
`results/rebase-sanitizers.md` answers zero AddressSanitizer, LeakSanitizer,
`runtime error` and ThreadSanitizer lines on both legs;
`results/rebase-coverage.md` answers 281 of 281 lines for `system.cpp`, 79 of 79
for `core.hpp` and 20 of 20 for the header, with the marker count at 84.

The sanitizer legs carry one entry that stops building on this host, and
`results/rebase-sanitizers.md` and `results/rebase-base.md` hold the measurement
together. `test/source/harness_limit_disclosure_test.cpp:131` and `:132` trip
`-Werror,-Wunused-but-set-global` under clang 23.1.4, so CTest reports
`harness_limit_disclosure_test` as `Not Run`. That file stands byte-identical at
head and at base, and the base wave rebuilt that one target under the same
compiler and met the same two diagnostics. GCC has no such diagnostic, so the
plain tree and the coverage tree build the entry and pass it. The CI jobs name
`clang++` from the Ubuntu package. `results/ci-base.md` reads that compiler at
`Clang 21.1.8` in the `sanitize` job of run `42156116508`, and there
`harness_limit_disclosure_test` builds and passes as test number 72. The run of
this branch carries its own verdict, and `T028` reads it.

## What no reading here claims

The head CI verdict stands on the pull request that `T028` opens, and this
record claims nothing about it. A branch push starts no run, because
`.github/workflows/ci.yml` triggers `push` on `master` alone. The run that
`T028` reads starts when the request against `master` opens, so that run
postdates every commit here. `gh pr view` answers the head sha, the run number
and the verdict, and the request body carries all three once the run ends. The
same statement covers the squashed head. The next section carries the readings
taken on it, and none of them reports a CI run.

## How this record checks itself

`test -s` answers non-empty for each of the ten files under `results/`, and
this file stands among them. Four counts check the branch itself:
`git rev-list --count origin/master..HEAD` answers 1, `git status --porcelain`
answers nothing, `git show --name-only --format= HEAD` names 18 paths under
`specs/050-limit-table-lifecycle`, and `git ls-files` names 10 files under
`results/`. The prose gate over the feature directory reads 18 sources, 5102
units and 0 findings, after `git add -N` let that mode see the new records, and
the pinned speller answers no misspelling for any of the 18. A `--paths` run
over a file the index does not know answers `0 sources`, and such a reading
proves nothing, so no count above came from such a run. The standing fault of
`harness_hunt_cli_test` fails at `858c55d9`, at `02892b57` and here, so no
change in this lane caused it.
