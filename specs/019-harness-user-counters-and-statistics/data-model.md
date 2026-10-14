# Data Model: Harness User Counters and Statistics

Phase 1 output for `specs/019-harness-user-counters-and-statistics/spec.md`.
The entities are the Key Entities of the spec, plus the records the run
and the report move between them. Every anchor is the audit-point line
of `a7776b6`, and each `+` marked field is new in this spec.

## E-01: Counter slot and the fixed store

One run's counters live in the state detail as
`std::array<CounterSlot, kMaxUserCounters>` (`include/speedgun-ng/benchmark.hpp`,
new; research D-01, FR-001, FR-007).

| Field | Type | Source |
| --- | --- | --- |
| `name` | `char[kMaxCounterNameBytes]`, a 32-byte inline buffer holding the name and terminator | + bounded by the name-length failure rule (D-01) |
| `value` | `double`, the flag-neutral value the body last set | + (FR-001) |
| `flags` | `CounterFlag`, fixed at the slot's first open | + (FR-008) |
| `base` | `CounterBase`, fixed at the slot's first open | + (FR-008) |
| `used` | `bool` | + |

Relationships: `State::setCounter` scans slots for a name match, writes
the value into a match, opens a free slot otherwise; the runner's flag
pass (E-02) reads the slots once per repetition after the loop; the row
copies the finished values (E-07).

Validation: opening the seventeenth name fails the run with a reason
stating `kMaxUserCounters` (FR-001). A name longer than
`kMaxCounterNameBytes` minus one fails the run with a reason stating the
bound (D-01). A second `setCounter` for a used name carrying different
flags or a different base fails the run naming that name (FR-008). The
store allocates nothing inside the timed loop, so the loop-shape gate
sees `setCounter` as body code alone (FR-007).

State transitions: a slot moves free, to open (name, flags, base fixed),
to value updates for the run's life. An open slot never changes its
flags, its base, or its name, and never frees during the run.

## E-02: Counter, flags, base, and the flag pass

`Counter` is a public class in namespace `sg`
(`include/speedgun-ng/benchmark.hpp`, new; FR-002, FR-003).

| Piece | Values | Source |
| --- | --- | --- |
| `CounterFlag` | `NONE`, `RATE`, `AVG_THREADS`, `ITERATION_INVARIANT`, `AVG_ITERATIONS`, `INVERT`, composable by `operator|` | + mirrors the cited `Counter::Flags` (`counter.h:33-44`) |
| `CounterBase` | `K1000`, `K1024` | + mirrors the cited `OneK` (`counter.h:46`) |
| `Counter` | value, flags, base, constructor defaults `0.0`, `NONE`, `K1000` | + (FR-002) |

The flag pass is the runner's cold path after the loop; it is an
operation, stored state (D-02).

| Step | Rule | Source |
| --- | --- | --- |
| rate | `v = v / (primaryNs / 1e9)` | FR-003, FR-004; `src/counter.cc:22-44` |
| thread average | `v = v / 1` while threads are out of scope | FR-004, Q-3 |
| iteration invariant | `v = v * iterations` | FR-003 |
| average over iterations | `v = v / iterations` | FR-003 |
| invert | `v = 1 / v`, last and always | FR-003 |
| zero divisor | `primaryNs <= 0`: the neutral value stands, the row's counter set carries the disclosure | FR-005, Edge Cases |

Relationships: `primaryNs` is the decision value of the run
(`runner.cpp:463-466`), the same number the qualification and the growth
rule read (PC-2).

## E-03: Throughput counters

Two preset counter shapes opened through state calls
(FR-009, FR-010).

| Call | Name opened | Flags | Base |
| --- | --- | --- | --- |
| `setBytesProcessed(bytes)` | `bytes_per_second` | `RATE` | `K1024` |
| `setItemsProcessed(items)` | `items_per_second` | `RATE` | `K1000` |

Relationships: both are ordinary E-01 slots, so the flag pass, the
disclosure rule, and the statistics of any named counter apply to them;
their printed value scales by their base like any counter (FR-006).

Validation: repeated calls per iteration accumulate by the body's own
arithmetic: the call sets the value the body passes, matching the cited
assignment semantics (`state.h:86-91`, `:107-112`).

## E-04: Label

One string per run and per row (FR-011, FR-012; clarification
2026-10-14).

| Field | Type | Source |
| --- | --- | --- |
| `State` label | `std::string`, last write wins inside a repetition | + (FR-011) |
| `ResultRow::label` | `std::string`, copied from the state at row build | + (FR-011) |

Relationships: the report prints ` label=<text>` after the name column
when the string is non-empty; the aggregate result's label prints only
when every repetition's string is equal, the cited rule
(`src/statistics.cc:178-186`).

Validation: a label containing a newline is a precondition violation at
`setLabel` (Edge Cases). Any other text prints verbatim. The field is
one per row, so no escape or quoting rule exists.

## E-05: Custom statistic

A (name, unit, function) triple on the family record
(FR-013 to FR-015).

| Field | Type | Source |
| --- | --- | --- |
| `name` | printed token, written at `addStatistic` | + (FR-013) |
| `unit` | `StatUnit::TIME` scales with the row unit; `StatUnit::PERCENTAGE` prints unscaled | + mirrors the cited `StatisticUnit` (`statistics.h:29`) |
| `function` | `StatisticFunc = std::function<bool(const std::vector<double>&, double&)>` over the clean repetition values of the primary time | + mirrors the cited `StatisticsFunc` (`statistics.h:33`) |

Relationships: the runner evaluates each statistic after `Statistics`
for the row is built, appending `(name, value, unit)` records the report
prints as fields beside the built-ins; a function returning false
appends nothing for that statistic (FR-015).

Validation: a name equal to `mean`, `median`, `sd`, `cv`, `min`, or
`max` is a precondition violation at `addStatistic` (Edge Cases). The
call stands behind the standing setter precondition (FR-046).

## E-06: Fit selection and the family fit

The family record carries the selection; the runner batch carries the
result (FR-019 to FR-023).

| Piece | Type | Source |
| --- | --- | --- |
| `RegistryEntry` fit selection | `BigO` plus an optional `BigOFun`, default `NONE` | + mirrors the cited `Complexity` overloads (`benchmark_api.h:135-136`) |
| `State` complexity N | `std::int64_t`, written by `setComplexityN`, default non-positive | + (`state.h:99-105`) |
| instance N | the complexity N when set, else the instance's range value (D-08) | + |
| fit row | a `ResultRow` with `kind` `BIG_O` or `RMS`, `label` the quantity name (empty for the time), coefficient in the time field or `counters[0]`, RMS percentage in the time field of the RMS row | + (D-09) |
| fit result | one synthetic `BenchmarkResult` per fitted family, named after the family, rows in pair order: time, counters in open order, metrics in attach order | + (FR-022) |

Relationships: the fit reads each family's per-instance aggregate value
per quantity; the synthetic result joins the batch's result list so the
report, the mode filter, and any later file output consume fit rows
through the same path (D-09).

Validation: fewer than two instances, or an instance with a
non-positive N, produces the named reason and prints no fit rows
(FR-023). A
quantity absent from one instance ends that quantity's pair with the
same rule (FR-022 edge, spec Edge Cases). The coefficient and RMS follow
the closed form of D-07; `AUTOMATIC` keeps the least RMS with the
constant curve as the fallback (FR-021).

## E-07: Family record and result model, extended

`RegistryEntry` (`source/harness/detail/internal.hpp:44-72`) and the
public result structs gain fields; no shipped field changes type
(FR-046).

| Field | Type | Source |
| --- | --- | --- |
| `RegistryEntry::statistics` | vector of E-05 triples | + (FR-013) |
| `RegistryEntry::reportAggregatesOnly`, `displayAggregatesOnly` | `bool`, default `false` | + (FR-016, FR-017) |
| `RegistryEntry::bigO`, `bigOFun` | E-06 selection | + (FR-020) |
| `RegistryEntry::timeUnit` | `std::optional<TimeUnit>` replacing the captured value | + (FR-027, D-11) |
| `ResultRow::label` | `std::string` | + (FR-011) |
| `ResultRow::counters` | `std::vector<double>`, flag-applied values in `BenchmarkResult::counterLabels` order | + (FR-006) |
| `BenchmarkResult::counterLabels` | `std::vector<std::string>`, name order fixed at the first row built | + mirrors `metricLabels` (`benchmark.hpp:184-186`) |
| `BenchmarkResult::counterStats` | `std::vector<Statistics>`, per named counter over clean rows | + (FR-006) |
| `BenchmarkResult::hideRepetitionRows` | `bool`, set when either mode stands | + (D-06) |
| `BenchmarkResult::pauseSamplesFolded` | `bool`, true when the run netted its closed pause pairs by folding | + (FR-052, FR-054) |
| `RowKind` | gains `BIG_O` and `RMS` | + (D-09) |

Relationships: the runner writes every field once per repetition and
once per aggregate; the report reads what it prints and ignores the
rest, the standing separation.

Validation: a counter name equal to an attached metric label fails the
run naming the collision, checked when the first row is built
(Edge Cases). The report hides repetition rows only when
`hideRepetitionRows` stands and the result holds an aggregate row, so a
single-run row never disappears (FR-018).

## E-08: Run capacity and the unit resolution (the two fixes)

| Piece | Rule | Source |
| --- | --- | --- |
| points per leaf column | the endpoint points of every run the phase rule can start, plus pause room of 2 × observed pairs per iteration × iterations from the previous attempt, capped at `kMaxPauseSamplesPerLeaf` (32'768 raw points, 256 KiB at 8 bytes each) | FR-025, FR-051, D-10 |
| recorder reservation | per leaf column, sized at plan build between attempts; no pause room without family pause history | FR-025, FR-053 |
| fold fallback | an attempt without history or over the cap nets closed pairs into the row's running totals at point commit, from the same two raw points, printing the raw path's columns | FR-052, D-10 |
| `kMaxPausePerRun` | deleted with its guard, its capacity term, its bound tests, and the `ninthPause` mode | FR-024, FR-026 |
| batch default unit | the `getDefaultTimeUnit()` value snapshot taken at `speedgunMain` entry | FR-027, D-11 |
| result unit | entry value, else batch snapshot, else the process default read at run start | FR-027 |

Relationships: the pause intervals stay sample-ordinal pairs under the
H3 netting rule (017 E-03), and a run with any number of closed pairs
nets every column as before (FR-026). The unit reaches
`BenchmarkResult::timeUnit` (`benchmark.hpp:193`) and the printed
columns through it.

Validation: an open pause at loop exit stays the existing contract
violation; a closed pair count above the bound cannot occur because the
loop shape caps it (D-10). The `BenchmarkResult::timeUnit` doxygen at
`:188-192` loses the captured-at-registration sentence with this change
(FR-027).
