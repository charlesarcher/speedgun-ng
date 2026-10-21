# Feature Specification: Regression Gate

**Feature Branch**: `026-regression-gate`

**Created**: 2026-10-20

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"Regression gate, item N5 (old H8) of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md`. Short name: regression-gate.
Target machine: gary. Lane: harness (N1 and N3 done, N5 now, then N7). It
runs in parallel with N6 multi-pass collection (corsairone). N3 and N4
are merged. thrawn is paused. A CI job can compare a baseline JSON report
with a new JSON report and get a pass or fail result. After N5, speedgun
(or any binary built on the harness) can load two reports, match the
benchmarks by name, and compare them by rules. A rule names any value
that the report holds (time, percentile, counter, N2 formula metric or
custom statistic), a direction and a threshold. The gate prints a
comparison table and exits with a status that tells CI the result. With
no rule, the gate compares the median time per iteration with a 5%
threshold. N7 (full command line, compare mode) builds on it. [...]"

## Clarifications

### Session 2026-10-20

- Q: Where does the noise warning a report carries reach the caller, when
  the loaded result types hold no field for it? → A: a fourth parallel
  table of the loaded report value, one optional warning record per
  result, with the record type moved to the public header; the result
  types and the report writer keep their fields (D-17, FR-041, FR-070).
- Q: When a matched benchmark's rows hold no row a rule can read, as a
  complexity fit result does, does the run fault or answer a state? → A:
  the entry prints `SKIPPED` and does not fail while strict mode stands
  off, and only a rule whose whole filtered scope holds such results
  stops with status 2 (D-20, FR-011, FR-036, FR-040).
- Q: Does the default `@median` aggregate make a rule that names no
  aggregate on a percentile or a statistic malformed? → A: the default
  applies to a value the report holds as a statistics record, so
  `p99:lower:5%` reads whole and a written suffix on a percentile or a
  statistic is the rule fault (D-21, FR-016, FR-018).
- Q: Which field of the statistics record is the spread the noise guard
  holds against an absolute threshold? → A: `sampleStdDev` against an
  absolute threshold and `coefficientOfVariation` against a relative one,
  each read from the value's own record, and either report over its bound
  marks the row (D-19, FR-042, FR-046).
- Q: In what unit does an absolute threshold stand when the value is a
  percentile cell or a custom statistic, and what does a unit difference
  between the two reports change? → A: the stored unit of the value,
  nanoseconds for a time column, a percentile cell or a `TIME` statistic,
  percentage points for a `PERCENTAGE` statistic and the counter's unit
  for a counter or metric column; the table prints both values and the
  absolute change in the unit the baseline declares, and the difference
  changes no comparison (D-22, FR-027, FR-055).

## Run-time binding

This spec binds no spec number, commit, or source line beyond the base
it records here.

- **Spec number**: 026. `master` at `aec8fce` carries `specs/001`
  through `specs/023` and `specs/025`; 024 stays reserved for N6, the
  multi-pass feature running on corsairone, and no open pull request
  carries a spec directory at all (`gh pr list` answers an empty list on
  2026-10-20). 026 is therefore the next number free on `master` and on
  every branch. A rebase that finds 026 taken takes the next free number
  and records the change here.
- **Branch base**: `aec8fce` (2026-10-20), the head of `master` at
  branch creation: version 0.14.0 (`CMakeLists.txt:7`), harness
  `SOVERSION` 8 (`CMakeLists.txt:247`), `speedgun-ng` archive
  `SOVERSION` 2 (`CMakeLists.txt:47`). N1, N2, N3 and N4 are merged.
- **Names**: every symbol named below is an anchor read at the audit
  point on that commit. A plan re-reads each one on the base it runs
  against and records any drift.

## Decisions

Charles accepted D-1 to D-14 on 2026-10-20 as recommendations of the
prompt this spec comes from. They are governance for this feature, and
each is recorded again at the FR that carries it. D-15 to D-19 are
derived at this audit point from the code facts PC-2, PC-6, PC-7 and
PC-10; Charles has not ruled on them, and each stands as a recommendation
he can overturn. The clarification session of 2026-10-20 settled the two
open halves of D-17 and D-19 and added D-20 to D-22. Those five stand as
this session's decisions, each sourced at its `- Q:` bullet above, and
none of them carries the owner's approval.

- **D-1 Library surface**: one exported call compares two loaded reports
  under a list of rules and answers one result per matched benchmark and
  rule plus one overall verdict, and one exported call parses one rule
  text, so the command line and a library user read the same grammar. An
  empty rule list answers the default rule (C-1, FR-001 to FR-007).
- **D-2 One value name space**: every value a report holds carries one
  name, and a rule names that value: `time`, `real_time`, `cpu_time`,
  `p<point>`, `counter:<label>`, `metric:<label>`, `stat:<name>`. Names
  are case-sensitive (C-3, FR-012 to FR-019).
- **D-3 Aggregate suffix**: `@median` is the default aggregate, with
  `@mean`, `@min` and `@max` beside it, written after the value name. The
  gate reads the statistics records the report carries for the aggregate,
  and a run with one row and no statistics answers every aggregate with
  that row's value (C-3, FR-015 to FR-017).
- **D-4 Threshold semantics**: the delta runs forward for a
  lower-is-better value and backward for a higher-is-better one; a
  relative test divides the delta by the magnitude of the baseline, an
  absolute test compares the delta with the threshold in the unit of the
  value, and a rule with both thresholds regresses only when both are
  exceeded, so an absolute floor stops a large relative change on a tiny
  value (C-4, FR-025 to FR-028).
- **D-5 Default rule scope**: `time@median:lower:5%` applies when no rule
  is given. Given rules replace it; the caller who wants the time column
  beside a metric rule names the time column again (C-4, FR-033 to
  FR-034).
- **D-6 Command-line rule grammar**: `--compare-rule=<value>[@<agg>]:`
  `<lower|higher>:<threshold>[:<filter>]`, repeatable, with the threshold
  as `5%`, `abs=12.5` or `5%,abs=12.5`. The parser reads the `counter:`,
  `metric:` and `stat:` prefix first, because those prefixes carry their
  own colon (C-3, C-7, FR-020 to FR-023, FR-049).
- **D-7 Validation before comparison**: every rule is checked against
  both reports, inside the scope of its own filter, before any value is
  compared. A rule that names no value, a metric that answers nothing,
  and a filter that matches no benchmark each stop the run with one line
  and status 2 (C-5, FR-035 to FR-040).
- **D-8 Noise guard**: a carried noise warning, a spread wider than the
  rule's own threshold, or a multiplexed metric marks the row `NOISY`,
  and a `NOISY` row never fails the run while `--compare-strict` is off
  (C-6, FR-041 to FR-046).
- **D-9 Unmatched and unmeasured benchmarks**: a name in one report only
  prints `ADDED` or `REMOVED`, and a benchmark whose outcome is not
  measured in either report prints `SKIPPED`. None of them fails the run
  while `--compare-strict` is off (C-2, FR-009, FR-011).
- **D-10 Compare mode input**: `--compare=<baseline>,<contender>` names
  the two files and runs no benchmark. The split stands at the first
  comma (C-7, FR-047 to FR-048).
- **D-11 Rules file deferred**: option files, environment defaults and a
  rules file belong to N7. This feature ships the repeatable
  command-line option and the library call, and N7 reads a rules file
  through the same parse call (Scope).
- **D-12 Output**: a console table with the benchmark, the rule, the
  baseline value, the contender value, the change in percent and in
  absolute terms, and the state word; and `--json=<path>` in compare mode
  writes the same comparison as a document described by
  `docs/schema/compare-v1.json`, built with the vendored writer (C-8,
  FR-053 to FR-058).
- **D-13 Schema version pairing**: the two reports must carry the same
  `schemaVersion` text. A mismatch stops with one line naming both
  versions and status 2, and a report whose major version the reader does
  not know already stops inside the load call (C-7, FR-051).
- **D-14 Version and merge order**: N5 takes the next free minor version
  after every feature that merges before it, and the harness `SOVERSION`
  moves 8 to 9 for the grown public surface of the harness archive. The
  `speedgun-ng` archive `SOVERSION` stays 2 (FR-065 to FR-066).
- **D-15 Label escape**: a metric label, a counter label and a statistic
  name carry no character rule on `master`, so one can hold a colon, a
  comma or an at sign (PC-7). A prefixed value name therefore reads a
  backslash escape for `:`, `,`, `@` and the backslash itself inside the
  label part, and the comparison reads the unescaped label (FR-020).
- **D-16 Percentile columns in scope**: a percentile cell is addressed by
  point in the primary time column, the column the report prints first
  (PC-5). A percentile cell of the other time column or of a metric
  column needs a column qualifier the grammar cannot carry while a label
  may hold a colon, so those cells stay with N7 (FR-014, Scope).
- **D-17 The carried noise warning**: the reader checks the warning
  record a report carries and lets its three values go, because no result
  field holds them (PC-2). The gate reads the warning the file carries,
  so the reader answers it to the caller; recomputing it from the loaded
  value would compare the baseline against the cpu count of the machine
  that runs the gate (FR-070, FR-041). The clarification settled where: a
  fourth parallel table of the loaded report value, one optional record
  per result, the record type moved to the public header, and the result
  types and the report writer unchanged (PC-1, PC-2).
- **D-18 Statistics cover the primary column only**: the aggregate
  statistics of a repeated run summarize the primary time column, and the
  report holds no statistics record for the other time column (PC-6). An
  aggregate named on a column with no statistics record in a repeated run
  is a missing value; a single-row run answers every aggregate with its
  one row (FR-017, FR-036).
- **D-19 Noise bound for every rule form**: the guard reads the value's
  own statistics record, `coefficientOfVariation` against the rule's
  relative threshold and `sampleStdDev` against its absolute threshold in
  the stored unit of the value, so a rule with an absolute threshold and
  no relative one still answers a bound and the guard stays defined for
  every rule form (FR-042, FR-046). The clarification of 2026-10-20 named
  the two fields and settled that either report over its bound marks the
  row.
- **D-20 A result with no readable row**: a matched benchmark whose rows
  hold no row a rule can read, the complexity fit result of PC-6 among
  them, answers a `SKIPPED` entry with no values and does not fail while
  strict mode stands off; a rule whose whole filtered scope holds only
  such results is the fault FR-040 states. The default rule reaches the
  fit entry of any report that holds a complexity family, so the fault
  reading would stop every such run (FR-011, FR-036, FR-040).
- **D-21 The default aggregate beside a single-value name**: `@median` is
  the default for a value the report holds as a statistics record. A
  percentile cell and a custom statistic hold one value per benchmark,
  take no aggregate, and answer that value when the rule writes no
  suffix; a written suffix on either is the rule fault (FR-016, FR-018,
  FR-033).
- **D-22 The unit of an absolute threshold**: the threshold stands in the
  stored unit of the value: nanoseconds per iteration for a time column,
  a percentile cell or a `TIME` statistic, percentage points for a
  `PERCENTAGE` statistic, and the counter's own unit for a counter or
  metric column. The table prints both value columns and the absolute
  change in the unit the baseline report declares, so one column carries
  one unit (FR-027, FR-055).

## Audit point

Read on 2026-10-20 at `aec8fce` on Gary, the machine this feature
develops on: one socket, 16 cores, 32 hardware threads, two L3 groups. No
claim in this spec depends on that shape; the tests build report values
in code or read them from fixture files, and no test runs a benchmark or
reads a clock.

The roadmap file the prompt names,
`/workspace/speedgun-ng-harness-roadmap.md`, is absent from this machine,
so no claim here rests on it. The prompt text is the authority for scope.

## Preconditions

### PC-1: N3 JSON reports merged, and it is the only reader. PASS.

`loadReport` stands at `include/speedgun-ng/benchmark.hpp:2330` and
answers `std::expected<ReportData, std::string>`. `ReportData` stands at
`:553` with `results`, `rowRaw` and `contextRaw`, one outer entry per
result. The reader is `source/harness/json_load.cpp`, the writer is
`source/harness/json_report.cpp`, and the schema file is
`docs/schema/report-v1.json`. The schema's top keys are `schemaVersion`,
`speedgun` and `benchmarks` (`docs/schema/report-v1.json:7`). No command
line uses the load call today, so this feature is its first user.

### PC-2: the values a report holds, and the one value it drops. PASS, with one gap the gate needs.

The schema carries, per benchmark: `name`, `suite`, `case`, `outcome`,
`reason`, `hideRepetitionRows`, `timeUnit`, `primaryIsCpu`,
`pauseSamplesFolded`, `percentilePoints`, `contexts`, `noiseWarning`,
`rows`, `metricLabels`, `counterLabels`, `counterStats` and
`statisticValues`. Per row: `kind`, `label`, `iterations`,
`timePerIterationNs`, `cpuTimePerIterationNs`, `endpointSamples`,
`pauseSamplesFolded`, `timeAvailability`, `metrics`, `counters`,
`counterDisclosure`, `fit`, `overheadFloorNs`, `percentiles`,
`chunkCount`, `chunkIterations`, `timeStats`, `metricStats`,
`contextPercentiles` and `contextRaw`, and per benchmark `raw`.

The loaded value reaches all of them through `BenchmarkResult`
(`include/speedgun-ng/benchmark.hpp:441`) and `ResultRow` (`:352`),
except the noise warning: the reader reads the `noiseWarning` record,
checks its three fields, and lets the values go, because no result field
holds them (`source/harness/json_load.cpp:729` to `:743`). The warning
type and its predicate live inside the harness
(`source/harness/detail/json_report.hpp:137`, `:154`), and that predicate
reads the affinity mask of the running process for its cpu count
(`source/harness/json_report.cpp:472` to `:481`). D-17 records what the
gate does about that.

### PC-3: the option table, the usage-error path, and the entry point. PASS.

`speedgunMain` stands at `source/harness/cli.cpp:290` and the long
option table at `:321` to `:344` with `filter`, `list`, `repetitions`,
`min-time`, `iterations`, `warmup-time`, `dry-run`, `counter`, `metric`,
`catalog`, `percentiles`, `no-percentiles`, `placement`, `topology`,
`json` and `json-raw`. The usage errors print one line and answer status
2 at `:70`, `:122`, `:142`, `:164`, `:248`, `:371`, `:385`, `:392`,
`:406`, `:426`, `:446` and `:468`. The entry point closes with the
postcondition
`SG_ENSURE((status != 0) == (anyFailed || anyInterrupted || reportFailed),`
at `:566`, and the status table of
`specs/015-benchmark-harness-core/contracts/cli.md:83` lists the rows
that entry point answers today. A compare mode that answers status 1 for
a regression and status 2 for a rule fault moves both (FR-052).

### PC-4: what the reader does with a schema version. PASS, with the drift fact.

The writer stamps `1.0` (`source/harness/json_report.cpp:28`, `:530`).
The reader takes the text before the first dot and refuses anything whose
major is not `1` (`source/harness/json_load.cpp:1117` to `:1128`), so a
minor difference loads. The reader walks the document against a table of
schema nodes and accepts a key the table does not list wherever the
schema leaves the object open, while a closed object names the key
(`source/harness/json_load.cpp:1000` to `:1030`). A field N6 adds to the
report therefore loads as an unknown key today, which is the fact the
pass-field rule of this spec rests on.

### PC-5: the console printers the comparison table follows. PASS.

In `source/harness/report.cpp`: `printFitRow` (`:88`),
`printContextDisclosure` (`:164`), `printNoiseWarning` (`:193`),
`placementField` (`:211`), `selectRunnableCpuCount` (`:224`),
`printContext` (`:234`), `printPercentileLine` (`:257`),
`percentileColumnLabel` (`:282`), `percentileColumnDivisor` (`:300`),
`printRowPercentiles` (`:309`) and `printResult` (`:331`). The percentile
columns stand in positional order: column 0 the primary time, column 1
the other time column, column 2 onward the metric columns (`:287` to
`:296`), and a counter column carries no percentile cells. The unit scale
that turns stored nanoseconds into printed values is `unitScale` (`:70`).

### PC-6: the rows and statistics a repeated run writes. PASS.

A measured run with more than one repetition appends one aggregate row
(`source/harness/runner.cpp:2163`), and that row carries `timeStats`
(`:2195`) and one `metricStats` entry per metric column (`:2229`), while
the result carries one `counterStats` entry per counter column (`:2262`)
and the attached `statisticValues`
(`include/speedgun-ng/benchmark.hpp:468`). The time statistics summarize
the primary column only: the value pushed for each clean repetition is
the manual or real column when the family chose one and the cpu column
otherwise (`source/harness/runner.cpp:2185` to `:2196`). A run of one
repetition writes no aggregate row and leaves `timeStats` null.
`Statistics` holds `samples`, `mean`, `median`, `sampleStdDev`,
`coefficientOfVariation`, `min` and `max`
(`include/speedgun-ng/benchmark.hpp:231`). `MetricValue` holds `value`,
`runningRatio`, `scaled` and `availability` (`:212`), and `Availability`
answers `COUNTABLE`, `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`,
`SCOPE_REFUSED` and `GAP` (`include/speedgun-ng/counters_core.hpp:87`).
`RunOutcome` answers `MEASURED`, `SKIPPED` and `FAILED`
(`benchmark.hpp:77`), and `RowKind` answers `REPETITION`, `AGGREGATE`,
`BIG_O`, `RMS` and `SKIPPED` (`:88`). A complexity fit result carries
`BIG_O` and `RMS` rows under the family name and no time statistics at
all (`source/harness/runner.cpp:2436` to `:2466`), so it holds no
comparable value for any rule.

### PC-7: the label character check the prompt asked for. PASS, and the answer is that no rule exists.

`addMetric` takes any text as the label and states no character rule
(`include/speedgun-ng/benchmark.hpp:1758` to `:1768`). `addStatistic`
refuses only the seven built-in aggregate names in its precondition
(`:1786`). A counter label is the counter name the plan carried
(`source/harness/runner.cpp:2131`), and the built-in catalog names hold a
slash and no colon (`machine/thread_cpu`, `machine/process_cpu`,
`machine/monotonic`, `machine/tsc`), while a user counter address is free
text. A label can therefore hold `:`, `,` and `@`, so the grammar needs
the escape D-15 records (FR-020).

### PC-8: the pattern rule a rule filter must match. PASS.

`--filter` compiles a `std::regex` and searches the instance name with
`std::regex_search` (`source/harness/cli.cpp:529` to `:536`), answers one
line and status 2 for an invalid expression (`:470` to `:474`), and
answers one line on standard error with status 0 when no benchmark
matches (`:545` to `:550`, the row `the filter matched no benchmark | zero,
with a message` of the contract table). A rule filter uses that same
pattern rule, and its own no-match answer is the new status 2 row FR-039
states, which FR-052 carries into that table (FR-024).

### PC-9: the documentation gates and their budgets. PASS.

`README.md` runs 713 lines and `PAGE_LINE_CAP` reads 713
(`tools/docs/example_drift.py:132`); `SECTION_LINE_CAP` reads 25 (`:133`);
`SECTIONS` lists the 23 main-page capability titles (`:30` to `:54`) and
`SECTION_OWNERS` binds each title to one example file (`:59`). The last
two moves are recorded in that file's own comment for specs/025 and
specs/023. `docs/pages/harness.md` runs 1093 lines and its last
capability section is `## JSON reports` (`:1018`). The example targets
are built by `add_docs_example` with a `run_docs_` runner beside each one
(`example/CMakeLists.txt:45` to `:47`), and `example_run` is a ctest
entry that drives `test/example_run.sh` (`test/CMakeLists.txt:1136` to
`:1139`).

### PC-10: the test surface a fixture file needs. PASS.

No `test/data/` directory exists today, and no C++ test reads a committed
data file: the report tests write their own file and read it back
(`test/source/harness_json_report_test.cpp:41` to `:53`). The test
targets do receive source-tree paths on their command line, the shape
`add_test(NAME hwloc_purity_scan COMMAND bash
${CMAKE_CURRENT_SOURCE_DIR}/../tools/hwloc/hwloc_purity_scan.sh
${CMAKE_SOURCE_DIR})` at `test/CMakeLists.txt:82`. A new `test/source/`
file needs its `add_executable`, `target_link_libraries` and `add_test`
entry there before `ctest` sees it.

### PC-11: the shape gates this feature must leave alone. PASS.

`test/cli_shape.sh` locates the benchmark loop of `speedgunMain` by its
`runner.run(*entry)` line and counts the interrupt-flag readings inside it
(`test/cli_shape.sh:31` to `:41`), so a compare mode that returns before
that loop keeps the gate's anchors intact. `test/loop_shape.sh` pins the
per-iteration cycle of the timed loop, and this feature touches no
per-iteration path.

## Roadmap position

N5 of the harness roadmap, the item the roadmap numbered H8. It sits in
the gary lane: N1 percentiles and raw capture landed, N3 JSON reports
landed, N5 runs now, then N7, the path to the CI gate. N4 topology
placement landed on `master` through PR #42 as spec 025.

The gate is the reason the report file exists: N3 wrote a stable,
versioned document so that this feature could compare two of them. N7
builds the full command line, the option files and the environment
defaults on the compare mode this feature opens.

N6 multi-pass collection runs in parallel on corsairone and reserves spec
024. thrawn is paused.

## Parallel work and the version rule

- **Shared files**: the project `VERSION` in `CMakeLists.txt`, the two
  `SOVERSION` lines, `README.md`, the quick-reference table,
  `PAGE_LINE_CAP` in `tools/docs/example_drift.py`,
  `example/CMakeLists.txt`, the `example_run` list, the
  `test/CMakeLists.txt` target lists, the option table of
  `source/harness/cli.cpp`, and `docs/schema/report-v1.json`. `master` at
  `aec8fce` carries no changelog file.
- **Ownership against N6**: N6 adds pass fields to the report schema and
  to the run. This feature reads reports through the load call and
  changes no producer, no result type and no field of the report
  document. The reader's node table already takes an unknown key wherever
  the schema leaves the object open (PC-4), which is what makes the two
  features loadable in either merge order.
- **Pass-field rule**: the feature that merges second makes the
  comparator answer the pass fields of the other lane, or states in one
  line that it ignores them, and adds the test for that answer. If N6
  lands first, this feature rebases and that test lands here.
- **Conflict rule**: rebase onto the new `master`; no merge. Keep the
  lines of both features in every shared list, take the next free minor
  version, raise `PAGE_LINE_CAP` again by the size of this feature's own
  section, then run the full gate set and push once with
  `--force-with-lease`, showing `git ls-remote origin <branch>` after the
  push.
- **Version rule**: D-14. N5 reserves 0.16.0 and N6 reserves 0.15.0.
  `master` reads 0.14.0 (`CMakeLists.txt:7`), so whichever of the two
  lands first takes 0.15.0 and the other rebases to 0.16.0. The harness
  `SOVERSION` moves 8 to 9 (`CMakeLists.txt:247`), and the `speedgun-ng`
  archive `SOVERSION` stays 2 (`CMakeLists.txt:47`).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Get a pass or fail answer over two reports (Priority: P1)

A developer or a CI job holds a baseline report from a known-good build
and a report from the build under test. One command loads both, matches
the benchmarks by name, applies the rules, prints one line per benchmark
and rule, and answers a status the CI job can branch on. With no rule
given, the gate compares the median primary time per iteration and
tolerates a 5% drift.

**Why this priority**: this is the whole deliverable. A CI job that can
load two files and answer 0 or 1 is the value; every other story sits on
top of it.

**Independent Test**: write two report documents whose median time
columns differ by 3% and by 9%, run the gate with no rule, and read the
status and the table. The pair of documents needs no benchmark run of
its own: they are fixture files.

**Acceptance Scenarios**:

1. **Given** two reports whose median primary time differs by 3%,
   **When** the gate runs with no rule, **Then** the table prints one row
   per benchmark with state `OK`, and the process answers status 0.
2. **Given** two reports whose median primary time differs by 9%,
   **When** the gate runs with no rule, **Then** the table prints that
   benchmark with state `REGRESSED`, the change column carries the
   percent and the absolute delta, and the process answers status 1.
3. **Given** two reports where the contender is faster on every
   benchmark, **When** the gate runs with no rule, **Then** every row
   prints `IMPROVED` and the process answers status 0.
4. **Given** a baseline report and a contender report that name different
   benchmarks, **When** the gate runs, **Then** the names that appear in
   one document only print `ADDED` or `REMOVED`, and the process answers
   status 0 while strict mode stands off.

---

### User Story 2 - Name the value and the rule (Priority: P1)

A maintainer who cares about a percentile, a counter metric or a custom
statistic writes one rule per value they care about: the value name, the
aggregate, the direction, the threshold and an optional benchmark filter.
The gate applies exactly those rules and no others.

**Why this priority**: a gate that only checks the median time cannot
protect the values a performance owner watches by name, and the value
name space is what makes a rule readable in a CI log.

**Independent Test**: build two report values in code that carry a
percentile column, a counter column, a metric column and three attached
statistics, then run the comparison over each value kind with each
aggregate, each direction and each threshold form.

**Acceptance Scenarios**:

1. **Given** two reports with a percentile point of 99 in both, **When**
   the rule is `p99:lower:10%`, **Then** the table prints one row per
   benchmark under that rule and the verdict follows the 99th percentile
   cell of the primary time column.
2. **Given** a rule `metric:ipc:higher:3%:bmHash.*`, **When** the gate
   runs, **Then** the rule applies to the benchmarks whose name matches
   the pattern and reads the `ipc` metric column of each.
3. **Given** a rule `counter:bytes@mean:higher:abs=1e6`, **When** the gate
   runs, **Then** the row compares the mean of the `bytes` counter column
   and fails only when the delta exceeds one million.
4. **Given** a rule with a relative and an absolute threshold, **When**
   the contender's value moves 20% on a tiny baseline and stays under the
   absolute floor, **Then** the row prints `OK`.
5. **Given** two rules on the command line, **When** the gate runs,
   **Then** the median time rule is absent unless the caller wrote it.

---

### User Story 3 - Get one clear line when the rule or the file is wrong (Priority: P1)

A rule typed at a keyboard can be malformed, can name a value the reports
do not hold, or can filter down to nothing. A file can be missing,
unparseable, or written by a schema this build cannot read. Each fault
answers one line that names the fault and a status that tells CI the run
never started.

**Why this priority**: a gate that answers a silent pass on a typo is
worse than no gate, so the fault path carries as much weight as the
comparison path.

**Independent Test**: run the command surface against each malformed rule
text, each unknown value name, each empty filter and each broken file,
and read the line and the status.

**Acceptance Scenarios**:

1. **Given** `--compare-rule=time:lower:` with no threshold, **When** the
   gate runs, **Then** one line names the rule text and the fault, and
   the process answers status 2.
2. **Given** a rule naming `p99` while neither report carries a 99 point,
   **When** the gate runs, **Then** one line names the rule, the
   benchmark and the report, and the process answers status 2.
3. **Given** a rule whose filter matches no benchmark in either report,
   **When** the gate runs, **Then** one line names the rule and the
   filter, and the process answers status 2.
4. **Given** a baseline path that does not exist, **When** the gate runs,
   **Then** one line names the path, and the process answers status 2.
5. **Given** two reports whose `schemaVersion` texts differ, **When** the
   gate runs, **Then** one line names both versions, and the process
   answers status 2.
6. **Given** a rule with a relative threshold over a baseline value of
   zero, **When** the gate runs, **Then** one line names the benchmark
   and the value, and the process answers status 2.

---

### User Story 4 - Refuse to fail on a noisy measurement (Priority: P2)

A run whose contexts oversubscribed the cpus, a metric the hardware
multiplexed, or a value whose own spread is wider than the threshold it is
being held to, cannot carry a verdict. The gate marks such a row `NOISY`,
prints the reason, and keeps the run passing; strict mode turns those
rows back into failures.

**Why this priority**: the gate's first CI use is a job that must not go
red on a measurement the harness itself distrusts. The guard refines the
verdict of stories 1 and 2 without changing them.

**Independent Test**: build report pairs whose rows carry a noise warning,
a coefficient of variation above the rule threshold, or a multiplexed
metric, and read the state word and the status with strict mode off and
on.

**Acceptance Scenarios**:

1. **Given** a baseline report that carries a noise warning for one
   benchmark, **When** the gate runs, **Then** that benchmark's rows
   print `NOISY` with the reason, and the process answers status 0.
2. **Given** a value whose statistics record exceeds the rule's bound in
   either report, **When** the gate runs, **Then** the row prints
   `NOISY` and the reason names the bound it exceeded.
3. **Given** a metric whose running ratio sits below one in either
   report, **When** the gate runs, **Then** the row prints `NOISY`.
4. **Given** the same pair, **When** the gate runs with
   `--compare-strict`, **Then** the `NOISY` rows fail and the process
   answers status 1.

---

### User Story 5 - Read the comparison as a library, and as a document (Priority: P2)

A build on top of the harness wants the comparison as values it can
read: one entry per benchmark and rule with both values, the change and
the state, plus one overall verdict. A CI job wants the same comparison as
a file it can archive next to the two reports.

**Why this priority**: the library call is what N7 and any consumer build
on, and the written document is what makes a gate result inspectable
after the job ends. The console table already serves the human, so this
story deepens what that table answers.

**Independent Test**: call the comparison directly on two loaded report
values and read the entries; run the command surface with `--json` in
compare mode and check the written document against the comparison
schema.

**Acceptance Scenarios**:

1. **Given** two loaded report values and one rule, **When** the
   comparison call runs, **Then** it answers one entry per matched
   benchmark and rule, each carrying both values, the change and the
   state, and one overall verdict.
2. **Given** a rule text, **When** the parse call reads it, **Then** it
   answers the value name, the aggregate, the direction, both thresholds
   and the filter, or one error naming the fault.
3. **Given** the compare mode with `--json=<path>`, **When** the gate
   runs, **Then** one file stands at that path, it answers the comparison
   schema, and it carries one entry per table row.
4. **Given** the same two reports, **When** the compare mode runs twice
   with the same rules, **Then** the two written files are byte for byte
   equal.

---

### User Story 6 - Read the page, copy the block, run the example (Priority: P3)

A reader of the main page learns the gate from one short section: the two
options, one rule example, the state words, and the recorded output of a
run that CI executes.

**Why this priority**: the feature ships once a reader can use it, and the
documentation gates bind the page to a runnable example.

**Independent Test**: run the example gate and the drift gate over the
page, and read the recorded output of the new example against its
expected file.

**Acceptance Scenarios**:

1. **Given** the main page, **When** the drift gate runs, **Then** the new
   section sits inside its line budget, its snippet comes from the new
   example file, and the page stays inside the page cap.
2. **Given** the example's recorded output, **When** the example gate runs
   it, **Then** the recorded block matches the output the example
   produces.
3. **Given** the quick-reference table, **When** a reader looks up the
   compare calls or the new options, **Then** a row names each one and
   links the section.

### Edge Cases

- A report holds no benchmark at all: the comparison answers an empty
  table, and the run answers status 0 with a line that says the pair
  matched nothing.
- A benchmark whose outcome is `SKIPPED` or `FAILED` in either report:
  the row prints `SKIPPED`, carries no values, and does not fail while
  strict mode stands off.
- A complexity fit result, whose rows carry a fit and no time statistics:
  it holds no value for any rule, so its entry prints `SKIPPED` beside the
  entries of the family's instances, and a rule whose filter reaches only
  such results answers no value and stops with status 2 (D-20).
- A run with one repetition and no statistics record: every aggregate
  answers that row's single value, including `@median`.
- A repeated run whose every repetition carried a gap: its aggregate row
  holds a statistics record with a zero sample count and zero in every
  field, and a rule over it reads that zero, which makes a relative
  threshold a rule fault.
- A rule naming an aggregate on a column with no statistics record in a
  repeated run: the value is missing and the run answers status 2.
- A rule writing an aggregate suffix on a percentile or a custom
  statistic: the rule is malformed and the run answers status 2, and a
  rule that writes no suffix reads that single value (D-21).
- A label that holds a colon, a comma or an at sign: the escaped form
  names it, and the unescaped form answers a rule fault that names the
  label.
- A statistic name that looks like an aggregate suffix: the escape
  separates the two, and an unescaped at sign ends the name where the
  parser reads the aggregate marker.
- A value available in one report and absent in the other: the value is
  missing, the line names the report, and the run answers status 2.
- A metric available in one report and multiplexed in the other: the value
  is present, and the row prints `NOISY`.
- A baseline of zero with an absolute threshold only: the absolute test
  answers on its own, and the row can regress.
- A contender value of zero: the delta runs the same way, and an
  improvement never fails.
- A percentile point present in one report and absent from the other: the
  value is missing and the run answers status 2.
- A threaded benchmark: the comparison reads the merged row of the result,
  and the per-context values stay out of the comparison.
- A benchmark name that differs only in its argument list or its thread
  count: the names do not match, and each prints `ADDED` or `REMOVED`.
- A path that holds a comma: the split stands at the first comma, so a
  path with a comma is not supported by this option form.
- Two reports written by different builds of speedgun: the comparison
  prints both versions in its header, and the version texts must still
  match.
- Two reports that declare different time units: the comparison reads the
  stored values, and the table prints both value columns and the absolute
  change in the unit the baseline declares (D-22).
- A rule list where two rules name the same value: both apply, and the
  table prints one row per rule.
- A rule whose filter matches a benchmark the batch filter also excludes:
  the batch filter applies first, and the rule then answers no match.
- A file that parses as JSON but is not a report document: the load call
  answers one error and the gate answers status 2.
- A report whose schema major version this build does not know: the load
  call answers one error naming the version, and the gate answers status
  2.

## Requirements *(mandatory)*

### Compare surface (C-1)

- **FR-001**: The system shall compare a baseline report value with a
  contender report value under a list of rules and answer one comparison
  entry for each matched benchmark and rule, plus one overall verdict.
- **FR-002**: The comparison shall answer its entries in the run order of
  the baseline report, with the added names after them in the
  contender's order.
- **FR-003**: The comparison shall answer one error text, and no partial
  entries, for a rule it cannot read or a value it cannot find.
- **FR-004**: The system shall parse one rule text into a rule value
  through one call that the command surface and a library caller both
  use, so one grammar answers both.
- **FR-005**: The parse call shall answer one error text naming the fault
  for a rule text it cannot read.
- **FR-006**: The comparison shall read report documents only through the
  report load call; no second reader shall exist.
- **FR-007**: The comparison shall run no benchmark, read no clock and
  sample nothing.

### Matching (C-2)

- **FR-008**: The comparison shall match a benchmark of the baseline with
  a benchmark of the contender by the full instance name, the arguments
  and the thread count included.
- **FR-009**: A name present in the contender only shall answer an
  `ADDED` entry, and a name present in the baseline only a `REMOVED`
  entry, each with the state word and no values.
- **FR-010**: Matching shall ignore the order the two documents hold their
  benchmarks in, so a reordered run still matches.
- **FR-011**: A benchmark whose outcome is not measured in either report,
  or whose rows hold no row the rule can read, shall answer a `SKIPPED`
  entry with no values (D-20).

### Value names and rules (C-3)

- **FR-012**: A rule shall name one value of the report, one aggregate for
  a value the report holds as a statistics record, one direction, one
  threshold pair and one optional benchmark filter (D-21).
- **FR-013**: The value name `time` shall name the primary time per
  iteration, the column the result's primary flag selects, and
  `real_time` and `cpu_time` shall name those two columns directly.
- **FR-014**: A value name of the form `p<point>` shall name the
  percentile cell of that point in the primary time column, read through
  the percentile points and the percentile cells the report carries
  (D-16).
- **FR-015**: A value name of the form `counter:<label>`, `metric:<label>`
  or `stat:<name>` shall name the user counter, the counter or formula
  metric, or the custom statistic carried under that label or name.
- **FR-016**: The aggregate suffix shall default to `@median` for a value
  the report holds as a statistics record, and shall answer `@mean`,
  `@min` and `@max` beside it, and the gate shall read the aggregate from
  the statistics record the report carries for that value (D-21).
- **FR-017**: A result that carries one row and no statistics record shall
  answer every aggregate with that row's value (D-18).
- **FR-018**: A percentile value and a custom statistic value shall carry
  one value per benchmark and shall take no aggregate; a written
  aggregate suffix on either shall be a rule fault, and a rule that
  writes none shall read that single value (D-21).
- **FR-019**: Value names, labels and statistic names shall be
  case-sensitive.
- **FR-020**: A prefixed value name shall accept a backslash escape for a
  colon, a comma, an at sign and a backslash inside the label part, and
  the comparison shall read the unescaped label (D-15, PC-7).
- **FR-021**: The rule reader shall take the `counter:`, `metric:` and
  `stat:` prefix before it splits the rest of the text, so a label that
  carries its own colon reads whole.
- **FR-022**: A rule shall carry a relative threshold, an absolute
  threshold, or both; a rule with neither shall be a rule fault.
- **FR-023**: The threshold text shall answer a fraction written as a
  percent, an absolute written with the `abs=` marker, or both written
  together, and any other text shall be a rule fault.
- **FR-024**: The filter field shall use the same pattern rule as the
  batch filter option: a search over the full benchmark name, and an
  invalid pattern shall be a rule fault (PC-8).

### Thresholds and the row verdict (C-4)

- **FR-025**: For a lower-is-better value the delta shall be the
  contender value minus the baseline value, and for a higher-is-better
  value the baseline value minus the contender value.
- **FR-026**: The relative test shall regress a row when the delta over
  the magnitude of the baseline exceeds the relative threshold.
- **FR-027**: The absolute test shall regress a row when the delta
  exceeds the absolute threshold, in the stored unit of the value:
  nanoseconds per iteration for a time column, a percentile cell or a
  statistic that holds a time, percentage points for a statistic that
  holds a percentage, and the counter's own unit for a counter or metric
  column (D-22).
- **FR-028**: A rule with both thresholds shall regress a row only when
  both tests are exceeded.
- **FR-029**: A row whose delta is an improvement shall never regress and
  shall print `IMPROVED`.
- **FR-030**: A row whose delta clears every threshold of its rule shall
  print `OK`.
- **FR-031**: A baseline of zero under a rule that carries a relative
  threshold shall be a rule fault that names the benchmark and the value,
  and the run shall answer status 2.
- **FR-032**: The gate shall read the aggregate row of a result when that
  result carries one, and the single measured row when it carries none.
- **FR-033**: The default rule shall be the median primary time,
  lower-is-better, five percent relative, and it shall apply only when the
  caller gave no rule at all.
- **FR-034**: When the caller gave rules, the default rule shall not be
  added to them.

### Rule validation before the comparison (C-5)

- **FR-035**: The gate shall check every rule against both reports,
  inside the scope of that rule's own filter, before it compares any
  value.
- **FR-036**: A value name that matches no value of a matched benchmark
  within the rule's filter shall stop the run with one line naming the
  rule, the benchmark and the report, and status 2, except a benchmark
  whose rows hold no row the rule can read, which answers the `SKIPPED`
  entry of FR-011 (D-18, D-20).
- **FR-037**: A metric whose availability is not countable in either
  report shall count as a missing value.
- **FR-038**: A percentile point present in one report and absent from the
  other shall count as a missing value.
- **FR-039**: A rule filter that matches no benchmark in either report
  shall stop the run with one line naming the rule and the filter, and
  status 2.
- **FR-040**: A rule whose filter reaches only results holding no
  comparable row shall stop the run with one line naming the rule and the
  benchmark, and status 2 (PC-6, D-20).

### Noise guard (C-6)

- **FR-041**: A benchmark whose report carries a noise warning in either
  document shall mark that benchmark's rows `NOISY`, and the gate shall
  read the warning the document carries (D-17, PC-2).
- **FR-042**: A value whose statistics record exceeds the rule's bound
  shall mark the row `NOISY`: the coefficient of variation against a
  relative threshold, the sample standard deviation against an absolute
  threshold in the stored unit of the value, and either report over its
  bound is enough (D-19).
- **FR-043**: A metric whose running ratio sits below one in either report
  shall mark the row `NOISY`.
- **FR-044**: A `NOISY` row shall not fail the run, and the table shall
  print the reason beside the state word.
- **FR-045**: A strict option shall turn every `NOISY` row into a failure
  and keep the reason in the table.
- **FR-046**: A value that carries no statistics record shall answer the
  noise guard through the carried warning and the running ratio alone.

### Command line (C-7)

- **FR-047**: A compare option shall name a baseline path and a contender
  path, run the comparison, and run no benchmark.
- **FR-048**: The compare option shall split its argument at the first
  comma, and an argument with no comma, or an empty path on either side,
  shall be a usage fault.
- **FR-049**: A repeatable rule option shall carry one rule text in the
  grammar of D-6, and each occurrence shall add one rule.
- **FR-050**: The batch filter option shall limit the benchmarks every
  rule applies to, in compare mode as in run mode.
- **FR-051**: A missing file, a file the reader refuses, a schema version
  mismatch between the two reports, a malformed rule, an unknown value
  name, a missing value and a filter that matches nothing shall each print
  one line and answer status 2.
- **FR-052**: The option table, the usage-error path and the closing
  postcondition of the entry point shall carry the compare mode, and the
  status table of `specs/015-benchmark-harness-core/contracts/cli.md` shall
  gain the rows this mode answers (PC-3).

### Output (C-8)

- **FR-053**: The gate shall print a table with one row per benchmark and
  rule, carrying the benchmark name, the rule, the baseline value, the
  contender value, the change in percent and in absolute terms, and the
  state word.
- **FR-054**: The state words shall be `OK`, `REGRESSED`, `IMPROVED`,
  `NOISY`, `ADDED`, `REMOVED` and `SKIPPED`, and no other word shall
  appear.
- **FR-055**: The table shall follow the column and unit rules the console
  report already uses, and shall print a time value, in each value column
  and in the absolute change, in the unit the baseline report declares
  (PC-5, D-22).
- **FR-056**: The gate shall print a header naming the two files, the
  speedgun version of each report and the schema version of each.
- **FR-057**: A `--json=<path>` option in compare mode shall write the
  comparison as one document, built with the vendored JSON writer, and
  `docs/schema/compare-v1.json` shall describe that document.
- **FR-058**: Two runs of the compare mode over the same two files and the
  same rules shall write byte-identical comparison documents, and a test
  shall check a written document against the comparison schema.

### Main-page documentation (C-9)

- **FR-059**: The main page shall carry a `Regression gate` section of at
  most 25 lines whose snippet comes from one runnable example under
  `example/docs/`.
- **FR-060**: That example shall build and run in CI through a `run_docs_`
  target and an `example_run` entry, with a recorded expected output file,
  and the drift gate shall stay clean over the new section title and its
  owner (PC-9).
- **FR-061**: The quick-reference table shall gain a row for each new
  public name and each new option, each linking the new section.
- **FR-062**: `docs/pages/harness.md` shall carry the detail: the value
  name space, the aggregate forms, the threshold grammar, the state words
  and the noise guard.
- **FR-063**: `PAGE_LINE_CAP` shall rise by the measured size of this
  feature's own section and rows, recorded in the gate's own comment the
  way specs/023 and specs/025 recorded theirs.

### Surface, version, and discipline

- **FR-064**: The comparison calls and their rule, option and result
  values shall be exported from the harness archive and shall follow the
  naming rules of the constitution.
- **FR-065**: The harness `SOVERSION` shall move 8 to 9 for the grown
  public surface, and the `speedgun-ng` archive `SOVERSION` shall stay 2
  (D-14).
- **FR-066**: The project version shall take the next free minor at rebase
  time, 0.15.0 or 0.16.0 by the merge order against N6.
- **FR-067**: The reader extension FR-070 needs shall use the parser's
  error-code form, and each error code it can answer shall carry its own
  test.
- **FR-068**: The comparison document shall be written with the vendored
  JSON writer, and no new library shall enter the tree.
- **FR-069**: Every line and branch of the new code shall be covered, with
  no exclusion marker, no waiver row and no partial row.
- **FR-070**: The gate shall read the noise warning a report carries, so
  the reader shall answer it to the caller as one optional record per
  result kept beside the loaded results, holding the values the file
  carries (PC-2, D-17).
- **FR-071**: The tests shall use report fixture files and report values
  built in code, with no benchmark run, no sleep and no clock, and every
  value kind, aggregate, direction, threshold form, filter, fault and exit
  branch shall run.
- **FR-072**: The time-source gate, the loop-shape gate and the
  entry-point shape gate shall stay green, and no per-iteration path shall
  change (PC-11).
- **FR-073**: The report reader shall keep accepting a key it does not
  recognize wherever the report schema leaves an object open, so a field
  another lane adds loads without fault (PC-4).
- **FR-074**: A comparison document the file system refuses shall print
  one line naming the path and answer status 2 (PC-3).

## Key Entities *(include if feature involves data)*

- **Report pair**: the baseline document and the contender document, each
  read through the load call, each naming its own speedgun version and
  schema version.
- **Value name**: one text that names one value a report holds: a time
  column, a percentile cell, a counter column, a metric column or a custom
  statistic, with the label escape of D-15.
- **Aggregate**: which summary of that value the rule reads, `median`,
  `mean`, `min` or `max`, read from the statistics record the report
  carries.
- **Direction**: whether a larger value is better or a smaller one is.
- **Threshold pair**: the relative fraction and the absolute floor, at
  least one of them present, the floor written in the stored unit of the
  value (D-22).
- **Rule filter**: the benchmark pattern one rule applies to, in the
  pattern rule of the batch filter option.
- **Comparison entry**: one benchmark under one rule, with both values,
  the change in percent and in absolute terms, the state word and, for a
  `NOISY` row, the reason.
- **State word**: one of `OK`, `REGRESSED`, `IMPROVED`, `NOISY`, `ADDED`,
  `REMOVED`, `SKIPPED`.
- **Verdict**: the one answer of the comparison: pass, or the rows that
  failed.
- **Noise reason**: the account a `NOISY` row prints: the carried warning,
  the statistics bound the value exceeded, or the multiplexed metric.
- **Comparison document**: the written form of the table, described by
  `docs/schema/compare-v1.json`, carrying the two inputs, the rules
  applied, one entry per row and the verdict.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A gate run over a report pair whose median primary time
  differs by 3% answers status 0 with every row `OK`, and a pair that
  differs by 9% answers status 1 with that row `REGRESSED` and the change
  printed in percent and in absolute terms. A pair whose baseline also
  holds a complexity fit entry answers status 0 with that entry `SKIPPED`
  and no fault (D-20).
- **SC-002**: A gate run with no rule applies exactly one rule, the median
  primary time at five percent relative, and a gate run with two rules
  applies exactly those two.
- **SC-003**: Every value kind of the name space answers a comparison: the
  primary time, each named time column, a percentile point, a counter
  label, a metric label and a statistic name, each under each aggregate it
  accepts, in both directions, with each threshold form.
- **SC-004**: Each fault form (a malformed rule text, a missing threshold,
  an unknown value name, a missing value in one report, a filter that
  matches nothing, a zero baseline under a relative threshold, a missing
  file, an unreadable file, a schema version mismatch) answers exactly one
  line naming the fault and status 2, and no comparison table prints.
- **SC-005**: A compare-mode run loads two files and runs no benchmark: the
  registry runs nothing, no clock is read, and the run answers its status
  from the comparison alone.
- **SC-006**: A `NOISY` row never fails the run with strict mode off and
  always fails it with strict mode on, and the printed reason names which
  of the three noise conditions applied: the carried warning, the
  statistics bound the value exceeded, or the running ratio (D-19).
- **SC-007**: A test suite over report values built in code and fixture
  files under `test/data/` covers every value kind, aggregate, direction,
  threshold form, filter, fault and exit branch with no benchmark run, no
  sleep and no clock.
- **SC-008**: The coverage gates report 100% line and 100% branch over the
  new code with no exclusion marker.
- **SC-009**: Two compare-mode runs over the same pair and rules write
  byte-identical comparison documents, and a test checks a written
  document against `docs/schema/compare-v1.json`.
- **SC-010**: The loop-shape gate reports the same per-iteration
  instruction cycle as it reports at `aec8fce` under both compilers it
  drives, and the entry-point shape gate reports no finding.
- **SC-011**: The main page's `Regression gate` section runs at most 25
  lines, its snippet comes from the new example, its recorded output
  matches the example's expected file through `example_run`, and
  `python3 tools/docs/example_drift.py .` reports no finding.
- **SC-012**: The full gate set passes on the branch: the `ci-ubuntu` and
  `ci-rocky` builds, `ctest`, `ci-sanitize`, `ci-tsan`, clang-tidy and
  cppcheck with no new finding, `format-check`, `spell-check`,
  `prose-lint`, the time-source gate, the page-drift gates, the DBC gates,
  and the coverage gates.
- **SC-013**: A gate run over a pair whose `schemaVersion` texts differ
  answers one line naming both versions and status 2.

## Constraints

- **Counters-only.** Every time and counter value comes from
  `sg::counters`, and `test/time_source_gate.sh` stays clean (Additional
  Constraints of the constitution). The gate reads no clock and samples
  nothing (FR-007).
- **One reader.** A report document reaches the gate through the load call
  of specs/023 and through nothing else (FR-006).
- **No timed-path change.** The feature touches no per-iteration code, so
  the loop-shape gate stays at its base reading (FR-072).
- **Exit statuses.** Status 0 answers a pass, status 1 answers one or more
  regressions, and status 2 answers a usage, rule, load or schema fault.
  Each fault prints exactly one line.
- **Contracts at the boundary.** A bad option or rule text on the command
  line is a usage fault that prints one line and answers status 2; a bad
  argument on the comparison call is a contract violation.
- **Coverage.** 100% line and 100% branch, with no `LCOV_EXCL` marker under
  `source/harness/` (FR-069).
- **Test discipline.** Report fixture files and values built in code; no
  sleep, no clock, no benchmark run in a gate test (FR-071). `ctest -j 2`
  stays the runner. The coverage preset keeps `-fprofile-update=atomic` and
  stays free of `-D_GLIBCXX_ASSERTIONS`.
- **Vendored privacy holds.** The install tree, the package files and the
  dynamic symbol table stay free of simdjson, and the comparison document
  uses the vendored writer (FR-068).
- **Docs budget.** `SECTION_LINE_CAP` stays 25; the page cap moves by the
  measured amount through a recorded decision (FR-063).
- **No host assumptions.** No test names a cpu count or a host size, and
  every test passes on a four-cpu CI runner without skipping.
- **Prose and commits.** Principle XI and the commit lint bind every word
  and every commit; commit messages carry blank lines between body
  paragraphs and one footer block with `Approved-by:` then `Refs:`.
- **Rebase discipline.** Rebase, push with `--force-with-lease`, and show
  `git ls-remote origin <branch>` after each push.

## Scope

In: FR-001 to FR-074. The comparison surface, the rule grammar and its
parse call, the value name space with its label escape, the default rule,
the compare mode on the command line, the noise guard, the console table,
the comparison document and its schema file, the reader extension that
answers the carried noise warning, the main-page section with its runnable
example, the quick-reference rows, the `docs/pages/harness.md` detail, the
tests, and the version effect.

Out, each owned by a later roadmap item or another lane:

- the full command line for the gate, option files, environment defaults
  and a rules file (N7);
- report generators, histograms and plots, and any use of the vendored
  HdrHistogram (N8);
- multi-pass fields of the report document (N6, corsairone), under the
  pass-field rule above;
- percentile cells of the other time column or of a metric column, which
  need a column qualifier the grammar cannot carry while a label may hold
  a colon (D-16);
- per-context values of a threaded run: the comparison reads the merged
  row;
- statistical tests beyond the two thresholds and the noise guard, a
  Mann-Whitney U test included;
- a report writer of any other format, and any change to the fields of the
  report document;
- a gate that runs the benchmarks itself: the compare mode loads two
  files, and the job that produces them stays with CI and N7.

## Assumptions

- The roadmap file the prompt names sits outside this repository and is
  absent from this machine, so no claim here rests on it. The prompt text
  is the authority for scope.
- The comparison reads the merged row of a result, because the report
  holds one row per repetition and one aggregate, and the per-context
  columns answer a policy question this gate does not ask.
- A rule applies to a benchmark pair, and the pair's row selection follows
  the aggregate row rule FR-032, so a repeated run is compared on its
  aggregate and a single run on its one row.
- The unit of an absolute threshold is the stored unit of the value that
  D-22 names: nanoseconds per iteration for a time column, a percentile
  cell or a `TIME` statistic, percentage points for a `PERCENTAGE`
  statistic, and the counter's own unit for a counter or metric column.
- The two reports come from builds that print the same value kinds; the
  version header of the table lets a reader see when they do not.
- Fixture files under `test/data/` reach a test through a path the build
  passes to it, the shape `test/CMakeLists.txt` already uses for the script
  gates (PC-10).
- A statistic name that collides with a built-in aggregate name cannot
  reach a report, because the statistic attach call refuses those seven
  names (PC-7).
- Threading tests build report values in code, so no test needs hardware, a
  PMU or privileges.

## Budget note

`README.md` stands at 713 lines of a 713 cap, and `PAGE_LINE_CAP` in
`tools/docs/example_drift.py` is 713 (PC-9), so the `Regression gate`
section and its quick-reference rows cannot fit as they stand. The cap
moves by the measured amount when the section lands, the smallest fitting
amount, recorded here and in the gate's own comment, the way specs/023
recorded the move from 692 to 713. `SECTION_LINE_CAP` stays 25, prose is
trimmed first, no snippet is dropped, and no earlier spec is edited. The
new section title joins the `SECTIONS` list of the gate, and its snippet
joins `SECTION_OWNERS` with `regression_gate.cpp` as its owner. If a
rebase brings N6's section in first, this feature's cap move is measured
against the page as the rebase leaves it.

## Dependencies

- The report surface of specs/023: `loadReport`, `ReportData`, the reader of
  `source/harness/json_load.cpp`, the writer of `source/harness/json_report.cpp`
  and the schema file `docs/schema/report-v1.json` (PC-1, PC-4).
- The result types the comparison reads: `BenchmarkResult`, `ResultRow`,
  `Statistics`, `MetricValue`, `PercentileValue`, `StatisticValue`,
  `RunOutcome`, `RowKind` and `Availability` in the public headers (PC-2,
  PC-6).
- The noise predicate and its record: `NoiseWarning` and `noiseWarningOf`
  of `source/harness/detail/json_report.hpp` (PC-2).
- The entry point and the option scanner: `speedgunMain`, the long option
  table and the usage-error path of `source/harness/cli.cpp`, and the
  status table of `specs/015-benchmark-harness-core/contracts/cli.md`
  (PC-3).
- The console printers and the unit scale this feature mirrors:
  `source/harness/report.cpp` (PC-5).
- The aggregate row and statistics builder: `source/harness/runner.cpp`
  (PC-6).
- The pattern rule of the batch filter: `source/harness/cli.cpp` (PC-8).
- The vendored simdjson: the reader's DOM error-code form and the writer's
  `builder::string_builder`, under the privacy rules of specs/004 FR-013 to
  FR-017.
- D1's main-page structure and drift gate: `README.md`,
  `tools/docs/example_drift.py` (`SECTIONS`, `SECTION_OWNERS`,
  `PAGE_LINE_CAP`, `SECTION_LINE_CAP`, `SECTION_LINE_CAPS`), and the
  example run machinery (`add_docs_example`, the `run_docs_` targets and
  `run-examples` in `example/CMakeLists.txt`) (PC-9).
- The test machinery: the target lists of `test/CMakeLists.txt`, the
  `example_run` entry, and the report-test precedent of
  `test/source/harness_json_report_test.cpp` (PC-10).
- The constitution gates of Principle VIII: the counters-only scan, the
  loop-shape gate, the entry-point shape gate, the naming rules, Principle
  II, IV.1, the coverage gates, `ci-sanitize`, `ci-tsan`, clang-tidy and
  cppcheck, the DBC gates, and the prose and commit lint.

## Citations

Symbol and section anchors, each read at the audit point `aec8fce`.

| Artifact | Location |
| --- | --- |
| `loadReport` | `include/speedgun-ng/benchmark.hpp:2330` |
| `ReportData` | `include/speedgun-ng/benchmark.hpp:553` |
| `ResultRow` and its fields | `include/speedgun-ng/benchmark.hpp:352` |
| `BenchmarkResult` and its fields | `include/speedgun-ng/benchmark.hpp:441`, `:468` |
| `Statistics` | `include/speedgun-ng/benchmark.hpp:231` |
| `MetricValue` | `include/speedgun-ng/benchmark.hpp:212` |
| `StatisticValue` | `include/speedgun-ng/benchmark.hpp:274` |
| `PercentileValue` | `include/speedgun-ng/benchmark.hpp:338` |
| `RunOutcome`, `RowKind` | `include/speedgun-ng/benchmark.hpp:77`, `:88` |
| `Availability` | `include/speedgun-ng/counters_core.hpp:87` |
| `addMetric`, `addStatistic` | `include/speedgun-ng/benchmark.hpp:1758`, `:1786` |
| Reader: schema version check | `source/harness/json_load.cpp:1117` to `:1128` |
| Reader: open object key rule | `source/harness/json_load.cpp:1000` to `:1030` |
| Reader: noise record read and dropped | `source/harness/json_load.cpp:729` to `:743` |
| Writer: schema version stamp | `source/harness/json_report.cpp:28`, `:530` |
| `NoiseWarning`, `noiseWarningOf`, `runnableCpuCount` | `source/harness/detail/json_report.hpp:137`, `:154`; `source/harness/json_report.cpp:472` to `:481` |
| Aggregate row, time statistics, metric and counter statistics | `source/harness/runner.cpp:2163`, `:2185` to `:2196`, `:2229`, `:2262` |
| Counter label source | `source/harness/runner.cpp:2131` |
| Complexity fit rows | `source/harness/runner.cpp:2436` to `:2466` |
| `speedgunMain`, option table, usage errors, closing postcondition | `source/harness/cli.cpp:290`, `:321` to `:344`, `:80`, `:132`, `:152`, `:174`, `:276`, `:405`, `:419`, `:435`, `:448`, `:462`, `:498`, `:518`, `:548`, `:646` |
| Batch filter pattern rule and its no-match answer | `source/harness/cli.cpp:470` to `:474`, `:529` to `:536`, `:545` to `:550` |
| Console printers and unit scale | `source/harness/report.cpp:70`, `:88`, `:164`, `:193`, `:234`, `:257`, `:309`, `:331` |
| Percentile column order | `source/harness/report.cpp:282` to `:296` |
| Report schema top keys | `docs/schema/report-v1.json:7` |
| Exit status table of the CLI contract | `specs/015-benchmark-harness-core/contracts/cli.md:83` to `:101` |
| Version and `SOVERSION` | `CMakeLists.txt:7`, `:47`, `:247` |
| Drift gate caps and section list | `tools/docs/example_drift.py:30` to `:54`, `:59`, `:132`, `:133` |
| Main page and quick-reference table | `README.md` (713 lines), `README.md:597` |
| Harness guide | `docs/pages/harness.md` (1093 lines), `:1018` |
| Docs example targets | `example/CMakeLists.txt:45` to `:47` |
| `example_run` entry | `test/CMakeLists.txt:1136` to `:1139` |
| Source-dir path passed to a test command | `test/CMakeLists.txt:82` |
| Report test file read-back precedent | `test/source/harness_json_report_test.cpp:41` to `:53` |
| Entry-point shape gate anchors | `test/cli_shape.sh:31` to `:41` |
| Timed-loop shape gate | `test/loop_shape.sh` |
| Vendored include and privacy rules | `specs/004-vendor-simdjson/spec.md` FR-013 to FR-017 |
| JSON report artifacts this feature builds on | `specs/023-json-reports/spec.md` |
| Constitution IV.1, VIII gates, II, V.1, XI, counters-only, footer | `.specify/memory/constitution.md` |

## Open questions

The prompt carried its open items as recommendations, and Charles accepted
each on 2026-10-20, so the decisions stand at D-1 to D-14 and again at the
FR that carries them.

Three items derived at this audit point stay for Charles to overturn.
Each already stands encoded in the FR that carries it, so what stays open
is his ruling over the recommendation, and the spec text holds that
recommendation as written:

- **D-15 label escape**: labels carry no character rule on `master`
  (PC-7), so a colon, a comma or an at sign can sit inside one. The
  recommendation is a backslash escape inside the label part of a prefixed
  value name.
- **D-16 percentile columns in scope**: the report holds percentile cells
  per column in positional order (PC-5), and the grammar cannot carry a
  column qualifier while a label may hold a colon. The recommendation is
  that N5 names the primary time column's cells and N7 extends the
  grammar.
- **D-18 statistics cover the primary column only**: the report holds no
  statistics record for the other time column (PC-6). The recommendation is
  that an aggregate named on such a column in a repeated run is a missing
  value.

The clarification session of 2026-10-20 settled the other two derived
items and added three more, each sourced at its `- Q:` bullet: where the
loaded value answers the carried warning (D-17), the statistics field the
noise guard holds against each threshold form (D-19), the answer of a
result with no readable row (D-20), the default aggregate beside a name
that carries one value (D-21), and the unit an absolute threshold stands
in (D-22). Those five are this session's decisions and carry no owner
approval.

Three code-level names still wait for the plan, and none is an open
question of behavior: the spelling of the comparison call and its parse
call, the names of the rule, option and result values, and the name of
the new translation units.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature adds a user-visible mode
  and new public names, so FR-059 to FR-063 carry the `Regression gate`
  section, its `example/docs/` region with its `run_docs_` target and
  `example_run` entry, its quick-reference rows, and the
  `docs/pages/harness.md` detail in the same change, inside the budget the
  budget note records.
