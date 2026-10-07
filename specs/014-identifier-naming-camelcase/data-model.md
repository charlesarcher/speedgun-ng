# Data model: identifier naming

The entities below are the ones `spec.md` names. Field rules come from
the requirements and from research D-01 through D-08. This file does not
list every identifier the rename will touch.

## Naming rule

One constitutional spelling rule. The constitution text holds the rule.
A clang-tidy option implements it, or the research records that the
check cannot express it.

| Field | Rule |
| --- | --- |
| Id | `N-1` through `N-11`, or `constants` for the `kPascalCase` spelling |
| Applies to | The kind of name the rule names (type, function, macro, enumerator, variable, member, namespace, template parameter, file name, acronym, tag) |
| Spelling | PascalCase, lowerCamelCase, `UPPER_SNAKE_CASE`, `kPascalCase`, or lower_case, as the rule states |
| Check option | The key in research D-05, or `review` when D-05 records no option |
| Authority | `.specify/memory/constitution.md` after the 2.14.0 amendment |

Validation:

- A rule that exists only in `.clang-tidy`, a document, or this feature's
  specification fails FR-009.
- N-9 does not rename a file, a CMake target, a CMake option, a preset,
  or the package.
- N-10 and N-11 have no clang-tidy option (D-05). Review enforces them.
  Each tag object is listed in `ConstexprVariableIgnoredRegexp` and the
  list entry cites N-11.

## Exception entry

One spelling the language, the standard library, a vendor, or the
platform requires. The exception list in `spec.md` is the only list.
The amendment copies it into the constitution.

| Field | Rule |
| --- | --- |
| Spelling | The token that must keep its current spelling |
| Reason | The protocol, vendor, or generator that requires that spelling |
| Check escape | The `IgnoredRegexp` or `IgnoreMainLikeFunctions` setting in D-05 |
| Suppression | A remaining `NOLINT` cites this entry. A suppression the new rules make unnecessary is removed (FR-019). |

Validation:

- No exception exists outside the list in `spec.md`.
- `empty()` at `include/speedgun-ng/counters_measurement.hpp:124` is the
  standard-protocol name the specification confirms.
- `SPEEDGUN_NG_EXPORT` stays, and it carries no `SG_` prefix.
- `main` stays. Identifiers inside `external/` stay.

## Identifier

A C++ name the project owns in `include/`, `source/`, `test/`,
`example/`, or `tools/`.

| Field | Rule |
| --- | --- |
| Spelling | The token as declared |
| Kind | Type, function, enumerator, variable, member, constant, macro, namespace, template parameter, or tag |
| Owner | Project, or an exception entry |
| Scope | Shipped header, detail namespace inside a shipped header, file-local helper, or test-only |
| New spelling | The spelling the matching naming rule requires |

Validation:

- FR-013 covers every owned name, including names added by specs 012 and
  013 and by the 0.4.1 patch.
- A namespace stays lower_case (N-7).
- A string literal, catalog name, object path, provenance string, unit
  token, read-mode label, and error message keep their text (FR-016).
- The member `availability` stays. The type becomes `Availability`. The
  qualification workaround at the two sites in the specification goes
  (FR-018).

## Rename map entry

One old spelling paired with one new spelling. The map is published in
this feature directory and in `docs/`. This command does not fill the
map.

| Field | Rule |
| --- | --- |
| Old spelling | The spelling declared before the rename |
| New spelling | The spelling the naming rule requires |
| Header | The shipped header that declares the name |
| Kind | The identifier kind |
| Detail | True when the name is in a detail namespace |

Validation:

- A shipped header is a header under `include/` that the package installs.
- A detail name is in the map (FR-015).
- A test-only name stays out of the map. A file-local helper stays out.
  FR-013 still renames those names.
- A search over the trees FR-015 names finds no old shipped-header
  spelling outside the map and the exception list.
- `specs/007-counters-and-timers/citations-log.md` gains one entry that
  points at the map. Other closed spec directories stay unedited, except
  the logged wording correction in FR-020.

## Gate baseline

The default-branch commit that passes every hard gate before the first
rename commit. The audit point is not that commit.

| Field | Rule |
| --- | --- |
| SHA | The head of a green Continuous Integration run. Absent until D-06 step 3 |
| Test set | The passing test names at that SHA |
| Gate results | Pass or fail for each hard gate |
| Name-check counts | Finding count per translation unit at that SHA |
| Predecessor | The format repair of the 21 files the specification lists. No identifier changes. |

Validation:

- FR-021 forbids a rename commit before this record exists in `plan.md`.
- Comparisons in FR-001, FR-002, and FR-003 use this commit.
- The last green run,
  `6aafd2dc8a1310ee335820ad8dc4dedd0eb0ae17`, is not this entity. It
  predates the counters names FR-013 renames.
- The audit point
  `6d32efcab3c13c3d41470c1c621d78839ce11543` is not this entity. Run
  37553123469 failed the lint job and skipped the rest.

## State

The rename sequence has a fixed order (research D-02). A later group
does not start while an earlier group fails to compile, fails its
tests, or fails the format check.

1. Gate baseline recorded.
2. Rename groups 1 through 9, one commit each, declaration and call
   sites together.
3. Closing commit: constitution 2.14.0, clang-tidy keys,
   `WarningsAsErrors`, version fields, rename map, document edits.

Until the closing commit, `WarningsAsErrors` stays empty. The name check
is a head gate (FR-004), and each earlier commit still compiles, passes
its tests, and passes the format check (FR-007).
