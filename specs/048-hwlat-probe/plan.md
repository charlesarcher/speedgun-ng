# Implementation Plan: Hardware Latency Probe with SMI Attribution

**Branch**: `048-hwlat-probe` | **Date**: 2026-11-05 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/048-hwlat-probe/spec.md`

## Summary

The probe gains a method that names the source of a gap, and the tracer
gains the kernel's hardware-latency detector as a third engine.
`--jitter-probe` takes `method=hwlat` beside `tsc`, `fwq` and `ftq`: the
standing gap loop keeps its shape, and the run reads `MSR_SMI_COUNT`
through `/dev/cpu/<cpu>/msr` at each gap and at the two window edges, so
each gap carries the count change of the span that ends at it and each
probe cpu carries the two window deltas beside its gap figures.
`--os-noise` takes `hwlat` beside `osnoise` and `timerlat`: the run saves
six standing values, writes the mask, the per-cpu mode, the detector
width and window, the threshold, the engine word and `tracing_on`, folds
three figures from each tracer cpu's buffer text, and writes the saved
values back in reverse order on each path the process controls. Both surfaces disclose `not available` and continue
with status 0. The schema moves to `1.15` with eleven new keys, the loader
reads the six figures back, the comparison gains six rule names, and the
harness `SOVERSION` rises from `21` to `22` with the version move to
`0.32.0`.

## Technical Context

**Language/Version**: C++23, the `cxx_std_23` feature the standing
targets state.

**Primary Dependencies**: no new dependency. The feature rides seams and
units that stand today: the file-root seam
(`source/detail/proc_root.hpp:35`, `:52`), the probe loop and its folds
(`source/harness/jitter_probe.cpp`), the tracer lifecycle
(`source/harness/noise_tracer.cpp`), the entry-point wiring
(`source/harness/cli.cpp`), the console printer
(`source/harness/report.cpp:987`, `:1469`), the writer
(`source/harness/json_report.cpp`), the loader
(`source/harness/json_load.cpp`), the comparison
(`source/harness/compare.cpp`), the schema
(`docs/schema/report-v1.json`), and the option module
(`source/harness/options.cpp:680`, `:686`). Standard files carry every
access: `std::ifstream` for text, `::open` and `::pread` for the model-
specific register node, and `std::from_chars` for each figure.

**Storage**: none of its own. The run reads two kernel interfaces and
writes seven tracefs files and writes back the six values it saved, the
`tracing_on` write aside. The one document the feature writes stands the
report file `--json` names.

**Testing**: the repository's own harness, one executable per area,
registered in `test/CMakeLists.txt`. Three executables join:
`harness_latency_counters_test`, `harness_hwlat_probe_test` and
`harness_hwlat_tracer_test`. Twelve standing executables carry the rest
of the new checks. Fake files stand under a temporary root the seam
installs; the fake provider and the scripted `WindowReader` carry time,
so no test sleeps and no test reads a clock.

**Target Platform**: Linux only. The suite passes on a host without
root, without `msr` nodes and without tracefs, because every such read
stands faked (spec FR-001, FR-032).

**Performance Goals**: the gap loop keeps its standing cost. The kept gap
record keeps its 16 bytes (`source/harness/detail/jitter_probe.hpp:47`),
the loop keeps its no-allocation proof
(`test/source/harness_jitter_probe_alloc_test.cpp`), and the counter read
runs once per kept gap and twice per window: one `pread` of 8 bytes on a
descriptor the probe opens once.

**Constraints**: 100% line coverage, a branch shortfall at or under 21
rows, 84 exclusion markers as the ceiling, the prose gate, the DBC pair
gate at 347 interfaces with 0 gaps, the drift gate, and the `README.md`
cap of 1,004 lines against the 1,001 lines of master.

**Scale/Scope**: two new units, 11 new report keys, two new enum values,
six new comparison names, three new executables, roughly 900 new source
lines and 1,600 new test lines.

## Constitution Check

Gates that bind this lane, each with the obligation the plan meets:

- I standard-first: no `P0` trick enters the probe loop; the one
  hot-path addition stands one `pread` outside the measured interval.
- II Design By Contract: every new function carries doxygen with `\pre`,
  `\post` or `\invariant`, and the `SG_REQUIRE` / `SG_ENSURE` pair the
  DBC pair gate pairs at the interface (347 interfaces, 0 gaps today).
- III R-DCUT with TDD: this plan records TDD as its execution mode. Each
  module lands as a test task that reaches red, then an implementation
  task that reaches green.
- IV and IV.1 documentation: new public members carry doxygen, the guide
  page carries both surfaces, the example carries its region, and the
  main page keeps its rows within the cap.
- V style: `clang-format` 18.1.8 clean, camelCase identifiers, no
  abbreviation outside the V.2 list.
- VI coverage: every new line covered, no new exclusion marker, and each
  error path reachable through a fake: a path that stands a directory, a
  lowered file-size limit, a short read, a truncated text.
- VII performance discipline: the invariant above, and the attribution
  figures reported as counts and deltas, never as a single number.
- VIII hard gates: format, spell, drift, time-source, prose, DBC pair,
  build with clang-tidy and cppcheck, sanitizers, coverage, and the full
  and pinned `ctest` runs on corsairone.
- IX spec-driven: this plan, `tasks.md`, the contracts and the code
  follow the spec's numbering, and FR-001 to FR-006 keep the lane's ids.
- X anti-slop: no new option value, no new abstraction beyond the two
  units, no speculative field, and no second seam.
- XI prose: Simplified Technical English in every added line.

Complexity Tracking records one allowance, and the lane grants no new
exception: the allowance stands in the tree today. The branch shortfall of
spec SC-002 reads at most 21 rows, the
number `tools/dbc/coverage_gate.sh:102` computes today, and line coverage
stands 100% with no new exclusion marker (spec FR-038).

## Project Structure

### Documentation (this feature)

```text
specs/048-hwlat-probe/
├── spec.md                     # the feature, FR-001 to FR-039, SC-001 to SC-010
├── plan.md                     # this file
├── research.md                 # R-01 to R-16: the measured facts and the choices
├── data-model.md               # E-01 to E-08: records, fields, units, availability
├── quickstart.md               # the two runs, the two gates, the fake-file recipe
├── contracts/
│   ├── cli-options.md          # the method word, the engine word, the error tables
│   ├── report-json.md          # the eleven new keys, their kinds and their order
│   ├── console-report.md       # the new console lines and the limits line
│   └── rule-names.md           # the six new comparison names
├── checklists/requirements.md
├── tasks.md                    # /speckit.tasks output
└── results/                    # the two recorded root runs and the gate records
```

The standing contracts spec `027` and spec `036` carry amendments in the
commit that lands the code (spec FR-039).

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp        # M2: HWLAT in two enums, three fields on JitterStats
source/harness/
├── detail/latency_counters.hpp          # M1 (new): the two counter sources
├── latency_counters.cpp                 # M1 (new): the msr reader, the NMI reader, the vendor gate
├── detail/jitter_probe.hpp              # M3: the delta ring, the window-edge snapshots, the marks
├── jitter_probe.cpp                     # M3: the method word, the gap-path read, the folds
├── detail/noise_tracer.hpp              # M4: the saved-value set, the detector figures
├── noise_tracer.cpp                     # M4: the engine word, the setup order, the restore
├── detail/json_report.hpp               # M5: the two word tables
├── json_report.cpp                      # M5: the schema version, the eleven keys
├── json_load.cpp                        # M6: the six figures, the settings, the mark kind
├── report.cpp                           # M7: the console figures and the limits line
├── compare.cpp                          # M8: the six names, two refusal lines
└── cli.cpp                              # M3, M4 wiring: one owner at a time, the integrator's file
docs/schema/report-v1.json               # M5: the eleven keys, two enums, six x-order moves
docs/pages/harness.md                    # M9: the method section, the engine subsection
README.md                                # M9: the two rows, within the cap
example/docs/hwlat_probe.cpp             # M9 (new): the example
example/docs/hwlat_probe.expected        # M9 (new): its recorded report
example/CMakeLists.txt                   # M9: add_docs_example
test/CMakeLists.txt                      # the integrator: three new executables, one shape entry
test/source/
├── harness_latency_counters_test.cpp    # M1 (new)
├── harness_hwlat_probe_test.cpp         # M3 (new)
├── harness_hwlat_tracer_test.cpp        # M4 (new)
├── harness_jitter_cli_test.cpp          # M3: the method word, the refusals
├── harness_jitter_probe_alloc_test.cpp  # M3: the no-allocation proof, extended
├── harness_noise_test.cpp               # M4: the setup order, the restore paths
├── harness_json_report_test.cpp         # M5: the eleven keys
├── harness_json_schema_test.cpp         # M5: the schema checker
├── harness_jitter_bytes_test.cpp        # M5: byte identity for a run that names neither word
├── harness_json_load_test.cpp           # M6: the round trip and the older document
├── harness_noise_report_test.cpp        # M7: the console lines and the record
├── harness_jitter_window_test.cpp       # M7: the probe block figures
├── harness_compare_test.cpp             # M8: the six names
├── harness_compare_cli_test.cpp         # M8: the refusal lines and the statuses
└── harness_cli_table_test.cpp           # M3, M10: the option-value guard, the version readers
```

Harness sources need no CMake edit: the target globs
`source/harness/*.cpp` with `CONFIGURE_DEPENDS`
(`CMakeLists.txt:288`). Test executables need one: the integrator adds
each `add_executable` block.

**Structure decision**: two new units under `source/harness/` for the two
kernel interfaces, the attribution inside the probe unit that already
owns the gap loop, the detector inside the tracer unit that already owns
the tracefs lifecycle, and one owner per shared file.

## Design

### Decision index

The spec carries D-1 to D-15. This plan adds P-1 to P-16, each one a
choice the code must make once.

- **P-1**: one unit answers both counter sources:
  `source/harness/detail/latency_counters.hpp`. It exports the vendor
  gate, the descriptor pair for the register node, the count read, and
  the `NMI` text reader. The probe unit and the pre-check read reach it;
  no other unit opens a kernel interface directly.
- **P-2**: the gap path stays allocation-free. The probe opens the node
  once at start with `::open`, reads with `::pread` into a stack buffer,
  and closes at stop. `std::ifstream` stands only on the window-edge text
  reads. The no-allocation executable keeps its exit 0.
- **P-3**: one gap's mark rides a per-cpu ring of 8-byte unsigned
  changes, the storage shape the standing series ring uses
  (`detail/jitter_probe.hpp:145`, the element record `SeriesRecord` at
  `:52`). The gap ring and the change ring append
  in the same step of the gap path, share one capacity and one drop
  counter, so index `i` of one answers index `i` of the other. One element
  holds this gap's read minus the read before it, so the mark answers the
  span between two reads (spec D-7). The fold reads a change above zero as
  `SMI` and zero as `NOT_SMI`, and counts the marked ones into
  `smiGapCount`.
- **P-4**: the vendor gate reads `vendor_id` from `/proc/cpuinfo` under
  the seam. `GenuineIntel` supports the counter. Any other reading, and
  an unreadable file, answer unsupported, and the figures disclose
  `not available` (spec D-4).
- **P-5**: the three per-cpu figures ride `JitterStats`: `smiDelta` and
  `nmiDelta` as `std::optional<std::uint64_t>`, `smiGapCount` as an
  unsigned count. The one record feeds `jitter.stats[]` and
  `precheck.cpus[].stats`, so the schema carries the three keys in each
  place that record stands, and a run of another method writes none of the
  three. The latch snapshot carrier `JitterLatchSnapshot`
  (`detail/jitter_probe.hpp:86`) gains the change vector beside its
  `series` vector, because the pre-check folds through `gapStatsOf`
  (`:316`) at `source/harness/cli.cpp:1393`.
- **P-6**: the gap mark rides the standing `gaps[]` items as `smi`, with
  the words `SMI` and `NOT_SMI` or `null`, written only for the `hwlat`
  method under the standing `--json-raw=gaps` gate. The loader reads the
  kind and refuses a value that carries none, the way it treats
  `stamp` and `gapNs` today, and keeps no public gap record.
- **P-7**: the detector's settings ride `PrecheckTracerRecord` as
  `windowUs`, `widthUs` and `thresholdUs`, and its figures ride
  `PrecheckTracerCpuRecord` as `maxGapNs`, `gapCount` and `nmiCount`,
  each optional. The writer emits all six keys for the `hwlat` engine
  alone, so an `osnoise` document keeps its standing bytes.
- **P-8**: `NoiseTracerSession` carries `std::vector<TracerSetting>`
  with one `{path, value}` pair per file the run saved, in write order.
  `tracing_on` stands outside the vector: the restore writes `0`, the
  standing shape. The restore walks the vector in reverse, so the kernel's
  own reset runs before the session puts the host threshold back, and it
  returns the list of paths whose write failed, so the caller can print
  one line each.
- **P-9**: a setup that fails partway restores what it wrote before it
  answers the empty session. The standing engines gain the same
  behaviour in their own file, and their restore order and saved set
  stand unchanged; the lane records the amendment beside the spec `036`
  contract line.
- **P-10**: the detector's three figures come from the ring-buffer text
  the kernel writes at `sys/kernel/tracing/per_cpu/cpu<id>/trace`, the
  path `kernel/trace/trace.c:7672` creates. One detection stands on one
  line in the shape `kernel/trace/trace_output.c:1531` prints: `#` then a
  sequence number, then `inner/outer(us): <inner>/<outer>`, then `ts:`,
  then `count:`, then `nmi-count:` only when the count stands above zero
  (`kernel/trace/trace_output.c:1538`, `:1546` to `:1547`), then
  `nmi-total:` only where the kernel holds no
  `CONFIG_GENERIC_SCHED_CLOCK` (`:1543` to `:1545`).
  `maxGapNs` takes the larger of the two figures times 1000, the rule
  `kernel/trace/trace_hwlat.c:296` uses for its own maximum. `gapCount`
  counts the lines of the cpu. `nmiCount` sums the `nmi-count` fields and
  answers 0 where none stands. The header legend lines start with `#` and
  a space and carry no digit second, so the parse rule that needs a digit
  second skips them (spec A-6).
- **P-11**: the comparison takes two tables. The three probe figures join
  `kJitterFields`, which moves from 32 rows to 35 and carries the count
  word `thirty-five` in its refusal line and in the rule-name contract.
  The three detector figures join a new `kHwlatFields` table of 3 rows
  behind a new head word: `hwlat:<cpu|any>:<hwlat-figure>` with the
  standing direction and threshold suffix. The standing jitter grammar
  keeps its shape, and the detector names keep their own refusal line
  beside the jitter one at `source/harness/compare.cpp:423` to `:431`.
- **P-12**: the availability disclosure rides the standing
  `jitter.disclosure` text, so no new key carries it, and the console
  prints one line per source per run.
- **P-13**: the limits statement rides one new optional key
  `jitter.limits`, written for the `hwlat` method alone, and the console
  prints the same sentence once.

- **P-14**: the run writes `hwlat_detector/mode` with the word `per-cpu`
  before the engine word, so each tracer cpu carries its own detector
  thread and states figures of its own. The kernel default `round-robin`
  moves one thread across the mask and leaves the cpus it never reached
  with an empty buffer (spec D-9). The mode file stands in the saved set,
  so a host that starts in `per-cpu` ends there.
- **P-15**: a host whose `current_tracer` already reads `hwlat` ahead of
  the run answers the unavailable shape of spec FR-023, with one
  disclosure line, and the session writes no file. The kernel returns
  success without initializing a tracer that already stands
  (`kernel/trace/trace.c:5043` to `:5044`), and `tracer_init` stands as
  the only path that resets the cpu buffers (`:4709` to `:4713`, reached
  at `:5111` to `:5112`), so the live detector's lines would answer for
  this run. The run gains no refusal, because the run never fails on the
  tracer.
- **P-16**: the saved value of `hwlat_detector/mode` holds the bracketed
  word alone. The read form lists the three words with the active one
  inside brackets (`kernel/trace/trace_hwlat.c:629-632`, `:642`), and the
  write handler compares one stripped word (`:692`, `:694`, `:706-711`),
  so the whole read form would answer `EINVAL` at the restore.

### Modules and dependency order

| ID | Module | Owned files | Needs | Covers | Tests |
| --- | --- | --- | --- | --- | --- |
| M1 | Counter sources | `latency_counters.{hpp,cpp}`, `harness_latency_counters_test.cpp` | none | FR-001, FR-007 to FR-011 | new executable |
| M2 | Public surface | `benchmark.hpp` | none | FR-002 words, FR-025 fields, FR-027 fields | existing tables |
| M3 | Probe attribution | `detail/jitter_probe.hpp`, `jitter_probe.cpp`, three jitter test files | M1, M2 | FR-002, FR-005, FR-012 to FR-017, FR-030 | `harness_hwlat_probe_test`, three standing files |
| M4 | Detector engine | `detail/noise_tracer.hpp`, `noise_tracer.cpp`, `harness_noise_test.cpp`, `harness_hwlat_tracer_test.cpp` | M2 | FR-003, FR-018 to FR-024 | new plus standing executable; a host that already runs `hwlat` answers the unavailable shape (P-15) |
| M5 | Writer and schema | `detail/json_report.hpp`, `json_report.cpp`, schema, three test files | M2 | FR-004, FR-025 to FR-027, FR-029, FR-031 | three standing executables |
| M6 | Reader | `json_load.cpp`, `harness_json_load_test.cpp` | M2, M5 key names | FR-004, FR-026, FR-029 | standing executable |
| M7 | Console | `report.cpp`, `harness_jitter_window_test.cpp`, `harness_noise_report_test.cpp` | M2 | FR-016, FR-027, FR-030, FR-031 | two standing executables |
| M8 | Comparison | `compare.cpp`, two compare test files | M2, M6 | FR-028 | two standing executables; the standing table reader answers a `double` (`:244`), so a nullable figure needs its absent check ahead of that read, before the fault line at `:1563` to `:1576` |
| M9 | Contracts, docs, example | contracts, guide page, `README.md`, example pair, `example/CMakeLists.txt`, the nine standing contract rows of specs `026`, `027`, `034` and `036` | M3, M5, M7 | FR-006, FR-036 to FR-039 | the gates |
| M10 | Integration | `cli.cpp`, `test/CMakeLists.txt`, version and `SOVERSION` | M3, M4 | FR-003 wiring, version checks | the full suite |

Waves, with the rule that two modules never own one file:

1. W1: M2. Small, and it unblocks every other module.
2. W2 (parallel): M1, M4, M5, M7.
3. W3 (parallel): M3, M6, M8, and the contract half of M9.
4. W4: M10, the docs and example half of M9, the gates, coverage, the
   pinned suite and the two recorded runs.

Build protocol for the waves: the tree `build` holds one builder. A
wave's workers check each translation unit with
`g++ -fsyntax-only -std=c++23 -Iinclude -Isource -Isource/harness
-Ibuild/export <file>` and add `-DSG_SCHEMA_PATH='"docs/schema/report-v1.json"'`
for the schema test. The integrator owns every `cmake --build` and
`ctest` run, records the red run of each test task, then the green run
of each implementation task, and sends the failing output back to the
owning worker.

### Risks and trade-offs

- **R-1**: one `pread` per kept gap lengthens the loop's gap path by one
  system call. Mitigation: the read runs after the gap passes the
  threshold, outside the measured interval; the descriptor opens once;
  the no-allocation executable keeps its exit 0; SC-006 proves the
  standing methods lose no byte.
- **R-2**: the detector's per-cpu figures come from ring-buffer text, and
  the text carries a header whose legend lines also start with `#`. The
  parse rule takes a line only when `#` stands at column 1 and a digit
  stands at column 2. A test plants a header of the standing shape, so a
  rule that takes legend lines reaches red.
- **R-3**: the two detector knobs carry the kernel's own bounds, and
  `width` must stay under `window`. The lane writes width before window,
  so a write of a width above the standing window cannot fail. A test
  plants the standing values and asserts the write order.
- **R-4**: the kernel saves `tracing_thresh` when the engine word lands,
  so its own reset puts back the run's value and only the session holds
  the host's. The reverse restore order answers that, and a test plants a
  non-zero standing threshold and asserts the two writes in order. The
  kernel keeps the run's threshold as the default for a later run that
  names none, a residue no user-space write can reach, so the guide page
  states it (spec D-8, FR-036).
- **R-5**: `mode`, `window` and `width` stand mode `0640`. A non-root read
  fails, and the run answers the not-available shape of spec FR-023 with
  status 0, because the feature owns no root refusal, so the saved set
  never stands incomplete. A test plants an unreadable knob and expects
  that shape.
- **R-6**: coverage cannot reach a real root path in the suite. The
  fakes carry every branch: a refused open, a short read, a directory in
  a file's place, a lowered file-size limit, and a truncated text. SC-005
  records the two real runs by hand on corsairone.
- **R-7**: the comparison's new head word `hwlat:` enters the rule
  grammar. A test plants the standing jitter names and asserts their
  refusals stay word for word, so the new branch cannot steal a name.
- **R-8**: `README.md` has three lines of headroom. The docs task counts
  the file after each edit, and SC-009 holds the cap.

### Logical view

The user-space method, one probe cpu:

```text
probe start     open msr node (root-only read)     NMI text read   record edge pair
     |
  gap loop  ---- gap above threshold? ---- no --> continue
     |                    |
    yes                   |
     +--------- read SMI count, keep change with the gap
     |
probe stop      NMI text read   record edge pair
     |
  fold          window delta = edge pair change
                marked gap count = changes above zero
                three figures onto the cpu's JitterStats
```

The detector engine, one run:

```text
setup    read current_tracer, tracing_cpumask, hwlat_detector/mode,
         width, window, tracing_thresh
         write tracing_cpumask           (kernel reads it at the start)
         write hwlat_detector/mode=per-cpu
         write hwlat_detector/width      (kernel bound: width < window)
         write hwlat_detector/window
         write tracing_thresh            (nanoseconds)
         write current_tracer=hwlat      (kernel takes its save set here)
         write tracing_on=1              (kernel starts the sampling)
   run    caller's pre-check span
  read    per_cpu/cpu<N>/trace lines    max gap, gap count, NMI count
restore  tracing_on=0
         write back every saved value, last written first
         state one line per file whose write failed
```

Both paths answer `not available` and status 0 when a source stands
missing, unreadable or malformed (spec FR-011, FR-023).

### Physical view

- One new library unit pair `source/harness/latency_counters.cpp` plus
  `source/harness/detail/latency_counters.hpp`, inside the `speedgun-ng`
  library target. The glob picks the source up with no CMake edit.
- The probe unit and the tracer unit call the new unit. No other unit
  reaches a kernel interface directly, and the file-root seam stays the
  one door for every path.
- Three new test executables join `test/CMakeLists.txt`; one new
  `report_shape` entry joins the example fold.
- No new link dependency, no new installed header beyond the standing
  public header, and no new installed file.

### Public API surface added

`include/speedgun-ng/benchmark.hpp`:

- `JitterMethod::HWLAT`, the fourth method word. The document word reads
  `HWLAT`, the console word and the command-line word read `hwlat`.
- `TracerEngine::HWLAT`, the third engine word, written `hwlat`.
- On `JitterStats`: `smiDelta` and `nmiDelta` as
  `std::optional<std::uint64_t>`, and `smiGapCount` as
  `std::uint64_t`.
- On `PrecheckTracerRecord`: `windowUs`, `widthUs` and `thresholdUs` as
  `std::optional<std::uint64_t>`.
- On `PrecheckTracerCpuRecord`: `maxGapNs`, `gapCount` and `nmiCount` as
  `std::optional<std::uint64_t>`.

Eleven document keys: `smiDelta`, `nmiDelta` and `smiGapCount` in
`jitter.stats[]` and in `precheck.cpus[].stats`; `smi` in `gaps[]`;
`windowUs`, `widthUs` and `thresholdUs` in `precheck.tracer`; `maxGapNs`,
`gapCount` and `nmiCount` in `precheck.tracer.cpus[]`; and `limits` in
`jitter`.

### Version and SOVERSION

- Version `0.31.0` to `0.32.0` (spec D-5).
- Harness `SOVERSION` `21` to `22`: `JitterStats`,
  `PrecheckTracerRecord` and `PrecheckTracerCpuRecord` change layout.
- Archive `SOVERSION` stays `6`: no archive type changes, and the readers
  live in the harness.
- Schema `1.14` to `1.15`: keys are added, no key changes kind or name,
  and no old document loses a key.

## Test Plan

Execution mode: test-driven, per constitution III. Each implementation
task carries the test that names its requirement, and the pair lands in
one commit (spec FR-035). The integrator records the red run of the test
before the implementation task turns it green, and records the green run
in the task's result line.

Seams the tests use, all standing:

- `sg::detail::installTestProcRoot` for every file read, fake tree under
  a temporary directory.
- `sg::detail::installTestReader` and the scripted `WindowReader` for
  window time.
- `sg::counters::FakeProvider` for the counters.
- `ProcRootGuard` and `TracingRootGuard` to scope a fake tree.
- `denyTracingPath` and a lowered `RLIMIT_FSIZE` for a refused open and a
  refused write.
- A forked child for the signal path, the way the standing tracer test
  carries it.

No test sleeps, no test reads a wall clock, and no test names a cpu that
the host lacks (spec FR-032, FR-033).

Fake tree the new tests build, under one root:

```text
proc/cpuinfo                              vendor_id line
proc/interrupts                           NMI row with per-cpu columns
dev/cpu/3/msr                             8-byte little-endian counter at 0x34
sys/kernel/tracing/current_tracer         nop
sys/kernel/tracing/tracing_cpumask        ffffffff
sys/kernel/tracing/tracing_thresh         10000
sys/kernel/tracing/hwlat_detector/mode    round-robin
sys/kernel/tracing/hwlat_detector/width   500000
sys/kernel/tracing/hwlat_detector/window  1000000
sys/kernel/tracing/per_cpu/cpu3/trace     header plus #1 inner/outer lines
```

### Requirement coverage

| Requirement | Module | Executables and gates |
| --- | --- | --- |
| FR-001 | M1, M4 | `harness_latency_counters_test`, `harness_hwlat_tracer_test` |
| FR-002 | M2, M3 | `harness_jitter_cli_test`, `harness_hwlat_probe_test` |
| FR-003 | M4, M10 | `harness_hwlat_tracer_test`, `harness_noise_test` |
| FR-004 | M5, M6, M7, M8 | `harness_json_report_test`, `harness_json_load_test`, `harness_noise_report_test`, `harness_compare_test` |
| FR-005 | M3, M4 | `harness_jitter_cli_test`, `harness_hwlat_tracer_test` |
| FR-006 | M9 | prose gate, drift gate, `report_shape` |
| FR-007 | M1 | `harness_latency_counters_test` |
| FR-008 | M1, M3 | `harness_latency_counters_test`, `harness_hwlat_probe_test` |
| FR-009 | M1 | `harness_latency_counters_test` |
| FR-010 | M1 | `harness_latency_counters_test` |
| FR-011 | M1, M3, M4 | the three new executables |
| FR-012 | M3 | `harness_hwlat_probe_test`, `harness_jitter_probe_alloc_test` |
| FR-013 | M3 | `harness_hwlat_probe_test` |
| FR-014 | M3 | `harness_hwlat_probe_test` |
| FR-015 | M3 | `harness_hwlat_probe_test` |
| FR-016 | M3, M5, M7 | `harness_hwlat_probe_test`, `harness_json_report_test`, `harness_jitter_window_test` |
| FR-017 | M3 | `harness_hwlat_probe_test` |
| FR-018 | M4 | `harness_hwlat_tracer_test`, `harness_jitter_cli_test` |
| FR-019 | M4 | `harness_hwlat_tracer_test` |
| FR-020 | M4 | `harness_hwlat_tracer_test` |
| FR-021 | M4 | `harness_hwlat_tracer_test`, `harness_noise_test` |
| FR-022 | M4, M10 | `harness_hwlat_tracer_test` |
| FR-023 | M4 | `harness_hwlat_tracer_test` |
| FR-024 | M10 | the gate record in `specs/048-hwlat-probe/results/` |
| FR-025 | M2, M5 | `harness_json_report_test` |
| FR-026 | M5, M6 | `harness_json_report_test`, `harness_json_schema_test`, `harness_json_load_test` |
| FR-027 | M2, M4, M5, M7 | `harness_noise_report_test`, `harness_json_report_test` |
| FR-028 | M8 | `harness_compare_test`, `harness_compare_cli_test` |
| FR-029 | M5 | `harness_json_schema_test` |
| FR-030 | M7 | `harness_jitter_window_test`, `harness_noise_report_test` |
| FR-031 | M7, M10 | `harness_jitter_window_test` |
| FR-032 | all test work | every run on an unprivileged host |
| FR-033 | all test work | `test/time_source_gate.sh` |
| FR-034 | M3, M4, M5 | the SC-001 count in the task results |
| FR-035 | M10 | the commit plan and the gate record |
| FR-036 | M9 | drift gate |
| FR-037 | M9 | `report_shape`, drift gate |
| FR-038 | M10 | the gate record |
| FR-039 | M9 | spell gate, prose gate |

### Scenario coverage

- SC-001: `harness_hwlat_probe_test` carries one scenario per attribution
  case, one per not-available case, and one per signal path.
- SC-002: the coverage run names the line and branch figures, with the
  branch shortfall stated and no new exclusion marker.
- SC-003: the full suite, then the pinned run
  `taskset -c 0,1 ctest --preset prepush-quick -j 2`.
- SC-004: the gate record lists each gate with its exit code.
- SC-005: two root runs on corsairone, recorded under `results/`.
- SC-006: `harness_jitter_bytes_test` compares the bytes of a run that
  names neither new word against the master document.
- SC-007: the recorded detector run lists each file with its before value
  and its after value.
- SC-008: `harness_hwlat_probe_test` counts the marked gaps and the gaps
  whose change stands above zero in the same scenario.
- SC-009: the docs task reports the `README.md` line count.
- SC-010: the drift gate and the guide page sections.

## Constitution Check (post-design re-check)

No gate gained a violation in design. Two records stand open for the
implement phase, both inherited and neither widened here: the restore
defect this lane fixes for the shared lifecycle (spec D-15, recorded
against spec `036`), and the standing engine figure path
`sys/kernel/tracing/osnoise/cpu<id>`, which names no file the kernel
creates. The new engine reads `sys/kernel/tracing/per_cpu/cpu<id>/trace`,
which the kernel does create, and the lane records the standing path as
an observation for spec `036` without changing it.

## Complexity Tracking

| Item | Reading | Source |
| --- | --- | --- |
| Branch coverage shortfall | at most 21 uncovered branch rows, with line coverage at 100% and the exclusion-marker count at or under 84 | standing allowance at `tools/dbc/coverage_gate.sh:102`, the ceiling `coverage_gate.sh:45` checks; spec SC-002, FR-038 |

No other gate needs an exception.
