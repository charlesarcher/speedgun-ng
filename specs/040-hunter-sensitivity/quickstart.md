# Quickstart: the hunter sensitivity and baseline delta (specs/040)

The validation scenarios that prove the feature end to end. The
details stand in [plan.md](./plan.md), the contracts in
[contracts/hunt-ranks-api.md](./contracts/hunt-ranks-api.md), and
the shapes in [data-model.md](./data-model.md).

## Prerequisites

- A Linux host with PMU access for the acceptance runs; the
  acceptance host stands thrawn (AMD Zen 5). The scripted rows run
  anywhere: the tests build the records directly, no PMU needed.
- The `dev` presets configured: `cmake --preset=ci-rocky` drives the
  release build into `build` on a fresh clone.

## Build and the test rows

```
$ cmake --preset=ci-rocky
$ cmake --build build
$ ctest --test-dir build --output-on-failure -R harness_hunt
$ ctest --test-dir build --output-on-failure -R harness_json
```

Every row stands exit 0. The rank rows cover the figures and their
disclosures, the JSON rows cover the round trip at schema minor
`1.9`, and the CLI rows cover the keys, the refusals and the table.

## The end-to-end scenario

1. Record a baseline document with a hunt over a benchmark family of
   at least 3 arg points:

   ```
   $ build/speedgun-ng --benchmark=<family regex> --args=... \
         --hunt=all --json=baseline.json
   exit 0
   ```

2. Run the contender with the baseline and the two ranks:

   ```
   $ build/speedgun-ng --benchmark=<family regex> --args=... \
         --hunt=all --hunt-rank=sensitivity \
         --hunt-baseline=baseline.json \
         --hunt-emit-rules=rules.txt --json=contender.json
   exit 0
   ```

   The ranked table carries the sensitivity column (the correlation
   and the slope) and the delta column; the matching-rules
   disclosure prints once beside the figures; `rules.txt` holds one
   N5 rule line per promoted event.

3. Measure the promoted events as counter columns and read the gate:

   The rules file\'s lines name catalog addresses, and
   `promoted.txt` carries them one per line for the `--counter=@`
   form. The follow-up runs measure the promoted events as counter
   columns, and the comparison of the two follow-up documents
   answers the verdicts under the gate file the record derives, the
   lines of `rules.txt` under the `metric:` prefix the `--counter`
   columns carry (FR-021, the round trip of SC-003):

   ```
   $ build/speedgun-ng --benchmark=<family regex> --args=... \
         --counter=@promoted.txt --json=contender.json
   exit 0

   $ build/speedgun-ng --compare=baseline.json,contender.json \
         --compare-rules=gate-measured.txt
   exit 0 or 1, the gate\'s verdict
   ```

   The hunt document itself carries its rates in the per-entry hunt
   objects and no counter columns, so the comparison of the hunt
   documents stands refused (specs/026 FR-036), and a rule whose event
   measures zero stands the zero-baseline refusal (specs/026 FR-031).

## The refusal scenarios

- A run with `--hunt-rank=delta` and no `--hunt-baseline` prints one
  line naming the value and ends with status 2.
- A run with a baseline whose norm differs from the contender's
  prints one line naming the mismatch and ends with status 2.
- A family under 3 arg points prints the sensitivity cells `n/a`
  and ends with status 0.

## The acceptance records

The thrawn acceptance runs stand recorded under
[results/](./results/), one record per run naming the commands, the
exit codes and the artifacts (FR-052). Each record below links:

- The sensitivity record: the family run of scenario 1 and 2 with
  the ranked table: [sensitivity.md](./results/sensitivity.md).
- The delta record: the baseline pair of scenario 2 with the
  disclosure: [delta.md](./results/delta.md).
- The round-trip record: scenario 3 with the comparison verdicts:
  [round-trip.md](./results/round-trip.md).
- The refusal record: the delta rank beside no baseline:
  [refusal.md](./results/refusal.md).
