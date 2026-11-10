# Acceptance record (T017)

Reads taken at the pre-squash lane head
`438436585c0887d5da94dadb5bcec1a7cd2b241e` (18 lane commits above
`origin/master` at `1f1f9b20`). This record is committed as an
artifact commit. The head the build ran at differs from the squashed
tree only by lane-record files under `specs/049-names-old-records/`:
this record, the task checkmarks of the lane's own steps, and the
note wording these convergence passes corrected. No build input,
gate, marker, or version moved between the built head and the
squashed tree, so the Principle VIII reading stands at the squashed
tree. The pull-request body carries the squashed sha;
this record names no sha of its own commit, as the precedent records
of specs/048 and specs/050 do.

## Host

- Kernel `7.3.3-1-cachyos`.
- `g++ (GCC) 16.2.1 20260810`; `clang version 23.1.4`.
- AMD Ryzen 9 9950X3D 16-Core Processor: cpu family 26 (0x1A),
  model 68 (0x44).

## FR-009 reads

- `codespell` at the repository root: exit 0. One stray untracked
  catalog-dump artifact (`harness_catalog_out.txt`, dated 2026-11-09,
  never tracked) was removed before the read; see the group log.
- `python3 tools/docs/example_drift.py .`: exit 0, silent.
- `python3 tools/prose/prose_gate.py --base origin/master`: 0 findings
  over 92 sources and 1387 units.
- `cmake --preset=ci-rocky` configure, `cmake --build build -j 10`:
  exit 0, release developer-mode tree in `build`.
- `ctest --test-dir build --output-on-failure`: `100% tests passed out
  of 190`, total 241.48 sec. Entry 188
  `harness_topdown_run_test` reports Skipped; running the binary
  directly answers one of its 29 scenarios skipped, a host
  availability skip the test itself reports, and no scenario failed.
- Marker read `git grep -c LCOV_EXCL -- source include`: 84, the base
  figure (FR-007).

## Census and scope reads

- 193 base lines pair one-to-one with 193 rewrites; no file gained or
  lost a line (FR-003).
- Masked equality read: 39 distinct tokens and 122 occurrences after,
  token-for-token equal to the base snapshot, zero per-line
  mismatches (FR-004, SC-003).
- Recorded read over the rewritten lines: 49 distinct and 136
  occurrences; the 10-occurrence delta sits inside the D-2 spans the
  decision table replaces (research R7).
- Agreed scope grep over `specs docs`: 0 lines (FR-001, SC-001).
- Tree-wide read: 30 out-of-scope matches, all in the FR-005 files
  (fixture 19+5, `ci.yml` 4, `CMakeLists.txt` 1, `CODEOWNERS` 1);
  `docs/` carries none (SC-002).
- Version `0.34.0`, report schema `1.16`, harness SOVERSION 22,
  counters SOVERSION 8: unchanged, as FR-011's numbers clause
  requires; no rebase addendum triggered because `origin/master` still
  answers `1f1f9b20cec97198e5cd88778167431eef2c6eb6`.
