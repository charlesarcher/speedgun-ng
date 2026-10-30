# The round-trip record of specs/040

- Host: thrawn, AMD Ryzen Threadripper 9960X 24-Core Processor, 48
  threads, kernel `7.3.0-5-cachyos`; `/proc/sys/kernel/perf_event_paranoid`
  reads `2`.
- Date: 2026-10-30.
- Build: the release build at `build/`, the tree at `11d09477` on branch
  `040-hunter-sensitivity`; the build command below stands at exit 0.
- Pair: the delta record's documents, `reports/baseline-delta.json`
  and `reports/contender-delta.json`, beside `rules.txt`.

## The command

```
$ build/example/docs_labels \
      --compare=specs/040-hunter-sensitivity/results/reports/baseline-delta.json,specs/040-hunter-sensitivity/results/reports/contender-delta.json \
      --compare-rules=specs/040-hunter-sensitivity/results/rules.txt
exit 2
```

## The console block

```
error: the rule 'counter:cpu/bp_l1_tlb_fetch_hit.if2m@median:higher:10%' finds no counter:cpu/bp_l1_tlb_fetch_hit.if2m for bmScanBlocks/cells:256 in the baseline report
```

## The deviation

The comparison read `rules.txt` with no edit, parsed every rule, and
refused at the first rule's value resolution. The compare's `counter:`
rules read the counter columns a run's rows carry (specs/026), and the
hunt documents carry their event rates in the per-entry hunt objects
with no counter columns, so every rule stands a missing value and the
run ends at status 2 (specs/026 FR-036). SC-003 names the shape this
run aims at: the comparison answers the verdicts the rules name.
