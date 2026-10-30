# The round-trip record of specs/040

- Host: thrawn, AMD Ryzen Threadripper 9960X 24-Core Processor, 48
  threads, kernel `7.3.0-5-cachyos`; `/proc/sys/kernel/perf_event_paranoid`
  reads `2`.
- Date: 2026-10-30.
- Build: the release build at `build/`, the tree at `2a378f37` on branch
  `040-hunter-sensitivity`; the build command below stands at exit 0.
- Pair: the delta record\'s rules file `rules.txt` (35 lines), the
  address list `reports/promoted.txt` (the 20 catalog addresses the
  rules file\'s lines name, one per line), the gate files
  `reports/gate-rules.txt` (the rules file\'s lines under the
  `metric:` prefix) and `reports/gate-measured.txt` (the gate file
  with the one rule the follow-up runs measure at zero dropped), and
  the two follow-up documents `reports/baseline-rules.json` and
  `reports/contender-rules.json`.

## The commands

```
$ flock /tmp/sg040.lock cmake --build build -j8
exit 0

$ build/example/docs_labels --filter='bmScanBlocks' --min-time=0.2 \
      --counter=@specs/040-hunter-sensitivity/results/reports/promoted.txt \
      --json=specs/040-hunter-sensitivity/results/reports/baseline-rules.json
exit 0

$ build/example/docs_labels --filter='bmScanBlocks' --min-time=0.2 \
      --counter=@specs/040-hunter-sensitivity/results/reports/promoted.txt \
      --json=specs/040-hunter-sensitivity/results/reports/contender-rules.json
exit 0
```

The two follow-up runs measure the promoted events as `--counter`
columns, the addresses `rules.txt`\'s lines name; both end at status
0, and each document carries the events as report columns under the
`metric:` namespace, the columns the `--counter` arm attaches.

## The console block

The follow-up runs print the counter columns beside the time columns;
the aggregate row carries the statistics record of every counter:

```
speedgun-ng 0.26.0 build=Release
bmScanBlocks/cells:256   iterations=  11770500  real time/iter (ns)=   23.610  cpu time/iter (ns)=   23.585  ...  cpu/l2_cache_req_stat.ls_rd_blk_x=    0.000 ...
bmScanBlocks/cells:256    AGGREGATE  primary time/iter (ns) mean=   23.607 median=   23.589 sd=    0.034 cv=  0.0014 min=   23.585 max=   23.646 n=3  label=summed
...
```

## The comparisons

The comparison of the baseline hunt document against the contender
under the rules file stands refused: the hunt document carries its
rates in the per-entry hunt objects and no counter columns, so every
`counter:` rule stands a missing value in the baseline report
(specs/026 FR-036).

```
$ build/example/docs_labels --compare=specs/040-hunter-sensitivity/results/reports/baseline.json,specs/040-hunter-sensitivity/results/reports/contender-rules.json \
      --compare-rules=specs/040-hunter-sensitivity/results/rules.txt
exit 2
```

```
error: the rule 'counter:cpu/l2_cache_req_stat.ls_rd_blk_x@median:lower:10%' finds no counter:cpu/l2_cache_req_stat.ls_rd_blk_x for bmScanBlocks/cells:256 in the baseline report
```

The comparison of the two follow-up documents under the rules file
stands refused too: the `counter:` prefix names the user counters a
benchmark attaches through its state, and the `--counter` columns
stand under the `metric:` namespace, so the rules name no value the
runs carry (specs/026 FR-036).

```
$ build/example/docs_labels --compare=specs/040-hunter-sensitivity/results/reports/baseline-rules.json,specs/040-hunter-sensitivity/results/reports/contender-rules.json \
      --compare-rules=specs/040-hunter-sensitivity/results/rules.txt
exit 2
```

```
error: the rule 'counter:cpu/l2_cache_req_stat.ls_rd_blk_x@median:lower:10%' finds no counter:cpu/l2_cache_req_stat.ls_rd_blk_x for bmScanBlocks/cells:256 in the baseline report
```

The comparison under the gate file `gate-rules.txt` answers the value
resolution and stands refused at the first rule the follow-up runs
measure at zero: a percent of a zero baseline has no value
(specs/026 FR-031).

```
$ build/example/docs_labels --compare=specs/040-hunter-sensitivity/results/reports/baseline-rules.json,specs/040-hunter-sensitivity/results/reports/contender-rules.json \
      --compare-rules=specs/040-hunter-sensitivity/results/reports/gate-rules.txt
exit 2
```

```
error: the rule 'metric:cpu/ls_misal_loads.ma64@median:lower:10%' cannot measure bmScanBlocks/cells:256: the baseline of metric:cpu/ls_misal_loads.ma64 is zero
```

The comparison under the gate file `gate-measured.txt` answers the
verdicts: 34 rules over the three entries, 102 rows, 30 `IMPROVED`,
15 `OK`, 15 `REGRESSED` and 42 `NOISY`; the run ends at status 1, the
failed comparison a regressing verdict stands for (specs/026 FR-052).

```
$ build/example/docs_labels --compare=specs/040-hunter-sensitivity/results/reports/baseline-rules.json,specs/040-hunter-sensitivity/results/reports/contender-rules.json \
      --compare-rules=specs/040-hunter-sensitivity/results/reports/gate-measured.txt
exit 1
```

```
compare: baseline specs/040-hunter-sensitivity/results/reports/baseline-rules.json (speedgun 0.26.0, schema 1.9)
compare: contender specs/040-hunter-sensitivity/results/reports/contender-rules.json (speedgun 0.26.0, schema 1.9)
benchmark                rule                                    baseline       contender     change%          change  state
bmScanBlocks/cells:256   metric:cpu/ex_no_retire.not_complete@median:higher:10%          36.074          12.456    +65.471%         +23.618  REGRESSED
...
bmScanBlocks/cells:4096  metric:cpu/ls_not_halted_p0_cyc.p0_freq_cyc@median:lower:10%        3017.287        1946.248    -35.497%       -1071.039  IMPROVED
bmScanBlocks/cells:4096  metric:cpu/bp_l1_tlb_fetch_hit.all@median:higher:10%        2085.935        2075.676     +0.492%         +10.258  OK
```

## The deviation

The round trip stands on the arrangement the registry supports, and
the record names the three facts the arrangement rests on:

- The hunt document carries no counter columns, so the comparison of
  the hunt documents stands refused (specs/026 FR-036); the gate
  reads two runs that measure the promoted events.
- The `--counter` arm attaches its columns under the `metric:`
  namespace, so the emitted `counter:` rules name no value the
  follow-up runs carry (specs/026 FR-036); the gate files stand
  derived, the rules file\'s lines under the prefix the columns
  carry.
- A rule whose event measures zero stands the zero-baseline refusal
  under its relative threshold (specs/026 FR-031); the measured gate
  file drops the one rule whose event the runs measure at zero.

SC-003 names the shape this run aims at: the comparison reads the
emitted file again with no edit and answers the verdicts the rules
name. The emitted `counter:` prefix stands against the second fact
above, and the ruling on the prefix belongs to the spec.
