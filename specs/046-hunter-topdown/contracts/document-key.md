# Contract: the `topdownArea` key of the JSON document

**Feature**: specs/046-hunter-topdown | **Files**: `include/speedgun-ng/hunt.hpp`,
`source/harness/json_report.cpp`, `source/harness/json_load.cpp`,
`docs/schema/report-v1.json` | **Audit point**: `b1dd7670`
**Spec**: FR-007, FR-019 to FR-023, FR-028, FR-033, D-11, D-12

## What this contract covers

One key joins the per-event node of a hunt document. This contract fixes its
name and its place. It fixes the rule for a member that holds nothing, and the
schema change. It fixes the rule for a document an older build wrote, and the
rebase point with the specs/047 lane. `contracts/area-mapper.md` fills the member
and `contracts/console-cell.md` reads it on the console.

## The member

`RankedEvent` gains one member after `delta`
(`include/speedgun-ng/hunt.hpp:471`). It is a plain `std::string`, the kind
`label` carries on a row
(`include/speedgun-ng/benchmark.hpp:419`, written at
`source/harness/json_report.cpp:467`). It defaults to empty, so a record no run
stamped holds nothing. `RankedEvent` carries no `\pre` or `\post` section today
(`include/speedgun-ng/hunt.hpp:442-453`), so a member adds no doc-gate row of its
own.

## The written key

`writeEntryHunt` writes the per-event record
(`source/harness/json_report.cpp:587-606`) and `delta` stands last in it at
`source/harness/json_report.cpp:604`. FR-019 states the rule: the writer writes
the key only where the member holds a value, the rule the console keeps. One
conditional write follows `delta`, inside the same block, through the plain
string form of `source/harness/json_report.cpp:593`:

```cpp
if (!ranked.topdownArea.empty()) {
  put(builder, entry, "topdownArea", ranked.topdownArea);
}
```

Round 3 states the same reading: a conditional write after `delta` is the shape
FR-019 states. Two texts move with that line.

| Text | Move | Anchor |
|---|---|---|
| the node comment | it asserts that no key of the node is ever absent, and FR-019 moves it | `source/harness/json_report.cpp:569-572` |
| the schema node | `x-order` and `properties` gain the key, `required` keeps it out | `docs/schema/report-v1.json:948-978` |

`putOrNull` carries no part in this write. That helper gives the `null` spelling
to a text a run may leave unset (`source/harness/json_report.cpp:225-238`), and
FR-019 gives that spelling no writer here. The console states the same absence by
the missing cell (`contracts/console-cell.md`, FR-014).

## The schema node

The per-event object is closed at `docs/schema/report-v1.json:946-979`.

| Field | Change | Anchor |
|---|---|---|
| `x-order` | add `"topdownArea"` after `"delta"`, last | `:948-958`, `:957` |
| `required` | no change, the key stays out | `:959-966` |
| `properties` | add `"topdownArea": { "type": "string" }` after `"delta"` | `:968-978`, `:977` |
| `additionalProperties` | no change, `false` stands | `:967` |

`checkNode` reads `required` for the keys it lists alone
(`source/harness/json_load.cpp:2968-2972`) and walks `x-order` over the keys a
document holds (`source/harness/json_load.cpp:2973-3000`). A document with this
key away therefore passes, and a key the closed node does not list still refuses
(`source/harness/json_load.cpp:2976-2984`). The node stays closed, so a stale key
in a planted document still answers one refusal.

## The version bytes

| Byte | From | To | Anchor |
|---|---|---|---|
| the stamped minor | `1.13` | `1.14` | `source/harness/json_report.cpp:47` |
| the history clause | the 1.13 clause | one clause naming the new key, the per-event node and specs/046 | `source/harness/json_report.cpp:31-46`, shape at `:41-46` |
| the guide page | "A run writes `1.13`" | the same sentence with the new minor and the new key named | `docs/pages/harness.md:1397-1403` |

The schema file carries no version value to move. `schemaVersion` there is
declared as a plain string (`docs/schema/report-v1.json:11-13`) and `$id` names
the major alone (`docs/schema/report-v1.json:3`). A key added inside major 1
moves the minor, the rule the guide page states at
`docs/pages/harness.md:1399-1400`. FR-023 moves every pinned assertion of the current
minor, and every comment that names it, in the same commit as the writer. It
moves the two later-minor plants one minor above the new current. The version
numbers of `contracts/counters-grouping.md` ride the ABI commit, and this minor
rides the writer's commit (FR-020, FR-028).

## The reader

`readRankedEvent` owns the per-event read
(`source/harness/json_load.cpp:1884-1964`). The key rides `optionalField`
(`source/harness/json_load.cpp:86-94`) after the rank-figure block
(`source/harness/json_load.cpp:1941-1962`), and the required-key vector keeps its
six names (`source/harness/json_load.cpp:1890-1898`). FR-021 states the three
arms, and it is the rule that file already carries for a key a later minor added
(`source/harness/json_load.cpp:80-85`).

```cpp
if (const auto area = optionalField(value, "topdownArea"); area.has_value()) {
  auto text = asString(*area, where + "/topdownArea");
  if (!text.has_value()) {
    return std::unexpected(text.error());
  }
  event.topdownArea = *text;
}
```

| State of the key | Answer | Machine of that arm |
|---|---|---|
| absent | the member stays empty, and the document loads | `optionalField` answers no value (`source/harness/json_load.cpp:86-94`) |
| a string | the member holds it | `asString` keeps a string and refuses the rest (`source/harness/json_load.cpp:135-144`) |
| a `null`, a number, a bool or an array | one refusal at the pointer `.../entries/<i>/topdownArea` | FR-021: a value of another kind keeps its own refusal |

The fold of `readFigure` takes a `null` as nothing
(`source/harness/json_load.cpp:1945-1948`). This key carries no such arm:
FR-021 names three arms, and a `null` is no string. The designated initializer at
`source/harness/json_load.cpp:1912-1924` needs no new line, because the member
defaults to empty. The comment above the rank figures at
`source/harness/json_load.cpp:1935-1940` gains one sentence that names the new
key.

## A document an older build wrote

FR-021 asks a document of an older minor to load unchanged and to leave the area
empty. Three standing facts give that answer. The version rule reads the major
alone (`docs/pages/harness.md:1397-1399`). The reader answers an absent optional
key with nothing (`source/harness/json_load.cpp:86-94`). The schema keeps the key
out of `required`. The case already has its machine in
`aDocumentPredatingThePassFieldsLoads`
(`test/source/harness_json_load_test.cpp:746-760`), which strips each added key
alone and loads. The new case strips `"topdownArea"` from a written document and
loads it the same way.

## The specs/047 rebase point

The two lanes both reach `source/harness/json_load.cpp`, so this lane names its
surface there exactly:

- 046 edits `readRankedEvent` alone. It adds no include, no file-scope helper,
  and no change to `optionalField` (`source/harness/json_load.cpp:86-94`),
  `field` (`:66-78`), `settled` (`:98-108`) or `checkNode` (`:2935-3027`).
- The required-key vector at `source/harness/json_load.cpp:1890-1898` stays
  untouched, so no shared line moves.
- In `test/source/harness_json_load_test.cpp` the work of this lane stands in
  `theHuntRankFieldsLoadBackEqual` (`test/source/harness_json_load_test.cpp:313-388`)
  and in the `Mutation` plant table (`:829-860`). The two later-minor plants at
  `:108` and `:1200` move to `1.15` under FR-023. Those three functions are the
  surface of this lane in that file.
- The two lanes cannot collide unless both edit one of those functions. Whoever
  merges second rebases, keeps both sides, and reruns every gate.

## Verification

| Fact | Machine |
|---|---|
| the key stands after `delta`, last among the keys of the per-event node | `area-key-stands-after-delta`, joining `test/source/harness_hunt_json_test.cpp:2056`, beside `huntKeysInOrder` at `:938-949` (FR-019) |
| a member that holds nothing writes no key, and the node keeps its other bytes | the same scenario, over the record that stamps nothing (FR-019) |
| the schema pairs the written node at the stamped minor 1.14 | `entryHuntKeysPairAtStampedMinor` (`test/source/harness_json_schema_test.cpp:204-236`), stamp check at `:232-235` (FR-020) |
| a present key reads back equal, an absent key unsets, another kind refuses | `theAreaKeyReadsBackEqual`, `aDocumentWithoutTheAreaKeyLoads`, `anAreaOfAnotherKindRefuses`, joining `test/source/harness_json_load_test.cpp:2205-2236` (FR-021) |
| a wrong kind is refused at its pointer, and no minor comparison enters the loader | the third scenario above, beside a `Mutation` row at that pointer in the plant table (`test/source/harness_json_load_test.cpp:829-860`) |
| the two later-minor plants answer one minor above the new current | `test/source/harness_json_load_test.cpp:108`, `:1200` (FR-023) |
| the document of a run with no `--hunt` differs in the two version values alone | the three commands of the plan section 5.2, over `test/data/hunt_no_hunt_document.json` (FR-022, SC-003) |
