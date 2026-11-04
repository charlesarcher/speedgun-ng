# Record the loader convergence readings

Record commits over `master` at `561095df`, the tip of the merged lane
of PR #74. The loader code, its tests, the guide rule, the version move
and the 024 law amendment stand as merged.

Head: `a77e75e6`, the head the readings below ran at. This pull request
carries 9 record commits over `561095df`.

## The host

chilly, an AMD Ryzen 9 7950X3D with 32 cpus. Kernel `Linux
7.3.1-1-cachyos x86_64`. gcc 16.2.1 (20260810). clang-format 18.1.8
pinned by `FORMAT_COMMAND`. cmake 4.4.4. lcov 2.x at
`~/.local/share/sg-tools/lcov2`, passed in with `-D LCOV_EXECUTABLE` and
`-D GENHTML_EXECUTABLE`.

## The suites

- `ctest --test-dir build --output-on-failure`: 178 tests, 176 passed, 1
  failed, 1 skipped, exit 8.
- `taskset -c 0,1 ctest --test-dir build --output-on-failure`: the same
  178, the same one failure, the same skip, exit 8. A second pinned run
  on a host at rest, load average 1.27 at the end, reads the same.
- The failing entry is `harness_jitter_window_test` on
  `theSeriesRoundTripsThroughLoadReport`, and the base tree at
  `b1dd7670` answers the same row. The skip is
  `harness_topdown_run_test`, which needs a PMU.
- ASan and UBSan: 171 of 173. ThreadSanitizer: 171 of 174 with one race
  at `jitter_probe.cpp:708 in seriesFigures`. A tsan tree built at
  `b1dd7670` answers the same two failures and the same race.

## The coverage

- Base `b1dd7670`, host: lines 99.5% (17485 of 17566), functions 97.3%
  (1804 of 1854), branches 98.8% (8593 of 8701).
- Head, host: lines 99.5% (17519 of 17600), functions 97.3% (1804 of
  1854), branches 98.8% (8635 of 8743).
- Head, `source/harness/json_load.cpp`: lines 99.2% (2194 lines),
  functions 100.0% (178 of 178), and every arm of the three checks is
  reached.

The lane adds 34 instrumented lines and 42 branch records, every one
covered, and no new uncovered row: the uncovered counts stand unchanged
at 81 lines, 50 functions and 108 branches. The host branch shortfall
reads 108 rows at the base and at the head, above the allowance of 21
rows. The rule is 100 percent lines and a shortfall of 21 rows or fewer,
and this lane does not claim the gate passes. The host gate exits 3 at
the base and at the head on the standing `lcov` record at
`source/harness/runner.cpp:707`, and the CI coverage job writes no
artifact at either sha, so the union figure has no measurement point.
The owner ruling stands pending as issue #76, and the jitter row stands
as issue #77.

## The gates

Lint with the pinned formatter, codespell, the example drift check, the
prose range gate over `origin/master`, the prose tree gate over the
feature, the dbc doc gate and pair gate at 345 interfaces with no gap,
and the clang-tidy and cppcheck build all answer 0.

## The matrix

Run `41772312542` reads at the record head: the nine lint, audit and
analysis jobs pass, `docs` stands skipped, and the five suite jobs stop
on standing entries, with the same race in `tsan` and the same missing
coverage artifact. No loader unit and no compare unit stands among them:
`harness_json_load_test` and `harness_compare_cli_test` pass in every
job.

## What lands here

- The evidence moves off `/tmp`, into the machine-local `build/logs/`
  tree, and the citations in the spec, the data model and the quickstart
  name it.
- The thread sanitizer race and the coverage gate gain base readings at
  `b1dd7670`.
- Three suite runs on a host at rest read 177 of 178.
- Two convergence passes close: the artifacts drop the shape wording for
  the measured 34 lines and 42 records, the quickstart drops its exit-0
  claim, the Scope list names the capture pair and the Spec Kit pointer,
  and the guide's loader section names the two entries the method check
  leaves alone.

No source, test, build file, or public header changes. The files are the
feature's spec, plan, research, data model, quickstart, contracts, task
list, two records, and the loader section of the guide.

Refs: specs/047-loader-cross-field T-LCF
Approved-by: OmO
