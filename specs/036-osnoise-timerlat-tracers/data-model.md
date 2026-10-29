# Data Model: Osnoise and Timerlat Tracer Integration

Phase 1 output of `/speckit.plan`. The entities below extend the
standing pre-check data model (`specs/034-host-noise-precheck/data-model.md`);
nothing there changes shape. Each entity names its fields, its
validation rules and its owner.

## Entity map

```text
TracerEngine (new, public enum)
  └─ NONE | OSNOISE | TIMERLAT
JitterProbeOptions (public, +2 fields)
  ├─ tracerEngine: TracerEngine
  └─ tracerCpus: vector<uint32_t>      (empty names the candidates)
PrecheckRecord (public, +1 optional member)
  └─ tracer: optional<PrecheckTracerRecord>
PrecheckTracerRecord (new, public)
  ├─ engine: string                    (the word the value named)
  └─ cpus: vector<PrecheckTracerCpuRecord>, ascending
PrecheckTracerCpuRecord (new, public)
  ├─ cpu: uint32_t
  └─ nmi, irq, softirq, thread, hardware: optional<uint64_t> each
NoiseTracerSession (new, internal)
  ├─ engine, cpus
  ├─ previousTracer: the standing current_tracer word
  └─ previousCpuMask: the standing tracing_cpumask text
The option row and its value constant (detail)
  └─ kOsNoiseOptionValue at 297, the block 298 to 319 free
The tracer JSON object (the schema governs)
  └─ inside the standing precheck object, after skippedGroups
```

## E-16: `TracerEngine` and the option fields

The engine the `--os-noise` value names, and the cpus the tracer runs
on. The enum closes the engine set at the type level; `NONE` names no
tracer, and a run whose value stands absent carries `NONE`.

- Fields: `tracerEngine` (the closed enum), `tracerCpus` (the cpu
  list; an empty vector names the pre-check's candidate set, spec
  FR-008).
- Validation: the parser accepts the engine word `osnoise` or
  `timerlat` alone, then at most one `cpus` key; the key's value
  stands read whole by the standing cpu-list reader, so every cpu
  stands inside the process affinity mask and the online set, and no
  cpu stands twice (spec FR-002, FR-003). A planned cpu named in the
  list stands no refusal (spec FR-008, the clarify answer).
- Owner: the public struct (`include/speedgun-ng/benchmark.hpp`); the
  parser (`source/harness/noise_tracer.cpp`), declared in
  `source/harness/detail/noise_tracer.hpp`.

## E-17: `PrecheckTracerRecord` and `PrecheckTracerCpuRecord`

The tracer record one run carries: the engine word, and one entry per
tracer cpu with the five figures. The entry names its cpu, so the
cpus the tracer ran on stand the array itself (spec FR-014, D-3).

- Fields: `engine` (the word `osnoise` or `timerlat`, the enum word
  the value named); `cpus`, ascending, each entry the cpu index and
  the five figures `nmi`, `irq`, `softirq`, `thread`, `hardware`,
  each `std::optional<std::uint64_t>`.
- Validation: each figure stands at or above 0; an absent figure
  stands the source the engine states no figure for, the file form of
  the console's `not available` (spec FR-010); a cpu whose files
  state nothing at read time stands the entry with every figure
  absent (spec Edge Cases); the entry count stands the tracer cpu
  count, one per cpu of the session, ascending.
- Lifecycle: stands on the record when the tracer ran; a run that
  requests no tracer and a run whose tracer stands not available
  leave the member empty (spec FR-015, R-12 of research.md).
- Owner: the public struct; the reader and writer of the report
  (`source/harness/json_load.cpp`, `source/harness/json_report.cpp`).

## E-18: `NoiseTracerSession`

The engine state one run owns across the pre-check window, held by
the entry-point thread from the setup to the restore.

- Fields: `engine` and `cpus` (the run's own choice), the standing
  `current_tracer` word and the standing `tracing_cpumask` text the
  setup read for the restore.
- Validation: the setup answers an empty optional when any write
  fails (the not-available shape, spec FR-016, D-5); the session
  stands live only across the window, and the restore stands the
  last touch the run makes (R-5 of research.md).
- Owner: `source/harness/detail/noise_tracer.hpp` (the type) and
  `source/harness/noise_tracer.cpp` (the functions).

## E-19: the option row and the value constant

One row joins the standing option table (`source/harness/options.cpp`,
the `optionRows` array), and the value constant joins the block
comment of `source/harness/detail/options.hpp`.

- Fields: the row `{name: "os-noise", value: 297, argument: REQUIRED,
  fromEnvironment: false, help, owner: "specs/036-osnoise-timerlat-tracers"}`;
  the constant `kOsNoiseOptionValue` at `297`.
- Validation: no standing name moves and no standing value moves; the
  name and value uniqueness tests carry the row (spec FR-001,
  `test/source/harness_cli_table_test.cpp`); the block comment names
  `298` to `319` free after the row joins.
- Owner: the option unit; the grammar and the error surface stand in
  [contracts/cli-options.md](./contracts/cli-options.md).

## E-20: the tracer JSON object

The optional member of the standing `precheck` object, after
`skippedGroups`. The key sets and their order stand in
`docs/schema/report-v1.json` and in
[contracts/report-json.md](./contracts/report-json.md).

- The `precheck` object's key set with the moved `x-order`:

```json
["serial", "windowMs", "cpus", "hostSettings", "skippedGroups", "tracer"]
```

  The `required` list of the object stands unchanged: the `tracer`
  key joins `properties` and `x-order` and stands outside `required`,
  the whole optionality mechanics the standing checker reads (R-11 of
  research.md). The top-level `x-order` stands unchanged (spec
  FR-006), and the schema minor stands `1.7`.
- The `tracer` object:

```json
{
  "x-order": ["engine", "cpus"],
  "required": ["engine", "cpus"],
  "additionalProperties": false,
  "engine": {"type": "string", "enum": ["osnoise", "timerlat"]},
  "cpus": {"type": "array"}
}
```

- One `cpus` entry:

```json
{
  "x-order": ["cpu", "nmi", "irq", "softirq", "thread", "hardware"],
  "required": ["cpu", "nmi", "irq", "softirq", "thread", "hardware"],
  "additionalProperties": false,
  "cpu": {"type": "integer"},
  "nmi": {"type": ["integer", "null"]},
  "irq": {"type": ["integer", "null"]},
  "softirq": {"type": ["integer", "null"]},
  "thread": {"type": ["integer", "null"]},
  "hardware": {"type": ["integer", "null"]}
}
```

- Validation: the harness checker reads every written document against
  the key set, the key order and the value kinds, the standing rule of
  specs/023; `loadReport` reads the object back through the standing
  `optionalField` path, the way `hunt` reads (spec FR-014).
- Lifecycle: written once per run whose tracer ran; a run that
  requests no tracer and a run whose tracer stands not available
  write no key (spec FR-015).

## State transitions

```text
run start
  └─ --os-noise absent ........ no tracer, no member, no key, no lines
  └─ --os-noise settled
       └─ the batch carries a pre-check window?
            ├─ no ....... one refusal line, exit status 2, before anything
            └─ yes
                 └─ the setup writes succeed?
                      ├─ no ... the tracer lines stand not available,
                      │        no member, no key, exit status 0
                      └─ yes
                           ├─ the pre-check window(s) run beside the
                           │  started tracer (every window in serial mode)
                           ├─ the figures stand read once after the window
                           ├─ the restore returns the standing values
                           └─ the member stands, the key stands,
                              the lines stand the figures
```

## What is deliberately absent

- No tuning key: a threshold, a window length or a trace-buffer shape
  stands a follow-up with its own grammar record (spec Out of scope).
- No trace-line parse: the harness reads the count files alone (spec
  Out of scope).
- No reset write between serial windows: the figures stand the
  engine's cumulative totals (spec FR-009).
- No second timing window and no new probe context (spec FR-009).
- No CCD mapping in the record: the cpus stand alone, and the CCD
  mapping stands in the acceptance evidence (spec FR-014, D-7).
- No version check: the file probe answers the only question the run
  asks (R-4 of research.md).
- No root requirement: the feature never refuses a host that runs
  without root and never asks for rights (spec Lane identity).
