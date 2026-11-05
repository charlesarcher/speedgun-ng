# Phase 0 Research: Hunter top-down areas

Feature and branch: `046-hunter-topdown`, cut 2026-11-03 from `master` at
`b1dd76707788ab71bcea184877c7e9b727cde881`. The authority for every ruling
below is `specs/046-hunter-topdown/spec.md`, which carries the decisions as
D-1 to D-17, the audit point as AP-1 to AP-12, the requirements as FR-001 to
FR-034 and the outcomes as SC-001 to SC-010. This file records the evidence
that decided each ruling, the reading each ruling refused, the gate lanes with
their base answers, the risks with their mitigations, and the open items.

Host for every live reading: `corsairone`, one socket, Intel Skylake-X, 24
cpus, kernel `7.3.0-4-cachyos`, GCC 16.2.1 and clang 23.1.3
(`/tmp/046-research/audit.md`). Line numbers cite the base commit, which is
the checked-out tree.

## 1. Sources for every figure

| Tag | File | What it holds |
| --- | --- | --- |
| `spec.md` | `specs/046-hunter-topdown/spec.md` | The rulings, the audit point, the requirements, the outcomes |
| audit | `/tmp/046-research/audit.md` | Host, tool versions, the base build and suite, the README and page line counts, one live hunt |
| topdown | `/tmp/046-research/topdown.md` | The `N10` surface, the vendored key census, the reachability verdicts |
| hunt-core | `/tmp/046-research/hunt-core.md` | The hunt surface, the run path, the emit writers, the insertion points |
| gates | `/tmp/046-research/artifacts-gates.md` | Gate mechanics, presets, rule tables, the artifact house style |
| PLAN | `/tmp/046-research/PLAN.md` | The working plan, the analyzer reading, the coverage second opinion |
| core | `/tmp/046-plan/core.md` | The mapper design at signature level, the two companion shapes |
| surface | `/tmp/046-plan/surface.md` | The console, document, version and documentation surface with its measurements |
| tests | `/tmp/046-plan/tests.md` | The test plan, the gate plan with base readings, the pin list |
| matrices | `/tmp/046-plan/gatebase/doc-matrix.json` and `pair-matrix.json` | The two DBC matrices from one hand run at the base |

Code is cited as `path:line` for a line read in this tree. A figure with no
command beside it comes from a report named in the row.

## 2. Decision table

The spec holds seventeen decisions. There is no D-18: the pointer
`D-1..D-18` at `/tmp/046-plan/core.md:8` names a decision the spec does not
carry, and the two round-3 rulings instead stand as FR-033 and FR-034.

| # | Ruled | Refused |
| --- | --- | --- |
| D-1 | Read the node event list through `counters::TopdownNode::events` | A counters change to expose node events |
| D-2 | No option, no short option, no environment variable | An off switch on the area column |
| D-3 | The fallback names the vendored file stem, verbatim | A `Topic` or `category` field; a hand-written event list |
| D-4 | The deepest node wins, ties by table order, one path recorded | A list of every node an event belongs to |
| D-5 | Every event of an Arm identity answers `unmapped` | An Arm vendor table |
| D-6 | No rule text changes; the area enters no rule | A rule form over areas |
| D-7 | The area is derived: no counter, no clock, no kernel read | A measurement pass for the area |
| D-8 | `RankedEvent` gains `std::string topdownArea`; harness 20 to 21, archive 5 to 6 | A side map over addresses; an optional member |
| D-9 | One short level-1 pass beside the hunt stages, console only | Level 1 inside the stage-1 plan; the entry `topdown` object |
| D-10 | Node names verbatim, joined with `" > "` | A cut name for the short form |
| D-11 | One key in the closed per-event object, out of `required` | An open object; a required key |
| D-12 | Emit bytes stand; two no-hunt fixtures stand unchanged | A live before-and-after byte comparison |
| D-13 | One `area=<path>` cell after the specs/040 cells, no field width | A padded column; a separate line |
| D-14 | `corsairone` is the acceptance host; CI never accepts an area | A CI run as the acceptance of a value |
| D-15 | 100 percent line, branch allowance 21, no new marker | A marker to reach the number |
| D-16 | One read-only accessor `topdownEventGroups` in the counters library | A second json reader in the harness; widening `CatalogEntry` |
| D-17 | Two asks per run: level 6 for the area, level 1 for the block | One shared level for both uses |

## 3. Evidence per decision

### D-1 The node event list is public already

**Evidence**: `TopdownNode` publishes `events` as a `std::vector<std::string>`
(`include/speedgun-ng/counters_topdown.hpp:239`) and the selection fills it
from the spine leaves of the row
(`source/counters/linux_pmu/topdown_select.cpp:525-527`). The verdict in
topdown section 7 reads: reachable today, `topdown_select.*` needs no change.

**Refused**: a new counters surface for node events, and a harness walk of
`formula.core.leaves`. FR-008 names `events` as the only read.

### D-2 No option

**Evidence**: FR-012 states that `source/harness/detail/options.hpp` stands
unchanged and that no option, short option or environment variable joins. The
area column and the level-1 block stand in every hunter run.

**Refused**: a switch to hide the column. A later off switch takes the free
option value block the spec names and its own comment.

### D-3 The vendored file stem is the only vendor category

**Evidence**: the key census over the pinned tree answers 542 json files that
parse and no `Topic`, `Category` or `Layer` key anywhere, with `MetricGroup`
on metric rows alone (topdown section 7). `PmuTableEntry` holds the row's bare
`name` (`source/counters/detail/pmu.hpp:37`) and the file it was parsed out of
(`source/counters/detail/pmu.hpp:80`). Over the pinned Skylake-X directory the
stems are `cache` 157 rows, `memory` 115, `pipeline` 102, `frontend` 49,
`virtual-memory` 28, `floating-point` 13, `other` 6 and five `uncore-*` files
with 3312 rows, 3782 in all (AP-7). Zen 4 publishes `branch.json` and Zen 5
publishes `l2-cache.json`, so the vocabulary is per directory.

**Refused**: the `Topic` field the request assumed, because the data holds
none. Also refused: an event list written in the harness, which FR-004 bars.

### D-4 Deepest node, table order on a tie

**Evidence**: `TopdownTree::nodes` holds every node in table order
(`include/speedgun-ng/counters_topdown.hpp:301`), so the lowest index is the
first row (topdown section 7). Recording one path keeps the closed object
small, which D-4 states as the reason.

**Refused**: a list of every node a shared event belongs to, and the
shallowest node. Both move the record shape without a reader that needs them.

### D-5 Arm answers `unmapped`

**Evidence**: the vendored scope is `tools/perf/pmu-events/arch/x86`
(`external/pmu-events/RECORD:6`), the selection answers `TopdownUnsupport::ARM`
(`include/speedgun-ng/counters_topdown.hpp:334`) for an `MIDR` before it reads
any directory (`source/counters/linux_pmu/topdown_select.cpp:415-423`), and
FR-005 then answers `unmapped` for every address.

**Refused**: an Arm table. The Scope section gives new vendor tables to the
`N10` lane.

### D-6 No rule text changes

**Evidence**: D-6 keeps the meaning `N10` gives a `topdown:` rule, and the
area is a printed and recorded fact. The Scope section bars any use of the
area inside `--compare-rules`.

**Refused**: a rule form over an area string.

### D-7 The area is derived

**Evidence**: FR-001 states that the answer follows from the values given, and
`topdownTree` carries the same purity in its own post
(`include/speedgun-ng/counters_topdown.hpp:427-429`). The stamp reads the
records and the three inputs alone.

**Refused**: a pass that measures the area. FR-010 forbids any move of a rank,
a budget or an emitted byte.

### D-8 One member, two SOVERSION rises

**Evidence**: `RankedEvent` stands at `include/speedgun-ng/hunt.hpp:454-472`
and ends with `delta` at `:471`, the shape the specs/040 comment block uses.
The exported numbers at the base are the harness `SOVERSION` 20
(`CMakeLists.txt:407`) and the archive `SOVERSION` 5 (`CMakeLists.txt:134`).
D-8 states the two rises, and the empty string is the state of a document
older than minor 1.14 or of a hand-built record.

**Refused**: an `std::optional<std::string>`, because D-8 names a plain
`std::string`. Also refused: a side map keyed by address, which is a second
source of truth beside the record.

### D-9 One short level-1 pass, console only

**Evidence**: a hunt builds its `detail::RunOptions` twice, at
`source/harness/hunt.cpp:420-427` and at `source/harness/hunt.cpp:1048-1063`,
and neither sets `topdownLevel`, so the run collects no top-down row (AP-8).
`stripTopdownColumns` (`source/harness/runner.cpp:699`, called at
`:3528`) is what turns collected leaves into `BenchmarkResult::topdown`. Core
section 2.1 measures the cost of the other shape: the four node formulas join
the stage-1 column set, so the pass split and the multiplexing of that stage
move. Round 3 refused the carrier in the document, because it can move rank
order and adds an object no requirement asks for.

**Refused**: level 1 inside the stage-1 plan, and the entry
`BenchmarkResult::topdown` (`include/speedgun-ng/benchmark.hpp:975`) as the
carrier.

### D-10 The path is verbatim

**Evidence**: a node carries `name`, `vendorName` and `parent`
(`include/speedgun-ng/counters_topdown.hpp:234-236`) and the `parent` text
names the `name` of another node, so one field walks a path. The measured
pair is `topdown.intel.tma_l1_bound` above `topdown.intel.tma_memory_bound`
(core section 5). Round 3 kept the short form of US1 out, because a cut name
is a rename FR-004 forbids.

**Refused**: a harness-side cut of the vendor prefix.

### D-11 One key of a closed object

**Evidence**: the per-event object lists `x-order`
(`docs/schema/report-v1.json:948-958`), keeps a `required` list
(`:959-966`) and stands closed at `:967`. The checker refuses an unlisted key
of a closed node (`source/harness/json_load.cpp:2976-2984`) and a key out of
the stated order (`:2987-2998`). The three specs/040 figures hold that shape
today at `:955-957` and `:975-977`.

**Refused**: an open object, which cannot tell an absent key from an old
writer, and a required key, which breaks the load FR-021 asks for.

### D-12 Byte identity, and no live byte proof

**Evidence**: `toCounterSet` answers addresses and `toCompareRules` reads the
sign of `delta` (`include/speedgun-ng/hunt.hpp:499-508`, `:520-531`), so an
area moves no byte. AP-10 measures that one unchanged binary answers
different emit bytes from run to run, and that a non-zero `--hunt-seed`
aborts at `source/counters/fold.cpp:166`. The two committed fixtures of a
no-hunt run pin a stale minor of their own: the document carries
`schemaVersion` 1.4 and version 0.19.0 (tests section 5.2).

**Refused**: a live before-and-after byte comparison as the proof of FR-011.
The proof is one unit over one `EntryHunt` and a diff that names no hunk.

### D-13 The console cell

**Evidence**: `printHuntRanking` prints the ranked row at
`source/harness/report.cpp:1341-1353` and the unranked row at `:1356-1364`,
each after `printSensitivityCells` at `:1349` and `:1363`. The flag cell reads
` multiplexed` at `:1351` and the mark cell reads ` mark=` at `:1364`. The
cell shape beside them is one leading space with no width
(`source/harness/report.cpp:1186`, `:1196`).

**Refused**: a field width, and a line of its own. Both move bytes the
recorded blocks hold.

### D-14 Hosts

**Evidence**: D-14 names `corsairone` as the acceptance host, sends the AMD
rows to another lane's gap, and states that the CI runners answer the refusal
path. The base suite and the two-cpu lane were both measured here (AP-1,
AP-11).

**Refused**: a CI run as the acceptance of an area value.

### D-15 Coverage

**Evidence**: `tools/dbc/coverage_gate.sh:102` carries `allowance=21`, and the
gate reads 100 percent line (gates section B6). The mapper is a pure function
over three inputs, so core section 1.4 lists one unit case for each of its
four arms with no host in the case.

**Refused**: an exclusion marker to reach the number. FR-031 bars one, and
gates section B6 records the 007 audit that cleared 91 markers and found two
defects under them.

### D-16 The grouping accessor

**Evidence**: `pmuLoadTable` and `PmuTableEntry` sit in the private seam
`source/counters/detail/pmu.hpp`, loaded lazily once per directory
(`:125-128`), and no file under `source/harness/` includes that header
(topdown section 7). `CatalogEntry` carries no file and no event list, and
`TopdownSource::files` marks a file as contributing for any row of it, so it
is no event-to-file map. The listing is read once per run: five hybrid
directories hold one name twice inside one file under a second `Unit` device,
`arrowlake` 180 rows, `lunarlake` 64, `meteorlake` 62, `pantherlake` 61 and
`alderlake` 47, and no pinned directory holds one name in two files (D-16).
The cost is one header declaration, one definition beside `fillSourceRecord`
(`source/counters/linux_pmu/topdown_select.hpp:38`), and the archive rise.

**Refused**: a second vendored-json reader inside the harness, as a second
source of truth over data the library parses. Also refused: a
`CatalogEntry` field, which moves another exported layout.

### D-17 Two asks, one run

**Evidence**: AP-9 measured one ask per level on this host: a level-1 tree
settles a path for 7 of the 497 printed addresses and a level-6 tree for 58.
AP-12 measured the cost: 503 ms for the first table read of a process and
0.57 ms for the second, from one cached read. `TopdownSelection::level`
carries the difference (`include/speedgun-ng/counters_topdown.hpp:323`) and
the range 1 to 6 stands required (`source/counters/linux_pmu/topdown_select.cpp:411`).

**Refused**: one shared level. A shared level 6 would print a level-6 block,
and a shared level 1 would answer no node path for 490 of the 497 rows.

### The two rulings that carry a requirement number

**FR-033** rules that one identity, one selection and one table read feed both
uses, that the two levels stand apart in one run, and that a unit can plant the
identity and the table on the companion path with no new public field.
**FR-034** rules that an explicit `--topdown` level names the one block and
that the implicit level 1 applies only where no `--topdown` stands. The
refusal each refused is a single level held in one field for both uses, which
is what `RunOptions::topdownLevel` (`source/harness/detail/internal.hpp:751`)
would give if the hunt set it.

## 4. Measured host facts

AP-1 to AP-12 stand in `spec.md` as the audit point. This table names what each
one decides and the command that produced it, where a command exists.

| Fact | Reading | Source of the reading |
| --- | --- | --- |
| AP-1 | 178 entries, 3 failures, all three in files the 045 lane owns | base `ctest` run, audit |
| AP-2 | lint, spell and drift each 0; both DBC gates 345 interfaces and 0 gaps | base gate run, matrices |
| AP-2b | the range form is void at the branch point | `prose_gate.py` warning |
| AP-3 | plan reads `events=497 passes=249 method=PASSES`, 20 ranked rows | one live hunt |
| AP-4 | `cpu/branch-instructions` and `cpu/BR_INST_RETIRED.ALL_BRANCHES` are two rows of one address | the AP-3 capture against the pinned directory |
| AP-5 | 1000 page lines against a cap of 1004, and `Hunter mode` 24 of 25 | `wc -l`, the drift tool |
| AP-6 | `lcov` dies at startup and the coverage gate answers exit 3 | `lcov --version` |
| AP-7 | 3782 event rows, 109 named by a formula, 361 grouped, 27 unmapped | a walk of the pinned directory against AP-3 |
| AP-8 | neither hunt `RunOptions` sets `topdownLevel` | `hunt.cpp:420-427`, `:1048-1063` |
| AP-9 | level 1 settles 7 paths, level 6 settles 58 over the same 497 rows | one ask per level |
| AP-10 | emit bytes of one unchanged binary differ between runs | two runs with both emit options |
| AP-11 | the two-cpu lane answers 5 failures over 171 entries | the taskset run at the base |
| AP-12 | 503 ms for the first table read, 0.57 ms for the second | two asks in one process |

```sh
cmake --preset=ci-rocky
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
cmake -P cmake/lint.cmake
cmake -P cmake/spell.cmake
python3 tools/docs/example_drift.py --sections --names --options --forbidden .
taskset -c 0,1 ctest --test-dir build -L quick -j 2 --timeout 120
build/example/memory_walk --hunt --hunt-budget=90s
build/example/first_benchmark --hunt --hunt-budget=90s
lcov --version
```

Three readings need a note. AP-7 counts vendored rows, published on this host
or not, and AP-9 scores the printed rows, so the two bases differ: 109 against
58 for one run. AP-10 also measured that the abort of a named seed is the
assertion at `source/counters/fold.cpp:166`, a base defect this feature names
and does not carry. AP-12 shows that the deep ask rides a read the run pays
for anyway, which is the Principle VII ground for D-17.

## 5. Gate lanes

Each lane states what it reads, what it answered at `b1dd7670`, and the
command, copied whole. Base answers come from the audit point, from the two
DBC matrices written at the base, from PLAN for the analyzer and coverage
readings, and from the commit body of the base that gates section B8 quotes.

| Lane | Reads | Base answer |
| --- | --- | --- |
| Build and suite | every target, every CTest entry | green build, 178 entries, 3 failures (AP-1) |
| Two-cpu quick | entries labeled `quick` under two cpus | 171 entries, 5 failures (AP-11) |
| Format and spell | the formatted trees, then `codespell` | 0 and 0 (AP-2) |
| Docs drift | pages, regions, the page budget, the option and name tables | 0 with no flag and 0 with all four flags (AP-2) |
| Prose, range form | the lines one commit range adds | void at the branch point (AP-2b) |
| Prose, tree form | every tracked source | 0 findings over 966 sources |
| DBC doc gate | the Doxygen xml of `include/speedgun-ng` | 345 interfaces, 0 gaps |
| DBC pair gate | that matrix against `SG_*` tokens in `include`, `source`, `tools` | 345 interfaces, 0 gaps |
| Static analysis | clang-tidy and cppcheck in a fresh tree | 20,862 distinct findings, 682 cppcheck lines, 0 naming findings |
| Coverage | the instrumented run of the whole suite | `coverage_gate.sh` exit 3, no ratio (AP-6) |
| Time source | every time source outside `source/counters/` | no base reading in the reports |
| Tier labels | labels, slow rows, budgets, skip rows | no base reading in the reports |
| Aggregate local gate | the prepush steps in order | no base reading in the reports |

```sh
# The two DBC gates, by hand, because the target cached a python with no PyYAML.
doxygen build/dbc-gate/Doxyfile
/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml \
    --registry tools/dbc/macros.yaml --out /tmp/046-logs/doc-matrix.json
/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng \
    --src2 source tools --registry tools/dbc/macros.yaml \
    --doc-matrix /tmp/046-logs/doc-matrix.json \
    --out /tmp/046-logs/pair-matrix.json
# Static analysis, in a fresh tree only.
cmake -S . -B build/dev --preset=ci-ubuntu
cmake --build build/dev -j"$(nproc)" 2>&1 | tee /tmp/046-logs/dev-build.log
# Prose, the three forms AP-2b names.
python3 tools/prose/prose_gate.py --base b1dd7670
python3 tools/prose/prose_gate.py --check commit --base <sha> --head <sha>
cmake -D PROSE_MODE=tree -D PYTHON_COMMAND=/usr/bin/python3 -P cmake/prose-lint.cmake
```

Four facts carry the lanes. `--xml` names a directory, so a path that names
`index.xml` reads `XML directory not found` (AP-2). The two matrices written
at the base carry 345 interfaces each, which settles the 342 figures the
earlier notes quoted. The tree form lists tracked paths, so a new file needs
`git add -N` before it is read, and a run that reads no source is void
(AP-2b). Reconfiguring `build` with `ci-ubuntu` runs no analyzer, so the
analyzer lane needs `build/dev` (gates section B1).

The coverage ratio of the acceptance report comes from the coverage job of the
pull request. This host states its exit code beside it. The second opinion is
a `gcovr` read of the same tree, and PLAN names it as incomparable to the gate
number: 19,514 of 19,979 lines and 15,148 of 16,592 branch outcomes under a
loaded run. The new files must read zero missing lines against that command.

## 6. Risks and mitigations

| Risk | Mitigation the change carries |
| --- | --- |
| The companion moves a rank, a budget or an emitted byte | The pass runs beside the stages on its own options (D-9); the two hunt records keep their fields; FR-011's unit and the empty diff carry the proof |
| A refused host reads as a broken mapper | FR-005 answers `unmapped` for every address, one refusal line prints from `source/harness/cli.cpp:1583`, and SC-007 reports the three counts |
| A CI run is read as the acceptance of an area value | D-14 bars it, and SC-009 and SC-010 hold the AMD rows as pending |
| `lcov` exit 3 is read as a coverage failure | The report names exit 3 with its tool line and takes the ratio from the pull request job (AP-6, SC-004) |
| A recorded block holds a word the host supplies | FR-027 scripts the identity through the seam of FR-016; the fold rule stands as the fallback with a planted pair that reads red without it |
| The main page cannot pay | The swap ruling of round 3 gives the block the place of one region and moves that region to the guide page; `SECTION_LINE_CAPS` stands |
| specs/047 edits the same reader | 046 edits `readRankedEvent` alone, and the second merger rebases and reruns every lane |
| A version check settles on a comment | FR-028 makes both readers read a property line beside its target, because the comment chain names old numbers on purpose |
| The prose gate reads green at the branch point | Every claim rides the range form, the commit form and the tree form (AP-2b) |
| The analyzer lane reports nothing from a tree with no launcher | The lane runs in `build/dev` and names the tree each number came from |
| A base failure or a flake is read as new | SC-005 is a name diff over both base lists, and the two flake names re-run alone |
| Upstream data puts one name in two files of one directory | The insert-if-absent rule stands documented at the site (D-16), and the count is a re-pinning check |
| A host claim is made for a part no host holds | SC-010 keeps those rows synthetic and the report names them pending |

## 7. Open items for the implementation

1. The spec holds no D-18, and the two round-3 rulings stand as FR-033 and
   FR-034. Write against the spec numbers, and name the plan pointer as stale.
2. US3 AC-3 says a `--dry-run` hunt opens no counter. The base dry run takes
   one repetition and one iteration (`source/harness/runner.cpp:2896`,
   `:3138`) and a hunt dry run also prices a pass through `measurePassNs`
   (`source/harness/hunt.cpp:427`). Only the mapper and the stamp read no
   counter (FR-001, D-7). Settle the wording or scope the claim.
3. The plan artifacts still carry the superseded console word `area=n/a`
   (core section 3.3, tests section 6). FR-014 rules no cell when the member
   stands empty, and the empty rule is what keeps `hunt_ranks.expected`
   unchanged.
4. The plan proposes a `null` for an unset member in the document. FR-019
   rules that the key stands only where the member holds a value, so the key
   is absent, the schema type is a plain string, and a `null` reaching the
   reader is a refusal under FR-021. Record that reading at the writer.
5. The payable page trims read eight in one AP-5 run and five in another, and
   the swap needs none of them; surface section 4.3 measures six of ten named
   sites as payable, with five of the ten inside fenced regions. Measure in
   the commit that writes, and name each line.
6. The block header prints `passes=` from the collection that ran
   (`source/harness/report.cpp:365-368`). The companion must carry a pass
   count of its own, beside the entry figure at
   `include/speedgun-ng/benchmark.hpp:936`.
7. FR-033 asks a unit to plant the identity and the table on the companion
   path. Settle the seam shape, and keep `RunOptions` and `HuntOptions`
   unchanged.
8. `RunOptions::topdownLevel` also gates the scope line
   `topdown: core-wide coverage=` (`source/harness/cli.cpp:1584-1593`), so a
   hunt run that sets it prints a line no requirement names. Keep the two
   asks apart in one internal value, as FR-033 requires.
9. D-16 drops the text before the first `/`, and the in-tree helper
   `leafPartOf` (`source/harness/detail/hunt_selection.hpp:99-104`) cuts after
   the last `/`. The two agree while a device path holds one name, which is
   what `source/counters/linux_pmu/provider.cpp:338` builds. State the rule
   and that ground at the site.
10. The stamp must stand before the deferred console
    (`source/harness/cli.cpp:1563-1564`, printed at `:2036-2042`), the writer
    and the two emit files (`:2182-2204`). The batch pass ends at
    `source/harness/cli.cpp:1987`, which is where the specs/040 stamps stand.
11. The DBC count grows by the interfaces D-16 names. The head must state the
    number it measures beside the command that measured it.
12. The AMD host row, the Intel hybrid row and the Arm row stay pending rows
    with no hardware claim (SC-009, SC-010, D-14).
