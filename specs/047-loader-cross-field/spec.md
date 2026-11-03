# Feature Specification: Loader Cross-Field Checks for the Pass Fields

**Feature Branch**: `047-loader-cross-field`

**Created**: 2026-11-03

**Status**: Draft

**Input**: User description: ticket T-LCF of the harness roadmap, section 7,
the deferred item H-6 of PR #44. `loadReport` reads the pass fields of each
entry one field at a time and checks that the fields agree nowhere, so a
document the writer cannot make loads with no error and hands wrong figures to
the comparison. The ticket adds the cross-field checks to spec 024 FR-038 and
to the loader. Condensed below; the request is the authority for scope,
numbering and the accepted answers.

## Decisions

The request carries four open questions with recommended answers. Each one is
taken as the decision below, and each names the reading it settles. No
implementer reopens them.

- **D-1 Number and branch (Q on the number).** The feature is 047, on the
  branch `047-loader-cross-field`, cut from the `master` head at branch time.
  Measured at branch time: `specs/` holds no 045, 046 or 047;
  `git ls-remote --heads origin` names no such branch; `gh pr list` answers no
  open pull request. 045 belongs to T-MERGE-FIX and 046 to N14c, so 047 stands
  free.
- **D-2 Version (Q-3).** The loader refuses documents it accepted before. No
  feature arrives, no public type moves, and no key is added. The version takes
  the next free patch: `0.30.1`, measured free at branch time over
  `CMakeLists.txt`, `docs/`, `source/`, `include/` and `test/`. The harness
  `SOVERSION` stays 20 and the counters archive `SOVERSION` stays 5, the
  numbers `CMakeLists.txt:50-58` records for the unreleased window. The schema
  stays 1.13.
- **D-3 The rule set.** Three pairs of fields carry checks, in the words of the
  request: the method beside the count, the spread beside the count, and the
  pass tag of a metric column beside the count. Section "The rules" states them.
- **D-4 The pair rule (FR-002).** Each check reads a pair. A pair whose one key
  stands absent answers no check, so a document written before specs/024 loads
  as it does today and no key becomes required.
- **D-5 The refusal text (Q-2).** A refusal keeps the standing shape of the
  loader: the field path, a colon, and the words that name the fault. No new
  error type and no new wording style join the reader.
- **D-6 Older schema versions (Q-1).** A check runs on every document that
  carries its pair, whatever schema version the document names. A schema 1.0
  document carries no pass key, so it loads as it stands.
- **D-7 Documentation (Q-4).** The loader section of `docs/pages/harness.md`
  gains two to four lines that state the checks. The main page teaches no
  loader rule, so `README.md` and its `example/` regions stand unchanged
  (IV.1 check: no main-page impact).
- **D-8 The law lands first (FR-001).** The amendment of spec 024 FR-038 stands
  in its own commit, ahead of the loader commit, and carries
  `Refs: specs/024-multipass-collection T-LCF`.
- **D-9 One refusal per document.** The reader stops at the first fault it
  reaches, the standing rule of the loader (`firstFailingPartStopsTheFold`
  measures it at `test/source/harness_json_load_test.cpp:706-739`). The order
  that fixes is D-16, so a document that carries a tag fault and a method fault
  answers the tag refusal.
- **D-10 The writer, the schema file, the schema checker and the comparison
  stand unchanged.** A written document carries every key of every pair
  (measured below), so no rule of this feature can refuse one. The comparison
  already answers a refused document with status 2 and one quoted line
  (`source/harness/compare.cpp:1876-1890`).
- **D-11 The accepted forms.** The writer's own document for each method, a
  stopped-loop entry, and a schema 1.0 document. Each keeps its test.
- **D-12 Coverage.** No new exclusion construct joins the tree: no
  `LCOV_EXCL`, no `--omit-lines`, no `--ignore-errors`, no `--remove`. The
  branch shortfall stays within the recorded allowance of 21 rows
  (`tools/dbc/coverage_gate.sh:102`), and the record names the number read.
  The allowance is a shortfall budget: an arm a test reaches adds to the total
  and to the hit, so it costs nothing, and an arm no test reaches costs one row
  (`:111-118`). The CI union reads 0 rows short today
  (`specs/043-fwq-ftq/plan.md:122`), so all 21 rows stand free.
- **D-13 The overlap with specs/046.** Both lanes touch `json_load.cpp` and
  `harness_json_load_test.cpp`, in different readers of that file. The second
  to merge rebases, keeps both sides after reading them, and runs every gate
  again. No file of specs/045 stands in this lane's scope.
- **D-14 Rule 1 runs on a measured, hunt-free entry (clarify round 1).** The
  ticket words rule 1 over every entry. Measured at the base, the writer makes
  two forms that rule 1 as worded refuses. The runner settles the method before
  the warm-up and publishes a count of 0 beside `MULTIPLEX` or `PASSES` on four
  stop paths (`source/harness/runner.cpp:2884`, `:2889`, `:2923`, `:2928`), and
  the calibration of pass 0 inside the pass loop hands the same answer while no
  fold stands (`:3162`, `:3168`, `:2954-2955`). An interrupt names the outcome
  `SKIPPED` (`:94-98`). The hunt path names the method the operator forced
  beside the count the collection published
  (`source/harness/cli.cpp:1788-1789`), in the branch that fills the entry's
  hunt record (`:1775`), the key the writer writes
  (`source/harness/json_report.cpp:859-862`). A committed test writes that
  document and reads its bytes
  (`test/source/harness_hunt_cli_test.cpp:1277-1302`). Rule 1 therefore runs for an entry
  whose outcome is `MEASURED` and that carries no `hunt` key. Rules 2 and 3 need
  no waiver: a stopped collection publishes no spread and no tag
  (`source/harness/runner.cpp:1720-1722`), and the merge, the only writer of a
  tag, runs above one pass (`:3303`, `source/harness/pass_plan.cpp:322`).
- **D-15 The refusal text (clarify round 1).** One refusal is one line, with one
  leading path naming the pair's non-count field, and the words carry the
  partner's full path. The standing shape is
  `"<path>: <words>"` (`source/harness/json_load.cpp:57-58`, `:168`,
  `:207-208`, `:2759-2761`). The shape makes every US-1 scenario a binary check.
- **D-16 The order (clarify round 1).** The tag check runs in `readEntry` after
  the pass block, over the rows the entry has read. The loaded row index and
  metric index equal the document indices
  (`source/harness/json_load.cpp:2640-2656`, `:1589-1598`), so the path rebuilds
  byte for byte. The pass block keeps its place, and the standing refusals keep
  theirs.
- **D-17 Presence (clarify round 1).** Rule 1 counts a key present when it
  stands. Rules 2 and 3 count a key present when it stands and holds a non-null
  value (`source/harness/json_load.cpp:2744-2745`, `:388-389`). A value of the
  wrong kind is refused by the standing kind check ahead of every pair check
  (`:135-144`, `:158-171`, `:196-211`).
- **D-18 The source of each accepted form (clarify round 1).** The scripted run
  of the load test registers one metric column on an object that publishes no
  counter limit, so it settles `MULTIPLEX`
  (`test/source/detail/json_fixture.hpp:96-122`,
  `source/harness/pass_plan.cpp:131-135`). The split form and the stopped form
  of a split run stand hand-built in the load test, and the split form of a
  written document stands in the pass report test
  (`test/source/harness_pass_report_test.cpp:833-835`).
- **D-19 The shape of a check keeps its arms covered (clarify round 2).**
  Measured with the coverage flags of the `coverage-linux` preset and
  `gcov -b -c` on this host (`/tmp/notes-047/shape/`): a pair guard of one `&&`
  plus a min/max pair per method answers 8 branch records with none taken zero;
  rule 2 answers 6; rule 3, 12. A `switch` over the three method words answers a
  fourth jump-table arm that no document can reach, because the standing kind
  check refuses a word outside the three names
  (`source/harness/json_load.cpp:2728-2732`), so it costs one dark row. The
  checks therefore read: one `if` per refusal, a pair guard as one `&&` of the
  two key tests, and no `switch`. No check stands as `SG_ENSURE`: the extract
  step omits those lines (`cmake/coverage.cmake:101`), and the enforce response
  calls `std::abort()` (`include/speedgun-ng/dbc.hpp:213`), so no in-process
  test can reach the true arm; a load error is the right shape for a parser of
  external input (FR-007). The throw arms of the path string are filtered
  (`cmake/coverage.cmake:100`).
- **D-20 The lane host needs two pinned tools (clarify round 2).** The tree is
  formatted by clang-format 18.1.8, which CI installs by pip
  (`.github/workflows/ci.yml:33-34`); the host `/usr/bin/clang-format` is
  23.1.4 and names a long list of untouched files as badly formatted, so the
  lane installs the pinned tool (`uv tool install clang-format==18.1.8`) and
  runs the gate through `FORMAT_COMMAND`. The host `lcov` needs the Perl module
  `Capture::Tiny`, which stands at `~/perl5/lib/perl5` and needs `PERL5LIB` in
  the environment of the coverage commands. Both are host tool conditions: no
  gate, preset, or tool file of the repository changes.
- **D-21 The compare pairing (clarify round 2).** No committed fixture carries a
  pass-field disagreement of schema major 1, so the pairing of a pair refusal
  with status 2 stands untested until this lane plants one. The plant goes in
  `test/source/harness_compare_cli_test.cpp`, which already carries the shape
  helper `oneFaultLine` (`:782-800`), and needs no new fixture file: that file
  writes documents at run time.

## The rules

The rules are the whole added law. Each reads a pair of fields of one entry,
and each runs only when both keys of its pair stand present.

1. The method beside the count. An entry whose method is `AUTO` names a count
   of 0. An entry whose method is `MULTIPLEX` names a count of 1. An entry
   whose method is `PASSES` names a count of 1 or more. The rule runs for an
   entry whose outcome is `MEASURED` and that carries no `hunt` key (D-14).
2. The spread beside the count. An entry whose `passSpread` stands non-null
   names a count of 2 or more.
3. The pass tag beside the count. A metric column whose `pass` stands non-null
   names an entry count of 2 or more, and the tag, a 0-based pass index, stands
   below that count.

Presence, per rule (D-17). Rule 1 counts a key present when it stands in the
entry, and a value of the wrong kind keeps the standing kind refusal ahead of
every pair check. Rules 2 and 3 count a key present when it stands and holds a
non-null value, the reading the loader already takes at those two keys.

One refusal is one line in the standing shape `<path>: <words>` (D-15). The
path names the pair's non-count field: `/benchmarks/<e>/metricMethod`,
`/benchmarks/<e>/passSpread`, `/benchmarks/<e>/rows/<r>/metrics/<m>/pass`. The
words name the partner by its full path.

The checks run in read order, and the first fault stops the read: the rows of
the entry, the standing `spreadWarning` check, rule 1, rule 2, and rule 3 over
the rows in order (D-16).

The `spreadWarning` check of specs/024 FR-038 stands as it is, ahead of the
three new checks. The pair rule of FR-002 covers the three new checks, and
leaves that standing check as it reads a document today.

## Preconditions

Measured at the base `b1dd7670` (version 0.30.0, schema 1.13, harness
`SOVERSION` 20, archive `SOVERSION` 5). A citation that names a line number
here re-anchors at plan time by symbol.

- The reader reads the four entry keys at `source/harness/json_load.cpp`
  2726-2764, one field at a time, and the metric key at 387-395. It compares
  one pair today: `spreadWarning` against `passSpread` at 2752-2763, the rule
  specs/024 FR-038 states.
- The writer writes the four entry keys at `source/harness/json_report.cpp`
  849-855 and the metric key at 323, so every written document carries every
  key of every pair.
- The runner publishes the count: 0 for `AUTO`, and the standing pass count for
  every other method (`source/harness/runner.cpp:1724-1729`). A split names two
  passes or more, and the kernel-multiplexed plan names one
  (`source/harness/runner.cpp:1703-1708`).
- The schema lists the four entry keys as required
  (`docs/schema/report-v1.json:517-520`) and the metric key as required
  (654-662), so a written document answers the checker with all of them.
- The method words are `AUTO`, `PASSES` and `MULTIPLEX`
  (`source/harness/detail/json_report.hpp:126-131`), and the warning limit
  stands at 5.0 percent (`:192`).
- The reader's refusal shapes today: `"<path>: expected a number, found
  string"`, `"<path>: a count stands negative"`, `"<path>: '<word>' stands
  outside the names the schema lists"`, and
  `"<path>: the document's warning disagrees with its pass spread"`.
- The rich fixture of the load test carries a schema 1.0 document whose entry
  names the stopped outcome, a reason, `PASSES`, a count of 2, a spread of 3.5
  and a metric tag of 1
  (`test/source/harness_json_load_test.cpp:181-218`). It answers the two
  accepted forms SC-001 names, and it stands valid under the new rules.
- The runner publishes a count of 0 beside a settled non-`AUTO` method on four
  stop paths (`source/harness/runner.cpp:2884`, `:2889`, `:2923`, `:2928`), and
  the in-loop stops publish the passes that stand, none before the first fold
  (`:3162`, `:3168`, `:3187`, `:2954-2955`). An interrupt names the outcome
  `SKIPPED` (`:94-98`).
- The hunt path overrides the method of an entry that carries a hunt record
  (`source/harness/cli.cpp:1775`, `:1788-1789`), the record the writer writes as
  the entry's `hunt` key (`source/harness/json_report.cpp:859-862`).
- The merge is the only writer of a metric tag, and it runs above one pass
  (`source/harness/runner.cpp:3303`, `source/harness/pass_plan.cpp:322`).
  `passSpread` answers nothing below two pass times (`pass_plan.cpp:404-408`),
  so rules 2 and 3 refuse no document the writer makes.
- The scripted run of the load test registers one metric column on an object
  that publishes no counter limit, so it settles `MULTIPLEX` with a count of 1
  (`test/source/detail/json_fixture.hpp:96-122`,
  `source/harness/pass_plan.cpp:131-135`).
- The committed report documents answer the rules: `test/data/gate_schema_11.json`
  names `AUTO` with 0, `test/data/hunt_no_hunt_document.json` names `AUTO` with
  0, and `test/data/report_views/multipass.json` names `PASSES` with 3 beside
  tags 0, 1, 2 and one null. `test/data/gate_schema_20.json` names `AUTO` with
  1 and schema major 2, and the version refusal stands ahead of the entry read
  (`source/harness/json_load.cpp:3129-3135`,
  `test/source/harness_compare_cli_test.cpp:877-883`).
- The version is pinned in three places: `CMakeLists.txt:59`,
  `test/source/views_test.cpp:279` and
  `test/source/harness_cli_table_test.cpp:1453`.
- The comparison answers a refused document with status 2 and one quoted line
  (`source/harness/compare.cpp:1876-1890`), the acceptance of US-1.

## Roadmap position

T-LCF of the harness roadmap, section 7: the loader cross-field checks for the
pass fields. The ticket is the deferred item H-6 of PR #44. The checks bind the
pass disclosure specs/024 FR-038 publishes, so the law lands as an amendment of
that requirement and the reader enforces it.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Refuse a document the writer cannot make (Priority: P1)

A tool author hands `loadReport` a report that a hand edit broke: the method
names `auto` while the count names four passes, or a metric column names pass
seven of a two-pass run. Today the reader accepts the document, and the
comparison reads wrong figures out of it. The reader refuses the document,
names the entry and the field, and the comparison exits 2 with one quoted line.

**Why this priority**: A reader that accepts an impossible document hands wrong
figures downstream. This story is the whole value of the ticket.

**Independent Test**: Hand the reader one document per rule of "The rules",
each carrying one planted disagreement, and read the refusal. Each refusal
names the field path the plant stands at.

**Acceptance Scenarios**:

1. **Given** a document whose method is `AUTO` and whose count is 2, **When**
   the reader reads the entry, **Then** the reader refuses it and names
   `/benchmarks/0/metricMethod` and `/benchmarks/0/passCount`.
2. **Given** a document whose method is `MULTIPLEX` and whose count is 3,
   **When** the reader reads the entry, **Then** the reader refuses it and
   names the two fields.
3. **Given** a document whose method is `PASSES` and whose count is 0, **When**
   the reader reads the entry, **Then** the reader refuses it and names the two
   fields.
4. **Given** a document with a non-null spread and a count of 1, **When** the
   reader reads the entry, **Then** the reader refuses it and names
   `/benchmarks/0/passSpread` and `/benchmarks/0/passCount`.
5. **Given** a document with a metric tag of 2 and a count of 2, **When** the
   reader reads the entry, **Then** the reader refuses it and names
   `/benchmarks/0/rows/0/metrics/0/pass`.
6. **Given** a document with a metric tag of 1 and a count of 1, **When** the
   reader reads the entry, **Then** the reader refuses it and names the tag.
7. **Given** a refused document, **When** the comparison reads it as a
   baseline, **Then** the comparison exits 2 with one quoted line.

---

### User Story 2 - Keep every document the writer makes (Priority: P1)

A user loads a report the harness wrote, a report from a run whose loop
stopped, and a stored baseline of schema 1.0. Each loads as it did before, and
the comparison reaches its verdict as it did before.

**Why this priority**: A stricter reader that refuses a valid document breaks
the comparison of stored baselines, the use specs/026 FR-051 protects.

**Independent Test**: Round-trip each accepted form through the writer and the
reader, and load each hand-built form. Every load answers the values it wrote.

**Acceptance Scenarios**:

1. **Given** a written document of a one-pass run, **When** the reader reads
   it, **Then** it loads and answers `AUTO` with a count of 0.
2. **Given** a written document of a multiplexed run, **When** the reader reads
   it, **Then** it loads and answers `MULTIPLEX` with a count of 1.
3. **Given** a written document of a split run, **When** the reader reads it,
   **Then** it loads and answers `PASSES` with its count, its spread and its
   tags.
4. **Given** a document whose entry names the stopped outcome, **When** the
   reader reads it, **Then** it loads.
5. **Given** a schema 1.0 document that carries no pass key, **When** the
   reader reads it, **Then** it loads and leaves each member at the default of
   a run that collected no pass.

---

### Edge Cases

- A document that carries one key of a pair and lacks the other answers no
  check of that pair, and loads.
- A spread that stands null beside a count of 1 loads: rule 2 reads a spread
  that stands non-null.
- A metric tag that stands null beside a count of 1 loads: rule 3 reads a tag
  that stands non-null.
- A count that is not an integer, and a method word outside the three names,
  keep the refusals the reader gives today.
- A document that carries two faults answers the refusal of the first the
  reader reaches, and says nothing of the second (D-9).
- A tag of a metric column of a row of a later entry names that entry in its
  path, and the check reads that entry's own count.
- A document of schema version 1.0, 1.12 and 1.13 answers the same rules. The
  reader gates the major alone (`source/harness/json_load.cpp:3129-3135`), and
  1.0 carries no pass key (D-6).
- A stopped entry that names `MULTIPLEX` or `PASSES` with a count of 0 loads:
  rule 1 runs on a `MEASURED` entry (D-14).
- A `MEASURED` entry that names `MULTIPLEX` with a count of 3 and carries a
  `hunt` key loads: rule 1 runs on a hunt-free entry (D-14).
- A document that names a count with no method key loads, and answers the
  default method with that count. A document that names `PASSES` with no count
  key loads, and answers the default count of 0. FR-002 leaves both loading, and
  no key of a pair becomes required (FR-004).
- A document whose `spreadWarning` disagrees with its spread keeps the standing
  refusal, which stands ahead of the three new checks.
- A document that carries a tag fault and a method fault answers the tag
  refusal, the order of D-16.

## Requirements *(mandatory)*

### The law (C-1)

- **FR-001**: The spec of specs/024-multipass-collection shall gain the rules
  of "The rules" in its FR-038, in a commit ahead of the loader commit and
  carrying `Refs: specs/024-multipass-collection T-LCF`. (D-8.)
- **FR-002**: The reader shall run a check of a pair only when both keys of
  that pair stand present, on the presence reading of D-17: rule 1 counts a key
  that stands, rules 2 and 3 count a key that stands with a non-null value.
  (D-4, D-17.)
- **FR-003**: When the method key and the count key of an entry both stand
  present, and the entry names the `MEASURED` outcome and carries no `hunt` key,
  the reader shall refuse an entry whose method is `AUTO` and whose count is not
  0, an entry whose method is `MULTIPLEX` and whose count is not 1, and an entry
  whose method is `PASSES` and whose count is below 1. (D-14.)
- **FR-004**: A document that lacks one key of a pair shall load as it does
  today, and no key of a pair shall become required.
- **FR-005**: When the count key stands present and the spread key stands
  present with a non-null value, the reader shall refuse the entry whose count
  stands below 2.
- **FR-006**: When the count key stands present and the pass tag of a metric
  column stands present with a non-null value, the reader shall refuse the
  entry whose count stands below 2, and the entry whose 0-based tag is not
  below the count. The check runs over every row of that entry, and over no
  context block, where no metric value stands.
- **FR-007**: Every refusal of a pair check shall travel the standing load
  error path, answer one line whose leading path names the pair's non-count
  field and whose words name the partner by its full path, and carry no new
  error type. (D-5, D-15.)
- **FR-008**: The comparison shall answer a document refused by a pair check
  with status 2 and one quoted line, through the path it has today. One
  comparison test shall plant a pair fault into a document of schema major 1, so
  the line it quotes names the pair's own fault: the standing fixture
  `test/data/gate_schema_20.json` names schema major 2, and the version gate
  refuses it ahead of the entry read (`source/harness/json_load.cpp:3129-3135`,
  `test/source/harness_compare_cli_test.cpp:875-884`). The shape stands measured
  at `test/source/harness_compare_cli_test.cpp:782-800`. (D-10, D-21.)
- **FR-009**: The `spreadWarning` check of specs/024 FR-038 shall keep its
  present rule, its text and its place ahead of the three new checks, and the
  pair rule of FR-002 shall not gate it: it reads an absent spread as no spread.

### The documents that must keep loading (C-2)

- **FR-010**: Every document the writer writes shall load, for a one-pass run,
  a multiplexed run and a split run, and each round trip shall answer the
  values it wrote. The scripted run of the load test answers the multiplex form,
  and the split form of a written document stands in the pass report test
  (D-18).
- **FR-011**: A document whose entry names the stopped outcome shall load, in
  the form the writer makes it: the settled method beside a count of 0, an empty
  spread and empty tags (D-14). The hand-built stopped entry of the rich
  fixture, which names `PASSES` with a count of 2, shall go on loading.
- **FR-012**: A schema 1.0 document that carries no pass key shall load and
  leave each pass member at the default of a run that collected no pass.
- **FR-013**: The writer, the schema file, the schema checker and the
  comparison rules shall stand unchanged by this feature. (D-10.)

### Disclosure (C-3)

- **FR-014**: The loader section of `docs/pages/harness.md` shall state the
  three checks in two to four sentences, the page wrapping at 80 columns, and
  `README.md` shall stand unchanged. No gate counts lines of a docs section: the
  drift gate compares marked regions and the forbidden classes, and the prose
  gate reads rules per line, so the count stands as a review item. (D-7, IV.1.)

### Version and numbering (C-4)

- **FR-015**: The project version shall move to the next free patch at branch
  time, `0.30.1` at the measured base, and the two test sites that pin the
  version shall move with it. The two `SOVERSION` numbers and the schema
  version shall stand unchanged. (D-2.)

### Tests and gates (C-5)

- **FR-016**: Each refusal of "The rules" shall carry its own test, and each
  accepted form of "The rules" shall carry its own test, including the
  stopped-loop form and the schema 1.0 form. CTest registers one entry for the
  whole load test (`test/CMakeLists.txt:775-783`), so the case names, which the
  test log prints, carry the count.
- **FR-017**: No test of this feature shall sleep, read a clock, name a
  made-up cpu, or require a real PMU.
- **FR-018**: The change shall keep line coverage at 100 percent, shall keep
  the branch shortfall within the recorded allowance of 21 rows, and shall add
  no coverage exclusion construct. The measurement point is the coverage job's
  union of a full suite, the reading the gate takes
  (`tools/dbc/coverage_gate.sh:86-88`). A developer host reads higher by design,
  because an entry there stops early and leaves the arms past its stop dark
  (`:90-101`), so the lane reports the host reading at the base and at the head
  beside the union figure. (D-12.)
- **FR-019**: The change shall pass `ctest --test-dir build --output-on-failure`
  in full and the same command under `taskset -c 0,1` on the lane host, and
  shall pass the five gates at their own command lines: `cmake -P cmake/lint.cmake`
  with the pinned clang-format 18.1.8 (`.github/workflows/ci.yml:33-34`),
  `cmake -P cmake/spell.cmake`, `python3 tools/docs/example_drift.py .`,
  `python3 tools/prose/prose_gate.py --check all --mode range --base
  origin/master`, and the dbc pair gate. (D-20.)
- **FR-020**: Each test shall stand in the same commit as the code it tests,
  and every commit shall follow the commit template of the constitution.

## Clarifications

### Session 2026-11-03

Round 1 settled the reader axis: three workers read the loader, the writer, the
runner and the load test, and each answer below carries its measurement.

- Q: Does rule 1, as the ticket words it, refuse a document the harness writes?
  → A: Yes, in two forms, so rule 1 runs on a `MEASURED` entry that carries no
  `hunt` key. The runner settles the method before the warm-up and publishes a
  count of 0 beside `MULTIPLEX` or `PASSES` on four stop paths
  (`source/harness/runner.cpp:2884`, `:2889`, `:2923`, `:2928`), and an
  interrupt names the outcome `SKIPPED` (`:94-98`). The hunt path names the
  forced method beside the count the collection published
  (`source/harness/cli.cpp:1788-1789`) in the branch that fills the entry's hunt
  record (`:1775`), the key the writer writes
  (`source/harness/json_report.cpp:859-862`). Rules 2 and 3 keep no waiver: a
  stopped collection publishes no spread and no tag
  (`source/harness/runner.cpp:1720-1722`), the merge writes every tag above one
  pass (`:3303`, `source/harness/pass_plan.cpp:322`), and `passSpread` answers
  nothing below two times (`pass_plan.cpp:404-408`). (D-14, FR-003.)
- Q: Where does the tag check run, and which fault does a document with two
  faults report? → A: In `readEntry`, after the pass block, over the rows the
  entry has read. The loaded row and metric indices equal the document indices
  (`source/harness/json_load.cpp:2640-2656`, `:1589-1598`), so the refusal path
  rebuilds byte for byte. The order is: the rows, the standing `spreadWarning`
  check, rule 1, rule 2, rule 3. The first fault stops the read, the standing
  stop of the fold (`:120-133`, measured by
  `test/source/harness_json_load_test.cpp:706-739`). (D-16, D-9.)
- Q: What counts as "both keys present" for each rule? → A: Rule 1 counts a key
  present when it stands in the entry, and a value of the wrong kind keeps the
  standing kind refusal ahead of every pair check
  (`source/harness/json_load.cpp:135-144`, `:158-171`, `:196-211`). Rules 2 and
  3 count a key present when it stands and holds a non-null value, the reading
  the loader takes today (`:2744-2745`, `:388-389`). (D-17, FR-002.)
- Q: A refusal names two fields, and one line carries one leading path. Which
  shape? → A: One line whose leading path names the pair's non-count field, with
  the partner's full path in the words:
  `/benchmarks/0/metricMethod: the method 'AUTO' disagrees with the pass count
  at /benchmarks/0/passCount`. The shape keeps the standing style
  (`source/harness/json_load.cpp:57-58`, `:2759-2761`) and makes every US-1
  scenario a binary check. (D-15, FR-007.)
- Q: Which rows does the tag check read, and does a context block carry a metric
  value? → A: Every row of the entry that carries the count, and no context
  block. A context block carries percentile columns and raw columns alone
  (`source/harness/json_load.cpp:1658-1689`, `:296-360`); metric values live at
  `BenchmarkResult::rows` to `ResultRow::metrics` to `MetricValue::pass`
  (`include/speedgun-ng/benchmark.hpp:885`, `:407`, `:251`). (FR-006.)
- Q: Does the pair rule of FR-002 gate the standing `spreadWarning` check? → A:
  No. That check reads a different pair, runs when its own key stands, and reads
  an absent spread as no spread, because `spreadWarned(nullopt)` is false
  (`source/harness/json_report.cpp:1038-1041`). A document with
  `"spreadWarning":true` and no `passSpread` key refuses today and goes on
  refusing. (FR-009.)
- Q: Which schema versions does "any older minor of major 1" name for a test? →
  A: 1.0, 1.12 and 1.13. The reader gates the major alone
  (`source/harness/json_load.cpp:3129-3135`). (Edge Cases, D-6.)
- Q: Does US-1's comparison scenario need a new test? → A: No. The standing path
  answers status 2 with one quoted line for any load refusal
  (`source/harness/compare.cpp:1876-1890`), and
  `test/source/harness_compare_cli_test.cpp:782-799` measures that shape.
  (FR-008.)
- Q: What count does the writer's stopped form name, and is the rich fixture's
  stopped entry that form? → A: The writer's stopped form names a count of 0
  beside its settled method, with an empty spread and empty tags. The rich
  fixture's stopped entry names `PASSES` with a count of 2, a hand-built form
  the writer does not make
  (`test/source/harness_json_load_test.cpp:181-218`). FR-011 names both.
- Q: Can one scripted run produce the three accepted written forms? → A: No. The
  scripted run of the load test registers one metric column on an object that
  publishes no limit, so it settles `MULTIPLEX`
  (`test/source/detail/json_fixture.hpp:96-122`,
  `source/harness/pass_plan.cpp:131-135`). The split form of a written document
  stands in the pass report test (`test/source/harness_pass_report_test.cpp:833-835`),
  and the load test builds the split form by hand. (D-18, FR-010.)
- Q: Is a pass index 0-based? → A: Yes. The merge names the measuring pass from
  0 (`source/harness/detail/pass_plan.hpp:47-49`), and US-1 scenario 5 plants a
  tag of 2 beside a count of 2. (FR-006.)
- Q: Which documents does the pair rule leave loading, beside the pre-specs/024
  form? → A: A count with no method key, and a method with no count key. The
  pair rule answers both, and no key becomes required by the back door. (Edge
  Cases, FR-004.)
- Q: Does the schema checker carry a cross-field rule? → A: No. The schema states
  key sets, order and kinds (`docs/schema/report-v1.json:517-520`, `:654-662`,
  `:913-919`), and `loadReport` runs no schema check
  (`source/harness/json_load.cpp:3101-3247`). The loader is the only gate, and
  the docs lines of FR-014 say so.
- Q: Where does the coverage number stand? → A: In the pull request body beside
  the G-20 figures, with the host reading at the base and at the head and the
  reading of the changed file at the head. (SC-002, D-12.)

Round 2 settled the gates axis and the compatibility axis. Two workers measured
on this host, and each answer below carries its measurement.

- Q: Does the allowance of 21 rows bound the arms the change adds? → A: No. It
  bounds the shortfall. An arm a test reaches adds to the total and to the hit,
  so it costs nothing; an arm no test reaches costs one row
  (`tools/dbc/coverage_gate.sh:111-118`). The CI union reads 0 rows short today
  (`specs/043-fwq-ftq/plan.md:122`), so all 21 rows stand free. Measured with
  the coverage flags and `gcov -b -c`: 8 records for rule 1 with a pair guard and
  a min/max pair, 6 for rule 2, 12 for rule 3, none taken zero; the two guards of
  D-14 add four, all coverable. (D-19, FR-018.)
- Q: Which shape of a check keeps every arm covered? → A: One `if` per refusal, a
  pair guard as one `&&` of the two key tests, and no `switch` over the method
  words: a jump table adds a fourth arm no document reaches, because the standing
  kind check refuses a word outside the three names
  (`source/harness/json_load.cpp:2728-2732`). No check stands as `SG_ENSURE`: the
  extract step omits those lines (`cmake/coverage.cmake:101`) and the enforce
  response calls `std::abort()` (`include/speedgun-ng/dbc.hpp:213`). (D-19.)
- Q: Does the change need a new test target, a new CTest entry, or a new
  fixture? → A: No to all three. The load test runs 27 functions today, called
  from `main` (`test/source/harness_json_load_test.cpp:2205-2238`), and its
  target and CTest entry exist (`test/CMakeLists.txt:775-783`). The compare
  pairing of D-21 goes in `harness_compare_cli_test.cpp`, which writes documents
  at run time. (D-21, FR-016.)
- Q: Which host tools must the lane pin? → A: clang-format 18.1.8, which CI
  installs by pip (`.github/workflows/ci.yml:33-34`): the host 23.1.4 names a
  long list of untouched files as badly formatted and the gate exits 1, and the
  pinned tool answers exit 0. And `PERL5LIB`, which carries `Capture::Tiny` for
  the host `lcov` (`~/perl5/lib/perl5`): without it the capture, the extract and
  the gate exit 2, 2 and 3. (D-20.)
- Q: Which committed document carries a pair the rules refuse? → A: One,
  `test/data/gate_schema_20.json:66-68`, `AUTO` beside a count of 1. The version
  gate refuses it ahead of the entry read
  (`source/harness/json_load.cpp:3129-3135`), so
  `test/source/harness_compare_cli_test.cpp:875-884` keeps its verdict. Every
  other committed document with a pass pair answers the rules:
  `report_views/multipass.json` (`PASSES`, 3, tags 0 to 2), `gate_schema_11.json`
  and `hunt_no_hunt_document.json` (`AUTO`, 0), the specs/038 to specs/040
  reports, the series fixture and `kRich`. The twelve gate fixtures and the
  report views of `markup.json`, `multi.json`, `nospeedgun.json` and
  `single.json` carry no entry pass key, and a metric tag that stands null
  answers no check. (FR-011, FR-012.)
- Q: Can the schema file state the three rules? → A: No. The checker's language
  carries `type`, `required`, `x-order`, `additionalProperties`, `enum`,
  `properties` and `items` (`source/harness/json_load.cpp:2815-2897`), and
  `checkReport` is a separate reader from `loadReport` (`:3055-3097`). No
  cross-field construct exists, and the loader is the only gate. (FR-013.)
- Q: What does the version move touch? → A: The three pinned sites of FR-015.
  `SG_PROJECT_VERSION` is a `PRIVATE` define of the harness target
  (`CMakeLists.txt:323-327`), so the harness library rebuilds whole and every
  harness test relinks. The docs sample pair keeps the version of the run that
  recorded it, and the shape gate folds digits
  (`tools/docs/report_shape.py:140-150`), so it stands. (FR-015.)
- Q: Which docs text passes the drift and prose gates? → A: Three sentences after
  `docs/pages/harness.md:1429` that name the three pairs, the pair rule, and the
  three method words. The drift gate's own classes answer no hit for the docs
  page (`tools/docs/example_drift.py:491-503`), and the scan strips inline code
  spans first (`:455-462`), so a field name in backticks is safe. The prose rules
  answer no finding. (FR-014.)
- Q: Which spec claims does no gate verify? → A: Six, and each now names its
  check. The allowance citation, re-anchored to `coverage_gate.sh:102`. The
  coverage measurement point, named by FR-018. The gate command lines, named by
  FR-019. The docs line count, now a count of sentences and a review item
  (FR-014). The test case count, carried by the case names in the test log
  (FR-016). The compare pairing, carried by the plant of D-21 (FR-008).
- Q: What is a stopped-loop document, and what is a schema 1.0 document? → A: The
  Key Entities block names both, with the schema words for the outcome
  (`docs/schema/report-v1.json:527-530`) and the version rule
  (`source/harness/json_load.cpp:3125-3135`).

## Key Entities

- **Pass disclosure**: the four fields one entry carries about the collection
  method: the settled method, the pass count, the pass spread, and the spread
  warning. Specs/024 FR-038 publishes them.
- **Pass tag**: the index of the pass that measured one metric column, the
  field a one-pass run leaves null.
- **Pair**: the two fields one check reads. A pair with a missing key answers
  no check.
- **Refusal**: one load error naming the field path the fault stands at, the
  one line the comparison quotes.
- **Accepted form**: a document a writer can make, the writer of the harness
  included, and the form a check must leave loading.
- **Stopped-loop document**: a report whose entry names the `SKIPPED` outcome and
  a reason that names why the loop stopped (`docs/schema/report-v1.json:527-530`,
  `source/harness/runner.cpp:90-98`).
- **Schema 1.0 document**: a report whose `schemaVersion` reads `1.0` and that
  carries no pass key, no hunt key and no topdown key
  (`source/harness/json_load.cpp:3125-3135`, `docs/pages/harness.md:1395-1403`),
  the form of the twelve gate fixtures in `test/data`.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: One test stands for each refusal and one for each accepted form,
  the stopped-loop document and the schema 1.0 document included. The case names
  in the load test's log carry the count (FR-016).
- **SC-002**: Line coverage reads 100 percent, the branch shortfall reads 21
  rows or fewer with the number recorded in the pull request body beside the
  G-20 figures, and no new exclusion construct appears in the change. The
  coverage job's union of a full suite judges that gate; a developer host reads
  higher, because an entry there can stop early and leave the arms past its stop
  dark, so the lane reports the host reading at the base and at the head, and
  the reading of the changed file at the head.
- **SC-003**: `ctest` in full and `ctest` under `taskset -c 0,1` pass on the
  lane host chilly, and no test of this feature sleeps, reads a clock, or
  names a made-up cpu.
- **SC-004**: The lint, spell, example-drift, prose and dbc-pair gates pass at
  the command lines of FR-019, on the host of D-20.
- **SC-005**: Every document the harness wrote before this change loads after
  it, measured by the round-trip tests of FR-010 to FR-012.

## Constraints

- ASD-STE100 Simplified Technical English stands over every generated word,
  with the constitution's Principle XI.
- Each test stands in the same commit as the code it tests. No code shaped for
  coverage alone.
- Commit author and committer: `OmO <omo@speedgun-ng.local>`. Titles run 50
  characters or fewer, body lines 72 or fewer, and the footer's last two lines
  are `Refs: specs/047-loader-cross-field T-LCF` and `Approved-by: OmO`.
- Rebase only. No merge commit. No CI change.
- The first push is normal. A later push uses
  `git push --force-with-lease` against the last pushed sha, and
  `git ls-remote origin 047-loader-cross-field` then equals `git rev-parse
  HEAD`. A failed lease stops the lane and reports.
- The lane never merges.
- Local acceptance (G-20) runs on chilly, and the pull request body names the
  host, the kernel, the GCC version, the head sha, the two `ctest` counts and
  the coverage figures. A CI run on a GitHub runner answers no acceptance.

## Scope

In scope:

- `specs/024-multipass-collection/spec.md`, the FR-038 amendment only.
- `source/harness/json_load.cpp`, `readEntry` and its helpers.
- `test/source/harness_json_load_test.cpp`, new functions called from its
  `main`.
- `test/source/harness_compare_cli_test.cpp`, the one pairing test of D-21. No
  entry of `test/CMakeLists.txt` changes: the two targets and their CTest
  entries already exist (`test/CMakeLists.txt:775-783`).
- `docs/pages/`, the loader section.
- `specs/047-loader-cross-field/`, this feature's artifacts.
- `CMakeLists.txt` version line and the two version test sites.

Out of scope:

- The writer, the schema file, the schema checker, and the comparison.
- The files of specs/045: `source/counters/`, the jitter-probe and FWQ/FTQ
  files, `test/prose-gate-fixture/`, `specs/043-fwq-ftq/`.
- The hunt record code of specs/046.
- The public API, the two `SOVERSION` numbers, the schema version, and every
  option value.

## Assumptions

- The lane host is chilly (AMD Zen 4, Ryzen 9 7950X3D). The checks read no
  host fact, so no second-vendor run is needed.
- Specs/045 and specs/046 land on their own branches. The rebase rule of D-13
  settles the overlap with specs/046.
- The owner's four recommended answers stand as D-2, D-5, D-6 and D-7.
- The comparison path of FR-008 needs no change, measured at
  `source/harness/compare.cpp:1876-1890`.

## Dependencies

- Specs/024 FR-038, the requirement this feature amends.
- Specs/023, the reader and its refusal shapes.
- Specs/026 FR-051, the stored baseline a schema 1.0 document answers.
- The coverage allowance of 21 rows recorded by specs/030 D-14.

## Citations

- `specs/024-multipass-collection/spec.md:658-676`, FR-038 as it stands.
- `source/harness/json_load.cpp:2726-2764`, the entry's pass block.
- `source/harness/json_load.cpp:387-395`, the metric pass tag.
- `source/harness/json_report.cpp:849-855`, `:323`, the writer of the keys.
- `source/harness/runner.cpp:1724-1729`, the publisher of the count.
- `docs/schema/report-v1.json:517-520`, `:654-662`, the required keys.
- `source/harness/compare.cpp:1876-1890`, the status 2 path.
- `tools/dbc/coverage_gate.sh:102`, the allowance of 21 rows, and `:86-101` for
  the measurement point and the host reading.
- `test/source/harness_json_load_test.cpp:181-218`, the schema 1.0 fixture.

## Open questions

None. The four questions of the request stand taken as D-2, D-5, D-6 and D-7.

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes no behavior the root
  `README.md` teaches. The loader rule lands in `docs/pages/harness.md`
  (FR-014, D-7).
- Principle II: the reader gains no new public interface, so no new contract
  stands to document. The checks run inside the documented `loadReport`.
- Principle VI: each refusal ships with its test in the same commit (FR-020).
- Principle VIII: the gates of FR-019 are the hard gates of the change.
- Principle IX: the four artifacts stand in `specs/047-loader-cross-field/`.
- Principle X: the change touches the loader, its tests, one spec line, one
  docs section and the version line, and nothing else (Scope).
