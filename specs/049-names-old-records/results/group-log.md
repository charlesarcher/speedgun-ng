# Group rewrite log (T014)

Reads taken at the wave, every figure from the rewriter dry-runs,
the apply TOTAL lines, and the orchestrator green reads. The rewriter
is `/tmp/tnames_rewrite.py`; per-group file lists are the seven
`/tmp/tnames_group*.txt` lists partitioning the 71 recorded files.

| Group | Task | Files | Lines | Occ. | Applied | Lane grep after | Numstat |
|---|---|---|---|---|---|---|---|
| A (030) | T004 | 26 | 55 | 55 | 55 | 0 | 55/55 |
| B (039) | T005 | 7 | 32 | 32 | 32 | 0 | 32/32 |
| C (029) | T006 | 4 | 26 | 26 | 26 | 0 | 26/26 |
| D (043) | T007 | 2 | 16 | 16 | 16 | 0 | 16/16 |
| E (050, 048, 041, 045) | T008 | 11 | 23 | 23 | 23 | 0 | 23/23 |
| F (024, 007, 014, 037) | T009 | 15 | 33 | 38 | 38 | 0 | 33/33 |
| G (six lanes) | T010 | 6 | 8 | 8 | 8 | 0 | 8/8 |
| Total | | 71 | 193 | 198 | 198 | 0 | 193/193 |

Group F carries the two named sentence repairs in
`specs/014-identifier-naming-camelcase`: the doubled run word lost one
word in `spec.md`, and the sentence-initial run word reads `Run` in
`plan.md`; both files read clean under the prose gate and codespell.

## Phase 3 green reads (T011, T012)

- Changed-line pairing: 193 base lines pair one-to-one with 193
  rewrites across the seven group commits; no file gained or lost a
  line.
- Masked equality read: 39 distinct tokens and 122 occurrences after,
  token-for-token equal to `/tmp/tnames-id-masked-before.txt`, with 0
  per-line mismatches (FR-004, SC-003).
- Recorded read over the rewritten lines: 49 distinct and 136
  occurrences; the 10-occurrence loss against the 146 base figure
  sits entirely inside the D-2 spans the decision table replaces,
  exactly as research R7 separates the reads.
- Agreed scope grep over `specs docs`: 0 lines (SC-001).
- Tree-wide read: 30 lines, all out-of-scope matches in the FR-005
  files: 19 in `test/prose-gate-fixture/commits/cases.yaml`, 5 in its
  runner, 4 in `ci.yml`, 1 in `CMakeLists.txt`, 1 in `CODEOWNERS`
  (SC-002).

## Gate reads (T013)

- `codespell` at the root: exit 0. A stray untracked
  `harness_catalog_out.txt` at the root (dated 2026-11-09, a
  catalog-dump artifact, git-ignored by absence of any tracking) was
  removed before the read; it was never part of the tree.
- `python3 tools/docs/example_drift.py .`: exit 0, silent.
- `python3 tools/prose/prose_gate.py --base origin/master`: 0 findings
  over 92 sources and 1387 units.
- Marker read `git grep -c LCOV_EXCL -- source include`: 84, unchanged
  (FR-007).

The merge base has not moved (`origin/master` still `1f1f9b20`), so
the D-10 rebase and its addendum do not trigger; FR-011's base counts
stand at the recorded readings.
