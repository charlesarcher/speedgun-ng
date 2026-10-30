# Analysis round 3: specs/039-context-groups

Analyzed on 2026-10-30 on branch `039-context-groups` at HEAD `Meta: Clear the analysis round-2 findings`, the commit that answers
round 2's nine findings and states the claim this pass tests:

> Two more would have misled a reader or a task. The CLI table test admits
> no value band holding `299`, so T011 now moves the block constants and
> the `inNewBlock` predicate beside the row count.

The read `git diff master...HEAD` names 14 files: 13 under
`specs/039-context-groups/` beside `.specify/feature.json`, so the source
tree stands as master's. Basis: the constitution, then `spec.md`,
`plan.md`, `tasks.md`, `data-model.md`, the two files under `contracts/`,
`checklists/requirements.md` and the four clarification rounds, then a
re-read of the cited source, of the host's own topology, and of the gates
the lane runs. Gate reads at HEAD: `prose-lint` over `bea6af5b..HEAD`
answers 23 sources, 3346 units and 0 findings, and `example_drift` exits 0
with no flag. No build directory stands in the tree, so the host readings
come from the resolver's arms read against `/sys`.

This pass read the wave order against the standing checks each wave
touches, the ten rewritten task lines (T011, T012, T015, T025, T033 to
T038), the report-stamp sites, the two record kinds against the host the
spec names, and the four clauses that state the publication route.

## Closure of round 2

- G1. CLOSED. T012 lands the `README.md` option row, the guide row beside
  `--placement` and the `PAGE_LINE_CAP` move with its reason in one commit
  (`tasks.md:142-155`), and FR-039 states the order in the spec
  (`spec.md:790-795`). `README.md` measures 940 lines against a cap of 940
  (`tools/docs/example_drift.py:222`), so wave 3 owns the move its own line
  forces. The plan's risk row still names M-H as the cap's home
  (`plan.md:306-308`), and wave 6's section move is real work, so that row
  reads short by one wave.
- G2. CLOSED. T025 names four `1.8` literals, two comments, two later-minor
  plants and the three splices, in the words "move the three reads that cut
  the stamped value by the width of `1.8` to a read of the value's own
  extent" (`tasks.md:267-284`). The suite holds exactly three such reads:
  `test/source/harness_json_load_test.cpp:99`, the same file at `:1076`, and
  `test/source/harness_json_schema_test.cpp:79`. No fourth fixed-width read
  of the stamp stands under `test/`, and `docs/schema/report-v1.json`
  carries no `schemaVersion` const to move.
- G3. CLOSED. T011 now names the block constants and bounds at `:250-268`
  with the `inNewBlock` predicate at `:832-848` "so the lane block holds the
  value `299`" (`tasks.md:134-141`), and T012 names the slot: the row sits
  ahead of `hunt` beside `os-noise` (`tasks.md:144-146`), index 29 of 39,
  with `kRowsBeforeHunt` moving from 29 to 30. The predicate admits 290 to
  296, 260 and 297 today (`test/source/harness_cli_table_test.cpp:844-848`),
  which is the hole round 2 named.
- G4. OPEN. The host half moved: T034 now states "the host run,
  `--placement=core` over the host's own two L3 groups, sixteen contexts,
  disclosing two groups of eight" beside "the seam run, the named test and
  installed shape that carries SC-001's four groups of two"
  (`tasks.md:355-361`). The `core` clause reads right: the arm seats one
  context per core over 16 cores (`placement_preset.cpp:123-126`, the place
  test at `:191-198`), so `gary` answers two groups of eight. The `l3`
  clause of the same sentence names no context count, and the only count the
  sentence carries is sixteen. That arm offers two places on this host, so
  sixteen contexts reach the fit refusal at exit 2 and no console block
  names two single-member groups; two contexts answer that disclosure.
- G5. CLOSED. The listing now carries `l3-0` with slots `[0,1]` and `l3-1`
  with `[2,3]`, and the diagram below pairs `ctx0` with `ctx1` and `ctx2`
  with `ctx3` (`plan.md:316-333`). Four contexts over four L3 groups of two
  cores each touch two groups under the `core` arm, so caption, listing and
  diagram read one shape now.
- G6. CLOSED. The guide row moved into T012, the task that lands the table
  row (`tasks.md:148-151`), and T015 reads the option surface
  (`tasks.md:170-177`). `harness_cli_table_test` compares the guide's option
  table against the live table (`test/source/harness_cli_table_test.cpp:1498-1583`),
  so the two move in one commit and the test's verify reads green.
- G7. CLOSED. D-7 now keeps "the standing route" for "a run with no
  partition, and a run whose single group holds every context"
  (`spec.md:306-308`), and FR-026 cuts at the same place
  (`spec.md:721-727`), with D-9 and the edge row holding the one-group run
  on the standing participant set (`spec.md:316-319`, `:548-550`). H3 names
  the two lines that still state the slot route with no count.
- G8. CLOSED. The identity reads now cite FR-036 (`spec.md:77`) and "R-5,
  FR-036, FR-038" (`spec.md:83`), the requirements that state the synthetic
  seam rule and the record rule. H1 names six citations elsewhere in the
  same file that still point at requirements stating something else.
- G9. CLOSED. Table C now holds 16 rows in FR-018's clause order, one clause
  per row (`contracts/group-layout.md:47-62`), so T007's "every fault row of
  the layout contract's table C" (`tasks.md:91-98`), `plan.md:221`'s "the 16
  fault shapes of FR-018", SC-004's 18 cases (`spec.md:834-836`) and T037's
  "all eighteen cases" (`tasks.md:371-373`) count one set. The tail "each
  asserting one line naming the path and the entry" reads past the contract
  for the nine rows whose `Line names` column names no entry, and that
  column beside FR-021 (`spec.md:700-704`) governs the text, so no count
  rests on the sentence.

## New findings

### H1. MINOR: six citations name requirements that state something else

- Artifact: `spec.md:35-39`, `:45-51`, `:53-57`, `:283-287`, `:299-308`,
  `:433-434`.

> - Option values: the free block `298` to `319` stands open ... so this
>   lane's one new row takes `299` and the block comment names the split
>   (FR-010). The hunt block `280` to `289` stands specs/033's.

The tree of the artifacts: FR-010 (`spec.md:635-640`) states that a
one-context instance carries no group and no threaded round, and the row
rule with the value `299` stands in FR-011 (`spec.md:641-648`). The other
five read the same way.

| Line | Names | The rule stands in |
| --- | --- | --- |
| `spec.md:39` | FR-002 | FR-032 (`:751-755`) |
| `spec.md:51` | FR-025 | FR-030 (`:744-747`), FR-032 |
| `spec.md:57` | FR-010 | FR-011 (`:641-648`) |
| `spec.md:287` | FR-014 | FR-016 (`:665-668`) |
| `spec.md:308` | FR-021 | FR-026, already beside it |
| `spec.md:433` | FR-016 | FR-018 (`:674-682`) |

FR-002 fixes group membership, FR-025 fixes the ungrouped pair of rounds,
FR-014 refuses the key `cpus`, FR-021 fixes the text of a refusal line, and
FR-016 fixes the vendored reader, so a reader who follows each citation
lands on a different rule than the citing sentence states. Round 2's G8
cleared the two identity reads of the same shape; this pass read every
`FR-` citation outside the requirement list against the requirement it
names and found these six.

Remediation: swap the six citations for the requirements the sentences
state: FR-032, FR-032, FR-011, FR-016, FR-026 alone, and FR-018.

### H2. MINOR: T015 names a file that holds no `--help` expectation

- Artifact: `tasks.md:170-177`, the M-D test plan at `plan.md:233-235`.

> Read the option surface the new row touches:
> `test/source/harness_cli_sources_test.cpp` for the source table, the
> `--help` bytes of `test/source/harness_cli_test.cpp`, and the standing
> replay of `test/base_cli/run_matrix.py`

The tree: `test/source/harness_cli_test.cpp` names `--help` nowhere; it
holds usage-status cases and option effects read from the written report,
which the new row reaches. The help bytes stand at
`test/source/harness_cli_table_test.cpp:1183-1206`, inside the file T011
moves, and that check builds its expectation from `optionRows()`, so T012's
row covers it and no move stands owed. The other two sites read right:
`test/source/harness_cli_sources_test.cpp:2478` walks the live table with no
fixed count, and `test/base_cli/run_matrix.py` replays 26 cases whose
`--help` case carries no console record
(`test/data/base_cli_console.txt:21-25`).

Remediation: point the second site at
`test/source/harness_cli_table_test.cpp:1183-1206`.

### H3. MINOR: the plan's M-E row and T020 state the slot route with no count

- Artifact: `plan.md:241-246`, `tasks.md:219-228`, against D-7
  (`spec.md:299-308`) and FR-026 (`spec.md:721-727`).

> T020 [4] Add the per-attempt publication slot beside the outcome vector:
> context 0 writes the setup text then the ready flag with release order,
> every other body waits on that flag with acquire order before it copies
> the text (FR-026, D-7).

The tree: FR-026's second sentence keeps "the standing publication and the
standing teardown point" for a run that carries no partition or one group
holding every context, and D-7 reads the same way since round 2. A layout of
one entry is legal (D-9, SC-005's one-group case at `spec.md:837-840`), so a
grouped attempt of one group stands named in the standing route by the spec
and in the slot route by these two lines. T020 cites FR-026 alone, and the
M-E row carries the same unconditional wording.

Remediation: name the count in T020's opening clause, "Add the per-attempt
publication slot for an attempt whose partition holds more than one group",
and the M-E row reads off the task it names.

### H4. MINOR: the plan's M-E row reverses which barriers take whose place

- Artifact: `plan.md:238-240`, against D-1 (`spec.md:240-243`), FR-022
  (`spec.md:705-708`), FR-025 (`spec.md:718-720`) and T019
  (`tasks.md:212-218`).

> the attempt builds one start barrier and one stop barrier per group, and
> the two whole-run barriers take their place (`:2066-2067`)

The tree: `source/harness/runner.cpp:2066-2067` holds the two whole-run
barriers today, T019 builds one `sg::ContextBarrier` per group per point
there, and FR-025 keeps the standing pair for a run that carries no
partition. Read as written, the clause hands the place back to the whole-run
barriers, which is the shape D-1 reserves for the ungrouped run.

Remediation: rewrite the clause as "the per-group barriers take the place of
the two whole-run barriers at `:2066-2067`".

### H5. MINOR: T039's verify reads false one wave after it lands

- Artifact: `tasks.md:378-381`, `tasks.md:430-432`, `plan.md:101`, SC-010
  (`spec.md:856-858`), FR-038 (`spec.md:781-789`).

> Write `specs/039-context-groups/quickstart.md`, the record index naming
> the question each record answers, and the re-run commands (FR-038,
> SC-010). Verify: every file under `results/` stands linked from the
> quickstart.

The tree: T050 writes `results/verdict.md` in wave 8, one wave behind the
quickstart, and no task returns to `quickstart.md`. SC-010 and FR-038 ask
for links to the acceptance records, and the landed precedent keeps its
verdict off the index: `specs/038-report-view-generators/quickstart.md`
links `results/run_views.sh`, `results/placement.md`, `results/tracers.md`,
`results/views/` and `results/reports/` and names `results/verdict.md`
nowhere, while `plan.md:101` states that `results/` holds "one record per
acceptance run + verdict". T039's wording covers every file, so the
invariant the task records reads false at the lane's own close.

Remediation: scope the verify to "every record the index names", or name the
verdict link in T050.

## Checks that passed

- Consistency: `prose-lint` over `bea6af5b..HEAD` answers 23 sources, 3346
  units and 0 findings, and `example_drift` exits 0 with no flag.
- Consistency: eight waves, and each verify command names tests that resolve
  in `test/CMakeLists.txt` today or land in the wave that registers them. 26
  CTest names appear across the artifacts, 21 resolve now, and the five that
  do not are the lane's new files, each named "with their registration" by
  the task that writes it: T001, T005, T007, T010, T017.
- Consistency: 40 requirements appear across 50 tasks once a range such as
  "FR-029 to FR-032" expands, and the plan's coverage map carries 40 rows.
- Wave 3: the table holds 38 rows today (`optionRows()` in
  `source/harness/options.cpp`), the contract literal moves to 39, and the
  three readers of the row take it live: the table test parses the source
  text, `test/source/harness_cli_sources_test.cpp:2478` walks `optionRows()`,
  and the help check counts its lines off the same call. The base matrix
  holds no preset case and no help record, so the row moves no replay.
- Wave 4: no standing gate counts barriers; `test/barrier_shape.sh` gates
  the optimization barrier and `test/loop_shape.sh` the timed loop, and the
  round change touches neither.
- Wave 5: an appended disclosure segment breaks no standing assertion. Every
  check either searches a substring (`countLinesWith` at
  `test/source/harness_placement_report_test.cpp:212-218`, `lineHolding` at
  `test/source/harness_context_report_test.cpp:425-433`, `find` at
  `test/source/harness_numa_placement_test.cpp:823`) or records an ungrouped
  run byte for byte (`test/source/harness_threads_test.cpp:2141-2143`,
  `harness_placement_report_test.cpp:305-311`). The two recorded preset runs
  (`example/docs/jitter_probe.expected:101`,
  `example/docs/numa_placement.expected:5`) move with T027, whose verify
  regex reaches every `report_shape` case, and
  `example/docs/reading_the_report.expected:24` prints `NO_PIN` and stands.
- Wave 5: the schema stamp's only fixed-width reads are the three T025
  names, and `docs/schema/report-v1.json` holds no `schemaVersion` const.
- Wave 6: the drift gate's `SECTIONS` (`tools/docs/example_drift.py:33-65`),
  `SECTION_OWNERS` (`:70-102`) and `PAGE_LINE_CAP` (`:222`) are the sites
  T012 and T030 name, the option scan reads the `| Option |` rows off
  `README.md` (`:763-776`), and the guide's table pairs a page row with a
  table row by name (`test/source/harness_cli_table_test.cpp:1516-1534`), so
  the page row's place reads free.
- Measured: `gary` reads 16 cores, 32 threads, two L3 groups and one memory
  node from `/sys/devices/system/cpu`. The resolver's place count is the core
  count under `core`, the L3 group count under `l3`
  (`source/harness/placement_preset.cpp:127-131`) and the mask-touching cpu
  count under `node` (`:147-191`), so `core` with sixteen contexts answers
  two groups of eight and `node` with any count up to 32 answers one group
  named `node-0`. Those are the readings T034's `core` clause and T035's host
  clause state.
- Measured: the two-node shape stands installed at
  `test/source/harness_cli_placement_test.cpp:260-301`, the arm T035's
  citation covers, and at `installNodeLayout` in
  `test/source/harness_numa_placement_test.cpp:133-175`, and T017 names the
  two-node scenario the record points at.
- Consistency: D-7, FR-026, D-9 and the edge row cut the publication route at
  more than one group, and the one-group run keeps the standing rounds and
  the standing publication, so the lane states one route per shape.
- Grounding: 118 explicit `path:line` reads across the seven artifacts land
  inside their files, and each bare `:NNN` read lands in the file its
  sentence names.
- Counts: 40 requirements, 50 tasks, 10 success criteria, 16 fault clauses in
  FR-018, 16 rows in the layout contract's table C, 8 rows in the partition
  contract's table C, 18 refusal cases in SC-004 with FR-019's two pairings
  (`spec.md:231-232`), 38 option rows in the table today, and 940 lines in
  `README.md` at the cap.
- Consistency: the plan's documentation tree lists the first two
  clarification rounds beside the `analysis-roundN.md` pattern
  (`plan.md:89-102`), and rounds 3 and 4 stand as their own files beside it;
  no task and no gate reads that tree.

## Verdict

Round 2's remediation cleared eight of its nine findings, and G9's count now
reads one set across four artifacts. This pass filed no BLOCKER and no
MAJOR: G4 stays open on one missing count in T034, and five minors sit in
`spec.md`, `plan.md` and `tasks.md` wording. The wave order holds: no wave
from 3 to 8 closes with a red test on the readings measured above, the row
value 299 reaches an admitted band, the stamp move covers every fixed-width
read, and the disclosure move reaches the two recorded preset runs inside
its own wave. The lane needs three file edits and six one-line moves before
implement, each landing in the artifact that already owns the sentence, and
a fourth analysis round reads nothing new behind them.
