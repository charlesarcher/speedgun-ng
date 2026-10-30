# The refusal record of specs/040

- Host: thrawn, AMD Ryzen Threadripper 9960X 24-Core Processor, 48
  threads, kernel `7.3.0-5-cachyos`; `/proc/sys/kernel/perf_event_paranoid`
  reads `2`.
- Date: 2026-10-30.
- Build: the release build at `build/`, the tree at `11d09477` on branch
  `040-hunter-sensitivity`; the build command below stands at exit 0.

## The command

```
$ build/example/docs_labels --filter='bmScanBlocks' --hunt=all --hunt-rank=delta
exit 2
```

## The console block

The line stands on standard error:

```
--hunt-rank: 'delta' needs --hunt-baseline
```

The run prints the one line and ends at status 2 before any entry runs
(FR-011, the refusal shape of FR-023).
