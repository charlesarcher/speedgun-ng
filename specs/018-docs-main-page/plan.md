# Implementation Plan: Main Page Documentation Rewrite

**Branch**: `018-docs-main-page` | **Date**: 2026-10-13 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/018-docs-main-page/spec.md`

**Audit point**: `8655201` (name existence, option rows, gate surfaces,
and section maps are checked against this commit).

## Summary

Rewrite the root `README.md` from a maintainer board into a teaching
page: a first screen that states what the library is and where its
numbers come from, a quick start that builds, installs, links, and runs a
first benchmark, twelve short feature sections in the shape of purpose,
snippet, and result, a two-table quick reference, a planned-work list,
and a one-sentence tail. The nine maintainer sections move whole to
`docs/pages/building.md`, and the section map below is the audit
worksheet for that move. Every page block lives in a marked region of a
file under `example/`: `cpp` snippets in example sources that CI
compiles, command, package, and sample regions in text-truth files a python drift gate
registered through ctest holds each page block equal to its region in
every tree configuration; a shape check runs the quick start binary and
compares its report to the sample block with the suite's own row-shape
mask. The documentation index regroups into user pages before internals.
The change amends the constitution to 2.19.0 with sub-rule IV.1 and the
example compile gate as a Principle VIII hard-gate entry, and adds the
matching checklist line to the spec and tasks templates. No library file
changes (FR-036); versions and SOVERSIONs stay put (FR-037).

## Technical Context

**Language/Version**: Markdown for all prose; Python 3 standard library
only for the two gate tools (the `tools/prose/prose_gate.py` precedent);
C++23 for the two new example programs; CMake 3.20 or newer (the
`cmake_minimum_required` floor of the root `CMakeLists.txt`).

**Primary Dependencies**: none new. Doxygen with the m.css theme arrives
through the existing `docs/Doxyfile.in` and `docs/conf.py.in` plumbing;
the gates are self-contained scripts.
**Storage**: N/A. The gates read repository source text and one program's
stdout. Nothing is persisted.

**Testing**: ctest script tests in the established pure-script idiom
(`dbc_dependency_scan`, `hwloc_purity_scan`, `pmu_events_check` style):
`example_drift`, `example_drift_fixtures`, `report_shape`,
`report_shape_fixtures`, `snippet_compile`. Fixture harnesses in the
`test/prose-gate-fixture/` pattern. Review devices for the two criteria
CI does not judge (SC-001; SC-011 caps land as acceptance readings, D-14).

**Target Platform**: Linux (Ubuntu for the main CI jobs, Rocky Linux for
the container job), the same families the existing matrix covers.

**Project Type**: documentation rewrite plus gate tooling on top of an
existing C++23 library; zero library source edits.

**Performance Goals**: no runtime target for the library (it does not
change). Gate budgets: `example_drift` reads text only and must stay
inside about 2 seconds; `report_shape` runs one benchmark at default
settings (about 1 to 2 seconds) and carries a 120-second ctest timeout.

**Constraints**: FR-036 (library files and example sources take comment
markers only), FR-037 (project version 0.8.0 and SOVERSIONs unchanged),
FR-039 (example code takes every time and counter value from the
counters library through public headers), SC-011 caps (400 total page
lines, 25 per section, D-14), Principle XI prose law on every artifact
and page, Principle VIII amendment rule for the new gate.

**Scale/Scope**: one rewritten 400-line page; one new docs page
inheriting about 230 lines of moved content; four new example files
(about 300 lines total: two C++ sources, one shell region file, one
CMake region file); two python tools (about 380 lines total); one
fixture harness (about 240 lines); five ctest registrations; one CI
step; one constitution amendment; two template lines.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Verdicts against constitution 2.18.0, principles I to XI, at plan time
(before Phase 0 research). The post-design re-check records the final
verdicts at the end of this plan.

| Principle | Verdict | Basis |
| --- | --- | --- |
| I Standard-first | PASS | No library code. The new example programs are standard C++23 over public headers, the same posture every `example/` file holds. |
| II Design by Contract | PASS | No public interface is added or changed. The two python tools are internal gate scripts; their contracts live in `contracts/` and their fixtures, the `prose_gate.py` posture. |
| III R-DCUT | PASS | The plan carries the logical view, the physical view, one interaction diagram, and a test plan (sections below). TDD mode: the fixture harness lands red before the gate exists, the `prose-gate-fixture` precedent. |
| IV Documentation | PASS | This feature writes documentation as its product and adds IV.1 for later features (FR-027). No public docstrings change because no public API changes. |
| V Style | PASS | The example files pass `clang-format` 18.1.8 like every C++ file; region markers are comments and carry no identifiers (V.1 untouched). Prose follows the XI law. |
| VI Coverage | PASS | The gcov gates measure application code; gate scripts are covered by their fixture suites through ctest, the `prose_gate` precedent (see `contracts/example-drift.md` R-9). |
| VII Performance discipline | PASS | Nothing hot changes. The plan states gate run budgets and the reason widths stay out of the shape check (measured in `test/source/harness_cli_test.cpp`). |
| VIII Hard CI gates | PASS with amendment obligation | The change adds a gate, so VIII's own rule binds: the example compile gate enters the hard-gate list by amendment in the same change that lands it (FR-030, research D-08). No commit carries the gate outside the law. |
| IX Spec-driven workflow | PASS | This plan is the Phase 0/1 artifact for spec 018. The test plan includes the release-preset build (`ci-ubuntu`) once per feature. |
| X Anti-slop discipline | PASS | Two tools with one job each, zero new frameworks, zero speculative knobs. Rejected larger designs are recorded in `research.md`. |
| XI Prose standards | PASS | Every artifact, page, and moved paragraph sits under the prose gate; code fences and inline code are auto-exempt in `tools/prose/prose_gate.py`; zero findings is FR-040's bar. |

No violation reaches the Complexity Tracking table.

## Project Structure

### Documentation (this feature)

```text
specs/018-docs-main-page/
├── plan.md              # this file
├── research.md          # Phase 0: D-01..D-14 decisions with alternatives
├── data-model.md        # Phase 1: E-01..E-12 entities and validation rules
├── quickstart.md        # Phase 1: ten runnable validation scenarios
├── contracts/
│   ├── main-page-structure.md   # page shape, scans, budgets, table row sets
│   ├── example-drift.md         # marker grammar, comparison, failures, fixtures
│   └── report-shape.md          # sample block, fold rule, capture, registration
├── spec.md              # input specification (unchanged by this command)
└── tasks.md             # Phase 2 output (/speckit.tasks; not created here)
```

### Source Code (repository root)

```text
README.md                          # rewritten teaching page (E-01 rules)
docs/
├── pages/building.md              # NEW: receives all nine dropped sections
├── pages/harness.md               # H1 retitled to drop spec identifiers (D-07)
└── conf.py.in                     # LINKS_NAVBAR1: user group, then internals
example/
├── first_benchmark.cpp            # NEW: the quick start program (D-05)
├── docs_snippets.cpp              # NEW: regions for the taught gaps (D-06)
├── quickstart.sh                  # NEW: quick-start command regions (D-05)
├── quickstart/CMakeLists.txt      # NEW: find_package and link-line region;
│                                  # the clean-room consumer builds this file
├── first_benchmark.expected       # NEW: sample output region `report` (S-1)
├── benchmark_example.cpp          # region markers added (comment-only, FR-036)
└── CMakeLists.txt                 # two add_example calls + harness link lines
tools/docs/
├── example_drift.py               # NEW: block/region match, --sections,
│                                  # --names/--options/--forbidden, --emit-blocks
└── report_shape.py                # NEW: sample block vs live report shape
test/
├── CMakeLists.txt                 # add_test, unconditional: example_drift,
│                                  # example_drift_fixtures, snippet_compile,
│                                  # report_shape_fixtures; report_shape guarded
│                                  # by if(TARGET first_benchmark)
├── snippet-compile/run.sh         # NEW: emit blocks, syntax-compile each (R-10)
└── example-drift-fixture/
    ├── run_example_drift_fixtures.py    # NEW: scratch-tree cases, planted probes
    └── run_report_shape_fixtures.py     # NEW: mask golden pairs (S-5)
.github/workflows/ci.yml           # lint job: one direct python3 example_drift.py step
.specify/memory/constitution.md    # 2.19.0 amendment: IV.1, VIII entry, lineage,
                                   # footer, Sync Impact Report
.specify/templates/spec-template.md    # main-page checklist line (FR-029)
.specify/templates/tasks-template.md   # main-page checklist line (FR-029)
```

**Structure Decision**: the repository layout is unchanged; the feature
lands inside existing directories. The one new directory is `tools/docs/`
alongside the established `tools/prose/` and `tools/pmu_events/` tooling.
Gate scripts live under `tools/`, fixtures under `test/`, tests
registered in `test/CMakeLists.txt`, each following the
`dbc_dependency_scan` pure-script idiom. Nothing moves out of `example/`,
and `add_example` keeps sole ownership of example targets, so the
standalone-example property that `downstream-consumer` proves stays
intact.

## Section map (FR-023)

The nine dropped sections of `README.md` at the audit point and their
destinations. Every command, tool name, tripwire description, and
version-pin mechanic in a row moves whole to the named destination
(FR-022, SC-006). The prose-gate ban on CI job names (FR-038) binds the
main page alone, so job names survive here.

| # | Old section | Destination in `docs/pages/building.md` | Facts that must appear |
| --- | --- | --- | --- |
| 1 | `## Contracts` | `## Contract checks` | `SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT`, `SG_ASSERT`, `SG_*_ALWAYS`; `include/speedgun-ng/dbc.hpp`; the `speedgun-ng_CONTRACTS` option with `ignore`, `observe`, `enforce`, `quick_enforce` semantics; `cmake --build build/dev -t dbc-gate`; the `dbc-gate` and `consumer-release` job roles; links to `dbc-migration.md` and `dbc-overhead.md` |
| 2 | `## Re-pinning hwloc` | `## Dependency re-pinning` then `### hwloc` | submodule path `external/hwloc`; checkout/commit/assertion steps; `source/hwloc/hwloc_gate.cpp`; the lockstep sentence; autoconf, automake, libtool, patch with both the apt and dnf command lines |
| 3 | `## Re-pinning simdjson` | `### simdjson` | `external/simdjson`; `source/simdjson/simdjson_gate.cpp`; native-CMake note |
| 4 | `## Re-pinning HdrHistogram_c` | `### hdrhistogram_c` | `external/hdrhistogram_c`; `source/hdrhistogram/hdrhistogram_gate.cpp`; the zlib redirect sentence (root `CMakeLists.txt`, no system package consulted) |
| 5 | `## Re-pinning zlib` | `### zlib` | `external/zlib`; both assertions in `source/zlib/zlib_gate.cpp`; the `ZLIB_VERNUM` hex rule with the 1.3.2 to `0x1320` example; the `Z_PREFIX` rename bracket; the gate-proves-symbol sentence |
| 6 | `## Re-pinning yaml-cpp` | `### yaml-cpp` | `external/yaml-cpp`; the expected-version constant in the `import_yaml_cpp` bracket; the configure-time tripwire reason (0.9.0 exposes no version macro; the `project()` line read; no git metadata) |
| 7 | `## Re-pinning quill` | `### quill` | `external/quill`; `quill::VersionMajor`/`VersionMinor`/`VersionPatch` compiled constants in `quill/Backend.h`; the header-only pair of facts (no archive merge; the about-150-times inline-graph measurement); `quill_dependency_check`; the system-include marking; the option-pinning ingestion bracket |
| 8 | `## Re-pinning pmu-events` | `### pmu-events` | `external/pmu-events`; the `RECORD` manifest fields (ref, fetch URL, date, per-file sha256); the configure-time tripwire; both `tools/pmu_events/update_pmu_events.py` invocations (`--check`, `--to`); kernel.org cgit with the GitHub mirror fallback; the source-ref policy DCR with the strip example; `ctest -R pmu_events_check` |
| 9 | `## Quality gates` | `## Quality gates` | the clang-format 18.1.8 pin and both `FORMAT_COMMAND` commands; `format-check` and `format-fix`; `cmake -P cmake/prose-lint.cmake` with its direct and target equivalents and the exit-code table; `tools/prose/prose_rules.yaml` as the Principle XI projection; `ctest -R prose_gate_fixtures`; the coverage gate mechanics (coverage.cmake in the developer-mode block, lcov/genhtml hard requirements, the `coverage-linux` preset, the `GD.pm`/perl-gd footnote) |

Two non-map dispositions: the title block's origin paragraph shrinks to
the one-sentence tail on the main page (spec decision Q-5, FR-038 keeps
the tail free of identifiers), and `## Build` is replaced on the main
page by the quick start, which carries the platform claim, the install
commands, the `find_package` block, and the link line in reader-facing
form (FR-003 to FR-007).

## Design

### Logical view

Three pairing relations carry the whole design, and one amendment binds
them to the law.

```text
 README.md                      example/                    CI
 +---------------------------+  +------------------------+  +----------------------+
 | feature section           |  | region                 |  | test, test-rocky     |
 |   H3 + purpose + block  --+--> // sg-example:begin    |  |   ctest: example_    |
 |                           |  | ...compiled code...    |  |     drift + fixtures,|
 | quick-reference tables    |  | // sg-example:end      |  |     report_shape +   |
 |   name rows  -------------+--> public headers         |  |     fixtures,        |
 | option table              |  +------------------------+  |     snippet_compile  |
 |   rows (nine long opts) --+-- source/harness/cli.cpp   |  | lint: direct         |
 | sample output block ------+-- first_benchmark stdout --+-- shape fold   |       |
 +-------------+-------------+                                        example_drift +
               | links once near the end                              scans (R-7)
               v                                                      forbidden
 docs/pages/building.md  <---- nine dropped sections (section map)
               | indexed by
               v
 docs/conf.py.in LINKS_NAVBAR1:  user pages (about, building, harness)
                                 then internals (barrier rule, overhead
                                 records, migration, rename map)

 .specify/memory/constitution.md 2.19.0: IV.1 law + VIII gate entry
   binds:  every feature carries its page block and region in one change
```

The invariant the design defends: a page block and its region are one
fact with two spellings. The drift gate (E-11) tests equality; the shape
check (E-05) tests the printed report against the sample block; the name
scan (E-06) and the option table rule (E-07) test the prose surface
against the source surface; IV.1 makes later features hold the same
invariant.

### Physical view and interaction

```text
sequenceDiagram
    participant P as Page edit (README.md)
    participant E as example region
    participant D as tools/docs/example_drift.py
    participant C as ctest (test, test-rocky)
    participant L as lint job
    P->>D: block sg-example=file#region
    E->>D: region text
    C->>D: python3 example_drift.py REPO_ROOT
    L->>D: python3 example_drift.py .
    D-->>C: exit 0 clean / exit 1 with file, region, line pairs
    D-->>L: same verdict without any configure step
```

`report_shape` (contract S-4) is registered only when target
`first_benchmark` exists, so a `BUILD_EXAMPLES=OFF` tree keeps the
mandatory drift gate and lacks the run checks (FR-035 split);
`snippet_compile` and both fixture runners register unconditionally
(the report-shape fixtures drive a fake binary script, no target).
The ctest registrations of the drift gate and the lint-job step land
only after the rewritten page exists. The fixture-runner scripts
themselves land early and unregistered: their red state lives in a
script no test yet runs, so no intermediate commit is red (see the
execution order in the test plan). CI wiring follows research
D-09: no new job, one `lint` step, two existing `ctest` steps, the
existing `docs` job as FR-026's build home.

### Public surface

The feature changes no public API (FR-036). Header files, library
sources, the project version 0.8.0, and every SOVERSION stay untouched
(FR-037). The example tree gains two files and marker comments only; the
`add_example` idiom keeps the tree standalone.

## Test Plan

Every success criterion maps to one mechanism and one surface. Review
devices appear where the spec's clarify session keeps the judgment off
CI.

| Criterion | Mechanism | Surface |
| --- | --- | --- |
| SC-001 first screen teaches | fresh-model pass per change plus three new-reader passes once (quickstart scenario 2) | PR review only |
| SC-002 page alone builds | clean-clone walkthrough of the quick start incl. install + out-of-tree `find_package` consumer of the page's blocks (scenario 1) | PR QA run; `downstream-consumer` keeps proving the package path |
| SC-003 snippets compile | each snippet is a complete top-level declaration region in a compiled example file; `--sections` proves every FR-009 section exists with a marked block; `ctest -R snippet_compile` syntax-compiles every emitted page block standalone with a planted probe (scenario 4) | `test`, `test-rocky` build + ctest; `lint` direct step |
| SC-004 drift gate | `ctest -R "example_drift"`: default pass, fixtures with every planted case (scenario 3) | `test`, `test-rocky` ctest; `lint` direct step |
| SC-005 forbidden tokens | scan inside the default `example_drift` run (contract C-B) | same surfaces |
| SC-006 facts survived | section-map walkthrough greps (scenario 6) | PR review |
| SC-007 index order | `cmake -P cmake/docs-ci.cmake` then index inspection (scenario 7) | `docs` job on master; local per feature |
| SC-008 amendment landed | greps over constitution and templates (scenario 8) | PR review |
| SC-009 quick reference honest | `--names` and `--options` scans inside `example_drift` (R-7) | same as SC-004 |
| SC-010 prose clean | `cmake -P cmake/prose-lint.cmake` over the range (scenario 5) | `prose-lint` job |
| SC-011 budgets | binary caps: 400 total lines, 25 per section (scenario 10) | PR review, at acceptance |
| SC-012 sample honest | `ctest -R report_shape` on the built binary (scenario 4) | `test` job ctest step |

Order of work follows Principle III's TDD note: the two fixture runners
and their planted cases land first and report the missing gates (red);
the gate tools turn them green in pairs (`example_drift` with its
runner, `report_shape` with its mask runner); the amendment lands with
the gate law text; the page rewrite follows, so every block lands
already paired with its region; gate registration (ctest entries, the
lint-job step) lands after the page, so every intermediate commit is
green and bisectable; the PR squash-merges to one master commit, which
makes the amendment and the gate atomic on master as Principle VIII
requires (research D-08). The index regroup and the `harness.md`
retitle close the set. Release-preset parity
(`cmake --preset=ci-ubuntu` plus full `ctest`) runs once before the pull
request (Principle IX, quickstart scenario 9).

## Constitution Check (post-design re-check)

Re-check after Phase 1 design (research.md, data-model.md, contracts/,
quickstart.md).

| Principle | Verdict | Basis after design |
| --- | --- | --- |
| I Standard-first | PASS | Design adds only standard-library python and public-header C++23 example code. |
| II Design by Contract | PASS | Contracts are written for the two tools (`contracts/example-drift.md`, `contracts/report-shape.md`) with fixture assertions; no public interface exists to annotate. |
| III R-DCUT | PASS | Logical view, physical view, and the interaction diagram above; the test plan names every artifact's covering check; fixtures-before-gate recorded. |
| IV Documentation | PASS | IV.1 text, its landing site, and the template lines are designed (research D-08); the page itself follows E-01. |
| V Style | PASS | Marker grammar keeps identifiers out of comments; example files are ordinary clang-format targets; table and heading shapes fixed in contracts. |
| VI Coverage | PASS | Fixture matrix R-8 plus mask golden pairs S-5; liveness rule (planted probes) carried from `test/time_source_gate.sh`. |
| VII Performance discipline | PASS | Gate budgets stated in Technical Context; the shape check's width tolerance carries its measured reason. |
| VIII Hard CI gates | PASS | The VIII entry text, its atomic landing rule, and version 2.19.0 are fixed in research D-08; no gate runs without its entry. |
| IX Spec-driven workflow | PASS | Artifact set complete; Phase 2 (`tasks.md`) is the next command's product; release-preset parity inside the test plan. |
| X Anti-slop discipline | PASS | Each component traces to a numbered FR; fourteen recorded decisions carry the rejected larger alternatives. |
| XI Prose standards | PASS | All prose surfaces enumerated; scans are mechanical; the artifacts themselves are gate-scope and written to the law. |

Verdict: every gate passes before and after design. Complexity Tracking
stays empty.

## Complexity Tracking

No constitution violation requires justification.
