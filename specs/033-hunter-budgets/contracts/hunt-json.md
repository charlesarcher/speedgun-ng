# Contract: the `hunt` JSON extension at schema 1.7 (033)

Extends `specs/029-hunter-core/contracts/hunt-json.md`; the standing
sixteen keys and their order stand. (FR-020 to FR-023, R-11, R-12.)

## New keys (top-level `hunt` object)

| Key | Type | Presence | Content |
| --- | --- | --- | --- |
| `sample_budget` | string | only when the sample budget stands | canonical `e` clause text, rebuilt from whole nanoseconds (`0.5e`) |
| `multiplex_cap` | string | only when the cap stands | canonical `g` clause text (`32g`) |
| `derived_repeats` | integer | only when a figure priced the stage under a sample budget | the derived count (033 N-03) |
| `mux_groups` | integer array | only when the cap split the plan | group sizes in plan order |

Presence rules:

- an unset budget writes no key (not a null key): a hunt run without
  the budgets writes the 1.6 bytes apart from `schemaVersion`
  (FR-022);
- each key joins `properties` and `x-order` in the same commit as
  its writer (the US2 pair first, the US3 pair after); `required`
  keeps the standing sixteen, so a 1.4 to 1.6 hunt
  document stays valid and `loadReport` reads it equal (FR-021);
- `x-order` position: after `stage_two_events`, the final order
  fixed as listed above; the US3 keys insert, `multiplex_cap`
  after `sample_budget` and `mux_groups` after `derived_repeats`.

## Read rules (`loadReport`)

- missing key: the record's field stands unset;
- present key: kind must match; each refusal carries its own test:
  wrong kind, an `e` text with no unit, an `e` text with no amount,
  a negative or fractional `g`, a non-integer in `mux_groups`;
- round trip: write then read answers equal records, fractions
  included (`0.5e` rebuilds identically).

## Console parity (FR-023)

The plan line carries `sample=<text>` and `mux=<text>` in the same
states the JSON keys hold, and the two disclosure lines spell the
`derived_repeats` / `mux_groups` facts; the console and the document
state one fact from one record.
