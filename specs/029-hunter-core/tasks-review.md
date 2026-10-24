# Adversarial review: `specs/029-hunter-core/tasks.md`

Audit point: HEAD `668596f`. Reviewed against `spec.md` (D-1 to D-23, PC-1 to
PC-9, FR-001 to FR-094, SC-001 to SC-010), `plan.md` (phases P1 to P10, gates
V-1 to V-10, Shared-list ledger, Test Plan), `research.md` (PD-01 to PD-20),
`data-model.md`, `contracts/`, `quickstart.md` and the tree at that commit.

Scope: machine-locatability, dependency order, registration against the
verification tables, `[P]` write-set honesty, spot-verification against spec FRs
and ACs and plan gates, and the binding conventions. Every line citation named by
a task was opened and read in full.

## What holds up

Line citations were checked exhaustively and are almost all exact at this
commit: `CMakeLists.txt:9` (`VERSION 0.16.0`), `:53` (archive `SOVERSION 3`)
with its comment block at `:43-52`, `:261` (harness `SOVERSION 10`) with its
block at `:239-260`, `:196-200` (the `CONFIGURE_DEPENDS` glob);
`benchmark.hpp:624-641` (`ReportData`); `internal.hpp:434` (`RunOptions`, with
`counterAddresses`, `metricNames` and the four compare members inside `:434-476`
as T028 claims); `runner.cpp:49-82` with its two guards, call sites at `:2569`,
`:2601`, `:2634` exactly, `:1400` (`seatPass`), `:1434` (pass-0 seat),
`:2286-2292` (the `SG_INVARIANT` fuse), `:1363-1367` (the pass column list),
`:866,873` (`kMaxUserCounters`), `:2202` and `:2301` (the dry run's one
iteration); `cli.cpp:69-71`, `:73-81`, `:326-339`, `:346` (the one
`longOptions[]` table, and no `optional_argument` row exists there today, so
T029's "first optional-argument row" is true); `json_report.cpp:31`, `:43-78`,
`:500-510`, `:583-598`; `json_load.cpp:81-118`, `:103`, `:654`, `:1190`,
`:1214-1219`; `report.cpp:36`, `:216`, `:410`; `pass_plan.hpp:27`, `:41-107`,
`:63-107`; `counters_fake.hpp:103`, `:135-248`, `:268`;
`test/CMakeLists.txt:607`, `:1079-1083`, `:1089-1091`, `:1185-1192`;
`example/CMakeLists.txt:74`; `test/example_run.sh:35-36`;
`harness_cli_test.cpp:820`, `:1186`, `:1437`; `harness_json_load_test.cpp:113`,
`:529`; `harness_compare_test.cpp:1191-1202`; `README.md:621` and `:766`;
`docs/pages/harness.md:1067`; `docs/schema/report-v1.json:7` and `:28-50` (the
entry `x-order` holds 21 entries, so "the 22nd entry" is exact);
`tools/docs/example_drift.py:32`, `:63`, `:155` (`PAGE_LINE_CAP = 773`).
No `TBD`, no "as needed", no unlocated task anywhere in the file.

Conventions check clean: PD ids are `PD-01` to `PD-20` with no malformed id in
`tasks.md`, `plan.md` or `research.md`; the D-2 literal stage rule (stage 1 over
every column at one repeat, stage 2 over the top `3 x top`, cut whole, stage 1
never shrinks) reads the same in T031 and T033; the D-13 optional norm refusal
is driven at T015 and worded at `contracts/hunt-cli.md:85`; the zero-column
exit-0 host (D-12) is at T015, T017 and T018; the below-3 device refusal is a
plan-level error at T015 and the exit-2 console refusal at T018, which is the
spelling at `contracts/hunt-cli.md:86`; the dry-run one-iteration-per-pass row at
T018 matches `contracts/hunt-cli.md:116` and `:159`; the gapped window stands in
both lists at T017; values `270` to `277` inside the reserved `270` to `279`
block match `spec.md:145` and `:654` and `PD-17`; harness 11 and archive 4 match
`FR-090` and the plan's ABI table; the quickstart figures in T032 equal
`quickstart.md:116` character for character.

## Findings

### 1. Phase 5 writes `hunt` keys the schema refuses until Phase 7, so its own gate cannot go green

Location: Phase 5, T038 (conformance row), T039 (`writeEntryHunt` "as the last
act of `writeEntry` at `json_report.cpp:500-510`"), T041 (the story gate);
Phase 7, T047.

Problem: the benchmark entry node of `docs/schema/report-v1.json:72` is closed
(`additionalProperties: false`), and `source/harness/json_load.cpp:1105-1108`
turns a key a closed node does not list into a hard refusal naming the pointer.
T039 puts `hunt` inside that node in Phase 5; T047 adds the key to the entry
`x-order` and `properties` only in Phase 7. T038 says so itself: the conformance
cases are "green once Phase 7 lands the schema keys". Then T041 runs
`ctest --test-dir build -R 'harness_hunt_json' --output-on-failure` as the
Phase 5 gate, which runs those knowingly failing cases, and plan.md's
step-and-verify requires `ctest -j 2` green before the next phase starts. The
phase ships red, and an implementer who follows the phase boundary has no
green way to close it.

Fix: move T047 (the two closed hunt nodes, the root and entry `x-order` and
`properties` entries) into Phase 5 ahead of T039, and leave the rest of the
shared list (README, option rows, tool lists, page cap) in Phase 7. The schema
keys are the one shared-list row with an in-tree consumer before Phase 7. The
alternative is to split the conformance case out of T038 into a Phase 7 task,
which is worse: it leaves a writer emitting nonconforming bytes for two phases.

### 2. T049 moves a version pin that must not move, and breaks `harness_compare_cli_test`

Location: Phase 7, T049, third of its four rows (`test/source/harness_compare_cli_test.cpp:516`).

Problem: `harness_compare_cli_test.cpp:495-518` compares two committed fixture
documents (`gate_ok_base.json`, `gate_schema_11.json`) and asserts at `:515-516`
that the second header line names the *fixture's* own header, `speedgun 0.16.0`
and `schema 1.1`. Those values come from `test/data/gate_schema_11.json:2`
(`"schemaVersion": "1.1"`) and `:4` (`"version": "0.16.0"`). Nothing that
`kSchemaVersion` (`json_report.cpp:31`) or `VERSION` (`CMakeLists.txt:9`) does
reaches that assertion; the test exists to prove a `1.0` baseline compares
against a `1.1` contender across the minor (`:491-494`, D-13, SC-013). Editing
`:516` to `1.4` makes the test fail, and no task in the file touches the
fixture. The same line's `speedgun 0.16.0` pin would break the same way if an
implementer carried the `0.19.0` bump into it.

Fix: drop `harness_compare_cli_test.cpp:516` from T049 and state in the task
that `test/data/gate_schema_11.json` is a frozen `1.1` document the bump must not
touch, exactly as the task already says for `:492` and
`harness_compare_test.cpp:1191-1202`. Worth naming the three `1.0` document
fixtures in the same file that also stay (`harness_json_load_test.cpp:158`,
`:177`, `:507`), since the task's "four rows" phrasing invites a sweep.

### 3. Plan phase P8 and gate V-8 appear nowhere in `tasks.md`

Location: `tasks.md:21-25` (the auditability claim); the Phase 3 heading (tasks.md
line 88, "Plan phases: P4 and P5, gates V-4 and V-5"); T030.

Problem: the file promises "Plan phase numbers (P1 to P10) and gate numbers (V-1
to V-10) are named in each phase heading". Grep finds no `P8` and no `V-8` in
the file. All of plan P8's work (the eight options in `hunt_cli.cpp`, the
`cli.cpp` rows and constants, the `report.cpp` printers,
`harness_cli_table_test.cpp`) is folded into Phase 3, which is labeled P4 and
P5. V-8's own observable end state ("`bash test/cli_shape.sh` stays clean",
plan.md P8 row) therefore has no gate number: T030 runs that command under
V-4 and V-5. An implementer or auditor who reconciles gates to tasks finds one
gate missing and cannot tell whether the CLI story was verified.

Fix: either name P8 and V-8 on the Phase 3 tasks that carry the CLI and console
work (T018, T019, T020, T027, T028, T029, T030) or split Phase 3 into a P4/P5
half and a P8 half. Same for P2 and P3, which Phase 2 carries under one heading
labeled "CRITICAL" without naming them.

### 4. T049 depends on T048, yet both carry `[P]` and the ordering section runs them together

Location: Phase 7, T048, T049; "Parallel Opportunities", "Phase 7: T047 to T050
run together on four disjoint files"; `tasks.md:16-19` (the `[P]` definition:
"no dependency on an open task is left open").

Problem: T049 repoints `harness_json_load_test.cpp:113` ("the version this build
writes loads") and `:529` to `1.4`, which passes only once T048 has moved
`kSchemaVersion`. Disjoint write sets, live dependency. Run as written, either
commit in the pair leaves the tree red, against the constitution's atomic and
bisectable commit rule.

Fix: mark T049 unmarked and place it under T048, or state that T048 and T049 are
one commit.

### 5. Unmarked implementation tasks are declared parallel, contradicting the file's own format rule

Location: "Parallel Opportunities", Phase 3 ("T021, T025, T027, T028 and T029 run
beside those pairs on their own files") and Phase 5 ("T039 and T040 run
together"); `tasks.md:16-19` ("`[P]`: runs in parallel with the other `[P]`
tasks of its phase once the unmarked tasks above it have landed").

Problem: T021, T025, T027, T028, T029, T039 and T040 carry no `[P]`, so by the
stated rule they are sequential and must not be dispatched together; the "Stop
and validate" text in MVP First and "Parallel Example" both dispatch them as a
batch. Two readers get two orders.

Fix: add `[P]` to those tasks (their write sets are disjoint:
`detail/hunt_plan.hpp`, `runner.cpp`, `report.cpp`, `hunt_cli.cpp`, `cli.cpp`,
`json_report.cpp`, `json_load.cpp`) or rewrite the ordering section to say they
are sequential.

### 6. Two tasks "Create" the same file: `source/harness/statistics.cpp`

Location: Phase 1, T005 (creates it beside `hunt_plan.cpp`, `hunt.cpp`,
`hunt_cli.cpp`); Phase 2, T009 ("Create `source/harness/detail/statistics.hpp`
and `source/harness/statistics.cpp`").

Problem: the file gets created twice, the second time as a whole-file write that
silently discards whatever T005 left in it. That is also how a stub with real
content in it is lost.

Fix: T009 should say "create the header, fill `source/harness/statistics.cpp`
that T005 created", so the write is an edit of an existing file.

### 7. `harness_json_schema_test.cpp`, named by gate V-7, is in no task

Location: plan.md P7 row ("`harness_json_schema_test.cpp` passes"); Phase 5 T041
and Phase 7 T054; grep of `tasks.md` returns no mention.

Problem: the test exists at master (`test/source/harness_json_schema_test.cpp`,
registered at `test/CMakeLists.txt:613-621` with `SG_SCHEMA_PATH`) and its
oracle is `checkReport` over `docs/schema/report-v1.json`, the file T047 edits.
No task reads it, so the one gate V-7 names for the schema move is verified only
by the whole-suite run at T054, after the fact.

Fix: name it in the Phase 5 gate command (alongside T041's two existing
commands) once finding 1 places the schema keys there.

### 8. Three locators point at code with a different name than the task uses

Location: T022 ("the `devicePart` split of `runner.cpp:39-47`"), T022 ("the
`System::local().object(\"machine\")` and `children()` walk of
`catalog.cpp:81-88`"), T049 ("Move the four rows" for an edit of four files
across three targets).

Problem: `runner.cpp:39-47` defines `splitAddress`, not `devicePart`;
`catalog.cpp:81-88` holds the tail of `printObject` and the `children()` loop at
`:83-85`, while the `object("machine")` lookup stands at `:90`. Both are
resolvable by reading, so neither builds the wrong thing, but a task whose named
symbol is absent from the cited lines is the kind of imprecision that makes an
implementer re-derive the seam.

Fix: name `splitAddress` and cite `catalog.cpp:83-92`; split T049 into one task
per file or reword it to "the four rows across these files".

### 9. `[P]` write-set audit (reported for the record, no defect)

No `[P]` pair shares a file. Across the set the review was asked to name:
`CMakeLists.txt` has one task (T001, `[P]`), `docs/schema/report-v1.json` one
(T047), `README.md` one (T051, unmarked), `internal.hpp` one (T004), `cli.cpp`
one (T029), `hunt.hpp` one (T002). No task edits `CMakePresets.json`, which is
correct: plan.md's Build system row states preset-driven configuration only and
hard rule 14 leaves CI and presets alone. `test/CMakeLists.txt` is split between
T007 and T050 in different phases, both unmarked or non-conflicting, and
`report.cpp` between T027 and T035 in different phases. Findings 4 and 5 above
are dependency and marking errors; neither names a shared write set.

## Judgment

Nine findings, two of which (1 and 2) put a named gate red exactly where the
file says to read it: the Phase 5 checkpoint cannot close while the schema
closes its entry node, and T049 instructs an edit to a cross-minor comparison
assertion that a committed fixture owns. Both have cheap fixes that stay inside
the plan's own rules. The rest are order, marking and locator precision that an
attentive implementer survives.

VERDICT: FAIL
