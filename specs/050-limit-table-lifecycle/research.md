# Research: Lifecycle of the Programmable Limit Table

Phase 0 output of `/speckit.plan` for `specs/050-limit-table-lifecycle/spec.md`.
Each entry names the decision it records, the reading that decides it, and the
losing alternative. Every cite reads the working tree, read at head `1b5d8b70`
of branch `050-limit-table-lifecycle` (version 0.33.0, report schema 1.15
(`source/harness/json_report.cpp:50`), counters archive `SOVERSION 8`, harness
`SOVERSION 21`). The rebase onto the tree that has taken specs/041-counter-probe
moved code lines, so each number below names the head line that carries that
reading today. A reading that names code this lane deleted keeps the base
`858c55d9` in place of a line number. The spec, the plan and four evidence notes
read the same tree: `/tmp/050-design-notes.md`,
`/tmp/050-clarify-round1.md`, `/tmp/050-clarify-round2.md` and
`/tmp/050-dbc-notes.md`. A figure taken from one keeps its number and gains that
path. A bare `:NNN` continues the file above it. The lane host is kennykiller,
an Intel Core Ultra 7 265K with 20 cpus, kernel `7.3.1-1-cachyos`, GCC 16.2.1.

Numbering: the entries here read `R-1` to `R-11`. Entry `R-n` records decision
`D-n` of the specification, cited as `spec D-5`. The two spellings name
different files.

## R-1: the number, and the three checks that keep it free

- **Decision**: The feature is 050 on `050-limit-table-lifecycle`, cut from
  `origin/master` (spec D-1). Three checks decide it, and the lane runs all three
  again at branch time, so a sibling that claims 050 first costs one re-read.
- **Rationale**: Re-read on 2026-11-05 for this file. `ls specs/` lists 001 to
  040 and 042 to 047 beside this lane's own directory, and names no 041, 048 or
  049. `git branch -r` names no ref that starts `048-`, `049-` or `050-`.
  `gh pr list --state all` answers 79 heads, and none of them names 041, 048, 049
  or 050 (`/tmp/050-clarify-round2.md:166`). 041 stands reserved for the N6-P
  planning lane, which changes how the counter limit is found
  (`specs/043-fwq-ftq/spec.md:30-32`, `specs/042-topdown/spec.md:493-495`); spec
  D-1 records 048 and 049 as reserved the same way.
  `.specify/feature.json` names `specs/050-limit-table-lifecycle`, and
  `git rev-parse origin/master` still reads `858c55d9`. Both read again at head
  `22ea70ac`.
- **Alternatives considered**: 048 or 049, both reserved. The next free number at
  branch time, which spec D-1 states for a lane that finds 050 taken and records
  in `plan.md`.

## R-2: the version step, its nine lines, and the rebuild that follows

- **Decision**: `0.32.0` moves to `0.33.0` at nine lines across five files, the
  counters `SOVERSION` line moves from 7 to 8, and the harness number and the
  schema stand (spec D-2, FR-012, hard rule 12).
- **Rationale**: The five sites read `CMakeLists.txt:78`,
  `test/source/views_test.cpp:316-317`,
  `test/source/harness_cli_table_test.cpp:1489-1490`,
  `example/docs/hunt_topdown.expected:2` and the captured version line at
  `docs/pages/harness.md:2586`. The rebase onto the tree that has taken
  specs/041-counter-probe FR-040a, FR-053 moved three of those five lines down
  their files. Re-run at this head, the scan is one command:
  `grep -rIn --exclude-dir='build*' --exclude-dir=.git '0\.33\.0' .`. The
  shipped set holds `CMakeLists.txt`, `test`, `example` and `docs`. There the
  scan answers thirteen lines across six files, and FR-012 names nine of them.
  Seven of those nine carry the value, and two are comments that quote the same
  digits at `test/source/views_test.cpp:307` and
  `test/source/harness_cli_table_test.cpp:1474`. The other four lines are one
  comment at `CMakeLists.txt:71` that records this lane's own step, and three
  lines that specs/041-counter-probe wrote for its own release:
  `test/source/harness_cli_table_test.cpp:129`,
  `example/docs/counter_probe.expected:2` and `docs/pages/harness.md:1371`.
  Each quotes the project version, so the step of this lane moves it too: the
  first read 0.30.1 there, and the other two read 0.32.0. The
  history lines at `CMakeLists.txt:60-61` close the specs/046 record and keep
  the number that release took, beside the records at `:30-63` where each
  keeps the number of its own release. The precedent patch move `a69455b6`
  (`CMake: Move the version to 0.30.1`) touches one `VERSION` line, and it
  leaves every history line alone, because each records the release that took a
  number. Outside the shipped set `0.31.0` survives in the artifacts of
  specs/046, specs/045 and specs/041, and in one vendored lock line at
  `external/yaml-cpp/MODULE.bazel.lock:150`. The number `0.33.0` names no file
  outside this lane's own artifacts and the four lines above, so the minor
  stands free. The two archive numbers read `CMakeLists.txt:168` and
  `CMakeLists.txt:444`, and each version test compares the pair in file order
  (`test/source/views_test.cpp:322`,
  `test/source/harness_cli_table_test.cpp:1495`), so each number holds three
  checks in each file, six between the two. `SG_PROJECT_VERSION` is a `PRIVATE`
  compile definition of the harness target (`CMakeLists.txt:357-361`), so the
  move rebuilds that target whole and relinks every harness test. The banner
  line the capture pair quotes prints at `source/harness/report.cpp:492`. A
  minor fits the change, because hard rule 12 asks a hand-kept number to move
  with a public header change and two members of `System` leave its private
  section. `Object` keeps its one member `void* m_node`
  (`counters_system.hpp:194`), `System` keeps its one member
  `std::unique_ptr<Impl> m_impl` (`:383`), and the two new members land in the
  private header (`source/counters/detail/core.hpp:42-50`, `:196-256`), which
  stands outside both exported classes
  (`include/speedgun-ng/counters_system.hpp:36`, `:253`). The comment beside
  the counters `SOVERSION` line states that same rule for that number
  (`CMakeLists.txt:166-167`), and the harness number last moved when its
  exported layouts moved (`:441-443`).
- **Alternatives considered**: A patch step on the number the base carries.
  Spec D-2 first measured a free patch, and the rebase that took
  specs/041-counter-probe moved that number, so the next free value is the
  minor `0.33.0`. A kept `0.32.0` leaves the build printing one number while
  `CMakeLists.txt:78` states another, and `test/source/views_test.cpp:316`
  reads that file, so the suite reports the gap.

## R-3: where the table lives, and what the move deletes

- **Decision**: The table becomes one member of `System::Impl` beside the five it
  already holds, and the anonymous-namespace accessor with its function-local
  static goes away (spec D-3, FR-001).
- **Rationale**: The base kept the accessor `programmableLimitTable()` at lines
  187-191 of `source/counters/system.cpp`, under its comment at lines 179-186,
  inside an anonymous namespace that closed at line 193. The lane deletes that
block. The writer stands at `source/counters/system.cpp:293` in the commit
  block of `System::registerProvider`, and the reader stands at `:436`, inside
  `Object::programmableCounters()`. `System::Impl`
  (`source/counters/detail/core.hpp:196-256`) holds the provider list at
  `:198`, the object map at `:199`, the handle map at `:200`, the alias map at
  `:207` and the open flag at `:220`, so the member joins a set the file
  already reads and no new type joins the tree. `Impl` is a private nested type
  (`include/speedgun-ng/counters_system.hpp:382`) defined in the private
  header, so no exported layout moves. The harness number keeps 21, and the
  counters number moves with the public header under hard rule 12.
- **Alternatives considered**: Keep the accessor and clear it at the open
  boundary. Two `System` objects then still share one map, the pair answers 2 and
  2 (`/tmp/050-clarify-round1.md:60-62`), and FR-006 fails. One process-wide map
  keyed by `System`: one more indirection serving one caller, which Principle X.2
  prohibits (`.specify/memory/constitution.md:862-869`).

## R-4: what the node stores, and why the pointer names a `System`

- **Decision**: `TreeNode` gains one member, the `System` that owns the node, set
  at both creation sites: the machine root at `source/counters/system.cpp:188`
  and the registered object at `:281` (spec D-4, FR-007).
- **Rationale**: The limit reader `Object::programmableCounters()` at
  `:434-443` is a `const noexcept` member that holds only `void* m_node`
  (`include/speedgun-ng/counters_system.hpp:189-194`), and the base `TreeNode`
  carried kind, path, alias, description and leaves alone (`struct TreeNode` in
  `source/counters/detail/core.hpp`). The two walk reads need a `System`, and a
  table pointer answers neither of them. At the base, `Object::parent()` bound
  `System::local()`, looked the parent path up in that system's object map and
  called the private `System::handleFor`; `Object::children()` bound the same
  handle, walked `m_impl->objects` and called `handleFor` again (base
  `858c55d9`, `source/counters/system.cpp`). At head, both read the owner
  instead, at `source/counters/system.cpp:451` and `:506`.
  (`include/speedgun-ng/counters_system.hpp:371-372`) and `friend class Object`
stands at `:369`, so an `Object` member may call it
  through the owner. One member reaches the table
  (`owner->m_impl->limits`) and both walks, which is the shortest route that also
  carries FR-009 (plan.md, Decision index).
- **Alternatives considered**: A pointer to `System::Impl` reaches `limits` and
  reaches no `handleFor`, so FR-009 needs a second field. A direct table pointer
  leaves both walks on the local tree. No field leaves FR-007 unmet and leaves the
  two walk reads wrong for every handle of a built `System`.

## R-5: the reachability of a second `System`, and the seven routes that fail

- **Decision**: `System()` and `~System()` move from the private section to the
  public one, each with its doxygen block. FR-010 bars an `\invariant` line for
  `System`, so no such line joins the two blocks. The doc gate requires an
  invariant of a class only where that class documents one
  (`tools/dbc/dbc_doc_gate.py:464-476`). Reading 1 of spec D-5 stands taken, and
  the copy operations stay deleted (FR-008,
  `include/speedgun-ng/counters_system.hpp:256-257`).
- **Rationale**: Round 1 tried seven routes against the base and recorded what
  each one answered (`/tmp/050-clarify-round1.md:21-53`).

  | Route | What the measurement said |
  | --- | --- |
  | direct construction | `'sg::counters::System::System()' is private within this context` at the base, where the pair stood in the private section of `include/speedgun-ng/counters_system.hpp` (base `858c55d9`) |
  | a subclass of `System` | the implicit pair is deleted, because the base pair stood inaccessible (base `858c55d9`) |
  | a subclass of `Object` | `'sg::counters::Object::Object(void*)' is private within this context` at `include/speedgun-ng/counters_system.hpp:189` |
  | the private header through a test include | `'struct sg::counters::System::Impl' is private within this context` at `source/counters/detail/core.hpp:196`, because `struct Impl;` sits in the private section at `include/speedgun-ng/counters_system.hpp:382`; the include pattern stands precedented (`test/source/counters_metric_trees_test.cpp:37-38`, `test/CMakeLists.txt:697-699`), and a filled `TreeNode` still carries no handle |
  | an exported factory | `local()` at `include/speedgun-ng/counters_system.hpp:286` builds the process's one `System`, and `compile(const System&, ...)` at `counters_measurement.hpp:1099` and `:1115` takes one and makes none |
  | a second copy of the library | the test binary links `build/libspeedgun-ng.a`, and its own translation unit still cannot name the constructor |
  | `fork()` | two processes, where US-1's Independent Test asks for one binary |

  The macro route (`#define private public` before the include) compiles, links
  and runs (`/tmp/050-clarify-round1.md:55-58`). The standard bars replacing a
  keyword by macro expansion, so the device has no defined behavior, and spec D-5
  records it as the route Principle X rejects. Four base answers carry the change:
  with both objects alive one path answers 2 and 2
  (`/tmp/050-clarify-round1.md:60-62`); an unscripted path on the reading `System`
  answers 4 (`/tmp/050-clarify-round2.md:99`); after the `System` that scripted 8
  ends, a fresh one answers 8 (`/tmp/050-clarify-round2.md:100`); the two walk
  reads answer `nullptr` and 0 (`/tmp/050-clarify-round2.md:102`). The fifth case
  answers 4, no value and no value at the base, and FR-002 requires that it keeps
  doing so (`/tmp/050-clarify-round2.md:101`).
- **Alternatives considered**: Reading 2 of spec D-5 keeps the file list closed and
  gives US-1 a source-shape scan. That scan shows one static gone, answers
  nothing about a second `System`, and leaves SC-001 unmet. A subclass and a
  hand-built handle both fail on the two access lines above.

## R-6: the thread rule, and the one place its words may stand

- **Decision**: The rule stays as it stands, and the thread words go into the
  descriptive text above the contract lines of `Object::programmableCounters`
  (spec D-6, FR-003).
- **Rationale**: Three facts hold the rule. The handle map keeps its own lock
  with its comment at `source/counters/detail/core.hpp:201-206`, and that lock
  covers the lookup and the insert as one step inside `handleFor`
  (`source/counters/system.cpp:351-368`). The open boundary is one atomic
  (`source/counters/detail/core.hpp:220`) whose store runs through
  `compare_exchange_strong` at `:228-229`, so the false-to-true step happens
  once at any thread count. Two `System` objects hold two tables and share no
  mutable object, so no lock spans them. The wording rule comes from the pair
  gate: the interface stands at `include/speedgun-ng/counters_system.hpp:75-93`
  with `\pre none` and `\post none` at `:89-90`, and a documented kind that
  carries text needs a macro of that kind in the body, which the gate names
  `documented-not-enforced` (`tools/dbc/dbc_pair_gate.py:769-786`). The two
  `none` lines therefore stand, and the thread words sit above them. The thread
  sanitizer job stays the arbiter of a race. Its job stands at
  `.github/workflows/ci.yml:230`, its preset at `:282` and its run at
  `:296-300`.
- **Alternatives considered**: A lock over the table, which FR-003 bars and
  spec D-6 bars. A `\pre` line that states the open boundary, which costs one
  row the gate names and needs a fuse on a read whose miss returns no value
  (`source/counters/system.cpp:439-441`); FR-010 takes `none` where nothing
  stands checkable.

## R-7: the counters passage, and the main page that stays put

- **Decision**: Two to four sentences join the counters passage of
  `docs/pages/harness.md`, and `README.md` stands unchanged (spec D-7, FR-011).
- **Rationale**: The passage opens at `docs/pages/harness.md:1608` under the
  heading `Multi-pass collection`, and the paragraph at `:1617-1620` already
  names `Object::programmableCounters` as one of the two facts that drive the
  split, so the ownership rule joins the text that owns the fact. The drift
  gate's user-page classes forbid a commit hash, a spec identifier, a
  requirement identifier, a task identifier, a constitution principle number
  and a contract label (`tools/docs/example_drift.py:491-503`), the page list
  names `docs/pages/harness.md` (`:483`), and the scan strips fenced blocks and
  inline code spans first (`:449-463`), so a name in backticks is safe and a
  bare spec directory name is a hit. The main-page check reads every limit site
  of the root page: the method paragraph at `README.md:327`, the disclosure
  sentence at `:349` and the quick-reference row at `:875`. None states who
  owns a limit or when it
  ends, and the section's one snippet is the marked region
  `example/docs/metric_passes.cpp#metric-passes` at `README.md:328`, a file this
  lane edits nowhere, so the IV.1 check answers no main-page impact.
- **Alternatives considered**: A main-page section. IV.1 asks for one when
  user-visible behavior moves, and it names the marked regions as the pair the
  drift gate compares (`.specify/memory/constitution.md:579-598`). A sentence
  in the passage that cites a requirement identifier, which the class reports.

## R-8: the two sibling tables, and the readings that keep them out

- **Decision**: `probedKindTable()` and the resolved metric table stay as they
  stand, and Follow-on work names both (spec D-8).
- **Rationale**: The probe table is a function-local static of the same shape
  at `source/counters/system.cpp:173-177`, with no clear. Its writer stands in
  another translation unit: `detail::noteProbedKinds`, declared at
  `source/counters/detail/pmu.hpp:286` and called at
  `source/counters/linux_pmu/provider.cpp:839`. The read runs through
  `detail::probedKindsAt` at `source/counters/system.cpp:479` into the decision
  at `:545-571`. A probe wrote one verdict while one `System` lived, and a
  later `System` that wrote nothing answered `targets=2 avail=0`, the same
  figure (`/tmp/050-clarify-round2.md:114-115`; the cpu bit reads 2 at
  `include/speedgun-ng/counters_core.hpp:113`). The second site names one
  `System` for the whole process: `metricHostCurrent()` caches
  `&System::local()` in a `static const` at
  `source/counters/linux_pmu/metric_formula.cpp:1616`, the assignment at
  `:1619`, inside `:1611-1624`; `resolvedMetricTable()` caches rows in a
  `static std::map` at `:1503` and looks them up by the directory alone at
  `:1506` (`:1497-1516`); and
  `source/counters/linux_pmu/topdown_select.cpp:186-189` binds the same handle
  per call at `:187`.
- **Alternatives considered**: Move the probe table in this change. Its hook takes
  no owner today, so the signature moves with it and the change gains a second
  file the request does not name (`/tmp/050-clarify-round1.md:118`). Ship a case
  that shows the surviving verdict as a defect, and the tree carries a test that
  proves a broken rule (`/tmp/050-clarify-round1.md:119`).

## R-9: the two coverage constants, both re-measured

- **Decision**: The changed lines read 100 percent lines, and the `LCOV_EXCL`
  marker count stays at or below 84 (spec D-9, SC-002). Three branch measures
  stand apart here, and only one of them bars on this host. The branch shortfall
  that bars here is the per-file reading of the changed sources. No changed file
  may gain a dark branch row beyond the 6 the base leaves, three in each of two
  files. The 21 rows are the tree-wide allowance of the CI coverage job, the
  measure `tools/dbc/coverage_gate.sh:102-118` bars against. The 450 rows are
  the recorded host tree reading, and no gate in this lane bars against that
  figure.
- **Rationale**: `allowance=21` stands at `tools/dbc/coverage_gate.sh:102`, and
  the gate compares the measured shortfall against it at `:114`. It prints the
  figure at `:115` when the shortfall fits and at `:119` when it exceeds the
  allowance. The comment at `:86-92` states that the allowance counts the union
  of the CI coverage job, where every entry runs to its end. A developer host
  reads higher, because an entry there can stop early and leave the arms past
  its stop dark (`:90-92`). An arm a test reaches costs nothing, and an arm no
  test reaches costs one row. The trace comes from the `coverage` target, whose
  summary command names that script and that file
  (`cmake/coverage.cmake:113-117`), and the extract step drops every `SG_*`
  line before the rate (`cmake/coverage.cmake:101`). The gate exits 1 at
  `:45-48` when a trace reads below 100 percent lines, so it prints no branch
  figure for a host trace. This host's `build/coverage/coverage.info` reads 97.5
  percent lines, so every host figure here comes from that file. Its summary
  pair reads 8379 of 8829 branches, a tree shortfall of 450 rows at the base
  (`/tmp/050-base-cov-report.log:338-340`). That tree reading bars nothing on
  this host, because the gate stops on the line check first. The per-file
  reading is the one that bars here, and it names the two files with dark rows.
  `source/counters/system.cpp` reads 137 of 140 branch rows, and
  `source/counters/detail/core.hpp` reads 31 of 34
  (`/tmp/050-base-cov-target-files.json`). Each holds 3 dark rows, so the two
  together hold the 6 the bar counts. The public header
  `include/speedgun-ng/counters_system.hpp` carries its own entry: 20 of 20
  lines and 6 of 6 branch rows, and no dark row of its own. The count re-reads
  84 markers in 13 files across `source/` and `include/`: 27 `LCOV_EXCL_LINE`,
  19 `LCOV_EXCL_BR_LINE`, 13 `LCOV_EXCL_BR_START`, 13 `LCOV_EXCL_BR_STOP`, 6
  `LCOV_EXCL_START` and 6 `LCOV_EXCL_STOP`. Two files this lane edits carry
  markers already, `source/counters/system.cpp` with 12 and
  `source/counters/detail/core.hpp` with 2. The two blocks in
  `source/counters/system.cpp` at `:510-513` and `:537-540` cover the return
  epilogues of `counters()` and `children()`. Every line the lane changes in that file stands outside those
  two blocks. The lane deletes the accessor block at `:179-191`, writes
  `root->owner` after `:201` and `node->owner` after `:293`, and edits `:305`,
  `:449`, `:463` and `:518`.
- **Alternatives considered**: A marker on a changed line, which SC-002 bars
  as a new construct and spec D-9 bars by the count. A case that reaches no
  new arm spends rows of the tree-wide allowance of 21. The two changed files
  that hold dark rows carry 3 each at the base, 6 together; the plan's Coverage
  plan expects that figure to stay.

## R-10: the host, the base suite, and the two tools the lane pins

- **Decision**: Acceptance runs on kennykiller on the squashed head, and the base
  state of that host stands recorded beside it (spec D-10, SC-005).
- **Rationale**: Re-read on this host: the kernel reads `7.3.1-1-cachyos`,
  `g++ --version` reads `16.2.1 20260810`, `nproc` reads 20, and `/proc/cpuinfo`
  names an Intel Core Ultra 7 265K. `ctest --test-dir build -N` counts 181
  entries. The measured base run reads 180 of 181 in 94.3 s, and the run under
  `taskset -c 0,1` reads 180 of 181 in 367.4 s
  (`/tmp/050-design-notes.md:111-116`). The one failure is
  `harness_hunt_cli_test`, and its two assertion lines read
  `test/source/harness_hunt_cli_test.cpp:3134` and
  `test/source/harness_hunt_cli_test.cpp:3683`; they stand as the base state
  and no run of this lane caused them. Three entries skip through
  `SKIP_RETURN_CODE 2`: `counters_pmu_test` (`test/CMakeLists.txt:469-470`),
  `counters_overhead` (`:675-676`) and `harness_topdown_run_test`
  (`:2359-2360`).
  The pinned pair stands at `/tmp/050-tools-venv/bin/clang-format` (18.1.8) and
  `/tmp/050-tools-venv/bin/codespell` (2.4.3), the versions CI installs by pip
  (`.github/workflows/ci.yml:33-34`), and the two gates take them through
  `FORMAT_COMMAND` and `SPELL_COMMAND`; `/usr/bin/clang-format` answers 23.1.4 and
  no `codespell` stands on `PATH`. The base analyzer run reads 51287 warning
  lines over 303 paths as the log spells them, and those paths name 273 files.
  `grep -c ': warning:'` answers that total, and it belongs to the base run
  alone (`/tmp/050-design-notes.md:117-120`). The per-path table
  `/tmp/050-base-analyzer-findings.json` holds one row for each of those 303
  strings, and its rows sum to the same 51287 lines. 13 files stand under more
  than one spelling, and `source/counters/detail/core.hpp` is one of them. That
  file is one of the three this lane edits: its own path carries 76 warning
  lines and `source/counters/linux_pmu/../detail/core.hpp` carries 19.
  clang-tidy reports a finding once per
  translation unit that reaches it, so a file figure is a distinct count times a
  unit count. `source/counters/system.cpp` answers 22 findings in its one unit,
  and `source/counters/detail/core.hpp` answers 19 in each of 4 units.
  `include/speedgun-ng/counters_system.hpp` answers 5 in each of 110 units, and
  those readings give the 22, the 76 and the 550 of the note. The new limit test
  reaches that header through `include/speedgun-ng/counters.hpp:21`, so one more
  unit lifts a file figure with no new finding. One finding counts as one file,
  line, column and check name, and reading the message text as well splits a
  position in two: that header answers 5 positions and 6 whole-line rows, because
  one position carries two message texts, 109 units of one and 1 of the other. A
  comparison against these figures therefore reads distinct positions per file
  beside the unit count, which is what SC-004 asks: no new finding.
- **Alternatives considered**: A GitHub runner for acceptance. Spec D-10 states
  that such a run answers no acceptance, and the owner ruling G-20 names this
  host. The host formatter, which reports findings in dozens of files this lane
  touches nowhere, and SC-003 names the pins.

## R-11: the overlap, and everything a rebase re-runs

- **Decision**: Before each push the lane rebases on `master`, reads both sides of
  every conflict, and runs every gate again on the rebased head (spec D-11).
- **Rationale**: The base reads `858c55d9`, re-read at head `22ea70ac`.
  `git rev-list --count 858c55d9..HEAD` answers ten commits ahead, and nothing
  stands behind that base. The two open pull requests
  at measure time, number 81 on `046-amd-acceptance` and number 80 on
  `046-report-rows`, change only files under `specs/046-hunter-topdown/`, so no
  hunk meets the ten files the plan's source list names. The reserved 041 names
  the lane that "changes how the counter limit is found"
  (`specs/042-topdown/spec.md:493-495`), so the meeting points for that work
  stand in `source/counters/system.cpp`, `source/counters/detail/core.hpp` and the
  counters test files. The limit test this lane keeps as its regression reads one
  `System` through `System::local()` (`test/source/counters_objects_limit_test.cpp:57`,
  `:66`) and its four cases run from one `main` (`:117-126`), so a rebase that
  moves that file moves a regression this lane depends on. A rebase re-runs every
  command of the plan's Gate commands block: the two `ctest` runs, the five gates,
  the `ci-ubuntu` build that carries clang-tidy and cppcheck, the two sanitizer
  trees, and the coverage trace with its gate.
- **Alternatives considered**: A merge commit, which the Constraints section
  forbids for a linear base branch. A push that skips the re-run, which leaves the
  recorded verdicts against a head the pull request no longer shows (Principle
  VIII).

## Deviations

Two readings in the spec and the notes needed a correction of a line or of a
wording. No decision moves.

- FR-012 first named the five files that carry the version as a value, seven
  lines in all. Two more lines quote the same digits in prose
  (`test/source/views_test.cpp:307`,
  `test/source/harness_cli_table_test.cpp:1474`), and they read stale once the
  value moves. Nine lines stand across those five files, which is what FR-012
  names now, and R-2 keeps the count by file. A tenth line quotes the digits at
  `CMakeLists.txt:61`, holds the specs/046 record, and stays beside the past
  records.
- `/tmp/050-design-notes.md:158` places `System::~System() = default;` at line
  204, one line above the base line 205 that held it. At head the definition
  stands at `source/counters/system.cpp:192`, below the constructor at
  `:181-190`.

FR-001's fallback clause, the case where a reader cannot reach `System::Impl`,
stands unreachable once the `System` pointer of spec D-4 lands, and round 2
records that reading (`/tmp/050-clarify-round2.md:210`). Spec D-5 keeps the
clause as the recorded alternative, so this file carries no deviation for it.
The spec's Open questions section reads `None`, so this file adds no reading of
its own.
