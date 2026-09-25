# Tasks: Vendor yaml-cpp as a Private, Pinned Submodule

**Input**: Design documents from `specs/006-vendor-yaml-cpp/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/build-integration.md](contracts/build-integration.md), [contracts/privacy-contract.md](contracts/privacy-contract.md), [quickstart.md](quickstart.md)

**Tests**: Included. The Test Plan in plan.md records TDD mode (Principle III): the two `tools/yaml/` scripts are the seam-level tests (seed a violation fixture, watch red, implement, green), and the CTest nm-archive proof is red until the merge argument lands.

**Organization**: Tasks are grouped by user story so each story can be implemented and tested independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = build with pinned vendored yaml-cpp; US2 = consumers cannot observe yaml-cpp; US3 = re-pinning documented
- Every task names its exact file path

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Bring the pinned sources into the tree

- [X] T001 Add the git submodule `external/yaml-cpp` from `https://github.com/jbeder/yaml-cpp.git` checked out at commit `56e3bb550c91fd7005566f19c079cb7a503223cf` (tag `yaml-cpp-0.9.0`), producing the `.gitmodules` record and the submodule gitlink; confirm `git submodule status external/yaml-cpp` records the SHA and the MIT `LICENSE` file is present in the submodule root (FR-001, FR-005; quickstart section 2). Do not modify any file inside the vendored tree.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The ingestion bracket, the wrapper TU, and the link/merge wiring. Everything the library needs before any story can be verified.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T002 Add `import_yaml_cpp()` to `CMakeLists.txt`, placed after the `import_hdrhistogram()` call site and modeled on the `import_simdjson`/`import_zlib` brackets: (a) guard: `FATAL_ERROR` naming `git submodule update --init external/yaml-cpp` when `external/yaml-cpp/CMakeLists.txt` is absent (FR-002); (b) version tripwire: `file(READ external/yaml-cpp/CMakeLists.txt)` plus regex `project\( *YAML_CPP[^\)]*VERSION ([0-9.]+)`, compare to the expected constant `0.9.0`, mismatch aborts with a `FATAL_ERROR` naming expected `0.9.0` and found, no git metadata consulted (FR-003, R-002); (c) scoped constants from the build-integration contract table: clear `CMAKE_CXX_FLAGS`, `CMAKE_COMPILE_WARNING_AS_ERROR OFF`, `CMAKE_CXX_FLAGS_SANITIZE ""`, `CMAKE_CXX_FLAGS_COVERAGE ""`, `CMAKE_CXX_CLANG_TIDY ""`, `CMAKE_CXX_CPPCHECK ""`, `CMAKE_CXX_VISIBILITY_PRESET hidden`, `CMAKE_VISIBILITY_INLINES_HIDDEN ON`, `CMAKE_POSITION_INDEPENDENT_CODE ON`, `CMAKE_POLICY_DEFAULT_CMP0077 NEW`, `BUILD_SHARED_LIBS OFF`, `YAML_BUILD_SHARED_LIBS OFF`, `YAML_CPP_BUILD_CONTRIB OFF`, `YAML_CPP_BUILD_TOOLS OFF`, `YAML_CPP_BUILD_TESTS OFF`, `YAML_CPP_INSTALL OFF`, `YAML_CPP_FORMAT_SOURCE OFF` (FR-009/FR-010/FR-011/FR-012/FR-017; R-004/R-005/R-006/R-007/R-008); (d) `add_subdirectory(external/yaml-cpp "${CMAKE_BINARY_DIR}/_yaml-cpp" EXCLUDE_FROM_ALL)` (R-012); (e) SYSTEM include promotion of the `yaml-cpp` interface include dirs inside the function scope (R-009). Leave `CMAKE_CXX_STANDARD` untouched (C++23 inherited). Carry the adjacent comments required by the build-integration contract section 4. Prohibited: any `find_package`/`pkg_check_modules` naming the FR-018 pattern, any PUBLIC/INTERFACE edge, any exclusion option (FR-004/FR-007/FR-013a).
- [X] T003 [P] Create `source/yaml/yaml_gate.cpp`: include `<yaml-cpp/yaml.h>` (SYSTEM-promoted via T002), hold `[[gnu::used]] [[maybe_unused]] constinit auto const link_proof = &YAML::Load;` as the object-level link proof (R-003), zero runtime lines, no casts; mirror the structure and sole-includer/link-proof comments of `source/hdrhistogram/hdrhistogram_gate.cpp` (FR-007, FR-014, SC-009).
- [X] T004 Wire the library in `CMakeLists.txt` (after T002, same file): register the gate TU with `target_sources(speedgun-ng_speedgun-ng PRIVATE source/yaml/yaml_gate.cpp)`; add `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:yaml-cpp::yaml-cpp>)`: PRIVATE only, BUILD_INTERFACE-wrapped, no PUBLIC edge (FR-007). TDD order (IX, Principle III): with the gate TU registered but the merge argument still absent, run `nm -C build/dev/libspeedgun-ng.a | grep -c 'YAML::'`, record the red verdict (zero merged members), then extend the existing `vendored_archive_merge` call to `vendored_archive_merge(speedgun-ng_speedgun-ng hdr_histogram_static zlibstatic yaml-cpp)` (R-010) and rerun green. No change to `cmake/VendoredArchiveMerge.cmake` (already variadic).

**Checkpoint**: Foundation ready: configure succeeds with the pinned tree, the static archive builds, and the guard/tripwire diagnostics fire on demand. Story verification begins.

---

## Phase 3: User Story 1: The project builds with a pinned, vendored yaml-cpp (Priority: P1) 🎯 MVP

**Goal**: A clean clone with submodules builds on Linux and macOS; yaml-cpp compiles from the pinned submodule into the static library, is never resolved from the system, and a version-declaring revision other than 0.9.0 fails configure with a readable diagnostic.

**Independent Test**: `cmake --preset=dev` / `cmake --build --preset=dev` / `ctest --preset=dev` plus `git submodule status`, the tripwire run (quickstart section 3), the pristine-worktree check (section 4), `ctest -R yaml_nm_proof` (section 5), and the uninitialized-submodule guard (section 11). All run against a build satisfying this story alone.

### Tests for User Story 1 ⚠️

> Write these FIRST: seed a violation fixture, watch red, implement, green (TDD mode recorded in the Test Plan).

- [X] T005 [P] [US1] Create `tools/yaml/yaml_purity_scan.sh`, modeled on `tools/simdjson/simdjson_purity_scan.sh`: (a) zero `find_package\( *yaml[-_]?cpp` or `pkg_check_modules\([^)]*yaml[-_]?cpp` calls (case-insensitive; the `pkg_check_modules` module name is not the first argument, so the name may follow the prefix and any `REQUIRED`/`QUIET` keyword) in `CMakeLists.txt`/`*.cmake` outside `external/`, `build/`, `prefix*`, `.git/`, `.specify/`, `.omo/` (FR-004, SC-008); (b) zero `yaml[-_]?cpp` references under `include/` (FR-014). The script file itself sits outside the scan set (documented self-match property).
- [X] T006 [P] [US1] Create `tools/yaml/yaml_nm_proof.sh`, modeled on `tools/zlib/zlib_nm_proof.sh`, three facts from one `nm --format=bsd` pass over the built `libspeedgun-ng.a`: (A) more than zero defined `T`/`t` symbols containing `4YAML` from members not named `yaml_gate`; (B) a `*yaml_gate*` member carries an undefined `U` reference matching `_ZN4YAML4Load`; (C) a defined `_ZN4YAML4Load` symbol lives in a different member, resolving the gate reference in-archive (FR-007, SC-009, R-003/R-010). Takes source-dir and build-dir arguments like the zlib proof.

### Implementation for User Story 1

- [X] T007 [US1] Register both scripts in `test/CMakeLists.txt` beside the simdjson/hdrhistogram/zlib pairs: `add_test(NAME yaml_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/yaml/yaml_purity_scan.sh ${CMAKE_SOURCE_DIR})` and `add_test(NAME yaml_nm_proof COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/yaml/yaml_nm_proof.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})` (depends on T005, T006).

**Checkpoint**: US1 fully functional: clean-clone build green, `ctest -R yaml_` green, tripwire and guard diagnostics proven (quickstart sections 1–5, 10, 11, 12).

---

## Phase 4: User Story 2: Consumers cannot observe yaml-cpp (Priority: P1)

**Goal**: Install-tree, package files, shared-library symbol tables, runtime dependencies, and a downstream consumer run on a machine carrying yaml-cpp are all provably free of yaml-cpp (pattern `yaml[-_]?cpp`, plus `4YAML` on symbol dumps).

**Independent Test**: Install, audit the prefix (`find prefix/ -iname '*yaml*cpp*'`, package-file grep), shared build with `nm -D`/`ldd` audits, and the reused `test/consumer/` configure/build/run on a machine with a system yaml-cpp (quickstart sections 6, 7, 8, 9). Runs against any build satisfying US1.

### Implementation for User Story 2

- [X] T008 [US2] Add the audit steps to `.github/workflows/ci.yml` per research R-011: (a) `test` job after install: `find prefix/ -iname '*yaml*cpp*'` must be empty (FR-015, SC-002) and `grep -riE 'yaml[-_]?cpp' prefix/lib/cmake/speedgun-ng/*.cmake` must be empty (FR-016, SC-003); (b) `shared-audit` job: `nm -D --defined-only` greps for `yaml[-_]?cpp` and for `4YAML` both empty, `ldd` names no yaml-cpp object (FR-017, SC-004); (c) `downstream-consumer` job: a premise step building the vendored tree out-of-tree into `/usr/local` mirroring the existing `sys-hdr` step (`cmake -S external/yaml-cpp -B sys-yaml -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF && cmake --build sys-yaml && sudo cmake --install sys-yaml --prefix /usr/local`), then greps the consumer configure and build logs with `yaml[-_]?cpp` for zero matches (FR-019, SC-005). Checkout steps already carry `submodules: true`; no new host toolchain; no existing step weakened.
- [X] T009 [US2] Add one `elif grep -q 'yaml-cpp'` classifier branch to `tools/dbc/dependency_scan.sh`, beside the `hwloc_vendor`/`simdjson`/`hdr_histogram`/`zlibstatic` branches, classifying the `PRIVATE` link on the library target as vendored-private so `dbc_dependency_scan` stays green (R-011, FR-004).
- [X] T010 [US2] Verify the privacy surfaces end to end on the local machine per quickstart sections 6–9 and 12: install-tree audit, package-file audit, shared-audit (`nm -D` × 2, `ldd`), forced-static proof under a shared parent (`find build/shared-audit -iname '*yaml*' -name '*.so*'` empty), and the downstream consumer run in `test/consumer/` with a system yaml-cpp present: all zero-hit / exit-0 verdicts (FR-012, FR-015, FR-016, FR-017, FR-019; SC-002 through SC-005).

**Checkpoint**: US1 and US2 both work: the install tree, package files, shared symbols, and consumer are yaml-cpp-free with command evidence.

---

## Phase 5: User Story 3: Re-pinning yaml-cpp is documented and auditable (Priority: P3)

**Goal**: One short README section states the three re-pinning steps; the tripwire enforces the version-assertion bump; the pinned SHA recorded in the spec keeps the pin auditable.

**Independent Test**: Follow the README section on a scratch clone against a different release tag (e.g. `yaml-cpp-0.8.0`): build green after the bump; move the tag with the constant untouched: configure fails at the tripwire (quickstart section 13).

### Implementation for User Story 3

- [X] T011 [US3] Add a `## Re-pinning yaml-cpp` section to `README.md`, beside the existing four re-pinning sections: check out the new tag (`git -C external/yaml-cpp checkout <tag>`), commit the submodule pointer, bump the expected-version constant in the `import_yaml_cpp` bracket of `CMakeLists.txt`; state that the tripwire is configure-time (a version-string read of the submodule's `project()` line) because 0.9.0 exposes no version macro, unlike the simdjson/HdrHistogram_c compile-time asserts (FR-006; the module-documentation duty of the build-integration contract).
- [X] T012 [US3] Run the re-pin rehearsal per quickstart section 13 on a scratch clone: checkout `yaml-cpp-0.8.0`, bump the bracket constant to `0.8.0`, build green; restore to `yaml-cpp-0.9.0` with the constant restored and confirm configure fails before the bump is applied (SC-006). Tear down per quickstart Teardown.

**Checkpoint**: All three stories independently functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T013 Run the full local gate suite green: `cmake --preset=dev`, `cmake --build --preset=dev`, `ctest --preset=dev` (including the two new `yaml_*` tests), and `cmake -P cmake/prose-lint.cmake` for the README/spec prose changes (FR-008; Principle VIII, XI). On a macOS developer machine, `cmake --preset=ci-macos` configures and `cmake --build --preset=ci-macos` builds green (SC-001 macOS clause; developer-local per the spec Assumptions).
- [X] T014 Confirm the coverage and sanitizer exclusions per quickstart section 10: `grep -e '-fsanitize' -e '--coverage' build/dev/_yaml-cpp/CMakeFiles/yaml-cpp.dir/flags.make build/sanitize/_yaml-cpp/CMakeFiles/yaml-cpp.dir/flags.make` exits 1 with no output, and the coverage trace lists `source/yaml/yaml_gate.cpp` with zero runtime lines (FR-009, FR-010; Test Plan coverage strategy).
- [X] T015 Verify `git status` reports `external/yaml-cpp` unmodified after the full build series (FR-013, SC-007), and that no option, preset, or cache variable excludes any of the five vendored dependencies (FR-013a).
- [X] T016 Final privacy-contract sweep: re-run quickstart sections 2–9 and 11–13 verdicts against the final HEAD state; record each command and its binary verdict as the change's evidence (X.4).

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies: start immediately.
- **Foundational (Phase 2)**: Depends on T001 (the submodule must exist to be guarded, tripwired, and added). T002 and T003 touch different files and may run in parallel; T004 follows T002 (same file) and needs T003 to exist at build time. **Blocks all user stories.**
- **User Story 1 (Phase 3)**: Depends on Phase 2 (the scripts audit and prove the Phase-2 artifacts). T005/T006 parallel; T007 follows them.
- **User Story 2 (Phase 4)**: Depends on Phase 2 (needs an installable, shared-buildable library); the US1 nm proof should be green first since T010's evidence includes the merged archive. T008 and T009 touch different files and may run in parallel; T010 follows both.
- **User Story 3 (Phase 5)**: Depends on T002 (the constant the README documents and the rehearsal bumps).
- **Polish (Phase 6)**: Depends on all delivered stories.

### Within Each User Story

- Scripts (tests) written and run red before their wiring is trusted green (TDD mode, Principle III)
- Implementation before verification tasks
- Story checkpoint before moving to the next phase

### Parallel Opportunities

- T002 ∥ T003 (different files)
- T005 ∥ T006 (different files)
- T008 ∥ T009 (different files)
- US3's T011 (README) may run in parallel with any US2 task (different file) once T002 lands

---

## Parallel Example: Foundational + US1 tests

```bash
# After T001 lands, launch together:
Task: "T002 import_yaml_cpp() bracket in CMakeLists.txt"
Task: "T003 gate TU in source/yaml/yaml_gate.cpp"

# Once the bracket builds, launch the test pair together:
Task: "T005 yaml_purity_scan.sh in tools/yaml/"
Task: "T006 yaml_nm_proof.sh in tools/yaml/"

# US2 implementation pair:
Task: "T008 CI audit steps in .github/workflows/ci.yml"
Task: "T009 classifier branch in tools/dbc/dependency_scan.sh"
```

---

## Implementation Strategy

### MVP First (User Story 1 + User Story 2 are both P1)

1. Complete Phase 1: submodule pinned.
2. Complete Phase 2: bracket, gate TU, link + merge (CRITICAL, blocks everything).
3. Complete Phase 3: purity scan + nm proof green → the dependency is ingested, pinned, proven present-but-private at archive level.
4. Complete Phase 4: privacy audits green → the invisibility contract holds. MVP = the full P1 pair; US1 alone is not shippable because a leaking artifact becomes a breaking change to remove later (spec: Why this priority).
5. **STOP and VALIDATE** at each checkpoint.

### Incremental Delivery

1. Setup + Foundational → dependency ingested.
2. US1 → build/pin/presence proven (checkpoint).
3. US2 → privacy proven (MVP complete).
4. US3 → re-pinning documented; cheap maintenance path.
5. Polish → full gate evidence for the PR.

---

## Notes

- [P] tasks = different files, no dependencies; T002/T004 share `CMakeLists.txt` and never run in parallel.
- The FR-018 audit pattern is `yaml[-_]?cpp` (case-insensitive); symbol audits add the `4YAML` namespace marker.
- Prohibited everywhere (build-integration contract section 1): system-discovery calls, PUBLIC/INTERFACE edges to `yaml-cpp`, exclusion options, in-tree generation, `--exclude-libs` for yaml-cpp.
- Verify the tripwire red (quickstart section 3) before trusting the bracket; the gate TU must keep zero runtime lines (coverage stays accurate).
- Commit after each task or logical group; `<Section>: <one-line imperative>` per the constitution's PR template; `Refs: specs/006-vendor-yaml-cpp`.
- Avoid: touching `CMakePresets.json`, `.codespellrc`, or `cmake/VendoredArchiveMerge.cmake`, all verified to need no change.
