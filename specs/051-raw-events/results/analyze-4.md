# Analyze Pass 4: specs/051-raw-events (cross-artifact consistency after 1b9ed084)

**Mode**: read-only. No artifact, source, build, or git state was modified;
this file is the only write.
**Artifacts**: `spec.md`, `plan.md`, `tasks.md` at head `1b9ed084` on
`051-raw-events`, with `research.md`, `data-model.md`, `quickstart.md` and
`checklists/requirements.md` as supporting texts, and
`.specify/memory/constitution.md` (2.20.0) for the gates. Tasks stand written
against base `0fce6651`. `git diff --name-only 0fce6651 1b9ed084` lists only
paths under `specs/`, so every `path:line` in the artifacts reads the same at
both heads; every citation in this report was re-read at `1b9ed084`.
**Method**: the speckit.analyze detection passes with fresh eyes. Prior reports
were read for closure only, never as evidence, and their line numbers are stale
by 20 to 25 lines. The pass rebuilt the requirement inventory and the maps in
both directions, the physical view against task file ownership in both
directions, the dependency graph including the T042 edges, the build-tree and
file serialization rules, the citations, the scope guard, the gate wording
against the tracked tools, the counts, and the closure of all thirteen pass-3
findings at current line numbers.
**History**: pass 1 found 2 high and 9 lower, remediated at `036b764d`. Pass 2
found 5 medium and 8 low, remediated at `7da7e1fd`. Pass 3 found 5 medium and
8 low, remediated at `1b9ed084`, which also split the two test registrations:
T005 and T008 now land build blocks, and each `add_test` line joins its unit's
green task at T015 and T018. Pass 1's ORD-3, the FR-015 position between FR-007
and FR-008, stands by ruling and is not re-opened here.

## Findings

| ID | Category | Severity | Location(s) | Summary | Recommendation |
|---|---|---|---|---|---|
| SPLIT-1 | Inconsistency / file serialization | MEDIUM | tasks.md:36-38, 274-277, 381-386, 806-807 against tasks.md:386 and plan.md:400-402 | The split put a second writer into `test/CMakeLists.txt` inside the counters chain. T015 lands the counters `add_test` line in T005's block, and T008 lands the harness build block in the same file. T005 precedes T008, and T015 precedes T018 through T016 and T017, but no depends clause orders T008 against T015, and the plan runs W4 beside W3. The rule sentence at tasks.md:36-38 still names one pair: "the counters block (T005) lands before the harness block (T008)". | Name T008 in T015's Depends clause, and widen the rule to the four edits in the order T005, T008, T015, T018 in the format section and at tasks.md:806-807. Misbuild read: a lost block fails the T008 build read or the T015 `ctest --test-dir build -N -R counters_raw_event_test` read, so no wrong binary passes a gate silently. |
| SPLIT-2 | Inconsistency / TDD preamble | MEDIUM | tasks.md:16-18, 804 against tasks.md:199-201, 212-213, 278-279, 293-294 | The Tests preamble states the rule the split replaced: "Each new unit stands written, registered, and observed red before the code that turns it green". T005 says the `add_test` line joins at T015 and T006 says the unit is unregistered until T015, so registration now follows the implementing tasks. The dependency section repeats the old label: "T004 to T006: W2, in order (author, register, red read)". | Write the preamble as written, built, and observed red, and label W2 as author, build block, red read. Misbuild read: none, because each task names its own commands. |
| SPLIT-3 | Artifact drift / wave contract | MEDIUM | plan.md:226-227, 393-396, 404-406 against tasks.md:274-277, 381-383, 439-441, 812 | The plan still describes the pre-split shape. The physical view calls the file "two registrations"; the W2 and W4 rows open "its `add_executable` and `add_test` block", which W2 and W4 no longer complete; the W3 and W5 rows name no CMake file, yet T015 and T018 edit that file; the order prose keeps "the two registrations in separate tasks"; and tasks.md:812 keeps "One file" for W5. Four tasks open that file across four waves. | Name the four edits in the physical view, add `test/CMakeLists.txt` to the W3 and W5 rows, and rewrite plan.md:404-406 to the order T005, T008, T015, T018. Misbuild read: none on its own, and this is the wording that authorizes the SPLIT-1 pairing. |
| TAG-1 | Coverage map drift | LOW | tasks.md:790 against plan.md:279 and tasks.md:234 | The pass-3 fix rebuilt the story cells from the tags, and the cells now match the tag sets exactly. That leaves one cell short of its own check: the US1 row names `rawRunConsoleAndJson`, which the plan's harness table assigns to US1, and the task that writes that case, T007, carries only [US2] and [US3]. `duplicateRawAddressTwoColumns` is US1-3 by the same table and stands in the same unit. | Tag T007, T008 and T009 with [US1], or name a US1 check that T018 owns. Misbuild read: none. |
| QS-2 | Gate wording parity | LOW | quickstart.md:128-129 against tasks.md:582-587 and cmake/coverage.cmake:150-157 | The reviewer's coverage lines configure and build `build/coverage`, then run the `coverage` target. That target captures, extracts, lists and summarizes an existing trace, so the quickstart sequence has no trace to capture: it runs neither the suite in that tree nor the `.gcda` reset that T027 names. Pass 2 raised this as QS-1 and pass 3 recorded it closed. | Give the block the T027 order: build, delete the `.gcda` files, run ctest in `build/coverage`, then the coverage target. Misbuild read: none, and the lane's own T027 leg already runs the correct order. |
| XI7-2 | Constitution wording | LOW | quickstart.md:63-64 | One of the eight code spans pass 3 listed still wraps across a line break: the width line begins on 63 and closes on 64, so constitution.md:1059-1060 breaks there. The other seven spans now sit on one line. | Reflow the sentence so the span starts and ends on one line. Misbuild read: none. |
| WRAP-2 | Wording artifact | LOW | tasks.md:540-541, 759-760, 816, 820 | Four hard breaks split a phrase: two end a line at "Depends", one puts "T024 through T037" on a line of its own, and one puts "tree, and" alone under "the shared `build`". The last of the four pre-dates the head commit, and the T042 hunk at 815-816 is new. | Re-wrap those four clauses at a phrase boundary. Misbuild read: none. |
| GUARD-3 | Scope guard parity | LOW | plan.md:233 against tasks.md:70-74 | The two untouched lists match on every name except two entries. The plan names `docs/schema/report-v1.json` where the guard names `docs/schema/`, a directory that tracks four documents, and the plan names `benchmark.hpp`, a path the guard reaches through `include/`. | State the directory form in the plan, or point the plan at the guard as the list of record. Misbuild read: none, because T037 enforces the wider guard. |

## Pass-3 finding closure

Each line is quoted from `1b9ed084`.

GUARD-2 (MEDIUM), closed. plan.md:236-238:

```text
file, every part of `options.cpp` outside the counter row text at
`:560-565` (the address and environment machinery at `:1441-1462` and
`:1631-1653` stands untouched), and `.github/` (no CI edits).
```

UNMAP-2 (MEDIUM), closed. tasks.md:789:

```text
| Red units | T004 to T009 | `results/red-units.md`, direct binary runs |
```

All 42 task ids now appear in at least one map row, and every row cell names a
task that exists.

MAP-3 (MEDIUM), closed. tasks.md:790-793 list, for US1, US2, US3 and US4, the
task sets `[US1]`, `[US2]`, `[US3]` and `[US4]` exactly. The four cells hold 11,
10, 5 and 2 tasks, and the tag sets hold the same. TAG-1 records the one check
that falls outside that framing.

DEP-2 (MEDIUM), closed. tasks.md:541:

```text
       on T018, T019, T020, T021 and T023; T024 through T037 depend on T042.
```

tasks.md:815 repeats it, and all fourteen tasks from T024 to T037 name T042 in
their own Depends clause. T022 reaches the commit through T023.

GATE-2 (MEDIUM), closed by the second option. T040 re-reads prose over the tree
at tasks.md:738:

```text
       `python3 tools/prose/prose_gate.py --check prose --mode tree` exits 0,
```

`--paths` narrows the check, so with no `--paths` the read covers the records
that T035, T036 and T038 write after T030 (tools/prose/prose_gate.py:165-170).

MAP-4 (LOW), closed. tasks.md:788 is one line, as every other row of that table
is.

COUNT-1 (LOW), closed. tasks.md:686-687:

```text
       the T034 analyzer counts beside the base counts, the fast reads of
       T028 to T032,
```

ORD-4 (LOW), closed. The Phase 7 heading stands at tasks.md:526, its order
sentence at 528-532, and T042 at 534, ahead of T024 at 547. Phase 6 ends with its
independent test at 521-522.

XI7-1 (LOW), partly closed, carried on as XI7-2. Seven of the eight spans sit on
one line: spec.md:261-262, tasks.md:196, tasks.md:274, tasks.md:542, tasks.md:604,
tasks.md:724, and the first quickstart line at quickstart.md:62-63. The width-line
span at quickstart.md:63-64 still wraps.

WRAP-1 (LOW), partly closed, carried on as WRAP-2. tasks.md:590 and 760 are clean;
tasks.md:540-541, 759-760, 816 and 820 keep a split phrase.

EDGE-1 (LOW), closed. tasks.md:297-298:

```text
       W3 carries it. Depends on T006 (the same record) and on T008; T016
       already names this read in its own depends line.
```

The rule lives in the dependency section at tasks.md:808, and T016's own clause
names T009.

DOC-1 (LOW), closed. plan.md:366-368:

```text
└── results/             # FR-012 and FR-013 readings, host pair record,
│                        # base readings, red-unit reads, the commit
│                        # handle and the pull-request body draft
```

CHK-1 (LOW), closed. checklists/requirements.md:34 reads "The clarify loop ran
seven passes".

## Checks that came back clean

1. Requirement inventory and both maps. Fifteen FR ids and six SC ids in
   `spec.md`, each with a tasks row and a named check, and US1 to US4 with rows.
   `FR-023` and `FR-090` inside spec sentences belong to specs/029 and name
   standing lines, so they need no row. Every FR and SC tag on a task lands in
   that id's row. The plan's scenario tables hold 12 and 10 case names, and T004
   and T007 restate those 22 names with the same counts and no extra case.
2. Physical view against task ownership, both ways. The plan's paths map to one
   owning task each: `pmu.hpp` T010, `system.cpp` T014, `plan.cpp` T013,
   `provider.cpp` T011, `group_io.cpp` T012, `runner.cpp` T016 and T017,
   `options.cpp` T019, `README.md` T020, `harness.md` T021, `CMakeLists.txt`
   T022, the two units T004 and T007, and `test/CMakeLists.txt` T005, T008, T015
   and T018. No task edits a path outside that list and `results/`. T037's
   expected diff is that list plus `specs/`. SPLIT-3 covers the plan rows that
   lag the fourth pair.
3. Dependency graph. All 42 tasks topologically sort, so no cycle. T042 depends
   on T018, T019, T020, T021 and T023, and T024 to T037 depend on T042. Over the
   shared `build` tree the order is total: T001, T003, T005, T006, T010, T015,
   T016, T017, T018, T024, T033 and T034, and a depends clause orders each pair
   in that list. `build/raw-red` holds T008 and T009,
   and `build/sanitize`, `build/tsan` and `build/coverage` hold one task each,
   which is what `CMakePresets.json` gives at :101, :122, :132 and :143.
   `ci-rocky`, `ci-ubuntu` and `ci-ubuntu-test` inherit `ci-build` at
   `${sourceDir}/build`. Over the file sets the single unordered pair is T008
   against T015, and both open `test/CMakeLists.txt`: SPLIT-1.
4. Split text against the tree it writes. `test/CMakeLists.txt:553-559` is the
   `counters_pmu_provider_test` block, `add_executable` at 553 through `add_test`
   at 559, and `:1837-1841` is the `harness_hunt_cli_test` block,
   `add_executable` at 1837 through `add_test` at 1841. T005's three lines and
   T008's four lines match the house shape at both anchors, and the two
   `add_test` texts in T015 and T018 match the one-line form the file already
   carries 87 times, 16 of them `counters_*`. The harness form needs the source
   include directory, and T008 carries it, as `harness_limit_disclosure_test`
   does at :868. The counters unit reaches the private headers through relative
   includes, the pattern at `counters_linux_pmu_seam_test.cpp:50-59`, so T005's
   block needs no include directory of its own.
5. Citations. 290 `path:line` citations across the six artifacts resolve to a
   tracked file, and every range stays inside its file: 158 distinct ranges.
   46 load-bearing ranges were read line by line at this head, and they carry
   what the artifacts claim: `system.cpp:573-590` with the near-miss arm at
   :582-589 and `probedKindTable` at :165-177, `system.cpp:305-312` with
   `no object at path` at :311, `pmu.hpp:259-276` and :278-292 and :742,
   `provider.cpp:258-285` inside `probeDevice` at :245, :325-332 and :341-342,
   :516-535, :537-557, :667-730 with the errno table at :722-729, :767, :797-862,
   :838-841, `plan.cpp:445-466` with `providerIndex` at :505, :474, :502-537,
   :668-692, `group_io.cpp:153-198`, :178-187, :200-225 with `exclude_kernel` at
   :221, `runner.cpp:986-1030` with :1012-1013, :1021, :1022-1026 and the
   `return 2` at :1027, :1437-1443, :1445-1455, :1487 and :1494, :1586-1597,
   :81-89, `hunt_selection.hpp:106-131` with the predicate at :116,
   `hunt-cli.md:59` and :83 against the pin at `harness_hunt_cli_test.cpp:394-398`,
   `harness_cli_test.cpp:729-755` with :731 and :769-775, `options.cpp:560-565`
   as the `counter` row, :1441-1462, :1631-1653, `cli.cpp:106-128` and :960-970,
   `report.cpp:32-39` and :727-748, `json_report.cpp:58`, :532-538, :873-880,
   :881-888, :1118-1121, `metric_formula.cpp:1273-1305` with :1277 and :1290,
   `counters_measurement.hpp:99`, :103, :132-141, :607-608,
   `counters_system.hpp:143-178` with the lookup at :147,
   `counters_core.hpp:87-95`, :162-184, :205-206, `core.hpp:30-39`,
   `internal.hpp:640`, `options.hpp:250`, `report-v1.json:689-696`,
   `catalog.cpp:43-60`, `counters_pmu_provider_test.cpp:179-268` with :184, :247
   and :309, `harness_hunt_cli_test.cpp:394-398`, `README.md:970`,
   `harness.md:637-651`, :675, :2577-2579, :2621, `CMakeLists.txt:78-86`, :167-172,
   :453-456, `example_drift.py:256`, :567-581, :798-804, `ci.yml:34`, and
   `constitution.md:463-465` and :801-803.
6. Readings re-run here. `ctest --test-dir build -N` answers `Total Tests: 190`.
   The marker read sums 84. `README.md` measures 1003 against
   `PAGE_LINE_CAP = 1004`. `CMakeLists.txt` reads `VERSION 0.34.0` at :82,
   counters `SOVERSION 8` at :172 and harness `SOVERSION 22` at :456, and each of
   the three greps the tasks run answers one line. `json_report.cpp:58` reads
   `1.16`. `build/speedgun-ngConfigVersion.cmake:13` reads `0.34.0`.
   `clang-format --version` reads 18.1.8, the version `.github/workflows/ci.yml:34`
   pins. `/usr/bin/clang++` exists. The host reads `AuthenticAMD`, family 26,
   model 68, `cpu/type` 4 and `events/instructions` `event=0xc0`.
7. Gate wording against the tracked tools. `allowance=21` at
   `tools/dbc/coverage_gate.sh:108`, the D-14 message at :121-123, the 100.0
   percent line arm at :51-53. `--ignore-errors mismatch,negative` at
   `cmake/coverage.cmake:73`, the `inconsistent` comment at :47-59, the target
   named `coverage` at :150-151. `cmake/lint.cmake` honors `FORMAT_COMMAND` at :9,
   `cmake/prose-lint.cmake` honors `PROSE_MODE`, `PROSE_CHECK` and `PROSE_BASE`
   at :17-31, and `cmake/spell.cmake` exists. `prose_gate.py` narrows with
   `--paths` at :165-170 and refuses `--mode tree` with `--check commit` at
   :183-187, which is what T030 and T040 rely on. `test/time_source_gate.sh`
   takes the repository root. `example_drift.py` applies `PAGE_LINE_CAP` to the
   main page and matches README rows to the option table at :798-804.
   `.clang-tidy:21` keeps `WarningsAsErrors` at `readability-identifier-naming`.
   T026's tsan clause matches `constitution.md:801-803`. The prose gate over this
   feature ran clean at this head: 11 sources, 2804 units, 0 findings, exit 0.
8. Scope guard. The guard and the plan list match except GUARD-3. T037's three
   reads cover every guard entry, and its directory patterns cover the four
   tracked documents under `docs/schema/`.
9. Counts and numbers. 42 ids, T001 to T042, no gap and no duplicate; 12 `[P]`,
   10 `[R]`, 4 `[O]`; seven waves in the plan table, seven phases in tasks, and
   W7 carries 7a in the plan row at plan.md:398. `0.34.0` at base and `0.35.0`
   at the head, `1.16`, `22`, and `8` stated the same way in spec, plan, tasks,
   quickstart and research R-12. The registered count reads 190 at base, 190
   through both red reads at tasks.md:207 and :287, and 192 at T024, and no
   artifact states 191 anywhere.

**Constitution alignment issues**: none. Principle VIII's gates all appear in
Phase 7 with no weakened configuration. III's TDD order holds: each unit is
written, built and observed red before the tasks that turn it green, and T006
makes a case that cannot compile at base a pause-and-ask signal, which keeps the
red read a run and not a compile guess. X.3 stands as the guard and T037, X.4 as
the step and check plan, XI.6's pinned quotations stay exempt, and XI.7's
one-line span rule breaks at one span in XI7-2. The two inherited tensions stay
as the tracked tools state them: VI's branch line against the recorded allowance
of 21, and VII's baseline duty against the constitution's own open deferral at
`constitution.md:463-465`.

**Unmapped tasks**: none.

**Metrics**

- Total requirements: 15 FR, 6 SC, plus US1 to US4
- Total tasks: 42 (T001 to T042; T042 stands at the head of Phase 7)
- Coverage: every FR, SC and US id carries tasks and a named check, and all 42
  tasks appear in at least one row
- Ambiguity count: 0
- Duplication count: 0; the two shape recognizers stay one contract in two
  places, recorded and priced in research R-01
- Critical issues: 0
- High: 0, Medium: 3, Low: 5

## Next actions

Nothing blocks `/speckit-implement`. Every gate verdict, count and citation reads
as written at this head. The three MEDIUM items are the same split seen from the
graph and from the plan, and each costs one or two lines:

1. SPLIT-1: add T008 to T015's Depends clause and state the four-edit order for
   `test/CMakeLists.txt` in the format section and at tasks.md:806-807.
2. SPLIT-3: one physical-view line, the W3 and W5 wave rows, and the order
   sentence at plan.md:404-406.
3. SPLIT-2: one word in the Tests preamble and one label in the dependency
   section.

The five LOW items are the map cell that omits its own check's author, the
quickstart coverage lines, one wrapped code span, four hard-wrap splits, and one
directory name in the plan's untouched list. None of the eight changes a build
verdict.

Remediation was not applied: no artifact, source, or git state was edited. Any
subset can be drafted on request.
