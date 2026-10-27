# Feature Specification: Host Noise Pre-Check

**Feature Branch**: `034-host-noise-precheck`

**Created**: 2026-10-27

**Status**: Draft

**Input**: User description, opening quoted:

> Build the host noise pre-check, item N13b of the roadmap (section
> 10.3), on the jitter probe that master carries since PR #46. Before
> the benchmarks start, the harness probes the cpus the run will use,
> warns about the ones that show noise, names the ones that pin best,
> and discloses where the noise comes from without root. A strict
> setting makes a noisy planned cpu a usage error, so CI never
> records a bad run.

The request fixes the rulings P-1 to P-9; they stand in Decisions as
D-1 to D-9.

## Spec identity and run-time binding

- Spec number: `034`. `specs/` on master `3cc375d` holds `001` through
  `030` and `032` (read 2026-10-27). `031` is reserved for N0b host
  validation, `033` for N14a-2 hunter budgets. `git ls-remote --heads`
  names no ref carrying `034` or `host-noise-precheck`. The
  `gh pr list` check did not run: `gh` carries no token in this
  session, so
  the PR list stands on the directory and remote-branch checks, the
  standing record the PC list carries.
- Branch: `034-host-noise-precheck`, cut from master `3cc375d` on
  2026-10-27. The request names that hash as the base, and master
  stands on it at branch time, so no moved-base record applies.
- Version: master stands at `0.20.1` (`CMakeLists.txt:15`), harness
  `SOVERSION 12` (`CMakeLists.txt:298`), archive `SOVERSION 4`
  (`CMakeLists.txt:80`). The lane takes the next free minor,
  `0.21.0`. The harness `SOVERSION` rises by one only for a
  public-ABI change; the archive `SOVERSION` stays `4`; the check is
  recorded beside the version move, in the commit that moves it
  (FR-004, FR-005).
- Report schema: master stands at `1.5` beside
  `source/harness/json_report.cpp:36`, the top-level `x-order` at
  `docs/schema/report-v1.json:7`. The pre-check adds keys, so the
  schema minor takes the next free value, `1.6`, with the top-level
  `x-order` moved in the same commit (FR-006).
- Option values: no option row is added and no option value moves.
  The pre-check rides the standing `--jitter-probe` value grammar of
  `specs/027-jitter-probe/contracts/cli-options.md` (`mode=precheck`,
  `precheck=<ms>`, `strict`), which master carries in the N13a range
  `260` to `269` (`source/harness/detail/options.hpp:82`).

## Roadmap position

- Item N13b of the harness roadmap, section 10.3. N13a (the jitter
  probe, spec 027) is merged through PR #46. The pre-check is the
  first of the pre-check lane items; the N13d and N13e lanes follow
  and reuse the file-root seam (D-7). Roadmap section 10.6 stays in
  force in full.

## Lane identity

- The acceptance shape stands on the request's lane host: a
  Threadripper 9960X, 24 cores, 48 threads, several CCDs with one L3
  each. The several-L3 shape is the shape the pre-check exists to
  see: cross-cpu and per-CCD noise. The suite itself stays
  host-agnostic: the cpus come from the process affinity mask, and
  the suite passes on a 2-cpu host (FR-021). The request names the
  host; the spec carries the shape alone, the file-hygiene rule of
  `specs/032-fix-merged-features` FR-024 binding repository files.

## Audit point

- Repository: `master` at `3cc375d` (project version `0.20.1`, report
  schema `1.5`, harness `SOVERSION 12`, archive `SOVERSION 4`), read
  2026-10-27 on the lane host.
- The `--jitter-probe` value grammar stands at
  `specs/027-jitter-probe/contracts/cli-options.md:23`; the
  `precheck=` grammar row at `:31`; the `precheck=0` error-table row
  at `:77`; the error surface and exit statuses at `:64` to `:115`.
- The console probe block stands at `source/harness/report.cpp:670`
  (`printJitterReport`): one line per stats field in the E-03 order,
  the sixteen fields `gapCount`, `gapsPerSecond`, `minNs`,
  `medianNs`, `meanNs`, `p90Ns`, `p99Ns`, `p999Ns`, `p9999Ns`,
  `maxNs`, `totalNs`, `totalPercent`, `runtimeNs`, `thresholdNs`,
  `loopPeriodNs`, `droppedGaps`.
- The probe options stand public at
  `include/speedgun-ng/benchmark.hpp:1937` (`JitterProbeOptions`:
  `mode`, `precheckMs`, `strict`, beside the cpu list, the threshold
  and the gap count); a suite call binds them the way the command
  line does.
- `sg::NoiseWarning` stands public at
  `include/speedgun-ng/benchmark.hpp:668`; its disclosure lines print
  beside the results and stand unchanged by this feature (FR-025).
- The online-set seam stands: `installTestOnlineCpus` and
  `onlineCpuSet()` at `source/harness/topology.cpp:327` to `:366`;
  the planned cpus of a run come from the execution-context plans it
  serves.
- The timing-margin record stands at
  `specs/032-fix-merged-features/results/record.md`: a test bound
  that walks near the sanitizer's own load reads flaky, and a bound
  keeps an order of magnitude of margin inside the contamination
  signal it discriminates against.

## Standing facts this lane reads

- The jitter probe's own record shape, the `idle` preset and the
  overlap rule stand in
  `specs/027-jitter-probe/contracts/cli-options.md`; the pre-check
  reuses them and changes none of the three (FR-002).
- The noise-warning disclosure of `sg::NoiseWarning` stands beside
  the pre-check block and is not replaced (FR-025).
- The attribution sources and their parse risks stand in roadmap
  section 10.1, the sysjitter and osnoise rows.
- The roadmap's section 10.6 rules stay in force in full.

## Clarifications

### Session 2026-10-27

- Q: When strict mode finds a noisy planned cpu, does the refusal
  wait for the full pre-check window or stop at the first noisy
  measurement? → A: The pre-check completes its full window (every
  window in serial mode); one refusal line then names the first
  noisy planned cpu in cpu order, and the exit status is 2 (FR-014).
- Q: Does the softirq disclosure carry one aggregate figure per cpu
  or one figure per softirq kind? → A: One figure per softirq kind
  the host states, each kind named, an unstated or unparsable kind
  reading `not available` for that kind alone (FR-016).
- Q: What does the pinning-candidate line name? → A: Every planned
  cpu with its gap rate, the lowest rate first; tied cpus stand in
  cpu order (FR-013).

## Decisions

Decisions the request fixes for this feature, recorded as
requirements:

- **D-1: the surface stands (P-1).** `mode=precheck`, `precheck=<ms>`,
  `precheck=serial` and `strict` carry the feature. No new option row
  joins, no option value moves, and no new key joins the value
  grammar. The standing `strict` key gates the pre-check noise
  refusal the same way it gates the unavailable-TSC refusal today.
  The `precheck` key carries two accepted forms: the millisecond
  value and the word `serial`. The request names both readings of its
  own clause "no new value": (a) no new token joins the grammar, and
  the serial shape arrives another way; (b) the clause names the
  option-lane constants and the mode list, and the `serial` word is
  the one grammar addition the item itself names, beside the standing
  `<ms>` form of the same key. Reading (b) stands: the feature
  description, the P-1 value list and the acceptance shape (a
  misspelled serial exits 2) each name the token. The grammar row of
  the contract gains the word in the same commit as the parser
  (FR-002), and a misspelling keeps the standing status-2 shape
  (FR-003).
- **D-2: the window shape (P-2).** All planned cpus run in the same
  window, so the pre-check sees cross-cpu effects. The default window
  is 200 ms; `precheck=<ms>` sets the length. `precheck=serial` runs
  one planned cpu at a time (roadmap D-13.6). The probe candidates
  run in the same window as the planned cpus. `mode=both` runs the
  pre-check and then the alongside probe; `mode=alongside` alone runs
  no pre-check.
- **D-3: the warning rule (P-3).** The threshold is the probe's own
  threshold (the standing default and the `threshold=` value). A gap
  rate or a max gap above it warns. Each warning names the cpu and
  the two figures, the gap rate and the max gap, one line each, and
  the run continues. The
  pinning-candidate line names the planned cpus with the lowest gap
  rate and stands after the warnings.
- **D-4: the strict refusal (P-4).** In strict mode a planned cpu
  above the threshold is a usage error: one line naming the cpu, the
  gap rate and the max gap, exit status 2, before any benchmark
  starts, so CI does not record a bad run; the pre-check block stands
  unprinted, the refusal standing the one line of the standing error
  surface. The probe candidates never refuse; they inform the
  pinning-candidate line of the non-strict path alone.
- **D-5: the attribution deltas (P-5).** Each measured cpu discloses
  the window delta of its interrupt count, its softirq counts, and
  its context-switch and schedule counts, beside its gap figures. The
  deltas come from `/proc/interrupts`, `/proc/softirqs` and the
  per-cpu context-switch counts of `/proc/schedstat`, read before and
  after the window, and each disclosed figure names its source.
- **D-6: the host settings disclosure (P-6).** `isolcpus`, `nohz_full`
  and `rcu_nocbs` from the kernel command line, the cpufreq governor
  of each measured cpu, and the IRQ affinity that names each measured
  cpu stand disclosed once per run, each with `not available` where
  the host does not state it.
- **D-7: the injectable file root (P-7).** Every `/proc` and `/sys`
  read of the pre-check goes through one file-root seam that the
  tests install fake files into, the way the time source is injected.
  CI needs no real noise and no real PMU. The seam is a named detail
  seam the later N13d and N13e lanes reuse.
- **D-8: the report (P-8).** The pre-check prints its block before
  the benchmark tables, one block per measured cpu, the field names
  the console probe block uses. The JSON gains the pre-check record
  with the schema minor that follows the standing rule: the lane adds
  its keys to `docs/schema/report-v1.json`, moves the top-level
  `x-order` in the same commit, and takes the next free minor.
  `loadReport` reads the record back.
- **D-9: the docs and the example (P-9).** A `README.md` section of
  at most 25 lines, the option and field detail in
  `docs/pages/harness.md`, one runnable example under `example/docs/`
  with its `.expected` file, and the quick-reference rows.

## Out of scope

- The N13d and N13e lanes: they reuse the file-root seam and carry
  their own items.
- The host-validation feature (spec 031) and the N14a-2 hunter
  budgets (spec 033): both stand reserved.
- No change to the jitter record shape, the `idle` preset and the
  overlap rule rows of
  `specs/027-jitter-probe/contracts/cli-options.md` (FR-002), to the
  `sg::NoiseWarning` type and its disclosure (FR-025), or to the
  option-lane constants (FR-001).
- No new hard runtime dependency: the `/proc` and `/sys` reads use
  the standard file interfaces.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - CI refuses a noisy host before it records a run (Priority: P1)

A CI job runs the suite with `strict` on a host whose planned cpus
show noise above the threshold. The run stops before any benchmark
starts: one line names the cpu, the field and the figure, and the
exit status is 2. No bad run is recorded.

**Why this priority**: the request's own goal: CI never records a bad
run.

**Independent Test**: with fake `/proc` content and a fake clock
installed through the file-root seam and the time source, a run whose
planned cpu stands above the threshold exits 2 after one line, and no
benchmark row prints.

**Acceptance Scenarios**:

1. **Given** fake gap figures above the threshold on a planned cpu,
   **When** the run carries `mode=precheck,strict`, **Then** one line
   names the cpu, the field and the figure, and the exit status is 2.
2. **Given** the same figures with `strict` absent, **When** the run
   completes, **Then** the warning line prints, the run continues,
   and the exit status is 0.

### User Story 2 - The pre-check warns and names the pinning candidates (Priority: P1)

A user runs the suite with `mode=precheck` on a shared host. Each
planned cpu whose gap rate or max gap stands above the threshold gets
one warning line naming the cpu, the field and the figure. After the
window, one line names the planned cpus with the lowest gap rate as
the pinning candidates. The run continues and the benchmarks record.

**Why this priority**: the warning and the pinning line are the
feature's daily value: a user learns which cpu to pin before the run
spends its time.

**Independent Test**: with fake figures that stand one planned cpu
above the threshold and one below, the run prints one warning line,
then the pinning line, then the benchmark tables, and exits 0.

**Acceptance Scenarios**:

1. **Given** a planned cpu above the threshold, **When** the window
   closes, **Then** one warning line names the cpu, the field and the
   figure.
2. **Given** the warnings printed, **When** the disclosure closes,
   **Then** one line names the planned cpus with the lowest gap rate.
3. **Given** every planned cpu below the threshold, **When** the run
   completes, **Then** no warning line prints and the pinning line
   still names the candidates.

### User Story 3 - The pre-check attributes the noise without root (Priority: P2)

A user without root runs the pre-check. Each measured cpu discloses
the window delta of its interrupt count, its softirq counts, and its
context-switch and schedule counts, beside its gap figures, each
figure naming its source. The host settings (`isolcpus`, `nohz_full`,
`rcu_nocbs`, the per-cpu governor, the per-cpu IRQ affinity) stand
disclosed once per run. A source that is missing, unreadable or
unparsable discloses `not available` for that source alone, and the
run continues.

**Why this priority**: the attribution is what turns a warning into a
cause the user can act on, and it must work without root.

**Independent Test**: with fake `/proc` files installed through the
seam, each disclosed delta matches the fake files' before-and-after
difference and names its source; a deleted source reads the
`not available` wording and the exit status stays 0.

**Acceptance Scenarios**:

1. **Given** fake `/proc/interrupts` counts that rise during the
   window on a measured cpu, **When** the block prints, **Then** the
   cpu's interrupt delta matches the fake difference and names its
   source.
2. **Given** a host that states no `nohz_full` setting, **When** the
   host settings print, **Then** that setting reads `not available`
   and the others stand as stated.
3. **Given** an unparsable `/proc/schedstat` line, **When** the block
   prints, **Then** the switch and schedule figures read the
   `not available` wording, the gap figures stand, and the exit
   status is 0.

### User Story 4 - The report carries the pre-check (Priority: P2)

A user reads the console report and the JSON report of a run with the
pre-check. The console block stands before the benchmark tables, one
block per measured cpu, the field names the console probe block
uses. The JSON carries the pre-check record under the moved schema
minor, and `loadReport` reads it back. A run without the pre-check
gives the same console report and the same JSON bytes as master,
apart from the `schemaVersion` value.

**Why this priority**: the record is what a compare run and a stored
report carry; its shape is contract.

**Independent Test**: the JSON of a pre-check run round-trips through
`loadReport`; the byte comparison of a pre-check-free run against
master differs in the `schemaVersion` value alone.

**Acceptance Scenarios**:

1. **Given** a run with `mode=precheck`, **When** the console report
   prints, **Then** the pre-check block stands before the benchmark
   tables, one block per measured cpu.
2. **Given** the JSON document of the run, **When** `loadReport`
   reads it, **Then** the pre-check record stands with the keys the
   schema lists and the order `x-order` states.
3. **Given** a run with no `--jitter-probe` option, **When** its
   console report and JSON are compared byte for byte against
   master's, **Then** the `schemaVersion` value stands as the one
   difference.

### User Story 5 - The docs teach the pre-check (Priority: P3)

A reader opens the root `README.md`, `docs/pages/harness.md` and the
example. The page carries the pre-check section of at most 25 lines,
the option and field detail stands in the User-guide page, one
runnable example prints the block the page shows, and the
quick-reference rows name the members the page reaches.

**Why this priority**: the docs gates bind every feature; the reader
path is a fact the drift gate keeps equal to the code.

**Independent Test**: the example builds and runs, its `.expected`
file matches its printed rows, and the drift, snippet-compile, run
and report-shape gates pass.

**Acceptance Scenarios**:

1. **Given** the root page, **When** the line count of the pre-check
   section is taken, **Then** it stands at 25 lines or fewer.
2. **Given** the example binary, **When** it runs, **Then** its rows
   match the `.expected` file beside it.
3. **Given** the quick-reference rows, **When** the drift gate reads
   them against the public headers, **Then** every segment of every
   row appears.

### Edge Cases

- A misspelled serial word (`searial`) in a `precheck` value: one
  line, exit status 2 (FR-003).
- `precheck=0`, a value that is not a number or a word, or a value at
  or above 2³²: the standing error-table row stands, one line, exit
  status 2 (FR-003).
- `mode=alongside` alone: no pre-check runs, and the report stays
  byte-identical to master's apart from the `schemaVersion` value
  (FR-007, FR-024).
- A measured cpu whose attribution source is missing, unreadable or
  unparsable: `not available` for that source alone, the gap figures
  stand, the exit status stays 0 (FR-018).
- No candidate cpu exists (a 2-cpu host with both cpus planned): the
  pinning line discloses the empty candidate set, and the run
  continues (Assumptions).
- A probe candidate above the threshold in strict mode: no refusal;
  the candidate informs the pinning line alone (FR-015).
- Every planned cpu quiet in strict mode: no refusal, the run
  continues, and the benchmark tables record (FR-014).
- A host whose kernel command line states two of the three settings:
  the stated two stand disclosed, the unstated one reads the
  `not available` wording (FR-017).
- The `machine/tsc` leaf stands missing: the pre-check discloses
  `not available` for the probe and the run continues; with
  `strict` the
  run exits 2, the standing rule of specs/027 that P-1 carries
  (FR-007).

## Requirements *(mandatory)*

### The surface, the version and the schema (A)

- **FR-001** (D-1): No option row shall be added, no option value
  shall move, and no new key shall join the value grammar; the
  `--jitter-probe` option and the standing grammar carry the feature.
- **FR-002** (D-1): The `precheck` key shall carry two accepted
  forms: the millisecond value and the word `serial`; the grammar row
  and the error-table row of `precheck=` in
  `specs/027-jitter-probe/contracts/cli-options.md` shall gain the
  word in the same commit as the parser; the record shape, the
  `idle` preset and the overlap rule rows of that contract shall
  change none.
- **FR-003** (D-1): A `precheck` value that carries neither a number
  nor the serial word, a value out of the standing bounds (`0`, a
  value at or above 2³²), or a misspelled serial word shall print one
  line and exit with status 2, the standing error shape.
- **FR-004**: The project version shall move to `0.21.0`, the next
  free minor at branch time, and the commit that moves it shall
  record the ABI check beside the move.
- **FR-005**: The harness `SOVERSION` shall rise by one only when the
  feature's public ABI changes; the serial form joins the public
  options struct (`include/speedgun-ng/benchmark.hpp:1937`), so the
  recorded check shall state the expected harness `SOVERSION`; the
  archive `SOVERSION` shall stay `4`, and no `SOVERSION` shall go
  down.
- **FR-006** (D-8): The schema minor shall take `1.6`, the next free
  value; the pre-check record's keys shall join
  `docs/schema/report-v1.json`; the top-level `x-order` shall move in
  the same commit.

### The window (B)

- **FR-007** (D-2): `mode=precheck` shall run the full pre-check
  before the first benchmark starts; `mode=both` shall run the
  pre-check and then the alongside phase; `mode=alongside` alone
  shall run no pre-check; no `--jitter-probe` option shall run no
  pre-check.
- **FR-008** (D-2): The pre-check window shall cover all planned cpus
  at the same time; `precheck=<ms>` shall set the window length; the
  default shall stand at 200 ms.
- **FR-009** (D-2): `precheck=serial` shall run one planned cpu at a
  time, each measured cpu taking its own `precheck` window
  (roadmap D-13.6); the serial shape reaches the pre-check phase
  alone, and the alongside phase of `mode=both` stands its standing
  shape.
- **FR-010** (D-2): The window shall measure each planned cpu and
  each probe candidate, the cpus the `idle` preset would take, one
  per L3 group the plans touch, with the standing probe loop, the
  standing threshold and the standing gap buffer.
- **FR-011**: The window shall be bounded by the injected time
  source; no test shall sleep and no test shall read a clock.

### Warnings and the pinning line (C)

- **FR-012** (D-3): Each planned cpu whose gap rate or whose max gap
  stands above the threshold shall receive one warning line naming
  the cpu and the two figures, the gap rate and the max gap, one
  line per cpu; the run shall continue.
- **FR-013** (D-3): After the window, one line shall name every
  planned cpu with its gap rate, the lowest rate first, as the
  pinning candidates; tied cpus stand in cpu order; the line shall
  stand after the warnings.
- **FR-014** (D-4): In strict mode, a planned cpu above the threshold
  shall stand a usage error: the pre-check shall complete its full
  window (every window in serial mode), one line shall then name the
  first noisy planned cpu in cpu order with the gap rate and the max
  gap, and the exit status shall stand at 2, before any benchmark
  starts; the pre-check block stands unprinted, the refusal standing
  the one line of the standing error surface on standard error.
- **FR-015** (D-4): The probe candidates shall never refuse; in
  strict mode they shall inform the pinning-candidate line alone.

### The disclosure without root (D)

- **FR-016** (D-5): Each measured cpu shall disclose, beside its gap
  figures, the window delta of its interrupt count, a delta for each
  softirq kind the host states, and its context-switch and schedule
  counts; each disclosed figure shall name its source; the sources
  shall stand read before and after the window.
- **FR-017** (D-6): The run shall disclose once: the kernel command
  line's `isolcpus`, `nohz_full` and `rcu_nocbs` settings, the
  cpufreq governor of each measured cpu, and the IRQ affinity that
  names each measured cpu; each figure shall stand beside the
  `not available` wording where the host does not state it.
- **FR-018**: A source that is missing, unreadable or unparsable
  shall disclose `not available` for that source alone; the run
  shall never fail on it.

### The seam and the test surface (E)

- **FR-019** (D-7, AC-b1): Every `/proc` and `/sys` read of the
  pre-check shall go through one file-root seam; the tests shall
  install fake files into the seam; CI shall need no real noise and
  no real PMU.
- **FR-020** (D-7): The seam shall stand as a named detail seam; the
  N13d and N13e lanes shall reuse it.
- **FR-021**: No fixed cpu number shall stand in code or in a test;
  the cpus shall come from `sched_getaffinity`; the suite shall pass
  under `taskset -c 0,1` and on a 2-cpu host.
- **FR-022**: A timing bound in the tests shall sit an order of
  magnitude inside the contamination signal it discriminates
  against; the margin shall stand recorded beside the bound, the
  record of `specs/032-fix-merged-features/results/record.md`
  standing as the precedent.

### The report (F)

- **FR-023** (D-8): The pre-check shall print its block before the
  benchmark tables, one block per measured cpu, the field names the
  console probe block uses; the attribution deltas shall stand beside
  the gap figures in the block.
- **FR-024** (D-8): The JSON shall gain the pre-check record;
  `loadReport` shall read it back; a run without the pre-check shall
  write the same console report and the same JSON bytes as master,
  apart from the `schemaVersion` value.
- **FR-025**: The `sg::NoiseWarning` disclosure shall stand beside
  the pre-check block; the feature shall replace none of it.

### Documentation and example (G)

- **FR-026** (D-9): The root `README.md` shall gain a pre-check
  section of at most 25 lines; `docs/pages/harness.md` shall carry
  the option and field detail; one runnable example under
  `example/docs/` shall ship with its `.expected` file; the
  quick-reference rows shall stand.

### Gates (H)

- **FR-027**: Coverage shall read 100% lines and 100% branches; no
  `LCOV_EXCL` marker shall join and no new `--ignore-errors` entry
  shall join.
- **FR-028**: No lambda shall stand inside an `SG_*` macro;
  contract-only helpers shall carry `[[maybe_unused]]`; simdjson
  shall stand the only JSON reader the change uses.

## Key Entities *(include if feature involves data)*

- `JitterProbeOptions`
  (`include/speedgun-ng/benchmark.hpp:1937`): the public options
  struct; the serial form joins it, and the edit is the public-ABI
  candidate the ABI check of FR-005 records.
- The file-root seam (new detail seam, `source/harness/detail/`):
  the one root every `/proc` and `/sys` read of the pre-check goes
  through; the tests install fake files into it (D-7).
- The pre-check record (`docs/schema/report-v1.json`, the writer and
  `loadReport`): the JSON record the run carries beside the jitter
  record; its keys and order stand in the schema (D-8).
- `sg::NoiseWarning`
  (`include/speedgun-ng/benchmark.hpp:668`): the standing disclosure
  beside the pre-check block; unchanged (FR-025).
- The planned cpus and the probe candidates: the planned cpus come
  from the execution-context plans; the candidates come from the
  `idle` preset resolution of specs/027 D-4, one per L3 group the
  plans touch.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The strict case stands: with fake figures above the
  threshold on a planned cpu, the run prints one line and exits 2,
  and no benchmark row prints.
- **SC-002**: The warning and pinning cases stand: from fake files,
  each noisy planned cpu prints one warning line, the pinning line
  stands after the warnings, and the run exits 0.
- **SC-003**: The attribution cases stand: from fake files, each
  delta matches the fake before-and-after difference and names its
  source; a missing source reads `not available`, and the exit status
  stays 0.
- **SC-004**: The record round-trips: `loadReport` reads the pre-check
  record of a written document, and the schema carries the new keys
  with the moved `x-order`.
- **SC-005**: Coverage reads 100.0% lines and 100.0% branches; no
  `LCOV_EXCL` marker and no new `--ignore-errors` entry stands.
- **SC-006**: The suite passes under `taskset -c 0,1` and on a
  2-cpu host.
- **SC-007**: A pre-check-free run's console report and JSON match
  master's byte for byte, apart from the `schemaVersion` value.
- **SC-008**: The gates pass: `example_drift`, `snippet_compile`,
  `example_run`, `report_shape`, `prose-lint`, `spell-check`, and the
  sanitizer and thread-sanitizer jobs.

## Constraints

The request's hard rules bind this spec; they restate constitution
VI, VII and VIII:

- Coverage stays 100% lines and 100% branches; no `LCOV_EXCL` marker,
  no waiver, no new `--ignore-errors` entry; no code shaped for
  coverage.
- No sleep and no clock in a test: the window is bounded by the
  injected time source, and the attribution deltas come from the
  injected files.
- No fixed cpu numbers: the cpus come from `sched_getaffinity`, and
  the suite passes under `taskset -c 0,1` and on a 2-cpu host.
- A timing bound in the tests sits an order of magnitude inside the
  contamination signal it discriminates against; the margin stands
  recorded (FR-022), the record of specs/032 standing as the
  precedent.
- No lambda inside an `SG_*` macro; `[[maybe_unused]]` on
  contract-only helpers; simdjson only for JSON.
- A `SOVERSION` rises only for a public-ABI change; the archive
  `SOVERSION` stays `4`; the check rides the version commit.
- The roadmap's section 10.6 rules stay in force in full.
- Each new commit follows the commit template and passes
  `prose-lint`.

## Assumptions

- The JSON placement of the pre-check record: the record stands as a
  top-level object named `precheck`, after `jitter` and before
  `benchmarks`. D-8 fixes the record, the schema rule and the minor
  move; it names no placement, so the record follows the jitter
  record's own pattern. A different placement is a plan decision
  recorded against the same schema rule.
- The serial form joins the public options struct; the exact field
  shape (a flag beside `precheckMs`, or a sum type over the two
  forms) is a plan decision. The ABI check of FR-005 records the
  outcome either way.
- The pinning line with no candidate cpu discloses the empty
  candidate set and the run continues; the candidates inform alone
  (D-4), and a 2-cpu host plans both cpus.
- In serial mode the planned cpus run before the candidates, and each
  cpu follows the ascending order of its list. D-2 fixes the
  one-at-a-time shape; the order is an informed default.
- The roadmap stands outside the repository; its sections 10.1 and
  10.6 rules are read from the lane record the request carries.
- `gh` carries no token in this session; the open-PR check stands on
  the directory and remote-branch checks (Spec identity).
- The request names the lane host; the spec carries the hardware
  shape alone (Lane identity), the file-hygiene rule of specs/032
  FR-024 binding.

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes user-visible
  behavior, so the root `README.md` section, its `example/` region
  and the quick-reference rows land in the same change (FR-026), and
  the `example_run` gate covers the example.
- Principles VI and VIII: the coverage bar, the determinism rules and
  the gate set are restated as binding requirements (Constraints).
- Principle IX: the full workflow applies; the change touches
  behavior (a new phase, refusals, report shape) and possibly a
  public header (`JitterProbeOptions`).
- Principle X.1: the two open readings (the serial token against the
  "no new value" clause; the JSON placement) are recorded with both
  candidates and the chosen one (D-1, Assumptions).
- Principle X.3: each commit touches the files its requirement names,
  and nothing else.
- Principle XI: this file holds no em-dash, no contrastive framing,
  no filler vocabulary and no vouching word, and every command and
  option sits in a code span.
