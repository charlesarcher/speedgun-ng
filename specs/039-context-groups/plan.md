# Implementation Plan: Context Groups for the Threaded Run

**Branch**: `039-context-groups` | **Date**: 2026-10-30 |
**Spec**: [spec.md](./spec.md)

**Input**: Feature specification from
`/specs/039-context-groups/spec.md`

## Summary

The contexts of one instance gain a partition: groups that start and
pace behind rounds of their own. One pure function turns the run's
placement plan into the partition, one reader turns a YAML layout file
into the same value, the runner splits its two standing rendezvous
points by group, and the console and the report name each context's
group. The ungrouped run keeps every byte of the shape master prints.
Ten modules, eight waves, TDD throughout.

## Technical Context

**Language/Version**: C++23, `CMAKE_CXX_EXTENSIONS=OFF`
(Additional Constraints), the standing strict warning set
(`CMakePresets.json`).

**Primary Dependencies**: the standing set and nothing new. The
vendored `yaml-cpp` `0.9.0` of specs/006 gains its first reader; the
vendored hwloc stays behind the standing seams; `simdjson` keeps the
report parsing (R-3, FR-016).

**Storage**: the JSON report document and its schema
`docs/schema/report-v1.json`, minor `1.10` (R-4, FR-030).

**Testing**: CTest executables, one `main()` per file with `fail()` and
`check()` and fork-plus-`execv` scenario children. No gtest, no sleep,
no clock read (FR-037, specs/035 FR-018).

**Target Platform**: Linux on GCC and Clang, `ci-ubuntu` and
`ci-rocky`; the suite passes under `taskset -c 0,1` (FR-036, VIII).

**Performance Goals**: no arrival, wait or dispatch inside a measured
window (`runner.cpp:2062-2065`). A grouped attempt adds one publication
slot and one extra node poll, both outside every window, and the
ungrouped attempt adds nothing to its rounds.

**Constraints**: counters-only time, so no clock joins the barrier or
the tests (`test/time_source_gate.sh`); zero new runtime dependency;
hidden symbol visibility with the public surface in
`include/speedgun-ng/` only; yaml-cpp stays out of `include/`
(specs/006 FR-014, A8).

**Scale/Scope**: 40 requirements, 10 success criteria. Three new harness
translation units, one new harness header block, one new CLI option
row, one new public member, five new test executables, three existing
test files moved by the schema literal.

**Project Type**: a library and CLI feature with recorded evidence
runs.

**Execution mode**: TDD (Principle III). The covering test task of each
wave stands written and observed failing before the implementation task
that turns it green.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | `ci-ubuntu` clang-tidy and cppcheck, gate check `readability-identifier-naming`, pinned C++23 | PASS |
| II. DBC | new interfaces carry `\pre`, `\post`, `\invariant` and `SG_REQUIRE`, `SG_ENSURE`; `dbc-gate` | PASS |
| III. R-DCUT | this plan carries the logical and physical views and the test plan; TDD recorded | PASS |
| IV.1 Main page | the groups section, its region and its rows land with the behavior (M-H) | PASS |
| V. Style | `format-check`; N-1 to N-12 spellings, `ContextGroup`, `groupOfContext`, `m_` free public members | PASS |
| VI. Coverage | 100 percent line, branch and DBC over the added code, no exclusion marker | PASS |
| VII. Performance | rounds and the publication slot stand outside every window | PASS |
| VIII. Hard gates | build, ctest, sanitize, tsan, static analysis, naming, format, spell, prose, drift, time-source, coverage, two-cpu | PASS |
| IX. Spec-driven | spec, plan, tasks, implementation, all four present | PASS |
| X. Anti-slop | no grouping switch, one partition function, one reader, no group dimension in the row shape | PASS |
| XI. Prose | `prose-lint` over the lane range, 0 findings | PASS |

**Gate weakening requested**: no. **Gate strengthening requested**: no.
No gate fails, so **Complexity Tracking** carries justifications only.

## Project Structure

### Documentation (this feature)

```text
specs/039-context-groups/
├── spec.md                       # 40 FRs, 10 SCs, rulings D-1 to D-12
├── clarifications-round1.md      # five splits, reconciled
├── clarifications-round2.md      # second pass over the amended spec
├── plan.md                       # this file
├── data-model.md                 # the partition, the group, the layout grammar
├── contracts/                    # group-partition.md, group-layout.md
├── quickstart.md                 # the evidence index
├── checklists/requirements.md
├── analysis-roundN.md            # one per analyze pass
├── tasks.md
└── results/                      # one record per acceptance run + verdict
```

### Source Code (repository root)

```text
include/speedgun-ng/
└── benchmark.hpp                 # ContextDisclosure gains `group` (M-A)

source/harness/
├── detail/internal.hpp           # ContextGroup, groupOfContext, the layout
│                                 # reader and the fit entry, the option slot
├── group_partition.cpp           # the partition from a preset plan (M-B)
├── group_layout.cpp              # the YAML layout reader, yaml-cpp (M-C)
├── group_fit.cpp                 # the layout arm of the fit walk (M-D)
├── options.cpp                   # the `--group-layout` row (M-D)
├── detail/options.hpp            # kGroupLayoutOptionValue = 299 (M-D)
├── cli.cpp                       # the pairing refusals (M-D)
├── hunt_cli.cpp                  # the hunt pairing line (M-D)
├── placement_preset.cpp          # the fit walk gains the layout arm (M-D)
├── runner.cpp                    # per-group rounds, publication, teardown (M-E)
├── report.cpp                    # the console group segment (M-F)
├── json_report.cpp               # the `group` key, schema minor (M-G)
└── json_load.cpp                 # the optional key on load (M-G)

source/yaml/yaml_gate.cpp         # unchanged; the link proof stays (M-C)

docs/schema/report-v1.json        # the optional `group` key (M-G)
docs/pages/harness.md             # the option-reference row (M-D)
README.md                         # the groups section and its rows (M-H)
example/docs/context_groups.cpp   # the section's region (M-H)
tools/docs/example_drift.py       # SECTIONS, SECTION_OWNERS, page cap (M-H)

test/source/harness_group_partition_test.cpp   # M-B
test/source/harness_group_layout_test.cpp      # M-C
test/source/harness_group_cli_test.cpp         # M-D
test/source/harness_group_run_test.cpp         # M-E
test/source/harness_group_report_test.cpp      # M-F, M-G
test/CMakeLists.txt               # the five registrations (M-B to M-G)
```

**Structure Decision**: the partition lives in the harness beside the
placement code that computes the plan it reads, in two new translation
units that the harness `GLOB_RECURSE` already picks up
(`CMakeLists.txt:240-245`). The layout reader sits beside them rather
than under `source/yaml/`, because its caller is the CLI and the fit
walk, and it takes yaml-cpp the way `json_load.cpp` takes simdjson: a
PRIVATE `BUILD_INTERFACE` link on the harness target
(`CMakeLists.txt:264-267`). specs/006 FR-014 keeps yaml-cpp out of
`include/` and names `source/` as the includer scope; this plan records
the second `source/` includer, and the purity scan and the nm proof
keep their readings.

## Design

### Decision index

| Ruling / question | Decision |
| --- | --- |
| R-1 the barrier is the group's own | D-1: the two standing rounds split by group, none spans |
| R-2 two sources, one partition | D-2: preset or file, the partition-naming pairing refuses |
| R-3 the vendored reader | D-5, FR-016: `source/harness/group_layout.cpp` |
| R-4 the report carries the groups | D-10, FR-029 to FR-031: the per-context key |
| R-5 the synthetic seam | FR-036: the seam, plus `installTestLayout` for exact counts |
| Which unit each preset groups by | D-3: the unit the plan already names, `plan.group`, `plan.node` |
| How a group is named | D-4: `<unit>-<ordinal>`, or the file's `name`, or `g<index>` |
| Round 1/Q1 the cpu form | D-6: slots only, the key `cpus` refuses (FR-014) |
| Round 1/Q2 the fixture pair | D-7, FR-026: context 0 keeps the pair, publication crosses no round |
| Round 1/Q3 no L3 unit | FR-006: `kNoGroup` names no partition |
| Round 1/Q4 the hunt pairing | FR-019: the standing combination refusal grows |
| Round 1/Q5 the machine line | FR-028: the preset-in-effect rule holds |
| Round 2/Q1 the pairing read | FR-019: the resolved preset, read at the fit checkpoint |
| Round 2/Q2 the node number | D-4: the plan's own unit value |
| Round 3/Q1 the pairing key | FR-019, D-2: the partition the preset names, the line keeps one rule |
| P-1 how the publication travels | the per-attempt publication slot below, section "The attempt" |
| P-2 how the rounds are stored | one `std::vector<std::unique_ptr<sg::ContextBarrier>>` per point |
| P-3 where the yaml reader lives | harness TU, PRIVATE `BUILD_INTERFACE` link |
| P-4 where the partition is computed | once per instance, beside the context build |

### Modules and dependency order

The feature splits into ten modules. The order below stands the landing
order; the brace marks the parallel group.

```text
M-A  (version, header field)
 └─ M-B  M-C          (partition ∥ layout reader)
      └─ M-D          (option row, refusals, fit walk)
           └─ M-E     (the runner's rounds)
                └─ M-F  M-G   (console ∥ report)
                     └─ M-H  (docs)
                          └─ M-I  (evidence)
                               └─ M-J  (gates, convergence)
```

- **M-A the surface and the numbers**
  (`include/speedgun-ng/benchmark.hpp`, `CMakeLists.txt`,
  `source/harness/json_report.cpp`). Public surface: `ContextDisclosure`
  gains `std::string group`, camelBack under N-12, empty when the run
  carries no partition. Version `0.25.0` to `0.26.0`
  (`CMakeLists.txt:30`), harness `SOVERSION` `16` to `17`
  (`CMakeLists.txt:333`), archive `SOVERSION` stays `4`
  (`CMakeLists.txt:95`). Test plan: the disclosure rows of
  `harness_group_report_test` (wave 1 carries the header, wave 5 the
  bytes), the stamped-byte rows of `harness_json_report_test`.
  Contracts: group-partition.md A-1. Depends on nothing.
- **M-B the partition** (`source/harness/group_partition.cpp`). Public
  surface: none; harness-internal `sg::detail::ContextGroup`,
  `groupPartition`, `groupOfContext` (`internal.hpp`). Reads
  `PlacementPlan` and the preset, returns the groups in first-appearance
  order of their slots (D-3, D-4, FR-006 to FR-010). No thread, no
  allocation policy beyond the vector, no clock. Test plan:
  `harness_group_partition_test` (wave 2): the six presets over four
  layouts, the `kNoGroup` case, the one-group case, the one-context
  case, the name spellings, the plan-unchanged case. Depends on M-A.
- **M-C the layout reader** (`source/harness/group_layout.cpp`). Public
  surface: none; harness-internal `parseGroupLayout(path, contexts)`
  answering `std::expected<std::vector<ContextGroup>, std::string>`
  (FR-012 to FR-018, FR-021). yaml-cpp reads the document; every fault
  answers the one-line text the CLI prints. Test plan:
  `harness_group_layout_test` (wave 2): the accept set, the 16 fault
  shapes of FR-018, the fault text's path and entry naming. Depends on
  M-B for the record type.
- **M-D the option row, the pairings and the fit walk**
  (`source/harness/detail/options.hpp`, `source/harness/options.cpp`,
  `source/harness/cli.cpp`, `source/harness/hunt_cli.cpp`,
  `source/harness/group_fit.cpp`, `source/harness/placement_preset.cpp`,
  `docs/pages/harness.md`). Public surface: the CLI row
  `--group-layout` on the value `299`, no environment source, the
  `--help` line the table generates. The fit walk gains the layout arm
  and the partition read the pairing refusal needs (FR-011, FR-019 to
  FR-021). Test plan: `harness_group_cli_test`
  (wave 3): the row, the pairing refusals, the batch stop, the replay of
  `test/base_cli/run_matrix.py`; `harness_cli_table_test` moves to 39
  rows and re-checks the guide table. Depends on M-B and M-C.
- **M-E the runner's rounds** (`source/harness/runner.cpp`). Public
  surface: none. The partition is computed once per instance beside the
  context build (`:1626-1644`); a partitioned run builds one start
  barrier and one stop barrier per group in place of the two whole-run
  barriers (`:2066-2067`), and a run that carries no partition keeps
  that standing pair; the body arrives at its own group's rounds
  (`:2126`, `:2149`); the fixture pair stays on context 0 inside its
  own body, where `Fixture` and `callbackState` stand locals
  (`:2115-2118`): where the partition holds more than one group, the
  setup outcome travels on the publication slot, and after its own stop
  arrival context 0 waits on the settlement flags of the other bodies
  before the teardown half runs; a run with no partition, or one group
  holding every context, keeps the standing publication and teardown
  point (FR-022 to FR-027).
  Test plan: `harness_group_run_test` (wave 4): the four-group run, the
  two-node run, the one-group run, the stranded-ledger case, the fixture
  failure case, the ungrouped comparison. Depends on M-D.
- **M-F the console segment** (`source/harness/report.cpp`). Public
  surface: none. The group rides the context line as its last segment
  and the machine line keeps its rule (FR-028). Test plan:
  `harness_group_report_test` (wave 5): the segment bytes for pinned and
  unpinned contexts, the no-group byte identity, the `report_shape`
  samples. Depends on M-E.
- **M-G the report and the schema** (`source/harness/json_report.cpp`,
  `source/harness/json_load.cpp`, `docs/schema/report-v1.json`). Public
  surface: `ContextDisclosure::group` answers on load (FR-029 to
  FR-033). `kSchemaVersion` moves `1.8` to `1.10` with its history
  comment (`json_report.cpp:28-39`), the schema gains the optional key
  at the context item, and the reader answers an empty name for a
  document without it. Test plan: `harness_group_report_test` and the
  moved literals of `harness_json_report_test`,
  `harness_json_load_test` and `harness_noise_report_test`
  (FR-030, FR-031). Depends on M-F.
- **M-H the docs** (`README.md`, `example/docs/context_groups.cpp`,
  `tools/docs/example_drift.py`, `docs/pages/harness.md`). The main page
  gains the groups section, its region and its quick-reference rows;
  `SECTIONS` and `SECTION_OWNERS` gain the section; the page cap moves
  with the page and records the reason (FR-039, IV.1). Test plan:
  `example_drift`, `example_drift_fixtures`, `snippet_compile`,
  `example_run`, `report_shape`. Depends on M-F and M-G.
- **M-I the evidence** (`specs/039-context-groups/results/`,
  `quickstart.md`, one driving script). The five acceptance runs, one
  record each, the reports and views beside them (FR-038). Depends on
  M-H.
- **M-J the gate sweep** (no source of its own). The release build, the
  sanitizer and tsan suites, the two-cpu run, coverage, clang-tidy and
  cppcheck, the prose, spell, format, drift, dbc and time-source gates,
  and the vendor proofs (FR-036, FR-037, FR-040, VIII). Depends on M-I.

### Risks

- **A stranded round hangs the run.** The participant count comes from
  the same partition the dispatcher walks, and the standing ledger check
  (`runner.cpp:1955-1961`) fires before any wait the harness performs.
  The stranded-member scenario stands covered by a test (FR-027).
- **A synthetic shape truncates inside a small affinity mask.** Exact
  group counts stand asserted through `installTestLayout`
  (`internal.hpp:790`), the way `fourGroupShape` does today; the
  `hwloc_topology_set_synthetic` descriptions carry the read-path
  scenarios (FR-036, specs/035 FR-017).
- **The teardown moves for multi-group runs only.** Existing fixture
  scenarios watch the pair's order; the ungrouped path keeps its standing
  point, and `harness_fixture_test` stands as the guard.
- **yaml-cpp in the harness target.** The nm proof reads the archive,
  the purity scan reads `include/` and the build files, and the consumer
  test reads the installed package: all three run in M-J. The link stays
  `BUILD_INTERFACE` PRIVATE, and the comment at `CMakeLists.txt:753`
  restates the includer count.
- **The schema literal moves.** Three test files stamp `1.8` in four
  places (`harness_json_report_test.cpp:546`,
  `harness_json_load_test.cpp:707`, `harness_noise_report_test.cpp:423`
  and `:453`); the task moves each in the same commit as the constant
  (FR-030).
- **The page stands at its cap.** `README.md` holds 940 lines and
  `PAGE_LINE_CAP` is 940 (`example_drift.py:222`); M-H moves the cap in
  the same change and records the reason beside it.
- **Contention at landing.** Spec 037 and spec 031 stand in flight; the
  version, the schema minor, the option value and the `SOVERSION` get
  re-read at landing and moved one step where a landing took them
  (FR-032).

### Logical view: the grouped attempt

```text
resolvePreset(preset, layout, n) ──> PlacementPlan { cpu[], group[], node[] }
                                          │
                              groupPartition(preset, plan, n)
                                          ▼
      the core arm over four L3 groups that hold eight cores:
                        partition = [ Group{name:"l3-0", slots:[0,1]},
                                      Group{name:"l3-1", slots:[2,3]} ]
                                          │
                 attempt: one ContextBarrier per group, per point
                                          │
   ctx0 ──setup──┐            ┌── start(g0) ── body ── stop(g0) ──┐
   ctx1 ─────────┤            │                                   ├──┤
                 │  publish   ▼                                   │  │  poll all DONE
   ctx2 ─────────┤            ┌── start(g1) ── body ── stop(g1) ──┘  │
   ctx3 ─────────┘                                                     │
   ctx0: teardown after each settled flag  <───────────────────┘
```

The publication slot carries context 0's setup outcome to every body,
and each body raises a settlement flag after its stop arrival; the
group rounds carry only their own members, and context 0's teardown
half waits on those flags inside its own body.

### Physical view

- `speedgun-ng_harness` (`CMakeLists.txt:246`) gains
  `source/harness/group_partition.cpp`, `group_layout.cpp` and
  `group_fit.cpp` through the existing `GLOB_RECURSE`
  (`CMakeLists.txt:240-245`).
- The harness target gains
  `target_link_libraries(speedgun-ng_harness PRIVATE $<BUILD_INTERFACE:yaml-cpp::yaml-cpp>)`
  beside the simdjson line (`CMakeLists.txt:264-267`); the vendored
  SYSTEM-include promotion at `CMakeLists.txt:744-748` already keeps
  yaml headers out of the strict set.
- The public header `include/speedgun-ng/benchmark.hpp` grows one
  member, so the harness `SOVERSION` moves to `17`; the merged archive
  keeps `4` (`CMakeLists.txt:95`, hard rule 12).
- Five test executables register beside the placement tests
  (`test/CMakeLists.txt:989-1043` pattern): `add_executable`, link
  `speedgun-ng::harness`, include `${CMAKE_SOURCE_DIR}/source/harness`,
  `cxx_std_23`, `add_test`. `harness_group_run_test` carries
  `PROCESSORS` the way `harness_context_barrier_test` does
  (`test/CMakeLists.txt:1250`); every new entry stays in the `quick`
  tier unless its reading proves otherwise (`test_tier_labels`).
- The scenario processes that install a topology stand fork-plus-`execv`
  children, the standing shape of `harness_topology_test.cpp:96-113`.

### Public API surface added

- `sg::ContextDisclosure::group` (`std::string`): the group name of one
  context, empty where the run carries no partition. `BenchmarkResult`
  and `ReportData` carry the record, so a console reader, a library
  caller and `loadReport` all reach the same value.
- No new public function, no new public type, no change to any existing
  signature.

### Version and `SOVERSION`

- Version: `0.25.0` to `0.26.0`, the next free minor at the cut
  (`CMakeLists.txt:30`).
- Harness `SOVERSION`: `16` to `17`; a public struct's layout moves
  (FR-005).
- Archive `SOVERSION`: stays `4`; no exported archive layout moves.
- Report schema: `1.8` to `1.10`. Spec 037 cut first and its graph
  record takes `1.9`, so this lane lands one past it (R-4, FR-032).
- Option values: `kGroupLayoutOptionValue = 299`, one past the `298`
  the earlier lane's `--graph` row takes; the block comment names the
  split.
- The commit that moves the numbers records the re-read beside them
  (FR-032).

## Test Plan

**Execution mode**: TDD (constitution III). The covering test task of
each wave stands written and observed failing before the implementation
task that turns it green, and the pair lands in one commit.

**Fixture route**: exact machine shapes stand installed as
`MachineLayout` records through `installTestLayout`
(`source/harness/detail/internal.hpp:790`); the read path stands driven
through `installTestTopologyDescription` (`:820-823`) with descriptions
such as `L3:4 Core:2 PU:2` and `Node:2 L3:2 Core:2 PU:2`. No test reads
`/sys`, and no test reads the host topology (FR-036, R-5).

**Determinism**: no sleep, no clock, no wall-order assertion. A grouped
scenario proves its rounds by completing, which proves each round's
participant count, and by the disclosure bytes the run writes. Threaded
scenarios carry `PROCESSORS` and stay in the `quick` tier.

### Waves

- **Wave 1 (M-A)**: the field, the version, the `SOVERSION`. Verify:
  `ctest --test-dir build -R "harness_json_report_test|harness_group_report_test" --output-on-failure`
  green after `cmake --preset=ci-rocky && cmake --build build`.
- **Wave 2 (M-B, M-C)**: the partition and the reader, the two pure
  units. Verify:
  `ctest --test-dir build -R "harness_group_partition_test|harness_group_layout_test" --output-on-failure`
- **Wave 3 (M-D)**: the row, the pairings, the fit walk. Verify:
  `ctest --test-dir build -R "harness_group_cli_test|harness_cli_table_test|base_cli_matrix" --output-on-failure`
- **Wave 4 (M-E)**: the rounds, the publication, the teardown. Verify:
  `ctest --test-dir build -R "harness_group_run_test|harness_context_barrier_test|harness_fixture_test" --output-on-failure`
- **Wave 5 (M-F, M-G)**: the console segment, the report key, the
  schema. Verify:
  `ctest --test-dir build -R "harness_group_report_test|harness_json_schema_test|harness_json_load_test|harness_context_report_test" --output-on-failure`
- **Wave 6 (M-H)**: the main page, the region, the rows, the guide.
  Verify:
  `ctest --test-dir build -R "example_drift|example_run|snippet_compile|report_shape" --output-on-failure`
- **Wave 7 (M-I)**: the acceptance runs and their records. Verify: the
  script exits 0, and each record names its command, exit code and
  artifact.
- **Wave 8 (M-J)**: convergence. Verify: the release preset build,
  `ctest --test-dir build --output-on-failure`, the sanitizer and tsan
  presets, `taskset -c 0,1 ctest --preset prepush-quick -j 2`, the
  coverage target at 100 percent lines and branches, `ci-ubuntu` with
  clang-tidy and cppcheck at zero findings on the gate checks,
`prose-lint`, `spell-check`,
  `format-check`, the DBC gate, the time-source gate, the yaml purity
  scan and nm proof.

### FR coverage map

| FR | Wave | Test |
| --- | --- | --- |
| FR-001 | 2, 4 | `harness_group_partition_test`, `harness_group_run_test` |
| FR-002 | 2, 3 | `harness_group_partition_test`, `harness_group_layout_test` |
| FR-003 | 3 | `harness_group_cli_test` |
| FR-004 | 2, 4 | partition ungrouped rows, `harness_group_run_test` |
| FR-005 | 1 | `harness_group_report_test`, the version read |
| FR-006 | 2 | `harness_group_partition_test` |
| FR-007 | 2 | `harness_group_partition_test` |
| FR-008 | 2 | `harness_group_partition_test` |
| FR-009 | 2 | `harness_group_partition_test`, `harness_placement_preset_test` |
| FR-010 | 2, 4 | partition one-context row, `harness_group_run_test` |
| FR-011 | 3 | `harness_cli_table_test`, `harness_group_cli_test` |
| FR-012 | 2 | `harness_group_layout_test` |
| FR-013 | 2 | `harness_group_layout_test` |
| FR-014 | 2 | `harness_group_layout_test` |
| FR-015 | 2 | `harness_group_layout_test` |
| FR-016 | 2, 8 | `yaml_purity_scan`, `yaml_nm_proof`, the build |
| FR-017 | 2 | `harness_group_layout_test` |
| FR-018 | 2, 3 | `harness_group_layout_test`, `harness_group_cli_test` |
| FR-019 | 3 | `harness_group_cli_test` |
| FR-020 | 3 | `harness_group_cli_test` |
| FR-021 | 2, 3 | the fault-text rows of both tests |
| FR-022 | 4 | `harness_group_run_test` |
| FR-023 | 4 | `harness_group_run_test` |
| FR-024 | 4 | `harness_group_run_test`, `harness_context_barrier_test` |
| FR-025 | 4 | `harness_group_run_test` |
| FR-026 | 4 | `harness_group_run_test`, `harness_fixture_test` |
| FR-027 | 4 | `harness_group_run_test` |
| FR-028 | 5 | `harness_group_report_test`, `report_shape` |
| FR-029 | 5 | `harness_group_report_test` |
| FR-030 | 5 | `harness_json_schema_test`, `harness_json_report_test` |
| FR-031 | 5 | `harness_json_load_test` |
| FR-032 | 1, 5 | the version read, the stamped bytes |
| FR-033 | 5 | the byte-identity rows of `harness_group_report_test` |
| FR-034 | 4, 8 | `harness_jitter_preset_test`, the full suite |
| FR-035 | 5 | `harness_group_report_test`, `harness_context_report_test` |
| FR-036 | 2, 4, 8 | the two-cpu run, the seam rows |
| FR-037 | 8 | the coverage target, the suite run |
| FR-038 | 7 | the records and the quickstart links |
| FR-039 | 6 | `example_drift`, `example_run`, `snippet_compile` |
| FR-040 | 1, 8 | the docs target, `dbc-gate` |

## Constitution Check (re-check after design)

*Re-check after Phase 1 design.*

| Principle | Status |
| --- | --- |
| I. Standard-first | PASS: the new units compile in the release preset with the pinned baseline |
| II. DBC | PASS: `groupPartition`, `parseGroupLayout` and the fit entry carry contracts in `contracts/` and in the header, stated once |
| III. R-DCUT | PASS: views, test plan and TDD mode present |
| IV.1 | PASS: M-H carries section, region and rows |
| V. Style | PASS: names fixed by the module list |
| VI. Coverage | PASS: every new unit has its own test executable |
| VII. Performance | PASS: rounds outside windows; no new read on the measuring path |
| VIII. Gates | PASS: M-J runs them all |
| IX. Spec-driven | PASS: four artifacts, tasks next |
| X. Anti-slop | PASS: the additions below are the whole list |
| XI. Prose | PASS: the plan and the tasks carry the same gate as the code |

## Complexity Tracking

- `sg::detail::ContextGroup` and `groupPartition`: the partition is the
  item's own fact and needs no shape the plan does not use. No
  configurability, no comparator, no serialization of its own.
- `parseGroupLayout`: one file, one grammar, one answer type. No
  streaming, no schema object, no writer.
- The publication slot: one per-attempt object carrying one string, one
  ready flag and one settlement flag per context, because a partition
  leaves the standing round unable to publish context 0's setup or to
  order its teardown (D-7, FR-026). No mutex, no condition
  variable, no queue.
- The per-group barrier vectors: the existing `sg::ContextBarrier` does
  the work, one object per group per point. No new synchronization type
  joins the tree.
- The group dimension stops at the disclosure: the merged row, its
  divisor and the per-context columns keep their shape (FR-035).
- No other abstraction, configurability or extension point joins the
  lane: no grouping switch, no public group handle, no second layout
  source, no cpu form (Out of scope).
