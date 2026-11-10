# Analyze Pass 3: specs/051-raw-events (cross-artifact consistency after 7da7e1fd)

**Mode**: read-only. No artifact, source, build, or git state was modified;
this file is the only write.
**Artifacts**: `spec.md`, `plan.md`, `tasks.md` at head `7da7e1fd` on
`051-raw-events`. Tasks stand written against base `0fce6651`, whose source
tree equals the head source tree, so every `path:line` reads the same at
both heads. `research.md`, `data-model.md`, `quickstart.md` and
`checklists/requirements.md` carry supporting text;
`.specify/memory/constitution.md` (2.20.0) carries the gates.
**Method**: the speckit.analyze detection passes over the current text, with
fresh eyes on the pass-2 remediation: requirement inventory, the FR, SC and
US maps in both directions, the physical view against task file ownership in
both directions, the dependency DAG including the T042 edges, citation
resolution, the scope guard, gate wording against the tracked tools, the
counts, and the closure of every pass-2 finding.
**History**: pass 1 found 2 high and 9 lower findings, remediated at
`036b764d`. Pass 2 found 5 medium and 8 low, remediated at `7da7e1fd`. Pass
1's ORD-3, the FR-015 position between FR-007 and FR-008, stands by ruling.

## Findings

| ID | Category | Severity | Location(s) | Summary | Recommendation |
|---|---|---|---|---|---|
| GUARD-2 | Inconsistency / scope guard | MEDIUM | plan.md:232-237 against plan.md:221, plan.md:329-331, tasks.md:69-74, tasks.md:451-460 | The untouched list still ends with "the option file and environment machinery". The same plan opens `source/harness/options.cpp` at `:560-565` in the physical view and greps the new sentence there in step 6, and T019 makes that edit with hunk bounds 560 to 565. The tasks guard names no option file, so the contradiction lives inside the plan. Pass-2 GUARD-1 added three names to the list and left this one. | Replace the entry with the parts that stay: the gather and environment machinery at `options.cpp:1441-1462` and `options.cpp:1631-1653`. Keep T019's row text outside the untouched list. |
| UNMAP-2 | Coverage gap | MEDIUM | tasks.md:759-787 against tasks.md:192-220, 268-295 | Four tasks appear in no map row: T005 and T008 (the two registrations) and T006 and T009 (the two red reads). Pass 2 added the Base-readings row for W1 and left the W2 and W4 work unlisted, while tasks.md:206-219 and 284-295 put both red reads in `results/red-units.md`. | Add one row: `Red units \| T004 to T009 \| the two red reads in results/red-units.md`. |
| MAP-3 | Coverage map drift | MEDIUM | tasks.md:781 against tasks.md:375, 429, 313 | The pass-2 fix put `[US4]` on T015 and T018. The US4 row names T011, T018 and T037: T015 is missing, and T011 carries no `[US4]` tag. The US1, US2 and US3 rows together leave out eleven tasks that carry their tag (T005, T006, T008, T009, T010 to T015, T018). The tags and the rows are two claims about the same relation. | Pick one authority. Either widen each story row to every task carrying its tag, or state in the format section that a story row lists only the tasks holding that story's named checks; then add T015 to the US4 row and tag T011 or drop it from the row. |
| DEP-2 | Dependency gap | MEDIUM | tasks.md:513-524 against tasks.md:451-493, 448-449 | T042 depends on T018 and T023. T019, T020 and T021 (`options.cpp`, `README.md`, `docs/pages/harness.md`) carry no edge into it, yet T042 `git add`s those three files and its verify asserts `git status --porcelain -- source test README.md docs CMakeLists.txt` prints nothing. W6 runs beside W2 through W5, so the graph allows a T042 read with three carriers still uncommitted. T022 already arrives through T023. | Name T019, T020 and T021 in T042's Depends line. |
| GATE-2 | Gate ordering | MEDIUM | tasks.md:602-608 against tasks.md:658-668, 670-679, 694-706, 720-737 | T030 is the lane's only prose read over `specs/051-raw-events`, and it stands before `results/host-acceptance.md` (T035), `results/gates.md` (T036) and `results/pr-body.md` (T038) exist. T040 re-reads format, spell, drift and time-source and leaves prose out; T039 reads the commit message alone. A prose finding in those three records reaches the CI prose job first, while T030 states that a finding gets cleared before T039. | Move T030 after T036 and T038, or add `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events` to T040's re-reads. |
| MAP-4 | Table structure | LOW | tasks.md:776-777 | The Base-readings row ends mid-cell and the next line holds the closing pipe: `  diffs against \|`. Pandoc's GFM reader closes the row at 776 and renders the continuation as a stray row whose only cell reads "diffs against". Every other row of that table stays on one line. | Put the row on one line. |
| COUNT-1 | Count | LOW | tasks.md:675 against tasks.md:677 | T036 lists "the four fast reads" in the gates record. Its Depends line names five legs that open no build tree: T028 format, T029 spell, T030 prose, T031 drift, T032 time-source. The prose leg is the one the sentence omits. | Write "the five fast reads", or name the prose read beside the four. |
| ORD-4 | Placement | LOW | tasks.md:446, 510-513 against plan.md:396, tasks.md:530-531 | T042 sits under "Phase 6: Wave 6 - the docs carriers and the numbers (FR-010, FR-012)", after that phase's independent-test line. The plan's wave table puts step 7a in W7, and the Phase 7 order sentence says T042's commit stands first. Pass-2 PLAN-1 asked for a W7 home, and the plan half of that fix landed. | Move the T042 block to the head of Phase 7, or retitle the Phase 6 heading to name both homes. |
| XI7-1 | Constitution wording | LOW | spec.md:261-262; tasks.md:194-195, 270-271, 521-522, 592-593, 711-712; quickstart.md:62-64 | Eight code spans wrap across a line break: the `listed as scope-refused (FR-023)` line, the two `add_executable` blocks, the `git status --porcelain` command, the clang-format version string, the `Refs:` footer, and the two quickstart refusal lines. Principle XI.7 keeps every code span on one line, so a wrapped command cannot read as prose. | Re-wrap each sentence so a span starts and ends on the same line. |
| WRAP-1 | Wording artifact | LOW | tasks.md:519-520, 577-579, 746-748 | Three hard wraps from `7da7e1fd` leave a one-word line: `Depends` over `on T018 and T023`, `the ctest log` over `names` over `no Not Run entry`, and `Depends on` over `T040`. The sentences survive; each clause breaks in the middle of a phrase. | Re-wrap those three clauses at a phrase boundary. |
| EDGE-1 | Ambiguity | LOW | tasks.md:292 against tasks.md:796, 408 | T009's description carries "This read stands before T016 opens that file" inside the text a reader or tool scans for the dependency clause, while T016 names T009 as a dependency. The same rule reads cleanly in the dependency section at 796. | Keep the rule in the dependency section, and phrase the task note as "T016 follows this read". |
| DOC-1 | Artifact inventory | LOW | plan.md:365-366 against tasks.md:206-219, 284-295, 694-706 | The plan's tree comment for `results/` names the FR-012 and FR-013 readings, the host pair record, the base readings and the commit handle. It omits `results/red-units.md` (T006, T009) and `results/pr-body.md` (T038). | Name both records in the comment. |
| CHK-1 | Stale record | LOW | checklists/requirements.md:34 against results/clarify-pass7.md | The checklist states that the clarify loop ran six passes. `results/clarify-pass7.md` records a seventh pass for this feature and reads the same artifacts. | Change the count to seven. |

## Pass-2 finding closure

| Pass 2 | Status at `7da7e1fd` |
|---|---|
| DEP-1 (MEDIUM) | Closed. Every task from T024 to T037 names T042 in its own Depends line, step 7a exists in the plan's step plan, and `results/commits.md` carries the built-head handle T040 reads. The inbound edges to T042 stay incomplete: DEP-2. |
| PLAN-1 (MEDIUM) | Partly closed. The plan gained step 7a and the W7 wave row. T042 keeps its Phase 6 seat: ORD-4. |
| LANE-1 (MEDIUM) | Closed. T039 squashes the T042 commit plus the records written since, T040 folds its rows back with `git commit --amend --no-edit` and re-reads the one-commit read, and T041 states that no commit follows that amend. |
| BUILD-1 (MEDIUM) | Closed. T005 depends on T003, so the analyzer read of W1 closes the shared `build` tree before W2 registers, and T008 keeps `build/raw-red` for W4. |
| MAP-1 (MEDIUM) | Closed. The FR-013 and FR-014 rows carry T042. |
| SC-1 (LOW) | Closed. The plan's SC map says "the scripted refusal scenarios" and names five scenarios; no count stands in that sentence. |
| MAP-2 (LOW) | Partly closed. SC-004 gained T024 and the FR-015 row gained T004 and T015. The `[US4]` tags arrived without a matching cell in the US4 row: MAP-3. |
| UNMAP-1 (LOW) | Partly closed. The Base-readings row covers T001 to T003. T005, T006, T008 and T009 carry no row: UNMAP-2, and the new row itself breaks: MAP-4. |
| CIT-1 (LOW) | Closed. `tools/dbc/coverage_gate.sh:121-123` is the D-14 message, and the spec's Assumptions paraphrase matches `specs/029-hunter-core/spec.md:666-669`. |
| QS-1 (LOW) | Closed. Both sanitizer legs in the quickstart carry `CXX=clang++`, and the coverage block runs the suite before the coverage target. |
| GUARD-1 (LOW) | Partly closed. `metric_formula.cpp`, `example/` and the existing test files joined the plan list. The option-file entry contradicts the plan's own physical view: GUARD-2. |
| MARK-1 (LOW) | Closed. T042 carries `[O]`, and T023, T035 and T036 carry `[R]`. |
| VERIFY-1 (LOW) | Closed. The `counter` option row spans `options.cpp:560-565`, so T019's hunk bound matches its own claim. |

## Checks that came back clean

1. Requirement inventory. Fifteen FR ids and six SC ids, each with a tasks
   row, a tasks map cell and a named plan-side check; US1 to US4 each own a
   map row. The FR-015 position stays as pass 1 ruled.
2. Maps both ways. No map cell names a task id missing from the task list,
   and every FR, SC and US row label exists in `spec.md`. The plan's two
   scenario tables hold twelve and ten case names, and T004 and T007 restate
   those twenty-two names with matching counts and no added case name.
3. Physical view and file ownership both ways. The plan's thirteen paths each
   have one owning task: `pmu.hpp` T010, `system.cpp` T014, `plan.cpp` T013,
   `provider.cpp` T011, `group_io.cpp` T012, `runner.cpp` T016 and T017,
   `options.cpp` T019, `README.md` T020, `harness.md` T021, `CMakeLists.txt`
   T022, `test/CMakeLists.txt` T005 and T008, and the two new units T004 and
   T007. No task opens a path outside that list and `results/`. T037's
   expected diff is that list plus `specs/`.
4. Dependency graph. With T042 read as depending on T018 and T023, the graph
   over 42 tasks holds no cycle. The shared `build` chain runs
   T001, T003, T005, T006, T010, T015, T016, T017, T018, T024, T033, T034 in
   that order, so one writer holds that tree at a time. T008 and T009 own
   `build/raw-red`; T025, T026 and T027 own `build/sanitize`, `build/tsan`
   and `build/coverage`, the three directories `CMakePresets.json` gives those
   presets, and `ci-rocky`, `ci-ubuntu` and `ci-ubuntu-test` inherit
   `ci-build` at `${sourceDir}/build`. The twelve `[P]` tasks open twelve
   distinct files.
5. Citations. 110 `path:line` citations across the four documents resolve to a
   tracked file and stay inside the file. The 70 distinct ranges were read line
   by line. The load-bearing set holds: `resolveLeafCore` at
   `system.cpp:573-590` with the near-miss arm at `:582-589`, `probedKindTable`
   at `:165-177`, `hunt_selection.hpp:106-131` with the predicate at `:116`,
   `provider.cpp:325-342`, `:258-285`, `:537-557`, `:667-730`, `:722-729`,
   `:797-862`, `:45-64`, `plan.cpp:445-466`, `:474`, `:502-537` with
   `providerIndex`, `:668-692`, `group_io.cpp:153-198` and `:200-225` with
   `exclude_kernel`, `runner.cpp:986-1030` with `:1012-1013` and `:1022-1026`
   and the `return 2` at `:1027`, `:1435-1502`, `:1437-1443`, `:1586-1597`,
   `counters_measurement.hpp:99`, `:103`, `:132-141`, `:607-608`,
   `counters_system.hpp:143-178` with the lookup call at `:147`,
   `counters_core.hpp:87-95`, `:162-184`, `:205-206`, `metric_formula.cpp:1273-1305`
   with the call at `:1290`, `options.cpp:560-565`, `:1441-1462`, `:1631-1653`,
   `report.cpp:32-39`, `:727-748`, `json_report.cpp:58`, `:532-538`,
   `:873-880`, `:881-888`, `:1118-1121`, `cli.cpp:106-128`, `:960-970`,
   `internal.hpp:640`, `options.hpp:250`, `core.hpp:30-39`, `pmu.hpp:278-292`,
   `catalog.cpp:43-60`, `report-v1.json:689-696`, `hunt-cli.md:59` and `:83`
   against `hunt_cli.cpp:640-645`, `hunt.cpp:729-733` and the pin at
   `harness_hunt_cli_test.cpp:394-398`, `README.md:970`, `harness.md:637-651`,
   `:675`, `:2577-2579`, `:2621`, `CMakeLists.txt:78-81`, `:82`, `:167-172`,
   `:453-455`, `:456`, `test/CMakeLists.txt:553-559` and `:1837-1841`,
   `counters_pmu_provider_test.cpp:179-268` with the recorder at `:184` and
   the `withCpumask` arm at `:247`, `harness_cli_test.cpp:729-755` and
   `:769-775`, `harness_limit_disclosure_test.cpp:1-40`, `:206-310`,
   `:288-310`, `harness_gap_test.cpp:1-12`,
   `counters_linux_pmu_seam_test.cpp:50-59` and `:4110-4148`,
   `example_drift.py:256` and `:567-581`, `coverage.cmake:47-59`, `:68-81`,
   `:73`, `:111-135`, `coverage_gate.sh:51-53`, `:108`, `:121-123`, and
   `.github/workflows/ci.yml:34`.
6. Readings re-run on this host. `ctest --test-dir build -N` answers 190, so
   T005's 191 and T008's 192 hold. The marker read
   `git grep -c LCOV_EXCL -- source include` sums 84. `README.md` measures
   1003 lines under `PAGE_LINE_CAP = 1004`. `CMakeLists.txt` reads
   `VERSION 0.34.0` at `:82`, counters `SOVERSION 8` at `:172` and harness
   `SOVERSION 22` at `:456`; `json_report.cpp:58` reads `1.16`; and
   `build/speedgun-ngConfigVersion.cmake` reports `0.34.0`. The host pair
   reads type `4` and `event=0xc0`. `/usr/bin/clang++` exists, and
   `clang-format --version` reads 18.1.8, the version the lint job pins.
7. Gate wording against the tracked tools. `allowance=21` sits at
   `coverage_gate.sh:108`, and the D-14 message stands at `:121-123`. The
   capture names the two error classes at `coverage.cmake:73`, and the
   `inconsistent` class comment stands at `coverage.cmake:47-59`. The gate
   target carries the name `coverage` at `coverage.cmake:150-151`;
   `cmake/lint.cmake` honors `FORMAT_COMMAND`;
   `cmake/prose-lint.cmake` honors `PROSE_MODE`, `PROSE_BASE` and
   `PROSE_CHECK`; `prose_gate.py` accepts `--check prose --mode tree --paths`,
   and reserves a usage error for `--check commit` with either;
   `test/time_source_gate.sh` documents one argument, `<repo-root>`;
   `example_drift.py` scans `USER_GUIDE_PAGES`, the two User-guide pages, and
   applies `PAGE_LINE_CAP` to the main page; and T026's tsan clause matches
   `constitution.md:801-803`, which names any gate exception a P2 under
   Principle I. `build/raw-red` sits under `build/`, which the `.codespellrc`
   skip list covers through `*/build`.
8. Scope guard both ways. The guard and the plan list agree on every name
   except the one in GUARD-2. `benchmark.hpp` lives at
   `include/speedgun-ng/benchmark.hpp`, so the plan's extra name sits inside
   the guard's `include/` entry. T037's three reads cover every guard entry,
   including the existing-test clause through `test/source`.
9. Counts. 42 task ids, T001 to T042, with no gap and no duplicate; twelve
   `[P]`, ten `[R]`, four `[O]`; seven waves in the plan table and seven
   phases in tasks; `0.34.0` at the base and `0.35.0` at the head, with schema
   `1.16` and SOVERSION `22` and `8` stated the same way in spec, plan, tasks,
   quickstart and research.

**Constitution alignment issues**: none. Principle VIII's gates all appear in
W7 with no weakened configuration, III's TDD order holds in W2 and W4, X.3
appears as the guard and T037, X.4 as the step and check plan, and XI.6's
pinned quotations stay exempt text. Two inherited tensions read as the tracked
tools state: VI's 100 percent branch line against the recorded
`allowance=21`, and VII's baseline duty against the constitution's own open
deferral (`constitution.md:463-465`, plan.md:102-110, research R-15).

**Unmapped tasks**: T005, T006, T008, T009 (UNMAP-2). Every other task carries
a tag, and all but the four in MAP-3 land in a row for that tag.

**Metrics**

- Total requirements: 15 FR, 6 SC, plus US1 to US4
- Total tasks: 42 (T001 to T042; T042 sits after T023 in the file)
- Coverage: every FR, SC and US id carries tasks and a named check; 4 tasks
  carry no row
- Ambiguity count: 1 (EDGE-1)
- Duplication count: 0; the two shape recognizers are one contract in two
  places, recorded and priced in research R-01
- Critical issues: 0
- High: 0, Medium: 5, Low: 8

## Next actions

Nothing blocks `/speckit-implement`: a careful reader executes the lane as
written. Five MEDIUM findings each cost one line or one block:

1. DEP-2 and ORD-4: give T042 the three carrier edges and move it under
   Phase 7, so the commit that names the built head also holds every file it
   stages.
2. UNMAP-2 and MAP-3: one new row, one cell, and one decision about whether
   tags or rows carry the story mapping.
3. GATE-2: order T030 after the records it must read, or re-read prose in
   T040.
4. GUARD-2: one plan cell, so the plan stops listing as untouched the file its
   own physical view edits.

The eight LOW findings are the broken table row, the fast-read count, the
wrapped code spans and hard-wrapped clauses, the T009 note, the `results/`
inventory comment, and the stale clarify pass count.

Remediation was not applied: no artifact, source, or git state was edited. Any
subset can be drafted on request.
