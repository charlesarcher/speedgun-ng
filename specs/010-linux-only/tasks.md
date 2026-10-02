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

- [X] T001 Record the pre-change inventory in `specs/010-linux-only/tasks.md` under a `Phase 1 outcomes` heading, so SC-002's before-and-after counts have a committed baseline: 8 live-claim lines in 2 files, 37 preserved-seam lines across 8 files, and 116 historical lines under `specs/`. The eight live-claim lines are `cmake/ImportAutotoolsSubmodule.cmake` lines 68, 140, 141, 204, 205, 567, 568 and `.codespellrc` line 12, and `.codespellrc` line 12 is classified as a historical record under FR-016a, which leaves seven live-claim lines to change. Record the seam inventory verbatim from the Phase 0 sweep (research.md R-004) so a later pass can diff against it without re-running the sweep. Per Constitution X.4, a completion claim without recorded evidence is non-compliant

- [X] T002 [P] Trim `CMakeUserPresets.json` so no local preset inherits a preset this feature deletes. That file is gitignored at `.gitignore:10` and carries `dev-darwin` inheriting `ci-darwin` and `dev-win64` inheriting `ci-win64`. CMake validates every preset in the resolved set before resolving any single one, so a dangling `inherits` fails the whole directory and takes `cmake --preset=dev` and `ctest --preset=dev` with it. Verify with `cmake --list-presets`, which must exit 0 (FR-018a; research.md R-002; quickstart section 1). Leave `dev-common`, `dev-linux`, `dev`, and `dev-coverage` in place: their whole chain survives. Do NOT commit this file; its fix carries zero tracked diff, and that is the point

**Checkpoint**: The baseline is on record and the local preset file resolves. Every later verification command can run.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The governance amendment. Nine required statements in one file, and every other surface in this feature points at them.

**⚠️ CRITICAL**: US1 must not land before this. A README that claims Linux is supported while the constitution still lists three platforms is a window where the project's two authorities disagree, and the window is the whole point of the change. US4's supersession statement is part of this edit, so that story cannot verify until it lands.

- [X] T003 Amend `.specify/memory/constitution.md` to 2.11.0, MAJOR, in the four text sites the contract names plus the governance metadata. (a) Sync Impact Report: a new HTML comment at the top of the file, above the existing 2.10.0 report, which becomes a `Prior report` beside the 2.7.0 and 2.8.0 reports, which both stay (S8, FR-005, FR-006). (b) VIII's hard gate list: name Linux and enumerate no other platform, and replace the sentence reinstating the Windows gate when an upstream port lands, because a future specification adds a platform and landing an upstream port does not restore one (S1, S2, FR-001). (c) Additional Constraints: the Language clause's supported-platform definition names Linux alone, and the preserved warning-set clause drops the `/W4 /permissive-` on MSVC reference while keeping the GCC and Clang family (S3, S4, FR-002). (d) IX's per-feature release-build clause names `ci-ubuntu` and no other preset; the obligation of one release build per feature is unchanged (S5, FR-003). (e) Open deferrals: delete the macOS-runner entry whole, keeping the other two entries. That deletion is what retires `specs/009-vendor-quill` T038 by reference (S6, FR-004). (f) Version lineage table: add the 2.11.0 row with its rationale, set the footer to `**Version**: 2.11.0 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-10-02`, and leave every prior row including 2.7.0 and 2.8.0 (S7, FR-005). (g) In the Sync Impact Report, name `specs/003-vendor-hwloc`, `specs/004-vendor-simdjson`, `specs/005-vendor-hdrhistogram`, `specs/006-vendor-yaml-cpp`, and `specs/009-vendor-quill` as superseded on the platform question, and state the on-ramp by naming all seven deleted presets plus the note that every per-platform block is untouched (S9, FR-007, FR-008). Prohibited: deleting or renumbering a principle, deleting a prior report, adding, removing, or weakening any gate, threshold, warning class, analyzer invocation, job, or dependency, and any sentence claiming a future port is scheduled, funded, or planned (contract C2, P1 through P5, FR-019)

- [X] T004 Verify the amendment against all nine required statements and all five prohibitions in `specs/010-linux-only/contracts/platform-policy.md`, recording each with its verdict in `specs/010-linux-only/tasks.md` under a `Phase 2 outcomes` heading. Read S1 through S9 off the constitution by search and confirm each is present; read P1 through P5 and confirm each holds. Run the quickstart section 2 commands: `sed -n '/### VIII. CI Quality Gates/,/### IX\./p' .specify/memory/constitution.md | head -12`, `grep -nE 'ci-macos|ci-windows|AppleClang|MSVC' .specify/memory/constitution.md` whose hits must all sit inside a `Prior report` block, and `tail -3` for the version footer (SC-003, FR-001 through FR-008)

**Checkpoint**: The constitution reads 2.11.0, names Linux alone in its gate list, and supersedes 2.7.0 and 2.8.0 by a report that keeps both.

---

## Phase 3: User Story 1 - A developer reads one true statement of what is supported (Priority: P1) 🎯 MVP

**Goal**: Every command a developer is told to run works on Linux, every platform statement they read is true, and a macOS or Windows reader is told plainly that no build is offered.

**Independent Test**: Follow `README.md` and `AGENTS.md` end to end on Linux, running every command each names. Then read the three configure-time diagnostics and confirm none names an unsupported platform. Every command resolves and every platform statement matches the amendment from Phase 2.

### Implementation for User Story 1

- [X] T005 [P] [US1] Amend `README.md`. (a) In the `## Build` section, state that Linux is the supported platform and that no build is offered for macOS or Windows, and name the two Linux distribution families the CI jobs cover. (b) In the hwloc re-pinning section's autotools bootstrap block, delete the Homebrew line and the `brew install autoconf automake libtool` line, keeping the Debian-family `apt-get` line and the RPM-family `dnf` line. (c) Leave the other five re-pinning sections, the Contracts section, and the Quality gates section alone; the Quality gates section already documents the pinned formatter and says nothing about a platform (FR-012, FR-013, FR-014; quickstart section 3)

- [X] T006 [P] [US1] Amend `AGENTS.md`. (a) In the build, test, and verify section, name `ci-ubuntu` plus `cmake --build build` on Linux and drop the `cmake --preset=ci-macos` on macOS and `cmake --preset=ci-windows` on Windows clauses, so the instruction matches the amended IX clause T003 rewrote. (b) In the same section, state that the platform set is Linux and that a future specification adds a platform. (c) In the CI matrix sentence, keep Linux with clang-tidy and cppcheck, keep sanitizers and coverage, and drop the macOS and Windows entries. (d) In the personalization section, change nothing; it concerns the machine-local wiki and names no platform (FR-014)

- [X] T007 [US1] Amend the three diagnostic strings and the one header comment in `cmake/ImportAutotoolsSubmodule.cmake`, and nothing else in that file. (a) Line 68's header comment lists the package sets the module handles: drop the `and brew` phrase, keeping apt-get and dnf. (b) Lines 140 to 142, the Windows abort: change `this module supports Linux and macOS only.` to name Linux alone, and keep the sentence naming the upstream `contrib/windows-cmake/` on-ramp verbatim, because that sentence is the port's entry point. (c) Lines 204 to 205, the missing-host-tools abort: delete the `macOS: brew install autoconf automake libtool (BSD patch ships with macOS).` fragment and keep the Debian-family and RPM-family lines. (d) Lines 567 to 568, the catch-all abort: change `Supported set: Linux and macOS.` to name Linux alone, keeping the on-ramp sentence. (e) Change no branch and no other comment: `if(WIN32)` at line 136, `elseif(UNIX OR APPLE)` at line 144, `if(NOT APPLE AND IAS_MERGE_INTO)` at line 452, the `Platform block` comments at lines 74, 137, and 433, and the Homebrew comment at line 170 all stay byte-identical (FR-013a, FR-016; research.md R-003, R-005; contract C2 P5)

**Checkpoint**: US1 is fully functional: a developer on Linux reads only true statements, and a developer on macOS or Windows is told no build is offered.

---

## Phase 4: User Story 2 - The gate set and its governance match the policy (Priority: P1)

**Goal**: No preset offers a build no runner has ever run, every surviving preset resolves, and the CI matrix is untouched.

**Independent Test**: Audit `CMakePresets.json` for a preset naming an unsupported platform and find zero. Configure through one preset per surviving inheritance shape. Count the CI jobs and compare against `fbdfc6f`.

### Implementation for User Story 2

- [X] T008 [US2] Delete seven presets from `CMakePresets.json`: `flags-appleclang`, `flags-msvc`, `ci-darwin`, `ci-win64`, `ci-macos`, `ci-windows`, and `ci-multi-config`. That leaves 15 configure presets, 1 build preset, and 1 test preset. The file is preset schema version 2, so it carries no `include` array and none can pull a deleted preset back in. The seven form a closed cluster under `inherits`: every edge naming a deleted preset originates inside a deleted preset, so no survivor is left with a deleted parent. Keep the JSON's existing key order, indentation, and trailing newline so the diff shows only the seven removals. Do NOT remove `ci-std`, which survives on `ci-linux`'s reference, and do NOT remove `ci-build`, `coverage-linux`, `dev-mode`, `flags-gcc-clang`, `cmake-pedantic`, `cppcheck`, `clang-tidy`, `ci-linux`, `ci-linux-ignore`, or `ci-linux-audit` (FR-009, FR-010; research.md R-001)

- [X] T009 [US2] Verify the preset set and record each verdict in `specs/010-linux-only/tasks.md` under a `Phase 4 outcomes` heading. (a) Run the quickstart section 5 Python check: 15 survivors and zero dangling `inherits`. (b) Run `cmake --preset=dev`, `cmake --preset=ci-ubuntu`, and `cmake --preset=ci-coverage`, each of which must exit 0; these three cover every inheritance shape the survivors use, which is a single parent, a two-element list, and a five-element list. A fresh binary directory needs `doxygen` on `PATH` because `cmake/dbc-gate.cmake` requires it. (c) Confirm the six presets the CI jobs use all survive: `ci-coverage`, `ci-sanitize`, `ci-ubuntu`, `ci-rocky`, `ci-linux-audit`, `ci-linux-ignore`, across nine invocations in seven jobs. (d) Run `git diff fbdfc6f..HEAD -- .github/` and confirm it is empty (FR-010, FR-011, SC-004, SC-005; research.md R-007)

- [X] T010 [US2] Verify the CI matrix is untouched, recording the verdict in `specs/010-linux-only/tasks.md` under a `Phase 4 outcomes` section. Run `grep -cE '^  [a-z-]+:$' .github/workflows/ci.yml`, which must return 12, being 11 jobs plus the `push` trigger, and `grep -nE 'runs-on:' .github/workflows/ci.yml | grep -v ubuntu-26.04`, which must return nothing. Note in the record that `test-rocky` names a `rockylinux:10` container, which is a second Linux distribution. A second operating system is a different thing, and that a token sweep of `.github/` returns zero platform hits, which is why this feature edits no workflow file (FR-011, SC-005)

**Checkpoint**: US1 and US2 both work independently. No surface claims a platform the project cannot test.

---

## Phase 5: User Story 3 - A future port stays additive (Priority: P2)

**Goal**: A maintainer who wants macOS or Windows support adds it without touching anything this feature changed, and the on-ramp is written down.

**Independent Test**: Diff the tree against `fbdfc6f` and confirm zero lines changed inside any per-platform branch and zero lines changed in any C++ file. Read the amendment's on-ramp statement and confirm it names every deleted preset.

### Implementation for User Story 3

- [X] T011 [US3] Prove the preserved seams did not move, and record the verdict in `specs/010-linux-only/tasks.md` under a `Phase 5 outcomes` heading. (a) `git diff fbdfc6f..HEAD -- source include test example` must be empty: no C++ file changes at all. (b) `git diff fbdfc6f..HEAD -- CMakeLists.txt` must be empty, since its 15 platform hits are all `MSVC`, `WIN32`, and `CMAKE_HOST_WIN32` shims and their comments. (c) `git diff fbdfc6f..HEAD -- cmake/variables.cmake cmake/VendoredArchiveMerge.cmake` must be empty. (d) `git diff fbdfc6f..HEAD -- cmake/ImportAutotoolsSubmodule.cmake` must touch only lines 68, 140, 141, 204, 205, 567, and 568, so read the diff hunk by hunk and confirm no hunk touches a branch line. (e) List the 45 seam lines from T001's record beside the diff and confirm every one is absent from it (FR-016, SC-006; contract C3)

- [X] T012 [US3] Prove the on-ramp is written down, and record the verdict in `specs/010-linux-only/tasks.md` under a `Phase 5 outcomes` heading. Read the Sync Impact Report T003 wrote and confirm it names all seven deleted presets by name and states that every per-platform block is untouched. Confirm it also states that a future specification adds a platform and nothing more, so no reader infers a scheduled port. A port that restores a preset inherits the branches this feature preserved, which is the property FR-016 exists to protect (FR-008, SC-008)

- [X] T013 [US3] Confirm the tracked diff touches exactly the five files SC-006 names, and record the file list in `specs/010-linux-only/tasks.md` under a `Phase 5 outcomes` heading. Run `git diff --name-only fbdfc6f..HEAD` and confirm it lists `.specify/memory/constitution.md`, `CMakeLists.txt` never, `CMakePresets.json`, `README.md`, `AGENTS.md`, and `cmake/ImportAutotoolsSubmodule.cmake`, plus this feature's own files under `specs/010-linux-only/`. Confirm `CMakeUserPresets.json` is absent, since T002's fix is gitignored and carries no tracked diff, and confirm `.codespellrc` is absent because FR-016a preserves its comment (FR-017, FR-018, SC-006)

**Checkpoint**: US1, US2, and US3 all work independently. A future port is additive.

---

## Phase 6: User Story 4 - The merged record stays intact and is superseded by name (Priority: P3)

**Goal**: The five merged vendor specs keep their original platform text, and a reader who finds that text learns from the constitution that it no longer holds.

**Independent Test**: Diff `specs/` against `fbdfc6f` and confirm zero merged file changed. Then read the amendment's supersession statement and confirm it names every spec it supersedes.

### Implementation for User Story 4

- [X] T014 [US4] Prove the five merged vendor specs are byte-identical to `fbdfc6f`, recording the verdict in `specs/010-linux-only/tasks.md` under a `Phase 6 outcomes` heading. Run `git diff fbdfc6f..HEAD -- specs/003-vendor-hwloc specs/004-vendor-simdjson specs/005-vendor-hdrhistogram specs/006-vendor-yaml-cpp specs/009-vendor-quill` and confirm it is empty. Each spec keeps its Fixed decision stating that Linux is the enforced gate, that macOS must keep building for developers, and that Windows stays possible by design. Those statements were true when the specs shipped and stay true as records of that decision; the amendment supersedes them prospectively (FR-017, SC-008)

- [X] T015 [US4] Prove the supersession is discoverable from the constitution, recording the verdict in `specs/010-linux-only/tasks.md` under a `Phase 6 outcomes` heading. Run `grep -nE 'specs/00(3|4|5|6|9)-' .specify/memory/constitution.md` and confirm at least one hit inside the Sync Impact Report, naming every superseded spec. Then locate `specs/009-vendor-quill` T038 in that feature's own task file and record that it retires by reference: the constitution's deferral entry, which T003 deleted, is the only thing holding it open, so its closure needs no edit to any merged file and T038 stays marked open in the 009 task file (FR-004, FR-007, SC-008)

**Checkpoint**: All four user stories are independently functional.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T016 Run the FR-015 platform-token audit from `specs/010-linux-only/quickstart.md` section 3 over the full thirteen-token set and the full path list, and record every hit in `specs/010-linux-only/tasks.md` under a `Phase 7 outcomes` heading with its path, its line, and its bucket. Expected: zero hits in the live-claim bucket, every remaining hit landing in preserved-seam, historical-record, or third-party-tooling. The narrow four-token pattern this replaces caught none of the three diagnostics T007 fixed, so the width is load-bearing (FR-015, SC-002; research.md R-004; contract C3)

- [X] T017 Run the full local gate suite green and record each command with its pass or fail result in `specs/010-linux-only/tasks.md` under a `Phase 7 outcomes` section: `cmake --build --preset=dev -j 8`, `ctest --preset=dev` reporting 42 of 42, `cmake --preset=ci-ubuntu` then `cmake --build build -j 8`, `cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake`, `cmake --build build/dev -t prose-lint`, `cmake --build build/dev -t spell-check`, and `cmake --build build/dev -t dbc-gate`. Pass `FORMAT_COMMAND=clang-format-18` because the repository's `lint` job installs that version and a newer local formatter names files the pinned one calls clean. If `dbc-gate` reports exit 127 for a missing `doxygen` binary, record it as this machine's missing external tool and say so in the record. Reporting a gate failure the change did not cause is the wrong verdict (SC-007)

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

---

### Phase 1 outcomes

Recorded per Constitution X.4, which requires recorded evidence. Every line
number below was read at the base commit `fbdfc6f`.

**Live-claim lines, 8 in 2 files.** These are the lines T005, T006, and T007
change or preserve by name.

| Line | File | Text | Fate |
|------|------|------|------|
| 68 | `cmake/ImportAutotoolsSubmodule.cmake` | header comment listing "the apt-get, dnf and brew package sets" | T007 changes, drops "and brew" |
| 140 | `cmake/ImportAutotoolsSubmodule.cmake` | "this module supports Linux and macOS only." | T007 changes |
| 141 | `cmake/ImportAutotoolsSubmodule.cmake` | "Linux and macOS only. The documented Windows on-ramp is the" | T007 changes |
| 204 | `cmake/ImportAutotoolsSubmodule.cmake` | "macOS: brew install autoconf automake libtool (BSD patch " | T007 deletes |
| 205 | `cmake/ImportAutotoolsSubmodule.cmake` | "ships with macOS)." | T007 deletes |
| 567 | `cmake/ImportAutotoolsSubmodule.cmake` | "platform '${CMAKE_SYSTEM_NAME}'. Supported set: Linux and " | T007 changes |
| 568 | `cmake/ImportAutotoolsSubmodule.cmake` | "macOS. The documented Windows on-ramp is the upstream " | T007 changes |
| 12 | `.codespellrc` | "# \"Sur\" is the macOS Big Sur codename in specs/003-vendor-hwloc docs." | FR-016a preserves it |

Seven of the eight change. Line 12 stays because it explains why the token
`sur` is exempt from the spelling gate, and the token's origin is the fact the
comment records (research.md R-005).

**Preserved-seam lines, 37 across 8 files.** Every one must be absent from
the diff T011 measures. The per-file counts below sum to 37, and the earlier
figure of 45 in this task's own text was wrong: it came from the sweep's bucket
tally, which counted lines this feature's four edits legitimately touch.

| File | Count | Lines |
|------|-------|-------|
| `CMakeLists.txt` | 15 | 249, 251, 253, 256, 257, 347, 348, 349, 354, 355, 529, 531, 532, 536, 537 |
| `cmake/variables.cmake` | 3 | 12, 16, 30 |
| `cmake/VendoredArchiveMerge.cmake` | 1 | 52 |
| `cmake/ImportAutotoolsSubmodule.cmake` | 6 | 74, 136, 137, 144, 433, 452 |
| `source/counters/clock_provider.cpp` | 5 | 16, 17, 63, 89, 118 |
| `include/speedgun-ng/speedgun-ng.hpp` | 1 | 8 |
| `source/counters/linux_pmu/table_parse.cpp` | 1 | 258 |
| five vendored gate units | 5 | one line each |

The gate-unit lines are `source/hdrhistogram/hdrhistogram_gate.cpp:29`,
`source/hwloc/hwloc_gate.cpp:23`, `source/simdjson/simdjson_gate.cpp:25`,
`source/yaml/yaml_gate.cpp:20`, and `source/zlib/zlib_gate.cpp:28`, each a
comment recording that MSVC coverage is absent. The two lines inside
`ImportAutotoolsSubmodule.cmake` that T007 must leave alone while editing the
file around them are 74, the upstream on-ramp documentation, and 137, the
label marking the Windows block unsupported, which stays true.

**Historical-record lines, 116 under `specs/` alone.** Not itemised here.
They are inventoried in research.md R-004 and none changes under FR-017.

**Third-party tooling, 5 lines.** All in `.specify/scripts/bash/common.sh` at
102, 252, 296, and 302, plus
`.specify/scripts/bash/create-new-feature.sh:218`. Spec Kit scaffolding
carrying portability comments about the tool. Out of scope.

**False positives the token set also returns.** The word "windows" meaning a
measurement window, in `include/speedgun-ng/counters_measurement.hpp:471` and
`:891`, `test/CMakeLists.txt:302`,
`test/source/counters_clock_push_test.cpp:107`, `:109`, `:139`, and `:352`,
`test/source/counters_recorder_test.cpp:9`, and
`source/counters/linux_pmu/group_io.cpp:1` and `:4`. Listed so a later reader
can see each was judged.

- **T002**: `dev-darwin` and `dev-win64` removed from `CMakeUserPresets.json`.
  `cmake --list-presets` exits 0. `dev-common`, `dev-linux`, `dev`, and
  `dev-coverage` stay, and their whole chain survives because every preset
  they inherit is one of the 15 survivors. `git status --short` shows no entry
  for the file, confirming FR-018a's claim that the fix carries zero tracked
  diff (research.md R-002; quickstart section 1)

### Phase 2 outcomes

Recorded per Constitution X.4. Every check below was run at the state this
pass produced, on Linux.

**T003, the amendment.** The constitution reads 2.11.0 with
`Last Amended` 2026-10-02. The Sync Impact Report sits at the top of the file
and the 2.10.0 report became a `Prior report` beside the 2.9.1, 2.9.0, 2.8.0,
2.7.0, and 2.6.0 reports, all six retained. The diff is 81 insertions and 29
deletions across five regions: the new report, three clause edits, the deleted
deferral entry, and the lineage row plus version footer.

**T004, the nine statements and five prohibitions.**

| Check | Result | Evidence |
|---|---|---|
| S1 gate list names Linux alone | PASS | the clause reads "Builds succeed for developer and CI presets on Linux (GCC/Clang)"; zero occurrences of "macOS (AppleClang)" or "Windows (MSVC)" remain inside it |
| S2 reinstatement clause replaced | PASS | "landing that port reinstates this gate" returns 0 hits; the clause now reads "a future specification adds a platform. An upstream port landing does not reinstate this gate." |
| S3 supported-platform definition | PASS | the Additional Constraints Language clause reads "Linux on GCC and Clang is the supported platform" |
| S4 MSVC warning flags dropped | PASS | "/W4 /permissive-" returns 0 hits; the GCC and Clang family stays |
| S5 release-build clause narrowed | PASS | the ci-macos preset name returns 0 hits; the clause names ci-ubuntu and the build command |
| S6 macOS deferral entry closed | PASS | "Principle VIII macOS enforcement" returns 0 hits; the block retains its other two entries |
| S7 lineage row and version footer | PASS | one lineage row whose version column reads 2.11.0 and whose date column reads 2026-10-02; the footer reads "**Version**: 2.11.0 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-10-02" |
| S8 prior reports retained and superseded | PASS | five `Prior report (2.7.0` through `(2.10.0` blocks present; the Sync Impact Report states the supersession of 2.7.0 and 2.8.0 |
| S9 supersession names and on-ramp | PASS | all five spec directories named in the report; all seven deleted presets named there |
| P1 no prior report deleted | PASS | the six Prior report blocks are intact |
| P2 no principle renamed or removed | PASS | eleven `### <numeral>.` principle headings, unchanged |
| P3 gate set content unchanged | PASS | zero gate lines deleted from the diff |
| P4 no scheduled port claim | PASS | the only matches for "scheduled", "planned", or "funded" are the report's own disclaimer and a pre-existing line about a coverage job |
| P5 no branch-removed claim | PASS | zero matches for a claim that a branch was removed or foreclosed |

**Environment gap found while verifying, outside the change.** This machine's
`~/.local/bin/cppcheck` and `~/.local/bin/doxygen` were symlinks into
`/tmp/opencode/tools/`, and that directory no longer exists, so both tools were
dangling. The `dev` preset runs cppcheck on every compile through the
machine-local `dev-common` preset, so `cmake --build --preset=dev` failed at
exit 2 with "Error running 'cppcheck'" on two translation units before the fix.
cppcheck is restored from the `cppcheck` wheel and now reports version 2.17.1,
against 2.22.0 before the loss. doxygen is restored from the distribution
package at version 1.18.0, matching what the symlink pointed at, and it does
not run: the packaged binary links `libclang.so.23.1`, `libclang-cpp.so.23.1`,
and `libLLVM.so.23.1`, while this machine has the 22.1 series. Upgrading
`llvm-libs` to 23 would change the toolchain every gate in this repository runs
under, which this change does not warrant, so doxygen is left unavailable here
and the two targets that need it are recorded as environment gaps in Phase 7.
Neither gap is caused by this feature: its diff touches four diagnostic strings
and no C++ file.

### Phase 3 outcomes

**T005, `README.md`.** The `## Build` section now opens with the platform
statement: Linux is supported on the distribution families the CI jobs cover,
macOS and Windows are unsupported and no build is offered for either, and a
future specification may add a platform. The hwloc autotools bootstrap block
kept its Debian-family and RPM-family lines and lost the Homebrew line and the
`brew install` line, which is a two-line deletion. A token sweep of the file
returns one hit, at the statement itself, which FR-012 requires.

**T006, `AGENTS.md`.** The build, test, verify section now names
`cmake --preset=ci-ubuntu` then `cmake --build build` and drops the `ci-macos`
and `ci-windows` clauses, matching the amended IX clause. A new paragraph
states that Linux is the supported platform with `ci-ubuntu` on Ubuntu and
`ci-rocky` on Rocky Linux as the release presets, that macOS and Windows are
unsupported, and that a future specification adds a platform. The CI matrix
sentence reads "Linux on GCC and Clang (clang-tidy, cppcheck), plus sanitizers
and coverage". The personalization section is untouched. One token hit remains,
at the unsupported statement, which is the required statement.

**T007, `cmake/ImportAutotoolsSubmodule.cmake`.** Four sites changed and the
diff is 5 insertions against 7 deletions across four hunks:

| Site | Change |
|------|--------|
| line 68 header comment | "the apt-get, dnf and brew package sets" reads "the apt-get and dnf package sets" |
| Windows abort | "this module supports Linux and macOS only." reads "this module supports Linux only." |
| missing-host-tools abort | the "macOS: brew install autoconf automake libtool (BSD patch ships with macOS)." fragment is deleted; both Linux lines stay |
| catch-all abort | "Supported set: Linux and macOS." reads "Supported set: Linux." |

Both on-ramp sentences survive verbatim, including the
`contrib/windows-cmake/` wrapper path. Confirmed unchanged in the working tree:
`if(WIN32)` at line 136, `elseif(UNIX OR APPLE)` at line 144,
`if(NOT APPLE AND IAS_MERGE_INTO)` at line 450, the Homebrew libtool-alias
comment at line 170, the on-ramp documentation at line 74, and the
"Platform block" labels at lines 137 and 433. `cmake --preset=dev` configures
at exit 0 and the build reaches exit 0 once cppcheck is restored.

### Phase 4 outcomes

**T008, `CMakePresets.json`.** Seven presets deleted: `flags-appleclang`,
`flags-msvc`, `ci-darwin`, `ci-win64`, `ci-macos`, `ci-windows`,
`ci-multi-config`. The diff is 46 deletions and zero insertions, so the change
is a pure removal and no surviving line was reformatted. 22 configure presets
become 15; the single build preset and the single test preset, both
`ci-sanitize`, stay. The surviving set is `cmake-pedantic`, `dev-mode`,
`cppcheck`, `clang-tidy`, `ci-std`, `flags-gcc-clang`, `ci-linux`,
`ci-linux-ignore`, `ci-linux-audit`, `coverage-linux`, `ci-coverage`,
`ci-sanitize`, `ci-build`, `ci-ubuntu`, and `ci-rocky`. `ci-std` and `ci-build`
stay because `ci-linux` and three CI presets inherit them.

An earlier attempt re-serialised the file through a JSON writer and exploded
every `inherits` array from one line to three, touching 24 surviving lines. It
was reverted and the deletion redone by line span, which is why the diff shows
no insertions.

**T009, preset resolution.** Zero dangling `inherits`, confirming
research.md R-001's finding that the seven form a closed cluster. All six
presets the CI jobs use survive. Configure exits 0 for `dev`, `ci-ubuntu`, and
`ci-coverage`, covering the three inheritance shapes the survivors use: a
single parent, a two-element list, and a five-element list. `cmake
--list-presets` exits 0. `git diff fbdfc6f..HEAD -- .github/` is empty.

**T010, the CI matrix.** `grep -cE '^  [a-z-]+:$'` returns 12, being 11 jobs
plus the `push` trigger, unchanged from the base commit. Every `runs-on:` line
reads `ubuntu-26.04`, and `test-rocky` names a `rockylinux:10` container, which
is a second Linux distribution and not a second operating system. No workflow
file is edited, confirmed by an empty diff over `.github/` both against
`fbdfc6f` and in the working tree.

### Phase 5 outcomes

**T011, the seams did not move.**

| Check | Result | Evidence |
|---|---|---|
| no C++ file changed | PASS | `git diff fbdfc6f..HEAD -- source include test example` is empty |
| `CMakeLists.txt` unchanged | PASS | the diff is empty, so all 15 of its shim lines and comments stand |
| `cmake/variables.cmake` and `cmake/VendoredArchiveMerge.cmake` unchanged | PASS | the diff is empty, so 4 seam lines stand |
| `ImportAutotoolsSubmodule.cmake` hunks | PASS | four hunks, at lines 65, 138, 200, and 564, none touching a branch line |
| all 37 seam lines unchanged | PASS | each was compared by content against `fbdfc6f` |

Two of the six module seam lines moved in **line number** while keeping their
text, because the host-tools edit above them removed two lines: the
ELF-only archiver comment is at 431, having been at 433, and
`if(NOT APPLE AND IAS_MERGE_INTO)` is at 450, having been at 452. Both were
verified by content, and both texts are byte-identical to `fbdfc6f`. A
line-number-only check reports them as changed, which is why the comparison
here is by content.

**T012, the on-ramp is written down.** All seven deleted presets are named in
the Sync Impact Report: `flags-appleclang` and `flags-msvc` once each,
`ci-darwin` and `ci-multi-config` once each, `ci-macos` three times,
`ci-windows` twice. The report states that a future specification adds a
platform and nothing more, so no reader infers a scheduled port, and it states
that the `if(WIN32)`, `if(APPLE)`, `if(MSVC)`, and `if(UNIX)` branches and
every `_WIN32` branch are untouched.

**T013, the tracked diff is five files.** `git diff --name-only fbdfc6f..HEAD`,
excluding this feature's own directory, returns
`.specify/memory/constitution.md`, `AGENTS.md`, `CMakePresets.json`,
`README.md`, and `cmake/ImportAutotoolsSubmodule.cmake`. `CMakeLists.txt` is
absent, so SC-006's list is exact. `CMakeUserPresets.json` is absent because
T002's fix is gitignored. `.codespellrc` is absent because FR-016a preserves
its comment.

**Correction to this feature's own record.** T001's task text and the Phase 1
outcomes both said 45 preserved-seam lines. The per-file table in the Phase 1
outcomes sums to 37: 15 in `CMakeLists.txt`, 3 in `cmake/variables.cmake`, 1 in
`cmake/VendoredArchiveMerge.cmake`, 6 in the autotools module, 5 in
`source/counters/clock_provider.cpp`, 1 in
`include/speedgun-ng/speedgun-ng.hpp`, 1 in
`source/counters/linux_pmu/table_parse.cpp`, and 5 across the vendored gate
units. The 45 came from the Phase 0 sweep's bucket tally, which counted lines
this feature's four legitimate edits touch. Both the task text and the outcomes
now read 37, so the record matches its own table. No requirement changes: FR-016
counts branch lines, and 37 is the correct count of them.

### Phase 6 outcomes

**T014, the merged record is intact.** `git diff fbdfc6f..HEAD` over
`specs/003-vendor-hwloc`, `specs/004-vendor-simdjson`,
`specs/005-vendor-hdrhistogram`, `specs/006-vendor-yaml-cpp`, and
`specs/009-vendor-quill` is empty. All five keep their Fixed decision stating
that Linux is the enforced gate, that macOS must keep building for developers,
and that Windows stays possible by design.

**T015, the supersession is discoverable.** The Sync Impact Report names every
superseded spec at lines 32 to 34: `specs/003-vendor-hwloc`,
`specs/004-vendor-simdjson`, `specs/005-vendor-hdrhistogram`,
`specs/006-vendor-yaml-cpp`, and `specs/009-vendor-quill`. `specs/009` T038
sits at line 619 of that feature's own task file, still marked open, and line
656 records that it stays open. It retires by reference: the constitution's
macOS deferral entry, deleted by T003, was the only thing holding it, so no
merged file needed editing and T038's own entry is left exactly as the feature
that wrote it recorded it.

### Phase 7 outcomes

**T016, the FR-015 audit.** 54 hits over the thirteen-token set and the
fourteen-path list. Every one is classified; the live-claim bucket holds
**0**.

| Bucket | Count | Where |
|--------|-------|-------|
| Required unsupported-statement | 2 | `README.md:19`, `AGENTS.md:50` |
| Preserved seam, build system | 19 | `CMakeLists.txt` 15, `cmake/variables.cmake` 3, `cmake/VendoredArchiveMerge.cmake` 1 |
| Preserved seam or on-ramp record, autotools module | 11 | `cmake/ImportAutotoolsSubmodule.cmake` 74, 136, 137, 141, 142, 144, 170, 431, 450, 566, 567 |
| Preserved seam, C++ | 12 | `source/counters/clock_provider.cpp` 5, five vendored gate units 5, `include/speedgun-ng/speedgun-ng.hpp` 1, `source/counters/linux_pmu/table_parse.cpp` 1 |
| Historical record | 1 | `.codespellrc:12` |
| False positive, a measurement window | 9 | `include/speedgun-ng/counters_measurement.hpp` 2, `test/source/counters_clock_push_test.cpp` 4, `test/source/counters_recorder_test.cpp` 1, `test/CMakeLists.txt` 1, `source/counters/linux_pmu/group_io.cpp` 1 |

The contract's four buckets needed a fifth class, and the plan did not name
it. `README.md:19` and `AGENTS.md:50` are the statements FR-012 and FR-014
require, and each says macOS and Windows are **unsupported**. Bucket 1 of the
contract is a live claim, defined as text telling a reader a platform is
supported, buildable, or usable, so a statement of the opposite kind is not in
it. Calling these two "live claims" would be wrong and would leave SC-002
unsatisfiable, and calling them preserved seams would be wrong too, because
neither is a seam. They are the required unsupported-statement, and they are
listed here so a reader counting 54 hits can account for every one.

Lines 141, 142, 566, and 567 of the autotools module sit inside the two aborts
this change edited, and they name `contrib/windows-cmake/`. They are the
on-ramp record FR-008 requires and the owner instructed be kept verbatim, so
they are a preserved seam inside a changed hunk, and outside bucket 1. A
test that removed the word "Linux" and left them would satisfy a naive audit
while destroying the port's entry point, which is the failure mode FR-015's
four-bucket classification exists to prevent.

Before the change the same audit returned 8 live-claim lines. It now returns
0, so SC-002's before-and-after holds.

**T017, the gate suite.** Every gate this host can run is green. Two are not
runnable here, and both are the same missing `libclang.so.23.1`, neither
caused by this change.

| Gate | Verdict | Evidence |
|------|---------|----------|
| `cmake --build --preset=dev -j 8` | PASS | exit 0, with cppcheck on every compile |
| `ctest --preset=dev` | 41 of 42 | the one failure is `dbc_gate_fixtures`, see below |
| `cmake --preset=ci-ubuntu` then `cmake --build build -j 8` | PASS | exit 0 |
| `format-check` with `clang-format-18` | PASS | exit 0, 0 badly formatted files |
| `prose-lint` | PASS | 17 sources, 1109 units examined, 0 findings |
| `spell-check` | PASS | exit 0 |
| `dbc-gate` | NOT RUNNABLE | exit 2: doxygen cannot load `libclang.so.23.1` |
| `dbc_gate_fixtures` | FAIL, environment | exit 127 from doxygen, same missing library |

**The two failures belong to this host.** Measured, with no assumption:
checking out the base commit `fbdfc6f` and running
`ctest --test-dir build/dev -R dbc_gate_fixtures` gives exit 8 on the same
test with the same cause, so the failure predates every commit on this branch.
The chain is that `~/.local/bin/doxygen` was a symlink into
`/tmp/opencode/tools/`, that directory no longer exists, and the distribution
doxygen 1.18.0 replaces it links `libclang.so.23.1`, `libclang-cpp.so.23.1`,
and `libLLVM.so.23.1` while this host carries the 22.1 series. `llvm-libs` 23
is available and installing it would fix both, and it would also change the
toolchain every other gate in this repository runs under, which a
documentation change does not warrant. The CI `dbc-gate` job installs its own
doxygen and is the authority for those two.

A third environment fault was found and fixed during this phase. cppcheck was
dangling by the same mechanism, and the `dev` preset runs it on every compile
through the machine-local `dev-common` preset, so the build failed at exit 2
with "Error running 'cppcheck'" before this pass. It is restored from the
`cppcheck` wheel at version 2.17.1, against 2.22.0 before the loss, and the
build has been green since. The symlink now points into
`~/.local/share/tools/`, which is on the filesystem and clear of `/tmp`, so
the breakage cannot recur the same way.

**Self-corrections this pass.** Two defects in the feature's own work were
caught by the gates and fixed, with no green verdict claimed for either: an
XI.2 contrast construction in the `AGENTS.md` platform paragraph, and a 61-character commit
title against the configured 50. The AGENTS.md fix also exposed a sequencing
error, because the correction sat unstaged while the two commits were made and
neither carried it. Both commits were rebuilt from the constitution commit so
the final text and the final messages are the committed ones.
