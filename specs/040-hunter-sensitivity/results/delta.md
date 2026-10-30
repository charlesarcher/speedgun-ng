# The delta record of specs/040

- Host: thrawn, AMD Ryzen Threadripper 9960X 24-Core Processor, 48
  threads, kernel `7.3.0-5-cachyos`; `/proc/sys/kernel/perf_event_paranoid`
  reads `2`.
- Date: 2026-10-30.
- Build: the release build at `build/`, the tree at `11d09477` on branch
  `040-hunter-sensitivity`; the build command below stands at exit 0.
- Family: `bmScanBlocks` of `example/docs/labels.cpp`, run through
  `build/example/docs_labels`; three arg points, `cells:256`,
  `cells:512` and `cells:4096`; three repetitions per instance; the
  aggregate row's median prices each point.

## The commands

```
$ build/example/docs_labels --filter='bmScanBlocks' --min-time=0.2 \
      --hunt=all --hunt-budget=60s \
      --json=specs/040-hunter-sensitivity/results/reports/baseline-delta.json
exit 0

$ build/example/docs_labels --filter='bmScanBlocks' --min-time=0.2 \
      --hunt=all --hunt-budget=60s --hunt-rank=delta \
      --hunt-baseline=specs/040-hunter-sensitivity/results/reports/baseline-delta.json \
      --hunt-emit-rules=specs/040-hunter-sensitivity/results/rules.txt \
      --json=specs/040-hunter-sensitivity/results/reports/contender-delta.json
exit 0
```

The first command records the baseline document, the quickstart's
scenario 1; the second reruns beside it under the delta rank, with the
rules file, scenario 2. Both end at status 0.

## The console blocks

```
speedgun-ng 0.26.0 build=Release
hunt: baseline matched by full instance name, events by catalog address
hunt: plan events=353 passes=89 method=PASSES estimate=88288ns budget=60s stage1=353 stage2=60
...
hunt: rank 1 cpu/bp_l1_tlb_fetch_hit.if2m instr=15.344 cv=7.35 correlation=n/a slope=n/a delta=262.50
hunt: rank 2 cpu/ls_bad_status2.stli_other instr=1.587 cv=0.00 correlation=n/a slope=n/a delta=200.00
hunt: rank 3 cpu/bp_redirects.resync instr=0.000 cv=173.21 correlation=n/a slope=n/a delta=n/a
...
hunt: plan summary events=341/353 passes=89 budget=60s
```

The disclosure prints once, above the tables. The delta column carries
the signed percent against the matched event's baseline rate, and the
delta key orders by the delta's magnitude; the first entry's table
ranks `bp_l1_tlb_fetch_hit.if2m` first at `262.50`. The sensitivity
cells stand `n/a`, the standing registry limitation the sensitivity
record names.

## The rules file

`rules.txt` holds 23 lines, one `counter:` rule per promoted event in
rank order, each threshold the run's noise figure `10%`:

```
counter:cpu/bp_l1_tlb_fetch_hit.if2m@median:higher:10%
counter:cpu/ls_bad_status2.stli_other@median:higher:10%
counter:cpu/bp_redirects.resync@median:lower:10%
...
counter:cpu/l2_cache_req_stat.ls_rd_blk_l_hit_x@median:higher:10%
```

## The artifacts

- `reports/baseline-delta.json`: schema minor `1.9`, the norm `instr`,
  the rank key `rate`, three entries.
- `reports/contender-delta.json`: schema minor `1.9`, the norm
  `instr`, the rank key `delta`, three entries.
- `rules.txt`: the promoted events' rule lines, 23 of them.
