# Contract: the vendor grouping accessor of the counters library

**Feature**: specs/046-hunter-topdown | **Files**:
`include/speedgun-ng/counters_topdown.hpp` with
`source/counters/linux_pmu/topdown_select.cpp` | **Audit point**: `b1dd7670`
**Spec**: FR-004, FR-008, FR-028, D-13, D-16

## What this contract covers

One read-only accessor joins the counters surface, and it is the whole counters
change of this feature (FR-008). This contract fixes its types and its contract in
the words the DBC macros carry. It fixes the doc-gate form, the arms that answer
an empty listing, the order it answers in, and the duplicate-name case. It fixes
the archive consequence. `area-mapper.md` reads the answer.

## Why a harness reader cannot reach this today

`pmuLoadTable` and `PmuTableEntry` stand in the private counters seam
`source/counters/detail/pmu.hpp` (`:35-81`, `:127-128`), and no file under
`source/harness/` includes that header. The refused alternative is a second reader
of the same vendored json inside the harness. It would stand as a second source of
truth over data the library already parses (D-16, Scope).

## The record and the call

```cpp
/**
 * @brief specs/046 FR-004, FR-008, D-16: the vendor grouping of one row of the
 * selected directory: the bare name beside the file stem that row stands in.
 *
 * `group` is the vendor word: `cache`, `memory`, `pipeline`, `frontend` and the
 * rest of the stems of the selected directory, each spelled as the vendor spells
 * it. The vocabulary belongs to the directory, so Skylake-X answers `cache`
 * where Zen 5 answers `l2-cache` (D-3, AP-7).
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownEventGroup
{
  std::string event;  ///< The bare name one row carries, with no device prefix.
  std::string group;  ///< The file stem of that row, verbatim.
};

/// @brief specs/046 FR-004, FR-008, D-16: the grouping listing of one identity:
/// the pairs the directory that identity selects holds.
///
/// The call reads the embedded table of the selected directory, and that table is
/// build-time data the loader holds once per directory. No vendored path opens at
/// run time and no host probe is made (D-16).
///
/// \pre none
/// \post The answer holds one pair per name of the selected directory, in the
///       order that directory loads, and the first row that carries a repeated
///       name owns its pair (D-16).
/// \post `group` is `PmuTableEntry::fileName` with its suffix dropped, and the
///       call renames nothing (FR-004).
/// \post An identity that selects no directory answers an empty listing (D-16).
/// \post The call reads no counter, opens no event, reads no clock, opens no
///       path, and reads no `MIDR` field beyond the directory the identity
///       selects (D-13).
/// \invariant none
[[nodiscard]] SPEEDGUN_NG_EXPORT auto
topdownEventGroups(const CpuIdentity& identity)
    -> std::vector<TopdownEventGroup>;
```

D-16 sets the shape of that post. It names a fact about the listing the call
built, and names no fact about the contents of a vendor file. The listing answers
every row of the directory, event rows and metric rows alike.

## The doc-gate form

The doc gate reads the public headers and fails a public function whose block
lacks a dedicated `\pre` or `\post` section
(`tools/dbc/dbc_doc_gate.py:14-18`). An explicit `\pre none` counts as present at
that same rule. The pair gate scans the sources for the registry macros and pairs
each documented kind with an enforced kind (`tools/dbc/dbc_pair_gate.py:10-15`).
Both drift directions fail the gate: documented-not-enforced and
enforced-not-documented (`:17-20`).

Three consequences for this one interface:

- The documented `\post` lines need `SG_ENSURE` checks in the definition,
  because `SG_ENSURE` is the registry entry for the postcondition kind
  (`tools/dbc/macros.yaml:11-14`).
- `\pre none` asks for no `SG_REQUIRE` line. A `\invariant none` asks for
  nothing, because a function row pairs precondition, postcondition and assertion
  alone (`tools/dbc/dbc_pair_gate.py:25-29`).
- The header comment and its body land in one commit. A documented postcondition
  without its check is a gap on the day it is written.

The shape `cpuIdentity` already carries serves as the model: `\pre none` in the
header (`include/speedgun-ng/counters_topdown.hpp:80-84`) and one `SG_ENSURE` in
the body (`source/counters/linux_pmu/topdown_select.cpp:142-146`). The new
definition enforces one line, the pair rule of a repeated name, because that fact
follows from no other statement of the function. The empty listing of an identity
that selects no directory stands in the `\post` text and reaches a unit case,
because the guard that returns it already decides the fact, and a check that
repeats a guard states nothing.

Both DBC gates read 345 interfaces with 0 gaps at the base, run by hand with
`/usr/bin/python3` (AP-2). The doc gate reads `include/speedgun-ng`, so the head
adds two interfaces there: this function and its record. The acceptance report
states the pair of numbers its own run answers.

## The arms that answer an empty listing

| State of the identity | Listing | Rule |
|---|---|---|
| no mapping-file row names a directory | empty | `topdownTable` states the same arm for the rows of that identity (`include/speedgun-ng/counters_topdown.hpp:200-204`) |
| an unknown vendor, or an identity an `MIDR` names | empty | both land in the arm above, so every event of such a run reads `unmapped` (D-5, D-16) |

A tree refusal does not empty the listing. A directory can hold event rows and
still answer no top-down row, and the CI fleet is that case (D-14). The mapper
answers `unmapped` ahead of any listing read (FR-005), so the listing of a refused
host reaches no area.

## The strip, and how the harness reads the listing

The accessor answers in the order the loaded table holds its rows. `pmuLoadTable`
loads a directory lazily and once (`source/counters/detail/pmu.hpp:126-128`), so a
second call answers the same order with no second read. The embedder sorted the
file list (`cmake/embed_pmu_blob.cmake:91`) and the loader walks the directory in
that order (`include/speedgun-ng/counters_topdown.hpp:202-204`).

The listing carries bare names, so the harness drops the device prefix of a hunt
address before it reads the index (D-16, FR-004). The prefix is the part before
the first `/`.
`leafPartOf` of `source/harness/detail/hunt_selection.hpp:99-104` cuts after the
last `/`. The two agree while a device path holds one name, the shape
`source/counters/linux_pmu/provider.cpp:338` builds.

The accessor owns the first row of a repeated name, so the harness index carries
no dedup rule (D-16). The harness reads the listing once per run and holds no list
of event names of its own (FR-004).

## The duplicate-name case

The measurement stands over the 46 pinned `arch/x86` directories, which hold
46,837 event rows and 5,944 metric rows (D-16). No pinned directory holds one name
in two files. Five hybrid directories repeat a name inside one file under a second
`Unit` device (D-16). `arrowlake` answers 180 names over 231 surplus rows, and
`lunarlake` 64, `meteorlake` 62, `pantherlake` 61 and `alderlake` 47 answer one
surplus row each.
`arrowlake/DL1.DIRTY_EVICTION` stands under `cpu_atom` and `cpu_lowpower` as the
named case (D-16).

The rule rides one insert-if-absent line, documented at the site (D-16). Those
five directories are refused as hybrid before any area is settled (D-5), so a
pinned directory reaches that arm in the counters unit alone. That unit carries
the case, and the coverage bar of FR-031 reads it.

## The archive consequence

| Number | At the base | After this change | Line |
|---|---|---|---|
| project version | 0.30.0 | 0.31.0 | `CMakeLists.txt:59` |
| harness `SOVERSION` | 20 | 21 | `CMakeLists.txt:407` |
| archive `SOVERSION` | 5 | 6 | `CMakeLists.txt:134` |

An exported layout of the counters archive moves, so its number rises (D-8,
D-16). One commit carries all four moves (FR-028), and each line keeps the comment
that names the record and the requirement. The schema minor rides the writer's
commit instead, beside the code that writes it (FR-020, FR-023).

Two readers bind these numbers, and both move in that commit.
`theVersionBindingsReadTheMovedNumbers` reads the project version, the two numbers
and the pair at `test/source/harness_cli_table_test.cpp:1453-1461`. The block in
`test/source/views_test.cpp:279-287` reads the same four. A substring check alone
can be settled by a comment. The build file states `SOVERSION 5` on its property
line at `CMakeLists.txt:134` and again inside a comment at `:406`.
Both files therefore read the numbers through a helper that keeps only a line
whose first word is `SOVERSION ` (`test/source/harness_cli_table_test.cpp:1413`).
FR-028 names the failure that helper catches: a move of the archive number alone
makes the pair read `6 20` and fails both readers. The pair string moves from
`"5 20"` to `"6 21"`.

## Verification

| Fact | Machine |
|---|---|
| the listing carries the stems the vendor spells, for the identity of the host | `test/source/counters_topdown_groups_test.cpp`, `groups-carry-the-vendors-file-stems` and `accessor-reads-the-identitys-directory` (FR-004) |
| an identity that selects no directory answers an empty listing | `test/source/counters_topdown_groups_test.cpp`, `an-arm-identity-answers-no-pair` and `a-refused-directory-still-groups-its-rows` (D-16, ruling R2) |
| a name two rows carry answers one pair, owned by the first row | `one-name-answers-one-pair`, in the same unit (D-16, FR-031) |
| a build with no vendored directory skips instead of failing | the entry reserves the skip code 2 the way `counters_topdown_select_test` does (`test/CMakeLists.txt:560-563`), and `test/tier_labels.sh:80` holds the matching row |
| both DBC gates answer 0 gaps with the two new interfaces | the two hand commands of AP-2, with the number the run answers |
| the archive number is read from a property line | `ctest --test-dir build -R 'harness_cli_table_test\|views_test' --output-on-failure` (FR-028) |
| the counters change stays the one accessor | the diff of `source/counters` names `topdown_select.cpp` alone (FR-008) |
