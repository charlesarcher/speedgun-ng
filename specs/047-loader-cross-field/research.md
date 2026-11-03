# Research: Loader Cross-Field Checks for the Pass Fields

Phase 0 output of `/speckit.plan` for `specs/047-loader-cross-field/spec.md`.
Each entry names the reading it fixes, the code fact that decides it, and the
losing alternative. Every cite reads the working tree at the base `b1dd7670`
(version 0.30.0, schema 1.13, harness `SOVERSION` 20, archive `SOVERSION` 5), the
base of branch `047-loader-cross-field`. The spec, the plan and the three clarify
notes read the same tree, and a bare `:NNN` continues the file above it. The lane
host is chilly, an AMD Ryzen 9 7950X3D, GCC 16.2.1.

Numbering: the entries here read `R-1` to `R-15`, and a decision of the
specification reads `D-1` to `D-21` without a leading zero and is cited as
`spec D-14`. The two spellings name different files.

## R-1: where the three checks sit, and which pairs they read

- **Decision**: Three pair checks at the end of `readEntry`, after the record reads: the
  method beside the count, the spread beside the count, and a metric column's pass tag
  beside the count (spec D-3). The law lands in spec 024 FR-038 first (FR-001, D-8).
- **Rationale**: Every field of every pair stands in scope there: the outcome lands at
  `source/harness/json_load.cpp:2498`, the rows read at `:2636-2656`, the pass block at
  `:2726-2764`, and the hunt key at `:2770`. The insertion point stands after the
  top-down record at `:2791`, so the standing refusals keep their place.
- **Alternatives considered**: The tag check in `readMetricValue`: it holds no count. A
  pass over the loaded values in `loadReport`: the standing refusals move with it, and
  `spec D-16` needs the document's own order.

## R-2: what rule 1 waives, and why rules 2 and 3 need no waiver

- **Decision**: Rule 1 runs for an entry whose outcome is `MEASURED` and that carries no
  `hunt` key (spec D-14, FR-003). Rules 2 and 3 run on every entry that carries their
  pair.
- **Rationale**: The runner settles the method before the warm-up
  (`source/harness/runner.cpp:1624`) and publishes a count of 0 beside that word on four
  stop paths (`:2884`, `:2889`, `:2923`, `:2928`) and in the calibration of pass 0
  (`:3162`). An interrupt names the outcome `SKIPPED` (`:94-98`), so the guard covers
  both stop shapes. The hunt path overwrites the method with the word the operator
  forced (`source/harness/cli.cpp:1788-1789`) in the branch that fills the entry's
  `hunt` key (`:1775`, `json_report.cpp:859`).
- **Alternatives considered**: Drop the lower bound, so `MULTIPLEX` names 0 or 1: a
  measured `PASSES` entry with a count of 0 then loads, and User Story 1 scenario 3
  fails. Rules 2 and 3 need no waiver: the merge writes the spread and every tag above
  one pass (`runner.cpp:3303`, `pass_plan.cpp:322`), and a stopped collection publishes
  neither (`runner.cpp:1720-1722`).

## R-3: what counts as a present key, per rule

- **Decision**: Rule 1 counts a key present when it stands in the entry. Rules 2 and 3
  count a key present when it stands and holds a non-null value (spec D-17, FR-002).
- **Rationale**: The loader reads the spread as non-null at
  `source/harness/json_load.cpp:2744-2745` and the tag at `:388-389`, and the schema
  allows null at both (`report-v1.json:918`, `:681`). A value of the wrong kind keeps
  the standing kind refusal ahead of every pair check (`:135-144`, `:196-211`).
- **Alternatives considered**: One presence reading for all three rules: a null spread
  beside a count of 1 then answers a refusal, and the writer writes that pair for every
  one-pass run (`json_report.cpp:854`), so FR-010 fails. The pair rule leaves a count
  with no method key loading, and FR-004 takes that reading.

## R-4: the shape of a refusal line

- **Decision**: One refusal is one line, with one leading path naming the pair's
  non-count field, and the words carry the partner's full path (spec D-15, FR-007). Four
  lines stand, one per refusal arm.
- **Rationale**: The standing shape is the path, a colon, and the words
  (`source/harness/json_load.cpp:57-58`, `:207-208`), and the standing pair check names
  the field it validates at `:2759-2761`. The comparison prints the string as the reader
  built it (`compare.cpp:1876-1891`), and the loaded indices equal the document indices,
  so the path rebuilds byte for byte (`:2640`).
- **Alternatives considered**: The words name the partner by key word, the style of
  `:2761`: a test that looks for `/benchmarks/0/passCount` fails, and User Story 1
  scenarios 1 to 4 ask for both fields. Two lines per refusal: the reader returns one
  string, and `oneFaultLine` checks for exactly one line.

## R-5: the order of the checks, and the standing warning check

- **Decision**: The order runs: the rows, the standing `spreadWarning` check, rule 1,
  rule 2, rule 3 over the rows. The first fault stops the read, and the standing check
  keeps its place (spec D-16, D-9, FR-009).
- **Rationale**: The rows read at `json_load.cpp:2636-2656` and the pass block at
  `:2726-2764`, so the tag is known before the count and the count before the new
  checks. The standing stop is `allSettled` at `:120-133`, measured by
  `firstFailingPartStopsTheFold` (`harness_json_load_test.cpp:706-739`). The standing
  check reads `spreadWarning` against `passSpread`, and reads an absent spread as no
  spread (`json_report.cpp:1038-1041`).
- **Alternatives considered**: Move the pass block ahead of the rows: the standing
  refusals move with it. Gate the standing check by FR-002: a document with a true
  warning and no spread key refuses today and would then load.

## R-6: which rows the tag check reads, and the base of the tag

- **Decision**: The check reads every row of the entry that carries the count, and no
  context block. The tag is a 0-based pass index and stands below the count (FR-006).
- **Rationale**: Metric values live in one place: `BenchmarkResult::rows`
  (`benchmark.hpp:885`) to `ResultRow::metrics` (`:407`) to `MetricValue::pass`
  (`:251`). A context block carries percentile columns and raw columns alone
  (`json_load.cpp:1658-1689`, `:296-360`). The merge names the measuring pass from 0
  (`detail/pass_plan.hpp:47-49`), and User Story 1 scenario 5 plants a tag of 2 beside a
  count of 2.
- **Alternatives considered**: Read the context blocks: they carry no metric value, so a
  check there adds dark arms. A 1-based reading: the merge's numbering says otherwise,
  and scenario 5 then loads.

## R-7: the shape of a check that keeps every arm covered

- **Decision**: One `if` per refusal, a pair guard as one `&&` of the two key tests, a
  min/max pair per method, no `switch` over the method words, and no check as
  `SG_ENSURE` (spec D-19).
- **Rationale**: Measured with the `coverage-linux` flags and `gcov -b -c` on the lane
  host: the pair guard with a min/max pair answers 8 branch records with none taken
  zero, rule 2 answers 6, and rule 3 answers 12. The `switch` shape answers 14 with one
  taken zero, because `asEnum` refuses a word outside the three names
  (`json_load.cpp:2728-2732`), so its jump table holds an arm no document reaches. The
  extract step omits the arms of an `SG_*` line (`coverage.cmake:101`), and the enforce
  response calls `std::abort()` (`dbc.hpp:213`).
- **Alternatives considered**: The `switch`, which costs one dark row. The `&&`/`||`
  chain, measured at 16 records, which covers and costs eight more. `SG_ENSURE`, for a
  document fault: a load error is the right shape for a parser of external input
  (FR-007).

## R-8: where each accepted form comes from

- **Decision**: The scripted run of the load test answers the multiplex form. The pass
  report test carries the split form of a written document, and the load test builds the
  split and stopped forms by hand. The rich fixture answers the schema 1.0 form (spec
  D-18, FR-010 to FR-012).
- **Rationale**: The scripted run registers one metric column on an object that
  publishes no counter limit, so `settleMethod` answers `MULTIPLEX`
  (`json_fixture.hpp:96-122`, `pass_plan.cpp:131-135`) with a count of 1. A real split
  run settles `PASSES` with a count of 1 in memory at `harness_pass_report_test.cpp:833-835`, a site that writes no document, and
  the writer's stopped form names a count of 0 beside its settled word. The rich fixture
  names `SKIPPED`, `PASSES`, a count of 2, a spread of 3.5 and a tag of 1, in schema 1.0
  (`harness_json_load_test.cpp:181-218`).
- **Alternatives considered**: One scripted run for all three written forms: the fixture
  registers one column and scripts no limit, so it settles one method. A committed split
  document in `test/data`: the Scope admits no new fixture.

## R-9: the schema versions, the version gate, and the schema file

- **Decision**: The tests name 1.0, 1.12 and 1.13, every check runs on every document
  that carries its pair, and the version gate stands unchanged (spec D-6). The schema
  file, the checker, the writer and the comparison stand unchanged (D-10, FR-013).
- **Rationale**: The reader gates the major alone at `json_load.cpp:3129-3135`, so a
  minor carries no reading a check can branch on, and a 1.0 document carries no pass key
  (`harness.md:1395-1403`). The checker's language carries `type`, `required`,
  `x-order`, `properties` and `items` alone (`json_load.cpp:2815-2897`), and carries no
  conditional and no `allOf`: no cross-field rule stands expressible. `checkReport` is a
  separate reader (`:3055-3097`), and `loadReport` runs no schema check (`:3101-3247`),
  so the loader is the only gate.
- **Alternatives considered**: A version test in the code: the major gate stands ahead
  of the entry read, so an arm on a minor reaches no document and costs one dark row.
  "Any older minor of major 1" as a test rule: the set stands open, so no case name
  closes it.

## R-10: the compare pairing, and the documents the rules leave loading

- **Decision**: One test in `harness_compare_cli_test.cpp` plants a pair fault into a
  document of schema major 1 that the test writes at run time (spec D-21, FR-008).
- **Rationale**: The compare path answers status 2 with one quoted line for any load
  refusal (`compare.cpp:1876-1891`), and `oneFaultLine` measures that shape
  (`harness_compare_cli_test.cpp:782-800`). The survey read 40 committed documents with
  a benchmarks array, 57 entries, and 11 embedded documents. One carries a refused pair,
  `test/data/gate_schema_20.json:66-68`, and names schema version 2.0, so the version
  gate refuses it ahead of the entry read.
- **Alternatives considered**: Reuse `gate_schema_20.json` for the pairing: the line it
  quotes names the version. Add a fixture to `test/data`: that file writes documents at
  run time today, and a committed plant becomes a document a later rule must keep
  loading.

## R-11: the measurement point of the coverage gate

- **Decision**: The coverage job's union of a full suite judges the gate, and the lane
  reports the host reading at the base and at the head beside it (spec D-12, FR-018,
  SC-002).
- **Rationale**: The gate counts the union of the CI coverage job
  (`tools/dbc/coverage_gate.sh:86-88`), and a developer host reads higher, because an
  entry there can stop early and leave the arms past its stop dark (`:90-101`). The host
  reading at the base reads 99.5 percent lines (17484 of 17565) and 98.8 percent
  branches (8593 of 8701), with two coverage entries failing on this host today. The
  union reads 0 rows short against the allowance of 21 (`:102`,
  `specs/043-fwq-ftq/plan.md:122`), and the allowance bounds the shortfall, so an arm a
  test reaches costs nothing and an arm no test reaches costs one row (`:111`).
- **Alternatives considered**: The host reading as the judge: it can pass 21 with no new
  dark arm, and it can fail with none. The line figure alone: FR-018 names lines at 100
  percent and the shortfall at 21 rows.

## R-12: the version move, its three sites, and the rebuild

- **Decision**: The version moves 0.30.0 to 0.30.1 at three sites: `CMakeLists.txt:59`,
  `test/source/views_test.cpp:279` and `test/source/harness_cli_table_test.cpp:1453`
  (spec D-2, FR-015). The two `SOVERSION` numbers and the schema version stand.
- **Rationale**: `SG_PROJECT_VERSION` is a `PRIVATE` compile definition of the harness
  target (`CMakeLists.txt:323-327`), so the flag change touches every translation unit
  of that target: the library rebuilds whole and every harness test relinks
  (`test/CMakeLists.txt:776`). The shape gate folds digits and the build field
  (`report_shape.py:140-150`), so a recorded block keeps its own run's version.
- **Alternatives considered**: Keep 0.30.0: the reader refuses documents it accepted
  before. A minor move: no feature arrives, no public type moves and no key joins, so
  `spec D-2` takes the next free patch.

## R-13: the docs text and the drift gate classes

- **Decision**: Three sentences join the loader section of `docs/pages/harness.md` after
  `:1429`, and they name the three pairs, the pair rule and the three method words.
  `README.md` stands unchanged (spec D-7, FR-014).
- **Rationale**: The loader section stands at `:1424-1429` and states the loader rule
  today. The drift gate's docs classes forbid a commit hash, a spec identifier, a
  requirement identifier, a task identifier and a contract label on a user page
  (`tools/docs/example_drift.py:491-503`), and the scan strips fenced blocks and inline
  code spans first (`:449-463`), so a field name in backticks is safe.
- **Alternatives considered**: A main-page section: the root page teaches no loader
  rule, and the IV.1 check answers no main-page impact (spec D-7). A docs line that
  names a requirement identifier: the class answers a hit.

## R-14: the two host tools the lane pins

- **Decision**: The lane installs clang-format 18.1.8 and runs the lint gate through
  `FORMAT_COMMAND`, and carries `PERL5LIB` in the environment of the coverage commands.
  No gate, preset, or tool file changes (spec D-20).
- **Rationale**: CI installs clang-format 18.1.8 by pip
  (`.github/workflows/ci.yml:33-34`), and the tree stands formatted by it. The host
  `/usr/bin/clang-format` answers 23.1.4, names untouched files as badly formatted, and
  the gate exits 1; the gate takes the tool by name through `FORMAT_COMMAND`
  (`cmake/lint.cmake:9`). The host `lcov` needs `Capture::Tiny` at `~/perl5/lib/perl5`:
  without it the capture, the extract and the gate exit 2, 2, 3.
- **Alternatives considered**: Change the lint gate so the host tool passes: `spec D-20`
  bars a gate change. Take the coverage figure from the CI artifact alone: FR-018 names
  the host reading at the base and at the head.

## R-15: the specs/046 overlap, the rebase rule, and the test wiring

- **Decision**: Both lanes touch `json_load.cpp` and `harness_json_load_test.cpp` in
  different readers. The second lane to merge rebases, keeps both sides, and runs every
  gate again (spec D-13). No new target, CTest entry or fixture joins the change
  (FR-016).
- **Rationale**: specs/046 takes the hunt record at `json_load.cpp:2770-2777`, and this
  lane takes the end of `readEntry` at `:2791`, so no hunk overlaps. The load test runs
  27 functions called from `main` (`harness_json_load_test.cpp:2205-2238`), and its
  target and its CTest entry exist (`test/CMakeLists.txt:775-783`). CTest registers one
  entry for the whole file, so the case names the test log prints carry the count
  (FR-016, SC-001).
- **Alternatives considered**: A new test file: it needs the six registration lines of
  `test/CMakeLists.txt:775-783` and a new CTest entry. A merge commit: the base branch
  stays linear (spec Constraints).

## Open readings

The clarify notes leave five readings open, and each closes here.

- The allowance citation. The notes read `coverage_gate.sh:99` for the 21 rows, and the
  constant stands at `:102`. The spec cites `:102`, and `R-11` takes that line.
- The coverage figure. The host has no working `lcov` at the base, so the union figure
  comes from the coverage job's artifact, and the host reading stands beside it in the
  pull request body (FR-018, SC-002).
- The docs count. FR-014 counts sentences, and no gate counts lines of a docs section.
- The test count. CTest registers one entry for the load test, so the case names in the
  test log carry the count, and the plan's FR coverage map names the case for each
  requirement.
- The rebase order. Neither lane knows which of specs/046 and this lane lands second, so
  `spec D-13` states the rule as a step of the gate commands.
