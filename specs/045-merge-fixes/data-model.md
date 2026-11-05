# Data Model: 045-merge-fixes

Spec: [spec.md](./spec.md). Base: master `b1dd7670`. No public layout
moves; every entity below stands in an internal namespace or a spec
directory.

## Entities

### `KernelCalls` (internal seam, gains one field)

- File: `source/counters/linux_pmu/kernel_calls.hpp` (`sg::counters::detail`).
- Fields today: eight function references, one per kernel call and
  kernel-published path (`openEvent`, `mapPage`, `unmapPage`,
  `readGroup`, `ioctlEvent`, `closeFd`, `devicesRoot`,
  `paranoidPath`).
- New field: `cpuidWords`, a function reference returning the
  `CpuidWords` value. The real set fills it from the CPUID probe that
  `table_parse.cpp:56-75` carries today.
- Relationships: read through `kernelCalls()`; installed per test
  through `installKernelCalls`.
- Contract: the struct's `\post` gains the ninth field's non-null
  rule; the real set and every scripted set fill it.

### `CpuidWords` (existing, unchanged)

- File: `source/counters/linux_pmu/pmu_ident.hpp:18-27`.
- Fields: the leaf 0 words `EAX`, `EBX`, `EDX`, `ECX`, the leaf 1
  answer flag, and the leaf 1 `EAX`.
- Relationship: input of `pmuIdentFromWords`, which stays a pure
  function of the words.

### `pmuIdentCurrent` (existing, cache removed)

- File: `source/counters/linux_pmu/table_parse.cpp:113-126`.
- Change: the function-local `static const PmuIdent` goes; each call
  reads `kernelCalls().cpuidWords()` and decodes through
  `pmuIdentFromWords`.
- Validation rule: a scripted install takes effect on the next call,
  on every host.

### `JitterLatchSnapshot` (new internal type)

- File: `source/harness/detail/jitter_probe.hpp` (`sg::harness::detail`).
- Fields, one object per probe cpu:
  - the series prefix copied at the latch line;
  - the kept count at the latch (`seriesKeptAtLatch`);
  - the dropped-sample count at the latch;
  - the five calibration figures (`workCount`, `unitTimeNs`,
    `targetNs`, `quantumNs`, `calibrationSpanNs`);
  - the method.
- Publication: the probe thread copies the fields once at the latch
  line, after `seriesKeptAtLatch` stands stored
  (`source/harness/jitter_probe.cpp:589`), and release-stores one
  flag; the mid-run fold sites acquire-load the flag before they read.
- Consumers: the mode=both record fold (`source/harness/cli.cpp:1379-1385`)
  and the method noise gate (`source/harness/cli.cpp:1302-1325`). The
  run-level and per-benchmark folds keep the join rule.
- Contract: `\pre` the flag stands acquired at the read site;
  `\post` the fold answers the same `JitterSeriesStats` the state
  fold answers over the latched window.
- Open shape decision: the `tsc` both-run gap fold
  (`source/harness/cli.cpp:1368`) either reads gap figures the
  snapshot carries or reads behind the join; the implement phase
  records the choice in the commit B2 body, and the FR-011 mapping
  confirms it.

### Marker ledger (spec artifact)

- File: the PR body, with the counts mirrored in
  `specs/045-merge-fixes/results/`.
- Fields: marker count by file; kept list with one reason per kept
  marker; the "waits for ruling" state for a marker no ruling covers.
- Baseline: 94 at `b1dd7670` (by file: `clock_provider.cpp` 17,
  `plan.cpp` 13, `group_io.cpp` 15, `system.cpp` 12, `provider.cpp` 9,
  `fast_read.cpp` 7, `fold.cpp` 5, `dbc.hpp` 4, `table_parse.cpp` 4,
  `counters_provider.hpp` 3, `detail/core.hpp` 2, `marker.cpp` 2,
  `encode.cpp` 1).
- Target: 84 at the Part A head; no net-new exclusion line.

### Prose fixture runner (existing script, reshaped)

- File: `test/prose-gate-fixture/run_prose_gate_fixtures.py`.
- State: loads `tools/prose/prose_gate.py` once through `importlib`;
  every gate call except the timing case runs in process with captured
  stdout/stderr, a per-case working directory, and the exit code read
  from the return value; `main` keeps the `GATE_RUNS` tally of every gate
  invocation and asserts the work counts of each phase, and times nothing.
- Invariant: the loader mutants re-validate per load, so the rule
  data loads per call.

### Spec 043 Phase 10 row (existing checklist rows)

- File: `specs/043-fwq-ftq/tasks.md`, rows T048 to T057 and T061.
- States: ticked (its verify commands pass at the 045 head) or open
  (a note line under the row names the 045 allowance or the owning
  043 lane). No Verify line moves.

### Acceptance record (new spec artifact)

- File: `specs/045-merge-fixes/results/`, one record per host.
- Fields: host name, kernel, GCC version, head sha; the five items of
  the spec's local acceptance section; the corsairone gap statement
  when the host is unreachable or its lcov repair fails.
