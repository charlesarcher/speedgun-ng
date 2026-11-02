# Data Model: Vendor-neutral Top-down Analysis

**Feature**: `042-topdown` | **Spec**: `specs/042-topdown/spec.md`

Every entity this feature adds or changes, its fields, the relationships, the
validation rules with the requirement that states each one, and the states one
run goes through. `FR-nnn`, `D-n`, `PC-n` and `SC-nn` cite `spec.md`. The
house shape comes from `specs/024-multipass-collection/data-model.md` and
`specs/022-metric-formulas/data-model.md`.

## Evidence

Code facts carry a `path:line` opened on branch `042-topdown` at
`65c48be994f1144cdd068841f83b98e8b2f31148` (`git rev-parse HEAD`). Table facts
and host facts carry the command that produced them.

| Fact | Figure | Command |
| --- | --- | --- |
| Host identity | `GenuineIntel`, family 6, model 85, stepping 4, `Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz` | `grep -E 'vendor_id\|model name\|cpu family' /proc/cpuinfo`, then `grep -m1 '^stepping' /proc/cpuinfo` |
| Kernel, cpus, paranoia | `7.3.0-4-cachyos`, 24 cpus, `perf_event_paranoid` at -1 | `uname -r`, `nproc`, `cat /proc/sys/kernel/perf_event_paranoid` |
| SMT siblings of cpu 0 | `0,12`: two threads per core, and the object tree answers no core object (PC-6) | `cat /sys/devices/system/cpu/cpu0/topology/thread_siblings_list` |
| Pinned reference | `ref: linux-7.2.4` at `external/pmu-events/RECORD:1`; the `----` rule at line 8; hash lines from line 9 | `cat external/pmu-events/RECORD` |
| Table rows | 275 metric rows in `arch/x86/skylakex/skx-metrics.json`; 102 carry a level token: L1 4, L2 8, L3 25, L4 34, L5 17, L6 14 | `python3` walk of that file |
| Threshold text | all 102 level rows carry `MetricThreshold`; 146 rows of the file carry it | same walk |
| Group tokens | 98 tokens whose stem names another row; 102 whose stem names none (`tma_L1` 4 up to `tma_L6` 14) | same walk |
| AMD tables | `arch/x86/amdzen4/pipeline.json` and `arch/x86/amdzen5/pipeline.json`: 14 metric rows each, 1 group-free helper (`total_dispatch_slots`), 5 `PipelineL1` including `smt_contention`, 8 `PipelineL2` | `python3` walk of both files |
| File hashes | `skx-metrics.json` `1ba7dd5b…`, `amdzen4/pipeline.json` `e62ee36b…`, `amdzen5/pipeline.json` `39546024…`; each equals its own `RECORD` line | `python3` `hashlib.sha256` per file, then a `grep` of `RECORD` |
| Catalog verdicts over those 102 rows | 33 published, 69 refused: 48 `UNKNOWN_CONSTANT` (39 on `#core_wide`, 9 on `#SMT_on`), 20 `UNRESOLVED`, 1 `UNSUPPORTED` | `./build/example/docs_command_line --catalog` (24999 lines), then a walk of lines whose `group=` carries a level token |
| Print object of those rows | 69 under `machine/`, 33 under `cpu/` | same capture, split on each line's address |

## Library model

New public header `include/speedgun-ng/counters_topdown.hpp` (D-4). A public
addition moves the archive `SOVERSION` from 4 to 5 (`CMakeLists.txt:110`).

### E-01 `CpuIdentity` (this feature's own seam, D-13)

| Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- |
| `vendor` | `std::string` | The vendor text CPUID leaf 0 answers; empty names an unreadable cpu | FR-003, D-4 |
| `family` | `int` | Decimal, the mapping-file spelling | FR-003 |
| `model` | `int` | The mapping-file spelling | FR-003 |
| `stepping` | `int` | Negative names no stepping class; it splits `skylakex` from `cascadelakex` | FR-003, `source/counters/detail/pmu.hpp:62-72` |
| `coreType` | `std::string` | Core type of a hybrid part; empty on every production host | FR-003, D-13 |
| `midr` | `std::optional<std::uint32_t>` | Arm `MIDR`; unset on every production host | FR-003, D-13 |

`cpuIdentity()` fills the four fields the library already reads and leaves the
last two absent, so no reader for either exists in this feature. A test hands
in all six (FR-003).

### E-02 `TableFile` and E-03 `TopdownSource` (the source record)

| Entity | Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- | --- |
| `TableFile` | `name` | `std::string` | The name the embedded index gives the file | FR-015, D-12 |
| `TableFile` | `manifestHash` | `std::string` | The file's hash, read from `RECORD` at build time; the report spells the key `manifest_hash` (`contracts/topdown-report-key.md:46-49`) | FR-015, D-12, PC-12 |
| `TopdownSource` | `reference` | `std::string` | The `ref:` line of the manifest: `linux-7.2.4` | FR-015, D-12 |
| `TopdownSource` | `directory` | `std::string` | The selected directory, for example `arch/x86/skylakex/` | FR-001, FR-015 |
| `TopdownSource` | `files` | `std::vector<TableFile>` | One entry per file that contributed a row, in the embedder's sorted order | FR-015, D-12 |

### E-04 `TopdownRow` and E-05 `TopdownTable` (the listing the selection reads)

| Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- |
| `name` | `std::string` | The table's `MetricName`, verbatim | FR-010 |
| `group` | `std::string` | The table's `MetricGroup` text, verbatim; it carries level and parent | FR-013, FR-014 |
| `unit` | `std::string` | The table's `ScaleUnit` text, verbatim | FR-010 |
| `threshold` | `std::string` | `MetricThreshold` text; empty where the row carries none | D-9, D-11, FR-017 |
| `formula` | `Expression<Dim<0, 0>>` | The scaled spine N2 publishes | FR-010 |
| `published` | `bool` | False for a row this host refuses | FR-016 |
| `refusal` | `MetricRefusal` | The kind, meaningful when `published` is false | FR-016 |
| `reason` | `std::string` | The refusal text; empty for a published row | FR-016 |
| `TopdownTable.source` | `TopdownSource` | Where the rows came from | FR-015 |
| `TopdownTable.rows` | `std::vector<TopdownRow>` | Every row of the directory, published and refused alike, in table order | FR-016, PC-11 |

The hybrid predicate of FR-006 reads the scope of a directory's event rows. The
scope is the row's `Unit` text: the parse stores it in `PmuTableEntry::unit`
(`source/counters/detail/pmu.hpp:45`, filled at
`source/counters/linux_pmu/table_parse.cpp:419` for a metric row and `:515` for
an event row) and the resolution carries it as `ParsedMetricRow::scope`
(`source/counters/linux_pmu/metric_formula.hpp:107-114`). Two core scopes among
those rows, `cpu_core` and `cpu_atom`, make the directory hybrid, and one scope
selects normally. The selection fixture `test/source/detail/topdown_fixture.hpp`
carries event rows of both core types, so a two-scope directory stands in data
with no host to read, and `fixtureEvent` sets that same field for the metric
fixtures (`test/source/detail/metric_fixture.hpp:50-59`).

### E-06 `TopdownNode`, E-07 `TopdownRefusal`, E-08 `TopdownTree`

| Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- |
| `name` | `std::string` | Vendor-neutral name: one of the four shared names at level 1, `topdown.<vendor>.<table name>` below it | FR-011, FR-012, D-2 |
| `vendorName` | `std::string` | The row's own `MetricName`, printed and recorded beside the neutral name | FR-010, D-2 |
| `parent` | `std::string` | Neutral parent name; empty at level 1 | FR-013 |
| `level` | `int` | From the row's own level token | FR-014 |
| `formula` | `Expression<Dim<0, 0>>` | The row's spine, so a caller can fold it | FR-010 |
| `events` | `std::vector<std::string>` | Each spine leaf's `LeafCore::address` (`include/speedgun-ng/counters_measurement.hpp:97-107`), the address the planner groups on | FR-010, FR-021 |
| `threshold` | `std::string` | Table text; nothing evaluates it | FR-017, D-9 |
| `TopdownRefusal.name/parent/level` | `std::string`, `std::string`, `int` | The row's identity and depth; `parent` is empty where the table names none | FR-016 |
| `TopdownRefusal.refusal/reason` | `MetricRefusal`, `std::string` | The kind and the reason, copied unchanged | FR-016, FR-031 |
| `TopdownTree.source` | `TopdownSource` | The source record | FR-015 |
| `TopdownTree.depth` | `int` | The deepest level the table holds, after the clamp | FR-008, FR-020 |
| `TopdownTree.amd` | `bool` | Chooses the fifth level-1 value and the 100 percent statement | D-3, FR-011 |
| `TopdownTree.nodes` | `std::vector<TopdownNode>` | Every node of every level, table order | FR-010 |
| `TopdownTree.refused` | `std::vector<TopdownRefusal>` | Every refused row at or above `depth` | FR-016 |

One C++ name serves both refused-row carriers, `TopdownRefusal`, the library's
row here and the record row of E-16, the way `TopdownNode` serves both node
carriers. The JSON key stays `refused` and its item keys stay `name`, `kind` and
`reason` (`contracts/topdown-report-key.md:58-61`). This file is the authority
for the C++ names and the report contract is the authority for the JSON
spellings.

### E-09 `TopdownSelection`, E-10 `TopdownUnsupport`, E-11 `TopdownUnsupported`

| Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- |
| `TopdownSelection.identity` | `CpuIdentity` | The identity the answer follows from | FR-002, FR-003 |
| `TopdownSelection.level` | `int` | The asked level, 1 to 6 | FR-018, FR-019 |
| `TopdownSelection.table` | `std::optional<TopdownTable>` | The seam a test plants rows in | FR-002, FR-009, PC-9 |
| `TopdownUnsupport` | closed `enum class` | `UNKNOWN_VENDOR`, `NO_DIRECTORY`, `HYBRID`, `ARM`, `NO_TOPDOWN_ROWS` | FR-004 to FR-007 |
| `TopdownUnsupported.kind` | `TopdownUnsupport` | The reason class | FR-004 |
| `TopdownUnsupported.vendor` | `std::string` | The vendor named in the message | FR-040 |
| `TopdownUnsupported.identity` | `std::string` | `family 6 model 85 stepping 4`, or the `MIDR` | FR-040, D-14 |
| `TopdownUnsupported.detail` | `std::string` | The directory read, or the Arm sentence | FR-005, FR-007 |
| `message()` | `-> std::string` | One line naming vendor, identity and reason | FR-040, D-14 |

`topdownTable(identity)` answers E-05 and `topdownTree(selection)` answers
`std::expected<TopdownTree, TopdownUnsupported>`. Both are pure over their
inputs: no counter read, no event open, no clock read, no CPUID call inside
(FR-002, FR-009).

### E-12 `EmbeddedFile` and `EmbeddedDir` (build-time source data)

`EmbeddedFile` gains `sha256` and `EmbeddedDir` gains `reference` (D-12). Both
are internal to `source/counters/linux_pmu/embedded_tables.hpp`, so neither
moves an exported layout. The values come from `RECORD` at build time, and no
run-time path read reaches them (D-12, PC-12). `PmuTableEntry`
(`source/counters/detail/pmu.hpp:35-58`) gains `fileName` with the same change,
so a published row can name its file (PC-12).

### E-13, E-14 The two host facts this feature adds

| Fact | Shape | Fixed by |
| --- | --- | --- |
| `#core_wide` | A new reader answers the coverage flag from the process CPU set at the first table read: 1 when the set holds every SMT sibling, 0 when one stands outside, and nothing when the mask or a sibling list is unreadable. The value settles once, because a conditional folds at load, so the console states the scope. It answers no slot width; the width is a literal of the table | FR-042, D-6, PC-5 |
| `#SMT_on` | `readSmtState` (`source/counters/linux_pmu/metric_tables.hpp:144-153`) keeps the object tree first, and one sibling-list read of one cpu answers the flag when the tree answers no core object | FR-054, D-19, PC-6 |
| `coreWideCoverage()` | The query that publishes the settled flag, in `include/speedgun-ng/counters_metrics.hpp` beside `metricByName` (`:81`) and defined in `source/counters/linux_pmu/metric_formula.cpp` over the new host-fact row. It answers `std::optional<CoreCoverage>`, with `CoreCoverage` a closed `enum class : std::uint8_t` holding `WHOLE_CORE` and `ONE_THREAD`. It answers nothing before the first table read and one value for the life of the process afterwards, and it states no width | FR-042, D-6, PC-5 |

`coreWideCoverage()` is the only way a caller reads the settled flag: no
private reader stands beside it, and `source/harness/cli.cpp` prints its reading
as the one scope line ahead of the first run (FR-042). The sibling list this
host publishes for cpu 0 is `0,12`, which is the fact the fallback reads. The three constants `#num_dies`, `#SYSTEM_TSC_FREQ` and
`#has_pmem` keep their present refusal (FR-043).

## Harness model

### E-15 `RunOptions.topdownLevel` (changed record)

| Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- |
| `topdownLevel` | `std::optional<std::uint64_t>` | An engaged optional is the option stood; the value is the level asked | FR-019, D-5 |

`RunOptions` is harness-internal (`source/harness/detail/internal.hpp:606`),
so this member moves no exported layout. No bool flag stands beside it.

### E-16 `TopdownRecord` and its two sub-records (exported)

New in `include/speedgun-ng/benchmark.hpp` beside
`std::optional<hunt::EntryHunt> hunt;` (`:747`), held as
`std::optional<TopdownRecord> topdown` in `BenchmarkResult` (`:651`).

| Record | Field | Type | Meaning | Fixed by |
| --- | --- | --- | --- | --- |
| `TopdownNode` | `name` / `vendorName` / `parent` | `std::string` | The three names of E-06; `parent` is empty at level 1 | FR-032 |
| `TopdownNode` | `level` | `std::uint64_t` | The node's level | FR-014, FR-032 |
| `TopdownNode` | `value` | `std::optional<double>` | The node's percent; empty for a gap | FR-030, FR-032 |
| `TopdownNode` | `availability` | `sg::counters::Availability` | The availability the merge published, `GAP` among them (`include/speedgun-ng/counters_core.hpp:87-94`) | FR-030 |
| `TopdownNode` | `threshold` | `std::string` | Table text, no verdict | FR-017, FR-029 |
| `TopdownRefusal` | `name` / `refusal` / `reason` | `std::string` × 3 | A refused row with its kind word and its reason | FR-016, FR-033 |
| `TopdownRecord` | `supported` / `reason` | `bool`, `std::string` | Unsupported state with its one reason | FR-033, D-14 |
| `TopdownRecord` | `reference` / `directory` | `std::string` × 2 | The manifest reference and the selected directory | FR-015, FR-032 |
| `TopdownRecord.files` | `std::vector<TableFile>` | One entry per contributing file, E-02's `name` and `manifestHash`, in the embedder's sorted order. The report spells the pair `name` and `manifest_hash` (`contracts/topdown-report-key.md:46-49`) | FR-015, FR-032, D-12 |
| `TopdownRecord` | `askedLevel` / `level` | `std::uint64_t` × 2 | The level asked and the level shown, so the clamp is visible | FR-020 |
| `TopdownRecord` | `nodes` / `refused` | vectors | The tree and its refusals | FR-032, FR-033 |

An absent `topdown` member means the option never stood, the way `hunt` works
today (FR-035). No vendor flag joins the record: `TopdownTree.amd` (E-08) is the
library's own choice of the fifth level-1 name and of the 100 percent statement
the block prints (FR-011, FR-028), and the closed report object carries no key
for it (`contracts/topdown-report-key.md`).

### E-17 The tree column (changed internal carrier)

`MetricColumn` (`source/harness/detail/internal.hpp:1343-1354`) stays as it is
and carries a node: the label is the node's vendor name, the spine is the
node's formula, and `foldColumn` is the run's fold position. The columns are
appended after the `--metric` loop (`source/harness/runner.cpp:1315-1326`) and
read by the standing leaf grouping and limit read
(`source/harness/runner.cpp:1348-1364`), so one plan covers both sets (FR-021,
FR-023). `stripTopdownColumns` moves each node's figure into
`result.topdown` and drops that position from every metric view of the result
(FR-023, FR-032). `printPassDisclosure` returns on empty `metricLabels`
(`source/harness/report.cpp:301-307`), so that guard gains the record test and
a `--topdown` run with no `--metric` still states its passes (FR-021).

### E-18 The report object (schema and reader)

The writer adds `writeEntryTopdown` beside `writeEntryHunt`
(`source/harness/json_report.cpp:509`) and writes the key after `hunt` at
`:715-718`. The reader adds `readEntryTopdown` beside `readEntryHunt`
(`source/harness/json_load.cpp:1799`) where `optionalField(value, "hunt")`
stands (`:2342-2349`). `docs/schema/report-v1.json` gains `"topdown"` in the
entry `x-order` after `"hunt"` (`:406`) and one property beside the entry
`hunt` property (`:827`); `required` (`:408-427`) and
`additionalProperties: false` (`:429`) do not move, so the key stays optional.
`kSchemaVersion` moves 1.11 to 1.12 (`source/harness/json_report.cpp:43`,
D-17). The schema types the version as a string and holds no number
(`docs/schema/report-v1.json:11-13`).

## Entities this feature only reads

| Entity | Owner | One line |
| --- | --- | --- |
| Metric row `MetricListing` | specs/022 | `include/speedgun-ng/counters_metrics.hpp:91-104`; the selection reads group, unit, published, refusal and reason, and it reads the whole listing wherever a row prints (FR-016, PC-11) |
| Refusal kind `MetricRefusal` | specs/022 | `include/speedgun-ng/counters_metrics.hpp:33-44`; nine kinds, copied unchanged into E-07 (FR-016) |
| Availability | the counters public model | `include/speedgun-ng/counters_core.hpp:87-94`; `GAP` is the node's gap state (FR-030) |
| Host fact reader | specs/022 | `source/counters/linux_pmu/metric_tables.hpp:105-153`; counts come from the object tree, and `#core_wide` is the one constant this feature adds (D-6) |
| Pass plan `PassPlan` | specs/024 | `source/harness/detail/pass_plan.hpp:50-59`; the run hands one set to the planner and states the passes it produced (FR-021) |
| Counter limit fact | specs/024 | read through `Object::programmableCounters()` at `source/harness/runner.cpp:1364`; no limit code changes (FR-022) |
| Metric column `MetricColumn` | specs/022, specs/023 | `source/harness/detail/internal.hpp:1343-1354`; the carrier a node rides into the plan and the fold |
| Compare rule `CompareRule` | specs/026 | `include/speedgun-ng/benchmark.hpp:1062-1071`; gains no member, because `topdown:` joins the prefix table at `source/harness/compare.cpp:55-56` (FR-037) |
| Vendored table row `PmuTableEntry` | specs/022 | `source/counters/detail/pmu.hpp:35-58`; the two added members are named in E-12 and in the carrier table below (D-11, PC-12) |
| Identity reader `PmuIdent` | specs/022 | `source/counters/detail/pmu.hpp:62-72`; the four shared fields of E-01 come from it (FR-003) |
| Directory selection | specs/022 | `pmuSelectDirectory` at `source/counters/detail/pmu.hpp:99-100`; the mapping file stays the only selector (FR-001) |
| Option table | specs/028 | one row for `--topdown[=<level>]` at internal value 350, the block comment at `source/harness/detail/options.hpp:80-82` ends at 340 (D-5) |

## Relationships

```text
CpuIdentity E-01 --maps to--> PmuIdent --directory--> TopdownTable E-05
TopdownTable E-05 --group text--> level token + parent token
                 --one row--> TopdownNode E-06  xor  TopdownRefusal E-07
TopdownNode E-06 *--1 TopdownNode (parent, an earlier element)
TopdownNode E-06 *--* LeafCore address --grouped by source--> PassPlan (024)
TopdownTree E-08 --1 TopdownSource E-03 --build time--> RECORD ref: + per-file sha256
TopdownTree E-08 --stripTopdownColumns--> TopdownRecord E-16 --writeEntryTopdown--> "topdown" key
"topdown" key --readEntryTopdown--> TopdownRecord E-16 --value--> CompareRule (026)
```

## Validation rules

| Rule | Refusal | Stated by |
| --- | --- | --- |
| `topdownTree` takes a level outside 1 to 6 | `SG_REQUIRE` fires in the source; no value reaches the tree | FR-019, D-4 |
| `midr` stands | `TopdownUnsupported` with `ARM`; the line names the `MIDR` and states that no Arm table is vendored | FR-005, D-7 |
| Vendor is neither `GenuineIntel` nor `AuthenticAMD` | `UNKNOWN_VENDOR` with the vendor text | FR-004 |
| Mapping file matched no row | `NO_DIRECTORY` with vendor, family and model | FR-001, D-1 |
| Selected directory is a hybrid directory, or `coreType` stands | `HYBRID` with the sentence that this feature makes no claim for a hybrid part | FR-006, D-13 |
| Selected table holds no level token in either spelling | `NO_TOPDOWN_ROWS` with the directory it read, for example `arch/x86/amdzen3/` | FR-007 |
| A row carries no level token | It is no node and no refusal; `total_dispatch_slots` of both AMD tables is that row | FR-014, PC-4 |
| A `_group` token's stem names no row of the directory | It states no parent; measured: 102 such tokens over the 102 level rows, and each of the 98 deeper rows carries one token that does name a row | FR-013 |
| A row names a parent that is itself refused | It refuses with the same reason and the report lists it | edge case, FR-016 |
| A row prints under an object other than `machine/` | It stays in the read: 33 of the 102 print under `cpu/` on this host | FR-016, PC-11 |
| Asked level is deeper than the table | The tree clamps to the table's own `depth`, and the block adds `topdown: the table holds no level deeper than N` | FR-008, FR-020 |
| Threshold text reaches the feature | It rides as text into the tree, the block and the record; `parseFormula` (`source/counters/linux_pmu/metric_formula.hpp:102-103`) refuses all of it, so no part evaluates a verdict | FR-017, D-9 |
| A collected node reads a gap | The node prints `not available` with no number, and the record keeps `value` empty with its availability | FR-030 |
| `--topdown` with no attached value | Level 1, and the run prints a tree at status 0 | FR-019, SC-05 |
| `--topdown=0`, `=7`, `=x` | One quoted line and status 2: `--topdown: expected 1 to 6, got '0'`, in the shape of `source/harness/options.cpp:227` | FR-050 |
| `--topdown` in an option file | `the option 'topdown' cannot stand in an option file` from the default arm at `source/harness/options.cpp:1517-1528` | D-5 |
| `--topdown 2` with a space | The level is not taken, because `getopt_long` reads an optional value attached only, the rule `--hunt` records at `source/harness/options.cpp:694-697` | D-5 |
| A rule writes `topdown:<node>@mean` | `this value carries one value and takes no aggregate` from `source/harness/compare.cpp:431-435`, a refusal this feature adds: `:400` gives every prefix but `stat:` an aggregate today, and FR-037 compares one node value, so `contracts/topdown-rule.md:46` excludes `topdown:` | FR-037 |
| A rule names a node no report carries, or runs against a report with no top-down | One quoted line and status 2 from `missingValueFault` (`source/harness/compare.cpp:1416-1424`): `the rule '<text>' finds no topdown:backend_bound for <name> in the contender report` | FR-038, FR-039, D-14 |
| A `topdown` key stands in a document with a missing or unknown inner key | The loader refuses the document against the closed key set, the shape `readEntryHunt` uses (`source/harness/json_load.cpp:1799`) | FR-034 |
| A node in a written record carries no parent below level 1 | The loader refuses it with the existing malformed-document shape; an absent object and an empty object both read | edge case, FR-034 |
| A run writes without the option | The document equals the same run before this feature, and the schema version is the only difference | FR-035, SC-06 |
| A page index carries the fixed-counter mark | The bound masks bit 30 off before the test, and a number outside the range the instruction accepts still answers "not allowed" (`source/counters/linux_pmu/fast_read.cpp:48-51`) | FR-053, D-18, PC-7 |
| A constant no top-down row reaches | `#num_dies`, `#SYSTEM_TSC_FREQ` and `#has_pmem` keep `no reader answers the constant …`; `#core_wide` answers from E-13 and `#SMT_on` from E-14 | FR-042, FR-043 |

## Invariants

| # | Invariant | Held by |
| --- | --- | --- |
| I-1 | Every node exists because a row states it, and every `parent` names an earlier element one level up | E-06 contract, FR-013 |
| I-2 | The answer is pure over identity plus rows, so two calls answer one tree | FR-002, FR-009 |
| I-3 | A published row carries a non-empty `formula` and an empty `reason`; a refused row carries the pair the other way | E-04 contract, FR-016 |
| I-4 | Nodes and refusals are disjoint, and together they cover every level row at or above `depth` | E-08, FR-016 |
| I-5 | No time column, no metric column and no counter column of the run changes, because the strip runs after the merge and outside every window | FR-023, FR-024 |
| I-6 | A run without the option writes no `topdown` key at all | FR-035, E-16 |
| I-7 | No new refusal kind joins `MetricRefusal`, and the loader drops no row | D-20, PC-11 |

## States of one run

```text
option absent -------------------------------> no selection, no record, no block
option present -> selection
                   unsupported -> one message, record with supported=false, run's own exit status
                   supported  -> clamp (asked -> depth) -> nodes + refusals
                 -> columns appended -> one plan -> passes (outside every window)
                 -> fold -> merge -> strip (figures move to result.topdown)
                 -> block print (level, nodes, gaps, refusals, source)
                 -> report write -> report read -> rule compare
```

`depth` never moves after the clamp, a refused row never enters the collected
set, and a gap keeps its state to the record: no state revisits an earlier one
(FR-016, FR-020, FR-024, FR-030). An unsupported run changes no exit status
(FR-041).

## Carrier changes that move a SOVERSION

| Carrier | Change | Exported | Effect |
| --- | --- | --- | --- |
| `include/speedgun-ng/counters_topdown.hpp` | new public header with E-01 to E-11 | yes | archive `SOVERSION` 4 to 5 (`CMakeLists.txt:110`, D-4) |
| `MetricListing` (`include/speedgun-ng/counters_metrics.hpp:91-104`) | gains `threshold` | yes | same archive rise, one exported layout (D-4, D-11) |
| `MetricView` (`include/speedgun-ng/counters_metrics.hpp:63-68`) | gains `threshold` | yes | same archive rise (D-11) |
| `BenchmarkResult` (`include/speedgun-ng/benchmark.hpp:651`) | gains `std::optional<TopdownRecord> topdown` | yes | harness `SOVERSION` 19 to 20 (`CMakeLists.txt:375`, the specs/037 shape at `:369-374`, D-4) |
| `TopdownRecord`, `TopdownNode`, `TopdownRefusal` | new exported records in that header | yes | the same harness rise, because they sit in the exported layout (D-4) |
| `PmuTableEntry`, `ParsedMetricRow`, `MetricDefinition`, `EmbeddedFile`, `EmbeddedDir` | gain `threshold`, `fileName`, `sha256`, `reference` | no | internal carriers; no SOVERSION move (D-11, D-12, PC-12) |
| `RunOptions`, `MetricColumn`, `OptionRow`, `CompareRule`, `PassPlan` | unchanged shape | no | the option joins the table and the prefix list, so no layout moves (D-5, FR-037) |

Two libraries, two independent numbers, each with the comment that names the
record and the requirement (FR-047).
