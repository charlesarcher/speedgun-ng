# Quickstart: Standalone Counters Library

**Feature**: `007-counters-and-timers` | **Date**: 2026-09-25 | **Plan**: [plan.md](plan.md)

A validation run guide: every section is a command plus an observable verdict (constitution X.4). Contracts: [contracts/](contracts/); entities: [data-model.md](data-model.md). Prerequisites: Linux dev box or CI-equivalent container, GCC/Clang from the dev preset, `cmake >= 3.20`, no privileges, `perf_event_paranoid` left at 2.

## 1. Build and run the suite

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev -R counters
```

Expected: all `counters_*` tests pass with zero skips beyond the documented probe gates (`counters_overhead` skips fast-mode sections with the failure named on non-x86 or gated hosts). No root, no sysctl change, no sleep-dependent test.

## 2. Standalone proof (SC-001)

```sh
cmake --build --preset=dev -t counters_standalone_example
ldd build/dev/example/counters_standalone_example | grep -i speedgun   # or: readelf -d
```

Expected: the example builds against public headers plus std only, runs, prints a folded metric line (`IPC`-shape value with ratio and scaled fields), exit 0. The link manifest names `speedgun-ng` alone; grep over the example source finds zero third-party includes.

## 3. Exactness of the fake spine (SC-006, US1, US2)

```sh
ctest --preset=dev -R "counters_core|counters_fake|counters_recorder"
```

Expected: hand-driven point sequences fold to hand-computed metrics with zero tolerance: window folds, `fold_pairs` series, first-to-last, a crafted `2^64` wrap subtracting out, ring drop accounting, ratio disclosure (`ratio 1.0 scaled false` for clock/push/fake sources; product-of-exponents for composites). Repeated `.metric()` calls return identical results while the provider read counter stays frozen (US1 scenario 5).

## 4. Dimension violations fail at compile (US1 scenario 3)

```sh
ctest --preset=dev -R counters_compile_fail
```

Expected: negative-compile cases (`bytes + monotonic` addition, mismatched-tag subtraction) fail to compile under the harness; positive cases (quotients, scalar scale) compile. The `metric_result` struct shape makes disclosure omission unrepresentable (contract C-MEA / FR-019).

## 5. Contract traps abort, release included where mandated (FR-027, A6)

```sh
ctest --preset=dev -R counters_trap
```

Expected: the trap fixture pair proves abort semantics out of process: scope misuse (`metric` before `finish`), capacity overrun under `hard_stop` (also in a release-configured checker build: memory safety never semantic-gated), push decrement at fold, invalid fold range. Absence-of-marker assertions per the dbc trap pattern.

## 6. Giraffe, out-of-tree (SC-003, US5)

```sh
cmake --build --preset=dev -t counters_giraffe_example
./build/dev/example/counters_giraffe_example   # expect exit 0
git status --porcelain source include | grep -v '^$'   # after building the example
```

Expected: the giraffe provider registers `menagerie/giraffe-2` with its `honks` counter through public headers only; a scope measures `honks / monotonic` to a honk rate from one shared sampling action; the example target runs with exit 0; the core tree is unmodified by its existence.

## 7. Clocks and push counters, unprivileged (US4)

```sh
ctest --preset=dev -R "counters_clock|counters_push"
```

Expected: monotonic, thread-CPU, and process-CPU deltas over CPU-bound work are positive and within the calibration tolerance of each other; `add(1000)` folds to exactly 1000; `bytes / monotonic` folds to the byte rate with the standard disclosure; the machine catalog lists the clock leaves `countable` with achieved modes; on x86 with a calibrated TSC the `tsc` leaf appears with frequency provenance (and the scaled flag where the platform sets it).

## 8. Zero-allocation sample path (SC-005)

```sh
ctest --preset=dev -R counters_noalloc
```

Expected: with counting global `operator new`/`delete` installed in the test TU, `sample()` over a filled recorder records zero allocations.

## 9. Fan-out across objects (SC-007, US3)

```sh
ctest --preset=dev -R counters_objects
```

Expected: fake two-package multi-core tree: enumeration reports kind, canonical path, description, parent, alias, catalog; alias and canonical path resolve to one object; selection returns exactly the matches; cross-object composition (`imc bytes / machine monotonic`) folds from one action; fan-out IPC over every core reconciles against the shared total; duplicate path/name registration fails recoverably.

## 10. PMU availability at paranoid 2 (SC-002, US6)

```sh
ctest --preset=dev -R counters_pmu
cat /proc/sys/kernel/perf_event_paranoid   # expect 2 on a default host
```

Expected on an unprivileged host: the provider opens, merges vendored tables with sysfs aliases, and hardware entries report `permission_blocked` while clocks and push stay `countable`; the suite passes. On a privileged host (developer evidence): named Intel/AMD events carry descriptions; a group (instructions + cycles) measures; IPC equals the raw-column quotient; the enabled/running ratio is disclosed.

## 11. Vendored tables and the upgrade tool (SC-009, US8)

```sh
python3 tools/pmu_events/update_pmu_events.py --check          # tree vs RECORD vs gate
ctest --preset=dev -R pmu_events_check                         # synthetic fixtures, both directions
```

Expected: `--check` exits 0 on the clean tree and exits 1 naming the file on any drift (hash or gate constant); the fixture pair passes clean and fails drifted; the check performs no network access. A configure-time tripwire fails the build when the gate constant and `RECORD` disagree (FR-043).

## 12. Budgets and calibration (SC-004, SC-010)

```sh
ctest --preset=dev -R counters_overhead
```

Expected: the clock-only plan `sample()` reports a stated nanosecond distribution (min/median/max); on a fast-capable, probe-passing host the fast-mode plan lands in the tens-of-cycles regime and the same plan forced to syscall mode in the microsecond regime, both published side by side in `docs/pages/counters-overhead.md`; on other hosts the fast sections report their skip reason. The pass check is binary on the same host: the fast-mode median is at least 100x below the syscall-mode median (SC-004). The fold-absent-from-sample-path check (SC-010) reads the same benchmark: sample-path cost tracks the read sequence, with fold cost measured separately off the path.

## 13. Full gates

```sh
cmake --build build/dev -t dbc-gate prose-lint format-check
cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize && ctest --preset=ci-sanitize
cmake --build build/coverage -t coverage
```

Expected: dbc-gate clean (every new public interface pairs doxygen contracts with runtime checks); prose-lint clean over the added prose; sanitizers clean; the 100% line/branch/DBC coverage gates hold for `include/speedgun-ng/` and `source/` additions.

## Success-criteria index

| SC | Section | Verdict source |
|---|---|---|
| SC-001 | 2 | example build+run, link manifest |
| SC-002 | 1, 10 | ctest green unprivileged at paranoid 2 |
| SC-003 | 6 | giraffe example run, core untouched |
| SC-004 | 12 | published distributions, side by side |
| SC-005 | 8 | zero allocation count |
| SC-006 | 3 | exact folds incl. wrap and series |
| SC-007 | 9 | fan-out reconciliation |
| SC-008 | 3, 4, 10 | disclosure fields end to end |
| SC-009 | 11 | `--check` both directions, no network |
| SC-010 | 12 | sample-path cost without fold cost |
