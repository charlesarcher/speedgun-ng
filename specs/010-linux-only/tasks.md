# Tasks: Linux as the Supported Platform

**Input**: Design documents from `specs/010-linux-only/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [contracts/platform-policy.md](contracts/platform-policy.md), [quickstart.md](quickstart.md), [checklists/requirements.md](checklists/requirements.md)

**Tests**: Included, and they take the form of audits. No CTest entry is added: this feature changes no code, so there is no target for one, and the plan records that decision. Every requirement is verified by a command in [quickstart.md](quickstart.md), and the audit that carries the most weight is FR-015's platform-token sweep with its four-bucket classification.

**Organization**: Tasks are grouped by user story so each story can be implemented and verified independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = a developer reads one true statement of what is supported; US2 = the gate set and its governance match the policy; US3 = a future port stays additive; US4 = the merged record stays intact
- Every task names its exact file path

## Path Conventions

Base commit for every diff in this feature is `fbdfc6f`, the `master` tip this branch forked from. Use `git diff fbdfc6f..HEAD` throughout.

Audit buckets, defined in [contracts/platform-policy.md](contracts/platform-policy.md) section C3: **live claim** changes, **preserved seam** stays byte-identical, **historical record** stays, **third-party tooling** is out of scope.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Capture the before state the audit is measured against, and clear the one machine-local blocker that makes every later verification fail for an unrelated reason.

- [ ] T001 Record the pre-change inventory in `specs/010-linux-only/tasks.md` under a `Phase 1 outcomes` heading, so SC-002's before-and-after counts have a committed baseline: 8 live-claim lines in 2 files, 45 preserved-seam lines across 8 files, and 116 historical lines under `specs/`. The eight live-claim lines are `cmake/ImportAutotoolsSubmodule.cmake` lines 68, 140, 141, 204, 205, 567, 568 and `.codespellrc` line 12, and `.codespellrc` line 12 is classified as a historical record under FR-016a, which leaves seven live-claim lines to change. Record the seam inventory verbatim from the Phase 0 sweep (research.md R-004) so a later pass can diff against it without re-running the sweep. Per Constitution X.4, a completion claim without recorded evidence is non-compliant

- [ ] T002 [P] Trim `CMakeUserPresets.json` so no local preset inherits a preset this feature deletes. That file is gitignored at `.gitignore:10` and carries `dev-darwin` inheriting `ci-darwin` and `dev-win64` inheriting `ci-win64`. CMake validates every preset in the resolved set before resolving any single one, so a dangling `inherits` fails the whole directory and takes `cmake --preset=dev` and `ctest --preset=dev` with it. Verify with `cmake --list-presets`, which must exit 0 (FR-018a; research.md R-002; quickstart section 1). Leave `dev-common`, `dev-linux`, `dev`, and `dev-coverage` in place: their whole chain survives. Do NOT commit this file; its fix carries zero tracked diff, and that is the point

**Checkpoint**: The baseline is on record and the local preset file resolves. Every later verification command can run.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The governance amendment. Nine required statements in one file, and every other surface in this feature points at them.

**⚠️ CRITICAL**: US1 must not land before this. A README that claims Linux is supported while the constitution still lists three platforms is a window where the project's two authorities disagree, and the window is the whole point of the change. US4's supersession statement is part of this edit, so that story cannot verify until it lands.

- [ ] T003 Amend `.specify/memory/constitution.md` to 2.11.0, MAJOR, in the four text sites the contract names plus the governance metadata. (a) Sync Impact Report: a new HTML comment at the top of the file, above the existing 2.10.0 report, which becomes a `Prior report` beside the 2.7.0 and 2.8.0 reports, which both stay (S8, FR-005, FR-006). (b) VIII's hard gate list: name Linux and enumerate no other platform, and replace the sentence reinstating the Windows gate when an upstream port lands, because a future specification adds a platform and landing an upstream port does not restore one (S1, S2, FR-001). (c) Additional Constraints: the Language clause's supported-platform definition names Linux alone, and the preserved warning-set clause drops the `/W4 /permissive-` on MSVC reference while keeping the GCC and Clang family (S3, S4, FR-002). (d) IX's per-feature release-build clause names `ci-ubuntu` and no other preset; the obligation of one release build per feature is unchanged (S5, FR-003). (e) Open deferrals: delete the macOS-runner entry whole, keeping the other two entries. That deletion is what retires `specs/009-vendor-quill` T038 by reference (S6, FR-004). (f) Version lineage table: add the 2.11.0 row with its rationale, set the footer to `**Version**: 2.11.0 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-10-02`, and leave every prior row including 2.7.0 and 2.8.0 (S7, FR-005). (g) In the Sync Impact Report, name `specs/003-vendor-hwloc`, `specs/004-vendor-simdjson`, `specs/005-vendor-hdrhistogram`, `specs/006-vendor-yaml-cpp`, and `specs/009-vendor-quill` as superseded on the platform question, and state the on-ramp by naming all seven deleted presets plus the note that every per-platform block is untouched (S9, FR-007, FR-008). Prohibited: deleting or renumbering a principle, deleting a prior report, adding, removing, or weakening any gate, threshold, warning class, analyzer invocation, job, or dependency, and any sentence claiming a future port is scheduled, funded, or planned (contract C2, P1 through P5, FR-019)

- [ ] T004 Verify the amendment against all nine required statements and all five prohibitions in `specs/010-linux-only/contracts/platform-policy.md`, recording each with its verdict in `specs/010-linux-only/tasks.md` under a `Phase 2 outcomes` heading. Read S1 through S9 off the constitution by search and confirm each is present; read P1 through P5 and confirm each holds. Run the quickstart section 2 commands: `sed -n '/### VIII. CI Quality Gates/,/### IX\./p' .specify/memory/constitution.md | head -12`, `grep -nE 'ci-macos|ci-windows|AppleClang|MSVC' .specify/memory/constitution.md` whose hits must all sit inside a `Prior report` block, and `tail -3` for the version footer (SC-003, FR-001 through FR-008)

**Checkpoint**: The constitution reads 2.11.0, names Linux alone in its gate list, and supersedes 2.7.0 and 2.8.0 by a report that keeps both.

---

## Phase 3: User Story 1 - A developer reads one true statement of what is supported (Priority: P1) 🎯 MVP

**Goal**: Every command a developer is told to run works on Linux, every platform statement they read is true, and a macOS or Windows reader is told plainly that no build is offered.

**Independent Test**: Follow `README.md` and `AGENTS.md` end to end on Linux, running every command each names. Then read the three configure-time diagnostics and confirm none names an unsupported platform. Every command resolves and every platform statement matches the amendment from Phase 2.

### Implementation for User Story 1

- [ ] T005 [P] [US1] Amend `README.md`. (a) In the `## Build` section, state that Linux is the supported platform and that no build is offered for macOS or Windows, and name the two Linux distribution families the CI jobs cover. (b) In the hwloc re-pinning section's autotools bootstrap block, delete the Homebrew line and the `brew install autoconf automake libtool` line, keeping the Debian-family `apt-get` line and the RPM-family `dnf` line. (c) Leave the other five re-pinning sections, the Contracts section, and the Quality gates section alone; the Quality gates section already documents the pinned formatter and says nothing about a platform (FR-012, FR-013, FR-014; quickstart section 3)

- [ ] T006 [P] [US1] Amend `AGENTS.md`. (a) In the build, test, and verify section, name `ci-ubuntu` plus `cmake --build build` on Linux and drop the `cmake --preset=ci-macos` on macOS and `cmake --preset=ci-windows` on Windows clauses, so the instruction matches the amended IX clause T003 rewrote. (b) In the same section, state that the platform set is Linux and that a future specification adds a platform. (c) In the CI matrix sentence, keep Linux with clang-tidy and cppcheck, keep sanitizers and coverage, and drop the macOS and Windows entries. (d) In the personalization section, change nothing; it concerns the machine-local wiki and names no platform (FR-014)

- [ ] T007 [US1] Amend the three diagnostic strings and the one header comment in `cmake/ImportAutotoolsSubmodule.cmake`, and nothing else in that file. (a) Line 68's header comment lists the package sets the module handles: drop the `and brew` phrase, keeping apt-get and dnf. (b) Lines 140 to 142, the Windows abort: change `this module supports Linux and macOS only.` to name Linux alone, and keep the sentence naming the upstream `contrib/windows-cmake/` on-ramp verbatim, because that sentence is the port's entry point. (c) Lines 204 to 205, the missing-host-tools abort: delete the `macOS: brew install autoconf automake libtool (BSD patch ships with macOS).` fragment and keep the Debian-family and RPM-family lines. (d) Lines 567 to 568, the catch-all abort: change `Supported set: Linux and macOS.` to name Linux alone, keeping the on-ramp sentence. (e) Change no branch and no other comment: `if(WIN32)` at line 136, `elseif(UNIX OR APPLE)` at line 144, `if(NOT APPLE AND IAS_MERGE_INTO)` at line 452, the `Platform block` comments at lines 74, 137, and 433, and the Homebrew comment at line 170 all stay byte-identical (FR-013a, FR-016; research.md R-003, R-005; contract C2 P5)

**Checkpoint**: US1 is fully functional: a developer on Linux reads only true statements, and a developer on macOS or Windows is told no build is offered.

---

## Phase 4: User Story 2 - The gate set and its governance match the policy (Priority: P1)

**Goal**: No preset offers a build no runner has ever run, every surviving preset resolves, and the CI matrix is untouched.

**Independent Test**: Audit `CMakePresets.json` for a preset naming an unsupported platform and find zero. Configure through one preset per surviving inheritance shape. Count the CI jobs and compare against `fbdfc6f`.

### Implementation for User Story 2

- [ ] T008 [US2] Delete seven presets from `CMakePresets.json`: `flags-appleclang`, `flags-msvc`, `ci-darwin`, `ci-win64`, `ci-macos`, `ci-windows`, and `ci-multi-config`. That leaves 15 configure presets, 1 build preset, and 1 test preset. The file is preset schema version 2, so it carries no `include` array and none can pull a deleted preset back in. The seven form a closed cluster under `inherits`: every edge naming a deleted preset originates inside a deleted preset, so no survivor is left with a deleted parent. Keep the JSON's existing key order, indentation, and trailing newline so the diff shows only the seven removals. Do NOT remove `ci-std`, which survives on `ci-linux`'s reference, and do NOT remove `ci-build`, `coverage-linux`, `dev-mode`, `flags-gcc-clang`, `cmake-pedantic`, `cppcheck`, `clang-tidy`, `ci-linux`, `ci-linux-ignore`, or `ci-linux-audit` (FR-009, FR-010; research.md R-001)

- [ ] T009 [US2] Verify the preset set and record each verdict in `specs/010-linux-only/tasks.md` under a `Phase 4 outcomes` heading. (a) Run the quickstart section 5 Python check: 15 survivors and zero dangling `inherits`. (b) Run `cmake --preset=dev`, `cmake --preset=ci-ubuntu`, and `cmake --preset=ci-coverage`, each of which must exit 0; these three cover every inheritance shape the survivors use, which is a single parent, a two-element list, and a five-element list. A fresh binary directory needs `doxygen` on `PATH` because `cmake/dbc-gate.cmake` requires it. (c) Confirm the six presets the CI jobs use all survive: `ci-coverage`, `ci-sanitize`, `ci-ubuntu`, `ci-rocky`, `ci-linux-audit`, `ci-linux-ignore`, across nine invocations in seven jobs. (d) Run `git diff fbdfc6f..HEAD -- .github/` and confirm it is empty (FR-010, FR-011, SC-004, SC-005; research.md R-007)

- [ ] T010 [US2] Verify the CI matrix is untouched, recording the verdict in `specs/010-linux-only/tasks.md` under a `Phase 4 outcomes` section. Run `grep -cE '^  [a-z-]+:$' .github/workflows/ci.yml`, which must return 12, being 11 jobs plus the `push` trigger, and `grep -nE 'runs-on:' .github/workflows/ci.yml | grep -v ubuntu-26.04`, which must return nothing. Note in the record that `test-rocky` names a `rockylinux:10` container, which is a second Linux distribution. A second operating system is a different thing, and that a token sweep of `.github/` returns zero platform hits, which is why this feature edits no workflow file (FR-011, SC-005)

**Checkpoint**: US1 and US2 both work independently. No surface claims a platform the project cannot test.

---

## Phase 5: User Story 3 - A future port stays additive (Priority: P2)

**Goal**: A maintainer who wants macOS or Windows support adds it without touching anything this feature changed, and the on-ramp is written down.

**Independent Test**: Diff the tree against `fbdfc6f` and confirm zero lines changed inside any per-platform branch and zero lines changed in any C++ file. Read the amendment's on-ramp statement and confirm it names every deleted preset.

### Implementation for User Story 3

- [ ] T011 [US3] Prove the preserved seams did not move, and record the verdict in `specs/010-linux-only/tasks.md` under a `Phase 5 outcomes` heading. (a) `git diff fbdfc6f..HEAD -- source include test example` must be empty: no C++ file changes at all. (b) `git diff fbdfc6f..HEAD -- CMakeLists.txt` must be empty, since its 15 platform hits are all `MSVC`, `WIN32`, and `CMAKE_HOST_WIN32` shims and their comments. (c) `git diff fbdfc6f..HEAD -- cmake/variables.cmake cmake/VendoredArchiveMerge.cmake` must be empty. (d) `git diff fbdfc6f..HEAD -- cmake/ImportAutotoolsSubmodule.cmake` must touch only lines 68, 140, 141, 204, 205, 567, and 568, so read the diff hunk by hunk and confirm no hunk touches a branch line. (e) List the 45 seam lines from T001's record beside the diff and confirm every one is absent from it (FR-016, SC-006; contract C3)

- [ ] T012 [US3] Prove the on-ramp is written down, and record the verdict in `specs/010-linux-only/tasks.md` under a `Phase 5 outcomes` heading. Read the Sync Impact Report T003 wrote and confirm it names all seven deleted presets by name and states that every per-platform block is untouched. Confirm it also states that a future specification adds a platform and nothing more, so no reader infers a scheduled port. A port that restores a preset inherits the branches this feature preserved, which is the property FR-016 exists to protect (FR-008, SC-008)

- [ ] T013 [US3] Confirm the tracked diff touches exactly the five files SC-006 names, and record the file list in `specs/010-linux-only/tasks.md` under a `Phase 5 outcomes` heading. Run `git diff --name-only fbdfc6f..HEAD` and confirm it lists `.specify/memory/constitution.md`, `CMakeLists.txt` never, `CMakePresets.json`, `README.md`, `AGENTS.md`, and `cmake/ImportAutotoolsSubmodule.cmake`, plus this feature's own files under `specs/010-linux-only/`. Confirm `CMakeUserPresets.json` is absent, since T002's fix is gitignored and carries no tracked diff, and confirm `.codespellrc` is absent because FR-016a preserves its comment (FR-017, FR-018, SC-006)

**Checkpoint**: US1, US2, and US3 all work independently. A future port is additive.

---

## Phase 6: User Story 4 - The merged record stays intact and is superseded by name (Priority: P3)

**Goal**: The five merged vendor specs keep their original platform text, and a reader who finds that text learns from the constitution that it no longer holds.

**Independent Test**: Diff `specs/` against `fbdfc6f` and confirm zero merged file changed. Then read the amendment's supersession statement and confirm it names every spec it supersedes.

### Implementation for User Story 4

- [ ] T014 [US4] Prove the five merged vendor specs are byte-identical to `fbdfc6f`, recording the verdict in `specs/010-linux-only/tasks.md` under a `Phase 6 outcomes` heading. Run `git diff fbdfc6f..HEAD -- specs/003-vendor-hwloc specs/004-vendor-simdjson specs/005-vendor-hdrhistogram specs/006-vendor-yaml-cpp specs/009-vendor-quill` and confirm it is empty. Each spec keeps its Fixed decision stating that Linux is the enforced gate, that macOS must keep building for developers, and that Windows stays possible by design. Those statements were true when the specs shipped and stay true as records of that decision; the amendment supersedes them prospectively (FR-017, SC-008)

- [ ] T015 [US4] Prove the supersession is discoverable from the constitution, recording the verdict in `specs/010-linux-only/tasks.md` under a `Phase 6 outcomes` heading. Run `grep -nE 'specs/00(3|4|5|6|9)-' .specify/memory/constitution.md` and confirm at least one hit inside the Sync Impact Report, naming every superseded spec. Then locate `specs/009-vendor-quill` T038 in that feature's own task file and record that it retires by reference: the constitution's deferral entry, which T003 deleted, is the only thing holding it open, so its closure needs no edit to any merged file and T038 stays marked open in the 009 task file (FR-004, FR-007, SC-008)

**Checkpoint**: All four user stories are independently functional.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [ ] T016 Run the FR-015 platform-token audit from `specs/010-linux-only/quickstart.md` section 3 over the full thirteen-token set and the full path list, and record every hit in `specs/010-linux-only/tasks.md` under a `Phase 7 outcomes` heading with its path, its line, and its bucket. Expected: zero hits in the live-claim bucket, every remaining hit landing in preserved-seam, historical-record, or third-party-tooling. The narrow four-token pattern this replaces caught none of the three diagnostics T007 fixed, so the width is load-bearing (FR-015, SC-002; research.md R-004; contract C3)

- [ ] T017 Run the full local gate suite green and record each command with its pass or fail result in `specs/010-linux-only/tasks.md` under a `Phase 7 outcomes` section: `cmake --build --preset=dev -j 8`, `ctest --preset=dev` reporting 42 of 42, `cmake --preset=ci-ubuntu` then `cmake --build build -j 8`, `cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake`, `cmake --build build/dev -t prose-lint`, `cmake --build build/dev -t spell-check`, and `cmake --build build/dev -t dbc-gate`. Pass `FORMAT_COMMAND=clang-format-18` because the repository's `lint` job installs that version and a newer local formatter names files the pinned one calls clean. If `dbc-gate` reports exit 127 for a missing `doxygen` binary, record it as this machine's missing external tool and say so in the record. Reporting a gate failure the change did not cause is the wrong verdict (SC-007)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies, can start immediately
- **Foundational (Phase 2)**: Depends on Setup, BLOCKS US1 and US4. The amendment is the governance anchor both reference, and landing a document that claims Linux is supported while the constitution still lists three platforms opens a window where the project's two authorities disagree
- **User Stories (Phase 3+)**: US2, US3, and US4 depend on Phase 2. US1 depends on Phase 2 for the same reason. US2, US3, and US4 otherwise touch different concerns and can proceed in parallel
- **Polish (Phase 7)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: starts after Foundational, touches `README.md`, `AGENTS.md`, and `cmake/ImportAutotoolsSubmodule.cmake`, none of which another story touches
- **User Story 2 (P1)**: starts after Foundational, touches `CMakePresets.json` and reads `.github/workflows/ci.yml`. It reads the workflow and edits nothing there
- **User Story 3 (P2)**: starts after Foundational, edits nothing. Every task is a diff read plus a record, so it can run alongside US1 and US2, and it must run after them to see their diffs
- **User Story 4 (P3)**: starts after Foundational, edits nothing outside this feature's own task file. It reads the amendment T003 wrote, so it runs after Phase 2

### Within Each User Story

- T005 and T006 touch different files and run in parallel; T007 follows both, because its diagnostics and the README describe the same supported set
- T009 and T010 are independent verifications and run in parallel after T008
- T011, T012, and T013 all read diffs, so they follow the edits they measure: T011 and T013 after T005 through T008, T012 after T003
- T014 and T015 are independent of each other

### Parallel Opportunities

- T002 runs alongside T001
- T005, T006 run in parallel
- T009, T010 run in parallel
- T011, T012, T013, T014, T015 run in parallel once the edits they measure have landed
- T016, T017 run in parallel after every story phase completes

---

## Parallel Example: Foundational plus US2's edit

```bash
# Launch the independent Setup pieces together:
Task: "Record the pre-change inventory in specs/010-linux-only/tasks.md"
Task: "Trim CMakeUserPresets.json so no local preset inherits a deleted one"

# US2's preset deletion touches one file and nothing else in Phase 4:
Task: "Delete seven presets from CMakePresets.json"
```

---

## Implementation Strategy

### MVP First (User Story 1 and User Story 2 are both P1)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational, the amendment
3. Complete Phase 3: User Story 1, the documents
4. Complete Phase 4: User Story 2, the presets
5. **STOP and VALIDATE**: run quickstart sections 1 through 6 and confirm every verdict
6. Ship

### Incremental Delivery

1. Complete Setup and Foundational, governance settled
2. Add US1, a developer reads only true statements
3. Add US2, no preset claims a platform the project cannot test
4. Add US3, a future port is additive
5. Add US4, the merged record is intact and superseded
6. Each story adds value while the previous ones keep holding

### Parallel Team Strategy

1. One person completes Phase 2, the amendment, because it is one file with nine required statements and cannot be split across two editors
2. Then one person takes US1's three files, another takes US2's preset deletion, serialized on nothing
3. US3 and US4 are verification passes and run in parallel with each other once the edits land

---

## Notes

- [P] tasks touch different files with no dependencies
- [Story] label maps each task to its user story for traceability
- Each user story is independently completable and testable
- Every requirement is verified by a command in `specs/010-linux-only/quickstart.md`; no CTest entry is added because no code changes
- Commit after each task or logical group; the house template is `<Section>: <imperative>` at 50 characters or fewer with an `Approved-by:` footer and `Refs: specs/010-linux-only`
- This feature amends the constitution, so its own commit history carries a governance change. Keep that in its own commit, separate from the document edits, so a reviewer reads the gate change on its own
- Stop at any checkpoint to validate the story independently
- Avoid: vague tasks, two tasks editing one file in the same parallel batch, any edit to a file under `specs/003`, `004`, `005`, `006`, or `009`, and any edit to a per-platform branch