# Implementation Plan: Hunter Sensitivity and Baseline Delta

**Branch**: `040-hunter-sensitivity` | **Date**: 2026-10-30 |
**Spec**: [spec.md](./spec.md)

**Input**: Feature specification from
`/specs/040-hunter-sensitivity/spec.md`

## Summary

The hunt gains the two figures the roadmap's N14b names: the
sensitivity rank (the Spearman rank correlation and the least-squares
slope of an event's rate against the time per iteration, over one
benchmark family's entries of one run) and the delta rank (the signed
relative change of an event's median rate against a baseline document
loaded through `loadReport`, matched by the specs/026 rules). The
console carries the two columns, `--hunt-rank` orders by them,
`--hunt-emit-rules` writes the promoted events as N5 rules the
comparison reads again, the JSON carries the figures at schema minor
1.9, and the version stands at `0.26.0` with harness `SOVERSION` 17.

## Technical Context

**Language/Version**: C++23, CMake 3.20, the standing presets.
**Primary Dependencies**: none beyond the vendored set (hard rule: no
new runtime dependency).
**Storage**: N/A (documents in, documents out).
**Testing**: CTest, the `dev` preset for the loop, the coverage,
sanitizer and tsan presets for the gates.
**Target Platform**: Linux, GCC and Clang; the acceptance host gary
(AMD Zen 5).
**Project Type**: library plus CLI (the standing layout).
**Performance Goals**: the ranks add no pass, run no second
collection and read no clock; the rank computation stands pure over
collected data.
**Constraints**: the README section stands inside its 25-line cap;
the option values stand the hunt block's reserve; the landings
serialize on the shared files the spec names.
**Scale/Scope**: two option rows, three record fields, one public
call, two statistics helpers, the rank stamping, the JSON keys, the
docs region and the evidence records.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after design at
the foot of this file.*

- I Standard-first: the new code stays inside the pinned Core
  Guidelines baseline; clang-tidy and cppcheck run over it in the
  `ci-ubuntu` build.
- II DBC: every new public and detail interface carries doxygen
  `\pre`/`\post`/`\invariant` and enforces them through the
  contract facility; the contracts land in one place each.
- III R-DCUT: this plan is the design artifact; the test plan below
  records TDD as the execution mode.
- IV.1 Main page: the `Hunter mode` section, its example region and
  the quick-reference rows move in the same change (FR-040, FR-041).
- V.1 Naming: the new identifiers follow the standing camel-case
  rule; the name-check gate runs in CI.
- VI Tests: every requirement carries at least one test (FR-051);
  the tests stand deterministic and fixture-fed, no sleep, no clock.
- VII Performance: the ranks stand pure computations; no timing path
  changes.
- VIII Gates: the convergence wave runs the full gate list (FR-053).
- IX Spec-driven: the four artifacts stand or land under
  `specs/040-hunter-sensitivity/`.
- X Anti-slop: the modules below touch only the files the feature
  needs; no speculative generality, no second implementation of
  anything the standing code carries (D-8, R-2, R-4).
- XI Prose: every artifact passes the prose gate before its commit.

## Project Structure

### Documentation (this feature)

```text
specs/040-hunter-sensitivity/
├── spec.md              # the specification (landed)
├── plan.md              # this file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output
│   └── hunt-ranks-api.md
├── checklists/          # the spec quality checklist (landed)
├── tasks.md             # Phase 2 output (/speckit.tasks)
└── results/             # the gary evidence records (FR-052)
```

### Source Code (repository root)

```text
include/speedgun-ng/hunt.hpp            # M-A: the record fields,
                                        #   toCompareRules
source/harness/detail/statistics.hpp    # M-B: spearman, slope
source/harness/statistics.cpp           # M-B: the definitions
source/harness/detail/hunt_ranks.hpp    # M-C: the stamping surface
source/harness/hunt_ranks.cpp           # M-C: the definitions
source/harness/hunt.cpp                 # M-E: the batch stamping
source/harness/hunt_cli.cpp             # M-E: the rank keys, the
                                        #   refusals
source/harness/options.hpp/options.cpp  # M-E: the two rows
source/harness/json_report.cpp          # M-D: the writer keys
source/harness/json_load.cpp            # M-D: the reader keys
docs/schema/report-v1.json              # M-D: schema 1.9
test/source/harness_hunt_rank_test.cpp  # the rank rows
test/source/harness_hunt_json_test.cpp  # the JSON rows
test/source/harness_hunt_cli_test.cpp   # the CLI rows
test/source/harness_cli_table_test.cpp  # the uniqueness rows
example/docs/hunt_ranks.cpp             # M-F: the docs region
README.md                               # M-F: the section, the rows
CMakeLists.txt                          # M-A: version, SOVERSION
```

**Structure Decision**: the feature stays inside the standing hunt
surface. The figures stand one statistics surface (M-B), one pure
stamping module (M-C), and thin wiring in the CLI, the run path and
the document writer (M-E, M-D). No new target joins: the test rows
join the standing test binaries, the way specs/033's rows joined the
standing hunt binaries.

## Design

### Decision index

| Ruling / question | Decision |
| --- | --- |
| R-1 one baseline reader | D-6: `loadReport` alone; the CLI loads once and hands the extracted rates down |
| R-2 matching named once | the entry matcher stands one implementation, shared with the compare mode: `matchPair`'s name rule moves to a shared detail helper the compare calls and the delta reuses |
| R-3 one rules grammar | `toCompareRules` emits through the standing `CompareRule` text form; no second writer |
| R-4 one statistics surface | Spearman joins `sg::detail` beside `statisticsOf`; the slope reuses the complexity fit's least-squares sums, moved beside it so one implementation stands |
| Clarify Q1 the rule threshold | the run's noise threshold; the direction from the measured delta's sign, `lower` where no delta stands (D-7) |
| Clarify Q2 delta without baseline | refusal, one line, exit 2 (FR-011 family) |
| Clarify Q3 cross-norm baseline | refusal beside FR-005, one line naming the mismatch, exit 2 |
| Sensitivity points | one point per family entry of one run: the entry's aggregate median time per iteration, the event's normalized rate (D-2) |
| Rank order under the new keys | `rankEvents` keeps its standing keys; the batch stamping pass stamps the figures and assigns the ranks under the requested key (D-3) |

### Modules and dependency order

The feature splits into seven modules; each carries its public
surface, its contracts and its test plan, and each lands apart. The
brace marks a parallel group.

```text
1. M-A  the public surface and the version bindings
2. { M-B  the statistics helpers
     M-D  the JSON keys and the schema minor }   # parallel
3. M-C  the rank figures (pure stamping)
4. M-E  the CLI, the console and the run path
5. { M-F  the docs and the example
     M-G  the evidence records }                 # parallel
```

- **M-A the public surface and the version bindings**
  (`include/speedgun-ng/hunt.hpp`, `source/harness/detail/options.hpp`,
  `source/harness/detail/options.cpp`, `CMakeLists.txt`). Public
  surface: `RankedEvent` gains `sensitivityCorrelation`,
  `sensitivitySlope` and `delta`, each `std::optional<double>`, and
  `EntryHunt` gains `toCompareRules(std::size_t topN, double
  noisePercent)` beside `toCounterSet`, the pair-shape the spec names
  (FR-024, D-1). The bindings: the two option rows at `281` and `282`
  (FR-020), the version at `0.26.0`, harness `SOVERSION 17`, archive
  `SOVERSION 4` (FR-050), `kSchemaVersion` at `1.9` (FR-032's
  constant half). Contracts: hunt-ranks-api.md A-1, B-1. Test plan:
  the version and schema assertions of the standing test
  binaries (wave 1) and the CLI table uniqueness rows. Nothing builds before M-A.
- **M-B the statistics helpers**
  (`source/harness/detail/statistics.hpp`,
  `source/harness/statistics.cpp`, the complexity fit's sums moved
  beside them). Detail surface: `spearmanCorrelation` over one point
  set, answering an optional double: the correlation with average
  ranks at ties, nothing when either coordinate stands without
  spread; `leastSquaresSlope` over the same point shape, reusing the
  complexity fit's sums under one implementation (D-8, FR-006).
  Contracts: hunt-ranks-api.md B-2. Test plan: the statistics rows of
  `harness_hunt_rank_test` (wave 2): the monotone set at 1, the
  anti-monotone set at -1, the tie handling, the no-spread case, the
  exact-line slope, the reuse rows. Depends on M-A alone, so it lands
  beside M-D.
- **M-C the rank figures** (`source/harness/detail/hunt_ranks.hpp`,
  `source/harness/hunt_ranks.cpp`). Detail surface, all pure
  functions over the collected records: the family grouping of the
  run's entries by suite and case name; the point assembly per event
  address (FR-001, D-2); the sensitivity stamp
  (`sensitivityCorrelation`, `sensitivitySlope`, `n/a` under 3
  points, FR-002); the baseline rate extraction from a loaded
  document's per-entry hunt objects and the norm-pair check (FR-003,
  FR-005, D-4, D-6); the delta stamp with the `n/a` disclosures
  (FR-004, D-5); the rank assignment under
  `--hunt-rank=sensitivity|delta` with the noisy events unranked
  (FR-011, D-3). Contracts: hunt-ranks-api.md B-3 to B-6. Test plan:
  the rank rows of `harness_hunt_rank_test` (wave 3), one scenario
  block per function. Depends on M-B.
- **M-D the JSON keys and the schema minor**
  (`source/harness/json_report.cpp`, `source/harness/json_load.cpp`,
  `docs/schema/report-v1.json`). The per-entry hunt object gains
  `correlation`, `slope` and `delta` in its `x-order`, null for a
  figure the run did not measure, the closed-node rule kept (FR-030);
  the reader reads them back equal (FR-031); the schema file gains
  the keys at minor `1.9` (FR-032). Contracts: hunt-ranks-api.md
  B-7. Test plan: the round-trip rows of `harness_hunt_json_test` and
  `harness_json_load_test` (wave 4). Depends on M-A,
  so it lands beside M-B.
- **M-E the CLI, the console and the run path**
  (`source/harness/hunt_cli.cpp`, `source/harness/hunt.cpp`,
  `source/harness/options.cpp`, the console printer). The CLI: the
  rank keys `sensitivity` and `delta` accepted, `score` alone
  refused (FR-010); the two rows wired with their refusals: an empty
  value, `--hunt-emit-rules` beside no hunt, `--hunt-baseline`
  beside no hunt, `--hunt-rank=delta` beside no `--hunt-baseline`,
  each one line and exit 2 (FR-022, FR-023, the clarify Q2 ruling).
  The run path: the baseline loaded once through `loadReport` before
  the entries run, the extracted rates and the norm check handed to
  the batch stamping pass, the figures stamped after every entry's
  hunt stands ranked and before the console table prints and the
  document writes, the rules file written through `toCompareRules`
  when `--hunt-emit-rules` stands (FR-021, FR-003, FR-005). The
  console: the two columns beside the standing four, `n/a` cells,
  the matching-rules disclosure printed once (FR-012, FR-004).
  Contracts: hunt-ranks-api.md B-8, B-9. Test plan: the CLI rows of
  `harness_hunt_cli_test` (wave 4). Depends on M-C.
- **M-F the docs and the example** (`README.md`,
  `example/docs/hunt_ranks.cpp`, `example/docs/hunt_ranks.expected`,
  `CMakeLists.txt`). The section's sentences teach the two ranks
  inside the 25-line cap, the marked snippet comes from the new
  region, the quick-reference rows join for the two options (FR-040);
  the drift gates carry the region and the rows (FR-041). Test plan:
  the drift gates (wave 6). Depends on M-E.
- **M-G the evidence records** (`specs/040-hunter-sensitivity/results/`).
  One record per run on gary naming the commands, the exit codes and
  the artifacts, the quickstart linking each (FR-052). Test plan:
  the records' exit codes stand 0 and the links resolve (wave 7).
  Depends on M-F.

### Risks

- The family grouping rides the standing suite/case fields of the
  run's results; a family of one entry stands the `n/a` disclosure,
  never an error (FR-002).
- The matcher move (R-2) touches `compare.cpp`; the compare tests
  stay the guard: the compare rows run unchanged in every wave's
  full-suite pass.
- The document keys move a closed node; the writer, the reader and
  the schema move in one commit, and the round-trip rows pin the
  equal-values rule (FR-031).
- The `n/a` cells stand a fixed-width console change; the table
  width rows of the standing CLI tests pin the column count.

### Logical view: the rank path

```text
per entry:  runHunt(entry, options)      -> EntryHunt (rate, cv,
                                            noisy, standing ranks)
after the batch:
  familyPoints(entries of one suite+case, address)
      -> [(timePerIter, rate)] per shared address
  spearmanCorrelation(points), leastSquaresSlope(points)
      -> sensitivityCorrelation, sensitivitySlope  (n/a < 3 points)
  baselineRates = extract(loadReport(--hunt-baseline))
      -> {name: {norm, address: rate}}
  normCheck(contenderNorm, baselineNorm) -> refuse, 2 on mismatch
  delta(entry, baselineRate) -> 100 x (c - b) / b, n/a disclosures
  rankByFigure(entries, key) -> ranks under the key, noisy unranked
console: the two columns   JSON: correlation, slope, delta
promotion: toCompareRules(top, noise) -> N5 rule lines -> file
```

### Physical view

- The public surface stands in `include/speedgun-ng/hunt.hpp`,
  namespace `sg::hunt`, exported through the standing export header.
- The detail surface stands under `source/harness/detail/`; the
  definitions compile into the `speedgun-ng_harness` target beside
  the standing hunt files.
- The test rows join the standing binaries:
  `harness_hunt_rank_test` carries the statistics and rank rows,
  `harness_hunt_json_test`, `harness_json_load_test` and
  `harness_json_schema_test` the document rows, and
  `harness_hunt_cli_test` with `harness_cli_table_test` the CLI
  rows. The fixture documents stand under `test/data/hunt_ranks/`.
- The example stands at `example/docs/hunt_ranks.cpp` with its
  `.expected` file and a `run_docs_` target, the standing seam.

### Public API surface added

- `sg::hunt::RankedEvent`: `sensitivityCorrelation`,
  `sensitivitySlope`, `delta`, each `std::optional<double>`, empty
  when the run measured none (FR-030's record half, D-1).
- `sg::hunt::EntryHunt::toCompareRules(std::size_t topN, double
  noisePercent)` -> `std::vector<std::string>`: the promoted events'
  rule texts, the pair shape of `toCounterSet` (FR-024, D-7).
- `sg::hunt::HuntOptions`: the baseline path the CLI settled, empty
  when the run named none.
- No other public name moves; the detail surface stands in
  `source/harness/detail/hunt_ranks.hpp` and carries the doxygen
  contracts the source enforces.

### Version and `SOVERSION`

- Version: `0.25.0` -> `0.26.0`, the next free minor at this cut
  (`CMakeLists.txt:30`).
- Harness `SOVERSION`: `16` -> `17`: the record fields and
  `toCompareRules` join the exported surface (FR-050, D-1).
- Archive `SOVERSION`: stays `4` (`CMakeLists.txt:95`).
- Report schema: `1.8` -> `1.9`
  (`source/harness/json_report.cpp:39`, FR-032).
- Option values: `281` and `282` from the hunt block's reserve;
  `283` to `289` stay free (FR-020).
- The commit that moves the numbers records the checks beside them.

## Test Plan

**Execution mode**: TDD (constitution III). The covering test task
of each wave stands written and observed failing before the
implementation task that turns it green; tasks.md carries the mode
per wave.

The tests build the records directly (`EventSamples`, `EntryHunt`,
`RankedEvent`) and load fixture documents under
`test/data/hunt_ranks/` through `loadReport`; the scripted seam
(`FakeProvider`) covers the run-path rows. No test sleeps, reads a
clock, or runs a timing measurement: the figures stand pure over
collected data (FR-006). The suite passes under `taskset -c 0,1`
and on a 2-cpu host.

### Waves

- **Wave 1 (M-A)**: the surface compiles, the fields link,
  `toCompareRules` declares, the version stands at `0.26.0` with
  `SOVERSION 17` / archive `4`, the schema constant reads `1.9`,
  the two option rows join the table. Verify:
  `ctest --preset=dev -R 'harness_hunt|harness_cli_table'`, plus
  the version and schema assertion rows.
- **Wave 2 (M-B, M-D in parallel)**: the statistics rows and the
  JSON round-trip rows. Verify: `ctest --preset=dev -R
  'harness_hunt|harness_json'`, red first, then green.
- **Wave 3 (M-C)**: the rank rows: the family points, the 3-point
  rule, the stamps, the delta formula and its disclosures, the
  norm check, the rank assignment. Verify:
  `ctest --preset=dev -R harness_hunt_rank_test`.
- **Wave 4 (M-E)**: the CLI and run-path rows: the rank keys, the
  refusals, the columns, the rules file, the batch stamping through
  the scripted seam. Verify: `ctest --preset=dev -R
  'harness_hunt_cli_test|harness_hunt_rank_test'`.
- **Wave 5 (M-F)**: the docs gates. Verify: `ctest --preset=dev -R
  'example_drift|snippet_compile|example_run'` and
  `cmake -P cmake/prose-lint.cmake`.
- **Wave 6 (M-G)**: the evidence records on gary. Verify: the
  records' exit codes stand 0 and the quickstart links resolve.
- **Wave 7 (convergence)**: the full suite, the coverage gate, the
  sanitizer and tsan presets, the release build (`ci-ubuntu` once
  for the analyzer findings), the prose gate over the range.
  Verify: `ctest --test-dir build --output-on-failure`, the
  coverage read at 100.0/100.0, `ci-sanitize`, `ci-tsan`,
  `cmake --preset=ci-ubuntu && cmake --build build`.

### FR coverage map

| FR | Wave | Test |
| --- | --- | --- |
| FR-001 | 3 | the family-points rows: grouping, one point per entry, the norm's rate |
| FR-002 | 3 | the under-3 rows: the `n/a` stamp, exit 0 |
| FR-003 | 3, 4 | the extraction rows over fixture documents; the delta formula rows; the run-path rows through the scripted seam |
| FR-004 | 3, 4 | the disclosure rows: missing event, missing entry, zero baseline rate; the once-printed rules line |
| FR-005 | 3, 4 | the reader-fault, schema-pairing and norm-mismatch refusal rows |
| FR-006 | 2 | the statistics rows: the monotone, the tie, the no-spread, the exact slope, the one-implementation rows |
| FR-010 | 4 | the rank-key rows: `sensitivity`, `delta` accepted; `score` refused |
| FR-011 | 3, 4 | the ordering rows: correlation descending, the tie chain; the delta-magnitude rows; the noisy-unranked rows; the no-baseline refusal |
| FR-012 | 4 | the console rows: the two columns, the `n/a` cells, the standing columns unchanged |
| FR-020 | 1 | the option-row rows: values `281` and `282`, the uniqueness test |
| FR-021 | 4 | the rules-file rows: the grammar lines, the threshold, the direction, the round trip through `--compare-rules` |
| FR-022 | 4 | the refusal rows: no hunt, empty value, unwritable file; the empty-file exit-0 row |
| FR-023 | 4 | the refusal rows: no hunt, empty value |
| FR-024 | 1, 4 | the contract rows of `toCompareRules`: the pair shape, no `detail/` type, the contract enforcement |
| FR-030 | 2, 4 | the writer rows: the three keys, the null rule, the closed node |
| FR-031 | 2 | the reader rows: the equal-values round trip |
| FR-032 | 1, 2 | the schema constant and the schema-file rows at `1.9` |
| FR-040 | 5 | the section rows: the cap, the snippet, the result sentence, the quick-reference rows |
| FR-041 | 5 | `example_drift`, `snippet_compile`, `example_run` |
| FR-050 | 1 | the version assertions: `0.26.0`, `SOVERSION 17`, archive `4`; the commit records the check |
| FR-051 | 2-4 | every FR of (A) to (E) names its test in this map |
| FR-052 | 6 | the records name the commands, the exit codes, the artifacts; the quickstart links |
| FR-053 | 7 | the coverage read at 100.0/100.0, no `LCOV_EXCL`, no new `--ignore-errors`, `ci-sanitize`, `ci-tsan`, `taskset -c 0,1`, the prose gate |

## Constitution Check (re-check after design)

The design holds the gate list: library-first (the surface in
`include/speedgun-ng/hunt.hpp`, the implementation under
`source/harness/`), no dependency (R-3, D-8), one implementation per
standing rule (R-2, R-4, D-8), the tests deterministic and
fixture-fed, the coverage gates named (FR-053), the contracts
carried in one place each (hunt-ranks-api.md), the docs gates named
(FR-041), the version record named (FR-050). The matcher move
touches one standing file; the compare tests stand the guard. No
complexity violation stands; the Complexity Tracking table stays
empty.

## Complexity Tracking

> Empty: no violation stands.
