# Contract: the `area=` console cell and the level-1 block of a hunt run

**Feature**: specs/046-hunter-topdown | **Files**: `source/harness/report.cpp`
with `source/harness/cli.cpp` and `source/harness/detail/internal.hpp` |
**Audit point**: `b1dd7670`
**Spec**: FR-013 to FR-017, FR-027, FR-033 to FR-036, D-9, D-11, D-13

## What this contract covers

One console cell and one companion block join the hunt block, and both are
console-only. This contract fixes the form of the cell and its place in both
ranked row kinds and the unranked row. It fixes the rule for a member that stands
empty, the place of the block, and the channel that carries it. It lists the
console bytes that must not move, with the machine that proves each one.

## The bytes today, recorded

`example/docs/hunt_rank.expected` holds the ranked rows of the scripted example,
and the guide page quotes that region at `docs/pages/harness.md:2269`:

```text
hunt: rank 1 cpu/hunt_branches instr=2000.000 cv=0.00 correlation=n/a slope=n/a delta=n/a
hunt: rank - cpu/hunt_stores instr=0.000 cv=0.00 correlation=n/a slope=n/a delta=n/a mark=beyond-top
```

`printHuntRanking` fills both shapes
(`source/harness/report.cpp:1336-1366`).

## The cell

One helper in the anonymous namespace prints the cell. It stands beside
`printSensitivityCells` (`source/harness/report.cpp:1183-1200`) and carries:

```cpp
/// @brief specs/046 FR-013, FR-014, D-13: the one `area=` cell of a ranked
/// row or of an unranked row.
///
/// \pre none
/// \post The call prints one ` area=` cell holding the area of the record, or
///       prints nothing at all where that member stands empty. The cell
///       carries one leading space and no field width. The call measures
///       nothing, reads no clock and opens no file.
/// \invariant none
```

The format string reads `" area=%s"` with the member as a `std::string_view`,
which matches the cells before it (`source/harness/report.cpp:1186`,
`:1196-1198`).

| Row kind | Place of the cell | Anchor |
|---|---|---|
| ranked row | after the ` delta=` cell, before the `flag=` cell | `source/harness/report.cpp:1349-1352` |
| unranked row | after the ` delta=` cell, before the `mark=` cell | `source/harness/report.cpp:1363-1364` |

FR-013 states the same place: the cell prints where a row closes.
The printer of the sensitivity cells keeps no cell of its own. Its post names
one ` correlation=` cell, one ` slope=` cell and one ` delta=` cell
(`source/harness/report.cpp:1179-1182`). An implementer who moves the cell into
that function moves that post with it (FR-013).

The cell holds what the mapper answered. Its content is a node path joined with
` > `, a vendor file stem, or `unmapped` (`contracts/area-mapper.md`). This file
composes no separator of its own.

## The rule for a member that stands empty

A record whose area member holds nothing prints no cell and no separator, the
rule FR-014 states. `printResult` carries the precedent in its own comment. An empty entry prints
no block, so a run with no `--graph` prints the bytes it printed before the
record existed (`source/harness/detail/internal.hpp:1336-1341`).

FR-014 also separates this rule from the specs/040 cells. Those answer `n/a` for
a field a run measured and could not fill, while an empty area names a record no
hunt stamped. FR-009 gives every record of the run an area, `unmapped` included,
so a run leaves no ranked row without a cell. The empty arm therefore belongs to
a record no run wrote. `example/docs/hunt_rank.cpp:108-116` builds such records
through `sg::hunt::rankEvents`, and `printResult` prints them. Its recorded block
stands at `example/docs/hunt_rank.expected:6-10`.

## Where the block stands

FR-016 puts the block after the hunt block of that benchmark closes, that is
after `printHuntSkips` returns
(`source/harness/report.cpp:813-815`). That one function prints the skipped rows
at `source/harness/report.cpp:1380-1385` and the summary line at `:1396-1402`.
Its shape stands, because two callers declare it by hand
(`test/source/harness_hunt_report_test.cpp:92`,
`example/docs/hunt_rank.cpp:28`), so the block rides after that function and not
inside it (D-9).

| Fact | Ruling |
|---|---|
| one block per benchmark | the standing call prints one block per benchmark today (`source/harness/report.cpp:765`), and a hunter run reads the same count |
| the standing call keeps its place for every other run | it sits under the tables and above the jitter line (`source/harness/report.cpp:762-765`) |
| a hunter run reads no second block | the standing call stands suppressed for that run, so FR-017 of specs/042-topdown stands unchanged |
| the level is the one `--topdown` names where that option stands | FR-034; the implicit level-1 pass applies where no `--topdown` stands |
| a hunter dry run prints the block too | a dry run runs one iteration per pass (`source/harness/hunt.cpp:1050-1053`), so its block carries one-iteration figures (round 5) |

`printTopdownBlock` owns every rule of the block: the header line, the four
level-1 rows, the metric lines, the footnote and the balance lines
(`source/harness/report.cpp:359-442`). It returns where a record stands absent or
unsupported (`source/harness/report.cpp:361-363`). This contract reuses that body
and states no rule of it.

## The channel that carries it

FR-036 fixes the path. The answer reaches the printer with
`BenchmarkResult::topdown` left unset, because that field writes the document
(`source/harness/json_report.cpp:867-869`) and feeds the pass disclosure
(`source/harness/report.cpp:305`). No object joins the document, which keeps the
areas of its own rows (D-11). `printResult` already takes two optional internal
carriers beside the result, so the level-1 record rides as the third one there
(`source/harness/detail/internal.hpp:1342-1345`).

The companion pass runs with the instance handoff clear, so neither list prints.
Both lists fill on that handoff (`source/harness/detail/internal.hpp:326-330`,
`source/harness/runner.cpp:3076`, `source/harness/runner.cpp:3112`) (FR-036).

## The two lines behind the option the user named

FR-035 names two lines that stay behind the option a user named. One is the
`topdown: core-wide coverage=` line (`source/harness/cli.cpp:1584-1593`), and the
other is any pass disclosure of the companion's own collection. An internal bit beside
`topdownLevel` tells the two asks apart
(`source/harness/cli.cpp:303`). The collection rides its own internal option
record, never the process record that `source/harness/cli.cpp:756` fills and
`:1505` hands to the entry runner (D-9).

| Line | Guard after this change | Anchor |
|---|---|---|
| the refusal line | the one call of the helper, its guard widened to the internal bit | `source/harness/cli.cpp:1583`, `:301-316` |
| the coverage line | `topdownLevel` alone, so a hunter run prints neither line | `source/harness/cli.cpp:1584-1593` |

The one-per-process rule of FR-017 is that single call, and no stored state
behind it (FR-035).

## The console bytes that must not move

| Bytes | Anchor | Machine |
|---|---|---|
| the plan line | `source/harness/report.cpp:1272-1309` | `plan-line-shape` (`test/source/harness_hunt_report_test.cpp:503`) |
| the cut line | `source/harness/report.cpp:1315-1318` | `plan-line-budget-cut` (`test/source/harness_hunt_report_test.cpp:1271`) |
| the sample line and the groups line | `source/harness/report.cpp:1132-1169`, `:1323-1324` | `plan-line-mux-groups` (`test/source/harness_hunt_report_test.cpp:1279`) |
| the skipped rows | `source/harness/report.cpp:1380-1385` | `skipped-list-line` (`test/source/harness_hunt_report_test.cpp:1162`) |
| the summary line | `source/harness/report.cpp:1396-1402` | `hunt-block-order` (`test/source/harness_hunt_report_test.cpp:1000`) |
| the order of the hunt lines | `source/harness/report.cpp:812-816` | `hunt-block-order` (`test/source/harness_hunt_report_test.cpp:1000`) |
| the pass disclosure, the `pass=` cells and the row figures | the entry record collects no further pass (FR-015, FR-035) | `entry-run-figures-stand-equal-between-base-and-head`, a new scenario of `test/source/harness_hunt_report_test.cpp` |
| every byte of a run with no hunt | the standing call at `source/harness/report.cpp:765` keeps its place | `no-hunt-bytes` (`test/source/harness_hunt_json_test.cpp:2065`) over the committed capture `hunt_no_hunt_console.txt` (`:396`) |
| the recorded block of the scripted example | `example/docs/hunt_rank.expected:6-10` | `report_shape_hunt_rank_sample` (`test/CMakeLists.txt:2081-2088`) |
| the exit status and the one refusal line of a refused host | the standing helper (`source/harness/cli.cpp:1583`) | FR-017, in `test/source/harness_hunt_cli_test.cpp` |

The no-hunt case compares the run's capture against the committed fixture named at
`test/source/harness_hunt_json_test.cpp:391-396`.

## The recorded blocks, and why no fold rule lands

FR-027 answers the fold question for this feature: a recorded region holds no
real level-1 block, so `tools/docs/report_shape.py` gains no fold rule. The
example of FR-025 writes its own records, and `printHuntRanking` stands at
namespace scope (`source/harness/report.cpp:573`) for a caller to reach. A real
block carries the table path, the reference line, the digests and the node names
of the host. FR-027 states the recipe for a later change that records one.

Two standing facts carry the guard today. The identity mask of
`tools/docs/report_shape.py` folds runs of digits and runs of whitespace
(`mask()` at `:140-165`). `example/docs/hunt_rank.expected` holds no area value of a live
host, because its records are hand-built and carry no area at all
(`example/docs/hunt_rank.cpp:108-116`).

## Verification

| Fact | Machine |
|---|---|
| the cell prints after `delta=` on a ranked row and before `mark=` on an unranked row | `area-cell-after-delta-and-before-the-flag` (`test/source/harness_hunt_report_test.cpp`) |
| an empty member prints no cell and no separator, and the rest of the line keeps its bytes | `no-area-cell-for-an-empty-member` (FR-014) |
| the block stands after the hunt block closes, beside the summary line | `hunt-block-order` (`test/source/harness_hunt_report_test.cpp:1283`), widened to a hunter run (FR-016) |
| a hunter document holds no topdown object | the unit FR-036 names, over a written hunter document |
| a hunter run prints no coverage line and no second pass disclosure | two new scenarios of `test/source/harness_hunt_cli_test.cpp`, which owns the console capture of the run (FR-035, FR-034) |
| a refused host prints one refusal line and keeps its exit status | `test/source/harness_hunt_cli_test.cpp` (FR-017) |
| every byte above holds | `ctest --test-dir build -R 'harness_hunt_report_test\|harness_hunt_json_test' --output-on-failure` |
