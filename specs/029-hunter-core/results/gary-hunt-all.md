# AC-10 hardware record: gary, hunter mode (Zen 5)

Task T061, acceptance AC-10, FR-093. Every figure below comes from the run
captured on 2026-10-22 in the lane directory
`/home/archerc/sg029-lanes/t061-gary/` (`hunt_rec.log`,
`gary-hunt-rec.json`, `catalog.log`, `hunt_rec_wrap.log`, `glob_wrap.log`).

The companion record `corsairone-hunt-all.md` in this directory stands at
commit `08ebe0a`. This record stands at `cb570f2` plus the uncommitted hunt
sources of the lane, and it runs `example/docs_throughput` with one instance
selected by `--filter`, while that one runs the quickstart benchmark whole.
The pair therefore covers two microarchitecture families and isolates one
variable each, host and tree.

Host: `gary`, AMD Ryzen 9 9950X3D 16-Core Processor, 32 threads, kernel
`7.2.10-1-cachyos`, one package, two L3 groups.

```
topology: packages=1 l3-groups=2 cores=16 threads=32 mask=0-31
l3 0: 0-7,16-23
l3 1: 8-15,24-31
```

## The invocation

Run in `~/sg029-t061` on `gary`, reached over ssh:

```
./docs_throughput --filter="bmEncode/items:65536" --min-time=0.2 \
  --hunt=all --hunt-top=20 --json gary-hunt-rec.json
```

Exit status 0. Wall time 0.49 s, from `date +%s%N` on either side of the
process. The host printed `speedgun-ng 0.19.0 build=Release` as the first
line of the log.

The binary is `example/docs_throughput` of this repository, built on
`corsairone` in `/home/archerc/sg029-lanes/t061-gary` with
`cmake --preset=ci-rocky` followed by
`cmake --build build --target docs_throughput`, then copied to the host.
It needs only `libstdc++`, `libm`, `libgcc_s` and `libc` at run time, and
both hosts carry glibc 2.44, so the copy runs natively:
`--list` there answers the three registered instances.

The source is a snapshot of the `029-hunter-core` working tree taken at
15:53 while HEAD stood at `cb570f2`, with the hunt sources carrying the
uncommitted work of the lane. The hashes pin what was compiled:

```
8dfd5808c76d hunt.cpp
2d9ce31254b9 hunt_cli.cpp
0c8c424d6918 hunt_plan.cpp
2496f1c2c6dd report.cpp
8e79cff61155 runner.cpp
```

Co-tenancy disclosure: the lane probe at 15:39 saw load average 0.09. The
record run went at 16:42:42 under load average 11.68, 9.49, 8.43 with three
users logged in and a co-resident process at 312 percent CPU for three days.
The machine was occupied, and the record says so.

## `perf_event_paranoid`

```
$ cat /proc/sys/kernel/perf_event_paranoid
2
```

Read inside the record run, the same value the lane probe read at 15:39.

## The device parts the catalog listed

```
$ ./docs_throughput --catalog > catalog.log
$ wc -l catalog.log
467 catalog.log
```

The parts, with the lines each one holds:

| device part | lines |
| --- | --- |
| `cpu` | 402 |
| `machine` | 35 |
| `amd_iommu_0` | 24 |
| `msr` | 4 |
| `power_core` | 1 |
| `power` | 1 |

The core part splits further, and the split is what reconciles the catalog
with the plan:

```
cpu lines: 402
cpu counter lines: 355
cpu metric lines: 47
cpu countable: 402
anchors:
catalog: cpu/cpu-cycles
catalog: cpu/instructions
```

`--hunt=all` reaches the counter lines of the core parts, so 355 `cpu`
counter lines stand behind the 353 planned events: the plan keeps
`cpu/cpu-cycles` and `cpu/instructions` as the anchors of every pass, and
they stand out of the hunted set. The 47 metric lines never enter a plan.

## The published counter limits

A pass carries the two anchors plus `limit - 2` hunted events. The record run
planned 353 events over 89 passes, which is four hunted events per pass, so
the `cpu` device published 6 programmable counters. One probe pins the same
number from a second selection:

```
$ ./docs_throughput --filter="bmScale/1" --min-time=0.02 --hunt="cpu/*" --hunt-top=3
hunt: plan events=355 passes=89 method=PASSES estimate=not available budget=none
hunt: plan summary events=343/355 passes=89 spend=not available budget=none
```

355 events over the same 89 passes is again four hunted events per pass on
top of the two anchors.

The limit stands published in two places, both of them the plan itself. No
listing prints a limit for a device wide enough to hold a pass:
`source/harness/hunt_cli.cpp` reads `Object::programmableCounters()` only to
refuse a device narrower than the three slots a pass needs, and this host
triggered that refusal for no part. A glob over each of `msr`, `power`,
`power_core` and `amd_iommu_0` exited 0 with `events=0 passes=0` and one
`not-encodable` skipped line per event, so those parts carry no plan to read
a limit from.

## The plan line and the measured passes

```
hunt: plan events=353 passes=89 method=PASSES estimate=not available budget=none
bmEncode/items:65536     iterations=     17597  real time/iter (ns)=15576.529 \
  cpu time/iter (ns)=15554.049  overhead floor (ns)=   90.000  endpoint samples=2
hunt: plan summary events=341/353 passes=89 spend=not available budget=none
```

The report of the same run gives the measured side:

```
passes 89  planned_events 353  ran_events 341  repeats 3  budget None
stages [{'first': 0, 'count': 353, 'repeats': 3}]
entry hunt: method PASSES  pass_count 90  retried_passes 1  top 20
entries 353  skipped 12
```

89 passes were planned, 90 were measured, and one pass was retried. The
skipped list holds 12 lines, all of kind `gap`, and the first three are:

```
hunt: skipped cpu/fp_ret_x87_fp_ops.all gap
hunt: skipped cpu/fp_ret_sse_avx_ops.add_sub_flops gap
hunt: skipped cpu/fp_ret_sse_avx_ops.mult_flops gap
```

The 353 entries split across the marks as 20 ranked, 282 `beyond-top`,
39 `noisy`, 12 `unrepeated`.

## The top 20 table

The console printed these 20 rows; `instr=` is the rate per 1000 instructions
and `cv` is the coefficient of variation in percent, both in the console
spelling.

| rank | address | `instr` | cv | samples |
| --- | --- | --- | --- | --- |
| 1 | `cpu/ex_ret_instr` | 1000.000 | 0.00 | 3 |
| 2 | `cpu/de_src_op_disp.all` | 800.797 | 0.00 | 3 |
| 3 | `cpu/de_dis_ops_from_decoder.any_integer_dispatch` | 800.777 | 0.00 | 3 |
| 4 | `cpu/ex_ret_ops` | 800.621 | 0.00 | 3 |
| 5 | `cpu/de_src_op_disp.op_cache` | 799.829 | 0.00 | 3 |
| 6 | `cpu/ls_dispatch.all` | 300.564 | 0.04 | 3 |
| 7 | `cpu/ls_dispatch.ld_dispatch` | 200.451 | 0.04 | 3 |
| 8 | `cpu/branch-instructions` | 200.061 | 0.00 | 3 |
| 9 | `cpu/ex_ret_brn` | 200.061 | 0.00 | 3 |
| 10 | `cpu/ex_ret_cond` | 199.931 | 0.00 | 3 |
| 11 | `cpu/ex_ret_fused_instr` | 199.898 | 0.01 | 3 |
| 12 | `cpu/de_no_dispatch_per_slot.backend_stalls` | 197.048 | 9.18 | 3 |
| 13 | `cpu/ls_not_halted_cyc` | 126.460 | 0.58 | 3 |
| 14 | `cpu/ic_tag_hit_miss.all_instruction_cache_accesses` | 111.451 | 1.76 | 3 |
| 15 | `cpu/ic_tag_hit_miss.instruction_cache_hit` | 111.253 | 1.75 | 3 |
| 16 | `cpu/op_cache_hit_miss.all_op_cache_accesses` | 108.333 | 0.55 | 3 |
| 17 | `cpu/op_cache_hit_miss.op_cache_hit` | 108.061 | 0.55 | 3 |
| 18 | `cpu/bp_l1_tlb_fetch_hit.all` | 100.375 | 0.01 | 3 |
| 19 | `cpu/bp_l1_tlb_fetch_hit.if4k` | 100.345 | 0.00 | 3 |
| 20 | `cpu/ls_dispatch.store_dispatch` | 100.110 | 0.04 | 3 |

`samples` is the window count the run states for every planned event: the
settled `--hunt-repeats` is 3 and the single stage carries all 353 events at
`repeats: 3`. The report publishes no per-row count, so the one retried pass
stands as the caveat: it gave the four events of that pass a fourth window,
and no field names which four.

The 20 rows above equal the 20 ranked entries of `gary-hunt-rec.json` in
order, in rate and in cv, compared field by field with zero mismatches.

## What this record proves, and what it leaves open

Hunter mode ran end to end on Zen 5 hardware: 353 core events entered the
plan, 90 passes were measured, 341 events produced a usable sample, 12
declared a gap, and the ranked table agrees with the report. The event that
stands first, `cpu/ex_ret_instr`, counts the same kernel event as the
`cpu/instructions` anchor, so its rate of 1000.000 per 1000 instructions is
the arithmetic identity the harness reports when an event and its divisor are
the same counter, and ranks 2 through 5, at 800 per 1000 instructions, carry
the dispatch and retire widths a Zen 5 core really has. The CV column is small
where the ratio is structural and grows where the event is sparse, which is
the behavior `--hunt-noise=10` expects of it.

Three things stand open. The pass loop kept the whole 353-event hunt inside
0.49 s of wall time, so 267 pass windows of the plan averaged about 1.8 ms
each: the `--min-time=0.2` window belongs to the instance, and the record
states no measurement of what each pass window holds. The `--json` report
carries `rate`, `cv`, `noisy`, `multiplexed` and `rank` per entry and no
sample count, so the per-row window count in the table rests on the settled
`--hunt-repeats` value beside the one retried pass. The rates are ratios to
the instruction anchor, so a co-tenancy effect that moves the event and the
anchor together stays invisible in the CV, which matters on a host at load
average 11.68. The 12 `gap` events are the twelve floating-point counters of
the catalog (`fp_ret_x87_fp_ops`, ten `fp_ret_sse_avx_ops` masks and
`fp_ops_retired_by_width.x87_uops_retired`); `bmEncode` is a byte-fill loop of
integer arithmetic (`example/docs/throughput.cpp:13`), and the kind stands as
all the run publishes for each of them.
