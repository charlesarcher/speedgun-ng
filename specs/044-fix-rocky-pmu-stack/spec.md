# Feature Specification: Fix Rocky PMU Stack Smashing and the Harness Jitter Window Flake

**Feature Branch**: `fix/rocky-pmu-stack`

**Created**: 2026-11-01

**Status**: Draft

**Input**: User description: "speedgun-ng: fix two intermittent test-rocky failures — (B-1) stack smashing aborts in `counters_linux_pmu_seam_test` caused by the test giving `FastContext` a stack address to release, plus a double-release copy-assignment defect in `FastContext`; (B-2) a rare `harness_jitter_window_test` gate failure of unproven cause, diagnosed before any change."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The PMU seam test stops aborting on the Rocky runner (Priority: P1)

A contributor pushes a change and the `test-rocky` CI job runs `counters_linux_pmu_seam_test`. Today, in roughly 3 runs of 100, the process aborts with `*** stack smashing detected ***` from `multiplexWindowScenario()`. The cause is not the scenario's logic: the test stores the address of a stack variable in a `FastContext`, and the context's destructor calls `munmap` on that stack address. Every contributor, and every later contributor, must be able to run this test any number of times and never see an abort.

**Why this priority**: an intermittent abort on the release CI gate hides real regressions behind noise and can fail an unrelated merge. It is fully understood, with a root cause and a fix defined.

**Independent Test**: build at the `ci-rocky` preset, run the seam test 1000 times on the development host, and count aborts. Before the fix, about 3% abort; after it, none. Repeatable without any CI job.

**Acceptance Scenarios**:

1. **Given** the test builds at `ci-rocky`, **When** the seam test runs 1000 times on the development host, **Then** every run exits 0 and no run reports stack smashing.
2. **Given** a `FastContext` that holds a descriptor and a mapping, **When** it is moved from and the moved-from value is destroyed, **Then** the destruction closes no descriptor and unmaps no mapping.
3. **Given** a `FastContext` target that already holds a descriptor and a mapping, **When** a second context is move-assigned into it, **Then** the target's old descriptor and mapping are released exactly once and the target holds exactly the source's resources.
4. **Given** the four sites that put a fake event page in a `FastContext` (the multiplex-window and raw-page sites in `multiplexWindowScenario`, and the two pinned-read sites in `migratedThreadScenario`, one per contract build), **When** each site runs, **Then** the context's mapping is one the test itself created with a real anonymous mapping, so the destructor releases what it owns.
5. **Given** the partial-open scenario fills an array of contexts, **When** the array is destroyed, **Then** every descriptor and every mapping the scenario acquired is released exactly once.

---

### User Story 2 - The harness jitter window test is either fixed or left alone with the diagnosis recorded (Priority: P1)

A contributor sees `harness_jitter_window_test` fail on `test-rocky` with the message that the gate entry loads the folds of the probe cpus whose windows kept gaps (specs/027 FR-027, E-08). The cause is not proven: on the current master it passes every one of 100 repeated local runs, and it failed once in about 15 recent CI runs. The suspicion is noise in the gate's wait, which counts reads of a shared fake time series from more than one thread; the cause stands unproven.

**Why this priority**: an unproven cause must not be papered over with sleeps, timeouts, or retries; the specification demands diagnosis first, and leaves the test untouched if the failure does not repeat.

**Independent Test**: run the test 500 times under each of four load setups and record the counts. If 2000 local runs produce no failure, the test stays as it is and B-2 stays open in the pull request body with the counts. If a failure repeats and a cause is isolated, the fix makes the gate wait end on an event the test controls; a margin no longer ends the wait.

**Acceptance Scenarios**:

1. **Given** the diagnosis protocol runs to its stop condition, **When** 2000 local runs over the four setups produce no failure, **Then** no test change is made, the counts and setups are reported in the pull request body, and B-2 is stated as open.
2. **Given** a failure reproduces and its cause is isolated, **When** the fix lands, **Then** the gate's waits end on a count of the probe thread's own reads, kept apart from the runner's reads through the fake provider's seam, with no sleep, clock read, timeout, or retry added.
3. **Given** either outcome, **When** the change is reviewed, **Then** the recorded evidence (counts per setup, and per-failure captures when they exist) is in the pull request body.

---

### User Story 3 - The pull request carries only what the task owns (Priority: P2)

The repository owner receives one pull request against master that fixes B-1 (and B-2 where its evidence supports a fix), carries the local acceptance counts, changes no CI workflow, gate, or public surface, and is left unmerged for the owner.

**Why this priority**: the work is a bug fix with a bounded blast radius; the value is a mergeable, evidence-backed PR; the work adds no feature.

**Independent Test**: review the branch diff and the PR body: no file under `.github/workflows/`, `cmake/`, `tools/ci/`, `tools/dbc/` changed; no public header changed; the counts are pasted; the PR stands open and unmerged.

**Acceptance Scenarios**:

1. **Given** the branch is complete, **When** the diff against its base is inspected, **Then** it touches only the library's internal PMU seam and the two test files, with no CI or gate change.
2. **Given** the acceptance protocol ran, **When** the PR body is read, **Then** it names the host, the kernel, and the local counts, and states B-1 fixed and B-2 fixed or open with its evidence.
3. **Given** the branch is ready, **When** it is pushed, **Then** the first push is `git push -u origin fix/rocky-pmu-stack`, each later push is `git push --force-with-lease=fix/rocky-pmu-stack:<last pushed sha> origin fix/rocky-pmu-stack`, a failed lease stops the push with a report, and the pull request to master carries the title `Counters: Stop unmapping test stack` and stands unmerged for the owner.

---

### Edge Cases

- What happens when a `FastContext` copy is eliminated entirely by the compiler? The class must not declare a copy operation for the compiler to use; a deleted copy constructor and copy assignment make any accidental copy a compile-time error, checked by a static assertion in the test.
- What happens when a moved-from context is destroyed twice (once as a source, once at scope exit)? The moved-from value owns nothing, so the second destruction is the same no-op the unopened context already exercises (specs/012 FR-015).
- What happens when the stack randomization puts a fake page exactly on a page boundary? After the fix this state no longer exists: the context's mapping is always one the test mapped, so the destructor's `munmap` always addresses a mapping it owns.
- What happens when the jitter window gate keeps no gap because a foreign thread consumed reads? That is B-2's suspected cause; it is changed only if the diagnosis protocol isolates it, and then only by separating the probe's own reads from the runner's at the fake provider's seam.

## Requirements *(mandatory)*

### Functional Requirements

**B-1: `FastContext` lifetime (library `source/counters/detail/` and `test/source/counters_linux_pmu_seam_test.cpp`)**

- **FR-001**: `FastContext` MUST be move-only: the copy constructor and the copy assignment are deleted, and the move constructor and the move assignment transfer the descriptor, the mapping, and its length, leaving the source owning nothing (descriptor `-1`, no mapping, length `0`). Each new move operation carries the `\pre` and `\post` rows of `contracts/move-contracts.md` (Principle II): the moved-from postcondition stands enforced with `SG_ENSURE`, the move assignment's different-values precondition with `SG_REQUIRE`, and the contract pair gate stays green.
- **FR-002**: The move assignment MUST release what the target holds before it takes the source's resources, so the target never holds two descriptors or two mappings.
- **FR-003**: Destroying a moved-from `FastContext` MUST release nothing.
- **FR-004**: No test site MUST give `FastContext` a mapping the test does not own: the four sites that put a fake event page in a `FastContext` (the multiplex-window and raw-page sites in `multiplexWindowScenario`, and the two pinned-read sites in `migratedThreadScenario`, one per contract build) MUST place the page in a real anonymous mapping the test creates, and MUST NOT clear the context's mapping before destruction as a work-around.
- **FR-005**: The partial-open scenario MUST construct each array element in place or move into it, so every descriptor and mapping is released exactly once.
- **FR-006**: The test suite MUST add: a run-time test that a moved-from context's destruction touches nothing, counted with the existing descriptor and mapping count helpers; a run-time test that a move assignment releases the target's old descriptor and mapping exactly once; and a compile-time check that `FastContext` is neither copy constructible nor copy assignable.
- **FR-007**: The library change and its tests MUST land in one commit.
- **FR-008**: The internal seam MUST stay internal: no public header may name `FastContext`, and the change MUST NOT alter the public ABI or the library versioning. (Verified: `git grep -n FastContext include/` returns nothing at the base.)

**B-2: harness jitter window diagnosis (test `test/source/harness_jitter_window_test.cpp`, or the library where the cause sits)**

- **FR-009**: B-2 MUST be diagnosed before any change: the test MUST be run 500 times under each of four setups: `taskset -c 0,1`; `taskset -c 0`; all cpus with a load on every cpu, where the load tool is `stress-ng --cpu $(nproc)` or one busy loop per cpu, with the chosen tool recorded beside the counts; and a local Rocky 10 container. The counts stand recorded. The diagnosis on the base found the failure: it reproduces under full-cpu load (264 of 500 runs), while `taskset -c 0` cannot run the test at all because the test's own affinity precondition asks for two cpus (specs/027 FR-040), so the setup yields no signal about the gate; the report stands in `diagnosis-b2.md`.
- **FR-010**: When a run fails, the diagnosis MUST record the gate that failed (`bmGate` or `bmPassGate`), the read-count values at each wait, the probe's measured threshold, and the kept-gap stamps against the window edges, using a temporary debug print that is never committed.
- **FR-011**: If 2000 local runs over the four setups produce no failure, the test MUST NOT be changed; the counts and setups are reported in the pull request body and B-2 is stated as open.
- **FR-012**: If a cause is isolated, the fix MUST go where the cause sits (test or library), MUST NOT add a sleep, a clock read, a timeout, or a retry, and MUST make the gate's wait end on an event the test controls: a count of the probe's own reads, kept apart from the runner's reads through the fake provider's seam. The seam is the reader-install seam (`installTestReader`) over the provider's own scripted window, so the count covers every probe cpu the run probes; the waits' targets derive from the per-cpu pass structure the probe documents (1,024 reads per cpu), and the checks the waits serve are the entry-level and run-level fold checks. The fix and its test land in one commit.

**Hard constraints (all requirements)**

- **FR-013**: The change MUST NOT touch `.github/workflows/`, `cmake/`, `tools/ci/`, or `tools/dbc/`, MUST NOT alter any gate, and MUST NOT modify any file the open pull request #68 (`reconcile-fix-2026-10-30`) changes. The branch starts at `d454bcff`; when master has moved, the branch starts at the new master and the first commit body records that sha.
- **FR-014**: The change MUST NOT alter the public ABI, the installed headers, or the library versioning.
- **FR-015**: Line and branch coverage MUST stay at 100% under the gate as it stands on the base, with the exclusion allowance at 21; no new `LCOV_EXCL` marker, `--omit-lines`, `--ignore-errors`, or `--remove` may be added, and no code may be written only to reach a branch.
- **FR-016**: Tests MUST NOT use a sleep, a clock read, or a made-up cpu; cpu values MUST come from the affinity the host reports; the tests MUST pass under `taskset -c 0,1`.
- **FR-017**: All acceptance MUST be local; a GitHub runner's result is neither proof of a fix nor of a bug. The Rocky 10 check MUST run in a local container with the packages the `test-rocky` job installs, or be reported as not run when the host has no container tool.
- **FR-018**: Every commit MUST carry its tests in the same commit as the change they test, follow the repository's commit template (title at 50 characters or fewer, a why-body wrapped at 72 columns, footer block `Refs: specs/044-fix-rocky-pmu-stack` then `Approved-by: OmO`), and the branch MUST stay linear through rebase only.

### Key Entities *(include if feature involves data)*

- **FastContext**: the per-thread fast-read context the PMU seam owns: a descriptor, a mapped page, its length, the owning thread, and the pinned processor. Its lifetime rules are the defect B-1 fixes.
- **Fake event page**: the test-built stand-in for the kernel's mapped page, which B-1 moves from stack storage into an owned anonymous mapping.
- **Read count of the fake time series**: the counter the jitter window gates wait on, shared between the probe cpus and the runner; the seam that separates the probe's own reads from the runner's is where a B-2 fix would live.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The seam test runs 1000 times at `ci-rocky` on the development host with 0 aborts (before the fix: about 3 of 100).
- **SC-002**: Each of the two tests runs 100 times per preset at `ci-rocky` and at `ci-sanitize` (50 plain runs and 50 runs under `taskset -c 0,1`), with 0 failures.
- **SC-003**: The B-2 diagnosis report exists with a count for each of the four setups, and B-2 is either fixed with the cause recorded or explicitly open with the counts.
- **SC-004**: Every hard gate the base branch already runs stays green, including the full test suite, the suite under two pinned cpus, the sanitizing build, the coverage gate at its existing allowance, the lint, spelling, prose, contract-pair, and documentation-drift gates.
- **SC-005**: The pull request body names the host and kernel, pastes the local counts, and states B-1 fixed and B-2 fixed or open with its evidence; the pull request is left unmerged for the owner.

## Assumptions

- The instruction for this session runs the Spec Kit workflow over the bug-fix brief in `prompt.md`; the brief's own note that the task is "not a Spec Kit task" is a statement about scope (no roadmap item), and the workflow governs the process here, while the brief's hard rules govern the content.
- The push-and-pull-request step of the brief (§5) is in scope: the branch is pushed and one pull request is opened, never merged, once acceptance is green.
- A moved-from `FastContext`'s `owner` and `pinnedCpu` members carry the same vacuous status as the rest of the moved-from value: no member needs a defined moved-from value beyond "owns nothing", because every use of a moved-from context is already excluded by contract.
- The `perf_event_open` grant on the development host does not affect the fix: the changed scenarios never open a real event, they own their mappings directly.
- Where the brief's acceptance loop and the coverage gate interact, the loop's counts are evidence for the pull request body; the loop changes no gate.

## Constitution Checklist

- Main-page impact check (IV.1): this change alters no user-visible behavior; it fixes an internal seam's lifetime rules and two tests, so the root `README.md` and its `example/` regions need no update.
