# Tasks: Names and User Paths Out of Old Records (T-NAMES)

**Branch**: `049-names-old-records` | **Spec**: [spec.md](./spec.md) |
**Plan**: [plan.md](./plan.md)

Labels: `[P]` parallel-safe (disjoint files, no git commands by the
worker); `[R]` read-only check; `[O]` orchestrator-only (git or PR
surface). Every task names its verification. TDD order: the red read
(T001, T002) precedes every rewrite task; the green reads (T011 to
T013) follow them.

## Phase 1: Baseline and tool

- [x] T001 [O][R] Write `results/base-state.md`: the agreed-grep counts
      per group A to G from research R6, the 30-line kept reading with
      its file groups, the marker read (84), the version line, and this
      host's kernel, compiler, and CPU figures. Verify: every figure
      equals the research R6 and R8 tables, and the group sums add to
      193 over 71 files.
- [x] T002 [R] Snapshot the base lines and their identifier multiset.
      Save the 193 base line bodies to `/tmp/tnames-lines-before.json`
      keyed by file, and the token multiset to
      `/tmp/tnames-id-before.txt` with one scan: the alternation
      `\b[0-9a-f]{4,}\b|\b[0-9]{2,}\b`, each match position counted
      once, over the bodies as written. Save the masked equality read
      too: the same scan after masking markdown links to URLs, bare
      `https?://` spans, a case-insensitive `run <digits>` word,
      `PR #<digits>`, and `#<digits>` spans, in
      `/tmp/tnames-id-masked-before.txt`. Verify: the recorded
      multiset holds 49 distinct tokens and 146 occurrences, the
      masked read holds 39 distinct and 122 occurrences, and the
      per-file line counts sum to the research R6 group figures.
- [x] T003 [R] Write the throwaway rewriter `/tmp/tnames_rewrite.py`
      implementing the plan's decision table: home prefix to `$HOME`,
      bare `/home/<user>` to `$HOME`, Actions links and bare Actions
      URLs to plain-text run ids, pull links and pull URLs to `#<n>`
      and `PR #<n>`, and recorded endpoints, slugs, and quoted
      expressions to the `<owner>` spelling. Verify: dry-run over each
      group lists the research R6 changed-line and occurrence figures
      while writing nothing, and a token-shape probe per table row
      produces the table's replacement.

## Phase 2: Rewrites (parallel, disjoint file sets)

Each group task: dry-run the rewriter over the group's file list and
check the reported occurrence count against the Occurrences column of
research R6; apply; then read the diff line by line and fix any line
where the mechanical replacement breaks a sentence (FR-003 grammar
clause). The worker edits files only; the orchestrator commits each
group after its green read.

- [x] T004 [P] Group A: the 26 files under `specs/030-ci-speedup/`.
      Verify: dry-run reports 55 occurrences; agreed grep over the lane
      prints 0 lines; the diff changes exactly the 55 base lines.
- [x] T005 [P] Group B: the 7 files under `specs/039-context-groups/`.
      Verify: dry-run reports 32 occurrences; 0 lines; exactly 32 lines
      changed.
- [x] T006 [P] Group C: the 4 files under `specs/029-hunter-core/`.
      Verify: dry-run reports 26 occurrences; 0 lines; exactly 26 lines
      changed.
- [x] T007 [P] Group D: the 2 files under `specs/043-fwq-ftq/`.
      Verify: dry-run reports 16 occurrences; 0 lines; exactly 16 lines
      changed.
- [x] T008 [P] Group E: the 11 files under `specs/050-*`,
      `specs/048-*`, `specs/041-*`, `specs/045-*`. Verify: dry-run
      reports 23 occurrences; 0 lines; exactly 23 lines changed.
- [x] T009 [P] Group F: the 15 files under `specs/024-*`, `specs/007-*`,
      `specs/014-*`, `specs/037-*`. Two named sentence repairs join the
      rewrite here: the doubled run word in `specs/014-identifier-naming-camelcase/spec.md`
      (`run run <id>`) loses one word, and the sentence-initial run
      word in `specs/014-identifier-naming-camelcase/plan.md` takes
      its capital letter. Verify: dry-run reports 38 occurrences; 0
      lines; exactly 33 lines changed plus the two repairs; after the
      rewrite the prose gate and codespell read those two files
      clean.
- [x] T010 [P] Group G: the 6 files under `specs/040-*`, `specs/031-*`,
      `specs/036-*`, `specs/035-*`, `specs/034-*`, `specs/032-*`.
      Verify: dry-run reports 8 occurrences; 0 lines; exactly 8 lines
      changed.

## Phase 3: Green reads

- [x] T011 [R] Identifier census after: rerun the exact T002 reads,
      recorded and masked, over the same line bodies as they now stand
      in the working tree. Verify: the masked multisets before and
      after are equal at 39 distinct tokens and 122 occurrences, line
      by line, and any difference names its line and is fixed before
      close (FR-004).
- [x] T012 [R] Scope and kept greps: agreed grep over `specs docs`
      prints 0 lines; tree-wide grep prints exactly the 30 kept lines
      in the FR-005 files (SC-001, SC-002).
- [x] T013 [R] Gate reads, each exit 0 with its figure recorded:
      `codespell` (spell), `python3 tools/docs/example_drift.py .`
      (drift), `python3 tools/prose/prose_gate.py --base origin/master`
      (prose over the range), and the marker read at 84 (FR-007,
      FR-009, SC-004). T018 re-runs each of these at the squashed
      head.

## Phase 4: Records, squash, PR

- [x] T014 [O] Write `results/group-log.md` (per group: base line
      count, occurrences, files, changed lines, green-read output).
      Verify: figures match the rewriter dry-runs and the phase-2
      commits.
- [x] T015 [O] Commit the pointer move of `.specify/feature.json` as a
      spec-file commit beside the lane artifacts (done: the pointer
      and its trailing-newline repair rode the pass-four artifact
      commit). Verify:
      `git show --name-only` lists pointer and spec files only, and
      the agreed grep over `specs/049-names-old-records` prints
      nothing at that commit (FR-012).
- [x] T016 [O] (No rebase needed: `origin/master` unchanged at the
      final push read; no arrivals, no addendum.) Rebase onto the
      current `origin/master`; when the merge
      base moved, re-derive the base counts into
      `results/base-state.md` as a dated addendum (FR-011, D-10), and
      rewrite any arrivals that match with the same tooling before
      anything else runs. Verify: the agreed grep over `specs docs`
      reads 0 lines after the rebase, the addendum is committed, and
      no history below the merge base was rewritten (FR-008).
- [x] T017 [R][O] At the pre-squash head, one `cmake --preset=ci-rocky`
      configure, `cmake --build build -j 10`, and
      `ctest --test-dir build --output-on-failure`; then write
      `results/acceptance.md` naming that head, the host figures,
      every FR-009 reading, the marker figure, and the suite pass
      count, with the note that the head the build ran at differs
      from the squashed tree only by lane-record files under the
      feature directory, and that the pull-request body carries the
      squashed sha (the precedent records name no sha of their own
      commit). Commit the
      record as an artifact commit (FR-009, SC-004, SC-005, Principle
      VIII). Verify: the suite answers every test passing at that
      head, and the record is committed before the squash.
- [x] T018 [O] Walk the FR-012 grep: the agreed grep over
      `specs/049-names-old-records` prints nothing at every commit in
      `git log --format=%H origin/master..HEAD`, and
      `python3 tools/prose/prose_gate.py --check commit --base
      origin/master` reads 0 findings across every lane commit at
      this pre-squash walk. Squash the lane to
      one commit with the Pull Request Quality template: title
      `Docs: Remove names and user paths from old records` (50 chars
      or fewer), why-body at 72 columns, footer paragraph `Refs:
      specs/049-names-old-records` then `Approved-by: OmO`, author and
      committer `OmO <omo@speedgun-ng.local>`. Re-run the T012 greps,
      the T013 fast reads, and the marker read at the squashed head,
      whose tree differs from the built tree only by lane-record
      files under the feature directory. First push
      `git push -u origin 049-names-old-records`; later pushes use
      `git push --force-with-lease=049-names-old-records:<expected-sha>`
      and `git ls-remote origin refs/heads/049-names-old-records` must
      equal the local head; stop on a lease failure. Open one pull
      request to `master` with the body from `results/acceptance.md`
      (squashed sha, host figures, gate readings) and the footer
      rule. Never merge; close no issue. Verify:
      `python3 tools/prose/prose_gate.py --check commit --base
      origin/master` reports 0 findings; `git log --oneline
      origin/master..HEAD | wc -l` reads 1; the fast reads answer 0
      at the squashed head; `gh pr list` shows exactly one open PR
      whose head sha equals the local head.

## FR to task map

| FR | Tasks |
|---|---|
| FR-001, SC-001 | T012 |
| FR-002 | T003, T004 to T010 |
| FR-003 | T003, T004 to T010 (grammar read) |
| FR-004 | T002, T011 |
| FR-005, SC-002 | T012 |
| FR-006, SC-005 | T012, T015, T016, T017, T018 |
| FR-007 | T001, T013 |
| FR-008 | T016 (rebase only, no history rewrite) |
| FR-009, SC-004 | T013, T017 |
| FR-010 | T018 (commit-lint reads over every lane commit and over the squashed commit) |
| FR-011 (with D-10) | T016, T017, T018 |
| FR-012 | T015, T017, T018 (the agreed grep over the lane directory at every lane commit) |
| lane records | T001, T014, T016 (the D-10 addendum), T017, T018 |
| SC-003 | T001, T016 |

## Parallel schedule

Wave 1: T001, T002, T003. Wave 2: T004 to T010 together (seven
workers, disjoint files). Wave 3: T011, T012, T013. Wave 4: T014,
T015, T016, T017, T018 in order.

## Phase 5: Convergence

- [x] T019 [R] (done, then superseded by T020: the note fix moved a
      further record, so a counted file list could not hold) Correct the tree-equality note in
      `results/acceptance.md` and in item 9 of `quickstart.md` per
      FR-009, SC-004, and plan step 8 (contradicts). Both records say
      the squashed commit reproduces the record commit's tree and that
      the head the build ran at differs from that tree by the record
      file alone. The trees answer two files. The built head named in
      the record, `438436585c0887d5da94dadb5bcec1a7cd2b241e`, and the
      squashed head differ by `results/acceptance.md` and by
      `specs/049-names-old-records/tasks.md`: the squash step ticked
      T016, T017, and T018 and added the note that no rebase was
      needed, so that file moved after the build ran. The record
      commit, the child of the built head that adds the record file and
      stands at `5660ce50` in `git reflog show 049-names-old-records`,
      and the squashed head differ by that same tasks file. Name both
      files and keep the reason the Principle VIII reading still
      stands: both files are lane records, so no build input, gate, or
      marker moved between the built head and the squashed tree.
      Verify: the corrected note in both files names exactly the two
      paths above; `git diff --name-only
      438436585c0887d5da94dadb5bcec1a7cd2b241e HEAD` lists exactly
      those two paths; `git diff --name-only 5660ce50 HEAD` lists the
      same two paths, the note edit itself included; the agreed grep over
      `specs docs` still prints 0 lines and the agreed grep over
      `specs/049-names-old-records` still prints 0 lines (FR-012);
      `codespell`, `python3 tools/docs/example_drift.py .`, and
      `python3 tools/prose/prose_gate.py --base origin/master` each
      exit 0; `git diff --name-only origin/master...HEAD` still lists
      the same 82 paths and `git grep -c LCOV_EXCL -- source include`
      still sums 84.

- [x] T020 [R] Settle the tree note once: every site (FR-009, plan
      step 8, T017, T018, `results/acceptance.md`, quickstart item 9)
      states the difference generically, lane-record files under the
      feature directory, because each correction of a counted list
      moves another record. Verify: `git diff --name-only
      438436585c0887d5da94dadb5bcec1a7cd2b241e HEAD` lists only files
      under `specs/049-names-old-records/`; no site names a file
      count; the agreed greps read 0; codespell, example_drift, prose
      range, prose tree over the lane, and commit-lint each exit 0;
      markers 84; version, schema, and SOVERSION unchanged.
