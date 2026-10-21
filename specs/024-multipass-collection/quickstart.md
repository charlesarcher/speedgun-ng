# Quickstart and Validation: Multi-pass metric collection

Feature: 024-multipass-collection. Contracts:
[`contracts/`](contracts/). Entities: [data-model.md](data-model.md).
Decisions: [research.md](research.md).

## Prerequisites

- Linux x86-64 (the only supported platform);
- GCC or Clang, the toolchains of the two Linux presets
  (`CMakePresets.json:139,143`);
- `cmake` at 3.20 or newer (`CMakeLists.txt:1`);
- a `build/` directory of your choice; `ci-rocky` writes `./build`.

The recorded PMU comparison runs on `corsairone` only, the host where the
spec's PC-3 measured the kernel rotating an oversize group and where its
`cpu` device publishes a limit of 4 (CPUID leaf 0AH `EAX[15:8]`, the vendor
rule of research D-01).

## Build

```bash
cmake --preset=ci-rocky        # Release dev-mode into ./build
cmake --build build -j"$(nproc)"
```

Analyzer pass, once per feature: `cmake --preset=ci-ubuntu`, then the same
build command (clang-tidy and cppcheck wired in).

## Run the test suite

```bash
ctest --test-dir build -j2 --output-on-failure
```

`ctest -j2` is the spec's invocation: its acceptance record pins `ctest -j 2`,
and the coverage preset keeps `-fprofile-update=atomic` and passes no
`-D_GLIBCXX_ASSERTIONS` (spec FR-037).
The suite must be green before any recorded run.

## Validation entry points (fake provider; run anywhere CI runs)

| Check | Command | Expect |
|---|---|---|
| Limit decode decision tables | `ctest --test-dir build -R counters_counter_limits_test` | every decoder arm passes on synthetic words: the leaf 0 vendor words both directions, the Intel and Zhaoxin core answer with a zero version answering nothing, the AMD answer behind the maximum-extended gate, the device table, the leaf 0x23 hybrid gate answering nothing, and the watchdog adjustment in first-match order including the zero that publishes no limit |
| Fact publication | `ctest --test-dir build -R counters_objects_limit_test` | the fake publishes its scripted limit through `Object::programmableCounters`; unscripted sources publish nothing |
| Planner and merge tables | `ctest --test-dir build -R harness_pass_plan_test` | first-fit order, over-limit alone, the 64-column set, empty set, gaps kept by the merge table, the two-provider ride-along |
| Pass execution and merge | `ctest --test-dir build -R harness_pass_collect_test` | calibration once, per-pass compile/open/close, merged columns from the right passes, every metric column at running ratio 1.0 with no column dropped |
| Threaded pass order | `ctest --test-dir build -R harness_pass_collect_test` (the order case) | two contexts, two passes: window opens counted per pass, no overlap; the test sleeps for nothing |
| Disclosure and identity | `ctest --test-dir build -R harness_pass_report_test` | the `passes` line and warning line match the golden; every spread fixture of [data-model.md](data-model.md) E-06 prints its pinned decimal and warning verdict; the byte-identity fixture matches the master capture |
| Option contract | `ctest --test-dir build -R harness_cli_test` | refusals and exit status 2 per [`contracts/metric-method-option.md`](contracts/metric-method-option.md); command-line-wins and last-wins |
| Option table parity | `ctest --test-dir build -R example_drift` | the README option row `--metric-method` matches the table in `source/harness/cli.cpp` |
| Docs example | `ctest --test-dir build -R example_run` | `docs_metric_passes` runs and prints its declared rows |

Manual fake walkthrough: run a harness test binary directly, for instance
`./build/test/harness_pass_report_test --gtest_filter='*spread*`, and read
the printed fixture numbers against the contract tables.

## Real-PMU validation (corsairone, outside CI)

```bash
cmake --build build
tools/pmu_events/multipass_comparison.sh build
```

The script (research D-15) runs on a master build and a feature build and
records, under `specs/024-multipass-collection/` (SC-01, SC-09, FR-035):

1. the over-limit set
   (`cpu_clks`, `instructions`, `branch-instructions`, `branches`,
   `cpu-cycles`, `bus-cycles`; the set research D-15 pins) under
   `--metric-method=multiplex`, `passes` and `auto` on one binary;
2. a single-metric reference run per event on the master build, for the
   per-metric value comparison of SC-01;
3. wall time per configuration and a quiet-window noise marker for SC-09;
4. the `passes` disclosure line and per-column `ratio=` fields of each run.

Expected: `passes` and `auto` split the set into two passes on this host
(4-counter limit, six event leaves); metric values land within the
comparison tolerance of the reference runs; `multiplex` keeps the one-plan
`ratio=` behavior of master; the one-pass fits of a small metric set print
master's bytes.

No CI job runs this script (`tools/` holds it precisely so CTest and the
workflows never see it, FR-035; research D-15).

## Reviewer and gate checks

- Prose, on this session's artifacts (untracked, so tree mode with `--paths`
  carries them; range mode covers them at implement time):

```bash
python3 tools/prose/prose_gate.py --check prose --mode tree \
  --paths \
  specs/024-multipass-collection/plan.md \
  specs/024-multipass-collection/research.md \
  specs/024-multipass-collection/data-model.md \
  specs/024-multipass-collection/quickstart.md \
  specs/024-multipass-collection/contracts/metric-method-option.md \
  specs/024-multipass-collection/contracts/metric-method-selector.md \
  specs/024-multipass-collection/contracts/programmable-counters-fact.md \
  specs/024-multipass-collection/contracts/pass-disclosure.md
```
- Spell: `codespell` over the same files (the repository `.codespellrc`
  applies).
- Drift: `python3 tools/docs/example_drift.py` once the README section and
  the `metric_passes` example exist (phase P6 of the plan).
- Docs examples: `sh test/example_run.sh build` once `docs_metric_passes`
  builds.
- No new `LCOV_EXCL` marker: `grep -rn 'LCOV_EXCL' include source` counts
  398 marker lines at the audit point, measured 2026-10-20; this feature's
  files add none (spec PC-5, FR-032).

## Determinism notes

Every table above runs without a wall clock, without sleeps and without a
PMU: the limit is a fake parameter, the fixture numbers are integers, and the
threaded order test reads counts after joins (research D-14; constitution
III.3).
