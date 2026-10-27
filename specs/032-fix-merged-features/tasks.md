# Tasks: Fix Merged Features

**Input**: Design documents from `specs/032-fix-merged-features/`

**Prerequisites**: plan.md, spec.md (FR-005 to FR-065, SC-001 to SC-008,
Decisions D-01 to D-16), research.md (R-01 to R-12), data-model.md,
contracts/ (three files), quickstart.md

**Tests**: Included. FR-055 puts each fix's test first and seen
failing, in the same commit as the fix; each story's test task names
the executable and the red run it observes, and the fix task that
follows turns it green in that commit. Principle VI holds 100 percent
line and branch coverage on those paths. No test sleeps, reads a wall
clock, makes a cpu up, calls `setenv` or `clearenv`, or holds a
lambda inside an `SG_*` macro (FR-056, FR-057, Constraints); a child
receives a variable through its environment array, and the suite
passes under `taskset -c 0,1` (SC-004).

**Organization**: Phase 1 carries the run reading, the version
reservation and the standing-record confirmations. Phase 2 carries the
group B check items: each confirms master already holds a fix, records
the file:line evidence, and reopens a fix task when a check does not
hold. Phases 3 to 8 carry the six user stories in priority order.
Phase 9 carries the coverage proof, the process gates and the
pull-request record. Every task names exact file paths, and every
bound from data-model.md, contracts/ and research.md is quoted so
nothing is left to implementation-time discretion.

**Check-item rule**: a group B check closes with the file:line
evidence and the commit hash recorded in the results record
(`specs/032-fix-merged-features/results/record.md`) and in the
pull-request comment. A check the master tree does not hold reopens as
a fix under the story that owns the file, with its test written first
(FR-055).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies); the `add_test` registration in `test/CMakeLists.txt` is the one shared edit among the test tasks, serialized by appending in task-id order
- **[Story]**: User story ownership (US1 to US6 from spec.md)
- Setup and Polish tasks carry no story label

## Phase 1: Setup (Run Reading and Version Reservation)

**Purpose**: The CI run the figures stand on, the version lane, and
the fact checks SC-007 records. No behavior arrives in this phase.

**CRITICAL**: T001 precedes every check item: FR-064 binds the full
run read before the work starts.

- [x] T001 Request a `gh` token for the session, read the full run `40438223916` job by job, and record each job's state in `specs/032-fix-merged-features/results/record.md` beside the request's standing record (PC-11, FR-064); each check item of Phase 2 confirms against this run before it closes
- [x] T002 Record the version reservation in the task note and carry it into the first commit that needs it: `CMakeLists.txt:15` holds `0.20.0`, `:298` holds harness `SOVERSION 12`, `:80` holds archive `SOVERSION 4`; with no public-header edit the branch moves to `0.20.1`, with the `SkippedKind` value of T036 it moves to `0.21.0` with harness `SOVERSION 13`, and the ABI check is recorded beside the move in that commit (FR-001, FR-002); the versions agree in each file the merge touched (FR-004); the archive `SOVERSION` stays `4` in every case (D-11); the report schema stays `1.5` (`json_report.cpp:36`)
- [x] T003 [P] Record the option-lane confirmation in `specs/032-fix-merged-features/results/record.md`: `source/harness/detail/options.hpp` adds no row and moves no value; the lanes hold `260` to `269` the jitter-probe family, `270` to `279` the hunter family, `290` to `319` the command-line family (FR-003)
- [x] T004 [P] Record the SC-007 fact checks in `specs/032-fix-merged-features/results/record.md` with the file:line evidence: `SPEEDGUN_JSON` matches no environment row (`options.hpp:109` to `:115` derives the name from a flagged row and no row is named `json`; the unmatched-name rule at `options.cpp:1310` to `:1325` leaves it unread), and the idle preset exits 2 with one line when no cpu is free (`cli.cpp:805` to `:818`)

**Checkpoint**: The run is read, the lane is recorded, and the
standing facts carry their evidence.

## Phase 2: Foundational (Group B Check Items)

**Purpose**: Confirm master holds each fix the earlier lanes landed.
A check that does not hold reopens a fix under the story that owns the
file; the confirmations gate the stories because a reopened fix moves
its story's ground.

- [x] T005 [P] [US1] Confirm the shadow rename stands and the build of each CI preset passes: `git log --oneline -1 7d8dfdd` resolves, `cmake --preset=ci-rocky` and `cmake --build build` succeed, and the evidence goes to `results/record.md` (FR-005, PC-1)
- [x] T006 [P] [US1] Confirm the mutation test anchors at schema `1.5`: `658af81` resolves, the mutation test reads the version once, and no `1.4` figure remains in the test file that owns the anchor (FR-006, PC-2)
- [x] T007 [P] [US1] Confirm the hunt-block rows and the CLI table test stand: `1c47e66` and `d7e22c2` resolve, `harness_cli_table_test.cpp` passes, and `clang-format 18.1.8` reports no change over `test/source` (FR-007, PC-3)
- [x] T008 [P] [US1] Confirm the trim stands: `cd2f2f1` and `f507b8f` resolve, `README.md` holds 835 lines, `PAGE_LINE_CAP = 835` stands once at `tools/docs/example_drift.py:192`, and the drift gate passes (FR-008, PC-4)
- [x] T009 [P] [US1] Confirm the affinity-cpu scenario stands: `502d42b` resolves, the owning test passes under `taskset -c 0,1`, `taskset -c 0-3` and the full mask, no `SKIP_RETURN_CODE` stands on the entry, and `test/tier_labels.sh` holds master's eight rules (FR-009, PC-5)
- [x] T010 [P] [US1] Confirm the lint job of run `40438223916` passed with the spell step included, on the full run T001 read, and record that the spell gate needs no CI change (FR-010)
- [x] T011 [P] [US1] Confirm each of the twelve reverts of PC-6 is on master by `git log --grep` on its reverted hash (the twelve pairs stand in spec.md PC-6) and record each found commit in `results/record.md`; the suite passes on master as it stands (FR-011)
- [x] T012 [P] [US1] Confirm the `setenv|GCOV_PREFIX|sgcov` scan over `test/source` returns nothing and no test calls `setenv` (FR-012; research R-07 measured it clean 2026-10-27)
- [x] T013 [P] [US1] Confirm the rank mark stands outside the macro: `42ba692` resolves, the helper stands at `source/harness/hunt_plan.cpp:97`, no lambda stands inside an `SG_*` macro in `source/` or `include/`, and the coverage target reports no `inconsistent` error (FR-013, PC-7)
- [x] T014 [US1] Confirm the online-set seam stands as research R-03 measures (`topology.cpp:327`, the readers at `execution_context.cpp:655` and `jitter_probe.cpp:711`), that tests install a set with a gap and check both bounds, and that the probe-cpu affinity-mask rule at `specs/027-jitter-probe/contracts/cli-options.md:71` stays its own rule; the gap test and the fallback fix themselves belong to US2 (T019 to T021) (FR-014)
- [x] T015 [P] [US1] Confirm the duplicate-cpu requirement stands with its three records: the spec row at `specs/027-jitter-probe/spec.md:598` to `:601`, the grammar row at `contracts/cli-options.md:28`, and the error-table row at `:79` (FR-015)
- [x] T016 [P] [US1] Confirm no hunt row carries `fromEnvironment`, with the option table as the evidence (FR-016)

**Checkpoint**: Every check item carries its evidence, and every
reopened fix has its task in the story phases below.

## Phase 3: User Story 2 - One precedence for the probe value (Priority: P1)

**Goal**: One rule decides which `--jitter-probe` value runs,
regardless of the source, and a family's own probe wins over both the
option file and the command line.

**Independent Test**: three runs that differ only in the source of the
probe value produce the same probe placement, and a family probe
overrides both other sources (spec.md, User Story 2).

### Tests for User Story 2 (written first, seen failing, same commit as the fix, FR-055)

- [x] T017 [US2] Write the failing precedence test in `test/source/harness_jitter_cli_test.cpp`: an option file that carries `jitter-probe=` stands as a command-line value would; a family's own probe wins over an option-file value; the command-line value stands when the command line and the option file both carry one (D-01, FR-028); observe the red run that shows an option-file value overriding a family's own probe today (PC-10)
- [x] T018 [US2] Write the failing duplicate-cpu tests in `test/source/harness_jitter_cli_test.cpp`: `--jitter-probe=0,0` and `--jitter-probe=0-1,1` each print one line quoting the value and exit 2 (D-04, FR-029); skip the shape the standing suite already covers
- [x] T019 [US2] Write the failing fallback tests through the seam in `test/source/harness_topology_test.cpp` and the owning jitter test: install an online set with a gap, for example `0-3` and `8-11`, make the online line unreadable, and check both bounds (`execution_context.cpp:655`, `jitter_probe.cpp:711`) take membership from the gap; no count fills the set (D-02, D-03, FR-021); observe the red run that shows the count fallback (`topology.cpp:358` to `:365`) answering indexes the mask does not carry

### Implementation for User Story 2

- [x] T020 [US2] Fix the precedence in `source/harness/options.cpp` and the family settlement it feeds: a family's own probe wins over both a command-line value and an option-file value, an option-file value carries the same standing as a command-line value, and the command-line value stands when both carry one (D-01, T017); ship with T017 in one commit
- [x] T021 [US2] Fix the unreadable-line fallback of `onlineCpuSet()` in `source/harness/topology.cpp` to answer the process affinity mask instead of filling by index from `onlineCpuCount()`, update the three comments research R-07 names (`internal.hpp:742`, `:755`, `topology.cpp:343`) to the mask wording, and ship with T019 in one commit (D-02, FR-021)
- [x] T022 [US2] Add the duplicate-cpu refusal for the range shape `0-1,1` beside the standing list-shape refusal in the parse path `source/harness/jitter_probe.cpp` owns, one line quoting the value, exit 2, and ship with T018 in one commit (D-04, FR-029)
- [x] T023 [US2] Make the scan step and the settle step of `--jitter-probe` run one code path in `source/harness/jitter_probe.cpp`, keep the behavior, and give each behavior that stays a test in the executable T017 used (M-7, FR-019)
- [x] T024 [US2] Root-cause and fix the family-probe intermittent failure: no retry, no sleep, no timeout and no debug print joins; remove the `e781eda` print in the fix commit and ship the fix with its test in one commit (M-12, FR-022)
- [x] T025 [US2] Cover the batch-probe arm reachable through a family path with tests, delete code no input can reach, and record the reason in the commit body: the arm runs only while `familyProbes` is empty (`cli.cpp:776` to `:777`) (PR 46 P-3, FR-027)
- [x] T026 [US2] After T021 lands, take the coverage trace on the branch head and list in `results/record.md` each `--omit-lines` pattern of `cmake/coverage.cmake:92` to `:98` and each allowance row of `tools/dbc/coverage_gate.sh:60` that no longer matches code, with the trace and the zero list as evidence; ask before any removal (D-12, FR-026)

**Checkpoint**: The three sources agree, the fallback answers the
mask, and the keep-set question carries its evidence.

## Phase 4: User Story 3 - A hunt refuses ambient interference and pairs with the probe (Priority: P2)

**Goal**: An ambient value stops a hunt with one line naming the
source, and the hunt and the jitter probe run together.

**Independent Test**: a run with one ambient variable set exits 2 with
one quoted line, and a hunt with the probe completes and reports both
records (spec.md, User Story 3).

### Tests for User Story 3 (written first, seen failing, same commit as the fix, FR-055)

- [x] T027 [US3] Write the failing ambient-value tests in `test/source/harness_hunt_cli_test.cpp`, one per variable: `SPEEDGUN_COUNTER`, `SPEEDGUN_METRIC`, `SPEEDGUN_PLACEMENT` each stop a hunt with exit 2 and one quoted line that names the source; the child receives the variable through its environment array, not `setenv` (D-07, FR-034, FR-056)
- [x] T028 [US3] Write the failing combination test in `test/source/harness_hunt_cli_test.cpp` and the report check in `test/source/harness_hunt_json_test.cpp`: `--hunt` with `--jitter-probe` completes and the report carries both the hunt and the jitter records (D-08, FR-035)

### Implementation for User Story 3

- [x] T029 [US3] Make the ambient-value block stand in the hunt parse path `source/harness/hunt_cli.cpp` and `source/harness/options.cpp` own: the run stops on the first ambient variable it reads and the line names that source; more than one stops on the first (D-07, Edge Cases); ship with T027 in one commit
- [x] T030 [US3] Support `--hunt` with `--jitter-probe` end to end, add the docs row to `docs/pages/harness.md`, and ship with T028 in one commit (D-08, FR-035)
- [x] T031 [P] [US3] Confirm the hunt's first pass carries no retry, against master, and record the evidence in `results/record.md` (D-10, FR-037, PC-9)
- [x] T032 [P] [US3] Record the seam-publicity evidence in `results/record.md`: the `FakeProvider` open-failure seam stays public in the counters archive and the archive `SOVERSION` stays `4` (D-11, FR-038)
- [x] T033 [P] [US3] Confirm internal flag parameters use options structures and contract-only helpers carry `[[maybe_unused]]`, at the sites PR 47 P-5 and P-8 name, and record the evidence (FR-039, FR-040)
- [x] T034 [US3] Record the hunter acceptance run's move to spec 031 in `results/record.md` and close the item: the per-CPU-type hunter acceptance rows the PR 47 prompt asked for belong to the host-validation feature (PR 47 P-9, FR-041)
- [x] T035 [US3] Confirm each PR 47 item P-10 to P-18 against master and close it with evidence in `results/record.md`; an item the master tree does not settle goes to the pull request as a question and stops on its own commit (FR-042)
- [x] T036 [US3] Add a `SkippedKind` value in `include/speedgun-ng/hunt.hpp:158` only when an item above needs one, move the version to `0.21.0` and the harness `SOVERSION` to `13` in the same commit with the ABI check recorded, join the `report.cpp availabilityName` table, and take the reservation of T002 (D-09, FR-036, FR-001)
- [x] T037 [P] [US3] Confirm the hunt parsers' process exit against master and close with evidence or fix with a test in `test/source/harness_hunt_cli_test.cpp` (review B 10, FR-043)
- [x] T038 [US3] Give `--hunt=none` and the source order rule their tests in `test/source/harness_hunt_cli_test.cpp`, written first and seen failing where a gap stands (review B 11, FR-044)
- [x] T039 [US3] Give the refused-combination rule its environment and option-file tests in `test/source/harness_hunt_cli_test.cpp`: a refused combination is refused from every source (review B 12, FR-045)
- [x] T040 [P] [US3] Give the hunt with `--interleave`, `--threads` and `--jitter-probe` its tests (review B 13, FR-046)
- [x] T041 [P] [US3] Give the raw-code rule with globs its test in `test/source/harness_hunt_cli_test.cpp` (review B 18, FR-047)
- [x] T042 [P] [US3] Give the `ABSENT` relabel abort its test (review B 20, FR-048)
- [x] T043 [P] [US3] Close the real-PMU test item against the host's catalog (`source/harness/catalog.cpp` and the pmu events it embeds), or name the gap in the pull request (review B 21, FR-049)
- [x] T044 [P] [US3] Give the `--hunt-noise` grammar its tests (review B 22, FR-050)
- [x] T045 [P] [US3] Give a failing device lookup its test in `test/source/harness_hunt_cli_test.cpp` (review B 23, FR-051)
- [x] T046 [US3] Extend the counter refusals to every source with tests in `test/source/harness_hunt_cli_test.cpp` and the counters tests: an unresolvable `--counter` address exits 2 from every source, including a valid PMU path this host lacks, and an uncountable leaf exits 2 (D-05, D-06, FR-032, FR-033)

**Checkpoint**: The hunt refuses ambient interference from every
source, pairs with the probe, and each PR 47 item carries its evidence
or its question.

## Phase 5: User Story 4 - The compare table quotes user text (Priority: P2)

**Goal**: The name cell and the rule-text cell print quoted, so a name
with a separator cannot break the table.

**Independent Test**: a compare run whose baseline carries a name with
a quote or a separator prints the cell quoted, and the reason cell
stays unquoted (spec.md, User Story 4).

### Tests and implementation for User Story 4 (test first, same commit, FR-055)

- [x] T047 [US4] Write the failing quoting tests in `test/source/harness_compare_test.cpp`: a baseline name that carries a quote character prints the name cell quoted once, and a rule text with the same shape prints the rule-text cell quoted once with the reason cell unchanged (User Story 4, FR-030)
- [x] T048 [US4] Quote the name cell and the rule-text cell with `detail::quotedText`, once, at the print site `source/harness/compare.cpp:1906` to `:1907`; the reason cell stays unquoted; ship with T047 in one commit (FR-030)

**Checkpoint**: The table stays parseable for every row that follows
a quoted cell.

## Phase 6: User Story 5 - The docs agree with the code (Priority: P3)

**Goal**: Each option, key and exit status in the three features' docs
matches the code, the hunt samples name the current version, and the
schema carries one key order.

**Independent Test**: the docs sweep reports zero disagreements, and
the two hunt samples carry the current version (spec.md, User Story
5).

### Tests for User Story 5 (written first, seen failing, same commit as the fix, FR-055)

- [x] T049 [US5] Write the failing schema tests in `test/source/harness_json_schema_test.cpp`: a document with a duplicate top-level key fails, and the one top-level `x-order` key equals `["schemaVersion", "speedgun", "run", "jitter", "benchmarks", "hunt"]` (FR-017; the two keys at `docs/schema/report-v1.json:7` and `:8` stand today, research R-01)
- [x] T050 [P] [US5] Write the failing writer-order check in `test/source/harness_json_report_test.cpp`: the writer emits `jitter` (`json_report.cpp:590`) before `benchmarks` and before `hunt` (`:676`), matching the `x-order` key of T049 (FR-017)

### Implementation for User Story 5

- [x] T051 [US5] Reduce `docs/schema/report-v1.json:7` and `:8` to one top-level `x-order` key, `["schemaVersion", "speedgun", "run", "jitter", "benchmarks", "hunt"]`, and ship with T049 and T050 in one commit (FR-017)
- [x] T052 [US5] Re-record the two hunt samples from the current build: `docs/pages/harness.md:1509` (the block around `:1520`) and `example/docs/hunt_rank.expected:2`, both carrying `speedgun-ng 0.20.0 build=Release` or the branch version, every other sample unchanged, so the version figures agree in each file the merge touched; update the page regions and pass the drift gate in the same commits (M-6, FR-018, FR-004, research R-09)
- [x] T053 [US5] Run the docs sweep over the three features' docs against the code: each option, key and exit status checked, each disagreement carrying its fix or its pull-request question; record zero disagreements or the list in `results/record.md` (M-6, FR-018, SC-008)
- [x] T054 [US5] Reproduce the `loop_shape` failure under GCC 14 with the Rocky toolchain, name the cause in `results/record.md`, and land the fix in the code or in the test `test/loop_shape.sh` owns; no compiler check joins; the `test` job at GCC 15.2.0 still passes (M-10, FR-020)
- [x] T055 [US5] Read each of the 38 conflict blocks the PR 47 rebase resolved by keeping both sides against master, stand each correct or fix it with its test, and name the eight files the merge commit `4f701ae` did not repair in `results/record.md` (M-15, FR-023)

**Checkpoint**: The docs, the schema and the samples agree with the
code, and the merge residue is read and settled.

## Phase 7: User Story 6 - The comments and the specs carry requirements only (Priority: P3)

**Goal**: No person's name, no acceptance wording and no host name
stands in the files the three PRs added or changed; the two coverage
comments say what the code does.

**Independent Test**: a scan over the files the diff from `fea69c5`
to `4f701ae` names finds no person's name and no acceptance
wording in the rewritten targets, and the two `cmake/coverage.cmake`
comments name no person (spec.md, User Story 6).

- [x] T056 [US6] Rewrite the named targets of Baseline B as requirement text: the requirement-language sites at `specs/028-full-command-line/spec.md:1127`, `specs/029-hunter-core/spec.md:62`, and `specs/029-hunter-core/plan-review-2.md:12`, `:39` and `:80`; each rewritten line states the requirement and names no person and no host (M-17, FR-024)
- [x] T057 [US6] Scan the files `git diff --name-only fea69c5 4f701ae` names for a person's name, acceptance wording and a host name; rewrite each hit as requirement text; `LICENSE`, `CODEOWNERS` and `CMakeLists.txt` stay untouched (PR 47 P-4, review B 24 and 25, FR-024)
- [x] T058 [US6] Rewrite the two comments at `cmake/coverage.cmake:57` and `:85` to `:86` as what the code does, with no name and no decision record; no other change in that file rides the commit (M-19, FR-025; the person and the `D-16 in specs/022-metric-formulas/plan.md` record stand there today, research R-05)

**Checkpoint**: The specs, the comments and the PR 46/47/49 trees
carry requirements only.

## Phase 8: User Story 1 - A push goes green end to end (Priority: P1, the acceptance story)

**Goal**: Every CI job passes on the head, and the coverage gate reads
100 percent for lines and for branches with only the accepted sides.

**Independent Test**: the CI run of the pushed head reports success on
every job, and the coverage gate reports 100.0 percent lines with the
branch shortfall holding only the D-13 list and the D-12 keep-set
(spec.md, User Story 1).

**Why this phase stands last**: US1 is the acceptance outcome the
other five stories deliver; its tasks run after their fixes land. The
check items of Phase 2 carry the [US1] label because they close the
reds the merge carried.

- [x] T059 [US1] Take the zero list from the compiler the coverage job uses on the branch head, name that compiler in `specs/032-fix-merged-features/spec.md` as FR-031 fixes, give each side a real input reaches a test, and list each side D-13 accepts with its `gcov` block and arc data; the gate change D-13 allows is the narrowest `lcov` option in `cmake/coverage.cmake` alone, and when no narrow filter removes exactly the proven sides the item stops and the data goes to the pull request as a question; the branch shortfall holds only this list and the D-12 keep-set (D-13, D-14, FR-031, SC-002; research R-10 names the runner and its default g++)
- [x] T060 [US1] Prove coverage reaches 100 percent including `detail/hunt_selection.hpp` and the glob star arm, through real inputs and tests, no code shaped to move a count (PR 47 P-3, D-14, FR-052, SC-002)
- [x] T061 [US1] Run `cmake --preset=ci-rocky`, `cmake --build build`, `ctest --test-dir build --output-on-failure`, then once `cmake --preset=ci-ubuntu` and `cmake --build build` with clang-tidy and cppcheck wired in, and record the results in `results/record.md`; every job on the head reports success (FR-005, SC-001, AGENTS.md build loop)
- [x] T062 [US1] Run the suite under `taskset -c 0,1` and record the pass in `results/record.md`; the standing CI covers the rest of SC-004 (SC-004)
- [x] T063 [US1] Run the docs drift gates (`example_drift`, `snippet_compile`, `example_run`, `report_shape`) and the drift gate of `tools/docs/example_drift.py`, and record the pass (FR-008, FR-018, FR-062)
- [x] T064 [US1] Confirm every figure in `results/record.md` comes from the CI run of the last pushed commit and every job log is read (FR-061)

**Checkpoint**: The branch head is green job by job, and the coverage
shortfall holds only the accepted sides.

## Phase 9: Polish & Cross-Cutting Concerns

**Purpose**: The process gates of group F and the pull-request record.

- [x] T065 [P] Check each new commit's footer reads exactly `Refs: specs/032-fix-merged-features` then `Approved-by: OmO`, in that order, with no blank line between them and no e-mail form anywhere in the footer, and record the audit in `results/record.md` (FR-053)
- [x] T066 [P] Check each new commit holds a subject of 50 characters or fewer, body lines of 72 or fewer, and a body that says only what the code does, and record the audit in `results/record.md` (PR 46 Q-3, FR-054)
- [x] T067 [P] Verify each fix shipped with its test, written first and seen failing, in the same commit, by walking the commit series in `results/record.md` against the test tasks above (FR-055)
- [x] T068 [P] Run the standing scans: no test calls `setenv` or `clearenv` and no child receives a variable except through its environment array (FR-056); no lambda stands inside an `SG_*` macro in `source/` or `include/` (FR-057); text from a user makes no contract fail (FR-058)
- [x] T069 [P] Audit the CI-file boundary: `.github/workflows/ci.yml`, `CMakePresets.json`, `cmake/coverage.cmake`, `cmake/lint.cmake`, `cmake/spell.cmake`, `tools/dbc/`, `tools/prose/`, `tools/docs/example_drift.py`, `.codespellrc` carry no change without approval, except the narrow `lcov` option D-13 proves and a missing revert, which stops and reports first (FR-059, SC-006)
- [x] T070 [P] Check each behavior change shipped with docs and an example, each README section holds 25 lines or fewer, and `README.md` holds 835 lines (D-16, FR-062)
- [x] T071 [P] Confirm each new commit passes `prose-lint` and the CI run shows the job green (M-13, FR-063, SC-005)
- [x] T072 Verify the branch rebased only, pushed once with `--force-with-lease`, and proved the push with `git ls-remote` (FR-060)
- [x] T073 Run the validation guide `quickstart.md` end to end and record the outcome (quickstart.md, SC-003)
- [x] T074 Write the one pull-request comment that lists each item with its commit or its question, the coverage numbers, and the CI run; it names no approval, no acceptance and no person, and every item of the request stands closed or asked (FR-065, SC-003)

**Checkpoint**: The gates hold, the record is complete, and the
branch is ready for the approved merge.

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies; T001 blocks every check item (FR-064)
- **Foundational (Phase 2)**: depends on T001; the check items are independent of each other and gate the stories they reopen into
- **US2 (Phase 3)**: depends on T014 (the seam confirmation); T026 depends on T021; T024 and T025 depend on T017's harness only
- **US3 (Phase 4)**: independent of US2 except T040 (the hunt with `--jitter-probe` rides the combination of T030) and T046 (the counters refusals stand beside T029's parse path); T036 depends on no other task until an item needs the value
- **US4 (Phase 5)**: independent; `compare.cpp` is US4's alone
- **US5 (Phase 6)**: T051 depends on T049 and T050; T052 and T053 depend on the version the branch settles (T002, T036); T055 is independent
- **US6 (Phase 7)**: independent of Phases 3 to 6; T058 touches `cmake/coverage.cmake`, so it serializes with T059's `lcov` option by commit order on that one file
- **US1 (Phase 8)**: depends on every story phase; T059 depends on T026 (the keep-set list) and T058; T060 depends on the hunt fixes of Phase 4
- **Polish (Phase 9)**: depends on all story phases; T074 last

### Within Each Story

- Tests written first, seen failing, then the fix, in the same commit (FR-055)
- The check items of a story precede its fixes, because a check that holds closes its item with evidence instead
- The docs row and the example ride the fix commit (FR-062)

### Parallel Opportunities

- Phase 2's check items T005 to T016 run in parallel: each records evidence in one file
- US2, US4 and US6 touch disjoint files (`options.cpp`/`topology.cpp`/`jitter_probe.cpp`, `compare.cpp`, the spec trees) and can run in parallel
- Within US3, T031 to T035 and T037 to T045 run in parallel; the `harness_hunt_cli_test.cpp` edits serialize by task-id order
- Phase 9's T065 to T071 run in parallel after the stories land

## Implementation Strategy

### MVP first (User Story 2)

1. Complete Phase 1 and Phase 2: the run is read, the checks carry evidence, and the reopened fixes are named
2. Complete Phase 3: the precedence, the fallback, the duplicate refusal, the one code path and the family-probe root cause land with their tests
3. **STOP and VALIDATE**: `ctest --test-dir build --output-on-failure` green, the three sources agree, the trace answers the mask

### Incremental delivery

1. Setup + Foundational → the ground is measured
2. US2 → the probe behaves by one rule
3. US4 → the compare table is safe (independent, can land before US3)
4. US3 → the hunt refuses interference and pairs with the probe
5. US5, US6 → the docs, the schema and the prose agree with the code
6. US1 → the gates run green end to end
7. Polish → the process gates and the pull-request record close the branch

### Commit discipline

One item, one commit, the files the item names and nothing else (the
spec's Principle X.3 note); rebase only; push once with
`--force-with-lease` and prove it with `git ls-remote` (FR-060).
