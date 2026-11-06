# Tasks: Lifecycle of the Programmable Limit Table

**Input**: Design documents from `specs/050-limit-table-lifecycle/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for the user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/limit-table-ownership.md](./contracts/limit-table-ownership.md),
[quickstart.md](./quickstart.md)

**Read at head**: this file states each `path:line` and each count as read at
the head it records, `fba814d2`. That head is one commit over `origin/master`
`02892b57`, so a code cite measures the code this lane shipped. Where a sentence
names the base `858c55d9`, it names the tree this lane first measured, and the
rebase of T026 moved the lines that specs/041-counter-probe shifted.

**Tests**: TDD mode (plan.md, Modules and dependency order). The new test file
stands written and observed red before the code that turns it green. The red has
two halves, and both are measurements. A case that builds a `System` cannot
compile while `System()` stands private, and the compiler line stands recorded. The
behavior red stands already measured against the base archive, one process per
case: the scripted pair answers 2 and 2, the declared unscripted path answers 4,
the ended `System` leaves 8 behind, and the two walk reads answer `nullptr` and 0.
The test lands in the same commit as the code it tests (Principle III).

**Organization**: tasks stand grouped by user story behind the landing order
plan.md fixes: the base state first, the authors and the red, the storage and the
reads, the walk reads, the version, the gates, the evidence, then the three
commits. The wave table under ``Parallel execution, by wave`` fixes the order of
work, and the phase headings only group that work by story. The case numbers that
`spec.md` SC-001 and the Test plan of `plan.md` use name the five cases, and do not
state the order a writer adds them to the file. Every task names its
binary verify: a ctest name with an exit status, a gate command with an exit
status, a grep with a match count, or a line count.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel. Two [P] tasks write disjoint files, and they
  share no build directory, no CMake file, no docs page, no version block and
  no coverage configuration. A chain that shares one of these stands sequential
  and carries no [P]. The rule governs files: two tasks that name one target in
  different files stay parallel, because a target name is no file.
- **[Story]**: one tag every header carries, and it names the story, the
  requirement or the decision the task serves (`US1`, `FR-009`, `D-10`). Where
  a story owns the work, the tag reads `US1` or `US2`.
- **Authoring versus running**: a [P] task writes files and reports what it wrote.
  No [P] task builds, runs ctest, or runs a gate: a wave gate task runs the build
  and the suite once, after the parallel authors land. That is what keeps the
  shared `build` directory out of the parallel set.
- Every description names the exact file path, the requirements it closes, and the
  verify command.

---

## Phase 1: Foundational - the base state and the tools (wave 1)

**Purpose**: the lane measures the ground it lands on, so every later figure reads
as a difference. No source file changes in this phase. T001 writes the record, and
T002 appends to it.

- [x] T001 [SC-005] Record the base state in
  `specs/050-limit-table-lifecycle/results/base-state.md`:
  the base commit, the host, the kernel, the compiler, the two `ctest` verdicts with
  their times, the one pre-existing failure `harness_hunt_cli_test` with the two
  assertions that fail there: the one-block rule at
  `test/source/harness_hunt_cli_test.cpp:3134` and the re-executed scenario at
  `:3683`, both opened at head `fba814d2`. The record re-measures that entry
  alone on this host at the pre-squash head `22ea70ac`, with the command line
  `ctest --test-dir build -R harness_hunt_cli_test --output-on-failure`. It
  names the three skipped entries and the CI job
  verdicts of run 41946446748. For the analyzer it writes two counts per edited
  file: the raw finding count and the distinct count. Every one of the two counts
  stands beside the number of translation units that carry it. A finding counts
  once per file, line, column and check name, and its message text plays no part.
  That key keeps one position one finding, whatever the text a unit prints for
  it. The base reads 22 raw and 22 distinct over 1 unit for
  `source/counters/system.cpp`, and 76 and 19 over 4 units for
  `source/counters/detail/core.hpp`. The third file
  `include/speedgun-ng/counters_system.hpp` reads 550 raw and 5 distinct over 110
  units, because each of those units repeats the same five findings. The record
  keeps 51287 as the base total reading of the whole tree. That figure is the
  count `grep -c ': warning:'` answers over the base analyzer log
  `/tmp/050-dev-build.log`. Those warning lines span 303 distinct path strings,
  and those path strings name 273 files. One file can stand under two spellings:
  `source/counters/detail/core.hpp` carries 76 of those lines under its own
  path, and `source/counters/linux_pmu/../detail/core.hpp` carries 19. Name
  `/tmp/050-base-analyzer-findings.json` as the per-file table re-derived from
  that same log, and cite it for a figure that names one file. Depends on
  nothing: it opens the lane.
  Verify: `test -s specs/050-limit-table-lifecycle/results/base-state.md` exits 0, and `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/050-limit-table-lifecycle` answers 0 findings.

- [x] T002 [SC-003] Stand up the two pinned tools outside the repository, in
  a tool directory of the host, at `clang-format` 18.1.8 and `codespell`, and
  append their paths and versions to `results/base-state.md`. The host
  formatter is 23.1.4 and no speller stands installed, so a run without the
  pins measures the host (SC-003). Depends on T001: the two figures join the
  record T001 wrote.
  Verify: `<tool-dir>/clang-format --version` prints 18.1.8, and `cmake -D FORMAT_COMMAND=<tool-dir>/clang-format -P cmake/lint.cmake` exits 0 at the base head.

**Independent test**: the phase changes no source file, and one document names
every figure the later phases compare against.

---

## Phase 2: User Story 1 - One limit table for each System (Priority: P1) - authors, the red, the move

**Goal**: a `System` reads the limits its own providers published, and a path a
second `System` scripted answers nothing to the first.

**Order**: T003, T004 and T009 author three disjoint files in wave 2, and T005
records the red against a private `System()`. In wave 3, T006 moves the storage
in `source/counters/detail/core.hpp`, and T007 then T013 take
`source/counters/system.cpp`, one writer. T008 owns the public header beside
them, and the test file gains two writers: T014 adds the walk case, then T011
adds the ended-`System` case. T010 then gates the wave, the first build and the
first suite run that hold all five cases. Commit 1 carries the two walk reads
beside the storage move.

**Independent test**: `ctest --test-dir build -R counters_limit_lifecycle_test`
answers the scripted pair 4 and 2 at the head; at the base the same shape answers
2 and 2, measured against the base archive and recorded in
`results/base-state.md`.

- [x] T003 [P] [US1] Write `test/source/counters_limit_lifecycle_test.cpp` in the
  shape of `test/source/counters_objects_limit_test.cpp`: the banner comment with
  the spec and requirement ids, the helpers `fail` and `check`, three case
  functions, and `main`. Each case function takes the prefix `case`, and no helper
  carries it, so one grep counts the cases alone. The prefix joins the name in
  camelBack, with no underscore: N-2 governs a function name, and that rule
  outranks a shorter grep. Read N-2 at `.specify/memory/constitution.md:621`,
  and read `.clang-tidy`, which sets `FunctionCase` to `camelBack` at `:40-41`
  and makes a naming finding an error at `:21`.
  `caseTwoSystemsCarryTheirOwnLimits` scripts 4 on one path through the first
  `System` and 2 through the second and reads both (FR-004).
  `caseDeclaredPathWithoutScriptAnswersNothing` has the second `System` declare
  `cpu` with one leaf and script nothing while the first scripts 4, and reads
  nothing (FR-006). `caseOneSystemAnswersAsAtTheBase` reads a scripted object, an
  unscripted object and the `machine` root of one built `System` (FR-002). Each
  case registers its providers before its first read, reads the fake provider
  only, and names no cpu the fake did not declare. `main` calls each case once.
  Depends on T001. Do not build and do not run.
  Verify: `wc -l test/source/counters_limit_lifecycle_test.cpp` exceeds 100, `grep -c 'auto case' test/source/counters_limit_lifecycle_test.cpp` answers 3 at this step, and `grep -c '^  case' test/source/counters_limit_lifecycle_test.cpp` answers 3 at this step. T011 and T014 add two more cases, so the head answers 5.

- [x] T004 [P] [US1] Add the target and the CTest entry for that file to
  `test/CMakeLists.txt`, beside the `counters_objects_limit_test` block at
  `:715-720`: `add_executable`, `target_link_libraries` against
  `speedgun-ng::speedgun-ng`, `target_compile_features` with `cxx_std_23`, and
  `add_test`. Copy the comment style of the block beside it, and name the spec and
  the requirements in it. Write no `set_tests_properties` block: the suite keeps
  its skip rules in the list at `test/tier_labels.sh:81-94`, and the check at
  `:221-228` faults any entry that carries a code that list does not give it, so
  the `test_tier_labels` entry fails on a new name that copies the model block's
  rule. This entry needs no privilege, so it skips on no condition. Depends on
  T001. Do not build.
  Verify: `grep -c counters_limit_lifecycle_test test/CMakeLists.txt` answers 5,
  the `add_executable` line, the source-path line indented under it, the
  `target_link_libraries` line, the `target_compile_features` line and the
  `add_test` line. `ctest --test-dir build -R test_tier_labels` exits 0, and
  that entry reads the registration back from the configured tree.

- [x] T005 [US1] Record the red. Run `cmake --preset=ci-rocky`, then
  `cmake --build build > /tmp/050-red-build.log 2>&1`, and read from that log the
  compiler diagnosis for `counters_limit_lifecycle_test.cpp`: the private
  `System()` at line 293 of the base `include/speedgun-ng/counters_system.hpp`,
  a line the move of T008 deletes. Append the
  diagnosis and the four base behavior figures to `results/base-state.md`.
  Depends on T003 and T004, and stands before T008, because this build must
  still see `System()` private.
  Verify: `cmake --build build > /tmp/050-red-build.log 2>&1` exits nonzero, and
  `grep -c 'is private' /tmp/050-red-build.log` answers at least 1.

- [x] T006 [P] [US1] Move the storage: in `source/counters/detail/core.hpp` add
  `std::map<std::string, std::size_t> limits` to `System::Impl` (`:196-256`) with
  the comment that states the fill-before-open and read-after-open order, and add
  `System* owner` to `TreeNode` (`:42-50`) with one line naming what it points at
  (FR-001, FR-007, D-3, D-4). These two greps are the `System::Impl` half of
  the FR-001 evidence that the `plan.md` coverage map and the contract evidence
  row name. T007 carries the other half. Depends on T005: the storage moves
  once the red stands recorded. It runs beside T008, because the two headers
  are disjoint and neither builds. One file, one writer, and T007 follows it:
  that writer reads the storage this header gains.
  Verify: `grep -c 'limits;' source/counters/detail/core.hpp` answers 1, and `grep -c 'System\* owner' source/counters/detail/core.hpp` answers 1.

- [x] T007 [US1] Move the write and the read in `source/counters/system.cpp`, in
  one edit pass: delete the anonymous-namespace accessor and its static, which
  stood at `:179-191` at the base `858c55d9`; write `root->owner = this;` in the
  constructor at `:188`, one line before the emplace at `:189`; write
  `node->owner = this;` in `registerProvider` after the last field fill there,
  the `node->leaves` move at `:280`, so the node carries its owner before
  `staged.push_back` at `:282` and before the emplace that puts it in the tree
  at `:287`; write `m_impl->limits` at the commit block (`:285-296`) in place
  of the old table; and read `node->owner->m_impl->limits` in
  `Object::programmableCounters()` (`:434-443`). Keep every other line of the
  three functions byte-identical (FR-001, FR-004, FR-005, FR-006, FR-007). Both
  nodes carry an owner before any read can reach them. That accessor grep is the
  `source/counters/system.cpp` reading of FR-001 that the `plan.md` coverage map
  and the contract evidence row name. It answers 3 at the base and 0 at the head.
  Depends on T006. One file, one writer.
  Verify: `grep -c 'programmableLimitTable' source/counters/system.cpp` answers 0, and `grep -c 'owner = this' source/counters/system.cpp` answers 2, one statement at each creation site.

- [x] T008 [P] [US1] Move `System()` and `~System()` from the private section to
  the public one in `include/speedgun-ng/counters_system.hpp`, give each the
  doxygen block FR-010 names (`@brief` text, `\pre none`, `\post none`), state the
  ownership rule in the class text with the words `ends with its`, keep
  `friend class Object`, keep `~System()` declared here and defined out of line,
  and keep the copy operations deleted (FR-008, FR-010, D-5). Write the thread
  rule of FR-003 into the `Object::programmableCounters` comment block
  (`:75-91`), above the `\pre none` line at `:89`, in the words of D-6:
  registration writes the table before its `System` opens, a read follows that
  open, and two `System` objects share nothing (FR-003). Do not build; the wave
  gate T010 runs the doc gate. Depends on T005: the red needs a private
  `System()`.
  Verify: `grep -n 'System();\|~System();' include/speedgun-ng/counters_system.hpp` answers two lines in the public section, `:269` and `:278`, above the `private:` line at `:368`; `grep -c '@brief' include/speedgun-ng/counters_system.hpp` answers 22, `grep -c 'pre none' include/speedgun-ng/counters_system.hpp` answers 16, and `grep -c 'post none' include/speedgun-ng/counters_system.hpp` answers 14. The base `858c55d9` reads 18, 13 and 12; the rebase onto `specs/041-counter-probe` adds two `@brief` blocks and one `\pre` line, and this task adds two blocks that carry both. `grep -c 'share nothing' include/speedgun-ng/counters_system.hpp` answers at least 1.

- [x] T009 [P] [US1] Add the ownership rule to the counters passage of
  `docs/pages/harness.md`, the multi-pass paragraph that names
  `Object::programmableCounters` at `:1617-1620`: two to four sentences saying the
  table belongs to one `System`, ends with its `System`, and answers its own
  limits (FR-011, D-7). The unit is sentences. Page prose wraps at 80 columns, so
  the verify counts added lines in a band. That paragraph stands outside every
  `sg-example` region, and the nearest marker sits at
  `docs/pages/harness.md:1656`, 36 lines below its last line. No captured
  block in `README.md` can move, so that file stands unchanged. Do not build.
  Depends on T001.
  Verify: `git diff --numstat -- README.md` answers nothing, and `git diff --unified=0 -- docs/pages/harness.md | grep -c '^+[^+]'` answers between 2 and 8 at this step, before the single commit carries the passage. The head answers 0, because that passage stands committed.

- [x] T010 [US1] Wave gate. Build `build`, then run the new test by name, then the
  whole suite with `ctest --test-dir build -j 20 --output-on-failure`, then the
  pinned-core run with `taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure`,
  then the four gates, and fix what fails. Those two command lines carry the base
  readings of 94.3 seconds and 367.4 seconds in `results/base-state.md`, so keep
  each figure with the flags that produced it. This build is the first that holds
  the five written cases, and this run is
  the first that reads them. The `dbc-gate` target reads a configured checked
  tree, so run first the two lines the plan's gate block places before that
  target:
  `cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3`
  and `cmake --build build/dev`. Read every suite verdict against
  `results/base-state.md`: an entry that passed at the base must pass here. The
  lint run and the spell run use the two pinned tools T002 stands up.
  Depends on T002, T006, T007, T008, T009, T011, T013 and T014, because commit 1
  carries the two walk reads and the five cases beside the storage move.
  Verify: `ctest --test-dir build -R counters_limit_lifecycle_test --output-on-failure` exits 0; no `ctest` entry that passed at the base fails at the head under `ctest --test-dir build -j 20 --output-on-failure` on the host, and no `ctest` entry that passed at the base fails under `taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure`; the base answers 180 passed of 181 entries under each of those two commands; the one base failure `harness_hunt_cli_test` stands recorded in `results/base-state.md` and in CI run 41946446748, and this lane fixes none of the base failures; `cmake -D FORMAT_COMMAND=<pin> -P cmake/lint.cmake`, `cmake -D SPELL_COMMAND=<pin> -P cmake/spell.cmake`, `cmake --build build/dev`, `cmake --build build/dev --target dbc-gate` and `python3 tools/prose/prose_gate.py --check all --mode range --base origin/master` each exit 0.

## Phase 3: User Story 2 - The table ends with its System (Priority: P2)

**Goal**: a `System` that ends leaves no limit behind, and a fresh `System` over
the same scripted path starts from an empty table.

**Independent test**: the ended-`System` case reads nothing at the head and read 8
at the base, one process each.

- [x] T011 [US2] Add `caseAnEndedSystemLeavesNoEntry` to
  `test/source/counters_limit_lifecycle_test.cpp`: a scoped `System` scripts 8 for
  `cpu`, ends, and a fresh `System` that declares `cpu` with one leaf and scripts
  nothing answers no value; the same function then builds a third `System` that
  scripts 1 and reads 1, the second scenario of US-2 (FR-005, FR-006). `main`
  gains the one matching call. Depends on T003 and T014: one test file, one
  writer at a time, and this task closes that file for wave 3. It lands before
  the wave gate T010, where the five cases first build and first run. Do not
  build and do not run.
  Verify: `grep -c 'auto case' test/source/counters_limit_lifecycle_test.cpp` answers 5 and `grep -c '^  case' test/source/counters_limit_lifecycle_test.cpp` answers 5.

- [x] T012 [US2] Read the shipped rule in the three places that state it, and
  correct any one of them that drifts. They are the contract file
  `specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md`, the
  counters passage `docs/pages/harness.md:1617-1620`, and the class text of
  `System` in `include/speedgun-ng/counters_system.hpp`. The three readings must
  say one thing (FR-011, IV.1). At the base, only the contract carries the words
  `ends with its`, at `:93`. The header and the page carry neither, so the
  three-file count shows whether both authors landed. A correction joins the
  commit that owns its file: commit 1 (T017) carries the header and the page,
  and the record commit (T024) carries the contract. Depends on T008 and T009.
  Do not build.
  Verify: `grep -l 'ends with its' include/speedgun-ng/counters_system.hpp docs/pages/harness.md specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md | wc -l` answers 3, and the prose gate over the feature directory answers 0 findings.

---

## Phase 4: The two walk reads (FR-009)

**Goal**: a handle of any `System` answers its own tree for `parent()` and
`children()`.

**Order**: T013, T014, T011 and T015 run in wave 3, before the wave gate T010.
T015 reads what T013 wrote, writes no file, and runs no build, so it stands
after that task and before the gate.

**Independent test**: the walk case answers `cpu` and one entry at the head and
answered `nullptr` and 0 at the base.

- [x] T013 [FR-009] Read `node->owner` in place of `System::local()` in
  `Object::parent()` (`source/counters/system.cpp:445-460`) and `Object::children()`
  (`:503-528`), keeping every other line of both functions identical, and keep
  `System::local()` itself untouched (FR-009, D-5). Depends on T007: one file,
  one writer. The same writer lands this move before the wave gate T010, so
  commit 1 carries the whole file. No case reads a walk path before T014, which
  also lands before that gate.
  Verify: `grep -c 'System::local()' source/counters/system.cpp` answers 1, the surviving site being `System::local()` itself.

- [x] T014 [FR-009] Add `caseTheWalkReadsStayOnTheirOwnTree` to
  `test/source/counters_limit_lifecycle_test.cpp`: one built `System` holds
  `machine`, `cpu` and `cpu/core-0`; `parent()` of `cpu/core-0` answers `cpu`,
  `children()` of `cpu` answers one entry, `parent()` of the `machine` root answers
  no pointer, and `children()` of the leaf answers none. The leg on the local
  `System` takes the same two reads through
  `*System::local().object("machine")`, a handle of the tree the process shares.
  No case registers a provider into that `System`, so it holds only the
  `machine` root: `parent()` answers no pointer and `children()` answers no
  entry (FR-009, SC-001). `main` gains the fourth call. Depends on T003, for one
  test file and one writer, and on T013, for the code the case reads. It lands
  in wave 3, before the wave gate T010, and T011 takes the file after it. That
  gate is where this case first builds and first runs. Do not build and do
  not run.
  Verify: `grep -c 'auto case' test/source/counters_limit_lifecycle_test.cpp` answers 4 and `grep -c '^  case' test/source/counters_limit_lifecycle_test.cpp` answers 4.

- [x] T015 [FR-009] Read the two changed functions against the contract file and
  the data model: `the route each read takes` in
  `specs/050-limit-table-lifecycle/data-model.md` names the owner for all three
  reads, and no third read may follow the owner without a requirement that asks for
  it (X.3). Depends on T013, whose two functions it reads. It writes no file
  and runs no build, so it stands in wave 3 after T013 and before the wave
  gate T010.
  Verify: `grep -n 'System::local()' source/counters/system.cpp` names one site, and `grep -c 'in the same three steps' specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md` answers 2, one row per walk read at `:113` and `:114`.

---

## Phase 5: The version record and the landing (wave 4)

- [x] T016 [FR-012] Move the version to 0.33.0 across the nine lines FR-012
  names: `CMakeLists.txt:78`, `test/source/views_test.cpp:307`, `:316` and
  `:317`, `test/source/harness_cli_table_test.cpp:1474`, `:1489` and `:1490`,
  `example/docs/hunt_topdown.expected:2` and `docs/pages/harness.md:2586`. The
  rebase onto the tree that has taken specs/041-counter-probe FR-040a, FR-053
  took the free patch this decision first measured, so the number is a minor.
  The base `858c55d9` reads 0.31.0, that rebase brings the 0.32.0 of
  specs/041-counter-probe, and 0.33.0 is the next free minor (D-2).
  The history line `CMakeLists.txt:61` stays where it stands, because it records
  the release that took 0.31.0. It sits beside the lines at `:30-58`, which keep
  0.26.0 to 0.30.0 for the same reason. The precedent commit `a69455b6` moved
  only its `VERSION` line, and left every line of that record alone. Two of the
  nine stand as one pair the drift gate compares: the capture at
  `example/docs/hunt_topdown.expected:2` and the page block at
  `docs/pages/harness.md:2584-2586`, so the gate runs after the move. Three
  further lines carry the stamp for specs/041-counter-probe: the comment at
  `test/source/harness_cli_table_test.cpp:129`, the capture at
  `example/docs/counter_probe.expected:2` and the page line at
  `docs/pages/harness.md:1371`. That pair stands under the same gate, so the
  commit moves those three with the nine. Move the counters `SOVERSION` line to
  8 at `CMakeLists.txt:168`, because this feature changes the public counters
  header.
  Keep the harness line at 21 at `CMakeLists.txt:444`, because no harness header
  changes for this feature. No exported layout moves, because the table and the
  tree node live in the private header (D-4). Keep the comment beside each
  number that states what moves it (D-2, FR-010, hard rule 12). Build before
  any reading: `cmake --preset=ci-rocky` and `cmake --build build`. The stamp
  reaches these binaries through a `PRIVATE` define of the harness target, so
  the readers answer only in a rebuilt tree. That is the reading the precedent
  commit `a69455b6` records for its own move.
  `test/source/harness_cli_test.cpp:841` names the reader that compares a built
  binary against the tree. Depends on T017: the edits ride after commit 1 lands,
  so no version line joins it.
  Verify: `grep -rl '0\.31\.0' test example docs` names no file, `grep -c 'VERSION 0.33.0' CMakeLists.txt` answers 1, `grep -c '0\.31\.0' CMakeLists.txt` answers 2 for the two record lines that name that number, `grep -c 'SOVERSION 8' CMakeLists.txt` answers 1, `grep -c 'SOVERSION 21' CMakeLists.txt` answers 1, `ctest --test-dir build -R 'views_test|harness_cli_table_test'` exits 0, and `python3 tools/docs/example_drift.py .` exits 0, the command the tree runs at `tools/ci/prepush.sh:183` and `.github/workflows/ci.yml:31`.

- [x] T017 [SC-001] Land commit 1, `Counters: ...`, with six pathspecs in one
  commit: `source/counters/detail/core.hpp`, `source/counters/system.cpp`,
  `include/speedgun-ng/counters_system.hpp`,
  `test/source/counters_limit_lifecycle_test.cpp`, `test/CMakeLists.txt`, and
  `docs/pages/harness.md` for its counters passage (Principle III, plan.md
  Modules). The wave gate ran all five cases, so this commit carries a test file
  that holds all five. Message form: the `<Section>: <one-line imperative>`
  title at 50 characters or fewer, a why-body wrapped at 72, and the two footer
  lines `Refs: specs/050-limit-table-lifecycle T-LTL` and `Approved-by: OmO`.
  Depends on T010, T012 and T015, because the T012 reading can correct the header
  and the page, and this commit owns both files.
  Verify: the record commit that held these readings named those six paths and nothing else, and the squash of T025 folded it into this head, so `git show --stat --format=%B HEAD` names the whole lane. The prose gate in range mode answers 0 findings over the range from `origin/master`.

- [x] T018 [FR-012] Land commit 2, `CMake: ...`, with the pathspecs of T016
  (D-2). Depends on T016 and T017.
  Verify: T016 wrote those five files alone, and the squash of T025 folded that commit into this head, so `git show --stat HEAD` names the whole lane and carries those five among them.

---

## Phase 6: Gates, coverage and evidence (wave 5)

- [x] T019 [SC-004] Run the checked build once per feature:
  `cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3`
  and `cmake --build build/dev`. Count a finding once per file, line, column and
  check name, so one position in one file is one row whatever its message text
  says. Compare that distinct count for each of the three edited files against the
  base distinct counts 22, 19 and 5. Write the translation-unit count of this run
  beside each figure (SC-004, plan.md Risks). A header states its findings
  once in every unit that includes it, so a raw total rises with each unit added
  and names no new finding. The base total of 51287 stands as one reading of one
  base log, and the head need not match it. Depends on T018.
  Verify: the build exits 0, the three distinct counts do not rise above 22, 19 and 5, and the reading names the unit count of this run.

- [x] T020 [SC-001] Run the sanitizers: `cmake --preset=ci-sanitize` with
  `cmake --build build/sanitize` and `ctest --test-dir build/sanitize -j 1
  --output-on-failure --no-tests=error`, then the same three steps for `ci-tsan`.
  Write both verdicts into `specs/050-limit-table-lifecycle/results/sanitizers.md`:
  the two `ctest` runs, their entry counts, and every failing entry by name. Read
  each one against the base, where the `sanitize` job fails one entry and the
  `tsan` job passes (plan.md Risks). The thread sanitizer job stays the arbiter of
  a race (D-6), so both readings stand on that record. Depends on T019.
  Verify: both `ctest` runs fail no entry that the base run passed; `test -s specs/050-limit-table-lifecycle/results/sanitizers.md` exits 0, and `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/050-limit-table-lifecycle/results/sanitizers.md` answers 0 findings.

- [x] T021 [SC-002] Measure coverage: `cmake --preset=ci-coverage`,
  `cmake --build build/coverage`, `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error`,
  `cmake --build build/coverage -t coverage`, then
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`. Write that
  reading into `specs/050-limit-table-lifecycle/results/coverage.md`. That gate
  run exits 1 at `tools/dbc/coverage_gate.sh:45-48` when the line rate of the
  trace reads below 100 percent. Its branch section at
  `tools/dbc/coverage_gate.sh:102-118` prints no figure on this host, so the
  record reads its own figures from the trace file. One named command carries
  each measure. Each row states its measure: the line figure of one changed
  file, the branch figure of one changed file, or the tree-wide pair. This pass
  over the `SF:`, `DA:` and `BRDA:` rows answers the per-file pair of each
  changed file:
  `awk '/^SF:/{sf=substr($0,4)} /^DA:/{split(substr($0,4),d,","); ln[sf]++; if (d[2]+0>0) lh[sf]++} /^BRDA:/{split(substr($0,6),b,","); bn[sf]++; if (b[4]=="0") bd[sf]++} END{for (s in ln) printf "%s %d of %d lines, %d of %d branches\n", s, lh[s], ln[s], bn[s]-bd[s], bn[s]}' build/coverage/coverage.info`
  For the tree-wide pair, this summary answers both rates:
  `/tmp/lcov-soft --branch-coverage --summary build/coverage/coverage.info`
  The base answers 282 of 282 lines and 137 of 140 branches for
  `source/counters/system.cpp`. It answers 79 of 79 lines and 31 of 34 branches
  for `source/counters/detail/core.hpp`. It answers 20 of 20 lines and 6 of 6
  branches for `include/speedgun-ng/counters_system.hpp`, the third file this
  lane edits. That file carries no dark branch row. The two files that carry
  dark rows therefore hold 6 of them between them, three in each file. The rows
  answer these figures, because a record can state a summary field its own rows
  contradict. The `BRF:` field of the `source/counters/system.cpp` record reads
  134, and its `BRDA:` rows number 140. The tree-wide pair at the base reads
  97.5 percent lines over 17786 and 94.9 percent branches over 8829. That pair
  leaves 450 branch rows dark across the tree, and the record keeps that figure
  as one reading of the host trace. Two rules bar the head. The first bar is
  100 percent of the lines of each of the three changed files, read from the
  `DA:` rows of the trace. The line rule of the in-tree gate reads the summary
  of one whole trace at `tools/dbc/coverage_gate.sh:45-48`. That exit says
  nothing about any one file. The second bar is that the two files which carry
  dark rows add none beyond the 6 the base carries. The allowance of 21 rows at
  `tools/dbc/coverage_gate.sh:102` is the tree-wide ledger the gate applies to
  the union trace of the CI `coverage` job. The record names it as that CI
  measure. The record states the host tree pair as a reading the lane does not
  bar. It names the marker count and the exact command line that
  produced each figure. It records the host `lcov` defect the way the wrapped
  read prints it. The three sites it names are
  `source/harness/runner.cpp:707`, `source/harness/runner.cpp:820` and
  `source/harness/hunt_topdown.cpp:155`, and the count line reads
  `inconsistent: 3`.
  A plain `lcov` run names one of those three and stops, so the record cites
  the wrapped read (SC-002, D-9, plan.md Coverage plan).
  Depends on T020.
  Verify: `test -s specs/050-limit-table-lifecycle/results/coverage.md` exits 0, every line the change wrote reads covered in that table, `grep -cE '[0-9]+ of [0-9]+ lines' specs/050-limit-table-lifecycle/results/coverage.md` answers at least 2, `grep -cE '[0-9]+ of [0-9]+ branches' specs/050-limit-table-lifecycle/results/coverage.md` answers at least 2, the marker count `grep -rn LCOV_EXCL source include | wc -l` stays at or below 84, and `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/050-limit-table-lifecycle/results/coverage.md` answers 0 findings.

- [x] T022 [SC-005] Write the acceptance record
  `specs/050-limit-table-lifecycle/results/acceptance.md` from the base record of
  T001, `specs/050-limit-table-lifecycle/results/sanitizers.md` and
  `specs/050-limit-table-lifecycle/results/coverage.md`, the last of which carries
  the readings of T021. It carries host, kernel and compiler, the full `ctest`
  count, and the `taskset -c 0,1` count. Each reading stands beside the head sha
  that `git rev-parse HEAD` answered when that reading ran. The squash in T025
  and the amends in T026 and T027 move that sha, and T028 moves no head,
  because its reading comes after the push. The record therefore names each
  reading at the head it tested, and it claims nothing about the squashed head.
  It states that the head CI verdict stands on the pull request T028
  opens. It names each base failure as pre-existing and gives that failure its
  base evidence (SC-005, D-10). The record names the five gate verdicts of
  SC-003: the format gate, the spell gate, the drift gate, the prose gate and
  the `dbc-gate` target. It also carries the checked build of T019, with its
  distinct analyzer counts and its unit count. That record names the two
  sanitizer runs of T020 and the coverage reading of T021. Depends on T021.
  Verify: `test -s specs/050-limit-table-lifecycle/results/acceptance.md` exits 0 and the prose gate over the feature directory answers 0 findings.

- [x] T023 [SC-005] Write the pull request body
  `specs/050-limit-table-lifecycle/results/pr-body.md` in the Pull Request
  Quality form of the constitution: the summary, the rule as shipped, the
  acceptance table of T022, the coverage figures, the follow-on tables this lane
  leaves, and the footer `Refs:` and `Fixes #` lines the ticket names. Write one
  line in the shape the gate prints, and name its measure on that same line:
  `branch shortfall 6 rows (two files, host trace)`. The number is the sum of
  dark branch rows over the two changed files which carry dark rows. The T021
  table states that sum per file. It reads 6 at the base: 3 in
  `source/counters/system.cpp` and 3 in `source/counters/detail/core.hpp`. Keep
  that line apart from the allowance. The allowance of 21 rows at
  `tools/dbc/coverage_gate.sh:102` belongs to the union trace of the CI
  `coverage` job. The in-tree gate applies that allowance to a tree-wide count.
  The host tree figure of 450 dark rows stands as a recorded reading of the
  host trace, and the body cites the coverage record of T021 for that figure.
  Write each `ctest` count as `N passed of M entries`, and give each of the two
  `SOVERSION` lines of `CMakeLists.txt`, `SOVERSION 8` and `SOVERSION 21`, one
  line of its own.
  The body names the CI leg of SC-001 and the base verdicts of run 41946446748,
  with the jobs that failed there. It names
  `specs/050-limit-table-lifecycle/results/acceptance.md` as the record of the
  readings, and it states that the head CI verdict stands on the pull request.
  T028 builds the body of that request from this file plus its CI paragraph.
  Depends on T022.
  Verify: `grep -c 41946446748 specs/050-limit-table-lifecycle/results/pr-body.md` answers at least 1, `grep -cE 'branch shortfall [0-9]+ rows' specs/050-limit-table-lifecycle/results/pr-body.md` answers 1, `grep -cE '[0-9]+ passed of [0-9]+ entries' specs/050-limit-table-lifecycle/results/pr-body.md` answers 2, and `grep -c 'SOVERSION 8\|SOVERSION 21' specs/050-limit-table-lifecycle/results/pr-body.md` answers 2.

- [x] T024 [SC-005] Land commit 3, `Docs: ...`, with the five record files and
  the contract file: `specs/050-limit-table-lifecycle/results/base-state.md`,
  `specs/050-limit-table-lifecycle/results/sanitizers.md`,
  `specs/050-limit-table-lifecycle/results/coverage.md`,
  `specs/050-limit-table-lifecycle/results/acceptance.md`,
  `specs/050-limit-table-lifecycle/results/pr-body.md`, and
  `specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md` (plan.md
  commits, D-10). That contract joins the list so a wording correction from
  T012 has a commit that carries it. The file enters that commit only where
  T012 changed its words. The record lands before the squash, so the squashed
  head carries it. The message form follows T017. Depends on T023.
  Verify: the record commit named five paths under `specs/050-limit-table-lifecycle`, and it answered 6 where the T012 reading reworded the contract `specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md`, the one path that could join them. The squash of T025 folded that commit into this head, so `git show --name-only --format= HEAD` names the whole lane. Its body carries the `Refs: specs/050-limit-table-lifecycle T-LTL` footer, and no record path stands outside that directory.

- [x] T025 [D-10] Squash the lane into one commit and re-measure on that head
  (D-10, G-23). Rebuild before any reading: `cmake --preset=ci-rocky` and
  `cmake --build build`, so each binary on that head carries the stamp the tree
  now holds. The reading then covers the two `ctest` runs, the five gates that
  T022 names, and the coverage summary. Append both readings to
  `results/acceptance.md`, and mark which reading came before the squash and
  which after. Each one stands beside the sha that `git rev-parse HEAD` answers
  for the squashed head, read before the amend that carries the record. That
  append joins the squashed commit through `git commit --amend --no-edit`, so
  the lane keeps one commit and the tree stands clean. The amend adds record
  files only, so it changes no file those readings measured. T026 and T027 fold
  their own writes into that same commit, and neither creates a second one.
  T028 writes on the pull request, outside the commit. Depends on T024.
  Verify: `git rev-list --count origin/master..HEAD` answers 1, `git status --porcelain` answers nothing, the two `ctest` counts on the squashed head match the readings of T010, and on that head each of the five SC-003 commands exits 0: `cmake -D FORMAT_COMMAND=<the clang-format 18.1.8 pin of T002> -P cmake/lint.cmake`, `cmake -D SPELL_COMMAND=<the codespell pin of T002> -P cmake/spell.cmake`, `python3 tools/docs/example_drift.py .`, `python3 tools/prose/prose_gate.py --check all --mode range --base origin/master`, and `cmake --build build/dev --target dbc-gate` on a `build/dev` configured with the `-DPython3_EXECUTABLE=/usr/bin/python3` line that T010 names.

- [x] T026 [D-11] Re-run every gate after a rebase on `master`, reading both
  sides of every conflict, before any push (D-11). The re-recorded gate verdicts
  join that same single commit through `git commit --amend --no-edit`, once the
  rebase lands. Depends on T025.
  Verify: `git merge-base --is-ancestor origin/master HEAD` exits 0, `git status --porcelain` answers nothing, and the gate verdicts of T010 stand re-recorded in `results/acceptance.md` for the new head.

- [x] T027 [SC-005] Run `/speckit.converge` over
  `specs/050-limit-table-lifecycle/`, the step that closes wave 5 in plan.md. It
  reads each artifact against the code and the records, in the way that command
  defines the reading. Every artifact correction that reading finds joins the one
  squashed commit through `git commit --amend --no-edit`, the way T025 folds in
  its append. That amend is the step that produces the clean tree this task then
  verifies, and the commit keeps the message T017 states. Depends on T026.
  Verify: `git status --porcelain` answers nothing, `git rev-list --count origin/master..HEAD` answers 1, and the prose gate over the feature directory answers 0 findings.

- [ ] T028 [SC-001] Push the branch, open its pull request, and read the CI leg
  of SC-001. The lane owner runs this task, and no worker runs it. First, push
  the branch: `git push -u origin 050-limit-table-lifecycle`. The first push
  carries no lease flag, and a later push carries
  `--force-with-lease=050-limit-table-lifecycle:<last-pushed-sha>`. One push
  follows the last amend of T027, so no run stands cancelled. Second, open the
  pull request:
  `gh pr create --base master --head 050-limit-table-lifecycle --title <the title T017 states> --body-file specs/050-limit-table-lifecycle/results/pr-body.md`
  A branch push starts no run, because `.github/workflows/ci.yml:7-14` triggers
  `push` on `master` only, so this request is what starts the run to read.
  Third, read the run that the `pull_request` event starts on this head. Compare
  it with the base verdicts of run 41946446748. No job that passed at the base
  may turn red, and no `ctest` entry that passed at the base may fail. Fourth,
  record that verdict and its run number on the pull request. Add the CI
  paragraph to a copy of
  `specs/050-limit-table-lifecycle/results/pr-body.md`, write that copy outside
  the tree at `/tmp/050-pr-body-ci.md`, then run
  `gh pr edit --body-file /tmp/050-pr-body-ci.md`. That write stays off the
  commit: an amend would move the head, cancel the run whose verdict this step
  read, and start another run. The commit's record then says where the CI
  verdict stands, and the request shows that verdict. The request carries the
  squashed head as `headRefOid`, which `gh pr view` answers and `SC-005`
  records. The run tests the merge commit that the `pull_request` event checks
  out, and that merge commit has a sha apart from `headRefOid`. Depends on T027.
  Verify: `gh pr view --json headRefOid --jq .headRefOid` answers the head sha, `gh pr view --json body --jq .body` names that run number, `git rev-list --count origin/master..HEAD` answers 1, and `git status --porcelain` answers nothing.

---

## Dependencies and execution order

| Task | Depends on |
| --- | --- |
| T001 | none: it opens the lane |
| T002 | T001 |
| T003 | T001 |
| T004 | T001 |
| T005 | T003, T004 |
| T006 | T005 |
| T007 | T006 |
| T008 | T005 |
| T009 | T001 |
| T010 | T002, T006, T007, T008, T009, T011, T013, T014 |
| T011 | T003, T014 |
| T012 | T008, T009 |
| T013 | T007 |
| T014 | T003, T013 |
| T015 | T013 |
| T016 | T017 |
| T017 | T010, T012, T015 |
| T018 | T016, T017 |
| T019 | T018 |
| T020 | T019 |
| T021 | T020 |
| T022 | T021 |
| T023 | T022 |
| T024 | T023 |
| T025 | T024 |
| T026 | T025 |
| T027 | T026 |
| T028 | T027 |

Every row names a task that finishes earlier, so the order runs forward. Three
rows name a higher number, because the task ids stay put while the work moves:
T010 waits for T011, T013 and T014, which land before that gate; T011 waits for
T014, the other writer of its test file; and T016 waits for T017, so no version
line joins commit 1.

- Phase 1 gates nothing but the honesty of every later figure. T001 and T002 write
  one file, in that order.
- Wave 2 holds T003, T004 and T009: three disjoint files, and no worker builds.
- T005 builds once between the waves, while `System()` still stands private.
- Wave 3 holds T006, T007, T008, T011, T013, T014 and T015. T006 writes the
  private header, then T007 and T013 write `source/counters/system.cpp`, and
  T008 owns the public header beside them.
- T003, then T014, then T011 are one writer on the test file, in that order, and
  all five cases stand written before the wave gate.
- T013 follows T007 on `source/counters/system.cpp`, before the wave gate, so
  one writer holds that file end to end and commit 1 carries the whole move.
- T015 reads the two walk functions after T013, writes no file, and finishes
  before that gate too.
- T010 is the single wave gate where the build and the suite run, and its two
  tool gates use the pinned tools T002 stands up.

## Parallel execution, by wave

| Wave | Files | Tasks |
| --- | --- | --- |
| 1 | `specs/050-limit-table-lifecycle/results/base-state.md` | T001, then T002 |
| 2 | `test/source/counters_limit_lifecycle_test.cpp` | T003 |
| 2 | `test/CMakeLists.txt` | T004 |
| 2 | `docs/pages/harness.md` | T009 |
| 3 | `source/counters/detail/core.hpp`, then `source/counters/system.cpp` | T006, T007, then T013 |
| 3 | `include/speedgun-ng/counters_system.hpp` | T008 |
| 3 | `test/source/counters_limit_lifecycle_test.cpp`, after T003 and T013 | T014, then T011 |
| 4 | `specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md` | T012, before T017 |
| 4 | commit 1 | T017 |
| 4 | `CMakeLists.txt` | T016 |
| 4 | `test/source/views_test.cpp` | T016 |
| 4 | `test/source/harness_cli_table_test.cpp` | T016 |
| 4 | `example/docs/hunt_topdown.expected` | T016 |
| 4 | `docs/pages/harness.md`, the captured banner | T016 |
| 4 | the rebuild of `build` | T016, after those edits |
| 4 | commit 2 | T018 |
| 5 | the checked build of `build/dev` | T019 |
| 5 | `specs/050-limit-table-lifecycle/results/sanitizers.md` | T020 |
| 5 | `specs/050-limit-table-lifecycle/results/coverage.md` | T021 |
| 5 | `specs/050-limit-table-lifecycle/results/acceptance.md` | T022 |
| 5 | `specs/050-limit-table-lifecycle/results/pr-body.md` | T023 |
| 5 | the record commit | T024 |
| 5 | the squash | T025 |
| 5 | the rebase on `master` | T026 |
| 5 | `specs/050-limit-table-lifecycle/`: the artifacts and the records | T027 |
| 5 | the push, the pull request and the CI reading | T028 |

No author of wave 2 or wave 3 builds. T005 builds once, after wave 2 lands and
before wave 3 starts. T016 rebuilds `build` before its reading, and T025 rebuilds
it again on the squashed head, because the version stamp reaches a test binary
through a `PRIVATE` define.
T010 runs the build and the suite once these files land, the walk reads among
them, and the five cases stand written when it runs. T015 reads the two walk
functions in wave 3, after T013 and before that gate. T012 reads the three files
in wave 4 before T017 lands commit 1, so a correction has its commit.

## Implementation strategy

The minimum shippable lane is T001 through T011, where the wave table runs T013,
T014 and T015 inside that span and the gate T010 closes it: one `System`, one
table, the walk reads correct, five cases green. Phase 3's contract reading
(T012) and the version commit (T016 to T018) follow, then the gates, the record
commit (T024) and the squash (T025). Three tasks amend the commit after that
squash: T025, T026 and T027. Each folds its write into the one amended commit
through `git commit --amend --no-edit`. T028 writes nothing inside the tree,
because its reading comes after the push, and it records the CI verdict on the
pull request. No task after the squash creates a commit. That one amended
commit is the head the merge takes, and no second commit stands on the branch. A
merge stands blocked until T025 measures the squashed head, T026 clears a
rebase, and T027 clears the converge reading with a clean tree. T028 closes it
by opening the pull request and recording the CI verdict there.
## Phase 7: Convergence

- [ ] T029 [Constitution II] Reconcile the process-wide limit-source store with the
  per-`System` limit table this lane shipped, or narrow the two postconditions to the
  `System` they read against. `source/counters/detail/limit_fill.hpp:66-68` states that
  a filled answer carries a count exactly where it names a finder that is not `NONE`,
  and that `objectLimitSource(device.path)` answers that same finder.
  `source/harness/detail/json_report.hpp:302-305` repeats the claim for the run-level
  record. The store spans the process, because its writer records beside the figure it
  publishes (`source/counters/detail/limit_source_record.hpp:38-47`), while the counts
  now stand in one table per `System` (`source/counters/system.cpp:293`). A second
  `System` can therefore answer a count whose path holds no record, or answer none
  while the path holds one. `clearLimitSources()`
  (`source/counters/detail/limit_source_record.hpp:49-58`) is a test seam for one
  process, and no call drops the labels of a `System` that ends. Choose one reading and
  write it into the files of specs/041-counter-probe: give the store the lifetime of the
  `System` that published the figure, or state in both postconditions that the pair
  stands for `System::local()`. This lane leaves both files alone, because
  specs/050-limit-table-lifecycle names the limit table and no other surface.
  Depends on T028.
  Verify: one `System` that scripted a limit and one that scripted none answer different counts for the same path, and the biconditional of `limit_fill.hpp:66-68` still holds for both, or both postconditions name the `System` they read against. `cmake --build build` answers no `error:` line, and `ctest --test-dir build -R 'limit_source|counter_limits|limit_disclosure'` answers 100 percent passed.

- [ ] T030 [SC-001] Read the two sanitizer jobs of this head and settle the clang
  question this host raised. `test/source/harness_limit_disclosure_test.cpp:131` and
  `:132` hold two file-scope counters that clang 23.1.4 reports as
  `-Werror,-Wunused-but-set-global`, so the entry builds on no sanitizer tree of this
  host and CTest reports it `Not Run`. That file reads byte-identical here and at
  `02892b57`, so the diagnostic arrives with specs/041-counter-probe and not with this
  lane. GCC carries no such diagnostic, so the plain tree and the coverage tree build
  the entry and pass it. The `sanitize` job (`.github/workflows/ci.yml:147`) sets
  `CXX: clang++` at `:153`, and the `tsan` job stands at `:230`, so both name a clang
  from the Ubuntu package whose release no reading here names. Read both job logs of the
  run this pull request starts. If the entry compiles there, record the host toolchain
  as a local condition in `results/acceptance.md`. If it does not, mark the two counters
  `[[maybe_unused]]` or read them in an assertion, in a change that owns
  specs/041-counter-probe, and open that change on its own. Depends on T028.
  Verify: the run number of this head stands recorded beside both job verdicts, and each log answers either no diagnostic for that file or the two lines quoted above.

- [ ] T031 [plan: R-2] Make the version binding of `views_test` independent of the
  working directory. `test/source/views_test.cpp:57-68` walks up to eight parents from
  `std::filesystem::current_path()` for a `CMakeLists.txt` that carries `project(`, so a
  build tree outside the source tree finds none, and the three checks at `:314-319`
  answer a failure message where they name a number. That behaviour cost
  one entry to the base reading of `results/rebase-base.md`, which records it as a fault
  of the layout and not of the code. Pass the source root to the binary at build time,
  or set the working directory of the CTest entry at `test/CMakeLists.txt:715-722` to the
  source root. Depends on T027.
  Verify: from a build tree placed outside the source tree, `ctest --test-dir <tree> -R views_test` answers 100 percent passed, and `git status --porcelain` names the test file or one build file afterwards.
