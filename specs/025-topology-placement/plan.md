# Implementation Plan: Topology Placement

**Branch**: `022-topology-placement` | **Date**: 2026-10-19 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/025-topology-placement/spec.md`

## Summary

The harness reads the machine layout once per process through the
vendored hwloc, turns a preset name plus a context count into one cpu per
execution context, feeds that cpu into the placement slot feature 020
already ships, and discloses the layout and each context's `cpu@group` in
the report. Five presets ship: `none`, `core`, `l3`, `smt`, `compact`. A
`--topology` listing flag prints the layout without running a benchmark.
No public signature moves, and both `SOVERSION` values stay.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language at 2.3.0 of its lineage)

**Primary Dependencies**: the vendored hwloc 2.14.0 at
`external/hwloc`, embedded mode, `sg_` symbol prefix, libxml2 off
(feature 003); the feature 020 placement seam
(`PlacementSlot`, `PlacementFn`, `setPlacementFunction`,
`detail::placementRecordOf`); `getopt_long` inside `source/harness/cli.cpp`

**Storage**: N/A: no file, no database. The layout lives in process
memory for the life of the process.

**Testing**: the repository's own harness (`speedgun-ng_test` pattern:
one executable per area plus `add_test`), the fake counters provider, an
injected `MachineLayout`, and the installed `PlacementFn` replacement of
feature 020. `ctest -j 2`.

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). thrawn
(4 L3 groups, 24 cores, 48 threads, one node) is the reference host.

**Project Type**: library feature: a C++ archive surface plus a command
line of the harness entry point. No new target.

**Performance Goals**: the layout read runs once per process and adds no
work to a measured window; the placement decision is a table walk at
thread-start setup. No benchmark regression is claimed, so no number is
promised (Principle VII: a claim carries a number, a platform, and a
distribution).

**Constraints**: 100 percent line and branch coverage on new paths; no
sleep and no clock in tests; every time and counter value from
`sg::counters`; one line and exit status 2 for a bad option value; the
README section stays within `SECTION_LINE_CAP` 25 and `PAGE_LINE_CAP`
rises by the measured amount; hwloc stays private to `source/`.

**Scale/Scope**: eight source files touched or added in
`source/harness/`, one public header extended by one handle option, four
test executables, one docs example, three docs pages, three build files.

**Execution mode**: TDD (Principle III). Every test task is written and
observed failing before the code task that turns it green exists. The two
US4 comparison scenarios ride in the red-first pair of the stories that
ship the behavior they compare: the three-shape comparison rides T023,
which appends to the file T008 creates and runs before the resolver of
T011, and the disclosure comparison rides T024, which appends to the file
T015 creates and runs before the printer of T016.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 standard surface; the only non-standard call stays inside the hwloc gate pattern already ratified by feature 003, and `pthread_setaffinity_np` stays where 020 put it | PASS |
| II. Design by contract | The preset resolver is a pure function with an `SG_REQUIRE` on a context count of 1 or above and an `SG_ENSURE` that a plan holds distinct cpus; the hwloc read carries one contract macro, on the `hwloc_topology_init` answer no scenario can drive (Q-8) | PASS |
| III. R-DCUT | Requirements FR-001 to FR-036 map to the design decisions R-01 to R-13 below and to `tasks.md`. Execution mode: TDD, recorded above and in `tasks.md` | PASS |
| IV. Documentation | README section, `docs/pages/harness.md` section, quick-reference rows, one runnable `example/docs/` program | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the feature head | PASS |
| VI. Coverage | 100 percent line and branch on new paths, no exclusion marker; the real-hwloc path is covered by one host test that runs on the CI machine | PASS |
| VII. Performance | No work moves into a measured window: the read happens before the first context starts, and the disclosure reads no clock (FR-024) | PASS |
| VIII. CI gates | Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, coverage, docs drift, prose, report shape, DBC, time-source, name-check | PASS |
| IX. Spec-driven | spec, plan, research, data-model, contracts, quickstart, tasks all present before code | PASS |
| X. Anti-slop | No speculative abstraction: the preset resolver is one switch over five names, and no interface is introduced for a single implementation | PASS |
| XI. Prose | The gate walks `git ls-files`, so at this point it clears the tracked tree only, while these artifacts stand untracked and await the run that lands with the branch's first commit, as `checklists/requirements.md` records | PASS for the tracked tree, pending for these artifacts |

**Gate weakening requested**: no. **Gate strengthening requested**: no. The
one gate threshold that moves is `PAGE_LINE_CAP` in
`tools/docs/example_drift.py`, the drift gate's own page budget, raised by
the measured size of the new section and its rows under the rule FR-028
states; no constitution text changes and no gate loses a check.

No gate fails, so **Complexity Tracking stays empty**: no new target, no
new public type, no new dependency.

## Project Structure

### Documentation (this feature)

```text
specs/025-topology-placement/
├── spec.md              # FR-001 to FR-036, SC-01 to SC-12, Clarifications Q-1 to Q-8
├── plan.md              # this file
├── research.md          # R-01 to R-13 with rationale and rejected alternatives
├── data-model.md        # MachineLayout, Preset, PlacementPlan, option carriers
├── quickstart.md        # runnable validation on thrawn and in CI
├── contracts/
│   ├── topology-placement.md   # preset table, fit rule, exit statuses
│   ├── report-topology.md      # topology line and the cpu@group field
│   ├── cli-placement.md        # --placement and --topology, option table rows
│   └── page-and-examples.md    # README section, docs example, caps
├── checklists/
│   └── requirements.md
└── tasks.md             # Phase 2 output of /speckit.tasks
```

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp        # BenchmarkHandle::placement(std::string_view)
source/harness/
├── topology.cpp                         # NEW: the one harness TU that includes <hwloc.h>;
│                                        #      builds MachineLayout; the layout install seam
├── placement_preset.cpp                 # NEW: preset name parsing and the pure resolver
├── detail/internal.hpp                  # MachineLayout, Preset, PlacementPlan, RunOptions
│                                        #      fields, internal declarations
├── cli.cpp                              # --placement=<name>, --topology, the batch fit check
├── runner.cpp                           # ContextPolicy from the plan; per-family preset field
├── execution_context.cpp                # default placement function gains the group text
├── registry.cpp                         # BenchmarkHandle::placement beside threads
└── report.cpp                           # topology line in the context block; field width

test/
├── source/harness_topology_test.cpp         # NEW: real read, injected layouts, mask rule
├── source/harness_placement_preset_test.cpp # NEW: five presets, fit limits, determinism
├── source/harness_placement_report_test.cpp # NEW: topology line, cpu@group, no-preset bytes
├── source/harness_cli_placement_test.cpp    # NEW: --placement, --topology, exit 2 cases
└── CMakeLists.txt                           # four add_executable/add_test pairs

example/
├── docs/topology_placement.cpp              # NEW: the README snippet's program
└── CMakeLists.txt                           # add_docs_example(topology_placement)

README.md                                    # "Topology placement" section + rows
docs/pages/harness.md                        # detail section
tools/docs/example_drift.py                  # section entry plus PAGE_LINE_CAP raise
tools/hwloc/hwloc_nm_proof.sh                # harness archive link proof (R-01)
```

**Structure decision**: everything new stands inside the two archives
that already exist. The layout read joins the harness archive, where the
layout is private state (FR-003), and the preset resolver is a pure
function over `MachineLayout` with no hwloc type in its signature, so the
tests never touch hwloc.

**Files outside the feature directory this change edits**, each with the
reason the spec's Parallel work rule asks for: `CMakeLists.txt` (the
harness target's private hwloc include and link, and the `VERSION` line),
`include/speedgun-ng/benchmark.hpp` (the one handle option this feature adds,
a member function on the existing `BenchmarkHandle`, which moves no exported
layout),
`specs/015-benchmark-harness-core/contracts/cli.md` (that option table is
the CLI contract feature 015 owns, and a new option joins it in the same
commit as the parser), `tools/docs/report_shape.py` (only when a new line
class reaches the one sample block it compares),
`tools/docs/example_drift.py` (the section registration and the measured
page cap),
`tools/hwloc/hwloc_nm_proof.sh` (the archive link proof), `README.md` and
`docs/pages/harness.md` (Principle IV), `test/CMakeLists.txt` (the four
new test executables and their `add_test` entries, each carrying the
`source/harness` include directory the way `harness_gap_test` does),
`example/CMakeLists.txt` (the new docs example and its runner target),
`specs/020-execution-contexts-core/contracts/report-disclosure.md` (its
placement value set gains `cpu@group` in the same commit as the printer, per
the spec Budget note), and `docs/pages/building.md` (the `SOVERSION` rule and
the release paragraph T036 records). No source file of the other two lanes' features is edited.

## Design

### Decision index

| Id | Decision | Requirement |
| --- | --- | --- |
| R-01 | The harness archive owns the hwloc include: `source/harness/topology.cpp` is the one harness unit that includes `<hwloc.h>`, and `speedgun-ng_harness` takes the vendored include and the imported target `PRIVATE`, which supplies the staged headers and the build-order edge while the objects arrive from the members already merged into the library archive | FR-004, FR-031, Q-1 |
| R-02 | `MachineLayout` is a plain value: ordered vectors of groups, cores and threads with ordinal ids, plus the mask it was built from | FR-001, FR-003 |
| R-03 | The preset resolver is pure: `resolvePreset(Preset, const MachineLayout&, int contexts)` returns a `PlacementPlan` whose `cpu` stands empty when the context count exceeds `places`; no hwloc type crosses it | FR-007 to FR-014 |
| R-04 | Two installs in `source/harness/detail/internal.hpp`: `installTestLayout(std::optional<MachineLayout>)` wins over the read, and `installTestTopologyDescription(std::optional<std::string>)` hands the read a synthetic description whose rejection drives the unreadable branch; the read stays cached in one function-local static, so an install must precede the first read | FR-002, FR-006, Q-5 |
| R-05 | Placement still travels through the existing slot: the runner builds `ContextPolicy {SPIN, PlacementSlot::cpu(plan.cpu[i])}` when a plan stands, and `noPin()` when no plan stands | FR-008, FR-010, D-1 |
| R-06 | The default placement function composes the placement field as `cpu@group` by looking the group up in the layout; `NO_PIN` keeps its bytes, the noise warning of feature 020 keeps its condition, and the field width of `report.cpp` already fits the pair | FR-021, FR-022, FR-023, Q-2 |
| R-07 | Names: handle `placement(std::string_view)`, command line `--placement=<name>`, listing `--topology`; a name the system lacks and a value that names cpus each stop with one line and status 2; an unreadable layout makes the `--topology` listing print one line and exit with status 2 | FR-015 to FR-017, FR-036, Q-3 |
| R-08 | The fit check runs once in `speedgunMain` after selection and before the first run, over every selected instance that carries a preset other than `none`: the unreadable-layout stop runs for each of them whatever the context count (FR-005, Q-6), and the place-count stop runs only where the context count stands above 1 (Q-7) | FR-005, FR-018, FR-022, Q-4 |
| R-09 | The topology line prints in the context block beside the version line, once per report, when any selected instance runs with a preset other than `none` over a run of more than one context, and reads no clock | FR-020, FR-022, FR-024 |
| R-10 | The docs example carries two two-context families: one with the handle preset `compact`, the widest of the five, and one with no preset so the command line can show a shape, so it runs on any host; the page section, its rows and its caps move with it | FR-025 to FR-029 |
| R-11 | Four test executables, listed under Source, each linked against the harness archive | FR-034, SC-04 |
| R-12 | The `nm` proof extends to the harness archive: it must show the prefixed hwloc references resolving inside the build, and the purity scan keeps its rule | FR-031, R-01 |
| R-13 | One host test reads the real layout on the CI machine and asserts the package and thread counts stand above zero, the core and group counts stand at zero or above, and every thread sits inside the mask; every other test injects | FR-005, SC-04 |

### Risks

- **Cross-archive symbols (R-01)**. `MERGE_INTO` is a static-archive
  slurp and is ignored for shared configurations
  (`cmake/ImportAutotoolsSubmodule.cmake:466-472`), and CI builds both
  shapes: static by default (`cmake/variables.cmake:9`) and shared at
  `.github/workflows/ci.yml:262`. The harness archive therefore holds
  undefined `sg_hwloc_*` references and no vendored member, and the
  vendored archive reaches the final link through the harness's `PRIVATE`
  link of the imported target, which CMake records as a link-only
  interface for a static library. T005 measures both configurations, and
  T031 asserts the harness subject as undefined-references-only: the
  library archive's own A, B and C assertions stay as feature 003 shipped
  them.
- **Disclosure field width**. `kPlacementFieldWidth` is
  `digits10 + 1` of `std::uint32_t`, so 10 characters. `cpu@group` fits
  for every realistic cpu index; a cpu index of 9 digits, with the group
  digits beside it, exceeds the field and the standing cut mark appears. T015 pins
  the pair for the largest cpu index the injected layouts use.
- **Report-shape stability**. The context block gains a line only under a
  preset, and no `report_shape` binary runs with one (`first_benchmark` at
  `test/CMakeLists.txt:955`, `docs_reading_the_report` at `:970`), so the
  script and all three recorded sample blocks keep their bytes; the script is
  edited only if a new line class reaches the one sample block it compares,
  and the README "Reading the report" region holds a C++ snippet that gains
  no report line (T018).
- **Parallel lanes**. N1 touches the runner and the report too. The merge
  order N1 before N4 is recorded in the spec; a rebase keeps both lines in
  every shared list.
- **A mask holding one cpu**. `compact` places as many contexts as the
  mask holds threads, so on a mask holding a single cpu the docs example's
  preset family stops with the fit line. CI machines hold more than one
  cpu, and the stop follows the Edge Cases rule "a preset that fits the
  machine but exceeds the mask exits with status 2". FR-029 asks the
  example to pick a preset the host can fill, and a single-cpu mask is
  the one shape a fixed preset cannot answer: recorded here as the
  residual of R-10.

### Logical view: the layout read

```text
speedgunMain
  └─ layout()                     # once per process, one function-local static
       ├─ installed test layout?  ── yes ─▶ use it            (FR-006)
       ├─ installed topology description?  ── yes ─▶ set it   (FR-006, R-04)
       └─ hwloc: init, load, walk PU/Core/L3/PACKAGE inside
          sched_getaffinity ∩ topology_get_cpuset
                                  ──▶ MachineLayout           (FR-001, FR-005)
```

hwloc's own sets follow the machine and the control group, measured at
the audit point under `taskset -c 0,1,8,9` against the system hwloc 2.15.0,
so the process affinity mask
supplies the restriction and the walk skips every processing unit outside
it (FR-005).

A read that fails leaves no layout: a run with a preset exits 2, and a
run without one behaves as feature 020 ships (FR-005).

### Logical view: one threaded run

```text
instance (thread count N, preset P)
  ├─ batch fit check (before any run starts)      → exit 2 + one line   (FR-018)
  ├─ resolvePreset(P, layout, N) → PlacementPlan {cpu[i], group[i]}     (FR-009)
  ├─ contexts: ContextPolicy{SPIN, PlacementSlot::cpu(cpu[i])}          (FR-008)
  ├─ default PlacementFn pins and returns "cpu@group" + applied         (FR-021)
  └─ disclosure prints ctxN=SPIN/24@0/applied                           (FR-021)
```

### Logical view: the report

```text
speedgun-ng 0.12.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48     # only with a preset
bmSum                    iterations=   1066261  real time/iter (ns)=  657.575 ...
bmFill/8/threads:4 ...
  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/6@1/applied ...
```

A report with no preset prints the block exactly as feature 020 ships it
(FR-022).

### Physical view

- `topology.cpp` is the only harness TU that includes `<hwloc.h>`; the
  library archive's `source/hwloc/hwloc_gate.cpp` keeps its version
  tripwire and its link proof unchanged.
- `MachineLayout`, `Preset` and `PlacementPlan` are declared in
  `source/harness/detail/internal.hpp`, the internal header no consumer
  installs, so FR-003 and FR-031 hold.
- The preset resolver and the layout walk are separate translation units:
  the resolver never sees an hwloc type, which is what lets the preset
  tests run without hwloc at all.

### Public API surface added

| Name | Kind | Notes |
| --- | --- | --- |
| `BenchmarkHandle::placement(std::string_view presetName)` | handle option | one preset name; a name the system lacks is a run-time stop with status 2, the way a bad option value stops today |

No new type, no new header, no new target, no new free function. No
existing signature moves and no exported layout changes (FR-030).

### Version and `SOVERSION`

- `VERSION` takes the next free minor at merge: `0.12.0`, the percentiles
  lane having taken `0.11.0` at `8eac15d` (FR-032).
- Harness `SOVERSION` stays 7 and the archive `SOVERSION` stays 2. The
  rule `docs/pages/building.md` records is a move of an exported layout,
  and a non-virtual member function on an existing exported class moves
  no layout: no data member, no virtual table, no exported struct change
  (FR-033, SC-10). The page narrates two moves, 3 to 4 at 0.8.0 and 4 to
  5 at 0.9.0, and each moved exported data with it: the row model, the
  aggregate statistics and the complexity fit at 0.9.0. The move to 6 that
  the page has yet to narrate came from data members of `State` and
  `BenchmarkResult`, and the move to 7 came from the chunk fields, the chunk
  cadence, the row percentile columns and the percentile selector of the
  percentiles lane.
  The comment beside `SOVERSION 7` in `CMakeLists.txt` at `:215-224` names the
  move to 6, for the `State` thread index and context count, the
  `BenchmarkHandle` thread surface, the `BenchmarkResult` contexts disclosure
  and the execution context shipping in this archive, and the move to 7, for
  the chunk fields, the chunk cadence, the row percentile columns and the
  percentile selector. It carries no member-function distinction. T036 adds
  the 0.10.0 and 0.11.0 paragraphs and that distinction, and amends the 0.9.0
  paragraph so its items separate the exported layout moves from the member
  functions that moved with them, so the rule and its applications agree.

## Test Plan

| Executable | Covers | Method |
| --- | --- | --- |
| `test/source/harness_topology_test.cpp` | FR-001 to FR-006, SC-04 one group, four groups, siblings off and a smaller mask | one host read asserting the package and thread counts above zero, the core and group counts at zero or above, and every thread inside the mask; injected layouts of 1 group, 4 groups and siblings off, and a mask narrowed in-process by `sched_setaffinity`; an installed synthetic description (no L3, no cores, one cpu) and a rejected description that leaves `readable` false |
| `test/source/harness_placement_preset_test.cpp` | FR-007 to FR-019, SC-01 to SC-03, SC-04 siblings on | `resolvePreset` over the injected layouts, a two-sibling layout among them; the five presets; the fit limit of each; determinism across two calls |
| `test/source/harness_placement_report_test.cpp` | FR-020 to FR-024, SC-05, SC-07 | the installed `PlacementFn` replacement plus a captured report: the topology line, the `cpu@group` field, the byte-identical no-preset block, the unchanged noise warning, and the pair `4294967295@0`, which exceeds `kPlacementFieldWidth` and prints the cut mark |
| `test/source/harness_cli_placement_test.cpp` | FR-015 to FR-018, FR-036, SC-06, SC-12 | the option table driven directly: `--placement` with each bad value, `--topology` output and exit 0, the listing of a rejected description in a fresh process, the handle-over-CLI precedence, the batch fit check over a `threadRange` family, the `none` skip path and the run of one context |
| `report_shape` checks | FR-022 | the three registered comparisons keep their bytes: every binary runs with no preset, so T018 confirms the script needs no edit |
| `example_run` | FR-025, FR-029 | `docs_topology_placement` builds, runs, and its marked region matches the page (SC-09) |
| thrawn real-host check of `quickstart.md` | SC-01, SC-02, SC-11 | the recorded runs of `quickstart.md`, kept out of CI because the fit limits a user meets are the machine's |

Gate set at the feature head: format, clang-tidy and cppcheck through
`ci-ubuntu`, the `ci-sanitize` and `ci-tsan` sanitizer builds, the
`ci-coverage` build with the 100 percent rule, docs drift, prose, report
shape, DBC gate, the `time_source_gate` entry of `test/CMakeLists.txt`,
`ctest -j 2`, and the two hwloc proofs `tools/hwloc/hwloc_nm_proof.sh` and
`tools/hwloc/hwloc_purity_scan.sh` (FR-031, FR-034, FR-035, SC-08).

Coverage note: the coverage gate is aggregate and unwaivable
(`cmake/coverage.cmake:77-79` extracts `source/`, and
`tools/dbc/coverage_gate.sh:35` requires 100.0 percent branches), so every
branch `topology.cpp` and every new branch of `cli.cpp` must stay reachable
from a test. Three routes reach them, and one answer stays outside the
measured set. The one host read in `harness_topology_test` covers the real
machine. An installed topology description string makes the read call
`hwloc_topology_set_synthetic()`: a description the library rejects
returns -1 and folds into the same readable decision, which drives both
the shape branches (a synthetic machine with no L3 and no cores, a
one-cpu machine) and the failure branch with no broken host. The
synthetic component is built into the vendored configuration: upstream
`config/hwloc.m4:205` lists it in the unconditional component set, so
`--disable-libxml2` keeps it. The walk stays branch-light for the same
reason: one loop over the processing units with a parent, core and L3
lookup per unit, and no per-object-type switch tree (Principle VI,
FR-034). The one answer no scenario can drive, the `hwloc_topology_init`
answer, rides a one-line contract macro whose line the extract omits at
`cmake/coverage.cmake:76`, the way `source/harness/registry.cpp:526`
carries its undrivable precondition; no guard stands ahead of
`hwloc_topology_destroy`, because that macro aborts before the walk is
entered (FR-034, SC-08, Q-8).

## Constitution Check

*(Post-design pass: does the design as written honor I to XI?)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. The standard C++23 surface carries the feature; the one non-standard library, hwloc, stays inside `source/harness/topology.cpp` behind the vendored pin. |
| II | PASS. `SG_REQUIRE` on `contexts >= 1` and `SG_ENSURE` on pairwise distinct cpus stand at the resolver, and the init answer rides one contract macro, the single branch no scenario can drive (Q-8). |
| III | PASS. FR-001 to FR-036 map to R-01 to R-13, every task cites a requirement id, a success criterion or the constitution clause it serves, and each test task names the red run it observes. |
| IV | PASS. The README section, the `docs/pages/harness.md` section, the quick-reference rows and the runnable example ship in the same lane, held by the drift gate. |
| V | PASS. clang-format, clang-tidy and cppcheck stay clean at the feature head, and the new files join the standing configuration with no suppression. |
| VI | PASS. 100 percent line and branch coverage on new paths with no exclusion marker; the host read, the injected seam and the rejected description drive the branches. |
| VII | PASS. The layout read happens before any context starts and never inside a measured window; the report prints one line per report and reads no clock. |
| VIII | PASS. Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, coverage, docs drift, prose, report shape, DBC, time-source, name-check. No gate loses a check. |
| IX | PASS. spec, plan, research, data-model, contracts, quickstart and tasks all stand in `specs/025-topology-placement/` before implementation begins. |
| X | PASS. One switch over five preset names, one layout record, no speculative interface, and the two install seams reuse the standing test pattern. |
| XI | PASS for the tracked tree, pending for these artifacts. The gate walks `git ls-files`, and these artifacts stand untracked at this point, so no run of it covers them yet; T037 runs the gate over the branch at the feature head. |

The design
adds no target, no public type, and no dependency; the only new include
of an hwloc header sits in `source/`, which feature 003 FR-021 allows;
the coverage rule is met by the host read plus the injected seam; the
docs budget move is recorded in the spec's Budget note and lands in
`tools/docs/example_drift.py` in the same commit as the README section.

## Complexity Tracking

No violation to justify.
