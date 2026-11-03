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
  measures it at `test/source/harness_json_load_test.cpp:706-739`). The entry's
  pass block answers its own refusals, and the pass-tag check reads the rows
  the entry has read.
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
  (`tools/dbc/coverage_gate.sh:99`), and the record names the number read.
- **D-13 The overlap with specs/046.** Both lanes touch `json_load.cpp` and
  `harness_json_load_test.cpp`, in different readers of that file. The second
  to merge rebases, keeps both sides after reading them, and runs every gate
  again. No file of specs/045 stands in this lane's scope.

## The rules

The rules are the whole added law. Each reads a pair of fields of one entry.

1. The method beside the count: `AUTO` names a count of 0, `MULTIPLEX` names a
   count of 1, and `PASSES` names a count of 1 or more.
2. The spread beside the count: a spread that stands non-null names a count of
   2 or more.
3. The pass tag beside the count: a tag that stands non-null names a count of
   2 or more, and the tag stands below the count.

The spreadWarning check of specs/024 FR-038 stands as it is.

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
  path.
- A document of schema version 1.13, and one of any older minor of major 1,
  answer the same rules (D-6).

## Requirements *(mandatory)*

### The law (C-1)

- **FR-001**: The spec of specs/024-multipass-collection shall gain the rules
  of "The rules" in its FR-038, in a commit ahead of the loader commit and
  carrying `Refs: specs/024-multipass-collection T-LCF`. (D-8.)
- **FR-002**: The reader shall run a check of a pair only when both keys of
  that pair stand present in the document. (D-4.)
- **FR-003**: When the method key and the count key of an entry both stand
  present, the reader shall refuse an entry whose method is `AUTO` and whose
  count is not 0, an entry whose method is `MULTIPLEX` and whose count is not
  1, and an entry whose method is `PASSES` and whose count is below 1.
- **FR-004**: A document that lacks one key of a pair shall load as it does
  today, and no key of a pair shall become required.
- **FR-005**: When the count key stands present and the spread key stands
  present with a non-null value, the reader shall refuse the entry whose count
  stands below 2.
- **FR-006**: When the count key stands present and the pass tag of a metric
  column stands present with a non-null value, the reader shall refuse the
  entry whose count stands below 2, and the entry whose tag is not below the
  count.
- **FR-007**: Every refusal of a pair check shall travel the standing load
  error path, name the entry and the field path the fault stands at, and carry
  no new error type. (D-5.)
- **FR-008**: The comparison shall answer a document refused by a pair check
  with status 2 and one quoted line, through the path it has today. (D-10.)
- **FR-009**: The `spreadWarning` check of specs/024 FR-038 shall keep its
  present rule, its text and its place.

### The documents that must keep loading (C-2)

- **FR-010**: Every document the writer writes shall load, for a one-pass run,
  a multiplexed run and a split run, and each round trip shall answer the
  values it wrote.
- **FR-011**: A document whose entry names the stopped outcome shall load.
- **FR-012**: A schema 1.0 document that carries no pass key shall load and
  leave each pass member at the default of a run that collected no pass.
- **FR-013**: The writer, the schema file, the schema checker and the
  comparison rules shall stand unchanged by this feature. (D-10.)

### Disclosure (C-3)

- **FR-014**: The loader section of `docs/pages/harness.md` shall state the
  three checks in two to four lines, and `README.md` shall stand unchanged.
  (D-7, IV.1.)

### Version and numbering (C-4)

- **FR-015**: The project version shall move to the next free patch at branch
  time, `0.30.1` at the measured base, and the two test sites that pin the
  version shall move with it. The two `SOVERSION` numbers and the schema
  version shall stand unchanged. (D-2.)

### Tests and gates (C-5)

- **FR-016**: Each refusal of "The rules" shall carry its own test, and each
  accepted form of "The rules" shall carry its own test, including the
  stopped-loop form and the schema 1.0 form.
- **FR-017**: No test of this feature shall sleep, read a clock, name a
  made-up cpu, or require a real PMU.
- **FR-018**: The change shall keep line coverage at 100 percent, shall keep
  the branch shortfall within the recorded allowance of 21 rows, and shall add
  no coverage exclusion construct. (D-12.)
- **FR-019**: The change shall pass `ctest` in full and `ctest` under
  `taskset -c 0,1` on the lane host, and shall pass the lint, spell,
  example-drift, prose and dbc-pair gates.
- **FR-020**: Each test shall stand in the same commit as the code it tests,
  and every commit shall follow the commit template of the constitution.

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

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: One test stands for each refusal and one for each accepted form,
  the stopped-loop document and the schema 1.0 document included.
- **SC-002**: Line coverage reads 100 percent, the branch shortfall reads 21
  rows or fewer with the number recorded, and no new exclusion construct
  appears in the change.
- **SC-003**: `ctest` in full and `ctest` under `taskset -c 0,1` pass on the
  lane host chilly, and no test of this feature sleeps, reads a clock, or
  names a made-up cpu.
- **SC-004**: The lint, spell, example-drift, prose and dbc-pair gates pass.
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
- `test/source/harness_json_load_test.cpp` and the test entries of
  `test/CMakeLists.txt`.
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
- `tools/dbc/coverage_gate.sh:99`, the allowance of 21 rows.
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
