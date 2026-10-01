# Implementation Plan: Vendor quill as a Private, Pinned Submodule

**Branch**: `009-vendor-quill` | **Date**: 2026-09-30 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/009-vendor-quill/spec.md`

**Artifacts**: [research.md](research.md) · [data-model.md](data-model.md) · [quickstart.md](quickstart.md) · [contracts/build-integration.md](contracts/build-integration.md) · [contracts/privacy-contract.md](contracts/privacy-contract.md)

## Summary

Bring quill v13.0.0 into the tree the way hwloc, simdjson, HdrHistogram_c,
zlib, and yaml-cpp already are: a pinned git submodule at `external/quill`,
consumed by a scope-isolated `add_subdirectory` bracket in the root
`CMakeLists.txt`, linked privately through a build-only edge, and invisible on
every surface a downstream consumer can observe.

quill breaks that pattern in one way that shapes the whole plan. It is
header-only: its build defines an interface target, compiles nothing, and
defines every symbol in the same translation unit that declares it. Three
consequences follow, each measured against the pinned tree and not
assumed:

1. There is no vendored archive, so no archive merge, no archive sanitizer
   exclusion, and nothing for an archive symbol scan to inspect (R-007).
2. The dependency cannot be proven by an undefined reference, because zero
   undefined quill symbols exist at any optimization level. Forcing one costs
   half a megabyte of never-called code at `-O3 -DNDEBUG` against 3,744 bytes for
   the version constants alone, a factor of about 150. The proof
   is a runnable check outside the shipped archive instead (R-008, per the
   Clarifications of 2026-09-30).
3. The bracket technique the four compiled imports use to keep vendored
   warnings out of the strict set clears flag variables inside their own
   bracket, and quill compiles nothing there, so it does nothing. The include
   path is promoted to a system include instead, which is the mechanism the
   four brackets already use as their second step (R-006).

## Technical Context

**Language/Version**: C++23 (`CMAKE_CXX_EXTENSIONS=OFF`); quill requires C++17
or newer and inherits 23 from the presets

**Primary Dependencies**: quill `v13.0.0`, pinned at commit
`eb802a37c7d585840324886a3d8648c9c2159952`, MIT, no upstream submodules,
interface-only target, bundled formatter 12.0.2 under its own namespace

**Storage**: none; no runtime data, no schema, no persistent state

**Testing**: CTest. Two new entries: a purity scan in the shape of the five
existing ones, and a runnable dependency check under a distinct name
(R-012). No new C++ unit-test target under `test/source/`, matching the
four prior imports.

**Target Platform**: Linux enforced (GCC and Clang, `ubuntu-26.04` and Rocky
Linux 10); macOS builds developer-locally; Windows stays unprecluded

**Project Type**: C++ library with a CMake build

**Performance Goals**: the shipped archive grows by under 64 KB when quill is
added (SC-009a). A benchmarking library must not carry never-called logging
code in its measured artifact.

**Constraints**: zero external runtime dependencies after install; zero quill
references on any consumer-observable surface; no quill option reaches this
project's configuration; the configured build type is unchanged by the
ingestion; the submodule worktree stays pristine

**Scale/Scope**: one submodule, one wrapper unit, two test entries, one
dependency-classification branch, one README section, five to eight CI audit
steps

## Constitution Check

*GATE: evaluated before Phase 0 research and re-evaluated after Phase 1
design.*

### Pre-research evaluation

| Principle | Gate | Verdict |
|-----------|------|---------|
| I | Standard-first | PASS with justification. The dependency's one hard requirement is the platform threads package, already present and already installed by every CI job. No new runtime dependency ships. |
| II | Design By Contract | PASS. No public API, so no new contract annotations. The wrapper unit and both tests carry ordinary comments in the house style. |
| III | R-DCUT | PASS. Research recorded as R-001 through R-015 with decisions, rationale, and rejected alternatives. |
| IV | Documentation | PASS. One README re-pinning section (FR-007), plus this feature's spec, plan, research, data model, contracts, and quickstart. |
| V | Style and formatting | PASS. The wrapper unit and the two scripts follow the four existing per-dependency exemplars. `format-check` and `spell-check` already exclude `external/`. |
| VI | 100% line, branch, and DBC coverage | PASS after a change to the capture command. The wrapper unit has zero executable lines, matching the four existing wrapper units. quill's inline code lands in an instrumented project unit, so it reaches the trace, and R-013's verification found the capture aborting on it; `cmake/coverage.cmake` gained `--ignore-errors mismatch`. Measured after the change: 100.0% lines, 100.0% branches, zero vendored paths in the trace. |
| VII | Performance discipline | PASS. SC-009a bounds the archive growth. quill's runtime hazards, its spinning backend thread and its hardware timestamp counter calibration, are kept out of the shipped library entirely by Fixed decision 8. |
| VIII | Hard CI gates | PASS with three verification items. Every existing Linux job stays green; the release preset gates the strict-warning claim; the example's link-manifest audit is verified and keeps its current allowlist (R-011). |
| IX | Spec-driven development | PASS. spec, plan, tasks, code, and tests, in that order. |
| X | Anti-slop discipline | PASS. No speculative abstraction, no new build-system module, one bracket, no premature generalization. The one place a prior pattern was copied without working, the archive proof, was replaced. |
| XI | Prose standards | PASS. This feature's documents carry no em dashes, no contrastive framing, no filler, and no meta-editorializing, verified by hand because the mechanical gate does not cover `specs/`. |

**Gate weakening requested**: none. The example's link-manifest audit keeps
its current allowlist and is verified and keeps its allowlist (R-011). The
Windows MSVC gate stays suspended under amendment 2.7.0 for the vendored
autotools ingestion; quill adds no autotools and does not reinstate or
extend that suspension.

### Post-design re-evaluation

| Principle | Change after design | Verdict |
|-----------|--------------------|---------|
| VI | R-013 established that no coverage-tooling change is needed, subject to verification | PASS, verification item retained |
| VIII | A4 and A5 read the installed shared object. That is a deliberate narrowing of where the audit looks. Justified in the non-exposure contract: auditing the archive would require whitelisting quill's unhideable singletons and would then miss a singleton escaping into the dynamic symbol table | PASS, deviation documented |
| X | Two requirements exist only because a copied pattern did not survive contact with a header-only dependency: FR-010b records why no flag-clearing block appears, and FR-013a pins the whole option set, which avoids a denylist | PASS |

No violation requires the Complexity Tracking table. Every deviation from the
prior imports is forced by a measured property of quill and is recorded in
[contracts/privacy-contract.md](contracts/privacy-contract.md) section 4 with
its reason.

## Project Structure

### Documentation (this feature)

```text
specs/009-vendor-quill/
├── spec.md                          # /speckit.specify output, amended by /speckit.clarify
├── plan.md                          # This file (/speckit.plan output)
├── research.md                      # Phase 0 output
├── data-model.md                    # Phase 1 output
├── quickstart.md                    # Phase 1 output
├── contracts/
│   ├── build-integration.md         # Phase 1 output
│   └── privacy-contract.md          # Phase 1 output
├── checklists/
│   └── requirements.md              # /speckit.specify output, re-validated at /speckit.clarify
└── tasks.md                         # Phase 2 output (/speckit.tasks; created later)
```

### Source Code (repository root)

```text
external/quill/                                  # new submodule, pinned (FR-001)
.gitmodules                                      # one added stanza
CMakeLists.txt                                   # import_quill() bracket + 2 lines (target_sources, target_link_libraries)
source/quill/quill_gate.cpp                      # new: the one quill header, the version assertions (FR-002, FR-014)
tools/quill/quill_purity_scan.sh                 # new: discovery-call and public-header greps (A7, A8)
tools/quill/quill_dependency_check.cpp           # new: the runnable proof (A6, FR-008a)
tools/dbc/dependency_scan.sh                     # one added vendored-private branch (R-015)
test/CMakeLists.txt                              # 2 added add_test entries (R-012)
README.md                                        # one added "## Re-pinning quill" section (FR-007)
.github/workflows/ci.yml                         # audits in test, shared-audit, downstream-consumer (R-014)
```

Not touched, deliberately: `include/speedgun-ng/` gains nothing (FR-014),
`CMakePresets.json` gains nothing, `cmake/` gains no module (R-001),
`test/consumer/` gains nothing (the absence of the name is the entry),
`test/compile-fail/` gains nothing, `docs/` gains nothing, and `example/`
gains nothing.

One file above that list carries a conditional in place of a flat no:
`cmake/coverage.cmake` gains one flag. R-013 originally expected no change
there and verification overturned that: the extract allowlist does drop every
vendored path, but the capture itself aborts on an lcov exception-tag mismatch
inside a vendored header, which takes the coverage target down. The capture
command gains `--ignore-errors mismatch`, scoped to that one lcov error class.
Task T021 owns it.

**Structure Decision**: the existing layout, extended in place. Every one of
the six prior vendor imports added its dependency to the same seven places,
and quill needs the same seven with two differences: the wrapper unit carries
no symbol reference, and the second proof is a runnable check in place of an
archive scan. No new directory outside `source/quill/` and `tools/quill/`.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| FR-018 permits two exported quill symbols in a shared build, a relaxation of the zero-export rule | The pinned tree marks both thread helpers `QUILL_ATTRIBUTE_USED`, which forces emission whether or not anything calls them, and `QUILL_EXPORT`, which resolves to default visibility on GCC and Clang. Both are reachable from any translation unit that includes `quill/Backend.h`, and that header is the sole publisher of the version constants FR-002 asserts on. Measured: a shared build of this project exports exactly those two, both weak and stateless. | Editing the vendored tree to drop the attributes: FR-001 pins upstream's bytes, and editing them voids the pin. A project-wide linker version script: it changes export policy on every platform, including the Windows gate amendment 2.7.0 already suspends, and interacts with the generated export header. |
| `cmake/coverage.cmake` capture gains `--ignore-errors mismatch` | A vendored header's inline code is emitted into this project's own instrumented translation units, so the capture sees it. Every vendored tree already lands in the raw trace, and quill is the first whose headers trip lcov's exception-tag consistency check, which aborted the capture and took the whole coverage target down. The flag is scoped to that single lcov error class; gcov's diagnostics stay live and the extract step still removes every vendored path before a gate number is read. | Excluding the path at capture time: lcov's `--exclude` cannot combine with `--capture` in the installed version. A removal pass after capture: the capture never completes. Excluding the gate unit from instrumentation: it has no executable lines to instrument, and the contamination arrives through this project's own unit. |

The remaining deviations from the prior imports are forced by a measured
property of quill and are documented in
[contracts/privacy-contract.md](contracts/privacy-contract.md) section 4:
the runnable dependency check in place of an archive symbol scan, the
symbol audits reading the installed shared object, the bracket clearing
nothing, and the consumer premise built from the pinned tree. None of
those weakens a gate.
