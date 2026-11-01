# Clarify Critic: specs/044-fix-rocky-pmu-stack

Critic review of `spec.md` against `prompt.md` and the constitution (Principles IX and XI). The review ran on 2026-11-01 at base `d454bcff`. Each question below carries the critic's own decision; nobody needs to answer it.

## Questions

### 1. The spec drops the base-commit rule and the PR #68 exclusion

**Context**: The brief's section 0 pins the base to `d454bcff`, orders a re-branch when master has moved, and orders the new sha into the first commit body. It also forbids any touch of the files PR #68 (`reconcile-fix-2026-10-30`) changes. The spec carries neither rule; user story 3 and FR-013 list only CI paths and public headers.

**Question**: Must the spec carry the base pin, the re-branch rule, and the PR #68 file exclusion?

**Best answer**: Yes. Add a hard constraint: the branch starts at `d454bcff`; when master has moved, the branch starts at the new master and the first commit body records that sha; no file PR #68 changes may be modified.

**Why**: Both rules bound the diff scope. A contributor who follows the spec alone can rebase onto a moved master and lose the recorded base, or can edit a file an open PR owns and break that PR.

### 2. The spec leaves the push and pull-request mechanics out

**Context**: The spec's assumptions declare brief section 5 in scope. That section names the PR title `Counters: Stop unmapping test stack`, the first push with `-u`, later pushes with `--force-with-lease=fix/rocky-pmu-stack:<last pushed sha>`, and a stop with a report when the lease fails. The spec carries none of it; user story 3 and SC-005 cover only the unmerged state.

**Question**: Which PR mechanics belong in the spec?

**Best answer**: Add them to user story 3's acceptance scenarios: the PR title is `Counters: Stop unmapping test stack`; the first push is `git push -u origin fix/rocky-pmu-stack`; each later push is `git push --force-with-lease=fix/rocky-pmu-stack:<last pushed sha> origin fix/rocky-pmu-stack`; when the lease fails, the work stops and reports.

**Why**: The lease protocol is a safety rule against overwriting another person's push. Its absence makes an unsafe plain force push spec-compliant.

### 3. The new move operations carry no contract requirement

**Context**: The brief orders the `\pre`/`\post` contracts with `SG_ENSURE` for the library change, and Principle II (Design By Contract) is non-negotiable; the `dbc_pair_gate` runs on the branch. FR-001 and FR-002 define the move semantics but mention no contract.

**Question**: Must the spec require contracts on the new move constructor and move assignment?

**Best answer**: Yes. Extend FR-001: each new move operation carries `\pre`/`\post` contracts checked with `SG_ENSURE`, a moved-from source owns nothing is the postcondition, and the contract pair gate stays green.

**Why**: Without the clause, an implementation can pass the spec and fail the repository's contract gate, or add operations the constitution rejects as unchecked.

### 4. FR-004 counts four scenarios where the test has four sites in two scenarios

**Context**: The brief names four code sites: two in `multiplexWindowScenario` (the multiplex-window and raw-page contexts) and two in `migratedThreadScenario` (the two pinned-read contexts, one per contract build). FR-004 and user story 1's acceptance scenario 4 call them "the four scenarios that build a fake event page".

**Question**: Should the spec name sites or scenarios?

**Best answer**: Sites. Reword both places to: "the four sites that put a fake event page in a `FastContext` (the multiplex-window and raw-page sites in `multiplexWindowScenario`, and the two pinned-read sites in `migratedThreadScenario`, one per contract build)".

**Why**: A task executor that searches for four scenario functions finds two, and may stop at an incomplete fix. Two of the four sites sit in one scenario, and the two pinned-read sites run in separate build arms.

### 5. SC-002's run count is ambiguous

**Context**: The brief's acceptance loop runs each test 100 times per preset: 50 plain runs and 50 runs under `taskset -c 0,1`. SC-002 reads "runs 100 times at `ci-rocky` and 100 times at `ci-sanitize` — plain and under two pinned cpus", which a reader can take as 100 plain runs plus 100 pinned runs per preset.

**Question**: What does the 100 count cover?

**Best answer**: 100 runs per test per preset in total: 50 plain and 50 under `taskset -c 0,1`, exactly as the brief's loop states. Reword SC-002 to "50 plain runs and 50 runs under `taskset -c 0,1` per test per preset, 0 failures".

**Why**: The count is the PR body's evidence. An ambiguous count makes the evidence unverifiable against the loop that produced it.

## Verdict

The spec needs these edits before it converges:

1. Add the base-commit rule and the PR #68 file exclusion (question 1).
2. Add the PR title, the push protocol, and the lease-failure stop (question 2).
3. Add the DBC contract clause for the new move operations (question 3).
4. Reword FR-004 and user story 1's acceptance scenario 4 from four scenarios to four named sites in two scenarios (question 4).
5. Reword SC-002 to a 50-plus-50 split (question 5).

Minor edits, no question needed:

- FR-009's third setup leaves the load tool open. The brief allows `stress-ng --cpu $(nproc)` or one busy loop per cpu. State both options, and require the chosen one in the recorded counts.
- FR-018 names the footer keys but leaves the `Refs:` value open. The value is `specs/044-fix-rocky-pmu-stack`.
- Principle XI defects sit in the spec's own prose: em-dashes at lines 15, 33, 86, 89, 111, and 126 (the line 9 em-dash sits inside the verbatim user-description quote, which XI.6 exempts); contrastive `, not` framing at lines 33, 37, 51, 53, 89, and 122; `so that` at line 79. The prose gate runs against the base, so rewrite these lines.

Note for the orchestrator: the working tree already carries the FR-010 debug print as an uncommitted change to `test/source/harness_jitter_window_test.cpp`; FR-010's "never committed" wording covers that state.
