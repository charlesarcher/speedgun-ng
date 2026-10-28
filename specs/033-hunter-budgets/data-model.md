# Data Model: Hunter Budgets and Promotion (033)

Date: 2026-10-28. Extends `specs/029-hunter-core/data-model.md`; the
029 entities (E-01 to E-11) stand unchanged; 033 numbers its
own entities N-01 to N-08. Validation rules cite
the 033 FRs.

## N-01 Sample budget

The bound on one event's measurement inside a stage.

| Field | Type | Notes |
| --- | --- | --- |
| `sampleBudgetNs` | `std::optional<std::uint64_t>` | whole nanoseconds; parsed from the `e` clause (fractional seconds admitted, FR-004); absent means no bound |

State: absent → the stage rule repeats stand; present → the derived
count of N-03 applies.

## N-02 Multiplex cap

The bound on the events one multiplexed group holds.

| Field | Type | Notes |
| --- | --- | --- |
| `multiplexCap` | `std::optional<std::size_t>` | whole count 1 to 10000 from the `g` clause (FR-004); absent means no cap |

## N-03 Derived repeats

The repeat count the planner derives under the sample budget.

| Field | Type | Notes |
| --- | --- | --- |
| `derivedRepeats` | `std::optional<std::size_t>` | `min(stageRule, max(1, fit))`, R-02; present exactly when a figure priced the stage (FR-001, FR-002) |

State transitions (per stage):

```text
no sample budget ........................ stage rule repeats stand
sample budget, no calibration figure ... stage rule repeats stand, record: unpriced
sample budget, fit >= 1 .................. derived repeats, record: derived d of m
sample budget, fit < 1 ..................... one repeat, record: overrun
```

## N-04 Multiplex group

One consecutive block of planned addresses one group opens.

| Field | Type | Notes |
| --- | --- | --- |
| `muxGroups` | `std::vector<std::size_t>` | group sizes in plan order; every entry at or below the cap; sum equals the planned count; empty when the method is not `MULTIPLEX` or no cap stands (FR-003) |

## N-05 Promoted counter set

The list one entry's ranking answers, and the file the run writes.

| Field | Type | Notes |
| --- | --- | --- |
| emit list | `std::vector<std::string>` | `toCounterSet(topN)`: ranked entries only, rank ascending, capped at `topN` (FR-013) |
| emit file | text file | the deduplicated union of the per-entry lists, entry order then rank order, one address per line, no header; possibly empty (FR-011) |

Invariants: no address stands twice in the file; an event with a
null rank never enters; the file reads again through
`--counter=@<file>` to one report column per line (FR-015).

## N-06 Budget disclosure

The figures the run publishes.

| Field | Console | JSON key | Kind |
| --- | --- | --- | --- |
| two-stage budget | `budget=<text>` (standing) | `budget` (standing) | text or null, standing |
| sample budget | `sample=<e text>` when set | `sample_budget` | conditional text |
| multiplex cap | `mux=<g text>` when set | `multiplex_cap` | conditional text |
| derived repeats | disclosure line when the count moved | `derived_repeats` | conditional integer |
| group sizes | disclosure line when capped | `mux_groups` | conditional integer array |

Rule: an unset budget writes and prints nothing new (FR-022); the
texts are rebuilt canonically from the stored values (research D-5).

## N-07 The shared address-file record

`readAddressFileLines(path, context)` answers the file's address
elements under spec 029's separator rule: each line cuts at the
first `#`, blank lines stand ignored, the rest splits on commas, and
a comma beside nothing stands as an empty element. One level only.
The reader itself refuses in one line only an unopenable file,
naming the path; each caller refuses an empty element and a file
holding no address in its own sentence:
the hunt route names the selection text, the `--counter` route names
the option value (FR-014; research D-7).

## N-08 The option row

| Field | Value |
| --- | --- |
| name | `hunt-emit-set` |
| value | `280` (`kHuntEmitSetOption`) |
| argument | required |
| environment | no (the hunt rows' shape) |
| owner | `specs/033` |

`281` to `289` stay free; the `options.hpp` block comment moves with
the row (FR-010).

## Validation rules

- Grammar: R-01 refusals, `contracts/budget-grammar.md` (FR-004).
- Emit: R-07/R-08, `contracts/counter-file.md` (FR-011, FR-012).
- Reader: R-09, `contracts/counter-file.md` (FR-014).
- Presence: R-11/R-12, `contracts/hunt-json.md` (FR-020 to FR-022).
