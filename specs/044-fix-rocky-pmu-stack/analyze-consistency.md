# Cross-Artifact Consistency Analysis: `specs/044-fix-rocky-pmu-stack`

Date: 2026-11-01. Method: the `/speckit.analyze` pass over `spec.md`,
`plan.md`, `tasks.md`, `research.md`, `data-model.md`,
`contracts/move-contracts.md`, `quickstart.md`, `diagnosis-b2.md`, and
`checklists/requirements.md`, against `prompt.md` and the constitution
(IX binds the artifacts, XI the prose). The base for every line check is
`d454bcff`, the branch head this analysis ran at. Severity labels:
`blocker` stops `/speckit.implement`, `major` misleads implementation or
fails a gate until edited, `minor` is precision work.

Evidence base: every cited line number was read back from the files at
`d454bcff`; the presets `ci-rocky`, `ci-sanitize`, and `ci-linux-ignore`
were read back from `CMakePresets.json`; the coverage allowance was read
back from `tools/dbc/coverage_gate.sh` (`allowance=21` at line 102); the
Rocky package list was read back from `.github/workflows/ci.yml`
(`DNF_PACKAGES`, line 594); the open pull request #68's file set was
fetched from `refs/pull/68/head` (`7c00ee6c`) and diffed against
`d454bcff`: 45 files, zero overlap with the six planned source files.
The prose gate itself checked nothing at this head (the artifacts stand
uncommitted, so its range is empty); the scan below ran the XI
vocabularies over the artifact text directly.

## Findings

### B1 - blocker - `tasks.md` T022, `research.md` risk 3

The install list omits one gate-body run. The run at
`test/source/harness_jitter_window_test.cpp:941` (the split run that
writes `harness_jitter_window_split.json`) passes no `--filter`, so the
`bmGate` and `bmPassGate` bodies execute under its probe. After T023
re-targets the waits, both bodies read the counting reader's accessor;
no reader stands installed at that run, the accessor reports a failure,
and the suite fails at that scenario. The list's first entry, `:723`,
names a comment line; the roundtrip run's `captureRun` stands at
`:741`.

Remediation: in T022, replace the site list with
"`:741`, `:941`, `:1004`, `:1044`, `:1081`, `:1260`, `:1268` at the
base"; in `research.md` risk 3, replace the run list with the same
seven sites and keep the two-cpu note for `:741`.

### M1 - major - `contracts/move-contracts.md` (the paragraph before "Move constructor")

The file reads: "The definitions stand beside the destructor in the
seam header, where `fastContextClose` is declared". This contradicts
`research.md` (the header-inline placement stands rejected),
`plan.md` (definitions out of line in `fast_read.cpp`), and T011 (out
of line, outside the platform branch). An implementer who follows the
contracts puts the definitions in the wrong file.

Remediation: replace the sentence with: "The declarations stand beside
the destructor in the seam header; the definitions stand out of line in
`fast_read.cpp` beside `fastContextClose`, and the release reaches the
one release path (specs/012 FR-013)."

### M2 - major - `spec.md` FR-001, `plan.md` Constitution Check II

FR-001 reads: "Each new move operation carries `\pre`/`\post`
contracts checked with `SG_ENSURE`". The move assignment's
different-values precondition is enforced with one `SG_REQUIRE` per
`contracts/move-contracts.md` and per `plan.md` II; `SG_ENSURE` covers
the postconditions alone. `plan.md` II's "one doxygen `\pre`/`\post`
pair" also names one `\post` row where the contracts carry two.

Remediation: FR-001 becomes: "Each new move operation carries the
`\pre` and `\post` rows of `contracts/move-contracts.md` (Principle
II): the moved-from postcondition is enforced with `SG_ENSURE`, the
move assignment's different-values precondition with `SG_REQUIRE`, and
the contract pair gate stays green." `plan.md` II becomes: "each new
move operation carries the doxygen `\pre` row and `\post` rows of
`contracts/move-contracts.md` in the seam header, enforced with
`SG_ENSURE` beside the out-of-line definition; the move assignment's
different-values precondition carries one `SG_REQUIRE` with its
reason."

### M3 - major - foreign citations stand unqualified in six artifacts

`spec.md:87` (`FR-040`), `spec.md:66` (`FR-015`), `tasks.md:187`
(`R-01`), `tasks.md:209` (`FR-028`), `tasks.md:222` (`FR-039`),
`research.md:265/300/321/325/341/347` (`R-02`, `FR-028`, `E-05`,
`FR-007`, `FR-039`, `R-04`, `R-05`, `R-01`),
`data-model.md:50/70/75` (`FR-013`, `FR-015`),
`data-model.md:32/43/44/71` (`FR-024`, `FR-031`, `FR-040`, `FR-045`),
`contracts/move-contracts.md:25/59` (`FR-013`),
`diagnosis-b2.md:12/13/65/66` (`FR-040`, `FR-022`, `FR-029`,
`FR-027`, `E-08`) name other features' numbers with no spec pointer.
The owning features are `specs/012-counters-defect-resolution` and
`specs/027-jitter-probe`. The collision bites: under this feature's own
numbering, `FR-013` names the no-CI rule and `FR-015` names the
coverage rule, so `data-model.md:50`, `data-model.md:70`, and
`contracts/move-contracts.md:25` read as wrong requirements.
`quickstart.md:79` shows the project's own form
("specs/035 FR-013, FR-022").

Remediation: qualify every foreign token with its owning spec
directory, the form `quickstart.md` uses. The parentheticals that mean
"one release path" become `(specs/012 FR-013)` or drop in favor of this
feature's `FR-002`; the "unopened context's no-op" parenthetical at
`spec.md:66` and `data-model.md:75` becomes `(specs/012 FR-015)`; the
`data-model.md:70` default-row citation becomes `(FR-003)`.

### M4 - major - `tasks.md` T002 and T043 disagree on `.specify/feature.json`

T002 commits `.specify/feature.json`; T043's diff scope names only the
six source files and `specs/044-fix-rocky-pmu-stack/`, so the scope
check fails on the file T002 commits. T002's file list also omits this
report, `analyze-consistency.md`, which stands in the directory its
"and nothing else" rule covers.

Remediation: T043's named set gains `.specify/feature.json`; T002's
list gains `analyze-consistency.md`.

### M5 - major - `tasks.md` T046 admits a retry the brief forbids

T046 reads: "a failed lease stops the push with a report and gets
investigated before any retry". `prompt.md` section 5 reads: "If the
lease fails, stop and report. Do not push again."

Remediation: T046 becomes: "a failed lease stops the push with a
report; the executor stops and reports; no retry runs (`prompt.md`
section 5, spec acceptance scenario 3)."

### M6 - major - Principle XI violations the prose gate reports (T040)

Four violations stand in plain prose:

- `plan.md:68` (Constitution Check I): "the tests must adapt to,
  not a deviation." (XI.2)
- `diagnosis-b2.md:12`: the cell's text
  the cell's contrastive text beside an em-dash (XI.1, XI.2: the
  em-dash connector before it, and the contrastive shape)
- `diagnosis-b2.md:48`: the em-dash connector after the
  `maxNs=1000000` code span (XI.1)
- `diagnosis-b2.md:64`: the em-dash connector after "magnitude" (XI.1)

The placeholder cells at `diagnosis-b2.md:11` and `:14` hold a lone
U+2014 each; the gate's em-dash rule may report a content cell, and
`none` is the safe replacement there. `plan.md:144`'s placeholder row
follows the constitution's own table shape.

Remediation edits:

- `plan.md:68` becomes: "is standard-conformant behavior the tests must
  adapt to; the standard states the rule."
- `diagnosis-b2.md:12` becomes: "...so the failure is
  `the mask holds the cpus the scenario asks for (specs/027 FR-040)`;
  the refusal is environmental, and this setup yields no signal about
  the gate."
- `diagnosis-b2.md:48` becomes: the text after the code span turns
  into a colon clause: "the fold's maximum kept gap stays at the first
  pass's jump (1,000,000), while..."
- `diagnosis-b2.md:64` becomes: "...at the pass's magnitude; that
  unblocking is the failure the CI run reported at the gate entry
  check (specs/027 FR-027, E-08)..."

### m1 - minor - line-range imprecision in `tasks.md` and `research.md`

All corrections verified against `d454bcff`:

- T022's "`:96-117`" for the `seriesJump()` / `passGateCalls()`
  pattern: the helpers stand at `:115-123` and `:177-185`.
- T023's "`:146-164`" and "`:166-172`": the pass wait stands at
  `:146-157`, the jump wait at `:158-169`.
- T023's "`:235-241`" and "`:244-250`": the rebase wait stands at
  `:234-242`, the ride wait at `:243-251`.
- T021's "`:535-549`" for `GapSeriesReader`: the class stands at
  `:535-545`.
- `research.md`'s "`:96-98` and `:115-117`" for the pattern: same
  correction as T022; its "`:535-549`": `:535-545`; its
  "`:575-577`" for the cpu list: `:570-572`; its "`:1276-1279`" for
  the p99 checks: `:1279-1282`.

### m2 - minor - `tasks.md` T012 carries no `FR-005` citation

FR-005's content stands inside T012 (the partial-open temporary) and
T011 (the move assignment the deleted copy selection reaches), and
`research.md` cites FR-005. Add "(FR-005)" to T012's partial-open
clause.

### m3 - minor - the footer order departure stands unrecorded

FR-018 and the commit rules order the footer `Refs:` then
`Approved-by: OmO`, per `prompt.md` section 3; the constitution's Pull
Request Quality template shows `Approved-by:` above `Refs:`. Append to
the commit rules block: "The footer order follows `prompt.md` section
3; the template's example order does not govern this feature."

### m4 - minor - `diagnosis-b2.md` overstates the reproduction rate

The host measured 1 abort in 200 plain runs; the text says this
"matches the brief's about 3 of 100". Edit to: "which sits below the
brief's about 3 of 100 (the brief measured 6 of 200) and confirms".

### m5 - minor - the `git ls-remote` check stands in no task

`prompt.md` section 5 requires the check of
`git ls-remote origin fix/rocky-pmu-stack` against the sha
`git rev-parse HEAD` names. Append to T046: the remote ref names the
sha the local head names.

### m6 - minor - the aggregate rule carries two names

`plan.md` says "the C++23 aggregate rule"; `data-model.md` says "under
the C++20 rules the project builds with". The rule stands since C++20
and the project builds C++23 (XI.7: one term, one concept). Edit
`data-model.md` to: "under the aggregate rules the project builds
with: a rule standing since C++20 ([dcl.init.aggr])".

### m7 - minor - T030 reconfigures the tree the ci-rocky loops measure

`ci-rocky` and `ci-ubuntu` share the `build` tree (`ci-build`,
`binaryDir: ${sourceDir}/build`). T030 configures `ci-ubuntu` into
`build` before T031, T033, and T035 through T037 run from the same
tree, so the loops measure a clang tree while their text names
`ci-rocky`. Remediation: move T030 after T042, or start T031 with
"reconfigure with `cmake --preset=ci-rocky`".

### m8 - minor - `plan.md` Structure Decision undercounts the touch set

"the change touches the two files the defects live in and the two test
files that measure them" omits the `kPassReads` move pair the structure
block above it lists. Append: "; the `kPassReads` move touches
`source/harness/jitter_probe.cpp` and
`source/harness/detail/jitter_probe.hpp`".

## Verdict

The artifact set is consistent and implementable after the edits
above. Counts: 1 blocker, 6 major, 8 minor.

What verified clean:

- Coverage: all 18 functional requirements map to at least one task;
  SC-001 through SC-005 map to T031 through T045; FR-011's branch
  stands closed by the diagnosis (the failure reproduced, so no task
  exists for it, by design). No task contradicts a requirement after
  M1, M2, and M5 land.
- Commit grouping: T017 and T026 each land one commit per FR-007 and
  FR-012, tests ride with their changes, no task commits a partial
  group, and the three commit titles fit 50 characters in template
  sections (`Docs`, `Counters`, `Harness`).
- Approaches: `plan.md`'s B-1 and B-2 designs match `research.md` and
  `diagnosis-b2.md`; the counting-reader seam, the 1,024-read targets,
  the margin removals, and the kept spin bound and yield all agree
  across `plan.md`, `research.md`, `data-model.md`, and the tasks.
- Scope: the six planned source files match `prompt.md`'s bounds,
  touch no `PR #68` file (verified by fetch and diff), touch no
  installed header, and no CI, gate, or public surface file.
- Hard rules: the coverage allowance reads 21 at
  `tools/dbc/coverage_gate.sh:102`; the Rocky package list matches
  `.github/workflows/ci.yml:594`; the push, lease, local-only,
  container, and never-merge rules stand in the spec and tasks (T044
  through T046), with the two protocol gaps M5 and m5 above.
- Prose: `spec.md`, `tasks.md`, `research.md`, and `data-model.md`
  scan clean against the XI vocabularies outside verbatim quotation;
  the violations stand in `plan.md` and `diagnosis-b2.md` (M6).

Next action: land B1 and the M-series edits in `tasks.md` and the
design artifacts before `/speckit.implement`; the minors can ride the
same edit pass.
