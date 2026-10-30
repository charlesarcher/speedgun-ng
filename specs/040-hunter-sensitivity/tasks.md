# Tasks: Hunter Sensitivity and Baseline Delta

**Input**: Design documents from `/specs/040-hunter-sensitivity/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories),
[contracts/hunt-ranks-api.md](./contracts/hunt-ranks-api.md),
[data-model.md](./data-model.md), [quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Test Plan): the covering test task of
each wave stands written and observed failing before the
implementation task that turns it green. The test rows join the
standing test binaries plan.md names
(`harness_hunt_rank_test`, `harness_hunt_json_test`,
`harness_hunt_cli_test`, `harness_cli_table_test`,
`harness_json_load_test`, `harness_json_schema_test`); no new test
target joins.

**Organization**: tasks stand grouped by wave behind the module
landing order plan.md fixes (M-A to M-G): the public surface and the
version bindings stand first, the statistics surface and the document
keys land in one parallel block, the rank figures stand next, the
CLI, the console and the run path follow, the docs and the evidence
close. Every task names its exact file path, the requirements it
closes, and the verify command with an exit status.

## Format: `[ID] [P?] [Wave] Description`

- **[P]**: can run in parallel (different files, or one file behind
  disjoint scenario blocks)
- **[Wave]**: the wave this task serves (the plan's wave list)
- Every description names the exact file path, the requirements it
  closes, and the verify command

## Requirement coverage

| FR | Wave | Test tasks |
| --- | --- | --- |
| FR-001 | 3 | T018, T022 |
| FR-002 | 3 | T018, T022 |
| FR-003 | 3, 4 | T019, T023, T031, T034 |
| FR-004 | 3, 4 | T020, T024, T031, T034 |
| FR-005 | 3, 4 | T019, T023, T029, T032 |
| FR-006 | 2 | T010, T011 |
| FR-010 | 4 | T028, T032 |
| FR-011 | 3, 4 | T021, T025, T029, T032 |
| FR-012 | 4 | T030, T033 |
| FR-020 | 1 | T002, T003 |
| FR-021 | 4 | T026, T027, T031, T034 |
| FR-022 | 4 | T029, T032 |
| FR-023 | 4 | T029, T032 |
| FR-024 | 1, 4 | T004, T005, T026, T027 |
| FR-030 | 2, 4 | T012, T015, T031 |
| FR-031 | 2 | T013, T016 |
| FR-032 | 1, 2 | T008, T009, T014, T017 |
| FR-040 | 5 | T036 |
| FR-041 | 5 | T035, T036 |
| FR-050 | 1 | T006, T007 |
| FR-051 | 2-4 | every FR of (A) to (E) names its test in this table |
| FR-052 | 6 | T037, T038 |
| FR-053 | 7 | T039 |

---

## Phase 1: Setup and Foundational - the public header, the version move and the fixtures (module M-A, wave 1)

**Purpose**: the surface every story stands on, the version bindings
the lane stands on, and the fixtures the later rows load. No other
module builds before this phase closes.

- [ ] T001 [P] Write the fixture documents under
  `test/data/hunt_ranks/`: one baseline document whose matched entry
  carries per-entry hunt objects with known median rates and a norm
  word, one baseline whose matched entry carries no hunt object, one
  baseline with a rate at zero, one baseline whose top-level hunt
  object names a norm the contender carries none of, and one
  multi-entry family document (FR-051's fixture half, D-4, D-5).
  Verify: the files load through `loadReport` in a temporary check
  run of `ctest --preset=dev -R harness_json_load_test` (the standing
  reader binary).
- [ ] T002 [P] Write the failing option rows in
  `test/source/harness_cli_table_test.cpp`: the two rows
  `--hunt-emit-rules` at value `281` and `--hunt-baseline` at value
  `282` join the contract-order list and the value list, both named
  constants, long spelling only, no short letter, no environment name
  (FR-020, PC-4). Verify:
  `ctest --preset=dev -R harness_cli_table_test` red on the two rows.
- [ ] T003 Add the two option rows to
  `source/harness/detail/options.hpp` and
  `source/harness/detail/options.cpp` at values `281` and `282`,
  named constants, long spelling only; values `283` to `289` stay
  free (FR-020, PC-4). Verify:
  `ctest --preset=dev -R harness_cli_table_test` green.
- [ ] T004 [P] Write the failing surface rows in
  `test/source/harness_hunt_rank_test.cpp` (the surface block): a
  default `RankedEvent` answers the three fields
  `sensitivityCorrelation`, `sensitivitySlope` and `delta` empty, and
  `EntryHunt::toCompareRules(std::size_t, double)` declares beside
  `toCounterSet` with no `detail/` type in the signature (FR-024,
  D-1). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the missing
  fields (the block fails to compile).
- [ ] T005 Extend the public surface in
  `include/speedgun-ng/hunt.hpp`: `RankedEvent` gains the three
  optional fields, `EntryHunt` gains the `toCompareRules`
  declaration with the doxygen contract of B-1 (FR-024, D-1).
  Verify: `ctest --preset=dev -R harness_hunt_rank_test` green on the
  surface block.
- [ ] T006 [P] Write the failing version rows in
  `test/source/harness_cli_table_test.cpp` (the version block, the
  scenario block disjoint from T002's): the project version reads
  `0.26.0`, the harness `SOVERSION` reads `17`, and the archive
  `SOVERSION` reads `4` (FR-050). Verify:
  `ctest --preset=dev -R harness_cli_table_test` red on the three
  rows.
- [ ] T007 Move the version block in `CMakeLists.txt`: `VERSION
  0.26.0`, the harness `SOVERSION 17`, the archive `SOVERSION 4`, the
  comment recording the checks beside the numbers (FR-050, PD-20).
  Verify: `ctest --preset=dev -R harness_cli_table_test` green, and
  the commit message carries the record.
- [ ] T008 [P] Write the failing schema-constant rows: the minor row
  of `test/source/harness_json_report_test.cpp` moves from `1.8` to
  `1.9`, and `test/source/harness_hunt_json_test.cpp` pins the
  document's `schemaVersion` at `1.9` (FR-032's constant half).
  Verify:
  `ctest --preset=dev -R 'harness_hunt_json|harness_json_report'` red
  on the minor rows.
- [ ] T009 Move `kSchemaVersion` to `1.9` in
  `source/harness/json_report.cpp` (FR-032). Verify:
  `ctest --preset=dev -R 'harness_hunt_json|harness_json_report|harness_json_schema'`
  green.

## Phase 2: The statistics surface and the document keys in parallel (modules M-B and M-D, wave 2)

**Purpose**: the two helpers the figures call, and the three document
keys the reader pairs at the new minor. Each module stands behind
Phase 1 alone, so the two land beside each other.

- [ ] T010 [P] Write the failing statistics rows in
  `test/source/harness_hunt_rank_test.cpp` (the statistics block):
  the monotone point set answers a correlation of `1`, the
  anti-monotone set answers `-1`, the tie set averages the ranks, the
  set with no spread in either coordinate answers nothing, the set
  under three points answers nothing, the exact line answers its
  slope, and the slope reads the sums the complexity fit shares
  (FR-006, B-2, B-3). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the block.
- [ ] T011 [P] Implement `sg::detail::spearmanCorrelation` and
  `sg::detail::leastSquaresSlope` in
  `source/harness/detail/statistics.hpp` and
  `source/harness/statistics.cpp`: the complexity fit's least-squares
  sums move beside them so one implementation stands, the fit calling
  the moved helper unchanged (FR-006, D-8, R-4). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` green on the
  statistics block, and
  `ctest --preset=dev -R harness_complexity_test` green (the fit's
  behavior stands unchanged).
- [ ] T012 [P] Write the failing writer rows in
  `test/source/harness_hunt_json_test.cpp`: the per-entry hunt object
  carries `correlation`, `slope` and `delta` in its `x-order` after
  `rank`, a figure the run did not measure writes `null`, and every
  key stands in every hunt document (FR-030, B-9). Verify:
  `ctest --preset=dev -R harness_hunt_json_test` red on the block.
- [ ] T013 [P] Write the failing reader rows in
  `test/source/harness_json_load_test.cpp`: a written document loads
  back with the three rank fields equal, `null` included (FR-031,
  B-9). Verify:
  `ctest --preset=dev -R harness_json_load_test` red on the block.
- [ ] T014 [P] Write the failing schema rows in
  `test/source/harness_json_schema_test.cpp`: the per-entry hunt
  entry node of `docs/schema/report-v1.json` carries the three keys
  in its `x-order` at minor `1.9` (FR-032, B-9). Verify:
  `ctest --preset=dev -R harness_json_schema_test` red on the block.
- [ ] T015 [P] Implement the writer keys in
  `source/harness/json_report.cpp`: `writeEntryHunt` appends
  `correlation`, `slope` and `delta` after `rank`, `putOrNull`
  writing `null` for an empty figure (FR-030, D-7). Verify:
  `ctest --preset=dev -R harness_hunt_json_test` green.
- [ ] T016 [P] Implement the reader keys in
  `source/harness/json_load.cpp`: the per-entry hunt object reads the
  three fields back (FR-031). Verify:
  `ctest --preset=dev -R harness_json_load_test` green.
- [ ] T017 Add the three keys to `docs/schema/report-v1.json` at
  their `x-order` position (FR-032). Verify:
  `ctest --preset=dev -R harness_json_schema_test` green.
  Note: T015, T016 and T017 land in one commit, for the closed node
  moves once (plan.md Risks).

## Phase 3: The rank figures (module M-C, wave 3)

**Purpose**: the pure stamping surface: the family points, the
sensitivity stamp, the baseline extraction, the delta stamp and the
rank assignment, all over the collected records alone.

- [ ] T018 [P] Write the failing family and stamp rows in
  `test/source/harness_hunt_rank_test.cpp` (the family block): the
  entries of one suite and case name group as one family, one point
  per entry carries the aggregate row's median time per iteration and
  the event's normalized rate under the settled norm, a family under
  three entries and a no-spread point set leave both sensitivity
  fields empty (FR-001, FR-002, B-4, D-2). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the block.
- [ ] T019 [P] Write the failing extraction rows in
  `test/source/harness_hunt_rank_test.cpp` (the baseline block): the
  fixtures of T001 load through `loadReport`, the extraction reads
  the matched entry's hunt rates by catalog address, and the
  norm-pair check answers the mismatch text the refusal prints
  (FR-003, FR-005, B-5, D-6). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the block.
- [ ] T020 [P] Write the failing delta rows in
  `test/source/harness_hunt_rank_test.cpp` (the delta block): the
  matched pair stands `100 x (contender - baseline) / baseline`, and
  a missing match on either side, a missing baseline hunt object and
  a baseline rate at or below zero leave the field empty (FR-004,
  B-6, A-5, D-5). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the block.
- [ ] T021 [P] Write the failing rank rows in
  `test/source/harness_hunt_rank_test.cpp` (the rank block):
  `sensitivity` orders by the correlation descending with ties by the
  slope and then the catalog address, `delta` orders by the magnitude
  descending with ties by the catalog address, the noisy events stand
  unranked under both keys, only an event that carries the key's
  figure carries a rank, and an empty set prints in the standing
  order with empty rank fields (FR-011, B-7, A-3, A-4, D-3). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the block.
- [ ] T022 Create `source/harness/detail/hunt_ranks.hpp` and
  `source/harness/hunt_ranks.cpp`: the family grouping, the point
  assembly and the sensitivity stamp with the doxygen contracts of
  B-4, the new definition joined to the `speedgun-ng_harness` source
  list in `CMakeLists.txt` (FR-001, FR-002). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` green on the family
  block.
- [ ] T023 Implement the baseline extraction and the norm-pair check
  in `source/harness/hunt_ranks.cpp`: the compare mode's `matchPair`
  name rule moves to a shared detail helper the compare calls
  unchanged and the extraction reuses (FR-003, FR-005, B-5, R-2).
  Verify: `ctest --preset=dev -R harness_hunt_rank_test` green on the
  baseline block, and
  `ctest --preset=dev -R harness_compare_test` green (the compare
  calls the moved helper unchanged).
- [ ] T024 Implement the delta stamp in
  `source/harness/hunt_ranks.cpp` with the disclosures of B-6
  (FR-004, A-5, D-5). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` green on the delta
  block.
- [ ] T025 Implement the rank assignment under the two new keys in
  `source/harness/hunt_ranks.cpp` (FR-011, B-7, A-3, A-4). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` green on the rank
  block.

## Phase 4: The CLI, the console and the run path (module M-E, wave 4)

**Purpose**: the figures reach the user and the document: the rank
keys, the refusal family, the two console columns, the rules file and
the batch stamping pass.

- [ ] T026 [P] Write the failing rules rows in
  `test/source/harness_hunt_rank_test.cpp` (the rules block):
  `toCompareRules` answers one
  `counter:<event>@median:<lower|higher>:<n>%` line per promoted
  event, capped at `topN` in rank order, the threshold carrying
  `noisePercent` for every line, the direction standing from the
  delta's sign with `lower` where the delta stands empty, and an
  entry whose rank stands empty holding no line (FR-021, FR-024, B-1,
  D-7). Verify:
  `ctest --preset=dev -R harness_hunt_rank_test` red on the block.
- [ ] T027 Implement `EntryHunt::toCompareRules` in
  `source/harness/hunt_plan.cpp` beside `toCounterSet` (FR-024, B-1).
  Verify: `ctest --preset=dev -R harness_hunt_rank_test` green on the
  rules block.
- [ ] T028 [P] Write the failing rank-key rows in
  `test/source/harness_hunt_cli_test.cpp` (the rank-key block):
  `--hunt-rank` accepts `sensitivity` and `delta` beside the standing
  `rate` and `cv`, and `score` alone keeps the refusal message that
  names N14b (FR-010, PC-1). Verify:
  `ctest --preset=dev -R harness_hunt_cli_test` red on the block.
- [ ] T029 [P] Write the failing refusal rows in
  `test/source/harness_hunt_cli_test.cpp` (the refusal block): each
  refusal prints one stderr line naming the value or the mismatch and
  exits 2: `--hunt-rank=delta` beside no `--hunt-baseline`,
  `--hunt-baseline` beside no hunt, an empty `--hunt-baseline` value,
  `--hunt-emit-rules` beside no hunt, an empty `--hunt-emit-rules`
  value, an unwritable rules path, a baseline file `loadReport`
  refuses, a schema pairing the comparison refuses, and a baseline
  norm different from the contender's (FR-011, FR-022, FR-023,
  FR-005, B-8). Verify:
  `ctest --preset=dev -R harness_hunt_cli_test` red on the block.
- [ ] T030 [P] Write the failing console rows in
  `test/source/harness_hunt_cli_test.cpp` (the console block): the
  ranked table carries the sensitivity column and the delta column
  beside the standing event, rate, CV and noisy flag, an unmeasured
  figure prints `n/a` in its cell, and the `--hunt-top` cap, the
  skipped list and the plan summary stand unchanged (FR-012). Verify:
  `ctest --preset=dev -R harness_hunt_cli_test` red on the block.
- [ ] T031 [P] Write the failing run-path rows in
  `test/source/harness_hunt_cli_test.cpp` (the run block, over a
  `FakeProvider`): the baseline loads once before the entries run,
  the batch stamping pass runs after every entry's hunt stands ranked
  and before the console table prints and the document writes, the
  rules file writes after the stamping pass so the lines carry the
  stamped figures, and the matching-rules disclosure prints once
  beside the figures (FR-003, FR-004, FR-021, FR-030, B-10, FR-051).
  Verify:
  `ctest --preset=dev -R harness_hunt_cli_test` red on the block.
- [ ] T032 Implement the CLI settlement in
  `source/harness/hunt_cli.cpp` and
  `source/harness/detail/options.cpp`: the rank keys accepted, the
  refusal family of T029, and `include/speedgun-ng/hunt.hpp` gaining
  the `HuntOptions` baseline path the CLI settles, empty when the run
  named none (FR-010, FR-011, FR-022, FR-023, FR-005, B-8). Verify:
  `ctest --preset=dev -R harness_hunt_cli_test` green on the rank-key
  and refusal blocks.
- [ ] T033 [P] Implement the console columns in
  `source/harness/report.cpp`: the two columns beside the standing
  four, the `n/a` cells, the standing columns unchanged (FR-012).
  Verify:
  `ctest --preset=dev -R harness_hunt_cli_test` green on the console
  block, and
  `ctest --preset=dev -R harness_cli_table_test` green (the table
  width rows stand pinned).
- [ ] T034 Implement the run path in `source/harness/hunt.cpp` and
  `source/harness/hunt_cli.cpp`: the baseline loaded once through
  `loadReport` before the entries run, the extracted rates and the
  norm check handed to the batch stamping pass, the figures stamped
  after every entry's hunt stands ranked and before the console table
  prints and the document writes, the rules file written through
  `toCompareRules` when `--hunt-emit-rules` stands (FR-003, FR-004,
  FR-021, B-10). Verify:
  `ctest --preset=dev -R 'harness_hunt_cli_test|harness_hunt_rank_test'`
  green.

## Phase 5: The docs teach the ranks (module M-F, wave 5)

**Purpose**: the page, its region and the quick-reference rows move
with the feature, and the drift gates hold them equal to the code.

- [ ] T035 [P] Write the docs region at
  `example/docs/hunt_ranks.cpp` with its
  `example/docs/hunt_ranks.expected`: one registered benchmark the
  `example_run` gate drives, the run printing the two rank columns
  the result sentence names, the region registered through the
  standing `run_docs_` seam (FR-041, IV.1). Verify:
  `ctest --preset=dev -R 'example_run|snippet_compile'` runs the
  region and the rows print.
- [ ] T036 [P] Move the main-page rows in `README.md`: the
  `Hunter mode` section gains the sentences that teach the two ranks
  inside its 25-line cap, one marked snippet sourced from the region
  of T035, one result sentence, and the quick-reference rows for
  `--hunt-emit-rules` and `--hunt-baseline`; the drift gate's tables
  gain the region and the section (FR-040, FR-041, PC-7). Verify:
  `ctest --preset=dev -R 'example_drift|snippet_compile|example_run'`
  green, and `cmake -P cmake/prose-lint.cmake` reads no finding.

## Phase 6: The evidence records (module M-G, wave 6)

**Purpose**: the acceptance runs on gary stand recorded under the
results directory, the way specs/029 and specs/033 recorded theirs.

- [ ] T037 Run the acceptance evidence on gary: record one document
  per run under `specs/040-hunter-sensitivity/results/`: the
  sensitivity run of the quickstart's scenario 2, the delta run with
  the matching-rules disclosure, and the rules round trip of scenario
  3, each record naming the commands, the exit codes and the
  artifacts (FR-052, SC-001 to SC-003). Verify: every recorded
  command exits 0, and every record names the commands and the
  artifacts.
- [ ] T038 [P] Link every record from
  `specs/040-hunter-sensitivity/quickstart.md` beside the scenario it
  proves (FR-052). Verify: every link resolves on the checked-out
  tree.

## Phase 7: Polish, gates and convergence (wave 7)

- [ ] T039 Run the full suite and the hard gates:
  `ctest --test-dir build --output-on-failure`, the coverage read at
  100.0 percent lines and 100.0 percent branches with no
  `LCOV_EXCL` marker and no new `--ignore-errors` entry, the
  `ci-sanitize` and `ci-tsan` presets, the release build
  (`cmake --preset=ci-ubuntu && cmake --build build`) once for the
  analyzer findings, the suite under `taskset -c 0,1`, and
  `cmake -P cmake/prose-lint.cmake` (FR-053, SC-006). Verify: every
  gate green, and the coverage read recorded in the results
  directory.
- [ ] T040 Confirm the standing surfaces stand untouched: the compare
  rows run unchanged (the matcher move of T023 keeps the compare
  green), the option values `283` to `289` stay free, the standing
  rank keys `rate` and `cv` stand unchanged, and the doxygen `docs`
  target documents the new contracts (FR-006, FR-020, FR-024).
  Verify:
  `ctest --preset=dev -R 'harness_compare|harness_cli_table'` green,
  and the doxygen build warns on nothing new.

## Dependencies & Execution Order

### Phase Dependencies

- Phase 1 (M-A) stands first: the surface, the version bindings and
  the fixtures stand the ground every later task builds on.
- Phase 2 (M-B, M-D) depends on Phase 1 alone; its two modules stand
  in one parallel block.
- Phase 3 (M-C) depends on the statistics of Phase 2 (the stamp calls
  the helpers of B-2 and B-3) and on the fixtures of T001.
- Phase 4 (M-E) depends on Phase 3: the run path stamps through the
  surface M-C lands, and the rules file writes through the
  `toCompareRules` of T027.
- Phase 5 (M-F) depends on Phase 4: the example prints the columns
  the CLI carries.
- Phase 6 (M-G) depends on the built feature and the lane host gary.
- Phase 7 closes the feature and stands last.

### Parallel Blocks

- Phase 1: T001, T002, T004, T006 and T008 stand five red-writing
  tasks over disjoint files (T002 and T006 stand behind disjoint
  scenario blocks of one file); T003, T005, T007 and T009 turn them
  green in the order the red rows name.
- Phase 2: T010/T011 (the statistics) and T012 to T017 (the document
  keys) stand one parallel block, the two modules plan.md lands
  beside each other; the red rows of T012 to T014 stand disjoint
  files.
- Phase 3: T018 to T021 stand four red blocks of one file, each
  behind its own scenario block; T022 to T025 turn them green in
  order.
- Phase 4: T026 and T028 to T031 stand the red rows over two files;
  T027 and T032 to T034 turn them green, T033 standing beside the CLI
  and run-path tasks over its own file.
- T035 and T036 stand in parallel over the region and the page;
  T036's snippet reads the region T035 lands. T038 stands beside
  T037's runs.

## Implementation Strategy

### MVP First (User Stories 1 and 2)

Phases 1 to 3 deliver the figures: the library computes the
sensitivity and the delta over the records, the disclosures stand
enforced once, and every later task stands testable. Phase 4 wires
the figures into the run, the console and the document.

### Incremental Delivery

Each phase leaves the suite green: a phase closed is a feature slice
that builds, passes its tests, and stands committed. T015 to T017
stand the one exception: the three land in one commit, for the closed
node moves once.

## Notes

- The commit plan: one logical block per phase, each commit compiling
  and passing its tests, the titles following the template with the
  standing sections (`Harness: ...` for the hunt, the documents and
  the CLI, `CMake: ...`, `Docs: ...`, `Meta: ...`), the `Refs:`
  footer naming `specs/040-hunter-sensitivity`.
- The evidence runs ride the lane host gary and stand a script beside
  the records, no CI gate (FR-052).
- The compare tests stand the guard of the matcher move (R-2): the
  compare rows run unchanged in every wave's full-suite pass.
- The version, the `SOVERSION` and the schema minor stand re-checked
  at rebase against the tree master then stands at, the reservation
  rule of specs/029 FR-090 and FR-053.
