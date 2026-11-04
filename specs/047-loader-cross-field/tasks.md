# Tasks: Loader Cross-Field Checks for the Pass Fields

**Input**: Design documents from `specs/047-loader-cross-field/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for the user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/loader-pass-pairs.md](./contracts/loader-pass-pairs.md),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Modules and dependency order). The test task of
each wave stands written and observed failing before the code task that turns it
green. The test and its code land in one commit (FR-020). A red stands as a
run-time failure, because every symbol the new cases read already stands in the
tree.

**Organization**: tasks stand grouped by user story behind the landing order
plan.md fixes: the law first, the red wave, the accepted forms, the reader, the
docs and the version, the gates, the evidence. Every task names its binary
verify: a ctest name with an exit status, a gate command with an exit status, a
grep with a match count, or a line count.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel. Two [P] tasks touch disjoint files and share no
  build directory, no CMake target, no docs page, no version block and no
  coverage configuration. A chain that shares one of these stands sequential and
  carries no [P].
- **[Story]**: the user story the task serves (`US1`, `US2`).
- Every description names the exact file path, the requirements it closes, and
  the verify command.

---

## Phase 1: Foundational - the law lands first (wave 1)

**Purpose**: spec 024 FR-038 carries the rule before the reader enforces it
(FR-001, D-8). Nothing in the reader changes in this phase.

- [x] T001 [P] Amend FR-038 of `specs/024-multipass-collection/spec.md`
  (`:658-671`): add the three pair checks of "The rules", the pair rule of
  FR-002, the presence reading of D-17, and the rule-1 waiver of D-14 with its
  reason (the stop paths publish a count of 0 beside a settled method, and a
  hunt entry names the forced method beside the published count). Keep the
  standing `spreadWarning` sentence unchanged, and keep the EARS form of the
  requirement (FR-001, FR-003, FR-009).
  Verify: `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/024-multipass-collection` answers 0 findings, and `grep -c 'metricMethod' specs/024-multipass-collection/spec.md` rises by at least 1 over the base count. Commit as `Docs:` with the pathspec `specs/024-multipass-collection/spec.md` and the footer `Refs: specs/024-multipass-collection T-LCF`.

**Independent test**: the diff touches one file, one requirement, and no code.

---

## Phase 2: User Story 1 - Refuse a document the writer cannot make (Priority: P1) - the red wave

**Goal**: every refusal of the three rules answers one line whose leading path
names the pair's non-count field, and the first fault stops the read.

**Independent test**: the load test binary answers the planted refusals at the
head of the lane, and answers nothing at the base. The red count is recorded.

- [x] T002 [P] [US1] Write the five refusal cases in
  `test/source/harness_json_load_test.cpp`, before `main` (the file runs 27
  cases today, `:2205-2238`), and add five calls in `main` between `:2207` and
  `:2234`: `thePassMethodRefusesItsCount` (a measured entry with `AUTO` and 2,
  `MULTIPLEX` and 3, `MULTIPLEX` and 0, `PASSES` and 0),
  `thePassSpreadRefusesItsCount` (a spread of 3.5 beside a count of 1),
  `thePassTagRefusesItsCount` (a tag of 2 beside a count of 2, and a tag of 1
  beside a count of 1, in a row of a later entry for the path),
  `thePassPairRefusalNamesItsPartnerPath` (the four lines of plan.md, "The
  refusal lines", byte for byte), and `thePassPairChecksRunInReadOrder` (a
  document with a tag fault and a method fault answers the method refusal, one
  with a spread fault and a tag fault answers the spread refusal, and one whose
  `spreadWarning` disagrees answers the standing refusal ahead of the three). Plant on the rich fixture `kRich` (`:181-218`) and on the scripted
  run, and set `outcome` to `MEASURED` where rule 1 must run. No case sleeps,
  reads a clock, names a made-up cpu, or needs a PMU (FR-003, FR-005, FR-006,
  FR-007, FR-009, FR-016, FR-017, D-15, D-16).
  Verify: `cmake --build build --target harness_json_load_test` exits 0, then `./build/test/harness_json_load_test` exits non-zero and prints `N checks failed` with five `FAILED:` lines naming the new cases (red observed; record N), and `ctest --test-dir build -R '^harness_json_load_test$'` reports the entry as Failed.
- [x] T003 [US1] Write the pairing case `aPassFieldDisagreementAnswersOneQuotedLine`
  in `test/source/harness_compare_cli_test.cpp`, beside `oneFaultLine`
  (`:782-800`), and add its call in `main` (`:1477-1512`). The case writes a
  document of schema major 1 at run time, in the way the file does today, plants
  `AUTO` beside a `passCount` of 1 in one entry, and asserts status 2, one line
  across the two streams, and the pair's own words in that line. The standing
  `aForeignSchemaMajorAnswersTheLoadFault` (`:875-884`) keeps its answer, because
  the version gate stands ahead of the entry read (FR-008, D-21).
  Verify: `cmake --build build --target harness_compare_cli_test` exits 0, then `./build/test/harness_compare_cli_test test/data` exits non-zero and names the case (red observed), and `ctest --test-dir build -R '^harness_compare_cli_test$'` reports the entry as Failed.

**Checkpoint**: US1 stands red, and the refusal texts stand fixed by the tests.
Measured with the reader absent: the load binary printed `13 checks failed`, every line
naming one of the five cases, and the compare command answered 2 failures.

---

## Phase 3: User Story 2 - Keep every document the writer makes (Priority: P1) - the accepted forms

**Goal**: every document the writer makes loads, in each of its shapes, and the
pair rule keeps a document with one key absent loading.

**Independent test**: the accepted-form cases pass at the base and at the head;
the waiver cases pass at the base, because the reader enforces nothing there
today.

- [x] T004 [US2] Write the four accepted-form cases in
  `test/source/harness_json_load_test.cpp`, and add four calls in `main`:
  `aStoppedEntryKeepsItsMethodAndCount` (a `SKIPPED` entry with `MULTIPLEX` and
  0, and with `PASSES` and 0, beside the rich fixture's `PASSES` and 2),
  `aHuntEntryKeepsItsMethodAndCount` (a measured entry with `MULTIPLEX` and 3
  beside a minimal `hunt` object: the six keys, two empty arrays),
  `aPassPairWithOneKeyAbsentLoads` (a count of 2 with no method key, `PASSES`
  with no count key, a null spread beside a count of 1, a null tag beside a count
  of 1), and `everyWrittenPassFormLoads` (the scripted run's document, `AUTO`
  with 0, `PASSES` with 3 beside tags 0, 1, 2, the schema 1.0 fixture, and the
  same document at schema 1.12 and 1.13; each round trip answers the values it
  wrote) (FR-002, FR-004, FR-010, FR-011, FR-012, FR-016, FR-017, D-14, D-18).
  Verify: `cmake --build build --target harness_json_load_test` exits 0, and `./build/test/harness_json_load_test` exits non-zero with the five T002 failures standing and no new `FAILED:` line naming one of the four new cases (record both counts).

**Checkpoint**: US2 stands green at the base, and US1 stays red. Measured: the
four accepted-form cases print no `FAILED:` line of their own, and the red count
of the load binary stays 13.

---

## Phase 4: The reader - the three checks (wave 3)

**Goal**: `readEntry` enforces the three rules, in the order of D-16, and turns
both stories green in one commit.

- [x] T005 [US1] [US2] Add the three checks at the end of `readEntry` in
  `source/harness/json_load.cpp`, after the top-down record read (`:2784-2791`),
  before the closing `return {}`: rule 1 behind the guard of D-14 (the method key
  stands, the count key stands, the outcome reads `MEASURED`, no `hunt` key
  stands) with the three method comparisons of FR-003 and the word from
  `detail::enumWord` over `detail::kMetricMethodWords`; rule 2 behind the count
  key and a non-null spread, refusing a count below 2; rule 3 over
  `result.rows` and each row's `metrics`, refusing a count below 2 and a 0-based
  tag at or above the count. Write each check as one `if`, each pair guard as one
  `&&`, and no `switch` (D-19). Name the four refusal lines of plan.md exactly
  (D-15). Comment the block with specs/047 FR-003, FR-005, FR-006 and the waiver
  of D-14, in the standing comment style of the pass block at `:2720-2725`.
  Check the diff names no writer, schema, checker, comparison, public header, runner, CLI, `.github/` or specs/045 file (FR-013). Verify: `cmake --build build` exits 0, `./build/test/harness_json_load_test` exits 0, `./build/test/harness_compare_cli_test test/data` exits 0, `ctest --test-dir build -R 'harness_json_(load|report|schema)_test|harness_compare' --output-on-failure` exits 0, and `ctest --test-dir build --output-on-failure` exits 0 with the one known load-sensitive row named if it fails.
- [x] T006 [P] Add the four sentences after `docs/pages/harness.md:1429`, in the
  section "### The loader rule": the three pairs, the pair rule that keeps an
  older document loading, and the three method words with their counts. Carry no
  requirement id, no spec identifier, no commit hash and no principle number, the
  classes the drift gate forbids on a user page (FR-014, D-7).
  Verify: `python3 tools/docs/example_drift.py .` exits 0, and `python3 tools/prose/prose_gate.py --check prose --mode tree --paths docs/pages/harness.md` answers 0 findings.
- [x] T007 Move the version from `0.30.0` to `0.30.1` at the five pinned
  sites: `CMakeLists.txt:59`, `test/source/views_test.cpp:279-280` (and the
  comment at `:270`), `test/source/harness_cli_table_test.cpp:1453-1454` (and the
  comments at `:129` and `:1440`), and the current capture pair
  `example/docs/fwq_ftq.expected:2` with the block of `docs/pages/harness.md`
  paired with it at `:1970`. Leave the two `SOVERSION` numbers, the schema
  version and the older captures alone (FR-015, D-2).
  Verify: `cmake --build build` exits 0 (the harness library rebuilds whole, the version define is `PRIVATE`), and `ctest --test-dir build -R '^(views_test|harness_cli_table_test)$' --output-on-failure` exits 0.

**Checkpoint**: both stories are green, the page states the rule, and the version
reads 0.30.1. Measured: both binaries print `ok`, at 36 cases and at 38, and the
two readers of the stamp pass in the rebuilt tree.

---

## Phase 5: Gates - the hard gates of Principle VIII (wave 4)

**Purpose**: every gate runs at its own command line, on the pinned tools of
D-20, and each exit stands recorded.

- [x] T008 (partial) Run the suite twice: `ctest --test-dir build --output-on-failure`, then
  `taskset -c 0,1 ctest --test-dir build --output-on-failure`. Record both counts,
  the failures, and the skips (FR-019, SC-003). The known load-sensitive rows of
  `harness_jitter_window_test` and the PMU skip of `harness_topdown_run_test`
  stand named in the record.
  Verify: the pinned run exits 0; the full run exits 0 on an idle host, and a failure there names the load and the row.
  Reading against the clause (partial): the pinned run exits 8 and the full run
  exits 8, each with 176 passed, 1 failed and 1 skipped of 178. The failing row is
  `harness_jitter_window_test` on `theSeriesRoundTripsThroughLoadReport`, and the
  base tree answers the same row.
- [x] T009 Configure the checked build, then run the five gates:
  `cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3`
  (the Python pin carries PyYAML for the pair gate, the package CI installs,
  `.github/workflows/ci.yml:88`), then
  `cmake -D FORMAT_COMMAND="$HOME/.local/bin/clang-format" -P cmake/lint.cmake`,
  `cmake -P cmake/spell.cmake`, `python3 tools/docs/example_drift.py .`,
  `python3 tools/prose/prose_gate.py --check all --mode range --base origin/master`,
  and `cmake --build build/dev --target dbc-gate` (the target form of the pair
  gate, `cmake/dbc-gate.cmake`). The range gate reads every commit message of
  the range, so it carries the commit-template check of FR-020. Record the five
  exits (FR-019, FR-020, SC-004, D-20).
  Verify: each command exits 0, and the range gate answers 0 findings.
- [x] T010 Run the checked build once per feature, in the `build/dev` tree T009
  configured: `cmake --build build/dev`. Record the clang-tidy and cppcheck
  verdicts (FR-019, SC-004, Principle V, Principle VIII). The Clang compile and
  the sanitizer jobs of the matrix stand in CI
  (`.github/workflows/ci.yml:313`, `:147`, `:230`), and their verdicts read into the
  record of T013.
  Verify: the command exits 0, and no new suppression joins the tree.
- [x] T011 (partial) Run both sanitizer suites, in the standing order:
  `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j "$(nproc)"`,
  `ctest --test-dir build/sanitize -j 1 --output-on-failure --no-tests=error`,
  then `cmake --preset=ci-tsan`, `cmake --build build/tsan -j "$(nproc)"`,
  `ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error`. The
  two presets stand apart, because the compilers reject the pairing (Principle
  VIII).
  Verify: all six commands exit 0, and no race and no sanitizer report appears
  (FR-019, SC-004, Principle VIII).
  Reading against the clause (partial): the configure and build commands exit 0;
  the ASan and UBSan suite answers 171 of 173 with one failure, and the tsan suite
  answers 171 of 174 with two failures and one race at
  `jitter_probe.cpp:708 in seriesFigures`. The base tsan tree answers the same two
  failures and the same race, and no suppression joins the tree.
- [x] T012 (partial) Measure coverage: `cmake --preset=ci-coverage`,
  `cmake --build build/coverage -j "$(nproc)"`,
  `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error`,
  `cmake --build build/coverage --target coverage`, then
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`, with
  `PERL5LIB="$HOME/perl5/lib/perl5"` in the environment of all five. Record the
  host reading at the head beside the base reading of 99.5 percent
  lines and 98.8 percent branches, the reading of `source/harness/json_load.cpp`
  alone, and the union figure from the coverage job's artifact. Add no exclusion
  construct (FR-018, SC-002, D-12, D-19, D-20).
  Verify: the line reading of `json_load.cpp` reads 100 percent, the new branch
  records read reached, and the union shortfall stands at 21 rows or fewer.
  Reading against the clause (partial): `json_load.cpp` reads 99.2 percent of 2194
  lines, and its 81 dark lines stand at the base; the new branch records read
  reached; the union has no measurement point at base or head, and the host gate
  exits 3 on the `lcov` record at `source/harness/runner.cpp:707` at base and
  head.
Measured on the host: `json_load.cpp` reads 99.2 percent of 2194 lines and 100
percent of 178 functions; the lane adds 34 lines and 42 branches and the
uncovered counts stand unchanged at 81, 50 and 108; the host shortfall is the
standing 108 rows of the base, and the gate reading is the CI union.

---

## Phase 6: Evidence and landing (wave 5)

- [x] T013 Write `specs/047-loader-cross-field/results/acceptance-2026-11-03.md`:
  the host facts (host, kernel, machine, compiler), the two ctest counts, the five
  gate exits, the tidy verdict, the ASan/UBSan verdict and the thread-sanitizer
  verdict, the three coverage readings, the red counts of T002, T003 and T004,
  the case count read by `grep -c` over the `main` call list, and the SC-001 to
  SC-005 verdicts (FR-016, FR-018, SC-001, SC-002, SC-005).
  Verify: the file names both ctest counts, the five gate exits and the three coverage figures, and `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/047-loader-cross-field` answers 0 findings once the file is tracked.
- [x] T014 Write `specs/047-loader-cross-field/results/pr-body.md`: the host, the
  kernel, the compiler version, the head sha, the two ctest counts, the three
  coverage readings and the union figure, the five gate exits, the three
  sanitizer and tidy verdicts, the two rebase notes for specs/046, the reading of
  the branch allowance against Principle VI and VIII (D-12), and the reading of
  the README loader row behind the IV.1 check (SC-002, D-13).
  Verify: the body names the host, the kernel, the compiler, the head sha, both
  counts and the coverage figures.
- [x] T015 Land the lane, the lane operation of the spec's Constraints block
  (no requirement id stands behind it): `git push -u origin 047-loader-cross-field` first, then
  `git push --force-with-lease` after any later rewrite; `gh pr create` with the
  body of T014; `git ls-remote origin 047-loader-cross-field` equals
  `git rev-parse HEAD`.
  Verify: the three commands answer the equality and the pull request number.
  Measured: pull request 74 stands open, and the remote line equals the head at
  each push.

---

## Dependencies

```text
T001 (the law)            -> independent, lands first
T002 (red, refusals)      -> independent of T001
T003 (red, compare)       -> independent of T001, T002
T004 (accepted forms)     -> after T002 (one file, one main)
T005 (the reader)         -> after T002, T003, T004 (turns both red waves green)
T006 (docs)               -> after T005 (a page states a rule the reader enforces)
T007 (version)            -> after T006 (one build tree), lands last of the code
                             waves
T010 (the checked build)  -> after T009 (it builds the tree T009 configured)
T008 .. T012 (gates)      -> after T005, T006, T007
T013, T014 (evidence)     -> after T008 .. T012
T015 (landing)            -> after T013, T014
```

## Parallel blocks

- `T001` runs beside `T002`: a spec file and a test file share nothing. `T003`
  follows `T002`: the two test binaries build in the same `build` tree, so the
  two share a build directory and stand sequential, and each names its own
  target.
- `T006` then `T007` run in order after T005: the version move rebuilds the
  harness library whole in the tree the drift gate of T006 reads, so the two
  share a build directory and stand sequential.
- `T009` then `T010` run in order: T009 configures `build/dev` and runs the pair
  gate there, so T010 builds the tree T009 configured.
- `T011` and `T012` do not run in parallel: each drives a full suite over its own
  build directory, and the host carries the load of one at a time.

## MVP scope

The MVP is T001 to T005: the law, the red wave, and the reader. It closes both
user stories, and the loader refuses every document the three rules name. The
docs sentences of T006 ride the second commit beside the reader, so the page
states the rule the day the reader lands. The version, the gates and the
evidence follow, and no part of the MVP is separable from them: FR-020 binds each test to the code of T005, and
Principle VIII binds the gates to the head.

---

## Phase 7: Convergence

- [x] T016 Re-run `ctest --test-dir build --output-on-failure` and
  `taskset -c 0,1 ctest --test-dir build --output-on-failure` on an idle host
  until the pinned run exits 0, and run
  `ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error` at
  the base `b1dd7670` so the race at `jitter_probe.cpp:708` carries a base
  reading; write both readings into
  `specs/047-loader-cross-field/results/acceptance-2026-11-03.md` and the
  pull request body per FR-019, SC-003, SC-004, Constitution VIII (partial)
  Measured: three runs on a quiet host (the full suite, the pinned suite, and a
  second pinned run) each answer 177 of 178 with the one failure at
  `harness_jitter_window_test` on `theSeriesRoundTripsThroughLoadReport`, and
  ctest exits 8; the base tree answers the same row, so the pinned run does not
  reach exit 0 on this host. The tsan tree at `b1dd7670` answers 2 failures of 174
  and the same race at `jitter_probe.cpp:708 in seriesFigures`.
- [x] T017 Produce the coverage union figure at the gate's measurement point:
  complete a coverage suite whose entries all pass, clear the `lcov`
  inconsistent record at `source/harness/runner.cpp:707` that stops
  `tools/dbc/coverage_gate.sh` at exit 3, read the union shortfall against the
  21-row allowance, and record the union figure with the
  `source/harness/json_load.cpp` line reading in
  `specs/047-loader-cross-field/results/pr-body.md` beside the G-20 figures per
  FR-018, SC-002, Constitution VI (partial)
  Measured: the union has no measurement point on this matrix at base or head, so
  the host reading carries the lane's delta: 34 lines and 42 branches added, every
  one covered, and the uncovered counts unchanged at 81 lines, 50 functions and
  108 branches. The host gate exits 3 at base and at head on the same `lcov`
  record at `source/harness/runner.cpp:707`; clearing it needs an ignore class the
  spec bars this lane from adding, and that file stands untouched here.
- [x] T018 Rewrite the four sentences of the loader section of
  `docs/pages/harness.md:1431-1435` so they state the pair rule that keeps an
  older document loading and the three method words with their counts, inside
  two to four sentences at 80 columns and free of the classes the drift gate
  forbids; re-run `python3 tools/docs/example_drift.py .` and
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  docs/pages/harness.md` per FR-014, D-7, T006 (partial)
  Measured: four sentences at `docs/pages/harness.md:1431-1436` now carry the
  three pairs, the pair rule, the three method words with their counts and the
  refusal shape; the drift gate and the docs prose check answer 0.
- [x] T019 Restate the lane figures in
  `specs/047-loader-cross-field/results/acceptance-2026-11-03.md` and
  `specs/047-loader-cross-field/results/pr-body.md` from
  `git diff --shortstat b1dd7670..HEAD`: the file count, the insertion count,
  the source-and-test count, and the head sha per FR-016, SC-002, T013, T014
  (partial)
  Measured: 14 commits (`db91c935` .. `561095df`), 20 files, 3240 insertions, 17
  deletions at the merged tip, and 5 files with 603 insertions for source and tests.
  PR #75 carries 9 record commits over `561095df`, the count that
  `git rev-list --count origin/master..HEAD` answers at the final head.
- [x] T020 Keep the untracked `prompt.md` at the repository root out of the lane,
  so the working tree carries no root file inside the feature's Scope per the
  Scope section of spec.md (unrequested)
  Measured: the file stands untracked, in no commit and in no pull request, and it
  is the owner's ticket text: the owner decides its place, and the lane leaves it
  where it stands.

---

## Phase 8: Convergence, second pass

- [x] T021 Write the coverage reading into
  `specs/047-loader-cross-field/spec.md` D-12 and FR-018 as a fact, with the owner
  ruling pending: the union figure has no
  measurement point at the base or at the head, the host gate exits 3 on the
  `lcov` inconsistent record at `source/harness/runner.cpp:707`, the lane's
  evidence is its own delta of 34 instrumented lines and 42 branch records with
  every one covered and the uncovered counts unchanged at 81 lines, 50 functions
  and 108 branches, and the host reading of `source/harness/json_load.cpp` stands
  at 99.2 percent of 2194 lines with the 81 dark lines standing at the base. Carry
  the same paragraph into `specs/047-loader-cross-field/results/pr-body.md`, and
  file the `runner.cpp:707` record and the `harness_jitter_window_test` row as two
  issues whose numbers the record names. per FR-018, SC-002, Constitution VI,
  Constitution VIII (partial)
  Verify: `grep -c 'P2 exception' specs/047-loader-cross-field/spec.md` answers 0,
  `gh issue list --search 'runner.cpp:707'` answers one row, and
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  specs/047-loader-cross-field` answers 0 findings.
  Measured: D-12 and FR-018 carry the reading as a fact: the host shortfall of 108
  rows at the base and at the head, above the allowance of 21, the lane's delta with
  every added line and branch record covered, and no claim that the gate passes. The
  owner ruling stands pending as #76, and the jitter row stands as #77, both named in
  the record and the pull request body.
- [x] T022 Mark T008, T011 and T012 `(partial)` in
  `specs/047-loader-cross-field/tasks.md`, and add one line per gate to
  `specs/047-loader-cross-field/results/acceptance-2026-11-03.md` that states the
  reading against the verify clause: the full and the pinned `ctest` each answer
  176 passed, 1 failed and 1 skipped of 178 with exit 8, the ASan and UBSan suite
  answers 171 of 173 with one failure, and the tsan suite answers 171 of 174 with
  two failures and one race at `jitter_probe.cpp:708 in seriesFigures`; carry the
  base reading beside each line. per FR-019, SC-003, SC-004, Constitution VIII
  (partial)
  Verify: `grep -c '(partial)' specs/047-loader-cross-field/tasks.md` answers 7,
  and the prose tree check over the feature directory answers 0 findings.
  Measured: T008, T011 and T012 carry (partial) with a reading against each
  clause, and the record carries one line per gate beside its base reading. The
  tree's (partial) count reads 16, where the clause estimated 7.
- [x] T023 Replace the claim at `specs/047-loader-cross-field/quickstart.md:43`
  that both `ctest` runs answer exit 0 at acceptance with the reading the record
  carries: each run answers 176 passed, 1 failed and 1 skipped of 178, ctest exits
  8, and the base tree answers the same row; keep the sentence that names the
  pinned run as the record for the load-sensitive FWQ rows. per FR-019, SC-003,
  T008, T016 (contradicts)
  Verify: `grep -c 'exit 0 at acceptance' specs/047-loader-cross-field/quickstart.md`
  answers 0, and the prose tree check answers 0 findings.
  Measured: the quickstart names 176 passed, 1 failed and 1 skipped of 178
  with exit 8, and the base row beside it.
- [x] T024 Restate the branch-record figures from the record's measured delta in
  `specs/047-loader-cross-field/plan.md:77`, `plan.md:325-332`,
  `quickstart.md:132`, `data-model.md:35`, `spec.md:138` and `spec.md:579`: the
  tree adds 34 instrumented lines and 42 branch records, every one covered, and
  rule 1 reads one comparison per method, so the `min/max pair per method` wording
  and the 8, 6 and 12 arm counts leave the artifacts; keep the D-19 shape rule of
  one `if` per refusal, one `&&` per pair guard and no `switch`. per FR-018, D-19,
  SC-002 (contradicts)
  Verify: `grep -c 'min/max pair' specs/047-loader-cross-field/spec.md` answers 0,
  `grep -c 'min/max pair' specs/047-loader-cross-field/research.md` answers 0,
  `grep -c '26, plus 4' specs/047-loader-cross-field/data-model.md` answers 0, and
  the prose tree check answers 0 findings.
  Measured: the six places read one comparison per method and the measured 34
  lines and 42 records; no min/max wording and no 8, 6, 12 arm count stands.
- [x] T025 Restate the lane figures at shas a ref reaches in
  `specs/047-loader-cross-field/results/acceptance-2026-11-03.md:9,13`,
  `results/pr-body.md:15` and the measured note of T019 at `tasks.md:335-336`: the
  merged lane at `561095df` reads 14 commits over `b1dd7670`, 20 files, 3240
  insertions and 17 deletions with PR #74 merged, and the record lane at the head
  reads 18 commits, 20 files, 3274 insertions and 17 deletions with PR #75 open;
  name four record commits over the merge. per FR-016, SC-002, T013, T014, T019
  (contradicts)
  Verify: `git merge-base --is-ancestor 561095df origin/master` exits 0,
  `git diff --shortstat b1dd7670..561095df` prints 20 files and 3240 insertions,
  and `grep -c 76e76667 specs/047-loader-cross-field/results/*.md` answers 0.
  Measured: the records and the task note name `561095df` at 14 commits, 20
  files and 3240 insertions, and the record lane at 18 commits with 3274
  insertions; no artifact names `76e76667`.
- [x] T026 Amend the Scope section of `specs/047-loader-cross-field/spec.md:715-732`:
  add `example/docs/fwq_ftq.expected` with its paired block of
  `docs/pages/harness.md` to the in-scope list beside the version line, add
  `.specify/feature.json` as the Spec Kit pointer the lane moves, and scope the
  out-of-scope line about the jitter-probe and FWQ/FTQ files to the source and
  test files of specs/043 and specs/045. per FR-015, D-2, X.3 (contradicts)
  Verify: `grep -c 'fwq_ftq.expected' specs/047-loader-cross-field/spec.md` answers
  at least 1, and the prose tree check answers 0 findings.
  Measured: the Scope list names the capture pair and `.specify/feature.json`,
  and the out-of-scope line reads to the source and test files of the jitter probe
  and of the FWQ/FTQ path.
- [x] T027 Restate the added-line count at
  `specs/047-loader-cross-field/results/acceptance-2026-11-03.md:84`,
  `results/pr-body.md:59`, `tasks.md:213` and `tasks.md:314` from the record's own
  table: 17600 minus 17566 stands 34 instrumented lines added, all covered, and
  the branch delta stands 42. per FR-018, SC-002, X.4 (contradicts)
  Verify: `python3 -c 'print(17600-17566, 17519-17485, 8743-8701)'` prints
  `34 34 42`, and `grep -c '35 instrumented' specs/047-loader-cross-field/results/*.md`
  answers 0.
  Measured: 17600 minus 17566 stands 34; the records, the T012 note and the
  T017 note read 34 lines and 42 records.
- [x] T028 Restate the base host coverage reading at
  `specs/047-loader-cross-field/plan.md:337` and `research.md:177` from
  `build/logs/base-cov-capture.log`: 99.5 percent lines (17485 of 17566), 97.3
  percent functions (1804 of 1854) and 98.8 percent branches (8593 of 8701).
  per FR-018, SC-002 (contradicts)
  Verify: `grep -c '17484 of 17565' specs/047-loader-cross-field/*.md` answers 0,
  and `grep -c '17485 of 17566' build/logs/base-cov-capture.log` answers at least 1.
  Measured: the plan and research read 17485 of 17566, the figure the base
  capture log carries; the pattern of the verify clause stands only in that clause.
- [x] T029 Amend `specs/047-loader-cross-field/research.md:186-188` and
  `plan.md:64` from three version sites to the five sites FR-015 names: the
  project block, the two readers of the stamp with their comments, and the current
  capture pair. per FR-015, D-2 (contradicts)
  Verify: `grep -c 'three sites' specs/047-loader-cross-field/research.md` answers
  0, and `grep -c 'three version sites' specs/047-loader-cross-field/plan.md`
  answers 0.
  Measured: R-12 and the plan's scale line read five sites, and name them.
- [x] T030 Rewrite the loader-section block at `docs/pages/harness.md:1431-1436`
  so it stays inside the two-to-four-sentence budget of FR-014 and names the two
  entries the method check leaves alone: a stopped entry, and an entry that carries
  a hunt record. per FR-014, D-7, Constitution IV (partial)
  Verify: `python3 tools/docs/example_drift.py .` exits 0, and
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  docs/pages/harness.md` answers 0 findings.
  Measured: four sentences at `docs/pages/harness.md:1431-1436` name the
  stopped entry and the hunt entry; the drift check and the docs prose check answer 0.
- [x] T031 Record in `specs/047-loader-cross-field/results/acceptance-2026-11-03.md`
  the two git identities the lane's commits carry, author
  `OmO <omo@speedgun-ng.local>` on all 18 and the committer identity of the merge
  (not OmO) on 17, and state which identity the Constraints block of
  `spec.md:695-697` binds. per FR-020, Pull Request Quality (partial)
  Verify: `git log --format='%an <%ae>' b1dd7670..HEAD | sort -u` prints one line,
  and `git log --format='%cn <%ce>' b1dd7670..HEAD | sort -u` prints the two
  identities the record names.
  Measured: the record names the author on all 18 commits, the two committer
  identities, and that the Constraints block binds the author identity.
- [x] T032 Correct the citation in the Verification row at
  `specs/047-loader-cross-field/contracts/loader-pass-pairs.md:144` from FR-011 to
  FR-008, the requirement the version-refusal case answers. per FR-008, FR-011
  (contradicts)
  Verify: `grep -c '(FR-011)' specs/047-loader-cross-field/contracts/loader-pass-pairs.md`
  answers 0.
  Measured: the version-refusal row cites FR-008. The clause's count of 0
  overreaches: the two stopped-entry rows cite FR-011, the requirement that governs
  them, and stand.
- [x] T033 Restate `specs/047-loader-cross-field/plan.md:62-64`, `plan.md:120`,
  `plan.md:264`, `plan.md:396` and the T006 line of `tasks.md:140` from about 40
  added lines and three sentences to the 48 lines at the end of `readEntry` and
  the four sentences at `docs/pages/harness.md:1431-1436`. per FR-014, FR-015,
  X.4 (contradicts)
  Verify: `grep -c 'three sentences' specs/047-loader-cross-field/plan.md` answers
  0, and `git diff --numstat b1dd7670..HEAD -- source/harness/json_load.cpp`
  prints 48.
  Measured: the plan and the T006 line read 48 lines and four sentences, and
  the numstat prints 48.

---

## PR 75 fixes 1

Owner review of PR #75, round 1. Records only: no source, test, CMake, CI or gate
change. Base: the PR 75 branch at `fdea0388`.

- [x] R75-001 Remove the person name and the email from all files of the feature.
  Write "the committer identity of the merge (not OmO)". Check:
  `git grep -niE '[Cc]harles|[Aa]rcher|@[g]mail' -- specs/047-loader-cross-field`
  prints nothing. The same pattern over the whole tree prints 195 lines in
  pre-existing files of other features: machine path strings that carry the account
  name, GitHub owner URLs in their records, and the prose-gate fixtures. None is a
  person name in prose, and none stands in this pull request's files.
  Measured: the record and the task row read the committer identity of the merge
  (not OmO); the check over the feature tree prints nothing; the whole-tree pattern
  prints 195 lines in pre-existing files of other features.
- [x] R75-002 Correct the counts: the merged lane reads 14 commits
  (`db91c935` .. `561095df`); this pull request reads N commits over `561095df`,
  where N is `git rev-list --count origin/master..HEAD` at the final head.
  Measured: 14 commits (`db91c935` .. `561095df`), 20 files, 3240 insertions and 17
  deletions at the merged tip; PR #75 reads 9 commits over `561095df` at the final
  head.
- [x] R75-003 Remove "P2 exception" from D-12, FR-018 and T021. Write the fact
  only: the host shortfall reads 108 branches at the base and at the head, above
  the allowance of 21; the lane adds no uncovered line or branch; the owner ruling
  is pending (issue #76). Do not write that the gate passes.
  Measured: the three places carry the fact and no ruling: 108 rows at the base and
  at the head above the allowance of 21, no uncovered line and no uncovered branch
  added, the owner ruling pending as #76, and no claim that the gate passes.
- [ ] R75-004 Add rows PM-001 to PM-003 of `speedgun-ng-pr74-fixes-1-prompt.md` to
  tasks.md, and tick the rows that this pull request completes.
- [ ] R75-005 Write the pull request body again in `results/pr-body.md` with whole
  lines (no break inside a word, lines 72 or fewer). Give host chilly, the kernel,
  the GCC version, the measured head sha, the `ctest` counts (full and
  `taskset -c 0,1`) and the coverage figures. Then run
  `gh pr edit 75 --body-file specs/047-loader-cross-field/results/pr-body.md`.
- [x] R75-006 Run `cmake -P cmake/spell.cmake` and
  `python3 tools/prose/prose_gate.py --base origin/master`. Both exit 0.
  Measured: the spell gate exits 0 with `.codespellrc` as its only config, and the
  prose range gate over `origin/master` answers 0 findings across 17 sources.
- [ ] R75-007 Tick R75-001 to R75-007.
