# Tasks: Hardware Latency Probe with SMI Attribution

**Input**: Design documents from `/specs/048-hwlat-probe/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
pair stands written and observed failing before the code task that
turns it green. FR-035 keeps a test and its code in one commit, so each
implementation task names the ctest run that turns green as its verify.

**Organization**: tasks stand grouped by user story behind the module
order plan.md fixes (M1 to M10) and its four waves (W1 to W4). Each
phase names the module it leads and the wave it lands in; a phase that
spans two waves says so in its title. Workers check each unit with
`g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export <file>`
and add `-DSG_SCHEMA_PATH='"docs/schema/report-v1.json"'` for the
schema unit. The integrator owns every `cmake --build` and `ctest` run,
records each red before the green, and returns the failing output to
the owning worker.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel; no other task of the same wave names a
  file this task names
- **[Story]**: the story the task serves (`US1` to `US6`), in the story
  phases alone
- Every description names the exact file path, the requirement ids it
  closes, and the verify command

A file that carries several tasks lists them one owner at a time with
no `[P]`, the way plan.md names `source/harness/cli.cpp` the
integrator's file.

---

## Phase 1: Setup and Foundational - the public surface and the test executables (module M2)

**Purpose**: the types every story stands on, and the executables the
later phases reach red in. No module builds before this phase closes.

- [X] T001 Open `harness_latency_counters_test` in `test/CMakeLists.txt`
  beside the standing harness units: the `add_executable` line, the
  harness link, the `source/harness` include, the `cxx_std_23` line and
  the `add_test` line, and stand
  `test/source/harness_latency_counters_test.cpp` with the standing
  `check` and `fail` scaffold and a `main` that prints the failure count
  (FR-032, FR-034, III). Verify: `ctest --test-dir build -N -R '^harness_latency_counters_test$'` exits 0 with one entry listed
- [X] T002 Open `harness_hwlat_tracer_test` in `test/CMakeLists.txt` the
  same way, and stand `test/source/harness_hwlat_tracer_test.cpp` with
  the same scaffold (FR-032, FR-034, III). Verify: `ctest --test-dir build -N -R '^harness_hwlat_tracer_test$'` exits 0 with one entry listed
- [X] T003 [P] Add the surface in `include/speedgun-ng/benchmark.hpp`:
  `JitterMethod::HWLAT` and `TracerEngine::HWLAT`, each last in its
  list; on `JitterStats` the fields `smiDelta`, `nmiDelta` and
  `smiGapCount` after `droppedGaps`; on `PrecheckTracerRecord` the
  fields `windowUs`, `widthUs` and `thresholdUs`; on
  `PrecheckTracerCpuRecord` the fields `maxGapNs`, `gapCount` and
  `nmiCount` after `hardware`; the counters hold no value when their
  source answers nothing, and every member carries its doxygen contract
  (FR-002, FR-018, FR-025, FR-027, D-1, D-2, D-13, P-5, P-7). Verify:
  `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export -x c++ include/speedgun-ng/benchmark.hpp` exits 0 and `grep -c "HWLAT" include/speedgun-ng/benchmark.hpp` counts two lines

**Checkpoint**: the surface stands, and the two executables configure.
The probe executable joins Phase 3, whose rows reach red in wave W3.

---

## Phase 2: Foundational - the two counter sources (module M1, wave W2)

**Purpose**: one unit answers both kernel counter reads, so no other
unit opens a kernel interface directly (P-1).

- [X] T004 Write the failing register rows in
  `test/source/harness_latency_counters_test.cpp` over fake trees: one
  cpu reads `MSR_SMI_COUNT` at address `0x34` from `dev/cpu/<cpu>/msr`
  as one 8-byte little-endian unsigned value; an absent node, a
  directory, a short read and a node without rights each leave the
  count absent; the node stands opened read-only and never for write
  (FR-001, FR-007, FR-011, D-3, P-1). Verify: `ctest --test-dir build -R harness_latency_counters_test --output-on-failure` exits non-zero (red observed)
- [X] T005 Write the failing vendor and `NMI` rows in the same file:
  the gate reads the `vendor_id` line of `proc/cpuinfo` under the file
  root, answers supported for `GenuineIntel` and unsupported for any
  other text and for an unreadable file; the reader takes one cpu's
  column of the `NMI` line of `proc/interrupts`, answers absent when the
  line stands absent, and leaves the standing `interrupts` sum named
  apart (FR-009, FR-010, FR-011, D-4, P-4). Verify: `ctest --test-dir build -R harness_latency_counters_test --output-on-failure` exits non-zero (red observed)
- [X] T006 Declare the unit in
  `source/harness/detail/latency_counters.hpp`: the vendor gate, the
  descriptor pair for the register node, the count read and the `NMI`
  text reader, each with its doxygen `\pre`, `\post` or
  `\invariant` (FR-001, FR-007, FR-009, FR-010, II, P-1). Verify:
  `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export -x c++ source/harness/detail/latency_counters.hpp` exits 0
- [X] T007 Implement the register read in
  `source/harness/latency_counters.cpp`: one read-only `::open` under
  the file root, one `::pread` of 8 bytes into a stack buffer at `0x34`,
  the close, the absent answer for each fault T004 plants, and no write
  call anywhere (FR-007, FR-011, D-3, P-2) (depends on T004, T006; the
  rows of T004 turn green). Verify: `ctest --test-dir build -R harness_latency_counters_test --output-on-failure` exits 0
- [X] T008 Implement the gate and the text reader in
  `source/harness/latency_counters.cpp`: a file-local reader over
  `procRootPath` reads the two files, the shape `noise_tracer.cpp` sets,
  since `readUnderRoot` stands undeclared in every header,
  `std::from_chars` settles each figure, and a truncated line
  answers absent (FR-009, FR-010, FR-011, P-1, P-4) (depends on T005,
  T007; the rows of T005 turn green). Verify: `ctest --test-dir build -R harness_latency_counters_test --output-on-failure` exits 0
- [X] T009 Hold the two refusals in
  `source/harness/latency_counters.cpp`: no module load, no module
  parameter, no second counter, and no substitute named for a part that
  states none (FR-008, FR-010, FR-024, D-4, X.2). Verify: `grep -rnE "modprobe|request_module|insmod" source/harness/latency_counters.cpp source/harness/detail/latency_counters.hpp` reads no match and `grep -c "GenuineIntel" source/harness/latency_counters.cpp` counts at least one line
- [X] T010 Pair the contracts of the new unit in
  `source/harness/detail/latency_counters.hpp` and
  `source/harness/latency_counters.cpp`: one enforced kind per
  documented kind, and `[[maybe_unused]]` on a contract-only helper
  (FR-038, II, X.2). Verify: `grep -c "SG_" source/harness/latency_counters.cpp` counts at least two lines and the DBC pair gate of Phase 11 reads 0 gaps

**Checkpoint**: the sources stand green on fake files, and no path
writes a kernel node.

---

## Phase 3: User Story 1 - a gap names the SMI that stole the cpu (Priority: P1) (module M3, wave W3)

**Goal**: the probe reads the SMI counter once per kept gap and once at
each window edge, carries the change of the span that ends at each gap,
marks the gap `SMI` or `NOT_SMI`, and folds the two window deltas and
the marked count onto that cpu's figures. The method word that opens
the path stands in Phase 6, whose tasks land in wave W2 ahead of this
phase.

**Independent Test**: over a root with a fake `msr` node and a fake
`proc/interrupts`, a counter that rises between the two reads that
bound one gap marks that gap `SMI`, a flat counter marks every gap
`NOT_SMI`, and a missing node marks no gap and keeps status 0.

### Tests for User Story 1 (write first, observe failure)

- [X] T011 [US1] Open `harness_hwlat_probe_test` in `test/CMakeLists.txt` the
  same way, and stand `test/source/harness_hwlat_probe_test.cpp` with
  the same scaffold (FR-032, FR-034, III). Verify: `ctest --test-dir build -N -R '^harness_hwlat_probe_test$'` exits 0 with one entry listed
- [X] T012 [US1] Write the failing gap-mark rows in
  `test/source/harness_hwlat_probe_test.cpp`: a node that rises by one
  between the two reads that bound one gap marks that gap `SMI`, a flat
  node marks it `NOT_SMI`, the first gap answers the opening edge read,
  and index `i` of the change ring answers index `i` of the gap ring
  (FR-012, FR-013, FR-014, D-7, P-3, SC-001, SC-008). Verify: `ctest --test-dir build -R harness_hwlat_probe_test --output-on-failure` exits non-zero (red observed)
- [X] T013 [US1] Write the failing per-cpu figure rows in the same
  file: `smiDelta` answers the change of the two edge reads of that
  cpu's window, `nmiDelta` answers the change of the same window,
  `smiGapCount` counts the kept gaps whose span carries a change above
  zero, and the window stands the pre-check window in the pre-check
  phase and the probe's own run span in the alongside phase (FR-016,
  FR-017, P-5). Verify: `ctest --test-dir build -R harness_hwlat_probe_test --output-on-failure` exits non-zero (red observed)
- [X] T014 [US1] Write the failing not-available rows in the same file:
  an absent node, a node without rights, a short read, a part that
  states no documented counter and a `/proc/interrupts` read with no
  `NMI` line each leave the marks off every gap, keep the standing figures,
  print the standing wording once per source for the run and keep status
  0; one cpu keeps its own figures while its neighbor answers absent
  (FR-011, FR-015, FR-017, D-14, P-12, SC-001). Verify: `ctest --test-dir build -R harness_hwlat_probe_test --output-on-failure` exits non-zero (red observed)
- [X] T015 [US1] Write the failing buffer and edge rows in the same
  file: a full buffer drops record and change in one step and raises
  `droppedGaps` once, the marked count counts kept gaps only, a gap at
  the last instant carries the change of the span that ends at it while
  the window delta keeps the change of the two edge reads, and a gap the
  probe's own read opens answers `NOT_SMI` (FR-012, FR-013, FR-014,
  D-7, E-03). Verify: `ctest --test-dir build -R harness_hwlat_probe_test --output-on-failure` exits non-zero (red observed)
- [X] T016 [P] [US1] Extend the cost proof in
  `test/source/harness_jitter_probe_alloc_test.cpp`: the no-allocation
  executable keeps its exit 0 with the register read on the gap path,
  and the kept gap record keeps `static_assert(sizeof(GapRecord) == 16)`
  with its two standing fields (FR-012, VII, P-2, plan R-1). Verify: `ctest --test-dir build -R harness_jitter_probe_alloc_test --output-on-failure` exits 0, and a planted allocation on the gap path turns the same run non-zero

### Implementation for User Story 1

- [X] T017 [P] [US1] Add the change ring in
  `source/harness/detail/jitter_probe.hpp`: one
  `std::vector<std::uint64_t>` per probe cpu beside the gap ring, one
  capacity and one drop counter shared with it, the vector pre-sized to
  the run's `gapCount`, and the latch snapshot carries the change
  prefix beside the gap prefix (FR-012, FR-013, D-7, P-3, E-03). Verify:
  `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export -x c++ source/harness/detail/jitter_probe.hpp` exits 0 and `grep -c "sizeof(GapRecord) == 16" source/harness/detail/jitter_probe.hpp` counts one line
- [X] T018 [US1] Add the reads in `source/harness/jitter_probe.cpp`:
  open the node once at start through the unit of Phase 2, read once
  after a gap passes the threshold and outside the measured interval,
  read the two window edges, close at stop, and store record and change
  in the one keep-or-drop step (FR-012, FR-013, P-2, plan R-1) (depends on
  T012, T015, T017; the rows of T012 stay red until T019 folds them).
  Verify: `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export source/harness/jitter_probe.cpp` exits 0
- [X] T019 [US1] Fold the attribution in `source/harness/jitter_probe.cpp`:
  a change above zero marks the gap `SMI`, a change of zero marks it
  `NOT_SMI`, an absent source leaves every gap unmarked and answers
  `smiGapCount` 0, the two deltas answer the edge pairs, and the
  disclosure names each source once for the run (FR-013 to FR-017, D-7,
  D-14, P-5, P-12) (depends on T013, T014, T018; the rows of T012 to
  T015 turn green). Verify: `ctest --test-dir build -R harness_hwlat_probe_test --output-on-failure` exits 0
- [X] T020 [US1] Hold the loop cost in
  `source/harness/jitter_probe.cpp`: no read stands on the path of a gap
  below the threshold, no second seam enters the loop, and no background
  sampler reads the counter (FR-012, FR-013, VII, X.2) (depends on T016;
  the cost proof stands green). Verify: `ctest --test-dir build -R harness_jitter_probe_alloc_test --output-on-failure` exits 0 and `grep -c "pread" source/harness/jitter_probe.cpp` counts at most two lines

**Checkpoint**: the marks, the two deltas and the marked count answer
the fake counter shapes, the not-available paths keep status 0, and the
gap record holds its 16 bytes.

---

## Phase 4: User Story 2 - the kernel detector runs for the run (Priority: P2) (module M4 and M8, wave W2 and wave W3)

**Goal**: the engine word `hwlat` runs the kernel's detector across
the pre-check window: the setup saves the six standing values
`current_tracer`, `tracing_cpumask`, `hwlat_detector/mode`,
`hwlat_detector/width`, `hwlat_detector/window` and `tracing_thresh`,
writes seven files in the order the requirement states, folds three
figures from each tracer cpu's buffer text, and the restore writes
each saved value back in the reverse of the write order. The wave W3
half adds the six comparison names.

**Independent Test**: over a fake tracefs tree the run holds the engine
word during the window and leaves every saved file at the value the tree
held before the run on the normal end, a failed write and the `SIGINT`
path.

### Tests for User Story 2 (write first, observe failure)

- [X] T021 [US2] Write the failing setup-order rows in
  `test/source/harness_hwlat_tracer_test.cpp`: the setup reads the
  standing text of each of the six saved files first, then writes
  `tracing_cpumask`, then `hwlat_detector/mode` with `per-cpu`, then
  `hwlat_detector/width` with `5000`, then `hwlat_detector/window` with
  `10000`, then `tracing_thresh` with `1000000`, the three constants spec
  A-3 names, then `current_tracer`
  with `hwlat`, then `tracing_on` with `1`, each path under the file
  root, the mask in the word width the standing mask carried (FR-018,
  FR-019, FR-020, D-9, D-10, P-14, A-3). Verify: `ctest --test-dir build -R harness_hwlat_tracer_test --output-on-failure` exits non-zero (red observed)
- [X] T022 [US2] Write the failing restore rows in the same file:
  the restore writes `tracing_on` at `0`, then each saved value in the
  reverse of the write order, so `current_tracer` stands before
  `tracing_thresh` and the kernel's own reset runs ahead of the host
  threshold; the six saved files stand their pre-run values; a setup
  that fails after one write restores each file it wrote before it
  answers the empty session, and the two standing engines gain the same
  fix; a failed restore write answers one line naming the file and the
  saved value and the run keeps its status (FR-003, FR-021, FR-022,
  D-8, D-12, D-15, P-8, P-9, SC-007). Verify: `ctest --test-dir build -R harness_hwlat_tracer_test --output-on-failure` exits non-zero (red observed)
- [X] T023 [US2] Write the failing figure rows in the same file over
  the fake text at `sys/kernel/tracing/per_cpu/cpu<id>/trace`: a line
  counts only when `#` stands at column 1 and a digit at column 2, so a
  header legend line stands skipped, `maxGapNs` takes the larger of the
  two microsecond figures times 1000, `gapCount` counts the lines of the
  cpu, `nmiCount` sums the `nmi-count` fields and answers 0 where lines
  stand and none carries one, and a text with no line answers all three
  figures absent (FR-018, FR-027, A-6, P-10, plan R-2). Verify: `ctest --test-dir build -R harness_hwlat_tracer_test --output-on-failure` exits non-zero (red observed)
- [X] T024 [US2] Write the failing path rows in the same file: no
  tracefs at the standing path, a knob without rights, a write that
  fails inside the window and a `SIGINT` sent to a forked child each
  reach the restore list and leave the user-space figures whole at
  status 0 (FR-003, FR-021, FR-023, D-15, plan R-5, plan R-6). Verify: `ctest --test-dir build -R harness_hwlat_tracer_test --output-on-failure` exits non-zero (red observed)
- [X] T025 [P] [US4] Write the failing engine-word rows in
  `test/source/harness_noise_test.cpp`: `hwlat` settles
  `sg::TracerEngine::HWLAT`, the refusal reads `--os-noise: the engine
  'hwlato' is not osnoise, timerlat or hwlat`, and a `--os-noise`
  value on a batch with no `--jitter-probe` option and on a mode that
  runs no pre-check window each answer one line at status 2 (FR-003,
  FR-005, FR-018, D-2, D-6). Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T026 [P] [US2] Write the failing comparison rows in
  `test/source/harness_compare_test.cpp`: `smiDelta`, `nmiDelta` and
  `smiGapCount` resolve at indices 32, 33 and 34 of a table of 35 rows
  and read the carrier the standing jitter names read, every standing
  name keeps its index, and `hwlat:any:maxGapNs` reads the tracer record
  with the standing direction and threshold suffix (FR-004, FR-028,
  P-11, E-08). Verify: `ctest --test-dir build -R harness_compare_test --output-on-failure` exits non-zero (red observed)
- [X] T027 [P] [US2] Write the failing comparison refusal rows in
  `test/source/harness_compare_cli_test.cpp`: an unknown jitter field
  reads `names none of the thirty-five jitter fields`, an unknown
  detector figure reads `names none of the three hwlat figures`, each
  fault prints one line at status 2 before any comparison runs, a figure
  the record carries as absent reads 0, a `hwlat:` name on a report that
  carries no tracer record prints one line at status 2, and one on a
  record with no entry for the cpu the rule names prints that same line,
  and the standing refusals hold word for word over the grown table
  (FR-028, P-11, plan R-7, SC-001).
  Verify: `ctest --test-dir build -R harness_compare_cli_test --output-on-failure` exits non-zero (red observed)

- [X] T089 [US2] Write the failing standing-tracer rows in
  `test/source/harness_hwlat_tracer_test.cpp`: a tree whose
  `current_tracer` text already reads `hwlat` before the setup leaves the
  session writing no file at all, the tracer block printing the line
  `tracer: hwlat already stands as the current tracer` ahead of the
  standing `tracer: not available` line, the user-space figures whole and
  the exit status 0
  (FR-019, FR-023, P-15). Verify: `ctest --test-dir build -R harness_hwlat_tracer_test --output-on-failure` exits non-zero (red observed)

### Implementation for User Story 2

- [X] T028 [P] [US2] Add the saved set in
  `source/harness/detail/noise_tracer.hpp`: a pair type with `path` and
  `value`, a `std::vector<TracerSetting>` beside the standing members of
  `NoiseTracerSession` holding six pairs in write order, and the restore
  result list, each with its doxygen contract (FR-019, FR-021, FR-022,
  D-8, P-8, E-07, II). Verify: `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export -x c++ source/harness/detail/noise_tracer.hpp` exits 0
- [X] T029 [US2] Add the engine word and the setup order in
  `source/harness/noise_tracer.cpp`: `engineWordOf` gains its third
  branch, the setup reads the six standing texts before its first write,
  the saved `hwlat_detector/mode` value holds the bracketed
  word alone, so `none [round-robin] per-cpu` saves `round-robin`, and the
  seven writes follow the order T021 states with the width ahead of the
  window (FR-019, FR-003, FR-018, FR-020, D-9, P-14, P-16, plan R-3)
  (depends on T021, T025, T028; the rows of T021 stay red until T031).
  Verify: `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export source/harness/noise_tracer.cpp` exits 0
- [X] T030 [US2] Add the figure read in `source/harness/noise_tracer.cpp`:
  one entry per tracer cpu in session order, the parse rule that needs a
  digit second, three optional figures, and no stored buffer of the
  run's own (FR-018, FR-027, A-6, P-10) (depends on T023, T029).
  Verify: `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export source/harness/noise_tracer.cpp` exits 0
- [X] T031 [US2] Rewrite the restore in `source/harness/noise_tracer.cpp`:
  the mode value reads back the one saved word,
  `tracing_on` at `0` first, then the saved vector in reverse, the
  restore write compares the one stripped word, the
  return of the paths whose write failed, and the restore of what a
  partial setup wrote ahead of the empty session for all three engines
  (FR-003, FR-021, FR-022, D-8, D-12, D-15, P-8, P-9, P-16) (depends on T022,
  T024, T030, T089; the rows of T021 to T025 and T089 turn green). Verify: `ctest --test-dir build -R "harness_hwlat_tracer_test|harness_noise_test" --output-on-failure` exits 0
- [X] T032 [P] [US2] Grow the tables in `source/harness/compare.cpp`:
  three rows join `kJitterFields` at indices 32, 33 and 34, the refusal
  line moves its count word to `thirty-five`, a `kHwlatFields` table of
  three rows stands behind the head word `hwlat` with its own refusal
  line, and a reader for a nullable figure answers a missing figure and
  a number (FR-004, FR-028, P-11) (depends on T026, T027; the rows of
  T026 and T027 turn green). Verify: `ctest --test-dir build -R "harness_compare_test|harness_compare_cli_test" --output-on-failure` exits 0

**Checkpoint**: the order, the six saved values, the reverse restore, the
three figures and the six names pass, and every not-available path keeps
status 0.

---

## Phase 5: User Story 3 - the report states what the probe cannot see (Priority: P2) (module M5, M6 and M7, wave W2 and wave W3)

**Goal**: the console, the document and the loader carry the new
figures, the document carries the limits statement, the schema moves to
`1.16`, and a run that names neither new word writes the bytes master
writes. The wave W3 half adds the loader.

**Independent Test**: a run of the method prints the limits line once
and writes the limits key, a run that names neither new word writes no
new key, and a document written before the feature loads unchanged.

### Tests for User Story 3 (write first, observe failure)

- [X] T033 [P] [US3] Write the failing writer rows in
  `test/source/harness_json_report_test.cpp`: `smiDelta`, `nmiDelta` and
  `smiGapCount` stand after `droppedGaps` in `jitter.stats[]`, the
  per-benchmark items and `precheck.cpus[].stats` for the `hwlat` method
  alone, each per-benchmark item reading `null` for both deltas and folding
  its count from its own kept gaps (spec FR-025), and `series` stands absent for the method, the
  way it stands absent for `tsc` (D-1); `smi` stands beside each raw gap
  under `--json-raw=gaps` with
  `SMI`, `NOT_SMI` or `null`; `windowUs`, `widthUs` and `thresholdUs`
  stand beside the engine word and `maxGapNs`, `gapCount` and `nmiCount`
  beside `hardware` for that engine alone; `limits` stands beside
  `disclosure` for the method alone; `kSchemaVersion` reads `1.16`
  (FR-004, FR-025 to FR-027, FR-031, D-5, D-13, P-7, P-13). Verify: `ctest --test-dir build -R harness_json_report_test --output-on-failure` exits non-zero (red observed)
- [X] T034 [P] [US3] Write the failing schema rows in
  `test/source/harness_json_schema_test.cpp`: the six `x-order` lists of
  `jitter`, `jitter.stats[]`, the gap item, `precheck.cpus[].stats`,
  `precheck.tracer` and `precheck.tracer.cpus[]` read the order the
  writer writes, the two enum lists gain `HWLAT` and `hwlat`, no new name
  joins a `required` list, each touched object keeps
  `additionalProperties` at `false`, and the top-level `x-order` stands
  unchanged (FR-004, FR-029, D-6). Verify: `ctest --test-dir build -R harness_json_schema_test --output-on-failure` exits non-zero (red observed)
- [X] T035 [US3] Write the failing byte-identity rows in
  `test/source/harness_jitter_bytes_test.cpp`: a run that names neither
  new word writes a document whose bytes fold equal to the standing
  fixture apart from the schema and version values, and an `osnoise`
  document loses no byte and no name (FR-025, FR-029, SC-006). Verify:
  `ctest --test-dir build -R harness_jitter_bytes_test --output-on-failure`
  exits non-zero (red observed)
- [X] T036 [P] [US3] Write the failing loader rows in
  `test/source/harness_json_load_test.cpp`: the six figures read back
  with equal values, a gap item whose `smi` carries neither word
  refuses, a `null` reads as the absent figure, and a document of schema
  `1.14` loads with no new key and no complaint (FR-004, FR-026, FR-029,
  A-7). Verify: `ctest --test-dir build -R harness_json_load_test --output-on-failure` exits non-zero (red observed)
- [X] T037 [P] [US3] Write the failing probe-block rows in
  `test/source/harness_jitter_window_test.cpp`: the lines `smiDelta:`,
  `nmiDelta:` and `smiGapCount:` stand after `droppedGaps:` with one
  column per probe cpu for the method alone; `jitter: SMI count not
  available` and `jitter: NMI count not available` print once per run
  per source; `jitter: probe runs with interrupts enabled` prints once;
  the pre-check block prints `smi delta:` naming `/dev/cpu/<n>/msr`,
  `nmi delta:` naming `/proc/interrupts` and `smi gaps:` per measured
  cpu, with the `not available` token in an absent figure position
  (FR-004, FR-016, FR-030, FR-031, P-12, P-13). Verify: `ctest --test-dir build -R harness_jitter_window_test --output-on-failure` exits non-zero (red observed)
- [X] T038 [P] [US2] Write the failing tracer-block rows in
  `test/source/harness_noise_report_test.cpp`: the line `tracer:
  windowUs=<us> widthUs=<us> thresholdUs=<us>` stands after the standing
  engine header, three figure lines stand per tracer cpu in ascending
  order after the `hardware` line, `tracer: not available` stands
  unchanged, and one line per failed restore names the file and the
  saved value ahead of the pre-check block (FR-020, FR-022, FR-027,
  D-12). Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits non-zero (red observed)

### Implementation for User Story 3

- [X] T039 [P] [US3] Move the word tables in
  `source/harness/detail/json_report.hpp`: `HWLAT` joins
  `kJitterMethodWords`, `hwlat` joins `kTracerEngineWords`, and each
  `static_assert` moves to the new last enumerator, so a table that
  misses a word fails the build (FR-002, FR-018, FR-029, E-01). Verify:
  `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export -x c++ source/harness/detail/json_report.hpp` exits 0 and `grep -c "HWLAT" source/harness/detail/json_report.hpp` counts at least one line
- [X] T040 [US3] Write the keys in `source/harness/json_report.cpp`: the
  eleven keys in the six objects in the order T034 states, the method
  gate on the three stats keys, `smi` and `limits`, the engine gate on
  the six tracer keys, and `kSchemaVersion` at `1.16`, the two standing pins
  at `test/source/harness_hunt_json_test.cpp:1587` and
  `test/source/harness_topdown_report_test.cpp:816` moving with the stamp in
  a commit of their own (FR-004, FR-025 to
  FR-027, FR-029, FR-031, D-5, P-5, P-7, P-13) (depends on T033, T039;
  the rows of T033 turn green). Verify: `ctest --test-dir build -R harness_json_report_test --output-on-failure` exits 0
- [X] T041 [P] [US3] Extend `docs/schema/report-v1.json`: the eleven keys
  join `properties` with their kinds, the six `x-order` lists move, the
  two enum lists gain their word, and no new name joins a `required`
  list (FR-004, FR-029, D-6) (depends on T034, T040; the rows of T034
  turn green). Verify: `ctest --test-dir build -R harness_json_schema_test --output-on-failure` exits 0
- [X] T042 [P] [US3] Read the figures in `source/harness/json_load.cpp`
  through the standing optional path: the three stats figures, the six
  tracer figures and the `smi` kind check beside `stamp` and `gapNs`,
  with no new required key, so an older document loads unchanged
  (FR-004, FR-026, FR-029, A-7) (depends on T036, T040; the rows of T036
  turn green). Verify: `ctest --test-dir build -R harness_json_load_test --output-on-failure` exits 0
- [X] T043 [P] [US3] Print the lines in `source/harness/report.cpp`: the
  three probe figure lines, the two disclosures, the limits line, the
  three pre-check attribution lines each naming its source file, the
  tracer settings line, the three tracer figure lines and one
  restore-failure line per failed path, every standing line
  byte-identical and the `not available` token held by
  `kUnavailableFigure` (FR-004, FR-016, FR-022, FR-027, FR-030, FR-031,
  P-12, P-13) (depends on T037, T038, T040; the rows of T037 and T038
  turn green). Verify: `ctest --test-dir build -R "harness_jitter_window_test|harness_noise_report_test" --output-on-failure` exits 0
- [X] T044 [US3] Re-record the byte fixtures in
  `test/source/harness_jitter_bytes_test.cpp` for the moved schema and
  version values, keeping every other byte (FR-029, SC-006) (depends on
  T035, T040; the rows of T035 turn green). Verify: `ctest --test-dir build -R harness_jitter_bytes_test --output-on-failure` exits 0

**Checkpoint**: the eleven keys, the closed schema at `1.16`, the loader,
the console lines and the byte identity all pass.

---

## Phase 6: User Story 4 - a bad value refuses before anything runs (Priority: P2) (module M3, wave W2)

**Goal**: the fourth method word settles in the standing grammar, and
every invalid value of the two grammars answers one quoted line at
status 2 before any benchmark runs. The engine-word half of the refusal
surface stands in T025, where the engine word lands.

**Independent Test**: for each invalid value of the two error tables the
run prints one line naming the fault and exits 2 with no benchmark
output.

### Tests for User Story 4 (write first, observe failure)

- [X] T045 [US4] Write the failing method-word rows in
  `test/source/harness_jitter_cli_test.cpp`: `method=hwlat` settles
  `sg::JitterMethod::HWLAT`, the refusal reads `--jitter-probe: the
  method 'smi' is not tsc, fwq, ftq or hwlat`, a second `method` key
  reads `--jitter-probe: the key 'method' stands twice in the value`,
  a bad value from an option file line names the path, the line and the
  token, and each case prints one line at status 2 with no benchmark
  output (FR-002, FR-005, D-1, D-6). Verify: `ctest --test-dir build -R harness_jitter_cli_test --output-on-failure` exits non-zero (red observed)
- [X] T046 [P] [US4] Write the unchanged-table guard rows in
  `test/source/harness_cli_table_test.cpp`: the name and value
  uniqueness checks stand over the unchanged table,
  `kJitterProbeOptionValue` holds `260`, `kOsNoiseOptionValue` holds
  `320`, and the block comment keeps `321` to `329` free (FR-002,
  FR-018, D-1, D-2, X.2). Verify: `ctest --test-dir build -R harness_cli_table_test --output-on-failure` exits 0 (the table gains no row, and this task records that reading)

### Implementation for User Story 4

- [X] T047 [US4] Add the fourth method word in
  `source/harness/jitter_probe.cpp`: the branch accepts `hwlat`, the
  accepted-word list the refusal prints stands in the same branch, and
  the method keeps the standing threshold rule of 200 ns, rising to ten
  times the measured loop period, with an explicit `threshold=` value
  over both (FR-002, FR-005, D-1, D-11, A-4) (depends on T045, T046;
  the rows of T045 turn green). Verify: `ctest --test-dir build -R "harness_jitter_cli_test|harness_cli_table_test" --output-on-failure` exits 0
- [X] T048 [US4] Hold the status surface in
  `test/source/harness_jitter_cli_test.cpp`: a run of `method=hwlat` on
  a host that withholds both sources keeps status 0, a missing `msr`
  node answers no refusal line, and the only status-2 paths for the two
  new words stand the value refusals. The same run keeps the series group
  and the noise gate off, because the method holds no series: the gate and
  the series lines read the two calibrated methods alone (D-1)
  (depends on T047). Verify: `ctest --test-dir build -R harness_jitter_cli_test --output-on-failure` exits 0

**Checkpoint**: four method words and three engine words settle, every
other value refuses one line at status 2, and the table gains no row.

---

## Phase 7: User Story 5 - the docs teach both surfaces (Priority: P3) (module M9, wave W4)

**Goal**: the guide page carries one section per surface, the example
and its recorded run stand guarded by the drift gate and the shape gate,
and the main page holds its cap.

**Independent Test**: the example builds and runs, the `.expected` file
folds equal against the run, the page block matches its region, and the
prose gate reads no finding on the new lines.

- [X] T049 [US5] Add the two sections to `docs/pages/harness.md`:
  the method word joins the jitter section and the engine word joins the
  noise tracer subsection in the page's standing shape; the two sections
  state the three rules a reader cannot read off the figures: a mark
  answers the span between two counter reads, the detector runs one
  thread per tracer cpu with interrupts off for its width, and a restore
  leaves the kernel's remembered threshold at the run's value; the
  added lines carry no spec id, no requirement id, no commit hash and no
  kernel reference (FR-006, FR-031, FR-036, XI). Verify: `python3 tools/prose/prose_gate.py --check prose --mode tree` exits 0 and `grep -c "probe runs with interrupts enabled" docs/pages/harness.md` counts at least one line
- [X] T050 [P] [US5] Stand the example pair: `example/docs/hwlat_probe.cpp`
  and its recorded run in `example/docs/hwlat_probe.expected`, the
  markers `# sg-example:begin report` and `# sg-example:end report`
  around the recorded block, the print behind `speedgunMain`, and the
  `--list` answer kept to the registry (FR-006, FR-037, IV.1). Verify:
  `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export example/docs/hwlat_probe.cpp` exits 0 and `grep -c "sg-example:begin report" example/docs/hwlat_probe.expected` counts one line
- [X] T051 [US5] Register the example and its page block:
  `add_docs_example(hwlat_probe)` in `example/CMakeLists.txt`, the
  `report_shape_hwlat_probe_sample` entry with its 120 s ceiling and its
  one-line budget row in `test/CMakeLists.txt`, and the one
  `<!-- sg-sample-output: example/docs/hwlat_probe.expected#report -->`
  block in `docs/pages/harness.md` built from the recorded file (FR-006,
  FR-037, SC-010) (depends on T049, T050). Verify: `ctest --test-dir build -R "example_drift|snippet_compile|example_run|report_shape" --output-on-failure` exits 0
- [X] T052 [P] [US5] Move the two option rows in `README.md` in place:
  the `--jitter-probe` row names the fourth method word and the
  `--os-noise` row names the third engine word, the teaching of the two
  new words stays on the guide page, and the page holds the
  `PAGE_LINE_CAP` of 1,004 lines against the 1,001 lines of master
  (FR-006, FR-037, SC-009, IV.1). Verify: `python3 tools/docs/example_drift.py .` exits 0 and `wc -l < README.md` counts at most 1004
- [X] T053 [US5] State the residue and the difference in
  `docs/pages/harness.md`: the kernel keeps the run's threshold as the
  default a later run takes when it names none, and the two surfaces
  differ in privilege, threshold unit and interrupt state (FR-031,
  FR-036, FR-038, D-8) (depends on T049). Verify: `python3 tools/docs/example_drift.py .` exits 0 and `python3 tools/prose/prose_gate.py --check prose --mode tree` exits 0

**Checkpoint**: the drift gate folds equal, the snippet compiles, the
example runs, the shape gate passes and the prose gate reads no finding.

---

## Phase 8: User Story 6 - the acceptance runs stand recorded (Priority: P3) (module M10, wave W4)

**Goal**: two root runs on corsairone stand recorded, one per surface,
each naming the host, the kernel, the compiler, the command, the exit
code and the figures. The two runs replay the head Phase 9 lands.

**Independent Test**: the results directory holds one record per run, the
quickstart links each record, and the detector record names each file's
value before and after.

- [ ] T054 [P] [US6] Record the user-space run in
  `specs/048-hwlat-probe/results/user-space.md`, rights needed: root on
  corsairone, the lane host gives none to this session: the host, kernel
  and compiler lines, the command quickstart.md names, the exit code,
  the console figures, the keys the document wrote, and the reading of
  the same command without root (FR-038, SC-005) (depends on T059 to
  T062). Verify: `grep -c "method=hwlat" specs/048-hwlat-probe/results/user-space.md` counts at least one line and the record names the exit code
- [ ] T055 [P] [US6] Record the detector run in
  `specs/048-hwlat-probe/results/detector.md`, rights needed: root on
  corsairone, the lane host gives none to this session: the six saved
  files each with the value before the run and the value after it, the
  `tracing_on` reading of 0, the three figures per tracer cpu, the
  kernel's own `dmesg` lines for the detector if any, and the residue note (FR-021,
  FR-024, FR-038, SC-005, SC-007) (depends on T059 to T062). Verify:
  `grep -c "hwlat_detector/mode" specs/048-hwlat-probe/results/detector.md`
  counts at least two lines, the before row and the after row
- [ ] T056 [US6] Append the two raw snapshot blocks to
  `specs/048-hwlat-probe/results/detector.md`, the tracefs reading ahead
  of the run and the reading after it, beside the line that states the
  standing `diff` of the two stands empty: the table T055 wrote carries
  each value, and these two blocks carry the whole text of both reads,
  rights needed: root on corsairone, the
  lane host gives none to this session (FR-021, SC-007) (depends on
  T055). Verify: `diff /tmp/tracing-before.txt /tmp/tracing-after.txt` exits 0 on the lane host and the record names that empty diff

**Checkpoint**: one record per surface stands, with the before and after
table of the six saved files and the empty diff.

---

## Phase 9: Integration - the entry point, the record build and the version (module M10, wave W4)

**Purpose**: the wiring the stories stand on: the lifecycle calls, the
record build, the disclosure print and the version record.
`source/harness/cli.cpp` and `CMakeLists.txt` stand under one owner at
a time.

- [X] T057 Cover the third engine word in the standing
  missing-window refusal in `source/harness/cli.cpp`: a `--os-noise`
  value on a batch with no `--jitter-probe` option and on a mode that
  runs no pre-check window each print one line at status 2 ahead of any
  run (FR-003, FR-005, FR-018, D-2) (depends on T025). Verify:
  `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0
- [ ] T058 Open the counters in the probe path of
  `source/harness/cli.cpp`: reach the unit of Phase 2 for the two
  sources, take the edge pair with the latch snapshot, and leave the
  standing path untouched for the three standing methods (FR-001,
  FR-016, FR-025, D-7, P-1) (depends on T019, T043). Verify:
  `g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness -Ibuild/export source/harness/cli.cpp` exits 0
- [X] T059 Build the record in `source/harness/cli.cpp`:
  `buildJitterReportRecord` fills `smiChanges` beside `gaps`, the three
  figures ride each cpu's `JitterStats`, and a cpu whose source stands
  absent leaves the list empty so the writer answers `null` for its
  gaps (FR-015, FR-016, FR-025, FR-026, D-13, P-6) (depends on T040,
  T042, T058; the rows of T013 and T033 turn green). Verify: `ctest --test-dir build -R "harness_hwlat_probe_test|harness_json_report_test" --output-on-failure` exits 0
- [ ] T060 Wire the lifecycle in `source/harness/cli.cpp`: the
  setup stands before the pre-check window, the figures stand read after
  it ends, the restore runs on each path the process controls, one line
  per failed restore prints ahead of the pre-check block, and the
  disclosure keeps the run at status 0 (FR-003, FR-021, FR-022, FR-023,
  D-12, D-15) (depends on T031, T043; the rows of T022, T024 and T038
  turn green). Verify: `ctest --test-dir build -R "harness_noise_test|harness_noise_report_test" --output-on-failure` exits 0
- [X] T061 Keep the two surfaces apart in `source/harness/cli.cpp`:
  a run that names `method=hwlat` and `--os-noise=hwlat` records each
  surface on its own keys, and no figure of one enters the other
  (FR-004, FR-027, D-6) (depends on T059, T060). Verify: `ctest --test-dir build -R harness_hwlat_tracer_test --output-on-failure` exits 0
- [X] T062 [P] Move the version in `CMakeLists.txt`: the project
  `VERSION` reads `0.32.0`, the harness `SOVERSION` moves from `21` to
  `22` with the comment naming the three layouts that grew and the two
  enums that gained a word, and the archive `SOVERSION` holds at `6`,
  because both readers live on the harness side; move the two readers
  that assert the numbers (FR-004, FR-038, D-5, VIII) (depends on T003,
  T040). Verify: `ctest --test-dir build -R "harness_cli_table_test|views_test" --output-on-failure` exits 0 and `grep -c "^    SOVERSION 22" CMakeLists.txt` counts one line
- [X] T063 Run the whole suite at the integrated head in the
  `build` tree (FR-032, FR-038, VIII) (depends on T057 to T062). Verify:
  `ctest --test-dir build --output-on-failure` exits 0

**Checkpoint**: the head stands green end to end, and the version names
the ABI move.

---

## Phase 10: Docs and Contracts - the nine standing files (module M9, wave W3)

**Purpose**: each amendment rides the commit that lands the code it
describes (FR-039). This phase lands in wave W3, ahead of the docs phase
that quotes the same words.

- [X] T064 [P] Move the method-word rows in
  `specs/027-jitter-probe/contracts/cli-options.md`: the grammar names
  four words, the word row names `hwlat`, and the error row reads `A
  method word outside the four` (FR-002, FR-005, FR-039) (depends on
  T047). Verify: `grep -c "outside the four" specs/027-jitter-probe/contracts/cli-options.md` counts one line
- [X] T065 [P] Move the engine-word rows in
  `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md`: the
  grammar names three words, the word row names `hwlat`, the error row
  reads `An engine word outside the three`, and the restore row records
  the partial-setup fix the shared lifecycle gains (FR-003, FR-018,
  FR-021, FR-039, D-15) (depends on T031). Verify: `grep -c "outside the three" specs/036-osnoise-timerlat-tracers/contracts/cli-options.md` counts one line
- [X] T066 [P] Move the report rows in
  `specs/027-jitter-probe/contracts/report-json.md` and
  `specs/034-host-noise-precheck/contracts/report-json.md`: the stats
  items carry the nineteen names of `JitterStats` and the gap item
  carries the `smi` mark (FR-025, FR-026, FR-039) (depends on T040,
  T043). Verify: `grep -c "nineteen" specs/027-jitter-probe/contracts/report-json.md` counts at least two lines and `grep -c "nineteen" specs/034-host-noise-precheck/contracts/report-json.md` counts at least one line
- [X] T067 [P] Move the tracer rows in
  `specs/036-osnoise-timerlat-tracers/contracts/report-json.md`: the
  engine kind names `hwlat`, three settings rows join above the `cpus`
  row, and three figure rows join below the `hardware` row (FR-027,
  FR-039) (depends on T040). Verify: `grep -c "windowUs" specs/036-osnoise-timerlat-tracers/contracts/report-json.md` counts at least one line
- [X] T068 [P] Move the console rows in
  `specs/034-host-noise-precheck/contracts/console-report.md` and
  `specs/036-osnoise-timerlat-tracers/contracts/console-report.md`: the
  field names are the nineteen the console probe block uses, three
  `hwlat` lines follow the per-cpu attribution group, and three figure
  lines follow the standing `hardware` line (FR-016, FR-027, FR-039)
  (depends on T043). Verify: `grep -c "nineteen" specs/034-host-noise-precheck/contracts/console-report.md` counts at least two lines
- [X] T069 [P] Move the rule rows in
  `specs/027-jitter-probe/contracts/rule-names.md` and
  `specs/026-regression-gate/contracts/rule-parse.md`: the field row
  names the 35 names, the refusal reads `names none of the thirty-five
  jitter fields`, and the value production gains the `hwlat` alternative
  (FR-028, FR-039) (depends on T032). Verify: `grep -c "thirty-five" specs/027-jitter-probe/contracts/rule-names.md` counts at least one line and `grep -c "hwlat" specs/026-regression-gate/contracts/rule-parse.md` counts at least one line

**Checkpoint**: the nine standing contract files carry the new words,
each in the commit that lands the code it describes.

---

## Phase 11: Polish and Cross-Cutting Concerns

**Purpose**: the hard gates of Principle VIII at the feature head, the
coverage contract, the pinned suite, the gate records and the commits.

- [X] T070 [P] Run the drift gate over the page, the regions and the
  option rows (FR-037, FR-038, IV.1). Verify: `python3 tools/docs/example_drift.py .` exits 0
- [X] T071 [P] Run the format gate over the lane's sources (V, FR-038).
  Verify: `cmake -P cmake/lint.cmake` exits 0
- [X] T072 [P] Run the spell gate (FR-038, FR-039). Verify: `cmake -P cmake/spell.cmake` exits 0
- [X] T073 [P] Run the time-source gate over the lane's files (FR-033,
  FR-038). Verify: `bash test/time_source_gate.sh .` exits 0
- [X] T074 [P] Run the prose gate in tree mode over the added lines; the
  range form needs a base that stands behind HEAD (FR-006, FR-038, XI).
  Verify: `python3 tools/prose/prose_gate.py --check prose --mode tree` exits 0
- [X] T075 [P] Run the DBC pair gate through the build target; the base
  reads 347 interfaces with 0 gaps, and every new interface carries its
  documented kind and its enforced kind (II, FR-038). Verify: `cmake --build build -t dbc-gate` exits 0 and its report names 0 gaps
- [ ] T076 [P] Run the analyzer build in a fresh tree, because a
  reconfigure over a built tree recompiles no unit and an empty finding
  list proves nothing (V.1, VIII, SC-004). Verify: `cmake -S . -B build/dev --preset=ci-ubuntu` exits 0 and `cmake --build build/dev` exits 0
- [ ] T077 [P] Run the sanitizer lane (VIII, SC-004). Verify: `cmake --preset=ci-sanitize` exits 0, `cmake --build build/sanitize` exits 0 and `ctest --test-dir build/sanitize --preset ci-sanitize` exits 0
- [ ] T078 [P] Run the thread-sanitizer lane (VIII, SC-004). Verify: `cmake --preset=ci-tsan` exits 0, `cmake --build build/tsan` exits 0 and `ctest --test-dir build/tsan --preset ci-tsan` exits 0
- [ ] T079 Run the coverage contract in `build/coverage`: configure,
  build, run the suite and produce the trace; the reading holds 100%
  line coverage, the branch shortfall stands at or under 21 rows, the
  tree holds at most 84 exclusion markers, and no marker joins under
  `source/harness` (FR-038, SC-002, VI) (depends on T063). Verify: `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0, `grep -rn "LCOV_EXCL" source/harness` reads no match and `grep -rn "LCOV_EXCL" source test example include tools | wc -l` reads at most 84
- [X] T080 Run the full suite and read the entry count: three new
  executables and one new shape entry join the 181 entries of record, and
  the 13 tests of the attribution record stand among them (FR-032, SC-001,
  SC-003, VIII)
  (depends on T063). Verify: `ctest --test-dir build --output-on-failure` exits 0, `ctest --test-dir build -N | tail -n 1` reads `Total Tests: 185`, and `grep -hoE 'the[A-Za-z]+Rows' test/source/harness_hwlat_probe_test.cpp test/source/harness_hwlat_tracer_test.cpp test/source/harness_latency_counters_test.cpp | sort -u | wc -l` reads at least 13
- [ ] T081 Run the pinned suite on corsairone in the `build/prepush`
  tree (FR-032, SC-003, VIII) (depends on T063). Verify: `taskset -c 0,1 ctest --preset prepush-quick -j 2` exits 0
- [X] T082 [P] Run the no-sleep and no-clock review over the three new
  test files (FR-032, FR-033). Verify: `grep -rlnE "sleep_for|steady_clock|system_clock|gettimeofday" test/source/harness_latency_counters_test.cpp test/source/harness_hwlat_probe_test.cpp test/source/harness_hwlat_tracer_test.cpp` reads no match
- [X] T083 [P] Run the code-discipline review over the lane's sources:
  no lambda stands inside an `SG_` macro, the unit of Phase 2 stands the
  only kernel interface reader, and simdjson stands the only JSON reader
  the change uses, and no unit of the lane reaches for a privilege the
  process does not hold (FR-024, FR-038, II, X.2). Verify: `grep -rnE "SG_(REQUIRE|ENSURE|INVARIANT)\(.*\] *\(" source/harness/latency_counters.cpp source/harness/noise_tracer.cpp source/harness/jitter_probe.cpp` reads no match, `grep -rnE "nlohmann|rapidjson" source/harness | wc -l` reads 0 and `grep -rniE "setuid|capset|daemon|modprobe|request_module|insmod" source/harness | wc -l` reads 0
- [ ] T084 Write the gate record in
  `specs/048-hwlat-probe/results/gates.md`: each gate with its command
  and its exit code, the coverage reading with its line and branch
  figures, the marker count, the entry count and the pinned suite
  reading (FR-038, SC-003, SC-004) (depends on T070 to T081). Verify:
  `grep -c "exits 0" specs/048-hwlat-probe/results/gates.md` counts at
  least nine lines
- [ ] T085 Read the CI run of the pushed head and add its job readings
  to `specs/048-hwlat-probe/results/gates.md`: every job green, the
  coverage figures named, and the run reference recorded. This task needs
  a push of the squashed head to the remote and the owner's review of the
  pull request, two rights this lane's session does not hold (VIII,
  FR-038) (depends on T084). Verify: `grep -cE "lint|test|sanitize|tsan|coverage|dbc-gate|prose" specs/048-hwlat-probe/results/gates.md` counts at least seven lines
- [ ] T086 Land the foundational commit: the surface of Phase 1 and the
  unit of Phase 2 with their tests, the title form
  `<Section>: <one-line imperative>` at 50 characters or fewer, the
  why-body wrapped at 72 columns, the author name `OmO` with the address
  `omo@speedgun-ng.local`, and the footer lines
  `Refs: specs/048-hwlat-probe N13d` and `Approved-by: OmO` (FR-035,
  IX, XI). Verify: `git log -1 --format=%s | wc -c` reads at most 51 and `git log -1 --format=%B | grep -c "Approved-by: OmO"` counts one line
- [ ] T087 Land the record and docs commit: the writer, the schema, the
  reader, the console, the comparison, the guide page, the example pair
  and the nine contract files with their rows and tests, each test standing in the
  commit that carries the code it tests, on the same title, body, author
  and footer shape (FR-035, FR-039) (depends on T064 to T069). Verify:
  `git show --stat --format=%B HEAD | grep -c "Refs: specs/048-hwlat-probe N13d"`
  counts one line
- [ ] T088 Land the integration commit and squash the lane: the wiring,
  the version and the records, then one rebase and squash so the base
  stays linear and the merge stands a pull request with `--squash`, on
  the same title, body, author and footer shape (FR-035, VIII, IX)
  (depends on T080 to T087). Verify: `git log --oneline origin/master..HEAD | wc -l` reads 1 and `git log -1 --format=%an <%ae>` reads `OmO <omo@speedgun-ng.local>`

**Checkpoint**: every hard gate stands green at the head, the gate
records stand written, and the lane stands one commit on a linear base.

---

## Dependencies and Execution Order

### Module graph

```text
M2 (W1) ──> M1, M4, M5, M7 (W2) ──> M3, M6, M8 (W3) ──> M10 (W4) ──> M9 docs (W4)
M9 contracts land in W3, ahead of the M9 docs half in W4.
```

- Phase 1 blocks every later phase. Phase 2 blocks Phase 3, because the
  gap path reads the unit of Phase 2.
- Phase 9 needs Phases 2 to 6 green and blocks Phase 8.
- Phase 10 needs the code rows it describes and lands ahead of Phase 7.
- Phase 11 needs every phase above green.

### Parallel execution

- **Wave W1**: T001 and T002 run one after another on
  `test/CMakeLists.txt`, then T003 stands alone.
- **Wave W2** (parallel): Phase 2 (M1), Phase 4's W2 half (M4), Phase
  5's W2 half (M5 and M7) and Phase 6 (M3, the method word). The four
  blocks touch disjoint files. T029 and T030 stand behind T028 and name
  `source/harness/noise_tracer.cpp` one owner at a time.
- **Wave W3** (parallel): Phase 3 (M3), Phase 4's W3 half (M8), Phase
  5's W3 half (M6), T011 and Phase 10 (M9 contracts). No block names a
  file another block names in the same wave.
  `source/harness/jitter_probe.cpp` stands in W2 for T047 and in W3 for
  T018 and T019, and no two of them run at once.
- **Wave W4** (parallel at the start, then serial): Phase 7 and T062 run
  in parallel; `source/harness/cli.cpp`, `CMakeLists.txt`,
  `test/CMakeLists.txt` and `README.md` each stand one owner at a time;
  T079 to T088 run one after another.
- Within a phase the test tasks marked [P] run in parallel, because each
  names a test file no other [P] task names.

### MVP note

The MVP stands at the end of Phase 3 with Phase 6 landed: a run of
`--jitter-probe=idle,method=hwlat` marks its gaps and folds the two deltas
and the marked count into the record, and an invalid value refuses before
anything runs. Phase 5 puts the eleven keys into the document, and the
detector, the console detail, the docs and the records follow by
priority.

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Complete Phase 1: the surface and the executables.
2. Complete Phase 2: the two counter sources.
3. Complete Phase 6: the method word and the refusals (wave W2).
4. Complete Phase 3: the ring, the reads and the folds.
5. **STOP and VALIDATE**: the fake-counter scenarios print and record
   the marks, and the cost proof holds.

### Incremental Delivery

1. Phase 1 and Phase 2: the surface and the sources stand (W1, W2).
2. Phase 4, Phase 5 and Phase 6: the engine, the record surfaces and the
   grammar stand (W2, W3; four workers).
3. Phase 3: the attribution closes User Story 1 (W3).
4. Phase 10: the nine contract files ride the code commits (W3).
5. Phase 9: the integration closes the wiring and the version (W4).
6. Phase 7 and Phase 8: the docs and the two acceptance records stand
   (W4).
7. Phase 11: the gates, the coverage contract, the pinned suite, the gate
   records and the commits close the lane.

### Notes on placement

- T089 carries the next free id, because it joined after the numbering
  closed. It stands in the User Story 2 test block ahead of T028, and turns
  green with T031.
- `test/CMakeLists.txt` carries three single-owner registration tasks:
  T001, T002 and T011, and T051 adds the shape entry in W4.
- Phase 8 depends on Phase 9: the two acceptance runs replay the head
  the integration lands, and the dependency stands stated here rather
  than in the phase order.
- The engine-word refusal rows of User Story 4 stand in T025, because the
  engine word lands with the detector engine in module M4.
- The plan names module M3 in W3; this list lands the method-word branch
  of FR-002 in W2 as T047, because the attribution of Phase 3 reads that
  branch, and the covering test task T045 stands with it.

- [X] T049 [US6] Teach the method and the engine in
  `docs/pages/harness.md`. The lead names the two counter sources
  and what one mark answers, and the option table, the field table and the names
  table carry the fourth word and the three figures. The noise tracer names the
  third engine word in its grammar, in its refusal and in its record, states the
  settings line the detector writes, the rule that one thread per tracer cpu runs
  with interrupts disabled for the width, and the threshold the kernel keeps after
  the restore. The probe names both sources for both surfaces. The tree-wide prose
  gate reads 1027 sources, 197535 units, 0 findings, and `grep -c` finds
  `probe runs with interrupts enabled` twice on the page.
- [X] T051 [US6] [P] Pair the example in `example/docs/hwlat_probe.cpp`, and the
  parent adds `add_docs_example(hwlat_probe)` to `example/CMakeLists.txt` and the
  `report_shape_hwlat_probe_sample` entry, with the 120-second budget exception,
  to `test/CMakeLists.txt`. The file calls `sg::runJitterProbe` with
  `sg::JitterMethod::HWLAT` and `sg::TracerEngine::HWLAT` and prints nothing of
  its own, and `sg::speedgunMain` runs after the call. The recorded run of
  `example/docs/hwlat_probe.expected` answers the shape gate.
- [X] T052 [US6] Move the two option rows in place in `README.md`:
  the `--jitter-probe` row reads `method=tsc|fwq|ftq|hwlat`, and the `--os-noise`
  row names the three engines. The two capability-summary lines that named two
  methods or two engines name the third with them, in the same lines. The file
  stands at 1001 lines, below the 1004 bound, and
  `python3 tools/docs/example_drift.py .` reads no findings.
- [X] T053 [US6] Record the run in `example/docs/hwlat_probe.expected`, beside the
  page block that matches it line for line. The block carries the three
  attribution lines inside each pre-check unit, the three detector figures after
  `droppedGaps`, the two disclosures and no series line, and the shape gate folds
  it equal against a real run. `tools/docs/report_shape.py` reads no finding on
  this host, and its cpu-fold leaves the figures, the method word and the line
  classes standing.
