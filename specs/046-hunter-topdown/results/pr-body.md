# The area of a ranked event

The branch reads the top-down area of each event the hunter ranks. `--hunt` prints one
`area=` cell beside each rank row, and one level-1 block per benchmark after the table.
The written record carries a `topdownArea` key beside each hunt record at schema `1.14`,
and the record of the level-1 companion ask stands beside it. A host that holds no
top-down table answers `unmapped` for every rank and prints no block. The group of a
rank reads from the vendor mapping file the host carries, so one listing answers
`front-end-bound` where another answers `l2-cache`.

Roadmap item N14c. Spec `specs/046-hunter-topdown/spec.md`.

## The lane

- One commit over `master`.
- 47 files changed, 9,060 added lines, 163 removed lines.
- Source: 893 added lines in 10 files. `source/harness/hunt_topdown.cpp` stands new
  beside the hunter, with its private header `detail/hunt_topdown.hpp`.
- Tests: 3,429 added lines in 16 files, with two new executables.
  `counters_topdown_groups_test` reads the vendor grouping, and
  `harness_hunt_topdown_test` reads the area mapper.
- One new example: `example/docs/hunt_topdown.cpp`, with its recorded console bytes in
  `hunt_topdown.expected`, quoted in the hunter section of the harness page.
- Version `0.31.0`, schema `1.14`, harness `SOVERSION 21`, counters archive
  `SOVERSION 6`.
- The DBC documentation gate and the pairing gate read 347 interfaces and report no
  gap, where the base read 345 (SC-006).

## The base this branch sits on

`master` moved 26 commits during the lane. It took specs/047 in, which carries the
project to `0.30.1`, and it took the meta pass that clears the gate rows PRs 71 to 73
left red. This branch stands on `3639c4a5`. The version rows here read `0.31.0`, harness
`SOVERSION 21` and counters `SOVERSION 6`, because specs/047 took the patch and left the
minor free. Two rebase resolutions adopted the corrected wording of the two ABI notes
that pass rewrote. The lane measured against `b1dd7670` as its comparison base, and each
reading below names that base where it matters.

Format, spell, drift, the prose gate in both forms, and both contract gates ran again on
the new base and read clean. The units that read the version rows and the shared loader
pass there. The coverage, sanitizer, aggregate-gate, analyzer and documentation rows
were measured before those moves, at the head of the lane before the rebase. The gate
list below carries the reading taken at the landed content `858c55d9`.

## The host

- Host `CorsairOne`, kernel `Linux 7.3.0-4-cachyos x86_64`, 24 cpus.
- Intel Core i9-9920X, model 85, the part the `arch/x86/skylakex/` table names.
- `kernel.perf_event_paranoid` at -1, set by the owner for this work.
- gcc 16.2.1, clang 23.1.3, cmake 4.4.2.
- Version line of a build at the head: `speedgun-ng 0.31.0 build=Release`.

## The acceptance record

`specs/046-hunter-topdown/results/acceptance-hunt.md` carries the commands, the host
identity, the ranked-row groups and the level-1 block verbatim. Three head runs and one
base run of `example/memory_walk` and `example/first_benchmark` each ranked 497 events
over 249 passes. The three counts of the ranked rows stand at 58 node-mapped, 412
list-group and 27 unmapped in every head run, which are the AP-9 figures exactly
(SC-007).

The base run printed 505 lines and no block. Each head run prints 524 lines, of which
the block and its file list account for 19. The console bytes of one binary path read
62,142 at the base and 76,105 at the head, and the plan line of both reads
`events=497 passes=249`. The estimate moves from 916,569 ns to 908,103 ns (FR-015).

AP-10 stands, so no before-and-after byte comparison of two live runs proves rank
equality. Two head runs of one binary differ at 75 of 497 positions, and head against
base differs at 93. FR-010 rests on the committed byte fixtures instead. AP-12 stands
beside it: the first `topdownTree` ask of a process reads the table in 503 ms, which no
base hunter run paid.

## What the gates read here

- `cmake -P cmake/lint.cmake` and `cmake -P cmake/spell.cmake` answer exit 0.
- `python3 tools/docs/example_drift.py --sections --names --options --forbidden .`
  answers exit 0.
- The prose gate reads 0 findings in all three forms: the range over `b1dd7670`, the
  message of each commit, and the whole tree at 1,011 sources (AP-2b).
- `ctest -N` lists 181 entries at the head and 173 at the base. AP-1's 178 matches
  neither count.
- The full head suite answers 178 of 181 in 2,126 s. Its failures are the AP-1 trio
  `counters_kernel_calls_test`, `counters_pmu_provider_test` and
  `harness_jitter_window_test`, and no other name (SC-005).
- The release tree at the landed content `858c55d9` answers the full suite at 181 of 181
  in 2,127.91 s. `counters_kernel_calls_test`, `counters_pmu_provider_test` and
  `harness_jitter_window_test` pass at 0.03 s, 0.08 s and 8.11 s, so the AP-1 red row
  stands cleared at this base. Rows above that name the trio record the host before the
  meta pass.
- The two-cpu lane `taskset -c 0,1 ctest --preset prepush-quick -j 2` fails 4 of 174 at
  the head: the trio and `prose_gate_fixtures`. Its `timing.whole-tree` unit read
  `wall:10.671s` against a 10.0 s bound and passes when run alone. The base lane under
  the same taskset fails 4 of 171: the trio and `example_run`, which answers
  `docs_topdown registered bmTopdown but printed no row`. Neither fourth name reaches
  code this branch writes, and specs/042 records the same bound on this host.
- The analyzer reads a fresh `ci-ubuntu` tree at the head against a clean worktree of
  `b1dd7670`. The head lists 21,189 findings where the base lists 20,862, and naming
  findings read 0 at both ends. No check family appears at the head that the base does
  not answer. Four files stand new to the branch. Existing files carry 137 further
  findings over 44 pairs of file and check. Nine of them stand in
  `source/counters/detail/pmu.hpp`, which the branch edits nowhere: a new include route
  brings them into view.
- The sanitizer lane of `cmake --preset=ci-sanitize` configures and builds at exit 0.
  Its `ctest --test-dir build/sanitize` answers 11 failures of 176 in 3,882 s: the
  AP-1 trio, `prose_gate_fixtures`, and six units past their ceilings, `harness_cli_test`
  and `harness_cli_sources_test` at 300 s, `base_cli_matrix` at 120 s,
  `harness_jitter_cli_test`, `harness_hunt_cli_test` and `example_run`. The eleventh was
  `harness_hunt_topdown_test`, which this branch writes. Its mapping claim counts the
  pages a process holds, and the allocator of the sanitizer maps pages as it needs them,
  so the count moved on calls that read nothing. The claim now stands outside sanitizer
  builds, and the rerun below names what survives.
- The thread sanitizer lane of `cmake --preset=ci-tsan` configures and builds at exit 0.
  Its `ctest --test-dir build/tsan` answers 10 failures of 177: the AP-1 trio,
  `prose_gate_fixtures`, `harness_jitter_cli_test`, `counters_topdown_groups_test`,
  `report_shape_fwq_ftq_sample`, and the three timeouts `base_cli_matrix`,
  `harness_hunt_cli_test` and `example_run`. `counters_topdown_groups_test` carries the
  same mapping claim and stands fixed the same way. `report_shape_fwq_ftq_sample` runs
  `example/docs_fwq_ftq`, and specs/042 records the same probe race under this lane on
  this host.
- `tools/ci/prepush.sh --self-check` answers exit 0, with all four fault paths reading
  pass. `tools/ci/prepush.sh --full` answers exit 8. Every gate step of it passes: drift
  at 0 s, format at 36 s, time-source at 2 s, spell at 5 s, prose at 2 s over 70 sources
  with 0 findings, configure at 2 s and build at 16 s. The quick tier at full affinity
  then fails 4 of 174: the AP-1 trio and `counters_overhead`, whose line reads
  `published floor median 108.0 ns against 72.0 ns measured` for one `rdpmc` read taken
  while 24 entries ran together.
- `ctest --test-dir build/sanitize --rerun-failed` answers 10 failures of 11 after the
  mapping claim moved, with the mapper unit gone from the list.
  `counters_topdown_groups_test` answers exit 0 in the thread sanitizer tree.
- The `docs` target does not complete on this host, at the base and at the head alike. It
  needs `speedgun-ng_DEVELOPER_MODE` and `BUILD_MCSS_DOCS` on, and a python with the m.css
  module set. It then stops in `mcss/documentation/doxygen.py` with `'NoneType' object has
  no attribute 'attrib'`, after Doxygen has written its XML. The page warnings read 157 at
  the base and 158 at the head. The added line is one `hunter-mode` link, the shape
  `docs/pages/harness.md` carries 21 times already. The `docs` job of the pull request
  stands as the row of record for documentation.

## What this branch carries for the base

`lcov` stood broken on this host before the lane ran. It died with `Perl API version
v5.42.0 of DateTime.c does not match v5.44.0`, so the coverage gate answered exit 3 and
named no ratio. The repair stands outside this repository. Four user-local perl modules
rebuilt for perl 5.44 replace the stale ones under `~/.local/lib/perl5`, and `lcov`,
`genhtml`, `geninfo` and `llvm2lcov` name that tree in their own `use lib` line. The
tool now answers `lcov --version` with `LCOV version 2.5.0-beta`. No repository file
moved, and no
system package moved. The coverage rows below rest on that repair.

## What this host cannot close

- SC-009 and SC-010 stand pending. No AMD or Arm host stands in reach here, so the
  vendor stems of the second maker rest on planted tables and on the Intel acceptance
  hunt above.
- The coverage ratio of record is the `coverage` job of the pull request. The host
  readings below come from one instrumented tree on one machine.
- Four units pass their ceilings under instrumentation here at the base and at the
  head, so their counters reach no local trace: `base_cli_matrix`,
  `harness_jitter_cli_test`, `harness_hunt_cli_test` and `example_run`.
- `prose_gate_fixtures` bounds its own whole-tree run at 10 seconds and reads 10.671 s
  under the two-cpu lane here. specs/042 records 16.74 s for the same unit alone.

## Acceptance closeout correction (2026-11-08)

SC-IC-5 of `Success Criteria - this extension` of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) owns the shape of this row: the
paragraph it names keeps its text and its heading, and this row names the line it
replaces. `## The acceptance record` reads at `:56-60` (read at `a55728af`), and the quoted
text stands verbatim at `:57-59` inside the sentence that opens at `:56`:

```text
base run of `example/memory_walk` and `example/first_benchmark` each ranked 497 events
over 249 passes. The three counts of the ranked rows stand at 58 node-mapped, 412
list-group and 27 unmapped in every head run, which are the AP-9 figures exactly
```

This section was written against `af54d3f0`, the head this branch carries. A
cite into a spec, plan or tasks file reads at `af54d3f0`, and a source or page
anchor reads at the merged base `a55728af`, where it stands unchanged.

Owner: SC-007 of `Measurable Outcomes` of `specs/046-hunter-topdown/spec.md` (read at `a55728af`). What answers differently: a
run gives a rank to the first `min(top, ranked)` addresses, and the default `top` stands
at 20 (`include/speedgun-ng/hunt.hpp:135` (read at `a55728af`), `source/harness/hunt_plan.cpp:371-374` (read at `a55728af`)). The
run prints every address that carries no rank as a `hunt: rank -` row
(`source/harness/report.cpp:1478` (read at `a55728af`)), and the `area=` cell stands on both row kinds
(`source/harness/report.cpp:1471` (read at `a55728af`) and `source/harness/report.cpp:1488` (read at `a55728af`), under FR-013 and
FR-014 of `The console (C)` of `specs/046-hunter-topdown/spec.md` (read at `a55728af`)). The three counts therefore read
over the printed rows, and SC-007 words them that way. The figures inside the quoted text
are the standing text of the lines they come from, and the task row IA46-004
of `specs/046-hunter-topdown/tasks.md` (read at `af54d3f0`) records the two commands at the closeout
head. Replacement, for the two sentences at `:56-60`: "Three head runs and one base run of
`example/memory_walk` and `example/first_benchmark` each printed one `area=` cell per
printed row, the ranked rows and the `hunt: rank -` rows alike, and 20 addresses carried a
rank under the limit at `include/speedgun-ng/hunt.hpp:135` (read at `a55728af`). The three counts of the
printed rows stand beside the `events=` summary line in
`specs/046-hunter-topdown/results/acceptance-hunt.md` (read at `af54d3f0`), under
its heading `Acceptance closeout corrections (2026-11-08)` (SC-007)."

## Corrections at the merged base (2026-11-09)

This block was written against `af54d3f0`, the head this branch carries, whose
base is the merged head `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the base
head that the `Base head` row of `The base this work reads` of
`specs/046-hunter-topdown/spec.md` (read at `af54d3f0`) names.
The record line and the source lines below were opened at `a55728af`, and they
stand unchanged at `af54d3f0`, because this branch's delta is markdown-only. A
cite into `specs/048-hwlat-probe/spec.md` names text this branch wrote and reads
at `af54d3f0`, because that spec holds no `SC-IC-` id at `a55728af`. A bare `:n` in this block names the artifact the sentence names last,
and this file when it names none; a line of this file at or below 160 stands
at `a55728af` too, and a line above reads at `af54d3f0`.

SC-IC-5 of `Success Criteria - this extension` of
`specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) owns the shape of this
section. The section above keeps its text and its heading, and
the row below names the lines it replaces and gives the reading that holds,
beside the head its target was read at. No row here carries a blocked reading.

- **Row 1. The area-cell claim at `:189-192`.** Owner: SC-007 of
  `Measurable Outcomes` of `specs/046-hunter-topdown/spec.md` (read at
  `a55728af`), the criterion that row names.
  Quoted claim, verbatim from `:189-191`:

  ```text
  Three head runs and one base run of
  `example/memory_walk` and `example/first_benchmark` each printed one `area=` cell per
  printed row, the ranked rows and the `hunt: rank -` rows alike,
  ```

  Which record the run supports:
  `specs/046-hunter-topdown/results/acceptance-hunt.md:65-68` (read at `a55728af`) records that the
  base run at `b1dd7670` printed 505 lines and no top-down block, and it names
  the area cell as the second new text over that base. A base run with no
  top-down block and no new area text prints no area cell, so that record holds
  and the claim at `:189-192` fails for the base run. The three head runs answer
  the claim.

  When the cell prints: `source/harness/report.cpp:1316-1318` (read at `a55728af`) prints one ` area=`
  cell where `RankedEvent::topdownArea` holds text, and the call prints nothing
  where that member stands empty. The cell therefore stands on the rows that
  carry it, on the ranked rows and on the `hunt: rank -` rows alike
  (`source/harness/report.cpp:1471` (read at `a55728af`) and `source/harness/report.cpp:1488` (read at `a55728af`)).

  Replacement clause, for the clause that stands at `:189-192`: "Three head runs
  of `example/memory_walk` and `example/first_benchmark` each printed one `area=`
  cell on every printed row that carried an area, the ranked rows and the
  `hunt: rank -` rows alike, and 20 addresses carried a rank under the limit at
  `include/speedgun-ng/hunt.hpp:135` (read at `a55728af`). The base run printed 505 lines, no top-down
  block, and no `area=` cell
  (`specs/046-hunter-topdown/results/acceptance-hunt.md:65-68` (read at `a55728af`))."

- **Row 2. The SC-IC-5 cite at `:164`.** That line opened the section above with
  a range into `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), and the lines
  it opened close SC-IC-4, the criterion that speaks about the `dbc-gate` target.
  The line now names SC-IC-5 of `Success Criteria - this extension` of
  `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), the criterion this section
  answers, so no line number stands for it in either section.
