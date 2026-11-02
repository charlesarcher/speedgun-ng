# Quickstart and Validation: Vendor-neutral top-down analysis

Feature: 042-topdown. Spec: [spec.md](spec.md). Plan: [plan.md](plan.md).
This page is a run guide. Figures come from commands run on the acceptance
host on 2026-11-02 at `f2a9f14a`, and the two commits that follow it rename one
constant, move one helper behind a private header, and change no figure. Each
`...` in a quoted line marks text cut from its middle.

## Prerequisites

| Need | Check | This host |
| --- | --- | --- |
| Linux x86-64, the only supported platform | `uname -r` | `7.3.0-4-cachyos` |
| Intel acceptance host (SC-10) | `lscpu` | `Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz`, model 85, 12 cores, 2 threads per core |
| Table directory the loader selects | `cat /sys/devices/cpu/caps/pmu_name` | `skylake` |
| Counter access | `sysctl kernel.perf_event_paranoid` | `kernel.perf_event_paranoid = -1` |
| Toolchain | `c++ --version`; `cmake --version` | `GCC 16.2.1`; `cmake 4.4.2` |
| Build preset | `ci-rocky` (`CMakePresets.json:168`) | configured, writes `./build` |
| Both archives | `ls build/*.a` | `libspeedgun-ng.a`, `libspeedgun-ng-harness.a` |

All of it stands at `kernel.perf_event_paranoid` -1, the state PC-7 measured:
`counters_pmu_test` prints `fast_rdpmc entries: 497 at perf_event_paranoid -1`.

## Build

```bash
cmake --preset=ci-rocky        # Release developer mode into ./build
cmake --build build -j"$(nproc)"
```

Once per feature, build `ci-ubuntu` (`CMakePresets.json:157`) the same way. It
carries clang-tidy and cppcheck. A scratch program links the counters archive
with `-I include -I source -I build/export -ldl -lpthread`.

## Five fastest checks

Tree gates only, cheapest first.

| # | Command | Cost, wall | Verdict today |
| --- | --- | --- | --- |
| 1 | `python3 tools/docs/example_drift.py .` | 0.27 s | pass, silent |
| 2 | `cmake -P cmake/prose-lint.cmake` | 1.27 s | pass: `26 sources, 2600 units examined, 0 findings, 0 skipped` |
| 3 | `bash test/time_source_gate.sh .` | 1.48 s | pass: `time_source_gate: clean` |
| 4 | `cmake -P cmake/spell.cmake` | 4.37 s | pass |
| 5 | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | 30.02 s | pass, silent |

Gates 2 and 4 read the spec corpus, so its size moves their counts and cost,
and `tools/ci/prepush.sh:183-203` runs the same tools in its own order.
Next costs, both pass: `bash test/snippet-compile/run.sh .` 53.32 s, 37
snippets compiled, and `bash test/example_run.sh build` 134.23 s. `ctest -R topdown`
answers `No tests were found!!!`: the units stand unregistered.

## First runs

The reader's first run, once the option and the memory-bound benchmark exist:

```bash
./build/example/benchmark_example --filter '^bmSum$' --topdown=1 --min-time=1
./build/example/benchmark_example --filter '^bmSum$' --topdown=3 --min-time=1
./build/example/memory_walk --topdown=1 --min-time=1
./build/example/memory_walk --topdown=3 --min-time=1
```

All four runs execute. Each answers exit 0 with the header, the scope line
`topdown: core-wide coverage=one_thread`, the level-1 nodes and the refused rows.
The fixed-counter zeros are gone: those rows compose the alias this kernel
publishes for the counter, and the shares that publish sit inside 0 to 100.
Measured at level 1: `docs_topdown` answers frontend 65.61 and retiring 31.19,
`bmSum` within it answers frontend 0.60 and retiring 67.68, and `memory_walk`
answers frontend 0.02 and retiring 1.21. `--topdown=3` answers `level=3
passes=26` on that filter, with frontend 0.59 and retiring 67.90. Two nodes
answer `not available` on both examples, `tma_backend_bound` and
`tma_bad_speculation`, and the cause is this part's counter budget: their spines
need five general-purpose events, the part publishes four
(`Object::programmableCounters` answers 4, and CPUID leaf 0xA answers
`generalPurpose=4 width=48`), the kernel leaves one event unscheduled on its
page, and each of the two nodes answers `"availability": "GAP"` on every one of
twelve runs of `docs_topdown`, with no figure that is not a finite number.
specs/012 FR-002 marks such an action as one that measured nothing while the
other events keep their counts in their own columns. On eight general-purpose
counters the spine fits and all four shares publish, so the two missing shares
are a host fact for this part (PC-15).

The finished block prints the level it shows, the level-1 values in percent,
and each deeper node under its own parent (FR-026). Every node shows its
vendor-neutral name beside its value and the vendor name it carries (FR-027).
D-2 names the four level-1 nodes: `topdown.frontend_bound`,
`topdown.bad_speculation`, `topdown.backend_bound`, `topdown.retiring`. A
deeper request than the table holds prints its deepest level and says so
(FR-020). A gap prints with no value (FR-030), and each refused row prints once
at the level shown, with its reason (FR-031).

Those precondition refusals are closed, and the loader's own words answer it. No
row tagged `TopdownL1` carries a refusal: `#core_wide` reads the sibling list of
the cpu that carries the run (FR-042), and `#SMT_on` answers from one cpu's
sysfs sibling list (FR-054).

```bash
$ ./build/example/first_benchmark --catalog | grep 'mode=metric' | grep TopdownL1
catalog: cpu/tma_frontend_bound ... refusal=- reason=-
catalog: cpu/tma_bad_speculation ... refusal=- reason=-
```

The metric resolves, counts and publishes:

```bash
$ ./build/example/first_benchmark --metric tma_frontend_bound --dry-run --min-time=1
speedgun-ng 0.29.0 build=Release
bmSum ... tma_frontend_bound=   17.192 ratio=1.000 scaled=1 gap=0
```

The fixed-counter operands read counts, so the row that once printed `-nan`
prints a number: `de61faf3` composes the alias this kernel publishes for each
fixed counter, and PC-17 keeps a figure no arithmetic took off every surface.

```bash
$ ./build/example/first_benchmark --metric tma_info_thread_ipc --dry-run --min-time=1
bmSum ... tma_info_thread_ipc=    2.231 ratio=1.000 scaled=0 gap=0
```

## Acceptance list

SC-10 names the host, the kernel, the values, the refused rows and the test
counts. Each row holds today's measured state and what changes it.

| Entry | Measured today | Changes with |
| --- | --- | --- |
| `--topdown=1` and `--topdown=3` on one compute-bound benchmark | exit 0, the header, `topdown: core-wide coverage=one_thread`, the reference line `topdown: reference=linux-7.2.4` and one file line per contributing table; `level=1 passes=5` with `bmSum` frontend 0.60 and retiring 67.68, `level=3 passes=26` with frontend 0.59 and retiring 67.90, and `not available` for `backend_bound` and `bad_speculation`; `docs_topdown` answers 65.61 and 31.19; `--metric-method=multiplex` prints `not available` for all four | FR-053, FR-054 |
| Memory-bound benchmark (FR-058) | `./build/example/memory_walk` runs: `bmMemoryWalk iterations=  33756745`, `real time/iter (ns)=   42.564`, `bytes_per_second=94537988.242`, a 32 MiB dependent chase against `L3 cache: 19.3 MiB` (`lscpu`); `cpu/CYCLE_ACTIVITY.STALLS_MEM_ANY` covers the whole iteration; the level-1 block prints frontend 0.02 and retiring 1.21 and refuses the backend node the premise needs | FR-058, FR-053 |
| Refused-row record (FR-033) | 23 refused rows of the 102 that carry a `TopdownL` tag: 22 `UNRESOLVED` and 1 `UNSUPPORTED`, with no `UNKNOWN_CONSTANT`; by level 0/4, 0/8, 6/25, 12/34, 5/17, 0/14. Over all 279 metric rows: 91 refused, 85 `UNRESOLVED`, 4 `UNSUPPORTED`, and 2 `UNKNOWN_CONSTANT` naming `#SYSTEM_TSC_FREQ` outside the top-down set | FR-042, FR-054 |
| Live values | `zero count: cpu/tx-abort mode 1 value 0.000` printed by `./build/test/counters_pmu_test`; `tma_info_thread_ipc` answers `refusal=- reason=-` in `--catalog` and `tma_info_thread_ipc=    2.231` in a dry run | FR-053 |
| Test counts | `ctest --test-dir build --output-on-failure`: `100% tests passed out of 171` in 2097.34 s, with `harness_cli_test` `Passed  139.22 sec`; `ctest --test-dir build -E harness_cli_test -j 12 --timeout 120 --output-on-failure`: `99% tests passed, 1 tests failed out of 170` in 250.94 s, the one failure `prose_gate_fixtures` | FR-055 |
| Full suite with `harness_cli_test` | `harness_cli_test` answers `Passed  139.22 sec` inside the full suite at this HEAD. PC-8 stands recorded for the loaded builds: the same test reaches its 300 s bound in the sanitizer build (`test/CMakeLists.txt:2123-2128`), where `base_cli_matrix` measures 286.2 s against its 120 s property | FR-055 |
| Two-cpu run (FR-049) | `taskset -c 0,1 ctest --test-dir build -L quick -j 2 --timeout 120`: `99% tests passed, 1 tests failed out of 164` in 662.89 s, one failure, `prose_gate_fixtures`; the counters subset on those two cpus answers `100% tests passed out of 31` in 27.70 s; `harness_cli_test` carries no quick label | FR-049, with the units FR-055 registers |
| AMD level 2 (SC-11) | pending: no AMD host here, so no `topdown.smt_contention` line prints on hardware; the five level-1 values, the fifth name and the 100 percent statement answer in `harness_topdown_run_test` over the Zen 4 and Zen 5 fixture rows and the scripted source | FR-057, pending row and seam tests only |
| `PERF_METRICS` parts (SC-12) | pending: no host here carries `PERF_METRICS`, so no hardware claim stands for the parts that answer a level-1 breakdown from that msr; the rows of the vendored tables answer in `counters_topdown_select_test` and the scripted leaves in `harness_topdown_run_test` | FR-057, pending row and seam tests only |
| Intel hybrid parts (SC-12) | pending: no hybrid host here, so no hardware claim stands for a part whose directory publishes `cpu_core` and `cpu_atom`; the `HYBRID` refusal answers in `counters_topdown_select_test` over the event rows of `arch/x86/alderlake/` and over a second two-scope directory label | FR-057, pending row and seam tests only |
| Arm parts (SC-12) | pending: no Arm host stands on the supported platform and this feature vendors no Arm table; the `ARM` refusal answers in `counters_topdown_select_test` over an `MIDR` a test hands in, and the same identity answers the record and the console line in the two harness units | FR-057, pending row and seam tests only |

Three facts stand under those rows: no unit of this feature opens the kernel event
interface, reads a clock, or sleeps (FR-048, SC-08). The seam units prove them
by driving the host through a fake root: one directory stands where `/` stands,
and no device is opened.
`installTestProcRoot` of `source/detail/proc_root.hpp`, the root FR-054 moved to
in this branch, stands a directory in place of `/` the way
`test/source/harness_precheck_test.cpp` drives it, so a sibling list is a file a
test writes. `registerMetricHost` of
`test/source/detail/metric_fixture.hpp:311` registers a `FakeProvider` whose
object tree and scripted counters answer with no event opened. `test/tier_labels.sh`
holds the audit that keeps that registration honest: its `MASTER_SKIP` baseline at
`:80` lists the entries entitled to the `SKIP_RETURN_CODE 2` a host that grants no
event access answers, its `FORBIDDEN` list at `:97` names the constructs, and its
`determinism` fault at `:217` fails a registration that sleeps, reads a clock for
its verdict or retries. Read against `build`, the audit answers `read: 171
entries, 164 quick, 7 slow` and `ok: every entry carries one label and a budget,
and neither registration breaks FR-048`.

## Coverage run

```bash
cmake --preset=ci-coverage     # CMakePresets.json:113
cmake --build build/coverage -j "$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage -t coverage
```

The gate is `tools/dbc/coverage_gate.sh`. It demands `lines......: 100.0%`
(`:45-48`) and passes a branch shortfall only inside `allowance=21` (`:102`).
SC-09 and FR-051 hold both numbers and add no exclusion marker, and the audit of
`d454bcff..HEAD` answers no new marker line. The build answers here in 366.68 s.
The trace step stops on this host at lcov's own perl: `Perl API version v5.42.0
of XString.c does not match v5.44.0`, because the modules under
`~/.local/lib/perl5` were built for perl 5.42 while `/usr/bin/perl` answers 5.44,
and `tools/dbc/coverage_gate.sh` answers exit 3 with the tool-failure line it
reserves for itself. No ratio stands written on this host, so the ratio verdict
comes from the `coverage` job of `.github/workflows/ci.yml`, which carries its
own lcov.

## Troubleshooting

| Text you see | What it means |
| --- | --- |
| `unknown option '--topdown'; --help lists the options` | A build from before FR-019 landed. Rebuild from this branch. |
| `no reader answers the constant '#core_wide' (FR-006)` | A build from before FR-042, which answers the constant from the sibling list of the cpu that carries the run. |
| `this host publishes no value for the constant '#SMT_on' (FR-006)` | A build from before FR-054, which answers the fact from one cpu's sysfs sibling list. |
| `zero count: cpu/cpu-cycles mode 1 value 0.000` | A build from before `de61faf3`, whose fixed-counter rows compose the alias this kernel publishes for each fixed counter. The line `counters_pmu_test` prints today names `cpu/tx-abort`, a counter this host counts as zero. |
| `No such file or directory` for `memory_walk` | A build from before FR-058. `example/memory_walk.cpp` ships, and `add_example(memory_walk)` registers it. |
| `30 - prose_gate_fixtures (Failed)` | Its `timing.whole-tree` verdict times one whole-tree prose-gate run against `TIMING_BOUND_SECONDS = 10.0` (`test/prose-gate-fixture/run_prose_gate_fixtures.py:84`). Run alone it passes: three runs measured 9.123, 9.238 and 9.721 seconds over `921 sources, 169672 units examined`. Inside a loaded suite it fails, and the verdict line answers `observed=exit:0 wall:12.954s` under `-j 12` and `observed=exit:0 wall:11.565s` in the two-cpu run. PC-13 records it as a host load condition, and the timeouts the cli tests reach in the sanitizer builds sit under the same condition (PC-8). |
| `BA ==> BY, BE` with `Run again with FIX=YES to fix these errors.` | `BA` is a hex CPU model number of the vendored mapping file, and it stands on the allowlist (`.codespellrc`). A new finding needs a text change, or an allowlist line that carries its reason. |
| `XI2.CONTRASTIVE family=XI.2` | The prose gate names a contrastive phrase. State what the thing is. |
