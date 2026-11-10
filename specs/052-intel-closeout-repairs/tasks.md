# Tasks: Intel Closeout Repairs

**Input**: Design documents from `specs/052-intel-closeout-repairs/`

**Prerequisites**: `spec.md`, `plan.md`, `research.md`,
`checklists/requirements.md`

**Tests**: This feature asks for a test behind every requirement. TDD order
holds where the fault is reachable: the gate script lands first and reads red
at a tree that carries no fix, and the fix lands afterwards.

**Format**: `[ID] [P?] [Story] Description`

- **[P]**: runs in parallel with the other **[P]** tasks of the same phase: a
  different file, a different writer, no shared build step

- **[Story]**: `US1` the ignore build, `US2` the sanitizer lanes, `US3` the
  record a reader on another machine opens, `US4` the delivered tip

- One writer holds one file inside one phase. A child writes its report under
  `/tmp/icr/` and stages nothing, because the commits stand in one hand.
- A task that rewrites a tracked file reads it first, builds the whole
  replacement text in memory, compares its length against
  `git show HEAD:<path>`, and only then opens the path for writing. A record
  file lost to a truncated write costs the closeout its evidence.

## Phase 1: Foundational (wave 0)

- [ ] T001 [FR-001] Record the base state in `/tmp/icr/base-state.md`:
  `git rev-parse HEAD` and the tip of `origin/master` the branch stands on, read
  with `git rev-parse origin/master`, which answered `1627e756` after the
  rebased merges of PR #104, #105, #106 and #107, the entry count of each lane tree read
  with `ctest --test-dir <tree> -N`, the two consumer faults with their file
  and line, the DBC target exit and its message under the interpreter the cache
  picked and under `/usr/bin/python3`, read in the CI form
  `cmake -S . -B build/dbc --preset=ci-ubuntu` then
  `cmake --build build/dbc --target dbc-gate`, the same configure line once more
  with `-D Python3_EXECUTABLE=/usr/bin/python3` added, the compiler each lane tree
  configured with, and the stale coverage capture with its gate line and both
  denominators

- [ ] T002 [P] [FR-001] [US2] Bring every tree the pass names up from nothing.
  Eight trees, and each gets its own directory. A preset that names no `-B` writes
  the `binaryDir` of its own chain, and `ci-build` writes `${sourceDir}/build`,
  which `ci-rocky`, `ci-ubuntu` and `ci-ubuntu-test` inherit unchanged, so a bare
  `cmake --preset=<name>` puts three roles into the release tree. Configure with
  the preset and the directory together, the form
  `.github/workflows/ci.yml:1049`, `:1084` and `:1284` use: `build` from
  `ci-rocky`, which also carries the CI test shape because `ci-ubuntu-test`
  resolves to the same `binaryDir` and the same cache variables, `build/dbc` from
  `ci-ubuntu`, `build/analyzer` from `ci-ubuntu`, `build/sanitize` from
  `ci-sanitize`, `build/tsan` from `ci-tsan`, `build/coverage` from
  `ci-coverage`, `build/consumer-release` from `ci-linux-ignore` with
  `speedgun-ng_DEVELOPER_MODE=OFF`, and `build/consumer-release-dev` from the same
  preset with `speedgun-ng_DEVELOPER_MODE=ON`. Each line reads
  `cmake --preset=<name> -B <tree>`, so no two roles share one cache. Name the
  seven child trees before `build` in the removal loop: seven of the eight stand
  inside `build`, and `rm -rf build` takes them with it, which is how one run lost
  seven absence lines and printed `ABSENT` for trees that stood configured. For each
  tree: read whether the directory holds a `CMakeCache.txt`, remove any that does
  with `rm -rf <tree>` and print that removal with its object count into the base
  state, then configure it, and print the count of objects the build produced with
  `find <tree> -name '*.o' | wc -l`. The removal list is those eight directories.
  Every other directory under `build` belongs to another lane: T001 prints an
  inventory of them and no task of this feature removes one. A directory that
  already stands configured, a build whose count answers zero, and a launcher
  whose log stands empty are three shapes of one fault, and none of the three
  yields a verdict. `build/analyzer` stood configured `2026-11-08` with 364 objects,
  and the parent removal took it; the fresh configure answers zero objects.
  Then build each tree to its last target with
  `cmake --build <tree> -j "$(nproc)" -- -k`, and run the same command again and
  keep the no-op answer, so no figure later rests on a partly built tree (R-17).
  `build/analyzer` is the one tree this task leaves configured and unbuilt: T031
  reads the object count of its first build as the proof that the analyzer ran,
  and a build here would spend that proof. Configure `build/sanitize` and
  `build/tsan` with `CXX=clang++`, which is the pin
  `.github/workflows/ci.yml:153` and `:236` set and the compiler the base lane
  rows were read from, and print each tree's compiler into the base state
  (FR-001, SC-009, R-17)

- [ ] T003 [P] [FR-008] Read the fence and the constants at the tree the pass
  builds.
  `git diff --name-only c9187ba3..1f1f9b20 -- include source test example tools cmake .github CMakeLists.txt CMakePresets.json`
  answers nothing, which is the reading that keeps the closeout figures in force.
  `CMakeLists.txt:87` reads `VERSION 0.35.0` at `1627e756`: PR #106 carried that
  bump from 0.34.0, the figure the order states, and this feature moves neither.
  `source/harness/json_report.cpp:58` reads schema 1.16, the two `SOVERSION`
  values read 22 and 8, and `git grep -c 'LCOV_EXCL' -- source include` sums to 84
  (FR-008, D-1)

## Phase 2: User Story 1, the gate before the fix (Priority: P1, wave 1)

Story goal: a consumer that configures the ignore semantic builds, and one
CTest entry keeps it building. Independent test: the new entry plus the two
consumer builds.

- [ ] T004 [P] [US1] Write `test/ignore_contracts_shape.sh`: compile
  `source/harness/noise_tracer.cpp` and `example/docs/hunt_ranks.cpp` (D-7) at
  semantic 0 and semantic 2 with every compiler on `PATH` that targets x86-64,
  under `-Wall -Wextra -Werror=unused-function -Werror=unused-variable`, plant
  the probe in a scratch copy of `noise_tracer.cpp`, print one line per step,
  skip with exit 0 and a `skip:` line where no compiler targets the platform.
  Reuse the compile form of R-8 and its two `-D` defines, because a compile
  without them answers for a different translation unit, and take the export
  include directory from the build-directory argument the way
  `test/simulation_mark_shape.sh:41-62` does. The script takes the source and
  build directories as `$1` and `$2`, so no `Release` path and no version string
  stands baked into it (FR-005, FR-006, SC-002, R-8)

- [ ] T005 [P] [US1] Register the entry in `test/CMakeLists.txt` inside the Linux
  region, beside the `simulation_mark_shape` registration that stands at
  `:219-220` on `origin/master`:
  `add_test(NAME ignore_contracts_shape COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/ignore_contracts_shape.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})`,
  with the comment that names the semantics and the probe, and with no
  `LABELS`, no `TIMEOUT` and no `WORKING_DIRECTORY` (FR-005, FR-013)

- [ ] T006 [FR-002] Record the red in the CI test shape:
  `cmake --preset=ci-rocky`,
  `cmake --build build -j "$(nproc)"`, then
  `ctest --test-dir build --no-tests=error -R '^ignore_contracts_shape$' --output-on-failure`.
  Keep the exit and the compiler line in `/tmp/icr/w1-red.log`: the tree carries
  no attribute, so each of the two semantic-0 compiles answers non-zero on its
  own step (FR-002, FR-003, D-4)

- [ ] T007 [US1] Wave gate G1, run against the `build` tree that T006
  configured: `ctest --test-dir build -N | grep -c '^  Test '` answers
  one above the T001 reading for that tree, and the four-tree count belongs to
  G4; `bash test/tier_labels.sh build "$(command -v ctest)"` exits 0,
  which is the two-argument form that file's usage line asks for;
  `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` names no file
  beyond the two the base carries (R-19), which is the comparison
  `/tmp/icr/gate-run.sh` reads against `/tmp/icr/format-base.txt`; `codespell`
  exits 0;
  `cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake` reports no finding;
  `ctest --test-dir build --no-tests=error -R '^example_drift$' --output-on-failure`
  exits 0 (FR-005, FR-013, FR-030)

## Phase 3: User Story 1, the two attributes (wave 2)

- [ ] T008 [US1] Add `[[maybe_unused]]` and the reason comment to
  `activationAnswered` in `source/harness/noise_tracer.cpp`, following
  `source/harness/compare.cpp:1156`, then run the formatter over the hunk
  (FR-002, R-6)

- [ ] T009 [US1] Add `[[maybe_unused]]` to `records` at
  `example/docs/hunt_ranks.cpp:147` (D-7), below the `sg-example:begin hunt-ranks`
  region that spans lines 15 to 33, then run the formatter over the hunk
  (FR-003, R-7)

- [ ] T010 [US1] [SC-001] Wave gate G2: the new entry exits 0;
  `cmake --build build/consumer-release -j "$(nproc)"` and
  `cmake --build build/consumer-release-dev -j "$(nproc)"` both exit 0 and
  print no unused-name diagnostic;
  `ctest --test-dir build --no-tests=error -R '^example_drift$' --output-on-failure`
  exits 0; `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` names no
  file beyond the two the base carries (R-19), and
  `git diff --unified=0 -- source/harness/noise_tracer.cpp example/docs/hunt_ranks.cpp` (D-7)
  shows the formatter inside the hunks this task wrote, because a formatting
  hunk outside them is a formatting-only change and V forbids combining one with
  content. Both logs keep their command, head and
  load witness (FR-002 to FR-004, SC-001, SC-002)

## Phase 4: User Story 2, the drivers lose wall time (Priority: P2, wave 3)

Story goal: each sanitizer red is green inside its ceiling or a blocked row
that names an owning path. Independent test: the four entries, measured alone.

- [ ] T011 [P] [US2] Print the base reading of `test/base_cli/run_matrix.py`
  into `/tmp/icr/pool-base.md`: the verdict text
  `26 of 26 with all rows matching` and the non-zero answer of the fixture that
  plants a failure. Then, in that file, run the 26 rows through a bounded pool of
  six and name each
  reroute document after its row, keeping every row, every comparison and the
  verdict text (FR-009, FR-010, FR-012, FR-029, R-10)

- [ ] T012 [P] [US2] Print the base reading of `test/example_run.sh` into
  `/tmp/icr/pool-base.md`: the 37 `PASS:` lines and the non-zero answer of the
  fixture that plants a failure. Then, in that file, sweep the 37 example files
  through a bounded pool of six, each file keeping its two launches and its
  pairing checks, and print the verdict lines in file order (FR-009, FR-010,
  FR-012)

- [ ] T013 [P] [US2] Print the base reading of
  `test/source/harness_hunt_cli_test.cpp` into `/tmp/icr/pool-base.md`: the
  child names and the child count one run prints. Then, in that file, start the
  67 scenario children six at a time, keep the check on each child's output, and
  print the failure lines in scenario order (FR-009, FR-010, FR-029)

- [ ] T014 [US2] Re-run the two readings T011 to T013 printed at the base, now
  against the changed driver, and diff them: run the fixture that plants a
  failure through the changed driver and keep the non-zero answer; run the driver
  whole and keep the verdict text word for word
  (`26 of 26 with all rows matching`, 37 `PASS:` lines). For
  `harness_hunt_cli_test`, print the child names and the child count at the
  base and at the tip from the same run and diff them, so a pool that silently
  lost a scenario shows as a missing name, and a green entry then carries its
  child count (FR-012, FR-029 class I)

- [ ] T015 [US2] [SC-003] Wave gate G3, one block at a time on a host that
  carries nothing else: the four entries alone in `build/sanitize` with the CI
  option strings, the same four alone in `build/tsan`, then each lane's suite
  in the CI form. Print the entry count, the exit, the wall time and the load
  average before and after each block. Where `harness_jitter_cli_test` still
  passes its ceiling on one lane and misses it on the other, write the blocked
  row in the shape FR-019 defines, with the clause of Principle VIII it answers
  and the P2 registration beside them, and with both shapes FR-010 asks for, the
  bounded run and the
  direct run: the owning paths `source/harness/jitter_probe.cpp` and
  `source/harness/runner.cpp`, the ceiling that binds the entry, which the
  trailing loop at `test/CMakeLists.txt:2508-2513` sets to 300 s because the entry
  names no `TIMEOUT` of its own and `:1149` carries `RUN_SERIAL` alone, and the
  two figures. Quote beside them the reading that shows no lever stands
  inside `test/source/harness_jitter_cli_test.cpp`. PR #107 grew that file by 94
  lines and moved its constants, so this task reads the file at the tree it
  measures and prints the line numbers of its own reading. The reading taken at
  `1f1f9b20` stands as the shape: `--iterations=2000` at `:2305` then rode one
  mode that costs 1.87 s on the thread lane, the other 36 batch shapes read
  `--repetitions=1 --iterations=2`, the file held no sleep, no wait and no window
  count, and four named modes summed to 49.09 s of the 548.74 s, which left
  499.65 s in the remaining 24 modes and the 43 in-process scenarios, each of
  which pays an instrumented start (`/tmp/icr/jitter-modes-tsan.log`). At
  `origin/master` the same constant reads `:2388` and the file holds 315 `check(`
  calls against 307, so the sweep runs again at the built tree before the row
  prints a figure. Run every name in the three shapes
  FR-010 names and print three exits and three wall times: the bounded shape
  above, the parallel lane form with `-j "$(nproc)"` and no `-R`, and the direct
  shape, which leaves CTest behind and runs the entry's own executable from the
  tree's test directory, as the base record's ceiling controls do:
  `cd build/tsan/test && time ./harness_jitter_cli_test`. A `TIMEOUT` property
  beats `--timeout`, so no `ctest` form lifts a ceiling: the probe at
  `/tmp/ctestprobe` registers a 3 s sleep with `TIMEOUT 1` and
  `ctest --timeout 100000` cuts it at 1.01 s. Each block prints the compiler of
  the tree it ran in, because 548.74 s and 499.65 s come from one compiler's tree
  and a row that names no compiler cannot be compared with either (FR-009 to
  FR-011, FR-014, SC-003, SC-004)

## Phase 5: The first pass at the candidate head (Priority: P2, wave 4)

- [ ] T016 [US4] [FR-026] Run the pass of FR-026 at the candidate head, one log
  per step under `/tmp/icr/pass1/`: the release suite, the two pinned release
  shapes, the address lane, the thread lane, the coverage chain, the coverage
  gate, the prose, spell, page-drift, tier-label and format gates, both consumer
  trees and the DBC target. FR-026 binds
  the pass to the pinned shape each lane's base row carries, and the base
  carries two per lane across three lanes: 897.9 s and 891.0 s in the address
  lane, 1,536.0 s and 1,534.0 s in the thread lane, and the two release masks at
  1,459.90 s and 738.27 s, each run under the mask and job width its base row
  names (`specs/048-hwlat-probe/results/acceptance-final-intel.md:1636-1649`
  together with `:298-299`). R-18 prices the pass at 6.8 h of a quiet host. Every log opens
  with its command, its head, its working tree state, its compiler and its load
  witness, and closes with its exit and wall time. Gate **G4** reads the four
  lane counts, the six pinned shapes, the gate line of the coverage run with its
  own denominators, the analyzer finding count, and no row without a witness
  (FR-001, FR-010, FR-013, FR-026, SC-009)

- [ ] T017 [US3] [FR-017] Run each entry the coverage suite cuts at
  its ceiling by hand from `build/coverage`, and print the exit and the wall
  time of each hand run

- [ ] T018 [US3] [FR-015] Read the gate on the capture T016 wrote:
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`. Print the
  gate line, the allowance, and the `lines` and `branches` pairs of that one
  run. List the rows that stand dark and the entry whose stop left each one
  dark. Where those rows are the six the ledger at
  `tools/dbc/coverage_gate.sh:61-107` names, mark the row BLOCKED in the shape
  FR-019 defines, with the root right it lacks and the clause of Principle VIII
  it answers, and keep the run as a labeled host reading. The script
  also exits 2 for a usage fault and 3 when the capture tool fails: either exit
  prints that tool's own complaint and the command in a row that names no
  verdict (FR-015, FR-016, SC-005, R-11)

- [ ] T031 [US4] [FR-026] Read the static-analysis verdict in the
  `build/analyzer` tree T002 left configured and unbuilt. Run the build under
  `set -o pipefail`, which is the reason `.github/workflows/ci.yml:379-381` gives
  for the `tee` target there, with `set -euo pipefail` at `:385`, `:392` and
  `:398`:
  `set -o pipefail; cmake --build build/analyzer -j "$(nproc)" 2>&1 | tee /tmp/icr/pass1/analyzer-build.log`.
  Print `find build/analyzer -name '*.o' | wc -l` before and after that build,
  beside the count CI itself reads from the log,
  `grep -c 'Building CXX object' "$log"` at `.github/workflows/ci.yml:416`. That
  count step carries `if: always()` (`:411`), so it prints after a failed build as
  well, and `.clang-tidy` holds `readability-identifier-naming` under
  `WarningsAsErrors`, which stops the compile. The pair of counts and the exit
  give four readings. A non-zero exit with findings on that list is a red the gate
  answers, and the row names the findings. Exit 0 with an object count above zero
  is the verdict, and the row prints that count. Exit 0 with a count that stands
  unchanged at zero objects is a tree that built nothing: this task names no
  verdict, and prints the two counts, the command and T002's removal line in the
  shape FR-015 gives a tool fault. A non-zero exit with nothing on that list is a
  compile fault, and the row prints the compiler's own complaint in that same
  shape. Then apply the finding step of `.github/workflows/ci.yml:418-440`:
  collect the findings, read the `WarningsAsErrors` list out of `.clang-tidy`,
  count the findings that stand on that list, and keep the run at exit 0 only when
  that count is zero (FR-026, Principle VIII)

## Phase 6: User Story 3, the record a reader on another machine opens (Priority: P3, wave 5)

- [ ] T019 [FR-025] Rebase onto `origin/master` as it then stands. The first
  rebase of this branch ran on 2026-11-10 onto `1627e756` before any figure; this
  task is the one that runs before the squash. After it, re-read every line cite
  this feature prints into the two records and into `test/CMakeLists.txt`, and
  record the state of any pull request that stands open against `master` beside
  the reading (D-16, D-13)

- [ ] T020 [P] [US3] Append the dated repairs
  block to `specs/048-hwlat-probe/results/acceptance-final-intel.md`: the lane
  table with the pass head handle, the six pinned shapes with their masks and job
  widths, the three-shapes section per red name, the blocked row of the thread
  entry FR-009 names, the coverage rows with their own denominators, the DBC row
  with its interpreter and interface count or its blocked row, and the root rows.
  The block adds no correction line for `048:1546` and `048:1547`: PR #104
  corrected both on `origin/master`, where they already print `$HOME`-relative
  paths, and FR-022 bars rewriting a line that stands corrected; the block prints
  the `grep -n '/home/'` reading that answers nothing. Every blocked row in the block carries the field set FR-019 defines,
  read back by a scan for each field (FR-001, FR-019 to FR-024, SC-005, SC-006,
  SC-007, SC-009)

- [ ] T021 [P] [US3] Append the dated block to
  `specs/041-counter-probe/results/acceptance-final-intel.md` where a figure
  of its family moved, keeping every figure of the row it answers. The line
  `041:352` this task was to correct stands corrected on `origin/master` since
  PR #104, so this task adds no correction line for it and prints the reading
  `grep -n '/home/'` gives, which answers nothing (FR-020, FR-021, FR-022,
  SC-007)

- [ ] T022 [P] [US3]
  Append a block to
  `specs/046-hunter-topdown/results/acceptance-final-intel.md` only where a
  family figure moved, and record the reading that decided it either way
  (FR-020)

- [ ] T023 [US3] Run the audit checks under `/tmp/icr/audit/`: the line-count
  audit of the blocks PR #99, #102 and #103 landed, with a planted deletion in
  a scratch copy answering non-zero; the home-path scan with a planted
  absolute path answering non-zero; the machine-name scan with a planted machine
  name answering non-zero; `git grep -c 'LCOV_EXCL' -- source include` summed at
  84 with a planted marker answering non-zero;
  `git diff --name-only 1f1f9b20 HEAD -- include/speedgun-ng/dbc.hpp` answering
  nothing with a planted header line answering non-zero; the diff of
  `test/CMakeLists.txt` over the base answering no `TIMEOUT` change, joined with
  the diff of the four drivers scanned for `sleep`, a clock call and a processor
  pin, each guard carrying a scratch copy that answers non-zero; the case-count
  diff of every driver this lane changed, with a scratch copy that drops one
  planted case answering non-zero; and the fence diff of the Constraints list in
  `spec.md`, which also carries the red name that stands outside the fence
  (FR-007, FR-008, FR-011, FR-014, FR-016, FR-021, FR-022, FR-024, FR-029,
  SC-004)

- [ ] T024 [US3]
  [FR-030] Wave gate G5: `cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake`,
  the range form against `origin/master`, `codespell`,
  `cmake -P cmake/lint.cmake`, `cmake --build build/dbc --target dbc-gate`,
  which sits outside `ALL` so nothing else runs it, and the drift entry, each
  with its exit (FR-018, FR-030, SC-006)

## Phase 7: User Story 4, one tip and one body (Priority: P4, wave 6)

- [ ] T025 [US4] [FR-025] Squash to one commit with the template of the
  constitution: a section token, a title of 50 columns or fewer, a why-body
  wrapped at 72, `Refs:` lines for 052, 048, 046 and 041, and
  `Approved-by: OmO`. Verify one commit stands between the base and the head,
  read with `git merge-base origin/master HEAD` and
  `git log --oneline <merge-base>..HEAD`, the form that holds after T019's rebase.
  At the head this file names, `git log --oneline 1627e756..HEAD` answers one row.

- [ ] T026 [US4] [FR-026] Run the second pass of FR-026 at that
  squashed head: the six pinned shapes, the coverage chain and gate, the prose,
  spell, page-drift, tier-label and format gates, and the DBC target or its
  blocked row. Every figure this pass does not re-read carries its first-pass
  value with both heads named, prints the two-head diff below, and prints the
  entry count beside the wall time. Where a figure moved,
  write the new figure into the block, squash again, and run the pass again at
  the head that carries the figures. The two-head reading is ruling 29's form,
  and it decides the analyzer and every carried row: print
  `git diff --name-only <pass-1 head> <squashed head> -- include source test example cmake tools external CMakeLists.txt CMakePresets.json .clang-tidy .github/workflows`,
  carry the pass-1 verdict with both heads named where that prints nothing, and
  send the analyzer tree through a rebuild whose object count the row prints
  where it prints a name. Every figure row prints that diff where its head
  differs from the head an earlier row read, and a name in it sends the full list
  of the first pass. Gate **G6** reads the second-pass figures and the carried
  rows against the rows the record prints (D-3, FR-001, FR-004, FR-009, FR-013,
  FR-015, FR-018, FR-019, FR-026, FR-030, SC-001, SC-003, SC-005, SC-009)

- [ ] T027 [US4] [FR-025] [FR-027] Push: `git push -u origin fix/intel-closeout-repairs`
  first, and the lease form for any later push. The expected full hash of the
  lease form comes from the same remote read taken before that push, and every
  push is followed by
  `git ls-remote origin refs/heads/fix/intel-closeout-repairs`, whose answer must
  equal the local head. Print both hashes (FR-027, SC-008)

- [ ] T028 [US4] [FR-025] [FR-028] Open one pull request
  with `gh pr create`, its body carrying the tip hash, the count per lane, the
  root status and every deferred cause in lines of 72 columns or fewer. Merge
  nothing and close no issue, and keep #76 open (SC-008)

## Dependencies and execution order

Phase 1 opens the work: the base readings and the completed builds stand under
every later figure. Phase 2 ends at the red, so the gate exists before the
fix. Phase 3 needs that red. Phase 4 touches no file of phases 2 and 3, so
T011, T012 and T013 run beside them. Phase 5 needs every code task. Phase 6
needs the figures of phase 5 and the rebase of T019. Phase 7 needs the record.

T004 and T005 carry `[P]` and touch different files. Nothing orders them, because
`plan.md` fixes the file name `test/ignore_contracts_shape.sh` that the
registration names, so both writers read the same name from this plan. Everything
else inside a phase runs in parallel except the wave gates.

## Parallel execution, by wave

| Wave | Parallel tasks | Writer count | Gate |
| --- | --- | --- | --- |
| 0 | T002, T003 | 2 | none, both feed later rows |
| 1 | T004 beside T005, then T006 | 2 | G1 at T007 |
| 2 | T008 and T009 in one hand | 1 | G2 at T010 |
| 3 | T011, T012, T013 | 3 | G3 at T015, measurements one at a time |
| 4 | T016 with T031, then T017 and T018 | 1 | G4 over the pass log |
| 5 | T020, T021, T022 after T019 | 3 | G5 at T024 |
| 6 | T025 to T028 in one hand | 1 | G6 at the remote read and the pull request |

## Implementation strategy

The minimum publishable change is US1: T001 to T010. It lands the gate, both
attributes and both consumer builds, and it can stand on its own while the
lanes stay where they are. US2 follows as the driver wave, then the first
pass, then the record, then delivery. A wave whose gate reads red stops the
waves behind it, and the branch keeps its work in the tree, and a stash stands
unused.

## Phase 8: Convergence

- [ ] T029 [FR-029] Run the cross-artifact analysis over `spec.md`, `plan.md`,
  `research.md` and `tasks.md` with a fresh read-only child that carries no
  history of an earlier round, and print the severity counts of each round into
  `/tmp/icr/analyze-rounds.md`. Remediate every critical and high finding it
  names. Then run one fresh child over only the seams that remediation touched,
  and stop: D-17 sets that bound on the owner's instruction of 2026-11-10. A
  medium or low finding that still stands gets one line in `plan.md` Complexity
  Tracking that says why it does not block implementation, and a further full
  round runs only on the owner's word

- [ ] T030 [FR-029] Run the convergence pass against the codebase **before**
  T027 pushes, append the work that stands unbuilt to this file, implement it, and
  stop when only inconsequential items stand open. If it writes a tracked file
  after T027 or T028, the bounded second pass of FR-026 runs again at the head
  that carries that write, and the record gains its row: no head moves while the
  matrix it reports stands behind it (FR-001, FR-024, D-17)
