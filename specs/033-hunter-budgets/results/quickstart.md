# Zen 5 acceptance record: gary, hunter budgets (033)

Task T032, FR-045, SC-009, D-11. Every figure below was read out of a
run made on 2026-10-29 on `gary`, the host the feature names. The runs
went while the branch `033-hunter-budgets` stood at `74811b66`, and the
suite counts below stand at that head. That head sits on the rebase onto
the `master` that had already taken `specs/035-numa-placement`, which
moved this feature's version reservation to `0.23.0`; an earlier version
of this record named heads that a rebase had left on no branch, and the
convergence wave re-ran every row and both suite passes against a head
that stands on the branch. The shape follows
`specs/034-host-noise-precheck/results/quickstart.md`.

Host: `gary`, AMD Ryzen 9 9950X3D 16-Core Processor, 32 threads, kernel
`7.3.0-5-cachyos`, one package, two L3 groups.

```
topology: packages=1 l3-groups=2 cores=16 threads=32 mask=0-31
l3 0: 0-7,16-23
l3 1: 8-15,24-31
$ cat /proc/sys/kernel/perf_event_paranoid
2
```

The binary is `build/example/docs_throughput` of this working tree,
configured and built with `cmake --preset=ci-rocky`; it prints
`speedgun-ng 0.23.0 build=Release` as the first line of every run.

Co-tenancy disclosure: `uptime` read `load average: 3.42, 6.27, 7.30`
while these runs went and counted two users. The machine was occupied,
and the record says so.

## The budgeted acceptance

| Scenario | Command | Exit | Observation |
| --- | --- | --- | --- |
| Budgeted hunt | `./build/example/docs_throughput --filter=bmEncode --hunt=all --hunt-budget=120s,0.5e,32g --hunt-top=5 --json /tmp/033-r1.json` | 0 | plan line carries all three clause texts: `hunt: plan events=353 passes=89 method=PASSES estimate=48950ns budget=120s sample=0.5e mux=32g stage1=353 stage2=15`; no `hunt: plan mux groups=` line stands anywhere in the log, and the settlement word is `method=PASSES`, which is the pairing the contract states |
| Emitted set | `./build/example/docs_throughput --filter=bmEncode --hunt=all --hunt-budget=120s,0.5e,32g --hunt-top=5 --hunt-emit-set=/tmp/033-culprits.txt --json /tmp/033-r2.json` | 0 | `wc -l` reads 6 lines; the first two are `cpu/branch-instructions` and `cpu/ref-cycles`, the ranks 1 and 2 of both instances |
| Read back | `./build/example/docs_throughput --filter=bmEncode --counter=@/tmp/033-culprits.txt` | 0 | six counter columns on each printed row, `cpu/branch-instructions= 2050.001 ratio=1.000 scaled=0 gap=0` first; `grep -c ABSENT` over the log answers 0 |
| Conflict | `./build/example/docs_throughput --hunt-emit-set=/tmp/033-x.txt` | 2 | one line on stderr, `--hunt-emit-set: '/tmp/033-x.txt' needs --hunt`, stdout empty, and no file written |
| Two-stage budget, stage 2 standing | `./build/example/docs_throughput --filter="bmEncode/items:65536" --hunt=all --hunt-budget=120s,0.5e,32g --hunt-top=3` | 0 | `hunt: plan events=353 passes=89 method=PASSES estimate=1319870ns budget=120s sample=0.5e mux=32g stage1=353 stage2=9`: both stages named, no cut line, no `hunt: plan mux groups=` line; the summary closes `hunt: plan summary events=341/353 passes=89 budget=120s` |
| Two-stage budget, stage 2 cut | `./build/example/docs_throughput --filter="bmEncode/items:65536" --hunt=all --hunt-budget=1p,0.5e,32g --hunt-top=3 --json /tmp/033-r5d.json` | 0 | the plan line drops the stage fields whole, `hunt: plan events=353 passes=89 method=PASSES estimate=2046110ns budget=1p sample=0.5e mux=32g`, and the line below discloses the decision: `hunt: plan budget cut stage=2 reason=over-budget`; the document answers `stage_two_events: 0` and one stage |
| Sample budget below one repeat | `./build/example/docs_throughput --filter="bmEncode/items:65536" --hunt=all --hunt-budget=120s,0.001e,32g --hunt-top=3 --hunt-seed=7 --json /tmp/033-r7.json` | 0 | `hunt: plan sample repeats=3 over-budget` between the plan line and the summary, and the document of that same run writes `derived_repeats: 3` |

## What the rows hold, and what they leave open

`--filter=bmEncode` selects two instances, `bmEncode/items:1024` and
`bmEncode/items:65536`; the quickstart writes `--filter=<one>`, and the
rows above that name a bare `bmEncode` therefore ran a hunt per
instance, two plan lines and two ranked tables each. The rows that name
`bmEncode/items:65536` hold one instance.

The groups disclosure stands absent because the settlement is
`PASSES`, and the settlement is `PASSES` because the device publishes
its own limit: 353 hunted events seat over 89 passes, four events per
pass on top of the two anchors, so no pass ever holds 32 hunted events
and the `32g` cap has nothing to split. The cap still reaches the plan
line as `mux=32g`, exactly as SC-003 states for a `PASSES` settlement.
SC-003's seven groups of 32, 32, 32, 32, 32, 32 and 8 stand only in the
scripted fixture, where no device limit binds; no run on this host
produced a `hunt: plan mux groups=` line.

The seconds clause never reached the cut decision on this host. The
calibration figure a seconds budget is weighed against is the hunt
entry's own pass cost, 14830 ns at the run quoted above (`estimate_ns`
1319870 over 89 passes at the stage-1 repeat of 1), so a `1s` budget
allows some 67,000 passes while the plan costs 89 stage-1 runs plus
three stage-2 runs per repeat, at most 3089 at the option bound of
`--hunt-repeats=1000`. The cut row above reaches the decision through
the `p` clause, which bounds the pass count directly. The record states
the cut disclosure from that run and states that no seconds clause
produced it here. The estimate moves with the host: the same filter read
1344790 ns when this record was first written, 2055010 ns on the
convergence re-run and 1319870 ns here, and the runs of this sitting
spread from 1319870 ns to 2047890 ns, with the machine occupied each
time.

The emitted file is the deduplicated union of the two instances' emit
lists: five ranked addresses each, six distinct, best first. The
read-back run printed one column per line of that file and no `ABSENT`.

## The suite at the lane head

The runs above went at `74811b66`, and the suite ran at that head in the
shared `build` directory configured with `cmake --preset=ci-rocky`, on
the tree that carries `specs/035-numa-placement` and so counts 143 tests
rather than the 141 of the pre-rebase branch:

```
$ ctest --test-dir build --output-on-failure
100% tests passed out of 143
Total Test time (real) = 211.79 sec
exit status 0

$ taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure
100% tests passed out of 143
Total Test time (real) = 169.25 sec
exit status 0
```

Every test under `test/source/` passes both ways and none skips. The
pinned pass used to fail on `report_shape_jitter_sample`, the shape gate
over the jitter probe's recorded sample: the block holds 122 lines of a
many-cpu host, `precheck cpu 1 (candidate)` and the per-cpu governor and
irq-affinity rows among them, while a run pinned to two cpus prints 92
lines and answers `isolcpus=not available`. The block comes from
`cf4a0230` (`Docs: Record the quiet host the example stands on`).
`test: Fold cpu counts out of the shape gate` reads the host's cpu
multiplicity out of both sides of that comparison, so the gate states
the shape the program owns and a run under an affinity mask describes a
block recorded on a wider host. FR-042 asks for a suite that passes
pinned, and it now does.

Two coverage-lane runs named `harness_jitter_cli_test` and
`harness_hunt_json_test` as failures, and the second was a real one. The
byte row asked the fixture for a scripted run of its own per document,
and the fake provider hands its point sequence to whoever reads it, so
the second document carried other figures than the first: the row stood
in the optimized build and failed in the instrumented ones, where CI
caught it. The row now takes one scripted run and writes both documents
from it. The jitter row passed on an idle machine and in the lane that
followed it, so the record states it as the load of that machine.

## No Intel run

No Intel run stands recorded (corsairone down; the record says so).

## The CI verdict

The rebase onto the tree that landed specs/036 replaced the head this
record cited, and its run with it. The run of the rebased head closed
green: 14 jobs passed, and the docs deploy job stands skipped by its
own condition, which asks for a push to `master`.

```
run 40921391242, head 6037e712, 20m23s
lint 36s  prose-lint 46s  test 6m00s  test-rocky 20m17s
tsan 13m12s  sanitize 6m00s  coverage 15m40s  dbc-gate 1m24s
static-analysis library 13m04s, tests 37m19s, examples 18m10s
shared-audit 2m27s  downstream-consumer 2m14s  consumer-release 3m17s
```

The spans of `test-rocky` and the tests analysis carry the wait of a
first attempt that ended red, named below, so they run long beside the
total.

The coverage artifact of that run reads 100.0% lines over 13646
instrumented lines and 99.8% branches, 7044 of 7060, a 16-row shortfall
inside the 19-row allowance the gate records under D-14. The reconciled
lane takes version 0.24.0, harness SOVERSION 15 and schema minor 1.8 on
the tree that rebase landed, the next free numbers past what specs/036
took on master.

Three earlier runs of the lane came back red, and every finding they
named stood for something real. `lint` named a row the naming rename
had pushed past the column limit. `prose-lint` named a contrastive
sentence in this record, and on the run of `8a0b3b95` a double hyphen
in a commit message body, both Principle XI.1. `tsan` and `sanitize`
named the byte row of `harness_hunt_json_test`, which now takes one
scripted run for both of its documents.

The first attempt of the run above came back red on Rocky, where
`counters_linux_pmu_seam_test` aborted with stack smashing two tenths
of a second in. The code is counters-side, this lane moved none of it,
the Ubuntu job of the same run and the local suite read it green, and
the re-run of the same head passes, so the record marks the reading
unreproduced.

The run of `ffcd9555`, the head the second convergence wave replaced,
came back red on three findings. `prose-lint` named an XI.2 contrast in
that verdict record's own body, and the message left the branch with
the rewrite. `example_run` named `bmSquares/cells:8192` printing no row
on Ubuntu, and `harness_jitter_window_test` failed on Rocky; no commit
between that head and this one touches either check, and both close
green at this head, so the record marks them unreproduced.
