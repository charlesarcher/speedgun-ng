# Feature Specification: CI Speed-Up

**Feature Branch**: `030-ci-speedup`

**Created**: 2026-10-25

**Status**: Ready

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"CI speed-up, item N0 of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md` (proposal R-1). Short name:
ci-speedup. Make CI and the local check faster. Do not weaken a gate.
After this change: the CI wall time of a master push goes from 28 min to
15 min or less; a quick local check before a push takes 5 min or less on
a lane host."

## Clarifications

### Session 2026-10-25

- Q: The base commit is already red on `lint`, and the closest full run
  of the same tree family failed on a coverage shortfall of 63 lines and
  121 branches and on one `prose-lint` XI.2 finding. Does this feature
  close those reds, so that AC-3's 100% reading and the "all jobs green"
  condition hold on its own run? → A: it closes the `lint` red alone,
  through one formatting-only commit (FR-065), and it moves no coverage
  number and no test verdict away from the base tree. The shortfall and
  the prose finding stay the property of spec 032: SC-003 reads the
  equality, and SC-012 names each red job with its base record (FR-064).

## Spec identity and run-time binding

- Spec number: `030`. `specs/` on master holds `001` through `029`
  (read at the base commit on 2026-10-25). `git ls-remote origin` names
  no ref carrying `030` or `speedup`. `gh pr list --state all --limit 8`
  returns PR 50 (`pre030-master-fix`, MERGED) as the newest head. The
  number 030 is free, so no renumbering applies.
- Branch: `030-ci-speedup`, cut from master
  `f507b8fe5e18f1aa4749dc5412ef6b072fcae9b1` on 2026-10-25.
- Base: the pre-030 direct fix. `7d8dfdd` ("Harness: Rename hunt retry
  pass parameter") is an ancestor of the base commit, and
  `7d8dfdd..f507b8f` holds fourteen commits: the twelve reverts, then
  `cd2f2f1` ("Docs: Trim the main page under the cap") and `f507b8f`
  ("Docs: Restore jitter probe facts"). `README.md` measures 834 lines
  at the base, inside the 835 cap.
- The twelve reverts, newest first: `5dc31e7` (test: Relocate the
  children's profile counters), `9b176ff` (Serialize the coverage
  capture), `e33d98a` (Pin the coverage compiler to g++-14), `9ee9be0`
  (Match the capture's gcov to the compiler), `25b7af2` (Pin the
  coverage compiler to g++-16), `5d4bb2d` (Source g++-16 from the
  toolchain PPA), `80ef590` (Fold the children's profile trees back in),
  `c4203a4` (Raise the instrumented test timeout), `7bbb68c` (Separate
  an Unreadable Trace From a Gap), `334a092` (Ignore the Inconsistent
  Class in genhtml), `aa3b477` (Ignore the Inconsistent Class When
  Listing), `b0d0f9b` (Give lcov Its Error Class Early).
- Version: `CMakeLists.txt:15` holds `VERSION 0.20.0`, `CMakeLists.txt:63`
  holds `SOVERSION 4` for the archive target and `CMakeLists.txt:281`
  holds `SOVERSION 12` for the harness target. This feature adds no
  public API, no option, no schema key and no environment variable, so
  all three values stay (FR-058, SC-011).
- Roadmap: item N0, proposal R-1. Order of the next items: 030, then 032
  (fix the three merged features), then 031 (host validation). Item 033
  can run beside 031.
- Lane: the lane the owner picks. PR #46, #47, #49 and #50 are MERGED,
  so no lane holds an open PR at the audit point of this writing.

## Audit point

Two readings bind this spec, taken the same way.

1. Base reading, 2026-10-25, carried below. `gh api
   runs/<id>/timing` gives `run_duration_ms` for the wall. `gh api
   runs/<id>/jobs` gives `completed_at - started_at` per job and per
   step. Skipped jobs stay out of the runner-minute sum.
2. Branch reading, the last run of `030-ci-speedup` before the pull
   request is ready. Same method, plus `ccache -s` per job, `ctest -N`
   per job, and the coverage artifact.

## Baseline

### A. The reading the request carries

| Run | Head | Wall | Runner minutes | Slowest job |
|-----|------|------|----------------|-------------|
| 39708015373 | master `bd10d76` | 28.9 min | 111 | test-rocky 27.0 min (toolchain install 991 s), test 25.6 min |
| 39910150207 | PR #46 `981c478` | 33.6 min | 118 | tsan 33.1 min (`harness_jitter_cli_test` timeout), test 26.3 min |
| 39883295074 | PR #46 `7eb46e4` | 45.7 min | 134 | test 45.2 min (build 2342 s) |
| 39941934864 | PR #47 `d6d60f2` | 47.1 min | 125 | test 46.5 min (build 2453 s) |

Step times of run 39708015373, as the request records them:

| Job | Build | Test |
|-----|-------|------|
| `test` (`ci-ubuntu`, analyzers in the build) | 1296 s | 202 s |
| `test-rocky` (`ci-rocky`, no analyzers) | 282 s | 322 s |
| `coverage` | 797 s | 326 s |
| `sanitize` | 404 s | 373 s |
| `tsan` | 266 s | 278 s |

### B. Re-measured on the base tree family

The base head carries no full run of its own (section C), so the
re-measurement reads the newest green master push between `bd10d76` and
the base: run 40042928839 at `fea69c5` (2026-10-24), the last master
commit before the twelve reverted commits. `fea69c5` is an ancestor of
`7d8dfdd`, and the base adds the 029 work plus the reverts on top of it.

| Field | Reading |
|-------|---------|
| Wall | 45.0 min (`run_duration_ms` 2697000) |
| Runner minutes | 109.3 (job sum 6557 s) |
| Slowest job | `test`, 2621 s |
| Job durations | test 2621, coverage 1310, test-rocky 699, sanitize 659, tsan 563, consumer-release 275, downstream-consumer 155, shared-audit 117, dbc-gate 48, prose-lint 42, lint 30, docs 38 |

Step times of the five heavy jobs of the same run:

| Job | Install | Configure | Build | Test | Other |
|-----|---------|-----------|-------|------|-------|
| `test` | analyzers 12 s | 4 s | 2277 s | 315 s | install-tree and package audits 0 s each |
| `test-rocky` | toolchain 45 s | 2 s | 303 s | 320 s | |
| `coverage` | LCov 34 s | 10 s | 829 s | 334 s | process 60 s, codecov 21 s |
| `sanitize` | clang plus deps 128 s | 10 s | 285 s | 218 s | |
| `tsan` | clang plus deps 13 s | 3 s | 251 s | 278 s | |

Per-test reading of the same run, from each job's `ctest` output:

| Job | Entries | `dbc_semantics_matrix` | Slowest other entry | Test step total |
|-----|---------|------------------------|---------------------|-----------------|
| `test` | 120 | 308.53 s | `snippet_compile` 43.99 s | 314.48 s |
| `test-rocky` | 120 | 316.64 s | `snippet_compile` 50.29 s | 320.01 s |
| `coverage` | 120 | 322.86 s | `snippet_compile` 48.36 s | 332.75 s |
| `sanitize` | 120 | 213.67 s | `snippet_compile` 27.13 s | 217.07 s |
| `tsan` | 117 | 273.94 s | `snippet_compile` 50.47 s | 277.27 s |

Two readings follow. The `ctest` step of every job holds little besides
the matrix entry, and the build step outranks the matrix as the largest
single cost on this tree (2277 s in `test`).

### C. The base head's own runs

| Run | Head | Verdict |
|-----|------|---------|
| 40203022166 | base `f507b8f`, push 2026-10-25 | `lint` failed in 31 s; the other eleven jobs skipped |
| 40135765542 | `7d8dfdd`, push 2026-10-24 | `lint` failed in 21 s; the other eleven jobs skipped |

The `Lint` step names `test/source/harness_cli_table_test.cpp` and stops
at `cmake/lint.cmake:51`. That finding predates this feature: it comes
from the reconcile edit of PR #47, and spec 032 owns it. Because `lint`
gates every other job, the base carries no timing for the jobs that
matter, so the re-measurement of the request's obligation reads section B.

## Preconditions

Each line was read at the base commit on 2026-10-25.

- **PC-1: parallelism. PASS as the request states it.** `ci.yml` holds
  803 lines and twenty `-j 2` sites, at lines 67, 71, 110, 123, 144,
  153, 168, 222, 244, 249, 264, 366, 376, 391, 395, 524, 544, 651, 692,
  696. Six are `ctest` lines; the rest are `cmake --build` lines. The
  repository is public, so a hosted `ubuntu-26.04` runner gives 4 vCPUs.
- **PC-2: no build cache. PASS.** No job names `ccache`, `sccache` or
  `actions/cache`; the only `cache` word in `ci.yml` belongs to the
  CMake cache check. Every job that builds starts from an empty tree.
- **PC-3: preset inheritance. PASS, with one note.** `ci-ubuntu`
  inherits `ci-build`, `ci-linux`, `clang-tidy`, `cppcheck` and
  `dev-mode`. `ci-rocky` inherits `ci-build`, `ci-linux` and `dev-mode`:
  the same list with the analyzers removed, and `ci-build` alone carries
  `binaryDir`. A preset for an analyzer-free CI build therefore needs a
  new name, and D-2 settles it.
- **PC-4: analyzer scope. PASS.** `.clang-tidy:21` sets
  `WarningsAsErrors: 'readability-identifier-naming'`, so that one check
  fails a build. The cppcheck launcher carries no `--error-exitcode`.
  `CMakeLists.txt:145-146` and `CMakeLists.txt:663-664` clear both
  launchers for a vendored tree, so the analyzer scope is each project
  translation unit the default build compiles under the Release flags of
  `ci-linux`.
- **PC-5: matrix script. PASS, with a path correction.** The script sits
  at `tools/dbc/semantics_matrix.sh` (362 lines), registered as the
  single entry `dbc_semantics_matrix` at `test/CMakeLists.txt:139`
  inside the Linux block. It reads only `CXX` (`:54`), takes `JOBS` from
  `nproc` (`:60`), builds in a `mktemp -d /tmp/sg-dbc-matrix.XXXXXX`
  tree (`:76`), configures with `-DCMAKE_BUILD_TYPE=None` (`:181`),
  `-DCMAKE_CXX_FLAGS="$flags"` (`:182`) and `-DCMAKE_CXX_COMPILER`
  (`:186`), builds only `dbc_test` with `-j "$JOBS"` (`:228`), and calls
  `run_cell` four times (`:323` to `:326`) plus the observe probe
  (`:330`). No cell carries `--coverage`.
- **PC-6: compiler families. PASS from the configure logs of run
  39708015373.** `test` and `coverage` use GCC 15.2.0, `test-rocky` uses
  GCC 14.3.1, `sanitize` and `tsan` use Clang 21.1.8 through
  `env: { CXX: clang++ }`. Re-read at the branch audit point from the
  logs of that run before D-5 is applied.
- **PC-7: per-test budgets. PASS with a count correction.** Nine entries
  hold a `TIMEOUT` at the base: 120 s at `test/CMakeLists.txt:580`,
  `:742`, `:1392`, `:1408`, `:1425`, `:1440`, `:1456`, `:1473`, and 300 s
  for `example_run` at `:1487`. Every other entry takes the CTest default
  of 1500 s. `ci.yml` holds no `--timeout` flag and no `timeout-minutes`
  key at the base, so the request's removal instruction has nothing to
  remove.
- **PC-8: job graph. PASS.** Twelve jobs, at `ci.yml` lines 13 (`lint`),
  44 (`coverage`), 91 (`sanitize`), 125 (`tsan`), 155 (`test`), 224
  (`test-rocky`), 251 (`shared-audit`), 348 (`downstream-consumer`), 495
  (`consumer-release`), 675 (`dbc-gate`), 704 (`prose-lint`), 757
  (`docs`). `docs` holds `needs: [sanitize, test, test-rocky]` at `:759`
  and `if: github.ref == 'refs/heads/master'`. The workflow has no
  `concurrency` key.
- **PC-9: coverage configuration. PASS.** The capture command
  (`cmake/coverage.cmake:57-66`) carries
  `--ignore-errors mismatch,negative,inconsistent`. The extract command,
  the summary command (`tools/dbc/coverage_gate.sh`) and `genhtml` carry
  no ignore class. The extract filter is
  `--filter exception,branch,brace`. This file stays as master has it.
- **PC-10: the base verdicts. FAIL, and the Clarifications section
  above settles what this feature does about them.**
  `lint` is red at the base (section C). The closest full run of this
  tree family, run 39941934864 at the PR #47 head, adds two more reds
  this feature did not cause: `coverage` failed on a shortfall of 99.4%
  lines (9752 of 9815) and 97.5% branches (4772 of 4893), and
  `prose-lint` failed on one `XI2.CONTRASTIVE` finding in commit
  `297521f`. On that run the `inconsistent` and `mismatch` lcov records
  printed as warnings and the extract, summary and `genhtml` stages all
  completed.

## Residual need of the twelve reverted commits

Each row of the request's twelve is recorded here with its verdict and
the route a live need takes inside the approved scope.

- `00e5848` "Meta: Serialize the coverage capture". **Need: yes.**
  Profile counters merge into one `gcda` per object with no lock, so
  concurrent test processes lose updates and the capture reads gaps no
  test leaves (measured 2026-10-24 in that commit's body).
  **Route:** FR-044 keeps the coverage lane at a parallelism the counter
  merge tolerates, either `-j 1` or a `RESOURCE_LOCK` over the entries
  that fork exec children (D-8).
- `5fe94fb` "Meta: Fold the children's profile trees back in" and
  `872086c` "test: Relocate the children's profile counters".
  **Need: yes, as a pair.** Each exec child leaves its counters in a
  `/tmp/sgcov-*` tree, and the capture needs those trees merged before
  `lcov -c` reads the build tree. **Route:** FR-044 and FR-049 reach the
  same trace inside the approved `ci.yml` scope, with a CI run as the
  evidence and no new ignore class.
- `a7e7ce5` (g++-14 pin), `c5d7a5f` (g++-16 pin), `ea75686` (toolchain
  PPA source) and `e9a3907` (match the capture's gcov to the compiler).
  **Need: no.** The coverage lane configures with the runner's default
  compiler (GCC 15.2.0 at run 39708015373) and reaches the gate with the
  classes `cmake/coverage.cmake` already carries. A pin revives the
  gcov-match need. **Route:** FR-042 keeps pins, toolchain sources and
  gcov-matching logic out.
- `45d5e42` (ignore the class in genhtml), `f82406e` (ignore it when
  listing) and `b0d0f9b` (give lcov its error class early).
  **Need: not observed.** At the closest full run of this tree family
  those stages printed warnings and completed (PC-10), and the
  `coverage` failure there was a shortfall. **Route:** FR-041 adds no
  ignore class to the extract, summary or `genhtml` command; a run that
  shows a stage abort on that class reopens the point.
- `dcbdfa6` "dbc: Separate an Unreadable Trace From a Gap".
  **Need: probably.** `lcov --summary` can refuse a trace whose every
  gated measure reads 100.00 percent, and the gate then reports a
  shortfall about a full trace. The 029 source that triggers it is on
  master. **Route:** FR-043 splits the two exits inside the approved
  `tools/dbc/coverage_gate.sh`, by reading lcov's own output, so no
  ignore class is needed.
- `3f3b56c` "CI: Raise the instrumented test timeout". **Need: yes for
  the need, no for the shape.** At least one instrumented entry outruns
  the 1500 s CTest default in a sanitizer lane. **Route:** FR-031 to
  FR-034 set a per-test budget with named exceptions, each carrying a
  measured per-lane number from a CI run, and keep `--timeout` and any
  raised `timeout-minutes` out (FR-034).

## Decisions

- **D-1: the analyzer job is sharded, and the result cache is refused.**
  Option (a) of the request keeps a clang-tidy and cppcheck result cache.
  Hard rule 5 forbids caching an analyzer result, and a hard rule outranks
  a recommendation, so option (b) is chosen: split `static-analysis`
  across two or three runners by target list. If the sharded job still
  passes 15 min on a pull-request tree, that timing is recorded and no
  unit is dropped to meet the target (SC-003).
- **D-2: a named preset, with the redundancy recorded.** The new
  configure preset `ci-ubuntu-test` inherits `ci-build`, `ci-linux` and
  `dev-mode`. PC-3 shows `ci-rocky` holds the same three, so the new
  preset duplicates an existing inheritance. The new name is kept: a
  Rocky Linux name on an Ubuntu runner hides intent, and one preset per
  intent is the pattern the file already follows.
- **D-3: `ci-ubuntu` stays untouched.** `dbc-gate` configures it at
  `ci.yml:689` and `prose-lint` at `ci.yml:752`, so a change there would
  move two gates this feature does not own (FR-013).
- **D-4: the matrix becomes five entries.** Each of the four cells of
  `tools/dbc/semantics_matrix.sh:323-326` and the probe of `:330` gets
  one `ctest` entry with its own build directory, `JOBS` from a CMake
  value in place of `nproc` (`:60`), `PROCESSORS` set to that value, and
  every check of its cell kept. The probe sources are written once per
  entry (FR-018 to FR-021).
- **D-5: one compiler family per lane, with the proof recorded.** A
  CMake value decides registration and is set in `test` (GCC 15.2.0),
  `test-rocky` (GCC 14.3.1) and `tsan` (Clang 21.1.8). `coverage` (the
  same GCC 15.2.0 as `test`) and `sanitize` (the same Clang 21.1.8 as
  `tsan`) leave it unregistered. PC-5 is the proof that nothing is lost:
  the script configures with no preset, sets its own flags and
  `CMAKE_BUILD_TYPE=None`, reads only `CXX`, builds in `/tmp`, and never
  adds a count to the coverage trace. Each of the three lanes records its
  compiler version from its own configure log (FR-022, FR-023).
- **D-6: tiers come from one list.** One list in `test/CMakeLists.txt`
  gives each entry the label `quick` or `slow`, default `slow` never
  applies, and each `slow` row carries its reason (`dbc_semantics_matrix`
  cells and `snippet_compile` are today's slow set). CI runs `ctest` with
  no label filter. The presets are `prepush`, `prepush-quick` and
  `prepush-full`; the name `dev` stays unused, because
  `CMakeUserPresets.json` is git-ignored (`.gitignore:13`) and carries a
  `dev` preset on lane hosts, where a duplicate name is a CMake error.
- **D-7: the budget is per entry and measured.** `TIMEOUT 300` lands only
  where no `TIMEOUT` stands, so the eight 120 s values and the 300 s
  `example_run` value keep their readings. An exception names its entry,
  its reason and the per-lane number from a CI run that supports it.
  PC-7 records that no `--timeout` and no `timeout-minutes` exist at the
  base, so no removal is owed.
- **D-8: the coverage lane keeps its instrumented suite separated.** The
  gcda merge has no lock (the `00e5848` body, measured 2026-10-24), so
  the coverage lane starts at `ctest -j 1` for the instrumented suite and
  moves to a `RESOURCE_LOCK` over the exec-child entries if that
  serialization costs more than the 334 s the step holds. AC-3's cold
  against warm equality is the measurement that settles it (FR-044).
- **D-9: cache policy.** `ccache` joins the CI presets as
  `CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER`.
  `CCACHE_DIR` sits inside the workspace, because the rocky container
  keeps a different home directory. The cache key joins these segments,
  in this order: the prefix `ccache`, the job name, the preset name, the
  hash of the compiler's `--version` output, the hash of
  `CMakePresets.json` and `external/**`, and the run id. The restore
  keys are that key with the run id dropped, so a key that never changes
  is never saved again. A pull request restores only; a push to master
  saves. Each job prints `ccache -z` before its build and `ccache -s`
  after. `CCACHE_MAXSIZE` is sized so every lane stays under the 10 GB
  repository budget. `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS` stay unset
  (FR-006 to FR-011).
- **D-10: the pre-push script mirrors CI.** `tools/ci/prepush.sh` runs
  `tools/docs/example_drift.py .`,
  `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` with
  clang-format 18.1.8, `test/time_source_gate.sh .`,
  `cmake -P cmake/spell.cmake` and
  `python3 tools/prose/prose_gate.py --base origin/master`, then a
  `prepush` build at `-j $(nproc)`, then
  `ctest --preset prepush-quick -j $(nproc)` once at full affinity and
  once under `taskset -c 0,1`, then clang-tidy with
  `--checks=-*,readability-identifier-naming` on the changed units,
  because that is the check that fails CI. `--full` adds the full tier,
  the `consumer-release` build and the tsan configure and link. Every
  step prints its time; a bad option exits 2; a `clang-format` other than
  18.1.8 stops with an error before formatting (FR-028 to FR-030).
- **D-11: concurrency.** The workflow carries
  `group: ${{ github.workflow }}-${{ github.ref }}` and
  `cancel-in-progress: ${{ github.event_name == 'pull_request' }}`, so a
  new push to a pull request cancels the older run and a master run
  stands (FR-039).
- **D-12: `docs` waits for the analyzers.** `docs` gains
  `static-analysis` in its `needs` at `ci.yml:759`, so a docs deploy
  cannot pass a failing analyzer (FR-038).
- **D-13: the analyzer scope stays the default build.** `run-clang-tidy`
  and `cppcheck --project` over `compile_commands.json` stay out of scope:
  that list also names the vendored units, and its cppcheck configuration
  differs from the preset launcher's. PC-4 names the two clearing sites
  that define today's scope (FR-049).

## Out of scope

- Host validation: no `tools/hostcheck/`, no `validation/` directory and
  no `hostcheck` label. That is spec 031, after this one.
- Any command-line option, environment variable or schema key (FR-057).
- A container image build and push for the rocky lane: a registry and
  `packages: write` sit outside the approved scope, so FR-035 keeps the
  dnf package cache instead.
- Any change to `VERSION` or a `SOVERSION` (FR-058).
- Any new `LCOV_EXCL` marker, `--ignore-errors` entry, skipped test,
  lowered coverage bar, cached analyzer result or raised timeout
  (FR-040, FR-041, FR-043, FR-047, FR-034, FR-017).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Get the full verdict on a push inside 15 minutes (Priority: P1)

A contributor pushes a branch and waits for the CI verdict. Today the
wait is 28 to 47 minutes, because every job compiles two translation
units at a time on a four-vCPU runner, the test job runs the analyzers
inside its own build, and nothing is reused between runs. After this
change the same push answers with every gate still in place, and the
wait stops at 15 minutes with a warm cache.

**Why this priority**: This is the request's headline number and the
roadmap item's reason to exist. Every other story serves it.

**Independent Test**: Read `run_duration_ms` and the per-job durations of
a run of the branch with `gh`, and compare them with the Baseline
tables. No other story's deliverable is needed to make the reading.

**Acceptance Scenarios**:

1. **Given** a branch with a warm cache, **When** a pull request pushes,
   **Then** the median wall time of three such runs is 15 min or less and
   no job is skipped, red or missing.
2. **Given** the same commit, **When** one run starts with an empty
   cache, **Then** every job gives the same verdict as the warm runs and
   the run is recorded with its wall time and slowest job.
3. **Given** a master push, **When** the run finishes, **Then** the
   recorded table names each job's wall time and each build and test step
   time next to the base reading of the same job.

---

### User Story 2 - Run one local pre-push command inside 5 minutes (Priority: P2)

A contributor runs `tools/ci/prepush.sh` on a lane host before pushing.
The script runs the same checks the `lint` job runs, the prose gate, a
cached build, and the quick test tier twice, once with the whole cpu set
and once pinned to two cpus, and prints the time of each step. A bad
option exits 2. The `--full` form adds the slow tier, the consumer
release build and the tsan configure and link.

**Why this priority**: The second headline number, and the only part of
this feature a contributor touches by hand every day. It depends on the
cache and the tiers, so it follows story 1 in build order.

**Independent Test**: Time the script on the recorded lane host with a
warm `ccache`, read the per-step lines, run it with a bad option, and
read the exit status.

**Acceptance Scenarios**:

1. **Given** a lane host with a warm `ccache`, **When** the script runs
   with no option, **Then** it finishes in 5 min or less and prints one
   timing line per step.
2. **Given** any checkout, **When** the script gets an option it does
   not know, **Then** it exits 2.
3. **Given** a host whose `clang-format` is not 18.1.8, **When** the
   script reaches the format step, **Then** it stops with an error and
   formats nothing.
4. **Given** `--full`, **When** the script runs, **Then** the slow tier,
   the `consumer-release` build and the tsan configure and link also
   report a time.

---

### User Story 3 - Read the analyzer verdict as its own job (Priority: P3)

A reviewer opens a run and finds `static-analysis` as a job of its own,
with the finding set the analyzers produce. The `test` job configures the
analyzer-free preset and spends its time running tests. Both analyzers
still read every project translation unit the default build compiles,
under the same flags and header filter, and a finding fails the job.

**Why this priority**: It is the largest single saving on the current
tree (2277 s of `test` build at the base reading), and it moves a gate
onto a new job, so it carries the highest verification duty.

**Independent Test**: Upload the sorted `file:line:check` finding list of
the last master run and of the branch, compare them, then add one
`readability-identifier-naming` violation on a throwaway branch and read
the failing `static-analysis` verdict on a warm cache.

**Acceptance Scenarios**:

1. **Given** the branch and master, **When** both finding lists are
   sorted, **Then** the two lists are equal.
2. **Given** a warm cache, **When** a branch adds one naming violation,
   **Then** `static-analysis` fails and `test` still passes.
3. **Given** a run of the branch, **When** its `test` job is read,
   **Then** the job carries its install, install-tree audit, pmu-events
   and test steps and no analyzer build.

---

### User Story 4 - See one matrix row per cell and per compiler (Priority: P4)

A contributor who breaks a contract semantic sees which cell broke in
which compiler's job. Today one entry runs five serial configures inside
one job and three jobs pay for the same work twice over. After the split,
each cell is one entry, five in all, and the matrix runs in the three
lanes that name distinct compilers.

**Why this priority**: It is the biggest `ctest` saving (the entry holds
308 to 323 s of a 315 to 333 s step on three jobs) and it changes which
jobs run which entries, so AC-4 has to account for every difference.

**Independent Test**: Read `ctest -N` per job before and after, read the
per-entry durations of the new five entries, and read the compiler
version each of the three lanes records.

**Acceptance Scenarios**:

1. **Given** the `test`, `test-rocky` and `tsan` jobs, **When** their
   test lists are read, **Then** each carries five matrix entries and
   each entry names its cell.
2. **Given** the `coverage` and `sanitize` jobs, **When** their lists are
   read, **Then** the matrix entries are absent and the reason is
   recorded with the evidence of D-5.
3. **Given** four matrix entries running at the same time, **When** the
   job is read, **Then** no entry starts more compile jobs than its
   `JOBS` value and `PROCESSORS` equals that value.

---

### User Story 5 - Pick the quick tier locally while CI keeps everything (Priority: P5)

A contributor chooses a tier. `test/CMakeLists.txt` holds one list that
gives every entry `quick` or `slow`, `test_tier_labels` proves the two
labels partition the suite with no overlap and no gap, and CI passes no
label filter at all.

**Why this priority**: The 5-minute local number depends on it, and a
wrong partition would quietly drop a test from a tier.

**Independent Test**: Run `ctest -N -L quick` and `ctest -N -L slow`,
compare the union with `ctest -N`, and read `test_tier_labels` fail on a
planted overlap.

**Acceptance Scenarios**:

1. **Given** a configured tree, **When** the label test runs, **Then**
   the two label lists are disjoint and their union equals the full list.
2. **Given** a CI job, **When** its `ctest` line is read, **Then** no
   label filter appears in it.
3. **Given** a slow entry, **When** the list is read, **Then** that entry
   carries a written reason.

---

### User Story 6 - See a slow test fail in its own change (Priority: P6)

An entry that outruns its budget fails where it was written. The
registration sets a 300 s budget on every entry that holds none, keeps
every lower value, and lists each exception with the measured per-lane
number that justifies it.

**Why this priority**: It keeps the speed-ups from hiding a new slow
test, which is how the base tree reached a 2277 s build step unnoticed.

**Independent Test**: Read the budget of every entry against the measured
per-entry times of the branch runs, and read the exceptions list against
those numbers.

**Acceptance Scenarios**:

1. **Given** every job of a run, **When** the per-entry times are read,
   **Then** no entry exceeds its budget.
2. **Given** an entry with a budget other than 300 s, **When** the
   exceptions list is read, **Then** it names the entry, the reason and
   the CI number behind it.
3. **Given** an entry whose `TIMEOUT` master sets to 120 s, **When** the
   new registration runs, **Then** that entry keeps 120 s.

---

### User Story 7 - Keep a toolchain install from owning a job (Priority: P7)

An install step costs 991 s on master one day and 32 s another day, and
nobody can predict which. The rocky lane keeps its dnf package cache and
the Ubuntu lanes keep their apt archives in a cache keyed on the package
list, so a restore feeds the installer from local files.

**Why this priority**: It removes the widest variance from the wall
clock, and it is the one saving that depends on a mirror's mood.

**Independent Test**: Read the install-step time of each run of the
branch, and read a run where the cache restore missed.

**Acceptance Scenarios**:

1. **Given** a push to master, **When** the rocky lane's install step is
   read, **Then** its time is recorded for that run.
2. **Given** a cache whose key matches no prior save, **When** the job
   runs, **Then** the verdict is unchanged and the install step carries
   its time.

---

### User Story 8 - Cancel the run a new push superseded (Priority: P8)

Two pushes to one pull request leave one run standing: the newer. A
master run is never canceled, so a recorded master measurement never
vanishes.

**Why this priority**: It is a small workflow key, and it protects the
measurement of every other story from canceled and re-queued runs.

**Independent Test**: Push twice to a pull-request branch in quick
succession and read both run statuses, then read a master run's status.

**Acceptance Scenarios**:

1. **Given** a running pull-request run, **When** a second push lands on
   the same ref, **Then** the older run reports canceled.
2. **Given** a running master push, **When** another master push lands,
   **Then** the first run keeps running.

### Edge Cases

- A cold cache gives a verdict identical to a warm cache, per job
  (story 1, AC-6).
- A cache restored from a different compiler version or a different
  preset gives no hit and no wrong object: the key names both.
- A naming violation added on a warm cache still fails
  `static-analysis`, because CMake runs the analyzers before the compile
  on every unit and no analyzer result is cached (story 3, AC-5).
- Two tests that share a cpu pin, a cpu reservation, a file or a device
  node collide at `-j $(nproc)`: a `RESOURCE_LOCK` separates them, and a
  retry or a sleep stays out as a cure.
- A test whose threads outrun the runner's cpus skews a timing-sensitive
  check: `PROCESSORS` bounds the scheduler's decision.
- The instrumented suite loses profile counters when test processes and
  their exec children write one `gcda` at once: the coverage lane keeps
  those entries separated (D-8) and AC-3 proves the trace.
- A cache hit does not make a stale object: `ccache` keys on the
  compiler version, so a runner image bump is a miss.
- A hang now meets a 300 s budget in place of the 1500 s default, so it
  surfaces sooner and its budget entry names it.
- An entry whose sanitizer-lane time passes 300 s fails until its
  exception is written with a measured number; the fix never raises a
  lane-wide flag.
- An analyzer job sharded across runners must still cover every unit:
  dropping a target to meet 15 min is refused (D-1).
- The matrix runs on three lanes and each lane records its own compiler
  version, so a compiler bump shows up as a changed record and not a
  silent loss of coverage.
- A `docs` deploy after a failing analyzer is refused by `needs`
  (D-12).
- A canceled pull-request run has no timing to report: the branch
  reading uses runs that completed.
- A rocky lane whose cache restore misses has no local packages: the
  minimal `dnf -y install tar zstd` path still installs the toolchain.
- A `clang-format` other than 18.1.8 formats differently and would write
  a diff CI rejects: the script stops first.

## Requirements *(mandatory)*

### Parallelism and scheduling (C-1)

- **FR-001**: When any job of `.github/workflows/ci.yml` compiles a
  tree, the workflow shall set build parallelism to the runner's cpu
  count in place of the fixed `-j 2` at all twenty sites PC-1 names.
- **FR-002**: When any job runs `ctest`, the workflow shall set test
  parallelism to the runner's cpu count, with the coverage-lane
  exception of FR-044.
- **FR-003**: Where a test starts more than one thread, or a matrix cell
  starts more than one compile job, the registration shall set
  `PROCESSORS` to that count, so `ctest` never places more spinning
  threads on a runner than that runner has cpus.
- **FR-004**: Where two tests share a cpu pin, a cpu reservation, a file
  or a device node, the registration shall set `RESOURCE_LOCK` on both,
  following the `counters_trap_checked_out` precedent of
  `test/CMakeLists.txt:297`.
- **FR-005**: When a test fails at the higher parallelism, the change
  shall remove the collision at its shared resource. A retry loop, a
  sleep, `--repeat until-pass` and `RUN_SERIAL` as a cure shall stay
  absent.

### Build cache (C-2)

- **FR-006**: The CI presets shall pass `ccache` as
  `CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER`.
- **FR-007**: Each job that builds shall keep its cache under
  `CCACHE_DIR` inside the workspace and restore it through
  `actions/cache` under the key and restore keys of D-9.
- **FR-008**: A pull-request run shall restore the cache and save
  nothing; a push to master shall save it.
- **FR-009**: Each job shall print `ccache -z` before its build and
  `ccache -s` after it, and the pull request shall report those hit
  rates.
- **FR-010**: A cold-cache run and a warm-cache run of one commit shall
  give the same verdict in every job.
- **FR-011**: `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS` shall stay unset
  in every CI job: `CCACHE_BASEDIR` hands the compiler relative paths,
  which changes `__FILE__`, `std::source_location` and the source paths
  written into a `.gcno` file.

### Static analysis (C-3)

- **FR-012**: The repository shall carry a configure preset
  `ci-ubuntu-test` that inherits `ci-build`, `ci-linux` and `dev-mode`.
- **FR-013**: The `ci-ubuntu` preset shall keep the inheritance master
  has, because `dbc-gate` (`ci.yml:689`) and `prose-lint` (`ci.yml:752`)
  configure it.
- **FR-014**: The `test` job shall configure `ci-ubuntu-test` and keep
  every step it holds today apart from the analyzer build: the analyzer
  install, the install-tree and package audits, the standalone example
  link manifest, the vendored pmu-events integrity check and `ctest`.
- **FR-015**: A job named `static-analysis` shall configure `ci-ubuntu`
  and build the tree, so the same translation units reach both analyzers
  under the same Release flags by construction.
- **FR-016**: A finding of either analyzer shall fail `static-analysis`.
- **FR-017**: No analyzer result shall be cached. `ccache` shall cache
  compiles only, because CMake runs both analyzers before the compile of
  every unit, also on a cache hit.
- **FR-018**: `static-analysis` shall shard its target list across two or
  three runners that together read every unit FR-015 covers (D-1).

### Semantics matrix lanes (C-4)

- **FR-019**: The matrix shall register one `ctest` entry per cell of
  `tools/dbc/semantics_matrix.sh:323-326` and one for the observe probe
  of `:330`, five entries in all.
- **FR-020**: Each entry shall build into its own directory and take its
  `JOBS` value from a CMake value in place of `nproc` (`:60`).
- **FR-021**: Each entry shall set `PROCESSORS` to its own `JOBS` value.
- **FR-022**: Each entry shall keep every check of the cell it comes
  from, and the probe sources shall be written once per entry.
- **FR-023**: A CMake value shall decide registration. It shall be set
  for the `test`, `test-rocky` and `tsan` lanes and left unset for
  `coverage` and `sanitize`, and each of the three registering lanes
  shall record the compiler version of its own configure log.
- **FR-024**: The spec, plan or pull request shall record the evidence
  that the two lanes which drop the matrix lose no check: the script
  configures with no preset, sets `CMAKE_BUILD_TYPE=None` and
  `CMAKE_CXX_FLAGS`, reads only `CXX`, builds under `/tmp`, and carries
  no `--coverage`.

### Tiers and the pre-push path (C-5)

- **FR-025**: Every test entry shall carry exactly one label from one
  list in `test/CMakeLists.txt`, `quick` or `slow`, with `quick` as the
  default and a written reason beside each `slow` entry.
- **FR-026**: A test named `test_tier_labels` shall read
  `ctest -N -L quick` and `ctest -N -L slow` and fail when the two lists
  overlap or when their union leaves out an entry of `ctest -N`.
- **FR-027**: Every CI job shall run `ctest` with no label filter.
- **FR-028**: The repository shall carry the configure preset `prepush`
  (dev-mode with the ccache launcher) and the test presets
  `prepush-quick` (label `quick`) and `prepush-full` (no filter). The
  preset name `dev` shall stay unused.
- **FR-029**: `tools/ci/prepush.sh` shall run the steps D-10 names, in
  that order, and print the time of each step.
- **FR-030**: When `tools/ci/prepush.sh` gets an option it does not
  know, it shall exit 2. When the host's `clang-format` is not 18.1.8, it
  shall stop with an error before formatting. With `--full`, it shall
  also run the slow tier, the `consumer-release` build and the tsan
  configure and link, each with its own time.

### Per-test budget (C-6)

- **FR-031**: At the end of `test/CMakeLists.txt` and
  `example/CMakeLists.txt` the registration shall read the directory's
  `TESTS` property and set `TIMEOUT 300` on every entry that holds no
  `TIMEOUT`, keeping every lower value master sets.
- **FR-032**: Each exception shall be listed in that same function with
  its reason and the measured per-lane number, read from a CI run, that
  supports it.
- **FR-033**: No test entry shall exceed its budget in any job.
- **FR-034**: No `ctest` line shall carry a `--timeout` flag, no job
  shall raise a `timeout-minutes` value, and no test shall pass because
  a ceiling moved (FR-005).

### Toolchain and package cache (C-7)

- **FR-035**: The rocky lane shall keep its dnf package cache
  (`keepcache=1`) in a cache keyed on its package list and restore it
  after a minimal `dnf -y install tar zstd`.
- **FR-036**: The Ubuntu lanes shall cache `/var/cache/apt/archives` in a
  cache keyed on the package list of that job.
- **FR-037**: Every run shall record the time of each install step, and a
  cache restore miss shall leave the verdict unchanged.

### Workflow mechanics (C-8)

- **FR-038**: The `docs` job shall add `static-analysis` to its `needs`
  at `ci.yml:759`.
- **FR-039**: The workflow shall carry the `concurrency` key of D-11, so
  a new push to a pull-request ref cancels the older run and a master run
  is never canceled.

### Gates that stay where they are (C-9)

- **FR-040**: Coverage shall hold 100% lines and 100% branches with the
  `brace` filter in place and no waiver. No `LCOV_EXCL` marker shall be
  added, and `cmake/coverage.cmake` shall keep the capture command master
  has after the reverts, so no `--ignore-errors` entry grows.
- **FR-041**: The extract command, the summary command and `genhtml`
  shall gain no ignore class, on the record of the residual table row for
  `45d5e42`, `f82406e` and `b0d0f9b`.
- **FR-042**: No lane shall pin a compiler, add a toolchain source, or
  gain gcov-matching logic, on the record of the row for `a7e7ce5`,
  `c5d7a5f`, `ea75686` and `e9a3907`.
- **FR-043**: When `lcov --summary` refuses a trace whose gated measures
  all read 100.00 percent, `tools/dbc/coverage_gate.sh` shall report a
  tool failure and keep a shortfall a separate exit, reading lcov's own
  output, so the fix needs no ignore class.
- **FR-044**: The coverage lane shall run its instrumented suite at a
  parallelism the unlocked `gcda` merge tolerates, either `-j 1` or a
  `RESOURCE_LOCK` over the entries that fork exec children, and the
  coverage trace of that lane shall carry the exec children's counts
  (D-8).
- **FR-045**: Every job that runs on master shall still run: `lint`,
  `test`, `test-rocky`, `coverage`, `sanitize`, `tsan`,
  `consumer-release`, `downstream-consumer`, `shared-audit`, `dbc-gate`,
  `prose-lint` and `docs`, with `static-analysis` added.
- **FR-046**: Each sanitizer job shall run every test entry it runs
  today, apart from the matrix entries D-5 moves.
- **FR-047**: No test shall be skipped or dropped from CI. An entry with
  a `SKIP_RETURN_CODE` shall keep the rule master sets for it.
- **FR-048**: Tests shall stay deterministic: no sleep, no clock read, no
  retry loop, no `--repeat until-pass`.
- **FR-049**: Both analyzers shall read every translation unit they read
  today, under the same configuration, flags and header filter (PC-4,
  D-13), and the sorted finding list of the branch shall equal master's.
- **FR-050**: The `## Quick start` section of `README.md` shall name the
  two tiers and `tools/ci/prepush.sh` in 25 lines or fewer.

### Measurement and reporting (C-10)

- **FR-051**: The pull request shall carry a table of each job's wall
  time and each build and test step time, before (run 39708015373 and
  the re-measured run of section B) and after (the last run of the
  branch), each read with `gh`.
- **FR-052**: The pull request shall report the median wall time of three
  master pushes and three pull-request pushes of the branch with a warm
  cache, the install-step time of each of those runs, one cold-cache run,
  and one run of the PR #47 head at the audit point with the slowest job
  named.
- **FR-053**: The pull request shall give, per job, the difference
  between the `ctest -N` list of master and that of the branch, with the
  reason for each difference.
- **FR-054**: The pull request shall upload the sorted `file:line:check`
  finding lists of both analyzers from the last master run and from the
  branch as artifacts, and shall show one throwaway branch where a single
  added `readability-identifier-naming` violation fails
  `static-analysis` on a warm cache. That throwaway branch shall be
  deleted.
- **FR-055**: The pull request shall report one cold-cache run and two
  warm-cache runs of one commit with an equal verdict per job, and three
  `sanitize` and `tsan` runs at the full affinity of the runner.
- **FR-056**: One pull-request comment shall carry the before and after
  tables, the `ccache -s` hit rates, the `ctest -N` list difference and
  the finding-list proof.

### Scope, version and history mechanics (C-11)

- **FR-057**: This change shall add no command-line option, no
  environment variable and no schema key.
- **FR-058**: `VERSION 0.20.0` and both `SOVERSION` values shall equal
  master's, and the check shall be recorded.
- **FR-059**: The work shall rebase onto master, push with
  `git push --force-with-lease`, and show
  `git ls-remote origin 030-ci-speedup` after each push.
- **FR-060**: Each commit shall carry a title of 50 characters or fewer,
  a body wrapped at 72 columns, and one footer block that names
  `Refs: specs/030-ci-speedup` and then `Approved-by: OmO`, in that
  order. No other approval line shall appear.
- **FR-061**: `python3 tools/prose/prose_gate.py --base origin/master`
  shall pass before each push.
- **FR-062**: The repository shall hold no ruling, acceptance or approval
  text that names a person as the approver.
- **FR-063**: After each push the work shall read the log of every CI job
  and fix the failures this change introduces.
- **FR-064**: When a job is already red on the base commit for a reason
  this change did not cause, this change shall leave that red to the spec
  that owns it, and the pull request shall name the job, the base record
  that shows the red predates this change, and the owning spec. This
  change shall add no source or test edit that belongs to another
  feature's deliverable, and it shall introduce no red of its own
  (SC-012).
- **FR-065**: The branch shall carry one formatting-only commit that
  clears the base `Lint` step. `cmake -D FORMAT_COMMAND=clang-format -P
  cmake/lint.cmake` names one file at the base,
  `test/source/harness_cli_table_test.cpp`, at its `hunt` and `master`
  ternary chain (`:886` to `:889`), and clang-format 18.1.8 is
  idempotent there, so one pass clears the step. The commit shall carry
  no other change, so constitution V keeps formatting apart from content
  (SC-012).

## Key Entities *(include if feature involves data)*

- **Run record**: one workflow run, held by its id, head, wall time and
  the sum of its completed jobs' durations. Every table compares two of
  these.
- **Job record**: one job of a run, held by name, verdict, duration and
  its step durations. The before and after tables hold one row per pair.
- **Step record**: one step of a job, named as the workflow names it,
  with its duration. `Build` and `Test` carry the two numbers the
  speed-ups move.
- **Test entry record**: one `ctest` name, with its tier label, its
  budget, its `PROCESSORS` and `RESOURCE_LOCK` values, and its measured
  time in each lane that runs it.
- **Cache key**: the string D-9 defines. One key names one compiler
  version, one preset, one job and one run.
- **Matrix cell**: one of five contract-semantic configurations, held by
  cell name, `NDEBUG` state, optimization flag, semantic mode, expected
  assertion count and compiler family.
- **Compiler family record**: one lane's compiler version, read from that
  lane's own configure log, and the matrix entries that family runs.
- **Finding list**: the sorted set of `file:line:check` lines both
  analyzers produce, one artifact from master and one from the branch.
- **Pre-push step**: one step of `tools/ci/prepush.sh`, with the command
  it mirrors in CI and the time it printed on the recorded lane host.
- **Residual need row**: one reverted commit, its need after this spec,
  and the in-scope route that need takes. The table above holds twelve.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001** (AC-1): The pull request carries the before and after table:
  every job's wall time and every `Build` and `Test` step time from run
  39708015373, from the re-measured run of section B, and from the last
  run of the branch, each read with `gh`.
- **SC-002** (AC-2): The median wall time of three master pushes and
  three pull-request pushes of the branch is 15 min or less with a warm
  cache; the install-step time of each run is recorded; one cold-cache
  run is timed; one run of the PR #47 head at the audit point is timed
  with its slowest job named.
- **SC-003** (AC-3): The coverage artifact of the branch carries the
  same file list as the base tree's and moves no line or branch count
  away from it, and a run with `CCACHE_DISABLE=1` and a warm-cache run
  of one commit produce the same sorted `SF`, `DA` and `BRDA` records.
  The base reads 99.4% lines and 97.5% branches (PC-10): the branch
  reproduces both numbers and adds no shortfall, and the 100% bar of
  FR-040 closes in spec 032.
- **SC-004** (AC-4): Each job's `ctest -N` list difference against master
  is given with a reason per line, and the only differences are the
  matrix entries of D-5.
- **SC-005** (AC-5): The two uploaded analyzer finding lists are equal,
  and one throwaway branch with a single added naming violation fails
  `static-analysis` on a warm cache and is then deleted.
- **SC-006** (AC-6): One cold-cache run and two warm-cache runs of one
  commit give an equal verdict in every job, and `tsan` and `sanitize`
  each run three times at the runner's full affinity.
- **SC-007** (AC-7): `tools/ci/prepush.sh` finishes in 5 min or less on
  the recorded lane host with a warm `ccache`; `--full` prints a time per
  step; an unknown option exits 2.
- **SC-008** (AC-8): No test entry exceeds its budget in any job, and the
  exceptions list carries a reason plus a measured CI number for each
  entry.
- **SC-009** (AC-9): `VERSION 0.20.0`, `SOVERSION 4` and `SOVERSION 12`
  equal master's.
- **SC-010**: The `## Quick start` section of `README.md` names both
  tiers and the pre-push script in 25 lines or fewer, and the page passes
  the drift gate.
- **SC-011**: The `static-analysis` finding list of the branch equals
  master's while `readability-identifier-naming` still fails a planted
  violation, so the gate keeps its bite after the split.
- **SC-012**: Every job of the branch's last run is green apart from
  the reds PC-10 records at the base, and the pull request names each
  such job, the base record that shows it predates this change, and the
  spec that owns it (FR-064, FR-065).

## Constraints

The request's hard rules bind this spec; they restate constitution VI,
VIII and IX. Summarized where they bind, with the requirement that
carries each:

- No gate weakens; coverage stays 100% lines and 100% branches with the
  `brace` filter, no waiver, no new `LCOV_EXCL` marker, no new
  `--ignore-errors` entry, and `cmake/coverage.cmake` stays as master has
  it (hard rule 1; FR-040, FR-041, FR-043).
- Every master job still runs, and each sanitizer job keeps its test set
  apart from the matrix entries (hard rule 2; FR-045, FR-046).
- No test is skipped; `SKIP_RETURN_CODE` keeps master's rule (hard rule
  3; FR-047).
- Determinism holds: no sleep, no clock read, no retry loop, no
  `--repeat until-pass` (hard rule 4; FR-005, FR-048).
- Both analyzers keep their unit set, configuration, flags and header
  filter, and no analyzer result is cached (hard rule 5; FR-017, FR-049).
- A cache changes no result; `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS`
  stay unset (hard rule 6; FR-010, FR-011).
- `RESOURCE_LOCK` and `RUN_SERIAL` are the mechanisms that keep tests
  sharing a resource apart, with the master precedent PC-1 names (hard
  rule 7; FR-004).
- No timeout is raised to make a test pass; a per-test budget replaces it
  (hard rule 8; FR-031 to FR-034).
- The `## Quick start` section of `README.md` carries both tiers and the
  pre-push script in 25 lines or fewer, and the page keeps its
  `PAGE_LINE_CAP` reading (hard rule 9; FR-050).
- Every CI job log is read after each push and this change's own failures
  are fixed (hard rule 10; FR-063, FR-064).
- A red the base already carries stays with the spec that owns it,
  and the single exception is the formatting-only commit that lets the
  job graph run at all (Clarifications of 2026-10-25; FR-064, FR-065).
- Rebase, `git push --force-with-lease`, and `git ls-remote` shown after
  each push (hard rule 11; FR-059).
- Commit template, footer order, and `prose_gate.py` before each push
  (hard rule 12; FR-060, FR-061).
- No ruling, acceptance or approval text naming a person as approver
  (hard rule 13; FR-062).
- Host validation, `tools/hostcheck/`, `validation/` and the `hostcheck`
  label stay in spec 031 (hard rule 14; Out of scope).

## Assumptions

- The hosted runner for a public repository keeps 4 vCPUs, so
  `-j $(nproc)` means 4 and `ctest -j 4` is the new concurrency. A runner
  bump changes only the number the command reads.
- The `test` and `coverage` lanes share one compiler family and the
  `sanitize` and `tsan` lanes share another (PC-6), so a matrix run in
  one lane of a family answers for the other. PC-5 and FR-024 carry the
  proof, and PC-6 is re-read at the branch audit point before the split
  is trusted.
- Analyzer findings other than `readability-identifier-naming` stay
  reported and non-failing, exactly as master's `.clang-tidy` and cppcheck
  launcher set them. This feature changes who runs the analyzers, and the
  finding list equality of SC-005 is the guard.
- The 10 GB repository cache budget is shared by all lanes, so
  `CCACHE_MAXSIZE` is set per lane and the sum of those sizes is
  checked against the budget.
- A pre-existing red at the base stays the property of the spec that
  owns it, and this spec records it in section C and PC-10 either way.
  FR-064 holds that decision and FR-065 holds its one exception.
- The lane host named for SC-007 is the machine the owner picks, and the
  script's timing record names it. No host validation is assumed here.
- clang-format 18.1.8 is the CI formatter, so a local run of the format
  step uses that version or stops (D-10).

## Constitution Checklist

- Main-page impact check (IV.1): yes, in one section. The `## Quick start`
  section of `README.md` names the two tiers and `tools/ci/prepush.sh` in
  25 lines or fewer (FR-050, SC-010). No library behavior, snippet
  region or quick-reference row changes, so no `example/` region moves
  and the drift, snippet and report-shape gates keep their current
  inputs.
- Principle VI and VIII: the coverage bar, the analyzer unit set, the
  sanitizer test sets, the job set and the determinism rules are restated
  as binding requirements (FR-040 to FR-049), because this feature moves
  the ground those gates stand on.
- Principle VIII, where the analyzer verdict is read: VIII names the
  `test` job's build log and a contributor's local build as the two
  places a finding shows up. FR-015 moves the CI reading to
  `static-analysis`, FR-013 keeps `ci-ubuntu` carrying both launchers for
  the local build, and FR-050 names the local command on the main page.
  The gate set of VIII is untouched: both launchers reach every unit the
  `ci-ubuntu` preset compiles and a naming finding fails the build
  (FR-016, FR-018, FR-045). VIII forbids a silent change and requires a
  written justification, so the pull request carries the statement above
  beside its gate table.
- Principle IX: this feature changes build configuration, so the full
  workflow applies. `plan.md` carries the logical and physical views of
  the job graph and the preset graph, plus the test plan for the tier and
  label gate.
- Principle X.1: the base reds are recorded with their settlement, and
  the choice is written down in the open (FR-064, FR-065); the analyzer option the request offers is refused with its
  reason (D-1); the redundant preset is kept with its reason (D-2).
- Principle X.3: the diff touches `.github/workflows/ci.yml`,
  `CMakePresets.json`, `test/CMakeLists.txt`, `example/CMakeLists.txt`,
  `test/tier_labels.sh`, `tools/ci/prepush.sh`, `tools/dbc/semantics_matrix.sh`,
  `tools/dbc/coverage_gate.sh`, `tools/docs/example_drift.py`, the
  `## Quick start` section FR-050 names, and the single formatting-only
  pass over `test/source/harness_cli_table_test.cpp` that FR-065 asks
  for. The readings the measurement requirements demand are written to
  `specs/030-ci-speedup/results/` as one short record per reading, and
  those records are repository files subject to the prose and spell
  gates. No library source, header, public option or schema key changes
  (FR-057).
- Principle XI: this file holds no em-dash, no contrastive framing, no
  filler vocabulary and no vouching word, and every command sits in a code
  span.

## Dependencies and Citations

- `gh api` reads of `charlesarcher/speedgun-ng` actions runs: 39708015373
  (`bd10d76`), 39910150207, 39883295074, 39941934864 (`d6d60f2`),
  40042928839 (`fea69c5`), 40135765542 (`7d8dfdd`), 40203022166
  (`f507b8f`, the base). Job and step durations come from
  `runs/<id>/jobs`; walls from `runs/<id>/timing`; per-test times from
  the job logs of 40042928839 and 39941934864.
- Repository reads at the base commit `f507b8f`: `.github/workflows/ci.yml`
  (803 lines, twenty `-j 2` sites, twelve jobs, no `concurrency`, no
  `timeout-minutes`), `CMakePresets.json` (preset inheritance of PC-3),
  `CMakeLists.txt` (`VERSION` at `:15`, `SOVERSION` at `:63` and `:281`,
  launcher clearing at `:145-146` and `:663-664`),
  `test/CMakeLists.txt` (`dbc_semantics_matrix` at `:139`,
  `RESOURCE_LOCK` at `:297`, nine `TIMEOUT` entries),
  `tools/dbc/semantics_matrix.sh` (362 lines, PC-5),
  `cmake/coverage.cmake` (PC-9), `tools/dbc/coverage_gate.sh`,
  `.clang-tidy:21`, `.gitignore:13`, `README.md` (834 lines).
- Roadmap: `/workspace/speedgun-ng-harness-roadmap.md`, item N0, proposal
  R-1 (the request's file, held outside the repository).
- Order and approval record for this feature, together with the reverted
  commits' rulings, live outside the repository, on the rule FR-062
  states. Spec 031 (`host-validation`) follows spec 032 (the three merged
  features).
