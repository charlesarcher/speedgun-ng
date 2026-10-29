# Tasks: Hunter Budgets and Promotion (033)

**Input**: Design documents from `specs/033-hunter-budgets/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the code task that
turns it green; the pair lands together in the wave's commit series.

**Organization**: tasks stand grouped by user story so each story can
stand implemented and tested independently. US1 and US2 are both P1;
US1 (the round trip) ships first as the MVP because the item's
acceptance criterion names it.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependencies)
- **[Story]**: the user story this task serves (`US1` to `US4`)
- Every description names the exact file path and the requirements
  (FR) and decisions (R) it closes

---

## Phase 1: Setup (re-anchor the surface)

**Purpose**: the audit anchors of the plan stand re-read at the
branch head before any edit.

- [X] T001 Re-anchor every seam symbol of plan.md "Technical Context"
  at the current branch head and record any moved line in that
  section: `parseHuntBudget` (`source/harness/hunt_cli.cpp:440`), the
  `counter` gather arm (`source/harness/options.cpp:1537`), the
  conflict check (`source/harness/cli.cpp:524`), the batch hunt call
  (`source/harness/cli.cpp:1248`), the report write
  (`source/harness/cli.cpp:1536`), `writeHunt` budget text
  (`source/harness/json_report.cpp:706-732`), `readHunt`
  (`source/harness/json_load.cpp:1461`), `printHuntPlan`
  (`source/harness/report.cpp`), `splitStages` pricing
  (`source/harness/hunt_plan.cpp:204`), the hunt option constants
  (`source/harness/detail/options.hpp:93-94`); no code edit

---

## Phase 2: Foundational (blocking prerequisites)

**Purpose**: the shared file reader and the option constants every
story stands on. No story work begins before this phase closes.

- [X] T002 [P] Write the failing shared-reader tests in
  `test/source/harness_hunt_cli_test.cpp`: `detail::readAddressFileLines`
  answers address elements under spec 029's separator rule
  (`source/harness/hunt_cli.cpp:130`, `:171`): each line cuts at the
  first `#`, blank lines stand ignored, one line of `addr1,addr2`
  answers two elements, and a comma beside nothing stands as an
  empty element; an unopenable file refuses in one line naming the
  path; reads one level only (no nested `@`); the existing hunt
  `@file` selections and refusal sentences stay byte-identical
  (029 FR-006 stands, FR-014, R-09)
- [X] T003 Move `readSelectionFile` and `splitElements`
  together (`source/harness/hunt_cli.cpp:130`, `:171`) to
  `detail::readAddressFileLines`, the separator rule riding with
  them: the declaration joins
  `source/harness/detail/hunt_selection.hpp`, the implementation
  lands in a new `source/harness/hunt_selection.cpp` (the harness
  `GLOB_RECURSE` at `CMakeLists.txt:224` picks the TU up), and
  `expandHuntSelection` (`source/harness/hunt_cli.cpp:502`) delegates
  keeping its own refusal sentences; the header exposes both
  halves, the option's own selection text still running through the
  splitter (`hunt_cli.cpp:514`); the tests of T002 turn green
  (R-09) (depends on T002)
- [X] T004 [P] Add `inline constexpr int kHuntEmitSetOption = 280;`
  beside the hunt constants in `source/harness/detail/options.hpp`
  and move the block comment (the `280` to `289` note reads: 280
  taken, 281 to 289 stay free); add the settled-path member
  `huntEmitSet` to `RunOptions` in
  `source/harness/detail/internal.hpp`; no behavior yet (FR-010,
  R-13)

**Checkpoint**: the reader and the constant stand; story work opens.

---

## Phase 3: User Story 1 - Hand the culprits to `--counter` (Priority: P1) 🎯 MVP

**Goal**: `--hunt-emit-set=<file>` writes the ranked events and
`--counter=@<file>` reads them again; the round trip runs in CI.

**Independent Test**: the scripted pair of T012 (hunt with emit,
re-read run, one column per emitted address, none `ABSENT`) closes
roadmap criterion 12.7-6 with no PMU hardware (spec US1).

### Tests for User Story 1 ⚠️ (write first, observe failing)

- [X] T005 [P] [US1] Write the failing `toCounterSet` tests in
  `test/source/harness_hunt_rank_test.cpp`: rank ascending; cap at
  `topN`; a null rank never enters; `topN` zero answers none; `topN`
  above the ranked count answers all; a second call answers equal
  lists (FR-013; contracts/counter-file.md contract lines: "the
  answer holds at most `topN` addresses, the ranked entries in
  ascending rank")
- [X] T006 [P] [US1] Write the failing emit tests in
  `test/source/harness_hunt_cli_test.cpp`: the file holds the run's
  ranked addresses one per line no header, best first; a two-entry
  run writes the deduplicated union, entry order then rank order; a
  hunt that ranks nothing writes an empty file with exit 0;
  `--hunt-emit-set` with no `--hunt`, and beside `--hunt=none`,
  prints one line naming the value and exits 2; an empty value
  prints one line and exits 2; an unwritable destination prints one
  stderr line naming path and reason and leaves status 2 (the
  standing `writeReport` sentence shape) (FR-010 to FR-012, SC-004, R-07,
  R-08)
- [X] T007 [US1] Write the failing `--counter=@<file>` tests in
  `test/source/harness_hunt_cli_test.cpp`: the file's lines become
  one counter column each (20 addresses give 20 columns; one
  line of two comma-separated addresses gives two); `#` comments and
  blank lines are skipped; an unknown address prints one line
  naming the value and ends the run with status 2 (FR-014, D-8); a
  missing file, a comma beside nothing (the empty address), and a
  file holding no address each print one line naming the value and
  exit 2;
  the option-file and environment sources carry the same grammar
  per item: `SPEEDGUN_COUNTER=@culprits.txt;cpu/instructions` reads
  the file and keeps the literal address beside it;
  no count cap binds the file form (FR-014, R-09;
  contracts/counter-file.md) (after T006: the same file)

### Implementation for User Story 1

- [X] T008 [US1] Declare `toCounterSet` on `EntryHunt` in
  `include/speedgun-ng/hunt.hpp` (doxygen `\pre` none, `\post` the
  answer holds at most `topN` addresses, the ranked entries in
  ascending rank, measures nothing, reads no clock) and define it in
  `source/harness/hunt_plan.cpp`, its `SG_ENSURE` postcondition paired with the `\post` (the dbc-gate); the commit moves
  `CMakeLists.txt:15` to the next free minor, `0.23.0` once the
  rebase onto the tree that had taken `0.22.0` landed, and
  `CMakeLists.txt:298` to `SOVERSION 14`; its body records the ABI check with the archive
  `SOVERSION` staying `4` (FR-013, FR-040, R-06, R-15) (depends on
  T005; the tests of T005 turn green)
- [X] T009 [US1] Add the table row `{"hunt-emit-set",
  kHuntEmitSetOption, OptionArgument::REQUIRED, false, help,
  "specs/033"}` with its parse arm in `source/harness/options.cpp`,
  the parse body `parseHuntEmitSet` in `source/harness/hunt_cli.cpp`
  (empty value: the standing one-line exit-2 refusal; else the path
  settles on `RunOptions::huntEmitSet`), and the refusal joins
  `validateHuntCombination` in
  `source/harness/hunt_cli.cpp:660` ahead of its early unset-hunt
  return, its `\post` line moving with it: an emit path with no
  hunt, `--hunt=none` among them, leaves one line naming the value
  with status 2. The same commit moves the
  `--hunt-emit-set` row of the `README.md` Option table and the
  `docs/pages/harness.md` row carrying the table's help string
  (`tools/docs/example_drift.py:744` and
  `test/source/harness_cli_table_test.cpp:1489-1510` read the table
  in both directions; IV.1), and its contract in
  `test/source/harness_cli_table_test.cpp` moves the same way:
  `kContract` gains the 37th row and the hunt-block bound widens to
  270 to 289 beside its header comment (FR-044)
  (FR-010, FR-012, FR-030, R-13, R-08)
  (depends on T004, T006)
- [X] T010 [US1] Implement `writeHuntEmitSet` in
  `source/harness/hunt_cli.cpp` (per entry `toCounterSet(options.top)`;
  the deduplicated union, entry order then rank order; one address
  per line; on an unwritable destination the standing report-file
  failure sentence and a named emit-failed flag that joins the
  `cli.cpp:1544` postcondition's disjunction) and call it from
  `source/harness/cli.cpp`
  after the batch beside the `writeReport` call, guarded by the
  settled path; the tests of T006 turn green (FR-011, FR-012, R-07)
  (depends on T008, T009)
- [X] T011 [US1] Intercept the `@` form in the `counter` gather arm
  of `source/harness/options.cpp:1537`: every `sourceItems` item
  starting with `@` reads one level through
  `detail::readAddressFileLines`, the other items staying
  addresses, the refusals ride the arm's standing `refuse` route,
  an unresolvable address takes the entry point's standing
  refusal, one line naming the value and exit 2 (FR-014, D-8); the
  tests of T007 turn green (R-09) (depends on T003, T007)
- [X] T012 [US1] Add the failing full round-trip test in
  `test/source/harness_hunt_cli_test.cpp` on the scripted host
  (`test/source/detail/pass_fixture.hpp`): a hunt with
  `--hunt-emit-set=F` writes `F`; a second scripted run of
  `--counter=@F` reports one counter column per address of `F`,
  none `ABSENT`; no clock, no sleep, no hardware (FR-015, SC-001, criterion
  12.7-6)
- [X] T013 [US1] Make the round trip of T012 pass with the smallest
  fix at the site the failing test names: emit order in
  `source/harness/hunt_cli.cpp` or the gather interception in
  `source/harness/options.cpp`; no new abstraction (FR-015, SC-001) (depends
  on T010, T011, T012)

**Checkpoint**: the round trip stands green in CI; the item's
acceptance criterion holds without any budget.

---

## Phase 4: User Story 2 - Bound one event's measurement (Priority: P1)

**Goal**: the `e` clause bounds one event's measurement inside a
stage; the plan discloses the derived repeats, the overrun, and the
unpriced state.

**Independent Test**: the fixed-estimate planner fixtures of T014
(derivation, re-price, drop reason) plus the console and JSON
fixtures close SC-002 and SC-005 with no clock.

### Tests for User Story 2 ⚠️ (write first, observe failing)

- [X] T014 [P] [US2] Write the failing planner tests in
  `test/source/harness_hunt_plan_test.cpp`: with a fixed estimate and
  seed the derived repeats equal
  `min(stageRule, max(1, floor(sampleNs / (calibrationNsPerPass ×
  passRuns(plan, stage)))))` (`stageRule`: the stage's own rule
  figure, 1 at stage 1 of a two-stage plan, `options.repeats`
  elsewhere); the estimate re-prices from the derived count;
  the standing stage-drop decision reads the re-priced figure and
  keeps its `over-budget` cut line; a budget below one repeat's
  estimate keeps one repeat and records the overrun; no calibration
  figure keeps the stage rule repeats and records the unpriced state;
  the estimate re-prices when the derived count moves its first
  stage, while a composed plan's plan-level estimate keeps the
  stage-1 figure (the standing split-plan fixture stays green);
  composed with the two-stage budget: stage 1 stays one repeat, stage
  2 takes the derived count; the derivation reads no clock (FR-001,
  FR-002, FR-005, R-02, R-03, R-05; data-model N-03 state table)
- [X] T015 [P] [US2] Write the failing grammar tests in
  `test/source/harness_hunt_cli_test.cpp`, the rows of
  contracts/budget-grammar.md verbatim: `0.5e` and `1e` settle whole
  nanoseconds (`0.5e` keeps `500000000ns`); `0.0000000001e` and an
  `e` word above `kMaxWholeSeconds` refuse; `0.5e,2e` (repeated
  kind), `120s,,32g` shape break (`120s,,1e` for this phase), `120x`
  (no suffix), and leading or trailing blank each print one line
  quoting the whole value and exit 2; `120s` and `8p` parse exactly
  as spec 029 shipped; `1e,120s` (any order) settles both (FR-004,
  R-01)
- [X] T016 [P] [US2] Write the failing console and JSON tests in
  `test/source/harness_hunt_report_test.cpp` and
  `test/source/harness_hunt_json_test.cpp`: the standing plan line
  prints byte-identical for a run without budgets; `sample=<text>`
  appears only when set; the disclosure lines read
  `hunt: plan sample repeats=<d> of <m>`, `hunt: plan sample
  repeats=1 over-budget`, `hunt: plan sample unpriced` in their
  states; the JSON round-trips `sample_budget` (canonical text:
  `0.50e` rebuilds as `0.5e`) and `derived_repeats`; an unset run
  writes the standing bytes apart from `schemaVersion`; a pre-1.7
  hunt document (no keys) loads equal; the refusals wrong kind, `e`
  text with no unit, and `e` text with no amount each carry their own
  test (FR-020 to FR-023, R-10 to R-12; contracts/hunt-json.md)

### Implementation for User Story 2

- [X] T017 [US2] Add `std::optional<std::uint64_t> sampleBudgetNs` to
  `HuntOptions` in `include/speedgun-ng/hunt.hpp` with its contract
  line, and grow `parseHuntBudget` in `source/harness/hunt_cli.cpp`
  to the clause loop: split on `,`, each kind at most once, any
  order; `s`/`p` keep the standing whole-count rule; `e` reads through the
  `parseSeconds` rule hoisted, with `kMaxWholeSeconds`, into
  `source/harness/detail/options.hpp` beside the hunt constants
  (from `options.cpp:138-141`, `:244-275`), so both parsers share
  one bound (finite seconds above zero, fraction allowed, nothing
  below one nanosecond, at most `kMaxWholeSeconds`) into whole
  nanoseconds; every refusal quotes
  the whole value; structure the loop so the `g` clause joins in US3
  without a rewrite; the tests of T015 turn green (FR-004, R-01)
  (depends on T015)
- [X] T018 [US2] Implement the derivation: the repeat-state fields
  land on the internal plan record in
  `source/harness/detail/hunt_plan.hpp`, `splitStages` in
  `source/harness/hunt_plan.cpp` derives the stage repeats, re-prices
  the estimate, and records derived/overrun/unpriced; `runHuntNamed`
  in `source/harness/hunt.cpp` carries the state onto `RunHunt`; the
  two `SG_ENSURE` stage postconditions
  (`source/harness/hunt_plan.cpp:503-510`) and the `HuntStage`
  doc (`include/speedgun-ng/hunt.hpp:314-317`) move to the rule
  that a stage's guarantee figure is the derived count (spec
  D-12); the
  tests of T014 turn green (FR-001, FR-002, FR-005, R-02, R-03)
  (depends on T014, T017)
- [X] T019 [US2] Grow `printHuntPlan` in `source/harness/report.cpp`:
  the conditional `sample=<canonical text>` segment after the
  standing `budget=` and the two disclosure lines in their three
  states; an unset budget
  prints the standing bytes; the tests of T016 (report side) turn
  green (FR-023, R-10) (depends on T018)
- [X] T020 [US2] Land the JSON half in one commit: `writeHunt` in
  `source/harness/json_report.cpp` appends `sample_budget` and
  `derived_repeats` after `stage_two_events` with conditional
  presence (written only when set); `readHunt` in
  `source/harness/json_load.cpp` keeps the standing sixteen in
  the key list and reads the two new keys through `optionalField`
  (`source/harness/json_load.cpp:85`): missing stands unset, a
  present key keeps its kind refusal, with the `e`-text reader in
  the `asBudget` refusal family and one test per refusal
  code (hard rule 7);
  `docs/schema/report-v1.json` gains the two properties, the moved
  `x-order`, and `required` keeps the standing sixteen;
  `source/harness/json_report.cpp:36` moves `kSchemaVersion` to
  `1.7`, and the standing literals move with it in the same commit:
  the written-document check in
  `test/source/harness_json_report_test.cpp:543-549` and the planted
  mutation anchor of `test/source/harness_json_load_test.cpp:707`;
  writer and reader keep the simdjson builder and
  error-code forms; the tests of T016 (JSON side) turn green
  (FR-020 to FR-022 incl. FR-021, R-11, R-12, SC-006)
  (depends on T016, T018)

**Checkpoint**: the sample budget bounds, discloses, and round-trips;
a run without it prints and writes the standing bytes.

---

## Phase 5: User Story 3 - Bound one multiplexed pass (Priority: P2)

**Goal**: the `g` clause caps each multiplex group; a capped
`MULTIPLEX` plan splits into consecutive groups disclosed in the
plan.

**Independent Test**: the 200-events cap-32 fixture of T021 (seven
groups 32, 32, 32, 32, 32, 32 and 8 in plan order) plus the `PASSES`
no-op check close SC-003.

### Tests for User Story 3 ⚠️ (write first, observe failing)

- [X] T021 [P] [US3] Write the failing planner tests in
  `test/source/harness_hunt_plan_test.cpp`: 200 events on a no-limit
  device with cap 32 record `muxGroups` sizes 32, 32, 32, 32, 32, 32,
  8 in plan order; a cap at or above the event count records one
  group; a `PASSES` settlement records no groups and the cap binds
  nothing, and a forced `--metric-method=multiplex` beside a limit
  records none either (spec 029 D-16); composed with the two-stage
  budget the cap applies inside every stage of the split plan; the
  plan's passes carry the groups and the Runner's loop seats them,
  with a scripted per-group open refusal reaching the standing
  single-event retry (D-18 seam) (FR-003, FR-005, R-04, R-05)
- [X] T022 [P] [US3] Write the failing grammar, console and JSON
  tests in `test/source/harness_hunt_cli_test.cpp`,
  `test/source/harness_hunt_report_test.cpp` and
  `test/source/harness_hunt_json_test.cpp`: the `g` clause accepts
  whole counts 1 to 10000 and refuses 0, 10001, fractions, and a
  repeated kind (one line quoting the whole value);
  `mux=<text>` prints only when set; the disclosure line reads
  `hunt: plan mux groups=<n>` with the sizes after it; the JSON
  round-trips `multiplex_cap` and `mux_groups`, omits both when
  unset, and refuses a non-integer member of `mux_groups` (FR-003,
  FR-004, FR-020 to FR-023)

### Implementation for User Story 3

- [X] T023 [US3] Add `std::optional<std::size_t> multiplexCap` to
  `HuntOptions` in `include/speedgun-ng/hunt.hpp` and the `g` clause
  to the `parseHuntBudget` loop in `source/harness/hunt_cli.cpp`
  (whole count, "1 to 10000"); the grammar tests of T022 turn green
  (FR-003, FR-004, R-01) (depends on T017, T022)
- [X] T024 [US3] Implement the split: the group table type and
  declaration land in `source/harness/detail/hunt_plan.hpp`,
  `planHunt` in `source/harness/hunt_plan.cpp` records the
  consecutive groups when the settlement is `MULTIPLEX`
  (`hunt_plan.cpp:171-176`), the window build at
  `source/harness/hunt_plan.cpp:186-194` seats one group per pass,
  and the plan's handoff passes (`buildHandoff`,
  `source/harness/hunt.cpp:416`) carry them to the Runner's loop
  (`source/harness/hunt.cpp:984-1013`); the tests of T021 turn green (FR-003,
  FR-005, R-04) (depends on T018, T021, T023)
- [X] T025 [US3] Land the disclosure half in one commit: the `mux=`
  segment and the groups line in `printHuntPlan`
  (`source/harness/report.cpp`); `writeHunt`/`readHunt` gain
  `multiplex_cap` and `mux_groups` (conditional presence,
  absent-by-default, the non-integer refusal);
  `docs/schema/report-v1.json` gains the two properties and the
  `x-order` insert in the same commit (`multiplex_cap` after
  `sample_budget`, `mux_groups` after `derived_repeats`, the final
  order per the contract), the `mux_groups` refusal carrying
  its own test (hard rule 7); the tests of T022 turn green
  (FR-003, FR-020 to FR-023 incl. FR-021,
  R-11, R-12, SC-006) (depends on T020, T022, T024)

**Checkpoint**: a large catalog produces groups the host can open,
disclosed; an uncapped plan prints the standing bytes.

---

## Phase 6: User Story 4 - Read the page and copy a working block (Priority: P3)

**Goal**: the README, the guide, the option rows and the example
teach the budgets and the promotion inside their caps.

**Independent Test**: the drift gate passes with the `Hunter mode`
section within 25 lines, and the extended example matches its
`.expected` on a CI runner (SC-008).

- [X] T026 [P] [US4] Grow the "Hunter mode" section of `README.md`
  within `SECTION_LINE_CAP` (25 lines total, `tools/docs/example_drift.py:195`),
  files the quick-reference rows in the README's `Quick reference`
  section beside the standing hunt rows (the 029 shape; the cap
  counts the `Hunter mode` section alone), and moves the field and
  grammar rows in `docs/pages/harness.md` (the `--hunt-emit-set` option row
  shipped with T009): the clause grammar
  (`contracts/budget-grammar.md` table), the `--counter=@<file>`
  rows, and the four JSON fields
  (`contracts/hunt-json.md` rows) (FR-030, IV.1, R-14)
- [X] T027 [US4] Extend `example/docs/hunt_rank.cpp` inside its
  scripted host-independent shape: a ranked record built through the
  pure `rankEvents` over scripted samples answers `toCounterSet`, the
  program prints the promoted addresses, and the result sentence
  names the `--counter=@<file>` reading they serve; re-record
  `example/docs/hunt_rank.expected` and move the byte-exact
  `sg-sample-output` block of `docs/pages/harness.md:1616-1630`
  in the same commit (`example_drift` and `report_shape` compare
  that block with the file); the `run_docs_hunt_rank` entry
  (`example/CMakeLists.txt:77`) and `example_run` pass on this
  machine and on a CI runner (FR-031, R-14, research D-9)
  (after T026: the same file)
- [X] T028 [US4] Run the drift gates (`example_drift`,
  `example_run`, `report_shape`) and measure the README length: the
  page stands at its measured length under `PAGE_LINE_CAP`, and the
  `--hunt-emit-set` row moved that cap from 880 to 881 beside the
  recorded decision at `tools/docs/example_drift.py:200` (FR-030,
  PC-9)
  (depends on T026, T027)

**Checkpoint**: the page teaches the feature inside its caps.

---

## Phase 7: Polish and cross-cutting

- [X] T029 [P] Run the full suite at the feature head
  (`ctest --test-dir build --output-on-failure`) and the whole suite
  again under `taskset -c 0,1`; every test under `test/source/`
  passes both ways, none skips; any timing-bound assertion stands an
  order of magnitude inside the spec 032 contamination signal
  (FR-042, FR-043, SC-007)
- [X] T030 [P] Run the `ci-coverage` preset of `CMakePresets.json` (the
  `coverage-linux` base CI runs): 100%
  line and branch over `source/harness/` with no `LCOV_EXCL` marker
  and no new `--ignore-errors` entry; a gap stops the work and
  reports its cause with `gcov -b` evidence (FR-041)
- [X] T031 Build once in the release preset (`cmake --preset=ci-ubuntu`,
  `cmake --build build`) so clang-tidy and cppcheck report at the
  optimizer; clear every finding; run the `ci-sanitize` and
  `ci-tsan` presets (SC-007); run `format-check`, `spell-check`,
  `prose-lint`, the dbc-gate, `test/time_source_gate.sh`, and the
  name-check; `harness_cli_table_test` stands green on the moved
  contract (VIII, FR-044) (depends on T029, T030: the release
  configure rewrites the shared `build` cache)
- [X] T032 Run the gary acceptance and record it: the budgeted
  `--hunt=all` run on `example/docs_throughput` (one `--filter`ed
  instance, the 029 gary-record form), the emitted file, the `--counter=@` read-back,
  and the conflict refusal land in a scenario/command/exit/observation
  table in `specs/033-hunter-budgets/results/` in the spec 034
  shape; the record states the standing no-Intel fact (FR-045, SC-009, R-16, D-11,
  quickstart acceptance table) (depends on T029, T030, T031)
- [X] T033 Lane mechanics: rebase onto master (re-read
  `CMakeLists.txt` and take the next free minor and schema minor at
  the rebase, the 029 reservation rule), push once per round with
  `--force-with-lease` and show `git ls-remote origin <branch>`,
  open the pull request, read every CI job after each push including
  the coverage artifact; this work never merges the PR (029 FR-094
  shape, plan "Rebase and lane rules") (depends on T032)

---

## Dependencies and execution order

```text
Phase 1: T001
Phase 2: T002 -> T003   T004 (parallel)
Phase 3 (US1):  T005,T006 (parallel, failing), T007 after T006
                T008 <- T005;  T009 <- T004,T006;  T010 <- T008,T009
                T011 <- T003,T007;  T012 (failing);  T013 <- T010,T011,T012
Phase 4 (US2):  T014,T015,T016 (parallel, failing)
                T017 <- T015;  T018 <- T014,T017;  T019 <- T018;  T020 <- T018,T016
Phase 5 (US3):  T021,T022 (parallel, failing)
                T023 <- T017,T022;  T024 <- T018,T021,T023;  T025 <- T020,T024,T022
Phase 6 (US4):  T026 -> T027 (same file);  T028 <- T026,T027
Phase 7:        T029,T030 (parallel) -> T031 -> T032 -> T033
```

US1 needs no budget work; US2 needs no promotion work; US3 needs the
US2 clause loop (T017); US4 documents what shipped and needs the
public surface of US1 plus the grammar of US2/US3 settled.

## Parallel groups

- Failing-test waves: T005/T006 (T007 follows T006, same
  file); T014/T015/T016; T021/T022;
  T026 then T027 (same file); T029/T030 (T031 follows both).
- Independent singletons: T002 with T004; T008 with T009 with T011
  (different files: `hunt.hpp`+`hunt_plan.cpp`, `options.cpp`+
  `hunt_cli.cpp`, `options.cpp` gather arm: the two `options.cpp`
  tasks touch different arms and may conflict on rebase; run T011
  after T009 when both land in one wave).

## MVP strategy

Ship Phase 1, Phase 2, and Phase 3 (US1) first: the round trip
(criterion 12.7-6) delivers the item's headline with no budget work.
Then US2 (the bound that closes the time story), then US3 (the bound
that opens the large catalog), then US4 (the guide section, the
grammar and field rows, and the example; the two option-table gate
rows IV.1 demands ship inside T009 beside the option row itself).
Polish closes the gates and the record.

## Phase 8: Convergence

- [X] T034 Derive stage 2's repeats from its own pass runs per FR-001, FR-002, contracts/budget-grammar.md, N-03 (contradicts)
- [X] T035 Stand reader rows for the two unexercised sample-budget arcs per FR-041, SC-007, Constitution VI (partial)
- [X] T036 Make the suite pass under `taskset -c 0,1` per FR-042, T029 (partial)
- [X] T037 Restate the stage repeat contract in `hunt.hpp` for the derived count per D-12, Constitution II (contradicts)
- [X] T038 Print the record's derived repeats in the overrun line per FR-002, FR-023, contracts/hunt-json.md, N-06 (contradicts)
- [X] T039 Correct the unknown-address row to the entry point's standing refusal per FR-014, D-8, contracts/counter-file.md (contradicts)
- [X] T040 Except `--hunt-emit-set` from the beside-no-`--hunt` sentence per FR-030, FR-012 (contradicts)
- [X] T041 Re-record the SOVERSION 14 note for the record-layout moves per FR-040, hard rule 12 (contradicts)
- [X] T042 State the capped pass shape and the new plan fields in `planHunt`'s postcondition per Constitution II, FR-003 (partial)
- [X] T043 Correct the guide's hunt option count per FR-030 (contradicts)
- [X] T044 Move the hunt-block bound text in the table test's report per FR-044 (partial)
- [X] T045 Add the `hunt::HuntSampleState` row to the guide's name list per FR-030 (partial)
- [X] T046 Drop the unreachable zero guard beside T034's fix per Constitution X.2 (partial)
- [X] T047 Reuse `parseSeconds`' refusal sentence for the `e` clause per Constitution X.3, R-01 (partial)
- [X] T048 Re-anchor the acceptance record's heads to reachable commits per FR-045, Constitution X.4 (partial)
- [X] T049 Re-run and re-record the acceptance rows the convergence wave changed per FR-045, SC-009 (partial)

## Phase 9: Convergence, second pass

- [ ] T050 Refuse the `e` clause spellings the document reader refuses per FR-004, contracts/budget-grammar.md (contradicts)
- [ ] T051 State the hunt option count and the reserved id block in the comments that own them per Constitution IV (partial)
- [ ] T052 Join a hunt address to its run column through one seam per Constitution X.2, D-23 (partial)
- [ ] T053 Drop the unread leaf copy from the column-join record per Constitution VII (partial)
- [ ] T054 Drop the test-side check that reads no production code per Constitution VI, X.4 (partial)
- [ ] T055 Name the four budget clause kinds in the option's help text per FR-004, FR-030 (partial)
- [ ] T056 State the counter file's comma separator in the guide's counter paragraph per FR-014, contracts/counter-file.md (partial)
- [ ] T057 Stop the example naming a file the run never writes per Constitution IV.1 (partial)
- [ ] T058 Restate the contrastive comments the prose gate cannot read per Constitution XI.2, XI.6 (partial)
- [ ] T059 Restate the version reservation and the overrun quote in the plan and the spec note per FR-040, FR-002 (partial)
- [ ] T060 Re-anchor the plan's re-anchor block to a reachable head per Constitution X.4 (partial)
- [X] T061 Restate the unknown-address rows, the version row and the page-cap row in the task list per FR-014, FR-040, FR-030 (partial)
