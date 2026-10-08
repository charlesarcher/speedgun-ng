---
description: "Task list for identifier naming camel case"
---

# Tasks: Identifier Naming CamelCase

**Input**: Design documents from
`/specs/014-identifier-naming-camelcase/`

**Prerequisites**: plan.md (required), spec.md (required for user
stories), research.md, data-model.md, contracts/, quickstart.md

**Tests**: Principle IX pairs each implementation task with a covering
check. FR-001 says a test changes identifier text alone, so a rename
task names the existing `ctest` command as its check. A new procedure
in the plan's test plan is its own task and names its command. The
plan does not record a red-first mode.

**Organization**: Tasks are grouped by user story. Execution order
follows research D-02, so the rename story phase precedes the law
story phase. Both stories are P1. The reason is in Dependencies.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3, US4)
- The checkbox line carries a repository path

## Path Conventions

Single project. Paths are repository-root relative.

- Shipped headers: `include/speedgun-ng/`
- Sources: `source/counters/`, `source/simulation/`
- Tests: `test/source/`, `test/compile-fail/`, `test/CMakeLists.txt`
- Gates and version: `.clang-tidy`, `CMakeLists.txt`,
  `cmake/install-rules.cmake`
- Documents: `docs/pages/`, `README.md`, `AGENTS.md`
- This feature: `specs/014-identifier-naming-camelcase/`

### Reading order for every task

1. `specs/014-identifier-naming-camelcase/spec.md` for the requirement
2. `specs/014-identifier-naming-camelcase/research.md` for the decision,
   cited by its id in D-01 through D-08
3. `specs/014-identifier-naming-camelcase/contracts/` for the obligation
4. `specs/014-identifier-naming-camelcase/plan.md` for the verification
   row
5. `specs/014-identifier-naming-camelcase/quickstart.md` for the command

### Rules that bind every task

- One commit per task, or the closing-commit group the Dependencies
  section names. The commit title is `<Section>: <imperative>` at 50
  characters or fewer, with a why-body at 72 columns, `Approved-by:`,
  and `Refs: specs/014-identifier-naming-camelcase`.
- Every changed line traces to a requirement (Principle X.3).
- Identifiers inside `external/` stay on the exception list. Vendored
  names stay on the exception list (spec.md Exceptions).
- A `file:line` citation is an audit-point anchor. After T001, find
  the site by the named token when the line has moved.
- File names, CMake names, namespaces, and the package name keep their
  spelling (N-9, FR-016).
- Verify a rename commit with
  `cmake --build --preset=dev && ctest --preset=dev --output-on-failure`
  and the format check. A failure stops the sequence (FR-007).
- A task closed only against the dev preset stays open. The feature
  builds once in the release preset before its tasks are called done
  (Principle IX). That build is T030.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Produce the gate baseline D-06 requires before any rename
commit opens.

- [X] T001 Record the 21 format-check paths from CI run 37553123469 `specs/014-identifier-naming-camelcase/plan.md`
  in `specs/014-identifier-naming-camelcase/plan.md`, repair the
  format of those files, and leave every identifier at its
  audit-point spelling. This task is the format repair. A rename
  commit does not absorb it (D-06, FR-021).
- [X] T002 After that repair is on the default branch and CI is green, `specs/014-identifier-naming-camelcase/plan.md`
  record the green head SHA, the per-translation-unit name-check
  finding counts, and the sampling figures in
  `specs/014-identifier-naming-camelcase/plan.md`. The comparison
  head for FR-001, FR-002, and FR-003 is that SHA. Commit
  `6aafd2dc8a1310ee335820ad8dc4dedd0eb0ae17` is an earlier green run
  (D-06, D-07, FR-021).

**Checkpoint**: plan.md holds the baseline record. No rename commit
exists yet.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Fix the rename command every later commit uses.

**Critical**: No rename commit opens before T001 and T002 are done.

- [X] T003 Record the D-01 command in `specs/014-identifier-naming-camelcase/quickstart.md`
  `specs/014-identifier-naming-camelcase/quickstart.md`: clang-tidy
  23.1.1 with a temporary copy of the new naming options, export
  fixes, `clang-apply-replacements` 23.1.1, then `clang-format` on
  the files that commit touches. A missed name is a hand edit in
  that same commit. The live `.clang-tidy` file stays unchanged
  until the closing commit (D-01, D-02, FR-013).

**Checkpoint**: The command is written. User-story commits can start.

---

## Phase 3: User Story 2 - Rename without a behavior change (Priority: P1)

**Goal**: Every identifier the project owns uses the new spelling.
Counter readings, catalog names, messages, and machine code stay the same
after symbol names are normalized.

**Independent Test**: The test set at the gate baseline passes at the
rename head. The normalized machine-code comparison reports no
difference. The name check reports zero findings. The name-check half
runs after the closing commit, because D-02 keeps
`WarningsAsErrors` empty until that commit.

### Implementation for User Story 2

Each task runs the T003 command on one include-graph family and
updates that family's call sites in the same commit (D-02, FR-007,
FR-013). Umbrella headers `include/speedgun-ng/counters.hpp` and
`include/speedgun-ng/speedgun-ng.hpp` ride the commit of a header
they include when a name in that header changes.

- [X] T004 [US2] Rename owned identifiers in `include/speedgun-ng/dbc.hpp`,
  `tools/dbc/macros.yaml`, and the compile-fail tests under
  `test/compile-fail/` that name dbc identifiers. Add
  `test/source/dbc_literal_fixture.cpp`, register it in
  `test/CMakeLists.txt`, and confirm the fixture literal keeps its
  text after `clang-apply-replacements`. Remove the five naming
  suppressions FR-019 lists. List any suppression that remains, with
  its exception entry, in `spec.md` (D-01, D-02, FR-016, FR-019).
- [X] T005 [US2] Rename owned identifiers in `include/speedgun-ng/counters_core.hpp`
  `include/speedgun-ng/counters_core.hpp` and every call site of a
  name it declares, including the qualification at
  `include/speedgun-ng/counters_core.hpp:289`. The type spelling
  becomes `Availability`. The member spelling `availability` stays
  (FR-018, D-02).
- [X] T006 [US2] Rename owned identifiers in `include/speedgun-ng/counters_provider.hpp`
  `include/speedgun-ng/counters_provider.hpp` and the remaining call
  sites of names it declares (D-02, FR-013).
- [X] T007 [US2] Rename owned identifiers in `include/speedgun-ng/counters_measurement.hpp`
  `include/speedgun-ng/counters_measurement.hpp` and the remaining
  call sites, including the qualification at
  `include/speedgun-ng/counters_measurement.hpp:427`. The member
  spelling `availability` stays (FR-018, D-02).
- [X] T008 [US2] Rename owned identifiers in `include/speedgun-ng/counters_clock.hpp`
  `include/speedgun-ng/counters_clock.hpp` and
  `source/counters/clock_provider.cpp` in one commit (D-02).
- [X] T009 [US2] Rename owned identifiers in `include/speedgun-ng/counters_pmu.hpp`
  `include/speedgun-ng/counters_pmu.hpp` and the call sites in
  `source/counters/linux_pmu/provider.cpp` (D-02, FR-013).
- [X] T010 [US2] Rename owned identifiers in `include/speedgun-ng/counters_push.hpp`
  `include/speedgun-ng/counters_push.hpp` and
  `source/counters/push_provider.cpp` in one commit (D-02).
- [X] T011 [US2] Rename owned identifiers in `include/speedgun-ng/counters_fake.hpp`
  `include/speedgun-ng/counters_fake.hpp` and
  `source/counters/fake_provider.cpp` in one commit (D-02).
- [X] T012 [US2] Rename owned identifiers in `include/speedgun-ng/counters_system.hpp`
  `include/speedgun-ng/counters_system.hpp` and
  `source/counters/system.cpp` in one commit (D-02).
- [X] T013 [US2] Rename owned identifiers in `include/speedgun-ng/simulation.hpp`
  `include/speedgun-ng/simulation.hpp` and
  `source/simulation/marker.cpp` in one commit (D-02).
- [X] T014 [US2] Rename file-local helpers that no header declares in `source/counters/linux_pmu/`
  `source/counters/linux_pmu/` (`encode.cpp`, `fast_read.cpp`,
  `group_io.cpp`, `table_parse.cpp`, `embedded_tables.hpp`)
  (D-02, FR-013).
- [X] T015 [US2] Rename file-local helpers in `source/counters/fold.cpp`
  and `source/counters/plan.cpp` (D-02, FR-013).
- [X] T016 [US2] Sweep `tools/`, `test/`, `example/`, and
  `.github/workflows/ci.yml` for an FR-017 match an earlier commit
  left unchanged. The output strings `consumer: pmu catalog entries`
  and `embedded:` stay. The namespace token in
  `tools/dbc/asm_smoke.sh` stays (FR-016, FR-017).

**Checkpoint**: Each commit in T004 through T016 builds, passes
`ctest --preset=dev`, and passes the format check. String literals,
catalog names, object paths, provenance strings, unit tokens,
read-mode labels, and error messages keep their text (FR-016).

---

## Phase 4: User Story 1 - One naming law (Priority: P1)

**Goal**: The constitution holds every naming rule, the constant
spelling, and the exception list. A configuration file, a
documentation page, and a spec hold no naming rule on their own
authority.

**Independent Test**: A reviewer compares the constitution text with
the rule list and the exception list in this specification. Each rule
appears. Each exception appears. Each name-check setting matches the
rule it implements.

### Implementation for User Story 1

T017, T018, and T019 land in the closing commit with the User Story 3
and User Story 4 tasks. D-08 requires the amendment and the
enforcement flip in that same commit.

- [X] T017 [US1] Copy rules N-1 through N-11, the `kPascalCase` constant `specs/014-identifier-naming-camelcase/spec.md`
  spelling, and the exception list from
  `specs/014-identifier-naming-camelcase/spec.md` into
  `.specify/memory/constitution.md`. The data-model Id rule is
  "`N-1` through `N-11`, or `constants` for the `kPascalCase`
  spelling". Authority is the constitution after the 2.14.0
  amendment. Add the Sync Impact Report, the lineage row 2.14.0, the
  X.3 replacement, the Principle VIII gate item for
  `readability-identifier-naming`, and the Last Amended date
  (D-08, FR-009, FR-010, FR-011, FR-012). The amendment states that
  a later spec, plan, or suppression cannot create a deviation, and
  that a suppression names its exception entry.
- [X] T018 [US1] Set `.clang-tidy` naming keys to the constitution
  rules, using option names from `clang-tidy 23.1.1 --dump-config`
  (`CamelCase` for PascalCase, `camelBack` for lowerCamelCase).
  Set `HeaderFilterRegex`. Set `WarningsAsErrors` to
  `readability-identifier-naming` alone. List each tag object in
  `ConstexprVariableIgnoredRegexp` and cite N-11 on that entry
  (D-05, FR-004, FR-011).
- [X] T019 [US1] Make every naming sentence in `docs/pages/`,
  `README.md`, `AGENTS.md`, and any Spec Kit template that states
  a naming convention match the constitution text. A rule that exists
  only in `.clang-tidy`, a document, or this feature's specification
  fails FR-009 (FR-011).
- [X] T020 [US1] Plant one misnamed identifier in `include/speedgun-ng/dbc.hpp`
  `include/speedgun-ng/dbc.hpp`, run the name-check step, confirm
  the step fails, and remove the plant in the same working tree
  before the closing commit is finished (SC-011, FR-004).

**Checkpoint**: The constitution and `.clang-tidy` match. The plant
is gone. The name check reports zero findings on the closing commit.

---

## Phase 5: User Story 3 - A reader can migrate (Priority: P2)

**Goal**: A downstream reader sees project version 0.5.0, shared-object
version 2, and a rename map of every changed shipped-header spelling.

**Independent Test**: The version fields match the decision in FR-008.
A search finds an old shipped-header spelling only in the rename map
and the exception list.

### Implementation for User Story 3

These tasks join the closing commit named under User Story 1.

- [X] T021 [US3] Move the project version from 0.4.1 to 0.5.0 at `CMakeLists.txt:7`
  `CMakeLists.txt:7` and the shared-object version from 1 to 2 at
  `CMakeLists.txt:47`. Update the version notes at
  `include/speedgun-ng/counters_measurement.hpp:25` and
  `include/speedgun-ng/counters_measurement.hpp:31`. Record the
  0.5.0 entry in `specs/014-identifier-naming-camelcase/plan.md`.
  `SameMinorVersion` at `cmake/install-rules.cmake:39` stays.
  `CMakeLists.txt:137`, `:232`, `:384`, and `:558` stay
  (FR-008, D-02).
- [X] T022 [US3] Publish the rename map at `specs/014-identifier-naming-camelcase/rename-map.md`
  `specs/014-identifier-naming-camelcase/rename-map.md` and a copy
  with the same entries at `docs/pages/identifier-rename-map.md`.
  Each entry has four fields, quoted from
  `contracts/rename-map.md`: "Old spelling. New spelling. Shipped
  header that declares the name. Kind (type, function, enumerator,
  variable, member, constant, macro, or tag)." A fifth mark records
  a detail-namespace name. A detail name counts. A test-only name
  stays out of the map (FR-015).
- [X] T023 [US3] Add a successor-log entry in `specs/007-counters-and-timers/citations-log.md`
  `specs/007-counters-and-timers/citations-log.md` that points at
  `specs/014-identifier-naming-camelcase/rename-map.md` (FR-015).
- [X] T024 [US3] Confirm a package request for 0.4 rejects a 0.5 package in `cmake/install-rules.cmake`
  package because `SameMinorVersion` stays in
  `cmake/install-rules.cmake:39` (FR-008).
- [X] T025 [US3] Search `include/`, `source/`, `test/`,
  `example/`, `tools/`, `docs/`, `.github/workflows/`, `README.md`,
  and `AGENTS.md`. The search matches identifier tokens. It skips a
  string literal and the texts FR-016 keeps. An old shipped-header
  spelling appears only in
  `specs/014-identifier-naming-camelcase/rename-map.md`,
  `docs/pages/identifier-rename-map.md`, and the exception list
  (FR-015, SC-006).

**Checkpoint**: Version fields match FR-008. The map covers every
changed shipped-header spelling.

---

## Phase 6: User Story 4 - Stale sentences match the live rule (Priority: P3)

**Goal**: The live overhead page omits the removed probe name. The
follow-up specification states the hand-kept shared-object rule. The
successor log records that closed-directory edit.

**Independent Test**: `docs/pages/counters-overhead.md` line 299 omits
`pmu_probe_fast`. The follow-up specification states the hand-kept
rule. The successor log has an entry for the edit.

### Implementation for User Story 4

These edits join the closing commit. Other closed spec directories
stay unedited (FR-020).

- [X] T026 [P] [US4] Remove the `pmu_probe_fast` name from line 299 of `docs/pages/counters-overhead.md`
  `docs/pages/counters-overhead.md` (FR-020).
- [X] T027 [P] [US4] Edit `specs/013-counters-defect-followup/spec.md:891`
  `specs/013-counters-defect-followup/spec.md:891` so the answer
  calls the shared-object version a hand-kept number (FR-020).
- [X] T028 [P] [US4] Edit `specs/013-counters-defect-followup/checklists/requirements.md:44`
  `specs/013-counters-defect-followup/checklists/requirements.md:44`
  so that sentence matches the spec answer (FR-020).
- [X] T029 [US4] Record the closed-directory correction in `specs/007-counters-and-timers/citations-log.md`
  `specs/007-counters-and-timers/citations-log.md`, beside the
  section "Corrections after the 013 merge". Add that correction to
  the entry T023 created. Keep the entry pointing at
  `specs/014-identifier-naming-camelcase/rename-map.md`. Do not
  append a second entry (FR-015, FR-020).

**Checkpoint**: The two stale sentences match the live rule. One
successor-log entry records the map and the closed-directory edit.

---

## Phase 7: Polish and cross-cutting concerns

**Purpose**: Run the checks that apply to the rename head.

- [X] T030 Build the release preset once from `CMakeLists.txt` with
  `cmake --preset=ci-ubuntu` and `cmake --build build`
  (Principle IX, plan Test Plan step 3).
- [X] T031 Run the D-03 comparison after T030 and record it in `specs/014-identifier-naming-camelcase/plan.md`: build the release preset twice
  with `speedgun-ng_CONTRACTS=ignore`, normalize owned mangled
  symbols with `nm` and `llvm-cxxfilt`, strip the address column, and
  diff `objdump -d` output. Both tasks use `build/`. The permitted
  difference is the contract-predicate string
  FR-002 names. Record the result in
  `specs/014-identifier-naming-camelcase/plan.md` (FR-002, SC-002).
- [X] T032 [P] Run the D-04 macro-collision check and record it in `specs/014-identifier-naming-camelcase/plan.md` with
  `clang++ -dM -E -std=c++23` over a translation unit that includes
  every public header and the Linux headers a library unit includes.
  The checked set is every new enumerator and every new constant.
  `NONE`, `GAP`, `SYSCALL`, `CPU`, `THREAD`, `ABSENT`, `BYTES`, and
  `OPS` are in that set. Record the result in
  `specs/014-identifier-naming-camelcase/plan.md` (FR-006, SC-007).
- [X] T033 [P] Run the name check and record zero findings in `specs/014-identifier-naming-camelcase/plan.md`
  `specs/014-identifier-naming-camelcase/plan.md`. Confirm no other
  static-analysis count of a translation unit rose (FR-004, SC-004).
- [X] T034 [P] Run the downstream consumer job in `.github/workflows/ci.yml`. The output lines
  `consumer: pmu catalog entries` and `embedded:` stay. The job
  file is `.github/workflows/ci.yml` (FR-005, FR-017).
- [X] T035 [P] Re-measure the overhead figures on the reference host `docs/pages/counters-overhead.md`
  in `docs/pages/counters-overhead.md` (Linux 7.2.4-1-cachyos,
  Ryzen 9 9950X3D) and compare them with the gate-baseline figures.
  The noise bound is five percent, the bound the page gates at
  lines 406-411 (FR-003, D-07).
- [X] T036 Run every scenario in `specs/014-identifier-naming-camelcase/quickstart.md`
  `specs/014-identifier-naming-camelcase/quickstart.md` and record
  each result beside that scenario. Step 10 runs the gates FR-005
  names (FR-005, SC-008, plan Test Plan).

**Checkpoint**: Every hard gate the quickstart names has a recorded
result. The release preset has been built once.

---

## Dependencies and execution order

### Phase dependencies

- **Setup (Phase 1)**: No dependencies. T002 follows T001 because the
  baseline SHA is the green CI head of the format repair.
- **Foundational (Phase 2)**: Depends on Setup. Blocks every rename
  commit.
- **User Story 2 (Phase 3)**: Depends on Foundational. T004 through
  T016 run in order. Each commit builds before the next opens (FR-007).
- **User Story 1, User Story 3, and User Story 4**: Depend on T016.
  They share one closing commit (D-02 item 10, D-08). The amendment
  and the `WarningsAsErrors` flip land together, because a gate item
  whose check fails the build has to be enforceable on that commit.
- **Polish (Phase 7)**: Depends on the closing commit.

### User story dependencies

- **User Story 2 (P1)**: Starts after Foundational. It has no dependency
  on User Story 1. Its per-commit check is the dev build, `ctest`, and
  the format check.
- **User Story 1 (P1)**: Starts after T016. The name-check half of the
  User Story 2 independent test runs on this commit.
- **User Story 3 (P2)**: Starts after T016. Version fields and the map
  join the closing commit.
- **User Story 4 (P3)**: Starts after T016. T026, T027, and T028 touch
  different files and can be drafted in parallel. T029 edits
  `specs/007-counters-and-timers/citations-log.md` after T023, because
  both tasks append to that file.

### Within each user story

- The covering check is named in the task.
- A declaration and its call sites land in the same commit (D-02).
- The closing commit is complete before polish starts.

### Parallel opportunities

- T026, T027, and T028 touch different files.
- T032 through T035 touch different result records and can run after
  the closing commit. T031 follows T030 because both use `build/`.
- T004 through T016 are sequential. A later family includes an earlier
  header, so a parallel rename breaks FR-007.

---

## Parallel example: User Story 4

```bash
# Draft the three wording edits together, then append the log entry:
Task: "Edit docs/pages/counters-overhead.md line 299"
Task: "Edit specs/013-counters-defect-followup/spec.md:891"
Task: "Edit specs/013-counters-defect-followup/checklists/requirements.md:44"
```

---

## Implementation strategy

### MVP first (User Story 2, after the gate baseline)

1. Complete Phase 1: Setup.
2. Complete Phase 2: Foundational.
3. Complete Phase 3: User Story 2.
4. **Stop and validate**: each commit in T004 through T016 builds,
   passes `ctest --preset=dev`, and passes the format check.
5. The name-check half of the User Story 2 independent test waits for
   the closing commit. D-02 keeps the enforcement flip there so a
   mid-sequence spelling stays buildable.

User Story 1 is also P1. The law and the enforcement flip land after
the rename, in the closing commit.

### Incremental delivery

1. Setup plus Foundational produces the baseline and the command.
2. User Story 2 produces a buildable rename sequence.
3. The closing commit adds the law, the version break, the map, and
   the stale-sentence edits.
4. Polish records the head checks.

### Parallel team strategy

1. The team completes Setup and Foundational together.
2. One implementer walks T004 through T016 in order.
3. After T016, one implementer can draft T026, T027, and T028 while
   another drafts the constitution text and the rename map. Those
   drafts join one closing commit.

---

## Notes

- [P] tasks use different files and have no ordering dependency.
- [Story] labels map a task to a user story.
- The closing commit is the shared integration point for US1, US3, and
  US4. Splitting that commit leaves a head whose gate item fails.
- Commit after each rename task. Commit the closing group once.
- Stop at any checkpoint and validate that story's named check.

---

## Phase 8: Convergence

- [X] T037 Write the public-data-member naming rule into `.specify/memory/constitution.md` V.1, the rule `.clang-tidy:83` `PublicMemberCase: camelBack` already enforces, through the Governance procedure, per FR-009, FR-011, Constitution V.1 (contradicts) — CRITICAL
- [X] T038 Run the full FR-005 hard-gate set at the rename head `baca50d` and record the result beside Step 10 of `specs/014-identifier-naming-camelcase/quickstart.md`, per FR-005, SC-008, FR-021 (partial) — HIGH
- [X] T039 Re-measure the `docs/pages/counters-overhead.md` gated medians on the reference host until they sit within five percent of the gate-baseline figures, or record an explicit maintainer acceptance of the host-state deviation, per FR-003, SC-003 (partial) — HIGH
- [X] T040 Update the stale old-spelling mentions in comments and test prose (`source/counters/detail/pmu.hpp`, `source/counters/linux_pmu/provider.cpp`, `source/counters/linux_pmu/table_parse.cpp:394`, `test/source/counters_recorder_test.cpp:266`, `test/source/counters_pmu_test.cpp:421`) to the live spellings, keeping the `dbc_literal_fixture.cpp` literal and the `plan.cpp` catalog strings untouched, per FR-015, SC-006 (partial) — LOW
- [X] T041 Decide the fate of the untracked `.specify/integrations/generic.manifest.json`: commit it or add a `.gitignore` entry, per FR-016 scope (unrequested) — LOW

## Phase 9: Convergence

A second converge pass over the tree at the convergence head. The rename, the gate, the version fields, the constitution, and the records all verify clean; three residuals remain.

- [X] T042 Rename the surviving old-spelling tokens in the non-Linux branches: `detail::pmu_state` at `source/counters/linux_pmu/provider.cpp:48` to `detail::PmuState`, and `System::local().register_provider` at `test/source/counters_pmu_test.cpp:1151` to `registerProvider`; re-run the FR-015 search and confirm it comes clean, per FR-013, FR-015, SC-006 (partial) — HIGH
- [X] T043 Remove the qualification workaround at `include/speedgun-ng/counters_measurement.hpp:429`: drop the `::sg::counters::` qualification on the `Availability availability` member and correct the comment at 426-428, whose same-spelling collision the rename resolved, matching the unqualified sibling at `counters_core.hpp:285`, per FR-018 (partial) — MEDIUM
- [X] T044 Update the remaining old-spelling mentions in comments and docs prose: `include/speedgun-ng/speedgun-ng.hpp:7,24`, `include/speedgun-ng/counters_provider.hpp:71-74`, `include/speedgun-ng/counters_system.hpp:131`, `include/speedgun-ng/counters_fake.hpp:33-40,150,175`, `source/counters/detail/core.hpp:199`, `source/counters/detail/pmu.hpp:718`, `source/counters/linux_pmu/provider.cpp:818`, `tools/quill/quill_dependency_check.cpp:62`, `test/source/speedgun-ng_test.cpp:52`, `test/source/counters_tsc_test.cpp:13`, `test/source/counters_pmu_test.cpp:935`, `test/source/counters_linux_pmu_seam_test.cpp:2593,2637,2661`, and `docs/pages/counters-overhead.md:4,97,98,219,228`, per FR-015 and the T040 precedent (partial) — LOW

## Phase 10: Convergence

A third converge pass over the tree at head `59378c4`. The rename, the law, the version fields, the map, the FR-015 search, and the records all verify clean; three residuals remain.

- [X] T045 Run the FR-005 hard-gate set at the current convergence head (the three commits `5e770f4`, `bf8327c`, `59378c4` postdate the recorded run at `a734c6c` and the green CI run at `5bebe6d`, and two of them edit C++), and record the run beside Step 10 of `specs/014-identifier-naming-camelcase/quickstart.md`, per FR-005, SC-008 (partial) — HIGH
- [X] T046 Relabel the rename-map entries whose names are public data members (`frequency_hz`, `running_ratio`, and the six `RecorderHandle` members `m_capacity`, `m_columns`, `m_dropped`, `m_head`, `m_impl`, `m_wrapped`) to Kind `member` and Rule N-12 in both `specs/014-identifier-naming-camelcase/rename-map.md` and `docs/pages/identifier-rename-map.md`, the rule the 2.15.0 amendment wrote for that shape, per FR-011, FR-015, Constitution V.1 N-12 (partial) — LOW
- [X] T047 Record the FR-017 re-search at the rename head in `specs/014-identifier-naming-camelcase/plan.md`: list each gate, registry, script, and workflow hit found at the head with its disposition, including the updated `tools/dbc/macros.yaml`, `test/counters_tsc_read_shape.sh`, `test/consumer/` call sites, and the `sg::dbc::check[A-Z]` match in `.github/workflows/ci.yml`, and the staying tokens in `tools/dbc/asm_smoke.sh`, per FR-017 (partial) — LOW

## Phase 11: Convergence

A fourth converge pass over the tree at head `835481b` (CI run 37762327363 green). The rename, the law, the version fields, the map pair, the FR-015 search, the FR-017 record, and the gate records all verify clean; five residuals remain.

- [X] T048 Write the FR-012 obligation into constitution V.1 through the Governance procedure: a later spec, plan, local naming override, or suppression cannot create a deviation, a deviation requires a constitutional amendment, and a suppression names the V.2 exception entry it applies — the sentence T017 required the amendment to state and V.1 does not carry, per FR-009, FR-012, US1/AC1 (partial) — HIGH
- [X] T049 Run the second SC-011 plant the `contracts/naming-check.md` Proof names — one misnamed identifier in a `.cpp` under `source/`, confirm the name-check step fails, remove the plant — and record the result beside Step 6 of `specs/014-identifier-naming-camelcase/quickstart.md`; only the `dbc.hpp` plant is recorded today, per SC-011, FR-004 (partial) — HIGH
- [X] T050 Add concepts and type traits to the N-1 enumeration in constitution V.1, the shapes the spec's N-1 names and `contracts/naming-check.md` says remain PascalCase under N-1 while the constitution text omits them, per FR-009, FR-011 (partial) — MEDIUM
- [X] T051 Record CI run 37762327363 at head `835481b` (all eleven executed jobs success, docs skipped) beside Step 10 of `specs/014-identifier-naming-camelcase/quickstart.md`, the record the head advance since run 37760326237 leaves stale, per FR-005, SC-008 (partial) — LOW
- [X] T052 Drop `cyc` and `offset` from `ConstexprVariableIgnoredRegexp` in `.clang-tidy` or cite the V.2 entry that covers them: both name ordinary local variables in `source/counters/detail/pmu.hpp`, `source/counters/linux_pmu/fast_read.cpp`, and the seam test, not tag objects, so the entry is an exception outside the closed V.2 list, per Constitution V.2, FR-011, FR-012 (contradicts) — LOW

## Phase 12: Convergence

A fifth converge pass over the tree at head `075054f`. The law at 2.16.0, the `.clang-tidy` keys, the version fields, the map pair, the FR-015 search, the FR-017 record, the suppressions, the qualification, and the prose lint at the head all verify clean; one residual remains.

- [X] T053 Push the five commits `786eca9`, `0c2fe72`, `10ee6c9`, `b5f3a6f`, and `075054f` to `origin/014-identifier-naming-camelcase`, let the CI run at the new head conclude every hard gate, and record that run beside Step 10 of `specs/014-identifier-naming-camelcase/quickstart.md` — the record names run `37762327363` at `835481b`, and no CI evidence exists for the current head, per FR-005, SC-008 (partial) — MEDIUM

## Phase 13: Convergence

A sixth converge pass over the tree at head `ac10efc`. The law at 2.16.0, the `.clang-tidy` keys, the version fields, the map pair, the FR-015 search, the suppressions, the qualification, and the single citations-log entry all verify clean, and the head's own CI run is green; one record residual remains.

- [X] T054 Record CI run `37769547155` at head `ac10efc` beside Step 10 of `specs/014-identifier-naming-camelcase/quickstart.md` — all eleven executed jobs success on attempt 2, the docs job skipped, the coverage job rerun after its attempt-1 failure at `counters/system.cpp` 866 of 867 branches, the machine-root gcov attribution the step's record already names as host-topology flaky, the local gate at the head reading 867 of 867 — the record the head advance past `eb1f444` leaves stale, per FR-005, SC-008 (partial) — LOW

## Phase 14: Convergence

A seventh converge pass over the tree at head `f8b8d0c`. The law at 2.16.0, the `.clang-tidy` keys, the version fields, the map pair, the FR-015 search, the FR-017 record, the suppressions, and the qualification all verify clean, and the local coverage gate reads 867 of 867 branches at GCC 16.2.1; one residual remains.

- [X] T055 Fix the coverage exclusion the head's CI run exposes: CI run `37773644494` at `f8b8d0c` concluded failure because the coverage job read 866 of 867 branches at `counters/system.cpp`, and the run's `coverage-info` artifact pins the uncovered branch to line 585, the false leg of the `Object::children` direct-child ternary, which sits outside the T066 `LCOV_EXCL_BR_START`/`LCOV_EXCL_BR_STOP` pair whose stop closes after the true leg at 581. Move the stop after the closing parenthesis of the ternary so the exclusion covers every branch of the host-dependent test, re-run the local coverage gate, and push so CI confirms the job at the new head, per FR-005, SC-008 (contradicts) — HIGH

## Phase 15: Convergence

An eighth converge pass over the tree at head `4f73163`. The law at 2.16.0, the `.clang-tidy` keys, the version fields, the map pair, the FR-015 search, the FR-017 record, the naming suppressions, the qualification, the closed-directory edits, the format check, and the coverage gate at 860 of 860 branches all verify clean, and CI run `37778739445` at the head concluded success across eleven executed jobs with the docs job skipped; two record residuals remain.

- [X] T056 Record the per-commit walk the rename sequence promises: Step 2 of `specs/014-identifier-naming-camelcase/quickstart.md` states one result for the closing commit, while SC-005 and the plan's FR-007 verification row name a range walk over each commit of the sequence. Walk the nine rename commits `f7591cc` through `fb7ba67`, running `cmake --build --preset=dev`, `ctest --preset=dev`, and the format check at each, and record each commit's result beside Step 2, per FR-007, SC-005, plan: FR-007/SC-005 verification row (partial) — MEDIUM
- [ ] T057 Record CI run `37778739445` at head `4f73163` beside Step 10 of `specs/014-identifier-naming-camelcase/quickstart.md` — eleven executed jobs success, the docs job skipped, the coverage job at 860 of 860 branches — the record the head advance past `1aa3793` leaves stale, per FR-005, SC-008 (partial) — LOW
