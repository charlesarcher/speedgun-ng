# Tasks: Raw Event-Code Support (G-31)

**Input**: design documents from `specs/051-raw-events/`

**Branch**: `051-raw-events` | **Spec**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md)

**Prerequisites**: `plan.md` (the Step and Check Plan, the FR map, the wave
table), `spec.md` (user stories, FRs, edge list), `research.md` (R-01 to
R-16), `data-model.md` (E-1 to E-6), `quickstart.md` (the reviewer's read
set), `.specify/memory/constitution.md` (Principles VII, VIII, X).

**Read at base**: every `path:line` and every count stands as read at
`0fce6651`, the specs/049 squashed head and the parent of this lane's
documents. A read that compares against the base names that commit.

**Tests**: TDD mode, the mode the plan's Constitution Check records under
Principle III. Each new unit stands written, built at base, and observed
red by direct run before the code that turns it green: T004 to T006 for
the counters unit, T007 to T009 for the harness unit. The `add_test`
line joins each unit at its green task (T015, T018), so the registered
suite answers at base while a unit reads red. The verify of an implementation task is
that unit going green (T015, T018). No task rewrites a red unit so that it
passes, and no task edits an existing test expectation (SC-004).

**Organization**: the wave table of plan.md fixes the order. W1 the base
readings, W2 the counters red, W3 the counters layers, W4 the harness red,
W5 the harness arms, W6 the docs carriers and the numbers, W7 the gates,
the acceptance run, the records and the lane. Order rule from the plan: W3
follows W2, W5 follows W4 and W3, W7 follows every wave, and W6 opens no
file the chain opens, so it runs beside W2 through W5. Phase headings group
the work; numbering follows the phase order W1, W2, W4-authoring, W3, W5,
W6, W7. The dependency lines decide the order; the numbers are a
convenience.

## Format: `[ID] [P?] [Tag] Description`

- **[P]**: the task opens files no other open task opens, and the plan's
  wave table allows the concurrency. Two tasks over `test/CMakeLists.txt`
  never run together: the counters block (T005) lands before the harness
  block (T008), and each `add_test` line lands at the unit's green task,
  T015 then T018. A [P] author task writes files and reads only diffs and
  greps: the gate task of its wave compiles and runs the suite once
  (Principle X.4 keeps each check binary). A [P] gate task owns a build
  tree no other [P] gate task opens.
- **[R]**: a read-only task over commands and one `results/` record. It
  changes no source file.
- **[O]**: an orchestrator task over the git or pull-request surface. An
  [O] task carries no [P].
- **[Tag]**: the story, requirement or criterion the task closes (`US1`,
  `FR-005`, `SC-005`).
- Every task names the exact files it opens and its exact verify commands.

## Build discipline on this host

- `ci-rocky`, `ci-ubuntu` and `ci-ubuntu-test` inherit `ci-build`, whose
  `binaryDir` is `${sourceDir}/build`, so they share one tree: one build
  there at a time, `-j 10`, no [P] pair over it. `ci-sanitize` stands at
  `build/sanitize`, `ci-tsan` at `build/tsan`, `ci-coverage` at
  `build/coverage`, each under the repository disk. `/tmp` is a small
  tmpfs, and a full tree there fakes a coverage drop.
- A preset switch never clears a cache variable an earlier preset set.
  Before a `ci-rocky` read over a tree that ran `ci-ubuntu`, purge first:
  `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK`.
- `ci-sanitize` and `ci-tsan` take the compiler the environment names, and
  host GCC cannot build the sanitize leg, so both configure with
  `CXX=clang++`. Each sanitizer row names the compiler its cache holds.
- The base registered entry count is 190 (`ctest --test-dir build -N`
  at `0fce6651`). Each unit's `add_test` line joins its green task (T015,
  T018), so 190 stands while either unit reads red and the two lines take
  the count to 192.

## Scope guard (Principle X.3, plan Physical view)

No task edits: `.github/`, anything under `include/`, `docs/schema/`,
`source/harness/detail/hunt_selection.hpp`, `source/harness/hunt_cli.cpp`,
`source/harness/hunt.cpp`, `source/harness/report.cpp`,
`source/harness/json_report.cpp`, `source/harness/json_load.cpp`,
`source/harness/catalog.cpp`, `source/counters/linux_pmu/metric_formula.cpp`,
`example/`, or an existing test file. T037 reads the guard over the whole lane.

---

## Phase 1: Wave 1 - the base readings

**Purpose**: the lane measures the ground it lands on, so every later
figure reads as a difference. No source file changes here.

- [x] T001 [R] Run the checked-in daily loop at the lane base and take the
      two listing figures the head compares against:
      `cmake --preset=ci-rocky`, `cmake --build build -j 10`,
      `ctest --test-dir build --output-on-failure --no-tests=error -j 10`,
      then `./build/example/docs_command_line --catalog | wc -l` and
      `ctest --test-dir build -N | tail -1`. Depends on nothing: this task
      opens the lane.
      Verify: each command exits 0; the ctest summary reports 0 failed
      entries and no `Not Run` entry; the registered count reads 190 and the
      `--catalog` figure stands recorded.

- [x] T002 [R] Write `specs/051-raw-events/results/base-readings.md`: the
      base commit `0fce6651`; the host figures from
      `grep -m1 'vendor_id' /proc/cpuinfo`,
      `grep -m1 'cpu family' /proc/cpuinfo`,
      `grep -m1 '^model' /proc/cpuinfo` (AuthenticAMD, family 26, model
      68); `cat /sys/bus/event_source/devices/cpu/type` (4) and
      `cat /sys/bus/event_source/devices/cpu/events/instructions`
      (`event=0xc0`); the T001 suite verdict, pass count and time; the
      `--catalog` line count; the registered count 190; the marker read
      `git grep -c LCOV_EXCL -- source include | awk -F: '{s+=$2} END {print s}'`
      (84); the four number reads at base
      (`grep -n '^    VERSION 0.34.0' CMakeLists.txt`,
      `grep -n '^    SOVERSION 8' CMakeLists.txt`,
      `grep -n '^    SOVERSION 22' CMakeLists.txt`,
      `grep -n 'kSchemaVersion = "1.16"' source/harness/json_report.cpp`);
      `grep PACKAGE_VERSION build/speedgun-ngConfigVersion.cmake` (the
      configure prints no version line of its own); and `wc -l README.md`
      (1003, under the `PAGE_LINE_CAP = 1004` of
      `tools/docs/example_drift.py:256`). Depends on T001.
      Verify: `test -s specs/051-raw-events/results/base-readings.md`
      exits 0, and `grep -c` over that file answers at least one hit for
      each of `0.34.0`, `SOVERSION 8`, `SOVERSION 22`, `1.16`, `84`,
      `1003`, `190`, `0xc0`.

- [x] T003 [R] Read the two analyzers at base, so the head read of T034 is
      a difference and not a guess, in a tree of its own (a name on the
      spell skip list, so the shared ci-rocky `build` stays untouched):
      `cmake --preset=ci-ubuntu -B build-tidy`,
      `cmake --build build-tidy --target clean`, then
      `cmake --build build-tidy -j 10 2>&1 | tee /tmp/sg051-base-analyzer.log`.
      Record into `results/base-readings.md` one count per edited file from
      `grep -c ': warning:'` over that log for `source/counters/detail/pmu.hpp`,
      `source/counters/system.cpp`, `source/counters/plan.cpp`,
      `source/counters/linux_pmu/provider.cpp`,
      `source/counters/linux_pmu/group_io.cpp`, `source/harness/runner.cpp`,
      `source/harness/options.cpp`, beside the translation-unit count from
      `grep -c 'Building CXX object' /tmp/sg051-base-analyzer.log`. The
      shared `build` tree is never reconfigured by this task. Depends on T002: the
      figures join the record it wrote.
      Verify: the build exits 0 at base;
      `grep -cF '[readability-identifier-naming]' /tmp/sg051-base-analyzer.log`
      answers 0, the arm Principle VIII's name check reads (the bare name
      also matches a standing NOLINT comment at
      `test/source/detail/gcov_flush.hpp:18`, so the bracketed form is
      the check-attributed read); the per-file counts and the unit count
      stand in the record.

**Independent test**: no source file moved, and one document names every
figure the later phases compare against.

---

## Phase 2: Wave 2 - the counters unit, written red (US1, US2)

**Goal**: the counters side of raw recognition has one named unit before
any counters file changes.

- [x] T004 [P] [US1] [US2] Write
      `test/source/counters_raw_event_test.cpp` with twelve case functions
      named exactly as the counters scenario table of plan.md names them:
      `rawAcceptRecordsTypeAndConfigWord` (FR-001, SC-001 scripted pin: a
      scripted `cpu` device of type 4, address `cpu/rc0`, and the recorded
      attribute naming type 4 and config `0xc0`);
      `rawAttrMatchesCatalogRowAttr` (FR-003: the exclusion, group-read and
      time-pair words equal a catalog row of that device);
      `rawProbeErrnoArms` (FR-004: `EINVAL`, `EOPNOTSUPP` and `ENOENT`
      settle not-encodable, `EACCES` settles permission-blocked);
      `rawProbeScopeRelabel` (FR-004 edge: a cpumask device plus `EACCES`
      settles scope-refused); `absentDeviceRefusesWithoutProbe` (FR-002: a
      device with no sysfs directory answers the standing
      `no object at path` wording with zero scripted opens);
      `wideLeafNeverProbes` (FR-005 library side: seventeen digits refused,
      the hex text never truncated); `probeRunsOncePerAddress` (FR-004,
      FR-008: a repeated resolve of one address opens one probe pair);
      `typedRouteUnitNoneEventsOne` (FR-007: the typed route on `rc0`
      reaches the unit token `none` and the events^1 dimension of
      `include/speedgun-ng/counters_core.hpp:205-206`, and a wrong
      dimension answers the standing line);
      `formulaResolvesRawEventName` (FR-007: a synthetic metric row whose
      event name is raw-shaped resolves through
      `source/counters/linux_pmu/metric_formula.cpp:1273-1305`);
      `scriptedPlanCompilesAndMeasuresRawLeaf` (FR-011, FR-015:
      `compileCore` over a resolved raw leaf passes the standing gate,
      opens once, folds a value); `listingsNeverReadRawTable` (FR-015:
      after a raw resolve the seeded leaf count, the catalog shape and the
      near-miss candidates answer as at base);
      `duplicateSpellsDistinctAddresses` (edge, FR-008: `r43` twice is one
      entry, `r0043` and `r43` are two addresses). Fixtures: the scripted
      kernel and scripted sysfs pair of
      `test/source/counters_pmu_provider_test.cpp:179-268`, the open
      recorder at `:184`, the builder `writeDevice` with its `withCpumask`
      arm at `:247`, and the private-seam includes on the
      `test/source/counters_linux_pmu_seam_test.cpp:50-59` pattern. The
      frameworkless `fail`/`check` convention with `main` calling each case
      once. FR-011 bounds: no sleep, no clock read, no made-up cpu, no
      invented kernel figure. Depends on nothing in the chain: this file is
      new. Do not build and do not run.
      Verify: `grep -c -F <name> test/source/counters_raw_event_test.cpp`
      answers at least 1 for each of the twelve names, and
      `grep -cE 'sleep|steady_clock|chrono::' test/source/counters_raw_event_test.cpp`
      answers 0.

- [x] T005 [US1] [US2] Add the counters unit build to `test/CMakeLists.txt`
      in one block after the `counters_pmu_provider_test` block at
      `test/CMakeLists.txt:553-559`: `add_executable(counters_raw_event_test source/counters_raw_event_test.cpp)`,
      `target_link_libraries(counters_raw_event_test PRIVATE speedgun-ng::speedgun-ng)`,
      `target_compile_features(counters_raw_event_test PRIVATE cxx_std_23)`.
      The `add_test(NAME counters_raw_event_test COMMAND counters_raw_event_test)`
      line joins at T015, so the registered suite keeps its base answers
      while this unit reads red. This block lands before the harness block
      of T008, because the two registrations share one file. Depends on
      T004 and on T003: the analyzer read of W1 holds the shared `build`
      tree until it closes.
      Verify: `cmake --build build --target counters_raw_event_test -j 10`
      exits 0 (the unit compiles against the base tree);
      `ctest --test-dir build -N | tail -1` answers 190.

- [x] T006 [R] [US1] [US2] Observe the counters red against the unmodified
      counters layer: `cmake --build build --target counters_raw_event_test -j 10`,
      then run that built binary directly; its frameworkless `main`
      returns nonzero on a failed check. The unit is unregistered until
      T015, so no ctest read of the standing suite can go red on it.
      Write `specs/051-raw-events/results/red-units.md` with the counters
      half: each failing case name with the one line that shows the
      missing behavior. A case that cannot compile at base is a
      pause-and-ask signal for the orchestrator, never a seam to
      pre-create. Depends on T005.
      Verify: the direct binary run exits nonzero;
      `test -s specs/051-raw-events/results/red-units.md` exits 0 and names
      at least one failing case.

**Independent test**: one unit that builds at base, runs directly, and
fails, with its failing names on record.

---

## Phase 3: Wave 4 - the harness unit, written red (US2, US3)

**Goal**: the harness side has one named unit. This wave runs beside W2 and
W3: it opens `test/source/harness_raw_events_test.cpp` and one block of
`test/CMakeLists.txt`, and no counters or harness source file.

- [x] T007 [P] [US1] [US2] [US3] Write `test/source/harness_raw_events_test.cpp`
      with ten case functions named exactly as the harness scenario table
      of plan.md names them: `unknownDeviceRawLine` (FR-002, US2-1:
      `nosuch/r4000` answers the standing no-object line at status 2 and no
      benchmark ran); `wideLeafWidthLine` (FR-005, US2-2, SC-002: `cpu/r`
      plus seventeen digits answers `raw event code exceeds 16 hex digits`
      in the origin, address, message shape, with zero scripted opens for
      that leaf); `wideLeafBeatsAbsentDevice` (FR-005: a too-wide leaf that
      also names an unpublished device answers the width line);
      `refusedProbeListedAsLines` (FR-004, US2-3: a scripted `EINVAL`
      answers `listed as not-encodable (FR-023)` and a scripted `EACCES`
      answers `listed as permission-blocked (FR-023)`);
      `scopeRefusedRawLine` (FR-011 edge: a scripted cpumask device plus
      `EACCES` prints `listed as scope-refused (FR-023)` at the standing
      status); `refusedRawEndsRunBeforeMeasurement` (edge: one refused raw
      address among good ones prints no row); `duplicateRawAddressTwoColumns`
      (FR-008, US1-3: one address written twice prints two labels over one
      scripted open for the word); `rawColumnDisclosesEverywhere` (FR-006,
      US3, SC-003: the console label, `metricLabels`, each row's
      `metrics[].label`, and the option line of a run whose option came from
      a file or the environment all carry `cpu/rc0`, and the written
      document passes the schema check at `1.16`);
      `limitLineCountsRawColumn` (FR-009: the raw column takes a seat
      against the scripted device limit and the `counter limit:` line stays
      byte-identical); `rawRunConsoleAndJson` (US1: the scripted run prints
      the column with its value and writes the JSON pair). Shape: a
      scripted kernel under the real provider with one re-exec per scenario,
      the pattern of `test/source/harness_limit_disclosure_test.cpp:1-40`
      and its scripted run at `:288-310`, beside the one-script-per-mode
      re-exec of `test/source/harness_gap_test.cpp:1-12`. Each child
      registers `ClockProvider` before the scripted `PmuProvider`, the
      order a scripted `speedgunMain` needs. Every device is the scripted
      one. FR-011 bounds as in T004. Do not build and do not run.
      Verify: `grep -c -F <name> test/source/harness_raw_events_test.cpp`
      answers at least 1 for each of the ten names;
      `grep -cE 'sleep|steady_clock|chrono::' test/source/harness_raw_events_test.cpp`
      answers 0.

- [x] T008 [US2] [US3] Add the harness unit build to `test/CMakeLists.txt`
      in one block after the `harness_hunt_cli_test` block at
      `test/CMakeLists.txt:1837-1841`: `add_executable(harness_raw_events_test source/harness_raw_events_test.cpp)`,
      `target_link_libraries(harness_raw_events_test PRIVATE speedgun-ng::harness)`,
      `target_include_directories(harness_raw_events_test PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)`,
      `target_compile_features(harness_raw_events_test PRIVATE cxx_std_23)`.
      The `add_test(NAME harness_raw_events_test COMMAND harness_raw_events_test)`
      line joins at T018, so the registered suite keeps its base answers
      while this unit reads red. Depends on T005 (the counters block
      stands first in this same file) and on T007. Every read of this task
      runs in a tree of its own, `build/raw-red`, so W3 may build the
      shared `build` beside it.
      Verify: `cmake --preset=ci-rocky -B build/raw-red` then
      `cmake --build build/raw-red --target harness_raw_events_test -j 10`
      exits 0 (the unit compiles against the base tree);
      `ctest --test-dir build/raw-red -N | tail -1` answers 190.

- [x] T009 [R] [US2] [US3] Observe the harness red while
      `source/harness/runner.cpp` still stands unmodified, in the same
      dedicated tree as T008:
      `cmake --build build/raw-red --target harness_raw_events_test -j 10`,
      then run that built binary directly. The unit is unregistered until
      T018. Append the harness half to
      `specs/051-raw-events/results/red-units.md`: each failing case name,
      and any case that already answers green because the counters arm of
      W3 carries it. Depends on T006 (the same record) and on T008; T016
      already names this read in its own depends line.
      Verify: the direct binary run exits nonzero; the record names each
      failing harness case.

**Independent test**: one unit that builds at base in its own tree, runs
directly, and fails while the harness arms stand absent, with its failing
names on record.

---

## Phase 4: Wave 3 - the counters layers (US1, US2, FR-015)

**Order**: W3 follows W2. One writer per file. T010 declares the shape and
the table; T011, T012 and T013 open three disjoint files beside each other;
T014 adds the lookup arm and the storage; T015 turns the unit green.

- [x] T010 [US1] Declare the raw shape and the table in
      `source/counters/detail/pmu.hpp`: the raw-shape predicate
      `hasRawCodeShape`, a lower-case `r` then one or more hex digits of
      either case (the shape of FR-001; R-01 is the one contract in two
      places, beside the harness predicate at
      `source/harness/detail/hunt_selection.hpp:116`), `kMaxRawHexDigits`
      at 16 under the N-8 constant rule, the address-keyed raw-entry table
      `rawEventTable` with its contract comment beside the
      probed-kinds readers at `:278-292` (key: the canonical
      `<device>/<leaf>`; readers: the core lookup, the plan record read, the
      window resolve; never a reader: `PmuProvider::enumerate`, the
      near-miss candidate loops, every listing surface), and the raw
      synthesis entry point `synthesizeRawEntry`. Carry `\pre`/`\post` doxygen in the style of
      the extracted decisions, and quote FR-005's width rule in the table
      comment. Depends on T006.
      Verify: `cmake --build build -j 10` exits 0 with no `error:` line;
      `grep -c 'kMaxRawHexDigits' source/counters/detail/pmu.hpp` answers
      at least 1.

- [x] T011 [P] [US1] Implement the synthesis in
      `source/counters/linux_pmu/provider.cpp`: the raw entry built from the
      device facts the loader already settled (the `type` read at `:325-332`
      and the `cpumask` presence at `:341-342` stand at device load, so a
      raw address joins no sysfs read), the per-kind probe on the
      `probeDevice` pattern at `:258-285` through `pmuProbe` at `:667-730`,
      the one errno table at `:722-729`, the scope relabel through
      `scopeSettledState` at `:537-557`, and the off-Linux stub beside the
      reduced-catalog block at `:45-64`. No second errno table joins the
      path, and no host capability check joins it (D-3). Depends on T010.
      Verify: `grep -c -F 'synthesizeRawEntry' source/counters/linux_pmu/provider.cpp`
      answers at least 1 (the definition beside the probe it uses), and the
      same grep over `source/counters/detail/pmu.hpp` answers at least 1
      (the declaration);
      `git diff --name-only 0fce6651 -- test/source/counters_pmu_provider_test.cpp test/source/counters_linux_pmu_seam_test.cpp`
      prints nothing. Build nothing: T015 compiles this wave.

- [x] T012 [P] [US1] Read the table in the window: in
      `source/counters/linux_pmu/group_io.cpp`, `resolve` at `:153-198`
      takes the synthesized entry for a raw leaf over the device it already
      found, and `fillAttr` at `:200-225` stands untouched so the
      exclusion, group-read and time-pair settings arrive by construction
      with `words = {{0, configWord}}` (FR-003). Depends on T010.
      Verify: `git diff -U0 0fce6651 -- source/counters/linux_pmu/group_io.cpp | grep -c '^-.*fillAttr'`
      answers 0, and the same read for `exclude_kernel` answers 0. Build
      nothing: T015 compiles this wave.

- [x] T013 [P] [US1] Read the table in the plan: `compileCore` in
      `source/counters/plan.cpp` consults the raw-entry table where the tree
      scan stands at `:445-466`, and the standing gate at `:474` and
      `:668-692` and the provider grouping at `:502-537` run unchanged, so
      the compiled plan gains the raw leaf's record while the published
      listings keep their base answers (FR-015). Depends on T010.
      Verify: `git diff -U0 0fce6651 -- source/counters/plan.cpp | grep -c '^-.*providerIndex'`
      answers 0, which keeps the grouping of `:502-537` intact. Build
      nothing: T015 compiles this wave.

- [x] T014 [US1] [US2] Add the raw arm to `resolveLeafCore` in
      `source/counters/system.cpp` at `:573-590`: the shape test, the width
      gate at sixteen digits, the synthesized entry, and a return ahead of
      the `has no counter named` near-miss arm at `:582-589`, so near-miss
      candidates keep coming from seeded leaves. Store the table beside
      `probedKindTable` at `:165-177` with the fill-before-read lifecycle
      the probed-kinds comment states. Depends on T010 and T011.
      Verify: `git diff -U0 0fce6651 -- source/counters/system.cpp | grep -c '^-.*has no counter named'`
      answers 0, and `git diff -U0 0fce6651 -- source/counters/system.cpp | grep -c '^-.*probedKindTable'`
      answers 0. Build nothing: T015 compiles this wave.

- [x] T015 [US1] [US2] [US4] Land the counters unit's
      `add_test(NAME counters_raw_event_test COMMAND counters_raw_event_test)`
      line in T005's block, then turn the unit green, the implementation
      verify of T010 through T014: `cmake --build build -j 10`, then
      `ctest --test-dir build -R counters_raw_event_test --output-on-failure`,
      then the counters neighbors. Depends on T012, T013, T014 and T008:
      its `add_test` line edits the same `test/CMakeLists.txt`, so the
      file's four writers run in the order T005, T008, T015, T018.
      Verify: `cmake --build build -j 10` exits 0;
      `ctest --test-dir build -N -R counters_raw_event_test` lists exactly
      1 entry; that entry exits 0 with all twelve case names run;
      `ctest --test-dir build -R 'counters_pmu_provider_test|counters_linux_pmu_seam_test|counters_limit_lifecycle_test|counters_limit_fill_test' --output-on-failure`
      exits 0; `git diff --name-only 0fce6651 -- test/source` lists
      `counters_raw_event_test.cpp`, adds `harness_raw_events_test.cpp`
      once T007 has landed it, and lists no other path under
      `test/source`.

**Independent test**: the counters unit passes on a scripted kernel while
every pre-existing counters entry passes with an unedited expectation.

---

## Phase 5: Wave 5 - the harness arms (US1, US2, US3, FR-015)

**Order**: W5 follows W4 and W3. One file, so T017 follows T016.

- [x] T016 [US2] Add the three arms to `counterAddressFailure` in
      `source/harness/runner.cpp` at `:986-1030`, in the order FR-005 sets:
      the width check first through the harness predicate `hasRawCodeShape`
      of `source/harness/detail/hunt_selection.hpp:106-131` (read there,
      never edited), printing `raw event code exceeds 16 hex digits` inside
      the standing origin, address, message shape at `:1022-1026` at the
      standing status 2; then the device read; then the state read through
      the raw-aware `resolveLeafCore`, whose returned `LeafCore` carries the
      settled `avail` (`include/speedgun-ng/counters_measurement.hpp:103`)
      printed by the standing
      `listed as <state> (FR-023)` line at `:1012-1013` with the state words
      of `source/harness/report.cpp:32-39`. The harness opens nothing
      itself. Depends on T015 and T009.
      Verify: `grep -c -F 'raw event code exceeds 16 hex digits' source/harness/runner.cpp`
      answers 1;
      `git diff -U0 0fce6651 -- source/harness/runner.cpp | grep -c '^-.*listed as'`
      answers 0; `cmake --build build -j 10` exits 0.

- [x] T017 [US1] [US3] Add the column-build raw arm in
      `source/harness/runner.cpp` at `:1435-1502`: a raw-shaped leaf skips
      the catalog search, because the published listings carry no raw entry
      (FR-015), and reaches the same lookup through the typed route
      `object->counter<Dim<0, 1>>` of
      `include/speedgun-ng/counters_system.hpp:143-178`; the unit token
      `none` maps through `unitFromToken` and `dimensionOf` to events^1
      (`include/speedgun-ng/counters_core.hpp:162-184`, `:205-206`), so no
      `CatalogEntry` picks the dimension; the no-such-leaf line stays
      reserved for names that are not raw-shaped. Depends on T016 (one
      file).
      Verify: `cmake --build build -j 10` exits 0;
      `ctest --test-dir build -R harness_cli_test --output-on-failure` exits
      0 and `git diff --name-only 0fce6651 -- test/source/harness_cli_test.cpp`
      prints nothing.

- [x] T018 [US1] [US2] [US3] [US4] Land the harness unit's
      `add_test(NAME harness_raw_events_test COMMAND harness_raw_events_test)`
      line in T008's block, then turn the unit green and read the
      standing pins beside it:
      `ctest --test-dir build -R harness_raw_events_test --output-on-failure`,
      then the pinned neighbors, then the diff reads over the pinned files.
      Depends on T017.
      Verify: the entry exits 0 with all ten case names run;
      `ctest --test-dir build -R 'harness_cli_test|harness_hunt_cli_test|harness_json_schema_test|counters_raw_event_test' --output-on-failure`
      exits 0; `git diff --name-only 0fce6651 -- test/source/harness_cli_test.cpp test/source/harness_hunt_cli_test.cpp test/source/harness_json_schema_test.cpp docs/schema/report-v1.json source/harness/detail/hunt_selection.hpp`
      prints nothing, which carries SC-004 and FR-007's hunter half (the
      pin at `test/source/harness_hunt_cli_test.cpp:394-398`).

**Independent test**: the harness unit passes on a scripted kernel, and the
pinned refusal lines answer byte-for-byte with their test files unedited.

---

## Phase 6: Wave 6 - the docs carriers and the numbers (FR-010, FR-012)

**Order**: W6 opens no file the chain opens, so these four author tasks run
beside W2 through W5. Each opens one file, so all four carry [P].

- [x] T019 [P] [FR-010] Write the one counter option description in
      `source/harness/options.cpp` at `:560-565`: it names the raw
      acceptance (a leaf spelled `r` then hex digits: the hex text is the
      perf config word, the device part names the PMU) and keeps the
      standing `--hunt` refusal in the same row. One row, one change, and no
      other line of the option table moves.
      Verify: `grep -c 'raw event code' source/harness/options.cpp` answers
      at least 1; `git diff -U0 0fce6651 -- source/harness/options.cpp | grep -E '^@@'`
      names hunks covering lines 560 to 565 only; `sed -n '560p' source/harness/options.cpp`
      still opens with `{"counter"`.

- [x] T020 [P] [FR-010] Update the README quick row in place at
      `README.md:970`. The row stays one line and the page stays at or under
      the `PAGE_LINE_CAP = 1004` of `tools/docs/example_drift.py:256` (base
      1003). No new example region joins the page: D-8 states the reason, an
      example built on host config words cannot run on the CI matrix.
      Verify: `wc -l README.md` answers at most 1004;
      `sed -n '970p' README.md | grep -c 'raw event code'` answers 1;
      `git diff --name-only 0fce6651 -- README.md example/` lists
      `README.md` only.

- [x] T021 [P] [FR-010] Update the guide: the counter section of
      `docs/pages/harness.md` at `:637-651` gains the raw acceptance and one
      pointer to the standing hunter refusal, and the option table row at
      `:675` carries the same sentence. The hunter prose at `:2577-2579` and
      the hunter example line at `:2621` stand untouched, and the page keeps
      requirement ids and task ids out of its prose.
      Verify: `sed -n '637,651p;675p' docs/pages/harness.md | grep -c 'raw event code'`
      answers at least 1;
      `git diff -U0 0fce6651 -- docs/pages/harness.md | grep -E '^@@'` names
      no hunk covering lines 2577 to 2579 or line 2621.

- [x] T022 [P] [FR-012] Move the version block in `CMakeLists.txt`:
      `VERSION 0.35.0` at `:82`, with the reservation reading recorded in the
      comment above (the specs/048 note at `:78-81` is the wording
      precedent, and specs/029 FR-090 is the rule); counters `SOVERSION 8`
      at `:172` stays, with a comment naming the no-public-header condition
      in the style of the specs/048 note at `:453-455`; harness
      `SOVERSION 22` at `:456` stands untouched.
      Verify: `grep -n '^    VERSION 0.35.0' CMakeLists.txt`,
      `grep -n '^    SOVERSION 8' CMakeLists.txt` and
      `grep -n '^    SOVERSION 22' CMakeLists.txt` each answer exactly one
      line; `grep -n 'kSchemaVersion = "1.16"' source/harness/json_report.cpp`
      answers one line and that file stays out of the diff;
      `git diff --name-only 0fce6651 -- include/ docs/schema/` prints
      nothing.

- [x] T023 [R] [FR-012] [SC-006] Write `specs/051-raw-events/results/numbers.md`,
      the one file for the FR-012 readings: the version, the two SOVERSION
      readings and the schema constant, each beside the command that read it;
      the counters case named as the kept reading at 8 because no public
      header moved, with 9 recorded as the reading a header move would give;
      and the empty diff read over `include/` and `docs/schema/`. Depends on
      T022.
      Verify: `grep -c` over that file answers at least one hit for
      `0.35.0`, `SOVERSION 8`, `SOVERSION 22` and `1.16`;
      `git diff --name-only 0fce6651 -- include/` prints nothing at the read
      of this record.

**Independent test**: three texts and one version block carry the new
acceptance, and the number readings answer the plan's D-6 pair.

---

## Phase 7: Wave 7 - the gates, the acceptance run, the records, the lane (FR-013, FR-014)

**Order**: W7 follows every wave, and T042's commit stands first: every
W7 build or gate read runs at that committed built head. One gate per
task, each with its own exit status and figure. The shared `build` tree
carries one build at a time; the sanitizer, tsan and coverage legs stand
in their own trees and run [P].

- [x] T042 [O] [FR-013] [FR-014] Commit the lane work before the gates run, so
      every W7 verdict names a built head: `git add` the seven source files,
      the two new units, `test/CMakeLists.txt`, `CMakeLists.txt`, `README.md`,
      `docs/pages/harness.md` and the records written so far, then one commit
      in the house shape (why-body, one footer paragraph). Record the output
      of `git rev-parse HEAD` in `specs/051-raw-events/results/commits.md`
      as the built-head handle; no sha enters that file from memory.
      Depends on T018, T019, T020, T021 and T023; T024 through T037
      depend on T042.
      Verify: `git status --porcelain -- source test README.md docs CMakeLists.txt`
      prints nothing; `git log --oneline 0fce6651..HEAD | wc -l` answers
      at least 1; `git cat-file -t` on the sha recorded in `commits.md` answers
      `commit`.

- [x] T043 [FR-012] Version pin carriers, found by the T024 suite leg at
      a3c465ac: FR-012 moves the project figure to 0.35.0, and two standing
      version records pin it (`test/source/harness_cli_table_test.cpp`
      :129, :1505, :1521-1522 and `test/source/views_test.cpp` :308,
      :317-318, seven lines). A tree census over test, example, docs,
      README.md, tools, cmake, include and source names no other carrier.
      The move mirrors specs/048's `test: Move the version rows to 0.34.0`
      (68372d75), same files, same shape, mover clause included; SOVERSION
      rows stay unedited. Landed as b0cd97e2. T024 and the five cheap gate
      legs (T028 to T032) re-read at this new built head, and T025 to T027
      run against it.
      Verify: `grep -rn '0\.34\.0' test example docs README.md tools cmake
      include source` prints nothing at the read of this task.

- [x] T044 [FR-013] Coverage closure for the T027 red at b0cd97e2 (line
      arm 13 dark rows, branch arm 37 against the constant allowance
      of 21). The lane drove its own 15 dark branch outcomes and 13
      lines to zero, with no marker anywhere (the cap of 84 stands at
      84): five new counters scenarios (scope and errno arms of
      rawStateRule at provider.cpp:777/784/796/818/820/845 as the trace
      names them, the syscall read mode, one upper-case hex spelling, the
      no-PMU-path refusal over the System-seeded machine node, whose
      bare-System step drives the null settledDeviceState arm because
      the System constructor publishes the machine node itself at
      system.cpp:197-206), two new harness scenarios (machine/r4000 on
      the CLI route, and the library route through the runner API where
      the checkpoint never runs, covering runner.cpp:1020/1021 and the
      raw refusal arm at :1515-1527, which stays in the source because
      a library caller skips the checkpoint, so the arm is live FR-023
      and FR-033 behavior and an assert there would be release-build
      UB), and two transforms in system.cpp (isHexDigit takes std::isxdigit; a
      companion assert over the stampRawRecord miss arm was reverted
      the same lane at da32efe9, because the counters case
      rawStampSkipsControlLeafNames proved that arm live: the provider
      drops entry-less devices at provider.cpp:1006, yet a device whose
      every entry the seed probe refused still registers, and a node
      whose only leaves carry the control names then drives the
      partition to its end, so the early return stands as behavior and
      that case covers its line and branch outcomes). Landed as
      bb45fa52 and 9033535e. One orchestrator
      instruction was withdrawn on cited reads: a ClockProvider was
      ordered into the bare-System step, and the worker objected with
      three reads (the System constructor seeds the machine node at
      system.cpp:197-206; registerProvider reads the node at :240;
      the standing comment at :341-350 states the machine find answers
      on every host), so the step stands as written and the corrected
      fact is this block. A second batch (commit named in
      results/commits.md) closed the last three rows: a scripted node
      whose only leaves carry the enabled and running control names
      drives both false arms of the stamp partition, since the
      provider appends its own pair of those names whenever any entry
      is countable (provider.cpp:315-339) and the tree refuses one
      object holding two leaves of one name (system.cpp:48-59), so a
      countable control device cannot register and the refused-word
      shape is the only one that lights both arms on every host; and a
      bare r plus an r-run with a non-hex tail state the catalog route
      for unshaped leaves. If the rerun still holds the 641 row at
      zero, that arm is the empty-tail entry the size gate ahead of it
      makes unreachable, and the honest note is the standing lcov
      exclusion class already in that file at :344-350 and :60-81,
      raised as a question rather than written into the source. T024, T025, T026,
      T027 and the five cheap legs re-read at the new built head named
      in results/commits.md; the standing 22 dark outcomes (seven of
      them the topdown family, host-skipped at entry 185 on this AMD
      host) ride the census in the T027 row.
      Verify: the rerun coverage target exits 0, or the shortfall at
      most 22 stands recorded with the per-file census for the owner
      ruling.

- [x] T024 [FR-013] Full suite at the lane head:
      `cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK`,
      `cmake --preset=ci-rocky`, `cmake --build build -j 10`,
      `ctest --test-dir build --output-on-failure --no-tests=error -j 10`.
      Depends on T018, T019, T020, T021, T022 and T042.
      Verify: the suite exits 0 with 0 failed entries and no `Not Run`
      entry; `ctest --test-dir build -N | tail -1` answers 192.

- [x] T025 [P] [FR-013] Address and undefined-behavior sanitizer leg:
      `CXX=clang++ cmake --preset=ci-sanitize`,
      `cmake --build build/sanitize -j 10`, then
      `ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 10`.
      Depends on T018, T022 and T042.
      Verify:
      `grep -m1 'CMAKE_CXX_COMPILER:FILEPATH' build/sanitize/CMakeCache.txt`
      names `/usr/bin/clang++`; ctest exits 0 with no `Not Run` entry; the
      log holds no `ERROR: AddressSanitizer`, no `runtime error:` line and no
      leak report.

- [x] T026 [P] [FR-013] Thread-sanitizer leg: `CXX=clang++ cmake --preset=ci-tsan`,
      `cmake --build build/tsan -j 10`, then
      `TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 10`.
      When `harness_context_barrier_test` is the one failure, re-read it
      alone with
      `ctest --test-dir build/tsan -R harness_context_barrier_test --output-on-failure`
      and record both reads: that entry flakes about one run in ten under a
      thread sanitizer on this host. Depends on T018, T022 and T042.
      Verify:
      `grep -m1 'CMAKE_CXX_COMPILER:FILEPATH' build/tsan/CMakeCache.txt`
      names `/usr/bin/clang++`; ctest exits 0, or the only failing entry is
      `harness_context_barrier_test`, the alone read exits 0, and
      `results/gates.md` carries both reads with the P2 exception clause
      Principle VIII frames; the log holds no `WARNING: ThreadSanitizer`
      line.

- [ ] T027 [P] [FR-013] [SC-005] Coverage gate through the tracked target:
      `cmake --preset=ci-coverage`, `cmake --build build/coverage -j 10`,
      `find build/coverage -name '*.gcda' -delete` so the trace names one
      run, `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error`,
      `cmake --build build/coverage -t coverage`. Depends on T018, T022 and T042.
      Verify: the summary of the gate prints `lines` at `100.0%` and the
      branch arm reports a shortfall of at most 21 rows with the recorded
      D-14 message of `tools/dbc/coverage_gate.sh:121-123`; the ctest log
      names no `Not Run` entry;
      `git grep -c LCOV_EXCL -- source include | awk -F: '{s+=$2} END {print s}'`
      answers at most 84 and
      `git diff -U0 0fce6651 -- source include | grep -c '^+.*LCOV_EXCL'`
      answers 0; `git diff --name-only 0fce6651 -- cmake/coverage.cmake tools/dbc/coverage_gate.sh`
      prints nothing, which keeps the capture at the tracked
      `--ignore-errors mismatch,negative` of `cmake/coverage.cmake:73` and
      keeps the `inconsistent` class absent; record the host note when
      `genhtml` exits 2 for its perl GD load fault after the verdict printed
      (`docs/pages/building.md` names that gap).

- [x] T028 [FR-013] Format gate: `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`.
      Depends on T018, T019, T020, T021, T022 and T042.
      Verify: exit 0, with `clang-format --version` reading
      `clang-format version 18.1.8`, the version the lint job pins at
      `.github/workflows/ci.yml:34`.

- [x] T029 [FR-013] Spell gate: `cmake -P cmake/spell.cmake`. Depends on
      T018, T019, T020, T021, T022 and T042.
      Verify: exit 0. A hit naming a path under a build directory is an
      artifact of the skip list in `.codespellrc` and stands recorded; a hit
      in a tracked file is a defect cleared before T039.

- [x] T030 [FR-013] Prose gate over the lane:
      `cmake -D PROSE_MODE=range -D PROSE_BASE=0fce6651 -P cmake/prose-lint.cmake`,
      then the tree read over the lane artifacts:
      `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events`.
      Depends on T018, T019, T020, T021, T022, T006, T009, T023 and T042.
      Verify: both exit 0 with 0 findings; Principle XI.6 puts a finding at
      lint parity, so it gets cleared before T039.

- [x] T031 [FR-013] Page-drift gate: `python3 tools/docs/example_drift.py .`.
      Depends on T018, T019, T020, T021 and T042.
      Verify: exit 0, which carries the option-row table read at
      `tools/docs/example_drift.py:567-581`, the page-line cap at `:256`, and
      the forbidden-class scan of the two User-guide pages; the ctest
      entries `example_drift`, `snippet_compile`, `example_run` and
      `report_shape` exit 0 in the T024 suite.

- [x] T032 [FR-013] Time-source gate of the counters-only constraint:
      `bash test/time_source_gate.sh .`. Depends on T018, T019, T020, T021 and T042.
      Verify: exit 0 with no `file:line` hit printed.

- [x] T033 [FR-013] [SC-001] Host acceptance pair on the built binary:
      `./build/example/docs_command_line --iterations=1 --counter=cpu/instructions --counter=cpu/rc0`,
      then the same run with `--json=/tmp/sg051.json`; then the four refusal
      reads of quickstart section 3:
      `./build/example/docs_command_line --counter=nosuch/r4000; echo "status $?"`,
      `./build/example/docs_command_line --counter=cpu/r12345678901234567; echo "status $?"`,
      `./build/example/docs_command_line --counter=nosuch/r12345678901234567; echo "status $?"`,
      `./build/example/docs_command_line --counter=cpu/r4000 --list; echo "status $?"`;
      then the disclosure reads of quickstart section 4 with
      `printf 'counter=cpu/rc0\n' > /tmp/sg051_opts.txt`, the run carrying
      `--options=/tmp/sg051_opts.txt --json=/tmp/sg051b.json`, the two
      greps for the option line and the `cpu/rc0=` column, and the python
      read of `schemaVersion`, `metricLabels` and each row's
      `metrics[].label`; then the listing read
      `./build/example/docs_command_line --catalog | wc -l`. Depends on T024,
      the read that built that binary and T042.
      Verify: the first run prints two columns labeled `cpu/instructions`
      and `cpu/rc0`, both with a positive figure for config word `0xc0`;
      the JSON read answers `1.16` and names `cpu/rc0` in `metricLabels` and
      in the row labels; the first three refusal reads exit 2 and print the
      no-object line, the width line and the width line; the listing read
      exits 0 and prints no address line; the `--catalog` figure equals the
      base figure of `results/base-readings.md`.

- [x] T034 [FR-013] Static-analysis leg, in the separate analyzer tree so
      the shared `build` keeps its ci-rocky configuration:
      `cmake --preset=ci-ubuntu -B build-tidy`,
      `cmake --build build-tidy --target clean`,
      `cmake --build build-tidy -j 10 2>&1 | tee /tmp/sg051-head-analyzer.log`.
      Depends on T003 (the base counts it compares against) and on T033 (the
      acceptance read wants the `ci-rocky` binary) and T042.
      Verify: the build exits 0, which the `WarningsAsErrors` of `.clang-tidy`
      gives for `readability-identifier-naming` alone;
      (the per-file-count wording below was amended at T044 after the
      head read, because the amended rule is what the analyzer tree
      itself enforces: a count above base is recorded with its
      per-check delta and is clean when every added check stands
      outside the `WarningsAsErrors` list, the two classes new to
      their own files measured at base across the tree at 446 hits
      for `misc-const-correctness` and 1460 for
      `readability-simplify-boolean-expr`;)
      `grep -cF '[readability-identifier-naming]' /tmp/sg051-head-analyzer.log`
      answers 0 (the bracketed form is the check-attributed
      read, and clang-tidy writes the suffix -warnings-as-errors
      inside those same brackets, so a finding shows under the
      plain grep while the literal form matches only a clean run;
      quote both reads); for each edited file the `grep -c ': warning:'` count is at
      or below the base count recorded in `results/base-readings.md`.

- [x] T035 [R] [FR-013] Write `specs/051-raw-events/results/host-acceptance.md`,
      the one file for the acceptance reading: family 26, model 68, vendor
      `AuthenticAMD`, the device type 4 and the alias text `event=0xc0`, the
      two column labels with both figures, the four refusal lines with their
      exit statuses, the carrier counts from the option-file run, the
      `--catalog` figure beside the base figure, and the note that the
      scripted units pin type 4 and word `0xc0` (SC-001, FR-013). Depends on
      T033 and T042.
      Verify: `grep -c 'family 26'`, `grep -c 'model 68'` and
      `grep -c '0xc0'` each answer at least one hit in that file;
      `test -s specs/051-raw-events/results/host-acceptance.md` exits 0.

- [x] T036 [R] [FR-013] Write `specs/051-raw-events/results/gates.md`, the one
      file for the FR-013 gate readings: one row per leg with the command,
      the exit status and the figure, naming the T024 suite count, the T027
      coverage figures (lines, branch shortfall, marker count, suppression
      classes), the T025 and T026 sanitizer verdicts with their compilers,
      the T034 analyzer counts beside the base counts, the fast reads of
      T028 to T032,
      and the head each read ran at. Depends on T024, T025, T026, T027,
      T028, T029, T030, T031, T032, T033, T034 and T042.
      Verify: `grep -c` over that file answers at least one hit for `100.0`,
      `21`, `84`, `clang++`; `test -s` exits 0.

- [x] T037 [R] [FR-014] Scope read over the lane diff, Principle X.3:
      `git diff --name-only 0fce6651` lists the seven source files (five
      under `source/counters/`, two under `source/harness/`), the two new
      test units, `test/CMakeLists.txt`, `CMakeLists.txt`, `README.md`,
      `docs/pages/harness.md` and files under `specs/051-raw-events/`, and
      nothing else. Depends on T018, T019, T020, T021, T022, T023 and T042.
      Verify:
      `git diff --name-only 0fce6651 | grep -c -E '^(\.github/|include/|docs/schema/|example/)'`
      answers 0;
      `git diff --name-only 0fce6651 -- source/harness/detail/hunt_selection.hpp source/harness/hunt_cli.cpp source/harness/hunt.cpp source/harness/report.cpp source/harness/json_report.cpp source/harness/json_load.cpp source/harness/catalog.cpp source/counters/linux_pmu/metric_formula.cpp`
      prints nothing; `git diff --name-only 0fce6651 -- test/source` lists
      exactly the two new units and the two version pin carriers of
      T043 (`test/source/harness_cli_table_test.cpp`,
      `test/source/views_test.cpp`).

- [x] T038 [R] [FR-014] Write `specs/051-raw-events/results/pr-body.md`, the
      text of the pull-request description: the host figures, the gate
      readings as one row per leg with the exit status, the coverage figures,
      the sanitizer compilers, the numbers of SC-006, the two-column
      acceptance figures, and the stacking note. The note states that this
      branch stands on `0fce6651`, the head of the open pull request for
      specs/049, so the pull request shows both commits until the owner lands
      that one, the arrangement the spec's Assumptions record. The committed
      copy names no sha: the sha joins when T041 posts, read from
      `git rev-parse HEAD`. Depends on T035 and T036.
      Verify: `test -s specs/051-raw-events/results/pr-body.md` exits 0;
      `grep -c -E '^[0-9a-f]{40}$' specs/051-raw-events/results/pr-body.md`
      answers 0 in the committed copy.

- [x] T039 [O] [FR-014] Squash the lane (the T042 commit plus the record
      files written since) to one commit with the Pull Request Quality template: title `Harness: Add raw event-code counter columns`,
      a why-body wrapped at 72 columns naming G-31 and the routing through
      the one core lookup, one footer paragraph carrying
      `Refs: specs/051-raw-events` then `Approved-by: OmO`, author and committer
      `OmO <omo@speedgun-ng.local>`. No `Fixes #` line: the lane closes no
      issue. Depends on T036, T037, T038.
      Verify: `git log --oneline 0fce6651..HEAD | wc -l` answers 1;
      `python3 tools/prose/prose_gate.py --check commit --base HEAD~1`
      answers 0 findings; `git log -1 --format='%an <%ae> %cn <%ce>'` names
      the OmO identity for author and committer.

- [x] T040 [O] [FR-013] Re-read the cheap verdicts at the squashed head and
      record the tree equality in `specs/051-raw-events/results/gates.md`:
      the marker read, the four number greps of T022, the two-column
      acceptance run of T033, `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`,
      `cmake -P cmake/spell.cmake`, `python3 tools/docs/example_drift.py .`,
      `bash test/time_source_gate.sh .`,
      `python3 tools/prose/prose_gate.py --check prose --mode tree` exits 0,
      and `git diff --name-only <built-head> HEAD` where `<built-head>` is the
      T042 commit, the sha read from
      `specs/051-raw-events/results/commits.md` at the read of this record.
      Depends on T039.
      Verify: the last read lists only paths under
      `specs/051-raw-events/`, which is the reading that carries every T024
      to T034 verdict to the squashed head (Principle VIII: a recorded
      verdict names the head it ran at); each fast read exits 0; the two
      columns print positive figures again at that head; after the record
      lands, `git add specs/051-raw-events/results/gates.md` and
      `git commit --amend --no-edit`, and
      `git log --oneline 0fce6651..HEAD | wc -l` still answers 1.

- [ ] T041 [O] [FR-014] Push and open the pull request, and stop there.
      `git push -u origin 051-raw-events`; a later push uses
      `git push --force-with-lease=051-raw-events:<expected-sha>`. Build the
      posted body from `results/pr-body.md` with one leading `Head:` line
      whose sha comes from `git rev-parse HEAD`, into a file outside the
      tree, then
      `gh pr create --base master --head 051-raw-events --title "Harness: Add raw event-code counter columns" --body-file <that file>`.
      Never merge; close no issue; no commit follows the T040 amend.
      Depends on T040.
      Verify: `git ls-remote origin refs/heads/051-raw-events` equals
      `git rev-parse HEAD`; `gh pr list --state open --head 051-raw-events --json number`
      shows exactly one entry; `gh pr view 051-raw-events --json state,mergedAt`
      answers `OPEN` with `mergedAt` null; the sha in the posted body equals
      `git rev-parse HEAD`; and once the CI coverage job of that pull
      request has finished, read its union number from the job log or
      its `coverage-info` artifact and append the real coverage row to
      `results/gates.md`, superseding the projected union row: the
      projection stands only until that run exists, and a union above
      21 reopens the packet with the union as the evidence.

---

## FR to task map

| Requirement | Tasks | Named check |
| --- | --- | --- |
| FR-001 | T004, T010, T014 | `rawAcceptRecordsTypeAndConfigWord` |
| FR-002 | T004, T007, T011, T016 | `absentDeviceRefusesWithoutProbe`, `unknownDeviceRawLine` |
| FR-003 | T004, T012 | `rawAttrMatchesCatalogRowAttr` |
| FR-004 | T004, T007, T011 | `rawProbeErrnoArms`, `rawProbeScopeRelabel`, `refusedProbeListedAsLines`, `probeRunsOncePerAddress` |
| FR-005 | T004, T007, T010, T014, T016 | `wideLeafNeverProbes`, `wideLeafWidthLine`, `wideLeafBeatsAbsentDevice` |
| FR-006 | T007, T017, T033 | `rawColumnDisclosesEverywhere` and the host carrier reads |
| FR-007 | T004, T014, T017, T018 | `typedRouteUnitNoneEventsOne`, `formulaResolvesRawEventName`, the untouched pin at `test/source/harness_hunt_cli_test.cpp:394-398` |
| FR-008 | T004, T007 | `duplicateRawAddressTwoColumns`, `duplicateSpellsDistinctAddresses` |
| FR-009 | T007, T018 | `limitLineCountsRawColumn` |
| FR-010 | T019, T020, T021, T031 | the three carrier greps plus the drift gate |
| FR-011 | T004, T007, T015, T018 | every case of the two new units |
| FR-012 | T022, T023 | the version and SOVERSION greps and `results/numbers.md` |
| FR-013 | T042, T024 to T036, T040 | `results/gates.md`, `results/host-acceptance.md` |
| FR-014 | T042, T037, T038, T039, T041 | one squashed commit, the remote read, one open pull request |
| FR-015 | T004, T010, T012, T013, T014, T015, T017 | `listingsNeverReadRawTable`, `scriptedPlanCompilesAndMeasuresRawLeaf` |
| Base readings | T001, T002, T003 | the base figures every head read diffs against |
| Red units | T004 to T009 | `results/red-units.md`, direct binary runs |
| US1 | T004, T005, T006, T007, T010, T011, T012, T013, T014, T015, T017, T018 | `rawRunConsoleAndJson`; the host pair lives in SC-001 |
| US2 | T004, T005, T006, T007, T008, T009, T014, T015, T016, T018 | the four refusal lines, scripted |
| US3 | T007, T008, T009, T017, T018 | `rawColumnDisclosesEverywhere` |
| US4 | T015, T018 | the untouched pins of `harness_cli_test` and `harness_hunt_cli_test` |
| SC-001 | T004, T033, T035 | the scripted pin and the host pair |
| SC-002 | T004, T007, T033 | the four refusal lines |
| SC-003 | T007, T033 | the four carriers and the schema read |
| SC-004 | T015, T018, T024, T037 | unedited test files, green pins |
| SC-005 | T027, T036 | 100.0 lines, shortfall at most 21, at most 84 markers |
| SC-006 | T022, T023 | 0.35.0, 1.16, 22, 8 |

## Dependencies and execution order

- T001 to T003: W1, in order (one record file).
- T004 to T006: W2, in order (author, build block, red read); T005
  follows T003 so the shared tree stays single-writer.
- T007 to T009: W4 beside W2 and W3. T008 follows T005 because one file
  carries both registrations, and T009 follows T006 because one record
  carries both red reads. T009 stands before T016, which opens that file.
- T010 to T015: W3 after T006. T011, T012 and T013 run beside each other on
  disjoint files. T014 follows T010 and T011. T015 follows T012, T013,
  T014 and T008 (fourth writer of the shared registration file).
- T016 to T018: W5 after T015 and T009. One file, so T017 follows T016.
- T019 to T023: W6 beside W2 to W5. T019 to T022 open four disjoint files.
  T023 follows T022.
- T042 follows T018, T019, T020, T021 and T023: it commits the lane
  work; T024 through T037 all depend on it, and its `commits.md` sha is
  the built-head handle T040 reads.
- T042 then T024 to T041: W7 after every wave. T024 owns the shared
  `build` tree; T034 reads the analyzers in the separate `build-tidy`
  tree. T025, T026 and T027 own their trees and run beside each
  other. T028 to T032 need no build tree. T033 follows T024 and precedes
  T034. T035 and T036 collect the records, T037 reads the scope, and T038
  writes the description text. T039 squashes, T040 re-reads at the squashed
  head, T041 pushes and opens the pull request.

## Parallel execution, by wave

```text
W1: T001 -> T002 -> T003                       (one record file)
W2: T004 -> T005 (after T003, shared tree) -> T006
W4: T007 (beside W2 and W3) -> T008 (after T005, tree build/raw-red) ->
    T009 (after T006, tree build/raw-red)
W3: T010 -> { T011 | T012 | T013 } -> T014 -> T015 (after T008 too)
W5: T016 -> T017 -> T018
W6: { T019 | T020 | T021 | T022 } -> T023      (four disjoint files)
W7: T042 (commit) -> T024; { T025 | T026 | T027 };
    T028; T029; T030; T031; T032;
    T033 -> T034; T035; T036; T037; T038; T039 -> T040 -> T041
```

Two [P] tasks never open one file, and no [P] task shares a build tree with
another [P] task.

## Implementation strategy

The minimum shippable lane is W1 through W5: the base readings, the two red
units, the five counters files, the two harness arms, and the two green
reads T015 and T018. That increment carries US1, US2, US4 and the scripted
half of US3, and each wave closes behind its own binary checkpoint. W6 adds
the disclosure texts and the version, and W7 clears the gate set, the host
acceptance pair, the records and the lane. T039 leaves one commit on the
branch; T040 folds its record rows into that same commit by amend, and no
task adds a commit after T039.

## Notes

- The standing refusal texts, the hunter line and the pinned lines of
  `test/source/harness_cli_test.cpp:729-755` are quotations and stay
  byte-for-byte; XI.6 exempts a pinned quotation from prose style.
- Every `results/` record names the head it read, per the read-at-base rule
  above, so a later correction states the paths that already differ.
