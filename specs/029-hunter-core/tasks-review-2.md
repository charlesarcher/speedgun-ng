# Adversarial review, round two: `specs/029-hunter-core/tasks.md`

Audit point: `tasks.md` as committed at `1daa010`. Round one:
`specs/029-hunter-core/tasks-review.md`, taken at `668596f`, `VERDICT: FAIL`,
nine numbered findings.

Part one closes findings 1 to 8 against the current file, reading the cited tree
again rather than trusting the round-one citations. Part two hunts the damage of
the reweave: Phase 5, Phase 7, the Phase Dependencies, Story Independence,
Parallel Opportunities, Parallel Example, Notes, MVP text and the coverage table.
Findings 9 of round one (the `[P]` write-set audit) reported no defect and is
re-checked here only where a `[P]` moved: no `[P]` pair inside one phase shares a
file.

## Part one: closure of round-one findings 1 to 8

| # | Finding | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Phase 5 wrote `hunt` keys the schema refused until Phase 7 | CLOSED | `tasks.md:200` puts T047 (`docs/schema/report-v1.json`) inside Phase 5, ahead of T039 at `tasks.md:201` and T040 at `:202`. Phase 7's task list (`tasks.md:253-259`: T048, T049, T050, T051, T052, T053, T054) carries no T047, and the Phase 7 Purpose at `tasks.md:239-241` records the exception: "the schema keys of T047 excepted because they ride with Phase 5 where a closed schema node needs them". Phase Dependencies `tasks.md:311` ("T047 lands the schema keys before the writers of T039 and T040") and Story Independence `tasks.md:330` agree. The reason is real in the tree: the entry node of `docs/schema/report-v1.json:72` holds `additionalProperties: false`, and `source/harness/json_load.cpp:1105-1108` turns an unlisted key of a closed node into a refusal naming the pointer. |
| 2 | T049 moved a pin a committed fixture owns | CLOSED | `tasks.md:254` now reads "Move the three rows that name the written version", names `harness_json_load_test.cpp:113`, `:529` and `docs/pages/harness.md:1067`, and states the frozen set: `test/data/gate_schema_11.json` with `harness_compare_cli_test.cpp:516` and `:492`, `harness_compare_test.cpp:1191-1202`, and "the three `1.0` fixtures at `harness_json_load_test.cpp:158`, `:177` and `:507`". Verified in the tree: `harness_json_load_test.cpp:113` is `withVersion("\"1.1\"")` under "the version this build writes loads"; `:529` is the `schemaVersion` kind mutation row; `:158`, `:177`, `:507` are the three `"1.0"` documents; `harness_compare_cli_test.cpp:515-516` asserts `speedgun 0.16.0` and `schema 1.1` for `gate_schema_11.json`, whose own `:2` and `:4` carry those values; `docs/pages/harness.md:1067` states "A run writes `1.1`". A grep of `test/source`, `test/data`, `example` and `tools` for the written version finds no other file that names it, so "three rows" is complete. |
| 3 | Plan P8 and gate V-8 named nowhere; P2/P3 unnamed | CLOSED | `tasks.md:93`: "Plan phases: P4, P5 and P8, gates V-4, V-5 and V-8"; `tasks.md:96-97` names the carriers and V-8's own check. `tasks.md:72`: "Plan phases: P2 and P3, gates V-2 and V-3". Residual of the same auditability kind, not the same defect: see new finding 4. |
| 4 | T049 depended on T048 while both carried `[P]` | CLOSED | `tasks.md:254` carries no `[P]` and stands under T048 at `tasks.md:253`; Parallel Opportunities `tasks.md:359-360`: "T049 follows T048 because it repoints the version rows at the value T048 writes". |
| 5 | Unmarked tasks declared parallel | CLOSED | `[P]` now stands on T021 (`:122`), T025 (`:126`), T027 (`:128`), T028 (`:129`), T029 (`:130`), T039 (`:201`), T040 (`:202`). Ordering text agrees: `tasks.md:348-351` runs them "beside those pairs on their own files", `tasks.md:354-356` for Phase 5, and the Parallel Example at `tasks.md:368-388` dispatches a subset of that order. Each of those tasks owns one file no other same-phase `[P]` task touches. |
| 6 | T005 and T009 both created `statistics.cpp` | CLOSED | `tasks.md:76` (T009): "Create `source/harness/detail/statistics.hpp` and fill the `source/harness/statistics.cpp` that T005 created". T005 at `tasks.md:53` creates it. |
| 7 | `harness_json_schema_test` named by V-7 stood in no task | CLOSED | `tasks.md:203` (T041): "…`ctest --test-dir build -R harness_json_schema_test --output-on-failure` for the conformance of the keys T047 lands (plan P7 names this test for the schema move)". |
| 8 | Three locators named code with another name | CLOSED | `tasks.md:123` (T022) names "the `children()` walk of `source/harness/catalog.cpp:83-92` under the `object(\"machine\")` lookup at `:90`" and "the `splitAddress` split of `runner.cpp:39-47`". Tree: `runner.cpp:39-47` is `splitAddress`; `catalog.cpp:83-85` is the `children()` loop and `:90` the `object("machine")` lookup, so the cited span holds both. `tasks.md:254` says "three rows", no longer "four rows". |

Eight of eight closed. Part two finds one blocker the fix for finding 2 left
behind, plus a smaller set of ordering and auditability breaks.

## Part two: new findings

### 1. T049 forbids the edit T053 audits as landed, so the ledger audit is red either way

Location: `tasks.md:258` (T053) against `tasks.md:254` (T049); `plan.md:453`
(Shared-list ledger row for `test/source/harness_compare_cli_test.cpp`).

Problem: T053 audits "the plan's Shared-list ledger row by row against the
working tree" and lists `test/source/harness_compare_cli_test.cpp` among the
files, exempting only the last one: "and confirm that last file carries no
edit". The ledger row for that file, `plan.md:453`, reads: "line 516 expects the
console words `schema 1.1`; the value moves with the bump and the pre-1.1
baseline comparison of line 492 stays as it is". T049 now says the opposite of
the first clause. The tree proves T049 right — `harness_compare_cli_test.cpp:515-516`
asserts the header line of `gate_schema_11.json`, whose own `:2` and `:4` carry
`1.1` and `0.16.0`, so no move of `kSchemaVersion` or `VERSION` reaches it and an
edit to `1.4` fails `harness_compare_cli_test`. Both branches therefore end red:
carry the ledger row and the test fails, keep the fixture and T053 reports an
unlanded ledger row — the exact shape round one's finding 1 had, a named check
that cannot close where the file points the implementer. Nothing in the file
records that this ledger row is deliberately not carried, and Principle IX makes
a silent deviation from the plan a defect of its own.

Fix: exempt the file inside T053 the way `harness_pass_report_test.cpp` is
exempted, and name the deviation where the merge notes are written, for example:
"`test/source/harness_compare_cli_test.cpp` carries no edit either: its `:516`
asserts the frozen fixture's own header, so the ledger row of plan.md that moves
that value is not carried, and the deviation is recorded in the merge notes with
its reason". Amending `plan.md:453` in the same commit is the better half of the
fix; the T053 wording is the part that keeps this file self-consistent.

### 2. T047's `[P]` contradicts the order that makes Phase 5 green

Location: `tasks.md:200` (T047 `[P]`), `tasks.md:201-202` (T039, T040 `[P]`);
`tasks.md:16-18` (the `[P]` definition); `tasks.md:354-356`.

Problem: the format rule claims of `[P]` tasks that "the write sets are disjoint
and no dependency on an open task is left open". T039 and T040 do depend on T047:
the entry node at `docs/schema/report-v1.json:72` is closed and
`json_load.cpp:1105-1108` refuses a key a closed node does not list, so a writer
or a reader that lands before the schema keys leaves
`ctest -R harness_hunt_json` red. Parallel Opportunities gives the true order
("T047 lands the schema keys first, then T039 and T040 run together"), so the
file again offers two orders, and the `[P]` claim on T047 is the false one. This
is round one's finding 4 relocated to the place finding 1 was fixed.

Fix: drop `[P]` from T047. As an unmarked task standing above T039 and T040 it
carries exactly the intended meaning, which is the pattern T048 and T049 now use
at `tasks.md:253-254` and `tasks.md:359-360`.

### 3. The two V-7 checkpoints disagree with the half-split the headings state

Location: `tasks.md:177` against `tasks.md:205-207`; `tasks.md:250-251` against
`tasks.md:261-262`; `tasks.md:354-356`; `plan.md:472`.

Problem: the headings split the gate: Phase 5 is "gate V-7 (conformance half)"
and Phase 7 is "gates V-9 and the version rows of V-7". Parallel Opportunities
`tasks.md:354-356` confirms where conformance actually lands: "T038's cases go
green with them". But the Phase 5 checkpoint (`:205-207`) never mentions
conformance or the schema keys — it stops at round trip, refusal by pointer and
no `hunt` key — while the Phase 7 checkpoint is labelled `(V-7, V-9)` whole and
opens with "a hunt document conforms at both nodes and a planted out-of-order
hunt key is a finding", an outcome no Phase 7 task produces, against its own
Purpose at `:239-241`. plan.md V-7 (`:472`) states all three clauses as one gate,
so the halves are the tasks file's own construction and its checkpoints are the
only place a reader checks them. An implementer closing Phase 5 has no line item
saying the schema keys are proved; one closing Phase 7 goes looking for a task
that plants a bad hunt key and finds only T038 two phases back.

Fix: add to the Phase 5 checkpoint "and the document conforms at both hunt nodes,
which `harness_json_schema_test` and the planted findings of T038 prove"; retitle
the Phase 7 checkpoint "(V-7 version rows, V-9)" and reduce its first clause to
the version statement plus "the full suite at T054 keeps the Phase 5 conformance
green".

### 4. The P8/V-8 attribution omits the test V-8 names first, and T030 numbers it elsewhere

Location: `tasks.md:96-97`, `tasks.md:116` (T018), `tasks.md:131` (T030),
`tasks.md:158` (T032), `tasks.md:166` (T037); `plan.md:473`.

Problem: plan.md's V-8 row reads "`harness_hunt_cli_test.cpp` and
`harness_hunt_report_test.cpp` pass, `harness_cli_table_test.cpp` reports unique
names, unique values and hunt values inside 270 to 279, and `bash
test/cli_shape.sh` stays clean". `harness_hunt_cli_test.cpp` is filled by T018 and
run by T030, which labels its run "(Principle IX, V-4, V-5)";
`harness_hunt_report_test.cpp` is T032's file, run at T037 under "(V-6)".
Neither T018 nor T032 appears in the carrier list at `:96-97`, so a V-8-to-task
reconciliation still misses one of the four V-8 clauses — finding 3's defect
moved rather than closed, one clause smaller.

Fix: name T018 beside T019, T020, T027, T028, T029 and T030 at `:96`, and add
V-8 to T030's own labels at `:131`. Naming T032 as a V-8 carrier in Phase 4 is
optional: T054's whole-suite run covers that clause, and saying so in one clause
at `:96-97` is enough.

## Severity

Finding 1 ends red on every path and needs one sentence in T053 plus a plan
amendment. Finding 2 is a dispatch ambiguity whose wrong branch leaves a red
intermediate commit. Findings 3 and 4 are auditability of the gate-to-task
mapping, the property `tasks.md:18-19` promises. No `[P]` pair inside one phase
shares a file, task IDs T001 to T064 are complete and unique, the coverage table
rows that name the moved tasks (`FR-052`, `FR-053`, `FR-090`, `FR-091`, `SC-006`)
all point at tasks that exist in the phase they claim, and the file holds no
dangling reference to a task that no longer exists.

VERDICT: FAIL
