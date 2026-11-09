# Phase 15 closeout: the FWQ example widen and its measured gain

Record of the `fwq-example-n13e` lane of spec 045. Written 2026-11-09 on
thrawn. Section 1 and section 2 stand measured at the interim working
tree of `fix/fwq-example-n13e`, before the squash. Sections 3 to 9 stand
pending-final-head: each names the reading it will carry, and each
stands filled once the branch is squashed and the measured head stands
named.

## 1. Host, base and head

Host of record: thrawn, AMD Ryzen Threadripper 9960X (Zen 5, 24 cores,
48 threads), kernel `7.3.0-5-cachyos`. Scratch stands under
`$HOME/scratch/fwq-example-n13e/`.

| Read | Command | Value |
| --- | --- | --- |
| affinity | `python3 -c 'import os; print(len(os.sched_getaffinity(0)))'` | 48 cpus |
| online cpus | `cat /sys/devices/system/cpu/online` | `0-47` |
| GCC | `gcc --version` | `16.2.1 20260810` |
| clang | `clang --version` | 23.1.3 |
| clang-format | `clang-format --version` | 18.1.8 |
| lcov | `lcov --version` | 2.6-0 |
| cmake | `cmake --version` | 4.4.4 |
| ctest | `ctest --version` | 4.4.4 |

Base: `origin/master` at `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the
PR #100 merge. `git merge-base --is-ancestor a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a HEAD`
exits 0 at the branch head, so the branch carries the base.

Branch: `fix/fwq-example-n13e`, cut from the latest `origin/master`. It
carries the spec 036 trace-line lane beside this one, and no file stands
in both lanes. At the interim head the branch diff over the base names
the `spec.md`, `plan.md` and `tasks.md` of the two specs alone.

Head: the single squashed commit of the branch, carrying both lanes.
Every required lane of section 7 stands measured at that tree. The rule this record keeps: a tracked file carries no SHA of
its own commit, and each dated interim row names the pre-squash SHA it
stands taken at. The readings of section 2 stand taken at an interim
tree, and the squash re-runs the cheap gates of this lane at the
measured head.

## 2. The widen and its page pair (FR-034, SC-029)

At the branch base the `fwq-ftq` region of `example/docs/fwq_ftq.cpp`
carries its markers at `:33` and `:46` and its body at `:34-:44`. The
`README.md` block that quotes it carries its marker at `:765`, its fence
at `:766` and its body at `:767-:777`. The widen adds one line after the
`.mode` line of each, and no other line of either file moves.
`docs/pages/harness.md` and `example/docs/fwq_ftq.expected` stand
untouched, and `git diff --name-only` over the pair names `README.md` and
`example/docs/fwq_ftq.cpp` alone.

The two hunks, from
`git diff --unified=0 -- example/docs/fwq_ftq.cpp README.md`:

```diff
diff --git a/README.md b/README.md
index bc5e95d6..f4fe12c3 100644
--- a/README.md
+++ b/README.md
@@ -770,0 +771 @@ auto main(int argc, char** argv) -> int
+                      .gapCount = 4096,
diff --git a/example/docs/fwq_ftq.cpp b/example/docs/fwq_ftq.cpp
index 24f33119..4d26433d 100644
--- a/example/docs/fwq_ftq.cpp
+++ b/example/docs/fwq_ftq.cpp
@@ -37,0 +38 @@ auto main(int argc, char** argv) -> int
+                      .gapCount = 4096,
```

The diff answers two added lines and no removed line, and the two added
lines stand byte-identical to one another. The byte form of the added
line, from a Python `repr` of the line in each file:

```text
'                      .gapCount = 4096,'
```

The line carries 22 leading spaces, the `AlignAfterOpenBracket: Align`
reading of the file's `.clang-format`, and the file keeps
`ColumnLimit: 80`. The designator stands in declaration order: `mode` at
`include/speedgun-ng/benchmark.hpp:2558`, `gapCount` at `:2559`,
`precheckMs` at `:2560`. The figure 4096 stands inside `kMinGapCount` and
`kMaxGapCount` at `:2465-:2466`, so the require at
`source/harness/cli.cpp:413-415` holds.

The format reading at clang-format 18.1.8:

| Command | Reading |
| --- | --- |
| `clang-format --style=file example/docs/fwq_ftq.cpp` | the answer equals the file byte for byte, so `cmp` exits 0 |
| `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0 with no finding, taken at the interim tree that carried this lane's two edits alone |

The drift gate in both forms (FR-036):

| Command | Exit |
| --- | --- |
| `python3 tools/docs/example_drift.py .` | 0 |
| `python3 tools/docs/example_drift.py . --names --options --forbidden --sections` | 0 |

A later run of the same format gate over the shared working tree names
`test/source/harness_noise_test.cpp`, a file of the spec 036 lane, and
exits 1. That finding stands outside this lane's owned pair, and the
cheap gates stand re-run over the whole tree at the measured head in
section 7.

The page budgets after the widen, with the base reading beside each:

| Read | Base | After the widen | Cap |
| --- | --- | --- | --- |
| `README.md` lines | 1002 | 1003 | 1004 |
| section `Fixed work and time quanta` lines | 18 at `:763-:780` | 19 at `:763-:781` | 25 |

Both readings come from the gate's own `check_sections` and
`feature_sections` of `tools/docs/example_drift.py`. The base column
comes from the same two reads over the page at the branch base, a tree
the diff of `README.md` and `example/` against the base SHA answers
unchanged.

The pair stays a program the suite builds and runs. The lane's own tree
stands in `build-ex`, and `build` stands untouched by this lane:

| Command | Exit | Reading |
| --- | --- | --- |
| `cmake --preset=ci-rocky -B build-ex` | 0 | `build-ex` configured Release, and the cache reads `SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON` |
| `CMAKE_BUILD_PARALLEL_LEVEL=48 cmake --build build-ex` | 0 | `docs_fwq_ftq` links at `build-ex/example/docs_fwq_ftq` |
| `bash test/example_run.sh build-ex` | 0 | `PASS: docs_fwq_ftq (2 instances)` |
| `bash test/snippet-compile/run.sh . clang++` | 0 | `emitted 39 cpp snippet of 39 cpp fence`, `pass 37_fwq-ftq.snippet`, `39 snippet compile` |

## 3. The shape reading (US-11 scenario 3, SC-031, D-26)

At the squashed tree, the `report_shape_fwq_ftq_sample` entry walls
beside the phase 14 "This tree" column of the pre-widen example:

| Lane | Command | Pre-widen | Now |
| --- | --- | --- | --- |
| release | `ctest --test-dir build -R '^report_shape_fwq_ftq_sample$'` | 15.84 s | 1.29 s |
| two-cpu pinned | `taskset -c 0,1` over the same | 8.93 s | 1.33 s |
| four-cpu | `taskset -c 0,1,2,3` over the same | 13.14 s | 1.45 s |

The entry walls in the reading-only lanes carry no comparison: the
sanitizer tree answers 1.69 s, the thread-sanitizer tree 1.34 s, and
the instrumented tree 1.94 s.

The recorded block of `example/docs/fwq_ftq.expected` carries 156
content lines between its two marker lines, and the example at the
squashed tree answers 156 lines. The warning-line count stands 2 on
the recorded side and 2 on the run side. The drift gate answers 0
findings in both forms.

## 4. The timing protocol and the measured gain (FR-035, SC-030)

The baseline side stands fixed at the branch base: the tree
`$HOME/scratch/fwq-example-n13e/base/build-base`, configured `ci-rocky`
from `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a` and built once. Five
alone-entry runs of
`ctest --test-dir build-base -R '^report_shape_fwq_ftq_sample$'` on an
idle host answer 10.39, 11.23, 12.19, 11.74 and 11.29 s; the median
stands 11.29 s.

The head side stands five runs of the same command in `build`, the
`ci-rocky` tree of the squashed tree: 1.29, 1.29, 1.28, 1.34 and 1.26 s;
the median stands 1.29 s. The ratio of the medians answers 8.75x, with
the head side spread over 0.08 s and the base side over 1.80 s.

The earlier preliminary set (11.35, 12.42, 12.24, 11.95 and 11.84 s at
the pre-squash commit `9f1daf31`) stands a preliminary row only; that
commit stands unreachable from the branch.

The diff proofs ride the shared-branch shape of the plan: the diff from
the branch base to the squashed tree over the timed paths answers empty
apart from the four files of the spec 036 lane
(`source/harness/noise_tracer.cpp`, `source/harness/detail/noise_tracer.hpp`,
`test/source/harness_noise_test.cpp` and
`test/source/harness_noise_report_test.cpp`). The example names no
tracer engine, so the trace-line fold answers no call inside the timed
entry, and the entry's figures ride the same code both sides.

## 5. The earlier figures with their provenance

None of these stands compared with the measured ratio as an equal.

- The 17.6 estimate: instrumented tree at `2d4d8c57`, the standalone
  `build/coverage/example/docs_fwq_ftq` with the CLI key `gaps=4096`,
  six transforms, 50.31 s to 2.86 s (`cov-gaps4096.txt`).
- The 92 reading: release tree at `2d4d8c57`, `precheck=500, gaps=4096`,
  six transforms, 19.5 s to 212 ms.
- The ring sweep rows at `gaps=1024` through `gaps=32768` and the
  default, recorded in the phase 14 record.
- The branch-base alone-entry readings of 11.17 s, 11.37 s and 11.40 s
  at `results/barrier-fwq-closeout.md:174`, the ctest form at the base.

## 6. The recorded block's provenance (D-26)

2026-11-09: the ring bound of 4096 points stands widened in the example
and its README pair. `example/docs/fwq_ftq.expected` stands recorded
from the pre-widen example on the older recording host (the file names
version 0.30.1 and a 16-thread topology), and the block stands not
re-recorded: the drift gate and the example-run gate answer 0 findings
at the squashed tree with the recorded bytes kept.

## 7. The lanes at the measured head (FR-037, SC-032, SC-033)

All rows stand measured at the squashed tree on 2026-11-09.

| Lane | Command | Reading |
| --- | --- | --- |
| release suite | `ctest --test-dir build -j 48` | 190/190 pass, 71 s |
| two-cpu pinned | `taskset -c 0,1 ctest --test-dir build -j 2` | 190/190 pass, 211 s |
| four-cpu | `taskset -c 0,1,2,3 ctest --test-dir build -j 4` | 190/190 pass, 120 s |
| sanitizer | `ci-sanitize` tree, `ctest -j 24` | 185/185 pass, 127 s, zero sanitizer lines |
| thread-sanitizer | `ci-tsan` tree, `ctest -j 24` | 186/186 pass, zero ThreadSanitizer lines |
| instrumented | `ci-coverage` tree, `ctest -j 24` | 185/185 pass |
| coverage gate | `tools/dbc/coverage_gate.sh` | lines 100.0% (18779 of 18779), branch shortfall 21 inside the allowance of 21, markers 84 |
| cheap gates | format, spell, prose tree and range, drift, dbc fixtures, time-source | all 0 findings, exit 0 |
| analyzer | `cmake --preset=ci-ubuntu -B build-an` | `noise_tracer.cpp` 57 findings at the base against 43 at the head; `harness_noise_test.cpp` 223 against 220; no new check class stands on a changed line |
| exported names | `nm -D --defined-only` over the installed shared objects | harness 210, library 250; the sorted lists answer no difference against the base |
| consumer | `cmake -S test/consumer` against the installed prefix | configures, builds, links, runs, exit 0 |
| N13E-004 | the spec 036 record | the two sudo runs and the unrooted row stand filled there at the same tree |

The analyzer row carries one note: `bugprone-exception-escape` stands
once on the unchanged `main` line of `harness_noise_test.cpp`, a check
the base tree already answers thirteen times across its files. The
string-building the new test rows add reaches it through the call
graph; no changed line carries a new check class.

## 8. The push and the pull request (SC-033)

The branch stands pushed with `git push -u origin fix/fwq-example-n13e`
from the squashed tree; each later push rides
`--force-with-lease` against the saved expected SHA. The remote read
`git ls-remote origin refs/heads/fix/fwq-example-n13e` answers equal to
`git rev-parse HEAD` after the push. `gh auth status` on this machine
answers `You are not logged into any GitHub hosts`, so the pull-request
body stands delivered through
`$HOME/scratch/fwq-example-n13e/pr-body.md`, which names the measured
head beside every required lane. The branch stands unmerged for the
owner's `gh pr merge` with `--squash --admin`.

## 9. Rows left open (D-27)

- O-5 stands as phase 14 wrote it: the FTQ half of the example keeps
  its own bound and its own reading.
- O-6 stands as phase 14 wrote it: the phase 14 estimate column keeps
  its provenance labels and answers no re-derivation.
- O-7 closes with this phase: the widen pair of the example and the
  README stands landed, and the drift gate answers both forms green.
- O-8 stands deferred by the owner's ruling; this phase opens no work
  on it.
