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

## Phase: N13e trace-line parsing plan (2026-11-09)

**Base**: `origin/master` at `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`
(version 0.34.0, report schema 1.16, harness `SOVERSION` 22, archive
`SOVERSION` 8). **Branch**: `fix/fwq-example-n13e`, the phase 15 branch
of `specs/045-merge-fixes`; the FWQ example lane and this lane share the
branch, and no file stands in both. **Spec**: the trace-line phase of
[spec.md](./spec.md): FR-027 to FR-030, SC-008 to SC-011, D-8 to D-12.
**Host of record**: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
threads, kernel `7.3.0-5-cachyos`. **Tasks**: N13E-001 the read,
N13E-002 the cases, N13E-003 the record and the contract line, N13E-004
the shared acceptance set; the four rows land in `tasks.md` in this
phase's tasks pass.

### Summary of the phase

The figure read of the two standing engines reads a file no reachable
tree carries. `figuresOfCpu` folds `sys/kernel/tracing/osnoise/cpu<id>`,
and the 2026-10-29 records found that path absent with either engine set
(`results/osnoise.md`, `results/timerlat.md`), so the five figures print
`not available` on the host of record. The phase moves the read to the
per-cpu trace buffer the two engines already write,
`sys/kernel/tracing/per_cpu/cpu<id>/trace`, the path the detector read of
specs/048 already opens, and retires the count-file reader with its
fixture format and its cases (D-12).

Nothing else moves. The five figures keep their names and their
optionality, so the record layout, the schema, the version and the two
`SOVERSION` numbers stand, and no public header changes. The detector
read, the setup and restore order, the option grammar and the value
parser stand untouched.

### Design

#### The figure read (N13E-001)

1. **One path, one read, three folds.** `noiseTracerFigures`
   (`source/harness/noise_tracer.cpp`) reads
   `sys/kernel/tracing/per_cpu/cpu<id>/trace` once per cpu through
   `readTracerFile` and hands the text to one fold keyed on the engine:
   `gapFiguresOfCpu` for `hwlat`, unchanged, and one of the two new
   folds below for the standing engines. The path expression stands
   written once in the loop for all three engines, which is D-9: one seam,
   one path, one read per engine, the line rule keyed on the engine.
2. **Two folds, one rule each.** `osnoiseFiguresOfCpu(cpu,
   text)` and `timerlatFiguresOfCpu(cpu, text)` join `gapFiguresOfCpu`
   in the anonymous namespace of `noise_tracer.cpp`. Each carries one
   rule and one postcondition. One function with an engine parameter
   would carry a postcondition that differs per engine and a branch
   inside the fold: the generality X.2 refuses.
3. **The shared line parts.** Three helpers stand beside them:
   `tokensOf(line)` splits a line on blank, tab and carriage return into
   a `std::vector<std::string_view>`; `standsAnchored(tokens, at)`
   answers whether the token ahead of position `at` ends with `:`, the
   timestamp-colon anchor both engines share; `decimalOf(token)` answers
   digits, one `.`, digits. `countOfToken` keeps its name and answers
   the digits predicate; its parameter becomes `std::string_view` so a
   token needs no copy, and the detector's three call sites in its two
   helpers keep their bytes. Two constants carry the tail widths: `kPeriodColumns` at nine
   and `kActivationColumns` at six.
4. **The header rule stands first.** A line whose first character stands
   `#` stands a header line and answers no period line and no activation
   line, whatever its tail. A blank line answers no tokens and falls out
   of the token-count guard.
5. **The osnoise period rule.** A line stands one period line when it
   carries at least `kPeriodColumns + 1` tokens, its last nine answer
   digits, digits, digits-dot-digits and then six groups of digits, and
   the token ahead of them ends with `:`. The last five of the nine
   stand the counters, positional, in the order the line prints them:
   `HW`, `NMI`, `IRQ`, `SIRQ`, `THREAD` into the record's `hardware`,
   `nmi`, `irq`, `softirq` and `thread` figures. The first four of the
   nine (runtime, noise, the percent figure, the widest single noise)
   answer the shape and no figure of their own. A bare integer where the
   decimal figure stands answers no period line.
6. **The timerlat activation rule.** A line stands one activation line
   when it carries at least `kActivationColumns + 1` tokens, its last
   six answer `#` and digits, `context`, a context word,
   `timer_latency`, digits and `ns`, and the token ahead of them ends
   with `:`. Nothing follows `ns`, because the six stand the tail. The
   word `irq` counts the `irq` figure and the word `thread` counts the
   `thread` figure; any other word stands no line. The two counts stand
   independent: no line looks for its pair.
7. **Sums, counts, zero, absence.** The osnoise fold keeps five
   `std::uint64_t` accumulators and the timerlat fold keeps two counters,
   each beside a parsed-line count. Beside a parsed line the fold writes
   its figures, so a counter column that reads 0 on every line stands a
   filled 0, the shape `gapFiguresOfCpu` answers for `nmiCount`. Beside
   no parsed line the entry keeps every figure absent, the answer a
   missing file and an unreadable file already give through
   `readTracerFile`. The sums stand the totals of the lines the buffer
   holds at the read (D-11).
8. **The detector read stands untouched.** `kGapMarker`, `kNmiField`,
   `widerGapOf`, `nmiOf` and `gapFiguresOfCpu` keep their bytes. No
   column table and no line rule stands shared between the detector read
   and the two new folds, which the out-of-scope line fixes.

#### The contract text (II)

The declaration of `noiseTracerFigures` in
`source/harness/detail/noise_tracer.hpp` keeps its `\pre` and rewrites
its body and its `\post` in this shape:

```text
/// @brief The figure read of the two standing engines and of the
/// detector (specs/036 FR-010, FR-027, R-07; specs/048 FR-018, FR-027):
/// one entry per cpu of `session.cpus`, in the session order. All three
/// engines read the per-cpu buffer text
/// `sys/kernel/tracing/per_cpu/cpu<id>/trace` through the standing
/// file-root seam, one read per cpu (D-9). A line whose first character
/// stands `#` stands a header line and answers no figure. For `osnoise`
/// the last nine tokens of a period line answer digits, digits,
/// digits-dot-digits and six groups of digits behind a token that ends
/// with the timestamp colon, and the last five of them sum into the
/// hardware, nmi, irq, softirq and thread figures. For `timerlat` the
/// six tokens after that token stand `#` and digits, `context`, a
/// context word, `timer_latency`, digits and `ns`; the `irq` and
/// `thread` words count the two figures, and the other three stand
/// absent for that engine alone (D-10). A line beside those shapes
/// stands skipped. A buffer that carries no parsed line, stands
/// missing, or stands unreadable stands every figure of that cpu
/// absent, and a counter column that reads 0 on every parsed line
/// stands a filled 0 (R-07). The read never fails the run (FR-016).
/// The function stands on the entry-point thread, outside every timed
/// window (R-05).
///
/// \pre `session` stands the setup's answer, the engine naming a
///       tracer
/// \post the answer carries one entry per cpu of `session.cpus`, in
///       the session order
/// \post the figures of a standing engine stand filled when that cpu's
///       buffer holds a parsed line and stand absent beside none, and
///       the three figures the `timerlat` engine states nothing for
///       stand absent whatever that buffer holds (FR-027, D-10)
```

In the source each new fold carries a leading comment in the shape
`gapFiguresOfCpu` carries today and one `SG_ENSURE` at its answer:

```cpp
  const bool any = periods > 0;
  SG_ENSURE(any == (record.hardware.has_value() && record.nmi.has_value()
                    && record.irq.has_value() && record.softirq.has_value()
                    && record.thread.has_value()),
            "osnoiseFiguresOfCpu answers five figures beside a period line "
            "and none beside no line");
```

```cpp
  const bool any = counted;
  SG_ENSURE(any == (record.irq.has_value() && record.thread.has_value())
                && !record.nmi.has_value() && !record.softirq.has_value()
                && !record.hardware.has_value(),
            "timerlatFiguresOfCpu answers two figures beside an activation "
            "line and keeps the three the engine states nothing for absent");
```

`noiseTracerFigures` keeps its `SG_ENSURE` on the entry count. The two
folds stand in the anonymous namespace, the shape `gapFiguresOfCpu`
carries today, so the DBC documentation and pairing gates read them as
they read that function now.

#### What the change deletes

- `source/harness/noise_tracer.cpp`: `struct CountSource`,
  `kCountSources` and `figuresOfCpu`. `countOfToken` stays: the
  detector's two helpers and the new folds read their digits through it.
- `test/source/harness_noise_test.cpp`: the fixture helper
  `cpuCountFile`, the path helper `countPathsOf`, and the four cases
  that fold the retired format, `theFiguresReadOneEntryPerTracerCpu`,
  `theMissingSourceWordStandsAbsentForThatSourceAlone`,
  `theCpuWithNoCountFiguresStandsEveryFigureAbsent` and
  `theFigureArmRows`.
- No other code names the retired path. The prose surfaces that still
  name it stand listed in the record (FR-029).

The per-source absence proof does not go with
`theMissingSourceWordStandsAbsentForThatSourceAlone`. Under the buffer
rule a standing engine states all five figures or none, and the
per-source rule of FR-010 lives in the `timerlat` reading (D-10), which
`theTimerlatLinesCountTheTwoContexts` carries.

#### What the change re-anchors

The serial-window and `mode=both` rows carry the only observable proof of
the read-after-the-window and no-reset rules, so they move to buffer text
through the same seam (FR-028):

- `bufferPath(cpu)` replaces `cpuCountFile(cpu)` and answers
  `sys/kernel/tracing/per_cpu/cpu<id>/trace`, the name and the spelling
  the specs/048 fixture already uses in
  `test/source/harness_hwlat_tracer_test.cpp`. `countPathsOf` becomes
  `bufferPathsOf`.
- `TracerWindowReader` takes `bufferPaths` and appends instead of
  rewriting: its first read opens the buffer with `std::ios::app` and
  writes the appended period lines, leaving the earlier lines standing. A
  rewrite would read as the reset the row proves absent.
- `kPreWindowFigures` and `kPostWindowFigures` become `kPreWindowLines`
  and `kAppendedLines`: the legend bytes with one period line, and one
  period line. Their counters stand chosen so the sums over both sets
  answer the five figures the standing check asserts, which keeps that
  check's bytes and renames it `checkWindowSums`:

```text
       osnoise/1-2542397 [001] ....1 1241576.199778: 1000000       1659  99.83410      19      5      3      4      5      6
       osnoise/1-2542397 [001] ....1 1241577.199779: 1000000       1403  99.85970       8     10      8      8      8      8
```

  The sums answer nmi 11, irq 12, softirq 13, thread 14 and hardware 15.
  The record's figures name the sum of both lines, so the row proves the
  read landed after the append and that nothing cleared the earlier line.
- The rows that write the fixture stand re-pathed with no change of
  assertion: `theWindowLifecycleRows` (the serial pair, `mode=both`) and
  `theTracerCpuRows` (the named list, the absent key, the wholly planned
  set).

#### The record, the quickstart and the contract line (N13E-003)

- One new file, `specs/036-osnoise-timerlat-tracers/results/trace-line-thrawn.md`:
  the audit point repeated (the legend bytes, the four verified lines,
  the `period_us` read, the per-cpu `buffer_size_kb` read); each
  acceptance command with its exit and its window token; the resolved
  cpu list and the figures beside the CCD naming the standing records
  use; the tracing state each run found and the state it left, the saved
  word, the saved mask and `tracing_on` read before and after; the
  unrooted suite row apart; the dated rows labeled preliminary with the
  pre-squash head each stands taken at; and the stale-wording list of
  FR-029, which carries the public header comment, the spec 034
  report-json line, this lane's option contract, the data model, the
  research rows, the quickstart wording and the two standing edge-case
  rows of `spec.md` that name the count files and the cumulative-totals
  reading.
- `quickstart.md`: one link row to that record in the acceptance
  section. No other quickstart line moves, the count-file wording
  included.
- `contracts/console-report.md`: one rule line, in the same change as
  the code:

```text
- The five source figures of the `osnoise` engine stand the sums of the
  five interference-counter columns of the period lines that cpu's
  buffer `sys/kernel/tracing/per_cpu/cpu<id>/trace` holds at the read.
  The `timerlat` engine states the irq and thread figures from its
  activation lines and prints the standing `not available` wording for
  the other three (specs/036 FR-027, D-9, D-10). A buffer that holds no
  parsed line prints the standing wording for every figure of that cpu.
```

#### Acceptance (N13E-004)

The two sudo runs of the local-acceptance block, each with
`precheck=3000` against the one-second period the audit point names, the
unrooted `ctest` row apart, and the shared lane set of spec 045 phase 15
(BFW-012) at the final squashed head. A run whose host refuses the
tracefs writes stands BLOCKED with the command and the refusal, and the
suite row answers on the standing tree either way.

### Test plan (N13E-002)

| Case | What it asserts |
| --- | --- |
| `theFiguresFoldThePeriodLinesOfEachCpuBuffer` | two cpus in the session order, the five figures the sums of each cpu's verified period lines (SC-008) |
| `theTimerlatLinesCountTheTwoContexts` | the verified pair counts irq 1 and thread 1, and nmi, softirq and hardware stand absent (SC-008, D-10) |
| `theMixedBufferAnswersTheSumsOfItsParsedLines` | legend, verified lines, malformed lines, a blank line and a planted `#` line with a period tail: the sums come from the parsed lines alone (SC-008) |
| `theZeroCounterColumnStandsAFilledZero` | a counter column reading 0 on every period line answers a filled 0 (SC-008) |
| `theCpuWithNoParsedLineStandsEveryFigureAbsent(0..3)` | an empty buffer, a missing file, a buffer path planted as a directory, and a legend-only buffer each answer every figure absent (SC-008) |
| `thePeriodShapeRows` | each malformed period shape, as a buffer of that one line, answers the absence rule (SC-008) |
| `theActivationShapeRows` | each malformed activation shape, as a buffer of that one line, answers the absence rule (SC-008) |
| `theLegendAndPlantedHashLinesStandNoLine` | the audit point's legend bytes and a planted `#` line that carries a period tail answer the absence rule (the header precedence of Round 2) |
| `theWindowLifecycleRows`, `theTracerCpuRows` | the re-anchored serial, `mode=both`, named-list, absent-key and wholly-planned rows: the sums cover every held line, the engine stands up before the reads, and the tree stands restored (FR-008, FR-009) |
| `theTimerlatFiguresPrintTwoFilledAndThreeAbsent` in `harness_noise_report_test.cpp` | a hand-built `timerlat` record prints the two filled figures and the standing wording in the other three positions, and the standing block keeps its bytes (FR-013) |

The two shape tables stand written as shape arrays in the shape of the
retired `theFigureArmRows`, so each row prints its own shape on failure.
The period table carries a non-digit in each of the three digit-only
non-counter columns, a signed figure in a counter column, a bare integer
and a `99.` in the decimal column, an eight-figure tail, a ten-figure tail that puts
a bare figure in the anchor position, and a line whose token ahead of the
nine carries no colon. The activation table carries no `#`, `contexts`
for `context`, a context word outside the two, `timer_latencies`, a
non-digit latency, `n` for `ns`, a token after `ns`, five tokens after
the anchor, and no colon anchor.

No case sleeps, reads a clock or names a fixed cpu: the cpus come from
`pickCpus`, and the window stays bounded by the scripted reader
(FR-019).

### Coverage plan (VI)

- The three engine arms of `noiseTracerFigures` stand covered by the
  osnoise and timerlat cases above and by
  `theFiguresFoldTheLinesOfEachCpuBuffer` in
  `test/source/harness_hwlat_tracer_test.cpp`.
- `tokensOf`: the no-token arm by the blank line of the mixed buffer, the
  token arm by every verified line.
- `standsAnchored`: true by the verified lines, false by the no-anchor
  row of each table and the ten-figure row of the period table.
- The period shape: each predicate answers false once in
  `thePeriodShapeRows` and true in
  `theFiguresFoldThePeriodLinesOfEachCpuBuffer`.
- The activation shape: each predicate answers false once in
  `theActivationShapeRows` and true in
  `theTimerlatLinesCountTheTwoContexts`.
- The zero-column arm answers true in
  `theZeroCounterColumnStandsAFilledZero`, where the verified host line
  already carries HW 0 and NMI 0, and false in the sums rows.
- The parsed-line-count guard answers false in
  `theCpuWithNoParsedLineStandsEveryFigureAbsent` and true in the sums
  rows.
- `readTracerFile` keeps its two arms: the missing file and the planted
  directory, both in the absence case, the same denial the specs/048 row
  plants at the same path.
- No new `LCOV_EXCL` marker joins anywhere. The retired reader takes its
  branches with it, so the tree-wide branch shortfall stands at or below
  the standing 21 and the marker count at or below the standing 84
  (SC-011, spec 045 SC-032). Net new cold rows: zero.

### Constitution Check

- I: the parser is standard library only: `std::istringstream`,
  `std::string_view`, `std::from_chars`, `std::array`. No dependency
  joins and no regular expression.
- II: the declaration of `noiseTracerFigures` carries the rewritten
  `\post` pair, and each fold carries one `SG_ENSURE` at its answer, the
  shape `gapFiguresOfCpu` carries today.
- VI: every branch of the new parser stands named above with the case
  that runs it, no exclusion marker joins, and the fault the phase
  closes ships with the cases that pin the parsed forms.
- VII: one file read per cpu per run, the count the retired read already
  paid; the fold stands outside every timed window and touches no timed
  loop.
- VIII: no gate, filter, allowance, marker, ignore-error class or
  timeout moves; the shared acceptance set of spec 045 phase 15 stands
  the gate for both lanes.
- X.2: no general trace parser. Two named folds over one buffer text, no
  column table shared with the detector read, no event-code surface
  (G-31), no automatic ranks (G-32), no tuning key.
- X.3: the count-file reader, its source-word table, its fixture helper
  and its four cases stand removed in the same change that removes their
  subject; `countOfToken` stays because the detector read still reads
  through it.
- XI: the comments, the record, the contract line and the commit follow
  ASD-STE100.

### Risks and the open items

- Kernel shape variance. A kernel that prints other shapes answers the
  absence rule, the standing shape of a host the format does not match.
  The record names the kernel, the read date and the verified bytes, so
  a reader can tell a parse miss from a host change.
- Buffer ring truncation. The sums stand the totals of the lines the
  buffer holds (D-11). At the acceptance window (18,000 ms of the
  probe's tick time base, 4.3 s of host time) the osnoise buffer holds
  about four period lines, while the timerlat buffer holds thousands
  of activation lines against a 1,410 kB ring, so the record carries
  the per-cpu `buffer_size_kb` read beside the figures and a truncated
  buffer shows there with its capacity named.
- Fixture drift from the verified bytes. The verified fixture lines
  stand copied byte for byte from the audit point, so the suite pins what
  the host printed on 2026-11-09; the window rows plant their counter
  columns beside the verified shape. The risk runs the other way: a fixture
  that keeps passing while no reachable host prints those bytes. The
  record's audit-point read at the final head answers it.
- The scripted append stands a scripted stand-in. The fake tree never
  clears a buffer, so the re-anchored rows prove the run's read order
  and its absence of a reset write. The kernel's clear-on-set behaviour
  stands proven by the audit-point host read; the suite carries
  the scripted stand-in.
- The report-test rendering row. The standing byte-identity fixtures
  (`kBlockHead`, `kTracerLines`, `kNotAvailableLine`) keep their bytes,
  and the new row splices its own lines beside them. A printer change for
  absent figures fails the new row, and the standing fixture bytes stay
  unchanged.
- The acceptance figures depend on the resolved candidate set. On thrawn
  the compact two-thread plan resolves one candidate cpu, so SC-010
  reads one cpu at the record's run and grows with the plan; the record
  names the resolved list beside the figures.
- The shared branch. The two lanes of `fix/fwq-example-n13e` touch
  disjoint files. A finding that needs a file outside this lane's scope,
  a `README.md` line, `test/CMakeLists.txt` or a preset, stands reported
  before any edit.
