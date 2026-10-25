# Tasks: CI Speed-Up

**Input**: Design documents from `specs/030-ci-speedup/`

**Prerequisites**: [spec.md](spec.md) (FR-001 to FR-065, SC-001 to SC-012,
PC-1 to PC-10), [plan.md](plan.md) (P-01 to P-13), [research.md](research.md),
[data-model.md](data-model.md) (E-01 to E-10), [contracts/](contracts/),
[quickstart.md](quickstart.md)

**Tests**: two new `ctest` entries carry tests, because Principle III asks for
the test before the code and this change edits no library source.
`test_tier_labels` and the budget audit are written and seen failing before the
label and budget blocks land (T010 before T011 and T012). Every other task is
build or workflow configuration, so its proof is a reading from a CI run, a
`ctest -N` listing, or the coverage artifact, named in the task and written to
a handoff file under `specs/030-ci-speedup/results/`. No test sleeps, reads a
clock for its verdict, retries, or skips (FR-048).

**Organization**: tasks are grouped by user story so each story can be
implemented and tested on its own. Phase numbers map to the Task Planning
Summary in plan.md. The DAG section at the end is the machine-readable form a
workflow driver reads, one run per phase.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: its write set is disjoint from every other **[P]** task of its phase
  and it leaves no dependency open, so the group can run at once. Two tasks that
  edit the same file never both carry **[P]**. A **[P]** task may name
  dependencies that land before its wave, and the DAG section gives the exact
  wave order.
- **[Story]**: `US1` to `US8`, from spec.md. Setup, Foundational and Polish
  carry no story label.
- Every task names the exact file it creates or edits.

## File Lanes

Three files are shared by most of the feature, so a phase's parallel width comes
from distinct files, and same-file tasks form a chain.

| Lane | Files | Writers |
| --- | --- | --- |
| workflow | `.github/workflows/ci.yml` | T007, T013, T014, T016, T026 to T028, T048 to T050, T052, T055, T056 |
| presets | `CMakePresets.json` | T002, T005, T015, T034 |
| registration | `test/CMakeLists.txt`, `example/CMakeLists.txt` | T006, T009, T010, T011, T012, T033, T044 |
| tier audit | `test/tier_labels.sh` | T010, T039, T045 |
| scripts | `tools/dbc/semantics_matrix.sh`, `tools/dbc/coverage_gate.sh`, `tools/ci/prepush.sh` | T003, T020 to T024, T032, T054 |
| docs | `README.md`, `tools/docs/example_drift.py`, `quickstart.md` | T041, T042, T061 |
| evidence | `specs/030-ci-speedup/results/**` | the record tasks, one file per node |
| formatting | `test/source/harness_cli_table_test.cpp` | T001 |

Two runs of the DAG may run at once when their lane sets are disjoint. The
`workflow` lane is the widest bottleneck, so the strategy in the last section
spreads the workflow edits over the phases, three workflow nodes at most in one.

## Handoff Files

DAG nodes pass data through files, never through a prompt. Every record task
writes `specs/030-ci-speedup/results/<phase>/<node>.md` with the head sha, the
run id it read, the command, and the numbers. The convention is written by T004
and each node adds one file. A downstream node reads the named file, so a
re-run of an upstream node needs no prompt edit.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the three things everything else waits behind: a green `lint`, the
launcher preset, and the scaffold the DAG writes its evidence into.

- [X] T001 Format `test/source/harness_cli_table_test.cpp` with clang-format 18.1.8 (one pass is enough; the file is idempotent at the `hunt` and `master` ternary chain at `:886` to `:889`), confirm `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` passes, and commit it alone as the branch's first commit with the `<Section>: <one-line imperative>` form and a why-body (FR-065, constitution V)
- [X] T002 [P] Add the abstract configure preset `ci-cache` to `CMakePresets.json` inheriting `dev-mode` and carrying `CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER` set to `ccache`, naming no `generator` and no `binaryDir` so `cmake --preset=ci-cache` cannot configure (P-02, FR-006)
- [X] T003 [P] Write the scaffold of `tools/ci/prepush.sh`: `set -euo pipefail`, the `--help` and unknown-option handling (exit 2 before any step), the `--full` flag, and the `step()` helper that prints `prepush: <name> <seconds> s` (contracts/prepush-script.md)
- [X] T004 [P] Write `specs/030-ci-speedup/results/README.md` with the handoff convention: path form `results/<phase>/<node>.md`, the four fields every record carries (head sha, run id, command, numbers), and the rule that every record passes `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results` and `codespell specs/030-ci-speedup/results/` before its node reports done, because these records are repository files the `spell-check` and `prose-lint` jobs read (FR-051 to FR-056)

**Checkpoint**: `lint` would pass, `ci-cache` exists, and a node has a named
place to write its reading.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: parallelism that does not create flakes, the tier labels, and the
per-entry budget. Every story reads these.

**⚠️ CRITICAL**: no story work begins before this phase lands.

- [X] T005 [P] Add the presets `ci-ubuntu-test`, `prepush` and the test presets `prepush-quick` and `prepush-full` to `CMakePresets.json` (contracts/presets.md): `ci-ubuntu-test` inherits `ci-build`, `ci-linux`, `dev-mode` and `ci-cache` and sets `SPEEDGUN_DBC_MATRIX=ON` with no analyzer preset in its list; `prepush` inherits `ci-build`, `dev-mode` and `ci-cache` with `binaryDir` `${sourceDir}/build/prepush`; the two test presets point at the `prepush` configure preset, `prepush-quick` filtering the `quick` label, both with `noTestsAction` `error`. Leave `ci-ubuntu` exactly as master has it (FR-012, FR-013, FR-028, D-3, P-06)
- [X] T006 [P] Guard the single matrix registration at `test/CMakeLists.txt:139` with `if(SPEEDGUN_DBC_MATRIX)` inside the existing Linux block, and declare the `SPEEDGUN_TEST_JOBS` cache variable with default `2` (P-01, P-04)
- [X] T007 [P] In `.github/workflows/ci.yml`, replace every `-j 2` build and test site with `-j "$(nproc)"` except the instrumented suite at `:71`, which goes to `-j 1` (FR-001, FR-002, FR-044, P-11), and write the list of changed sites to `specs/030-ci-speedup/results/ph2/parallelism.md`
- [X] T008 [P] Write the collision audit to `specs/030-ci-speedup/results/ph2/collisions.md`: read the registrations that set `PROCESSORS` or `RESOURCE_LOCK` (`test/CMakeLists.txt:297`, `:660`), then grep `test/source/` for fixed file names written into the working directory, and name one lock per shared resource (FR-004, P-13). Known sites: `counters_trap_checked` scratch file, `harness_cli_test` writing `harness_cli_column_note.txt` into its working directory, `tools/dbc` scratch paths under `/tmp`
- [X] T009 In `test/CMakeLists.txt`, apply the locks and `PROCESSORS` values the audit names, keeping the master lock `counters_trap_checked_out` and the master `PROCESSORS 2` on `harness_cli_test`, and give `harness_cli_test` a lock of its own for `harness_cli_column_note.txt` (FR-003, FR-004, FR-005). Depends on T008
- [X] T010 Write `test/tier_labels.sh` and register the entry `test_tier_labels` in `test/CMakeLists.txt`: the script takes the build directory and the `ctest` program as arguments, and the registration passes `${CMAKE_BINARY_DIR}` and `${CMAKE_CTEST_COMMAND}`, so the entry runs from any working directory. It runs `ctest -N`, `ctest -N -L quick` and `ctest -N -L slow` against the build tree, fails on an overlap, fails when the union misses a name, fails when a listed slow entry is absent from the suite, fails when an entry carries no `TIMEOUT` and no exception row, and fails when a registration carries a sleep, a clock read for its verdict, a retry, or a `SKIP_RETURN_CODE` value master does not set. Run it and see it fail against the unlabeled suite before T011 and T012 land (FR-026, FR-031, FR-047, FR-048, Principle III). Depends on T006
- [X] T011 In `test/CMakeLists.txt`, add the `SPEEDGUN_SLOW_TESTS` list with one reason comment per row (`dbc_matrix_off_o0`, `dbc_matrix_off_o2`, `dbc_matrix_on_o0`, `dbc_matrix_on_o2`, `dbc_matrix_observe_off_o0`, `snippet_compile`) and a trailing block that reads the directory `TESTS` property and applies `LABELS slow` or `LABELS quick` to every entry (P-05, FR-025). Depends on T010
- [X] T012 In `test/CMakeLists.txt` and `example/CMakeLists.txt`, add the budget block that reads the directory `TESTS` property and sets `TIMEOUT 300` on every entry whose `TIMEOUT` is unset, leaving the eight master 120 s values (`test/CMakeLists.txt:580`, `:742`, `:1392`, `:1408`, `:1425`, `:1440`, `:1456`, `:1473`) and the 300 s `example_run` at `:1487` as they stand, and every `SKIP_RETURN_CODE` value master sets stays where master sets it (P-07, FR-031, FR-047). Depends on T011

**Checkpoint**: `ctest -R test_tier_labels` fails only on the checks the later
phases close, `cmake --preset=ci-ubuntu-test` configures, and the suite runs at
`nproc` with locks where resources are shared.

---

## Phase 3: User Story 1 - The Full Verdict Inside 15 Minutes (Priority: P1) 🎯 MVP

**Goal**: `ccache` behind every CI build with a key that names the compiler, the
preset and the job, so a warm build stops re-running 2277 s of compiles and the
verdict arrives inside 15 min (SC-002).

**Independent Test**: one run with the key forced to miss and two warm runs of
one commit: the per-job verdicts match, and the `Build` step time of each job is
reported beside its base 2277 s in `test` (quickstart.md, "Cache behavior").

- [X] T013 [US1] In `.github/workflows/ci.yml`, add the `ccache` install step and the `actions/cache` restore step to every building job, with `CCACHE_DIR` set to `${{ github.workspace }}/.ccache`, the key of D-9 joining the prefix `ccache`, the job name, the preset name, a hash of `${CXX:-c++} --version`, `hashFiles('CMakePresets.json', 'external/**')` and `github.run_id`, restore-keys holding that string with the run id dropped, and the save step gated to master pushes (FR-006 to FR-009, P-10)
- [X] T014 [US1] In `.github/workflows/ci.yml`, put `ccache -z` before and `ccache -s` after each `Build` step so the hit rate lands in the job log, and confirm no step sets `CCACHE_BASEDIR` or `CCACHE_SLOPPINESS` (FR-009, FR-011). Depends on T013
- [X] T015 [P] [US1] Add `ci-cache` to the `inherits` of `ci-sanitize`, `ci-tsan` and `ci-coverage` in `CMakePresets.json`, so every build in CI runs through the launcher beside the local `prepush` preset that already carries it (FR-006, FR-028)
- [X] T016 [US1] In `.github/workflows/ci.yml`, set `CCACHE_MAXSIZE` per lane and write the sizes with their sum against the 10 GB repository budget to `specs/030-ci-speedup/results/ph3/maxsize.md` (FR-007). Depends on T014
- [ ] T017 [P] [US1] Force one cold run by bumping the key prefix, then write `specs/030-ci-speedup/results/ph3/cold.md` with each job's `Build` time, `ccache -s` reading, and conclusion (FR-055, SC-006). Depends on T016
- [ ] T018 [P] [US1] Write `specs/030-ci-speedup/results/ph3/warm.md` with two warm runs of one commit: each job's `Build` time and hit rate, and the `CCACHE_DISABLE=1` run of the same commit for the trace equality of FR-010. Add the three `sanitize` runs and the three `tsan` runs at the runner's full affinity that SC-006 asks for, each with its verdict, and drop any run that was canceled (FR-055, SC-006). Depends on T014
- [ ] T019 [US1] Write `specs/030-ci-speedup/results/ph3/verdict.md` comparing the cold and warm runs job by job, naming any test that passes in one and fails in the other (FR-053, SC-006). Depends on T017, T018

**Checkpoint**: a warm `test` job `Build` step sits under its base 2277 s by
enough that the job's own wall time lands inside the 15 min median SC-002 sets, and
cold against warm verdicts agree.

---

## Phase 4: User Story 2 - One Local Pre-Push Command Inside 5 Minutes (Priority: P2)

**Goal**: `tools/ci/prepush.sh` runs the CI checks locally in 5 min or less on a
4-vCPU host with a warm cache, printing one timing line per step (SC-007).

**Independent Test**: `tools/ci/prepush.sh` prints one line per step and exits
0; `tools/ci/prepush.sh --nope` exits 2 with nothing run; the recorded lane-host
run finishes inside 5 min.

This story is one file, so its tasks form a chain. Its parallelism comes from
running the whole phase beside a phase whose lanes do not include
`tools/ci/prepush.sh`.

- [X] T020 [US2] In `tools/ci/prepush.sh`, add steps 1 to 5 through `step()`: `python3 tools/docs/example_drift.py .`, `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, `bash test/time_source_gate.sh .`, `cmake -P cmake/spell.cmake`, `python3 tools/prose/prose_gate.py --base origin/master` (FR-029, D-10). Depends on T003
- [X] T021 [US2] In `tools/ci/prepush.sh`, add steps 6 to 9: `cmake --preset=prepush`, `cmake --build --preset prepush -j "$(nproc)"`, `ctest --preset prepush-quick -j "$(nproc)"`, and `taskset -c 0,1 ctest --preset prepush-quick -j 2` (FR-029, FR-030, D-6). Depends on T020
- [X] T022 [US2] In `tools/ci/prepush.sh`, add step 10: `clang-tidy --checks=-*,readability-identifier-naming` over the units `git diff --name-only origin/master` names, with the same header filter and the same failing check CI uses (FR-030, FR-016). Depends on T021
- [X] T023 [US2] In `tools/ci/prepush.sh`, add the three `--full` steps: the slow tier through `ctest --preset prepush-full`, the consumer-release configure and build the `consumer-release` job runs, and the `ci-tsan` configure and link (FR-029, D-10). Depends on T022
- [X] T024 [US2] In `tools/ci/prepush.sh`, add the fault paths: unknown option exits 2 before any step, a `clang-format --version` other than 18.1.8 stops before formatting with the version found printed, a missing tool exits 1 naming the tool, and a failing step prints its own command and exit status. Add a `--self-check` mode that drives each fault path in place, prints one `self-check: <name> <status>` line per path, and exits 0 only when every path behaves as written, which is the proof route plan.md's Test Plan names for shell code (FR-030, VI). Depends on T023
- [ ] T025 [US2] Write `specs/030-ci-speedup/results/ph4/timing.md` with a full local run on the named lane host: the timing line of every step, their sum, the wall time, and the cache state, and the `--self-check` lines (FR-056, SC-007). Depends on T024

**Checkpoint**: the script is the whole local verdict and its timing lines sum
to within the wall time.

---

## Phase 5: User Story 3 - The Analyzer Verdict as Its Own Job (Priority: P3)

**Goal**: a sharded `static-analysis` job carries both analyzers, so the test
job builds without them and no analyzer is skipped or narrowed (SC-005, SC-011).

**Independent Test**: the union of the three shards' `file:line:check` findings
equals master's list, and a branch carrying one planted
`readability-identifier-naming` violation fails `static-analysis` (quickstart.md,
"The analyzer split").

- [X] T026 [US3] In `.github/workflows/ci.yml`, add the job `static-analysis` with `needs: [lint]`, a three-value matrix named `library`, `tests`, `examples`, the `ci-ubuntu` configure, and the three build scopes of P-08: `--target speedgun-ng_speedgun-ng speedgun-ng_harness`, `cmake --build build/test`, `cmake --build build/example` (FR-015, FR-018, D-1)
- [X] T027 [US3] In `.github/workflows/ci.yml`, strip the analyzer install step and the analyzer build from the `test` job so it configures `ci-ubuntu-test`, keeping its install and audit steps as they are (FR-015, FR-045). Depends on T026
- [X] T028 [US3] In `.github/workflows/ci.yml`, make each shard print the count of translation units it compiled and upload its sorted `file:line:check` finding list as a named artifact (FR-018, FR-054). Depends on T027
- [ ] T029 [P] [US3] Write `specs/030-ci-speedup/results/ph5/finding-diff.md` holding master's finding list from the last master run beside the union of the branch's three shard artifacts, with the diff of the two sorted sets (FR-049, SC-005, SC-011). Depends on T028
- [ ] T030 [P] [US3] On a throwaway branch cut from the feature branch, plant one `readability-identifier-naming` violation in a library unit, push, and write `specs/030-ci-speedup/results/ph5/planted.md` naming the shard that failed and the finding line; delete the throwaway branch afterwards (FR-016, SC-011). Depends on T028
- [ ] T031 [US3] Repeat the planted-violation run with a warm cache and write `specs/030-ci-speedup/results/ph5/warm-bite.md`, which records that the analyzer still ran before the compile and the shard still failed (FR-017). Depends on T030

**Checkpoint**: `test` builds without analyzers, and the analyzer verdict lands
in its own job with an unchanged finding set.

---

## Phase 6: User Story 4 - One Matrix Row per Cell and per Compiler (Priority: P4)

**Goal**: five `ctest` entries replace one 308 s aggregate, on the three lanes
whose compilers differ, so a break names its cell and its compiler (SC-004).

**Independent Test**: `ctest -N` on a `ci-ubuntu-test` tree lists the five
entries and a `ci-coverage` tree lists none, and the per-job test-list difference
against master holds only the matrix moves (contracts/test-registration.md).

- [X] T032 [P] [US4] In `tools/dbc/semantics_matrix.sh`, accept the optional second argument naming one cell, keep `run_cell` intact for that cell, keep the no-argument form running all five, read `JOBS` from the environment in place of the `nproc` read at `:60`, and write the probe sources once per invocation. Each run prints its cell name, its `JOBS` value and its compiler version on the first three lines, so the record FR-023 asks for lands in the job log of the lane that ran it (FR-019 to FR-023, P-03, P-04)
- [X] T033 [US4] In `test/CMakeLists.txt`, replace the single `dbc_semantics_matrix` registration at `:139` with the five entries `dbc_matrix_off_o0`, `dbc_matrix_off_o2`, `dbc_matrix_on_o0`, `dbc_matrix_on_o2`, `dbc_matrix_observe_off_o0`, each passing its script cell name and `SPEEDGUN_TEST_JOBS`, each with `PROCESSORS` set to `SPEEDGUN_TEST_JOBS` (FR-019, FR-003, D-4). Depends on T032
- [X] T034 [P] [US4] Set `SPEEDGUN_DBC_MATRIX=ON` in `ci-ubuntu-test`, `ci-rocky` and `ci-tsan` in `CMakePresets.json`, leaving it unset in `ci-coverage` and `ci-sanitize` (P-01, D-5). Depends on T005
- [X] T035 [P] [US4] Write `specs/030-ci-speedup/results/ph6/lanes.md` reading, per lane, whether the five entries are registered, from `ctest -N` of that lane's tree (FR-023, FR-024). Depends on T033, T034
- [X] T036 [P] [US4] Write `specs/030-ci-speedup/results/ph6/compiler-records.md` with each registering lane's compiler version, read from that lane's own configure log and cross-checked against the version its matrix entry prints, and re-read PC-6 at the branch head (FR-023, D-5). Depends on T033
- [X] T037 [P] [US4] Write `specs/030-ci-speedup/results/ph6/list-diff.md` with the `ctest -N` difference against master for each of the five lanes, so the only differences are the matrix entries (FR-053, SC-004). Depends on T033
- [X] T038 [P] [US4] Write `specs/030-ci-speedup/results/ph6/dropping-lanes.md` with the FR-024 evidence: the script configures with no preset and `CMAKE_BUILD_TYPE=None` with its own `CMAKE_CXX_FLAGS` (`tools/dbc/semantics_matrix.sh:181`, `:182`), reads only `CXX` (`:54`), builds under `mktemp -d` (`:76`), and adds no count to a coverage trace (FR-024, D-5). Depends on T032

**Checkpoint**: five rows, three lanes, and a named compiler per lane in the
job log.

---

## Phase 7: User Story 5 - The Quick Tier Locally while CI Keeps Everything (Priority: P5)

**Goal**: the labels partition the suite, CI keeps passing no filter, and the
main page teaches both tiers (SC-010).

**Independent Test**: the two label lists are disjoint, their union equals
`ctest -N`, and no CI `ctest` line carries `-L` or `-LE`.

- [X] T039 [P] [US5] In `test/tier_labels.sh`, close the label checks against the filled list: the two label lists partition `ctest -N`, no entry carries both labels, and every `SPEEDGUN_SLOW_TESTS` row names an entry that exists (FR-025, FR-026). Depends on T011
- [X] T040 [P] [US5] Read every `ctest` line in `.github/workflows/ci.yml` and write `specs/030-ci-speedup/results/ph7/no-filter.md` listing each line with its `-L` and `-LE` state; add a filter nowhere (FR-027)
- [ ] T041 [P] [US5] Add to `README.md`, inside `## Quick start`, a plain fenced `sh` block with one sentence each side that names the `quick` and `slow` tiers and `tools/ci/prepush.sh`, in 25 lines or fewer (FR-050, SC-010, P-09)
- [ ] T042 [US5] Move `PAGE_LINE_CAP` in `tools/docs/example_drift.py:192` from 835 to the measured page length with the file's comment form naming the block that moved it, and confirm `python3 tools/docs/example_drift.py .` passes (P-09). Depends on T041
- [ ] T043 [P] [US5] Write `specs/030-ci-speedup/results/ph7/tiers.md` with the quick tier's time and the full suite's time on the same tree, and the two label lists' entry counts (SC-007 context, FR-025). Depends on T039

**Checkpoint**: the local quick tier is provably a subset, and CI still runs
everything.

---

## Phase 8: User Story 6 - A Slow Test Fails in Its Own Change (Priority: P6)

**Goal**: no entry sits under the CTest default, so a hang costs 300 s in the
change that caused it (SC-008).

**Independent Test**: the audit entry passes, and no entry exceeds its budget in
any lane of the branch's last run.

- [ ] T044 [US6] In `test/CMakeLists.txt` and `example/CMakeLists.txt`, write the exceptions block beside the budget loop: one row per entry that keeps a value other than 300, with its reason and the per-lane number read from a CI run of this branch (FR-032, D-7). Depends on T012, T037
- [ ] T045 [US6] In `test/tier_labels.sh`, close the budget check: every entry reports a `TIMEOUT` or an exception row, and the eight master 120 s entries plus `example_run` carry their listed values (FR-031, FR-033). Depends on T044
- [ ] T046 [US6] Write `specs/030-ci-speedup/results/ph8/audit.md` with the slowest entries per lane read from the job logs of the branch's last run, each beside its budget (FR-033, SC-008). Depends on T044
- [ ] T047 [US6] Write `specs/030-ci-speedup/results/ph8/exceptions.md` holding the final exceptions table with its measured numbers, and confirm no `ctest` line carries `--timeout` and no job carries a raised `timeout-minutes` (FR-032, FR-034). Depends on T046

**Checkpoint**: the budget audit passes and the exceptions carry numbers.

---

## Phase 9: User Story 7 - A Toolchain Install Cannot Own a Job (Priority: P7)

**Goal**: package lists survive across runs, so a 991 s install step stops
deciding the wall time (SC-002).

**Independent Test**: each install step's time is recorded, and a warm run shows
the same or a shorter install than the base 991 s worst case.

- [ ] T048 [US7] In `.github/workflows/ci.yml`, cache the rocky container's dnf packages through `keepcache` and `actions/cache`, and reorder its install step to install the minimal bootstrap set first (FR-014, FR-035)
- [ ] T049 [US7] In `.github/workflows/ci.yml`, cache the ubuntu runners' apt archives with `actions/cache` on the package list directories, keeping the package versions CI installs unchanged (FR-014, FR-036). Depends on T048
- [ ] T050 [US7] In `.github/workflows/ci.yml`, confirm every install step keeps a name that `gh run view --json steps` reports, and print nothing extra (FR-037). Depends on T049
- [ ] T051 [US7] Write `specs/030-ci-speedup/results/ph9/install-steps.md` with every install step's time on a warm run and on a cache-miss run, and the install-step time of each of the six runs FR-052 counts (FR-037, FR-052, SC-002). Depends on T050

**Checkpoint**: the install steps carry recorded times and no longer own a job.

---

## Phase 10: User Story 8 - Cancel the Run a New Push Superseded (Priority: P8)

**Goal**: a superseded pull-request run stops and gives up its runner slot, so
the next run starts on a free machine.

**Independent Test**: two pushes to one pull-request ref leave the older run
canceled, and a master run stands to its end.

- [ ] T052 [US8] In `.github/workflows/ci.yml`, add the top-level `concurrency` key with `group: ${{ github.workflow }}-${{ github.ref }}` and `cancel-in-progress` true only for the `pull_request` event (FR-039)
- [ ] T053 [US8] Write `specs/030-ci-speedup/results/ph10/cancel.md` with the two push runs on one pull-request ref, naming which reports canceled and which ran, plus one master run that completed, and confirm no canceled run enters a timing table (FR-039, SC-002, story 8). Depends on T052

**Checkpoint**: the cancellation is observed and no master measurement is
canceled.

---

## Phase 11: Polish & Cross-Cutting Concerns

**Purpose**: the coverage lane, the `docs` edge, the tables, and the full gate
run.

- [X] T054 [P] In `tools/dbc/coverage_gate.sh`, capture the status of the `lcov --branch-coverage --summary` call, which the `|| true` at the call swallows today, and add `exit 3` for a tool failure where `lcov` exited nonzero and its summary shows every gated measure at 100.0% or shows no measure at all, keeping `exit 0` for a pass, `exit 1` for a shortfall and `exit 2` for a usage fault, with the refusal message naming `lcov` as the failing tool. Reach both new exits through a fixture trace: a short trace for the shortfall exit and a real `lcov --summary` refusal for the tool-failure exit, and no fixture fabricates a passing trace (FR-043, P-12)
- [ ] T055 [P] In `.github/workflows/ci.yml`, settle the coverage lane: keep `-j 1` at `:71` or move to `-j "$(nproc)"` with `RESOURCE_LOCK` over the exec-child entries named by T008, and write the step time beside the base 334 s to `specs/030-ci-speedup/results/ph11/coverage-lane.md` (FR-044, D-8, P-11)
- [ ] T056 In `.github/workflows/ci.yml`, add `static-analysis` to the `needs` of `docs` at `:759`, leaving the other three entries in place (FR-038, D-12). Depends on T026, T055
- [ ] T057 [P] Write `specs/030-ci-speedup/results/ph11/trace-equality.md` comparing the branch's `coverage-info` artifact against the base tree's on the `SF` file list and every `DA` and `BRDA` record, and confirming no new `LCOV_EXCL` marker, no ignore class, no pin and no raised ceiling (FR-010, FR-040 to FR-042, SC-003). Depends on T054, T055
- [ ] T058 [P] Write `specs/030-ci-speedup/results/ph11/tables.md` with the before and after wall table read from run 39708015373 and the re-measured run of the spec's section B against the last run of the branch, the per-job `Build` and `Test` step table, the median wall time of three master pushes and three pull-request pushes of the branch with a warm cache, the cache-miss wall time and slowest job, the analyzer unit counts, and one run of the PR #47 head timed with its slowest job named. Take the readings from runs that ran to completion (FR-051, FR-052, FR-054, SC-001, SC-002). Depends on T057
- [ ] T059 Write `specs/030-ci-speedup/results/ph11/pr-report.md` holding the pull-request text: the two tables, the `ccache -s` hit rates, the `ctest -N` list difference with its reason per line, the finding-list proof, the residual-need list copied from spec.md, the commit map with each reverted commit's verdict, the base reds named with their base records and owning specs, the `Approved-by: OmO` footer line, and no `Co-Authored-By` trailer (FR-053, FR-056, FR-060 to FR-064, SC-012). Depends on T058
- [ ] T060 Run every gate in the plan's Implementation Strategy against the branch head and write the results to `specs/030-ci-speedup/results/ph11/gates.md`, including `cmake --preset=ci-rocky` with build and `ctest`, `ci-ubuntu`, `ci-sanitize`, `ci-tsan`, the coverage target, `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`, `cmake -P cmake/prose-lint.cmake`, `python3 tools/docs/example_drift.py .`, `bash test/time_source_gate.sh .`, and the DBC gates. Add the three equality reads the scope requirements ask for: `VERSION 0.20.0` with both `SOVERSION` values equal to master's, no new command-line option, environment variable or schema key against master, and the prose gate plus `codespell` over `specs/030-ci-speedup/results/` (FR-057, FR-058, SC-009). Depends on T056, T057
- [ ] T061 Reconcile `specs/030-ci-speedup/quickstart.md` with what landed: every command form, artifact name and exit status it prints must match the branch. Depends on T060

**Checkpoint**: every SC has a reading in `results/`, and the pull request can be
written from `pr-report.md`.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 Setup**: starts immediately. T001 gates everything, because `lint`
  skips the other eleven jobs while it is red.
- **Phase 2 Foundational**: needs Phase 1. Blocks every story: the labels, the
  budgets, the locks and the `ci-ubuntu-test` preset are read by later phases.
- **Phases 3 to 10 (US1 to US8)**: need Phase 2. Cross-story edges below.
- **Phase 11 Polish**: needs the stories its readings cover.

### Cross-Story Edges

- US3 needs US1: the analyzer job builds on a warm cache, and FR-017's bite is
  measured there (T026 after T013).
- US4 needs the Phase 2 guard and `SPEEDGUN_TEST_JOBS` (T033 after T006).
- US6 needs US4's per-lane logs for its exception numbers (T044 after T037).
- US5 needs the Phase 2 label list (T039 after T011).
- US8 needs nothing beside Phase 2, so it can land at any point after it.
- Phase 11 needs US1, US3, US4 and US6 for the tables and the trace equality.

### Lane Contention

- `workflow` (`.github/workflows/ci.yml`) is written by T007, T013, T014, T016,
  T026 to T028, T048 to T050, T052, T055, T056: one writer at a time, in that
  order.
- `presets` (`CMakePresets.json`) is written by T002, T005, T015, T034.
- `registration` is written by T006, T009 to T012, T033, T044.
- `tier audit` is written by T010, T039, T045.
- The `evidence` lane has one file per node, so its tasks are always parallel
  with each other.

### Rebase Note

`specs/031`, `specs/032` and `specs/033` run on other machines and touch
`.github/workflows/ci.yml`, `CMakePresets.json` and `test/CMakeLists.txt`.
Rebase onto `master` before each phase that edits a shared file, keep the
formatting-only commit first, and re-run `ctest -R test_tier_labels` after each
rebase, because a rebased registration can drop a label or a budget.

---

## DAG for a Workflow Driver

One run per phase, `key` stable so a re-start returns the existing run. Each
node names its tasks, its write set and its upstream nodes. A node prompt is the
line below plus "follow task `T###` in `specs/030-ci-speedup/tasks.md` and write
the named handoff file": the file carries the detail, so a retry needs no prompt
edit. Categories follow the driver's routing table: `quick` for a single-file
mechanical edit, `unspecified-low` for a configuration change with a reading to
check, `unspecified-high` where several files and a measurement meet. No run
exceeds eight nodes.

```json
{
  "feature": "030-ci-speedup",
  "runs": [
    {
      "key": "030-ph1", "name": "Setup", "phase": 1,
      "nodes": [
        {"id": "format", "tasks": ["T001"], "category": "quick", "files": ["test/source/harness_cli_table_test.cpp"], "dependsOn": []},
        {"id": "preset_cache", "tasks": ["T002"], "category": "quick", "files": ["CMakePresets.json"], "dependsOn": []},
        {"id": "scaffold", "tasks": ["T003"], "category": "quick", "files": ["tools/ci/prepush.sh"], "dependsOn": []},
        {"id": "ledger", "tasks": ["T004"], "category": "quick", "files": ["specs/030-ci-speedup/results/README.md"], "dependsOn": []}
      ]
    },
    {
      "key": "030-ph2", "name": "Foundational", "phase": 2, "requires_runs": ["030-ph1"],
      "nodes": [
        {"id": "preset_test", "tasks": ["T005"], "category": "unspecified-low", "files": ["CMakePresets.json"], "dependsOn": []},
        {"id": "matrix_guard", "tasks": ["T006"], "category": "unspecified-low", "files": ["test/CMakeLists.txt"], "dependsOn": []},
        {"id": "parallelism", "tasks": ["T007"], "category": "unspecified-low", "files": [".github/workflows/ci.yml", "specs/030-ci-speedup/results/ph2/parallelism.md"], "dependsOn": []},
        {"id": "audit", "tasks": ["T008"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph2/collisions.md"], "dependsOn": []},
        {"id": "locks", "tasks": ["T009"], "category": "unspecified-low", "files": ["test/CMakeLists.txt"], "dependsOn": ["audit", "matrix_guard"]},
        {"id": "tier_test", "tasks": ["T010"], "category": "unspecified-high", "files": ["test/tier_labels.sh", "test/CMakeLists.txt"], "dependsOn": ["locks"]},
        {"id": "tier_list", "tasks": ["T011"], "category": "unspecified-low", "files": ["test/CMakeLists.txt"], "dependsOn": ["tier_test"]},
        {"id": "budgets", "tasks": ["T012"], "category": "unspecified-low", "files": ["test/CMakeLists.txt", "example/CMakeLists.txt"], "dependsOn": ["tier_list"]}
      ]
    },
    {
      "key": "030-ph3", "name": "US1 cache", "phase": 3, "requires_runs": ["030-ph2"],
      "nodes": [
        {"id": "cache_steps", "tasks": ["T013"], "category": "unspecified-high", "files": [".github/workflows/ci.yml"], "dependsOn": []},
        {"id": "cache_stats", "tasks": ["T014"], "category": "quick", "files": [".github/workflows/ci.yml"], "dependsOn": ["cache_steps"]},
        {"id": "preset_inherit", "tasks": ["T015"], "category": "quick", "files": ["CMakePresets.json"], "dependsOn": []},
        {"id": "maxsize", "tasks": ["T016"], "category": "quick", "files": [".github/workflows/ci.yml", "specs/030-ci-speedup/results/ph3/maxsize.md"], "dependsOn": ["cache_stats"]},
        {"id": "cold", "tasks": ["T017"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph3/cold.md"], "dependsOn": ["maxsize"]},
        {"id": "warm", "tasks": ["T018"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph3/warm.md"], "dependsOn": ["cache_stats"]},
        {"id": "verdict", "tasks": ["T019"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph3/verdict.md"], "dependsOn": ["cold", "warm"]}
      ]
    },
    {
      "key": "030-ph4", "name": "US2 prepush", "phase": 4, "requires_runs": ["030-ph2"],
      "nodes": [
        {"id": "prepush_gates", "tasks": ["T020"], "category": "unspecified-low", "files": ["tools/ci/prepush.sh"], "dependsOn": []},
        {"id": "tier_run", "tasks": ["T021"], "category": "unspecified-low", "files": ["tools/ci/prepush.sh"], "dependsOn": ["prepush_gates"]},
        {"id": "naming", "tasks": ["T022"], "category": "unspecified-low", "files": ["tools/ci/prepush.sh"], "dependsOn": ["tier_run"]},
        {"id": "full_steps", "tasks": ["T023"], "category": "quick", "files": ["tools/ci/prepush.sh"], "dependsOn": ["naming"]},
        {"id": "faults", "tasks": ["T024"], "category": "quick", "files": ["tools/ci/prepush.sh"], "dependsOn": ["full_steps"]},
        {"id": "timing", "tasks": ["T025"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph4/timing.md"], "dependsOn": ["faults"]}
      ]
    },
    {
      "key": "030-ph5", "name": "US3 analyzers", "phase": 5, "requires_runs": ["030-ph3"],
      "nodes": [
        {"id": "job", "tasks": ["T026"], "category": "unspecified-high", "files": [".github/workflows/ci.yml"], "dependsOn": []},
        {"id": "strip_test", "tasks": ["T027"], "category": "unspecified-low", "files": [".github/workflows/ci.yml"], "dependsOn": ["job"]},
        {"id": "artifacts", "tasks": ["T028"], "category": "quick", "files": [".github/workflows/ci.yml"], "dependsOn": ["strip_test"]},
        {"id": "finding_diff", "tasks": ["T029"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph5/finding-diff.md"], "dependsOn": ["artifacts"]},
        {"id": "planted", "tasks": ["T030"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph5/planted.md"], "dependsOn": ["artifacts"]},
        {"id": "warm_bite", "tasks": ["T031"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph5/warm-bite.md"], "dependsOn": ["planted"]}
      ]
    },
    {
      "key": "030-ph6", "name": "US4 matrix", "phase": 6, "requires_runs": ["030-ph2"],
      "nodes": [
        {"id": "script", "tasks": ["T032"], "category": "unspecified-high", "files": ["tools/dbc/semantics_matrix.sh"], "dependsOn": []},
        {"id": "entries", "tasks": ["T033"], "category": "unspecified-low", "files": ["test/CMakeLists.txt"], "dependsOn": ["script"]},
        {"id": "lane_flag", "tasks": ["T034"], "category": "quick", "files": ["CMakePresets.json"], "dependsOn": []},
        {"id": "lanes", "tasks": ["T035"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph6/lanes.md"], "dependsOn": ["entries", "lane_flag"]},
        {"id": "compilers", "tasks": ["T036"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph6/compiler-records.md"], "dependsOn": ["entries"]},
        {"id": "list_diff", "tasks": ["T037"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph6/list-diff.md"], "dependsOn": ["entries"]},
        {"id": "dropping", "tasks": ["T038"], "category": "unspecified-low", "files": ["specs/030-ci-speedup/results/ph6/dropping-lanes.md"], "dependsOn": ["script"]}
      ]
    },
    {
      "key": "030-ph7", "name": "US5 tiers", "phase": 7, "requires_runs": ["030-ph2"],
      "nodes": [
        {"id": "partition", "tasks": ["T039"], "category": "unspecified-low", "files": ["test/tier_labels.sh"], "dependsOn": []},
        {"id": "no_filter", "tasks": ["T040"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph7/no-filter.md"], "dependsOn": []},
        {"id": "readme", "tasks": ["T041"], "category": "quick", "files": ["README.md"], "dependsOn": []},
        {"id": "page_cap", "tasks": ["T042"], "category": "quick", "files": ["tools/docs/example_drift.py"], "dependsOn": ["readme"]},
        {"id": "tiers", "tasks": ["T043"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph7/tiers.md"], "dependsOn": ["partition"]}
      ]
    },
    {
      "key": "030-ph8", "name": "US6 budgets", "phase": 8, "requires_runs": ["030-ph6"],
      "nodes": [
        {"id": "exceptions", "tasks": ["T044"], "category": "unspecified-low", "files": ["test/CMakeLists.txt", "example/CMakeLists.txt"], "dependsOn": []},
        {"id": "budget_audit", "tasks": ["T045"], "category": "unspecified-low", "files": ["test/tier_labels.sh"], "dependsOn": ["exceptions"]},
        {"id": "per_entry", "tasks": ["T046"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph8/audit.md"], "dependsOn": ["exceptions"]},
        {"id": "exception_table", "tasks": ["T047"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph8/exceptions.md"], "dependsOn": ["per_entry"]}
      ]
    },
    {
      "key": "030-ph9", "name": "US7 package caches", "phase": 9, "requires_runs": ["030-ph2"],
      "nodes": [
        {"id": "dnf_cache", "tasks": ["T048"], "category": "unspecified-low", "files": [".github/workflows/ci.yml"], "dependsOn": []},
        {"id": "apt_cache", "tasks": ["T049"], "category": "unspecified-low", "files": [".github/workflows/ci.yml"], "dependsOn": ["dnf_cache"]},
        {"id": "step_names", "tasks": ["T050"], "category": "quick", "files": [".github/workflows/ci.yml"], "dependsOn": ["apt_cache"]},
        {"id": "install_times", "tasks": ["T051"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph9/install-steps.md"], "dependsOn": ["step_names"]}
      ]
    },
    {
      "key": "030-ph10", "name": "US8 concurrency", "phase": 10, "requires_runs": ["030-ph2"],
      "nodes": [
        {"id": "concurrency", "tasks": ["T052"], "category": "quick", "files": [".github/workflows/ci.yml"], "dependsOn": []},
        {"id": "cancel_record", "tasks": ["T053"], "category": "quick", "files": ["specs/030-ci-speedup/results/ph10/cancel.md"], "dependsOn": ["concurrency"]}
      ]
    },
    {
      "key": "030-ph11", "name": "Polish", "phase": 11,
      "requires_runs": ["030-ph3", "030-ph4", "030-ph5", "030-ph6", "030-ph7", "030-ph8", "030-ph9", "030-ph10"],
      "nodes": [
        {"id": "gate_exits", "tasks": ["T054"], "category": "unspecified-high", "files": ["tools/dbc/coverage_gate.sh"], "dependsOn": []},
        {"id": "coverage_lane", "tasks": ["T055"], "category": "unspecified-high", "files": [".github/workflows/ci.yml", "specs/030-ci-speedup/results/ph11/coverage-lane.md"], "dependsOn": []},
        {"id": "docs_needs", "tasks": ["T056"], "category": "quick", "files": [".github/workflows/ci.yml"], "dependsOn": ["coverage_lane"]},
        {"id": "trace_equality", "tasks": ["T057"], "category": "unspecified-high", "files": ["specs/030-ci-speedup/results/ph11/trace-equality.md"], "dependsOn": ["gate_exits", "coverage_lane"]},
        {"id": "tables", "tasks": ["T058"], "category": "unspecified-high", "files": ["specs/030-ci-speedup/results/ph11/tables.md"], "dependsOn": ["trace_equality"]},
        {"id": "pr_report", "tasks": ["T059"], "category": "writing", "files": ["specs/030-ci-speedup/results/ph11/pr-report.md"], "dependsOn": ["tables"]},
        {"id": "gates", "tasks": ["T060"], "category": "unspecified-high", "files": ["specs/030-ci-speedup/results/ph11/gates.md"], "dependsOn": ["docs_needs", "trace_equality"]},
        {"id": "quickstart", "tasks": ["T061"], "category": "quick", "files": ["specs/030-ci-speedup/quickstart.md"], "dependsOn": ["gates"]}
      ]
    }
  ]
}
```

### Driving the Runs

Start one run per phase, in phase order, and let the waves work inside a run.
Start a second run only when its `requires_runs` are settled and its file set is
disjoint from every running node: `030-ph4`, `030-ph6`, `030-ph7`, `030-ph9` and
`030-ph10` compete for the `workflow` or `registration` lanes, so at most one
`workflow` writer and one `registration` writer run at a time. Inside one run,
`030-ph7` fans out widest, with five nodes over five distinct files.

Recovery follows the driver's per-node verbs: retry one failed node with an
amended prompt, and a settled run reuses every completed node's output. A record
node that read a stale handoff file is retried, never the implementation node
beside it. Two driver notes from past runs here: node ids are literal strings in
every `dependsOn`, because the SDK's `node()` hands back no id; and `tasks.md`
has one writer, the driver, so a node ticks nothing and reports its own task ids
in its handoff file instead.

---

## Parallel Example: Phase 2

```bash
# Wave 1, four disjoint write sets:
Task: "T005 ci-ubuntu-test in CMakePresets.json"
Task: "T006 matrix guard in test/CMakeLists.txt"
Task: "T007 -j sites in .github/workflows/ci.yml plus its list in results/ph2"
Task: "T008 collision audit into results/ph2/collisions.md"

# Wave 2, once the audit and the guard land:
Task: "T009 locks from the audit in test/CMakeLists.txt"

# Wave 3 to 5, one writer on test/CMakeLists.txt at a time:
Task: "T010 test/tier_labels.sh and its registration"
Task: "T011 SPEEDGUN_SLOW_TESTS and the label block in test/CMakeLists.txt"
Task: "T012 budget block in test/CMakeLists.txt and example/CMakeLists.txt"
```

## Parallel Example: Phase 6

```bash
# Wave 1, two files:
Task: "T032 cell argument in tools/dbc/semantics_matrix.sh"
Task: "T034 SPEEDGUN_DBC_MATRIX in CMakePresets.json"

# Wave 2, the entries and the evidence that needs only the script:
Task: "T033 five entries in test/CMakeLists.txt"
Task: "T038 results/ph6/dropping-lanes.md"

# Wave 3, three evidence files at once:
Task: "T035 results/ph6/lanes.md"
Task: "T036 results/ph6/compiler-records.md"
Task: "T037 results/ph6/list-diff.md"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Phase 1: the formatting-only commit, `ci-cache`, the scaffold.
2. Phase 2: parallelism, locks, labels, budgets. This is where the suite stops
   running two jobs at a time.
3. Phase 3: `ccache` end to end, cold against warm verdicts equal.
4. **STOP and VALIDATE**: the `Build` step of `test` under its base 2277 s, the
   two traces equal under `CCACHE_DISABLE=1`, and nothing else moved. That is
   AC-2 and AC-4 in part, and it is the largest single saving.

### Incremental Delivery

1. Setup plus Foundational → the branch builds at `nproc` and every entry has a
   label and a budget.
2. US1 → the cache lands. Push and read the wall table (SC-001, SC-002).
3. US2 → the local 5-minute check exists (SC-007).
4. US3 → the analyzers move out and the finding set proves equal (SC-005).
5. US4 → five rows on three lanes (SC-004).
6. US5 → the tiers are taught on the page (SC-010).
7. US6 → no entry sits under a default ceiling (SC-008).
8. US7 → install steps stop owning a job (SC-002).
9. US8 → a superseded run cancels (story 8).
10. Phase 11 → the coverage lane, the `docs` edge, the tables, the gate run, the
    pull-request text (SC-003, SC-009, SC-011, SC-012).

### Order Rules

1. The formatting-only commit is first and alone, because `lint` gates the graph.
2. One writer per lane at a time. A second node never edits a file an open node
   holds.
3. A test task lands before the task it gates: T010 before T011 and T012.
4. Every evidence task runs after the implementation whose numbers it quotes, and
   writes its own file, so two evidence nodes never collide.
5. Rebase before each phase that edits a shared file, then re-run
   `test_tier_labels`.
6. Commit per task or per logical group with the `<Section>: <one-line
   imperative>` form, a why-body, `Fixes #n`, `Refs: specs/030-ci-speedup` and
   `Approved-by: OmO` (FR-060, FR-061). Push after a green local gate run
   (FR-062).
7. A node never edits `specs/030-ci-speedup/tasks.md`: the driver ticks the box
   when the node's handoff file shows the work, because a shared checklist under
   concurrent nodes loses ticks.

---

## Notes

- 61 tasks, `T001` to `T061`. Per story: US1 7, US2 6, US3 6, US4 7, US5 5, US6
  4, US7 4, US8 2, plus Setup 4, Foundational 8 and Polish 8.
- 26 tasks carry **[P]**. The count is bounded by file lanes, and the DAG names
  the edges that keep a lane single-writer.
- No task leaves a change in `include/` or `source/`, so no library test changes
  and the shape gates keep their base readings (FR-057 to FR-059). T030 plants
  its violation on a throwaway branch and deletes that branch.
- `specs/030-ci-speedup/results/` is the ledger and a repository path. A claim
  with no file in it is unverified, and every record passes the prose gate and
  `codespell` before its node reports done (T004).
