# Quickstart and Validation: Vendor-neutral top-down analysis

Feature: 042-topdown. Spec: [spec.md](spec.md). Plan: [plan.md](plan.md).
This page is a run guide. Figures come from commands run on the acceptance
host on 2026-11-02 at `ca7e1f84`, whose code matches the `8d6da7fd` its build
carries. Each `...` in a quoted line marks text cut from its middle.

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
Measured at level 1: `docs_topdown` answers frontend 68.22 and retiring 32.39,
and `memory_walk` answers frontend 0.01 and retiring 1.16. Two nodes answer
`not available` on both examples, `tma_backend_bound` and `tma_bad_speculation`,
and the cause is this part's counter budget: their spines need five
general-purpose events, the part publishes four (`Object::programmableCounters`
answers 4, CPUID leaf 0xA agrees), the kernel leaves one event unscheduled with
`index = 0` on its page, the fast read refuses it at 66 of 66 sampling actions,
and specs/012 FR-002 marks such an action as one that measured nothing while the
other five events keep their counts in their own columns. On eight
general-purpose counters the spine fits and all four shares publish, so the two
missing shares are a host fact for this part.

The finished block prints the level it shows, the level-1 values in percent,
and each deeper node under its own parent (FR-026). Every node shows its
vendor-neutral name beside its value and the vendor name it carries (FR-027).
D-2 names the four level-1 nodes: `topdown.frontend_bound`,
`topdown.bad_speculation`, `topdown.backend_bound`, `topdown.retiring`. A
deeper request than the table holds prints its deepest level and says so
(FR-020). A gap prints with no value (FR-030), and each refused row prints once
at the level shown, with its reason (FR-031).

While the preconditions stand, those rows answer refusals, in the loader's words:
```bash
$ ./build/example/first_benchmark --catalog | grep 'mode=metric' | grep TopdownL1
catalog: machine/tma_frontend_bound ... refusal=UNKNOWN_CONSTANT reason=no reader answers the constant '#core_wide' (FR-006)
catalog: machine/tma_bad_speculation ... refusal=UNKNOWN_CONSTANT reason=this host publishes no value for the constant '#SMT_on' (FR-006)
```

All four level-1 rows answer `refusal=UNKNOWN_CONSTANT`: three name
`#core_wide`, and `tma_bad_speculation` names `#SMT_on`. One named row prints
that text ahead of the table:

```bash
$ ./build/example/first_benchmark --metric tma_frontend_bound --dry-run --min-time=1
tma_frontend_bound: tma_frontend_bound: no reader answers the constant '#core_wide' (FR-006) (UNKNOWN_CONSTANT)
bmSum ... tma_frontend_bound=    0.000 ratio=1.000 scaled=0 gap=0 refusal=absent
```

A row that reaches no host constant already publishes, so the counter path is
the remaining gap: `tma_info_thread_ipc` answers `refusal=- reason=-` in
`--catalog` and prints `tma_info_thread_ipc=     -nan` in a dry run, because
its fixed-counter operands read zero (PC-7).

## Acceptance list

SC-10 names the host, the kernel, the values, the refused rows and the test
counts. Each row holds today's measured state and what changes it.

| Entry | Measured today | Changes with |
| --- | --- | --- |
| `--topdown=1` and `--topdown=3` on one compute-bound benchmark | exit 0, the header, `topdown: core-wide coverage=one_thread`, two shares of the four, `frontend_bound` 68.22 and `retiring` 32.39, and `not available` for `backend_bound` and `bad_speculation`; `--metric-method=multiplex` prints `not available` for all four | FR-053, FR-054 |
| Memory-bound benchmark (FR-058) | `./build/example/memory_walk` runs: `bmMemoryWalk iterations=33194006`, `real time/iter (ns)=43.750`, `bytes_per_second=91628262.121`, a 32 MiB dependent chase against `L3 cache: 19.3 MiB` (`lscpu`); `cpu/CYCLE_ACTIVITY.STALLS_MEM_ANY` covers the whole iteration; the level-1 block prints two shares and refuses the backend node the premise needs | FR-058, FR-053 |
| Refused-row record (FR-033) | 69 refused rows of 102: 48 `UNKNOWN_CONSTANT` (39 `#core_wide`, 9 `#SMT_on`), 20 `UNRESOLVED`, 1 `UNSUPPORTED`; by level 4/4, 8/8, 15/25, 21/34, 10/17, 11/14 | FR-042, FR-054 |
| Live values | `zero count: cpu/cpu-cycles mode 1 value 0.000` printed by `./build/test/counters_pmu_test` | FR-053 |
| Test counts | `ctest --test-dir build -E harness_cli_test -j 12 --timeout 120 --output-on-failure`: `99% tests passed, 1 tests failed out of 165` in 252.32 s | FR-055 |
| Full suite with `harness_cli_test` | PC-8 shows no result line inside the 300 s `TIMEOUT` (`test/CMakeLists.txt:2123-2128`) and measures about 59 minutes over its 2,002 in-process calls (`test/source/harness_cli_test.cpp:1266-1290`), caused by `source/harness/cli.cpp:579` | FR-055 |
| Two-cpu run (FR-049) | `taskset -c 0,1 ctest --test-dir build -L quick -j 2 --timeout 120`: `99% tests passed, 1 tests failed out of 159` in 834.97 s, one failure, `prose_gate_fixtures`; the counters subset on those two cpus answers `100% tests passed out of 29` in 68.54 s; `harness_cli_test` carries no quick label | FR-049, with the units FR-055 registers |
| AMD level 2 (SC-11) | pending: no AMD host here, so no `topdown.smt_contention` line prints on hardware; the five level-1 values, the fifth name and the 100 percent statement answer in `harness_topdown_run_test` over the Zen 4 and Zen 5 fixture rows and the scripted source | FR-057, pending row and seam tests only |
| `PERF_METRICS` parts (SC-12) | pending: no host here carries `PERF_METRICS`, so no hardware claim stands for the parts that answer a level-1 breakdown from that msr; the rows of the vendored tables answer in `counters_topdown_select_test` and the scripted leaves in `harness_topdown_run_test` | FR-057, pending row and seam tests only |
| Intel hybrid parts (SC-12) | pending: no hybrid host here, so no hardware claim stands for a part whose directory publishes `cpu_core` and `cpu_atom`; the `HYBRID` refusal answers in `counters_topdown_select_test` over the event rows of `arch/x86/alderlake/` and over a second two-scope directory label | FR-057, pending row and seam tests only |
| Arm parts (SC-12) | pending: no Arm host stands on the supported platform and this feature vendors no Arm table; the `ARM` refusal answers in `counters_topdown_select_test` over an `MIDR` a test hands in, and the same identity answers the record and the console line in the two harness units | FR-057, pending row and seam tests only |

Three facts stand under those rows: no unit of this feature opens the kernel event
interface, reads a clock, or sleeps (FR-048, SC-08). The seam units prove them
by driving the host through a fake root: one directory stands where `/` stands,
and no device is opened.
`installTestProcRoot` of `source/harness/detail/proc_root.hpp`, the root FR-054
moves to `source/detail/proc_root.hpp`, stands a directory in place of `/` the
way `test/source/harness_precheck_test.cpp` drives it, so a sibling list is a
file a test writes. `registerMetricHost` of
`test/source/detail/metric_fixture.hpp:311-368` registers a `FakeProvider` whose
object tree and scripted counters answer with no event opened. `test/tier_labels.sh`
holds the audit that keeps the registration honest: its `MASTER_SKIP` baseline at
`:77-86` lists the entries entitled to the `SKIP_RETURN_CODE 2` a host that
grants no event access answers, and its `determinism` fault at `:91-96` fails a
registration that sleeps, reads a clock for its verdict or retries.

## Coverage run

```bash
cmake --preset=ci-coverage     # CMakePresets.json:113
cmake --build build/coverage -j "$(nproc)"
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage -t coverage
```

The gate is `tools/dbc/coverage_gate.sh`. It demands `lines......: 100.0%`
(`:45-48`) and passes a branch shortfall only inside `allowance=21` (`:102`).
SC-09 and FR-051 hold both numbers and add no exclusion marker. The run stands
unreachable: `ls build/coverage` answers `prose-gate-fixtures` alone.

## Troubleshooting

| Text you see | What it means |
| --- | --- |
| `unknown option '--topdown'; --help lists the options` | The option stands unwritten in this build (D-5, FR-019). |
| `no reader answers the constant '#core_wide' (FR-006)` | PC-5. FR-042 adds the reader; levels 1 and 2 print nothing until then. |
| `this host publishes no value for the constant '#SMT_on' (FR-006)` | PC-6. FR-054 answers the fact from one cpu's sysfs sibling list. |
| `zero count: cpu/cpu-cycles mode 1 value 0.000` | PC-7. FR-053 bounds the counter number beneath the fixed-counter marker. |
| `No such file or directory` for `memory_walk` | FR-058 stands unwritten; no shipped example is memory-bound. |
| `30 - prose_gate_fixtures (Failed)` | Its `timing.whole-tree` verdict times one whole-tree prose-gate run against `TIMING_BOUND_SECONDS = 10.0` (`test/prose-gate-fixture/run_prose_gate_fixtures.py:84`), and inside the loaded two-cpu suite run `observed=exit:0 wall:11.559s`. Run alone it passes: the timed run measures 8.911 to 8.989 seconds and `ctest --test-dir build -R '^prose_gate_fixtures$'` passes in 16.02 seconds. PC-13 and PC-8 record it as a host load condition this feature does not touch. |
| `BA ==> BY, BE` with `Run again with FIX=YES to fix these errors.` | `BA` is a hex CPU model number of the vendored mapping file, and it stands on the allowlist (`.codespellrc`). A new finding needs a text change, or an allowlist line that carries its reason. |
| `XI2.CONTRASTIVE family=XI.2` | The prose gate names a contrastive phrase. State what the thing is. |
