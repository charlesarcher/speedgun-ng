# Phase 1 Contract: the Platform Policy Amendment

**Feature**: `010-linux-only` | **Spec**: [spec.md](../spec.md) | **Research**: [research.md](../research.md)

This feature exposes no interface to a user or another system. It edits one
governance document, one build-configuration file, and two prose documents.
This contract exists for the governance text, because the constitution
becomes binding the moment it is amended, and a later amendment that drops
one of these statements should be able to cite what was required here
(research.md R-008).

## C1: Required statements in the amendment

The amendment to `.specify/memory/constitution.md` carries all nine. Each is
a statement a reader can locate by search, and each closes a specific clause.

| # | Required statement | Closes | Requirement |
|---|-------------------|--------|-------------|
| S1 | The hard gate list names Linux and enumerates no other platform | VIII, first bullet, "Linux (GCC/Clang), macOS (AppleClang), Windows (MSVC)" | FR-001 |
| S2 | The Windows-suspension sentence is replaced by a statement that a future specification adds a platform, and that landing an upstream port is not what restores one | VIII, first bullet, "landing that port reinstates this gate" | FR-001 |
| S3 | The supported-platform definition names Linux alone | Additional Constraints, Language, "The CI matrix (Linux GCC/Clang, macOS AppleClang, Windows MSVC) defines supported platforms" | FR-002 |
| S4 | The preserved warning-set clause names the Linux flag family and drops the MSVC flag reference | Additional Constraints, Warnings and hardening, "`/W4 /permissive-` on MSVC" | FR-002 |
| S5 | The per-feature release-build clause names `ci-ubuntu` and no other preset | IX, "`cmake --preset=ci-macos` on macOS; `cmake --preset=ci-windows` on Windows" | FR-003 |
| S6 | The Open deferrals block carries no macOS-runner entry | Open deferrals, the entry beginning "Principle VIII macOS enforcement" | FR-004 |
| S7 | The version lineage table gains a 2.11.0 row with its rationale, the file version reads 2.11.0, and `Last Amended` reads 2026-10-02 | Governance, Amendments | FR-005 |
| S8 | A Sync Impact Report at the top of the file states that 2.11.0 supersedes the effect of the 2.7.0 Windows suspension and the 2.8.0 macOS deferral, and that both prior reports stay in the file | the file's own supersession pattern, set by the 2.9.1 report | FR-006 |
| S9 | The report names `specs/003-vendor-hwloc`, `specs/004-vendor-simdjson`, `specs/005-vendor-hdrhistogram`, `specs/006-vendor-yaml-cpp`, and `specs/009-vendor-quill` as superseded on the platform question, and states the on-ramp by naming every deleted preset | none | FR-007, FR-008 |

## C2: What the amendment must not do

| # | Prohibition | Requirement |
|---|------------|-------------|
| P1 | No prior report is deleted. The 2.7.0 and 2.8.0 reports stay in the file and their rows stay in the lineage table. | FR-006 |
| P2 | No principle is deleted, renamed, or renumbered. Only text inside an existing clause changes. | FR-005, FR-019 |
| P3 | No gate, threshold, warning class, analyzer invocation, job, or dependency is added, removed, or weakened. The gate set's content is identical; only its platform enumeration narrows. | FR-019 |
| P4 | No statement claims a future port is scheduled, funded, or planned. The amendment says a specification adds a platform and nothing more. | Fixed decision 1 |
| P5 | No amendment text asserts that a per-platform branch was removed, simplified, or consolidated. The branches stay. | FR-016 |

## C3: Audit buckets

FR-015's audit returns every hit with a path, a line, and one of these four
buckets. The bucket is the verdict; the count is not (research.md R-004).

| Bucket | Definition | Fate | Measured at the base commit |
|--------|------------|------|------------------------------|
| **Live claim** | Text that tells a reader a platform is supported, buildable, or usable. A diagnostic, a comment listing supported targets, a configuration surface, or prose. | Changes. | 8 lines in 2 files |
| **Preserved seam** | A branch, preprocessor conditional, or rationale comment whose function is to select or explain platform behaviour. | Stays byte-identical. | 45 lines across 8 files |
| **Historical record** | Anything under `specs/`, and any comment recording why something was suspended or excluded. | Stays. | 116 lines under `specs/` alone |
| **Third-party tooling** | Spec Kit scaffolding under `.specify/scripts/`, and anything under `external/`, `build/`, or `.opencode/node_modules/`. | Out of scope. | 5 lines in `.specify/scripts/bash/` |

The seam count breaks down as: `CMakeLists.txt` 15, `cmake/variables.cmake`
3, `cmake/VendoredArchiveMerge.cmake` 1, `cmake/ImportAutotoolsSubmodule.cmake`
6, `source/counters/clock_provider.cpp` 5, `include/speedgun-ng/speedgun-ng.hpp`
1, `source/counters/linux_pmu/table_parse.cpp` 1, and five vendored gate units
at one line each. The false positives the token set also returns, the
measurement-window uses of the word "windows" in `counters_measurement.hpp`,
`test/CMakeLists.txt`, `counters_clock_push_test.cpp`, and
`counters_recorder_test.cpp`, are third-party or historical by content and
are listed so a reader can see each was judged, with none missed.

## C4: Entities

No data model accompanies this feature, because no entity has a field, a
validation rule, or a state transition (research.md R-008). The five
entities the spec names are recorded here so a reader looks in one place.

| Entity | Shape | Governed by |
|--------|-------|-------------|
| **Supported platform set** | One member: Linux, on GCC and Clang, on the distribution families the CI jobs cover. | S1, S3 |
| **Per-platform seam** | A branch in `CMakeLists.txt` or `cmake/` selecting behaviour by operating system or compiler. Untouched. | FR-016, P5 |
| **Port on-ramp** | Re-add the deleted preset by name, configure, build, add a runner. Every deleted preset is listed in the amendment. | S9 |
| **Gate-shaped preset** | Seven, all deleted. A preset whose name reads as a CI claim. | FR-009 |
| **Superseded spec** | Five merged vendor specs, named by the amendment, none edited. | S9 |

## C5: Local preset hygiene

`CMakeUserPresets.json` is gitignored and carries no tracked diff, but its
presets must resolve (FR-018a, research.md R-002). On a machine where it
carries a preset inheriting a deleted one, that preset is removed. The
measurement is the configure step's exit status, and the failure it prevents
is `Invalid configure preset`, which CMake reports for the whole preset
directory. The one preset at fault is never named on its own.