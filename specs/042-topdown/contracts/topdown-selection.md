# Contract: the top-down selection call and its tree

**Feature**: specs/042-topdown | **Files**: `include/speedgun-ng/counters_topdown.hpp`
with `source/counters/linux_pmu/topdown_select.cpp` | **Audit point**: `65c48be9`
**Spec**: FR-001 to FR-017, FR-040, D-1, D-2, D-13, D-20

## What this contract covers

One call turns a CPU identity and a metric table into a top-down tree. This
contract fixes its types and the refusals it answers.

## The public calls

```cpp
struct SPEEDGUN_NG_EXPORT CpuIdentity {                    // FR-003, D-13
  std::string vendor;                 // CPUID leaf 0, 12 characters.
  int family = -1;                    // The mapfile spelling.
  int model = -1;                     // The mapfile spelling.
  int stepping = -1;                  // Negative names no stepping class.
  std::string coreType;               // Empty in production (D-13).
  std::optional<std::uint32_t> midr;  // Unset in production (D-13).
};
[[nodiscard]] SPEEDGUN_NG_EXPORT auto cpuIdentity() -> CpuIdentity;
[[nodiscard]] SPEEDGUN_NG_EXPORT auto topdownTable(const CpuIdentity&)
    -> TopdownTable;  // The vendored table of that identity (FR-001).
enum class TopdownUnsupport : std::uint8_t {               // FR-004 to FR-007
  UNKNOWN_VENDOR, NO_DIRECTORY, HYBRID, ARM, NO_TOPDOWN_ROWS,
};
struct SPEEDGUN_NG_EXPORT TopdownUnsupported {             // D-14, FR-040
  TopdownUnsupport kind = TopdownUnsupport::UNKNOWN_VENDOR;
  std::string vendor;
  std::string identity;  // "family 6 model 85 stepping 4", or the MIDR.
  std::string detail;    // The directory read, or the Arm sentence (D-7).
  [[nodiscard]] auto message() const -> std::string;  // One line, D-14.
};
[[nodiscard]] SPEEDGUN_NG_EXPORT auto
topdownTree(const TopdownSelection& options)
    -> std::expected<TopdownTree, TopdownUnsupported>;
```

`TopdownSelection` carries `identity`, `level` in 1 to 6, and an optional
`table` a test hands in. `TopdownTree` carries `source`, `depth`, `amd`,
`nodes` and `refused`; `TopdownNode` carries `name`, `vendorName`, `parent`,
`level`, `formula`, `events` and `threshold` (FR-010 to FR-012).

## Contract

| Name | Contract |
|---|---|
| `topdownTree` | `\pre` `options.level` stands in 1 to 6, enforced by `SG_REQUIRE`. `\post` the answer is pure over the identity and the rows: no counter read, no event open, no clock read, no CPUID read (FR-002, FR-009). `\post` `depth` is the asked level clamped to the deepest level the table holds (FR-008, FR-020). `\post` no node stands that a row does not state (FR-013). `\invariant` every parent names an earlier element, and a node's level is one above its parent's. |
| `topdownTable` | `\pre` none. `\post` an empty `source.directory` names an identity the mapping file matched no row for, and `rows` then stands empty. `\post` every row of the selected directory answers a verdict, published or refused, in the sorted file order the embedder emits (`cmake/embed_pmu_blob.cmake:41-42`). |
| `cpuIdentity` | `\pre` none. `\post` the first four fields equal `pmuIdentCurrent()` (`source/counters/detail/pmu.hpp:84`), `coreType` is empty and `midr` is unset: no reader for either exists in this feature (D-13). |
| `message()` | `\pre` none. `\post` one line naming the vendor, the identity and the reason (D-14, FR-040). |
| `TopdownNode::threshold` | `\pre` none. `\post` the table's own text byte for byte, empty where the row carries none. Nothing evaluates it (FR-017, D-9). |
| `coreWideCoverage` | `\pre` none. `\post` none, with its claims stated in the paragraph the header carries above the tags: nothing stands settled before the first call, one value then stands for the life of the process because a table resolves once per directory and the conditional over the constant folds when it resolves, a second call answers what the first answered, a settled value never moves inside one process, the value names a whole core with every sibling of it or one thread of a core and states no slot width, and an absent answer keeps the standing refusal of `#core_wide` (FR-042, FR-043, D-6). The tags read `none` because `tools/dbc/dbc_pair_gate.py` pairs a documented postcondition with a check of that kind and pairs a function row on precondition, postcondition and assertion only, so a tag here would ask for a check that repeats the value the call just settled. |

## Names, levels and parents

Level 1 carries `topdown.frontend_bound`, `topdown.bad_speculation`,
`topdown.backend_bound` and `topdown.retiring`, plus `topdown.smt_contention`
on an AMD table alone (FR-011, D-3); a deeper node carries
`topdown.<vendor>.<table name>` (FR-012). A level is a group token matching
`TopdownL1` to `TopdownL6` or `PipelineL1` or `PipelineL2`; a row with no level
token is no node (FR-014). A parent is one other group token ending in `_group`
whose stem names another row of the same directory (FR-013).

A `python3` walk over `MetricGroup` measures the fixtures.
`skylakex/skx-metrics.json` holds 275 rows and 102 level rows, split 4, 8, 25,
34, 17 and 14 over levels 1 to 6; each of the 98 deeper rows carries exactly
one parent token and the four level-1 rows carry none. `amdzen4/pipeline.json`
and `amdzen5/pipeline.json` hold 14 rows each: one group-free helper
(`total_dispatch_slots`, no node), five `PipelineL1` rows including
`smt_contention`, and eight `PipelineL2` rows with one parent each. All eight
level-2 names differ between the two files, so FR-052 needs both.

## The answers a caller can see

`topdownTree` answers one `TopdownUnsupported`; the harness prints `message()`
once on standard output, and no exit status changes (FR-041).

| Kind | Text |
|---|---|
| `UNKNOWN_VENDOR` | `topdown: <vendor> <identity>: no vendored table carries this vendor` |
| `NO_DIRECTORY` | `topdown: <vendor> <identity>: the mapping file names no table for this identity` |
| `HYBRID` | `topdown: <vendor> <identity>: the selected table '<directory>' publishes rows for two core scopes, and this feature makes no claim for a hybrid part` |
| `ARM` | `topdown: Arm <midr>: no Arm table is vendored` |
| `NO_TOPDOWN_ROWS` | `topdown: <vendor> <identity>: the selected table '<directory>' holds no row of any top-down group` |

A refused row carries the metric layer's reason unchanged. The texts a caller
can read are `no reader answers the constant '#core_wide' (FR-006)`
(`source/counters/linux_pmu/metric_formula.cpp:1310`), `this host publishes no
value for the constant '#SMT_on' (FR-006)` (`metric_formula.cpp:1315-1316`) and
`names no event the scope '<label>' reaches (FR-014)` (`metric_formula.cpp:1298`).
The kind prints in UPPER SNAKE from `metricRefusalName`
(`include/speedgun-ng/counters_metrics.hpp:51-52`, table at
`metric_formula.cpp:1630-1640`).

## Two rulings this contract settles

FR-006 keys the hybrid refusal on the scopes a directory's own event rows
publish, each read from its `Unit` text (`PmuTableEntry::unit`,
`source/counters/detail/pmu.hpp:45`), and a closed name list is the weaker
rule. A `python3` walk of the two shapes `parsePadded` reads
(`source/counters/linux_pmu/table_parse.cpp:528-573`) over the 46 directories
of `external/pmu-events/arch/x86` answers five with rows of both core classes,
with the `cpu_core` and `cpu_atom` counts behind each: `alderlake` 319 and 207,
`arrowlake` 329 and 295, `lunarlake` 331 and 307, `meteorlake` 323 and 252, and
`pantherlake` 347 and 256. Four of the five carry level rows too, 135, 139, 139
and 140, and the scope rule is what refuses those four. `pantherlake` is the
directory the mapping file gives models `CC` and `D5`
(`external/pmu-events/arch/x86/mapfile.csv:29`) and holds 606 event rows, no
metric row and no level row, so FR-007's `NO_TOPDOWN_ROWS` answers there on
either rule and a tree stands nowhere. The scope rule keeps its force for the
directories that do carry level rows: the next hybrid directory is a hybrid
directory at its first `cpu_atom` row, and a closed list names it one release
late. `grandridge`, `sierraforest` and `alderlaken` carry neither token: 324,
380 and 301 of their rows carry no `Unit` key, which is the core scope, and each
of the three holds 26 level rows, so all three select normally.

## What a test must pin

Every case stands in `test/source/counters_topdown_select_test.cpp` on the
seams at `source/counters/detail/pmu.hpp:99-100` and `:112`, pure (FR-048).

| Fact | Case |
|---|---|
| Model 85 stepping 4 selects `arch/x86/skylakex/`; family 25 model 17 selects `amdzen4` and family 26 model 17 selects `amdzen5`, because the mapping file matches a model pattern and takes the first row that hits (`mapfile.csv:44-47`) | `selectsTheTableOfTheIdentity` |
| Four Intel level-1 names with no parent; five AMD names with `smt_contention` | `levelOneCarriesTheSharedNames` (FR-011, SC-01, FR-052) |
| Both parent spellings resolve; a group-free row yields no node | `parentsComeFromTheGroupText` (FR-013, FR-014) |
| Each of the five texts above verbatim, with the identity in it | `unsupportedAnswersQuoteTheIdentity` (FR-004 to FR-007, FR-040) |
| Asking 6 of a level-1 table answers level 1, and `depth` says so | `levelDeeperThanTheTableClamps` (FR-008, FR-020) |
| A refused row stands in `refused` with kind and reason and in no `nodes` | `refusedRowsStayInTheDocument` (FR-016) |
| A level whose rows print under two objects loses no row | `rowsUnderTwoObjectsBothReach` (FR-016, D-20, PC-11) |
| Threshold text reaches the node unchanged; none means empty | `thresholdRidesAsTextUnchanged` (FR-017) |

## Shared file

`include/speedgun-ng/counters_metrics.hpp` gains the threshold on `MetricView`
and `MetricListing` (`:63-68`, `:91-104`), so the archive `SOVERSION` moves 4
to 5 (`CMakeLists.txt:110`, chain at `:102-109`). The same header publishes the
query for the settled `#core_wide` flag beside `metricByName` (`:81`):
`coreWideCoverage()` answers `std::optional<CoreCoverage>`, carrying the same
`SPEEDGUN_NG_EXPORT` mark as the rows above it. `CoreCoverage` is a closed
`enum class : std::uint8_t` with `WHOLE_CORE` and `ONE_THREAD`, the shape
`Availability` uses (`include/speedgun-ng/counters_core.hpp:87-94`), and
`source/counters/linux_pmu/metric_formula.cpp` defines the query over the
host-fact row the load-time reader fills. It answers nothing ahead of the first
table read and one value afterwards, and `source/harness/cli.cpp` prints that
reading as the one scope line ahead of the first run (FR-042, D-6). The new header joins
`NAME_SCAN_HEADERS` (`tools/docs/example_drift.py:530-538`) or its names cannot
carry quick-reference rows. `include/speedgun-ng/counters.hpp:14-21` stays as
it is: it lists eight headers and already omits the metric header.
