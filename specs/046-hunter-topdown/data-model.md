# Data Model: Hunter top-down areas

**Feature**: `046-hunter-topdown` | **Spec**: `specs/046-hunter-topdown/spec.md`

Every entity this feature adds or changes, the value one area holds, the two
documents it writes, and the bindings that move with it. `FR-nnn`, `D-n`,
`AP-n` and `SC-nnn` cite `spec.md`, which carries D-1 to D-17 and FR-001 to
FR-037. The house shape comes from `specs/042-topdown/data-model.md` and
`specs/040-hunter-sensitivity/data-model.md`.

## Evidence

Code facts carry a `path:line` opened on this tree at `b1dd7670`. Measured
figures name the report they came from: the audit point for host facts,
`/tmp/046-plan/core.md` for the mapper design, `/tmp/046-plan/surface.md` for
the surface, and `/tmp/046-plan/tests.md` for the pins.

| Fact | Figure | Where |
| --- | --- | --- |
| The ranked record and its last field | three optional figures ending at `delta` | `include/speedgun-ng/hunt.hpp:454-472` |
| The list the stamp walks | one record per planned column, ranked first | `include/speedgun-ng/hunt.hpp:474-489` |
| The batch pass the stamp joins | the specs/040 stamps, then the records move back | `source/harness/cli.cpp:1950-1987` |
| The node fields the mapper reads | `name`, `parent`, `level`, `events` | `include/speedgun-ng/counters_topdown.hpp:232-241` |
| The tree the mapper reads | nodes in table order, refusals beside them | `include/speedgun-ng/counters_topdown.hpp:296-303` |
| The five refusal kinds | `UNKNOWN_VENDOR`, `NO_DIRECTORY`, `HYBRID`, `ARM`, `NO_TOPDOWN_ROWS` | `include/speedgun-ng/counters_topdown.hpp:329-336` |
| The row field the grouping reads | the bare file name a row was parsed out of | `source/counters/detail/pmu.hpp:80` |
| The two hunt option records | neither names `topdownLevel` | `source/harness/hunt.cpp:420-427`, `:1048-1063` |
| The internal seams of the ask | level, table, identity | `source/harness/detail/internal.hpp:751`, `:759`, `:775` |
| The closed per-event schema object | order, required, closed, properties | `docs/schema/report-v1.json:946-979` |
| The two printers of a row | ranked branch, unranked branch | `source/harness/report.cpp:1336-1366` |

## 1. `RankedEvent::topdownArea` (the one public member, D-8)

| Aspect | Rule | Fixed by |
| --- | --- | --- |
| Type | `std::string` | D-8 |
| Position | last, after `delta` at `include/speedgun-ng/hunt.hpp:471`, in the specs/040 comment shape | D-8 |
| Empty means | no area was settled for this record: a document older than minor 1.14, or a record a caller built by hand | D-8 |
| Non-empty means | the area one run settled, `unmapped` included | D-8, FR-009 |
| Written by | the area stamp in the batch pass, after the ranks of every entry settle | FR-009 |
| Read by | the ranked-row printer, the document writer, `loadReport`, and any later caller of the public record | FR-013, FR-019, FR-021 |
| Must not read it | `toCounterSet` and `toCompareRules`, `planHunt`, the pass planner, the budget, the four rank sort keys, and the skipped list | FR-010, FR-011 |
| Layout effect | the exported layout of the harness archive moves | D-8 |

The member adds no interface, so no `\pre`, `\post` or `\invariant` tag joins:
the record's own block carries prose alone
(`include/speedgun-ng/hunt.hpp:442-453`). The comment that joins names the
record and the requirement, in the shape of the specs/040 block at
`include/speedgun-ng/hunt.hpp:464-468`.

A hand-built record reaches the console today: the docs example calls
`rankEvents` and stamps nothing
(`example/docs/hunt_rank.cpp:104-118`). That record holds an empty member, and
FR-014 prints no cell for it.

## 2. `TopdownEventGroup` and `topdownEventGroups` (the counters change, D-16)

Declared in `include/speedgun-ng/counters_topdown.hpp`, defined in
`source/counters/linux_pmu/topdown_select.cpp` beside `cpuIdentity()`
(`:134-156`) and `fillSourceRecord()`
(`source/counters/linux_pmu/topdown_select.hpp:38`). No other counters file
changes and the archive `SOVERSION` rises from 5 to 6.

| Entity | Field or signature | Rule | Fixed by |
| --- | --- | --- | --- |
| `TopdownEventGroup` | `std::string event` | the bare `EventName` of one vendored row, with no device part | D-16, FR-004 |
| `TopdownEventGroup` | `std::string group` | the stem of the file that row stands in, `.json` dropped, verbatim | D-16, FR-004 |
| accessor | `topdownEventGroups(const CpuIdentity&) -> std::vector<TopdownEventGroup>` | the pairs the selected directory holds, in table order | D-16, FR-008 |
| empty answer | an empty vector | the identity selects no directory: an `MIDR`, an unknown vendor, no mapping row | D-16, D-5 |

`pre`, `post` and `invariant` for the accessor: `\pre none`, because an
identity is data and no field of it refuses a call; `\post` the pairs of the
directory the identity selects, read from build-time data, with no counter
read, no clock read and no path read; `\invariant none` on the record, which
is data. The pairing follows the rule `cpuIdentity()` shows: `\pre none` in the
header (`include/speedgun-ng/counters_topdown.hpp:80-84`) with one
`SG_ENSURE` in the body (`source/counters/linux_pmu/topdown_select.cpp:143-148`)
and no `SG_REQUIRE`.

The file stem is cut inside the counters definition, the one place that reads
`PmuTableEntry::fileName` (`source/counters/detail/pmu.hpp:80`). A row whose
`metricName` stands non-empty names a formula, and it joins a pair like any other
row, because the accessor answers every row of the directory it reads
(`source/counters/detail/pmu.hpp:58-66`, ruling R4). Measured stems on this host: `cache`
157 rows, `memory` 115, `pipeline` 102, `frontend` 49, `virtual-memory` 28,
`floating-point` 13, `other` 6, and five `uncore-*` stems with 3312 rows
(AP-7).

## 3. The grouping index the harness builds

| Aspect | Rule | Fixed by |
| --- | --- | --- |
| Key | the leaf part of a hunt address, the device prefix dropped | D-16, FR-004 |
| Value | the vendor file stem of that row, verbatim | FR-004 |
| Build | one read of the listing per run, then one insert-if-absent line per row | D-16 |
| First row wins | a later row of a name already present changes nothing | D-16 |
| Empty index | the tree ask refused, so no listing is read | FR-005 |
| No rename | the harness holds no event list and no vendor word of its own | FR-004 |

The duplicate case the data holds: five hybrid directories carry one name
twice inside one file under a second `Unit` device, `arrowlake` 180 rows,
`lunarlake` 64, `meteorlake` 62, `pantherlake` 61 and `alderlake` 47, with
`arrowlake/DL1.DIRTY_EVICTION` under `cpu_atom` and `cpu_lowpower` as the
named case (D-16, FR-031). No pinned directory holds one name in two files
(AP-7). One insert-if-absent line carries the rule, documented at the site, and
adds no branch a pinned directory can reach.

Two keys differ on purpose. The node arm keys on the whole address, because a
node leaf is a catalog address in the same string space as a ranked address
(`include/speedgun-ng/hunt.hpp:210`, `:265`). The grouping arm keys on the leaf
part, because a vendored row carries a bare name and no device
(`source/counters/detail/pmu.hpp:37`). `leafPartOf` cuts after the last `/`
(`source/harness/detail/hunt_selection.hpp:99-104`), and D-16 drops the text
before the first `/`. The two agree while a device name holds no `/`, which is
what the pmu provider builds (`source/counters/linux_pmu/provider.cpp:338`).

## 4. The area value

| Kind | Answer | When | Example on this host |
| --- | --- | --- | --- |
| Node path | the `name` of the chosen node above the `name` of each parent, joined `" > "` | a node of the run's tree names the event | `topdown.backend_bound > topdown.intel.tma_l1_bound` |
| List grouping | one vendor file stem | no node names it and the selected directory holds a row | `cache`, `virtual-memory`, `uncore-cache` |
| Unmapped | the word `unmapped` | no node names it and no row names it, or the tree refused | `unmapped` |

| Rule | Statement | Fixed by |
| --- | --- | --- |
| Depth | the node of the greatest `level` that names the event wins | D-4, FR-001 |
| Tie | of two such nodes at one level, the lower index in `nodes` wins | D-4, FR-003 |
| One name | a node at level 1 answers its own name alone | FR-007 |
| Spelling | every name verbatim, with no cut, no prefix strip and no rename | D-10, FR-004 |
| Spelling match | a name equals the whole spelling a node carries, so a kernel alias reaches no node | FR-002, AP-4 |
| Refusal | an unsupported tree answers `unmapped` for every address, in all five kinds | FR-005, D-5 |
| Depth asked | the tree is the answer at level 6, which each table clamps to its own depth | D-17, FR-001 |
| Stability | one pair of inputs answers one text on every call | FR-006 |
| Never empty | the answer holds a value for every address | FR-001 |

The split one live hunt answers on this host: over the 497 printed rows, 58
node paths, 412 list groupings and 27 unmapped, and over the 20 ranked rows 1,
10 and 9 (AP-9). SC-007 scores that basis, and the 109 of AP-7 counts vendored
formula rows instead.

## 5. The document

### The per-event object before and after

| Position | At the base | At the head |
| --- | --- | --- |
| 1 to 6 | `address`, `rate`, `cv`, `noisy`, `multiplexed`, `rank` | unchanged, and still the whole `required` list |
| 7 to 9 | `correlation`, `slope`, `delta` | unchanged |
| 10 | none | `topdownArea`, a string, written only where the member holds a value |

| Aspect | Rule | Fixed by |
| --- | --- | --- |
| Key name | `topdownArea`, spelled as the C++ member | FR-019 |
| Position | after `delta`, which ends the order at `docs/schema/report-v1.json:957` and the properties at `:977` | FR-020 |
| Absence | the key is absent when the member stands empty, which is the rule the console keeps | FR-014, FR-019 |
| Required | the key stays out of `required` (`docs/schema/report-v1.json:959-966`) | FR-020, FR-021 |
| Closed object | `additionalProperties: false` stands, so an unknown key stays refused (`:967`) | D-11 |
| Order | the checker refuses a key out of the stated order (`source/harness/json_load.cpp:2987-2998`) | D-11 |
| Writer site | one line after the `delta` write at `source/harness/json_report.cpp:604` | FR-019 |

### The reader, and its three states

| Document state | Member state | Refusal | Fixed by |
| --- | --- | --- | --- |
| key absent | empty | none | FR-021 |
| key present, a string | that text | none | FR-021 |
| key present, another kind | unchanged | the standing refusal at the pointer into the document | FR-021 |

The read rides `optionalField` (`source/harness/json_load.cpp:86-94`), the
shape the loader already uses for a key a later minor added
(`source/harness/json_load.cpp:1941-1955`). The loader gains no minor-version
comparison (FR-021). The designated list at
`source/harness/json_load.cpp:1912-1924` names every member by hand, so the new
member reads there or defaults to empty.

### The version of the document

| Item | Base | Head | Site |
| --- | --- | --- | --- |
| `kSchemaVersion` | `1.13` | `1.14` | `source/harness/json_report.cpp:47` |
| Version-history comment | the 1.13 clause | one clause naming the key and this spec | `source/harness/json_report.cpp:31-46` |
| Schema `x-order` | ends at `delta` | `topdownArea` after `delta` | `docs/schema/report-v1.json:948-958` |
| Schema `properties` | ends at `delta` | one string property after `delta` | `docs/schema/report-v1.json:968-978` |
| Schema `required` | six keys | unchanged | `docs/schema/report-v1.json:959-966` |
| Current-minor pins and comments | `1.13` | `1.14`, in the writer commit | FR-023 |
| The two later-minor plants | `1.13` | `1.15`, one minor above the new current | FR-023 |

The schema file carries no version value to move: `schemaVersion` there is a
plain string type (`docs/schema/report-v1.json:11-13`). The prose that names
the minor stands in the guide page (`docs/pages/harness.md:1397-1403`) and moves
with the writer.

### The bytes that stand

A run with no `--hunt` differs from the same run at the base in the
schema-version value and in `speedgun.version` alone (FR-022). The two
committed fixtures of that run change by zero lines, because their own masking
drops the version value (D-12). The skipped list gains no key, so it keeps its
shape at `docs/schema/report-v1.json:981-993`. The emit files read `rank`,
`address` and `delta` only (`include/speedgun-ng/hunt.hpp:499-508`,
`:520-531`), so their bytes stand (FR-011).

## 6. The console row

```text
before: hunt: rank <n> <address> <norm>=<rate> cv=<cv> correlation=<c> slope=<s> delta=<d>[ multiplexed]
before: hunt: rank - <address> <norm>=<rate> cv=<cv> correlation=<c> slope=<s> delta=<d> mark=<word>
after:  the same two lines with one cell ` area=<path>` before the flag or the mark
```

| Aspect | Rule | Fixed by |
| --- | --- | --- |
| Insertion, ranked row | after `printSensitivityCells` at `source/harness/report.cpp:1349`, before the flag test at `:1350` | FR-013 |
| Insertion, unranked row | after `printSensitivityCells` at `:1363`, before the mark write at `:1364` | FR-014 |
| Shape | one leading space, no field width, in the shape of the cells beside it | D-13 |
| Empty member | no cell at all, so a hand-built row keeps every recorded byte | FR-014 |
| Both row kinds | the ranked row and the row that ends with `mark=` carry the cell when the member holds a value | FR-014 |
| Bytes that stand | the skipped rows, the plan line, the cut line, the sample line, the multiplex line and the summary line | FR-015 |
| The level-1 block | one block per run, after the ranked rows of that benchmark, with the standing call at `source/harness/report.cpp:765` suppressed for a hunter run | D-9, FR-016 |
| A refused host | the existing refusal line prints once and no status moves | FR-017 |

The block itself keeps every rule `N10` gave it: the header, the clamp line, the
source record, one line per node, one line per refused row and the note
(`source/harness/report.cpp:359-375`). The printer reads the harness record
(`include/speedgun-ng/benchmark.hpp:794-803`), which holds no event list, so the
deep ask reaches no console byte.

## 7. Version bindings

| Number | Base | Head | Reason | Site |
| --- | --- | --- | --- | --- |
| Project version | `0.30.0` | `0.31.0` | a new public member and a new public accessor | `CMakeLists.txt:59` |
| Harness `SOVERSION` | 20 | 21 | `RankedEvent` gains a member in the exported layout | `CMakeLists.txt:407` |
| Archive `SOVERSION` | 5 | 6 | the counters header gains an interface | `CMakeLists.txt:134` |
| Schema minor | `1.13` | `1.14` | one key inside the closed per-event object | `source/harness/json_report.cpp:47` |

All four move in one commit (FR-028). Each `CMakeLists.txt` move carries the
comment that names the record and the requirement, in the shape of the blocks at
`CMakeLists.txt:52-58`, `:129-133` and `:399-406`. Both version-binding readers
move in that same commit and read a `SOVERSION` property line beside their
target, so the comment chain that names old numbers cannot settle either check;
FR-028 names the four substring lines that stand today.

## 8. Relationships

```text
CpuIdentity --selects--> vendored directory --rows--> TopdownEventGroup listing
        |                                                    |
        +--topdownTree(level 6)--> TopdownTree.nodes   insert-if-absent index
        |                              |  name, parent,          | leaf part
        |                              |  level, events           |
        |                              v                          v
        +--topdownTree(level 1)--> area = path | stem | unmapped  (the mapper)
                                                   |
   EntryHunt.entries --> the area stamp --> RankedEvent.topdownArea
                       |                          |          |
                       |                          v          v
                       |                  console cell   topdownArea key
                       |                                  (after delta)
                       v
                 toCounterSet, toCompareRules  (read no area)
   level-1 record --> internal channel --> topdown block after the ranked rows
```

## 9. Validation rules

| Input | Answer | Stated by |
| --- | --- | --- |
| An address no node names and the directory holds no row for | `unmapped`, and the row keeps its rank | FR-004, edge case |
| An address two nodes name at one level | the lower index in `nodes` names the area | FR-003 |
| An address a formula names through one of two spellings | the other spelling answers its grouping or `unmapped` | FR-002, AP-4 |
| An address whose device is an uncore device | the grouping of its own `uncore-*` file | edge case, AP-7 |
| A tree answer that refused, in any of the five kinds | `unmapped` for every address | FR-005 |
| An identity with an `MIDR` | an empty listing, no nodes, `unmapped` everywhere | D-5, D-16 |
| An event the host refused | the skipped list stands as it does and holds no cell | edge case, FR-015 |
| A hunt whose every event was refused | no ranked row, so no cell and no key | edge case |
| A record the stamp never reached | no cell, no key | FR-014, FR-019 |
| A document at minor 1.13 or older | every member empty, no refusal | FR-021, SC-002 |
| A document with a non-string `topdownArea` | the standing refusal with the pointer into the document | FR-021 |
| A run beside `--topdown` with a level | that level names the one block, and the mapper keeps level 6 | FR-034, FR-033 |
| A `--dry-run` hunt | the plan bytes stand and no block prints | FR-015, US3 AC-3 |

## 10. Invariants

Each row is a sentence a contract macro can carry, with the macro the tree uses
for that kind.

| # | Sentence | Carried as | Held by |
| --- | --- | --- | --- |
| I-1 | The area of any address is never empty | `SG_ENSURE` on the return | the mapper, FR-001 |
| I-2 | Two calls with one address and one input record answer one text | `SG_ENSURE` on the return | the mapper, FR-006 |
| I-3 | The mapper reads no counter, opens no event, reads no clock and asks no kernel interface | header contract, plus the time-source gate over the harness | the mapper, FR-001 |
| I-4 | The tree and the refusal stand engaged in exactly one of the two states | `SG_INVARIANT` over the input record | the one read, D-17 |
| I-5 | The grouping index stands empty whenever the refusal stands | `SG_ENSURE` after the read | the one read, FR-005 |
| I-6 | One key stands per event name, and a later row of a held name changed nothing | `SG_ENSURE` after the build | the index, D-16 |
| I-7 | Every record of every hunt list of a hunt run holds a non-empty area | `SG_INVARIANT` at the end of the stamp | the stamp, FR-009 |
| I-8 | The stamp added no record, removed none and reordered none, and left every other field as it stood | `SG_ENSURE` over the loop | the stamp, FR-010 |
| I-9 | The skipped list holds the same entries it held before the stamp | `SG_ENSURE` over the list | the stamp, FR-009 |
| I-10 | One ` area=` cell prints for each row whose member holds a value, and none for a row that holds none | header contract of the cell helper, proved by the console unit | the printer, FR-013, FR-014 |
| I-11 | The document carries the key exactly where the member holds a value | the schema unit and the checker | the writer, FR-019 |
| I-12 | The two asks of one run differ in level alone | `SG_REQUIRE` on the ask builder | the ask builder, FR-033 |
| I-13 | A run with no `--hunt` writes the schema-version value and the project version as its only difference | the identity entries of the two fixtures | the writer, FR-018, FR-022 |
| I-14 | No public record of the harness gained a field for the companion | the version-binding readers and the DBC pair gate | the channel, FR-033 |

## 11. States of one value

```text
run with no hunt ............................ member empty, no cell, no key
hunt, tree refused .......................... unmapped, one refusal line, no block
hunt, tree answered -> stamp after the ranks settle
      node names it .......................... the path of the deepest node
      a row names it ......................... the file stem of that row
      nothing names it ....................... unmapped
   -> console cell -> document key -> loadReport -> the member again
```

The stamp runs once, after the ranks settle and before the deferred console, the
writer and the emit files (`source/harness/cli.cpp:1563-1564`,
`:1985-1987`, `:2036-2042`, `:2182-2204`). A value never changes after the stamp
and no state revisits an earlier one.
