# Research: Regression Gate

**Date:** 2026-10-21 · **Spec:** [spec.md](./spec.md)

The spec numbers its own decisions D-1 to D-22. This file numbers the
plan's decisions D-01 to D-20: a bare `D-nn` in an artifact names this
file's entry, and a spec decision cited outside `spec.md` is written
`spec D-nn`, because the two numberings name different decisions from
D-10 to D-20. The three names the spec left to the plan (its Open
questions close) are settled here: the comparison call and its parse call (D-01,
D-03), the rule, option and result values (D-08, E-01 to E-05 of
[data-model.md](./data-model.md)), and the new translation units (D-02).

## D-01: The public comparison surface sits in `benchmark.hpp`

**Decision:** `CompareState`, `Aggregate`, `Direction`, `CompareRule`,
`CompareEntry`, `CompareResult`, `NoiseWarning` and the two calls
`parseCompareRule` and `compareReports` are declared in
`include/speedgun-ng/benchmark.hpp`, beside `ReportData`
(`include/speedgun-ng/benchmark.hpp:553`) and `loadReport`
(`include/speedgun-ng/benchmark.hpp:2330`), and exported with
`SPEEDGUN_NG_EXPORT`.

**Rationale:** the calls answer and consume the public report types that
already live in that header, the same reason 023 put `loadReport` there
(023 plan D-01). A consumer of the gate includes one header. The header
is the only public surface the constitution allows (Principle V.1), and
the harness archive is the library that carries the surface (FR-064).

**Alternatives:** a new public header `include/speedgun-ng/compare.hpp`
splits the report surface across two headers for one feature and gives
the install tree a second header to govern; rejected. A header under
`source/harness/` would hide the public names; rejected.

## D-02: One new translation unit, `source/harness/compare.cpp`

**Decision:** the grammar, the value resolution, the validation pass,
the comparison engine, the console table and the comparison document
writer all live in `source/harness/compare.cpp`, declared in the new
detail header `source/harness/detail/compare.hpp`.

**Rationale:** the harness archive builds from
`GLOB_RECURSE ... "source/harness/*.cpp"` (`CMakeLists.txt:191`), so a
new `.cpp` needs no CMake edit. The rule grammar and the table's rule
column read the same parsed rule, and the document writer prints the
same entries the table prints; one translation unit keeps those three
agreeing by construction, the way 023 kept the writer and reader
agreeing through one detail header (023 D-01). The unit stays well under
`json_load.cpp`'s size.

**Alternatives:** splitting parse from engine from output repeats 023's
two-unit shape, but 023 split at a real seam (file bytes in, file bytes
out) and this feature has none between its parts; rejected as three
units where one closes the contract.

## D-03: The comparison call answers `std::expected<CompareResult, std::string>`

**Decision:** `compareReports(const ReportData& baseline, const
ReportData& contender, const std::vector<CompareRule>& rules, const
CompareOptions& options)` answers `std::expected<CompareResult,
std::string>`. `CompareOptions` carries `strict`, and the settings stand
in one struct so a later setting grows the call with the exported
signature unchanged (Charles, 2026-10-21, the PR #45 review round). It
validates every rule over its scope before comparing anything, and the
first fault stops the call with one error text naming the rule, the
benchmark and the report; no partial entries reach the caller (FR-003).

**Rationale:** the shape is the loader's (023 D-08): the caller decides
what a fault costs, and the command line maps the fault to status 2
while a library caller keeps the text. Validation before comparison is
what makes "no partial entries" true without a second pass.

**Alternatives:** answering entries plus a fault list lets a half table
reach the console, which FR-003 forbids; a `bool` plus an out parameter
is the shape the constitution's contract rules disfavor; rejected.

## D-04: `ReportData` carries the two header texts and the warning table

**Decision:** `ReportData` gains three fields: `schemaVersion` and
`speedgunVersion` (both `std::string`, filled by the load call from the
document header, empty for a value no document produced) and
`noiseWarnings` (`std::vector<std::optional<NoiseWarning>>`, one entry
per entry of `results` when the load call fills it). The report writer
keeps its current behavior: it stamps its own version strings
(`source/harness/json_report.cpp:28`, `:530`) and recomputes the warning
at write time, so the round-trip byte identity of 023 SC-009 stands.

**Rationale:** FR-006 forbids a second reader, so the header texts the
table prints (FR-056) and the version pairing check (FR-051) must
arrive through the load call; the struct is where the load call answers.
The reader already reads the noise record and discards it
(`source/harness/json_load.cpp:729` to `:743`), so the extension stores
what it already parses (FR-070). A run-path `ReportData` leaves all
three fields empty and the writer ignores them.

**Alternatives:** a separate header struct answered by a second call
reopens FR-006; keeping the versions out of `ReportData` forces the
command line to re-read the files; rejected.

## D-05: `NoiseWarning` moves to the public header; the predicate stays internal

**Decision:** the `NoiseWarning` struct moves from
`source/harness/detail/json_report.hpp:137` to
`include/speedgun-ng/benchmark.hpp` with its three fields unchanged
(`spinning`, `runnableCpus`, `sharesOneCpu`). `noiseWarningOf`
(`source/harness/detail/json_report.hpp:154`) stays internal and returns
`std::optional<sg::NoiseWarning>`.

**Rationale:** FR-070 and D-17 put the record type where the public
value can hold it. The predicate reads the affinity mask of the running
process (`source/harness/json_report.cpp:472` to `:481`), a host fact
the library surface should not re-export; the record is data, the
predicate is harness behavior.

**Alternatives:** duplicating the struct in the public header lets the
two spellings drift; rejected. Exporting the predicate widens the
surface past what the spec asks; rejected.

## D-06: The grammar reads a prefixed value name first, escapes inside the label

**Decision:** the parse reads the value name by prefix: text starting
`counter:`, `metric:` or `stat:` carries a label that ends at the first
unescaped `:` or `@`, and the backslash escapes `\:` , `\@` and `\\`
stand inside that label. An unescaped `@` after a `stat:` label ends
the name where the parser reads the aggregate marker. An unprefixed
name runs to the first `@` or `:` and is `time`, `real_time`,
`cpu_time` or `p<point>`.

**Rationale:** PC-7 found no character rule on labels on `master`, so a
label can hold a colon, a comma or an at sign (spec Open questions,
D-15). The escape is the spec's recommendation, carried here as the
assumption with its risk note: a label holding a literal backslash
before one of the three escape characters needs the doubled backslash,
and a label that already uses backslashes this way changes meaning.

**Alternatives:** quoting the label adds a second string syntax to a
five-field grammar; a length-prefixed label is unreadable on the command
line; both rejected.

## D-07: The threshold field takes one order

**Decision:** the threshold field is `N%`, or `abs=X`, or `N%,abs=X`:
the percent first, then the `abs=` marker, one comma between. Any other
spelling is a rule fault.

**Rationale:** D-6 fixes the written form as `5%,abs=12.5`; accepting
both orders doubles the parse branches for a convenience no FR asks
for. One order, one parse, a fault line that names the text.

**Alternatives:** free order costs two optional-field swaps; rejected.

## D-08: The rule value carries its text as given

**Decision:** `CompareRule` holds a `text` field: the exact text the
parse call read. The table's rule column (FR-053), the fault lines
(FR-003, FR-036) and the comparison document's `rules` array print that
text. A caller that builds a rule in code sets the field itself.

**Rationale:** re-rendering a rule from its fields must re-escape
labels, a second grammar-shaped function the feature does not need.
Carrying the text keeps one printed form for every surface, the way the
report carries the label text it was given.

**Alternatives:** rendering from fields keeps the value canonical and
adds an escaping function plus its branches; rejected.

## D-09: Row selection and the statistics-record rule

**Decision:** the value of a benchmark comes from the result's
`AGGREGATE` row when one stands, and from its single measured row when
none stands (`source/harness/runner.cpp:2163`). A named column has a
statistics record when it is the primary time column (`timeStats`,
`source/harness/runner.cpp:2195`), a metric column (`metricStats`,
`:2229`) or a counter column (`counterStats`, `:2262`). An aggregate
named on a column with no record in a repeated run is a missing value
(FR-036, spec D-18 carried); a single-row run answers every aggregate
with that row's value (FR-017). A result with no readable row, a fit
entry or a non-measured outcome, answers `SKIPPED` (FR-011, spec D-20).

**Rationale:** PC-6 read the aggregate row's records on `master`: the
report holds no record for the time column the primary is not, so
spec D-18's recommendation is the only reading that does not invent
a number. The risk note the spec carries: a `real_time@mean` rule over a
repeated run whose primary is cpu faults with status 2; the owner can
overturn it to a computed aggregate, which would need the reader to
carry per-column repetition values.

**Alternatives:** computing an aggregate from the repetition rows in the
gate duplicates the runner's job and breaks on aggregate-only reporting
(FR-016 hides the repetition rows); rejected.

## D-10: Percentile cells answer the primary time column

**Decision:** `p<point>` reads the chosen row's `percentiles` entry for
column 0, the primary time column, whose point list is
`BenchmarkResult::percentilePoints`. The point must stand in both
reports' lists and the cell must be available; each failure is a
missing value (FR-038).

**Rationale:** spec D-16 carried: the report holds cells per column in
positional order (`source/harness/report.cpp:282` to `:296`) and the
grammar cannot carry a column qualifier while a label may hold a colon.
The risk note: a percentile of a metric column is unreachable until the
grammar grows a qualifier; the spec defers that to N7.

**Alternatives:** a `metric/<name>/p99` path is a second grammar;
positional column numbers (`p99#1`) collide with the label colon rule;
both rejected for N5.

## D-11: The noise guard reads the value's own statistics record

**Decision:** for a value with a statistics record, the guard holds
`coefficientOfVariation` against the rule's relative threshold and
`sampleStdDev` against its absolute threshold, in the value's stored
unit (spec D-19, `include/speedgun-ng/benchmark.hpp:231`). Either
report over its bound marks the row `NOISY`. A carried warning
(spec D-17) marks the row by itself, and a metric column marks the
row when either report's `runningRatio` is below one (FR-043). A
percentile or statistic value carries no record, so its guard is the
warning and the running ratio alone (FR-046). `NOISY` replaces the
verdict word for the row; `--compare-strict` moves only the verdict,
never the word (FR-044, FR-045).

**Rationale:** the fields are the ones the report already carries for
the value; the guard adds no computation. The reason string names which
bound fired, so the table answers why (FR-044).

**Alternatives:** a single cv bound for both threshold forms ignores
spec D-19's split of relative against `coefficientOfVariation` and
absolute against `sampleStdDev`; rejected.

## D-12: The batch filter runs in the command-line path before the call

**Decision:** the compare mode applies `--filter` to the two loaded
values (erasing non-matching entries from `results` and the parallel
tables) before calling `compareReports`. The library call carries rule
filters only.

**Rationale:** the batch filter is a property of the run (FR-050); no
rule carries it. Pre-filtering the values keeps one scoping rule inside
the call (rule filter) and matches the edge case's order: the batch
filter first, then the rule answers no match. The regex compile and its
error line reuse the run path's shape (`source/harness/cli.cpp:470` to
`:474`).

**Alternatives:** a fifth parameter on the call splits scoping across
two layers; rejected.

## D-13: The schema version pairing check lives inside the comparison call

**Decision:** `compareReports` compares `schemaVersion` of the two
values first; a mismatch answers the error text naming both versions
(FR-051, D-13, SC-013). The command line prints that text as its one
line with status 2.

**Rationale:** the library surface must answer the same fault a
library caller can cause; putting the check in the command line leaves
the call able to compare mismatched documents.

**Alternatives:** a separate exported check call adds a name the spec
does not ask for; rejected.

## D-14: The comparison document reuses the builder and the schema checker

**Decision:** the document is written in `compare.cpp` with the vendored
`simdjson::...::builder::string_builder`, the writer's tool (023 D-03).
`docs/schema/compare-v1.json` follows the report schema's conventions
(`x-order`, `required`, inlined definitions, `docs/schema/report-v1.json:7`)
and the existing checker `checkReport`
(`source/harness/json_load.cpp:1067`) validates the written bytes
against it: the checker reads any schema file in those conventions, so
no second checker enters the tree.

**Rationale:** FR-057 asks for the schema file and FR-058 for the check
against it; `checkReport` is generic over the schema document already.
The harness
target links the vendored library `PRIVATE BUILD_INTERFACE`, so the new
unit sees the include path with no CMake edit and the install-tree and
symbol privacy of 004 FR-013 to FR-017 stand.

**Alternatives:** a hand-rolled emitter or an in-house checker violates
the constraints; a second checker function duplicates the walk; both
rejected.

## D-15: The entry point carries the compare mode through an early branch with its own postcondition

**Decision:** `speedgunMain` branches to the compare mode right after
the option scan (`source/harness/cli.cpp:464`), before registry
expansion: `const auto exit = detail::runCompareMode(options);` then
`SG_ENSURE((exit.status != 0) == exit.failed, ...)` then `return
exit.status;`. The closing postcondition at `:566` keeps its run-path
predicate.

**Rationale:** the compare mode runs no benchmark, so it must return
before the expansion at `:434` and before the run loop; the branch's
own `SG_ENSURE` is the entry point's postcondition for the mode
(FR-052), and `status` and `failed` are computed independently inside
`runCompareMode` (the verdict and the fault), which gives the ensure
content. `test/cli_shape.sh:31` to `:41` locates the loop by the
`runner.run(*entry)` line, which the branch leaves untouched (PC-11).

**Alternatives:** wrapping the whole run path in an `else` re-indents
130 lines for one branch; a shared flag at `:566` needs the compare
path to fall through the run loop's guards; both rejected.

## D-16: The docs example builds two report values in code and prints the comparison lines

**Decision:** `example/docs/regression_gate.cpp` registers one small
family (the run gate requires a non-empty `--list`,
`test/example_run.sh`) and its `main` builds two `ReportData` values in
code, calls `parseCompareRule` and `compareReports`, prints one line
per entry with its state word, then calls `speedgunMain`. The marked
region the README copies holds the comparison code; the `.expected`
file records the printed lines.

**Rationale:** the run gate runs the binary with `--min-time=0.01` and
checks rows, skips and the exit code; the registered family satisfies
it. Values built in code make the recorded comparison lines
deterministic, which a live two-run pair on a CI machine cannot be. The
example links the harness archive already (`example/CMakeLists.txt:40`
to `:47`).

**Alternatives:** driving `--compare` through the example's own
`speedgunMain` needs two report files with a real difference, which
reintroduces machine noise into the recorded block; rejected.

## D-17: The harness `SOVERSION` moves 8 to 9; the archive stays 2

**Decision:** `CMakeLists.txt:247` moves the harness `SOVERSION` from 8
to 9; the library archive's `SOVERSION` at `:47` stays 2. The project
`VERSION` at `:7` (0.14.0 at the base) takes the next free minor at
rebase time: 0.15.0 or 0.16.0 by the merge order against N6, which
reserves 024 on another machine.

**Rationale:** the feature grows the public surface of the harness
archive and changes the layout of the public struct `ReportData` by
adding fields, so the ABI move is required where 023's was a judgment
(023 D-16). The library archive gains no declaration, so its 2 stands.

**Alternatives:** keeping 8 ships a changed struct layout under an old
soname; rejected.

## D-18: The zero baseline prints `n/a` in the percent column

**Decision:** when the baseline magnitude is zero and the rule carries
an absolute threshold alone, the comparison answers the absolute test
alone (FR-022, FR-027; FR-031 rules out the relative form on a zero
baseline) and the table prints the token `n/a` in the percent
column; the document writes `null` in `changePercent`.

**Rationale:** a percent of a zero baseline has no value; the table
needs a printable token and `null` is the document's existing
unanswered form (023 D-12).

**Alternatives:** printing `inf` reads as a number the gate did not
compute; omitting the column shifts the table's columns per row; both
rejected.

## D-19: An empty pair answers an empty table and status 0

**Decision:** when the two reports share no benchmark name (or both
hold none), the comparison answers an empty entry list, the command
line prints one line saying the pair matched nothing, and the run
answers status 0. The rule-filter no-match fault (FR-039) fires only
for a rule that wrote a non-empty filter.

**Rationale:** the spec's empty-pair edge case states status 0 with a
line; FR-039 names "a rule filter" that matches nothing, and the
default rule's filter is empty. The two readings collide only for an
empty scope with no written filter, and the edge case resolves it.

**Alternatives:** faulting the empty pair contradicts the edge case;
rejected.

## D-20: The test surface reuses `sameValue` and passes fixtures through the test command

**Decision:** the new tests are `harness_compare_test` (parse, engine,
resolution, guard) and `harness_compare_cli_test` (option scan, status,
one-line faults, table, document), each with the three-line target
shape and the `source/harness` include directory
(`test/CMakeLists.txt:569` to `:577`). Fixture report documents live in
a new `test/data/` and reach the tests as a path argument in
`add_test` (`test/CMakeLists.txt:82` shape). Double comparisons reuse
the existing `sameValue` helper
(`test/source/detail/json_fixture.hpp:48`); `==` on a double is a hard
error under `-Werror=float-equal`. No test sleeps, reads a clock or
runs a benchmark; no test names a cpu count or host size; none skips.

**Rationale:** the reader-extension tests join the existing
`harness_json_load_test` target, whose read-back precedent
(`test/source/harness_json_report_test.cpp:41` to `:53`) shows the
file-then-value pattern. Fixtures as committed JSON files exercise the
real reader over the exact bytes the gate consumes.

**Alternatives:** building every fixture as a `ReportData` in code skips
the reader path the extension changes; rejected.
