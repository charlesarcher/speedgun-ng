# Phase 16 closeout: the probe's tick factor and the unit-label fix

Record of the `probe-unit-labels` lane of spec 045. Written 2026-11-10
on thrawn. Section 1 and section 2 stand measured: the branch's interim
commits carry the spec documents alone, so the built library in `build/`
compiles the base code, and every reading here stands at the branch
base, before the fix lands. Sections 3 to 8 stand open and fill as the
later rows land.

## 1. Host, base and head

Host of record: thrawn, AMD Ryzen Threadripper 9960X (Zen 5, 24 cores,
48 threads), kernel `7.3.0-5-cachyos`. Scratch stands under
`$HOME/scratch/probe-unit-labels/`.

| Read | Command | Value |
| --- | --- | --- |
| affinity | `python3 -c 'import os; print(len(os.sched_getaffinity(0)))'` | 48 cpus |
| online cpus | `cat /sys/devices/system/cpu/online` | `0-47` |
| GCC | `gcc --version` | `16.2.1 20260810` |
| gcov | `gcov --version` | `16.2.1 20260810` |
| clang | `clang --version` | `23.1.3` |
| clang-format | `clang-format --version` | `18.1.8` |
| lcov | `lcov --version` | `2.6-0` |
| cmake | `cmake --version` | `4.4.4` |
| ctest | `ctest --version` | `4.4.4` |

Base: `origin/master` at `1f1f9b20cec97198e5cd88778167431eef2c6eb6`, the
PR #101 merge. `git merge-base origin/master HEAD` answers the base, so
the branch stands cut from it.

Branch: `fix/probe-unit-labels`. The interim commits between the base
and the working tree name five files under `specs/045-merge-fixes/`
alone (`git diff --name-only 1f1f9b2..HEAD`), so no code line moves
before the measurement of section 2.

The rule this record keeps: a tracked file carries no SHA of its own
commit, and each dated interim row names the pre-squash SHA it stands
taken at.

## 2. The factor (FR-038, SC-034)

The estimate: the PR #100 and PR #101 discussions carry a reading of
about 4.19 ticks per true nanosecond for this host's `machine/tsc`
leaf. This section cites the 4.19 figure as the estimate the
measurement below verifies or corrects.

The measurement stands at `$HOME/scratch/probe-unit-labels/factor.cpp`.
The program links the built counters library
(`build/libspeedgun-ng.a`), mints one two-leaf plan over
`machine/tsc` and `machine/monotonic` in the shape `openSystemRecorder`
uses in `source/harness/jitter_probe.cpp`, and reads both leaves in
pairs over a bounded spin: the run stops at the first pair whose ns
delta passes 1,000,000, or at the cap of 100,000 pairs. The leaves are
the only time sources the program touches: no sleep and no clock call
stands anywhere in the run.

Compile, with the C++23 flag set of the `ci-rocky` preset, the tree's
own include directories, and the library archive the release build in
`build/` produced:

```sh
/usr/bin/c++ -std=c++23 \
  -D_GLIBCXX_ASSERTIONS=1 -fstack-protector-strong \
  -fcf-protection=full -fstack-clash-protection \
  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion \
  -Wcast-qual -Wformat=2 -Wundef -Werror=float-equal -Wshadow \
  -Wcast-align -Wunused -Wnull-dereference -Wdouble-promotion \
  -Wimplicit-fallthrough -Wextra-semi -Woverloaded-virtual \
  -Wnon-virtual-dtor -Wold-style-cast -Werror \
  -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -O3 -DNDEBUG \
  -I/home/archerc/code/speedgun-ng/include \
  -I/home/archerc/code/speedgun-ng/build/export \
  $HOME/scratch/probe-unit-labels/factor.cpp \
  /home/archerc/code/speedgun-ng/build/libspeedgun-ng.a \
  -o $HOME/scratch/probe-unit-labels/factor
```

Run, five times:

```sh
$HOME/scratch/probe-unit-labels/factor
```

Each run prints `deltaTicks`, `deltaNs`, and the ratio
`deltaTicks * 1000000000 / deltaNs` as the tick rate in Hz. The five
readings; the outputs also stand under `$HOME/scratch/probe-unit-labels/`
as `run-1.txt` through `run-5.txt`:

| Run | deltaTicks | deltaNs | rateHz |
| --- | --- | --- | --- |
| 1 | 4207518 | 1000004 | 4207501169 |
| 2 | 4201890 | 1000004 | 4201873192 |
| 3 | 4201428 | 1000013 | 4201373382 |
| 4 | 4202772 | 1000024 | 4202671135 |
| 5 | 4206468 | 1000014 | 4206409110 |

The five runs read 4.2014 to 4.2075 ticks per ns, mean 4.2040. The
verdict: the 4.19 estimate stands verified. The measured mean stands
about 0.3 percent above the estimate, inside the spread the five runs
carry, so the record carries 4.2040 GHz as the measured tick rate of
`machine/tsc` on this host. A figure named in ns under the pre-fix code
carried about 4.2 times its name, and the landed fix's disclosure
prints the rate the run measures at setup; section 3 cross-checks that
printed rate against this figure.

## 3. The before and after rows (US-13, SC-035)

The rows stand taken from the `docs_fwq_ftq` entry's own run, whose
series figures carry the calibration the fix scales, the jitter
example's rows standing zeros in both trees. The base side runs the
branch-base binary built in the scratch worktree
(`$HOME/scratch/probe-unit-labels/base-wt`, configured `ci-rocky` at
`1f1f9b20`); the head side runs the interim head's binary in `build`.
One run per side, exit 0 each, the outputs kept under
`$HOME/scratch/probe-unit-labels/rows/`.

| Row | Base (ticks) | Head (ns) |
| --- | --- | --- |
| `unitTimeNs` (cpu 0) | 966 | 1386 |
| `unitTimeNs` (cpu 24) | 1134 | 1008 |
| `maxValue` (cpu 0) | 1008 | 5922 |
| `seriesRuntimeNs` (cpu 0) | 2970198 | 213822 |

The reading: the base figures carry raw counter ticks, so the base
unit of 966 ticks stands at about 230 true ns, the 1 us target the
probe contract names carried in the wrong unit; the head figures carry
nanoseconds, the unit standing at about 1,008 true ns, the target
honored. The per-run spread stands the run's own noise; the record
quotes one run per side and names no threshold over the figures.

## 4. The tests (FR-040, SC-036)

The fixture: `test/source/harness_jitter_probe_test.cpp` declares the
`machine/monotonic` leaf beside the `machine/tsc` leaf, the way the
catalog's clock provider does, and the run helper scripts the rate pair
before every probe start: the pair stands on tail-only scripts, so the
spin's reads answer their deltas without consuming a point sequence,
and the scenario point sequences carry a two-read preamble the spin
consumes, the item's window opening at the sequence's first point. The
identity pair stands at one million ticks per one million ns, the rate
one tick per ns, the rate at which the projection is the identity and
every pre-existing scripted row keeps its bytes.

The new rows, with the readings:

- The unit time row: the pass steps 4,190 ticks, the projected period
  the state carries stands at 1,000 ns.
- The threshold row: an explicit 200,000 ns threshold against gaps of
  500,000 ticks, 119,330 ns at the rate, keeps none.
- The drift row: a long FWQ window's runtime stands the kept count
  times the projected 1,000 ns unit, exact.
- The FWQ-target row: the search lands the second count, the unit
  standing at the 1,000 ns target the true ns reading names.
- The FTQ row: the 5 us quantum holds five units, the counts the ns
  edge answers.

Failing-first reading (the pre-fix behavior): the two source files
stashed, the suite run on the amended fixture with the preamble removed
to the state the pre-fix code requires, the six new-row assertions
stand observed failing, one reading each:

```
PRE-FIX RED: the pass period stands at the projected figure (specs/045-merge-fixes phase 16, FR-039)
PRE-FIX RED: a gap the ns contract keeps below the threshold records none (specs/045-merge-fixes phase 16, FR-039)
PRE-FIX RED: the runtime stands the kept count times the projected unit, drift-free (specs/045-merge-fixes phase 16, FR-039)
PRE-FIX RED: the search lands the count whose unit stands at the true ns target (specs/045-merge-fixes phase 16, FR-039)
PRE-FIX RED: the unit time stands at the projected target figure (specs/045-merge-fixes phase 16, FR-039)
PRE-FIX RED: each quantum holds the five units the ns edge answers (specs/045-merge-fixes phase 16, FR-039)
```

The readings stand taken with the code stashed and the tests landed,
the run answering the six red lines with no contract abort and the
pre-existing rows green on the amended fixture, the identity proof the
fixture amendment stands on. The pass reading: the code restored, the
same suite answers `harness_jitter_probe_test: ok` with exit 0, every
row green. The command both readings run, one line each:
`cmake --build build --target harness_jitter_probe_test -j 48` and
`./build/test/harness_jitter_probe_test`.

One pre-existing scenario moved its vehicle: the out-of-order stamp
span scenario ran a decreasing read through the probe, and the
backward read stands refused by the projection's require now, so the
scenario's span-narrowing coverage stands tested on the hand-built
state the fold reads directly, the records and assertions the
scenario carried. The amendment traces to FR-039.

The disclosure clause lands with BFW-014 d:
`source/harness/cli.cpp`'s record assembly appends
`; time base tsc ticks scaled to ns at <R> Hz` after the threshold
clause, guarded on the measured rate, the unavailable path byte-identical;
the rate rides the per-cpu state's `tickRateHz`, set beside the loop
period from the rate the start measured. The clause spells the way the
existing clauses spell: one semicolon-separated statement, lower-case,
the figures the record carries. The document bytes row
(`theTimeBaseClauseRidesTheDisclosureUnchanged`) asserts the writer
carries the clause verbatim at the unchanged keys, schema 1.16; the CLI
rows (`scenarioRateDisclosure` with its measured-rate and rate-absent
children) assert the clause beside a measured rate and its absence at
the rate-absent identity. Both suites answer ok, exit 0. Two fixture
amendments of the cli test trace to the projection's fallout, each
named beside its row: the main provider's pair stands at the identity
rate, and the family probe's second leaf stands frozen, the spin
answering the rate-absent identity its orbit cannot bound.

The window-sided folds convert the runner's raw-tick window edges at
the state's own origin and rate, the mapping the item's reads used,
and the placement compares one time base; the rate-absent identity
passes the edges through. Three fixture pairs stand at the identity
rate their figures read: the window suite's and the hwlat suite's
second leaves at the tsc leaf's own step, and the precheck suite's at
one tick per ns, the exec'd children's mixed pair (the fixture's fake
tsc against the real provider's monotonic) standing the fiction the
rows refused. The amendments trace to FR-039.

Stands open: the coverage reading lands with BFW-015 d.

## 5. The hot-path reading (edge case, VII)

The alone-entry wall of the `docs_jitter_probe` entry, five runs per
side on thrawn at the full 48-cpu affinity mask (`sched_getaffinity`
names 48, the online list `0-47`), one run at a time, the record's
command, `--min-time=0.01`:

- Base (the branch-base binary in the scratch worktree): 0.184 s,
  0.170 s, 0.171 s, 0.175 s, 0.178 s; median 0.175 s.
- Head (the interim head `d3fa5a95`'s binary in `build`): 0.184 s,
  0.174 s, 0.168 s, 0.168 s, 0.172 s; median 0.172 s.
- The ratio of the medians stands at 0.981, inside the runs' spread:
  no regression stands measured, and the projection's per-read cost
  (one multiply, one divide, one modulo and the carried-remainder
  adds, once per read) stands inside the run's noise at this entry.

The head side stands preliminary at its pre-squash head and re-runs at
the measured head after the squash, the base side fixed. An interim
reading this record supersedes: the head side first measured 0.648 s
median against the same base, the ratio 3.714, the cause the rate
spin's recorder minting a two-column ring that wrapped under the cap,
the burn fixed at `d3fa5a95`; the superseded reading stands the record
of why the recorder's shape moved.

### The re-run at the measured head, and the load confound it names

The host this session carries a standing inference load: the maya
server holds roughly 10.7 cores' worth of CPU against the 48-cpu
affinity mask, its hottest thread sampling cpu 0 most often and 7, 25
and 26 beside it. The VII idle-host condition was therefore
unsatisfiable on the unpinned host, and the load stands recorded
beside every reading below (`/proc/loadavg` read 21-25 through the
runs).

Re-run at the measured head `a8e52a46`, interleaved base/head so the
load pairs the sides (base first in each pair):

- Base: 0.169, 0.169, 0.182, 0.189, 0.200; median 0.185 s.
- Head: 0.630, 0.630, 0.630, 0.653, 0.673; median 0.637 s.

The base readings stand within the original run's spread (0.17-0.20 at
both times); the head's do not, and the pair of discriminators below
names what moved.

- Discriminator (A), the measurement short-circuited: with
  `measureTickRate` standing at rate-absent before its first read (no
  spin, no plan, no clock leaf read anywhere in the probe), head reads
  0.168-0.175 against base 0.169-0.187: equal. The spin is the vector;
  the fix's own read path, the projection and the disclosure stand
  clean of it.
- Discriminator (B), the cpus pinned away from the load:
  `taskset -c 8-15` on both binaries, the spin live: base 0.169-0.181,
  head 0.169-0.178: equal. The ratio collapses the moment the
  calibration runs on cpus the maya load does not contend, and the
  fix's full cost (spin, projection, disclosure) stands inside noise
  on uncontended cpus.
- The fork discriminator: the rate pair measured inside a one-shot
  forked child, the child writing the rate through a pipe and exiting
  before the benchmark begins, still reads the head slow (0.658-0.670
  against base 0.191-0.211). The child's whole address space stands
  discarded at its exit, so no plan, window, ring or recorded column
  the measurement minted can reach the parent's reads: the interaction
  is not state the measurement leaves behind; it crosses a process
  boundary.
- The leaf discriminator: the spin's second leg read against
  `machine/monotonic_raw` instead of `machine/monotonic` still reads
  the head slow (0.645-0.673). The interaction is not specific to one
  clock leaf; it stands with any syscall-mode clock read the
  measurement plan carries.

### The open row the re-run names (VII, edge case)

The fix's full cost, spin, projection and disclosure together, stands
inside noise on uncontended cpus; the open row is the load-interaction
between the mono-read-heavy rate measurement and the benchmark
calibration that follows it, which under this session's standing load
reads 3.4x on the unpinned host and reads nothing on the pinned one.
The practical root cause stands named: the VII idle-host condition was
unsatisfiable on the unpinned host this session (maya at roughly 10.7
cores against a shared 48-cpu affinity mask). The owner rules the row;
pinning the measurement lanes to uncontended cpus is one ruling the
taskset discriminator already answers, and the three candidate
mechanisms stand named for it: the timekeeping page's behavior under
contention, the scheduler's placement of the calibration threads, and
cache-line sharing between the maya workload and the calibration loop.

## 6. The lanes at the measured head (FR-042, SC-037, SC-038)

The measured head: `a8e52a46`, the single squashed commit on
`fix/probe-unit-labels`. The lanes at the measured head, one reading
per command, the exit and the counts beside each:

| Lane | Command | Reading |
| --- | --- | --- |
| release | `ctest --test-dir build -j 48` | 190/190 pass |
| sanitizer | `ctest --test-dir build/sanitize -j 24` | 185/185 pass |
| tsan | `ctest --test-dir build/tsan -j 24` | 186/186 pass |
| coverage | `ctest --test-dir build/coverage -j 24` | 185/185 pass |
| coverage lines | `tools/dbc/coverage_gate.sh build/coverage/coverage.info` | 100.0% (18866 of 18866) |
| coverage branches | the same gate | shortfall 21, within the D-14 allowance of 21 |
| markers | `git grep -c LCOV_EXCL -- source include` | 84, the standing count unmoved |
| prose tree | `tools/prose/prose_gate.py --mode tree` | 0 findings |
| prose range | the same gate, `--base 1f1f9b2` | 0 findings |
| format | `cmake/lint.cmake` | clean |
| spell | `cmake/spell.cmake` | clean |
| DBC doc gate | `cmake --build build -t dbc-gate` | clean |
| DBC fixtures | `ctest -R dbc_gate_fixtures` | 1/1 pass |
| example drift | `tools/docs/example_drift.py .` (both forms) | clean |
| drift fixtures | `test/example-drift-fixture` runners | pass |
| report shape | `run_report_shape_fixtures.py` | pass |
| example run | `test/example_run.sh build` | exit 0 |
| example run fixtures | `test/example-run-fixture` | pass |
| snippet compile | `test/snippet-compile/run.sh . clang++` | 39 compiled |
| time source | `test/time_source_gate.sh .` | clean |
| exported names | `nm -D --defined-only libspeedgun-ng.so` | 250, the standing count unmoved |
| consumer | `test/consumer` against the installed prefix | builds clean |
| analyzer | clang-tidy 23.1.3, the touched files, base beside head | identical warning sets (150 and 152), no new finding |

The tsan and sanitizer lanes each met one timing-sensitive failure
under the loaded host in the batch runs; each re-run on its own read
100% and the re-run's reading stands above. The coverage branch
shortfall stood at 21 through the allowance's edge: the base tree
reads 21 beside the head's 21, the accepted classes holding, and the
dark rows of the touched file stand at the base's own count beside the
head's.

## 7. The push and the pull request (SC-038)

The branch stands one squashed commit; the push ran
`git push -u origin fix/probe-unit-labels`, the lease rule standing
beside every re-push this record's own final amend forces:
`git push --force-with-lease=fix/probe-unit-labels:<the pushed sha>`
against the sha the push before the amend left, so an upstream move
between the readings stands refused. The equality check reads
`git ls-remote origin refs/heads/fix/probe-unit-labels` beside
`git rev-parse HEAD`, and the two answer the same sha or the row
stands open.

The pull request body stands the untracked artifact
`$HOME/scratch/probe-unit-labels/pr-body.md`, the commit message's own
text with the measured-head readings beside it; the gh CLI stands
unauthenticated this session, so the operator opens the pull request
from that file and the body invents no token and no review.

## 8. Rows left open (D-31)

- O-5 and O-6: stand as phases 14 and 15 wrote them; nothing this
  phase measures touches either.
- O-7: closed by phase 15; stands closed.
- O-8: closed by FR-039 and FR-040; the projection stands the read
  move, the disclosure names the time base and the rate, and the
  fixtures read the identity the rate 0 passthrough keeps.
- The VII open row of section 5: the load-interaction between the
  mono-read-heavy rate measurement and the benchmark calibration that
  follows it stands the owner's ruling; the fix's full cost stands
  inside noise on uncontended cpus, the discriminators and the fork
  refutation stand recorded above, and the row names the practical
  root cause and the three candidate mechanisms.
