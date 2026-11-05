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
