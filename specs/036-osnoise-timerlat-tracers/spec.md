# Feature Specification: Osnoise and Timerlat Tracer Integration

**Feature Branch**: `036-osnoise-timerlat-tracers`

**Created**: 2026-10-29

**Status**: Draft

**Input**: User description, opening quoted:

> Extend the host-noise work, item N13e of the roadmap (section 4,
> split from H13), with the kernel's own noise tracers: `osnoise` and
> `timerlat` run through tracefs on the probe cpus while the pre-check
> measures, and the noise each source reports (NMI, IRQ, softirq,
> thread, hardware) lands beside the figures the user-space probe
> already prints. The user-space probe cannot see a kernel source; the
> tracer can.

The request fixes the rulings T-1 to T-5 of the lane prompt; they
stand in Decisions as D-1 to D-7, the two extra decisions covering the
shapes the prompt leaves open (the tracer cpu set and the run shape
without a pre-check window).

## Spec identity and run-time binding

- Spec number: `036`. `specs/` on master `5443448e` holds `001` through
  `035` (read 2026-10-29). `031` is reserved for N0b host validation,
  `033` for N14a-2 hunter budgets, whose pull request stands open on
  the remote branch `033-hunter-budgets`. `git ls-remote --heads` names
  no ref carrying `036` or `osnoise-timerlat` (read 2026-10-29). The
  `gh pr list` check did not run: `gh` carries no token in this
  session, so the pull-request list stands on the directory and
  remote-branch checks and on the lane prompt's own record.
- Branch: `036-osnoise-timerlat-tracers`, cut from master `5443448e`
  on 2026-10-29. Master stands on that hash at branch time, so no
  moved-base record applies.
- Version: master stands at `0.22.0` (`CMakeLists.txt:19`), harness
  `SOVERSION 13` (`CMakeLists.txt:302`), archive `SOVERSION 4`
  (`CMakeLists.txt:84`). The lane takes the next free minor,
  `0.23.0`. The tracer flag joins the public `JitterProbeOptions`
  struct (`include/speedgun-ng/benchmark.hpp:2011`) and the tracer
  record joins the public `PrecheckRecord` type
  (`include/speedgun-ng/benchmark.hpp:563`), so the exported layouts
  move: the harness `SOVERSION` rises from `13` to `14`, the archive
  `SOVERSION` stays `4`, and the commit that moves the numbers
  records the check beside them (FR-004, D-6).
- Report schema: master stands at `1.6` beside
  `source/harness/json_report.cpp:36` (`kSchemaVersion`), the
  top-level `x-order` at `docs/schema/report-v1.json:7`. The
  pre-check record the tracer feeds stands at `1.6` (the `precheck`
  object of `docs/schema/report-v1.json`). The tracer record gains
  keys the reader must carry, so the schema minor takes the next
  free value, `1.7`; the top-level `x-order` stays, and the
  `precheck` object's `x-order` moves in the same commit (D-3,
  FR-006).
- Option values: the tracer joins the standing option table as one
  new row, `--os-noise`, held as the named constant at `297`, the
  first free value of the block `297` to `319` the option master
  records (`source/harness/detail/options.hpp:76`). No standing name
  moves and no standing value moves. The name and value uniqueness
  test file carries the row (`test/source/harness_cli_table_test.cpp`,
  the `noNameAndNoValueRepeat` check at `:780` and the
  `theValueBlocksStayUniqueAndDisjoint` check at `:851`).

## Roadmap position

- Item N13e of the harness roadmap, section 4, split from H13. N13b
  (the host noise pre-check, spec 034) is merged, and this lane
  reuses the file-root seam spec 034 landed at
  `source/harness/detail/proc_root.hpp`. N14a-2 (spec 033) stands
  open. Roadmap section 10.6 stays in force in full.

## Lane identity

- The acceptance shape stands on the request's lane host: a
  Threadripper 9960X, 24 cores, 48 threads, one socket, several CCDs
  with one L3 each. The many-CCD shape is the acceptance axis: the
  tracer runs on one probe cpu per L3 group, and the acceptance
  records name the CCD each figure came from. Root is optional: the
  feature states what it can read and never refuses a host that runs
  without root.
- The suite itself stays host-agnostic: the tracefs root and the
  probe cpus come from injected seams, and the suite passes on a
  host with no kernel tracer and no root (FR-017, FR-018). The
  request names the host; the spec carries the shape alone, the
  file-hygiene rule of `specs/032-fix-merged-features` FR-024
  binding repository files.

## Audit point

- Repository: `master` at `5443448e` (project version `0.22.0`,
  report schema `1.6`, harness `SOVERSION 13`, archive
  `SOVERSION 4`), read 2026-10-29 on the lane host.
- The pre-check console block stands at `source/harness/report.cpp:986`
  (`printPrecheckReport`): one block per measured cpu with the
  sixteen field names the console probe block uses, the attribution
  deltas beside the gap figures each naming its source, the host
  settings once, then the warning lines and the pinning-candidate
  line.
- The pre-check record stands public at
  `include/speedgun-ng/benchmark.hpp:548` (`PrecheckCpuRecord`:
  `cpu`, `planned`, `stats`, `attribution`) and
  `include/speedgun-ng/benchmark.hpp:563` (`PrecheckRecord`:
  `serial`, `windowMs`, `cpus`, `host`, `skippedGroups`); the
  document carries it under the top-level `precheck` key
  (`docs/schema/report-v1.json`, the `precheck` object), and
  `loadReport` reads it back.
- The pre-check's probe candidates stand at one cpu per L3 group the
  plans touch, the `idle` preset resolution of
  `specs/027-jitter-probe/contracts/cli-options.md` (D-4); the
  pre-check record names their role `candidate`.
- The `--jitter-probe` value grammar stands at
  `specs/027-jitter-probe/contracts/cli-options.md:23`; the cpu-list
  rules (indexes and closed `a-b` ranges, comma separated, no cpu
  standing twice, inside the process affinity mask and the online
  set) stand in the grammar table, the key-once rule and the error
  surface at `:64` to `:115`.
- The standing option table stands at `source/harness/options.cpp:707`
  (the `--jitter-probe` row, the row shape every row follows); the
  option master records the value blocks at
  `source/harness/detail/options.hpp:69` to `:101`, with the block
  `297` to `319` recorded free at `:76`.
- The file-root seam stands at `source/harness/detail/proc_root.hpp:35`
  (`procRootPath`) and `:52` (`installTestProcRoot`): every `/proc`
  and `/sys` read of the pre-check resolves under the root the seam
  installs, the tests install fake files into it, and its contract
  names the N13d and N13e lanes as the reusers (specs/034 FR-019,
  FR-020, D-7).
- The results-evidence precedent stands at
  `specs/035-numa-placement/results/`: one record per run naming the
  setting, the boot, the commands and the exit codes, the quickstart
  linking each record.
- The timing-margin record stands at
  `specs/032-fix-merged-features/results/record.md`: a test bound
  that walks near the sanitizer's own load reads flaky, and a bound
  keeps an order of magnitude of margin inside the contamination
  signal it discriminates against.

## Standing facts this lane reads

- The pre-check block, the pinning candidates and the attribution
  lines of spec 034 stand unchanged; the tracer figures join them
  and amend only the pre-check contract line that names the noise
  sources (`specs/034-host-noise-precheck/contracts/console-report.md:36`).
- The tracefs root rides the `proc_root` seam of spec 034; tracefs
  sits under `/sys`, so the root the seam carries resolves the
  standing `/sys/kernel/tracing` path. The lane adds no dependency
  and no second seam (D-1).
- The option-table contract of
  `specs/015-benchmark-harness-core/contracts/cli.md` carries the
  standing rows; the lane amends it with the one new row and nothing
  else.
- The report-json contract of spec 034
  (`specs/034-host-noise-precheck/contracts/report-json.md`) carries
  the pre-check record; the lane amends it with the tracer keys.
- The roadmap's section 10.6 rules stay in force in full.

## Clarifications

### Session 2026-10-29

- Q: What do the noise-by-source figures name when the pre-check
  runs in serial mode: the engine's cumulative totals across every
  window, or one figure set per window? → A: The engine's cumulative
  totals since the tracer's start, read once after the measurement
  ends; the standing `serial` and `windowMs` fields name the window
  shape beside them (FR-009). The kernel's stats files state
  cumulative counts, so the run carries no reset write between
  windows and no second record shape.
- Q: Does the tracer run across the pre-check phase's window alone
  when the value carries `mode=both`, or across the alongside phase
  too? → A: The pre-check phase's window alone; the alongside phase
  runs tracer-free (FR-009). The tracer's own load perturbs the cpus
  it runs on, and the alongside phase measures the planned cpus
  while benchmarks run.
- Q: What happens when the `cpus=` key names a planned cpu: the run
  refuses, a warning line prints, or the list runs as named? → A:
  The list runs as named; the docs state beside the option that the
  tracer's own load perturbs the cpus it runs on (FR-008, FR-021).
  The value is an explicit user choice the run can honor, and the
  standing overlap refusal binds the auto-resolved alongside probe,
  a different surface.
- Q: Does the runtime JSON record name the CCD each tracer cpu
  belongs to, or the cpus alone? → A: The cpus alone; the CCD
  mapping stands in the acceptance evidence records (FR-014,
  FR-020). The runtime record carries the keys the run itself
  states, and the mapping comes from the `--topology` listing the
  evidence script reads.
- Q: Does the refusal of FR-011 cover a run that carries no
  `--jitter-probe` option at all? → A: The same one-line refusal
  with exit status 2 covers it (FR-011). The option names a
  measurement across a window the run does not carry, and a
  standalone tracer window would add the second timing window the
  lane keeps out of scope.

## Decisions

Decisions the request fixes for this feature, recorded as
requirements:

- **D-1: the access rides the standing seam (T-1).** The tracer runs
  through tracefs on the `proc_root` seam of spec 034, root
  optional, no new dependency. The tracefs root is the standing
  `/sys/kernel/tracing` path resolved under the seam's root; a host
  that carries no tracefs mount at that path, no tracer engine, or
  no rights for the tracefs setup writes stands `not available`, and
  the run never fails on it (FR-016).
- **D-2: the cpu set and the window (T-2).** The tracer runs on the
  pre-check's candidate cpus, one per L3 group the plans touch; the
  `cpus=` key names them when the value carries it (FR-008). The
  planned cpus stand outside the tracer's cpu set: the tracer's own
  load perturbs the cpus it runs on, and the planned cpus' gap
  figures are the ones the strict gate refuses on. The run sets the
  tracer before the pre-check measurement starts, runs it across the
  window the standing `--jitter-probe=precheck` grammar already
  owns, and reads the figures after the measurement ends: no second
  timing window and no new probe context. An `--os-noise` value on a
  run whose mode runs no pre-check window refuses one line with
  exit status 2 before anything runs: the option names a measurement
  across a window the run does not carry (FR-011).
- **D-3: the record (T-3).** The noise-by-source figures ride the
  standing pre-check block beside the figures spec 034 prints today,
  console and JSON, with the tracer engine's name and the cpus it
  ran on disclosed (FR-012, FR-013, FR-014). The JSON tracer record
  stands optional: a run that requests no tracer and a run whose
  tracer stands not available write the same JSON bytes as master
  apart from the `schemaVersion` value and the version figures the
  report carries (FR-015); the standing console lines stay
  byte-identical, and the not-available run's tracer lines stand the
  disclosure FR-016 names. The schema minor moves to `1.7` because
  the record gains keys the reader must carry, and the move stands
  recorded beside the version move (FR-006).
- **D-4: the grammar (T-4).** One option row, `--os-noise`, one value
  in the grammar `osnoise|timerlat[,cpus=<cpu-list>]`: one engine
  name, the two the kernel's tracing tree carries, and one optional
  `cpus` key whose value the standing cpu-list parser reads, each
  key standing once, the standing refusal shape for a bad value
  (FR-002, FR-003).
- **D-5: the not-available disclosure (T-1).** A host without
  tracefs, without the tracer engine, or without root prints the
  standing `not available` disclosure for the tracer lines alone and
  keeps the user-space pre-check whole: the exit status stays 0, and
  no refusal stands on the host's state. The standing not-available
  rule of spec 034 (FR-018 there) extends to the tracer lines
  (FR-016).
- **D-6: the ABI check.** The tracer flag joins the public
  `JitterProbeOptions` struct beside the standing option sources
  (the command line, then the option file, then the suite call, the
  source order spec 027 fixes), so the exported layout moves and the
  harness `SOVERSION` rises from `13` to `14`. The archive
  `SOVERSION` stays `4`. The commit that moves the numbers records
  the check, the way the `SOVERSION` comments of `CMakeLists.txt`
  record the ABI list of each lane that moved one.
- **D-7: the evidence (T-5).** The per-CCD acceptance runs stand
  recorded before the lane closes, under the results directory, the
  way spec 035 recorded the NPS evidence: one record per run naming
  the commands, the exit codes, the engine and the per-CCD tables,
  one probe cpu per L3 group, each recorded figure naming the CCD
  and the tracer engine it came from. A script records it, and no
  CI gate stands on it (FR-020).

## Out of scope

- The N13d lane and the N14a-2 hunter budgets (spec 033): both stand
  reserved or open, and neither moves here.
- A tracer tuning key (a threshold, a window length, a trace buffer
  shape): the grammar carries the engine and the cpu list alone; a
  tuning key is a follow-up with its own grammar record.
- Tracer output lines and trace markers: the harness reads the
  noise-by-source figures the engine's count files state and parses
  no trace line.
- Changes to the standing pre-check lines, the warning rule, the
  pinning-candidate line, the host-settings lines and the
  attribution lines: the tracer figures join beside them.
- Changes to the strict refusal rule: the strict gate keeps gating
  the planned cpus' gap figures alone, and the tracer figures gate
  nothing.
- A root requirement or a privilege path: the feature never refuses
  a host that runs without root and never asks for rights.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The tracer figures land beside the pre-check figures (Priority: P1)

A user runs the suite with `--jitter-probe=idle,mode=precheck` and
`--os-noise=osnoise` on a multi-CCD host. The tracer runs on one
candidate cpu per L3 group across the pre-check window, and the
pre-check block prints the noise-by-source figures beside the gap
figures, naming the engine and the cpus. The user-space probe cannot
see a kernel source; the tracer's figures name them.

**Why this priority**: the figures beside the standing block are the
feature's own value; without them the pre-check names no kernel
noise source.

**Independent Test**: with an injected tracefs root carrying fake
engine files and an injected probe seam, a run with the option prints
the tracer lines beside the standing block, and the JSON document
carries the figures the reader reads back.

**Acceptance Scenarios**:

1. **Given** an injected tracefs root with fake per-cpu count files
   and a run with `--os-noise=osnoise`, **When** the pre-check
   measurement ends, **Then** the block prints the engine's name,
   the cpus it ran on, and one figure per source per tracer cpu
   beside the standing figures.
2. **Given** the same run, **When** the JSON document is written and
   read back with `loadReport`, **Then** the document carries the
   tracer record and the reader returns it.

### User Story 2 - A host without the tracer keeps the run whole (Priority: P1)

A user runs the suite with the option on a host without tracefs,
without the tracer engine, or without root. The tracer lines print
the standing `not available` disclosure, the user-space pre-check
stands whole, and the run exits 0.

**Why this priority**: root is optional on the lane host and the
pre-check never fails on a host state; the disclosure names the
source the run could not read.

**Independent Test**: with an injected tracefs root that names no
engine files, the run prints the disclosure on the tracer lines
alone, keeps every standing line byte-identical, and exits 0.

**Acceptance Scenarios**:

1. **Given** an injected tracefs root with no tracer engine, **When**
   the run carries `--os-noise=timerlat`, **Then** the tracer lines
   print `not available`, the standing block stands unchanged, and
   the exit status is 0.
2. **Given** an injected tracefs root whose files deny the setup
   writes, **When** the run carries `--os-noise=osnoise`, **Then**
   the tracer lines print `not available` and the exit status is 0.

### User Story 3 - A bad value refuses before anything runs (Priority: P2)

A user mistypes the value. The run refuses one line on standard
error with exit status 2 before anything runs, the standing refusal
shape of the option surface.

**Why this priority**: the refusal shape is the standing contract
every option row keeps; the tracer row joins it.

**Independent Test**: with each invalid value of the error table,
the run prints one line naming the fault and exits 2 with no
benchmark output.

**Acceptance Scenarios**:

1. **Given** `--os-noise=hrtimers`, **When** the batch starts,
   **Then** one line names the engine word and the exit status is 2.
2. **Given** `--os-noise=osnoise,cpus=0,cpus=1`, **When** the batch
   starts, **Then** one line names the repeated key and the exit
   status is 2.
3. **Given** `--os-noise=osnoise` on a run whose mode runs no
   pre-check window, **When** the batch starts, **Then** one line
   names the missing pre-check window and the exit status is 2.

### User Story 4 - The acceptance runs stand recorded per CCD (Priority: P2)

The lane records the tracer runs on thrawn under the results
directory: one record per run naming the commands, the exit codes,
the engine and the per-CCD tables, one probe cpu per L3 group, each
recorded figure naming the CCD and the engine it came from. A later
lane reads the records and knows which engine and which CCD each
figure carries.

**Why this priority**: the many-CCD shape is the acceptance axis the
roadmap names, and the evidence is what the later lanes read.

**Independent Test**: the results directory carries one record per
acceptance run, and the quickstart links each record with its
commands.

**Acceptance Scenarios**:

1. **Given** the acceptance runs execute on thrawn, **When** the
   lane closes, **Then** each record names the commands, the exit
   codes, the engine, and the per-CCD tables with the CCD named
   beside each figure.
2. **Given** the recorded evidence, **When** the quickstart is read,
   **Then** it links every record.

### User Story 5 - The docs teach the tracer (Priority: P3)

A reader of the README and the guide page learns the `--os-noise`
row, the disclosure rule and the record from the same sections that
teach the pre-check, and the runnable example shows a run whose
bytes the shape gate guards.

**Why this priority**: the docs gates bind every feature, and the
drift gates fail a page whose quoted run drifts away from what the
binary prints.

**Independent Test**: the example builds and runs, its `.expected`
file matches the run through the shape fold, and the snippet
compiles.

**Acceptance Scenarios**:

1. **Given** the guide page's recorded example run, **When** the
   shape gate runs the example binary, **Then** the quoted block
   folds equal against the run.
2. **Given** the README section, **When** the prose gate runs,
   **Then** the section stands inside the 25-line cap with no
   finding.

### Edge Cases

- A host with no tracefs mount at the standing path: the tracer
  lines print `not available` and the exit status stays 0.
- A host with tracefs but no tracer engine: the same disclosure on
  the tracer lines alone.
- A host with the engine but no rights for the setup writes: the
  same disclosure; the feature never asks for root.
- An `--os-noise` value on a run whose mode runs no pre-check
  window: one refusal line, exit status 2, before anything runs.
- A `cpus=` list naming a cpu outside the process affinity mask or
  the online set, or a cpu standing twice in the list: the standing
  refusal shape, one line, exit status 2.
- A tracer cpu whose count files state nothing at read time: that
  cpu's lines print `not available` for the missing sources alone,
  the standing not-available rule.
- A pre-check in serial mode: the tracer stays set across the
  windows the mode owns, the figures name the noise the tracer cpus
  saw across the sequence, and the record's standing `serial` and
  `windowMs` fields name the window shape beside them.
- A pre-check in `mode=both`: the stop write and the figure read
  land behind the latch, the planned cpus' alongside figures stand
  unperturbed by the default candidate set, and the figures stand
  the engine's cumulative totals since the tracer's start (FR-009).

## Requirements *(mandatory)*

### The option row and the grammar (A)

- **FR-001**: The option table SHALL carry one new row, `--os-noise`,
  taking one required value, held as the named constant at the value
  `297`; no standing name SHALL move and no standing value SHALL
  move, and the name and value uniqueness test SHALL carry the row.
- **FR-002**: The value grammar SHALL read
  `--os-noise=osnoise|timerlat[,cpus=<cpu-list>]`: one engine name,
  `osnoise` or `timerlat`, then one optional `cpus` key whose value
  the standing cpu-list parser reads, the form the `--topology`
  listing prints, no cpu standing twice in the list; each key SHALL
  stand once, and a second occurrence of one key SHALL be an
  invalid value (T-4, D-4).
- **FR-003**: A bad value SHALL refuse one line on standard error
  with exit status 2 before anything runs: an engine word outside
  the two, a `cpus` value outside the standing cpu-list rules, and a
  repeated key each name their fault; a bad value from an option
  file line SHALL name the path, the line and the token, the
  standing file-fault shape.
- **FR-004**: The version SHALL stand at `0.23.0` with the harness
  `SOVERSION` at `14` and the archive `SOVERSION` at `4`, and the
  commit that moves the numbers SHALL record the ABI check (D-6).
- **FR-005**: The tracer flag SHALL join the public
  `JitterProbeOptions` struct, and the suite entry points SHALL bind
  it beside the standing option sources: the command line's value,
  then the option file's, then the suite call's, the source order
  spec 027 fixes (D-6).
- **FR-006**: The report schema minor SHALL stand at `1.7`; the
  `precheck` object's `x-order` SHALL move in the same commit, and
  the top-level `x-order` SHALL stay unchanged (D-3).

### The tracer run (B)

- **FR-007**: The tracer SHALL run through the tracefs root the
  standing `/sys/kernel/tracing` path names, resolved under the
  `proc_root` seam of spec 034; the lane SHALL add no dependency, no
  second seam and no mount-point search (T-1, D-1).
- **FR-008**: The tracer cpus SHALL be the cpus the `cpus=` key
  names, and the pre-check's candidate set, one cpu per L3 group the
  plans touch, when the key stands absent; every tracer cpu SHALL
  stand inside the process affinity mask and the online set, the
  standing parser rules (T-2, D-2). A `cpus=` list naming a planned
  cpu SHALL run as named, and the run SHALL not refuse the overlap;
  the docs carry the perturbation note beside the option (FR-021).
- **FR-009**: The run SHALL set the tracer engine before the
  pre-check measurement starts, run it across the pre-check window
  the standing grammar owns (every window in serial mode), and read
  the noise-by-source figures after the measurement ends; no second
  timing window SHALL join and no new probe context SHALL join
  (T-2, D-2). In `mode=both` the tracer runs across the pre-check
  phase's window alone: the stop write and the figure read land
  behind the latch the alongside phase starts behind, the default
  candidate set stands outside the planned cpus, and the planned
  cpus' alongside figures stand unperturbed; a `cpus=` list naming a
  planned cpu carries the perturbation note the docs state (FR-021).
  The figures SHALL be the engine's cumulative totals since the
  tracer's start, read once after the measurement ends; the run SHALL
  write no engine reset between serial windows, and the record's
  standing `serial` and `windowMs` fields name the window shape
  beside them.
- **FR-010**: The noise-by-source figures SHALL name the five sources
  the tracer engine reports: NMI, IRQ, softirq, thread and hardware;
  a source the engine states no figure for SHALL print the standing
  `not available` wording for that source alone, the rule of spec 034.
- **FR-011**: An `--os-noise` value on a run whose jitter-probe mode
  runs no pre-check window, and on a run that carries no
  `--jitter-probe` option at all, SHALL refuse one line with exit
  status 2 before anything runs (D-2).

### The record (C)

- **FR-012**: The tracer figures SHALL ride the standing pre-check
  block beside the figures spec 034 prints today, console and JSON;
  the standing pre-check lines, the warning lines, the
  pinning-candidate line and the host-settings lines SHALL stay
  byte-identical (T-3, D-3).
- **FR-013**: The console SHALL disclose the tracer engine's name
  and the cpus it ran on beside one figure line per source per
  tracer cpu, the sources in the NMI, IRQ, softirq, thread and
  hardware order.
- **FR-014**: The JSON document SHALL carry the optional tracer
  record inside the standing pre-check record, naming the engine,
  the cpus it ran on, and one entry per tracer cpu with the five
  figures; `loadReport` SHALL read it back (T-3, D-3). The record
  names cpus alone, and the CCD mapping stands in the acceptance
  evidence (D-7).
- **FR-015**: A run that requests no tracer, and a run whose tracer
  stands not available, SHALL write the same JSON bytes as master
  apart from the `schemaVersion` value and the version figures the
  report carries; every standing console line SHALL stay
  byte-identical, and the not-available run's tracer lines SHALL
  stand the disclosure FR-016 names (D-3).

### The not-available rule and the seams (D)

- **FR-016**: A host without tracefs at the standing path, without
  the tracer engine, or without the rights the tracefs setup writes
  need SHALL print the standing `not available` disclosure on the
  tracer lines alone, keep the user-space pre-check whole, and exit
  0; the run SHALL never fail on the tracer's absence (T-1, D-5).
- **FR-017**: Every tracefs read and write SHALL resolve under the
  injected root; the tests SHALL install fake tracefs files through
  the seam, and CI SHALL need no kernel tracer and no root
  (specs/034 FR-019, the seam contract).
- **FR-018**: No fixed cpu number and no fixed L3 group number SHALL
  stand in code or in a test; the tracer cpus SHALL come from the
  standing candidate resolution over the injected topology, and the
  suite SHALL pass under `taskset -c 0,1` and on a 2-cpu host.
- **FR-019**: No test SHALL sleep and no test SHALL read a clock:
  the tracer figures come from the injected files, and the timing
  bounds the suite keeps stand an order of magnitude inside the
  contamination signal they discriminate against (the record of
  `specs/032-fix-merged-features/results/record.md`).

### The evidence and the docs (E)

- **FR-020**: The results directory SHALL carry the acceptance runs
  on thrawn before the lane closes: one record per run, naming the
  commands, the exit codes, the engine and the per-CCD tables, one
  probe cpu per L3 group, each recorded figure naming the CCD and
  the tracer engine it came from, the CCD names read from the
  `--topology` listing the same host prints; the quickstart SHALL
  link every record. A script records it, and no CI gate stands on
  it (T-5, D-7).
- **FR-021**: The README SHALL carry a section of at most 25 lines
  teaching the tracer option, its disclosure rule and the
  perturbation note for a `cpus=` list naming a planned cpu, and the
  quick-reference table SHALL carry the rows.
- **FR-022**: `docs/pages/harness.md` SHALL carry the option and
  field detail beside the pre-check sections spec 034 landed.
- **FR-023**: One runnable example under `example/docs/` SHALL carry
  a run with the tracer figures beside the pre-check block, with its
  `.expected` file, and the shape gate SHALL fold it against the
  binary's own run.

### The standing contracts (F)

- **FR-024**: The option-table contract of
  `specs/015-benchmark-harness-core/contracts/cli.md` SHALL gain the
  `--os-noise` row; the console-report contract of spec 034 SHALL
  gain the tracer lines beside the standing block; the report-json
  contract of spec 034 SHALL gain the tracer keys; the report-shape
  contract of spec 034 SHALL gain the fold rows the tracer lines
  need; no other standing rule SHALL move.

### The gates (G)

- **FR-025**: Coverage SHALL read 100% lines and 100% branches; no
  `LCOV_EXCL` marker SHALL join and no new `--ignore-errors` entry
  SHALL join.
- **FR-026**: No lambda SHALL stand inside an `SG_*` macro;
  contract-only helpers SHALL carry `[[maybe_unused]]`; simdjson
  SHALL stand the only JSON reader the change uses (the standing
  gate shape of spec 034 FR-028).

## Key Entities *(include if feature involves data)*

- `JitterProbeOptions`
  (`include/speedgun-ng/benchmark.hpp:2011`): the public options
  struct; the tracer flag joins it, and the join is the public-ABI
  change the check of FR-004 records (D-6).
- `PrecheckRecord` and `PrecheckCpuRecord`
  (`include/speedgun-ng/benchmark.hpp:563`, `:548`): the public
  pre-check record; the optional tracer record joins
  `PrecheckRecord` with the engine, the cpus and the per-cpu figures
  (D-3).
- The `proc_root` seam (`source/harness/detail/proc_root.hpp:35`,
  `:52`): the one root every `/proc` and `/sys` read of the
  pre-check resolves under; the tracefs reads and writes resolve
  under it too, and the tests install fake tracefs files into it
  (D-1).
- The tracer record (`docs/schema/report-v1.json`, the `precheck`
  object): the optional JSON record with the engine, the cpus and
  the per-cpu figures; its keys and order stand in the schema (D-3).
- The option row and its value constant
  (`source/harness/detail/options.hpp`): the standing table's one
  new row at `297`, the block `297` to `319` recorded beside the
  standing blocks.
- The candidate cpus: the pre-check's own per-CCD selection, one cpu
  per L3 group the plans touch, the role the pre-check record names
  `candidate` (specs/027 D-4).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With an injected tracefs root and fake engine files, a
  run with `--os-noise=osnoise` prints the engine's name, the cpus
  and one figure per source per tracer cpu beside the standing
  pre-check block, and the standing lines fold byte-identical to
  master apart from the version figures the report carries.
- **SC-002**: The JSON document carries the optional tracer record,
  `loadReport` reads it back, and a run that requests no tracer
  stands byte-identical to master apart from the `schemaVersion`
  value and the version figures the report carries.
- **SC-003**: Every refusal of FR-003 and FR-011 prints one line and
  exits with status 2, and no benchmark output stands ahead of the
  refusal.
- **SC-004**: A host without tracefs, without the engine, or without
  the setup rights reads the `not available` disclosure on the
  tracer lines alone, and every such run exits 0.
- **SC-005**: The coverage gate reads 100.0% lines and 100.0%
  branches, with no `LCOV_EXCL` marker and no new `--ignore-errors`
  entry.
- **SC-006**: The acceptance evidence stands recorded under the
  results directory for the engines the lane exercises, each record
  naming the CCD and the engine beside its figures, and the
  quickstart links every record.
- **SC-007**: The docs gates stand green: the example drifts from
  nothing, the snippet compiles, and the prose gate reads no
  finding.

## Assumptions

- The engine names `osnoise` and `timerlat` name the two engines the
  kernel's tracing tree carries; a host whose tree names neither
  stands not available for the engine the value names.
- The tracefs root stands at the kernel's documented default path;
  a host that mounts tracefs elsewhere stands not available, and no
  mount-point search joins (X.2).

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes what the root
  `README.md` teaches, so the page and its `example/` regions move
  in the same change (FR-021, FR-023).
- Coverage (VI): 100% line and branch coverage, no exclusions, the
  timing-margin record stands beside the test bounds (FR-019).
- Performance (VII): the tracer adds no timed-path work: the setup
  and the read run outside every timed window, around the pre-check
  window the standing grammar owns.
- Prose (XI): every generated line clears the prose gate; the code
  spans stand on one line.

## Phase: N13e trace-line parsing (owner ruling G-28, 2026-11-09)

**Input**: Owner order of 2026-11-09, on thrawn. The acceptance records
of this lane (`results/osnoise.md`, `results/timerlat.md`) found the
pinned count-file format one reading short of any reachable tree: the
host tree carries no `osnoise/cpu<id>` file with either engine set, and
the five figures stand `not available` on the host of record. The
records left one question: re-anchor the parser to a tree that carries
the files, or take the trace-line surface the spec held out of scope.
Owner ruling G-28 approves the trace-line parsing as standing work of
this lane. This phase incorporates verified osnoise and timerlat
trace-line parsing into the figure read, and retires the count-file
read. Task ids: N13E-001 to N13E-004. Short name: fwq-example-n13e.

This phase adds no numbered spec, no option, no version reservation and
no record-layout change, and it replaces no earlier requirement, task
row, ruling or record of this file. It adds FR-027 to FR-030, SC-008 to
SC-011, D-8 to D-12, and the task rows N13E-001 to N13E-004 in
`tasks.md` in this phase's tasks pass. It rides the branch
`fix/fwq-example-n13e` of
`specs/045-merge-fixes` phase 15: one branch, one squashed commit, one
pull request, and one acceptance set at one head (BFW-012, N13E-004).
In this phase "the record" names
`specs/036-osnoise-timerlat-tracers/results/trace-line-thrawn.md`.

### Run-time binding (trace-line phase)

- Base: `origin/master` at `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`
  (version 0.34.0, report schema 1.16, harness `SOVERSION` 22, archive
  `SOVERSION` 8). The record layout stands: the five source figures
  keep their names and their optionality, so the schema, the version
  and the two `SOVERSION` numbers do not move. No public header
  changes.
- Branch: `fix/fwq-example-n13e`, the phase 15 branch of spec 045. The
  two lanes share one branch by the owner's order; no file stands in
  both lanes.
- Target host: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
  threads, kernel 7.3.0-5-cachyos. Acceptance is local (G-20). The
  acceptance runs need the rights a root shell carries for the tracefs
  writes, and the record names that need beside each run; a run
  without the rights stands BLOCKED with the refusal. The suite needs
  neither root nor a kernel tracer (FR-017 stands).
- Line numbers in the earlier sections of this file name the state at
  their own base. This phase finds every item by its symbol.

### The verified line forms (audit point)

The host of record, kernel 7.3.0-5-cachyos, read on 2026-11-09 with
each engine set, the tracer mask on one cpu, and tracing on. Both
engines write the per-cpu buffer
`sys/kernel/tracing/per_cpu/cpu<id>/trace`, the path the detector read
of specs/048 already reads.

The `osnoise` buffer carries one line per period, once per
`period_us`, the default 1,000,000 us standing read on the host of
record. The header legend names the nine figures after the timestamp:
RUNTIME IN US, NOISE IN US, percent OF CPU AVAILABLE, MAX SINGLE NOISE
IN US, then the interference counters HW, NMI, IRQ, SIRQ and THREAD.
The reading is positional: the five counters stand the last five
figures of a period line, and the legend labels stand as the kernel
printed them. The legend bytes stand beside the lines:

```text
#           TASK-PID      CPU# |||||   TIMESTAMP    RUNTIME      NOISE  AVAILABLE  IN US     HW    NMI    IRQ   SIRQ THREAD
       osnoise/1-2542397 [001] ....1 1241576.199778: 1000000       1659  99.83410      19      0      0   1006    176      4
       osnoise/1-2542397 [001] ....1 1241577.199779: 1000000       1403  99.85970       8      0      0   1000    147      0
```

The `timerlat` buffer carries one pair of activation lines per
activation, one per context:

```text
          <idle>-0       [001] d.h1. 1241586.794794: #1     context    irq timer_latency       334 ns
      timerlat/1-2542785 [001] ....1 1241586.794797: #1     context thread timer_latency      3448 ns
```

The re-confirmation of the count-file row stands repeated at this
read: `osnoise/per_cpu/cpu<id>/` carries `timerlat_fd` alone, and no
`osnoise/cpu<id>` file stands with either engine set. Setting
`current_tracer` clears the buffer, so the lines of a buffer stand the
run's own.

### Scope (trace-line phase)

In scope:

- `source/harness/noise_tracer.cpp` and
  `source/harness/detail/noise_tracer.hpp`: the figure read of the two
  standing engines.
- `test/source/harness_noise_test.cpp` and
  `test/source/harness_noise_report_test.cpp`: the fixture formats and
  the parsed-form cases.
- `specs/036-osnoise-timerlat-tracers/contracts/console-report.md`:
  the one amendment line naming the trace buffer as the source of the
  standing engines' figures.
- `specs/036-osnoise-timerlat-tracers/`: this `spec.md` phase section,
  the new `results/` record, the quickstart link row, `plan.md` and
  `tasks.md`. The standing records, data model, research rows and
  option contract keep their text as records.

Out of scope:

- The detector read of specs/048: the gap-line rule, the three
  detector figures and the settings line stand unchanged.
- The setup and restore order, the option grammar, the value parser,
  the record layout, the schema and version files, and every public
  header.
- Trace raw event codes (G-31) and automatic ranks (G-32): both stand
  future features.
- A tracer tuning key: the grammar still carries the engine and the
  cpu list alone.

### User scenarios (trace-line phase)

#### US-6 The tracer figures land filled on the host of record (Priority: P1)

An operator runs the pre-check with `--os-noise=osnoise` behind the
rights the writes need, and the tracer lines print the counts the
buffer carries. The same run with `timerlat` prints the two figures
that engine states and keeps the other three absent.

**Why this priority**: the lane shipped with every figure `not
available` on the host of record, and the question it left stands the
reason G-28 exists.

**Independent test**: the thrawn acceptance runs of FR-030 at the final
squashed head, and the fixture cases of FR-028.

**Acceptance scenarios**:

1. **Given** the final squashed head and the rights the writes need,
   **When** the osnoise acceptance run completes, **Then** the five
   source figures stand filled for every tracer cpu the run resolves,
   and the host tracing state stands restored.
2. **Given** the same shape with `timerlat`, **When** the run
   completes, **Then** the irq and thread figures stand filled, the
   other three stand the standing per-source wording, and the host
   state stands restored.

#### US-7 A maintainer pins the parsed forms without root (Priority: P1)

A maintainer runs the suite on a host with no root and no kernel
tracer, and the parsed forms stand proven through injected tracefs
text. No test sleeps, reads a clock or names a fixed cpu.

**Why this priority**: FR-017 and FR-019 are standing contracts of
this lane, and the coverage gate of VI runs this suite in every lane.

**Independent test**: `ctest --test-dir build -R 'harness_noise_test|harness_noise_report_test'` passes unrooted, and the coverage gate reads 100.0% lines at the final head.

**Acceptance scenarios**:

1. **Given** injected buffer text in the verified shapes, **When** the
   figure read folds it, **Then** the osnoise sums and the timerlat
   counts answer as the fixtures pin.
2. **Given** a buffer of header lines only, a buffer of malformed
   lines only, an empty buffer and a missing file, **When** the figure
   read folds them, **Then** no figure stands a guess, and the absence
   rule answers. A mixed buffer answers the sums of its parsed lines
   alone.

### Edge cases (trace-line phase)

- A buffer that carries header lines and no period line stands every
  figure of that cpu absent, the standing shape of a missing file.
- A period line whose nine figures answer no shape counts as no line,
  the standing rule that no figure stands a guess (R-07).
- A percent figure stands read as part of the line shape and answers
  no figure of its own.
- A `timerlat` buffer carries no nmi, softirq or hardware source, and
  those three figures stand absent for that engine alone.
- A serial pre-check writes no engine reset between windows (FR-009).
  The buffer stands cleared when the tracer is set, so the sums stand
  the totals of the lines the buffer holds at the read.
- A window shorter than the osnoise period holds no period line, and
  every figure stands absent the way a missing file answers. The
  acceptance rows name a window longer than the period for that
  reason.
- A host without the rights for the setup writes keeps the standing
  `not available` disclosure (FR-016), and the acceptance rows of
  FR-030 stand BLOCKED with the refusal.

### Requirements (trace-line phase)

- **FR-027** (N13E-001): The figure read of the two standing engines
  SHALL read the per-cpu trace buffer text at
  `sys/kernel/tracing/per_cpu/cpu<id>/trace`, resolved under the
  standing file-root seam, the path the detector read already reads,
  after the pre-check window ends and before the restore writes, the
  order the standing run keeps. A line whose first character stands
  `#` SHALL stand a header line and SHALL stand no period line and no
  activation line. For `osnoise`, a line SHALL stand one period line
  when its last nine whitespace-separated tokens answer the verified
  shape, digits, digits, digits-dot-digits, then six groups of digits,
  and the token ahead of them ends with the timestamp colon; a bare
  integer where the decimal figure stands answers no period line. The
  five interference-counter figures of the period lines SHALL sum into
  the record's hardware, nmi, irq, softirq and thread figures. For
  `timerlat`, a line SHALL stand one activation line when the tokens
  after the timestamp colon are exactly the six verified tokens
  `#<digits> context <word> timer_latency <digits> ns`, with nothing
  following; an `irq` word SHALL count the irq figure, a `thread` word
  SHALL count the thread figure, and any other word stands no line.
  The two counts stand independent: the lines answer no pairing rule. A line
  beside those shapes counts as no line, skipped, the standing rule
  that no figure stands a guess (R-07). A counter column that answers
  0 on every period line stands a filled 0, the shape the detector read
  answers for `nmiCount`. A buffer that carries no parsed line, stands
  missing, or stands unreadable stands every figure of that cpu
  absent. The sums stand the totals of the lines the buffer holds at
  the read. This reading narrows the cumulative-totals clause of
  FR-009 for the two standing engines: the figures answer the lines the
  buffer holds at the read. FR-009's clause stands the record of the
  retired read and keeps its text. The count-file read of
  `osnoise/cpu<id>` stands retired by
  this phase: its format never stood observed on a reachable tree, and
  the trace buffer stands the surface the host of record carries.
- **FR-028** (N13E-002): The parsed-form cases SHALL stand in
  `test/source/harness_noise_test.cpp` through injected tracefs text:
  the verified osnoise period line and timerlat activation line shapes
  byte for byte from the host read, header legend lines, malformed
  lines, a mixed buffer of parsed and malformed lines whose sums come
  from the parsed lines alone, a buffer with lines and no parsed line,
  an empty buffer, a missing file, a zero counter column, and the
  per-source absence of the three `timerlat` sources. The standing
  serial-window, `mode=both` and tracer-cpu-list rows,
  `theWindowLifecycleRows` and `theTracerCpuRows`, SHALL stand
  re-anchored from the
  retired count-file fixture to buffer text through the same seam, so
  the read-once-after-the-window and no-reset-between-windows proofs
  stay observable. Those rows SHALL append period lines to the buffer
  between reads and SHALL leave the earlier lines standing, so the sums
  cover every line the buffer holds and no reset can hide inside a
  rewrite. `test/source/harness_noise_report_test.cpp` builds
  its records by hand, so it SHALL gain one rendering row for the
  `timerlat` shape of two filled figures and three absent ones. No
  test SHALL sleep, read a clock or name a fixed cpu (FR-019). The
  root and tracefs needs of the acceptance runs SHALL stand explicit in
  the record, and a run without the rights SHALL stand BLOCKED with the
  command and the refusal.
- **FR-029** (N13E-003): One new record,
  `results/trace-line-thrawn.md`, SHALL carry both final-head runs of
  this phase: each acceptance command with its exit and its window,
  the figures the parse filled beside the CCD naming of the standing
  plan, the host tracing state each run found and the state it left,
  and the dated rows labeled preliminary with their pre-squash heads
  until the final head lands. The quickstart SHALL gain one link row
  to that record. The standing 2026-10-29 records keep their text as
  the record of what was decided then. The console contract SHALL carry
  the amendment that names the buffer path and the per-engine line
  reading in the same change as the code. The surfaces that keep
  naming the retired count files, the public header comment, the spec
  034 report-json line, this lane's option contract, this lane's
  report-json contract rows, the data model,
  the research rows and the quickstart wording, the specs/048 rows
  naming the retired path and symbol, listed as that lane's stale
  record, and the two standing
  edge-case rows of this file that name the count files and the
  cumulative-totals reading, stand listed in the record as stale
  wording this phase settles by ruling G-28; only the console contract
  and the quickstart link row move. The record SHALL repeat the audit
  point: the legend bytes, the verified lines, the `period_us` read and
  the per-cpu `buffer_size_kb` read, so the fixture bytes carry their
  host provenance. The record carries the per-cpu
  buffer size read beside each run's figures. No schema, version or
  `SOVERSION` value moves.
- **FR-030** (N13E-004): The shared acceptance set of spec 045 phase
  15 (BFW-012) SHALL run for this phase at the final squashed head,
  and the N13e rows SHALL join it: the osnoise and timerlat acceptance
  runs on thrawn behind the rights the writes need, each with a
  pre-check window that spans the one-second period the audit point
  names at least twice in host time, the window token named beside the
  host's time-stamp counter rate in the record: the probe's window
  counts raw counter ticks, so the standing 3,000 ms token spans 0.72 s
  on thrawn and holds no period line, the stated figures
  filled for every tracer cpu the run resolves, the host tracing state restored
  after each run to the saved word, the saved mask and `tracing_on` 0
  with the found reads named beside them, and the unrooted suite pass
  recorded apart from them.

### Success criteria (trace-line phase)

- **SC-008**: With an injected tracefs root, the verified osnoise
  fixture text answers the five figures as the sums of the counter
  columns over its period lines, and the verified timerlat fixture
  text answers the irq and thread counts with the other three figures
  absent. A mixed buffer answers the sums of its parsed lines alone.
  A zero counter column answers a filled 0. A buffer of header lines
  only, a buffer of malformed lines only, an empty buffer and a missing
  file answer the absence rule.
- **SC-009**: The suite passes with no root and no kernel tracer, and
  no test sleeps, reads a clock or names a fixed cpu.
- **SC-010**: The thrawn acceptance run of each standing engine fills
  its stated figures for every tracer cpu the run resolves, at the
  final squashed head with the window FR-030 names: every figure of the
  filled set of every tracer cpu reads filled. The record names the
  resolved cpu list and the per-cpu buffer size read, and the state
  each run found and the state it left: the saved word, the saved mask
  and `tracing_on` 0, beside the figures and the CCD.
- **SC-011**: The coverage gate at the final head reads 100.0% lines
  with the standing branch allowance, at or below 84 markers and no
  new marker, and the retired count-file reader leaves no cold row.

### Local acceptance (trace-line phase)

The shared lane set stands in spec 045 phase 15. The N13e rows stand
here, run at the final squashed head on thrawn:

```
sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck,precheck=3000 --os-noise=osnoise --json=$HOME/scratch/fwq-example-n13e/osnoise.json
sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck,precheck=3000 --os-noise=timerlat --json=$HOME/scratch/fwq-example-n13e/timerlat.json
ctest --test-dir build -R 'harness_noise_test|harness_noise_report_test'
```

The window key stands required: the osnoise period line arrives once
per period, one second on the host of record, and the standing default
window of 200 ms would hold no period line at all. Each sudo run names
the tracing state the run found and the state it left, the same three
reads the standing records carry. The unrooted suite row stands apart,
and it answers on the standing tree.

### Commit and record rules (trace-line phase)

The rules of spec 045 phase 15 govern the branch: one squashed commit,
the two `Refs:` trailers naming both lanes, author and committer `OmO`,
the push protocol with the lease, and the measured head named in the
pull-request body. The interim rows stand in this lane's record file.

### Decisions (trace-line phase)

- **D-8 (the trace-line surface)**: G-28 chose the surface the R-3
  alternatives carried. The count-file format stands pinned from
  documentation and never stood read on a reachable tree, and the
  per-cpu buffer stands the surface the host of record carries.
- **D-9 (one buffer path)**: the two standing engines read the same
  per-cpu trace path the detector read reads. One seam, one path, one
  read per engine, with the line rule keyed on the engine.
- **D-10 (timerlat states two sources)**: the activation lines name
  the irq and thread contexts. The engine states nothing for nmi,
  softirq and hardware, and those three figures stand absent for that
  engine alone, the per-source rule of FR-010.
- **D-11 (totals of the held lines)**: setting `current_tracer` clears
  the buffer, a read of the host of record, so the sum of the period
  lines stands the total since the tracer's start for the lines the
  fixed-size buffer still holds at the read. The record names the
  3,000 ms acceptance window and the per-cpu buffer size read beside the
  figures, so a reader can see the held lines cover the window.
- **D-12 (the count-file reader retires)**: X.3 removes orphans a
  change creates. The buffer read answers every host the lane can
  reach, so the count-file reader, its fixture format and its cases
  stand retired in this change. The standing records of the 2026-10-29
  runs keep their text as the record of what was decided then.

### Clarifications (trace-line phase)

#### Round 1, 2026-11-09

The clarification pass read the phase against the standing reader, the
fixture homes, the standing records and the audit point.

- Q: Can the acceptance commands as written fill the osnoise figures?
  (FR-030, local acceptance) → A: they could not. The standing default
  window is 200 ms and the osnoise period line arrives once per second
  on the host of record, so the buffer would hold no period line. The
  commands carry `precheck=3000`, and the record names the window.
- Q: Does a malformed line poison the cpu or stand skipped? (FR-027,
  SC-008) → A: skipped. The buffer carries heterogeneous lines, and
  the standing detector read already skips lines beside its shape. The
  absence rule answers only a buffer with no parsed line, a missing
  file and an unreadable file. A mixed-buffer fixture stands required,
  so the sums prove the skip.
- Q: What exactly pins a period line and a header line? (FR-027) → A:
  a header line stands `#` at column 1, the mirror of the detector's
  column rule. A period line answers the nine-token verified shape at
  the line tail, integer, integer, decimal, then six integers, with
  the token ahead ending at the timestamp colon. No task-name or cpu
  field check stands added.
- Q: Does a counter column reading 0 on every line stand filled or
  absent? (FR-027) → A: filled 0, the shape the detector read answers
  for `nmiCount`. The verified host line carries HW 0 and NMI 0.
- Q: Where does the read land against the restore? (FR-027) → A: after
  the window ends and before the restore writes, the standing order of
  FR-009, now pinned in the requirement. The re-anchored serial rows
  observe it.
- Q: What happens to the serial-window and `mode=both` rows that rode
  the retired count-file fixture? (FR-028) → A: they stand re-anchored
  to buffer text through the same seam. They carry the only observable
  proof of the read-once-after-the-window and no-reset rules.
- Q: Do the sums stand cumulative totals forever? (D-11) → A: they
  stand the totals of the lines the fixed-size buffer holds at the
  read. The record names the window, and the standing window keeps the
  buffer whole.
- Q: What does the contract amendment carry, and what stale wording
  stays? (FR-029) → A: the console contract carries the buffer path
  and the per-engine reading. The public header comment, the spec 034
  report-json line, the option contract, the data model, the research
  rows and the quickstart wording keep their text, stand listed in the
  record as stale wording, and only the console contract and one
  quickstart link row move.
- Q: What pins the timerlat activation line? (FR-027) → A: the strict
  token order after the timestamp colon, `#<digits> context <word>
  timer_latency <digits> ns`. Any other context word stands no line.
  The two counts stand independent; the observed pairing stands no
  rule.
- Q: Which record carries the phase, and do the task rows exist?
  (FR-029) → A: one new record, `results/trace-line-thrawn.md`, carries
  both final-head runs, and the quickstart gains one link. The four
  N13E rows land in `tasks.md` in the tasks phase.
- Q: Which test file carries the parsed forms? (FR-028) → A:
  `harness_noise_test.cpp` alone, the file with the seam and the
  fixtures. The report test builds records by hand, so it gains one
  rendering row for the two-filled, three-absent `timerlat` shape.
- Q: What does "restored" name? (FR-030, SC-010) → A: the saved word,
  the saved mask and `tracing_on` 0, with the found reads named beside
  them, the shape the standing records carry.

#### Round 2, 2026-11-09

The second pass walked the amended token rules against the audit-point
lines byte for byte and closed ten wording rows. The rules accept all
four verified lines: the osnoise line splits to thirteen tokens, the
last nine answer the pinned shape behind the timestamp-colon anchor,
and the five counters read into the five record figures.

- Q: Does a malformed line poison or skip? (SC-008, US-7) → A: skip.
  The absence rule stands a property of buffers: header-only,
  malformed-only, empty and missing buffers answer absence, and a mixed
  buffer answers the sums of its parsed lines alone.
- Q: Can a planted line that starts with `#` read as a period line?
  (FR-027) → A: it cannot: the header rule takes precedence, a `#`
  line stands no period line and no activation line. The audit point
  carries the legend bytes the host printed.
- Q: Where does the acceptance window get its bound? (FR-030) → A: the
  audit point names the one-second period, and FR-030 names the
  3,000 ms window against it.
- Q: Can a serial row's rewrite be mistaken for the reset it proves
  absent? (FR-028) → A: it cannot: the rows append period lines between
  reads and answer sums over every held line.
- Q: What keeps the ring from truncating the window? (D-11, SC-010) →
  A: the record names the window and the per-cpu buffer size read
  beside the figures.
- Q: Which stale rows of this file stand listed? (FR-029) → A: the two
  standing edge-case rows that name the count files and the
  cumulative-totals reading join the stale-wording list.
- Q: What is a decimal figure? (FR-027) → A: digits-dot-digits. A bare
  integer where the percent figure stands answers no period line.
- Q: May fields follow `ns`? (FR-027) → A: they may not: exactly the
  six verified tokens follow the timestamp colon.
- Q: Do the N13E task rows exist yet? (FR-029) → A: the tasks phase
  writes them into `tasks.md` before implement, as the round-1 answer
  states.
- Q: Which cpus do SC-010 and US-6 name? (SC-010) → A: every tracer
  cpu the run resolves; the record names the resolved list.

#### Round 3, 2026-11-09

The third pass checked the amended rows against the round-2 answers and
found that five answers had not landed in the rows they name. They
stand landed now: SC-008 and US-7 name buffers for the absence rule and
US-7 carries the mixed-buffer reading; SC-010 names every tracer cpu,
the resolved list and the buffer size read; D-11 names the 3,000 ms
window and the capacity read; FR-029 lists the two standing edge rows
and the capacity read; FR-030 names the 3,000 ms window. The audit
point adds that the reading is positional, the five counters standing
the last five figures of a period line while the legend labels stand as
printed. The pass re-verified the token rules against the four
audit-point lines byte for byte, and the phase stands converged.

### Assumptions (trace-line phase)

- thrawn is the host of record with the tool versions spec 045 phase
  15 records, and a root shell stands reachable for the acceptance
  writes at the live check of 2026-11-09.
- The verified line forms stand read on the host of record on
  2026-11-09, and a kernel that prints other shapes answers the
  absence rule, the standing shape of a host the format does not
  match.
- The owner rulings G-20, G-24, G-25 and G-28 sit outside this
  repository; the order of 2026-11-09 carries their operative content.

### Open, owner (trace-line phase)

- G-31 (raw event codes) and G-32 (automatic ranks) stand future
  features, unchanged.
- The count-file question the 2026-10-29 records left stands closed by
  G-28 and this phase.
