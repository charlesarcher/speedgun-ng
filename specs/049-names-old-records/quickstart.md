# Quickstart: Verifying the T-NAMES Cleanup Lane

From the repository root at the lane's squashed head:

1. Name grep over scope:
   `git grep -nE 'arche[r]c|char[l]es|@gmai[l]' specs docs`
   Expected: no lines, exit 1 from grep for no match.
2. Name grep tree-wide:
   `git grep -nE 'arche[r]c|char[l]es|@gmai[l]' | wc -l`
   Expected: 30 out-of-scope matches, only in the FR-005 files.
3. Marker read: `git grep -c LCOV_EXCL -- source include`
   Expected: the same sum as at base, 84.
4. Spell read: `codespell` from the root (loads `.codespellrc` and
   its skip list). Expected: exit 0.
5. Drift read: `python3 tools/docs/example_drift.py .`
   Expected: exit 0, printing nothing on a clean tree.
6. Prose read: `python3 tools/prose/prose_gate.py --base origin/master`
   Expected: 0 findings.
7. Commit read: `python3 tools/prose/prose_gate.py --check commit
   --base origin/master` Expected: 0 findings over the single commit.
8. File set: `git diff --name-only origin/master...HEAD` Expected:
   files under `specs/` or `docs/` plus the single
   `.specify/feature.json` line.
9. Build and suite at the lane's final tree (FR-009, Principle VIII):
   `cmake --preset=ci-rocky`, `cmake --build build -j 10`,
   `ctest --test-dir build --output-on-failure`. Expected: every test
   passes; the record names the head the reading ran at, the built
   head differs from the squashed tree only by lane-record files
   under `specs/049-names-old-records/`, and the pull-request body
   names the squashed sha.
10. Identity: `git log -1 --format=%an\ <%ae>` Expected:
    `OmO <omo@speedgun-ng.local>`.

The lane records each figure in `results/acceptance.md` beside the head
sha it was read at.
