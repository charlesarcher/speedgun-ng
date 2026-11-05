# Tasks: Counter-count group probe

**Input**: Design documents from `specs/041-counter-probe/`

**Prerequisites**: `spec.md` (FR-001 to FR-076, D-1 to D-10, F-1 to F-5),
`plan.md` (modules M1 to M9, D-3a, the TDD matrix), `contracts/`

**Tests**: Required and first. FR-070 puts each unit in the same commit as the code
it covers, and the unit reads red at `858c55d9` before that code lands. Every task
below either writes a scenario, writes the code a scenario names, or measures a
figure. No code task runs before its scenario exists.

**Base**: `858c55d9` on branch `041-counter-probe`. Record every base reading in
`results/base-readings.md` before any code lands, so a later delta has a left side.

## Format: `[ID] [P?] [Module] Description`

- **[P]**: the task touches files no other task in its wave touches, and no unfinished
  task of another module stands ahead of it. Two tasks that name one file never run
  together, even when they name different functions in it.
- **[Mn]**: the module of `plan.md` that owns the task.
- `Prove at the line` runs after the edit and states the reading the task claims.

## Path conventions

- Counters sources: `source/counters/`, `source/counters/detail/`,
  `source/counters/linux_pmu/`
- Public header: `include/speedgun-ng/counters_system.hpp`
- Harness: `source/harness/`
- Units: `test/source/`, registered in `test/CMakeLists.txt`
- Example: `example/docs/`, registered in `example/CMakeLists.txt`
- Docs: `README.md`, `docs/pages/harness.md`, `docs/schema/report-v1.json`
- Gates: `cmake/lint.cmake`, `cmake/spell.cmake`, `tools/docs/`, `tools/prose/`,
  `tools/dbc/`, `test/time_source_gate.sh`

## No build-list task for a new source

The counters archive globs its tree with `CONFIGURE_DEPENDS`
(`CMakeLists.txt:1035-1039`), so `source/counters/linux_pmu/group_probe.cpp` and
`source/counters/limit_source.cpp` need no listing line. A new unit needs a
`test/CMakeLists.txt` block, and the example needs one `add_docs_example` line. Both
of those blocks are named below, and `test/CMakeLists.txt` is the one file two tasks
share, so they sit in different waves.

## Commit boundaries

Ruling G-23 squashes the whole lane into one commit before the merge, and acceptance
is measured on that head. During the work, each module lands as its own local commit
so a red reading has a left side and a bisect point; the squash is T060.

## Wave plan

```text
Wave 1 (parallel)      M1, M2, M3, M9          disjoint files
Wave 2                 M4                       needs M1, M2, M3
Wave 3 (parallel)      M5, M6                   need M4
Wave 4 (parallel)      M7, M8                   disjoint files
Wave 5                 polish, gates, squash, PR
```
 ---

## Phase 1: Base readings (blocks every module)

- [x] T001 [M9] Write `results/base-readings.md` with the base figures this lane
  compares against, on `858c55d9`: `git describe` and `git rev-parse HEAD`, the version
  line of `./build/example/first_benchmark`, `grep -n 'VERSION 0\.' CMakeLists.txt`,
  the two `SOVERSION` lines (`:143`, `:419`), `grep -c LCOV_EXCL -r source include`,
  `wc -l README.md`, `ctest --test-dir build -N | tail -3`, and
  `cmake --build build --target dbc-gate 2>&1 | tail -2`.
  Prove at the line: the file names 0.31.0, 6, 21, 84, 1001, 181 entries, and
  `347 interfaces, 0 gaps` twice.
- [x] T002 [M9] Run the two suite rows at the base and save both logs:
  `ctest --test-dir build --output-on-failure` and
  `taskset -c 0,1 ctest --test-dir build --output-on-failure`. State the pass,
  fail and skip counts and name every skipped entry with its reason.
  Prove at the line: both rows are green, and the skip reason of
  `harness_topdown_run_test` is in the file. specs/046's AMD acceptance record names
  it, on the branch of pull request 81.
- [x] T003 [M9] Run the four cheap gates at the base and save the readings:
  `cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`,
  `python3 tools/docs/example_drift.py`, and
  `python3 tools/prose/prose_gate.py --check prose --mode tree`.
  Prove at the line: four exit-0 lines in `results/base-readings.md`.
- [x] T004 [M9] Capture the base coverage reading with the lane's own command, so
  T055 compares against a file and not a memory. State the line ratio, the branch
  ratio, the branch shortfall figure and the marker count.
  Prove at the line: `results/coverage-base.txt` exists and names both ratios.

## Phase 2: Wave 1, four modules in parallel

### M1 Group probe core (FR-010 to FR-021, FR-033, FR-034, F-1 to F-5)

- [x] T010 [P] [M1] Write the scenario table first, as a red unit:
  `test/source/counters_group_probe_test.cpp`, frameworkless `main` with one scenario
  per row of the scenario table of `contracts/group-probe.md`, installing a scripted
  `KernelCalls` through `installKernelCalls` behind an RAII restore, and the
  descriptor-count check this lane's copy of the seam-test helper provides. Register
  it in `test/CMakeLists.txt` in the shape of the `counters_counter_limits_test` block
  at `:665-673`. The unit needs no `SKIP_RETURN_CODE`: the scripted seam answers
  every question, so the unit never skips, and a skipping unit would give away
  branch coverage against the fixed allowance T056 reads.
  Scenarios: fits-6-refuses-7 with every open answered 0 (the F-1 shape, so the answer
  must be 6 and never 32); an oversize `EINVAL` refusal; a short `read`; `EACCES` on
  the first open; a size-1 short run; every size to the cap fits; `group_fd` of each
  member names the leader; the attribute words carry `exclude_kernel` and `disabled`
  and clear `pinned`; the descriptor count returns to its start on all five paths.
  Every scripted `KernelCalls` fills all nine fields the way
  `test/source/counters_linux_pmu_seam_test.cpp:236-266` does, because a blank field
  reaches the real call and the identity decode stands on the probe's path. The
  scripted pointers are plain function pointers, so the scripted state lives in
  file-scope globals and `resetFixture()` clears them between scenarios.
  Prove at the line: `cmake --build build --target counters_group_probe_test` fails on
  the missing declarations, which is the red reading. Record it in
  `results/red-readings.md`.
- [x] T011 [P] [M1] Declare the closed enumeration `GroupProbeStatus`, the options
  struct `GroupProbeOptions` and `probeGroupLimit` in
  `source/counters/detail/group_probe.hpp`, in the house shape of a `detail` header:
  `namespace sg::counters::detail`, `///` blocks with `\pre`, `\post` and
  `\invariant`, no bare `bool` parameter (FR-010), and the doc-gate reading a nested
  `detail` namespace stands outside (`tools/dbc/dbc_doc_gate.py:9-12`).
  Prove at the line: the header names no `bool` and no `syscall`.
- [x] T012 [P] [M1] Define `probeGroupLimit` in
  `source/counters/linux_pmu/group_probe.cpp`. The probe opens one group per size through the seam of
  D-7, sizes from 1 to `maxGroupSize`, the fit read exactly as F-3 states it
  (`time_running == time_enabled` with `time_enabled` past zero, from one read after
  the enable call), the stop at the first size that does not fit, no value for a
  permission refusal or a size-1 non-fit or a cap-wide fit, and every descriptor
  closed on every path including the refusal arms (FR-012 to FR-019). No sleep, no
  clock read, no `syscall`, no second seam (FR-011, FR-021).
  Prove at the line: `ctest --test-dir build -R counters_group_probe_test --output-on-failure`
  answers every scenario green.
- [x] T013 [M1] Run the unit under the two-cpu mask and the sanitizer preset, and
  record both counts in `results/red-readings.md` beside the red reading of T010.
  Prove at the line: `taskset -c 0,1 ctest --test-dir build -R counters_group_probe_test`
  is green.

### M2 Device and event rule (FR-030 to FR-034)

- [x] T020 [P] [M2] Add the scenarios to
  `test/source/counters_counter_limits_test.cpp`, before any code: `cpu`, `cpu_core`
  and `cpu_atom` answer yes to the probe-device rule; one `armv8_pmuv3_0` name answers
  yes; `amd_l3_0`, `amd_df_0` and `instructions` answer no; the x86 event answer names
  the branch-instructions event; an `armv8_` name answers no event at this audit point,
  because D-5 names no Arm host and the tree holds no Arm event table (FR-033, the round
  1 addendum); a device with no listed event answers no value (FR-031 to FR-034). Keep the file's style: the seam's `cpuidWords` carries leaf 0
  and leaf 1 only, so each row fills a `CounterLimitRegisters` by hand with `wordOf`
  and `wordsOf` (`test/source/counters_counter_limits_test.cpp:57-92`) and asks the
  pure decode.
  Prove at the line: the build of `counters_counter_limits_test` fails on the two
  missing declarations, and `results/red-readings.md` carries it.
- [x] T021 [P] [M2] Declare and define `isProbeCoreDevice` and `probeEventNameForDevice`
  in `source/counters/linux_pmu/counter_limits.hpp` and `.cpp`, beside
  `limitForDevice` at `:288-317`, with the `armv8_` prefix rule that
  `source/harness/hunt.cpp:44` already spells `kCorePrefix`. Do not change any rule of
  `limitForDevice`, `usableCounters` or `publishMinusOne`
  (`counter_limits.cpp:122-129`, FR-044).
  Prove at the line: the row for `counters_counter_limits_test` is green under
  `ctest --test-dir build -R counters_counter_limits_test --output-on-failure`.

### M3 Source surface (FR-040a, FR-050 to FR-053, D-3a)

- [x] T030 [P] [M3] Write the red unit `test/source/counters_limit_source_test.cpp`
  and register it in `test/CMakeLists.txt` (a hunk below T010's, in the same file, so
  the two never run together): `objectLimitSource` answers `Decode` for a recorded
  decode path, `Probe` for a recorded probe path, `None` for a path no provider
  recorded, and `None` again after the recorder is told the path publishes no limit
  (FR-040a).
  Prove at the line: the build fails on the missing enum and function, and
  `results/red-readings.md` carries it.
- [x] T031 [P] [M3] Declare `enum class LimitSource` and the exported
  `objectLimitSource(std::string_view objectPath) noexcept -> LimitSource` in
  `include/speedgun-ng/counters_system.hpp` beside `programmableCounters()` at `:85`,
  with the `///` block and `\pre`/`\post` the doc gate counts, and define it in the
  new `source/counters/limit_source.cpp` over a process table keyed by object path, the
  shape `programmableLimitTable()` already keeps at `source/counters/system.cpp:187-191`
  and `:305`. Declare the recorder in an internal header and export nothing else. Edit
  no line of `system.cpp` (PC-5).
  Prove at the line: the row for `counters_limit_source_test` is green under
  `ctest --test-dir build -R counters_limit_source_test --output-on-failure`, and
  `cmake --build build --target dbc-gate` answers
  `348 interfaces, 0 gaps`.
- [x] T032 [M3] State the exported-surface reason where the tree keeps it: the ledger
  comment beside the archive `SOVERSION` at `CMakeLists.txt:143` gains one entry naming
  this spec and the one new symbol. Do not change the number here; T051 does.
  Prove at the line: `git diff -- CMakeLists.txt` shows a comment-only hunk.

### M9 Measurement (FR-001 to FR-005), landed with the spec

- [x] T005 [P] [M9] Confirm `tools/group_probe.c` builds with `cc -O2 -Wall -Wextra
  -Wpedantic` and no project header, and that `results/requirement-1-zen5-paranoid2.txt`
  and `results/requirement-1-zen5-paranoid-minus1.txt` carry the host lines, the three
  tables and the descriptor balance of 0.
  Prove at the line: `cc -O2 -Wall -Wextra -Wpedantic -o /tmp/gp
  specs/041-counter-probe/tools/group_probe.c` is silent, and
  `grep -c 'left open at exit: 0' results/requirement-1-*.txt` answers 2.

## Phase 3 (Wave 2): M4 provider wiring, needs M1, M2 and M3

- [x] T040 [M4] Write the red unit `test/source/counters_limit_fill_test.cpp`, before
  the wiring: a decoded figure keeps the watchdog deduction and pays 0 opens (FR-030,
  FR-044); a silent decode publishes the figure the search answered and names `PROBE`
  (FR-031); the same device answers one search across two passes (FR-022); an uncore
  device pays 0 opens (FR-032); a device the event rule names nothing for pays 0 opens
  (FR-034); the watchdog at 1 costs a decoded figure one counter and costs a searched
  figure nothing (FR-020, FR-042); a search that reads no pair answers no value and
  settles for the process (FR-014, FR-018). Each scenario names a different device,
  because one device name carries one search for the life of the process (FR-022). The
  unit takes the decode's figure as an argument, so no row waits on the CPU of the
  runner: a row no CI host can reach is a row no one has tested.
  Prove at the line: the unit fails on the missing header, and
  `results/red-readings.md` carries it.
- [x] T041 [M4] Add `source/counters/detail/limit_fill.hpp` with
  `source/counters/linux_pmu/limit_fill.cpp`: one pass reads the decode first, then the
  device rule and the event rule before any open, then the search once per device per
  process in the shape of `hostCounterLimitRegisters` at `provider.cpp:101-106` (a
  function-local static), and records the finder beside the figure. Leave the watchdog
  deduction on the decoded path alone. Seat the pass in
  `PmuProvider::enumerate` at the `ObjectSeed` fill, and leave `system.cpp` alone
  (PC-5).
  Prove at the line: `ctest --test-dir build -R counters_limit_fill_test` is green,
  and the suite is green with the rewired enumerate.
- [x] T042 [M4] Prove the fill and the read agree: every scenario of that unit asserts
  `objectLimitSource(path)` beside the figure the pass returned, on all five arms
  (FR-040a, FR-043). One arm came out of the first green run as a defect in the pass: a
  pass that seated no figure left an earlier record standing, so the reader named a
  finder beside an absent figure. The pass now records on every arm, and `NONE` erases.
  Prove at the line: the unit is green, and the reading is in
  `results/red-readings.md`.
## Phase 4 (Wave 3): M5 disclosure and M6 example, in parallel, need M4

### M5 Plan text and written record (FR-041, FR-043, FR-045, FR-050 to FR-052)

- [x] T043 [P] [M5] Add the red scenarios. In
  `test/source/harness_pass_plan_test.cpp`: the same count reached as a decoded
  limit and as a probed limit answers one plan from `planPasses` and one settling
  from `settleMethod` (FR-045, FR-041); a small probed count raises the hunt
  refusal a small decoded count raises, and
  `lastRefusalMessage()` answers the pinned bytes of
  `specs/029-hunter-core/contracts/hunt-cli.md` (FR-045). In the unit that captures
  a run's console text (`test/source/harness_limit_disclosure_test.cpp`, registered
  at `test/CMakeLists.txt:851-856`): the run prints the limit line once for a
  scripted probed device, and every line the run printed at the base keeps its bytes
  (FR-050).
  Prove at the line: the unit builds and fails on the missing source word, and
  `results/red-readings.md` carries it.
- [x] T044 [P] [M5] Carry the source beside the limit where the harness gathers
  limits (`source/harness/runner.cpp:1578-1634`, the `sourceLimits` loop), and
  print the one console line FR-050 states for a run whose needed devices include
  a probed limit. No text of the audit point changes a byte: the plan prints
  nothing today (`source/harness/pass_plan.cpp` holds no print), and the pinned
  hunt refusal line of `source/harness/detail/hunt_selection.hpp:71-79` stands.
  Prove at the line: the row for `harness_limit_disclosure_test` is green under
  `ctest --test-dir build -R harness_limit_disclosure_test --output-on-failure`.
- [x] T045 [P] [M5] Add the `counterLimits` run-level record of FR-051: one entry
  per needed device with device, limit and source word. `kSchemaVersion` moves from
  1.14 to 1.15 at `source/harness/json_report.cpp:49`; the writer seats the record in
  the run-level place the `jitter` record already shows at
  `source/harness/json_report.cpp:1154-1175`; the record joins `x-order` and
  `properties` of `docs/schema/report-v1.json` and stays out of `required`; the
  loader answers no entry for a document older than 1.15 and keeps the answer inside
  the harness, out of `include/speedgun-ng/benchmark.hpp`; the seven pinned test
  sites the schema move touches are updated in the same task. Update the key rows of
  `docs/pages/harness.md:1395-1404` here; the N6 prose section is M7's and is a
  different line range.
  Prove at the line: the three rows are green under the row filter
  `ctest --test-dir build -R 'harness_json_report_test|harness_json_schema_test|harness_json_load_test' --output-on-failure`.

### M6 Example (FR-061 to FR-064)

- [x] T050 [P] [M6] Write `example/docs/counter_probe.cpp` and register it with one
  `add_docs_example(counter_probe)` line in `example/CMakeLists.txt`: for each core
  device, print the device name, its limit or `no limit`, and the source word
  `decode`, `probe` or `none`. Where the device carries a decoded limit, print the
  probe figure beside it, which is the check-mode reading of SC-003. On a host that
  refuses every `perf_event_open`, print `no limit` and exit 0, and print no word about
  kernel settings (FR-061, FR-062).
  Prove at the line: `cmake --build build --target docs_counter_probe` builds, and
  `./build/example/docs_counter_probe` exits 0. `example/CMakeLists.txt:46-54` names the
  target and the binary after the docs prefix, so the lane's names carry it.
- [x] T051 [P] [M6] Capture `example/docs/counter_probe.expected` from a real run
  on this host, keep only the stable words inside the marked region, and state in the
  file's own comment which lines a host may change. Run the drift gate and the shape
  guard.
  Prove at the line: `python3 tools/docs/example_drift.py` answers exit 0, and
  `ctest --test-dir build -R 'example_run|example_drift' --output-on-failure` is green.

## Phase 5 (Wave 4): M7 docs and M8 version ledger, in parallel

- [x] T052 [P] [M7] Rewrite the multi-pass section of `docs/pages/harness.md`
  (`:1568` heading, the limit sentence at `:1577-1580`): how a limit is found (the
  decode first, the probe where the decode is silent), what a probe answer means, what
  it costs on the measured host (28 opens inside F-5's band), that the probe takes no
  watchdog counter off, and that a device the decode answers pays nothing (FR-060).
  Copy the host-with-no-limit shape of `:1672-1691`, keep every marked region the drift
  gate reads, and add no forbidden prose class.
  Prove at the line: `python3 tools/docs/example_drift.py` and `python3
  tools/prose/prose_gate.py --check prose --mode tree` answer exit 0.
- [x] T053 [P] [M7] Add at most 3 lines to `README.md`, in an existing section, so the
  page stays at 1004 lines or fewer, or add none and state why in the record (FR-063).
  Prove at the line: `wc -l README.md` answers 1004 or less and
  `python3 tools/docs/example_drift.py` answers exit 0.
- [x] T054 [M8] Move the version ledger: project `VERSION` to the next free minor at
  rebase time (`CMakeLists.txt:64`, 0.32.0 if no lane took it), the counters archive
  `SOVERSION` from 6 to 7 (`:143`) for the one exported symbol of T031, and the harness
  `SOVERSION` left alone at 21 (`:419`) with one comment line naming that decision.
  Record each check against the base figures of T001, including the check that no other
  lane took a number first.
  Prove at the line: `ctest --test-dir build -R soVersionNumbers --output-on-failure`
  is green, and the two version-binding readers pass.

## Phase 6: Polish, gates, acceptance and the pull request

- [x] T055 [Polish] Run the sanitizer and thread-sanitizer rows and record both counts:
  `cmake --preset=ci-sanitize && cmake --build build-sanitize` (or the tree the preset
  names) with its `ctest`, and `cmake --preset=ci-tsan` with its `ctest`. Name every
  failing entry and every sanitizer line.
  Prove at the line: both rows are in `results/acceptance.md` with counts.
- [x] T056 [Polish] Run the coverage row: `cmake --preset=ci-coverage`,
  `cmake --build build/coverage`, the `.gcda` files deleted, `ctest -j 1
  --output-on-failure --no-tests=error` in that tree, then `cmake --build
  build/coverage -t coverage`, whose gate runs
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`. The `genhtml`
  step exits 2 on this host for a host reason `docs/pages/building.md` documents;
  the first four commands carry the verdict. Throw out and never record a reading
  whose suite log lists a `Not Run` entry. State the line ratio, the branch ratio,
  the branch shortfall figure against the allowance of 21 (`:102`, ruling G-8) and
  the marker count against 84 (FR-074). Add no `LCOV_EXCL`, no
  `--omit-lines`, no `--ignore-errors` and no `--remove`; if a line reads dark, fix the
  code or the scenario. The marker stays as it is.
  Prove at the line: the coverage gate answers with both figures, and
  `grep -c LCOV_EXCL -r source include` answers 84 or less.
- [x] T057 [Polish] Run the analyzer row once for the feature with
  `cmake --preset=ci-ubuntu && cmake --build build`, and state the clang-tidy and
  cppcheck readings against the base.
  Prove at the line: the analyzer names no new finding in the files this lane owns.
- [x] T058 [Polish] Run every cheap gate over the whole head, and the suite under the
  two-cpu mask: `cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`, `python3
  tools/docs/example_drift.py`, `python3 tools/prose/prose_gate.py --base
  origin/master`, `cmake --build build --target dbc-gate`, `test/time_source_gate.sh`,
  the full `ctest` row and the `taskset -c 0,1 ctest` row (FR-072, FR-075). If any
  new unit registers `SKIP_RETURN_CODE 2`, its row joins `MASTER_SKIP` in
  `test/tier_labels.sh:81-94` in the same commit, or `test_tier_labels` fails with a
  skip-rule fault; the plan's units script every answer and need no such row.
  Prove at the line: eight exit-0 lines in `results/gates.md`, and the dbc reading names
  348 interfaces with 0 gaps.
- [x] T059 [Polish] Check the two hard greps of the spec: no unit performs a real
  `perf_event_open` (FR-073), and no probe path names a timer or a clock (FR-021).
  Prove at the line: `git grep -l perf_event_open test/source/` names no file this lane
  added, and `git grep -n 'nanosleep\|clock_gettime\|std::this_thread' source/counters/linux_pmu/group_probe.cpp`
  is empty.
- [x] T060 [Polish] Squash the lane into one commit (ruling G-23) with the house
  template: a title of 50 characters or fewer, a why-body wrapped at 72, and the two
  real trailer lines `Refs: specs/041-counter-probe N6-P` and `Approved-by: OmO` as the
  last two lines, with no person name anywhere in the tree (ruling G-17).
  Prove at the line: `git log --oneline origin/master..HEAD` answers one line, and
  `python3 tools/prose/prose_gate.py --base origin/master` answers 0 findings.
- [x] T061 [Polish] Rebase on `origin/master`, read both sides of every conflict, and
  re-run T058 and T056 on the rebased head. Re-check the two numbers of T054 against
  what master now carries, and take the next free value if a lane took one.
  Prove at the line: the rebase left no merge commit, and the gate rows name the new
  head sha.
- [x] T062 [Polish] Write `results/acceptance.md` with every figure the spec promises:
  the requirement-1 table, the check-mode equality on the core device with both figures,
  both `ctest` rows, the sanitizer and tsan counts, the coverage figures with the branch
  shortfall and the marker count, the example output, the pending rows, and the host,
  kernel, compiler and squashed head sha (ruling G-20). Use `$HOME/` or build-relative
  paths.
  Prove at the line: every SC-001 to SC-010 row names a figure or names itself pending.
- [x] T063 [Polish] Push with `git push -u origin 041-counter-probe`, and check
  `git ls-remote origin refs/heads/041-counter-probe` against `git rev-parse HEAD`. A
  later push uses `--force-with-lease=041-counter-probe:<sha last pushed>`; a failed
  lease stops the lane and reports.
  Prove at the line: the two shas are equal.
- [x] T064 [Polish] Open one pull request with the G-20 fields in the body: host, CPU
  family and model, kernel, compiler, the base sha measured, the branch head sha, every
  count, the gates table, the pending rows and the reason one exported symbol moved the
  archive `SOVERSION`. Merge nothing.
  Prove at the line: the pull request number and head sha are in this file's last line.

## Dependencies

- Phase 1 blocks every module: a module's delta needs the base figures of T001 to
  T004, and the coverage delta needs T004's file.
- Phase 2 is the only wave with four independent modules. M1, M2, M3 and M9 own
  disjoint file sets and read no header another module writes in this wave, so four
  workers run them together. Nothing in Phase 3 or later starts before Phase 2 lands,
  because the provider wiring calls the probe, the rule and the recorder.
- T040 and T041 need T012 (the probe), T021 (the rule) and T031 (the recorder).
- Phase 4 needs T041: the plan text and the example both read a source the provider
  records. Within Phase 4, T043 to T045 and T050 to T051 are independent files and run
  together.
- Phase 5 needs Phase 4: T052 documents behaviour the plan text fixes, and T054 moves
  the number T032's comment names. T052, T053 and T054 own disjoint files and run
  together.
- Phase 6 runs after every phase, and its rows state what each machine answered.

Shared files, in the order the phases need them: `test/CMakeLists.txt` (T010
registers, T030 registers below it, disjoint hunks, never the same wave),
`docs/pages/harness.md` (T045 writes the key rows at `:1395-1404`, T052 rewrites the
N6 rows near `:1577`; different ranges, and T052 runs after T045), `CMakeLists.txt`
(T032 writes a comment, T054 moves the numbers, and T054 runs after T032), and
`test/source/counters_pmu_test.cpp` (T040, T042 and T041's proof, one lane).
`source/counters/system.cpp` stands untouched on purpose (PC-5).

## Parallel execution

Four workers run Wave 1 together: one takes M1 (T010 to T013), one takes M2 (T020 to
T021), one takes M3 (T030 to T032), and M9 (T005) is a check anyone can carry. Wave 2
is one worker on one file. Wave 3 runs two workers: M5 (T043 to T045) and M6 (T050 to
T051). Wave 4 runs two workers: M7 (T052, T053) and M8 (T054). Phase 6 is one worker,
because its rows measure one head.

A `[P]` mark states that the task's file set is alone in its wave. Two tasks that name
one file never run together, even when they name different functions of it, and a
worker never runs a task whose module's red reading it has not recorded.

## MVP scope

Modules M1, M2, M3 and M4 alone: the probe, the device rule, the source label and the
provider fill. That cut answers SC-001, SC-002, SC-004 and SC-005, and it changes what
a silent device plans, which is the value the item opens with. The plan text and the
report key (M5), the example (M6) and the docs page (M7) make the figure attributable
and teachable, and they ride the tail of the lane. The version ledger (M8) is not
optional: M3's exported symbol moves the archive number whether or not the docs land.

## Implementation strategy

1. Land Phase 1 and write the base figures down before anything else moves.
2. Land Wave 1 as three red-first pairs: T010 then T012, T020 then T021, T030 then
   T031. Each pair's red reading goes in `results/red-readings.md` with the command.
3. Land M4 (T040 to T042) and read a probed limit through `Object::programmableCounters()`
   in a unit. Nothing reaches the planner before this reading exists.
4. Land M5 and M6 together. Read the plan line of a scripted probe run and the example
   on this host, in that order, because the example's `.expected` is captured from a
   real run.
5. Land M7 and M8, then Phase 6 in one pass: sanitizers, coverage, analyzer, cheap
   gates, the two greps, squash, rebase, acceptance, push, pull request.
6. Stop and report on any of three readings: a branch shortfall above 21, a marker
   count above 84, or a `README.md` above 1004 lines. Each one is a gate the lane owns,
   and none is fixed by a marker.
