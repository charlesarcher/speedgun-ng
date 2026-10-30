# Analysis round 2: specs/039-context-groups

Analyzed on 2026-10-30 on branch `039-context-groups` at HEAD `a4ceb8d8`
(`Meta: Clear the analysis round-1 findings`). Its tree equals the tree
of `1c0d22f3`, the remediation commit the parent named, and the read
`git diff 1c0d22f3 HEAD` answers no difference. The read
`git diff master...HEAD` names thirteen files, all under
`specs/039-context-groups/` beside
`.specify/feature.json`, so the source tree stands as master's. Basis:
the constitution, then the current `spec.md`, `plan.md`, `tasks.md`,
`data-model.md`, the two files under `contracts/`,
`checklists/requirements.md` and the four clarification rounds with their
`## Reconciliation` sections, then a re-read of the cited source. Shape
claims were measured: the harness topology walk
(`source/harness/topology.cpp:132-212`) and the resolver arms
(`source/harness/placement_preset.cpp:105-212`) were replayed against the
vendored reader (`external/hwloc/hwloc/topology-synthetic.c` through the
built `libhwloc_embedded.a`) for the synthetic descriptions the plan
names. Gate reads at HEAD: `prose-lint` over `bea6af5b..HEAD` answers 21
sources, 2981 units and 0 findings, and `example_drift` exits 0 with no
flag and with `--options`. A citation sweep over the seven artifacts
resolves 113 `path:line` reads across 29 files with no start line past an
end of file, and a verify sweep resolves every CTest name the artifacts
run against the 146 names `test/CMakeLists.txt` registers.

## Closure of round 1

- F1. CLOSED. SC-001 names `--placement=core` over a layout of four L3
  groups holding eight cores (`spec.md:817-820`), and that shape's `core`
  plan answers the group sequence `0,0,1,1,2,2,3,3`
  (`placement_preset.cpp:113-126` with the `resize` at `:199-200`), which
  is four groups of two named `l3-0` to `l3-3`. The same shape drives
  US1 (`spec.md:362-366`, `:379-382`), D-3 (`spec.md:263-267`), T017
  (`tasks.md:186-187`) and T034 (`tasks.md:338-342`), and G4 names the
  evidence half of that move.
- F2. CLOSED. T003 moves "the three checks of
  `test/source/views_test.cpp:241-254`" with the CMake block
  (`tasks.md:53-54`), and that block is the tree's only standing read of
  the moved values: `0.25.0` and `SOVERSION` appear in no other test
  source.
- F3. CLOSED. T020 keeps the fixture pair inside context 0's own body and
  adds the settlement flags (`tasks.md:212-217`), where the tree puts the
  pair: `callbackState` at `runner.cpp:2117`, `fixture` at `:2118`, the
  standing teardown at `:2151-2157`. FR-026 (`spec.md:718-724`), D-7
  (`spec.md:299-305`) and the plan's M-E row (`plan.md:241-246`) read the
  same route, and G7 names the one shape where two of them disagree.
- F4. CLOSED. FR-011 states the main page's option row in the same change
  (`spec.md:642-644`), T015 names that row and the drift read that
  compares it (`tasks.md:164-166`), and T012 lands the table row in the
  same wave (`tasks.md:140-147`). G1 names a gate the move then trips.
- F5. CLOSED. `plan.md:221` reads "the 16 fault shapes of FR-018" and
  FR-018's clause list counts 16 clauses (`spec.md:671-679`). G9 names a
  count the same test plan still splits.
- F6. CLOSED. The edge row now reads "the run names no group"
  (`spec.md:552-554`), beside FR-010 (`spec.md:632-634`) and B-1's
  postcondition (`contracts/group-partition.md:36-38`).
- F7. CLOSED. The entity now reads "a name and its slots"
  (`spec.md:802-803`), and the refused form stands at FR-014
  (`spec.md:654-657`), D-6 (`spec.md:288-298`) and Out of scope
  (`spec.md:348-350`).
- F8. CLOSED. T025 names `harness_noise_report_test.cpp:423` and `:453`
  with the comments at `:411` and `:451` (`tasks.md:260-264`), and those
  four lines carry the writer stamp, the byte fixture and the two
  comments that name it.
- F9. CLOSED. `plan.md:301-305` counts three files and four places, and
  the tree holds that stamp or its needle at
  `harness_json_report_test.cpp:546`,
  `harness_json_load_test.cpp:707`, `harness_noise_report_test.cpp:423`
  and `:453`; Scale/Scope's "three existing test files" (`plan.md:53-54`)
  matches T025. G2 names work the same move needs beyond the literals.
- F10. CLOSED. T018's verify names `harness_threads_test`
  (`tasks.md:201`); all 24 CTest names across the artifacts' verifies
  resolve to entries registered in `test/CMakeLists.txt`.
- F11. CLOSED. `plan.md:71` and `plan.md:478` spell `dbc-gate`, the name
  `cmake/dbc-gate.cmake:40-41` defines as a custom target.
- F12. CLOSED. `spec.md:902-904` and `plan.md:76` now state the window
  rule with no round count, and no artifact names a round-count field.
- F13. CLOSED. T044 cites FR-037 beside VI (`tasks.md:384`), and every
  one of the 40 requirements appears in at least one task line once a
  range such as "FR-029 to FR-032" expands.
- F14. CLOSED. `plan.md:51` reads "Three new harness translation units"
  and the structure listing names those three (`plan.md:113-115`).
- F15. CLOSED. The index carries the round 2 and round 3 rows
  (`plan.md:172-174`), the R-2 row narrows to "the partition-naming
  pairing refuses" (`plan.md:161`), and M-D names "the partition read the
  pairing refusal needs" (`plan.md:230-231`).
- F16. CLOSED. `contracts/group-partition.md:19` reads "No entry point".
- F17. CLOSED. T027 names "the inline quotes at
  `docs/pages/harness.md:971` and `README.md:607`" (`tasks.md:282-283`),
  and both lines carry the disclosed placement field inline.

## New findings

### G1. BLOCKER: the main page row T015 adds breaks the page cap in wave 3

- Artifact: `tasks.md` T015 at `:162-168`, FR-011 at `spec.md:642-644`,
  FR-039 at `spec.md:787-789`.

> Add the option-reference row for `--group-layout` to
> `docs/pages/harness.md` beside the `--placement` row at `:680`, the
> text the table's help string copies, and the matching `| Option |`
> row in `README.md` that `table_rows` compares against
> `cli_options` (`example_drift.py:763-776`) (FR-011, FR-039).

The tree: `README.md` measures 940 lines today, and `PAGE_LINE_CAP` is
940 (`tools/docs/example_drift.py:222`), so `check_sections` reports a
finding for a page of 941 lines (`tools/docs/example_drift.py:638-639`).
That scan runs on every flagless read, which is how the suite runs it
(`test/CMakeLists.txt:1552-1556`). The cap move stands in wave 6 alone
(`tasks.md:310-311`), so wave 3 closes with `example_drift` red from
T015's own added line, and T015's verify runs that test.

Remediation: name the cap move at `tools/docs/example_drift.py:222` in
T015, the task that adds the page line.

### G2. BLOCKER: the schema move breaks three fixed-width splices no task names

- Artifact: `tasks.md` T025 at `:258-270`, `plan.md:301-305`, the M-G
  module row at `plan.md:256-265`, wave 5 at `plan.md:418-420`.

> move the `1.8` literals to `1.10` in
> `test/source/harness_json_report_test.cpp:546`,
> `test/source/harness_json_load_test.cpp:707` and its `1.9` plant at
> `:1076`, and `test/source/harness_noise_report_test.cpp:423` and
> `:453` with the comments at `:411` and `:451`

The tree: three sites cut the stamped value out of a written document by
width, and that width is the width of `"1.8"`.
`test/source/harness_json_load_test.cpp:96-101` replaces 5 characters at
`find("\"schemaVersion\":") + 16`, `:1076` does the same for its own
plant, and `test/source/harness_json_schema_test.cpp:79` replaces the
same 5 characters with `1` to plant a value of the wrong kind. A 6
character stamp `1.10` leaves a stray quote: the wrong-kind plant
becomes `{"schemaVersion":1",` and the later-minor plant becomes
`{"schemaVersion":"1.9"",`, and both answer a parse complaint, so the
pointer check at `test/source/harness_json_schema_test.cpp:110-115` and
the load check at `test/source/harness_json_load_test.cpp:104-105`
report. Wave 5's verify runs both tests (`tasks.md:269`, `tasks.md:278`).
The same move leaves the two `1.9` plants at `:103` and `:1076` stating a
later minor that `1.10` made earlier, and T025 moves one of the two.

Remediation: add the three width reads at `harness_json_load_test.cpp:99`
and `:1076` and `harness_json_schema_test.cpp:79` to T025, with the stamp
length as the width each cut takes.

### G3. MAJOR: no table position admits the row value 299

- Artifact: `tasks.md` T011 at `:134-139`, T012 at `:140-147`, FR-011 at
  `spec.md:638-645`, the M-D row at `plan.md:229`.

> Move `test/source/harness_cli_table_test.cpp` to the thirty-ninth row:
> the contract literal at `:209-248`, the row-count check at `:681-689`,
> the block-disjointness partition at `:858-939`, and the guide table
> byte-compare at `:1498-1583` (FR-011). Verify:
> `ctest --test-dir build -R harness_cli_table_test --output-on-failure`
> red on the row count.

The tree: the block rule sorts each row into a band by index, master
below `kMasterRows` and hunt at or above `kRowsBeforeHunt`
(`test/source/harness_cli_table_test.cpp:880-899` with the bounds at
`:253-254`), and the band predicates admit a letter, 256 to 259, 260, 270
to 289, 290 to 296 and 297 (`:835-848`, constants at `:261-268`). The
value FR-011 fixes, 299, stands in no band, so the row reports wherever
it lands: a master slot answers "a letter or a constant from 256 to
259", a hunt slot answers "a constant from 270 to 289", and a new slot
answers that same list, because `inNewBlock` stops at 297. The test's own
comment states that 298 to 319 stands free for the lanes that follow
(`:256-260`), and no predicate reads that block. The three lines
that decide the verdict, `:253-254` and `:844-848`, stand outside every
range T011 names. T012 names no slot either (`tasks.md:141-143`).

Remediation: add `test/source/harness_cli_table_test.cpp:253-254` and
`:844-848` to T011's site list, and name the slot T012 fills.

### G4. MAJOR: T034's record names a shape no command on a named host answers

- Artifact: `tasks.md` T034 at `:338-342`, FR-038 at `spec.md:778-783`,
  SC-001 at `spec.md:817-820`, the lane identity at `spec.md:78-83`.

> Record the `core` grouping over the eight-core, four-L3-group shape in
> `results/preset-core.md`, and the `l3` grouping's single-member groups
> beside it: the commands, the exit codes, the console blocks naming the
> groups, and the report documents beside them (FR-038, SC-001).

The tree: the shape is a machine with four L3 groups of two cores each.
Neither host the spec names has it, and no command reaches it: the two
seams that build a layout are harness-internal installs
(`source/harness/detail/internal.hpp:790`, `:823`) that a CLI process
never calls, and the precedent record drives CLI binaries on the host and
prints the host's own read (`specs/035-numa-placement/results/nps1.md`,
which measures `l3-groups=4 cores=24 threads=48`). Over that shape the
`core` arm seats six contexts in each of four units, so eight contexts
answer two groups holding six and two: the standing installed shape
models it at `test/source/harness_cli_placement_test.cpp:260-289`. Over
the lane host, two L3 groups of eight cores each (`spec.md:80-83`, and
the host's `index3` cache directories agree: cpus 0 to 7 with 16 to 23
stand against cpus 8 to 15 with 24 to 31), the same eight contexts
answer one group.

Remediation: in T034, name the host shape the record's command runs and
point SC-001's four-groups-of-two proof at the `harness_group_run_test`
scenario that installs it.

### G5. MINOR: the plan's logical view still carries the round 1 membership

- Artifact: the logical view at `plan.md:316-338`.

>       the core arm over four L3 groups that hold eight cores:
>                         partition = [ Group{name:"l3-0", slots:[0,1]},
>                                       Group{name:"l3-1", slots:[2,3]} ]

The tree: the caption names four groups and the listing carries two, and
the context rows below still pair `ctx0` with `ctx4` and `ctx1` with
`ctx5` (`plan.md:327-331`), the stride four contexts deal under `l3`
(`placement_preset.cpp:127-131`). The sequence the caption's own shape
produces is `0,0,1,1,2,2,3,3`, so `l3-2` holds slots `[4,5]` and `l3-3`
holds `[6,7]`, and `ctx4` stands in `l3-2`.

Remediation: rewrite the two context rows and the listing to the four
groups the caption names.

### G6. MINOR: T012's verify cannot read green while the guide row waits for T015

- Artifact: `tasks.md` T012 at `:140-147`, T015 at `:162-168`.

> `ctest --test-dir build -R "harness_group_cli_test|harness_cli_table_test" --output-on-failure`
> green on the row and the help line.

The tree: `harness_cli_table_test` compares the guide's option table
against the live table and reports when the two counts differ
(`test/source/harness_cli_table_test.cpp:1504-1513`), so the test stays
red from the moment T012's row lands until T015 adds the guide row
(`tasks.md:162-164`), and `ctest` exits nonzero over both names. The
wave's own checkpoint at T016 stands unaffected
(`tasks.md:169-171`).

Remediation: move the guide row into T012, the task that lands the table
row.

### G7. MINOR: D-7 and FR-026 state two publication routes for a one-group run

- Artifact: `spec.md` D-7 at `:299-305`, FR-026 at `:718-724`, D-9 at
  `:313-316`, the edge row at `:545-547`, T020 at `tasks.md:210-219`.

> A grouped run publishes context 0's setup outcome to every body
> through a route that carries no round across groups, and the ungrouped
> run keeps the standing route where its start round carries the
> publication (FR-021, FR-026).

The tree of the artifacts: D-7 draws the line between a grouped run and
an ungrouped run, while FR-026 draws it at the round count and keeps "the
standing publication and the standing teardown point" for a run whose
contexts meet one round (`spec.md:723-724`). A layout with one entry is
legal (D-9, US2 scenario 3 at `spec.md:415-418`) and its contexts meet
one start round, so the two clauses name different routes for it, and
T020 states the slot route with no condition.

Remediation: read D-7's `A grouped run` as a partition above one group,
in `spec.md:302`.

### G8. MINOR: two lane identity reads cite requirements that state something else

- Artifact: `spec.md:74-77` and `spec.md:79-83`.

> The suite stays host-agnostic: every layout a test exercises stands
> built on the synthetic seam, and the suite passes under
> `taskset -c 0,1` and on a two-cpu host (FR-030).

The tree: FR-030 states the report schema's minor and its key
(`spec.md:741-744`), and the seam and the two-cpu rule stand in FR-036
(`spec.md:768-772`), which `plan.md:38` cites for the same rule. The next
bullet cites FR-032 (`spec.md:83`) for a claim about the seam and the
records, and those rules stand in FR-036 and FR-038 (`spec.md:778-783`).

Remediation: point both citations at FR-036, and add FR-038 beside the
records clause at `spec.md:82`.

### G9. MINOR: the layout test is counted three ways

- Artifact: `tasks.md` T007 at `:91-98`, `contracts/group-layout.md:45-60`,
  `plan.md:221`, SC-004 at `spec.md:828-830`.

> every fault row of the layout contract's table C, each asserting one
> line naming the path and the entry (FR-012 to FR-018, FR-021, B-1, A-1
> to A-6)

The tree of the artifacts: table C holds 14 rows
(`contracts/group-layout.md:46-60`) and two of them carry two clauses
each, `:54` for the empty `slots` and the wrong member kind and `:56` for
the repeated slot and the unnamed context, so 14 rows cover FR-018's 16
clauses (`spec.md:671-679`). SC-004 counts 18 cases with FR-019's two
pairings (`spec.md:828-830`) and T037 records eighteen
(`tasks.md:349-351`). A scenario per row answers 14, two short of the
criterion the same plan row counts at 16 shapes.

Remediation: read T007's row list as the 16 fault shapes across table C's
14 rows.

## Checks that passed

- Consistency: `prose-lint` over `bea6af5b..HEAD` answers 21 sources,
  2981 units and 0 findings, and `example_drift` exits 0 with no flag and
  with `--options`.
- Grounding: 113 `path:line` reads across the seven artifacts resolve to
  29 files with no start line past an end of file, and every CTest name
  in the artifacts' verify commands resolves to one of the 146 names
  `test/CMakeLists.txt` registers.
- Consistency: 40 requirements appear across 50 tasks once ranges
  expand, and `FR-037` appears once, in T044 at `tasks.md:384`.
- Counts: FR-018 lists 16 clauses, table C holds 14 rows naming all 16,
  SC-004 counts 18 cases with FR-019's two pairings, `plan.md:221` names
  16 shapes, and T037 records 18.
- Measured: `L3:4 Core:2 PU:2` reads 16 threads, 8 cores and 4 L3 groups,
  and the `core` arm's group ordinals for eight contexts answer
  `0,0,1,1,2,2,3,3`, four dense groups of two. `Node:2 L3:2 Core:2 PU:2`
  answers the same sequence. `Core:8 PU:2` answers `kNoGroup` at every
  position, which is the read FR-006 (`spec.md:617-623`) and the round 3
  ruling turn into the ungrouped shape, and the allowed pairing T010
  tests (`tasks.md:128-129`).
- Measured: the standing installed four-group shape
  (`test/source/harness_cli_placement_test.cpp:260-289`, four groups of
  six cores and two threads) answers `0,0,0,0,0,0,1,1` for eight contexts
  under `core`, so the eight-core shape T017 names is a new installed
  shape, exactly as `plan.md:288-292` states.
- Soundness: the ledger fault runs on the thread that runs the attempt
  (`runner.cpp:1955-1961`) and precedes that thread's own settle poll
  (`:1970-1979`), so a stranded member ends in the contract abort ahead of
  every wait the harness performs, the new flag wait included, because
  that wait sits inside a body.
- Soundness: no new wait sits inside a per-context window. The setup half
  runs before the start arrival (`runner.cpp:2119-2126`), the standing
  teardown runs after the stop arrival (`:2149-2157`), and the settlement
  wait T020 adds stands on that same side of the stop arrival.
- Soundness: a pin that lands nowhere stands disclosed as the unapplied
  word (`source/harness/execution_context.cpp:107-150`), so a grouped
  scenario whose installed layout names cpus outside a two-cpu mask still
  runs, which is what T043's read and FR-036 need.
- Soundness: the reader checks the major alone
  (`source/harness/json_load.cpp:2424-2442`), so the 19 fixtures under
  `test/data` stamped `1.0`, `1.1`, `1.4` and `1.8` keep loading at
  `1.10` and the one fixture stamped `2.0` keeps refusing. The page
  checks at `test/source/views_test.cpp:888` and `:890` read the recorded
  fixture `test/data/report_views/multi.json`, so both moves leave
  `views_test` green beyond the three checks T003 moves.
- Grounding: the two option tables are keyed by name, at
  `test/source/harness_cli_table_test.cpp:1516-1534` and
  `tools/docs/example_drift.py:763-776`, so a row's position binds the
  C++ table's block rule alone.
- Grounding: `dbc-gate` stands as a custom target
  (`cmake/dbc-gate.cmake:40-41`), `kNoGroup` stands at
  `source/harness/detail/internal.hpp:51-52`, the free option block
  stands at 298 to 319 with 297 taken by the noise tracer
  (`source/harness/detail/options.hpp:79-80`, `:93`), and `README.md`
  holds 940 lines against the cap of 940.

## Verdict

Not implementable as written. Two BLOCKER findings, two MAJOR findings,
five MINOR findings, nine total, and every one of round 1's 17 findings
stands closed.

The smallest fix set: name the page cap move in T015 (G1), name the three
fixed-width splices in T025 (G2), name the two band lines in T011 and the
row's slot in T012 (G3), and rewrite T034's record to a command the lane
host answers (G4). Those four edits clear every gate that blocks a wave.
G5, G6, G7, G8 and G9 are one-line repairs to the plan's diagram, the
guide row's task, D-7's first clause, two citations and one count.
