# AC-10 hardware record: corsairone, hunter mode

Date: 2026-10-22. Host: CorsairOne (Intel i9-9920X, Cascade Lake-W
layout, single core device `cpu`). Tree: `/home/archerc/sg029-lanes/head-wt`
at commit `08ebe0a`, preset `ci-rocky`, release build. Binary:
`build/example/first_benchmark`, the quickstart benchmark (one benchmark,
`bmSum`). Every output below was captured in this session; exit statuses
were captured into a shell variable before piping.

Headline: `--hunt=all` does not complete on this host at this commit. It
aborts inside `rankEvents` before any `hunt:` line prints. Smaller
selections complete but measure nothing: every event lands in the skipped
list with kind `gap`. The scripted suites pass on the same tree, so the
abort stands as host behavior; the suites stay green around it.

## 1. The exact invocations

Primary run (the AC-10 run):

```text
$ cd /tmp/t060
$ /home/archerc/sg029-lanes/head-wt/build/example/first_benchmark \
    --hunt=all --json=/tmp/t060/hunt.json
$ echo $?
134
```

The abort killed stdio buffers, so the same invocation was repeated with
line-buffered stdout to preserve whatever printed before the abort:

```text
$ stdbuf -oL /home/archerc/sg029-lanes/head-wt/build/example/first_benchmark \
    --hunt=all --json=/tmp/t060/hunt2.json > hunt2_out.txt 2> hunt2_err.txt
$ echo $?
134
$ cat hunt2_out.txt
speedgun-ng 0.19.0 build=Release
$ cat hunt2_err.txt
[assertion] every address with no refusal kind holds at least one sample
free of a disclosure, the guard `statisticsOf` carries (predicate:
!rates.empty() || event.skipped.has_value()) at
/home/archerc/sg029-lanes/head-wt/source/harness/hunt_plan.cpp:246
$ # the shell reported: Aborted (core dumped), exit status 134
```

Supporting narrow-selection invocations (same binary unless named):

```text
--hunt=cpu/branch-instructions,cpu/bus-cycles,cpu/ref-cycles --hunt-repeats=2
--hunt=cpu/branch-instructions,cpu/bus-cycles,cpu/ref-cycles \
  --hunt-repeats=2 --json=/tmp/t060/aux.json
--hunt=cpu/cache-misses,cpu/el-capacity,cpu/el-conflict,cpu/el-start,cpu/tx-abort,cpu/tx-capacity,cpu/tx-conflict,cpu/branch-instructions \
  --hunt-repeats=2 --json=/tmp/t060/aux8.json
--hunt=cpu/<one of seven single events>,cpu/branch-instructions --hunt-repeats=1
--hunt=cpu/tx-abort,cpu/tx-capacity,cpu/tx-conflict --hunt-repeats=2
--hunt=cpu/<pair of tx events> --hunt-repeats=2
```

Exit statuses: three-event runs 0; eight-event run 134; each of the seven
single-event probes 0; both four-event halves 134; the three-tx-event
selection 134; each tx pair 0.

## 2. perf_event_paranoid

```text
$ cat /proc/sys/kernel/perf_event_paranoid
2
```

## 3. The device parts the catalog listed

`first_benchmark --catalog` exited 0 and printed 24,999 `catalog:` lines,
one per address, each with `description=`, `unit=`, `mode=`, `avail=` and
`refusal=` parts. Two leading lines, head-limited:

```text
catalog: machine/monotonic description=wall-clock time, monotonic across
the window unit=nanoseconds mode=syscall avail=countable refusal=-
catalog: machine/tsc description=raw time-stamp counter ticks; a count
asserting no rate unit=none mode=fast-tsc avail=countable refusal=-
```

Device families with entry counts (captured `uniq -c` table, verbatim;
50 devices across these 15 families):

```text
  16674 uncore_cha
   2472 uncore_imc
   2354 uncore_iio
   1240 uncore_m3upi
    868 uncore_m2m
    618 cpu
    456 uncore_irp
    155 machine
    102 uncore_iio_free_running
     25 uncore_pcu
     12 uncore_ubox
      8 cstate_pkg
      7 msr
      6 cstate_core
      2 power
```

`--hunt=all` selects the entries of core devices; on this host the core
device is `cpu` with 618 catalog entries. The aborted run printed no plan
line, so its selected event count is not captured directly (§5).

## 4. The published counter limits

No accepted run prints a limit number. The limit is recorded here as the
library's own value, derived from two captured plan lines under the
planner's seating rule that one pass holds its events plus the two anchor
leaves within the device limit (contracts/hunt-api.md: limit 6 with 200
events gives 50 passes of 4 events each; `planPasses` in
`source/harness/hunt_plan.cpp` applies the same rule):

```text
hunt: plan events=2 passes=1 method=PASSES estimate=not available budget=none
hunt: plan events=3 passes=2 method=PASSES estimate=not available budget=none
```

Two events seat in one pass, so the width is at least 2; three events
need two passes, so the width is below 3. Width 2 plus the two anchors
gives:

- device `cpu`: published limit 4.
- every other device on this host: no published limit. `limitForDevice`
  in `source/counters/linux_pmu/counter_limits.cpp` answers only for
  `cpu`, `cpu_core` and `cpu_atom`; the `cpuid` and `dmesg` reads that
  could state the hardware leaf independently were permission-refused
  for this user (`cpuid: open: Permission denied`, `dmesg` empty).
- `/proc/sys/kernel/nmi_watchdog` reads `0`, so the watchdog takes no
  counter away in `usableCounters`.

## 5. The plan line with the measured pass counts

Primary run: not taken. The `--hunt=all` run aborted in `rankEvents`
before the hunt output block printed; the only stdout it left is the
version banner shown in §1, and neither `hunt.json` nor `hunt2.json`
was ever written (both absent at capture time). The field stands:

```text
Status: defect observed, not acceptance evidence
```

Narrow runs printed real plan lines, captured verbatim:

```text
hunt: plan events=3 passes=2 method=PASSES estimate=not available budget=none
hunt: plan events=2 passes=1 method=PASSES estimate=not available budget=none
hunt: plan summary events=0/3 passes=2 spend=not available budget=none
hunt: plan summary events=0/2 passes=1 spend=not available budget=none
```

The summaries say no event produced a usable sample
(`events=0/3`, `events=0/2`).

## 6. The top 20 table with sample counts and coefficients of variation

Primary run: not taken. The same abort left no rank rows and no document,
so no top 20 table and no per-event sample counts exist for `--hunt=all`.
The field stands:

```text
Status: defect observed, not acceptance evidence
```

The widest completed run selected three events; its rank rows, skipped
list and summary, captured verbatim:

```text
hunt: rank - cpu/branch-instructions instr=0.000 cv=0.00 mark=unrepeated
hunt: rank - cpu/bus-cycles instr=0.000 cv=0.00 mark=unrepeated
hunt: rank - cpu/ref-cycles instr=0.000 cv=0.00 mark=unrepeated
hunt: skipped cpu/branch-instructions gap
hunt: skipped cpu/bus-cycles gap
hunt: skipped cpu/ref-cycles gap
hunt: plan summary events=0/3 passes=2 spend=not available budget=none
```

Every coefficient of variation reads `0.00` because no event holds more
than one usable sample; the sample counts are zero usable samples per
event (summary `events=0/3`), which is what `mark=unrepeated` states per
row. The JSON document of the same run (top-level `hunt`, then the entry
`hunt` object) reads, head-limited:

```text
top-level: planned_events=3 ran_events=0 passes=2 method=PASSES rank=rate
           norm=instr repeats=2 top=20 budget=null estimate_ns=null
entries:   branch-instructions rate=0.0 cv=0.0 rank=null noisy=false
           bus-cycles          rate=0.0 cv=0.0 rank=null noisy=false
           ref-cycles          rate=0.0 cv=0.0 rank=null noisy=false
skipped:   branch-instructions kind=gap  bus-cycles kind=gap
           ref-cycles kind=gap
```

## 7. The scripted refusals and the rank equals the printed table

The scripted suites (the refusal fixtures the quickstart walkthrough
runs, since this tree carries no `hunt_rank` docs example at
`08ebe0a`) pass on this host verbatim:

```text
1/6 Test #106: harness_hunt_plan_test ...........   Passed    0.01 sec
2/6 Test #107: harness_hunt_rank_test ...........   Passed    0.01 sec
3/6 Test #108: harness_hunt_collect_test ........   Passed    0.11 sec
4/6 Test #109: harness_hunt_cli_test ............   Passed   83.54 sec
5/6 Test #110: harness_hunt_report_test .........   Passed    0.00 sec
6/6 Test #111: harness_hunt_json_test ...........   Passed    0.00 sec
100% tests passed out of 6
```

The scripted group-refusal retry case (pass fell back, retried passes,
skipped list, exit status 0) stands inside `harness_hunt_collect_test`,
green here.

The rank equals the printed table within rounding on the real narrow
run: each JSON entry matches its printed row (`rate` 0.0 against
`instr=0.000`, `cv` 0.0 against `cv=0.00`, `rank` null against the
absent rank number and `mark=unrepeated`, skipped `kind=gap` against
each `hunt: skipped ... gap` line).

## 8. What this record proves and what it does not

It proves the hunt surface works end to end on real hardware at
`08ebe0a` for what completes: the attached `--hunt` form parses, the
catalog lists this host's 50 devices, the planner seats real passes
against the `cpu` limit of 4, refusals print with their kind, the
summary counts, and the JSON matches the console within rounding. It
proves the headline defect: on this host, a selection of four or more
events aborts the process inside `rankEvents` (guard at
`source/harness/hunt_plan.cpp:246`) when an unrefused event holds no
usable sample, and the smallest captured repro is three
tsx-class events (`tx-abort`, `tx-capacity`, `tx-conflict`), every pair
of which completes. `--hunt=all` therefore cannot complete here at
`perf_event_paranoid` 2, against the spec's expectation (SC-010) that
the run stands passed with kernel-scoped refusals disclosed in the
skipped list. The disclosure of the abort is part of this record; the
pair-level boundary is the finding to hand to whoever owns the
guard and the collection path.

It does not prove the ranking quality, noise marking or budget staging
on real samples, because no completed selection measured a single sample
(the zero-sample state is consistent with the host refusing event
windows, but this record does not isolate `perf_event_paranoid` 2 as
its sole cause: raising it, `strace` and the `cpuid` device were all
outside this task's read-only, unprivileged reach). It does not cover
gary (Zen 5, T061), and it does not diagnose the root cause of the
abort, only its reproducible boundary.

## 9. Gate outputs for this file

The gate command named in the task, run from the repo root, is refused
by the gate at this commit (the `files` mode does not exist here):

```text
$ python3 tools/prose/prose_gate.py --mode files --paths specs/029-hunter-core/results/corsairone-hunt-all.md
prose_gate.py: error: argument --mode: invalid choice: 'files' (choose from 'range', 'tree')
```

The gate's `tree` mode lists tracked files (`git ls-files`), so the
record was gated on a snapshot copy under
`/home/archerc/sg029-lanes/t060/pgate` (the tool, its rules file and
this record, nothing else). The first run reported one finding
(XI2.CONTRASTIVE at an early revision of the opening paragraph); after
that sentence was rewritten, the gate reads 0 findings on every
revision since. The last run over the content before this quotation
was updated reads, verbatim:

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/029-hunter-core/results/corsairone-hunt-all.md
prose-lint: 1 sources, 267 units examined, 0 findings, 0 skipped
```

(The unit count moves with the file; the re-run after this quotation
stood also reported 0 findings, pasted in the session report.)

## 10. The fix wait and the reconfirmed defect

The hunter defect has a dedicated fixer (hwfix2) working in
`source/harness/runner.cpp` and `source/harness/hunt.cpp`, checkpointing
to `/home/archerc/.hermes/omo/waves/029-implement-hw/hwfix2.md`. This
record waited for that fix with one bounded wait of 50 minutes, polling
the checkpoint every 30 seconds for a line starting `STAGE 4`, `STAGE4`,
`STOP WHEN MET` or `VERDICT`. The checkpoint file was never created
(the waves directory holds no `hwfix2.md` at capture time) and the wait
ended on its own cap:

```text
HWFIX2-CAP-EXPIRED
```

Per the standing rule the run was taken anyway, freshly, on the second
lane (branch head `ba015f0`, two commits after `08ebe0a`, the hunter
sources identical between them). The invocation, verbatim:

```text
$ cd /tmp/t060
$ stdbuf -oL /home/archerc/sg029-lanes/t060/wt/build/example/first_benchmark \
    --hunt=all --json=/tmp/t060/lane_all.json
$ echo $?
134
```

The abort reproduces byte for byte, guard and predicate identical, the
only difference the source path naming the second lane:

```text
[assertion] every address with no refusal kind holds at least one sample
free of a disclosure, the guard `statisticsOf` carries (predicate:
!rates.empty() || event.skipped.has_value()) at
/home/archerc/sg029-lanes/t060/wt/source/harness/hunt_plan.cpp:246
```

stdout again holds only the banner and `lane_all.json` was never
written. The narrow selection on the same binary completes and shows
the empty ranking exactly as §6 records it: every rank row prints
`hunt: rank -` with no rank number, `instr=0.000`, `cv=0.00`,
`mark=unrepeated`, every event lands in the skipped list with kind
`gap`, and the summary reads `events=0/3`.

The two fields that do not depend on the fix stand unchanged and were
re-read during the wait: `perf_event_paranoid` reads `2` (§2) and the
catalog device parts stand as §3 lists them. Until the fixer's verdict
arrives, sections 5 and 6 of this record are

```text
Status: defect observed, not acceptance evidence
```

and this file is a defect record under AC-10, ready to be superseded
by the acceptance capture the fixed tree will produce.

## 11. The acceptance capture on the fixed tree

Date: 2026-10-23. Tree: `029-hunter-core` at `3df7a76` with the hunt
slice uncommitted: `skippedOf` in `source/harness/hunt.cpp` answers
`SkippedKind::ABSENT` for an address whose windows opened and yielded no
rate, and the refusal route of `source/harness/cli.cpp` sets the failed
flag its status comes from. Lane `/home/archerc/sg029-lanes/hwfix`,
preset `ci-rocky`, release build. Binary: `build/example/first_benchmark`,
the same quickstart benchmark of §1, on the same host.

```text
$ cd /home/archerc/sg029-lanes/ac10
$ stdbuf -oL /home/archerc/sg029-lanes/hwfix/build/example/first_benchmark \
    --hunt=all --json=/home/archerc/sg029-lanes/ac10/hunt.json \
    > out.txt 2> err.txt
$ echo $?
0
$ wc -l < out.txt; wc -c < err.txt
990
0
```

Section 1 captured exit 134, no `hunt:` line and a core dump. The same
invocation on the fixed tree answers 0, prints the ranking and the
skipped list, and leaves stderr empty (AC-10).

| fact | value |
| --- | --- |
| plan line | `hunt: plan events=489 passes=245 method=PASSES estimate=not available budget=none` |
| summary line | `hunt: plan summary events=0/497 passes=245 spend=not available budget=none` |
| rank rows | 489 rows, each `instr=0.000 cv=0.00 mark=unrepeated` |
| skipped lines | 497 lines: 458 `absent`, 31 `gap`, 8 `permission-blocked` |
| document | `hunt.json`, 120531 bytes, `schemaVersion` `1.4` |
| host state | `perf_event_paranoid` 2, no `perf` utility installed |

The `spend` field carries the figure the caller hands `runHuntNamed`.
The command-line path hands none, so the summary records that the run
holds no calibration figure; T080 tracks wiring the overhead floor of
the finished row into that argument.

SC-010 states that the recorded `perf_event_paranoid` value may push
kernel-scoped events into the skipped list, that the disclosure belongs
to the recorded result, and that the run stands passed. The host state
row is that disclosure.

FR-041 names the shape this host answers: a host without PMU access shows
every event in the skipped list, prints the plan, and exits 0 (D-12). The
capture matches that shape on all three counts, and the abort of §1
violated it at the third count.

The usage route was re-checked on the same binary: `--hunt=cpu/cycles`
prints `--hunt: 'cpu/cycles' is not a catalog address`, then
`BENCHMARK_NO_RUN`, and answers 2 (FR-081).

Two things this capture does not carry. No event reached a rate, so every
row stands at `instr=0.000` and the row order carries no signal; a host
that measures needs `perf_event_paranoid` below 2, or a device that
answers user-space reads. The Zen 5 record on gary stays owed under
SC-010. Sections 5 and 6 stand superseded here for the plan line and the
top-20 table; those 20 rows are attached as
`corsairone-hunt-all-top20.txt`.
