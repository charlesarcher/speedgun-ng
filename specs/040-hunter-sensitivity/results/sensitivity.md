# The sensitivity record of specs/040

- Host: thrawn, AMD Ryzen Threadripper 9960X 24-Core Processor, 48
  threads, kernel `7.3.0-5-cachyos`; `/proc/sys/kernel/perf_event_paranoid`
  reads `2`.
- Date: 2026-10-30.
- Build: the release build at `build/`, the tree at `2a378f37` on branch
  `040-hunter-sensitivity`; the build command below stands at exit 0.
- Family: `bmScanBlocks` of `example/docs/labels.cpp`, run through
  `build/example/docs_labels`; three arg points, `cells:256`,
  `cells:512` and `cells:4096`; three repetitions per instance; the
  aggregate row's median prices each point. The suite groups the
  family (D-2): the case names carry the labels, and the three
  entries stand one family of three points.

## The commands

```
$ flock /tmp/sg040.lock cmake --build build -j8
exit 0

$ build/example/docs_labels --filter='bmScanBlocks' --min-time=0.2 \
      --hunt=all --hunt-budget=60s \
      --json=specs/040-hunter-sensitivity/results/reports/baseline.json
exit 0

$ build/example/docs_labels --filter='bmScanBlocks' --min-time=0.2 \
      --hunt=all --hunt-budget=60s --hunt-rank=sensitivity \
      --hunt-baseline=specs/040-hunter-sensitivity/results/reports/baseline.json \
      --json=specs/040-hunter-sensitivity/results/reports/contender.json
exit 0
```

The two hunt commands carry the quickstart's scenario 1 and scenario
2: the baseline document first, the contender beside it under the
sensitivity rank. Both end at status 0.

## The console blocks

The baseline run:

```
speedgun-ng 0.26.0 build=Release
hunt: plan events=353 passes=89 method=PASSES estimate=53489ns budget=60s stage1=353 stage2=60
bmScanBlocks/cells:256    AGGREGATE  primary time/iter (ns) mean=   25.288 median=   25.287 sd=    0.008 cv=  0.0003 min=   25.281 max=   25.296 n=3  label=summed
bmScanBlocks/cells:512    AGGREGATE  primary time/iter (ns) mean=   69.856 median=   70.050 sd=    0.427 cv=  0.0061 min=   69.366 max=   70.151 n=3  label=summed
bmScanBlocks/cells:4096   AGGREGATE  primary time/iter (ns) mean=  781.739 median=  783.414 sd=   61.310 cv=  0.0784 min=  719.609 max=  842.195 n=3  label=summed
...
hunt: plan summary events=341/353 passes=89 budget=60s
```

The contender run prints the matching-rules disclosure once, above the
tables, and ranks under the sensitivity key:

```
speedgun-ng 0.26.0 build=Release
hunt: baseline matched by full instance name, events by catalog address
hunt: plan events=353 passes=89 method=PASSES estimate=86419ns budget=60s stage1=353 stage2=60
...
hunt: rank 1 cpu/de_no_dispatch_per_slot.no_ops_from_frontend instr=1796.296 cv=6.23 correlation=1.0000 slope=1.2635 delta=3.16
hunt: rank 2 cpu/ex_ret_mmx_fp_instr.sse instr=144.444 cv=0.00 correlation=1.0000 slope=0.5503 delta=0.00
hunt: rank 3 cpu/fp_ops_retired_by_width.all instr=144.974 cv=0.00 correlation=1.0000 slope=0.5492 delta=0.00
...
hunt: rank - cpu/bp_de_redirect instr=0.000 cv=173.21 correlation=0.5000 slope=-0.0007 delta=-100.00 mark=noisy
...
hunt: plan summary events=341/353 passes=89 budget=60s
```

The sensitivity cells stand measured: the family's three entries
stand one family of three points, the correlation and the slope fill
beside the rate and the CV, and the table orders by the correlation
descending. The top cell carries the correlation `1.0000` and the
slope `1.2635`; the delta cells fill beside the baseline. A noisy
event stands unranked under the key and carries its figures in the
document (D-3).

## The artifacts

- `reports/baseline.json`: the baseline document, schema minor `1.9`,
  the norm `instr`, the rank key `rate`, three entries; the per-entry
  hunt records carry the figures the sensitivity stamp left.
- `reports/contender.json`: the contender document, schema minor
  `1.9`, the norm `instr`, the rank key `sensitivity`, three entries;
  the ranked records carry the correlation, the slope and the delta,
  and the records the key ranks none of carry the figures beside the
  null rank.
