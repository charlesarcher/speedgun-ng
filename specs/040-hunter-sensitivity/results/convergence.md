# The convergence record: hunter sensitivity (specs/040)

The gates the convergence wave ran, one section per gate, the
command, the verdict and the numbers. The tree stands at the release
build (`ci-rocky`) the whole wave ran against. Date: 2026-10-30,
host thrawn.

## The gates

- **The full suite**: `ctest --test-dir build` - 147 of 147 passed,
  twice (the second pass after the wave's fixes).
- **The docs gates**: `example_drift`, `example_drift_fixtures`,
  `snippet_compile`, `example_run`, `example_run_fixtures` - all
  passed. The wave's fixes: the guide page gained the two option
  rows' cells, the example's recorded output moved with the console's
  new cells, and the page block re-quoted the region the formatter
  reflowed.
- **The prose gate**: `cmake -P cmake/prose-lint.cmake` shape over
  the range `master..HEAD` and the commit gate - 0 findings after
  three commit-title rewrites (the gate's 50-character title rule).
- **The format gate**: `format-check` - the wave's 14 files stood
  badly formatted and stand clang-formatted now; the gate passes.
- **The spell gate**: `spell-check` - passed.
- **The time-source scan**: `test/time_source_gate.sh` - clean.
- **The DBC gate**: `cmake --build build --target dbc-gate` - failed
  with one finding, `sg::speedgunMain: enforced-not-documented`,
  pre-existing on master (the gate runs with the same finding on
  master's tree, verified by a stashed-tree run). The feature's
  interfaces stand documented and enforced; the finding is master's
  to carry.
- **The coverage, sanitizer, tsan and analyzer gates**: the heavy
  presets stand the CI run's on the pull request; this machine
  carries the release tree the feature's suite ran green on. The
  feature's files carry no new clang-tidy finding class beside the
  standing flood (the wave's workers checked each file's lines).

## The fixes the wave landed

- The guide page's option rows and the example's recorded output
  carry the console's new cells.
- The format gate's 14 files stand reformatted.
- The schema pins of the noise report test moved with the stamped
  minor (1.9).

## The contention the push measures

Specs/039's pushed branch (`origin/039-context-groups`, cut from the
same master `bea6af5b`) stands at version `0.26.0`, harness
`SOVERSION` `17` and schema `1.10`, and leaves the option values
`281` to `289` free beside this lane's `281` and `282`. The
reservation rule covers the second merge: the lane that merges first
keeps its values, the other takes the next free values at its
rebase, and the commit that moves the numbers records the check.
