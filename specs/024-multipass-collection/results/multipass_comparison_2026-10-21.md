# The FR-035 comparison run on corsairone, 2026-10-21

Hand-written record of the run that `multipass_comparison.md` beside this
file carries. The script rewrites that transcript; this note states what the
transcript does and does not prove, so a reader of SC-01 is not misled.

Command, from the repository root on `corsairone`:

```
$ ./tools/pmu_events/multipass_comparison.sh build /home/archerc/sg024-waves/024-prereq/audit-wt/build
```

The second argument is a master build (a `master` worktree at `7a75e95`,
library 0.12.0), used for the single-event reference runs. The feature build
is this branch at `db72a93` (library 0.13.0).

## What the transcript does show

The shape facts of SC-01 hold on real hardware here:

* the over-limit set under `--metric-method=multiplex` prints one plan:
  `passes  method=multiplex  count=1`, one row per metric column, each with
  its `ratio=` field, the single oversize plan of master's run. The
  disclosure line itself is new in this feature (FR-031): the master build
  prints the row and no `passes` line;
* the same set under `passes` splits into two passes:
  `passes  method=passes  count=2  spread=<n>%`, with the per-column `pass=`
  tag showing four columns in pass 0 and `bus-cycles` alone in pass 1;
* `auto` settles on the same two-pass plan, so the disclosure reads
  `method=passes` for it;
* the split run whose spread passes 5% carries the warning line the contract
  pairs with it, and the `multiplex` run prints no spread field at all.

Wall time per configuration and the host load at the run are in the
transcript. The host carried a load average near 3.7 from co-tenant build and
coverage lanes in this window, so the wall times and the pass spreads are not
a platform baseline (Principle VII). The quiet-window marker of the recorded
run is `cv= 0.0002`: the two windows of the bare body agreed to two parts in
ten thousand while the split runs of the same invocation printed spreads of
12.1% and 1.5%, so a spread of that size is a cost of the second pass. Runs of
this script across the evening gave the marker `0.0002`, `0.0006`, `0.0545`
and `0.0833` beside spreads from 1.5% to 26.4%: the marker moves with the load
of the host, and how much of a spread is host noise moves with it.

## What the transcript does not prove, and why

The numeric half of SC-01, the per-metric value against the reference run,
cannot be closed on this host in its current state. Every core-PMU column of
every run, on the feature build and on the master build alike, reads zero:

```
cpu-cycles=    0.000 ratio=1.000 scaled=0 gap=1  instructions=    0.000 ratio=1.000 scaled=0 gap=1 ...
```

`gap=1` with a zero value is the fold disclosing that the leaf delivered no
count across the window. The host condition explains those rows, and the
feature does not. Three facts fix that reading:

* the repository's own real-PMU suite names it on this host today:

  ```
  perf_event_paranoid = 2; hardware event probe granted
  zero count: cpu/cpu-cycles mode 1 value 0.000
  SKIP disclosed modes: cpu/cpu-cycles reads zero across the burn on this host, so no disclosed mode can be shown to read (FR-039)
  SKIP group read, multiplex, cpu target: no countable leaf that must move across the burn delivers a count on this host, so the kernel withholds counts and no ratio can be checked (FR-039)
  ```

  (`./build/test/counters_pmu_test`, run the same evening;)
* the master build reaches the same zeros on the same leaves, so nothing in
  this feature caused it: `--metric cpi` on master prints `cpi= -nan` (0/0)
  and `--counter cpu/cpu-cycles` prints `0.000 ratio=1.000 scaled=0 gap=1`;
* the host is not PMU-less. `perf_event_paranoid` is 2 where
  `docs/pages/counters-overhead.md` recorded 1 for the measurement that
  produced the 0.428174 and 0.543263 ratios of SC-01. A hand-written probe
  outside this script, opened and read the way the counters layer opens
  (`PERF_TYPE_HARDWARE`, `exclude_kernel=1`, per-process), does count here:

  ```
  paranoid=2
  cpu-cycles exclude_kernel=1: fd=3 errno=0 (ok)
  cpu-cycles exclude_kernel=0: fd=-1 errno=13 (Permission denied)
  instructions exclude_kernel=1: fd=4 errno=0 (ok)
  mmap=ok cap_user_rdpmc=1 time_enabled=0 time_running=0
  rdpmc raw=0
  counted cycles=360079470 acc=4e+07
  after: time_enabled=93358627 time_running=93358627
  ```

  So the conclusion "this host reaches no hardware PMU" would be false. What
  withholds the counts is specific to how the library's group is enabled at
  this `perf_event_paranoid` level, and chasing that difference is outside
  T062; it belongs to whoever owns the PMU read path, together with the fact
  that `bus-cycles` alone of the set does read (`0.013` per iteration in the
  split runs, `0.011` on master, `gap=0`).

To close SC-01 numerically: restore the host condition that
`docs/pages/counters-overhead.md` records (`perf_event_paranoid` 1), or fix
the enable path the probe above isolates, then re-run the same command. The
transcript then carries non-zero reference values beside the split values,
and the comparison the scenario asks for becomes a diff of rows.

## Where this run departs from research D-15, recorded

1. D-15 pins the set as `cpu_clks`, `instructions`, `branch-instructions`,
   `branches`, `cpu-cycles`, `bus-cycles`. Two of those names publish no leaf
   on this host: the catalog of `--catalog` has no `cpu/cpu_clks` and no
   `cpu/branches` (it has `cpu/branch-instructions` and `cpu/cpu-cycles`).
   Attached as leaves they are skipped, so the set lands on four columns at a
   published limit of four: one pass, no split, a vacuous comparison. The
   script therefore drives the over-limit set the feature ships,
   `example/docs/metric_passes.cpp`, which attaches
   `programmableCounters() + 1` single-leaf columns and so is over the limit
   on any host (five columns here).
2. The reference runs pass one event each as `--counter cpu/<event>`, not as
   `--metric <event>`: `--metric` resolves names of the vendored metric table
   only, so a single-leaf metric column cannot be asked for from the command
   line. A counter column of one leaf is the same value, the event alone in
   its own group.
3. The `--repetitions` flag does not reach the quiet-window binary: the
   family's own `repetitions(2)` wins over the command line
   (`source/harness/runner.cpp:1081`), so the marker is the `cv=` of those two
   windows. The script states that in the record and asks for nothing it
   cannot get.

## FR-035 standing of the script

`tools/pmu_events/multipass_comparison.sh` is registered with neither CMake
nor CTest (`grep -rn multipass_comparison CMakeLists.txt cmake example test
source include .github` prints nothing) and no workflow references it, so no
CI job depends on this host. It sits beside `update_pmu_events.py`, the other
PMU-facing tool.
