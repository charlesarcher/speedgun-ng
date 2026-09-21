---
description: "Dependency-ordered task list for vendoring hwloc as a private, pinned submodule"
---

# Tasks: Vendor hwloc as a Private, Pinned Submodule

**Input**: Design documents from `specs/003-vendor-hwloc/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/ingestion-module.md, contracts/privacy-contract.md, quickstart.md

**Tests**: Included. plan.md records TDD mode (Principle III): the purity scan and the static-archive `nm` proof are written red first against a fixture or a thin archive, then turned green. The ingestion module is build configuration, so its red-green runs at the quickstart surface (failing-before/after runs captured per X.4) while it is built. Per IX, the two proof-script tasks (T003, T004) precede the wrapper, module, and wiring tasks they gate, so each red observation runs against the pre-change baseline.

**Organization**: Tasks are grouped by user story. Setup and Foundational produce the blocking core; each user story is then independently testable.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Runs in parallel (different file, no dependency on an incomplete task)
- **[Story]**: User story the task serves (US1..US4)
- Every task names an exact file path
- IDs are stable once assigned; phase placement carries execution order

---

## Phase 1: Setup (Vendored Tree)

**Purpose**: Bring the pinned dependency into the tree.

- [ ] T001 Register hwloc as a git submodule at `external/hwloc` pinned to commit `b5660dff631a171a96a4b3abf5a170c9cf62d6ef` (tag `hwloc-2.14.0`), creating `.gitmodules`. Verify: `git submodule status external/hwloc` begins with that SHA, and `external/hwloc/COPYING` (BSD 3-Clause) is present (FR-001, FR-005).

---

## Phase 2: Foundational (Blocking Core)

**Purpose**: The red-first proof scripts, then the ingestion capability, the wrapper unit, and the call-site wiring. Every user story depends on these.

**CRITICAL**: No user story work begins until this phase is complete. TDD ordering (IX): the proof scripts T003 and T004 are written red first, against the pre-change baseline and a seeded fixture, before the wrapper (T005), module (T006), and wiring (T007) turn them green.

- [ ] T002 [P] Add `*/external` to the `skip` list in `.codespellrc` so the vendored tree is exempt from the spell gate (FR-009, R-013). Verify: `cmake -P cmake/spell.cmake` exits 0 with the submodule initialized.
- [ ] T003 [P] [US1] Red-first: create `tools/hwloc/hwloc_purity_scan.sh`, modeled on `tools/dbc/dependency_scan.sh`: assert zero `find_package(hwloc` and zero `pkg_check_modules(hwloc` in the build files and zero hwloc includes under `include/`, exiting 1 and naming every hit. Seed a violating fixture line, confirm the scan fails (red), then implement to green. This gates the module and wiring (T006, T007) that must introduce no discovery call (FR-004, FR-021, SC-009).
- [ ] T004 [P] [US1] Red-first: create `tools/hwloc/hwloc_nm_proof.sh`: run `nm` on the built `libspeedgun-ng.a` and assert the vendored members are present with the gate object's `sg_hwloc_get_api_version` reference resolving into them. Run it against the pre-merge thin archive to observe red, then green once the module and wiring merge lands (T006, T007) (FR-007, SC-011, R-010).
- [ ] T005 [P] Create `source/hwloc/hwloc_gate.cpp`, the sole translation unit that includes `<hwloc.h>`. It carries `static_assert(HWLOC_VERSION_MAJOR == 2 && HWLOC_VERSION_MINOR == 14 && HWLOC_VERSION_RELEASE == 0, ...)` naming expected hwloc 2.14.0, and an `[[maybe_unused]] constinit` function-pointer reference to `hwloc_get_api_version()` as the object-level link proof. The file holds zero runtime lines (FR-003, FR-007, FR-021, R-002, R-010).
- [ ] T006 [P] Create `cmake/ImportAutotoolsSubmodule.cmake`, the reusable module implementing the input contract in contracts/ingestion-module.md (`NAME`, `SUBMODULE_DIR`, `BOOTSTRAP` = `autogen.sh` | `PREGENERATED`, `CONFIGURE_ARGS`, `ARCHIVE`, optional `MERGE_INTO`). The POSIX block implements: a configure-time guard that aborts on a missing or empty `SUBMODULE_DIR` naming `git submodule update --init <SUBMODULE_DIR>`; a `find_program` probe of `autoconf`, `automake`, `libtool` (`glibtool` aliases), `patch`, `make`, `sh` that aborts with apt, dnf, and brew package names; a private vendor prefix keyed on `hash(CMAKE_C_COMPILER, compiler version, CMAKE_BUILD_TYPE, sanitizer-flag state, CONFIGURE_ARGS)`; an `ExternalProject` chain of copy, `autogen.sh`, out-of-tree configure, and make with `CONFIGURE_HANDLED_BY_BUILD ON`, `UPDATE_COMMAND ""`, `PATCH_COMMAND ""`, curated child `CFLAGS` (build-type `-O/-g` plus compatible hardening, zero `-fsanitize`, zero parent warning sets, zero `-Werror`) and explicit `CC`/`CXX`; staging of the archive and headers into that private prefix; a `STATIC IMPORTED GLOBAL` target with `IMPORTED_LOCATION`, `INTERFACE_INCLUDE_DIRECTORIES` (arriving as `SYSTEM` on the consumer so clang-tidy and cppcheck stay off the vendored C headers the wrapper includes, R-013), `INTERFACE_LINK_LIBRARIES` (`${CMAKE_DL_LIBS}` plus the platform thread library), and `BUILD_BYPRODUCTS`; and an `ar -M` `ADDLIB` merge helper for `MERGE_INTO` on static configurations. Unsupported platforms abort pointing at `contrib/windows-cmake/`. The header comment carries the input table, the guarantee prose, the sanitizer-exclusion policy and rationale, and a pointer to the contract file (FR-010 through FR-020, FR-025; R-001, R-004 through R-011, R-014; contracts/ingestion-module.md section 4).
- [ ] T007 Wire the call site in `CMakeLists.txt` after the export header: `include(ImportAutotoolsSubmodule)`, one `import_autotools_submodule(...)` call passing the five hwloc inputs with the `CONFIGURE_ARGS` from R-003, R-004, R-005, add `source/hwloc/hwloc_gate.cpp` to the library sources, `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE hwloc_vendor)`, and `MERGE_INTO speedgun-ng_speedgun-ng` for static builds. The call site holds inputs and imported-target usage only, with zero bespoke build commands (FR-007, FR-011, SC-010). Depends on T005 and T006.

**Checkpoint**: Foundation ready. The tree configures, the vendored archive ingests and merges, the wrapper compiles and links, and the two red-first proof scripts (T003, T004) now pass green. User-story work proceeds in priority order.

---

## Phase 3: User Story 1 - The project builds with a pinned, vendored hwloc (Priority: P1) - MVP

**Goal**: hwloc 2.14.0 compiles from the submodule, links into the library as a private static dependency, and the version tripwire plus the `nm` and purity proofs hold.

**Independent Test**: Clone with submodules, build, read `git submodule status`, run quickstart section 3 (version tripwire), and run `ctest -R 'hwloc_nm_proof|hwloc_purity_scan'`.

### Registration for User Story 1

The red-first proof scripts were created in Foundational (T003, T004); this phase registers them with CTest so `ctest` drives them.

- [ ] T008 [US1] Register `hwloc_purity_scan` and `hwloc_nm_proof` as CTest tests in `test/CMakeLists.txt`, following the `dbc_dependency_scan` pattern (`add_test(NAME ... COMMAND bash .../tools/hwloc/x.sh ${CMAKE_SOURCE_DIR})`). Verify: `ctest --preset=dev -R 'hwloc_purity_scan|hwloc_nm_proof' --output-on-failure` passes. Depends on T003 and T004.

**Checkpoint**: User Story 1 is independently verifiable. The version tripwire (quickstart section 3, SC-006) fires by moving the submodule to another tag and rebuilding: the `static_assert` diagnostic names expected hwloc 2.14.0.

---

## Phase 4: User Story 2 - Consumers cannot observe hwloc (Priority: P1)

**Goal**: Install tree, package files, shared-library symbols, and a downstream consumer test all show zero hwloc.

**Independent Test**: Install, audit the tree and package files, run the shared-build symbol and `ldd` audit, then run the downstream consumer test on a machine carrying hwloc. Depends only on Foundational.

### Implementation for User Story 2

- [ ] T009 [P] [US2] Create `test/consumer/CMakeLists.txt`: a plain `find_package(speedgun-ng REQUIRED)` project that links `speedgun-ng::speedgun-ng` and builds `main.cpp`. It never names hwloc (FR-026).
- [ ] T010 [P] [US2] Create `test/consumer/main.cpp`: a trivial consumer of the public speedgun-ng API exactly as README instructs, returning 0, with no hwloc reference (FR-026, SC-005).
- [ ] T011 [US2] Edit `.github/workflows/ci.yml`: add `submodules: true` to every `actions/checkout@v4` step, add `autoconf automake libtool patch` to the Linux jobs' `apt-get install` lists, and add `dnf -y install autoconf automake libtool patch` to the `test-rocky` toolchain step (FR-008, FR-009, R-012).
- [ ] T012 [US2] Edit `.github/workflows/ci.yml` `test` job: add post-install audit steps for the install tree (`find prefix/ -iname '*hwloc*'` empty, SC-002) and the package files (`grep -i hwloc prefix/lib/cmake/speedgun-ng/*.cmake` empty, SC-003) (FR-022, FR-023). Depends on T011.
- [ ] T013 [US2] Add a `shared-audit` job to `.github/workflows/ci.yml`: build `-DBUILD_SHARED_LIBS=ON`, install to a prefix, then assert `nm -D --defined-only <prefix>/lib/libspeedgun-ng.so` greps zero hwloc and `ldd <prefix>/lib/libspeedgun-ng.so` names no hwloc object. The audits run against the installed library, matching the privacy-contract A5 surface (FR-024, SC-004). Depends on T011.
- [ ] T014 [US2] Add a `downstream-consumer` job to `.github/workflows/ci.yml`: install the library to a prefix, `apt-get install libhwloc-dev` so the machine carries hwloc, then configure, build, and run `test/consumer` against the install with `CMAKE_PREFIX_PATH`. Assert each step exits 0 and grep the consumer configure log and link command for zero hwloc resolutions (FR-026, SC-005). Depends on T009, T010, and T011.

**Checkpoint**: User Stories 1 and 2 are independently verifiable. Every privacy surface carries zero hwloc.

---

## Phase 5: User Story 3 - The next autotools dependency reuses the ingestion capability (Priority: P2)

**Goal**: The module is a documented reusable capability, the call site is inputs-only, and the guarantees (stamp invalidation, toolchain diagnostics, platform confinement) hold.

**Independent Test**: Structure review against contracts/ingestion-module.md plus the build behavior from User Story 1, the stamp-invalidation run (quickstart section 9), and the missing-toolchain diagnostic (quickstart section 10). Depends on Foundational.

### Implementation for User Story 3

- [ ] T015 [US3] Extend `tools/hwloc/hwloc_purity_scan.sh` to assert the `CMakeLists.txt` call site holds zero `add_custom_command`, zero `execute_process`, zero archiver invocation (`ar`, `llvm-ar`, `ADDLIB`), and zero compiler-flag manipulation (`target_compile_options`, `-fsanitize`, `-Werror`), automating all four SC-010 prohibitions (inputs and imported-target usage only). Depends on T003.
- [ ] T016 [US3] Verify the module guarantees against contracts/ingestion-module.md and record X.4 evidence: the stamp-invalidation run (quickstart section 9: a build-type, compiler, or configure-argument change forces a visible full vendor rebuild, SC-008) and the missing-toolchain diagnostic (quickstart section 10: a `PATH` trim makes configure fail early naming `autoconf automake libtool patch` with apt/dnf/brew names, FR-018).
- [ ] T017 [US3] Verify `cmake/ImportAutotoolsSubmodule.cmake` against the contract with binary commands: the five documented inputs plus `PREGENERATED` and the optional `MERGE_INTO` are each passed by the hwloc call site (grep the single `import_autotools_submodule(` call for all five keywords, nonzero hits, FR-012); all platform-specific code sits inside `if(WIN32)` / `if(UNIX OR APPLE)` blocks and every `ExternalProject_Add`, `find_program`, and `file(` statement sits within a per-platform block (grep the module for those tokens outside the platform `if` guards, zero hits, FR-019); the unsupported-platform guard names `contrib/windows-cmake` (grep, nonzero).

**Checkpoint**: User Story 3 is verified by the command checks plus the User Story 1 build behavior.

---

## Phase 6: User Story 4 - Re-pinning hwloc is documented and auditable (Priority: P3)

**Goal**: One README section states the re-pin steps, and the pinned commit makes each pin auditable.

**Independent Test**: Follow the section on a scratch clone against a newer tag, then bump the tag while leaving the assertion untouched and confirm the build fails.

### Implementation for User Story 4

- [ ] T018 [US4] Add a re-pinning section to `README.md`: check out the new tag, commit the submodule pointer, bump the version assertion in `source/hwloc/hwloc_gate.cpp`. Add the host-toolchain note with apt, dnf, and brew package names (FR-006, R-011).
- [ ] T019 [US4] Rehearse the re-pin on a scratch clone against a newer tag per quickstart section 13: with all three steps the build goes green, and leaving the assertion bump out leaves the build red with the tripwire diagnostic (US4 scenarios 1 and 2, SC-006). Depends on T018.

**Checkpoint**: All user stories are independently verifiable.

---

## Phase 7: Polish and Cross-Cutting Concerns

**Purpose**: Whole-feature validation and gate confirmation.

- [ ] T020 [P] Run the full quickstart.md validation (sections 1 through 13) on Linux and record each binary verdict as X.4 evidence.
- [ ] T021 [P] Confirm every existing CI gate stays green with submodules fetched (`lint`, `coverage`, `sanitize`, `test`, `test-rocky`, `consumer-release`, `dbc-gate`, `prose-lint`, `docs`) and the `ci-macos` preset still configures and builds (FR-008, Principle VIII: no gate weakened; the Windows MSVC preset-build gate is suspended for this feature's lifetime by constitution amendment 2.7.0).
- [ ] T022 [P] Confirm `external/hwloc/COPYING` stays in-tree and is carried into the source distribution (FR-005). Verify with CPack: `cpack --config build/dev/CPackSourceConfig.cmake -G TGZ`, then `tar tzf speedgun-ng-*.tar.gz | grep external/hwloc/COPYING` returns a nonzero count and the packaged file matches `external/hwloc/COPYING` by `sha256sum`.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies, start immediately.
- **Foundational (Phase 2)**: Depends on Setup. Blocks every user story. Within it, the red-first proofs (T003, T004) precede the wrapper, module, and wiring (T005 through T007) they gate (Principle IX TDD ordering).
- **User Stories (Phases 3 through 6)**: All depend on Foundational.
  - US1 (P1) and US2 (P1) depend only on Foundational and run in parallel.
  - US3 (P2) depends on Foundational and the US1 build behavior.
  - US4 (P3) depends on Foundational.
- **Polish (Phase 7)**: Depends on all delivered user stories.

### Within Each User Story

- Tests are written and observed failing before the implementation that turns them green (TDD mode).
- The red-first proofs (T003, T004) precede the wrapper, module, and wiring (T005, T006, T007) they gate, and precede their CTest registration (T008).
- The consumer project files precede the CI job that drives them (T009, T010 before T014).

### Parallel Opportunities

- T002, T003, and T004 write separate files and run in parallel; T005 and T006 write separate files and run in parallel; T007 follows T005 and T006.
- T009 and T010 write separate consumer files and run in parallel.
- T011 through T014 all edit `.github/workflows/ci.yml` and run sequentially.
- After Foundational, US1 and US2 proceed in parallel across separate files; US4 proceeds independently.

---

## Parallel Example: Foundational (red-first proofs gate the code)

```bash
# Red-first seams, separate files, written before the code they gate:
Task: "tools/hwloc/hwloc_purity_scan.sh, failing on a seeded fixture line, then green (T003)"
Task: "tools/hwloc/hwloc_nm_proof.sh, red on a thin archive, then green after the merge (T004)"
# Then the wrapper (T005) and module (T006) in parallel, then the wiring/merge (T007),
# then registration in US1 (T008), which depends on T003 and T004.
```

## Parallel Example: User Story 2

```bash
# Consumer project, separate files:
Task: "test/consumer/CMakeLists.txt (T009)"
Task: "test/consumer/main.cpp (T010)"
# CI edits T011 through T014 are sequential (one file).
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Complete Phase 1: Setup.
2. Complete Phase 2: Foundational (blocks all stories), including the red-first proofs.
3. Complete Phase 3: User Story 1 (registration).
4. STOP and VALIDATE: build, `git submodule status`, quickstart section 3, and `ctest -R 'hwloc_nm_proof|hwloc_purity_scan'`.
5. Ship: the library carries a pinned, private, version-locked hwloc.

### Incremental Delivery

1. Setup + Foundational: the ingestion core works, and the red-first proofs pass green.
2. Add US1: ingestion is proven by the nm and purity proofs (MVP).
3. Add US2: the privacy contract is proven by the audits and the downstream consumer test.
4. Add US3: the reusable-module contract and guarantees are verified.
5. Add US4: re-pinning is documented and rehearsed.

### Parallel Team Strategy

1. One owner lands Setup + Foundational (the proofs, module, and wrapper are one cohesive change).
2. With Foundational done: one owner takes US1 (CTest registration), another takes US2 (consumer and CI audits), a third takes US4 (docs). US3 follows once US1 build behavior exists.

---

## Notes

- [P] means separate files with no dependency on an incomplete task.
- The `[USx]` label maps a task to a user story for traceability.
- Command fragments stay in code spans, matching Principle XI exemption for commands.
- Verify a red test fails against the pre-change baseline before implementing (X.4).
- The `.github/workflows/ci.yml` edits are sequential by necessity: one file.
- The vendored tree stays outside `include/`, `source/`, and `test/`, and outside every analysis, coverage, format, and spell gate.
