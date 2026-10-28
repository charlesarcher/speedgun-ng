# Zen 5 acceptance record: gary, hunter budgets (033)

Task T032, FR-045, SC-009, D-11. Every figure below was read out of a
run made on 2026-10-28 on `gary`, the host the feature names, while the
branch `033-hunter-budgets` stood at `6851429b`; the suite counts below
stand at the lane head `f28832c7`. The shape follows
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
`speedgun-ng 0.22.0 build=Release` as the first line of every run.

Co-tenancy disclosure: `uptime` read `load average: 7.93, 9.09, 10.13`
while these runs went, one user logged in. The machine was occupied,
and the record says so.

## The budgeted acceptance

| Scenario | Command | Exit | Observation |
| --- | --- | --- | --- |
| Budgeted hunt | `./build/example/docs_throughput --filter=bmEncode --hunt=all --hunt-budget=120s,0.5e,32g --hunt-top=5 --json /tmp/033-r1.json` | 0 | plan line carries all three clause texts: `hunt: plan events=353 passes=89 method=PASSES estimate=51620ns budget=120s sample=0.5e mux=32g stage1=353 stage2=15`; no `hunt: plan mux groups=` line stands anywhere in the log, and the settlement word is `method=PASSES`, which is the pairing the contract states |
| Emitted set | `./build/example/docs_throughput --filter=bmEncode --hunt=all --hunt-budget=120s,0.5e,32g --hunt-top=5 --hunt-emit-set=/tmp/033-culprits.txt --json /tmp/033-r2.json` | 0 | `wc -l` reads 6 lines; the first two are `cpu/branch-instructions` and `cpu/ref-cycles`, the ranks 1 and 2 of both instances |
| Read back | `./build/example/docs_throughput --filter=bmEncode --counter=@/tmp/033-culprits.txt` | 0 | six counter columns on each printed row, `cpu/branch-instructions= 2050.001 ratio=1.000 scaled=0 gap=0` first; `grep -c ABSENT` over the log answers 0 |
| Conflict | `./build/example/docs_throughput --hunt-emit-set=/tmp/033-x.txt` | 2 | one line on stderr, `--hunt-emit-set: '/tmp/033-x.txt' needs --hunt`, stdout empty, and no file written |
| Two-stage budget, stage 2 standing | `./build/example/docs_throughput --filter="bmEncode/items:65536" --hunt=all --hunt-budget=120s,0.5e,32g --hunt-top=3` | 0 | `hunt: plan events=353 passes=89 method=PASSES estimate=1344790ns budget=120s sample=0.5e mux=32g stage1=353 stage2=9`: both stages named, no cut line, no `hunt: plan mux groups=` line; the summary closes `hunt: plan summary events=341/353 passes=89 budget=120s` |
| Two-stage budget, stage 2 cut | `./build/example/docs_throughput --filter="bmEncode/items:65536" --hunt=all --hunt-budget=1p,0.5e,32g --hunt-top=3 --json /tmp/033-r5d.json` | 0 | the plan line drops the stage fields whole, `hunt: plan events=353 passes=89 method=PASSES estimate=1341141ns budget=1p sample=0.5e mux=32g`, and the line below discloses the decision: `hunt: plan budget cut stage=2 reason=over-budget`; the document answers `stage_two_events: 0` and one stage |
| Sample budget below one repeat | `./build/example/docs_throughput --filter="bmEncode/items:65536" --hunt=all --hunt-budget=120s,0.001e,32g --hunt-top=3 --hunt-seed=7` | 0 | `hunt: plan sample repeats=1 over-budget` between the plan line and the summary |

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
entry's own pass cost, 15110 ns at the run quoted above (`estimate_ns`
1344790 over 89 passes at the stage-1 repeat of 1), so a `1s` budget
allows some 66,000 passes while the plan costs 89 stage-1 runs plus
three stage-2 runs per repeat, at most 3089 at the option bound of
`--hunt-repeats=1000`. The cut row above reaches the decision through
the `p` clause, which bounds the pass count directly. The record states the cut disclosure from that
run and states that no seconds clause produced it here.

The emitted file is the deduplicated union of the two instances' emit
lists: five ranked addresses each, six distinct, best first. The
read-back run printed one column per line of that file and no `ABSENT`.

## The suite at the lane head

The runs above were made at `6851429b`. The suite ran twice at
`f28832c7`, the head the lane carries, in the shared `build` directory
configured with `cmake --preset=ci-rocky`, on a machine with nothing
else running:

```
$ ctest --test-dir build --output-on-failure
100% tests passed out of 141
Total Test time (real) = 196.94 sec
exit status 0

$ taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure
99% tests passed, 1 tests failed out of 141
Total Test time (real) = 158.47 sec
exit status 8
```

Every test under `test/source/` passes both ways and none skips. The
one pinned failure is `report_shape_jitter_sample`, the shape gate over
the jitter probe's recorded sample, and it answers the host rather than
this feature: the block holds 122 lines of a many-cpu host, `precheck
cpu 1 (candidate)` and the per-cpu governor and irq-affinity rows among
them, while a run pinned to two cpus prints 92 lines and answers
`isolcpus=not available`. The same test passes unpinned at the same
head. The block comes from `cf4a0230` (`Docs: Record the quiet host the
example stands on`), and no commit in this lane touches the jitter
example, its recorded output, or the gate.

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

The run of the pushed head closed green end to end: 14 jobs passed, and
the docs deploy job stands skipped by its own condition, which asks for
a push to `master`.

```
run 40771614859, head 9fb6e7bb, 38m17s
lint 40s  prose-lint 1m38s  test 9m20s  test-rocky 8m44s
tsan 13m28s  sanitize 7m7s  coverage 14m34s  dbc-gate 1m9s
static-analysis library 12m24s, tests, examples 17m25s
shared-audit 2m7s  downstream-consumer 2m35s  consumer-release 2m57s
```

The coverage artifact of that run reads 100.0% lines over 13110
instrumented lines and 99.8% branches, 6738 of 6754, a 16-row shortfall
inside the 19-row allowance the gate records under D-14.

Two earlier runs of the lane came back red, and every finding they
named stood for something real. `lint` named a row the naming rename
had pushed past the column limit. `prose-lint` named a contrastive
sentence in this record. `tsan` and `sanitize` named the byte row of
`harness_hunt_json_test`, which now takes one scripted run for both of
its documents.

