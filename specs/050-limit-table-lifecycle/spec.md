# Feature Specification: Lifecycle of the Programmable Limit Table

**Feature Branch**: `050-limit-table-lifecycle`

**Created**: 2026-11-05

**Status**: Draft

**Input**: User description: ticket T-LTL of the roadmap, section 7, the
deferred item H-10 of PR #44. `source/counters/system.cpp` keeps the
programmable-counter limit table in a function-local static. Every `System` in
one process reads that one table, an entry a first `System` learned stays
visible to a second `System`, and nothing clears it. This feature gives the
table a defined lifecycle: it belongs to one `System` and ends with it.
Condensed below; the request is the authority for scope, numbering and the
accepted answers.

## Decisions

The request carries three open questions with recommended answers. Each one is
taken as the decision below, and each names the reading it settles. Two further
decisions record what the base measures, because the request's file list and
its version premise rest on facts the base contradicts. No implementer reopens
the three questions of the request.

- **D-1 Number and branch.** The feature is 050, on the branch
  `050-limit-table-lifecycle`, cut from `origin/master`. Measured on 2026-11-05
  at `origin/master` `858c55d9`: `specs/` holds 001 to 040 and 042 to 047, and
  names no 048, 049 or 050 directory; `git branch -r` names no branch that
  starts `048-`, `049-` or `050-`; `gh pr list --state all` names no pull
  request whose head is such a branch. 041, 048 and 049 stand reserved, so 050
  stands free. A sibling lane can claim a number before this lane pushes, so
  the lane reads the same three checks again at branch time, takes the next
  free number when 050 stands taken, and records the reading in `plan.md`.
- **D-2 Version, schema and the two `SOVERSION` numbers (Q-2).** The rebase onto
  the tree that has taken specs/041-counter-probe FR-040a, FR-053 moved the
  numbers this decision first measured. The base now reads version 0.32.0,
  counters archive `SOVERSION 7` and harness `SOVERSION 21`. This feature takes
  the next free minor, 0.33.0, at the version line `CMakeLists.txt:78`. The
  schema stands unchanged here, because the feature writes no report field. The
  counters archive number goes to 8 (`CMakeLists.txt:168`), because this feature
  changes the public header `include/speedgun-ng/counters_system.hpp`. Hard rule
  12 asks the hand-kept number to move with a public header change. No exported
  layout moves: the table and the tree node live in the private header
  `source/counters/detail/core.hpp`, and `Object` keeps its one member (D-4,
  FR-008). The harness number stays 21 (`CMakeLists.txt:444`), because no
  harness header changes. Two public members of `System` leave its private
  section (D-5), so the version takes a minor step where this decision first
  measured a free patch.
- **D-3 Where the table lives (Q-1).** The table becomes state of `System::Impl`
  (`source/counters/detail/core.hpp:196-256`), beside the provider list, the
  object map, the handle map, the alias map and the open flag. The base kept the
  table behind the anonymous-namespace accessor `programmableLimitTable()` in
  `source/counters/system.cpp`, and that accessor with its function-local static
  goes away with the move.
- **D-4 How a reader reaches the table (Q-1's fallback).** The tree node gains a
  pointer to the `System` that owns it. Measured reason: the reader
  `Object::programmableCounters()` is a `const noexcept` member that holds only
  `void* m_node` (`include/speedgun-ng/counters_system.hpp:189-194`,
  `source/counters/system.cpp:434-443`), and the base `TreeNode` carried no back
  pointer. `struct TreeNode` now carries the `owner` member this decision adds
  (`source/counters/detail/core.hpp:42-50`). The request named a pointer to the
  owning `System::Impl`, and the plan settled the two readings on measurement.
  `Impl` holds no back pointer to its `System`
  (`source/counters/detail/core.hpp:196-256`), while `System::handleFor`, which
  the two walk reads of FR-009 call, is a private member of `System` that a
  member of `Object` may reach through `friend class Object`
  (`include/speedgun-ng/counters_system.hpp:369-372`). One `System` pointer
  answers both shapes of read: the limit read follows `owner->m_impl->limits`,
  and each walk read calls `handleFor`. A pointer to `Impl` answers the limit
  alone.
- **D-5 A second `System` needs a reachable constructor (the one addition to the
  request's file list).** Measured at the base: `System()` and `~System()` stood
  private, and `friend class Object` stood in that same section. Reading 1 is
  the taken reading, so the two members now stand public at
  `include/speedgun-ng/counters_system.hpp:269` and `:278`, and that friend
  stands at `:369`. `System::local()` (`source/counters/system.cpp:194-198`)
  returns the one `System` a process makes, and no other file constructs one,
  measured across `source/` and `test/`. `Object`'s constructor stands private
  too (`include/speedgun-ng/counters_system.hpp:189`), so a test cannot
  assemble a handle by hand either. US-1's acceptance and SC-001 ask for two
  `System` objects in one process, and nothing inside the request's file list
  can make one. Two readings stand recorded:
  - **Reading 1 (taken).** The lane moves `System()` and `~System()` from
    private to public in `include/speedgun-ng/counters_system.hpp`. Both
    symbols already exist inside the exported class, the copy operations stay
    deleted, and the class keeps its one member, so no exported layout moves.
    Hard rule 12 asks a hand-kept number to move with a public header change,
    so the counters archive goes to 8 under D-2. The two interfaces then carry
    doxygen
    the `\pre` and `\post` blocks FR-010 names, and FR-009 keeps
    the two walk reads correct, because a public second `System` makes them
    reachable (see there).
  - **Reading 2 (rejected, recorded).** The file list stays closed, FR-001
    takes its documented fallback, and US-1 keeps a source-shape scan in place
    of a behavior test. That scan can show one function-local static gone; it
    answers nothing about what a second `System` sees, so SC-001 stands unmet.
  The owner rules with one word at plan time. An owner who keeps Reading 1 and
  strikes FR-009 accepts two read paths that answer the local `System`'s tree
  for a handle of another `System`. This spec states Reading 1.
  The rejection rests on measurements taken at the base of this branch.
  Subclassing reaches nothing there, because both members stood private; the
  header names `System::Impl` inside a private section of the public class; and
  `Object(void*)` stays closed, so a hand-built `TreeNode` can carry no handle.
  The one mechanical route redefines the `private` keyword with a macro before
  the include. It compiles, links and runs, and Principle X rejects it as a test
  device, so no test of this lane uses it.
- **D-6 The thread rule (FR-003).** The rule stays as it is and the doc comment
  states it: registration writes the table before its `System` opens; a read
  follows the open of the same `System`; within one `System` no read races a
  write; two `System` objects hold two tables and share nothing, so no lock
  spans two `System` objects. The handle map keeps its own lock
  (`source/counters/detail/core.hpp:201-206`, specs/012 FR-010), the open
  boundary stays the atomic compare-and-exchange it is, and the thread
  sanitizer job stays the arbiter of a race.
- **D-7 Documentation (Q-3).** The counters passage of `docs/pages/harness.md`
  that already names `Object::programmableCounters` (`:1617-1620`) gains two to
  four sentences: the table belongs to one `System`, it ends with that `System`, and
  a second `System` learns its own limits. The `README.md` page and its
  `example/` regions stand unchanged: the main page teaches no rule about the
  lifetime of a catalog fact, and no snippet moves (IV.1 check: no main-page
  impact). The thread words of D-6 live in the header comment, beside the
  interface they govern.
- **D-8 The probe table stays.** `probedKindTable()`
  (`source/counters/system.cpp:173-177`) holds the same shape and the same
  lifetime, and its writer reaches it from another translation unit through
  `detail::noteProbedKinds` (`source/counters/detail/pmu.hpp:286`, called at
  `source/counters/linux_pmu/provider.cpp:839`). The request names the limit
  table, so the probe table stands untouched and the lane records it as
  follow-on work. The resolved metric table holds the same shape of defect, and
  Follow-on work names it beside this one.
- **D-9 Coverage and exclusions (SC-002).** The host bar reads every line of
  each changed file covered, counted from the rows of the host trace. No
  changed file carries a dark branch row beyond the six the base carries in
  `source/counters/system.cpp` and `source/counters/detail/core.hpp`. The
  recorded allowance of 21 rows is the CI measure, and it is tree-wide. The
  in-tree gate compares it against the union of the CI coverage job
  (`tools/dbc/coverage_gate.sh:102-118`), and the pull request body names that
  number. That trace summarizes 450 missing branch rows on the host, a reading
  the lane does not bar. No new `LCOV_EXCL`, no
  `--omit-lines`, no `--ignore-errors` and no `--remove` joins the tree, and the
  marker count stays at or below 84, measured 84 in `source/` and `include/` at
  the base. The allowance is a shortfall budget: an arm a test reaches costs
  nothing, and an arm no test reaches costs one row.
- **D-10 Acceptance and the squashed head (G-20, G-23).** Acceptance runs on
  kennykiller. Measured there at spec time: kernel `7.3.1-1-cachyos`, GCC
  16.2.1. Before the merge the lane squashes into one commit and measures
  acceptance on that head. A CI run on a GitHub runner answers no acceptance.
- **D-11 Overlap.** Other lanes merge to `master` while this lane works, and
  some change counters tests. Before each push the lane rebases on `master`,
  reads both sides of every conflict, and runs every gate again. The lane
  touches no CI file.

## Roadmap position

T-LTL of the roadmap, section 7, the deferred item H-10 of PR #44. PR #44 is
specs/024-multipass-collection, and its contract file
`specs/024-multipass-collection/contracts/programmable-counters-fact.md` states
the fact this feature keeps: what an object publishes, who publishes it, and
what no value means. That file's table of sources and its rule "a source that
cannot answer publishes nothing" stand unchanged; only the owner of the storage
moves.

## Clarifications

### Session 2026-11-05

- Q: Two acceptance cases read a path the reading `System` never declared, so
  `System::object()` answers an error and the case cannot fail before the table
  moves. Which read must those cases perform? (US-1 scenario 3, US-2 scenario 1)
  → A: A limit read needs an object. The rule covers the cross-`System` cases
  where the reading `System` never scripted the path it reads. In each such
  case, that `System` declares the path with one leaf and scripts no limit
  there, and the other `System` scripts the limit. Where the reading `System`
  scripts the path itself, its own scripted answer governs. Measured with a probe
  against the base
  archive: the fresh `System` answers 8 where the rule answers no value, so the
  case fails before the move and passes after it. A case that asserted the
  resolution error of an absent path would measure the path lookup and leave
  FR-005 and FR-006 with no behavior test.
- Q: Does the rule that no catalog fact of a finished `System` stays readable
  hold when two other process-wide tables carry catalog facts? (US-2, D-8)
  → A: It does not hold for those two tables, and the request names the limit
  table, so US-2 now states the rule for limits. Measured: a probe verdict a
  finished `System` wrote through `detail::noteProbedKinds` still answers
  `targets=2` for a later `System`, and `metricHostCurrent()`
  (`source/counters/linux_pmu/metric_formula.cpp:1611-1624`) caches the address
  of `System::local()` once per process. Both sites join Follow-on work. Moving
  the probe table would also move the signature of the hook that writes it, and
  would add changed lines no host-free test can reach, which puts SC-002 at risk
  for a fact the request does not name.
- Q: Does SC-001 carry a behavior case for the two walk reads FR-009 changes?
  (FR-009, SC-001) → A: SC-001 now names a fifth case: a second `System` that
  holds `cpu` and `cpu/core-0` answers `cpu` for `parent()` and one entry for
  `children()`. Measured at the base, both reads answer `nullptr` and `0`,
  because they reach `System::local()`. Without the case the changed lines read
  fully covered through `System::local()` in
  `test/source/counters_objects_test.cpp` while the new rule reads untested,
  which Principle VI forbids.
- Q: One hard gate names a command that answers 2 for want of arguments, so what
  measures the pairing of documentation and enforcement? (SC-003) → A: The lane
  runs `cmake --build build/dev --target dbc-gate`, the target that passes the
  script its four arguments (`cmake/dbc-gate.cmake:40-62`,
  `.github/workflows/ci.yml:1290`). Read at head `22ea70ac`: the bare script
  exits 2, the target in the `ci-ubuntu` tree exits 0 and prints 347 interfaces
  with no gap, and the same target in a tree configured with an interpreter that
  carries no PyYAML exits 2 for that reason.
- Q: What do the hard gates of SC-003 measure on this host, and how many doc-matrix
  rows does the move to public add? (SC-003, FR-010) → A: Read as
  written, two of them measure nothing, because `cmake` reads `-D` before `-P` and
  a flag after `-P` leaves the default tool in force. The host formatter is
  23.1.4 and no speller is installed. That formatter flags 140 files, two of
  them files this lane edits, and none of them a finding this lane introduces.
  The matrix gains one row, `System::~System`. The constructor adds none,
  because the doc gate exempts a member whose name matches a project friend
  name, so its block ships
  because Principle II asks for it. Measured on an out-of-tree copy with both
  members public: 348 interfaces, both gates at 0 gaps.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - One limit table for each System (Priority: P1)

A program builds one `System`, registers a fake event source that answers four
programmable counters for `cpu`, and reads the answer. The same program then
builds a second `System` in the same process, registers a source that answers
two for the same path, and reads again. The second read answers two. It carries
no trace of the four the first `System` learned. A path the second `System`
declares and never scripts answers no value, whatever the first scripted for
it.

**Why this priority**: A limit drives how a metric set splits into passes, so a
stale limit plans passes against a capacity the machine does not have. The
wrong figure reaches a report, which is the failure mode Principle VI rates
worse than no measurement.

**Independent Test**: One test binary builds the two `System` objects in turn
with the fake provider and asserts the four answers above. It needs no kernel
counter, no privilege and no second host.

**Acceptance Scenarios**:

1. **Given** a first `System` whose scripted source answers 4 for `cpu`, **When**
   the program reads `cpu` on that `System`, **Then** the answer is 4.
2. **Given** that `System` and a second `System` whose scripted source answers 2
   for the same path `cpu`, **When** the program reads `cpu` on the second,
   **Then** the answer is 2.
3. **Given** a first `System` that scripted 4 for `cpu`, and a second `System`
   that declares `cpu` with one leaf and scripts no limit, **When** the program
   reads `cpu` on the second, **Then** the answer carries no value.
4. **Given** the same pair, **When** the program reads `cpu` on the first
   `System` again, **Then** the answer stays 4.

---

### User Story 2 - The table ends with its System (Priority: P2)

A maintainer ends a `System`. The limits it learned end with it, and a later
`System` over the same scripted path starts from an empty table. No limit of a
finished `System` stays readable in the process.

**Why this priority**: The property is what keeps the next lane from inheriting
process-wide state. It rates below US-1, because a program that holds one
`System` at a time sees the same answers either way, and the value lands when
the library or a test ends one `System` and starts another.

**Independent Test**: One test binary ends a `System` that scripted a limit,
builds a fresh `System` that declares that path with one leaf and scripts no
limit, and asserts the fresh read carries no value. The same binary asserts a
fresh `System` that scripts its own value answers that value.

**Acceptance Scenarios**:

1. **Given** a `System` that scripted 8 for `cpu`, **When** that `System` ends
   and a fresh `System` declares `cpu` with one leaf and scripts no limit,
   **Then** the fresh read of `cpu` carries no value.
2. **Given** that ended `System`, **When** a fresh `System` scripts 1 for `cpu`,
   **Then** the fresh read answers 1.

---

### Edge Cases

- **A refused provider writes nothing.** Registration stages a whole provider
  and commits it only after every seed validates
  (`source/counters/system.cpp:228-296`). A provider refused for a duplicate
  path, a duplicate name, a taken alias or a bad unit token leaves the table of
  its own `System` untouched, and it touches no other table.
- **The machine root answers nothing.** A clock source consumes no programmable
  counter, so `machine` carries no entry whatever the other seeds of its
  provider script (specs/024 data-model E-02).
- **An object nobody scripted answers nothing.** No entry stands for its path,
  and the read returns no value.
- **Two objects of two `System` objects share a path.** Paths are canonical
  strings, and `cpu` on one `System` is `cpu` on the other. Each read answers
  its own table, which is the point of US-1.
- **A second registration into the same `System`.** The limits of the second
  provider join the table of that `System` and of no other.
- **Two `System` objects open in two threads.** Each table takes its writes
  before its own open and its reads after it, so the two never share a
  mutable object and no lock spans them (D-6).
- **A handle that outlives its `System`.** A handle names a node inside the tree
  of its `System` and stops being valid when that `System` ends. That rule
  stands as it is today; this feature adds no lifetime guarantee and removes
  none.

## Requirements *(mandatory)*

### The table and its owner

- **FR-001**: The programmable-counter limit table shall stand as state of one
  `System`, held in that `System`'s private state `System::Impl`, and the
  anonymous-namespace accessor with its function-local static shall go away with
  the move. When a reader cannot reach `System::Impl`, the change shall state
  the lifecycle in the code and in `docs/pages/`, and shall clear the table when
  the `System` that filled it ends. (Q-1, D-3, D-4.)
- **FR-002**: When a `System` reads the limit of one of its objects, the answer
  shall equal the answer the same object gives at the base: the count its own
  provider published for that canonical path, and no value where nothing
  published one. The single-`System` answer moves nowhere.
- **FR-003**: No new global mutable state shall join the tree. The thread rule
  shall stay as it is, and the doc comment of the interface that reads the table
  shall state it in the words of D-6.
- **FR-004**: When a provider registers, the limit of each object it publishes
  shall enter the table of the `System` that owns that registration call, and
  the table of no other `System`.
- **FR-005**: When a `System` ends, the table that `System` filled shall end with
  it, and no entry of that table shall answer a later read.
- **FR-006**: When an object of one `System` reads a limit and that `System`
  holds no entry for the object's canonical path, the read shall answer no
  value, whatever another `System` holds or held.
- **FR-007**: Each tree node shall name the `System` object that owns it, so a
  read through a handle reaches the table of that `System` alone. (D-4.)

### The reachability the tests need

- **FR-008**: A program shall build a `System` of its own through the library's
  normal interface, so the two stories gain behavior tests that build two
  `System` objects in one process. `System::local()` shall keep its present
  meaning: the shared handle of the process. The copy operations stay deleted.
  (D-5.)
- **FR-009**: When a handle of any `System` reads its parent or its children, the
  answer shall come from the tree of the `System` that owns the handle's node.
  Measured at the base, `Object::parent()` and `Object::children()` reached
  `System::local()` instead (base `858c55d9`, `source/counters/system.cpp`).
  That answer names the wrong tree for any `System` other than the local one.
  FR-008 makes that case reachable, so the lane carries the two reads with the
  owner pointer of FR-007. Each walk read starts at that pointer, reads the
  tree of that `System`, and reaches each returned handle through `handleFor`.
  SC-001 names the case that shows both answers.

### The record

- **FR-010**: The constructor and the destructor FR-008 makes reachable shall
  each carry a doxygen block with `@brief` text and `\pre` and `\post` lines in
  the house form: `none` where no fact stands checkable at that boundary, a
  stated fact with a matching enforcement macro where one stands checkable. The
  gate decides the shape: it reports a documented kind with no macro of that kind
  as `documented-not-enforced`, it accepts `none`, and a macro with no documented
  kind is drift (`tools/dbc/dbc_pair_gate.py:769-786`; the public `ClockProvider()`
  and `PmuProvider()` constructors pass with no macro). `System` keeps its
  present class text and gains the ownership rule in its descriptive text, with
  no `\invariant` line, because a stated invariant needs an `SG_INVARIANT` site.
  Measured on a copy of the header with the two members public: the matrix gains
  one row, `System::~System`, and the constructor gains none, because the doc gate
  exempts a member whose name matches a friend name of the project
  (`tools/dbc/dbc_doc_gate.py:356-362`, `:424-431`; `Object` declares
  `friend class System;` at `include/speedgun-ng/counters_system.hpp:181`). Both
  blocks ship because Principle II asks for them, and the gate reports no gap
  either way.
- **FR-011**: The counters passage of `docs/pages/harness.md` named in D-7 shall
  state the ownership rule of the table in two to four sentences, and `README.md`
  shall stand unchanged.
- **FR-012**: Nine lines across five files quote the version, and all nine shall
  read the minor number D-2 records. The rebase onto the tree that has taken
  specs/041-counter-probe FR-040a, FR-053 moved three of these lines down the
  file, and hard rule 12 is why a hand-kept number moves with a public header
  change. The version line reads 0.33.0 at `CMakeLists.txt:78`, the two
  assertions stand at `test/source/views_test.cpp:316-317` with their comment at
  `:307`, the two assertions stand at
  `test/source/harness_cli_table_test.cpp:1489-1490` with their comment at
  `:1474`, `example/docs/hunt_topdown.expected:2` carries the version, and the
  captured version line of `docs/pages/harness.md` stands at `:2586`. Three
  further lines quote the version for specs/041-counter-probe, at
  `test/source/harness_cli_table_test.cpp:129`,
  `example/docs/counter_probe.expected:2` and `docs/pages/harness.md:1371`. They
  already read 0.33.0, so this lane changes nothing there. A comment that names
  an old number misleads the reader as plainly as an assertion does, so it moves
  with the number. The history line `CMakeLists.txt:61` stays, because it
  records that specs/046 took 0.31.0, and a rewrite would erase that fact. The
  harness archive reads `SOVERSION 21` at `CMakeLists.txt:444`, and the counters
  archive reads `SOVERSION 8` at `CMakeLists.txt:168`.

## Key Entities

- **Limit table**: one `System`'s records that map an object's canonical path to
  the count of programmable counters that object's event source published. One
  record per object that answered, and no record for an object that did not
  answer.
- **Owning `System`**: the `System` a provider registered into. Its private
  state holds the tree, the providers, the handles and, after this feature, the
  limit table.
- **Tree node**: the private record of one countable object, holding kind,
  path, alias, description and leaves (`source/counters/detail/core.hpp:44-48`).
  FR-007 gives it a pointer to the `System` that owns it.
- **Handle (`Object`)**: the public view of one tree node. It holds one `void*`
  member, and that member stays its only member.
- **Open boundary**: the first resolution that closes registration and freezes
  the catalog (specs/007 FR-009). Registration writes stand before it, reads
  stand after it.
- **Scripted limit**: the count a `FakeProvider` publishes for one path through
  `setProgrammableCounters` (`include/speedgun-ng/counters_fake.hpp:163-170`). Every
  test of this feature reads scripted facts.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Five cases stand in the new counters test file, each one function
  with its own name and one `main` call list:

  1. The two `System` objects of US-1 with two different scripted limits on
     one path.
  2. The unscripted path of US-1 scenario 3.
  3. The ended `System` of US-2 scenario 1.
  4. The single-`System` answers of FR-002 for a scripted object, an
     unscripted object and the machine root.
  5. The two walk reads of FR-009 on a second `System` that holds `cpu` and
     `cpu/core-0`.

  Each cross-`System` case declares the path it reads on the `System` that
  reads it, so the read reaches a limit answer and not a path error. Every
  case reads the fake provider only. No case sleeps, reads a clock, names a
  made-up cpu, or needs a kernel counter or a privilege.

  The base is not clean. Read at head `22ea70ac`, the host `ctest` run on
  kennykiller fails on `harness_hunt_cli_test`. The command
  `git diff --name-only 858c55d9..HEAD` named no code file when this spec was
  written, so that head carried the base state. The rebase has since brought
  master's code and this lane's own commits into that range. CI run
  `41946446748`, whose head is `858c55d9`, reports three red jobs.
  `test-rocky` fails on `prose_gate_fixtures`,
  `harness_hunt_cli_test` and `harness_hunt_topdown_test`; `sanitize` fails
  on one entry; `coverage` fails where lcov answers `(inconsistent)` for
  `source/harness/hunt_topdown.cpp:155`. That run skips `docs` and passes
  eleven jobs. The lane fixes none of those failures, because none of them
  reads the counters code.

  The obligation is this. Every `ctest` entry that passes at the base passes
  at the head on kennykiller, both directly and under `taskset -c 0,1`, and
  in CI. No CI job that passes at the base turns red. The owner pushes the
  branch, opens the pull request against `master`, and reads the run that
  request starts. A branch push starts no run, because
  `.github/workflows/ci.yml:7-14` triggers `push` on `master` only and
  `pull_request` against `master`. The head, its sha, its run, and that run's
  verdict stand on the pull request, where `gh pr view` answers them. SC-005
  says the pull request body holds that record, and no committed file holds it.
- **SC-002**: The figures come from the host trace
  `build/coverage/coverage.info`, because the CI coverage job answers no trace
  at the base (SC-001). Two measures stand apart, and each keeps its own
  obligation. The host bar is per file: 100 percent of the lines of each
  changed file, read from the rows of that trace. No changed file carries a
  dark branch row beyond the six the base carries in
  `source/counters/system.cpp` and `source/counters/detail/core.hpp`. The
  allowance of 21 rows is the CI measure, and it is tree-wide. The in-tree gate
  compares it against the union of the CI coverage job
  (`tools/dbc/coverage_gate.sh:102-118`), and the pull request body names that
  number. That same trace summarizes 450 missing branch rows on the host, a
  reading the lane does not bar. The `LCOV_EXCL` marker count stays at or below
  84, and no new exclusion construct appears: no `LCOV_EXCL`, no
  `--omit-lines`, no `--ignore-errors`, no `--remove`. That bar governs every
  command that stands in the tree: a CMake file, a script, a preset, a gate.
  Reading the base trace on the host needs `--ignore-errors inconsistent` on a
  wrapper outside the repository, `/tmp/lcov-soft`. That read names three
  inconsistent sites and counts them: `source/harness/runner.cpp:708`,
  `source/harness/runner.cpp:821` and `source/harness/hunt_topdown.cpp:155`,
  then `inconsistent: 3`. This lane touches none of them. The CI reading stays
  as SC-001 records it: run `41946446748` names only
  `source/harness/hunt_topdown.cpp:155`.
- **SC-003**: These commands exit 0 on the squashed head:
  `cmake -D FORMAT_COMMAND=<clang-format 18.1.8> -P cmake/lint.cmake`,
  `cmake -D SPELL_COMMAND=<codespell> -P cmake/spell.cmake`,
  `python3 tools/docs/example_drift.py .`,
  `python3 tools/prose/prose_gate.py --base origin/master` and
  `cmake --build build/dev --target dbc-gate`. `cmake` reads
  `-D` before `-P`: the same flag after `-P` reaches the script and leaves the
  default tool in force, and the default is what the CI job names
  (`.github/workflows/ci.yml:37`). kennykiller carries `clang-format` 23.1.4 and no
  speller, so a run with the defaults stops at the missing tool. That run
  flags 140 files, two of them files this lane edits, and none of them a
  finding this lane introduces. The bare pair-gate script takes four arguments
  and answers 2 without them, so the lane runs it through its target, and that
  target needs a `Python3_EXECUTABLE` that carries PyYAML, which is what
  `plan.md` configures `build/dev` with.
- **SC-004**: The release presets build with no new analyzer finding:
  `cmake --preset=ci-rocky` with `cmake --build build`, and once per feature
  `cmake -S . -B build/dev --preset=ci-ubuntu` with `cmake --build build/dev` for
  clang-tidy and cppcheck, so the `ci-rocky` tree of the two `ctest` runs stays
  in place (Principle VIII, Principle IX).
- **SC-005**: The pull request body records the acceptance of G-20 on
  kennykiller: the host, the kernel, the GCC version, the head sha, the full
  `ctest` count, the `taskset -c 0,1` count, and the coverage figures of
  SC-002. The body also names the base failures SC-001 states, each with the
  reading that shows it, and it names CI run `41946446748`. The run that the
  pull request starts carries the CI reading SC-001 states. After that run ends,
  the body names its number, its verdict, and the sha of the head the request
  carries. `gh pr view` answers that verdict and that sha, and `gh pr edit`
  writes them into the body. No commit can carry a reading taken after it, so
  the body holds them and the lane amends no commit after that run. An amend
  plus `git push --force-with-lease` would move the head, cancel the run in
  progress (`.github/workflows/ci.yml:3-5`), and start a run no step reads.

## Constraints

- ASD-STE100 Simplified Technical English stands over every generated word,
  with the constitution's Principle XI.
- Each test stands in the same commit as the code it tests. No code shaped for
  coverage alone.
- Commit author and committer: `OmO <omo@speedgun-ng.local>`. Titles run 50
  characters or fewer, body lines 72 or fewer, and the footer's last two lines
  read `Refs: specs/050-limit-table-lifecycle T-LTL` and `Approved-by: OmO`.
- Rebase only. No merge commit. No CI change. The lane never merges.
- The first push is normal. A later push runs
  `git push --force-with-lease=050-limit-table-lifecycle:<sha last pushed>`, and
  `git ls-remote origin 050-limit-table-lifecycle` then equals
  `git rev-parse HEAD`. A failed lease stops the lane and reports.
- Before the merge the lane squashes into one commit and measures acceptance on
  that head (owner ruling G-23).
- Local acceptance (G-20) runs on kennykiller. A CI run on a GitHub runner
  answers no acceptance.
- No new hard runtime dependency. No new global mutable state (FR-003). No new
  lock, no new atomic, no changed thread rule (D-6).

## Scope

In scope:

- `source/counters/system.cpp`: the table's storage, its one write, its one
  limit read, and the two walk reads of FR-009.
- `source/counters/detail/core.hpp`: `System::Impl`, `TreeNode`, and any private
  header the move needs.
- `include/speedgun-ng/counters_system.hpp`: the two access changes of FR-008
  and the doxygen blocks of FR-010, the addition D-5 records.
- `test/source/`: the new counters lifecycle test file and any new function in
  an existing counters test.
- `test/CMakeLists.txt`: the CTest entry of the new file, entries only.
- `docs/pages/harness.md`: the two to four sentences of D-7.
- The version line and the eight other lines FR-012 measures, the capture pair
  of `example/docs/hunt_topdown.expected` among them.
- `specs/050-limit-table-lifecycle/` and `.specify/feature.json`.

Out of scope:

- `probedKindTable()` and the per-kind verdicts the availability probe records (D-8).
- The pass planner and every harness reader of the limit fact. The harness reads
  the fact through `Object::programmableCounters()` and nowhere else, and that
  surface keeps its signature, its meaning and its answers.
- The JSON report schema, the writer, the loader and the comparison.
- The harness `SOVERSION` number, the contract of specs/024
  `contracts/programmable-counters-fact.md`, and every CI file. The counters
  archive number moves under D-2 and FR-012, because this feature changes the
  public header.
- `Object::parent()` and `Object::children()` beyond the two reads of FR-009,
  and `Object::counters()` beyond the limit read of FR-002.

## Preconditions

Measured at `origin/master` `858c55d9` on 2026-11-05, the base the request
names. The rebase to head `fba814d2` moved code files, so each number below
names the head line that now carries that reading. A reading that names code
this lane removed keeps the base `858c55d9` in place of a line number, because
no head line carries it.
- The base table was a function-local static behind the anonymous-namespace
  accessor `programmableLimitTable()` (base `858c55d9`,
  `source/counters/system.cpp`). Its comment stated that the table fills while a
  provider registers and reads only after the catalog opens, so no read races a
  write.
- One write: `source/counters/system.cpp:293`, inside
  `System::registerProvider`, after the whole provider validates.
- One limit read: `source/counters/system.cpp:434-443`, in
  `Object::programmableCounters()`, through `void* m_node`.
- The base node held no limit field and no back pointer (`struct TreeNode` in
  `source/counters/detail/core.hpp`).
- The private state holds the tree, the providers, the handles, the alias map,
  the handle lock and the open flag: `source/counters/detail/core.hpp:196-256`.
- `System()` and `~System()` stood private at the base, and
  `friend class Object` stands in that section of
  `include/speedgun-ng/counters_system.hpp` at `:369`. `Object`'s constructor
  stands private at `:189`. `System::local()` builds the process's only `System`
  at `source/counters/system.cpp:194-198`.
- The limit fact arrives with specs/024: `44443baa Counters: Publish the
  programmable counter limit`, whose contract file is
  `specs/024-multipass-collection/contracts/programmable-counters-fact.md`.
- The counters limit test reads one `System`:
  `test/source/counters_objects_limit_test.cpp`, entered at
  `test/CMakeLists.txt:712-722`. Every counters test reaches the catalog through
  `System::local()`; no test builds a `System`.
- The coverage allowance reads `allowance=21` at `tools/dbc/coverage_gate.sh:102`,
  and the `LCOV_EXCL` marker count reads 84 across `source/` and `include/`.
- The lane host kennykiller runs kernel `7.3.1-1-cachyos` with GCC 16.2.1.

## Assumptions

- The lane host is kennykiller, the machine the request names, and this session
  measures it there. The work reads no host fact, so no second-vendor run is
  needed.
- The owner's three recommended answers stand as D-2, D-3 and D-7.
- Reading 1 of D-5 stands taken. The rule stands recorded there with the
  measurement that forces it, and the owner can strike it at plan time with one
  word.
- A `System` that a program builds behaves like `System::local()` for the rules
  this feature governs: the same registration rule, the same open boundary, the
  same tree of objects, the same handle map. Two catalog facts stay process-wide
  and answer the local `System` for every `System`: the probe verdict table of
  D-8, and the resolved metric table, which caches the address of
  `System::local()` once per process (`:1619` in
  `source/counters/linux_pmu/metric_formula.cpp`). Follow-on work names both.
  The feature tests the limit table, and FR-009 keeps the two walk reads on
  the tree of the `System` that owns the handle.
- The harness keeps reaching `System::local()`, so every pass-planning lane
  behaves as it does today. Measured: `source/harness/` names
  `System::local()` at `runner.cpp:58`, `:963`, `:1307`, `cli.cpp:666`,
  `catalog.cpp:90`, `hunt.cpp:192`, `hunt_cli.cpp:650`, `:724` and
  `jitter_probe.cpp:94`, `:141`.

## Dependencies

- Specs/024 FR-005 and FR-006, the published fact and the fake's scripted answer,
  with its contract file.
- Specs/007 FR-009, the open boundary that closes registration, and specs/012
  FR-010, the lock of the handle map that D-6 names. No requirement owns the
  rule that a refused provider writes nothing. The comment at
  `source/counters/system.cpp:209-210` states it, and the block at
  `source/counters/system.cpp:285-296` carries it: the staged batch lands only
  after every seed validates.
- The coverage allowance of 21 rows, read from the ledger the gate carries at
  `tools/dbc/coverage_gate.sh:61-64` and `:102`: 19 rows recorded by specs/030
  D-14 and two rows recorded by specs/031-host-validation.
- Principle II, IV.1, VI and VIII, and the owner rulings G-20 and G-23.

## Citations

- `source/counters/system.cpp` held the accessor and its static at the base
  `858c55d9`. The write stands at `:293`, the limit read at `:434-443`, the
  probe table at `:173-177`, its two `detail` hooks at `:533-543`, the two walk
  reads at `:445` and `:503`, and the process handle at `:194-198`.
- `source/counters/detail/core.hpp:42-50` `TreeNode`, `:196-256` `System::Impl`.
- `include/speedgun-ng/counters_system.hpp:75-93` the limit interface and its
  contract, `:189-194` the handle's one member, `:253-286` the `System` class
  head and `local()`, `:368-384` the private section.
- `include/speedgun-ng/counters_fake.hpp:163-170` `setProgrammableCounters`.
- `test/source/counters_objects_limit_test.cpp` the current four cases,
  `test/CMakeLists.txt:712-722` its entry.
- `docs/pages/harness.md:1608-1620` the counters passage that names the fact,
  `:2586` the captured version line.
- `CMakeLists.txt:78` version, `:168` counters archive `SOVERSION`, `:444`
  harness `SOVERSION`.
- `tools/dbc/coverage_gate.sh:102` the allowance,
  `tools/dbc/dbc_pair_gate.py:769-786` the pairing rule,
  `cmake/dbc-gate.cmake:40-62` the `dbc-gate` target that passes the script its
  four arguments, `.github/workflows/ci.yml:1290` the CI invocation.
- `specs/024-multipass-collection/contracts/programmable-counters-fact.md` the
  fact's contract, `specs/047-loader-cross-field/spec.md` the house shape this
  artifact follows.

## Follow-on work

- At the base, `Object::parent()` and `Object::children()` reached
  `System::local()` (base `858c55d9`, `source/counters/system.cpp`). FR-009
  lands the two reads this feature makes reachable; any wider use of the owner
  pointer of FR-007 belongs to its own change.
- `probedKindTable()` (`source/counters/system.cpp:173-177`) holds the same
  lifetime defect, and its writer sits in another translation unit
  (`source/counters/linux_pmu/provider.cpp:839`), so its fix needs the seam that
  hook already is. The lane records it and changes nothing (D-8). Measured: a
  verdict one `System` wrote for `cpu/cycles` answered `targets=2` for a later
  `System` after the first ended.
- The resolved metric table names one `System` for the whole process.
  `metricHostCurrent()` caches `&System::local()` once
  (`source/counters/linux_pmu/metric_formula.cpp:1611-1624`), and the rows
  resolved against it are cached by directory alone (`:1497-1516`);
  `source/counters/linux_pmu/topdown_select.cpp:186-189` binds the same handle
  per call. A metric a finished `System` resolved keeps answering that `System`.
  The request names the limit table, so the lane changes nothing there.

## Open questions

None. The three questions of the request stand taken as D-2, D-3 and D-7. D-5
stands recorded as the one point where the base contradicts the request's file
list, with both readings and the measurement that chooses between them, and the
owner rules it at plan time.

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes no rule the root
  `README.md` teaches and moves no `example/` region, so the page stands
  unchanged. The counters passage of `docs/pages/harness.md` carries the new
  words (D-7).
- Principle II: the limit interface keeps its `\pre` and `\post`, the two
  newly reachable interfaces gain theirs under FR-010, and the pair gate reports
  no drift (SC-003).
- Principle III: the user stories carry EARS requirements, and the plan carries
  the logical and physical views with the test plan.
- Principle VI: every changed line ships with its test in the same commit
  (SC-001). Coverage reads 100 percent of the lines of each changed file,
  counted from the rows of the host trace. No changed file carries a dark
  branch row beyond the six the base carries. The pull request body names the
  tree-wide 21-row allowance of the CI coverage job (SC-002).
- Principle VIII: the gates of SC-003 and SC-004 stand, no gate configuration
  moves, and the thread sanitizer job stays the arbiter of a race (D-6).
- Principle X.1: D-5, D-8 and FR-009 record each reading the base admits, the
  reading taken, and the reason.
