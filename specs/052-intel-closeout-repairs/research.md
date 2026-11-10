# Research: Intel Closeout Repairs

Phase 0 output of `/speckit.plan` for
`specs/052-intel-closeout-repairs/spec.md`. Each entry names the decision it
records, the reading that decides it, and the losing alternative. Every cite
reads the tree at `1627e756`, the tip of `origin/master` this branch rebased
onto on 2026-11-10, which carries PR #104, #105, #106 and #107. An earlier
reading here names the tree it read, and `1f1f9b20` is that tree
(version 0.34.0, report schema 1.16 (`source/harness/json_report.cpp:58`),
counters archive `SOVERSION 8`, harness `SOVERSION 22`, and 84 lines that
carry an exclusion marker within `source` and `include`).
`git diff --name-only c9187ba3..1f1f9b20` answered nothing under `include`,
`source`, `test`, `example` and `tools`, so that pair moved documents only and
the line numbers below read the code the closeout read at `c9187ba3`. PR #106
and #107 landed code on 2026-11-10 under `source/counters`, `source/harness` and
`test/source`, so a cite into `test/source/harness_jitter_cli_test.cpp` moves and
R-10 prints both readings. A bare `:NNN` continues the file above it. Six evidence notes read
the same host: `/tmp/icr/red_ignore.log`, `/tmp/icr/f1_ignore_builds.log`,
`/tmp/icr/gates-dbc.md`, `/tmp/icr/reg-inventory.md`,
`/tmp/icr/records-inventory.md` and `/tmp/icr/diag-sanitize.md`, whose step
logs stand under `/tmp/icr/san/`. A figure taken from one keeps its number and
gains that path. The lane host is CorsairOne, an Intel Core i9-9920X with 24
cpus, kernel `7.3.0-4-cachyos`, GCC 16.2.1 as `/usr/bin/c++`, clang 23.1.3,
lcov and genhtml 2.5.0-beta, `doxygen`, and `/usr/bin/python3` at 3.14.8 with
PyYAML 6.0.3.

Numbering: the entries here read `R-1` to `R-19`. Entry `R-n` records decision
`D-n` of the specification, cited as `spec D-5`. `R-17`, `R-18` and `R-19` carry
readings no decision names yet. The two spellings name different files.

## R-1: the number, and the four checks that keep it free

- **Decision**: The feature is 052 on `fix/intel-closeout-repairs`, cut from
  the tip of `origin/master` (spec D-1). 051 was the number at branch time.

- **Rationale**: `ls specs/` ends `047-loader-cross-field`, `048-hwlat-probe`,
  `049-names-old-records`, `050-limit-table-lifecycle`, `051-raw-events` and
  `052-intel-closeout-repairs`. Two numbers this lane ruled out at branch time got
  taken while the branch stood: PR #104 merged `049-names-old-records`
  2026-11-10T16:14:43Z, and PR #106 merged `051-raw-events`. The command
  `gh pr list --state open` answers one row now: PR #108, titled
  `Docs: Correct Zen 4 Acceptance Records`, on `fix/zen4-acceptance`. The command
  `gh pr list --state all --limit 200` counts 106 pull requests, so no free number
  below 052 stands unclaimed.

The branch name carries the work. The order names a number for the directory
alone, and names the branch by what the branch does.
`git rev-parse origin/master` read `1f1f9b20` at branch time.

- **Alternatives considered**: 049, reserved by
  `specs/050-limit-table-lifecycle` and claimed by the open pull request of
  the same number. 050, landed. The next free number at branch time, which
  spec D-1 states for a lane that finds 051 taken. PR #106 landed
  `specs/051-raw-events` on 2026-11-10, which is that case, and the directory
  moved to 052 the same day. The base moved with it to `1627e756`.

## R-2: the base code tree is the closeout tree, so a line number carries across

- **Decision**: The lane reads one code tree: `1f1f9b20` equals the closeout
  tree at `c9187ba3` for every file the fence holds (spec D-2).

- **Rationale**: `git diff --name-only c9187ba3..1f1f9b20` lists eight paths,
  all under `specs/`. Nothing moves under `include`, `source`, `test`,
  `example`, `tools`, `cmake` or `.github`. A ceiling, a registration line and
  a fixture path therefore read the same at the base and at the closeout,
  which is what lets a repairs row sit beside a closeout row without a second
  read of the code behind it.

- **Alternatives considered**: Reading every citation at both heads, which buys
  nothing a `git diff` answers once. Cutting the branch at `c9187ba3`, which
  would drop the closeout rows this feature repairs beside.

## R-3: a commit cannot carry its own hash, so a row names its head by handle

- **Decision**: Every figure row names the head it ran at through the branch,
  the commit that carries the block, and `git rev-parse HEAD` read at that
  commit. The pull request body prints the full hash (spec D-3).

- **Rationale**: A commit string cannot contain the hash of that commit, so a
  literal hash inside a row can only name a commit older than the row that
  prints it. Ruling 29 of `Rulings - this phase (29 to 42)` of
  `specs/048-hwlat-probe` carries a pair of heads for that reason, and its
  proof clause prints the diff between them.

This phase measures at the squashed tip, so the two heads of that ruling stand
equal: the pass runs at the tip, and the block that carries the figures gets
written before the squash. The clause then answers an empty list, and a reader
resolves every figure with one command.

- **Alternatives considered**: The two-head form of ruling 29, with a literal
  pass hash and a markdown-only delta between the pass head and the tip. It
  holds, and it costs one extra squash for a delta the order calls out as the
  thing to avoid. Quoting first- pass figures at the tip, which ruling 29 bars
  and the order bars by name.

## R-4: the fault check runs before the fix, at a tree that does not carry it

- **Decision**: The gate script reads red at `1f1f9b20` with the attribute
  absent, and green at the tip (spec D-4).

- **Rationale**: Both faults stand measured at the base: the consumer build
  stops with exit 2 at `source/harness/noise_tracer.cpp:499`, and the
  developer variant stops at `example/docs/hunt_ranks.cpp:147`
  (`/tmp/icr/red_ignore.log`, `/tmp/icr/f1_ignore_builds.log`). A tree that
  already carries the attribute cannot produce that red, so the source fix
  stands reverted in the working tree until the gate exists.

The order runs gate, red, fix, green, commit. The red also fixes the shape of
the gate: a scan that inspects nothing reads green at the base too, so the
script plants a probe in a scratch copy of a real unit and requires that probe
to fail the semantic-0 compile.

- **Alternatives considered**: Committing the attribute first and proving it
  afterwards, which leaves the regression untested against the fault it stands
  for. A unit test that calls the helper and discards its answer, which tests
  a build flag and not a warning.

## R-5: two names fault at the ignore semantic, and a keep-going build closes the set

- **Decision**: `activationAnswered` in `source/harness/noise_tracer.cpp` and
  `records` in `example/docs/hunt_ranks.cpp` are the whole set of names that
  stop an ignore build (spec D-4, spec D-5).

- **Rationale**: The first stop hides the second, because a build ends at its
  first error. A keep-going build over the developer consumer tree answers one
  remaining diagnostic at the base, and it names the example file.

The two trees differ by one flag. `build/consumer-release` configures with
`DEVELOPER_MODE=OFF`, holds no `example/` target, and stops at the library
name alone. `build/consumer-release-dev` configures with `DEVELOPER_MODE=ON`,
builds the examples, and stops at both names in turn.

The compile probe that carries the same shape costs seconds per file, so the
gate reads both semantics on both files with every compiler on `PATH`.

```
cd build/consumer-release-dev && cmake --build . -j 24 -- -k
example/docs/hunt_ranks.cpp:147:15: error: unused variable 'records' [-Werror=unused-variable]
```
- **Alternatives considered**: A wider name list guessed by grepping for helpers called only inside a
  postcondition, which finds candidates and proves none of them. Running both
  full trees at every step, which costs a configure and an example build to
  answer what one keep-going build answers.

## R-6: the fix shape is the attribute the tree already uses

- **Decision**: `[[maybe_unused]]` with a reason comment closes both names (spec
  D-6). - **Rationale**: The tree carries the shape at four sites already,
  each one a name the ignore semantic drops:
  `source/harness/compare.cpp:1156`, `source/harness/jitter_probe.cpp:1153`,
  `source/harness/pass_plan.cpp:90` and
  `source/harness/host_record_check.cpp:443`. The attribute moves no code and
  changes no release path: at `SG_CONTRACTS_SEMANTIC=2` the name keeps its
  call, and at `SG_CONTRACTS_SEMANTIC=0` it keeps its declaration and loses
  its use.

`SG_ENSURE` expands to `((void)0)` at `include/speedgun-ng/dbc.hpp:463` under
semantic 0, and the error promotion arrives from `cmake/variables.cmake:28`,
`CMAKE_COMPILE_WARNING_AS_ERROR` set to `ON` for a top-level build, so a
dropped use is an error on every lane.

- **Alternatives considered**: A `(void)` cast, which states nothing about the
  semantic that drops the call and leaves the reason out of the tree. Deleting
  the helper, which loses the postcondition the semantic-2 build checks.
  Moving the call out of the postcondition, which runs contract work in a
  build that asks for none.

## R-7: the fence widens by one file, and the widening is a recorded deviation

- **Decision**: `example/docs/hunt_ranks.cpp` joins the fence for one attribute
  at line 147 (spec D-7).

- **Rationale**: The order reads: "Prior closeout fence:
  `test/base_cli/run_matrix.py` + fixtures, `test/example_run.sh` + fixtures,
  `harness_hunt_cli_test.cpp`, `harness_jitter_cli_test.cpp`, and specs
  041/046/048 repair text." No `example/` path stands in that list, so the
  widening is a deviation from the order's own line, and the spec names it
  where the reconciliation pass reads it.

The attribute lands at `example/docs/hunt_ranks.cpp:147`. The drift gate reads
the region `sg-example:begin hunt-ranks` of that file, which spans lines 15 to
33, so the block the page copies stands unchanged and the drift entry answers
as it did at the base.

Reverting that one attribute returns the state to the recorded alternative: a
blocked row that names the file, and a developer-mode ignore build that stays
red at the tip.

- **Alternatives considered**: A blocked row in place of the attribute, which
  closes nothing US-1 asks and leaves red a build the lane can reach. Editing
  the example page beside the file, which moves a documented block and pulls
  the drift gate into a two-line fix.

## R-8: the gate is one shell script, one entry, and no new property

- **Decision**: One script over both files, registered by one `add_test` inside
  the Linux block, skipping by exit 0 (spec D-8).

- **Rationale**: `test/simulation_mark_shape.sh` and
  `test/counters_tsc_read_shape.sh` hold the shape: compile one translation
  unit per compiler per semantic, plant a probe, print one line per step. The
  registration copies those neighbors at `test/CMakeLists.txt:209-221`, inside
  the Linux region that spans `:148` to `:237`, so the script reads the
  repository root from `$1` and the binary directory from `$2`.

`test/tier_labels.sh` reads the registration files and fails a non-comment
line that names `sleep`, `while`, `retry`, `repeat`, `date` or
`clock_gettime`, so neither the new `add_test` lines nor the script name carry
any of those tokens.

Two trailing loops read the directory `TESTS` property: the label loop at
`test/CMakeLists.txt:2492-2499` gives every entry that names nothing the label
`quick`, and the ceiling loop at `:2508-2513` gives it 300 s, so the gate inherits both without a line that names a
ceiling. A skip needs no `MASTER_SKIP` row, because a shape script skips by
exiting 0 with a `skip:` line, and the `SKIP_RETURN_CODE` property stands
unused by it. A `SPEEDGUN_SLOW_TESTS` row stays out as well: that list holds
the entries a pre-push run drops. The gate's own cost is the seconds of four
compiles of one translation unit, and D-8 of the spec makes the entry print its
own wall time per compiler and per semantic at the tip, so no row rests on an
unmeasured cost.

The measured cost stands in `/tmp/icr/red_ignore.log`. One semantic-0 compile
of `noise_tracer.cpp` answers exit 1 in seconds at the base and exit 0 with
the attribute, on `g++` and on `clang++`, with `unused-function` and
`unused-variable` both under `-Werror` and no other diagnostic in either
answer.

```
add_test(NAME ignore_contracts_shape
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/ignore_contracts_shape.sh
                 ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
```
- **Alternatives considered**: A C++ unit test, which cannot reach a compile
  flag. A CI workflow step, which the fence bars and which a host without a
  second compiler cannot skip. A `TIMEOUT` written at the registration, which
  restates the 300 s the trailing loop already gives and pulls the gate into
  the exception list that specs/030 FR-032 keeps at
  `test/CMakeLists.txt:2515`.

The compile form the script drives, read at the base and at the tree that
carries the attribute:

```sh
c++ -O2 -std=c++23 -Wall -Wextra -Werror=unused-function \
    -Werror=unused-variable -DSG_BUILD_TYPE='"Release"' \
    -DSG_PROJECT_VERSION='"0.34.0"' -Iinclude -Isource \
    -Isource/harness -Ibuild/export -DSG_CONTRACTS_SEMANTIC=0 \
    -c source/harness/noise_tracer.cpp -o /tmp/probe.o
```

## R-9: every sanitizer red at the base is a ceiling cost

- **Decision**: All four red names lose to a ceiling, and no sanitizer report
  stands behind any of them (spec D-9).

- **Rationale**: `/tmp/icr/diag-sanitize.md` measures each name in three shapes
  with the CI option strings and the tree otherwise to itself.
  `base_cli_matrix` answers 117.86 s against its 120 s ceiling and
  `example_run` answers 298.45 s against 300 s, so both pass alone by 1.8% and
  0.5% of the ceiling and lose to any sibling load. The four-entry run at
  `-j 24` answers exit 8 in 447.01 s and times out those two names while the
  two names with margin pass, which is the pair of reds the base row on master
  prints.

The tsan lane carries the same three names at larger multiples, each read in
the direct shape with no ceiling: 165.76 s, 473.03 s and 548.74 s against
120 s, 300 s and 300 s. The trees of that run carry GCC, because this lane
configured them before it read the `CXX=clang++` pin that
`.github/workflows/ci.yml:153` and `:236` set for both sanitizer jobs. The
printed lines read `***Timeout`, and no AddressSanitizer,
UndefinedBehaviorSanitizer or ThreadSanitizer report stands in any of the
logs.

Two readings of one name stand in this record and a reader needs both. The base
ceiling-control row at
`specs/048-hwlat-probe/results/acceptance-final-intel.md:1667` reads 392.0 s
for `harness_jitter_cli_test` direct in the thread tree at `208bc520`, a tree
built with `CXX=clang++`. This lane's own direct reading of the same entry is
548.74 s in a thread tree built with GCC, and the load witness of the mode runs
that answer it reads 0.27 before and 1.89 after
(`/tmp/icr/jitter-modes-tsan.log`). Both readings exceed the 300 s ceiling, so
the blocked conclusion holds on either, and every tree this lane builds from
here carries `CXX=clang++` so a row answers a base row built the same way.

One reading stands outside the set. `harness_hunt_cli_test` had no executable
in `build/tsan/test` while that tree sat partly built, so it answered
`Not Run` alone in 0.16 s and, in the parallel shape, `Command: ""` beside
`Test Passed`. A finished build of that tree answers 295.97 s and exit 0 at a
load average of 3.78, which ruling 30 voids as a figure while it stands as
proof that the entry needs no fix. A row that reads a missing binary as green
is a tree gap, and the schedule of `plan.md` builds every tree to completion
before a figure counts.

- **Alternatives considered**: A race or a memory fault behind a red, which the
  logs answer: every printed line reads `***Timeout`. Raising a ceiling, which
  the order bars. Masking an entry to a fixed core set in its registration,
  which is a wall-time change outside the fence and one the pinned shapes
  already measure.

Figures from `/tmp/icr/diag-sanitize.md`, one row per name per lane:

| name | lane | ceiling | alone, ceiling on | direct, no ceiling | `-j 24` with the four |
|---|---|---|---|---|---|
| `base_cli_matrix` | `sanitize` | 120 s | 117.86 s exit 0 | 117.70 s exit 0 | 120.02 s `Timeout` |
| `base_cli_matrix` | `tsan` | 120 s | 120.26 s exit 8 | 165.76 s exit 0 |  |
| `example_run` | `sanitize` | 300 s | 298.45 s exit 0 | 296.98 s exit 0 | 300.01 s `Timeout` |
| `example_run` | `tsan` | 300 s | 300.18 s exit 8 | 473.03 s exit 0 |  |
| `harness_hunt_cli_test` | `sanitize` | 300 s | 230.18 s exit 0 | 230.35 s exit 0 | 245.62 s passed |
| `harness_jitter_cli_test` | `sanitize` | 300 s | 147.21 s exit 0 | 146.84 s exit 0 | 146.84 s passed |
| `harness_jitter_cli_test` | `tsan` | 300 s | 300.26 s exit 8 | 548.74 s exit 0 |  |

## R-10: the ceilings stay, and the drivers lose wall time

- **Decision**: Both drivers start their children one after another, and a
  bounded pool inside each driver is what closes the two shared reds (spec
  D-10).

- **Rationale**: `base_cli_matrix` runs 26 launches of `docs_command_line`
  through `test/base_cli/run_matrix.py`, and the per-row seconds sum to 117.30
  s of the 117.86 s wall. The bare instrumented start of one launch costs 1.52
  s under AddressSanitizer and 2.15 s under ThreadSanitizer, so 26 starts
  alone are about 39 s. `harness_hunt_cli_test` re-executes its own 84 MB
  binary once per scenario across 67 scenarios and adds about 137 refusal-row
  grandchildren.

A scratch copy of `run_matrix.py` at six rows at a time answers 25.80 s in the
address lane and 36.04 s in the thread lane, each with exit 0 and the same
verdict text as the serial driver. A scratch copy of `test/example_run.sh` at
six files at a time answers 56.39 s and 121.91 s with 37 `PASS:` lines. Those
figures carry the instrumented binaries of the same trees, so the two entries
reach their ceilings with 4.7x and 5.3x of margin.

`harness_hunt_cli_test` carries the same shape of cost and the same cure, and
the cure lives in a test source, so a build stands between the figure and the
change: 67 scenario children at six at a time answer 41.40 s through a scratch
driver over the same names. `harness_jitter_cli_test` answers 147.21 s on the
address lane and holds `RUN_SERIAL` because a probe window needs a quiet
machine, and on the thread lane it needs 548.74 s against 300 s. The two
levers that reach that ceiling are the ceiling itself and the per-run
instrumented cost inside `source/harness/jitter_probe.cpp` and
`source/harness/runner.cpp`.

The test source was read for a lever of its own, because the order's fence
carries it. `/tmp/icr/jitter-modes-tsan.log` records four mode runs in
`build/tsan/test` at head `2af15d23` with load 0.27 before and 1.89 after:
`family-planted-probe` 1.87 s, `unavailable` 1.86 s, `family-probe` 3.49 s and
`family-overlap-precheck` 41.87 s, all at exit 0. The file holds one work
constant above the floor, `--iterations=2000` at
`test/source/harness_jitter_cli_test.cpp:2305` of the `1f1f9b20` tree, read
again at `:2388` on `origin/master`, and that constant rides the
`family-planted-probe` mode alone, which costs 1.87 s: an empty body over 2000
iterations stays under the 2.15 s bare instrumented start, so cutting it buys
nothing. The other 36 batch shapes already read `--repetitions=1` with
`--iterations=2`, the file holds no sleep, no wait and no window count, and
its 305 `check` calls spread over 43 scenarios and 28 modes all stand at
`1f1f9b20`; the same file on `origin/master` answers 315 `check` calls after
PR #107, so the sweep and the line numbers run again at the tree that carries a
figure. The
four priced modes sum to 49.09 s, which leaves 499.65 s of the 548.74 s in the
remaining 24 modes and the 43 in-process scenarios, where each unit pays an
instrumented start that only `source/harness/` sets. That residual is the
reason no change inside the file reaches the ceiling.

- **Alternatives considered**: A pool inside `harness_jitter_cli_test`, which
  its own `RUN_SERIAL` ruling forbids, because a probe window with a sibling
  lands a jumped stamp in the wrong window. Dropping a row, a scenario or a
  check, which the order bars. A `sleep` or a wait to space the launches,
  which the order bars and the cost analysis does not need. A blocked row for
  `base_cli_matrix` and `example_run`, which the two proven figures answer.

Measured cuts, from `/tmp/icr/diag-sanitize.md`:

| entry | owned file | serial | at six at a time | state |
|---|---|---|---|---|
| `base_cli_matrix` | `test/base_cli/run_matrix.py` | 117.86 s / 165.76 s | 25.80 s / 36.04 s | proven on both lanes |
| `example_run` | `test/example_run.sh` | 298.45 s / 473.03 s | 56.39 s / 121.91 s | proven on both lanes |
| `harness_hunt_cli_test` | `test/source/harness_hunt_cli_test.cpp` | 230.35 s direct over 67 children in the `sanitize` tree of the table above, and 230.50 s in the repeat of that shape | 41.40 s over the same children, bounded at six | unproved, needs a build |
| `harness_jitter_cli_test` | none inside the fence | 147.21 s / 548.74 s | no width helps | blocked on the thread lane |
| `harness_jitter_cli_test` | the four modes named by `/tmp/icr/jitter-modes-tsan.log`, read at `2af15d23` | 548.74 s whole | 49.09 s for those four modes, 499.65 s left in the other 24 modes and the 43 scenarios | blocked |

## R-11: the coverage verdict reads one capture, and its denominator rides the row

- **Decision**: The coverage verdict reads the host capture of the tip through
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`, and the row
  prints the gate line beside the summary it read (spec D-11, FR-015).

- **Rationale**: `allowance=21` stands at `tools/dbc/coverage_gate.sh:108`. The
  ledger above it says what the constant counts: the union of the CI coverage
  job, where every entry runs to its end. That union read 16 rows at
  `bea6af5b`, 18 at `specs/032` and 19 at `specs/035`. A developer host reads
  higher, because an entry there can stop early and leave the arms past its stop
  dark. `tools/dbc/coverage_gate.sh:97-107` names which entries stop and which six
  rows their stops leave dark: `counters_linux_pmu_seam_test` fails the FR-019
  device-scope claim with `device 'cpu' holds 0 merged rows`, `harness_cli_test`
  skips the metric columns for want of a metric row that resolves to a published
  leaf, and their stops leave `source/counters/linux_pmu/fast_read.cpp:197`,
  `source/counters/linux_pmu/provider.cpp:567` and `:595`, and
  `source/harness/json_load.cpp:130` twice and `:1356`. No root right reaches those
  six: three of them stand in `source/harness/json_load.cpp`, a parser reading a
  row the entry skipped. What clears them is the union capture of the CI coverage
  job, where both entries run to their end, and the ledger says they stand covered
  there.

The base row on master reads the gate at 21 rows against the allowance of 21,
with lines at 100.0% over 18,683 of 18,683 and branches at 99.8% over 9,263 of
9,283 (`specs/048-hwlat-probe/results/acceptance-final-intel.md:1674-1688`).
The allowance therefore stands spent, and one dark branch row fails the gate.

The host capture that sits in `build/coverage/coverage.info`, written
2026-11-08 from a partial run, answers exit 1 today:
`branch shortfall 22 rows exceeds the ... allowance of 21`, over 18,664 of
18,664 lines and 9,273 branches. That denominator differs from the master row
by 19 lines and 10 branch positions, which is what the two readings of two
different captures look like side by side. No row here blends them.

- **Alternatives considered**: Reading the hosted union at the tip, which needs
  a runner this phase does not have. Reporting the host shortfall as a code
  defect, which the ledger's own sentence about the six rows answers. Letting
  a host shortfall stand as the coverage verdict while the line metric answers
  100.0%, which hides which of the two numbers the gate failed on.

The verdict shape at the tip, one row per reading:

| reading | command | what the row prints |
|---|---|---|
| the line metric | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | the gate line, then `lines` with its own denominator |
| the branch shortfall | the same run | the shortfall count, the allowance, and `branches` with its own denominator |
| the dark rows | `lcov --summary build/coverage/coverage.info` beside the row list the gate prints | each row that stands dark, and the entry whose stop left it dark |
| the picture step | `cmake --build build/coverage --target coverage` | its exit, and the `perl-GD` action the base row already names |

## R-12: the DBC target closes through its interpreter

- **Decision**: `cmake -B build -D Python3_EXECUTABLE=/usr/bin/python3` closes
  the target, and no shared gate file moves (spec D-12).

- **Rationale**: At the base the target answers exit 2 with
  `dbc gates require PyYAML (install python3-yaml): No module named 'yaml'`,
  because the cache of the tree named the interpreter under the operator home,
  `/home/archerc/.local/bin/python3.11`, and that interpreter carries no
  PyYAML.

Re-configuring the same tree with `-D Python3_EXECUTABLE=/usr/bin/python3`
answers exit 0 in 66 s. Both gates print `349 interfaces, 0 gaps`: the
document gate over `doc-matrix.json` and the pair gate over `pair-matrix.json`
(`/tmp/icr/gates-dbc.md`). `/usr/bin/python3` reads CPython 3.14.8 with PyYAML
6.0.3 on this host.

Two trees stand behind those two figures. The base reading ran in the release
tree `build`, configured `ci-rocky` with the interpreter flag added, which is the
pair of commands below. The row of the repairs block reads `build/dbc` in the shape
`.github/workflows/ci.yml:1284` configures for the DBC job,
`cmake -S . -B build/dbc --preset=ci-ubuntu -D Python3_EXECUTABLE=/usr/bin/python3`,
and T001 re-reads the figure there before any row quotes a count.

The interface count prints from its own run, so the row of the repairs block
carries the count the tip answers. The count moved from 342 at the head
`specs/042-topdown` recorded to 349 here, and that move belongs to the
interface set of the tree, to the record of the phase that added interfaces.

```
cmake -B build -D Python3_EXECUTABLE=/usr/bin/python3
cmake --build build --target dbc-gate
```
- **Alternatives considered**: Editing `cmake/dbc-gate.cmake` to pick an interpreter that carries PyYAML,
  which the fence bars and which moves a shared gate for one host's benefit.
  Installing PyYAML into the interpreter the cache picked, which is a machine-
  local act whose setup line prints a path under the operator home. A blocked
  row with the setup line, which the passing target answers.

## R-13: PR #104 corrected all six home-path lines, and this phase adds no correction row

- **Decision**: Rows this phase adds spell the home as `$HOME`, and this phase
  corrects none of the six lines it found at branch time, because PR #104 landed
  the scrub first (spec D-13, FR-022).

- **Rationale**: `grep -n '/home/'` over the three records answers five lines in
  `specs/048-hwlat-probe/results/acceptance-final-intel.md` and one in
  `specs/041-counter-probe/results/acceptance-final-intel.md`, and none in
  `specs/046-hunter-topdown/results/acceptance-final-intel.md`.

Three of the six already stand corrected: the correction ledger at
`specs/048-hwlat-probe/results/acceptance-final-intel.md:1036-1038` answers
the gate target interpreter row, the DBC target row of the gate table, and the
gate sentence of the rerun section, each with its reading kept and the host
fact restated as `$HOME/.local/bin/python3.11`.

The three that stood uncorrected at branch time, the two DBC gate output lines
at `:1546` and `:1547` and the cache line at
`specs/041-counter-probe/results/acceptance-final-intel.md:352`, stand corrected
on `origin/master`: the gate lines print
`$HOME/code/sg042-closeout/build/dev/dbc-gate/doc-matrix.json` and its pair, and
the cache line prints `_Python3_EXECUTABLE:INTERNAL=$HOME/.local/bin/python3.11`.
The reading that decides is `grep -n '/home/'` over the two records on
`origin/master`, which answers nothing.

- **Alternatives considered**: Rewriting the dated line in place, which loses
  the reading the closeout printed and breaks the rule that a dated section
  stands as written. Leaving the three lines, which fails FR-022 over a line
  the phase reads anyway.

## R-14: the root rows stay blocked, with today's reading beside them

- **Decision**: The root surface stays BLOCKED, the right it lacked gets named,
  and the reading gets refreshed (spec D-14, FR-023, and the standing blocked
  block at `specs/048-hwlat-probe/results/acceptance-final-intel.md:1075-1080`).

- **Rationale**: The base rows rest on four readings: `/sys/kernel/tracing` and
  `/dev/cpu/*/msr` stand root-owned, `sudo` answers that a password is
  required, `pkexec --disable-internal-agent id` answers 127 with
  `No authentication agent found.`, and the identity step exits 1 on its
  package query
  (`specs/048-hwlat-probe/results/acceptance-final-intel.md:1741-1748`).

The same commands read the same today on this host. `sudo -n true` answers 1
with `sudo: a password is required`. `pkexec --disable-internal-agent id`
answers 127 with
`Error executing command as another user: No authentication agent found.`
`/sys/kernel/tracing` reads `drwx------ 9 root root`, `/dev/cpu/0/msr` reads
`crw------- 1 root root 202, 0`, and `/sys/kernel/debug` reads
`drwx------ 65 root root`. The uid reads 1000.

`/proc/sys/kernel/perf_event_paranoid` reads -1, which opens perf events to an
unprivileged process and leaves the tracing file system, the `msr` character
devices and `hwlat_detector` root-only. The blocked root rows therefore name the
right they lack, and none of them counts the six dark coverage rows of R-11 among
the readings that right buys: that shortfall comes from two entries that stop
early on this host, and only the union capture of the CI coverage job runs them to
their end. A row that named `perf_event_paranoid` here would name a knob this host
already grants.

- **Alternatives considered**: Running the root surface through an elevation the
  host does not grant, which the order bars and the reading answers. Naming a
  host that grants the right, which G-24 bars. Marking the rows green because
  `perf_event_paranoid` reads -1, which confuses a perf counter with the
  tracing file system.

## R-15: one commit, one lease, one pull request

- **Decision**: One squashed commit carries the change, the first push runs
  `git push -u`, a later push runs the lease form, and one pull request opens
  and stays open (spec D-15).

- **Rationale**: The lease form names the value it expects, so a remote that
  moved during the work answers a refusal, and the commit the remote holds
  stays whole:
  `git push --force-with-lease=fix/intel-closeout-repairs:<expected-full-sha>`,
  then `git ls-remote origin refs/heads/fix/intel-closeout-repairs`, whose
  answer must equal the local head.

The `gh` on this host authenticates as the account that owns the repository
and carries the admin scope, so every merge path stands open to the lane and
the close comes from not calling it. The pull request opens with
`gh pr create`, and no merge, close or comment-then-merge step runs. Issue #76
stays open.

The commit carries the template of the constitution: a section token, a title
of 50 columns or fewer, a why-body wrapped at 72, `Refs:` lines, and
`Approved-by: OmO`.

- **Alternatives considered**: A merge under the admin scope, which the order
  bars twice. A force push without the lease, which can drop a commit the
  remote holds. Two pull requests, one for code and one for records, which
  splits the evidence the records cite.

## R-16: four pull requests merged, the base moved, and the branch rebased before any figure

- **Decision**: PR #104, #105, #106 and #107 merged on 2026-11-10 and moved
  `origin/master` to `1627e756`; the branch rebased onto that tip the same day,
  before any measurement, and rebases once more before its squash (spec D-16).

- **Rationale**: `gh pr view 104` reads state OPEN, base `master`, head branch
  `049-names-old-records`, created 2026-11-10T03:44:15Z, 82 files, and no
  merge date. Its file list names
  `specs/048-hwlat-probe/results/acceptance-final-intel.md`,
  `specs/041-counter-probe/results/acceptance-final-intel.md`,
  `specs/048-hwlat-probe/plan.md` and `.specify/feature.json`.

`git log --oneline 1f1f9b20..origin/master` answers four rows.
`git diff --name-only 1f1f9b20 origin/master -- include source test example` names
21 paths, `... -- source test include docs CMakeLists.txt cmake tools` names 23,
and the whole range names 140, so the base of every later figure is `1627e756`. No measurement stood taken when the merges landed, so no row needs
voiding. The line cites into the two records hold at the same numbers: the
ceiling controls still open at `:1651`, the lane rows at `:1637`, and the standing
blocked block at `:1073-1080`.

Two of the files it edits are files this feature appends to. R-13 names the
correction rows the block carries, and a scrub through #104 can move one of
those lines or answer it, so the corrections get read again after the rebase
and the appends get written after it.

- **Alternatives considered**: Waiting for #104 to merge before any record line
  stands written, which puts the code work, both sanitizer lanes and the
  coverage chain behind a document pull request. Rebasing now onto an open
  branch, which moves the base of a phase whose base row names a hash.

## R-17: the new entry moves every count, and a parallel run can score a missing binary as green

- **Decision**: Every count row prints the reading of its own run, because the
  new gate adds one entry to each Linux tree (extra reading, FR-013).

- **Rationale**: The dated lane rows read 185, 186, 185 and 190 entries:
  `sanitize` 185, `tsan` 186, `coverage` 185 and the developer release tree 190,
  all at head `208bc520`, read at
  `specs/048-hwlat-probe/results/acceptance-final-intel.md:1632`, `:1636` and
  `:1640`. The live base registers more:
  `git grep -c 'add_test(' -- test/CMakeLists.txt` answers 189 at `1f1f9b20` and
  191 at `1627e756`, the two names PR #106 added being `counters_raw_event_test`
  and `harness_raw_events_test`. Every count row therefore prints the reading of
  its own `ctest -N`, and one `add_test` inside the Linux region adds one entry on
  top of it.

A missing executable does not fail a parallel run on its own. In the thread
tree with a partly built test directory, `harness_hunt_cli_test` answered
`Not Run` in 0.16 s when run alone and, inside the `-j 24` shape,
`Command: ""` beside `Test Passed`. A pass therefore reads the entry count of
its own run and the per-entry lines behind a summary, and a row built on a
tree that did not finish building stands void and stays out of the record.

- **Alternatives considered**: Carrying the base counts forward, which fails
  FR-013 the moment the gate registers. Trusting a lane summary line alone,
  which the empty-command reading above answers.

## R-18: the witness rule, the tool pins, and what a full pass costs

- **Decision**: A wall figure counts when the host carried nothing else and the
  load average stood under 2.0, and the tools read the versions CI pins (extra
  reading, ruling 30, FR-026).

- **Rationale**: Ruling 30 of `Rulings - this phase (29 to 42)` of
  `specs/048-hwlat-probe` voids a wall time taken while another compiler,
  test, capture or trace ran, or while the load average stood at 2.0 or above.
  One figure in this research carries that mark: `harness_hunt_cli_test` in
  the thread tree answered 295.97 s and exit 0 at a load average of 3.78, so
  its row records the ceiling cost and the void mark together.

`.github/workflows/ci.yml:33-34` installs `codespell` and
`clang-format==18.1.8` and runs the lint script with
`-D FORMAT_COMMAND=clang-format`. The host answers
`clang-format version 18.1.8`, which matches the pin, so a format finding read
here reads the same there.

The suite costs 288.37 s in the release tree, 528.80 s in the address tree,
687.42 s in the thread tree and 3,907.66 s in the coverage tree at the base, so
the four suites alone read 5,412.25 s. FR-026 binds the pass to the pinned shape
each lane's base row carries, and the base carries two per lane across three
lanes. The six read 897.9 s and 891.0 s in the address lane, 1,536.0 s and
1,534.0 s in the thread lane
(`specs/048-hwlat-probe/results/acceptance-final-intel.md:1640-1644`), and
289.0 s and 302.0 s in the release lane at `:1647-1649`, where the same record
also prints 1,459.90 s and 738.27 s for the same two masks at another head
(`:298-299`). The larger pair prices the release lane, because a plan that
under-prices its own gate cannot schedule it.

| item | reading | hours |
|---|---|---|
| four suites | 5,412.25 s at the base, less once three drivers stop losing their ceiling | 1.50 |
| six pinned shapes | 897.9 + 891.0 + 1,536.0 + 1,534.0 + 1,459.90 + 738.27 = 7,057.07 s | 1.96 |
| the three shapes of FR-010 for four names on two lanes | about 4,600 s at the base readings, less once the bounded shapes clear | 1.28 |
| seven tree builds, warm caches | the release tree, which also carries the CI test shape, `build/dbc`, the analyzer tree, both sanitizer trees, coverage and both consumers | 1.50 |
| coverage chain, DBC target and the prose, spell, format and drift gates | the chain, its hand re-runs, 66 s for the DBC target, and the prose, spell, format and drift gates | 0.60 |
| total, one pass | ruling 30 stands every measurement one after another on an idle host | 6.8 |

One pass costs about 6.8 h and two full passes about 13.6 h of a quiet host. The
earlier figure of "near three hours" priced two pinned shapes and no analyzer
tree, and it stands replaced. The owner read this table on 2026-11-10 and bounded
the second pass (spec D-17): the six pinned shapes at 1.96 h, the coverage chain
with the DBC target and the named gates at 0.60 h, and the coverage tree build
that chain needs, about 0.5 h of the build line above, which reads about 3.1 h
for the second pass and about 9.9 h for the two together. No pinned shape stands
dropped, because FR-026 keeps all six in both passes.

- **Alternatives considered**: Running two lanes together to finish sooner,
  which voids both wall readings under ruling 30. Quoting the base suite times
  as tip figures, which the driver changes of R-10 move by design.

## Deviations

Two readings move a rule of the series, and both stand named where a reader
looks.

1. **One file inside the fence that the order does not name.**
   `example/docs/hunt_ranks.cpp` carries one attribute at line 147, and the
   `Owned edit scope` section of the order names no `example/` path. R-7
   carries the reason, the drift reading that keeps the page gate untouched,
   and the one-revert path back to the blocked row.

2. **One head where ruling 29 names two.** A commit cannot print its own hash, so
   that ruling carries
   a pass head beside a pull-request head. This phase measures at the squashed
   tip, so the two stand equal and the proof clause of that ruling answers an
   empty list. R-3 carries the reading, and R-3 carries the cost of the
   alternative too.
## R-19: the format gate reads red at the base, and twelve jobs stand behind it

- **Decision**: The lane records the two unformatted files as a base reading,
  pins them, fails only on a name beyond that pair, and asks the owner whether a
  format-only pull request lands first (spec Constraints, Principle VIII).

- **Rationale**: `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` names
  `source/harness/jitter_probe.cpp` at `:323` and `:1646`, and
  `test/source/harness_jitter_probe_test.cpp` at `:329` and `:352`. Both stand
  identical to `origin/master`: `git diff --name-only 1627e756 HEAD` names no file
  under `source` or `test`. `clang-format --version` answers 18.1.8, which is the
  pin `.github/workflows/ci.yml:34` installs for the `lint` job. `cmake/lint.cmake`
  globs the whole tree with `file(GLOB_RECURSE files ${PATTERNS})`, so the check
  reads the same two names whatever a later branch edits.

`gh pr checks 107` answers `lint fail` after 37 s on a pull request merged
2026-11-10T16:18:16Z, with 13 jobs `skipping`. Twelve of the thirteen real jobs of
that workflow carry `needs: [lint]`, and `docs` needs four of those, so coverage,
sanitize, tsan, static-analysis, test, test-rocky, shared-audit, both consumer
jobs, dbc-gate and prose-lint all stand off at `1627e756`. A blocked row of FR-019
suits the coverage union, the sanitizer lanes and the DBC gate in CI, and none of
them can answer at all while the base holds this pair, so the reading goes to the
owner ahead of the build wave, into the gate table of the plan and R-19 here.

- **Alternatives considered**: Formatting the two files inside this branch, which
  writes two paths the fence bars and mixes another lane's repair into a closeout
  commit. Reporting the format gate green because this branch adds no name, which
  states a delta and reads as a verdict. Dropping the format step from the lane
  gate, which loses the one check that would catch a hunk this lane writes
  unformatted.
