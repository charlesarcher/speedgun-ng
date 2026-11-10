# Analyze Pass 2: specs/051-raw-events (cross-artifact consistency after 036b764d)

**Mode**: read-only. No artifact, source, build, or git state was modified; this
file is the only write.
**Artifacts**: `spec.md`, `plan.md`, `tasks.md` at head `036b764d` (tasks written
against base `0fce6651`, whose source tree equals the head source tree, so every
`path:line` reads the same at both); `research.md`, `data-model.md`,
`quickstart.md` as supporting texts; `.specify/memory/constitution.md`.
**Method**: the speckit.analyze detection passes, run with fresh eyes over the
post-remediation text: requirement inventory, FR/SC coverage maps both ways,
physical-view to task file ownership both ways, dependency DAG and `[P]`
build-tree and file disjointedness (including the new T042 and the `build/raw-red`
trees), citation resolution, scope guard, standing-gate wording, and counts.
**Scope of pass 1**: 0 critical, 2 high, 6 medium, 3 low, all remediated in
`036b764d` except ORD-3 (kept by ruling: renumbering FR-015 would invalidate
every recorded FR-015 reference).

## Findings

| ID | Category | Severity | Location(s) | Summary | Recommendation |
|---|---|---|---|---|---|
| DEP-1 | Inconsistency / dependency | MEDIUM | tasks.md:30 ("The dependency lines decide the order"), 519, 794-796, against the `Depends on` lines at tasks.md:538, 546, 560, 573, 589, 594, 604, 609, 617, 634, 648, 662, 674, 684 | The new T042 states "T024 through T037 depend on T042" (tasks.md:519, repeated at 794-796 and in the wave block at 815), but not one of T024 to T037 names T042 in its own `Depends on` line. Since tasks.md:30 makes the per-task dependency lines the order of record, a lane that follows them runs the full suite, the sanitizers, coverage and the analyzers before the commit T042 exists, which is exactly the INC-2 failure the commit task was added to close. | Add `T042` to the `Depends on` clause of every task in T024-T037, or state in one line that the wave-level statement at 794-796 overrides the per-task clauses for this edge. |
| PLAN-1 | Coverage gap / artifact divergence | MEDIUM | plan.md:312-345 (Step and Check Plan, steps 1 to 10), plan.md:382-390 (wave table), plan.md:193-230 (physical view), plan.md:360 (results list); tasks.md:512-523 | T042 has no plan-side counterpart. The Step and Check Plan carries no commit step between step 7 (numbers) and step 8 (gates), the physical view names no branch write and no `results/commits.md`, and the Documentation structure lists results files for "FR-012 and FR-013 readings, host pair record" only. T042 also sits under the Phase 6 (Wave 6) heading while its own surface, the branch and `results/commits.md`, is the Wave 7 row of the plan wave table ("`specs/051-raw-events/results/` and the branch"), and tasks.md:7 says the plan's wave table fixes the order. | Add one plan step ("commit the lane work, record the built head in `results/commits.md`") before step 8, name `results/commits.md` in the results list, and move T042 under the Phase 7 heading or mark it as the W7 hand-off. |
| LANE-1 | Inconsistency / lane record | MEDIUM | tasks.md:706-707 ("the T042 commit and the later record commits"), 718-732 (T040), 741 ("no further commit follows T039"), 830-831 ("no task after it writes the tree"), 656-677 (T035, T036), 692-704 (T038) | No task commits the records written after T042: T035, T036, T038, and T042's own `results/commits.md`, so T039's parenthetical "the later record commits" names commits the lane never makes. T040 then writes into `results/gates.md` after the squash with no commit path at all, while T041 forbids a further commit and the Implementation strategy states that no task writes the tree after T039. The squashed-head re-read rows, the reading that carries every T024-to-T034 verdict to the head, therefore never reach the pushed head. | Name the commit for the post-T042 records (one `git add`-and-commit read in T038, or state that T039 squashes with `git reset --soft 0fce6651` from the working tree), and give T040 an amend or a commit line, or move the T040 rows into T036 and drop the post-squash write. |
| BUILD-1 | Inconsistency / parallel safety | MEDIUM | plan.md:384-385 (W1 and W2 rows), plan.md:392-396 ("so the shared `build` stays single-writer"), tasks.md:50-57, 118-135 (T003), 192-216 (T005, T006), 808-809 (W1 and W2 chains) | INC-1 was closed for the W4 side by giving T008 and T009 the `build/raw-red` tree, and that half is sound: T008 configures `ci-rocky -B build/raw-red` (a shape the presets support) and T009 builds the same tree. The W1 side is still open. T003 configures `ci-ubuntu` (clang-tidy plus cppcheck) into the shared `build`, cleans it, and rebuilds it; T005 and T006 drive the same `build` for the counters registration and the red read. No edge orders the two chains: T003 depends on T002, T004 depends on nothing, and T010 depends on T006 only, so the plan's new single-writer claim holds only under a serialized W1 and W2 reading. | Add one edge (T004 after T003, or T005 after T003), or take the base analyzer read in a tree of its own, and drop the file-only view of W1 and W2 in the wave table. |
| MAP-1 | Coverage map drift | MEDIUM | tasks.md:767, 768 against tasks.md:512 | The FR-to-task map no longer matches the tags it summarizes. T042 carries `[FR-013] [FR-014]`, yet the FR-013 row reads "T024 to T036, T040" and the FR-014 row reads "T037, T038, T039, T041". The map is the artifact that answers "which task closes this requirement", and the built-head identity that FR-013's recorded verdicts lean on is the missing cell. | Add T042 to both rows. |
| SC-1 | Count disagreement | LOW | plan.md:305-307 | The SC map added by the remediation reads "SC-002 the four scripted refusal scenarios" and then names five: `absentDeviceRefusesWithoutProbe`, `wideLeafNeverProbes`, `rawProbeErrnoArms`, `wideLeafWidthLine`, `refusedProbeListedAsLines`. The four is SC-002's four refusal inputs; the five are the scenarios that pin them. | Write "the four refusal inputs, pinned by five scripted scenarios", or name the four inputs and list the scenarios uncounted. |
| MAP-2 | Cross-reference drift | LOW | tasks.md:769, 777 against plan.md:308-309; tasks.md:773; tasks.md:171-175, 374-384 | Three map rows and the texts they summarize disagree. The FR-015 row's named checks (`listingsNeverReadRawTable`, `scriptedPlanCompilesAndMeasuresRawLeaf`) are written by T004 and run green at T015, and neither task is in the row. The SC-004 row lists T015, T018, T037 while the plan's SC map names the step-5 pin reads and the step-8 full suite, that is T024 as well. US4 has no tagged task anywhere, so its map row (T011, T018, T037) rests on the map alone. | Add T004 and T015 to the FR-015 row and T024 to the SC-004 row, or move the row content to the plan wording; tag the hunter-pin tasks with US4 or state that US4 is carried by untouched pins only. |
| UNMAP-1 | Unmapped tasks | LOW | tasks.md:83-135 against the map at tasks.md:751-779 | T001, T002 and T003, the base readings, appear in no FR, SC, or user-story row and carry no such tag. They support FR-013 (a head reading is a difference from a recorded base) and plan step 1, and pass 1 recorded "Unmapped tasks: none", which is not true of these three. | Either add T001-T003 to the FR-013 row or state in the map header that the W1 readings support every row and map to no single FR. |
| CIT-1 | Citation precision | LOW | tasks.md:575-576; coverage_gate.sh:108, 122, 126; spec.md:339-340 against specs/029-hunter-core/spec.md:666-669 | Two citations name the wrong line or the wrong words. T027 asks for "the recorded D-14 message of `tools/dbc/coverage_gate.sh:108`": line 108 is `allowance=21`, the message "D-14 recorded allowance of ${allowance}" prints at :122 and :126. spec.md:339-340 restates specs/029 FR-090 as "next free minor per feature"; FR-090 reads "version 0.19.0, or the next free minor at rebase (recorded reason)". | Cite `:108` for the constant and `:122` for the message, and quote FR-090's "next free minor at rebase (recorded reason)". |
| QS-1 | Gate wording parity | LOW | quickstart.md:126-128 against tasks.md:61-63, 542-545, 553-555, 569-573 | The reviewer's read set drives the sanitizer legs without the compiler the tasks make mandatory: `cmake --preset=ci-sanitize && cmake --build build/sanitize` and the same for `ci-tsan`, with no `CXX=clang++`. The lane's own build discipline records that host GCC cannot build the sanitize leg and that a row must name its compiler. The coverage line also builds the `coverage` target without the suite run in `build/coverage` and without the `.gcda` reset that T027 names, so a reviewer following section 6 reads a stale or empty trace. | Put `CXX=clang++` on both quickstart sanitizer lines, and give the coverage line the ctest run and the `find ... -delete` of T027. |
| GUARD-1 | Scope guard parity | LOW | plan.md:232-235, tasks.md:67-74 | The two untouched lists differ in both directions. The plan names `benchmark.hpp` and "the option file and environment machinery" and omits `source/counters/linux_pmu/metric_formula.cpp`, `example/`, and existing test files; the tasks name those three and omit `benchmark.hpp`. The task-level guard is the one a scope read enforces (T037), so `benchmark.hpp` sits outside it. | Make one list authoritative and have the other point at it; add `source/harness/benchmark.hpp` to the task guard. |
| MARK-1 | Marker conventions | LOW | tasks.md:33-45 against tasks.md:512, 497, 656, 668, 815-816 | Three marker uses drift from the format section. T042 works the git surface and carries no `[O]`, while T039 and T041 do. T023, T035 and T036 are read-only tasks over commands and one `results/` record each, the `[R]` definition, and carry no `[R]`, while T002, T006, T037, T038 and T040 do. The W7 block shows `{ T028 | T029 | T030 | T031 | T032 }` inside concurrency braces although none of the five carries `[P]`, the marker the section defines as permission to run beside another task. | Mark T042 `[O]`, mark T023, T035 and T036 `[R]`, and either mark T028-T032 `[P]` or write them as a sequence. |
| VERIFY-1 | Verify looseness | LOW | tasks.md:457-459 against source/harness/options.cpp:556-566 | T019 states that "no other line of the option table moves" and then checks that the `-U0` hunks cover "lines 559 to 566 only". Line 559 closes the `dry-run` row and line 566 opens the `metric` row, so the check passes while a neighboring row is edited. | Narrow the check to 560 to 565, the `counter` row, or state why the two boundary lines join the range. |

## Pass-1 finding closure

| Pass 1 | Status at `036b764d` |
|---|---|
| INC-1 (HIGH) | Partly closed. W4 owns `build/raw-red` (tasks.md:276-281, 283-287; plan.md:394-396) and the W2/W4/W3 collision is gone. The W1 pair T003 against T005/T006 is open: BUILD-1. |
| INC-2 (HIGH) | Partly closed. T042 supplies the built head and T040 now reads its sha from `results/commits.md`, so the tree-equality read can pass. The records after that commit have no commit path and T040 writes after the squash: LANE-1. |
| COV-1 (MEDIUM) | Closed. The plan carries an SC map (plan.md:303-310); every SC id now has a named plan-side reading. One count is wrong: SC-1. |
| COV-2 (MEDIUM) | Closed. FR-015 (spec.md:230-235) names three readers, matching D-5 (spec.md:370-378), the plan's raw-entry section (plan.md:187-190), data-model.md:70-72, and T010's table comment (tasks.md:313-318). |
| INC-3 (MEDIUM) | Closed. T037 reads "the seven source files (five under `source/counters/`, two under `source/harness/`)", which matches the physical view and T042's `git add` list. |
| ORD-1 (MEDIUM) | Closed. T015's `test/source` diff read is now conditional on T007 landing the harness unit. |
| AMB-1 (MEDIUM) | Closed. T010 names `hasRawCodeShape`, `kMaxRawHexDigits`, `rawEventTable`, `synthesizeRawEntry`, and T011's grep uses the name. The counters predicate name matches the harness one at `source/harness/detail/hunt_selection.hpp:116`; the two sit in `sg::counters::detail` and `sg::detail` and stay apart, and R-01 carries the reason for the split. |
| CIT-1 (MEDIUM) | Closed. T004 now cites `test/source/counters_linux_pmu_seam_test.cpp:50-59`, which is the private-seam include block. |
| ORD-2 (LOW) | Closed. The order sentence names the phase order W1, W2, W4-authoring, W3, W5, W6, W7. |
| ORD-3 (LOW) | Kept by ruling. FR-015 stands between FR-007 and FR-008 in spec.md:223-235; both maps resolve the id. |
| GATE-1 (LOW) | Closed. T026 now records both tsan reads "with the P2 exception clause Principle VIII frames", matching constitution.md:801-803. |

## Checks that came back clean

1. Requirement inventory. 15 FRs and 6 SCs, each with tasks and a named check;
   the FR-015 mid-list position is the only ordering oddity and is kept by
   ruling.
2. Maps both ways. Every FR-###, SC-### and US# in the tasks map exists in
   spec.md, and every task id in the map exists in tasks.md. Every scenario name
   in the plan's FR map and in the new SC map appears in the two plan scenario
   tables, and every one of those twelve plus ten names appears verbatim in T004
   and T007 with the same counts: twelve case functions, ten case functions.
3. Physical view and file ownership, both ways. The plan's thirteen paths
   correspond exactly to the tasks that open them (pmu.hpp T010, system.cpp
   T014, plan.cpp T013, provider.cpp T011, group_io.cpp T012, runner.cpp
   T016-T017, options.cpp T019, README.md T020, harness.md T021,
   CMakeLists.txt T022, test/CMakeLists.txt T005 and T008, the two new units
   T004 and T007), and T037's expected diff is that list plus `specs/`. W6 opens
   no file the W2-W5 chain opens, and the two `test/CMakeLists.txt` blocks stay
   ordered T005 before T008.
4. `[P]` disjointedness. The twelve `[P]` tasks open twelve distinct files, and the
   three `[P]` gate tasks own three distinct trees: `build/sanitize`,
   `build/tsan`, `build/coverage`, which is what `CMakePresets.json` gives those
   presets. `ci-rocky`, `ci-ubuntu` and `ci-ubuntu-test` inherit `ci-build` at
   `${sourceDir}/build`, so the shared-tree rule is stated correctly.
5. DAG. With T042 read as depending on T018 and T023, the graph is acyclic and
   T016 after T015 and T009, T034 after T033, T039 after T036-T038 all hold. The
   open question is the missing T042 edges (DEP-1); the graph holds no
   cycle.
6. Citations. Every citation touched by `036b764d` was content-checked:
   `counters_linux_pmu_seam_test.cpp:50-59`, `pmu.hpp:278-292`,
   `hunt_selection.hpp:116` (the predicate line, inside the `:106-131` block the
   plan cites), `provider.cpp:325-332` and `:341-342` (the `loadDevice` type and
   cpumask reads), `:258-285`, `:667-730`, `:722-729`, `:537-557`, `:45-64`,
   `test/CMakeLists.txt:1837-1841`. A further sample of load-bearing anchors was
   checked line by line: `resolveLeafCore` `system.cpp:573-590` with the
   near-miss arm at `:582-589` and `probedKindTable` at `:165-177`,
   `counters_measurement.hpp:607-608`, `:103`, `:132-141`,
   `counters_system.hpp:143-178`, `counters_core.hpp:162-184` and `:205-206`,
   `metric_formula.cpp:1273-1305`, `plan.cpp:445-466`, `:474`, `:502-537`,
   `:668-692`, `group_io.cpp:153-198` and `:200-225`, `core.hpp:30-39`,
   `runner.cpp:986-1030` with `:1012-1013` and `:1022-1026`, `:1435-1502`,
   `:1437-1443`, `:1586-1597`, `cli.cpp:106-128` and `:960-970`,
   `report.cpp:32-39` and `:727-748`, `json_report.cpp:58`, `:532-538`,
   `:873-880`, `:881-888`, `:1118-1121`, `options.cpp:560-565`, `:1441-1462`,
   `:1631-1653`, `internal.hpp:640`, `options.hpp:250`,
   `catalog.cpp:43-60`, `report-v1.json:689-696`, `hunt-cli.md:59` and `:83`
   against `hunt_cli.cpp:640-645`, `hunt.cpp:729-733` and the pin at
   `harness_hunt_cli_test.cpp:394-398`, `runner.cpp:81-89` and
   `hunt_selection.hpp:86-93`, `test/CMakeLists.txt:553-559`,
   `counters_pmu_provider_test.cpp:179-268` with the recorder at `:184`, the
   `writeDevice` `withCpumask` arm at `:247`, `harness_limit_disclosure_test.cpp
   :1-12`, `:206-310`, `harness_gap_test.cpp:1-12`,
   `harness_cli_test.cpp:729-755` and `:769-775`, README.md:970,
   `harness.md:637-651`, `:675`, `:2577-2579`, `:2621`,
   `example_drift.py:256`, `:567-581`, `coverage.cmake:47-59`, `:68-135`, `:73`,
   `ci.yml:34`, `.clang-tidy` `WarningsAsErrors`, `CMakeLists.txt:78-86`,
   `:167-172`, `:453-456`, `seam_test.cpp:4110-4148`, `:4195-4222`, `:4309-4329`.
   All resolve to the named construct. The two imprecisions are CIT-1.
7. Counts and numbers. `git grep -c LCOV_EXCL -- source include` sums 84;
   `ctest --test-dir build -N` answers `Total Tests: 190` in the existing tree,
   so T005's 191 and T008's 192 hold; `README.md` measures 1003 against
   `PAGE_LINE_CAP = 1004`; base `VERSION 0.34.0` at `CMakeLists.txt:82` with the
   specs/048 note at `:78-81`, counters `SOVERSION 8` at `:172` under the
   specs/050 note, harness `SOVERSION 22` at `:456` with the specs/048 precedent
   at `:453-455`; `kSchemaVersion = "1.16"` at `json_report.cpp:58`. 0.35.0 is
   free at the base. The twelve counters-plus-harness source files, the two
   units, and the four text and build carriers agree across the plan, T037,
   T042 and the map.
8. Standing gate wording. The coverage wording matches the tracked tools
   (`allowance=21` at `coverage_gate.sh:108`, `--ignore-errors mismatch,negative`
   at `coverage.cmake:73`, the `inconsistent` class recorded gone at `:47-59`);
   the analyzer gate keeps the zero-finding name check that `.clang-tidy` turns
   into a build failure; the tsan exception keeps the VIII P2 framing; the hunter
   refusal text at `hunt-cli.md:83` is byte-identical to the pin. No task opens a
   CI workflow, a public header, the schema document, or an existing test.

## Coverage summary

| Requirement | Has task? | Task IDs | Plan-side check |
|---|---|---|---|
| FR-001 | yes | T004, T010, T014 | plan.md:284 |
| FR-002 | yes | T004, T007, T011, T016 | plan.md:285-286 |
| FR-003 | yes | T004, T012 | plan.md:286 |
| FR-004 | yes | T004, T007, T011 | plan.md:287-289 |
| FR-005 | yes | T004, T007, T010, T014, T016 | plan.md:289-290 |
| FR-006 | yes | T007, T017, T033 | plan.md:291 |
| FR-007 | yes | T004, T014, T017, T018 | plan.md:291-294 |
| FR-008 | yes | T004, T007 | plan.md:294-295 |
| FR-009 | yes | T007, T018 | plan.md:296 |
| FR-010 | yes | T019, T020, T021, T031 | plan.md:296-297 |
| FR-011 | yes | T004, T007, T015, T018 | plan.md:297 |
| FR-012 | yes | T022, T023 | plan.md:297-298 (step 7) |
| FR-013 | yes | T024-T036, T040; T042 missing from the row (MAP-1) | plan.md:298 (step 8) |
| FR-014 | yes | T037, T038, T039, T041; T042 missing from the row (MAP-1) | plan.md:298-299 (step 10) |
| FR-015 | yes | T010, T012, T013, T014, T017; its named checks sit in T004 and T015 (MAP-2) | plan.md:299-300 |
| SC-001 | yes | T004, T033, T035 | plan.md:303-304 |
| SC-002 | yes | T004, T007, T033 | plan.md:305-307 (count wrong, SC-1) |
| SC-003 | yes | T007, T033 | plan.md:308 |
| SC-004 | yes | T015, T018, T037; T024 in the plan map (MAP-2) | plan.md:308-309 |
| SC-005 | yes | T027, T036 | plan.md:309 |
| SC-006 | yes | T022, T023 | plan.md:310 |
| US1-US4 | yes | map rows at tasks.md:770-773 | spec user stories; US4 carries no tag (MAP-2) |

**Constitution alignment issues**: none. The constitution MUST gates that
constrain this lane (VIII hard gates, VI coverage, III TDD, X.3 scope, X.4 step
and check, XI.6 pinned quotation) are carried, and the two inherited tensions
read as the tracked wording does: VI's 100 percent branch against the recorded
`allowance=21`, and VII's baseline duty against the constitution's own open
deferral (plan.md:106-110, research.md R-15). T026's tsan branch is now the
VIII-named exception carrying its P2 clause; no gate is softened.

**Unmapped tasks**: T001, T002, T003 (UNMAP-1). Every other task carries a tag
that resolves to a map row.

**Metrics**

- Total requirements: 15 FR, 6 SC (plus US1-US4)
- Total tasks: 42 (T001-T042); 12 `[P]`, 8 `[R]`, 2 `[O]`; 7 waves
- Coverage: 100 percent of FRs and SCs carry at least one task and one named
  check
- Ambiguity count: 0 (no unresolved placeholder or vague attribute; AMB-1 closed)
- Duplication count: 0 (the two shape recognizers are one contract in two
  places, recorded and priced in research R-01)
- Critical issues: 0
- High: 0, Medium: 5, Low: 8

## Next actions

Nothing blocks `/speckit-implement` the way pass 1's two HIGH findings did: the
lane can be executed as written by a careful reader. The five MEDIUM findings are
each a line or two, and all of them sit on the head-identity and build-tree
spine that the remediation touched:

1. DEP-1 and PLAN-1: put T042 into the dependency lines of T024-T037 and give it
   a plan step and a W7 home, so the built head is ordered, planned, and placed.
2. LANE-1: name the commit that carries the records written after T042, and the
   one that carries T040's rows, or move those rows into T036.
3. BUILD-1: serialize T003 against T005 and T006, or take the base analyzer read
   in its own tree.
4. MAP-1: two map cells.

The eight LOW findings are wording, marker and citation precision.

Remediation was not applied: no artifact, source, or git state was edited. Any
subset can be drafted on request.
