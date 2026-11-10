# Base state: T-NAMES cleanup at `1f1f9b20`

Read date: 2026-11-10. All reads ran on this host at the branch base
`1f1f9b20` (the live `origin/master` at lane start).

## Host figures

- kernel: `7.3.3-1-cachyos`, Linux
- compiler: `g++ (GCC) 16.2.1 20260810`; clang 23.1.4
- CPU: vendor `AuthenticAMD`, family 26 (0x1A), model 68 (0x44),
  `AMD Ryzen 9 9950X3D 16-Core Processor`

## Tree figures at base

- version 0.34.0; schema 1.16; harness SOVERSION 22; archive
  SOVERSION 8 rechecked at the final tree and recorded in `results/acceptance.md`
- marker read `git grep -c LCOV_EXCL -- source include` sums 84
- agreed grep over `specs docs`: 193 lines, 71 files, 18 lanes
- agreed grep tree-wide: 223 lines; 30 kept lines: 19
  `test/prose-gate-fixture/commits/cases.yaml`, 5
  `test/prose-gate-fixture/run_prose_gate_fixtures.py`, 4
  `.github/workflows/ci.yml`, 1 `CMakeLists.txt`, 1 `CODEOWNERS`, 0
  `LICENSE`

## Work-group counts (research R6)

| Group | Lanes | Lines | Files |
|---|---|---|---|
| A | 030-ci-speedup | 55 | 26 |
| B | 039-context-groups | 32 | 7 |
| C | 029-hunter-core | 26 | 4 |
| D | 043-fwq-ftq | 16 | 2 |
| E | 050, 048, 041, 045 | 23 | 11 |
| F | 024, 007, 014, 037 | 33 | 15 |
| G | 040, 031, 036, 035, 034, 032 | 8 | 6 |
| Total | 18 lanes | 193 | 71 |

## Identifier census snapshot (T002)

The recorded token multiset over the 193 base line bodies, from one
alternation scan `\b[0-9a-f]{4,}\b|\b[0-9]{2,}\b` with each match
position counted once, stands at `/tmp/tnames-id-before.txt` (49
distinct tokens, 146 occurrences). The equality read masks, in the
listed order, markdown links to URLs, bare `https?://` spans, a
case-insensitive `run <digits>` word, `PR #<digits>`, and `#<digits>`
spans, on both sides, and measures 39 distinct tokens and 122
occurrences (`/tmp/tnames-id-masked-before.txt`); the masked spans
carry exactly the identifiers the decision table replaces, whose
preservation the rewriter probes and the per-line equivalence prove.
The line bodies themselves sit at `/tmp/tnames-lines-before.json`
(71 files, 193 lines). T011 re-reads both reads after the
rewrites.
