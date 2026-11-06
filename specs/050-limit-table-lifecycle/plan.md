# Implementation Plan: Lifecycle of the Programmable Limit Table

**Branch**: `050-limit-table-lifecycle` | **Date**: 2026-11-05 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/050-limit-table-lifecycle/spec.md`

## Summary

`source/counters/system.cpp` keeps the programmable-counter limit table in a
function-local static, so one process shares one table: a limit a first `System`
learned stays readable to the next, and nothing clears it. This lane moves the
table into `System::Impl`, gives every tree node the address of the `System` that
owns it, and reads the table and the two walk reads through that owner. It moves
`System()` and `~System()` from private to public, because nothing else in the
tree can build a second `System` in one test binary, and the two stories ask for
one. Five cases in a new test file carry the behavior, and each one was measured
against the base archive first: four answer wrong today.

The version takes the minor 0.33.0 at `CMakeLists.txt:78`, and the counters
archive takes `SOVERSION 8` at `CMakeLists.txt:168`. The rebase onto the tree
that has taken specs/041-counter-probe FR-040a, FR-053 is what moved those two
numbers, and hard rule 12 asks a hand-kept number to follow a public header
change. The harness number stays 21 at `CMakeLists.txt:444`, because no harness
header moves and no exported layout moves. The table and the owner pointer sit
in the private header, and `Object` keeps its one member. Two sibling tables
keep their defect and stand recorded as follow-on work (D-8).

## Technical Context

**Language/Version**: C++23 (`cxx_std_23`), the tree's own standard.

**Primary Dependencies**: none added. The writer and the readers sit in
`source/counters/system.cpp`; the storage and the node live in
`source/counters/detail/core.hpp`; the public surface is
`include/speedgun-ng/counters_system.hpp`. The fake provider
(`include/speedgun-ng/counters_fake.hpp`) scripts every fact the tests read.

**Storage**: process memory only. The table moves from a function-local static to
one member of `System::Impl`. No file, no format, no schema change.

**Testing**: frameworkless C++ programs with the `check()` and `fail()`
convention, one executable per file, driven by CTest. One new file,
`test/source/counters_limit_lifecycle_test.cpp`, with its own CTest entry beside
the existing `counters_objects_limit_test` entry
(`test/CMakeLists.txt:712-722`).
The existing limit file stays the single-`System` regression.

**Target Platform**: Linux on x86-64, the two release presets `ci-rocky` and
`ci-ubuntu`. The lane host is kennykiller (Intel Core Ultra 7 265K, 20 cpus,
kernel `7.3.1-1-cachyos`, GCC 16.2.1). macOS and Windows stay unsupported.

**Project Type**: two shared libraries with one CLI front end. This change touches
the counters archive alone, and the harness reaches nothing new.

**Performance Goals**: none priced. The limit read runs at catalog resolution
time, outside every timed window and outside the sample loop; the change replaces
one static lookup with one pointer hop and the same map lookup.

**Constraints**: no new global mutable state (FR-003); no new option, no new
public type, no new error type; no CI change (spec.md, Constraints); the
harness `SOVERSION` number stands and the counters number moves to 8 (D-2);
the host bar reads 100 percent of the
lines of each changed file from the rows of the host trace, and no changed file
carries a dark branch row beyond the 6 rows the base carries; the allowance of
21 rows is the tree-wide measure of the CI coverage job, and that job answers no
trace at the base; no new exclusion construct and at most 84 `LCOV_EXCL` markers
(D-9); no test sleeps, reads a clock, names a made-up cpu, or needs a kernel
counter or a privilege (SC-001).

**Scale/Scope**: three source files, one new test file, one CTest entry, one docs
page with its capture pair, and the five record files. Wave 1 opens the set with
`results/base-state.md`, and wave 5 writes the other four:
`results/sanitizers.md`, `results/coverage.md`, `results/acceptance.md` and
`results/pr-body.md`. Read at head `22ea70ac`: 181 CTest entries, 84
`LCOV_EXCL` markers, 347 gated interfaces. The rebase onto the tree that has
taken specs/041-counter-probe FR-040a and FR-053, and this lane's own commits,
put code files inside `git diff --name-only 858c55d9..HEAD` at head
`fba814d2`.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate | Verdict | Basis |
| --- | --- | --- |
| I, standard first, P0 to P3 | pass | P0 correctness of the limit answer drives the lane; no P2 convenience joins it |
| II, Design By Contract | pass | no new predicate stands checkable at the two moved boundaries, so each carries `\pre none` and `\post none`, the accepted house form; the pair gate reports no gap |
| III, R-DCUT | pass | every requirement names its case in the FR coverage map, and each test lands in the same commit as the code it tests |
| IV, documentation | pass | the counters passage of `docs/pages/harness.md` gains the ownership rule; `README.md` states no rule about a catalog lifetime, so it stands unchanged |
| V, style | pass | clang-format 18.1.8 with `--style=file`, the pinned pair at SC-003 |
| VI, coverage | pass | the Coverage plan names the case that reaches every changed line and every arm |
| VII, performance discipline | pass | no timed path changes; the change adds one pointer hop on a resolution-time read |
| VIII, hard CI gates | pass | the Gate commands run the five gates of SC-003, both preset builds, the two sanitizer runs and the coverage gate on the squashed head. The record names every verdict of that block, the analyzer reading of the checked build among them |
| IX, spec-driven development | pass | spec, clarifications, plan, tasks and converge all stand in this directory |
| X, anti-slop | pass | no new type, no new seam, no generalization: the table moves, the pointer names the owner, and the access level moves; two sibling tables stay untouched |
| XI, prose | pass | `prose_gate.py --check all` runs over the range and over each commit message |

No gate fails, so Complexity Tracking stays empty.

## Project Structure

### Documentation (this feature)

```text
specs/050-limit-table-lifecycle/
├── spec.md                                   # requirements, clarifications
├── checklists/requirements.md                # quality checklist
├── plan.md                                   # this file
├── research.md                               # Phase 0: measured decisions
├── data-model.md                             # Phase 1: the table and its owner
├── contracts/limit-table-ownership.md        # Phase 1: the shipped rule
├── quickstart.md                             # Phase 1: the two-System read
├── tasks.md                                  # Phase 2 (/speckit.tasks)
└── results/                                  # implement: the five record files
    ├── base-state.md                         # the base readings
    ├── sanitizers.md                         # the two sanitizer runs
    ├── coverage.md                           # the coverage reading
    ├── acceptance.md                         # the verdicts and the counts
    └── pr-body.md                            # the pull request body
```

### Source (this feature)

```text
source/counters/detail/core.hpp          TreeNode gains `System* owner`;
                                         System::Impl gains `limits`
source/counters/system.cpp               the static accessor goes away; two node
                                         creations name their owner; the commit
                                         block writes the owning table; the limit
                                         read and the two walk reads follow the
                                         owner
include/speedgun-ng/counters_system.hpp  System() and ~System() move to public
                                         with their doxygen blocks, and the class
                                         text states the ownership rule
test/source/counters_limit_lifecycle_test.cpp   new: the five cases
test/CMakeLists.txt                      new target and CTest entry
docs/pages/harness.md                    the counters passage gains the rule;
                                         the captured version line moves
CMakeLists.txt                           version 0.33.0, counters SOVERSION 8
test/source/views_test.cpp               the version reading
test/source/harness_cli_table_test.cpp   the version reading
example/docs/hunt_topdown.expected       the captured version line
```

**Structure Decision**: no new directory, no new target beyond one test
executable. `source/counters/system.cpp` is a single-writer file for the whole
lane: the storage, the write, the three reads and the two node creations all land
there, so one worker holds that file at a time.

## Design

### Decision index

| Decision | Landing point |
| --- | --- |
| D-1 number, branch | this directory, `050-limit-table-lifecycle` |
| D-2 0.33.0, counters `SOVERSION` moves, harness kept | the version commit, nine lines |
| D-3 the table joins `System::Impl` | `source/counters/detail/core.hpp`, `System::Impl::limits` |
| D-4 the node names its owner | `TreeNode::owner`, set at `system.cpp:188` and `:281` |
| D-5 the two members move to public | `include/speedgun-ng/counters_system.hpp` |
| D-6 the thread rule, stated | the doc comment of `Object::programmableCounters` |
| D-7 the counters passage | `docs/pages/harness.md`, the multi-pass paragraph |
| D-8 the two sibling tables | Follow-on work, no code |
| D-9 coverage, no exclusion | the Coverage plan |
| D-10 acceptance on the squashed head | `results/`, the Gate commands |
| D-11 overlap with other lanes | the rebase step of the Gate commands |

Two readings of D-4 stand resolved here. The node stores `System*`, the address of
the owning `System`, and not a pointer to `System::Impl`: the two walk reads of
FR-009 reach `System::handleFor`, which a member of `Object` may call because
`friend class Object` stands at `counters_system.hpp:369`, and the same pointer
reaches `owner->m_impl->limits` for the limit read. One field answers both, and the
shortest route the spec names is the one that also carries FR-009.

### Modules and dependency order

Seven modules. Each one names the files it owns, and one file has one writer in
each wave.

| Module | Files | Needs |
| --- | --- | --- |
| M-1 storage | `source/counters/detail/core.hpp` | spec D-3, D-4 |
| M-2 write and reads | `source/counters/system.cpp` | M-1 |
| M-3 surface | `include/speedgun-ng/counters_system.hpp` | spec D-5 |
| M-4 behavior test | `test/source/counters_limit_lifecycle_test.cpp`, `test/CMakeLists.txt` | spec SC-001, M-3 to build |
| M-5 counters page | `docs/pages/harness.md` | spec D-7 |
| M-6 version record | `CMakeLists.txt`, `test/source/views_test.cpp`, `test/source/harness_cli_table_test.cpp`, `example/docs/hunt_topdown.expected`, `docs/pages/harness.md` | spec FR-012, M-5 for the shared page |
| M-7 record | `specs/050-limit-table-lifecycle/results/`: the five records `base-state.md`,
`sanitizers.md`, `coverage.md`, `acceptance.md` and `pr-body.md`, plus
`specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md`, which
commit 3 carries when the T012 reading rewords that page, a case that makes
six paths in that commit. | the five gate verdicts of SC-003, the checked build and its analyzer reading, the two sanitizer runs and the coverage reading |

M-5 and M-6 both touch `docs/pages/harness.md`, in different waves. M-5 writes
the counters passage in the authoring wave. M-6 moves the captured version line
at `:2586` in the version wave, so the authoring wave stays file-disjoint.

The lane lands three commits, in this order, each atomic and bisectable:

1. `Counters`: M-1, M-2, M-3, M-4 and the counters passage of M-5 in one commit.
   The test stands in the same commit as the code it tests (Principle III), and a
   docs page must not state a rule the code does not enforce.
2. `CMake`: M-6, the version move to 0.33.0 and the counters number to 8, with
   the nine lines that quote the version and the rebuild and re-run that follow.
   `cmake --preset=ci-rocky && cmake --build build`, because the version stamp
   reaches a test binary through a `PRIVATE` define of the harness target. The
   precedent commit `a69455b6` records that reconfigure, the whole-library
   rebuild and the relink of every harness test. The history line
   `CMakeLists.txt:61` stays, because it records the feature that took 0.31.0,
   beside the lines at `:30-63` that keep the older records.
3. `Docs`: M-7, the third commit, and it carries the five `results/` files:
   `base-state.md`, `sanitizers.md`, `coverage.md`, `acceptance.md` and
   `pr-body.md`. The acceptance record names `results/sanitizers.md` and
   `results/coverage.md`, which carry the two sanitizer readings and the
   coverage reading of the set. The verdict set reads: the five gate verdicts
   of SC-003, the checked build and its analyzer reading, the two sanitizer
   runs and the coverage reading. Commit 3 also carries the contract file
   `specs/050-limit-table-lifecycle/contracts/limit-table-ownership.md` when
   the T012 reading rewords that page, and that case makes six files in the
   commit. The pull request body stands drafted before this commit, and the
   commit lands before the squash.

Parallel blocks. `tasks.md` owns the execution order, and its wave numbers are
the numbers this plan uses. Its phase headings group work by user story, while
its wave table fixes the order workers run in. Wave 2 authors three files that
no other wave 2 task opens, so three workers author at once and none of them
builds:

- Wave 1: one worker records the base state, then stands up the two pinned
  tools.
- Wave 2, in parallel: the test file and the CTest entry of M-4, and the
  counters passage of M-5. No author builds, runs `ctest`, or runs a gate.
- Between wave 2 and wave 3, the red build: one worker builds and records the
  compiler line for `test/source/counters_limit_lifecycle_test.cpp`, which does
  not build at the base, because `System()` stands private. That is the red of
  the reachability. The red of the behavior stands measured against the base
  archive already, one process per case. The four figures stand in the Test plan
  below.
- Wave 3, in parallel: the private-header writer M-1 runs beside the
  public-header writer M-3. `tasks.md` marks both `[P]`, and neither task builds,
  so the wave runs two writers at once.
  Then M-2, the sole writer of `source/counters/system.cpp`, who lands the
  storage move and the two walk reads there. Wave 3 also carries the M-4
  writer of `test/source/counters_limit_lifecycle_test.cpp`, and one writer
  holds that file at a time. T003 writes three cases in wave 2; then T014 adds
  the walk case once T013 lands; then T011 adds the ended-`System` case.
- The wave gate closes wave 3: build once, run the new test by name, run the
  suite, and run the gates. All five cases of the new test file stand written
  before the wave gate builds and runs them. That gate is the first build of
  wave 3 and the first run of the suite, so no authoring task runs a compiler.
- Wave 4: commit 1 lands the wave 3 files, the new test with its five cases
  among them. Then M-6 lands as commit 2, and the rebuild
  `cmake --preset=ci-rocky && cmake --build build` precedes the version tests and
  the drift gate run.
- Wave 5: every gate on the head, the two sanitizer runs and the coverage
  reading, then M-7 as commit 3.
- Wave 5 closes with the squash to one commit, the last step that makes a
  commit. The acceptance re-measure on that head (D-10) starts with
  `cmake --preset=ci-rocky && cmake --build build`, and `/speckit.converge`
  runs at the end. Three writers still work after the squash. T025 adds its
  re-measure reading, T026 adds its rebase re-record, and T027 adds its
  converge reading. D-10 and the owner ruling G-23 keep the squashed head at one
  commit, so each of those three amends with `git commit --amend --no-edit`.
  Each reading names the sha it stood at, because a commit holds no reading
  taken after that commit exists. T028 writes nothing inside the tree, because
  its reading is the CI verdict of the head the owner pushed. That run starts
  after the push, so an amend would move the head and cancel the run whose
  verdict it records. That run tests the merge commit that the `pull_request`
  event checks out, and that merge commit has its own sha. The squashed head has
  a separate sha, which `gh pr view` answers as `headRefOid`. Both the verdict
  and that sha stand on the pull request, where T028 reads the sha and `SC-005`
  records it. The rebase of D-11 makes the difference harmless, because the
  branch head merges as it stands. An amend after a reading moves the head, and
  `cancel-in-progress` at `.github/workflows/ci.yml:3-5` then cancels the run of
  the head it superseded.
  The owner pushes after the last of those amends, so every record file stands
  inside the one commit. After that push the owner opens the branch as a pull
  request against `master`, because `.github/workflows/ci.yml:7-14` triggers
  `push` on `master` alone. That `pull_request` event starts the run the lane
  reads, and no push to a feature branch starts any run.

A worker that holds `source/counters/system.cpp` holds it alone, and no other
worker opens that file. That writer lands the storage move and the two walk
reads before the wave gate, so no half move reads the wrong table. The gate sees
the whole file, and both edits ride commit 1 with the rest of wave 3.

### Logical view: one table, one owner

The write path. `System::registerProvider` stages a provider
(`system.cpp:228-296`), validates every seed, and commits. The commit block
writes two things per object it
seeds: the node into `m_impl->objects`, with `owner` set to `this`, and the limit a
source answered into `m_impl->limits`, keyed by the canonical path. A refused
provider writes nothing, because the commit block never runs (Edge case, and the
rule at `:221-294`).

The read path. `Object::programmableCounters()` casts `m_node` to `TreeNode*`,
follows `node->owner`, and looks the path up in `owner->m_impl->limits`. A hit
answers the count, a miss answers no value. Today the same lookup reaches the one
process-wide table, and that is the whole defect.

The walk path. `Object::parent()` and `Object::children()` take `node->owner` in
place of `System::local()`, then walk that system’s handle map and object map as
they do now. `System::local()` keeps its meaning: the shared handle of the process.

The order rule stays: registration writes before the open of that `System`, a read
follows the open of the same `System`, and two `System` objects share no mutable
object (D-6).

### Physical view

`source/counters/detail/core.hpp`. `TreeNode` gains one member, `System* owner`,
with one comment line stating that it names the `System` whose tree holds the node.
`System::Impl` gains `std::map<std::string, std::size_t> limits`, with the comment
that carried the old accessor: one record per object whose source answered, no
record for the rest, filled during registration and read after the open.

`source/counters/system.cpp`. At the base this file held the anonymous-namespace
accessor at lines 187-191 under the comment block at lines 179-186; both go away
with the move, and that comment moves to the member. The two
`std::make_unique<TreeNode>()` sites at `:184` and `:264` set `owner` at `:188`
and `:281`. The commit block writes `m_impl->limits` at `:293`. Three reads
change their route and nothing else: `programmableCounters()` at `:434-443`,
`parent()` at `:445-460`, and `children()` at `:503-528`.

`include/speedgun-ng/counters_system.hpp`. `System()` and `~System()` move from the
private section to the public one, each with a doxygen block: `@brief` text,
`\pre none`, `\post none`. The class block gains the ownership rule in its
descriptive text. `friend class Object` stays where it is, `~System()` stays
declared here and defined out of line as `= default`, and the copy operations stay
deleted. The doc matrix gains one row, `System::~System`, and the gated
interface count reads 347 then 348. The doc gate exempts a member whose name
matches a friend name (`tools/dbc/dbc_doc_gate.py:356-362`, `:424-431`), so
`System()` gains no row (`include/speedgun-ng/counters_system.hpp:181`).

### Public API surface added

| Surface | Before | After |
| --- | --- | --- |
| `System::System()` | private | public, documented, no enforcement macro |
| `System::~System()` | private | public, documented, no enforcement macro |
| `System::local()` | public | unchanged in meaning and spelling |
| `Object` members, layout, `System` members | unchanged | unchanged, one member each |
| exported archive layout | `SOVERSION 7` | `SOVERSION 8`; the number moves, the layout does not |

### Risks

- **Two trees, one process.** A program that builds its own `System` and also uses
  the harness sees two catalogs, because the harness reaches `System::local()` at
  ten measured sites and registers there. This lane changes no harness call site;
  the rule that a handle belongs to the `System` that made it is what keeps the two
  trees from answering for each other.
- **A handle outlives its `System`.** The documented rule stands: a handle names a
  node inside its `System` and stops being valid when that `System` ends. The lane
  adds no lifetime guard and removes none.
- **`m_impl` is a `unique_ptr` to an incomplete type in the header.** The owner
  pointer names `System`, and `TreeNode` is a private type, so the private header
  needs no new include and no new forward declaration.
- **Analyzer findings.** The analyzer reports one finding once per translation
  unit that reaches the file, so one added test unit lifts a count with no new
  finding. The bar is the count of distinct findings per edited file, beside the
  unit count of that run. One finding counts as one file, line, column and check
  name: two units that answer those four alike are one finding, and reading the
  message text too splits one position in two. The header gives the example:
  `include/speedgun-ng/counters_system.hpp` carries 5 distinct findings over 110
  units, which reads 550. The raw tree total is a base reading only, and the lane
  does not ask the head to match it. The record carries each base reading of the
  checked build twice: as the raw count, and as the distinct count beside the
  unit count that produced it. The raw figures read 22 in
  `source/counters/system.cpp`, 76 in `source/counters/detail/core.hpp` and 550
  in `include/speedgun-ng/counters_system.hpp`.
  The tree total reads 51287 warning lines over 303 distinct path strings, and
  `grep -c ': warning:'` over `/tmp/050-dev-build.log` answers that figure.
  Those 303 path strings name 273 files, because one file can stand under two
  spellings. `source/counters/detail/core.hpp` shows it: its own path carries 76
  of those lines, and `source/counters/linux_pmu/../detail/core.hpp` carries 19.
  Cite that log for the total, and cite the per-file table
  `/tmp/050-base-analyzer-findings.json` for a per-file figure. The gate
  rules on no *new* finding, so the lane compares the distinct counts of the
  run.
- **The host suite is not clean at the base.** Read at head `22ea70ac`, which
  differs from `858c55d9` by spec artifacts alone. `harness_hunt_cli_test` fails
  on this host in both the full run and the `taskset -c 0,1` run.
  `counters_pmu_test`, `counters_overhead` and `harness_topdown_run_test` report
  Skipped. The record names all four. The obligation: every `ctest` entry that
  passes at the base passes at the head on this host. The lane fixes none of
  these entries, because none of them reads the counters code.
- **The ignore-semantics build.** `speedgun-ng_CONTRACTS=ignore` compiles every
  contract away, so a local that only a contract reads becomes an unused-variable
  error in the `consumer-release` job. The lane adds no contract and no such local.
- **Three CI jobs stand red at the base.** CI run 41946446748 on `858c55d9`
  fails `test-rocky` on three entries. One is `prose_gate_fixtures`, on a
  vocabulary probe and a `git ls-files` ownership fault, and the others are
  `harness_hunt_cli_test` and `harness_hunt_topdown_test`. The `sanitize` job
  fails one entry, and the `tsan` job passes. D-6 keeps the thread sanitizer
  job the arbiter of a race, so a regression on either sanitizer run stands
  recorded in `results/sanitizers.md`. The `coverage` job fails where `lcov`
  answers `(inconsistent)` for `source/harness/hunt_topdown.cpp:155`. Eleven
  jobs pass: `dbc-gate`, `consumer-release` and the three static-analysis
  shards among them. The `docs` job stands skipped. The obligation: no CI job
  that passes at the base turns red at the head. The lane fixes none of these
  three jobs, because none of them reads the counters code.
- **Overlapping lanes.** Other lanes change counters tests on `master`. Before each
  push the lane rebases, reads both sides of every conflict, and runs every gate
  again.

## Test Plan

### FR coverage map

| Requirement | Evidence |
| --- | --- |
| FR-001 table in `System::Impl`, accessor gone | case 1 and case 3 exercise the member; T006's `grep` answers 1 for the `limits` member, and T007's `grep` answers 0 for `programmableLimitTable` in `source/counters/system.cpp` |
| FR-002 single-`System` answers unchanged | `counters_objects_limit_test` (four cases, unchanged) plus case 4 on a built `System` |
| FR-003 no new global state, thread rule stated | the diff removes one static and adds one member; the `grep` in the task that owns the header answers that the thread sentence stands in the comment, and the `dbc-gate` run answers that no doc gap appears |
| FR-004 registration writes its own table | case 1: two systems, one path, answers 4 and 2 |
| FR-005 the table ends with its `System` | case 3: an ended system leaves no entry, answered 8 at the base |
| FR-006 no entry answers no value | case 2: a declared, unscripted path answers nothing, answered 4 at the base |
| FR-007 the node names its owner | every case reads through a node; both creation sites stand covered |
| FR-008 a program builds its own `System` | each case builds its own: case 1 two, case 2 two, the ended case three, the single-`System` case one, the walk case one |
| FR-009 the walk reads follow the owner | case 5: `parent()` answers `cpu` and `children()` one entry, answered `nullptr` and 0 at the base |
| FR-010 the two doc blocks, no drift | `cmake --build build/dev --target dbc-gate` exits 0 |
| FR-011 the counters page, `README.md` unchanged | the page lines ride commit 1; the prose gate and the drift gate run |
| FR-012 version and the nine lines that quote it | `views_test` and `harness_cli_table_test` read the version; the drift gate reads the capture; the record lists the nine moved lines and the history line `CMakeLists.txt:61` that stays |

### Coverage plan

Each changed line and each arm gets a case:

| Changed code | Case | Arms |
| --- | --- | --- |
| `System::System()` body, `root->owner` | every case | no branch |
| `System::~System()` | every case | no branch |
| `node->owner` in `registerProvider` | every case | no branch |
| `m_impl->limits.insert_or_assign` | case 1 | no branch |
| limit lookup hit and miss | case 1, case 2 | both arms |
| `parent()` on the owner | case 5 | hit and the no-parent arm, read on the `machine` root |
| `children()` on the owner | case 5 | one-entry arm and the empty arm, read on a leaf |
| the D-6 thread sentence in the `Object::programmableCounters` comment (`include/speedgun-ng/counters_system.hpp:83-87`) | T008, the task that owns that header | no branch: a `grep` names the sentence and `dbc-gate` exits 0 |

The last row carries the second half of FR-003. A doc comment holds no measured
line, so the row names the writing task and the two checks.

No new arm stands unreachable, so the lane reads its branch figures against
the base, and that base reading has three measures. The bar on the host is the
per-file measure of the table below. No dark branch row appears there beyond
the 6 rows the base carries. Those 6 stand in the two files that carry dark
rows: 3 in `source/counters/system.cpp` and 3 in
`source/counters/detail/core.hpp`. The coverage record and the pull request
body both name that number. The second measure is the host tree figure of 450
dark rows, the 94.9 percent of 8829 in the table below. The lane records that
figure as a reading it does not bar. The third measure is the allowance of 21
rows, the tree-wide measure of the CI coverage job, read at
`tools/dbc/coverage_gate.sh:102-118`. That job answers no trace at the base, so
no step reads that measure. `coverage_gate.sh` prints no branch figure for the
host trace, because it exits 1 at `tools/dbc/coverage_gate.sh:45-48`, before
its branch section runs. The host figures therefore come from the trace rows
of the table below. No exclusion construct joins the tree, and the marker
count stays 84 (D-9).

The base figures, measured at `858c55d9` on this host before any change:

| Measure | Base reading |
| --- | --- |
| `source/counters/system.cpp` | 282 of 282 lines, 137 of 140 branches |
| `source/counters/detail/core.hpp` | 79 of 79 lines, 31 of 34 branches |
| `include/speedgun-ng/counters_system.hpp` | 20 of 20 lines, 6 of 6 branch rows |
| tree, `lcov --summary` | 97.5 percent of 17786 lines, 94.9 percent of 8829 branches |

The public header carries two inline definitions: `Object::counter` at
`include/speedgun-ng/counters_system.hpp:143-178` and `Object::Object` at
`include/speedgun-ng/counters_system.hpp:189-190`. Those definitions give the
header its 20 measured lines at the base. The two members this lane moves,
`System()` at `include/speedgun-ng/counters_system.hpp:269` and `~System()` at
`:278`, are declarations only. Their definitions stand at
`source/counters/system.cpp:181` and `source/counters/system.cpp:192`, so the
move adds no measured line.
Two conditions on the base decide how the head reading gets reported. The tree
line figure reads below 100 percent on this host, because three entries stop
early here. That figure is the summary of the whole trace, and that summary is
what `coverage_gate.sh` bars with its line rule. The bar this lane holds is
per-file: 100 percent of the lines of each changed file, read from the rows of
the trace. The sizing note at `tools/dbc/coverage_gate.sh:86-101` says plainly
that a host run reports the host. Its allowance counts the union of the CI
coverage job, where every entry runs to its end. That job answers no trace at
the base, so SC-002 reads its figures from the host trace
`build/coverage/coverage.info`. The refusal carries no cause in this lane. Read
through `/tmp/lcov-soft`, `lcov` answers `(inconsistent)` at three sites and
reports `inconsistent: 3`: `source/harness/runner.cpp:708`,
`source/harness/runner.cpp:821` and `source/harness/hunt_topdown.cpp:155`. At
each site a function is hit and no line inside it is hit. A bare `lcov` run
names one of those three, and the site it names differs between runs. The same
refusal stands in the CI `coverage` job of run 41946446748, and that record
names `source/harness/hunt_topdown.cpp:155` alone. The lane changes no line of
those three files, records the refusal, and reads its own figures from the
per-file table above. That table gives a shortfall of 6 branch rows, 3 in each
of the two files that carry dark rows. The third changed file carries no dark
branch row and reads 20 of 20 lines.

SC-002 rules a new `--ignore-errors` flag out of every command that stands in
the tree: a CMake file, a script, a preset, a gate. Reading that broken trace
needs the flag, so the host reads the trace through a wrapper outside the
repository, `/tmp/lcov-soft`. `coverage_gate.sh` names its tool in the `LCOV`
variable, so the gate run takes the same wrapper. The capture command at
`cmake/coverage.cmake:69` carries the flag at the base, so it stands outside the
bar. No file of the tree gains such a flag.

### Gate commands

```bash
cmake --preset=ci-rocky && cmake --build build
ctest --test-dir build -j 20 --output-on-failure
taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure
cmake -D FORMAT_COMMAND=<pinned-clang-format-18.1.8> -P cmake/lint.cmake
cmake -D SPELL_COMMAND=<pinned-codespell> -P cmake/spell.cmake
python3 tools/docs/example_drift.py .
python3 tools/prose/prose_gate.py --check all --mode range --base origin/master
cmake -S . -B build/dev --preset=ci-ubuntu -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build/dev
cmake --build build/dev --target dbc-gate
cmake --preset=ci-sanitize && cmake --build build/sanitize -j "$(nproc)"
ctest --test-dir build/sanitize -j 1 --output-on-failure --no-tests=error
cmake --preset=ci-tsan && cmake --build build/tsan -j "$(nproc)"
ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error
export PERL5LIB="$HOME/perl5/lib/perl5"
cmake --preset=ci-coverage
cmake --build build/coverage -j "$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage --target coverage
bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
```

The pinned tools: clang-format 18.1.8 and codespell, the same versions CI installs
by pip (`.github/workflows/ci.yml:33-34`). This host carries clang-format 23.1.4 in
`/usr/bin` and no `codespell`, so the lane keeps both pins in a tool virtual
environment outside the repository and points `FORMAT_COMMAND` and `SPELL_COMMAND`
at them. A run of the format gate with the unpinned host formatter reports
140 files, a count exact for that formatter. Two of those 140 are files this
lane edits, and none of them is a finding this lane introduces. `PERL5LIB`
carries `Capture::Tiny` for the host
`lcov` on a host that keeps the module outside the default path; on this host
`perl -MCapture::Tiny` resolves from `/usr/share/perl5/vendor_perl` and the export
answers nothing. `Python3_EXECUTABLE` names an interpreter that carries PyYAML,
because the `dbc-gate` target stops without it. The checked build lands in `build/dev`, so
the `ci-rocky` tree of the two `ctest` runs stays in place.

The two sanitizer runs of the block above fill `results/sanitizers.md`, and the
coverage run fills `results/coverage.md`. The readings of the gates and the
checked build fill `results/acceptance.md`, which names the two records above.

Before each push: rebase on `master`, read both sides of every conflict, and run
every command above again on the rebased head (D-11).

## Constitution Check: design re-check

Passed. The design adds no public type, no header, no dependency, no option and no
CI change. It removes one function-local static and adds two private members, one
public access level for two existing members, one test file and one CTest entry.
Principle II is satisfied by `\pre none` and `\post none`, the form the gate
accepts and the two existing public provider constructors use. Principle X is
satisfied by the two sibling tables left alone. Principle III is satisfied by the
FR coverage map, which leaves no requirement without a case.

## Complexity Tracking

No gate fails, so this table stays empty.
