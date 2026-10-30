# The gate verdict of specs/039-context-groups

- Unit: specs/039-context-groups T050, one row per gate of Principle
  VIII with the reading the lane took, the task ledger marked, and this
  record linked from the quickstart beside the five acceptance records
  (FR-038, VIII, X.4).
- Branch: `039-context-groups` over the lane base `bea6af5b`. A count
  of this branch's commits goes stale at the commit that carries it, so
  the readings stand taken at the head the review fixes left, titled
  `test: Share the group shape, drop the tautology`, read on 2026-10-30
  on the lane host `gary`, 16 cores over two L3 groups and one memory
  node.
- Reads: each gate unit took its reading on the tree named beside it.
  Seven rows stand re-read on this record's tree the same day and match
  what they answered then: the suite row, the page-drift row, the prose
  row, the spell and format row, the time-source row, the DBC leg inside
  the coverage row, and the vendor-proof row. The prose read counts the
  sources this record adds, so its source count grows and its finding
  count stays at zero. Two passes followed this record and moved source, the
  convergence fix and the review fixes after it, so the suite row, the
  two sanitizer rows, the static-analysis row, the prose, spell, format
  and time-source rows, the doc-facing subset and the coverage claim all
  stand re-read at the head those fixes left. Each sanitizer row names
  the compiler behind every reading it carries, and the newest reading
  on each of the two stands taken on the `clang` compiler those two CI
  jobs drive. The independent read of the same head stands ahead of this
  record in the pull request's own CI matrix: the run at the head the
  review fixes left answered green on every leg except the entry wrinkle
  4 names, and the run on this head stands in flight as this line lands.
- Commits: a hash stands quoted only for the lane base. T051 rewrote one
  commit body, which moved every hash from
  `Meta: Clear the analysis round-3 findings` onward, so the lane's own
  commits stand named by title here.

## The gates of Principle VIII

| Gate of Principle VIII | The command | The tree | The reading |
| --- | --- | --- | --- |
| Builds succeed for developer and CI presets on Linux | `cmake --preset=ci-rocky`, `cmake --preset=ci-ubuntu`, each followed by `cmake --build build` over a clean tree | `build` | 271 translation units compiled on each preset, 0 warnings on the `ci-rocky` read; `ci-sanitize`, `ci-tsan` and `ci-coverage` each configured and built the suite, read in their own rows below; the host compiler is GCC 16.2.1 at `/usr/bin/c++`, and CI drives Clang on the `sanitize` and `tsan` jobs (`.github/workflows/ci.yml`), so both families stand read |
| All tests pass (`ctest`) | `ctest --test-dir build --output-on-failure` after each clean rebuild | `build` | `ci-rocky`: 152 of 152 in 299.42 s; `ci-ubuntu`: 147 of 147 in 139.33 s, and re-read at this record's tree at `-j 24`: 100 percent tests passed out of 147 in 22.80 s; re-read at the head the review fixes left: 152 of 152 on `ci-rocky` in 68.87 s at `-j 8`, and 147 of 147 on `ci-ubuntu` in 21.14 s at `-j 8`; the five-entry difference between the two presets' counts is the first wrinkle below, no failing entry stands in either tree |
| Sanitizer-clean: ASan and UBSan | `cmake --preset=ci-sanitize`, `cmake --build build`, `ctest --test-dir build/sanitize --output-on-failure` | `build/sanitize` | 147 of 147 in 183.78 s; 0 AddressSanitizer, UBSan, leak and SEGV lines in the run log; re-read at the head the review fixes left, on a rebuilt tree at `-j 6`: 147 of 147 in 33.29 s, 0 such lines; that tree names `clang` 23.1.3 for both arms in its cache, the compiler the `sanitize` job drives with `CXX: clang++` (`.github/workflows/ci.yml:153`); re-read once more at that head on a clean `clang` tree at `-j 6`: 271 translation units, 147 of 147 in 39.99 s, 0 such lines; a GCC sanitizer read stands unavailable on this host, since `ci-sanitize` configured to GCC 16.2.1 stops with `-Werror=maybe-uninitialized` raised inside libstdc++'s `<regex>` internals while compiling `source/harness/compare.cpp`, a file no lane commit touched, and that stop stands reproduced on a retry at `-j 4` |
| Thread-sanitizer-clean | `cmake --preset=ci-tsan`, `cmake --build build`, `ctest --test-dir build/tsan --output-on-failure` | `build/tsan` | 148 of 148 in 484.60 s; 0 ThreadSanitizer lines; the grouped run, barrier, threads and fixture tests stand named green inside that read; re-read at that head, on a rebuilt tree at `-j 6`: 148 of 148 in 115.50 s, 0 such lines; the `build/tsan` cache names `/usr/bin/cc` and `/usr/bin/c++`, the host GCC 16.2.1, so both readings above stand GCC readings; the `tsan` job drives `CXX: clang++` (`.github/workflows/ci.yml:236`), and a read on a clean `clang` tree at `-j 6` there answers 267 translation units, 147 of 148 in 104.34 s, 0 ThreadSanitizer lines, the one failing entry named in the third wrinkle below |
| Static-analysis-clean: clang-tidy and cppcheck | `cmake --preset=ci-ubuntu`, a clean rebuild, and the collection forms the CI `Fail on gate findings` step uses | `build` | 271 translation units, 16,482 distinct findings over 132 checks, 0 findings on the gate list; re-read at the head the review fixes left: 271 translation units, 16,491 findings over 132 checks, 0 on the gate; the one cppcheck line of error severity stands off that list and stands named below with the branch that carries it |
| Name-check-clean: `readability-identifier-naming` | the same rebuild, counted against `.clang-tidy:21`, which names `readability-identifier-naming` alone under `WarningsAsErrors` | `build` | 0 findings on the naming check; the zero stands proven live against a scratch file that does carry a naming finding, so the reading cannot come from a scan that inspects nothing |
| `format-check` and `spell-check` pass | `cmake --build build --target spell-check`, `cmake --build build --target format-check` | `build` | both exit 0; the spell read loads `.codespellrc`, and the lane's 17 side build directories stood removed first so the two reads see tracked sources only; re-read at the head the review fixes left, both exit 0 |
| Generated prose satisfies XI | `python3 tools/prose/prose_gate.py --check all --base bea6af5b`, the kernel `cmake -P cmake/prose-lint.cmake` drives | the tree, and the commit range `bea6af5b..HEAD` | 95 sources, 6,701 units, 0 findings, after T051 rewrote the one body a finding named, in `Meta: Clear the analysis round-3 findings`; re-read at the head the review fixes left: 110 sources, 7,133 units, 0 findings |
| Time-source-clean | `bash test/time_source_gate.sh .` | the tree | clean, and its planted-call self-check reports the call in both placements it plants, so a clean read cannot come from a scan that inspects nothing; re-read at the head the review fixes left |
| Page-drift-clean: the page equals the code it quotes | `ctest --test-dir build -R "example_drift|example_drift_fixtures|snippet_compile|example_run|example_run_fixtures|report_shape"` | `build` | 13 of 13 in 39.19 s at this record's tree, and 13 of 13 again at the head the review fixes left, the family the gate names whole: the drift read, its fixtures, the snippet compile, the run gate, its fixtures, and the seven report-shape goldens; the wave 5 and wave 6 checkpoints read the same family after the console segment, the option row, the page section and the cap move to 957 landed; the `lint` job drives the drift read directly in a tree that configures no examples |
| Coverage gates of VI pass: lines, branches and DBC | `cmake --preset=ci-coverage`, `cmake --build build`, the suite, then `cmake --build build/coverage --target coverage` | `build/coverage` | the suite reads 147 of 147 there; lines 100.0% on 14,158 of 14,158 lines; branches 7,343 of 7,360, 99.8%, a shortfall of 17 rows inside the D-14 recorded allowance of 19 rows, none of the 17 standing on a line this branch moved; no `LCOV_EXCL` marker stands in any lane file and no new ignored error class joined the route; `doc-gate` and `pair-gate` read 318 interfaces with 0 gaps, re-read at this record's tree; the second wrinkle below names the target's exit code |
| Critical-path performance metrics stay within baseline | none stands owed while the gate's own condition is unmet | n/a | the repository holds no per-platform baseline store and no CI job that compares one, so this gate names no reading to take; what this branch owed instead stands read at T022: no arrival, wait or dispatch sits inside a measured window, and no clock joins the attempt, with `harness_context_barrier_test` and `harness_group_run_test` green |
| Code is designed for debug and tracing | `cmake --build build --target dbc-gate`, beside the 18 refusal lines and the two disclosure channels the records quote | `build` | the release, sanitizer, tsan and coverage presets the lane read all inherit `dev-mode`, so the reads above ran with the standing diagnostics on; every new path answers with a line a reader sees, a refusal at status 2, the group segment on the console row, or the `group` key in the document; `doc-gate` and `pair-gate` read 318 interfaces with 0 gaps, so no new interface stands without its contract and no new path stands silent |
| Build and test results are recorded with a verdict per change | `ls specs/039-context-groups/results/`, read through [quickstart.md](../quickstart.md) | the tree | six records stand under `results/`: five acceptance records beside the driver `run_groups.sh`, and this verdict; each names its commands, its exit codes and its artifacts, and the quickstart links every one of them (FR-038, SC-010) |
| Weakening a gate configuration silently is forbidden | `git diff --name-only bea6af5b..HEAD`, read against `tools/`, `cmake/`, `.github/`, `.clang-tidy` and `.codespellrc` | the tree | one gate-side constant moved: `PAGE_LINE_CAP` from 940 to 957 at `tools/docs/example_drift.py:224-232`, each step carrying its measured reason beside it; no gate config, no check list, no spell skip and no ignored error class moved, so no gate's checks stand narrowed |

## The four wrinkles

1. The `ci-ubuntu` tree registers 147 entries where `ci-rocky` registers
   152. That preset leaves `SPEEDGUN_DBC_MATRIX` off, so the five
   `dbc_matrix_*` entries of specs/030 FR-019 stand unregistered there.
   The registration is specs/030 FR-019 and specs/030 D-5: a CMake value
   decides it, the lane sets that value on `test`, `test-rocky` and
   `tsan`, leaves it off `coverage` and `sanitize`, and the analyzer
   lane follows the release preset it builds with. A reader who counts
   entries across two trees should expect a five-entry difference and
   nothing more, and a reader who wants the matrix reads
   `cmake --preset=ci-rocky`, `cmake --preset=ci-ubuntu-test` or
   `cmake --preset=ci-tsan`.
2. `cmake --build build/coverage -t coverage` exits 2 after its gate
   verdict prints. The cause stands on this host, outside the lane:
   `genhtml` cannot load the perl `GD` module under `~/perl5`, which
   loads against an older perl and fails with
   `undefined symbol: Perl_hv_common_key_len`.
   `docs/pages/building.md:349` documents it, and specs/007 T181
   records it. The line and branch readings above
   stand the gate's own verdict, and the exit code belongs to the report
   step that follows the verdict.
3. `harness_context_barrier_test` stands flaky under a thread sanitizer
   on this host and stands green without one, and no report line stands
   behind the failure. Measured at the head the review fixes left: 9 of
   25 and 10 of 25 direct runs on the `clang` tree, 2 of 12 through
   `ctest` serial, 1 of 12 at `-j 6`, and 8 of 25 on the GCC tree,
   while 50 direct runs on the `ci-rocky` tree answered no failure. The
   two failing scenarios are `yieldWait` and `spinWait`, the two that
   read a waiting participant's `voluntary_ctxt_switches` and expect
   that counter to stand flat, so the entry fails on a counter reading
   with no sanitizer report behind it. Neither that test nor
   `source/harness/execution_context.cpp` carries a commit in this
   lane's range, and the test's last change predates the lane base, so
   the flake stands outside this lane's ownership and no lane change
   clears it, while the `tsan` job of the pull request's run on that
   head answered success. The mechanism stands unproven between two readings: the
   observer takes the counter after the released participant has
   parked, or the sanitizer's own thread bookkeeping moves it. The
   fourth finding below holds the sibling case, an entry this lane
   likewise does not own.

4. `harness_jitter_window_test` stands flaky on the runner and green
   here. The `test` job of the pull request's run at the head the review
   fixes left lost that entry while 151 of 152 entries passed, every
   other job then standing reported success, and that run was superseded
   when the branch moved. The run at the head before it
   answered 152 of 152 on the same leg. Read on this host, 20 direct
   runs on a quiet machine and 20 more while a loaded one carried two
   sanitizer suites answered no failure. specs/022 owns the entry, its
   assertions ride the FR-022 tolerance, and no commit in this lane's
   range touches it, so the reading stands recorded and the host owns
   the follow-up.

## The coverage claim in the lane's numbers

Lines stand at 100.0% over the tree and on every lane file, and all 268
gcov-recorded added lines answer a positive count (266 at the trace this
record closes on). The trace stands
re-read after the convergence fix `Harness: Refuse a layout path that
fails to read`, its counters purged and the CI suite route run again
(147 of 147), and the totals answer the same: the fix moved a branch
site from a caught exception to a checked value, so
`source/harness/group_layout.cpp:117` carries the two arcs the removed
`catch` gave up and answers them taken 5 and 50 times. Branches stand at
7,341 of 7,358, 99.8%, a shortfall of 17 rows inside the recorded D-14
allowance of 19 rows. The review fuse that bounded
`groupOfContext` moved the totals by two positions on each side: the
unbounded `while` contributed two branch positions that a bounded search
and a contract line do not reproduce, so 7,360 became 7,358 and 7,343
became 7,341, and the shortfall count stands unchanged with no row inside
a file this branch moved. The lane owned one of those rows, and
`test: Feed the partly numeric slot` cleared it. The third arc at
`source/harness/group_layout.cpp:91`, the second operand of the `slotOf`
guard, answered zero taken until the row 9 case fed a partly numeric
`slots` member; the trace now answers that arc taken once, which moved
the branch count from 7,342 to 7,343 and the shortfall from 18 rows to
17. None of the 17 rows stands on a line this branch moved: they sit at
`cli.cpp` with 12 rows, two of them carried by the pre-lane baseline,
`options.cpp` with 2, `json_load.cpp` with 1, and
`execution_context.cpp` with 2.

| The lane file | Branches taken |
| --- | --- |
| `source/harness/group_partition.cpp` | 12 of 12 |
| `source/harness/group_fit.cpp` | 14 of 14 |
| `source/harness/group_layout.cpp` | 112 of 112 |
| `source/harness/runner.cpp` | 764 of 764 |
| `source/harness/hunt_cli.cpp` | 222 of 222 |
| `source/harness/json_report.cpp` | 170 of 170 |
| `source/harness/report.cpp` | 251 of 251 |
| `include/speedgun-ng/benchmark.hpp` | 32 of 32 |
| `source/harness/detail/options.hpp` | 24 of 24 |

## The reads the lane owed outside Principle VIII

| The read | The command | The tree | The reading |
| --- | --- | --- | --- |
| The suite on two cpus (FR-036, SC-008) | `taskset -c 0,1 ctest --test-dir build-w8q -j 2`, and `ctest --preset prepush-quick` under the same mask | `build-w8q` | 152 of 152 in 289.90 s; the `quick` label reads 145 of 145 in 109.54 s, and again at `-j 2` in 62.30 s; the grouped tests, at eight slots widest, pass on two cpus |
| The vendor proofs the yaml link touches (FR-016) | `ctest --test-dir build -R "purity_scan|nm_proof"` | `build-w8q`, and `build` at this record's tree | 11 of 11 in 0.72 s: yaml purity 0 findings, the yaml nm proof legs A, B and C, the hwloc nm proof legs A to F, and simdjson purity 0 findings |
| The installed package (FR-016, specs/006) | `cmake --install` to a prefix under `/tmp`, the audits, and a consumer build against that prefix | the install prefix | zero traces of the six vendored names in the install tree or in the package config files; a downstream consumer configured against that prefix built, ran, and resolved none of the host's system yaml-cpp 0.9.0 |
| The contested bindings against the base (FR-032) | `git diff bea6af5b..HEAD` over the four bindings, read at T049 | the tree | version 0.25.0 to 0.26.0, harness `SOVERSION` 16 to 17, archive `SOVERSION` unchanged at 4 under hard rule 12, schema minor 1.8 to 1.10 with 1.9 left for the parallel lane, option value 299 with the block comment naming 298 for specs/037 and 300 to 319 free |

## The findings that stand outside the lane

1. The perl `GD` module under `~/perl5` loads against an older perl and
   answers `undefined symbol: Perl_hv_common_key_len`, which is why the
   coverage target exits 2 after its verdict prints. The host owns it,
   and `docs/pages/building.md:349` states it.
2. cppcheck answers one line of error severity,
   `source/harness/runner.cpp:2604:53: syntaxError`. It stands off the
   gate list, and `master` carries the same site, so the lane clears
   nothing here and owns nothing here.
3. `include/speedgun-ng/benchmark.hpp:1892-1896` holds the
   `reportsPerIteration` template with no test instantiation, so its
   function record reads 0 in the genhtml summary. Lines and branches,
   the two metrics the gate reads, stand at 100.0%, and the site sits
   far from any hunk this branch moved.
4. One coverage pass aborted `counters_linux_pmu_seam_test` with
   `*** stack smashing detected ***` at 0.10 s, which dropped the
   counters trace and dragged the summary to 99.3% lines. The same
   binary passes run directly, passes through `ctest` on another preset,
   and passes with a cleared environment, and the pass re-run read 147
   of 147 with lines back at 100.0%. The counters subsystem stands
   outside this lane, so the reading stands recorded and the host owns
   the follow-up.

## The task ledger

Every task of [tasks.md](../tasks.md) this lane landed stands checked
there, this one included, and no task stands checked that did not land.
The wave checkpoints in the lane ledger name the unit that took each
reading above, the reads that earned no commit of their own, and the
tasks the lane added while it ran: T052 for the file's route to the
runner, T053 for the guide's standing blocks, and the convergence phase
the `/speckit-converge` read appended, T054 for a layout path the reader
could not read and T055 for the two hunt pairings the main page left
out. Both convergence tasks stand landed and checked here (X.4).
