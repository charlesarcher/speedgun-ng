# Contract: the area mapper, its three inputs and its three answers

**Feature**: specs/046-hunter-topdown | **Files**: `source/harness/hunt_topdown.cpp`
with `source/harness/detail/hunt_topdown.hpp` | **Audit point**: `b1dd7670`
**Spec**: FR-001 to FR-009, FR-014, FR-029, D-1, D-3, D-4, D-7, D-10, D-16, D-17

## What this contract covers

One pure function maps one catalog address to one area. One stamp settles that
answer for every ranked record of a run. This contract fixes the inputs, the
three kinds of answer, the arms in order, the tags, and the unit that proves
each arm. The console cell stands in `console-cell.md`, the document key in
`document-key.md`, and the grouping accessor in `counters-grouping.md`.

## The pair and its gate shape

The pair stands beside the specs/040 stamp pair `source/harness/hunt_ranks.cpp`
with `source/harness/detail/hunt_ranks.hpp`. It keeps that pair's shape: the
namespace is `sg::detail`, one header carries tagged declarations, and one
source carries the bodies. The harness target globs `source/harness/*.cpp` with
`CONFIGURE_DEPENDS` (`CMakeLists.txt:279-284`), so the new source file needs no
`CMakeLists.txt` line.

The doc gate reads the public headers alone. Its scope is the public functions
and classes of the Doxygen XML it is handed
(`tools/dbc/dbc_doc_gate.py:9-12`). A nested `detail` namespace stands outside
that scope. The pair gate pairs the interfaces of that matrix
(`tools/dbc/dbc_pair_gate.py:789-800`). A header under `source/harness/detail/`
therefore joins no interface row. `source/harness/runner.cpp:694-698` states the
same reading for `stripTopdownColumns`: a declaration outside the read headers
keeps the pair gate from counting its tags or its checks. The two interfaces this
feature adds stand in `counters-grouping.md`.

## The three inputs

| Input | Type | Where it comes from |
|---|---|---|
| the catalog address of one ranked event | `std::string_view` | `RankedEvent::address` (`include/speedgun-ng/hunt.hpp:456`) |
| the tree answer of the run | `std::optional<sg::counters::TopdownTree>` | `topdownTree` (`include/speedgun-ng/counters_topdown.hpp:436`) at the deepest ask, level 6 |
| the vendor grouping | `const std::vector<sg::counters::TopdownEventGroup>&` | the accessor D-16 names, `counters-grouping.md` |

The level of the ask is a constant of the pair. The builder hands it to
`TopdownSelection::level` (`include/speedgun-ng/counters_topdown.hpp:323`), and
the library enforces the range 1 to 6 with `SG_REQUIRE`
(`source/counters/linux_pmu/topdown_select.cpp:411-414`). A table answers its
own deepest level when the ask runs deeper
(`include/speedgun-ng/counters_topdown.hpp:407-409`), so a level-6 ask is safe
for a shallow table. D-17 holds the two asks of one run apart (FR-033). The
area reads level 6 and the companion block reads level 1, behind one identity,
one selection and one table read. AP-9 measures the split on the acceptance
host: a level-1 tree settles 7 of its 497 printed addresses and a level-6 tree
settles 58.

## The function

```cpp
/// @brief specs/046 FR-001 to FR-007, D-4, D-10: the area of one catalog
/// address over the three inputs of the run.
///
/// \pre none
/// \post the answer stands never empty; with no tree it reads `unmapped` for
///       every vendor and every refusal kind (FR-005); with a tree it reads
///       the path of the deepest node whose `events` holds `address` as an
///       equal text (FR-002, FR-003, FR-007), else the group of the leaf part
///       of `address` (FR-004), else `unmapped`; two calls with the same three
///       inputs answer one text (FR-006); the call reads no counter, opens no
///       event, reads no clock and asks no kernel interface (FR-001, D-7)
/// \invariant the answer reads its three arguments alone
[[nodiscard]] auto topdownArea(
    std::string_view address,
    const std::optional<sg::counters::TopdownTree>& tree,
    const std::vector<sg::counters::TopdownEventGroup>& groups)
    -> std::string;
```

Two helpers stand beside it in the same pair, each with tags of its own.
`topdownDeepestNode` answers the index of the deepest node whose `events` holds
the address. On a tie it answers the lowest such index, and on no match it
answers nothing. `topdownPathOf` answers the names the `parent` links reach
above one node, root first.

## The tags in the words the macros carry

The registry names each macro its kind and its enforcement function
(`tools/dbc/macros.yaml:7-22`). The body carries one line per claim a caller
can rely on.

| Macro | Kind the registry states | Claim it guards |
|---|---|---|
| `SG_ENSURE` | `postcondition` | the answer of `topdownArea` stands never empty, so arm 4 below is the whole of the closing cases |
| `SG_ASSERT` | `assertion` | an index `topdownDeepestNode` answers names an element of `tree.nodes`, and its level stands at or above the level of the index it replaced |
| `SG_ENSURE` | `postcondition` | the text `topdownPathOf` answers starts with the name of the node it was given walked up to level 1, joined by `" > "` |
| `SG_REQUIRE` | `precondition` | none in this pair: the range of the level belongs to `topdownTree`, and `topdown_select.cpp:411-414` enforces it |

## The arms, in order

| # | Guard | Answer | Rule |
|---|---|---|---|
| 1 | no tree stands | `unmapped` | FR-005 |
| 2 | a node holds the address | the path of the deepest such node | FR-001, FR-002, FR-003, FR-007 |
| 3 | the grouping holds the bare name | that group, verbatim | FR-004, D-3 |
| 4 | otherwise | `unmapped` | D-3, Key Entities |

Arm 1 answers before the grouping is read, so a host the tree refuses reads no
listing at all (FR-005, D-16). Arm 3 drops the device prefix first, the part
before the first `/`, because the listing carries bare names (D-16, FR-004).
`leafPartOf` of `source/harness/detail/hunt_selection.hpp:99-104` cuts after the
last `/`. The two agree while a device path holds one name, the shape
`source/counters/linux_pmu/provider.cpp:338` builds, and that post already covers
an address with no separator. Arm 2 keys on the whole address, because a node
event is an address of the same shape.

## The two spellings of one address

AP-4 measured one address reaching the ranked table twice, once as
`cpu/branch-instructions` and once as `cpu/BR_INST_RETIRED.ALL_BRANCHES`. Only
the spelling a formula carries reaches a node. The other spelling answers the
group of the file that holds its row. Where no row of the selected directory
holds it, the answer reads `unmapped` (FR-002). An uncore address follows the
same rule. The bare name keys the group, and the group is the stem of its own
`uncore-*.json` file.

## The refusal arms

`TopdownUnsupport` holds five kinds: `UNKNOWN_VENDOR`, `NO_DIRECTORY`,
`HYBRID`, `ARM` and `NO_TOPDOWN_ROWS`
(`include/speedgun-ng/counters_topdown.hpp:329-336`). Every one of them answers
no tree, so arm 1 settles every address of such a run to `unmapped` (FR-005,
D-5). The mapper reads no refusal value: `TopdownUnsupported::message()`
(`include/speedgun-ng/counters_topdown.hpp:377`) owns the one line a caller
prints, and `console-cell.md` fixes where that line stands.

## The stamp

```cpp
/// @brief specs/046 FR-009: the area of every ranked record of every hunted
/// entry of one run.
///
/// \pre `results` holds one record per benchmark entry of one run and the hunt
///      records stand ranked
/// \post every record of `entries` of every hunt record carries the answer
///       `topdownArea` gives for its address, the ranked records and the
///       unranked ones alike; no other field of any record moves, no record is
///       added, removed or reordered, and `skipped` gains nothing (FR-010)
/// \invariant the areas read the records' addresses and the three inputs alone
auto stampTopdownAreas(std::vector<sg::BenchmarkResult>& results,
                       std::optional<sg::counters::TopdownTree> const& tree,
                       std::vector<sg::counters::TopdownEventGroup> const& groups)
    -> void;
```

`entries` covers every planned column of a run, the ranked ones first
(`include/speedgun-ng/hunt.hpp:474-489`), so one loop over it meets FR-009. The
call joins the batch stamping pass after the last rank stamp at
`source/harness/cli.cpp:1983` and before the records move back at
`source/harness/cli.cpp:1985-1987`.

## What a test must pin

Every case stands in `test/source/harness_hunt_topdown_test.cpp`, registered
beside the specs/042 block at `test/CMakeLists.txt:2249-2258`. The file plants
its tree and its listing by hand, the way
`test/source/harness_topdown_report_test.cpp:257` plants a record. It registers
no provider and reads no device (FR-030).

| Fact | Scenario |
|---|---|
| the deepest node of the planted tree wins, and the path carries every name above it | `area-takes-the-deepest-node` (FR-001, FR-007) |
| a second spelling of one address names no node | `second-spelling-names-no-node` (FR-002, AP-4) |
| two nodes of one depth answer the one that stands first | `tie-goes-to-the-first-row-in-table-order` (FR-003) |
| a name no node holds answers the stem of its own file | `area-falls-back-to-the-vendor-file-stem` (FR-004) |
| a level-1 node answers a path of one name | `level-one-node-answers-one-name` (FR-007) |
| the five refusal kinds, and an identity with no directory, answer `unmapped` | `arm-midr-answers-unmapped`, `unknown-vendor-answers-unmapped`, `no-directory-answers-unmapped`, `no-topdown-rows-answers-unmapped`, and `two-scope-rows-answer-unmapped` for the
hybrid kind, which no row of its directory reaches (FR-005) |
| two calls with one input pair answer one text | `two-calls-answer-one-area` (FR-006) |
| a repeated name arrives once, owned by the row that came first | `counters-grouping.md`, scenario `one-name-answers-one-pair`, because the accessor owns the first row (D-16) |
| the stamp reaches the unranked records and leaves an unhunted result alone | `stamp-sets-every-entry-including-unranked`, `stamp-leaves-an-unhunted-result-alone` (FR-009) |
| the ask of the run carries level 6 and the planted seams | `planted-run-asks-the-area-at-level-six`, `unplanted-run-asks-the-area-at-level-six` (FR-001, FR-033) |
| one call opens no event, reads no clock and reaches no source of its own | the `/proc/self/fd` and `/proc/self/maps` shape of `test/source/counters_topdown_select_test.cpp:200-231`, the case `oneCallReadsNoSourceOfItsOwn` at `:1243` (FR-001) |

Six synthetic identities carry the vendor arms, each from a helper the base
already holds. Skylake-X, Zen 4 and Zen 5 come from `intelIdentity` and
`amdIdentity`. An unsupported AMD part comes from `amdIdentity(25, 15)`. An Arm
`MIDR` comes from `kHandedMidr` at
`test/source/counters_topdown_select_test.cpp:142`. An unknown vendor comes
from `unmatchedIdentity` at the same file, `:150-154`. A path case needs a
planted `TopdownTable` beside the identity, because an identity alone answers
zero nodes on a host of another vendor (FR-029).

## Verification

| Claim | Machine |
|---|---|
| every arm above reaches a case with no host and no kernel interface | `ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure` |
| the counters change stays the one accessor | the diff of `source/counters` names the two files D-16 lists, beside `counters-grouping.md` |
| no rank, budget or emit byte moves with an area set | `console-cell.md` and `document-key.md`, each with its own machine |
| the mapper adds no dark branch | `tools/dbc/coverage_gate.sh` with the shortfall at or below 21 (`tools/dbc/coverage_gate.sh:102`) |
