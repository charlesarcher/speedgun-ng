# Contract: the hunt rank figures (specs/040)

The public surface `include/speedgun-ng/hunt.hpp` declares; the
implementation under `source/harness/` enforces. One contract stands
stated in one place: the header carries the doxygen, the source
carries the enforcement, and this file names the behavior the tests
pin. The doxygen contracts of the headers govern on disagreement
(Constitution II).

## A. The shared rules

- A-1: The figures stand pure computations over the collected
  records: no function reads a file, writes a file, reads a clock,
  reads a counter, throws to the caller or aborts (FR-006). The run
  path alone loads the baseline through `loadReport` and writes the
  rules file.
- A-2: An optional figure stands empty exactly when the run
  measured none: under 3 points, a no-spread point set, a missing
  match on either side, a baseline rate at or below zero (FR-002,
  FR-004, D-5). The empty state stands the `n/a` disclosure on the
  console and null in the document (FR-030).
- A-3: The noisy events stand unranked under both new keys, the
  standing rule of specs/029 D-7 (FR-011).
- A-4: Only an event that carries the requested key's figure
  carries a rank under that key; the ranks stand `1` to `k` with no
  gap (D-3, the standing rule of specs/029 I-5).
- A-5: The delta stands a signed percent of the matched event's
  median rate: `100 x (contender - baseline) / baseline` (D-4).

## B. The per-function contracts

### B-1 `EntryHunt::toCompareRules(topN, noisePercent)`

- Answers `std::vector<std::string>`; one line per promoted event,
  at most `topN` ranked entries in rank order.
- \pre `noisePercent` stands above `0.0` in the range the option
  states; `topN` names no constraint beyond the standing
  `toCounterSet` pair rule.
- \post each line reads
  `counter:<event>@median:<lower|higher>:<n>%`; the threshold
  carries `noisePercent` for every line; the direction stands
  `lower` for a negative delta, `higher` for a positive one,
  `lower` where the delta stands empty or at an exact zero, a zero
  standing no rise; an entry whose `rank`
  stands empty holds no line; the call measures nothing and reads
  no clock (FR-021, D-7).
- The pair shape of `toCounterSet(topN)`: the same cap rule, the
  same rank order (FR-024).

### B-2 `sg::detail::spearmanCorrelation(points)`

- Answers `std::optional<double>` over one point set.
- \pre `points` holds no empty pair (the assembly guarantees it).
- \post under 3 points, or a no-spread coordinate, answers empty;
  otherwise answers the Pearson correlation of the rank pairs,
  average ranks at ties, in `[-1, 1]`; a strictly monotone set
  answers `1` or `-1` (FR-001, FR-002, D-2).

### B-3 `sg::detail::leastSquaresSlope(points)`

- Answers `std::optional<double>`: the least-squares slope of the
  rate against the time per iteration, through the moved sums the
  complexity fit shares (D-8, R-4).
- \post under 2 points, or a no-spread time coordinate, answers
  empty; an exact line answers its slope to the arithmetic's
  precision.

### B-4 The family point assembly and the sensitivity stamp

- \pre the entries carry ranked `EntryHunt` records with the
  standing rate figures; the grouping reads the suite.
- \post one point per entry that measured the event, the time
  figure the entry's aggregate row median, the rate under the
  run's settled norm; under 3 points the stamp leaves both
  sensitivity fields empty, and the run exits 0 (FR-001, FR-002,
  D-2).

### B-5 The baseline extraction and the norm check

- \pre the baseline document stands a value `loadReport` answered.
- \post the extraction reads the matched entry's per-entry hunt
  object's rates by catalog address; a schema pairing the
  comparison refuses, and a baseline norm different from the
  contender's settled norm, each stand one stderr line naming the
  value or the mismatch and exit 2, before the entries run (FR-003,
  FR-005, D-6).

### B-6 The delta stamp

- \post a matched pair with a baseline rate above zero stands the
  signed percent of A-5; a missing match on either side, a missing
  baseline hunt object, and a baseline rate at or below zero leave
  the field empty, and the run exits 0 (FR-004, D-5).

### B-7 The rank assignment under the new keys

- \pre the stamping pass ran, so the key's figure stands on the
  records the key reads.
- \post `--hunt-rank=sensitivity` orders by the correlation
  descending, ties by the slope and then the catalog address;
  `--hunt-rank=delta` orders by the magnitude of the delta
  descending, ties by the catalog address; A-3 and A-4 hold; an
  event set that stands empty under the key prints in the standing
  order with empty rank fields and exits 0 (FR-011, D-3).

### B-8 The CLI settlement

- \post `--hunt-rank` accepts `sensitivity` and `delta`; `score`
  alone keeps the refusal naming its roadmap item; the two new rows
  stand at values `281` and `282`, long spelling only, no
  environment source; `--hunt-emit-rules` beside no hunt, an empty
  value, and an unwritable file each print one line naming the
  value and exit 2; `--hunt-baseline` beside no hunt and an empty
  value do the same; `--hunt-rank=delta` beside no
  `--hunt-baseline` refuses the same way; a hunt that promotes no
  event writes an empty rules file with exit 0 (FR-010, FR-020,
  FR-022, FR-023).

### B-9 The document keys

- \post the per-entry hunt object writes `correlation`, `slope`
  and `delta` in its `x-order`, null for an empty figure, every key
  in every hunt document; `loadReport` reads them back equal; the
  schema file pairs the keys at minor `1.9` (FR-030, FR-031,
  FR-032).

### B-10 The run-path sequencing

- \post the baseline loads once, before the entries run; the
  stamping pass runs after every entry's hunt stands ranked and
  before the console table prints and the document writes; the
  rules file, when asked, writes after the stamping pass, so the
  lines carry the stamped figures; the matching-rules disclosure
  prints once beside the figures (FR-003, FR-004, FR-021).
