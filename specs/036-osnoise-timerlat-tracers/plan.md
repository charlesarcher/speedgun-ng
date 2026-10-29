# Implementation Plan: Osnoise and Timerlat Tracer Integration

**Branch**: `036-osnoise-timerlat-tracers` | **Date**: 2026-10-29 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/036-osnoise-timerlat-tracers/spec.md`

## Summary

The pre-check gains the kernel's own noise tracers. One option row,
`--os-noise`, takes the engine word `osnoise` or `timerlat` and one
optional `cpus=` key the standing cpu-list reader parses. The run sets
the engine through the tracefs root the `proc_root` seam of spec 034
resolves (the standing `/sys/kernel/tracing` path), restricts it to the
tracer cpus (the `cpus=` list, else the pre-check's candidates), runs it
across the pre-check window, and reads the five noise-by-source counts
(NMI, IRQ, softirq, thread, hardware) once after the measurement ends.
The figures join the standing pre-check block beside the gap figures,
console and JSON, with the engine name and the cpus disclosed. A host
without tracefs, without the engine, or without the rights the setup
writes need prints the standing `not available` disclosure on the tracer
lines alone and exits 0. The JSON record stands optional inside the
`precheck` object at schema minor `1.7`. The public surface grows by the
`TracerEngine` enum, two fields on `JitterProbeOptions`, and the
optional `PrecheckTracerRecord` on `PrecheckRecord`; the harness
`SOVERSION` rises from `13` to `14` with the version move to `0.23.0`.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language)

**Primary Dependencies**: the standing pre-check unit and its seams: the
file-root seam (`source/harness/detail/proc_root.hpp`), the cpu-list
reader (`parseProbeCpuToken`, `source/harness/jitter_probe.cpp`), the
measured-set resolver (`precheckMeasuredSet`, same file), the option
gather's settle arm (the `--jitter-probe` case of
`source/harness/options.cpp`), the entry-point wiring
(`source/harness/cli.cpp`, the `batchProbed` block), the console printer
(`printPrecheckReport`, `source/harness/report.cpp:986`), the report
writer (`source/harness/json_report.cpp`), the simdjson reader and
schema checker (`source/harness/json_load.cpp`), and the schema
(`docs/schema/report-v1.json`). `std::filesystem` and the standard
streams carry every file access, so no new dependency joins (spec D-1).

**Storage**: N/A: no file and no database of the feature's own. The
tracer state lives in the tracefs files the host owns; the run reads the
standing values, writes its own, and restores the standing values after
the read. The one document the feature writes is the report file
`--json` names.

**Testing**: the repository's own harness (one executable per area plus
`add_test`): two new executables, `harness_noise_test` (the grammar, the
refusals, the engine session, the integration) and
`harness_noise_report_test` (the console lines, the JSON round trip, the
schema, the byte identity); the option-table rows extend
`harness_cli_table_test.cpp`. Fake tracefs trees stand written under a
temporary directory and installed through the file-root seam; the
pre-check window stays bounded by the scripted `WindowReader` through
`installTestReader`, so no test sleeps and no test reads a clock (spec
FR-019). Tests run with `ctest -j 2` and pass under `taskset -c 0,1`
and on a 2-cpu host (spec FR-018).

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily
loop drives `ci-rocky`; once per feature, `ci-ubuntu` builds with
clang-tidy and cppcheck wired in. CI needs no kernel tracer and no root
(spec FR-017): every tracefs read in the suite resolves fake files. The
acceptance shape stands on the request's lane host thrawn
(Threadripper 9960X, 24 cores, 48 threads, several CCDs with one L3
each), one probe cpu per L3 group (spec D-7).

**Project Type**: library feature: the public surface grows inside the
standing types, one detail header and one source file join, and the
option table gains one row. No new target, no new library.

**Performance Goals**: the engine setup, the figure read and the restore
stand outside every timed window, around the pre-check window the
standing grammar owns (spec VII note); the tracer adds no work inside
any timed loop; no benchmark baseline is claimed, so none is promised.

**Constraints**: every time value flows through `sg::counters` (the
time-source gate stays green; the feature reads counts and states no
clock); 100% line and branch coverage with no `LCOV_EXCL` marker and no
new `--ignore-errors` entry; no sleep and no clock in a test; one line
and exit status 2 for every bad value and for the missing pre-check
window; no fixed cpu number and no fixed L3 group number anywhere (spec
FR-018); no lambda inside an `SG_*` macro; `[[maybe_unused]]` on
contract-only helpers; simdjson stands the only JSON reader (spec
FR-026); the docs gates bind the README section and the example.

**Scale/Scope**: one detail header (`source/harness/detail/noise_tracer.hpp`)
and one source file (`source/harness/noise_tracer.cpp`), the public
types extended, the option row and its value parser, the pre-check
wiring in `source/harness/cli.cpp`, the console printer and the JSON
writer and reader extended, one schema extension, two new test
executables, one example section re-recorded, the docs rows, and the
results records of spec FR-020.

**Execution mode**: TDD (Principle III). Every test stands written and
observed failing before the code that turns it green exists; the Test
Plan records the waves, the covering tests and the gates each runs.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 surface. The tracefs access uses `std::filesystem` and the standard streams; the counts are integers the standard library parses; no compiler extension joins | PASS |
| II. Design by contract | The new public enum, fields and record types carry doxygen contracts on the struct; the engine session functions carry `\pre`/`\post`; the value parser, the figure reader and the JSON loader carry `SG_REQUIRE`/`SG_ENSURE` at their seams (the loader follows the standing `json_load.cpp` shape) | PASS |
| III. R-DCUT | Requirements FR-001 to FR-026 map to the decisions R-01 to R-14 below and to `tasks.md` (landing with `/speckit.tasks`). Execution mode: TDD, recorded above and in the Test Plan | PASS |
| IV. Documentation | README tracer section of at most 25 lines with its quick-reference rows (IV.1, spec FR-021), `docs/pages/harness.md` option and field detail (spec FR-022), the example and its `.expected` re-recorded (spec FR-023) | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the feature head; every new name follows V.1: PascalCase types, camelBack functions, `k`-prefixed constants, lower_case file names (R-12) | PASS |
| VI. Coverage | 100% line and branch on the new paths, no exclusion marker; every branch of the grammar, the availability probe, the setup, the figure read, the restore, the disclosure paths, the JSON write and read runs under the fake tracefs files and the scripted reader (spec FR-025) | PASS |
| VII. Performance | The setup, the figure read and the restore stand outside every timed window; the tracer runs across the pre-check window the standing grammar owns; no timed loop changes shape | PASS |
| VIII. CI gates | Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, tsan, coverage, name-check, prose, time-source, DBC, the schema shape check over every written document, and the docs gates (page drift, snippet compile, example run, report shape) | PASS |
| IX. Spec-driven | Spec, plan, research, data model, contracts and quickstart stand in `specs/036-osnoise-timerlat-tracers/`; `tasks.md` lands with `/speckit.tasks` before code | PASS |
| X. Anti-slop | The tracefs access rides the one standing file-root seam (R-01); the tracer cpus default to the candidate set the measured-set resolver already answers (R-09); the public surface grows by one enum, two fields and one optional record; the disclosure is the one fallback the spec asks for; no mount-point search, no tuning key, no trace-line parse (spec Out of scope) | PASS |
| XI. Prose | The gate walks `git ls-files`, so at this point it clears the tracked tree only, while these artifacts stand untracked and await the run that lands with the branch's first commit, the way specs/025 and specs/027 recorded | PASS for the tracked tree, pending for these artifacts |

**Gate weakening requested**: no. **Gate strengthening requested**:
no. No budget figure moves: the README section lands inside the
standing 25-line section cap, and the schema minor moves to `1.7` on
the standing key-growth rule of `specs/023`.

No gate fails, so **Complexity Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/036-osnoise-timerlat-tracers/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cli-options.md   # The value grammar, the error surface, the
│   │                    #   option-table row delta over specs/015
│   ├── console-report.md# The tracer lines delta over specs/034
│   └── report-json.md   # The tracer keys delta over specs/034
├── results/             # The acceptance records of spec FR-020; the
│                        #   tasks phase fills the records and the
│                        #   quickstart links them
└── tasks.md             # Phase 2 output (/speckit.tasks - NOT created here)
```

The standing contracts the spec names stand amended by the deltas
above: the option-table row joins
`specs/015-benchmark-harness-core/contracts/cli.md`, the tracer lines
join `specs/034-host-noise-precheck/contracts/console-report.md`, the
tracer keys join
`specs/034-host-noise-precheck/contracts/report-json.md`, and the
tracer fold rows join
`specs/034-host-noise-precheck/contracts/report-shape.md` in the commit
that lands the code (spec FR-024). This directory carries the deltas,
so the standing files stand untouched until that commit.

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp         # + TracerEngine, tracerEngine and
                                          #   tracerCpus on JitterProbeOptions,
                                          #   PrecheckTracerRecord and
                                          #   PrecheckTracerCpuRecord,
                                          #   PrecheckRecord::tracer
CMakeLists.txt                            # + version 0.23.0; harness SOVERSION 14;
                                          #   archive SOVERSION stays 4
source/harness/detail/options.hpp         # + kOsNoiseOptionValue at 297, the
                                          #   free-block comment amended
source/harness/options.cpp                # + the --os-noise row and the
                                          #   settle arm case
source/harness/detail/noise_tracer.hpp    # new: the value parser, the engine
                                          #   session, the figure read
source/harness/noise_tracer.cpp           # new: the implementations
source/harness/cli.cpp                    # + the missing-window refusal, the
                                          #   engine lifecycle around the
                                          #   pre-check window, the record build
source/harness/report.cpp                 # + the tracer lines in
                                          #   printPrecheckReport
source/harness/json_report.cpp            # + the tracer object; kSchemaVersion
                                          #   at 1.7
source/harness/json_load.cpp              # + the tracer reader (optionalField)
docs/schema/report-v1.json                # + the tracer key, the precheck
                                          #   x-order move, schema 1.7
docs/pages/harness.md                     # + the option and field detail
README.md                                 # + the tracer section (25-line cap)
example/docs/jitter_probe.cpp             # + the tracer section
example/docs/jitter_probe.expected        # re-recorded
tools/docs/example_drift.py               # + the section rows
tools/docs/report_shape.py                # + the tracer fold rows
test/source/harness_noise_test.cpp        # new: grammar, refusals, engine,
                                          #   integration
test/source/harness_noise_report_test.cpp # new: console, JSON, schema, bytes
test/source/harness_cli_table_test.cpp    # + the row uniqueness rows
test/CMakeLists.txt                       # + the two executables
specs/036-osnoise-timerlat-tracers/results/ # + the records of FR-020
```

**Structure Decision**: the feature extends the standing pre-check unit
and its seams; the two new files carry the tracer engine and its value
parser, the one place the tracefs access lives. No new target, no
second seam, no mount-point search (spec D-1, D-7).

## Design

### Decision index

- **R-01 The tracefs access rides the standing seam (spec FR-007, D-1).**
  Every tracefs read and write resolves
  `procRootPath("sys/kernel/tracing/<file>")`: the standing file root of
  specs/034 with the relative path the tracing tree carries under `/`.
  The tests install a fake tree with `installTestProcRoot`, so the suite
  needs no kernel tracer and no root (spec FR-017). No second seam, no
  mount-point search, no dependency.
- **R-02 The engine selection and the control files (spec FR-007, FR-016).**
  The run selects the engine by writing the word `osnoise` or `timerlat`
  to `current_tracer`, restricts it by writing the mask of the tracer
  cpus to `tracing_cpumask`, and starts and stops it with `tracing_on`
  (the three files stand at the root of the tracing tree). The write of
  the engine word stands the availability probe: a host that carries no
  tracefs mount at the standing path, no tracer engine, or no rights for
  the writes answers a failed write, and the session stands not
  available (spec D-5). The mask stands rendered in the word width the
  standing `tracing_cpumask` text carries, which the setup reads for the
  restore beside the standing `current_tracer` word (R-04).
- **R-03 The count files and the five sources (spec FR-010).** The
  figures stand read from the per-cpu count files the `osnoise` directory
  of the tracing tree carries, one file per cpu, named `osnoise/cpu<id>`,
  each file stating the cumulative counts of the noise sources of that
  cpu: NMI, IRQ, softirq, thread and hardware, the kernel's own word for
  the last one `hw`. The reading stands re-confirmed against the lane
  host's tree at implementation, where the acceptance runs of spec D-7
  need the engine anyway; a mismatch goes to the pull request as a
  question, the pattern specs/034 R-03 set for the schedstat mapping
  (research.md R-3 carries the confidence note).
- **R-04 The setup order and the restore (host hygiene).** The setup
  reads the standing `current_tracer` word and the standing
  `tracing_cpumask` text, writes the engine word, the tracer mask and
  `tracing_on` at `1`; after the figures stand read, the run writes
  `tracing_on` at `0`, restores the standing engine word and the
  standing mask, and answers nothing on a restore failure: the run
  never fails on the tracer (spec FR-016). The restore leaves the host
  as the run found it, because a tracer left running would carry the
  perturbation the option's own doc note names across every later run on
  the host.
- **R-05 The lifecycle point (spec FR-009, D-2).** The entry-point
  thread stands the engine up before the probe items are dispatched and
  reads the figures and tears the engine down after `JitterProbe::start()`
  returns. `start()` returns only behind the latch line, which every
  item reaches after its pre-check phase (the standing comment at
  `source/harness/jitter_probe.cpp`, the latch line), so the figures are
  complete at the read in both modes. The buffer touch and the
  measurement pass run beside the started tracer; the figures are the
  engine's cumulative totals since the tracer's start (spec FR-009), so
  the counts they add name no figure the record states. In `mode=both`
  the tracer's stop lands after the latch, and the alongside phase runs
  beside a tracer whose cpus stand outside the planned cpus by default
  (R-09): the alongside figures of the planned cpus stand unperturbed,
  and a tracer cpu a `cpus=` list names carries the perturbation the
  option's doc note states (spec FR-021).
- **R-06 The value grammar and the public fields (spec FR-002, FR-005,
  D-4, D-6).** The value grammar reads
  `--os-noise=osnoise|timerlat[,cpus=<cpu-list>]`. The public struct
  carries `TracerEngine tracerEngine = TracerEngine::NONE;` (the closed
  enum `NONE`, `OSNOISE`, `TIMERLAT`) and
  `std::vector<std::uint32_t> tracerCpus = {};` appended after
  `precheckSerial`. The parser (`parseOsNoiseValue`, declared beside
  `parseJitterProbeValue` in `source/harness/detail/noise_tracer.hpp`)
  reads the engine word, then one optional `cpus` key whose value
  `parseProbeCpuToken` reads whole: the standing rules stand reused, the
  affinity mask and the online set included, and no cpu stands twice.
  A second `cpus` key stands the standing repeated-key refusal. The
  enum closes the engine set at the type level, the way
  `JitterProbeMode` closes the modes; a string field would carry an
  open set the struct bounds nowhere.
- **R-07 The missing-window refusal (spec FR-011).** The check stands
  beside the standing combination check of the hunt family
  (`validateHuntCombination`), after the gather and ahead of every run:
  a settled `--os-noise` value beside a batch that carries no
  `--jitter-probe` option, or one whose mode runs no pre-check window
  (`ALONGSIDE`), prints one line and exits 2. The reading: the check
  reads the batch's probe options; a family-level probe stands a
  different surface, the pre-check record the tracer feeds stands the
  batch's alone, so a run whose batch carries no probe refuses. The
  compare mode carries no probe, so it refuses the same way.
- **R-08 The record type and the JSON shape (spec FR-014, FR-006, D-3).**
  `PrecheckRecord` gains
  `std::optional<PrecheckTracerRecord> tracer;` appended after
  `skippedGroups`. `PrecheckTracerRecord` carries the engine word and
  one entry per tracer cpu (`PrecheckTracerCpuRecord`: the cpu index and
  the five figures, each `std::optional<std::uint64_t>`); the entry
  names its cpu, so the cpus the tracer ran on stand the array itself.
  The writer emits the object after `skippedGroups`; the reader reads it
  with the standing `optionalField` pattern, the way `hunt` and
  `metricMethod` read. The schema gains the key in the `precheck`
  object's `properties` and `x-order` and stands outside its
  `required` list: the standing checker
  (`checkNode`, `source/harness/json_load.cpp`) checks a key of
  `properties` only when the document carries it, compares only the
  keys the document carries against `x-order`, and refuses any key a
  closed object does not list, so the three lists stand the whole
  optionality mechanics (research.md R-8). `kSchemaVersion` moves to
  `1.7`; the top-level `x-order` stays (spec FR-006).
- **R-09 The tracer cpus (spec FR-008, D-2).** The cpus the `cpus=` key
  names stand the tracer cpus; an absent key names the pre-check's
  candidate set, the cpus of the measured set the pre-check record
  names `candidate`, one per L3 group the plans touch. The set stands
  resolved once beside the measured set (`source/harness/cli.cpp`, the
  `batchProbed` block). A list naming a planned cpu runs as named; the
  run carries no overlap refusal for it, and the docs carry the
  perturbation note beside the option (spec FR-021). The default keeps
  the tracer's own load off the planned cpus, which is the fact the
  strict gate and the alongside phase stand on.
- **R-10 The byte identity (spec FR-012, FR-015).** A run that requests
  no tracer writes no tracer key and prints no tracer line, so its
  document stands byte-identical to master's apart from the
  `schemaVersion` value and the version figures the document carries.
  A run whose tracer stands not available prints
  the tracer lines with the `not available` wording (spec FR-016,
  SC-004) and writes no tracer key, so its document stands byte-identical
  to master's apart from the `schemaVersion` value and the version
  figures the document carries, and its standing
  console lines stand byte-identical, the tracer lines being the new
  ones beside them. The reading pins the two claims of spec FR-015:
  the byte identity binds the JSON in both cases and the standing
  console lines; the tracer lines of the not-available case stand the
  disclosure FR-016 asks for. The alternative reading (a not-available
  run prints no tracer line) contradicts spec FR-016 and SC-004, and
  stands recorded here as rejected (research.md R-12).
- **R-11 The evidence records (spec FR-020, D-7).** The acceptance runs
  on thrawn stand recorded under `results/`, one record per run, the
  pattern of `specs/035-numa-placement/results/`: the commands, the
  exit codes, the engine, the per-CCD tables with the CCD named beside
  each figure, the CCD names read from the `--topology` listing the
  same host prints. A script records the runs; no CI gate stands on the
  records; the quickstart links every record.
- **R-12 The naming (V.1).** `TracerEngine`, `PrecheckTracerRecord` and
  `PrecheckTracerCpuRecord` take PascalCase; `parseOsNoiseValue`,
  `noiseTracerSetup`, `noiseTracerFigures` and `noiseTracerRestore`
  take camelBack; `kOsNoiseOptionValue` takes the `k` prefix; the new
  files take lower_case (`noise_tracer.hpp`, `noise_tracer.cpp`).
- **R-13 The contract deltas (spec FR-024).** The one new contract file
  carries the value grammar, the error surface and the option-table row
  in the shape of `specs/027-jitter-probe/contracts/cli-options.md` and
  `specs/034-host-noise-precheck/contracts/cli-options.md`; the console
  and JSON deltas carry the line shapes and the key sets beside the
  standing ones. No standing rule moves: the strict gate keeps gating
  the planned cpus' gap figures alone, and the tracer figures gate
  nothing (spec Out of scope).
- **R-14 The console lines (spec FR-013).** `printPrecheckReport` prints
  the tracer disclosure after the host-settings lines and before the
  warning lines: one header line naming the engine and the cpus, then
  one figure line per source per tracer cpu, the sources in the NMI,
  IRQ, softirq, thread, hardware order, cpus ascending, each absent
  figure standing the `not available` wording. The not-available case
  stands one line, `tracer: not available`. The wording stays unpinned;
  the tests assert the presence, the order and the named figures, the
  standing rule of the pre-check block.

### Modules and dependency order

The feature splits into six modules; each carries its public surface,
its contracts and its test plan, and each lands apart behind the seams
the standing code owns. The order below stands the landing order; the
brace marks the parallel group.

```text
1. M-A  the public surface and the version move
2. { M-B  the option row and the value parser
     M-C  the tracer engine session
     M-E  the record surfaces }        # the three land in parallel
3. M-D  the pre-check integration
4. M-F  the docs and the example
5. M-G  the evidence records
```

- **M-A the public surface and the version move**
  (`include/speedgun-ng/benchmark.hpp`, `CMakeLists.txt`).
  Public surface: `TracerEngine`, `JitterProbeOptions::tracerEngine`,
  `JitterProbeOptions::tracerCpus`, `PrecheckTracerRecord`,
  `PrecheckTracerCpuRecord`, `PrecheckRecord::tracer`. Contracts: the
  doxygen paragraphs of the new members and types, the bounds
  (`tracerCpus` empty names the candidates; a non-empty list names cpus
  inside the process affinity mask and the online set, the standing
  rules). Test plan: the source-order binding test and the struct
  default test of `harness_noise_test` (wave 1); the version print test
  stays green across the `0.23.0` move, and the `SOVERSION` check rides
  the version commit (spec FR-004, D-6). Nothing else builds before
  M-A lands.
- **M-B the option row and the value parser**
  (`source/harness/detail/options.hpp`, `source/harness/options.cpp`,
  `source/harness/detail/noise_tracer.hpp`, `source/harness/noise_tracer.cpp`).
  Public surface: none (detail). Contracts: the grammar and the error
  surface of [contracts/cli-options.md](./contracts/cli-options.md);
  the row joins the table at the value `297`, the block comment names
  `298` to `319` free. Test plan: the grammar rows, every refusal of
  the error table, and the row uniqueness rows of
  `harness_cli_table_test` (wave 1). Depends on M-A.
- **M-C the tracer engine session**
  (`source/harness/detail/noise_tracer.hpp`, `source/harness/noise_tracer.cpp`).
  Public surface: none (detail): `noiseTracerSetup`, the session type,
  `noiseTracerFigures`, `noiseTracerRestore`. Contracts: the session
  functions carry `\pre`/`\post` on the tracefs files they touch; a
  failed setup or a failed write answers an empty session, the
  not-available shape (R-02, R-04). Test plan: the engine rows of
  `harness_noise_test` over fake tracefs trees (wave 2): the set, the
  mask write, the start, the five figures, the per-source absence, the
  restore, every not-available path. Depends on M-A alone, so it lands
  beside M-B.
- **M-E the record surfaces**
  (`source/harness/report.cpp`, `source/harness/json_report.cpp`,
  `source/harness/json_load.cpp`, `docs/schema/report-v1.json`).
  Public surface: none beyond M-A's types. Contracts:
  [contracts/console-report.md](./contracts/console-report.md) and
  [contracts/report-json.md](./contracts/report-json.md). Test plan:
  the console, round-trip, schema and byte-identity rows of
  `harness_noise_report_test` (wave 3), built against a hand-built
  record, so the module stands independent of M-C and M-D. Depends on
  M-A alone.
- **M-D the pre-check integration**
  (`source/harness/cli.cpp`). Public surface: none. Contracts: the
  lifecycle of R-05 and the cpus rule of R-09; the missing-window
  refusal of R-07. Test plan: the integration rows of
  `harness_noise_test` (wave 4): the engine stands up before the
  window and reads after it, the `cpus=` list runs as named, the
  candidates stand the default, the serial windows run the tracer
  across every window, `mode=both` keeps the tracer's stop behind the
  latch with the planned cpus free of the default tracer load, the
  not-available host keeps the run whole at exit 0.
  Depends on M-B and M-C.
- **M-F the docs and the example**
  (`README.md`, `docs/pages/harness.md`, `example/docs/jitter_probe.cpp`,
  `example/docs/jitter_probe.expected`). Contracts: spec FR-021 to
  FR-023; the quick-reference rows carry the new option row and the new
  public members, the drift gate binds them against the headers and the
  option table. Test plan: `example_drift`, `snippet_compile`,
  `example_run`, `report_shape`, the prose gate (wave 5). Depends on
  every module above.
- **M-G the evidence records**
  (`specs/036-osnoise-timerlat-tracers/results/`). Contracts: spec
  FR-020 and R-11. Test plan: none in CI (the spec stands no gate on
  the records); the check stands the quickstart's links and the records'
  own shape (wave 6). Depends on the built binary and the acceptance
  host.

### Risks

- The tracefs file facts stand on kernel documentation read from
  knowledge, and the per-cpu count-file name and line format stand
  re-confirmed against the lane host's tree at implementation; the
  acceptance runs of spec D-7 need the engine anyway, so the
  confirmation rides them, and a mismatch goes to the pull request as a
  question (R-03, research.md R-3).
- The `tracing_cpumask` word width varies across hosts: the setup reads
  the standing mask first and renders the new mask in the width it
  read, so no width stands fixed (R-02).
- The example's tracer section prints host-dependent figures: on a host
  without the rights the tracer lines stand the `not available`
  disclosure, and the `.expected` file is compared by shape, the
  pattern the jitter example already follows (R-14, M-F).
- The `mode=both` tracer stop lands after the latch: the default
  candidate set stands outside the planned cpus, and a `cpus=` list
  naming a planned cpu carries the perturbation note the docs state
  (R-05).

### Logical view: the tracer around the pre-check window

```text
run starts (the batch probe carries the pre-check and the option names
             an engine)
  └─ entry point, the standing combination check
       ├─ refuse the missing pre-check window (R-07)  [ahead of every run]
  └─ entry point, the batchProbed block
       ├─ resolve the measured set and the tracer cpus (R-09)
       ├─ noiseTracerSetup(engine, cpus)          [outside every window]
       │    ├─ read the standing tracer word and mask
       │    ├─ write the engine word, the mask, tracing_on 1
       │    └─ any write fails → empty session (not available)
       ├─ JitterProbe::start()
       │    └─ the pre-check window(s), the standing grammar (FR-009)
       ├─ start() returns behind the latch: the window stands closed
       ├─ noiseTracerFigures(session)             [outside every window]
       │    └─ one entry per tracer cpu, five figures, absent = null
       ├─ noiseTracerRestore(session)             [outside every window]
       └─ the record builds: PrecheckRecord::tracer stands the figures
report
  └─ console: the tracer lines beside the standing block (R-14)
  └─ JSON: the optional tracer object (R-08)
```

### Physical view

The feature touches the files the Source tree lists. The one new
detail header and the one new source carry the engine session and the
value parser; the public types grow inside the standing header; the
wiring stands in the entry point beside the standing pre-check block;
the report writer, loader and printer carry the two surfaces; the
schema and the docs carry the contract. The public surface grows by
one enum, two fields and one optional record; everything else stays
harness-internal.

### Public API surface added

| Name | Shape | Contract |
| --- | --- | --- |
| `TracerEngine` | closed enum: `NONE`, `OSNOISE`, `TIMERLAT` | the engine the `--os-noise` value names; `NONE` names no tracer (R-06) |
| `JitterProbeOptions::tracerEngine` | `TracerEngine`, default `NONE` | the field carries the struct's doxygen contract; the entry points bind it beside the standing option sources (spec FR-005, R-06) |
| `JitterProbeOptions::tracerCpus` | `std::vector<std::uint32_t>`, default empty | empty names the pre-check's candidate set; a non-empty list names cpus inside the process affinity mask and the online set (spec FR-008, R-09) |
| `PrecheckTracerRecord` | `engine` + one `PrecheckTracerCpuRecord` per tracer cpu | the optional record of `PrecheckRecord::tracer`; the JSON shape stands in [contracts/report-json.md](./contracts/report-json.md) (spec FR-014, R-08) |
| `PrecheckTracerCpuRecord` | cpu index + five optional counts | the figures of one tracer cpu; an absent figure stands `not available` on the console and `null` in the document (spec FR-010, FR-013) |
| `PrecheckRecord::tracer` | `std::optional<PrecheckTracerRecord>` | stands when the tracer ran; a run that requests no tracer and a run whose tracer stands not available leave it empty (spec FR-015, R-10) |

No other public name joins: the results surface through the console
block and the JSON record alone.

### Version and `SOVERSION`

The version moves to `0.23.0` (the next free minor at branch time).
The public type growth is a public-ABI change, so the harness
`SOVERSION` rises from `13` to `14`; the archive `SOVERSION` stays `4`;
the check stands recorded in the commit that moves the version, beside
the `SOVERSION` comments `CMakeLists.txt` keeps for every lane that
moved one (spec FR-004, D-6, R-12 of the spec).

## Test Plan

TDD waves; each wave's tests stand written and observed failing before
the code that turns them green:

1. **Wave 1, the surface and the grammar (FR-001 to FR-005, FR-011).**
   The row at `297` with no standing name or value moved
   (`harness_cli_table_test`, the `noNameAndNoValueRepeat` and
   `theValueBlocksStayUniqueAndDisjoint` rows plus the new block row);
   `osnoise` and `timerlat` accepted, with and without `cpus=`; a
   cpu-list value outside the standing rules, a repeated key and an
   engine word outside the two each print one line and exit 2; the
   missing pre-check window and the missing `--jitter-probe` each
   refuse one line at status 2; the source order binds the command
   line, then the option file, then the suite call. Gate: `ctest` on
   `harness_noise_test` and `harness_cli_table_test`.
2. **Wave 2, the engine session (FR-007, FR-010, FR-016, FR-017).**
   Over fake tracefs trees installed through the seam: the engine word
   set, the mask written in the standing width, `tracing_on` at `1`;
   the five figures read from the per-cpu files, a source the file
   does not state standing absent for that source alone; every
   not-available path (no tracing root, an unwritable tracer word, a
   failed engine write, denied setup writes) answers the empty session;
   the restore returns the standing word and mask. Gate: `ctest` on
   `harness_noise_test`.
3. **Wave 3, the record surfaces (FR-006, FR-012 to FR-015).** The
   console block carries the tracer lines after the host-settings
   lines, the sources in the standing order, the standing lines
   byte-identical; the JSON round trip reads the record back through
   `loadReport`; the schema carries the key with the moved `precheck`
   `x-order` and the top-level list unchanged; a tracer-free run and a
   not-available run write documents byte-identical to master's apart
   from the `schemaVersion` value and the version figures the
   document carries. Gate: `ctest` on
   `harness_noise_report_test`, the schema check, `report_shape`.
4. **Wave 4, the integration (FR-008, FR-009).** The engine stands up
   before the items are dispatched and reads after `start()` returns;
   the `cpus=` list runs as named, a planned cpu included; the absent
   key names the candidates of the injected topology; the serial mode
   runs the tracer across every window with no reset write between
   them; `mode=both` keeps the tracer's stop behind the latch with the
   planned cpus free of the default tracer load. Gate:
   `ctest` on `harness_noise_test` under `taskset -c 0,1`.
5. **Wave 5, the docs (FR-021 to FR-023).** The README section inside
   the 25-line cap with the perturbation note and the quick-reference
   rows; the guide page rows; the example section and its
   `.expected`. Gate: `example_drift`, `snippet_compile`,
   `example_run`, `report_shape`, `prose-lint`.
6. **Wave 6, the gates and the evidence (FR-018 to FR-020, FR-024 to
   FR-026).** The suite green under `taskset -c 0,1` and on a 2-cpu
   host; no test sleeps and no test reads a clock; coverage at 100%
   lines and branches with no exclusion marker; sanitizers and tsan
   clean; the naming and format gates clean; the contract deltas
   joined to the standing files; the acceptance records of thrawn
   linked from the quickstart. Gate: the full CI matrix and the
   coverage preset.

### FR coverage map

| FR | Covering test (the wave that runs it) |
| --- | --- |
| FR-001 | the row uniqueness rows of `harness_cli_table_test` (wave 1) |
| FR-002 | the grammar rows of `harness_noise_test` (wave 1) |
| FR-003 | the refusal rows of `harness_noise_test` (wave 1) |
| FR-004 | the version print test stays green across the move; the `SOVERSION` check rides the version commit (wave 1) |
| FR-005 | the source-order and struct-default rows of `harness_noise_test` (wave 1) |
| FR-006 | the schema rows of `harness_noise_report_test` (wave 3) |
| FR-007 | the engine rows over the fake root of `harness_noise_test` (wave 2) |
| FR-008 | the integration cpus rows of `harness_noise_test` (wave 4) |
| FR-009 | the integration window rows of `harness_noise_test` (wave 4) |
| FR-010 | the per-source figure rows of `harness_noise_test` (wave 2) |
| FR-011 | the missing-window refusal rows of `harness_noise_test` (wave 1) |
| FR-012 | the byte-identity rows of `harness_noise_report_test` (wave 3) |
| FR-013 | the console tracer-line rows of `harness_noise_report_test` (wave 3) |
| FR-014 | the JSON round-trip rows of `harness_noise_report_test` (wave 3) |
| FR-015 | the tracer-free and not-available byte rows of `harness_noise_report_test` (wave 3) |
| FR-016 | the not-available rows of `harness_noise_test` (wave 2) |
| FR-017 | every engine row runs the injected root (wave 2); CI needs no tracer and no root |
| FR-018 | the suite under `taskset -c 0,1` and on a 2-cpu host (wave 6) |
| FR-019 | the test review of the wave gates; no sleep, no clock (every wave) |
| FR-020 | the results records and the quickstart links (wave 6) |
| FR-021 | `example_drift` over the README rows and the prose gate (wave 5) |
| FR-022 | the guide rows of wave 5 and the drift gates |
| FR-023 | `snippet_compile`, `example_run`, `report_shape` (wave 5) |
| FR-024 | the contract deltas joined in wave 6; the tasks verify the join |
| FR-025 | the coverage preset (wave 6) |
| FR-026 | clang-tidy and the standing fixtures (wave 6) |

## Constitution Check

*Re-check after Phase 1 design.*

The design keeps every gate green: the public growth carries its
contracts (II); the timed paths stand untouched and the tracer work
stands outside every window (I, VII); the coverage plan names every new
branch through the fake tracefs files and the scripted reader (VI); the
seams reuse the standing patterns and the public surface grows by one
enum, two fields and one optional record (X); the one externally
sourced reading (the per-cpu count-file name and line format) carries
its recorded re-confirmation and its pull-request question path (R-03).
**PASS.**

## Complexity Tracking

> Empty: no constitution violation stands, so no justification row
> applies.
