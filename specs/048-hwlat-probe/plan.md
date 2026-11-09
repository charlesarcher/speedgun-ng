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
with status 0. The schema moves to `1.16` with eleven new keys, the loader
reads the six figures back, the comparison gains six rule names, and the
harness `SOVERSION` rises from `21` to `22` with the version move to
`0.34.0`.

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

**Storage**: none of its own. The run reads three kernel interfaces and
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
| M9 | Contracts, docs, example | contracts, guide page, `README.md`, example pair, `example/CMakeLists.txt`, the nine standing contract files of specs `026`, `027`, `034` and `036` | M3, M5, M7 | FR-006, FR-036 to FR-039 | the gates |
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
- **R-3**: the two detector knobs carry the kernel's own bounds, which
  refuse a value below the minimum or above the maximum and accept a
  width equal to the window (`kernel/trace/trace.c:6728` to `:6732`).
  The lane's own stricter bound stands in spec FR-020: `width` stands
  under `window`. The lane writes width before window, so a write of a
  width above the standing window cannot fail. A test plants the standing
  values and asserts the write order.
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

- Version `0.33.0` to `0.34.0` (spec D-5): the `0.32.0` this branch named
  and the `0.33.0` that followed stand taken on master.
- Harness `SOVERSION` `21` to `22`: `JitterStats`,
  `PrecheckTracerRecord` and `PrecheckTracerCpuRecord` change layout.
- Archive `SOVERSION` stands at `8`, where specs/050-limit-table-lifecycle
  left it: no archive type changes here, and the readers live in the harness.
- Schema `1.15` to `1.16`: keys are added, no key changes kind or name,
  and no old document loses a key. This branch cut at `1.14` and named `1.15`
  for its own move, which specs/041-counter-probe landed on master first, so
  the eleven keys take the next free minor.

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
sys/kernel/tracing/hwlat_detector/mode    none [round-robin] per-cpu
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

- SC-001: `harness_hwlat_probe_test` carries one test per attribution
  case, one per counter not-available case, and one per gap edge case.
  `harness_hwlat_tracer_test` carries the tracefs not-available cases
  and the four restore paths, the split the coverage table gives FR-019
  to FR-023.
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
| Branch coverage shortfall | at most 21 uncovered branch rows, with line coverage at 100% and the exclusion-marker count at or under 84 | the standing allowance at `tools/dbc/coverage_gate.sh:102`, the 100% line rule at `coverage_gate.sh:45`; the marker count is a lane reading under spec SC-002, taken by task T079's grep, since no tool checks it |

No other gate needs an exception.


---

## Acceptance plan: Intel final acceptance (2026-11-07)

**Branch**: `docs/intel-final-acceptance` | **Date**: 2026-11-07 |
**Spec**: [spec.md](./spec.md), section `Acceptance extension: Intel final
acceptance (2026-11-07)`

### Summary

The branch measures, then writes. It runs the standing acceptance commands on this
Intel host, keeps every raw capture outside the repository, and writes three records
that carry the command, the exit code and the output behind each claim. A row with no
output names the right it lacks and carries no figure.

### Technical context

- **Host**: corsairone, Intel Core i9-9920X (Skylake-X), one socket, 24 cpus, affinity
  and online set 0-23, `perf_event_paranoid` -1, `nmi_watchdog` 0.
- **Toolchain**: gcc 16.2.1, clang 23.1.3, gcov 16.2.1, lcov 2.5.0-beta,
  clang-tidy, cppcheck, doxygen, `/usr/bin/python3` with PyYAML, ccache.
- **Trees**: `build` (ci-rocky, the plain suite and the install rows),
  `build/coverage` (ci-coverage), `build/sanitize` and `build/tsan` (CI compiler
  `clang++`), `build/consumer-release` and `build/consumer-release-dev`
  (ci-linux-ignore), and `build/analyzer` for the static-analysis scopes. Every build
  directory stays under `build`, so no lane adds a path the tracked set can see.
- **Rights**: no root. `sudo` asks for a password, tracefs stands `0700 root` and the
  `msr` nodes stand `0600 root`. No escalation, no password, no sudoers change, no
  module load, no permission change.
- **Quiet checkout**: from the first `ctest` start of a sweep to its last `ctest` exit,
  the checkout that sweep's fixtures read holds no new change to a tracked file and no
  index change, beside the nine acceptance-artifact modifications this lane owns and
  states in the record. `prose_gate_fixtures` compares a gate subprocess scan
  with its own `git ls-files` walk of the same checkout, so `git add`, `git stash`,
  `git checkout` and a commit disturb it as much as a file write.

### Order of work

1. Part A: the standing reads and the host facts (IA48-001), then the unprivileged
   hwlat surfaces, each alone (IA48-002).
2. Part A root rows, each inside a session an operator approves and blocked in place of
   one: the two quickstart root commands, the six-value snapshot with `tracing_on` read
   beside it, and the controlled interruption with its own signal command
   (IA48-003 to IA48-005).
3. Part B: the two hunt commands at the measured head, three runs each, alone
   (IA46-001, IA46-002).
4. Part C: the counter-count pair on the Intel core device, alone, and the watchdog and
   device reads beside it (IA41-001, IA41-002).
5. The coverage chain in `build/coverage` (IA-END-001.1), in this order: configure,
   build, zero the counters, the `-j 1` suite, the single-entry re-run of any entry the
   suite cut, the `coverage` target, then the gate by hand. The re-run stands before the
   target so the trace holds it.
6. The instrumented lanes, one at a time (IA-END-001.2 to .6): the address and undefined
   lane, the thread sanitizer lane, the plain suite in the CI form, the two-cpu lane, the
   four-cpu lane. Each prints its `ctest -N` entry count and its load witness before it
   starts.
7. The static-analysis scopes and the remaining gates (IA-END-002.1 to .8): the
   `build/analyzer` scopes, the release build, the install rows and their audits, format,
   spell, drift in both forms, time source, the DBC gates, the pmu and host-record
   checks, the two counts, and the whole consumer-release job.
8. The records and the quickstart correction, after the last sweep exits
   (IA-END-003.1 to .3), then the spell gate again over the written prose, then the prose
   gate in tree form with the records visible to `git ls-files`, then one commit, then
   the squash, then the first push.
9. After the squash: the prose gate in range form from the base sha to the squashed
   head, then the pull request, then the base-tree run for any red name no tracked
   record carries (IA-END-001.8).

Windows. Steps 1 to 4 hold no other step: no second `ctest`, no build, and no hand
re-run runs inside a sweep window, and part B and part C hold no process that opens a
counter group, because the suite itself holds counter groups. A step that starts a
compiler in step 7 begins only after the last `ctest` of step 6 exits. The text gates -
format, spell, both drift forms, time source, the two counts, and the hand gate on a
finished tracefile - carry no window of their own, because none of them reads a duration
or starts a process.

Names that decide the windows: `counters_clock_raw_test` reserves all 24 processor
slots, and `base_cli_matrix`, `harness_hunt_cli_test`, `harness_jitter_cli_test` and
`example_run` are the entries a shared host pushes past its ceiling. ctest runs an entry
whose slot demand exceeds `-j` alone rather than skipping it, so a small `-j` does not
protect anything.

Load witness and stops. Before each timed step the lane prints `cat /proc/loadavg`,
the cpu affinity and a count of other `ctest` and compiler processes. A timed step that
starts on a host carrying another sweep's load is marked not run with that reading, not
run and recorded red. Two further stops bind: the privileged row stops where
`current_tracer` reads other than `nop` or `tracing_on` reads 1, and an analyzer scope
whose translation-unit count is zero is void and re-runs in a fresh tree.

Before any red enters a record, the lane re-runs that name alone in the same tree and
prints, for both readings: the ceiling as ctest states it, from `ctest --test-dir <tree> --show-only=json-v1`
on its own line, the elapsed time, the `-j`, the cpu mask and `cat /proc/loadavg`.
Only a line ctest prints as a timeout counts as a load candidate. A single-entry re-run
runs through ctest or from inside its own build directory, never from the repository
root, because several entries write by relative path. The base run in a second tree
stands after the last timed step, and only for a red name no tracked record carries.

### Command table

Each row is the string the lane runs, copied out of the tracked file named beside it.
Rows whose string is too long for a cell stand in the fenced blocks below the table.

| Row | Command | Source | Exit that decides |
|---|---|---|---|
| Release build | `cmake --preset=ci-rocky` then `cmake --build build --config Release -j "$(nproc)"` | ci.yml ci-rocky pair | each |
| Entry count | `ctest --test-dir build -N` | rule 24 | ctest |
| Plain suite | `ctest --test-dir build --output-on-failure --no-tests=error -C Release -j "$(nproc)"` | ci.yml test job, with `--test-dir` for its working directory | ctest |
| Two-cpu lane | `taskset -c 0,1 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 2` | the row above, masked | ctest |
| Four-cpu lane | `taskset -c 0,1,2,3 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 4` | the row above, masked | ctest |
| Install and audits | block A | ci.yml install steps | each audit |
| Coverage build | `cmake --preset=ci-coverage` then `cmake --build build/coverage -j "$(nproc)"` | ci.yml coverage job | each |
| Coverage zero | `lcov --zerocounters -d build/coverage`, a local step CI does not need | rule 22 | lcov |
| Coverage suite | `ctest -j 1 --output-on-failure --no-tests=error` in `build/coverage` | ci.yml coverage job | ctest |
| Coverage target | `cmake --build build/coverage -t coverage` | ci.yml coverage job | make |
| Coverage gate | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | coverage_gate.sh; cmake/coverage.cmake | the gate |
| Address and undefined | block B | ci.yml sanitizer job | ctest |
| Thread sanitizer | block C | ci.yml tsan job | ctest |
| Format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | ci.yml lint job | cmake |
| Spell | `cmake -P cmake/spell.cmake` | ci.yml lint job | cmake |
| Drift | `python3 tools/docs/example_drift.py .` then `python3 tools/docs/example_drift.py --sections --names --options --forbidden .` | ci.yml lint job; 046 quickstart | script |
| Time source | `bash test/time_source_gate.sh .` | ci.yml lint job | script |
| Prose, tree | `cmake -D PROSE_MODE=tree -D PROSE_CHECK=prose -P cmake/prose-lint.cmake` | prose-lint.cmake defaults | script |
| Prose, range | `cmake -D PROSE_MODE=range -D PROSE_BASE=<base-sha> -D PROSE_HEAD=<head-sha> -P cmake/prose-lint.cmake` | ci.yml prose job | script |
| DBC | block D | ci.yml dbc job; dbc-gate.cmake | four exits |
| Analyzer | block E | ci.yml analyzer job | the finding count |
| Consumer release | block F | ci.yml consumer-release job | the asserts |
| pmu events | `python3 tools/pmu_events/update_pmu_events.py --check` | ci.yml test job | script |
| Host records | `bash tools/ci/host_record_gate.sh build/test/host_record_check . validation` | ci.yml test job | script |
| README and markers | `wc -l README.md`, then `git grep -c LCOV_EXCL -- source include \| awk -F: '{s+=$NF} END {print s}'` | 045 close-rows record | the two counts |
| Hunt, walk | `./build/example/memory_walk --hunt --hunt-budget 120s`, console captured by redirection to a file outside the repository | 046 acceptance record | the binary |
| Hunt, benchmark | `./build/example/first_benchmark --hunt --hunt-budget 120s`, console captured by redirection | 046 acceptance record | the binary |
| Counter count | `./build/example/docs_counter_probe` | 041 tasks row; 041 acceptance record | the binary |
| Hwlat method | `./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact --jitter-probe=idle,method=hwlat --json=<capture>` | 048 quickstart, without its `sudo` prefix | the binary |
| Hwlat engine, unprivileged | `./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=<capture>` | 048 quickstart, without its `sudo` prefix | the binary |
| Hwlat example | `./build/example/docs_hwlat_probe` | 048 example target | the binary |
| Hwlat root runs | the two `quickstart.md` runs of lines 173 and 175, inside a root session or blocked | 048 quickstart | the operator |
| Round trip | `./build/example/first_benchmark --compare=<a>,<b> --compare-rule="jitter:run:any:smiGapCount:lower:abs=0"` and the same pair with `--compare-rule="hwlat:any:maxGapNs:lower:abs=200000"` | rule 9; 048 quickstart | the binary |

### Block A - install and the thirteen audits

```
cmake --install build --config Release --prefix prefix
```

then the twelve dependency audits and the standalone example link-manifest audit of the
CI test job, in the CI order, each with its own exit.

### Block B - address and undefined sanitizer

```
CXX=clang++ cmake --preset=ci-sanitize
cmake --build build/sanitize -j "$(nproc)"
ASAN_OPTIONS="strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1" UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j "$(nproc)"
```

CI runs its ctest step with `working-directory: build/sanitize`; `--test-dir` carries the
same directory here, and the option strings copy byte for byte.

### Block C - thread sanitizer

```
CXX=clang++ cmake --preset=ci-tsan
cmake --build build/tsan -j "$(nproc)"
TSAN_OPTIONS="halt_on_error=1:second_deadlock_stack=1" ctest --test-dir build/tsan --output-on-failure --no-tests=error -j "$(nproc)"
```

The same `--test-dir` reading as block B, and the same byte-for-byte option copy.

### Block D - the DBC gates

```
cmake --build build --target dbc-gate
doxygen build/dbc-gate/Doxyfile
/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml --registry tools/dbc/macros.yaml --out /tmp/dbc-doc.json
/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools --registry tools/dbc/macros.yaml --doc-matrix /tmp/dbc-doc.json --out /tmp/dbc-pair.json
```

The `--src2` word takes both `source` and `tools`; a copy that drops one narrows the
scan and still exits 0. The target runs in the plain tree at the measured head, where CI
uses `build/dev`, and the record names that difference.

### Block E - the static-analysis scopes

```
cmake --preset=ci-ubuntu -B build/analyzer -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
cmake --build build/analyzer --target speedgun-ng_speedgun-ng speedgun-ng_harness -j "$(nproc)"
cmake --build build/analyzer/test -j "$(nproc)"
cmake --build build/analyzer/example -j "$(nproc)"
```

Each build is captured to a log, then CI's own steps run over it unchanged: the
translation-unit count, the two finding extractors, the `LC_ALL=C sort -u`, and the loop
that reads `WarningsAsErrors` out of `.clang-tidy` and counts hits on each check name.

### Block F - the consumer-release job

The job runs as `.github/workflows/ci.yml` writes it, step by step, in both trees: the
two configures, the one cache-assert step with its three greps, the library build, the
archive symbol check, the four-target trap and `dbc_test` build, the trap runs under
`ulimit -c 0` and `timeout 30`, the checked-test build, its `ctest -R counters_trap` run,
and the `dbc_test` run.

### Gate notes

- The `coverage` target chains five commands, so its exit is make's. The gate run by
  hand carries the verdict, and only 0 is green: 1 a measured shortfall, 2 a usage
  fault, 3 a tool fault.
- The allowance of 21 counts the CI union. The record quotes the gate's own line for the
  host and for the base, labels every derived difference, and lists the cold rows of
  head and base, because the claim that this branch adds no new shortfall stands on the
  set of cold rows and not on two totals.
- The analyzer needs a fresh tree: a reconfigure of a built tree recompiles nothing, and
  an empty finding list then proves nothing. CI needs no launcher clear because its ccache
  stands cold; the host clears both launchers so a cache hit cannot stand in for an
  analyzer pass. The proof is the translation-unit count per
  scope, zero hits on the gate checks, and the statement that this branch owns no
  translation unit, so the expected count for owned files is zero by that fact alone.
- The trap fixtures carry mixed exits and CI also requires both trap exits to stand
  clear of 124: non-zero and not 124 for `dbc_trap_fixture`, 1 for `dbc_test`, non-zero
  and not 124 for the `overrun` mode, and 0 for the nine other modes of
  `counters_trap_fixture`. 124 is a kill, never a pass. Each expected exit stands inside
  the row as the pass condition.
- The DBC row carries four exits and the interface count: the target, the doxygen pass,
  the document gate and the pair gate, each naming its interpreter and whether that
  interpreter carries yaml. A target that fails for a missing module is a host fact, not
  a gate gap.
- `dbc_gate_fixtures` and `prose_gate_fixtures` start a bare `python3`, so the record
  names the `python3` of the suite run beside the suite's counts.
- ctest reads the lesser of an entry's `TIMEOUT` property and its own `--timeout`, so a
  larger `--timeout` cannot lift a ceiling and a smaller one binds; the record quotes the
  ceiling from `--show-only=json-v1` instead of restating a count of timed rows.
- The marker count is read over `source include` and summed, and decides against the 84
  the standing records carry for that set. The row names the directories it counts.
- The lane names the interpreter and the compiler for each gate. It disables no check.

### Risks

| Risk | Effect | Action |
|---|---|---|
| No root session | The two root rows and the interruption row stay blocked, and SC-005 and SC-007 stay unmet | Name the operator action, keep the rows blocked, continue the rest |
| A machine load crosses a ceiling | A suite entry times out and a false red enters the record | Load witness before each timed step, one sweep per window, re-run each red name alone and print both readings |
| A coverage sweep cannot finish | The union stands short | Re-run each cut entry alone before the target, name the entry, the command, the elapsed time and the missing trace, and claim no partial union |
| A tracked write or a staged index lands inside a sweep window | A prose fixture red that means nothing | No record write, no `git add` and no commit from the first `ctest` start to the last `ctest` exit, then re-run the fixture alone |
| A tree-form prose run sees no record | A clean mark over files the gate never read | Run the tree form after the record writes, with the records visible to `git ls-files`, and print the unit count |
| A timeout kill reads as an expected failure | A pass that never happened | Copy CI's not-124 condition into each trap row |
| A records-only amend moves the head | A pre-squash reading could read as final | Disclose both shas and the diff, re-run the decisive lanes at the final head, and re-run the range prose form |
| A red name stands outside every tracked record | No base evidence for it | Run the base tree after the last timed step, and put the base sha, the tree, the command and the exit beside it |

## Plan: Acceptance closeout, measured Intel evidence (2026-11-08)

**Branch**: `fix/intel-acceptance-closeout` | **Date**: 2026-11-08 |
**Spec**: [spec.md](./spec.md), section `Acceptance closeout: measured Intel
evidence (2026-11-08)`, with `IH-001` and `IH-002` of
[specs/046-hunter-topdown/spec.md](../046-hunter-topdown/spec.md) and `IP-001`
and `IP-002` of [specs/041-counter-probe/spec.md](../041-counter-probe/spec.md)

The companion sections stand in
[../046-hunter-topdown/plan.md](../046-hunter-topdown/plan.md) as
`Acceptance closeout: measured Intel evidence (2026-11-08) - the hunter part`
and in [../041-counter-probe/plan.md](../041-counter-probe/plan.md) as
`... - the counter-count part`. This section holds the shared context, the
shared order of work and the lane table. A companion section names only the
rows its own record carries, and it repeats no command from this table.

### Technical Context

- **Base and head**: base `2d4d8c57975a86db6c77536f7090aba2bdaf6417`, and
  `origin/master` at the same head (`/tmp/ic/base/identity.log`, and the
  identity table of the closeout spec). This section measured that base on
  2026-11-08. The phase now re-measures on `a55728af`, the base `Intel
  acceptance closeout at the merged head (2026-11-08)` of
  `specs/048-hwlat-probe/spec.md` names, and every figure below keeps the head
  whose run printed it. Pull request #98 measured
  `87d745d6c7e62d4da80198c689ddc7078f08ff3d`, and its counts stand as leads.
  The working tree already carries the branch named above
  (`/tmp/ic/base/identity.log`).
- **Host**: corsairone, `uname -srmo` `Linux 7.3.0-4-cachyos x86_64`
  (`/tmp/ic/w2-probe.md`, section 8). Intel Core i9-9920X (Skylake-X),
  non-hybrid, one socket, 24 cpus (`/tmp/ic/base/identity.log` answers
  `nproc` 24, online `0-23`, present `0-23`), affinity list `0-23`, so cpus 0
  and 1 stand inside the set the host allows.
- **Kernel access**: `perf_event_paranoid` -1, `nmi_watchdog` 0,
  `/sys/kernel/tracing` `drwx------ 9 root root`, `/dev/cpu/0/msr`
  `crw------- 1 root root 202, 0` (`/tmp/ic/base/identity.log`, read
  2026-11-08T14:09:33-06:00).
- **Two blockers**: a privileged session, and one perl module. `sudo -n -l`
  answers `sudo: a password is required` and `pkexec
  --disable-internal-agent id` answers `Error executing command as another
  user: No authentication agent found.` (`/tmp/ic/base/identity.log`), so
  tracefs and the `msr` nodes stand out of reach. `cmake --build
  build/coverage -t coverage` answers `ERROR: required module GD.pm not found
  on this system (see www.cpan.org).` at its last step and exits 2, one step
  after the gate that exits 0
  (`/tmp/ia3/cov/cov-target.log`, 13:24:18 to 14:08:50).
- **Toolchain**: g++ (GCC) 16.2.1 20260810, clang 23.1.3, gcov (GCC) 16.2.1,
  lcov 2.5.0-beta, cmake 4.4.2, ccache 4.11.3, doxygen
  `$HOME/.local/bin/doxygen` (`/tmp/ic/base/identity.log`).
- **Interpreters**: a bare `python3` answers `/usr/bin/python3`, CPython
  3.14.8, with `yaml` 6.0.3 (`/tmp/ic/base/identity.log`). Every tree records
  `Python3_EXECUTABLE` `$HOME/.local/bin/python3.11`, CPython 3.11.16
  (`/tmp/ic/base/san-cfg.log`, `Found Python3:`), and that interpreter holds
  no PyYAML (`results/acceptance-final-intel.md:218`). `dbc-gate` is the only
  gate that reads the recorded entry, and the route of ruling 22 names
  `/usr/bin/python3` for that tree at configure time
  (`/tmp/ic/w5-gates.md`, sections 1 and 4a).
- **Trees**: `build` (ci-rocky), `build/coverage` (ci-coverage),
  `build/sanitize` (ci-sanitize with `CXX=clang++`), `build/tsan` (ci-tsan
  with `CXX=clang++`), `build/dev` (ci-ubuntu, the DBC target) and
  `build/analyzer` (ci-ubuntu with both launcher entries cleared). Each tree
  except `build` sits inside `build`, so the release lane runs last and wipes
  last, and `build/dev` and
  `build/analyzer` configure after that wipe (ruling 2).
- **Measured at the base head**: the address and undefined lane holds 185
  entries (`/tmp/ic/base/san-count.log`), 183 passed, `base_cli_matrix` reads
  `***Timeout 120.04 sec`, `example_run` reads `***Timeout 300.04 sec`, the
  run totals 538.87 s of real time and ctest exits 8. Inside that lane
  `harness_hunt_cli_test` passed at 288.89 s and `harness_jitter_cli_test`
  passed at 180.67 s (`/tmp/ic/base/san-test.log`). The thread lane holds 186
  entries (`/tmp/ic/base/tsan-count.log`), 181 passed, and ctest exits 8 in
  702.64 s: `base_cli_matrix` reads `***Timeout 120.02 sec`,
  `report_shape_fwq_ftq_sample` reads `***Failed 110.27 sec`,
  `harness_hunt_cli_test` reads `***Timeout 300.06 sec`, `example_run` reads
  `***Timeout 300.03 sec` and `harness_jitter_cli_test` reads `***Timeout
  300.13 sec` (`/tmp/ic/base/tsan-test.log`). The coverage chain, a full chain
  recorded by pull request #99 at `5fe3d244`, a docs-only descendant of
  `2d4d8c57` whose two tracked files are `specs/048-hwlat-probe/
  results/acceptance-final-intel.md` and `specs/048-hwlat-probe/tasks.md`, on this host
  from 11:38 to 14:08 local, holds 185
  entries, cuts four of them, totals 3978.64 s and exits 8
  (`/tmp/ia3/cov/cov-test.log`); the four hand re-runs of that chain, printed at
  `5fe3d244`, each exit 0, at computed 285 s, 509 s, 571 s and 775 s, read from
  the start and end stamps of `/tmp/ia3/cov/hand-base_cli_matrix.log`,
  `hand-harness_jitter_cli_test.log`, `hand-harness_hunt_cli_test.log` and
  `hand-example_run.log`. The gate exits
  0 with lines 100.0% (18664 of 18664), functions 99.0% (1958 of 1978),
  branches 99.8% (9253 of 9273) and the line `coverage-gate: branch shortfall
  20 rows, within the  D-14 recorded allowance of 21 (accepted classes)`
  (`/tmp/ia3/cov/cov-gate.log`).
- **Page and marker caps, read at the base head**: `wc -l README.md` answers
  1002 against `PAGE_LINE_CAP = 1004` (`tools/docs/example_drift.py:256`), and
  a sum of `LCOV_EXCL` over `source include` answers 84 against the cap of 84.
- **Deferred cause with its evidence in hand**: `source/harness/cli.cpp:707-710`
  offers a `PmuProvider` before `:717` scans the command line, and the comment
  at `:702-706` states the construction walk at a measured 1.35 to 1.72 s per
  call. At `perf_event_paranoid` -1 the walk settles on about two
  `perf_event_open` calls per catalog entry (`/tmp/ic/w4-instrumented.md`,
  section 0). `source/counters/system.cpp:230-246` then refuses that provider
  as a `duplicate object path` in a scripted fixture, so the walk lands and the
  catalog never registers. Child counts: `base_cli_matrix` 26, `example_run` 74
  of which 37 are `--list` launches (`test/example_run.sh:48`),
  `harness_hunt_cli_test` 157 or more, `harness_jitter_cli_test` 45 or more
  (`/tmp/ic/w4-instrumented.md`, sections 1 to 4). That walk now carries a
  price on this host. In the release tree `docs_labels --list` and `--help`
  each answer exit 0 between 1.67 and 1.85 s over three runs, and in the
  address tree `--list`, `--help` and `--version` each answer exit 0 between
  2.19 and 2.39 s over three runs, with the load witness reading 0.34 0.91 1.61
  and 0.59 0.94 1.61 (`/tmp/ic/base/walk-price.log`). A launch that only prints
  pays for the walk, so the child counts price the two instrumented reds: 26
  launches cost 43.4 to 48.1 s of the 120 s ceiling in the release tree and
  56.9 to 62.1 s of it in the address tree, and 74 launches cost 123.6 to 136.9
  s of the 300 s ceiling in the release tree and 162.1 to 176.9 s of it in the
  address tree. Both ranges are computed from that log and those counts.

### Constitution Check

| Principle | What it requires in this phase | State |
|---|---|---|
| I. Standard-first (P0 to P3) | No critical-path code enters, so no P0 technique is invoked. P1 governs the record shape and the commit template. No P2 exception is registered or needed. P3 tools re-run over unchanged units. | Clear: the phase edits no `.cpp` or header |
| II. Design By Contract | The DBC gate that checks every contract re-runs unchanged, and a target that fails for a missing module is a host fact with its own row. No interface appears. | Clear, and the gate row carries four exits plus the interface count |
| III. R-DCUT | Requirements exist as IC, IH and IP clauses, this plan lands in the three `plan.md` files, and tasks land in the three `tasks.md` files. No code arrives, so no unit test arrives with it. TDD stands unused, and III asks `plan.md` to say so. | Clear |
| VI. Tests and coverage | The 100% line and branch gates re-run with their commands as tracked, no test drops, and no exclusion enters. VI asks a bug fix for a reproducing test, and ruling 23 leaves no defect for one to reproduce. | Clear: the coverage chain runs as tracked, and no entry is dropped |
| VII. Performance discipline | A hardware row needs noise-free execution: one lane at a time, a load witness printed, three consoles per command, and rank spread reported as a spread. A figure taken beside a second lane stands void. | Binding: ruling 17, phase 1 and phase 7 |
| VIII. CI quality gates | Every hard gate re-runs at the final head: builds, `ctest`, sanitizers, thread sanitizer, clang-tidy and cppcheck scopes, format, spell, prose, drift, time source, DBC, coverage, the two counts. No gate weakens, and no exception is taken. | Binding: phase 6, and no ceiling moves |
| IX. Spec-driven development | The phase lives in `specs/048-hwlat-probe`, `specs/046-hunter-topdown` and `specs/041-counter-probe` artifacts, and the checklist stands under `checklists/`. | Clear |
| X. Anti-slop | X.1 states each assumption beside the phase that uses it. X.3 keeps every changed line traceable to an IC, IH, IP or ruling clause and defers what lands outside the owned list. X.4 gives every phase a binary verify line. | Clear, and the deferred list names its reproduction |
| XI. Discourse and prose | Records, plan and commit prose hold no em-dash, no contrastive framing, no filler, and short active sentences. The tree gate runs after the writes and the range gate after the squash. | Binding: phase 4 and phase 7 |

### Project Structure

Files this phase edits, and what lands in each:

| File | What lands there |
|---|---|
| `specs/048-hwlat-probe/spec.md`, `specs/046-hunter-topdown/spec.md`, `specs/041-counter-probe/spec.md` | Already carry the closeout sections and the rulings. The phase changes them only where a run disproves a line, and it names that line. |
| `specs/048-hwlat-probe/plan.md` | This section. |
| `specs/046-hunter-topdown/plan.md`, `specs/041-counter-probe/plan.md` | The two companion sections named above. |
| `specs/048-hwlat-probe/tasks.md`, `specs/046-hunter-topdown/tasks.md`, `specs/041-counter-probe/tasks.md` | Task rows for the phases below, one or more per phase, each with its verify line, appended after the standing acceptance tasks, which end at `IA48-011` of `specs/048-hwlat-probe/tasks.md`, `IA46-003` of `specs/046-hunter-topdown/tasks.md` and `IA41-003` of `specs/041-counter-probe/tasks.md`. |
| `specs/048-hwlat-probe/checklists/intel-acceptance-closeout.md` | Stands written, and re-scored by this closeout on 2026-11-08. Five items carry wording that reads true against the section it scores, because an acceptance section names commands, paths and exits on purpose. It is re-scored again on 2026-11-09 for the citation form and for the two heads of ruling 29 of `Rulings - this phase (29 to 42)` of `specs/048-hwlat-probe/spec.md`, and it marks two new items. |
| `specs/048-hwlat-probe/results/acceptance-final-intel.md` | One dated section for the closeout head, with the corrected rows below listed here. |
| `specs/046-hunter-topdown/results/acceptance-final-intel.md`, `results/acceptance-hunt.md`, `results/pr-body.md` | The hunter rows below, each in the file that holds the claim. |
| `specs/041-counter-probe/results/acceptance-final-intel.md` | The counter-count rows below. `results/probe-window-trials.txt`, `results/requirement-1-shipped-shape.txt` and `results/acceptance.md` keep their text, and a dated row in that record names each line they hold that belongs to another host or to a tool copy outside the repository. |

Ruling 27 keeps history as history: a section the closeout must not rewrite
keeps its wording and its heading, and every new row prints its path as
`$HOME`-relative or build-relative.

Files the phase must not touch, with the reason a reader meets them:

| Excluded | Why it appears here |
|---|---|
| `source/**`, `include/**`, `example/**` | No acceptance row asks for a production change. The provider order at `source/harness/cli.cpp:707-710` is the largest single cost, and its fix lands here as a deferral. |
| `test/CMakeLists.txt` | It holds every ceiling: `:1014` grants `base_cli_matrix` 120 s, `:2278` grants `example_run` 300 s, `:2197` grants `report_shape_fwq_ftq_sample` 120 s, and `:2494-2499` grants 300 s to every name without one. No ceiling moves. |
| `test/source/detail/hunt_fixture.hpp` | Seven sources include it, and seven CTest entries run an including binary, `harness_hunt_json_test_no_hunt` at `test/CMakeLists.txt:1869` among them. Ruling 8 puts a shared header outside the list. |
| `test/base_cli/run_matrix.py`, `test/data/base_cli_matrix.txt`, `test/example_run.sh`, `test/source/harness_hunt_cli_test.cpp`, `test/source/harness_jitter_cli_test.cpp` | Owned by the spec's changed-file list and named by no diagnosis. Ruling 23 disproves the matrix reading, so no edit lands in any of them. |
| `tools/**`, including `tools/docs/report_shape.py` | `:368` holds the `timeout=110` grant inside `report_shape_fwq_ftq_sample`, tighter than the 120 s of `test/CMakeLists.txt:2197`. |
| `.github/**` and every gate script | `tools/dbc/coverage_gate.sh`, `cmake/coverage.cmake`, `cmake/dbc-gate.cmake`, `cmake/lint.cmake`, `cmake/spell.cmake`, `cmake/prose-lint.cmake`, `test/time_source_gate.sh` run as tracked. |
| `CMakePresets.json`, `docs/**`, `README.md`, validation and schema files | The caps and the pages stay as measured, and the drift gate reads them. |

Deferred list, each with the reproduction that names it:

1. The provider walk. Run `./build/example/first_benchmark --list` and time
   it, then read `source/harness/cli.cpp:702-710` beside `:717`. The count of
   children that pay it stands in Technical Context. Every row that crosses a
   ceiling for that reason stays open with this follow-up (ruling 23).
2. The scripted refusal. `source/counters/system.cpp:230-246` refuses a
   provider whose seed repeats an object path, and
   `test/source/detail/hunt_fixture.hpp:54,229-233` seeds `cpu` with
   `kHuntEvents` 200 (`:75`).
3. The 110 s grant. `tools/docs/report_shape.py:368` bounds the child of
   `report_shape_fwq_ftq_sample` inside the 120 s ceiling of
   `test/CMakeLists.txt:2197`.
4. The missing perl module. `GD.pm` stands absent, so the coverage target
   exits 2 at its HTML step while the gate exits 0
   (`/tmp/ia3/cov/cov-target.log`, `/tmp/ia3/cov/cov-gate.log`). The operator
   action names the package, and the two rows keep their own verdicts
   (ruling 3).
5. The privileged surfaces. tracefs and the `msr` nodes stand root-only, so
   SC-005 and SC-007 rows read BLOCKED with the operator block below.

### Phases and Implementation Ordering

The order prices every ceiling before any phase reads it. Phase 2 freezes the
base capture and the four ceiling prices, and phases 6 and 7 read the two
figures of the primary pair at the final head.

1. **Recheck the identity table and open the artifact directory (IC-001)**.
   Re-run each line of the closeout identity table at branch time: head and
   remote, ancestry, cpu, version, schema and SOVERSIONs, the two counts,
   `perf_event_paranoid`, `nmi_watchdog`, the two device modes, and the two
   root probes. Create `/tmp/ic/final` for every log, and keep no log inside
   the repository. Print `cat /proc/loadavg`, the affinity and a count of
   other `ctest` and compiler processes before each timed step.
   Verify: every reprinted line equals its table row, or the record names the
   difference beside the reading that moved.
2. **Freeze the base capture, and price the four ceilings (ruling 5, ruling
   21)**. Copy `build/coverage/coverage.info`, the gate log and the target log
   to `/tmp/ic/final/base-coverage/`, and print `sha256sum` on each copy.
   Write the within-head pair for the four names: the serial capture of
   `/tmp/ia3/cov/cov-test.log` beside the four hand re-runs at `5fe3d244`, each
   against its own ceiling (`test/CMakeLists.txt:1014`, `:2497`, `:2497`,
   `:2278`). Label that pair as the price of each ceiling, and keep the capture of that chain,
   recorded at `5fe3d244`, as the before figure of the primary pair.
   Verify: the four hand logs each answer exit 0, the copied trace hash equals
   the hash of the file it came from, and the four prices stand beside four
   ceilings.
3. **Run both instrumented lanes at the working head (IC-001, IC-003)**.
   Wipe and configure one tree at a time, run each suite alone, and keep the
   original failing run as the first of three runs per failed name. Then the
   isolated run in the same tree with the same compiler, the same sanitizer
   options and the same ceiling, then the bounded run under `taskset -c
   0,1,2,3` with `-j 4` (ruling 6). Record for each run the child command, the
   work or event count where the entry reports one, the exit, the reason, the
   grant inside the driving script and the ceiling `ctest` applies. `ctest -N`
   prints the entry count before each suite. A run that passes on a retry
   stands labeled a diagnostic, and a duration alone clears nothing. Beside
   each red name print the walk arithmetic: the child count of that name
   multiplied by the priced launch, read against the ceiling it cuts. That
   arithmetic has an answer at the base head already. `base_cli_matrix` runs 26
   launches, which cost 43.4 to 48.1 s of its 120 s ceiling in the release tree
   and 56.9 to 62.1 s of it in the address tree, and it cuts that ceiling at
   120.13 s alone and at 120.04 s under `taskset -c 0,1,2,3` with `-j 4`.
   `example_run` runs 74 launches, 37 of them `--list`, which cost 123.6 to
   136.9 s of its 300 s ceiling in the release tree and 162.1 to 176.9 s of it
   in the address tree, and it cuts that ceiling at 300.03 s alone and at
   300.01 s bounded (`/tmp/ic/base/alone-sanitize-base_cli_matrix.log`,
   `alone-sanitize-example_run.log`, `bounded4-sanitize.log`). The thread tree
   adds two more ceiling crossings: `harness_hunt_cli_test` at 300.15 s alone
   and 300.04 s bounded, `harness_jitter_cli_test` at 300.07 s alone and 300.04
   s bounded, whose 157 and 45 launches cost 262.2 to 290.4 s and 75.1 to 83.2
   s of the same 300 s ceiling in the release tree. The fifth thread name is a
   different row: `report_shape_fwq_ftq_sample` reads `***Failed 110.27 sec`
   inside the suite and answers exit 0 alone in 79.05 s, because that entry
   carries its own 110 s grant at `tools/docs/report_shape.py:368`, and the 120
   s ctest ceiling at `test/CMakeLists.txt:2197` never binds it.
   Verify: every red name carries three runs with six fields each, the grant
   column reads `none in the script` for `base_cli_matrix` and `example_run`,
   with `test/base_cli/run_matrix.py:211-227` and `test/example_run.sh:39-57`
   beside it, and it reads `110 s inside the script` for
   `report_shape_fwq_ftq_sample`, with `tools/docs/report_shape.py:368` beside
   it.
4. **Answer the catalog question with one scenario (ruling 9)**. Run the
   scenario `bare-hunt-whole-catalog` of the built test binary
   (`test/source/harness_hunt_cli_test.cpp:3516`, body
   `scenarioBareHuntSelectsTheCatalog`, selected through `argv[1]` at `:3703`),
   which answers `bare-hunt-whole-catalog: ok` when the plan of that fixture
   holds the scripted 200, then run
   `./build/example/first_benchmark --hunt --dry-run` and print the
   `hunt: plan events=` line that run answers. A figure of 200 keeps the
   scripted fixture
   (`test/source/detail/hunt_fixture.hpp:75,229-233`) and corrects the live
   claim at `specs/048-hwlat-probe/results/acceptance-final-intel.md:546-548`.
   A figure near the host catalog moves the cause into the fixture, and the
   fixture stands excluded by ruling 8, so the row goes open with that name.
   Verify: the line prints, and the record quotes it beside the scenario name
   and the exit.
5. **Correct the records (IC-006, IH-001, IH-002, IP-001, IP-002)**.
   Apply the listed corrections, each in the file and at the line that holds
   the claim. Old sections keep their text and read as history. Every new row
   states a figure a run at the final head answered, under the fixed point
   phase 9 holds: one amend, one full re-run at the new head, and a second
   difference stops the lane with both shas in the report.
   - 048: name `tracing_on` in the restore sentence at
     `results/acceptance-final-intel.md:454`, so the claim reaches one file and
     D-8 and FR-021 hold; state the residue as the kernel's remembered default
     and read `write` at `:192`; print the three refusal commands and the three
     distinct refusal texts at `:414-416`, where row b carries the engine-word
     line `noise_tracer.cpp:66-67` and row c carries the missing-window line
     `cli.cpp:191-193`; write `$HOME/.local/bin/python3.11` at `:218`, `:340`
     and `:561`; `:54` cites `results/user-space.md`, that file stands absent,
     so cite `A-5` of `Assumptions` of `specs/048-hwlat-probe/spec.md` and the
     `not available` paragraph of `The method run without root` of
     `specs/048-hwlat-probe/quickstart.md`; run the operator block at `:437-448`
     through the quickstart helper, so the mode reads as the one word that
     helper prints;
     give SC-IA-3 its six before-and-after rows, where `:179-182` now
     carries four, with `tracing_on` as a reading of its own and a seventh row
     that carries no figure (ruling 24).
   - 048 quickstart: keep `specs/048-hwlat-probe/quickstart.md` out of the
     edit list, because it stands outside the owned scope of this closeout.
     Record in the corrections table that the sentence in
     `The acceptance runs on corsairone` of
     `specs/048-hwlat-probe/quickstart.md` says the record carries seven
     tracefs reads, where ruling 33 of `Rulings - this phase (29 to 42)` of
     `specs/048-hwlat-probe/spec.md` sets six before-and-after readings and a
     seventh reading of `tracing_on`, and name that file as the place the
     wording belongs once a change owns it.
   - 048 matrix: record the reading as disproved. `case10` stands at
     `test/data/base_cli_matrix.txt:10` and `case21` at `:21`, each with the
     expected status `2` and the fault line it asks for, and
     `test/base_cli/run_matrix.py:348-356` already refuses a row whose exit
     moves away from the status the row declares. Record those anchors, state
     that IC-004 names no defect in the matrix, and keep every instrumented row
     that crosses a ceiling open with the provider-walk follow-up (ruling 23).
   - 046: state 497 printed rows with 20 of them ranked at
     `results/acceptance-hunt.md:4-5` and `results/pr-body.md:56-60`, beside
     the rank limit at `source/harness/hunt_plan.cpp:371-374`; scope or drop
     the per-rank claims at `results/acceptance-hunt.md:39-40`; `:57-58` cites
     `source/harness/cli.cpp:521`, the jitter answer, so point that citation at
     `source/harness/report.cpp:404-411`; name the ask levels
     `kTopdownAreaLevel {6}` (`source/harness/detail/hunt_topdown.hpp:42`) and
     `kTopdownCompanionLevel {1}` (`source/harness/detail/internal.hpp:1363`,
     moved by #100);
     print the `topdown: file` digest lines the run emits, in full, with the
     count read from that run, at
     `results/acceptance-hunt.md:45-46`, which now describe them; add the SC-010
     row for Intel parts measured through `PERF_METRICS`, pending and with its
     reason, beside the three pending rows at
     `results/acceptance-final-intel.md:159-160` and
     `:210-212`; name the criterion beside every table row (IH-002).
   - 041: give the parity row at
     `specs/041-counter-probe/results/acceptance-final-intel.md:96-97` the
     blocked reason of `:64-68`, which is the write to
     `/proc/sys/kernel/perf_event_paranoid`, and keep `limit=4` with `probe=4`
     as the measured agreement at `:92-95`; state the opens and reads for this
     host; keep the `/tmp` tool path at
     `results/requirement-1-shipped-shape.txt:100,107,123` as history and write
     the new row build-relative; label the host of
     `results/probe-window-trials.txt:1,17-19` as unverified, because its
     kernel string and `perf_event_paranoid=2` reading belong to another host;
     keep the watchdog rule applied once, at
     `source/counters/linux_pmu/limit_fill.cpp:93`, and name no second
     application as a correction (IP-001).
   Verify: one grep per correction. A search for `/home/archerc` over the new
   sections answers 0, the three refusal texts stand distinct, six
   before-and-after rows stand beside a seventh, and each corrected line names
   the file and the id it cites.
6. **Squash, then run the closeout set at the head that squash produced
   (ruling 20, G-23)**. One commit carries the artifacts. The set then runs in
   clean trees in this order, because `build` holds the other three: coverage,
   then the copy of `coverage.info` to `/tmp/ic/final`, then address and
   undefined, then thread, then the release lane with its masked runs, then the
   gates, then `build/dev`, then the hardware rows. Every figure the records
   state at that head comes from a run at that head.
   Verify: `git rev-parse HEAD` equals the head named in every row of the new
   sections, no row states a figure a run at another head answered, and every
   figure row names both heads of ruling 29 of `Rulings - this phase (29 to 42)`
   of `specs/048-hwlat-probe/spec.md` beside that ruling's markdown-only proof
   clause.
7. **Run the hardware rows at that head (IH-001, IP-001, IP-002, ruling 12,
   ruling 18)**. Each console stands alone on an idle host with its load
   witness printed. Three normal-method consoles per command: the two hunt
   commands of `specs/046-hunter-topdown/results/acceptance-hunt.md:14-16` and
   `./build/example/docs_counter_probe`. Print the `topdown: level=` line the
   run prints and the `topdown: file` digests the run emits, at their
   length from the binary, with the count read from that run. Print
   the parity statement twice: `counter limit: cpu limit=4 source=decode
   probe=4` at exit 0 at the standing `perf_event_paranoid` -1, and a second
   row BLOCKED for the `perf_event_paranoid` 2 reading with the control, the
   command that sets `2`, the command that restores the value, the missing
   right, and the standing value cited from the identity reading of that run. Print the opens and reads as one measured
   figure and one derived figure: the library path opens no device file, and
   the example's own search opens up to 15 and reads up to 5 for an answer of
   4, derived from the loop at `example/docs/counter_probe.cpp:329-338` with
   `kProbeCap` 32 at `:88`, because no acceptance row runs a tracer (ruling
   13). Report rank movement on both denominators per command: movement among
   the 20 ranked rows and movement across every printed row (ruling 15).
   Verify: three consoles per command, each with its own capture path, exit
   and load witness, and the level line and the digest lines the run emits
   printed verbatim at the count that run prints.
8. **Run the DBC gate route (IC-005, ruling 22)**. Configure
   `build/dev` with the supported interpreter, run the unchanged target, and
   run the manual gate commands beside it in the same tree. `ctest -R
   dbc_gate_fixtures` runs in that tree too. Nothing installs, no path enters
   a tracked file, and the row names the interpreter it used.
   Verify: the target, the document gate and the pair gate agree at exit 0,
   the interface count prints, and the fixture entry exits 0.
9. **Close the fixed point**. Compare every figure the records state with the
   run that answered it at that head. A difference amends the row, squashes
   again and re-runs the whole set at the new head. A second difference stops
   the lane, and the report names both shas and the two readings.
   Verify: at the closing head the comparison prints no difference, and every
   amended row names the head whose run answered it.
10. **Run the prose gates and land the commit (XI, VIII)**. Spell runs again
    over the written prose. The tree form runs after the record writes with the
    records visible to `git ls-files`, and it prints its unit count. The range
    form runs from the base sha to the squashed head. The commit then carries
    the title `Acceptance: Intel closeout records and matrix rows`, a body
    wrapped at 72 columns, and the footer block
    `Refs: specs/048-hwlat-probe`, `Refs: specs/046-hunter-topdown`,
    `Refs: specs/041-counter-probe`, `Approved-by: OmO`, with no `Fixes:`
    trailer. OmO stands as author and as committer.
    Verify: both prose forms exit 0 and print their unit counts, `git
    log -1 --format=%an <%ae> %cn <%ce>` answers OmO twice, and
    `git log -1 --format=%B` holds one title line of 50 characters or fewer,
    body lines of 72 columns or fewer and the four trailers in that order.
11. **Push and open the pull request (IC-007, SC-IC-6)**. The first push runs
    `git push -u origin fix/intel-acceptance-closeout`. A later push uses
    `git push --force-with-lease=fix/intel-acceptance-closeout:<expected-full-sha>`
    with the sha it last observed. A failed lease stops the lane. One pull
    request opens, nothing merges, and no issue closes. The body carries the
    base sha, the final head sha, every command with its exit and count, the
    root status, the hardware results, each diagnosed cause with its scoped
    fix or its deferral, the counts per lane and every open blocker in plain
    words. `.specify/feature.json` stays out of the commit.
    Verify: `git ls-remote` equals `git rev-parse HEAD`, and the body names the
    final head with that head's own counts.

### Lane Table

A row reads PASS when a run answered it. It reads FAIL when a run answered and
the contract did not hold. It reads BLOCKED when no run in scope could answer,
because a right, a tool or an operator action stands missing, and a BLOCKED row
carries no figure. A row whose fix sits in an excluded file reads open with a
named follow-up, and it carries its measured figures beside that name (ruling
4). Commands copy from `.github/workflows/ci.yml` through `/tmp/ic/w5-gates.md`
section 6, and `--test-dir` carries a working directory a job sets with
`working-directory`.

| Lane | Command | Tree | Shape | What it answers | Row word |
|---|---|---|---|---|---|
| Identity | block 7 below | tree, and every build tree | alone, before the first suite | the fields `IC-001` asks for: the source sha, the cpu affinity the host allowed, the compiler and the launcher of each tree, the interpreter each tree configured, `perf_event_paranoid`, `nmi_watchdog`, the `msr` reach, the tracefs reach; record row: `record`, the `Identity at the base head` table | PASS, or BLOCKED with the right it lacks |
| Entry count | `ctest --test-dir build -N`, then the same command with `--test-dir build/coverage` and with `--test-dir build/dev` | each | alone | the entry count `IC-001` asks of each suite: `build` answers it for the release suite, `build/coverage` answers it for the coverage suite, `build/dev` answers it for the fixture entry | PASS or FAIL |
| Release build | `cmake --preset=ci-rocky` then `cmake --build build --config Release -j "$(nproc)"` | `build` | alone | the tree every hardware row runs in, and it wipes `build` first (ruling 2); record row: `record`, the `Release build` row of the `The gates` table under `Part D` | PASS or FAIL |
| Release suite | `ctest --test-dir build --output-on-failure --no-tests=error -C Release -j "$(nproc)"` | `build` | alone, one lane | the suite count, the exit and every red name; record row: `record`, the `Plain, CI form` row of the `The suite lanes` table under `Part D` | PASS, FAIL, or open with the walk follow-up |
| Coverage denominators | `lcov --branch-coverage --summary build/coverage/coverage.info` | `build/coverage` | after the capture | this phase's base head stands `a55728af`; this host counts 9,273 branch positions at `2d4d8c57`, the head that older figure came from, and the hosted runner counts 9,823 at `5fe3d244`, so a local shortfall and a hosted shortfall name different totals and never compare; record row: `record`, the `Coverage: two shapes, two denominators` section | PASS or FAIL |
| Coverage, cold rows | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`, then `lcov --branch-coverage --summary build/coverage/coverage.raw.info`, then `lcov --branch-coverage --summary build/coverage/coverage.info` | `build/coverage` | after the block 1 run, the run that writes both files | `IC-002` of `specs/048-hwlat-probe/spec.md`, and the fourth row of the figure table of that section: for every cold row the gate prints, what the raw capture `build/coverage/coverage.raw.info` holds, what that trace holds when re-read through the extract step at `cmake/coverage.cmake:111-125`, and what the extracted trace `build/coverage/coverage.info` holds, which is the file the gate reads; record row: `record`, the `Coverage: two shapes, two denominators` section | PASS or FAIL |
| Two-cpu release | `taskset -c 0,1 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 2` | `build` | masked, alone | whether a narrow mask changes a ceiling; record row: `record`, the `Two cpus` row of the `The suite lanes` table under `Part D` | PASS or FAIL |
| Four-cpu release | `taskset -c 0,1,2,3 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 4` | `build` | the hosted width | the hosted shape answered at home; record row: `record`, the `Four cpus` row of the `The suite lanes` table under `Part D` | PASS or FAIL |
| Coverage chain | block 1 below | `build/coverage` | serial, then hand re-runs, then target and gate | the two numerators, the cold rows, the gate exit and the target exit; record row: `record`, the `Coverage: two shapes, two denominators` section and the `The coverage target, blocked at its HTML step` section | one verdict per step: the capture step PASS, the gate step PASS from the gate's own exit, the picture step BLOCKED with the operator action `pacman -S perl-gd gd`; the target's exit and the gate's exit print apart (ruling 3, ruling 32); the blocked step stands as measured at `2d4d8c57`, the head the older coverage reading came from |
| Address and undefined | block 2 below | `build/sanitize` | alone, then each red name alone at its ceiling, then that name's own command with no ceiling in its own working directory, then the suite under `taskset -c 0,1` with `-j 2`, then the suite under `taskset -c 0,1,2,3` with `-j 4`, then the suite a second time | the red names with the file that owns each fix, five across the two lanes at `2d4d8c57`: `base_cli_matrix` with `test/base_cli/run_matrix.py`, `harness_jitter_cli_test` with `test/source/harness_jitter_cli_test.cpp`, then open with a named follow-up: `harness_hunt_cli_test` with `test/source/detail/hunt_fixture.hpp`, `example_run` with the programs under `example/`, `report_shape_fwq_ftq_sample` with `tools/docs/report_shape.py` and its registration in `test/CMakeLists.txt`; the `SC-IC-9` pair with both exits and both elapsed times printed; record row: `record`, the `Three shapes per red name` section and the `The two instrumented lanes` section | PASS, FAIL, or open with the walk follow-up |
| Thread | block 3 below | `build/tsan` | the shapes of the row above, its narrow shape, its bounded shape and its second suite run | the same, for the race configuration, with both exits and both elapsed times of the two suite runs printed; the red names with the file that owns each fix, five across the two lanes at `2d4d8c57`: `base_cli_matrix` with `test/base_cli/run_matrix.py`, `harness_jitter_cli_test` with `test/source/harness_jitter_cli_test.cpp`, then open with a named follow-up: `harness_hunt_cli_test` with `test/source/detail/hunt_fixture.hpp`, `example_run` with the programs under `example/`, `report_shape_fwq_ftq_sample` with `tools/docs/report_shape.py` and its registration in `test/CMakeLists.txt`; record row: `record`, the `Three shapes per red name` section and the `The two instrumented lanes` section | PASS, FAIL, or open with the walk follow-up |
| Hunt, one scenario and one plan line | `./build/test/harness_hunt_cli_test bare-hunt-whole-catalog`, then `./build/example/first_benchmark --hunt --dry-run` | `build` | alone, in that order | the scenario's own answer line, then the `hunt: plan events=` line of the live host, which is the figure ruling 9 asks for; record row: `record`, the `IC-004` row that holds the scenario and that plan line | PASS or FAIL |
| DBC gates | block 4 below | `build/dev` | alone | the target, the two manual gates and the fixture entry; record row: `record`, the four `DBC` rows of the `The gates` table under `Part D` | PASS or BLOCKED with the operator action |
| Static analysis | `cmake --preset=ci-ubuntu -B build/analyzer -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=`, then `cmake --build build/analyzer --target speedgun-ng_speedgun-ng speedgun-ng_harness -j "$(nproc)"`, `cmake --build build/analyzer/test -j "$(nproc)"`, `cmake --build build/analyzer/example -j "$(nproc)"`. `ci.yml:374` carries the bare preset, and `ci.yml:386-387`, `:393`, `:399` carry the three builds against `build`; `-B build/analyzer` keeps this tree off the `build` directory `ci-rocky` owns, and the two launcher clears keep ccache from answering a scope from cache. | `build/analyzer` | fresh tree | the translation-unit count per scope and zero findings on the gate checks, where a scope that builds no translation unit is void and its empty finding list answers nothing (the risk list below); record row: `record`, the three `Analyzer` rows of the `The gates` table under `Part D` | PASS or FAIL |
| Lint | block 5 below | `build`, tree | alone | drift, format, time source, spell | PASS or FAIL |
| Prose, tree | `cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake` | tree | alone | XI over every written line, with the unit count printed; record row: `record`, the `Prose, tree` row of the `The gates` table under `Part D` | PASS or FAIL |
| Prose, range | `cmake -D PROSE_MODE=range -D PROSE_BASE=a55728af -D PROSE_HEAD="$(git rev-parse HEAD)" -P cmake/prose-lint.cmake` | tree | after the squash, with the head read at run time | XI over the added and modified lines of the squashed commit, whose left edge is the base `a55728af` and whose right edge is the head of that commit; record row: `record`, the `Prose, range` row of the `The gates` table under `Part D`, which the standing table leaves empty | PASS or FAIL |
| Install and audits | `cmake --install build --config Release --prefix prefix`, then the twelve dependency audits and the standalone link-manifest audit | `build` | alone | the install surface of the head | PASS or FAIL |
| Host records | `bash tools/ci/host_record_gate.sh build/test/host_record_check . validation` | `build` | alone | the host-record gate of the test job | PASS or FAIL |
| pmu tables | `python3 tools/pmu_events/update_pmu_events.py --check` | tree | alone | the vendored tables match the check | PASS or FAIL |
| Two counts | `wc -l README.md`, then the `LCOV_EXCL` sum over `source include` | tree | alone | 1002 against 1004 and 84 against 84, read at `615c0964` and again at `87d745d6` | PASS or FAIL |
| Downstream jobs | the shared-audit, downstream-consumer and consumer-release blocks of `/tmp/ic/w5-gates.md` section 6 | `build-shared`, `build` (the audit configure at `ci.yml:884`, which `IA48-056` owns and which is the last thing that tree does: it runs after every figure that needs the release tree, the release tree does not survive it, and `IA48-061` carries the record line that says so), `build-consumer`, `build/consumer-release`, `build/consumer-release-dev` | as the workflow writes them | the purity greps and the ignore-semantics asserts | PASS, FAIL, or BLOCKED where a system dependency stands absent |
| Hunt, walk | `./build/example/memory_walk --hunt --hunt-budget 120s` | `build` | three consoles, alone | area cells, ranked and printed rows, the `topdown: level=` line with its `passes=` and `table=` fields, the `topdown: file` digest lines at `no figure`, rank movement on both denominators, and the node-mapped, list-group and unmapped cells over the printed rows read beside the `level=1` companion block lines; record row: `record 046`, the `The ranked rows` and `The level-1 block` sections, and the rows of `Rows the final head must answer` in `specs/046-hunter-topdown/spec.md`, whose reading pairs stand in `The reading pairs the record must print` of `specs/046-hunter-topdown/plan.md` | PASS or FAIL |
| Hunt, benchmark | `./build/example/first_benchmark --hunt --hunt-budget 120s` | `build` | three consoles, alone | the same readings for the second binary, the digest lines at `no figure` included, and the node-mapped, list-group and unmapped cells over the printed rows read beside the `level=1` companion block lines; record row: `record 046`, the `The ranked rows` and `The level-1 block` sections, and the rows of `Rows the final head must answer` in `specs/046-hunter-topdown/spec.md`, whose reading pairs stand in `The reading pairs the record must print` of `specs/046-hunter-topdown/plan.md` | PASS or FAIL |
| Counter count | `./build/example/docs_counter_probe` | `build` | three consoles, alone | the limit line, the source word, the probe word; record row: `record 041`, the `The example, twice` table | PASS or FAIL |
| Counter device | `cat /sys/devices/cpu/caps/pmu_name`, then `cat /sys/bus/event_source/devices/cpu/type` | tree | alone, beside the three consoles | the pmu name line and the device type line `IP-001` asks for, read beside `perf_event_paranoid` and `nmi_watchdog`; record row: `record 041`, the `The two device type files` table and the `The device and the kernel settings` table | PASS or FAIL |
| Walk price | release leg: `./build/example/docs_labels --list` three runs, `./build/example/docs_labels --help` three runs. Instrumented leg: `./build/sanitize/example/docs_labels --list` three runs, `./build/sanitize/example/docs_labels --help` three runs, `./build/sanitize/example/docs_labels --version` three runs | `build`, then `build/sanitize` | the release leg in the release lane; the instrumented leg inside the block 2 lane, before the release wipe, which deletes every tree under `build`; each leg alone on an idle host, load witness printed | the price of one launch that only prints, and the child count of each instrumented entry; record row: `record`, the walk-price paragraph of the `Three shapes per red name` section | PASS, FAIL, or open with the walk follow-up |
| Parity at paranoid 2 | no command stands in scope | - | operator action | the historical environment IP-002 names | BLOCKED with the control and the value 2 |
| Root surfaces | block 6 below | `build` | one operator session | SC-005 and SC-007 restore rows, six values and `tracing_on` | BLOCKED with the operator block |
| Push | `git ls-remote origin refs/heads/fix/intel-acceptance-closeout` | tree | after each push | SC-IC-6 | PASS or FAIL |

Thirty rows carry the ten lanes of `The final-head set`, and the map stands
here. Lane 1, the coverage chain: `Coverage chain`, `Coverage denominators`,
`Coverage, cold rows`. Lane 2, the address and undefined suite: `Address and
undefined`, plus the instrumented leg of `Walk price`. Lane 3, the thread
suite: `Thread`. Lane 4, the release suite in `build`: `Release build`,
`Release suite`. Lane 5, the run bounded to cpus 0 and 1: `Two-cpu release`.
Lane 6, the run bounded to cpus 0 to 3: `Four-cpu release`. Lane 7, the walk
pricing: `Walk price` with both legs. Lane 8, the gate tree: `DBC gates`,
`Prose, tree`, `Prose, range`. Lane 9, the hardware consoles: `Hunt, one
scenario and one plan line`, `Hunt, walk`, `Hunt, benchmark`, `Counter count`,
`Counter device`. Lane 10, the static-analysis tree: `Static analysis`.
Nineteen rows answer those ten lanes, and eleven carry a duty the phase states
elsewhere: `Entry count`, `Identity`, `Lint`, `Install and audits`,
`Host records`, `pmu tables`, `Two counts`, `Downstream jobs`,
`Parity at paranoid 2`, `Root surfaces`, `Push`. In the last column, `record`
names `specs/048-hwlat-probe/results/acceptance-final-intel.md`, `record 046`
names `specs/046-hunter-topdown/results/acceptance-hunt.md`, `record 041` names
`specs/041-counter-probe/results/acceptance-final-intel.md`, and `Part D` names
the `Part D - the instrumented lanes and the gates` section of `record`. A
named section holds the figures of the head it measured, and a Pass 2 row lands
under its own heading beside them.

A run driver takes steps that no row names, and each one has a home here. The
identity step reads the fields of the `Identity` row through block 7. The step
that copies the coverage trace out before the release wipe stands as the last
line of block 1. The entry-count steps stand in the `Entry count` row, which
names `build`, `build/coverage` and `build/dev`. The two second suite runs
stand in the `Address and undefined` row and the `Thread` row, which carry
`SC-IC-9`. The cache-read step of the gate tree stands in the block 7 loop,
which reads `build/dev/CMakeCache.txt` beside the other trees. The scenario
step `./build/test/harness_hunt_cli_test bare-hunt-whole-catalog` and the plan
step `./build/example/first_benchmark --hunt --dry-run` stand in the `Hunt, one
scenario and one plan line` row. The `./build/example/memory_walk --list` step
stands in lane 7, the walk pricing, whose release leg runs the same shape. The
findings step of the static-analysis lane stands in the `Static analysis` row,
and that row holds its guard: a build of zero translation units is void, and an
empty finding list then answers nothing.

Ruling 30 of this phase's list governs the load witness of every timed step. A
figure stands when that lane's own witness line reports no other compiler, test,
capture or trace process, and a one-minute load average below 2.0. Any other
reading is void, and the lane re-takes it alone with its load witness beside
it. The witness line prints `cat /proc/loadavg`, the cpu affinity from
`taskset -pc $$`, and the count of other compiler, test, capture and trace
processes. A step whose witness cannot read prints no figure, and the driver
marks that step not run. The witness sentences in the `Load witness and stops`
paragraph of `Order of work` of this plan stand as history, and this paragraph
carries the ruling-30 shape for this phase.

Long rows, in full:

```
# block 1, coverage chain
rm -rf build/coverage
cmake --preset=ci-coverage
cmake --build build/coverage -j "$(nproc)"
lcov --zerocounters -d build/coverage
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage -t coverage
bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
cp -a build/coverage/coverage.info /tmp/ic/final/coverage.info
```

The `lcov --zerocounters` step is the local step CI does not need, because a
runner starts cold. The target runs after the hand re-runs, so the trace holds
them. The target's exit and the gate's exit stand as two verdicts (ruling 3),
and no flag enters either command (ruling 7).

The first line wipes the tree this block owns, `build/coverage`, before the
configure (ruling 2). The last line copies the trace to the run's artifact
directory outside the repository, and it stands before the release lane's wipe
of `build`. That one wipe deletes every tree under `build`, `build/coverage`
included, and the release lane runs last of the four suites (ruling 2).

```
# block 2, address and undefined
rm -rf build/sanitize
CXX=clang++ cmake --preset=ci-sanitize
cmake --build build/sanitize -j "$(nproc)"
ASAN_OPTIONS="strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1" UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j "$(nproc)"

# block 3, thread
rm -rf build/tsan
CXX=clang++ cmake --preset=ci-tsan
cmake --build build/tsan -j "$(nproc)"
TSAN_OPTIONS="halt_on_error=1:second_deadlock_stack=1" ctest --test-dir build/tsan --output-on-failure --no-tests=error -j "$(nproc)"
```

The option strings copy byte for byte from the workflow. The first line of
each block wipes the tree that block owns: `build/sanitize` in block 2,
`build/tsan` in block 3 (ruling 2). The release lane's wipe of `build` deletes
both trees, because `build` is their parent, so both blocks finish before it
(ruling 2).

The narrow shape replaces the final line of either block with `taskset -c 0,1 ctest ... -j 2`
inside the same tree, and the bounded shape replaces it with `taskset -c 0,1,2,3 ctest ... -j 4`.
Both shapes run in one tree at one head (`IC-003`, ruling 6). Each red name also
runs alone at its ceiling in that tree, then runs its own command with no
ceiling in its own working directory (ruling 34). The suite then runs a second
time in the same tree at the same head, and the record prints both exits and
both elapsed times (`SC-IC-9`).

```
# block 4, DBC gates
cmake -S . -B build/dev --preset=ci-ubuntu -D Python3_EXECUTABLE=/usr/bin/python3
cmake --build build/dev -t dbc-gate -j "$(nproc)"
doxygen build/dev/dbc-gate/Doxyfile
/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dev/dbc-gate/xml --registry tools/dbc/macros.yaml --out build/dev/dbc-gate/doc-matrix.json
/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools --registry tools/dbc/macros.yaml --doc-matrix build/dev/dbc-gate/doc-matrix.json --out build/dev/dbc-gate/pair-matrix.json
ctest --test-dir build/dev -R dbc_gate_fixtures --output-on-failure --no-tests=error -j "$(nproc)"
```

`--src2` takes both `source` and `tools` (`cmake/dbc-gate.cmake:44-55`), and a
copy that drops one narrows the scan and still exits 0. The four paths follow
`${CMAKE_CURRENT_BINARY_DIR}/dbc-gate` (`cmake/dbc-gate.cmake:14-19`), so a
`build/dev` tree prints `build/dev/dbc-gate` paths.

```
# block 5, lint
python3 tools/docs/example_drift.py .
python3 tools/docs/example_drift.py --sections --names --options --forbidden .
cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
bash test/time_source_gate.sh .
cmake -P cmake/spell.cmake
```

`ci.yml:31` runs the drift gate in its bare form. The four-flag form is the
standing step `results/acceptance-final-intel.md:336` ran at exit 0, and it
carries the same caps with the section, name, option and forbidden tables
checked by name.

```
# block 6, operator block for the root rows, in a session an operator opens
T=/sys/kernel/tracing
cat $T/current_tracer; cat $T/tracing_on
for f in current_tracer tracing_cpumask tracing_thresh \
         hwlat_detector/mode hwlat_detector/width hwlat_detector/window; do
  printf 'before %s %s\n' "$f" \
    "$(sed -e 's/.*\[\([^]]*\)\].*/\1/' $T/$f)"
done
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=$HOME/hwlat-detector.json
for f in current_tracer tracing_cpumask tracing_thresh \
         hwlat_detector/mode hwlat_detector/width hwlat_detector/window; do
  printf 'after %s %s\n' "$f" \
    "$(sed -e 's/.*\[\([^]]*\)\].*/\1/' $T/$f)"
done
```

The helper strips the mode list to the active word inside its brackets, and
`tracing_on` keeps a reading of its own. Before any write the block reads the
active tracer and `tracing_on`, and the row stops where an unrelated session
holds the tracer or a value reads unreadable. The interruption run prints the
same six values a second time after `kill -INT`. This session holds no
privilege, so every row in block 6 reads BLOCKED beside this block.

```
# block 7, the identity reads of IC-001, from the repository root
git rev-parse HEAD
taskset -pc $$
cat /sys/devices/system/cpu/online
cat /proc/sys/kernel/perf_event_paranoid
cat /proc/sys/kernel/nmi_watchdog
stat -c '%A %a %U:%G %n' /dev/cpu/0/msr /sys/kernel/tracing
for t in build build/coverage build/sanitize build/tsan build/dev build/analyzer; do
  printf '%s\n' "$t"
  grep -E '^(CMAKE_CXX_COMPILER|CMAKE_CXX_COMPILER_LAUNCHER|Python3_EXECUTABLE):' "$t/CMakeCache.txt"
done
```

The loop answers after each tree configures, because one `CMakeCache.txt`
holds the compiler, the launcher and the interpreter of its own tree, and a
tree that sets no launcher prints no launcher line. The two node rows answer
without a root session: the mode and the owner are the reading `IC-001` asks
for, and a write to either node stands outside this phase. The step runs again
before the first suite of the set, and the record prints its exit beside the
head sha.

### Risk and Rollback

What voids a measurement:

- A lane that ran while a second `ctest`, build or hand re-run ran. A figure
  taken on a host carrying another sweep's load is void (ruling 17).
- A tree configured or built at a head other than the head the row names
  (ruling 20).
- A coverage target run before the hand re-runs, so the trace holds no cut
  entry, and any partial union called a pass.
- A privileged run that started with `current_tracer` other than `nop`, or
  with `tracing_on` reading 1.
- An analyzer scope whose translation-unit count reads zero, because an empty
  finding list then proves nothing.
- A kill at exit 124 read as an expected failure, and a hosted job read as a
  local verdict (G-20).
- A tracked write or a staged index between the first `ctest` start of a
  sweep and its last exit, because `prose_gate_fixtures` and
  `dbc_gate_fixtures` compare a subprocess scan with a walk of the same
  checkout.
- A figure carried from `87d745d6` into a row that names the closeout head
  (ruling 10).

| Risk | Effect | Action |
|---|---|---|
| No privileged session | The SC-005 and SC-007 restore rows and the parity row at paranoid 2 stay open | Print the operator block, keep the rows BLOCKED with no figure, and finish every unprivileged row |
| `GD.pm` stands absent | The coverage target exits 2 one step after the gate passes | Two rows: the gate at its own exit, the HTML step BLOCKED with the package an operator installs |
| A ceiling crossing with no data failure | Up to five names stay red and no owned file holds a cause | Record the walk price, the child count and the refused registration, and keep each row open with the named follow-up (ruling 23). A priced launch of 1.67 to 1.85 s in the release tree and 2.19 to 2.39 s in the address tree (`/tmp/ic/base/walk-price.log`) already spends 56.9 to 62.1 s of the 120 s ceiling at 26 launches, and 162.1 to 176.9 s of the 300 s ceiling at 74 |
| The instrumented reds are a launch cost, and the fix sits before the option scan | No assertion fails, so no owned file holds a cause to change | Keep the cause where it belongs: `source/harness/cli.cpp:707-710` runs before `:717`, deferred with its reproduction (ruling 8), and every instrumented row prints the arithmetic beside it |
| A red sits inside its own grant | A row reads `***Failed` while the same entry passes alone | Two rows: `report_shape_fwq_ftq_sample` passes alone in 79.05 s, and its suite reading keeps its own exit beside the 110 s grant at `tools/docs/report_shape.py:368` |
| The provider walk lands in a fixture | A correction would sit in an excluded file | Ruling 8 holds: record the reproduction, defer the change, keep the row open |
| The record moves after a measurement | A second head | Phase 9 amends, squashes, re-runs the whole set once, and stops with both shas on a second difference |
| A push lease fails | A ref another session moved | Stop, report, refresh no lease, retry no push |

A void row gets re-taken in the same tree, with the same command, the same
options, the same ceiling and the same compiler, alone on an idle host, and the
record prints both readings: the void one with the load witness that voided it,
the fresh one with its own. A void reading stays as history under its heading.
Nothing deletes it, and no ceiling, grant or retry moves to make a re-run
answer differently. The branch carries one commit, so a rolled-back closeout
means one `git reset --hard` at the base sha, then a corrected squash pushed
with the lease the closeout constraints name. No production file changes, so no
binary can roll back.
