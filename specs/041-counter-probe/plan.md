# Implementation Plan: Counter-count group probe

**Branch**: `041-counter-probe` | **Date**: 2026-11-05 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/041-counter-probe/spec.md`

**Mode**: Test-driven. Each unit lands beside the code it covers, reads red at the
base commit `858c55d9` before that code lands, and reads green after it. No module
below ships its code without its unit, and every FR of the spec names the scenario
that proves it in `Test plan` below.

## Summary

The counters lane knows a device's programmable-counter count only from a CPUID
decode, and that decode is silent on Arm, on an Intel hybrid part that answers leaf
0x23, and on every device the decode does not name. This feature asks the kernel
instead: open one group of k copies of one general-purpose event, enable it, read the
leader once, and call the size a fit when `time_running == time_enabled` with
`time_enabled` past zero. The largest fitting size is the count. It fills the same
limit the decode fills, so the N6 planner packs passes where it multiplexes today.
One measurement program, one probe, one source label, one example, one docs section
(FR-001 to FR-076).

Three findings bind the design, and each names its evidence:

- **The open result proves nothing.** On the measured host every group open to k = 96
  answered 0, and no group above 6 was ever scheduled. A probe that read the open
  result alone would publish 32 as a limit (F-1, F-2).
- **One enabled read settles a size.** No sleep, no spin and no clock read stands
  between the enable call and that read (F-3, 50 trials of 50 in both directions), so
  the probe is syscalls only and costs the band F-5 states to reach 6 (F-5).
- **The source label needs one exported symbol.** The harness reaches no counters
  header under `source/`, the counters target carries `CXX_VISIBILITY_PRESET hidden`
  (`CMakeLists.txt:112`), and units reach internals only by a relative include that
  the harness cannot use across a shared-object boundary. See D-3a below.

## Technical Context

**Language/Version**: C++23, the standard the tree pins (`CMakeLists.txt`)

**Primary Dependencies**: the counters library and its kernel-call seam
(`source/counters/linux_pmu/kernel_calls.hpp`, PR #70), the CPUID decode of
`source/counters/linux_pmu/counter_limits.cpp`, the pass planner of specs/024
(`source/harness/pass_plan.cpp`), and the vendored JSON writer of specs/004

**Storage**: the report document (`docs/schema/report-v1.json`, one new key), the
example's `.expected` fixture, and the docs pages. No new data file, no new vendored
table, no new path read at run time beyond the seam.

**Testing**: frameworkless units with their own `main` and a scenario table, driven
through `ctest` tiers and the two-cpu mask; scripted kernel calls through
`installKernelCalls` with an RAII restore; the gate scripts `cmake/lint.cmake`,
`cmake/spell.cmake`, `tools/docs/example_drift.py`, `tools/prose/prose_gate.py` and
the two DBC gates.

**Target Platform**: Linux only, `ci-rocky` (GCC 16.2.1) and `ci-ubuntu` (clang with
clang-tidy and cppcheck). macOS and Windows stay unsupported.

**Project Type**: library plus a benchmark harness binary, with a generated docs page
set.

**Performance Goals**: the probe costs at most 528 opens and no clock read inside the
cap, and answers 6 on the measured host with 28 opens inside F-5's band (F-5). A device the
decode answers pays zero opens (FR-030). No run path gains a sleep.

**Constraints**: 100 percent line coverage, the branch allowance of 21 (ruling G-8),
the marker count of 84, `README.md` at 1004 lines or fewer (ruling G-7), ASD-STE100
throughout, and `source/counters/system.cpp` out of scope.

## Constitution Check

| Principle | Reading at this plan | Status |
|---|---|---|
| I, standard-first P0 to P3 | The probe reads the kernel's own contract for a group: `perf_event_open(2)` defines `time_enabled` and `time_running`, and the fit test is that documented pair. No vendor field is decoded by hand here. | pass |
| II, Design By Contract | The probe, its options struct, `objectLimitSource` and the source table each carry one contract with `\pre`, `\post` and, where it holds, `\invariant`. `cmake --build build --target dbc-gate` answers 347 interfaces and 0 gaps at the base; the head adds two exported interfaces and must answer 0 gaps. | pass, with a counted head reading |
| III, R-DCUT | Every module below names the requirement it realizes and the unit that proves it. Red comes first: each unit runs at the base before its code exists. | pass |
| IV, documentation | `docs/pages/harness.md` states how a limit is found (FR-060), the example carries the runnable block (FR-061), and `README.md` gains at most 3 lines (FR-063). | pass |
| VI, coverage | 100 percent lines, the branch shortfall figure stated, 84 markers or fewer, no new exclusion (FR-074). | pass, measured at the head |
| VII, performance discipline | The cost is measured before the design settled (F-5), the budget note states the worst case, and the decode path pays nothing new (FR-030). | pass |
| VIII, hard gates | Every gate in FR-075 runs on the squashed head, with the sanitizer, tsan, coverage and `ci-ubuntu` analyzer rows in the acceptance record. | pass, at the head |
| IX, spec-driven | This file, `tasks.md`, `contracts/` and the checklists stand with the spec. | pass |
| X, anti-slop | No option (D-2), no second seam (D-7), no speculative field on `Object` (D-3), and the event table is one lookup, not a general registry. | pass |
| XI, prose | ASD-STE100 in every line, and the commit template with the two trailer lines. | pass |

Two numbers move, and each names its reason in the `CMakeLists.txt` ledger: the
project version to `0.32.0`, and the counters archive `SOVERSION` from 6 to 7 for the
one new exported symbol. The harness `SOVERSION` stays 21: no exported harness layout
moves.

## D-3a: why the source label costs one exported symbol

The request recommended keeping the source in an internal record so the archive
`SOVERSION` stayed 6. Three reads of the tree settle it:

1. `source/harness/` includes no header under `source/counters/` (0 matches), and the
   counters library's `PUBLIC` include directory is `include/` alone
   (`CMakeLists.txt:148-152`).
2. The counters target builds with `CXX_VISIBILITY_PRESET hidden`
   (`CMakeLists.txt:112-113`), and `SPEEDGUN_NG_EXPORT` appears in no file under
   `source/counters/`. A counters-internal symbol is therefore invisible across the
   shared-object boundary, and `speedgun-ng::harness` is a separate installed library
   with its own `SOVERSION`.
3. Units reach internals today through a relative include from `test/source/`
   (`#include "../source/counters/linux_pmu/counter_limits.hpp"` at
   `test/source/counters_counter_limits_test.cpp:36`), which works because a unit
   compiles that header into itself. The harness cannot copy that trick without
   compiling a counters translation unit into the harness target, and that would
   break the one-owner rule for the object files.

So FR-050 is unmeasurable while the source stays unexported, and the cost is
one exported free function over a closed enumeration. The lane pays it in the
counters archive and records the reason in the commit body, the same way specs/046
recorded its accessor moving the archive from 5 to 6
(`specs/046-hunter-topdown/spec.md:89-90`). `Object` and every existing exported
layout stay as they are.

## Project Structure

### Documentation (this feature)

```text
specs/041-counter-probe/
  spec.md                      requirements, decisions, measured facts
  plan.md                      this file
  tasks.md                     the granular, parallel task list
  checklists/requirements.md   the requirements-quality checklist
  contracts/                   one contract file per module, below
  tools/group_probe.c          the requirement-1 program (FR-001)
  results/                     requirement-1 logs, the acceptance record, the PR body
```

### Source Code (repository root)

Nine modules. Each row names the files it owns alone, so two workers never open the
same file. The `Wave` column is the order the waves run in: a module starts when every
module it depends on has landed, and every module inside one wave is independent.

| # | Module | Files it owns alone | Unit it lands with | Waves | Depends on |
|---|---|---|---|---|---|
| M1 | Group probe core | `source/counters/detail/group_probe.hpp` (new), `source/counters/linux_pmu/group_probe.cpp` (new) | `test/source/counters_group_probe_test.cpp` (new) + its `test/CMakeLists.txt` hunk | 1 | seam (already on master) |
| M2 | Device and event rule | `source/counters/linux_pmu/counter_limits.hpp`, `source/counters/linux_pmu/counter_limits.cpp` | `test/source/counters_counter_limits_test.cpp` (extend) | 1 | nothing |
| M3 | Source surface | `include/speedgun-ng/counters_system.hpp` (enum + one function), `source/counters/limit_source.cpp` (new) | `test/source/counters_limit_source_test.cpp` (new) + its `test/CMakeLists.txt` hunk | 1 | nothing |
| M4 | Provider wiring | `source/counters/detail/limit_fill.hpp` with `source/counters/linux_pmu/limit_fill.cpp` (new), seated at `source/counters/linux_pmu/provider.cpp` | `test/source/counters_limit_fill_test.cpp` (new) | 2 | M1, M2, M3 |
| M5 | Plan text and record | `source/harness/runner.cpp`, `source/harness/pass_plan.cpp`, `source/harness/json_report.cpp`, `docs/schema/report-v1.json`, `docs/pages/harness.md:1395-1404` | `test/source/harness_pass_plan_test.cpp`, `test/source/harness_json_report_test.cpp`, `test/source/harness_json_schema_test.cpp`, the schema fixtures | 3 | M3, M4 |
| M6 | Example | `example/docs/counter_probe.cpp` (new), `example/docs/counter_probe.expected` (new), `example/CMakeLists.txt` | `test/example_run.sh`, the `report_shape_*` guard, `tools/docs/example_drift.py` | 3 | M3, M4 |
| M7 | Docs and main page | `docs/pages/harness.md` (N6 section, `:1568-1600` region), `README.md` | `tools/docs/example_drift.py`, `tools/prose/prose_gate.py` | 4 | M4, M5, M6 |
| M8 | Version ledger | `CMakeLists.txt` (version `0.32.0`, archive `SOVERSION` 7, the ledger comment) | the two version-binding readers already in the tree | 4 | M3 |
| M9 | Measurement and record | `specs/041-counter-probe/tools/`, `specs/041-counter-probe/results/` | the program builds with a C compiler alone | 1 | nothing |

Two files carry more than one module's edit, so the waves keep them apart:
`test/CMakeLists.txt` (M1 and M3 register in one wave, in disjoint hunks, and M1
lands first), and `docs/pages/harness.md` (M5 writes the report-key rows, M7 rewrites
the N6 rows; the ranges differ, and M7 runs after M5).

**Structure decision**: the probe is a counters-lane object, because the limit it
produces is filled where the provider publishes the limit. It joins no harness file
except the disclosure reads of M5, and it adds no build-list line: the counters tree
globs its sources with `CONFIGURE_DEPENDS` (`CMakeLists.txt:1035-1039`), so M1's and
M3's new translation units need no listing (specs/046 keeps the same rule).

## Dependency graph and the parallel waves

```text
Wave 1 (parallel, disjoint files)     Wave 2        Wave 3 (parallel)   Wave 4 (parallel)
M1 group probe core          ─┐
M2 device and event rule     ─┼──▶ M4 provider wiring ──┬─▶ M5 plan+record ─┬─▶ M7 docs+README
M3 source surface            ─┘                         └─▶ M6 example      └─▶ M8 version ledger
M9 measurement (done at spec time)
```

Within Wave 1, M1, M2, M3 and M9 own disjoint file sets and no module reads another's
header, so four workers run together. Within Wave 3, M5 and M6 run together: the
example reads the published limit and the new accessor, and the plan text reads the
same accessor, so both start when M4 lands. Wave 4's two modules touch disjoint files.

The squash rule (ruling G-23) collects all nine modules into one commit at the end,
and acceptance is measured on that head.

## Contracts

One contract file per module under `contracts/`, each stating the types, the
`\pre`/`\post` of every function, and the scenario table the unit must answer:

- `contracts/group-probe.md` (M1): `GroupProbeOptions`, `GroupProbeAnswer`,
  `GroupProbeStatus`, `probeGroupLimit`.
- `contracts/device-rule.md` (M2): `isProbeCoreDevice`, `probeEventForDevice`.
- `contracts/limit-source.md` (M3): `LimitSource`, `objectLimitSource`, and the
  internal recorder.
- `contracts/provider-fill.md` (M4): the fill order: decode, then probe, then
  publish, and the one-probe-per-device rule.
- `contracts/disclosure.md` (M5): the console line of FR-050 and the `counterLimits` record of FR-051.
- `contracts/example.md` (M6): the console shape on an answering host and on a
  refusing host.

## Test plan: TDD matrix

Every requirement names its proof. The `Reads red at the base` column is the TDD
step: the scenario runs against `858c55d9` before the module's code exists, and the
unit and its code stand in one commit.

| Requirement | Proof, by scenario | Where | Command |
|---|---|---|---|
| FR-001 to FR-004 | The program builds with `cc -O2` and no project header, and prints the three tables with the host lines | `specs/041-counter-probe/tools/group_probe.c` | `cc -O2 -o /tmp/gp specs/041-counter-probe/tools/group_probe.c && taskset -c 0,1 /tmp/gp` |
| FR-005 | Two scenarios: an oversize open that answers `EACCES`-free `EINVAL`, and an oversize open that answers 0 and runs short; both stop at the same size | `counters_group_probe_test` | `ctest --test-dir build -R counters_group_probe_test` |
| FR-010 | One options struct, no bare `bool` in the signature (a compile-time read of the header, plus the DBC gate) | `contracts/group-probe.md`, dbc gate | `cmake --build build --target dbc-gate` |
| FR-011 | The scripted opener answers every open of the probe; the probe path holds no direct `syscall` call | `counters_group_probe_test` | `ctest --test-dir build -R counters_group_probe_test` |
| FR-012 | `group_fd` of the leader is -1 and of every member is the leader's descriptor | same unit | same |
| FR-013 | The recorded attribute words show `exclude_kernel`, `disabled`, and `pinned` clear | same unit | same |
| FR-014 | A scripted open that answers 0 with `time_running` short is called a non-fit, so the answer is 6 and never 32 | same unit | same |
| FR-015 | Three stops: a refused open, a short read, and a short run, each at the same size | same unit | same |
| FR-016 | Every size to 32 fits, and the answer is no value | same unit | same |
| FR-017 | Size 1 runs short, and the answer is no value | same unit | same |
| FR-018 | Every open answers `EACCES`; the answer is no value and no size past the first is opened | same unit | same |
| FR-019 | The descriptor count at the seam returns to its start on all five paths (fit, non-fit, refusal, short read, cap) | same unit | same |
| FR-020 | The watchdog reads 1, a decode path loses one counter, and the probed figure keeps its count | `counters_limit_fill_test` (M4) | `ctest --test-dir build -R counters_limit_fill_test` |
| FR-021 | The probe path names no timer function, and the unit holds no sleep and no clock read | grep over M1's two files, plus the suite timer gate | `test/time_source_gate.sh` |
| FR-022 | Two enumerations and two plans pay one probe per device | `counters_limit_fill_test` | same |
| FR-030 | A device the decode answers pays 0 opens | `counters_limit_fill_test` | same |
| FR-031 | `cpu`, `cpu_core`, `cpu_atom` and one `armv8_pmuv3` name answer yes; `amd_l3_0` answers no | `counters_counter_limits_test` | `ctest --test-dir build -R counters_counter_limits_test` |
| FR-032 | Every uncore name answers no, and no probe opens for it | both units above | both |
| FR-033 | The x86 answer names the branch-instructions event; the Arm answer names a listed event that is neither cycles nor instructions | `counters_counter_limits_test` | same |
| FR-034 | A device with no listed event gets no probe and no limit | `counters_counter_limits_test`, `counters_limit_fill_test` | both |
| FR-040 | A probed count reaches `Object::programmableCounters()` through the fake and the provider | `counters_objects_limit_test`, `counters_limit_fill_test` | `ctest --test-dir build -R 'counters_objects_limit_test|counters_limit_fill_test'` |
| FR-040a | `objectLimitSource` answers `Decode`, `Probe` and `None` in the three scripted states, and `None` for a path no provider recorded | `counters_limit_source_test` | `ctest --test-dir build -R counters_limit_source_test` |
| FR-041 | One leaf set plans packed passes with a probed limit and multiplexes with none | `harness_pass_plan_test` | `ctest --test-dir build -R harness_pass_plan_test` |
| FR-042 | The decoded path keeps its deduction and the probed path takes none | `counters_counter_limits_test`, `counters_limit_fill_test` | both |
| FR-043 | A probe that answers no value leaves the run's method, exit status and console rows as the base left them | `harness_pass_plan_test`, `example_run` | `ctest --test-dir build -R 'harness_pass_plan_test|example_run'` |
| FR-044 | A decoded 1 still publishes no value | `counters_counter_limits_test` | same |
| FR-045 | The same count through the decoded path and the probed path produces one plan, and a small count raises the same hunt refusal in both | `harness_pass_plan_test`, `harness_hunt_cli_test` | `ctest --test-dir build -R 'harness_pass_plan_test|harness_hunt_cli_test'` |
| FR-050 | The console line names `probe`, and it prints only where the record names the probe. A decoded run prints nothing | `harness_limit_disclosure_test` | `ctest --test-dir build -R harness_limit_disclosure_test` |
| FR-051 | The document carries the key, the schema lists it, and `kSchemaVersion` reads 1.15 | `harness_json_report_test`, `harness_json_schema_test` | `ctest --test-dir build -R 'harness_json_report_test|harness_json_schema_test'` |
| FR-052 | A pinned pre-1.15 document loads, and the reader answers no source | `harness_json_load_test` | `ctest --test-dir build -R harness_json_load_test` |
| FR-053 | `soVersionNumbers` reads the moved archive number, and the two binding readers pass | the version-binding readers already registered | `ctest --test-dir build -R soVersionNumbers` |
| FR-060 | The docs page states the decode, the probe, the cost, and the no-deduction rule | `tools/docs/example_drift.py`, prose gate | `python3 tools/docs/example_drift.py` |
| FR-061, FR-062 | The example builds, runs, matches its `.expected`, and prints `no limit` with exit 0 where the kernel refuses | `example_run`, `report_shape_counter_probe_sample`, example_drift | `ctest --test-dir build -R 'example_run|example_drift'` |
| FR-063 | `README.md` reads 1004 lines or fewer, and the drift gate passes | example_drift, plus the count in the record | `wc -l README.md` |
| FR-064 | Drift gate green with the new example registered | example_drift | same |
| FR-070 | Each unit runs at `858c55d9` and reads red before its code lands; the record lists each red reading | the record | `git stash` of the code half, per module |
| FR-071 | The seven scripted shapes named in the requirement, one scenario each | `counters_group_probe_test`, `counters_limit_fill_test` | both |
| FR-072 | No sleep, no clock read, no invented cpu; the whole suite passes under the two-cpu mask | `test/time_source_gate.sh`, suite | `taskset -c 0,1 ctest --test-dir build --output-on-failure` |
| FR-073 | Real opens stand in the program and the example only | the coverage read plus a grep of `test/source/` | `git grep -l perf_event_open test/source/` |
| FR-074 | Line coverage 100 percent, branch shortfall stated and 21 or less, marker count 84 or less | `ci-coverage` preset and the coverage gate | the coverage block of the acceptance record |
| FR-075 | Every gate over the squashed head | the five gate commands plus sanitizers | the gate block of the acceptance record |
| FR-076 | One commit after the squash, no merge commit, no CI file in the diff | the record | `git log --format=%Hc%n` and the diff list |

## Gate and acceptance plan

Gates, on the squashed head (ruling G-23): `cmake -P cmake/lint.cmake`, `cmake -P
cmake/spell.cmake`, `python3 tools/docs/example_drift.py`, `python3
tools/prose/prose_gate.py --base origin/master`, `cmake --build build --target
dbc-gate`, `test/time_source_gate.sh`, the full and two-cpu `ctest` rows, `ci-sanitize`,
`ci-tsan`, `ci-coverage` with the gate figures, and `ci-ubuntu` for the analyzer row.

Acceptance on this host (ruling G-20) carries: the requirement-1 table; the
check-mode equality of the probe against the decode on `cpu`; both `ctest` rows; the
sanitizer and tsan counts; the coverage figures with the branch shortfall and the
marker count; the example's output; the pending rows; and the host, kernel, compiler
and squashed head sha.

What this host accepts, and what stays pending with no hardware claim: the
requirement-1 measurement, the check-mode equality on the core device, and every seam
test. Pending: Intel non-hybrid check-mode equality, Arm `armv8_*`, Intel hybrid with
leaf 0x23 answered, and AMD Zen 4.

## Risks

| Risk | Reading at plan time | Mitigation |
|---|---|---|
| A small probed count adds a hunt refusal | `settleMethod` reads whether any source publishes a limit, and `planPasses` reads the number, so a probed 4 packs like a decoded 4; the hunt refusal at `hunt_cli.cpp:712-738` reads the same figure | FR-045 with one test per reader; the record names any refusal a probed limit raises |
| The example cannot match a byte-exact `.expected` on a host-dependent figure | `example_drift` compares bytes for the marked region, and the two working shapes on a refusing host are degrade-the-columns and script-the-records (`example/docs/counter_metrics.cpp`, `example/docs/hunt_topdown.cpp`) | M6 takes the degrade shape: stable words, one line per core device, and the `no limit` answer on a refusing host |
| Another lane moves 1.14 or `SOVERSION` 6 first | Both files are single-line, and the rebase rule of the spec settles a collision | Re-read the two lines at every rebase and take the next free number |
| The probe opens on a host that gates it | Every refusal answers `None` (FR-018), and the measured host answers every shape at `perf_event_paranoid` 2 | The scripted refusal scenarios, plus the example on a refusing host |
| `linkRatioSlots` defect on this host | specs/046's AMD acceptance record names a hunt abort on the passes path at `858c55d9`, and M5's plan-text tests drive scripted objects, not a live hunt | M5 keeps to scripted identities; the abort stands with its own lane, and the acceptance record names it as a known host blocker |

## Complexity Tracking

No gate needs an exception. Two numbers move for one reason each (the version, and the
archive `SOVERSION` for one exported symbol), and the spec, this plan, the commit body
and the pull request body each state that reason.


---

## Acceptance plan: Intel final acceptance (2026-11-07) - the counter-count part

The shared plan and the command table stand in `specs/048-hwlat-probe/plan.md`. This
part reads one Intel core device and one example, and writes one record.

- **Reads**: `/sys/devices/cpu/caps/pmu_name`, `<device>/type`,
  `<device>/events/branch-instructions`, the device list under
  `/sys/bus/event_source/devices`, `kernel.perf_event_paranoid`, `kernel.nmi_watchdog`
  and the file modes of the paths the run touches.
- **Run**: `./build/example/docs_counter_probe`, the example of this spec, on a tree
  built at the measured head.
- **What the record gives**: the device line as this host read it, the published limit
  with its source word, the example's own `probe=` figure, the watchdog value, and the
  statement that the deduction is a no-op on a part with a fixed cycles counter.
- **What it may not give**: a hand subtraction, a raw config word, a typed probe tool,
  a sysctl write, a byte-match claim against another host's `.expected` file, or a
  status for the Arm, Intel hybrid and Zen 4 rows, which stay pending.
- **Refusal route**: a refused open or an absent core device marks the hardware row
  blocked and carries no figure.


## Acceptance closeout: measured Intel evidence (2026-11-08) - the counter-count part

**Spec**: [spec.md](./spec.md), `IP-001` and `IP-002`. The shared context, the
order of work and the lane table stand in
[../048-hwlat-probe/plan.md](../048-hwlat-probe/plan.md) under
`Plan: Acceptance closeout, measured Intel evidence (2026-11-08)`. This section
names only what the counter-count record carries.

### What the closeout runs here

- Three consoles of `./build/example/docs_counter_probe` at the final head, each
  alone with its load witness, each printing its limit line. The tool surface
  stays `counter_probe` because the standing record runs that one binary.
  `probe_window` takes no raw event word and answers no row here. IP-002 bars no
  existing hardware test tool, and this pass adds none (048 ruling 14).
- Two parity rows, never one. One row stands measured at the standing
  `perf_event_paranoid` value of `-1`: `counter limit: cpu limit=4 source=decode
  probe=4` at exit 0. One row stands BLOCKED for the `2` reading SC-010 asks
  for, and names the control, the value and the missing right (048 ruling 11).
- Opens and reads, stated as two numbers: the library path answers the limit with
  no device file open, and the example's own search opens up to 15 files and
  reads up to 5 before it answers 4. One number a run measures, one the standing
  proof arithmetic derives, and no acceptance row runs a tracer (048 ruling 13).

### What the closeout corrects in this record

- The parity row at `results/acceptance-final-intel.md:96-97` names
  `/dev/cpu/*/msr` as its reason. A group open is a syscall against the core
  device, and the `msr` node stands behind a different device type on this host
  (`/sys/bus/event_source/devices/msr/type` answers 11, `cpu/type` answers 4),
  so the node explains no refusal of group access. The blocked reason becomes the
  write to `/proc/sys/kernel/perf_event_paranoid`, which is the reason the record
  already gives at `:64-68`.
- `results/requirement-1-shipped-shape.txt:100`, `:107` and `:123` stand as
  history for a tool copy under `/tmp` that no longer exists. New rows name the
  tracked source and the built binary, and keep the earlier lines under their own
  heading.
- `results/probe-window-trials.txt:1`, `:17-19` carry a kernel string and a
  `perf_event_paranoid=2` reading that belong to another host. The closeout
  labels them as that host's readings and leaves the text standing.
- The watchdog deduction applies once, on the decoded arm
  (`source/counters/linux_pmu/limit_fill.cpp:93`), and the search arm states that
  the counter the watchdog holds already stands missing from its answer. No
  second application stands written as a correction.

### Verify

Three consoles with their witnesses and limit lines; two parity rows, one measured
and one blocked with no figure; opens and reads labeled measured and derived; one
grep per correction.

---

## Plan: Counter work in the merged-head closeout (2026-11-08)

**Spec**: [spec.md](./spec.md), the section `Counter work in the merged-head
closeout (2026-11-08)`. `IP-001` and `IP-002` keep their text, and that section
states what each one asks of a run at the merged base. The shared rules stand in
[../048-hwlat-probe/spec.md](../048-hwlat-probe/spec.md) under `Intel
acceptance closeout at the merged head (2026-11-08)`: the ten lanes of the
final-head set under `The final-head set`, the two passes at ruling 29 of this
phase's ruling list, the load witness at ruling 30 of that list, the two bounded
widths in `IC-003`, and the file list under `Files this phase may change`. This
part writes the one record that list names: the results artifact of the
`specs/041-counter-probe` entry. Every citation in this section names its
file, then the id, the ruling number with its ruling list, the heading or the
lane-table row it points at, the way the citation rule of that spec section
requires, and every row names the id that owns it.

Ruling 29 of this phase's ruling list names two heads, so every figure row of
this part names the head Pass 2 runs at and the head the pull request carries,
and prints that ruling's markdown-only proof clause:
`git diff --name-only <head Pass 2 ran at> <head the pull request carries>`
prints files under `specs/` alone, which leaves every figure in place, and a diff
that names any other path voids the figures and forces a fresh Pass 2.

### The base this part reads

| Row | Reading | Owner |
|---|---|---|
| Base head | `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the merge of pull request #100, whose parent answers `6fba993858a2fff97f2e0c617152019bf7fe20fd`, the merge of pull request #99 (`git rev-parse a55728af^`, read 2026-11-09; `specs/048-hwlat-probe/spec.md` under `Intel acceptance closeout at the merged head (2026-11-08)`) | IC-001 |
| Branch and tree | `fix/intel-acceptance-closeout`, rebased onto that head (the `Branch` row of the `The base this work reads` table of `specs/041-counter-probe/spec.md`); every console runs in `build`, the tree the `Counter count` row of the lane table of `specs/048-hwlat-probe/plan.md` names | IC-001 |
| Final head | no figure; ruling 29 of this phase's ruling list names two heads, the head Pass 2 runs at, which the squash after the Pass 1 writes produces, and the head the pull request carries, which the squash after the Pass 2 writes produces. The record prints the short sha of the head Pass 2 ran at beside each console it ran, and names the head the pull request carries beside it (`Intel acceptance closeout at the merged head (2026-11-08)` of `specs/048-hwlat-probe/spec.md`, and that ruling) | IC-007 |
| What the merge moved for this surface | `git diff --name-only 2d4d8c57 a55728af` answers 12 paths, read 2026-11-09 in this checkout. None sits under `source/counters/` or under `example/`, so this surface reaches the base unchanged. | IC-006 |
| The two harness moves | `source/harness/execution_context.cpp` and `source/harness/detail/jitter_probe.hpp` stand among those paths (`specs/048-hwlat-probe/spec.md` under `Intel acceptance closeout at the merged head (2026-11-08)`); `git grep -n` over this part's record answers no match in either file, so this part re-reads no anchor there | IC-006 |
| This checkout beside the base head | `git diff --name-only a55728af HEAD` answers 16 paths and every one sits under `specs/` (read 2026-11-09), so a code line read here reads the same at the base head | IC-006 |
| Anchors this part cites | re-read on 2026-11-09: `source/counters/linux_pmu/limit_fill.cpp:72`, `:88-95`, `:96-110`, `:103`, `:104`; `example/docs/counter_probe.cpp:88`, `:196`, `:253`, `:283-298`, `:303`, `:329-338`, `:384-388`; `example/CMakeLists.txt:93` | IC-006 |
| Cite in the base text of this file | the closing paragraph of `D-3a: why the source label costs one exported symbol` cites `specs/046-hunter-topdown/spec.md` with a numeric line range for the accessor move that raised the counters archive `SOVERSION` from 5 to 6; that range sits in text the base head carries and keeps its text, and the handle that governs it reads `D-8 Public surface` under `Decisions` of `specs/046-hunter-topdown/spec.md` | IC-006 |
| Head labels that stay put | `615c0964`, `87d745d6` and `2d4d8c57` keep the label of the run that printed each figure, and each of them answers no row of this part (the head-label paragraph under `The base this work reads` of `specs/041-counter-probe/spec.md`, ruling 35 of this phase's ruling list of `specs/048-hwlat-probe/spec.md`) | IC-006 |

### The three consoles

Each console runs from the checkout root, the directory that holds `build`. In
this worktree that directory prints as `$HOME/code/sg042-closeout`, the
`$HOME`-relative form `IC-006` of `specs/048-hwlat-probe/spec.md` asks for. Each
console stands alone on an idle host (ruling 17 of the 2026-11-08 closeout list
of that file), and its witness line prints `cat /proc/loadavg` beside a count of
the other compiler, test, capture and trace processes on the host. Ruling 30 of
this phase's ruling list of `specs/048-hwlat-probe/spec.md` governs that witness
and its four process classes. The `Load witness and stops` sentences under `Order
of work` of `specs/048-hwlat-probe/plan.md` are text the base head carries, and
this section leaves them as the base wrote them. This part reads the cpu affinity
from those sentences and takes the process list from ruling 30. A figure whose
witness reports a live compiler, test, capture or trace process, or a one-minute
load average at or above `2.0`, is void and gets re-taken alone (ruling 30 of that
list).

| Console | Working directory | Command | What the record answers | Figure | Owner |
|---|---|---|---|---|---|
| 1 | `$HOME/code/sg042-closeout` | `./build/example/docs_counter_probe` | the exit code, the limit line, the benchmark line, the witness line | no figure | IP-001 |
| 2 | `$HOME/code/sg042-closeout` | `./build/example/docs_counter_probe` | the same four, beside console 1 | no figure | IP-001 |
| 3 | `$HOME/code/sg042-closeout` | `./build/example/docs_counter_probe` | the same four, beside consoles 1 and 2 | no figure | IP-001 |

The reading line of the same pass carries the host facts IP-001 names, each one
with the command that reads it:

| Reading | Command | Figure | Owner |
|---|---|---|---|
| `pmu_name` | `cat /sys/devices/cpu/caps/pmu_name` | no figure | IP-001 |
| Device `type` | `cat /sys/bus/event_source/devices/cpu/type` | no figure | IP-001 |
| `perf_event_paranoid` | `cat /proc/sys/kernel/perf_event_paranoid` | no figure | IP-001, IP-002 |
| `nmi_watchdog` | `cat /proc/sys/kernel/nmi_watchdog` | no figure | IP-001 |

The tool surface stays `counter_probe`. The commands above answer every row,
`probe_window` takes no raw event word and answers no row here, and this part
adds no tool (ruling 14 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md`).

### The parity repeat

One command measures the parity repeat: `./build/example/docs_counter_probe`
from `$HOME/code/sg042-closeout`. The single limit line carries both figures:
the published limit with its source word, and the example's own search figure at
`example/docs/counter_probe.cpp:384-388`. The repeat takes its figures from the
three consoles above (the parity bullet under `What repeats at the final head` of
`specs/041-counter-probe/spec.md`), and the comparison names the permissions that
host answers at the final head (IP-001).

| Row | Claim | How it closes | Owner |
|---|---|---|---|
| Parity at the permissions of that head | the decoded figure and the searched figure agree | the three consoles, with the reading line beside them for the permissions of that run | IP-001; SC-003 owns the claim |
| Parity with the control at `2` | the reading a restricted historical environment asks for | BLOCKED, with no figure, and the row names the precondition in the `Audit point` list of `specs/041-counter-probe/spec.md` and the `Session 2026-11-05, round 1` answer of that spec on the `pid` and `cpu` the group binds to, because no `SC-` id owns that claim (ruling 16 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md`) | IP-002 |
| The watchdog rule | one application, on the decoded arm at `source/counters/linux_pmu/limit_fill.cpp:93` | the search arm at `:96-110` holds `probeEventNameForDevice` at `:103` and `searchedLimit` at `:104` and holds no deduction, so no second application stands written (the watchdog bullet under `What repeats at the final head` of `specs/041-counter-probe/spec.md`) | IP-001 |
| Opens and reads | two numbers, each labeled measured or derived | the decoded arm and the example's own walk, printed beside the code lines that ruling 13 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md` names | IP-001 |

Neither parity row borrows the other figure (ruling 11 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md`).

### The two readings of one control

The control is `/proc/sys/kernel/perf_event_paranoid`. It carries two readings,
and each reading belongs to one row.

| Reading | What it is | How it enters a row | Owner |
|---|---|---|---|
| `-1` | the value this host carries, and the value the measured parity row stands on | the final-head value comes from `cat /proc/sys/kernel/perf_event_paranoid` in the same pass; the `-1` at `results/acceptance-final-intel.md:124` sits inside the section that opens at `:100` and names no head, so it answers no row of this part (the paragraph under `Rows the final head must answer` of `specs/041-counter-probe/spec.md`) | IP-002; ruling 35 of this phase's ruling list of `specs/048-hwlat-probe/spec.md` |
| `2` | the value the restricted historical environment asks for | the BLOCKED row, which carries no figure at all | IP-002 |
| Set `2` | `sysctl -w kernel.perf_event_paranoid=2` | the BLOCKED row prints this line as the operator action | IP-002; ruling 37 of this phase's ruling list of `specs/048-hwlat-probe/spec.md` |
| Restore the host | `sysctl -w kernel.perf_event_paranoid=-1` | the BLOCKED row prints this line beside it and names the value the host answered before the change | IP-002; ruling 37 of this phase's ruling list of `specs/048-hwlat-probe/spec.md` |

No session in this lane can run either line. `sudo -n -l` answers `sudo: a
password is required` at exit 1 (`results/acceptance-final-intel.md:18`, `:147`),
and `pkexec` needs an interactive administrator authorization, because
`org.freedesktop.policykit.exec` defaults to `auth_admin` at
`/usr/share/polkit-1/actions/org.freedesktop.policykit.policy:66-70`
(`IP-002` of `specs/041-counter-probe/spec.md`). Ruling 37 of this phase's ruling
list of `specs/048-hwlat-probe/spec.md` keeps the row BLOCKED, bars any elevation
here, and bars any weakening of a kernel control. The standing blocked row at
`results/acceptance-final-intel.md:146`
names the control, the action and the missing right, and it prints no command
line, so the dated row adds the two lines above
(`IP-002` of `specs/041-counter-probe/spec.md`).

### The interpreter the contract target runs

Two places declare an interpreter and its library version, and both serve the
hosted lane (`The interpreter the contract target runs` of
`specs/041-counter-probe/spec.md`).

| Declaration | File and range | What it declares | Owner |
|---|---|---|---|
| Hosted job step | `.github/workflows/ci.yml:1265-1267` | the `dbc-gate` job installs `doxygen` and `python3-yaml` with the package manager | IC-005 |
| Gate script guard | `tools/dbc/dbc_gate_common.py:63-70` | an interpreter with no `yaml` module answers exit 2 with `dbc gates require PyYAML (install python3-yaml)` | IC-005 |

Both ranges re-read in this checkout on 2026-11-09. The target runs its two gate
scripts with the interpreter the tree configured, and `cmake/dbc-gate.cmake:44`
and `:49` name that interpreter. No manifest declares an interpreter for this
host, and this host carries two of them, one with no PyYAML
(`specs/048-hwlat-probe/results/acceptance-final-intel.md:217-218`,
`:694-695`).

| Row | Command | What the record prints | Owner |
|---|---|---|---|
| Configured interpreter | `grep '^Python3_EXECUTABLE' build/dev/CMakeCache.txt` | the path that gate tree configured, in `$HOME`-relative form (ruling 27 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md`); this checkout answers `/usr/bin/python3` today, and `specs/048-hwlat-probe/results/acceptance-final-intel.md:1158-1160` records a cache read that printed no such line for another tree, so the row keeps CMake's own configure line beside it | IC-005 |
| Library version | that interpreter with `-c 'import yaml; print(yaml.__version__)'` | the version that interpreter answers, beside the interpreter version; where no version answers, the row reads BLOCKED with that interpreter named (`The interpreter the contract target runs` of `specs/041-counter-probe/spec.md`) | IC-005 |
| Target exit | `cmake --build build/dev -t dbc-gate -j "$(nproc)"` | the target's own exit, beside the two unchanged manual gate readings (the `DBC gates` row of the lane table of `specs/048-hwlat-probe/plan.md`, whose block 4 holds those commands) | IC-005 |

The target stays unchanged, the two manual commands stay unchanged, and this part
edits no CI file (`IC-005` of `specs/048-hwlat-probe/spec.md`). The contract-gate
rows themselves stand in `specs/048-hwlat-probe/results/gates.md`, outside the
file list, and this part cites them by reference and rewrites none
(`Files this phase may change` of `specs/048-hwlat-probe/spec.md`, IC-006).

### Coverage stays another lane's row

| Row | Statement | Owner |
|---|---|---|
| Scope | the coverage chain is one lane of the final-head set (`The final-head set` of `specs/048-hwlat-probe/spec.md`), its rows live under `specs/048-hwlat-probe/results/`, and this part writes no coverage row into this lane's record | IC-002 |
| Two denominators | the denominator this host's own capture prints and the hosted denominator are two different totals, and no row compares them (ruling 31 of this phase's ruling list of `specs/048-hwlat-probe/spec.md`) | SC-IC-8 |
| One side answers nothing about the other | a local reading answers nothing about a hosted reading, and the hosted shortfall stands deferred with its reproduction (`Out of scope for this phase` of `specs/048-hwlat-probe/spec.md`) | IC-002 |

### The pending hardware rows

This base moves none of them (`Boundaries this base keeps` of
`specs/041-counter-probe/spec.md`).

| Row | Status at this base | Owner |
|---|---|---|
| Arm `armv8_*` | pending; no Arm host stands in reach | SC-010; IA41-002 |
| Intel hybrid with leaf 0x23 answered | pending; the device-shape limit needs a host whose threads reach the device the row asks about | SC-010 |
| AMD Zen 4 | pending; this host is a non-hybrid Intel part | SC-010; IA41-002 |
| Intel non-hybrid check-mode equality | no figure; the three consoles and the parity reading above answer it at the final head, and no reading stands closed at this writing (`SC-010` of `specs/041-counter-probe/spec.md`) | SC-010; IP-001 |
| No row moves on another host's reading | a completed row this host measured contradicts none of the three pending rows | IA41-002 |

### What closes this part

| Row | Word that closes it | Owner |
|---|---|---|
| Every console row and every reading row | carries a command, an exit code and a count a reader can rerun, or reads BLOCKED with the right it lacks | SC-IC-7 |
| One sha | the record, the pull request body and the remote branch agree on the head the pull request carries (`specs/048-hwlat-probe/spec.md`, ruling 40 of this phase's ruling list and `SC-IC-11` of that file) | ruling 40; SC-IC-11 |
| Text gates over this file | `cmake -P cmake/spell.cmake` and `cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake` each exit 0 from the checkout root (the `Lint`, `Prose, tree` and `Prose, range` rows of the lane table of `specs/048-hwlat-probe/plan.md`); the tree prose gate also runs directly as `/usr/bin/python3 tools/prose/prose_gate.py --check prose --mode tree --rules tools/prose/prose_rules.yaml` | IC-006 |
| This file | keeps every line it carried before this section, and this section adds | IC-006 |
| The two bounded widths | `taskset -c 0,1,2,3` with `-j 4` and `taskset -c 0,1` with `-j 2` belong to the `IC-003` triple, where the narrow shape stands beside the bounded run, and to the `Two-cpu release` row and the `Four-cpu release` row of the `Lane Table` of `specs/048-hwlat-probe/plan.md`, so the two sanitizer trees and the two release trees run both shapes and both of those rows name `build`, and no counter console runs either | IC-003 |
| Plan file beside the record | `specs/041-counter-probe/plan.md` stands inside the file list under `Files this phase may change` of `specs/048-hwlat-probe/spec.md`, whose seventh entry names the plan among the markdown artifacts of `specs/041-counter-probe`, so this section plans and carries no acceptance figure, and only `specs/041-counter-probe/results/acceptance-final-intel.md` takes acceptance rows | IC-006 |


## Plan: Zen 4 acceptance on chilly (2026-11-10) - the counter-count part

The spec section `Zen 4 acceptance phase: the AMD Zen 4 host chilly
(2026-11-10)` owns the requirements. The shared modules M0, M1, M2, M4, M7,
M8, M9, M10, the load witness, and the order stand in
`specs/046-hunter-topdown/plan.md`, section `Zen 4 acceptance plan on chilly
(2026-11-10) - the hunter part and the shared plan`. This section plans the
counter part and carries no acceptance figure.

### Modules of this part

| Module | Work | Needs | Produces |
|---|---|---|---|
| M3 counter runs | `./build/example/docs_counter_probe` three times, each run alone with its load witness, console captured by redirection; then the device reads: the device list, the `cpu` device `type` with its mode line, the standing absence of `caps/pmu_name`, `perf_event_paranoid`, `nmi_watchdog`, the two elevation readings. The witness void rule and the parity non-borrowing rule of the 046 plan apply to these consoles | M1, the quiet window after M2 | `$HOME/z4/p1/probe-<n>.log` (3), `$HOME/z4/p1/devices.log` |
| M6 record 041 | write `specs/041-counter-probe/results/acceptance-zen4.md`: the identity block, the device and settings table, the three consoles, the parity row at the host `paranoid` value, the opens and reads row with its arithmetic and cites, the watchdog row, the blocked parity row at `-1` in the IP-002 shape, the pending rows, and the dated measured row; tick the tasks | M3, M4 (suite figures), M5 (shared capture conventions) | the record, the ticks |

M3 runs after the hunter window closes, so no hunt process shares the host.
M6 runs parallel with M5.

### The parity reading, and what is measurable

This host answers `perf_event_paranoid` at 2. The parity of the published
limit against the probe answer is a measured row at that value, and the
example needs no elevation for it. The parity at `-1` needs the write, which
needs a root session; the two elevation readings of the identity step close
that door, so the `-1` row stands BLOCKED with the control, the value the
host answers, the set line, the restore line, and the missing right (IP-002).
No control is weakened.

### Opens and reads

The example walk answers the limit it prints. Where no tracer runs, the opens
and reads stand derived: the walk of `example/docs/counter_probe.cpp` pays
one open per copy of each size and one read of the group leader per size that
opens whole, and the short size pays its opens and no read. The record prints
the arithmetic beside the answer of the consoles, cites the lines of the
example that hold the walk and the cap, and states that the figure is derived
(standing IP-001 shape).

### Watchdog, once

The decoded arm applies the watchdog rule once. The record cites the decoded
arm of `source/counters/linux_pmu/limit_fill.cpp` and the nmi gate of
`source/counters/linux_pmu/counter_limits.cpp`, prints this host's
`nmi_watchdog` reading, and states the deduction that reading decides. No
second application stands anywhere in the record.

### What this part leaves untouched

No raw event code (G-31), no raw configuration word, no new tool, no new
option, no `.expected` compare claim, no source line, and no standing row of
the Zen 5 or Intel records. `tools/group_probe.c` answers no row.
