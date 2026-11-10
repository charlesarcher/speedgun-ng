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


---

## Phase 7: Intel final acceptance (2026-11-07) - the counter-count part

**Goal**: the Intel non-hybrid row of SC-010 carries figures, or names the right it
lacks. Depends on: a tree built at the measured head.

- [x] IA41-001.1 [read] Record the device list, the device `type` file, the
  `branch-instructions` event file, `perf_event_paranoid`, `nmi_watchdog` and the file
  modes. Verify: each fact names the path it came from and no raw config value stands
  printed.
- [x] IA41-001.2 [bench] Run `./build/example/docs_counter_probe` and keep the console,
  the exit code and the device line. Verify: the row gives the example's own `probe=`
  figure beside the published limit with its source word, states that the library pays no
  open on this host, cites the `.expected` file as another host's reading, and claims no
  match or failure of a byte compare.
- [x] IA41-001.3 [read] State the watchdog rule as the library applies it, exactly once,
  on the decoded figure only. Verify: the record names the no-op and adds no hand
  subtraction.
- [x] IA41-001.4 [read] Cite `specs/041-counter-probe/tools/group_probe.c` and
  `tools/probe_window.c` as the standing shape, and name `group_probe` not run because its
  first argument is a raw config word and its output prints one. Verify: the record runs
  no tool that takes a type word and prints no raw config value.
- [x] IA41-002.1 [read] Mark the parity row blocked: give this host's
  `perf_event_paranoid` reading, the device modes and the root status in the same row, and
  name the raising of that value as needing root. Verify: the row names the missing right,
  carries no figure, and words no `-1` reading as the equal of the standing `2` reading.
- [x] IA41-002.2 [read] Re-state the Arm, Intel hybrid and Zen 4 rows as pending, and
  name any completed row this host contradicts, or none. Verify: no pending row gained a
  status.


### Rerun at master `87d745d6` (2026-11-08)

- [x] IA41-003 [bench] Re-run the counter-count pair at master `87d745d6`. Verify: the
  record gives the limit line twice, the device facts, and the parity row as blocked.


## Acceptance closeout: measured Intel evidence (2026-11-08)

- [ ] IA41-004 [bench] Run `./build/example/docs_counter_probe` three times at the
  closeout head, each console alone with its load witness, and print the limit
  line each answers. Verify: three consoles, three limit lines, and no other
  hardware tool in the row.
- [x] IA41-005 [write] Split the parity row of `results/acceptance-final-intel.md`
  into one measured row at the standing `perf_event_paranoid` value and one row
  BLOCKED for the `2` reading with its control and value, correct the blocked
  reason that names `/dev/cpu/*/msr`, and state the opens and reads as one
  measured and one derived figure. Verify: the blocked row carries no figure, the
  measured row names its run, and the reason reads as the write to
  `/proc/sys/kernel/perf_event_paranoid`.
- [x] IA41-006 [write] Label the history that stands outside this host: the tool
  copy under `/tmp` at `results/requirement-1-shipped-shape.txt:100`, `:107` and
  `:123`, and the kernel and paranoid readings at
  `results/probe-window-trials.txt:1`, `:17-19`. Verify: each line keeps its text
  under its heading, and every new row names the tracked source and the built
  binary, and no new row names a path outside the repository.


## Intel acceptance closeout: counter rows at the final head (2026-11-09)

**Phase**: the counter-count part of the closeout that
`specs/048-hwlat-probe/spec.md` records under `Intel acceptance closeout at the
merged head (2026-11-08)`. IA41-007 to IA41-014 are that part's work list. Every
one of them writes
`specs/041-counter-probe/results/acceptance-final-intel.md`, the record the
`record 041` note under the lane table of `specs/048-hwlat-probe/plan.md` names,
and that file is the one file these rows change.

**Cites**: two ruling lists live in `specs/048-hwlat-probe/spec.md`: the 2026-11-08
closeout list, and this phase's list under `Rulings - this phase (29 to 42)`. A
cite below reads `ruling n of this phase's list` or `ruling n of the closeout
list`, and each one names that file.

**Head**: every figure comes from a Pass 2 run at the final head, the head ruling
29 of this phase's list defines. No row below prints a figure, and each row names
the run that must answer it. A row's lines land under a dated heading in the
record that names this phase and the head its run measured, each console log
stands outside the repository, and a printed path reads `$HOME`-relative or
build-relative (`IC-006` and `IC-007` of `specs/048-hwlat-probe/spec.md`). A log
path prints as the run printed it, the shape `IC-007` states, so it names the
label directory the run driver creates under `/tmp/ic` for its label. Block 1 of
the command blocks of `specs/048-hwlat-probe/plan.md` names a fixed
`/tmp/ic/final` for the copied coverage trace, and that copy line stands apart
from the label directory a run prints.

**Limits**: no row adds a timeout, a grant, a suppression or an exclusion
(`Out of scope for this phase` of `specs/048-hwlat-probe/spec.md`). A row that
needs a right this session lacks reads as an operator row, and it prints the
command, the restore command and the right it lacks (ruling 37 of this phase's
list). Each console runs alone from the checkout root
`$HOME/code/sg042-closeout`, and a reading that shares that idle window carries
the same witness line ruling 30 of this phase's list governs: `cat /proc/loadavg`, the cpu affinity from `taskset -pc $$`, and the
count of other compiler, test, capture and trace processes. Ruling 30 counts
those four classes, and a live process of any one of them voids the figure it
shares the window with. The `Load witness and stops` sentences under `Order of
work` of `specs/048-hwlat-probe/plan.md` count other `ctest` and compiler
processes for the lanes that paragraph introduces, so those two sentences stand
as the narrower witness that paragraph wrote, and ruling 30 governs the reading
here. One writer holds the record at a time, so rows marked parallel share a
sitting and land their lines one after another.

- [ ] IA41-007 [bench] Console 1 of the counter surface: run
  `./build/example/docs_counter_probe`, the command of the `Counter count` row of
  the lane table of `specs/048-hwlat-probe/plan.md`, in the `build` tree, with the
  witness line beside it. Owner: `IP-001` of `specs/041-counter-probe/spec.md`.
  Writes: one console row beside the standing table under `The example, twice
  (IA41-001.2)`, and it answers the duty IA41-004 carries at the final head.
  Verify: the row prints `git rev-parse HEAD` for that tree, the exit code, the
  limit line as that binary printed it, the benchmark line and the witness line,
  and a witness that reports another such process, or a one-minute load average at
  or above the bound of ruling 30 of this phase's list, voids that console and it
  is re-taken alone. Runs: waits for the `build` tree the `Release build` row of
  that lane table builds at the final head, and it runs alone (ruling 17 of the
  closeout list).

- [ ] IA41-008 [bench] Console 2: the same `./build/example/docs_counter_probe`,
  the second of the three normal-method consoles ruling 12 of the closeout list
  asks for. Owner: `IP-001`. Writes: one console row beside the row IA41-007
  lands. Verify: it prints its own exit code, limit line, benchmark line and
  witness line, it carries nothing over from console 1, and an alternative console
  stands used for none of the three (ruling 12 of the closeout list). Runs: waits
  for IA41-010, which reads in the same idle window.

- [ ] IA41-009 [bench] Console 3: the same `./build/example/docs_counter_probe`,
  the third console. Owner: `IP-001`. Writes: one console row beside the rows
  IA41-007 and IA41-008 land. Verify: three console rows stand under the record's
  dated heading, each with its own exit code, limit line, benchmark line and
  witness line, and the three limit lines agree or the record prints the
  difference as three readings of one command. Runs: waits for IA41-008, since one
  console runs at a time.

- [ ] IA41-010 [read] The reading line of that idle window: the two commands of
  the `Counter device` row of that lane table,
  `cat /sys/devices/cpu/caps/pmu_name` then
  `cat /sys/bus/event_source/devices/cpu/type`, beside the two companion readings
  that row names, `cat /proc/sys/kernel/perf_event_paranoid` and
  `cat /proc/sys/kernel/nmi_watchdog`, which block 7 of that plan reads the same
  way. Owner: `IP-001`. Writes: one reading row beside the standing `The two device
  type files (IP-001)` table and the standing `The device and the kernel settings
  (IA41-001.1)` table. Verify: four readings, each beside the command that read it
  and beside the head sha of the pass, and none carried from a standing row. Runs:
  waits for IA41-007, and IA41-008 follows it in that window.

- [ ] IA41-011 [write] The parity repeat at the permissions this host answers: one
  record row that compares the two figures the single limit line of the three
  consoles carries, the published limit with its source word and the example's own
  search figure at `example/docs/counter_probe.cpp:384-388`. Owner: `IP-001`, and
  `SC-003` of `specs/041-counter-probe/spec.md` owns the claim, the way `The parity
  repeat` of `specs/041-counter-probe/plan.md` states it. Writes: one measured
  parity row beside the standing `The parity rows (ruling 11, IP-002)` table.
  Verify: the row names the three consoles and the reading row, names the
  permissions of that pass, reads agree or reads differ, and takes nothing from
  IA41-012 (ruling 11 of the closeout list). Runs: waits for IA41-009 and IA41-010;
  it starts no process, so it is parallel with IA41-013, IA41-012 and IA41-014.

- [ ] IA41-012 [write] The operator row for the restricted reading: one record row
  that reads BLOCKED, carries no figure, and prints the pair
  `sysctl -w kernel.perf_event_paranoid=2` and
  `sysctl -w kernel.perf_event_paranoid=-1`, the two lines `IP-002` of
  `specs/041-counter-probe/spec.md` names, beside the read-first line
  `cat /proc/sys/kernel/perf_event_paranoid` and the identity reading of this pass
  that holds the value the host answered before any change (ruling 42 of this
  phase's list). Owner: `IP-002`, and `SC-010` of that spec owns the claim. Writes:
  one BLOCKED row beside the standing `The operator command pair for the restricted
  reading (2026-11-09)` section, whose pair this row re-answers at the final head.
  Verify: the row names the control `/proc/sys/kernel/perf_event_paranoid`, the
  operator line, the restore line, the identity row it cites, and the right this
  session lacks through the two elevation readings the driver steps
  `f0-elevate-sudo` and `f0-elevate-pkexec` answer, and it prints no
  figure. Runs: waits for IA41-010 for that cite; it starts no process, so it is
  parallel with IA41-011, IA41-013 and IA41-014.

- [ ] IA41-013 [read] The interpreter the contract target runs, read in the gate
  tree: `grep '^Python3_EXECUTABLE' build/dev/CMakeCache.txt`, then that
  interpreter with `-V` and with `-c 'import yaml; print(yaml.__version__)'`, then
  the unchanged `cmake --build build/dev -t dbc-gate -j "$(nproc)"` beside the two
  manual gate readings of block 4 of `specs/048-hwlat-probe/plan.md`. Owner:
  `IC-005` of `specs/048-hwlat-probe/spec.md`. Reads: the two declarations `The
  interpreter the contract target runs` of `specs/041-counter-probe/spec.md` names,
  the `dbc-gate` install step of `.github/workflows/ci.yml` and the refusal that
  answers `dbc gates require PyYAML (install python3-yaml)` in
  `tools/dbc/dbc_gate_common.py`. Writes: one interpreter row under the record's
  dated heading, and that row lands as a dated row beside the `DBC target` row of
  the `The gates` table of
  `Part D - the instrumented lanes and the gates` of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, the record row that
  holds the interpreter the contract target ran, and that standing row keeps its
  text and its heading. Verify: the row prints the interpreter as that cache printed it,
  CMake's own configure line beside it, the version that interpreter answers or no
  version, and that target's exit; where no version answers, the row reads BLOCKED
  with that interpreter named, and the contract-gate rows stay inside
  `specs/048-hwlat-probe/results/gates.md` as a citation this row does not rewrite.
  Runs: waits for the `DBC gates` row of that lane table to configure `build/dev`
  at the final head, and it runs after IA41-009, because that target starts a
  compiler; past that point it is parallel with IA41-011, IA41-012 and IA41-014.

- [ ] IA41-014 [read] The pending hardware rows: restate the four rows `SC-010` of
  `specs/041-counter-probe/spec.md` names, the Arm `armv8_*` row, the Intel hybrid
  row with leaf 0x23 answered, the AMD Zen 4 row, and the Intel non-hybrid
  check-mode equality row. Owner: `SC-010`, and `IA41-002` of that spec bars a
  pending row from a status it did not measure. That owner stands outside this
  phase's own ids: `SC-010` belongs to `specs/041-counter-probe/spec.md`, while
  the ids of this phase run `IC-001` to `IC-007` and `SC-IC-7` to `SC-IC-11` of
  `specs/048-hwlat-probe/spec.md`, so this row answers a criterion of the feature
  spec and says so. Reads: `Boundaries this base keeps`
  of `specs/041-counter-probe/spec.md` and `The pending hardware rows` of
  `specs/041-counter-probe/plan.md`. Writes: one pending-rows table under the
  record's dated heading, and that table lands as a dated row beside
  `The other rows` of `specs/041-counter-probe/results/acceptance-final-intel.md`,
  the record row that holds the pending readings, and that section keeps its
  text and its heading. Verify: the three hostless rows keep the word pending and
  name the host each one needs, the fourth reads no figure and names IA41-007 to
  IA41-009 and IA41-011 as the runs that answer it, and no row moves on a reading
  from another host. Runs: parallel, since it starts no process and needs no run.


### The carriers of the 2026-11-09 correction rows (IA41-015 and IA41-016)

The two 2026-11-09 blocks of `specs/041-counter-probe/results/acceptance-final-intel.md`
carry five rows between them, and no row of IA41-007 to IA41-014 names four of
them. The two rows below carry those four, and each lands a dated line that
names the record row it answers.

- [ ] IA41-015 [write] The four cite rows of `Corrections at the merged base
  (2026-11-09)` of `specs/041-counter-probe/results/acceptance-final-intel.md`:
  the lines at `:113`, `:114`, `:115` and `:116`, which stand under
  `The parity rows`, `Opens and reads`, `History labels` and the closing
  reading of that record's 2026-11-08 corrections, and which cite ruling 11,
  ruling 13, ruling 27 and `SC-IC-5` at ranges that open one line early. Owner:
  `IC-006` with `SC-IC-5` of `specs/048-hwlat-probe/spec.md`, and ruling 41 of
  this phase's list states the duty of a cite that names a ruling. Writes: four
  dated lines under this heading, each naming the record row whose cite it
  re-points, while every line above that corrections table keeps its text, its
  heading and its mark. Verify: the command below answers the four sites, each
  dated line names that file, the 2026-11-08 closeout list and the ruling
  number, or names `SC-IC-5` as that phase's criterion, each prints the head its
  target was read at, and no dated line carries a range into
  `specs/048-hwlat-probe/spec.md`. Runs: it starts no process, so it is parallel
  with IA41-013 and IA41-014; one writer holds that record at a time, so this row
  shares a sitting with them, and it stands before the squash ruling 29 of this
  phase's list asks for.
  ```
  grep -n "specs/048-hwlat-probe/spec.md:[0-9]" specs/041-counter-probe/results/acceptance-final-intel.md
  ```

- [ ] IA41-016 [write] The blocked row the pair section answers: the standing
  row of `The parity rows` of
  `specs/041-counter-probe/results/acceptance-final-intel.md` that names the
  control, the action and the missing right and carries no command line, beside
  the section `The operator command pair for the restricted reading (2026-11-09)`
  of the same record. Owner: `IP-002` of `specs/041-counter-probe/spec.md` asks a
  blocked row for both command lines, and ruling 42 of this phase's list bars a
  figure here. IA41-012 writes the pair row at the final head, and this row
  carries the standing row to that pair. Writes: one dated line under this
  heading beside that standing row, and the standing row keeps its text, its
  heading and its BLOCKED mark. Verify: the line prints the read-first command
  below with the value it answers, the line that sets `2`, the line that sets the
  value back, the right this session lacks through the two elevation readings the
  driver steps `f0-elevate-sudo` and `f0-elevate-pkexec` answer, and the record
  row that holds the pair, and it prints no figure. Runs: it starts no process,
  so it is parallel with IA41-012, IA41-013
  and IA41-014; one writer holds that record at a time, and it stands before the
  squash ruling 29 of this phase's list asks for.
  ```
  cat /proc/sys/kernel/perf_event_paranoid
  ```


## Zen 4 acceptance phase on chilly (2026-11-10) - tasks

The requirements stand in `Zen 4 acceptance phase: the AMD Zen 4 host chilly
(2026-11-10)` of `spec.md`; the shared modules and witness stand in
`specs/046-hunter-topdown/plan.md`; the counter part stands in `Plan: Zen 4
acceptance on chilly (2026-11-10) - the counter-count part` of `plan.md`.
Z4-000 and Z4-001 run under `specs/046-hunter-topdown/tasks.md` and this part
reads their logs.

- [x] Z4-041-001.1 [US5 M3] M3 counter runs: `./build/example/docs_counter_probe`
  three times, each run alone with the witness of the plan; then the device
  reads: device list, `cpu` device `type` with its mode line, the standing
  absence of `caps/pmu_name`, `perf_event_paranoid`, `nmi_watchdog`, and the
  two elevation readings. Writes: `$HOME/z4/p1/probe-<n>.log`,
  `$HOME/z4/p1/devices.log`. Verify: three exits 0; three identical limit
  lines; the device log prints the type and both control values. Runs: after
  the hunter window (Z4-046-001.1) closes; nothing else runs.
- [x] Z4-041-001.2 [US5 M3] The three rows of the record, from the captures: the
  parity row at this host's `paranoid` value (published limit, source word,
  probe answer, agreement), stated as the row of the value this host carries
  and borrowing no figure to the `-1` row (IP-002); the opens and reads row
  with the walk's arithmetic and the example's cites, stated as measured or
  derived; the watchdog row with the one decoded-arm cite and the host's
  `nmi_watchdog` reading.
  Writes: the three rows in `results/acceptance-zen4.md`, read from
  `$HOME/z4/p1/probe-1.log`, `probe-2.log`, `probe-3.log`, `devices.log`, and
  the code cites. Verify: the limit equals the
  probe answer in all three consoles; the arithmetic matches the printed
  answer. Runs: parallel with any quiet-window reading task.
- [x] Z4-041-002.1 [US5 M6] Write `results/acceptance-zen4.md`: identity, device
  and settings, three consoles, parity row, opens and reads row, watchdog row,
  the blocked parity row at `-1` in the IP-002 shape (control, host value,
  set line, restore line, missing right, no figure), the pending rows. Writes:
  the record. Verify: every path starts `$HOME/` or `build/`; the blocked row
  prints both command lines and no figure. Runs: after Z4-041-001.2 and
  Z4-000.4.
- [x] Z4-041-002.2 [US5 M6] The dated measured row beside the Zen 4 pending row
  of this spec names the head sha; the Arm row and the Intel hybrid row keep
  their pending text; the ticks of this file land. Writes: the spec section,
  this file's ticks. Verify: no standing row lost a line; the dated row prints
  the sha. Runs: after Z4-041-002.1.
- [x] Z4-000.7 [US5 M4] The shared rows beside this record: the two p1 suite
  rows of `Z4-000` in `specs/046-hunter-topdown/tasks.md` carry their figures
  into this record's suite section. The p2 re-run rows of `Z4-001` stand in
  the pull-request body of `Z4-001.3`, and enter no record file, so no writing
  follows the one commit. Writes: the record's suite section. Verify: each row
  prints entries, passed, failed, skipped. Runs: after Z4-041-002.1 and
  Z4-000.5, and before Z4-000.6 and Z4-001.1, so the record suite section
  stands inside the one commit.

Converge assessment (2026-11-10):

The assessment reads `spec.md`, `plan.md`, this file,
`results/acceptance-zen4.md`, the pull-request body at `$HOME/z4/pr-body.md`,
and the captures at `$HOME/z4/p1/` and `$HOME/z4/p3/` at the head
`24e379266e6fa49119972ef1000db23effa5d8c5`. It ran no build, no test, and no
benchmark, and every tick of this section stands as it is. Each CV41 row names
the file it touches and carries its severity in the first bracket.

- [x] CV41-1 [P2] `tasks.md` (Z4-041-001.2): its Writes artifact
  `$HOME/z4/p1/counter-figures.txt` stands absent on disk, so the tick names a
  file no pass wrote; the three rows it produced stand in
  `results/acceptance-zen4.md` with their figures.
- [x] CV41-2 [P2] `results/acceptance-zen4.md` (Z4-041-001): the witness line
  counts `ctest|cc1plus|cc1|gcc|g++|as|ninja|make`, the build and test classes
  of `$HOME/z4/p1/driver.sh`, and the record draws the void-window verdict from
  that count; add one line naming the counted classes, cite the driver, and
  state that no capture and no trace process ran in these windows.
- [x] CV41-3 [P3] `results/acceptance-zen4.md` (Z4-001 shared rows): the
  identity, device, and BLOCKED rows cite captures at `1f1f9b20` alone; add one
  dated line for the final-head re-read at
  `$HOME/z4/p3/facts-reread.log` of `24e379266e6fa49119972ef1000db23effa5d8c5`.
- [x] CV41-4 [P3] `results/acceptance-zen4.md` (Z4-041-001): the suite section
  states the standing desktop load and names no standing service; add the
  service line beside each suite row with its witness at
  `$HOME/z4/p3/strata-witness.log`, the pair the pull-request body prints.
- [x] CV41-5 [P3] `prompt.md` at the repository root stands untracked and
  outside the two spec trees this phase may change; keep it out of the records
  commit.
