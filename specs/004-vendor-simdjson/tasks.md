---
description: "Task list for 004-vendor-simdjson: vendor simdjson as a private, pinned submodule"
---

# Tasks: Vendor simdjson as a Private, Pinned Submodule

**Input**: Design documents from `specs/004-vendor-simdjson/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/build-integration.md](contracts/build-integration.md), [contracts/privacy-contract.md](contracts/privacy-contract.md), [quickstart.md](quickstart.md)

**Tests**: TDD is recorded in the plan Test Plan (Principle III). The two audit scripts follow red-before/green-after discipline: the purity scan against a seeded violation fixture, the nm proof against the thin (un-merged) archive. The bracket's red-green runs at the surface level per the Test Plan (quickstart sections 3, 4, 5, 11).

**Organization**: Tasks are grouped by user story so each story can be implemented and tested independently. US1 and US2 are both Priority P1 in spec.md; US1 (the vendored, pinned build) is the foundation US2 audits, so deliver US1 first as the MVP.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 / US2 / US3, mapping to spec.md user stories
- Exact file paths in every description

Paths are repository-relative (`CMakeLists.txt`, `source/`, `tools/`, `test/`); the vendored tree lives at `external/simdjson` (never `third_party/`).

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Bring the pinned vendored tree into the repository

- [X] T001 Register the simdjson submodule: `git submodule add https://github.com/simdjson/simdjson.git external/simdjson`, check out pinned commit `f5de14f09256982933af2849beb43778bd421ca7` (annotated tag `v4.6.11`, tag object `e153ffadd9ae29b00c90bedc76f65d25a993d2b5`), commit the `.gitmodules` entry and gitlink; verify `git submodule status external/simdjson` reports exactly that SHA (FR-001; do NOT edit `.codespellrc`, `*/external` already skips it per specs/003)

**Checkpoint**: `external/simdjson` exists at the pinned SHA with MIT and Apache-2.0 license files in-tree (FR-005).

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The `add_subdirectory` bracket and the two gate-configuration edits. Every exemption is explicit because a CMake child inherits the parent's flags, unlike the hwloc autotools child (research R-006).

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T002 Add the `import_simdjson()` bracket to `CMakeLists.txt`, beside the existing hwloc block (after line 88): a function containing the configure guard (`if(NOT EXISTS .../external/simdjson/CMakeLists.txt)` → `FATAL_ERROR` naming `git submodule update --init external/simdjson`), the scoped resets (`CMAKE_CXX_FLAGS ""`, `CMAKE_CXX_FLAGS_SANITIZE ""`, `CMAKE_CXX_FLAGS_COVERAGE ""`, `CMAKE_CXX_CLANG_TIDY ""`, `CMAKE_CXX_CPPCHECK ""`, `CMAKE_CXX_VISIBILITY_PRESET hidden`, `BUILD_SHARED_LIBS OFF`, `SIMDJSON_INSTALL OFF`: leave `SIMDJSON_DEVELOPER_MODE` unset), then `add_subdirectory(external/simdjson "${CMAKE_BINARY_DIR}/_simdjson" EXCLUDE_FROM_ALL)`, then promote the interface include to `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`; invoke it once and call adjacent comment per contract section 4 (inheritance asymmetry R-006, re-pinning pointer to README) (FR-002, FR-004, FR-009, FR-010, FR-011, FR-014, FR-016; R-004 through R-008)
- [X] T003 [P] Narrow the clang-tidy header-filter in `CMakePresets.json` line 43 (`CMAKE_CXX_CLANG_TIDY` cache entry `clang-tidy;--header-filter=^${sourceDir}/`) so it does not match `external/`, keeping vendored headers reached through the wrapper out of tidy findings while speedgun-ng code stays fully analyzed (FR-009, R-008)
- [X] T004 [P] Extend the vendored-private classifier in `tools/dbc/dependency_scan.sh` (the `hwloc_vendor` branch at lines 94-97) with a matching branch for a `simdjson` PRIVATE link on `speedgun-ng_speedgun-ng`, note citing specs/004 FR-007 privacy A2/A3/A6, so `dbc_dependency_scan` classifies the coming link as vendored-private instead of `library-runtime` (R-010)
- [X] T005 Validate the foundation: `cmake --preset=dev && cmake --build --preset=dev -j` exits 0; `find <binary-dir>/_simdjson -name 'libsimdjson.so*'` prints nothing (forced static, quickstart section 9); `git -C external/simdjson status --porcelain` is empty (SC-007); no simdjson file lands under any install prefix (`SIMDJSON_INSTALL OFF`, quickstart section 6 dry check)

**Checkpoint**: Foundation ready: `simdjson::simdjson` exists as a static, hidden, uninstrumented, install-off target; user stories can begin.

---

## Phase 3: User Story 1 - The project builds with a pinned, vendored simdjson (Priority: P1) 🎯 MVP

**Goal**: The speedgun-ng library compiles the wrapper TU `source/simdjson/simdjson_gate.cpp`, links simdjson privately, merges the vendored archive for static builds, and the version tripwire plus the purity/nm audits all hold.

**Independent Test**: Clone with submodules, build, inspect `git submodule status`, swap the submodule tag to fire the assertion, run `ctest -R 'simdjson_nm_proof|simdjson_purity_scan'`, configure a sanitize build, configure an uninitialized clone (quickstart sections 2, 3, 4, 5, 10, 11, 12: every check satisfies US1 alone).

### Implementation for User Story 1

- [X] T006 [P] [US1] TDD-create `tools/simdjson/simdjson_purity_scan.sh`, modeled on `tools/hwloc/hwloc_purity_scan.sh`: audit A7 (zero `find_package(simdjson` / `pkg_check_modules(simdjson`, case-insensitive, across `CMakeLists.txt`/`*.cmake` excluding `external/`, `build/`, `.git/`) and A8 (zero `simdjson` under `include/`); print every hit as `path:line: text`; exit 1 on any hit. RED first against a seeded violation fixture line, GREEN against the clean tree (TDD per plan Test Plan; FR-004, FR-013, SC-008; privacy contract A7/A8). The script never scans itself
- [X] T007 [US1] Create `source/simdjson/simdjson_gate.cpp` following `source/hwloc/hwloc_gate.cpp`: header comment (sole `<simdjson.h>` includer, version tripwire, link proof, README re-pinning pointer), `#include <simdjson.h>`, `static_assert(simdjson::SIMDJSON_VERSION_MAJOR == 4 && simdjson::SIMDJSON_VERSION_MINOR == 6 && simdjson::SIMDJSON_VERSION_REVISION == 11, "...expected simdjson 4.6.11...")`, and `[[maybe_unused]] constinit` function-pointer reference to `&simdjson::get_active_implementation` guarded by `#if defined(__GNUC__) [[gnu::used]]` (R-002, R-003: enum constants; `#if` reads macros only, cannot see them; zero runtime lines, no cast); register it with `target_sources(speedgun-ng_speedgun-ng PRIVATE source/simdjson/simdjson_gate.cpp)` and `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>)` in `CMakeLists.txt` beside the hwloc link (FR-003, FR-007, FR-013)
- [X] T008 [US1] TDD-create `tools/simdjson/simdjson_nm_proof.sh`, modeled on `tools/hwloc/hwloc_nm_proof.sh`, adapted for no symbol prefix: match the mangled/demangled substring `simdjson`: (A) vendored members present in `libspeedgun-ng.a`, (B) a `simdjson_gate` member carries an UNDEFINED `get_active_implementation` reference, (C) a defined `get_active_implementation` resolves in a different member; register `simdjson_purity_scan` and `simdjson_nm_proof` as CTest tests in `test/CMakeLists.txt`, same `add_test(NAME ... COMMAND bash ... ${CMAKE_SOURCE_DIR} ...)` pattern as the hwloc pair at lines 76-77. Run against the thin (un-merged) archive: expected RED: this is the recorded TDD red proving the merge's necessity (FR-007, SC-009; privacy contract A6; R-009)
- [X] T009 [US1] Add the post-build static-merge step to `CMakeLists.txt`: when `BUILD_SHARED_LIBS` is OFF, merge `$<TARGET_FILE:simdjson>` into `libspeedgun-ng.a` via `ar -M` with `ADDLIB` (`llvm-ar -M` on current macOS), mirroring the hwloc `MERGE_INTO` result (R-009: a thin archive leaves the gate's reference unresolved for consumers and cannot pass SC-009); rerun `ctest -R simdjson_nm_proof`: GREEN (SC-009, FR-007)
- [X] T010 [US1] Run the US1 validation set from `specs/004-vendor-simdjson/quickstart.md` and capture binary verdicts (Principle X.4): section 2 (pinned SHA), section 3 (tripwire fires on `v4.6.4`, names expected 4.6.11 and the enum values, restores green), section 5 (`ctest -R 'simdjson_nm_proof|simdjson_purity_scan'` green), section 10 (`ci-sanitize` build log: zero simdjson compiles carry `-fsanitize`, sanitize suite green), section 11 (clone without `--recurse-submodules`: configure aborts naming `git submodule update --init external/simdjson`, no system fallback), section 12 (system `libsimdjson-dev` installed: configure performs zero discovery) (FR-002, FR-003, FR-004, FR-008, FR-010, SC-006, SC-007, SC-008, SC-009)

**Checkpoint**: US1 fully functional: pinned build, tripwire, link proof, purity, sanitizer exclusion, guard, all proven independently.

---

## Phase 4: User Story 2 - Consumers cannot observe simdjson (Priority: P1)

**Goal**: Install-tree, package-file, shared-symbol, runtime-dependency, and downstream-consumer audits all prove simdjson invisible, wired into the existing CI jobs (R-010).

**Independent Test**: Install, audit prefix tree and package files, build shared and audit `.dynsym` + `ldd`, then run the reused `test/consumer/` project on a machine carrying simdjson and grep its logs (quickstart sections 6, 7, 8: runs against any US1 build).

### Implementation for User Story 2

- [X] T011 [US2] Add two steps to the `test` job in `.github/workflows/ci.yml`, beside the hwloc audits (lines 121-124): install-tree audit `test -z "$(find prefix/ -iname '*simdjson*')"` (A1, SC-002) and package-file audit `! grep -qi simdjson prefix/lib/cmake/speedgun-ng/*.cmake` (A2/A3, SC-003) (FR-014, FR-015)
- [X] T012 [US2] Add two steps to the `shared-audit` job in `.github/workflows/ci.yml`, beside the hwloc steps (near line 173): `! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -i simdjson` (A4, SC-004) and `! ldd prefix-shared/lib/libspeedgun-ng.so | grep -i simdjson` (A5, SC-004; R-005 forced static means no `libsimdjson` object exists to name) (FR-016)
- [X] T013 [US2] Extend the `downstream-consumer` job in `.github/workflows/ci.yml`: add `libsimdjson-dev` to its apt-get install line so the machine carries simdjson, and add a step `test -z "$(grep -i simdjson consumer-configure.log consumer-build.log)"` beside the hwloc log audit (near line 202); `test/consumer/` is reused unchanged: it never names simdjson, its success plus the grep is the authoritative privacy proof (FR-017, SC-005)
- [X] T014 [US2] Run the US2 validation set from `specs/004-vendor-simdjson/quickstart.md`: section 6 (install tree and package files print nothing), section 7 (shared build: `nm -D`, `ldd`, `find` all exit 1 with empty output), section 8 (consumer with `libsimdjson-dev` installed: configure/build/run exit 0, log greps zero) (SC-002, SC-003, SC-004, SC-005)

**Checkpoint**: US1 and US2 both functional: the privacy contract holds on every consumer-observable surface, proven by audits.

---

## Phase 5: User Story 3 - Re-pinning simdjson is documented and auditable (Priority: P3)

**Goal**: One short README section states the re-pinning steps; the pinned commit in spec.md makes each pin auditable.

**Independent Test**: Follow the README section on a scratch clone against a newer tag (green); move the tag without the assertion bump (red) (quickstart section 13).

### Implementation for User Story 3

- [X] T015 [US3] Add a "Re-pinning simdjson" section to `README.md`, beside the existing hwloc section: check out the new tag (`git -C external/simdjson checkout <tag>`), commit the submodule pointer, bump the version assertion in `source/simdjson/simdjson_gate.cpp`; state that the build fails until the assertion matches; keep the prose prose-gate clean (`cmake -P cmake/prose-lint.cmake`, Principle XI) (FR-006)
- [X] T016 [US3] Run the re-pin rehearsal from `specs/004-vendor-simdjson/quickstart.md` section 13 on a scratch clone: with all three steps the build goes green; with the tag moved and the assertion untouched the build is red with the section-3 tripwire diagnostic; restore the pin afterwards (US3 scenarios 1-2)

**Checkpoint**: All user stories independently functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T017 Run `cmake --preset=dev`, `cmake --build --preset=dev -j`, `ctest --preset=dev --output-on-failure`, and the full `specs/004-vendor-simdjson/quickstart.md` sections 0 through 13 end to end; record every binary verdict (X.4); tear down scenario artifacts per quickstart Teardown
- [X] T018 [P] Verify the gate set is unweakened (Principle VIII): coverage preset green with the coverage trace listing `source/simdjson/simdjson_gate.cpp` at 0 executable lines (plan Test Plan); `dbc_dependency_scan` and `hwloc_*` CTest green unchanged; `cmake -P cmake/prose-lint.cmake` green over the PR range
- [ ] T019 [P] Verify macOS developer path: `cmake --preset=ci-macos` configures, builds, and the merge step uses `llvm-ar -M` (developer-local, FR-008; no CI runner)
  - Status 2026-09-23: open, needs an Apple host. The merge script drives `${CMAKE_AR}` with an MRI script, the identical mechanism the shipped hwloc module uses on macOS; the developer run closes this task.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies: start immediately
- **Foundational (Phase 2)**: T002 depends on T001; T003 and T004 are independent of T002 (parallel); T005 depends on T002/T003/T004. BLOCKS all user stories
- **User Stories**: all depend on the Foundational checkpoint
  - US1 (Phase 3): sequential T007 → T008 → T009 (wrapper before nm proof, proof red before merge green); T006 [P] parallel with anything (script-only, different file); T010 last within US1
  - US2 (Phase 4): depends on US1 (needs the built, installable library); T011/T012/T013 all edit `ci.yml`: sequential; T014 last
  - US3 (Phase 5): depends on US1 (T007 defines the assertion the README names); T016 after T015
- **Polish (Phase 6)**: depends on all delivered stories; T018/T019 [P] parallel with T017

### User Story Dependencies

- **US1 (P1)**: after Foundational; no dependency on other stories: this is the MVP
- **US2 (P1)**: after US1 (audits run against the US1 build's install tree); independently testable per spec
- **US3 (P3)**: after US1 (assertion file must exist); independently testable

### Within Each User Story

- TDD red before implementation green (purity fixture; nm thin-archive red)
- Wrapper TU before nm proof; nm proof (red) before the merge (green)
- CI wiring before the local validation task that mirrors it

### Parallel Opportunities

- T003, T004 while T002/T005 run (different files)
- T006 any time in US1 (own file, no build dependency)
- T018, T019 in parallel with T017
- After Foundational, US1 is the only unblocked story (US2 needs US1's build, US3 needs US1's assertion): sequential delivery US1 → US2 → US3 matches the spec's priority reading of "both P1, foundation first"

---

## Parallel Example: Foundational + US1 start

```bash
# While T002 (bracket, CMakeLists.txt) is in progress:
Task: "T003 Narrow tidy header-filter in CMakePresets.json"
Task: "T004 Add simdjson classifier in tools/dbc/dependency_scan.sh"
Task: "T006 [US1] TDD purity scan script in tools/simdjson/simdjson_purity_scan.sh"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Phase 1: submodule at `f5de14f0...`
2. Phase 2: bracket + filter + classifier + foundation checkpoint (CRITICAL: blocks everything)
3. Phase 3: wrapper TU, nm proof (red), merge (green), US1 validation
4. **STOP and VALIDATE**: US1 standalone per quickstart sections 2-5, 10-12
5. Deliver: the pinned, vendored, tripwired build is the MVP

### Incremental Delivery

1. MVP above → the dependency is ingested, version-locked, proven
2. Add US2 CI audits → privacy contract proven on every surface → deliver
3. Add US3 README section + rehearsal → re-pinning cheap and traceable → deliver
4. Each story adds value without touching previous stories' files beyond the shared `ci.yml`

### Notes for the Implementer

- The bracket is a function scope: target-creating commands persist out of it, variable resets do not (R-006). `SIMDJSON_DEVELOPER_MODE` stays unset: declaring it adds a cache entry for an already-effective value (R-004).
- The audit pattern is the bare substring `simdjson` (case-insensitive): mangled symbols `_ZN8simdjson...`, discovery calls, and `include/` cleanliness all read on one pattern (privacy contract §1).
- No `find_package(simdjson`, no PUBLIC edge, no `--exclude-libs` (rejected: R-007), no `.codespellrc` edit (already covered).
- Commit after each task or logical group; `<Section>: <one-line imperative>` per the constitution PR template.

---

## Phase 7: Convergence

- [X] T020 Correct the simdjson pin provenance wording: tag `v4.6.11` is an annotated tag, so `e153ffadd9ae29b00c90bedc76f65d25a993d2b5` is the tag-object SHA while the pinned commit the gitlink records is `f5de14f09256982933af2849beb43778bd421ca7` (`git rev-parse v4.6.11^{commit}`); restate FR-001, US1/AC3, and the spec "Pinned commit provenance" assumption to name `f5de14f0…` as the recorded commit, delete the false "lightweight tag naming the commit directly" claim (an annotated tag object cannot be a gitlink), and update the expected `git submodule status` SHA in quickstart section 2 to `f5de14f0…`. Documentation only: the submodule is already correctly pinned at `f5de14f0` (v4.6.11), the version tripwire (FR-003) and every audit pass on it; no code, build, or submodule change. per FR-001 / US1-AC3 / spec Assumptions "Pinned commit provenance" (contradicts)
  - Status 2026-09-23: done. Provenance wording corrected across spec.md, plan.md, research.md, data-model.md, quickstart.md, and this tasks.md: the recorded/pinned commit now reads `f5de14f0…` everywhere, with `e153ffad…` kept only as the annotated-tag object. `git ls-remote` and `git submodule status` confirm the split (`v4.6.11` tag object `e153ffad`, commit `f5de14f0`). No code, build, or submodule change.
