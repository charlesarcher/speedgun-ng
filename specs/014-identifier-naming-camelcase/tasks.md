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
- [ ] T007 [US2] Rename owned identifiers in `include/speedgun-ng/counters_measurement.hpp`
  `include/speedgun-ng/counters_measurement.hpp` and the remaining
  call sites, including the qualification at
  `include/speedgun-ng/counters_measurement.hpp:427`. The member
  spelling `availability` stays (FR-018, D-02).
- [ ] T008 [US2] Rename owned identifiers in `include/speedgun-ng/counters_clock.hpp`
  `include/speedgun-ng/counters_clock.hpp` and
  `source/counters/clock_provider.cpp` in one commit (D-02).
- [ ] T009 [US2] Rename owned identifiers in `include/speedgun-ng/counters_pmu.hpp`
  `include/speedgun-ng/counters_pmu.hpp` and the call sites in
  `source/counters/linux_pmu/provider.cpp` (D-02, FR-013).
- [ ] T010 [US2] Rename owned identifiers in `include/speedgun-ng/counters_push.hpp`
  `include/speedgun-ng/counters_push.hpp` and
  `source/counters/push_provider.cpp` in one commit (D-02).
- [ ] T011 [US2] Rename owned identifiers in `include/speedgun-ng/counters_fake.hpp`
  `include/speedgun-ng/counters_fake.hpp` and
  `source/counters/fake_provider.cpp` in one commit (D-02).
- [ ] T012 [US2] Rename owned identifiers in `include/speedgun-ng/counters_system.hpp`
  `include/speedgun-ng/counters_system.hpp` and
  `source/counters/system.cpp` in one commit (D-02).
- [ ] T013 [US2] Rename owned identifiers in `include/speedgun-ng/simulation.hpp`
  `include/speedgun-ng/simulation.hpp` and
  `source/simulation/marker.cpp` in one commit (D-02).
- [ ] T014 [US2] Rename file-local helpers that no header declares in `source/counters/linux_pmu/`
  `source/counters/linux_pmu/` (`encode.cpp`, `fast_read.cpp`,
  `group_io.cpp`, `table_parse.cpp`, `embedded_tables.hpp`)
  (D-02, FR-013).
- [ ] T015 [US2] Rename file-local helpers in `source/counters/fold.cpp`
  and `source/counters/plan.cpp` (D-02, FR-013).
- [ ] T016 [US2] Sweep `tools/`, `test/`, `example/`, and
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

- [ ] T017 [US1] Copy rules N-1 through N-11, the `kPascalCase` constant `specs/014-identifier-naming-camelcase/spec.md`
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
- [ ] T018 [US1] Set `.clang-tidy` naming keys to the constitution
  rules, using option names from `clang-tidy 23.1.1 --dump-config`
  (`CamelCase` for PascalCase, `camelBack` for lowerCamelCase).
  Set `HeaderFilterRegex`. Set `WarningsAsErrors` to
  `readability-identifier-naming` alone. List each tag object in
  `ConstexprVariableIgnoredRegexp` and cite N-11 on that entry
  (D-05, FR-004, FR-011).
- [ ] T019 [US1] Make every naming sentence in `docs/pages/`,
  `README.md`, `AGENTS.md`, and any Spec Kit template that states
  a naming convention match the constitution text. A rule that exists
  only in `.clang-tidy`, a document, or this feature's specification
  fails FR-009 (FR-011).
- [ ] T020 [US1] Plant one misnamed identifier in `include/speedgun-ng/dbc.hpp`
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

- [ ] T021 [US3] Move the project version from 0.4.1 to 0.5.0 at `CMakeLists.txt:7`
  `CMakeLists.txt:7` and the shared-object version from 1 to 2 at
  `CMakeLists.txt:47`. Update the version notes at
  `include/speedgun-ng/counters_measurement.hpp:25` and
  `include/speedgun-ng/counters_measurement.hpp:31`. Record the
  0.5.0 entry in `specs/014-identifier-naming-camelcase/plan.md`.
  `SameMinorVersion` at `cmake/install-rules.cmake:39` stays.
  `CMakeLists.txt:137`, `:232`, `:384`, and `:558` stay
  (FR-008, D-02).
- [ ] T022 [US3] Publish the rename map at `specs/014-identifier-naming-camelcase/rename-map.md`
  `specs/014-identifier-naming-camelcase/rename-map.md` and a copy
  with the same entries at `docs/pages/identifier-rename-map.md`.
  Each entry has four fields, quoted from
  `contracts/rename-map.md`: "Old spelling. New spelling. Shipped
  header that declares the name. Kind (type, function, enumerator,
  variable, member, constant, macro, or tag)." A fifth mark records
  a detail-namespace name. A detail name counts. A test-only name
  stays out of the map (FR-015).
- [ ] T023 [US3] Add a successor-log entry in `specs/007-counters-and-timers/citations-log.md`
  `specs/007-counters-and-timers/citations-log.md` that points at
  `specs/014-identifier-naming-camelcase/rename-map.md` (FR-015).
- [ ] T024 [US3] Confirm a package request for 0.4 rejects a 0.5 package in `cmake/install-rules.cmake`
  package because `SameMinorVersion` stays in
  `cmake/install-rules.cmake:39` (FR-008).
- [ ] T025 [US3] Search `include/`, `source/`, `test/`,
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

- [ ] T026 [P] [US4] Remove the `pmu_probe_fast` name from line 299 of `docs/pages/counters-overhead.md`
  `docs/pages/counters-overhead.md` (FR-020).
- [ ] T027 [P] [US4] Edit `specs/013-counters-defect-followup/spec.md:891`
  `specs/013-counters-defect-followup/spec.md:891` so the answer
  calls the shared-object version a hand-kept number (FR-020).
- [ ] T028 [P] [US4] Edit `specs/013-counters-defect-followup/checklists/requirements.md:44`
  `specs/013-counters-defect-followup/checklists/requirements.md:44`
  so that sentence matches the spec answer (FR-020).
- [ ] T029 [US4] Record the closed-directory correction in `specs/007-counters-and-timers/citations-log.md`
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

- [ ] T030 Build the release preset once from `CMakeLists.txt` with
  `cmake --preset=ci-ubuntu` and `cmake --build build`
  (Principle IX, plan Test Plan step 3).
- [ ] T031 Run the D-03 comparison after T030 and record it in `specs/014-identifier-naming-camelcase/plan.md`: build the release preset twice
  with `speedgun-ng_CONTRACTS=ignore`, normalize owned mangled
  symbols with `nm` and `llvm-cxxfilt`, strip the address column, and
  diff `objdump -d` output. Both tasks use `build/`. The permitted
  difference is the contract-predicate string
  FR-002 names. Record the result in
  `specs/014-identifier-naming-camelcase/plan.md` (FR-002, SC-002).
- [ ] T032 [P] Run the D-04 macro-collision check and record it in `specs/014-identifier-naming-camelcase/plan.md` with
  `clang++ -dM -E -std=c++23` over a translation unit that includes
  every public header and the Linux headers a library unit includes.
  The checked set is every new enumerator and every new constant.
  `NONE`, `GAP`, `SYSCALL`, `CPU`, `THREAD`, `ABSENT`, `BYTES`, and
  `OPS` are in that set. Record the result in
  `specs/014-identifier-naming-camelcase/plan.md` (FR-006, SC-007).
- [ ] T033 [P] Run the name check and record zero findings in `specs/014-identifier-naming-camelcase/plan.md`
  `specs/014-identifier-naming-camelcase/plan.md`. Confirm no other
  static-analysis count of a translation unit rose (FR-004, SC-004).
- [ ] T034 [P] Run the downstream consumer job in `.github/workflows/ci.yml`. The output lines
  `consumer: pmu catalog entries` and `embedded:` stay. The job
  file is `.github/workflows/ci.yml` (FR-005, FR-017).
- [ ] T035 [P] Re-measure the overhead figures on the reference host `docs/pages/counters-overhead.md`
  in `docs/pages/counters-overhead.md` (Linux 7.2.4-1-cachyos,
  Ryzen 9 9950X3D) and compare them with the gate-baseline figures.
  The noise bound is five percent, the bound the page gates at
  lines 406-411 (FR-003, D-07).
- [ ] T036 Run every scenario in `specs/014-identifier-naming-camelcase/quickstart.md`
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
