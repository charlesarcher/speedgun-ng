# Feature Specification: Fix Merged Features

**Feature Branch**: `032-fix-merged-features`

**Created**: 2026-10-27

**Status**: Draft

**Input**: User description, opening quoted:

> Fix the merged features, item N0c of the roadmap. Three PRs merged on
> master unfinished: PR #46 (N13a jitter probe, spec 027) at 5fe94fb,
> PR #49 (N7 follow-up, spec 028) at 0783dcd, PR #47 (N14a hunter core,
> spec 029) at 4f701ae. Close each open item of those PRs. After this
> change, all CI jobs pass. Coverage is 100% for lines and for
> branches.

## Spec identity and run-time binding

- Spec number: `032`. `specs/` on master `500704bb757c1bb14a1457b792608deecc505266` holds `001` through `030` (read 2026-10-27). `git branch -a` names no ref carrying `032` or `fix-merged-features`. `031` is reserved for the host-validation feature, and the request fixes the order `030`, `032`, `031`. The `gh pr list` check did not run: `gh` carries no token in this session, so the PR list stands on the request's record (PR #51 is the newest merged head) and the directory and ref checks stand in for it. The number `032` is free, so no renumbering applies.
- Branch: `032-fix-merged-features`, cut from master `500704bb757c1bb14a1457b792608deecc505266` on 2026-10-27. The request names that hash as the base, and master stands on it at branch time, so no moved-base record applies.
- Version: `CMakeLists.txt:15` holds `VERSION 0.20.0`, `CMakeLists.txt:298` holds `SOVERSION 12` for the harness target, and `CMakeLists.txt:80` holds `SOVERSION 4` for the counters archive target. The lane takes the next free version above `0.20.0`. A change with no public-header edit takes `0.20.1`. The one candidate public-header edit is a new `SkippedKind` value (`include/speedgun-ng/hunt.hpp:158`); that edit takes `0.21.0` with harness `SOVERSION 13`. The archive `SOVERSION` stays `4`. The ABI check is recorded beside the version move, in the commit that moves it (FR-001, FR-002).
- Option values: no option row is added and no value moves. The merged lanes keep their blocks: `260` to `269` the jitter-probe family, `270` to `279` the hunter family, `290` to `319` the command-line family (FR-003).

## Audit point

- Repository: `master` at `500704bb757c1bb14a1457b792608deecc505266` (project version `0.20.0`, report schema `1.5`, harness `SOVERSION 12`, archive `SOVERSION 4`), read 2026-10-27 on the lane host.
- The first push run of that head is `40438223916`. The request records `lint`, `prose-lint`, `dbc-gate`, `shared-audit` and `downstream-consumer` as passed on it, and the PR #51 heads as green through `test`, `sanitize`, `tsan` and `test-rocky` after the 030 fixes. The job logs are unread in this session (`gh` carries no token), so reading the full run precedes the work (FR-064), and each check item is confirmed against the run before it closes.

## Baseline

### A. The reading the request carries

- Three pull requests merged on master unfinished: PR #46 (spec 027, the jitter probe) at `5fe94fb`, PR #49 (spec 028, the command-line follow-up) at `0783dcd`, and PR #47 (spec 029, the hunter core) at `4f701ae`. Master then took `7d8dfdd` (the shadow rename), the PR #50 reverts and README trim, and PR #51 (spec 030).
- The item sources the request names (the PR 46 fixes-6 and fixes-7 prompts, the PR 49 fixes-2 prompt, the PR 47 fixes-1 prompt, the review B r2 report, and the four `sg-merge` logs) are absent from `/workspace` on this host. The request body carries each item's text, and that body is the item source this spec encodes. The file:line references those sources carry are stale; each one is re-measured against master here, and the measurements stand in section B and the PC records.
- The merge resolved the PR 47 rebase and its version moves; the request narrows that residue to one check: the versions agree in each file (FR-004).
- The landed PR 47 history carries commit-footer and prose findings (74 of 165 commits without the footer shape the rules name); history stands as it is (D-15), and the rules bind each new commit (FR-063).
- Spec 030 hands four check items to this spec (`specs/030-ci-speedup/spec.md`, section "Items that spec 032 must check"). The hashes that section names are branch hashes: `9ddb50f`, `c491337`, `9e82e0c` and `744b132` are ancestors of no master commit. The master spellings, measured 2026-10-27, are `d7e22c2` (format), `658af81` (schema anchor), `1c47e66` (hunt rows) and `502d42b` (affinity cpus).

### B. Re-measured on the master tree

Measurements of 2026-10-27 on `500704bb757c1bb14a1457b792608deecc505266`:

- `README.md` measures 835 lines, inside the 835 cap, and `PAGE_LINE_CAP` stands once at `tools/docs/example_drift.py:192` (835).
- `docs/schema/report-v1.json:7` and `:8` carry two top-level `x-order` keys. The writer emits `jitter` (`source/harness/json_report.cpp:590`) before `benchmarks` and before `hunt` (`:676`), so the merged order `["schemaVersion", "speedgun", "run", "jitter", "benchmarks", "hunt"]` matches the writer's order.
- The scan `setenv|GCOV_PREFIX|sgcov` over `test/source` returns nothing.
- The scan `online (cpu )?count`, case-insensitive, over `specs/027-jitter-probe`, `include/` and `source/harness` returns nothing.
- The online-set seam stands: `installTestOnlineCpus` and `onlineCpuSet()` at `source/harness/topology.cpp:327` to `:366`, read by the execution-context bound (`source/harness/execution_context.cpp:655`) and by the option-value parse bound (`source/harness/jitter_probe.cpp:711`). The unreadable-line fallback fills the set by index from `onlineCpuCount()` (`topology.cpp:358` to `:365`); that fallback is this feature's target (FR-021).
- The coverage keep-set stands on master: five `--omit-lines` patterns at `cmake/coverage.cmake:92` to `:98` and a branch allowance of 19 at `tools/dbc/coverage_gate.sh:60`. The two comments at `cmake/coverage.cmake:57` and `:85` to `:86` carry a person's name and a decision record.
- The defect texts stand: `batchProbed` at `source/harness/cli.cpp:776` to `:777` reads `options.jitterProbe.has_value() && familyProbes.empty()`; `source/harness/compare.cpp:1906` to `:1907` prints `entry.name` and `entry.ruleText` raw; the debug print commit `e781eda` ("test: Print the segment the fold check fails on") is an ancestor of master.
- The requirement-language targets stand: `specs/028-full-command-line/spec.md:1127`, `specs/029-hunter-core/spec.md:62`, and `specs/029-hunter-core/plan-review-2.md:12`, `:39` and `:80`.
- The idle-preset fact holds: `source/harness/cli.cpp:805` to `:818` prints one line naming the preset word and returns 2 when no cpu is free (the FR-017 fact check; recorded with SC-007).

## Preconditions

- PC-1: the shadow rename is on master: `7d8dfdd` "Harness: Rename hunt retry pass parameter". PASS.
- PC-2: the mutation anchor is on master: `658af81` "test: Anchor the mutation at schema version 1.5"; the harness writes schema `1.5`. PASS.
- PC-3: the block-order fixes are on master: `1c47e66` "test: Order the hunt rows below the reserved block" and `d7e22c2` "test: Format the CLI table test". PASS.
- PC-4: the trim is on master: `cd2f2f1` "Docs: Trim the main page under the cap" and `f507b8f` "Docs: Restore jitter probe facts"; `README.md` measures 835 lines. PASS.
- PC-5: the affinity-cpu scenario is on master: `502d42b` "test: Take the jitter cpus from the affinity mask". PASS.
- PC-6: the twelve reverts are on master, each found by `git log --grep` on its reverted hash: `9b176ff` (`00e5848`), `e33d98a` (`a7e7ce5`), `25b7af2` (`c5d7a5f`), `5d4bb2d` (`ea75686`), `9ee9be0` (`e9a3907`), `80ef590` (`5fe94fb`), `334a092` (`45d5e42`), `aa3b477` (`f82406e`), `b0d0f9b` (`6113643`), `7bbb68c` (`dcbdfa6`), `c4203a4` (`3f3b56c`), `5dc31e7` (`872086c`). PASS.
- PC-7: the rank-mark move is on master: `42ba692` "Harness: Move the rank mark out of the macro", with the helper at `source/harness/hunt_plan.cpp:97`. PASS.
- PC-8: the coverage keep-set of the request stands on master, as Baseline B measures. PASS.
- PC-9: the first-pass retry item of the PR 47 review is closed; the hunt runs one pass on master. PASS, held by FR-037 at implementation.
- PC-10: the option-file defect stands: an option-file `jitter-probe=` value overrides a family's own probe today, and the precedence of D-01 is owed. OPEN, closed by FR-028.
- PC-11: `gh` carries no token in this session, so the job logs of run `40438223916` are unread. OPEN, closed by FR-064 when a token stands.

## Roadmap position

- Item N0c of the harness roadmap. Spec 030 (the CI speed-up) is merged; spec 031 (host validation) follows this feature. The order is `030`, `032`, `031`.

## Decisions

Decisions the request fixes for this feature, recorded as requirements:

- **D-01: one precedence for the probe value.** A family's own probe wins over both a command-line value and an option-file value. An option-file `jitter-probe=` value carries the same standing as a command-line value. When the command line and the option file both carry a value, the command-line value stands, the usual source order. A test covers each source (FR-028).
- **D-02: the online-set fallback reads the affinity mask.** When `/sys/devices/system/cpu/online` cannot be read, `onlineCpuSet()` answers the process affinity mask. A count is forbidden as the fallback source. The change is tested through the seam with a set that carries a gap, and both bounds are checked (FR-021).
- **D-03: the execution-context bound is a membership check.** A placement cpu is checked for membership in the online cpu set (FR-014).
- **D-04: duplicate cpus exit 2.** A probe cpu list with a duplicate value, given as a list (`0,0`) or inside a range (`0-1,1`), prints one line quoting the value and exits 2 (FR-029).
- **D-05: an unresolvable counter address exits 2.** A `--counter` address this host cannot resolve exits 2 with one quoted line, from every source, including a valid PMU path this host lacks (FR-032).
- **D-06: an uncountable leaf exits 2.** A leaf the catalog lists but this host cannot count exits 2 (FR-033).
- **D-07: an ambient value blocks a hunt.** An ambient `SPEEDGUN_COUNTER`, `SPEEDGUN_METRIC` or `SPEEDGUN_PLACEMENT` value stops a hunt with exit 2 and one quoted line that names the source (FR-034).
- **D-08: the hunt and the probe run together.** `--hunt` with `--jitter-probe` is supported, with a test and a docs row (FR-035).
- **D-09: a new `SkippedKind` value may join.** The public enum takes a value when an item needs one, and the ABI check records the harness `SOVERSION` move (FR-002, FR-036).
- **D-10: one pass.** The hunt's first pass carries no retry (FR-037).
- **D-11: the seam stays public.** The `FakeProvider` open-failure seam stays public in the counters archive, and the archive `SOVERSION` stays `4` (FR-038).
- **D-12: the coverage keep-set stands.** The five `--omit-lines` patterns (`cmake/coverage.cmake:92` to `:98`) and the branch allowance of 19 (`tools/dbc/coverage_gate.sh:60`) stay in force. After FR-021 lands, the pull-request comment lists each pattern and each allowance row that no longer matches code, with the coverage trace and the zero list as evidence. A pattern or a row is removed only after the request carries the approval; the allowance moves neither down nor up without one (FR-026).
- **D-13: exception-cleanup sides carry their proof.** A branch side enters the accepted list only when the coverage job's compiler proves it, through `gcov` block and arc data, to be an exception-cleanup side that no real input reaches. The spec names that compiler and lists each accepted side with its data. The one gate change such a proof buys is the narrowest `lcov` option that removes exactly the proven sides, in `cmake/coverage.cmake` alone. Every other filter change waits for a separate approval (FR-031).
- **D-14: a real input is a witness, and the code stays as it is.** A test may feed a real input that reaches a branch inside library code, for example a path long enough to move a `std::string` past its short buffer. That input is a test witness; no code is shaped to move a count (FR-031, FR-052).
- **D-15: history stands.** The landed PR 46, PR 47 and PR 49 history is accepted as it is. No rewrite. The rules bind each new commit (FR-053, FR-063).
- **D-16: the caps stay.** `README.md` keeps the 835-line cap, and each README section keeps the 25-line cap (FR-062).

## Out of scope

- The host-validation feature (spec 031), its `tools/hostcheck/` and `validation/` trees, and the per-CPU-type hunter acceptance rows: the acceptance run the PR 47 prompt asked for moves there (FR-041).
- No feature is added and no feature design changes. Each item closes a defect of the three merged features or checks a fix master carries.
- No CI file changes without approval: `.github/workflows/ci.yml`, `CMakePresets.json`, `cmake/coverage.cmake`, `cmake/lint.cmake`, `cmake/spell.cmake`, `tools/dbc/`, `tools/prose/`, `tools/docs/example_drift.py`, `.codespellrc`. The two exceptions: the narrow `lcov` option D-13 proves, and a missing revert, which stops and reports first.
- No new `--ignore-errors` entry, no new `LCOV_EXCL` marker, no waiver, and no code shaped for coverage. The D-12 keep-set is the one standing exception, and it stays until the approval D-12 names.
- No history rewrite of the landed commits (D-15).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A push goes green end to end (Priority: P1)

The maintainer pushes the branch. Every CI job passes on the head: builds, tests, sanitizers, thread sanitizer, analyzers, the coverage gate at 100% lines and branches, the drift and prose gates. The merged features' defects are closed, so the reds the merge carried are gone.

**Why this priority**: the request's own goal: after this change, all CI jobs pass, and coverage is 100% for lines and branches.

**Independent Test**: the CI run of the pushed head reports success on every job, and the coverage gate reports 100% lines and 100% branches with only the accepted sides.

**Acceptance Scenarios**:

1. **Given** the branch pushed, **When** the CI run of the head finishes, **Then** every job reports success.
2. **Given** the coverage gate runs, **When** it reads the trace, **Then** the line metric reads 100.0% and the branch shortfall holds only the accepted sides of D-13 and the keep-set of D-12.

### User Story 2 - One precedence for the probe value (Priority: P1)

A user passes `--jitter-probe` on the command line, or through an option file, or through a suite call that carries a family's own probe. The value that wins follows one rule regardless of the source, and a family's own probe wins over both file and command line.

**Why this priority**: the three sources disagree today, so the same inputs behave differently by source order; a benchmark result depends on it.

**Independent Test**: three runs that differ only in the source of the probe value produce the same probe placement, and a family probe overrides both other sources.

**Acceptance Scenarios**:

1. **Given** an option file that carries `jitter-probe=`, **When** the batch runs, **Then** the value stands as a command-line value would.
2. **Given** a family that carries its own probe and an option-file value, **When** the batch runs, **Then** the family's own probe runs.
3. **Given** the command line and the option file both carry a value, **When** the batch runs, **Then** the command-line value stands.

### User Story 3 - A hunt refuses ambient interference and pairs with the probe (Priority: P2)

A user runs `--hunt` on a host where `SPEEDGUN_COUNTER`, `SPEEDGUN_METRIC` or `SPEEDGUN_PLACEMENT` holds a value. The run stops with one line that names the source. A hunt combined with `--jitter-probe` runs, and the combination refusals work from every source.

**Why this priority**: an ambient value silently changes what a hunt measures; the combined mode is a documented gap.

**Independent Test**: a run with one ambient variable set exits 2 with one quoted line, and a hunt with the probe completes and reports both records.

**Acceptance Scenarios**:

1. **Given** `SPEEDGUN_METRIC` holds a value and `--hunt` is given, **When** the run starts, **Then** one line names the source and the exit status is 2.
2. **Given** `--hunt` with `--jitter-probe`, **When** the run completes, **Then** the report carries both the hunt and the jitter records.

### User Story 4 - The compare table quotes user text (Priority: P2)

A user compares two runs whose benchmark names or rule text carry characters the table grammar uses. The table prints each such cell quoted, so the table stays parseable.

**Why this priority**: a raw name with a separator breaks the table for every row that follows it.

**Independent Test**: a compare run whose baseline carries a name with a quote or a separator prints the cell quoted, and the reason cell stays unquoted.

**Acceptance Scenarios**:

1. **Given** a benchmark name that carries a quote character, **When** the compare table prints, **Then** the name cell is quoted once.
2. **Given** a rule text with the same shape, **When** the table prints, **Then** the rule-text cell is quoted once, and the reason cell is unchanged.

### User Story 5 - The docs agree with the code (Priority: P3)

A reader opens the docs of the three features. Each option, key and exit status in the docs matches the code, the hunt samples show the version of the build that recorded them, and the JSON schema carries one key order.

**Why this priority**: a sample recorded by an older build teaches a version the reader does not run.

**Independent Test**: the docs sweep of FR-018 reports zero disagreements, and the two hunt samples carry the current version.

**Acceptance Scenarios**:

1. **Given** `docs/pages/harness.md:1509` and `example/docs/hunt_rank.expected:2`, **When** the samples are re-recorded, **Then** both name the current build version.
2. **Given** a document with a duplicate key, **When** the schema test runs, **Then** the test fails.

### User Story 6 - The comments and the specs carry requirements only (Priority: P3)

A contributor reads the specs and the comments the three PRs added. Each carries requirement text: no person's name, no acceptance wording, no host name, and the coverage comments say what the code does.

**Why this priority**: an approver name inside a file outlives the approval and misleads the next reader.

**Independent Test**: a scan over the files `git diff --name-only fea69c5 4f701ae` names finds no person's name and no acceptance wording in the rewritten targets, and the two `cmake/coverage.cmake` comments name no person.

**Acceptance Scenarios**:

1. **Given** the target files of FR-024, **When** each named line is rewritten, **Then** the text states the requirement and names no person and no host.
2. **Given** `cmake/coverage.cmake:57` and `:85` to `:86`, **When** the comments are rewritten, **Then** each comment says what the code does.

### Edge Cases

- The command line and the option file both carry a `jitter-probe=` value: the command-line value stands (D-01).
- The online-cpu line cannot be read on a host whose affinity mask carries a gap: the set answers the mask, and both bounds check membership against the gap (D-02).
- A duplicate cpu that spans a range boundary (`0-1,1`): one line quotes the value and the exit status is 2 (D-04).
- A valid PMU path this host lacks: exit 2 with one quoted line, from every source (D-05).
- A hunt with an ambient value in more than one variable: the run stops on the first, and the line names that source (D-07).
- A hand-edited report document with a duplicate key: the schema test fails (FR-017).
- A test host with 2 cpus: the suite passes under `taskset -c 0,1` (SC-004).

## Requirements *(mandatory)*

### Version, ABI and option lanes (A)

- **FR-001**: The commit that moves the project version shall record the ABI check beside the move: with no public-header edit the version is `0.20.1`; with the `SkippedKind` addition the version is `0.21.0` and harness `SOVERSION` is `13`.
- **FR-002**: A `SOVERSION` shall rise only when that target's public ABI changes, the archive `SOVERSION` shall stay `4`, and a `SOVERSION` shall never go down (D-11).
- **FR-003**: No option row shall be added, no option value shall move, and each value shall stay in its lane block (`260` to `269`, `270` to `279`, `290` to `319`).
- **FR-004**: The versions shall agree in each file the merge touched: the project version, the report schema version, both `SOVERSION` lines, and the version figures in the docs samples.

### Check items closed on master (B)

Each FR in this group closes with the file:line evidence and the commit hash, recorded in the spec's results record and in the pull-request comment. A check the master tree does not hold reopens as a fix under group (C).

- **FR-005** (M-1): the shadow rename stands (`7d8dfdd`), and the build of each CI preset passes.
- **FR-006** (M-2): the mutation test anchors at schema `1.5` (`658af81`), reads the version once, and no `1.4` figure remains in the test.
- **FR-007** (M-3): the hunt-block rows stand (`1c47e66`, `d7e22c2`), the test passes, and `clang-format 18.1.8` reports no change.
- **FR-008** (M-5): the trim stands (`cd2f2f1`, `f507b8f`), `README.md` holds 835 lines, `PAGE_LINE_CAP` stands once, and the drift gate passes.
- **FR-009** (M-8): the affinity-cpu scenario stands (`502d42b`); the test passes under `taskset -c 0,1`, `taskset -c 0-3` and the full mask, with no `SKIP_RETURN_CODE` on the entry and `test/tier_labels.sh` at master's eight rules.
- **FR-010** (M-9): the lint job of run `40438223916` passed with the spell step included, confirmed on the full run; the spell gate needs no CI change.
- **FR-011** (M-14): each of the twelve reverts is on master, found by `git log --grep` on its hash (PC-6); the suite passes on master as it stands.
- **FR-012** (M-16): the `setenv`, `GCOV_PREFIX` and `sgcov` scan over `test/source` returns nothing, and no test calls `setenv`.
- **FR-013** (M-18): the rank mark stands outside the macro (`42ba692`, `hunt_plan.cpp:97`); no lambda stands inside an `SG_*` macro in `source/` or `include/`, and the coverage target reports no `inconsistent` error.
- **FR-014** (M-11 check half, D-03): the online-set seam stands (`topology.cpp:327` to `:366`) and both bounds read it (`execution_context.cpp:655`, `jitter_probe.cpp:711`); the tests install a set with a gap, for example `0-3` and `8-11`, and check both bounds; the old count-rule text scan returns nothing; the probe-cpu affinity-mask rule at `contracts/cli-options.md:71` stays its own rule.
- **FR-015** (PR 46 P-6): the duplicate-cpu requirement stands with its three records: the spec row (`specs/027-jitter-probe/spec.md:598` to `:601`), the grammar row (`contracts/cli-options.md:28`), and the error-table row (`:79`).
- **FR-016** (review B 9): no hunt row carries `fromEnvironment`; the option table is the evidence.

### Master defects (C)

- **FR-017** (M-4): `docs/schema/report-v1.json` shall carry one top-level `x-order` key, `["schemaVersion", "speedgun", "run", "jitter", "benchmarks", "hunt"]`; the writer order shall be checked against it; a test shall fail when a document carries a duplicate key.
- **FR-018** (M-6): the hunt samples shall be re-recorded from the current build (`docs/pages/harness.md:1509`, `example/docs/hunt_rank.expected:2`); every other sample shall stay unchanged; a sweep over the three features' docs shall check each option, key and exit status against the code, and each disagreement shall carry its fix or its pull-request question.
- **FR-019** (M-7): the scan step and the settle step of `--jitter-probe` shall run one code path; the behavior shall stay; each behavior that stays shall carry a test.
- **FR-020** (M-10): the `loop_shape` failure under GCC 14 shall be reproduced with the Rocky toolchain, the cause shall be named, and the fix shall land in the code or in the test; no compiler check shall join; the `test` job (GCC 15.2.0) shall still pass.
- **FR-021** (M-11 fix, D-02): the unreadable-line fallback of `onlineCpuSet()` shall answer the process affinity mask, tested through the seam with a set that carries a gap, and both bounds shall be checked.
- **FR-022** (M-12): the family-probe intermittent failure shall be root-caused and fixed; no retry, no sleep, no timeout, and no debug print shall join; the `e781eda` print shall be removed in the fix commit.
- **FR-023** (M-15): each of the 38 conflict blocks the PR 47 rebase resolved by keeping both sides shall be read against master and shall stand correct or be fixed; the eight files the merge commit `4f701ae` did not repair shall be named in the record.
- **FR-024** (M-17, PR 47 P-4, review B 24 and 25): the files PR 46, PR 47 and PR 49 added or changed (`git diff --name-only fea69c5 4f701ae`) shall carry requirement text with no person's name, no acceptance wording and no host name; the named targets of Baseline B shall be rewritten; `LICENSE`, `CODEOWNERS` and `CMakeLists.txt` shall stay untouched.
- **FR-025** (M-19): the two comments at `cmake/coverage.cmake:57` and `:85` to `:86` shall be rewritten as what the code does, with no name and no decision record; no other change in that file shall ride the commit.
- **FR-026** (M-20, D-12): the coverage keep-set shall stand; after FR-021 lands, the pull-request comment shall list each pattern and each allowance row that no longer matches code, with the coverage trace and the zero list as evidence, and shall ask before any removal.
- **FR-027** (PR 46 P-3): the batch-probe arm reachable through a family path shall be covered by tests; code no input can reach shall be deleted, and the reason shall stand in the commit body (the arm runs only while `familyProbes` is empty).
- **FR-028** (PR 46 P-7, D-01): the precedence shall follow D-01, with one test per source: the command line, the option file, and the family call.
- **FR-029** (D-04): the duplicate-cpu exit shall stand with a test for the list shape (`0,0`) and the range shape (`0-1,1`), each printing one line that quotes the value and exiting 2.

### The compare table and the counters (D, PR 49)

- **FR-030** (U-2): the name cell and the rule-text cell shall be quoted with `detail::quotedText`, once, at the print site; the reason cell shall stay unquoted.
- **FR-031** (U-1, D-13, D-14): the zero list shall be re-taken from the compiler the coverage job uses on the branch head, and that compiler shall be named in this spec; a side a real input reaches shall get a test; a side D-13 accepts shall be listed here with its `gcov` block and arc data; the gate change D-13 allows is the only one; when no narrow filter removes exactly the proven sides, the item shall stop and the data shall go to the pull request as a question.
- **FR-032** (D-05): the counter-address rule shall stand with a test from each source, including a valid PMU path this host lacks.
- **FR-033** (D-06): the uncountable-leaf rule shall stand with a test.

### The hunt and the probe (E, PR 47)

- **FR-034** (D-07): the ambient-value block shall stand with one test per variable and one quoted line that names the source.
- **FR-035** (D-08): the hunt and probe combination shall run, with a test and a docs row.
- **FR-036** (D-09): the `SkippedKind` addition shall land when an item needs it, with the recorded ABI check of FR-001 and FR-002.
- **FR-037** (D-10): the hunt's first pass shall carry no retry, confirmed against master.
- **FR-038** (D-11): the seam-publicity evidence shall stand in the record, and the archive `SOVERSION` shall stay `4`.
- **FR-039** (PR 47 P-5): internal flag parameters shall use options structures.
- **FR-040** (PR 47 P-8): contract-only helpers shall carry `[[maybe_unused]]`.
- **FR-041** (PR 47 P-9): the hunter acceptance run the PR 47 prompt asked for shall move to the host-validation feature (spec 031); this feature shall record the move and close the item.
- **FR-042** (PR 47 P-10 to P-18): each item shall be confirmed against master and closed with evidence; an item the master tree does not settle shall go to the pull request as a question and shall stop on its own commit.
- **FR-043** (review B 10): the hunt parsers' process exit shall be checked against master and closed with evidence or fixed with a test.
- **FR-044** (review B 11): `--hunt=none` and the source order rule shall stand with tests.
- **FR-045** (review B 12): a refused combination shall be refused from every source; the environment and the option file shall each get a test.
- **FR-046** (review B 13): the hunt with `--interleave`, `--threads` and `--jitter-probe` shall stand with tests.
- **FR-047** (review B 18): the raw-code rule with globs shall stand with tests.
- **FR-048** (review B 20): the `ABSENT` relabel abort shall stand with a test.
- **FR-049** (review B 21): the real-PMU test item shall be closed against the host's catalog, or the gap shall be named in the pull request.
- **FR-050** (review B 22): the `--hunt-noise` grammar shall stand with tests.
- **FR-051** (review B 23): a failing device lookup shall stand with a test.
- **FR-052** (PR 47 P-3, D-14): coverage shall reach 100% including `detail/hunt_selection.hpp` and the glob star arm, through real inputs and tests.

### Process and gates (F)

- **FR-053**: The commit footer shall read exactly `Refs: specs/032-fix-merged-features` then `Approved-by: OmO`, in that order, with no blank line between them, and no e-mail form anywhere in the footer.
- **FR-054**: A subject shall hold 50 characters or fewer, each body line 72 or fewer, and each body shall say only what the code does (PR 46 Q-3).
- **FR-055**: Each fix shall ship with its test, written first and seen failing, in the same commit.
- **FR-056**: No test shall call `setenv` or `clearenv`; a child shall receive a variable through its environment array.
- **FR-057**: No lambda shall stand inside an `SG_*` macro.
- **FR-058**: Text from a user shall never make a contract fail.
- **FR-059**: No CI file shall change without approval; the Out of scope list and its two exceptions state the boundary.
- **FR-060**: The branch shall rebase only, push once with `--force-with-lease`, and prove the push with `git ls-remote`.
- **FR-061**: Each figure in the records shall come from the CI run of the last pushed commit, and each job log shall be read.
- **FR-062**: Each behavior change shall ship with docs and an example; each README section shall hold 25 lines or fewer; the page cap shall stay 835 (D-16).
- **FR-063**: Each new commit shall pass `prose-lint` (M-13).
- **FR-064**: The full CI run of the base head shall be read before the work starts; with no token in the session, the token is requested first.
- **FR-065**: One pull-request comment shall list each item with its commit or its question, the coverage numbers, and the CI run; it shall name no approval, no acceptance and no person.

## Key Entities

- `OptionRow` (`source/harness/detail/options.hpp`): the one option table; this feature adds no row (FR-003).
- `SkippedKind` (`include/speedgun-ng/hunt.hpp:158`): the public enum a new value may join (D-09); its edit moves the version and the harness `SOVERSION` (FR-001).
- The online cpu set seam: `installTestOnlineCpus` and `onlineCpuSet()` (`topology.cpp:327` to `:366`), the readers at `execution_context.cpp:655` and `jitter_probe.cpp:711`, and the affinity-mask fallback (D-02).
- The coverage keep-set: the five `--omit-lines` patterns and the allowance of 19, kept until the approval D-12 names (FR-026).
- The top-level `x-order` list of `docs/schema/report-v1.json`, one key after FR-017.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001** (AC-1): every CI job on the pull-request head passes.
- **SC-002** (AC-2): coverage reads 100% lines and 100% branches; the accepted sides are the D-13 list and the D-12 keep-set, and nothing else.
- **SC-003** (AC-3): each item of the request is closed with a commit and a test, closed with the file:line that shows master already holds the fix, or carried in the pull-request comment as a question.
- **SC-004** (AC-4): the suite passes under `taskset -c 0,1` and on a real 2-cpu host.
- **SC-005** (AC-5): each new commit passes `prose-lint`.
- **SC-006** (AC-6): no CI file changed without approval.
- **SC-007**: the two fact checks are recorded with their evidence: `SPEEDGUN_JSON` matches no environment row (`source/harness/detail/options.hpp:109` to `:115` derives the name from a flagged row, and no row is named `json`; the unmatched-name rule at `options.cpp:1310` to `:1325` leaves it unread), and the idle preset exits 2 with one line when no cpu is free (`source/harness/cli.cpp:805` to `:818`).
- **SC-008**: the docs sweep of FR-018 reports zero disagreements, or each disagreement carries its fix or its question.

## Constraints

The request's hard rules bind this spec; they restate constitution VI, VIII and IX:

- Coverage stays 100% lines and 100% branches; no `LCOV_EXCL` marker, no waiver, no new `--ignore-errors` entry; the exceptions are the D-13 option and the D-12 keep-set.
- No code shaped for coverage; a test witnesses behavior with real inputs (D-14).
- Determinism: no sleep, no wall-clock read, no made-up cpu in a test.
- Tests pass under `taskset -c 0,1` and on a real 2-cpu host.
- A bad value from any source exits 2 with one quoted line.
- No `UNAPPLIED` pin in an exit-0 test.
- No `setenv` or `clearenv` in a test; a child receives a variable through its environment array.
- No lambda inside an `SG_*` macro; `[[maybe_unused]]` on contract-only helpers.
- A `SOVERSION` rises only for a public ABI change, and the archive `SOVERSION` stays 4.
- The JSON libraries the repo imports are the only ones a change uses; each flag set that grows uses an options structure.
- Rebase only; no merge commits; push once with `--force-with-lease`.
- Each new commit carries the footer of FR-053, passes `prose-lint`, and names no person in a file, a commit body or a comment.

## Assumptions

- When the command line and the option file both carry a `jitter-probe=` value, the command-line value stands; the request fixes the family's own probe above both and leaves that pair to the usual source order (D-01).
- The detail texts of PR 47 items P-10 to P-18 lived in the PR 47 fixes-1 prompt, which is absent from this host. Each item is confirmed against master, and an item the master tree does not settle becomes a pull-request question (FR-042).
- The coverage job builds with the toolchain the reverts of PC-6 restored, so FR-031 re-takes the zero list from that compiler and names it.
- `gh` reading needs a token supplied in the session; the request's job-state record stands until then (PC-11).

## Constitution Checklist

- Main-page impact check (IV.1): the hunt samples of FR-018 change two recorded outputs; the page regions and the drift gate are updated in the same commits. No quick-reference row changes, so no `example/` snippet region moves beyond the recorded outputs.
- Principles VI and VIII: the coverage bar, the determinism rules and the gate set are restated as binding requirements (Constraints), because the items move the ground the gates stand on.
- Principle IX: the full workflow applies; the change touches behavior (exit codes, precedence), possibly a public header (`SkippedKind`), and docs.
- Principle X.1: the request fixes every open decision; the one open reading (command line against option file for the same value) is resolved and recorded in D-01 and in Assumptions.
- Principle X.3: each commit touches the files its item names, and nothing else.
- Principle XI: this file holds no em-dash, no contrastive framing, no filler vocabulary and no vouching word, and every command and option sits in a code span.

