
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
  `VERSION` reads `0.34.0`, the harness `SOVERSION` moves from `21` to
  `22` with the comment naming the three layouts that grew and the two
  enums that gained a word, and the archive `SOVERSION` holds at `8`,
  where specs/050 left it, because both readers live on the harness side; move the two readers
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
- [X] T075 [P] Run the DBC pair gate through the build target; the merged
  base reads 349 interfaces with 0 gaps, and every new interface carries its
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
  executables and one new shape entry join the 186 entries of record, and
  the 17 tests of the attribution record stand among them (FR-032, SC-001,
  SC-003, VIII)
  (depends on T063). Verify: `ctest --test-dir build --output-on-failure` exits 0, `ctest --test-dir build -N | tail -n 1` reads `Total Tests: 190`, and `grep -hoE 'the[A-Za-z]+Rows' test/source/harness_hwlat_probe_test.cpp test/source/harness_hwlat_tracer_test.cpp test/source/harness_latency_counters_test.cpp | sort -u | wc -l` reads at least 13
- [ ] T081 Run the pinned suite on corsairone in the `build/prepush`
  tree (FR-032, SC-003, VIII) (depends on T063). Verify: `taskset -c 0,1 ctest --preset prepush-quick -j 2` exits 0
- [X] T082 [P] Run the no-sleep and no-clock review over the three new
  test files (FR-032, FR-033). Verify: `grep -rlnE "sleep_for|steady_clock|system_clock|gettimeofday" test/source/harness_latency_counters_test.cpp test/source/harness_hwlat_probe_test.cpp test/source/harness_hwlat_tracer_test.cpp` reads no match
- [X] T083 [P] Run the code-discipline review over the lane's sources:
  no lambda stands inside an `SG_` macro, the unit of Phase 2 stands the
  only kernel interface reader, and simdjson stands the only JSON reader
  the change uses, and no unit of the lane reaches for a privilege the
  process does not hold (FR-024, FR-038, II, X.2). Verify: `grep -rnE "SG_(REQUIRE|ENSURE|INVARIANT)\(.*\] *\(" source/harness/latency_counters.cpp source/harness/noise_tracer.cpp source/harness/jitter_probe.cpp` reads no match, `grep -rnE "nlohmann|rapidjson" source/harness | wc -l` reads 0 and `grep -rniE "setuid|capset|daemon|modprobe|request_module|insmod" source/harness | wc -l` reads 0
- [X] T084 Write the gate record in
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
  stands at 1002 lines, below the 1004 bound, and
  `python3 tools/docs/example_drift.py .` reads no findings.
- [X] T053 [US6] Record the run in `example/docs/hwlat_probe.expected`, beside the
  page block that matches it line for line. The block carries the three
  attribution lines inside each pre-check unit, the three detector figures after
  `droppedGaps`, the two disclosures and no series line, and the shape gate folds
  it equal against a real run. `tools/docs/report_shape.py` reads no finding on
  this host, and its cpu-fold leaves the figures, the method word and the line
  classes standing.

## Phase 12: Convergence

- [ ] T090 Read the coverage gate to a green exit: raise the instrumented-lane
  ceilings to the readings of the serial coverage run, re-run the coverage
  tree's suite with one worker and no entry left unrun, close every line the
  trace leaves cold, and record the line figure, the branch shortfall as a
  number beside the allowance of 21 and the marker count (per SC-002, SC-004,
  Constitution VI and VIII, T079) (missing, CRITICAL)

- [ ] T091 Run the two root-only acceptance passes on the lane host and write
  `results/user-space.md` and `results/detector.md`, each naming host,
  kernel, compiler, command, exit code and figures, with the six-file
  before-and-after table and `tracing_on` at 0 (per SC-005, SC-007,
  T054-T056) (missing, HIGH, owner action: `/sys/kernel/tracing` stands
  `0700`)

- [ ] T092 Record the CI reading of the head this lane carries: every job, its
  answer, the compiler that job builds with, and the coverage figures,
  in `results/gates.md` (per SC-004, Constitution VIII, T085) (partial,
  HIGH)

- [X] T093 Measure the pinned two-cpu lane on the lane host, the quick prepush
  preset over two workers pinned to cpus 0 and 1, and record it, and take the
  pinned lane out of the reading the CI jobs stand in for (per SC-003, T081)
  (measured on the lane host, recorded in `results/gates.md`, HIGH)

- [X] T094 Decide the rule the marked-gap count and the counter disclosure answer
  to one source: a node that answers after the window's opening edge
  prints `SMI count not available` beside `smi gaps: n`, so either gate
  the count on the source or state the split in
  `contracts/console-report.md` and add the report row that reads the
  pair together (per FR-015, FR-016, T019) (done at 1595ea5e, MEDIUM)

- [X] T095 Restate the three guide lines that call the counter a machine-check
  interrupt: the figure is the system-management-interrupt count (per
  FR-006, FR-007, FR-014; `docs/pages/harness.md:1894, :2380, :2382`)
  (done at 980997a9, MEDIUM)

- [X] T096 Install the file-root seam around the three `method=hwlat` rows of
  `harness_jitter_cli_test.cpp`, or record the exception beside the
  standing specs/034 CLI rows (per FR-001;
  `test/source/harness_jitter_cli_test.cpp:3126, :3204, :3215`)
  (done at 6af151ba, MEDIUM)

- [X] T097 Stand a row in the probe unit that runs the window with no node in the
  planted tree and reads the console disclosure once, or restate the
  FR-011 coverage row to name the two units that hold the claim (per
  FR-011, US1 AC-3, T014) (done at 3d5daec5, MEDIUM)

- [X] T098 Give `readNullableCount` its own sentence and move the read of
  `readJitterCpu` back onto `readJitterCpu`: one comment ends
  mid-sentence and its tail stands three lines away above the wrong
  function (per Constitution IV; `source/harness/json_load.cpp:454-459,
  :532`) (done at 4f7e2ec2, MEDIUM)

- [X] T099 Rewrite the contract row that states the per-benchmark count figure
  "stands 0" to the fold the spec, the task and the code carry (per
  FR-025; `specs/048-hwlat-probe/contracts/report-json.md:94-97`)
  (done at 6e218c6b, MEDIUM)

- [X] T100 Stand one named row for `osnoise` or `timerlat` that reads
  `current_tracer` and `tracing_cpumask` back after a mid-setup write
  fails, the behaviour the amended specs/036 contract row promises (per
  FR-021, D-15, P-9;
  `specs/036-osnoise-timerlat-tracers/contracts/cli-options.md:95`)
  (done at a2f6e008, MEDIUM)

- [X] T101 Stand one named row that installs a denial after a setup that
  succeeded and reads the run's exit status, the status clause of the
  restore-failure requirement (per FR-022, FR-034;
  `source/harness/cli.cpp:1277-1278`) (done at 3abed7ac, MEDIUM)

- [X] T102 Stand one named row over a tree that carries `sys/kernel/tracing` and
  no `hwlat_detector/` directory, reading `tracer: not available`, the
  user-space figures whole and status 0 (per SC-001, FR-023, FR-034;
  `test/source/harness_hwlat_tracer_test.cpp:1016, :1037`) (done at bd19314b, MEDIUM)

- [ ] T103 Re-take the cheap-gate rows at a head the branch carries, so the
  record names no head that stands off its own history (per SC-004,
  T084) (partial, MEDIUM)

- [X] T104 Restate the two latch-fold comments to name both gap methods, the
  `hwlat` record reads the same fold the `tsc` record names (per FR-016,
  T017; `source/harness/detail/jitter_probe.hpp:365`,
  `source/harness/jitter_probe.cpp:1262`) (done at 5d989492 and 1e75af96, LOW)

- [X] T105 Re-anchor the line citations the lane's own commits moved in
  `contracts/console-report.md`, `contracts/report-json.md` and
  `contracts/rule-names.md`, or name the token in place of the line (per
  FR-004, FR-029) (done at 09f20aaa and d53785a8, LOW)

- [X] T106 Add the two-word enum for the gap mark to
  `docs/schema/report-v1.json`, or drop the "one of" clause from the
  contract row the schema does not carry (per FR-029;
  `docs/schema/report-v1.json:245`) (closed by ruling at 93674d3f, LOW)

- [X] T107 Tick the README line count the record states at 1001 to the 1002 the
  file holds (per T052, FR-037; `specs/048-hwlat-probe/tasks.md:880`)
  (done at fc330b99, LOW)

- [X] T108 Read the loaded document's interrupt outcome on the interrupted-run
  row, the clause of US2 scenario 4 the row leaves unasserted (per
  FR-021; `test/source/harness_hwlat_tracer_test.cpp:1257-1287`)
  (done at 16f0bf20, LOW)

- [X] T109 Justify the detector engine the docs example requests, or record the
  host limit it carries: a host that holds tracefs prints a tracer block
  the recorded block lacks, and specs/036's example carries the same
  shape (per FR-037, SC-011) (recorded in `results/gates.md`, LOW)



---

## Phase 13: Intel final acceptance (2026-11-07) - records only

**Goal**: three records stand with a command, an exit code and an output behind every
row they call measured, and with the right named behind every row they call blocked.
**Depends on**: the branch, and a tree built at the measured head.
**Tags**: `[read]` is a read or a parse and never waits. `[bench]` needs the machine to
itself. `[blocked]` runs only inside a privileged session. `[write]` writes a repository
file and never runs while a ctest sweep reads the tree.

### Part A - the hwlat surface (IA48-001 to IA48-006)

Part B stands in `specs/046-hunter-topdown/tasks.md` Phase 10 and Part C in
`specs/041-counter-probe/tasks.md` Phase 7. The order at the end of this phase names
their rows beside the rows below.

- [x] IA48-001 [read] Before anything runs, read SC-005, SC-007, FR-019 to FR-024 and
  the current quickstart, then record the host facts the run reads: cpu model and family,
  kernel, `msr` node mode, tracefs mount and mode, detector presence read with the path
  tried and the refusal met, kernel config lines read with `zcat /proc/config.gz`, the
  `current_tracer` read attempt, and today's answer of the `NMI` line of
  `/proc/interrupts` beside the standing artifact's note. Verify: the record names the
  sections it read and each fact with the command that read it and the output it printed,
  and no command writes.
- [x] IA48-002.1 [bench] Run the method surface without rights twice, the second run to
  a second document path, and keep the console, the exit code and each document. Verify:
  the record quotes the source disclosure for `smi` and `nmi`, the exit code each run
  answered, and the two document paths the round trip later reads.
- [x] IA48-002.2 [bench] Run the engine surface without rights and keep the same three
  things. Verify: the record calls the row a denied path, quotes `not available`, and
  carries no tracer figure and no `precheck.tracer` key.
- [x] IA48-002.3 [bench] Run the `docs_hwlat_probe` example and keep its output. Verify:
  the record names the target and states that the committed `.expected` block belongs to
  another host.
- [x] IA48-002.4 [read] Run the three refusal commands. Verify: the record gives exit 2
  for each with the quoted line.
- [x] IA48-002.5.1 [read] Close the round trip with a `--compare` run over the two method
  documents of this run, and carry the answer of a `jitter:` rule and of a `hwlat:` rule
  over those same documents. Verify: the row names both document paths and both rule
  answers.
- [x] IA48-002.5.2 [read] Print the parser key read beside the round trip. Verify: the
  row marks it a parser read and not the round trip, and where no root session stands
  available it carries the `schemaVersion` answer alone and names the `precheck.tracer`
  key as blocked.
- [x] IA48-002.6 [bench] Run the quickstart's method command of the root-runs block
  unprivileged and label the row with that command. Verify: the row quotes that command
  from `quickstart.md` without its `sudo` prefix, adds no `cpus=` key, and carries the
  output it printed.
- [x] IA48-003 [read] Ask for the operator action the two root surfaces need, and mark
  both rows blocked while no session stands available. Verify: the record names the
  missing right and quotes both quickstart root commands, the `method=hwlat` run and the
  `--os-noise=hwlat` run, without their `sudo` prefixes.
- [ ] IA48-004.1 [blocked] Preflight the privileged row: read `current_tracer` and
  `tracing_on`, and apply the stop rule. Verify: the row names both files and both
  readings, and where `current_tracer` reads other than `nop` or `tracing_on` reads 1 it
  names the file, the value and the fact that no run executed, with no console shape and
  no status.
- [ ] IA48-004.2 [blocked] Snapshot the six files D-8 names - `current_tracer`,
  `tracing_cpumask`, `tracing_thresh`, `hwlat_detector/mode`, `hwlat_detector/width`,
  `hwlat_detector/window` - read `tracing_on` beside them as a reading of its own outside
  the six, state the mode as the word the quickstart helper prints with no brackets, and
  save the console text, the document, the exit code and the end state. Verify: the row
  names the values only when a root session answered them and carries none when it did
  not.
- [ ] IA48-005.1 [blocked] Compare the six saved values one by one after a normal end,
  and state that the restore never writes a prior value back. Verify: six rows with
  before and after when a root run stands behind them, else the six names with empty
  value cells and no figure anywhere.
- [ ] IA48-005.2 [blocked] Run the controlled interruption with its own signal command
  and its own exit status, and give it its own six before-and-after rows and its own
  `tracing_on` statement at 0. Verify: the interrupted rows name their own run, and with
  no root session the row names the missing right and carries no figure.
- [x] IA48-005.3 [read] State the threshold residue as documentary: name it, quote D-8,
  plan R-4 and the guide page, and state that no user-space read reaches it. Verify: the
  row quotes the three sources and carries no figure.
- [x] IA48-006 [read] State that a zero SMI count is a measured figure and that the
  planted-SMI claim stands on the fixture tests. Verify: the record keeps hardware
  evidence and fixture evidence in separate rows.

### Part D - combined local acceptance (IA-END-001 to IA-END-003)

- [x] IA-END-001.1 [bench] Run the coverage chain in order: `cmake --preset=ci-coverage`,
  `cmake --build build/coverage -j "$(nproc)"`, `lcov --zerocounters -d build/coverage`
  named as a local addition, the `-j 1` suite in `build/coverage`,
  `cmake --build build/coverage -t coverage`, then
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` by hand. Verify: five
  exits stand in the record, the gate exit carries the verdict, and only 0 reads green;
  where an entry cannot finish, the row names each single-entry re-run with its exit and
  elapsed time and the missing trace, and those re-runs stand before the target step.
- [x] IA-END-001.2 [bench] Rebuild `build/sanitize` with `CXX=clang++` and run it with
  CI's `ASAN_OPTIONS` and `UBSAN_OPTIONS`. Verify: the record gives pass, fail, skip and
  not-run counts, the exit, and each red name with its base evidence.
- [x] IA-END-001.3 [bench] Rebuild `build/tsan` with `CXX=clang++` and run it with CI's
  `TSAN_OPTIONS`. Verify: the same counts, the exit, and each red name with its base
  evidence.
- [x] IA-END-001.4 [bench] Run the plain suite in the CI form in `build`: `ctest --test-dir build --output-on-failure --no-tests=error -C Release -j "$(nproc)"`
  and print the entry count from `ctest -N`. Verify: the record gives pass, fail, skip and
  not-run counts from the output, the entry count and the parallelism, and names the
  `prepush-quick` reading not run.
- [x] IA-END-001.5 [bench] Run the same form under `taskset -c 0,1` with `-j 2`. Verify:
  the record prints the affinity and online reads first, and marks the row blocked if
  those cpus stand unpallowed.
- [x] IA-END-001.6 [bench] Run the same form on four allowed cpus with `-j 4`. Verify:
  the row states it is extra evidence and not a substitute.
- [x] IA-END-001.8 [bench] Where a red name stands outside every tracked record, build
  the base in a second tree after the last timed step and run that name there. Verify:
  the row carries the base sha, the tree, the command, the exit and the output (rule 27),
  and no base row is quoted before the head row it answers.
- [x] IA-END-001.7 [read] Read the line, function and branch figures, the cold rows, the
  host shortfall and the allowance of 21 from `build/coverage/coverage.info`, each with
  the command that counted it and the output it printed; take the base shortfall and the
  base cold rows from the standing coverage record or from a named base tracefile and say
  which; state that R-1, R-2 and W-1 resolve to `specs/045-merge-fixes` FR-015 and carry
  no definition of their own, that a host run claims no green against the union allowance,
  and that the G-25 repair stands pending where the standing suppression hides
  inconsistent data. Verify: four branch numbers stand in the row, the function figure is
  marked ungated, and the row claims this branch adds no new shortfall on the set of cold
  rows and not on two totals.
- [x] IA-END-002.1.1 [read] Run the format gate `cmake -D FORMAT_COMMAND=clang-format -P
  cmake/lint.cmake` and the spell gate `cmake -P cmake/spell.cmake`. Verify: both commands
  stand with their exits and the output they printed, and the row names the tools it
  used.
- [x] IA-END-002.1.2 [read] Run the drift gate `python3 tools/docs/example_drift.py .` in
  plain and four-flag form, and the time-source gate `bash test/time_source_gate.sh .`.
  Verify: the three commands stand with their exits and the output each printed.
- [x] IA-END-002.1.3 [bench] Configure and build the release tree that SC-004 names, with
  the command the standing gate record uses. Verify: the configure exit and the build exit
  stand in the row beside the lines each step printed.
- [x] IA-END-002.2.0 [read] After the last record write, re-run `cmake -P
  cmake/spell.cmake`, because the spell gate reads the prose this lane writes and its
  first run stood before those files existed. Verify: the row gives both exits and the
  output of the second run.
- [x] IA-END-002.2.1 [read] After every record write and before the squash, run the prose
  gate in tree form, `cmake -D PROSE_MODE=tree -D PROSE_CHECK=prose -P
  cmake/prose-lint.cmake`, with the new records visible to `git ls-files`. Verify: the row
  names the command with both words and prints the prose unit count the gate reports, and
  a form that examined nothing gets no completion mark.
- [x] IA-END-002.2.2 [read] After the squash, run the range form, `cmake -D PROSE_MODE=range
  -D PROSE_BASE=<base-sha> -D PROSE_HEAD=<squashed-head-sha> -P cmake/prose-lint.cmake`.
  Verify: the range row names both shas and the prose unit count, and a range that resolves
  to nothing gets no mark.
- [x] IA-END-002.3 [read] Run the DBC document gate and the DBC pair gate, first through
  the `dbc-gate` target and then by hand with the interpreter that carries yaml. Verify:
  four exits stand in the row - the target, the doxygen pass, the document gate and the
  pair gate - each naming its interpreter and whether that interpreter carries yaml, with
  the interface count, the argument list whole and the output each command printed.
- [x] IA-END-002.4 [bench] Build the analyzer in a fresh tree with `cmake --preset=ci-ubuntu -B build/analyzer`
  and the compiler launcher cleared, over the three CI scopes, and
  apply CI's two extractors and its gate-check loop. Verify: the row carries the
  translation-unit count per scope, zero hits on the gate checks, the statement that the
  branch owns no translation unit, and the delta against the base reading for any finding.
- [x] IA-END-002.5 [bench] Run the whole consumer-release job in both trees. Verify: the
  row carries each step's expected exit - non-zero for `dbc_trap_fixture`, 1 for
  `dbc_test`, non-zero for the `overrun` mode of `counters_trap_fixture` and 0 for its
  nine other modes - beside the cache asserts and the archive symbol check, and names exit
  124 as a timeout and never as a pass.
- [x] IA-END-002.6 [read] Count the README lines and the `LCOV_EXCL` markers. Verify:
  1,004 and 84 are the caps and both counts stand at or under them, each with the command
  that counted it.
- [x] IA-END-002.7 [bench] Install the plain tree with `cmake --install build --config Release --prefix prefix`
  and run the twelve install audits and the standalone example link-manifest audit. Verify: each audit stands with its own exit and none of them reads a
  timeout kill as a pass.
- [x] IA-END-002.8 [read] Run `python3 tools/pmu_events/update_pmu_events.py --check` and
  `bash tools/ci/host_record_gate.sh build/test/host_record_check . validation`. Verify:
  both commands stand with their exits and the output each printed, and the entry count of
  the suite comes from `ctest -N` and not from a hand count.
- [x] IA-END-003.1 [write] Write `specs/048-hwlat-probe/results/acceptance-final-intel.md`.
  Verify: the record names the host, cpu model, kernel, both compilers, the gcov and lcov
  versions, the source base sha and the measured head sha, and whether a root session
  stood available; every path stands build-relative or `$HOME`-relative; no row carries a
  completion mark without output; no row repeats `SC-011`; the record names T054 to T056
  unrun.
- [x] IA-END-003.2 [write] Write the matching records under specs/046 and specs/041.
  Verify: both records carry the same identity fields as the hwlat record, every hunter
  row names the run behind its figure, no counter row prints a raw config word, and
  neither record repeats `SC-011`.
- [x] IA-END-003.3 [write] Correct the quickstart sentence that names two records that do
  not exist, and state in the record that the two standing `SC-011` citations at
  `specs/048-hwlat-probe/results/gates.md` and `specs/048-hwlat-probe/tasks.md` stay as
  historical text. Verify: after the correction, `grep -n "results/"
  specs/048-hwlat-probe/quickstart.md` names only files that exist in that directory.
- [x] IA-END-003.4.1 [write] Squash to one commit, push with `git push -u origin
  docs/intel-final-acceptance`, and check the remote sha. Verify: `git ls-remote origin
  refs/heads/docs/intel-final-acceptance` equals `git rev-parse HEAD`.
- [x] IA-END-003.4.2 [write] Open one pull request with the G-20 body. Verify: the title
  stands at or under 50 characters, body lines at or under 72, and the footer block stands
  in order: `Refs: specs/048-hwlat-probe`, `Refs: specs/046-hunter-topdown`, `Refs:
  specs/041-counter-probe`, `Approved-by: OmO`.
- [x] IA-END-003.5 [write] After any records-only amend, disclose both shas and the exact
  diff, re-run the rows that can run, put the final counts in the body, and carry the
  squashed head sha in each record's identity block as the trailing amendment of that
  records-only amend. Verify: the body names the measured head and no row quotes a sha as
  evidence of its own commit.


### Order and concurrency

- Runs together, even beside a sweep, because they are seconds of cpu with no write and
  no timing claim: IA48-001, IA48-002.4, IA48-002.5.1, IA48-002.5.2, IA48-003, IA48-006,
  IA41-001.1, IA41-001.3, IA41-001.4, IA41-002.1, IA41-002.2, IA46-001.3, IA46-002.1 to
  IA46-002.5, IA-END-001.7 once the tracefile exists, IA-END-002.1.1, IA-END-002.1.2,
  IA-END-002.3, IA-END-002.6, IA-END-002.8.
- Stand alone, one at a time, and never beside another sweep or another build:
  IA48-002.1, IA48-002.2, IA48-002.3, IA48-002.6, IA41-001.2, IA46-001.1, IA46-001.2,
  IA-END-001.1 to .6, IA-END-002.1.3, IA-END-002.4, IA-END-002.5, IA-END-002.7.
  IA-END-001.8 stands after the last timed step of every lane.
- Wait for every sweep to exit, and run in this order: IA-END-003.1 to .3, then
  IA-END-002.2.0, then IA-END-002.2.1, then IA-END-003.4.1, then IA-END-002.2.2, then
  IA-END-003.4.2, then IA-END-003.5. A `git add` or a commit is a write the prose fixture can read, so the
  tree-form run stands after the last record write and before the squash.
- Before any timed step, read the load and say what it shows. After any red name, re-run
  that name alone in the same tree and print the load of both runs, so a reader can tell a
  load artifact from a defect.


### Rerun at master `87d745d6` (2026-11-08)

- [x] IA48-007 [bench] Re-measure the unprivileged surface at master `87d745d6`, one run per
  row, each with its load witness. Verify: the record names the same three refusals at exit
  2, the same three jitter figures and the same head.
- [x] IA48-008 [bench] Re-run the coverage chain, both sanitizer lanes, the three suite
  lanes and every gate step at that head. Verify: the record gives each exit, the entry
  counts, and the branch shortfall as rows against the allowance.
- [x] IA48-009 [read] Diagnose every sanitizer red with its ceiling, its plain-tree time and
  its length in the instrumented tree. Verify: the record names the ceiling, both times and
  the ratio for each red name, names the bound that binds the sample entry, and keeps
  hosted readings out of the acceptance rows.
- [x] IA48-010 [write] Correct the guide sentence that says two root runs stand recorded.
  Verify: the guide says no root session stood available, and the record carries the
  operator sequence for the seven reads.
- [x] IA48-011 [bench] Re-run the coverage chain at the merged head `2d4d8c57` and name the
  head each branch reading belongs to. Verify: the record gives the gate exit, the row count,
  the capture, and the two hosted head readings with their run ids.


## Acceptance closeout: measured Intel evidence (2026-11-08)

Branch `fix/intel-acceptance-closeout` at base `2d4d8c57975a86db6c77536f7090aba2bdaf6417`.
The order follows the plan phase of the same date. A row ticks only when its
verify line answers from a run at the head the row names.

- [x] IA48-012 [bench] Print the identity of the base head, then run the
  address-and-undefined lane and the thread lane at that head, each in its own
  clean tree, each alone on an idle host with its load witness printed, and keep
  the original failing run before any isolated diagnostic. IC-001's identity
  fields stand in the first section IA48-016 writes, from this row's log. Verify: the record gives each tree, the option strings, the entry
  count from `ctest -N`, the pass count, the total time, the exit and every red
  name with its printed status.
- [x] IA48-013 [read] Give every red name three runs: the suite run, one isolated
  run in the same tree with the same compiler, options and ceiling, and one run
  bounded to `taskset -c 0,1,2,3` with `-j 4`. Verify: each name carries three
  runs with the child command, the work or event count where the entry prints
  one, the exit, the reason, the grant inside the driving script and the ceiling
  `ctest` applies, and no retry that passes reads as a fix. IC-004 closes here and
  at IA48-015: no defect stands demonstrated inside the owned scope, so this
  closeout lands no code change and no new regression test.
- [x] IA48-014 [read] Price the per-launch device walk with the tracked binaries
  on an idle host, and state the child count of each instrumented entry beside
  the price. Verify: the log gives the exit and the seconds of each timed launch,
  and the counts come from the tracked data and script lines they name.
- [ ] IA48-015 [read] Settle the catalog claim: run the scenario
  `bare-hunt-whole-catalog` of the built test binary and
  `./build/example/first_benchmark --hunt --dry-run`, and print the plan line the
  second answers. Verify: the record quotes the scenario's answer line and the
  plan line with their exits, beside the claim that stands in the
  `The sanitizer diagnosis (IA-END-001.8)` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, the sentence that
  reads `this host offers 497 events`. That file names the range `:543-548` for
  the five hosted entry times, and no claim of this row sits there.
- [ ] IA48-016 [write] Write the dated record sections: the two instrumented
  lanes, the diagnosis table, the coverage before-and-after pair, the corrections
  table and the root-surface rows. Verify: every row names its head, its command,
  its tree and its exit, and every figure comes from a log under `/tmp/ic`.
- [ ] IA48-017 [write] Correct the standing 048 record: the restore sentence at
  `:454`, the residue sentence at `:192`, the three refusal rows at `:414-416`,
  the three home paths at `:218`, `:340` and `:561`, the absent citation at `:54`,
  the operator block at `:437-448`, and the missing six before-and-after rows
  beside `:179-182`. Verify: one grep per correction answers the old text absent
  from the new rows and the replacement present, and no new row prints `/home`.
- [ ] IA48-018 [read] Run the DBC gate route in `build/dev` with the supported
  interpreter, run the unchanged target and the manual gate commands side by side,
  and run `dbc_gate_fixtures` in that tree. Verify: the target, the document gate,
  the pair gate and the fixture entry agree at exit 0, the interface count prints,
  and the row names the interpreter it used.
- [ ] IA48-019 [bench] Run the closeout set at the head the squash produced, in
  clean trees in the plan order: coverage with its hand re-runs before the
  capture, then the two instrumented lanes, then the release lane with its two
  bounded cpu runs, then the gate commands, then the hardware rows, then the three
  static-analysis shards. Verify: every log sits outside the repository, and each
  row names that head and its own witness.
- [ ] IA48-020 [read] Fill the measured figures into the dated rows at that head
  and compare every figure with the run that answered it. Verify: the comparison
  prints no difference, or names the rows that moved.
- [ ] IA48-021 [write] On a difference, amend once, squash, and re-run the whole
  set at the new head. Verify: the second pass answers every row, and a second
  difference stops the lane with both shas and both readings in the report.
- [ ] IA48-022 [read] Run both prose forms, the spell gate, the drift and format
  gates, and the two counts. Verify: the tree form prints its unit count with the
  records staged, the range form covers the commit, and the counts read 1002
  against 1004 and 84 against 84.
- [ ] IA48-023 [write] Land one commit with OmO as author and committer, the title
  `Acceptance: Intel closeout records and matrix rows`, a body wrapped at 72
  columns, and the footer `Refs:` lines for 048, 046 and 041 in that order, then
  `Approved-by: OmO`. Verify: `git log -1` answers the title length, the body
  width, the four trailers and the two names, and `.specify/feature.json` stands
  outside the commit.
- [ ] IA48-024 [read] Push the branch and open one pull request. Verify:
  `git ls-remote origin refs/heads/fix/intel-acceptance-closeout` equals
  `git rev-parse HEAD`, and the body carries the base sha, the final head, the
  count of every lane, the root status, the hardware results, each deferred fix
  with the file that holds it, and every open blocker.

## Intel acceptance closeout at the merged head (2026-11-08)

Branch `fix/intel-acceptance-closeout` on base `a55728af`, the head
`Intel acceptance closeout at the merged head (2026-11-08)` of
`specs/048-hwlat-probe/spec.md` names. `The final-head set` and
`Files this phase may change` of that file bound these rows. Ruling 29 of this
phase's ruling list, `Rulings - this phase (29 to 42)`, splits the work into two
passes: IA48-025 to IA48-038 and IA48-045 to IA48-057 answer Pass 1 at the Pass 1
head, IA48-040 squashes those writes into the candidate head, IA48-041 re-runs
that set there and names both that sha and the final sha the squash after those
record writes produces and the pull request carries, and ruling 35 of that list
keeps every figure on the head whose run printed it. Commands copy from the `Lane Table` of
`specs/048-hwlat-probe/plan.md` and its command blocks, and a load witness
beside a timed step answers ruling 30 of this phase's list. No row changes a file
outside `Files this phase may change`, and no row adds a timeout, a grant, a
suppression or an exclusion. A row that needs a figure names the run that must
answer it. Tags: `[bench]` runs a lane, `[read]` settles a reading, `[write]`
writes records, `[operator]` needs a right this session lacks.

- [ ] IA48-025 [read] The identity reading, the `Identity` row of the
  `Lane Table`, read through block 7 of the command blocks of
  `specs/048-hwlat-probe/plan.md`. IC-001 owns the fields. Run it from the
  checkout root before the first suite, and read the cache loop again after
  each tree configures, because one `CMakeCache.txt` holds the entries of its
  own tree. File: the identity section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, with every log
  under the run's directory outside the repository. Verify: the record prints
  the source sha, the cpu affinity the host allowed, the compiler and the
  launcher of each tree, the interpreter each tree configured, the two kernel
  controls, and the reach of the `msr` node and of tracefs, each beside the
  exit this step answered beside that sha. Runs: alone, before the first
  suite; every other row of this section waits for it.
  ```
  git rev-parse HEAD
  taskset -pc $$
  cat /sys/devices/system/cpu/online
  cat /proc/sys/kernel/perf_event_paranoid
  cat /proc/sys/kernel/nmi_watchdog
  stat -c '%A %a %U:%G %n' /dev/cpu/0/msr /sys/kernel/tracing
  for t in build build/coverage build/sanitize build/tsan build/dev build/analyzer; do
    printf '%s\n' "$t"
    grep -E '^(CMAKE_CXX_COMPILER|CMAKE_CXX_COMPILER_LAUNCHER|Python3_EXECUTABLE):' "$t/CMakeCache.txt"
  done
  ```

- [ ] IA48-026 [bench] Lane 1 of `The final-head set`, the `Coverage chain`
  row, up to its serial suite. IC-001 and IC-002 own it. Wipe the tree that row's
  block names, configure it, build it, zero the counters, and run the suite in
  the serial shape the block writes. The count line copies the form the
  `Entry count` row of the `Lane Table` prints, aimed at this tree, and that
  count answers the coverage suite for the whole phase: the `Release build` row
  wipes `build`, which holds `build/coverage`, so no later row reads this tree.
  Verify: the record names the tree, the entry count `-N` prints for it, the
  pass count, the exit, the elapsed time and the name of every entry that
  crossed its ceiling, each beside its load witness; the count lands in the
  `Coverage: two shapes, two denominators` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, the record row that
  holds this lane's figures, and IA48-045 names this row for it.
  Runs: waits for IA48-025, then runs alone with no other build, suite, capture
  or trace running.
  ```
  rm -rf build/coverage
  cmake --preset=ci-coverage
  cmake --build build/coverage -j "$(nproc)"
  ctest --test-dir build/coverage -N
  lcov --zerocounters -d build/coverage
  ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
  ```

- [ ] IA48-027 [bench] The hand re-runs of the names IA48-026 recorded as cut,
  before any capture step. IC-002 owns them, and the shape cell of the
  `Coverage chain` row puts them between the serial suite and the target, so
  the trace holds them. Ruling 34 of this phase's list gives each cut entry
  its two shapes in one tree at one head: the entry at its own ceiling, then
  the registration run that
  `ctest --test-dir build/coverage --show-only=json-v1 -R '^<name>$'` prints,
  typed with the program, the arguments and the working directory it prints,
  where no CTest limit reaches. The reading lands in the
  `Coverage: two shapes, two denominators` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`. Verify: every cut
  name carries both shapes with its two exits, its two times and the two
  counts ruling 34 of this phase's list prints, and the record keeps the
  clause that a grant an entry enforces inside its driver still binds, the
  clause that ruling writes beside `tools/docs/report_shape.py:368`. Runs:
  waits for IA48-026, runs alone, and IA48-028 waits for it.

- [ ] IA48-028 [bench] The coverage target and the coverage gate as two
  verdicts. IC-002 owns the pair, ruling 3 of the 2026-11-08 closeout list keeps
  the two exits apart, and ruling 32 of this phase's list keeps the picture step
  outside the gate. Run both commands unchanged, then the copy line that ends
  block 1, then the two summary reads the `Coverage denominators` row and the
  `Coverage, cold rows` row name. The rows land in the
  `Coverage: two shapes, two denominators` section and the
  `The coverage target, blocked at its HTML step` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`. Verify: the record prints the target's exit and
  the gate's exit as two rows; the picture step reads BLOCKED with the operator
  action `pacman -S perl-gd gd` and the gate row keeps its own verdict from its
  own run; the numerator, the denominator and the shortfall stand beside the
  denominator that capture prints, which answers `SC-IC-8` for the run that
  printed it, the reading ruling 35 of this phase's list sets; every cold row
  carries what `build/coverage/coverage.raw.info`
  holds and what `build/coverage/coverage.info` holds, the file the gate reads;
  and neither command gained a flag, an ignore, an omit, a remove, an exclusion or
  an allowance; the same four readings at the base head stand beside the
  final-head pair, each named by the run that answered it, because this branch
  changes no compiled file: the file list `git diff --name-only a55728af HEAD`
  prints between those two heads holds markdown lines alone, a markdown line moves
  no compiled file and no test registration, and the record prints that list beside
  the pair, so the candidate head measures the base's code. Runs: waits
  for IA48-027, runs alone, and stands before IA48-032, whose wipe deletes this
  tree.
  ```
  cmake --build build/coverage -t coverage
  bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
  cp -a build/coverage/coverage.info /tmp/ic/final/coverage.info
  ```

- [ ] IA48-029 [bench] Lane 2 of `The final-head set`, the
  `Address and undefined` row. IC-001 owns the sweep. Wipe that block's tree,
  configure it, build it, and run the suite alone with the two option strings
  exactly as block 2 writes them. The row lands in the
  `The two instrumented lanes` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`. Verify: the
  record names the tree, the compiler, the entry count `-N` prints for it, the
  pass count, the exit and every red name with its printed status, and this
  run stands before any isolated diagnostic, as IC-001 asks. Runs: waits for
  IA48-028, because the coverage chain runs first of the four suites, and runs
  alone.
  ```
  rm -rf build/sanitize
  CXX=clang++ cmake --preset=ci-sanitize
  cmake --build build/sanitize -j "$(nproc)"
  ASAN_OPTIONS="strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1" UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j "$(nproc)"
  ```

- [ ] IA48-030 [bench] Lane 3 of `The final-head set`, the `Thread` row.
  IC-001 owns the sweep. Wipe that block's tree, configure it, build it, and run
  the suite alone with the option string exactly as block 3 writes it. The row
  lands in the same `The two instrumented lanes` section. Verify: the record names
  the tree, the compiler, the entry count `-N` prints for it, the pass count, the
  exit and every red name with its printed status, from this run alone. Runs:
  waits for IA48-029, because a suite runs alone, and IA48-032 waits for it,
  because the release wipe deletes this tree.
  ```
  rm -rf build/tsan
  CXX=clang++ cmake --preset=ci-tsan
  cmake --build build/tsan -j "$(nproc)"
  TSAN_OPTIONS="halt_on_error=1:second_deadlock_stack=1" ctest --test-dir build/tsan --output-on-failure --no-tests=error -j "$(nproc)"
  ```

- [ ] IA48-031 [read] The six shapes of the `Address and undefined` row and the
  `Thread` row, and the five red names `IC-004` names with the file that owns
  each. IC-003, IC-004, `SC-IC-9` and `SC-IC-10` own this row. In each tree, in
  that shape order: the suite alone, then each red name alone at its ceiling with
  the same compiler and the same options in the same tree, then that name's
  registration run with no ceiling in its own working directory, then the suite
  under `taskset -c 0,1` with `-j 2`, then the suite under `taskset -c 0,1,2,3`
  with `-j 4`, then the suite a second time in the same tree at the same head.
  The readings land as dated rows beside the standing table of the
  `Three shapes per red name` section and the standing table of the
  `The two instrumented lanes` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, the record rows the
  `Address and undefined` row and the `Thread` row of the `Lane Table` name, and
  those standing tables keep their text and their headings. A correction stands
  only inside `Files this phase may change`, carries a
  regression case that reads red before it and green after it, and keeps the
  behavior that entry tested; a cause that lands outside that list keeps its exact
  reproduction, names its file and stays open, as ruling 36 of this phase's list
  states. Verify: both exits and both elapsed times print for the two suite runs
  of each tree and the two runs report the same set of red names, which answers
  `SC-IC-9`; each of the five names carries the three runs of IC-003 with the six
  fields each; `harness_jitter_cli_test` names
  `test/source/harness_jitter_cli_test.cpp` as closable here; `base_cli_matrix`
  names its runner `test/base_cli/run_matrix.py` and carries its reproduction,
  because `IC-004` puts its cause outside; `harness_hunt_cli_test`, `example_run`
  and `report_shape_fwq_ftq_sample` name the file outside that list that holds
  their deferred change, which answers `SC-IC-10`. No row drops an assertion, skips a
  test, masks a failure, raises a timeout or adds a retry, and a run that passes
  on a retry reads as a diagnostic under ruling 33 of this phase's list. Runs: the
  address shapes wait for IA48-029, the thread shapes wait for IA48-030, and each
  shape runs alone.

- [ ] IA48-032 [bench] Lane 4 of `The final-head set`, the `Release build` row
  and the `Release suite` row: the plain shape. IC-001 owns the tree and the
  suite. The wipe the `Release build` row names deletes every tree under
  `build`, so the copy line of IA48-028 stands done first. Then the configure,
  the build and the suite in the forms those two rows print. The row lands as a
  dated row beside the `Plain, CI form` row of the `The suite lanes` table of
  `Part D - the instrumented lanes and the gates`, and that standing row keeps
  its text and its heading. Verify: the record names
  the compiler and the launcher the identity loop printed for this tree, the
  entry count `-N` prints for it, the pass count, the exit and every red name,
  each beside its load witness. Runs: waits for IA48-029, IA48-030, IA48-031
  and the instrumented leg of IA48-035, because ruling 2 of the 2026-11-08
  closeout list puts the release lane last of the four suites, and runs alone.
  ```
  rm -rf build
  cmake --preset=ci-rocky
  cmake --build build --config Release -j "$(nproc)"
  ctest --test-dir build --output-on-failure --no-tests=error -C Release -j "$(nproc)"
  ```

- [ ] IA48-033 [bench] Lane 5 of `The final-head set`, the `Two-cpu release`
  row: the narrow shape, IC-003's narrow reading of the same tree. The row lands
  as a dated row beside the `Two cpus` row of the `The suite lanes` table, and
  that standing row keeps its text and its heading. Verify: the exit, the pass
  count and every red name print beside the plain reading of IA48-032, and the row
  states whether the mask moved a ceiling crossing; no ceiling moves and no grant
  enters. Runs: waits for IA48-032, because this shape reuses that tree, and runs
  alone.
  ```
  taskset -c 0,1 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 2
  ```

- [ ] IA48-034 [bench] Lane 6 of `The final-head set`, the `Four-cpu release`
  row: the bounded shape, the width a hosted run answers. IC-003 asks the lane to
  print the width the workflow asks for, and `.github/workflows/ci.yml` holds that
  width. The row lands as a dated row beside the `Four cpus` row of the `The
  suite lanes` table, and that standing row keeps its text and its heading.
  Verify: the exit, the pass count and every red name print beside both release
  readings, and the row names the width it read from that workflow file. Runs:
  waits for IA48-033, because this shape reuses the same tree, and runs alone.
  ```
  taskset -c 0,1,2,3 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 4
  ```

- [ ] IA48-035 [bench] Lane 7 of `The final-head set`, the `Walk price` row,
  both legs. IC-004 owns the price. The instrumented leg runs inside the block 2
  lane before the release wipe, and the release leg runs in the release lane, each
  leg alone on an idle host with its load witness printed. The reading lands in the
  walk-price paragraph of the `Three shapes per red name` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`. Verify: the record
  prints the exit and the seconds of each of the three runs per command the row
  names, beside its witness, prints the child
  count of each instrumented entry counted from the tracked lines that hold its
  launches, and states what that count spends against the ceiling that entry cuts,
  a figure this run answers. Runs: the instrumented leg waits for IA48-029 and
  stands before IA48-032, the release leg waits for IA48-032, and the two legs
  never run together.
  ```
  ./build/example/docs_labels --list
  ./build/example/docs_labels --help
  ./build/sanitize/example/docs_labels --list
  ./build/sanitize/example/docs_labels --help
  ./build/sanitize/example/docs_labels --version
  ```

- [ ] IA48-036 [bench] Lane 8 of `The final-head set`, the gate tree: the
  `DBC gates` row, the `Prose, tree` row and the `Prose, range` row. IC-005
  owns the contract target and ruling 22 of the 2026-11-08 closeout list owns
  the interpreter route, which installs nothing and puts no personal path in a
  tracked file. Run block 4 unchanged: the configure, the `dbc-gate` target,
  the document gate, the pair gate and the fixture entry, all in one tree.
  Then both text gates. The readings land as dated rows beside the `DBC
  target`, `DBC doxygen`, `DBC document gate`, `DBC pair gate`, `Prose, tree`
  and `Prose, range` rows of the `The gates` table of
  `Part D - the instrumented lanes and the gates`, and each standing row keeps
  its text and its heading.
  `specs/048-hwlat-probe/results/gates.md` keeps its contract-gate rows
  untouched, cited by reference. Verify: the target, the two manual gates and
  the fixture entry agree at exit 0, the interface count prints, and the row
  names the interpreter the tree configured; the tree form prints its unit
  count with the records visible; the range form covers the commit from the
  base the row names to the head it reads at run time. Runs: the configure
  waits for IA48-032, because this tree stands under `build`; the `dbc-gate`
  target, the two manual gates and the fixture entry run before IA48-039, and
  their exits are figures IA48-039 writes; the tree form waits for that write,
  which is the last record write of Pass 1, so this row stands outside the set
  IA48-039 waits for and its unit count is a figure IA48-041 writes at the
  candidate head, read as the count of the tree that form scanned; the range
  form waits for the final squash inside IA48-041, so its right edge is the
  final sha, because a commit is a write the prose fixture reads; each gate
  runs alone.
  ```
  cmake -S . -B build/dev --preset=ci-ubuntu -D Python3_EXECUTABLE=/usr/bin/python3
  cmake --build build/dev -t dbc-gate -j "$(nproc)"
  doxygen build/dev/dbc-gate/Doxyfile
  /usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dev/dbc-gate/xml --registry tools/dbc/macros.yaml --out build/dev/dbc-gate/doc-matrix.json
  /usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools --registry tools/dbc/macros.yaml --doc-matrix build/dev/dbc-gate/doc-matrix.json --out build/dev/dbc-gate/pair-matrix.json
  ctest --test-dir build/dev -R dbc_gate_fixtures --output-on-failure --no-tests=error -j "$(nproc)"

  cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake
  cmake -D PROSE_MODE=range -D PROSE_BASE=a55728af -D PROSE_HEAD="$(git rev-parse HEAD)" -P cmake/prose-lint.cmake
  ```

- [ ] IA48-037 [bench] Lane 9 of `The final-head set`, the hardware consoles:
  `Hunt, one scenario and one plan line`, `Hunt, walk`, `Hunt, benchmark`,
  `Counter count` and `Counter device`. IH-001, IH-002, IP-001 and IP-002 own the
  readings, and ruling 12 of the 2026-11-08 closeout list asks three normal-method
  consoles per command, each alone with its load witness. The scenario pair settles
  the catalog claim ruling 9 of that list asks about, and it runs in the order that
  row prints. The readings land in the `The ranked rows` and `The level-1 block`
  sections of `specs/046-hunter-topdown/results/acceptance-hunt.md`, in the six
  rows of `Rows the final head must answer` of `specs/046-hunter-topdown/spec.md`,
  and in the `The example, twice` table and the `The two device type files` table
  of `specs/041-counter-probe/results/acceptance-final-intel.md`. Verify: three
  consoles per command each carry their own log path, exit and witness; the level
  line, the reference line and the digest lines print as the binary prints them, at
  the count that run prints, the reading ruling 41 of this phase's list sets; rank
  movement reads among the ranked rows and across every printed row, and the line
  that compares the two binaries carries its own label; the pmu name line and the
  device `type` line print beside the two kernel controls. Runs: waits for
  IA48-032, because these binaries come from that tree, and one console runs at a
  time, alone, as ruling 17 of the 2026-11-08 closeout list asks.
  ```
  ./build/test/harness_hunt_cli_test bare-hunt-whole-catalog
  ./build/example/first_benchmark --hunt --dry-run
  ./build/example/memory_walk --hunt --hunt-budget 120s
  ./build/example/first_benchmark --hunt --hunt-budget 120s
  ./build/example/docs_counter_probe
  cat /sys/devices/cpu/caps/pmu_name
  cat /sys/bus/event_source/devices/cpu/type
  ```

- [ ] IA48-038 [bench] Lane 10 of `The final-head set`, the `Static analysis`
  row, in a fresh tree of its own, the tree the `Assumptions` section of
  `specs/048-hwlat-probe/spec.md` allows. IC-001 owns the build and the
  `Risk and Rollback` section of `specs/048-hwlat-probe/plan.md` owns the
  guard: a scope that builds no translation unit reads void, and its empty
  finding list answers nothing. Verify: the translation-unit count prints for
  each of the three scopes, and both readings land as dated rows beside the
  three `Analyzer` rows of the `The gates` table, and those standing rows keep
  their text and their heading. Runs: waits for IA48-032, because this tree
  stands under `build`, and runs alone.
  ```
  cmake --preset=ci-ubuntu -B build/analyzer -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
  cmake --build build/analyzer --target speedgun-ng_speedgun-ng speedgun-ng_harness -j "$(nproc)"
  cmake --build build/analyzer/test -j "$(nproc)"
  cmake --build build/analyzer/example -j "$(nproc)"
  ```

- [ ] IA48-039 [write] Pass 1 of ruling 29 of this phase's list closes: the
  figures the Pass 1 lanes answered, IA48-025 to IA48-038 and IA48-045 to
  IA48-049 and IA48-051 to IA48-057, land in the three records. The two prose
  forms of IA48-036 stand outside that set, and IA48-041 writes their readings,
  because each form reads lines a record write produces. The records are:
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`,
  `specs/046-hunter-topdown/results/acceptance-hunt.md` and
  `specs/041-counter-probe/results/acceptance-final-intel.md`, each under its
  own dated heading. IC-006 owns the shape: an older line keeps its text and
  its heading while a dated row names the line it replaces, every path prints
  `$HOME`-relative or build-relative, and a log path prints as the run printed
  it, the reading ruling 27 of the 2026-11-08 closeout list sets. These writes
  are a tracked change, so ruling 29 reads them at the squash IA48-040 runs,
  and each figure here stands as a Pass 1 reading until IA48-041 replaces it.
  Verify: each figure names the sha `git rev-parse HEAD` answered for its run,
  the reading ruling 35 of this phase's list asks, no figure here reads as a
  final-head figure, and a search for a home path over the new sections
  answers none. Runs: waits for IA48-025 to IA48-035, IA48-037, IA48-038, the
  `dbc-gate` leg of IA48-036 that runs before its prose forms, IA48-045 to
  IA48-049 and IA48-051 to IA48-057. It waits for neither prose form of
  IA48-036: the tree form reads the lines this row writes, and the range form
  reads the squash IA48-040 lands. The two rows no longer wait on each other.
  This row writes no file beyond the three records named here.

- [ ] IA48-040 [write] The amend and the squash between the two passes, the
  lines IC-006 owns. Compare every figure the three records state with the run
  that answered it, the reading phase 9, `Close the fixed point`, of
  `Phases and Implementation Ordering` of `specs/048-hwlat-probe/plan.md` asks
  for. One difference amends the row, then the squash runs, because any
  tracked change after Pass 1 forces a new squash and a fresh Pass 2 under
  ruling 29 of this phase's list, and that squash produces the candidate head
  IA48-041 runs at. A second difference stops the lane, and the report names
  both shas and both readings. The commit keeps ruling 26 of the 2026-11-08
  closeout list. IA48-058 owns `.specify/feature.json`, the local pointer of
  the spec-driven tooling that stands outside `Files this phase may change`,
  and it is the one row of this phase that touches that file. That row
  restores the value the base head carries before this row commits, so this
  row's commit holds no line for it. Verify: IA48-058's
  `git status --porcelain .specify/feature.json` prints empty,
  `git log --oneline a55728af..HEAD` answers one commit, no tracked file
  outside `Files this phase may change` stands in it, and that commit's sha is
  the candidate sha every Pass 2 row prints. Runs: waits for IA48-039, for
  IA48-036's tree form and for IA48-058, and nothing runs beside it.
  ```
  git status --porcelain .specify/feature.json
  git log --oneline a55728af..HEAD
  ```

- [ ] IA48-041 [bench] Pass 2 of ruling 29 of this phase's list: the same set
  at the candidate head IA48-040 produced, the reading IC-007 asks for. Re-run
  IA48-025 to IA48-038 and IA48-045 to IA48-057 there, in clean trees, in the
  order ruling 2 of the 2026-11-08 closeout list sets, and re-take every
  figure there, because no row of this set may quote a Pass 1 run. Then write
  those figures into the record rows and squash again: that squash produces
  the final head, the head the pull request carries. Every row that carries a
  figure then names both shas, the candidate sha as the head the run stood on
  and the final sha as the head the pull request carries, and prints one proof
  clause, the file list `git diff --name-only <candidate-sha> <final-sha>`
  answers, with the sentence that the list holds markdown lines alone, so a
  markdown line moves no compiled file and no test registration and touches no
  ceiling and no grant. A row that prints a code anchor re-reads that anchor
  in the tree its run used, and the same clause carries that reading, because
  a markdown delta cannot move a code line. Verify: `SC-IC-7` answers from
  this set, so each lane carries a command, an exit and a count a reader can
  rerun at the candidate sha, or reads BLOCKED with the right it lacks;
  `SC-IC-8` answers from the gate run here; `SC-IC-9` answers from the two
  suite runs of each instrumented tree here; `git rev-parse HEAD` equals the
  candidate sha the new rows print and the final sha stands beside it in every
  row that carries a figure; that proof clause answers a markdown-only list
  for each such row; the `Prose, range` form of IA48-036 runs after that
  squash with its right edge at the final sha; and
  `git log --oneline a55728af..HEAD` answers one commit still. Runs: waits for
  IA48-040, each lane inside it runs alone in that order, and the writes and
  the closing squash stand after the last lane.

- [ ] IA48-042 [write] The push, the remote reading and the pull request body.
  IC-007 owns them, ruling 38 of this phase's list owns the lease, ruling 39
  owns the body and ruling 40 owns the one sha. Ruling 38 states that no
  remote branch of this name exists, and the `Branch` row of
  `Identity and binding` of the 2026-11-08 closeout section recorded the
  branch absent on the remote, so the push is a first push and runs the first
  line below. A later push carries the lease of ruling 38, the second line
  below with the full sha that push last observed, and a failed lease stops
  the lane with no retry and no refreshed lease. The third line below reads
  after every push, and IA48-041 names the final sha it must equal. The body
  appends to `specs/046-hunter-topdown/results/pr-body.md` under its own dated
  heading, and the standing body keeps its text and its heading. Verify: the
  remote reading equals the final sha IA48-041 wrote and equals
  `git rev-parse HEAD`, and those two readings answer `SC-IC-11`; the body
  names the final head sha, the candidate sha the figures ran at, the
  markdown-only proof clause of IA48-041, the count per lane, the root status
  and every deferred cause, each line no wider than ruling 39 of this phase's
  list sets; and record, body and remote branch name that one final sha, which
  closes ruling 40. Runs: waits for IA48-041. One pull request opens, nothing
  merges and no issue closes.
  ```
  git push -u origin fix/intel-acceptance-closeout
  git push --force-with-lease=fix/intel-acceptance-closeout:<expected-full-sha>
  git ls-remote origin refs/heads/fix/intel-acceptance-closeout
  git rev-parse HEAD
  ```

- [ ] IA48-043 [operator] The root surfaces, the `Root surfaces` row: block 6
  of `specs/048-hwlat-probe/plan.md`, typed in full in a session an operator
  opens. IC-006 owns the rows and ruling 37 of this phase's list keeps them BLOCKED
  without a privilege. The row stops where `current_tracer` reads other than the
  idle tracer, where `tracing_on` reads on, or where a value reads unreadable, the
  void list of `Risk and Rollback` of that plan. Reason this session cannot type
  it: no privileged session stands in reach, so tracefs and the `msr` node stay out
  of reach, and the phase collects no password and automates no elevation. Restore:
  the probe's own path puts back every setting it saved, the after loop below prints
  the six values again after a normal end and again after the `kill -INT` run, and
  the operator names the before value for any file that still differs. Verify: six
  before-and-after rows beside a seventh row that carries no figure land in the
  `Root-surface rows` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, the reading ruling 24
  of the 2026-11-08 closeout list sets, and every root row either carries both
  values of each file from one privileged run or reads BLOCKED beside this block,
  with the value the host answered cited from the identity section IA48-025 wrote,
  as ruling 42 of this phase's list asks. Runs: one operator session, alone, with
  no other lane running.
  ```
  T=/sys/kernel/tracing
  cat $T/current_tracer; cat $T/tracing_on
  for f in current_tracer tracing_cpumask tracing_thresh \
           hwlat_detector/mode hwlat_detector/width hwlat_detector/window; do
    printf 'before %s %s\n' "$f" \
      "$(sed -e 's/.*\[\([^]]*\)\].*/\1/' $T/$f)"
  done
  ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
    --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=$HOME/hwlat-detector.json
  for f in current_tracer tracing_cpumask tracing_thresh \
           hwlat_detector/mode hwlat_detector/width hwlat_detector/window; do
    printf 'after %s %s\n' "$f" \
      "$(sed -e 's/.*\[\([^]]*\)\].*/\1/' $T/$f)"
  done
  ```

- [ ] IA48-044 [operator] The restricted parity reading, the
  `Parity at paranoid 2` row. IP-002 of `specs/041-counter-probe/spec.md` owns
  it, ruling 11 of the 2026-11-08 closeout list gives parity its two rows, and
  ruling 42 of this phase's list bars a figure here. Operator line: the first
  line below. Read between: the counter example, the `Counter count` command,
  at that control. Restore line: the second line below, the value IP-002 names
  for this host, and the control itself stands at
  `/proc/sys/kernel/perf_event_paranoid`. Reason this session cannot type
  either line: no elevation route stands here, so the row reads BLOCKED and
  cites the identity section IA48-025 wrote for the value the host answered
  before the change, which the operator reads first. Verify: the parity
  reading of `specs/041-counter-probe/results/acceptance-final-intel.md`
  answers the claim `SC-010` owns with the pair of lines below and the
  example's answer at that control, the measured parity row at that host's
  standing value keeps its own figure, and the blocked row carries no figure.
  Runs: an operator session, alone, with no lane running, and the control
  reads back before the next lane starts.
  ```
  sysctl -w kernel.perf_event_paranoid=2
  sysctl -w kernel.perf_event_paranoid=-1
  ./build/example/docs_counter_probe
  ```

### The standing lanes the `Lane Table` names beside the ten

Ruling 20 of the 2026-11-08 closeout list widens the set a closeout repeats to
"every gate command the workflow carries, including the `dbc-gate` target", so
the rows below answer the lane rows the ten lanes above leave out. IC-007 owns
each run and `SC-IC-7` of `Success criteria - this phase` owns each shape,
because every row below answers the Pass 2 set of ruling 29 at the final
head.
Each row takes its command from the lane row it names and prints the record
row or section that holds its exits, and a row that needs a right or a package
this host lacks reads BLOCKED with the operator action and no figure.

- [ ] IA48-045 [read] The `Entry count` row: the count `IC-001` asks of each
  suite, read from the two trees this row can reach, the reading `SC-IC-7`
  shapes. Run the command of that row against each tree it names. Verify: the
  record prints one count per tree beside the exit that tree answered, names
  `build` as the release suite and `build/dev` as the fixture entry, and each
  count comes from this run. The two counts land as dated rows beside the
  `Plain, CI form` row of the `The suite lanes` table and beside the `DBC
  target` row of the `The gates` table of
  `Part D - the instrumented lanes and the gates`, and those standing rows keep
  their text and their heading. The coverage-suite count stands in IA48-026,
  because the `rm -rf build` line of IA48-032 deletes `build/coverage`, so this
  row reads no coverage tree. Runs: waits for IA48-032 and IA48-036, because a
  tree answers only once it configures, stands before IA48-039 and before
  IA48-056, which re-configures `build` out of its release shape, and runs
  alone, the shape that row states.
  ```
  ctest --test-dir build -N
  ctest --test-dir build/dev -N
  ```

- [ ] IA48-046 [bench] The `Lint` row, first command of block 5: the drift
  gate in the bare form the workflow runs, the reading the block 5 note of
  `specs/048-hwlat-probe/plan.md` names. IC-007 owns the run and `SC-IC-7`
  owns the shape. Verify: the reading lands as a dated row beside the `Drift`
  row of the `The gates` table, and that standing row keeps its text and its
  heading; the dated row carries this exit and the reading the gate printed.
  The page cap of the gate stands cited at
  `tools/docs/example_drift.py:256`, the section cap at
  `tools/docs/example_drift.py:257`. Runs: waits for IA48-025 only, stands
  before IA48-039, and runs alone.
  ```
  python3 tools/docs/example_drift.py .
  ```

- [ ] IA48-047 [bench] The `Lint` row, second command of block 5: the same
  drift gate in its four-flag form, the standing step the block 5 note cites
  by its record row. IC-007 owns the run and `SC-IC-7` owns the shape. Verify:
  the reading lands as a dated row beside the `Drift, four flags` row of the
  `The gates` table, and that standing row keeps its text and its heading; the
  dated row carries this exit, and the four tables the flags name, sections,
  names, options and forbidden words, each report under their own heading.
  Runs: waits for IA48-046,
  because one gate reads the same pages, stands before IA48-039, and runs
  alone.
  ```
  python3 tools/docs/example_drift.py --sections --names --options --forbidden .
  ```

- [ ] IA48-048 [bench] The `Lint` row, third command of block 5: the formatter
  gate. IC-007 owns the run and `SC-IC-7` owns the shape. Verify: the reading
  lands as a dated row beside the `Format` row of the `The gates` table, and
  that standing row keeps its text and its heading; the dated row carries this
  exit and the formatter version the run printed, and no file left the gate
  unscanned. Where the pinned formatter
  stands absent this row reads BLOCKED, names the version the tool-setup line at
  `.github/workflows/ci.yml:34` pins, prints that line below as the operator
  action, names the refusal `IC-005` records for this host's two interpreters,
  and carries no figure. Runs: waits for IA48-025, stands before IA48-039, and
  runs alone.
  ```
  cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
  # operator line, ci.yml:34
  pip3 install codespell 'clang-format==18.1.8'
  ```

- [ ] IA48-049 [bench] The `Lint` row, fourth command of block 5: the
  time-source gate. IC-007 owns the run and `SC-IC-7` owns the shape. Verify:
  the reading lands as a dated row beside the `Time source` row of the `The
  gates` table, and that standing row keeps its text and its heading; the
  dated row carries this exit and the finding list the script printed, empty
  or not. Runs: waits for IA48-025,
  stands before IA48-039, and runs alone.
  ```
  bash test/time_source_gate.sh .
  ```

- [ ] IA48-050 [bench] The `Lint` row, fifth command of block 5: the spell
  gate over the written prose, the step phase 10,
  `Run the prose gates and land the commit (XI, VIII)`, of
  `Phases and Implementation Ordering` puts after the record writes. IC-007
  owns the run and `SC-IC-7` owns the shape. Verify: the reading lands as a
  dated row beside the `Spell` row of the `The gates` table, and that standing
  row keeps its text and its heading; the dated row carries this exit and the
  checker version the run printed, and the run stands after the prose this
  phase wrote. Runs: waits for
  IA48-039, runs again inside IA48-041 at the candidate head, and runs alone.
  ```
  cmake -P cmake/spell.cmake
  ```

- [ ] IA48-051 [bench] The `Install and audits` row: block A of the acceptance
  plan of `specs/048-hwlat-probe/plan.md`, the heading
  `Block A - install and the thirteen audits`. IC-007 owns the run and
  `SC-IC-7` owns the shape. Run the install, then the twelve dependency audits
  and the standalone example link-manifest audit in the CI order, each with
  its own exit, the order `.github/workflows/ci.yml:513-540` and `:546-558`
  carries. The writes land in `prefix/`, which `.gitignore` holds, so no
  tracked file changes. Verify: the readings land as dated rows beside the
  `Install` row and beside the `Install audits` row of the `The gates` table,
  and those standing rows keep their text and their heading; the dated rows
  carry the install exit and one exit per audit, and no audit shares an exit
  with another. Runs: waits for IA48-032, because the
  tree it installs stands there, stands before IA48-039 and before IA48-056,
  which re-configures `build`, and runs alone.
  ```
  cmake --install build --config Release --prefix prefix
  ```

- [ ] IA48-052 [bench] The `Host records` row: the host-record gate of the
  test job. IC-007 owns the run and `SC-IC-7` owns the shape. Run the command
  that row prints. Verify: the reading lands as a dated row beside the `Host
  records` row of the `The gates` table, and that standing row keeps its text
  and its heading; the dated row carries this exit and the reading the script
  answered about the standing records. Runs: waits for IA48-032, because the
  binary the command names
  comes from that tree, stands before IA48-039 and before IA48-056, and runs
  alone.
  ```
  bash tools/ci/host_record_gate.sh build/test/host_record_check . validation
  ```

- [ ] IA48-053 [bench] The `pmu tables` row: the vendor table check over the
  vendored tables. IC-007 owns the run and `SC-IC-7` owns the shape. Run the
  command that row prints. Verify: the reading lands as a dated row beside the
  `pmu events` row of the `The gates` table, and that standing row keeps its
  text and its heading; the dated row carries this exit and the state the check
  answered for the tables. Runs: waits for IA48-025, stands before IA48-039,
  and runs alone.
  ```
  python3 tools/pmu_events/update_pmu_events.py --check
  ```

- [ ] IA48-054 [read] The `Two counts` row: the page count and the marker
  count. IC-007 owns the run and `SC-IC-7` owns the shape. Run
  `wc -l README.md`, then the `LCOV_EXCL` sum over `source include` that row
  names, the form its record row prints. Verify: the two counts land as dated
  rows beside the `README` row and beside the `Markers` row of the `The gates`
  table, and those standing rows keep their text and their heading; each dated
  row carries its count at this head, the page cap stands cited at
  `tools/docs/example_drift.py:256`, both caps stand
  beside the `Page and marker caps` row of the `Identity and binding` table of
  `Acceptance closeout: measured Intel evidence (2026-11-08)` of
  `specs/048-hwlat-probe/spec.md`, each standing figure keeps the head whose
  record printed it, and a figure no record names a head for reads `no figure`.
  Runs: waits for IA48-025, stands before IA48-039, and runs alone.
  ```
  wc -l README.md
  git grep -c LCOV_EXCL -- source include
  ```

- [ ] IA48-055 [bench] The `Downstream jobs` row, the shared-audit shape: the
  `shared-audit job` block of `/tmp/ic/w5-gates.md` section 6. IC-007 owns the
  run and `SC-IC-7` owns the shape. Run the audit configure, the build and the
  install into the prefix that block names, then the symbol and dependency
  greps the same block carries,
  `.github/workflows/ci.yml:713-792` among them, each with its own
  exit. Verify: the record prints one exit per grep beside the artifact it
  read, and a copy that drops one grep reads void, the reading the block 4
  note of `specs/048-hwlat-probe/plan.md` sets for a narrowed scan. The exits
  land as a dated row beside the `Install audits` row of the `The gates` table
  of `Part D - the instrumented lanes and the gates`, the standing row that
  holds the purity readings, and that row keeps its text and its heading.
  Runs:
  waits for IA48-038, because the compiler work stays serial, stands before
  IA48-039, and runs alone.
  ```
  cmake --preset=ci-linux-audit -B build-shared -D BUILD_SHARED_LIBS=ON -D CMAKE_BUILD_TYPE=Release   # ci.yml:704
  cmake --build build-shared -j "$(nproc)"                         # ci.yml:708
  cmake --install build-shared --prefix prefix-shared              # ci.yml:712
  ! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -i hwloc          # ci.yml:714
  ! ldd prefix-shared/lib/libspeedgun-ng.so | grep -i hwloc                            # ci.yml:716
  ! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -i simdjson        # ci.yml:718
  ! ldd prefix-shared/lib/libspeedgun-ng.so | grep -i simdjson                         # ci.yml:720
  ! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -i hdr             # ci.yml:722
  ! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep -iE 'zlib|libz|(z_)?(inflate|deflate|compress|uncompress|crc32)'   # ci.yml:727-728
  ! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep '4YAML'            # ci.yml:734
  ! nm -D --defined-only prefix-shared/lib/libspeedgun-ng.so | grep '8fmtquill'        # ci.yml:790
  ```

- [ ] IA48-056 [bench] The `Downstream jobs` row, the downstream-consumer
  shape: the `downstream-consumer job` block of `/tmp/ic/w5-gates.md` section
  6. IC-007 owns the run and `SC-IC-7` owns the shape. Run the audit configure
  of the tree the lane row names, the consumer configure and build, then the
  six purity greps and the catalog comparison block
  `.github/workflows/ci.yml:915-977` and `:979-989` carry. That job assumes
  the host carries a system copy of hwloc, simdjson, HdrHistogram_c, yaml-cpp
  and quill, the three build-and-install steps of
  `.github/workflows/ci.yml:850-880` among them. Where this host lacks one of
  the five, this row reads BLOCKED, names that package, prints the matching
  install line below as the operator action, states that no session here types
  a `sudo` line, and carries no figure; a system install reaches `/usr/local`,
  and this phase holds no restore line for it, so the row names that prefix
  for the operator to check. Verify: each grep carries its own exit beside the
  log pair it read, both logs stand outside the repository, and a BLOCKED
  reading names the package it lacks. The exits land as a dated row beside the
  `Step` table of the `The consumer-release job` section of
  `Part D - the instrumented lanes and the gates`, the record row that holds
  the consumer readings, and that table keeps its text and its heading. Runs:
  waits for IA48-051, because the consumer reads that prefix, and for
  IA48-032, IA48-033, IA48-034, IA48-035, IA48-037, IA48-045, IA48-052 and
  IA48-055, because this shape re-configures `build` with the audit preset and
  holds the compiler slot. This re-configure is the last thing this phase does
  to `build`: every row that reads that tree in its release shape stands
  before it, and IA48-061 writes into the record that the release tree does
  not survive it. This row stands before IA48-039 and runs alone.
  ```
  cmake --preset=ci-linux-audit -B build -D CMAKE_BUILD_TYPE=Release   # ci.yml:884
  cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release | tee consumer-configure.log   # ci.yml:891-894
  cmake --build build-consumer -v | tee consumer-build.log        # ci.yml:898
  test -z "$(grep -i hwloc consumer-configure.log consumer-build.log)"        # ci.yml:979
  test -z "$(grep -i simdjson consumer-configure.log consumer-build.log)"     # ci.yml:981
  test -z "$(grep -i hdr consumer-configure.log consumer-build.log)"          # ci.yml:983
  test -z "$(grep -iE 'zlib|libz' consumer-configure.log consumer-build.log)" # ci.yml:985
  test -z "$(grep -riE 'yaml[-_]?cpp' consumer-configure.log consumer-build.log)"   # ci.yml:987
  test -z "$(grep -riE 'quill' consumer-configure.log consumer-build.log)"    # ci.yml:989

  # operator lines, one per package this host lacks
  sudo cmake --install sys-hdr --prefix /usr/local    # ci.yml:855
  sudo cmake --install sys-yaml --prefix /usr/local   # ci.yml:865
  sudo cmake --install sys-quill --prefix /usr/local  # ci.yml:880
  ```

- [ ] IA48-057 [bench] The `Downstream jobs` row, the consumer-release shape:
  the `consumer-release job` block of `/tmp/ic/w5-gates.md` section 6. IC-007
  owns the run and `SC-IC-7` owns the shape. Configure and build both trees
  that block names, read the three cache asserts, then run the
  ignore-semantics assertions the same block lists: the `check_` symbol scan
  at `.github/workflows/ci.yml:1071-1081`, `dbc_trap_fixture` at `:1099-1117`,
  `counters_trap_fixture` at `:1122-1198`, the `counters_trap` selection at
  `:1204-1205` and `dbc_test` at `:1207-1225`. Verify: the record gives each
  assert its own exit as a dated row beside the `Step` table of the
  `The consumer-release job` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, and that table
  keeps its text and its heading. The run reads a kill at exit `124` as void, the reading G-20 of `Constraints - this extension` and
  the void list of `Risk and Rollback` of `specs/048-hwlat-probe/plan.md` set,
  and no ceiling moved. Runs: waits for IA48-056, which holds the compiler
  slot before it, stands before IA48-039, and runs alone.
  ```
  cmake --preset=ci-linux-ignore -B build/consumer-release -D CMAKE_BUILD_TYPE=Release -D speedgun-ng_DEVELOPER_MODE=OFF   # ci.yml:1048-1052
  grep -F 'CMAKE_BUILD_TYPE:STRING=Release' build/consumer-release/CMakeCache.txt          # ci.yml:1055-1056
  grep -F 'speedgun-ng_DEVELOPER_MODE:BOOL=OFF' build/consumer-release/CMakeCache.txt      # ci.yml:1057-1058
  grep -F 'speedgun-ng_CONTRACTS:STRING=ignore' build/consumer-release/CMakeCache.txt      # ci.yml:1059-1060
  cmake --build build/consumer-release -j "$(nproc)"              # ci.yml:1066
  cmake --preset=ci-linux-ignore -B build/consumer-release-dev -D CMAKE_BUILD_TYPE=Release -D speedgun-ng_DEVELOPER_MODE=ON   # ci.yml:1083-1087
  cmake --build build/consumer-release-dev -j "$(nproc)"          # ci.yml:1092
  ```

### The carriers the 2026-11-09 rulings name (IA48-058 to IA48-063)

`specs/048-hwlat-probe/spec.md` names three of these ids: `IA48-058` under the
`.specify/feature.json` assumption of `Assumptions`, `IA48-059` under the
four-figure table of `Success criteria - this phase (SC-IC-7 to SC-IC-11)`, and
`IA48-061` under rule 2, `One directory per configuration`, of
`Constraints - this extension`. `IA48-060` carries Row 1 of
`Corrections at the merged base (2026-11-09)` of
`specs/048-hwlat-probe/results/acceptance-final-intel.md`, the operator command
cell, and `IA48-059` carries Row 2, the two figures that carry no head label, so
that section holds two correction rows and each one carries an id. `IA48-062`
stands reserved for the spec line that names it, and `IA48-063` stands
unassigned. Row 3 of the 046 block, the stale page anchor beside the 046 spec
clarification, stays outside this phase under ruling 36 of this phase's list,
with its reproduction printed in that record, because both files stand outside
`Files this phase may change`. No `specs/048-hwlat-probe/results/pr-body.md`
stands written: one pull request carries one body, and that body is
`specs/046-hunter-topdown/results/pr-body.md`.

- [ ] IA48-058 [write] The local pointer `.specify/feature.json`, the one
  tracked file of this phase outside `Files this phase may change`, the file the
  `.specify/feature.json` assumption under `Assumptions` of
  `specs/048-hwlat-probe/spec.md` names, and the reading `IC-006` shapes. Read
  the pointer, read the value the base head carries, and restore that value.
  Verify: `git status --porcelain .specify/feature.json` prints empty before
  each commit, `git show a55728af:.specify/feature.json` names
  `specs/045-merge-fixes` while the working file reads `specs/048-hwlat-probe`,
  and one dated line under `Corrections at the merged base (2026-11-09)` of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` records that
  standing modification with its cause: the tooling holds the feature directory
  in use, and this phase works in `specs/048-hwlat-probe`. Runs: waits for
  IA48-039, stands before the commit inside IA48-040, starts no build and no
  suite, and reads no tree, so it runs beside any lane.
  ```
  git status --porcelain .specify/feature.json
  git show a55728af:.specify/feature.json
  git restore --source=a55728af --staged --worktree -- .specify/feature.json
  ```

- [ ] IA48-059 [read] The head labels of every figure row, the duty `SC-IC-7` of
  `Success criteria - this phase (SC-IC-7 to SC-IC-11)` and ruling 29 of
  `Rulings - this phase (29 to 42)` of `specs/048-hwlat-probe/spec.md` state,
  and the reading `IC-007` owns. Read each figure row of the three records
  against the two heads ruling 29 names: the head Pass 2 ran at, and the head
  the pull request carries. Verify: each figure row names both heads and prints
  the markdown-only proof clause of that ruling, that clause runs the diff line
  below and answers files under `specs/` alone, so a delta of markdown files
  moves no figure; the four hand re-runs of the table under the
  `Coverage: two shapes, two denominators` section of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, and the five red
  names under the `Three shapes per red name` section of the same record, each
  carry the head that printed them; a figure with no head beside it reads
  `no figure`. Runs: waits for IA48-041, because the head the pull request
  carries stands after that squash, and reads logs and records alone with no
  lane running beside it.
  ```
  git rev-parse HEAD
  git diff --name-only <head Pass 2 ran at> <head the pull request carries>
  ```

- [ ] IA48-060 [write] The operator command cell of the parity row, the one
  correction row of `Corrections at the merged base (2026-11-09)` of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`. `IC-006` shapes
  the row, `IP-002` of `specs/041-counter-probe/spec.md` owns the parity claim,
  ruling 11 of the 2026-11-08 closeout list gives parity its two rows, and
  ruling 42 of this phase's list bars a figure here. The pair comes from the
  operator block of the `Parity at paranoid 2` row of the `Lane Table` of
  `specs/048-hwlat-probe/plan.md`, the pair IA48-044 prints and IA41-011 lands
  in the 041 record. Verify: the row that opens `The parity row of` under the
  `Root-surface rows` table of
  `Part D - the instrumented lanes and the gates` carries
  `sysctl -w kernel.perf_event_paranoid=2` as its command cell with
  `sysctl -w kernel.perf_event_paranoid=-1` beside it as the way back, its
  figure cell keeps `none`, the rest of that row keeps its text, and the
  correction row keeps its text under its own heading. Runs: an operator line
  stands unread in this session, so this row starts no process; one writer holds
  the record at a time, so it shares a sitting with IA48-044 and stands before
  IA48-039.
  ```
  sysctl -w kernel.perf_event_paranoid=2
  sysctl -w kernel.perf_event_paranoid=-1
  ```

- [ ] IA48-061 [write] The record line that the release tree does not survive
  the audit re-configure of `build`, the line rule 2,
  `One directory per configuration`, of `Constraints - this extension` of
  `specs/048-hwlat-probe/spec.md` asks for and IA48-056 runs. Read the cache and
  the entry count of `build` after that re-configure, in the form the
  `Entry count` row of the `Lane Table` of `specs/048-hwlat-probe/plan.md`
  prints. Verify: one dated line stands beside the `Release build` row of the
  `The gates` table of
  `Part D - the instrumented lanes and the gates`, and that standing row keeps
  its text and its heading; the line names IA48-056 as the step that ends the
  release tree, names the settings the cache answers after it, states that the
  release suite answers no further reading from `build` inside this phase, and
  follows every figure that needs the release configuration. Runs: waits for
  IA48-056, lands its Pass 1 line before IA48-039, lands its Pass 2 line inside
  the write of IA48-041, which re-runs both rows at the candidate head, and
  starts no build and no suite of its own.
  ```
  grep -E '^(CMAKE_BUILD_TYPE|CMAKE_CXX_FLAGS|CMAKE_EXE_LINKER_FLAGS):' build/CMakeCache.txt
  ctest --test-dir build -N
  ```

### The id collisions the closeout leaves standing (2026-11-09)

`T049`, `T051`, `T052` and `T053` each answer two rows of this file: the row
under `Phase 7: User Story 5 - the docs teach both surfaces (Priority: P3)
(module M9, wave W4)`, and the row under `Notes on placement` of
`Implementation Strategy`. The base head `a55728af` carries all eight rows, so
this phase rewrites none of them, and no row this closeout writes takes a `T0`
id. Every closeout row carries an `IA48-`, `IA46-`, `IA41-` or `IA-END-` id, and
a reader who meets a `T0` id reads the phase heading beside it.
