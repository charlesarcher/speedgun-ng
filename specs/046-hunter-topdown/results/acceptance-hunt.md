# The acceptance hunt on the standing host

The hunt ran three times at head `cbeb678c` and once at the base. Every run
answered exit 0, ranked 497 events, and printed one `area` cell per ranked row,
so SC-007 has a reading from hardware behind it.

## The commands

T112 names `--timeout 120`. The harness carries no option of that name, and a
run with it answers `unknown option '--timeout'`. The hunt takes its bound from
`--hunt-budget`, so each run passed `--hunt-budget 120s`. The default budget is
`90s`. tasks.md carries the wrong option name, and the correction stands here.

```sh
./build/example/memory_walk --hunt --hunt-budget 120s
./build/example/first_benchmark --hunt --hunt-budget 120s
```

## The host

- Host `CorsairOne`, kernel `Linux 7.3.0-4-cachyos x86_64`, 24 cpus.
- Intel Core i9-9920X, model 85, the part the `arch/x86/skylakex/` table names.
- `kernel.perf_event_paranoid` at -1, set by the owner for this work.
- gcc 16.2.1 and clang 23.1.3.
- Version line: `speedgun-ng 0.31.0 build=Release`.

## The ranked rows

`memory_walk` planned 497 events over 249 passes and kept 493 after four gaps.
`first_benchmark` answered the same counts from the same catalog. Each run
grouped the ranked rows the same way.

| Rows | Kind | Values the cell answers |
|---|---|---|
| 58 | A rank that names a node of the tree | `backend_bound` 27, `frontend_bound` 17, `retiring` 11, `bad_speculation` 3 |
| 412 | A rank that names a group of the listing | `cache` 145, `memory` 113, `pipeline` 74, `frontend` 37, `virtual-memory` 25, `floating-point` 12, `other` 6 |
| 27 | A rank that neither the tree nor the listing names | `unmapped` |

The top four ranks read the slots and bubbles events and answer `unmapped`.
Ranks 8 to 12 read cache events and answer `cache`. The three counts stand at
58, 412 and 27 in all three head runs.

## The level-1 block

The run prints the block after the ranked rows. Its first two lines and its four
node rows follow verbatim, with the 13 table files and their hashes between them.

```text
topdown: level=1 passes=4 table=arch/x86/skylakex/
topdown: reference=linux-7.2.4
topdown:  topdown.backend_bound  tma_backend_bound  not available  tma_backend_bound > 0.2
topdown:  topdown.bad_speculation  tma_bad_speculation  not available  tma_bad_speculation > 0.15
topdown:  topdown.frontend_bound  tma_frontend_bound  not available  tma_frontend_bound > 0.15
topdown:  topdown.retiring  tma_retiring  not available  tma_retiring > 0.7 | tma_heavy_operations > 0.1
```

`not available` is the disclosure `source/harness/cli.cpp:521` sets where a node
carries no figure, so the block states the refusal of each level-1 node beside
the rule that would have set it. The block reaches the console because the
companion pass asks for the tree once per run, and the pass draws its figures
from the passes the hunt already pays for.

## What the base answered

The same hunt at `b1dd7670` printed 505 lines and no top-down block, and its
version line read `0.30.0`. Each head run prints 524 lines, of which the block
and its file list account for 19. The area cell is the second new text, and no
ranked row lost its event or its figure line.

Rank order is not a cross-run invariant on this host. Two head runs of the same
binary differ at 75 of 497 positions, and the head and the base differ at 93, so
no rank claim rests on these readings. FR-010 rests on the committed fixtures:
the emitted sets and the no-hunt console bytes stand byte identical, and the
example goldens hash unchanged.

## Acceptance closeout corrections (2026-11-08)

SC-IC-5 of `specs/048-hwlat-probe/spec.md:1676-1679` (read at `af54d3f0`) owns the shape of this
section. Every line above keeps its text and its heading. Each row below names
the line it replaces and gives the sentence that replaces it. IH-002 of
`specs/046-hunter-topdown/spec.md:1317-1321` (read at `af54d3f0`) names the criterion beside each
row, and the ruling at `specs/048-hwlat-probe/spec.md:1839-1842` (read at `af54d3f0`) says a row
names the audit point or the ruling where no `SC-` id owns the claim. This section
was written against `af54d3f0`, the head this branch carries. A cite into a
spec, plan or tasks file reads at `af54d3f0`, and a source or page anchor reads
at the merged base `a55728af`, where it stands unchanged.

No row here adds a figure of its own from a run. The figures inside a quoted
claim are the standing text of the line they come from. Every other figure
names the source line it came from. IA46-004
(`specs/046-hunter-topdown/tasks.md:1244-1249` (read at `af54d3f0`)) runs the two commands of
`## The commands` at the closeout head, and the ruling at
`specs/048-hwlat-probe/spec.md:1808-1811` (read at `af54d3f0`) keeps a figure of one head inside the
row that names that head.

- **Row 1. The ranked count at `:4`.** Owner: SC-007
  (`specs/046-hunter-topdown/spec.md:1034-1039` (read at `a55728af`)). Quoted claim, verbatim from
  `:4`:

  ```text
  answered exit 0, ranked 497 events, and printed one `area` cell per ranked row,
  ```

  What answers differently: `source/harness/hunt_plan.cpp:371-374` (read at `a55728af`) gives a rank
  to the first `min(top, ranked)` addresses, and the default `top` stands at 20
  (`include/speedgun-ng/hunt.hpp:135` (read at `a55728af`)). `source/harness/report.cpp:1478` (read at `a55728af`) prints
  every address that carries no rank as a `hunt: rank -` row.
  `source/harness/report.cpp:1471` (read at `a55728af`) prints the area cell on a ranked row, and
  `source/harness/report.cpp:1488` (read at `a55728af`) prints the same cell on the row that closes
  with a `mark=` cell. FR-013 (`specs/046-hunter-topdown/spec.md:583-589` (read at `a55728af`)) and
  FR-014 (`specs/046-hunter-topdown/spec.md:590-596` (read at `a55728af`)) state that pair.

  Replacement sentence, for the sentence that stands at `:3-5`: "Every run
  answered exit 0 and printed one `area=` cell per printed row, the ranked rows
  and the `hunt: rank -` rows alike. The ranked count stands at the
  `--hunt-top` default of 20 (`include/speedgun-ng/hunt.hpp:135` (read at `a55728af`)), so SC-007
  has a reading from hardware behind it."

- **Row 2. The per-rank reading at `:39-40`.** Owner: SC-008
  (`specs/046-hunter-topdown/spec.md:1040-1043` (read at `a55728af`)) beside the audit point AP-10
  (`specs/046-hunter-topdown/spec.md:284-294` (read at `a55728af`)). Quoted claims, verbatim from
  `:39` and `:40`:

  ```text
  The top four ranks read the slots and bubbles events and answer `unmapped`.
  Ranks 8 to 12 read cache events and answer `cache`.
  ```

  What answers differently: a rank follows the rate the run measured, so the
  rank of one address moves between runs of one binary. AP-10 records one
  unchanged binary putting one address first in one run and outside the ranked
  rows in the next. AP-9 (`specs/046-hunter-topdown/spec.md:268-283` (read at `a55728af`)) states
  the split of the ranked rows as the reading of one run.
  `source/harness/hunt_plan.cpp:371-374` (read at `a55728af`) takes the ranks from that measurement,
  so a rank range names no kind of area cell across runs.

  Replacement sentence, for the two sentences that stand at `:39-40`: "A rank
  follows the measurement of its own run, so no rank range names a kind of area
  cell across runs. The cells of the ranked rows stand recorded run by run in
  the consoles of IA46-004
  (`specs/046-hunter-topdown/tasks.md:1244-1249` (read at `af54d3f0`))."

- **Row 3. The citation at `:57-58`.** Owner: SC-007, the criterion behind the
  block this sentence describes
  (`specs/046-hunter-topdown/spec.md:1034-1039` (read at `a55728af`)). Quoted claim, verbatim across
  `:57` and `:58`:

  ```text
  `not available` is the disclosure `source/harness/cli.cpp:521` sets where a node
  carries no figure,
  ```

  What answers differently: `source/harness/cli.cpp:521` (read at `a55728af`) reads
  `record.disclosure = "not available" + placementTail;` inside
  `buildJitterReportRecord`, the unavailable answer of the jitter probe
  (`source/harness/cli.cpp:505` (read at `a55728af`)). The node line of the top-down block comes
  from the branch of `printTopdownBlock` at
  `source/harness/report.cpp:404-411` (read at `a55728af`), which prints a node's two names and the
  words `not available` where the record holds no figure for the node.

  Replacement sentence, for the first sentence of the paragraph at `:57-61`:
  "`not available` is the text `source/harness/report.cpp:404-411` (read at `a55728af`) prints where
  a node carries no figure, so the block states the refusal of each level-1
  node beside the rule that would have set it."

- **Row 4. The digests the section describes at `:45-46`.** Owner: IA46-002
  (`specs/046-hunter-topdown/spec.md:1267-1273` (read at `a55728af`)) asks the record for the file
  digests the block prints. No `SC-` id owns a digest, so the ruling at
  `specs/048-hwlat-probe/spec.md:1335-1338` (read at `a55728af`) and the digest ruling at
  `specs/048-hwlat-probe/spec.md:1847-1850` (read at `af54d3f0`) stand named with the row. Quoted
  claim, verbatim from `:46`:

  ```text
  with the 13 table files and their hashes between them.
  ```

  What answers differently: the fence at `:48-55` holds the level line, the
  reference line and the node rows, and it holds no `topdown: file` line.
  `source/harness/report.cpp:381-384` (read at `a55728af`) prints one line per contributing file of
  the selected directory, in the shape `topdown: file <name> <digest>`.
  IA46-004 (`specs/046-hunter-topdown/tasks.md:1244-1249` (read at `af54d3f0`)) records those lines
  at the length the binary prints them.

  Replacement sentence, for the clause that stands at `:46`: "and the
  `topdown: file` lines between them stand printed in the consoles of IA46-004,
  each digest at the length the binary prints it."

- **Row 5. The level of the area ask, named by no row.** No `SC-` id names an
  ask level, so the level ruling at
  `specs/048-hwlat-probe/spec.md:1339-1341` (read at `a55728af`) owns the row, beside IH-001
  (`specs/046-hunter-topdown/spec.md:1306-1316` (read at `af54d3f0`)) and FR-033
  (`specs/046-hunter-topdown/spec.md:549-560` (read at `a55728af`)). Standing line: `:49` carries
  the one level the section names, and no row names the level of the ask that
  settles the area cells.

  What answers differently: `source/harness/detail/hunt_topdown.hpp:42` (read at `a55728af`) holds
  `inline constexpr int kTopdownAreaLevel {6};`, and
  `source/harness/hunt_topdown.cpp:265` (read at `a55728af`) hands that value to
  `TopdownSelection::level` of the area ask. The companion hands
  `kTopdownCompanionLevel`, the value 1 at
  `source/harness/detail/internal.hpp:1340` (read at `a55728af`), at
  `source/harness/hunt_topdown.cpp:289` (read at `a55728af`). D-17
  (`specs/046-hunter-topdown/spec.md:178-184` (read at `a55728af`)) states that the two asks of one
  run differ in that figure alone.

  Sentence the row adds to the paragraph at `:57-61`: "The block prints the
  level-1 ask of `kTopdownCompanionLevel`, and the area cells come from the ask
  at `kTopdownAreaLevel`, the value 6 at
  `source/harness/detail/hunt_topdown.hpp:42` (read at `a55728af`), so one run holds its two asks
  apart (FR-033, D-17)."

- **Row 6. The Intel `PERF_METRICS` pending row.** Owner: SC-010
  (`specs/046-hunter-topdown/spec.md:1056-1059` (read at `a55728af`)), and IH-002
  (`specs/046-hunter-topdown/spec.md:1317-1321` (read at `af54d3f0`)) for the pending row with its
  reason. Standing line: this record carries no pending row, and
  `results/acceptance-final-intel.md:159-160` and `:210-212` (both read at `a55728af`) name the Arm, the
  Intel hybrid and the Zen 4 rows, and name no `PERF_METRICS` row. Quoted
  claim, the class SC-010 lists, verbatim from
  `specs/046-hunter-topdown/spec.md:1056` (read at `a55728af`) and `:1057`:

  ```text
  - **SC-010**: The parts with no host (Intel parts measured through
    `PERF_METRICS`, Intel hybrid, Arm, AMD Zen 4 and Zen 5 while no lane has a gap)
  ```

  What answers differently: `results/acceptance-amd-zen5.md:178-179` (read at `a55728af`) keeps that
  class pending, and `tasks.md:1185` (read at `a55728af`) and `:1188` record it pending. The
  `PERF_METRICS` row of `specs/042-topdown/quickstart.md:136` (read at `a55728af`) states that no
  host there carries the facility, and D-8 of that spec
  (`specs/042-topdown/spec.md:106-109` (read at `a55728af`)) settles synthetic acceptance for those
  parts. The hunter record of the Intel host carried no row for them.

  Row the section adds, with the reason SC-010 carries now:

  ```text
  - Intel parts measured through `PERF_METRICS`: pending, because no host on
    this lane carries the facility. The class keeps its synthetic seam rows and
    carries no hardware figure here (SC-010, IH-002).
  ```


- **Row 7. The block header cite inside FR-033.** Owner: IC-006
  (`specs/048-hwlat-probe/spec.md`, this closeout's correction clause) with
  SC-IC-5, which keeps an older line standing and adds a dated row beside it.
  Standing line: `specs/046-hunter-topdown/spec.md:554` (read at `a55728af`) cites
  `source/harness/report.cpp:365-368` (read at `a55728af`) for the block header that prints
  `passes=`. What answers differently: at this head the `std::printf` of that
  header opens at `source/harness/report.cpp:367` (read at `a55728af`) and its argument list ends at
  `:370`, so the cited span starts two lines early and stops one line short of
  the call. FR-033 keeps its text and its number, and this row carries the
  cite the tree answers. The rule FR-033 states holds as written: the header
  prints `passes=` from the collection that ran, and the two asks differ in
  `TopdownSelection::level` alone.

## Corrections at the merged base (2026-11-09)

This block was written against `af54d3f0`, the head this branch carries, whose
base is the merged head `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the base
head that the `Base head` row of `The base this work reads` of
`specs/046-hunter-topdown/spec.md` (read at `af54d3f0`) names.
Each cite prints the head it was read at. A cite into a spec or a plan names a
requirement id, a ruling number with its list, a heading, or a table row by its
first cell, because a spec line number moves under every writer of the tree.
Text past the base length of a spec reads at `af54d3f0`: the base copy of
`specs/048-hwlat-probe/spec.md` ends at 1476 lines, the base copy of
`specs/046-hunter-topdown/spec.md` at 1273 lines, and the base copy of
`specs/046-hunter-topdown/tasks.md` at 1239 lines. That
text is this branch's own and stands nowhere at the base, where the `SC-IC-`
ids are absent from all three specs. Every source, header and page anchor was
opened at `a55728af`, and this branch's delta is markdown-only, so those files
stand unchanged at `af54d3f0`. A bare `:n` in this block names the artifact the sentence names last,
and this file when it names none; a line of this file at or below 74 stands
at `a55728af` too, and a line above reads at `af54d3f0`.

SC-IC-5 of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), the criterion
under `Success Criteria - this extension`, owns
the shape of this section. Every line above keeps its text and its heading.
Each row below names the line it replaces and gives the target that holds,
beside the head its target was read at. No row below adds a figure of its own
from a run, and no row here carries a blocked reading.

- **Row 1. The ruling cite at `:82`.** Owner: IH-002 of
  `specs/046-hunter-topdown/spec.md` (read at `af54d3f0`), the requirement under
  `Requirements restated for the merged base (IH-001 to IH-002)`, which asks a
  row that carries no `SC-` id to name the ruling that owns it. Quoted claim
  from `:82-83`, with the line range it carried elided in brackets:

  ```text
  the ruling at `specs/048-hwlat-probe/spec.md` […] says a row
  names the audit point or the ruling where no `SC-` id owns the claim.
  ```

  What that cite lands on: closeout ruling 14, "The probe tool surface stays
  `counter_probe`", and that ruling speaks about the tools a probe run may use.
  Ruling 16 holds the meaning of the line: closeout ruling 16 under
  `Session 2026-11-08, round 1 (continued): hardware and records` of
  `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), "A criterion id
  comes from the spec that owns the claim", which states that a claim with no
  `SC-` id gets the audit point or the ruling that owns it, named in the row.

  Replacement clause, for the clause that stands at `:82-83`: "closeout ruling
  16 of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) says a row names
  the audit point or the ruling where no `SC-` id owns the claim."

- **Row 2. The ruling cite at `:93-94`.** Owner: closeout ruling 10 of
  `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), the head-label rule the line means.
  Quoted claim from `:93-94`, with the line range it carried elided in brackets:

  ```text
  `specs/048-hwlat-probe/spec.md` […] keeps a figure of one head inside the
  row that names that head.
  ```

  What that cite lands on: the close of closeout ruling 8, "Owned means the file
  list", and the opening of closeout ruling 9, "The hunt claim about a live
  catalog needs one run", both under `Session 2026-11-08, round 1` of that spec.
  Ruling 10 holds the meaning of the line: closeout ruling 10, "The record
  labels each head it measured, and the reruns are not carries", under the same
  heading, and it states that a
  figure keeps the head that printed it as its label. A lane that wants a figure
  at another head runs the lane again and writes its own row.

  Replacement clause, for the clause that stands at `:92-94`: "closeout ruling
  10 of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) keeps a figure of
  one head inside the row that names that head."

- **Row 3. The digest ruling cite at `:170`.** Owner: IA46-002, a
  requirement id of `specs/046-hunter-topdown/spec.md` (read at `a55728af`) under
  `Requirements - the hunter part (IA46-001 to IA46-002)`, which asks the record
  for the level-1 companion block and its source. The block itself stands in this
  record under `The level-1 block` at `:43`, the fence at `:48-55`. Quoted claim
  from `:169-170`, with the two line ranges it carried elided in brackets:

  ```text
  `specs/048-hwlat-probe/spec.md` […] and the digest ruling at
  `specs/048-hwlat-probe/spec.md` […] stand named with the row.
  ```

  What the second cite lands on: the close of closeout ruling 15, "Rank movement
  gets both denominators, per command", and the opening of closeout ruling 16,
  the criterion-id rule. Ruling 18 holds the meaning of the line: closeout
  ruling 18 under `Session 2026-11-08, round 1 (continued): hardware and
  records` of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), "The digest
  block prints verbatim", and it binds the digests, their length from the
  binary, and the level line that stands beside them.

  Replacement clause, for the clause that stands at `:169-170`: "ruling 13 of
  the hunter part under `Session 2026-11-07, round 1` of
  `specs/048-hwlat-probe/spec.md` (read at `a55728af`), `The table named`, and
  closeout ruling 18 (read at `af54d3f0`) stand named with the row."

- **Row 4. The companion-level anchor at `:201`.** Owner: FR-033 with D-17,
  both ids of `specs/046-hunter-topdown/spec.md` (read at `a55728af`), the pair that row cites. Quoted
  claim, verbatim from `:200-202`:

  ```text
  `kTopdownCompanionLevel`, the value 1 at
  `source/harness/detail/internal.hpp:1340`, at
  ```

  What stands at that anchor: `source/harness/detail/internal.hpp:1338-1340` (read at `a55728af`)
  declares `groupLayoutFitStatus`, and `:1340` holds the return type of that
  declaration. The constant stands at
  `source/harness/detail/internal.hpp:1363` (read at `a55728af`), where the file reads `inline
  constexpr std::uint64_t kTopdownCompanionLevel {1};`. Pull request #100 moved
  that file, and it merged as the base head of this phase, so the line numbers of
  the file moved with it
  (the paragraph under `The base this work reads` of
  `specs/046-hunter-topdown/spec.md` (read at `af54d3f0`) names the file among
  the ten paths pull request #100 moved, and the opening paragraph of `Intel
  acceptance closeout at the merged head (2026-11-08)` of
  `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) names it too). The hand-over the same
  sentence cites stands where that sentence puts it:
  `source/harness/hunt_topdown.cpp:289` (read at `a55728af`) reads
  `ask.topdownLevel = kTopdownCompanionLevel;`.

  Replacement clause, for the clause that stands at `:200-202`:
  "`kTopdownCompanionLevel`, the value 1 at
  `source/harness/detail/internal.hpp:1363` (read at `a55728af`), at
  `source/harness/hunt_topdown.cpp:289` (read at `a55728af`)."

- **Row 5. The SC-IC-5 cite at `:78`.** That line opens the section above with a
  line range into `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) whose
  first line falls inside SC-IC-4, the criterion that speaks about the
  `dbc-gate` target, so the cite names the criterion above the one it means.
  SC-IC-5 is the id to name, the criterion under `Success Criteria - this
  extension` that binds every row a closeout writes. Replacement clause, for the
  clause that stands at `:78`: "SC-IC-5 of `specs/048-hwlat-probe/spec.md` (read
  at `af54d3f0`), the criterion under `Success Criteria - this extension`, owns
  the shape of this section." This row keeps the older line's text and its
  heading, as SC-IC-5 asks.

## Pass 1 control figures at the candidate head (2026-11-09)

Pass 1 of ruling 29 of `Rulings - this phase (29 to 42)` of
`specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) ran the whole final-head set
at a candidate head. The run stood on `af54d3f0`, the short form of
`af54d3f0e52901339e8703fc7991eb3da0db4435`, the sha `git rev-parse af54d3f0`
prints, whose base is the merged head `a55728af`
(`a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`). Every figure below is a Pass 1
control figure measured at `af54d3f0`, and a Pass 2 run at the head Pass 2 runs
at replaces each of them, so no row here answers `IA46-004` of
`specs/046-hunter-topdown/tasks.md` at the head Pass 2 runs at. No standing row
moves and no standing table changes its text: a figure that belongs beside a
standing table lands here as a dated row that names that table by its heading.

Each figure names one step log under `/tmp/ic/p1/`, and each log opens with its
own command line, its own load witness and its own exit. Ruling 30 of the same
list holds a figure where that lane's own witness line reports no other
compiler, test, capture or trace process and a one-minute load average below
2.0. Every hunt console of this pass prints a one-minute average between 1.31
and 1.62 and reports `other ctest or compiler processes: 0`, so every figure in
this section stands, and none reads void.

**The three walk consoles.** Dated rows beside `The ranked rows`, which keeps its
text. The command `./build/example/memory_walk --hunt --hunt-budget 120s` of
`The commands` ran three times in the release tree `build` at `af54d3f0`, each
run alone, each at exit 0. Each console prints the plan line for 497 events over
249 passes, twenty numbered rank rows, and 477 further rows that close with a
`mark=` cell, so the printed rows carry the `area=` cell over the 497 events the
plan line names. The summary line of each console keeps 493 of those 497 after
four gaps.

Console `f6-walk-1.log`, exit 0, load witness `1.33 2.18 2.72 2/1569 3096576`, at `af54d3f0`. That witness adds `other ctest or compiler processes: 0`. The block prints the plan line, the benchmark line, the twenty ranked rows, the four `hunt: skipped` lines and the summary line in the order the log prints them, and leaves out the 477 `hunt: rank -` rows that stand between the last ranked row and the skipped lines.

```text
hunt: plan events=497 passes=249 method=PASSES estimate=938979ns budget=120s stage1=497 stage2=60
bmMemoryWalk             iterations=  15201621  real time/iter (ns)=   45.392  cpu time/iter (ns)=   45.286  overhead floor (ns)=  682.000  endpoint samples=2  items_per_second=22081948.978  bytes_per_second=88327795.912
hunt: rank 1 cpu/topdown-fetch-bubbles instr=2067.127 cv=3.45 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 2 cpu/topdown-slots-issued instr=1409.674 cv=0.91 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 3 cpu/topdown-total-slots instr=1353.406 cv=0.88 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 4 cpu/topdown-slots-retired instr=1136.229 cv=0.40 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 5 cpu/ref-cycles instr=1008.885 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 6 cpu/branch-instructions instr=227.048 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 7 cpu/L1D_PEND_MISS.PENDING_CYCLES_ANY instr=113.524 cv=6.10 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 8 cpu/mem-stores instr=82.922 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 9 cpu/branch-misses instr=17.769 cv=5.56 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 10 cpu/bus-cycles instr=8.885 cv=6.66 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 11 cpu/CORE_SNOOP_RESPONSE.RSP_IFWDFE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 12 cpu/CORE_SNOOP_RESPONSE.RSP_IFWDM instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 13 cpu/CORE_SNOOP_RESPONSE.RSP_IHITFSE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 14 cpu/CORE_SNOOP_RESPONSE.RSP_IHITI instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 15 cpu/CORE_SNOOP_RESPONSE.RSP_SFWDFE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 16 cpu/CORE_SNOOP_RESPONSE.RSP_SFWDM instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 17 cpu/CORE_SNOOP_RESPONSE.RSP_SHITFSE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 18 cpu/L1D_PEND_MISS.FB_FULL instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 19 cpu/L2_LINES_OUT.USELESS_HWPF instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 20 cpu/L2_LINES_OUT.USELESS_PREF instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: skipped cpu/INST_RETIRED.TOTAL_CYCLES_PS gap
hunt: skipped cpu/INT_MISC.CLEARS_COUNT gap
hunt: skipped cpu/INT_MISC.CLEAR_RESTEER_CYCLES gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

Console `f6-walk-2.log`, exit 0, load witness `1.31 2.16 2.71 1/1566 3096646`, at `af54d3f0`. That witness adds `other ctest or compiler processes: 0`. The block prints the plan line, the benchmark line, the twenty ranked rows, the four `hunt: skipped` lines and the summary line in the order the log prints them, and leaves out the 477 `hunt: rank -` rows that stand between the last ranked row and the skipped lines.

```text
hunt: plan events=497 passes=249 method=PASSES estimate=925782ns budget=120s stage1=497 stage2=60
bmMemoryWalk             iterations=  15728296  real time/iter (ns)=   45.999  cpu time/iter (ns)=   45.903  overhead floor (ns)=  681.000  endpoint samples=2  items_per_second=21784866.931  bytes_per_second=87139467.722
hunt: rank 1 cpu/topdown-fetch-bubbles instr=2186.575 cv=6.15 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 2 cpu/topdown-slots-issued instr=1406.713 cv=0.07 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 3 cpu/topdown-total-slots instr=1378.085 cv=3.18 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 4 cpu/ref-cycles instr=1153.011 cv=7.53 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 5 cpu/topdown-slots-retired instr=1136.229 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 6 cpu/branch-instructions instr=227.048 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 7 cpu/L1D_PEND_MISS.PENDING instr=141.165 cv=5.24 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_split_loads
hunt: rank 8 cpu/mem-stores instr=82.922 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 9 cpu/branch-misses instr=17.769 cv=6.19 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 10 cpu/bus-cycles instr=6.910 cv=7.87 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 11 cpu/CORE_SNOOP_RESPONSE.RSP_IFWDFE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 12 cpu/CORE_SNOOP_RESPONSE.RSP_IFWDM instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 13 cpu/CORE_SNOOP_RESPONSE.RSP_IHITFSE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 14 cpu/CORE_SNOOP_RESPONSE.RSP_IHITI instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 15 cpu/CORE_SNOOP_RESPONSE.RSP_SFWDFE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 16 cpu/CORE_SNOOP_RESPONSE.RSP_SFWDM instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 17 cpu/CORE_SNOOP_RESPONSE.RSP_SHITFSE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 18 cpu/IDI_MISC.WB_DOWNGRADE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 19 cpu/IDI_MISC.WB_UPGRADE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 20 cpu/L1D_PEND_MISS.FB_FULL instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: skipped cpu/INST_RETIRED.TOTAL_CYCLES_PS gap
hunt: skipped cpu/INT_MISC.CLEARS_COUNT gap
hunt: skipped cpu/INT_MISC.CLEAR_RESTEER_CYCLES gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

Console `f6-walk-3.log`, exit 0, load witness `1.44 2.18 2.71 1/1565 3096703`, at `af54d3f0`. That witness adds `other ctest or compiler processes: 0`. The block prints the plan line, the benchmark line, the twenty ranked rows, the four `hunt: skipped` lines and the summary line in the order the log prints them, and leaves out the 477 `hunt: rank -` rows that stand between the last ranked row and the skipped lines.

```text
hunt: plan events=497 passes=249 method=PASSES estimate=948441ns budget=120s stage1=497 stage2=60
bmMemoryWalk             iterations=  15416351  real time/iter (ns)=   44.742  cpu time/iter (ns)=   44.638  overhead floor (ns)=  675.000  endpoint samples=2  items_per_second=22402653.741  bytes_per_second=89610614.966
hunt: rank 1 cpu/topdown-fetch-bubbles instr=2166.831 cv=9.30 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 2 cpu/topdown-slots-issued instr=1447.187 cv=2.10 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 3 cpu/topdown-total-slots instr=1374.136 cv=4.41 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 4 cpu/topdown-slots-retired instr=1136.229 cv=0.50 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 5 cpu/branch-instructions instr=227.048 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 6 cpu/L1D_PEND_MISS.PENDING_CYCLES_ANY instr=120.434 cv=3.06 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 7 cpu/mem-stores instr=82.922 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 8 cpu/branch-misses instr=17.769 cv=8.33 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 9 cpu/CORE_SNOOP_RESPONSE.RSP_IFWDFE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 10 cpu/CORE_SNOOP_RESPONSE.RSP_IFWDM instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 11 cpu/CORE_SNOOP_RESPONSE.RSP_IHITFSE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 12 cpu/CORE_SNOOP_RESPONSE.RSP_IHITI instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 13 cpu/CORE_SNOOP_RESPONSE.RSP_SFWDFE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 14 cpu/CORE_SNOOP_RESPONSE.RSP_SFWDM instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 15 cpu/CORE_SNOOP_RESPONSE.RSP_SHITFSE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 16 cpu/IDI_MISC.WB_DOWNGRADE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 17 cpu/L1D_PEND_MISS.FB_FULL instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 18 cpu/L2_LINES_OUT.SILENT instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 19 cpu/L2_LINES_OUT.USELESS_HWPF instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 20 cpu/L2_LINES_OUT.USELESS_PREF instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: skipped cpu/INST_RETIRED.TOTAL_CYCLES_PS gap
hunt: skipped cpu/INT_MISC.CLEARS_COUNT gap
hunt: skipped cpu/INT_MISC.CLEAR_RESTEER_CYCLES gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

**The three benchmark consoles.** Dated rows beside `The ranked rows`, whose text
stands unchanged. The command
`./build/example/first_benchmark --hunt --hunt-budget 120s` of `The commands`
ran three times in `build` at `af54d3f0`, each run alone, each at exit 0. The
same three counts stand here: 497 events in the plan line, twenty numbered rank
rows, and 477 rows that close with a `mark=` cell.

Console `f6-bench-1.log`, exit 0, load witness `1.41 2.16 2.70 1/1565 3096767`, at `af54d3f0`. That witness adds `other ctest or compiler processes: 0`. The block prints the plan line, the benchmark line, the twenty ranked rows, the four `hunt: skipped` lines and the summary line in the order the log prints them, and leaves out the 477 `hunt: rank -` rows that stand between the last ranked row and the skipped lines.

```text
hunt: plan events=497 passes=249 method=PASSES estimate=1579407ns budget=120s stage1=497 stage2=60
bmSum                    iterations=   1052553  real time/iter (ns)=  665.635  cpu time/iter (ns)=  664.780  overhead floor (ns)=  676.000  endpoint samples=2
hunt: rank 1 cpu/topdown-slots-issued instr=859.897 cv=0.40 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 2 cpu/topdown-slots-retired instr=830.402 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 3 cpu/topdown-total-slots instr=399.964 cv=0.77 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 4 cpu/L1D_PEND_MISS.PENDING instr=371.357 cv=2.23 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_split_loads
hunt: rank 5 cpu/ref-cycles instr=298.330 cv=4.88 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 6 cpu/L1D_PEND_MISS.PENDING_CYCLES_ANY instr=259.773 cv=3.06 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 7 cpu/L1D_PEND_MISS.PENDING_CYCLES instr=256.397 cv=2.34 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 8 cpu/topdown-fetch-bubbles instr=209.932 cv=3.38 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 9 cpu/branch-instructions instr=202.292 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 10 cpu/L2_RQSTS.REFERENCES instr=82.711 cv=1.77 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 11 cpu/L2_RQSTS.ALL_PF instr=59.968 cv=1.67 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 12 cpu/L2_RQSTS.PF_HIT instr=51.439 cv=1.89 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 13 cpu/L1D.REPLACEMENT instr=42.910 cv=0.55 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 14 cpu/L2_RQSTS.ALL_DEMAND_REFERENCES instr=29.051 cv=1.34 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 15 cpu/L2_RQSTS.ALL_DEMAND_DATA_RD instr=25.853 cv=3.16 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 16 cpu/mem-stores instr=7.552 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 17 cpu/bus-cycles instr=2.132 cv=2.44 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 18 cpu/branch-misses instr=1.599 cv=3.15 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 19 cpu/L2_RQSTS.ALL_RFO instr=0.711 cv=6.93 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_lock_latency
hunt: rank 20 cpu/L2_RQSTS.RFO_HIT instr=0.711 cv=0.00 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_lock_latency
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: skipped cpu/INST_RETIRED.TOTAL_CYCLES_PS gap
hunt: skipped cpu/INT_MISC.CLEARS_COUNT gap
hunt: skipped cpu/INT_MISC.CLEAR_RESTEER_CYCLES gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

Console `f6-bench-2.log`, exit 0, load witness `1.45 2.16 2.70 2/1565 3096827`, at `af54d3f0`. That witness adds `other ctest or compiler processes: 0`. The block prints the plan line, the benchmark line, the twenty ranked rows, the four `hunt: skipped` lines and the summary line in the order the log prints them, and leaves out the 477 `hunt: rank -` rows that stand between the last ranked row and the skipped lines.

```text
hunt: plan events=497 passes=249 method=PASSES estimate=1567704ns budget=120s stage1=497 stage2=60
bmSum                    iterations=   1054233  real time/iter (ns)=  663.630  cpu time/iter (ns)=  662.769  overhead floor (ns)=  680.000  endpoint samples=2
hunt: rank 1 cpu/topdown-slots-issued instr=863.006 cv=0.27 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 2 cpu/topdown-slots-retired instr=830.402 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 3 cpu/topdown-total-slots instr=403.785 cv=2.95 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 4 cpu/L1D_PEND_MISS.PENDING instr=386.905 cv=0.34 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_split_loads
hunt: rank 5 cpu/ref-cycles instr=324.271 cv=2.34 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 6 cpu/L1D_PEND_MISS.PENDING_CYCLES_ANY instr=271.233 cv=2.12 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 7 cpu/L1D_PEND_MISS.PENDING_CYCLES instr=258.351 cv=0.26 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 8 cpu/branch-instructions instr=202.292 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 9 cpu/L2_RQSTS.REFERENCES instr=83.955 cv=3.77 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 10 cpu/L2_RQSTS.ALL_PF instr=57.569 cv=7.25 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 11 cpu/L2_RQSTS.PF_HIT instr=52.772 cv=7.59 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 12 cpu/L1D.REPLACEMENT instr=43.888 cv=0.71 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 13 cpu/L2_RQSTS.ALL_DEMAND_DATA_RD instr=27.008 cv=2.26 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 14 cpu/L2_RQSTS.DEMAND_DATA_RD_HIT instr=25.942 cv=1.42 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 15 cpu/mem-stores instr=7.552 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 16 cpu/bus-cycles instr=2.132 cv=2.37 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 17 cpu/branch-misses instr=1.599 cv=6.19 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 18 cpu/L2_RQSTS.ALL_RFO instr=0.800 cv=6.19 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_lock_latency
hunt: rank 19 cpu/L2_RQSTS.RFO_HIT instr=0.711 cv=6.93 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_lock_latency
hunt: rank 20 cpu/L2_RQSTS.PF_MISS instr=0.355 cv=0.00 correlation=n/a slope=n/a delta=n/a area=cache
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: skipped cpu/INST_RETIRED.TOTAL_CYCLES_PS gap
hunt: skipped cpu/INT_MISC.CLEARS_COUNT gap
hunt: skipped cpu/INT_MISC.CLEAR_RESTEER_CYCLES gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

Console `f6-bench-3.log`, exit 0, load witness `1.45 2.16 2.70 1/1565 3096883`, at `af54d3f0`. That witness adds `other ctest or compiler processes: 0`. The block prints the plan line, the benchmark line, the twenty ranked rows, the four `hunt: skipped` lines and the summary line in the order the log prints them, and leaves out the 477 `hunt: rank -` rows that stand between the last ranked row and the skipped lines.

```text
hunt: plan events=497 passes=249 method=PASSES estimate=1592355ns budget=120s stage1=497 stage2=60
bmSum                    iterations=   1052982  real time/iter (ns)=  665.581  cpu time/iter (ns)=  664.700  overhead floor (ns)=  674.000  endpoint samples=2
hunt: rank 1 cpu/topdown-slots-issued instr=862.296 cv=0.26 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 2 cpu/topdown-slots-retired instr=830.402 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 3 cpu/L1D_PEND_MISS.PENDING instr=430.348 cv=0.89 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_memory_bound > topdown.intel.tma_l1_bound > topdown.intel.tma_split_loads
hunt: rank 4 cpu/topdown-total-slots instr=405.561 cv=1.68 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 5 cpu/ref-cycles instr=324.271 cv=2.34 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 6 cpu/L1D_PEND_MISS.PENDING_CYCLES instr=277.186 cv=1.65 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 7 cpu/L1D_PEND_MISS.PENDING_CYCLES_ANY instr=274.876 cv=2.65 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 8 cpu/topdown-fetch-bubbles instr=216.596 cv=9.99 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 9 cpu/branch-instructions instr=202.292 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 10 cpu/L2_RQSTS.REFERENCES instr=78.269 cv=0.49 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 11 cpu/L2_RQSTS.ALL_PF instr=48.952 cv=5.34 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 12 cpu/L1D.REPLACEMENT instr=47.441 cv=0.76 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 13 cpu/L2_RQSTS.PF_HIT instr=46.198 cv=1.42 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 14 cpu/L2_RQSTS.DEMAND_DATA_RD_HIT instr=29.673 cv=3.45 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 15 cpu/L2_RQSTS.ALL_DEMAND_DATA_RD instr=29.407 cv=2.57 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 16 cpu/L2_RQSTS.ALL_DEMAND_REFERENCES instr=28.962 cv=4.05 correlation=n/a slope=n/a delta=n/a area=cache
hunt: rank 17 cpu/mem-stores instr=7.552 cv=0.00 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 18 cpu/bus-cycles instr=2.221 cv=2.34 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 19 cpu/branch-misses instr=1.599 cv=6.19 correlation=n/a slope=n/a delta=n/a area=unmapped
hunt: rank 20 cpu/L2_RQSTS.ALL_CODE_RD instr=1.510 cv=8.81 correlation=n/a slope=n/a delta=n/a area=cache
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: skipped cpu/INST_RETIRED.TOTAL_CYCLES_PS gap
hunt: skipped cpu/INT_MISC.CLEARS_COUNT gap
hunt: skipped cpu/INT_MISC.CLEAR_RESTEER_CYCLES gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

**The list console.** A dated row beside `What the base answered`. The command
`./build/example/memory_walk --list` ran at `af54d3f0`, answered exit 0 with the
load witness `1.50 2.11 2.67 1/1560 3097135`, and printed one line:
`bmMemoryWalk`.

**The plan console.** A dated row beside `The commands`, whose commands stand
unchanged. The command `./build/example/first_benchmark --hunt --dry-run` ran at
`af54d3f0` with the load witness `1.50 2.11 2.67 2/1560 3097173` at exit 0. Its
plan line answers `estimate=not available` and `budget=none`, it prints no
numbered rank row, and it prints 497 rows that close with a `mark=` cell. Its
first rank row, the divider row, reads `mark=unrepeated` with a three-node
`area` path:

```text
hunt: plan events=497 passes=249 method=PASSES estimate=not available budget=none
bmSum                    iterations=         1  real time/iter (ns)= 1533.000  cpu time/iter (ns)= 1530.000  overhead floor (ns)=  682.000  endpoint samples=2
hunt: rank - cpu/ARITH.DIVIDER_ACTIVE instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a area=topdown.backend_bound > topdown.intel.tma_core_bound > topdown.intel.tma_divider mark=unrepeated
hunt: plan summary events=493/497 passes=249 budget=none
```

**The scenario console.** A dated row beside the `Hunt, one scenario and one plan
line` row of `Lane Table` of `specs/048-hwlat-probe/plan.md`. The command
`./build/test/harness_hunt_cli_test bare-hunt-whole-catalog` ran at `af54d3f0`
with the load witness `1.62 2.12 2.67 1/1559 3097228` at exit 0 and printed
one line: `bare-hunt-whole-catalog: ok`.

**The two level lines.** A dated row beside `The level-1 block`, whose fenced
block stands unchanged. Each of the six hunt consoles of this pass prints
`topdown: level=1 passes=4 table=arch/x86/skylakex/` and
`topdown: reference=linux-7.2.4` ahead of the table file list, and that list
carries one `topdown: file` line per table file the record carries, the reading
ruling 41 of `Rulings - this phase (29 to 42)` states. Pass 1 wrote those digest
lines into each log under `/tmp/ic/p1/`, and this section copies none of them,
because that ruling bars a row from pinning their count. `IA46-004` of
`specs/046-hunter-topdown/tasks.md` carries the digest list, and the Pass 2 run
of that row prints it.

**The figures this section leaves untaken at `af54d3f0`.** Pass 1 ran no hunt
console in an instrumented tree, so no figure here carries a hunt run under
`build/sanitize` or `build/tsan`. Pass 1 printed no rank movement reading across
the three consoles of one command, and it printed no node-mapped, list-group or
unmapped cell reading over the printed rows beside the `level=1` companion
block, so the two `Hunt` rows of `Lane Table` of `specs/048-hwlat-probe/plan.md`
carry those readings at the head Pass 2 runs at.
