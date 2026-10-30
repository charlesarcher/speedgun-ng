# Analysis round 4: specs/039-context-groups

Read on 2026-10-30 on branch `039-context-groups`, judging the head the
lane held mid-pass, which stands now as `840caf31`
(`Harness: Add group to the disclosure record`): the split landed after
this pass read, and it left `12a3e3ec`
(`Meta: Task the message re-read`) beside that disclosure unit. That
tree stands three commits past `5d244efa`
(`Meta: Clear the analysis round-3 findings`), the commit that answered
round 3's open G4 and its five minors. `14ffeb2a`
(`CMake: Move the version to 0.26.0`) landed wave 1's version unit,
`840caf31` landed wave 1's disclosure unit, and `12a3e3ec` carries two
task edits: `T046` now clears the side build trees before the spell and
format reads, and `T051` re-reads every lane commit message. The
`tasks.md` numbers below are read at that tree, and the next landing,
`e93655f7` (`Meta: Name the analyzer gate`), widened `T004` and `T045`
and moves a number written after `T004` by four lines and one written
after `T045` by six. The tree no longer stands as master's, the fact
round 3's provenance recorded the other way: `git diff master...HEAD`
names 20 files, 14 under `specs/039-context-groups/` beside
`.specify/feature.json`, `CMakeLists.txt`, `test/CMakeLists.txt`,
`include/speedgun-ng/benchmark.hpp` and
`test/source/harness_group_report_test.cpp`. Basis: the constitution,
then `spec.md`, `plan.md` and `tasks.md` read whole, the two files under
`contracts/`, then a re-read of every cited site in the source, the
docs, the schema, the gate scripts and `/sys`, then the code of the two
landed units. Gate reads: the prose check over `bea6af5b` to the judged
tree answers 19 sources, 3563 units and 0 findings; the commit check
over the same range exits 1 with one finding, `XI2.CONTRASTIVE` on the
body `Meta: Clear the analysis round-3 findings` carried before `T051`
rewrote it, which the open `T051` owns. Wave 1's two touched tests pass
in the green side tree (`views_test`, `harness_group_report_test` ), and
the red side tree holds the test object with no binary, the shape the
plan's Execution mode asks for.

## Closure of round 3

- G4. CLOSED. `specs/039-context-groups/tasks.md:362-363` names the count:

  > `--placement=l3` with two contexts disclosing two single-member
  > groups; and the seam

  The `l3` arm answers one place per L3 group
  (`source/harness/placement_preset.cpp:127-131`, and the count test at
  `:191-198`), and `/sys` on this host reads two L3 groups over 16 cores, so
  two contexts reach two groups of one and the sixteen of the `core` clause
  reach two groups of eight.
- H1. CLOSED. The six citations stand swapped in the six sentences that
  needed them: `specs/039-context-groups/spec.md:39` reads `(FR-032, D-2)`,
  `:50-51` reads `(R-4, FR-030, FR-032)`, `:57` reads `(FR-011)`, `:287`
  reads `(FR-016)`, `:308` reads `(FR-026)`, and `:433` names the list
  FR-018 states. Each named requirement now states the rule of the sentence
  that cites it.
- H2. CLOSED. `specs/039-context-groups/tasks.md:174-176` points the help
  expectation at `test/source/harness_cli_table_test.cpp:1183-1206`, where
  `theHelpAnswerPrintsOneLinePerRow` builds its expectation from
  `optionRows()`, and the verify at `tasks.md:177` dropped
  `harness_cli_test`, the file that names `--help` nowhere.
- H3. CLOSED. `specs/039-context-groups/tasks.md:227-229` reads "The slot
  route stands only where the partition holds more than one group" and names
  both standing shapes, and the plan's M-E row says the same at
  `specs/039-context-groups/plan.md:244-249`, so task and plan read one cut
  beside D-7 (`spec.md:299-308`) and FR-026 (`spec.md:721-727`).
- H4. CLOSED. `specs/039-context-groups/plan.md:238-241` reads "a
  partitioned run builds one start barrier and one stop barrier per group in
  place of the two whole-run barriers" and keeps the standing pair for a run
  that carries no partition, which is FR-022 beside FR-025 and D-1.
- H5. CLOSED. `specs/039-context-groups/tasks.md:385-386` scopes the
  quickstart verify to `T033`'s script and the five records of `T034` to
  `T038`, and `tasks.md:441-444` moves the verdict link into `T050` with a
  verify that the quickstart links it, so the invariant holds at the close
  of wave 7 and again at the close of wave 8.

## New findings

### J1. MINOR: two version-block citations name the two-sources decision

- Artifact: `specs/039-context-groups/spec.md:39`,
  `specs/039-context-groups/spec.md:616`.

> - Version: master stands at `0.25.0` (`CMakeLists.txt:30`). The lane
>   takes the next free minor, `0.26.0`. ... and the commit records the
>   check (FR-032, D-2).

> - **FR-005**: ... the project version, the harness `SOVERSION` and the
>   archive `SOVERSION` SHALL stand at the values FR-032 records, with the
>   ABI check named in the commit that moves them (D-2).

The tree: D-2 stands at `spec.md:245-253` and states the two partition
sources, the pairing refusal and the `none` preset's standing shape. It
names no version, no `SOVERSION`, no archive number and no ABI check, and no
decision in D-1 to D-12 states them. FR-032 (`spec.md:751-755`) states the
four numbers and the landing re-read alone. This pass read the 57 `D-`
mentions outside the Decisions section against the decision each names, the
surface round 3's citation audit left unread, and these are the two that do
not land. The two range mentions (`spec.md:18`, `plan.md:91`) and the plan's
decision index list rulings, and every other one names a decision that
states the rule of the sentence that cites it. Line 39 is one of the six
passages round 3's remediation rewrote, and the rewrite moved the `FR` half
of the pair and left the `D` half.

Remediation: read the version bullet's pair as `(FR-032)` and FR-005's
tail with no decision citation; `D-10` (`spec.md:320-325`) is the
decision that names the per-context record, should FR-005 want one
there. The fix landed with this record: `214a3189`
(`Meta: Close the analysis loop`) reads the version bullet's pair as
`(FR-032)` and FR-005's tail as `(FR-032, D-10)`.

Nothing else stands. The three files hold no clause that contradicts another clause,
no citation that names the wrong requirement beside the two above, no verify command
that names an unregistered test or a path outside the tree, and no wave that closes
red on the readings measured below.

## Checks that passed

- Counts: 40 requirements (`spec.md:601-798`), 51 tasks (`T001` to `T051`,
  no gap, no repeat), 10 success criteria, and 40 rows in the plan's FR
  coverage map (`plan.md:442-481`). All 40 FR names appear across the tasks
  once a range such as "FR-029 to FR-032" expands.
- Counts: FR-018 (`spec.md:674-682`) lists 16 fault clauses in the order the
  layout contract's table C rows stand
  (`specs/039-context-groups/contracts/group-layout.md:47-62`, 16 rows), so
  `plan.md:221`'s "the 16 fault shapes of FR-018", `T007`'s "every fault row
  of the layout contract's table C" (`tasks.md:94-95`) and SC-004's 18 of 18
  with FR-019's two pairings (`spec.md:231-232`, `:834-836`) and `T037`'s
  "all eighteen cases" (`tasks.md:376-377`) count two sets that agree: 16
  grammar faults and 2 pairings. The partition contract's table C holds 8
  rows (`contracts/group-partition.md:67-74`).
- Citations: 112 explicit `path:line` reads, 49 bare `:NNN` reads whose
  sentence names the file, and 78 path-only reads across the three files.
  Every explicit read lands inside its file, every bare read resolves to the
  file its sentence names, and the 25 path-only targets that do not exist
  are the files this lane writes.
- Wave 1: `harness_group_report_test` now registers
  (`test/CMakeLists.txt:1049-1055`), so 21 of the 25 distinct CTest names in
  the verify commands resolve today and the four that do not are the lane's
  remaining new executables, each named "with their registration" by the
  task that writes it: `harness_group_partition_test` (`T005`),
  `harness_group_layout_test` (`T007`), `harness_group_cli_test` (`T010`),
  `harness_group_run_test` (`T017`).
- Wave 1's header change: `sg::ContextDisclosure` gained
  `std::string group = {}` as its last member with a `\pre` and `\post`
  block (`include/speedgun-ng/benchmark.hpp:463-481`). The five construction
  sites (`source/harness/runner.cpp:1662`, `source/harness/json_load.cpp:1891`,
  `test/source/harness_placement_report_test.cpp:172`,
  `test/source/harness_context_report_test.cpp:238`,
  `test/source/harness_pass_plan_test.cpp:902`) use designated or partial
  initialization, so the added member breaks none, and the doc gate fails an
  in-scope function that lacks `\pre` or `\post` and an
  invariant-bearing class that lacks `\invariant`, so a plain member owes no
  third tag.
- Pointer drift measured and filed as nothing: the audit readings
  `CMakeLists.txt:30`, `:95` and `:333` and `benchmark.hpp:454-462` were
  exact on `bea6af5b`, and wave 1's comment block moved the version line to
  36, the archive number to 101, the harness number to 345 and the record to
  454 to 482. The block names its base
  ("Repository: `master` at `bea6af5b`", `spec.md:87`), the task that edited
  those lines landed against the values, and the open task that re-reads
  them, `T049` (`tasks.md:433-436`), names the four bindings with no line
  number, so no open instruction points at a moved line.
- Wave 3: `optionRows()` answers 38 rows (`source/harness/options.cpp:470-714`)
  and the table test's contract holds 38
  (`test/source/harness_cli_table_test.cpp:209-248`), `kRowsBeforeHunt`
  stands at 29 (`:254`) with `hunt` at index 29, and `inNewBlock` (`:844-848`)
  admits no value above 297, so `T011`'s move and `T012`'s row close one gap
  inside one wave and the help line and the guide row move with it
  (`:1186-1206`, `:1498-1583`). `README.md` measures 940 lines against a
  `PAGE_LINE_CAP` of 940 (`tools/docs/example_drift.py:222`), which is the
  move `T012` carries in its own commit.
- Wave 3: the environment-source walk
  (`test/source/harness_cli_sources_test.cpp:2478-2481`) skips every row that
  carries no environment source, so the new row adds it no case and `T015`'s
  narrowed verify names both files its move list reaches.
- Wave 5: the stamp move reaches every fixed-width read. `T025` names the
  four `1.8` literals (`test/source/harness_json_report_test.cpp:546`,
  `test/source/harness_json_load_test.cpp:707`,
  `test/source/harness_noise_report_test.cpp:423` and `:453`), the two
  comments (`:411`, `:451`) and the three reads that cut the value by its
  width (`test/source/harness_json_load_test.cpp:99`, `:1076`,
  `test/source/harness_json_schema_test.cpp:79`), and each site reads as
  stated. The other `1.8` readers under `test/`
  (`test/source/views_test.cpp:400`, `:534`, `:664`, `:812`, `:890`, `:998`
  and the five documents under `test/data/report_views/`) read the bytes of
  documents they write themselves, so the move reaches none of them, and
  `docs/schema/report-v1.json:11-13` states `schemaVersion` as a plain
  string, so no const moves there. The context item stands where `T026`
  cites it (`docs/schema/report-v1.json:386-402`).
- Wave 5: the disclosure samples `T027` names hold the lines it quotes
  (`docs/pages/harness.md:596`, `:971`, `:979`, `:1568`, `README.md:607`).
- Measured: `/sys/devices/system/cpu` reads 32 threads over 16 cores with
  two L3 instances shared by `0-7,16-23` and `8-15,24-31`, so the host
  answers two L3 groups of eight cores. The `core` arm seats one context per
  core (`source/harness/placement_preset.cpp:123-126`), so `T034`'s sixteen
  contexts disclose two groups of eight, and the `l3` arm seats one context
  per group with the fit refusal above that count (`:127-131`, `:191-198`),
  so its two contexts disclose two single-member groups. `T017`'s installed
  four-group shape carries SC-001's four groups of two
  (`test/source/harness_cli_placement_test.cpp:260-297`).
- Route wording: D-7 (`spec.md:299-308`), FR-026 (`spec.md:721-727`), the
  plan's M-E row (`plan.md:236-249`) and `T020` (`tasks.md:220-229`) cut the
  slot route at more than one group and keep the standing publication for no
  partition and for one group holding every context. The same four cut the
  barrier route at "no partition", which is FR-022 beside FR-025, and D-9
  (`spec.md:316-319`) holds the one-group run on the standing participant
  set, so each route states one rule per shape.
- Records: `T039`'s index reaches `T033`'s script and the five records, and
  `T050` adds the verdict beside them, so FR-038's "the quickstart SHALL
  link every record" (`spec.md:781-789`) and SC-010's "five records at
  least" (`spec.md:856-858`) read satisfied at both closes, beside the
  precedent the landed lane sets:
  `specs/038-report-view-generators/quickstart.md` links its script, its
  records and its two artifact directories, and names `results/verdict.md`
  in no link.
- Coverage of the story list: US3's Independent Test (`spec.md:433-434`)
  names FR-018's list and its scenario 7 carries the pairing, and SC-004
  counts the 18 with FR-019's two pairings, so the pairing reads covered by
  the criterion that counts it.
- Landed tasks and their boxes: wave 1 landed two units with every box in
  `tasks.md` unchecked, and `T050` carries "mark the landed tasks in this
  file" at wave 8 (`tasks.md:441-443`). No rule in the constitution's X.4
  fixes the marking moment, so the deferral reads as the lane's own choice.

## Verdict

The lane is ready to implement: round 3's six sites stand closed, wave 1
has landed green with its two tests passing on the judged tree, and the
one minor citation this pass filed is cleared in `214a3189`, so the
three artifacts carry no open defect.