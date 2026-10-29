# Research: Osnoise and Timerlat Tracer Integration

Phase 0 output of `/speckit.plan` for
`specs/036-osnoise-timerlat-tracers/spec.md`. Every technical unknown
the plan could carry stands resolved here; each decision names the
Decision, the Rationale and the Alternatives considered. A fact whose source stands
outside this repository stands labeled with that source; the facts
the repository itself carries stand unlabeled. no web access stood available in this session,
and the lane host of this session carries no tracefs rights, so the
tracefs facts stand on documentation knowledge, with the
re-confirmation path each one names.

## R-1: the tracefs root and the relative paths

- **Decision**: every tracefs access resolves
  `procRootPath("sys/kernel/tracing/<file>")`: the standing file-root
  seam of specs/034 (`source/harness/detail/proc_root.hpp`) with the
  relative path the tracing tree carries under `/`. The production root
  stands `/` (the seam default), so the reads name
  `/sys/kernel/tracing/<file>`; the tests install a fake tree.
- **Rationale**: the seam is the named reuse point the spec fixes (spec
  FR-007, D-1, and the seam contract of specs/034 FR-019, whose own
  contract names the N13e lane). `/sys/kernel/tracing` stands the
  kernel's documented default tracefs mount point (kernel documentation,
  `Documentation/filesystems/tracefs.rst` and the tracing HOWTO), so the
  standing path needs no search; a host that mounts tracefs elsewhere
  stands not available, the reading the spec's Assumptions section
  fixes. `std::filesystem` joins no dependency.
- **Alternatives considered**: a second seam dedicated to tracefs (the
  spec forbids a second seam, D-1); a mount-point search over
  `/proc/mounts` (the spec forbids it, and X.2 rejects the machinery
  for a path the kernel documents); a `tracefs` library dependency
  (a new hard runtime dependency the Additional Constraints forbid
  without justification the spec does not carry).

## R-2: the engine selection and the control files

- **Decision**: the run selects the engine by writing the word
  `osnoise` or `timerlat` to `current_tracer` at the root of the
  tracing tree, restricts it by writing the mask of the tracer cpus to
  `tracing_cpumask`, and starts and stops it with `tracing_on` at `1`
  and `0`. The write of the engine word stands the availability probe.
- **Rationale**: both engines stand trace-array tracers of the tracing
  tree, selected through `current_tracer`; the osnoise and timerlat
  tracer pages of the kernel documentation enable them with exactly
  this sequence (`echo osnoise > tracing/current_tracer`, then
  `tracing_on`), and `tracing_cpumask` bounds the cpus the tracer's
  per-cpu threads run on: the osnoise tracer runs one sampling thread
  per cpu of `tracing_cpumask`, and a mask write stops and starts the
  threads the new mask names (kernel documentation,
  `Documentation/trace/osnoise-tracer.rst`,
  `Documentation/trace/timerlat-tracer.rst`, read from knowledge;
  confidence high for the three file names and the write sequence).
  The write stands the probe because a host that lacks the mount, the
  engine or the rights answers a failed write on the first touch, the
  state the disclosure names (spec D-5); a failed `current_tracer`
  write leaves the standing word unchanged, so the probe costs nothing
  beside the restore.
- **Alternatives considered**: reading `available_tracers` as the
  probe (the file names the registered tracers, and a built-as-module
  engine absent from the list stands indistinguishable from a present
  one the rights refuse; the write decides what the run needs); an
  `osnoise/cpus` file (a per-engine cpu file the documentation memory
  of this session cannot pin; the standing `tracing_cpumask` covers
  the restriction for both engines, so the second file buys nothing);
  cgroup or cpuset restriction (a host-state change the run must not
  make).

## R-3: the count files and the five sources

- **Decision**: the figures stand read from the per-cpu count files
  the `osnoise` directory of the tracing tree carries: one file per
  cpu, named `osnoise/cpu<id>`, each file stating the cumulative
  counts of the noise sources of that cpu. The five sources stand the
  NMI, IRQ, softirq, thread and hardware counts; the kernel's own
  word for the last one stands `hw`, and the reader maps it to the
  record's `hardware` figure. Both engines share the directory: the
  timerlat tracer rides the osnoise counters machinery, so the same
  per-cpu files answer both engine words.
- **Rationale**: the spec fixes the figures as the engine's cumulative
  totals read once after the measurement ends (spec FR-009) and the
  sources as the five the engine reports (spec FR-010), and the
  Out-of-scope section excludes the trace lines, so the figures ride
  files. The osnoise tracer's counters name exactly the five sources
  the spec lists, NMI, IRQ, softirq, thread and hardware (kernel
  documentation, `Documentation/trace/osnoise-tracer.rst`; the trace
  event the tracer publishes carries the same five fields beside the
  noise total).
- **Confidence note**: the `osnoise` directory, the five source names
  and the cumulative, per-cpu shape stand documentation knowledge of
  this session; the exact file spelling `osnoise/cpu<id>` and the
  per-line format of the counts stand one reading short of the tree.
  The implementation re-confirms both against the lane host's tree,
  where the acceptance runs of spec D-7 stand the engine up anyway;
  a mismatch goes to the pull request as a question, the pattern
  specs/034 R-03 set for the schedstat mapping. The plan binds the
  file name and the line format to that re-confirmation, and the
  tests bind the parser to the fixture format the re-confirmation
  pins.
- **Alternatives considered**: parsing the tracer's trace-buffer lines
  (the spec excludes it, Out of scope); reading the trace event
  format files under `events/osnoise/` (a second surface the count
  files make unnecessary); a per-window read (the spec fixes one
  read after the measurement ends, FR-009).

## R-4: the version bounds

- **Decision**: the run stands no version check. The osnoise tracer
  exists since Linux 5.13 and the timerlat tracer since Linux 5.17
  (kernel documentation, the tracer pages; confidence high); the
  per-cpu count files exist in the kernels the lane hosts run, and
  the availability probe of R-2 plus the per-source absence rule of
  spec FR-010 absorb every shape a host lacks.
- **Rationale**: a version check would read a version file or parse a
  release string the tracing tree does not standardize, and the files
  the run touches already answer the only question the run asks:
  can this host carry the engine (spec D-5). A missing file, an
  unwritable file and a file whose figure stands absent each stand
  the disclosure or the null the spec fixes, so the bounds stand
  unobservable at run time.
- **Alternatives considered**: a minimum-kernel gate in the option
  parser (a fixed bound the spec forbids: no host state refuses a
  run, FR-016); a per-file feature table keyed on the kernel version
  (machinery X.2 rejects: the probe answers it).

## R-5: the setup order and the restore

- **Decision**: the setup reads the standing `current_tracer` word and
  the standing `tracing_cpumask` text, writes the engine word, writes
  the tracer mask rendered in the word width the standing mask
  carried, and writes `tracing_on` at `1`. After the figures stand
  read, the run writes `tracing_on` at `0`, restores the standing
  engine word and the standing mask, and answers nothing on a restore
  failure. Every step runs on the entry-point thread, outside every
  timed window.
- **Rationale**: the read-before-write shape gives the restore its
  values and gives the mask render its width, so no width stands
  fixed (the `tracing_cpumask` word width varies with the cpu count;
  the kernel accepts the width the file itself prints). The restore
  leaves the host as the run found it: a tracer left running would
  carry the perturbation the option's own doc note names across every
  later run on the host, and the lane host stands exclusive during a
  run (spec VII). The run never fails on the tracer (spec FR-016), so
  a restore failure stands disclosed by nothing and refused by
  nothing.
- **Alternatives considered**: leaving the tracer set after the run
  (the perturbation note would bind the whole host afterwards); a
  teardown that deletes engine state the run did not create (the run
  owns only the three writes it made).

## R-6: the lifecycle point and the `mode=both` stop

- **Decision**: the entry-point thread stands the engine up before the
  probe items are dispatched, and reads the figures and tears the
  engine down after `JitterProbe::start()` returns. `start()` returns
  only behind the latch line, which every item reaches after its
  pre-check phase (`source/harness/jitter_probe.cpp`, the latch-line
  comment), so the figures stand complete at the read in both modes.
- **Rationale**: the spec fixes the order "set before the pre-check
  measurement starts, read after the measurement ends" (spec FR-009,
  D-2), and the standing synchronization gives both points for free:
  the dispatch stands the set point, the latch stands the end point.
  The buffer touch and the measurement pass run beside the started
  tracer; the figures are the cumulative totals since the tracer's
  start (FR-009), so the extra counts name no figure the record
  states and no reset write joins (FR-009 forbids one between serial
  windows). In `mode=both` the items leave the latch into the
  alongside phase while the stop write lands on the entry thread; the
  default candidate set stands outside the planned cpus (R-8), so the
  alongside figures of the planned cpus stand unperturbed, and a
  tracer cpu a `cpus=` list names carries the perturbation note the
  docs state (spec FR-021).
- **Alternatives considered**: a second barrier round between the
  pre-check phase and the alongside phase, with the holder stopping
  the tracer between them (a new synchronization point beside the
  proven ones, for a window the default cpu set already closes); a
  per-item stop (file I/O inside the pinned work item, the shape the
  probe loop forbids); starting the tracer after the measurement pass
  (a synchronization point inside the item's flow the standing
  barriers do not carry).

## R-7: the figure read and the per-source absence

- **Decision**: the read answers one entry per tracer cpu, in the
  session's cpu order, each entry carrying the five figures as
  `std::optional<std::uint64_t>`; a cpu whose file stands missing,
  unreadable or unparsable stands the entry with every figure absent;
  a source the file states no figure for stands that figure absent
  alone (spec FR-010). The read never fails the run.
- **Rationale**: the spec fixes the per-source `not available` rule
  for a source the engine states no figure for (FR-010) and the
  per-cpu shape for a cpu whose files state nothing at read time
  (spec Edge Cases). The optional figure stands the record's whole
  availability answer, the file form of the console wording, the
  pattern the attribution deltas of specs/034 already carry (the
  `null` of `contracts/report-json.md` there).
- **Alternatives considered**: a per-cpu availability flag beside the
  figures (the five optionals carry the same fact one level finer);
  an error return beside the figures (the run never fails on the
  tracer, FR-016).

## R-8: the tracer cpus

- **Decision**: the `cpus=` key names the tracer cpus; an absent key
  names the pre-check's candidate set, the cpus the measured set
  names `candidate`, one per L3 group the plans touch. The list runs
  as named, a planned cpu included, with no overlap refusal, and the
  docs carry the perturbation note beside the option (spec FR-021).
- **Rationale**: the spec fixes both the default (FR-008) and the
  no-refusal reading (the clarify answer of 2026-10-29, Q3: the value
  stands an explicit user choice the run can honor, and the standing
  overlap refusal binds the auto-resolved alongside probe, a
  different surface). The candidate set stands the measured set's
  own answer, so the resolver stands reused unchanged (X.3), and the
  default keeps the tracer's own load off the planned cpus, the fact
  the strict gate and the alongside phase stand on (D-2).
- **Alternatives considered**: refusing the overlap (the clarify
  answer fixes the opposite); intersecting the list with the
  candidates (a silent edit of an explicit choice); resolving the
  default from the `idle` preset again (the measured set already
  carries the answer).

## R-9: the missing-window refusal

- **Decision**: the check stands beside the standing combination check
  of the hunt family (`validateHuntCombination`, called from
  `source/harness/cli.cpp` after the gather and ahead of every run):
  a settled `--os-noise` value beside a batch that carries no
  `--jitter-probe` option, or one whose mode stands `ALONGSIDE`,
  prints one line and exits 2. The check reads the batch's probe
  options alone.
- **Rationale**: the spec fixes the refusal for the two shapes (FR-011,
  the clarify answer on Q5: the same one-line refusal covers a run
  that carries no `--jitter-probe` at all), and the standing
  combination check stands the house shape for a cross-option rule
  that needs two settled values. The pre-check record the tracer
  feeds stands the batch's alone, so a family-level probe stands
  outside the check: the batch carries no probe, and the refusal
  stands (spec D-2: the option names a measurement across a window
  the run does not carry). The compare mode carries no probe, so it
  refuses the same way.
- **Alternatives considered**: the refusal inside the value parser
  (the parser settles one row and reads no other row's mode); a
  silent ignore beside a probe-free run (the clarify answer fixes
  the refusal); a warning line beside `mode=alongside` (the clarify
  answer fixes exit status 2).

## R-10: the public fields

- **Decision**: `JitterProbeOptions` gains
  `TracerEngine tracerEngine = TracerEngine::NONE;` and
  `std::vector<std::uint32_t> tracerCpus = {};`, appended after
  `precheckSerial`; `TracerEngine` stands the closed enum `NONE`,
  `OSNOISE`, `TIMERLAT`. `PrecheckRecord` gains
  `std::optional<PrecheckTracerRecord> tracer;` appended after
  `skippedGroups`.
- **Rationale**: the aggregate append stands source-compatible for
  standing callers, the pattern both prior probe growths followed
  (`precheckSerial` of specs/034); the enum closes the engine set at
  the type level the way `JitterProbeMode` closes the modes, and the
  empty-vector-means-default rule stands the struct's own
  (`cpus` stands the same way for the `idle` preset, specs/027).
  The optional record stands the one shape a tracer-free run and a
  not-available run leave empty (spec FR-015), the way
  `JitterReportRecord` stands optional on the writer.
- **Alternatives considered**: a `std::string` engine field (an open
  set the struct bounds nowhere, and the enum word table the writer
  needs stands closed anyway); a sentinel inside `tracerCpus` (the
  empty vector already names the default); a separate options struct
  for the tracer (a second carrier beside the standing one, and the
  suite entry point binds the probe options whole).

## R-11: the JSON key mechanics

- **Decision**: the `precheck` object of `docs/schema/report-v1.json`
  gains the `tracer` key in its `properties` and in its `x-order`
  (after `skippedGroups`) and stands outside its `required` list; the
  top-level `x-order` stands unchanged; `kSchemaVersion` moves to
  `1.7`.
- **Rationale**: the standing checker (`checkNode`,
  `source/harness/json_load.cpp`) checks a key of `properties` only
  when the document carries it, compares only the keys the document
  carries against `x-order`, and refuses any key a closed object
  (`additionalProperties: false`) does not list; the three lists
  together stand the whole optionality mechanics, and the
  `precheck` object stands closed. The per-benchmark `hunt` object
  stands the standing precedent: it joins `x-order` without joining
  `required`, and `loadReport` reads it with `optionalField`, the
  reader the tracer record reuses. The top-level list stays because
  the record stands inside the `precheck` object (spec FR-006), and
  the minor moves because the record gains keys the reader must
  carry (spec D-3).
- **Alternatives considered**: a second top-level object (the spec
  fixes the record inside the pre-check record, FR-014); a `required`
  entry beside a null-valued always-present key (a `null` the
  tracer-free run would write, against the byte identity of FR-015);
  a schema `oneOf` (the checker reads kinds, key sets and order
  alone).

## R-12: the byte-identity reading

- **Decision**: the byte identity of spec FR-015 binds the JSON in
  both cases (a tracer-free run and a not-available run write no
  `tracer` key, so the document stands byte-identical to master's
  apart from the `schemaVersion` value and the version figures the
  document carries) and binds the standing
  console lines; the not-available case prints the tracer lines with
  the `not available` wording, the disclosure spec FR-016 and SC-004
  fix.
- **Rationale**: spec FR-016 and User Story 2 fix the disclosure on
  the tracer lines of a not-available run, so the console of that
  run stands master's block plus the disclosure lines; the reading
  states both requirements at once, each fact its own sentence, the
  way XI.2 asks a load-bearing distinction carried.
- **Alternatives considered**: a not-available run that prints no
  tracer line (contradicts spec FR-016 and SC-004); a not-available
  run that writes a `tracer` key with null figures (against the byte
  identity of FR-015 and the optional-record shape of D-3).

## R-13: the console lines

- **Decision**: `printPrecheckReport` prints the tracer disclosure
  after the host-settings lines and before the warning lines: one
  header line naming the engine and the cpus, then one figure line
  per source per tracer cpu, the sources in the NMI, IRQ, softirq,
  thread, hardware order, cpus ascending, each absent figure standing
  the `not available` wording. The not-available case stands one
  line, the tracer and the disclosure.
- **Rationale**: the host-settings lines and the tracer lines stand
  the run-level disclosures of the block, and the warning and
  pinning lines stand the rule lines that read on the figures; the
  disclosure region stands ahead of the rule region, the order the
  standing block already carries (the host settings precede the
  warnings). One line per (cpu, source) pair stands the shape spec
  FR-013 words ("one figure line per source per tracer cpu").
- **Alternatives considered**: one line per cpu with the five
  figures (five figures on one line stands against the wording FR-013
  fixes); the tracer lines before the host settings (the standing
  block's disclosure order would name no reason for the move); the
  tracer lines after the pinning line (the rule lines would stand
  between two disclosure regions).

## R-14: the evidence records

- **Decision**: the acceptance runs on thrawn stand recorded under
  `specs/036-osnoise-timerlat-tracers/results/`, one record per run,
  named for the engine (`osnoise.md`, `timerlat.md`), beside a
  `plan.md` that states the command set, the pattern of
  `specs/035-numa-placement/results/`. A script records the runs; no
  CI gate stands on the records; the quickstart links every record.
- **Rationale**: the spec fixes the shape (FR-020, D-7): the
  commands, the exit codes, the engine and the per-CCD tables, one
  probe cpu per L3 group, each figure naming the CCD and the engine,
  the CCD names read from the `--topology` listing the same host
  prints. The specs/035 records stand the house precedent the spec
  names.
- **Alternatives considered**: CI-run acceptance (the lane host's
  CCD shape stands the acceptance axis, and CI carries no such
  host); a single combined record (the spec fixes one record per
  run).

## Unresolved items

- The exact spelling of the per-cpu count files (`osnoise/cpu<id>`)
  and the per-line format of the counts stand re-confirmed against the
  lane host's tree at implementation (R-3); the acceptance runs need
  the engine anyway, so the confirmation rides them, and a mismatch
  goes to the pull request as a question. No design and no test
  contract stands blocked: the tests bind the parser to the fixture
  format the re-confirmation pins, and the contract files pin the
  record shape, which the file format stands upstream of.
- The `tracing_cpumask` word width stands rendered from the standing
  mask the setup reads (R-5), so no width stands fixed and no
  re-confirmation stands needed.
