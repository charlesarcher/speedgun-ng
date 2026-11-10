# Implementation Plan: Raw Event-Code Support (G-31)

**Branch**: `051-raw-events` | **Date**: 2026-11-10 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/051-raw-events/spec.md`

## Summary

Let a counter address end in a perf raw spelling, `cpu/r4000`, and the run
counts that config word on the named PMU device. Raw recognition joins the
one counters core lookup, `resolveLeafCore`
(`source/counters/system.cpp:573-590`, declared
`include/speedgun-ng/counters_measurement.hpp:607-608`), so the typed
library route (`include/speedgun-ng/counters_system.hpp:143-178`) and the
metric-formula route (`source/counters/linux_pmu/metric_formula.cpp:1290`)
follow it for free. A probe through the standing `pmuProbe`
(`source/counters/linux_pmu/provider.cpp:667-730`) settles availability,
the one errno table (`provider.cpp:722-729`) maps refusals, and the scope
rule (`provider.cpp:537-557`) relabels a scoped device. Synthesized raw
entries live in one address-keyed raw-entry table beside the frozen tree,
the shape the probed-kinds records already take
(`source/counters/system.cpp:165-177`, `source/counters/detail/pmu.hpp:278-292`).
The tree gains no leaf, so `--catalog`, the near-miss lists, and the
published listings answer as at base. The harness leaf-match stage
(`source/harness/runner.cpp:986-1030`) adds the width refusal and keeps
every standing line byte-for-byte. Disclosure rides the existing carriers:
the address as typed is already the column label
(`runner.cpp:1437-1443`), the `metricLabels` entry
(`runner.cpp:1586-1597`), the JSON `metrics[].label`
(`source/harness/json_report.cpp:532-538`), and the option-disclosure value
(`source/harness/cli.cpp:106-128`). The schema keeps 1.16
(`json_report.cpp:58`). No public header moves, so the counters SOVERSION
stays 8 (D-6).

## Technical Context

**Language/Version**: C++23 (`CMAKE_CXX_EXTENSIONS=OFF`, constitution
Additional Constraints).

**Primary Dependencies**: none new. The counters library, the harness, the
existing kernel seam (`source/counters/linux_pmu/kernel_calls.hpp:59-109`),
and the scripted-kernel test fixtures.

**Storage**: no new persisted state. The raw-entry table is process memory
beside the frozen tree, the lifecycle `system.cpp:170-172` already states
for the probed-kinds table.

**Testing**: scripted-kernel and scripted-sysfs units for every
host-independent behavior (the `counters_pmu_provider_test.cpp:179-268` and
`harness_limit_disclosure_test.cpp:206-310` fixture patterns), plus one
host acceptance pair: `cpu/instructions` beside `cpu/rc0` (SC-001).

**Target Platform**: Linux. This host reports vendor `AuthenticAMD`, cpu
family 26, model 68; `/sys/bus/event_source/devices/cpu/type` reads `4`,
and `/sys/bus/event_source/devices/cpu/events/instructions` reads
`event=0xc0`.

**Project Type**: library plus CLI, one repository (library-first rule).

**Performance Goals**: resolve-time work only, bounded per raw address and
inside the untimed checkpoint (`source/harness/cli.cpp:960-970`). The timed
loop and the report path run byte-identical code (research R-15).

**Constraints**: no public-header move (D-6 keeps counters SOVERSION 8);
no new JSON key and schema stays 1.16 (D-4); `--hunt` and
`State::setCounter` untouched (D-7, specs/029 FR-005 contract
`specs/029-hunter-core/contracts/hunt-cli.md:59,83`); no CI workflow edits;
README edit fits in place under `PAGE_LINE_CAP = 1004`
(`tools/docs/example_drift.py:256`, the page measures 1003 lines).

**Scale/Scope**: five counters files, two harness files, one option-row
text, three docs carriers, two new test units, one version block.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- I: C++23, standard facilities only (`std::from_chars` for the hex word,
  the pattern `source/counters/linux_pmu/encode.cpp` already uses). No P0
  designation: no new critical-path code. No P2 exception.
- II: the new private helpers carry `\pre`/`\post` doxygen and `SG_ENSURE`
  postconditions the way the extracted decisions at
  `provider.cpp:516-557` and `plan.cpp:668-692` state them. A malformed
  address is a recoverable refusal, never a contract abort: these inputs
  come from a user option, and `resolveLeafCore` is total (no `\pre`).
- III: R-DCUT with TDD mode. Each FR gets a named scenario observed red
  before the implementing change turns it green (test plan below). This
  plan records TDD mode as III requires.
- IV: doxygen on the new interface points; the raw-entry table carries its
  contract comment in `source/counters/detail/pmu.hpp`, the place
  `pmu.hpp:278-292` already reserves for an address-keyed side table.
- IV.1: user-visible behavior on an existing option. The carriers are the
  option description (`source/harness/options.cpp:560-565`), the README
  quick row in place (`README.md:970`), and the guide counter section
  (`docs/pages/harness.md:637-651`) with its option table row
  (`harness.md:675`), in one change (FR-010). No new example region: D-8
  states the reason, an example built on host config words cannot run on
  the CI matrix.
- V: `format-check` clean; names follow V.1 (`kMaxRawHexDigits` under N-8,
  camelBack functions under N-2); `hasRawCodeShape` keeps its V.2-clean
  spelling untouched.
- VI: tests land in the same change, one per FR; the coverage gate reads
  below keep 100 percent lines, the recorded branch allowance of 21
  (`tools/dbc/coverage_gate.sh:108`), and the marker count at or below 84
  with no new `LCOV_EXCL` marker.
- VII: measurements named in research R-15: resolve-path syscall counts and
  a diff read that proves the report path and timed loop carry no new
  instruction. The reasoned exemption is recorded: resolve-time only, no
  hot-loop change, and the VII baseline infrastructure stays under the
  constitution's open deferral.
- VIII: gates planned, none weakened (research R-14): `ci-rocky` and
  `ci-ubuntu` release builds with the full `ctest` suite, `ci-sanitize`,
  `ci-tsan`, clang-tidy and cppcheck through the `ci-ubuntu` launchers,
  the `readability-identifier-naming` zero-finding read that
  `WarningsAsErrors` in `.clang-tidy` turns into a build failure,
  the coverage target at `cmake/coverage.cmake:68-135` with the
  `mismatch,negative` allowances only (`coverage.cmake:73`, the
  `inconsistent` class stays gone, `coverage.cmake:47-59`),
  `format-check`, `spell-check`, `prose-lint`, `time_source_gate.sh`, and
  the example-drift gate. No CI workflow file is touched.
- IX: spec, plan, research, data-model, quickstart here; tasks and
  implement follow.
- X: the change stays minimal: one predicate, one table, three read arms,
  two harness arms, one option text. No new public surface, no new
  abstraction serving one caller, no speculative hook (X.2).
- XI: prose in this plan and in every comment obeys XI.1 through XI.7; the
  pinned refusal texts are verbatim quotations, exempt under XI.6.

Gate verdict after Phase 0 and Phase 1: PASS, no violation to justify.

## Design

### Logical view: the raw path end to end

1. Gather. The option layer keeps every address verbatim behind its origin
   (`source/harness/options.cpp:1441-1462`, `1631-1653`); it runs no shape
   check, exactly as at base.
2. Identity checkpoint. `counterAddressFailure` (`runner.cpp:986-1030`),
   called from `source/harness/cli.cpp:960-970`, gains three arms for a
   raw-shaped leaf, in this order: width refusal (FR-005, ahead of the
   device lookup), device read, then the state read. The state reaches the
   harness through the raw-aware `resolveLeafCore`, whose returned
   `LeafCore` carries the settled `avail`
   (`include/speedgun-ng/counters_measurement.hpp:103`); the counters
   layer runs the one probe pair per address through `pmuProbe`
   (`provider.cpp:667-730`), and the harness opens nothing itself. The
   state prints through the standing `listed as <state> (FR-023)` line
   (`runner.cpp:1012-1013`, state words `source/harness/report.cpp:32-39`).
   A too-wide leaf answers the FR-005 line in the same three-field shape
   (`runner.cpp:1022-1026`) and opens nothing.
3. Column build. The column loop (`runner.cpp:1435-1502`) keeps its
   catalog search for a leaf that is not raw-shaped. A raw-shaped leaf
   skips that search, because the published listings never carry a raw
   entry (FR-015), and reaches the same raw-aware lookup through the typed
   route `object->counter<Dim<0, 1>>`
   (`include/speedgun-ng/counters_system.hpp:143-178`). Its unit token
   `none` maps through `unitFromToken` and `dimensionOf` to events^1
   (`include/speedgun-ng/counters_core.hpp:162-184`, `:205-206`), so no
   `CatalogEntry` is needed to pick the dimension. The spine dedups by the
   address string (`counters_measurement.hpp:132-141`), which gives
   FR-008's two columns over one leaf and the `r0043`/`r43` distinctness.
4. Plan compile. `compileCore` (`source/counters/plan.cpp:402-488`) reads
   the raw-entry table for a raw-shaped address where the tree scan sits
   (`plan.cpp:445-466`), then the standing gate
   (`plan.cpp:474`, `plan.cpp:668-692`) and the standing provider grouping
   (`plan.cpp:502-537`) run unchanged. The compiled plan gains the raw
   leaf's record (FR-015).
5. Window. `resolve` (`source/counters/linux_pmu/group_io.cpp:153-198`)
   finds the device by path and, for a raw leaf, the synthesized entry in
   the raw-entry table; `fillAttr` (`group_io.cpp:200-225`) is untouched
   and takes `words = {{0, configWord}}`, which reproduces the exclusion,
   group-read, and time-pair settings of a catalog row (FR-003).
6. Report. Console and JSON print the label as the address string with no
   code change (`report.cpp:727-748`, `json_report.cpp:532-538`, `873-880`).
   The option-disclosure line prints the value as the source spelled it
   (`cli.cpp:106-128`). No key joins the document; the schema check at 1.16
   passes because nothing moved (`json_report.cpp:58`,
   `test/source/harness_json_schema_test.cpp`).

### The raw-entry table

One table, keyed by the canonical address, the shape
`system.cpp:165-177` and `pmu.hpp:278-292` state for the probed-kinds
records. Value: the synthesized entry (name as typed, `words`, availability,
read mode, probed kinds, device settings carried as a `LeafRecord`,
`source/counters/detail/core.hpp:30-39`). Written once, by the raw probe
step, before any read. Readers: the core lookup, `compileCore`'s record
read, and `group_io resolve`. Never readers: `PmuProvider::enumerate`
(`provider.cpp:797-862`), the near-miss candidate lists
(`system.cpp:306-312`, `system.cpp:582-585`), and every listing surface.
The tree itself gains no leaf.

### Physical view: file-level changes

```text
CMakeLists.txt                      VERSION 0.35.0 with the reservation-rule comment
                                    (block at :78-86); counters SOVERSION 8 kept, its
                                    comment records the no-public-header condition
                                    (:167-172, the specs/048 note at :453-455 is the
                                    precedent); harness SOVERSION 22 untouched (:456)
source/counters/detail/pmu.hpp      raw-shape predicate, kMaxRawHexDigits, raw-entry
                                    table declare-and-contract, raw synthesis entry
                                    point, beside :278-292
source/counters/system.cpp          resolveLeafCore raw arm (:573-590); table storage
                                    beside probedKindTable (:165-177)
source/counters/plan.cpp            compileCore record read consults the table
                                    (:445-466); no gate or grouping change
source/counters/linux_pmu/provider.cpp
                                    raw synthesis from the settled device facts (the
                                    loadDevice type and cpumask reads at :325-342
                                    stand at device load), per-kind probes on the
                                    probeDevice pattern (:258-285), the one errno
                                    table (:722-729), scopeSettledState (:537-557);
                                    off-Linux stub beside :45-64
source/counters/linux_pmu/group_io.cpp
                                    resolve raw arm: table entry over the found device
                                    (:153-198); fillAttr untouched
source/harness/runner.cpp           counterAddressFailure arms (:986-1030) and column
                                    build raw arm (:1435-1502); shape check reuses the
                                    harness predicate (hunt_selection.hpp:116-131)
source/harness/options.cpp          counter row description text only (:560-565)
README.md                           row 970 text, in place (cap 1004 at
                                    tools/docs/example_drift.py:256; page at 1003)
docs/pages/harness.md               counter section :637-651 and option row :675;
                                    :2577-2579 and :2621 stand untouched
test/CMakeLists.txt                 build blocks in two waves (counters
                                    pattern at :553-559, harness pattern at
                                    :1837-1841); each `add_test` line joins
                                    its unit's green task
test/source/counters_raw_event_test.cpp   new unit (below)
test/source/harness_raw_events_test.cpp   new unit (below)
```

Untouched, by ruling: `include/` (D-6), `hunt_selection.hpp`,
`hunt_cli.cpp`, `hunt.cpp`, `benchmark.hpp`, everything under
`docs/schema/`,
`json_report.cpp`, `json_load.cpp`, `report.cpp`, `catalog.cpp`,
`metric_formula.cpp`, everything under `example/`, every existing test
file, every part of `options.cpp` outside the counter row text at
`:560-565` (the address and environment machinery at `:1441-1462` and
`:1631-1653` stands untouched), and `.github/` (no CI edits).

### Test plan (TDD, one test per FR)

New unit `test/source/counters_raw_event_test.cpp` (scripted kernel and
scripted sysfs, the `counters_pmu_provider_test.cpp:179-268` fixtures;
private seams on the `counters_linux_pmu_seam_test.cpp:50-59` include
pattern):

| Scenario | FR |
|---|---|
| `rawAcceptRecordsTypeAndConfigWord`: `cpu/rc0` on a scripted type-4 device; the recorded attribute names type 4 and config `0xc0` | FR-001, SC-001 scripted pin |
| `rawAttrMatchesCatalogRowAttr`: exclusion, group-read, and time-pair words equal a catalog row's | FR-003, FR-011 |
| `rawProbeErrnoArms`: EINVAL, EOPNOTSUPP, ENOENT to not-encodable; EACCES to permission-blocked | FR-004, SC-002 |
| `rawProbeScopeRelabel`: cpumask device plus EACCES settles scope-refused | FR-004 edge |
| `absentDeviceRefusesWithoutProbe`: no sysfs directory answers the standing no-object wording; zero scripted opens | FR-002 |
| `wideLeafNeverProbes`: seventeen digits refused, never truncated | FR-005 library side |
| `probeRunsOncePerAddress`: repeated resolve, one probe pair | FR-004, FR-008 |
| `typedRouteUnitNoneEventsOne`: `Object::counter<Dim<0,1>>("rc0")` succeeds, unit token `none` (dimensionOf at `counters_core.hpp:205-206`); `Dim<1,0>` answers the standing dimension line | FR-007 |
| `formulaResolvesRawEventName`: a synthetic table row whose formula event name is raw-shaped resolves through `metric_formula.cpp:1273-1305` | FR-007 |
| `scriptedPlanCompilesAndMeasuresRawLeaf`: `compileCore` over a resolved raw leaf, gate pass, one open, folded value | FR-011, FR-015 |
| `listingsNeverReadRawTable`: after a raw resolve, the seeded leaf count, the `--catalog` shape, and the near-miss candidates answer as at base | FR-015 |
| `duplicateSpellsDistinctAddresses`: `r43` twice is one entry; `r0043` and `r43` are two | edge, FR-008 |

New unit `test/source/harness_raw_events_test.cpp` (scripted kernel under
the real provider plus in-process or forked runs, the
`harness_limit_disclosure_test.cpp:288-310` and `harness_gap_test.cpp:1-12`
patterns; every address device is the scripted one, no made-up cpu and no
made-up kernel figure):

| Scenario | FR |
|---|---|
| `unknownDeviceRawLine`: `nosuch/r4000` answers the no-object line, status 2, no benchmark ran | FR-002, US2-1 |
| `wideLeafWidthLine`: `cpu/r<17 digits>` answers `raw event code exceeds 16 hex digits` in the origin, address, message shape; scripted opens for that leaf: zero | FR-005, US2-2, SC-002 |
| `wideLeafBeatsAbsentDevice`: too wide plus unpublished device answers the width line | FR-005, FR-011 |
| `refusedProbeListedAsLines`: scripted EINVAL answers `listed as not-encodable (FR-023)`; scripted EACCES answers `listed as permission-blocked (FR-023)` | FR-004, US2-3 |
| `scopeRefusedRawLine`: scripted cpumask device plus EACCES prints `listed as scope-refused (FR-023)` at the standing status | FR-011 edge |
| `refusedRawEndsRunBeforeMeasurement`: one refused raw address among good ones; no row prints | edge |
| `duplicateRawAddressTwoColumns`: one address twice; two labels, one scripted open for the word | FR-008, US1-3 |
| `rawColumnDisclosesEverywhere`: console label, `metricLabels`, each row's `metrics[].label`, and the option line of a file- or environment-sourced run carry `cpu/rc0`; the written document still passes the schema check at 1.16 | FR-006, US3, SC-003 |
| `limitLineCountsRawColumn`: the raw column takes a seat against the scripted device limit and the `counter limit:` line stays byte-identical | FR-009 |
| `rawRunConsoleAndJson`: scripted run prints the column with its value and writes the JSON pair | US1 |

Existing files stay unedited in their expectations (SC-004): the pinned
counter refusal lines (`harness_cli_test.cpp:729-755`), the pinned hunter
line (`harness_hunt_cli_test.cpp:394-398` against
`specs/029-hunter-core/contracts/hunt-cli.md:83`), the catalog and listing
scenarios, and the schema and report-shape units all answer at base.

FR map, one named check for each requirement. FR-001
`rawAcceptRecordsTypeAndConfigWord`; FR-002
`absentDeviceRefusesWithoutProbe` and `unknownDeviceRawLine`; FR-003
`rawAttrMatchesCatalogRowAttr`; FR-004 `rawProbeErrnoArms`,
`rawProbeScopeRelabel`, `refusedProbeListedAsLines`, and
`probeRunsOncePerAddress`; FR-005 `wideLeafNeverProbes` and
`wideLeafWidthLine`, with `wideLeafBeatsAbsentDevice` for the arm order;
FR-006 `rawColumnDisclosesEverywhere`; FR-007
`typedRouteUnitNoneEventsOne` and `formulaResolvesRawEventName`, the
hunter half of that requirement carried by the untouched pin at
`harness_hunt_cli_test.cpp:394-398`; FR-008
`duplicateRawAddressTwoColumns` and `duplicateSpellsDistinctAddresses`;
FR-009 `limitLineCountsRawColumn`; FR-010 the three carrier reads of step
6; FR-011 every scenario in the two new units; FR-012 the version and
SOVERSION reads of step 7; FR-013 the gate reads of step 8; FR-014 the
commit and pull-request reads of step 10; FR-015
`listingsNeverReadRawTable` and `scriptedPlanCompilesAndMeasuresRawLeaf`.
FR-012 and FR-013 readings land in `results/`.

SC map, one named reading per criterion. SC-001
`rawAcceptRecordsTypeAndConfigWord` beside the step-9 host pair record;
SC-002 the scripted refusal scenarios
(`absentDeviceRefusesWithoutProbe`, `wideLeafNeverProbes`,
`rawProbeErrnoArms`, `wideLeafWidthLine`, `refusedProbeListedAsLines`);
SC-003 `rawColumnDisclosesEverywhere` with the schema read; SC-004 the
step-5 pin reads and the step-8 full suite; SC-005 the step-8 coverage
arms; SC-006 the step-7 number readings.

## Step and Check Plan (X.4)

```text
1. Base readings          -> verify: ci-rocky ctest green; marker read
   `git grep -c LCOV_EXCL -- source include` sums 84; version reads answer
   0.34.0, 1.16, 22, 8; the readings land in
   `specs/051-raw-events/results/base-readings.md`
2. Red counters unit      -> verify: counters_raw_event_test.cpp builds at
   base and its direct run fails; the add_test line joins the green task
3. Counters layers        -> verify: that unit green (shape, probe, table,
   lookup, plan read, window arms)
4. Red harness unit       -> verify: harness_raw_events_test.cpp builds at
   base and its direct run fails, in its own tree build/raw-red
5. Harness arms           -> verify: that unit green; harness_cli_test,
   harness_hunt_cli_test, harness_json_schema_test unchanged and green
6. Docs carriers          -> verify: one grep exits 0 in each carrier for
   the raw sentence (options.cpp:560-565, harness.md:637-651 and :675,
   README.md:970); `python3 tools/docs/example_drift.py` exits 0; README
   line count ≤ 1004
7. Numbers                -> verify: CMakeLists VERSION 0.35.0; SOVERSION
   8 and 22 read unchanged; json_report.cpp:58 unchanged; no file under
   include/ in `git diff --name-only`; readings land in results/
7a. Lane commit           -> verify: one commit carries source, tests, docs
   and the version block; `results/commits.md` holds the sha read from
   `git rev-parse HEAD`, the built-head handle every gate names
8. Gates                  -> verify: ctest full; coverage target through
   cmake/coverage.cmake (100.0 lines, shortfall ≤ 21, markers ≤ 84, no new
   LCOV_EXCL, `lcov` runs with `mismatch,negative` only); ci-ubuntu build
   with clang-tidy and cppcheck reports no new finding; ci-sanitize and
   ci-tsan suites clean; format, spell, prose, time-source reads exit 0
9. Host acceptance        -> verify: one run with `--counter=cpu/instructions
   --counter=cpu/rc0` prints two positive columns of config word 0xc0;
   results record names family 26, model 68, the alias, and the words
10. Lane record           -> verify: one squashed commit with the template;
   `git ls-remote` equals the local head; one open pull request; no merge,
   no issue close
```

## Project Structure

### Documentation (this feature)

```text
specs/051-raw-events/
├── spec.md
├── plan.md              # this file
├── research.md          # R-numbered decisions with measurements
├── data-model.md        # raw address, raw entry, raw-entry table, verdicts
├── quickstart.md        # the reviewer's read set
├── checklists/requirements.md
├── tasks.md             # /speckit.tasks output
└── results/             # FR-012 and FR-013 readings, host pair record,
│                        # base readings, red-unit reads, the commit
│                        # handle and the pull-request body draft
```

The standing line shapes are contract text already carried by the spec
(FR-004, FR-005) and by the pinned tests, so this feature adds no
`contracts/` directory.

### Source Code (repository root)

The physical view above is the file list. New code sits in the five
counters files, two harness files, and two new test units; no directory is
created outside `test/source/`.

**Structure Decision**: the raw arm joins the one existing lookup, the one
existing probe, and the one existing print path; the only new structure is
the address-keyed table beside the frozen tree.

## Parallelization

One lane, one worktree. Seven waves, each naming the files it opens, so no
two tasks in one wave open one file.

| Wave | Steps | Files opened |
|---|---|---|
| W1 | 1 | `specs/051-raw-events/results/base-readings.md` |
| W2 | 2 | `test/source/counters_raw_event_test.cpp`, its `add_executable` build block in `test/CMakeLists.txt` |
| W3 | 3 | `source/counters/detail/pmu.hpp`, `source/counters/system.cpp`, `source/counters/plan.cpp`, `source/counters/linux_pmu/provider.cpp`, `source/counters/linux_pmu/group_io.cpp`, the counters `add_test` line in `test/CMakeLists.txt` |
| W4 | 4 | `test/source/harness_raw_events_test.cpp`, its `add_executable` build block in `test/CMakeLists.txt` |
| W5 | 5 | `source/harness/runner.cpp`, the harness `add_test` line in `test/CMakeLists.txt` |
| W6 | 6 and 7 | `source/harness/options.cpp`, `README.md`, `docs/pages/harness.md`, `CMakeLists.txt`, `specs/051-raw-events/results/` |
| W7 | 7a and 8 to 10 | `specs/051-raw-events/results/` and the branch |

Order: W3 follows W2, W5 follows W4 and W3, W7 follows every wave. W2 and
W4 each build their red state against the unmodified tree, so W4 may start
beside W2 and W3; its red reads configure their own tree
(`cmake --preset=ci-rocky -B build/raw-red`), so the shared `build` stays
single-writer. Their `test/CMakeLists.txt` blocks are the one file two
waves share: the tasks writer keeps the build blocks in separate tasks
with the counters block first, and each `add_test` line joins its unit's
green task, so the file's writers run in the order T005, T008, T015,
T018. W6 opens no file the chain opens,
so it runs beside W2 through W5. `results/` takes one file per reading, so
W1, W6 and W7 name different files there.

## Complexity Tracking

No violations. One predicate, one table, three read arms, two harness
arms, one option text, no new public surface, no new dependency.
