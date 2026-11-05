# Fix the gates PRs 71 to 73 left red

One commit against `master` at `561095df`; the head sha stands in the
pull-request header. Version stays
`0.30.1`. No schema and no SOVERSION change stands in the lane.

## What was broken

PRs #71, #72 and #73 left master red. The counters tests read host
facts:
CPUID words, the mapfile row, the vendor table. The jitter window entry
failed in every CI job. The tsan lane ended inside
harness_jitter_cli_test on a data race between the CLI fold and the
probe latch. The UBSan lane reported a signed overflow in a planted
series. The prose-gate fixture entry ran near its time bound, and the
coverage gate read 44 dark lines with a branch shortfall of 52 rows.

## What the commit does

Counters: scripted CPU identities through the cpuidWords seam, an errno
matrix over the kernel-call seam, and a scripted provider for the
mapfile row and the vendor table. The real-open test plants an attribute
that names no pmu, so the kernel refuses it on a host that gates the
call and on a host that opens it. The published-row clause of the
top-down tree moved from an unreachable arm into the SG_ENSURE
postcondition that states it.

Harness: the jitter window fixture paces its scripted clock at 1,000 ns
per unit, and the watched entry's body ends on the counting reader's
count of the probe's own reads, so the probe's samples stand inside the
entry's window by construction. The probe publishes a latch-time
snapshot and the CLI folds it, which removes the race. The sine arm
carries a signed delta that truncates toward zero, which removes the
overflow.

Prose gate: the fixture imports the gate as a module and runs every case
in one process, with the whole-tree scan keeping its own subprocess. It
tallies every gate invocation and asserts the invocation count of each
phase and the counters each tree-mode run reports. No wall clock stands
in the fixture.

## Counts at the head

- thrawn: kernel 7.3.0-5-cachyos, gcc 16.2.1, cmake 4.4.4, lcov 2.6-0,
  48 cpus (AMD Threadripper 9960X).
- CorsairOne: kernel 7.3.0-4-cachyos, gcc 16.2.1, cmake 4.4.2, lcov
  2.6-0, 24 cpus (Intel Core i9-9920X).
- dev suite: 178 of 178, one host skip (harness_topdown_run_test).
- two-cpu suite: 178 of 178 at a load of 3.36.
- ASan and UBSan: 173 of 173, no sanitizer line, no runtime error line.
- tsan: 174 of 174, zero WARNING: ThreadSanitizer lines, load 3.70
  before and 2.97 after.
- window entry: 100 standalone runs and 100 runs under a two-cpu mask
  each read 100 passes, at a load below 4.0.
- prose, format (clang-format 18.1.8), spell, example drift: clean.
- DBC doc-gate and pair-gate: 345 interfaces, 0 gaps each.
- markers: 84, down from 94. The kept list with one reason per marker
  stands in specs/045-merge-fixes/results/marker-ledger.md.
- coverage: lines 100.0% (17653 of 17653), functions 98.4% (1832 of
  1862), branches 99.4% (8699 of 8751).

## One open number

The coverage gate passes the line check and fails the branch check on a
developer host: 52 rows against the D-14 recorded allowance of 21. The
gate's own sizing note states that the allowance counts the union of the
CI coverage job, where every entry runs to its end, and that a developer
host reads higher because an entry there can stop early and leave the
arms past its stop dark. gcov 16.2.1 counts 8751 branch positions on
thrawn, where the allowance was calibrated on the CI compiler's count.
The CI coverage job carries the binding figure. No exclusion, no gate
change and no allowance change stands in this lane.

## Host-local measures, none in the repository

- thrawn: ~/.lcovrc with check_data_consistency = 0, and the coverage
  build with the compiler launcher variables empty, because ccache
  fronting gcov 16.2.1 leaves table_parse.cpp.gcda with a stamp that
  mismatches its notes.
- CorsairOne: PERL_DATETIME_PP and PERL5LIB for the perl DateTime
  module, ~/.lcovrc as above, and a PyYAML environment under ~/scratch.

## Records in the tree

- specs/045-merge-fixes/results/marker-ledger.md
- specs/045-merge-fixes/results/acceptance-thrawn.md
- specs/045-merge-fixes/results/acceptance-corsairone.md
- specs/045-merge-fixes/results/acceptance-corsairone-cov.md
- specs/045-merge-fixes/results/gates-f1007.md
- specs/045-merge-fixes/results/phase10-gates.md