# Implementation Plan: Host Noise Pre-Check

**Branch**: `034-host-noise-precheck` | **Date**: 2026-10-27 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/034-host-noise-precheck/spec.md`

## Summary

The jitter probe's pre-check phase grows into the item N13b host
noise pre-check. The phase already lines every chosen probe cpu at
one barrier and spins them together for the `precheck=<ms>` window
(`source/harness/jitter_probe.cpp:226`); this feature grows the
measured set from the planned cpus alone to the planned cpus plus the
probe candidates (the cpus the `idle` preset would take, one per L3
group the plans touch), adds `precheck=serial` for the one-cpu-at-a-
time shape, and reads the recorded gap stats against the probe's own
threshold: each noisy planned cpu gets one warning line, the
pinning-candidate line ranks the planned cpus by gap rate, and
`strict` turns a noisy planned cpu into a status-2 usage error before
any benchmark starts. Every `/proc` and `/sys` read of the phase goes
through one file-root seam the tests install fake files into; the
attribution deltas (interrupts, per-kind softirqs, the switch and
schedule counts) and the host settings (`isolcpus`, `nohz_full`,
`rcu_nocbs`, the per-cpu governor, the per-cpu IRQ affinity) stand
disclosed beside the gap figures, each naming its source, and
`not available` stands where the host does not state it. The console
begins a
pre-check block before the benchmark tables and the JSON document
gains a top-level `precheck` record at schema minor `1.6`. The public
surface grows by one field: `JitterProbeOptions::precheckSerial`.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language)

**Primary Dependencies**: the standing probe unit
(`source/harness/jitter_probe.cpp` beside
`source/harness/detail/jitter_probe.hpp`) and its seams: the
reader-install seam
(`installTestReader`, `detail/jitter_probe.hpp:140`), the online-set
seam (`installTestOnlineCpus`, `topology.cpp:327`), and the layout
seam (`installTestLayout`); the `idle` preset resolution and the
overlap rule of `specs/027-jitter-probe/contracts/cli-options.md`;
the counters `WindowReader` and the `FakeProvider`
(`include/speedgun-ng/counters_fake.hpp`) that bounds every window in
the tests; simdjson v4.6.11 for the JSON read (`json_load.cpp`); the
report writer (`source/harness/json_report.cpp`) and the console
block (`source/harness/report.cpp:670`). `std::filesystem` carries
the file-root seam, so no new dependency joins.

**Storage**: N/A: no file and no database. The attribution snapshots
live in process memory for the life of the phase. The one document
the feature writes is the report file `--json` names.

**Testing**: the repository's own harness (`speedgun-ng_test`
pattern: one executable per area plus `add_test`), one new test
executable `harness_precheck_test`, the grammar tests extended in
`harness_jitter_cli_test.cpp`; fake `/proc` and `/sys` trees written
under a temporary directory and installed through the file-root seam;
the window bounded by a scripted `WindowReader` through
`installTestReader`, so no test sleeps and no test reads a clock;
tests run with `ctest -j 2` and pass under `taskset -c 0,1`.

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily
loop drives `ci-rocky`; once per feature, `ci-ubuntu` builds with
clang-tidy and cppcheck wired in. The acceptance shape stands on the
request's lane host (several CCDs, one L3 each); every test passes on
a 2-cpu host: the cpus come from `sched_getaffinity` (FR-021).

**Project Type**: library feature: one public field added to an
existing options struct, everything else harness-internal. No new
target, no new option row.

**Performance Goals**: the pre-check stands outside every timed
window (the phase closes before the latch of
`jitter_probe.cpp:111`); the window loop keeps the probe loop's
shape: no allocation, no syscall, no I/O inside the loop (FR-005 of
specs/027); the `/proc` and `/sys` reads stand before and after the
window, never inside it.

**Constraints**: every time value flows through `sg::counters` (the
time-source gate stays green); 100% line and branch coverage with no
`LCOV_EXCL` marker and no new `--ignore-errors` entry; no sleep and
no clock in a test; one line and exit status 2 for every bad
`precheck` value; a timing bound in the tests sits an order of
magnitude inside the contamination signal it discriminates against
(the record of `specs/032-fix-merged-features/results/record.md`);
no fixed cpu number anywhere; no lambda inside an `SG_*` macro;
`[[maybe_unused]]` on contract-only helpers; simdjson only for JSON.

**Scale/Scope**: one detail header added
(`source/harness/detail/proc_root.hpp`), one public field added
(`include/speedgun-ng/benchmark.hpp`), the probe unit extended (the
measured set, the serial windows, the attribution reader, the
warning, pinning and refusal rules), the report writer and loader
extended, one schema extension, one new test executable, one example
section re-recorded, and the docs rows.

**Execution mode**: TDD (Principle III). Every test task is written
and observed failing before the code task that turns it green exists;
the Test Plan records the waves and the gates each runs.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 surface. The critical path stays the standing probe loop (specs/027 D-7, the P0 techniques already documented there); the pre-check window reuses it unchanged. The serial shape adds sequencer rounds beside the loop, outside the timed path. The `/proc` reads use `std::filesystem` and standard file interfaces, so no compiler extension joins | PASS |
| II. Design by contract | The new public field carries a doxygen contract on the struct; the seam functions carry `\pre`/`\post`; the attribution reader, the warning rule and the JSON loader carry `SG_REQUIRE`/`SG_ENSURE` at their seams (the loader follows the standing `json_load.cpp` shape) | PASS |
| III. R-DCUT | Requirements FR-001 to FR-028 map to the decisions R-01 to R-14 below and to `tasks.md` (landing with `/speckit.tasks`). Execution mode: TDD, recorded above and in the Test Plan | PASS |
| IV. Documentation | README pre-check section of at most 25 lines with its quick-reference rows (IV.1), `docs/pages/harness.md` option and field detail, one runnable example with its `.expected` file re-recorded (FR-026) | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the feature head; every new name follows V.1: PascalCase types, camelBack functions, `k`-prefixed constants, lower_case file names (R-13) | PASS |
| VI. Coverage | 100% line and branch on new paths, no exclusion marker; every branch of the grammar, the serial sequencer, the warning and refusal rules, the pinning rank, the attribution reads, the `not available` paths, the JSON write and read runs under scripted files and time (FR-027) | PASS |
| VII. Performance | The pre-check stands outside every timed window; the `/proc` reads stand outside the window; the loop keeps its no-allocation, no-syscall shape; no benchmark baseline is claimed, so none is promised | PASS |
| VIII. CI gates | Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, tsan, coverage, the docs gates (drift, snippet compile, example run, report shape), prose, DBC, time-source, name-check | PASS |
| IX. Spec-driven | Spec, plan and research stand in `specs/034-host-noise-precheck/`; `tasks.md` lands with `/speckit.tasks` before code | PASS |
| X. Anti-slop | One seam for every file read (R-02), one warning rule over the standing threshold (R-05), one sequencer reusing the proven barrier rounds (R-06); the candidate set reuses the `idle` resolution unchanged; the public surface grows by one field and nothing else; no fallback, no extension hook beyond the seam the spec asks for | PASS |
| XI. Prose | The gate walks `git ls-files`, so at this point it clears the tracked tree only, while these artifacts stand untracked and await the run that lands with the branch's first commit, the way specs/025 and specs/027 recorded | PASS for the tracked tree, pending for these artifacts |

**Gate weakening requested**: no. **Gate strengthening requested**:
no. No budget figure moves: the README section lands inside the
standing 25-line section cap, and `PAGE_LINE_CAP` moves only if the
measured re-record asks, the way the specs/023, specs/025 and
specs/027 moves preceded.

No gate fails, so **Complexity Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/034-host-noise-precheck/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cli-options.md   # The value-grammar delta over specs/027
│   ├── console-report.md# The console block contract
│   └── report-json.md   # The `precheck` record contract
└── tasks.md             # Phase 2 output (/speckit.tasks - NOT created here)
```

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp        # + precheckSerial on JitterProbeOptions
source/harness/detail/proc_root.hpp      # new: the file-root seam (R-02)
source/harness/detail/jitter_probe.hpp   # + the pre-check state and report types
source/harness/jitter_probe.cpp          # + the measured set, serial windows,
                                         #   the attribution reader, the rules
source/harness/options.cpp               # + the `precheck=serial` parse (R-09)
source/harness/report.cpp                # + the pre-check block (R-07)
source/harness/json_report.cpp           # + the `precheck` object (R-08)
source/harness/json_load.cpp             # + the `precheck` reader (R-08)
docs/schema/report-v1.json               # + the keys and the moved x-order
docs/pages/harness.md                    # + the option and field detail
README.md                                # + the pre-check section (25-line cap)
example/docs/jitter_probe.cpp            # + the pre-check section
example/docs/jitter_probe.expected       # re-recorded
test/source/harness_precheck_test.cpp    # new: the feature's tests
test/source/harness_jitter_cli_test.cpp  # + the grammar rows
```

**Structure Decision**: the feature extends the standing probe unit
and its seams; the one new file is the seam header the N13d and N13e
lanes reuse (D-7 of the spec). No new target, no new option row, no
second pin path, no second time source.

## Design

### Decision index

- **R-01 The measured set (FR-010).** The pre-check phase measures
  the planned cpus and the probe candidates. The planned cpus come
  from the execution-context plans; the candidates come from the
  `idle` preset resolution (one per L3 group the plans touch, the
  cpus no plan uses and that stand no SMT sibling of a planned cpu),
  reused unchanged from specs/027 D-4. The measured set stands
  computed once before the window opens.
- **R-02 The file-root seam (FR-019, FR-020).** One detail header
  `source/harness/detail/proc_root.hpp`: `procRoot()` answers the
  process-global root (`/` by default), `installTestProcRoot()`
  installs a test root for the test's life and restores the default.
  Every `/proc` and `/sys` read of the pre-check resolves under the
  root. The seam follows the standing patterns (`installTestReader`,
  `installTestOnlineCpus`), carries the same test-only spelling, and
  is the named seam the N13d and N13e lanes reuse.
- **R-03 The attribution deltas (FR-016).** The reader snapshots each
  measured cpu before the window and again after it: the interrupt
  delta (the sum of the cpu's column over the rows of
  `/proc/interrupts`), a delta for each softirq kind the file states
  (`/proc/softirqs`, per-kind, one figure per kind), and the
  context-switch and schedule figures (the `/proc/schedstat` cpu
  line: the run-queue delay figure names the switch figure, the
  timeslices figure names the schedule count; the roadmap 10.1 row is
  re-confirmed at implementation, and a mismatch goes to the pull
  request as a question). Each disclosed figure names its file.
- **R-04 The host settings (FR-017).** One snapshot per run:
  `isolcpus`, `nohz_full` and `rcu_nocbs` parsed from
  `/proc/cmdline`; the governor of each measured cpu from
  `/sys/devices/system/cpu/cpu<N>/cpufreq/scaling_governor`; the IRQ
  affinity from `/proc/irq/<n>/smp_affinity_list`, the mask form
  `smp_affinity` as the fallback. Each figure stands beside the
  `not available` wording where the host does not state it.
- **R-05 The warning rule (FR-012, FR-014).** The threshold is the
  probe's own threshold: the standing default (200 ns, or ten times
  the measured loop period when that stands larger) or the explicit
  `threshold=` value. The probe records a gap only above the
  threshold, so a planned cpu stands noisy when its window recorded
  at least one above-threshold gap: `maxNs` and `gapsPerSecond` stand
  above the threshold together. One warning line per noisy planned
  cpu names the cpu and the two figures, the gap rate and the max
  gap. The strict refusal carries the same two figures (FR-014).
- **R-06 The serial windows (FR-009).** `precheck=serial` runs one
  measured cpu per window, ascending cpu order, the planned cpus
  before the candidates. The measurement pass of every item runs
  first (the threshold resolution stands per cpu), then the windows
  run one at a time: one `ContextBarrier` round per window, the item
  whose turn it is spins for the span, the others wait. The
  synchronizers are the proven ones of `ProbeSession`; no new
  primitive joins. In the parallel shape the standing single window
  stands unchanged.
- **R-07 The console block (FR-023).** `printPrecheckReport` in
  `report.cpp` prints, before the benchmark tables: one block per
  measured cpu (the sixteen field names the probe block uses, the
  attribution lines beside the gap figures, each naming its source),
  then the host-settings disclosure once per run, then the warning
  lines and the pinning-candidate line (every planned cpu with its
  gap rate, lowest first, ties in cpu order); the warnings and the
  pinning line stand the non-strict path, and a strict run prints no
  block: the refusal stands the one line of the standing error
  surface on standard error, exit status 2. The `not available`
  wording follows the standing disclosure shape.
- **R-08 The JSON record (FR-006, FR-024).** The writer gains a
  top-level `precheck` object standing after `jitter` and before
  `benchmarks`; the schema gains the keys and the moved top-level
  `x-order` in the same commit; the minor moves to `1.6`.
  `loadReport` reads the record back through the standing simdjson
  path. A run without the pre-check writes no `precheck` key, so its
  document matches master's apart from the `schemaVersion` value.
- **R-09 The parse (FR-001 to FR-003).** The `precheck=` value
  accepts the word `serial` beside the standing millisecond number;
  the parse maps it to the new field; a token that is neither stands
  the standing one-line status-2 error. The option-value lanes stand
  untouched (`kJitterProbeOptionValue` stays `260`).
- **R-10 The public field (FR-004, FR-005).** `JitterProbeOptions`
  gains `bool precheckSerial = false;` appended after `strict`. The
  aggregate layout grows, so the harness `SOVERSION` rises to `13`
  with the version move to `0.21.0`; the archive `SOVERSION` stays
  `4`; the ABI check rides the version commit.
- **R-11 The tests (FR-021, FR-022).** One new executable
  `harness_precheck_test`; the fake `/proc` and `/sys` trees stand
  under a temporary directory installed through the seam; the windows
  close through scripted `WindowReader` stamps (`installTestReader`),
  so no test sleeps and no test reads a clock; the cpus come from
  `sched_getaffinity`, and the suite passes under `taskset -c 0,1`
  and on a 2-cpu host. Every timing bound keeps the order-of-
  magnitude margin the record of specs/032 names.
- **R-12 The docs (FR-026).** The README section states the pre-check
  in at most 25 lines with its quick-reference rows; the User-guide
  page carries the option and field detail; the existing example
  gains a pre-check section and its `.expected` file is re-recorded;
  the drift, snippet-compile, run and report-shape gates cover it.
- **R-13 The naming (V.1).** `PrecheckCpuState`, `PrecheckHostSettings`
  and the report types take PascalCase; the reader and printer
  functions take camelBack; the constants take the `k` prefix; the
  seam header takes `lower_case` (`proc_root.hpp`).
- **R-14 The contract rows (FR-002).** The grammar row and the error
  table of `specs/027-jitter-probe/contracts/cli-options.md` gain the
  `serial` spelling in the same commit as the parser; the record
  shape, the `idle` preset and the overlap rule rows change none.

### Risks

- The `/proc` file shapes vary across kernel versions: the per-source
  `not available` rule absorbs a shape this host does not state, and
  the tests pin the shapes the fixtures carry (R-03, R-04).
- The serial sequencer adds one barrier round per window: the rounds
  are bounded by the measured-set size, the loop body stays
  allocation-free, and the synchronizers are the proven ones (R-06).
- The example's pre-check section prints host-dependent figures: the
  `.expected` file is compared by shape, the pattern the jitter
  example already follows (R-12).
- The schedstat field mapping rests on the roadmap 10.1 row this
  session cannot read: the plan pins the mapping, the implementation
  re-confirms it, and a mismatch goes to the pull request as a
  question (R-03).

### Logical view: the pre-check phase

```text
run starts
  └─ JitterProbe::start()
       ├─ resolve the measured set: planned cpus + candidates (R-01)
       ├─ every item: buffer touch, measurement pass (threshold per cpu)
       ├─ pre-check phase
       │    ├─ parallel: one window, every measured cpu spinning (R-06)
       │    └─ serial: one window per cpu, ascending, planned first (R-06)
       ├─ attribution snapshot after (R-03)  [before snapshot: pre-window]
       ├─ strict: a noisy planned cpu? → one refusal line, status 2 (R-05)
       └─ latch; precheck mode joins the probe here
  └─ benchmarks run (or not, on refusal)
report
  └─ console: pre-check blocks, host settings, warnings, pinning (R-07)
  └─ JSON: the `precheck` record (R-08)
```

### Logical view: one window's reads

```text
before the window                the window                after
─────────────────                ─────────                 ─────
/proc/interrupts  ┐              the probe loop,           the same three
/proc/softirqs    ├ snapshot     unchanged: one read,      files: snapshot
/proc/schedstat   ┘ (R-03)       one compare, one store,   (R-03)
                                 no I/O inside (R-01)
/sys + /proc/cmdline: one host-settings snapshot per run (R-04)
```

### Physical view

The feature touches the files the Source tree lists: the probe unit
carries the measured set, the serial sequencer, the attribution
reader and the rules; the detail header carries the pre-check state
types; the seam header is new and header-only; the report writer,
loader and printer carry the two surfaces; the schema and the docs
carry the contract. The public surface grows by one field
(`include/speedgun-ng/benchmark.hpp`); everything else stays
harness-internal.

### Public API surface added

| Name | Shape | Contract |
| --- | --- | --- |
| `JitterProbeOptions::precheckSerial` | `bool`, default `false` | the pre-check runs one measured cpu per window when set; the field carries the struct's doxygen contract (R-10) |

No other public name joins: the pre-check results surface through the
console block and the JSON record alone.

### Version and `SOVERSION`

The version moves to `0.21.0` (the next free minor at branch time).
The public struct edit is a public-ABI change, so the harness
`SOVERSION` rises to `13`; the archive `SOVERSION` stays `4`; the
check stands recorded in the commit that moves the version (FR-004,
FR-005, R-10).

## Test Plan

TDD waves; each wave's tests stand written and observed failing
before the code that turns them green:

1. **Wave 1, the grammar (FR-001 to FR-003).** `precheck=serial`
   accepted from the command line and the option file; a misspelled
   serial word, a mixed token, `precheck=0` and a value at or above
   2³² each print one line and exit 2; the option-value uniqueness
   test stands. Gate: `ctest` on `harness_jitter_cli_test`.
2. **Wave 2, the window (FR-007 to FR-011).** The measured set carries
   the planned cpus and the candidates; the parallel window covers
   all of them at once; the serial shape runs one cpu per window in
   ascending order, planned first; the window closes through the
   scripted reader with no wait. Gate: `ctest` on
   `harness_precheck_test` under `taskset -c 0,1`.
3. **Wave 3, the rules (FR-012 to FR-015).** One warning line per
   noisy planned cpu; the pinning line ranks every planned cpu,
   lowest first, ties in cpu order; the strict refusal completes the
   window, prints one line and exits 2 before any benchmark; the
   candidates never refuse; the quiet run continues; the empty
   candidate set discloses. Gate: `ctest` on `harness_precheck_test`.
4. **Wave 4, the disclosure (FR-016 to FR-018).** The deltas from fake
   files match the fake before-and-after differences and name their
   sources; per-kind softirqs; the host settings once per run; every
   missing, unreadable and unparsable shape reads `not available`
   for that source alone. Gate: `ctest` on `harness_precheck_test`.
5. **Wave 5, the report (FR-023, FR-024).** The console block stands
   before the benchmark tables with the probe block's field names;
   the JSON record round-trips through `loadReport`; the schema
   carries the keys and the moved `x-order`; the pre-check-free run
   matches master byte for byte apart from `schemaVersion`. Gate:
   `ctest`, the schema check, `report_shape`.
6. **Wave 6, the docs (FR-026).** The README section, the User-guide
   rows, the example and its `.expected`. Gate: `example_drift`,
   `snippet_compile`, `example_run`, `report_shape`.
7. **Wave 7, the gates (FR-027, FR-028).** Coverage at 100% lines and
   branches with no exclusion marker; sanitizers and tsan clean; the
   naming and format gates clean; `prose-lint` on every commit. Gate:
   the full CI matrix.

## Constitution Check

*Re-check after Phase 1 design.*

The design keeps every gate green: the one public field carries its
contract (II); the P0 surface stays the standing probe loop (I); the
coverage plan names every new branch (VI); the pre-check stands
outside the timed windows (VII); the seams reuse the proven patterns
and the public surface grows by one field (X). The one open mapping
(schedstat) carries its recorded fallback: re-confirm at
implementation, else a pull-request question (R-03). **PASS.**

## Complexity Tracking

> Empty: no constitution violation stands, so no justification row
> applies.
