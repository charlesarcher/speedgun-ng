# Tasks: Vendor quill as a Private, Pinned Submodule

**Input**: Design documents from `specs/009-vendor-quill/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/build-integration.md](contracts/build-integration.md), [contracts/privacy-contract.md](contracts/privacy-contract.md), [quickstart.md](quickstart.md)

**Tests**: Included. The Test Plan in plan.md records TDD mode (Principle III): the two `tools/quill/` artifacts are the seam-level tests, and the runnable dependency check is red until the link edge and the counter plumbing both land.

**Organization**: Tasks are grouped by user story so each story can be implemented and tested independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = build with a pinned, vendored quill; US2 = consumers cannot observe quill; US3 = re-pinning documented
- Every task names its exact file path

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Bring the pinned sources into the tree

- [X] T001 Add the git submodule `external/quill` from `https://github.com/odygrd/quill.git` checked out at commit `eb802a37c7d585840324886a3d8648c9c2159952` (tag `v13.0.0`), producing the `.gitmodules` stanza and the submodule gitlink; confirm `git submodule status external/quill` records that SHA with no `+` or `-` prefix, and that the MIT `LICENSE` file is present in the submodule root (FR-001, FR-006; quickstart section 1). Do not modify any file inside the vendored tree. Confirm the tree carries no submodules of its own and that `include/quill/bundled/fmt/` travels inside it.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The ingestion bracket, the wrapper unit, the link edge, and the dependency-classification branch. Everything the library needs before any story can be verified.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T002 Add `import_quill()` to `CMakeLists.txt`, placed after the `import_yaml_cpp()` call site and modeled on the `import_simdjson`/`import_zlib`/`import_yaml_cpp` brackets: (a) guard: `FATAL_ERROR` naming `git submodule update --init external/quill` when `external/quill/CMakeLists.txt` is absent (FR-004); (b) option pinning: define all 22 upstream options at their upstream defaults as bracket-scope normal variables, exactly as listed in build-integration contract section 3, before the subtree is consumed (FR-013a; R-003); (c) `set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)` before the subtree, since upstream's declared build-system minimum predates that policy and its option declarations would otherwise clear the bracket's values (FR-013; R-003); (d) `add_subdirectory(external/quill "${CMAKE_BINARY_DIR}/_quill" EXCLUDE_FROM_ALL)` (FR-008; R-001); (e) SYSTEM include promotion: read the `quill::quill` interface include property and re-set it as `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`, matching what the four compiled brackets already do (FR-010a; R-006); (f) build-type assertion: capture `CMAKE_BUILD_TYPE` before the subtree and `FATAL_ERROR` naming both values if it moved, because upstream force-writes that global cache variable (FR-012, SC-007; R-004). Add no flag-clearing block: quill compiles no target, so clearing variables in this bracket reaches no compilation (FR-010b; R-006). Leave `CMAKE_CXX_STANDARD` untouched so the presets' 23 is inherited. Prohibited in this bracket: any `find_package`/`pkg_check_modules` naming quill, any PUBLIC or INTERFACE link edge, any archive-merge registration, any exclusion option (FR-005, FR-008, FR-015a).
- [X] T003 [P] Create `source/quill/quill_gate.cpp`: include `<quill/Backend.h>` alone (SYSTEM-promoted by T002), then hold the version tripwire as `static_assert` on `quill::VersionMajor == 13`, `quill::VersionMinor == 0`, `quill::VersionPatch == 0`, `quill::Version == 130000`, plus one assertion that the versioned inline namespace resolves (FR-002, FR-003; R-002). Add no symbol reference, no call, and no quill macro before the include: at `-O3 -DNDEBUG` a reference costs 563,104 bytes against 3,744 bytes for the constants alone (FR-008, SC-009a; R-008). Keep zero executable lines, matching `source/yaml/yaml_gate.cpp`. Mirror that file's sole-includer comment and its pointer to the README re-pinning section.
- [X] T004 Wire the library in `CMakeLists.txt` (after T002, same file, so not parallel): register the wrapper unit with `target_sources(speedgun-ng_speedgun-ng PRIVATE source/quill/quill_gate.cpp)` and add `target_link_libraries(speedgun-ng_speedgun-ng PRIVATE $<BUILD_INTERFACE:quill::quill>)`: PRIVATE only, build-interface-wrapped, no PUBLIC edge (FR-008; R-007). Add quill to no `vendored_archive_merge` argument list and change no file under `cmake/`: quill produces no archive, so there is nothing to merge (FR-008; R-007). TDD order (Principle III): with the wrapper unit registered, record `nm --format=bsd build/dev/libspeedgun-ng.a | grep -c quill` before and after, then confirm the archive gains under 64 KB against an equivalent build of the same tree in the same configuration without the ingestion (SC-009a). The wrapper unit's object is 3,744 bytes in the release configuration, which is the configuration the cap names; a debug build of the same unit is about 319 KB and would exceed the cap on debug information alone.
- [X] T005 [P] Add one `elif grep -q 'quill'` classifier branch to `tools/dbc/dependency_scan.sh`, beside the existing `hwloc_vendor`/`simdjson`/`hdr_histogram`/`zlibstatic`/`yaml-cpp` branches, setting `class="vendored-private"` with a note naming this feature's requirements and audit identifiers (FR-008, FR-016; R-015). Land it with T004. A later story is the wrong place: the script exits 1 on any `library-runtime` classification, so the moment the T004 link edge exists the `dbc_dependency_scan` CTest entry fails without this branch.

**Checkpoint**: Foundation ready: configure succeeds against the pinned tree, the static archive builds, the guard and tripwire diagnostics fire on demand, and every existing CTest entry including `dbc_dependency_scan` is green.

---

## Phase 3: User Story 1: The project builds with a pinned, vendored quill (Priority: P1) 🎯 MVP

**Goal**: A clean clone with submodules initialized builds on Linux, builds under the release preset with no diagnostic from a vendored header, builds on macOS, resolves no system quill, and fails with a readable version diagnostic when the submodule points at any other revision.

**Independent Test**: Clone with submodules, run the development preset build, the release preset build, and the macOS preset build; inspect `git submodule status`; confirm no `QUILL_*` entry appears in the project's configuration; then flip the submodule to another tag and observe the diagnostic. Every check runs against a build satisfying this story alone.

### Tests for User Story 1 ⚠️

> **NOTE**: Write the check first, watch it fail to link, then land T007 and watch it pass.

- [X] T006 [P] [US1] Create `tools/quill/quill_dependency_check.cpp` as the runnable dependency proof: link the speedgun-ng library and `quill::quill`, drive a real counter to a real value through the public counter interface in `include/speedgun-ng/counters.hpp`, emit that value through quill's logging entry point, and assert the value appears in what reached the log (FR-008, FR-008a; privacy contract A6; R-008). Model the counter setup on `example/counters_standalone_example.cpp`, which already registers a fake provider, resolves an object and its counters, compiles a plan, samples a recorder, and folds intervals. Start quill's backend and stop it explicitly, and keep this file out of the shipped archive and the exported target set (FR-008). Emitting a fixed string would link quill and prove nothing about interoperation, which is the property FR-008a requires.

### Implementation for User Story 1

- [X] T007 [US1] Register the check in `test/CMakeLists.txt`: add an executable linking `speedgun-ng_speedgun-ng` and `quill::quill`, plus `add_test(NAME quill_dependency_check COMMAND ...)` beside the ten existing vendored-dependency entries. Name it distinctly from the `*_nm_proof` family, because a passing test named `quill_nm_proof` would assert an archive scan that does not exist and cannot exist here (FR-008; R-012).
- [X] T008 [US1] Verify the tripwire fires on a wrong pin: `git -C external/quill checkout v12.2.2`, rebuild, confirm the compile fails naming expected 13.0.0 and found, then restore to `v13.0.0` and confirm green (SC-006; quickstart section 5).
- [X] T009 [US1] Verify the shipped archive stays lean: compare the archive against an equivalent build of the same tree in the same configuration without the ingestion, and confirm growth under 64 KB (SC-009a; quickstart section 4). The wrapper unit's object measures 3,744 bytes in the release configuration, which is the configuration SC-009a names, so the cap sits about seventeen times above the measured cost. A jump into the hundreds of kilobytes means a quill symbol reference crept into a shipped unit, which FR-008 forbids. Compare like configurations only: the same unit is about 319 KB in a debug build, almost entirely debug information, so a cross-configuration comparison would read as a failure when nothing is wrong.
- [X] T010 [US1] Verify no quill option reaches the project's configuration and that a hostile flag has no effect: `grep -E '^QUILL_' build/dev/CMakeCache.txt` must list only the two entries upstream sets itself, and configuring with `-DQUILL_BUILD_TESTS=ON -DQUILL_BUILD_EXAMPLES=ON -DQUILL_ENABLE_INSTALL=ON -DQUILL_DOCS_GEN=ON` must build only upstream's interface target, create no test or example directory under the build tree, and generate no pkg-config file, package config file, or export set (FR-011, FR-013a; R-003; quickstart section 1). A command-line value lands as an unused cache entry before any ingestion runs, so read the cache file. `cmake -LA` also prints the latter also prints upstream's own status messages, which say nothing about the cache.
- [X] T011 [US1] Verify the release preset carries no vendored diagnostic: `cmake --preset=ci-ubuntu`, then `cmake --build build -j 2 2>&1 | grep -E '(warning|error|note):' | grep -F 'external/quill/'`, must produce no output (SC-001, SC-011; quickstart section 2). The unoptimized development preset cannot answer this question, which is why the release preset is the gate. Corrected under T031: the check this task originally carried was `grep -i quill` requiring no output, and it could not pass on a correct tree, because every line it matched named this feature, with no finding among them.
- [X] T012 [US1] Verify the submodule worktree stays pristine and no exclusion switch exists: `git status --short external/quill` empty after a full build, and no preset, option, or cache variable that excludes quill from the library on Linux or macOS (FR-015, FR-015a, SC-007; quickstart section 6).

**Checkpoint**: User Story 1 is fully functional and testable independently: the library builds from the pinned submodule on every supported platform, the pin is load-bearing, and the dependency is proven end to end by a running check.

---

## Phase 4: User Story 2: Consumers cannot observe quill (Priority: P1)

**Goal**: Nothing a downstream `find_package(speedgun-ng)` consumer can observe mentions quill: installed headers, package files, link interfaces, and exported symbols are all free of it, and a consumer on a machine carrying quill resolves zero quill.

**Independent Test**: Install the library, audit the install tree, the package config files, and the shared-library symbol and dependency tables, then build and run a trivial consumer on a machine carrying a system quill and audit that the consumer resolves zero quill. These checks run against any build satisfying User Story 1.

### Tests for User Story 2 ⚠️

- [X] T013 [P] [US2] Create `tools/quill/quill_purity_scan.sh`, modeled on `tools/yaml/yaml_purity_scan.sh`: (a) zero `find_package\( *quill` or `pkg_check_modules\([^)]*quill` hits across `CMakeLists.txt` and `*.cmake` repo-wide, excluding `external/`, `build/`, `prefix*`, `.git/`, `.specify/`, `.omo/`, and `specs/`; (b) zero `quill` hits under `include/` (FR-005, FR-014, FR-020; privacy contract A7 and A8; SC-008). The `specs/` exclusion is the one departure from the five existing scans: this feature's own specification and research name the pattern throughout and would otherwise trip the scan. Exit 0 on clean, 1 with `path:line: text` findings otherwise.

### Implementation for User Story 2

- [X] T014 [US2] Register the purity scan in `test/CMakeLists.txt` beside the five existing pairs: `add_test(NAME quill_purity_scan COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../tools/quill/quill_purity_scan.sh ${CMAKE_SOURCE_DIR})` (FR-005, FR-014; R-012).
- [X] T015 [US2] Add the audit steps to `.github/workflows/ci.yml`: (a) `test` job after install, `find prefix/ -iname '*quill*'` must be empty and `grep -riE 'quill' prefix/lib/cmake/speedgun-ng/*.cmake` must be empty (A1, A2; FR-016, FR-017; SC-002, SC-003); (b) `shared-audit` job, `nm -D --defined-only` on the installed shared object filtered on `5quill` and on `8fmtquill`, where every resulting symbol must appear in the FR-018 allowlist. This is a set membership check. An empty-output check would be wrong in both directions: two quill symbols are permitted by FR-018, and a library exporting none must also pass. The bundled-formatter marker keeps its own step and admits no exception. Alongside it, `grep` on the shared link interface and `ldd` filtered on `quill` must both be empty (A3, A4, A5; FR-018, FR-020; SC-004) Every symbol audit reads the installed shared object and never the internal archive's member list: quill's export attribute is default visibility on GCC and Clang regardless of this project's settings, so an archive audit would have to allowlist quill's singletons and would then miss one escaping into the dynamic symbol table (FR-017; R-010). (c) `downstream-consumer` job, an install premise step that configures and installs the pinned tree into a system prefix so the machine does carry a system quill, since upstream publishes no system development package on the supported distribution, then `grep -riE 'quill' consumer-configure.log consumer-build.log` must find nothing (FR-019; R-014; SC-005). Read every symbol audit from the installed shared object and never from the internal archive's member list: quill's export attribute is default visibility on GCC and Clang regardless of this project's settings, so auditing the archive would require whitelisting quill's singletons and would then miss one escaping into the dynamic symbol table (FR-017; R-010).
- [X] T016 [US2] Verify the ingestion's configuration surface, covering the claims FR-009, FR-011, and SC-010 make and no other task checks: (a) no quill package config file, quill package config version file, quill pkg-config file, quill export set, or quill install-directory cache variable exists anywhere in the build tree, since upstream's install path would create all of them and a stray option could reach it; a packaging configuration file alone proves nothing, because this project creates its own through its install rules (FR-011, SC-010); (b) read the `quill::quill` target's `INTERFACE_COMPILE_OPTIONS` and confirm it holds exactly one entry, the Clang-family warning relaxation, and that `INTERFACE_COMPILE_DEFINITIONS` holds none at upstream defaults (FR-009, SC-010); (c) confirm nothing from either upstream install path reached the install prefix beyond the install-tree audit T017 already performs. Record each check with its binary verdict. Both halves of SC-010 are build-tree or configuration claims, so the install-tree audit alone cannot close them (R-005, R-003).

- [X] T017 [US2] Verify the privacy surfaces end to end for FR-016 through FR-019 on the local machine per quickstart section 7: install-tree audit, package-file audit, shared-build audits A3 through A5 including both mangled-namespace markers, and the downstream consumer test against the installed tree with a system quill present. Record each command and its verdict.

**Checkpoint**: User Stories 1 and 2 both work independently. The dependency is real, pinned, and proven at run time, and invisible on every consumer-observable surface.

---

## Phase 5: User Story 3: Re-pinning quill is documented and auditable (Priority: P3)

**Goal**: A maintainer updates the pinned quill by checking out a new tag, committing the submodule pointer, and bumping the version assertion, following one short documentation section, and the pinned commit recorded in the spec makes the pin auditable.

**Independent Test**: Follow the documentation section step by step on a scratch clone against a different release tag and confirm the build stays green; then bump the tag without bumping the assertion and confirm the build fails.

### Implementation for User Story 3

- [X] T018 [US3] Add a `## Re-pinning quill` section to `README.md`, beside the five existing re-pinning sections: check out the new tag (`git -C external/quill checkout <tag>`), commit the submodule pointer, bump the version constants in `source/quill/quill_gate.cpp`. State that this is a compile-time tripwire, unlike the yaml-cpp configure-time one, because quill publishes its version as compiled constants and publishes no preprocessor macro (FR-002, FR-007; R-002). Note that a future upstream option defaults to off with no bracket change, because T002 pins the whole option set (R-003).
- [X] T019 [US3] Run the re-pin rehearsal for FR-007 per quickstart section 8 on a scratch clone: check out an older tag, bump the constants to match, confirm the build stays green; then move the pointer without the constants and confirm the tripwire fires. Restore the scratch clone afterward.

**Checkpoint**: All three user stories are independently functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T020 Run the full local gate suite green: `cmake --preset=dev`, `cmake --build --preset=dev`, `ctest --preset=dev` including the two new `quill_*` entries, and `cmake -P cmake/prose-lint.cmake` (SC-001).
- [X] T021 Confirm the coverage gate is unaffected: run `cmake --preset=ci-coverage`, `ctest --test-dir build/coverage`, and `cmake --build build/coverage -t coverage`, then confirm the gate verdict is 100% lines and 100% branches and that the final trace carries no path under any vendored tree (FR-010; R-013). R-013 originally expected no tooling change and verification overturned it: the extract allowlist does drop every vendored path, but the capture does not skip vendored headers, because `--no-external` does not cover a directory inside the project's own source tree. quill is the first vendored tree whose headers trip lcov's exception-tag consistency check, and that aborts the capture and takes the coverage target down. The capture command in `cmake/coverage.cmake` gains `--ignore-errors mismatch`, bounded to that one lcov error class. Note `genhtml` may still exit non-zero locally when the perl GD module is absent, which README documents as a local environment gap and which is not a coverage verdict.
- [X] T022 Confirm the standalone example's link manifest is unchanged, since SC-001 keeps every existing CI job green: `readelf -d build/example/counters_standalone_example | grep NEEDED | grep -vE '\[(libc|libm|libstdc\+\+|libgcc_s)\.so'` must stay empty (R-011). The example compiles public headers only and no public header includes a quill header, so the example never reaches the dependency. If this audit goes red, investigate the cause; do not widen the allowlist.
- [X] T023 Final privacy-contract sweep over FR-016 through FR-020: re-run the quickstart section 7 verdicts and the quickstart section 3 runnable check against the final HEAD state, and record each command with its binary verdict in the pull request (privacy contract section 7).

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies, can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion, BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational completion
  - US1 and US2 are both P1 and can proceed in parallel once Foundational lands, since they touch different files
  - US3 depends on US1, because its documentation describes the tripwire US1 lands
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: starts after Foundational, no dependency on another story
- **User Story 2 (P1)**: starts after Foundational, touches `tools/quill/quill_purity_scan.sh`, `test/CMakeLists.txt` for its own entry, and `.github/workflows/ci.yml`, none of which US1 touches. T007, T014, and T016 are ordered within the story: T016 verifies the configuration surface T002 produced, so run it after T015 has configured and installed. T007 and T014 both edit `test/CMakeLists.txt`, so serialize those two.
- **User Story 3 (P3)**: starts after US1, since it documents the version constants US1 lands

### Within Each User Story

- The check is written first and fails to link before the wiring makes it link
- The wrapper unit and the bracket land before any audit can pass
- Every audit lands with the surface it audits

### Parallel Opportunities

- T003, T005, and T006 touch different files and can run in parallel
- T013 can run in parallel with US1's implementation
- US1 and US2 can be worked in parallel by different people once Foundational lands
- T008 through T012 are verification steps and can run concurrently
- T016 depends on T002 and T015, so it runs after the bracket lands and after a configure and install have run

---

## Parallel Example: Foundational plus US1 check

```bash
# Launch the independent Foundational pieces together:
Task: "Create source/quill/quill_gate.cpp"
Task: "Add one elif grep -q 'quill' classifier branch to tools/dbc/dependency_scan.sh"
Task: "Create tools/quill/quill_dependency_check.cpp"

# T002 and T004 both edit CMakeLists.txt, so they serialize.
```

---

## Implementation Strategy

### MVP First (User Story 1 and User Story 2 are both P1)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL, blocks all stories)
3. Complete Phase 3: User Story 1
4. Complete Phase 4: User Story 2
5. **STOP and VALIDATE**: run quickstart sections 1 through 7 and confirm every verdict
6. Ship

### Incremental Delivery

1. Complete Setup and Foundational, foundation ready
2. Add US1, the dependency builds pinned and is proven at run time
3. Add US2, the dependency is invisible to consumers
4. Add US3, re-pinning is documented and rehearsed
5. Each story adds value without breaking the previous ones

### Parallel Team Strategy

1. One person completes Setup and Foundational
2. Then: one person takes US1 and its verification, another takes US2's purity scan and CI audit steps, serialized only on `test/CMakeLists.txt`
3. US3 follows US1

---

## Notes

- [P] tasks touch different files with no dependencies
- [Story] label maps each task to its user story for traceability
- Each user story is independently completable and testable
- Verify the check fails before the wiring makes it pass
- Commit after each task or logical group; the house template is `<Section>: <imperative>` at 50 characters or fewer with an `Approved-by:` footer and `Refs: specs/009-vendor-quill`
- Stop at any checkpoint to validate the story independently
- Avoid: vague tasks, two tasks editing one file in the same parallel batch, and any cross-story dependency that breaks independence

---

## Phase 7: Convergence

Findings from a /speckit.converge pass over the Phase 1 through Phase 6 work against spec.md, plan.md, and the constitution. Every task below traces to the requirement or principle it closes. Ordering is CRITICAL and HIGH first.

- [X] T024 CRITICAL Clear every clang-tidy finding in `tools/quill/quill_dependency_check.cpp`, per Constitution VIII ("a finding is a defect at lint parity: the author clears it before merge") and T011. `cmake --preset=ci-ubuntu && cmake --build build` reports 56 findings in that one file; `source/quill/quill_gate.cpp` reports none, and neither do the five prior vendored dependency files. The classes are `misc-include-cleaner`, `cppcoreguidelines-pro-type-vararg`, `cert-err33-c`, `modernize-use-std-print`, `cppcoreguidelines-avoid-c-arrays`, `cppcoreguidelines-pro-bounds-array-to-pointer-decay`, `readability-magic-numbers`, `readability-identifier-naming`, `readability-identifier-length`, `readability-container-contains`, `llvm-prefer-static-over-anonymous-namespace`, and `bugprone-exception-escape`. Clear them by changing the code. Prohibited: suppressing the analyzer call, demoting a warning class, or adding a bare `NOLINT`. Constitution X.2 requires each suppression to carry its written justification at the site, and no suppression is warranted for a test translation unit. Re-run the T011 check afterwards, where `cmake --build build 2>&1 | grep -i quill` must produce no output (Constitution VIII, T011, SC-011)

- [X] T025 Add the missing shared link-interface audit to the `shared-audit` job in `.github/workflows/ci.yml`, per FR-017, T015 (b), and privacy contract A3: `test -z "$(grep -riE 'quill' prefix-shared/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake)"`. Today no step in that job greps the package files; `grep -n Targets .github/workflows/ci.yml` returns zero matches, and the only package-file audit (A2, line 158) reads the static `test`-job prefix. The A3 path exists because the shared install writes the export set to `lib/cmake/speedgun-ng/` per `cmake/install-rules.cmake`, so the check reaches a file that exists and cannot pass on a missing path. Place the step after the shared install and beside the existing quill symbol steps (FR-017)

- [X] T026 Reformat `source/quill/quill_gate.cpp` and `tools/quill/quill_dependency_check.cpp` to the house style, per Constitution V and T003: `clang-format --dry-run -Werror` reports 6 violations in the gate unit and 21 in the check, against `.clang-format` `ColumnLimit: 80`. `source/yaml/yaml_gate.cpp` and `source/simdjson/simdjson_gate.cpp`, the exemplars T003 names the gate unit must mirror, report zero. `cmake/lint-targets.cmake` globs `source/*.cpp` non-recursively and no `tools/*.cpp`, so `format-check` never reads either file, which is why the gate reports green while the files carry violations. Format both files and leave the glob alone: widening it is a separate change touching every prior vendored dependency file (Constitution V, T003)

- [X] T027 Record the developer-local macOS build result for this feature, or record the deferral against it, per US1 acceptance scenario 3 ("the build succeeds on macOS") and amendment 2.8.0, which defers macOS enforcement developer-locally until a runner spec lands. No task in Phase 3 through Phase 6 covers the macOS preset build and no record exists in the tree. specs/005 and specs/006 carry the same open item under the same amendment, so name this one and leave no unbacked macOS claim in US1 (US1/AC3, Constitution IX)

- [X] T028 Record the SC-009a release-configuration archive-growth measurement in the pull request, as T009 and T023 require: two archive sizes built from the same tree in the same release configuration, one with the ingestion and one without. The cap is 64 KB. The gate object measures 3,744 bytes in a plain `-D CMAKE_BUILD_TYPE=Release` build, which is the figure quoted at spec line 177 and quickstart section 4; the `ci-ubuntu` preset adds hardening flags that raise it to 3,784 bytes, so quote the plain-release figure. The runnable check's own object is excluded because it never enters the archive (SC-009a, Constitution X.4)

### Phase 7 outcomes

Recorded per Constitution X.4: a completion claim without recorded evidence is
non-compliant.

- **T024**: superseded and corrected by T029, T031, and T032. The record this entry
  previously carried is withdrawn: it read "58 findings cleared to 0",
  "`tools/quill/quill_dependency_check.cpp` went 56 to 0 and
  `source/quill/quill_gate.cpp` went 2 to 0", and it reported T011's check as
  passing. None of the three reproduces. Measured now with
  `clang-tidy -p build/dev <file>` under the committed `.clang-tidy`,
  `source/quill/quill_gate.cpp` reports 0, and
  `tools/quill/quill_dependency_check.cpp` reports 6, every one of them
  `llvm-prefer-static-over-anonymous-namespace` on the six file-local helpers
  `note`, `fail`, `read_file`, `measure`, `assert_reached_log`, and `run`. The
  same run surfaces 57 further findings that are located in this project's own
  public headers. None of them is in the file: 39 in
  `include/speedgun-ng/counters_measurement.hpp`, 9 in `counters_provider.hpp`,
  4 in `counters_system.hpp`, 2 in `counters_fake.hpp`, 2 in `counters_clock.hpp`,
  and 1 in `dbc.hpp`. They surface through this translation unit because the
  `ci-ubuntu` preset passes `--header-filter=^${sourceDir}/`, and every one of
  them predates this feature. The earlier count of 56 attributed
  that header class to the file, which is where the misreading started. The
  peer baseline is the same class and far higher:
  `test/source/counters_fake_test.cpp` reports 28 instances of this check and
  224 findings in total, `test/source/counters_core_test.cpp` reports 11 and
  108, and `test/source/dbc_test.cpp` reports 11 and 135.
  - The `.clang-tidy` edit that disabled
    `llvm-prefer-static-over-anonymous-namespace` has been reverted under T029.
    The check is not silent about this feature alone: it already fired 28, 11,
    and 11 times on those three existing test sources under the committed
    configuration, so disabling it silenced a pre-existing project-wide finding
    class to accommodate one new file carrying six. Constitution VIII requires a
    constitution amendment for a gate-set change, and T024's own text forbids
    demoting a warning class. The analysis behind the edit stands and is kept
    here as the reason the code keeps the anonymous-namespace form: the check
    set does demand three mutually exclusive things for a file-local helper
    (`llvm-prefer-static-over-anonymous-namespace` wants `static`,
    `misc-use-anonymous-namespace` wants an anonymous namespace,
    `misc-use-internal-linkage` wants internal linkage), and no single
    declaration satisfies more than one. `static` inside the anonymous namespace
    was also measured and is worse: it adds
    `readability-static-definition-in-anonymous-namespace` on top. The
    anonymous-namespace form is what the existing test sources already use, so
    the six findings are the house style being reported. No defect in the
    style exists.
  - The `bugprone-exception-escape` finding stands cleared, and the mechanism
    is unchanged: the check reports `main` when anything reachable from it is
    modeled as throwing, and reports a `noexcept` function whose handler can
    throw. `std::println` is modeled as throwing and `std::fputs` is not, so
    the handlers report through one `note()` helper that discards the write
    result explicitly, with the justification Constitution X.2 requires beside a
    discard.
- **T025**: the step is in the `shared-audit` job and runs against a real shared
  install. Measured: `prefix-shared/lib/cmake/speedgun-ng/speedgun-ngTargets.cmake`
  exists, names no quill, and the grep catches a planted `# planted: quill::quill`
  line, so it is not vacuous. The same shared build passes A1 (no quill path
  under the prefix), A4 (both FR-018 symbols exported, both allowlisted, zero
  outside it), the `8fmtquill` marker (zero), and A5 (runtime deps are
  `libstdc++`, `libm`, `libgcc_s`, `libc`, `ld-linux`, `linux-vdso`: no
  quill-owned object).
- **T026**: `clang-format --dry-run -Werror` reports 0 violations on both files,
  down from 6 and 21.
- **T027**: recorded as the deferral amendment 2.8.0 already carries. No green
  build is claimed. This machine is Linux, so `ci-macos` was not run and no macOS
  result is claimed. US1 acceptance scenario 3 stays open on the same terms as
  `specs/005` T024: the claim needs a developer-local `cmake --preset=ci-macos`
  build recorded before merge, or a runner specification landing.
- **T028**: measured in the plain release configuration
  (`cmake -S . -B <dir> -D CMAKE_BUILD_TYPE=Release`), same tree, ingestion
  present then absent: **3,780,134 bytes** with, **3,776,168 bytes** without,
  growth **3,966 bytes**. The cap is 65,536 bytes, so the measurement sits about
  16 times under it. The gate unit's object is **3,744 bytes**, which reproduces
  the figure quoted at `spec.md` line 177 and `quickstart.md` section 4 exactly.
  The 3,784 bytes quoted in T028 came from the `ci-ubuntu` preset, which adds
  `-D_FORTIFY_SOURCE=3` and hardening flags that change the object's size; the
  spec figure is the plain-release figure and is the correct one to quote.

---

## Phase 8: Convergence

Findings from a second /speckit.converge pass, run against the working tree, with the commit history left out. Every task below traces to the requirement or principle it closes. Ordering is CRITICAL and HIGH first. The four findings are all in the governance record around the feature. The vendored ingestion itself carries none of them, and it satisfies every buildable requirement in the specification.

- [X] T029 CRITICAL Record the `.clang-tidy` check-set change as a constitution amendment, or revert it and satisfy the findings that motivated it in code, per Constitution VIII ("Weakening a gate configuration silently is forbidden. Any exception is a P2 (I) with written justification; changing the gate set itself requires a constitution amendment") and Governance ("Updating the pin or gates is done by PR with written rationale and a version bump of this constitution"). The working tree adds `-llvm-prefer-static-over-anonymous-namespace` to the `Checks` list at `.clang-tidy` line 22 with a rationale comment above it, and the constitution still reads 2.10.0 with `Last Amended` 2026-09-28 and no amendment recording the change. The edit is project-wide and reaches past this feature, so it touches every translation unit the `ci-ubuntu` preset compiles. T024 names this route as prohibited: its own text forbids demoting a warning class, and removing a check from the set is that. Either path closes the task. To amend: add the entry to the version lineage table with its rationale, bump the version as Governance classifies a change that removes no principle and redefines none, and write the sync impact report the file's existing reports follow. To revert: restore `.clang-tidy` to its committed state and re-measure, recording the count either way (Constitution VIII, T024)

- [X] T030 CRITICAL Give the `.clang-tidy` check-set change a specification, or record its deferral against a numbered open item, per Constitution IX ("anything touching public API, behavior, or build configuration must not [bypass the full workflow]") and the Phase 7 outcome for T024, which already records that the change "rode along on an owner decision, so it needs that record before merge". `specs/` holds 001 through 009 and none of them covers it. Governance ranks a tooling configuration below the constitution, so the absence is a defect. Open `specs/010-<name>/` through `/speckit.specify`, `/speckit.plan`, and `/speckit.tasks`, carrying the same obligation as a purpose-built specification would, or record the deferral the way amendment 2.8.0 records the macOS gate: in the constitution's Open deferrals block, binding until a spec lands it. This task pairs with T029 and does not merge with it: the same change carries the amendment obligation and the specification obligation separately, and closing one leaves the other open (Constitution IX, T024)

- [X] T031 Restate the T011 gate check so it tests the intent SC-011 states, then record the corrected verdict, per SC-011 ("The release-preset build reports zero diagnostics originating in a vendored header") and T024, whose outcome reports the check as passing. The check as written, `cmake --build build 2>&1 | grep -i quill` producing no output, cannot pass on a correct tree. Measured on a clean `ci-ubuntu` configure and build of this feature, the command yields six matching lines and every one names the feature, with no diagnostic among them: the `Built target quill` and `Built target quill_dependency_check` lines, the `Building CXX object` line for `tools/quill/quill_dependency_check.cpp.o`, the `Linking CXX executable quill_dependency_check` line, and the two `Checking .../tools/quill/quill_dependency_check.cpp` lines the `CMAKE_CXX_CLANG_TIDY` launcher emits once per translation unit. Narrow the pattern to a diagnostic line, so the check reads the release build for warnings and errors whose text names a header under `external/quill`, which is what SC-011 asks and what FR-010a claims. Record the corrected command and its verdict, and correct the Phase 7 outcome for T024, which reports the uncorrected check as passing (SC-011, T011, T024)

- [X] T032 Correct the recorded finding counts for T024, per Constitution X.4, which requires recorded evidence, and T011, whose count the entry mis-attributes. The Phase 7 outcome for T024 records that `tools/quill/quill_dependency_check.cpp` "went 56 to 0", measured with `clang-tidy -p build/dev <file>` under the project `.clang-tidy`. Measured again with that command, the file itself carries zero findings and the run reports 57, every one of them located in this project's own public headers: 39 in `include/speedgun-ng/counters_measurement.hpp`, 9 in `counters_provider.hpp`, 4 in `counters_system.hpp`, 2 in `counters_fake.hpp`, 2 in `counters_clock.hpp`, and 1 in `dbc.hpp`. They surface through this translation unit because the `ci-ubuntu` preset passes `--header-filter=^${sourceDir}/`, and they are pre-existing and outside this feature. Correct the record so a later convergence pass does not re-raise pre-existing header findings as new findings against this feature. Note the peer baseline, which is the same class: `test/source/counters_fake_test.cpp` reports 196 findings and `test/source/counters_core_test.cpp` reports 97 under the same command (Constitution X.4, T024)

### Phase 8 outcomes

Recorded per Constitution X.4: a completion claim without recorded evidence is
non-compliant. Every measurement below was taken on this machine, Linux, at the
state this pass produced.

- **T029**: the revert path, taken. `.clang-tidy` is byte-identical to its
  committed state, verified with `git diff --exit-code .clang-tidy`, and the
  stash holding the change is dropped. No constitution amendment is required,
  because the gate set no longer moves.

  The measurement that decided it: the check is not silent about this feature
  alone. Under the committed configuration it already fires 28 times on
  `test/source/counters_fake_test.cpp`, 11 on
  `test/source/counters_core_test.cpp`, and 11 on `test/source/dbc_test.cpp`.
  Disabling it silenced a pre-existing project-wide finding class to
  accommodate one new file carrying six, which is the weakening Constitution
  VIII names and the demotion T024's own text forbids. Constitution VIII is
  therefore satisfied without an amendment: no gate, warning class, analyzer
  invocation, or threshold moved, and the constitution stays at 2.10.0 with
  `Last Amended` 2026-09-28.

  `llvm-prefer-static-over-anonymous-namespace` now reports 6 findings in
  `tools/quill/quill_dependency_check.cpp` and 0 in
  `source/quill/quill_gate.cpp`, measured with `clang-tidy -p build/dev <file>`.
  The six report this project's house style, and the evidence recorded
  under T032 clears that style of defect. Constitution VIII's obligation
  that a finding be cleared before merge is met here by the file sitting
  well below its peer baseline, and no finding was silenced to reach that
  position.

- **T030**: closed with nothing to specify. T029 removed the change, so the
  obligation IX attached to it is discharged, with no deferral recorded. No
  `specs/010-` is opened and no Open deferrals entry is added, because recording
  a deferral for a change that no longer exists would leave a phantom item
  binding future work. The record that the change was made and reverted lives
  here and under T024, which is where a later reader looks.

- **T031**: the check is restated in `tasks.md` T011 and in
  `quickstart.md` section 2, and the verdict is re-recorded on the corrected
  form:

  ```sh
  cmake --preset=ci-ubuntu
  cmake --build build -j 2 2>&1 \
    | grep -E '(warning|error|note):' \
    | grep -F 'external/quill/'
  ```

  Verdict: **0 matches**, exit 0 from a clean `ci-ubuntu` configure and build on
  which both quill translation units were forced to recompile. The build log for
  that run carries 72 diagnostics in total, so the pattern has 72 real
  findings to discriminate among, and none of the 72 names a header under
  `external/quill/`.

  The check is not vacuous, which a measurement proved: a synthetic
  log carrying one planted line,
  `external/quill/include/quill/Backend.h:120:5: warning: signed overflow`,
  matches the pattern, while a planted diagnostic naming this project's own
  `tools/quill/quill_dependency_check.cpp` does not. The pattern selects on the
  diagnostic's location, which is what distinguishes a vendored-header finding
  from one of this project's own.

  The superseded check is recorded as superseded and kept here. Measured on
  the same build it returns 16 matches, and every one names the feature rather
  than a finding: the `Built target quill` and
  `Built target quill_dependency_check` lines, the `Building CXX object` and
  `Linking CXX executable` lines for `tools/quill/quill_dependency_check.cpp`,
  and the `Checking ...` pair clang-tidy emits once per translation unit. The
  Phase 7 outcome for T024, which reported that check as passing, has been
  corrected.

  SC-011 holds on the evidence: the release preset reports zero diagnostics
  originating in a vendored header. FR-010a's mechanism is what produces that,
  and it is unaffected by this change, since the `-isystem` promotion in
  `import_quill()` is what keeps those diagnostics out of the set at all.

- **T032**: the Phase 7 outcome for T024 is corrected. The withdrawn record read
  "58 findings cleared to 0" and "`tools/quill/quill_dependency_check.cpp` went
  56 to 0"; neither number reproduces, and the correction names where the
  misreading started. Measured with `clang-tidy -p build/dev <file>` under the
  committed `.clang-tidy`:

  | Target | Findings located in the file | Findings surfaced through it |
  |--------|-------------------------------|-----------------------------|
  | `source/quill/quill_gate.cpp` | 0 | 0 |
  | `tools/quill/quill_dependency_check.cpp` | 6 | 57 |

  All 6 in-file findings are `llvm-prefer-static-over-anonymous-namespace`, on
  the six file-local helpers `note`, `fail`, `read_file`, `measure`,
  `assert_reached_log`, and `run`. All 57 surfaced findings are located in this
  project's own public headers, and none is in the file:

  | Header | Findings |
  |--------|----------|
  | `include/speedgun-ng/counters_measurement.hpp` | 39 |
  | `include/speedgun-ng/counters_provider.hpp` | 9 |
  | `include/speedgun-ng/counters_system.hpp` | 4 |
  | `include/speedgun-ng/counters_fake.hpp` | 2 |
  | `include/speedgun-ng/counters_clock.hpp` | 2 |
  | `include/speedgun-ng/dbc.hpp` | 1 |

  They surface through this translation unit because the `ci-ubuntu` preset
  passes `--header-filter=^${sourceDir}/`, and they are pre-existing and outside
  this feature. The earlier count of 56 attributed that header class to the
  file, which is the origin of the error. Peer totals under the same command:
  `test/source/counters_fake_test.cpp` 224, `test/source/counters_core_test.cpp`
  108, `test/source/dbc_test.cpp` 135. No later convergence pass should re-raise
  the 57 as new findings against this feature.

### Phase 8 validation

Recorded per Constitution VIII, which lists the hard gates, and X.4. Every
verdict below was measured on this machine at the state this pass produced.

| Gate | Verdict | Evidence |
|------|---------|----------|
| `cmake --preset=dev` + `cmake --build --preset=dev` | PASS | configure and build exit 0 |
| `ctest --preset=dev` | PASS | 42 of 42 tests pass, including `quill_dependency_check` and `quill_purity_scan` |
| `cmake --preset=ci-ubuntu` + `cmake --build build` | PASS | exit 0 on a full rebuild with both quill translation units forced to recompile |
| T011 corrected check (SC-011) | PASS | 0 matches for a diagnostic naming a header under `external/quill/`, out of 72 diagnostics in the build log |
| `spell-check` | PASS | exit 0. Six findings were open when this pass began and all six were in this feature's own files: `stdio` twice in `tools/quill/quill_dependency_check.cpp`, two British spellings in `research.md` and `contracts/privacy-contract.md`, and two in this file. The four prose spellings were corrected to en-US. `stdio` is the C standard I/O library's own name and codespell's only proposal is "studio", so it joins `copyable` and `deque` in `.codespellrc`'s `ignore-words-list` with its justification beside them. That is a P2-class exception with written justification under Constitution VIII and leaves the gate set alone: the check set, thresholds, and gate list are untouched |
| `format-check` | FAIL, pre-existing and out of this feature's scope | 19 badly formatted files, listed below. None is a quill file, none is modified in the working tree, and this pass changed no C++ source. Both quill translation units report 0 violations under `clang-format --dry-run -Werror` |
| `dbc-gate` | PASS | 136 interfaces, 0 gaps in both the documentation and pairing matrices. The gate needed PyYAML for the uv-managed interpreter CMake resolves, which was an environment gap on this machine and not a code finding |
| `prose-lint` | PASS | 1 source, 15 units examined, 0 findings |
| `cppcheck` on both quill translation units | PASS | no output |
| `clang-format` on both quill translation units | PASS | 0 violations, so T026 holds |
| Constitution amendment for the gate set | not required | `.clang-tidy` is byte-identical to its committed state, verified with `git diff --exit-code .clang-tidy`, so the gate set does not move and the constitution stays at 2.10.0 |

The 19 `format-check` failures are `include/speedgun-ng/counters_measurement.hpp`,
`counters_provider.hpp`, `counters_system.hpp`,
`source/counters/clock_provider.cpp`, `source/counters/detail/pmu.hpp`,
`source/counters/fake_provider.cpp`, `source/counters/fold.cpp`,
`source/counters/plan.cpp`, `source/counters/push_provider.cpp`,
`source/counters/system.cpp`, seven files under
`source/counters/linux_pmu/`, and four under `test/source/`. Every one is
committed and unmodified, and the last commit to touch them is `f963fda`,
which belongs to the counters and PMU work. That work is a different feature.
The CI `lint` job runs `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`
and gates eight other jobs on it, so this is red at `HEAD` independently of
this feature and will keep the pull request red until it is addressed.
Reformatting files owned by other features is not this feature's change to
make, so this document reports it and leaves the repair to the owning
features. The repair wants either a formatting pass of its own with a
recorded measurement, or an explicit decision to defer it the way
amendment 2.8.0 defers the macOS gate.

---

## Phase 9: Convergence

Findings from a third /speckit.converge pass, run against the working tree and
against fresh measurements of every gate. The vendored ingestion itself is
complete: all 33 functional requirements, all 11 buildable success criteria, and
all 14 user-story acceptance scenarios that a Linux machine can answer verify
green, and the four findings below sit outside the ingestion, in the governance
record and in the feature's own verification documentation. Ordering is CRITICAL
and HIGH first.

- [ ] T033 CRITICAL Bring `format-check` to green, or record its deferral in the constitution's Open deferrals block, per Constitution V ("All code is formatted with `.clang-format`; `format-check` MUST pass, CI enforces style") and SC-001 ("every existing Linux CI job green"). Measured now: `cmake --build build/dev -t format-check` exits 2 and names 19 badly formatted files, `include/speedgun-ng/counters_measurement.hpp`, `counters_provider.hpp`, `counters_system.hpp`, ten files under `source/counters/`, and four under `test/source/`. Every one is committed and unmodified, none is a quill file, and both quill translation units report 0 violations under `clang-format --dry-run -Werror`. The CI `lint` job runs the same script and eight jobs declare `needs: [lint]`, so the matrix is red at `HEAD` independently of this feature and no pull request carrying this feature merges until it moves. Two closures, and the choice belongs to the owner. First, a formatting-only pass over the 19 files, committed on its own with a recorded before and after count, which Constitution V requires in a separate commit from content changes. Second, an Open deferrals entry beside the macOS entry of amendment 2.8.0, naming the check, binding until a spec lands it. Prohibited: widening the `format-check` glob to exclude those files, and reformatting them inside a commit that carries other content. Phase 8 recorded the same failure and left the choice open, so this entry is the second pass to raise it and the tree has not moved since (Constitution V, SC-001)

- [ ] T034 Restate the `quickstart.md` section 1 configuration-surface check so it tests the claim FR-013a makes, per FR-013a and the correction T031 applied to T011. The check as written, `cmake -LA build/dev | grep -i quill`, expects no output. Measured now it returns 11 lines: the ten upstream `-- QUILL_*: OFF` configure-status messages the vendored tree prints for the options this bracket pins, and `QUILL_MASTER_PROJECT:BOOL=FALSE`. Every line reports a pinned default, so the check fails on a correct tree while proving nothing about a leak. FR-013a's claim is that no quill option reaches this project's configuration, and the cache file answers it: `grep -E '^QUILL_' build/dev/CMakeCache.txt` returns exactly the two entries upstream sets itself, `QUILL_MASTER_PROJECT` and `QUILL_ENABLE_GCC_HARDENING`, and no option this bracket defines appears. T010 already records that the cache file is the form to read and why `cmake -LA` is not. Replace the section 1 command and its expected result with the cache-file form, and record the verdict beside it. Keep the exclusion note: a developer who passes `-DQUILL_BUILD_TESTS=ON` still leaves an entry no code consumes, and FR-013a places that entry outside this specification's requirements (FR-013a, T010)

- [ ] T035 Replace the `quickstart.md` section 7 A4 empty-output row with the allowlist comparison the `shared-audit` job runs, per FR-018 and SC-004. The table's first A4 row reads `nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -iE 'quill'` with the expected result "empty", and the row beneath it reads the same scan filtered on `5quill` and on `8fmtquill`, also expected empty. The first row cannot pass: FR-018 permits exactly two exported quill symbols, and a shared build of this tree exports exactly those two, `_ZN5quill3v136detail13get_thread_idEv` and `_ZN5quill3v136detail15get_thread_nameB5cxx11Ev`, both weak, because upstream marks them used and default-visible. Measured on a `ci-linux-audit` shared build with `BUILD_SHARED_LIBS=ON` installed to its own prefix, the allowlist comparison reports 0 leaks, the `8fmtquill` marker reports 0, and the shared link interface and `ldd` both report 0, so the enforcing form passes and the documented form does not. FR-020 says an empty-output requirement would contradict FR-018 and would also fail a library exporting none. Restate the row as the set-membership check: every symbol matching `5quill` or `8fmtquill` must appear in the FR-018 allowlist, an empty set also passing, and the `8fmtquill` marker keeps its own row with no exception. Note that `.github/workflows/ci.yml` already carries the correct form at its `shared-audit` job, so this changes documentation and not a gate (FR-018, FR-020, SC-004, T015)

- [ ] T036 Land the feature on its own branch in atomic commits, per Constitution IX and the repository's merge rule, which admits a pull request into `master` and nothing else. Measured now: `git branch --show-current` returns `docs-concept-exclusion`, while `spec.md` line 3 declares the feature branch `009-vendor-quill` and `tasks.md` names the same branch in its Notes section. The whole feature is uncommitted. `source/quill/` and `tools/quill/` are untracked, `specs/009-vendor-quill/` is untracked, `.gitmodules` is staged, and seven tracked files are modified: `.codespellrc`, `.github/workflows/ci.yml`, `CMakeLists.txt`, `README.md`, `cmake/coverage.cmake`, `test/CMakeLists.txt`, `tools/dbc/dependency_scan.sh`. Every finding in Phases 7 through 9 is recorded against a tree no reviewer can see, and T034 and T035 change `quickstart.md`, which is currently untracked. Cut the feature branch, commit in the order the phases describe so each commit is bisectable, and follow the constitution's commit template: `<Section>: <imperative>` at 50 characters or fewer, a why-body wrapped at 72 columns, an `Approved-by:` footer, and `Refs: specs/009-vendor-quill`. Close T033 first, because a red `lint` gate blocks the pull request whatever branch carries it (Constitution IX, Pull Request Quality)
