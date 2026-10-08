# Feature Specification: Identifier Naming CamelCase

**Feature Branch**: `014-identifier-naming-camelcase`

**Created**: 2026-10-07

**Status**: Draft

**Input**: User description: "Identifier naming refactor: PascalCase types and lowerCamelCase functions across the project. The maintainer chose one naming scheme for the whole project before the benchmark harness adds a large public surface. The rename lands as one breaking change to the public API, and the harness starts on the new names. The naming rules are constitutional law."

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `6d32efcab3c13c3d41470c1c621d78839ce11543` |
| Audit-point date | 2026-10-06 19:39:34 -0500 |
| Default branch | `master` (`origin/master` after fetch) |
| Feature number | 014, the next free number under `specs/` |
| Short name | `identifier-naming-camelcase` |
| Constitution version | 2.13.0, last amended 2026-10-04 |
| Project version | 0.4.1 |
| Shared-object version | 1 |
| Package compatibility | `SameMinorVersion` |

The feature branch is `014-identifier-naming-camelcase`. It starts at the audit point.

Citations of the form `file:line` name the audit-point tree. A later edit can move a line. The plan re-anchors any citation it uses.

### Preconditions

PC-1 holds. `specs/012-counters-defect-resolution/tasks.md` has 131 complete tasks and no open task. `specs/013-counters-defect-followup/tasks.md` has 63 complete tasks and no open task. The version list in `specs/013-counters-defect-followup/plan.md` records the 0.4.1 patch.

PC-1a holds. `git cherry origin/master` reports no unmerged commit on these remote branches:

| Branch | Patch-equivalent commits | Unmerged commits |
| --- | --- | --- |
| `origin/012-counters-defect-resolution` | 9 | 0 |
| `origin/013-counters-defect-followup` | 5 | 0 |
| `origin/fix-013-review-defects` | 2 | 0 |

Each branch was rebase-merged. A branch with no unmerged commit carries no parallel rename work. Deletion of these branches is the maintainer's decision. This feature does not delete them.

PC-2 holds. No local branch and no remote branch has a harness name.

PC-4 holds. `CMakeLists.txt` sets the project version to 0.4.1 and the shared-object version to 1. `cmake/install-rules.cmake` writes `SameMinorVersion`. The follow-up documents call the shared-object version a hand-kept number. The rule at the audit point matches that account. FR-008 follows that rule.

PC-3 does not hold. Continuous Integration run [37553123469](https://github.com/charlesarcher/speedgun-ng/actions/runs/37553123469) on the audit point finished with a format-check failure. The lint job listed 21 badly formatted files. Every other job was skipped. The audit point therefore has no passing test set, no remaining gate results, and no name-check finding count per translation unit.

The 21 files are:

- `include/speedgun-ng/counters_measurement.hpp`
- `include/speedgun-ng/counters_provider.hpp`
- `include/speedgun-ng/counters_system.hpp`
- `source/counters/clock_provider.cpp`
- `source/counters/detail/pmu.hpp`
- `source/counters/fake_provider.cpp`
- `source/counters/fold.cpp`
- `source/counters/linux_pmu/embedded_tables.hpp`
- `source/counters/linux_pmu/encode.cpp`
- `source/counters/linux_pmu/fast_read.cpp`
- `source/counters/linux_pmu/group_io.cpp`
- `source/counters/linux_pmu/provider.cpp`
- `source/counters/linux_pmu/table_parse.cpp`
- `source/counters/plan.cpp`
- `source/counters/push_provider.cpp`
- `source/counters/system.cpp`
- `test/source/counters_clock_push_test.cpp`
- `test/source/counters_linux_pmu_seam_test.cpp`
- `test/source/counters_noalloc_test.cpp`
- `test/source/counters_pmu_test.cpp`
- `test/source/counters_provider_ext_test.cpp`

The format repair changes no identifier. tasks.md T001 records the 21 paths and repairs the format before any rename commit. The plan names the gate baseline before the first rename commit. The gate baseline is a default-branch commit where every hard gate passes and the rename has not started. Comparisons in FR-001, FR-002, and FR-003 use the gate baseline. They do not use the audit point, because the audit point has no passing gate record.

## Clarifications

### Session 2026-10-07

- Q: Which renamed names must the migration map include? → A: Shipped-header names, including detail

## User Scenarios & Testing

### User Story 1 - One naming law (Priority: P1)

The maintainer opens the constitution and finds every naming rule, the constant spelling, and the full exception list in that text. A configuration file, a documentation page, and a spec hold no naming rule on their own authority. A later harness author reads the same text and uses it.

**Why this priority**: The harness is the next feature. It adds a public surface. One law must exist before that surface exists.

**Independent Test**: A reviewer compares the constitution text with the rule list and the exception list in this specification. Each rule appears. Each exception appears. Each name-check setting matches the rule it implements.

**Acceptance Scenarios**:

1. **Given** the constitution at the audit point, **When** the amendment lands, **Then** the constitution states rules N-1 to N-11, the constant spelling, and the exception list.
2. **Given** the amended constitution, **When** a reviewer compares `.clang-tidy` with that text, **Then** each naming key matches the rule it implements.
3. **Given** a guidance file that states a naming convention, **When** the amendment lands, **Then** that file states the same convention as the constitution, or it drops the sentence and points at the constitution.
4. **Given** the Governance section, **When** the amendment lands, **Then** the file carries a Sync Impact Report, a version lineage row, a new version, and a new Last Amended date.

### User Story 2 - Rename without a behavior change (Priority: P1)

The maintainer renames every identifier the project owns. A caller uses the new spellings. Counter readings, catalog names, messages, and machine code stay the same after symbol names are normalized. The name check reports zero findings, and a finding fails the build.

**Why this priority**: The public names are the break. The harness starts on those names. A behavior change would make the break unsafe.

**Independent Test**: The test set at the gate baseline passes at the rename head. The normalized machine-code comparison reports no difference. The name check reports zero findings.

**Acceptance Scenarios**:

1. **Given** the gate baseline, **When** the rename sequence completes, **Then** every test that passed there passes, and no test is removed or weakened.
2. **Given** a release-preset object file built with contracts ignored, **When** symbol names are normalized, **Then** its machine-code text matches the gate-baseline twin. The known contract-text difference in FR-002 is the one permitted difference.
3. **Given** a translation unit after the rename, **When** the name check runs, **Then** it reports zero findings, and no other check gains a finding in that unit.
4. **Given** one commit in the sequence, **When** that commit is built, **Then** it compiles, its tests pass, and the format check passes.
5. **Given** a string literal whose text once matched an identifier, **When** the rename completes, **Then** that literal keeps its text.

### User Story 3 - A reader can migrate (Priority: P2)

A downstream reader sees a new project version and a higher shared-object version. The reader opens the rename map and finds every shipped-header spelling that changed. A package request for 0.4 rejects a 0.5 package.

**Why this priority**: The rename breaks the public API and the ABI on the 0.x line. The reader needs a map and a version that records the break.

**Independent Test**: The version fields match the decision in FR-008. A search finds no old shipped-header spelling outside the rename map and the exception list.

**Acceptance Scenarios**:

1. **Given** the rename head, **When** a reader checks the version list, **Then** the project version is 0.5.0 and the shared-object version is 2.
2. **Given** the published rename map, **When** a search runs over the owned trees and the documents in FR-015, **Then** every changed shipped-header spelling appears in the map. A detail name counts. A test-only name stays out of the map.
3. **Given** a package request for version 0.4, **When** the consumer resolves a 0.5 package, **Then** the request rejects that package.

### User Story 4 - Stale sentences match the live rule (Priority: P3)

A reader of the live overhead page no longer meets the name of a removed probe. A reader of the follow-up specification meets the hand-kept shared-object rule, and the successor log records the closed-directory edit.

**Why this priority**: The two sentences are documentation. They do not block the rename. The rename edits documents anyway, and the follow-up sentence states a rule this feature follows.

**Independent Test**: The overhead page no longer names `pmu_probe_fast`. The follow-up specification states the hand-kept rule. The successor log has an entry for the edit.

**Acceptance Scenarios**:

1. **Given** `docs/pages/counters-overhead.md` at the audit point, **When** the rename completes, **Then** line 299 no longer names `pmu_probe_fast`.
2. **Given** the clarify answer at `specs/013-counters-defect-followup/spec.md:891`, **When** the logged edit lands, **Then** that answer calls the shared-object version a hand-kept number.
3. **Given** the same sentence at `specs/013-counters-defect-followup/checklists/requirements.md:44`, **When** the logged edit lands, **Then** that sentence matches the spec answer.
4. **Given** the successor log, **When** the edit lands, **Then** one entry records the closed-directory correction and points to the rename map.

### Edge Cases

- A short enumerator can collide with a macro from a Linux header or a C library header. A macro replaces the token before the compiler reads the enumerator. FR-006 names the check. The highest-risk tokens at the audit point are `NONE`, `GAP`, `SYSCALL`, `CPU`, `THREAD`, `ABSENT`, `BYTES`, and `OPS`.
- An always-on contract stringifies its predicate. A renamed identifier inside that predicate changes the string. FR-002 records that difference and no other machine-code difference.
- A string literal keeps its text when that text once matched an identifier. The same holds for a catalog name, an object path, a provenance string, a unit token, a read-mode label, and an error message.
- The member `availability` and the type `availability` share a spelling at two sites. The qualification `::sg::counters::availability` exists for that clash. After the rename the type is `Availability` and the member stays `availability`. The qualification goes. The sites are `include/speedgun-ng/counters_core.hpp:289` and `include/speedgun-ng/counters_measurement.hpp:427`.
- A suppression that the new rules make unnecessary goes away. A suppression that remains names the exception entry it applies.
- The format check already fails at the audit point. A rename commit does not absorb that repair. The gate baseline lands first.
- Closed spec directories stay unedited, except the logged wording correction in User Story 4. Historical mentions of `pmu_probe_fast` in those directories stay.
- `cmake/variables.cmake` defaults `BUILD_SHARED_LIBS` to off. Vendored subdirectories force it off. `.github/workflows/ci.yml:251` configures a shared build, so the shared-object version is live in that job. The bump from 1 to 2 still records the break.
- The consumer job matches output lines `consumer: pmu catalog entries` and `embedded:`. Those strings stay.
- The namespace token in `tools/dbc/asm_smoke.sh` (`sg::dbc` and `_ZN2sg3dbc`) stays, because namespaces stay. The `sg::dbc::check_` match in `.github/workflows/ci.yml` names enforcement functions. That match updates with those functions.

## Requirements

### Functional Requirements

- **FR-001**: The rename shall change no observable behavior. Every test that passed at the gate baseline shall pass after the rename. No test shall be removed. No test shall be weakened. A test whose text names an identifier shall change that text alone.

- **FR-002**: Each release-preset object file, built with contracts ignored, shall have machine-code text identical to its gate-baseline twin after symbol names are normalized. Normalization replaces each owned symbol spelling with a stable token, strips the address column, and leaves instruction text and immediates. An address column is not a difference. An immediate that changes is a difference, apart from the contract-predicate string. One known difference is permitted: an always-on contract stringifies its predicate, and a renamed identifier inside the predicate changes that string. The plan states the normalization and the comparison.

- **FR-003**: The published sampling figures in `docs/pages/counters-overhead.md` shall stay within five percent of the gate-baseline figures on the reference host `Linux 7.2.4-1-cachyos x86_64`, AMD Ryzen 9 9950X3D.

- **FR-004**: The name check shall report zero findings across the tree after the rename. A finding at the rename head shall fail the build. No other static-analysis count of a translation unit shall rise. The name check is `readability-identifier-naming`. At the audit point `WarningsAsErrors` is empty, and the static-analysis gate reports findings against a baseline. Principle VIII states that changing the gate set requires a constitution amendment. The amendment in FR-009 adds this check to the hard gate list of Principle VIII.

- **FR-005**: Every hard gate shall pass at the rename head. The set covers the test set, the contract gate, the format check, the spell check, and the prose check. It covers the sanitizer presets, the thread-sanitizer job, and the leak audits. It covers the downstream consumer job, including the catalog comparison and the embedded-row comparison. It covers the line, branch, and contract coverage gates.

- **FR-006**: A new enumerator spelling or a new constant spelling shall not collide with a macro visible in any translation unit. The collision check includes the Linux and C library headers that a library translation unit includes. It then includes each public header. It fails when an enumerator token or a constant token is a defined macro. The check is one translation unit that includes each public header and the Linux headers a library translation unit includes. The checked set is every new enumerator and every new constant. The eight tokens in Edge Cases are in that set. The plan records the result.

- **FR-007**: The rename shall land as a bisectable sequence. Each commit shall compile, shall pass its tests, and shall rename one coherent group, for example one header and its users. A formatting change that a longer or shorter name forces shall go in the same commit as that name. A whitespace change with no renamed identifier shall stay out of the sequence. The format check shall pass at every commit. This reading covers the name-length reflow. It leaves Principle V's separate formatting commit in force for every other formatting change, including the audit-point format repair.

- **FR-008**: The public API change shall carry a version bump. The rename changes every public name and every mangled symbol, so it breaks the API and the ABI on the 0.x line. The project version shall move from 0.4.1 to 0.5.0. The shared-object version shall move from 1 to 2. `SameMinorVersion` shall stay. A 0.4 request shall reject a 0.5 package. The version table in this feature's plan shall record the rename. Closed plan lists stay unedited. The chosen reading and the rejected reading are in Decisions.

- **FR-009**: The feature shall amend `.specify/memory/constitution.md`. The amendment shall write every naming rule (N-1 to N-11), the constant spelling, and the full exception list into the constitution text. A rule that exists only in `.clang-tidy`, a document, or this specification fails this requirement. The amendment shall resolve the conflict with X.3. The naming rules of Principle V shall govern every file. Local style shall yield on naming. Local style shall still govern indentation, comment conventions, and file organisation.

- **FR-010**: The amendment shall follow the Governance procedure at the audit point. It shall record the rationale. It shall bump the constitution version. It shall write a Sync Impact Report at the head of the file. It shall add a version lineage row. It shall set a new Last Amended date. The constitution version shall become 2.14.0. The chosen reading and the rejected reading are in Decisions.

- **FR-011**: The constitution shall be the single source of truth for naming. Every other naming source shall derive from it and shall match it. A guidance file matches by stating the same convention, or by dropping that sentence and pointing at the constitution. The sources are the naming keys in `.clang-tidy`, with prefixes and ignore patterns, and `.clang-format`. They also include `docs/pages/`, `README.md`, `AGENTS.md`, every agent skill, rule, or guidance file, and every Spec Kit template that states a naming convention. On a conflict the constitution wins. At the audit point `.clang-format` holds no identifier-naming key. The Case keys in that file govern layout, and they stay. The audit search found identifier-case keys only in `.clang-tidy`.

- **FR-012**: Every later spec shall obey the rules, starting with the benchmark harness. A deviation shall require a constitutional amendment. A spec, a plan, a local naming override, or a suppression comment shall not create a deviation. A suppression shall name the constitution exception entry it applies.

- **FR-013**: Every identifier the project owns in `include/`, `source/`, `test/`, `example/`, and `tools/` shall follow the naming rules. The set includes the names added by specs 012 and 013 and by the 0.4.1 patch. Examples confirmed at the audit point: `availability::scope_refused`, `availability::gap`, `point_sink::put_disclosure`, `target_mask`, `target_thread_bit`, `no_disclosure_column`, `m_column_count`, `first_index_of`, and `device_page_fast_verdict`.

- **FR-014**: The exception list below shall be the only exception list. The amendment shall carry it into the constitution. The specification shall list each applied exception with its reason. At the audit point the public headers declare one standard-protocol name, `empty()`, at `include/speedgun-ng/counters_measurement.hpp:124`. The reason is the standard container protocol.

- **FR-015**: A rename map shall be published in this feature directory and in the documentation. Each entry pairs an old spelling with a new spelling. A shipped header is a header under `include/` that the package installs. The map shall list every changed name declared in a shipped header. A name in a detail namespace counts. A test-only name stays out of the map. A file-local helper stays out of the map. FR-013 still renames those names. `specs/007-counters-and-timers/citations-log.md` shall gain one entry that points to the map. A search shall find no old shipped-header spelling outside the map and the exception list. The search matches identifier tokens. It skips a string literal, a catalog name, an object path, a provenance string, a unit token, a read-mode label, and an error message. The search covers `include/`, `source/`, `test/`, `example/`, `tools/`, `docs/`, `.github/workflows/`, `README.md`, and `AGENTS.md`.

- **FR-016**: The feature shall not change behavior, output text, catalog names, object paths, provenance strings, unit tokens, read-mode labels, or error messages. It shall not add a deprecated alias for an old name. It shall not rename a file, a CMake target, a CMake option, a preset, or the package. It shall not edit a closed spec directory, except the logged wording correction in FR-020.

- **FR-017**: Every gate, registry, script, and CI step that names a C++ identifier shall be updated in the same change as that identifier. The audit search found these hits:

  - `tools/dbc/macros.yaml` names `sg::dbc::check_precondition`, `sg::dbc::check_postcondition`, `sg::dbc::check_invariant`, and `sg::dbc::check_assertion`.
  - `test/counters_tsc_read_shape.sh` matches `read_points.*point_sink` and `probe_arm`.
  - `tools/dbc/asm_smoke.sh` matches the namespace `sg::dbc` and `_ZN2sg3dbc`. That token stays under N-7.
  - `test/consumer/embedded_count.cpp` calls `sg::counters::detail::pmu_load_table`.
  - `test/consumer/main.cpp` calls `sg::simulation_start`, `sg::counters::pmu_provider`, and `sg::counters::system::local`.
  - `.github/workflows/ci.yml` compiles both consumer sources, matches `consumer: pmu catalog entries` and `embedded:`, and matches `sg::dbc::check_`. The output strings stay. The function match updates.

  Implementation shall search every gate, script, and workflow again at the rename head and shall list each hit.

- **FR-018**: The qualification workaround at the two sites in Edge Cases shall go. The specification lists those two sites. A further site of the same clash, found at implementation, shall join the list and shall lose the qualification.

- **FR-019**: The rename shall remove each naming suppression that the new rules make unnecessary. The specification shall list each suppression that remains, with its exception entry. At the audit point the tree holds five naming suppressions. Under the adopted rules each one becomes unnecessary:

  - `include/speedgun-ng/dbc.hpp:43` on `Kind`
  - `include/speedgun-ng/dbc.hpp:59` on `ViolationRecord`
  - `include/speedgun-ng/dbc.hpp:66` on `predicateText`
  - `test/compile-fail/negative_runtime_only_constraint.cpp:14` on `kConstraint`
  - `test/compile-fail/positive/static_assert_constraint.cpp:15` on `kConstraint`

- **FR-020**: The feature shall correct two stale documentation claims. `docs/pages/counters-overhead.md:299` names `pmu_probe_fast`, a function the 0.4.1 patch removed. `specs/013-counters-defect-followup/spec.md:891` says the shared-object version takes the major position. The same sentence sits at `specs/013-counters-defect-followup/checklists/requirements.md:44`. The closed-directory edit shall be recorded in the successor log, beside the section "Corrections after the 013 merge". Historical mentions in other closed specs stay.

- **FR-021**: No rename commit shall land before the gate baseline exists. The plan shall name that commit. The plan shall record the passing test set, the gate results, and the name-check finding count per translation unit at that commit.

### Naming rules

These rules are constitutional. FR-009 carries each one into the constitution. The constitution wins on any difference from this list.

The numbers below are the spec's own and predate the constitution's final list. Principle V.1 of the constitution carries the numbering that governs, and a later spec, plan, or task cites those numbers. The mapping: spec N-1 to N-5 match; spec N-6 splits into constitution N-12 for a public aggregate member and constitution N-10 for the `m_` prefix; spec N-7 is constitution N-6; spec N-8 is constitution N-7; spec N-9 matches; spec N-10 lives inside constitution N-1; spec N-11 matches; the unnumbered named-constant paragraph below is constitution N-8. `rename-map.md` already carries the constitution's numbers.

- **N-1**: Types use PascalCase. The set covers classes, structs, unions, enumeration types, type aliases, concepts, and type traits. Examples at the audit point: `point_sink` becomes `PointSink`, `metric_result` becomes `MetricResult`, `points_view` becomes `PointsView`, `availability` becomes `Availability`, `target_mask` becomes `TargetMask`, `target_kind` becomes `TargetKind`, `read_mode` becomes `ReadMode`, `unit` becomes `Unit`. `Kind` and `ViolationRecord` already comply.

- **N-2**: Functions and methods use lowerCamelCase. The rule covers free functions, member functions, static functions, virtual functions, and constexpr functions. File-local helpers follow the same rule. Examples: `put_disclosure` becomes `putDisclosure`, `pmu_load_table` becomes `pmuLoadTable`, `check_precondition` becomes `checkPrecondition`, `first_index_of` becomes `firstIndexOf`, `device_page_fast_verdict` becomes `devicePageFastVerdict`.

- **N-3**: Macros use `UPPER_SNAKE_CASE` with the `SG_` prefix. The existing `SG_*` macros keep their names.

- **N-4**: Enumerators use `UPPER_SNAKE_CASE`. Examples: `availability::gap` becomes `Availability::GAP`, `availability::scope_refused` becomes `Availability::SCOPE_REFUSED`, `read_mode::fast_rdpmc` becomes `ReadMode::FAST_RDPMC`, `read_mode::syscall` becomes `ReadMode::SYSCALL`, `target_kind::cpu` becomes `TargetKind::CPU`, `unit::nanoseconds` becomes `Unit::NANOSECONDS`, `unit::none` becomes `Unit::NONE`.

- **N-5**: Variables and parameters use lowerCamelCase, local const variables included. The `point_sink` constructor parameter `column_count` becomes `columnCount`.

- **N-6**: Public data members of aggregates use lowerCamelCase with no prefix. Examples: `frequency_hz` becomes `frequencyHz`, `running_ratio` becomes `runningRatio`, `predicateText` stays `predicateText`. Private and protected data members keep the `m_` prefix with a lowerCamelCase remainder. `m_column_count` becomes `m_columnCount`.

- **N-7**: Namespaces stay lower_case. Examples: `sg`, `sg::counters`, `sg::dbc`, `detail`.

- **N-8**: Template parameters stay PascalCase.

- **N-9**: File names stay lower_case with underscores. Example: `include/speedgun-ng/counters_measurement.hpp`. CMake target names, CMake options, preset names, and the package name are no C++ identifiers and stay unchanged.

- **N-10**: An acronym in a name is one word. Examples: `Pmu`, `Tsc`, `Dbc`, as in `PmuWindow` and `tscTicks`.

- **N-11**: A tag type drops its `_t` suffix and takes PascalCase. `hard_stop_t` becomes `HardStop`. `ring_t` becomes `Ring`. The tag object beside the type uses lowerCamelCase. `hard_stop` becomes `hardStop`. `ring` stays `ring`.

Named constants use `kPascalCase`. The set covers constexpr variables and static const variables at namespace scope and at class scope, and it covers variable templates. Tag objects use lowerCamelCase under N-11. A public static const member or a constexpr member uses `kPascalCase`. N-6 applies to non-static data members of aggregates. `UPPER_SNAKE_CASE` stays limited to macros and enumerators. Examples: `kHeaderWords` stays, `target_thread_bit` becomes `kTargetThreadBit`, `no_disclosure_column` becomes `kNoDisclosureColumn`, `simulation_start_tag` becomes `kSimulationStartTag`, `dim_same` becomes `kDimSame`, `kConstraint` stays.

### Exceptions

A name that the language or the standard library looks up by spelling keeps that spelling. No exception exists outside this list.

- Standard container and range protocol: `begin`, `end`, `cbegin`, `cend`, `rbegin`, `rend`, `size`, `empty`, `data`, `swap`.
- Standard member types: `value_type`, `size_type`, `difference_type`, `reference`, `const_reference`, `pointer`, `iterator`, `const_iterator`, `iterator_category`, `iterator_concept`, `element_type`.
- Specializations in `std`: `std::hash`, `std::formatter`, `std::tuple_size`, `std::tuple_element`, and their members.
- Structured binding protocol: `get`.
- Operator functions, user-defined literal suffixes, and the names of standard functions the code calls.
- Names that a vendored library or the platform defines: hwloc, simdjson, HdrHistogram_c, yaml-cpp, zlib, quill, and Linux and POSIX names. The fields of `perf_event_mmap_page` and `perf_event_attr` are kernel names, for example `cap_user_rdpmc`, `time_mult`, and `pmc_width`.
- The export macro `SPEEDGUN_NG_EXPORT`. The export-header generator writes it from the target name, and it carries no `SG_` prefix.
- `main` in each executable, and every identifier inside `external/`.

### Key Entities

- **Naming rule**: One constitutional spelling rule, N-1 to N-11, plus the constant spelling. The constitution text holds the rule. The name-check setting implements it.
- **Exception entry**: One spelling the language, the standard library, a vendor, or the platform requires. A suppression cites the entry.
- **Identifier**: A C++ name the project owns. The rename map records each shipped-header name that changes.
- **Rename map**: The old spelling and the new spelling for each changed name declared in a shipped header, including a detail namespace. Closed specs cite the map through the successor log.
- **Gate baseline**: The default-branch commit that passes every hard gate before the first rename commit. FR-021 defines it. The audit point is not that commit.

## Success Criteria

### Measurable Outcomes

- **SC-001**: The passing test set at the rename head equals the gate-baseline set by name, and every test passes.
- **SC-002**: The normalized machine-code comparison of FR-002 reports no difference for any object file, apart from the recorded contract-text difference.
- **SC-003**: The sampling figures of FR-003 stay within five percent on the reference host.
- **SC-004**: The name check reports zero findings. No translation unit gains a finding of another check.
- **SC-005**: Every commit in the sequence builds and passes the test set and the format check, as the quickstart range walk shows.
- **SC-006**: The rename map covers every shipped-header name that changed, including a detail namespace. A search finds no old shipped-header spelling outside the map and the exception list, over the trees FR-015 names.
- **SC-007**: The collision check of FR-006 reports no macro collision.
- **SC-008**: Every gate of FR-005 passes.
- **SC-009**: The project version is 0.5.0 and the shared-object version is 2. The version list records the rename.
- **SC-010**: The constitution text states every naming rule and every exception. Each naming key matches the constitution rule it implements. No guidance file of FR-011 states a different convention. A file that drops the sentence and points at the constitution states no different convention. The constitution version, Sync Impact Report, version lineage row, and Last Amended date follow FR-010.
- **SC-011**: An uncommitted plant of one misnamed identifier in `include/speedgun-ng/dbc.hpp` fails the name-check step. The plant is removed before the closing commit is finished. The plan records the result.

## Assumptions

- The stakeholder is the maintainer, and the next stakeholder is the harness author. No consumer outside the repository is known. The rename map serves migration. Deprecated aliases stay out of scope.
- The five readings in Decisions come from the feature description's recommendations. Each rejected reading is recorded. A later clarify pass can replace a reading before planning. The requirements above use the chosen readings, so the specification stays testable.
- The rename mechanism, the order of the commit groups, and the scripts that prove FR-002 belong to the plan. This specification states the outcome each script must show.
- The reference host for FR-003 is the host the overhead page already uses. The plan records that host. The noise bound is five percent.
- `cmake/variables.cmake` defaults `BUILD_SHARED_LIBS` to off. The CI shared build at `.github/workflows/ci.yml:251` turns it on, so the shared-object version is live in that job. The bump still records the break.

## Decisions

Each item records the candidates and the chosen reading.

1. **Proposed rules N-5, N-6, N-8, N-10, and N-11.** One candidate accepts the five rules as proposed. The other replaces N-6 with an `m` prefix (`mColumnCount`) or a trailing underscore (`columnCount_`). Chosen: accept all five. The `m_` prefix already marks every private member. N-6 changes only the remainder.

2. **Named constants.** Candidates: `kPascalCase`, `UPPER_SNAKE_CASE`, or lowerCamelCase. Chosen: `kPascalCase` for constants, lowerCamelCase for tag objects. `UPPER_SNAKE_CASE` stays for macros and enumerators. That choice keeps the macro-collision surface of FR-006 on enumerators and macros. `UPPER_SNAKE_CASE` for constants would widen that surface.

3. **Version of the break.** Candidates: 0.5.0 under the rules at the audit point, or a move to 1.0.0. Chosen: 0.5.0, shared-object version 2, `SameMinorVersion` kept. A 1.0.0 release would claim a stable API before the harness exists.

4. **Stale documentation.** Candidates: fold both claims into this feature, or move both out of scope by name. Chosen: fold both in, including the checklist twin of the follow-up sentence. The rename edits `docs/pages/` anyway. The follow-up sentence sits in a closed spec. The 0.4.1 patch edited that directory and logged the change, so a logged edit there has a precedent.

5. **Constitution version.** Candidates: MINOR 2.14.0, or MAJOR 3.0.0. Governance reserves MAJOR for an incompatible principle removal or redefinition, and MINOR for a new principle or materially expanded guidance. The amendment adds rules and a gate, and it narrows one X.3 clause. Chosen: 2.14.0. The 2.13.0 gate addition and the 2.11.0 platform narrowing were MINOR. A reviewer who reads the X.3 change as a redefinition can reopen this reading before planning.
