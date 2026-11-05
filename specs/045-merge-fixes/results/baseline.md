# Baseline at b1dd7670 (2026-11-03)

## Marker count

`git grep -c LCOV_EXCL -- source include` reads 94, by file:
`clock_provider.cpp` 17, `plan.cpp` 13, `group_io.cpp` 15,
`system.cpp` 12, `provider.cpp` 9, `fast_read.cpp` 7, `fold.cpp` 5,
`dbc.hpp` 4, `table_parse.cpp` 4, `counters_provider.hpp` 3,
`detail/core.hpp` 2, `marker.cpp` 2, `encode.cpp` 1.

## Suite

`ctest --test-dir build -j 8` over the `ci-rocky` build: 177 of 178
pass; `harness_jitter_window_test` fails, which is the FR-006 defect
(PR #73 run 41576309608). `harness_topdown_run_test` skips at this
host. This machine stands in for thrawn; corsairone runs at the
acceptance phase.

## Coverage, sanitize, tsan

The base figures stand as the PR jobs recorded them and the spec
cites them: the PR #71 coverage job read 16051 of 16059 lines
(`group_io.cpp` 2, `provider.cpp` 6) and a branch shortfall of 25;
the PR #72 coverage state read 44 dark lines and 52 short branch
positions; the tsan job reported the `seriesFigures` race; the
sanitize job reported the negative-delta conversion at
`harness_fwq_ftq_test.cpp:237`. The fix head re-measures all three;
this lane runs no duplicate instrumented baseline build.

## The five standing tools

`cmake -P cmake/prose-lint.cmake` and `tools/prose/prose_gate.py
--base origin/master` read 0 findings at the base (the range carries
the spec-file commits only).
