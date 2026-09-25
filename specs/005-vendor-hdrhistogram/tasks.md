---
description: "Task list for 005-vendor-hdrhistogram: vendor HdrHistogram_c (plus companion zlib) as private, pinned submodules"
---

# Tasks: Vendor HdrHistogram_c as a Private, Pinned Submodule

**Input**: Design documents from `specs/005-vendor-hdrhistogram/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/build-integration.md](contracts/build-integration.md), [contracts/privacy-contract.md](contracts/privacy-contract.md), [quickstart.md](quickstart.md)

**Tests**: TDD is recorded in the plan Test Plan (Principle III). The four audit scripts follow red-before/green-after discipline: the purity scans against a seeded violation fixture, the nm proofs against the thin (un-merged) archive. The two brackets and the redirect run their red-green at the surface level against the pre-change tree (quickstart sections 3, 4, 5, 6, 9, 12), captured per X.4. Section 6 is the install()-override's red-until-implemented gate.

**Organization**: Tasks are grouped by user story so each story can be implemented and tested independently. US1 and US2 are both Priority P1 in spec.md; US1 (the vendored, pinned build) is the foundation US2 audits, so deliver US1 first as the MVP. The companion zlib (Fixed decision 6) is inseparable from US1: HdrHistogram_c's logging component requires it, so every US1 surface covers both trees.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 / US2 / US3, mapping to spec.md user stories
- Exact file paths in every description

Paths are repository-relative (`CMakeLists.txt`, `source/`, `tools/`, `test/`); the vendored trees live at `external/hdrhistogram_c` and `external/zlib` (never `third_party/`). Audit patterns are fixed by FR-019: `hdr[-_]?histogram` (case-insensitive) for HdrHistogram_c, `zlib`/`libz` for zlib, plus the `hdr_` symbol-prefix flag and the bare `inflate`/`deflate`/`compress`/`uncompress` names on shared builds.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Bring the two pinned vendored trees into the repository

- [X] T001 Register the HdrHistogram_c submodule: `git submodule add https://github.com/hdrhistogram/HdrHistogram_c.git external/hdrhistogram_c`, check out pinned commit `18c7a324383dded1451d15621cd018b0048057d0` (lightweight tag `0.11.10` naming the commit directly), commit the `.gitmodules` entry and gitlink; verify `git submodule status external/hdrhistogram_c` reports exactly that SHA; its MIT `LICENSE.txt` and `COPYING.txt` stay in-tree (FR-001, FR-005)
- [X] T002 Register the zlib submodule: `git submodule add https://github.com/madler/zlib.git external/zlib`, check out pinned commit `da607da739fa6047df13e66a2af6b8bec7c2a498` (annotated tag `v1.3.2` peeling to that commit), commit the `.gitmodules` entry and gitlink; verify `git submodule status external/zlib` reports exactly that SHA; its `LICENSE` stays in-tree (FR-001a, FR-005)

**Checkpoint**: both `external/` trees exist at their pinned SHAs with license files in-tree.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The two scoped `add_subdirectory` brackets and the zlib redirect between them in `CMakeLists.txt`, the generalized archive-merge module, and the dependency-classifier extension. Every exemption is explicit because a CMake child inherits the parent's flags, unlike the hwloc autotools child (R-011). Ordering is load-bearing: the zlib bracket and its redirect must run before the HdrHistogram_c bracket (R-008).

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T003 Add the `import_zlib()` bracket to `CMakeLists.txt`, beside the existing hwloc/simdjson blocks (before them, so the redirect lands before the HdrHistogram_c bracket per R-008 ordering): a function containing the configure guard (`if(NOT EXISTS .../external/zlib/CMakeLists.txt)` → `FATAL_ERROR` naming `git submodule update --init external/zlib`), the scoped resets (`CMAKE_C_FLAGS ""`, `CMAKE_CXX_FLAGS ""`, `CMAKE_C_FLAGS_SANITIZE ""`, `CMAKE_C_FLAGS_COVERAGE ""`, `CMAKE_C_CLANG_TIDY ""`, `CMAKE_C_CPPCHECK ""`, `CMAKE_C_VISIBILITY_PRESET hidden`, `CMAKE_POSITION_INDEPENDENT_CODE ON`, `BUILD_SHARED_LIBS OFF`, `ZLIB_BUILD_SHARED OFF`, `ZLIB_BUILD_STATIC ON`, `ZLIB_INSTALL OFF`, `ZLIB_BUILD_TESTING OFF`, `ZLIB_PREFIX ON`), then `add_subdirectory(external/zlib "${CMAKE_BINARY_DIR}/_zlib" EXCLUDE_FROM_ALL)`, then promote `zlibstatic`'s interface include to `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`; invoke it once; adjacent comment per contract section 4 (inheritance asymmetry, ordering, re-pinning pointer to README) (FR-002, FR-009, FR-010, FR-011, FR-012, FR-021, FR-022; R-004, R-007, R-009, R-010, R-011, R-012)
- [X] T004 Add the zlib redirect to `CMakeLists.txt` immediately after the `import_zlib()` invocation, at parent scope: `if(NOT TARGET ZLIB::ZLIB)` → `add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)` with `INTERFACE_LINK_LIBRARIES zlibstatic`; then force the `FindZLIB` cache to the vendored copy: `set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}" CACHE PATH "" FORCE)`, `set(ZLIB_LIBRARY "${zlib_BINARY_DIR}/libz${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE FILEPATH "" FORCE)`, `set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)`; comment records why the alias is insufficient (directory-scoped, absent when `ZLIB_BUILD_SHARED OFF`) and that FindZLIB never probes the host (FR-020; R-008)
- [X] T005 Add the `import_hdrhistogram()` bracket to `CMakeLists.txt`, after the redirect: a function containing the configure guard (same shape, naming `git submodule update --init external/hdrhistogram_c`), the scoped resets (`CMAKE_C_FLAGS ""`, `CMAKE_CXX_FLAGS ""`, `CMAKE_C_FLAGS_SANITIZE ""`, `CMAKE_C_FLAGS_COVERAGE ""`, `CMAKE_C_CLANG_TIDY ""`, `CMAKE_C_CPPCHECK ""`, `CMAKE_C_VISIBILITY_PRESET hidden`, `CMAKE_POSITION_INDEPENDENT_CODE ON`, `BUILD_SHARED_LIBS OFF`, `HDR_HISTOGRAM_BUILD_SHARED OFF`, `HDR_HISTOGRAM_BUILD_STATIC ON`, `HDR_HISTOGRAM_INSTALL_SHARED OFF`, `HDR_HISTOGRAM_INSTALL_STATIC OFF`, `HDR_HISTOGRAM_BUILD_PROGRAMS OFF`; `HDR_LOG_REQUIRED` deliberately left at its `ON` default), the scope-limited `macro(install) endmacro()` override neutralizing the three unconditional rules (package-config `.cmake`, `.pc`, `include/hdr/*.h`) with its written justification beside it, then `add_subdirectory(external/hdrhistogram_c "${CMAKE_BINARY_DIR}/_hdrhistogram" EXCLUDE_FROM_ALL)`, then promote `hdr_histogram_static`'s interface include to `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`; invoke it once; the override must not shadow speedgun-ng's own installs (they live in `cmake/install-rules.cmake`, a different scope). Its correctness is proven empirically by the quickstart section 6 audit (US2's T019) (FR-002, FR-009, FR-010, FR-011, FR-012, FR-017; R-004, R-007, R-009, R-010, R-011, R-012)
- [X] T006 [P] Create `cmake/VendoredArchiveMerge.cmake`: `vendored_archive_merge(<library-target> <archived-target>...)`, the generalization of `cmake/SimdjsonArchiveMerge.cmake`: same `/tmp` + MRI `ar -M` mechanism (CREATE, ADDLIB own members, ADDLIB each vendored archive, SAVE, ranlib), static-library targets only, shared configurations skipped with a STATUS message; re-route `simdjson_archive_merge(<target>)` in `cmake/SimdjsonArchiveMerge.cmake` through the shared core as a thin named wrapper, behavior-preserving so 004's green gates are not regressed (R-006; constitution X.2/X.3)
- [X] T007 [P] Extend the vendored-private classifier in `tools/dbc/dependency_scan.sh` (beside the existing `hwloc_vendor`/`simdjson` branches at lines 96-103) with branches classifying `hdr_histogram` and `zlibstatic`/`ZLIB::ZLIB` PRIVATE links on `speedgun-ng_speedgun-ng` as `vendored-private`, notes citing specs/005 FR-007/FR-021 privacy A2/A3/A6, so `dbc_dependency_scan` stays green when the links land (R-013)
- [X] T008 Validate the foundation: `cmake --preset=dev && cmake --build --preset=dev -j` exits 0; `find <binary-dir>/_zlib <binary-dir>/_hdrhistogram -name '*.so*'` prints nothing (forced static, quickstart section 10 shape); `git -C external/hdrhistogram_c status --porcelain` and `git -C external/zlib status --porcelain` are empty (SC-007); `grep -iE 'ZLIB_INCLUDE_DIR|ZLIB_LIBRARY' <binary-dir>/CMakeCache.txt` records the vendored paths (SC-010 shape)

**Checkpoint**: Foundation ready: `zlibstatic` (static, hidden, `z_`-prefixed, install-off) and `hdr_histogram_static` (static, hidden, logging-on against the redirect, installs neutralized) exist as uninstrumented targets; user stories can begin.

---

## Phase 3: User Story 1 - The project builds with a pinned, vendored HdrHistogram_c (Priority: P1) 🎯 MVP

**Goal**: The speedgun-ng library compiles the two wrapper TUs, links each vendored archive PRIVATE, merges both for static builds, and the version tripwires plus the purity/nm audits all hold.

**Independent Test**: Clone with submodules, build, inspect `git submodule status`, swap either submodule tag to fire its assertion, run `ctest -R 'hdrhistogram_nm_proof|zlib_nm_proof|purity_scan'`, configure a sanitize build, configure an uninitialized clone, install a system `libhdrhistogram-c-dev` and `zlib1g-dev` and reconfigure (quickstart sections 2, 3, 4, 5, 9, 11, 12, 13: every check satisfies US1 alone).

### Implementation for User Story 1

- [X] T009 [P] [US1] TDD-create `tools/hdrhistogram/hdrhistogram_purity_scan.sh`, modeled on `tools/simdjson/simdjson_purity_scan.sh`: audit A7 (zero `find_package( *hdr[-_]?histogram` / `pkg_check_modules( *hdr[-_]?histogram`, case-insensitive, across `CMakeLists.txt`/`*.cmake` excluding `external/`, `build/`, prefix dirs, `.git/`) and A8 (zero `hdr[-_]?histogram` under `include/`); print every hit as `path:line: text`; exit 1 on any hit; the script never scans itself. RED first against a seeded violation fixture line, GREEN against the clean tree (TDD per plan Test Plan; FR-004, FR-014, FR-019, SC-008; privacy contract A7/A8)
- [X] T010 [P] [US1] TDD-create `tools/zlib/zlib_purity_scan.sh`, same model: A8z (zero `zlib`/`libz` under `include/`, case-insensitive) and an assertion that no `find_package`/`pkg_check_modules` naming zlib under any `zlib`/`libz` spelling resolves to the host in project build files; the host-resolution proof stays with the CI SC-010 configure-record check (R-013), so the scan does not duplicate it. RED against a seeded fixture, GREEN clean (TDD; FR-014, FR-019; privacy contract A8)
- [X] T011 [US1] Create `source/hdrhistogram/hdrhistogram_gate.cpp` following `source/simdjson/simdjson_gate.cpp`: header comment (sole HdrHistogram_c header includer, version tripwire, link proof, README re-pinning pointer), `#include <hdr/hdr_histogram.h>` plus `#include <hdr/hdr_histogram_version.h>` (verified 0.11.10: `HDR_HISTOGRAM_VERSION` lives only in that configure-generated header, absent from `hdr_histogram.h`; the gate TU stays the single wrapper unit), `static_assert(std::string_view{HDR_HISTOGRAM_VERSION} == "0.11.10", "...expected HdrHistogram_c 0.11.10...")` (the only version identifier upstream exposes is the string macro, R-002), and `[[maybe_unused]] [[gnu::used]] constinit` function-pointer reference to `&hdr_alloc` (zero runtime lines, no cast, R-005); register with `target_sources(speedgun-ng_speedgun-ng PRIVATE source/hdrhistogram/hdrhistogram_gate.cpp)` and `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:hdr_histogram_static>)` in `CMakeLists.txt` beside the bracket (FR-003, FR-007, FR-014; R-002, R-005)
- [X] T012 [US1] Create `source/zlib/zlib_gate.cpp`, same shape: `#include <zlib.h>`, `static_assert(std::string_view{ZLIB_VERSION} == "1.3.2" && ZLIB_VERNUM == 0x1320u, "...expected zlib 1.3.2...")` (no `ZLIB_VER_PATCH` exists, R-003), and `[[maybe_unused]] [[gnu::used]] constinit` reference to `&zlibVersion` (under `Z_PREFIX` the header renames it to `z_zlibVersion`, matching the archive symbol, R-005); register with `target_sources(... PRIVATE source/zlib/zlib_gate.cpp)` and `target_link_libraries(... PRIVATE $<BUILD_INTERFACE:zlibstatic>)` (FR-003, FR-021, FR-014; R-003, R-005)
- [X] T013 [US1] TDD-create `tools/hdrhistogram/hdrhistogram_nm_proof.sh` (A6: `nm libspeedgun-ng.a` lists `hdr_` vendored members; a `hdrhistogram_gate` member carries an UNDEFINED `hdr_alloc` reference; a defined `hdr_alloc` resolves in a different member) and `tools/zlib/zlib_nm_proof.sh` (A6z: `z_`-prefixed zlib members present; the gate's `z_zlibVersion`/`zlibVersion` reference resolves into a different member), both modeled on `tools/simdjson/simdjson_nm_proof.sh`; register all four scripts (`hdrhistogram_purity_scan`, `hdrhistogram_nm_proof`, `zlib_purity_scan`, `zlib_nm_proof`) as CTest tests in `test/CMakeLists.txt`, same `add_test(NAME ... COMMAND bash ... ${CMAKE_SOURCE_DIR} ...)` pattern as the simdjson pair at lines 82-83. Run the nm proofs against the thin (un-merged) archive: expected RED; the recorded TDD red proving the merge's necessity (FR-007, FR-021, SC-009, SC-010; privacy contract A6/A6z)
- [X] T014 [US1] Wire the merge in `CMakeLists.txt`: `include(cmake/VendoredArchiveMerge.cmake)` and call `vendored_archive_merge(speedgun-ng_speedgun-ng hdr_histogram_static zlibstatic)` beside the existing `simdjson_archive_merge(speedgun-ng_speedgun-ng)` call (both POST_BUILD steps compose: each ADDLIBs the target's own members first); shared configurations skip it (the PRIVATE link plus hidden objects absorb both at link time); rerun `ctest -R 'hdrhistogram_nm_proof|zlib_nm_proof'`: GREEN (FR-007, FR-021, SC-009, SC-010; R-006)
- [X] T015 [US1] Run the US1 validation set from `specs/005-vendor-hdrhistogram/quickstart.md` and capture binary verdicts (Principle X.4): section 2 (both pinned SHAs), section 3 (tripwires: checkout `0.11.9` fires the hdr gate diagnostic naming 0.11.10, checkout `v1.3.1` fires the zlib gate naming 1.3.2/`0x1320`, restore both pins green), section 4 (both worktrees pristine after a full build), section 5 (`ctest -R 'hdrhistogram_nm_proof|zlib_nm_proof'` green), section 9 (`zlib1g-dev` installed: cache records vendored `ZLIB_INCLUDE_DIR`/`ZLIB_LIBRARY`, no `Found ZLIB` at a system prefix), section 11 (`ci-sanitize`: zero vendored compiles carry `-fsanitize`, suite green), section 12 (clone without `--recurse-submodules`: configure aborts naming the init command, no system fallback), section 13 (`libhdrhistogram-c-dev` installed: zero discovery, purity scan green) (FR-002, FR-003, FR-004, FR-007, FR-008, FR-010, FR-020, SC-006, SC-007, SC-008, SC-009, SC-010)

**Checkpoint**: US1 fully functional: both dependencies pinned, tripwired, privately linked, merged, proven independently.

---

## Phase 4: User Story 2 - Consumers cannot observe HdrHistogram_c (Priority: P1)

**Goal**: Install-tree, package-file, shared-symbol, runtime-dependency, and downstream-consumer audits prove both vendored trees invisible, wired into the existing CI jobs (R-013).

**Independent Test**: Install, audit the prefix tree and package files for both patterns, build shared and audit `.dynsym` + `ldd` (plus the `hdr_`-prefix and bare zlib-name checks), then run the reused `test/consumer/` project on a machine carrying both system packages and grep its logs (quickstart sections 6, 7, 8: runs against any US1 build).

### Implementation for User Story 2

- [X] T016 [US2] Add four steps to the `test` job in `.github/workflows/ci.yml`, beside the existing hwloc/simdjson audits (lines 121-127): install-tree audit `test -z "$(find prefix/ -iname '*hdr*histogram*')"` and `test -z "$(find prefix/ -iname '*zlib*' -o -iname '*libz*')"` (A1/A1z, SC-002), package-file audit `! grep -riE 'hdr[-_]?histogram' prefix/lib/cmake/speedgun-ng/*.cmake` and `! grep -riE 'zlib|libz' prefix/lib/cmake/speedgun-ng/*.cmake` (A2/A2z, SC-003) (FR-015, FR-016, FR-019)
- [X] T017 [US2] Add audit steps to the `shared-audit` job in `.github/workflows/ci.yml`, beside the hwloc/simdjson steps (lines 176-182): `! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -iE 'hdr[-_]?histogram'`; `! nm -D --defined-only --format=posix prefix-shared/lib/libspeedgun-ng.so | cut -d' ' -f1 | grep -E '^_?hdr_'` (the `nm` text format starts lines with the address, so a line-start symbol anchor would pass vacuously; the posix format puts the symbol name in field 1); `! nm -D --defined-only ... | grep -iE 'zlib|libz'`; `! nm -D --defined-only ... | grep -wE 'inflate|deflate|compress|uncompress'`; `! ldd prefix-shared/lib/libspeedgun-ng.so | grep -iE 'hdr|libz|zlib'` (A4/A4z/A5, SC-004, SC-011; R-007 forced static means no separate object exists to name) (FR-017, FR-022)
- [X] T018 [US2] Extend the `downstream-consumer` job in `.github/workflows/ci.yml`: add `libhdrhistogram-c-dev` and `zlib1g-dev` to its apt-get install step so the machine carries both, and add log-audit steps `test -z "$(grep -iE 'hdr[-_]?histogram' consumer-configure.log consumer-build.log)"` and `test -z "$(grep -iE 'zlib|libz' consumer-configure.log consumer-build.log)"` beside the hwloc/simdjson log audits (lines 209-211); `test/consumer/` is reused unchanged: it names no vendored dependency, so its configure/build/run exit 0 plus the zero greps is the authoritative privacy proof (FR-018, SC-005)
- [X] T019 [US2] Run the US2 validation set from `specs/005-vendor-hdrhistogram/quickstart.md`: section 6 (install tree and package files print nothing for both patterns; this is the empirical red-until-implemented gate of the `install()` override: a stray `prefix/include/hdr/`, `hdr_histogram-config.cmake`, or `hdr_histogram.pc` means R-010's scratch-directory fallback applies), section 7 (shared build: all `nm -D`, `ldd`, greps exit 1 with empty output), section 8 (consumer with both system packages installed: configure/build/run exit 0, both log greps zero) (FR-012, SC-002, SC-003, SC-004, SC-005, SC-010, SC-011)

**Checkpoint**: US1 and US2 both functional: the privacy contract holds on every consumer-observable surface, proven by audits.

---

## Phase 5: User Story 3 - Re-pinning HdrHistogram_c is documented and auditable (Priority: P3)

**Goal**: One short README section per dependency states the three steps; the pinned commits in spec.md make each pin auditable.

**Independent Test**: Follow each README section on a scratch clone against a different release tag (`0.11.9`, `v1.3.1`; both pins are the newest upstream releases today) (green); move a tag without the assertion bump (red) (quickstart section 14).

### Implementation for User Story 3

- [X] T020 [US3] Add "Re-pinning HdrHistogram_c" and "Re-pinning zlib" sections to `README.md`, beside the existing hwloc/simdjson sections: per dependency: check out the new tag (`git -C external/hdrhistogram_c checkout <tag>` / `git -C external/zlib checkout <tag>`), commit the submodule pointer, bump that dependency's version assertion in its gate file (`source/hdrhistogram/hdrhistogram_gate.cpp` / `source/zlib/zlib_gate.cpp`); state that the build fails until the assertion matches; keep the prose prose-gate clean (`cmake -P cmake/prose-lint.cmake`, Principle XI) (FR-006)
- [X] T021 [US3] Run the re-pin rehearsal from `specs/005-vendor-hdrhistogram/quickstart.md` section 14 on a scratch clone against a different release tag of either dependency (`0.11.9`, `v1.3.1`; both pins are the newest upstream releases today): with all three steps the build goes green; with the tag moved and the assertion untouched the build is red with the section-3 tripwire diagnostic; restore both pins afterwards (US3 scenarios 1-2)

**Checkpoint**: All user stories independently functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T022 Run `cmake --preset=dev`, `cmake --build --preset=dev -j`, `ctest --preset=dev --output-on-failure`, and the full `specs/005-vendor-hdrhistogram/quickstart.md` sections 0 through 14 end to end; record every binary verdict (X.4); tear down scenario artifacts per quickstart Teardown
- [X] T023 [P] Verify the gate set is unweakened (Principle VIII): coverage preset green with the coverage trace listing both gate TUs at 0 executable lines (plan Test Plan); `dbc_dependency_scan` and the hwloc/simdjson CTest entries green unchanged; `cmake -P cmake/prose-lint.cmake` green over the PR range; the FR-013a invariant: grep the build files for any option/preset/cache variable able to exclude hwloc, simdjson, HdrHistogram_c, or zlib from the library: zero found
- [ ] T024 [P] Verify the macOS developer path: `cmake --preset=ci-macos` configures and builds, and the merge step drives `${CMAKE_AR}` (`llvm-ar -M`) as it does for hwloc/simdjson (developer-local, FR-008; no CI runner)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies - start immediately; T001 then T002 (both write `.gitmodules`)
- **Foundational (Phase 2)**: T003 → T004 → T005 (same file `CMakeLists.txt`, and the redirect must sit between the brackets); T006 and T007 [P] independent (different files); T008 depends on T003-T005. BLOCKS all user stories
- **User Stories (Phase 3+)**: all depend on the Foundational checkpoint
  - US1 (Phase 3): T009/T010 [P] any time (own files, script-only); T011 and T012 edit `CMakeLists.txt` (registrations) and depend on T003-T005, sequential with each other; T013 depends on T011/T012 (symbol names); T014 depends on T013 (red) and T006 (module); T015 last within US1
  - US2 (Phase 4): depends on US1 (needs the built, installable library); T016/T017/T018 all edit `ci.yml`: sequential; T019 last
  - US3 (Phase 5): depends on US1 (the gate files the README names must exist); T021 after T020
- **Polish (Phase 6)**: depends on all delivered stories; T023/T024 [P] parallel with T022

### User Story Dependencies

- **US1 (P1)**: after Foundational; no dependency on other stories: this is the MVP
- **US2 (P1)**: after US1 (audits run against the US1 build's install tree); independently testable per spec
- **US3 (P3)**: after US1 (assertion files must exist); independently testable

### Within Each User Story

- TDD red before implementation green (purity fixtures; nm thin-archive red)
- Gate TU before its nm proof; nm proof (red) before the merge (green)
- CI wiring before the local validation task that mirrors it
- zlib bracket → redirect → HdrHistogram_c bracket: the ordering is a hard configure-time dependency (R-008)

### Parallel Opportunities

- T006, T007 while T003-T005 run (different files)
- T009, T010 any time in US1 (own files, no build dependency)
- T023, T024 in parallel with T022
- After Foundational, US1 is the only unblocked story (US2 needs US1's build, US3 needs US1's gate files): sequential delivery US1 → US2 → US3 matches the spec's priority reading of "both P1, foundation first"

---

## Parallel Example: Foundational + US1 start

```bash
# While T003/T004/T005 (brackets + redirect, CMakeLists.txt) are in progress:
Task: "T006 Generalize archive merge into cmake/VendoredArchiveMerge.cmake"
Task: "T007 Extend classifier in tools/dbc/dependency_scan.sh"
Task: "T009 [US1] TDD purity scan in tools/hdrhistogram/hdrhistogram_purity_scan.sh"
Task: "T010 [US1] TDD purity scan in tools/zlib/zlib_purity_scan.sh"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Phase 1: both submodules at `18c7a324...` and `da607da7...`
2. Phase 2: zlib bracket + redirect + HdrHistogram_c bracket + merge module + classifier + foundation checkpoint (CRITICAL: blocks everything)
3. Phase 3: gate TUs, nm proofs (red), merge wiring (green), US1 validation
4. **STOP and VALIDATE**: US1 standalone per quickstart sections 2-5, 9, 11-13
5. Deliver: the pinned, vendored, tripwired build is the MVP

### Incremental Delivery

1. MVP above → both dependencies ingested, version-locked, proven
2. Add US2 CI audits → privacy contract proven on every surface → deliver
3. Add US3 README sections + rehearsal → re-pinning cheap and traceable → deliver
4. Each story adds value without touching previous stories' files beyond the shared `ci.yml`

### Notes for the Implementer

- The brackets are function scopes: target-creating commands persist out of them, variable resets do not (R-011). The `install()` override is a `macro(install) endmacro()` defined inside `import_hdrhistogram()` only; speedgun-ng's own installs live in `cmake/install-rules.cmake` and must be unaffected; quickstart section 6 is the empirical check.
- The zlib audit is the `zlib`/`libz` pair plus the bare `inflate`/`deflate`/`compress`/`uncompress` names; the HdrHistogram_c audit is `hdr[-_]?histogram` plus the `hdr_` prefix. A `hdrhistogram`-only search does not satisfy the FR-019 audits.
- `find_package(ZLIB)` inside the HdrHistogram_c subtree is permitted: it is redirected to the vendored copy (contract §1). No `find_package`/`pkg_check_modules` naming HdrHistogram_c under any spelling may exist anywhere in project build files.
- `CMakePresets.json` needs no change (its clang-tidy preset already excludes `external/`); `.codespellrc` needs no change (`*/external` already skipped); touching either would weaken a shared gate (R-013, constitution VIII).
- `HDR_LOG_REQUIRED` stays at its `ON` default and `ZLIB_PREFIX ON` is non-negotiable: disabling logging to dodge zlib is prohibited (Fixed decision 6); `Z_PREFIX` is what keeps the bare zlib names out of existence (FR-022).
- No PUBLIC edge, no option or preset that excludes any of the four vendored dependencies (FR-013a), no patched vendored CMakeLists (SC-007).
- Commit after each task or logical group; `<Section>: <one-line imperative>` per the constitution PR template.

---

## Phase 7: Convergence

- [X] T025 Add the `libz` case-insensitive pattern to the three zlib leak audits in `.github/workflows/ci.yml` so each searches both `zlib` and `libz`: install-tree L134 `find prefix/ -iname '*zlib*'` -> `find prefix/ \( -iname '*zlib*' -o -iname '*libz*' \)`; package-file L136 `! grep -qi zlib ...` -> `! grep -qiE 'zlib|libz' ...`; consumer-log L234 `grep -i zlib` -> `grep -iE 'zlib|libz'`. A leaked `libz.a`/`libz.so` must trip the audit, the vacuous-pass case FR-019 forbids (per FR-019, SC-002, SC-003) (partial)
- [X] T026 Broaden the hdr shared runtime-dependency audit in `.github/workflows/ci.yml` L195 `! ldd prefix-shared/lib/libspeedgun-ng.so | grep -i hdrhistogram` to the FR-019 spelling class `grep -iE 'hdr[-_]?histogram'`, so a `libhdr_histogram*`/`libhdr-histogram*` leak cannot pass on the glued spelling alone (per FR-019, SC-004) (partial)

---

## Phase 8: Convergence

- [X] T027 Search the shared-build zlib symbol audit in the `shared-audit` job of `.github/workflows/ci.yml` (L196-199) with the FR-019 pattern class: extend `grep -iE 'zlibVersion|(z_)?(inflate|deflate|compress|uncompress|crc32)'` to include the `zlib|libz` alternatives, so an exported `z_zlibCompileFlags` or any symbol name carrying `zlib`/`libz` trips the audit instead of passing vacuously (per SC-011, FR-019, FR-022) (partial)
- [X] T028 Broaden the shared-build zlib runtime-dependency audit in the `shared-audit` job of `.github/workflows/ci.yml` (L200-201) from `grep -E 'libz(\\.|\\.so)'` to the case-insensitive mandated patterns `grep -iE 'zlib|libz'`, so an `ldd` line naming a `libzlib`/`libzng`-class object cannot pass on the narrow dot-anchored spelling (per SC-004, SC-010, FR-019) (partial)
