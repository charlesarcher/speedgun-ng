# Feature Specification: Intel Closeout Repairs

**Feature Branch**: `052-intel-closeout-repairs`

**Created**: 2026-11-10

**Status**: Draft

**Input**: Owner order of 2026-11-10 (`prompt.md`): close the Intel closeout
gaps PR #103 left open. Fix the ignore-contract unused helper, clear the
sanitizer reds inside owned scope, fix or block the DBC target setup, scrub
user-home paths, and keep the root rows blocked. The order extends
specs/048-hwlat-probe, specs/046-hunter-topdown and
specs/041-counter-probe with one dated repairs phase, and it allows a narrow
source fix in `source/harness/noise_tracer.cpp` for the deferred
unused-variable fault. Short name: intel-closeout-repairs. Condensed below;
the order is the authority for scope, numbering and the accepted answers.

## Decisions

Each decision names the reading it settles and the measurement behind it.
Times and dates stand on the host clock of `corsairone`, which runs six hours
behind coordinated universal time, so the host clock read 2026-11-09 evening
while the calendar date above is 2026-11-10.

- **D-1 Number, branch and base.** The feature is 052 on the branch
  `fix/intel-closeout-repairs`, cut from `origin/master`. 051 stood free at
  branch time and PR #106 landed `specs/051-raw-events` on 2026-11-10, so D-1
  of specs/050-limit-table-lifecycle and R-1 here hand the lane the next free
  number. The branch cut moved with it: the branch now stands on `1627e756`,
  which carries PR #104, #105, #106 and #107. Measured on 2026-11-10 (host
  2026-11-09 19:33): `git fetch origin` moved `origin/master` from `f73facd8` to
  `1f1f9b20`, the merge of PR #103, and four further merges on 2026-11-10 moved
  it to `1627e756`, which `git rev-parse origin/master` answers now.
  `git merge-base --is-ancestor 1627e756 HEAD` answers zero. At the time the
  order was written `gh pr list --state open` answered no row; it now answers
  one row, PR #108 `Docs: Correct Zen 4 Acceptance Records` on branch
  `fix/zen4-acceptance`, opened 2026-11-10T16:24:52Z, and D-16 carries that
  reconciliation too. `ls specs/` ends `047-loader-cross-field`, `048-hwlat-probe`,
  `049-names-old-records`, `050-limit-table-lifecycle`, `051-raw-events` and this
  directory: PR #104 took 049 and PR #106 took 051, so 052 is the number this
  feature carries. `git ls-remote --heads origin` names no branch of this name
  before this branch pushed, so no reconciliation of a pushed branch stands
  open. The order states version 0.34.0, schema 1.16, harness `SOVERSION` 22,
  archive `SOVERSION` 8 and 84 markers. Four of the five agree at this base:
  `source/harness/json_report.cpp:58` answers schema 1.16, the two `SOVERSION`
  lines answer 22 and 8, and `git grep -c 'LCOV_EXCL' -- source include` sums to
  84. The version moved: `CMakeLists.txt:87` reads `VERSION 0.35.0` at
  `1627e756`, and 0.34.0 stands at `1f1f9b20` and at `c9187ba3`, because PR #106
  carried the bump. No version, schema or `SOVERSION` moves *in this feature*,
  because no public header and no report field moves.
- **D-2 The base code tree is the closeout tree.** The command
  `git diff --name-only c9187ba3..1f1f9b20 -- include source test example tools cmake .github CMakeLists.txt CMakePresets.json`
  prints nothing.
  Only `specs/` moved between the two. That reading is why the lane reuses the
  lane trees the closeout left configured and built, and a reader needs it
  before comparing any wall time in this file with a figure on master.
- **D-3 A commit cannot carry its own hash.** ICR-006 asks the appended
  record to cite the final SHA, and that record lands inside the commit whose
  hash it would print. A commit cannot hold its own hash.
  - **Reading 1 (taken).** The second pass runs at the squashed tip, so the
    two heads ruling 29 of `Rulings - this phase (29 to 42)` of
    specs/048-hwlat-probe names stand equal, and that ruling's proof clause
    reads as an empty answer. A figure row names its head by handle: the
    branch, the commit that carries this block, and the command
    `git rev-parse HEAD` run at that commit. The pull request body prints the
    same hash in full, and `git ls-remote` answers it on the remote. A reader
    resolves every figure by one command, and no row quotes a hash that had to
    exist before its own run.
  - **Reading 2 (rejected, recorded).** Amend once after the second pass so a
    record can print the hash the earlier pass read. That breaks the
    one-commit rule of the order, and the hash in the text belongs to a commit
    no longer reachable.
- **D-4 Two names fault at the ignore semantic, and one attribute closes
  each.** Measured at the base with the tree the closeout left at
  `build/consumer-release`:

  ```
  cmake --preset=ci-linux-ignore -B build/consumer-release \
    -D CMAKE_BUILD_TYPE=Release -D speedgun-ng_DEVELOPER_MODE=OFF   # exit 0
  cmake --build build/consumer-release -j "$(nproc)"                # exit 2
  ```

  The compiler prints one line, kept whole below:
  `source/harness/noise_tracer.cpp:499:6: error: ... 'activationAnswered' defined but not used [-Werror=unused-function]`.
  At the ignore semantic
  `SG_CONTRACTS_SEMANTIC` is 0, so `SG_ENSURE` expands to `((void)0)` at
  `include/speedgun-ng/dbc.hpp:463`, and the one call of the helper at
  `source/harness/noise_tracer.cpp:551` disappears. `-Wunused` sits in the
  flag set, and `cmake/variables.cmake:28` sets `CMAKE_COMPILE_WARNING_AS_ERROR`
  to `ON` for a top-level build. The `add_compile_options(-Werror)` line at `:33`
  stands behind a `CMAKE_VERSION VERSION_LESS "3.24"` test, and this host answers
  cmake 4.4.2, so the promotion that binds here is the one at `:28`.
- **D-5 A second name of the same class stands behind the first.** With the
  first name closed, `cmake --build build/consumer-release-dev -j 24 -- -k`
  exits 2 with one line, kept whole below:
  `example/docs/hunt_ranks.cpp:147:15: error: unused variable 'records' [-Werror=unused-variable]`.
  The keep-going form prints one error line for
  the whole tree, so the two names are the full set at that semantic in that
  tree. `records` feeds one `SG_ENSURE` at `example/docs/hunt_ranks.cpp:148`
  and nothing else. The closeout record names only the first fault, because
  its build stopped at the first error.
- **D-6 The fix shape is the attribute the tree already uses.** Each name
  gains `[[maybe_unused]]` and a comment that states the reason. The tree
  carries the precedent: `source/harness/compare.cpp:1156` holds a helper
  whose comment says the call site stands inside `SG_ENSURE`,
  `source/harness/jitter_probe.cpp:1153` and `source/harness/pass_plan.cpp:90`
  carry the attribute for the same reason, and
  `source/harness/host_record_check.cpp:443` carries it on a local. The
  attribute changes nothing at the enforce semantic, weakens no macro and
  leaves `-Werror=unused-function` live for every other name. A call of the
  helper that discards its answer stands rejected: it states nothing, and it
  moves work into the release path to serve a build flag.
- **D-7 The fence widens by one file, and the widening is recorded.** The
  order names `source/harness/noise_tracer.cpp` as the one source file, and its
  `Owned edit scope` section reads: "Prior closeout fence:
  `test/base_cli/run_matrix.py` + fixtures, `test/example_run.sh` + fixtures,
  `harness_hunt_cli_test.cpp`, `harness_jitter_cli_test.cpp`, and specs
  041/046/048 repair text." That quotation records the order's wording for the
  reconciliation pass, and the Constraints section below is the fence this
  feature executes. The list above holds no `example/` path, so
  `example/docs/hunt_ranks.cpp` sits outside it and Reading 1 is a recorded
  deviation from the order's own fence line, named here for the reconciliation
  pass to read. Ruling 36 of
  `Rulings - this phase (29 to 42)` of specs/048-hwlat-probe keeps a diagnosis
  outside the list as a recorded deferral. Two readings stand.
  - **Reading 1 (taken).** The fence widens by that one file. US1 asks that
    the ignore-contract builds succeed at the tip, and no file inside the
    closed fence can close the developer-mode variant. The attribute lands at
    line 147, outside the `sg-example:begin hunt-ranks` region of that file,
    which spans lines 15 to 33, so the page-drift gate reads an unchanged
    block. The pull request body names the widening.
  - **Reading 2 (rejected, recorded).** The fence stays closed and
    `hunt_ranks.cpp` gets the blocked row FR-019 defines, with its reproduction
    added. US1 then closes for the consumer tree alone, and the
    developer-mode ignore build stays red for the next reader. Reverting one
    attribute returns this decision to that reading.
- **D-8 A gate keeps the ignore semantic building.** The regression must fail
  when a name that feeds only `SG_ENSURE` stands without the attribute.
  `test/simulation_mark_shape.sh` and `test/counters_tsc_read_shape.sh` hold
  the shape this feature copies: compile one translation unit per compiler for
  semantic 0 and semantic 2, and plant a probe so a clean run cannot come from
  a scan that inspects nothing. The taken shape is one new gate script over
  both files above, one CTest entry, every compiler on `PATH`, both semantics,
  and `-Werror` on the two warning names. The planted probe is a helper in a
  scratch copy of a real unit that must fail the semantic-0 compile and pass
  the semantic-2 compile. Measured cost: one such compile of
  `source/harness/noise_tracer.cpp` at semantic 0 exits 1 before the attribute
  and exits 0 after it, and no other diagnostic appears. FR-005 carries the duty
  that the entry's row prints its wall time per compiler and per semantic, read
  at the tip. A host that holds no compiler for the target prints the compiler
  set it found, and the reading says so.
- **D-9 Every sanitizer red at the base is a ceiling cost.** The dated rows of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` read 2 failed of 185
  in `sanitize`, 3 of 186 in `tsan` and 4 of 185 in `coverage` at head
  `208bc520`, and each named entry reports `Timeout`. Those entry counts are the
  dated rows, which stand behind this base. The count of `add_test(` lines answers
  189 at `1f1f9b20` and 191 at `1627e756`, read with `git grep -c` over
  `test/CMakeLists.txt` at each rev, and T001 reads each live tree with
  `ctest --test-dir <tree> -N`. The same entries exit 0
  when their binaries run with no ceiling. The ceiling controls of
  `specs/048-hwlat-probe/results/acceptance-final-intel.md:1654-1667` print
  seven such direct runs: 285.0 s for
  `base_cli_matrix` in `coverage`, 704.0 s and 332.0 s for `example_run` in
  `coverage` and `sanitize`, 572.0 s for `harness_hunt_cli_test` in `coverage`,
  482.0 s and 392.0 s for `harness_jitter_cli_test` in `coverage` and `tsan`, and
  138.0 s for `base_cli_matrix` in `sanitize`. No row names a sanitizer report, so
  no row claims a race or an invalid access.
- **D-10 The ceilings stay, and the drivers lose wall time.** A ceiling is a
  test property, and `ctest --timeout` cannot lift one: the property wins.
  `test/CMakeLists.txt` hands every entry that names no `TIMEOUT` the quick
  label and 300 s, and `base_cli_matrix` carries 120 s. That file sits outside
  the fence, and FR-011 is the normative home of the rule that no ceiling moves
  here. Each owned driver then either loses
  wall time at its own cost or its row stays open with the owning path, the
  figure and the operator action named. A driver that runs its cases one after
  another gains from a bounded pool inside the driver, which keeps every case.
  Cutting cases stands rejected: the rows of `base_cli_matrix` and the docs
  examples carry requirements of their own. A planted sleep stands rejected on
  the order's own rule against clocks in tests. The plan measures each entry
  in three shapes before it chooses, so the choice rests on a wall time.
- **D-11 The coverage allowance is spent, so this feature adds no branch.**
  `tools/dbc/coverage_gate.sh:108` holds allowance 21. The closeout chain read
  20 rows at `af54d3f0` and 21 rows at `208bc520`, and the gate passed the
  second with nothing left; lines read 100.0% at 18,683 of 18,683. Nothing in
  this feature adds a branch position to instrumented code, adds an
  `LCOV_EXCL` marker past the 84 lines the base carries, or passes
  `--ignore-errors` to any capture. Each of the two source fixes adds one
  attribute, which adds no position, and the new gate is a shell script, which
  adds no instrumented line at all. The host chain still needs the hand
  re-runs, because four entries die at their ceilings in a serial coverage run
  and each passes when its own binary runs by hand.
- **D-12 The DBC target closes through its interpreter.**
  `cmake/dbc-gate.cmake` runs doxygen, then `tools/dbc/dbc_doc_gate.py`, then
  `tools/dbc/dbc_pair_gate.py`, all with `${Python3_EXECUTABLE}`, the
  interpreter the tree configured. The closeout trees configured an
  interpreter under the operator home that carries no PyYAML, so the target
  exits 2 with `dbc gates require PyYAML`. The taken route configures the lane
  tree with the interpreter that carries PyYAML, through
  `-D Python3_EXECUTABLE=/usr/bin/python3`, then builds the `dbc-gate` target
  and records its exit and its interface count. The alternative the order
  allows is the blocked row FR-019 defines, whose operator action is that
  configure line. No edit lands in `cmake/dbc-gate.cmake` or any other shared gate
  file, so a fresh clone keeps its own reading and the record states what an
  operator does there.
- **D-13 Rows this phase adds carry no home path.** Six lines in two records
  named a path under the operator home at branch time: five in
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` at lines 218, 340,
  565, 1546 and 1547, and one in
  `specs/041-counter-probe/results/acceptance-final-intel.md` at line 352. Two
  of them, 1546 and 1547, are rows PR #103 added. PR #104 landed on 2026-11-10
  and corrected that set: `grep -n '/home/'` over the two records on
  `origin/master` answers nothing, and the rows print `$HOME`-relative paths.
  The list stands here as the reading that was true at branch time, and this
  phase rewrites none of the six. The order keeps the dated history of PR #99,
  #102 and #103. Where this phase adds a dated block it follows the standing
  pattern of `Corrections to the sections above` of the same file: a dated block
  adds a correction list, each correction row quotes the row it supersedes and
  prints the portable spelling, and the superseded line stays where it stands.
  Every row this phase writes spells the operator home as `$HOME` and names an
  interpreter by file name.
- **D-14 The root rows stay blocked with the right they lack.** The
  refreshed reading of the host stands in the new dated block: the package
  query, the sudo probe, the pkexec probe, the tracefs mode and the machine
  specific register modes. Each blocked row keeps its operator line and its
  restore line. The phase collects no password, automates no elevation and
  weakens no kernel control. The standing blocked block at
  `specs/048-hwlat-probe/results/acceptance-final-intel.md:1075-1080` is the
  shape this follows, and IA48-003 is the rule behind it.
- **D-15 One commit, one lease, one pull request.** One squashed commit
  carries the change, with the template of the constitution: a section token,
  a title of 50 columns or fewer, a why-body wrapped at 72, `Approved-by: OmO`,
  and `Refs:` lines for 052, 048, 046 and 041. The first push runs
  `git push -u origin fix/intel-closeout-repairs`. A later push runs
  `git push --force-with-lease=fix/intel-closeout-repairs:<expected-full-sha>`
  and every push stands followed by
  `git ls-remote origin refs/heads/fix/intel-closeout-repairs`, whose answer must equal the local
  head. One pull request opens and never merges, and no issue closes: #76
  stays open.
- **D-16 A concurrent pull request stands open, and the base holds.** Two
  cases fall under this decision. PR #104, the record scrub, opened on branch
  `049-names-old-records` against `master` with 82 files after this branch was
  cut, and merged 2026-11-10T16:14:43Z. PR #108, the Zen 4 record correction,
  stands open on branch
  `fix/zen4-acceptance` against `master` since 2026-11-10T16:24:52Z, and it
  edits two of the same records. What #104 said about its own file list holds
  for #108: it edits
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`,
  `specs/041-counter-probe/results/acceptance-final-intel.md`,
  `specs/048-hwlat-probe/plan.md` and `.specify/feature.json`: two of the three
  records this feature appends to, the plan that carries the pinned duty of
  FR-026, and the file that names the feature directory. The order's
  precondition of no open pull request no longer holds, so the order's
  reconciliation clause applies and this decision is that reconciliation.
  - **Reading 1 (taken).** The base is the tip of `origin/master` as it stands
    when a measurement starts, and no measurement had started when the four
    pull requests merged. PR #104, #105, #106 and #107 landed on 2026-11-10 and
    moved the tip to `1627e756`; the branch rebased onto it the same day, before
    any figure, so no row carries a base that no longer stands. Work continues
    on `fix/intel-closeout-repairs`. Before the squash of FR-025 the branch
    rebases once more onto `origin/master` as it then stands. The two record
    appends are written after that rebase, because a merged #108 can move a line
    a row cites or answer it. That file
    `.specify/feature.json` stays uncommitted, so its conflict stays outside
    the branch. FR-026 re-reads every wall figure at the tip, so a rebased
    markdown parent changes no number in the record.
  - **Reading 2 (rejected, recorded).** Wait for #104 to merge before any
    record line is written. The code work, both sanitizer lanes and the
    coverage chain then wait on a document pull request, and the close date of
    the order moves with it.

- **D-17 The owner set two bounds on 2026-11-10.** The order asked for an
  analysis loop that ends when a pass reports nothing, and for a full second pass
  at the final SHA (the owner order of 2026-11-10, ICR-008). The owner narrowed
  both the same day, with the measured price in front of them. The analysis loop
  ends when every critical and high finding of a round stands remediated and one
  fresh reader has re-read the seams that round's remediation touched; a medium or
  low finding that still stands goes into `plan.md` Complexity Tracking with the
  line that says why it does not block implementation. The second pass covers the
  six pinned shapes, the coverage chain and gate, the named gates and the DBC
  target or its blocked row, and FR-026 carries every figure it does not re-read.
  The readings behind the choice: three rounds answered 46, 29 and 37 findings in
  85, 28 and 70 minutes of reader time, and one pass prices 6.8 h against 13.6 h
  for two.

## Roadmap position

The Intel acceptance series runs through PR #96, #98, #99, #102 and #103. PR
#103 landed the second-pass figures and left four lines open: the ignore build
fault, the sanitizer reds, the DBC target exit and the user-home paths. This
feature closes those lines and appends one dated repairs block to each of
specs/048-hwlat-probe, specs/046-hunter-topdown and
specs/041-counter-probe. It closes the deferral ruling 36 of specs/048-hwlat-probe
opened for the `noise_tracer` fault. It starts no new capability, and the root
surface, the perl-GD picture step and the hosted runners stay where the
closeout left them.

## Clarifications

### Session 2026-11-10

Three questions stood. Each one is reconciled against the owner order of
2026-11-10, and each answer names the line of that order it serves.

- Q: Which lanes does the green-or-blocked rule bind at the tip: the `sanitize`
  and `tsan` lanes, or the `coverage` lane as well? (FR-009, FR-017, SC-003)
  → A: The two sanitizer lanes. ICR-003 names the address, undefined-behavior
  and thread sanitizer failures, and ICR-004 asks the coverage lane for a gate
  verdict. The coverage suite keeps the hand re-run route of FR-017, and a
  coverage entry gains a blocked row when its own hand run answers non-zero.
- Q: What does the record print for an entry that passes at the base and turns
  red in a sanitizer lane at the tip? (FR-009, FR-013, SC-003) → A: The same
  two outcomes bind every sanitizer-lane red at the tip, and the row names the
  head that first named the entry. The base set moves between two runs of one
  head, so a set read from one dated row cannot carry the rule.
- Q: For a requirement that guards a value the base already holds, what evidence
  answers FR-029, which asks a red run ahead of the change? → A: Three evidence
  classes. A fault check answers non-zero at the base and zero at the tip. An
  invariant check answers zero at both heads beside one planted violation that
  answers non-zero. A process fact stands read from the host or the remote, and
  it carries no planted violation.
- Q: Which reading carries a sanitizer verdict at the tip: a lane's own suite
  run, or every shape FR-010 prints, a pinned release row included? → A: The
  lane's own suite run in the form CI runs carries the verdict, because
  Principle VIII holds that run hard. Every other shape stands as evidence
  beside its witness, and ruling 6 of the closeout ruling list already works
  that way: the isolated run and the bounded run explain a name the suite run
  already named. FR-026 still runs the pinned shapes, because the order asks
  them and the base rows carry them.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A consumer builds with contracts ignored (Priority: P1)

A downstream consumer configures the project with the ignore contract semantic
and builds it. Today the compiler stops on a helper nothing calls at that
semantic, so the consumer cannot build the library at all, and the
developer-mode variant stops on a second name of the same class. At the tip
both trees build, and a gate entry keeps that outcome checked on any host.

**Why this priority**: It is the only red that stops a build, and it is the
one defect the owner already deferred with a named file.

**Independent Test**: Configure and build both ignore-semantic trees at the
tip, and run the new gate entry. Both builds exit 0 and the entry exits 0, on
a tree where nothing else of this feature landed.

**Acceptance Scenarios**:

1. **Given** a tree at the tip, **When** the ignore preset configures and the
   build runs with `speedgun-ng_DEVELOPER_MODE=OFF`, **Then** the build exits
   0 and prints no unused-name diagnostic.
2. **Given** the same tree, **When** the same pair runs with
   `speedgun-ng_DEVELOPER_MODE=ON`, **Then** the build exits 0.
3. **Given** the new gate entry, **When** a reader plants a helper that feeds
   only a postcondition in a scratch copy, **Then** the compile at semantic 0
   exits 1 and the compile at semantic 2 exits 0.
4. **Given** the enforce semantic, **When** the library lanes build, **Then**
   their exits stay as they stand today.

---

### User Story 2 - A maintainer reads a sanitizer lane (Priority: P2)

A maintainer opens the `sanitize` or `tsan` lane and finds a small set of
failed entries. Today each name reports a ceiling timeout, and no row says
whether the entry hides a race, a slow driver or a host fact. At the tip each
name is either green inside its ceiling or a blocked row that names the
owning path, the figure behind the claim and the action an operator takes.

**Why this priority**: Two lanes stay red on the tip, and Principle VIII calls
both hard. The work reports a host fact or a fence fact at worst, so it ranks
under the build fault.

**Independent Test**: Run each named entry in its lane in the bounded shape,
the direct shape and the parallel shape, and read its row. Every row matches
its lane run, and no ceiling value moved in the tree.

**Acceptance Scenarios**:

1. **Given** an entry named red at the base, **When** the entry runs bounded,
   direct and in the parallel shape at the tip, **Then** its row prints all
   three figures with their exits.
2. **Given** an entry that still cannot finish inside its ceiling, **When**
   the record is read, **Then** its row names the owning path, the measured
   wall time and the exact operator action.
3. **Given** the tree at the tip, **When** a reader greps the ceiling values,
   **Then** every value equals the value at the base.
4. **Given** a driver this phase changed, **When** its cases run, **Then** the
   case count and every case name stay as they stand at the base.

---

### User Story 3 - A reader on another machine opens a record (Priority: P3)

A reader opens an Intel record on a machine whose accounts differ. Rows that
print a path under one operator's home read as noise there, and the rows about
the root surface must still say what the lane lacked. At the tip every row this
phase adds or corrects spells such a path as `$HOME`, and every blocked root
row names the right it lacks with the fresh host reading behind it.

**Why this priority**: Nothing here fails a build or a test, and the owner
named it as a medium finding with a bound: only rows this branch touches.

**Independent Test**: Read the new dated blocks and grep them for a home path;
read each corrected row beside the row it supersedes. The figures match, and
the spelling differs.

**Acceptance Scenarios**:

1. **Given** a row this phase adds, **When** a reader greps it, **Then** no
   path begins under the operator home.
2. **Given** a row PR #103 added that carries a home path, **When** this phase
   corrects it, **Then** the correction row keeps every figure and states the
   portable spelling.
3. **Given** a blocked root row, **When** the phase closes, **Then** the row
   names the right it lacks, the operator line and the restore line, and no
   row prints a password prompt or an elevation attempt that succeeded.

---

### User Story 4 - The owner lands one tip and reads one body (Priority: P4)

The owner wants one commit, one tip and one pull request whose body carries the
head hash, the per-lane counts, the root status and every deferred cause. At
the tip the second pass ran on exactly that hash, the branch pushed under a
lease, and the remote answers the same hash.

**Why this priority**: It is the delivery gate for the other three stories,
and it holds no code of its own.

**Independent Test**: Read the pull request body, then run
`git ls-remote origin refs/heads/fix/intel-closeout-repairs` and compare. The
hash matches, one commit carries the change, and the branch stands unmerged.

**Acceptance Scenarios**:

1. **Given** the squashed tip, **When** the second pass runs, **Then** every
   step of its list reports an exit at that hash and the body prints it.
2. **Given** the branch, **When** a reader counts its commits over the base,
   **Then** one commit stands there and its message carries the template.
3. **Given** the pull request, **When** the phase closes, **Then** it stands
   open and no issue closed.

---

### Edge Cases

- What happens when a host carries one compiler? The new gate entry covers the
  compilers on `PATH` and skips with a reason when none targets the platform,
  and the tier-label gate keeps its row for the skip.
- What happens when a lane tree no longer matches the tip? The lane
  reconfigures and rebuilds that tree, and every figure in the record names
  the head and the tree that answered it.
- What happens when a hand re-run of a ceiling-cut entry leaves the tree dirty
  with generated coverage files? The record states which files the run wrote
  and how the tree came back clean, and the tracked diff stays empty.
- What happens when the `dbc-gate` target still exits 2 at the tip with the
  interpreter named? Its row becomes the blocked row FR-019 defines, with the
  field set that requirement names.
- What happens when a sanitizer red turns out to sit outside the fence? The
  row names the owning path and defers the change, as ruling 36 requires.
- What happens when the coverage gate reads above 21 rows at the tip? The
  phase reports it as a host reading with its denominator, names the rows, and
  takes no allowance widening, because widening is a maintainer decision.
- What happens when the hosted runners answer figures that differ from the
  host figures? Both stand recorded with their denominators, and no row
  compares a host row count with a hosted row count as movement.
- What happens when an entry passes at the base and turns red in a sanitizer
  lane at the tip? It carries the same two outcomes, and its row names the head
  that first named it.
- What happens when the remote branch moves while the lane works? The lease
  push fails, the lane stops, and the reconciliation stands recorded.
- What happens when the pull-request tool cannot authenticate? The branch
  stands pushed, the body stands written to a file outside the repository, and
  the record states that no pull request number exists yet (ICR-008).
- What happens when a pinned bound names a red that the lane's suite run does
  not name? The row prints the shape, the command and the witness FR-009 names
  for such a red, and the entry stays outside SC-003's count.

## Requirements *(mandatory)*

### Builds at both contract semantics (ICR-001, ICR-002)

- **FR-001**: The lane shall build every tree it measures from the head whose
  record carries its figures, and each figure shall name that head, the tree
  and the host facts behind it (ICR-001).
- **FR-002**: `source/harness/noise_tracer.cpp` shall compile with no unused
  name at the ignore semantic and at the enforce semantic, with `-Werror` on
  the unused-function and unused-variable warnings.
- **FR-003**: `example/docs/hunt_ranks.cpp` shall compile with no unused name
  at the ignore semantic and at the enforce semantic, under the same flags.
- **FR-004**: Both ignore-semantic consumer trees shall build at the tip: the
  variant with `speedgun-ng_DEVELOPER_MODE=OFF` and the variant with it `ON`.
- **FR-005**: A new CTest entry shall compile both files above for semantic 0
  and semantic 2 with every compiler on `PATH` that targets the platform, and
  it shall exit 0 at the tip. Each file compiles in its own step, so a failing
  first step cannot hide the reading of the second. The entry's row shall print
  its wall time per compiler and per semantic, read at the tip, with the
  compiler set it found.
- **FR-006**: That entry shall carry a planted probe: a scratch copy whose
  name feeds only a postcondition shall fail the semantic-0 compile and pass
  the semantic-2 compile, so a clean run cannot come from a scan that
  inspects nothing.
- **FR-007**: No change shall land in `include/speedgun-ng/dbc.hpp`, in any
  contract macro, or in the meaning of the ignore semantic.
- **FR-008**: The version, the report schema and the two `SOVERSION` numbers
  shall keep the values the base carries.

### The sanitizer lanes (ICR-003)

- **FR-009**: Each entry named red in the `sanitize` lane or the `tsan` lane,
  at the base or at the tip, shall reach one of two outcomes at the tip: it
  passes inside its ceiling, or its row states BLOCKED in the shape FR-019
  defines, with its measured figures added. Its row shall name the
  head that first named the entry (ICR-003). The reading that decides is the
  lane's own suite run in the form CI runs. A red that only a pinned bound or
  an isolated run names shall gain a row with its command, its exit, its wall
  time and its load witness, and that row carries no further obligation. The
  set this phase tracks holds four entries: `base_cli_matrix`, `example_run`,
  `harness_hunt_cli_test` and `harness_jitter_cli_test`. At the base the first,
  the second and the fourth stand red in a sanitizer lane's own suite run, and
  `harness_hunt_cli_test` stands red in the coverage lane's run, where FR-017
  binds it; the pools of this phase reach that entry in both sanitizer lanes as
  well, because its wall cost is the same shape there.
- **FR-010**: Each such entry shall be measured in three shapes at the tip:
  bounded inside its ceiling from the CTest working directory, direct with no
  ceiling, and inside the parallel lane form. The row prints all three exits
  and all three wall times.
- **FR-011**: No ceiling value shall move, no entry shall lose a case, and no
  test shall gain a sleep, a clock or a planted processor.
- **FR-012**: Any change that shortens a driver shall keep the driver's case
  names, its rows and its verdict rules intact, and its own fixtures shall
  still plant each failure the fixture names.
- **FR-013**: The entry count of each lane shall stand recorded before and
  after, so a reader sees the count the figures belong to.
- **FR-014**: A red whose cause sits outside the fence shall be reported with
  its reproduction and its owning path, and no code line outside the fence
  shall change for it.

### Coverage and the DBC target (ICR-004, ICR-005)

- **FR-015**: The coverage gate at the tip shall read 100% lines, a branch
  shortfall of 21 rows or fewer, and it shall reach that verdict with no
  `--ignore-errors` flag and no new exclusion marker (ICR-004). When the capture
  it reads stands above 21 rows on a host that holds no right to buy the rows
  the ledger names as purchased, the row shall state BLOCKED in the shape
  FR-019 defines, with the reading the host answered and its line and branch
  denominators added. When the
  gate exits for a tool fault, its usage exit or its capture exit, the row shall
  print the tool's own complaint and the command, and it shall name no verdict.
- **FR-016**: The count of lines that carry an exclusion marker within `source`
  and `include` shall stay 84, read by the command
  `git grep -c 'LCOV_EXCL' -- source include` summed over its rows.
- **FR-017**: Each entry the serial coverage run cuts at its ceiling shall
  also run by hand from the coverage tree, and its row shall print the wall
  time and the exit of that run. When such a hand run answers non-zero, that
  entry shall carry the blocked row FR-019 defines.
- **FR-018**: The `dbc-gate` target shall exit 0 at the tip under the
  interpreter the lane configured, and the row shall print that interpreter by
  file name and the interface count its own run answered (ICR-005).
- **FR-019**: When that target cannot exit 0, its row shall state BLOCKED. That
  row, and every other blocked row this feature writes, shall carry the fields
  this requirement names: the reading that stands in the way, the right or tool
  the host or the target lacks, the operator action as a command an operator
  copies, the value to restore, the owning path, and, where the row stands in
  place of a hard gate, the clause of Principle VIII it answers together with
  the P2 registration the Constraints section requires. Borrowers add one named
  field each and restate none of this set: the DBC row of FR-018 names the
  configure line and the interpreter the target found, a root row of FR-023 names
  the refreshed host reading, a lane row of FR-009 names the head that first
  named the entry and its measured figures, and a coverage row of FR-015 names
  the reading the host answered with its line and branch denominators. A row of
  FR-017 adds nothing beside this set. Where the thing that stands in the way is
  a ceiling this feature does not raise, the operator action names the ceiling
  line and the property that sets it, and the value to restore names the ceiling
  in force today and the wall time the entry needs.

### The record and the pull request (ICR-006, ICR-007, ICR-008)

- **FR-020**: A dated repairs block shall stand appended to
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`, and
  specs/046-hunter-topdown and specs/041-counter-probe shall each gain such a
  block only where a figure of theirs moved (ICR-006).
- **FR-021**: No dated block that PR #99, #102 or #103 landed shall lose a
  line, and every superseded row this phase answers shall stand in a
  correction list that keeps the original line where it sits.
- **FR-022**: Every row this phase adds or corrects shall print no path under
  the operator home, and one line of each new block shall state the
  spelling rule it followed (ICR-005, D-13). All six lines D-13 names stand corrected on
  `origin/master` since PR #104, and no task of this phase rewrites a dated line
  that stands corrected.
- **FR-023**: Every blocked root row shall carry the fields FR-019 defines, with
  the refreshed host reading behind it added, and the set stays BLOCKED
  (ICR-007). The standing blocked block at
  `specs/048-hwlat-probe/results/acceptance-final-intel.md:1073-1080` names the
  shape, and IA48-003 the rule.
- **FR-024**: No row shall name a machine other than the host that carried the
  measurement (G-24).
- **FR-025**: One squashed commit shall carry the change with the commit
  template of the constitution. The branch shall stand rebased onto
  `origin/master` as it then stands before that squash, and FR-022's
  correction list shall re-read the lines it names after the rebase. The pull
  request body shall print the tip hash, the count per lane, the root status
  and every deferred cause in lines of 72 columns or fewer (ICR-008).
- **FR-026**: The first pass shall run at the candidate head and shall cover the
  release tree, the sanitizer tree, the thread-sanitizer tree, the coverage tree
  and gate or the blocked clause of FR-015, the analyzer tree with its finding
  count on the `WarningsAsErrors` list, the prose, spell, page-drift, tier-label
  and format gates, both ignore-consumer trees and the DBC target or its blocked
  row. The second pass shall run on the exact squashed tip and shall cover the
  six pinned shapes, the coverage chain and gate or the blocked clause of
  FR-015, those named gates, and the DBC target or its blocked row. A figure the
  second pass does not re-read shall carry the first-pass value with both heads
  named and the reading
  `git diff --name-only <first-pass head> <squashed head> -- include source test example cmake tools external CMakeLists.txt CMakePresets.json .clang-tidy .github/workflows`
  printed beside it, and where that diff prints a name the second pass runs the
  whole list of the first pass. That pathspec carries the build configuration, the
  analyzer rule file and the workflow, because an analyzer verdict and a lane
  figure turn on those files as much as on the code. Each pass shall carry a pinned shape for each
  lane whose base row in `specs/048-hwlat-probe` carries one, run under the same
  processor mask and job width as that base row. Each such row shall print the entry count, the
  exit and the wall time. Every step prints its exit (ICR-008).
- **FR-027**: The first push shall run
  `git push -u origin fix/intel-closeout-repairs`, a later push shall use the lease
  form, and each
  push shall be followed by a remote read whose hash equals the local head. The
  expected full hash of the lease form shall come from a remote read taken
  before that push, and the row shall print both hashes.
- **FR-028**: One pull request shall stand open at the close, no merge shall
  happen, and no issue shall close.

### Verification of every requirement

- **FR-029**: Each requirement above shall name its check in `plan.md`, and
  each check shall carry one of three evidence classes. A fault check answers
  non-zero at the base and zero at the tip. An invariant check answers zero at
  both heads beside one planted violation that answers non-zero. A process fact
  stands read from the host or the remote, and it carries no planted violation.
  A fault reading is taken at the base code tree with the check present and the
  fix absent, the state R-4 records. Every row of the record shall name the
  command that answered it.
- **FR-030**: The prose gate, the spell gate and the page-drift gate shall
  exit 0 over the text this phase adds, in both the tree form and the range
  form.

## Key Entities

- **Lane reading**: one command, its exit, its wall time, the tree it ran in
  and the head it ran at. Every figure in the record resolves to one.
- **Red name**: one CTest entry of the four-name set FR-009 names, read red in a
  sanitizer lane or the coverage lane at the base or at the tip, tracked to its
  outcome: green inside its ceiling, or the blocked row FR-019 defines. The row
  names the head that first named it.
- **Blocked row**: a row that carries the field set FR-019 lists, and no other
  shape.
- **Correction row**: a row that answers a row an earlier dated block landed,
  keeps every figure, and changes the spelling of a path.
- **Ignore-semantic gate**: the CTest entry that compiles a real translation
  unit at two contract semantics for every compiler on the host, with its
  planted probe.
- **Repairs block**: the dated section each of the three specs gains, with its
  lane figures, its correction list and its open rows.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Both ignore-semantic consumer builds exit 0 at the tip, and the
  count of unused-name diagnostics they print is zero.
- **SC-002**: The entry FR-005 names and the probe FR-006 plants answer, at the
  tip, the exits those two requirements state.
- **SC-003**: Take each entry that runs red in a sanitizer lane's own suite
  run, at the base or at the tip. Each such entry stands green inside its
  ceiling or carries a blocked row that names an owning path. The count of
  entries in neither of those two states stands zero. A red a pinned bound names
  alone is evidence with the witness FR-009 names, and it stands outside this
  count.
- **SC-004**: Zero ceiling values move between the base and the tip, and the
  case count of each driver this phase changed matches its base count.
- **SC-005**: The coverage gate at the tip exits 0 with 100% lines and a
  branch shortfall of 21 rows or fewer, or its row fires the blocked clause of
  FR-015. The count of coverage rows in neither state stands zero, and the count
  of lines carrying an exclusion marker stays 84.
- **SC-006**: The `dbc-gate` target reports an exit at the tip, and a reader
  can copy the command that answers it from the record.
- **SC-007**: Zero lines under the new dated blocks begin a path with the
  operator home, and every correction row keeps the figures of the row it
  answers.
- **SC-008**: One commit stands between the base and the tip, the remote hash
  equals the local hash, the pull request body prints that hash, and the pull
  request stands open and unmerged.
- **SC-009**: The prose, spell and page-drift gates exit 0 at the tip, the
  analyzer tree prints its finding count on the `WarningsAsErrors` list from a
  step that compiled at least one translation unit, and the suite count of every
  lane stands recorded with its figures.

## Constraints

- Principle VIII holds both sanitizer lanes, the coverage gates, the
  static-analysis and name-check gates, and the prose gates as hard. A blocked row reports a host or fence fact; it waives
  nothing. A hard gate the tip cannot read green for gains a row that names the
  clause of Principle VIII it answers and registers the gap as a P2 with
  written justification.
- The fence is: `source/harness/noise_tracer.cpp`, its internal header when
  required, `example/docs/hunt_ranks.cpp` under D-7, the prior closeout drivers
  `test/base_cli/run_matrix.py` with the data files
  `test/data/base_cli_matrix.txt` and `test/data/base_cli_console.txt` it
  reads, `test/example_run.sh` with the `example/docs/` units it compiles,
  `test/source/harness_hunt_cli_test.cpp`,
  `test/source/harness_jitter_cli_test.cpp`, the new gate script and its
  registration, and the repair text of specs/041, specs/046, specs/048 and this
  feature. No task of this phase writes a fixture, and the gate script plants
  its probe in a scratch copy.
- This list is the one fence statement of the feature. `plan.md`, `tasks.md`
  and every decision cite it, and a path that joins it carries the decision id
  that widened it.
- Out of the fence: `.github/workflows/`, `CMakePresets.json`,
  `cmake/dbc-gate.cmake`, the `TIMEOUT` values in `test/CMakeLists.txt` while
  that file's `add_test` region stays inside the fence, the shared gate
  scripts, and `README.md`, which this phase leaves alone.
- Acceptance is local (G-20). No test gains a sleep, a clock or a planted
  processor.
- No `.specify/feature.json` line lands in the commit, and `prompt.md` stays
  untracked.

## Scope

In scope: the two unused names and their attribute, the new ignore-semantic
gate, the wall-cost work inside three owned drivers and the blocked row for the
fourth, the DBC target reading,
the three dated repairs blocks with their correction list, the refreshed root
rows, one push under a lease and one pull request.

Out of scope: raising a ceiling, editing a workflow or a preset, widening the
branch allowance, adding an exclusion marker, moving a version, a schema or a
`SOVERSION`, the perl-GD picture step, the root surface, the hosted runners,
and any machine other than the host that carries the measurements.

## Preconditions

- `origin/master` stands at `1627e756`, read with `git rev-parse origin/master`,
  and `git merge-base --is-ancestor 1627e756 HEAD` exits 0.
- No pull request stood open when the order was written. PR #104 opened after the
  branch was cut and merged, and PR #108 stands open now. D-16 carries both
  readings.
- The host carries GCC 16.2.1 as `/usr/bin/c++` and clang 23.1.3, lcov
  2.5.0-beta working, `doxygen`, `/usr/bin/python3` with PyYAML, and no
  passwordless elevation.
- The lane trees exist as a starting state: the release tree, `build/analyzer`,
  `build/sanitize`, `build/tsan`, `build/coverage`, `build/consumer-release` and
  `build/consumer-release-dev`, each configured and built at a head earlier than
  this base. T001 reads each of them before T002 removes every tree that carries
  a `CMakeCache.txt` and configures it again, so no figure in this feature comes
  from a tree that stands stale.
- The root surface stays closed: the tracing file system and the machine
  specific register files stay owner-only here.

## Assumptions

- The owner order of 2026-11-10 is the authority for scope and numbering, and
  its `ICR-` identifiers map onto the `FR-` identifiers in the sections above.
- A wall time measured on this host reads differently on a hosted runner, so
  each row carries its denominator and its shape.
- The four named entries stay the only reds of the sanitizer and coverage lanes
  at the base. The lane rows at
  `specs/048-hwlat-probe/results/acceptance-final-intel.md:1636-1649` read 2
  failed of 185 in `sanitize`, 3 of 186 in `tsan` and 4 of 185 in `coverage`, and
  the ceiling controls at `:1651-1666` name the entries those failures belong
  to.
- The record convention of these files holds: a figure names its head, a
  citation names a handle, and a blocked row names its operator action.
- The owner accepts the one-file fence widening of D-7. Reading 2 there
  stands recorded so one revert returns to it.

## Dependencies

- PR #103, which landed the second-pass figures this phase repairs beside.
- specs/048-hwlat-probe rulings 30, 35, 36, 37, 38, 39 and 40, which govern the
  witness, the head label, the fence, the blocked rows, the push, the body and
  the one-hash rule.
- specs/002-prose-commit-lint for the prose gate, and specs/030 D-14 for the
  branch allowance of 21.
- `test/simulation_mark_shape.sh` and `test/counters_tsc_read_shape.sh` as the
  shape the new gate copies.

## Citations

- `specs/048-hwlat-probe/results/acceptance-final-intel.md`, the sections
  `Second pass figures at the final head (2026-11-09)` and
  `Corrections to the sections above (IC-006)`.
- `specs/048-hwlat-probe/spec.md`, `Rulings - this phase (29 to 42)`, rulings
  30, 35 to 40.
- `include/speedgun-ng/dbc.hpp:463`, `cmake/variables.cmake:28`,
  `cmake/dbc-gate.cmake`, `tools/dbc/coverage_gate.sh:108`.
- `source/harness/compare.cpp:1156`, `source/harness/jitter_probe.cpp:1153`,
  `source/harness/pass_plan.cpp:90`, `source/harness/host_record_check.cpp:443`.
- `test/simulation_mark_shape.sh`, `test/counters_tsc_read_shape.sh`,
  `test/tier_labels.sh`.

## Follow-on work

- The root surface stays closed here. It needs an operator who holds the
  right, and no feature supplies that right.
- The perl-GD picture step needs one package install by an operator, which
  closes the picture row.
- A future feature can widen the branch allowance or retire it. That is a
  maintainer decision beside a constitution amendment, and this phase takes
  none.

## Open questions

None stand open. D-3 and D-7 each record a rejected reading so a reader sees
the alternative the owner did not take.

## Constitution Checklist

- Main-page impact check (IV.1): no. This phase changes no user-visible
  behavior, no option row and no public header, so the main page, its
  `example/` regions and its quick-reference rows stay unchanged. The
  page-drift gate still runs at the tip, because one changed file stands inside
  the `example/` tree it scans.
