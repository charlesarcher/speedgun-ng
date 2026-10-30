# Research: Hunter Sensitivity and Baseline Delta (040)

Date: 2026-10-30. Branch `040-hunter-sensitivity`. Each entry records
the decision, the rationale, and the alternatives considered, per the
plan workflow. The spec's Clarifications and Decisions stand behind
the entries that cite them; nothing here reopens them.

## PD-01 The Spearman correlation with average ranks at ties

**Decision**: `spearmanCorrelation` ranks each coordinate with
average ranks at ties, computes the Pearson correlation of the rank
pairs, and answers an empty optional when either coordinate stands
without spread or when the point count stands under 3.

**Rationale**: average ranks at ties stand the standard formulation,
and the point-count floor is the spec's own rule (FR-001, FR-002,
D-2). The no-spread case answers nothing because no rank order
exists to correlate; the standing `n/a` disclosure carries it
(FR-002). The helper joins `sg::detail` beside `statisticsOf`, the
one summary surface the harness reports and a hunt rank calls
(specs/029 FR-093), so the hunt adds no second copy (R-4, D-8).

**Alternatives considered**: the textbook `1 - 6Σd²/(n(n²-1))`
shortcut (rejected: it holds only with no ties, and tie-heavy counter
rates make the tie case common); a combined score figure (rejected:
out of scope, the `score` key stays refused).

## PD-02 The slope under one least-squares implementation

**Decision**: `leastSquaresSlope` over the same point shape reuses
the complexity fit's least-squares sums (`source/harness/complexity.cpp:69`),
moved beside `statisticsOf` so one implementation stands; the
complexity fit calls the moved helper unchanged.

**Rationale**: the standing sums compute `Σxy`, `Σx²`, `Σx`, `Σy`
over a point set, the exact arithmetic the slope needs; moving the
helper beside the statistics surface keeps the complexity fit's
behavior byte-identical (its tests run unchanged) and satisfies R-4's
one-implementation rule (D-8, PC-2).

**Alternatives considered**: a second slope helper inside the hunt
(rejected: R-4 names no second copy); a slope through the counters
library (rejected: the figure is a harness rank figure, the counters
surface carries none today).

## PD-03 The sensitivity points over one family

**Decision**: the batch stamping pass groups the run's entries by
suite and case name; one point per entry, the point carrying the
entry's aggregate row median time per iteration and the event's
normalized rate under the run's settled norm; an event stands its
figures over the entries that measured it, with the `n/a` disclosure
under 3 points.

**Rationale**: the spec fixes the point set over one family of one
run (D-2); the aggregate row's median stands the primary time per
iteration the report already carries (the spec's Assumptions), so no
second timing joins. The stamping stands after every entry's hunt
and before the console table prints and the document writes, so the
console and the JSON read the same figures (FR-030, FR-012).

**Alternatives considered**: points across separate runs (rejected:
the spec binds one run); the mean time per iteration (rejected: the
median stands the aggregate the report prints).

## PD-04 The baseline match through one matcher

**Decision**: the compare mode's `matchPair` name rule moves to a
shared detail helper; the compare calls it unchanged and the delta
extracts the baseline entry's hunt rates through the same pairing.

**Rationale**: R-2 carries the matching rules by reference to
specs/026 and names none twice; `matchPair` stands the implementation
of that rule (`source/harness/compare.cpp:1368`), and the compare
tests stay the guard. The delta matches events by catalog address
inside the matched entry (D-4), the rate figures standing in the
baseline document's per-entry hunt object (specs/029 FR-050).

**Alternatives considered**: a second name-matching loop inside the
hunt (rejected: R-2's one-name-twice rule); filter-scoped matching
(rejected: the hunt names no filter; the standing rule pairs by full
instance name).

## PD-05 The delta formula and its disclosures

**Decision**: `delta = 100 x (contender - baseline) / baseline` of
the matched event's median rate, signed; the `n/a` disclosure for a
missing event on either side, a missing baseline hunt object, and a
baseline rate at or below zero.

**Rationale**: the spec fixes the formula and the disclosure set
(D-4, D-5, FR-003, FR-004); the zero-denominator case follows the
standing anchor rule that a denominator at zero answers no figure
(specs/029 rankEvents contract). The run exits 0 in every disclosure
case (D-5).

**Alternatives considered**: an absolute-only figure (rejected: the
signed percent names the direction the rules file needs); a
magnitude-only sort key with a signed document figure (rejected: one
figure, one meaning, XI.7's one-term rule).

## PD-06 The emitted rule's threshold and direction

**Decision**: the threshold carries the run's noise threshold
(`options.noise`, a percent), one figure for every promoted event;
the direction stands from the measured delta's sign, `lower` for a
drop, `higher` for a rise, `lower` where no delta stands.

**Rationale**: the clarify session of 2026-10-30 settled the
threshold (spec Clarifications); the direction ruling comes from the
same session's answer, and the `lower` default stands the direction
the prompt's example names. The threshold rides `options.noise`
because the hunt already uses it to separate a real change from
noise, so the emitted gate fires on a beyond-noise move of a
promoted event.

**Alternatives considered**: the measured delta magnitude
(rejected: one sample's size baked into a threshold, and no baseline
leaves the run nothing to emit); the mixed figure (rejected: two
figures in one file against XI.7's one-term rule).

## PD-07 The JSON key names and the null rule

**Decision**: the per-entry hunt object gains `correlation`,
`slope` and `delta`, in that order after `rank`; `putOrNull` writes
null for a figure the run did not measure; the schema file gains the
keys at minor `1.9` with the `x-order` moved in the same commit.

**Rationale**: the closed-node rule of the standing node
(`source/harness/json_report.cpp:499-540`) writes every key of every
hunt document with null for an unset optional; the key names stay
short inside a node whose context names the hunt entry (FR-030,
FR-032).

**Alternatives considered**: `sensitivity_correlation` and
`sensitivity_slope` (rejected: the node's context already names the
entry, and the standing keys stand short: `rate`, `cv`, `rank`).

## PD-08 The CLI refusals join the standing shapes

**Decision**: `--hunt-rank=delta` beside no `--hunt-baseline`,
`--hunt-baseline` beside no hunt, an empty value, and the
norm-mismatch pair fault each print one stderr line naming the value
or the mismatch and exit 2, beside the refusal shapes
`hunt_cli.cpp` and the hunt run path carry today.

**Rationale**: the clarify session settled the delta-rank refusal
and the norm refusal; the standing refusal family
(`hunt_cli.cpp:49-56`, specs/033 FR-012) fixes the shape: one line,
the value named, status 2 (FR-005, FR-022, FR-023).

**Alternatives considered**: an all-`n/a` run for the missing
baseline (rejected: an option combination the user controls takes
the refusal, a measurement outcome the run cannot control takes the
disclosure).
