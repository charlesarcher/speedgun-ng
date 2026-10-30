# Analysis round 1: specs/039-context-groups

Analyzed on 2026-10-30 on branch `039-context-groups` at `c46a45e1`.
`git diff master...HEAD` names eleven files, all under
`specs/039-context-groups/`, so the source tree matches `master`. Basis:
the constitution, then `spec.md`, `plan.md`, `tasks.md`, `data-model.md`,
the two files under `contracts/`, the three checklists, and the three
clarification rounds. Round 3 landed during this pass, and every finding
below was re-read against the amended text.

Readings of the tree. The nine artifacts carry 176 `path:line` citations
over 100 distinct sites; every cited file exists, and no cited start
line stands past its end of file. `prose-lint` over `master..HEAD`
reports 10 sources, 2379 units, zero findings. The command
`python3 tools/docs/example_drift.py . --options` exits zero, so the
option-row scan runs and passes today. Three untracked files sit at the
repository root (`harness_noise_report_na.json`,
`harness_noise_report_plain.json`, `harness_noise_report_schema.json`),
written by a test run outside this pass.

## Findings

### F1. BLOCKER: SC-001 names an `l3` run the fit step always stops

- Artifact: `spec.md:354-376` (User Story 1, its Independent Test,
  Acceptance Scenario 1), `spec.md:804-807` (SC-001), `plan.md:311-312`,
  `tasks.md` T017 at `:180-191`, T033 at `:323-327`, T034 at `:328-330`.

The criterion:

> **SC-001**: A run of eight contexts over a four-L3-group layout under
> `--placement=l3` discloses exactly four groups of two contexts, each
> named `l3-0` to `l3-3`, and each context names the group its plan unit
> names (FR-006, FR-008).

The tree:

- `source/harness/placement_preset.cpp:127-131`: the `l3` arm deals one
  cpu for each distinct L3 group.
- `source/harness/placement_preset.cpp:192-198`: a count above the place
  count clears `cpu` and keeps `places` at the group count.
- `source/harness/placement_preset.cpp:656-669`: the fit step turns that
  pair into one refusal line at exit status 2.
- `test/source/harness_placement_preset_test.cpp:202-219`: over the
  installed four-group shape four contexts answer four places, and five
  contexts answer the empty plan.
- `test/source/harness_cli_placement_test.cpp:659-671`: five contexts
  under four places exit 2, print one line, and print no row.

Argument. FR-009 bars the lane from changing that deal, so an `l3` plan
can never hold two contexts in one group. Eight contexts over four
groups reach the fit checkpoint and stop there. US1 is the P1 story,
T034 records that run as SC-001's evidence, and T033 requires its driving
script to exit zero. The same shape drives the logical view's
example partition, where `l3-0` holds slots `[0,4]`.

Remediation: name `core` in SC-001, in the US1 paragraphs and in the
`plan.md:311-312` example. Over the shape the plan already describes,
`L3:4 Core:2 PU:2`, the `core` plan deals eight places and its
`plan.group` reads `0,0,1,1,2,2,3,3`, which is four groups of two. Keep
`l3` for the one-context-per-group case.

### F2. BLOCKER: the version move breaks `views_test`, which no artifact names

- Artifact: `tasks.md` T003 at `:49-55`, `plan.md:51-54` (Scale/Scope),
  M-A, the FR coverage rows FR-005 and FR-032, wave 8, and `tasks.md`
  T040.

The task:

> Move the project version `0.25.0` to `0.26.0` in `CMakeLists.txt:30`
> and the harness `SOVERSION` `16` to `17` in `CMakeLists.txt:333`

The tree:

- `test/source/views_test.cpp:246-253` reads the root `CMakeLists.txt`
  and checks the project version against `0.25.0`, the harness
  `SOVERSION` against `16`, and the archive `SOVERSION` against `4`.
- `test/CMakeLists.txt:768-777` registers that file as the CTest test
  `views_test`.

Argument. After T003 the project block answers `0.26.0` and the harness
line answers `SOVERSION 17`, so two checks report and the test binary
exits nonzero. T040 requires every test to pass on both release
presets, so wave 8 cannot close. The plan's FR map sends FR-005 and
FR-032 to "the version read" and names no standing test that reads it.

Remediation: add `test/source/views_test.cpp:246-253` to T003.

### F3. MAJOR: the teardown route stands outside the frame that holds the fixture

- Artifact: `tasks.md` T020 at `:205-212`, `plan.md:232-242` (M-E),
  `spec.md:707-713` (FR-026), the round-1 Q2 answer, and the round-3
  dropped bullet that reads the same plan text.

The requirement and the task:

> **FR-026**: ... and SHALL run context 0's fixture teardown after every
> body of the attempt has settled, on context 0's own thread (D-7).

> the teardown node runs on context 0 after `launchBodies` returns when
> the run carries more than one group (FR-026, D-7).

The tree:

- `source/harness/runner.cpp:2115-2118`: `State callbackState` and
  `std::unique_ptr<Fixture> fixture` are locals of one body invocation.
- `source/harness/runner.cpp:2149-2157`: the standing teardown runs
  inside that same invocation, on context 0's thread, after the stop
  barrier.
- `source/harness/runner.cpp:1913-1928`: one `WorkNode` per body index
  stands on the heap, and the node vector is local to the launcher.
- `source/harness/runner.cpp:2190`: `launchBodies(body, count)` runs on
  the thread that runs the attempt and returns once every body has
  joined.

Argument. Once `launchBodies` returns, context 0's body frame is gone,
so the `Fixture` instance and its `State` are out of reach, and the code
there runs on the attempt thread. T020 names one publication slot, and
it carries the setup text plus the ready flag. No artifact names a slot
that carries the fixture object and the callback state to the teardown.

Remediation: name that route in T020, with a per-attempt slot that holds
the fixture pointer and the attempt `State`. The other route keeps the
teardown inside context 0's body and names the flag that tells context 0
the last group settled.

### F4. MAJOR: the main page needs an option row that no task owns

- Artifact: `spec.md` FR-011 at `:629-640`, `plan.md:221-231` (M-D),
  `plan.md:259-265` (M-H), `tasks.md` T012, T015 at `:160-164`, T030 at
  `:297-303`.

The gate:

> f"{rel(main_page)}: long option '{option}' has no option row"

`tools/docs/example_drift.py:540-547` reads the row table out of
`source/harness/options.cpp`, and `:763-776` compares it against the
rows of the `| Option |` table on `README.md`. That scan runs whenever
the tool runs with no flag, which is how the suite runs it
(`test/CMakeLists.txt:1553-1556`). The option table stands at
`README.md:894`, inside the Quick reference section that starts at
`:783`.

Argument. T012 adds the C++ row, so the next drift run names
`--group-layout` and reports a missing row. T015's and T030's own verify
lines run `example_drift`, and T016 requires the full suite green, so
the gap lands in wave 3. FR-011 names the guide's option reference, and
T030 names the section, its region, its quick-reference rows, `SECTIONS`
and the page cap. None of them names an option row.

Remediation: name the `README.md:894` option row for `--group-layout` in
T030.

### F5. MINOR: the fault-shape count in M-C matches nothing

- Artifact: `plan.md:218-220`.

The line reads "the accept set, the fifteen fault shapes". FR-018
(`spec.md:660-668`) lists 16 fault clauses, table C of
`contracts/group-layout.md:45-60` holds 14 rows because two rows join
two clauses each, and SC-004 (`spec.md:815-817`) counts 18 cases with
the two pairings.

Remediation: write "the 16 fault shapes of FR-018" in `plan.md:219`.

### F6. MINOR: the one-context edge case contradicts FR-010

- Artifact: `spec.md:546-548` against `spec.md:623-625` and
  `contracts/group-partition.md:36-38`.

The edge row says a one-context instance "carries no threaded round, so
the layout names one group of that one context". FR-010 states that a
one-context instance "SHALL carry no group and no threaded round", and
contract B-1's post answers the empty vector for a one-context run.

Remediation: read the row as the file's text, in `spec.md:547`, so the
run answers no group.

### F7. MINOR: Key Entities still describes the refused cpu form

- Artifact: `spec.md:788-790` against D-6 (`spec.md:284-293`), FR-014
  and the Out of scope row at `spec.md:852-858`.

The entity line reads:

> a name and either slots or cpus (FR-012). Its grammar is closed.

FR-014 refuses the key `cpus`, FR-017's grammar names `slots` only, and
D-6 records the deviation. The entity still carries the form the release
refuses.

Remediation: drop "or cpus" in `spec.md:790`.

### F8. MINOR: T025 cites a line that holds something else

- Artifact: `tasks.md` T025 at `:251-257`.

The task moves the literals at `harness_noise_report_test.cpp:403` and
`:423`. Line `:423` is the `1.8` stamp check. Line `:403` holds the
`x-order` literal of the schema document. The file's second `1.8` plant
stands at `:453`, and no artifact names it.

Remediation: move the citation to `test/source/harness_noise_report_test.cpp:453`.

### F9. MINOR: the schema-literal file list counts four files, the tree holds three

- Artifact: `plan.md:255-257` (M-G test plan), `plan.md:294-295` (the
  risk row "Four tests stamp `1.8`"), against `plan.md:53-54` and
  `tasks.md` T025.

The tree carries the stamp in `test/source/harness_json_report_test.cpp:546`,
`test/source/harness_json_load_test.cpp:707`, and
`test/source/harness_noise_report_test.cpp:423` plus `:453`.
`test/source/harness_json_schema_test.cpp` holds no `1.8` literal; it
reads the schema document. Scale/Scope says three files and T025 names
three.

Remediation: replace `harness_json_schema_test` with
`harness_noise_report_test` in `plan.md:257` and read the risk row as
three tests.

### F10. MINOR: T018's verify names a test the suite does not register

- Artifact: `tasks.md:195-197`.

The line reads
`ctest --test-dir build -R "harness_runner_test|harness_group_partition_test"`.
No file under `test/` carries the name `harness_runner_test` and
`test/CMakeLists.txt` registers no such target, so that half of the
regular expression matches nothing and `ctest` exits zero over the tests
it did run.

Remediation: point the first alternative at a registered test, or drop
it.

### F11. MINOR: the plan names the DBC check `dbc_gate`

- Artifact: `plan.md:71` (the II row) and `plan.md:466` (the FR-040
  coverage row).

The build target is `dbc-gate` (`cmake/dbc-gate.cmake:40`) and the CI job
builds that name (`.github/workflows/ci.yml:1227`, `:1283`).

Remediation: write `dbc-gate` in both plan rows.

### F12. MINOR: the VII rows promise a round count nothing carries

- Artifact: `spec.md:889-891` and `plan.md:76`.

Both state that the round count per attempt rides the disclosure so a
pacing regression shows. No requirement, contract row, module or task
names such a count. FR-024 and FR-025 fix the round rules, and FR-028
and FR-029 fix the two channels. The five records in `tasks.md` T033 to
T039 name commands, exit codes, disclosures and reports.

Remediation: drop the clause from both rows, or add the count to FR-028
and to T036.

### F13. MINOR: FR-037 is the one requirement no task names

- Artifact: `tasks.md` (all 50 tasks), `plan.md:463` (the FR-037
  coverage row: wave 8, "the coverage target, the suite run").

Every other requirement appears in at least one task line, counted with
the ranges such as "FR-029 to FR-032" expanded: 39 of 40. T044 runs the
coverage preset, the suite and the coverage target. It cites VI, the
three new units and `LCOV_EXCL`, and never cites FR-037. The
requirement's own numbers, 100 percent lines and 100 percent branches,
sit outside the task's stated reading.

Remediation: cite FR-037 beside VI in `tasks.md:367-371`.

### F14. MINOR: the scale line counts two new units, the plan adds three

- Artifact: `plan.md:51-54` against the structure listing, the physical
  view and `tasks.md` T044 at `:367-371`.

The scale line reads "Two new harness translation units". The plan's own
structure and physical view add `source/harness/group_partition.cpp`,
`source/harness/group_layout.cpp` and `source/harness/group_fit.cpp`, and
T044 requires coverage over all three.

Remediation: write "three new harness translation units" in
`plan.md:52`.

### F15. MINOR: the plan's decision index stops at round 1 and keeps the old pairing shorthand

- Artifact: `plan.md:158-175`, the row at `:161`, `plan.md:221-231`
  (M-D).

The index carries R-1 to R-5, the five round-1 questions and the four P
notes. It names no round-2 answer and no round-3 answer, and the R-2 row
reads "preset or file, the pairing refuses". FR-019 now refuses beside a
preset that names a partition for the read in hand and allows the
pairing the round-3 ruling settled, and T013 reads each instance's
partition. M-D names the pairing refusals and no partition read.

Remediation: add the round-2 and round-3 rows to the index and narrow
`plan.md:161` to "the partition-naming pairing refuses".

### F16. MINOR: contract A-5 counts two entry points where the file states three

- Artifact: `contracts/group-partition.md:19-20`.

A-5 reads "Neither entry point reads a clock, allocates a thread, opens
a file besides the layout path it was handed, or throws to its caller."
Section B states three entries: B-1 `groupPartition`, B-2
`groupOfContext` and B-3 the fit entry. Only B-3 is handed a layout
path.

Remediation: write "No entry point" at
`specs/039-context-groups/contracts/group-partition.md:19`.

### F17. MINOR: T027's sample list misses two inline quotes of the placement field

- Artifact: `tasks.md:272-276`.

T027 names the three `contexts=` lines in `docs/pages/harness.md` and
the `report_shape` goldens. Two prose lines quote the same disclosed
field inline: `docs/pages/harness.md:971` and `README.md:607`, the
second beside a `.placement("node")` example at `README.md:602`. The
goldens and marked page blocks move under the gate's block comparison;
an inline code span stands outside that comparison, so nothing moves
them.

Remediation: add both lines to the list T027 re-reads.

## Checks that passed

- Consistency: 40 requirements, 40 rows in the plan's FR coverage map,
  and 39 of the 40 named by at least one task line once the ranges are
  expanded (F13 names the exception).
- Consistency: the option value 299 stands free, because
  `source/harness/detail/options.hpp:76-93` states 298 to 319 free and
  the last taken value is 297.
- Consistency: 38 rows stand in the table today
  (`test/source/harness_cli_table_test.cpp:209`), so T011's move to 39
  matches T012's added row and the row-count check at `:681-689`.
- Consistency: the schema story agrees across artifacts. The context
  item is closed at `docs/schema/report-v1.json:386-402`, the reader
  reads its fields by key at `source/harness/json_load.cpp:1874-1895`,
  and an added optional key shifts nothing; `kSchemaVersion` and its
  history stand at `source/harness/json_report.cpp:28-39`.
- Consistency: the tasks name ten modules, the plan states ten modules,
  and each module carries tasks in one wave.
- Grounding: 176 citations over 100 distinct sites resolve to files in
  the tree and none points past an end of file (F8 names the one line
  whose content differs).
- Grounding: the block-bounding citations land exactly:
  `source/harness/report.cpp:168-191` for `printContextDisclosure`,
  `source/harness/options.cpp:1150-1174` for `readTextFile`,
  `source/harness/cli.cpp:744-755` for the fit walk plus `printContext`,
  `source/harness/options.cpp:470-714` for `optionRows`,
  `source/harness/detail/options.hpp:81-93` for the option values, and
  `tools/docs/example_drift.py:222` for `PAGE_LINE_CAP = 940` against
  the 940 lines of `README.md`.
- Grounding: the runner citations land: `runner.cpp:2066-2067` holds the
  two whole-run barriers, `:2126` and `:2149` hold the two arrivals, and
  `:2190` holds `launchBodies`.
- Grounding: `prose-lint` over `master..HEAD` reads zero findings, and
  `example_drift` with `--options` passes today.
- Implementability: the version sites exist, `CMakeLists.txt:30` holds
  the project version, `:333` the harness `SOVERSION` and `:95` the
  archive `SOVERSION` (F2 names the standing test left behind).
- Implementability: the vendored reader can join the harness target.
  `CMakeLists.txt:246-267` defines the harness before
  `add_subdirectory(yaml-cpp)` at `:738-740`, and a `BUILD_INTERFACE`
  link to a target created by a later `add_subdirectory` configures
  (checked in a scratch CMake project). Position independent code stands
  on inside that bracket at `:723`, the purity scan reads `include/`
  only, and the nm proof reads the archive, so yaml stays off the public
  surface.
- Implementability: the fit entry has one call site at
  `source/harness/cli.cpp:744`, and `placementFitStatus` already walks
  the resolved instances there, which is where the amended FR-019 reads
  the partition.
- Implementability: T011's four sites land on the contract literal, the
  row-count check, the block-disjointness check and the guide byte
  compare, at `:209`, `:681`, `:858` and `:1498`.
- Constitution: I, no new dependency beyond the vendored yaml-cpp
  already in the tree, and one added public member.
- Constitution: II, both contract files state pre and post clauses, name
  the doxygen blocks as the authority on disagreement, and the DBC gate
  exists as a build target (F11 names the spelling).
- Constitution: III, each coding wave names its failing test task ahead
  of the implementation task it guards, at T001, T005, T007, T010, T017,
  T023 and T025, and the plan states TDD as the execution mode.
- Constitution: IV, FR-039 plus M-H carry the main page section, the
  region, the owner rows and the page cap move in one module.
- Constitution: V, the new identifiers fit N-1 to N-12: `ContextGroup`,
  `groupOfContext`, `kGroupLayoutOptionValue`, `group_partition.cpp`, and
  no new macro.
- Constitution: VI, T044 states 100 percent lines and branches over the
  three new units with no `LCOV_EXCL` marker.
- Constitution: VII, T022 reads the round rules, and FR-024 keeps every
  round outside a measured window (F12 names the one loose clause).
- Constitution: VIII, T040 to T047 name the build, suite, sanitizer,
  tsan, two-cpu, coverage, analyzer, naming, format, spell, prose, DBC,
  time-source and vendor-proof readings, and the drift gate runs in the
  verifies of T015, T027 and T030.
- Constitution: IX and X, four artifacts stand with the side documents,
  and the plan adds no barrier kind, no fallback and no second source of
  the partition.
- Soundness: the partition model holds A-1 and A-2 for both sources, and
  the empty partition answers the ungrouped shape of FR-004; `kNoGroup`
  at `source/harness/detail/internal.hpp:51-52` is the sentinel FR-006
  reads.
- Soundness: the refusal set is checkable, because the fit step runs at
  `cli.cpp:744` before any instance starts, and the amended pairing rule
  reads a value that step already resolves.
- Soundness: the group rounds take their participant counts from the
  same partition the dispatcher walks, and the standing ledger fault at
  `runner.cpp:1955-1961` precedes every wait the harness performs
  (FR-027, T021).
- Soundness: the schema key, the console segment and the round trip agree
  in FR-028 to FR-031, and an ungrouped run keeps byte identity through
  FR-033, SC-006 and T038.

## Verdict

Two BLOCKER findings, two MAJOR findings, 13 MINOR findings, 17 total.

The lane is not implementable as written. Both blockers cost one line
each. Name the preset that can put two contexts in one L3 group (F1),
and name `test/source/views_test.cpp` in the version task (F2). Fix F3
before wave 4 by naming the route that carries the fixture to its
teardown. Fix F4 before wave 3 by naming the main page's option row,
because T015's own verify runs the gate that reports it. The 13 minors
are count, citation and wording repairs, and each carries one place to
change.

## Round 2 recheck

Re-read on 2026-10-30. This checkout holds no later text. The commit
list ahead of `master` holds seven entries, HEAD stands at `c46a45e1`,
`git status` reports no modified tracked file, and the working copy of
`specs/039-context-groups/tasks.md` hashes equal to
`git show HEAD:specs/039-context-groups/tasks.md`, both `239d549328d0`.
`specs/039-context-groups/clarifications-round4.md` is new and
untracked. It records no answer. It drops two candidates that match F5
and F7, and reads both as wording with no decision behind them.

- F13 stands. Over all 50 tasks, with a range such as "FR-029 to
  FR-032" read as its members, `tasks.md` names 39 of the 40
  requirements, and the string `FR-037` occurs zero times. T044 at
  `:371-375` cites VI, the three units and `LCOV_EXCL`, and cites
  FR-037 nowhere.
- F4 stands. T030 at `:297-303` names the section, the region, its
  quick-reference rows, `SECTIONS`, `SECTION_OWNERS` and the page cap at
  `tools/docs/example_drift.py:222`, and names no `| Option |` row.
  T015 at `:160-164` names the guide row only. `example_drift.py:763-776`
  still reports a long option that has no option row on the main page,
  and the suite runs that scan with no flag (`test/CMakeLists.txt:1553`).
- F1, F2 and F3 stand. SC-001 at `spec.md:804-807` still names `l3`, and
  `plan.md:311-312` still shows two contexts in one `l3` group. T003 at
  `tasks.md:49-55` still moves the version with no standing test named,
  and T020 at `tasks.md:205-212` still seats the teardown node after
  `launchBodies` returns. `test/source/views_test.cpp:246-253`,
  `source/harness/placement_preset.cpp:127-131` and
  `source/harness/runner.cpp:2115-2157` stand as first read. A later
  `tasks.md` reached no checkout visible here, so a copy elsewhere needs
  its own re-read for F4 and F13.
