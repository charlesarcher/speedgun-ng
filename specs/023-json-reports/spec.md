# Feature Specification: JSON Reports

**Feature Branch**: `023-json-reports`

**Created**: 2026-10-20

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"JSON reports, item N3 (old H7) of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md`. Short name: json-reports.
Target machine: gary. Lane: harness (N1 done, N3 now, then N5, N7). It
runs in parallel with N2 (PR #41, corsairone) and N4 (thrawn). The
harness writes one JSON file for a run. The file holds the results, the
statistics, the N1 percentiles, the raw points when the user asks for
them, and the context disclosure. A later run can load the file again.
N5 (regression gate) compares two of these files, so the schema must be
stable and versioned. The console report does not change. [...]"

## Clarifications

### Session 2026-10-20

- Q: With no JSON Schema validator in the tree and no new library
  allowed, what answers the claim that a test checks a written file
  against the schema? → A: the test reads the schema file and checks
  the written document's key set, key order and value kinds against it
  with a small in-house checker, and no validator library enters the
  tree (FR-008).
- Q: When a benchmark failed and the report write also failed, which
  exit status does the process answer? → A: status 2 answers, and the
  status table of `contracts/cli.md` gains that precedence row
  (FR-030, FR-050).
- Q: What does the public load call answer: the run's own result types
  or a read-only view of them? → A: the same result types the run
  produces, so a loaded value writes back byte for byte (FR-022,
  FR-026).
- Q: Where do the retained chunk value columns live? → A: beside the
  result, in a record the runner answers the writer, and the public
  layout of the result types keeps its fields (D-12, FR-018, FR-044).
- Q: Is the file written when SIGINT ends the batch early? → A: yes,
  the results that completed are written before the exit status is
  answered (FR-002, FR-004).

## Run-time binding

This spec binds no spec number, commit, or source line beyond the base
it records here.

- **Spec number**: 023. `master` at `c7f1769` carries `specs/001`
  through `specs/021`, and 021 belongs to the merged N1 spec. The open
  pull request #41 branch `021-metric-formulas` carries
  `specs/021-metric-formulas`, which collides with that merged number;
  Charles directed on 2026-10-20 that PR #41 rebase onto `c7f1769` and
  take spec number 022, so 022 stays reserved for N2 and 023 is the
  next number free on `master` and on every open pull request branch.
  A rebase that finds 023 taken takes the next free number and records
  the change here.
- **Branch base**: `c7f1769` (2026-10-19), the head of `master` at
  branch creation: version 0.11.0 (`CMakeLists.txt:7`), harness
  `SOVERSION` 7 (`CMakeLists.txt:225`), `speedgun-ng` archive
  `SOVERSION` 2 (`CMakeLists.txt:47`).
- **Names**: every symbol named below is an anchor read at the audit
  point. A plan re-reads each one on the base it runs against and
  records any drift.

## Decisions

Charles accepted D-1 to D-11 and D-13 on 2026-10-20 as recommendations
of the prompt this spec comes from. They are governance for this
feature, and each is recorded again at the FR that carries it. D-12 is
derived at this audit point from D-6 and the code fact PC-3; Charles has
not ruled on it, and it stands as a recommendation he can overturn.

- **D-1 Library**: the vendored simdjson writes the report and reads it
  back. No new library enters the tree, and no in-house emitter is
  written. Where the pinned version lacks a call the writer needs, the
  work stops and reports the gap; the submodule does not move without
  approval (PC-5, FR-047).
- **D-2 One file per run**: one document holds every benchmark of one
  entry-point invocation, in run order (FR-001, FR-002).
- **D-3 Round trip**: a test writes a report, loads it, writes it
  again, and checks the two files byte for byte (FR-026, SC-002).
- **D-4 Schema version**: a `schemaVersion` field of `1.0` stands at
  the top of the document. The schema itself lives at
  `docs/schema/report-v1.json` in JSON Schema draft 2020-12. A minor
  change adds fields; a major change breaks the loader on purpose
  (FR-007, FR-008, FR-010).
- **D-5 Percentiles in the file**: the percentile points and values of
  a run that computed them always stand in the file, each with its
  availability, and an unavailable value stands as `null` (FR-015).
- **D-6 Raw chunk values**: the option `--json-raw=none|chunks` selects
  whether the per-context chunk values and their iteration count stand
  in the file, and the default is `none`. The console report keeps its
  bytes at either value (FR-018, FR-019).
- **D-7 Context section**: each benchmark entry carries the wait mode,
  the placement text, the applied flag, and the noise warning, with the
  values the console prints (FR-016, FR-017).
- **D-8 Output option**: `--json=<path>` names the file. No option
  means no file, so a run today behaves exactly as it does now. A bad
  option value stops before any benchmark runs; a path the file system
  refuses reports one line after the console report and exits with
  status 2 (FR-028, FR-030, FR-031).
- **D-9 Fixed order and one number format**: key order follows the
  schema and the field order of the C++ types, benchmarks keep run
  order, and every number uses one fixed format, so two runs of the
  same values give a clean diff (FR-009, FR-011).
- **D-10 Timed window**: no allocation and no I/O stands inside a
  timed window. The writer runs after every benchmark finishes and
  every context joins (FR-001, FR-033, FR-034).
- **D-11 Loader errors**: a missing required field, a wrong type, or an
  unknown major version gives one clear error, and a command surface
  that loads a bad file exits with status 2 (FR-023, FR-024, FR-027).
- **D-12 Chunk value retention**: the chunk value columns of a run
  reach the writer beside the result, in a record the runner answers
  when the writer asks, because at the base those columns exist only
  inside the runner and disappear once the percentile columns are built
  (PC-3). The raw option turns that retention on, following the 021
  precedent that the chunk capture is gated on the output that reads
  it, and the public layout of `ResultRow` and `BenchmarkResult` keeps
  its fields (FR-018, FR-020, FR-044).
- **D-13 Version and merge order**: N3 takes the next free minor
  version after every feature that merges before it, and the harness
  `SOVERSION` moves 7 to 8 for the grown public surface of the harness
  archive. The `speedgun-ng` archive `SOVERSION` stays 2 (FR-043,
  FR-044).

## Audit point

Read on 2026-10-20 at `c7f1769` on Gary, the machine this feature
develops on: one socket, one memory node, 16 cores and 32 hardware
threads, two L3 groups. No claim in this spec depends on that shape;
the threading tests run on the fake provider.

## Preconditions

### PC-1: N1 percentiles and raw capture merged. PASS.

`c7f1769` is the N1 head, and `specs/021-percentiles-raw-capture/`
carries its closed artifacts. The version reads 0.11.0 at
`CMakeLists.txt:7`, the harness `SOVERSION` reads 7 at
`CMakeLists.txt:225`, and the archive `SOVERSION` reads 2 at
`CMakeLists.txt:47`. This feature inherits the percentile fields and
the chunk cadence N1 landed.

### PC-2: the result types the file carries. PASS, with a wider field list than the prompt names.

- `PercentileValue { available, value }`:
  `include/speedgun-ng/benchmark.hpp:337`.
- `ResultRow`: `include/speedgun-ng/benchmark.hpp:351`. Its fields are
  `kind`, `iterations`, `timePerIterationNs`, `cpuTimePerIterationNs`,
  `endpointSamples` (`:364`), `pauseSamplesFolded` (`:368`),
  `timeAvailability` (`:372`), `metrics` (`:374`), `counters`,
  `counterDisclosure` (`:381`), `label` (`:386`), `fit` (`:390`),
  `overheadFloorNs` (`:391`), `percentiles` (`:398`), `chunkCount`
  (`:401`), `chunkIterations` (`:402`), `timeStats` (`:403`) and
  `metricStats` (`:404`).
- `ContextDisclosure { waitMode, placement, applied }`:
  `include/speedgun-ng/benchmark.hpp:421`.
- `BenchmarkResult`: `include/speedgun-ng/benchmark.hpp:440`. Its
  fields are `name`, `suite`, `caseName`, `outcome`, `rows`, `reason`
  (`:452`), `metricLabels`, `counterLabels`, `counterStats` (`:463`),
  `statisticValues` (`:468`), `hideRepetitionRows` (`:473`),
  `timeUnit` (`:480`), `pauseSamplesFolded` (`:483`), `contexts`
  (`:488`), `percentilePoints` (`:492`), `primaryIsCpu` (`:498`) and
  `contextPercentiles` (`:502`).

The prompt's list omits `reason`, `hideRepetitionRows`,
`pauseSamplesFolded`, `statisticValues` and `counterStats` on
`BenchmarkResult`, and `endpointSamples`, `pauseSamplesFolded`,
`timeAvailability`, `counterDisclosure`, `label`, `overheadFloorNs`,
`timeStats` and `metricStats` on `ResultRow`. The schema covers what the
code carries, so a field the console prints has a home in the file
(FR-013, FR-014).

### PC-3: the percentile helpers, and where the chunk values live. PASS, and the answer is that they do not live after the run.

- The helpers: `buildChunkValues`
  (`source/harness/detail/percentiles.hpp:30`), `minSamplesFor` (`:44`),
  `percentileAt` (`:54`), `normalizePercentilePoints` (`:64`) and
  `buildPercentileColumns` (`:78`).
- The single-context path builds the chunk value columns as a local at
  `source/harness/runner.cpp:271`, scales them with
  `divideChunkValues` at `:317` and feeds them to
  `buildPercentileColumns` at `:319`.
- The threaded path keeps them in a fold record at
  `source/harness/runner.cpp:1464` to `:1512`, copies them into a local
  `mergedValues` at `:494` to `:505`, and answers the merged and
  per-context percentile columns at `:527` and `:531`.
- What survives the run is the percentile values
  (`include/speedgun-ng/benchmark.hpp:398`, `:502`) and the pair
  `chunkCount` and `chunkIterations` (`:401`, `:402`). No result field
  holds a chunk value column.

`--json-raw=chunks` therefore needs a home for those columns in the
result value, and D-12 records that. The recorder itself is a per-attempt
object the run releases, so no later pass can rebuild the columns.

### PC-4: the console printers, the option scanner, and the exit-status rule. PASS.

- The printers in `source/harness/report.cpp`:
  `printContextDisclosure` (`:173`), `printNoiseWarning` (`:227`),
  `runnableCpuCount` (`:203`), `printContext` (`:287`),
  `printPercentileLine` (`:299`), `printRowPercentiles` (`:351`) and
  `printResult` (`:373`).
- The context header prints the project version and the build type and
  nothing else (`source/harness/report.cpp:291`): no host, no cpu
  model, no date. The file follows it and carries no run date (FR-005).
- The scanner in `source/harness/cli.cpp`: `speedgunMain` (`:218`), the
  long option table (`:249`), the usage-error exits (`:67`, `:119`,
  `:139`, `:161`) and the unknown-option exit with status 2 (`:332`).
  `RunOptions` stands at `source/harness/detail/internal.hpp:324`.
- The entry point closes with the postcondition
  `SG_ENSURE((status != 0) == (anyFailed || anyInterrupted), ...)` at
  `source/harness/cli.cpp:415`, and the status table of
  `specs/015-benchmark-harness-core/contracts/cli.md:73` lists no
  status 2 row for a report write. A write failure that exits 2 moves
  both (FR-050).

### PC-5: the JSON library on master. PASS, with two build facts.

- `external/simdjson` is the only JSON library on `master`, pinned at
  4.6.11 (`external/simdjson/include/simdjson/simdjson_version.h:7`),
  imported by `import_simdjson()` (`CMakeLists.txt:109`) and merged
  into `libspeedgun-ng.a` by `simdjson_archive_merge`
  (`CMakeLists.txt:170`). The version tripwire stands at
  `source/simdjson/simdjson_gate.cpp:12`. No nlohmann, glaze,
  rapidjson, yyjson or fetched JSON library stands in the tree. Quill
  carries a JSON sink for log lines, and yaml-cpp writes YAML, so
  neither is a report writer.
- The writer the prompt names exists in the pinned tree:
  `simdjson::SIMDJSON_IMPLEMENTATION::builder::string_builder`
  (`external/simdjson/include/simdjson/generic/builder/json_string_builder.h:51`)
  with `start_object`, `end_object`, `start_array`, `end_array`,
  `append_key_value`, `append`, `append_raw`, the escaped string forms,
  `view()` and `c_str()`. The static-reflection builder needs C++26
  reflection and stays out of this feature.
- A number written by that builder goes through
  `simdjson::internal::to_chars`
  (`external/simdjson/include/simdjson/generic/builder/json_string_builder-inl.h:749`),
  a Grisu2 digit-generation writer
  (`external/simdjson/src/to_chars.cpp:701`, `:761`). It round-trips
  every value it writes and stays within 17 digits; the shortest digit
  count of the wider interval is not its guarantee. FR-011 states the
  property the feature needs: round-trip equality and byte stability.
- Build fact one: `specs/004-vendor-simdjson/spec.md:109` (FR-013)
  allows simdjson headers in `source/` translation units and prefers
  one internal wrapper unit. A translation unit under
  `source/harness/` satisfies that rule; the comment at
  `source/simdjson/simdjson_gate.cpp:1` records the single-unit reading
  that held when the vendor spec landed, and the plan decides whether
  N3 adds a second unit of its own.
- Build fact two: the vendored include path reaches the harness
  nowhere today. `CMakeLists.txt:161` links simdjson to
  `speedgun-ng_speedgun-ng` as a private build-interface dependency, and
  `speedgun-ng_harness` links only `speedgun-ng::speedgun-ng`
  (`CMakeLists.txt:197`). The build must give the harness the vendored
  headers while keeping specs/004 FR-014, FR-015 and FR-016: no
  simdjson file in the install tree, no simdjson target or path in the
  package files, and no simdjson symbol in the dynamic symbol table
  (FR-048).

## Roadmap position

N3 of the harness roadmap, the item the roadmap numbered H7. It sits in
the middle of the gary lane: N1 landed, N3 runs now, then N5 comparison,
then N7, the path to the CI gate. The percentile values N1 landed are
the tail this file carries, and the file itself is the input N5
compares.

Two features run in parallel on other machines: N2 metric formulas on
corsairone (PR #41) and N4 topology placement on thrawn. The three are
orthogonal by design.

The roadmap file the prompt names sits outside this repository and is
absent from this machine, so no claim here rests on it. The prompt text
is the authority for scope.

## Parallel work and the version rule

- **Shared files**: the project `VERSION` in `CMakeLists.txt`, the two
  `SOVERSION` lines, `README.md`, the quick-reference table,
  `PAGE_LINE_CAP` in `tools/docs/example_drift.py`,
  `example/CMakeLists.txt`, the `example_run` list, and the
  `test/CMakeLists.txt` target lists. `master` at `c7f1769` carries no
  changelog file.
- **Ownership against N2**: PR #41 edits the counters library, the
  `--catalog` path, and the route that fills `ResultRow::metrics` and
  `BenchmarkResult::metricLabels`. This feature reads those fields and
  writes them into the schema; it changes no type and no producer
  (FR-049).
- **Ownership against N4**: N4 edits the `ContextDisclosure` text,
  `printContextDisclosure` in `source/harness/report.cpp`, and
  `source/harness/cli.cpp`. This feature writes the disclosure fields
  into the schema and puts its writer and reader in new files under
  `source/harness/`; it edits no console printer (FR-049).
- **Conflict rule**: rebase onto the new `master`; no merge. Keep the
  lines of both features in every shared list, take the next free minor
  version, raise `PAGE_LINE_CAP` again by the size of this feature's own
  section, then run the full gate set and push once with
  `--force-with-lease`, showing `git ls-remote origin <branch>` after
  the push.
- **Field drift rule**: if N2 or N4 added a field to `ResultRow`,
  `BenchmarkResult` or `ContextDisclosure` before this feature rebases,
  the schema gains that field, the minor schema version rises, and the
  round-trip test covers it (FR-010).
- **Version rule**: D-13. PR #41 landed 0.12.0 and PR #42 landed 0.13.0,
  so this feature takes 0.14.0. The harness `SOVERSION`
  moves 7 to 8, and the archive `SOVERSION` stays 2 (FR-043, FR-044).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Get a machine-readable report of a run (Priority: P1)

A developer wants the numbers of a run somewhere a program can read
them. They add `--json=run.json` to the command line, run the suite as
they always did, and find one file with every benchmark of that run:
the rows, the aggregate statistics, the percentile points N1 landed,
the labels, and the context disclosure. The console report prints
exactly what it printed before.

**Why this priority**: this is the item in one sentence. Every later
roadmap item that reads a measurement (N5 comparison, N8 plots) waits on
this file, and the schema has to exist before any of them can start.

**Independent Test**: run one docs example with `--json=` pointing at a
path in a writable directory, then check the file against
`docs/schema/report-v1.json` and against the values the same run
printed. Every benchmark the console printed appears once, in run
order, with the same values.

**Acceptance Scenarios**:

1. **Given** a suite and `--json=<path>`, **When** the run finishes,
   **Then** one JSON document stands at that path and validates against
   the schema of the repository.
2. **Given** the same run, **When** the file and the console report are
   compared, **Then** every benchmark and every printed value of the
   console appears in the file with the same value.
3. **Given** a run with no `--json` option, **When** the run finishes,
   **Then** no file appears and the console bytes match the base
   commit.
4. **Given** a run whose benchmark failed, **When** `--json` names a
   path, **Then** the file carries that benchmark with its outcome and
   its reason, and the exit status follows the table of
   `contracts/cli.md`.

---

### User Story 2 - Read a report back (Priority: P1)

A developer loads yesterday's file and gets the same C++ values the run
produced. A test writes a file, loads it, writes it again, and the two
files match byte for byte, so the loader and the writer agree and the
format loses nothing.

**Why this priority**: N5 compares two files, and a comparison is only
as sound as the load that produced the second side. A writer with no
reader is a format nobody can prove.

**Independent Test**: write a report for a scripted suite, load it
through the public call, write it again with the same settings, and
compare the two files byte for byte.

**Acceptance Scenarios**:

1. **Given** a written report, **When** the public call loads it,
   **Then** the result values carry the name, outcome, rows, labels,
   percentile points and context disclosure the file describes.
2. **Given** that loaded value, **When** the writer writes it again
   with the same raw setting, **Then** the second file matches the
   first byte for byte.
3. **Given** a file whose every percentile value stands as `null`,
   **When** the loader reads it, **Then** each of those values answers
   with its availability false.

---

### User Story 3 - Put the raw chunk values in the file (Priority: P2)

A developer chasing a tail wants every chunk value beside the ranks
computed from them. `--json-raw=chunks` writes the per-context chunk
value columns beside `chunkCount` and `chunkIterations`. The default
`none` keeps the file small, and the console report looks the same at
either value.

**Why this priority**: the ranks cover the common diagnosis, so the raw
column is control and volume, and it waits behind the file itself. It
also carries the one storage decision this audit point derived (D-12).

**Independent Test**: run one scripted suite twice, once with
`--json-raw=none` and once with `--json-raw=chunks`, and check that the
second file carries the chunk columns of every context while the first
carries none, and that the console bytes match between the two runs.

**Acceptance Scenarios**:

1. **Given** `--json-raw=chunks`, **When** the run finishes, **Then**
   each row carries the chunk value columns in the column order the
   percentile columns use.
2. **Given** `--json-raw=none`, **When** the run finishes, **Then** the
   file carries no chunk value column.
3. **Given** `--json-raw=chunks` with percentile output off, **When**
   the run finishes, **Then** the chunk values are captured and written
   while the console prints no percentile line.
4. **Given** either value, **When** the console report prints, **Then**
   its bytes match the report of the same run with no JSON option.

---

### User Story 4 - Get one clear line when the file or the option is wrong (Priority: P2)

A mistyped `--json-raw=chunk` stops the run before any benchmark
executes. A path in a directory that does not exist runs the benchmarks,
prints the report, then says one line and exits with status 2. A file
with an unknown schema version, a missing field, or a field of the
wrong type gives one error that names what it found.

**Why this priority**: the failure paths are where a machine-readable
format costs the most when it is wrong, and the house rule for a bad
option is one line and status 2.

**Independent Test**: drive the command line and the loader with a
scripted suite: an empty path, each invalid raw value, a path under a
missing directory, an unwritable path, an empty file, a truncated file,
a file with an unknown major version, a file missing a required field,
and a file with a field of the wrong type.

**Acceptance Scenarios**:

1. **Given** `--json-raw=chunk`, **When** the program starts, **Then**
   it prints one line on standard error and exits with status 2, and no
   benchmark runs.
2. **Given** `--json=` with an empty path, **When** the program starts,
   **Then** it prints one line and exits with status 2 before any
   benchmark runs.
3. **Given** `--json=/no/such/dir/run.json`, **When** the run
   finishes, **Then** the console report prints, one line names the
   path, and the exit status is 2.
4. **Given** a file whose major schema version the loader does not
   know, **When** the loader reads it, **Then** it answers one error
   naming the version it read.
5. **Given** a run where one benchmark failed and `--json` names an
   unwritable path, **When** the run finishes, **Then** the console
   report prints, one line names the path, and the exit status is 2.

---

### User Story 5 - Compare two runs without a diff of noise (Priority: P2)

A developer keeps a baseline file and diffs a new run against it. Two
runs of the same measurements write the same bytes: the key order is
fixed, the benchmarks keep run order, and every number uses one format.
The schema version at the top tells a reader, and N5, which schema a
file carries.

**Why this priority**: the value is in the comparison N5 will build, and
it costs nothing to fix the order and the number format now, while the
writer is being written.

**Independent Test**: run the same scripted suite twice into two paths
and compare the files byte for byte; then read the schema version field
of the file and check it against the schema file's own version.

**Acceptance Scenarios**:

1. **Given** two runs of one scripted suite with the same options,
   **When** the two files are compared, **Then** they match byte for
   byte.
2. **Given** a written file, **When** its keys are read in document
   order, **Then** the order matches the order the schema declares.
3. **Given** a value the run cannot answer, **When** the file writes
   it, **Then** it stands as `null` with its availability, and reading
   the written numbers back gives the values the run held.

---

### User Story 6 - Read the page, copy the block, run the example (Priority: P3)

A reader opens the main page, reads the JSON reports section, copies the
block over the canonical includes, builds it, and gets the file the
section's result sentence names. The quick-reference table carries the
two new options and the load call, and the harness guide carries the
field list.

**Why this priority**: Principle IV.1 makes the main page an obligation
of the feature, and the drift and run gates enforce it.

**Independent Test**: `python3 tools/docs/example_drift.py .` reports no
finding, and the `example_run` gate runs the program that carries the
region and produces the file the result sentence names.

**Acceptance Scenarios**:

1. **Given** the merged page, **When** the drift gate runs, **Then**
   the JSON reports section, its region, its owner example file, and
   its quick-reference rows agree.
2. **Given** the docs example, **When** `example_run` runs it, **Then**
   the file it writes carries the fields its result sentence names.

---

### Edge Cases

- No benchmark matches the filter: the run produces no result, and no
  file appears (FR-006).
- `--list` or `--catalog` with `--json`: no benchmark runs, so no file
  appears (FR-006).
- `--dry-run` with `--json`: the run produces its planned rows and no
  measurement, and the file carries what the run produced (FR-003).
- A benchmark whose outcome is `SKIPPED`, `DISABLED`, `FAILED` or
  `INTERRUPTED`: the entry carries the outcome and the reason text
  (FR-012).
- A percentile value the run could not answer: it stands as `null`
  beside its availability (FR-015).
- A manual-time run with no primary fold: the primary time percentile
  cells answer `null`, and the columns the run did answer carry values
  (FR-015).
- A threaded result: the file carries one context record per context
  and the per-context percentile columns of `contextPercentiles`
  (FR-015, FR-016).
- A placement text longer than the console field width: the file
  carries the whole text, since the console cut is a formatting choice
  of the printer (FR-016).
- A complexity fit result: the fit row of the synthetic result carries
  its quantity, curve, coefficient and RMS, or the reason a fitted
  quantity stands without a pair (FR-013).
- A result with `hideRepetitionRows`: the file carries the flag and
  every row the result holds, since the flag is a console printing
  choice (FR-014).
- An empty label, an empty reason, an empty placement: each stands as an
  empty string, and the loader reads it back as empty (FR-012).
- A run interrupted by SIGINT: the file is written before the exit
  status is answered, it carries the results that completed, and the
  interrupted benchmark carries its outcome (FR-002, FR-004).
- A path in a directory that does not exist, an unwritable path, or a
  write that fails part way: one line names the path, the console
  report still prints, and the exit status is 2 even when a benchmark
  of the run failed (FR-030, FR-050).
- `--json-raw` with a value outside `none` and `chunks`, an empty
  value, or an empty path: one line on standard error and exit status 2
  before any benchmark runs (FR-029, FR-030).
- A file that is empty, truncated, or not JSON at all: the loader
  answers one error naming the position it failed at (FR-024).
- A file with a field of the wrong type, or a required field missing:
  one error naming the field (FR-024).
- A file whose major schema version the loader does not know: one error
  naming the version, and no partial result (FR-023).
- A file with keys in another order: the loader reads it, since the
  fixed order is a writing rule and a diff property, and a requirement
  the loader does not impose (FR-009, FR-022).
- A file with a chunk column of a size that disagrees with
  `chunkCount`: one error naming the field pair (FR-024).
- A run with the raw option on and a large chunk count: the file grows
  with the chunk count, and the loader reads it within the parser's own
  document bound (FR-022, Assumptions).

## Requirements *(mandatory)*

The groups C-1 to C-7 below carry the five capabilities the prompt
names: the writer (prompt C-1), the schema (prompt C-2), the loader
(prompt C-3), the options (prompt C-4) and the documentation
(prompt C-5). The file content and the cost discipline each stand as a
group of their own.

### Writer (C-1)

- **FR-001**: When the user names an output path, the harness shall
  write one JSON document for the whole run, after every selected
  benchmark has finished and every context of every threaded run has
  joined (D-2, D-10).
- **FR-002**: The document shall hold one entry per benchmark the run
  produced, in run order, including the complexity fit entries the
  console prints between families.
- **FR-003**: The document shall carry the same values the console
  report prints for that run. The writer shall add no measurement, and
  the console report shall keep the bytes it prints today at every
  setting of the new options.
- **FR-004**: Writing the file shall not change the exit status the run
  earns, except for the write-failure row FR-030 adds.
- **FR-005**: The writer shall read no clock and sample nothing, and
  the document shall carry no run date, matching the context header the
  console prints (PC-4).
- **FR-006**: A run that produces no benchmark result shall write no
  file: the filter matched nothing, list mode ended early, or the
  catalog listing ended early.

### Schema and versioning (C-2)

- **FR-007**: The document shall name its schema with a major and a
  minor part, and the first schema shall read `1.0` (D-4).
- **FR-008**: The repository shall carry that schema at
  `docs/schema/report-v1.json` in JSON Schema draft 2020-12. A test
  shall read that file and check every written document's key set, key
  order and value kinds against it, and no schema validator library
  shall enter the tree (D-4).
- **FR-009**: Key order shall follow the schema and the field order of
  the C++ values, and no key order shall depend on a hash container or
  on the order a run happened to visit. The loader shall accept a file
  whose keys stand in another order (D-9).
- **FR-010**: A minor schema change shall add fields and keep the
  meaning of every field it carries; a major change may break the loader
  on purpose. A field another lane adds to a result type before this
  feature rebases joins the schema, raises the minor version, and joins
  the round-trip test (PC-2, FR-023).
- **FR-011**: Every number of the document shall round-trip: reading the
  written token back shall give the value the run held, and two runs of
  the same values shall write the same bytes (D-9, PC-5).

### Content (C-3)

- **FR-012**: Each benchmark entry shall carry its name, suite, case,
  outcome, and the reason text when the outcome is not measured.
- **FR-013**: Each row entry shall carry its kind, iteration count, the
  two time columns, the endpoint sample count, the pause-folded flag,
  the primary availability, the metric columns, the counter columns,
  the counter disclosure token, the label, the overhead floor, the
  complexity fit fields, the chunk count, the chunk iteration count,
  and the aggregate statistics of the row (PC-2).
- **FR-014**: Each benchmark entry shall carry the metric labels, the
  counter labels, the counter statistics, the statistic values, the
  time unit, the hide-repetition-rows flag, the pause-folded flag, the
  primary-is-cpu flag, and the percentile points of the run (PC-2).
- **FR-015**: The percentile values of each row and of each context
  shall stand in the document, one per reported column and per point,
  each with its availability. A value the run could not answer shall
  stand as `null` (D-5).
- **FR-016**: A threaded entry shall carry one context record per
  context: the wait mode, the placement text, and the applied flag. The
  placement text shall stand whole, without the field-width cut the
  console printer applies (D-7, PC-4).
- **FR-017**: The noise warning of a result shall stand in the document
  as the condition the console check applies, with the two counts that
  check prints (D-7, PC-4).
- **FR-018**: With the raw content option at `chunks`, each row and
  each context shall carry the chunk value columns in the column order
  the percentile columns use, beside the chunk count and the chunk
  iteration count. Those columns shall reach the writer beside the
  result value, and the public layout of the result types shall keep
  its fields (D-6, D-12, PC-3).
- **FR-019**: With the raw content option at `none`, the document shall
  carry no chunk value column.
- **FR-020**: The raw content option shall turn the chunk capture on for
  a run whose percentile output stands off, and the console report shall
  stay free of percentile lines for that run (D-12).
- **FR-021**: The document shall carry no value the console report does
  not compute for that run. A value the run cannot answer shall appear
  as `null` with its availability.

### Loader (C-4)

- **FR-022**: A public call of the harness library shall read a report
  file and answer the same result values the run produces, including
  the raw columns when the file carries them, so that a loaded value
  writes back byte for byte (the loader capability the prompt names).
- **FR-023**: The loader shall refuse a document whose major schema
  version it does not know, with one error naming the version it read,
  and it shall answer no partial result (D-11).
- **FR-024**: The loader shall answer one error for a document that is
  not valid JSON, for a required field that stands missing, for a field
  of the wrong type, and for a chunk column whose size disagrees with
  the chunk count. The error shall name the field, or the position for
  a document that is not JSON at all (D-11).
- **FR-025**: A value the document holds as `null` shall load as a value
  whose availability is false.
- **FR-026**: Writing a loaded result with the settings that produced
  its file shall give the same bytes as that file (D-3).
- **FR-027**: A command surface that loads a report file shall exit with
  status 2 after one line when the loader answers an error. This feature
  adds no such surface, so the rule binds the loader's error channel and
  the driver that exercises it (D-11).

### Options (C-5)

- **FR-028**: A command-line option shall name the path of the report
  file for the run (D-8).
- **FR-029**: A command-line option shall set the raw content of the
  file to `none` or `chunks`, and the default shall be `none` (D-6).
- **FR-030**: An empty path, or a raw value outside the two words, shall
  print one line on standard error and exit with status 2 before any
  benchmark runs, on the usage-error path the existing options already
  take (PC-4). A path the file system refuses shall print the console
  report first, then one line naming the path, and the exit status
  shall be 2, and that 2 stands even when a benchmark of the run
  failed with status 1 (D-8).
- **FR-031**: With no output option, no file shall be written, and the
  run shall behave as it does at the base commit.

### Cost and window discipline (C-6)

- **FR-032**: No allocation and no I/O shall stand inside a timed
  window at any setting of the new options (D-10).
- **FR-033**: The writer shall run after the last run of the batch and
  after every context of every threaded run has joined (D-10).
- **FR-034**: The time-source gate shall stay clean over every file this
  feature adds or edits, and the writer shall read no clock.
- **FR-035**: The loop-shape gate shall stay clean: the per-iteration
  codegen of a timed loop shall not change, and any new boundary work
  shall sit on the cold path the loop cursor already has.

### Main-page documentation (C-7)

- **FR-036**: The main page shall gain a "JSON reports" section of at
  most 25 lines: one purpose sentence, one marked snippet, one result
  sentence (Principle IV.1).
- **FR-037**: The section title shall join the feature-section list of
  the drift gate, and its snippet shall belong to one owner example
  file (PC-4, budget note).
- **FR-038**: The snippet shall come from a runnable example under
  `example/docs/` that CI builds through a `run_docs_` target and runs
  through an `example_run` entry.
- **FR-039**: The quick-reference table shall gain a row for each new
  option and for the load call, and the option rows shall match the
  option table the drift gate reads.
- **FR-040**: `docs/pages/harness.md` shall carry the detail: the field
  list of the document, the version rule, the `null` rule, the raw
  column rule, and the loader's error rule.
- **FR-041**: `PAGE_LINE_CAP` shall rise by the measured size of the new
  section and its rows, through the recorded decision of the budget note
  (PC-4).

### Surface, version, and discipline

- **FR-042**: The feature shall add its writer and reader in new
  translation units under `source/harness/`, and shall edit no console
  printer.
- **FR-043**: The feature takes the next free minor version: 0.14.0,
  because PR #41 landed 0.12.0 and PR #42 landed 0.13.0 (D-13).
- **FR-044**: The harness `SOVERSION` moves from 7 to 8, because the
  public surface of the harness archive grows: the load call and the
  report options. The constitution's Refactoring and Evolution rule
  keeps `SOVERSION` consistent with a public API change, and the same
  rule moved that `SOVERSION` from 6 to 7 for N1. The public layout of
  `ResultRow` and `BenchmarkResult` keeps its fields, so no exported
  struct layout moves (D-12, D-13). The `speedgun-ng` archive
  `SOVERSION` stays 2, because the counters library gains nothing.
- **FR-045**: New code shall ship with tests at 100% line and 100%
  branch coverage, with no `LCOV_EXCL` marker, waiver, residual row, or
  open row. Where a gap stays, the change shall stop and report the
  cause with `gcov -b` evidence.
- **FR-046**: Tests shall use the fake provider and barriers. No test
  shall sleep, and no test shall read a wall clock.
- **FR-047**: No new library shall enter the tree. The vendored simdjson
  shall write the document and read it back, and the pinned version
  shall not move for this feature (D-1, PC-5).
- **FR-048**: The install tree shall stay free of the vendored library:
  no simdjson header, archive, package file, target reference, or
  exported symbol, and the harness shall reach the vendored headers
  through a build-interface-only dependency (PC-5, specs/004 FR-014,
  FR-015, FR-016).
- **FR-049**: This feature shall edit no source file owned by N2 or N4.
  A shared-list edit shall keep the lines of both features after a
  rebase, and an edit outside that boundary shall stop and record its
  reason in the plan first.
- **FR-050**: The exit-status postcondition of the entry point and the
  status table of `specs/015-benchmark-harness-core/contracts/cli.md`
  shall gain the write-failure row FR-030 states, including the
  precedence of status 2 over the failed-benchmark status (PC-4).

## Key Entities *(include if feature involves data)*

- **Report document**: one JSON document for one entry-point
  invocation. It names its schema version, then the benchmarks of the
  run in run order.
- **Schema version**: the major and minor pair at the top of the
  document, and the schema file of the repository that describes the
  document. A minor adds fields; a major may break the loader.
- **Benchmark entry**: one benchmark of the run: its identity, its
  outcome, its rows, its labels, its statistics, its percentile points
  and values, its context records, and its noise warning.
- **Row entry**: one repetition or aggregate row, with the columns the
  console prints and the aggregate statistics of the row.
- **Percentile cell**: one point of one column of one row or context,
  with its availability. An unanswered cell stands as `null`.
- **Chunk column**: the ordered chunk values of one column of one
  context, present only when the raw content option asks for it
  (D-12).
- **Raw column record**: the chunk value columns of one run, answered
  by the runner beside the result value and read by the writer; the
  public result types hold no such field (D-12).
- **Context record**: the wait mode, the placement text and the applied
  flag of one context of a threaded run.
- **Noise warning**: the condition the console check applies, with the
  spinning count and the runnable cpu count it prints.
- **Loader error**: the one answer the loader gives for a document it
  cannot read, naming the field or the position.
- **Report options**: the output path and the raw content setting, both
  command-line, with the defaults of no file and `none`.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A run with `--json=<path>` leaves one file at that path,
  that file validates against `docs/schema/report-v1.json`, and every
  benchmark the console printed appears in it once, in run order, with
  the values the console printed.
- **SC-002**: A test that writes a report, loads it and writes it again
  finds the two files byte for byte equal.
- **SC-003**: With no output option, no file appears, and the console
  report of a scripted suite matches the report of the same suite at
  `c7f1769`.
- **SC-004**: Each invalid option form (an empty path, a raw value
  outside the two words) exits with status 2 after one line on standard
  error with no benchmark run, and an unwritable path exits with status
  2 after the console report prints.
- **SC-005**: A file with an unknown major version, a missing required
  field, a field of the wrong type, or a chunk column whose size
  disagrees with the chunk count gives one error naming the version or
  the field, and the loader answers no partial result.
- **SC-006**: A test that counts allocations across a timed window
  reports zero allocations with the raw content option at `chunks`.
- **SC-007**: The loop-shape gate reports the same per-iteration
  instruction cycle for a timed loop as it reports at `c7f1769`, under
  both compilers it drives.
- **SC-008**: The coverage gates report 100% line and 100% branch over
  the new code with no exclusion marker.
- **SC-009**: Two runs of one scripted suite with the same options write
  byte-identical files.
- **SC-010**: For a two-context scripted run, the file carries one
  context record per context and the per-context percentile columns, and
  each value matches the console disclosure and percentile lines of the
  same run.
- **SC-011**: The main page's JSON reports section prints the fields its
  result sentence names, through `example_run`, and
  `python3 tools/docs/example_drift.py .` reports no finding.
- **SC-012**: The full gate set passes on the branch: the `ci-ubuntu`
  and `ci-rocky` builds, `ctest`, `ci-sanitize`, `ci-tsan`, clang-tidy
  and cppcheck with no new finding, `format-check`, `spell-check`,
  `prose-lint`, the time-source gate, the page-drift gates, the DBC
  gates, and the coverage gates.

## Constraints

- **Counters-only.** Every time and counter value comes from
  `sg::counters`, and `test/time_source_gate.sh` stays clean (Additional
  Constraints of the constitution). The writer reads no clock (FR-005,
  FR-034).
- **No I/O and no allocation in the timed window.** The writer runs
  after the batch, and the raw columns reach it through the result value
  (FR-032, D-12).
- **Console bytes stay.** The console report prints the same bytes at
  every setting of the new options (FR-003).
- **Vendored privacy holds.** The install tree, the package files and
  the dynamic symbol table stay free of simdjson (FR-048).
- **Contracts at the boundary.** A bad path or a bad raw value on the
  command line is a usage error that prints one line and exits with
  status 2; a bad argument on the load call is a contract violation.
- **Coverage.** 100% line and 100% branch, no `LCOV_EXCL` under
  `source/harness/` (FR-045).
- **Test discipline.** The fake provider and barriers; no sleep, no
  clock (FR-046). `ctest -j 2` stays the runner. The coverage preset
  keeps `-fprofile-update=atomic` and stays free of
  `-D_GLIBCXX_ASSERTIONS`.
- **Docs budget.** `SECTION_LINE_CAP` stays 25; the page cap moves by
  the measured amount through a recorded decision (FR-041).
- **Prose and commits.** Principle XI and the commit lint bind every
  word and every commit; commit messages carry blank lines between body
  paragraphs and one footer block with `Approved-by:` then `Refs:`.
- **Rebase discipline.** Rebase, push with `--force-with-lease`, and
  show `git ls-remote origin <branch>` after each push.

## Scope

In: FR-001 to FR-050. The writer, the versioned schema and its schema
file, the loader, the two command-line options, the raw chunk column
retention D-12 records, the main-page section with its runnable example,
the quick-reference rows, the `docs/pages/harness.md` detail, the tests,
and the version effect.

Out, each owned by a later roadmap item or another lane:

- comparison against a baseline, and any pass or fail verdict over two
  files (N5);
- a compare mode on the command line, and environment-variable defaults
  (N7);
- histograms, plots, and any use of the vendored HdrHistogram (N8);
- metric formulas and the counters catalog (N2, corsairone, PR #41);
- topology placement and presets (N4, thrawn);
- NUMA memory-node placement (N12);
- a command-line mode that loads a file: this feature ships the load
  call, and the surface that uses it belongs to N5 and N7;
- compression, streaming, or a file per benchmark: one file per run
  stands at D-2.

## Assumptions

- The roadmap file the prompt names sits outside this repository and is
  absent from this machine, so no claim here rests on it. The prompt
  text is the authority for scope.
- The output path is a run-wide command-line setting. No per-family
  handle call names a path, because one document holds one run (D-2) and
  a per-family path has no meaning inside it.
- The load call answers the same result types the run produces, settled
  in the 2026-10-20 clarification session, so a loaded value writes
  back byte for byte (FR-022, FR-026).
- Numbers use the vendored writer's fixed double format, which
  round-trips every value and stays within 17 digits (PC-5). The
  shortest digit count of the wider interval is not a property this
  feature claims, and FR-011 states round-trip equality and byte
  stability, the properties a diff and a comparison need.
- A file with the raw columns grows with the chunk count of the run. The
  loader reads it through the parser's own document bound, and no
  streaming reader is in scope.
- The noise warning of a result is a property of that result, the way
  the console check computes it per result (PC-4).
- Threading tests run on the fake provider, so no test needs hardware,
  a PMU, or privileges.
- The schema file is a repository artifact a test checks. specs/004
  FR-014 keeps the install tree free of vendored content, and the
  install rules of this feature add the schema file only if the plan
  records that choice.

## Budget note

`README.md` stands at 659 lines of a 659 cap, and `PAGE_LINE_CAP` in
`tools/docs/example_drift.py` is 659 (PC-4), so the JSON reports section
and its quick-reference rows cannot fit as they stand. The cap moves by
the measured amount when the section lands, the smallest fitting amount,
recorded here and in the gate's own comment, the way `specs/021`
recorded its move from 638 to 659. `SECTION_LINE_CAP` stays 25, prose is
trimmed first, no snippet is dropped, and no earlier spec is edited. The
new section title joins the `SECTIONS` list of the gate, and its snippet
joins `SECTION_OWNERS` with one owner example file. If a rebase brings
another feature's section in first, this feature's cap move is measured
against the page as the rebase leaves it.

## Dependencies

- N1's percentile surface: `PercentileValue`, the percentile columns of
  `ResultRow` and `BenchmarkResult`, the chunk pair, and the percentile
  helpers of `source/harness/detail/percentiles.hpp` (PC-3).
- The result types: `BenchmarkResult`, `ResultRow`, `ContextDisclosure`
  and `Statistics` in `include/speedgun-ng/benchmark.hpp` (PC-2).
- The entry point and the option scanner: `speedgunMain` and the long
  option table of `source/harness/cli.cpp`, `RunOptions` of
  `source/harness/detail/internal.hpp`, and the status table of
  `specs/015-benchmark-harness-core/contracts/cli.md` (PC-4).
- The console printers this feature mirrors: `printResult`,
  `printRowPercentiles`, `printContextDisclosure` and `printNoiseWarning`
  of `source/harness/report.cpp` (PC-4).
- The vendored simdjson: `external/simdjson` at 4.6.11,
  `import_simdjson()` and `simdjson_archive_merge()` in
  `CMakeLists.txt`, and the version tripwire of
  `source/simdjson/simdjson_gate.cpp` (PC-5).
- The privacy contract of the vendored library:
  `specs/004-vendor-simdjson/spec.md` FR-013 to FR-017.
- D1's main-page structure and drift gate: `README.md`,
  `tools/docs/example_drift.py` (`SECTIONS`, `SECTION_OWNERS`,
  `PAGE_LINE_CAP`, `SECTION_LINE_CAP`, `SECTION_LINE_CAPS`), and the
  example run machinery (`add_docs_example`, the `run_docs_` targets and
  `run-examples` in `example/CMakeLists.txt`).
- The counters library for the test fixtures: `FakeProvider` and the
  allocation-counting precedent of `test/source/counters_noalloc_test.cpp`.
- The constitution gates of Principle VIII: the counters-only scan, the
  loop-shape gate, the naming rules, Principle II, IV.1, the coverage
  gates, `ci-sanitize`, `ci-tsan`, clang-tidy and cppcheck, the DBC
  gates, and the prose and commit lint.

## Citations

Symbol and section anchors, each read at the audit point `c7f1769`.

| Artifact | Location |
| --- | --- |
| `PercentileValue` | `include/speedgun-ng/benchmark.hpp:337` |
| `ResultRow` and its fields | `include/speedgun-ng/benchmark.hpp:351`, `:364`, `:368`, `:372`, `:374`, `:381`, `:386`, `:390`, `:391`, `:398`, `:401`, `:402`, `:403`, `:404` |
| `ContextDisclosure` | `include/speedgun-ng/benchmark.hpp:421` |
| `BenchmarkResult` and its fields | `include/speedgun-ng/benchmark.hpp:440`, `:452`, `:463`, `:468`, `:473`, `:480`, `:483`, `:488`, `:492`, `:498`, `:502` |
| `buildChunkValues`, `minSamplesFor`, `percentileAt`, `normalizePercentilePoints`, `buildPercentileColumns` | `source/harness/detail/percentiles.hpp:30`, `:44`, `:54`, `:64`, `:78` |
| Chunk value columns as run-time locals | `source/harness/runner.cpp:271`, `:317`, `:319`, `:494`, `:505`, `:1464`, `:1512` |
| Merged and per-context percentile columns | `source/harness/runner.cpp:527`, `:531` |
| `printContextDisclosure`, `printNoiseWarning`, `runnableCpuCount` | `source/harness/report.cpp:173`, `:227`, `:203` |
| Context header, percentile line, row, result | `source/harness/report.cpp:287`, `:299`, `:351`, `:373` |
| `speedgunMain`, option table, usage errors, exit-status postcondition | `source/harness/cli.cpp:218`, `:249`, `:67`, `:119`, `:139`, `:161`, `:332`, `:415` |
| `RunOptions` | `source/harness/detail/internal.hpp:324` |
| Exit status table and option table of the contract | `specs/015-benchmark-harness-core/contracts/cli.md:9`, `:73` |
| Vendored simdjson version, import, merge, tripwire | `external/simdjson/include/simdjson/simdjson_version.h:7`, `CMakeLists.txt:109`, `CMakeLists.txt:170`, `source/simdjson/simdjson_gate.cpp:12` |
| `string_builder` and its number path | `external/simdjson/include/simdjson/generic/builder/json_string_builder.h:51`, `external/simdjson/include/simdjson/generic/builder/json_string_builder-inl.h:749` |
| The writer's double algorithm | `external/simdjson/src/to_chars.cpp:701`, `:761` |
| Vendored include and privacy rules | `specs/004-vendor-simdjson/spec.md:109` (FR-013), FR-014 to FR-017 |
| Harness target link and source glob | `CMakeLists.txt:191`, `:197` |
| Version and `SOVERSION` | `CMakeLists.txt:7`, `:47`, `:225` |
| Drift gate caps and section list | `tools/docs/example_drift.py:30`, `:112`, `:113`, `:118` |
| Main page and quick-reference table | `README.md` (659 lines), `README.md:551` |
| Harness guide | `docs/pages/harness.md` (901 lines) |
| Allocation-counting precedent | `test/source/counters_noalloc_test.cpp` |
| Gate scripts | `test/time_source_gate.sh`, `test/loop_shape.sh`, `test/cli_shape.sh`, `test/example_run` entries |
| Gate registration and CI jobs | `test/CMakeLists.txt`; the `lint`, `coverage`, `sanitize`, `tsan`, `dbc-gate` and `prose-lint` jobs of `.github/workflows/ci.yml` |
| N1 artifacts | `specs/021-percentiles-raw-capture/spec.md` and its `tasks.md` |
| Constitution IV.1, VIII gates, II, V.1, XI, counters-only, footer | `.specify/memory/constitution.md` |

## Open questions

The prompt carried its open items as recommendations, and Charles
accepted each on 2026-10-20, so the decisions stand at D-1 to D-11 and
D-13 and again at the FR that carries them.

Two items are not settled by the prompt, and the 2026-10-20
clarification session answered both, so neither is open any more:

- **D-12 chunk value retention**: the raw chunk columns exist only
  inside the runner at the base (PC-3), so `--json-raw=chunks` needs a
  home for them. The answer is that they travel beside the result in a
  record the runner answers the writer, and the public result layout
  keeps its fields (FR-018, FR-044). Charles can overturn it.
- **FR-050 exit status**: a report write that fails exits with status
  2, and the entry point's postcondition and the status table of
  `contracts/cli.md` carry no such row today (PC-4). The answer is that
  the row joins the contract in the same change, and status 2 takes
  precedence over a failed benchmark (FR-030). Charles can overturn it.

Three code-level names still wait for the plan, and none is an open
question of behavior: the spelling of the load call, the name of the
record that carries the retained chunk columns, and the name of the new
translation units.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes user-visible
  behavior, so FR-036 to FR-041 carry the JSON reports section, its
  `example/docs/` region with its `run_docs_` target and `example_run`
  entry, its quick-reference rows, and the `docs/pages/harness.md`
  detail in the same change, inside the budget the budget note records.
