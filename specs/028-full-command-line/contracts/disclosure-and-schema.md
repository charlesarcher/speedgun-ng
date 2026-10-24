# Contract: Disclosure and the `run` Object

**Surfaces:** the console disclosure line, `detail::writeReport`'s new
parameter, the `run` key of the report document,
`docs/schema/report-v1.json` (research D-18, D-24, D-30; data model
E-05, E-08, E-09). **Covers:** FR-037 to FR-043, FR-084, FR-086; SC-009.

## Precedence

The gather settles every option from the highest source that names it:
command line, option file, environment, master default (FR-037, spec
D-1). A repeatable set by a higher source replaces the whole lower list,
and the comparison rule list is settled by the highest source that names
rules, a lower rule read for its fault before it drops (FR-038, FR-063),
in the order of [contracts/rules-file.md](./rules-file.md). A value its
reader refuses answers its fault line and status 2 whatever a higher
source names beside it (FR-027, FR-033, SC-003).

## The console line

One line, printed after the version line of `detail::printContext`
(`source/harness/cli.cpp:618`) and before the first benchmark row
(FR-042):

```text
options from a file or the environment: --min-time=20ms; --filter=bm; interleave=7
```

The prefix is exact. Then one entry per disclosed option in table order,
written `--<name>=<value>`; then `interleave=<seed>` when the option was
given, from either source; `; ` between entries. A repeatable value
joins its entries with `;` inside the entry, which is why the separator
between entries carries a leading space.

The line stands when any option came from a file or the environment, or
when `--interleave` stands given (FR-039). With no non-command-line use
and no interleaving, neither the line nor the object appears (FR-041).

## The document object

The top-level key `run` stands between `speedgun` and `benchmarks`: the
writer emits it after the `speedgun` object
(`source/harness/json_report.cpp:585` to `:590`) and before
`benchmarks` (`:591`).

```json
{
  "schemaVersion": "1.3",
  "speedgun": { "version": "0.18.0", "buildType": "Release" },
  "run": {
    "interleave": 7,
    "options": [
      { "option": "--min-time", "source": "file", "value": "20ms" },
      { "option": "--filter", "source": "environment", "value": "bm" }
    ]
  },
  "benchmarks": []
}
```

`interleave` is an unsigned integer, present only when the option was
given. `options` is an array of objects in table order, each with the
string keys `option`, `source` and `value`; `source` is `file` or
`environment`, and a `command line` use never appears (FR-039, FR-040).
The `option` text is the long spelling with `--` in both cases, the form
FR-039 gives for the console entry, and the `source` key carries which
source settled it; the environment name follows from the row by the rule
of research D-09.

The object is written with the same `simdjson::builder::string_builder`
helpers the rest of the document uses (`source/harness/json_report.cpp:26`),
and the stamped `schemaVersion` takes the minor read from master at
rebase (research D-19, FR-086).

## The schema

`docs/schema/report-v1.json` gains `run` in `x-order` at `:7` between
`speedgun` and `benchmarks`, and a `run` description in `properties` at
`:10`. `required` at `:8` keeps the three base keys, and
`additionalProperties` at `:9` stays true, so the key is optional and its
members stay open (FR-043, spec D-21).

The checker walks the document's own keys and requires each key's index
in `x-order` to stand at or after the previous one
(`source/harness/json_load.cpp:1098` to `:1120`), so a present key must
sit in the stated position; an absent optional key is never visited. The
reader answers no fault for the object's contents (spec D-21): the
document above reads as a report whose `run` key the reader ignores.

## The seam

`detail::writeReport` takes a `RunDisclosure` (E-08) beside the
`ReportData` it already takes. `ReportData` gains no field and the
public headers gain no symbol, so the harness `SOVERSION` stays at the
value `master` carries (`CMakeLists.txt:261`) and the archive
`SOVERSION` stays 3 (`:53`)
(FR-076, FR-077, research D-24).

## Oracle

`harness_json_report_test` writes a report through `detail::writeReport`
with a `RunDisclosure` built in the test and asserts the key order of
the written document, the two entry objects, the `interleave` key present
and absent, and the absence of the whole object when the disclosure is
empty; it runs the schema checker over each written document.
`harness_json_load_test` loads a document that carries `run` with open
members and asserts no fault (FR-043, FR-084, research D-30).
`harness_cli_sources_test` asserts the console line text, the table
order of the entries, the `interleave=<seed>` tail and the absence of
the line for a command-line-only run. `base_cli_matrix` and the shape
fold of `tools/docs/report_shape.py` cover SC-007: a run with no new
option writes the base key set and prints the base labels.
