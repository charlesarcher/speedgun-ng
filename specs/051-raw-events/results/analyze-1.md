# Analyze Pass 1: specs/051-raw-events (cross-artifact consistency)

**Mode**: read-only. No artifact, source, or git state was modified; this file is
the only write.
**Artifacts**: `spec.md`, `plan.md`, `tasks.md` at head `f40da7ab` (tasks written
against base `0fce6651`), plus `research.md`, `data-model.md`, `quickstart.md` as
supporting texts; `.specify/memory/constitution.md` (v2.20.0 line references).
**Method**: speckit.analyze detection passes: requirement inventory, task
coverage mapping, physical-view/file-ownership matching, dependency-graph
acyclicity, `[P]` disjointedness, citation resolution against tracked files,
standing-gate wording, and count agreement.

## Findings

| ID | Category | Severity | Location(s) | Summary | Recommendation |
|---|---|---|---|---|---|
| INC-1 | Inconsistency / parallel safety | HIGH | plan.md:370-371, plan.md:383-385; tasks.md:50-52, tasks.md:776-778 | The plan says the wave file lists make concurrency safe and that "W4 may start beside W2 and W3", and the tasks state the build discipline "one build there at a time" for the single `build` tree, yet W2 (T005, T006), W4 (T008, T009) and W3 (T010, T015) each run `cmake --build build -j 10`. No dependency serializes them: T009 (deps T006, T008) runs beside T010 and T015, and T008 runs beside T006. Two CMake configure/build passes over one `binaryDir` corrupt the red/green verdicts the TDD steps depend on. | Add an explicit edge or note: the `build` tree is a serialized resource like `test/CMakeLists.txt` (e.g. T009 after T015, or W4 registers without building until W3's checkpoint T015). The `[P]`-only statement at tasks.md:785-786 does not cover this pair. |
| INC-2 | Inconsistency / lane record | HIGH | tasks.md:679-689 (T039), tasks.md:691-703 (T040), tasks.md:765-770 | No task commits before T039, so at T024 the built head is `0fce6651` with a dirty tree. T040's carrying read is `git diff --name-only <built-head> HEAD` with `<built-head>` = "the head the T024 build ran at", and its verify demands that the read "lists only paths under `specs/051-raw-events/`". Under the written sequence that read lists every lane source file, so the check cannot pass; T039's "squash the lane to one commit" and `git log --oneline 0fce6651..HEAD | wc -l` = 1 likewise presuppose commits the tasks never create. | Name the commit point (commit the lane work before the W7 gates, then squash), or change T040 to a tree read that does not presuppose a commit (`git status --porcelain` empty plus the recorded head). Without one of these the FR-013 "carries every T024 to T034 verdict to the squashed head" claim has no check behind it. |
| COV-1 | Coverage gap | MEDIUM | plan.md:284-301 (FR map), plan.md (no SC-005, SC-006); tasks.md:749-750 | Every FR has a plan-side named check, and the tasks map covers SC-001..SC-006, but `SC-005` and `SC-006` never appear in plan.md: the plan's map is FR-only, so the coverage criterion and the numbers criterion have no plan-side check. The work exists in plan step 7 and step 8 prose; it is not mapped to the criterion ids. | Extend the plan's map with the six SC ids, or add the SC ids to steps 7 and 8 so the plan states which read closes SC-005 and SC-006. |
| COV-2 | Coverage gap / requirement text | MEDIUM | spec.md:231-232 (FR-015); plan.md:216-218 and 154-156 (physical view, logical view step 5); data-model.md E-3 ("Readers, and only these"); tasks.md:340-352 (T012) | FR-015 authorizes two readers of the raw-entry table — "the core lookup and ... the plan's leaf-record read" — while the plan, the data model, and T012 make the provider window read (`group_io resolve`, group_io.cpp:178-187) a third reader. D-5 (spec.md:375) does name the measurement path, so the spec contradicts itself and the normative FR is narrower than the planned change. | Widen FR-015's reader clause to name the window read, matching D-5, data-model E-3, and T012. |
| INC-3 | Count disagreement | MEDIUM | tasks.md:654-656 (T037); plan.md:196-230 (physical view), plan.md:360-362 | T037's scope read expects "the six source files"; the plan's physical view and the tasks' own file opens name seven source files (five counters: `pmu.hpp`, `system.cpp`, `plan.cpp`, `provider.cpp`, `group_io.cpp`; two harness: `runner.cpp`, `options.cpp`). The plan's own Scale/Scope line says "five counters files, two harness files". | Change T037 to "seven source files", or name the seven paths in the read so the scope check is checkable rather than near-miss. |
| ORD-1 | Ordering / verify precondition | MEDIUM | tasks.md:369-374 (T015), tasks.md:777-778 | T015's verify includes "`git diff --name-only 0fce6651 -- test/source` lists exactly the two new units", but T015 depends only on T012, T013, T014. The harness unit (`test/source/harness_raw_events_test.cpp`) is written by T007 in W4, which runs beside W3, so the checkpoint read can fail with one unit present while everything it checks is green. | Make T015's diff read conditional on W4, or scope it to `test/source/counters_raw_event_test.cpp`; alternatively add the T007 dependency the read already assumes. |
| AMB-1 | Underspecification / placeholder | MEDIUM | tasks.md:328 (T011 verify), tasks.md:305-311 (T010), plan.md:199-201; against tasks.md:46 | T011's verify is `grep -c -F '<synthesis entry point of T010>' ...`: an unresolved placeholder. Neither T010, the plan's physical view, nor the data model names the counters-side synthesis entry point or the counters raw-shape predicate (only the untouched harness predicate `hasRawCodeShape` is named), so two tasks must invent the same identifier. tasks.md:46 promises exact verify commands. | Name both identifiers in T010 (one line each) and substitute the name into T011's grep. |
| CIT-1 | Citation error | MEDIUM | tasks.md:180; plan.md:236-238 | T004 cites the private-seam include pattern at `test/source/counters_linux_pmu_seam_test.cpp:11-15`; that range is the header comment about the one test that reaches a private seam and a coverage exclusion. The private-seam include block is at `:50-59`, which the plan cites correctly ("private seams on the `counters_linux_pmu_seam_test.cpp:50-59` include pattern"). | Point T004 at `:50-59` (or cite `:14-18` for the policy sentence and `:50-59` for the includes). |
| ORD-2 | Ordering wording | LOW | tasks.md:26-29; tasks.md:76, 140, 221, 296, 381, 435, 504 | The header states "the task numbers follow the same order" after listing W1..W7, but the phases run W1, W2, W4, W3, W5, W6, W7 (Phase 3 = Wave 4, Phase 4 = Wave 3). The wave-level dependency text is right; only the order sentence misleads. | Restate the sentence: phases group by wave, and the W4 red unit precedes the W3 layers in task numbering. |
| ORD-3 | Numbering wording | LOW | spec.md:216-235 | The FR list runs FR-001..FR-007, then FR-015 (spec.md:223-234), then FR-008..FR-014: the newest id sits mid-list. Both plan and tasks map FR-015 correctly, so the effect is reading order only. | Leave the id, or renumber in a spec edit if the list order matters to the reader; do not renumber in tasks/plan (that would break the two maps). |
| GATE-1 | Gate wording | LOW | tasks.md:529-545 (T026); spec.md FR-013; constitution VIII:745-749, 801-803 | T026 passes its leg when "the only failing entry is `harness_context_barrier_test`" and an alone re-read exits 0. Principle VIII lists "All tests pass (`ctest`)" and thread-sanitizer-clean as hard gates and requires "Any exception is a P2 (I) with written justification"; the task gives the justification but not the P2 framing. The flake itself is a tracked, recorded fact (specs/030 results, specs/039 verdict), and the thread-sanitizer condition that VIII names (no `WARNING: ThreadSanitizer`) is still checked. | Either state the recorded flake as the exception with its justification in the task and in `results/gates.md`, or drop the branch and re-run the leg. |

## Checks that came back clean

1. Requirement coverage. spec.md carries 15 functional requirements (FR-001 to
   FR-015) and 6 success criteria (SC-001 to SC-006). Each FR appears in plan.md's
   FR map (plan.md:284-301) with a named scenario or read, and in tasks.md's FR to
   task map (tasks.md:722-750) with a named check; each SC appears in the tasks
   map. FR-023 and FR-090 in spec.md are other features' ids quoted in standing
   line text; they are not this feature's requirements.
2. Physical view. The 13 edit targets of plan.md:196-230 are each opened by
   exactly one task group, and no task opens a file the plan omits. The two other
   paths inside the physical-view block (`hunt_selection.hpp`,
   `tools/docs/example_drift.py`) are read-only references, which the scope guard
   at tasks.md:65-72 keeps that way. The `results/` records that tasks create are
   authorized by plan.md:351 and the wave table at plan.md:375-381.
3. Dependency graph. 41 tasks (T001-T041), 0 cycles, no dependency naming an
   unknown task, no edge that points from an earlier task to a later-numbered one.
4. `[P]` markers. 12 tasks carry `[P]` (T004, T007, T011, T012, T013, T019-T022,
   T025-T027). Each opens a file no other `[P]` task opens; the three analyzer-free
   `[P]` author tasks (T004, T007) do not build, the counters `[P]` trio (T011,
   T012, T013) open disjoint files under one dependency (T010), and the three `[P]`
   gate tasks own distinct trees (`build/sanitize`, `build/tsan`, `build/coverage`).
   Both `test/CMakeLists.txt` registrations are non-`[P]` and serialized
   (tasks.md:34-35, tasks.md:756-758). INC-1 is the one parallel claim that does
   not hold.
5. Forbidden surfaces. `.github/`, `include/`, `docs/schema/`,
   `hunt_selection.hpp`, `hunt_cli.cpp`, `hunt.cpp`, `report.cpp`,
   `json_report.cpp`, `json_load.cpp`, `catalog.cpp`,
   `metric_formula.cpp`, `example/`, and existing test files appear in tasks only
   as guard text or as `git diff` reads. No task edits an existing test
   expectation; the counter description change at options.cpp:564 is pinned
   nowhere (the tracked occurrences are README.md:970, docs/pages/harness.md:675
   and options.cpp:564, and `example_drift.py` compares option names only, at
   :798-808).
6. Standing gate wording. `--ignore-errors mismatch,negative` at
   `cmake/coverage.cmake:73` with the `inconsistent` class recorded gone at
   :47-59; `allowance=21` at `tools/dbc/coverage_gate.sh:108` and the `100.0%`
   line arm at :51-54; the marker read sums 84 at this head; the page reads 1003
   against `PAGE_LINE_CAP = 1004` at `tools/docs/example_drift.py:256`. The
   artifacts' wording matches all of it.
7. Counts and numbers. Base `VERSION 0.34.0` at CMakeLists.txt:82, counters
   `SOVERSION 8` at :172, harness `SOVERSION 22` at :456, `kSchemaVersion =
   "1.16"` at json_report.cpp:58, registered entries `Total Tests: 190` in the
   existing tree (tasks.md:62 claims 190, rising to 192 with T005 and T008),
   host figures AuthenticAMD / family 26 / model 68 / type 4 / `event=0xc0` read
   as the artifacts state, and `build/speedgun-ngConfigVersion.cmake` exists for
   the T002 read. Target 0.35.0, schema 1.16, SOVERSION 8 and 22, and the cap 1004
   agree across spec.md D-6/SC-006, plan.md, research.md R-12/R-13, tasks.md
   T002/T022/T023 and quickstart.md.
8. Citations. 166 distinct `path:line` or `path:start-end` citations across the
   six artifacts resolve to tracked files with every range inside the file. About
   110 of them were content-checked against the named symbol or line
   (`resolveLeafCore` at system.cpp:573-590 and declared at
   counters_measurement.hpp:607-608; `pmuProbe` provider.cpp:667-730; the errno
   table :722-729; `scopeSettledState` :537-557; `counterAddressFailure`
   runner.cpp:986-1030 with the three-field print at :1022-1026 and `return 2` at
   :1027; the column loop :1435-1502; `metricLabels` :1586-1597; json_report.cpp
   :532-538 and :873-880; `compileCore` plan.cpp:402-488, tree scan :445-466, gate
   :474, `availabilityGatePasses` :668-692, grouping :502-537; `resolve`
   group_io.cpp:153-198 and `fillAttr` :200-225; `probedKindTable`
   system.cpp:165-177; `PmuEntry`/probed-kinds pmu.hpp:259-292; the
   `harness_hunt_cli_test.cpp:394-398` pin against
   `specs/029-hunter-core/contracts/hunt-cli.md:59,83` and specs/029 FR-005 at
   its spec.md:547; the test registration anchors
   `test/CMakeLists.txt:553-559` and :1837-1841; the preset names at
   CMakePresets.json:121, 130, 157, 168; the clang-format 18.1.8 pin at
   `.github/workflows/ci.yml:34`), with no error other than CIT-1. Where plan,
   research and tasks give different ranges for one site
   (`hunt_selection.hpp:116-131` vs `:106-131`; `runner.cpp:1022-1026` vs
   `:1022-1027`; `plan.cpp:502-537` vs `:505-536`) every range lands inside the
   same construct, so no finding follows.
9. Constitution references used by the artifacts exist and mean what the
   artifacts say: III TDD mode (constitution.md:552-556), IV.1 (:579),
   X.3 (:884), X.4 (:901), XI.6 (:1006), N-2 (:621), N-8 (:634, so
   `kMaxRawHexDigits` is legal). T039's commit template matches Pull Request
   Quality (:1099-1132) and matches tracked practice: no `Fixes #` line is needed
   for a lane that closes no issue, and the prose gate has no such check.

## Coverage summary

| Requirement | Has task? | Task IDs (tasks.md:722-750) | Plan-side check |
|---|---|---|---|
| FR-001 | yes | T004, T010, T014 | plan.md:284-285 |
| FR-002 | yes | T004, T007, T011, T016 | plan.md:285-286 |
| FR-003 | yes | T004, T012 | plan.md:286 |
| FR-004 | yes | T004, T007, T011 | plan.md:287-288 |
| FR-005 | yes | T004, T007, T010, T014, T016 | plan.md:288-289 |
| FR-006 | yes | T007, T017, T033 | plan.md:290-291 |
| FR-007 | yes | T004, T014, T017, T018 | plan.md:291-292 |
| FR-008 | yes | T004, T007 | plan.md:293-294 |
| FR-009 | yes | T007, T018 | plan.md:295-296 |
| FR-010 | yes | T019, T020, T021, T031 | plan.md:296 |
| FR-011 | yes | T004, T007, T015, T018 | plan.md:297 |
| FR-012 | yes | T022, T023 | plan.md:297 (step 7) |
| FR-013 | yes | T024-T036, T040 | plan.md:298 (step 8) |
| FR-014 | yes | T037, T038, T039, T041 | plan.md:298 (step 10) |
| FR-015 | yes | T010, T012, T013, T014, T017 | plan.md:299-300 |
| SC-001 | yes | T004, T033, T035 | plan.md:51, 246 |
| SC-002 | yes | T004, T007, T033 | plan.md:248, 268 |
| SC-003 | yes | T007, T033 | plan.md:274 |
| SC-004 | yes | T015, T018, T037 | plan.md:278 |
| SC-005 | yes | T027, T036 | none by id (COV-1) |
| SC-006 | yes | T022, T023 | none by id (COV-1) |

**Constitution alignment issues**: none. Two standing tensions are inherited, not
introduced here, and the artifacts state them as the tracked wording does:
VI's "100% branch" gate against the recorded `allowance=21`
(`tools/dbc/coverage_gate.sh:108`), and VII's critical-path baseline duty against
the constitution's own open deferral of the baseline infrastructure
(plan.md:120-124, research.md R-15). GATE-1 is the one place this lane's own
wording softens an VIII gate without the P2 framing.

**Unmapped tasks**: none. Every task maps to at least one FR, SC, or user story
through its tag and the map at tasks.md:722-750.

**Metrics**

- Total requirements: 15 FR, 6 SC (plus US1-US4)
- Total tasks: 41 (T001-T041), 12 marked `[P]`, 7 waves
- Coverage: 100% of FRs and SCs carry at least one task and one named check
- Ambiguity count: 1 (AMB-1)
- Duplication count: 0
- Critical issues: 0
- High: 2, Medium: 6, Low: 3

## Next actions

Two HIGH findings should clear before `/speckit-implement`, both cheap:

1. INC-1: add the `build`-tree serialization to tasks.md (one edge or one
   sentence), since the TDD red/green reads depend on a single writer per tree.
2. INC-2: name the commit point the lane record presupposes, or rewrite T040's
   tree-equality read; then T039's squash read has something to squash.

The six MEDIUM findings are single-line edits: widen FR-015's reader clause
(COV-2), fix T037's file count (INC-3), fix T015's diff read (ORD-1), name the
synthesis entry point and predicate (AMB-1), fix the seam-test citation
(CIT-1), and add the SC ids to the plan map (COV-1). The three LOW findings are
wording.

Remediation was not applied: no artifact was edited. Say the word and the
edits for findings 1 and 2, or for any subset, can be drafted.
