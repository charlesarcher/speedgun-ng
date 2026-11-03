# Implementation Plan: Loader Cross-Field Checks for the Pass Fields

**Branch**: `047-loader-cross-field` | **Date**: 2026-11-03 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/047-loader-cross-field/spec.md`

## Summary

The reader takes the four pass fields of a report entry one field at a time and
checks their agreement nowhere, so a hand-edited document the writer cannot make
loads with no error and hands wrong figures to the comparison. This lane adds
three pair checks to `readEntry`: the settled method beside the pass count, the
pass spread beside the pass count, and a metric column's pass tag beside the pass
count. Each check runs only when both keys of its pair stand present, so a
document written before specs/024 loads as it does today. Each refusal is one
line in the standing loader shape, and the first fault stops the read.

Rule 1 runs on an entry whose outcome is `MEASURED` and that carries no `hunt`
key. The measurement behind that waiver is in D-14: the runner publishes a count
of 0 beside a settled `MULTIPLEX` or `PASSES` on four stop paths, and a hunt run
names the method the operator forced beside the count its own collection
published. Rules 2 and 3 need no waiver, because the merge writes a pass tag only
above one pass and a spread answers nothing below two pass times.

The writer, the schema file, the schema checker and the comparison stand
unchanged (D-10). The law of the checks lands in spec 024 FR-038 first, in its
own commit (D-8). The version moves 0.30.0 to 0.30.1 (D-2).

## Technical Context

**Language/Version**: C++23 (`cxx_std_23`), the tree's own standard.

**Primary Dependencies**: none added. The reader is `source/harness/json_load.cpp`;
the word tables and `enumWord` are `source/harness/detail/json_report.hpp`, and
the loader already includes that header.

**Storage**: files read at run time only, the report document. No new file, no
new key, no schema change.

**Testing**: frameworkless C++ programs with the `check()` and `fail()`
convention, one executable per file, driven by CTest. The new cases join
`test/source/harness_json_load_test.cpp` and
`test/source/harness_compare_cli_test.cpp`; both targets and their CTest
entries exist (`test/CMakeLists.txt:775-783`).

**Target Platform**: Linux on x86-64, the two release presets `ci-rocky` and
`ci-ubuntu`. The lane host is chilly (AMD Zen 4, Ryzen 9 7950X3D), GCC 16.2.1.
macOS and Windows stay unsupported.

**Project Type**: two shared libraries with one CLI front end. This change
touches the harness library's report reader alone.

**Performance Goals**: none priced. The checks read keys the reader has already
read, in the same pass over the document, and the reader is not a measured path.

**Constraints**: 100 percent line coverage and a branch shortfall of 21 rows or
fewer at the union measurement point (`tools/dbc/coverage_gate.sh:102`), with no
new exclusion construct (D-12, FR-018); no public API change, no `SOVERSION`
change, no schema change, no new error type, no new option (FR-013, D-10); no
test sleeps, reads a clock, names a made-up cpu, or needs a PMU (FR-017).

**Scale/Scope**: one function in one source file, about 40 added lines; nine new
test functions in one test file and one in another; three sentences on one docs
page; one spec amendment; three version sites.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status at plan time | Evidence in this plan |
| --- | --- | --- |
| I, standard-first P0 to P3 | pass | The checks use the reader's own helpers over the standing word tables, and add no dependency. |
| II, Design By Contract | pass | A document is external input, so a disagreement answers a load error, and no `SG_ENSURE` stands where a refusal is honest (D-19, FR-007). |
| III, R-DCUT | pass | Each new test names the requirement it proves. The rule-1 waiver comes from a measured writer form and carries two tests. |
| IV, documentation | pass | Three sentences join the loader section of the page that already states the loader rule (FR-014, D-7). |
| V, style | pass | The lane runs the pinned clang-format 18.1.8 (D-20) and adds no suppression. clang-tidy and cppcheck run in `ci-ubuntu`. |
| VI, coverage | pass | The check shape is measured with `gcov -b -c`: 26 records, about 30 with the guards of D-14, none taken zero (D-19). |
| VII, performance discipline | pass | The checks read keys the reader already read, and touch no measured path. |
| VIII, hard CI gates | pass | The gate commands, the pinned tools and the measurement point stand in the sections below. No CI file changes. |
| IX, spec-driven | pass | Spec, two clarify rounds, this plan, then tasks. Every design point names its decision. |
| X, anti-slop | pass | Three checks in one reader, each one `if`: no new type, no new file, no new mechanism. |
| XI, prose | pass | `cmake/prose-lint.cmake` reports 0 findings over this directory and over the commit range. |
| Pull Request Quality | pass | Every commit names its exact pathspecs, wraps its body at 72 columns, and ends with the two footer lines. |

Two rows carry a detail worth the sentence. No public function changes, so
Principle II writes no contract. The pair rule of FR-002 is the standing shape
of the loader, so Principle X adds no mechanism. The lane's first commit message
failed `CM.BODY-WRAP` at 73 to 78 columns and stands rewrapped at 72, and the
range gate answers 0 findings.
No gate fails, so Complexity Tracking stays empty.

## Project Structure

### Documentation (this feature)

```text
specs/047-loader-cross-field/
├── spec.md                    # Requirements FR-001 to FR-020, SC-001 to SC-005
├── plan.md                    # This file
├── research.md                # Decision, rationale and alternatives per point
├── data-model.md              # The entry, the pairs, the refusal, the accepted forms
├── quickstart.md              # Build, run, acceptance and gate order
├── contracts/
│   └── loader-pass-pairs.md   # The three checks, their guards, their refusal lines
├── checklists/
│   └── requirements.md        # Requirements quality checklist, passed
└── tasks.md                   # Phase 2 output, not written by this file
```

### Source (this feature)

```text
specs/024-multipass-collection/
└── spec.md                    # FR-038 gains the three rules and the pair rule

source/harness/
└── json_load.cpp              # readEntry: three checks after the record reads

docs/pages/
└── harness.md                 # the loader section, three sentences

test/source/
├── harness_json_load_test.cpp # nine new functions, called from main
└── harness_compare_cli_test.cpp # one pairing test, D-21

CMakeLists.txt                 # the version line, 0.30.1
test/source/views_test.cpp     # the pinned version string
test/source/harness_cli_table_test.cpp # the pinned version string
```

**Test duties**: `harness_json_load_test.cpp` carries every refusal of the three
rules, the two waivers of rule 1, the pair rule, the refusal texts, the read
order, and each accepted form: the scripted run's multiplex document, a split
document, a stopped document, a schema 1.0 document, and documents of schema
1.12 and 1.13. `harness_compare_cli_test.cpp` carries the pairing of a pair
refusal with status 2 and one quoted line, over a document of schema major 1
that the lane writes at run time (D-21).

**Structure decision**: the checks sit in `readEntry`, the reader of the entry,
because all four fields of all three pairs are in scope there and in no smaller
place. The rows read at `json_load.cpp:2636-2656`, the pass block reads at
`:2726-2764`, and the hunt record reads at `:2770-2777`. The checks go after the
record reads, at the end of `readEntry`, where the outcome, the method, the
count, the spread, the hunt key and the rows all stand read. No new function
joins the file's public surface, and no helper joins a header.

**Deliberate boundaries**: no edit to `source/harness/json_report.cpp`,
`docs/schema/report-v1.json`, the schema checker, `source/harness/compare.cpp`,
`source/harness/runner.cpp`, `source/harness/cli.cpp`, any public header, any
file under `.github/`, and any file of specs/045 (FR-013, D-10, Scope). The
writer keeps its four keys and its metric tag key exactly as they stand: the
lane refuses documents, it writes none.

## Design

### Decision index

| Decision | Landing point |
| --- | --- |
| D-1 number, branch, version | the branch, `CMakeLists.txt:59` |
| D-2 patch version 0.30.1 | the version commit, three sites |
| D-3 three pairs | the three checks in `readEntry` |
| D-4, D-17 pair rule and presence | the guard of each check |
| D-5, D-15 refusal text | the four refusal strings |
| D-6 schema versions | no version test in the code; the tests name 1.0, 1.12, 1.13 |
| D-7 docs | `docs/pages/harness.md`, the loader section |
| D-8 the law lands first | the first commit, `specs/024-multipass-collection/spec.md` |
| D-9, D-16 one refusal, the order | the position of the checks at the end of `readEntry` |
| D-10 writer, schema, checker, compare unchanged | the diff, and the FR-013 review |
| D-11 accepted forms | the accepted-form tests |
| D-12 coverage, no exclusion | the Coverage plan |
| D-13 the specs/046 overlap | the rebase step of the Gate commands |
| D-14 rule 1 waiver | the two guards of rule 1, and its two tests |
| D-18 the source of each form | the scripted run, and the hand-built split document |
| D-19 the check shape | one `if` per refusal, one `&&` per pair guard, no `switch` |
| D-20 the pinned host tools | the Gate commands |
| D-21 the compare pairing | the one test of `harness_compare_cli_test.cpp` |

### Modules and dependency order

The lane lands five commits, in this order, each atomic and bisectable:

1. `Docs`: spec 024 FR-038 gains the three rules, the pair rule and the rule-1
   waiver. `Refs: specs/024-multipass-collection T-LCF` (FR-001, D-8).
2. `Harness`: the three checks in `json_load.cpp`, the nine load tests, the one
   compare test, and the three docs sentences. Each test stands in the same
   commit as the code it tests (FR-020). The docs lines ride here, because a
   docs page must not state a rule the reader does not enforce.
3. `CMake`: the version move to 0.30.1, the two pinned test sites, and the
   rebuild that follows (FR-015).
4. `Docs`: the acceptance record, the gate verdicts, the coverage figures and
   the pull request body (FR-018, SC-002).

The red step of TDD runs before commit 2: the nine load tests and the one
compare test run against the unpatched reader, and each refusal case fails. The
record names the count of failures.

### Logical view: the check sequence

`readEntry` reads, in order: the required keys, the rows, the labels, the
statistics, the pass block, the hunt record, the top-down record. The checks
append after the record reads, so the standing refusals keep their place and the
first fault the reader reaches stops the read (D-9, D-16).

```text
rows                                    # :2636-2656, unchanged
pass block                              # :2726-2764, unchanged
spreadWarning check                     # :2752-2763, unchanged (FR-009)
hunt record, top-down record            # :2770-2791, unchanged
rule 1  method beside count             # new
rule 2  spread beside count             # new
rule 3  pass tag beside count, per row  # new
```

Rule 1 reads, in words: when the method key and the count key both stand, and
the entry names `MEASURED`, and the entry carries no hunt key, refuse when the
method is `AUTO` and the count is not 0, when the method is `MULTIPLEX` and the
count is not 1, and when the method is `PASSES` and the count is below 1. The
method word of the refusal text comes from `detail::enumWord` over
`detail::kMetricMethodWords`.

Rule 2 reads: when the count key stands and the spread key stands with a
non-null value, refuse when the count stands below 2. The spread's non-null
reading is the one the standing `spreadWarning` check takes at `:2745`.

Rule 3 reads: when the count key stands, walk the rows the entry has read, and
for every metric column whose pass tag stands non-null, refuse when the count
stands below 2, and refuse when the tag stands at or above the count. The tag is
a 0-based pass index. A context block carries no metric value, so the walk
covers `result.rows` alone (FR-006).

Each check is one `if`, and each pair guard is one `&&` of the key tests, so
every operand is its own branch record and every arm has a test (D-19). No
`switch` stands over the three method words: the jump table adds an arm no
document can reach, because the standing kind check refuses a word outside the
three names (`json_load.cpp:2728-2732`).

### The refusal lines

The path names the pair's non-count field, and the words name the partner by its
full path (D-15). Four lines stand, one per refusal arm:

```text
/benchmarks/0/metricMethod: the method 'AUTO' disagrees with the pass count at /benchmarks/0/passCount
/benchmarks/0/passSpread: a pass spread needs a pass count of 2 or more at /benchmarks/0/passCount
/benchmarks/0/rows/0/metrics/0/pass: a pass tag needs a pass count of 2 or more at /benchmarks/0/passCount
/benchmarks/0/rows/0/metrics/0/pass: the pass tag 2 stands at or above the pass count 2 at /benchmarks/0/passCount
```

The shape is the standing one: the path, a colon, the words
(`json_load.cpp:57-58`, `:2759-2761`). No new error type joins the reader
(FR-007). The comparison quotes the line and answers status 2 through the path
it has today (`compare.cpp:1876-1890`, FR-008).

### Physical view

One file, one function, one insertion point:

| Site | Change |
| --- | --- |
| `source/harness/json_load.cpp`, end of `readEntry`, after `:2791` | the three checks, about 40 lines, with the comment that names specs/047 FR-003, FR-005 and FR-006 and the waiver of D-14 |
| `test/source/harness_json_load_test.cpp`, before `main` | nine functions, and nine calls in `main` between `:2207` and `:2234` |
| `test/source/harness_compare_cli_test.cpp` | one function, and one call in its `main` |
| `docs/pages/harness.md`, the loader section | three sentences |
| `specs/024-multipass-collection/spec.md`, FR-038 | the three rules, the pair rule, the waiver |
| `CMakeLists.txt:59`, `test/source/views_test.cpp:279`, `test/source/harness_cli_table_test.cpp:1453` | 0.30.1 |

`SG_PROJECT_VERSION` is a `PRIVATE` define of the harness target
(`CMakeLists.txt:323-327`), so the version commit rebuilds the harness library
whole and relinks every harness test (D-20 of the clarify record, FR-015).

### Public API surface added

None. No header changes, no exported symbol changes, no option changes, no
error type changes. The two `SOVERSION` numbers stand: harness 20, archive 5.
The schema version stands at 1.13, and `docs/schema/report-v1.json` lists every
key of every pair already (`:517-520`, `:654-662`).

### Risks

| Risk | Handling |
| --- | --- |
| specs/046 touches the same reader and the same test file | D-13: the second lane to merge rebases, keeps both sides after reading them, and runs every gate again. |
| The two guards of D-14 add branch arms | Each arm carries a test: a measured document, a stopped document, a hunt document, and the pair-guard arms (D-19). |
| The host coverage reading exceeds the allowance | The allowance binds the union of a full suite (`coverage_gate.sh:86-88`), and the lane reports the host reading beside it (FR-018, SC-002). |
| The load-sensitive FWQ rows of `harness_jitter_window_test` fail on a loaded host | The pinned run `taskset -c 0,1 ctest` is the record, and the known base failure stands named in the acceptance record. |
| A planted compare document becomes a committed fixture a later rule refuses | The compare test writes its document at run time, as the file does today; no fixture joins `test/data` (D-21). |
| The rule-1 waiver reads as a hole in the law | Spec 024 FR-038 states the waiver with its reason, so the law and the reader agree (FR-001). |

The host reading at the base of this lane is 99.5 percent lines and 98.8 percent
branches, because two coverage entries fail on this host today. The lane records
the host delta of `json_load.cpp` beside the union figure, in the pull request
body.

## Test Plan

### FR coverage map

| Requirement | Test |
| --- | --- |
| FR-001 the law lands first | the first commit, and the review of its diff |
| FR-002, FR-004 the pair rule | `aPassPairWithOneKeyAbsentLoads` |
| FR-003 rule 1, and the D-14 waiver | `thePassMethodRefusesItsCount`, `aStoppedEntryKeepsItsMethodAndCount`, `aHuntEntryKeepsItsMethodAndCount` |
| FR-005 rule 2 | `thePassSpreadRefusesItsCount` |
| FR-006 rule 3, per row, 0-based | `thePassTagRefusesItsCount` |
| FR-007 the refusal text | `thePassPairRefusalNamesItsPartnerPath` |
| FR-008 the compare pairing | `aPassFieldDisagreementAnswersOneQuotedLine` in `harness_compare_cli_test.cpp` |
| FR-009 the standing warning check keeps its place | `thePassPairChecksRunInReadOrder`, beside the standing case at `:2752` |
| FR-010, FR-011, FR-012 the accepted forms | `everyWrittenPassFormLoads`, `aStoppedEntryKeepsItsMethodAndCount`, beside the standing `aDocumentPredatingThePassFieldsLoads` |
| FR-013 writer, schema, checker, compare unchanged | the diff review, and the standing schema tests of `harness_json_schema_test.cpp` |
| FR-014 the docs sentences | the drift gate, and the review of the count |
| FR-015 the version move | `views_test.cpp:279`, `harness_cli_table_test.cpp:1453` |
| FR-016, FR-017 the test count and the test facts | this map, and the case names in the load test's log |
| FR-018 coverage | the Coverage plan |
| FR-019 the gates | the Gate commands |
| FR-020 the commit template | `python3 tools/prose/prose_gate.py --check all --mode range --base origin/master` |

Nine new functions join the load test, which runs 27 today
(`harness_json_load_test.cpp:2205-2238`), and one joins the compare test. No
target and no CTest entry changes.

### Coverage plan

The measured arm counts, taken with the coverage flags of the `coverage-linux`
preset and `gcov -b -c` on this host: 8 records for a pair guard plus a min/max
pair per method, 6 for rule 2, 12 for rule 3, none taken zero. The two guards of
D-14 add four. Every arm has a test in the map above, so the change adds no dark
arm (D-19).

The measurement point is the coverage job's union of a full suite
(`tools/dbc/coverage_gate.sh:86-88`), where the branch shortfall reads 0 rows
today (`specs/043-fwq-ftq/plan.md:122`) against an allowance of 21. The host
reading at the base of this lane is 99.5 percent lines (17484 of 17565), 97.3
percent functions and 98.8 percent branches (8593 of 8701), because two coverage
entries fail on this host today. The lane records the host reading at the base
and at the head, the reading of `json_load.cpp` at the head, and the union
figure from the coverage job's artifact, all in the pull request body (FR-018,
SC-002).

No exclusion construct joins the tree: no `LCOV_EXCL`, no `--omit-lines`, no
`--ignore-errors` in a project file, no `--remove` (D-12). The host `lcov` needs
`PERL5LIB` for `Capture::Tiny`, and the coverage commands of the quickstart
carry it (D-20).

### Gate commands

```bash
cmake --preset=ci-rocky && cmake --build build
ctest --test-dir build --output-on-failure
taskset -c 0,1 ctest --test-dir build --output-on-failure
cmake -D FORMAT_COMMAND="$HOME/.local/bin/clang-format" -P cmake/lint.cmake
cmake -P cmake/spell.cmake
python3 tools/docs/example_drift.py .
python3 tools/prose/prose_gate.py --check all --mode range --base origin/master
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/047-loader-cross-field
cmake --preset=ci-ubuntu && cmake --build build   # clang-tidy and cppcheck once per feature
cmake --preset=ci-coverage
cmake --build build/coverage -j "$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage --target coverage
bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
```

The pinned tools: clang-format 18.1.8 at `~/.local/bin` (CI installs the same
version by pip, `.github/workflows/ci.yml:33-34`), and `PERL5LIB` carrying
`Capture::Tiny` for the host `lcov` (D-20). The dbc pair gate runs through its
own target, `python3 tools/dbc/dbc_pair_gate.py`.

## Constitution Check: design re-check

Passed. The design adds no public type, no header, no dependency, no option, no
CI file and no configuration knob. It adds three checks in one reader, ten test
functions in two existing files, three sentences on one page, one spec
amendment and one version move. The contracts add no gate, and the data model
adds no carrier: every field the checks read already stands in
`BenchmarkResult`, and the change stores nothing.

## Complexity Tracking

No violations. The one simpler shape available and refused is recorded in
`research.md`: a check that reads the method word beside the count for every
entry, which the writer's stopped and hunt documents refuse, measured at
`runner.cpp:2884` and `cli.cpp:1788-1789`.
