# Research: JSON Reports

**Feature**: `specs/023-json-reports/spec.md` | **Date**: 2026-10-20
**Anchors**: read at `c7f1769`, the head of `master` this branch cuts from
**Numbering**: this file's D-01 to D-17 are the plan's decisions. The spec
numbers its own decisions D-1 to D-13, and a reference to one of them
names it as spec D-nn.

## D-01: One translation unit carries the writer and the reader

**Decision**: `source/harness/json_report.cpp` holds the document writer,
the document reader, the enum name tables and the key lists.
`source/harness/detail/json_report.hpp` declares them for `cli.cpp` and
the tests.

**Rationale**: a format has one key order and one set of enum spellings.
Putting both directions in one file makes that agreement one list, and a
key added on one side has its partner in the same file. The harness
archive globs `source/harness/*.cpp` (`CMakeLists.txt:191`), so the new
unit needs no source-list edit, and the coverage, sanitizer and DBC gates
already run over that archive.

**Alternatives considered**: a writer unit and a reader unit, rejected
because the two key lists drift apart and the schema checker then reads a
document neither side owns; a `report_json.cpp` beside `report.cpp`,
rejected because `report.cpp` is the console printer and N4 topology
placement edits it, so a shared file would put two features in one diff.

## D-02: The harness target takes the vendored headers as a build-only link

**Decision**: add `simdjson::simdjson` to `speedgun-ng_harness` as
`PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>` beside the existing link
at `CMakeLists.txt:197`.

**Rationale**: `specs/004-vendor-simdjson` FR-013 allows the vendored
headers only in `source/` translation units and asks for one wrapper unit;
`json_report.cpp` is that unit. `$<BUILD_INTERFACE:>` keeps the include
path out of the installed interface, so specs/004 FR-014, FR-015 and
FR-016 hold: the install
tree, the package files and the dynamic symbol table stay free of
simdjson. The counters archive already links the same target the same way
(`CMakeLists.txt:161`), and `simdjson_archive_merge` (`:170`) folds the
parser object into the archive that needs it.

**Alternatives considered**: a separate `speedgun-ng_json` object library,
rejected as a fourth target for one file; `PUBLIC` linking, rejected
because it would leak the vendored include path into the harness
interface and fail the purity scan; compiling simdjson into the harness by
hand, rejected because the import target exists.

## D-03: The writer uses `string_builder`, not the reflection API

**Decision**: build the document with the imperative
`simdjson::builder::string_builder` calls (`start_object`, `append_key`,
`append`, `append_raw`, `end_object`), and keep the key order in the call
order.

**Rationale**: the document mirrors the repository's result types, whose
field order is the order N5 reads, and the imperative builder writes keys
in the order the code appends them. The reflection API of simdjson 4.6
builds objects from a struct with a compile-time field map, which would
put the JSON shape in a macro over the public types and make the public
layout the format. The public layout must stay free to change (D-07).

**Alternatives considered**: `nlohmann::json` or a hand-rolled writer,
both rejected by the no-new-library rule and by FR-048; the reflection
builder, rejected for the layout coupling; `std::print` over a format
string, rejected because escaping and the number format then sit in the
test's expectation.

## D-04: The reader uses the DOM parser, one parse per file

**Decision**: read the file into a `simdjson::padded_string`, parse once,
and walk the object tree into the result types.

**Rationale**: a report is a batch artifact a human or a script reads
after the run, its size is the run's row count times its column count,
and the DOM parser answers that in one pass with the array and object
indexing the loader needs. The streaming reader exists for inputs that do
not fit in memory; no report of this shape does.

**Alternatives considered**: the simdjson streaming reader, rejected as
more code for an input that fits; a line-oriented reader, rejected because
the format is JSON and the checker in D-05 already parses it.

## D-05: The schema file is a JSON Schema document, and the checker reads it

**Decision**: `docs/schema/report-v1.json` is a JSON Schema 2020-12
document. `test/source/harness_json_schema_test.cpp` parses it with the
same vendored parser and checks three things over a written document: the
key set of every object matches the schema's `properties`, the key order
matches the schema's declaration order, and every value's kind matches
the declared `type`.

**Rationale**: FR-008 asks that a written report validate against the
schema, and the repository ships no validator. Walking `properties` in
declaration order is about forty lines over the DOM, and it enforces the
key order the format promises, which a general validator would not check.
Keeping the schema as a real JSON Schema document keeps it useful to
outside tools.

**Alternatives considered**: vendor a validator, rejected by the
no-new-library rule; a hand-written table in the test, rejected because it
drifts from the schema file a reader depends on; no schema file, rejected
by FR-008 and by the loader's version rule.

## D-06: Enum spellings in the file follow the C++ enumerators

**Decision**: the file writes `MEASURED`, `SKIPPED`, `FAILED`,
`REPETITION`, `AGGREGATE`, `COUNTABLE`, `PERMISSION_BLOCKED`,
`NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`, `GAP`, `SPIN`, `YIELD`,
`BLOCK`, `NANOSECONDS`, `MICROSECONDS`, `MILLISECONDS`, `SECONDS`, `TIME`,
`PERCENTAGE`. One table per enum lives in `json_report.cpp`.

**Rationale**: the console printers keep lowercase spellings of their own
(`availabilityName` at `source/harness/report.cpp:35`, `waitModeName` at
`:138`), and those strings are the console's contract, edited by N4 and
quoted by the docs examples. The file's reader needs a table either way,
and the enumerator spelling is the one the public headers already name, so
a value that round-trips needs no second vocabulary.

**Alternatives considered**: reuse the console spellings, rejected because
they are a report contract other features edit; write the underlying
integer, rejected because a report a human opens must name its outcome.

## D-07: The raw chunk columns travel beside the result

**Decision**: `RawColumns` holds one `std::vector<std::vector<double>>`
per reported column, in the column order of the row it belongs to. The
`Runner` keeps a `RawColumns` per row and per context when the raw setting
asks for chunks, filled from `ContextFolds::chunkValues`
(`source/harness/runner.cpp:329`, filled at `:1464` to `:1512`) and from
the single-context local at `:271`. `ResultRow` and `BenchmarkResult`
gain no field.

**Rationale**: the chunk columns are the run's residue: no console row
reports them. N1 dropped them after `buildPercentileColumns` read them, so the
writer has nothing to write today (spec D-12). Keeping them beside the
result leaves the public layout, the DBC pair gate and the console printer
untouched, and it leaves N4 free to edit the printers without touching the
format.

**Alternatives considered**: a `chunkColumns` field on `ResultRow`,
rejected because it moves a public struct layout for a value no console
row prints and would take the `SOVERSION` move on the struct rather than
on the new call; a second return value from `Runner::run`, rejected
because every caller of that call changes; a file the writer re-reads,
rejected as I/O in the middle of a run.

## D-08: The load call answers `std::expected<ReportData, std::string>`

**Decision**: `loadReport(const std::string& path)` in
`include/speedgun-ng/benchmark.hpp` answers
`std::expected<ReportData, std::string>`, where `ReportData` holds the
`std::vector<BenchmarkResult>` plus the `RawColumns` of each row and
context.

**Rationale**: a bad file is an input error, and a contract abort would
end the caller's process over a typo in a path. `std::expected` is the
C++23 standard answer and the constitution's standard-first rule takes it.
`ReportData` is the writer's input type, so the loader answers exactly the
value the writer consumed and FR-026's byte-identical round trip has
somewhere to keep the raw columns (clarification session 2026-10-20,
question 3).

**Alternatives considered**: `std::optional`, rejected because the reader
must name why a file failed; an exception, rejected because the harness
builds with no exception path on the run side and the caller then has no
visible failure channel; out-parameters, rejected as pre-C++23.

## D-09: The writer runs once, after the last console row

**Decision**: `speedgunMain` calls the writer after the last
`fitForFamilyEnd(nullptr)` and before the status postcondition
(`source/harness/cli.cpp:418-426`).

**Rationale**: the console prints one result as its run finishes, so a
write placed after the run loop holds every result of the batch, and the
interrupt path reaches the same point because the loop ends on the
interrupt flag (FR-004, clarification question 5). One call means one
file, one open, one close.

**Alternatives considered**: a write inside the per-result loop, rejected
because the format is one document with one array; a write from a
destructor, rejected because the exit status then cannot see the failure;
a write before the fit rows, rejected because the fit rows belong to the
result the file carries.

## D-10: A failed write answers status 2

**Decision**: a report the writer cannot write sets the process status to
2, and that status wins over a failed benchmark's 1. The postcondition at
`source/harness/cli.cpp:423` gains the third case, and
`specs/015-benchmark-harness-core/contracts/cli.md` gains the row.

**Rationale**: the existing postcondition reads `(status != 0) == (anyFailed
|| anyInterrupted)`, so a status of 2 with nothing failed breaks it unless
the contract says otherwise. Status 2 is the option-error status the same
file already answers (`cli.cpp:340`), and a report the user asked for and
did not get is a usage-level failure: the run's measurement stands, but the
artifact they asked for is missing. FR-030 and FR-050 carry it
(clarification question 2).

**Alternatives considered**: status 1, rejected because a caller could not
tell a failed benchmark from a missing report; status 3, rejected because
no table in the repository names it; a warning and status 0, rejected
because a script that then reads no file has no signal.

## D-11: The noise warning fields come from the console's own predicate

**Decision**: the file writes `noiseWarning` as
`{"spinning": bool, "runnableCpus": n, "oversubscribed": bool}`, computed
by the same call the console check uses: `selectRunnableCpuCount`
(`source/harness/report.cpp:203`, declared at
`source/harness/detail/internal.hpp:278`).

**Rationale**: FR-017 asks that the file carry the condition the console
check applies, with the two counts that check prints. The decision half
of that rule, `selectRunnableCpuCount`, already stands in
`source/harness/detail/internal.hpp:278` and the writer calls it. The
counting of spinning contexts and the adjacent-placement test stand in the
anonymous namespace of `source/harness/report.cpp:229`, and FR-042 and
FR-049 keep that console printer out of this feature, so the new unit
carries its own copy of those two lines. The covering test compares the
file's three fields with the warning line of the same run, so a drift
fails a test; a reader would have to notice it first.

**Alternatives considered**: recompute the count in the writer, rejected
as the drift; omit the fields, rejected by FR-036; write only the boolean,
rejected because a reader triaging a slow run wants the count it did not
see on the console.

## D-12: An unavailable value is `null` inside a two-field cell

**Decision**: every percentile and every availability-gated value writes
`{"available": true, "value": <number>}` or
`{"available": false, "value": null}`.

**Rationale**: N1 already draws the line between a value the run cannot
answer and a value it interpolated (FR-011 of 021: nothing is
interpolated). A bare `null` loses the reason the schema can name; a
missing key loses the position in the column list. The cell keeps the
column position and the reason together, and the loader maps `null` back
to `available` false with no second rule.

**Alternatives considered**: `null` alone, rejected because the schema
then cannot say the key is required; the string `"not available"` as the
console prints, rejected because a number field must stay a number field;
omitting the key, rejected because a reader cannot tell absent from
unanswered.

## D-13: Version and build type come from the compile definitions

**Decision**: the header writes `speedgun.version` and
`speedgun.buildType` from `SG_PROJECT_VERSION` and `SG_BUILD_TYPE`, the
definitions `CMakeLists.txt:203-208` already pass.

**Rationale**: FR-005 forbids a clock read in the writer, and a run date
would make every report of the same run differ in bytes, which breaks
FR-011's byte rule. The two definitions already exist and name the build
that produced the file.

**Alternatives considered**: a timestamp, rejected by FR-005 and FR-011;
the host name, rejected for the same reason; nothing, rejected because a
reader comparing two reports wants the build that made each.

## D-14: `--json-raw=chunks` widens the capture gate

**Decision**: the gate that turns chunk capture on reads the percentile
output or the raw setting. The plan keeps the chunk plan, the recorder
capacity and the cursor cycle exactly as 021 settled them.

**Rationale**: FR-020 asks that a run with the percentile output off still
capture the chunk values when the raw setting asks for them. 021 gated
capture on the output, so the change is one condition on the cold path
that picks the plan. The per-iteration cycle keeps the shape 021 settled.
The loop
gate and the null-body cost therefore stay at the `c7f1769` value.

**Alternatives considered**: always capture, rejected because a run that
reports nothing would pay the recorder capacity for nothing; capture only
with the output on and refuse `--json-raw=chunks`, rejected by FR-020.

## D-15: The docs example is a writer-only example

**Decision**: `example/docs/json_reports.cpp` runs a scripted family and
names the fields its own result sentence reads back; `add_docs_example`
takes `json_reports` (`example/CMakeLists.txt:40`, the list at `:50-70`),
and the README gains one section titled `JSON reports` with its region,
its two quick-reference rows and the `docs/pages/harness.md` detail.

**Rationale**: the drift tool maps a section title to its owner example
(`tools/docs/example_drift.py:30`, `SECTION_OWNERS`), and one example per
section is the rule. A loader example would need a file on disk inside the
example run, which the runner does not stage.

**Alternatives considered**: two examples, rejected by the one-section
rule; a shell example beside the C++ one, rejected because `command_line`
already owns the shell shape and the option table quotes it; no example,
rejected by Principle IV.1's runnable-snippet rule.

## D-16: The harness `SOVERSION` moves 7 to 8

**Decision**: the harness archive `SOVERSION` moves from 7
(`CMakeLists.txt:225`) to 8, and the project version takes the next free
minor. The `speedgun-ng` archive `SOVERSION` stays 2.

**Rationale**: `loadReport` and `ReportData` are public API in the harness
archive, and the constitution's Refactoring and Evolution rule makes a
public API change a versioned change. No exported struct layout moves,
because the raw columns sit in the new type and not in `ResultRow`
(D-07), so the move comes from the added call.

**Alternatives considered**: leave `SOVERSION` alone, rejected because a
link against the old archive would accept a call it does not have; move
the `speedgun-ng` `SOVERSION` too, rejected because the counters library
gains nothing.

## D-17: `RunOutcome` has three values

**Decision**: the file writes the three outcomes of
`include/speedgun-ng/benchmark.hpp:76-81`: `MEASURED`, `SKIPPED`,
`FAILED`. An interrupted run reports as skipped, the rule
`specs/015-benchmark-harness-core/contracts/cli.md:103-108` already states.

**Rationale**: the spec's edge-case bullet named `DISABLED` and
`INTERRUPTED`, and neither enumerator exists. The analyze pass of
2026-10-20 rewrote the bullet to the three that do, so the schema's
`enum` list and the writer's table agree with the header.

**Alternatives considered**: add the two enumerators, rejected as a public
API change no requirement asks for; write the interrupt as a fourth
outcome, rejected because it would split one state into two spellings the
console does not print.
