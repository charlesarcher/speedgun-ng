# Tasks: Fix Rocky PMU Stack Smashing and the Harness Jitter Window Flake

**Input**: Design documents from `specs/044-fix-rocky-pmu-stack/`

**Prerequisites**: [plan.md](plan.md) (required), [spec.md](spec.md)
(required for user stories), [research.md](research.md),
[data-model.md](data-model.md), [contracts/move-contracts.md](contracts/move-contracts.md),
[quickstart.md](quickstart.md), [diagnosis-b2.md](diagnosis-b2.md)

**Tests**: the spec mandates tests; FR-007 and FR-012 bind each group's
tests to the commit that carries the change they test.

**Organization**: tasks are grouped by user story. Two commit groups bind
the work: the B-1 library change and its tests land in ONE commit
(FR-007), the B-2 fix and its test land in ONE commit (FR-012). No task
commits a partial group; only the two commit tasks and the setup
commit task run `git commit`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Every task names exact file paths; line numbers name the base
  `d454bcff`.

## Commit rules (FR-018, every commit task)

Title at 50 characters or fewer, imperative, the shape
`<Section>: <one-line>`. Body lines wrapped at 72 columns. Footer
block, in this order:

```text
Refs: specs/044-fix-rocky-pmu-stack
Approved-by: OmO
```

The footer order follows `prompt.md` section 3; the constitution's
template example order does not govern this feature.

---

## Phase 0: Setup

**Purpose**: pin the branch base and land the spec directory in its
own first commit.

- [ ] T001 [P] Verify the branch state: `git branch --show-current`
      names `fix/rocky-pmu-stack` and `git rev-parse --short HEAD`
      names `d454bcff`. When master has moved past `d454bcff`, start
      the branch at the new master and keep the new sha for T002's
      commit body (FR-013).
- [ ] T002 Commit the spec directory in its first commit: every file
      under `specs/044-fix-rocky-pmu-stack/` (`spec.md`, `plan.md`,
      `research.md`, `data-model.md`, `contracts/move-contracts.md`,
      `quickstart.md`, `diagnosis-b2.md`, `clarify-critic.md`,
      `clarify-evidence.md`, `clarify-round2.md`,
      `analyze-consistency.md`, `analyze-readiness.md`,
      `checklists/requirements.md`, `tasks.md`) plus the feature
      pointer `.specify/feature.json`, and nothing else.
      Title `Docs: Plan the Rocky PMU stack fix`. The body records the
      base sha from T001; when master has moved, the body records the
      new master's sha (FR-013). Footer per the commit rules above.

**Checkpoint**: the branch carries exactly one commit, the spec
directory, and no source file stands changed.

---

## Phase 1: User Story 1 - The PMU seam test stops aborting on the Rocky runner (Priority: P1) 🎯 MVP

**Goal**: `FastContext` becomes move-only with contracts, the four
fake-event-page sites map pages the test owns, the six
designated-initializer sites move to member assignment, and the two
count tests and the static assertion pin the lifetime rules. The
whole group lands in ONE commit (FR-007).

**Independent Test**: build at `ci-rocky`, run
`test/source/counters_linux_pmu_seam_test.cpp`'s binary 1000 times,
count 0 aborts (SC-001).

### Implementation

- [ ] T010 [P] [US1] `source/counters/detail/pmu.hpp`: delete the copy
      constructor and the copy assignment (`= delete`); declare
      `FastContext(FastContext&& other) noexcept;` and
      `auto operator=(FastContext&& other) noexcept -> FastContext&;`
      beside the destructor, each with the doxygen `\pre`/`\post`
      rows of `contracts/move-contracts.md`; add the explicitly
      defaulted default constructor `FastContext() = default;` (the
      aggregate note in `data-model.md`: the move constructor ends
      the aggregate status, and the member initializers must survive
      by name). Declarations only; no definition in the header
      (research.md, rejected placements).
- [ ] T011 [P] [US1] `source/counters/linux_pmu/fast_read.cpp`:
      define both move operations out of line beside
      `fastContextClose`, outside the `#if SG_PMU_FAST_X86` / `#else`
      branch. Each definition copies the five fields, empties the
      source's triple (`fd = -1`, `map = nullptr`,
      `mapLength = 0`), and the assignment calls
      `fastContextClose(*this)` before the take (FR-002). Enforce
      with the named captures of `contracts/move-contracts.md`: the
      assignment's `SG_REQUIRE(this != &other, ...)` and the two
      `SG_ENSURE` pairs. Depends on T010's declarations.
- [ ] T012 [P] [US1]
      `test/source/counters_linux_pmu_seam_test.cpp`: replace the six
      designated-initializer constructions (the multiplex site at
      `:720`, the raw page at `:779`, the two pinned-read sites at
      `:896` and `:910`, the lifetime scenario's context at `:965`,
      and the partial-open scenario's assigned temporary at `:1019`)
      with a default-constructed
      value and member assignment by name, the shape
      `fastContextOpen` uses (`fast_read.cpp:228-232`). The
      value-initialized array at `:1010` and the default
      constructions at `:1039` and `:1831` need no edit once T010's
      defaulted default constructor stands (data-model.md, the
      aggregate note). The partial-open clause carries FR-005 with
      T011's move assignment.
- [ ] T013 [US1]
      `test/source/counters_linux_pmu_seam_test.cpp`: give the four
      fake-event-page sites (`:720-724`, `:779-783`, `:896-900`,
      `:910-914`) an owned anonymous mapping: map one page with
      `mmap(nullptr, length, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)` at the length
      `::sysconf(_SC_PAGESIZE)` reports, build `makeEventPage`'s
      value in the mapping, put the mapping's address and the page
      size into the context, and let the destructor release it. The
      stack `page` and `rawPage` objects become the mapped pointers,
      because the scenarios read and mutate them at about twenty
      later sites; the multiplex scenario's field mutations go
      through the mapped pointer. No site clears the context's `map`
      before destruction (FR-004). Overlaps T012's lines, so it
      follows T012.
- [ ] T014 [US1]
      `test/source/counters_linux_pmu_seam_test.cpp`: add the two
      run-time count tests beside the lifecycle scenarios
      (`:3819-3821`), counted with `openDescriptorCount()` (`:172`)
      and `mappingCount()` (`:191`) (FR-006): a moved-from
      destruction test (baseline counts, a context holding a real
      descriptor and an owned anonymous mapping, a move construction,
      a field check on target and source, scope exit, the counts
      back at the baseline) and a move-assignment test (baseline
      counts, a target and a source each holding a real descriptor
      and an owned mapping, the assignment, a mid-block check at
      exactly one above the baseline, a field check, scope exit, the
      counts back at the baseline).
- [ ] T015 [US1]
      `test/source/counters_linux_pmu_seam_test.cpp`: add the
      compile-time check that `FastContext` is neither copy
      constructible nor copy assignable:
      `static_assert(!std::is_copy_constructible_v<
      sg::counters::detail::FastContext> &&
      !std::is_copy_assignable_v<sg::counters::detail::FastContext>);`
      with `#include <type_traits>` (FR-006).
- [ ] T016 [US1] Build and run once as the pre-commit checkpoint:
      `cmake --preset=ci-rocky`, then
      `cmake --build build --target counters_linux_pmu_seam_test -j "$(nproc)"`,
      then run `build/test/counters_linux_pmu_seam_test` and expect
      exit 0 (quickstart.md).
- [ ] T017 [US1] Commit T010 through T015 as ONE commit (FR-007):
      `source/counters/detail/pmu.hpp`,
      `source/counters/linux_pmu/fast_read.cpp`,
      `test/source/counters_linux_pmu_seam_test.cpp`. Title
      `Counters: Make FastContext move-only`. The why-body records
      the partial-open reading chosen, the move-assignment reading of
      research.md risk 9, and the moved-from member status of the
      spec's Assumptions. Footer per the commit rules above. No
      partial group stands committed before this task.

**Checkpoint**: one commit carries the B-1 group; the seam test runs
clean at `ci-rocky`.

---

## Phase 2: User Story 2 - The harness jitter window gate fix (Priority: P1)

**Goal**: the gate waits end on a count of the probe's own reads,
installed through the reader-install seam over the fake provider's
scripted window. The whole group lands in ONE commit (FR-012). The
diagnosis stands done in `diagnosis-b2.md` (FR-009): the failure
reproduced under full-cpu load, so the fix applies; no sleep, clock
read, timeout, or retry joins any wait (FR-012, FR-016).

**Independent Test**: build at `ci-rocky`, run
`test/source/harness_jitter_window_test.cpp`'s binary 100 times plain
and 100 times under `taskset -c 0,1`, count 0 failures (SC-002).

### Implementation

- [ ] T020 [P] [US2] Move `kPassReads` into the internal header:
      `source/harness/detail/jitter_probe.hpp` gains the
      `kPassReads` constant (the value `1024`,
      `source/harness/jitter_probe.cpp:50` at the base, spelled once
      for the probe and the test), and
      `source/harness/jitter_probe.cpp` drops its local definition
      and uses the header's. The header is internal (specs/027 R-01); the move
      touches no installed header (FR-014).
- [ ] T021 [P] [US2]
      `test/source/harness_jitter_window_test.cpp`: add
      `CountingReader`, a `WindowReader` decorator in the shape
      `GapSeriesReader` (`:535-545`) uses: it holds the window the
      fake provider's public `open()` returns over the
      `machine/tsc` leaf, counts one increment on one relaxed
      `std::atomic<std::uint64_t>` (add `#include <atomic>`) per
      `readPoints` call, and
      delegates to the underlying window. The scripted series, the
      tail jumps, and the disclosure behave exactly as today. The
      accessor for the count reports a failure when no reader stands
      installed; it dereferences no null pointer.
- [ ] T022 [US2]
      `test/source/harness_jitter_window_test.cpp`: install the
      counting reader through `installTestReader`
      (`source/harness/detail/jitter_probe.hpp:144`) at every
      gate-body run (`:741`, `:941`, `:1004`, `:1044`, `:1081`,
      `:1260`, `:1268` at the base), after the scenario's
      `setPoints` calls; keep the raw reader pointer and the armed
      cpu count in function-local state (the `seriesJump()` /
      `passGateCalls()` pattern, `:115-123` and `:177-185`); run
      `captureRun`; clear the install and the pointer afterwards,
      the specs/027 FR-028 gap scenario's pattern (`:863-865`).
- [ ] T023 [US2]
      `test/source/harness_jitter_window_test.cpp`: re-target the
      four waits onto the counting reader's count. The `bmGate` pass
      wait (`:146-157`), the `bmGate` jump wait (`:158-169`), the
      `bmPassGate` rebase wait (`:234-242`), and the `bmPassGate`
      ride wait (`:243-251`) end on the count at the targets the
      per-cpu pass structure documents: `kPassReads` times the armed
      cpu count for each pass's worth of rides, one read per armed
      cpu for the rebase wait. The `bmGate` pass wait's target stands
      met by construction at body entry, because the measurement pass
      completes behind the latch before the first body call; the wait
      stays as an ordering signal and never spins. Remove the
      20,000-action margin, the
      `passGateBase() * 2 + 64` margin, and `passGateBase()` itself,
      whose only use the change removes. Keep the spin bound at
      1,000,000,000 and the yield (specs/027 FR-039). Depends on
      T021 and T022.
- [ ] T024 [US2] Grep the wait seam after the change:
      `git grep -n readActions test/source/harness_jitter_window_test.cpp`
      names no gate body. The provider's `readActions` keeps its
      published meaning for the counters tests; the gate bodies stop
      reading it (research.md risk 2).
- [ ] T025 [US2] Build and run once as the pre-commit checkpoint:
      `cmake --build build --target harness_jitter_window_test -j "$(nproc)"`,
      then run `build/test/harness_jitter_window_test` and expect
      exit 0 (quickstart.md).
- [ ] T026 [US2] Commit T020 through T023 as ONE commit (FR-012):
      `source/harness/detail/jitter_probe.hpp`,
      `source/harness/jitter_probe.cpp`,
      `test/source/harness_jitter_window_test.cpp`. Title
      `Harness: End the gate waits on a count`. The why-body records
      the cause from `diagnosis-b2.md` and the targets' derivation
      from the documented 1,024 reads per cpu. Footer per the commit
      rules above. No partial group stands committed before this
      task.

**Checkpoint**: one commit carries the B-2 group; the gate suite runs
clean plain and pinned.

---

## Phase 3: User Story 3 - The evidence and the pull request (Priority: P2)

**Goal**: every gate the base already runs stands green, the local
counts stand recorded, and one pull request against master stands
open and unmerged with the evidence in its body.

### Build and validation (quickstart.md, SC-001 through SC-004)

- [ ] T030 [US3] Build the static-analysis configuration once for the
      feature: `cmake --preset=ci-ubuntu` then
      `cmake --build build` (clang-tidy and cppcheck over every
      translation unit; AGENTS.md, build/test/verify).
- [ ] T031 [US3] Run the `ci-rocky` 100-run loop of quickstart.md
      from `build/test`, reconfigured first with
      `cmake --preset=ci-rocky` when the tree last configured a
      different preset: each of
      `counters_linux_pmu_seam_test` and
      `harness_jitter_window_test` 50 times plain and 50 times under
      `taskset -c 0,1`, all 200 runs exit 0 (SC-002, FR-016).
- [ ] T032 [US3] Build `ci-sanitize` (`cmake --preset=ci-sanitize`,
      `cmake --build build/sanitize`) and run the same 100-run loop
      from `build/sanitize/test`, 50 plain and 50 under
      `taskset -c 0,1`, all 200 runs exit 0 (SC-002).
- [ ] T033 [US3] Run the 1000-repetition seam loop of quickstart.md
      from `build/test` and record the abort count, expected
      `seam-test aborts: 0 of 1000` (SC-001).
- [ ] T034 [US3] Run the Rocky 10 container loop of quickstart.md
      (`podman run --rm --cap-add SYS_NICE -v "$PWD:/src" -w /src
      rockylinux/rockylinux:10`, the `test-rocky` job's package
      list, `ctest` filtered to the two tests, plus the 1000-run
      loop inside the container) and record the container counts
      (SC-003, SC-005). A host with no container tool reports the
      loop as not run in the pull request body (FR-017).
- [ ] T035 [US3] Run the full suite at `ci-rocky`:
      `ctest --test-dir build --output-on-failure` (SC-004).
- [ ] T036 [US3] Run the full suite under two cpus:
      `taskset -c 0,1 ctest --test-dir build --output-on-failure`
      (SC-004, FR-016).
- [ ] T037 [US3] Run the coverage gate:
      `cmake --build build --target coverage`. The target drives
      `tools/dbc/coverage_gate.sh` over `build/coverage.info`
      (`cmake/coverage.cmake`); 100% line and branch at the existing
      allowance of 21, no new `LCOV_EXCL` marker, no
      `--omit-lines`, no `--ignore-errors`, no `--remove` (FR-015).
- [ ] T038 [US3] Run the lint gate:
      `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`
      (the prepush step order of `tools/ci/prepush.sh`).
- [ ] T039 [US3] Run the spell gate: `cmake -P cmake/spell.cmake`.
- [ ] T040 [US3] Run the prose gate:
      `python3 tools/prose/prose_gate.py --base origin/master`.
- [ ] T041 [US3] Run the contract-pair gate:
      `cmake --build build --target dbc-gate`. The target drives
      `tools/dbc/dbc_doc_gate.py` and
      `tools/dbc/dbc_pair_gate.py` over
      `include/speedgun-ng`, `source`, and `tools`
      (`cmake/dbc-gate.cmake`); the new move operations' doxygen
      rows and `SG_ENSURE`/`SG_REQUIRE` pairs keep it green (FR-001).
- [ ] T042 [US3] Run the documentation drift gate:
      `python3 tools/docs/example_drift.py .` (no user-visible
      behavior changed, so the root `README.md` needs no update;
      spec Constitution Checklist).
- [ ] T042b [US3] Run the consumer-release build the CI job runs:
      `cmake --preset=ci-linux-ignore -B build/consumer-release
      -D CMAKE_BUILD_TYPE=Release
      -D speedgun-ng_DEVELOPER_MODE=OFF`, then
      `cmake --build build/consumer-release -j "$(nproc)"`
      (`tools/ci/prepush.sh`, the consumer-release job's commands).

### Scope, push, and the pull request

- [ ] T043 [US3] Check the diff scope:
      `git diff --name-only origin/master` names only
      `source/counters/detail/pmu.hpp`,
      `source/counters/linux_pmu/fast_read.cpp`,
      `test/source/counters_linux_pmu_seam_test.cpp`,
      `test/source/harness_jitter_window_test.cpp`,
      `source/harness/detail/jitter_probe.hpp`,
      `source/harness/jitter_probe.cpp`,
      `specs/044-fix-rocky-pmu-stack/`, and
      `.specify/feature.json`. Nothing under
      `.github/workflows/`, `cmake/`, `tools/ci/`, `tools/dbc/`
      stands changed, no file of the open pull request #68
      (`reconcile-fix-2026-10-30`) stands changed (FR-013), no
      installed header under `include/` stands changed, and
      `git grep -n FastContext include/` returns nothing (FR-008,
      FR-014).
- [ ] T044 [US3] Push the branch the first time:
      `git push -u origin fix/rocky-pmu-stack` (spec acceptance
      scenario 3). The branch stands linear through rebase only
      (FR-018).
- [ ] T045 [US3] Open the pull request:
      `gh pr create --base master` with the title
      `Counters: Stop unmapping test stack`. The body names the host
      and the kernel, pastes the counts of T031 through T036 and the
      Rocky container counts (or the not-run report of T034), and
      states B-1 fixed and B-2 fixed with the cause and counts of
      `diagnosis-b2.md` (SC-005). The pull request stands open and
      unmerged for the owner; it is never merged.
- [ ] T046 [US3] Every later push runs
      `git push --force-with-lease=fix/rocky-pmu-stack:<last pushed sha> origin fix/rocky-pmu-stack`;
      a failed lease stops the push with a report; the executor
      stops and reports; no retry runs (`prompt.md` section 5, spec
      acceptance scenario 3). The check `git ls-remote origin
      fix/rocky-pmu-stack` names the sha `git rev-parse HEAD`
      names (`prompt.md` section 5).

**Checkpoint**: every gate green, the branch pushed, the pull request
open and unmerged with the evidence in its body.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 0)**: no dependencies; the spec-directory commit
  T002 stands first on the branch.
- **User Story 1 (Phase 1)**: depends on Phase 0. T010 through T015
  are edits with no commit; T016 validates; T017 closes the group in
  one commit.
- **User Story 2 (Phase 2)**: depends on Phase 0; it touches no file
  of Phase 1, so its edits T020 and T021 can run in parallel with
  Phase 1's edits, while its commit T026 stands second on the branch
  after T017.
- **User Story 3 (Phase 3)**: depends on T017 and T026; the
  validation tasks T030 through T042 run at the feature head, and
  the push and pull request tasks T043 through T046 run last.

### Within Each User Story

- Edits first, the build-and-run checkpoint second, the commit task
  last; no task commits a partial commit group.
- Same-file tasks stand sequential: T012 before T013 (overlapping
  lines), T021 before T022 before T023.

### Parallel Opportunities

- T010, T011, and T012 touch three different files and can run in
  parallel; T020 and T021 touch a third file pair and can run in
  parallel with all of Phase 1's edits.
- The validation tasks T030 through T042 touch no source file; the
  loop tasks T031 through T034 each drive one build tree, so they
  run one build tree at a time on one host.
