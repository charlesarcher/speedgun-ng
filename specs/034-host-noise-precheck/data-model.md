# Data Model: Host Noise Pre-Check

Phase 1 output of `/speckit.plan`. The entities below extend the
standing probe data model (`specs/027-jitter-probe/data-model.md`);
nothing there changes shape. Each entity names its fields, its
validation rules and its owner.

## Entity map

```text
JitterProbeOptions (public, +1 field)
  └─ precheckSerial: bool
ProbeSession (standing) ─ drives ─┐
PrecheckCpuState (new, internal)  │ one per measured cpu
  ├─ role: planned | candidate
  ├─ stats: JitterStats (standing fold)
  └─ attribution: AttributionDelta
AttributionDelta (new, internal)
  ├─ interruptsDelta: optional<uint64>
  ├─ softirqs: per kind, optional<uint64>
  ├─ switchDelta: optional<uint64>      (the schedstat delay figure)
  └─ scheduleDelta: optional<uint64>    (the schedstat timeslices figure)
PrecheckHostSettings (new, internal, one per run)
  ├─ kernel settings: isolcpus, nohz_full, rcu_nocbs, each optional
  ├─ governor per measured cpu: optional<string>
  └─ irq affinity per measured cpu: list of IRQ numbers
PrecheckRecord (new, JSON) ── the document the schema governs
```

## E-11: `PrecheckCpuState`

One measured cpu of the pre-check phase, planned or candidate. The
standing `ProbeCpuState` carries the buffer and the folds unchanged;
the pre-check adds the role and the attribution beside them.

- Fields: the cpu index, the role, the `JitterStats` fold of the
  window, the `AttributionDelta`, and one availability flag per
  attribution source.
- Validation: the cpu index stands inside the process affinity mask;
  the role stands one of the two values; a source that could not
  stand read or parsed stands `not available`, never a zero
  (spec FR-018).
- Owner: the probe unit (`source/harness/jitter_probe.cpp`), the
  type in `source/harness/detail/jitter_probe.hpp`.

## E-12: `AttributionDelta`

The window delta of one cpu's kernel counters, each figure optional.

- Fields: `interruptsDelta` (the sum of the cpu's column over the
  rows of `/proc/interrupts`), one softirq delta per kind the file
  states, `switchDelta` (the `/proc/schedstat` run-queue delay
  figure) and `scheduleDelta` (the timeslices figure).
- Validation: each delta stands at or above 0; a figure the host
  does not state, states unreadable, or states unparsable stands
  `not available` for that figure alone (R-3 to R-5 of research.md);
  no delta stands negative under a valid parse.
- Owner: the attribution reader of the probe unit.

## E-13: `PrecheckHostSettings`

The host facts the run discloses once.

- Fields: the kernel command line's `isolcpus`, `nohz_full` and
  `rcu_nocbs` values, each optional; the cpufreq governor of each
  measured cpu, optional per cpu; the IRQ numbers whose affinity
  names each measured cpu.
- Validation: a setting the command line does not name stands
  `not available`; a governor file that is missing, unreadable or
  unparsable stands `not available` for that cpu alone; the IRQ
  reader accepts the list form and falls back to the mask form
  (research R-6).
- Owner: the host-settings reader of the probe unit.

## E-14: the `precheck` JSON record

The top-level document object, after `jitter` and before
`benchmarks` (research R-9). The keys and their order stand in
`docs/schema/report-v1.json` and in [contracts/report-json.md](./contracts/report-json.md).

- Fields: the mode word, the window length, the serial flag, the
  measured cpu list with each cpu's role, per-cpu gap stats (the
  sixteen standing field names), the per-cpu attribution deltas with
  their sources, the host settings, and the disclosure text the
  console block prints.
- Validation: the harness checker reads the document against the
  schema's key set, key order and value kinds, the standing rule of
  specs/023; `loadReport` reads the record back (spec FR-024).
- Lifecycle: written once per run that carried the pre-check; a run
  without the pre-check writes no key.

## E-15: the pre-check console block

The console surface (see [contracts/console-report.md](./contracts/console-report.md)):
one block per measured cpu, the host settings once, the warning
lines, the pinning line, the strict refusal line. No entity stands
public; the block is printed text the report writer owns
(`source/harness/report.cpp`).

## State transitions

```text
run start
  └─ mode?
       ├─ no --jitter-probe ........ no pre-check, no record, no key
       ├─ mode=alongside ........... no pre-check (standing phase)
       └─ precheck | both
            └─ resolve the measured set (planned + candidates)
                 └─ serial?
                      ├─ no .. one window, every cpu spinning
                      └─ yes . one window per cpu, ascending,
                               planned first
                           └─ strict and a noisy planned cpu?
                                ├─ no .. latch, benchmarks run, status 0
                                └─ yes . one refusal line, status 2,
                                         before any benchmark
```

The attribution snapshots stand read before the first window opens
and after the last window closes, in both shapes.

## What is deliberately absent

- No public result type: the console block and the JSON record carry
  the results (research R-1, spec X.2).
- No persistence: the attribution snapshots live for the run alone.
- No retry, no second window, no re-measurement: the phase runs once
  per run, the way the standing probe runs once per batch.
- No new option row, no new mode value, no new key in the value
  grammar: the `serial` word beside the standing millisecond form is
  the whole surface (spec FR-001, FR-002).
