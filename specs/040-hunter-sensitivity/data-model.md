# Data model: the hunter sensitivity and baseline delta (specs/040)

The ranks stand on the standing hunt records; this file records the
shapes the feature adds and the fields it reads.

## The types the feature adds

- **`sg::hunt::RankedEvent`** gains three fields, each
  `std::optional<double>`, empty when the run measured none:
  `sensitivityCorrelation`, the Spearman rank correlation of the
  event's rates against the family's times per iteration;
  `sensitivitySlope`, the least-squares slope of the same points;
  `delta`, the signed percent
  `100 x (contender - baseline) / baseline` of the matched event's
  median rate. The record stays data; the console, the document and
  the rank order read it.
- **`sg::hunt::EntryHunt::toCompareRules(std::size_t topN, double
  noisePercent)`**: answers the promoted events' N5 rule texts, at
  most `topN` ranked entries in rank order, one line per event:
  `counter:<event>@median:lower:<n>%` for a drop, `higher` for a
  rise, `lower` where no delta stands; the threshold carries
  `noisePercent` for every line. The pair shape of `toCounterSet`
  (FR-024, D-7).
- **`sg::hunt::HuntOptions`** gains the baseline path the CLI
  settled, empty when the run named none. The CLI refuses the
  malformed combinations before the record exists.
- **The detail point record** (`source/harness/detail/hunt_ranks.hpp`):
  one sensitivity point, the time per iteration and the event's
  normalized rate; the detail surface groups the points per family
  and per event address.
- **The detail baseline map**: the extracted rates of one loaded
  baseline document, per benchmark entry name: the baseline's norm
  word and the per-event median rates of its per-entry hunt object.

## The standing types the ranks read

- `sg::hunt::EntryHunt` (`include/speedgun-ng/hunt.hpp`): `entries`
  carries one `RankedEvent` per planned column, the ranked ones
  first; `top` carries the promoted-set cap `toCompareRules` reads.
- `sg::hunt::EventSamples` and `HuntSample`: the collected rows the
  normalized rate stands from (specs/029's records, unchanged).
- `sg::ReportData` and `sg::BenchmarkResult`
  (`include/speedgun-ng/benchmark.hpp`): `results` carries one entry
  per benchmark; `suite` and `caseName` group the family; the
  aggregate row's `timePerIterationNs` median stands the point's
  time figure; `hunt` carries the entry's `EntryHunt`.
- `loadReport` (`include/speedgun-ng/benchmark.hpp:2878`): the one
  baseline input (R-1); the loaded document's per-entry hunt objects
  stand the rate figures the delta matches against.
- The compare matcher (`source/harness/compare.cpp:1368`, moved to
  the shared detail surface): the specs/026 name rule the baseline
  pairing reuses (R-2).
- `sg::detail::statisticsOf` and the moved least-squares sums: the
  one statistics surface (D-8, R-4).

## State and lifecycle

- The figures stand empty until the batch stamping pass runs: after
  every entry's hunt stands ranked, before the console table prints
  and the document writes. A run with one entry, a family under 3
  entries, and a run without a baseline leave the fields their
  disclosure cases name (FR-002, FR-004).
- The rank fields carry no state machine: a figure stands measured
  or empty, and the empty state stands the `n/a` disclosure on the
  console and null in the document (FR-030).

## Validation rules

- The norm-pair check refuses a baseline whose top-level hunt object
  names a norm different from the contender's settled norm, one line
  naming the mismatch, exit 2 (FR-005, the clarify Q3 ruling).
- The baseline file the loader refuses, and a schema pairing the
  comparison refuses, exit 2 with one line naming the value (FR-005).
- A baseline rate at or below zero stands the `n/a` disclosure, no
  error (D-5).
