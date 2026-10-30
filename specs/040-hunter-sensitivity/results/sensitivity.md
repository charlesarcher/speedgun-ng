# The sensitivity record of specs/040

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
hunt: plan events=353 passes=89 method=PASSES estimate=49929ns budget=60s stage1=353 stage2=60
bmScanBlocks/cells:256   iterations=   7938437  real time/iter (ns)=   32.276  cpu time/iter (ns)=   32.231  overhead floor (ns)=   91.000  endpoint samples=2  label=summed
bmScanBlocks/cells:256    AGGREGATE  primary time/iter (ns) mean=   31.669 median=   31.419 sd=    0.487 cv=  0.0154 min=   31.358 max=   32.231 n=3  label=summed
bmScanBlocks/cells:512    AGGREGATE  primary time/iter (ns) mean=   59.569 median=   59.510 sd=    0.187 cv=  0.0031 min=   59.418 max=   59.777 n=3  label=summed
bmScanBlocks/cells:4096   AGGREGATE  primary time/iter (ns) mean=  718.475 median=  718.422 sd=    0.158 cv=  0.0002 min=  718.351 max=  718.653 n=3  label=summed
...
hunt: plan summary events=341/353 passes=89 budget=60s
```

The contender run prints the matching-rules disclosure once, above the
tables, and ranks under the sensitivity key:

```
hunt: baseline matched by full instance name, events by catalog address
...
hunt: rank - cpu/cache-misses instr=0.000 cv=173.21 correlation=n/a slope=n/a delta=-100.00 mark=noisy
hunt: rank - cpu/branch-misses instr=3.175 cv=68.88 correlation=n/a slope=n/a delta=200.00 mark=noisy
hunt: rank - cpu/l2_cache_req_stat.all instr=30.688 cv=16.14 correlation=n/a slope=n/a delta=-3.33 mark=noisy
...
hunt: plan summary events=341/353 passes=89 budget=60s
```

Every sensitivity cell stands `n/a`; the delta cells fill beside the
baseline; the table prints in the standing order with empty rank
fields (D-3).

## The artifacts

- `reports/baseline.json`: the baseline document, schema minor `1.9`,
  the norm `instr`, the rank key `rate`, three entries.
- `reports/contender.json`: the contender document, schema minor
  `1.9`, the norm `instr`, the rank key `sensitivity`, three entries;
  every per-entry hunt record carries `"correlation":null` and
  `"slope":null` beside the delta the match measured.

## The deviation

The quickstart's scenario names a family of at least 3 arg points, and
`bmScanBlocks` stands exactly 3. The sensitivity cells stand `n/a` in
every record: the standing family labels its argument, so the case
names read `bmScanBlocks/cells:N`, and the family grouping reads the
trailing `args:` segment alone (D-2), which leaves every instance one
family of one point; under 3 points the stamp leaves both sensitivity
fields empty (B-2, B-4). The family's aggregate medians spread from
`31.419` ns to `718.422` ns, so the time coordinate carries the spread
the figure reads; the case names keep the grouping from seeing it.
SC-001 names the shape this run aims at: the sensitivity column filled
for the shared events and the table ordered by it.
